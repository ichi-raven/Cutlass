
//attention : (bx, spacey) == set y, binding x (regardless of register type)

cbuffer ModelCB : register(b0, space0)
{
	float4x4 world;
	float4x4 view;
	float4x4 proj;
	float3 cameraPos;
};

cbuffer ShadowCB : register(b1, space0)
{
	float4x4 lightViewProj;
	float4x4 lightViewProjBias;
};

struct VSInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
	float4 joint0;
	float4 weight0;
};

struct VSOutput
{
	float4 pos : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output;
	float4 inPos = float4(input.pos.xyz, 1.0f);
	output.pos = mul(mul(lightViewProj, world), inPos);

	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	float distance = input.pos.z / input.pos.w;

	return float4(float3(distance).xyz, 1);
}