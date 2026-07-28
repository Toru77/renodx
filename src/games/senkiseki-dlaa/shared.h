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
  float dlaa_per_object_motion;    // 0=Off, 1=On — per-object MV from VS injection
  float dlaa_velocity_scale;       // [0.1..5] default 1.0 — scale motion vectors
  float dlaa_debug_view;           // 0=Off, 1=Velocity visualization
  float dlaa_debug_logging;        // 0=Off, 1=On — debug logging

  // ── Jitter (written by addon, read by injected VS) ──
  float jitter_offset_x;           // Current frame's sub-pixel jitter X (NDC)
  float jitter_offset_y;           // Current frame's sub-pixel jitter Y (NDC)
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

#endif

#endif  // SRC_GAMES_SENKISEKI3_DLAA_SHARED_H_
