#ifndef SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP

#include "./slideshow.hpp"

#include "src/event_types/windowing_events.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/pixel_store/rgba_image.hpp"
#include "src/utils/unwrap.hpp"
#include "src/utils/synchronized.hpp"

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

	template<class AppWindow, class TaskQueue>
	class slideshow_window_event_handler
	{
	public:
		explicit slideshow_window_event_handler(AppWindow window, TaskQueue task_queue):
			m_window{window},
			m_task_queue{task_queue}
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
			auto image_to_load = m_current_slideshow->get_current_entry();
			if(image_to_load == nullptr)
			{ return; }

			fprintf(stderr, "(i) Loading image %s\n", image_to_load->path().c_str());
			unwrap(m_task_queue).submit(
				[
					image_to_load = image_to_load->path(),
					rect = m_target_rectangle,
					current_image = std::ref(m_current_image)
				](){
					utils::unwrap(current_image) = image_file_loader::load_rgba_image(image_to_load, rect);
				}
			);
		}

		void handle_event(update_window const&)
		{
			auto image_to_show = m_current_image.take_value();
			if(!image_to_show.is_empty())
			{
				fprintf(stderr, "(i) Image loaded\n");
			}
		}

		bool application_should_exit() const
		{ return m_application_should_exit; }

	private:
		AppWindow m_window;
		TaskQueue m_task_queue;

		slideshow* m_current_slideshow{nullptr};
		image_file_loader::image_rectangle m_target_rectangle;
		utils::synchronized<pixel_store::rgba_image> m_current_image;
		bool m_application_should_exit{false};
	};
}

#endif