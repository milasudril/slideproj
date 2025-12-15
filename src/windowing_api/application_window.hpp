#ifndef SLIDEPROJ_WINDOWING_API_APPLICATION_WINDOW_HPP
#define SLIDEPROJ_WINDOWING_API_APPLICATION_WINDOW_HPP

namespace slideproj::windowing_api
{
	class application_window
	{
	public:
		virtual ~application_window() = default;
		virtual void enable_fullscreen() = 0;
		virtual void disable_fullscreen() = 0;
		virtual bool fullscreen_is_enabled() const = 0;

	private:
	};
}

#endif