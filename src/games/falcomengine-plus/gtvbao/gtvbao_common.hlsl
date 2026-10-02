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

// Provide the VA_SATURATE macro that GTVBAO.hlsli expects.
#ifndef VA_SATURATE
#define VA_SATURATE(x) saturate(x)
#endif

// Dynamic settings from push_constants at b13.
#define GT_VBAO_USE_DEFAULT_CONSTANTS 0

// Use fp32 math for fxc / cs_5_0 compatibility.
#define GT_VBAO_USE_HALF_FLOAT_PRECISION 0

// Enable visibility bitmask AO (replaces GTAO horizon angles).
#define GT_VBAO_USE_BITMASK 1

// Floor for the "normal Z preservation" tuning weight. Scaling n.z to exactly 0
// discards the only component that distinguishes an up-facing surface from a
// down-facing one, and renormalizing the remaining XY turns the normal into a
// horizontal vector for every pixel. Clamp the weight instead of the result so
// the knob stays usable without ever producing a degenerate normal.
#define GT_VBAO_MIN_NORMAL_Z_SCALE 0.05

// We do NOT compute bent normals (AO visibility only).
// #define GT_VBAO_COMPUTE_BENT_NORMALS

// GI is enabled per-shader-variant via GT_VBAO_COMPUTE_GI.
// This common header provides the bindings used when it is defined.

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
#ifdef RENODX_KAI
  float4x4 prevViewProj_g  : packoffset(c85);   // Kai: prevViewProj_g at c85
#elif defined(RENODX_SORA2ND)
  float4x4 prevViewProj_g  : packoffset(c75);   // Sora 2nd: prevViewProj_g at c75
#else
  float4x4 prevViewProj_g  : packoffset(c74);   // Sora 1st: prevViewProj_g at c74
#endif
};

// ── User settings via push_constants (b13) ──
// Must be declared BEFORE GTVBAO.hlsli so GTVBAO_MainPass GI path can reference it.
cbuffer cb_gtvbao : register(b13)
{
  float GTVBAO_quality;
  float GTVBAO_denoise_passes;
  float GTVBAO_radius;
  float GTVBAO_falloff_range;
  float GTVBAO_radius_multiplier;
  float GTVBAO_final_value_power;
  float GTVBAO_sample_distribution_power;
  float GTVBAO_thin_occluder_compensation;
  float GTVBAO_depth_mip_sampling_offset;
  float GTVBAO_denoise_blur_beta;
  float GTVBAO_noise_index;
  float GTVBAO_debug_mode;
  float GTVBAO_denoise_is_last_pass;
  // The c[NN] tags below are 0-based indices into the array built by
  // BuildGTVBAOPushConstants() in addon.cpp; the two lists must stay in lockstep.
  float GTVBAO_normal_input_mode;       // c[13] - 0=depth-derived, 1=MRT g-buffer
  float GTVBAO_mrt_normal_available;    // c[14]
  float GTVBAO_normal_influence;        // c[15]
  float GTVBAO_normal_depth_blend;      // c[16]
  float GTVBAO_normal_sharpness;        // c[17]
  float GTVBAO_normal_edge_rejection;   // c[18]
  float GTVBAO_normal_z_preservation;   // c[19]
  float GTVBAO_normal_detail_response;  // c[20]
  float GTVBAO_normal_max_darkening;    // c[21]
  float GTVBAO_normal_darkening_mode;   // c[22]
  float GTVBAO_normal_transform_mode;   // c[23] - 0=view_g+flipZ, 1=viewInv_g, 2=passthrough, 3=view_g, 4=viewInv_g+flipZ
  // - GI parameters -
  float g_gi_enabled;                   // c[24] - 0=off, 1=on
  float g_gi_light_exposure;            // c[25] - HDR light buffer exposure scale [0.001..10]
  float g_gi_intensity;                 // c[26] - GI intensity [0..5]
  float g_gi_saturation;                // c[27] - GI saturation [0..2]
  float g_gi_multibounce;               // c[28] - multi-bounce enable (0/1)
  float g_gi_multibounce_strength;      // c[29] - feedback intensity [0..10]
  float g_gi_multibounce_saturation;    // c[30] - feedback color saturation [0..2]
  float g_gi_multibounce_max_clamp;     // c[31] - max multi-bounce per-channel (0=off)
  // - SSGI / IS-FAST noise -
  float g_vbgi_debug_view;              // c[32] - SSGI debug view mode
  float GTVBAO_isfast_enabled;          // c[33] - IS-FAST enable (0/1)
  float GTVBAO_isfast_strength;         // c[34] - IS-FAST noise strength [0..1]
  float GTVBAO_isfast_debug;            // c[35] - IS-FAST texture loaded flag (set by shader check)
  float GTVBAO_adaptive_mode;           // c[36] - 0=GI color, 1=albedo
  float GTVBAO_adaptive_luma_strength;  // c[37] - target luma (0=off)
  float GTVBAO_adaptive_luma_blend;     // c[38] - blend original<->normalized
  float GTVBAO_isfast_spatial_scale;    // c[39] - IS-FAST spatial scale [0.25..4]
  float GTVBAO_isfast_temporal_speed;   // c[40] - IS-FAST temporal speed [0..5]
  float GTVBAO_isfast_seed;             // c[41] - IS-FAST seed offset [0..64]
  // - Denoiser -
  float GTVBAO_denoise_leak_threshold;  // c[42] - edge leak threshold [1..4], default 2.5
  float GTVBAO_denoise_leak_strength;   // c[43] - edge leak strength [0..1], default 0.5
  float GTVBAO_temporal_blend;          // c[44] - always 0 (spatial only)
  float GTVBAO_disocclusion_threshold;  // c[45] - unused (spatial only)
  float GTVBAO_noise_type;              // c[46] - 0=IS-FAST, 1=IGN, 2=Hilbert
  // - GTVBAO upgrades: always On (UI toggles removed) -
  float GTVBAO_gtvbao_cdf_enabled;      // c[47] - always 1
  float GTVBAO_gtvbao_cosine_enabled;   // c[48] - always 1
  float GTVBAO_gtvbao_cosine_mode;      // c[49] - 0=Weight, 1=Project, 2=CDF
  float GTVBAO_gtvbao_thickness_enabled;// c[50] - always 1
  float GTVBAO_prefilter_enabled;       // c[51] - 0=Off, 1=On
  // - Foliage -
  float GTVBAO_exclude_foliage;         // c[52] - 0=Off, 1=On - skip AO on foliage
  float GTVBAO_foliage_ao_value;        // c[53] - [0..1] - AO value for excluded foliage
  float GTVBAO_foliage_channel_mode;    // c[54] - 0=o1.w (Sora), 1=o1.z (Kai)
  float GTVBAO_foliage_mask_valid;      // c[55] - 1 = mask pre-pass ran this frame (mask is fresh)
  float GTVBAO_denoise_stage;           // c[56] - denoise_last dispatch mode: 0=spatial combined, 2=spatial-final-no-temporal, 4=GI-only tail
  // - a-trous wavelet filter -
  float GTVBAO_atrous_enabled;          // c[57] - 0=Off, 1=On - wavelet spatial filter
  float GTVBAO_atrous_depth_sigma;      // c[58] - [0.05..4] relative depth edge-stop strength
  float GTVBAO_atrous_normal_sigma;     // c[59] - [2..128] normal edge-stop power
  float GTVBAO_atrous_step;             // c[60] - wavelet stride for this dispatch (1, 2, 4)
  // - Half-resolution spatial pipeline (appended; Full path ignores these) -
  float GTVBAO_resolution;              // c[61] - 0=Full, 1=Half
  float GTVBAO_upscale_plane_sigma;     // c[62] - reconstruction plane edge-stop sigma
  float GTVBAO_upscale_normal_power;    // c[63] - reconstruction normal weight power
  float GTVBAO_upscale_debug;           // c[64] - reconstruction diagnostics mode
  // ── VBGI (GI) MRT normal settings — INDEPENDENT of the AO ones above ──
  // VBGI is a bleeding effect and often wants different geometry handling from
  // AO, so it gets its own copy rather than mirroring c[13]/c[15]/c[19]/c[23].
  // These are push-constant scalars, so both consumers branch on a UNIFORM
  // value: no warp divergence and no extra memory traffic, and only one of the
  // MRT / depth paths ever executes. Texel mapping, decode and transform
  // arithmetic are deliberately NOT duplicated -- those are correctness
  // surfaces, and letting AO and GI disagree on them is how drift starts.
  float GTVBAO_gi_normal_input_mode;      // c[65] - 0=depth, 1=MRT g-buffer
  float GTVBAO_gi_normal_influence;       // c[66] - xy scale, 1 = untouched
  float GTVBAO_gi_normal_z_preservation;  // c[67] - z scale, 1 = untouched
  float GTVBAO_gi_normal_transform_mode;  // c[68] - same encoding as c[23]
  // Fraction of the local direct light re-emitted per bounce (diffuse albedo).
  // Read only by the multi-bounce accumulate pass.
  float g_gi_multibounce_bounce_fraction;  // c[69] - [0..0.5], 0.15 = subtle
  // c[70] 1 = the GI sample path may reuse the à-trous normal pre-decode.
  // Host enables it only when the pre-decode ran, the GI/AO normal transforms
  // match and the GI normal scaling is neutral, so the prepped normal equals
  // the per-sample decode result.
  float GTVBAO_gi_prepped_normal;          // c[70] - 0/1
};

// ── Half-res → full-res block-center mapping (odd-dimension safe) ──
// halfTexel covers full block {2*h, 2*h+1}; center representative is 2*h+1,
// clamped for odd full dims (e.g. full=5, half=3: h=2 -> min(5,4)=4).
int2 GTVBAO_HalfToFullCenter(int2 halfTC, int2 fullDims) {
  return min(halfTC * 2 + 1, max(fullDims - 1, int2(0, 0)));
}
float2 GTVBAO_HalfToFullCenterUV(int2 halfTC, int2 halfDims, int2 fullDims) {
  int2 fullTC = GTVBAO_HalfToFullCenter(
      clamp(halfTC, int2(0, 0), max(halfDims - 1, int2(0, 0))), fullDims);
  return (float2(fullTC) + 0.5) / max(float2(fullDims), float2(1, 1));
}

// Point-load full-res depth at the block center for a half-res texel
// (odd-safe). halfDims = denoiser/AO working domain.
float GTVBAO_LoadMappedDepth(Texture2D<float> depthTex, int2 halfTC, int2 halfDims) {
  uint fw, fh;
  depthTex.GetDimensions(fw, fh);
  int2 fullDims = int2(max(fw, 1u), max(fh, 1u));
  int2 htc = clamp(halfTC, int2(0, 0), max(halfDims - 1, int2(0, 0)));
  return depthTex.Load(int3(GTVBAO_HalfToFullCenter(htc, fullDims), 0));
}

// ── GI parameters are native cb_gtvbao fields (c[25]-c[33]) — no aliases needed.
#define g_isfast_enabled        GTVBAO_isfast_enabled            // IS-FAST enable (0/1)
#define g_isfast_strength       GTVBAO_isfast_strength           // IS-FAST noise strength
#define g_isfast_texture_loaded GTVBAO_isfast_debug              // IS-FAST: 1=texture loaded
#define g_isfast_spatial_scale GTVBAO_isfast_spatial_scale        // IS-FAST spatial scale [0.25..4]
#define g_isfast_temporal_speed GTVBAO_isfast_temporal_speed      // IS-FAST temporal speed [0..5]
#define g_isfast_seed_offset   GTVBAO_isfast_seed                 // IS-FAST seed offset [0..64]
// Legacy adaptive macros — hardcoded to 0 (no effect), slots repurposed for IS-FAST
#define g_gi_adaptive_r         0.f
#define g_gi_adaptive_g         0.f
#define g_gi_adaptive_b         0.f
#define g_gi_adaptive_mode      GTVBAO_adaptive_mode             // 0=GI color, 1=albedo
#define g_gi_adaptive_luma_strength GTVBAO_adaptive_luma_strength // [0..5] target luma
#define g_gi_adaptive_luma_blend GTVBAO_adaptive_luma_blend       // [0..1] blend
// g_gi_light_exposure (c[25]) is a native field.
// ── GI resources are passed as function parameters to GTVBAO_MainPass ──
// (avoids fxc X3003 redefinition errors from forward declarations).
// The wrapper .cs_5_0.hlsl files declare and pass them.

// ── MRT normal helpers ──
// The encode/decode itself lives in include/mrt_normal.hlsli, shared with the
// shadow compute passes so the two features cannot disagree about what a texel
// means. Only the GTVBAO-specific reshaping stays here, so every pass that
// touches an MRT normal (main passes, normal_prep, upscale, à-trous) shares one
// definition and cannot drift.
// DecodeMrtNormalAsIs stays per-wrapper only as a thin binding shim: it reads
// that pass's own `g_srcMrtNormal`, which is a per-pass resource.

// Texel address for a screen UV in a source whose resolution differs from the
// AO working domain. Both the main pass and the GI per-sample path must land on
// the SAME texel for the same UV, or the two read different normals for one
// pixel. Previously the GI path truncated `uv * dims` with no texel-centre
// offset while the main pass used `floor((pix + 0.5) * scale)`, which could
// disagree by up to a texel.
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

// The normal-tuning shaping that BuildSelectedInputNormal applies, extracted so
// the GI path can apply exactly the same shaping to its per-sample normal.
// Without this, the GI term silently ignored every MRT normal tuning setting
// and AO and VBGI diverged as soon as a knob was moved. At the neutral defaults
// (influence 1, z_preservation 1) this is the identity, so it does not change
// current output -- it only keeps the two paths from drifting apart later.
//
// `viewNormal` must already be in GTVBAO view space (i.e. post
// TransformNormalToView). `influence` and `zPreservation` are passed in rather
// than read from the AO constants so VBGI can apply its own independent values
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
// The mode is a full 2x2 so every combination can be A/B tested in-game without
// a rebuild: {view_g, viewInv_g} x {flip Z, no flip}.
//
//   0 = view_g    + flip Z   <- believed correct
//   1 = viewInv_g (no flip)
//   2 = passthrough
//   3 = view_g    (no flip)
//   4 = viewInv_g + flip Z
// `transformMode` is passed in rather than read from the AO constant so VBGI
// can select a different matrix through the same code.
float3 TransformNormalToView(float3 decoded, float transformMode)
{
  if (!FalcomNormalValid(decoded)) return float3(0.0, 0.0, 0.0);

  const int mode = (int)transformMode;

  // 2: leave the world-space normal completely alone (diagnostic only - the
  // result is not in view space at all, so the AO maths is meaningless).
  if (mode == 2) return FalcomSafeNormalize3(decoded, float3(0.0, 0.0, 0.0));

  // viewInv_g is the inverse of view_g, so for a rigid (orthonormal rotation)
  // view matrix it is the transpose of view_g's rotation. It is only a
  // meaningful normal transform if the two spaces happen to coincide, which is
  // why modes 1 and 4 exist purely as comparison baselines.
  const bool use_viewInv = (mode == 1 || mode == 4);
  const bool flip_z      = (mode == 0 || mode == 4);

  float3x3 m = (float3x3)view_g;
  if (use_viewInv) m = (float3x3)viewInv_g;

  float3 vn = mul(m, decoded);
  if (flip_z) vn.z = -vn.z;
  return FalcomSafeNormalize3(vn, float3(0.0, 0.0, 0.0));
}

float3 DecodeMrtNormalAsIs(uint2 texel);

#include "GTVBAO.h"
#include "GTVBAO.hlsli"

// ── GI denoise (3×3 edge-stopped bilateral) ──
// Shared by the spatial denoise path (stage 0) and the à-trous GI tail (stage
// 4) so the two can never diverge. The two pixels a thread owns are filtered
// from one shared 4×3 fetch grid (12 depth + 12 GI loads instead of 16 + 16),
// Full mode uses a point Load for the depth tap (identical to the point
// SampleLevel at a texel centre; Half mode keeps the SampleLevel block-centre
// mapping), the per-pixel GI length is hoisted out of the tap loop, and the
// depth/colour exponentials are evaluated as one exp of the summed exponent.
void GTVBAO_DenoiseGI(uint2 pixCoordBase, GTAOConstants consts,
    Texture2D<float4> srcGI, Texture2D<float> srcDepth,
    SamplerState samp, RWTexture2D<float4> outGI)
{
    uint w, h;
    srcGI.GetDimensions(w, h);

    const int2 maxTC = int2(max((int)w - 1, 0), max((int)h - 1, 0));
    // Grid: columns cover leftPixel.x-1 .. leftPixel.x+2, rows cover
    // leftPixel.y-1 .. leftPixel.y+1, which is both side pixels' 3×3.
    const int2 gridBase = clamp(int2(pixCoordBase), int2(0, 0), maxTC) - int2(1, 1);
    const bool halfDepth = GTVBAO_resolution > 0.5f;

    float4 giGrid[4][3];
    float depthGrid[4][3];
    [unroll]
    for (int gy = 0; gy < 3; ++gy)
    {
        [unroll]
        for (int gx = 0; gx < 4; ++gx)
        {
            int2 tc = clamp(gridBase + int2(gx, gy), int2(0, 0), maxTC);
            giGrid[gx][gy] = srcGI.Load(int3(tc, 0));
            if (halfDepth)
            {
                float2 uv = (float2(tc) + 0.5) * consts.ViewportPixelSize;
                depthGrid[gx][gy] = srcDepth.SampleLevel(samp, uv, 0);
            }
            else
            {
                depthGrid[gx][gy] = srcDepth.Load(int3(tc, 0));
            }
        }
    }

    const int2 offsets[8] = {
        int2(-1,-1), int2(0,-1), int2(1,-1),
        int2(-1, 0),            int2(1, 0),
        int2(-1, 1), int2(0, 1), int2(1, 1)
    };

    [unroll]
    for (int side = 0; side < 2; ++side)
    {
        int2 pixCoord = int2(pixCoordBase) + int2(side, 0);
        if (pixCoord.x >= (int)w || pixCoord.y >= (int)h) continue;

        float4 centerGI = giGrid[side + 1][1];
        float centerDepth = depthGrid[side + 1][1];
        float invCenterLen = 1.0 / max(length(centerGI.rgb), 0.001);

        float4 sum = centerGI;
        float weightSum = 1.0;

        [unroll]
        for (uint i = 0; i < 8; ++i)
        {
            int2 off = offsets[i];
            float4 neighborGI = giGrid[side + 1 + off.x][1 + off.y];
            float neighborDepth = depthGrid[side + 1 + off.x][1 + off.y];

            float depthDiff = abs(centerDepth - neighborDepth);
            float colorDiff = length(neighborGI.rgb - centerGI.rgb) * invCenterLen;
            float weight = exp(-(depthDiff * 10.0 + colorDiff * 2.0));

            sum += neighborGI * weight;
            weightSum += weight;
        }

        outGI[pixCoord] = sum / max(weightSum, 0.001);
    }
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
  consts.EffectFalloffRange = saturate(GTVBAO_falloff_range);
  consts.RadiusMultiplier = max(0.3, GTVBAO_radius_multiplier);
  consts.FinalValuePower = max(0.5, GTVBAO_final_value_power);
  consts.DenoiseBlurBeta = max(0.01, GTVBAO_denoise_blur_beta);
  consts.SampleDistributionPower = max(1.0, GTVBAO_sample_distribution_power);
  consts.ThinOccluderCompensation = max(0.0, GTVBAO_thin_occluder_compensation);
  consts.DepthMIPSamplingOffset = max(0.0, GTVBAO_depth_mip_sampling_offset);
  consts.NoiseIndex = (int)GTVBAO_noise_index;
  consts.Padding0 = 0.0;

  return consts;
}

#endif  // SRC_GAMES_SORA_VANILLAPLUS_GTVBAO_COMMON_HLSL_
