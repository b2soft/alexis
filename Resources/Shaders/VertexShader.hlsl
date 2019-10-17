struct PSInput
{
	float2 uv : TEXCOORD0;
	float4 position : SV_POSITION;
};

struct SceneOffset
{
	float4 test;
};

ConstantBuffer<SceneOffset> cb : register(b0);

PSInput main(float4 position : POSITION, float4 uv : TEXCOORD)
{
	PSInput result;

    result.position = position + cb.test * 0.5;
	result.uv = uv;

	return result;
}