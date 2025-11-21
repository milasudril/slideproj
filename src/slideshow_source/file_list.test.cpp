//@	{"target":{"name":"./file_list.test"}}

#include "./file_list.hpp"

#include "testfwk/testfwk.hpp"

#include <array>

TESTCASE(slideproj_slideshow_source_file_list_load_and_sort)
{
	slideproj::slideshow_source::file_list items;

	items.append("foo/lenna.jpg")
		.append("foo/bar.jpg")
		.append("foo/kaka.jpg");

	EXPECT_EQ(
		items[0],
		(
			slideproj::slideshow_source::file_list_entry{
				slideproj::slideshow_source::file_id{0}, "foo/lenna.jpg"
			}
		)
	);

	EXPECT_EQ(
		items[1],
		(
			slideproj::slideshow_source::file_list_entry{
				slideproj::slideshow_source::file_id{1}, "foo/bar.jpg"
			}
		)
	);

	EXPECT_EQ(
		items[2],
		(
			slideproj::slideshow_source::file_list_entry{
				slideproj::slideshow_source::file_id{2}, "foo/kaka.jpg"
			}
		)
	);

	items.sort([](auto const& a, auto const& b){
		return a.path() < b.path();
	});

	EXPECT_EQ(items[0].path(), "foo/bar.jpg");
	EXPECT_EQ(items[1].path(), "foo/kaka.jpg");
	EXPECT_EQ(items[2].path(), "foo/lenna.jpg");
}