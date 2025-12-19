//@	{"dependencies_extra":[{"ref":"./slideshow_presentation_controller.o", "rel":"implementation"}]}

#ifndef SLIDEPROJ_APP_SLIDESHOW_PRESENTATION_CONTROLLER_HPP
#define SLIDEPROJ_APP_SLIDESHOW_PRESENTATION_CONTROLLER_HPP

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

	template<class T>
	concept slideshow_event_handler = requires(
		T& obj,
		slideshow_navigator& navigator
	)
	{
		{obj.handle_event(navigator, slideshow_step_event{})} -> std::same_as<void>;
		{obj.handle_event(navigator, slideshow_transition_end_event{})} -> std::same_as<void>;
		{obj.handle_event(navigator, slideshow_time_event{})} -> std::same_as<void>;
	};

	struct type_erased_slideshow_event_handler
	{
		void* object;
		void (*handle_sse)(void*, slideshow_navigator&, slideshow_step_event);
		void (*handle_stee)(void*, slideshow_navigator&, slideshow_transition_end_event);
		void (*handle_ste)(void*, slideshow_navigator&, slideshow_time_event);
	};

	struct slideshow_presentation_descriptor
	{
		slideshow_clock::duration transition_duration;
		bool loop;
	};

	class slideshow_presentation_controller : public slideshow_navigator
	{
	public:
		using clock = slideshow_clock;

		template<
			image_display ImageDisplay,
			title_display TitleDisplay,
			file_collector::file_metadata_provider FileMetadataProvider,
			slideshow_event_handler EventHandler
		>
		explicit slideshow_presentation_controller(
			utils::task_queue& task_queue,
			ImageDisplay& img_display,
			TitleDisplay& title_display,
			std::reference_wrapper<FileMetadataProvider const> file_metadata_provider,
			EventHandler& event_handler,
			slideshow_presentation_descriptor const& params
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
			m_event_handler{
				.object = &event_handler,
				.handle_sse = [](void* object, slideshow_navigator& navigator, slideshow_step_event event) {
					static_cast<EventHandler*>(object)->handle_event(navigator, event);
				},
				.handle_stee = [](void* object, slideshow_navigator& navigator, slideshow_transition_end_event event) {
					static_cast<EventHandler*>(object)->handle_event(navigator, event);
				},
				.handle_ste = [](void* object, slideshow_navigator& navigator, slideshow_time_event event) {
					static_cast<EventHandler*>(object)->handle_event(navigator, event);
				}
			},
			m_params{params}
		{}

		void set_window_size(pixel_store::image_rectangle rect)
		{
			m_target_rectangle = rect;
			if(m_current_slideshow != nullptr)
			{ start_slideshow(*m_current_slideshow); }
		}

		void step_forward() override;

		void step_backward() override;

		void go_to_begin() override;

		void go_to_end() override;

		void start_slideshow(std::reference_wrapper<slideshow> slideshow);

		void present_image(slideshow_entry const& entry);

		void prefetch_image(ssize_t offset);

		void fetch_image(slideshow_entry const& entry);

		void present_image(loaded_image const& img);

		void update_clock(clock::time_point now);

	private:
		std::reference_wrapper<utils::task_queue> m_task_queue;
		slideshow* m_current_slideshow{nullptr};
		pixel_store::image_rectangle m_target_rectangle{};
		utils::rotating_cache<loaded_image, utils::power_of_two{3}> m_loaded_images;
		type_erased_image_display m_image_display;
		type_erased_title_display m_title_display;
		file_collector::type_erased_file_metadata_provider m_file_metadata_provider;
		type_erased_slideshow_event_handler m_event_handler;
		std::unordered_map<file_collector::file_id, bool> m_present_immediately;
		std::optional<clock::time_point> m_transition_start;

		slideshow_presentation_descriptor m_params;
	};
}

#endif