#ifndef SLIDEPROJ_RENDERER_IMAGE_DISPLAY_HPP
#define SLIDEPROJ_RENDERER_IMAGE_DISPLAY_HPP

#include "./gl_mesh.hpp"

#include "src/pixel_store/rgba_image.hpp"

namespace slideproj::renderer
{
	class image_display
	{
	public:
		void show_image(pixel_store::rgba_image const&)
		{}

	private:
		gl_mesh<unsigned int> m_mesh{
			std::array<unsigned int, 6>{
				0, 1, 2, 0, 2, 3
			}
		};

	};
}

#endif