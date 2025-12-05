//@	{
//@		"dependencies_extra":[
//@			{"ref":"glfw3", "rel":"implementation", "origin":"pkg-config"},
//@			{"ref":"glew", "rel":"implementation", "origin":"pkg-config"}
//@		]
//@	}

#ifndef SLIDEPROJ_IMAGE_PRESENTER_IMAGE_PRESENTER_HPP
#define SLIDEPROJ_IMAGE_PRESENTER_IMAGE_PRESENTER_HPP

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include <memory>
#include <format>

namespace slideproj::image_presenter
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

	struct renderer_version
	{
		uint32_t major;
		uint32_t minor;
	};

	class glfw_context
	{
	public:
		glfw_context():m_impl{std::make_shared<impl>()}
		{}

		static GLFWvidmode const& get_primary_monitor_video_mode()
		{
			auto const monitor = glfwGetPrimaryMonitor();
			if(monitor == nullptr)
			{ throw glfw_exception{"Failed to retrieve primary monitor: {}"}; }

			auto const vidmode = glfwGetVideoMode(monitor);
			if(vidmode == nullptr)
			{ throw glfw_exception{"Failed to retrieve video mode of primary monitor: {}"}; }

			return *vidmode;
		}

		void set_hits_for_current_video_mode()
		{
			auto const& vidmode = get_primary_monitor_video_mode();
			glfwWindowHint(GLFW_RED_BITS, vidmode.redBits);
			glfwWindowHint(GLFW_GREEN_BITS, vidmode.greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, vidmode.blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, vidmode.refreshRate);
		}

		void poll_events()
		{ glfwPollEvents(); }

		void select_opengl_version(renderer_version ver)
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, ver.major);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, ver.minor);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		}

	private:
		struct impl
		{
			impl()
			{
				if(glfwInit() == GLFW_FALSE)
				{
					char const* errmsg = nullptr;
					glfwGetError(&errmsg);
					throw glfw_exception{"Failed to initialize GLFW: {}"};
				}
			}

			~impl()
			{ glfwTerminate(); }
		};

		std::shared_ptr<impl> m_impl;
	};

	class gl_context
	{
	public:
		void enable_vsync()
		{
			glfwSwapInterval(1);
			m_use_vsync = true;
		}

		void disable_vsync()
		{
			glfwSwapInterval(0);
			m_use_vsync = false;
		}

	private:
		friend class application_window;

		gl_context()
		{
			auto const res = glewInit();
			if(res != GLEW_OK)
			{ throw glew_exception{"Failed to load OpenGL functions: {}", res}; }

			disable_vsync();
		}

		bool m_use_vsync{false};
	};

	class application_window
	{
	public:
		explicit application_window(glfw_context ctxt)
		{
			ctxt.set_hits_for_current_video_mode();
			m_handle = handle{glfwCreateWindow(800, 500, "slideproj", nullptr, nullptr)};
			if(m_handle == nullptr)
			{ throw glfw_exception{"Failed to create a window: {}"}; }
		}

		void swap_buffers()
		{ glfwSwapBuffers(m_handle.get()); }

		gl_context& activate_render_context()
		{
			glfwMakeContextCurrent(m_handle.get());
			if(!m_gl_ctxt.has_value())
			{ m_gl_ctxt = gl_context{}; }
			return *m_gl_ctxt;
		}

		template<class EventHandler>
		void set_event_handler(std::reference_wrapper<EventHandler> eh)
		{
			glfwSetWindowUserPointer(m_handle.get(), &eh);
			glfwSetFramebufferSizeCallback(
				m_handle.get(),
				[](GLFWwindow* window, int width, int height) {
					auto eh = static_cast<EventHandler*>(glfwGetWindowUserPointer(window));
					eh->frame_buffer_size_changed(width, height);
				}
			);

			// Synthesize a frame_buffer_size_changed event to make sure the size is up-to-date
			{
				int width;
				int height;
				glfwGetFramebufferSize(m_handle.get(), &width, &height);
				eh.get().frame_buffer_size_changed(width, height);
			}
		}

	private:
		struct deleter
		{
			void operator()(GLFWwindow* window) const
			{ glfwDestroyWindow(window); }
		};
		using handle = std::unique_ptr<GLFWwindow, deleter>;
		handle m_handle;
		std::optional<gl_context> m_gl_ctxt;
	};
}

#endif