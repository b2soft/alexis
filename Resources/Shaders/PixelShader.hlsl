struct PSInput
{
	float2 uv : TEXCOORD;
};

//Texture2D g_texture : register(t0);
//SamplerState g_sampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    return 1.0;
    //g_texture.Sample(g_sampler, input.uv);
}