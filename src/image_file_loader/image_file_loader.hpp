//@	{"dependencies_extra":[{"ref":"./image_file_loader.o", "rel":"implementation"}]}

#ifndef SLIDEPROJ_IMAGE_FILE_LOADER_IMAGE_FILE_LOADER_HPP
#define SLIDEPROJ_IMAGE_FILE_LOADER_IMAGE_FILE_LOADER_HPP

#include "src/file_collector/file_collector.hpp"

#include <algorithm>
#include <unordered_map>
#include <OpenImageIO/imageio.h>

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
		explicit exif_query_result(std::filesystem::path const& path);

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
		file_collector::file_clock::time_point m_timestamp;
		enum pixel_ordering m_pixel_ordering = pixel_ordering::top_to_bottom_left_to_right;
	};

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
	};

	static_assert(file_collector::file_metadata_provider<image_file_metadata_repository>);
};

#endif