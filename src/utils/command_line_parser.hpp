#ifndef SLIDEPROJ_UTILS_COMMAND_LINE_PARSER_HPP
#define SLIDEPROJ_UTILS_COMMAND_LINE_PARSER_HPP

#include "src/utils/transparent_string_hash.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace slideproj::utils
{
	class parsed_command_line
	{
	public:
		parsed_command_line() = default;

		explicit parsed_command_line(int argc, char** argv);

		template<class Key>
		auto const get_option(Key&& key) const
		{
			auto const i = m_args.find(std::forward<Key>(key));
			if(i == std::end(m_args))
			{ return std::optional<std::span<std::string>>{}; }
			return std::optional{std::span{i->second}};
		}

	private:
		string_lookup_table<std::vector<std::string>> m_args;
	};
};

#endif