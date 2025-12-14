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
		image_display()
		{
			update_scale();
		}

		void show_image(pixel_store::rgba_image const& img)
		{
			auto const w = img.width();
			auto const h = img.height();
			fprintf(stderr, "(i) image_display %p: Showing image of size %u x %u\n", this, w, h);
			m_input_aspect_ratio = static_cast<float>(w)/static_cast<float>(h);
			update_scale();
		}

		void set_window_size(pixel_store::image_rectangle const& rect)
		{
			fprintf(stderr, "(i) image_display %p: Target rectangle updated\n", this);
			m_output_aspect_ratio = static_cast<float>(rect.width)/static_cast<float>(rect.height);
			update_scale();
		}

		void update()
		{
			m_shader_program.bind();
			m_mesh.bind();
			gl_bindings::draw_triangles();
		}

		void update_scale()
		{
			fprintf(
				stderr,
				"(i) image_display update_scale: input_aspect_ratio = %.8g, m_output_aspect_ratio = %.8g\n",
				m_input_aspect_ratio,
				m_output_aspect_ratio
			);

			auto scale_x = 1.0f;
			auto scale_y = 1.0f;

			if(m_output_aspect_ratio >= 1.0f)
			{
				auto const output_scale_x = 1.0f/m_output_aspect_ratio;
				auto const output_scale_y = 1.0f;
				auto const input_scale_x = std::min(m_input_aspect_ratio, m_output_aspect_ratio);
				auto const input_scale_y = input_scale_x/m_input_aspect_ratio;
				scale_x = input_scale_x*output_scale_x;
				scale_y = input_scale_y*output_scale_y;
			}
			else
			{
				auto const output_scale_x = 1.0f;
				auto const output_scale_y = m_output_aspect_ratio;
				auto const input_scale_y = std::min(1.0f/m_input_aspect_ratio, 1.0f/m_output_aspect_ratio);
				auto const input_scale_x = input_scale_y*m_input_aspect_ratio;
				scale_x = input_scale_x*output_scale_x;
				scale_y = input_scale_y*output_scale_y;
			}

			m_shader_program.set_uniform(0, scale_x, scale_y, 1.0f, 0.0f);
		}

	private:
		float m_output_aspect_ratio = 1.0f;
		float m_input_aspect_ratio = 1.0f;

		pixel_store::image_rectangle m_current_rect;
		gl_mesh<unsigned int> m_mesh{
			std::array<unsigned int, 6>{
				0, 1, 2, 0, 2, 3
			}
		};

		gl_program m_shader_program{
			gl_shader<GL_VERTEX_SHADER>{R"(#version 460 core
layout (location = 0) uniform vec4 scale;

const vec4 coords[4] = vec4[4](
	vec4(-1.0f, -1.0f, 0.0, 1.0f),
	vec4(1.0f, -1.0f, 0.0, 1.0f),
	vec4(1.0f, 1.0f, 0.0, 1.0f),
	vec4(-1.0f, 1.0f, 0.0, 1.0f)
);

const vec4 origin = vec4(0.0f, 0.0f, 0.0f, 1.0f);

const vec2 uv_coords[4] = vec2[4](
	vec2(0.0f, 1.0f),
	vec2(1.0f, 1.0f),
	vec2(1.0f, 0.0f),
	vec2(0.0f, 0.0f)
);

out vec2 tex_coord;

void main()
{
	gl_Position = scale*(coords[gl_VertexID] - origin) + origin;
	tex_coord = uv_coords[gl_VertexID];
}
)"},
			gl_shader<GL_FRAGMENT_SHADER>{R"(#version 460 core
out vec4 fragment_color;
in vec2 tex_coord;

void main()
{
	fragment_color = vec4(tex_coord.x, tex_coord.y, 0.5f, 1.0f);
}
)"
			}
		};
	};
}

#endif