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

  // Cubemap controls (lighting + glass): 0 = Vanilla, 1 = Improved
  float cubemap_improvements_enabled;
  // Lighting cubemap mip boost scale (applies in lighting shader only)
  float cubemap_lighting_mip_boost;
  // Floor cubemap mip scale (applies in floor_0x8337B262)
  float floor_cubemap_mip_scale;

  // Lighting shadow controls
  float shadow_base_softness;

  // Fog color correction controls (lighting_0x430ED091 / lightingsoft_0xF6C55E5F)
  float fog_color_correction_enabled;
  float fog_hue;
  float fog_chrominance;
  float fog_avg_brightness;
  float fog_min_brightness;
  float fog_min_chroma_change;
  float fog_max_chroma_change;
  float fog_lightness_strength;
  float fog_color_correction_strength;

  // Volumetric fog improvements (volfog_0xBD7DFE49)
  float volfog_tricubic_enabled;
  float volfog_color_correction_strength;

  // SSS controls (0x534E54EA)
  float foliage_translucency_scale;
  float foliage_opacity_scale;
  float foliage_ssao_scale;
  float foliage_sss_enabled;
  float foliage_sss_strength;
  float foliage_sss_sample_count;
  float foliage_sss_surface_thickness;
  float foliage_sss_contrast;
  float foliage_sss_jitter_enabled;
  float foliage_debug_mode;
  float foliage_sss_height_enabled;
  float foliage_sss_height_min;
  float foliage_sss_height_max;
  float foliage_sss_height_fade;
  float foliage_sss_vertical_reject;
  float foliage_sss_max_darkening;
  float foliage_sss_bright_reject_threshold;
  float foliage_sss_bright_reject_fade;

  // Character SSGI Composite (lighting_0x430ED091 / lightingsoft_0xF6C55E5F in-shader mode)
  float char_gi_enabled;  // 0=off, 1=on
  float char_gi_strength;
  float char_gi_alpha_scale;
  float char_gi_chroma_strength;
  float char_gi_luma_strength;
  float char_gi_shadow_power;
  float char_gi_headroom_power;
  float char_gi_max_add;
  float char_gi_dark_boost;
  float char_gi_debug_mode;
  float char_gi_debug_scale;
  float char_gi_debug_chars_only;
  float char_gi_bright_boost;
  float char_gi_peak_luma_cap;
  float char_gi_depth_reject;
  float char_gi_normal_reject;
  float char_gi_ao_influence;
  float char_gi_reject_strength;
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
