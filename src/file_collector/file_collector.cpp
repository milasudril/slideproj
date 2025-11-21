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

void slideproj::file_collector::sort(
	file_list& files,
	std::span<file_metadata_field const> sort_by,
	type_erased_file_metadata_provider metadata_provider,
	type_erased_string_comparator string_comparator
)
{
	files.sort(
		[
			sort_by,
			metadata_provider,
			string_comparator
		](file_list_entry const& a, file_list_entry const& b) {
			auto const& metadata_a = metadata_provider.get_metadata(metadata_provider.object, a);
			auto const& metadata_b = metadata_provider.get_metadata(metadata_provider.object, b);
			auto strcmp = [string_comparator](std::string_view a, std::string_view b){
				return string_comparator.compare(string_comparator.object, a, b);
			};
			for(auto field:sort_by)
			{
				auto const res = compare_field(field, metadata_a, metadata_b, strcmp);
				if(res < 0)
				{ return true; }
				if(res > 0)
				{ return false;}
			}
			return false;
		}
	);
}