#ifndef SLIDEPROJ_UTILS_TASK_QUEUE_HPP
#define SLIDEPROJ_UTILS_TASK_QUEUE_HPP

#include <functional>
#include <thread>
#include <queue>

namespace slideproj::utils
{
	class task_queue
	{
	public:
		task_queue():
			m_worker{
				[this](){
					run_tasks();
				}
			}
		{}

		template<class Callable>
		void submit(Callable&& func)
		{
			std::lock_guard lock{m_mutex};
			m_tasks.push(std::forward<Callable>(func));
			m_cv.notify_one();
		}

		void clear()
		{
			std::lock_guard lock{m_mutex};
			m_tasks = std::queue<std::function<void()>>{};
			m_cv.notify_one();
		}

		~task_queue()
		{
			m_should_stop = true;
			m_cv.notify_one();
			m_worker.join();
		}

	private:
		void run_tasks()
		{
			while(true)
			{
				std::unique_lock lock{m_mutex};
				m_cv.wait(
					lock,
					[this](){
						return m_should_stop == true || !m_tasks.empty();
					}
				);

				if(m_should_stop == true)
				{ return; }

				auto task_to_run = std::move(m_tasks.front());
				m_tasks.pop();
				lock.unlock();
				try
				{ task_to_run(); }
				catch(std::exception const& exception)
				{ fprintf(stderr, "%s", exception.what()); }
			}
		}

		std::mutex m_mutex;
		std::condition_variable m_cv;
		std::queue<std::function<void()>> m_tasks;
		std::atomic<bool> m_should_stop{false};
		std::thread m_worker;
	};
}

#endif