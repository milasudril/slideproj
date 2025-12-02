//@	{"target":{"name":"input_filter.test"}}

#include "./input_filter.hpp"

#include "testfwk/testfwk.hpp"

TESTCASE(slideproj_app_input_filter_pattern_wildcard_in_the_middle)
{
	slideproj::app::input_filter_pattern const pattern{"img*.jpg"};

	EXPECT_EQ(pattern.matches("IMG0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.BMP"), false);
	EXPECT_EQ(pattern.matches("FOO.JPG"), false);
}

TESTCASE(slideproj_app_input_filter_pattern_wildcard_in_the_middle_single_char_at_end)
{
	slideproj::app::input_filter_pattern const pattern{"img*j"};

	EXPECT_EQ(pattern.matches("IMG0001.J"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.J"), true);
	EXPECT_EQ(pattern.matches("IMG.J"), true);
	EXPECT_EQ(pattern.matches("IMG.B"), false);
	EXPECT_EQ(pattern.matches("FOO.J"), false);
}

TESTCASE(slideproj_app_input_filter_pattern_wildcard_at_end)
{
	slideproj::app::input_filter_pattern const pattern{"img*"};

	EXPECT_EQ(pattern.matches("IMG0001.J"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.J"), true);
	EXPECT_EQ(pattern.matches("IMG.J"), true);
	EXPECT_EQ(pattern.matches("IMG.B"), true);
	EXPECT_EQ(pattern.matches("FOO.J"), false);
}

TESTCASE(slideproj_app_input_filter_pattern_wildcard_at_begin)
{
	slideproj::app::input_filter_pattern const pattern{"*.jpg"};

	EXPECT_EQ(pattern.matches("IMG0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.BMP"), false);
	EXPECT_EQ(pattern.matches("FOO.JPG"), true);
}

TESTCASE(slideproj_app_input_filter_pattern_multiple_wildcards)
{
	slideproj::app::input_filter_pattern const pattern{"foo*img*.jpg"};

	EXPECT_EQ(pattern.matches("foobarIMG0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.JPG"), false);
	EXPECT_EQ(pattern.matches("foobarblah.0001.JPG"), false);
}

TESTCASE(slideproj_app_input_filter_pattern_multiple_consecutive_wildcards)
{
	slideproj::app::input_filter_pattern const pattern{"foo**.jpg"};

	EXPECT_EQ(pattern.matches("foobarIMG0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.JPG"), false);
}

TESTCASE(slideproj_app_input_filter_pattern_multiple_wildcards_no_match_in_between)
{
	slideproj::app::input_filter_pattern const pattern{"foo*i*.jpg"};

	EXPECT_EQ(pattern.matches("foobar_bajs_whatever.JPG"), false);
}

TESTCASE(slideproj_app_input_filter_pattern_single_wildcard_alone_matches_anything)
{
	slideproj::app::input_filter_pattern const pattern{"*"};

	EXPECT_EQ(pattern.matches("foobar_bajs_whatever.JPG"), true);
	EXPECT_EQ(pattern.matches(""), false);
}