
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "PVRNativeApi/OGLES/OpenGLESBindings.h"

#define Cos(th) cos(3.1415926/180*(th))
#define Sin(th) sin(3.1415926/180*(th))
int          CreateShaderProg(const char* VertFile, const char* FragFile, GLuint& program);
int          CreateShaderProgGeom(char* VertFile,char* GeomFile,char* FragFile,int in,int out,int n);


