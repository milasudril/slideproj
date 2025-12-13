#ifndef SLIDEPROJ_RENDERER_GL_MESH_HPP
#define SLIDEPROJ_RENDERER_GL_MESH_HPP

#include "./gl_buffer.hpp"
#include "./gl_vertex_array.hpp"

#include <tuple>

namespace slideproj::renderer
{
	template<class IndexType, class ... VertexAttributeTypes>
	class gl_mesh
	{
		private:
			struct dummy{};

		public:
			using vbo_tuple = std::conditional_t<
				sizeof...(VertexAttributeTypes) == 0,
				dummy,
				std::tuple<gl_vertex_buffer<VertexAttributeTypes>...>
			>;

			template<class IndexArray, class ... VertexAttribArrays>
			requires(sizeof...(VertexAttribArrays) == sizeof...(VertexAttributeTypes))
			explicit gl_mesh(
				IndexArray&& index_array,
				VertexAttribArrays&&... vertex_attribs
			):
				m_index_buffer{std::forward<IndexArray>(index_array)},
				m_vertex_attribs{gl_vertex_buffer{std::forward<VertexAttribArrays>(vertex_attribs)}...}
			{
				m_vao.set_buffer(m_index_buffer);
				if constexpr(sizeof...(VertexAttributeTypes) != 0)
				{
					apply([this](auto const&... buffers){
						GLuint current_index = 0;
						((m_vao.set_buffer(current_index++, buffers)), ...);
					},
					m_vertex_attribs);
				}
			}

			void bind() const
			{ m_vao.bind(); }

			size_t get_index_count() const
			{	return std::size(m_index_buffer.faces()); }

		private:
			gl_vertex_array m_vao;
			gl_index_buffer<IndexType> m_index_buffer;
			[[no_unique_address]] vbo_tuple m_vertex_attribs;
	};
}

#endif
