#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(set = 1, binding = 0) uniform materialUB
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    uint useTexture;
    uint edgeFlag;
};

layout(set = 1, binding = 1) uniform sampler2D diffuseTex;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 debugPos;

layout(location = 0) out vec4 outColor;

void main() 
{
    // outColor = vec4(debugPos, 1.f);
    // return;

    vec4 color = texture(texSampler, fragUV);

    vec4 lightDirection = vec4(0, 1.f, 1.f, 0);
    vec3 normal = normalize(fragNormal);
    vec3 toLightDirection = normalize(lightDirection.xyz);
    float lmb = clamp(dot(toLightDirection, normalize(fragNormal)),0,1);

    vec3 baseColor = color.rgb;
    baseColor = baseColor * lmb;
    baseColor += baseColor * ambient.xyz;
    
    outColor = vec4(baseColor.rgb, color.w);
}