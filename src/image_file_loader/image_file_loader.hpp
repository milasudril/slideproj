//@	{"dependencies_extra":[{"ref":"./image_file_loader.o", "rel":"implementation"}]}

#ifndef SLIDEPROJ_IMAGE_FILE_LOADER_IMAGE_FILE_LOADER_HPP
#define SLIDEPROJ_IMAGE_FILE_LOADER_IMAGE_FILE_LOADER_HPP

#include "src/file_collector/file_collector.hpp"

#include <unordered_map>

namespace slideproj::image_file_loader
{
	struct image_file_info:file_collector::file_metadata
	{

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