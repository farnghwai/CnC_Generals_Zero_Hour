// PS 1.1 to PS 2.0 HLSL Migration

// Constant registers matching the original shader
float3 ColorConversionFactor : register(c0); // Used in dot product
float3 ScaleFactor : register(c1); // Used for scaling
float4 BlendColor : register(c2); // Used in interpolation

// Texture sampler (same as original shader's t0)
sampler2D TextureSampler : register(s0);

// Input structure
struct PS_INPUT
{
    float2 TexCoord : TEXCOORD0;
};

// Output structure
struct PS_OUTPUT
{
    float4 Color : COLOR0;
};

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    // Sample texture (equivalent to "tex t0")
    float4 texColor = tex2D(TextureSampler, input.TexCoord);
    
    // black & white conversion
    // Dot product (equivalent to "dp3 r1, t0, c0")
    float dotResult = dot(texColor.rgb, ColorConversionFactor);
    
    // modulate by filter color (inverse of red)
    // Multiply by scale factor (equivalent to "mul r1, r1, c1")
    float3 scaledResult = dotResult * ScaleFactor;
           
    // Linear interpolation (equivalent to "lrp r0, c2, 1-r1, t0")
    // Note: lrp(x,y,s) = x*(1-s) + y*s, but in the original code it's c2*(1-r1) + t0*r1
    float3 oneMinusR1 = 1.0 - scaledResult;
    float4 modifiedColor = float4(oneMinusR1, texColor.a); // Preserve original alpha

    // blend modified image into original image so smooth fade in/out
    output.Color = lerp(texColor, modifiedColor, BlendColor);
    
    return output;
}