uniform sampler2D sampler2d;
varying mediump vec2	myTexCoord;
varying lowp vec3 colorFactor; 

void main (void)
{
	lowp vec4 texColor = texture2D(sampler2d, myTexCoord);
	lowp float grey = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
	if (grey<0.5)
		gl_FragColor = vec4(0., 0., 0., 1.0);	
	else
		gl_FragColor = texColor;
}
