#ifndef SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP

#include "./slideshow.hpp"

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
			if(event.action != event_types::button_action::release)
			{ return; }

			auto const image_to_show = [&](auto const& event){
				if(event.button == event_types::mouse_button_index::left)
				{ return current_slideshow.get().get_previous_entry(); }
				else
				if(event.button == event_types::mouse_button_index::right)
				{ return current_slideshow.get().get_next_entry(); }
				return static_cast<file_collector::file_list_entry const*>(nullptr);
			}(event);

			if(image_to_show != nullptr)
			{ fprintf(stderr, "(i) Showing %s\n", image_to_show->path().c_str()); }
		}

		AppWindow window;
		std::reference_wrapper<slideshow> current_slideshow;

		bool application_should_exit{false};
	};
}

#endif