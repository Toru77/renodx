// FalcomSSRCS.cs_5_0.hlsl — Falcom Engine+ generic simple screen-space SSR (compute).
// No Hi-Z, no temporal/motion-vector/denoise. One thread per screen pixel.
// Non-linear (biased) sample distribution along the reflection ray, linearized-depth
// crossing (validated Sora positive-distance convention), binary refinement, then the
// captured frame color. Output RGBA16F: rgb = reflection color, a = combined confidence.
// Confidence = hitConf * distanceConf * edgeConf * grazingConf.
// Input : t0 captured color, t1 captured depth, t2 captured mrtTexture0 (normal),
//         b0 cb_scene, b13 { sampleCount, maxDist, thickness, distanceFade, edgeFade, grazingFade },
//         s0 point clamp
// Output: u0 ssr_raw

cbuffer cb_scene : register(b0)
{
    float4x4 view_g     : packoffset(c0);
    float4x4 viewInv_g  : packoffset(c4);
    float4x4 proj_g     : packoffset(c8);
    float4x4 projInv_g  : packoffset(c12);
};

cbuffer cb_ssr : register(b13)
{
    float g_sampleCount;
    float g_maxDist;
    float g_thickness;
    float g_distanceFade;
    float g_edgeFade;
    float g_grazingFade;
    float g_charOccStrength;
    float g_charOccUpness;
};

Texture2D<float4> g_colorTex : register(t0);
Texture2D<float>  g_depthTex : register(t1);
Texture2D<uint4>  g_mrt0Tex  : register(t2);
SamplerState      g_pointClamp : register(s0);

RWTexture2D<float4> g_out : register(u0);

static const float SSR_FLT_MAX = 3.402823466e+38;
static const float kDistanceBias = 1.5;  // concentrate samples near the origin
static const uint  kBinarySteps = 5u;    // internal binary refinement iterations

// ── Depth linearization (identical math to the validated SSR/GTVBAO path; handles
//    standard and reversed Z by sign guard; gives positive view-space distance). ──
void GetDepthUnpackConsts(out float mul_c, out float add_c)
{
    mul_c = -proj_g[3][2];
    add_c =  proj_g[2][2];
    if (mul_c * add_c < 0.0) add_c = -add_c;
}

float LinearizeDepth(float ndc_depth)
{
    float mul_c, add_c;
    GetDepthUnpackConsts(mul_c, add_c);
    float denom = add_c - ndc_depth;
    float z = (abs(denom) > 1e-8) ? (mul_c / denom) : 0.0;
    z = max(z, 0.0);
    return isfinite(z) && z > 0.0 ? z : SSR_FLT_MAX;
}

// Project a view-space position (negative Z in front) to a screen UV.
float2 ProjectToUV(float3 view_pos)
{
    float4 clip = mul(float4(view_pos, 1.0), proj_g);
    float2 ndc = clip.xy / clip.w;
    return float2(ndc.x * 0.5 + 0.5, 1.0 - (ndc.y * 0.5 + 0.5));
}

// Decode surface normal from mrtTexture0 spherical encoding (validated convention).
float3 DecodeWorldNormal(int2 px, int2 size)
{
    int2 tc = clamp(px, int2(0, 0), size - int2(1, 1));
    uint4 mrt = g_mrt0Tex.Load(int3(tc, 0));
    float2 enc = float2(mrt.x, mrt.y) * (1.0 / 32767.5) - 1.0;
    float azimuth = 3.14159274 * enc.x;
    float ring = sqrt(saturate(1.0 - enc.y * enc.y));
    float3 n = float3(cos(azimuth) * ring, sin(azimuth) * ring, enc.y);
    if (dot(n, n) < 1e-6) n = float3(0.0, 0.0, -1.0);
    return normalize(n);
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint w, h;
    g_out.GetDimensions(w, h);
    if (dtid.x >= w || dtid.y >= h) return;

    const int2 px = int2(dtid.xy);
    const float2 uv = (float2(px) + 0.5) / float2(w, h);

    g_out[px] = float4(0.0, 0.0, 0.0, 0.0);

    const float hw_depth = g_depthTex.Load(int3(px, 0));
    if (hw_depth <= 0.0 || hw_depth >= 1.0) return;  // sky

    // Reconstruct the surface view-space position (validated convention).
    float ndc_x = uv.x * 2.0 - 1.0;
    float ndc_y = 1.0 - uv.y * 2.0;
    float4 clip_pt = float4(ndc_x, ndc_y, hw_depth, 1.0);
    float4 vp = mul(clip_pt, projInv_g);
    float3 P = vp.xyz / vp.w;

    // Normal → view space (empirical-canonical), view vector, reflection.
    float3 n_world = DecodeWorldNormal(px, int2(w, h));
    float3 N = normalize(mul(n_world, (float3x3)view_g));
    float3 V = normalize(-P);
    float nv = dot(N, V);
    if (nv <= 0.0) return;
    float3 R = 2.0 * nv * N - V;  // reflect(-V, N)

    // Non-linear march: samples concentrated near the origin, spreading toward maxDist.
    const float maxDist = max(g_maxDist, 0.001);
    const uint  count = max((uint)g_sampleCount, 2u);
    const float thickness = max(g_thickness, 1e-4);
    const float invCount = 1.0 / float(count);

    float3 cur = P;
    float3 prev = P;  // last in-front point (for binary refinement)
    bool crossed = false;
    float penetration = 0.0;     // rayDist - sceneDist at the crossing step (depth behind)
    float localStepLen = 1.0;    // view-space length of the crossing step
    for (uint i = 1u; i <= count; ++i) {
        float u = float(i) * invCount;
        float t = maxDist * pow(u, kDistanceBias);
        prev = cur;
        cur = P + R * t;
        localStepLen = length(cur - prev);
        float2 suv = ProjectToUV(cur);
        if (any(suv < 0.0) || any(suv > 1.0)) break;  // ray left the screen
        int2 spx = clamp(int2(suv * float2(w, h)), int2(0, 0), int2(w, h) - int2(1, 1));
        float sceneDist = LinearizeDepth(g_depthTex.Load(int3(spx, 0)));
        if (sceneDist >= SSR_FLT_MAX * 0.5) continue;  // sky at this texel
        float rayDist = -cur.z;
        // Hit only when the ray is strictly BEHIND the surface by at least thickness.
        if (rayDist >= sceneDist + thickness) {
            penetration = rayDist - sceneDist;
            crossed = true;
            break;
        }
    }

    if (crossed) {
        // Binary refinement between the last in-front point (prev) and the crossing point (cur).
        for (uint b = 0u; b < kBinarySteps; ++b) {
            float3 mid = (prev + cur) * 0.5;
            float2 muv = ProjectToUV(mid);
            if (any(muv < 0.0) || any(muv > 1.0)) {
                cur = mid;  // clamp toward the crossing side
                continue;
            }
            int2 mpx = clamp(int2(muv * float2(w, h)), int2(0, 0), int2(w, h) - int2(1, 1));
            float mDist = -mid.z;
            float sDist = LinearizeDepth(g_depthTex.Load(int3(mpx, 0)));
            if (mDist >= sDist + thickness) cur = mid; else prev = mid;
        }
        float2 fuv = saturate(ProjectToUV(cur));

        // Confidence factors.
        // hitConf: decisive vs grazing/borderline depth crossing (penetration relative to step).
        float hitConf = saturate(penetration / max(localStepLen, 1e-3));
        // distanceConf: far hits lose authority.
        float hitT = length(cur - P);
        float distanceConf = 1.0 - smoothstep(0.0, 1.0, saturate(hitT / maxDist)) * g_distanceFade;
        // edgeConf: screen-edge vignette.
        float minEdge = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
        float edgeBand = max(g_edgeFade * 0.25, 1e-4);
        float edgeConf = smoothstep(0.0, edgeBand, minEdge);
        // grazingConf: reduce authority at grazing view angles.
        float grazingBand = lerp(0.01, 0.4, g_grazingFade);
        float grazingConf = smoothstep(0.0, grazingBand, nv);

        float conf = saturate(hitConf * distanceConf * edgeConf * grazingConf);

        // Character-induced disocclusion: if the ray from a NON-character (e.g. floor)
        // surface hits a CHARACTER pixel on a HORIZONTAL surface, that character is a
        // foreground occluder between the camera and the true reflected scene, not a
        // genuine reflection target (SSR cannot render mirrored images). Reduce
        // confidence so the Dynamic/Vanilla fallback takes over smoothly. Vertical
        // surfaces (mirrors/walls, low upness) keep the legitimate character reflection.
        if (g_charOccStrength > 0.0f) {
            int2 hitPx = clamp(int2(fuv * float2(w, h)), int2(0, 0), int2(w, h) - int2(1, 1));
            bool charHit  = ((g_mrt0Tex.Load(int3(hitPx, 0)).w & 1u) != 0u);
            bool charOrig = ((g_mrt0Tex.Load(int3(px, 0)).w & 1u) != 0u);
            if (charHit && !charOrig) {
                float upness = abs(n_world.y);
                float upLo = max(g_charOccUpness - 0.25, 0.0);
                float upHi = min(g_charOccUpness + 0.25, 1.0);
                float occFactor = smoothstep(upLo, upHi, upness);
                conf *= 1.0 - g_charOccStrength * occFactor;
            }
        }

        g_out[px] = float4(max(0.0, g_colorTex.SampleLevel(g_pointClamp, fuv, 0).rgb), conf);
    }
}