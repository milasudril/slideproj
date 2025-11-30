//@	{
//@		"dependencies_extra":[
//@			{"ref":"icu-uc", "rel":"implementation", "origin":"pkg-config"}
//@		]
//@	}

#ifndef SLIDEPROJ_APP_INPUT_FILTER_HPP
#define SLIDEPROJ_APP_INPUT_FILTER_HPP

#include <filesystem>
#include <unicode/unistr.h>
#include <unicode/stringoptions.h>

namespace slideproj::app
{
	class input_filter
	{
	public:
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
	};
}

#endif