
#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <XnLog.h>
#include <XnCppWrapper.h>
#include <XnPropNames.h>
#include <signal.h>
#include "Capture.h"
#include <cstring>
#include "openni_device.h"
#include <iostream>
#include <stdlib.h>
#include "gpu_add.h"
#include <math.h>
#include "PVRNativeApi/OGLES/OpenGLESBindings.h"
#include "egl_x11.h"  // window creation
#include "X11/Xlib.h" // keyboard codes
#include "rgb_processing.h"
#include "vector_ops.h"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtc/type_ptr.hpp"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define SAMPLE_XML_PATH "SamplesConfig.xml"
#define RECORDING_FILE_NAME "Captured.oni"
#define RAW_NODE_NAME "Recorder1"

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
	#include <conio.h>
	#define XN_KB_HIT _kbhit()
#else
	#define XN_KB_HIT FALSE
#endif

#define CHECK_RC(rc, what)										\
	if (rc != XN_STATUS_OK)										\
	{												\
		printf("%s failed: %s\n", what, xnGetStatusString(rc));					\
		return rc;										\
	}
#define _USE_MATH_DEFINES

int SENSOR=1;
int DEBUG=0; // texture
int debugOptions=8;
int avg, lastAvg;

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

using namespace xn;


long double firstTime, lastTime, currentTime;
int nbFrames=0;
unsigned int mapX =640; 
unsigned int mapY = 480;
static const size_t pixelCount = mapX * mapY *3;
unsigned int screenWidth, screenHeight;
void * pixels, *pixConv, *rendered;
static const size_t pixelsSize = mapX*mapY*sizeof(XnRGB24Pixel);
int tolerance=1600; // found out empirically

int keyboard;
float  tx=0;//-0.9;
float ty=0;//-0.8,
float tz=1.7f; 
int gridSize=24;
int grids=0;
float zoom=1.7f;
int fps;
int automatic=0;

extern ImageMetaData g_ImageMD;

// texture related
char *tex_buf[3];
char * img_crop; 
GLuint tex[6];
int offset =1920000;

// vertex shader texture related

	// the vertex and fragment shader opengl handles
	GLuint m_uiVertexShader, m_uiFragShader;

	// the program object containing the 2 shader objects
	GLuint m_uiProgramObject;

	// texture handle
	GLuint	m_uiTexture;
	GLuint	m_uiTextureIn;
	GLuint	m_uiFbTexture;

	// vbo handle
	GLuint m_ui32Vbo;

	// framebuffer
	GLuint displayFramebuffer;
	
	// renderbuffer
	GLuint displayRenderbuffer;
	//
	unsigned int m_ui32VertexStride;
	// Index to bind the attributes to vertex shaders
	#define VERTEX_ARRAY	0
	#define TEXCOORD_ARRAY	1

	// Size of the texture we create
	#define TEX_SIZE		128

	EGL_X11 window;	
	RgbProcessing rgbProc;

/*!*********************************************************************************************************************
\brief	Validate if required extension is supported 
\return	Return true if the the extension is supported
\param	extension Extension to validate
***********************************************************************************************************************/
bool isExtensionSupported(const char *extension)
{
	// Copied from https://www.opengl.org/archives/resources/features/OGLextensions/
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	/* Extension names should not have spaces. */
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return false;
	extensions = glGetString(GL_EXTENSIONS);
	/* It takes a bit of care to be fool-proof about parsing the
	 OpenGL extensions string. Don't be fooled by sub-strings,
	 etc. */
	start = extensions;
	for (;;) {
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
		  break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
		  if (*terminator == ' ' || *terminator == '\0')
		    return true;
		start = terminator;
	}
	return false;
}



void cropImage(int width, int height, int sourceWidth, int offset, void * source, void * dest){
		for (int j=0; j<width; j++, offset+=height*3) {
			memcpy(dest+j*height*3, source+offset, width*3); 
			offset += (sourceWidth - width)*3;
		}
}

void cropImageAddAlpha(int width, int height, int sourceWidth, int offset, void * source, void * dest){
		 unsigned char * s = (unsigned char *)source;
		 unsigned char * d = (unsigned char *)dest;

		memset(dest, 0, width*height*4);
		for (int j=0; j<height; j++) {
//			memcpy(dest+j*height*4, source+offset, width*4); 
			for (int i=0; i<width; i++, s+=3, d+=4) {
				memcpy(d, s, 3);				
			}
			s+=(sourceWidth - width)*3;
		}
}

void drawImage(){
		// Clears the color buffer
	gl::UseProgram(m_uiProgramObject); // texture shader
	gl::Clear(GL_COLOR_BUFFER_BIT);

	/*
		Bind the projection model view matrix (PMVMatrix) to
		the associated uniform variable in the shader
	*/

	// Matrix used for projection model view
	float afIdentity[] = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};

	// First gets the location of that variable in the shader using its name
	int i32Location = gl::GetUniformLocation(m_uiProgramObject, "myPMVMatrix");

	// Projection and view matrices
	glm::mat4		m_mProjection, m_mView;
	m_mProjection = glm::perspective(glm::pi<float>()/1.0f, 1.0f, 1.0f, 2000.0f);
	m_mView = glm::lookAt(glm::vec3(tx, ty, zoom), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
//	m_mView = glm::lookAt(glm::vec3(tx, ty, sqrt(zoom*zoom-tx*tx-ty*ty)), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
//printf("looking from (zoom: %2.4f): %2.4f, %2.4f, %2.4f\n", zoom, tx, ty, tz);
/*
	float	m_fAngleX;
	float	m_fAngleY;
	// Rotate the model matrix
	glm::mat4 mModel, mRotX, mRotY;
	mRotX = glm::eulerAngleX(m_fAngleX);
	mRotY = glm::eulerAngleY(m_fAngleY);

	mModel = mRotY * mRotX;

	m_fAngleX = 0; //0.01f;
	m_fAngleY = 0; //0.01f;
*/
	// Set model view projection matrix
	glm::mat4 mModelView, mMVP;
	mModelView = m_mView ;//* mModel;
	mMVP =  m_mProjection * mModelView;

// 	use the shader for the transformations 
//	glUniformMatrix4fv(m_ShaderProgram.auiLoc[eMVPMatrix], 1, GL_FALSE, glm::value_ptr(mMVP));

	// Then passes the matrix to that variable
	gl::UniformMatrix4fv(i32Location, 1, GL_FALSE, glm::value_ptr(mMVP) );


	// Bind the VBO
	gl::BindBuffer(GL_ARRAY_BUFFER, m_ui32Vbo);

	gl::BindTexture(GL_TEXTURE_2D, m_uiTexture);

	int width, height;
	if (DEBUG%debugOptions==1){ 	// depth
		width = height = 512;
		cropImage(width, height, 640, 0, pixels, pixConv);
		gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixConv);
	}		
	else if (DEBUG%debugOptions==2 ){ // rgb
		width = height = 256;		
		cropImage(width, height, 640, 0, pixConv, pixels);
		gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	}
	else				   // map
		gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, img_crop);//tex_buf[0]);//pTexData);

	// Pass the vertex data
	gl::EnableVertexAttribArray(VERTEX_ARRAY);
	gl::VertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, m_ui32VertexStride, 0);

	// Pass the texture coordinates data
	gl::EnableVertexAttribArray(TEXCOORD_ARRAY);
	gl::VertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, m_ui32VertexStride, (void*) (3 * sizeof(GLfloat)));

	// Draws a non-indexed triangle array
	gl::DrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Unbind the VBO
	gl::BindBuffer(GL_ARRAY_BUFFER, 0);

	
}

void handleKey(KeySym keysym, char * buffer)
{
   
   if ((keysym >= XK_space) && (keysym <= XK_asciitilde)){
      printf ("Ascii key:- ");
      printf("%s\n", buffer) ;
	switch (buffer[0]){
	// texture switches
	case 'z': zoom+=0.1;	break;
	case 'Z': zoom-=0.1;	break;
	case 'd': DEBUG++; 	break;
	case 'D': DEBUG--;	break;

	// depth image switches
	case 'g': if (grids) grids = 0; else grids=1; break;
	case 's': gridSize++; 	break;
	case 'S': if (gridSize > 1) gridSize--;	break;
	case 't': tolerance+=200; break;
	case 'T': tolerance-=200; break;
	case 'a': if (automatic) automatic =0; else automatic = 1; break;

	}
	printf("Automatic %d\nZoom %f\nDebug %d\nGrid size %d\nTolerance %d\n", automatic, zoom, DEBUG, gridSize, tolerance);

   }
   else if ((keysym >= XK_Shift_L) && (keysym <= XK_Hyper_R)){
      printf ("modifier key:- ");
      switch (keysym){
      case XK_Shift_L: printf("Left Shift\n"); break;
      case XK_Shift_R: printf("Right Shift\n");break;
      case XK_Control_L: printf("Left Control\n");break;
      case XK_Control_R: printf("Right Control\n");	break;
      case XK_Caps_Lock: printf("Caps Lock\n");	break;
      case XK_Shift_Lock: printf("Shift Lock\n");break;
      case XK_Meta_L: printf("Left Meta\n");	break;
      case XK_Meta_R: printf("Right Meta\n");	break;

      }
    }
   else if ((keysym >= XK_Left) && (keysym <= XK_Down)){
      printf("Arrow Key:-");
      switch(keysym){
      case XK_Left: {tx -= 0.1; };break;
      case XK_Up:    {ty += 0.1;};break;
      case XK_Right: {tx += 0.1;};break;
      case XK_Down: {ty -= 0.1; };break;	
      }
	printf("tx= %f tx= %f \n", tx, ty);
    }

   else if ((keysym == XK_BackSpace) || (keysym == XK_Delete) || (keysym == XK_space) )
	{tx=-0.0;ty=-0.0; zoom=1.8f;}

   else if ((keysym >= XK_KP_0) && (keysym <= XK_KP_9)){
       printf("Number pad key %d\n", keysym -  XK_KP_0);
   }
   else if (keysym == XK_Break || keysym == XK_Escape) {
        printf("closing display\n");
	window.Cleanup();
	exit(0);
 
   }else{
//      printf("Not handled\n");
    }
}

void setOrientation(int direction, int * dirCount, int zoomDiff) {
		if (dirCount[direction]>300) // if 
		switch(direction){
			case 0: { if (ty>-1) ty-=0.07; } break; // down
			case 1: { if (ty<1)  ty+=0.07; } break; // up
			case 2: { if (tx>-1) tx-=0.07; } break; // left
			case 3: { if (tx<1)  tx+=0.07; } break; // right
		}
		else 
		if (zoomDiff<100)	
			if (avg > lastAvg) zoom += float(zoomDiff)*0.009f;	
			else
				if (zoom > 1.5f) zoom -= float(zoomDiff)*0.009f;
}

void setOrientation2(int direction, int *dirCount, int zoomDiff) {
float scaleX = 0.0002;
float scaleY = 0.0007;
float comeback = 0.05;
float maxAngleX = 0.8;
float maxAngleY = 1.0;

if (dirCount[0] > 285 || dirCount[1] > 285){
	if (dirCount[0] < dirCount[1])	{
		if (ty>-maxAngleY) ty-=dirCount[0]*scaleY;}// down
	else
		if (ty<maxAngleY)  ty+=dirCount[1]*scaleY; // up
	}
else
	if (ty<-0.1) ty+=comeback;
	else if (ty>0.1) ty-=comeback;	

if (dirCount[2] > 400 || dirCount[3] > 400){
	if (dirCount[2] > dirCount[3])	{
		if (tx>-maxAngleX) tx-=dirCount[2]*scaleX;} // left
	else
		if (tx<maxAngleX)  tx+=dirCount[3]*scaleX; // right
	}
else
	if (tx<-0.1) tx+=comeback;
	else if (tx>0.1) tx-=comeback;	
/*
if (zoomDiff<10)	
		if (avg > lastAvg) if (zoom < 1.6) zoom += float(zoomDiff)*0.01f;	
		else
			if (zoom > 1.2f) zoom -= float(zoomDiff)*0.01f;
*/
}

void setOrientation3(float x1, float y1, float z1,
		float x2, float y2, float z2, 
		float x3, float y3, float z3)
{
float maxAngleX = 0.707; // 0.707 ~= sqrt(1/2) = sin(45deg) = cos(45deg)
float maxAngleY = 0.707; // 0.707 ~= sqrt(1/2) = sin(45deg) = cos(45deg)



// Get normal
float nx, ny, nz, nx1, ny1, nz1, nx2, ny2, nz2, nx3, ny3, nz3;
normal3pts(x1, y1, z1, x2, y2, z2, x3, y3, z3, &nx1, &ny1, &nz1);
normal3pts(x2, y2, z2, x3, y3, z3, x1, y1, z1, &nx2, &ny2, &nz2);
normal3pts(x3, y3, z3, x1, y1, z1, x2, y2, z2, &nx3, &ny3, &nz3);
nx=(nx1+nx2+nx3)/3;
ny=(ny1+ny2+ny3)/3;
nz=(nz1+nz2+nz3)/3;
// calculate angles between normal and x and y axes, respectively
// since the normal is a unit vector, and x and y axes vectors, too
// the respective component from the normal will be the 
// cosine of the angle 
float tx_tmp, ty_tmp;
if (abs(nx) > maxAngleX)
{
	tx_tmp = maxAngleX;
}
else
{
	tx_tmp = nx; 
}

if (abs(ny) > maxAngleY)
{
	ty_tmp = maxAngleY;
}
else
{
	ty_tmp = ny; 
}
tx = zoom*tx_tmp;
ty = zoom*ty_tmp;
tz = zoom*sqrt(1 - tx_tmp*tx_tmp - ty_tmp*ty_tmp);

//printf("Normal: %1.f, %1.f, %1.f\n", nx, ny, nz);
/*
if (zoomDiff<10)	
		if (avg > lastAvg) if (zoom < 1.6) zoom += float(zoomDiff)*0.01f;	
		else
			if (zoom > 1.2f) zoom -= float(zoomDiff)*0.01f;
*/
}


void update()
{
   char buf[80];
   int direction;
   int dirCount[4];
   KeySym key = window.getKey(buf);
   handleKey(key, buf);
	
   if (SENSOR) {
  	 readFrame();

//   avg =  openni_process (pixels, 640, 480); // show normal depth map
	if (DEBUG%debugOptions==0 || DEBUG%debugOptions==1) // map and depth 
		avg = openni_process_grid (pixels, 640, 480, grids, gridSize, tolerance, &direction, dirCount ); // show grid
	 
	openni_rgb (pixConv, 640, 480, gridSize); // show grid
	
	if (automatic) { // change zoom, tx, ty 
		int zoomDiff = abs(lastAvg-avg);
//		setOrientation3(1.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		XnRGB24Pixel * p = ( XnRGB24Pixel *) pixels;
		
		const int x1 = 10;
		const int y1 = 140;
		const int x2 = 500;
		const int y2 = 140;
		const int x3 = 255;
		const int y3 = 340;
		const int offset1 = x1+y1*640;
		const int offset2 = x2+y2*640;
		const int offset3 = x3+y3*640;

#ifndef NORMAL_FILTER_DEPTH

		float z1 = ((offset1+p)->nRed) ;
		float z2 = ((offset2+p)->nRed );
		float z3 = ((offset3+p)->nRed );

#else
/*		const int left1 = offset1-1;
		const int right1 = offset1+1;
		const int up1 = offset1-640;
		const int down1 = offset1 + 640;
		const int upleft1 = up1-1;
		const int upright1 = up1+1;
		const int downleft1 = down1-1;
		const int downright1 = down1+1;

		const int left2 = offset2-1;
		const int right2 = offset2+1;
		const int up2 = offset2-640;
		const int down2 = offset2 + 640;
		const int upleft2 = up2-1;
		const int upright2 = up2+1;
		const int downleft2 = down2-1;
		const int downright2 = down2+1;


		const int left3 = offset3-1;
		const int right3 = offset3+1;
		const int up3 = offset3-640;
		const int down3 = offset3 + 640;
		const int upleft3 = up3-1;
		const int upright3 = up3+1;
		const int downleft3 = down3-1;
		const int downright3 = down3+1;



		float z1 = (((float)(p+offset1)->nRed) + (p+left1)->nRed + (p+right1)->nRed 
				+ (p+up1)->nRed + (p+upleft1)->nRed + (p+upright1)->nRed
				+ (p+down1)->nRed + (p+downleft1)->nRed + (p+downright1)->nRed)/9;
		float z2 = (((float)(p+offset2)->nRed) + (p+left2)->nRed + (p+right2)->nRed 
				+ (p+up2)->nRed + (p+upleft2)->nRed + (p+upright2)->nRed
				+ (p+down2)->nRed + (p+downleft2)->nRed + (p+downright2)->nRed)/9;
		float z3 = (((float)(p+offset3)->nRed) + (p+left3)->nRed + (p+right3)->nRed 
				+ (p+up3)->nRed + (p+upleft3)->nRed + (p+upright3)->nRed
				+ (p+down3)->nRed + (p+downleft3)->nRed + (p+downright3)->nRed)/9 ;
*/
		const int n_patch_height = 10;
		const int n_patch_width = 10;
		const int patch_upleft1 = offset1 - (n_patch_width/2) - (n_patch_height/2)*640;
		const int patch_upleft2 = offset2 - (n_patch_width/2) - (n_patch_height/2)*640;
		const int patch_upleft3 = offset3 - (n_patch_width/2) - (n_patch_height/2)*640;
		XnRGB24Pixel *p1=p+patch_upleft1, *p2=p+patch_upleft2, *p3=p+patch_upleft3;

		float z1=0, z2=0, z3=0;
		for (int i=0; i < n_patch_height; i++)
		{
			for (int j=0; j < n_patch_width; j++)
			{
				z1 += p1->nRed;
				z2 += p2->nRed;
				z3 += p3->nRed;	
				p1++;
				p2++;
				p3++;
			}
		}
		z1/=n_patch_height*n_patch_width;
		z2/=n_patch_height*n_patch_width;
		z3/=n_patch_height*n_patch_width;	
#endif /* NORMAL_FILTER_DEPTH */		
		
/*             
        	XnRGB24Pixel *p1 = p+10+140*640;
		XnRGB24Pixel *p2 = p+500+140*640;
		XnRGB24Pixel *p3 = p+320+300*640;

                float z1 = (p1->nRed + (p1+1)->nRed + (p1-1)->nRed)/3;
                float z2 = (p1->nRed + (p2+1)->nRed + (p2-1)->nRed)/3;
                float z3 = (p3->nRed + (p3+1)->nRed + (p3-1)->nRed)/3;
*/
setOrientation3(x3/640.0, y3/480.0,z3 / 225.0,
		x1/640.0, y1/480.0,  z1 / 225.0 , 
		x2/640.0, y2/480.0, z2 / 225.0);
//	setOrientation3(0.0, 0.0, 0.0,
//			1.0, 0.0, 0.0,
//			0.0, 1.0, 0.0);
//		setOrientation2(direction, dirCount, zoomDiff);
//		printf("%d \n", dirCount[direction]);
	}


   } // endif SENSOR

/*
	char *it;
	it = tex_buf[0];
	if (offset > 5000000) 
		offset =1920000;
	else
		offset += 3900;
	

	for (int j=0; j<480; j++, offset+=1920) {
		memcpy(img_crop+j*1920, it+offset, 1920); // 1920 = 640 x 3
		offset += 1980; //660*3; // picture resx = 1300, 980 = 1300 - 320, 660 = 130 - 640
	}
*/
	

   char msg[100];
   currentTime = time(0);
   nbFrames++;
   if (difftime(currentTime, lastTime) >= 1.0f)
   {
      getCaptureMessage(msg);

      fps = nbFrames;
      printf("fps %d %s \n", nbFrames, msg );
        		nbFrames = 0;
			lastTime = time(0);
			if (difftime(currentTime, firstTime) >= 600.0f) ;
				;//exit(0);

	printf ("\n    avg:    %d    %d\n\n", avg, lastAvg);
	lastAvg = avg;
   }

}

void intHandler(int foo) {
	char msg[100];

	// Frees the OpenGL handles for the program and the 2 shaders
	gl::DeleteProgram(m_uiProgramObject);
	gl::DeleteShader(m_uiFragShader);
	gl::DeleteShader(m_uiVertexShader);

	// Delete the VBO as it is no longer needed
	gl::DeleteBuffers(1, &m_ui32Vbo);


	window.Cleanup();
	captureStop(1); 
//	GLUTHelper::LeaveLoop();
	getCaptureMessage(msg);	printf("%s \n",msg);
}


void loadTextures()
{
	// Fragment and vertex shaders code
	const char* pszFragShader = "\
		uniform sampler2D sampler2d;\
		varying mediump vec2	myTexCoord;\
		void main (void)\
		{\
		    gl_FragColor = texture2D(sampler2d,myTexCoord);\
		}";
	const char* pszVertShader = "\
		attribute highp vec4	myVertex;\
		attribute mediump vec4	myUV;\
		uniform mediump mat4	myPMVMatrix;\
		varying mediump vec2	myTexCoord;\
		void main(void)\
		{\
			gl_Position = myPMVMatrix * myVertex;\
			myTexCoord = myUV.st*vec2(1.0, -1.0); \
			/* mirror texture - multiply with vec2 */\
		}";

	// Create the fragment shader object
	m_uiFragShader = gl::CreateShader(GL_FRAGMENT_SHADER);

	// Load the source code into it
	gl::ShaderSource(m_uiFragShader, 1, (const char**)&pszFragShader, NULL);

	// Compile the source code
	gl::CompileShader(m_uiFragShader);

	// Check if compilation succeeded
	GLint bShaderCompiled;
    gl::GetShaderiv(m_uiFragShader, GL_COMPILE_STATUS, &bShaderCompiled);
	if (!bShaderCompiled)
	{
		// An error happened, first retrieve the length of the log message
		int i32InfoLogLength, i32CharsWritten;
		gl::GetShaderiv(m_uiFragShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);

		// Allocate enough space for the message and retrieve it
		char* pszInfoLog = new char[i32InfoLogLength];
        gl::GetShaderInfoLog(m_uiFragShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);

		/*
			Displays the message in a dialog box when the application quits
			using the shell PVRShellSet function with first parameter prefExitMessage.
		*/
		char* pszMsg = new char[i32InfoLogLength+256];
		strcpy(pszMsg, "Failed to compile fragment shader: ");
		strcat(pszMsg, pszInfoLog);
		printf("\n%s\n", pszMsg);

		delete [] pszMsg;
		delete [] pszInfoLog;
	}

	// Loads the vertex shader in the same way
	m_uiVertexShader = gl::CreateShader(GL_VERTEX_SHADER);
	gl::ShaderSource(m_uiVertexShader, 1, (const char**)&pszVertShader, NULL);
	gl::CompileShader(m_uiVertexShader);
    gl::GetShaderiv(m_uiVertexShader, GL_COMPILE_STATUS, &bShaderCompiled);
	if (!bShaderCompiled)
	{
		int i32InfoLogLength, i32CharsWritten;
		gl::GetShaderiv(m_uiVertexShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
        gl::GetShaderInfoLog(m_uiVertexShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		char* pszMsg = new char[i32InfoLogLength+256];
		strcpy(pszMsg, "Failed to compile vertex shader: ");
		strcat(pszMsg, pszInfoLog);

		delete [] pszMsg;
		delete [] pszInfoLog;
	}

	// Create the shader program
    m_uiProgramObject = gl::CreateProgram();

	// Attach the fragment and vertex shaders to it
    gl::AttachShader(m_uiProgramObject, m_uiFragShader);
    gl::AttachShader(m_uiProgramObject, m_uiVertexShader);

	// Bind the custom vertex attribute "myVertex" to location VERTEX_ARRAY
    gl::BindAttribLocation(m_uiProgramObject, VERTEX_ARRAY, "myVertex");
	// Bind the custom vertex attribute "myUV" to location TEXCOORD_ARRAY
    gl::BindAttribLocation(m_uiProgramObject, TEXCOORD_ARRAY, "myUV");

	// Link the program
    gl::LinkProgram(m_uiProgramObject);

	// Check if linking succeeded in the same way we checked for compilation success
    GLint bLinked;
    gl::GetProgramiv(m_uiProgramObject, GL_LINK_STATUS, &bLinked);

	if (!bLinked)
	{
		int i32InfoLogLength, i32CharsWritten;
		gl::GetProgramiv(m_uiProgramObject, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
		gl::GetProgramInfoLog(m_uiProgramObject, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		
		char* pszMsg = new char[i32InfoLogLength+256];
		strcpy(pszMsg, "Failed to link program: ");
		strcat(pszMsg, pszInfoLog);
		printf("\n%s\n", pszMsg);
		delete [] pszMsg;
		delete [] pszInfoLog;
	}

	// Actually use the created program
	gl::UseProgram(m_uiProgramObject);

	// Sets the sampler2D variable to the first texture unit
	gl::Uniform1i(gl::GetUniformLocation(m_uiProgramObject, "sampler2d"), 0);

	// Sets the clear color
//	glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // light blue
	gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f); // black



	// will be used in the shaders
//	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 512);
//	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, 512);

	// tectures
	gl::GenTextures(1, &m_uiTexture);
	gl::GenTextures(1, &m_uiTextureIn);
	gl::BindTexture(GL_TEXTURE_2D, m_uiTexture);
	gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);//pTexData);
	gl::TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	gl::TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	// frame buffer
	gl::GenFramebuffers(2, &displayFramebuffer);
	gl::GenRenderbuffers(2, &displayRenderbuffer);
	gl::BindRenderbuffer(GL_RENDERBUFFER, displayRenderbuffer);
	gl::BindFramebuffer(GL_FRAMEBUFFER, displayFramebuffer);

	gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, displayRenderbuffer);
	gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_EXT , GL_TEXTURE_2D, m_uiTexture, 0);

	// setup rendertargets 
	GLenum attachments[2] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
	//if (!CPVRTgles2Ext::IsGLExtensionSupported("GL_EXT_draw_buffers"))
	//	printf( "ERROR: GL_EXT_draw_buffers extension is required to run this example.\n");

	if (!isExtensionSupported("GL_EXT_draw_buffers"))
		printf( "ERROR: GL_EXT_draw_buffers extension is required to run this example.");

	glext::DrawBuffersEXT(1, attachments);

	gl::BindTexture(GL_TEXTURE_2D, 0);
	gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
	gl::BindRenderbuffer(GL_RENDERBUFFER, 0);

	// Allocates one texture handle

	gl::BindTexture(GL_TEXTURE_2D, m_uiTexture);

///////////////////// load texture from file

   FILE *tex_file[3];
   int bytes_read, image_sz = 1300*1500*3; // enter resolution manually for now
//   const char* filenames[] = {"img/collage.txt"};
   const char* filenames[] = {"img/collage.txt"};

   for (int i=0; i<1; i++) // only one file
   {

   	tex_buf[i] = (char *)malloc(image_sz);
   	tex_file[i] = fopen( filenames[i], "rb");	
	assert(tex_file[i]!=NULL);
	
	if (tex_file[i] && tex_buf[i])
   	{
		bytes_read=fread(tex_buf[i], 1, image_sz, tex_file[i]);
		//      assert(bytes_read == image_sz);  // some problem with file?
		fclose(tex_file[i]);
   	}
   }

	img_crop = (char *)malloc(640*480*3);
	int offset=1920000;
	char *it;
	int width=256;
	int height=256;
	it = tex_buf[0];
	for (int j=0; j<width; j++, offset+=height*3) {
		memcpy(img_crop+j*width*3, it+offset, width*3); // 1920 = 640 x 3
		offset += (1300 - width)*3; // 1980 = 660*3; // picture resx = 1300, 980 = 1300 - 320, 660 = 1300 - 640
	}


///////////////////// load texture from file

	// Creates the data as a 32bits integer array (8bits per component)
	GLuint* pTexData = new GLuint[TEX_SIZE*TEX_SIZE];
	for (int i=0; i<TEX_SIZE; i++)
	for (int j=0; j<TEX_SIZE; j++)
	{
		// Fills the data with a fancy pattern
		GLuint col = (255<<24) + ((255-j*2)<<16) + ((255-i)<<8) + (255-i*2);
		if ( ((i*j)/8) % 2 ) col = (GLuint) (255<<24) + (255<<16) + (0<<8) + (255);
		pTexData[j*TEX_SIZE+i] = col;
	}

//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pTexData);

//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // attempt to have non-square textures :)
	gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_crop);//pTexData);

	// Non power of two texture resolutions - DIDN't WORK
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Deletes the texture data, it's now in OpenGL memory
	delete [] pTexData;

	// Create VBO for the triangle from our data

	// Interleaved vertex data
	GLfloat afVertices[] = {-0.4f,-0.4f,0.0f, // Pos
				 0.0f,0.0f ,	  // UVs - texture coord
				 0.4f,-0.4f,0.0f,
				 1.0f,0.0f ,
				-0.4f,0.4f ,0.0f,
				 0.0f,1.0f,
				 0.4f,0.4f ,0.0f,
				 1.0f,1.0f};

	gl::GenBuffers(1, &m_ui32Vbo);

	m_ui32VertexStride = 5 * sizeof(GLfloat); // 3 floats for the pos, 2 for the UVs

	// Bind the VBO
	gl::BindBuffer(GL_ARRAY_BUFFER, m_ui32Vbo);

	// Set the buffer's data
	gl::BufferData(GL_ARRAY_BUFFER, 4 * m_ui32VertexStride, afVertices, GL_DYNAMIC_DRAW);


	// Unbind the VBO
	gl::BindBuffer(GL_ARRAY_BUFFER, 0);

	// Enable culling
	gl::Enable(GL_CULL_FACE);
}


void graphicsInit()
{
	gl::initGl();
	glext::initGlext();
	loadTextures();
	rgbProc.SetDefaultFrameBuffer(displayFramebuffer);
	rgbProc.SetDefaultRenderBuffer(displayRenderbuffer);
	rgbProc.SetDefaultShaderProgram(m_uiProgramObject);
	rgbProc.SetDefaultTexture(m_uiFbTexture);
	rgbProc.Init();

}

void xtionInit(const char* oniFile) {

	XnStatus nRetVal = XN_STATUS_OK;
	nRetVal = xnLogInitFromXmlFile(SAMPLE_XML_PATH);
	if (nRetVal != XN_STATUS_OK)
	{
		printf("Log couldn't be opened: %s. Running without log", xnGetStatusString(nRetVal));
	}

	if (oniFile != NULL)
	{
		printf("Opening oni file: %s\n", oniFile);
		XnStatus rc = openDeviceFile(oniFile);
		if (rc != XN_STATUS_OK)
		{
		        printf("Open oni file failed: %s\n", xnGetStatusString(rc));
		}
	}
	else
	{

	       // Xtion Init
		XnStatus rc = XN_STATUS_OK;
		EnumerationErrors errors;

		rc = openDeviceFromXml(SAMPLE_XML_PATH, errors);
		if (rc == XN_STATUS_NO_NODE_PRESENT)
		{
		        XnChar strError[1024];
		        errors.ToString(strError, 1024);
		        printf("%s\n", strError);
		}
		else if (rc != XN_STATUS_OK)
		{
		        printf("Open failed: %s\n", xnGetStatusString(rc));
		}

	//	setImageResolution( 1); // 0 = QVGA (default), 1 = VGA, 2 = SXVGA
	}
}

static const char* oniFile = "/media/sf_Data/seahorse_2.oni";

int main(int argc, char* argv[])
{
   	signal(SIGINT, intHandler);
	xtionInit(oniFile);
	window.Init();
	graphicsInit();
	lastTime = time(0); // for computing fps
	firstTime = time(0);

        mapX = (((unsigned short)(mapX-1) / 512) + 1) * 512;
        mapY = (((unsigned short)(mapY-5) / 512) + 1) * 512;
	pixels = (unsigned char *) malloc( mapX * mapY * 4);
	pixConv = (unsigned char *) malloc( mapX * mapY * 4);
	rendered = (unsigned char *) malloc( mapX * mapY * 4);
	int hist[255];
	int width= 256;
	int height=256;

	while(1) {
		update();
		if ( DEBUG%debugOptions==4  ){ // grayscale
			cropImage(256, 256, 640, 0, pixConv, pixels);
			rgbProc.grayscaleShader(pixels, pixels, 256, 256, displayFramebuffer, m_uiTexture, GL_RGB,m_ui32VertexStride, tx, ty, zoom);
		} 
		else if ( DEBUG%debugOptions==3  ){ // 2 shaders, first with read image and giving it to the other
			cropImage(width, height, 640, 0, pixConv, pixels);
//			rgbProc.threshColorShader(pixels, rendered, width, height, displayFramebuffer, m_uiTexture, GL_RGBA, m_ui32VertexStride, tx, ty, zoom);
//			rgbProc.processGrid(rendered, width*4, height*4, grids, gridSize, tolerance);

			gl::BindTexture(GL_TEXTURE_2D, m_uiTextureIn);
			gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
			gl::TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			gl::TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );			
			gl::BindTexture(GL_TEXTURE_2D, 0);
			rgbProc.threshColorShader(pixels, rendered, width, height, displayFramebuffer, m_uiTexture, GL_RGB, m_ui32VertexStride, tx, ty, zoom);

		}	
		else if ( DEBUG%debugOptions==5  ){ // edge
			cropImage(256, 256, 640, 0, pixConv, pixels);
			gl::BindTexture(GL_TEXTURE_2D, m_uiTextureIn);
			gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
			gl::BindTexture(GL_TEXTURE_2D, 0);
		rgbProc.hueShader(pixels, pixels, 256, 256, displayFramebuffer, m_uiTexture, GL_RGB, m_ui32VertexStride, tx, ty, zoom);
		} 
		else if ( DEBUG%debugOptions==6  ){ // contrast
			cropImage(256, 256, 640, 0, pixConv, pixels);
			gl::BindTexture(GL_TEXTURE_2D, m_uiTextureIn);
			gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
			gl::BindTexture(GL_TEXTURE_2D, 0);
		
			rgbProc.edgeShader(pixels, pixConv, 256, 256, displayFramebuffer, m_uiTexture,GL_RGB, m_ui32VertexStride, tx, ty, zoom); // get map colors
//			rgbProc.contrastShader(pixels, pixConv, 256, 256, m_ui32Vbo, m_uiTexture, m_ui32VertexStride, tx, ty, zoom); // get map colors
		
		} 
		else if ( DEBUG%debugOptions==7  ){ // brightness
			cropImage(256, 256, 640, 0, pixConv, pixels);
			gl::BindTexture(GL_TEXTURE_2D, m_uiTextureIn);
			gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
			gl::BindTexture(GL_TEXTURE_2D, 0);
		rgbProc.contrastShader(pixels, pixels, 256, 256, displayFramebuffer, m_uiTexture, GL_RGB,m_ui32VertexStride, tx, ty, zoom);
		} 
		else
			drawImage();	
	
		window.WriteText(0.3f, 7.5f, 0.75f, 0xFFFFFFFF, "fps %d", fps);
		window.SwapBuffers();
//		rgbProc.Histogram(pixConv, 640, 480, 3, hist, 255, RgbProcessing::HISTO_RED);

	}

	return 0;
}
