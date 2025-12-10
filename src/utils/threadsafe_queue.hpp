#ifndef SLIDEPROJ_UTILS_THREADSAFE_QUEUE_HPP
#define SLIDEPROJ_UTILS_THREADSAFE_QUEUE_HPP

#include <thread>
#include <queue>
#include <mutex>
#include <optional>

namespace slideproj::utils
{
	template<class T>
	class threadsafe_queue
	{
	public:
		void push(T&& obj)
		{
			std::lock_guard lock{m_mutex};
			m_queue.push(std::move(obj));
		}

		std::optional<T> try_pop()
		{
			std::lock_guard lock{m_mutex};
			if(m_queue.empty())
			{ return std::nullopt; }
			auto ret = std::move(m_queue.front());
			m_queue.pop();
			return ret;
		}


	private:
		std::mutex m_mutex;
		std::queue<T> m_queue;
	};
}

#endif