// PS 1.1 to PS 2.0 HLSL Migration

// Constant registers matching the original shader
sampler2D baseTex : register(s0); // t0
sampler2D sparkleTex : register(s1); // t1
sampler2D noiseTex : register(s2); // t2
sampler2D shroudTex : register(s3); // t3

// Input structure
struct PS_INPUT
{
    float2 texCoord0 : TEXCOORD0; // For baseTex
    float2 texCoord1 : TEXCOORD1; // For sparkleTex
    float2 texCoord2 : TEXCOORD2; // For noiseTex
    float2 texCoord3 : TEXCOORD3; // For shroudTex
    float4 color : COLOR0; // Interpolated vertex color (v0 in ps.1.1) 
};

// Output structure
struct PS_OUTPUT
{
    float4 Color : COLOR0;
};

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float4 base = tex2D(baseTex, input.texCoord0);
    float4 sparkle = tex2D(sparkleTex, input.texCoord1);
    float4 noise = tex2D(noiseTex, input.texCoord2);
    float4 shroud = tex2D(shroudTex, input.texCoord3);
                   
    // mul r0, v0, t0 ; blend vertex color into t0.
    float4 result0 = input.color * base;

    // mul r1, t1, t2 ; mul
    float4 result1 = sparkle * noise;

    // add r0.rgb, r0, t3
    result0.rgb = result0.rgb + shroud.rgb;

    // +mul r0.a, r0, t3
    result0.a = result0.a * shroud.a;

    // add r0.rgb, r0, r1
    result0.rgb = result0.rgb + result1.rgb;
    
    // blend modified image into original image so smooth fade in/out
    output.Color = result0;
    
    return output;
}