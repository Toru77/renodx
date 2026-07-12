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

#ifdef GT_VBAO_SHOW_DEBUG_VIZ
#include "vaShared.hlsl"
#endif

#if defined( GT_VBAO_SHOW_NORMALS ) || defined( GT_VBAO_SHOW_EDGES ) || defined( GT_VBAO_SHOW_BENT_NORMALS )
RWTexture2D<float4>         g_outputDbgImage    : register( u2 );
#endif

#include "GTVBAO.h"

#define GT_VBAO_PI               	(3.1415926535897932384626433832795)
#define GT_VBAO_PI_HALF             (1.5707963267948966192313216916398)

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

// R11G11B10_UNORM <-> float3
float3 GTVBAO_R11G11B10_UNORM_to_FLOAT3( uint packedInput )
{
    float3 unpackedOutput;
    unpackedOutput.x = (float)( ( packedInput       ) & 0x000007ff ) / 2047.0f;
    unpackedOutput.y = (float)( ( packedInput >> 11 ) & 0x000007ff ) / 2047.0f;
    unpackedOutput.z = (float)( ( packedInput >> 22 ) & 0x000003ff ) / 1023.0f;
    return unpackedOutput;
}
// 'unpackedInput' is float3 and not float3 on purpose as half float lacks precision for below!
uint GTVBAO_FLOAT3_to_R11G11B10_UNORM( float3 unpackedInput )
{
    uint packedOutput;
    packedOutput =( ( uint( VA_SATURATE( unpackedInput.x ) * 2047 + 0.5f ) ) |
        ( uint( VA_SATURATE( unpackedInput.y ) * 2047 + 0.5f ) << 11 ) |
        ( uint( VA_SATURATE( unpackedInput.z ) * 1023 + 0.5f ) << 22 ) );
    return packedOutput;
}
//
lpfloat4 GTVBAO_R8G8B8A8_UNORM_to_FLOAT4( uint packedInput )
{
    lpfloat4 unpackedOutput;
    unpackedOutput.x = (lpfloat)( packedInput & 0x000000ff ) / (lpfloat)255;
    unpackedOutput.y = (lpfloat)( ( ( packedInput >> 8 ) & 0x000000ff ) ) / (lpfloat)255;
    unpackedOutput.z = (lpfloat)( ( ( packedInput >> 16 ) & 0x000000ff ) ) / (lpfloat)255;
    unpackedOutput.w = (lpfloat)( packedInput >> 24 ) / (lpfloat)255;
    return unpackedOutput;
}
//
uint GTVBAO_FLOAT4_to_R8G8B8A8_UNORM( lpfloat4 unpackedInput )
{
    return (( uint( saturate( unpackedInput.x ) * (lpfloat)255 + (lpfloat)0.5 ) ) |
            ( uint( saturate( unpackedInput.y ) * (lpfloat)255 + (lpfloat)0.5 ) << 8 ) |
            ( uint( saturate( unpackedInput.z ) * (lpfloat)255 + (lpfloat)0.5 ) << 16 ) |
            ( uint( saturate( unpackedInput.w ) * (lpfloat)255 + (lpfloat)0.5 ) << 24 ) );
}

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

// packing/unpacking for edges; 2 bits per edge mean 4 gradient values (0, 0.33, 0.66, 1) for smoother transitions!
lpfloat GTVBAO_PackEdges( lpfloat4 edgesLRTB )
{
    // integer version:
    // edgesLRTB = saturate(edgesLRTB) * 2.9.xxxx + 0.5.xxxx;
    // return (((uint)edgesLRTB.x) << 6) + (((uint)edgesLRTB.y) << 4) + (((uint)edgesLRTB.z) << 2) + (((uint)edgesLRTB.w));
    // 
    // optimized, should be same as above
    edgesLRTB = round( saturate( edgesLRTB ) * 2.9 );
    return dot( edgesLRTB, lpfloat4( 64.0 / 255.0, 16.0 / 255.0, 4.0 / 255.0, 1.0 / 255.0 ) ) ;
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

#ifdef GT_VBAO_SHOW_DEBUG_VIZ
float4 DbgGetSliceColor(int slice, int sliceCount, bool mirror)
{
    float red = (float)slice / (float)sliceCount; float green = 0.01; float blue = 1.0 - (float)slice / (float)sliceCount;
    return (mirror)?(float4(blue, green, red, 0.9)):(float4(red, green, blue, 0.9));
}
#endif

// http://h14s.p5r.org/2012/09/0x5f3759df.html, [Drobot2014a] Low Level Optimizations for GCN, https://blog.selfshadow.com/publications/s2016-shading-course/activision/s2016_pbs_activision_occlusion.pdf slide 63
lpfloat GTVBAO_FastSqrt( float x )
{
    return (lpfloat)(asfloat( 0x1fbd1df5 + ( asint( x ) >> 1 ) ));
}
// input [-1, 1] and output [0, PI], from https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
lpfloat GTVBAO_FastACos( lpfloat inX )
{ 
    const lpfloat PI = 3.141593;
    const lpfloat HALF_PI = 1.570796;
    lpfloat x = abs(inX); 
    lpfloat res = -0.156583 * x + HALF_PI; 
    res *= GTVBAO_FastSqrt(1.0 - x); 
    return (inX >= 0) ? res : PI - res; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Visibility Bitmask helpers (Therrien/Levesque/Gilet 2023)
// Replaces GTAO's two horizon angles with a uint bitmask of N_b sectors.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GT_VBAO_BITMASK_SECTOR_COUNT 32u

// Population count (count set bits in uint).
uint GTVBAO_CountBits(uint v)
{
    v = v - ((v >> 1u) & 0x55555555u);
    v = (v & 0x33333333u) + ((v >> 2u) & 0x33333333u);
    v = (v + (v >> 4u)) & 0x0F0F0F0Fu;
    v = v + (v >> 8u);
    v = v + (v >> 16u);
    return v & 0x3Fu;
}

// Mark sectors between minHorizon and maxHorizon as occluded.
// minHorizon, maxHorizon: normalized [0, 1] across the hemisphere slice.
// Uses ceil rounding: sample needs to at least touch a sector to activate it.
uint GTVBAO_UpdateSectors(float minHorizon, float maxHorizon, uint globalOccludedBitfield)
{
    uint startHorizonInt = (uint)(minHorizon * GT_VBAO_BITMASK_SECTOR_COUNT);
    uint angleHorizonInt = (uint)ceil((maxHorizon - minHorizon) * GT_VBAO_BITMASK_SECTOR_COUNT);
    uint angleHorizonBitfield = angleHorizonInt > 0u
        ? (0xFFFFFFFFu >> (GT_VBAO_BITMASK_SECTOR_COUNT - angleHorizonInt))
        : 0u;
    uint currentOccludedBitfield = angleHorizonBitfield << startHorizonInt;
    return globalOccludedBitfield | currentOccludedBitfield;
}

// ═══════════════════════════════════════════════════════════════
// GTVBAO helpers — Ground Truth Visibility Bitmask AO upgrades
// ═══════════════════════════════════════════════════════════════

// ── SinStep: C1-continuous sinusoid s-curve ──
float GTVBAO_SinStep(float x) {
    return x - sin(6.283185307f * x) * 0.159154943f;
}

// ── InvSinStep: inverse via Newton iteration ──
float GTVBAO_InvSinStep(float y) {
    float x = y;
    [unroll]
    for (int i = 0; i < 3; i++) {
        float fx = x - sin(6.283185307f * x) * 0.159154943f;
        float dfx = 1.0f - cos(6.283185307f * x);
        x = x - (fx - y) / max(dfx, 1e-5f);
    }
    return saturate(x);
}

// ── QBias: quadratic bias for CDF morphing ──
float GTVBAO_QBias(float x, float b) {
    float xx = x * x;
    return xx / (2.0f * b * (1.0f - x) + xx);
}

// ── InvSinStep with stretch parameter ──
float GTVBAO_InvSinStepStretch(float y, float s) {
    if (s < 0.001f) return y;
    float stretched = y * s + (1.0f - s) * 0.5f;
    float inv = GTVBAO_InvSinStep(stretched);
    return saturate((inv - (1.0f - s) * 0.5f) / s);
}

// ── Cosine-weighted slice sampling (Mode 3: CDF importance sampling, optimized) ──
// Maps uniform [0,1] to phi angle [0, PI] from cosine-weighted lobe.
// Per doc.txt: outer SinStep/InvSinStep cancelled by working in mapped space.
float GTVBAO_SampleSliceCosine_Mode3(float rnd, float NdotV) {
    float sinNV = sqrt(saturate(1.0f - NdotV * NdotV));
    float s = GTVBAO_QBias(sinNV, 0.15f);
    float y = GTVBAO_InvSinStepStretch(rnd, s);
    return y * GT_VBAO_PI;
}

// ── Cosine-weighted slice sampling (Mode 2: Ray projection) ──
// Samples direction from cosine lobe around N, projects to screen-space slice angle.
float GTVBAO_SampleSliceCosine_Mode2(float rnd0, float rnd1, float3 N_view, float3 viewVec) {
    // Sample from cosine lobe around N in view space
    float phi_lobe = 6.283185307f * rnd0;
    float cosTheta = sqrt(rnd1);
    float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));
    // Build orthonormal basis around N_view
    float3 T, B;
    if (abs(N_view.z) < 0.999f) {
        T = normalize(cross(float3(0, 0, 1), N_view));
    } else {
        T = float3(1, 0, 0);
    }
    B = cross(N_view, T);
    // Cosine-lobe direction in view space
    float3 lobeDir = T * (sinTheta * cos(phi_lobe))
                   + B * (sinTheta * sin(phi_lobe))
                   + N_view * cosTheta;
    // Slice direction is the XY screen-space component of the lobe direction
    float phi = atan2(lobeDir.y, lobeDir.x);
    if (phi < 0.0f) phi += GT_VBAO_PI;
    return phi;
}

// ── CDF remapping of horizon angles ──
// Accounts for non-uniform sample density near the view pole.
float GTVBAO_RemapHorizonCDF(float t, float NdotV) {
    // t is the [0,1] mapped horizon angle from baseline VBAO
    // Apply correction: near view pole (high |NdotV|), distribution is narrower
    float sinNV = sqrt(saturate(1.0f - NdotV * NdotV));
    // Blend factor: 0 = pure VBAO (sinNV=1, NdotV=0), 1 = view pole (sinNV=0)
    float blend = 1.0f - sinNV;
    // CDF-corrected value: compress toward 0.5 near the pole
    float corrected = lerp(t, 0.5f + (t - 0.5f) * sinNV, blend);
    return saturate(corrected);
}

// ── Per-sample thickness offset ──
// Computes back-face position using sample direction instead of fixed viewVec.
float3 GTVBAO_ThicknessOffset(float3 sampleDelta, float sampleDist, float3 viewVec, float thickness) {
    float3 sampleDir = sampleDelta / max(sampleDist, 1e-5f);
    // Offset along the sample direction (instead of viewVec)
    return sampleDelta - sampleDir * thickness;
}

uint GTVBAO_EncodeVisibilityBentNormal( lpfloat visibility, lpfloat3 bentNormal )
{
    return GTVBAO_FLOAT4_to_R8G8B8A8_UNORM( lpfloat4( bentNormal * 0.5 + 0.5, visibility ) );
}

void GTVBAO_DecodeVisibilityBentNormal( const uint packedValue, out lpfloat visibility, out lpfloat3 bentNormal )
{
    lpfloat4 decoded = GTVBAO_R8G8B8A8_UNORM_to_FLOAT4( packedValue );
    bentNormal = decoded.xyz * lpfloat3( 2.0, 2.0, 2.0 ) - lpfloat3( 1.0, 1.0, 1.0 );   // could normalize - don't want to since it's done so many times, better to do it at the final step only
    visibility = decoded.w;
}

void GTVBAO_OutputWorkingTerm( const uint2 pixCoord, lpfloat visibility, lpfloat3 bentNormal, RWTexture2D<uint> outWorkingAOTerm )
{
    visibility = saturate( visibility / lpfloat(GT_VBAO_OCCLUSION_TERM_SCALE) );
#ifdef GT_VBAO_COMPUTE_BENT_NORMALS
    outWorkingAOTerm[pixCoord] = GTVBAO_EncodeVisibilityBentNormal( visibility, bentNormal );
#else
    outWorkingAOTerm[pixCoord] = uint(visibility * 255.0 + 0.5);
#endif
}

// "Efficiently building a matrix to rotate one vector to another"
// http://cs.brown.edu/research/pubs/pdfs/1999/Moller-1999-EBA.pdf / https://dl.acm.org/doi/10.1080/10867651.1999.10487509
// (using https://github.com/assimp/assimp/blob/master/include/assimp/matrix3x3.inl#L275 as a code reference as it seems to be best)
lpfloat3x3 GTVBAO_RotFromToMatrix( lpfloat3 from, lpfloat3 to )
{
    const lpfloat e       = dot(from, to);
    const lpfloat f       = abs(e); //(e < 0)? -e:e;

    // WARNING: This has not been tested/worked through, especially not for 16bit floats; seems to work in our special use case (from is always {0, 0, -1}) but wouldn't use it in general
    if( f > lpfloat( 1.0 - 0.0003 ) )
        return lpfloat3x3( 1, 0, 0, 0, 1, 0, 0, 0, 1 );

    const lpfloat3 v      = cross( from, to );
    /* ... use this hand optimized version (9 mults less) */
    const lpfloat h       = (1.0)/(1.0 + e);      /* optimization by Gottfried Chen */
    const lpfloat hvx     = h * v.x;
    const lpfloat hvz     = h * v.z;
    const lpfloat hvxy    = hvx * v.y;
    const lpfloat hvxz    = hvx * v.z;
    const lpfloat hvyz    = hvz * v.y;

    lpfloat3x3 mtx;
    mtx[0][0] = e + hvx * v.x;
    mtx[0][1] = hvxy - v.z;
    mtx[0][2] = hvxz + v.y;

    mtx[1][0] = hvxy + v.z;
    mtx[1][1] = e + h * v.y * v.y;
    mtx[1][2] = hvyz - v.x;

    mtx[2][0] = hvxz - v.y;
    mtx[2][1] = hvyz + v.x;
    mtx[2][2] = e + hvz * v.z;

    return mtx;
}

void GTVBAO_MainPass( const uint2 pixCoord, lpfloat sliceCount, lpfloat stepsPerSlice, const lpfloat2 localNoise, lpfloat3 viewspaceNormal, const GTAOConstants consts, 
    Texture2D<lpfloat> sourceViewspaceDepth, SamplerState depthSampler, RWTexture2D<uint> outWorkingAOTerm, RWTexture2D<float> outWorkingEdges
#ifdef GT_VBAO_COMPUTE_GI
    , bool enableGI, float giIntensity,
    Texture2D<float4> lightBuffer, SamplerState lightSampler,
    Texture2D<uint4> mrtNormalTexture,
    RWTexture2D<float4> outGI
    , RWTexture2D<float4> outDebug
#endif
    )
{                                                                       
    float2 normalizedScreenPos = (pixCoord + float2( 0.5, 0.5 )) * consts.ViewportPixelSize;

    lpfloat4 valuesUL   = sourceViewspaceDepth.GatherRed( depthSampler, float2( pixCoord * consts.ViewportPixelSize )               );
    lpfloat4 valuesBR   = sourceViewspaceDepth.GatherRed( depthSampler, float2( pixCoord * consts.ViewportPixelSize ), int2( 1, 1 ) );

    // viewspace Z at the center
    lpfloat viewspaceZ  = valuesUL.y; //sourceViewspaceDepth.SampleLevel( depthSampler, normalizedScreenPos, 0 ).x; 

    // viewspace Zs left top right bottom
    const lpfloat pixLZ = valuesUL.x;
    const lpfloat pixTZ = valuesUL.z;
    const lpfloat pixRZ = valuesBR.z;
    const lpfloat pixBZ = valuesBR.x;

    lpfloat4 edgesLRTB  = GTVBAO_CalculateEdges( (lpfloat)viewspaceZ, (lpfloat)pixLZ, (lpfloat)pixRZ, (lpfloat)pixTZ, (lpfloat)pixBZ );
    outWorkingEdges[pixCoord] = GTVBAO_PackEdges(edgesLRTB);

    // ── Foliage early-out ──
    // Foliage is forward-rendered so its material type isn't in the deferred mrt1.
    // Instead, detect foliage by depth edge density: foliage has many depth discontinuities
    // in a small neighborhood (alpha-tested leaves/branches) vs. solid geometry.
    if (GTVBAO_exclude_foliage > 0.5f) {
      float dzL = abs(viewspaceZ - pixLZ);
      float dzR = abs(viewspaceZ - pixRZ);
      float dzT = abs(viewspaceZ - pixTZ);
      float dzB = abs(viewspaceZ - pixBZ);
      float dzThreshold = 0.02f;  // viewspace depth delta threshold
      uint edgeCount = 0u;
      if (dzL > dzThreshold) edgeCount++;
      if (dzR > dzThreshold) edgeCount++;
      if (dzT > dzThreshold) edgeCount++;
      if (dzB > dzThreshold) edgeCount++;
      if (edgeCount >= 3u) {  // 3+ edges = likely foliage (alpha-tested surface)
        outWorkingAOTerm[pixCoord] = (uint)(GTVBAO_foliage_ao_value * 255.0f);
#ifdef GT_VBAO_COMPUTE_GI
        outGI[pixCoord] = float4(0, 0, 0, 0);
#endif
        return;
      }
    }

	// Generating screen space normals in-place is faster than generating normals in a separate pass but requires
	// use of 32bit depth buffer (16bit works but visibly degrades quality) which in turn slows everything down. So to
	// reduce complexity and allow for screen space normal reuse by other effects, we've pulled it out into a separate
	// pass.
	// However, we leave this code in, in case anyone has a use-case where it fits better.
#ifdef GT_VBAO_GENERATE_NORMALS_INPLACE
    float3 CENTER   = GTVBAO_ComputeViewspacePosition( normalizedScreenPos, viewspaceZ, consts );
    float3 LEFT     = GTVBAO_ComputeViewspacePosition( normalizedScreenPos + float2(-1,  0) * consts.ViewportPixelSize, pixLZ, consts );
    float3 RIGHT    = GTVBAO_ComputeViewspacePosition( normalizedScreenPos + float2( 1,  0) * consts.ViewportPixelSize, pixRZ, consts );
    float3 TOP      = GTVBAO_ComputeViewspacePosition( normalizedScreenPos + float2( 0, -1) * consts.ViewportPixelSize, pixTZ, consts );
    float3 BOTTOM   = GTVBAO_ComputeViewspacePosition( normalizedScreenPos + float2( 0,  1) * consts.ViewportPixelSize, pixBZ, consts );
    viewspaceNormal = (lpfloat3)GTVBAO_CalculateNormal( edgesLRTB, CENTER, LEFT, RIGHT, TOP, BOTTOM );
#endif

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

#ifdef GT_VBAO_SHOW_NORMALS
    g_outputDbgImage[pixCoord] = float4( DisplayNormalSRGB( viewspaceNormal.xyz ), 1 );
#endif

#ifdef GT_VBAO_SHOW_EDGES
    g_outputDbgImage[pixCoord] = 1.0 - float4( edgesLRTB.x, edgesLRTB.y * 0.5 + edgesLRTB.w * 0.5, edgesLRTB.z, 1.0 );
#endif

#if GT_VBAO_USE_DEFAULT_CONSTANTS != 0
    const lpfloat effectRadius              = (lpfloat)consts.EffectRadius * (lpfloat)GT_VBAO_DEFAULT_RADIUS_MULTIPLIER;
    const lpfloat sampleDistributionPower   = (lpfloat)GT_VBAO_DEFAULT_SAMPLE_DISTRIBUTION_POWER;
    const lpfloat thinOccluderCompensation  = (lpfloat)GT_VBAO_DEFAULT_THIN_OCCLUDER_COMPENSATION;
    const lpfloat falloffRange              = (lpfloat)GT_VBAO_DEFAULT_FALLOFF_RANGE * effectRadius;
#else
    const lpfloat effectRadius              = (lpfloat)consts.EffectRadius * (lpfloat)consts.RadiusMultiplier;
    const lpfloat sampleDistributionPower   = (lpfloat)consts.SampleDistributionPower;
    const lpfloat thinOccluderCompensation  = (lpfloat)consts.ThinOccluderCompensation;
    const lpfloat falloffRange              = (lpfloat)consts.EffectFalloffRange * effectRadius;
#endif

    const lpfloat falloffFrom       = effectRadius * ((lpfloat)1-(lpfloat)consts.EffectFalloffRange);

    // fadeout precompute optimisation
    const lpfloat falloffMul        = (lpfloat)-1.0 / ( falloffRange );
    const lpfloat falloffAdd        = falloffFrom / ( falloffRange ) + (lpfloat)1.0;

    lpfloat visibility = 0;
    lpfloat totalWeight = 0.0; // GTVBAO cosine mode 0: accumulate actual weight sum
#ifdef GT_VBAO_COMPUTE_BENT_NORMALS
    lpfloat3 bentNormal = 0;
#else
    lpfloat3 bentNormal = viewspaceNormal;
#endif
#ifdef GT_VBAO_COMPUTE_GI
    float3 giAccum = float3(0, 0, 0);
#endif
    // ── Debug accumulators for bitmask viz (modes 6-8) ──
    uint debugTotalSectorCoverage = 0u;
    uint debugTotalSamples = 0u;
    uint debugFirstSliceBitmask = 0u;
    bool debugFirstSliceSaved = false;
    // ── SSGI debug view 5: sample activity counters ──
    uint activityContributed = 0u;  // samples that added GI
    uint activityRejected = 0u;     // samples valid but newCount==0

#ifdef GT_VBAO_SHOW_DEBUG_VIZ
    float3 dbgWorldPos          = mul(g_globals.ViewInv, float4(pixCenterPos, 1)).xyz;
#endif

    // see "Algorithm 1" in https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
    {
        const lpfloat noiseSlice  = (lpfloat)localNoise.x;
        const lpfloat noiseSample = (lpfloat)localNoise.y;

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
            GTVBAO_OutputWorkingTerm( pixCoord, 1, viewspaceNormal, outWorkingAOTerm );
            return;
        }
#endif

#ifdef GT_VBAO_SHOW_DEBUG_VIZ
        [branch] if (IsUnderCursorRange(pixCoord, int2(1, 1)))
        {
            float3 dbgWorldNorm     = mul((float3x3)g_globals.ViewInv, viewspaceNormal).xyz;
            float3 dbgWorldViewVec  = mul((float3x3)g_globals.ViewInv, viewVec).xyz;
            //DebugDraw3DArrow(dbgWorldPos, dbgWorldPos + 0.5 * dbgWorldViewVec, 0.02, float4(0, 1, 0, 0.95));
            //DebugDraw2DCircle(pixCoord, screenspaceRadius, float4(1, 0, 0.2, 1));
            DebugDraw3DSphere(dbgWorldPos, effectRadius, float4(1, 0.2, 0, 0.1));
            //DebugDraw3DText(dbgWorldPos, float2(0, 0), float4(0.6, 0.3, 0.3, 1), float4( pixelDirRBViewspaceSizeAtCenterZ.xy, 0, screenspaceRadius) );
        }
#endif

        // this is the min distance to start sampling from to avoid sampling from the center pixel (no useful data obtained from sampling center pixel)
        const lpfloat minS = (lpfloat)pixelTooCloseThreshold / screenspaceRadius;

        //[unroll]
        for( lpfloat slice = 0; slice < sliceCount; slice++ )
        {
            lpfloat sliceK = (slice+noiseSlice) / sliceCount;
            // lines 5, 6 from the paper
            lpfloat phi;
            if (GTVBAO_gtvbao_cosine_enabled > 0.5f) {
                float NdotV = saturate(dot((float3)viewspaceNormal, (float3)viewVec));
                float rnd0 = frac(noiseSample + sliceK * 0.618034f);
                int mode = (int)GTVBAO_gtvbao_cosine_mode;
                if (mode == 0) {
                    // Mode 1: uniform, weight applied later
                    phi = sliceK * GT_VBAO_PI;
                } else if (mode == 1) {
                    // Mode 2: ray projection from cosine lobe
                    float rnd1 = frac(noiseSample + sliceK * 0.381966f);
                    phi = GTVBAO_SampleSliceCosine_Mode2(rnd0, rnd1, (float3)viewspaceNormal, (float3)viewVec);
                } else {
                    // Mode 3 (default): CDF importance sampling
                    phi = GTVBAO_SampleSliceCosine_Mode3(rnd0, NdotV);
                }
            } else {
                phi = sliceK * GT_VBAO_PI;
            }
            lpfloat cosPhi = cos(phi);
            lpfloat sinPhi = sin(phi);
            lpfloat2 omega = lpfloat2(cosPhi, -sinPhi);       //lpfloat2 on omega causes issues with big radii

            // convert to screen units (pixels) for later use
            omega *= screenspaceRadius;

            // line 8 from the paper
            const lpfloat3 directionVec = lpfloat3(cosPhi, sinPhi, 0);

            // line 9 from the paper
            const lpfloat3 orthoDirectionVec = directionVec - (dot(directionVec, viewVec) * viewVec);

            // line 10 from the paper
            //axisVec is orthogonal to directionVec and viewVec, used to define projectedNormal
            const lpfloat3 axisVec = normalize( cross(orthoDirectionVec, viewVec) );

            // alternative line 9 from the paper
            // float3 orthoDirectionVec = cross( viewVec, axisVec );

            // line 11 from the paper
            lpfloat3 projectedNormalVec = viewspaceNormal - axisVec * dot(viewspaceNormal, axisVec);

            // line 13 from the paper
            lpfloat signNorm = (lpfloat)sign( dot( orthoDirectionVec, projectedNormalVec ) );

            // line 14 from the paper
            lpfloat projectedNormalVecLength = length(projectedNormalVec);
            lpfloat cosNorm = (lpfloat)saturate(dot(projectedNormalVec, viewVec) / projectedNormalVecLength);

            // line 15 from the paper
            lpfloat n = signNorm * GTVBAO_FastACos(cosNorm);

#ifdef GT_VBAO_USE_BITMASK
            // ═══════════════════════════════════════════════════════════════
            // Visibility Bitmask AO + optional GI
            // Replaces GTAO horizon angles with a uint bitmask.
            // Therrien/Levesque/Gilet 2023, Algorithm 1.
            // ═══════════════════════════════════════════════════════════════

            // Bitmask for this slice (0 = unoccluded, 1 = occluded).
            uint sliceBitmask = 0u;
#ifdef GT_VBAO_COMPUTE_GI
            float3 sliceGI = float3(0, 0, 0);
#endif

            [unroll]
            for( lpfloat step = 0; step < stepsPerSlice; step++ )
            {
                // R1 sequence (http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/)
                const lpfloat stepBaseNoise = lpfloat(slice + step * stepsPerSlice) * 0.6180339887498948482;
                lpfloat stepNoise = frac(noiseSample + stepBaseNoise);

                lpfloat s = (step+stepNoise) / (stepsPerSlice);
                s = (lpfloat)pow( s, (lpfloat)sampleDistributionPower );
                s += minS;

                lpfloat2 sampleOffset = s * omega;
                lpfloat sampleOffsetLength = length( sampleOffset );
                const lpfloat mipLevel = (lpfloat)clamp( log2( sampleOffsetLength ) - consts.DepthMIPSamplingOffset, 0, GT_VBAO_DEPTH_MIP_LEVELS );
                sampleOffset = round(sampleOffset) * (lpfloat2)consts.ViewportPixelSize;

                // ── Sample both sides along the slice ──
                // Unroll the two sides: side 0 = +offset, side 1 = -offset.
                [unroll]
                for( int side = 0; side < 2; side++ )
                {
                    float sideSign = (side == 0) ? 1.0 : -1.0;
                    float2 sampleScreenPos = normalizedScreenPos + sampleOffset * sideSign;
                    float  SZ = sourceViewspaceDepth.SampleLevel( depthSampler, sampleScreenPos, mipLevel ).x;
                    float3 samplePos = GTVBAO_ComputeViewspacePosition( sampleScreenPos, SZ, consts );

                    float3 sampleDelta = samplePos - float3(pixCenterPos);
                    lpfloat sampleDist = (lpfloat)length( sampleDelta );

                    // ── Experimental fix mode from push constant c[24] ──
                    int fixMode = (int)GTVBAO_copyback_preserve_yzw;

                    // Skip samples beyond the effect radius.
                    if (sampleDist > effectRadius) continue;

                    // Compute effective thickness.
                    // BUG: constant thickness can exceed sampleDist for near samples,
                    // pushing back-face behind the pixel and saturating all sectors.
                    float thickness = (float)thinOccluderCompensation;
                    // Fix 1: clamp thickness to 50% of sample distance.
                    if (fixMode == 1) thickness = min(thickness, (float)sampleDist * 0.5f);
                    // Fix 2: clamp thickness to 100% of sample distance.
                    if (fixMode == 2) thickness = min(thickness, (float)sampleDist);
                    // Fix 3: scale thickness by (1 - distance/radius) → near=thicker, far=thinner.
                    if (fixMode == 3) thickness = thickness * (1.0f - (float)sampleDist / max(effectRadius, 0.001f));
                    // Fix 4: skip sample if back-face would go behind pixel.
                    if (fixMode == 4 && (float)sampleDist < thickness) continue;
                    // Fix 5: skip sample if thickness > 2x sample distance.
                    if (fixMode == 5 && (float)sampleDist < thickness * 2.0f) continue;

                    // ── Front-face and back-face (offset by thickness) ──
                    float3 sampleHorizonVec = (float3)(sampleDelta / sampleDist);
                    float3 sampleDeltaBack;
                    if (GTVBAO_gtvbao_thickness_enabled > 0.5f) {
                        // GTVBAO: per-sample thickness direction
                        sampleDeltaBack = GTVBAO_ThicknessOffset(sampleDelta, (float)sampleDist, (float3)viewVec, thickness);
                    } else {
                        // Baseline: offset along view vector
                        sampleDeltaBack = sampleDelta - (float3)viewVec * thickness;
                    }
                    float3 sampleHorizonVecBack = normalize( sampleDeltaBack );

                    // Horizon cosines relative to viewVec (same as GTAO's shc).
                    float shc_front = dot(sampleHorizonVec, (float3)viewVec);
                    float shc_back  = dot(sampleHorizonVecBack, (float3)viewVec);

                    // Convert to angles, shift from viewVec to projected normal, map to [0,1].
                    // From authors' supplemental code (bitmask_tips.txt).
                    float2 frontBackHorizon = float2(shc_front, shc_back);
                    frontBackHorizon.x = (float)GTVBAO_FastACos((lpfloat)frontBackHorizon.x);  // [0, π]
                    frontBackHorizon.y = (float)GTVBAO_FastACos((lpfloat)frontBackHorizon.y);

                    // Shift: authors' N = -sign(...)*acos = -n (GTAO convention).
                    // Substituting N = -n: mapped = (sd*-angle + n + π/2)/π
                    float sd = sideSign;  // samplingDirection: +1 right, -1 left
                    frontBackHorizon = saturate(((sd * -frontBackHorizon) + (float)n + GT_VBAO_PI_HALF) / GT_VBAO_PI);

                    // ── GTVBAO: CDF remap horizon angles ──
                    if (GTVBAO_gtvbao_cdf_enabled > 0.5f) {
                        float NdotV = saturate(dot((float3)viewspaceNormal, (float3)viewVec));
                        frontBackHorizon.x = GTVBAO_RemapHorizonCDF(frontBackHorizon.x, NdotV);
                        frontBackHorizon.y = GTVBAO_RemapHorizonCDF(frontBackHorizon.y, NdotV);
                    }

                    // samplingDirection inverts min/max ordering.
                    frontBackHorizon = (sd >= 0.0) ? frontBackHorizon.yx : frontBackHorizon.xy;

                    // Compute sample bitmask and update global bitmask.
                    uint sampleMask = GTVBAO_UpdateSectors(frontBackHorizon.x, frontBackHorizon.y, 0u);

                    // ── Debug: track per-sample sector coverage ──
                    debugTotalSectorCoverage += GTVBAO_CountBits(sampleMask);
                    debugTotalSamples += 1u;

#ifdef GT_VBAO_COMPUTE_GI
                    // ── GI contribution (paper Algorithm 1, line 23) ──
                    // b_j & ~b_i: sectors this sample covers that are NOT yet occluded.
                    uint newSectors = sampleMask & ~sliceBitmask;
                    uint newCount = GTVBAO_CountBits(newSectors);
                    if (newCount > 0u && enableGI)
                    {
                        // Read HDR light color at sample position (with exposure scale).
                        float3 lightColor = lightBuffer.SampleLevel(lightSampler, sampleScreenPos, 0).rgb * g_gi_light_exposure;

                        // Light direction from pixel to sample.
                        float3 lightDir = (float3)(sampleDelta / sampleDist);
                        float NdotL = saturate(dot((float3)viewspaceNormal, lightDir));

                        // Sample normal for (n_j · −l_j) weighting.
                        // Use MRT g-buffer normal if available, else fall back to pixel normal.
                        float3 sampleNormal = (float3)viewspaceNormal;
                        if (GTVBAO_normal_input_mode > 0.5 && GTVBAO_mrt_normal_available > 0.5)
                        {
                            // Map sample screen pos to MRT texel.
                            uint mw2, mh2;
                            mrtNormalTexture.GetDimensions(mw2, mh2);
                            if (mw2 > 0 && mh2 > 0)
                            {
                                int2 mrtTc = int2(sampleScreenPos * float2(mw2, mh2));
                                mrtTc = clamp(mrtTc, int2(0,0), int2(mw2-1, mh2-1));
                                float3 decoded = DecodeMrtNormalAsIs((uint2)mrtTc);
                                sampleNormal = TransformNormalToView(decoded);
                            }
                        }
                        float NsDotL = saturate(dot(sampleNormal, -lightDir));

                        float weight = (float)newCount / (float)GT_VBAO_BITMASK_SECTOR_COUNT;
                        sliceGI += weight * lightColor * NdotL * NsDotL * giIntensity;
                        activityContributed += 1u;
                    } else if (enableGI) {
                        activityRejected += 1u;
                    }
#endif // GT_VBAO_COMPUTE_GI

                    sliceBitmask |= sampleMask;
                }
            }

            // ── Debug: save first slice's final bitmask ──
            if (!debugFirstSliceSaved) {
                debugFirstSliceBitmask = sliceBitmask;
                debugFirstSliceSaved = true;
            }

            // ── AO for this slice: fraction of unoccluded sectors ──
            lpfloat sliceAO = (lpfloat)1.0 - (lpfloat)GTVBAO_CountBits(sliceBitmask) / (lpfloat)GT_VBAO_BITMASK_SECTOR_COUNT;

            // ── GTVBAO Mode 0: cosine weight per slice ──
            if (GTVBAO_gtvbao_cosine_enabled > 0.5f
                && (int)GTVBAO_gtvbao_cosine_mode == 0) {
                // Weight by cosine falloff from projected normal direction
                float angN = GT_VBAO_PI_HALF - (float)n; // projected normal angle
                float cosWeight = saturate(cos((float)phi - angN));
                sliceAO *= cosWeight;
                totalWeight += cosWeight;
            }
            visibility += sliceAO;

#ifdef GT_VBAO_COMPUTE_GI
            giAccum += sliceGI;
#endif

#else // !GT_VBAO_USE_BITMASK — original GTAO horizon-angle path (preserved)
            // this is a lower weight target; not using -1 as in the original paper because it is under horizon, so a 'weight' has different meaning based on the normal
            const lpfloat lowHorizonCos0  = cos(n+GT_VBAO_PI_HALF);
            const lpfloat lowHorizonCos1  = cos(n-GT_VBAO_PI_HALF);

            // lines 17, 18 from the paper, manually unrolled the 'side' loop
            lpfloat horizonCos0           = lowHorizonCos0; //-1;
            lpfloat horizonCos1           = lowHorizonCos1; //-1;

            [unroll]
            for( lpfloat step = 0; step < stepsPerSlice; step++ )
            {
                // R1 sequence (http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/)
                const lpfloat stepBaseNoise = lpfloat(slice + step * stepsPerSlice) * 0.6180339887498948482; // <- this should unroll
                lpfloat stepNoise = frac(noiseSample + stepBaseNoise);

                // approx line 20 from the paper, with added noise
                lpfloat s = (step+stepNoise) / (stepsPerSlice); // + (lpfloat2)1e-6f);

                // additional distribution modifier
                s       = (lpfloat)pow( s, (lpfloat)sampleDistributionPower );

                // avoid sampling center pixel
                s       += minS;

                // approx lines 21-22 from the paper, unrolled
                lpfloat2 sampleOffset = s * omega;

                lpfloat sampleOffsetLength = length( sampleOffset );

                // note: when sampling, using point_point_point or point_point_linear sampler works, but linear_linear_linear will cause unwanted interpolation between neighbouring depth values on the same MIP level!
                const lpfloat mipLevel    = (lpfloat)clamp( log2( sampleOffsetLength ) - consts.DepthMIPSamplingOffset, 0, GT_VBAO_DEPTH_MIP_LEVELS );

                // Snap to pixel center (more correct direction math, avoids artifacts due to sampling pos not matching depth texel center - messes up slope - but adds other 
                // artifacts due to them being pushed off the slice). Also use full precision for high res cases.
                sampleOffset = round(sampleOffset) * (lpfloat2)consts.ViewportPixelSize;

#ifdef GT_VBAO_SHOW_DEBUG_VIZ
                int mipLevelU = (int)round(mipLevel);
                float4 mipColor = saturate( float4( mipLevelU>=3, mipLevelU>=1 && mipLevelU<=3, mipLevelU<=1, 1.0 ) );
                if( all( sampleOffset == 0 ) )
                    DebugDraw2DText( pixCoord, float4( 1, 0, 0, 1), pixelTooCloseThreshold );
                [branch] if (IsUnderCursorRange(pixCoord, int2(1, 1)))
                {
                    //DebugDraw2DText( (normalizedScreenPos + sampleOffset) * consts.ViewportSize, mipColor, mipLevelU );
                    //DebugDraw2DText( (normalizedScreenPos + sampleOffset) * consts.ViewportSize, mipColor, (uint)slice );
                    //DebugDraw2DText( (normalizedScreenPos - sampleOffset) * consts.ViewportSize, mipColor, (uint)slice );
                    //DebugDraw2DText( (normalizedScreenPos - sampleOffset) * consts.ViewportSize, saturate( float4( mipLevelU>=3, mipLevelU>=1 && mipLevelU<=3, mipLevelU<=1, 1.0 ) ), mipLevelU );
                }
#endif

                float2 sampleScreenPos0 = normalizedScreenPos + sampleOffset;
                float  SZ0 = sourceViewspaceDepth.SampleLevel( depthSampler, sampleScreenPos0, mipLevel ).x;
                float3 samplePos0 = GTVBAO_ComputeViewspacePosition( sampleScreenPos0, SZ0, consts );

                float2 sampleScreenPos1 = normalizedScreenPos - sampleOffset;
                float  SZ1 = sourceViewspaceDepth.SampleLevel( depthSampler, sampleScreenPos1, mipLevel ).x;
                float3 samplePos1 = GTVBAO_ComputeViewspacePosition( sampleScreenPos1, SZ1, consts );

                float3 sampleDelta0     = (samplePos0 - float3(pixCenterPos)); // using lpfloat for sampleDelta causes precision issues
                float3 sampleDelta1     = (samplePos1 - float3(pixCenterPos)); // using lpfloat for sampleDelta causes precision issues
                lpfloat sampleDist0     = (lpfloat)length( sampleDelta0 );
                lpfloat sampleDist1     = (lpfloat)length( sampleDelta1 );

                // approx lines 23, 24 from the paper, unrolled
                lpfloat3 sampleHorizonVec0 = (lpfloat3)(sampleDelta0 / sampleDist0);
                lpfloat3 sampleHorizonVec1 = (lpfloat3)(sampleDelta1 / sampleDist1);

                // any sample out of radius should be discarded - also use fallof range for smooth transitions; this is a modified idea from "4.3 Implementation details, Bounding the sampling area"
#if GT_VBAO_USE_DEFAULT_CONSTANTS != 0 && GT_VBAO_DEFAULT_THIN_OBJECT_HEURISTIC == 0
                lpfloat weight0         = saturate( sampleDist0 * falloffMul + falloffAdd );
                lpfloat weight1         = saturate( sampleDist1 * falloffMul + falloffAdd );
#else
                // this is our own thickness heuristic that relies on sooner discarding samples behind the center
                lpfloat falloffBase0    = length( lpfloat3(sampleDelta0.x, sampleDelta0.y, sampleDelta0.z * (1+thinOccluderCompensation) ) );
                lpfloat falloffBase1    = length( lpfloat3(sampleDelta1.x, sampleDelta1.y, sampleDelta1.z * (1+thinOccluderCompensation) ) );
                lpfloat weight0         = saturate( falloffBase0 * falloffMul + falloffAdd );
                lpfloat weight1         = saturate( falloffBase1 * falloffMul + falloffAdd );
#endif

                // sample horizon cos
                lpfloat shc0 = (lpfloat)dot(sampleHorizonVec0, viewVec);
                lpfloat shc1 = (lpfloat)dot(sampleHorizonVec1, viewVec);

                // discard unwanted samples
                shc0 = lerp( lowHorizonCos0, shc0, weight0 ); // this would be more correct but too expensive: cos(lerp( acos(lowHorizonCos0), acos(shc0), weight0 ));
                shc1 = lerp( lowHorizonCos1, shc1, weight1 ); // this would be more correct but too expensive: cos(lerp( acos(lowHorizonCos1), acos(shc1), weight1 ));

                // thickness heuristic - see "4.3 Implementation details, Height-field assumption considerations"
#if 0   // (disabled, not used) this should match the paper
                lpfloat newhorizonCos0 = max( horizonCos0, shc0 );
                lpfloat newhorizonCos1 = max( horizonCos1, shc1 );
                horizonCos0 = (horizonCos0 > shc0)?( lerp( newhorizonCos0, shc0, thinOccluderCompensation ) ):( newhorizonCos0 );
                horizonCos1 = (horizonCos1 > shc1)?( lerp( newhorizonCos1, shc1, thinOccluderCompensation ) ):( newhorizonCos1 );
#elif 0 // (disabled, not used) this is slightly different from the paper but cheaper and provides very similar results
                horizonCos0 = lerp( max( horizonCos0, shc0 ), shc0, thinOccluderCompensation );
                horizonCos1 = lerp( max( horizonCos1, shc1 ), shc1, thinOccluderCompensation );
#else   // this is a version where thicknessHeuristic is completely disabled
                horizonCos0 = max( horizonCos0, shc0 );
                horizonCos1 = max( horizonCos1, shc1 );
#endif


#ifdef GT_VBAO_SHOW_DEBUG_VIZ
                [branch] if (IsUnderCursorRange(pixCoord, int2(1, 1)))
                {
                    float3 WS_samplePos0 = mul(g_globals.ViewInv, float4(samplePos0, 1)).xyz;
                    float3 WS_samplePos1 = mul(g_globals.ViewInv, float4(samplePos1, 1)).xyz;
                    float3 WS_sampleHorizonVec0 = mul( (float3x3)g_globals.ViewInv, sampleHorizonVec0).xyz;
                    float3 WS_sampleHorizonVec1 = mul( (float3x3)g_globals.ViewInv, sampleHorizonVec1).xyz;
                    // DebugDraw3DSphere( WS_samplePos0, effectRadius * 0.02, DbgGetSliceColor(slice, sliceCount, false) );
                    // DebugDraw3DSphere( WS_samplePos1, effectRadius * 0.02, DbgGetSliceColor(slice, sliceCount, true) );
                    DebugDraw3DSphere( WS_samplePos0, effectRadius * 0.02, mipColor );
                    DebugDraw3DSphere( WS_samplePos1, effectRadius * 0.02, mipColor );
                    // DebugDraw3DArrow( WS_samplePos0, WS_samplePos0 - WS_sampleHorizonVec0, 0.002, float4(1, 0, 0, 1 ) );
                    // DebugDraw3DArrow( WS_samplePos1, WS_samplePos1 - WS_sampleHorizonVec1, 0.002, float4(1, 0, 0, 1 ) );
                    // DebugDraw3DText( WS_samplePos0, float2(0,  0), float4( 1, 0, 0, 1), weight0 );
                    // DebugDraw3DText( WS_samplePos1, float2(0,  0), float4( 1, 0, 0, 1), weight1 );

                    // DebugDraw2DText( float2( 500, 94+(step+slice*3)*12 ), float4( 0, 1, 0, 1 ), float4( projectedNormalVecLength, 0, horizonCos0, horizonCos1 ) );
                }
#endif
            }

#if 1       // I can't figure out the slight overdarkening on high slopes, so I'm adding this fudge - in the training set, 0.05 is close (PSNR 21.34) to disabled (PSNR 21.45)
            projectedNormalVecLength = lerp( projectedNormalVecLength, 1, 0.05 );
#endif

            // line ~27, unrolled
            lpfloat h0 = -GTVBAO_FastACos((lpfloat)horizonCos1);
            lpfloat h1 = GTVBAO_FastACos((lpfloat)horizonCos0);
#if 0       // we can skip clamping for a tiny little bit more performance
            h0 = n + clamp( h0-n, (lpfloat)-GT_VBAO_PI_HALF, (lpfloat)GT_VBAO_PI_HALF );
            h1 = n + clamp( h1-n, (lpfloat)-GT_VBAO_PI_HALF, (lpfloat)GT_VBAO_PI_HALF );
#endif
            lpfloat iarc0 = ((lpfloat)cosNorm + (lpfloat)2 * (lpfloat)h0 * (lpfloat)sin(n)-(lpfloat)cos((lpfloat)2 * (lpfloat)h0-n))/(lpfloat)4;
            lpfloat iarc1 = ((lpfloat)cosNorm + (lpfloat)2 * (lpfloat)h1 * (lpfloat)sin(n)-(lpfloat)cos((lpfloat)2 * (lpfloat)h1-n))/(lpfloat)4;
            lpfloat localVisibility = (lpfloat)projectedNormalVecLength * (lpfloat)(iarc0+iarc1);
            visibility += localVisibility;
#endif // GT_VBAO_USE_BITMASK

#ifdef GT_VBAO_COMPUTE_BENT_NORMALS
            // see "Algorithm 2 Extension that computes bent normals b."
            lpfloat t0 = (6*sin(h0-n)-sin(3*h0-n)+6*sin(h1-n)-sin(3*h1-n)+16*sin(n)-3*(sin(h0+n)+sin(h1+n)))/12;
            lpfloat t1 = (-cos(3 * h0-n)-cos(3 * h1-n) +8 * cos(n)-3 * (cos(h0+n) +cos(h1+n)))/12;
            lpfloat3 localBentNormal = lpfloat3( directionVec.x * (lpfloat)t0, directionVec.y * (lpfloat)t0, -lpfloat(t1) );
            localBentNormal = (lpfloat3)mul( GTVBAO_RotFromToMatrix( lpfloat3(0,0,-1), viewVec ), localBentNormal ) * projectedNormalVecLength;
            bentNormal += localBentNormal;
#endif
        }
        // Normalize: use sum of cosine weights (Mode 0) or uniform slice count
        visibility /= (GTVBAO_gtvbao_cosine_enabled > 0.5f
                        && (int)GTVBAO_gtvbao_cosine_mode == 0)
                        ? max(totalWeight, (lpfloat)1e-5)
                        : (lpfloat)sliceCount;
        visibility = pow( visibility, (lpfloat)consts.FinalValuePower );
        visibility = max( (lpfloat)0.03, visibility ); // disallow total occlusion (which wouldn't make any sense anyhow since pixel is visible but also helps with packing bent normals)

#ifdef GT_VBAO_COMPUTE_GI
        giAccum /= max((float)sliceCount, 1.0);

        // ── RGB Adaptive Boost: amplify channels proportional to dominance ──
        // Mode 0 (GI Color): boost based on GI's own per-channel strength.
        // Mode 1 (Albedo): boost based on surface color at pixel center.
        float3 adaptiveWeights;
        if (g_gi_adaptive_mode < 0.5f) {
            // Mode 0: GI's own color determines dominance.
            float giMaxChan = max(max(giAccum.r, giAccum.g), giAccum.b) + 0.001;
            adaptiveWeights = saturate(giAccum / giMaxChan);
        } else {
            // Mode 1: sample surface albedo at pixel center for dominance.
            float3 albedo = lightBuffer.SampleLevel(lightSampler, normalizedScreenPos, 0).rgb;
            float albedoMax = max(max(albedo.r, albedo.g), albedo.b) + 0.001;
            adaptiveWeights = saturate(albedo / albedoMax);
        }
        giAccum.r *= 1.0 + adaptiveWeights.r * g_gi_adaptive_r;
        giAccum.g *= 1.0 + adaptiveWeights.g * g_gi_adaptive_g;
        giAccum.b *= 1.0 + adaptiveWeights.b * g_gi_adaptive_b;

        // ── Adaptive Luma Normalization: boost dim GI toward target brightness ──
        // Evens out indoor/outdoor GI so both respond to the same settings.
        if (g_gi_adaptive_luma_strength > 0.001f) {
            float giLuma = dot(giAccum, float3(0.299, 0.587, 0.114));
            float target = g_gi_adaptive_luma_strength;
            // Only boost dim GI up; don't crush bright GI down.
            float scale = (giLuma > 0.001f && giLuma < target) ? (target / giLuma) : 1.0;
            giAccum *= lerp(1.0, scale, g_gi_adaptive_luma_blend);
        }
#endif

#ifdef GT_VBAO_COMPUTE_BENT_NORMALS
        bentNormal = normalize(bentNormal) ;
#endif
    }

#if defined(GT_VBAO_SHOW_DEBUG_VIZ) && defined(GT_VBAO_COMPUTE_BENT_NORMALS)
    [branch] if (IsUnderCursorRange(pixCoord, int2(1, 1)))
    {
        float3 dbgWorldViewNorm = mul((float3x3)g_globals.ViewInv, viewspaceNormal).xyz;
        float3 dbgWorldBentNorm = mul((float3x3)g_globals.ViewInv, bentNormal).xyz;
        DebugDraw3DSphereCone( dbgWorldPos, dbgWorldViewNorm, 0.3, VA_PI*0.5 - acos(saturate(visibility)), float4( 0.2, 0.2, 0.2, 0.5 ) );
        DebugDraw3DSphereCone( dbgWorldPos, dbgWorldBentNorm, 0.3, VA_PI*0.5 - acos(saturate(visibility)), float4( 0.0, 1.0, 0.0, 0.7 ) );
    }
#endif

    GTVBAO_OutputWorkingTerm( pixCoord, visibility, bentNormal, outWorkingAOTerm );

#ifdef GT_VBAO_COMPUTE_GI
    // ── SSGI debug view 5: sample activity heatmap ──
    // Red = rejected samples (valid but newCount==0), Green = contributed, Yellow = mix
    int vbgiDbgMode = (int)g_vbgi_debug_view;
    if (vbgiDbgMode == 5) {
        float totalActivity = (float)max(activityContributed + activityRejected, 1u);
        float r = (float)activityRejected / totalActivity;
        float g = (float)activityContributed / totalActivity;
        outDebug[pixCoord] = float4(r, g, 0.0, 1.0);
    } else {
    // ── Debug views 6-8: bitmask visualizations (dedicated UAV, independent of GI) ──
    int dbgMode = (int)GTVBAO_debug_mode;
    if (dbgMode == 6) {
        float avgCoverage = (debugTotalSamples > 0u)
            ? (float)debugTotalSectorCoverage / (float)(debugTotalSamples * GT_VBAO_BITMASK_SECTOR_COUNT)
            : 0.0f;
        outDebug[pixCoord] = float4(avgCoverage, 0.5f * (1.0f - avgCoverage), 1.0f - avgCoverage, 1.0f);
    } else if (dbgMode == 7) {
        float avgOccluded = (debugTotalSamples > 0u)
            ? (float)debugTotalSectorCoverage / (float)max(debugTotalSamples, 1u)
            : 0.0f;
        float gray = 1.0f - saturate(avgOccluded / (float)GT_VBAO_BITMASK_SECTOR_COUNT);
        outDebug[pixCoord] = float4(gray, gray, gray, 1.0f);
    } else if (dbgMode == 8) {
        uint bm = debugFirstSliceBitmask;
        float r = (float)(bm & 0xFFu) / 255.0f;
        float g = (float)((bm >> 8u) & 0xFFu) / 255.0f;
        float b = (float)((bm >> 16u) & 0xFFu) / 255.0f;
        outDebug[pixCoord] = float4(r, g, b, 1.0f);
    } else {
        outDebug[pixCoord] = float4(0, 0, 0, 0);
    }
    } // end vbgiDbgMode==5 vs bitmask debug

    if (enableGI)
        outGI[pixCoord] = float4(giAccum, (float)visibility);
    else
        outGI[pixCoord] = float4(0, 0, 0, 0);
#endif
}

// weighted average depth filter
lpfloat GTVBAO_DepthMIPFilter( lpfloat depth0, lpfloat depth1, lpfloat depth2, lpfloat depth3, const GTAOConstants consts )
{
    lpfloat maxDepth = max( max( depth0, depth1 ), max( depth2, depth3 ) );

    const lpfloat depthRangeScaleFactor = 0.75; // found empirically :)
#if GT_VBAO_USE_DEFAULT_CONSTANTS != 0
    const lpfloat effectRadius              = depthRangeScaleFactor * (lpfloat)consts.EffectRadius * (lpfloat)GT_VBAO_DEFAULT_RADIUS_MULTIPLIER;
    const lpfloat falloffRange              = (lpfloat)GT_VBAO_DEFAULT_FALLOFF_RANGE * effectRadius;
#else
    const lpfloat effectRadius              = depthRangeScaleFactor * (lpfloat)consts.EffectRadius * (lpfloat)consts.RadiusMultiplier;
    const lpfloat falloffRange              = (lpfloat)consts.EffectFalloffRange * effectRadius;
#endif
    const lpfloat falloffFrom       = effectRadius * ((lpfloat)1-(lpfloat)consts.EffectFalloffRange);
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

// This is also a good place to do non-linear depth conversion for cases where one wants the 'radius' (effectively the threshold between near-field and far-field GI), 
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
    lpfloat dm1 = GTVBAO_DepthMIPFilter( depth0, depth1, depth2, depth3, consts );
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

        lpfloat dm2 = GTVBAO_DepthMIPFilter( inTL, inTR, inBL, inBR, consts );
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

        lpfloat dm3 = GTVBAO_DepthMIPFilter( inTL, inTR, inBL, inBR, consts );
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

        lpfloat dm4 = GTVBAO_DepthMIPFilter( inTL, inTR, inBL, inBR, consts );
        outDepth4[ baseCoord / 8 ] = (lpfloat)dm4;
        //g_scratchDepths[ groupThreadID.x ][ groupThreadID.y ] = dm4;
    }
}

lpfloat4 GTVBAO_UnpackEdges( lpfloat _packedVal )
{
    uint packedVal = (uint)(_packedVal * 255.5);
    lpfloat4 edgesLRTB;
    edgesLRTB.x = lpfloat((packedVal >> 6) & 0x03) / 3.0;          // there's really no need for mask (as it's an 8 bit input) but I'll leave it in so it doesn't cause any trouble in the future
    edgesLRTB.y = lpfloat((packedVal >> 4) & 0x03) / 3.0;
    edgesLRTB.z = lpfloat((packedVal >> 2) & 0x03) / 3.0;
    edgesLRTB.w = lpfloat((packedVal >> 0) & 0x03) / 3.0;

    return saturate( edgesLRTB );
}

#ifdef GT_VBAO_COMPUTE_BENT_NORMALS
typedef lpfloat4 AOTermType;            // .xyz is bent normal, .w is visibility term
#else
typedef lpfloat AOTermType;             // .x is visibility term
#endif

void GTVBAO_AddSample( AOTermType ssaoValue, lpfloat edgeValue, inout AOTermType sum, inout lpfloat sumWeight )
{
    lpfloat weight = edgeValue;    

    sum += (weight * ssaoValue);
    sumWeight += weight;
}

void GTVBAO_Output( uint2 pixCoord, RWTexture2D<uint> outputTexture, AOTermType outputValue, const uniform bool finalApply )
{
#ifdef GT_VBAO_COMPUTE_BENT_NORMALS
    lpfloat     visibility = outputValue.w * ((finalApply)?((lpfloat)GT_VBAO_OCCLUSION_TERM_SCALE):(1));
    lpfloat3    bentNormal = normalize(outputValue.xyz);
    outputTexture[pixCoord.xy] = GTVBAO_EncodeVisibilityBentNormal( visibility, bentNormal );
#else
    // Multiply back OCCLUSION_TERM_SCALE on final output to undo the
    // pre-denoise division in GTVBAO_OutputWorkingTerm (same as bent-normal path).
    lpfloat vis = outputValue;
    if (finalApply) vis = saturate(vis * (lpfloat)GT_VBAO_OCCLUSION_TERM_SCALE);
    outputTexture[pixCoord.xy] = uint(vis * 255.0 + 0.5);
#endif
}

void GTVBAO_DecodeGatherPartial( const uint4 packedValue, out AOTermType outDecoded[4] )
{
    for( int i = 0; i < 4; i++ )
#ifdef GT_VBAO_COMPUTE_BENT_NORMALS
        GTVBAO_DecodeVisibilityBentNormal( packedValue[i], outDecoded[i].w, outDecoded[i].xyz );
#else
        outDecoded[i] = lpfloat(packedValue[i]) / lpfloat(255.0);
#endif
}

void GTVBAO_Denoise( const uint2 pixCoordBase, const GTAOConstants consts, Texture2D<uint> sourceAOTerm, Texture2D<lpfloat> sourceEdges, SamplerState texSampler, RWTexture2D<uint> outputTexture, const uniform bool finalApply )
{
    const lpfloat blurAmount = (finalApply)?((lpfloat)consts.DenoiseBlurBeta):((lpfloat)consts.DenoiseBlurBeta/(lpfloat)5.0);
    const lpfloat diagWeight = 0.85 * 0.5;

    AOTermType aoTerm[2];   // pixel pixCoordBase and pixel pixCoordBase + int2( 1, 0 )
    lpfloat4 edgesC_LRTB[2];
    lpfloat weightTL[2];
    lpfloat weightTR[2];
    lpfloat weightBL[2];
    lpfloat weightBR[2];

    // gather edge and visibility quads, used later
    const float2 gatherCenter = float2( pixCoordBase.x, pixCoordBase.y ) * consts.ViewportPixelSize;
    lpfloat4 edgesQ0        = sourceEdges.GatherRed( texSampler, gatherCenter, int2( 0, 0 ) );
    lpfloat4 edgesQ1        = sourceEdges.GatherRed( texSampler, gatherCenter, int2( 2, 0 ) );
    lpfloat4 edgesQ2        = sourceEdges.GatherRed( texSampler, gatherCenter, int2( 1, 2 ) );

    AOTermType visQ0[4];    GTVBAO_DecodeGatherPartial( sourceAOTerm.GatherRed( texSampler, gatherCenter, int2( 0, 0 ) ), visQ0 );
    AOTermType visQ1[4];    GTVBAO_DecodeGatherPartial( sourceAOTerm.GatherRed( texSampler, gatherCenter, int2( 2, 0 ) ), visQ1 );
    AOTermType visQ2[4];    GTVBAO_DecodeGatherPartial( sourceAOTerm.GatherRed( texSampler, gatherCenter, int2( 0, 2 ) ), visQ2 );
    AOTermType visQ3[4];    GTVBAO_DecodeGatherPartial( sourceAOTerm.GatherRed( texSampler, gatherCenter, int2( 2, 2 ) ), visQ3 );

    for( int side = 0; side < 2; side++ )
    {
        const int2 pixCoord = int2( pixCoordBase.x + side, pixCoordBase.y );

        lpfloat4 edgesL_LRTB  = GTVBAO_UnpackEdges( (side==0)?(edgesQ0.x):(edgesQ0.y) );
        lpfloat4 edgesT_LRTB  = GTVBAO_UnpackEdges( (side==0)?(edgesQ0.z):(edgesQ1.w) );
        lpfloat4 edgesR_LRTB  = GTVBAO_UnpackEdges( (side==0)?(edgesQ1.x):(edgesQ1.y) );
        lpfloat4 edgesB_LRTB  = GTVBAO_UnpackEdges( (side==0)?(edgesQ2.w):(edgesQ2.z) );

        edgesC_LRTB[side]     = GTVBAO_UnpackEdges( (side==0)?(edgesQ0.y):(edgesQ1.x) );

        // Edges aren't perfectly symmetrical: edge detection algorithm does not guarantee that a left edge on the right pixel will match the right edge on the left pixel (although
        // they will match in majority of cases). This line further enforces the symmetricity, creating a slightly sharper blur. Works real nice with TAA.
        edgesC_LRTB[side] *= lpfloat4( edgesL_LRTB.y, edgesR_LRTB.x, edgesT_LRTB.w, edgesB_LRTB.z );

#if 1   // this allows some small amount of AO leaking from neighbours if there are 3 or 4 edges; this reduces both spatial and temporal aliasing
        const lpfloat leak_threshold = (lpfloat)GTVBAO_denoise_leak_threshold; const lpfloat leak_strength = (lpfloat)GTVBAO_denoise_leak_strength;
        lpfloat edginess = (saturate(4.0 - leak_threshold - dot( edgesC_LRTB[side], lpfloat4( 1.0, 1.0, 1.0, 1.0 ) )) / (4-leak_threshold)) * leak_strength;
        edgesC_LRTB[side] = saturate( edgesC_LRTB[side] + edginess );
#endif

#ifdef GT_VBAO_SHOW_EDGES
        g_outputDbgImage[pixCoord] = 1.0 - lpfloat4( edgesC_LRTB[side].x, edgesC_LRTB[side].y * 0.5 + edgesC_LRTB[side].w * 0.5, edgesC_LRTB[side].z, 1.0 );
        //g_outputDbgImage[pixCoord] = 1 - float4( edgesC_LRTB[side].z, edgesC_LRTB[side].w , 1, 0 );
        //g_outputDbgImage[pixCoord] = edginess.xxxx;
#endif

        // for diagonals; used by first and second pass
        weightTL[side] = diagWeight * (edgesC_LRTB[side].x * edgesL_LRTB.z + edgesC_LRTB[side].z * edgesT_LRTB.x);
        weightTR[side] = diagWeight * (edgesC_LRTB[side].z * edgesT_LRTB.y + edgesC_LRTB[side].y * edgesR_LRTB.z);
        weightBL[side] = diagWeight * (edgesC_LRTB[side].w * edgesB_LRTB.x + edgesC_LRTB[side].x * edgesL_LRTB.w);
        weightBR[side] = diagWeight * (edgesC_LRTB[side].y * edgesR_LRTB.w + edgesC_LRTB[side].w * edgesB_LRTB.y);

        // first pass
        AOTermType ssaoValue     = (side==0)?(visQ0[1]):(visQ1[0]);
        AOTermType ssaoValueL    = (side==0)?(visQ0[0]):(visQ0[1]);
        AOTermType ssaoValueT    = (side==0)?(visQ0[2]):(visQ1[3]);
        AOTermType ssaoValueR    = (side==0)?(visQ1[0]):(visQ1[1]);
        AOTermType ssaoValueB    = (side==0)?(visQ2[2]):(visQ3[3]);
        AOTermType ssaoValueTL   = (side==0)?(visQ0[3]):(visQ0[2]);
        AOTermType ssaoValueBR   = (side==0)?(visQ3[3]):(visQ3[2]);
        AOTermType ssaoValueTR   = (side==0)?(visQ1[3]):(visQ1[2]);
        AOTermType ssaoValueBL   = (side==0)?(visQ2[3]):(visQ2[2]);

        lpfloat sumWeight = blurAmount;
        AOTermType sum = ssaoValue * sumWeight;

        GTVBAO_AddSample( ssaoValueL, edgesC_LRTB[side].x, sum, sumWeight );
        GTVBAO_AddSample( ssaoValueR, edgesC_LRTB[side].y, sum, sumWeight );
        GTVBAO_AddSample( ssaoValueT, edgesC_LRTB[side].z, sum, sumWeight );
        GTVBAO_AddSample( ssaoValueB, edgesC_LRTB[side].w, sum, sumWeight );

        GTVBAO_AddSample( ssaoValueTL, weightTL[side], sum, sumWeight );
        GTVBAO_AddSample( ssaoValueTR, weightTR[side], sum, sumWeight );
        GTVBAO_AddSample( ssaoValueBL, weightBL[side], sum, sumWeight );
        GTVBAO_AddSample( ssaoValueBR, weightBR[side], sum, sumWeight );

        aoTerm[side] = sum / sumWeight;

        GTVBAO_Output( pixCoord, outputTexture, aoTerm[side], finalApply );

#ifdef GT_VBAO_SHOW_BENT_NORMALS
        if( finalApply )
        {
            g_outputDbgImage[pixCoord] = float4( DisplayNormalSRGB( aoTerm[side].xyz /** aoTerm[side].www*/ ), 1 );
        }
#endif

    }
}


// ──────────────────────────────────────────────────────────────────────────
// Poisson Denoiser (denoiser_type = 2)
// Based on "Self-Supervised Poisson-Gaussian Denoising" (Khademi et al., WACV 2021)
// and "Poisson2Sparse" (2022). Uses luma/depth/normal similarity weights.
// ──────────────────────────────────────────────────────────────────────────

// ── Procedural Poisson disk sample: golden-angle spiral ──
// Returns (offset2D, radius01). The offset is in [-radius, +radius] range.
float3 GTVBAO_PoissonSample(uint idx, uint total, float rotAngle)
{
    // Golden-angle spiral: 2*PI * (golden ratio conjugate) * idx + rotation
    float angle = 6.283185307f * 0.381966011f * (float)idx + rotAngle;
    float radius = (total <= 1u) ? 0.0f : ((float)idx / (float)(total - 1u));
    return float3(cos(angle) * radius, sin(angle) * radius, radius);
}

// ── Hash-based rotation angle from pixel + frame ──
float GTVBAO_PoissonRotation(uint2 pixCoord)
{
    // Standard hash: frac(sin(dot) * largePrime)
    float h = frac(sin(dot((float2)pixCoord, float2(12.9898f, 78.233f))) * 43758.5453f);
    return h * 6.283185307f; // 0..2PI
}

// ── Compute viewspace normal from depth (5-tap Sobel-like) ──
float3 GTVBAO_ComputeNormalFromDepth(
    uint2 pixCoord, uint2 texSize,
    Texture2D<float> depthTex, SamplerState samp)
{
    float2 uv = ((float2)pixCoord + 0.5f) / (float2)texSize;
    float2 ts = 1.0f / (float2)texSize;

    float dC = depthTex.SampleLevel(samp, uv, 0);
    float dL = depthTex.SampleLevel(samp, uv - float2(ts.x, 0), 0);
    float dR = depthTex.SampleLevel(samp, uv + float2(ts.x, 0), 0);
    float dT = depthTex.SampleLevel(samp, uv + float2(0, ts.y), 0);
    float dB = depthTex.SampleLevel(samp, uv - float2(0, ts.y), 0);

    // Choose closer neighbor for each axis (depth edge-aware)
    float dx = (abs(dL - dC) < abs(dR - dC)) ? (dC - dL) : (dR - dC);
    float dy = (abs(dB - dC) < abs(dT - dC)) ? (dC - dB) : (dT - dC);

    // Reconstruct viewspace positions for center and neighbors
    float2 uvL = uv - float2(ts.x, 0);
    float2 uvD = uv - float2(0, ts.y);
    float3 vC = float3((uv * 2.0f - 1.0f) * dC, dC);
    float3 vL = float3((uvL * 2.0f - 1.0f) * dL, dL);
    float3 vD = float3((uvD * 2.0f - 1.0f) * dB, dB);
    float3 vR = float3(((uv + float2(ts.x, 0)) * 2.0f - 1.0f) * dR, dR);
    float3 vU = float3(((uv + float2(0, ts.y)) * 2.0f - 1.0f) * dT, dT);

    float3 dpdx = (abs(dL - dC) < abs(dR - dC)) ? (vC - vL) : (vR - vC);
    float3 dpdy = (abs(dB - dC) < abs(dT - dC)) ? (vC - vD) : (vU - vC);

    float3 n = cross(dpdx, dpdy);
    return (dot(n, n) > 1e-10f) ? normalize(n) : float3(0, 0, 1);
}

// ── Poisson denoise for AO (single-channel) ──
// AO-value similarity only — depth edges naturally appear in AO data.
// Respects DenoiseBlurBeta. Intermediate passes use relaxed phi.
void GTVBAO_DenoiseAO_Poisson(
    uint2 pixCoordBase, GTAOConstants consts,
    Texture2D<uint> srcAO, Texture2D<float> depthTex,
    Texture2D<uint4> mrtNormalTex,
    SamplerState samp, RWTexture2D<uint> outAO,
    const uniform bool finalApply)
{
    uint w, h;
    srcAO.GetDimensions(w, h);
    uint totalSamples = max(1u, (uint)GTVBAO_poisson_samples);

    float beta = max(0.5f, consts.DenoiseBlurBeta);
    float betaScale = 20.0f / beta;
    float lumaPhi = max(0.5f, GTVBAO_poisson_luma_phi * betaScale);
    if (!finalApply) { lumaPhi *= 4.0f; totalSamples = max(2u, totalSamples / 2u); }
    float sampleRadiusPx = (finalApply ? 2.5f : 1.5f) * (float)max(w, h) * 0.015f;
    float invLumaPhi = 1.0f / lumaPhi;

    // Center weight proportional to DenoiseBlurBeta (matching existing denoiser)
    float centerWeight = max(1.0f, beta * 0.5f);

    // Shared rotation for both pixels
    float h0 = frac(sin(float(pixCoordBase.x) * 12.9898f + float(pixCoordBase.y) * 78.233f) * 43758.5453f);

    [unroll(2)]
    for (int side = 0; side < 2; side++)
    {
        int2 pc = int2(pixCoordBase.x + side, pixCoordBase.y);
        if (pc.x >= (int)w || pc.y >= (int)h) continue;

        float centerAO = (float)srcAO.Load(int3(pc, 0)) * 0.0039215686f; // /255

        // ── Optional depth-aware bilateral pre-filter ──
        if (GTVBAO_prefilter_enabled > 0.5f) {
            float centerDepth = depthTex.Load(int3(pc, 0));
            float filteredSum = centerAO, filteredW = 1.0f;
            [unroll]
            for (int dy2 = -1; dy2 <= 1; dy2++) {
                [unroll]
                for (int dx2 = -1; dx2 <= 1; dx2++) {
                    if (dx2 == 0 && dy2 == 0) continue;
                    int2 npc = int2(pc.x + dx2, pc.y + dy2);
                    if (npc.x < 0 || npc.y < 0 || npc.x >= (int)w || npc.y >= (int)h) continue;
                    float nAO = (float)srcAO.Load(int3(npc, 0)) * 0.0039215686f;
                    float nDepth = depthTex.Load(int3(npc, 0));
                    float depthW = exp(-abs(centerDepth - nDepth) * 10.0f);
                    filteredSum += nAO * depthW;
                    filteredW += depthW;
                }
            }
            centerAO = filteredSum / filteredW;
        }

        float sum = centerAO * centerWeight, totalW = centerWeight;

        [loop]
        for (uint i = 0u; i < totalSamples; i++)
        {
            float a = (float)i * 2.3999632297f + h0 * 6.283185307f;
            float r = (totalSamples <= 1u) ? 0.0f : ((float)i / (float)(totalSamples - 1u));
            int2 nc = int2(float2(pc) + float2(cos(a) * r, sin(a) * r) * sampleRadiusPx + 0.5f);
            nc = clamp(nc, int2(0, 0), int2(w - 1, h - 1));

            float nAO = (float)srcAO.Load(int3(nc, 0)) * 0.0039215686f;
            float wgt = max(1.0f - abs(centerAO - nAO) * invLumaPhi, 0.0f);
            sum += nAO * wgt;
            totalW += wgt;
        }

        float denoised = sum / max(totalW, 1e-5f);
        GTVBAO_Output(pc, outAO, (AOTermType)denoised, finalApply);
    }
}

// ── Poisson denoise for GI (float3 color) ──
// Luma + depth + normal similarity. Respects DenoiseBlurBeta.
void GTVBAO_DenoiseGI_Poisson(
    uint2 pixCoordBase, GTAOConstants consts,
    Texture2D<float4> srcGI, Texture2D<float> depthTex,
    Texture2D<uint4> mrtNormalTex,
    SamplerState samp, RWTexture2D<float4> outGI)
{
    uint w, h;
    srcGI.GetDimensions(w, h);
    uint totalSamples = max(1u, (uint)GTVBAO_poisson_samples);

    float beta = max(0.5f, consts.DenoiseBlurBeta);
    float betaScale = 20.0f / beta;
    float lumaPhi  = max(0.01f, GTVBAO_poisson_luma_phi * betaScale);
    float depthPhi = max(0.01f, GTVBAO_poisson_depth_phi * betaScale);
    float normalPhi = max(0.01f, GTVBAO_poisson_normal_phi * betaScale);
    float sampleRadiusPx = 2.5f * (float)max(w, h) * 0.015f;
    float invLumaPhi = 1.0f / max(lumaPhi, 0.001f);
    float invDepthPhi = 1.0f / max(depthPhi, 0.001f);

    float h0 = frac(sin(float(pixCoordBase.x) * 12.9898f + float(pixCoordBase.y) * 78.233f) * 43758.5453f);

    [unroll(2)]
    for (int side = 0; side < 2; side++)
    {
        int2 pc = int2(pixCoordBase.x + side, pixCoordBase.y);
        if (pc.x >= (int)w || pc.y >= (int)h) continue;

        float centerDepth = depthTex.Load(int3(pc, 0));
        float3 centerGI = srcGI.Load(int3(pc, 0)).rgb;
        float centerLuma = dot(centerGI, float3(0.299f, 0.587f, 0.114f));

        float3 centerNormal;
        if (GTVBAO_mrt_normal_available > 0.5f) {
            uint4 packed = mrtNormalTex.Load(int3(pc, 0));
            float2 enc = float2((float)packed.x, (float)packed.y) * (1.0f / 32767.5f) + float2(-1.0f, -1.0f);
            float azimuth = 3.14159274f * enc.x;
            float sin_a, cos_a;
            sincos(azimuth, sin_a, cos_a);
            float ring = sqrt(saturate(1.0f - enc.y * enc.y));
            centerNormal = float3(cos_a * ring, sin_a * ring, enc.y);
        } else {
            centerNormal = GTVBAO_ComputeNormalFromDepth(pc, uint2(w, h), depthTex, samp);
        }

        float3 sum = centerGI;
        float totalW = 1.0f;

        [loop]
        for (uint i = 0u; i < totalSamples; i++)
        {
            float a = (float)i * 2.3999632297f + h0 * 6.283185307f;
            float r = (totalSamples <= 1u) ? 0.0f : ((float)i / (float)(totalSamples - 1u));
            int2 nc = int2(float2(pc) + float2(cos(a) * r, sin(a) * r) * sampleRadiusPx + 0.5f);
            nc = clamp(nc, int2(0, 0), int2(w - 1, h - 1));

            float3 nGI = srcGI.Load(int3(nc, 0)).rgb;
            float nLuma = dot(nGI, float3(0.299f, 0.587f, 0.114f));
            float nDepth = depthTex.Load(int3(nc, 0));

            float lw = max(1.0f - abs(centerLuma - nLuma) * invLumaPhi, 0.0f);
            float dw = max(1.0f - abs(centerDepth - nDepth) * invDepthPhi, 0.0f);

            float3 nNormal;
            if (GTVBAO_mrt_normal_available > 0.5f) {
                uint4 packed = mrtNormalTex.Load(int3(nc, 0));
                float2 enc = float2((float)packed.x, (float)packed.y) * (1.0f / 32767.5f) + float2(-1.0f, -1.0f);
                float az = 3.14159274f * enc.x;
                float sa, ca;
                sincos(az, sa, ca);
                float ring = sqrt(saturate(1.0f - enc.y * enc.y));
                nNormal = float3(ca * ring, sa * ring, enc.y);
            } else {
                nNormal = GTVBAO_ComputeNormalFromDepth(nc, uint2(w, h), depthTex, samp);
            }
            float nw = pow(saturate(dot(centerNormal, nNormal)), normalPhi);

            sum += nGI * lw * dw * nw;
            totalW += lw * dw * nw;
        }

        outGI[pc] = float4(sum / max(totalW, 1e-5f), 1.0f);
    }
}

// ── Merged AO+GI Poisson denoise — single sample loop, shared reads ──
void GTVBAO_Denoise_Poisson(
    uint2 pixCoordBase, GTAOConstants consts,
    Texture2D<uint> srcAO, Texture2D<float4> srcGI,
    Texture2D<float> depthTex, Texture2D<uint4> mrtNormalTex,
    SamplerState samp,
    RWTexture2D<uint> outAO, RWTexture2D<float4> outGI,
    const uniform bool finalApply)
{
    uint w, h;
    srcAO.GetDimensions(w, h);
    uint totalSamples = max(1u, (uint)GTVBAO_poisson_samples);

    float beta = max(0.5f, consts.DenoiseBlurBeta);
    float betaScale = 20.0f / beta;

    float aoLumaPhi = max(0.5f, GTVBAO_poisson_luma_phi * betaScale);
    uint aoTotalSamples = totalSamples;
    if (!finalApply) { aoLumaPhi *= 4.0f; aoTotalSamples = max(2u, aoTotalSamples / 2u); }
    float aoRadiusPx = (finalApply ? 2.5f : 1.5f) * (float)max(w, h) * 0.015f;
    float aoInvLumaPhi = 1.0f / aoLumaPhi;

    bool doGI = (g_gi_enabled > 0.5f);
    float giLumaPhi, giDepthPhi, giNormalPhi, giInvLPhi, giInvDPhi, giRadiusPx;
    if (doGI) {
        giLumaPhi  = max(0.01f, GTVBAO_poisson_luma_phi * betaScale);
        giDepthPhi = max(0.01f, GTVBAO_poisson_depth_phi * betaScale);
        giNormalPhi = max(0.01f, GTVBAO_poisson_normal_phi * betaScale);
        giInvLPhi = 1.0f / max(giLumaPhi, 0.001f);
        giInvDPhi = 1.0f / max(giDepthPhi, 0.001f);
        giRadiusPx = 2.5f * (float)max(w, h) * 0.015f;
    }

    float h0 = frac(sin(float(pixCoordBase.x) * 12.9898f + float(pixCoordBase.y) * 78.233f) * 43758.5453f);

    [unroll(2)]
    for (int side = 0; side < 2; side++)
    {
        int2 pc = int2(pixCoordBase.x + side, pixCoordBase.y);
        if (pc.x >= (int)w || pc.y >= (int)h) continue;

        float centerAO = (float)srcAO.Load(int3(pc, 0)) * 0.0039215686f;
        float sumAO = centerAO, totalWAO = 1.0f;

        float3 sumGI = float3(0, 0, 0);
        float totalWGI = 0.0f;
        float centerDepth = 0.0f, centerLuma = 0.0f;
        float3 centerNormal = float3(0, 0, 0);
        if (doGI) {
            centerDepth = depthTex.Load(int3(pc, 0));
            float3 cGI = srcGI.Load(int3(pc, 0)).rgb;
            centerLuma = dot(cGI, float3(0.299f, 0.587f, 0.114f));
            sumGI = cGI; totalWGI = 1.0f;
            if (GTVBAO_mrt_normal_available > 0.5f) {
                uint4 p = mrtNormalTex.Load(int3(pc, 0));
                float2 e = float2((float)p.x, (float)p.y) * (1.0f / 32767.5f) + float2(-1.0f, -1.0f);
                float az = 3.14159274f * e.x; float sa, ca; sincos(az, sa, ca);
                centerNormal = float3(ca * sqrt(saturate(1.0f - e.y * e.y)), sa * sqrt(saturate(1.0f - e.y * e.y)), e.y);
            } else {
                centerNormal = GTVBAO_ComputeNormalFromDepth(pc, uint2(w, h), depthTex, samp);
            }
        }

        uint maxS = doGI ? max(aoTotalSamples, totalSamples) : aoTotalSamples;
        [loop]
        for (uint i = 0u; i < maxS; i++)
        {
            float a = (float)i * 2.3999632297f + h0 * 6.283185307f;

            if (i < aoTotalSamples) {
                float r = (aoTotalSamples <= 1u) ? 0.0f : ((float)i / (float)(aoTotalSamples - 1u));
                int2 nc = int2(float2(pc) + float2(cos(a) * r, sin(a) * r) * aoRadiusPx + 0.5f);
                nc = clamp(nc, int2(0, 0), int2(w - 1, h - 1));
                float nAO = (float)srcAO.Load(int3(nc, 0)) * 0.0039215686f;
                float aw = max(1.0f - abs(centerAO - nAO) * aoInvLumaPhi, 0.0f);
                sumAO += nAO * aw; totalWAO += aw;
            }

            if (doGI && i < totalSamples) {
                float r = (totalSamples <= 1u) ? 0.0f : ((float)i / (float)(totalSamples - 1u));
                int2 nc = int2(float2(pc) + float2(cos(a) * r, sin(a) * r) * giRadiusPx + 0.5f);
                nc = clamp(nc, int2(0, 0), int2(w - 1, h - 1));
                float3 nGI = srcGI.Load(int3(nc, 0)).rgb;
                float nDepth = depthTex.Load(int3(nc, 0));
                float lw = max(1.0f - abs(centerLuma - dot(nGI, float3(0.299f, 0.587f, 0.114f))) * giInvLPhi, 0.0f);
                float dw = max(1.0f - abs(centerDepth - nDepth) * giInvDPhi, 0.0f);

                float3 nN;
                if (GTVBAO_mrt_normal_available > 0.5f) {
                    uint4 p = mrtNormalTex.Load(int3(nc, 0));
                    float2 e = float2((float)p.x, (float)p.y) * (1.0f / 32767.5f) + float2(-1.0f, -1.0f);
                    float az = 3.14159274f * e.x; float sa, ca; sincos(az, sa, ca);
                    nN = float3(ca * sqrt(saturate(1.0f - e.y * e.y)), sa * sqrt(saturate(1.0f - e.y * e.y)), e.y);
                } else {
                    nN = GTVBAO_ComputeNormalFromDepth(nc, uint2(w, h), depthTex, samp);
                }
                float nw = pow(saturate(dot(centerNormal, nN)), giNormalPhi);
                sumGI += nGI * lw * dw * nw; totalWGI += lw * dw * nw;
            }
        }

        float dAO = sumAO / max(totalWAO, 1e-5f);
        GTVBAO_Output(pc, outAO, (AOTermType)dAO, finalApply);
        if (doGI) outGI[pc] = float4(sumGI / max(totalWGI, 1e-5f), 1.0f);
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
