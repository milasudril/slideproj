//@	{
//@		"target": {"name": "glob_string.o"},
//@		"dependencies": [{"ref":"icu-uc", "rel":"implementation", "origin":"pkg-config"}]
//@	}

#include "./glob_string.hpp"
#include <unicode/unistr.h>
#include <unicode/stringoptions.h>
#include <utility>
#include <algorithm>

slideproj::utils::glob_string::glob_string(std::string_view pattern_string)
{
	auto converted = icu::UnicodeString::fromUTF8(pattern_string);
	converted.foldCase(U_FOLD_CASE_DEFAULT)
		.toUTF8String(m_pattern);
}

bool slideproj::utils::glob_string::matches(std::string_view string_to_match) const
{
	auto converted = icu::UnicodeString::fromUTF8(string_to_match);
	std::string case_folded;
	converted.foldCase(U_FOLD_CASE_DEFAULT)
		.toUTF8String(case_folded);

	enum class state{
		mach_until_wildcard,
		consume_until_next_match,
		is_match_complete
	};
	auto current_state = state::mach_until_wildcard;
	auto to_match_iter = std::begin(std::as_const(case_folded));
	auto pattern_iter = std::begin(std::as_const(m_pattern));
	auto const pattern_end = std::end(std::as_const(m_pattern));
	auto match_start = pattern_iter;
	auto const match_end = std::end(std::as_const(case_folded));

	while(to_match_iter != match_end && pattern_iter != pattern_end)
	{
		auto const pattern_char = *pattern_iter;
		auto const match_char = *to_match_iter;

		switch(current_state)
		{
			case state::mach_until_wildcard:
			{
				switch(pattern_char)
				{
					case '*':
						current_state = state::consume_until_next_match;
						++pattern_iter;
						if(pattern_iter == pattern_end)
						{ return true; }
						break;

					default:
						if(match_char != pattern_char)
						{ return false; }

						++pattern_iter;
						++to_match_iter;
						if(to_match_iter == match_end && pattern_iter == pattern_end)
						{ return true; }
				}
				break;
			}

			case state::consume_until_next_match:
			{
				switch(pattern_char)
				{
					case '*':
						++pattern_iter;
						if(pattern_iter == pattern_end)
						{ return true; }
						break;

					default:
						if(match_char == pattern_char)
						{
							current_state = state::is_match_complete;
							match_start = pattern_iter;
							++pattern_iter;
							++to_match_iter;

							if(to_match_iter == match_end && pattern_iter == pattern_end)
							{ return true; }
						}
						else
						{ ++to_match_iter; }
				}
				break;
			}

			case state::is_match_complete:
			{
				switch(pattern_char)
				{
					case '*':
						current_state = state::consume_until_next_match;
						++pattern_iter;
						if(pattern_iter == pattern_end)
						{ return true; }
						break;

					default:
						if(match_char != pattern_char)
						{
							current_state = state::consume_until_next_match;
							pattern_iter = match_start;
							++to_match_iter;
						}
						else
						{
							++to_match_iter;
							++pattern_iter;
							if(to_match_iter == match_end && pattern_iter == pattern_end)
							{ return true; }
						}
				}
				break;
			}
		}
	}
	return false;
}

std::vector<slideproj::utils::glob_string>
slideproj::utils::make_glob_strings(std::vector<std::string> const& strings)
{
	std::vector<slideproj::utils::glob_string> ret;
	std::ranges::transform(strings, std::back_inserter(ret), [](auto const& item){
		return glob_string{item};
	});
	return ret;
}