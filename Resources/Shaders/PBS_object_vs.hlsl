#define RootSig "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"CBV(b0, space = 0, flags = DATA_STATIC)," \
"CBV(b1, space = 0, flags = DATA_STATIC)," \
"DescriptorTable(SRV(t0, numDescriptors = 3), visibility=SHADER_VISIBILITY_PIXEL)," \
"StaticSampler(s0, filter = FILTER_ANISOTROPIC)"

struct CameraParams
{
	matrix ViewMatrix;
	matrix ProjMatrix;
};

struct ModelParams
{
	matrix ModelMatrix;
};

ConstantBuffer<CameraParams> CameraCB : register(b0);
ConstantBuffer<ModelParams> ModelCB : register(b1);

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
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 position : SV_Position;
};

[RootSignature(RootSig)]
VSOutput main(VSInput input)
{
	VSOutput output;

	matrix mvpMatrix = mul(CameraCB.ProjMatrix, mul(CameraCB.ViewMatrix, ModelCB.ModelMatrix));
	float4 worldPos = float4(input.position, 1.0f);

	output.position = mul(mvpMatrix, worldPos);
	output.uv0 = input.uv0;

	float3 normal = mul(ModelCB.ModelMatrix, float4(input.normal, 0.0f)).xyz;
	output.normal = normal;

	float3 tangent = mul(ModelCB.ModelMatrix, float4(input.tangent, 0.0f)).xyz;
	output.tangent = tangent;

	return output;
}