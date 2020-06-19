#define RootSig "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"CBV(b0, space = 0, flags = DATA_STATIC)," \
"DescriptorTable(SRV(t0, numDescriptors = 3))," \
"StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR)"

struct CameraParams
{
	matrix ViewMatrix;
	matrix ProjMatrix;
};

ConstantBuffer<CameraParams> CameraCB : register(b0);

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
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

	matrix mvpMatrix = mul(CameraCB.ProjMatrix, CameraCB.ViewMatrix);
	float4 worldPos = float4(input.position, 1.0f);

	output.position = mul(mvpMatrix, worldPos);

	output.uv0 = input.uv0;

	return output;
}