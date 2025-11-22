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

	template<class T>
	concept bitmask_enum = std::is_enum_v<T> &&
	requires(T e)
	{
		{enable_bitmask_operators(e)};
	};

	template<class T>
	requires(std::is_enum_v<T>)
	constexpr auto to_underlying(T value)
	{
		using underlying = std::underlying_type_t<T>;
		return static_cast<underlying>(value);
	}

	template<bitmask_enum T>
	constexpr T operator~(T value)
	{ return static_cast<T>(~to_underlying(value)); }

	template<bitmask_enum T>
	constexpr T operator|(T a, T b)
	{ return static_cast<T>(to_underlying(a) | to_underlying(b)); }

	template<bitmask_enum T>
	constexpr T operator&(T a, T b)
	{ return static_cast<T>(to_underlying(a) & to_underlying(b)); }

	template<bitmask_enum T>
	constexpr T operator^(T a, T b)
	{ return static_cast<T>(to_underlying(a) ^ to_underlying(b)); }

	template<bitmask_enum T>
	constexpr T& operator|=(T& a, T b)
	{ return a = a | b; }

	template<bitmask_enum T>
	constexpr T& operator&=(T& a, T b)
	{ return a = a & b; }

	template<bitmask_enum T>
	constexpr T& operator^=(T& a, T b)
	{ return a = a ^ b; }

	template<bitmask_enum T>
	constexpr auto is_set(T a, T value)
	{ return static_cast<bool>(a & value); }
}
#endif