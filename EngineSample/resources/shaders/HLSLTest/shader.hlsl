
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

cbuffer LightCB : register(b2, space0)
{
	float4 lightDirection;
	float4 lightColor;
}

//combined image sampler(set : 1, binding : 0)
Texture2D<float4> tex : register(t0, space1);
SamplerState testSampler : register(s0, space1);

struct VSInput
{
	float3 pos : POSITION;
	float3 color : COLOR;
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

inline float4 lambert(float3 normal, float3 lightDir, float4 lightColor)
{
	return lightColor * max(dot(normal, lightDir), 0);
}

inline float4 phong(float3 camera, float3 pos, float3 lightDir, float3 normal, float4 lightColor)
{
	return lightColor * pow(max(dot(normalize(camera - pos), reflect(lightDir, normal)), 0), 5.f);
}

float4 PSMain(VSOutput input) : SV_Target0
{
	float4 baseColor = tex.Sample(testSampler, input.uv);

	float4 lightDiffuse = lambert(input.normal, lightDirection, lightColor);

	float4 lightSpecular = phong(cameraPos, input.worldPos, lightDirection, input.normal, lightColor);

	float3 light = ambient + lightDiffuse + lightSpecular;

	return float4((baseColor.xyz * light).xyz, baseColor.w); 
}