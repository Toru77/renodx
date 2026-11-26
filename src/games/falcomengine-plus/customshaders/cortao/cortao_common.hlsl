// CORTAO adaptation for Falcom Engine+ (D3D11 / fxc).
// Reference: reference/CORTAO-main/README.md
// SPDX-License-Identifier: MIT

#define CORTAO_HLSL 1
#define CORTAO_PERSPECTIVE_PROJECTION 1
#define CORTAO_16_BIT 0            // fxc has no 16-bit types (README "HLSL specific information")
#define CORTAO_PACKED_NORMAL_BITS 10 // same in every pass (README prepass defines)

#include "../../include/mrt_normal.hlsli"

cbuffer cb_scene : register(b0)
{
  float4x4 view_g : packoffset(c0);
  float4x4 viewInv_g : packoffset(c4);
  float4x4 proj_g : packoffset(c8);
  float4x4 projInv_g : packoffset(c12);
};

// Mirrors CortaoPushConstants in addon.cpp; both blocks must stay in lockstep.
cbuffer cb_cortao : register(b13)
{
  uint  CORTAO_width;                         // c[0]  CORTAOConfigurationParameters::width
  uint  CORTAO_height;                        // c[1]  ::height
  uint  CORTAO_mip_count;                     // c[2]  ::mipCount
  uint  CORTAO_random_seed;                   // c[3]  ::randomSeed
  float CORTAO_max_radius;                    // c[4]  ::maxRadius
  float CORTAO_max_screenspace_radius_height_ratio; // c[5]
  float CORTAO_mip_bias;                      // c[6]  ::mipBias
  float CORTAO_max_slope_compensation;        // c[7]
  float CORTAO_backface_threshold;            // c[8]
  float CORTAO_backface_boost;                // c[9]
  float CORTAO_filter_depth_sensitivity;      // c[10]
  float CORTAO_filter_normal_sensitivity;     // c[11]
  float CORTAO_intensity;                     // c[12] README non-photorealistic pow()
  float CORTAO_filter_enabled;                // c[13] 1 = the filter pass runs this frame (M3)
  float CORTAO_foliage_exclude;               // c[14] 1 = foliage mask fresh and exclusion on (M2)
  float CORTAO_foliage_ao_value;              // c[15] (M2)
  float CORTAO_debug_mode;                    // c[16] 0, or 22 Raw AO, 26 Input Normal, 28 Bent Normal (M4)
  float CORTAO_pad0;                          // c[17] pads the block to a full register
  float CORTAO_pad1;                          // c[18]
  float CORTAO_pad2;                          // c[19]
};

// CORTAO view space is engine view_g space; no Z flip, unlike TransformNormalToView.
// CORTAO calls this only for non-sky pixels, so the game decode applies.
float3 FalcomCortaoViewNormal(Texture2D<uint4> mrt, int2 pix)
{
  uint mw, mh;
  mrt.GetDimensions(mw, mh);
  const int2 t = GTVBAO_MrtTexel((float2(pix) + 0.5) * (float2(mw, mh) / max(float2(CORTAO_width, CORTAO_height), 1.0.xx)), float2(mw, mh));
  const float3 n = DecodeFalcomMrtNormalAsGame(mrt.Load(int3(t, 0)).xy);
  return FalcomSafeNormalize3(mul((float3x3)viewInv_g, n), float3(0.0, 0.0, 1.0));
}

// SVGF raw layout, no GTVBAO ×1.5 byte scale.
void FalcomCortaoWriteVisibility(RWTexture2D<float4> outRaw, Texture2D<uint> foliageMask, uint2 p, float visibility)
{
  // GTVBAO parity (GTVBAO.hlsli:746-752): blend foliage receivers toward full visibility.
  if (CORTAO_foliage_exclude > 0.5f && foliageMask.Load(int3(p, 0)) != 0u)
    visibility = lerp(visibility, 1.0, 1.0 - saturate(CORTAO_foliage_ao_value));
  outRaw[p] = float4(visibility, visibility, visibility, 1.0);
}

// Writes the scalar visibility (SVGF raw) and the reference bent-normal output (occlusion, bent.xyz),
// both with the same foliage-blended visibility.
void FalcomCortaoWriteOutputs(RWTexture2D<float4> outRaw, RWTexture2D<float4> outBent, Texture2D<uint> foliageMask, uint2 p, float4 ssao) {
  float visibility = ssao.x;
  if (CORTAO_foliage_exclude > 0.5f && foliageMask.Load(int3(p, 0)) != 0u)
    visibility = lerp(visibility, 1.0, 1.0 - saturate(CORTAO_foliage_ao_value));
  outRaw[p] = float4(visibility, visibility, visibility, 1.0);
  outBent[p] = float4(visibility, ssao.yzw);
}
