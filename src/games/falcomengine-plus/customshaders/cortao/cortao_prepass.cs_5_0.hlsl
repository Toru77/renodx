// Falcom Engine+ CORTAO prepass (SM 5.0). SPDX-License-Identifier: MIT

#include "cortao_common.hlsl"
#include "../../reference/CORTAO-main/src/shaders/CORTAOPrepass.h"

Texture2D<float> g_srcGameDepth : register(t0);
Texture2D<uint4> g_srcMrtNormal : register(t1);

// The reference's RWTexture2D packedDataBufferMipmaps[8] UAV array needs SM 5.1;
// fxc/SM 5.0 uses eight separate UAVs, one R32_FLOAT mip each (README line 1105).
RWTexture2D<float> g_outPackedMip0 : register(u0);
RWTexture2D<float> g_outPackedMip1 : register(u1);
RWTexture2D<float> g_outPackedMip2 : register(u2);
RWTexture2D<float> g_outPackedMip3 : register(u3);
RWTexture2D<float> g_outPackedMip4 : register(u4);
RWTexture2D<float> g_outPackedMip5 : register(u5);
RWTexture2D<float> g_outPackedMip6 : register(u6);
RWTexture2D<float> g_outPackedMip7 : register(u7);

cortao_float cortao_loadDepth(cortao_int2 pixelCoords) { return g_srcGameDepth.Load(int3(pixelCoords, 0)); }
cortao_float3 cortao_loadNormal(cortao_int2 pixelCoords) { return FalcomCortaoViewNormal(g_srcMrtNormal, g_srcGameDepth, pixelCoords); }
void cortao_writePackedData(cortao_int2 pixelCoords, cortao_int mipLevel, cortao_uint data) {
  const float v = asfloat(data);
  if (mipLevel == 0) g_outPackedMip0[pixelCoords] = v;
  else if (mipLevel == 1) g_outPackedMip1[pixelCoords] = v;
  else if (mipLevel == 2) g_outPackedMip2[pixelCoords] = v;
  else if (mipLevel == 3) g_outPackedMip3[pixelCoords] = v;
  else if (mipLevel == 4) g_outPackedMip4[pixelCoords] = v;
  else if (mipLevel == 5) g_outPackedMip5[pixelCoords] = v;
  else if (mipLevel == 6) g_outPackedMip6[pixelCoords] = v;
  else g_outPackedMip7[pixelCoords] = v;
}

// Transcription of CORTAO::populatePrepassParameters (src/CORTAOParameters.h), isPerspective = true.
CORTAOPrepassParameters FalcomCortaoPopulatePrepassParameters() {
  // Engine projection to CORTAO planes, using GTVBAO's validated linearisation L = mul / (add − d).
  float mul = -proj_g[3][2];
  float add = proj_g[2][2];
  if (mul * add < 0.0) add = -add;
  // CORTAO nearPlane = distance at depth 0, farPlane = distance at depth 1 (README nearPlane/farPlane).
  const float inf = asfloat(0x7f800000u);
  const float nearPlane = (add == 0.0) ? inf : mul / add;
  const float farPlane = ((add - 1.0) == 0.0) ? inf : mul / (add - 1.0);

  CORTAOPrepassParameters p = (CORTAOPrepassParameters)0;
  p.packedResolution = CORTAO_width | (CORTAO_height << 16u);
  if (isinf(nearPlane)) {
    p.depthScale = 1.0 / farPlane;
    p.depthOffset = 0.0;
  } else if (isinf(farPlane)) {
    p.depthScale = -1.0 / nearPlane;
    p.depthOffset = 1.0 / nearPlane;
  } else {
    p.depthScale = (nearPlane - farPlane) / (nearPlane * farPlane);
    p.depthOffset = 1.0 / nearPlane;
  }
  p.randomSeed = CORTAO_random_seed;
  p.mipCount = CORTAO_mip_count;
  return p;
}

[numthreads(8, 8, 1)]
void main(uint2 pixelCoords : SV_DispatchThreadID) { cortao_prepass(pixelCoords, FalcomCortaoPopulatePrepassParameters()); }
