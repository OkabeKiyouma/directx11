cbuffer constants : register(b1)
{
    float2 offset;
};

struct VS_Input
{
    float3 pos : POS;
    float4 color : COL;
    float2 uv :TEX;
};

struct VS_Output
{
    float4 position : SV_POSITION;
    float4 color : COL;
    float2 uv : TEXCOORD;
};

Texture2D myTexture : register(t0);
SamplerState mySampler : register(s0);

VS_Output vs_main(VS_Input input)
{
    VS_Output output;
    output.position = float4(input.pos.xy+offset,input.pos.z, 1.0f);
    output.color = input.color;
    output.uv = input.uv;
    //output.color = float4(1.0,1.0,1.0,1.0);

    return output;
}

float4 ps_main(VS_Output input) : SV_TARGET
{
    return input.color * myTexture.Sample(mySampler, input.uv);
}
