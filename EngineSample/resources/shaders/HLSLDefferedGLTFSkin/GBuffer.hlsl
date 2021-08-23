
//attention : (bx, spacey) == set y, binding x (regardless of register type)

static const int MAX_BONE_NUM = 128;

cbuffer ModelCB : register(b0, space0)
{
	float4x4 world;
	float4x4 view;
	float4x4 proj;
	float receiveShadow;
	float lighting;
	float2 padding2;
};

cbuffer BoneCB : register(b1, space0)
{
	uint useBone;//if use bone 1 else 0
	float3 padding;
	float4x4 boneMat[MAX_BONE_NUM];
}

//combined image sampler(set : 1, binding : 0)
Texture2D<float4> tex : register(t0, space1);
SamplerState testSampler : register(s0, space1);

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
	float4 pos : SV_Position;
	float3 normal : Normal;
	float2 uv0 : Texcoord0;
	float4 worldPos;
};

struct PSOut
{
	float4 albedo : SV_Target0;
	float4 normal : SV_Target1;
	float4 worldPos : SV_Target2;
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
	

	output.pos = mul(mul(mul(proj, view), world), skinnedPos);
	output.normal = mul(world, float4(input.normal, 0.f)).xyz;
	output.uv0 = input.uv0;
	//output.worldPos = mul(world, inPos);
	output.worldPos = mul(world, skinnedPos);

	return output;
}

PSOut PSMain(VSOutput input)
{
	PSOut psOut;
	psOut.albedo = tex.Sample(testSampler, input.uv0);
	psOut.normal = float4((input.normal / 2.f + 0.5f), lighting);
	psOut.worldPos = float4(input.worldPos.xyz, receiveShadow);

	return psOut;
}