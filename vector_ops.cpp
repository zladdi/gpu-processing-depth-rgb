#include "vector_ops.h"
#include <cmath>
 
void normal3pts(float x1, float y1, float z1,
		float x2, float y2, float z2,
		float x3, float y3, float z3,
		float* nx, float* ny, float* nz)
{

	// calculate cross product between two vectors
	cross3((x2-x1), (y2-y1), (z2-z1), 
		(x3-x1), (y3 - y1), (z3 - z1),
		nx, ny, nz);
	// normalize
	float n_norm = norm3(*nx, *ny, *nz);	
	*nx /= n_norm;
	*ny /= n_norm;
	*nz /= n_norm;
}


void cross3(float x1, float y1, float z1,
	    float x2, float y2, float z2, 
	    float* cx, float* cy, float* cz)
{
	*cx = y1*z2 - y2*z1;
	*cy = x2*z1 - x1*z2;
	*cz = x1*y2 - x2*y1; 
}

float norm3(float x, float y, float z)
{
	return sqrt(x*x + y*y + z*z);
}
