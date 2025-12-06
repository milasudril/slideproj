#ifndef SLIDEPROJ_PIXEL_STORE_RGBA_IMAGE_HPP
#define SLIDEPROJ_PIXEL_STORE_RGBA_IMAGE_HPP

#include "./basic_image.hpp"
#include "./pixel_types.hpp"

namespace slideproj::pixel_store
{
	using rgba_pixel = pixel_type<float, 4>;
	using rgba_image = basic_image<rgba_pixel>;
}

#endif