/* Copyright (c) 2019-2021, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "platform.h"

#include <algorithm>
#include <ctime>
#include <mutex>
#include <vector>

#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "common/logging.h"
#include "platform/filesystem.h"
#include "platform/parsers/CLI11.h"
#include "platform/plugins/plugin.h"

namespace vkb
{
std::vector<std::string> Platform::arguments = {};

std::string Platform::external_storage_directory = "";

std::string Platform::temp_directory = "";

PositionalCommand Platform::app("sample", "Start a sample with the given id");
SubCommand        Platform::samples("samples", "List all samples", {});
FlagCommand       Platform::sample(FlagType::OneValue, "sample", "s", "Start a sample --sample/--s ID");
FlagCommand       Platform::test(FlagType::OneValue, "test", "t", "Start a test --test/--t ID");
FlagCommand       Platform::benchmark(FlagType::OneValue, "benchmark", "", "Run a benchmark for a set amount of frames");
FlagCommand       Platform::width(FlagType::OneValue, "width", "", "Set the window width --width WIDTH");
FlagCommand       Platform::height(FlagType::OneValue, "height", "", "Set the window height --height HEIGHT");
FlagCommand       Platform::headless(FlagType::FlagOnly, "headless", "", "Run in headless mode --headless");
FlagCommand       Platform::batch_categories(FlagType::ManyValues, "category", "c", "A category to run in batch mode, --category={api,performance,extensions}");
FlagCommand       Platform::batch_tags(FlagType::ManyValues, "tag", "t", "A tag to run in batch mode, --tag={any,Arm}");
SubCommand        Platform::batch("batch", "Run multiple samples", {&Platform::batch_categories, &Platform::batch_tags});

bool Platform::initialize(const std::vector<Plugin *> &plugins = {})
{
	auto sinks = get_platform_sinks();

	auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());

#ifdef VKB_DEBUG
	logger->set_level(spdlog::level::debug);
#else
	logger->set_level(spdlog::level::info);
#endif

	logger->set_pattern(LOGGER_FORMAT);
	spdlog::set_default_logger(logger);

	LOGI("Logger initialized");

	auto args = get_arguments();
	args.insert(args.begin(), "vulkan_samples");
	parser = std::make_unique<CLI11CommandParser>("vulkan_samples", "Vulkan Samples\n\nA collection of samples to demonstrate the Vulkan best practice.\n\nUse [SUBCOMMAND] --help for specific subcommand information\n\n", args);

	CommandGroup window_options("Window Options", {&Platform::width, &Platform::height, &Platform::headless});

	if (!parser->parse({&Platform::app, &Platform::sample, &Platform::test, &Platform::batch, &Platform::samples, &Platform::benchmark, &window_options}))
	{
		auto help = parser->help();
		LOGI("");
		for (auto &line : help)
		{
			LOGI(line);
		}
		LOGI("");
		return false;
	}

	parser = std::make_unique<Parser>(plugins);

	// Process command line arguments
	if (!parser->parse(arguments))
	{
		return false;
	}

	OptionalProperties properties;

	// Subscribe plugins to requested hooks and store activated plugins
	for (auto *plugin : plugins)
	{
		OptionalProperties requested_properties;
		if (plugin->activate_plugin(this, *parser.get(), &requested_properties))
		{
			properties = combine<OptionalProperties>(properties, requested_properties);

			auto &plugin_hooks = plugin->get_hooks();
			for (auto hook : plugin_hooks)
			{
				auto it = hooks.find(hook);

				if (it == hooks.end())
				{
					auto r = hooks.emplace(hook, std::vector<Plugin *>{});

					if (r.second)
					{
						it = r.first;
					}
				}

				it->second.emplace_back(plugin);
			}

			active_plugins.emplace_back(plugin);
		}
	}

	// Sort out stuff here
	create_window({}, {});

	if (!window)
	{
		LOGE("Window creation failed!");
		return false;
	}

	if (!app_requested())
	{
		LOGE("An app was not requested, can not continue");
		return false;
	}

	return true;
}

bool Platform::prepare()
{
	return true;
}

void Platform::main_loop()
{
	while (!window->should_close())
	{
		try
		{
			// Load the requested app
			if (app_requested())
			{
				if (!start_app())
				{
					throw std::runtime_error{"Failed to load Application"};
				}

				// Compensate for load times of the app by updating a single frame
				// TODO: Setting the framerate to 60 fps isnt a long term fix. There should be a way of disabling features of app from running on the first frame possibly a skip_frame flag in Application::update()
				timer.tick<Timer::Seconds>();
				active_app->update(0.01667f);
			}

			update();

			window->process_events();
		}
		catch (std::exception e)
		{
			LOGE("{}", e.what());
			LOGE("Failed when running application {}", active_app->get_name());
			LOGI("Attempting to continue");
			call_hook(Hook::OnAppError, [this](Plugin *plugin) { plugin->on_app_error(active_app->get_name()); });
			if (!app_requested())
			{
				LOGI("No application queued");
				// TODO: There is definitely a better way of managing errors created by several applications. Currently out of scope
				// Propogate last error
				throw e;
			}
		}
	}
}

void Platform::update()
{
	auto delta_time = static_cast<float>(timer.tick<Timer::Seconds>());

	if (focused)
	{
		call_hook(Hook::OnUpdate, [&delta_time](Plugin *plugin) { plugin->on_update(delta_time); });

		if (render_properties.use_fixed_simulation_fps)
		{
			delta_time = render_properties.fixed_simulation_fps;
		}

		active_app->update(delta_time);
	}
}        // namespace vkb

std::unique_ptr<RenderContext> Platform::create_render_context(Device &device, VkSurfaceKHR surface) const
{
	auto context = std::make_unique<RenderContext>(device, surface, window->get_width(), window->get_height());

	context->set_surface_format_priority({{VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	                                      {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	                                      {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	                                      {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}});

	context->request_image_format(VK_FORMAT_R8G8B8A8_SRGB);

	context->set_present_mode_priority({
	    VK_PRESENT_MODE_MAILBOX_KHR,
	    VK_PRESENT_MODE_FIFO_KHR,
	    VK_PRESENT_MODE_IMMEDIATE_KHR,
	});

	context->request_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);

	return std::move(context);
}

void Platform::terminate(ExitCode code)
{
	if (code == ExitCode::UnableToRun)
	{
		parser->print_help();
	}

	if (active_app)
	{
		std::string id = active_app->get_name();

		call_hook(Hook::OnAppClose, [&id](Plugin *plugin) { plugin->on_app_close(id); });

		active_app->finish();
	}

	active_app.reset();
	window.reset();

	spdlog::drop_all();

	call_hook(Hook::OnPlatformClose, [](Plugin *plugin) { plugin->on_platform_close(); });
}

void Platform::close() const
{
	if (window)
	{
		window->close();
	}
}

void Platform::request_properties(PlatformProperties properties)
{
}

void Platform::call_hook(const Hook &hook, std::function<void(Plugin *)> fn) const
{
	auto res = hooks.find(hook);
	if (res != hooks.end())
	{
		for (auto plugin : res->second)
		{
			fn(plugin);
		}
	}
}

void Platform::set_focus(bool focused)
{
	this->focused = focused;
}

void Platform::set_window(std::unique_ptr<Window> &&window)
{
	this->window = std::move(window);
}

const std::string &
    Platform::get_external_storage_directory()
{
	return external_storage_directory;
}

const std::string &Platform::get_temp_directory()
{
	return temp_directory;
}

float Platform::get_dpi_factor() const
{
	return window->get_dpi_factor();
}

Application &Platform::get_app()
{
	assert(active_app && "Application is not valid");
	return *active_app;
}

Application &Platform::get_app() const
{
	assert(active_app && "Application is not valid");
	return *active_app;
}

Window &Platform::get_window() const
{
	return *window;
}

std::vector<std::string> &Platform::get_arguments()
{
	return Platform::arguments;
}

void Platform::set_arguments(const std::vector<std::string> &args)
{
	arguments = args;
}

void Platform::set_external_storage_directory(const std::string &dir)
{
	external_storage_directory = dir;
}

void Platform::set_temp_directory(const std::string &dir)
{
	temp_directory = dir;
}

void Platform::input_event(const InputEvent &input_event)
{
	if (properties.process_input_events && active_app)
	{
		active_app->input_event(input_event);
	}
}

std::vector<spdlog::sink_ptr> Platform::get_platform_sinks()
{
	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	return sinks;
}

bool Platform::app_requested()
{
	return requested_app != nullptr;
}

void Platform::request_application(const apps::AppInfo *app)
{
	requested_app = app;
}

bool Platform::start_app()
{
	auto *requested_app_info = requested_app;
	// Reset early incase error in preperation stage
	requested_app = nullptr;

	if (active_app)
	{
		auto execution_time = timer.stop();
		LOGI("Closing App (Runtime: {:.1f})", execution_time);

		auto app_id = active_app->get_name();

		active_app->finish();
	}

	active_app = requested_app_info->create();

	active_app->set_name(requested_app_info->id);

	if (!active_app)
	{
		LOGE("Failed to create a valid vulkan app.");
		return false;
	}

	if (!active_app->prepare(*this))
	{
		LOGE("Failed to prepare vulkan app.");
		return false;
	}

	call_hook(Hook::OnAppStart, [&](Plugin *plugin) { plugin->on_app_start(requested_app_info->id); });

	return true;
}

void Platform::resize(uint32_t width, uint32_t height)
{
	if (window)
	{
		auto actual_extent = window->resize({width, height});

		if (active_app)
		{
			active_app->resize(actual_extent.width, actual_extent.height);
		}
	}
}
}        // namespace vkb
