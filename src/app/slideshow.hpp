#ifndef SLIDEPROJ_APP_SLIDESHOW_HPP
#define SLIDEPROJ_APP_SLIDESHOW_HPP

#include "src/file_collector/file_collector.hpp"
#include <sys/types.h>

namespace slideproj::app
{
	struct slideshow_entry
	{
		ssize_t index = -1;
		file_collector::file_list_entry source_file;

		constexpr bool is_valid() const
		{ return index >= 0; }
	};

	class slideshow
	{
	public:
		slideshow() = default;

		explicit slideshow(file_collector::file_list&& files, ssize_t start_at = 0):
			m_files{std::move(files)}
		{ set_current_index(start_at); }

		auto get_entry(ssize_t offset) const
		{
			auto const read_from = m_current_index + offset;
			if(read_from < 0 || read_from >= std::ssize(m_files))
			{ return slideshow_entry{}; }

			return slideshow_entry{
				.index = read_from,
				.source_file = m_files[read_from]
			};
		}

		[[nodiscard]] auto step(ssize_t offset)
		{
			auto const read_from = m_current_index + offset;
			if(read_from < 0 || read_from >= std::ssize(m_files))
			{ return m_current_index; }

			auto const ret = m_current_index;
			m_current_index = read_from;
			return ret;
		}

		void set_current_index(ssize_t index)
		{
			if(std::size(m_files) <= 1)
			{
				m_current_index = 0;
				return;
			}

			m_current_index = std::clamp(index, static_cast<ssize_t>(0), std::ssize(m_files) - 1);
		}

		void go_to_end()
		{
			if(std::size(m_files) == 0)
			{
				m_current_index = 0;
				return;
			}
			m_current_index = std::size(m_files) - 1;
		}

		void go_to_begin()
		{ set_current_index(0); }

		ssize_t get_current_index() const
		{ return m_current_index; }

		bool empty() const
		{ return m_files.empty(); }

	private:
		file_collector::file_list m_files;
		ssize_t m_current_index{0};
	};

	enum class step_direction{forward, backward, none};

	inline constexpr step_direction make_step_direction_from_string(std::string_view str)
	{
		if(str == "forward")
		{ return step_direction::forward; }
		if(str == "backward")
		{ return step_direction::backward; }
		if(str == "none" || str == "paused")
		{ return step_direction::none; }

		throw std::runtime_error{"Unsupported step direction"};
	}

	struct slideshow_step_event
	{
		step_direction direction;
	};

	using slideshow_clock = std::chrono::steady_clock;

	struct slideshow_transition_end_event
	{
		slideshow_clock::time_point when;
	};

	struct slideshow_time_event
	{
		slideshow_clock::time_point when;
	};

	class slideshow_navigator
	{
	public:
		virtual void step_forward() = 0;
		virtual void step_backward() = 0;
		virtual void go_to_begin() = 0;
		virtual void go_to_end() = 0;
	};
}

#endif