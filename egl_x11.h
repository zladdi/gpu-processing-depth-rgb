#ifndef EGL_X11_H
#define EGL_X11_H

#include <stdio.h>
#include <string>
#include "X11/Xlib.h"
#include "X11/Xutil.h"
#include "PVRNativeApi/OGLES/OpenGLESBindings.h"
#include "PVRNativeApi/OGLES/OpenGLESBindings.h"
#include "PVRPlatformGlue/EGL/ExtensionLoaderEgl.h"

/******************************************************************************

 Based on OGLES2 HelloAPI from the PowerVR SKD examples.

******************************************************************************/

class EGL_X11
{
private:
	// X11 variables
	Window				x11Window;
	Display*			x11Display;
	long				x11Screen;
	XVisualInfo*		x11Visual;
	Colormap			x11Colormap;

	// EGL variables
	EGLDisplay			eglDisplay;
	EGLConfig			eglConfig;
	EGLSurface			eglSurface;
	EGLContext			eglContext;
	GLuint	ui32Vbo; // Vertex buffer object handle

	/*
		EGL has to create a context for OpenGL ES. Our OpenGL ES resources
		like textures will only be valid inside this context
		(or shared contexts).
		Creation of this context takes place at step 7.
	*/
	EGLint ai32ContextAttribs[3];

	Window					sRootWindow;
    	XSetWindowAttributes	sWA;
	unsigned int			ui32Mask;
	int						i32Depth;
	int 					i32Width, i32Height;
	// Text rendering
	//CPVRTPrint3D print3D; 


public:
	EGL_X11();
	bool Init();
	void Cleanup();
	void SwapBuffers();
	int WindowWidth();
	int WindowHeight();
	bool InitTextRendering();
	void CleanupTextRendering();
	void WriteText(float posX, float posY, float scale,
		unsigned int color, const std::string& formatString, ...);
	KeySym getKey(char * buffer);
};

#endif /* EGL_X11_H */

