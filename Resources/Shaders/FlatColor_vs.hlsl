#define RootSig "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"CBV(b0, space = 0, flags = DATA_STATIC)," \
"CBV(b1, space = 0, flags = DATA_STATIC)" 

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

[RootSignature(RootSig)]
float4 main(VSInput input) : SV_Position
{
	matrix mvpMatrix = mul(CameraCB.ProjMatrix, mul(CameraCB.ViewMatrix, ModelCB.ModelMatrix));
	float4 worldPos = mul(mvpMatrix,float4(input.position, 1.0f));
	return worldPos;
}