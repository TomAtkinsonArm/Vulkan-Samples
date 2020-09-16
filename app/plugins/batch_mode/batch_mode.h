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

#include <chrono>
#include <vector>

#include "apps.h"
#include "platform/plugins/plugin_base.h"
#include "timer.h"

using namespace std::chrono_literals;

namespace plugins
{
using BatchModeTags = vkb::PluginBase<vkb::tags::Entrypoint, vkb::tags::FullControl>;

class BatchMode : public BatchModeTags
{
  public:
	BatchMode();

	virtual ~BatchMode() = default;

	virtual bool is_active(const vkb::Parser &parser) override;

	virtual void init(const vkb::Parser &parser, vkb::OptionalProperties* properties) override;

	virtual void on_update(float delta_time) override;

	virtual void on_app_error(const std::string &app_id) override;

  private:
	/// The list of suitable samples to be run in conjunction with batch mode
	std::vector<apps::AppInfo *> sample_list{};

	/// An iterator to the current batch mode sample info object
	std::vector<apps::AppInfo *>::const_iterator sample_iter;

	/// The amount of time run per configuration for each sample
	std::chrono::duration<float, vkb::Timer::Seconds> sample_run_time_per_configuration{3s};

	float elapsed_time{0.0f};

	bool wrap_to_start = false;

	void load_next_app();
};
}        // namespace plugins