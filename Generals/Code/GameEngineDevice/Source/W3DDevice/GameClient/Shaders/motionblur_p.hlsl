//-----------------------------------------------------------------------------
// motionblur_p.hlsl - Migrated to PS 2.0 HLSL using PS_OUTPUT struct
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Sampler Inputs (matching original t0, t1, t2, t3 texture stages)
//-----------------------------------------------------------------------------
sampler2D BaseTexSampler : register(s0); // Corresponds to original t0
sampler2D BumpMapSampler : register(s1); // Corresponds to original t1 (used for bump coords/sampling)
sampler2D EnvMapSampler : register(s2); // Corresponds to original t2 (sampled by texbem)
sampler2D MotionBlurSampler : register(s3); // Corresponds to original t3

//-----------------------------------------------------------------------------
// Uniform Constants (matching original c0)
//-----------------------------------------------------------------------------
float4 EnvMapColorFactor : register(c0); // Corresponds to original c0

//-----------------------------------------------------------------------------
// Input structure from Vertex Shader
//-----------------------------------------------------------------------------
struct PS_INPUT
{
    float4 color : COLOR0; // Interpolated vertex color (v0)
    float2 texCoord0 : TEXCOORD0; // Texture coordinates for Tex0
    float2 texCoord1 : TEXCOORD1; // Texture coordinates for Tex1 (bump map)
    float2 texCoord2 : TEXCOORD2; // Base texture coordinates for Tex2 (environment map)
    float2 texCoord3 : TEXCOORD3; // Texture coordinates for Tex3
};

//-----------------------------------------------------------------------------
// Output structure for Pixel Shader
//-----------------------------------------------------------------------------
struct PS_OUTPUT
{
    float4 color : COLOR0; // Output pixel color (r0)
};

//-----------------------------------------------------------------------------
// Pixel Shader Main Function
//-----------------------------------------------------------------------------
PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;

    // Sample the base texture (original: tex t0)
    float4 baseTexColor = tex2D(BaseTexSampler, input.texCoord0); // t0

    // Sample the bump map (original: tex t1 used as input to texbem)
    // texbem t2, t1: Samples bump map (t1), perturbs t1 coords, samples env map (t2)
    // We assume the bump map stores offsets in R and G channels, range [0, 1]
    float4 bumpSample = tex2D(BumpMapSampler, input.texCoord1); // Sample t1

    // Convert bump map sample [0, 1] to offset [-1, 1] for perturbation
    // Note: Original texbem might have included matrix transforms or different scaling.
    // This is a common basic implementation.
    float2 perturbation = (bumpSample.rg * 2.0f) - 1.0f;

    // Calculate perturbed texture coordinates for the environment map lookup
    // Adds the offset derived from the bump map (t1) to the base coordinates (t1)
    float2 perturbedEnvUV = input.texCoord2 + perturbation * 0.05;

    // Sample the environment map using the perturbed coordinates (original: texbem t2, t1)
    float4 envMapColor = tex2D(EnvMapSampler, perturbedEnvUV); // t2 result

    // Sample the motion blur texture (original: tex t3)
    float4 motionBlurTexColor = tex2D(MotionBlurSampler, input.texCoord3); // t3

    // Temporary registers equivalent to r0, r1
    float4 r0 = (float4) 0;
    
    // Calculate intermediate result r0 (original: mul r0.rgb, v0, t0)
    r0.rgb = input.color.rgb * baseTexColor.rgb;
    // Calculate intermediate result r0 (original: +add r0.a, v0, t0)
    r0.a = input.color.a + baseTexColor.a; // saturate handles the '+' prefix

    // Calculate intermediate result r1 (original: mul r1, t2, c0)
    float4 r1 = envMapColor * EnvMapColorFactor; // r1

    // Add environment map contribution (original: add r0.rgb, r0, r1)
    r0.rgb = r0.rgb + r1.rgb;

    // Modulate alpha by motion blur texture alpha (original: +mul r0.a, r0, t3)
    r0.a = r0.a * motionBlurTexColor.a; // saturate handles the '+' prefix

    // Final output color (original result was in r0)
    output.color = r0;

    return output;
}

