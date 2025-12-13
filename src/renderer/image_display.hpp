#ifndef SLIDEPROJ_RENDERER_IMAGE_DISPLAY_HPP
#define SLIDEPROJ_RENDERER_IMAGE_DISPLAY_HPP

#include "./gl_mesh.hpp"
#include "./gl_shader.hpp"

#include "src/pixel_store/basic_image.hpp"
#include "src/pixel_store/rgba_image.hpp"

namespace slideproj::renderer
{
	class image_display
	{
	public:
		void show_image(pixel_store::rgba_image const&)
		{}

		void set_window_size(pixel_store::image_rectangle const& rect)
		{
			fprintf(stderr, "(i) image_display %p: Target rectangle updated\n", this);
			if(rect.height >= rect.width)
			{
				auto const aspect_ratio = static_cast<float>(rect.width)/static_cast<float>(rect.height);
				m_shader_program.set_uniform(0, 1.0f, aspect_ratio, 1.0f, 1.0f);
			}
			else
			{
				auto const aspect_ratio = static_cast<float>(rect.height)/static_cast<float>(rect.width);
				m_shader_program.set_uniform(0, aspect_ratio, 1.0f, 1.0f, 1.0f);
			}
		}

		void update()
		{
			m_shader_program.bind();
			m_mesh.bind();
			gl_bindings::draw_triangles();
		}

	private:
		gl_mesh<unsigned int> m_mesh{
			std::array<unsigned int, 6>{
				0, 1, 2, 0, 2, 3
			}
		};

		gl_program m_shader_program{
			gl_shader<GL_VERTEX_SHADER>{R"(#version 460 core
layout (location = 0) uniform vec4 world_scale;

const vec4 coords[4] = vec4[4](
	vec4(-1.0f, -1.0f, 0.0, 1.0f),
	vec4(1.0f, -1.0f, 0.0, 1.0f),
	vec4(1.0f, 1.0f, 0.0, 1.0f),
	vec4(-1.0f, 1.0f, 0.0, 1.0f)
);

const vec4 origin = vec4(0.0f, 0.0f, 0.0f, 1.0f);

void main()
{
	gl_Position = world_scale*(coords[gl_VertexID] - origin) + origin;
}
)"},
			gl_shader<GL_FRAGMENT_SHADER>{R"(#version 460 core
out vec4 fragment_color;

void main()
{
	fragment_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
)"
			}
		};
	};
}

#endif