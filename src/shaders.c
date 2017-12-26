///////////////////////////////////////////////////////////////////////////////
/// @file:		shaders.c
/// @author:	Jacob Adkins (jpadkins)
/// @brief:		Struct containing sources for different shaders
///////////////////////////////////////////////////////////////////////////////

#include "shaders.h"

const struct _shaders shaders = {
	{
	"													\n\
	#version 330 core									\n\
														\n\
	layout (location = 0) in vec3 position;				\n\
	layout (location = 1) in vec2 texcoord;				\n\
														\n\
	out vec2 vtexcoord;									\n\
														\n\
	uniform mat4 transform;								\n\
														\n\
	void main(void) {									\n\
		gl_Position = transform * vec4(position, 1.0f); \n\
		vtexcoord = texcoord;							\n\
	}"
	},
	{
	"													\n\
	#version 330 core									\n\
														\n\
	in vec2 vtexcoord;									\n\
														\n\
	uniform sampler2D tex;								\n\
														\n\
	void main(void) {									\n\
		gl_FragColor = texture(tex, vtexcoord);			\n\
	}"
	}
};
