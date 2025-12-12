#ifndef SLIDEPROJ_APP_SLIDESHOW_CONTROLLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_CONTROLLER_HPP

#include "./slideshow.hpp"

#include "src/windowing_api/event_types.hpp"
#include "src/windowing_api/application_window.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/pixel_store/rgba_image.hpp"
#include "src/utils/bidirectional_sliding_window.hpp"
#include "src/utils/unwrap.hpp"
#include "src/utils/task_queue.hpp"
#include <thread>

namespace slideproj::app
{
	class slideshow_controller
	{
	public:
		explicit slideshow_controller(utils::task_queue& task_queue):
			m_task_queue{task_queue}
		{}

		void set_window_size(image_file_loader::image_rectangle rect)
		{ m_target_rectangle = rect; }

		void step_forward()
		{
			if(m_current_slideshow == nullptr)
			{ return; }

			auto& current_image = m_loaded_images.get_current_element();
			if(current_image.is_empty())
			{
				fprintf(stderr, "(i) Busy, will try again later (1)\n");
				m_pending_user_inputs.push([this](){
					fprintf(stderr, "(i) Invoking buffered step_forward\n");
					step_forward();

				});
				return;
			}

			auto const image_file_to_show = m_current_slideshow->step_and_get_entry(1);
			if(image_file_to_show != nullptr)
			{
				m_loaded_images.step_forward();
				prefetch_next_image();
				auto& image_to_show = m_loaded_images.get_current_element();
				if(image_to_show.is_empty())
				{
					fprintf(stderr, "(i) Busy, will show %s when ready\n", image_file_to_show->path().c_str());
					m_pending_user_inputs.push([this](){ present_image(m_loaded_images.get_current_element()); });
					return;
				}
				fprintf(stderr, "(i) Showing %s\n", image_file_to_show->path().c_str());
				present_image(image_to_show);
			}
		}

		void step_backward()
		{
			if(m_current_slideshow == nullptr)
			{ return; }

			auto const image_to_show = m_current_slideshow->step_and_get_entry(-1);
			if(image_to_show != nullptr)
			{ fprintf(stderr, "(i) Showing %s\n", image_to_show->path().c_str()); }
		}

		void prefetch_next_image()
		{
			auto img_to_replace = m_loaded_images.get_element_to_replace();
			if(img_to_replace->is_empty())
			{
				m_pending_user_inputs.push([this](){ prefetch_next_image(); });
			}

			*img_to_replace = pixel_store::rgba_image{};
			utils::unwrap(m_task_queue).submit(
				utils::task{
					.function = [
						image_to_load = m_current_slideshow->get_entry(1),
						rect = m_target_rectangle
					](){
						std::this_thread::sleep_for(std::chrono::seconds{5});
						if(image_to_load != nullptr)
						{ return image_file_loader::load_rgba_image(image_to_load->path(), rect); }
						else
						{ return pixel_store::rgba_image{}; }
					},
					.on_completed = [this, img_to_replace](auto&& result) {
						fprintf(stderr, "(i) Next image fetched\n");
						*img_to_replace = std::move(result);
						while(!m_pending_user_inputs.empty())
						{
							m_pending_user_inputs.front()();
							m_pending_user_inputs.pop();
						}
					}
				}
			);
		}

		void start_slideshow(std::reference_wrapper<slideshow> slideshow)
		{
			fprintf(stderr, "(i) Slideshow loaded\n");
			m_current_slideshow = &slideshow.get();
			unwrap(m_task_queue).submit(
				utils::task{
					.function = [
							images_to_load = std::array{
								m_current_slideshow->get_entry(-1),
								m_current_slideshow->get_entry(0),
								m_current_slideshow->get_entry(1)
							},
							rect = m_target_rectangle
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
						return std::remove_cvref_t<decltype(m_loaded_images)>{std::move(img_array)};
					},
					.on_completed = [this](auto&& img_array) mutable {
						fprintf(stderr, "(i) First images loaded\n");
						m_loaded_images = std::move(img_array);
						present_image(m_loaded_images.get_current_element());
					}
				}
			);
		}

		void present_image(pixel_store::rgba_image const& image)
		{
			fprintf(stderr, "(i) Showing image width = %u, height = %u\n", image.width(), image.height());
		}

	private:
		std::reference_wrapper<utils::task_queue> m_task_queue;
		slideshow* m_current_slideshow{nullptr};
		image_file_loader::image_rectangle m_target_rectangle{};
		utils::bidirectional_sliding_window<pixel_store::rgba_image, 1> m_loaded_images;
		std::queue<std::move_only_function<void()>> m_pending_user_inputs;
	};
}

#endif