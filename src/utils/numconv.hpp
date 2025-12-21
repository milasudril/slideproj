#ifndef SLIDEPROJ_UTILS_NUMCONV_HPP
#define SLIDEPROJ_UTILS_NUMCONV_HPP

#include <string_view>
#include <algorithm>
#include <charconv>
#include <optional>
#include <concepts>
#include <type_traits>

namespace slideproj::utils
{
	template<class T>
	requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
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

	template<std::unsigned_integral T>
	inline constexpr float to_normalized_float(T value)
	{
		return static_cast<float>(value)
			/static_cast<float>(std::numeric_limits<T>::max());
	}

	inline constexpr float to_normalized_float(float value)
	{ return value; }
}

#endif