//@	{
//@		"target":{"name":"image_presenter.o"},
//@		"dependencies":[
//@			{"ref":"glfw3", "rel":"implementation", "origin":"pkg-config"},
//@			{"ref":"glew", "rel":"implementation", "origin":"pkg-config"}
//@		]
//@	}

#include "./image_presenter.hpp"

namespace
{
	constinit size_t glfw_use_count = 0;
}


slideproj::image_presenter::glfw_window::glfw_window()
{
	if(glfw_use_count == 0)
	{
		if(glfwInit() == GLFW_FALSE)
		{
			char const* errmsg = nullptr;
			glfwGetError(&errmsg);
			throw glfw_exception{"Failed to initialize GLFW: {}"};
		}
	}
}