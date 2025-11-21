//@	{"target": {"name":"file_collector.o"}}

#include "./file_collector.hpp"
#include <filesystem>

slideproj::file_collector::file_list
slideproj::file_collector::make_file_list(std::filesystem::path const& input_directory)
{
	file_list ret;
	namespace fs = std::filesystem;
	using diropts = fs::directory_options;
	using dir_iter = fs::recursive_directory_iterator;
	for (auto const& dir_entry : dir_iter{input_directory, diropts::skip_permission_denied})
	{
		// For now, only store regular files. Directories could be useful to generate automatic
		// title screens
		if(dir_entry.is_regular_file())
		{ ret.append(dir_entry.path());}
	}

	return ret;
}