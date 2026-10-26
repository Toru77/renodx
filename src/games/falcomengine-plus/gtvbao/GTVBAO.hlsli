///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2021, Intel Corporation 
// 
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GTVBAO is based on GTAO/GTSO "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion", 
// https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
// 
// Implementation:  Filip Strugar (filip.strugar@intel.com), Steve Mccalla <stephen.mccalla@intel.com>         (\_/)
// Version:         (see GTVBAO.h)                                                                            (='.'=)
// Details:         https://github.com/GameTechDev/GTVBAO                                                     (")_(")
//
// Version history: see GTVBAO.h
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "GTVBAO.h"

#define GT_VBAO_PI               	(3.1415926535897932384626433832795)
#define GT_VBAO_PI_HALF             (1.5707963267948966192313216916398)
#define GT_VBAO_RCP_PI              (0.31830988618379067153776752674503)
#define GT_VBAO_RCP_PI_HALF         (0.63661977236758134307553505349006)

#ifdef GTVBAO_GI
#define GTVBAO_AO_OUT_TYPE RWTexture2D<float4>
#else
#define GTVBAO_AO_OUT_TYPE RWTexture2D<uint>
#endif

#ifndef GT_VBAO_USE_HALF_FLOAT_PRECISION
#define GT_VBAO_USE_HALF_FLOAT_PRECISION 1
#endif

#if defined(GT_VBAO_FP32_DEPTHS) && GT_VBAO_USE_HALF_FLOAT_PRECISION
#error Using GT_VBAO_USE_HALF_FLOAT_PRECISION with 32bit depths is not supported yet unfortunately (it is possible to apply fp16 on parts not related to depth but this has not been done yet)
#endif 


#if (GT_VBAO_USE_HALF_FLOAT_PRECISION != 0)
#if 1 // old fp16 approach (<SM6.2)
    typedef min16float      lpfloat; 
    typedef min16float2     lpfloat2;
    typedef min16float3     lpfloat3;
    typedef min16float4     lpfloat4;
    typedef min16float3x3   lpfloat3x3;
#else // new fp16 approach (requires SM6.2 and -enable-16bit-types) - WARNING: perf degradation noticed on some HW, while the old (min16float) path is mostly at least a minor perf gain so this is more useful for quality testing
    typedef float16_t       lpfloat; 
    typedef float16_t2      lpfloat2;
    typedef float16_t3      lpfloat3;
    typedef float16_t4      lpfloat4;
    typedef float16_t3x3    lpfloat3x3;
#endif
#else
    typedef float           lpfloat;
    typedef float2          lpfloat2;
    typedef float3          lpfloat3;
    typedef float4          lpfloat4;
    typedef float3x3        lpfloat3x3;
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Inputs are screen XY and viewspace depth, output is viewspace position
float3 GTVBAO_ComputeViewspacePosition( const float2 screenPos, const float viewspaceDepth, const GTAOConstants consts )
{
    float3 ret;
    ret.xy = (consts.NDCToViewMul * screenPos.xy + consts.NDCToViewAdd) * viewspaceDepth;
    ret.z = viewspaceDepth;
    return ret;
}

float GTVBAO_ScreenSpaceToViewSpaceDepth( const float screenDepth, const GTAOConstants consts )
{
    float depthLinearizeMul = consts.DepthUnpackConsts.x;
    float depthLinearizeAdd = consts.DepthUnpackConsts.y;
    // Optimised version of "-cameraClipNear / (cameraClipFar - projDepth * (cameraClipFar - cameraClipNear)) * cameraClipFar"
    return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

lpfloat4 GTVBAO_CalculateEdges( const lpfloat centerZ, const lpfloat leftZ, const lpfloat rightZ, const lpfloat topZ, const lpfloat bottomZ )
{
    lpfloat4 edgesLRTB = lpfloat4( leftZ, rightZ, topZ, bottomZ ) - (lpfloat)centerZ;

    lpfloat slopeLR = (edgesLRTB.y - edgesLRTB.x) * 0.5;
    lpfloat slopeTB = (edgesLRTB.w - edgesLRTB.z) * 0.5;
    lpfloat4 edgesLRTBSlopeAdjusted = edgesLRTB + lpfloat4( slopeLR, -slopeLR, slopeTB, -slopeTB );
    edgesLRTB = min( abs( edgesLRTB ), abs( edgesLRTBSlopeAdjusted ) );
    return lpfloat4(saturate( ( 1.25 - edgesLRTB / (centerZ * 0.011) ) ));
}

float3 GTVBAO_CalculateNormal( const float4 edgesLRTB, float3 pixCenterPos, float3 pixLPos, float3 pixRPos, float3 pixTPos, float3 pixBPos )
{
    // Get this pixel's viewspace normal
    float4 acceptedNormals  = saturate( float4( edgesLRTB.x*edgesLRTB.z, edgesLRTB.z*edgesLRTB.y, edgesLRTB.y*edgesLRTB.w, edgesLRTB.w*edgesLRTB.x ) + 0.01 );

    pixLPos = normalize(pixLPos - pixCenterPos);
    pixRPos = normalize(pixRPos - pixCenterPos);
    pixTPos = normalize(pixTPos - pixCenterPos);
    pixBPos = normalize(pixBPos - pixCenterPos);

    float3 pixelNormal =  acceptedNormals.x * cross( pixLPos, pixTPos ) +
                        + acceptedNormals.y * cross( pixTPos, pixRPos ) +
                        + acceptedNormals.z * cross( pixRPos, pixBPos ) +
                        + acceptedNormals.w * cross( pixBPos, pixLPos );
    pixelNormal = normalize( pixelNormal );

    return pixelNormal;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Visibility Bitmask helpers (Therrien/Levesque/Gilet 2023)
// Replaces GTAO's two horizon angles with a uint bitmask of N_b sectors.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GT_VBAO_BITMASK_SECTOR_COUNT 32u


// ═══════════════════════════════════════════════════════════════
// GTVBAO helpers — Ground Truth Visibility Bitmask AO upgrades
// ═══════════════════════════════════════════════════════════════

// bufferc.txt:490-504
float GTVBAO_SliceRelCDF_Uniform(float x, float angN, bool isPhiLargerThanAngN)
{
    if (x <= 0.0f || x >= 1.0f) return x;

    float phi = x * GT_VBAO_PI - GT_VBAO_PI_HALF;

    bool c = isPhiLargerThanAngN;

    float m0 = c ? 2.0f : 0.0f;
    float m1 = c ? -1.0f : 1.0f;

    float d0 = 0.5f * (m0 + m1 * cos(angN - phi) + sin(angN));

    return d0;
}

// bufferc.txt:506-522
float GTVBAO_SliceRelCDF_Cos(float x, float angN, float cosN, bool isPhiLargerThanAngN)
{
    if (x <= 0.0f || x >= 1.0f) return x;

    float phi = x * GT_VBAO_PI - GT_VBAO_PI_HALF;

    bool c = isPhiLargerThanAngN;

    float n0 = c ? 3.0f : 1.0f;
    float n1 = c ? -1.0f : 1.0f;
    float n2 = c ? 4.0f : 0.0f;

    float t0 = n0 * cosN + n1 * cos(angN - 2.0f * phi) + (n2 * angN + (n1 * 2.0f) * phi + GT_VBAO_PI) * sin(angN);
    float t1 = 4.0f * (cosN + angN * sin(angN));

    return t0 / (abs(t1) > 1e-6f ? t1 : 1e-6f);
}

// bufferc.txt:339-359
float4 GTVBAO_GetQuaternion(float3 to)
{
    float3 xyz = float3(to.y, -to.x, 0.0f);
    float s = -to.z;

    float u = rsqrt(max(0.0f, s * 0.5f + 0.5f));
    s = 1.0f / u;
    xyz *= u * 0.5f;

    return float4(xyz, s);
}

// bufferc.txt:372-382
float3 GTVBAO_Transform_Qz0(float3 v, float4 q)
{
    float k = v.y * q.x - v.x * q.y;
    float g = 2.0f * (v.z * q.w + k);

    float3 r;
    r.xy = v.xy + q.yx * float2(g, -g);
    r.z = v.z + 2.0f * (q.w * k - v.z * dot(q.xy, q.xy));

    return r;
}

// bufferc.txt:385-395
float3 GTVBAO_Transform_Vz0Qz0(float2 v, float4 q)
{
    float o = q.x * v.y;
    float c = q.y * v.x;

    float3 b = float3(o - c, -o + c, o - c);

    return float3(v, 0.0f) + 2.0f * (b * float3(q.y, q.x, q.w));
}

// bufferc.txt:170-236 (HQ branch, 196-198)
float2 GTVBAO_SampleSliceDir(float3 vvsN, float rnd01)
{
    float ang0 = rnd01 * GT_VBAO_PI;

    float2 dir0 = float2(cos(ang0), sin(ang0));

    float l = length(vvsN.xy);

    if (l == 0.0f) return dir0;

    // flip dir0 into hemi-circle of rsN.xy
    dir0 *= dot(dir0, vvsN.xy) < 0.0f ? -1.0f : 1.0f;

    float2 n = vvsN.xy / l;

    float2 dir;
    {
        float x = dir0.x * n.y - dir0.y * n.x;

        float s = l;
        s += (s - s * s) * 0.15f;

        float y = acos(clamp(x * sin(s * GT_VBAO_PI_HALF), -1.0f, 1.0f)) * GT_VBAO_RCP_PI_HALF;

        float ys = 1.0f / max(s, 1e-6f);
        dir.y = ys - ys * y;
        dir.x = sqrt(clamp(1.0f - dir.y * dir.y, 0.0f, 1.0f));
    }

    return float2(dir.x * n.x - dir.y * n.y,
                  dir.y * n.x + dir.x * n.y);
}

// Screen-space direction (pixels, unit length) of the view-space direction D at P.
float2 GTVBAO_ScreenDirFromViewDir(float3 P, float3 D, GTAOConstants consts)
{
    float2 dNdc = (D.xy - (P.xy / P.z) * D.z) / P.z;
    float2 dUv = dNdc / consts.NDCToViewMul;
    float2 dPx = dUv / consts.ViewportPixelSize;
    return dPx * rsqrt(max(dot(dPx, dPx), 1e-12f));
}

void GTVBAO_OutputWorkingTerm( const uint2 pixCoord, lpfloat visibility, GTVBAO_AO_OUT_TYPE outWorkingAOTerm )
{
    if (GTVBAO_final_pass < 0.5f)
        visibility = visibility / lpfloat(GT_VBAO_OCCLUSION_TERM_SCALE);
    visibility = saturate( visibility );
    outWorkingAOTerm[pixCoord] = uint(visibility * 255.0 + 0.5);
}

void GTVBAO_MainPass( const uint2 pixCoord, lpfloat sliceCount, lpfloat stepsPerSlice, const lpfloat2 localNoise, lpfloat3 viewspaceNormal, const GTAOConstants consts, 
    Texture2D<lpfloat> sourceViewspaceDepth, SamplerState depthSampler, GTVBAO_AO_OUT_TYPE outWorkingAOTerm,
    Texture2D<uint4> mrtNormalTexture,
    Texture2D<uint> foliageMaskTexture,
    RWTexture2D<float4> outDebug
    )
{                                                                       
    float2 normalizedScreenPos = (pixCoord + float2( 0.5, 0.5 )) * consts.ViewportPixelSize;

    const bool g_trackDebug = GTVBAO_debug_mode > 21.5f;

    // ── Low-resolution mode: output domain is 1/d per axis, depth input stays full. ──
    // Full path below is untouched; low-res uses block-center point loads
    // (GTVBAO_RepresentativePixel, odd-safe) instead of Gather.
    // The point loads are deliberate: GatherRed's centre component is a
    // *neighbour* of its anchor texel, so gathering here would feed a 2x2
    // block-edge depth in as the pixel centre and corrupt pixCenterPos and
    // the edge term.
    uint g_fullDepthW, g_fullDepthH;
    sourceViewspaceDepth.GetDimensions(g_fullDepthW, g_fullDepthH);
    const bool g_lowRes = GTVBAO_resolution > 0.5f;
    const int2 g_workDims = g_lowRes ? consts.ViewportSize : int2(g_fullDepthW, g_fullDepthH);

    lpfloat viewspaceZ;
    lpfloat pixLZ, pixTZ, pixRZ, pixBZ;
    if (!g_lowRes) {
      lpfloat4 valuesUL   = sourceViewspaceDepth.GatherRed( depthSampler, float2( pixCoord * consts.ViewportPixelSize )               );
      lpfloat4 valuesBR   = sourceViewspaceDepth.GatherRed( depthSampler, float2( pixCoord * consts.ViewportPixelSize ), int2( 1, 1 ) );
      viewspaceZ  = valuesUL.y;
      pixLZ = valuesUL.x;
      pixTZ = valuesUL.z;
      pixRZ = valuesBR.z;
      pixBZ = valuesBR.x;
    } else {
      int2 htc = clamp(int2(pixCoord), int2(0, 0), max(g_workDims - 1, int2(0, 0)));
      int2 ftc = GTVBAO_RepresentativePixel(htc, int2(g_fullDepthW, g_fullDepthH));
      int2 fmax = int2(max((int)g_fullDepthW - 1, 0), max((int)g_fullDepthH - 1, 0));
      viewspaceZ = (lpfloat)sourceViewspaceDepth.Load(int3(ftc, 0));
      pixLZ = (lpfloat)sourceViewspaceDepth.Load(int3(clamp(ftc + int2(-1, 0), int2(0, 0), fmax), 0));
      pixRZ = (lpfloat)sourceViewspaceDepth.Load(int3(clamp(ftc + int2( 1, 0), int2(0, 0), fmax), 0));
      pixTZ = (lpfloat)sourceViewspaceDepth.Load(int3(clamp(ftc + int2( 0,-1), int2(0, 0), fmax), 0));
      pixBZ = (lpfloat)sourceViewspaceDepth.Load(int3(clamp(ftc + int2( 0, 1), int2(0, 0), fmax), 0));
    }

#ifdef GTVBAO_GI
    if (viewspaceZ >= 60000.0) {
        outWorkingAOTerm[pixCoord] = float4(0.0f, 0.0f, 0.0f, -1.0f);
        return;
    }
#endif

    // Cache texture dimensions for foliage mask checks (only when mask is fresh this frame).
    uint g_mrtW, g_mrtH;
    mrtNormalTexture.GetDimensions(g_mrtW, g_mrtH);
    uint g_maskW = 1, g_maskH = 1;
    if (GTVBAO_exclude_foliage > 0.5f && GTVBAO_foliage_mask_valid > 0.5f)
        foliageMaskTexture.GetDimensions(g_maskW, g_maskH);

    lpfloat4 edgesLRTB  = GTVBAO_CalculateEdges( (lpfloat)viewspaceZ, (lpfloat)pixLZ, (lpfloat)pixRZ, (lpfloat)pixTZ, (lpfloat)pixBZ );
    // ── Foliage detection (toggle on) ──
    // Foliage PS sets o1.w bit 15 (0x8000) to mark the pixel.
    // When detected: normal AO is fully computed, then blended toward
    // fully bright (255) by (1 - foliage_ao_value) after GTVBAO_OutputWorkingTerm.
    //   foliage_ao_value = 1.0 → keep normal AO (blend factor 0)
    //   foliage_ao_value = 0.0 → fully bright (blend factor 1)
    bool gtvbao_foliage_pixel = false;
    bool checkFoliage = GTVBAO_exclude_foliage > 0.5f;
    if (checkFoliage) {
      // Low-resolution modes test the whole d x d block, so the mark cannot be
      // missed by the parity the single-texel ratio mapping happens to select.
      gtvbao_foliage_pixel = GTVBAO_FoliageMarked(
          mrtNormalTexture, pixCoord, (uint2)g_workDims, uint2(g_fullDepthW, g_fullDepthH),
          uint2(g_mrtW, g_mrtH), GTVBAO_foliage_channel_mode >= 0.5f, (uint)GTVBAO_Divisor());
    }

    // Move center pixel slightly towards camera to avoid imprecision artifacts due to depth buffer imprecision; offset depends on depth texture format used
#ifdef GT_VBAO_FP32_DEPTHS
    viewspaceZ *= 0.99999;     // this is good for FP32 depth buffer
#else
    viewspaceZ *= 0.99920;     // this is good for FP16 depth buffer
#endif

    const float3 pixCenterPos   = GTVBAO_ComputeViewspacePosition( normalizedScreenPos, viewspaceZ, consts );
    const lpfloat3 viewVec      = (lpfloat3)normalize(-pixCenterPos);
    
    // prevents normals that are facing away from the view vector - GTVBAO struggles with extreme cases, but in Vanilla it seems rare so it's disabled by default
    // viewspaceNormal = normalize( viewspaceNormal + max( 0, -dot( viewspaceNormal, viewVec ) ) * viewVec );

    const lpfloat effectRadius              = (lpfloat)consts.EffectRadius * (lpfloat)consts.RadiusMultiplier;
    const lpfloat sampleDistributionPower   = (lpfloat)consts.SampleDistributionPower;
    const lpfloat bitmaskThickness          = (lpfloat)consts.BitmaskThickness;

    lpfloat visibility = 0;
    // ── Per-pixel setup (bufferc.txt:628-633) ──
    const float3 V          = (float3)viewVec;
    const float3 N          = (float3)viewspaceNormal;
    const float3 positionVS = pixCenterPos;
    const int  cosineMode   = (int)GTVBAO_gtvbao_cosine_mode;
    const bool useUniform   = GTVBAO_hemisphere_weighting > 0.5f;
    const float4 Q_toV      = GTVBAO_GetQuaternion(V);
    const float4 Q_fromV    = Q_toV * float4(-1.0f, -1.0f, -1.0f, 1.0f);
    float3 normalVVS        = N;
    if (!useUniform && cosineMode == 2) normalVVS = GTVBAO_Transform_Qz0(N, Q_fromV);
    bool degenerate = false;
    // ── Debug state: first slice's bitmask (view 25) and slice AO sums (view 24) ──
    uint debugFirstSliceBitmask = 0u;
    bool debugFirstSliceSaved = false;
    // float, not lpfloat, so the accumulation stays in fp32.
    float dbgSliceAOSum = 0.0f;        // sum of per-slice unweighted sliceAO
    float dbgSliceN = 0.0f;            // number of slices accumulated
#ifdef GTVBAO_GI
    float3 giSum = 0.0f;               // sum of per-slice GI
    float occSum = 0.0f;               // sum of per-slice occluded fraction (for view 31)
#endif

    {
        // quality settings / tweaks / hacks
        const lpfloat pixelTooCloseThreshold  = 1.3;      // if the offset is under approx pixel size (pixelTooCloseThreshold), push it out to the minimum distance

        // approx viewspace pixel size at pixCoord; approximation of NDCToViewspace( normalizedScreenPos.xy + consts.ViewportPixelSize.xy, pixCenterPos.z ).xy - pixCenterPos.xy;
        const float2 pixelDirRBViewspaceSizeAtCenterZ = float2( viewspaceZ, viewspaceZ ) * consts.NDCToViewMul_x_PixelSize;

        lpfloat screenspaceRadius   = effectRadius / (lpfloat)pixelDirRBViewspaceSizeAtCenterZ.x;

        // fade out for small screen radii
        visibility += saturate((10 - screenspaceRadius)/100)*0.5;

#if 0   // sensible early-out for even more performance; disabled because not yet tested
        [branch]
        if( screenspaceRadius < pixelTooCloseThreshold )
        {
            GTVBAO_OutputWorkingTerm( pixCoord, 1, outWorkingAOTerm );
            return;
        }
#endif

        // this is the min distance to start sampling from to avoid sampling from the center pixel (no useful data obtained from sampling center pixel)
        const lpfloat minS = (lpfloat)pixelTooCloseThreshold / screenspaceRadius;

        for (int slice = 0; slice < (int)sliceCount; slice++)
        {
            // Random numbers (deviation 1)
            const float2 nz  = (float2)localNoise;
            const float  k   = (float)slice;
            const float2 p   = frac(float2(nz.x + k * 0.7548776662466927f, nz.y + k * 0.5698402909980532f));
            const float4 rnd = float4(p.x, p.y, frac(p.x + 2.0f * p.y), frac(2.0f * p.x + p.y));

            // ── Slice direction ──
            float3 smplDirVS;
            if (useUniform || cosineMode == 0) {
                // bufferc.txt:547-569 (mode 1), 654-677 (uniform)
                float2 dir0 = float2(cos(rnd.x * GT_VBAO_PI), sin(rnd.x * GT_VBAO_PI));
                smplDirVS = GTVBAO_Transform_Vz0Qz0(dir0, Q_toV);
            } else if (cosineMode == 1) {
                // bufferc.txt:571-622
                float2 s2 = rnd.xy * 2.0f - 1.0f;
                float ang = GT_VBAO_PI * s2.x;
                float s1p = sqrt(saturate(1.0f - s2.y * s2.y));
                float3 sph = GTVBAO_Transform_Qz0(float3(float2(cos(ang), sin(ang)) * s1p, s2.y), Q_toV);
                float3 cosDirVS = normalize(sph + N);
                smplDirVS = normalize(cosDirVS - V * dot(V, cosDirVS));
            } else {
                // bufferc.txt:624-651
                float2 dir0 = GTVBAO_SampleSliceDir(normalVVS, rnd.x);
                smplDirVS = GTVBAO_Transform_Vz0Qz0(dir0, Q_toV);
            }
            const float2 screenDir = GTVBAO_ScreenDirFromViewDir(positionVS, smplDirVS, consts);

            // ── Slice construction ──
            // bufferc.txt:681-701
            const float3 sliceN      = cross(V, smplDirVS);
            const float3 projN       = N - sliceN * dot(N, sliceN);
            const float  projNSqrLen = dot(projN, projN);
            if (projNSqrLen < 1e-12f) {
#ifdef GTVBAO_GI
                outWorkingAOTerm[pixCoord] = float4(0.0f, 0.0f, 0.0f, 1.0f);
                return;
#else
                degenerate = true;
                continue;
#endif
            }
            const float  projNRcpLen = rsqrt(projNSqrLen);
            const float  cosN        = dot(projN, V) * projNRcpLen;
            const float3 T           = cross(sliceN, projN);
            const float  sgn         = dot(V, T) < 0.0f ? -1.0f : 1.0f;
            const float  angN        = sgn * acos(clamp(cosN, -1.0f, 1.0f));
            const float  angOff      = angN * GT_VBAO_RCP_PI + 0.5f;

            // ── Horizons (bufferc.txt:705-795) ──
#ifdef GTVBAO_GI
            float3 gi0 = 0.0f;
#endif
            uint occBits = 0u;
            for (int side = 0; side < 2; side++)
            {
                // d = -1 first with jitter rnd.z, then d = +1 with 1 - rnd.z (bufferc.txt:715-717)
                const float d = side == 0 ? -1.0f : 1.0f;
                const float z = side == 0 ? rnd.z : 1.0f - rnd.z;
                const bool  isPhiLargerThanAngN = d > 0.0f;

                [loop]
                for (int i = 0; i < (int)stepsPerSlice; i++)
                {
                    lpfloat s = (lpfloat)(((float)i + z) / (float)stepsPerSlice);
                    // pow(s,1)==s: skip the pow when distribution is exactly 1.0.
                    if (sampleDistributionPower != 1.0f) {
                      // Closed forms for the exponents reachable from the UI's
                      // common values: one sqrt / one multiply instead of the
                      // log2+exp2 pair a generic pow() needs.
                      if (sampleDistributionPower == 1.5f)      s = (lpfloat)(s * sqrt(s));
                      else if (sampleDistributionPower == 2.0f) s = (lpfloat)(s * s);
                      else if (sampleDistributionPower == 3.0f) s = (lpfloat)(s * s * s);
                      else                                      s = (lpfloat)pow( s, (lpfloat)sampleDistributionPower );
                    }
                    s += minS;

                    const lpfloat sampleOffsetLength = (lpfloat)(s * screenspaceRadius);
                    const lpfloat mipLevel = (lpfloat)clamp( log2( sampleOffsetLength ) - consts.DepthMIPSamplingOffset, 0, GT_VBAO_DEPTH_MIP_LEVELS );
                    const float2 sampleOffset = round(d * sampleOffsetLength * screenDir) * consts.ViewportPixelSize;
                    const float2 sampleScreenPos = normalizedScreenPos + sampleOffset;
                    // Samples outside the screen contribute nothing (bufferc.txt:726)
                    if (sampleScreenPos.x < 0.0f || sampleScreenPos.x > 1.0f || sampleScreenPos.y < 0.0f || sampleScreenPos.y > 1.0f) continue;

                    float SZ = sourceViewspaceDepth.SampleLevel( depthSampler, sampleScreenPos, mipLevel ).x;
                    // ── Foliage cast-AO prevention: skip samples that land on foliage ──
                    // Only when the mask is fresh (pre-pass dispatched this frame).
                    if (GTVBAO_exclude_foliage > 0.5f && GTVBAO_foliage_mask_valid > 0.5f && !gtvbao_foliage_pixel) {
                        int2 mc = int2(saturate(sampleScreenPos) * float2(g_maskW, g_maskH));
                        if (foliageMaskTexture.Load(int3(mc, 0)) != 0u) continue;
                    }
                    float3 samplePos = GTVBAO_ComputeViewspacePosition( sampleScreenPos, SZ, consts );

                    // bufferc.txt:732-740 (constant thickness in view depth, deviation 5)
                    float3 deltaPosFront = samplePos - positionVS;
                    float3 deltaPosBack  = GTVBAO_ComputeViewspacePosition( sampleScreenPos, SZ + bitmaskThickness, consts ) - positionVS;

                    // bufferc.txt:749-758
                    float2 horCos = float2(dot(normalize(deltaPosFront), V), dot(normalize(deltaPosBack), V));
                    float2 horAng = acos(clamp(horCos, -1.0f, 1.0f)) * d;
                    float2 hor01  = saturate(horAng * GT_VBAO_RCP_PI + angOff);
                    if (d < 0.0f) hor01 = hor01.yx;

                    // bufferc.txt:760-767 (cosine) / 769-779 (uniform)
                    if (useUniform) {
                        hor01.x = GTVBAO_SliceRelCDF_Uniform(hor01.x, angN, isPhiLargerThanAngN);
                        hor01.y = GTVBAO_SliceRelCDF_Uniform(hor01.y, angN, isPhiLargerThanAngN);
                    } else {
                        hor01.x = GTVBAO_SliceRelCDF_Cos(hor01.x, angN, cosN, isPhiLargerThanAngN);
                        hor01.y = GTVBAO_SliceRelCDF_Cos(hor01.y, angN, cosN, isPhiLargerThanAngN);
                    }
                    hor01 = saturate(hor01 + rnd.w * (1.0f / 32.0f));

                    // bufferc.txt:783-791
                    uint2 horInt = uint2(floor(hor01 * 32.0f));
                    uint mX = horInt.x < 32u ? (0xFFFFFFFFu << horInt.x) : 0u;
                    uint mY = horInt.y != 0u ? (0xFFFFFFFFu >> (32u - horInt.y)) : 0u;
                    uint sampleMask = mX & mY;

#ifdef GTVBAO_GI
                    uint visBits0 = sampleMask & ~occBits;
                    if (visBits0 != 0u) {
                        const float3 sampleRad = 1.0f;
                        float vis0 = 1.0f;
                        if (GTVBAO_vbgi_backface_mode > 0.5f && GTVBAO_mrt_normal_available > 0.5f) {
                            const float3 D0 = DecodeFalcomMrtNormal(
                                mrtNormalTexture.Load(int3(GTVBAO_MrtTexel(sampleScreenPos * float2(g_mrtW, g_mrtH), float2(g_mrtW, g_mrtH)), 0)).xy);
                            if (FalcomNormalValid(D0)) {
                                const float3 N0 = TransformNormalToView(D0);
                                const float3 projN0 = N0 - sliceN * dot(N0, sliceN);
                                const float projN0SqrLen = dot(projN0, projN0);
                                if (projN0SqrLen != 0.0f) {
                                    const float u = dot(projN, projN0) * projNRcpLen * rsqrt(projN0SqrLen);
                                    if (GTVBAO_vbgi_backface_mode < 1.5f) {
                                        vis0 = clamp((1.0f - u) * 2.0f, 0.0f, 1.0f);
                                    } else {
                                        const bool flipT = dot(T, N0) < 0.0f;
                                        float hor = acos(clamp(u, -1.0f, 1.0f)) * GT_VBAO_RCP_PI;
                                        if (flipT) hor = 1.0f - hor;
                                        const bool isPhiLargerThanAngN = hor * GT_VBAO_PI - GT_VBAO_PI_HALF > angN;
                                        if (useUniform) hor = GTVBAO_SliceRelCDF_Uniform(hor, angN, isPhiLargerThanAngN);
                                        else            hor = GTVBAO_SliceRelCDF_Cos(hor, angN, cosN, isPhiLargerThanAngN);
                                        hor = saturate(hor + rnd.w * (1.0f / 32.0f));
                                        uint horInt = (uint)floor(hor * 32.0f);
                                        uint visBitsN = horInt < 32u ? (0xFFFFFFFFu << horInt) : 0u;
                                        if (!flipT) visBitsN = ~visBitsN;
                                        visBits0 &= visBitsN;
                                    }
                                }
                            }
                        }
                        gi0 += sampleRad * ((float)countbits(visBits0) / (float)GT_VBAO_BITMASK_SECTOR_COUNT * vis0);
                    }
                    occBits |= sampleMask;
#else
                    occBits |= sampleMask;
#endif
                }
            }

            // ── Debug: save first slice's final bitmask ──
            if (g_trackDebug && !debugFirstSliceSaved) {
                debugFirstSliceBitmask = occBits;
                debugFirstSliceSaved = true;
            }

            // ── Slice AO and weight (bufferc.txt:797-809) ──
            const float occ = (float)countbits(occBits) / (float)GT_VBAO_BITMASK_SECTOR_COUNT;
#ifdef GTVBAO_GI
            giSum += gi0;
            occSum += occ;
#else
            float sliceWeight = 1.0f;
            if (!useUniform && cosineMode == 0) {
                sliceWeight = max(0.0f, (1.0f / projNRcpLen) * (cosN + angN * sin(angN)));
            }
            visibility += sliceWeight - sliceWeight * occ;
#endif

            // ── Slice AO accumulation for the debug view (24) ──
            if (g_trackDebug) {
                dbgSliceAOSum  += 1.0f - occ;
                dbgSliceN      += 1.0f;
            }
        }

        // bufferc.txt:812 (a degenerate slice makes the pixel fully visible, bufferc.txt:689)
        if (degenerate) visibility = 1.0f;
        else visibility /= (lpfloat)sliceCount;
        visibility = pow( visibility, (lpfloat)consts.FinalValuePower );
        visibility = max( (lpfloat)0.03, visibility ); // disallow total occlusion (which wouldn't make any sense anyhow since pixel is visible but also helps with packing bent normals)
    }

#ifdef GTVBAO_GI
    const float3 giAvg = giSum / (float)sliceCount;
    outWorkingAOTerm[pixCoord] = float4(giAvg, 1.0f);
    // GI debug views 30-31 (32-35 stay zero until implemented). Same full-res block write as the AO views.
    if (g_trackDebug) {
        float4 dbgValue = float4(0.0f, 0.0f, 0.0f, 1.0f);
        if ((int)GTVBAO_debug_mode == 30) {
            dbgValue = float4(giAvg, 1.0f);
        } else if ((int)GTVBAO_debug_mode == 31) {
            float rawGI = giAvg.r + (1.0f - occSum / (float)sliceCount);
            dbgValue = float4(rawGI, rawGI, rawGI, 1.0f);
        }
        GTVBAO_WriteDebug(outDebug, pixCoord, dbgValue);
    }
#else
    GTVBAO_OutputWorkingTerm( pixCoord, visibility, outWorkingAOTerm );

    // ── Foliage post-blend (no early-out, normal AO computed first) ──
    // Blend written AO toward 255 (fully visible) based on foliage_ao_value.
    //   value=1 → blend=0 → keep AO unchanged
    //   value=0 → blend=1 → fully bright (AO=1.0)
    if (gtvbao_foliage_pixel && GTVBAO_exclude_foliage > 0.5f) {
      float aoBlend = 1.0f - saturate(GTVBAO_foliage_ao_value);
      uint cur = outWorkingAOTerm[pixCoord];
      uint blended = (uint)(lerp((float)cur, 255.0f, aoBlend) + 0.5f);
      outWorkingAOTerm[pixCoord] = blended;
    }

    // ── Debug views 22-26 (written last, after the AO and the foliage blend) ──
    // Each view fills the full-res d x d block that this AO pixel covers, so
    // Low-resolution mode has no holes. The write never changes the AO.
    if (g_trackDebug) {
        int dbgMode = (int)GTVBAO_debug_mode;
        float4 dbgValue = float4(0.0f, 0.0f, 0.0f, 1.0f);
        if (dbgMode == GTVBAO_DEBUG_RAW_AO) {
            // Stored AO byte of this frame, before temporal and filter.
            float rawAO = (float)outWorkingAOTerm[pixCoord] / 255.0f;
            if (GTVBAO_final_pass < 0.5f) rawAO *= 1.5f;
            dbgValue = float4(rawAO, rawAO, rawAO, 1.0f);
        } else if (dbgMode == GTVBAO_DEBUG_TEMPORAL) {
            // Default blue; the temporal pass overwrites it with valid_fraction when it runs.
            dbgValue = float4(0.0f, 0.0f, 0.5f, 1.0f);
        } else if (dbgMode == GTVBAO_DEBUG_SLICE_AO) {
            float sliceAO = (dbgSliceN > 0.0f) ? dbgSliceAOSum / dbgSliceN : 0.0f;
            dbgValue = float4(sliceAO, sliceAO, sliceAO, 1.0f);
        } else if (dbgMode == GTVBAO_DEBUG_BITMASK) {
            // First slice's final mask: popcount, lowest set bit, highest set bit.
            uint bm = debugFirstSliceBitmask;
            if (bm != 0u) {
                dbgValue = float4((float)countbits(bm) / 32.0f,
                                  (float)firstbitlow(bm) / 31.0f,
                                  (float)firstbithigh(bm) / 31.0f, 1.0f);
            }
        } else if (dbgMode == GTVBAO_DEBUG_NORMAL) {
            float3 nrm = (float3)viewspaceNormal;
            if (!all(nrm == 0.0f)) dbgValue = float4(nrm * 0.5f + 0.5f, 1.0f);
        }
        GTVBAO_WriteDebug(outDebug, pixCoord, dbgValue);
    }
#endif
}

static const float GTVBAO_DEPTH_MIP_RANGE = 0.31875f;   // 0.75 * radius 0.25 * multiplier 1.7 (shipped defaults)
static const float GTVBAO_DEPTH_MIP_FALLOFF = 0.615f;

// weighted average depth filter
lpfloat GTVBAO_DepthMIPFilter( lpfloat depth0, lpfloat depth1, lpfloat depth2, lpfloat depth3 )
{
    lpfloat maxDepth = max( max( depth0, depth1 ), max( depth2, depth3 ) );

    const lpfloat falloffRange              = (lpfloat)GTVBAO_DEPTH_MIP_FALLOFF * (lpfloat)GTVBAO_DEPTH_MIP_RANGE;
    const lpfloat falloffFrom       = (lpfloat)GTVBAO_DEPTH_MIP_RANGE * ((lpfloat)1-(lpfloat)GTVBAO_DEPTH_MIP_FALLOFF);
    // fadeout precompute optimisation
    const lpfloat falloffMul        = (lpfloat)-1.0 / ( falloffRange );
    const lpfloat falloffAdd        = falloffFrom / ( falloffRange ) + (lpfloat)1.0;

    lpfloat weight0 = saturate( (maxDepth-depth0) * falloffMul + falloffAdd );
    lpfloat weight1 = saturate( (maxDepth-depth1) * falloffMul + falloffAdd );
    lpfloat weight2 = saturate( (maxDepth-depth2) * falloffMul + falloffAdd );
    lpfloat weight3 = saturate( (maxDepth-depth3) * falloffMul + falloffAdd );

    lpfloat weightSum = weight0 + weight1 + weight2 + weight3;
    return (weight0 * depth0 + weight1 * depth1 + weight2 * depth2 + weight3 * depth3) / weightSum;
}

// This is also a good place to do non-linear depth conversion for cases where one wants the 'radius' (effectively the threshold between near-field and far-field AO), 
// is required to be non-linear (i.e. very large outdoors environments).
lpfloat GTVBAO_ClampDepth( float depth )
{
#ifdef GT_VBAO_USE_HALF_FLOAT_PRECISION
    return (lpfloat)clamp( depth, 0.0, 65504.0 );
#else
    return clamp( depth, 0.0, 3.402823466e+38 );
#endif
}

groupshared lpfloat g_scratchDepths[8][8];
void GTVBAO_PrefilterDepths16x16( uint2 dispatchThreadID /*: SV_DispatchThreadID*/, uint2 groupThreadID /*: SV_GroupThreadID*/, const GTAOConstants consts, Texture2D<float> sourceNDCDepth, SamplerState depthSampler, RWTexture2D<lpfloat> outDepth0, RWTexture2D<lpfloat> outDepth1, RWTexture2D<lpfloat> outDepth2, RWTexture2D<lpfloat> outDepth3, RWTexture2D<lpfloat> outDepth4 )
{
    // MIP 0
    const uint2 baseCoord = dispatchThreadID;
    const uint2 pixCoord = baseCoord * 2;
    float4 depths4 = sourceNDCDepth.GatherRed( depthSampler, float2( pixCoord * consts.ViewportPixelSize ), int2(1,1) );
    lpfloat depth0 = GTVBAO_ClampDepth( GTVBAO_ScreenSpaceToViewSpaceDepth( depths4.w, consts ) );
    lpfloat depth1 = GTVBAO_ClampDepth( GTVBAO_ScreenSpaceToViewSpaceDepth( depths4.z, consts ) );
    lpfloat depth2 = GTVBAO_ClampDepth( GTVBAO_ScreenSpaceToViewSpaceDepth( depths4.x, consts ) );
    lpfloat depth3 = GTVBAO_ClampDepth( GTVBAO_ScreenSpaceToViewSpaceDepth( depths4.y, consts ) );
    outDepth0[ pixCoord + uint2(0, 0) ] = (lpfloat)depth0;
    outDepth0[ pixCoord + uint2(1, 0) ] = (lpfloat)depth1;
    outDepth0[ pixCoord + uint2(0, 1) ] = (lpfloat)depth2;
    outDepth0[ pixCoord + uint2(1, 1) ] = (lpfloat)depth3;

    // MIP 1
    lpfloat dm1 = GTVBAO_DepthMIPFilter( depth0, depth1, depth2, depth3 );
    outDepth1[ baseCoord ] = (lpfloat)dm1;
    g_scratchDepths[ groupThreadID.x ][ groupThreadID.y ] = dm1;

    GroupMemoryBarrierWithGroupSync( );

    // MIP 2
    [branch]
    if( all( ( groupThreadID.xy % uint2( 2, 2 ) ) == uint2( 0, 0 ) ) )
    {
        lpfloat inTL = g_scratchDepths[groupThreadID.x+0][groupThreadID.y+0];
        lpfloat inTR = g_scratchDepths[groupThreadID.x+1][groupThreadID.y+0];
        lpfloat inBL = g_scratchDepths[groupThreadID.x+0][groupThreadID.y+1];
        lpfloat inBR = g_scratchDepths[groupThreadID.x+1][groupThreadID.y+1];

        lpfloat dm2 = GTVBAO_DepthMIPFilter( inTL, inTR, inBL, inBR );
        outDepth2[ baseCoord / 2 ] = (lpfloat)dm2;
        g_scratchDepths[ groupThreadID.x ][ groupThreadID.y ] = dm2;
    }

    GroupMemoryBarrierWithGroupSync( );

    // MIP 3
    [branch]
    if( all( ( groupThreadID.xy % uint2( 4, 4 ) ) == uint2( 0, 0 ) ) )
    {
        lpfloat inTL = g_scratchDepths[groupThreadID.x+0][groupThreadID.y+0];
        lpfloat inTR = g_scratchDepths[groupThreadID.x+2][groupThreadID.y+0];
        lpfloat inBL = g_scratchDepths[groupThreadID.x+0][groupThreadID.y+2];
        lpfloat inBR = g_scratchDepths[groupThreadID.x+2][groupThreadID.y+2];

        lpfloat dm3 = GTVBAO_DepthMIPFilter( inTL, inTR, inBL, inBR );
        outDepth3[ baseCoord / 4 ] = (lpfloat)dm3;
        g_scratchDepths[ groupThreadID.x ][ groupThreadID.y ] = dm3;
    }

    GroupMemoryBarrierWithGroupSync( );

    // MIP 4
    [branch]
    if( all( ( groupThreadID.xy % uint2( 8, 8 ) ) == uint2( 0, 0 ) ) )
    {
        lpfloat inTL = g_scratchDepths[groupThreadID.x+0][groupThreadID.y+0];
        lpfloat inTR = g_scratchDepths[groupThreadID.x+4][groupThreadID.y+0];
        lpfloat inBL = g_scratchDepths[groupThreadID.x+0][groupThreadID.y+4];
        lpfloat inBR = g_scratchDepths[groupThreadID.x+4][groupThreadID.y+4];

        lpfloat dm4 = GTVBAO_DepthMIPFilter( inTL, inTR, inBL, inBR );
        outDepth4[ baseCoord / 8 ] = (lpfloat)dm4;
        //g_scratchDepths[ groupThreadID.x ][ groupThreadID.y ] = dm4;
    }
}


// Generic viewspace normal generate pass
float3 GTVBAO_ComputeViewspaceNormal( const uint2 pixCoord, const GTAOConstants consts, Texture2D<float> sourceNDCDepth, SamplerState depthSampler )
{
    float2 normalizedScreenPos = (pixCoord + float2( 0.5, 0.5 )) * consts.ViewportPixelSize;

    float4 valuesUL   = sourceNDCDepth.GatherRed( depthSampler, float2( pixCoord * consts.ViewportPixelSize )               );
    float4 valuesBR   = sourceNDCDepth.GatherRed( depthSampler, float2( pixCoord * consts.ViewportPixelSize ), int2( 1, 1 ) );

    // viewspace Z at the center
    float viewspaceZ  = GTVBAO_ScreenSpaceToViewSpaceDepth( valuesUL.y, consts ); //sourceViewspaceDepth.SampleLevel( depthSampler, normalizedScreenPos, 0 ).x; 

    // viewspace Zs left top right bottom
    const float pixLZ = GTVBAO_ScreenSpaceToViewSpaceDepth( valuesUL.x, consts );
    const float pixTZ = GTVBAO_ScreenSpaceToViewSpaceDepth( valuesUL.z, consts );
    const float pixRZ = GTVBAO_ScreenSpaceToViewSpaceDepth( valuesBR.z, consts );
    const float pixBZ = GTVBAO_ScreenSpaceToViewSpaceDepth( valuesBR.x, consts );

    lpfloat4 edgesLRTB  = GTVBAO_CalculateEdges( (lpfloat)viewspaceZ, (lpfloat)pixLZ, (lpfloat)pixRZ, (lpfloat)pixTZ, (lpfloat)pixBZ );

    float3 CENTER   = GTVBAO_ComputeViewspacePosition( normalizedScreenPos, viewspaceZ, consts );
    float3 LEFT     = GTVBAO_ComputeViewspacePosition( normalizedScreenPos + float2(-1,  0) * consts.ViewportPixelSize, pixLZ, consts );
    float3 RIGHT    = GTVBAO_ComputeViewspacePosition( normalizedScreenPos + float2( 1,  0) * consts.ViewportPixelSize, pixRZ, consts );
    float3 TOP      = GTVBAO_ComputeViewspacePosition( normalizedScreenPos + float2( 0, -1) * consts.ViewportPixelSize, pixTZ, consts );
    float3 BOTTOM   = GTVBAO_ComputeViewspacePosition( normalizedScreenPos + float2( 0,  1) * consts.ViewportPixelSize, pixBZ, consts );
    return GTVBAO_CalculateNormal( edgesLRTB, CENTER, LEFT, RIGHT, TOP, BOTTOM );
}
