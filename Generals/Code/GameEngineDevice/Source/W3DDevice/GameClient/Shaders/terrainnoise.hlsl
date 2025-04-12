//-----------------------------------------------------------------------------
// terrainnoise.hlsl - Migrated to PS 2.0 HLSL using PS_OUTPUT struct
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Sampler Declarations
//-----------------------------------------------------------------------------
sampler2D Tex0 : register(s0); // Base layer 1
sampler2D Tex1 : register(s1); // Base layer 2
sampler2D Tex2 : register(s2); // Modulator 1

//-----------------------------------------------------------------------------
// Input Structure (from Vertex Shader)
//-----------------------------------------------------------------------------
struct PS_INPUT
{
    float4 Color : COLOR0; // Interpolated vertex color (v0)
    float2 TexCoord : TEXCOORD0; // Texture coordinates
};

//-----------------------------------------------------------------------------
// Output Structure (to Render Target)
//-----------------------------------------------------------------------------
// PS_OUTPUT is a user-defined name, not a keyword.Color
// It must contain at least one member with a COLOR semantic
// to write to the render target.
struct PS_OUTPUT
{
    float4 Color : COLOR0; // Output color for the pixel
    // You could add more outputs here for Multiple Render Targets (MRTs)
    // e.g., float4 NormalData : COLOR1;
};

//-----------------------------------------------------------------------------
// Pixel Shader Main Function (PS 2.0)
//-----------------------------------------------------------------------------
// The main function now returns the PS_OUTPUT structure
PS_OUTPUT main(PS_INPUT input)
{
    // Declare the output structure variable
    PS_OUTPUT output;

    // Sample the textures
    float4 texColor0 = tex2D(Tex0, input.TexCoord);
    float4 texColor1 = tex2D(Tex1, input.TexCoord);
    float4 texColor2 = tex2D(Tex2, input.TexCoord);

    // Perform calculations
    // lrp r0, v0.a, t1, t0 ; alpha blend between 2 textures
    // lerp(a, b, s) = a + s*(b-a) = (1-s)*a + s*b
    // PS 1.1 lrp dst, factor, src1, src0 calculates (factor * src1) + ((1 - factor) * src0)
    // So, lerp(texColor0, texColor1, input.Color.a) matches the original logic.    
    float4 result = lerp(texColor0, texColor1, input.Color.a); // Blend base layers
    
    // mul r0, r0, v0 ; apply diffuse lighting (and potentially tint)
    result = result * input.Color; // Apply lighting/tint
    
    // mul r0, r0, t2 ; modulate with texture 2    
    result = result * texColor2; // Modulate
    
    // Assign the final calculated color to the output structure member
    output.Color = result;

    // Return the entire output structure
    return output;
}

