struct PixelShaderInput
{
    float4 normal : NORMAL;
    float2 uv0 : TEXCOORD;
};

Texture2D DiffuseTexture : register(t0);

//SamplerState LinearRepeatSampler : register(s0);
SamplerState AnisotropicSampler : register(s0);

float4 main(PixelShaderInput input) : SV_Target
{
    //float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, input.uv0);
    float4 texColor = DiffuseTexture.Sample(AnisotropicSampler, input.uv0);

    //return float4(input.uv0, 0.0, 1.0);

    return texColor;
}