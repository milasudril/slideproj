#ifndef SLIDEPROJ_UTILS_SYNCHRONIZED_HPP
#define SLIDEPROJ_UTILS_SYNCHRONIZED_HPP

#include <shared_mutex>
#include <mutex>

namespace slideproj::utils
{
	template<class T>
	class synchronized
	{
	public:
		template<class Callable>
		auto read(Callable&& func) const
		{
			std::shared_lock lock{m_mutex};
			return std::forward<Callable>(func)(m_value);
		}

		template<class Callable>
		auto write(Callable&& func)
		{
			std::unique_lock lock{m_mutex};
			return std::forward<Callable>(func)(m_value);
		}

	private:
		mutable std::shared_mutex m_mutex;
		T m_value;
	};
}
#endif