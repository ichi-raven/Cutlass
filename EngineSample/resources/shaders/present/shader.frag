#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2D albedoSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D worldPosSampler;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

vec4 meanBlur(sampler2D s, vec2 uv)
{
    vec4 color;
    float coef = 0.001f;
    for(int i = -1; i <= 1; ++i)
        for(int j = -1; j <= 1; ++j)
        {
            vec2 uvMean = vec2(min(max(uv.x + coef * i, 0), 1), min(max(uv.y + coef * j, 0), 1));
            color += texture(s, uvMean);    
        }
    return color / 9.f;
}

void main()
{
    vec2 uv;
    if(inUV.x <= 0.5)
    {
        uv.x = inUV.x * 2.f;
        if(inUV.y <= 0.5)//target
        {
            uv.y = inUV.y * 2.f;
            outColor = meanBlur(texSampler, uv);
        }
        else//albedo
        {
            uv.y = ((inUV.y - 0.5f) * 2.f);
            outColor = meanBlur(albedoSampler, uv);
        }
    }
    else
    {
        uv.x = ((inUV.x - 0.5f) * 2.f);
        if(inUV.y <= 0.5)//normal
        {
            uv.y = inUV.y * 2.f;
            outColor = meanBlur(normalSampler, uv);
        }
        else//worldPos
        {
            uv.y = ((inUV.y - 0.5f) * 2.f);
            outColor = meanBlur(worldPosSampler, uv);
        }
    }

}