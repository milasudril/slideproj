#ifndef SLIDEPROJ_APP_SLIDESHOW_CONTROLLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_CONTROLLER_HPP

#include "./slideshow.hpp"

#include "src/file_collector/file_collector.hpp"
#include "src/utils/rotating_cache.hpp"
#include "src/image_file_loader/image_file_loader.hpp"
#include "src/pixel_store/rgba_image.hpp"
#include "src/utils/unwrap.hpp"
#include "src/utils/task_queue.hpp"

namespace slideproj::app
{
	struct loaded_image
	{
		ssize_t index;
		file_collector::file_list_entry source_file;
		pixel_store::rgba_image image_data;
	};

	template<class T>
	concept image_display = requires(T& x, pixel_store::rgba_image const& img)
	{
		{x.show_image(img)}->std::same_as<void>;
	};

	struct type_erased_image_display
	{
		void* object;
		void (*show_image)(void* object, pixel_store::rgba_image const& img);
	};

	class slideshow_controller
	{
	public:
		template<image_display ImageDisplay>
		explicit slideshow_controller(utils::task_queue& task_queue, ImageDisplay& img_display):
			m_task_queue{task_queue},
			m_image_display{
				.object = &img_display,
				.show_image = [](void* object, pixel_store::rgba_image const& img) {
					static_cast<ImageDisplay*>(object)->show_image(img);
				}
			}
		{}

		void set_window_size(pixel_store::image_rectangle rect)
		{ m_target_rectangle = rect; }

		void step_forward()
		{
			if(m_current_slideshow == nullptr)
			{ return; }

			m_current_slideshow->step(1);
			prefetch_image(1);
			present_image(m_current_slideshow->get_entry(0));
		}

		void step_backward()
		{
			if(m_current_slideshow == nullptr)
			{ return; }

			m_current_slideshow->step(-1);
			prefetch_image(-1);
			present_image(m_current_slideshow->get_entry(0));
		}

		void start_slideshow(std::reference_wrapper<slideshow> slideshow)
		{
			fprintf(stderr, "(i) Slideshow loaded\n");
			m_current_slideshow = &slideshow.get();
			present_image(m_current_slideshow->get_entry(0));
			prefetch_image(1);
			prefetch_image(-1);
		}

		void present_image(slideshow_entry const& entry)
		{
			if(!entry.is_valid())
			{ return; }

			auto& cached_entry = m_loaded_images[entry.index];
			if(cached_entry.has_value() && cached_entry->source_file.id() == entry.source_file.id()) [[likely]]
			{ present_image(*cached_entry); }
			else
			{
				fprintf(stderr, "(i) Image not loaded. Fetching first.\n");
				fetch_image(entry, true);
			}
		}

		void prefetch_image(ssize_t offset)
		{
			auto entry = m_current_slideshow->get_entry(offset);
			if(entry.is_valid())
			{ fetch_image(entry, false);  }
		}

		void fetch_image(slideshow_entry const& entry, bool present_result)
		{
			unwrap(m_task_queue).submit(
				utils::task{
					.function = [
						path_to_load = entry.source_file.path(),
						rect = m_target_rectangle
					](){
						return image_file_loader::load_rgba_image(path_to_load, rect);
					},
					.on_completed = [
						&cached_entry = m_loaded_images[entry.index],
						source_file = entry.source_file,
						index = entry.index,
						present_result,
						this
					](auto&& result) mutable {
						fprintf(stderr, "(i) Image %ld %s loaded\n", index, source_file.path().c_str());
						cached_entry = loaded_image{
							.index = index,
							.source_file = std::move(source_file),
							.image_data = std::move(result)
						};
						if(present_result)
						{ present_image(*cached_entry); }
					}
				}
			);
		}

		void present_image(loaded_image const& img)
		{
			fprintf(stderr, "(i) Showing image %ld %s\n", img.index, img.source_file.path().c_str());
			m_image_display.show_image(m_image_display.object, img.image_data);
		}

	private:
		std::reference_wrapper<utils::task_queue> m_task_queue;
		slideshow* m_current_slideshow{nullptr};
		pixel_store::image_rectangle m_target_rectangle{};
		utils::rotating_cache<loaded_image, utils::power_of_two{2}> m_loaded_images;
		type_erased_image_display m_image_display;
	};
}

#endif