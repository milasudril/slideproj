//@	{
//@		"dependencies_extra":[
//@			{"ref":"glfw3", "rel":"implementation", "origin":"pkg-config"},
//@			{"ref":"glew", "rel":"implementation", "origin":"pkg-config"}
//@		]
//@	}

#ifndef SLIDEPROJ_IMAGE_PRESENTER_IMAGE_PRESENTER_HPP
#define SLIDEPROJ_IMAGE_PRESENTER_IMAGE_PRESENTER_HPP

#include "src/windowing_api/event_types.hpp"
#include "src/windowing_api/application_window.hpp"

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

		GLFWvidmode const& get_primary_monitor_video_mode()
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

		void restore_vsync() const
		{
			if(m_use_vsync)
			{ glfwSwapInterval(1); }
			else
			{ glfwSwapInterval(0); }
		}

	private:
		friend class glfw_window;

		gl_context()
		{
			auto const res = glewInit();
			if(res != GLEW_OK)
			{ throw glew_exception{"Failed to load OpenGL functions: {}", res}; }

			disable_vsync();
		}

		bool m_use_vsync{false};
	};

	constexpr auto to_typing_keyboard_scancode(int value)
	{
		constexpr auto X11_scancode_offest = 8;
		return windowing_api::typing_keyboard_scancode{value - X11_scancode_offest};
	}

	constexpr auto to_button_action(int value)
	{
		switch(value)
		{
			case GLFW_PRESS:
				return windowing_api::button_action::press;
			case GLFW_REPEAT:
				return windowing_api::button_action::repeat;
			case GLFW_RELEASE:
				return windowing_api::button_action::release;
			default:
				return windowing_api::button_action::release;
		}
	}

	constexpr auto to_typing_keyboard_modifier_mask(int value)
	{
		return static_cast<windowing_api::typing_keyboard_modifier_mask>(value);
	}

	struct window_rectangle
	{
		int x;
		int y;
		int width;
		int height;
	};

	class glfw_window:public windowing_api::application_window
	{
	public:
		explicit glfw_window(glfw_context ctxt):m_ctxt{ctxt}
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

		void toggle_fullscreen() override
		{
			if(glfwGetWindowMonitor(m_handle.get()) == nullptr)
			{
				m_saved_window_rect = get_window_rect();
				auto const& vidmode = m_ctxt.get_primary_monitor_video_mode();
				glfwSetWindowMonitor(
					m_handle.get(),
					glfwGetPrimaryMonitor(),
					0,
					0,
					vidmode.width,
					vidmode.height,
					vidmode.refreshRate
				);
			}
			else
			{
				glfwSetWindowMonitor(
					m_handle.get(),
					nullptr,
					m_saved_window_rect.x,
					m_saved_window_rect.y,
					m_saved_window_rect.width,
					m_saved_window_rect.height,
					GLFW_DONT_CARE
				);
			}

			if(m_gl_ctxt.has_value())
			{ m_gl_ctxt->restore_vsync(); }
		}

		template<class EventHandler>
		void set_event_handler(std::reference_wrapper<EventHandler> eh)
		{
			glfwSetWindowUserPointer(m_handle.get(), &eh.get());
			glfwSetFramebufferSizeCallback(
				m_handle.get(),
				[](GLFWwindow* window, int width, int height) {
					auto eh = static_cast<EventHandler*>(glfwGetWindowUserPointer(window));
					eh->handle_event(
						windowing_api::frame_buffer_size_changed_event{
							.width = width,
							.height = height
						}
					);
				}
			);

			glfwSetWindowCloseCallback(
				m_handle.get(),
				[](GLFWwindow* window) {
					auto eh = static_cast<EventHandler*>(glfwGetWindowUserPointer(window));
					eh->handle_event(windowing_api::window_is_closing_event{});
				}
			);

			glfwSetKeyCallback(
				m_handle.get(),
				[](GLFWwindow* window, int, int scancode, int action, int modifiers) {
					auto eh = static_cast<EventHandler*>(glfwGetWindowUserPointer(window));
					eh->handle_event(
						windowing_api::typing_keyboard_event{
							.scancode = to_typing_keyboard_scancode(scancode),
							.action = to_button_action(action),
							.modifiers = to_typing_keyboard_modifier_mask(modifiers)
						}
					);
				}
			);

			glfwSetMouseButtonCallback(
				m_handle.get(),
				[](GLFWwindow* window, int button, int action, int modifiers) {
					auto eh = static_cast<EventHandler*>(glfwGetWindowUserPointer(window));
					eh->handle_event(
						windowing_api::mouse_button_event{
							.button = windowing_api::mouse_button_index{button},
							.action = to_button_action(action),
							.modifiers = to_typing_keyboard_modifier_mask(modifiers)
						}
					);
				}
			);

			// Synthesize a frame_buffer_size_changed event to make sure the size is up-to-date
			{
				windowing_api::frame_buffer_size_changed_event event;
				glfwGetFramebufferSize(m_handle.get(), &event.width, &event.height);
				eh.get().handle_event(event);
			}
		}

		window_rectangle get_window_rect() const
		{
			window_rectangle ret{};
			glfwGetWindowPos(m_handle.get(), &ret.x, &ret.y);
			glfwGetWindowSize(m_handle.get(), &ret.width, &ret.height);
			return ret;
		}

		void set_window_rect(window_rectangle const& val)
		{
			glfwSetWindowSize(m_handle.get(), val.width, val.height);
			glfwSetWindowPos(m_handle.get(), val.x, val.y);
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
		glfw_context m_ctxt;
		window_rectangle m_saved_window_rect{0, 0, 800, 500};
	};
}

#endif