struct PSInput
{
	float2 uv0 : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct PSOutput
{
	float4 gb0 : SV_Target0;
	float4 gb1 : SV_Target1;
	float4 gb2 : SV_Target2;
};

Texture2D BaseColor : register(t0);
Texture2D Normal : register(t1);
Texture2D MetalRoughness : register(t2);

SamplerState AnisotropicSampler : register(s0);

float3 NormalSampleToWorld(float3 normalSample, float3 unitNormal, float3 tangent)
{
	float3 normalT = 2.0f * normalSample - 1.0f;
	float3 N = unitNormal;
	float3 T = normalize(tangent - dot(tangent, N) * N);
	float3 B = cross(N, T);

	float3x3 TBN = float3x3(T, B, N);
	float3 bumpedNormal = mul(transpose(TBN), normalT);

	return bumpedNormal;
}

PSOutput main(PSInput input)
{
	float4 texColor = BaseColor.Sample(AnisotropicSampler, input.uv0);
	float4 normalMap = Normal.Sample(AnisotropicSampler, input.uv0);
	float4 metalRoughness = MetalRoughness.Sample(AnisotropicSampler, input.uv0);

	float3 normal = NormalSampleToWorld(normalMap.xyz, input.normal, input.tangent);

	PSOutput output;
	output.gb0 = pow(texColor, 2.2);
	output.gb1 = float4(normal * 0.5f + 0.5f, 0.0);
	output.gb2 = metalRoughness;

	return output;
}