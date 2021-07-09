
//attention : (bx, spacey) == set y, binding x (regardless of register type)

cbuffer ModelCB : register(b0, space0)
{
	float4x4 world;
	float4x4 view;
	float4x4 proj;
	float3 cameraPos;
};

cbuffer MaterialCB : register(b1, space0)
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
}

cbuffer BoneCB : register(b2, space0)
{
	float4x4 jointMat[256];
}

//combined image sampler(set : 1, binding : 0)
Texture2D<float4> tex : register(t0, space1);
SamplerState testSampler : register(s0, space1);

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
	float4 pos : SV_Position;
	float3 normal : Normal;
	float2 uv0 : Texcoord0;
	float2 uv1 : Texcoord1;
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
	float4 inPos = float4(input.pos.xyz, 1.0f);
	float4x4 boneAll = 
	jointMat[int(input.joint0.x)] * input.weight0.x + 
	jointMat[int(input.joint0.y)] * input.weight0.y +
	jointMat[int(input.joint0.z)] * input.weight0.z +
	jointMat[int(input.joint0.w)] * input.weight0.w;
	
	output.pos = mul(mul(mul(mul(proj, view), world), boneAll), inPos);
	output.normal = mul(world, float4(input.normal, 0.f)).xyz;
	output.uv0 = input.uv0;
	output.uv1 = float2(0, 0);
	//output.worldPos = mul(world, inPos);
	output.worldPos = mul(mul(world, boneAll), inPos);

	return output;
}

PSOut PSMain(VSOutput input)
{
	PSOut psOut;
	psOut.albedo = tex.Sample(testSampler, input.uv0);
	psOut.normal = float4((input.normal / 2.f + 0.5f), 1.f);
	psOut.worldPos = input.worldPos;

	return psOut;
}