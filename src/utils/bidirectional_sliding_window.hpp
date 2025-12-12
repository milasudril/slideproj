#ifndef SLIDEPROJ_UTILS_BIDIRECTIONAL_SLIDING_WINDOW_HPP
#define SLIDEPROJ_UTILS_BIDIRECTIONAL_SLIDING_WINDOW_HPP

#include <array>
#include <utility>
#include <cstddef>
#include <limits>
#include <cstdio>

namespace slideproj::utils
{
	template<class T, size_t Radius>
	class bidirectional_sliding_window
	{
	public:
		static constexpr auto elem_count = 2*Radius + 1;

		bidirectional_sliding_window() = default;

		explicit bidirectional_sliding_window(std::array<T, elem_count>&& vals):
			m_values{std::move(vals)}
		{}

		void step_forward()
		{
			m_index_to_replace = (elem_count + m_current_index - Radius)%elem_count;
			m_current_index = (m_current_index + 1)%elem_count;
		}

		void step_backward()
		{
			m_index_to_replace = (m_current_index + Radius)%elem_count;
			m_current_index = (elem_count + m_current_index - 1)%elem_count;
		}

		T const& get_current_element() const
		{
			fprintf(stderr, "get_current_element: current_index = %zu,  index_to_replace = %zu\n", m_current_index, m_index_to_replace);
			return m_values[m_current_index];

		}

		// TODO: C++26 optional reference
		T* get_element_to_replace()
		{
			fprintf(stderr, "get_element_to_replace: index_to_replace = %zu\n", m_index_to_replace);
			if(m_index_to_replace == std::numeric_limits<size_t>::max())
			{ return nullptr; }

			return &m_values[m_index_to_replace];
		}

		auto const& get_values() const
		{ return m_values; }

	private:
		std::array<T, elem_count> m_values;
		size_t m_current_index = elem_count/2;
		size_t m_index_to_replace = std::numeric_limits<size_t>::max();
	};
}
#endif