#ifndef SLIDEPROJ_APP_SLIDESHOW_CONTROLLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_CONTROLLER_HPP

#include "./slideshow.hpp"

#include "src/windowing_api/event_types.hpp"
#include "src/windowing_api/application_window.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/pixel_store/rgba_image.hpp"
#include "src/utils/bidirectional_sliding_window.hpp"
#include "src/utils/unwrap.hpp"
#include "src/utils/synchronized.hpp"
#include "src/utils/task_queue.hpp"

namespace slideproj::app
{
	class slideshow_controller
	{
	public:
		explicit slideshow_controller(utils::task_queue& task_queue):
			m_task_queue{task_queue}
		{}

		void step_forward()
		{
			auto const image_to_show = m_current_slideshow->step_and_get_entry(1);
			if(image_to_show != nullptr)
			{ fprintf(stderr, "(i) Showing %s\n", image_to_show->path().c_str()); }
		}

		void step_backward()
		{
			auto const image_to_show = m_current_slideshow->step_and_get_entry(-1);
			if(image_to_show != nullptr)
			{ fprintf(stderr, "(i) Showing %s\n", image_to_show->path().c_str()); }
		}
#if 0
		void start_slideshow(std::reference_wrapper<slideshow> slideshow)
		{
			fprintf(stderr, "(i) Slideshow loaded\n");
			m_current_slideshow = &slideshow.get();
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
#endif

	private:
		std::reference_wrapper<utils::task_queue> m_task_queue;
		slideshow* m_current_slideshow{nullptr};
		image_file_loader::image_rectangle m_target_rectangle{};
		// TODO: This needs to be a threadsafe queue
		utils::synchronized<std::move_only_function<void()>> m_worker_result;
		utils::bidirectional_sliding_window<pixel_store::rgba_image, 1> m_loaded_images;
		bool m_application_should_exit{false};
	};
}

#endif