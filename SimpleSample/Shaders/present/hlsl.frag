Texture2D<float4> texture : register(t0);

SamplerState texSampler : register(s0);

struct VS_OUTPUT
{
    float2 inUV : TEXCOORD0;
};

float4 main(VS_OUTPUT input): SV_Target0
{
    return texture.sample(texSampler, input.inUV);
}