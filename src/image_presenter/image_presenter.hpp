//@	{"dependencies_extra":[{"ref":"glfw3", "rel":"implementation", "origin":"pkg-config"}]}

#ifndef SLIDEPROJ_IMAGE_PRESENTER_IMAGE_PRESENTER_HPP
#define SLIDEPROJ_IMAGE_PRESENTER_IMAGE_PRESENTER_HPP

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include <memory>

namespace slideproj::image_presenter
{
	class glfw_context
	{
	public:
		glfw_context():m_impl{std::make_shared<impl>()}
		{}

	private:
		struct impl
		{
			impl()
			{
				if(glfwInit() == GLFW_FALSE)
				{
					char const* errmsg = nullptr;
					glfwGetError(&errmsg);
					throw std::runtime_error{std::format("Failed to initialize GLFW: {}", errmsg)};
				}
			}

			~impl()
			{ glfwTerminate(); }
		};

		std::shared_ptr<impl> m_impl;
	};
}

#endif