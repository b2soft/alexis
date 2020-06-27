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
	//float3x3 TBN : TBNBASIS;
	float4 normal : NORMAL;
	float4 worldPos : WORLDPOS;
	float4 position : SV_Position;
};

[RootSignature(RootSig)]
VSOutput main(VSInput input)
{
	VSOutput output;

	matrix mvpMatrix = mul(CameraCB.ProjMatrix, mul(CameraCB.ViewMatrix, ModelCB.ModelMatrix));
	float4 worldPos = float4(input.position, 1.0f);

	//output.oPos = mul(ModelCB.ModelMatrix, worldPos);
	output.position = mul(mvpMatrix, worldPos);
	output.uv0 = input.uv0;

	//output.normal = float4(mul(ModelCB.ModelMatrix, input.normal).xyz, 0.0);
	//output.normal = float4(input.normal.xyz, 0.0);

	//output.TBN = float3x3(input.tangent.rgb, input.bitangent.rgb, input.normal.rgb);
	//output.TBN = mul((float3x3)ModelCB.ModelMatrix,output.TBN);

	float4 normal = mul(ModelCB.ModelMatrix, float4(input.normal, 0.0f));
	output.normal = normal;

	// float4 tangent = mul(ModelCB.ModelMatrix, float4(input.tangent, 0.0f));
	// tangent = normalize(tangent);

	// float4 bitangent = mul(ModelCB.ModelMatrix, float4(input.bitangent, 0.0f));
	// bitangent = normalize(bitangent);

	//output.TBN = float3x3(input.tangent.rgb, input.bitangent.rgb, input.normal.rgb);

	output.worldPos = mul(ModelCB.ModelMatrix, worldPos);

	return output;
}