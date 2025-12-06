#ifndef SLIDEPROJ_UTILS_CHARCONV_WRAPPER_HPP
#define SLIDEPROJ_UTILS_CHARCONV_WRAPPER_HPP

#include <string_view>
#include <ranges>
#include <charconv>
#include <optional>
#include <concepts>

namespace slideproj::utils
{
	template<std::integral T>
	constexpr std::optional<T> to_number(std::string_view sv, std::ranges::minmax_result<T> accepted_range)
	{
		T value;
		auto res = std::from_chars(std::begin(sv), std::end(sv), value);
		if(res.ec != std::errc{} || res.ptr != std::end(sv))
		{ return std::nullopt;}

		if(value < accepted_range.min || value > accepted_range.max)
		{ return std::nullopt; }

		return value;
	}
}

#endif