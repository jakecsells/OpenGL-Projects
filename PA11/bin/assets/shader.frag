#version 130

varying vec2 v_UV;
uniform sampler2D gSampler;
in vec3 LightIntensity;
void main(){
   gl_FragColor = vec4(LightIntensity, 1.0) * texture2D(gSampler, vec2(v_UV[0], v_UV[1]));
}