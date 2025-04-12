//-----------------------------------------------------------------------------
// wave_v.hlsl - Migrated to vs_2_0 HLSL using PS_OUTPUT struct
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
// VS 1.1 c[0], c[1] were CV_ZERO, CV_ONE - not typically needed in VS 2.0+
static const int CV_ZERO = 0;
static const int CV_ONE = 1;

static const int CV_WORLDVIEWPROJ_0 = 2;
static const int CV_WORLDVIEWPROJ_1 = 3;
static const int CV_WORLDVIEWPROJ_2 = 4;
static const int CV_WORLDVIEWPROJ_3 = 5;

static const int CV_TEXPROJ_0 = 6;
static const int CV_TEXPROJ_1 = 7;
static const int CV_TEXPROJ_2 = 8;
static const int CV_TEXPROJ_3 = 9;
static const int CV_PATCH_SCALE_OFFSET = 10;

// World * View * Projection Matrix (transposed from VS 1.1 c[2]-c[5])
float4x4 WorldViewProj : register(c2); // World * View * Projection matrix

// Texture Projection parameters (from VS 1.1 c[6])
// c[CV_TEXPROJ_0].xy = scale factors
// c[CV_TEXPROJ_0].zw = offset factors
float4 TexProjParams : register(c6); // Scale and offset for water alpha texture

// Patch Scale/Offset for Alpha Texture UVs (from VS 1.1 c[10])
// c[CV_PATCH_SCALE_OFFSET].xy = offset
// c[CV_PATCH_SCALE_OFFSET].zw = scale
float4 PatchScaleOffset : register(c10); // c5 in VS 2.0

//-----------------------------------------------------------------------------
// Input / Output Structures
//-----------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Position : POSITION0; // Mapped from v0 (V_POSITION)
    float4 Diffuse : COLOR0; // Mapped from v1 (V_DIFFUSE)
    float2 TexCoord0 : TEXCOORD0; // Mapped from v2 (V_TEXTURE)
    // float2 texCoord1 : TEXCOORD1; // Mapped from v3 (V_TEXTURE2) - Not used in original shader
};

struct VS_OUTPUT
{
    float4 Position : POSITION0; // Output Clip Space Position (was oPos)
    float4 Diffuse : COLOR0; // Output Diffuse Color (was oD0)
    float2 TexCoord0 : TEXCOORD0; // Output Bump Map UVs (was oT0)
    float2 TexCoord1 : TEXCOORD1; // Output Projected UVs for Env Map (was oT1)
#ifdef DO_WATER_ALPHA_TEXTURE
    float2 TexCoord2    : TEXCOORD2;    // Output Alpha Map UVs (was oT2)
#endif
};


//-----------------------------------------------------------------------------
// Vertex Shader: main
//-----------------------------------------------------------------------------
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0; // Initialize output structure

    // Transform position to clip space
    // Note: VS 1.1 used dp4 with individual constant vectors (rows).
    // HLSL 2.0 uses mul with a float4x4 matrix. Ensure matrix is loaded correctly (transposed if needed).
    // Assuming WorldViewProj is loaded correctly for standard HLSL mul:
    output.Position = mul(input.Position, WorldViewProj);

    // Pass through the bump map texture coordinates
    output.TexCoord0 = input.TexCoord0;

    // Pass through the diffuse color
    output.Diffuse = input.Diffuse;

    // Calculate projected texture coordinates (often for reflection/refraction)
    // Equivalent to the rcp, mul, mad sequence in VS 1.1 for oT1
    // 1. Perspective divide (clip space -> NDC)
    float2 ndcPos = output.Position.xy / output.Position.w;
    // 2. Scale and offset for screen/texture space
    // VS 1.1: mad oT1.xy, r1.xy, c[CV_TEXPROJ_0].xy, c[CV_TEXPROJ_0].zw
    // r1.xy = ndcPos
    // c[CV_TEXPROJ_0].xy = TexProjParams.xy (scale)
    // c[CV_TEXPROJ_0].zw = TexProjParams.zw (offset)
    output.TexCoord1.xy = ndcPos.xy * TexProjParams.xy + TexProjParams.zw;

    
#ifdef DO_WATER_ALPHA_TEXTURE
    // Generate UV coordinates for the alpha texture based on position XZ
    // VS 1.1: mad oT2.xy, v0.xz, c[CV_PATCH_SCALE_OFFSET].zw, c[CV_PATCH_SCALE_OFFSET].xy
    // v0.xz = input.Position.xz
    // c[CV_PATCH_SCALE_OFFSET].zw = PatchScaleOffset.zw (scale)
    // c[CV_PATCH_SCALE_OFFSET].xy = PatchScaleOffset.xy (offset)
    output.TexCoord2.xy = input.Position.xz * PatchScaleOffset.zw + PatchScaleOffset.xy;
#endif

    return output;
}
