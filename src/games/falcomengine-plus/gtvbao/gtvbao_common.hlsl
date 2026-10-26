///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO integration — shared declarations
//
// GTVBAO is based on GTAO/GTSO
// "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion"
// https://github.com/GameTechDev/GTVBAO
//
// Kai-vanillaplus approach: bind game's scene CBV directly to b0,
// read proj_g from it, build GTAOConstants in-shader.
// User settings come via push_constants at b13.
//
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SRC_GAMES_SORA_VANILLAPLUS_GTVBAO_COMMON_HLSL_
#define SRC_GAMES_SORA_VANILLAPLUS_GTVBAO_COMMON_HLSL_

// Use fp32 math for fxc / cs_5_0 compatibility.
#define GT_VBAO_USE_HALF_FLOAT_PRECISION 0

// Floor for the "normal Z preservation" tuning weight. Scaling n.z to exactly 0
// discards the only component that distinguishes an up-facing surface from a
// down-facing one, and renormalizing the remaining XY turns the normal into a
// horizontal vector for every pixel. Clamp the weight instead of the result so
// the knob stays usable without ever producing a degenerate normal.
#define GT_VBAO_MIN_NORMAL_Z_SCALE 0.05

// Falcom MRT world-normal decode, shared with the shadow compute passes.
// Must come first: the helpers below and GTVBAO.hlsli both call it.
#include "../include/mrt_normal.hlsli"

// ── Game's scene constant buffer (b0) ──
// Must be declared BEFORE GTVBAO.hlsli so GTVBAO_MainPass can reference its members.
cbuffer cb_scene : register(b0)
{
  float4x4 view_g          : packoffset(c0);
  float4x4 viewInv_g       : packoffset(c4);
  float4x4 proj_g          : packoffset(c8);
  float4x4 projInv_g       : packoffset(c12);
  float4x4 viewProjInv_g   : packoffset(c20);
};

// ── User settings via push_constants (b13) ──
// Must be declared BEFORE GTVBAO.hlsli so GTVBAO_MainPass can reference it.
cbuffer cb_gtvbao : register(b13)
{
  float GTVBAO_quality;                  // c[00] - 0=Low 2x4, 1=Medium 3x4, 2=High 3x6, 3=Ultra 4x10
  float GTVBAO_radius;                   // c[01]
  float GTVBAO_radius_multiplier;        // c[02]
  float GTVBAO_final_value_power;        // c[03]
  float GTVBAO_sample_distribution_power;// c[04]
  float GTVBAO_bitmask_thickness;        // c[05]
  float GTVBAO_depth_mip_sampling_offset;// c[06]
  // The c[NN] tags below are 0-based indices into the array built by
  // BuildGTVBAOPushConstants() in addon.cpp; the two lists must stay in lockstep.
  float GTVBAO_noise_index;              // c[07] - frame % 64
  float GTVBAO_debug_mode;               // c[08]
  float GTVBAO_final_pass;               // c[09]
  float GTVBAO_normal_input_mode;        // c[10] - 0=depth-derived, 1=MRT g-buffer
  float GTVBAO_mrt_normal_available;     // c[11]
  float GTVBAO_normal_influence;         // c[12]
  float GTVBAO_normal_depth_blend;       // c[13]
  float GTVBAO_normal_sharpness;         // c[14]
  float GTVBAO_normal_edge_rejection;    // c[15]
  float GTVBAO_normal_z_preservation;    // c[16]
  float GTVBAO_normal_detail_response;   // c[17]
  float GTVBAO_normal_max_darkening;     // c[18]
  float GTVBAO_isfast_enabled;           // c[19] - IS-FAST enable (0/1)
  float GTVBAO_isfast_strength;          // c[20] - IS-FAST noise strength [0..1]
  float GTVBAO_isfast_debug;             // c[21] - IS-FAST texture loaded flag (set by shader check)
  float GTVBAO_isfast_spatial_scale;     // c[22] - IS-FAST spatial scale [0.25..4]
  float GTVBAO_isfast_temporal_speed;    // c[23] - IS-FAST temporal speed [0..5]
  float GTVBAO_isfast_seed;              // c[24] - IS-FAST seed offset [0..64]
  float GTVBAO_noise_type;               // c[25] - 0=IS-FAST, 1=IGN, 2=Hilbert
  float GTVBAO_gtvbao_cosine_mode;       // c[26] - 0=Weight, 1=Project, 2=CDF
  // - Foliage -
  float GTVBAO_exclude_foliage;          // c[27] - 0=Off, 1=On - skip AO on foliage
  float GTVBAO_foliage_ao_value;         // c[28] - [0..1] - AO value for excluded foliage
  float GTVBAO_foliage_channel_mode;     // c[29] - 0=o1.w (Sora), 1=o1.z (Kai)
  float GTVBAO_foliage_mask_valid;       // c[30] - 1 = mask pre-pass ran this frame (mask is fresh)
  // - spatial filter -
  float GTVBAO_filter_kind;              // c[31] - 0=Separable bilateral, 1=A-trous
  float GTVBAO_filter_radius;            // c[32] - pixel radius
  float GTVBAO_filter_taps;              // c[33] - taps per axis (separable) or square kernel side (a-trous)
  float GTVBAO_filter_step;              // c[34] - a-trous stride for this dispatch (1, 2, 4)
  // - Low-resolution spatial pipeline (appended; Full path ignores these) -
  float GTVBAO_resolution;               // c[35] - 0=Full, 1=Half, 2=Quarter
  float GTVBAO_hemisphere_weighting;     // c[36] - 0=Cosine, 1=Uniform
  // - Spatial filter direction and temporal accumulation (appended) -
  float GTVBAO_filter_direction;         // c[37] - separable pass: 0=Horizontal, 1=Vertical, 2=Both (a-trous)
  float GTVBAO_temporal_history_weight;  // c[38] - maximum blend toward validated history
  float GTVBAO_temporal_depth_rejection; // c[39] - relative depth difference allowed between history and current
  float GTVBAO_temporal_normal_rejection;// c[40] - normal dot product below which history taps fade out
  float GTVBAO_temporal_history_clamp;   // c[41] - 3x3 neighbourhood clamp, k sigma (0 = off)
  float GTVBAO_temporal_history_valid;   // c[42] - 1 when the history may be used (reset == None)
};

// ── Low-res → full-res block-center mapping (odd-dimension safe) ──
// AO texel tq covers the full block {d*tq .. d*tq + d-1} (d = 1 Full, 2 Half, 4 Quarter); its representative
// pixel is the block centre, clamped for odd full dims (e.g. full=5, d=2, ao=3: tq=2 -> min(5,4)=4).
float GTVBAO_Divisor() {
  return exp2(round(clamp(GTVBAO_resolution, 0.f, 2.f)));
}

// Debug view modes (GTVBAO_debug_mode). 27 is reserved.
#define GTVBAO_DEBUG_RAW_AO 22
#define GTVBAO_DEBUG_TEMPORAL 23
#define GTVBAO_DEBUG_SLICE_AO 24
#define GTVBAO_DEBUG_BITMASK 25
#define GTVBAO_DEBUG_NORMAL 26

// Writes a debug value into every full-res texel of the d x d block that AO pixel pixCoord covers.
void GTVBAO_WriteDebug(RWTexture2D<float4> outDebug, uint2 pixCoord, float4 value) {
  uint dw, dh;
  outDebug.GetDimensions(dw, dh);
  const uint d = (uint)GTVBAO_Divisor();
  const uint2 base = pixCoord * d;
  for (uint by = 0; by < d; ++by) {
    for (uint bx = 0; bx < d; ++bx) {
      const uint2 tc = base + uint2(bx, by);
      if (tc.x < dw && tc.y < dh) outDebug[tc] = value;
    }
  }
}

// AO texel -> representative full-res pixel (block centre, odd-safe).
int2 GTVBAO_RepresentativePixel(int2 tc, int2 fullDims) {
  const int d = (int)GTVBAO_Divisor();
  return min(tc * d + d / 2, max(fullDims - 1, int2(0, 0)));
}

#define g_isfast_enabled        GTVBAO_isfast_enabled            // IS-FAST enable (0/1)
#define g_isfast_strength       GTVBAO_isfast_strength           // IS-FAST noise strength
#define g_isfast_texture_loaded GTVBAO_isfast_debug              // IS-FAST: 1=texture loaded
#define g_isfast_spatial_scale GTVBAO_isfast_spatial_scale        // IS-FAST spatial scale [0.25..4]
#define g_isfast_temporal_speed GTVBAO_isfast_temporal_speed      // IS-FAST temporal speed [0..5]
#define g_isfast_seed_offset   GTVBAO_isfast_seed                 // IS-FAST seed offset [0..64]

// ── MRT normal helpers ──
// The encode/decode itself lives in include/mrt_normal.hlsli, shared with the
// shadow compute passes so the two features cannot disagree about what a texel
// means. Only the GTVBAO-specific reshaping stays here, so every pass that
// touches an MRT normal (main passes, normal_prep, upscale, à-trous) shares one
// definition and cannot drift.
// DecodeMrtNormalAsIs stays per-wrapper only as a thin binding shim: it reads
// that pass's own `g_srcMrtNormal`, which is a per-pass resource.

// Texel address for a screen UV in a source whose resolution differs from the
// AO working domain. Every MRT normal reader must land on
// the SAME texel for the same UV, or the two read different normals for one
// pixel.
//
// The caller passes the already-scaled texel-space coordinate (i.e.
// `uv * dims`), NOT a normalized UV, and that scaling is done ONCE by the
// caller. Computing `(pix + 0.5) / work * dims` here instead would round twice
// and can land just under an integer boundary, e.g. half-res 960 -> full 1920
// at pix 252 gives 504.99999999999994 and floors to 504 where the old
// `floor((252 + 0.5) * 2)` correctly gave 505. Keep the arithmetic in the
// form the main pass used to keep the existing mapping bit-identical.
int2 GTVBAO_MrtTexel(float2 uvTexelSpace, float2 mrtDims)
{
  int2 d = max(int2(mrtDims), int2(1, 1));
  return clamp(int2(floor(uvTexelSpace)), int2(0, 0), d - 1);
}

// ── Foliage mark test (bit 15 of the MRT normal's Sora .w / Kai .z) ──
// Full mode (d == 1) tests the single texel the work->MRT ratio selects, which is the
// exact expression the main pass always used. Low-resolution modes (d = 2, 4) test the whole
// full-res footprint of the AO texel: the {d*t .. d*t+d-1} block, mapped through
// the full->MRT ratio. A single representative texel is insufficient because
// alpha-tested foliage marks only the leaf texels, so a block's other texels can
// hold the mark the representative misses.
bool GTVBAO_FoliageMarked(Texture2D<uint4> mrtNormal, uint2 workTC, uint2 workDims,
                          uint2 fullDims, uint2 mrtDims, bool useZ, uint d)
{
  if (d == 1u)
  {
    const float2 scale = float2(mrtDims) / max(float2(workDims), 1.0.xx);
    uint4 mrtV = mrtNormal.Load(int3(GTVBAO_MrtTexel((float2(workTC) + 0.5) * scale, float2(mrtDims)), 0));
    uint mrtC = useZ ? mrtV.z : mrtV.w;
    return (mrtC & 0x8000u) != 0u;
  }

  const float2 scaleFull = float2(mrtDims) / max(float2(fullDims), 1.0.xx);
  const int2 blockBase = int2(workTC * d);
  [loop]
  for (int by = 0; by < (int)d; ++by)
  {
    [loop]
    for (int bx = 0; bx < (int)d; ++bx)
    {
      uint4 mrtV = mrtNormal.Load(int3(
          GTVBAO_MrtTexel((float2(blockBase + int2(bx, by)) + 0.5) * scaleFull, float2(mrtDims)), 0));
      uint mrtC = useZ ? mrtV.z : mrtV.w;
      if ((mrtC & 0x8000u) != 0u) return true;
    }
  }
  return false;
}

// The normal-tuning shaping that BuildSelectedInputNormal applies, extracted so
// every MRT normal reader applies the same shaping. At the neutral defaults
// (influence 1, z_preservation 1) this is the identity, so it does not change
// current output.
//
// `viewNormal` must already be in GTVBAO view space (i.e. post
// TransformNormalToView). `influence` and `zPreservation` are passed in rather
// than read from the AO constants so callers can apply their own values
// through the same code. Returns a unit vector.
float3 GTVBAO_TuneNormal(float3 viewNormal, float influence, float zPreservation)
{
  // Neutral scaling leaves the vector unchanged, and the input is already unit
  // length (TransformNormalToView normalizes), so the multiply + renormalize
  // would be a no-op. Any real scaling still goes through the full path below.
  if (influence == 1.0f && zPreservation == 1.0f)
    return viewNormal;

  float3 tuned = viewNormal;
  tuned.xy *= max(0.0, influence);
  tuned.z  *= max(GT_VBAO_MIN_NORMAL_Z_SCALE, zPreservation);
  return FalcomSafeNormalize3(tuned, viewNormal);
}

// GTVBAO's own view space is +Z forward: GTVBAO_ComputeViewspacePosition
// returns ret.z = viewspaceDepth (positive, increasing with distance) and
// viewVec = normalize(-pixCenterPos) points back at the camera, so a
// camera-facing normal has NEGATIVE z there.
//
// The engine's view_g is -Z forward (D3D right-handed). Proof: the vanilla
// lighting shader negates the view-space z to build a fog coordinate --
//   sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl:379,381
//     r1.w = dot(view_g._m02_m12_m22_m32, r4.xyzw);  // view-space z
//     r3.z = -r1.w / volumeCameraFarClip_g;          // negated
// An already-positive view z would make that fog coordinate negative.
//
// So the two spaces are Z-mirrored relative to each other and the transform
// must negate Z. Without it, NdotV = saturate(dot(N, viewVec)) collapses
// toward 0 (GTVBAO.hlsli:516), the horizon search treats the surface as
// edge-on, and AO reports occlusion that tracks the per-frame noise rotation
// instead of the geometry.
//
// X and Y already agree: GTVBAO's NDCToViewMul maps uv.x to +right and uv.y to
// +up, matching the engine's world-up convention.
//
// This is the former mode 4 (viewInv_g plus flip Z), the shipped default.
float3 TransformNormalToView(float3 decoded)
{
  if (!FalcomNormalValid(decoded)) return float3(0.0, 0.0, 0.0);

  float3 vn = mul((float3x3)viewInv_g, decoded);
  vn.z = -vn.z;
  return FalcomSafeNormalize3(vn, float3(0.0, 0.0, 0.0));
}

float3 DecodeMrtNormalAsIs(uint2 texel);

#include "GTVBAO.h"
#include "GTVBAO.hlsli"

// World-space (view) position and normal at full-res pixel px; false for sky or an invalid normal.
bool GTVBAO_SurfaceAt(Texture2D<float> depth, Texture2D<float4> normals, int2 px, uint2 fullSize,
                      GTAOConstants consts, out float3 P, out float3 N)
{
  const float z = depth.Load(int3(px, 0));
  N = normals.Load(int3(px, 0)).xyz;
  const float2 uv = (float2(px) + 0.5) / float2(fullSize);
  P = GTVBAO_ComputeViewspacePosition(uv, z, consts);
  return z > 0.0 && FalcomNormalValid(N);
}

// World-space (view) position and normal of AO texel tq at its representative pixel.
bool GTVBAO_RepresentativeSurface(Texture2D<float> depth, Texture2D<float4> normals, int2 tq, uint2 fullSize,
                                  GTAOConstants consts, out float3 P, out float3 N)
{
  return GTVBAO_SurfaceAt(depth, normals, GTVBAO_RepresentativePixel(tq, int2(fullSize)), fullSize, consts, P, N);
}

// Depth-derived view-space normal at full-res pixel px: block-centre cross of the depth texture (clamped).
float3 GTVBAO_DepthNormalAt(Texture2D<lpfloat> depth, int2 px, uint2 full_size, GTAOConstants consts)
{
  const int2 fullDims = int2(max(full_size, 1u));
  const int2 fmax = max(fullDims - 1, int2(0, 0));
  const float2 fullPx = 1.0 / max(float2(fullDims), float2(1, 1));
  const float2 uC = (float2(px) + 0.5) * fullPx;
  const float z = depth.Load(int3(px, 0));
  const float l = depth.Load(int3(clamp(px + int2(-1, 0), int2(0, 0), fmax), 0));
  const float r = depth.Load(int3(clamp(px + int2( 1, 0), int2(0, 0), fmax), 0));
  const float t = depth.Load(int3(clamp(px + int2( 0,-1), int2(0, 0), fmax), 0));
  const float b = depth.Load(int3(clamp(px + int2( 0, 1), int2(0, 0), fmax), 0));
  float3 C = GTVBAO_ComputeViewspacePosition(uC, z, consts);
  float3 L = GTVBAO_ComputeViewspacePosition(uC + float2(-1, 0) * fullPx, l, consts);
  float3 R = GTVBAO_ComputeViewspacePosition(uC + float2( 1, 0) * fullPx, r, consts);
  float3 T = GTVBAO_ComputeViewspacePosition(uC + float2( 0,-1) * fullPx, t, consts);
  float3 B = GTVBAO_ComputeViewspacePosition(uC + float2( 0, 1) * fullPx, b, consts);
  return (float3)GTVBAO_CalculateNormal(
      GTVBAO_CalculateEdges((lpfloat)z, (lpfloat)l, (lpfloat)r, (lpfloat)t, (lpfloat)b),
      C, L, R, T, B);
}

// ── Build GTAOConstants from scene CB + push constants ──
GTAOConstants BuildGTAOConstants(uint2 viewport_size)
{
  GTAOConstants consts = (GTAOConstants)0;

  const float2 viewport_size_f = max(float2(viewport_size), 1.0.xx);
  consts.ViewportSize = int2(viewport_size);
  consts.ViewportPixelSize = 1.0.xx / viewport_size_f;

  float depth_linearize_mul = -proj_g[3][2];
  float depth_linearize_add = proj_g[2][2];
  if (depth_linearize_mul * depth_linearize_add < 0.0)
  {
    depth_linearize_add = -depth_linearize_add;
  }
  consts.DepthUnpackConsts = float2(depth_linearize_mul, depth_linearize_add);

  const float tan_half_fov_y = 1.0 / proj_g[1][1];
  const float tan_half_fov_x = 1.0 / proj_g[0][0];
  consts.CameraTanHalfFOV = float2(tan_half_fov_x, tan_half_fov_y);

  consts.NDCToViewMul = float2(consts.CameraTanHalfFOV.x * 2.0, consts.CameraTanHalfFOV.y * -2.0);
  consts.NDCToViewAdd = float2(-consts.CameraTanHalfFOV.x, consts.CameraTanHalfFOV.y);
  consts.NDCToViewMul_x_PixelSize = consts.NDCToViewMul * consts.ViewportPixelSize;

  consts.EffectRadius = max(0.001, GTVBAO_radius);
  consts.RadiusMultiplier = max(0.3, GTVBAO_radius_multiplier);
  consts.FinalValuePower = max(0.5, GTVBAO_final_value_power);
  consts.SampleDistributionPower = max(1.0, GTVBAO_sample_distribution_power);
  consts.BitmaskThickness = max(0.0, GTVBAO_bitmask_thickness);
  consts.DepthMIPSamplingOffset = max(0.0, GTVBAO_depth_mip_sampling_offset);
  consts.NoiseIndex = (int)GTVBAO_noise_index;

  return consts;
}

#endif  // SRC_GAMES_SORA_VANILLAPLUS_GTVBAO_COMMON_HLSL_
