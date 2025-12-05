#ifndef SLIDEPROJ_EVENT_TYPES_WINDOWING_EVENTS_HPP
#define SLIDEPROJ_EVENT_TYPES_WINDOWING_EVENTS_HPP

namespace slideproj::event_types
{
	struct frame_buffer_size_changed_event
	{
		int width;
		int height;
	};

	struct window_is_closing_event
	{};
}

#endif