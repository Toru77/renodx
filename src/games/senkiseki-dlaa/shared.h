#ifndef SRC_GAMES_SENKISEKI3_DLAA_SHARED_H_
#define SRC_GAMES_SENKISEKI3_DLAA_SHARED_H_

// Senkiseki3 DLAA ShaderInjectData
// 32-bit aligned for push constant injection at b13.
// All float fields to ensure alignment in cbuffer.

struct ShaderInjectData {
  // ── DLAA ──
  float dlaa_enabled;              // 0=Off (FXAA passthrough), 1=DLAA
  float dlaa_preset;               // DLSS preset: 0=Default, 1=F, 2=J, 3=K, 4=L, 5=M
  float dlaa_jitter_enabled;       // 0=Off, 1=On — camera projection jitter
  float dlaa_jitter_sign;          // NGX jitter sign: 0=FlipBoth, 1=FlipX, 2=FlipY, 3=Current
  float dlaa_jitter_test;          // 0=Off, 1=On — 8px fixed viewport shift to verify rasterization jitter
  float dlaa_force_reset;          // 0=Off, 1=On — force NGX InReset=1 every frame (no history)
  float dlaa_zero_mv;              // 0=Off, 1=On — force all motion vectors to 0 (A/B: MVs help?)
  float dlaa_mv_direction;         // 0=Off, 1=On — flip MV sign (previous-current vs current-previous)
  float dlaa_mv_threshold;         // px — zero out MVs below this magnitude (kills static noise)
  float dlaa_depth_source;         // 0=Auto (last full-res depth push), 1..3 = hash-gate to a pass
  float dlaa_per_object_motion;    // 0=Off, 1=On — per-object MV from VS injection
  float dlaa_velocity_scale;       // [0.1..5] default 1.0 — scale motion vectors
  float dlaa_debug_view;           // 0=Off, 1=HSV, 2=Arrows, 3=Magnitude, 4=Reprojection
  float dlaa_debug_scale;          // MV debug display multiplier (speed -> brightness/length)
  float dlaa_debug_logging;        // 0=Off, 1=On — debug logging

  // ── DLSS feature flags (A/B test) ──
  float dlaa_flag_is_hdr;          // 0=Off, 1=On — input color is HDR
  float dlaa_flag_depth_inverted;  // 0=Off, 1=On — depth is reverse-Z (1=near)
  float dlaa_flag_auto_exposure;   // 0=Off, 1=On — DLSS computes exposure

  // ── Jitter (written by addon, read by injected VS) ──
  float jitter_offset_x;           // Current frame's sub-pixel jitter X (NDC)
  float jitter_offset_y;           // Current frame's sub-pixel jitter Y (NDC)

  // Previous frame's ViewProjection (row-major), injected by the addon each
  // frame for per-object motion vectors. Senkiseki3 does NOT populate cb0 c74
  // (that was the Sora engine), so the VS reads it from b13 instead.
  float prev_view_proj[16];

  // ── MV texture precision A/B (addon-side only, not read by shaders) ──
  float dlaa_velocity_format;  // 0 = r16g16_float (16-bit), 1 = r32g32_float (32-bit)

  // ── Effect/particle exclusion (read by the velocity compute via push constants) ──
  float dlaa_exclude_effects;  // 0=Off, 1=On — mask particles/effects out of DLAA (invalid-MV opt-out)

  // ── Jitter method (addon-side; read by the addon + VS injection) ──
  float dlaa_jitter_method;    // 0=Per VS (jitter inside replaced VSs), 1=Global (patch shared ViewProjection)
};

#ifndef __cplusplus
#if ((__SHADER_TARGET_MAJOR == 5 && __SHADER_TARGET_MINOR >= 1) || __SHADER_TARGET_MAJOR >= 6)
cbuffer shader_injection : register(b13, space0) {
#else
cbuffer shader_injection : register(b13) {
#endif
  ShaderInjectData shader_injection_data : packoffset(c0);
}

#define DLAA_ENABLED              shader_injection_data.dlaa_enabled
#define DLAA_PRESET               shader_injection_data.dlaa_preset
#define DLAA_JITTER_ENABLED       shader_injection_data.dlaa_jitter_enabled
#define DLAA_PER_OBJECT_MOTION    shader_injection_data.dlaa_per_object_motion
#define DLAA_VELOCITY_SCALE       shader_injection_data.dlaa_velocity_scale
#define DLAA_DEBUG_VIEW           shader_injection_data.dlaa_debug_view
#define DLAA_DEBUG_LOGGING        shader_injection_data.dlaa_debug_logging
#define DLAA_JITTER_X             shader_injection_data.jitter_offset_x
#define DLAA_JITTER_Y             shader_injection_data.jitter_offset_y
#define DLAA_EXCLUDE_EFFECTS      shader_injection_data.dlaa_exclude_effects
#define DLAA_JITTER_METHOD        shader_injection_data.dlaa_jitter_method

#endif

#endif  // SRC_GAMES_SENKISEKI3_DLAA_SHARED_H_
