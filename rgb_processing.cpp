#include "rgb_processing.h"
#include <cstdio>
#include <string>
//#include "shader_util.h"
#include "cvglsl.h"
#include "egl_x11.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

static const std::string xctShaderFolderPath("shaders/");

static const std::string xctRedHistoVertexShaderFilename(
			xctShaderFolderPath + "redHistogram.vsh");
static const std::string xctGreenHistoVertexShaderFilename(
			xctShaderFolderPath + "greenHistogram.vsh");
static const std::string xctBlueHistoVertexShaderFilename(
			xctShaderFolderPath + "blueHistogram.vsh");
static const std::string xctLuminanceHistoVertexShaderFilename(
			xctShaderFolderPath + "luminanceHistogram.vsh");


static const std::string xctHistoAccumulationFragmentShaderFilename(
			xctShaderFolderPath + "histogramAccumulation.fsh");

std::string xctDrawVSFilename(xctShaderFolderPath + "draw.vsh");
std::string xctDrawFSFilename(xctShaderFolderPath + "draw.fsh");


std::string xctGrayscaleVSFilename(xctShaderFolderPath + "grayscale.vsh");
std::string xctGrayscaleFSFilename(xctShaderFolderPath + "grayscale.fsh");

std::string xctHueVSFilename(xctShaderFolderPath + "hue.vsh");
std::string xctHueFSFilename(xctShaderFolderPath + "hue.fsh");

std::string xctEdgeVSFilename(xctShaderFolderPath + "edge.vsh"); 
std::string xctEdgeFSFilename(xctShaderFolderPath + "edge.fsh");

std::string xctContrastVSFilename(xctShaderFolderPath + "contrast.vsh"); 
std::string xctContrastFSFilename(xctShaderFolderPath + "contrast.fsh");

std::string xctBrightnessVSFilename(xctShaderFolderPath + "brightness.vsh"); 
std::string xctBrightnessFSFilename(xctShaderFolderPath + "brightness.fsh");

std::string xctThreshColorVSFilename(xctShaderFolderPath + "threshColor.vsh"); 
std::string xctThreshColorFSFilename(xctShaderFolderPath + "threshColor.fsh");


static const GLuint xcnPositionAttribHistoVertexShader = 0; // mapping activation

RgbProcessing::RgbProcessing(GLuint nDefaultShaderProgram)
	: GpuProcessing(nDefaultShaderProgram),
	  m_nRedHistoShaderProgram(0),
	  m_nGreenHistoShaderProgram(0),
	  m_nBlueHistoShaderProgram(0),
	  m_nLuminanceHistoShaderProgram(0),
	  m_nGrayscaleShaderProgram(0)
{
}

bool RgbProcessing::Init()
{
	// load histogram shaders
	if(!LoadShaders(xctRedHistoVertexShaderFilename.c_str(),
			xctHistoAccumulationFragmentShaderFilename.c_str(),
			m_nRedHistoShaderProgram))
	{
		printf("Failed to load red histogram shaders\n");
		return false;
	}
	
	if(!LoadShaders(xctGreenHistoVertexShaderFilename.c_str(),
			xctHistoAccumulationFragmentShaderFilename.c_str(),
			m_nGreenHistoShaderProgram))
	{
		printf("Failed to load green histogram shaders\n");
		return false;
	}

	if(!LoadShaders(xctBlueHistoVertexShaderFilename.c_str(),
			xctHistoAccumulationFragmentShaderFilename.c_str(),
			m_nBlueHistoShaderProgram))
	{
		printf("Failed to load blue histogram shaders\n");
		return false;
	}

	if(!LoadShaders(xctLuminanceHistoVertexShaderFilename.c_str(),
			xctHistoAccumulationFragmentShaderFilename.c_str(),
			m_nLuminanceHistoShaderProgram))
	{
		printf("Failed to load luminance histogram shaders\n");
		return false;
	}
/*
	if(!LoadShaders(xctGrayscaleVSFilename.c_str(),xctGrayscaleFSFilename.c_str(),m_nGrayscaleShaderProgram ))
	{
		printf("Failed to load grayscale shaders\n");
		return false;
	}
*/
// 	TODO
// 	1. add shader compilation errors to shader_util.cpp
//	2. make shader_util.cpp attribute independent - don't link inside attributes
	CreateShaderProg(xctDrawVSFilename.c_str(),xctDrawFSFilename.c_str(),m_nDrawShaderProgram );
	CreateShaderProg(xctGrayscaleVSFilename.c_str(),xctGrayscaleFSFilename.c_str(),m_nGrayscaleShaderProgram );
	CreateShaderProg(xctHueVSFilename.c_str(),xctHueFSFilename.c_str(), m_nHueShaderProgram);
	CreateShaderProg(xctEdgeVSFilename.c_str(),xctEdgeFSFilename.c_str(), m_nEdgeShaderProgram);
	CreateShaderProg(xctContrastVSFilename.c_str(),xctContrastFSFilename.c_str(), m_nContrastShaderProgram);
	CreateShaderProg(xctThreshColorVSFilename.c_str(),xctThreshColorFSFilename.c_str(), m_nThreshColorShaderProgram);

	return true;
}

bool RgbProcessing::Histogram(void* pPixels, int nWidth,
	int nHeight, int nPixelSize, int* pnHistogram,
	int nNumHistoBins, HistogramType eHistoType, int nPixelElementSize)
{
	GLenum ePixelElementDataType;
	switch(nPixelElementSize)
	{
		case 1:
			ePixelElementDataType = GL_UNSIGNED_BYTE;
			break;
		case 2:
			ePixelElementDataType = GL_UNSIGNED_SHORT;
			break;
		default:
			printf("Unsupported pixel element size %d\n",
 nPixelElementSize);
			return false;
	}

	GLuint nShaderProgram = 0;
	switch(eHistoType)
	{
		case HISTO_RED:
			nShaderProgram = m_nRedHistoShaderProgram;
			break;
		case HISTO_GREEN:
			nShaderProgram = m_nGreenHistoShaderProgram;
			break;
		case HISTO_BLUE:
			nShaderProgram = m_nBlueHistoShaderProgram;
			break;
		case HISTO_LUMINANCE:
		default:
			nShaderProgram = m_nLuminanceHistoShaderProgram;
			break;
	}

	if (!BeginProcessing(nShaderProgram, nNumHistoBins, 1, GL_RGBA))
	{
		return false;
	}

	//clear the ouput texture
	gl::ClearColor(0.0, 0.0, 0.0, 1.0);
	gl::Clear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Enable blending because this enables addition 
	// to look into - how do you subtract/multiply/division
	gl::BlendEquation(GL_FUNC_ADD);
	gl::BlendFunc(GL_ONE, GL_ONE);
	gl::Enable(GL_BLEND);



	// //////// mapping data from RAM to GPU

	// Use "render to vertex buffer" technique by setting pixel data as vertex attribute
	// Depth pixel is 16 bit wide -> use GL_SHORT as attribute type
	// this is the maapping of in_vals_1 to the shader (memory transfer RAM to GPU)
	// not sure if correct - something missing (how are the values mapped? is each value mapped to one vertex? )
	gl::VertexAttribPointer (xcnPositionAttribHistoVertexShader, nPixelSize, ePixelElementDataType, GL_FALSE, 0, pPixels); 	// put depth image pixel values in parallel in the vertex shader
	// we agreed that this is a 2-dimensional array where in_vals_1 is the matrix with the second parameter giving the number of elements in each vertex
	gl::EnableVertexAttribArray(xcnPositionAttribHistoVertexShader); // mapping activation

//        glBindFramebuffer(GL_ARRAY_BUFFER, 0); // should be called here before glDrawArrays
// http://stackoverflow.com/questions/7617668/glvertexattribpointer-needed-everytime-glbindbuffer-is-called

	//////////
	////////// GPU Calculation by calling DrawArrays

	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
//	printf("start drawing points\n");
	gl::DrawArrays(GL_POINTS, 0, nWidth*nHeight);			// run shader program on the vertex shader pixel values
//	printf("Finished drawing points\n");


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
	unsigned char  acBuf[4*nNumHistoBins];
 	gl::ReadPixels(0, 0, nNumHistoBins, 1, GL_RGBA, GL_UNSIGNED_BYTE, acBuf); // GL_ALPHA takes num_vals-5 elements and zeroes them, while GL_RGB zeroes all

	for (int nIndex=0; nIndex < nNumHistoBins; nIndex++)
	{
		*pnHistogram = acBuf[4*nIndex];pnHistogram++;
	}


	// Disable blending again
	gl::Disable(GL_BLEND);
	if (!EndProcessing())
	{
		return false;
	}
	return true;
}
void RgbProcessing::drawShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ)
{	
	gl::UseProgram(m_nDrawShaderProgram); //unbind the shader
	runShader(pRgb, pGray, width, height, m_ui32Vbo, m_uiTexture, format, m_ui32VertexStride, eyeX, eyeY, eyeZ);
}

void RgbProcessing::grayscaleShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ)
{	
	gl::UseProgram(m_nGrayscaleShaderProgram); //unbind the shader
	runShader(pRgb, pGray, width, height, m_ui32Vbo, m_uiTexture, format, m_ui32VertexStride, eyeX, eyeY, eyeZ);
}

void RgbProcessing::hueShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ)
{	
	gl::UseProgram(m_nHueShaderProgram); //unbind the shader
	runShader(pRgb, pGray, width, height, m_ui32Vbo, m_uiTexture, format, m_ui32VertexStride, eyeX, eyeY, eyeZ);
}

void RgbProcessing::edgeShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ)
{	
	gl::UseProgram(m_nEdgeShaderProgram); //unbind the shader
	runShader(pRgb, pGray, width, height, m_ui32Vbo, m_uiTexture, format, m_ui32VertexStride, eyeX, eyeY, eyeZ);
}

void RgbProcessing::contrastShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ)
{	
	gl::UseProgram(m_nContrastShaderProgram); //unbind the shader
	runShader(pRgb, pGray, width, height, m_ui32Vbo, m_uiTexture, format, m_ui32VertexStride, eyeX, eyeY, eyeZ);
}

void RgbProcessing::brightnessShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ)
{	
	gl::UseProgram(m_nBrightnessShaderProgram); //unbind the shader
	runShader(pRgb, pGray, width, height, m_ui32Vbo, m_uiTexture, format, m_ui32VertexStride, eyeX, eyeY, eyeZ);
}

void RgbProcessing::threshColorShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ)
{	
	gl::UseProgram(m_nThreshColorShaderProgram); //unbind the shader
	int VERTEX_ARRAY = 0;
	int TEXCOORD_ARRAY = 1;
//	BeginProcessing(m_nGrayscaleShaderProgram, width, height, GL_RGB);

int i32Location = gl::GetUniformLocation(m_nGrayscaleShaderProgram, "myPMVMatrix");
	glm::mat4		m_mProjection, m_mView;
	m_mProjection = glm::perspective(glm::pi<float>()/6.0f, 1.0f, 1.0f, 2000.0f);
	m_mView = glm::lookAt(glm::vec3(eyeX, eyeY, eyeZ), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	// Set model view projection matrix
	glm::mat4 mModelView, mMVP;
	mModelView = m_mView ;//* mModel;
	mMVP =  m_mProjection * mModelView;

// 	use the shader for the transformations 
//	glUniformMatrix4fv(m_ShaderProgram.auiLoc[eMVPMatrix], 1, GL_FALSE, glm::value_ptr(mMVP));
	// Then passes the matrix to that variable
	gl::UniformMatrix4fv(i32Location, 1, GL_FALSE, glm::value_ptr(mMVP) );
		
//	glBindBuffer(GL_ARRAY_BUFFER, m_ui32Vbo);
	gl::BindTexture(GL_TEXTURE_2D, m_uiTexture);
	gl::TexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, 0);
	gl::BindTexture(GL_TEXTURE_2D, 0);
	
	gl::BindFramebuffer(GL_FRAMEBUFFER, m_ui32Vbo);
	

	gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_uiTexture, 0);

	if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		printf("FB problem\n"); return;}
	else
		printf("FB bound successfully\n");
		

	gl::EnableVertexAttribArray(VERTEX_ARRAY);
	gl::VertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, m_ui32VertexStride, 0);

	// Pass the texture coordinates data
	gl::EnableVertexAttribArray(TEXCOORD_ARRAY);
	gl::VertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, m_ui32VertexStride, (void*) (3 * sizeof(GLfloat)));

//	GLenum attachments[2] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
//	m_ext.glDrawBuffersEXT(1, attachments);


	
	gl::DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//	glDrawArrays(GL_POINTS, 0, width*height);			// run shader program on the vertex shader pixel values
// 	glReadPixels(0, 0, width*4, height*4, GL_RGBA, GL_UNSIGNED_BYTE, pGray); // GL_ALPHA takes num_vals-5 elements and zeroes them, while GL_RGB zeroes all
// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pGray);

//	glUseProgram(m_nThreshColorShaderProgram); //unbind the shader
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	gl::UseProgram(0); //unbind the shader
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
	gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
	gl::BindTexture(GL_TEXTURE_2D, 0);	

}

void RgbProcessing::runShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ)
{
	int VERTEX_ARRAY = 0;
	int TEXCOORD_ARRAY = 1;
//	BeginProcessing(m_nGrayscaleShaderProgram, width, height, GL_RGB);

int i32Location = gl::GetUniformLocation(m_nGrayscaleShaderProgram, "myPMVMatrix");

	glm::mat4		m_mProjection, m_mView;
	m_mProjection = glm::perspective(glm::pi<float>()/6.0f, 1.0f, 1.0f, 2000.0f);
	m_mView = glm::lookAt(glm::vec3(eyeX, eyeY, eyeZ), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	// Set model view projection matrix
	glm::mat4 mModelView, mMVP;
	mModelView = m_mView ;//* mModel;
	mMVP =  m_mProjection * mModelView;


// 	use the shader for the transformations 
//	glUniformMatrix4fv(m_ShaderProgram.auiLoc[eMVPMatrix], 1, GL_FALSE, glm::value_ptr(mMVP));

	// Then passes the matrix to that variable
	gl::UniformMatrix4fv(i32Location, 1, GL_FALSE, glm::value_ptr(mMVP) );
		
//	glBindBuffer(GL_ARRAY_BUFFER, m_ui32Vbo);
	gl::BindTexture(GL_TEXTURE_2D, m_uiTexture);
	gl::TexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, 0);
	gl::BindTexture(GL_TEXTURE_2D, 0);

	gl::BindFramebuffer(GL_FRAMEBUFFER, m_ui32Vbo);
	gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_uiTexture, 0);

	if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
{		printf("FB problem\n"); return;}
	else
		printf("FB bound successfully\n");
		

	gl::EnableVertexAttribArray(VERTEX_ARRAY);
	gl::VertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, m_ui32VertexStride, 0);

	// Pass the texture coordinates data
	gl::EnableVertexAttribArray(TEXCOORD_ARRAY);
	gl::VertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, m_ui32VertexStride, (void*) (3 * sizeof(GLfloat)));


	gl::DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
// for drawing scaled up points:
//			int GL_VERTEX_PROGRAM_POINT_SIZE = 0x8642;
//			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
//	glDrawArrays(GL_POINTS, 0, width*height);			// run shader program on the vertex shader pixel values
//	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pGray);	

	gl::UseProgram(0); //unbind the shader
	gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glBindTexture(GL_TEXTURE_2D, 0);
}

int RgbProcessing::processGrid (void* pixels, unsigned int mapX, unsigned int mapY, int gridFlag, int divs, int tolerance) // draws on the pixels through pTex variable
{
	int nx = mapX / divs; // width of a cell
	int ny = mapY / divs; 
	unsigned int cellcounts[divs][divs];
	int cMax = 225;
	float scale = 0.0225; // cMax/MAX_DEPTH = 225 / 10000
	unsigned int temp;
	int cellx, celly;
	long int pixSum=0;

        XnRGBA32Pixel* pix = (XnRGBA32Pixel*)pixels;
	memset(cellcounts, 0, divs*divs*sizeof(unsigned int));
		
	// draw grid and count pixels that are in cells that are within the threshold distance
	for (int y = 0; y < mapY; ++y) // loop colors pixels
	{
		celly = y / ny;
		for (int x = 0; x < mapX; x++, pix++)
		{
//			printf("%d   %d   %d\n", pix->nRed, pix->nGreen, pix->nBlue);
			pixSum = (pix->nRed + pix->nGreen + pix->nBlue )/30;	
			cellx = x / nx;
//			cellcounts[cellx][celly]+= pixSum;

		if (gridFlag==1) {
			if (x % nx == 0)
				for (int j=0; j<3; j++) {
					(pix+j)->nRed = (pix+j)->nGreen = cMax;(pix+j)->nBlue = 0; }

			if (y % ny == 0)
				for (int j=0; j<3; j++) {
					(pix+j)->nRed = (pix+j)->nGreen = cMax;(pix+j)->nBlue = 0;}
		}

		}
//		printf("%u ", cellcounts[cellx][celly]);
	}
//	printf("\n---------eof \n ");
	

if (gridFlag==1) { // color grids with red or green depending on the cell neighbour difference and tolerance

        pix = (XnRGBA32Pixel*)pixels;
	for (int y = 0; y < mapY; ++y) // loop colors pixels
	{
		celly = y / ny;
		for (int x = 0; x < mapX; x++, pix++)
		{
			cellx = x / nx;

			if (celly > 0 && celly < divs - 3) {
				int difCurrent =  abs(cellcounts[cellx][celly] - cellcounts[cellx][celly+1]);
				if (difCurrent < tolerance)
					(pix)->nRed = pix->nRed/4;
				else {
					(pix)->nGreen = pix->nGreen/4;
					}
					 
			}
		}
//		printf("%u ", cellcounts[cellx][celly]);
	}
//	printf("\n---------eof \n ");
	}

	return 1;

}
