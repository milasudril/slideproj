//@	{"target":{"name":"bidirectional_sliding_window.test"}}

#include "./bidirectional_sliding_window.hpp"

#include "testfwk/testfwk.hpp"

TESTCASE(bidirectional_sliding_window_3_elements)
{
	slideproj::utils::bidirectional_sliding_window<int, 1> vals{
		std::array{1, 2, 3}
	};

	// Initially no value can be replaced
	EXPECT_EQ(vals.get_element_to_replace(), nullptr);

	// Current value is 2
	auto const val1 =  vals.get_current_element();
	EXPECT_EQ(val1, 2);

	// After moving forward, the element to replace is the first one
	{
		vals.step_forward();
		EXPECT_EQ(vals.get_current_element(), 3);
		auto const val_to_replace = vals.get_element_to_replace();
		REQUIRE_NE(val_to_replace, nullptr);
		EXPECT_EQ(*val_to_replace, 1);
		*val_to_replace = 4;
	}

	// Move backwards
	{
		vals.step_backward();
		EXPECT_EQ(vals.get_current_element(), 2);
		auto const val_to_replace = vals.get_element_to_replace();
		REQUIRE_NE(val_to_replace, nullptr);
		EXPECT_EQ(*val_to_replace, 4);
		*val_to_replace = 1;
	}

	// Move backwards again
	{
		vals.step_backward();
		EXPECT_EQ(vals.get_current_element(), 1);
		auto const val_to_replace = vals.get_element_to_replace();
		REQUIRE_NE(val_to_replace, nullptr);
		EXPECT_EQ(*val_to_replace, 3);
	}
}

TESTCASE(bidirectional_sliding_window_5_elements)
{
	slideproj::utils::bidirectional_sliding_window<int, 2> vals{
		std::array{1, 2, 3, 4, 5}
	};

	// Initially no value can be replaced
	EXPECT_EQ(vals.get_element_to_replace(), nullptr);

	// Current value is 3
	auto const val1 =  vals.get_current_element();
	EXPECT_EQ(val1, 3);

	// After moving forward, the element to replace is the first one
	{
		vals.step_forward();
		EXPECT_EQ(vals.get_current_element(), 4);
		auto const val_to_replace = vals.get_element_to_replace();
		REQUIRE_NE(val_to_replace, nullptr);
		EXPECT_EQ(*val_to_replace, 1);
		*val_to_replace = 6;
	}

	// Move backwards
	{
		vals.step_backward();
		EXPECT_EQ(vals.get_current_element(), 3);
		auto const val_to_replace = vals.get_element_to_replace();
		REQUIRE_NE(val_to_replace, nullptr);
		EXPECT_EQ(*val_to_replace, 6);
		*val_to_replace = 1;
	}

	// Move backwards again
	{
		vals.step_backward();
		EXPECT_EQ(vals.get_current_element(), 2);
		auto const val_to_replace = vals.get_element_to_replace();
		REQUIRE_NE(val_to_replace, nullptr);
		EXPECT_EQ(*val_to_replace, 5);
	}
}