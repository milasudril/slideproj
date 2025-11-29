//@	{
//@		"dependencies_extra":[
//@			{"ref":"./image_file_loader.o", "rel":"implementation"},
//@			{"ref":"OpenImageIO", "rel":"implementation", "origin":"pkg-config"}
//@		]
//@	}

#ifndef SLIDEPROJ_IMAGE_FILE_LOADER_IMAGE_FILE_LOADER_HPP
#define SLIDEPROJ_IMAGE_FILE_LOADER_IMAGE_FILE_LOADER_HPP

#include "src/file_collector/file_collector.hpp"
#include "src/utils/utils.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <unordered_map>
#include <OpenImageIO/imageio.h>
#include <Imath/half.h>

namespace slideproj::image_file_loader
{
	enum class pixel_ordering{
		top_to_bottom_left_to_right,
		top_to_bottom_right_to_left,
		bottom_to_top_right_to_left,
		bottom_to_top_left_to_right,
		left_to_right_top_to_bottom,
		right_to_left_top_to_bottom,
		right_to_left_bottom_to_top,
		left_to_right_bottom_to_top
	};

	template<class Rep>
	struct exif_date_time_value
	{
		Rep value;
	};

	std::optional<std::array<int, 6>> make_ymdhms(exif_date_time_value<std::string_view> edtv);

	std::optional<statx_timestamp> make_statx_timestamp(exif_date_time_value<std::string_view> edtv);

	inline auto make_statx_timestamp(exif_date_time_value<std::string> const& edtv)
	{ return make_statx_timestamp(exif_date_time_value{std::string_view{edtv.value}}); }

	std::optional<statx_timestamp>
	get_statx_timestamp(OIIO::ImageSpec const& spec, std::string_view field_name);

	inline auto get_statx_timestamp(OIIO::ImageSpec const& spec)
	{
		auto ret = get_statx_timestamp(spec, "Exif:DateTimeOriginal");
		if(!ret)
		{ return get_statx_timestamp(spec, "DateTime"); }
		return ret;
	}

	std::optional<file_collector::file_clock::time_point>
	get_timestamp_from_fs(std::filesystem::path const& path);

	class exif_query_result
	{
	public:
		exif_query_result() = default;

		explicit exif_query_result(OIIO::ImageSpec const& spec);

		// TODO(c++26) Use optional reference
		std::string const* description() const
		{ return (m_valid_fields & description_valid)? &m_description : nullptr; }

		// TODO(c++26) Use optional reference
		file_collector::file_clock::time_point const* timestamp() const
		{ return (m_valid_fields&timestamp_valid)? &m_timestamp : nullptr; }

		enum pixel_ordering pixel_ordering() const
		{ return m_pixel_ordering; }

	private:
		static constexpr size_t description_valid = 0x1;
		static constexpr size_t timestamp_valid = 0x2;

		size_t m_valid_fields = 0;
		std::string m_description;
		file_collector::file_clock::time_point m_timestamp{};
		enum pixel_ordering m_pixel_ordering = pixel_ordering::top_to_bottom_left_to_right;
	};

	inline auto load_exif_query_result(std::filesystem::path const& path)
	{
		auto img_reader = OIIO::ImageInput::open(path);
		if(!img_reader)
		{ return exif_query_result{}; }

		// TODO: Need test image in order to test this
		return exif_query_result{img_reader->spec()};
	}

	struct image_file_info:file_collector::file_metadata
	{
		enum pixel_ordering pixel_ordering;
	};

	image_file_info load_metadata(std::filesystem::path const& path);

	class image_file_metadata_repository
	{
	public:
		image_file_info const& get_metadata(file_collector::file_list_entry const& entry) const;

	private:
		mutable std::unordered_map<file_collector::file_id, image_file_info> m_cache;
	};	static_assert(file_collector::file_metadata_provider<image_file_metadata_repository>);

	// Image conversion pipeline
	//
	// 2. Convert to premultiplied RGBA (if necessary)
	// 3. Fix orientation (if necessary)

	template<class Type, class IntensityTransferFunction>
	requires(std::is_empty_v<IntensityTransferFunction>)
	struct sample_type
	{
		Type value;
		constexpr float to_linear_float() const
		{ return IntensityTransferFunction::to_linear_float(value); }

		constexpr float to_raw_float() const
		{ return utils::to_normalized_float(value); }
	};

	template<class IntensityTransferFunction, class ... Types>
	using sample_types_with_itf = std::variant<sample_type<Types, IntensityTransferFunction>...>;

	struct linear_intensity_mapping
	{
		template<class T>
		static constexpr float to_linear_float(T value)
		{ return utils::to_normalized_float(value); }
	};

	struct srgb_intensity_mapping
	{
		template<class T>
		static constexpr float to_linear_float(T value)
		{
			auto const val = utils::to_normalized_float(value);
			return (val <= 0.04045f)? val/12.92f : std::pow((val + 0.055f)/1.055f, 2.4f);
		}
	};

	struct g22_intensity_mapping
	{
		template<class T>
		static constexpr float to_linear_float(T value)
		{
			auto const val = utils::to_normalized_float(value);
			return std::pow(val, 2.2f);
		}
	};

	template<class SampleType, size_t ChannelCount>
	struct pixel_type
	{};

	template<class SampleType>
	struct pixel_type<SampleType, 1>
	{
		SampleType gray;
	};

	template<class SampleType>
	struct pixel_type<SampleType, 2>
	{
		SampleType gray;
		SampleType alpha;
	};

	template<class SampleType>
	struct pixel_type<SampleType, 3>
	{
		SampleType red;
		SampleType green;
		SampleType blue;
	};

	template<class SampleType>
	struct pixel_type<SampleType, 4>
	{
		SampleType red;
		SampleType green;
		SampleType blue;
		SampleType alpha;
	};

	template<class SampleType, size_t ChannelCount>
	constexpr auto to_linear_rgba(pixel_type<SampleType, ChannelCount> value)
	{
		if constexpr(ChannelCount == 1)
		{
			auto const converted_value = value.gray.to_linear_float();
			return pixel_type<float, 4>{
				.red = converted_value,
				.green = converted_value,
				.blue = converted_value,
				.alpha = 1.0f
			};
		}
		else
		if constexpr(ChannelCount == 2)
		{
			auto const conv_gray = value.gray.to_linear_float();
			auto const conv_alpha = value.alpha.to_raw_float();
			return pixel_type<float, 4>{
				.red = conv_gray,
				.green = conv_gray,
				.blue = conv_gray,
				.alpha = conv_alpha
			};
		}
		else
		if constexpr(ChannelCount == 3)
		{
			return pixel_type<float, 4>{
				.red = value.red.to_linear_float(),
				.green = value.green.to_linear_float(),
				.blue = value.blue.to_linear_float(),
				.alpha = 1.0f
			};
		}
		else
		{
			return pixel_type<float, 4>{
				.red = value.red.to_linear_float(),
				.green = value.green.to_linear_float(),
				.blue = value.blue.to_linear_float(),
				.alpha = value.alpha.to_raw_float()
			};
		}
	}

	template<size_t ChannelCount, class IntensityTransferFunction>
	using pixel_buffer_varying_sample_size = std::variant<
		std::unique_ptr<pixel_type<sample_type<uint8_t, IntensityTransferFunction>, ChannelCount>[]>,
		std::unique_ptr<pixel_type<sample_type<uint16_t, IntensityTransferFunction>, ChannelCount>[]>,
		std::unique_ptr<pixel_type<sample_type<Imath::half, IntensityTransferFunction>, ChannelCount>[]>,
		std::unique_ptr<pixel_type<sample_type<float, IntensityTransferFunction>, ChannelCount>[]>
	>;

	template<class IntensityTransferFunction>
	using pixel_buffer_varying_channel_count = utils::concatenate_variants_t<
		pixel_buffer_varying_sample_size<1, IntensityTransferFunction>,
		pixel_buffer_varying_sample_size<2, IntensityTransferFunction>,
		pixel_buffer_varying_sample_size<3, IntensityTransferFunction>,
		pixel_buffer_varying_sample_size<4, IntensityTransferFunction>
	>;

	using pixel_buffer = utils::concatenate_variants_t<
		pixel_buffer_varying_channel_count<linear_intensity_mapping>,
		pixel_buffer_varying_channel_count<srgb_intensity_mapping>,
		pixel_buffer_varying_channel_count<g22_intensity_mapping>
	>;

	static_assert(std::variant_size_v<pixel_buffer> == 48);

	enum class intensity_transfer_function_id{linear, srgb, g22, invalid = -1};

	constexpr auto to_intensity_transfer_function_id(std::string_view sv)
	{
		if(sv == "Linear")
		{ return intensity_transfer_function_id::linear; }
		if(sv == "sRGB")
		{ return intensity_transfer_function_id::srgb; }
		if(sv == "Gamma2.2")
		{ return intensity_transfer_function_id::g22; }
		return intensity_transfer_function_id::invalid;
	}

	enum class sample_value_type_id{uint8, uint16, float16, float32, invalid = -1};

	constexpr auto to_value_type_id(OIIO::TypeDesc const& type)
	{
		if(type == OIIO::TypeDesc::UINT8)
		{ return sample_value_type_id::uint8; }

		if(type == OIIO::TypeDesc::UINT16)
		{ return sample_value_type_id::uint16; }

		if(type ==OIIO::TypeDesc::HALF)
		{ return sample_value_type_id::float16; }

		if(type == OIIO::TypeDesc::FLOAT)
		{ return sample_value_type_id::float32; }

		return sample_value_type_id::invalid;
	}

	class pixel_type_id
	{
	public:
		pixel_type_id() = default;

		constexpr explicit pixel_type_id(
			intensity_transfer_function_id itf_id,
			size_t channel_count,
			sample_value_type_id type_id
		): m_value{std::numeric_limits<size_t>::max()}
		{
			if(itf_id == intensity_transfer_function_id::invalid)
			{ return; }

			if(channel_count < 1 || channel_count > 4)
			{ return; }

			if(type_id == sample_value_type_id::invalid)
			{ return; }

			auto const tf = static_cast<size_t>(itf_id);
			auto const channel_index = channel_count - 1;
			m_value = static_cast<size_t>(type_id) + 4*channel_index + 16*tf;
		}

		constexpr bool is_valid() const
		{ return m_value != std::numeric_limits<size_t>::max(); }

		constexpr auto value() const
		{ return m_value; }

	private:
		size_t m_value = std::numeric_limits<size_t>::max();
	};

	struct make_uninitialized_pixel_buffer_tag{};

	class variant_image
	{
	public:
		variant_image() = default;

		explicit variant_image(
			pixel_type_id pixel_type,
			uint32_t w,
			uint32_t h,
			make_uninitialized_pixel_buffer_tag
		);

		auto width() const
		{ return m_width; }

		auto height() const
		{ return m_height; }

		auto pixel_count() const
		{ return static_cast<size_t>(width())*static_cast<size_t>(height()); }

		bool is_empty() const
		{ return m_width == 0 || m_height == 0; }

		template<class Callable>
		decltype(auto) visit(Callable&& cb)
		{
			return std::visit(
				[cb = std::forward<Callable>(cb), w = m_width, h = m_height](auto const& item){
					return cb(item.get(), w, h);
				},
				m_pixels
			);
		}

		template<class Callable>
		decltype(auto) visit(Callable&& cb) const
		{
			return std::visit(
				[cb = std::forward<Callable>(cb), w = m_width, h = m_height]<class T>(T const& item){
					using elem_type = T::element_type;
					return cb(static_cast<elem_type const*>(item.get()), w, h);
				},
				m_pixels
			);
		}

		template<class PixelType>
		PixelType const* pixels() const
		{
			auto item = std::get_if<std::unique_ptr<PixelType[]>>(&m_pixels);
			if(item == nullptr)
			{ return nullptr; }
			return item->get();
		}


	private:
		uint32_t m_width{0};
		uint32_t m_height{0};
		pixel_buffer m_pixels;
	};

	variant_image load_image(OIIO::ImageInput& input);

	variant_image load_image(std::filesystem::path const&);

	template<class T>
	class fixed_typed_image
	{
	public:
		fixed_typed_image() = default;

		explicit fixed_typed_image(
			uint32_t w,
			uint32_t h,
			make_uninitialized_pixel_buffer_tag
		):
			m_width{w},
			m_height{h},
			m_pixels{std::make_unique_for_overwrite<T[]>(static_cast<size_t>(w)*static_cast<size_t>(h))}
		{}

		auto width() const
		{ return m_width; }

		auto height() const
		{ return m_height; }

		auto pixel_count() const
		{ return static_cast<size_t>(width())*static_cast<size_t>(height()); }

		bool is_empty() const
		{ return m_width == 0 || m_height == 0; }

		auto pixels()
		{ return m_pixels.get(); }

		auto pixels() const
		{ return static_cast<T const*>(m_pixels.get()); }

	private:
		uint32_t m_width{0};
		uint32_t m_height{0};
		std::unique_ptr<T[]> m_pixels;
	};

	template<class PixelType>
	auto downsample_to_linear(PixelType const* pixels, uint32_t w, uint32_t h, uint32_t scaling_factor)
	{
		auto const w_out = w/scaling_factor;
		auto const h_out = h/scaling_factor;
		using pixel_type_ret = pixel_type<float, PixelType::channel_count>;
		fixed_typed_image<pixel_type_ret> ret{w_out, h_out, make_uninitialized_pixel_buffer_tag{}};
		for(uint32_t y = 0; y != h_out; ++y)
		{
			for(uint32_t x = 0; x != w_out; ++x)
			{
				pixel_type_ret avg{};
				for(uint32_t eta = 0; eta != scaling_factor; ++eta)
				{
					for(uint32_t xi = 0; xi != scaling_factor; ++xi)
					{ avg += pixels[(x + xi) + (y + eta)*w].to_linear_float(); }
				}
				avg /= static_cast<float>(scaling_factor*scaling_factor);
				ret[x + y*w] = avg;
			}
		}
		return ret;
	}

	template<class PixelType>
	auto to_rgba(PixelType const* pixels, uint32_t w, uint32_t h)
	{
		using pixel_type_ret = pixel_type<typename PixelType::value_type, 4>;
		fixed_typed_image<pixel_type_ret> ret{w, h, make_uninitialized_pixel_buffer_tag{}};
		for(uint32_t y = 0; y != h; ++y)
		{
			for(uint32_t x = 0; x != w; ++x)
			{ ret[x + y*w] = pixels[x + y*w].to_rgba(); }
		}
		return ret;
	}

	fixed_typed_image<pixel_type<float, 4>>
	make_linear_rgba_image(variant_image const& input, uint32_t scaling_factor);
};

#endif