#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec3 debugPos;


void main() 
{
    vec4 inPos =  vec4(inPosition, 1.0);
    inPos.z -= 10;
    fragColor = inColor;
    fragUV = inUV;
    fragNormal = inNormal;
    vec4 pos = proj * view * model * inPos;
    debugPos = inPosition;
    gl_Position = pos;
}