#ifndef SLIDEPROJ_UTILS_UTILS_HPP
#define SLIDEPROJ_UTILS_UTILS_HPP

#include <concepts>
#include <optional>
#include <type_traits>
#include <string_view>
#include <charconv>
#include <algorithm>

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