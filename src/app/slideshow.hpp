#ifndef SLIDEPROJ_APP_SLIDESHOW_HPP
#define SLIDEPROJ_APP_SLIDESHOW_HPP

#include "src/file_collector/file_collector.hpp"

namespace slideproj::app
{
	class slideshow
	{
	public:
		slideshow() = default;

		explicit slideshow(file_collector::file_list&& files, ssize_t start_at = 0):
			m_files{std::move(files)}
		{ set_current_index(start_at); }

		// TODO: C++26 optional reference
		file_collector::file_list_entry const* get_entry(ssize_t offset)
		{
			auto const read_from = m_current_index + offset;
			if(read_from < 0 || read_from >= std::ssize(m_files))
			{ return nullptr; }

			return &m_files[read_from];
		}

		// TODO: C++26 optional reference
		file_collector::file_list_entry const* step_and_get_entry(ssize_t offset)
		{
			auto const read_from = m_current_index + offset;
			if(read_from < 0 || read_from >= std::ssize(m_files))
			{ return nullptr; }

			m_current_index = read_from;
			return &m_files[read_from];
		}

		void set_current_index(ssize_t index)
		{ m_current_index = std::clamp(index, static_cast<ssize_t>(-1), std::ssize(m_files) - 1); }

		size_t get_current_index() const
		{ return m_current_index; }

		bool empty() const
		{ return m_files.empty(); }

	private:
		file_collector::file_list m_files;
		ssize_t m_current_index{0};
	};
}

#endif