TextureCube envMap : register(t0);

SamplerState CubeSampler : register(s0);

struct PSInput
{
	float4 localPos : POSITION;
};

float4 main(PSInput input) : SV_TARGET
{
	float3 skyColor = envMap.Sample(CubeSampler, input.localPos.xyz).rgb;

	//skyColor = skyColor / (skyColor + float3(1.0f, 1.0f, 1.0f));
	//skyColor = pow(skyColor, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

	return float4(skyColor, 1.0f);
}