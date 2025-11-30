//@	{"dependencies_extra":[{"ref":"./file_collector.o", "rel":"implementation"}]}

#ifndef SLIDEPROJ_FILE_COLLECTOR_FILE_COLLECTOR_HPP
#define SLIDEPROJ_FILE_COLLECTOR_FILE_COLLECTOR_HPP

#include <chrono>
#include <linux/stat.h>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <functional>

namespace slideproj::file_collector
{
	class file_id
	{
	public:
		constexpr explicit file_id(size_t value):
			m_value{value}
		{}

		constexpr size_t value() const
		{ return m_value; }

		constexpr bool operator==(file_id const&) const = default;
		constexpr bool operator!=(file_id const&) const = default;

	private:
		size_t m_value;
	};

	class file_list_entry
	{
	public:
		explicit file_list_entry(file_id id, std::filesystem::path const& path):
			m_id{id}, m_path{path}
		{}

		file_id id() const
		{ return m_id; }

		std::filesystem::path path() const
		{ return m_path; }

		bool operator==(file_list_entry const&) const = default;
		bool operator!=(file_list_entry const&) const = default;

	private:
		file_id m_id;
		std::filesystem::path m_path;
	};

	class file_list
	{
	public:
		size_t size() const
		{ return std::size(m_entries); }

		auto begin() const
		{ return std::begin(m_entries); }

		auto end() const
		{ return std::end(m_entries); }

		template<class Predicate>
		file_list& sort(Predicate&& pred)
		{
			std::ranges::sort(m_entries, std::forward<Predicate>(pred));
			return *this;
		}

		file_list& append(std::filesystem::path const& path)
		{
			file_id new_id{size()};
			m_entries.push_back(file_list_entry{new_id, path});
			return *this;
		}

		auto& operator[](size_t index) const
		{
			// NOTE: This is expected to be safe since index will be validated
			//       before calling this function
			return m_entries[index];
		}

	private:
		std::vector<file_list_entry> m_entries;
	};

	template<class T>
	concept input_filter = requires(T const& obj, std::filesystem::directory_entry const& item){
		{obj.accepts(item)} -> std::same_as<bool>;
	};

	struct type_erased_input_filter{
		void const* object;
		bool (*accepts)(void const*, std::filesystem::directory_entry const&);
	};

	file_list make_file_list(
		std::filesystem::path const& input_directory,
		type_erased_input_filter input_filter
	);

	template<input_filter InputFilter>
	file_list make_file_list(
		std::filesystem::path const& input_directory,
		InputFilter&& input_filter
	)
	{
		return make_file_list(
			input_directory,
			type_erased_input_filter{
				.object = &input_filter,
				.accepts = [](void const* obj, std::filesystem::directory_entry const& entry){
					return static_cast<InputFilter const*>(obj)->accepts(entry);
				}
			}
		);
	}

	enum class file_metadata_field{
		in_group,
		caption,
		timestamp
	};

	class file_clock
	{
	public:
		using rep = __int128;
		using period = std::nano;
		using duration = std::chrono::duration<rep, period>;
		using time_point = std::chrono::time_point<file_clock>;

		static constexpr time_point create(statx_timestamp const& tv)
		{
			return time_point{
				duration{rep{1'000'000'000}*rep{tv.tv_sec} + rep{tv.tv_nsec}}
			};
		}
	};

	struct file_metadata
	{
		file_clock::time_point timestamp;
		std::string in_group;
		std::string caption;
	};

	template<class T>
	concept string_comparator = requires(T const& obj, std::string_view a, std::string_view b){
		{obj(a, b)} -> std::same_as<std::strong_ordering>;
	};

	template<string_comparator StringComparator>
	inline auto compare_field(
		file_metadata_field field,
		file_metadata const& a,
		file_metadata const& b,
		StringComparator const& strcmp
	)
	{
		switch(field)
		{
			case file_metadata_field::timestamp:
				return a.timestamp <=> b.timestamp;
			case file_metadata_field::in_group:
				return strcmp(a.in_group, b.in_group);
			case file_metadata_field::caption:
				return strcmp(a.caption, b.caption);
		}
		__builtin_unreachable();
	}

	template<class T>
	concept file_metadata_provider = requires(T const& obj, file_list_entry const& item){
		{obj.get_metadata(item)} -> std::convertible_to<file_metadata const&>;
	};

	struct type_erased_file_metadata_provider{
		void const* object;
		file_metadata const& (*get_metadata)(void const*, file_list_entry const&);
	};

	struct type_erased_string_comparator{
		void const* object;
		std::strong_ordering (*compare)(void const*, std::string_view a, std::string_view b);
	};

	void sort(
		file_list& files,
		std::span<file_metadata_field const> sort_by,
		type_erased_file_metadata_provider metadata_provider,
		type_erased_string_comparator string_comparator
	);

	template<
		file_metadata_provider FileMetadataProvider,
		string_comparator StringComparator
	>
	void sort(
		file_list& files,
		std::span<file_metadata_field const> sort_by,
		FileMetadataProvider const& metadata_provider,
		StringComparator const& string_comparator
	)
	{
		sort(
			files,
			sort_by,
			type_erased_file_metadata_provider{
				.object = &metadata_provider,
				.get_metadata = [](void const* handle, file_list_entry const& item) -> file_metadata const& {
					return static_cast<FileMetadataProvider const*>(handle)->get_metadata(item);
				}
			},
			type_erased_string_comparator{
				.object = &string_comparator,
				.compare = [](void const* handle, std::string_view a, std::string_view b) {
					return (*static_cast<StringComparator const*>(handle))(a, b);
				}
			}
		);
	}

	template<
		input_filter InputFilter,
		file_metadata_provider FileMetadataProvider,
		string_comparator StringComparator
	>
	inline file_list make_file_list(
		std::filesystem::path const& input_directory,
		InputFilter&& input_filter,
		std::span<file_metadata_field const> sort_by,
		FileMetadataProvider const& metadata_provider,
		StringComparator const& string_comparator
	)
	{
		auto ret = make_file_list(input_directory, std::forward<InputFilter>(input_filter));
		sort(ret, sort_by, metadata_provider, string_comparator);
		return ret;
	}

	std::optional<file_clock::time_point> get_timestamp(std::filesystem::path const& path);
}

template<>
struct std::hash<slideproj::file_collector::file_id>
{
	auto operator()(slideproj::file_collector::file_id id) const
	{
		static_assert(std::is_empty_v<std::hash<size_t>>);
		return std::hash<size_t>{}(id.value());
	}
};

#endif