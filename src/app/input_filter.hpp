//@	{
//@		"dependencies_extra":[
//@			{"ref":"./input_filter.o", "rel":"implementation"},
//@			{"ref":"icu-uc", "rel":"implementation", "origin":"pkg-config"}
//@		]
//@	}

#ifndef SLIDEPROJ_APP_INPUT_FILTER_HPP
#define SLIDEPROJ_APP_INPUT_FILTER_HPP

#include <filesystem>
#include <unicode/unistr.h>
#include <unicode/stringoptions.h>
#include <vector>
#include <span>

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

	class input_filter
	{
	public:
		input_filter() = default;
		explicit input_filter(std::span<std::string const>){}

		bool accepts(std::filesystem::directory_entry const& entry) const
		{
			auto const name_extension = icu::UnicodeString::fromUTF8(entry.path().extension().string());
			auto const look_for = icu::UnicodeString::fromUTF8(".bmp");

			return name_extension.caseCompare(
				0, name_extension.length(),
				look_for, 0, look_for.length(),
				U_FOLD_CASE_DEFAULT
			) == 0;
		}

	private:
		std::vector<input_filter_pattern> m_patterns;
	};
}

#endif