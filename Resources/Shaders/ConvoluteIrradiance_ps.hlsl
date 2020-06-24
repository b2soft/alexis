#include "PBS_helpers.hlsli"

TextureCube envMap : register(t0);

SamplerState CubeSampler : register(s0);

struct PSInput
{
	float4 localPos : POSITION;
};

float4 main(PSInput input) : SV_TARGET
{
	float3 normal = normalize(input.localPos.xyz);
	float3 irradiance = float3(0.f, 0.f, 0.f);

	float3 up = float3(0.f, 1.0f, 0.f);
	float3 right = cross(up, normal);
	up = cross(normal, right);

	float sampleDelta = 0.025f;
	float nrSamples = 0.0f;
	for (float phi = 0.0; phi < 2.0 * k_pi; phi += sampleDelta)
	{
		for (float theta = 0.0; theta < 0.5 * k_pi; theta += sampleDelta)
		{
			// Spherical To Cartesian (Tangent)
			float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

			// Tangent To World
			float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

			irradiance += envMap.Sample(CubeSampler, sampleVec).rgb * cos(theta) * sin(theta);
			nrSamples++;
		}
	}

	irradiance = k_pi * irradiance * (1.0 / nrSamples);

	return float4(irradiance, 1.0f);
}