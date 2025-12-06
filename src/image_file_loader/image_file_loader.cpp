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
slideproj::image_file_loader::get_timestamp_from_exif_data(OIIO::ImageSpec const& spec, std::string_view field_name)
{
	OIIO::ustring attr_val;
	if(!spec.getattribute(field_name, OIIO::TypeString, &attr_val))
	{ return std::nullopt; }

	return make_statx_timestamp(exif_date_time_value{attr_val.string()});
}

slideproj::image_file_loader::exif_query_result::exif_query_result(OIIO::ImageSpec const& spec):
	m_valid_fields{0}
{
	auto timestamp = get_timestamp_from_exif_data(spec);
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
}

slideproj::image_file_loader::image_file_info
slideproj::image_file_loader::load_metadata(std::filesystem::path const& path)
{
	auto const exif_info = load_exif_query_result(path);
	image_file_info ret{};
	ret.timestamp = exif_info.timestamp() != nullptr?
			*exif_info.timestamp()
			// TODO: It is maybe better to let file_collector set the timestamp itself, is we cannot
			//       get a value from exif
			: file_collector::get_timestamp(path).value_or(file_collector::file_clock::time_point{});
	ret.caption = exif_info.description() != nullptr?
			*exif_info.description():
			path.stem().string();
	// TODO: Look for a file in parent directory with a descriptive name
	ret.in_group = path.parent_path();
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

slideproj::image_file_loader::image_rectangle
slideproj::image_file_loader::image_file_metadata_repository::get_dimensions(
	std::filesystem::path const& path
)
{
	auto input = OIIO::ImageInput::open(path);
	if(input == nullptr)
	{ return image_rectangle{}; }

	auto const& spec = input->spec();

	if(spec.width <= 0 || spec.height <= 0)
	{ return image_rectangle{}; }

	return image_rectangle{
		.width = static_cast<uint32_t>(spec.width),
		.height = static_cast<uint32_t>(spec.height)
	};
}

slideproj::image_file_loader::variant_image::variant_image(
	pixel_type_id pixel_type,
	enum alpha_mode alpha_mode,
	uint32_t w,
	uint32_t h,
	enum pixel_ordering pixel_ordering,
	make_uninitialized_pixel_buffer_tag
):
	m_alpha_mode{alpha_mode},
	m_width{0},
	m_height{0},
	m_pixel_ordering{pixel_ordering}
{
	if(!pixel_type.is_valid() || m_pixel_ordering == pixel_ordering::invalid)
	{ return; }

	m_pixels = utils::make_variant<pixel_buffer>(
		pixel_type.value(), [
			array_len = static_cast<size_t>(w) * static_cast<size_t>(h)
		]<class T>(utils::make_variant_type_tag<T>){
			using elem_type = typename T::element_type;
			return std::make_unique_for_overwrite<elem_type[]>(array_len);
		}
	);
	m_width = w;
	m_height = h;
}

slideproj::image_file_loader::variant_image
slideproj::image_file_loader::load_image(OIIO::ImageInput& input)
{
	auto const& spec = input.spec();
	if(spec.width <= 0 || spec.height <= 0 || spec.nchannels <= 0)
	{ return variant_image{}; }

	auto const alpha_mode =
		(spec.nchannels%2 == 0 && spec.get_int_attribute("oiio:UnassociatedAlpha")) == 0?
		alpha_mode::premultiplied:
		alpha_mode::straight;

	variant_image ret{
		pixel_type_id{
			to_intensity_transfer_function_id(spec.get_string_attribute("OIIO:ColorSpace")),
			static_cast<size_t>(spec.nchannels),
			to_value_type_id(spec.format)
		},
		alpha_mode,
		static_cast<uint32_t>(spec.width),
		static_cast<uint32_t>(spec.height),
		to_pixel_ordering_from_exif_orientation(spec.get_int_attribute("Orientation")),
		make_uninitialized_pixel_buffer_tag{}
	};
	ret.visit([&input, &spec](auto pixel_buffer, auto&&...){
		input.read_image(0, 0, 0, spec.nchannels, spec.format, pixel_buffer);
	});
	return ret;
}

slideproj::image_file_loader::fixed_typed_image<
	slideproj::image_file_loader::pixel_type<float, 4>
>
slideproj::image_file_loader::make_linear_rgba_image(
	variant_image const& input,
	uint32_t scaling_factor
)
{
	auto ret = input.visit([
		scaling_factor,
		pixel_ordering = input.pixel_ordering()
	](auto pixels, uint32_t w, uint32_t h) {
		auto downsampled = downsample_to_linear(pixels, w, h, scaling_factor);
		if(downsampled.is_empty())
		{ return fixed_typed_image<pixel_type<float, 4>>{}; }

		switch(pixel_ordering)
		{
			case pixel_ordering::top_to_bottom_right_to_left:
				downsampled = apply_pixel_ordering<pixel_ordering::top_to_bottom_right_to_left>(downsampled);
				break;
			case pixel_ordering::bottom_to_top_right_to_left:
				downsampled = apply_pixel_ordering<pixel_ordering::bottom_to_top_right_to_left>(downsampled);
				break;
			case pixel_ordering::bottom_to_top_left_to_right:
				downsampled = apply_pixel_ordering<pixel_ordering::bottom_to_top_left_to_right>(downsampled);
				break;
			case pixel_ordering::left_to_right_top_to_bottom:
				downsampled = apply_pixel_ordering<pixel_ordering::left_to_right_top_to_bottom>(downsampled);
				break;
			case pixel_ordering::right_to_left_top_to_bottom:
				downsampled = apply_pixel_ordering<pixel_ordering::right_to_left_top_to_bottom>(downsampled);
				break;
			case pixel_ordering::right_to_left_bottom_to_top:
				downsampled = apply_pixel_ordering<pixel_ordering::right_to_left_bottom_to_top>(downsampled);
				break;
			case pixel_ordering::left_to_right_bottom_to_top:
				downsampled = apply_pixel_ordering<pixel_ordering::left_to_right_bottom_to_top>(downsampled);
				break;
			default:
				break;
		}
		return to_rgba(downsampled.pixels(), downsampled.width(), downsampled.height());
	});

	if(input.alpha_mode() == alpha_mode::straight)
	{
		auto const pixels = ret.pixels();
		std::transform(
			pixels, pixels + ret.pixel_count(),
			pixels,
			[](auto item){
				auto const alpha = item.alpha;
				return slideproj::image_file_loader::pixel_type<float, 4>{
					.red = alpha*item.red,
					.green = alpha*item.green,
					.blue = alpha*item.blue,
					.alpha = alpha
				};
			}
		);
	}

	return ret;
}

uint32_t slideproj::image_file_loader::compute_scaling_factor(image_rectangle input, image_rectangle fit)
{
	if(input.width <= fit.width && input.height <= fit.height)
	{ return 1; }

	auto const input_aspect_ratio = static_cast<double>(input.width)/static_cast<double>(input.height);
	auto const output_aspect_ratio = static_cast<double>(fit.width)/static_cast<double>(fit.height);

	return (input_aspect_ratio >= output_aspect_ratio)? input.width/fit.width : input.height/fit.height;
}

slideproj::image_file_loader::fixed_typed_image<slideproj::image_file_loader::pixel_type<float, 4>>
slideproj::image_file_loader::make_linear_rgba_image(
	variant_image const& input,
	image_rectangle fit
)
{
	auto w_in = input.width();
	auto h_in = input.height();
	if(is_transposed(input.pixel_ordering()))
	{ std::swap(w_in, h_in); }

	return make_linear_rgba_image(
		input, compute_scaling_factor(
			image_rectangle{
				.width = w_in,
				.height = h_in
			},
			fit
		)
	);
}