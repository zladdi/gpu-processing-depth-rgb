uniform sampler2D sampler2d;
varying mediump vec2	myTexCoord;
varying lowp vec3 colorFactor; 

void main (void)
{
	lowp float grey = dot(texture2D(sampler2d, myTexCoord).rgb, vec3(0.299, 0.587, 0.114));
	gl_FragColor = vec4(grey, grey, grey, 1.0);
}
