#ifndef SLIDEPROJ_WINDOWING_API_APPLICATION_WINDOW_HPP
#define SLIDEPROJ_WINDOWING_API_APPLICATION_WINDOW_HPP

namespace slideproj::windowing_api
{
	enum class cursor_mode
	{
		normal,
		hidden,
		disabled
	};

	class application_window
	{
	public:
		virtual ~application_window() = default;
		virtual void enable_fullscreen() = 0;
		virtual void disable_fullscreen() = 0;
		virtual bool fullscreen_is_enabled() const = 0;
		virtual void set_cursor_mode(cursor_mode mode) = 0;
		virtual cursor_mode get_cursor_mode() const = 0;
	};
}

#endif