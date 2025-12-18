#ifndef SLIDEPROJ_APP_SLIDESHOW_PLAYBACK_CONTROLLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_PLAYBACK_CONTROLLER_HPP

#include "./slideshow.hpp"

#include <cstdio>

namespace slideproj::app
{
	class slideshow_playback_controller
	{
	public:
		void handle_event(slideshow_navigator&, slideshow_step_event event)
		{
			fprintf(stderr, "(i) Step\n");
			if(event.direction != step_direction::none)
			{ m_direction = event.direction; }
			m_latest_transtion_end.reset();
		}

		void handle_event(slideshow_navigator&, slideshow_transition_end_event event)
		{
			fprintf(stderr, "(i) Transition ended\n");
			m_latest_transtion_end = event.when;
		}

		void handle_event(slideshow_navigator& navigator, slideshow_time_event event)
		{
			if(m_latest_transtion_end.has_value() && m_direction != step_direction::none)
			{
				if(event.when - *m_latest_transtion_end >= m_step_delay)
				{
					fprintf(stderr, "(i) Frame expired\n");
					switch (m_direction)
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
			if(m_direction == step_direction::none)
			{ m_direction = m_saved_direction; }
			else
			{ m_saved_direction = std::exchange(m_direction, step_direction::none); }
		}


	private:
		std::chrono::duration<float> m_step_delay{6.0f};
		std::optional<slideshow_clock::time_point> m_latest_transtion_end;
		step_direction m_direction{step_direction::forward};
		step_direction m_saved_direction{step_direction::forward};
	};
}
#endif