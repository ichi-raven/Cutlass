#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texSampler;
// layout(binding = 1) uniform sampler2D albedoSampler;
// layout(binding = 2) uniform sampler2D normalSampler;
// layout(binding = 3) uniform sampler2D worldPosSampler;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

vec4 gaussianBlur(sampler2D s, vec2 uv)
{
    const float delta = 0.001f;
     
    vec4 color =
    0.0625 * texture(s, vec2(uv.x - delta, uv.y - delta)) +
    0.125  * texture(s, vec2(uv.x,         uv.y - delta)) +
    0.0625 * texture(s, vec2(uv.x + delta, uv.y - delta)) +
    0.125  * texture(s, vec2(uv.x - delta, uv.y))         + 
    0.25   * texture(s, vec2(uv.x,         uv.y))         +
    0.125  * texture(s, vec2(uv.x + delta, uv.y))         +
    0.0625 * texture(s, vec2(uv.x - delta, uv.y + delta)) +
    0.125  * texture(s, vec2(uv.x,         uv.y + delta)) +
    0.0625 * texture(s, vec2(uv.x + delta, uv.y + delta));

    return color;
}

void main()
{
    //outColor = texture(texSampler, inUV);
    outColor = gaussianBlur(texSampler, inUV);
    // vec2 uv;
    // if(inUV.x <= 0.5)
    // {
    //     uv.x = inUV.x * 2.f;
    //     if(inUV.y <= 0.5)//target
    //     {
    //         uv.y = inUV.y * 2.f;
    //         outColor = gaussianBlur(texSampler, uv);
    //     }
    //     else//albedo
    //     {
    //         uv.y = ((inUV.y - 0.5f) * 2.f);
    //         outColor = gaussianBlur(albedoSampler, uv);
    //     }
    // }
    // else
    // {
    //     uv.x = ((inUV.x - 0.5f) * 2.f);
    //     if(inUV.y <= 0.5)//normal
    //     {
    //         uv.y = inUV.y * 2.f;
    //         outColor = gaussianBlur(normalSampler, uv);
    //     }
    //     else//worldPos
    //     {
    //         uv.y = ((inUV.y - 0.5f) * 2.f);
    //         outColor = gaussianBlur(worldPosSampler, uv);
    //     }
    // }

}