#ifndef SLIDEPROJ_UTILS_UNWRAP_HPP
#define SLIDEPROJ_UTILS_UNWRAP_HPP

#include <functional>

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