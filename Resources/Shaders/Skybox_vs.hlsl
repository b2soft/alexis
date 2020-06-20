#define RootSig "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"CBV(b0, space = 0, flags = DATA_STATIC)," \
"DescriptorTable(SRV(t0, numDescriptors = 1),visibility=SHADER_VISIBILITY_PIXEL)," \
"StaticSampler(s0, filter = FILTER_ANISOTROPIC)"

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
	float3 localPos = pos.xyz;

	matrix rotView = CamCB.ViewMatrix;
	rotView._41 = 0;
	rotView._43 = 0;
	rotView._44 = 0;

	rotView._14 = 0;
	rotView._24 = 0;
	rotView._34 = 0;
	
	rotView._44 = 1;

	float4 clipPos = mul(CamCB.ProjMatrix, mul(rotView, pos));

	output.localPos = pos;
	output.position = clipPos.xyww;

	return output;
}
