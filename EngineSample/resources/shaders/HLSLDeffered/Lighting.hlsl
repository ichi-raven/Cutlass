cbuffer DirectionLight : register(b0, space0)
{
    float4 lightDirection;    // ライトの方向
    float4 lightColor;        // ライトのカラー
};

//cbuffer Scene : register(b1, space0)
// {
// 	float4x4 world;
// 	float4x4 view;
// 	float4x4 proj;
// 	float3 cameraPos;
// }

//combined image sampler(set : 1, binding : 0)
Texture2D<float4> albedoTex : register(t0, space1);
SamplerState albedoSampler : register(s0, space1);

//combined image sampler(set : 1, binding : 1)
Texture2D<float4> normalTex : register(t1, space1);
SamplerState normalSampler : register(s1, space1);

//combined image sampler(set : 1, binding : 2)
Texture2D<float4> worldPosTex : register(t2, space1);
SamplerState worldPosSampler : register(s2, space1);

struct VSOutput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

VSOutput VSMain(uint id : SV_VERTEXID)
{
	float x = float(id / 2);
	float y = float(id % 2);
	VSOutput output;
	output.pos = float4(x * 2.f - 1.f, y * 2.f - 1.f, 0, 1.f);
	output.uv = float2(x, y);

	return output;
}

inline float4 lambert(float3 normal, float3 lightDir, float4 lightColor)
{
	return lightColor * max(dot(normal, lightDir) * -1.f, 0);
}

inline float4 phong(float3 camera, float3 pos, float3 lightDir, float3 normal, float4 lightColor)
{
	return lightColor * pow(max(dot(normalize(camera - pos), reflect(lightDir, normal)) * -1.f, 0), 5.f);
}

float4 PSMain(VSOutput input) : SV_Target0
{
	float4 albedo = albedoTex.Sample(albedoSampler, input.uv);
	float4 normal = normalTex.Sample(normalSampler, input.uv);
	float4 worldPos = worldPosTex.Sample(worldPosSampler, input.uv);

	float4 ambient = float4(0.2f, 0.2f, 0.2f, 1.f);

	normal = (normal * 2.f) - 1.f;
	normal.w = 1.f;

	float4 lightDiffuse = lambert(normal.xyz, lightDirection.xyz, lightColor);

	float4 lightSpecular = phong(cameraPos, worldPos.xyz, lightDirection.xyz, normal.xyz, lightColor);

	float4 light = ambient + lightDiffuse + lightSpecular;

	return float4((albedo * light).xyz, albedo.w); 
}