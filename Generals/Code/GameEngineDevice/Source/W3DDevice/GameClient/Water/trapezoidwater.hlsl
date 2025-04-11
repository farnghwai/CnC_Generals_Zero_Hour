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
    
    // mul r0,v0,t0 ; blend vertex color and alpha into base texture. 
    float4 result = input.color * base;
    
    // mad r0.rgb, t1, t2, r0	; blend sparkles and noise 
    result.rgb = mad(sparkle.rgb, noise.rgb, result.rgb); // Use mad intrinsic
    
    // mul r0.rgb, r0, t3 ; blend in black shroud     
    result.rgb = result.rgb * shroud.rgb;
              
    // blend modified image into original image so smooth fade in/out
    output.Color = result;
    
    return output;
}