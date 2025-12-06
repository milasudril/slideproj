#ifndef SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP

#include "src/event_types/windowing_events.hpp"
#include "src/utils/unwrap.hpp"

#include <GL/glew.h>
#include <GL/gl.h>

namespace slideproj::app
{
	template<class AppWindow>
	class slideshow_window_event_handler
	{
	public:
		void handle_event(event_types::frame_buffer_size_changed_event event)
		{
			auto const w = event.width;
			auto const h = event.height;
			fprintf(stderr, "(i) Framebuffer size changed to %d %d\n", w, h);
			glViewport(0, 0, w, h);
		}

		void handle_event(slideproj::event_types::window_is_closing_event)
		{
			fprintf(stderr, "(i) Window is closing\n");
			application_should_exit = true;
		}

		void handle_event(event_types::typing_keyboard_event const& event)
		{
			if(
				event.scancode == event_types::typing_keyboard_scancode::f_11
				&& event.action == event_types::button_action::press
			)
			{ utils::unwrap(window).toggle_fullscreen(); }
			else
			{	fprintf(stderr, "(i) User pressed %d\n", event.scancode.value()); }
		}

		void handle_event(event_types::mouse_button_event const& event)
		{
			fprintf(stderr, "(i) User pressed %d\n", event.button.value());
		}

		AppWindow window;
		bool application_should_exit{false};
	};
}

#endif