uniform sampler2D sampler2d;
varying mediump vec2	myTexCoord;
varying lowp vec3 colorFactor; 

void main (void)
{
	gl_FragColor = vec4(texture2D(sampler2d, myTexCoord).rgb, 1.);
}
