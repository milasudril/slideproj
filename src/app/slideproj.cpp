//@	{"target":{"name":"slideproj.o"}}

#include "./input_filter.hpp"
#include "./slideshow_window_event_handler.hpp"
#include "./slideshow.hpp"

#include "src/app/slideshow_controller.hpp"
#include "src/pixel_store/rgba_image.hpp"
#include "src/file_collector/file_collector.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/glfw_wrapper/glfw_wrapper.hpp"
#include "src/utils/task_queue.hpp"
#include "src/utils/task_result_queue.hpp"
#include "src/renderer/image_display.hpp"

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

	slideproj::app::slideshow slideshow;
	slideproj::utils::task_result_queue task_results;
	slideproj::utils::task_queue pending_tasks{task_results};
	slideproj::renderer::image_display img_display{};
	slideproj::app::slideshow_controller slideshow_controller{pending_tasks, img_display};
	slideproj::app::slideshow_window_event_handler eh{slideshow_controller};
	main_window->set_event_handler(std::ref(eh));

	slideproj::file_collector::file_list file_list;
	pending_tasks.submit(
		slideproj::utils::task{
			.function = []() {
				slideproj::image_file_loader::image_file_metadata_repository metadata_repo;
				return slideproj::file_collector::make_file_list(
					// TODO: Use command line arguments
					"/home/torbjorr/Bilder",
					slideproj::app::input_filter{
						.include = std::vector{
							slideproj::app::input_filter_pattern{"*.jpg"},
							slideproj::app::input_filter_pattern{"*.jpeg"},
							slideproj::app::input_filter_pattern{"*.bmp"},
							slideproj::app::input_filter_pattern{"*.gif"},
							slideproj::app::input_filter_pattern{"*.png"}
						},
						.max_pixel_count = 1024*1024,
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
	auto t_start = std::chrono::steady_clock::now();
	while(!eh.application_should_exit())
	{
		auto now = std::chrono::steady_clock::now();
		task_results.drain();
		if(slideshow.empty())
		{
			if(!file_list.empty())
			{
				fprintf(stderr, "\n");
				slideshow = slideproj::app::slideshow{std::move(file_list)};
				eh.handle_event(
					slideproj::app::slideshow_loaded{
						.current_slideshow = std::ref(slideshow)
					}
				);
			}
			else
			{ fprintf(stderr,"\r(i) Collecting files %c", progress_char[k%8]); }
		}


		eh.handle_event(
			slideproj::app::update_window{
				.frame_number = k,
				.time_since_last_frame = now - t_start
			}
		);

		main_window->poll_events();
		glClear(GL_COLOR_BUFFER_BIT);
		main_window->swap_buffers();
		t_start = now;
		++k;
	}

	return 0;
}