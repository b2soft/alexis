#include "../PBS_helpers.hlsli"

float VanDerCorpusSequence(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

	return float(bits) * 2.3283064365386963e-10; // 0x100000000
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), VanDerCorpusSequence(i));
}

TextureCube envMap : register(t0);

SamplerState CubeSampler : register(s0);

struct RoughnessType
{
	float Roughness;
};

ConstantBuffer<RoughnessType> RoughnessCB : register(b1);

struct PSInput
{
	float4 localPos : POSITION;
};

float4 main(PSInput input) : SV_TARGET
{
	float3 N = normalize(input.localPos.xyz);
	float3 R = N;
	float3 V = R;

	const uint SAMPLE_COUNT = 1024u;
	float totalWeight = 0.0f;
	float3 prefilteredColor = 0.0f;
	for (uint i = 0u; i < SAMPLE_COUNT; ++i)
	{
		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, RoughnessCB.Roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0);
		if (NdotL > 0.0)
		{
			prefilteredColor += envMap.Sample(CubeSampler, L).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	prefilteredColor = prefilteredColor / totalWeight;

	return float4(prefilteredColor, 1.0);

}