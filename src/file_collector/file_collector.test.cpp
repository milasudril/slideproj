//@	{"target":{"name":"./file_collector.test"}}

#include "./file_collector.hpp"

#include "testfwk/testfwk.hpp"
#include "testfwk/validation.hpp"

#include <array>
#include <filesystem>
#include <unordered_map>

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

TESTCASE(slideproj_file_collector_file_clock)
{
	slideproj::file_collector::file_clock::time_point t{std::chrono::seconds{1643220649}};
	auto str = std::format("{}", t.time_since_epoch().count());
	EXPECT_EQ(str, "1643220649000000000");

	constexpr auto t2 = slideproj::file_collector::file_clock::create(
		statx_timestamp{
			.tv_sec = 0x7fff'ffff'ffff'ffffLL,
			.tv_nsec = 999'999'999,
			.__reserved = 0
		}
	);

	auto str2 = std::format("{}", t2.time_since_epoch().count());
	EXPECT_EQ(str2, "9223372036854775807999999999");
}

namespace
{
	struct file_metadata_provider
	{
		using file_id = slideproj::file_collector::file_id;
		using file_metadata = slideproj::file_collector::file_metadata;

		static constexpr std::array random_timestamps{
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1643220649}},
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1692772825}},
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1736678350}},
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1671528350}},
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1670981320}},
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1747655837}},
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1613510145}},
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1608130796}},
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1733967405}},
			slideproj::file_collector::file_clock::time_point{std::chrono::seconds{1671563660}}
		};

		file_metadata const& get_metadata(slideproj::file_collector::file_list_entry const& item) const
		{
			auto const i = values.find(item.id());
			if(i != std::end(values))
			{ return i->second; }

			auto const ip = values.insert(
				std::pair{
					item.id(),
					file_metadata{
						.timestamp = random_timestamps[item.id().value()],
						.in_group = item.path().parent_path(),
						.caption = item.path().filename()
					}
				}
			);
			return ip.first->second;
		}

		mutable std::unordered_map<file_id, file_metadata> values;
	};
}

TESTCASE(slideproj_file_collector_file_list_sort_with_metadata_provider)
{
	slideproj::file_collector::file_list files;

	std::array input_files{
		std::filesystem::path{"/home/sarah/Pictures/Family_Events/beach_sunset_001.jpg"},
		std::filesystem::path{"/home/sarah/Pictures/Screenshots/capture_20251121_1400.png"},
		std::filesystem::path{"/home/sarah/Pictures/Family_Events/birthday_party_2024.tar.gz"},
		std::filesystem::path{"/home/sarah/Pictures/Albums/Art_Projects/sketchbook_scan.tiff"},
		std::filesystem::path{"/home/sarah/Pictures/Camera_Roll/IMG_4567.raw"},
		std::filesystem::path{"/home/sarah/Pictures/Work_References/logo_design_draft_v3.svg"},
		std::filesystem::path{"/home/sarah/Pictures/wallpapers/desktop_background_nebula.webp"},
		std::filesystem::path{"/home/sarah/Pictures/Temporary/temp_edit_file.xcf"},
		std::filesystem::path{"/home/sarah/Pictures/Exports/web_gallery_small.zip"},
		std::filesystem::path{"/home/sarah/Pictures/Memes/funny_cat_reaction.gif"}
	};

	for(auto const& item: input_files)
	{ files.append(item);}
	EXPECT_EQ(std::size(files), std::size(input_files));

	using file_metadata_field = slideproj::file_collector::file_metadata_field;

	file_metadata_provider metadata_provider{};
	sort(
		files,
		std::array{file_metadata_field::timestamp, file_metadata_field::in_group, file_metadata_field::caption},
		metadata_provider,
		[](std::string_view a, std::string_view b){
			return a <=> b;
		}
	);

	std::vector<size_t> stored_ids;
	for(auto const& item :metadata_provider.values)
	{ stored_ids.push_back(item.first.value()); }
	std::ranges::sort(stored_ids);
	EXPECT_EQ(std::size(files), std::size(input_files));
	EXPECT_EQ(std::size(stored_ids), std::size(files));
	for(size_t k = 0; k != std::size(stored_ids); ++k)
	{ EXPECT_EQ(stored_ids[k], k); }

	auto prev_file_info = &metadata_provider.get_metadata(files[0]);
	for(size_t k = 1; k != std::size(files); ++k)
	{
		auto current_file_info = &metadata_provider.get_metadata(files[k]);
		EXPECT_GE(current_file_info->timestamp, prev_file_info->timestamp);
		if(current_file_info->timestamp == prev_file_info->timestamp)
		{
			EXPECT_GE(current_file_info->in_group, prev_file_info->in_group);
			if(current_file_info->in_group == prev_file_info->in_group)
			{ EXPECT_GE(current_file_info->caption, prev_file_info->caption)}
		}
		prev_file_info = current_file_info;
	}
}

namespace
{
	struct input_filter
	{
		bool accepts(std::filesystem::directory_entry const&) const
		{ return true; }
	};
}

TESTCASE(slideproj_file_collector_make_real_file_list)
{
	auto files = slideproj::file_collector::make_file_list(std::vector<std::string>{"testdata"}, input_filter{});
	for(auto const& item: files)
	{ EXPECT_EQ(item.path().is_absolute(), false); }
}
