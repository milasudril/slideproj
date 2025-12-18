//@	{"target":{"name":"glob_string.test"}}

#include "./glob_string.hpp"

#include "testfwk/testfwk.hpp"

TESTCASE(slideproj_utils_glob_string_wildcard_in_the_middle)
{
	slideproj::utils::glob_string const pattern{"img*.jpg"};

	EXPECT_EQ(pattern.matches("IMG0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.BMP"), false);
	EXPECT_EQ(pattern.matches("FOO.JPG"), false);
}

TESTCASE(slideproj_utils_glob_string_wildcard_in_the_middle_single_char_at_end)
{
	slideproj::utils::glob_string const pattern{"img*j"};

	EXPECT_EQ(pattern.matches("IMG0001.J"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.J"), true);
	EXPECT_EQ(pattern.matches("IMG.J"), true);
	EXPECT_EQ(pattern.matches("IMG.B"), false);
	EXPECT_EQ(pattern.matches("FOO.J"), false);
}

TESTCASE(slideproj_utils_glob_string_wildcard_at_end)
{
	slideproj::utils::glob_string const pattern{"img*"};

	EXPECT_EQ(pattern.matches("IMG0001.J"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.J"), true);
	EXPECT_EQ(pattern.matches("IMG.J"), true);
	EXPECT_EQ(pattern.matches("IMG.B"), true);
	EXPECT_EQ(pattern.matches("FOO.J"), false);
}

TESTCASE(slideproj_utils_glob_string_wildcard_at_begin)
{
	slideproj::utils::glob_string const pattern{"*.jpg"};

	EXPECT_EQ(pattern.matches("IMG0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.BMP"), false);
	EXPECT_EQ(pattern.matches("FOO.JPG"), true);
}

TESTCASE(slideproj_utils_glob_string_multiple_wildcards)
{
	slideproj::utils::glob_string const pattern{"foo*img*.jpg"};

	EXPECT_EQ(pattern.matches("foobarIMG0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.JPG"), false);
	EXPECT_EQ(pattern.matches("foobarblah.0001.JPG"), false);
}

TESTCASE(slideproj_utils_glob_string_multiple_consecutive_wildcards)
{
	slideproj::utils::glob_string const pattern{"foo**.jpg"};

	EXPECT_EQ(pattern.matches("foobarIMG0001.JPG"), true);
	EXPECT_EQ(pattern.matches("IMG.0001.JPG"), false);
}

TESTCASE(slideproj_utils_glob_string_multiple_wildcards_no_match_in_between)
{
	slideproj::utils::glob_string const pattern{"foo*i*.jpg"};

	EXPECT_EQ(pattern.matches("foobar_bajs_whatever.JPG"), false);
}

TESTCASE(slideproj_utils_glob_string_single_wildcard_alone_matches_anything)
{
	slideproj::utils::glob_string const pattern{"*"};

	EXPECT_EQ(pattern.matches("foobar_bajs_whatever.JPG"), true);
	EXPECT_EQ(pattern.matches(""), false);
}