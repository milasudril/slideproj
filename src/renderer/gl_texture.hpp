#ifndef SLIDEPROJ_RENDERER_GL_TEXTURE_HPP
#define SLIDEPROJ_RENDERER_GL_TEXTURE_HPP

#include "./gl_resource.hpp"
#include "./gl_types.hpp"

#include "src/pixel_store/rgba_image.hpp"

#include <bit>
#include <cassert>

namespace slideproj::renderer
{
	struct gl_texture_deleter
	{
		void operator()(GLuint handle) const
		{ glDeleteTextures(1, &handle); }
	};

	using gl_texture_handle = gl_resource<gl_texture_deleter>;

	struct gl_texture_descriptor
	{
		GLsizei width;
		GLsizei height;
		GLenum format;
		GLenum type;
		GLsizei num_mipmaps;

		auto operator<=>(gl_texture_descriptor const& other) const = default;
	};

	inline float aspect_ratio(gl_texture_descriptor const& descriptor)
	{
		return static_cast<float>(descriptor.width)/static_cast<float>(descriptor.height);
	}

	template<class T>
	struct to_gl_color_channel_layout;

	template<>
	struct to_gl_color_channel_layout<pixel_store::rgba_pixel>
	{
		static constexpr auto value = GL_RGBA;
	};

	template<class T>
	constexpr auto to_gl_color_channel_layout_v = to_gl_color_channel_layout<T>::value;

	inline GLenum gl_make_sized_format_red(GLenum type)
	{
		switch(type)
		{
			case GL_FLOAT:
				return GL_R32F;
			default:
				throw std::runtime_error{"Unimplemented type"};
		}
	}

	inline GLenum gl_make_sized_format_rg(GLenum type)
	{
		switch(type)
		{
			case GL_FLOAT:
				return GL_RG32F;
			default:
				throw std::runtime_error{"Unimplemented type"};
		}
	}

	inline GLenum gl_make_sized_format_rgb(GLenum type)
	{
		switch(type)
		{
			case GL_FLOAT:
				return GL_RGB32F;
			default:
				throw std::runtime_error{"Unimplemented type"};
		}
	}

	inline GLenum gl_make_sized_format_rgba(GLenum type)
	{
		switch(type)
		{
			case GL_FLOAT:
				return GL_RGBA32F;
			default:
				throw std::runtime_error{"Unimplemented type"};
		}
	}

	inline GLenum gl_make_sized_format(GLenum format, GLenum type)
	{
		switch(format)
		{
			case GL_RED:
				return gl_make_sized_format_red(type);
			case GL_RG:
				return gl_make_sized_format_rg(type);
			case GL_RGB:
				return gl_make_sized_format_rgb(type);
			case GL_RGBA:
				return gl_make_sized_format_rgba(type);
			default:
				throw std::runtime_error{"Unimplemented format"};
		}
	}

	inline size_t gl_get_sample_size(GLenum type)
	{
		switch(type)
		{
			case GL_FLOAT:
				return 4;
			default:
				throw std::runtime_error{"Unimplemented type"};
		}
	}

	inline size_t gl_get_pixel_size(GLenum format, GLenum type)
	{
		switch(format)
		{
			case GL_RED:
				return 1*gl_get_sample_size(type);
			case GL_RG:
				return 2*gl_get_sample_size(type);
			case GL_RGB:
				return 3*gl_get_sample_size(type);
			case GL_RGBA:
				return 4*gl_get_sample_size(type);
			default:
				throw std::runtime_error{"Unimplemented format"};
		}
	}

	inline auto get_image_size(gl_texture_descriptor const& descriptor)
	{
		return descriptor.width*descriptor.height*gl_get_pixel_size(descriptor.format, descriptor.type);
	}

	class gl_texture
	{
	public:
		explicit gl_texture():m_descriptor{0, 0, 0, 0, 0}
		{ }

		template<class T>
		explicit gl_texture(pixel_store::basic_image<T> const& pixels):gl_texture{}
		{ upload(pixels); }

		explicit gl_texture(gl_texture_descriptor const& descriptor):gl_texture{}
		{ set_format(descriptor); }

		auto& upload(std::span<std::byte const> data, gl_texture_descriptor const& descriptor)
		{
			if(descriptor != m_descriptor) [[unlikely]]
 			{ set_format(descriptor); }

			upload_impl(data);

			return *this;
		}

		template<class T>
		auto& upload(pixel_store::basic_image<T> const& pixels)
		{
			gl_texture_descriptor const descriptor{
				static_cast<GLsizei>(pixels.width()),
				static_cast<GLsizei>(pixels.height()),
				to_gl_color_channel_layout<T>::value,
				to_gl_type_id_v<T>,
				std::max(static_cast<GLsizei>(std::bit_width(std::max(pixels.width(), pixels.height()) - 1)), 1)
			};

			std::span const pixel_array{
				pixels.pixels(), static_cast<size_t>(pixels.width())*static_cast<size_t>(pixels.height())
			};

			return upload(std::as_bytes(pixel_array), descriptor);
		}

		auto& upload(std::span<std::byte const> data)
		{
			auto const image_size = get_image_size(m_descriptor);
			if(image_size != std::size(data)) [[unlikely]]
			{
				fprintf(stderr, "(!) Ignoring texture data of wrong size\n");
				return *this;
			}
			upload_impl(data);
			return *this;
		}

		void set_format(gl_texture_descriptor const& descriptor)
		{
			GLuint handle;
			glCreateTextures(GL_TEXTURE_2D, 1, &handle);
			assert(descriptor.width != 0);
			assert(descriptor.height != 0);
			assert(descriptor.num_mipmaps != 0);

			glTextureStorage2D(handle,
				descriptor.num_mipmaps,
				gl_make_sized_format(descriptor.format, descriptor.type),
				descriptor.width,
				descriptor.height);

			glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, descriptor.num_mipmaps > 1 ?
				GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			m_handle.reset(handle);
			m_descriptor = descriptor;
		}

		void bind(GLuint texture_unit) const
		{ glBindTextureUnit(texture_unit, m_handle.get()); }

		auto const& descriptor() const
		{ return m_descriptor; }

		auto handle() const
		{ return static_cast<uint32_t>(m_handle.get()); }

	private:
		void upload_impl(std::span<std::byte const> data)
		{
			glTextureSubImage2D(m_handle.get(),
				0,  // mipmap level
				0,  // x-offset
				0,  // y-offset
				m_descriptor.width,
				m_descriptor.height,
				m_descriptor.format,
				m_descriptor.type,
				std::data(data));

			if(m_descriptor.num_mipmaps != 0) [[unlikely]]
			{ glGenerateTextureMipmap(m_handle.get()); }
		}

		gl_texture_handle m_handle;
		gl_texture_descriptor m_descriptor;
	};
}

#endif
