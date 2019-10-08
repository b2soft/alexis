struct VSInput
{
	float3 position : POSITION;
	float2 uv0 : TEXCOORD;
};

struct VSOutput
{
	float2 uv0 : TEXCOORD;
	float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position, 1.0);
	output.uv0 = input.uv0;

	return output;
}