/* http://littlecheesecake.me/blog/13804700/opengles-shader */

uniform sampler2D 	sampler2d;
varying mediump vec2 	myTexCoord;
varying lowp vec3 	colorFactor; 

void main (void)
{
    lowp float T = 2.0;
    lowp vec2 st = myTexCoord.st;
    lowp vec3 irgb = texture2D(sampler2d, st).rgb;
    lowp vec3 black = vec3(0., 0., 0.);
    gl_FragColor = vec4(mix(black, irgb, T), 1.);

}
