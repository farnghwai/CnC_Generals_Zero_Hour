//-----------------------------------------------------------------------------
// roadnoise2.hlsl - Migrated to PS 2.0 HLSL using PS_OUTPUT struct
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Sampler Declarations
//-----------------------------------------------------------------------------
sampler2D RoadSampler : register(s0); // Original t0: road texture
sampler2D CloudSampler : register(s1); // Original t1: cloud shadow texture
sampler2D NoiseSampler : register(s2); // Original t2: noise texture

//-----------------------------------------------------------------------------
// Input Structure (from Vertex Shader)
//-----------------------------------------------------------------------------
struct PS_INPUT
{
    float2 TexCoord0 : TEXCOORD0; // Texture coordinates for RoadSampler
    float2 TexCoord1 : TEXCOORD1; // Texture coordinates for CloudSampler
    float2 TexCoord2 : TEXCOORD2; // Texture coordinates for NoiseSampler
    float4 Color : COLOR0; // Interpolated diffuse color (original v0)
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
    float4 roadColor = tex2D(RoadSampler, input.TexCoord0); // Original: tex t0
    float4 cloudColor = tex2D(CloudSampler, input.TexCoord1); // Original: tex t1
    float4 noiseColor = tex2D(NoiseSampler, input.TexCoord2); // Original: tex t2

    // modulate with cloud
    // mul r0, t0, t1
    float4 result = roadColor * cloudColor;
    
    // modulate with noise
    // mul r0, r0, t2
    result = result * noiseColor; 
    
    // apply diffuse lighting
    // mul r0, r0, v0
    result = result * input.Color;
    
    // Assign the final calculated color to the output structure member
    output.Color = result;

    // Return the entire output structure
    return output;
}

