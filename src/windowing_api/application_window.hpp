#ifndef SLIDEPROJ_WINDOWING_API_APPLICATION_WINDOW_HPP
#define SLIDEPROJ_WINDOWING_API_APPLICATION_WINDOW_HPP

namespace slideproj::windowing_api
{
	class application_window
	{
	public:
		virtual ~application_window() = default;
		virtual void toggle_fullscreen() = 0;

	private:
	};
}

#endif