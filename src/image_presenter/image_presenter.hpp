//@	{
//@		"dependencies_extra":[
//@			{"ref":"./image_presenter.o", "rel":"implementation"},
//@			{"ref":"glfw3", "rel":"implementation", "origin":"pkg-config"}
//@		]
//@	}

#ifndef SLIDEPROJ_IMAGE_PRESENTER_IMAGE_PRESENTER_HPP
#define SLIDEPROJ_IMAGE_PRESENTER_IMAGE_PRESENTER_HPP

#include "src/windowing_api/event_types.hpp"
#include "src/windowing_api/application_window.hpp"
#include "src/utils/instance_counter.hpp"

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <memory>
#include <format>

namespace slideproj::image_presenter
{
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
		static std::unique_ptr<glfw_window> create(char const* title)
		{
			return std::unique_ptr<glfw_window>{new glfw_window(title)};
		}

		~glfw_window() override
		{
			if(m_instance_counter.value() == 1)
			{ glfwTerminate(); }
		}

		GLFWvidmode const& get_primary_monitor_video_mode();

		void poll_events()
		{ glfwPollEvents(); }

		void swap_buffers()
		{ glfwSwapBuffers(m_handle.get()); }

		void enable_vsync()
		{
			glfwSwapInterval(1);
			m_vsync_enabled = true;
		}

		void disable_vsync()
		{
			glfwSwapInterval(0);
			m_vsync_enabled = false;
		}

		void toggle_fullscreen() override;

		template<class EventHandler>
		void set_event_handler(std::reference_wrapper<EventHandler> eh)
		{
			m_event_handler = &eh.get();
			static constexpr auto get_event_handler = [](GLFWwindow* window) {
				auto self = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
				return static_cast<EventHandler*>(self->m_event_handler);
			};

			glfwSetFramebufferSizeCallback(
				m_handle.get(),
				[](GLFWwindow* window, int width, int height) {
					auto eh = get_event_handler(window);
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
					auto eh = get_event_handler(window);
					eh->handle_event(windowing_api::window_is_closing_event{});
				}
			);

			glfwSetKeyCallback(
				m_handle.get(),
				[](GLFWwindow* window, int, int scancode, int action, int modifiers) {
					auto eh = get_event_handler(window);
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
					auto eh = get_event_handler(window);
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
		explicit glfw_window(char const* title);

		struct deleter
		{
			void operator()(GLFWwindow* window) const
			{ glfwDestroyWindow(window); }
		};
		using handle = std::unique_ptr<GLFWwindow, deleter>;

		[[no_unique_address]] utils::instance_counter<glfw_window> m_instance_counter;
		handle m_handle;
		void* m_event_handler;
		window_rectangle m_saved_window_rect{};
		bool m_vsync_enabled{false};
	};
}

#endif