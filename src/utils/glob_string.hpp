//@	{"dependencies_extra":[{"ref":"./glob_string.o", "rel":"implementation"}]}

#ifndef SLIDEPROJ_UTILS_GLOB_STRING_HPP
#define SLIDEPROJ_UTILS_GLOB_STRING_HPP

#include <string>
#include <string_view>
#include <vector>

namespace slideproj::utils
{
	class glob_string
	{
	public:
		explicit glob_string(std::string_view pattern_string);

		bool matches(std::string_view string_to_match) const;

	private:
		std::string m_pattern;
	};

	std::vector<glob_string> make_glob_strings(std::vector<std::string> const& strings);
}

#endif