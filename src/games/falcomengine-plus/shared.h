#ifndef SRC_GAMES_FALCOMENGINE_PLUS_SHARED_H_
#define SRC_GAMES_FALCOMENGINE_PLUS_SHARED_H_

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
  float volfog_noise_strength;         // [0..2], default 1.0 — noise strength (0=off, 1=natural, 2=boosted)
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
  float env_sss_hard_shadow_samples; // 0=auto (sampleCount/8), >0=override
  float env_sss_fade_out_samples;    // 0=auto (sampleCount/3), >0=override
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
  float env_sss_csm_gate;            // 0=Off, 1=On — skip SSS when CSM shadow is deep (indoor)
  // Debug view mode (0 = off, 1..4 = inspection views)
  float debug_show_env_sss;

  // —— Local Screen Space Shadows (Bend_SSS for point/spot lights) ——
  float local_sss_enabled;             // 0=Off, 1=On
  float local_sss_strength;            // [0..1]
  float local_sss_light_type;          // 0=Spot, 1=Point, 2=Both
  float local_sss_sample_count;        // [1..64]
  float local_sss_hard_shadow_samples; // 0=auto (sampleCount/8)
  float local_sss_fade_out_samples;    // 0=auto (sampleCount/3)
  float local_sss_surface_thickness;   // [0.001..0.2]
  float local_sss_contrast;            // [0..12]
  float local_sss_light_fade_start;    // [0..1]
  float local_sss_light_fade_end;      // [0..1]
  float local_sss_occluder_depth_scale;// [0..4]

  // —— GTVBAO (Visibility Bitmask AO + optional VBGI) ——
  float gtvbao_mode;                // 0=Off (vanilla AO), 1=On (Bitmask AO)
  float gtvbao_quality_level;       // 0=Low, 1=Medium, 2=High, 3=Ultra
  float gtvbao_denoise_passes;      // 0=Off, 1..3
  float gtvbao_radius;              // World-space effect radius
  float gtvbao_falloff_range;       // [0..1], default 0.615
  float gtvbao_radius_multiplier;   // [0.3..3], default 1.457
  float gtvbao_final_power;         // [0.5..5], default 2.2 (AO power)
  float gtvbao_sample_distribution; // [1..3], default 2.0
  float gtvbao_bitmask_thickness;   // [0.01..2.0], default 0.2 — world-space thickness for bitmask
  float gtvbao_depth_mip_offset;    // [2..6], default 3.30
  float gtvbao_denoise_blur_beta;   // Denoise sharpness, default 1.2
  float gtvbao_denoise_leak_threshold; // [1..4], default 2.5 — edge leak threshold (lower=more leak)
  float gtvbao_denoise_leak_strength; // [0..1], default 0.5 — edge leak strength (higher=less flicker)
  float gtvbao_denoiser_type;        // 0=Spatial, 1=Spatio-Temporal, 2=Poisson
  float gtvbao_temporal_blend;       // derived internally from temporal_frame_count
  float gtvbao_temporal_frame_count; // 0-16 frames accounted for temporal blending (0=off)
  float gtvbao_disocclusion_threshold; // [0.001..0.1], default 0.01 — depth diff to reject history
  float gtvbao_noise_type;         // 0=IS-FAST, 1=IGN, 2=Hilbert — noise selection when IS-FAST on
  float gtvbao_debug_view;          // 0=Off, 1=AO gray, 2=GI only, 3=Bitmask viz
  float gtvbao_debug_logging;       // 0=Off, 1=On
  float gtvbao_dedicated_bound;     // 0/1 — set at runtime: t22 holds valid GTVBAO AO
  float gtvbao_fix_experimental;    // 0=Off, 1-5=experimental (unused in bitmask path)
  float gtvbao_vbgi_bound;          // 0/1 — set at runtime: t23 holds valid VBGI
  float gtvbao_vbgi_debug;          // 0=Off (add GI), 1=On (replace scene with GI)

  // —— VBGI (Visibility-Based Global Illumination) ——
  float vbgi_enabled;               // 0=Off, 1=On (requires gtvbao_mode >= 1)
  float vbgi_intensity;             // [0..5], default 1.0
  float vbgi_saturation;            // [0..2], default 1.0
  float vbgi_char_mask_strength;    // [0..1], default 0 — reduce VBGI on characters (0=off, 1=fully masked)
  float vbgi_multibounce;           // 0=Off, 1=On
  float vbgi_multibounce_strength;  // [0..10], default 1.0 — feedback intensity
  float vbgi_multibounce_saturation;// [0..2], default 1.0 — color saturation of feedback
  float vbgi_multibounce_max_clamp; // [0..20], default 0 — max multi-bounce per-channel (0=off)
  float vbgi_adaptive_r;            // [0..1], default 0 — per-channel red adaptive boost
  float vbgi_adaptive_g;            // [0..1], default 0 — per-channel green adaptive boost
  float vbgi_adaptive_b;            // [0..1], default 0 — per-channel blue adaptive boost
  float vbgi_adaptive_mode;         // 0=GI color, 1=surface albedo
  float vbgi_adaptive_luma_strength;// [0..5], default 0 — target luma for normalization (0=off)
  float vbgi_adaptive_luma_blend;   // [0..1], default 0.5 — blend between original and normalized
  float vbgi_max_clamp;             // [0..20], default 0 — max GI per-channel (0=off)
  float vbgi_reduce_ao;             // 0=Off, 1=On — reduce AO where indirect light exists
  float vbgi_reduce_ao_strength;    // [0..5], default 1.0 — strength of AO reduction by indirect light
  float vbgi_debug_logging;         // 0=Off, 1=On — VBGI debug logging
  float vbgi_debug_view;            // 0=Off, 1=RawGI, 2=Denoised, 3=LightBuf, 4=Accum, 5=Samples, 6=LightColor
  float vbgi_affect_lights;         // 0=Off, 1=On — additively blend lightColor into GI
  float vbgi_lights_strength;       // [0..5], default 1.0 — multiplier for lightColor contribution
  float vbgi_lights_saturation;     // [0..100], default 1.0 — vibrance for lightColor: 0=gray, 1=neutral
  float vbgi_cascade_debug;         // 0=Off, 1=On — show shadowmapCascadeCount_g as color overlay
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

  // ── Kai / Daybreak 2 cubemap fields ──
  // Cubemap
  float cubemap_improvements_enabled;  // 0=Vanilla, 1=Improved
  float cubemap_lighting_mip_boost;    // [0.5..4], default 1.5 — lighting shader cubemap mip scale
  float floor_cubemap_mip_scale;       // [0..4], default 4 — floor reflection roughness/mip response
  // SSGI (Falcom native, not GTVBAO)
  float ssgi_mod_enabled;              // 0=Off, 1=On
  float ssgi_color_boost;              // [0..3], default 1 — scales SSGI RGB before power shaping
  float ssgi_alpha_boost;              // [0..3], default 1 — scales SSGI alpha before saturate
  float ssgi_pow;                      // [0.1..3], default 1 — pow(abs(color), Power) bounce response
  // Depth of Field
  float dof_mode;                      // 0=Vanilla, 1=Improved
  float dof_strength;                  // [0..2], default 1 — overall blend for improved DOF
  float dof_radius_scale;              // [0.25..2.5], default 1.33 — blur radius from CoC
  float dof_sample_count;              // [4..64], default 24
  float dof_near_scale;                // [0..2], default 1 — near-field CoC response
  float dof_far_scale;                 // [0..2], default 1 — far-field CoC response
  float dof_coc_curve;                 // [0.25..4], default 1 — pow(CoC, Curve)
  float dof_edge_threshold;            // [0.02..1], default 0.25 — CoC-mismatch rejection
  // Character SSGI Composite
  float char_gi_strength;              // [0..3], default 3 — overall GI contribution
  float char_gi_alpha_scale;           // [0..3], default 1 — sampled SSGI alpha scale
  float char_gi_chroma_strength;       // [0..2], default 0.5 — colorful GI component
  float char_gi_luma_strength;         // [0..1], default 0 — neutral GI brightness
  float char_gi_shadow_power;          // [0.1..4], default 1.25 — concentrates GI toward dark areas
  float char_gi_dark_boost;            // [0..4], default 0 — extra GI in darker regions
  float char_gi_bright_boost;          // [0..3], default 3 — boosts GI on brighter regions
  float char_gi_headroom_power;        // [0.1..4], default 1.25 — bright pixel GI rejection
  float char_gi_max_add;               // [0..1], default 0.02 — per-channel GI cap
  float char_gi_peak_luma_cap;         // [0..1], default 0 — caps peak GI brightness
  float char_gi_depth_reject;          // [0..16], default 2 — suppress GI across depth edges
  // Fog Color Correction
  float fog_color_correction_enabled;  // 0=Vanilla, 1=Improved
  float fog_hue;                       // [0..2], default 0
  float fog_chrominance;               // [0..2], default 0
  float fog_avg_brightness;            // [0..2], default 0.85
  float fog_min_brightness;            // [-0.5..1], default 0
  float fog_min_chroma_change;         // [0..4], default 0 — min chroma ratio
  float fog_max_chroma_change;         // [0..8], default 0 — max chroma ratio
  float fog_lightness_strength;        // [0..2], default 1 — fog lightness restoration
  float fog_color_correction_strength; // [0..1], default 0.5 — 2D fog correction blend
  // SSR (Kai — not wired in UI yet, fields needed for shader compilation)
  float ssr_mode;                      // 0=Off, 1=On
  float ssr_ray_count_scale;           // [0..5], default 1
  // Foliage translucency/opacity (Kai-specific)
  float foliage_translucency_scale;    // default 1
  float foliage_opacity_scale;         // default 1
  float foliage_ssao_scale;            // default 1
  // Kai GTVBAO bent normal / foliage mask fields
  float gtvbao_bent_diffuse_strength;
  float gtvbao_bent_diffuse_softness;
  float gtvbao_bent_specular_strength;
  float gtvbao_bent_specular_proxy_roughness;
  float gtvbao_bent_max_darkening;
  float gtvbao_bent_normals;
  float gtvbao_force_neutral_x;
  float gtvbao_debug_blackout;
  float gtvbao_ao_active_for_draw;
  float gtvbao_foliage_ao_blend;
  float gtvbao_foliage_mask_method;
  float gtvbao_mrt_normal_valid;
  float gtvbao_debug_mode;             // Kai GTVBAO debug mode (distinct from gtvbao_debug_view)
  float gtvbao_normal_input_mode;      // Kai: 0=off, 1=on (mirrors global g_gtvbao_normal_input_mode)
  // Kai char shadow / misc
  float char_shadow_strength;
  float foliage_debug_mode;
  float sss_dedicated_bound;
  float shadow_isfast_jitter_amount;
  float shadow_isfast_jitter_speed;
  float shadow_pcss_sample_mode;
  // Kai character VBGI debug/internal fields
  float char_gi_enabled;
  float char_gi_ao_influence;
  float char_gi_reject_strength;
  float char_gi_normal_reject;
  float char_gi_debug_mode;
  float char_gi_debug_scale;
  float char_gi_debug_chars_only;
  // Kai volfog fields
  float volfog_enabled;
  float volfog_tricubic_enabled;
  float volfog_is_fast_enabled;
  float isfast_noise_bound;
  float volfog_color_correction_strength;
  // Kai: GTVBAO VBGI consume Falcom SSGI
  float vbgi_kai_consume_falcom;       // 0=Off, 1=On — use Falcom SSGI to modulate GTVBAO VBGI
  float vbgi_kai_falcom_blend;         // [0..1], default 0.5 — how much Falcom SSGI modulates GTVBAO VBGI
  float vbgi_kai_gtvbao_only;          // 0=Off, 1=On — suppress Falcom SSGI output, GTVBAO VBGI only
  float shadow_edge_tint_kai;          // Kai-specific: 0=Off, 1=Improved (colored penumbra)
  float character_light_strength;      // [0..2], default 0 — scales chrLightIntensity_g hero light on characters
  // —— GTVBAO upgrade toggles ——
  float gtvbao_cdf_enabled;     // 0=Off, 1=On — CDF remap horizon angles
  float gtvbao_cosine_enabled;  // 0=Off, 1=On — cosine-weighted slice sampling
  float gtvbao_cosine_mode;     // 0=Weight, 1=Project, 2=CDF — cosine sampling method
  float gtvbao_thickness_enabled; // 0=Off, 1=On — per-sample thickness offset
  // —— Poisson denoiser ——
  float gtvbao_poisson_samples;     // [4..32], default 8 — Poisson disk sample count
  float gtvbao_poisson_luma_phi;    // [0.5..20], default 5 — luma/AO similarity falloff
  float gtvbao_poisson_depth_phi;   // [0.5..20], default 5 — depth similarity falloff
  float gtvbao_poisson_normal_phi;  // [0.5..20], default 5 — normal similarity falloff
  // —— Character GTVBAO / GTVBGI ——
  float char_gtvbao_mode;            // 0=Off, 1=On, 2=Combined
  float char_gtvbao_mask_strength;   // [0..1], 0=full AO on chars, 1=no AO on chars
  float char_gtvbgi_mask_strength;   // [0..1], 0=full GI on chars, 1=no GI on chars
  // —— GTVBAO pre-filter ——
  float gtvbao_prefilter_enabled;    // 0=Off, 1=On — depth-aware bilateral pre-filter on raw AO
  // —— BRDF Improvement ——
  float brdf_hammon_diffuse_enabled;       // 0=Off, 1=On
  float brdf_multiscatter_specular_enabled;// 0=Off, 1=On
  float brdf_diffuse_strength;             // [0..2] blend 0=vanilla → 1=Hammon
  float brdf_specular_strength;            // [0..2] blend 0=vanilla → 1=GGX+MS
  float brdf_roughness_min;                // [0..0.5] default 0.04
  float brdf_roughness_max;                // [0.5..1] default 1.0
  float brdf_f0_source;                    // reserved (0=specularColor)
  // —— GTVBAO foliage ——
  float gtvbao_exclude_foliage;            // 0=Off, 1=On — skip AO computation on foliage pixels
  float gtvbao_foliage_ao_value;           // [0..1] default 1.0 — AO value for excluded foliage (1=no occlusion)
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

#endif  // SRC_GAMES_FALCOMENGINE_PLUS_SHARED_H_
