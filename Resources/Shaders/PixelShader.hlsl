struct PixelShaderInput
{
    float4 normal : NORMAL;
    float2 uv0 : TEXCOORD;
};

float4 main(PixelShaderInput input) : SV_Target
{
    return input.normal;
}