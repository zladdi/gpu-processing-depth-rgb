attribute highp vec4	myVertex;
attribute mediump vec4	myUV;
uniform mediump mat4	myPMVMatrix;
varying mediump vec2	myTexCoord;
varying vec3 colorFactor;
void main(void)
{
	gl_Position = myPMVMatrix * myVertex;
	myTexCoord = myUV.st*vec2(1,-1); 
}
