#define RootSig "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"CBV(b0, flags = DATA_STATIC)," \
"CBV(b1, flags = DATA_STATIC)," \
"CBV(b2, flags = DATA_STATIC)," \
"CBV(b3, flags = DATA_STATIC)," \
"DescriptorTable(SRV(t0, numDescriptors = 4),visibility=SHADER_VISIBILITY_PIXEL)," \
"StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)"

struct LightParams
{
	matrix WVPMatrix;
};

ConstantBuffer<LightParams> LightCB : register(b0);

struct VSInput
{
	float3 position : POSITION;
};

struct VSOutput
{
	float4 position : SV_Position;
};

[RootSignature(RootSig)]
VSOutput main(VSInput input)
{
	VSOutput output;
	output.position = mul(LightCB.WVPMatrix, float4(input.position, 1.0));
	return output;
}