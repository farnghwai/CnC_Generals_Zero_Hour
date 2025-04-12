// PS 1.1 to PS 2.0 HLSL Migration

// Constant registers matching the original shader
sampler2D baseTex : register(s0); // t0
sampler2D bumpTex : register(s1); // t1
sampler2D envMapTex : register(s2); // t2

float4 reflectionModulation : register(c0); // Same as c0 in asm

// Input structure
struct PS_INPUT
{
    float2 texCoord0 : TEXCOORD0; // Base texture (t0)
    float2 texCoord1 : TEXCOORD1; // Bump map (t1)
    float2 texCoord2 : TEXCOORD2; // Environment map (t2) base coords    
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
    
    // tex t1: Sample bump/distortion map
    float4 offset = tex2D(bumpTex, input.texCoord1);
        
    // texbem t2, t1: Sample environment map with perturbed coordinates
    // We approximate 'texbem' by adding an offset derived from the bump map
    // to the environment map's texture coordinates.
    // Often, the bump map stores offsets in xy or rg channels.
    // The exact scaling/interpretation might need adjustment based on how
    // the bump map was created. A common method is to treat the sample
    // as signed offsets (-1 to +1 range) scaled by some factor.
    // Assuming bumpSample.xy contains the raw offsets for now:
    float2 perturbation = (offset.xy * 2.0f) - 1.0f; // Might need: (bumpSample.xy * 2.0 - 1.0) * BumpScale;
    float2 perturbedEnvUV = input.texCoord2 + perturbation * 0.05; // scale offset to simulate bump effect
    float4 reflection = tex2D(envMapTex, perturbedEnvUV);
    
    // mul r0, v0, t0: Blend vertex color with base texture
    float4 result = input.color * base;
    
    // mul r1.rgb, t2, c0: Scale environment map color by constant tint/intensity
    float3 reflectionColor = reflection.rgb * reflectionModulation.rgb;

    // add r0.rgb, r0, r1: Add reflection to the base color
    result.rgb = result.rgb + reflectionColor.rgb;
     
    output.Color = result;
    
    return output;
}