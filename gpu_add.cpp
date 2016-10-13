/*
Main resource for this code

- Pipeline overview:
http://www.lighthouse3d.com/tutorials/glsl-tutorial/pipeline-overview/
http://www.lighthouse3d.com/tutorials/glsl-tutorial/opengl-setup-for-glsl/

- huge GLSL shader example on github:
http://stackoverflow.com/questions/10316708/ios-glsl-is-there-a-way-to-create-an-image-histogram-using-a-glsl-shader
https://github.com/BradLarson/GPUImage/tree/master/framework

- future - brightness:
http://www.lighthouse3d.com/opengl/ledshader/index.php?page3


About vertex (vsh) and fragment (fsh) shaders
=============================================

A fragment shader is the same as pixel shader.

One main difference is that a vertex shader can manipulate the attributes of vertices. which are the corner points of your polygons.

The fragment shader on the other hand takes care of how the pixels between the vertices look. They are interpolated between the defined vertices following specific rules.

For example: if you want your polygon to be completely red, you would define all vertices red. If you want for specific effects like a gradient between the vertices, you have to do that in the fragment shader.

Put another way:

The vertex shader is part of the early steps in the graphic pipeline, somewhere between model coordinate transformation and polygon clipping I think. At that point, nothing is really done yet.

However, the fragment/pixel shader is part of the rasterization step, where the image is calculated and the pixels between the vertices are filled in or "coloured".

Just read about the graphics pipeline here and everything will reveal itself: http://en.wikipedia.org/wiki/Graphics_pipeline

http://stackoverflow.com/questions/4421261/vertex-shader-vs-fragment-shader


*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "PVRNativeApi/OGLES/OpenGLESBindings.h"
#include "shader_util.h"
#include "gpu_add.h"

static bool initTexture(int num_vals, GLuint *pFboTex)
{
	// setup texture
	gl::Enable(GL_TEXTURE_2D);
	gl::ActiveTexture(GL_TEXTURE1);	// TODO: find out whether this is useful here at all
	gl::GenTextures(1, pFboTex);
	gl::BindTexture(GL_TEXTURE_2D, *pFboTex);
	gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// This is necessary for non-power-of-two textures
	gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// allocate texture buffer
	// NOTE: GL_LUMINANCE cannot be used because it will result in a GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
	//       when trying to attach it to a frame buffer, so we use GL_RGBA instead
	gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, num_vals, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	
	// unbind again
	gl::BindTexture(GL_TEXTURE_2D, 0);
	return true;
}

static bool initFBO(GLuint *pFbo, GLuint fboTex)
{
	gl::ActiveTexture(GL_TEXTURE1);	// TODO: find out whether this is useful here at all
	
	gl::GenFramebuffers(1, pFbo);
	gl::BindFramebuffer(GL_FRAMEBUFFER, *pFbo);

	gl::BindTexture(GL_TEXTURE_2D, fboTex);
	gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
	
        GLenum status = gl::CheckFramebufferStatus(GL_FRAMEBUFFER);
	        
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
	        printf("Incomplete FBO: %x\n", status);
		return false;
	}

	// unbind texture & frame buffer
	gl::BindTexture(GL_TEXTURE_2D, 0);
	gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

static void setFBO(int num_vals, GLuint fbo)
{
        gl::BindFramebuffer(GL_FRAMEBUFFER, fbo);
              
	//set the viewport to be the size of the texture
	gl::Viewport(0, 0, num_vals, 1);
}

static void unsetFBO()
{	
	gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
}





bool gpu_add(int num_vals, short *in_vals_1, short *in_vals_2, float *out_vals)
{
	///////// 
	///////// Shader, Texture, Frame Buffers (GPU)  Initialization
	// FB = place where you render

	GLuint program, fbo, fboTex;
	GLboolean shaderCompilerSupport = GL_TRUE;
	gl::GetBooleanv(GL_SHADER_COMPILER, &shaderCompilerSupport);
	if (shaderCompilerSupport == GL_FALSE)
	{
		printf("No shader compiler support\n");
		return false;
	}



	if (!loadShaders("gpu_add.vsh", "gpu_add.fsh", &program))
	{
		printf("Could not load shaders\n");
		return false;
	}
	if (!initTexture(num_vals, &fboTex))	 
	{
		printf("Failed to initialize texture\n");
		return false;	
	}

	if (!initFBO(&fbo, fboTex))
	{
		printf("Failed to initialize frame buffer object\n");
		return false;
	}
	

	//bind shader
	gl::UseProgram(program);

	//bind buffer
	setFBO(num_vals, fbo);

	//clear the ouput texture
	gl::ClearColor(0.0, 0.0, 0.0, 1.0);
	gl::Clear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Enable blending because this enables addition 
	// to look into - how do you subtract/multiply/division
	gl::BlendEquation(GL_FUNC_ADD);
	gl::BlendFunc(GL_ONE, GL_ONE);
	gl::Enable(GL_BLEND);


	///////////
	// //////// mapping data from RAM to GPU

	// Use "render to vertex buffer" technique by setting pixel data as vertex attribute
	// Depth pixel is 16 bit wide -> use GL_SHORT as attribute type
	// this is the maapping of in_vals_1 to the shader (memory transfer RAM to GPU)
	// not sure if correct - something missing (how are the values mapped? is each value mapped to one vertex? )
	gl::VertexAttribPointer (ATTRIB_POSITION, 1, GL_UNSIGNED_SHORT, GL_FALSE, 0, in_vals_1); 	// put depth image pixel values in parallel in the vertex shader
	// we agreed that this is a 2-dimensional array where in_vals_1 is the matrix with the second parameter giving the number of elements in each vertex
	gl::EnableVertexAttribArray(ATTRIB_POSITION); // mapping activation

//        glBindFramebuffer(GL_ARRAY_BUFFER, 0); // should be called here before glDrawArrays
// http://stackoverflow.com/questions/7617668/glvertexattribpointer-needed-everytime-glbindbuffer-is-called

	//////////
	////////// GPU Calculation by calling DrawArrays

	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
	printf("start drawing points\n");
	gl::DrawArrays(GL_POINTS, 0, num_vals);			// run shader program on the vertex shader pixel values
	printf("Finished drawing points\n");


/*
	Mainly played with the following parameters
	- glReadPixels - exchanged width and height (1 with num_vals), RGBA and ALPHA
	- the input array - in_vals_1 and in_vals_2
	- glDrawArray - count - num_vals with 1
	- vertex shaders - enabling normalization

	Conclusions:
		- if the width and height are exchanged, and RGBA is used, then the first values in the output array are accumulated
*/	

	////////// 
	////////// GPU computation is done, save back to RAM


	// Download pixels
	//NOTE: The output buffer contains RGBA data (we set the output type
        //      when we initialize the texture). Because we are using alpha
	//      blending to accumulate values, the actual output is in the 
	//	alpha channel of the pixels in the output buffer. The 
	//      alpha channel byte is assumed to be the MSB of each 4-byte
	//      pixel.
	unsigned char  buf[4*num_vals];
 	gl::ReadPixels(0, 0, num_vals, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf); // GL_ALPHA takes num_vals-5 elements and zeroes them, while GL_RGB zeroes all

	for (int nIndex=0; nIndex < num_vals; nIndex++)
	{
		out_vals[nIndex] = (float)buf[4*nIndex];
	}


	// Disable blending again
	gl::Disable(GL_BLEND);
	gl::DeleteProgram(program);

	gl::UseProgram(0); //unbind the shader
	
	unsetFBO(); // unbind the FBO - should be before glDrawArrays - look up

	
	// maybe more stuff here because we don't want to interfere with rendering
	// look into if needed :)


	printf("Deleting frame buffer & texture\n");
	gl::DeleteFramebuffers(1, &fbo);
	gl::DeleteTextures(1, &fboTex);

	return true;
}




/*
int main(int argc, char* argv[])
{
	
	int num_vals = 16;
	short in_vals_1[num_vals];

	short in_vals_2[num_vals];

	short out_vals[num_vals];

	memset(in_vals_1, 0, sizeof(short)*num_vals);
	memset(in_vals_2, 0, sizeof(short)*num_vals);
	memset(out_vals, 0, sizeof(short)*num_vals);

	
	for (int i=0; i < num_vals; i++)
	{
		in_vals_1[i] = i + 1;
	}	
	
	gpu_add(num_vals, in_vals_1, in_vals_2, out_vals);

	for (int i=0; i < num_vals; i++)
	{
		printf("Val %d: %d\n", i, out_vals[i]);
	}	
}
*/
