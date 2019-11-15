struct PSInput
{
	float4 normal : NORMAL;
	float2 uv0 : TEXCOORD;
	float4 position : Test;
};

struct PSOutput
{
	float4 gb0 : SV_Target0;
	float4 gb1 : SV_Target1;
	float4 gb2 : SV_Target2;
};

Texture2D BaseColor : register(t0);
Texture2D Normal : register(t1);
Texture2D MetalRoughness : register(t2);

SamplerState AnisotropicSampler : register(s0);

PSOutput main(PSInput input)
{
	float4 texColor = BaseColor.Sample(AnisotropicSampler, input.uv0);
	float4 normal = Normal.Sample(AnisotropicSampler, input.uv0);
	float4 metalRoughness = MetalRoughness.Sample(AnisotropicSampler, input.uv0);

	texColor.a = input.position.x;
	normal.a = input.position.y;
	metalRoughness.a = input.position.z;

	PSOutput output;
	output.gb0 = texColor;
	output.gb1 = normal;
	output.gb2 = metalRoughness;

	return output;
}