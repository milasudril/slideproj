#ifndef SLIDEPROJ_UTILS_TASK_QUEUE_HPP
#define SLIDEPROJ_UTILS_TASK_QUEUE_HPP

#include <concepts>
#include <functional>
#include <thread>
#include <queue>

namespace slideproj::utils
{
	class task_completion_handler
	{
	public:
		template<class Callable>
		explicit task_completion_handler(Callable&& func):
			m_handler{std::forward<Callable>(func)}
		{}

		void finalize()
		{ m_handler(); }

	private:
		std::move_only_function<void()> m_handler{};
	};

	template<class Function, class OnCompleted>
	struct task
	{
		Function function;
		OnCompleted on_completed;
	};

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

		template<class Function, class OnCompleted>
		void submit(task<Function, OnCompleted>&& func)
		{
			std::lock_guard lock{m_task_queue_mtx};
			m_tasks.push(
				[
					on_completed = std::move(func.on_completed),
					function = std::move(func.function)
				]() mutable {
					return task_completion_handler{std::bind_front(std::move(on_completed), function())};
				}
			);
			m_task_queue_cv.notify_one();
		}

		~task_queue()
		{
			m_should_stop = true;
			m_task_queue_cv.notify_one();
			m_worker.join();
		}

		void finalize_completed_tasks()
		{
			while(true)
			{
				std::unique_lock completed_tasks_lock{m_completed_tasks_mtx};
				if(m_completed_tasks.empty())
				{ return; }

				auto completed_task = std::move(m_completed_tasks.front());
				m_completed_tasks.pop();
				completed_tasks_lock.unlock();

				completed_task.finalize();
			}
		}

	private:
		void run_tasks()
		{
			while(true)
			{
				std::unique_lock task_queue_lock{m_task_queue_mtx};
				m_task_queue_cv.wait(
					task_queue_lock,
					[this](){
						return m_should_stop == true || !m_tasks.empty();
					}
				);
				if(m_should_stop == true)
				{ return; }

				auto task_to_run = std::move(m_tasks.front());
				m_tasks.pop();
				task_queue_lock.unlock();
				try
				{
					auto task_result = task_to_run();
					std::lock_guard completed_tasks_lock{m_completed_tasks_mtx};
					m_completed_tasks.push(std::move(task_result));
				}
				catch(std::exception const& exception)
				{ fprintf(stderr, "%s", exception.what()); }
			}
		}

		std::mutex m_task_queue_mtx;
		std::condition_variable m_task_queue_cv;
		std::queue<std::move_only_function<task_completion_handler()>> m_tasks;
		std::atomic<bool> m_should_stop{false};
		std::thread m_worker;

		std::mutex m_completed_tasks_mtx;
		std::queue<task_completion_handler> m_completed_tasks;
	};
}

#endif