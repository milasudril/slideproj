//@	{"target": {"name":"image_file_loader.o"}}

#include "./image_file_loader.hpp"

slideproj::image_file_loader::image_file_info
slideproj::image_file_loader::load_metadata(std::filesystem::path const&)
{
	return image_file_info{};
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