/*
http://www.blog.nathanhaze.com/glsl-edge-detection/
*/

attribute highp vec4	myVertex;
attribute mediump vec4	myUV;
uniform mediump mat4	myPMVMatrix;
varying mediump vec2	myTexCoord;
varying vec3 colorFactor;
varying vec2 texc;

varying vec2 left_coord;
varying vec2 right_coord;
varying vec2 above_coord;
varying vec2 below_coord;
float d = 0.001;

void main()
{
	gl_Position = myPMVMatrix * myVertex;
	texc = myUV.st*vec2(1,-1); 
/*gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
texc = vec2(gl_MultiTexCoord0); */

left_coord = texc.xy + vec2(-d , 0);
right_coord = texc.xy + vec2(d , 0);
above_coord = texc.xy + vec2(0,d);
below_coord = texc.xy + vec2(0,-d);

}

