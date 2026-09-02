// SpecularIrradianceCS.cs_5_0.hlsl — Falcom Engine+ generic GGX specular prefilter (Phase 3)
// Port of Skyrim Community Shaders' SpecularIrradianceCS.hlsl (Michał Siejak), generic names.
// GGX NDF importance sampling with 16 Hammersley samples per output texel; mip-filtered
// sampling (solid-angle mip selection) for variance reduction. Input = inferred environment
// cube (with a hardware-generated mip chain); output = per-mip UAV of the filtered cube.
// Input : t0 inferred env cube (mips 0..N-1), s0 linear clamp, b13 { roughness }
// Output: u0 filtered cube mip (bound per-mip array UAV)

static const float Epsilon = 0.00001;

static const uint  NumSamples = 16;
static const float InvNumSamples = 1.0 / float(NumSamples);

cbuffer SpecularFilterCB : register(b13)
{
    float g_roughness;
};

TextureCube<float4> g_inputTex : register(t0);
RWTexture2DArray<float4> g_outTex : register(u0);

SamplerState g_linearClamp : register(s0);

// Compute Van der Corput radical inverse
// See: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;  // / 0x100000000
}

// Sample i-th point from Hammersley point set of NumSamples points total.
float2 sampleHammersley(uint i)
{
    return float2(i * InvNumSamples, radicalInverse_VdC(i));
}

// Importance sample GGX normal distribution function for a fixed roughness value.
// This returns normalized half-vector between Li & Lo.
float3 sampleGGX(float u1, float u2, float roughness)
{
    float alpha = roughness * roughness;

    float cosTheta = sqrt((1.0 - u2) / (1.0 + (alpha * alpha - 1.0) * u2));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);  // Trig. identity
    float phi = 6.283185307179586 * u1;

    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (3.14159265358979323846 * denom * denom);
}

// Calculate normalized sampling direction vector based on current fragment coordinates.
// Uses the bound (per-mip) output UAV dimensions so each mip dispatch uses its own size.
float3 GetSamplingVector(uint3 tid)
{
    uint outW, outH, outD;
    g_outTex.GetDimensions(outW, outH, outD);

    float2 st = float2(tid.xy) / float2(outW, outH);
    float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - 1.0;

    float3 v = 0;
    switch (tid.z)
    {
        case 0: v = float3( 1.0,  uv.y, -uv.x); break;
        case 1: v = float3(-1.0,  uv.y,  uv.x); break;
        case 2: v = float3( uv.x,  1.0, -uv.y); break;
        case 3: v = float3( uv.x, -1.0,  uv.y); break;
        case 4: v = float3( uv.x,  uv.y,  1.0); break;
        case 5: v = float3(-uv.x,  uv.y, -1.0); break;
    }
    return normalize(v);
}

// Compute orthonormal basis for converting from tangent/shading space to world space.
void ComputeBasisVectors(float3 N, out float3 S, out float3 T)
{
    T = cross(N, float3(0.0, 1.0, 0.0));
    T = lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));
    T = normalize(T);
    S = normalize(cross(N, T));
}

// Convert point from tangent/shading space to world space.
float3 TangentToWorld(float3 v, float3 N, float3 S, float3 T)
{
    return S * v.x + T * v.y + N * v.z;
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint outW, outH, outD;
    g_outTex.GetDimensions(outW, outH, outD);
    if (dtid.x >= outW || dtid.y >= outH) return;

    // Input cubemap dimensions at zero mip level.
    uint inW, inH, inLevels;
    g_inputTex.GetDimensions(0, inW, inH, inLevels);

    // Solid angle associated with a single cubemap texel at zero mip level.
    float wt = 4.0 * 3.14159265358979323846 / (6.0 * float(inW) * float(inH));

    // Approximation: assume zero viewing angle (isotropic reflections).
    float3 N = GetSamplingVector(dtid);
    float3 Lo = N;

    float3 S, T;
    ComputeBasisVectors(N, S, T);

    float3 color = 0;
    float weight = 0;

    // Convolve environment map using GGX NDF importance sampling.
    // Weight by cosine term since Epic claims it generally improves quality.
    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 u = sampleHammersley(i);
        float3 Lh = TangentToWorld(sampleGGX(u.x, u.y, g_roughness), N, S, T);

        // Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
        float3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;

        float cosLi = dot(N, Li);
        if (cosLi > 0.0)
        {
            // Use Mipmap Filtered Importance Sampling to improve convergence.
            float cosLh = max(dot(N, Lh), 0.0);

            // GGX NDF (D term) probability density function.
            // Scaling by 1/4 is due to change of density in terms of Lh to Li (and since N=V).
            float pdf = ndfGGX(cosLh, g_roughness) * 0.25;

            // Solid angle associated with this sample.
            float ws = 1.0 / (float(NumSamples) * pdf);

            // Mip level to sample from.
            float mipLevel = max(0.5 * log2(ws / wt) + 1.0, 0.0);

            color += g_inputTex.SampleLevel(g_linearClamp, Li, mipLevel).rgb * cosLi;
            weight += cosLi;
        }
    }
    color /= weight;

    g_outTex[dtid] = float4(max(0.0, color), 1.0);
}