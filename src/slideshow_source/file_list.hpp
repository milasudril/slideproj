#ifndef SLIDEPROJ_SLIDESHOW_SOURCE_FILE_LIST_HPP
#define SLIDEPROJ_SLIDESHOW_SOURCE_FILE_LIST_HPP

#include <vector>
#include <algorithm>
#include <filesystem>

namespace slideproj::slideshow_source
{
	class file_list
	{
	public:
		struct from_range_t{};

		template<class PathSource>
		explicit file_list(from_range_t, PathSource&& src):
			m_entries{std::begin(src), std::end(src)}
		{}

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

		auto& operator[](size_t index) const
		{
			// NOTE: This is expected to be safe since index will be validated
			//       before calling this function
			return m_entries[index];
		}

	private:
		std::vector<std::filesystem::path> m_entries;
	};
}

#endif