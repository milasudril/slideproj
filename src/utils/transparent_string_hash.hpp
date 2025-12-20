#ifndef SLIDEPROJ_UTILS_TRANSPARENT_STRING_HASH_HPP
#define SLIDEPROJ_UTILS_TRANSPARENT_STRING_HASH_HPP

#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace slideproj::utils
{
	struct transparent_string_hash
	{
		using hash_type = std::hash<std::string_view>;
		using is_transparent = void;

		constexpr size_t operator()(const char* str) const
		{ return hash_type{}(str); }

		constexpr size_t operator()(std::string_view str) const
		{ return hash_type{}(str); }

		constexpr size_t operator()(std::string const& str) const
		{ return hash_type{}(str); }
	};

	template<class Value>
	using string_lookup_table = std::unordered_map<
		std::string,
		Value,
		transparent_string_hash,
		std::equal_to<>
	>;

	using string_set = std::unordered_set<
		std::string,
		transparent_string_hash,
		std::equal_to<>
	>;
}

#endif