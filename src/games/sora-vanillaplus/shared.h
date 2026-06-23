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
  // Volumetric Fog IS-FAST jitter
  float volfog_isfast_enabled;         // runtime: derived from global IS-FAST + volfog toggle
  float volfog_isfast_texture_loaded;  // runtime: IS-FAST noise texture is available
  float volfog_jitter_enabled;         // 0=Off, 1=On — user toggle
  float volfog_jitter_amount;          // [0..2], default 0.5 — jitter strength
  float volfog_jitter_speed;           // [0..1024], default 237 — temporal speed
  float volfog_isfast_spatial_scale;   // [0.25..4], default 1.0 — volfog spatial scale
  float volfog_isfast_dedicated_sampler;// 0=s1 point, 1=s2 dedicated point-wrap sampler

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

  // —— XeGTAO (Visibility Bitmask AO + optional GI) ——
  float xegtao_mode;                // 0=Off (vanilla AO), 1=On (Bitmask AO)
  float xegtao_quality_level;       // 0=Low, 1=Medium, 2=High, 3=Ultra
  float xegtao_denoise_passes;      // 0=Off, 1..3
  float xegtao_radius;              // World-space effect radius
  float xegtao_falloff_range;       // [0..1], default 0.615
  float xegtao_radius_multiplier;   // [0.3..3], default 1.457
  float xegtao_final_power;         // [0.5..5], default 2.2 (AO power)
  float xegtao_sample_distribution; // [1..3], default 2.0
  float xegtao_bitmask_thickness;   // [0.01..2.0], default 0.2 — world-space thickness for bitmask
  float xegtao_depth_mip_offset;    // [2..6], default 3.30
  float xegtao_denoise_blur_beta;   // Denoise sharpness, default 1.2
  float xegtao_internal_resolution; // 50/75/100 %, default 75
  float xegtao_debug_view;          // 0=Off, 1=AO gray, 2=GI only, 3=Bitmask viz
  float xegtao_debug_logging;       // 0=Off, 1=On
  float xegtao_dedicated_bound;     // 0/1 — set at runtime: t22 holds valid XeGTAO AO
  float xegtao_fix_experimental;    // 0=Off, 1-5=experimental (unused in bitmask path)
  float xegtao_ssgi_bound;          // 0/1 — set at runtime: t23 holds valid GI
  float xegtao_ssgi_debug;          // 0=Off (add GI), 1=On (replace scene with GI)

  // —— SSGI (Screen Space Global Illumination via Visibility Bitmask) ——
  float ssgi_enabled;               // 0=Off, 1=On (requires xegtao_mode >= 1)
  float ssgi_intensity;             // [0..5], default 1.0
  float ssgi_saturation;            // [0..2], default 1.0
  float ssgi_char_mask_strength;    // [0..1], default 0 — reduce SSGI on characters (0=off, 1=fully masked)
  float ssgi_multibounce;           // 0=Off, 1=On
  float ssgi_multibounce_strength;  // [0..10], default 1.0 — feedback intensity
  float ssgi_multibounce_saturation;// [0..2], default 1.0 — color saturation of feedback
  float ssgi_adaptive_r;            // [0..1], default 0 — per-channel red adaptive boost
  float ssgi_adaptive_g;            // [0..1], default 0 — per-channel green adaptive boost
  float ssgi_adaptive_b;            // [0..1], default 0 — per-channel blue adaptive boost
  float ssgi_adaptive_mode;         // 0=GI color, 1=surface albedo
  float ssgi_adaptive_luma_strength;// [0..5], default 0 — target luma for normalization (0=off)
  float ssgi_adaptive_luma_blend;   // [0..1], default 0.5 — blend between original and normalized
  float ssgi_max_clamp;             // [0..20], default 0 — max GI per-channel (0=off)
  float ssgi_reduce_ao;             // 0=Off, 1=On — reduce AO where indirect light exists
  float ssgi_reduce_ao_strength;    // [0..5], default 1.0 — strength of AO reduction by indirect light
  float ssgi_debug_logging;         // 0=Off, 1=On — SSGI debug logging
  float ssgi_debug_view;            // 0=Off, 1=RawGI, 2=Denoised, 3=LightBuf, 4=Accum, 5=Samples, 6=LightColor
  float ssgi_affect_lights;         // 0=Off, 1=On — additively blend lightColor into GI
  float ssgi_lights_strength;       // [0..5], default 1.0 — multiplier for lightColor contribution
  float ssgi_lights_saturation;     // [0..100], default 1.0 — vibrance for lightColor: 0=gray, 1=neutral
  float ssgi_cascade_debug;         // 0=Off, 1=On — show shadowmapCascadeCount_g as color overlay
  float shadow_filter_method;       // 0=Off (single sample), 1=Falcom (10-tap PCF), 2=PCSS
  float shadow_edge_tint;           // 0=Off, 1=Falcom (vanilla red tint), 2=Improved (PCSS vibrancy)
  // —— PCSS (Percentage Closer Soft Shadows) ——
  float shadow_pcss_jitter_enabled;    // 0=Off, 1=On
  float shadow_pcss_jitter_amount;     // [0..1], default 1.0 — blend static→temporal
  float shadow_pcss_jitter_speed;      // [0..500], default 237.0 — temporal animation speed
  float shadow_base_softness;          // [0..0.5], default 0.2 — constant penumbra offset
  float shadow_penumbra_scale;         // [1..200], default 60.0 — penumbra width multiplier
  float shadow_pcss_search_radius;     // [1..100], default 30.0 — blocker search radius multiplier
  float shadow_pcss_filter_width;      // [0.1..5], default 1.0 — PCF filter width multiplier
  float shadow_pcss_depth_cap;         // [0.01..0.5], default 0.05 — max depth diff for penumbra
  float shadow_pcss_cascade_blend;     // [0.02..1], default 0.2 — cross-fade width (smaller=wider blend)
  // —— PCSS Experimental Fixes (0=off/default behavior) ——
  float shadow_pcss_fix_texel_radius;   // 0=Off, 1=On — texel-based filter radius (consistent across quality)
  float shadow_pcss_fix_clamp_cascade;  // [0..500], default 0 — max cascade world size (0=off), clamp to this
  float shadow_pcss_fix_min_radius;     // [0..100], default 0 — minimum filter radius in texels (0=off)
  float shadow_pcss_fix_auto_blend;     // 0=Off, 1=On — auto-scale cascade blend with split distance
  // —— Colored Shadow Penumbra (PCSS Improved mode) ——
  float shadow_penumbra_color_strength;// [0..2], default 1.0 — how strongly to apply vibrancy effect
  float shadow_penumbra_vibrance;      // [0..100], default 1.0 — 0=grayscale, 1=neutral, >1=vivid
  float shadow_penumbra_detection;     // [0.01..1], default 0.5 — what counts as penumbra (higher=wider)
  float shadow_penumbra_debug_view;    // 0=Off, 1=PenumbraMask, 2=TintColor, 3=Result, 4=SunColor
  float shadow_penumbra_color_brightness;// [0..5], default 1.0 — brightness multiplier for tint color
  float shadow_penumbra_falcom_blend;   // [0..1], default 0 — blend toward Falcom shadowEdgeColor tint
  float shadow_penumbra_edge_vibrance;  // [0..100], default 1.0 — vibrance applied to shadowEdgeColor in Improved
  float shadow_penumbra_lightcolor_blend;// [0..1], default 0 — blend tint toward sun color (lightColor_g)
  float shadow_penumbra_lightcolor_saturation;// [0..100], default 1.0 — vibrance for lightColor before blending
  // —— IS-FAST mirrors for shadow pass (set from g_isfast_* globals) ——
  float shadow_isfast_enabled;         // 0/1 — mirror of g_isfast_enabled
  float shadow_isfast_texture_loaded;  // 0/1 — set at runtime by addon
  float shadow_isfast_spatial_scale;   // [0.25..4], default 1.0
  float shadow_isfast_temporal_speed;  // [0..5], default 1.0
  float shadow_isfast_seed_offset;     // [0..64], default 0
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
