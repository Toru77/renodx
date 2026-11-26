// Falcom Engine+ CORTAO filter pass (SM 5.0). SPDX-License-Identifier: MIT

#include "cortao_common.hlsl"

#define CORTAO_FILTER_BENT_NORMAL 0
#define CORTAO_FILTER_SYMMETRY 1

#include "../../reference/CORTAO-main/src/shaders/CORTAOFilterPass.h"

Texture2D<float> g_srcPacked : register(t0);       // R32_FLOAT packed data (SM 5 workaround)
Texture2D<float> g_srcOcclusion : register(t1);    // R8_UNORM visibility written by the main pass
Texture2D<uint> g_srcFoliageMask : register(t2);
SamplerState g_samplerPointBorder : register(s0);  // nearest, border 0 (README filter pass)
RWTexture2D<float4> g_outSvgfRaw : register(u0);

cortao_uint4 cortao_gatherPackedData(cortao_float2 texCoords, cortao_int2 offset) {
  return asuint(g_srcPacked.Gather(g_samplerPointBorder, texCoords, offset));
}
cortao_half4 cortao_gatherSSAO(cortao_float2 texCoords, cortao_int2 offset) {
  return g_srcOcclusion.Gather(g_samplerPointBorder, texCoords, offset);
}
void cortao_writeFilterOutput(cortao_int2 pixelCoords, cortao_half ssao) {
  FalcomCortaoWriteVisibility(g_outSvgfRaw, g_srcFoliageMask, pixelCoords, ssao);
}

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
