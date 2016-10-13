#include "openni_gpu_histogram.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "GLES2/gl2.h"
#include "shader_util.h"


bool initTexture(hist_state_t* histState)
{
	// setup texture
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);	// TODO: find out whether this is useful here at all
	glGenTextures(1, &histState->fboTex);
	glBindTexture(GL_TEXTURE_2D, histState->fboTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// This is necessary for non-power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// allocate texture buffer
	// NOTE: GL_LUMINANCE cannot be used because it will result in a GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
	//       when trying to attach it to a frame buffer, so we use GL_RGBA instead
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, histState->outXRes, histState->outYRes, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	
	// unbind again
	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

bool initFBO(hist_state_t* histState)
{
	glActiveTexture(GL_TEXTURE1);	// TODO: find out whether this is useful here at all
	glGenFramebuffers(1, &histState->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, histState->fbo);

	glBindTexture(GL_TEXTURE_2D, histState->fboTex);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, histState->fboTex, 0);
		
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	        
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
	        printf("Incomplete FBO: %x\n", status);
		return false;
	}

	// unbind texture & frame buffer
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

void setFBO(hist_state_t* histState)
{
        glBindFramebuffer(GL_FRAMEBUFFER, histState->fbo);
	//set the viewport to be the size of the texture
	glViewport(0, 0, histState->outXRes, histState->outYRes);
}

void unsetFBO()
{	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


bool openni_gpu_histogram_init(hist_state_t* histState, int inXRes, int inYRes, int inMaxVal)
{
	memset( histState, 0, sizeof( *histState ) );
	histState->inXRes = inXRes;
	histState->inYRes = inYRes;
	histState->outXRes = inMaxVal;
	histState->outYRes = 1;

	GLboolean shaderCompilerSupport = GL_TRUE;
	glGetBooleanv(GL_SHADER_COMPILER, &shaderCompilerSupport);
	if (shaderCompilerSupport == GL_FALSE)
	{
		printf("No shader compiler support\n");
		return false;
	}

	if (!loadShaders("histogram.vsh", "histogram.fsh", &histState->shaderProgram))
	{
		return false;
	}
	histState->shaderProgramLoaded = true;

	if (!initTexture(histState))	 
	{
		printf("Failed to initialize texture\n");
		return false;	
	}

	if (!initFBO(histState))
	{
		printf("Failed to initialize frame buffer object\n");
		return false;
	}
	return true;
}

void openni_gpu_histogram_close(hist_state_t* histState)
{
	printf("Deleting frame buffer & texture\n");
	glDeleteFramebuffers(1, &histState->fbo);
	glDeleteTextures(1, &histState->fboTex);
}


void openni_gpu_histogram_calculate(hist_state_t* histState, const void* pDepth, float depthHist[])
{

	if (!histState->shaderProgramLoaded)
	{
		printf("No shader loaded\n");
		return;
	}

	// set frame buffer
	setFBO(histState);

	//bind shader
	glUseProgram(histState->shaderProgram);

	//clear the ouput texture
	glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Enable blending
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);

	// Use "render to vertex buffer" technique by setting pixel data as vertex attribute
	// Depth pixel is 16 bit wide -> use GL_SHORT as attribute type
	glVertexAttribPointer (ATTRIB_POSITION, 1, GL_UNSIGNED_SHORT, GL_FALSE, 0, pDepth); 	// put depth image pixel values in parallel in the vertex shader
	glEnableVertexAttribArray(ATTRIB_POSITION);

	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
	printf("start drawing points\n");
	glDrawArrays(GL_POINTS, 0, histState->outXRes * histState->outYRes);			// run shader program on the vertex shader pixel values
	printf("Finished drawing points\n");

	// Disable blending again
	glDisable(GL_BLEND);

	glUseProgram(0); //unbind the shader
	
	setFBO(0); // unbind the FBO

	// Download pixels
	unsigned char buf[histState->outXRes];
	
	glReadPixels(0, 0, histState->outXRes, histState->outYRes, GL_ALPHA, GL_UNSIGNED_BYTE, buf);

	for (int nIndex=1; nIndex < histState->outXRes; nIndex++)
	{
		depthHist[nIndex] = (float)buf[nIndex];
	}

}
