#define RootSig "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"DescriptorTable(SRV(t0, numDescriptors = 1), visibility=SHADER_VISIBILITY_PIXEL)," \
"StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT)"

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

[RootSignature(RootSig)]
VSOutput main(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position, 1.0);
	output.uv0 = input.uv0;

	return output;
}