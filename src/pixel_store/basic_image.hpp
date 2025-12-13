#ifndef SLIDEPROJ_PIXEL_STORE_BASIC_IMAGE_HPP
#define SLIDEPROJ_PIXEL_STORE_BASIC_IMAGE_HPP

#include <memory>
#include <cstdint>
#include <cstddef>

namespace slideproj::pixel_store
{
	struct image_rectangle
	{
		uint32_t width;
		uint32_t height;
	};

	struct make_uninitialized_pixel_buffer_tag{};

	template<class T>
	class basic_image
	{
	public:
		basic_image() = default;

		explicit basic_image(
			uint32_t w,
			uint32_t h,
			make_uninitialized_pixel_buffer_tag
		):
			m_width{w},
			m_height{h},
			m_pixels{std::make_unique_for_overwrite<T[]>(static_cast<size_t>(w)*static_cast<size_t>(h))}
		{}

		auto width() const
		{ return m_width; }

		auto height() const
		{ return m_height; }

		auto pixel_count() const
		{ return static_cast<size_t>(width())*static_cast<size_t>(height()); }

		bool is_empty() const
		{ return m_width == 0 || m_height == 0 || m_pixels == nullptr; }

		auto pixels()
		{ return m_pixels.get(); }

		auto pixels() const
		{ return static_cast<T const*>(m_pixels.get()); }

		auto operator()(uint32_t x, uint32_t y) const
		{ return m_pixels.get()[x + y*m_width]; }

		auto& operator()(uint32_t x, uint32_t y)
		{ return m_pixels.get()[x + y*m_width]; }

	private:
		uint32_t m_width{0};
		uint32_t m_height{0};
		std::unique_ptr<T[]> m_pixels;
	};
}
#endif