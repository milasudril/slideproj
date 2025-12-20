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

	parsed_arg parse_arg(char const* arg);

	class parsed_command_line
	{
	public:
		parsed_command_line() = default;

		explicit parsed_command_line(char const* appname, int argc, char const* const* argv);

		std::string_view get_action() const
		{ return m_action; }

		template<class Key>
		auto const get_option(Key&& key) const
		{
			auto const i = m_args.find(std::forward<Key>(key));
			if(i == std::end(m_args))
			{ return std::optional<std::span<std::string>>{}; }
			return std::optional{std::span{i->second}};
		}

	private:
		std::string m_action;
		string_lookup_table<std::vector<std::string>> m_args;
	};
};

#endif