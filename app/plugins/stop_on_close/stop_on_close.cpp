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

#include "stop_on_close.h"

#include <iostream>

namespace plugins
{
vkb::Flag stop_cmd = {"stop-on-close", vkb::Flag::Type::FlagOnly, "Halt the application before closing"};

StopOnClose::StopOnClose() :
    StopOnCloseTags("Stop on Close",
                    "Halt the application before exiting. (Desktop Only)",
                    {vkb::Hook::OnPlatformClose}, {vkb::FlagGroup(vkb::FlagGroup::Type::Individual, true, {&stop_cmd})})
{
}

bool StopOnClose::is_active(const vkb::Parser &parser)
{
	return parser.contains(stop_cmd);
}

void StopOnClose::init(const vkb::Parser &parser, vkb::OptionalProperties* properties)
{
}

void StopOnClose::on_platform_close()
{
#ifndef ANDROID
	std::cout << "Press any key to continue";
	std::cin.get();
#endif
}
}        // namespace plugins