//@	{"target":{"name":"image_file_loader.test"}}

#include "./image_file_loader.hpp"

#include "src/file_collector/file_collector.hpp"
#include "testfwk/testfwk.hpp"
#include "testfwk/testsuite.hpp"
#include "testfwk/validation.hpp"

TESTCASE(slideproj_image_file_get_metadata)
{
	slideproj::image_file_loader::image_file_metadata_repository repo;
	auto const& res = repo.get_metadata(
		slideproj::file_collector::file_list_entry{
			slideproj::file_collector::file_id{0},
			std::filesystem::path{"/dev/shm/img_0001.jpg"}
		}
	);

	auto timestamp = std::format("{}", res.timestamp.time_since_epoch().count());
	printf("%s %s %s\n", timestamp.c_str(), res.caption.c_str(), res.in_group.c_str());
	//EXPECT_EQ(timestamp, "9223372036854775807999999999");
}