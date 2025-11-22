//@	{"target":{"name":"slideproj.o"}}

#include "src/file_collector/file_collector.hpp"
#include "src/image_file_loader/image_file_loader.hpp"

int main()
{
	slideproj::image_file_loader::image_file_metadata_repository metadata_repo;
	auto files = slideproj::file_collector::make_file_list(
		// TODO: Use command line arguments
		"/home/torbjorr/Bilder",
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
#if 0
	for(auto const& item : files)
	{
		auto const& metadata = metadata_repo.get_metadata(item);
		printf("%s %s\n", metadata.in_group.c_str(), item.path().c_str());
	}
#endif

	return 0;

}