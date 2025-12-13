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

		void set_window_size(pixel_store::image_rectangle const&)
		{
			fprintf(stderr, "(i) image_display %p: Target rectangle updated\n", this);
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
const vec4 coords[4] = vec4[4](
	vec4(-0.5f, -0.5, 0.0, 1.0f),
	vec4(0.5, -0.5, 0.0, 1.0f),
	vec4(0.5, 0.5, 0.0, 1.0f),
	vec4(-0.5,0.5, 0.0, 1.0f)
);

void main()
{
	gl_Position = coords[gl_VertexID];
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