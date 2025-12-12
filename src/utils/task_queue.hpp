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

	template<class T>
	concept task_result_buffer = requires(T& obj, task_completion_handler&& result)
	{
		{obj.push(std::move(result))} -> std::same_as<void>;
	};

	struct type_erased_task_result_buffer
	{
		void* object;
		void (*push)(void* object, task_completion_handler&& result);
	};

	class task_queue
	{
	public:
		template<task_result_buffer ResultBuffer>
		task_queue(ResultBuffer& res_buffer):
			m_worker{
				[this](){
					run_tasks();
				}
			},
			m_result_buffer{
				.object = &res_buffer,
				.push = [](void* object, task_completion_handler&& result) {
					static_cast<ResultBuffer*>(object)->push(std::move(result));
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
					return task_completion_handler{
						[
							on_completed = std::move(on_completed),
							result = function()
						]() mutable {
							on_completed(std::move(result));
						}
					};
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
					m_result_buffer.push(m_result_buffer.object, task_to_run());
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

		type_erased_task_result_buffer m_result_buffer;
	};
}

#endif