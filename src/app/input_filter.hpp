#ifndef SLIDEPROJ_APP_INPUT_FILTER_HPP
#define SLIDEPROJ_APP_INPUT_FILTER_HPP

#include "src/utils/unwrap.hpp"
#include "src/utils/glob_string.hpp"

#include <filesystem>
#include <vector>
#include <algorithm>
#include <utility>

namespace slideproj::app
{
	template<class ImageDimensionProvider>
	struct input_filter
	{
		bool accepts(std::filesystem::directory_entry const& entry) const
		{
			if(std::ranges::any_of(include, [path = entry.path().string()](auto const& pattern) {
				return pattern.matches(path);
			}))
			{
				auto const dimensions = std::as_const(utils::unwrap(image_dimension_provider))
					.get_dimensions(entry.path());
				auto const w = static_cast<size_t>(dimensions.width);
				auto const h = static_cast<size_t>(dimensions.height);
				auto const img_pixel_count = w*h;
				return img_pixel_count > static_cast<size_t>(0) && img_pixel_count <= max_pixel_count;
			}
			return false;
		}

		std::vector<utils::glob_string> include;
		size_t max_pixel_count;
		[[no_unique_address]] ImageDimensionProvider image_dimension_provider;
	};
}

#endif