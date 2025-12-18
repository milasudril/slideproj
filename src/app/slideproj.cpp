//@	{"target":{"name":"slideproj.o"}}

#include "./input_filter.hpp"
#include "./slideshow_window_event_handler.hpp"
#include "./slideshow.hpp"

#include "src/app/slideshow_presentation_controller.hpp"
#include "src/pixel_store/rgba_image.hpp"
#include "src/file_collector/file_collector.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/glfw_wrapper/glfw_wrapper.hpp"
#include "src/utils/task_queue.hpp"
#include "src/utils/task_result_queue.hpp"
#include "src/renderer/image_display.hpp"
#include "src/windowing_api/application_window.hpp"
#include <chrono>
#include <linux/stat.h>
#include <valarray>

namespace slideproj::app
{
	struct slideshow_navigator_scheduler
	{
	public:
		void handle_event(slideshow_navigator&, slideshow_step_event event)
		{
			fprintf(stderr, "(i) Step\n");
			if(event.direction != step_direction::none)
			{ m_direction = event.direction; }
			m_latest_transtion_end.reset();
		}

		void handle_event(slideshow_navigator&, slideshow_transition_end_event event)
		{
			fprintf(stderr, "(i) Transition ended\n");
			m_latest_transtion_end = event.when;
		}

		void handle_event(slideshow_navigator& navigator, slideshow_time_event event)
		{
			if(m_latest_transtion_end.has_value() && m_direction != step_direction::none)
			{
				if(event.when - *m_latest_transtion_end >= m_step_delay)
				{
					fprintf(stderr, "(i) Frame expired\n");
					switch (m_direction)
					{
						case step_direction::forward:
							navigator.step_forward();
							break;
						case step_direction::backward:
							navigator.step_backward();
							break;
						default:
							break;
					}
				}
			}
		}

		void toggle_pause()
		{
			if(m_direction == step_direction::none)
			{ m_direction = m_saved_direction; }
			else
			{ m_saved_direction = std::exchange(m_direction, step_direction::none); }
		}


	private:
		std::chrono::duration<float> m_step_delay{6.0f};
		std::optional<slideshow_clock::time_point> m_latest_transtion_end;
		step_direction m_direction{step_direction::forward};
		step_direction m_saved_direction{step_direction::forward};
	};
}

int main()
{
	auto main_window = slideproj::glfw_wrapper::glfw_window::create("slideproj");
	fprintf(
		stderr,
		"(i) Initialized OpenGL. Vendor = %s, Renderer = %s, Version = %s\n",
		glGetString(GL_VENDOR),
		glGetString(GL_RENDERER),
		glGetString(GL_VERSION)
	);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	slideproj::app::slideshow slideshow;
	slideproj::utils::task_result_queue task_results;
	slideproj::utils::task_queue pending_tasks{task_results};
	slideproj::renderer::image_display img_display{};
	slideproj::image_file_loader::image_file_metadata_repository metadata_repo;
	slideproj::app::slideshow_navigator_scheduler nav_sched;
	slideproj::app::slideshow_presentation_controller slideshow_presentation_controller{
		pending_tasks,
		img_display,
		*main_window,
		std::cref(metadata_repo),
		nav_sched,
		std::chrono::seconds{2}
	};
	slideproj::app::slideshow_window_event_handler eh{
		slideshow_presentation_controller,
		slideproj::app::make_image_rect_sink_refs(slideshow_presentation_controller, img_display),
		nav_sched
	};
	main_window->set_event_handler(std::ref(eh));

	slideproj::file_collector::file_list file_list;
	pending_tasks.submit(
		slideproj::utils::task{
			// Safe to pass a reference to metadata_repo since slideshow controller will not touch
			// anything until the slideshow has been loaded.
			.function = [&metadata_repo]() {
				return slideproj::file_collector::make_file_list(
					// TODO: Use command line arguments
					"/home/torbjorr/Bilder",
					slideproj::app::input_filter{
						.include = std::vector{
							slideproj::utils::glob_string{"*.jpg"},
							slideproj::utils::glob_string{"*.jpeg"},
							slideproj::utils::glob_string{"*.bmp"},
							slideproj::utils::glob_string{"*.gif"},
							slideproj::utils::glob_string{"*.png"}
						},
						.max_pixel_count = 8192*8192,
						.image_dimension_provider = std::cref(metadata_repo)
					},
					std::array{
						slideproj::file_collector::file_metadata_field::in_group,
						slideproj::file_collector::file_metadata_field::timestamp
					},
					metadata_repo,
					[](auto a, auto b) {
						// TODO: Should use a local-correct comparison (or compare code.points rather than code-units)
						return a <=> b;
					}
				);
			},
			.on_completed = [&file_list](auto&& result) {
				file_list = std::move(result);
			}
		}
	);

	size_t k = 0;
	constexpr char const* progress_char = "-/|\\-/|\\";
	while(!eh.application_should_exit())
	{
		auto const now = std::chrono::steady_clock::now();
		task_results.drain();
		if(slideshow.empty())
		{
			if(!file_list.empty())
			{
				fprintf(stderr, "\n");
				slideshow = slideproj::app::slideshow{std::move(file_list)};
				slideshow_presentation_controller.start_slideshow(slideshow);
			}
			else
			{ fprintf(stderr,"\r(i) Collecting files %c", progress_char[k%8]); }
		}

		slideshow_presentation_controller.update_clock(now);

		main_window->poll_events();
		glClear(GL_COLOR_BUFFER_BIT);
		img_display.update();
		main_window->swap_buffers();
		++k;
	}

	return 0;
}