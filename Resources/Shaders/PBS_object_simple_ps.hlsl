struct PSInput
{
	float2 uv0 : TEXCOORD;
	float4 normal : NORMAL;
};

struct PSOutput
{
	float4 gb0 : SV_Target0;
	float4 gb1 : SV_Target1;
	float4 gb2 : SV_Target2;
};

struct MetalRoughness
{
	float4 MetalRoughness;
};

Texture2D BaseColor : register(t0);
ConstantBuffer<MetalRoughness> mrCB : register(b0, space1);

SamplerState AnisotropicSampler : register(s0);

PSOutput main(PSInput input)
{
	float4 texColor = BaseColor.Sample(AnisotropicSampler, input.uv0 * 100.f);
	float4 normal = float4(normalize(input.normal.xyz), 1.0);

	PSOutput output;
	output.gb0 = pow(texColor, 2.2f);
	output.gb1 = normal;
	output.gb2 = mrCB.MetalRoughness;

	return output;
}