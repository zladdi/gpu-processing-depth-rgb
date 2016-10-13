#include "egl_x11.h"
#include <cstdarg>
#include <cstring>

/******************************************************************************

 Based on OGLES2 HelloAPI from the PowerVR SKD examples.

******************************************************************************/

/// Index to bind the attributes to vertex shaders
//#define VERTEX_ARRAY	0

// Max width and height of the window
#define WINDOW_WIDTH	1000
#define WINDOW_HEIGHT	800

// Max letters for text to be rendered
#define MAX_LETTERS	255


static bool TestEGLError(const char* pszLocation)
{
	/*
		eglGetError returns the last error that has happened using egl,
		not the status of the last called function. The user has to
		check after every single egl call or at least once every frame.
	*/
	EGLint iErr = egl::GetError();
	if (iErr != EGL_SUCCESS)
	{
		printf("%s failed (%d).\n", pszLocation, iErr);
		return false;
	}

	return true;
}

EGL_X11::EGL_X11()
{
		x11Window	= 0;
		x11Display	= 0;
		x11Screen	= 0;
		x11Visual	= 0;
		x11Colormap	= 0;

	// EGL variables
		eglDisplay	= 0;
		eglConfig	= 0;
		eglSurface	= 0;
		eglContext	= 0;
	ui32Vbo = 0; // Vertex buffer object handle

	/*
		EGL has to create a context for OpenGL ES. Our OpenGL ES resources
		like textures will only be valid inside this context
		(or shared contexts).
		Creation of this context takes place at step 7.
	*/
	 ai32ContextAttribs[0] = EGL_CONTEXT_CLIENT_VERSION;
	 ai32ContextAttribs[1] = 2;
	 ai32ContextAttribs[2] =  EGL_NONE ;

}

bool EGL_X11::Init()
{
	// Initializes the display and screen
x11Display = XOpenDisplay( 0 );
	
	if (!x11Display)
	{
		printf("Error: Unable to open X display\n");
		Cleanup();
		return false;
	}
	x11Screen = XDefaultScreen( x11Display );

	// Gets the window parameters
	sRootWindow = RootWindow(x11Display, x11Screen);

	i32Depth = DefaultDepth(x11Display, x11Screen);
	x11Visual = new XVisualInfo;
	XMatchVisualInfo( x11Display, x11Screen, i32Depth, TrueColor, x11Visual);
	if (!x11Visual)
	{
		printf("Error: Unable to acquire visual\n");
		Cleanup();
		return false;
	}
    x11Colormap = XCreateColormap( x11Display, sRootWindow, x11Visual->visual, AllocNone );

    sWA.colormap = x11Colormap;

    // Add to these for handling other events
    sWA.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
    ui32Mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

	i32Width  = WINDOW_WIDTH  < XDisplayWidth(x11Display, x11Screen) ? WINDOW_WIDTH : XDisplayWidth(x11Display, x11Screen);
	i32Height = WINDOW_HEIGHT < XDisplayHeight(x11Display,x11Screen) ? WINDOW_HEIGHT: XDisplayHeight(x11Display,x11Screen);

	// Creates the X11 window
    x11Window = XCreateWindow( x11Display, RootWindow(x11Display, x11Screen), 0, 0, i32Width, i32Height,
								 0, CopyFromParent, InputOutput, CopyFromParent, ui32Mask, &sWA);

	XMapWindow(x11Display, x11Window);
	XFlush(x11Display);

	egl::initEgl();
	/*
		Step 1 - Get the default display.
		EGL uses the concept of a "display" which in most environments
		corresponds to a single physical screen. Since we usually want
		to draw to the main screen or only have a single screen to begin
		with, we let EGL pick the default display.
		Querying other displays is platform specific.
	*/
	eglDisplay = egl::GetDisplay((EGLNativeDisplayType)x11Display);

	/*
		Step 2 - Initialize EGL.
		EGL has to be initialized with the display obtained in the
		previous step. We cannot use other EGL functions except
		eglGetDisplay and eglGetError before eglInitialize has been
		called.
		If we're not interested in the EGL version number we can just
		pass NULL for the second and third parameters.
	*/
	EGLint iMajorVersion, iMinorVersion;
	if (!egl::Initialize(eglDisplay, &iMajorVersion, &iMinorVersion))
	{
		printf("Error: eglInitialize() failed.\n");
		Cleanup();
		return false;
	}

	/*
		Step 3 - Make OpenGL ES the current API.
		EGL provides ways to set up OpenGL ES and OpenVG contexts
		(and possibly other graphics APIs in the future), so we need
		to specify the "current API".
	*/
	egl::BindAPI(EGL_OPENGL_ES_API);

	if (!TestEGLError("eglBindAPI"))
	{
		Cleanup();
		return false;
	}

	/*
		Step 4 - Specify the required configuration attributes.
		An EGL "configuration" describes the pixel format and type of
		surfaces that can be used for drawing.
		For now we just want to use a 16 bit RGB surface that is a
		Window surface, i.e. it will be visible on screen. The list
		has to contain key/value pairs, terminated with EGL_NONE.
	 */
	EGLint pi32ConfigAttribs[5];
	pi32ConfigAttribs[0] = EGL_SURFACE_TYPE;
	pi32ConfigAttribs[1] = EGL_WINDOW_BIT;
	pi32ConfigAttribs[2] = EGL_RENDERABLE_TYPE;
	pi32ConfigAttribs[3] = EGL_OPENGL_ES2_BIT;
	pi32ConfigAttribs[4] = EGL_NONE;

	/*
		Step 5 - Find a config that matches all requirements.
		eglChooseConfig provides a list of all available configurations
		that meet or exceed the requirements given as the second
		argument. In most cases we just want the first config that meets
		all criteria, so we can limit the number of configs returned to 1.
	*/
	EGLint iConfigs;
	if (!egl::ChooseConfig(eglDisplay, pi32ConfigAttribs, &eglConfig, 1, &iConfigs) || (iConfigs != 1))
	{
		printf("Error: eglChooseConfig() failed.\n");
		Cleanup();
		return false;
	}

	/*
		Step 6 - Create a surface to draw to.
		Use the config picked in the previous step and the native window
		handle when available to create a window surface. A window surface
		is one that will be visible on screen inside the native display (or
		fullscreen if there is no windowing system).
		Pixmaps and pbuffers are surfaces which only exist in off-screen
		memory.
	*/
	eglSurface = egl::CreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType)x11Window, NULL);

    if (!TestEGLError("eglCreateWindowSurface"))
	{
		Cleanup();
		return false;
	}

	/*
		Step 7 - Create a context.
	*/
	eglContext = egl::CreateContext(eglDisplay, eglConfig, NULL, ai32ContextAttribs);
	if (!TestEGLError("eglCreateContext"))
	{
		Cleanup();
		return false;
	}

	/*
		Step 8 - Bind the context to the current thread and use our
		window surface for drawing and reading.
		Contexts are bound to a thread. This means you don't have to
		worry about other threads and processes interfering with your
		OpenGL ES application.
		We need to specify a surface that will be the target of all
		subsequent drawing operations, and one that will be the source
		of read operations. They can be the same surface.
	*/
	egl::MakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	if (!TestEGLError("eglMakeCurrent"))
	{
		Cleanup();
		return false;
	}

	// Text rendering 
	if (!InitTextRendering())
	{
		Cleanup();
		return false;
	}
	return true;
}

bool EGL_X11::InitTextRendering()
{
//	if (print3D.SetTextures(0,WindowWidth(),
//		WindowHeight(), false) != PVR_SUCCESS)
//	{
//		printf("Failed to initialize Print3D");
//		return false;
 //	}
	
	return true;
}

void EGL_X11::Cleanup()
{
	CleanupTextRendering();

	egl::MakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) ;
	egl::Terminate(eglDisplay);

	/*
		Step 11 - Destroy the eglWindow.
		Again, this is platform specific and delegated to a separate function
	*/
	if (x11Window) XDestroyWindow(x11Display, x11Window);
    	if (x11Colormap) XFreeColormap( x11Display, x11Colormap );
	if (x11Display) XCloseDisplay(x11Display);

    	delete x11Visual;
}

void EGL_X11::CleanupTextRendering()
{
	
	//print3D.ReleaseTextures();
}

void EGL_X11::WriteText(float posX, float posY, float scale, unsigned int color,
	const std::string& formatString, ...)
{
	va_list		args;
	static char	s_Text[MAX_LETTERS+1] = {0};
	
	// Reading the arguments to create our Text string
	va_start(args, formatString);
	vsnprintf(s_Text, MAX_LETTERS+1, formatString.c_str(), args);
	va_end(args);



	//print3D.Print3D(posX, posY, scale, color, s_Text);
	//print3D.Flush();
}

void EGL_X11::SwapBuffers() {
	egl::SwapBuffers(eglDisplay, eglSurface);
}

KeySym EGL_X11::getKey(char* bufferOut) {
		// Managing the X11 messages
		int i32NumMessages = XPending( x11Display );
		KeySym keysym;
		int buffer_size = 80;
		char buffer[80];

		for( int i = 0; i < i32NumMessages; i++ )
		{
			XEvent	event;
			XNextEvent( x11Display, &event );

			switch( event.type )
			{
			// Exit on mouse click
			case ButtonPress:
        			Cleanup();
	        		break;
			case KeyPress:
				XLookupString(&event.xkey,buffer,buffer_size, &keysym, 0);
				std::memcpy(bufferOut, buffer, 80);
				return keysym;
		            	break;

			default:
				break;
			}
		}

	return XK_F1; //default return
}

int EGL_X11::WindowWidth()
{
	return WINDOW_WIDTH; 
}

int EGL_X11::WindowHeight()
{
	return WINDOW_HEIGHT;
}


