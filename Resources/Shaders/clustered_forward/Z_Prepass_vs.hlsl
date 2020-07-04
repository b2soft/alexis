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
	matrix WorldMatrix;
};

ConstantBuffer<CameraParams> CameraCB : register(b0);
ConstantBuffer<ModelParams> ModelCB : register(b1);

[RootSignature(RootSig)]
float4 main(float4 pos : POSITION) : SV_POSITION
{
	matrix mvpMatrix = mul(CameraCB.ProjMatrix, mul(CameraCB.ViewMatrix, ModelCB.WorldMatrix));
	float4 worldPos = float4(pos.xyz, 1.0f);
	return mul(mvpMatrix, worldPos);
}