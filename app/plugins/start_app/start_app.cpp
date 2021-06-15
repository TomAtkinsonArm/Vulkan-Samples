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

#include "start_app.h"

#include "apps.h"

namespace plugins
{
StartApp::StartApp() :
    StartAppTags("Apps",
                 "A collection of flags to samples and apps.",
                 {}, {&app_cmd, &sample_cmd})
{
}

bool StartApp::is_active(const vkb::Parser &parser)
{
	return parser.contains(app_cmd) || parser.contains(sample_cmd);
}

void StartApp::init(const vkb::Parser &parser, vkb::OptionalProperties *properties)
{
	const apps::AppInfo *app = nullptr;

	if (parser.contains(app_cmd))
	{
		app = apps::get_app(parser.get_string(app_cmd));
	}

	if (parser.contains(sample_cmd))
	{
		auto *sample = apps::get_sample(parser.get_string(sample_cmd));
		if (sample != nullptr)
		{
			properties->window_properties.title = "Vulkan Samples: " + sample->name;
		}
	}

	if (app != nullptr)
	{
		properties->application_properties.id = app->id;
	}
}
}        // namespace plugins