//-----------------------------------------------------------------------------
// wave_p.hlsl - Migrated to PS 2.0 HLSL using PS_OUTPUT struct
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Samplers
//-----------------------------------------------------------------------------
sampler2D BumpSampler : register(s0); // Texture for bump map (was implicitly stage 1 for texbem)
sampler2D EnvMapSampler : register(s1); // Texture for environment map (was implicitly stage 1 result)
#ifdef DO_WATER_ALPHA_TEXTURE
sampler2D AlphaSampler  : register(s2); // Texture for alpha (was implicitly stage 2)
#endif

// Optional: Add a constant for bump scaling if needed
// float BumpScale : register(c??); // Example: Adjust bump intensity


//-----------------------------------------------------------------------------
// Input / Output Structures
//-----------------------------------------------------------------------------
// Input structure must match VS_OUTPUT structure from vertex shader
struct PS_INPUT
{
    float4 DiffuseColor : COLOR0; // Interpolated Diffuse Color (was v0)
    float2 TexCoord0 : TEXCOORD0; // Bump Map UVs (was t0 used by texbem)
    float2 TexCoord1 : TEXCOORD1; // Base Env Map UVs (was t1 before texbem modification)
#ifdef DO_WATER_ALPHA_TEXTURE
    float2 TexCoord2    : TEXCOORD2;    // Alpha Map UVs (was t2)
#endif
};

// Output structure for pixel color
struct PS_OUTPUT
{
    float4 Color : COLOR0; // Output pixel color (was r0)
};


//-----------------------------------------------------------------------------
// Pixel Shader: main
//-----------------------------------------------------------------------------
PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output; // Initialize output structure

    // PS 1.1 Breakdown:
    // tex t0          - Define t0 input texcoord register (used below)
    // texbem t1, t0   - Sample bump map at stage 1 using coords t0.
    //                 - Calculate perturbed coordinates based on bump sample.
    //                 - Store perturbed coordinates in temporary register t1.
    // mul r0, t1, v0  - Sample texture at stage 1 (EnvMapSampler) using perturbed coords from t1.
    //                 - Multiply result by interpolated vertex color v0.

    // PS 2.0 Implementation:
    // 1. Sample the Bump Map using the coordinates from TEXCOORD0 (t0)
    //    texbem often assumes the bump map stores signed offsets or normals.
    //    Assuming it stores a normal/vector in xy, needing expansion from [0,1] to [-1,1]
    float2 bumpVector = (tex2D(BumpSampler, input.TexCoord0).xy * 2.0f) - 1.0f;

    // Optional: Scale the bump vector if needed
    // bumpVector *= BumpScale;

    // 2. Perturb the Environment Map coordinates (TEXCOORD1) using the bump vector
    //    The exact perturbation method depends on how texbem was configured/used.
    //    A common method is simple addition:
    float2 perturbedEnvCoords = input.TexCoord1 + bumpVector.xy * 0.05;
    //    Note: More complex EMBM might involve matrix transforms or different scaling.
    //    This assumes texbem used t0 for bump lookup and perturbed t1 coordinates.

    // 3. Sample the Environment Map using the perturbed coordinates
    float4 envColor = tex2D(EnvMapSampler, perturbedEnvCoords);

#ifndef DO_WATER_ALPHA_TEXTURE

    // 4. Multiply environment map color by interpolated vertex diffuse color
    float4 result = envColor * input.DiffuseColor;
    output.Color = result;

#else // DO_WATER_ALPHA_TEXTURE

    // Alternate version using alpha texture

    // 4a. Sample the Alpha Texture using coordinates from TEXCOORD2 (t2)
    float4 alphaTex = tex2D(AlphaSampler, input.TexCoord2);

    // 4b. Multiply environment map color by interpolated vertex diffuse color for RGB
    float4 r0 = (float4) 0;    
    r0.rgb = envColor.rgb * input.DiffuseColor.rgb;

    // 4c. Multiply alpha texture value by interpolated vertex diffuse alpha for Alpha
    //     Assuming alpha is stored in the alpha channel of AlphaSampler.
    //     The original "+mul r0.a, t2, v0" is ambiguous. '+' usually means add-to-register,
    //     but r0.a wasn't initialized. Assuming a standard modulation was intended:
    r0.a = alphaTex.a * input.DiffuseColor.a;
    //     Alternatively, if alpha was stored in .r channel of t2:
    //     output.Color.a = alphaTex.r * input.DiffuseColor.a;

    output.Color = r0;
    
#endif // DO_WATER_ALPHA_TEXTURE

    return output;
}
