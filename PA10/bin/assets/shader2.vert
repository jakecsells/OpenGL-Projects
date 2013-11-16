in vec4 v_position;
in vec3 v_normal;

uniform mat4 view; // from lookAt() 
uniform mat4 projection; // perspective projection 

out vec3 Position;
out vec3 Normal;
out vec4 lightPosEye;

void main() {
    mat4 modelView = view * model[gl_InstanceID];
    mat3 normalMatrix = mat3(transpose(inverse(modelView)));
    //mat3 normalMatrix = mat3(modelView);

    vec4 lightPos = vec4(350, 350, 350, 1);
    lightPosEye = modelView * lightPos;

    Position = vec3(modelView * vertexCoord);
    Normal = normalize(normalMatrix * vertexNormal);

    gl_Position = projection * vec4(Position, 1.0);
}