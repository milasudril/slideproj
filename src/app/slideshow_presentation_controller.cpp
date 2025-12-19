//@	{"target": {"name":"slideshow_presentation_controller.o"}}

#include "./slideshow_presentation_controller.hpp"
#include "src/pixel_store/basic_image.hpp"
#include "src/pixel_store/rgba_image.hpp"


namespace
{
	slideproj::pixel_store::rgba_image display_error()
	{
		static constexpr const char* message{
			"                                                                                                             "
			"  **       **                                                                                                "
			"  ***      **                                                                                                "
			"  ****     **                             **                                                                 "
			"  ** **    **                             **                                                                 "
			"  **  **   **                                                                                                "
			"  **   **  **      *******               ***      ******* *****       ******       ******  *      *******    "
			"  **    ** **    ***     ***              **      **    **    **           **    ***     ***    **      ***  "
			"  **     ****    **       **              **      **    **    **     ***** **    **       **    ***********  "
			"  **      ***    ***     ***              **      **    **    **    **    ***    ***    ****    **           "
			"  **       **      *******              ******    **    **    **     ****** *      ****** **      ********   "
			"                                                                                          **                 "
			"                                                                                          **                 "
			"                                                                                 ***     ***                 "
			"                                                                                   *******                   "
			"                                                                                                             "
		};

		constexpr uint32_t height = 16;
		constexpr auto width = static_cast<uint32_t>(strlen(message))/height;

		slideproj::pixel_store::rgba_image ret{
			width,
			height,
			slideproj::pixel_store::make_uninitialized_pixel_buffer_tag{}
		};
		auto const message_end = message + width*height;
		std::transform(message, message_end, ret.pixels(), [](auto item){
			return item == '*'?
				slideproj::pixel_store::rgba_pixel{1.0f, 1.0f, 1.0f, 1.0f}:
				slideproj::pixel_store::rgba_pixel{0.0f, 0.0f, 0.0f, 0.0f};
			}
		);
		return ret;
	}
}


void slideproj::app::slideshow_presentation_controller::step_forward()
{
	if(m_current_slideshow == nullptr)
	{ return; }

	m_event_handler.handle_sse(m_event_handler.object, *this, slideshow_step_event{
		.direction = step_direction::forward
	});

	auto const index_before = m_current_slideshow->step(1);
	if(m_params.loop && m_current_slideshow->get_current_index() == index_before)
	{ m_current_slideshow->go_to_begin(); }

	present_image(m_current_slideshow->get_entry(0));
	prefetch_image(1);
	prefetch_image(2);
	prefetch_image(3);
}

void slideproj::app::slideshow_presentation_controller::step_backward()
{
	if(m_current_slideshow == nullptr)
	{ return; }

	m_event_handler.handle_sse(m_event_handler.object, *this, slideshow_step_event{
		.direction = step_direction::backward
	});

	auto const index_before = m_current_slideshow->step(-1);

	if(m_params.loop && index_before == m_current_slideshow->get_current_index())
	{ m_current_slideshow->go_to_end(); }
	present_image(m_current_slideshow->get_entry(0));
	prefetch_image(-1);
	prefetch_image(-2);
	prefetch_image(-3);
}

void slideproj::app::slideshow_presentation_controller::go_to_begin()
{
	if(m_current_slideshow == nullptr)
	{ return; }

	m_event_handler.handle_sse(m_event_handler.object, *this, slideshow_step_event{
		.direction = step_direction::backward
	});

	m_current_slideshow->go_to_begin();
	present_image(m_current_slideshow->get_entry(0));
	prefetch_image(1);
	prefetch_image(2);
	prefetch_image(3);
}

void slideproj::app::slideshow_presentation_controller::go_to_end()
{
	if(m_current_slideshow == nullptr)
	{ return; }

	m_event_handler.handle_sse(m_event_handler.object, *this, slideshow_step_event{
		.direction = step_direction::forward
	});

	m_current_slideshow->go_to_end();
	present_image(m_current_slideshow->get_entry(0));
	prefetch_image(-1);
	prefetch_image(-2);
	prefetch_image(-3);
}

void slideproj::app::slideshow_presentation_controller::start_slideshow(std::reference_wrapper<slideshow> slideshow)
{
	fprintf(stderr, "(i) Slideshow loaded\n");
	utils::unwrap(m_task_queue).clear();
	m_loaded_images.clear();
	m_current_slideshow = &slideshow.get();
	m_present_immediately.clear();
	m_transition_start.reset();
	m_image_display.set_transition_param(m_image_display.object, 1.0f);

	m_event_handler.handle_sse(m_event_handler.object, *this, slideshow_step_event{
		.direction = step_direction::none
	});

	present_image(m_current_slideshow->get_entry(0));
	prefetch_image(1);
	prefetch_image(2);
	prefetch_image(3);
	prefetch_image(-1);
	prefetch_image(-2);
	prefetch_image(-3);
}

void slideproj::app::slideshow_presentation_controller::present_image(slideshow_entry const& entry)
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

void slideproj::app::slideshow_presentation_controller::prefetch_image(ssize_t offset)
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

void slideproj::app::slideshow_presentation_controller::fetch_image(slideshow_entry const& entry)
{
	unwrap(m_task_queue).submit(
		utils::task{
			.function = [
				path_to_load = entry.source_file.path(),
				rect = m_target_rectangle
			](){
				try
				{
					auto ret = image_file_loader::load_rgba_image(path_to_load, rect);
					if(ret.is_empty())
					{
						// TODO: Write a proper error message (Requires some basic text utility)
						return display_error();

					}
					return ret;
				}
				catch(...)
				{ return display_error(); }
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

void slideproj::app::slideshow_presentation_controller::present_image(loaded_image const& img)
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

void slideproj::app::slideshow_presentation_controller::update_clock(clock::time_point now)
{
	if(m_transition_start.has_value())
	{
		auto time_since_transition_start = now - *m_transition_start;
		if(time_since_transition_start >= m_params.transition_duration)
		{
			time_since_transition_start = m_params.transition_duration;
			m_transition_start.reset();
			m_event_handler.handle_stee(
				m_event_handler.object,
				*this,
				slideshow_transition_end_event{.when = now}
			);
		}
		m_image_display.set_transition_param(
			m_image_display.object,
			time_since_transition_start/std::chrono::duration<float>(m_params.transition_duration)
		);
	}
	m_event_handler.handle_ste(m_event_handler.object, *this, slideshow_time_event{.when = now});
}