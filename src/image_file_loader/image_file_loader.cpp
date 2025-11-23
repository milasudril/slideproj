//@	{
//@	 "target": {"name":"image_file_loader.o"},
//@	 "dependencies":[{"ref":"OpenImageIO", "origin":"pkg-config"}]
//@	}

#include "./image_file_loader.hpp"

#include "src/file_collector/file_collector.hpp"
#include "src/utils/utils.hpp"

#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/ustring.h>
#include <algorithm>
#include <any>
#include <cstdio>
#include <cstring>
#include <linux/stat.h>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <OpenImageIO/imageio.h>
#include <ctime>

std::optional<std::array<int, 6>>
slideproj::image_file_loader::make_ymdhms(exif_date_time_value<std::string_view> edtv)
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

	if(token_index != std::size(tokens) - 1)
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
		auto const res = utils::to_number<int>(tokens[k], valid_ranges[k]);
		if(!res.has_value())
		{ return std::nullopt; }
		converted_values[k] = *res;
	}

	return converted_values;
}

std::optional<statx_timestamp> slideproj::image_file_loader::make_statx_timestamp(exif_date_time_value<std::string_view> edtv)
{
	auto const res = make_ymdhms(edtv);
	if(!res.has_value())
	{ return std::nullopt; }

	auto const& converted_values = *res;
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

std::optional<statx_timestamp>
slideproj::image_file_loader::get_statx_timestamp(OIIO::ImageSpec const& spec, std::string_view field_name)
{
	OIIO::ustring attr_val;
	if(!spec.getattribute(field_name, OIIO::TypeString, &attr_val))
	{ return std::nullopt; }

	return make_statx_timestamp(exif_date_time_value{attr_val.string()});
}

slideproj::image_file_loader::exif_query_result::exif_query_result(OIIO::ImageSpec const& spec):
	m_valid_fields{0},
	m_pixel_ordering{pixel_ordering::top_to_bottom_left_to_right}
{
	auto timestamp = get_statx_timestamp(spec);
	if(timestamp)
	{
		m_valid_fields |= timestamp_valid;
		m_timestamp = file_collector::file_clock::create(*timestamp);
	}

	OIIO::ustring desc_val;
	if(spec.getattribute("ImageDescription", OIIO::TypeString, &desc_val))
	{
		m_valid_fields |= description_valid;
		m_description = desc_val.string();
	}

	// TODO: Want to fetch ImageTitle as well, but it appears like OpenImageIO does not support that
	// field

	auto const orientation = spec.get_int_attribute("Orientation", 1);
	m_pixel_ordering = orientation>= 1 && orientation <=8?
		static_cast<enum pixel_ordering>(orientation - 1):pixel_ordering::top_to_bottom_left_to_right;
}

slideproj::image_file_loader::image_file_info
slideproj::image_file_loader::load_metadata(std::filesystem::path const& path)
{
	auto const exif_info = load_exif_query_result(path);
	image_file_info ret{};
	ret.timestamp = exif_info.timestamp() != nullptr?
			*exif_info.timestamp()
			: file_collector::get_timestamp(path).value_or(file_collector::file_clock::time_point{});
	ret.caption = exif_info.description() != nullptr?
			*exif_info.description():
			path.stem().string();
	// TODO: Look for a file in parent directory with a descriptive name
	ret.in_group = path.parent_path();
	ret.pixel_ordering = exif_info.pixel_ordering();

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

	printf("Loading metadata for %s\n", entry.path().c_str());

	return m_cache.insert(std::pair{entry.id(), load_metadata(entry.path())}).first->second;
}

void slideproj::image_file_loader::load_one_channel_image_as_grayscale_image(
	OIIO::ImageInput& input,
	std::span<color_value> output
)
{
	constexpr auto nchannels = 1;
	auto const& spec = input.spec();
	auto const tempbuff_size = spec.width*spec.height;
	auto const tempbuff = std::make_unique_for_overwrite<float[]>(tempbuff_size);
	input.read_image(0, 0, 0, nchannels, OIIO::TypeDesc::TypeFloat, tempbuff.get());

	std::transform(tempbuff.get(), tempbuff.get() + tempbuff_size, std::begin(output), [](auto val){
		return color_value{val, val, val, 1.0f};
	});
}

slideproj::image_file_loader::image::image(std::filesystem::path& path)
{
	auto img_reader = OIIO::ImageInput::open(path);
	if(img_reader == nullptr)
	{ return; }

	auto const& spec = img_reader->spec();
	if(spec.width <= 0 || spec.height <= 0)
	{ return; }

	m_pixels = std::make_unique<color_value[]>(spec.width*spec.height);
	m_width = static_cast<uint32_t>(spec.width);
	m_height = static_cast<uint32_t>(spec.height);

	switch(spec.nchannels)
	{
		case 1:
			load_one_channel_image_as_grayscale_image(*img_reader, pixels());
			break;
#if 0
		case 2:
			load_two_channel_image_as_grayalpha_image(*img_reader, pixels());
			break;
		case 3:
			load_three_channel_image_as_rgb_image(*img_reader, pixels());
			break;
		case 4:
			load_four_channel_image_as_rgba_image(*img_reader, pixels());
			break;
#endif
		default:
			*this = image{};
	}
}