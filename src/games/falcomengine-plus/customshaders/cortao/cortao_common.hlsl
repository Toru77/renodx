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
  float CORTAO_debug_mode;                    // c[16] 0, or 22 Raw AO, 23 Temporal Confidence (SVGF), 24 Filtered AO, 25 Filtered Bent Normal, 26 Input Normal, 28 Bent Normal
  float CORTAO_noise_type;                    // c[17] 0 = CORTAO hash (reference), 1 = Hilbert, 2 = IS-FAST (M6a)
  float CORTAO_noise_index;                   // c[18] temporal noise index: frame % 64 with Temporal Noise On, else 0 (M6a)
  float CORTAO_normal_influence;              // c[19] M12: MRT normal xy scale (1 = neutral)
  float CORTAO_normal_z_preservation;         // c[20] M12: MRT normal z scale, floored at 0.05 (1 = neutral)
  float CORTAO_normal_depth_blend;            // c[21] M12: MRT vs depth normal weight (1 = neutral)
  float CORTAO_normal_sharpness;              // c[22] M12: gamma on the depth blend, floored at 0.01 (1 = neutral)
  float CORTAO_normal_edge_rejection;         // c[23] M12: depth-edge rejection (0 = neutral)
  float CORTAO_normal_detail_response;        // c[24] M12: MRT/depth disagreement reweight (0 = neutral)
  float CORTAO_normal_max_mrt_weight;         // c[25] M12: cap on the final MRT weight (1 = neutral)
  float CORTAO_pad2;                          // c[26]
  float CORTAO_pad3;                          // c[27]
};

// M12: GTVBAO depth linearisation (BuildGTAOConstants DepthUnpackConsts, GTVBAO_ScreenSpaceToViewSpaceDepth)
// with GTVBAO_ClampDepth's 65504 branch (GT_VBAO_USE_HALF_FLOAT_PRECISION is defined as 0).
float FalcomCortaoGtvbaoDepth(Texture2D<float> depth, int2 pix) {
  uint w, h;
  depth.GetDimensions(w, h);
  const int2 maxPix = max(int2(w, h) - 1, int2(0, 0));
  const int2 t = clamp(pix, int2(0, 0), maxPix);
  float mul = -proj_g[3][2];
  float add = proj_g[2][2];
  if (mul * add < 0.0) add = -add;
  return clamp(mul / (add - depth.Load(int3(t, 0))), 0.0, 65504.0);
}

// M12: GTVBAO_ComputeViewspacePosition from the projection's NDCToViewMul/Add.
float3 FalcomCortaoGtvbaoViewPos(float2 uv, float z) {
  const float2 tanHalf = float2(1.0 / proj_g[0][0], 1.0 / proj_g[1][1]);
  return float3((float2(tanHalf.x * 2.0, tanHalf.y * -2.0) * uv + float2(-tanHalf.x, tanHalf.y)) * z, z);
}

// CORTAO view space is engine view_g space; no Z flip, unlike TransformNormalToView.
// CORTAO calls this only for non-sky pixels, so the game decode applies.
float3 FalcomCortaoViewNormal(Texture2D<uint4> mrt, Texture2D<float> depth, int2 pix)
{
  uint mw, mh;
  mrt.GetDimensions(mw, mh);
  const int2 t = GTVBAO_MrtTexel((float2(pix) + 0.5) * (float2(mw, mh) / max(float2(CORTAO_width, CORTAO_height), 1.0.xx)), float2(mw, mh));
  const float3 n = DecodeFalcomMrtNormalAsGame(mrt.Load(int3(t, 0)).xy);

  // M12 neutral fast path: all shaping values at their defaults reproduce the M11 line byte for byte.
  if (CORTAO_normal_influence == 1.0f
      && CORTAO_normal_z_preservation == 1.0f
      && CORTAO_normal_depth_blend >= 1.0f
      && CORTAO_normal_edge_rejection <= 0.0f
      && CORTAO_normal_detail_response <= 0.0f
      && CORTAO_normal_max_mrt_weight >= 1.0f)
    return FalcomSafeNormalize3(mul((float3x3)viewInv_g, n), float3(0.0, 0.0, 1.0));

  // GTVBAO's shaping runs in GTVBAO view space (z negated relative to engine view_g); mirror once on return.
  const bool mrtValid = FalcomNormalValid(n);
  float3 tuned = float3(0.0, 0.0, 0.0);
  if (mrtValid) {
    float3 mrtView = mul((float3x3)viewInv_g, n);
    mrtView.z = -mrtView.z;
    float3 mrtNormal = FalcomSafeNormalize3(mrtView, float3(0.0, 0.0, 0.0));
    tuned = mrtNormal;
    if (CORTAO_normal_influence != 1.0f || CORTAO_normal_z_preservation != 1.0f) {
      tuned.xy *= max(0.0, CORTAO_normal_influence);
      tuned.z  *= max(0.05, CORTAO_normal_z_preservation);
      tuned = FalcomSafeNormalize3(tuned, mrtNormal);
    }
    // Second fast path: no depth-derived term is active, so the tuned MRT normal is final.
    if (CORTAO_normal_depth_blend >= 1.0f
        && CORTAO_normal_edge_rejection <= 0.0f
        && CORTAO_normal_detail_response <= 0.0f
        && CORTAO_normal_max_mrt_weight >= 1.0f)
      return float3(tuned.xy, -tuned.z);
  }

  // GTVBAO_DepthNormalAt: five clamped depth loads, GTVBAO_CalculateEdges and GTVBAO_CalculateNormal.
  uint dw, dh;
  depth.GetDimensions(dw, dh);
  const float2 duv = 1.0 / max(float2(dw, dh), float2(1.0, 1.0));
  const float2 uC = (float2(pix) + 0.5) * duv;
  const float cZ = FalcomCortaoGtvbaoDepth(depth, pix);
  const float lZ = FalcomCortaoGtvbaoDepth(depth, pix + int2(-1,  0));
  const float rZ = FalcomCortaoGtvbaoDepth(depth, pix + int2( 1,  0));
  const float topZ = FalcomCortaoGtvbaoDepth(depth, pix + int2( 0, -1));
  const float botZ = FalcomCortaoGtvbaoDepth(depth, pix + int2( 0,  1));
  float3 C = FalcomCortaoGtvbaoViewPos(uC, cZ);
  float3 L = FalcomCortaoGtvbaoViewPos(uC + float2(-1, 0) * duv, lZ);
  float3 R = FalcomCortaoGtvbaoViewPos(uC + float2( 1, 0) * duv, rZ);
  float3 T = FalcomCortaoGtvbaoViewPos(uC + float2( 0,-1) * duv, topZ);
  float3 B = FalcomCortaoGtvbaoViewPos(uC + float2( 0, 1) * duv, botZ);

  float4 edgesLRTB = float4(lZ, rZ, topZ, botZ) - (float)cZ;
  float slopeLR = (edgesLRTB.y - edgesLRTB.x) * 0.5;
  float slopeTB = (edgesLRTB.w - edgesLRTB.z) * 0.5;
  float4 edgesLRTBSlopeAdjusted = edgesLRTB + float4(slopeLR, -slopeLR, slopeTB, -slopeTB);
  edgesLRTB = min(abs(edgesLRTB), abs(edgesLRTBSlopeAdjusted));
  edgesLRTB = saturate((1.25 - edgesLRTB / (cZ * 0.011)));

  float4 acceptedNormals = saturate(float4(edgesLRTB.x*edgesLRTB.z, edgesLRTB.z*edgesLRTB.y, edgesLRTB.y*edgesLRTB.w, edgesLRTB.w*edgesLRTB.x) + 0.01);
  L = normalize(L - C);
  R = normalize(R - C);
  T = normalize(T - C);
  B = normalize(B - C);
  float3 depthN = acceptedNormals.x * cross(L, T) +
                  + acceptedNormals.y * cross(T, R) +
                  + acceptedNormals.z * cross(R, B) +
                  + acceptedNormals.w * cross(B, L);
  depthN = normalize(depthN);

  if (!mrtValid) return float3(depthN.xy, -depthN.z);

  float edgeAtt = 1.0;
  if (CORTAO_normal_edge_rejection > 0.0) {
    const float delta = max(max(abs(lZ - cZ), abs(rZ - cZ)), max(abs(topZ - cZ), abs(botZ - cZ)));
    edgeAtt = 1.0 - saturate(saturate(delta * 1.0) * CORTAO_normal_edge_rejection);
  }

  float sharpness = max(0.01, CORTAO_normal_sharpness);
  float base_blend = pow(saturate(CORTAO_normal_depth_blend), 1.0 / sharpness);
  float normal_delta = 1.0 - saturate(dot(depthN, tuned));
  float detail_gain = lerp(1.0, lerp(0.35, 1.25, pow(normal_delta, 0.5)), saturate(CORTAO_normal_detail_response));
  float final_blend = saturate(base_blend * edgeAtt * detail_gain);
  final_blend *= saturate(CORTAO_normal_max_mrt_weight);

  const float3 g = FalcomSafeNormalize3(lerp(depthN, tuned, final_blend), depthN);
  return float3(g.xy, -g.z);
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
