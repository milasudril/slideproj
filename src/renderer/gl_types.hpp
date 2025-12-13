//@	{
//@	 "dependencies":[
//@			{"ref": "glew", "origin":"pkg-config"}
//@		]
//@	}

#ifndef SLIDEPROJ_RENDERER_GL_TYPES_HPP
#define SLIDEPROJ_RENDERER_GL_TYPES_HPP

#include <GL/glew.h>
#include <GL/gl.h>

namespace slideproj::renderer
{
	template<class T>
	struct to_gl_type_id
	{
		static constexpr auto value = to_gl_type_id<typename T::value_type>::value;
	};

	template<>
	struct to_gl_type_id<float>
	{
		static constexpr auto value = GL_FLOAT;
	};

	template<>
	struct to_gl_type_id<unsigned int>
	{
		static constexpr auto value = GL_UNSIGNED_INT;
	};

	template<>
	struct to_gl_type_id<short>
	{
		static constexpr auto value = GL_SHORT;
	};

	template<>
	struct to_gl_type_id<unsigned short>
	{
		static constexpr auto value = GL_UNSIGNED_SHORT;
	};

	template<>
	struct to_gl_type_id<unsigned char>
	{
		static constexpr auto value = GL_UNSIGNED_BYTE;
	};

	template<>
	struct to_gl_type_id<signed char>
	{
		static constexpr auto value = GL_BYTE;
	};

	template<class T>
	constexpr auto to_gl_type_id_v = to_gl_type_id<T>::value;
}

#endif