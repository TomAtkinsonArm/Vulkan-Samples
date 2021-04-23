/* Copyright (c) 2018-2021, Arm Limited and Contributors
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

#include "common/optional.h"
#include "common/vk_common.h"
#include "core/instance.h"

namespace vkb
{
/**
 * @brief An interface class, declaring the behaviour of a Window
 */
class Window
{
  public:
	struct Extent
	{
		uint32_t width;
		uint32_t height;
	};

	struct OptionalExtent
	{
		Optional<uint32_t> width;
		Optional<uint32_t> height;
	};

	enum class Mode
	{
		Headless,
		Fullscreen,
		FullscreenBorderless,
		Default
	};

	enum class Vsync
	{
		OFF,
		ON,
		Default
	};

	struct OptionalProperties
	{
		Optional<std::string> title;
		Optional<Mode>        mode;
		Optional<bool>        resizable;
		Optional<Vsync>       vsync;
		OptionalExtent        extent;
	};

	struct Properties
	{
		std::string title     = "";
		Mode        mode      = Mode::Default;
		bool        resizable = true;
		Vsync       vsync     = Vsync::Default;
		Extent      extent    = {1024, 768};
	};

	/**
	 * @brief Constructs a Window
	 * @param properties The preferred configuration of the window
	 */
	Window(const Properties &properties);

	virtual ~Window() = default;

	/**
	 * @brief Gets a handle from the platform's Vulkan surface 
	 * @param instance A Vulkan instance
	 * @returns A VkSurfaceKHR handle, for use by the application
	 */
	virtual VkSurfaceKHR create_surface(Instance &instance) = 0;

	/**
	 * @brief Checks if the window should be closed
	 */
	virtual bool should_close() = 0;

	/**
	 * @brief Handles the processing of all underlying window events
	 */
	virtual void process_events();

	/**
	 * @brief Requests to close the window
	 */
	virtual void close() = 0;

	/**
	 * @return The dot-per-inch scale factor
	 */
	virtual float get_dpi_factor() const = 0;

	/**
     * @return The scale factor for systems with heterogeneous window and pixel coordinates
     */
	virtual float get_content_scale_factor() const;

	/**
	 * @brief Attempt to resize the window - not gauranteed to change
	 * 
	 * @param extent The preferred window extent
	 * @return Extent The new window extent
	 */
	Extent resize(const Extent &extent);

	const Extent &get_extent() const;

  protected:
	Properties properties;
};
}        // namespace vkb
