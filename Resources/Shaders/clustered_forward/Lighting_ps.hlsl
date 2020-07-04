struct PSInput
{
	float2 uv0 : TEXCOORD;
	float3 normal : NORMAL;
	//float3 tangent : TANGENT;
};

Texture2D BaseColor : register(t0);
Texture2D Normal : register(t1);
Texture2D MetalRoughness : register(t2);

SamplerState AnisotropicSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
	float4 texColor = BaseColor.Sample(AnisotropicSampler, input.uv0);
	float4 normalMap = Normal.Sample(AnisotropicSampler, input.uv0);
	float4 metalRoughness = MetalRoughness.Sample(AnisotropicSampler, input.uv0);

	float3 normal = input.normal;// NormalSampleToWorld(normalMap.xyz, input.normal, input.tangent);

	float3 L = normalize(float3(5.0, 8.0, 15.0));
	return float4(texColor.rgb * max(dot(normal, L), 0.0), 1.0);
}