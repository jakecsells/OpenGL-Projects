#version 130

uniform mat4 mvpMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform ivec4 toggles; // (ambient, distant, point, spot)

attribute vec3 v_position;
attribute vec2 v_uv;
attribute vec3 v_normal;
varying vec2 v_UV;
out vec3 LightIntensity;
uniform vec3 RAVEMODE; // (red, green, blue)

struct LightInfo {
	vec4 Position; // Light position in eye coords.
	vec3 La; // Ambient light intensity
	vec3 Ld; // Diffuse light intensity
	vec3 Ls; // Specular light intensity
};
uniform LightInfo Light;

vec3 phongModel(vec4 position, vec3 norm, int amb, int pnt) {
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
	return ambient*amb + diffuse*pnt + spec*pnt;
}

vec3 phongModelDistant(vec4 position, vec3 norm, int dist) {
	// vec3 s = normalize(vec3(Light.Position - position));
	vec3 dir = normalize(vec3(-1,0,0));
	vec3 s = normalize(vec3(vec3(0.0,10.0,1.0) - vec3(position)));
	vec3 v = normalize(-position.xyz);
	vec3 r = reflect( -s, norm );
	// vec3 ambient = Light.La * Material.Ka;
	vec3 ambient = vec3(0.5,0.5,0.5);
	float sDotN = max( dot(dir,norm), 0.0 );
	// vec3 diffuse = Light.Ld * Material.Kd * sDotN;
	vec3 diffuse = vec3(0.5,0.5,0.5) * sDotN;
	vec3 spec = vec3(0.0);
	if( sDotN > 0.0 )
		spec = vec3(0.5,0.5,0.5) * pow( max( dot(r,v), 0.0 ), 0.5 );
		// spec = Light.Ls * Material.Ks * pow( max( dot(r,v), 0.0 ), Material.Shininess );
	return diffuse*dist + spec*dist;
}

vec3 INITIALIZERAVEMODE(vec4 position, vec3 norm, float x, float y, float z){
	// vec3 s = normalize(vec3(Light.Position - position));
	vec3 s = normalize(vec3(vec3(0.0,10.0,1.0) - vec3(position)));
	vec3 v = normalize(-position.xyz);
	vec3 r = reflect( -s, norm );
	// vec3 ambient = Light.La * Material.Ka;
	vec3 ambient = vec3(x,y,z);
	float sDotN = max( dot(s,norm), 0.0 );
	// vec3 diffuse = Light.Ld * Material.Kd * sDotN;
	vec3 diffuse = vec3(0.5,0.5,0.5) * sDotN;
	vec3 spec = vec3(0.0);
	if( sDotN > 0.0 )
		spec = vec3(0.5,0.5,0.5) * pow( max( dot(r,v), 0.0 ), 0.5 );
		// spec = Light.Ls * Material.Ks * pow( max( dot(r,v), 0.0 ), Material.Shininess );
	return ambient;
}

void main(void){
	LightIntensity = vec3(0.,0.,0.);
	vec3 eyeNorm = gl_NormalMatrix * v_normal;
	vec4 eyeCoords = modelViewMatrix * vec4(v_position, 1.0);
	vec3 s = normalize(vec3(vec4(0.0,10.0,1.0,1.0) - eyeCoords));
	if(toggles.w == 1)
		LightIntensity += INITIALIZERAVEMODE(eyeCoords, eyeNorm, RAVEMODE.x, RAVEMODE.y, RAVEMODE.z);
	else{
		LightIntensity += phongModel(eyeCoords, eyeNorm, toggles.x, toggles.z);
		LightIntensity += phongModelDistant(eyeCoords, eyeNorm, toggles.y);
	}
	v_UV = v_uv;
	gl_Position = mvpMatrix * vec4(v_position, 1.0);
}