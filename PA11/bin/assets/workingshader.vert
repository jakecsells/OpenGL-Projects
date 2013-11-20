#version 130

uniform mat4 mvpMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

attribute vec3 v_position;
attribute vec2 v_uv;
attribute vec3 v_normal;
varying vec2 v_UV;
out vec3 LightIntensity;

struct LightInfo {
	vec4 Position; // Light position in eye coords.
	vec3 La; // Ambient light intensity
	vec3 Ld; // Diffuse light intensity
	vec3 Ls; // Specular light intensity
};
uniform LightInfo Light;

// struct MaterialInfo {
// 	vec3 Ka; // Ambient reflectivity
// 	vec3 Kd; // Diffuse reflectivity
// 	vec3 Ks; // Specular reflectivity
// 	float Shininess; // Specular shininess factor
// };
// uniform MaterialInfo Material;

vec3 phongModel(vec4 position, vec3 norm) {
	// vec3 s = normalize(vec3(Light.Position - position));
	vec3 s = normalize(vec3(vec3(0.0,10.0,1.0) - vec3(position)));
	vec3 v = normalize(-position.xyz);
	vec3 r = reflect( -s, norm );
	// vec3 ambient = Light.La * Material.Ka;
	vec3 ambient = vec3(0.5,0.5,0.5);
	float sDotN = max( dot(s,norm), 0.0 );
	// vec3 diffuse = Light.Ld * Material.Kd * sDotN;
	vec3 diffuse = vec3(0.5,0.5,0.5) * sDotN;
	vec3 spec = vec3(0.0);
	if( sDotN > 0.0 )
		spec = vec3(0.5,0.5,0.5) * pow( max( dot(r,v), 0.0 ), 0.5 );
		// spec = Light.Ls * Material.Ks * pow( max( dot(r,v), 0.0 ), Material.Shininess );
	return ambient + diffuse + spec;
}

void main(void){
	vec3 eyeNorm = gl_NormalMatrix * v_normal;
	vec4 eyeCoords = modelViewMatrix * vec4(v_position, 1.0);
	vec3 s = normalize(vec3(vec4(0.0,10.0,1.0,1.0) - eyeCoords));
	LightIntensity = phongModel(eyeCoords, eyeNorm);
	v_UV = v_uv;
	gl_Position = mvpMatrix * vec4(v_position, 1.0);
}

