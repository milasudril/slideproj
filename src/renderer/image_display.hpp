#ifndef SLIDEPROJ_RENDERER_IMAGE_DISPLAY_HPP
#define SLIDEPROJ_RENDERER_IMAGE_DISPLAY_HPP

#include "./gl_mesh.hpp"
#include "./gl_shader.hpp"
#include "./gl_texture.hpp"

#include "src/pixel_store/basic_image.hpp"
#include "src/pixel_store/rgba_image.hpp"

namespace slideproj::renderer
{
	struct image_to_display
	{
		gl_texture texture;
		float aspect_ratio = 1.0f;
	};

	class image_display
	{
	public:
		image_display()
		{
			update_scale();
			m_shader_program.set_uniform(2, 1.0f);
		}

		void show_image(pixel_store::rgba_image const& img)
		{
			std::swap(m_next_image, m_current_image);
			auto const w = img.width();
			auto const h = img.height();
			m_next_image.aspect_ratio = static_cast<float>(w)/static_cast<float>(h);
			update_scale();
			m_next_image.texture.upload(img);
		}

		void set_window_size(pixel_store::image_rectangle const& rect)
		{
			m_output_aspect_ratio = static_cast<float>(rect.width)/static_cast<float>(rect.height);
			update_scale();
		}

		void set_transition_param(float t)
		{
			m_shader_program.set_uniform(2, std::clamp(t, 0.0f, 1.0f));
		}

		void update()
		{
			m_shader_program.bind();
			m_mesh.bind();
			m_current_image.texture.bind(0);
			m_next_image.texture.bind(1);
			gl_bindings::draw_triangles();
		}

		void update_scale()
		{
			auto current_scale_x = 1.0f;
			auto current_scale_y = 1.0f;
			auto next_scale_x = 1.0f;
			auto next_scale_y = 1.0f;

			auto const current_input_aspect_ratio = m_current_image.aspect_ratio;
			auto const next_input_aspect_ratio = m_next_image.aspect_ratio;

			if(m_output_aspect_ratio >= 1.0f)
			{
				auto const output_scale_x = 1.0f/m_output_aspect_ratio;
				auto const output_scale_y = 1.0f;
				auto const current_input_scale_x = std::min(current_input_aspect_ratio, m_output_aspect_ratio);
				auto const current_input_scale_y = current_input_scale_x/current_input_aspect_ratio;
				auto const next_input_scale_x = std::min(next_input_aspect_ratio, m_output_aspect_ratio);
				auto const next_input_scale_y = next_input_scale_x/next_input_aspect_ratio;
				current_scale_x = current_input_scale_x*output_scale_x;
				current_scale_y = current_input_scale_y*output_scale_y;
				next_scale_x = next_input_scale_x*output_scale_x;
				next_scale_y = next_input_scale_y*output_scale_y;
			}
			else
			{
				auto const output_scale_x = 1.0f;
				auto const output_scale_y = m_output_aspect_ratio;
				auto const current_input_scale_y = std::min(1.0f/current_input_aspect_ratio, 1.0f/m_output_aspect_ratio);
				auto const current_input_scale_x = current_input_scale_y*next_input_aspect_ratio;
				auto const next_input_scale_y = std::min(1.0f/next_input_aspect_ratio, 1.0f/m_output_aspect_ratio);
				auto const next_input_scale_x = next_input_scale_y*next_input_aspect_ratio;
				current_scale_x = current_input_scale_x*output_scale_x;
				current_scale_y = current_input_scale_y*output_scale_y;
				next_scale_x = next_input_scale_x*output_scale_x;
				next_scale_y = next_input_scale_y*output_scale_y;
			}

			m_shader_program.set_uniform(0, current_scale_x, current_scale_y, 1.0f, 0.0f);
			m_shader_program.set_uniform(1, next_scale_x, next_scale_y, 1.0f, 0.0f);
		}

	private:
		float m_output_aspect_ratio = 1.0f;

		image_to_display m_current_image;
		image_to_display m_next_image;

		gl_mesh<unsigned int> m_mesh{
			std::array<unsigned int, 6>{
				0, 1, 2, 0, 2, 3
			}
		};

		gl_program m_shader_program{
			gl_shader<GL_VERTEX_SHADER>{R"(#version 460 core
layout (location = 0) uniform vec4 current_scale;
layout (location = 1) uniform vec4 next_scale;
layout (location = 2) uniform float t;

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
	vec4 scale = mix(current_scale, next_scale, t);
	gl_Position = scale*(coords[gl_VertexID] - origin) + origin;
	tex_coord = uv_coords[gl_VertexID];
}
)"},
			gl_shader<GL_FRAGMENT_SHADER>{R"(#version 460 core
out vec4 fragment_color;
in vec2 tex_coord;
layout (location = 2) uniform float t;

layout (binding = 0) uniform sampler2D current_image;
layout (binding = 1) uniform sampler2D next_image;

void main()
{
	fragment_color = mix(texture(current_image, tex_coord), texture(next_image, tex_coord), t);
}
)"
			}
		};
	};
}

#endif