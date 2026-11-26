// Falcom Engine+ CORTAO main pass (SM 5.0). SPDX-License-Identifier: MIT

#include "cortao_common.hlsl"

#define CORTAO_RAY_COUNT 16
#define CORTAO_OCCLUDER_LOOKUP_TABLE 1
#define CORTAO_BENT_NORMAL_LOOKUP_TABLE 0

#include "../../reference/CORTAO-main/src/shaders/CORTAOMainPass.h"

Texture2D<float> g_srcPacked : register(t0);          // R32_FLOAT packed data (SM 5 workaround)
Texture2D<uint4> g_srcMrtNormal : register(t1);
Texture3D<float> g_srcOccluderLut : register(t2);     // 24x24x16 R16_UNORM, 16 rays
Texture2D<uint> g_srcFoliageMask : register(t3);      // M2
SamplerState g_samplerPointBorder : register(s0);     // nearest, border 0 (README main pass)
SamplerState g_samplerPointClamp : register(s1);      // nearest, clamp-to-edge
RWTexture2D<float4> g_outSvgfRaw : register(u0);
RWTexture2D<float> g_outOcclusion : register(u1);     // filter input, R8_UNORM (M3)
RWTexture2D<float4> g_outDebug : register(u3);        // u2 is the bent output (M4)

cortao_uint cortao_samplePackedData(cortao_float2 texCoords, cortao_float mipmap) {
  return asuint(g_srcPacked.SampleLevel(g_samplerPointBorder, texCoords, mipmap));
}
cortao_half3 cortao_loadNormal(cortao_int2 pixelCoords) {
  return (cortao_half3)FalcomCortaoViewNormal(g_srcMrtNormal, pixelCoords);
}
cortao_uint cortao_sampleOccluderLookupTable(cortao_float3 texCoords) {
  return (cortao_uint)(g_srcOccluderLut.SampleLevel(g_samplerPointClamp, texCoords, 0.0) * 65535.0 + 0.5); // tables/occluder/README SM 6.7 workaround
}
void cortao_writeResult(cortao_int2 pixelCoords, cortao_half occlusion, cortao_half3 bentNormal,
                        cortao_half3x3 tangentSpaceTransform, cortao_uint rayBits) {
  const float visibility = pow(saturate((float)occlusion), CORTAO_intensity); // README intensity tweak
  if (CORTAO_filter_enabled > 0.5f) g_outOcclusion[pixelCoords] = visibility;
  else FalcomCortaoWriteVisibility(g_outSvgfRaw, pixelCoords, visibility);
  if (CORTAO_debug_mode > 21.5f) {
    if ((int)CORTAO_debug_mode == 22) g_outDebug[pixelCoords] = float4(visibility.xxx, 1.0);
    else if ((int)CORTAO_debug_mode == 26) g_outDebug[pixelCoords] = float4((float3)tangentSpaceTransform[2] * 0.5 + 0.5, 1.0);
  }
}

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
  // cortao_mainPass returns before cortao_writeResult for sky pixels (CORTAOMainPass.h:374-377): pre-write full visibility.
  FalcomCortaoWriteVisibility(g_outSvgfRaw, pixelCoords, 1.0);
  if (CORTAO_debug_mode > 21.5f) g_outDebug[pixelCoords] = float4(0.25, 0.0, 0.25, 1.0);
  cortao_mainPass(pixelCoords, FalcomCortaoPopulateMainPassParameters());
}
