uniform sampler2D sampler2d;
varying mediump vec2	myTexCoord;
varying lowp vec3 colorFactor; 

lowp vec3 rgb2hsv(lowp vec3 c)
{
    lowp vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    lowp vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    lowp vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    highp float d = q.x - min(q.w, q.y);
    highp float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

lowp vec3 hsv2rgb(lowp vec3 c)
{
    lowp vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    lowp vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
void main() {
/*  lowp vec4 textureColor = texture2D(sTexture, vTextureCoord); */
    lowp vec4 textureColor = texture2D(sampler2d, myTexCoord);
    lowp vec3 fragRGB = textureColor.rgb;
    lowp vec3 fragHSV = rgb2hsv(fragRGB).xyz;
    lowp vec3 vHSV = vec3(1.0, 0.8, 0.99);
    fragHSV.x += vHSV.x / 360.0;
    fragHSV.yz *= vHSV.yz;
    fragHSV.xyz = mod(fragHSV.xyz, 1.0);
    fragRGB = hsv2rgb(fragHSV);
    gl_FragColor = vec4(fragRGB, textureColor.w);
} 


