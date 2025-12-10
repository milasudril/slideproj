#ifndef SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_WINDOW_EVENT_HANDLER_HPP

#include "./slideshow_controller.hpp"

#include "src/windowing_api/event_types.hpp"
#include "src/windowing_api/application_window.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/pixel_store/rgba_image.hpp"
#include "src/utils/bidirectional_sliding_window.hpp"
#include "src/utils/unwrap.hpp"
#include "src/utils/synchronized.hpp"
#include "src/utils/task_queue.hpp"

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
		explicit slideshow_window_event_handler(utils::task_queue& task_queue):
			m_task_queue{task_queue}
		{}

		void handle_event(
			windowing_api::application_window&,
			windowing_api::frame_buffer_size_changed_event event
		)
		{
			auto const w = event.width;
			auto const h = event.height;
			fprintf(stderr, "(i) Framebuffer size changed to %d %d\n", w, h);
			m_target_rectangle.width = static_cast<uint32_t>(w);
			m_target_rectangle.height = static_cast<uint32_t>(h);
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
			if(event.action != windowing_api::button_action::release || m_current_slideshow == nullptr)
			{ return; }

			auto const image_to_show = [&](auto const& event){
				if(event.button == windowing_api::mouse_button_index::left)
				{ return m_current_slideshow->step_and_get_entry(-1); }
				else
				if(event.button == windowing_api::mouse_button_index::right)
				{ return m_current_slideshow->step_and_get_entry(1); }
				return static_cast<file_collector::file_list_entry const*>(nullptr);
			}(event);

			if(image_to_show != nullptr)
			{ fprintf(stderr, "(i) Showing %s\n", image_to_show->path().c_str()); }
		}

		void handle_event(slideshow_loaded event)
		{
			fprintf(stderr, "(i) Slideshow loaded\n");
			m_current_slideshow = &event.current_slideshow.get();
			unwrap(m_task_queue).submit(
				[
					images_to_load = std::array{
						m_current_slideshow->get_entry(-1),
						m_current_slideshow->get_entry(0),
						m_current_slideshow->get_entry(1)
					},
					rect = m_target_rectangle,
					worker_result = std::ref(m_worker_result),
					loaded_images  = std::ref(m_loaded_images)
				](){
					std::array<pixel_store::rgba_image, std::tuple_size_v<decltype(images_to_load)>> img_array;
					for(size_t k = 0; k != std::size(images_to_load); ++k)
					{
						if(images_to_load[k] != nullptr)
						{
							auto const& path_to_load = images_to_load[k]->path();
							fprintf(stderr, "(i) Loading %s\n", path_to_load.c_str());
							img_array[k]  = image_file_loader::load_rgba_image(path_to_load, rect);
						}
					}
					utils::unwrap(worker_result) = [
						img_array = std::move(img_array),
						loaded_images
					]() mutable {
						fprintf(stderr, "(i) First images loaded\n");
						auto& pending_images = utils::unwrap(loaded_images);
						pending_images = typename decltype(loaded_images)::type{
							std::move(img_array)
						};
					};
				}
			);
		}

		void handle_event(update_window const&)
		{
			auto action = m_worker_result.take_value();
			if(action)
			{ action(); }
		}

		bool application_should_exit() const
		{ return m_application_should_exit; }

	private:
		std::reference_wrapper<utils::task_queue> m_task_queue;

		slideshow* m_current_slideshow{nullptr};
		image_file_loader::image_rectangle m_target_rectangle;
		// TODO: This needs to be a threadsafe queue
		utils::synchronized<std::move_only_function<void()>> m_worker_result;
		utils::bidirectional_sliding_window<pixel_store::rgba_image, 1> m_loaded_images;
		bool m_application_should_exit{false};
	};
}

#endif