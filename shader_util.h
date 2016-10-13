#ifndef SHADER_UTIL_H
#define SHADER_UTIL_H

#include "PVRNativeApi/OGLES/OpenGLESBindings.h"

enum {
	ATTRIB_POSITION
};

bool loadShaders(const char* vertShaderFilename, const char* fragShaderFilename, GLuint* program);
void createShaderProgram(const GLchar* vertSrc, const GLchar* fragSrc,GLuint& shaderProgram);

#endif /* SHADER_UTIL_H */
