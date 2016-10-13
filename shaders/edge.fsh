/* 
http://www.blog.nathanhaze.com/glsl-edge-detection/

varying mediump vec2	myTexCoord;
varying lowp vec3 colorFactor; 
*/

varying lowp vec2 texc;
uniform sampler2D sampler2d;

varying lowp vec2 left_coord;
varying lowp vec2 right_coord;
varying lowp vec2 above_coord;
varying lowp vec2 below_coord;

void main()
{

gl_FragColor = texture2D(sampler2d, texc);
gl_FragColor.a = 1.0; //Best to make sure nothing seems transparent

lowp vec2 c = texc.xy;
lowp vec4 diffuse_color = texture2D(sampler2d, texc);
lowp vec4 diffuse_colorL = texture2D(sampler2d, left_coord);
lowp vec4 diffuse_colorR = texture2D(sampler2d, right_coord);
lowp vec4 diffuse_colorA = texture2D(sampler2d, above_coord);
lowp vec4 diffuse_colorB = texture2D(sampler2d, below_coord);

gl_FragColor = (diffuse_colorL + diffuse_colorR +diffuse_colorA +diffuse_colorB ) - (4.0 *diffuse_color);
gl_FragColor = vec4(gl_FragColor.r, gl_FragColor.r, gl_FragColor.r, 1.0);
gl_FragColor.a = 1.0;

}
