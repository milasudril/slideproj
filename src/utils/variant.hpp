#ifndef SLIDEPROJ_UTILS_VARIANT_HPP
#define SLIDEPROJ_UTILS_VARIANT_HPP

#include <cstddef>
#include <variant>
#include <array>

namespace slideproj::utils
{
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
			concatenate_variants_t<std::variant<unsigned char>, std::variant<unsigned short>, std::variant<unsigned int>>,
			std::variant<unsigned char, unsigned short, unsigned int>
		>
	);
}
#endif