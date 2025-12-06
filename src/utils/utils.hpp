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

	template<class VariantType, class... TypesToAppend>
	struct append_to_variant
	{
	private:
		template<size_t... I>
		static consteval auto resolve_type(std::index_sequence<I...>)
		{
			return std::type_identity<
				std::variant<
					std::variant_alternative_t<I, VariantType>...,
					TypesToAppend...
				>
			>{};
		}
	public:
		using type = decltype(
			resolve_type(std::make_index_sequence<std::variant_size_v<VariantType>>{})
		)::type;
	};

	template<class VariantType, class... TypesToAppend>
	using append_to_variant_t = append_to_variant<VariantType,TypesToAppend...>::type;

	template<class VariantA, class VariantB, class... Tail>
	struct concatenate_variants
	{
	private:
		template<size_t... I>
		static consteval auto resolve_type(std::index_sequence<I...>)
		{
			using next_type = append_to_variant_t<
				VariantA,
				std::variant_alternative_t<I, VariantB>...
			>;

			if constexpr(sizeof...(Tail) == 0)
			{ return std::type_identity<next_type>{}; }
			else
			{ return std::type_identity<typename concatenate_variants<next_type, Tail...>::type>{}; }
		}
	public:
		using type = decltype(
			resolve_type(std::make_index_sequence<std::variant_size_v<VariantB>>{})
		)::type;
	};

	template<class VariantA, class VariantB, class... Tail>
	using concatenate_variants_t = concatenate_variants<VariantA, VariantB, Tail...>::type;

	static_assert(
		std::is_same_v<
			concatenate_variants_t<std::variant<uint8_t>, std::variant<uint16_t>, std::variant<uint32_t>>,
			std::variant<uint8_t, uint16_t, uint32_t>
		>
	);

	template<std::unsigned_integral T>
	inline constexpr float to_normalized_float(T value)
	{
		return static_cast<float>(value)
			/static_cast<float>(std::numeric_limits<T>::max());
	}

	inline constexpr float to_normalized_float(float value)
	{ return value; }

	template<class T>
	inline constexpr T& unwrap(std::reference_wrapper<T> obj)
	{ return obj.get(); }

	template<class T>
	inline constexpr T&& unwrap(T&& obj)
	{ return std::forward<T>(obj); }
}
#endif