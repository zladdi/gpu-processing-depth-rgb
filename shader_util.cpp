
#include "shader_util.h"
#include <iostream>
#include <fstream>
#include <errno.h>
#include <string.h>

#if defined(DEBUG)
#include <stdlib.h>
#include <stdio.h>
#endif

using namespace std;

void printShaderInfoLog(GLuint obj)
	{
	    int infologLength = 0;
	    int charsWritten  = 0;
	    char *infoLog;

		gl::GetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	    if (infologLength > 0)
	    {
	        infoLog = new char [infologLength];
	        gl::GetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
			printf("InfoLog %s\n",infoLog);
	        delete(infoLog);
	    }
	    else
		printf("InfoLog empty\n");
	}
 void PrintShaderLog(int obj,const char* file)
{
   int len=0;
   gl::GetShaderiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = new char[len];
      if (!buffer) printf("Cannot allocate %d bytes of text for shader log\n",len);
      gl::GetShaderInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s:\n%s\n",file,buffer);
      delete buffer;
   }
   gl::GetShaderiv(obj,GL_COMPILE_STATUS,&len);
   if (!len) printf("Error compiling %s\n",file);
}


unsigned long getFileLength(ifstream& file)
{   
	if(!file.good()) 
	{
		return 0;
	}
	unsigned long pos=file.tellg();
	file.seekg(0,ios::end);
	unsigned long len = file.tellg();
	file.seekg(ios::beg);
	return len;
}


int loadShaderSource(const char* filename, GLchar** shaderSource, unsigned long* len)
{
	ifstream file;
	file.open(filename, ios::in); // opens as ASCII!
	
	if(!file)
	{
		printf("Could not open file '%s': %s\n", filename, strerror(errno));
                return -1;
	}	     
	
	*len = getFileLength(file);
		        
	if (*len==0) 
	{
		file.close();
		return -2;   // Error: Empty File 
	}  
	*shaderSource = (GLchar*) new char[*len+1];
	if (*shaderSource == 0) {
		file.close();
		return -3;   // can't reserve memory
	}

	// len isn't always strlen cause some characters are stripped in ascii read...
	// it is important to 0-terminate the real length later, len is just max possible value... 
	(*shaderSource)[*len] = 0; 
	unsigned int i=0;	

	while (file.good())
	{
		(*shaderSource)[i] = file.get();       // get character from file.	
		if (!file.eof())
		{
			i++;
		}	
	}
	
	(*shaderSource)[i] = 0;  // 0-terminate it at the correct position
	file.close();
	return 0; // No Error
}


int unloadShaderSource(GLchar** shaderSource)
{
	   if (*shaderSource != 0) 
	   {
	
		   delete[] *shaderSource;
	   } 
	   *shaderSource = 0;
	   return 0;
}


bool linkProgram(GLuint prog)
{
	GLint status;
	gl::LinkProgram(prog);

#if defined(DEBUG)
	GLint logLength;
	gl::GetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		GLchar *log = (GLchar *)malloc(logLength);
		gl::GetProgramInfoLog(prog, logLength, &logLength, log);
		printf("Program link log:\n%s", log);
		free(log);
	}
#endif

	gl::GetProgramiv(prog, GL_LINK_STATUS, &status);
	
	if (status == 0)
	{
		return false;
	}
	
	return true;
}


bool compileShader(GLuint* shader, GLenum type, const char* filename)
{
	GLint status;
	GLchar *source;
	unsigned long sourceLen;
	if (loadShaderSource(filename, &source, &sourceLen) != 0)
	{
		printf("Failed to load shader %s\n", filename); 
		return false;
        }
	*shader = gl::CreateShader(type);
	gl::ShaderSource(*shader, 1, const_cast<const GLchar**>(&source), NULL);
	gl::CompileShader(*shader);
	
	unloadShaderSource(&source);

#if defined(DEBUG)
	GLint logLength;
	gl::GetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		GLchar *log = (GLchar *)malloc(logLength);
		gl::GetShaderInfoLog(*shader, logLength, &logLength, log);
		printf("Shader compile log:\n%s", log);
		free(log);
	}
#endif

	gl::GetShaderiv(*shader, GL_COMPILE_STATUS, &status);
	
	if (status == 0)
	{
		gl::DeleteShader(*shader);
		printf("Shader file '%s' failed to compile\n", filename);
		return false;
	}
	return true;
}


bool loadShaders(const char* vertShaderFilename, const char* fragShaderFilename, GLuint* program)
{
	GLuint vertShader, fragShader;

	// Create shader program
	*program = gl::CreateProgram();

	// Create and compile vertex shader
	if (!compileShader(&vertShader, GL_VERTEX_SHADER, vertShaderFilename))
        {
	        printf("Failed to compile vertex shader %s\n", vertShaderFilename);
//		printShaderInfoLog(vertShader);
 		PrintShaderLog(vertShader, vertShaderFilename);

		return false;
	}

	// Create and compile fragment shader
	if (!compileShader(&fragShader, GL_FRAGMENT_SHADER, fragShaderFilename))
	{
		printf("Failed to compile fragment shader %s\n", fragShaderFilename);
//		printShaderInfoLog(fragShader);
 		PrintShaderLog(fragShader, fragShaderFilename);

		return false;
	}

	// Attach vertex shader to program
	gl::AttachShader(*program, vertShader);

	// Attach fragment shader to program
	gl::AttachShader(*program, fragShader);

	// Bind attribute locations
	// this needs to be done prior to linking
	gl::BindAttribLocation(*program, ATTRIB_POSITION, "position");
//	glBindAttribLocation(program, ATTRIB_TEXTUREPOSITION, "inputTextureCoordinate");

	// Link program
	if (!linkProgram(*program))
	{
		printf("Failed to link program: %d\n", *program);
		if (vertShader)
		{
			gl::DeleteShader(vertShader);
			vertShader = 0;
		}
		if (fragShader)
		{
		 	gl::DeleteShader(fragShader);
			fragShader = 0;
		}
		if (*program)
		{
			gl::DeleteProgram(*program);
			program = 0;
		}
		return false;
	}

	// Get uniform locations
//	uniforms[UNIFORM_MODELVIEWMATRIX] = glGetUniformLocation(program, "modelViewProjMatrix");
//	uniforms[UNIFORM_TEXTURE] = glGetUniformLocation(program, "texture");

	// Release vertex and fragment shaders
	if (vertShader)
	{
		gl::DeleteShader(vertShader);
	}	
	if (fragShader)
	{
		gl::DeleteShader(fragShader);
	}
	return true;


}

void createShaderProgram(const GLchar* vertSrc, const GLchar* fragSrc,GLuint& shaderProgram)
{
    GLuint vertexShader;
    GLuint fragmentShader;
    GLint status;

    vertexShader = gl::CreateShader(GL_VERTEX_SHADER);
    gl::ShaderSource(vertexShader, 1, &vertSrc, NULL);
    gl::CompileShader(vertexShader);
    gl::GetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);	
    if (status == 0)
	{
		gl::DeleteShader(vertexShader);
		printf("Shader file '%s' failed to compile\n", vertSrc);
	}

    // Create and compile the fragment shader
    fragmentShader = gl::CreateShader(GL_FRAGMENT_SHADER);
    gl::ShaderSource(fragmentShader, 1, &fragSrc, NULL);
    gl::CompileShader(fragmentShader);

    gl::GetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);	
    if (status == 0)
	{
		gl::DeleteShader(fragmentShader);
		printf("Shader file '%s' failed to compile\n", fragSrc);
	}
	
    // Link the vertex and fragment shader into a shader program
    shaderProgram = gl::CreateProgram();
    gl::AttachShader(shaderProgram, vertexShader);
    gl::AttachShader(shaderProgram, fragmentShader);
/*    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
*/
}
