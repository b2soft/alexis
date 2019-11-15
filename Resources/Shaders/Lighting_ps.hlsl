struct PSInput
{
	float2 uv0 : TEXCOORD;
};

Texture2D gb0 : register(t0);
Texture2D gb1 : register(t1);
Texture2D gb2 : register(t2);

SamplerState PointSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
	float4 col = gb0.Sample(PointSampler, input.uv0);
	return col * 3.0f;
	//return col;
}