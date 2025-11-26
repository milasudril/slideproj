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

	template<class T>
	struct make_variant_type_tag
	{ using type = T; };

	template<size_t Index, class VariantType, class Factory>
	consteval void fill_make_variant_vtable(
		std::array<VariantType (*)(Factory&&), std::variant_size_v<VariantType>>& vtable
	)
	{
		if constexpr(Index == std::variant_size_v<VariantType>)
		{ return; }
		else
		{
			using type = std::variant_alternative_t<Index, VariantType>;
			using type_tag =  make_variant_type_tag<type>;
			static_assert(
				std::is_same_v<std::invoke_result_t<Factory, type_tag>, type>,
				"Factory returns wrong type"
			);
			vtable[Index] = [](Factory&& f) {
				return VariantType(std::forward<Factory>(f)(make_variant_type_tag<type>{}));
			};
			return fill_make_variant_vtable<Index + 1>(vtable);
		}
	}

	template<class VariantType, class Factory>
	consteval auto create_make_variant_vtable()
	{
			std::array<VariantType (*)(Factory&&), std::variant_size_v<VariantType>> ret{};
			fill_make_variant_vtable<0>(ret);
			return ret;
	}

	template<class VariantType, class Factory>
	constexpr auto make_variant_vtable = create_make_variant_vtable<VariantType, Factory>();

	template<class VariantType, class Factory>
	constexpr auto make_variant(size_t kind, Factory&& factory)
	{
		constexpr auto& vtable = make_variant_vtable<VariantType, Factory>;
		if(kind >= std::size(vtable)) [[unlikely]]
		{ return VariantType{}; }

		return vtable[kind](std::forward<Factory>(factory));
	}
}
#endif