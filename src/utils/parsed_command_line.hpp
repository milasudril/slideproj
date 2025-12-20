//@	{"dependencies_extra":[{"ref":"./parsed_command_line.o", "rel":"implementation"}]}

#ifndef SLIDEPROJ_UTILS_COMMAND_LINE_PARSER_HPP
#define SLIDEPROJ_UTILS_COMMAND_LINE_PARSER_HPP

#include "src/utils/transparent_string_hash.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <span>
#include <functional>

namespace slideproj::utils
{
	struct parsed_arg
	{
		std::string name;
		std::vector<std::string> value;
	};

	struct option_info
	{
		std::string description;
		std::vector<std::string> default_value;
		size_t cardinality;
		string_set valid_values{};

		// TODO: Add info about uniqueness, data type
	};

	parsed_arg parse_arg(char const* arg, string_lookup_table<option_info> const& valid_options);

	struct action_info
	{
		std::move_only_function<int(string_lookup_table<std::vector<std::string>> const&) const> main;
		std::string description;
		string_lookup_table<option_info> valid_options;
	};

	class parsed_command_line
	{
	public:
		parsed_command_line() = default;

		explicit parsed_command_line(
			char const* appname,
			std::span<char const* const> argv,
			string_lookup_table<action_info>&& valid_actions
		);

		int execute() const
		{ return m_action.main(m_args); }

	private:
		action_info m_action;
		string_lookup_table<std::vector<std::string>> m_args;

	};
};

#endif