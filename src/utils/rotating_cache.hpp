#ifndef SLIDEPROJ_UTILS_BIDIRECTIONAL_SLIDING_WINDOW_HPP
#define SLIDEPROJ_UTILS_BIDIRECTIONAL_SLIDING_WINDOW_HPP

#include <array>
#include <utility>

namespace slideproj::utils
{
	struct power_of_two
	{
		constexpr bool operator==(power_of_two const&) const = default;
		constexpr bool operator!=(power_of_two const&) const = default;

		constexpr size_t value() const
		{ return static_cast<size_t>(1) << m_val; }

		constexpr explicit power_of_two(size_t val):
			m_val{val}
		{}

		constexpr power_of_two() = default;

		size_t m_val;
	};

	template<class T, power_of_two Capacity>
	class rotating_cache
	{
	public:
		static constexpr auto elem_count = Capacity.value();

		auto& operator[](size_t index)
		{ return m_items[index%elem_count]; }

		auto const& operator[](size_t index) const
		{ return m_items[index%elem_count]; }


	private:
		std::array<std::optional<T>, elem_count> m_items;
	};
}
#endif