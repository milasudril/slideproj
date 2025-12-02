//@	{
//@		"dependencies_extra":[
//@			{"ref":"./input_filter.o", "rel":"implementation"},
//@			{"ref":"icu-uc", "rel":"implementation", "origin":"pkg-config"}
//@		]
//@	}

#ifndef SLIDEPROJ_APP_INPUT_FILTER_HPP
#define SLIDEPROJ_APP_INPUT_FILTER_HPP

#include <filesystem>
#include <vector>
#include <algorithm>

namespace slideproj::app
{
	class input_filter_pattern
	{
	public:
		explicit input_filter_pattern(std::string_view pattern_string);

		bool matches(std::string_view string_to_match) const;

	private:
		std::string m_pattern;
	};

	struct input_filter
	{
		bool accepts(std::filesystem::directory_entry const& entry) const
		{
			return std::ranges::any_of(include, [path = entry.path().string()](auto const& pattern) {
				return pattern.matches(path);
			});
		}

		std::vector<input_filter_pattern> include;
	};
}

#endif