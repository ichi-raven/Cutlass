
//attention : (bx, spacey) == set y, binding x (regardless of register type)

cbuffer ModelCB : register(b0, space0)
{
	float4x4 world;
	float4x4 view;
	float4x4 proj;
};

cbuffer MaterialCB : register(b1, space0)
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	uint useTexture;
}

//combined image sampler(set : 1, binding : 0)
Texture2D<float4> tex : register(t0, space1);
SamplerState testSampler : register(s0, space1);

struct VSInput
{
	float3 pos : POSITION;
	float4 color : COLOR;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct VSOutput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
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
	output.pos = mul(mul(mul(proj, view), world), inPos);
	output.color = input.color;
	output.uv = input.uv;
	output.normal = mul(world, float4(input.normal, 0.f)).xyz;
	output.worldPos = mul(world, inPos);

	return output;
}

PSOut PSMain(VSOutput input)
{
	PSOut psOut;
	psOut.albedo = tex.Sample(testSampler, input.uv);
	//補正
	psOut.normal = float4((input.normal / 2.f + 0.5f), 1.f);
	psOut.worldPos = input.worldPos;

	return psOut;
}