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

#include "stop_after.h"

namespace plugins
{
vkb::Flag stop_after_flag = {"stop-after-frame",
                             vkb::Flag::Type::FlagWithOneArg,
                             "Stop the application after a certain number of frames"};

StopAfter::StopAfter() :
    StopAfterTags("Stop After X",
                  "A collection of flags to stop the running application after a set period.",
                  {vkb::Hook::OnUpdate}, {vkb::FlagGroup(vkb::FlagGroup::Type::UseOne, true, {&stop_after_flag})})
{
}

bool StopAfter::is_active(const vkb::Parser &parser)
{
	return parser.contains(stop_after_flag);
}

void StopAfter::init(const vkb::Parser &parser, vkb::OptionalProperties* properties)
{
	remaining_frames = static_cast<uint32_t>(parser.get_int(stop_after_flag));
}

void StopAfter::on_update(float delta_time)
{
	remaining_frames--;

	if (remaining_frames <= 0)
	{
		platform->close();
	}
}
}        // namespace plugins