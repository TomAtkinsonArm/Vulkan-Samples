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

#pragma once

#include "platform/filesystem.h"
#include "platform/plugins/plugin_base.h"

namespace plugins
{
class Screenshot;

using ScreenshotTags = vkb::PluginBase<Screenshot, vkb::tags::Passive>;

class Screenshot : public ScreenshotTags
{
  public:
	Screenshot();

	virtual ~Screenshot() = default;

	virtual bool is_active(const vkb::Parser &parser) override;

	virtual void init(const vkb::Parser &parser, vkb::OptionalProperties* properties) override;

	virtual void on_update(float delta_time) override;

	virtual void on_app_start(const std::string &app_info) override;

	virtual void on_post_draw(vkb::RenderContext &context) override;

  private:
	uint32_t    current_frame = 0;
	uint32_t    frame_number;
	std::string current_app_name;

	bool        output_path_set = false;
	std::string output_path;
};
}        // namespace plugins