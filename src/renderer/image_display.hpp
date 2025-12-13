#ifndef SLIDEPROJ_RENDERER_IMAGE_DISPLAY_HPP
#define SLIDEPROJ_RENDERER_IMAGE_DISPLAY_HPP

#include "src/pixel_store/rgba_image.hpp"

#include <GL/glew.h>
#include <GL/gl.h>

namespace slideproj::renderer
{
	class gl_resource
	{
	public:
		gl_resource() = default;

		explicit gl_resource(GLuint value, void (*deleter)(GLsizei, GLuint const*)):
			m_value{value},
			m_deleter{deleter}
		{}

		~gl_resource()
		{ reset(); }

		auto get_deleter() const
		{ return m_deleter; }

		gl_resource(gl_resource&& other) noexcept:
			m_value{other.release()},
			m_deleter{other.get_deleter()}
		{}

		gl_resource& operator=(gl_resource&& other) noexcept
		{
			reset();
			m_value = other.release();
			m_deleter = other.get_deleter();
			return *this;
		};

		void reset()
		{
			if(m_value != 0)
			{ m_deleter(1, &m_value); }
		}

		[[nodiscard]] GLuint release()
		{ return std::exchange(m_value, 0); }

		GLuint get() const
		{ return m_value; }

	private:
		GLuint m_value{0};
		void (*m_deleter)(GLsizei, GLuint const*);
	};

	template<auto Factory>
	auto create_resource()
	{
		GLuint ret{};
		(*Factory)(1, &ret);
		if(ret == 0)
		{ throw std::runtime_error{"Failed to create OpenGL resource"}; }
		return ret;
	}

	class image_display
	{
	public:
		image_display():
			m_vertex_array{create_resource<&glCreateVertexArrays>(), glDeleteVertexArrays}
		{}

		void show_image(pixel_store::rgba_image const&)
		{}

	private:
		gl_resource m_vertex_array;
	};
}

#endif