struct PSInput
{
	float2 uv0 : TEXCOORD;
};

Texture2D hdrRT : register(t0);

SamplerState PointSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
	// Very basic tonemap - Reinhard tonemapping
	// http://filmicworlds.com/blog/filmic-tonemapping-operators/

	float3 x = hdrRT.Sample(PointSampler, input.uv0).rgb;
	//x *= 1.0f; // Hardcoded exponent
	//x = x / (1.0f + x);
	////x *= 0.4;
	//x = pow(x, (1.0f / 2.2f));

	return float4(x, 1.0f);
}