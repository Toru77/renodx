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

  // Lighting shadow controls
  float shadow_base_softness;

  // Car shading controls (car_0xDC173E86)
  float car_mode;
  float car_diffuse_scale;
  float car_specular_scale;
  float car_reflection_scale;
  float car_local_light_scale;
  float car_ambient_scale;
  float car_rim_scale;
  float car_shadow_scale;
  float car_ssr_scale;
  float car_cubemap_mip_scale;
  float car_cubemap_brightness;

  // Experimental lighting controls (lighting_0x430ED091 / lightingsoft_0xF6C55E5F)
  float exp_master_improved;
  float exp_env_brdf_enabled;
  float exp_env_brdf_strength;
  float exp_probe_sampling_enabled;
  float exp_probe_sampling_strength;
  float exp_probe_direction_strength;
  float exp_probe_mip_strength;
  float exp_horizon_occlusion_enabled;
  float exp_horizon_occlusion_strength;
  float exp_horizon_energy_fraction;
  float exp_horizon_power;
  float exp_fog_color_correction_enabled;
  float exp_fog_hue;
  float exp_fog_chrominance;
  float exp_fog_avg_brightness;
  float exp_fog_min_brightness;
  float exp_fog_color_correction_strength;

  float padding0;
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

