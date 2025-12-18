#ifndef SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP

#include "./slideshow.hpp"

#include "src/windowing_api/event_types.hpp"
#include "src/windowing_api/application_window.hpp"
#include "src/pixel_store/basic_image.hpp"
#include "src/utils/unwrap.hpp"

#include <GL/glew.h>
#include <GL/gl.h>

namespace slideproj::app
{
	template<class T>
	concept image_rect_sink = requires(T& obj, pixel_store::image_rectangle rect)
	{
		{ obj.set_window_size(rect) } -> std::same_as<void>;
	};

	struct image_rect_sink_ref
	{
		void* object;
		void (*set_window_size)(void*, pixel_store::image_rectangle);
	};

	template<class ... Args>
	auto make_image_rect_sink_refs(Args&... objects)
	{
		return std::array{
			image_rect_sink_ref{
				.object = &objects,
				.set_window_size = [](void* object, pixel_store::image_rectangle rect) {
					static_cast<Args*>(object)->set_window_size(rect);
				}
			}...
		};
	}

	template<class T>
	concept playback_controller = requires(T& x)
	{
		{ x.toggle_pause() } -> std::same_as<void>;
	};

	struct type_erased_playback_controller
	{
		void* object;
		void (*toggle_pause)(void*);
	};

	class slideshow_window_event_handler
	{
	public:
		template<playback_controller PlaybackController>
		explicit slideshow_window_event_handler(
			slideshow_navigator& navigator,
			std::span<image_rect_sink_ref const> rect_sinks,
			PlaybackController& playback_controller
		):
			m_navigator{navigator},
			m_rect_sinks{std::begin(rect_sinks), std::end(rect_sinks)},
			m_playback_controller{
				.object = &playback_controller,
				.toggle_pause = [](void* object){
					static_cast<PlaybackController*>(object)->toggle_pause();
				}
			}
		{}

		void handle_event(
			windowing_api::application_window&,
			windowing_api::frame_buffer_size_changed_event event
		)
		{
			auto const w = event.width;
			auto const h = event.height;
			fprintf(stderr, "(i) Framebuffer size changed to %d %d\n", w, h);
			pixel_store::image_rectangle const rect{
				.width = static_cast<uint32_t>(w),
				.height = static_cast<uint32_t>(h)
			};
			for(auto item : m_rect_sinks)
			{
				item.set_window_size(item.object, rect);
			}
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
			if(event.action == windowing_api::button_action::press)
			{
				if(event.scancode == windowing_api::typing_keyboard_scancode::f_11)
				{
					if(window.fullscreen_is_enabled())
					{ window.disable_fullscreen(); }
					else
					{ window.enable_fullscreen(); }
				}
				else
				if(event.scancode == windowing_api::typing_keyboard_scancode::whitespace)
				{ m_playback_controller.toggle_pause(m_playback_controller.object); }
			}

			if( event.action == windowing_api::button_action::press
				|| event.action == windowing_api::button_action::repeat
			)
			{
				if(event.scancode == windowing_api::typing_keyboard_scancode::arrow_left)
				{ utils::unwrap(m_navigator).step_backward(); }
				else
				if(event.scancode == windowing_api::typing_keyboard_scancode::arrow_right)
				{ utils::unwrap(m_navigator).step_forward(); }
				else
				if(event.scancode == windowing_api::typing_keyboard_scancode::home)
				{ utils::unwrap(m_navigator).go_to_begin(); }
				else
				if(event.scancode == windowing_api::typing_keyboard_scancode::end)
				{ utils::unwrap(m_navigator).go_to_end(); }
			}
		}

		void handle_event(
			windowing_api::application_window& window,
			windowing_api::mouse_button_event const& event
		)
		{
			if(event.action != windowing_api::button_action::release)
			{ return; }

			if(event.button == windowing_api::mouse_button_index::left)
			{ utils::unwrap(m_navigator).step_backward(); }
			else
			if(event.button == windowing_api::mouse_button_index::right)
			{ utils::unwrap(m_navigator).step_forward();}
			else
			if(event.button == windowing_api::mouse_button_index::middle)
			{
				if(window.get_cursor_mode() == windowing_api::cursor_mode::normal)
				{ window.set_cursor_mode(windowing_api::cursor_mode::hidden); }
				else
				{ window.set_cursor_mode(windowing_api::cursor_mode::normal); }
			}
		}

		bool application_should_exit() const
		{ return m_application_should_exit; }

	private:
		std::reference_wrapper<slideshow_navigator> m_navigator;
		std::vector<image_rect_sink_ref> m_rect_sinks;
		bool m_application_should_exit{false};
		type_erased_playback_controller m_playback_controller;
	};
}

#endif