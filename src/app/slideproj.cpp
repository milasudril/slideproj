//@	{"target":{"name":"slideproj.o"}}

#include "./input_filter.hpp"
#include "./slideshow_window_event_handler.hpp"
#include "./slideshow_playback_controller.hpp"
#include "./slideshow.hpp"
#include "./slideshow_presentation_controller.hpp"

#include "src/pixel_store/rgba_image.hpp"
#include "src/file_collector/file_collector.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/glfw_wrapper/glfw_wrapper.hpp"
#include "src/utils/task_queue.hpp"
#include "src/utils/task_result_queue.hpp"
#include "src/renderer/image_display.hpp"
#include "src/utils/transparent_string_hash.hpp"
#include "src/windowing_api/application_window.hpp"
#include "src/utils/parsed_command_line.hpp"

#include <chrono>

int create_file_list(slideproj::utils::string_lookup_table<std::vector<std::string>> const&)
{ return 0; }

int main(int argc, char** argv)
{
	try
	{
		std::array actions{
			std::pair{
				std::string{"create"},
				slideproj::utils::action_info{
					.main = create_file_list,
					.description = "Creates a list of image files to be used in a slideshow",
					.valid_options = slideproj::utils::string_lookup_table<slideproj::utils::option_info>{
						std::pair{
							"scan-directories",
							slideproj::utils::option_info{
								.description = "Directories to include",
								.default_value = std::vector<std::string>{"."},
								.cardinality = std::numeric_limits<size_t>::max()
							},
						},
						std::pair{
							"inlcude",
							slideproj::utils::option_info{
								.description = "Glob strings used to filter the input",
								.default_value = std::vector{
									std::string{"*.jpeg"},
									std::string{"*.jpg"},
									std::string{"*.bmp"},
									std::string{"*.png"},
									std::string{"*.exr"},
									std::string{"*.gif"},
									std::string{"*.tif"},
									std::string{"*.tiff"}
								},
								.cardinality = std::numeric_limits<size_t>::max()
							}
						},
						std::pair{
							"max-pixel-count",
							slideproj::utils::option_info{
								.description = "The maximum number of pixels in individual images",
								.default_value = std::vector{std::string{"33554432"}},
								.cardinality = 1
							}
						},
						std::pair{
							"order-by",
							slideproj::utils::option_info{
								.description = "The fields used to sort the file list in order of importance",
								.default_value = std::vector{std::string{"in_group"}, std::string{"timestamp"}},
								.cardinality = std::numeric_limits<size_t>::max(),
								.valid_values = slideproj::utils::string_set{"in_group", "timestamp", "caption"}
							}
						},
						std::pair{
							"output-files",
							slideproj::utils::option_info{
								.description = "A list of files to write the list of files to",
								.default_value = std::vector{std::string{"/dev/stdout"}},
								.cardinality = std::numeric_limits<size_t>::max()
							}
						},
					}
				}
			}
		};

		slideproj::utils::parsed_command_line cmdline{
			"slideproj",
			std::span{static_cast<char const* const*>(argv), static_cast<size_t>(argc)},
			slideproj::utils::string_lookup_table<slideproj::utils::action_info>{
				std::make_move_iterator(std::begin(actions)),
				std::make_move_iterator(std::end(actions))
			}
		};

		return cmdline.execute();
#if 0
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
		slideproj::app::slideshow_playback_controller playback_ctrl{
			slideproj::app::slideshow_playback_descriptor{
				.step_delay = std::chrono::seconds{6},
				.step_direction = slideproj::app::step_direction::forward
			}
		};
		slideproj::app::slideshow_presentation_controller slideshow_presentation_controller{
			pending_tasks,
			img_display,
			*main_window,
			std::cref(metadata_repo),
			playback_ctrl,
			slideproj::app::slideshow_presentation_descriptor{
				.transition_duration = std::chrono::seconds{2},
				.loop = true
			}
		};
		slideproj::app::slideshow_window_event_handler eh{
			slideshow_presentation_controller,
			slideproj::app::make_image_rect_sink_refs(slideshow_presentation_controller, img_display),
			playback_ctrl
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
#endif
	}
	catch(std::exception const& err)
	{
		fprintf(stderr, "(x) Error: %s\n", err.what());
		return -1;
	}
}