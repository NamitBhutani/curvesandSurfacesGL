#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out VS_OUT {
    vec3 pos;
    vec3 normal;
    vec3 color;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vs_out.pos = aPos;
    vs_out.normal = aNormal;
    vs_out.color = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}