//@	{"dependencies_extra":[{"ref":"./file_collector.o", "rel":"implementation"}]}

#ifndef SLIDEPROJ_FILE_COLLECTOR_FILE_COLLECTOR_HPP
#define SLIDEPROJ_FILE_COLLECTOR_FILE_COLLECTOR_HPP

#include <chrono>
#include <vector>
#include <algorithm>
#include <filesystem>

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

	file_list make_file_list(std::filesystem::path const& input_directory);

	enum class file_metadata_field{
		in_group,
		caption,
		timestamp
	};

	struct file_metadata
	{
		std::string in_group;
		std::string caption;
		std::chrono::file_clock timestamp;
	};

	class file_metadata_provider
	{
	public:
		virtual file_metadata const& get_metadata(file_list_entry const& item) const = 0;
	};

	file_list make_file_list(
		std::filesystem::path const& input_directory,
		file_metadata_provider const& metadata_provider,
		std::span<file_metadata_field const> sort_by
	);
}

#endif