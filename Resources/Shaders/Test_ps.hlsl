Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
Texture2D tex2 : register(t2);

SamplerState PointSampler : register(s0);

struct PSInput
{
	float2 uv0 : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
	return tex0.Sample(PointSampler, input.uv0);
}