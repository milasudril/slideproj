//@	{"target":{"name":"./file_collector.test"}}

#include "./file_collector.hpp"

#include "testfwk/testfwk.hpp"

#include <array>

TESTCASE(slideproj_file_collector_file_list_load_and_sort)
{
	slideproj::file_collector::file_list items;

	items.append("foo/lenna.jpg")
		.append("foo/bar.jpg")
		.append("foo/kaka.jpg");

	EXPECT_EQ(
		items[0],
		(
			slideproj::file_collector::file_list_entry{
				slideproj::file_collector::file_id{0}, "foo/lenna.jpg"
			}
		)
	);

	EXPECT_EQ(
		items[1],
		(
			slideproj::file_collector::file_list_entry{
				slideproj::file_collector::file_id{1}, "foo/bar.jpg"
			}
		)
	);

	EXPECT_EQ(
		items[2],
		(
			slideproj::file_collector::file_list_entry{
				slideproj::file_collector::file_id{2}, "foo/kaka.jpg"
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