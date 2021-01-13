#define RootSig "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"CBV(b0, space = 0, flags = DATA_STATIC)," \
"DescriptorTable(SRV(t0, numDescriptors = 1),visibility=SHADER_VISIBILITY_PIXEL)," \
"StaticSampler(s0, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT,addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)"

struct CameraParams
{
	matrix ViewMatrix;
	matrix ProjMatrix;
};

struct VSOutput
{
	float4 localPos : POSITION;
	float4 position : SV_Position;
};

ConstantBuffer<CameraParams> CamCB : register(b0);

[RootSignature(RootSig)]
VSOutput main(float4 pos : POSITION)
{
	VSOutput output;
	output.position = mul(CamCB.ProjMatrix, mul(CamCB.ViewMatrix, pos));
	output.localPos = pos;

	return output;
}