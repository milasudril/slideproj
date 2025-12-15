//@	{
//@		"target":{"name":"glfw_wrapper.o"},
//@		"dependencies":[
//@			{"ref":"glfw3", "rel":"implementation", "origin":"pkg-config"},
//@			{"ref":"glew", "rel":"implementation", "origin":"pkg-config"}
//@		]
//@	}

#include "./glfw_wrapper.hpp"

#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <GL/gl.h>

namespace
{
	class glfw_exception:public std::runtime_error
	{
	public:
		static char const* get_error_message()
		{
			char const* errmsg = nullptr;
			glfwGetError(&errmsg);
			return errmsg != nullptr? errmsg : "No error";
		}

		template<class... Args>
		explicit glfw_exception(std::format_string<char const*> message):
			std::runtime_error{std::format(message, get_error_message())}
		{}
	};

	class glew_exception:public std::runtime_error
	{
	public:
		template<class... Args>
		explicit glew_exception(std::format_string<char const*> message, GLenum glew_errno):
			std::runtime_error{
				std::format(
					message,
					reinterpret_cast<char const*>(glewGetErrorString(glew_errno))
				)
			}
		{}
	};
}

slideproj::glfw_wrapper::glfw_window::glfw_window(char const* title)
{
	if(m_instance_counter.value() == 1)
	{
		if(glfwInit() == GLFW_FALSE)
		{
			char const* errmsg = nullptr;
			glfwGetError(&errmsg);
			throw glfw_exception{"Failed to initialize GLFW: {}"};
		}
	}

	auto const& vidmode = get_primary_monitor_video_mode();
	glfwWindowHint(GLFW_RED_BITS, vidmode.redBits);
	glfwWindowHint(GLFW_GREEN_BITS, vidmode.greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, vidmode.blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, vidmode.refreshRate);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	m_handle = handle{glfwCreateWindow(800, 500, title, nullptr, nullptr)};
	if(m_handle == nullptr)
	{ throw glfw_exception{"Failed to create a window: {}"}; }

	glfwMakeContextCurrent(m_handle.get());
	auto const err = glewInit();
	if (GLEW_OK != err)
	{ throw glew_exception{"Failed to initialize GLEW: {}", err}; }

	m_saved_window_rect = get_window_rect();
	enable_vsync();
	glfwSetWindowUserPointer(m_handle.get(), this);
}

GLFWvidmode const& slideproj::glfw_wrapper::glfw_window::get_primary_monitor_video_mode()
	{
		auto const monitor = glfwGetPrimaryMonitor();
		if(monitor == nullptr)
		{ throw glfw_exception{"Failed to retrieve primary monitor: {}"}; }

		auto const vidmode = glfwGetVideoMode(monitor);
		if(vidmode == nullptr)
		{ throw glfw_exception{"Failed to retrieve video mode of primary monitor: {}"}; }

		return *vidmode;
	}