///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus — Custom SSR (Sora 2nd) shared declarations
//
// DX11 / Shader Model 5 compute pipeline. No wave intrinsics, no SM6 features.
// Scene constants come from the game's own cb_scene (b0), user settings via
// push constants (b13) — identical approach to the GTVBAO integration.
//
// References:
//   AMD FidelityFX SSSR (hierarchical traversal, hit validation)
//   Stachowiak/Uludag 2015 "Stochastic Screen-Space Reflections"
//   Heitz 2018 "Sampling the GGX Distribution of Visible Normals"
//   McGuire/Mara 2014 "Efficient GPU Screen-Space Ray Tracing"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SRC_GAMES_FALCOMENGINE_PLUS_SSR_COMMON_HLSL_
#define SRC_GAMES_FALCOMENGINE_PLUS_SSR_COMMON_HLSL_

// ── Game's scene constant buffer (b0) — subset used by SSR passes ──
cbuffer cb_scene : register(b0)
{
  float4x4 view_g          : packoffset(c0);
  float4x4 viewInv_g       : packoffset(c4);
  float4x4 proj_g          : packoffset(c8);
  float4x4 projInv_g       : packoffset(c12);
};

// ── SSR parameters via push constants (b13) ──
// Append-only: indices are stable across phases.
cbuffer cb_ssr : register(b13)
{
  float SSR_mode;                 // c0  master toggle
  float SSR_debug_view;           // c1  0=off, 1=hiz mip0, 2=hiz mip N, 3=conservatism
  float SSR_debug_mip;            // c2  mip index for debug views 2/3
  float SSR_max_ray_distance;     // c3  global ray length cap (world units)
  float SSR_thickness;            // c4  world-space thickness heuristic
  float SSR_roughness_threshold;  // c5  skip pixels above this roughness
  float SSR_frame_index;          // c6  frame counter % 64 (noise seed)
  float SSR_max_traversal_steps;  // c7  hierarchical march iteration cap
  float SSR_mirror_bias;          // c8  VNDF bias toward mirror direction
  float SSR_intensity;            // c9  reflection strength
  float SSR_denoise_radius;       // c10 max spatial kernel radius (px)
  float SSR_denoise_taps;         // c11 resolve tap count
  float SSR_half_res_trace;       // c12 reserved (phase 5)
  float SSR_radiance_source;      // c13 radiance source mirror (0=t0, 1=backbuffer copy)
  float SSR_scratch_width;        // c14 valid width of scratch data this dispatch
  float SSR_scratch_height;       // c15 valid height of scratch data this dispatch
  // ── Phase 2.1 traversal diagnostics (append-only) ──
  float SSR_bypass_validation;    // c16 0=Full ValidateHit, 1=in-bounds+finite-depth only
  float SSR_forced_ray_mode;      // c17 0=Production, 1=Fixed normal, 2=Screen diagonal, 3=Depth/Floor normal, 4=Mirror(deterministic)
  float SSR_normal_convention;    // c18 canonical: 1=mul(n,view_g)
  // ── Phase 2.2 self-hit sweep ──
  float SSR_self_hit_threshold_px;// c19 manhattan pixel threshold for self-intersection
  // ── Phase 2.3 backface A/B ──
  float SSR_backface_gate;        // c20 1=reject backface hits, 0=accept (diagnostic)
  // ── Phase 2.4 initial-advance displacement ──
  float SSR_initial_advance_bias; // c21 screen-space px minimum before FIRST depth test
  // ── Phase 2.7 thickness-gate isolation ──
  float SSR_thickness_gate;       // c22 1=reject by thickness confidence, 0=diagnostic off
  // ── Phase 2.9 perpendicular-distance metric ──
  float SSR_thickness_mode;       // c23 canonical: 1=Perpendicular
  // ── Phase 3 stochastic SSR (append-only) ──
  float SSR_diagnostics;          // c24 1=probe atomics + heavy debug payloads ON
  float SSR_rough_min;            // c25 roughness clamp lower bound (BRDF slider mirror)
  float SSR_rough_max;            // c26 roughness clamp upper bound
  float SSR_ray_count;            // c27 stochastic rays per pixel 1..4
  float SSR_rough_interp;         // c28 0=perceptual (alpha=r^2), 1=already alpha
  float SSR_stochastic;           // c29 1=VNDF production rays, 0=deterministic mirror
  float SSR_eligibility_mode;     // c30 0=Vanilla flag, 1=Custom material, 2=All opaque
  float SSR_eligibility_rough;    // c31 roughness cutoff for Custom Material eligibility
  // ── Phase 4 spatial reconstruction (append-only) ──
  float SSR_resolve_radius_max;   // c32 max resolve radius in px (1-16)
  float SSR_resolve_depth_sigma;  // c33 view-Z similarity sigma
  float SSR_resolve_normal_sigma; // c34 normal similarity exponent
  float SSR_resolve_rough_sigma;  // c35 roughness similarity sigma
  float SSR_roughness_alpha_blend; // c36 DEPRECATED: superseded by resolve coverage
  float SSR_same_surface_reject;   // c37 1=reject same-surface candidates, 0=diagnostic only
  float SSR_plane_delta_threshold; // c38 perpendicular-distance threshold for same-plane detection
  // ── Phase 3.Fix15 Mirror-vs-VNDF CompareProbe (append-only, read-only) ──
  float SSR_probe_pixel_x;         // c39 CompareProbe pixel X
  float SSR_probe_pixel_y;         // c40 CompareProbe pixel Y
  float SSR_isfast_active;         // c41 R2E.2: IS-FAST volume bound (1) / IGN fallback (0)
};

#define SSR_FLT_MAX 3.402823466e+38

// ── Depth linearization (identical math to GTVBAO BuildGTAOConstants — proven
// against this game's proj_g conventions, handles reversed-Z via sign guard) ──
void SSR_GetDepthUnpackConsts(out float mul_c, out float add_c)
{
  mul_c = -proj_g[3][2];
  add_c =  proj_g[2][2];
  if (mul_c * add_c < 0.0)
  {
    add_c = -add_c;
  }
}

// Hardware NDC depth -> positive linearized view-space Z.
// Invalid/sky values collapse to SSR_FLT_MAX so they never occlude MIN reduction.
float SSR_LinearizeDepth(float ndc_depth)
{
  float mul_c, add_c;
  SSR_GetDepthUnpackConsts(mul_c, add_c);
  float denom = add_c - ndc_depth;
  float z = (abs(denom) > 1e-8) ? (mul_c / denom) : 0.0;
  z = max(z, 0.0);
  return isfinite(z) && z > 0.0 ? z : SSR_FLT_MAX;
}

// Near-plane distance derived from projection linearization constants
// (no hardcoded assumption about Z convention). The two candidate distances
// correspond to ndc=0 and ndc=1; the smaller is the near plane regardless
// of whether this engine uses standard or reversed Z.
float SSR_NearPlaneDistance()
{
  float mul_c, add_c;
  SSR_GetDepthUnpackConsts(mul_c, add_c);
  float d0 = abs(mul_c) / max(abs(add_c),       1e-8);
  float d1 = abs(mul_c) / max(abs(add_c - 1.0), 1e-8);
  return min(d0, d1);
}

#endif  // SRC_GAMES_FALCOMENGINE_PLUS_SSR_COMMON_HLSL_
