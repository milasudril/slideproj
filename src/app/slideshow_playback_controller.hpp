#ifndef SLIDEPROJ_APP_SLIDESHOW_PLAYBACK_CONTROLLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_PLAYBACK_CONTROLLER_HPP

#include "./slideshow.hpp"

#include <cstdio>

namespace slideproj::app
{
	struct slideshow_playback_descriptor
	{
		slideshow_clock::duration step_delay = std::chrono::seconds{6};
		enum step_direction step_direction = step_direction::forward;
	};

	class slideshow_playback_controller
	{
	public:
		explicit slideshow_playback_controller(slideshow_playback_descriptor const& params):
			m_params{params},
			m_saved_step_direction{params.step_direction}
		{}

		void handle_event(slideshow_navigator&, slideshow_step_event event)
		{
			if(event.direction != step_direction::none && m_params.step_direction != step_direction::none)
			{ m_params.step_direction = event.direction; }
			m_latest_transtion_end.reset();
		}

		void handle_event(slideshow_navigator&, slideshow_transition_end_event event)
		{
			m_latest_transtion_end = event.when;
		}

		void handle_event(slideshow_navigator& navigator, slideshow_time_event event)
		{
			if(m_latest_transtion_end.has_value() && m_params.step_direction != step_direction::none)
			{
				if(event.when - *m_latest_transtion_end >= m_params.step_delay)
				{
					switch (m_params.step_direction)
					{
						case step_direction::forward:
							navigator.step_forward();
							break;
						case step_direction::backward:
							navigator.step_backward();
							break;
						default:
							break;
					}
				}
			}
		}

		void toggle_pause()
		{
			if(m_params.step_direction == step_direction::none)
			{ m_params.step_direction = m_saved_step_direction; }
			else
			{ m_saved_step_direction = std::exchange(m_params.step_direction, step_direction::none); }
		}


	private:
		std::optional<slideshow_clock::time_point> m_latest_transtion_end;
		slideshow_playback_descriptor m_params;
		step_direction m_saved_step_direction;
	};
}
#endif