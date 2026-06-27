///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus XeGTAO integration — shared declarations
//
// XeGTAO is based on GTAO/GTSO
// "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion"
// https://github.com/GameTechDev/XeGTAO
//
// Kai-vanillaplus approach: bind game's scene CBV directly to b0,
// read proj_g from it, build GTAOConstants in-shader.
// User settings come via push_constants at b13.
//
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SRC_GAMES_SORA_VANILLAPLUS_XEGTAO_COMMON_HLSL_
#define SRC_GAMES_SORA_VANILLAPLUS_XEGTAO_COMMON_HLSL_

// Provide the VA_SATURATE macro that XeGTAO.hlsli expects.
#ifndef VA_SATURATE
#define VA_SATURATE(x) saturate(x)
#endif

// Dynamic settings from push_constants at b13.
#define XE_GTAO_USE_DEFAULT_CONSTANTS 0

// Use fp32 math for fxc / cs_5_0 compatibility.
#define XE_GTAO_USE_HALF_FLOAT_PRECISION 0

// Enable visibility bitmask AO (replaces GTAO horizon angles).
#define XE_GTAO_USE_BITMASK 1

// We do NOT compute bent normals (AO visibility only).
// #define XE_GTAO_COMPUTE_BENT_NORMALS

// GI is enabled per-shader-variant via XE_GTAO_COMPUTE_GI.
// This common header provides the bindings used when it is defined.

// ── Game's scene constant buffer (b0) ──
// Must be declared BEFORE XeGTAO.hlsli so XeGTAO_MainPass can reference its members.
cbuffer cb_scene : register(b0)
{
  float4x4 view_g          : packoffset(c0);
  float4x4 viewInv_g       : packoffset(c4);
  float4x4 proj_g          : packoffset(c8);
  float4x4 projInv_g       : packoffset(c12);
#ifdef RENODX_KAI
  float4x4 prevViewProj_g  : packoffset(c85);   // Kai: prevViewProj_g at c85
#else
  float4x4 prevViewProj_g  : packoffset(c74);   // Sora 1st: prevViewProj_g at c74
#endif
};

// ── User settings via push_constants (b13) ──
// Must be declared BEFORE XeGTAO.hlsli so XeGTAO_MainPass GI path can reference it.
cbuffer cb_xegtao : register(b13)
{
  float xegtao_quality;
  float xegtao_denoise_passes;
  float xegtao_radius;
  float xegtao_falloff_range;
  float xegtao_radius_multiplier;
  float xegtao_final_value_power;
  float xegtao_sample_distribution_power;
  float xegtao_thin_occluder_compensation;
  float xegtao_depth_mip_sampling_offset;
  float xegtao_denoise_blur_beta;
  float xegtao_noise_index;
  float xegtao_debug_mode;
  float xegtao_denoise_is_last_pass;
  float xegtao_normal_input_mode;
  float xegtao_mrt_normal_available;
  float xegtao_normal_influence;
  float xegtao_normal_depth_blend;
  float xegtao_normal_sharpness;
  float xegtao_normal_edge_rejection;
  float xegtao_normal_z_preservation;
  float xegtao_normal_detail_response;
  float xegtao_normal_max_darkening;
  float xegtao_normal_darkening_mode;
  float xegtao_normal_transform_mode;   // 0=view_g, 1=viewInv_g, 2=passthrough
  float xegtao_copyback_preserve_yzw;
  float g_gi_enabled;                    // c[25] — 0=off, 1=on
  float g_gi_light_exposure;             // c[26] — HDR light buffer exposure scale [0.001..10]
  float g_gi_power;                      // c[27] — GI power curve (fixed 1.5)
  float g_gi_intensity;                  // c[28] — GI intensity [0..5]
  float g_gi_saturation;                 // c[29] — GI saturation [0..2]
  float g_gi_multibounce;                // c[30] — multi-bounce enable (0/1)
  float g_gi_multibounce_strength;       // c[31] — feedback intensity [0..10]
  float g_gi_multibounce_saturation;     // c[32] — feedback color saturation [0..2]
  float g_gi_multibounce_max_clamp;      // c[33] — max multi-bounce per-channel (0=off)
  float g_ssgi_debug_view;               // c[34] — SSGI debug view mode
  float xegtao_isfast_enabled;           // c[34] — IS-FAST enable (0/1)
  float xegtao_isfast_strength;          // c[35] — IS-FAST noise strength [0..1]
  float xegtao_isfast_debug;             // c[36] — IS-FAST texture loaded flag (set by shader check)
  float xegtao_adaptive_mode;           // c[37] — 0=GI color, 1=albedo
  float xegtao_adaptive_luma_strength;  // c[38] — target luma (0=off)
  float xegtao_adaptive_luma_blend;     // c[39] — blend original↔normalized
  float xegtao_isfast_spatial_scale;    // c[40] — IS-FAST spatial scale [0.25..4]
  float xegtao_isfast_temporal_speed;   // c[41] — IS-FAST temporal speed [0..5]
  float xegtao_isfast_seed;             // c[42] — IS-FAST seed offset [0..64]
  float xegtao_denoise_leak_threshold;  // c[43] — edge leak threshold [1..4], default 2.5
  float xegtao_denoise_leak_strength;   // c[44] — edge leak strength [0..1], default 0.5
  float xegtao_denoiser_type;           // c[45] — 0=Spatial, 1=Spatio-Temporal
  float xegtao_temporal_blend;          // c[46] — history blend weight [0.5..0.95], default 0.85
  float xegtao_disocclusion_threshold;  // c[47] — depth diff to reject history [0.001..0.1], default 0.01
  float xegtao_noise_type;             // c[48] — 0=IS-FAST, 1=IGN, 2=Hilbert
};

// ── GI parameters are native cb_xegtao fields (c[25]-c[33]) — no aliases needed.
#define g_isfast_enabled        xegtao_isfast_enabled            // IS-FAST enable (0/1)
#define g_isfast_strength       xegtao_isfast_strength           // IS-FAST noise strength
#define g_isfast_texture_loaded xegtao_isfast_debug              // IS-FAST: 1=texture loaded
#define g_isfast_spatial_scale xegtao_isfast_spatial_scale        // IS-FAST spatial scale [0.25..4]
#define g_isfast_temporal_speed xegtao_isfast_temporal_speed      // IS-FAST temporal speed [0..5]
#define g_isfast_seed_offset   xegtao_isfast_seed                 // IS-FAST seed offset [0..64]
// Legacy adaptive macros — hardcoded to 0 (no effect), slots repurposed for IS-FAST
#define g_gi_adaptive_r         0.f
#define g_gi_adaptive_g         0.f
#define g_gi_adaptive_b         0.f
#define g_gi_adaptive_mode      xegtao_adaptive_mode             // 0=GI color, 1=albedo
#define g_gi_adaptive_luma_strength xegtao_adaptive_luma_strength // [0..5] target luma
#define g_gi_adaptive_luma_blend xegtao_adaptive_luma_blend       // [0..1] blend
// g_gi_power (c[27]) and g_gi_light_exposure (c[26]) are native fields.

// ── GI resources are passed as function parameters to XeGTAO_MainPass ──
// (avoids fxc X3003 redefinition errors from forward declarations).
// The wrapper .cs_5_0.hlsl files declare and pass them.

// ── Helper function prototypes (defined in wrapper .cs_5_0.hlsl files) ──
float3 DecodeMrtNormalAsIs(uint2 texel);
float3 TransformNormalToView(float3 decoded);

#include "XeGTAO.h"
#include "XeGTAO.hlsli"

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

  consts.EffectRadius = max(0.001, xegtao_radius);
  consts.EffectFalloffRange = saturate(xegtao_falloff_range);
  consts.RadiusMultiplier = max(0.3, xegtao_radius_multiplier);
  consts.FinalValuePower = max(0.5, xegtao_final_value_power);
  consts.DenoiseBlurBeta = max(0.01, xegtao_denoise_blur_beta);
  consts.SampleDistributionPower = max(1.0, xegtao_sample_distribution_power);
  consts.ThinOccluderCompensation = max(0.0, xegtao_thin_occluder_compensation);
  consts.DepthMIPSamplingOffset = max(0.0, xegtao_depth_mip_sampling_offset);
  consts.NoiseIndex = (int)xegtao_noise_index;
  consts.Padding0 = 0.0;

  return consts;
}

#endif  // SRC_GAMES_SORA_VANILLAPLUS_XEGTAO_COMMON_HLSL_
