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
  float shadow_isfast_jitter_amount;
  float shadow_isfast_jitter_speed;

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
  float volfog_is_fast_enabled;
  float isfast_noise_bound;
  float volfog_color_correction_strength;

  // Wire alpha improvements (wirefence_0x26F1598B)
  float wire_alpha_mode;              // 0=Vanilla, 1=Stochastic Hash, 2=Stochastic IS-FAST
  float wire_alpha_sharpen;           // stochastic threshold shaping
  float wire_alpha_threshold_offset;  // additive offset to game alpha threshold
  float wire_alpha_temporal_amount;   // 0=static, 1=fully temporal
  float wire_alpha_temporal_speed;    // frame progression speed for temporal noise

  // Face shadow jitter improvements (face_0x0C968FD4)
  float face_jitter_mode;             // 0=Off, 1=Hash, 2=IS-FAST
  float face_jitter_amount;           // shadow-kernel jitter radius scale
  float face_jitter_speed;            // temporal progression speed
  float face_jitter_temporal_amount;  // 0=static, 1=fully temporal

  // Jar transparency jitter improvements (jar_0x4CB77B59)
  float jar_jitter_mode;             // 0=Off, 1=Hash, 2=IS-FAST
  float jar_jitter_amount;           // blend strength from vanilla to stochastic alpha
  float jar_jitter_speed;            // temporal progression speed
  float jar_jitter_temporal_amount;  // 0=static, 1=fully temporal
  float jar_alpha_threshold_offset;  // additive alpha threshold bias for stochastic compare
  float jar_alpha_sharpen;           // stochastic noise shaping

  // Depth of Field improvements (DOF_0xAB6DBF4D / DOF_0x2734F870)
  float dof_mode;                  // 0=Vanilla, 1=Improved (Method 3 / gather)
  float dof_strength;              // final blend strength
  float dof_radius_scale;          // scales cocMaxRadius from game constants
  float dof_sample_count;          // gather tap budget
  float dof_near_scale;            // near CoC shaping
  float dof_far_scale;             // far CoC shaping
  float dof_coc_curve;             // CoC power curve
  float dof_edge_threshold;        // CoC mismatch rejection

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

  // Dedicated SSS binding state for lighting fallback control.
  float sss_dedicated_bound;  // 0=no dedicated t15 SSS bound this draw, 1=valid
  // Dedicated XeGTAO binding state for lighting AO override control.
  float xegtao_dedicated_bound;  // 0=no dedicated t22 XeGTAO bound this draw, 1=valid
  // XeGTAO debug visualization mode for lighting shaders.
  float xegtao_debug_mode;  // 0=off, >0=debug view
  // XeGTAO MRT normal mode: 0=off(depth fallback), 1=view-transformed.
  float xegtao_normal_input_mode;
  // XeGTAO MRT normal source status this frame: 0=fallback depth normal path, 1=MRT normal path.
  float xegtao_mrt_normal_valid;
  // XeGTAO bent normals application in lighting: 0=off, 1=environment-only.
  float xegtao_bent_normals;
  // XeGTAO bent diffuse directional shading strength.
  float xegtao_bent_diffuse_strength;
  // XeGTAO bent diffuse cone softness (cosine-space transition width).
  float xegtao_bent_diffuse_softness;
  // XeGTAO bent specular-proxy attenuation strength.
  float xegtao_bent_specular_strength;
  // XeGTAO bent specular-proxy roughness control.
  float xegtao_bent_specular_proxy_roughness;
  // Maximum extra darkening from bent-normal modulation.
  float xegtao_bent_max_darkening;
  // Force neutral AO.X on draws that are gated away from XeGTAO consumption.
  float xegtao_force_neutral_x;
  // Force black debug output for XeGTAO debug modes 01..09 on this draw.
  float xegtao_debug_blackout;
  // True when this draw is consuming XeGTAO AO.X (custom-bind or copyback path).
  float xegtao_ao_active_for_draw;
  // Foliage-only XeGTAO AO blend factor: 0=no AO on foliage, 1=full XeGTAO AO.
  float xegtao_foliage_ao_blend;
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
