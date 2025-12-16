#ifndef SLIDEPROJ_APP_SLIDESHOW_CONTROLLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_CONTROLLER_HPP

#include "./slideshow.hpp"

#include "src/file_collector/file_collector.hpp"
#include "src/pixel_store/basic_image.hpp"
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
		pixel_store::image_rectangle target_rectangle;
	};

	template<class T>
	concept image_display = requires(T& x, pixel_store::rgba_image const& img, float t)
	{
		{x.show_image(img)}->std::same_as<void>;
		{x.set_transition_param(t)}->std::same_as<void>;
	};

	struct type_erased_image_display
	{
		void* object;
		void (*show_image)(void*, pixel_store::rgba_image const&);
		void (*set_transition_param)(void*, float);
	};

	template<class T>
	concept title_display = requires(T& x, char const* str)
	{
		{x.set_title(str)} -> std::same_as<void>;
	};

	struct type_erased_title_display
	{
		void* object;
		void (*set_title)(void* object, char const*);
	};

	class slideshow_controller
	{
	public:
		using clock = std::chrono::steady_clock;

		template<
			image_display ImageDisplay,
			title_display TitleDisplay,
			file_collector::file_metadata_provider FileMetadataProvider
		>
		explicit slideshow_controller(
			utils::task_queue& task_queue,
			ImageDisplay& img_display,
			TitleDisplay& title_display,
			std::reference_wrapper<FileMetadataProvider const> file_metadata_provider,
			clock::duration transition_duration
		):
			m_task_queue{task_queue},
			m_image_display{
				.object = &img_display,
				.show_image = [](void* object, pixel_store::rgba_image const& img) {
					static_cast<ImageDisplay*>(object)->show_image(img);
				},
				.set_transition_param = [](void* object, float t) {
					static_cast<ImageDisplay*>(object)->set_transition_param(t);
				}
			},
			m_title_display{
				.object = &title_display,
				.set_title = [](void* object, char const* value) {
					static_cast<TitleDisplay*>(object)->set_title(value);
				}
			},
			m_file_metadata_provider{
				.object = &file_metadata_provider.get(),
				.get_metadata = [](void const* object, file_collector::file_list_entry const& item)
					->file_collector::file_metadata const& {
					return static_cast<FileMetadataProvider const*>(object)->get_metadata(item);
				}
			},
			m_transition_duration{transition_duration}
		{}

		void set_window_size(pixel_store::image_rectangle rect)
		{
			m_target_rectangle = rect;
			if(m_current_slideshow != nullptr)
			{ start_slideshow(*m_current_slideshow); }
		}

		void step_forward()
		{
			if(m_current_slideshow == nullptr)
			{ return; }

			m_current_slideshow->step(1);
			present_image(m_current_slideshow->get_entry(0));
			prefetch_image(1);
			prefetch_image(2);
			prefetch_image(3);
		}

		void step_backward()
		{
			if(m_current_slideshow == nullptr)
			{ return; }
			m_transition_start = std::chrono::steady_clock::now();

			m_current_slideshow->step(-1);
			present_image(m_current_slideshow->get_entry(0));
			prefetch_image(-1);
			prefetch_image(-2);
			prefetch_image(-3);
		}

		void start_slideshow(std::reference_wrapper<slideshow> slideshow)
		{
			fprintf(stderr, "(i) Slideshow loaded\n");
			utils::unwrap(m_task_queue).clear();
			m_loaded_images.clear();
			m_current_slideshow = &slideshow.get();
			m_present_immediately.clear();
			m_transition_start.reset();
			m_image_display.set_transition_param(m_image_display.object, 1.0f);
			present_image(m_current_slideshow->get_entry(0));
			prefetch_image(1);
			prefetch_image(2);
			prefetch_image(3);
			prefetch_image(-1);
			prefetch_image(-2);
			prefetch_image(-3);
		}

		void present_image(slideshow_entry const& entry)
		{
			if(!entry.is_valid())
			{ return; }

			auto& cached_entry = m_loaded_images[entry.index];
			if(
				cached_entry.has_value() &&
				cached_entry->source_file.id() == entry.source_file.id() &&
				cached_entry->target_rectangle == m_target_rectangle
			) [[likely]]
			{ present_image(*cached_entry); }
			else
			{
				auto ip = m_present_immediately.insert(std::pair{entry.source_file.id(), true});
				if(ip.second)
				{
					fprintf(stderr, "(i) Image %ld not loaded. Fetching first.\n", entry.index);
					fetch_image(entry);
				}
				else
				{
					fprintf(stderr, "(i) Waiting for %ld image to be loaded\n", entry.index);
					ip.first->second = true;
				}
			}
		}

		void prefetch_image(ssize_t offset)
		{
			auto entry = m_current_slideshow->get_entry(offset);
			if(!entry.is_valid())
			{ return; }

			auto& cached_entry = m_loaded_images[entry.index];
			if(cached_entry.has_value() && cached_entry->source_file.id() == entry.source_file.id())
			{ return; }

			if(m_present_immediately.insert(std::pair{entry.source_file.id(), false}).second)
			{ fetch_image(entry); }
		}

		void fetch_image(slideshow_entry const& entry)
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
						entry,
						saved_rect = m_target_rectangle,
						this
					](auto&& result) mutable {
						fprintf(stderr, "(i) Image %ld loaded\n", entry.index);
						if(saved_rect != m_target_rectangle)
						{
							fprintf(stderr, "(i) Image %ld loaded, but window size changed\n", entry.index);
							fetch_image(entry);
							return;
						}

						cached_entry = loaded_image{
							.index = entry.index,
							.source_file = std::move(entry.source_file),
							.image_data = std::move(result),
							.target_rectangle = saved_rect
						};

						auto i = m_present_immediately.find(cached_entry->source_file.id());
						if(i != std::end(m_present_immediately))
						{
							if(i->second)
							{ present_image(*cached_entry); }
							m_present_immediately.erase(i);
						}
					}
				}
			);
		}

		void present_image(loaded_image const& img)
		{
			fprintf(stderr, "(i) Showing image %ld\n", img.index);
			m_image_display.set_transition_param(m_image_display.object, 0.0f);
			m_image_display.show_image(m_image_display.object, img.image_data);
			m_transition_start = clock::now();
			auto const& caption = m_file_metadata_provider.get_metadata(
				m_file_metadata_provider.object, img.source_file
			).caption;

			m_title_display.set_title(m_title_display.object, caption.c_str());
		}

		void update_clock(clock::time_point now)
		{
			if(m_transition_start.has_value())
			{
				auto time_since_transition_start = now - *m_transition_start;
				if(time_since_transition_start >= m_transition_duration)
				{
					time_since_transition_start = m_transition_duration;
					m_transition_start.reset();
				}
				m_image_display.set_transition_param(
					m_image_display.object,
					time_since_transition_start/std::chrono::duration<float>(m_transition_duration)
				);
			}
		}

	private:
		std::reference_wrapper<utils::task_queue> m_task_queue;
		slideshow* m_current_slideshow{nullptr};
		pixel_store::image_rectangle m_target_rectangle{};
		utils::rotating_cache<loaded_image, utils::power_of_two{3}> m_loaded_images;
		type_erased_image_display m_image_display;
		type_erased_title_display m_title_display;
		file_collector::type_erased_file_metadata_provider m_file_metadata_provider;
		std::unordered_map<file_collector::file_id, bool> m_present_immediately;
		std::optional<clock::time_point> m_transition_start;
		clock::duration m_transition_duration;
	};
}

#endif