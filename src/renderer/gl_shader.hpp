#ifndef SLIDEPROJ_RENDERER_GL_SHADER_HPP
#define SLIDEPROJ_RENDERER_GL_SHADER_HPP

#include "./gl_resource.hpp"

#include <memory>

namespace slideproj::renderer
{
	struct gl_shader_deleter
	{
		void operator()(GLuint handle) const
		{
			glDeleteShader(handle);
		}
	};

	using gl_shader_handle = gl_resource<gl_shader_deleter>;

	template<GLenum Type>
	class gl_shader
	{
	public:
		explicit gl_shader(char const* src):m_handle{glCreateShader(Type)}
		{
			glShaderSource(m_handle.get(), 1, &src, nullptr);
			glCompileShader(m_handle.get());
			GLint result{};
			glGetShaderiv(m_handle.get(), GL_COMPILE_STATUS, &result);
			if(result == GL_TRUE) [[likely]]
			{ return; }

			glGetShaderiv(m_handle.get(), GL_INFO_LOG_LENGTH, &result);
			auto const bufflength = result;
			auto msgbuff = std::make_unique<GLchar[]>(bufflength);
			GLsizei actual_length{};
			glGetShaderInfoLog(m_handle.get(), bufflength, &actual_length, msgbuff.get());
			throw std::runtime_error{reinterpret_cast<char const*>(msgbuff.get())};
		}

		auto get() const { return m_handle.get(); }

	private:
		gl_shader_handle m_handle;
	};

	struct gl_program_deleter
	{
		void operator()(GLuint handle) const
		{
			glDeleteProgram(handle);
		}
	};

	using gl_program_handle = gl_resource<gl_program_deleter>;

	class gl_program
	{
	public:
		explicit gl_program(gl_shader<GL_VERTEX_SHADER> const& vertex_shader,
			gl_shader<GL_FRAGMENT_SHADER> const& fragment_shader):
			m_handle{glCreateProgram()}
		{
			glAttachShader(m_handle.get(), vertex_shader.get());
			glAttachShader(m_handle.get(), fragment_shader.get());
			glLinkProgram(m_handle.get());
		}

		void bind() const
		{
			glUseProgram(m_handle.get());
		}

		gl_program& set_uniform(int index, float x, float y, float z, float w)
		{
			glProgramUniform4f(m_handle.get(), index, x, y, z, w);
			return *this;
		}

		gl_program& set_uniform(int index, float x, float y)
		{
			glProgramUniform2f(m_handle.get(), index, x, y);
			return *this;
		}

		gl_program& set_uniform(int index, float value)
		{
			glProgramUniform1f(m_handle.get(), index, value);
			return *this;
		}

		template<class T, size_t N>
		requires(std::tuple_size_v<T> == 4 && std::is_same_v<typename T::value_type, float>)
		gl_program& set_uniform(int index, std::array<T, N> const& vals)
		{
			glProgramUniform4fv(
				m_handle.get(),
				index,
				static_cast<GLsizei>(N),
				reinterpret_cast<float const*>(std::data(vals))
			);
			return *this;
		}

	private:
		gl_program_handle m_handle;
	};
}

#endif
