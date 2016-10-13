#ifndef GPU_PROCESSING_H
#define GPU_PROCESSING_H

#include "PVRNativeApi/OGLES/OpenGLESBindings.h"

class GpuProcessing
{
public:

	GpuProcessing(GLuint nDefaultShaderProgram = 0);

	void SetDefaultShaderProgram(GLuint nShaderProgram);
	void SetDefaultRenderBuffer(GLuint nShaderProgram);
	void SetDefaultFrameBuffer(GLuint nShaderProgram);
	void SetDefaultTexture(GLuint nTex);

	bool LoadShaders(const char* ptVertexShaderFilename,
		const char* ptFragmentShaderFilename,
		GLuint& rnShaderProgram);

	bool BeginProcessing(GLuint nShaderProgram, 
		int nTexWidth, int nTexHeight, GLenum eTexFormat);

	bool EndProcessing();

private:
	
	GLuint m_nDefaultShaderProgram;
	GLuint m_nCurrentShaderProgram;
	GLuint m_nCurrentFbo;
	GLuint m_nCurrentRbo;
	GLuint m_nCurrentFboTex;


	bool InitFbo(GLuint& rnFbo, GLuint& rnRbo, GLuint nFboTex);
	void SetFbo(int nWidth, int nHeight, GLuint nFbo);
	void UnsetFbo();

	bool InitTexture(int nWidth, int nHeight,
		GLenum eTexFormat, GLuint& rnFboTex);
};

#endif /* GPU_PROCESSING_H */
 


