//@	{"target":{"name":"./file_list.test"}}

#include "./file_list.hpp"

#include "testfwk/testfwk.hpp"

#include <array>

TESTCASE(slideproj_slideshow_source_file_list_load_and_sort)
{
	using from_range_t = slideproj::slideshow_source::file_list::from_range_t;
	slideproj::slideshow_source::file_list items{
		from_range_t{},
		std::array{
			std::filesystem::path{"foo/lenna.jpg"},
			std::filesystem::path{"foo/bar.jpg"},
			std::filesystem::path{"foo/kaka.jpg"}
		}
	};

	EXPECT_EQ(items[0], "foo/lenna.jpg");
	EXPECT_EQ(items[1], "foo/bar.jpg");
	EXPECT_EQ(items[2], "foo/kaka.jpg");

	items.sort([](auto const& a, auto const& b){
		return a < b;
	});

	EXPECT_EQ(items[0], "foo/bar.jpg");
	EXPECT_EQ(items[1], "foo/kaka.jpg");
	EXPECT_EQ(items[2], "foo/lenna.jpg");
}