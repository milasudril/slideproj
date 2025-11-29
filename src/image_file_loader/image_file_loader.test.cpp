//@	{"target":{"name":"image_file_loader.test"}}

#include "./image_file_loader.hpp"

#include "src/file_collector/file_collector.hpp"
#include "testfwk/testfwk.hpp"
#include "testfwk/testsuite.hpp"
#include "testfwk/validation.hpp"
#include <OpenImageIO/imageio.h>

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_empty_string)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_valid)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:15 00:00:00"
	});
	REQUIRE_EQ(res.has_value(), true);
	auto const& values = *res;
	EXPECT_EQ(values[0], 1582);
	EXPECT_EQ(values[1], 10);
	EXPECT_EQ(values[2], 15);
	EXPECT_EQ(values[3], 0);
	EXPECT_EQ(values[4], 0);
	EXPECT_EQ(values[5], 0);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_colon_after_sec)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:15 00:00:00:"
	});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_end_with_colon_after_min)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:15 00:00:"
	});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_end_with_colon_after_hour)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:15 00:"
	});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_end_after_min)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:15 00:00"
	});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_invalid_month)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:0:15 00:00:00"
	});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_invalid_day)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:0 00:00:00"
	});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_invalid_hour)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:15 24:00:00"
	});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_invalid_min)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:15 00:60:00"
	});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_invalid_sec)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:15 00:00:61"
	});
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_make_ydhms_from_edtv_leap_second_is_valid)
{
	auto res = make_ymdhms(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1582:10:15 00:00:60"
	});
	REQUIRE_EQ(res.has_value(), true);

	auto const& values = *res;
	EXPECT_EQ(values[0], 1582);
	EXPECT_EQ(values[1], 10);
	EXPECT_EQ(values[2], 15);
	EXPECT_EQ(values[3], 0);
	EXPECT_EQ(values[4], 0);
	EXPECT_EQ(values[5], 60);
}

TESTCASE(slideproj_image_file_loader_make_statx_timestamp_from_edtv)
{
	auto res1 = make_statx_timestamp(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1970:01:01 00:00:00"
	});
	REQUIRE_EQ(res1.has_value(), true);
	EXPECT_EQ(res1->tv_sec, 0);
	EXPECT_EQ(res1->tv_nsec, 0);

	auto res2 = make_statx_timestamp(slideproj::image_file_loader::exif_date_time_value<std::string_view>{
		"1970:01:02 00:00:00"
	});
	REQUIRE_EQ(res2.has_value(), true);
	EXPECT_EQ(res2->tv_sec, 86400);
	EXPECT_EQ(res2->tv_nsec, 0);
}

TESTCASE(slideproj_image_file_loader_get_statx_timestamp_from_oiio_spec_field_field_not_present)
{
	OIIO::ImageSpec spec{};
	auto res = slideproj::image_file_loader::get_statx_timestamp(spec, "foobar");
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_get_statx_timestamp_from_oiio_spec_field_field_wrong_type)
{
	OIIO::ImageSpec spec{};
	spec.attribute("foobar", 466);
	auto res = slideproj::image_file_loader::get_statx_timestamp(spec, "foobar");
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_get_statx_timestamp_from_oiio_spec_field)
{
	OIIO::ImageSpec spec{};
	spec.attribute("foobar", "1582:10:15 00:00:00");
	auto res = slideproj::image_file_loader::get_statx_timestamp(spec, "foobar");
	EXPECT_EQ(res.has_value(), true);
	EXPECT_EQ(res->tv_sec, -12219292800);
	EXPECT_EQ(res->tv_nsec, 0);
}

TESTCASE(slideproj_image_file_loader_get_statx_timestamp_from_oiio_spec_no_exif_date_and_time)
{
	OIIO::ImageSpec spec{};
	auto res = slideproj::image_file_loader::get_statx_timestamp(spec);
	EXPECT_EQ(res.has_value(), false);
}

TESTCASE(slideproj_image_file_loader_get_statx_timestamp_from_oiio_spec_dto_takes_precedence)
{
	OIIO::ImageSpec spec{};
	spec.attribute("DateTime", "1582:10:15 00:00:00");
	spec.attribute("Exif:DateTimeOriginal", "1583:10:15 00:00:00");
	auto res = slideproj::image_file_loader::get_statx_timestamp(spec);
	REQUIRE_EQ(res.has_value(), true);
	EXPECT_EQ(res->tv_sec, -12187756800);
	EXPECT_EQ(res->tv_nsec, 0);
}

TESTCASE(slideproj_image_file_loader_get_statx_timestamp_from_oiio_spec_only_dt_set)
{
	OIIO::ImageSpec spec{};
	spec.attribute("DateTime", "1582:10:15 00:00:00");
	auto res = slideproj::image_file_loader::get_statx_timestamp(spec);
	REQUIRE_EQ(res.has_value(), true);
	EXPECT_EQ(res->tv_sec, -12219292800);
	EXPECT_EQ(res->tv_nsec, 0);
}

TESTCASE(slideproj_image_file_loader_get_statx_timestamp_from_oiio_spec_only_dto_set)
{
	OIIO::ImageSpec spec{};
	spec.attribute("Exif:DateTimeOriginal", "1582:10:15 00:00:00");
	auto res = slideproj::image_file_loader::get_statx_timestamp(spec);
	REQUIRE_EQ(res.has_value(), true);
	EXPECT_EQ(res->tv_sec, -12219292800);
	EXPECT_EQ(res->tv_nsec, 0);
}

TESTCASE(slideproj_image_file_loader_exif_query_result_default_state)
{
	slideproj::image_file_loader::exif_query_result const result{};
	EXPECT_EQ(result.description(), nullptr);
	EXPECT_EQ(result.timestamp(), nullptr);
	EXPECT_EQ(
		result.pixel_ordering(),
		slideproj::image_file_loader::pixel_ordering::top_to_bottom_left_to_right
	);
}

TESTCASE(slideproj_image_file_loader_exif_query_result_from_oiio_sepc_no_field_set)
{
	OIIO::ImageSpec const spec{};
	slideproj::image_file_loader::exif_query_result const result{spec};
	EXPECT_EQ(result.description(), nullptr);
	EXPECT_EQ(result.timestamp(), nullptr);
	EXPECT_EQ(
		result.pixel_ordering(),
		slideproj::image_file_loader::pixel_ordering::top_to_bottom_left_to_right
	);
}

TESTCASE(slideproj_image_file_loader_exif_query_result_from_oiio_sepc_with_timestamp)
{
	OIIO::ImageSpec spec{};
	spec.attribute("Exif:DateTimeOriginal", "1582:10:15 00:00:00");
	slideproj::image_file_loader::exif_query_result const result{std::as_const(spec)};
	EXPECT_EQ(result.description(), nullptr);
	REQUIRE_NE(result.timestamp(), nullptr);
	EXPECT_EQ
		(std::chrono::duration_cast<std::chrono::seconds>(result.timestamp()->time_since_epoch()).count(), -12219292800);
	EXPECT_EQ(
		result.pixel_ordering(),
		slideproj::image_file_loader::pixel_ordering::top_to_bottom_left_to_right
	);
}

TESTCASE(slideproj_image_file_loader_exif_query_result_from_oiio_sepc_with_description)
{
	OIIO::ImageSpec spec{};
	spec.attribute("ImageDescription", "This is a test");
	slideproj::image_file_loader::exif_query_result const result{std::as_const(spec)};
	REQUIRE_NE(result.description(), nullptr);
	EXPECT_EQ(*result.description(), "This is a test");
	EXPECT_EQ(result.timestamp(), nullptr);
	EXPECT_EQ(
		result.pixel_ordering(),
		slideproj::image_file_loader::pixel_ordering::top_to_bottom_left_to_right
	);
}

TESTCASE(slideproj_image_file_loader_exif_query_result_from_oiio_sepc_with_orientation)
{
	OIIO::ImageSpec spec{};
	spec.attribute("Orientation", 2);
	slideproj::image_file_loader::exif_query_result const result{std::as_const(spec)};
	EXPECT_EQ(result.description(), nullptr);
	EXPECT_EQ(result.timestamp(), nullptr);
	EXPECT_EQ(
		result.pixel_ordering(),
		slideproj::image_file_loader::pixel_ordering::top_to_bottom_right_to_left
	);
}

#if 0
TESTCASE(slideproj_image_file_get_metadata)
{
	// TODO: Go and take some pictures that can be used as test data

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
#endif

TESTCASE(slideproj_image_file_loader_load_uint8_rgba_from_png_srbg)
{
	auto res = slideproj::image_file_loader::load_image("testdata/rgba_8bit_srgb.png");
	EXPECT_EQ(res.width(), 96);
	EXPECT_EQ(res.height(), 32);
	EXPECT_EQ(res.alpha_mode(), slideproj::image_file_loader::alpha_mode::straight);

	using expected_pixel_type = slideproj::image_file_loader::pixel_type<
		slideproj::image_file_loader::sample_type<
			uint8_t,
			slideproj::image_file_loader::srgb_intensity_mapping
		>,
		4
	>;
	auto pixels = res.pixels<expected_pixel_type>();
	EXPECT_EQ(pixels[0].red.value, 128);
	EXPECT_EQ(pixels[0].green.value, 0);
	EXPECT_EQ(pixels[0].blue.value, 0);
	EXPECT_EQ(pixels[0].alpha.value, 128);

	EXPECT_EQ(pixels[32].red.value, 0);
	EXPECT_EQ(pixels[32].green.value, 128);
	EXPECT_EQ(pixels[32].blue.value, 0);
	EXPECT_EQ(pixels[32].alpha.value, 128);

	EXPECT_EQ(pixels[64].red.value, 0);
	EXPECT_EQ(pixels[64].green.value, 0);
	EXPECT_EQ(pixels[64].blue.value, 128);
	EXPECT_EQ(pixels[64].alpha.value, 128);
}

TESTCASE(slideproj_image_file_loader_load_uint8_rgba_from_bmp_srbg)
{
	auto res = slideproj::image_file_loader::load_image("testdata/rgba_8bit_srgb.bmp");
	EXPECT_EQ(res.width(), 96);
	EXPECT_EQ(res.height(), 32);

	using expected_pixel_type = slideproj::image_file_loader::pixel_type<
		slideproj::image_file_loader::sample_type<
			uint8_t,
			slideproj::image_file_loader::srgb_intensity_mapping
		>,
		4
	>;
	auto pixels = res.pixels<expected_pixel_type>();
	EXPECT_EQ(pixels[0].red.value, 128);
	EXPECT_EQ(pixels[0].green.value, 0);
	EXPECT_EQ(pixels[0].blue.value, 0);
	EXPECT_EQ(pixels[0].alpha.value, 128);

	EXPECT_EQ(pixels[32].red.value, 0);
	EXPECT_EQ(pixels[32].green.value, 128);
	EXPECT_EQ(pixels[32].blue.value, 0);
	EXPECT_EQ(pixels[32].alpha.value, 128);

	EXPECT_EQ(pixels[64].red.value, 0);
	EXPECT_EQ(pixels[64].green.value, 0);
	EXPECT_EQ(pixels[64].blue.value, 128);
	EXPECT_EQ(pixels[64].alpha.value, 128);
}

#if 0
TESTCASE(slideproj_image_file_loader_load_rgba_imgage_32bit_from_exr)
{
	auto res = slideproj::image_file_loader::load("testdata/rgba_32bit_linear.exr");
	using expected_type = slideproj::image_file_loader::pixel_storage<slideproj::image_file_loader::color_value<float>>;
	auto const& uint8_img = std::get<expected_type>(res.pixels);
	EXPECT_EQ(uint8_img.width(), 96);
	EXPECT_EQ(uint8_img.height(), 32);

	auto data = uint8_img.pixels();
	printf("%.8g %.8g %.8g %.8g\n", data[0].r, data[0].g, data[0].b, data[0].a);
}
#endif


TESTCASE(slideproj_image_file_loader_load_rgba_image_from_png_srbg)
{
	auto res = slideproj::image_file_loader::load_rgba_image("testdata/rgba_8bit_srgb.png", 2);
#if 0
	EXPECT_EQ(res.width(), 96);
	EXPECT_EQ(res.height(), 32);
	EXPECT_EQ(res.alpha_mode(), slideproj::image_file_loader::alpha_mode::straight);

	using expected_pixel_type = slideproj::image_file_loader::pixel_type<
		slideproj::image_file_loader::sample_type<
			uint8_t,
			slideproj::image_file_loader::srgb_intensity_mapping
		>,
		4
	>;
	auto pixels = res.pixels<expected_pixel_type>();
	EXPECT_EQ(pixels[0].red.value, 128);
	EXPECT_EQ(pixels[0].green.value, 0);
	EXPECT_EQ(pixels[0].blue.value, 0);
	EXPECT_EQ(pixels[0].alpha.value, 128);

	EXPECT_EQ(pixels[32].red.value, 0);
	EXPECT_EQ(pixels[32].green.value, 128);
	EXPECT_EQ(pixels[32].blue.value, 0);
	EXPECT_EQ(pixels[32].alpha.value, 128);

	EXPECT_EQ(pixels[64].red.value, 0);
	EXPECT_EQ(pixels[64].green.value, 0);
	EXPECT_EQ(pixels[64].blue.value, 128);
	EXPECT_EQ(pixels[64].alpha.value, 128);
#endif
}