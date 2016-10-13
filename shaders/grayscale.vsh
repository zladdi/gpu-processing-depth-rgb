attribute highp vec4	myVertex;
attribute mediump vec4	myUV;
uniform mediump mat4	myPMVMatrix;
varying mediump vec2	myTexCoord;
varying vec3 colorFactor;
void main(void)
{
	lowp vec4 v = myVertex;
	gl_Position = myPMVMatrix * v;
	myTexCoord = myUV.st*vec2(1,-1);   
}

