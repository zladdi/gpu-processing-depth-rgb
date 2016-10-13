#include "cvglsl.h"

//
//  Read text file
//
static char* ReadText(const char *file)
{
   int   n;
   char* buffer;
   //  Open file
   FILE* f = fopen(file,"rt");
   if (!f) printf("Cannot open text file %s\n",file);
   //  Seek to end to determine size, then rewind
   fseek(f,0,SEEK_END);
   n = ftell(f);
   rewind(f);
   //  Allocate memory for the whole file
   buffer = (char*)malloc(n+1);
   if (!buffer) printf("Cannot allocate %d bytes for text file %s\n",n+1,file);
   //  Snarf the file
   if (fread(buffer,n,1,f)!=1) printf("Cannot read %d bytes for text file %s\n",n+1,file);
   buffer[n] = 0;
   //  Close and return
   fclose(f);
   return buffer;
}

//
//  Print Shader Log
//
static void PrintShaderLog(int obj,const char* file)
{
   int len=0;
   gl::GetShaderiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) printf("Cannot allocate %d bytes of text for shader log\n",len);
      gl::GetShaderInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s:\n%s\n",file,buffer);
      free(buffer);
   }
   gl::GetShaderiv(obj,GL_COMPILE_STATUS,&len);
   if (!len) printf("Error compiling %s\n",file);
}

//
//  Print Program Log
//
static void PrintProgramLog(int obj)
{
   int len=0;
   gl::GetProgramiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) printf("Cannot allocate %d bytes of text for program log\n",len);
      gl::GetProgramInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s\n",buffer);
   }
   gl::GetProgramiv(obj,GL_LINK_STATUS,&len);
   if (!len) printf("Error linking program\n");
}

//
//  Create Shader
//
static int CreateShader(GLenum type, const char* file)
{
   //  Create the shader
   int shader = gl::CreateShader(type);
   //  Load source code from file
   char* source = ReadText(file);
   gl::ShaderSource(shader,1,(const char**)&source,NULL);
   free(source);
   //  Compile the shader
   gl::CompileShader(shader);
   //  Check for errors
   PrintShaderLog(shader,file);
   //  Return name
   return shader;
}

//
//  Create Shader Program
//
int CreateShaderProg(const char* VertFile, const char* FragFile, GLuint& prog)
{
   //  Create program
   prog = gl::CreateProgram();
   //  Create and compile vertex shader
   if (VertFile)
   {
      int vert = CreateShader(GL_VERTEX_SHADER,VertFile);
      gl::AttachShader(prog,vert);
   }
   //  Create and compile fragment shader
   if (FragFile)
   {
      int frag = CreateShader(GL_FRAGMENT_SHADER,FragFile);
      gl::AttachShader(prog,frag);
   }

// 	Make it general, so don't add any attributes
//	glBindAttribLocation(prog, 0, "position");


   //  Link program
   gl::LinkProgram(prog);
   //  Check for errors
   PrintProgramLog(prog);
   //  Return name
   return prog;
}

//
//  Create Shader Program including Geometry Shader
//
int CreateShaderProgGeom(char* VertFile,char* GeomFile,char* FragFile,int in,int out,int n)
{
   //  Create program
   int prog = gl::CreateProgram();
   //  Create and compile vertex shader
   if (VertFile)
   {
      int vert = CreateShader(GL_VERTEX_SHADER,VertFile);
      gl::AttachShader(prog,vert);
   }
   //  Create and compile geometry shader
   //  Also set geometry parameters
   if (GeomFile)
   {
#ifdef GL_GEOMETRY_SHADER_EXT
      int geom = CreateShader(GL_GEOMETRY_SHADER_EXT,GeomFile);
      gl::AttachShader(prog,geom);
      glext::ProgramParameteriEXT(prog,GL_GEOMETRY_LINKED_INPUT_TYPE_EXT,in);
      glext::ProgramParameteriEXT(prog,GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT,out);
      glext::ProgramParameteriEXT(prog,GL_GEOMETRY_LINKED_VERTICES_OUT_EXT,n);
#else
      printf("Geometry shaders not supported\n");
#endif
   }
   //  Create and compile fragment shader
   if (FragFile)
   {
      int frag = CreateShader(GL_FRAGMENT_SHADER,FragFile);
      gl::AttachShader(prog,frag);
   }
   //  Link program
   gl::LinkProgram(prog);
   //  Check for errors
   PrintProgramLog(prog);
   //  Return name
   return prog;
}


