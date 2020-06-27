#define RootSig "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"CBV(b0, space = 0, flags = DATA_STATIC)," \
"CBV(b1, space = 0, flags = DATA_STATIC)," \
"CBV(b2, space = 0, flags = DATA_STATIC)," \
"DescriptorTable(SRV(t0, numDescriptors = 8),visibility=SHADER_VISIBILITY_PIXEL)," \
"StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT)," \
"StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)," \
"StaticSampler(s2, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)," \
"StaticSampler(s3, filter = FILTER_ANISOTROPIC, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)"

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