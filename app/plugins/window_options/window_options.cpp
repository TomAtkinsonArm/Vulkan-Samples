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

#include "window_options.h"

#include <algorithm>

namespace plugins
{
WindowOptions::WindowOptions() :
    WindowOptionsTags("Window Options",
                      "A collection of flags to configure window used when running the application. Implementation may differ between platforms",
                      {}, {&width_flag, &height_flag, &vsync_flag, &fullscreen_flag, &borderless_flag, &headless_flag})
{
}

bool WindowOptions::is_active(const vkb::Parser &parser)
{
	return true;
}

void WindowOptions::init(const vkb::Parser &parser, vkb::OptionalProperties *properties)
{
	if (parser.contains(width_flag))
	{
		int32_t width                   = parser.get_int(width_flag);
		properties->target_extent.width = static_cast<uint32_t>(width);
	}

	if (parser.contains(height_flag))
	{
		int32_t height                  = parser.get_int(height_flag);
		properties->target_extent.width = static_cast<uint32_t>(height);
	}

	if (parser.contains(headless_flag))
	{
		properties->window_properties.mode = vkb::WindowMode::Headless;
	}
	else if (parser.contains(fullscreen_flag))
	{
		properties->window_properties.mode = vkb::WindowMode::Fullscreen;
	}
	else if (parser.contains(borderless_flag))
	{
		properties->window_properties.mode = vkb::WindowMode::FullscreenBorderless;
	}

	if (parser.contains(vsync_flag))
	{
		std::string value = parser.get_string(vsync_flag);
		std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		if (value == "on")
		{
			properties->render_properties.vsync = vkb::VsyncMode::On;
		}
		else if (value == "off")
		{
			properties->render_properties.vsync = vkb::VsyncMode::Off;
		}
	}
}
}        // namespace plugins