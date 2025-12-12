#ifndef SLIDEPROJ_UTILS_TASK_RESULT_BUFFER_HPP
#define SLIDEPROJ_UTILS_TASK_RESULT_BUFFER_HPP

#include "./task_queue.hpp"

#include <queue>
#include <mutex>

namespace slideproj::utils
{
	class task_result_queue
	{
	public:
		void push(task_completion_handler&& obj)
		{
			std::lock_guard lock{m_mutex};
			m_queue.push(std::move(obj));
		}

		void drain()
		{
			while(true)
			{
				std::unique_lock lock{m_mutex};
				if(m_queue.empty())
				{ return; }
				auto val = std::move(m_queue.front());
				m_queue.pop();
				lock.unlock();
				val.finalize();
			}
		}

	private:
		std::mutex m_mutex;
		std::queue<task_completion_handler> m_queue;
	};
}

#endif