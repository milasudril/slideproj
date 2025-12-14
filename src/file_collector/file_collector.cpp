//@	{"target": {"name":"file_collector.o"}}

#include "./file_collector.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <filesystem>

slideproj::file_collector::file_list
slideproj::file_collector::make_file_list(
	std::filesystem::path const& input_directory,
	type_erased_input_filter input_filter
)
{
	file_list ret;
	namespace fs = std::filesystem;
	using diropts = fs::directory_options;
	using dir_iter = fs::recursive_directory_iterator;
	for (auto const& dir_entry : dir_iter{input_directory, diropts::skip_permission_denied})
	{
		// For now, only store regular files. Directories could be useful to generate automatic
		// title screens
		if(dir_entry.is_regular_file() && input_filter.accepts(input_filter.object, dir_entry))
		{ ret.append(dir_entry.path()); }
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

std::optional<slideproj::file_collector::file_clock::time_point>
slideproj::file_collector::get_timestamp(std::filesystem::path const& path)
{
	struct statx statxbuf{};
	auto res = statx(AT_FDCWD, path.c_str(), AT_NO_AUTOMOUNT, STATX_BTIME | STATX_MTIME, &statxbuf);
	if(res == -1)
	{ return std::nullopt; }

	if(statxbuf.stx_mask&STATX_BTIME)
	{	return file_clock::create(statxbuf.stx_btime); }
	else
	if(statxbuf.stx_mask&STATX_MTIME)
	{ return file_clock::create(statxbuf.stx_mtime); }

	return std::nullopt;
}