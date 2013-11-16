in vec3 Position;
in vec3 Normal;
in vec4 lightPosEye;

layout(location = 0) out vec4 FragColor;

vec3 ads() {
    vec3 Ka = vec3(0, 0.5, 0);
    vec3 Kd = vec3(0, 0.5, 0);
    vec3 Ks = vec3(0, 0.1, 0);
    vec3 intensity = vec3(0.3, 0.5, 0.0);
    float shininess = 0.1;

    vec3 n = normalize(Normal);
    vec3 s = normalize(vec3(lightPosEye) - Position);
    vec3 v = normalize(vec3(-Position));
    vec3 r = reflect(-s, n);

    return intensity * (Ka + Kd * max(dot(s, n), 0.0) + Ks * pow(max(dot(r,v), 0.0), shininess));
}

void main() {
    FragColor = vec4(ads(), 1);
}