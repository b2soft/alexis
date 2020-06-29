#define RootSig "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"CBV(b0, space = 0, flags = DATA_STATIC)" \

struct DepthParams
{
	matrix ModelMatrix;
	matrix DepthMVP;
};

ConstantBuffer<DepthParams> DepthCB : register(b0);

[RootSignature(RootSig)]
float4 main( float4 pos : POSITION ) : SV_POSITION
{
	matrix mvpMatrix = mul(DepthCB.DepthMVP, DepthCB.ModelMatrix);
	float4 worldPos = float4(pos.xyz, 1.0f);

	return mul(mvpMatrix, worldPos);
}