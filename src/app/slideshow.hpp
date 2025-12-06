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
		file_collector::file_list_entry const* get_current_entry()
		{
			if(m_current_index < 0 || m_current_index >= std::ssize(m_files))
			{ return nullptr; }
			return &m_files[m_current_index];
		}

		// TODO: C++26 optional reference
		auto get_next_entry()
		{
			set_current_index(m_current_index + 1);
			return get_current_entry();
		}

		// TODO: C++26 optional reference
		auto get_previous_entry()
		{
			set_current_index(m_current_index - 1);
			return get_current_entry();
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