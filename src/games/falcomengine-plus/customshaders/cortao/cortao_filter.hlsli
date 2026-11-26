// Falcom Engine+ CORTAO filter pass (SM 5.0). SPDX-License-Identifier: MIT

#include "cortao_common.hlsl"

#ifndef FALCOM_CORTAO_BENT_NORMALS
#define FALCOM_CORTAO_BENT_NORMALS 0
#endif
#define CORTAO_FILTER_BENT_NORMAL FALCOM_CORTAO_BENT_NORMALS
#define CORTAO_FILTER_SYMMETRY 1

#include "../../reference/CORTAO-main/src/shaders/CORTAOFilterPass.h"

Texture2D<float> g_srcPacked : register(t0);       // R32_FLOAT packed data (SM 5 workaround)
#if FALCOM_CORTAO_BENT_NORMALS
Texture2D<float4> g_srcOcclusion : register(t1);   // R8G8B8A8_SNORM (occlusion, bent)
#else
Texture2D<float> g_srcOcclusion : register(t1);    // R8_UNORM visibility written by the main pass
#endif
Texture2D<uint> g_srcFoliageMask : register(t2);
SamplerState g_samplerPointBorder : register(s0);  // nearest, border 0 (README filter pass)
RWTexture2D<float4> g_outSvgfRaw : register(u0);
#if FALCOM_CORTAO_BENT_NORMALS
RWTexture2D<float4> g_outBentNormal : register(u1);
#endif
RWTexture2D<float4> g_outDebug : register(u2);        // M6d debug views 24 / 25 (main pass and filter share the debug UAV)

cortao_uint4 cortao_gatherPackedData(cortao_float2 texCoords, cortao_int2 offset) {
  return asuint(g_srcPacked.Gather(g_samplerPointBorder, texCoords, offset));
}
#if FALCOM_CORTAO_BENT_NORMALS
// README SM 6.7 workaround (line 1107): non-packed R8G8B8A8_SNORM, one Gather per channel.
cortao_half4x4 cortao_gatherSSAO(cortao_float2 texCoords, cortao_int2 offset) {
  const float4 r = g_srcOcclusion.GatherRed(g_samplerPointBorder, texCoords, offset);
  const float4 g = g_srcOcclusion.GatherGreen(g_samplerPointBorder, texCoords, offset);
  const float4 b = g_srcOcclusion.GatherBlue(g_samplerPointBorder, texCoords, offset);
  const float4 a = g_srcOcclusion.GatherAlpha(g_samplerPointBorder, texCoords, offset);
  // Row i = texel i, the layout of the reference cortao_half4x4(unpackSnorm4x8(packed[0..3])).
  return cortao_half4x4(float4(r.x, g.x, b.x, a.x), float4(r.y, g.y, b.y, a.y),
                        float4(r.z, g.z, b.z, a.z), float4(r.w, g.w, b.w, a.w));
}
void cortao_writeFilterOutput(cortao_int2 pixelCoords, cortao_half4 ssao) {
  FalcomCortaoWriteOutputs(g_outSvgfRaw, g_outBentNormal, g_srcFoliageMask, pixelCoords, (float4)ssao);
  if ((int)CORTAO_debug_mode == 24) g_outDebug[pixelCoords] = float4(((float)ssao.x).xxx, 1.0);
  else if ((int)CORTAO_debug_mode == 25)
    g_outDebug[pixelCoords] = float4(FalcomSafeNormalize3((float3)ssao.yzw, float3(0.0, 0.0, 1.0)) * 0.5 + 0.5, 1.0);
}
#else
cortao_half4 cortao_gatherSSAO(cortao_float2 texCoords, cortao_int2 offset) {
  return g_srcOcclusion.Gather(g_samplerPointBorder, texCoords, offset);
}
void cortao_writeFilterOutput(cortao_int2 pixelCoords, cortao_half ssao) {
  FalcomCortaoWriteVisibility(g_outSvgfRaw, g_srcFoliageMask, pixelCoords, ssao);
  if ((int)CORTAO_debug_mode == 24) g_outDebug[pixelCoords] = float4(((float)ssao).xxx, 1.0);
}
#endif

// Transcription of CORTAO::populateFilterPassParameters.
CORTAOFilterPassParameters FalcomCortaoPopulateFilterPassParameters() {
  CORTAOFilterPassParameters p = (CORTAOFilterPassParameters)0;
  p.packedResolution = CORTAO_width | (CORTAO_height << 16u);
  p.depthSensitivity = CORTAO_filter_depth_sensitivity;
  p.normalSensitivity = CORTAO_filter_normal_sensitivity;
  return p;
}

[numthreads(8, 8, 1)]
void main(uint2 threadID : SV_DispatchThreadID) {
  cortao_filterPass(threadID, FalcomCortaoPopulateFilterPassParameters());
}
