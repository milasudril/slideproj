#ifndef SLIDEPROJ_RENDERER_GL_VERTEX_ARRAY_HPP
#define SLIDEPROJ_RENDERER_GL_VERTEX_ARRAY_HPP

#include "./gl_resource.hpp"
#include "./gl_buffer.hpp"
#include "./gl_types.hpp"

#include <memory>

namespace slideproj::renderer
{
	struct gl_vertex_array_deleter
	{
		void operator()(GLuint handle) const
		{
			glDeleteVertexArrays(1, &handle);
		}
	};

	using gl_vertex_array_handle = gl_resource<gl_vertex_array_deleter>;

	class gl_bindings
	{
	public:
		static auto vertex_index_type() { return s_bound_index_type; }

		static auto get_index_count() { return s_bound_elem_count; }

		static void draw_triangles()
		{ glDrawElements(GL_TRIANGLES, s_bound_elem_count, s_bound_index_type, nullptr); }

		static void draw_triangles_repeatedly(GLsizei count)
		{ glDrawElementsInstanced(GL_TRIANGLES, s_bound_elem_count, s_bound_index_type, nullptr, count); }

	protected:
		thread_local static inline GLsizei s_bound_elem_count;
		thread_local static inline GLenum s_bound_index_type;
	};

	class gl_vertex_array:protected gl_bindings
	{
	public:
		explicit gl_vertex_array()
		{
			GLuint handle;
			glCreateVertexArrays(1, &handle);
			m_handle.reset(handle);
		}

		template<class T>
		void set_buffer(GLuint port, gl_vertex_buffer<T> const& buffer)
		{
			using value_type = typename T::value_type;

			glVertexArrayVertexBuffer(m_handle.get(), port, buffer.get(), 0, sizeof(T));
			glVertexArrayAttribFormat(m_handle.get(), port, 3, to_gl_type_id_v<value_type>, GL_FALSE, 0);
			glEnableVertexArrayAttrib(m_handle.get(), port);
		}

		template<class T>
		void set_buffer(gl_index_buffer<T> const& buffer)
		{
			glVertexArrayElementBuffer(m_handle.get(), buffer.get());
			m_elem_count = static_cast<GLsizei>(std::size(buffer));
			m_index_type = to_gl_type_id_v<T>;
		}

		void bind() const
		{
			glBindVertexArray(m_handle.get());
			s_bound_elem_count = m_elem_count;
			s_bound_index_type = m_index_type;
		}


	private:
		gl_vertex_array_handle m_handle;
		GLsizei m_elem_count;
		GLenum m_index_type;
	};
}

#endif
