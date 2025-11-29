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

	struct make_uninitialized_pixel_storage_tag{};

	template<class PixelType>
	class pixel_storage
	{
	public:
		using value_type = PixelType;

		pixel_storage() = default;

		explicit pixel_storage(uint32_t w, uint32_t h, make_uninitialized_pixel_storage_tag):
			m_width{w},
			m_height{h},
			m_pixels{
				std::make_unique_for_overwrite<PixelType[]>(static_cast<size_t>(w)*static_cast<size_t>(h))
			}
		{}

		explicit pixel_storage(uint32_t w, uint32_t h):
			m_width{w},
			m_height{h},
			m_pixels{std::make_unique<PixelType[]>(static_cast<size_t>(w)*static_cast<size_t>(h))}
		{}

		auto width() const
		{ return m_width; }

		auto height() const
		{ return m_height; }

		auto pixel_count() const
		{ return static_cast<size_t>(width())*static_cast<size_t>(height()); }

		auto pixels() const
		{
			return std::span{
				m_pixels.get(),
				pixel_count()
			};
		}

		bool is_empty() const
		{ return m_width == 0 || m_height == 0 || m_pixels == nullptr; }

	private:
		uint32_t m_width{0};
		uint32_t m_height{0};
		std::unique_ptr<PixelType[]> m_pixels;
	};

	// Image conversion pipeline
	//
	// 1.
	//    template<class PixelValue>
	//    pixel_storage<rgba_value<float>> downsample_to_linear(span_2d<PixelValue const> input)
	//
	//    PixelValue: gray, gray + alpha, rgb, rgba
	//    SampleValue: uint8_t, uint16_t, half, float
	//    TransferFunction linear, sRGB, g22
	//
	//    Total number of overloads 4*4*3 = 12*4 = 48
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
	{};

	struct srgb_intensity_mapping
	{};

	struct g22_intensity_mapping
	{};

	using sample_types = utils::concatenate_variants_t<
		sample_types_with_itf<linear_intensity_mapping>,
		sample_types_with_itf<srgb_intensity_mapping>,
		sample_types_with_itf<g22_intensity_mapping>
	>;

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


	template<class ValueType>
	struct color_value
	{
		using value_type = ValueType;
		ValueType r;
		ValueType g;
		ValueType b;
		ValueType a;
	};

	// TODO: Add support for more color types
	using dynamic_pixel_storage = std::variant<
		pixel_storage<color_value<uint8_t>>,
		pixel_storage<color_value<float>>
	>;

	template<class T>
	struct oiio_type_desc
	{};

	template<>
	struct oiio_type_desc<uint8_t>
	{
		static constexpr auto value = OIIO::TypeDesc::UINT8;
	};

	template<>
	struct oiio_type_desc<float>
	{
		static constexpr auto value = OIIO::TypeDesc::FLOAT;
	};

	template<class T>
	constexpr OIIO::TypeDesc oiio_type_desc_v = oiio_type_desc<T>::value;

	template<class T>
	struct rgba_value
	{
		T red;
		T green;
		T blue;
		T alpha;
	};

	template<class T>
	struct rgb_value
	{
		T red;
		T green;
		T blue;
	};

	template<class T>
	struct gray_alpha_value
	{
		T gray;
		T alpha;
	};

	template<class T>
	struct gray_value
	{
		T gray;
	};

	template<class... SupportedTypes>
	class pixel_storage_2
	{
	public:
		pixel_storage_2() = default;

		template<class PixelType>
		explicit pixel_storage_2(
			uint32_t w,
			uint32_t h,
			std::reference_wrapper<PixelType*> stored_pointer,
			make_uninitialized_pixel_storage_tag
		):
			m_width{w},
			m_height{h},
			m_pixels{
				std::make_unique_for_overwrite<PixelType[]>(static_cast<size_t>(w)*static_cast<size_t>(h))
			}
		{ stored_pointer = m_pixels.get(); }

		template<class PixelType>
		explicit pixel_storage_2(
			uint32_t w,
			uint32_t h,
			std::reference_wrapper<PixelType*> stored_pointer
		):
			m_width{w},
			m_height{h},
			m_pixels{std::make_unique<PixelType[]>(static_cast<size_t>(w)*static_cast<size_t>(h))}
		{ stored_pointer = m_pixels.get(); }

		auto width() const
		{ return m_width; }

		auto height() const
		{ return m_height; }

		auto pixel_count() const
		{ return static_cast<size_t>(width())*static_cast<size_t>(height()); }

		auto pixels() const
		{
			return std::span{
				m_pixels.get(),
				pixel_count()
			};
		}

		bool is_empty() const
		{ return m_width == 0 || m_height == 0 || m_pixels == nullptr; }

	private:
		uint32_t m_width{0};
		uint32_t m_height{0};
		std::variant<std::unique_ptr<SupportedTypes[]>...> m_pixels;
	};

	enum class intensity_transfer_function{linear, srgb, g22};

	struct image
	{
		intensity_transfer_function transfer_function{intensity_transfer_function::linear};
		dynamic_pixel_storage pixels;
	};

	template<class ValueType>
	pixel_storage<color_value<ValueType>> load_rgba_image(OIIO::ImageInput& input)
	{
		auto const& spec = input.spec();
		static constexpr auto nchannels = 4;
		static constexpr auto format = oiio_type_desc_v<ValueType>;
		assert(spec.nchannels == nchannels);
		assert(spec.format == format);

		if(spec.width <= 0 || spec.height <= 0)
		{ return pixel_storage<color_value<ValueType>>{}; }

		pixel_storage<color_value<ValueType>> ret{
			static_cast<uint32_t>(spec.width),
			static_cast<uint32_t>(spec.height)
		};
		input.read_image(0, 0, 0, nchannels, format, ret.pixels().data());

		return ret;
	}

	template<class ValueType>
	dynamic_pixel_storage load_as_dynamic_pixel_storage(OIIO::ImageInput& input)
	{ return dynamic_pixel_storage{load_rgba_image<ValueType>(input)}; }

	inline dynamic_pixel_storage load_unsupported_rgba_image(OIIO::ImageInput&)
	{ return dynamic_pixel_storage{}; }

	using rgba_image_loader = dynamic_pixel_storage (*)(OIIO::ImageInput&);

	rgba_image_loader get_rgba_image_loader(OIIO::TypeDesc format);

	image load(std::filesystem::path const&);
};

#endif