//-----------------------------------------------------------------------------
// MotionBlur_v.hlsl - Migrated to vs_2_0 HLSL using PS_OUTPUT struct
//-----------------------------------------------------------------------------


// Input vertex structure
struct VS_INPUT
{
    float4 position : POSITION0; // Input position (v0)
    float4 color : COLOR0; // Input diffuse color (v1)
    float2 texCoord : TEXCOORD0; // Input texture coordinates (v2)
};

// Output vertex structure (to pixel shader)
struct VS_OUTPUT
{
    float4 position : POSITION; // Output position (oPos) - Must be clip space
    float4 color : COLOR0; // Output diffuse color (oD0)
    float2 texCoord0 : TEXCOORD0; // Output texture coordinates (oT0)
    float2 texCoord1 : TEXCOORD1; // Output texture coordinates (oT1)
    float2 texCoord2 : TEXCOORD2; // Output texture coordinates (oT2)
    float2 texCoord3 : TEXCOORD3; // Output texture coordinates (oT3)
};

// Vertex Shader Main Function
// This shader acts as a pass-through, copying input attributes to output.
// NOTE: It assumes the input position (input.position) is already
// in homogeneous clip space, as there's no transformation here.
// If model*view*projection transformation is needed, it should be added.
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output; // Initialize output structure

    output.position = input.position; // Pass through position (mov oPos, V_POSITION)
    output.color = input.color; // Pass through color (mov oD0, V_DIFFUSE)

    // Pass through the same texture coordinate to multiple stages
    output.texCoord0 = input.texCoord; // (mov oT0, V_TEXTURE)
    output.texCoord1 = input.texCoord; // (mov oT1, V_TEXTURE)
    output.texCoord2 = input.texCoord; // (mov oT2, V_TEXTURE)
    output.texCoord3 = input.texCoord; // (mov oT3, V_TEXTURE)

    return output;
}

