#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 debugPos;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(0.3f, 0.3f, 1.f, 1.f);
    return;
}