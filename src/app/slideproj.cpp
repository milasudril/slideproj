//@	{"target":{"name":"slideproj.o"}}

#include "./input_filter.hpp"

#include "src/file_collector/file_collector.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/image_presenter/image_presenter.hpp"

int main()
{
	slideproj::image_presenter::glfw_context gui_ctxt;
	gui_ctxt.select_opengl_version(
		slideproj::image_presenter::renderer_version{
			.major = 4,
			.minor = 6
		}
	);
	slideproj::image_presenter::application_window main_window{gui_ctxt};
	auto& gl_ctxt = main_window.activate_render_context();
	fprintf(
		stderr,
		"(i) Initialized OpenGL. Vendor = %s, Renderer = %s, Version = %s\n",
		glGetString(GL_VENDOR),
		glGetString(GL_RENDERER),
		glGetString(GL_VERSION)
	);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	gl_ctxt.enable_vsync();

#if 0
	slideproj::image_file_loader::image_file_metadata_repository metadata_repo;
	auto files = slideproj::file_collector::make_file_list(
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
#endif

	while(true)
	{
		gui_ctxt.poll_events();
		main_window.swap_buffers();
	}

#if 0
	for(auto const& item : files)
	{
		auto const& metadata = metadata_repo.get_metadata(item);
		printf("%s %s\n", metadata.in_group.c_str(), item.path().c_str());
	}
#endif

	return 0;

}