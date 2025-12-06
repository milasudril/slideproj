#ifndef SLIDEPROJ_UTILS_UTILS_HPP
#define SLIDEPROJ_UTILS_UTILS_HPP

#include <concepts>
#include <optional>
#include <type_traits>
#include <string_view>
#include <charconv>
#include <algorithm>
#include <array>
#include <cstddef>
#include <variant>
#include <cstdint>

namespace slideproj::utils
{
	template<class T>
	inline constexpr T& unwrap(std::reference_wrapper<T> obj)
	{ return obj.get(); }

	template<class T>
	inline constexpr T&& unwrap(T&& obj)
	{ return std::forward<T>(obj); }
}
#endif