#ifndef SLIDEPROJ_UTILS_SYNCHRONIZED_HPP
#define SLIDEPROJ_UTILS_SYNCHRONIZED_HPP

#include <mutex>

namespace slideproj::utils
{
	template<class T>
	class synchronized
	{
	public:
		synchronized& operator=(T&& value)
		{
			std::lock_guard lock{m_mutex};
			m_value = std::move(value);
			return *this;
		}

		T take_value()
		{
			std::lock_guard lock{m_mutex};
			return std::move(m_value);
		}

	private:
		mutable std::mutex m_mutex;
		T m_value;
	};
}
#endif