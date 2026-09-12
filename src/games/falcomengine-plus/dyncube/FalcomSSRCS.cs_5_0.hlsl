// FalcomSSRCS.cs_5_0.hlsl — Falcom Engine+ generic simple screen-space SSR (compute).
// No Hi-Z, no temporal/motion-vector/denoise. One thread per screen pixel.
// McGuire-style perspective-correct screen-space march (uniform pixel steps,
// near-plane clipped), linearized-depth interval crossing (validated Sora
// positive-distance convention), binary refinement, then the
// captured frame color. Output RGBA16F: rgb = reflection color, a = combined confidence.
// Confidence = hitConf * distanceConf * edgeConf * grazingConf.
// Input : t0 captured color, t1 captured depth, t2 captured mrtTexture0 (normal),
//         t3 IS-FAST noise volume (128x128x32 RG8; fallback SRV when unavailable),
//         b0 cb_scene, b13 { sampleCount, maxDist, thickness, distanceFade, edgeFade,
//         grazingFade, charOccStrength, charOccUpness, isfastEnabled, isfastBound,
//         isfastFrame, isfastStrength, isfastSpatial, isfastTemporal, isfastSeed, pad },
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
    float g_isfastEnabled;   // effective IS-FAST gate (master && SSR toggle), 0/1
    float g_isfastBound;     // 1 = noise texture available (else hash fallback)
    float g_isfastFrame;     // frame_index % 64 (temporal slice source)
    float g_isfastStrength;  // [0..1] blend hash phase -> noise phase
    float g_isfastSpatial;   // [0.25..4] noise spatial scale
    float g_isfastTemporal;  // [0..5] noise animation speed, 0 = frozen slice
    float g_isfastSeed;      // seed offset [0..64]
    float g_charComp;        // 0 = char bit in mrt .w (Sora), 1 = mrt .z shifted bit (Kai)
    float g_charShift;       // bit shift applied to the selected component (Sora 0, Kai 8)
    float g_isfastPad;       // padding (push-constant alignment)
};

Texture2D<float4> g_colorTex : register(t0);
Texture2D<float>  g_depthTex : register(t1);
Texture2D<uint4>  g_mrt0Tex  : register(t2);
Texture3D<float2> g_isfastNoise : register(t3);  // IS-FAST spatio-temporal blue noise (128x128x32 RG8)
SamplerState      g_pointClamp : register(s0);

RWTexture2D<float4> g_out : register(u0);

#include "dyncube_common.hlsli"

static const uint  kBinarySteps = 5u;    // internal binary refinement iterations

// Project a view-space position (negative Z in front) to a screen UV.
float2 ProjectToUV(float3 view_pos)
{
    float4 clip = mul(float4(view_pos, 1.0), proj_g);
    float2 ndc = clip.xy / clip.w;
    return float2(ndc.x * 0.5 + 0.5, 1.0 - (ndc.y * 0.5 + 0.5));
}

// Decode surface normal from mrtTexture0 spherical encoding (validated convention).
// Optionally returns the raw packed texel so callers needing its bit flags
// (e.g. the character bit) don't fetch the same texel twice.
float3 DecodeWorldNormal(int2 px, int2 size, out uint4 mrtRaw)
{
    int2 tc = clamp(px, int2(0, 0), size - int2(1, 1));
    mrtRaw = g_mrt0Tex.Load(int3(tc, 0));
    uint4 mrt = mrtRaw;
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
    // mrtOrigin reuses this exact fetch for the character-bit test below.
    uint4 mrtOrigin;
    float3 n_world = DecodeWorldNormal(px, int2(w, h), mrtOrigin);
    float3 N = normalize(mul(n_world, (float3x3)view_g));
    float3 V = normalize(-P);
    float nv = dot(N, V);
    if (nv <= 0.0) return;
    float3 R = 2.0 * nv * N - V;  // reflect(-V, N)

    // McGuire-style march: g_sampleCount caps traversal iterations, g_maxDist caps
    // the endpoint only (uniform pixel spacing, no world-distance distribution).
    const float maxDist = max(g_maxDist, 0.001);
    const uint  count = max((uint)g_sampleCount, 2u);
    const float thickness = max(g_thickness, 1e-4);
    // Depth-unpack constants are uniform for the whole dispatch; derive once
    // instead of per depth sample (identical values, less ALU).
    float ssaUnpackMul, ssaUnpackAdd;
    DynCubeGetDepthUnpackConsts(ssaUnpackMul, ssaUnpackAdd);

    // Clip the endpoint so the projected segment never goes behind the camera
    // (negative w would mirror UVs into a false in-bounds result). w(t) is linear.
    float4 clipP = mul(float4(P, 1.0), proj_g);
    float4 clipD = mul(float4(R, 0.0), proj_g);
    if (clipP.w <= 0.0) return;  // origin behind camera (should not happen for rasterized pixels)
    float tEnd = maxDist;
    if (clipD.w < -1e-9) {
        float wMin = max(clipP.w * 0.01, 1e-4);
        tEnd = min(tEnd, (wMin - clipP.w) / clipD.w);
    }
    if (tEnd <= 1e-4) return;  // no forward ray extent
    float3 endP = P + R * tEnd;

    // Project origin/clipped endpoint to pixel space (ProjectToUV convention).
    // Homogeneous (Q, k) interpolation keeps the 3D position perspective-correct.
    float4 c0 = clipP;
    float4 c1 = clipP + clipD * tEnd;
    float k0 = 1.0 / c0.w;
    float k1 = 1.0 / c1.w;
    float3 Q0 = P * k0;
    float3 Q1 = endP * k1;
    float2 pixScale = float2(w, h);
    float2 P0 = float2(c0.x * k0 * 0.5 + 0.5, 1.0 - (c0.y * k0 * 0.5 + 0.5)) * pixScale;
    float2 P1 = float2(c1.x * k1 * 0.5 + 0.5, 1.0 - (c1.y * k1 * 0.5 + 0.5)) * pixScale;

    // Degenerate projection (ray at a pixel): cover at least one pixel.
    float2 pixDelta = P1 - P0;
    if (dot(pixDelta, pixDelta) < 0.0001) {
        P1 += float2(0.01, 0.0);
        pixDelta = P1 - P0;
    }

    // Permute so x is the major axis (deterministic, no jitter).
    bool permute = abs(pixDelta.x) < abs(pixDelta.y);
    if (permute) {
        pixDelta = pixDelta.yx;
        P0 = P0.yx;
        P1 = P1.yx;
    }
    float stepDir = (pixDelta.x >= 0.0) ? 1.0 : -1.0;
    float invdx = stepDir / pixDelta.x;
    float2 dP = float2(stepDir, pixDelta.y * invdx);
    float3 dQ = (Q1 - Q0) * invdx;
    float dk = (k1 - k0) * invdx;

    // DDA iteration budget remains fixed.
    // Adaptive stride distributes that budget across the projected
    // major-axis segment so long rays can reach the endpoint without
    // increasing the number of depth samples.
    // This restores ray reach but increases screen-space spacing,
    // so thin features may be skipped at large stride.
    float majorLen = abs(pixDelta.x);
    float stride = max(1.0, ceil(majorLen / max((float)count, 1.0)));
    dP *= stride;
    dQ *= stride;
    dk *= stride;

    float2 originPix = float2(px) + 0.5;
    // DDA start phase: deterministic integer-hash by default; IS-FAST
    // spatio-temporal blue noise when enabled. Same phase offsets walk/Q/k
    // so they stay synchronized on the original camera-space ray.
    uint phaseHash = (uint)(px.x) * 1973u + (uint)(px.y) * 9277u + 26699u;
    phaseHash = (phaseHash ^ (phaseHash >> 13)) * 1274126177u;
    phaseHash ^= phaseHash >> 16;
    float hashPhase = (float)(phaseHash & 1023u) * (1.0 / 1024.0);
    float phase = hashPhase;
    if (g_isfastEnabled > 0.5 && g_isfastBound > 0.5) {
        // Manual frac wrap: g_pointClamp is point-clamp, not point-wrap
        // (same pattern as Kai shadow IS-FAST sampling).
        float2 nxy = frac((float2((uint)px.x, (uint)px.y) + 0.5) / 128.0 * max(g_isfastSpatial, 1e-4));
        float nz = frac((((float)(((uint)g_isfastFrame + (uint)max(g_isfastSeed, 0.0)) % 32u)) + 0.5) / 32.0 * max(g_isfastTemporal, 0.0));
        float noisePhase = g_isfastNoise.SampleLevel(g_pointClamp, float3(nxy, nz), 0).x;
        phase = lerp(hashPhase, noisePhase, saturate(g_isfastStrength));
    }
    float2 walk = P0 + dP * phase;
    float3 Q = Q0 + dQ * phase;
    float k = k0 + dk * phase;
    float end = P1.x * stepDir;

    float3 cur = P;
    float3 prev = P;  // previous march sample (step length + refinement fallback)
    float prevRayDist = -P.z;
    float3 bracketLo = P;      // most recent strictly-in-front sample (depthDiff < 0)
    bool hasBracketLo = false; // true once such a sample has been observed
    bool crossed = false;
    float penetration = 0.0;     // rayDist - sceneDist at the crossing step (depth behind)
    float localStepLen = 1.0;    // view-space length of the crossing step
    for (uint step = 0u; step < count && (walk.x * stepDir) <= end; ++step) {
        float2 hitPixF = permute ? walk.yx : walk;
        float3 stepPos = Q * (1.0 / k);
        float stepRayDist = -stepPos.z;
        if (all(abs(hitPixF - originPix) < 2.0)) {
            // Self-hit guard (~2 texels, AMD rule): advance state, skip testing.
            prev = stepPos;
            prevRayDist = stepRayDist;
        } else if (any(hitPixF < 0.0) || any(hitPixF >= float2(w, h))) {
            break;  // ray left the screen: miss (march UVs are never clamped)
        } else {
            int2 spx = int2(hitPixF);
            float sceneDist = DynCubeLinearizeDepth(g_depthTex.Load(int3(spx, 0)), ssaUnpackMul, ssaUnpackAdd);
            if (sceneDist < DynCubeFltMax * 0.5) {
                float rayLo = min(prevRayDist, stepRayDist);
                float rayHi = max(prevRayDist, stepRayDist);
                // Depth-interval overlap: ray slab vs [sceneDist, sceneDist + thickness].
                // Hit only when the ray is strictly BEHIND the surface by at least thickness.
                if (rayHi >= sceneDist && rayLo <= sceneDist + thickness) {
                    cur = stepPos;
                    penetration = stepRayDist - sceneDist;
                    localStepLen = length(cur - prev);
                    crossed = true;
                    break;
                }
                // Track the most recent strictly-in-front sample for zero-crossing refinement.
                if (stepRayDist - sceneDist < 0.0) {
                    bracketLo = stepPos;
                    hasBracketLo = true;
                }
            }
            prev = stepPos;
            prevRayDist = stepRayDist;
        }
        walk += dP;
        Q += dQ;
        k += dk;
    }

    if (crossed) {
        // Thickness gates coarse acceptance only; refinement targets the actual depth
        // crossing (depthDiff = 0) so Thickness never offsets the final hit position.
        // bracketLo is guaranteed strictly in front; otherwise fall back to prev
        // (bounded: within one coarse step of detection, near the true crossing).
        if (hasBracketLo) prev = bracketLo;
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
            float sDist = DynCubeLinearizeDepth(g_depthTex.Load(int3(mpx, 0)), ssaUnpackMul, ssaUnpackAdd);
            if (mDist >= sDist) cur = mid; else prev = mid;
        }
        float2 fuvRaw = ProjectToUV(cur);
        if (any(fuvRaw < 0.0) || any(fuvRaw > 1.0)) return;  // refined hit left the screen: miss
        float2 fuv = fuvRaw;
        // Backface rejection (AMD rule): a hit whose outward normal faces along the
        // reflection ray was struck from behind and carries wrong-side content.
        // Only the sign of the dot product matters, and the decode output has
        // nonzero length by construction, so normalization cannot flip the sign.
        uint4 mrtHitUnused;
        float3 hitN_view = mul(DecodeWorldNormal(int2(fuv * float2(w, h)), int2(w, h), mrtHitUnused), (float3x3)view_g);
        if (dot(hitN_view, R) > 0.0) return;

        // Confidence factors.
        // hitConf: decisive vs grazing/borderline depth crossing (sink rate past
        // the surface, relative to step). Thickness gates acceptance above and is
        // deliberately NOT subtracted here: penetration and localStepLen scale
        // together with DDA stride, so this ratio is iteration-count invariant,
        // while a fixed thickness penalty would grow as steps get finer.
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

        static const float kSsrExistFloor = 0.35;  // hit-existence preference floor (no UI yet)
        float existConf = lerp(kSsrExistFloor, 1.0, hitConf);
        float conf = saturate(existConf * distanceConf * edgeConf * grazingConf);

        // Character-induced disocclusion: if the ray from a NON-character (e.g. floor)
        // surface hits a CHARACTER pixel on a HORIZONTAL surface, that character is a
        // foreground occluder between the camera and the true reflected scene, not a
        // genuine reflection target (SSR cannot render mirrored images). Reduce
        // confidence so the Dynamic/Vanilla fallback takes over smoothly. Vertical
        // surfaces (mirrors/walls, low upness) keep the legitimate character reflection.
        if (g_charOccStrength > 0.0f) {
            int2 hitPx = clamp(int2(fuv * float2(w, h)), int2(0, 0), int2(w, h) - int2(1, 1));
            uint4 hitMrt = g_mrt0Tex.Load(int3(hitPx, 0));
            uint hitWord = (g_charComp > 0.5f) ? hitMrt.z : hitMrt.w;
            uint origWord = (g_charComp > 0.5f) ? mrtOrigin.z : mrtOrigin.w;
            bool charHit  = (((hitWord >> (uint)g_charShift) & 1u) != 0u);
            bool charOrig = (((origWord >> (uint)g_charShift) & 1u) != 0u);
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