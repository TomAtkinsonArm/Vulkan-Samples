/* Copyright (c) 2020-2021, Arm Limited and Contributors
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

#include "batch_mode.h"

#include "vulkan_sample.h"

namespace plugins
{
vkb::Flag batch_cmd{"batch", vkb::Flag::Type::Command, "Enable batch mode"};

vkb::Flag duration_flag{"duration", vkb::Flag::Type::FlagWithOneArg, "The duration which a configuration should run for in seconds"};

vkb::Flag wrap_flag{"wrap-to-start", vkb::Flag::Type::FlagOnly, "Once all configurations have run wrap to the start"};

vkb::Flag tags_flag{"T", vkb::Flag::Type::FlagWithManyArg, "Filter samples by tags"};

vkb::Flag categories_flag{"C", vkb::Flag::Type::FlagWithManyArg, "Filter samples by categories"};

using BatchModeSampleIter = std::vector<apps::AppInfo *>::const_iterator;

BatchMode::BatchMode() :
    BatchModeTags("Batch Mode",
                  "Run a collection of samples in sequence.",
                  {
                      vkb::Hook::OnUpdate,
                      vkb::Hook::OnAppError,
                  },
                  {
                      vkb::FlagGroup(vkb::FlagGroup::Type::Individual, false, {&batch_cmd}),
                      vkb::FlagGroup(vkb::FlagGroup::Type::Individual, true, {&duration_flag, &tags_flag, &categories_flag, &wrap_flag}),
                  })
{
}

bool BatchMode::is_active(const vkb::Parser &parser)
{
	return parser.contains(batch_cmd);
}

void BatchMode::init(const vkb::Parser &parser, vkb::OptionalProperties *properties)
{
	if (parser.contains(duration_flag))
	{
		sample_run_time_per_configuration = std::chrono::duration<float, vkb::Timer::Seconds>{parser.get_float(duration_flag)};
	}

	if (parser.contains(wrap_flag))
	{
		wrap_to_start = parser.get_bool(wrap_flag);
	}

	std::vector<std::string> tags;
	if (parser.contains(tags_flag))
	{
		tags = parser.get_list(tags_flag);
	}

	std::vector<std::string> categories;
	if (parser.contains(categories_flag))
	{
		categories = parser.get_list(categories_flag);
	}

	sample_list = apps::get_samples(categories, tags);

	if (sample_list.empty())
	{
		LOGE("No samples found")
		throw std::runtime_error{"Can not continue"};
	}

	sample_iter = sample_list.begin();

	// Stop the application from processing input
	properties->platform_properties.process_input_events = false;
	properties->window_properties.resizable              = false;

	// Request first sample
	platform->request_application((*sample_iter));
}

void BatchMode::on_update(float delta_time)
{
	elapsed_time += delta_time;

	// When the runtime for the current configuration is reached, advance to the next config or next sample
	if (elapsed_time >= sample_run_time_per_configuration.count())
	{
		elapsed_time = 0.0f;

		// Only check and advance the config if the application is a vulkan sample
		if (auto *vulkan_app = dynamic_cast<vkb::VulkanSample *>(&platform->get_app()))
		{
			auto &configuration = vulkan_app->get_configuration();

			if (configuration.next())
			{
				configuration.set();
				return;
			}
		}

		// Cycled through all configs, load next app
		load_next_app();
	}
}

void BatchMode::on_app_error(const std::string &app_id)
{
	// App failed, load next app
	load_next_app();
}

void BatchMode::load_next_app()
{
	// Wrap it around to the start
	++sample_iter;
	if (sample_iter == sample_list.end())
	{
		if (wrap_to_start)
		{
			sample_iter = sample_list.begin();
		}
		else
		{
			platform->close();
		}
	}
	else
	{
		// App will be started before the next update loop
		platform->request_application((*sample_iter));
	}
}
}        // namespace plugins