#ifndef SRC_GAMES_GENERIC_VANILLAPLUS_SHARED_H_
#define SRC_GAMES_GENERIC_VANILLAPLUS_SHARED_H_

// Keep this 32-bit aligned for push constant injection.
struct ShaderInjectData {
  float mod_enabled;
  float slider_1;
  float slider_2;
  float slider_3;
  // Volumetric haze AA mode: 0 = Vanilla, 1 = Improved (tricubic haze AA)
  float volfog_haze_aa_mode;

  // Character Shadowing
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
  // Shadow type: 0 = Camera, 1 = World, 2 = Combined
  float char_shadow_type;
  // Per-pass strengths (0..1).
  float char_shadow_camera_strength;
  float char_shadow_world_strength;

  // Environment Screen Space Shadows
  float env_sss_enabled;
  float env_sss_strength;
  float env_sss_sample_count;
  float env_sss_surface_thickness;
  float env_sss_contrast;
  float env_sss_jitter_enabled;
  float env_sss_height_enabled;
  float env_sss_height_min;
  float env_sss_height_max;
  float env_sss_height_fade;
  float env_sss_vertical_reject;
  float env_sss_max_darkening;
  float env_sss_bright_reject_threshold;
  float env_sss_bright_reject_fade;
  // Debug view mode (0 = off, 1..4 = inspection views)
  float debug_show_env_sss;
};

#ifndef __cplusplus
cbuffer shader_injection : register(b13) {
  ShaderInjectData shader_injection_data : packoffset(c0);
}

#define VANILLAPLUS_MOD_ENABLED shader_injection_data.mod_enabled
#define VANILLAPLUS_SLIDER_1    shader_injection_data.slider_1
#define VANILLAPLUS_SLIDER_2    shader_injection_data.slider_2
#define VANILLAPLUS_SLIDER_3    shader_injection_data.slider_3
#define VANILLAPLUS_VOLFOG_HAZE_AA shader_injection_data.volfog_haze_aa_mode
#define VANILLAPLUS_VOLFOG_HAZE_AA_STRENGTH 1.0
#endif

#endif  // SRC_GAMES_GENERIC_VANILLAPLUS_SHARED_H_
