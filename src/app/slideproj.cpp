//@	{"target":{"name":"slideproj.o"}}

#include "./input_filter.hpp"

#include "src/file_collector/file_collector.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/image_presenter/image_presenter.hpp"

int main()
{
	slideproj::image_file_loader::image_file_metadata_repository metadata_repo;
	auto files = slideproj::file_collector::make_file_list(
		// TODO: Use command line arguments
		"/home/torbjorr/Bilder",
		slideproj::app::input_filter{},
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

	slideproj::image_presenter::glfw_context gui_ctxt;
#if 0
	for(auto const& item : files)
	{
		auto const& metadata = metadata_repo.get_metadata(item);
		printf("%s %s\n", metadata.in_group.c_str(), item.path().c_str());
	}
#endif

	return 0;

}