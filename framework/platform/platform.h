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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "apps.h"
#include "common/optional.h"
#include "common/utils.h"
#include "common/vk_common.h"
#include "platform/application.h"
#include "platform/filesystem.h"
#include "platform/parser.h"
#include "platform/plugins/parser.h"
#include "platform/plugins/plugin.h"
#include "platform/properties.h"
#include "platform/window.h"

namespace vkb
{
enum class ExitCode
{
	Success     = 0, /* App prepare succeeded, it ran correctly and exited properly with no errors */
	UnableToRun = 1, /* App prepare failed, could not run */
	FatalError  = 2  /* App encountered an unexpected error */
};

class Platform
{
  public:
	Platform() = default;

	virtual ~Platform() = default;

	/**
	 * @brief Initialize the platform
	 * @param plugins plugins available to the platform
	 */
	virtual bool initialize(const std::vector<Plugin *> &plugins);

	/**
	 * @brief Handles the main loop of the platform
	 * This should be overriden if a platform requires a specific main loop setup.
	 */
	void main_loop();

	/**
	 * @brief Runs the application for one frame
	 */
	void update();

	/**
	 * @brief Terminates the platform and the application
	 * @param code Determines how the platform should exit
	 */
	virtual void terminate(ExitCode code);

	/**
	 * @brief Requests to close the platform at the next available point
	 */
	virtual void close() const;

	/**
	 * @brief Returns the working directory of the application set by the platform
	 * @returns The path to the working directory
	 */
	static const std::string &get_external_storage_directory();

	/**
	 * @brief Returns the suitable directory for temporary files from the environment variables set in the system
	 * @returns The path to the temp folder on the system
	 */
	static const std::string &get_temp_directory();

	/**
	 * @return The dot-per-inch scale factor
	 */
	virtual float get_dpi_factor() const;

	/**
	 * @return The VkInstance extension name for the platform
	 */
	virtual const char *get_surface_extension() = 0;

	virtual std::unique_ptr<RenderContext> create_render_context(Device &device, VkSurfaceKHR surface) const;

	virtual void request_properties(PlatformProperties properties);

	virtual void resize(uint32_t width, uint32_t height);

	virtual void input_event(const InputEvent &input_event);

	Window &get_window() const;

	void set_window(std::unique_ptr<Window> &&window);

	Application &get_app() const;

	Application &get_app();

	std::vector<std::string> &get_arguments();

	static void set_arguments(const std::vector<std::string> &args);

	static void set_external_storage_directory(const std::string &dir);

	static void set_temp_directory(const std::string &dir);

	static PositionalCommand app;
	static SubCommand        samples;
	static FlagCommand       sample;
	static FlagCommand       test;
	static FlagCommand       benchmark;
	static FlagCommand       width;
	static FlagCommand       height;
	static FlagCommand       headless;
	static FlagCommand       batch_tags;
	static FlagCommand       batch_categories;
	static SubCommand        batch;

	CommandParser *get_parser() const
	{
		return parser.get();
	}

  protected:
	std::unique_ptr<CommandParser> parser;
	void set_focus(bool focused);

	/**
	 * @brief Handles the creation of the window
	 * 
	 * @param properties Preferred window configuration
	 */
	virtual void create_window(const Extent &initial_extent, const WindowProperties &properties) = 0;

	template <class T>
	bool using_plugin() const;

	template <class T>
	T *get_plugin() const;

	void request_application(const apps::AppInfo *app);

	bool app_requested();

	bool start_app();

	void call_hook(const Hook &hook, std::function<void(Plugin *)> fn) const;

  protected:
	std::vector<Plugin *> active_plugins;

	std::unordered_map<Hook, std::vector<Plugin *>> hooks;

	std::unique_ptr<Window> window{nullptr};

	std::unique_ptr<Application> active_app{nullptr};

	virtual std::vector<spdlog::sink_ptr> get_platform_sinks();

	bool               focused;
	RenderProperties   render_properties;
	PlatformProperties properties;

  private:
	Timer timer;

	const apps::AppInfo *requested_app{nullptr};

	std::unique_ptr<Parser> parser;

	std::vector<Plugin *> plugins;

	/// Static so can be set via JNI code in android_platform.cpp
	static std::vector<std::string> arguments;

	static std::string external_storage_directory;

	static std::string temp_directory;
};

template <class T>
bool Platform::using_plugin() const
{
	return !plugins::with_tags<T>(active_plugins).empty();
}

template <class T>
T *Platform::get_plugin() const
{
	assert(using_plugin<T>() && "Plugin is not enabled but was requested");
	const auto plugins = plugins::with_tags<T>(active_plugins);
	return dynamic_cast<T *>(plugins[0]);
}
}        // namespace vkb
