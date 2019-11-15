struct PSInput
{
	float2 uv0 : TEXCOORD;
};

Texture2D hdrRT : register(t0);

SamplerState PointSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
	float4 x = hdrRT.Sample(PointSampler, input.uv0);
	// Very basic tonemap
	return x / (x + 1.0);
}