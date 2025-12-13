#ifndef SLIDEPROJ_RENDERER_GL_BUFFER_HPP
#define SLIDEPROJ_RENDERER_GL_BUFFER_HPP

#include "./gl_resource.hpp"

#include <memory>
#include <type_traits>
#include <array>
#include <span>

namespace slideproj::renderer
{
	struct gl_buffer_deleter
	{
		void operator()(GLuint handle) const
		{
			glDeleteBuffers(1, &handle);
		}
	};

	using gl_buffer_handle = gl_resource<gl_buffer_deleter>;

	template<class T>
	class gl_vertex_buffer
	{
	public:
		explicit gl_vertex_buffer():m_capacity{0}
		{
			GLuint buffer;
			glCreateBuffers(1, & buffer);
			m_buffer.reset(buffer);
		}

		template<size_t N>
		explicit gl_vertex_buffer(std::array<T, N> const& data):gl_vertex_buffer{std::span{data}}{}

		explicit gl_vertex_buffer(std::span<T const> data):gl_vertex_buffer()
		{
			upload(data);
		}

		void upload(std::span<T const> data)
		{
			auto const buffer_size = sizeof(T) * std::size(data);
			if(std::size(data) != m_capacity)
			{
				glNamedBufferStorage(m_buffer.get(), buffer_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
				m_capacity = std::size(data);
			}
			glNamedBufferSubData(m_buffer.get(), 0, buffer_size, std::data(data));
		}

		auto get() const { return m_buffer.get(); }

		auto const size() const { return m_capacity; }

	private:
		gl_buffer_handle m_buffer;
		size_t m_capacity;
	};

	template<std::unsigned_integral T>
	class gl_index_buffer
	{
	public:
		explicit gl_index_buffer():m_capacity{0}
		{
			GLuint buffer;
			glCreateBuffers(1, & buffer);
			m_buffer.reset(buffer);
		}

		template<size_t N>
		explicit gl_index_buffer(std::array<T, N> const& data):gl_index_buffer{std::span{data}}{}

		explicit gl_index_buffer(std::span<T const> data):gl_index_buffer()
		{
				upload(data);
		}

		void upload(std::span<T const> data)
		{
			auto const buffer_size = sizeof(T) * std::size(data);
			if(std::size(data) != m_capacity)
			{
				glNamedBufferStorage(m_buffer.get(), buffer_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
				m_capacity = std::size(data);
			}
			glNamedBufferSubData(m_buffer.get(), 0, buffer_size, std::data(data));
		}

		auto get() const { return m_buffer.get(); }

		auto const size() const { return m_capacity; }

	private:
		gl_buffer_handle m_buffer;
		size_t m_capacity;
	};
}

#endif
