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

	float4 x = hdrRT.Sample(PointSampler, input.uv0);
	x = max(0.0, x);
	//x *= 16; // Hardcoded exponent
	x = x / (1 + x);
	x = pow(x, 1 / 2.2);

	return x;
}