#ifndef SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP

#include "./slideshow_controller.hpp"

#include "src/windowing_api/event_types.hpp"
#include "src/windowing_api/application_window.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/utils/unwrap.hpp"

#include <GL/glew.h>
#include <GL/gl.h>

namespace slideproj::app
{
	struct slideshow_loaded
	{
		std::reference_wrapper<slideshow> current_slideshow;
	};

	struct update_window
	{
		size_t frame_number;
		std::chrono::steady_clock::duration time_since_last_frame;
	};

	class slideshow_window_event_handler
	{
	public:
		explicit slideshow_window_event_handler(slideshow_controller& slideshow_controller):
			m_slideshow_controller{slideshow_controller}
		{}

		void handle_event(
			windowing_api::application_window&,
			windowing_api::frame_buffer_size_changed_event event
		)
		{
			auto const w = event.width;
			auto const h = event.height;
			fprintf(stderr, "(i) Framebuffer size changed to %d %d\n", w, h);
			utils::unwrap(m_slideshow_controller).set_window_size(
				image_file_loader::image_rectangle{
					.width = static_cast<uint32_t>(w),
					.height = static_cast<uint32_t>(h)
				}
			);
			glViewport(0, 0, w, h);
		}

		void handle_event(
			windowing_api::application_window&,
			slideproj::windowing_api::window_is_closing_event
		)
		{
			fprintf(stderr, "(i) Window is closing\n");
			m_application_should_exit = true;
		}

		void handle_event(
			windowing_api::application_window& window,
			windowing_api::typing_keyboard_event const& event
		)
		{
			if(
				event.scancode == windowing_api::typing_keyboard_scancode::f_11
				&& event.action == windowing_api::button_action::press
			)
			{ window.toggle_fullscreen(); }
			else
			{	fprintf(stderr, "(i) User pressed %d\n", event.scancode.value()); }
		}

		void handle_event(
			windowing_api::application_window&,
			windowing_api::mouse_button_event const& event
		)
		{
			if(event.action != windowing_api::button_action::release)
			{ return; }

			if(event.button == windowing_api::mouse_button_index::left)
			{ utils::unwrap(m_slideshow_controller).step_backward(); }
			else
			if(event.button == windowing_api::mouse_button_index::right)
			{ utils::unwrap(m_slideshow_controller).step_forward();}
		}

		void handle_event(slideshow_loaded event)
		{ utils::unwrap(m_slideshow_controller).start_slideshow(event.current_slideshow); }

		void handle_event(update_window const&)
		{ }

		bool application_should_exit() const
		{ return m_application_should_exit; }

	private:
		std::reference_wrapper<slideshow_controller> m_slideshow_controller;
		bool m_application_should_exit{false};
	};
}

#endif