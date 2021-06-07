/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include "parser.h"

#include <iostream>
#include <set>
#include <sstream>

#include <spdlog/fmt/fmt.h>

#include "common/logging.h"
#include "common/strings.h"
#include "platform/plugins/plugin.h"

namespace vkb
{
Parser::Parser(const std::vector<Plugin *> &plugins)
{
	std::vector<Plugin *> entrypoints     = plugins::with_tags<tags::Entrypoint>(plugins);
	std::vector<Plugin *> not_entrypoints = plugins::without_tags<tags::Entrypoint>(plugins);

	// Dont mix well togther
	std::vector<Plugin *> aggressive = plugins::with_tags<tags::FullControl, tags::Stopping>(not_entrypoints);

	// Work well with any plugin
	std::vector<Plugin *> passives = plugins::with_tags<tags::Passive>(not_entrypoints);

	// Entrypoint - {aggressive, passive}
	std::unordered_map<Plugin *, std::vector<Plugin *>> usage;
	usage.reserve(entrypoints.size());

	for (auto entrypoint : entrypoints)
	{
		std::vector<Plugin *> compatable;

		if (!entrypoint->has_tag<tags::FullControl>() || entrypoint->has_tag<tags::Stopping>())
		{
			// The entrypoint does not dictate the applications functionality so allow other plugins to take control
			compatable.reserve(compatable.size() + aggressive.size());
			for (auto ext : aggressive)
			{
				compatable.push_back(ext);
			}
		}

		compatable.reserve(compatable.size() + passives.size());
		for (auto ext : passives)
		{
			compatable.push_back(ext);
		}

		usage.insert({entrypoint, compatable});
	}

	const char *spacer   = "  ";
	const char *app_name = "vulkan_samples";

	std::vector<std::string> docopt_help_lines;

	// usage line + line per entrypoint
	help_lines.reserve(2 + entrypoints.size());

	help_lines.push_back("Usage:");
	help_lines.push_back(fmt::format("{}{} {}", spacer, app_name, "help"));

	for (auto it = usage.begin(); it != usage.end(); it++)
	{
		std::stringstream out;

		out << spacer << app_name << " ";

		{
			// Append all entrypoint flag groups
			auto   entry  = it->first;
			auto & groups = entry->get_cli_commands();
			size_t i      = 0;
			for (auto &group : groups)
			{
				out << group.get_command();

				if (i < groups.size() - 1)
				{
					out << " ";
				}

				i++;
			}
		}
		{
			size_t i = 0;
			for (auto ext : it->second)
			{
				auto & groups = ext->get_cli_commands();
				size_t i      = 0;
				for (auto &group : groups)
				{
					out << group.get_command();

					if (i < groups.size() - 1)
					{
						out << " ";
					}

					i++;
				}
			}
		}

		help_lines.push_back(out.str());
	}

	help_lines.push_back("");        // New line

	std::set<Flag *> unique_flags;
	for (auto ext : plugins)
	{
		auto &groups = ext->get_cli_commands();
		for (auto &group : groups)
		{
			auto group_flags = group.get_flags();
			for (auto flag : group_flags)
			{
				unique_flags.insert(flag);
			}
		}
	}

	std::vector<std::string> options;
	std::vector<std::string> commands;

	size_t column_width = 30;

	for (auto flag : unique_flags)
	{
		auto command = flag->get_command();
		auto line    = fmt::format("{}{}{}{}", spacer, command, std::string(column_width - command.size(), ' '), flag->get_help());

		bool option = flag->get_type() == Flag::Type::FlagOnly ||
		              flag->get_type() == Flag::Type::FlagWithOneArg ||
		              flag->get_type() == Flag::Type::FlagWithManyArg;

		if (option)
		{
			// These flags must be added at docopt parse time for the flag keys to work
			options.push_back(line);
		}
		else
		{
			// These flags can be appended in the printed help and are not added to docopt
			commands.push_back(line);
		}
	}

	std::string help_command = fmt::format("{}{}{}{}", spacer, "help", std::string(column_width - std::string("help").size(), ' '), "Show the help menu");

	// Fill docopt lines
	docopt_help_lines.reserve(help_lines.size());
	docopt_help_lines.insert(docopt_help_lines.end(), help_lines.begin(), help_lines.end());

	// Docopt options
	docopt_help_lines.push_back("Options:");
	docopt_help_lines.reserve(options.size());
	docopt_help_lines.insert(docopt_help_lines.end(), options.begin(), options.end());

	// New line
	docopt_help_lines.push_back("");

	// Docopt extras
	docopt_help_lines.push_back("Extras:");
	docopt_help_lines.reserve(commands.size());
	docopt_help_lines.insert(docopt_help_lines.end(), commands.begin(), commands.end());
	docopt_help_lines.push_back(help_command);

	for (auto &line : docopt_help_lines)
	{
		docopt_help += line + "\n";
	}

	// Help Menu
	// Commands
	help_lines.push_back("Commands:");
	help_lines.reserve(commands.size());
	help_lines.insert(help_lines.end(), commands.begin(), commands.end());
	help_lines.push_back(help_command);

	// New line
	help_lines.push_back("");

	// Options
	help_lines.push_back("Options:");
	help_lines.reserve(options.size());
	help_lines.insert(help_lines.end(), options.begin(), options.end());

	// New line
	help_lines.push_back("");

	// Plugins
	help_lines.push_back("Plugins:");
	for (auto *plugin : plugins)
	{
		help_lines.push_back("");
		help_lines.push_back(fmt::format("{}", plugin->get_name()));
		help_lines.push_back(fmt::format("{}{}", spacer, plugin->get_description()));
		help_lines.push_back("");

		for (auto group : plugin->get_cli_commands())
		{
			for (auto flag : group.get_flags())
			{
				auto command = flag->get_command();
				auto line    = fmt::format("{}{}{}{}", spacer, command, std::string(column_width - command.size(), ' '), flag->get_help());
				help_lines.push_back(line);
			}
		}
	}
}

bool Parser::parse(std::vector<std::string> args)
{
	try
	{
		parsed_args = docopt::docopt_parse(docopt_help, args, false, false, false);

		return !contains("help");        // Stop execution of app if help command is used
	}
	catch (docopt::DocoptLanguageError e)
	{
		LOGE("LanguageError: {}", e.what());
		return false;
	}
	catch (docopt::DocoptArgumentError e)
	{
		LOGE("ArgumentError: {}", e.what());
		return false;
	}
	catch (std::exception e)
	{
		LOGE("Unknown Command: {}", e.what());
		return false;
	}
}

void Parser::print_help()
{
	for (auto &line : help_lines)
	{
		LOGI(line);
	}
}

bool Parser::contains(const Flag &flag) const
{
	return contains(flag.get_key());
}

bool Parser::contains(const std::string &key) const
{
	if (parsed_args.count(key) != 0)
	{
		const auto &result = parsed_args.at(key);

		if (result)
		{
			if (result.isBool())
			{
				return result.asBool();
			}

			return true;
		}
	}

	return false;
}

const docopt::value &Parser::get(const std::string &key) const
{
	return parsed_args.at(key);
}

const bool Parser::get_bool(const Flag &flag) const
{
	auto key = flag.get_key();

	if (contains(key))
	{
		auto result = get(key);
		if (result.isBool())
		{
			return result.asBool();
		}

		throw std::runtime_error("Argument option is not a bool type");
	}

	throw std::runtime_error("Couldn't find argument option");
}

const int32_t Parser::get_int(const Flag &flag) const
{
	auto key = flag.get_key();

	if (contains(key))
	{
		auto result = get(key);

		try
		{
			// Throws a runtime error if the result can not be parsed as long
			return static_cast<int32_t>(result.asLong());
		}
		catch (std::exception e)
		{
			throw std::runtime_error("Argument option is not int type");
		}
	}

	throw std::runtime_error("Couldn't find argument option");
}

const float Parser::get_float(const Flag &flag) const
{
	auto key = flag.get_key();

	if (contains(key))
	{
		auto result = get(key);

		if (result.isString())
		{
			// Throws a runtime error if the result can not be parsed as long
			return std::stof(result.asString());
		}

		throw std::runtime_error("Argument option could not be converted to float");
	}

	throw std::runtime_error("Couldn't find argument option");
}

const std::string Parser::get_string(const Flag &flag) const
{
	auto key = flag.get_key();

	if (contains(key))
	{
		auto result = get(key);
		if (result.isString())
		{
			return result.asString();
		}

		throw std::runtime_error("Argument option is not string type");
	}

	throw std::runtime_error("Couldn't find argument option");
}

const std::vector<std::string> Parser::get_list(const Flag &flag) const
{
	auto key = flag.get_key();

	if (contains(key))
	{
		auto result = get(key);
		if (result.isStringList())
		{
			return result.asStringList();
		}
		else if (result.isString())
		{
			// Only one item is in the list
			return {result.asString()};
		}

		throw std::runtime_error("Argument option is not vector of string type");
	}

	throw std::runtime_error("Couldn't find argument option");
}
}        // namespace vkb
