/* Copyright (c) 2021, Arm Limited and Contributors
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

#include "platform/desktop/desktop_platform.h"

#include <iostream>

#include "platform/desktop/glfw_window.h"
#include "platform/desktop/headless_window.h"

namespace vkb
{
namespace
{
Extent clamp_extent(const Extent &extent)
{
	return {std::max<uint32_t>(extent.width, 420), std::max<uint32_t>(extent.height, 320)};
}
}        // namespace

DesktopPlatform::DesktopPlatform()
{}

bool DesktopPlatform::initialize(const std::vector<Plugin *> &plugins)
{
	return Platform::initialize(plugins);
}

void DesktopPlatform::create_window(const Extent &initial_extent, const WindowProperties &properties)
{
	// Clamp window dimensions
	auto extent = clamp_extent(initial_extent);

	if (properties.mode == WindowMode::Headless)
	{
		window = std::make_unique<HeadlessWindow>(extent, properties);
	}
	else
	{
		window = std::make_unique<GlfwWindow>(this, extent, properties);
	}
}

void DesktopPlatform::input_event(const InputEvent &input_event)
{
	Platform::input_event(input_event);

	if (input_event.get_source() == EventSource::Keyboard)
	{
		const auto &key_event = static_cast<const KeyInputEvent &>(input_event);

		if (key_event.get_code() == KeyCode::Back ||
		    key_event.get_code() == KeyCode::Escape)
		{
			close();
		}
	}
}

void DesktopPlatform::resize(uint32_t width, uint32_t height)
{
	auto extent = clamp_extent({width, height});
	Platform::resize(extent.width, extent.height);
}

void DesktopPlatform::terminate(ExitCode code)
{
	// Clean up
	Platform::terminate(code);

	if (code == ExitCode::UnableToRun)
	{
		// Help shown, pause to allow the user to read
		std::cout << "Press any key to continue\n";
		std::cin.get();
	}
}
}        // namespace vkb