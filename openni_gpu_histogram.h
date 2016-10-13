#ifndef OPENNI_DEVICE_GPU_HISTOGRAM_H
#define OPENNI_DEVICE_GPU_HISTOGRAM_H

#include "GLES2/gl2.h"

typedef struct
{
  int inXRes;
  int inYRes;
  int outXRes;
  int outYRes;	 
  GLuint shaderProgram;
  bool shaderProgramLoaded;
  GLuint fbo;
  GLuint fboTex;
} hist_state_t;

bool openni_gpu_histogram_init(hist_state_t* histState, int inXRes, int inYRes, int inMaxVal);
void openni_gpu_histogram_calculate(hist_state_t* histState, const void* pDepth, float depthHist[]);
void openni_gpu_histogram_close(hist_state_t* histState);

#endif /* OPENNI_DEVICE_GPU_HISTOGRAM_H */
