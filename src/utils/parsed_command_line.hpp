//@	{"dependencies_extra":[{"ref":"./parsed_command_line.o", "rel":"implementation"}]}

#ifndef SLIDEPROJ_UTILS_COMMAND_LINE_PARSER_HPP
#define SLIDEPROJ_UTILS_COMMAND_LINE_PARSER_HPP

#include "src/utils/transparent_string_hash.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <span>

namespace slideproj::utils
{
	struct parsed_arg
	{
		std::string name;
		std::vector<std::string> value;
	};

	parsed_arg parse_arg(char const* arg, string_lookup_table<std::string> const& valid_options);

	struct action_info
	{
		int (*main)(string_lookup_table<std::vector<std::string>> const& args);
		string_lookup_table<std::string> valid_options;
	};

	class parsed_command_line
	{
	public:
		parsed_command_line() = default;

		explicit parsed_command_line(
			char const* appname,
			std::span<char const* const> argv,
			string_lookup_table<action_info> const& valid_actions
		);

		int execute() const
		{ return m_action.main(m_args); }

		template<class Key>
		auto const get_option(Key&& key) const
		{
			auto const i = m_args.find(std::forward<Key>(key));
			if(i == std::end(m_args))
			{ return std::optional<std::span<std::string>>{}; }
			return std::optional{std::span{i->second}};
		}

	private:
		action_info m_action;
		string_lookup_table<std::vector<std::string>> m_args;

	};
};

#endif