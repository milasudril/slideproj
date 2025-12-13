//@	{
//@	 "dependencies":[
//@			{"ref": "glew", "origin":"pkg-config"}
//@		]
//@	}

#ifndef SLIDEPROJ_RENDERER_GL_RESOURCE_HPP
#define SLIDEPROJ_RENDERER_GL_RESOURCE_HPP

#include <GL/glew.h>
#include <GL/gl.h>

#include <memory>

namespace slideproj::renderer
{
	template<class Int>
	struct nullable
	{
		Int val=0;
		nullable()=default;
		nullable(Int v):val(v){}
		friend bool operator==(std::nullptr_t, nullable const& self) {return !static_cast<bool>(self);}
		friend bool operator!=(std::nullptr_t, nullable const& self) {return static_cast<bool>(self);}
		friend bool operator==(nullable const& self, std::nullptr_t) {return !static_cast<bool>(self);}
		friend bool operator!=(nullable const& self, std::nullptr_t) {return static_cast<bool>(self);}
		nullable& operator=(std::nullptr_t)
		{
			val = 0;
			return *this;
		}

		operator Int() const {return val;}
	};

	template<class Int, class Deleter>
	struct int_deleter
	{
		using pointer = nullable<Int>;

		void operator()(pointer p) const
		{ Deleter{}(p); }
	};

	template<class Int, class Deleter>
	using int_resource = std::unique_ptr<nullable<Int>, int_deleter<Int, Deleter>>;

	template<class Deleter>
	using gl_resource = int_resource<GLuint, Deleter>;
}

#endif
