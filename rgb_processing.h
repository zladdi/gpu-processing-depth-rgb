#ifndef RGB_PROCESSING_H
#define RGB_PROCESSING_H

#include "PVRNativeApi/OGLES/OpenGLESBindings.h"
#include "gpu_processing.h"

class RgbProcessing : public GpuProcessing
{
public:
	enum HistogramType {
		HISTO_RED = 0,
		HISTO_GREEN,
		HISTO_BLUE,
		HISTO_LUMINANCE
	};

	RgbProcessing(GLuint nDefaultShaderProgram = 0);

	bool Init();
	
	bool Histogram(void* pPixels, int nWidth, int nHeight,
		 int nPixelSize, int* pnHistogram, int nNumHistoBins,
		 HistogramType eHistoType, int nPixelElementSize=1);

	//void RgbDetectLuminousArea();
//	void RgbToGrayscale(void* pRgb, void* pGray, int nWidth, int nHeight);
	void runShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ);
	void grayscaleShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ);
	void hueShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ);
	void edgeShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ);
	void contrastShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ);
	void brightnessShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ);
	void threshColorShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ);
	void drawShader(void* pRgb, void* pGray, int width, int height, GLuint m_ui32Vbo, GLuint m_uiTexture, GLenum format, unsigned int m_ui32VertexStride, float eyeX, float eyeY, float eyeZ);

int processGrid (void* pixels, unsigned int mapX, unsigned int mapY, int gridFlag, int divs, int tolerance);
private:
	GLuint m_nRedHistoShaderProgram;
	GLuint m_nGreenHistoShaderProgram;
	GLuint m_nBlueHistoShaderProgram;
	GLuint m_nLuminanceHistoShaderProgram;
	GLuint m_nGrayscaleShaderProgram;
	GLuint m_nHueShaderProgram;
	GLuint m_nEdgeShaderProgram;
	GLuint m_nContrastShaderProgram;
	GLuint m_nBrightnessShaderProgram;
	GLuint m_nThreshColorShaderProgram;
	GLuint m_nDrawShaderProgram;


typedef struct XnRGB24Pixel
 {
     unsigned char nRed;
     unsigned char nGreen;
     unsigned char nBlue;
 } XnRGB24Pixel;

typedef struct XnRGBA32Pixel
 {
     unsigned char nRed;
     unsigned char nGreen;
     unsigned char nBlue;
     unsigned char nAlpha;
 } XnRGBA32Pixel;



};

#endif /* RGB_PROCESSING_H */
