Texture2D envMap : register(t0);

SamplerState PointSampler : register(s0);

struct PSInput
{
	float4 localPos : POSITION;
};

static const float2 invAtan = float2(0.1591f, 0.3183f);
float2 SampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5f;
	uv.y = 1.0 - uv.y;
	return uv;
}

float4 main(PSInput input) : SV_TARGET
{
	float2 uv = SampleSphericalMap(normalize(input.localPos.xyz));
	float3 color = envMap.Sample(PointSampler, uv).rgb;

	return float4(color, 1.0f);
}