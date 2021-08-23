
//attention : (bx, spacey) == set y, binding x (regardless of register type)

cbuffer ModelCB : register(b0, space0)
{
	float4x4 world;
	float4x4 view;
	float4x4 proj;
	float receiveShadow;
	float lighting;
	float2 padding2;
};

cbuffer ShadowCB : register(b1, space0)
{
	float4x4 lightViewProj;
	float4x4 lightViewProjBias;
};

cbuffer BoneCB : register(b2, space0)
{
	uint useBone;//if use bone 1 else 0
	float3 padding;
	float4x4 boneMat[128];
};

struct VSInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv0 : TEXCOORD0;
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
	float4 skinnedPos = float4(input.pos.xyz, 1.0f);

	if(useBone)
	{
		float4x4 boneAll = 
		boneMat[int(input.joint0.x)] * input.weight0.x + 
		boneMat[int(input.joint0.y)] * input.weight0.y +
		boneMat[int(input.joint0.z)] * input.weight0.z +
		boneMat[int(input.joint0.w)] * input.weight0.w;
	
		skinnedPos = mul(boneAll, float4(input.pos.xyz, 1.0f));
	}
	

	output.pos = mul(mul(lightViewProj, world), skinnedPos);

	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	float distance = input.pos.z / input.pos.w;

	return float4(float3(distance).xyz, 1);
}