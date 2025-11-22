//@	{
//@	 "target": {"name":"image_file_loader.o"},
//@	 "dependencies":[{"ref":"OpenImageIO", "origin":"pkg-config"}]
//@	}

#include "./image_file_loader.hpp"
#include "src/file_collector/file_collector.hpp"

#include <cstring>
#include <linux/stat.h>
#include <stdexcept>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <OpenImageIO/imageio.h>

slideproj::image_file_loader::image_file_info
slideproj::image_file_loader::load_metadata(std::filesystem::path const& path)
{
	auto img_reader = OIIO::ImageInput::open(path);
	if(!img_reader)
	{ throw std::runtime_error{img_reader->geterror()}; }

	auto const& info = img_reader->spec();
	printf("%s\n",info.get_string_attribute("DateTime").c_str());
	printf("%s\n",info.get_string_attribute("Exif:DateTimeOriginal").c_str());


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