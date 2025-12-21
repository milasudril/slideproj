//@	{"target":{"name":"slideproj.o"}}

#include "./input_filter.hpp"
#include "./slideshow_window_event_handler.hpp"
#include "./slideshow_playback_controller.hpp"
#include "./slideshow.hpp"
#include "./slideshow_presentation_controller.hpp"

#include "src/config/user_dir_provider.hpp"
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
#include <nlohmann/adl_serializer.hpp>
#include <nlohmann/detail/output/serializer.hpp>
#include <nlohmann/json.hpp>

int create_file_list(slideproj::utils::string_lookup_table<std::vector<std::string>> const& args)
{
	fprintf(stderr, "(i) Creating list of files\n");
	slideproj::image_file_loader::image_file_metadata_repository metadata_repo;
	auto const maxnum_pixels = slideproj::utils::to_number(
		args.at("max-pixel-count").at(0),
		std::ranges::minmax_result{1U, 1'073'741'824U}
	);
	if(!maxnum_pixels.has_value())
	{ throw std::runtime_error{"Invalid value for max-pixel-count. Value should be within 1 and 2^30."}; }

	auto const file_list = slideproj::file_collector::make_file_list(
		args.at("scan-directories"),
		slideproj::app::input_filter{
			.include = slideproj::utils::make_glob_strings(args.at("include")),
			.max_pixel_count = *maxnum_pixels,
				.image_dimension_provider = std::cref(metadata_repo)
			},
		slideproj::file_collector::make_metadata_field_array(args.at("order-by")),
		metadata_repo,
		[](auto const& a, auto const& b) {
			auto const res = strcoll(a.c_str(), b.c_str());
			if(res < 0)
			{ return std::strong_ordering::less; }
			if(res == 0)
			{ return std::strong_ordering::equal; }
			return std::strong_ordering::greater;
		}
	);

	fprintf(stderr, "(i) Collected %zu files\n", file_list.size());

	nlohmann::json to_serialize;
	nlohmann::json slideproj_create_opts;
	slideproj_create_opts.emplace("max_pixel_count", *maxnum_pixels);
	slideproj_create_opts.emplace("order_by", args.at("order-by"));
	slideproj_create_opts.emplace("include", args.at("include"));
	slideproj_create_opts.emplace("scan_directories", args.at("scan-directories"));
	to_serialize.emplace("slideproj_create_opts", std::move(slideproj_create_opts));

	nlohmann::json serialized_file_list;
	for(auto const& item : file_list)
	{
		nlohmann::json entry;
		entry.emplace("path", item.path());
		serialized_file_list.push_back(std::move(entry));
	}
	to_serialize.emplace("files", std::move(serialized_file_list));

	std::ofstream output{args.at("output-file").at(0)};
	output << std::setw(2) << to_serialize << '\n';

	return 0;
}

int show_file_list(slideproj::utils::string_lookup_table<std::vector<std::string>> const& args)
{
	nlohmann::json serialized_file_list;
	{
		std::ifstream input{args.at("file").at(0)};
		if(!input.is_open())
		{ throw std::runtime_error{std::format("Error while trying to open file list: {}", strerror(errno))}; }
		input >> serialized_file_list;
	}

	auto const i = serialized_file_list.find("files");
	if(i == std::end(serialized_file_list))
	{ throw std::runtime_error{"No file list present in the input"}; }
	if(!i->is_array())
	{ throw std::runtime_error{"The list of files should be an array"}; }

	slideproj::file_collector::file_list file_list;
	for(auto const& item: *i)
	{
		auto const j = item.find("path");
		if(j == std::end(item))
		{ continue; }

		auto str = j->get_ptr<nlohmann::json::string_t const*>();
		if(str == nullptr)
		{ continue; }

		fprintf(stderr, "%s\n", str->c_str());
	}

	return 0;
}

int main(int argc, char** argv)
{
	try
	{
		setlocale(LC_ALL,"");
		auto user_dirs = slideproj::config::get_user_dirs();

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
								.default_value = std::vector<std::string>{user_dirs.pictures},
								.cardinality = std::numeric_limits<size_t>::max()
							},
						},
						std::pair{
							"include",
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
							"output-file",
							slideproj::utils::option_info{
								.description = "The file to save the list of files to",
								.default_value = std::vector{std::string{"/dev/stdout"}},
								.cardinality = 1
							}
						},
					}
				}
			},
			std::pair{
				std::string{"show"},
				slideproj::utils::action_info{
					.main = show_file_list,
					.description = "Shows a slideshow, given a file created by the create action",
					.valid_options = slideproj::utils::string_lookup_table<slideproj::utils::option_info>{
						std::pair{
							"file",
							slideproj::utils::option_info{
								.description = "The file to show",
								.default_value = std::vector<std::string>{"/dev/stdin"},
								.cardinality = 1
							}
						},
						std::pair{
							"fullscreen",
							slideproj::utils::option_info{
								.description = "Enables sets fullscreen mode at startup",
								.default_value = std::vector<std::string>{"no"},
								.cardinality = 1,
								.valid_values = slideproj::utils::string_set{"no", "yes"}
							}
						},
						std::pair{
							"hide-cursor",
							slideproj::utils::option_info{
								.description = "Hides the cursor at startup",
								.default_value = std::vector<std::string>{"no"},
								.cardinality = 1,
								.valid_values = slideproj::utils::string_set{"no", "yes"}
							}
						},
						std::pair{
							"step-delay",
							slideproj::utils::option_info{
								.description = "The time in seconds to wait before showing the next image",
								.default_value = std::vector<std::string>{"6"},
								.cardinality = 1,
							}
						},
						std::pair{
							"transition-duration",
							slideproj::utils::option_info{
								.description = "The time in seconds for transitions",
								.default_value = std::vector<std::string>{"2"},
								.cardinality = 1,
							}
						},
						std::pair{
							"step-direction",
							slideproj::utils::option_info{
								.description = "The direction to step",
								.default_value = std::vector{std::string{"forward"}},
								.cardinality = 1,
								.valid_values = slideproj::utils::string_set{"forward", "backward", "paused"}
							}
						},
						std::pair{
							"start-at",
							slideproj::utils::option_info{
								.description = "The index to start at, clamped to a valid range",
								.default_value = std::vector{std::string{"saved"}},
								.cardinality = 1
							}
						},
						std::pair{
							"loop",
							slideproj::utils::option_info{
								.description = "Enables loop mode (goes back to beginning/end at last/first entry",
								.default_value = std::vector<std::string>{"no"},
								.cardinality = 1,
								.valid_values = slideproj::utils::string_set{"no", "yes"}
							}
						},
						std::pair{
							"savestate-file",
							slideproj::utils::option_info{
								.description = "Sets the file used for state storage (the active slide at exit)",
								.default_value = std::vector<std::string>{user_dirs.savestates/"slideproj.json"},
								.cardinality = 1
							}
						}
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