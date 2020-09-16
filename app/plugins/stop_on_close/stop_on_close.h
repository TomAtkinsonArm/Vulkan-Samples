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

#include "platform/plugins/plugin_base.h"

namespace plugins
{
class StopOnClose;

// Passive behaviour
using StopOnCloseTags = vkb::PluginBase<StopOnClose, vkb::tags::Passive>;

class StopOnClose : public StopOnCloseTags
{
  public:
	StopOnClose();

	virtual ~StopOnClose() = default;

	virtual bool is_active(const vkb::Parser &parser) override;

	virtual void init(const vkb::Parser &parser, vkb::OptionalProperties *properties) override;

	virtual void on_platform_close() override;
};
}        // namespace plugins