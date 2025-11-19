#version 330 core

in VS_OUT {
    vec3 pos;
    vec3 normal;
    vec3 color;
} fs_in;

out vec4 FragColor;

uniform float ambientStrength;
uniform float lightStrength;
uniform float shininess;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    // ambient
    vec3 ambient = ambientStrength * fs_in.color;

    // diffuse
    vec3 lightDir = normalize(lightPos - fs_in.pos);
    vec3 normal = normalize(fs_in.normal);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * fs_in.color;

    // specular
    vec3 viewDir = normalize(viewPos - fs_in.pos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = vec3(lightStrength) * spec; 

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}