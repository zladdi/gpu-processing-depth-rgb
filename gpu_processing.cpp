#include "gpu_processing.h"
#include <cstdio>
#include "shader_util.h"

GpuProcessing::GpuProcessing(GLuint nDefaultShaderProgram)
	: m_nDefaultShaderProgram(nDefaultShaderProgram),
	  m_nCurrentShaderProgram(0),
	  m_nCurrentFbo(0),
	  m_nCurrentFboTex(0)
{
}

void GpuProcessing::SetDefaultShaderProgram(GLuint nShaderProgram)
{
	m_nDefaultShaderProgram = nShaderProgram;
}


void GpuProcessing::SetDefaultFrameBuffer(GLuint nShaderProgram)
{
	m_nCurrentFbo = nShaderProgram;
}

void GpuProcessing::SetDefaultRenderBuffer(GLuint nShaderProgram)
{
	m_nCurrentRbo = nShaderProgram;
}

void GpuProcessing::SetDefaultTexture(GLuint nTex)
{
	m_nCurrentFboTex = nTex;
}

bool GpuProcessing::InitTexture(int nWidth, int nHeight, GLenum eTexFormat, GLuint& rnFboTex)
{
	// setup texture
	gl::Enable(GL_TEXTURE_2D);
	gl::ActiveTexture(GL_TEXTURE1);	// TODO: find out whether this is useful here at all
//	glGenTextures(1, &rnFboTex);
	rnFboTex = m_nCurrentFboTex;
	gl::BindTexture(GL_TEXTURE_2D, rnFboTex);
	gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// This is necessary for non-power-of-two textures
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// allocate texture buffer
	// NOTE: GL_LUMINANCE cannot be used because it will result in a GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
	//       when trying to attach it to a frame buffer, so we use GL_RGBA instead
	gl::TexImage2D(GL_TEXTURE_2D, 0, eTexFormat, nWidth, nHeight, 0, eTexFormat, GL_UNSIGNED_BYTE, NULL);
	
	// unbind again
	gl::BindTexture(GL_TEXTURE_2D, 0);
	return true;
}

bool GpuProcessing::InitFbo(GLuint& rnFbo, GLuint& rnRbo, GLuint nFboTex)
{
	gl::Enable(GL_TEXTURE_2D);	
	gl::ActiveTexture(GL_TEXTURE1);	// TODO: find out whether this is useful here at all
	
//	glGenFramebuffers(1, &rnFbo);
	rnFbo = m_nCurrentFbo;
	rnRbo = m_nCurrentRbo;

	gl::BindFramebuffer(GL_FRAMEBUFFER, rnFbo);
	gl::BindRenderbuffer(GL_RENDERBUFFER, rnRbo);

	gl::BindTexture(GL_TEXTURE_2D, nFboTex);
	gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nFboTex, 0);
	
        GLenum eStatus = gl::CheckFramebufferStatus(GL_FRAMEBUFFER);
	        
	if (eStatus != GL_FRAMEBUFFER_COMPLETE)
	{
	        printf("Incomplete FBO: %x\n", eStatus);
		return false;
	}

	// unbind texture & frame buffer
	gl::BindTexture(GL_TEXTURE_2D, 0);
	gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
	gl::BindRenderbuffer(GL_RENDERBUFFER, 0);
	return true;
}

void GpuProcessing::SetFbo(int nWidth, int nHeight, GLuint nFbo)
{
        gl::BindFramebuffer(GL_FRAMEBUFFER, nFbo);
              
	//set the viewport to be the size of the texture
	gl::Viewport(0, 0, nWidth, nHeight);
}

void GpuProcessing::UnsetFbo()
{	
	gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
}



bool GpuProcessing::LoadShaders(const char* ptVertexShaderFilename,
	const char* ptFragmentShaderFilename,
	GLuint& rnShaderProgram)
{
		///////// 
	///////// Shader, Texture, Frame Buffers (GPU)  Initialization
	// FB = place where you render

	GLboolean bShaderCompilerSupport = GL_TRUE;
	gl::GetBooleanv(GL_SHADER_COMPILER, &bShaderCompilerSupport);
	if (bShaderCompilerSupport == GL_FALSE)
	{
		printf("No shader compiler support\n");
		return false;
	}



	if (!loadShaders(ptVertexShaderFilename, ptFragmentShaderFilename, &rnShaderProgram))
	{
		printf("Could not load shaders (vertex shader filename: %s; fragment shader filename: %s\n", ptVertexShaderFilename, ptFragmentShaderFilename);
		return false;
	}
	return true;
}

bool GpuProcessing::BeginProcessing(GLuint nShaderProgram, int nTexWidth, int nTexHeight, GLenum eTexFormat)
{
	GLuint nFbo, nRbo, nFboTex;
	if (!InitTexture(nTexWidth, nTexHeight, eTexFormat, nFboTex))	 
	{
		printf("Failed to initialize texture\n");
		return false;	
	}

	if (!InitFbo(nFbo, nRbo, nFboTex))
	{
		printf("Failed to initialize frame buffer object\n");
		return false;
	}
	
	
	//bind shader
	gl::UseProgram(nShaderProgram);
	return true;
}

bool GpuProcessing::EndProcessing()
{	
	//glDeleteProgram(program);

	gl::UseProgram(0); //unbind the shader
	
	UnsetFbo(); // unbind the FBO - should be before glDrawArrays - look up

	
	// maybe more stuff here because we don't want to interfere with rendering
	// look into if needed :)


	//printf("Deleting frame buffer & texture\n");
//	glDeleteFramebuffers(1, &m_nCurrentFbo);
//	glDeleteRenderbuffers(1, &m_nCurrentRbo);
//	glDeleteTextures(1, &m_nCurrentFboTex);

	m_nCurrentShaderProgram = m_nDefaultShaderProgram;
//	m_nCurrentFbo = 0;
//	m_nCurrentFboTex = 0;
	return true;
}


