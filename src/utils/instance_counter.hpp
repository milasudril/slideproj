#ifndef SLIDEPROJ_UTILS_INSTANCE_COUNTER_HPP
#define SLIDEPROJ_UTILS_INSTANCE_COUNTER_HPP

#include <cstddef>

namespace slideproj::utils
{
	template<class T>
	class instance_counter
	{
	public:
		instance_counter()
		{ ++m_value; }

		~instance_counter()
		{ --m_value; }

		size_t value() const
		{ return m_value; }

	private:
		inline static size_t m_value = 0;
	};
}
#endif