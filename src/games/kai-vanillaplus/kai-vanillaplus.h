#ifndef SRC_GAMES_KAI_VANILLAPLUS_INJECTION_H_
#define SRC_GAMES_KAI_VANILLAPLUS_INJECTION_H_

// Must remain 32-bit aligned for shader injection push constants.
struct SssInjectData {
  // Character Shadowing (char_0x445A1838)
  float char_shadow_mode;
  float char_shadow_sample_count;
  float char_shadow_hard_shadow_samples;
  float char_shadow_fade_out_samples;
  float char_shadow_surface_thickness;
  float char_shadow_contrast;
  float char_shadow_light_screen_fade_start;
  float char_shadow_light_screen_fade_end;
  float char_shadow_min_occluder_depth_scale;
  float char_shadow_jitter_enabled;
  float char_shadow_strength;

  // Lighting shadow controls (lighting_0x430ED091)
  float shadow_pcss_jitter_enabled;
  float shadow_pcss_sample_mode;

  // SSGI controls (lighting_0x430ED091)
  float ssgi_mod_enabled;
  float ssgi_color_boost;
  float ssgi_alpha_boost;
  float ssgi_pow;

  // SSR controls (SSR_0x209125C1)
  float ssr_mode;
  float ssr_ray_count_scale;

  float ssr_temporal_clamp_enable;
  float ssr_temporal_clamp_radius;
  float ssr_temporal_clamp_strength;

  float ssr_temporal_jitter_enable;
  float ssr_temporal_jitter_amount;

  float padding;
};

#ifndef __cplusplus
#if ((__SHADER_TARGET_MAJOR == 5 && __SHADER_TARGET_MINOR >= 1) || __SHADER_TARGET_MAJOR >= 6)
cbuffer sss_injection : register(b13, space0) {
#else
cbuffer sss_injection : register(b13) {
#endif
  SssInjectData sss_injection_data : packoffset(c0);
}
#endif

#endif  // SRC_GAMES_KAI_VANILLAPLUS_INJECTION_H_
