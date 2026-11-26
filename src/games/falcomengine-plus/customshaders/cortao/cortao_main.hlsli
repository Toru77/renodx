// Falcom Engine+ CORTAO main pass (SM 5.0). SPDX-License-Identifier: MIT

#include "cortao_common.hlsl"

#define CORTAO_RAY_COUNT 16
#define CORTAO_OCCLUDER_LOOKUP_TABLE 1
#ifndef FALCOM_CORTAO_BENT_NORMALS
#define FALCOM_CORTAO_BENT_NORMALS 0
#endif
#define CORTAO_BENT_NORMAL_LOOKUP_TABLE FALCOM_CORTAO_BENT_NORMALS
#define FALCOM_CORTAO_USER_NOISE 1  // Noise Type (M6a); CORTAO option = the reference expression below
#include "../gtvbao/GTVBAO.h"        // HilbertIndex, XE_HILBERT_WIDTH (shared with GTVBAO's Hilbert noise)

#include "../../reference/CORTAO-main/src/shaders/CORTAOMainPass.h"

Texture2D<float> g_srcPacked : register(t0);          // R32_FLOAT packed data (SM 5 workaround)
Texture2D<uint4> g_srcMrtNormal : register(t1);
Texture3D<float> g_srcOccluderLut : register(t2);     // 24x24x16 R16_UNORM, 16 rays
Texture2D<uint> g_srcFoliageMask : register(t3);      // M2
SamplerState g_samplerPointBorder : register(s0);     // nearest, border 0 (README main pass)
SamplerState g_samplerPointClamp : register(s1);      // nearest, clamp-to-edge
RWTexture2D<float4> g_outSvgfRaw : register(u0);
#if FALCOM_CORTAO_BENT_NORMALS
Buffer<cortao_half2> g_srcBentNormalLut : register(t4);  // 65536 x R8G8_SNORM
RWTexture2D<float4> g_outOcclusion : register(u1);      // filter input, R8G8B8A8_SNORM
RWTexture2D<float4> g_outBentNormal : register(u2);     // R8G8B8A8_SNORM
#else
RWTexture2D<float> g_outOcclusion : register(u1);     // filter input, R8_UNORM (M3)
#endif
RWTexture2D<float4> g_outDebug : register(u3);        // u2 is the bent output (M4)
Texture3D<float2> g_srcIsfastNoise : register(t5);    // IS-FAST 128x128x32 RG8 (fast_noise_ea.dds), M6a
Texture2D<float> g_srcGameDepth : register(t6);       // M12 game depth for normal shaping (captured_depth_srv)

static bool g_falcomCortaoMaskFoliage = false;
cortao_uint cortao_samplePackedData(cortao_float2 texCoords, cortao_float mipmap) {
  const cortao_uint packed = asuint(g_srcPacked.SampleLevel(g_samplerPointBorder, texCoords, mipmap));
  // Foliage does not cast for non-foliage receivers: packed 0 is "no geometry", which
  // cortao_computeRayOcclusion always culls (CORTAOMainPass.h: sky and off-screen samples have depth 0).
  if (g_falcomCortaoMaskFoliage) {
    uint mw, mh;
    g_srcFoliageMask.GetDimensions(mw, mh);
    const int2 mc = min(int2(saturate(texCoords) * float2(mw, mh)), int2(mw, mh) - 1);
    if (g_srcFoliageMask.Load(int3(mc, 0)) != 0u) return 0u;
  }
  return packed;
}
cortao_half3 cortao_loadNormal(cortao_int2 pixelCoords) {
  return (cortao_half3)FalcomCortaoViewNormal(g_srcMrtNormal, g_srcGameDepth, pixelCoords);
}
cortao_uint cortao_sampleOccluderLookupTable(cortao_float3 texCoords) {
  return (cortao_uint)(g_srcOccluderLut.SampleLevel(g_samplerPointClamp, texCoords, 0.0) * 65535.0 + 0.5); // tables/occluder/README SM 6.7 workaround
}
// Falcom Engine+ noise extension (FALCOM_CORTAO_USER_NOISE in CORTAOMainPass.h): the per-pixel random values of the main pass.
cortao_uint2 FalcomCortaoUserNoise(cortao_uint2 pixelCoords, cortao_uint randomSeed) {
  // CORTAO: the unmodified reference expression (CORTAOMainPass.h, FALCOM_CORTAO_USER_NOISE 0 branch).
  if (CORTAO_noise_type < 0.5f) return cortao_hash(cortao_seed(pixelCoords) + cortao_unpack16(randomSeed));
  const uint t = (uint)CORTAO_noise_index;
  float2 n;
  if (CORTAO_noise_type < 1.5f) {
    // Hilbert: R2 over the 64x64 Hilbert curve, the sequence of gtvbao_main.cs_5_0.hlsl SpatioTemporalNoise_Hilbert.
    const uint i = HilbertIndex(pixelCoords.x % XE_HILBERT_WIDTH, pixelCoords.y % XE_HILBERT_WIDTH) + 288u * (t % 64u);
    n = float2(frac(0.5 + (float)i * 0.75487766624669276005), frac(0.5 + (float)i * 0.5698402909980532659114));
  } else {
    // IS-FAST: the raw texel; CORTAO needs uniform random numbers, so no GTVBAO strength/scale/seed shaping.
    n = g_srcIsfastNoise.Load(int4(pixelCoords.x & 127u, pixelCoords.y & 127u, t & 31u, 0));
  }
  // 24 significant bits per channel in the high bits; the low byte (spiral flip bit) from the other channel.
  const uint2 q = uint2(saturate(n) * 16777215.0);
  return (q << 8u) | (q.yx & 0xFFu);
}
#if FALCOM_CORTAO_BENT_NORMALS
cortao_half3 cortao_loadBentNormalLookupTable(cortao_uint rayBits) {
  cortao_half2 bentNormal = g_srcBentNormalLut.Load(cortao_int(rayBits));
  return cortao_half3(bentNormal, sqrt(cortao_half(1.0) - dot(bentNormal, bentNormal))); //reconstruct z
}
void cortao_writeResult(cortao_int2 pixelCoords, cortao_half occlusion, cortao_half3 bentNormal,
                        cortao_half3x3 tangentSpaceTransform, cortao_uint rayBits) {
  const float visibility = pow(saturate((float)occlusion), CORTAO_intensity); // README intensity tweak
  if (CORTAO_filter_enabled > 0.5f) g_outOcclusion[pixelCoords] = float4(visibility, (float3)bentNormal);
  else FalcomCortaoWriteOutputs(g_outSvgfRaw, g_outBentNormal, g_srcFoliageMask, pixelCoords, float4(visibility, (float3)bentNormal));
  if (CORTAO_debug_mode > 21.5f) {
    // With the filter off, the filtered views (24, 25) equal the main-pass result (M6d).
    if ((int)CORTAO_debug_mode == 22 || ((int)CORTAO_debug_mode == 24 && CORTAO_filter_enabled < 0.5f)) g_outDebug[pixelCoords] = float4(visibility.xxx, 1.0);
    else if ((int)CORTAO_debug_mode == 26) g_outDebug[pixelCoords] = float4((float3)tangentSpaceTransform[2] * 0.5 + 0.5, 1.0);
    else if ((int)CORTAO_debug_mode == 28 || ((int)CORTAO_debug_mode == 25 && CORTAO_filter_enabled < 0.5f)) g_outDebug[pixelCoords] = float4((float3)bentNormal * 0.5 + 0.5, 1.0);
  }
}
#else
void cortao_writeResult(cortao_int2 pixelCoords, cortao_half occlusion, cortao_half3 bentNormal,
                        cortao_half3x3 tangentSpaceTransform, cortao_uint rayBits) {
  const float visibility = pow(saturate((float)occlusion), CORTAO_intensity); // README intensity tweak
  if (CORTAO_filter_enabled > 0.5f) g_outOcclusion[pixelCoords] = visibility;
  else FalcomCortaoWriteVisibility(g_outSvgfRaw, g_srcFoliageMask, pixelCoords, visibility);
  if (CORTAO_debug_mode > 21.5f) {
    if ((int)CORTAO_debug_mode == 22 || ((int)CORTAO_debug_mode == 24 && CORTAO_filter_enabled < 0.5f)) g_outDebug[pixelCoords] = float4(visibility.xxx, 1.0);
    else if ((int)CORTAO_debug_mode == 26) g_outDebug[pixelCoords] = float4((float3)tangentSpaceTransform[2] * 0.5 + 0.5, 1.0);
  }
}
#endif

// Transcription of CORTAO::populateMainPassParameters, flipY branch.
CORTAOMainPassParameters FalcomCortaoPopulateMainPassParameters() {
  const float projX = proj_g[0][0], projY = proj_g[1][1], projOffsetX = proj_g[2][0], projOffsetY = proj_g[2][1];
  const float invX = projInv_g[0][0], invY = projInv_g[1][1], invOffsetX = projInv_g[3][0], invOffsetY = projInv_g[3][1];
  CORTAOMainPassParameters p = (CORTAOMainPassParameters)0;
  p.packedResolution = CORTAO_width | (CORTAO_height << 16u);
  p.randomSeed = CORTAO_random_seed;
  p.normalBias = invX / (float)CORTAO_width * sqrt(2.0);
  p.radius = CORTAO_max_radius;
  p.unprojectParams = float4(+2.0 * invX, -2.0 * invY, -invX + invOffsetX, +invY - invOffsetY);
  p.projectionParams = float4(+0.5 * projX, -0.5 * projY, -0.5 + 0.5 * projOffsetX, -0.5 - 0.5 * projOffsetY);
  const float maxApparentCircleRadius = invY * CORTAO_max_screenspace_radius_height_ratio * 2.0;
  p.maxApparentCircleRadiusCoefficient = maxApparentCircleRadius / sqrt(maxApparentCircleRadius * maxApparentCircleRadius + 1.0);
  p.mipBias = log2(projY * (float)CORTAO_height * 0.5) + CORTAO_mip_bias;
  p.inverseMaxSlopeCompensation = 1.0 / CORTAO_max_slope_compensation;
  p.backfaceThreshold = CORTAO_backface_threshold;
  p.backfaceBoost = CORTAO_backface_boost;
  return p;
}

// Verbatim from CORTAOMainPassOcclusionReference.hlsl.
uint2 deinterleave(uint index) {
  uint2 p = uint2(index, index) >> uint2(0, 1);

  p &= 0x5555u;
  p = (p ^ (p >> 1u)) & 0x3333u;
  p = (p ^ (p >> 2u)) & 0x0f0fu;
  p = (p ^ (p >> 4u)) & 0x00ffu;

  return p;
}

#define WORKGROUP_SIZE 16
[numthreads(WORKGROUP_SIZE, WORKGROUP_SIZE, 1)]
void main(uint2 workgroupID : SV_GroupID, uint localIndex : SV_GroupIndex) {
  const uint2 pixelCoords = workgroupID * uint2(WORKGROUP_SIZE, WORKGROUP_SIZE) + deinterleave(localIndex);
  if (pixelCoords.x >= CORTAO_width || pixelCoords.y >= CORTAO_height) return;
  const bool receiverFoliage = CORTAO_foliage_exclude > 0.5f && g_srcFoliageMask.Load(int3(pixelCoords, 0)) != 0u;
  g_falcomCortaoMaskFoliage = CORTAO_foliage_exclude > 0.5f && !receiverFoliage;
  if (receiverFoliage && CORTAO_foliage_ao_value <= 0.0f && CORTAO_debug_mode < 21.5f) {
    FalcomCortaoWriteVisibility(g_outSvgfRaw, g_srcFoliageMask, pixelCoords, 1.0); // GTVBAO perf P2 parity
#if FALCOM_CORTAO_BENT_NORMALS
    g_outBentNormal[pixelCoords] = float4(1.0, 0.0, 0.0, 1.0);
#endif
#if FALCOM_CORTAO_BENT_NORMALS
    if (CORTAO_filter_enabled > 0.5f) g_outOcclusion[pixelCoords] = float4(1.0, 0.0, 0.0, 1.0);
#else
    if (CORTAO_filter_enabled > 0.5f) g_outOcclusion[pixelCoords] = 1.0;
#endif
    return;
  }
  // cortao_mainPass returns before cortao_writeResult for sky pixels (CORTAOMainPass.h:374-377): pre-write full visibility.
  FalcomCortaoWriteVisibility(g_outSvgfRaw, g_srcFoliageMask, pixelCoords, 1.0);
#if FALCOM_CORTAO_BENT_NORMALS
  g_outBentNormal[pixelCoords] = float4(1.0, 0.0, 0.0, 1.0);
  if (CORTAO_filter_enabled > 0.5f) g_outOcclusion[pixelCoords] = float4(1.0, 0.0, 0.0, 1.0);
#else
  if (CORTAO_filter_enabled > 0.5f) g_outOcclusion[pixelCoords] = 1.0;
#endif
  if (CORTAO_debug_mode > 21.5f) g_outDebug[pixelCoords] = float4(0.25, 0.0, 0.25, 1.0);
  cortao_mainPass(pixelCoords, FalcomCortaoPopulateMainPassParameters());
}
