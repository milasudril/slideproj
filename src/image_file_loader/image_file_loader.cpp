//@	{
//@	 "target": {"name":"image_file_loader.o"},
//@	 "dependencies":[{"ref":"OpenImageIO", "origin":"pkg-config"}]
//@	}

#include "./image_file_loader.hpp"
#include "src/file_collector/file_collector.hpp"

#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/ustring.h>
#include <algorithm>
#include <cstring>
#include <linux/stat.h>
#include <ranges>
#include <stdexcept>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <OpenImageIO/imageio.h>

std::optional<statx_timestamp> slideproj::image_file_loader::make_statx_timestamp(exif_date_time_value<std::string_view> edtv)
{
	std::array<std::string_view, 6> tokens{};
	auto tok_start = edtv.value.begin();
	auto tok_end = edtv.value.begin();
	size_t token_index = 0;
	while(tok_end != edtv.value.end())
	{
		if(token_index == std::size(tokens))
		{ return std::nullopt; }

		if(token_index != 2)
		{
			if(*tok_end == ':')
			{
				tokens[token_index] = std::string_view{tok_start, tok_end};
				tok_start = tok_end + 1;
				++token_index;
			}
		}
		else
		{
			if(*tok_end == ' ')
			{
				tokens[token_index] = std::string_view{tok_start, tok_end};
				tok_start = tok_end + 1;
				++token_index;
			}
		}
		++tok_end;
	}
	if(token_index == std::size(tokens))
	{ return std::nullopt; }

	tokens[token_index] = std::string_view{tok_start, tok_end};

	std::array<int, 6> converted_values{};
	constexpr std::array valid_ranges{
		std::ranges::min_max_result{-std::numeric_limits<int>::max(), std::numeric_limits<int>::max()},
		std::ranges::min_max_result{1, 12},
		std::ranges::min_max_result{1, 31},
		std::ranges::min_max_result{0, 23},
		std::ranges::min_max_result{0, 59},
		std::ranges::min_max_result{0, 60},
	};
	for(size_t k = 0; k != std::size(converted_values); ++k)
	{
		auto const res = to_number<int>(tokens[k], valid_ranges[k]);
		if(!res.has_value())
		{ return std::nullopt; }
		converted_values[k] = *res;
	}

	tm timestruct{};
	timestruct.tm_year = converted_values[0] - 1900;
	timestruct.tm_mon = converted_values[1] - 1;
	timestruct.tm_mday = converted_values[2];
	timestruct.tm_hour = converted_values[3];
	timestruct.tm_min = converted_values[4];
	timestruct.tm_sec = converted_values[5];

	errno = 0;
	auto secs = timegm(&timestruct);
	if(secs == static_cast<time_t>(-1) && errno != 0)
	{ return std::nullopt; }

	return statx_timestamp{
		.tv_sec = timegm(&timestruct),
		.tv_nsec = 0,
		.__reserved = 0
	};
}

std::optional<slideproj::file_collector::file_clock::time_point>
slideproj::image_file_loader::get_timestamp(OIIO::ImageSpec const& spec, std::string_view field_name)
{
	OIIO::ustring attr_val;
	if(!spec.getattribute(field_name, OIIO::TypeString, &attr_val))
	{ return std::nullopt; }

	auto timestamp = make_statx_timestamp(exif_date_time_value{attr_val.string()});
	if(!timestamp.has_value())
	{ return std::nullopt; }

	return slideproj::file_collector::file_clock::create(*timestamp);
}

slideproj::image_file_loader::exif_query_result::exif_query_result(std::filesystem::path const& path):
	m_valid_fields{0},
	m_pixel_ordering{pixel_ordering::top_to_bottom_left_to_right}
{
	auto img_reader = OIIO::ImageInput::open(path);
	if(!img_reader)
	{ throw std::runtime_error{img_reader->geterror()}; }

	auto const& spec = img_reader->spec();
	auto timestamp = get_timestamp(spec);
	if(timestamp)
	{
		m_valid_fields |= timestamp_valid;
		m_timestamp = *timestamp;
	}
}

slideproj::image_file_loader::image_file_info
slideproj::image_file_loader::load_metadata(std::filesystem::path const& path)
{
	exif_query_result exif_info{path};

	if(exif_info.timestamp() != nullptr)
	{
		auto const res = *exif_info.timestamp();
		auto timestamp = std::format("{}", res.time_since_epoch().count());
		printf("%s\n", timestamp.c_str());
	}

	struct statx statxbuf{};
	auto res = statx(AT_FDCWD, path.c_str(), AT_NO_AUTOMOUNT, STATX_BTIME | STATX_MTIME, &statxbuf);
	if(res == -1)
	{ throw std::runtime_error{strerror(errno)}; }

	image_file_info ret{};

	// TODO: Use EXIF data as primary source of truth
	ret.caption = path.stem();
	// TODO: Look for a file in parent directory with a descriptive name
	ret.in_group = path.parent_path();

	if(statxbuf.stx_mask&STATX_BTIME)
	{	ret.timestamp = file_collector::file_clock::create(statxbuf.stx_btime); }
	else
	if(statxbuf.stx_mask&STATX_MTIME)
	{ ret.timestamp = file_collector::file_clock::create(statxbuf.stx_mtime); }


	return ret;
}

slideproj::image_file_loader::image_file_info const&
slideproj::image_file_loader::image_file_metadata_repository::get_metadata(
	file_collector::file_list_entry const& entry
) const
{
	auto i = m_cache.find(entry.id());
	if(i != std::end(m_cache))
	{ return i->second; }

	return m_cache.insert(std::pair{entry.id(), load_metadata(entry.path())}).first->second;
}