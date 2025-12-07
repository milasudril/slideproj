#ifndef SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP

#include "./slideshow.hpp"

#include "src/event_types/windowing_events.hpp"
#include "src/utils/unwrap.hpp"

#include <GL/glew.h>
#include <GL/gl.h>

namespace slideproj::app
{
	struct slideshow_loaded
	{
		std::reference_wrapper<slideshow> current_slideshow;
	};

	template<class AppWindow>
	class slideshow_window_event_handler
	{
	public:
		explicit slideshow_window_event_handler(AppWindow window):
			m_window{window}
		{}

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
			m_application_should_exit = true;
		}

		void handle_event(event_types::typing_keyboard_event const& event)
		{
			if(
				event.scancode == event_types::typing_keyboard_scancode::f_11
				&& event.action == event_types::button_action::press
			)
			{ utils::unwrap(m_window).toggle_fullscreen(); }
			else
			{	fprintf(stderr, "(i) User pressed %d\n", event.scancode.value()); }
		}

		void handle_event(event_types::mouse_button_event const& event)
		{
			if(event.action != event_types::button_action::release || m_current_slideshow == nullptr)
			{ return; }

			auto const image_to_show = [&](auto const& event){
				if(event.button == event_types::mouse_button_index::left)
				{ return m_current_slideshow->get_previous_entry(); }
				else
				if(event.button == event_types::mouse_button_index::right)
				{ return m_current_slideshow->get_next_entry(); }
				return static_cast<file_collector::file_list_entry const*>(nullptr);
			}(event);

			if(image_to_show != nullptr)
			{ fprintf(stderr, "(i) Showing %s\n", image_to_show->path().c_str()); }
		}

		void handle_event(slideshow_loaded event)
		{
			fprintf(stderr, "(i) Slideshow loaded\n");
			m_current_slideshow = &event.current_slideshow.get();
		}

		bool application_should_exit() const
		{ return m_application_should_exit; }

	private:
		AppWindow m_window;
		slideshow* m_current_slideshow{nullptr};
		bool m_application_should_exit{false};
	};
}

#endif