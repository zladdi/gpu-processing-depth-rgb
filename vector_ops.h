#ifndef VECTOR_OPS_H
#define VECTOR_OPS_H

void normal3pts(float x1, float y1, float z1,
		float x2, float y2, float z2,
                   float x3, float y3, float z3,
                  float* nx, float* ny, float* nz);

 void cross3(float x1, float y1, float z1,
              float x2, float y2, float z2,
              float* cx, float* cy, float* cz);
 
 float norm3(float x, float y, float z);

#endif /* VECTOR_OPS_H */

