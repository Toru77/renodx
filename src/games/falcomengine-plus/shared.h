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
  // The screen-space shadow RAYMARCH behind these (mode 2, "Bend_SSS") was
  // The Bend_SSS character ray-march (char_shadow_* below) and the environment
  // "Sun SSS" ray-march (env_sss_* below) have both been deleted from every
  // shader in this mod, daybreak2/char included, and their settings are gone.
  // The slots stay declared so that the packoffset of every later field is
  // unchanged: this block is a contiguous range, and removing a line from the
  // middle would shift the whole tail of the struct on both sides of the C++ /
  // HLSL boundary.
  //
  // The one survivor is char_shadow_mode itself, which no longer selects a
  // ray-march but picks the character shadowing technique:
  //   0 = off, 1 = the engine's own camera-facing march, 2 = our custom
  //   camera-facing contact march (char_cam_* at the tail of this struct).
  // In every mode except 0 the character/SSAO pass writes the character shadow
  // into the AO target's .z channel, which is why the lighting shaders gate their
  // read on `char_shadow_mode >= 0.5f` and not on `== 1`. That engine code is
  // still in the Sora and Kai character/SSAO shaders.
  float char_shadow_mode;
  float char_shadow_sample_count;   // RETIRED: Bend_SSS, unread
  float char_shadow_hard_shadow_samples;   // RETIRED: Bend_SSS, unread
  float char_shadow_fade_out_samples;       // RETIRED: Bend_SSS, unread
  float char_shadow_surface_thickness;      // RETIRED: Bend_SSS, unread
  float char_shadow_contrast;               // RETIRED: Bend_SSS, unread
  float char_shadow_light_screen_fade_start; // RETIRED: Bend_SSS, unread
  float char_shadow_light_screen_fade_end;   // RETIRED: Bend_SSS, unread
  float char_shadow_min_occluder_depth_scale; // RETIRED: Bend_SSS, unread
  float char_shadow_jitter_enabled;          // RETIRED: Bend_SSS, unread
  // Shadow type: 0 = Camera, 1 = World, 2 = Combined. RETIRED: Bend_SSS, unread.
  float char_shadow_type;
  // Per-pass strengths (0..1). RETIRED: Bend_SSS, unread.
  float char_shadow_camera_strength;
  float char_shadow_world_strength;

  // Environment "Sun SSS" ray-march. RETIRED across the whole mod; unread.
  float env_sss_enabled;
  float env_sss_strength;
  float env_sss_sample_count;
  float env_sss_hard_shadow_samples; // RETIRED, unread
  float env_sss_fade_out_samples;    // RETIRED, unread
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
  float env_sss_csm_gate;
  // Foliage / character mask debug view. Still live: view 1 is the mask the
  // GTVBAO character options act on, 2 the contact term, 3 the AO the micro
  // pass reads, 4 raw mrt0.z.
  float debug_show_env_sss;

  // —— Local Screen Space Shadows (Bend_SSS for point/spot lights) ——
  // 0/1, DEPRECATED and unread. These eleven slots were allocated for a
  // per-local-light screen-space shadow that was never implemented -- no .hlsl
  // file in this mod has ever read them. The feature is now covered properly by
  // the Contact Shadows local-light march (cs_contact_local_*), which reuses the
  // shared clip-space marcher instead of duplicating it per game. Kept in place
  // so the packoffset of every field after this one is unchanged.
  float local_sss_enabled;             // RETIRED: unread
  float local_sss_strength;            // RETIRED: unread
  float local_sss_light_type;          // RETIRED: unread
  float local_sss_sample_count;        // RETIRED: unread
  float local_sss_hard_shadow_samples; // RETIRED: unread
  float local_sss_fade_out_samples;    // RETIRED: unread
  float local_sss_surface_thickness;   // RETIRED: unread
  float local_sss_contrast;            // RETIRED: unread
  float local_sss_light_fade_start;    // RETIRED: unread
  float local_sss_light_fade_end;      // RETIRED: unread
  float local_sss_occluder_depth_scale;// RETIRED: unread

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
  float gtvbao_temporal_blend;       // DEPRECATED: always 0 (spatial only)
  float gtvbao_temporal_frame_count; // DEPRECATED: unused (spatial only)
  float gtvbao_disocclusion_threshold; // DEPRECATED: unused (spatial only)
  float gtvbao_noise_type;         // 0=IS-FAST, 1=IGN, 2=Hilbert — noise selection when IS-FAST on
  float gtvbao_debug_view;          // 0=Off, 1=AO gray, 2=GI only, 3=Bitmask viz
  float gtvbao_debug_logging;       // 0=Off, 1=On
  float gtvbao_dedicated_bound;     // 0/1 — set at runtime: t22 holds valid GTVBAO AO
  float gtvbao_fix_experimental;    // 0=Off, 1-5 — composite-side AO encoding diagnostic (sora1st/sora2nd lighting only)
  float gtvbao_vbgi_bound;          // 0/1 — set at runtime: t23 holds valid VBGI
  float gtvbao_vbgi_debug;          // 0=Off (add GI), 1=On (replace scene with GI)

  // —— VBGI (Visibility-Based Global Illumination) ——
  float vbgi_enabled;               // 0=Off, 1=On (requires gtvbao_mode >= 1)
  float vbgi_intensity;             // [0..5], default 1.0
  float vbgi_saturation;            // [0..2], default 1.0
  float vbgi_char_mask_strength;    // [0..1], default 0 — reduce VBGI on characters (0=off, 1=fully masked)
  float vbgi_multibounce;           // 0=Off, 1=On
  float vbgi_multibounce_strength;  // [0..10], default 1.0 — extra gain on the normalized bounce
  // Fraction of the local direct light re-emitted per bounce (a diffuse
  // albedo). The accumulate pass normalises the feedback against the pixel's
  // own light, so this is the fraction actually added regardless of how much
  // the gather attenuated the GI, instead of a blind multiplier over an
  // unknown and highly geometry-dependent gain.
  float vbgi_multibounce_bounce_fraction; // [0..0.5], default 0.15
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
  float shadow_filter_method;       // 0=Off (single sample), 1=Falcom (10-tap PCF), 2=CHSS
  float shadow_edge_tint;           // 0=Off, 1=Falcom (vanilla red tint), 2=Improved (vibrancy)
  // —— CHSS (Contact-Hardening Soft Shadows) + shared jitter ——
  float shadow_pcss_jitter_enabled;    // 0=Off, 1=On (also used by Kai PCSS)
  float shadow_pcss_jitter_amount;     // [0..1], default 1.0 — blend static→temporal
  float shadow_pcss_jitter_speed;      // [0..500], default 237.0 — temporal animation speed
  float shadow_chss_noise_mode;        // 0=IGN, 1=IS-FAST (spatio-temporal blue noise, requires ISFASTMasterEnable)
  float shadow_base_softness;          // [0..0.5], default 0.2 — constant penumbra offset
  float shadow_chss_search_radius;     // [0.1..2], default 1.0 — world-space softness (blocker search/PCF max)
  float shadow_chss_penumbra_scale;    // [1..200], default 80.0 — penumbra width multiplier
  float shadow_chss_depth_cap;         // [0.01..1], default 0.05 — max depth diff for penumbra
  float shadow_chss_min_radius;        // [0..100], default 0 — minimum filter radius in texels (0=off)
  float shadow_chss_post_blur;         // [0..100], default 0 — blur strength (0=off/passthrough, 100=full)
  float shadow_chss_blocker_count;     // [4..64], default 16 — blocker search sample count
  float shadow_chss_sample_count;      // [4..64], default 16 — PCF filter sample count
  float shadow_chss_onset_bias;        // [0..0.2], default 0 — added to receiver/blocker separation before penumbra
  float shadow_chss_penumbra_curve;    // [0.5..4], default 2 — penumbra response exponent
  // —— Colored Shadow Penumbra (Improved mode) ——
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
  float cubemap_improvements_enabled;  // UNUSED (Improved Cubemap removed): kept for b13 layout stability, always init 1
  float cubemap_lighting_mip_boost;    // UNUSED (Improved Cubemap removed): kept for b13 layout stability, always init 1.5
  float floor_cubemap_mip_scale;       // UNUSED (Improved Cubemap removed): kept for b13 layout stability, always init 4
  // SSGI (Falcom native, not GTVBAO)
  float ssgi_mod_enabled;              // 0=Off, 1=On
  float ssgi_color_boost;              // [0..3], default 1 — scales SSGI RGB before power shaping
  float ssgi_alpha_boost;              // [0..3], default 1 — scales SSGI alpha before saturate
  float ssgi_pow;                      // [0.1..3], default 1 — pow(abs(color), Power) bounce response
  // Depth of Field
  float dof_mode;                      // 0=Vanilla, 1=Improved
  float dof_strength;                  // [0..2], default 1 — overall blend for improved DOF
  float dof_radius_scale;              // [0.25..2.5], default 1.33 — blur radius from CoC
  float dof_sample_count;              // taps: 12/18/24/30 from the Quality tiers, default 24
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
  // RETIRED and unread: no .hlsl file has ever read this. The per-technique
  // character control the mod needs is cs_micro_char_strength / cs_contact_char_strength.
  // Kept so the packoffsets after it do not move.
  float char_shadow_strength;
  // Foliage/character mask debug views. View 2 now shows the Contact Shadow term
  // rather than the retired SSS shadow channel.
  float foliage_debug_mode;
  // DEPRECATED and unread: flagged "the ssao pass's dedicated SSS shadow texture
  // is bound". The raymarch that produced that texture is gone, so nothing sets
  // this and nothing reads it. Kept so later packoffsets do not move.
  float sss_dedicated_bound;
  float shadow_isfast_jitter_amount;
  float shadow_isfast_jitter_speed;
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
  // —— GTVBAO upgrade toggles: always On (UI toggles removed, hardcoded in push constants) ——
  float gtvbao_cdf_enabled;     // DEPRECATED: always 1
  float gtvbao_cosine_enabled;  // DEPRECATED: always 1
  float gtvbao_cosine_mode;     // 0=Weight, 1=Project, 2=CDF — cosine sampling method (retained)
  float gtvbao_thickness_enabled; // DEPRECATED: always 1
  // —— Character GTVBAO / GTVBGI ——
  float char_gtvbao_mode;            // 0=Off, 1=On, 2=Combined
  float char_gtvbao_mask_strength;   // [0..1], 0=full AO on chars, 1=no AO on chars
  float char_gtvbgi_mask_strength;   // [0..1], 0=full GI on chars, 1=no GI on chars
  // —— GTVBAO pre-filter ——
  float gtvbao_prefilter_enabled;    // 0=Off, 1=On — depth-aware bilateral pre-filter on raw AO
  // —— BRDF Improvement ——
  float brdf_hammon_diffuse_enabled;       // 0=Off, 1=On — sun + local lights
  float brdf_multiscatter_specular_enabled;// 0=Off, 1=On
  float brdf_diffuse_strength;             // [0..2] blend 0=vanilla → 1=Hammon
  float brdf_specular_strength;            // [0..2] blend 0=vanilla → 1=GGX+MS
  float brdf_specular_peak_clamp;          // [0..16] soft GGX highlight cap, 0=off
  float brdf_roughness_min;                // [0..0.5] default 0.04
  float brdf_roughness_max;                // [0.5..1] default 1.0
  float brdf_f0_source;                    // reserved (0=specularColor)
  // —— GTVBAO foliage ——
  float gtvbao_exclude_foliage;            // 0=Off, 1=On — skip AO computation on foliage pixels
  float gtvbao_foliage_ao_value;           // [0..1] default 1.0 — 1=keep normal AO, 0=no AO (fully bright)
  float gtvbao_foliage_channel_mode;       // 0=o1.w (Sora), 1=o1.z (Kai)
  // —— Foliage Grass AO ——
  float foliage_grass_ao_enabled;          // 0=Off, 1=On
  float foliage_grass_ao_base;             // AO at root [0..1] default 0.25
  float foliage_grass_ao_tip;              // AO at tip [0..1] default 1.0
  float foliage_grass_ao_curve;            // power exponent [0.1..2] default 0.5
  // —— DOF anti-starvation (thin-feature sharp-line fix) ——
  float dof_sign_softness;                 // [0..1], default 0.4 — opposite-layer tap acceptance (0=hard reject)
  float dof_coverage_enabled;              // 0 Off, 1 On — coverage-aware composite (blend by same-layer fraction)
  // —— Denoiser upgrades: temporal settings deprecated (spatial only), à-trous kept ——
  float gtvbao_atrous_enabled;             // 0 Off, 1 On — à-trous wavelet spatial filter (replaces bilateral chain)
  float gtvbao_atrous_depth_sigma;         // [0.05..4] default 1.0 — relative depth edge-stop strength
  float gtvbao_atrous_normal_sigma;        // [2..128] default 32 — normal edge-stop power
  float gtvbao_atrous_passes;              // [1..3] default 1 — Full-resolution wavelet iterations; stride i is 1<<i (radius 2, 4, 8 px)
  // —— Dynamic Cubemaps (Sora 2nd) — Phase 0A/B standalone ———
  float dynCube_enabled;                   // 0=Off (vanilla t17), 1=On (dynamic)
  float dynCube_debug;                     // 0=Off(normal) 1=ShowDynamicCube 2=FaceViz 3=SolidColors 4=NoOverride
  float dynCube_resolution;                // 0=64 1=96 2=128 (default 128)
  float dynCube_history;                   // 0=Off (Phase0B no history) 1=On (Phase1 lerp 0.5)
  float dynCube_ggx;                       // 0=HW mips (cheap approx) 1=GGX (Phase3)
  float dynCube_capture_interval;          // frames between capture starts (1=fastest 2-stage cadence)
  float dynCube_roughness_boost;           // placeholder for future mip boost
  float dynCube_debug_logging;             // 0=Off 1=On — 1 log/sec diag for which part is broken
  float dynCube_debug_face;                // 0..5 face for preview when debug=1/2/5/6
  float dynCube_debug_mip;                 // 0..7 mip for GGX filtered preview when debug=8
  float dynCube_force_mip;                 // debug: -1=normal roughness LOD, 0..7=force t17 mip
  float dynCube_reserved_0;
  float dynCube_reserved_1;
  float dynCube_reserved_2;
  float dynCube_reserved_3;
  float dynCube_reserved_4;
  float dynCube_reserved_5;
  float dynCube_ssr_enabled;                     // 0=off 1=on (simple screen-space SSR)
  float dynCube_ssr_samples;                     // SSR march sample count [4..96], default 16
  float dynCube_ssr_distance;                    // SSR search distance, world units [4..192], default 20
  float dynCube_ssr_blur;                        // SSR Gaussian blur sigma, 0=sharp
  float dynCube_ssr_symmetric_weights;           // 0=legacy blur tap loop (default), 1=symmetric-pair loop (A/B test)
  float dynCube_ssr_distance_fade;               // hit-distance confidence falloff, 0..1
  float dynCube_ssr_edge_fade;                   // screen-edge confidence falloff, 0..1
  float dynCube_ssr_grazing_fade;                // grazing-angle confidence falloff, 0..1
  float dynCube_ssr_thickness;                   // depth tolerance, default 0.1
  float dynCube_ssr_char_occ_strength;              // SSR confidence reduction for character-induced horizontal-surface disocclusion (0..1)
  float dynCube_ssr_char_occ_upness;                // surface upness threshold where the character-hit confidence reduction begins (0..1)
  float dynCube_vanilla_blur;                    // vanilla cubemap mip-offset blur, 0=sharp
  float dynCube_capture_boost;             // capture color boost multiplier, default 1.0
  float dynCube_history_blend;             // temporal lerp weight, default 0.5
  float dynCube_history_pos_threshold;     // world-unit position compatibility threshold, default 0.5
  float dynCube_character_capture;              // 0=exclude characters (default), 1=capture characters
  float dynCube_force_vanilla;                  // 0=bind dynamic cube to t17, 1=keep vanilla t17 (A/B)
  float dynCube_force_dynamic;                  // 0=off 1=on (debug: dynamic cubemap only)
  float dynCube_force_ssr;                      // 0=off 1=on (debug: SSR only)
  float dynCube_layer_mix;                      // -1=automatic confidence blend, 0..2=manual (0=SSR,1=Dynamic,2=Vanilla)
  float dynCube_blur;                           // artistic mip-offset blur on the dynamic cube sample (fractional), 0=sharp
  float dynCube_reserved_6;
  float dynCube_reserved_7;
  float dynCube_lookup_direction_flip;        // reserved: sampling convention is baked in capture now (bare lookup correct); kept for b13 layout stability, always 0
  float dynCube_coverage_fade;                 // 0=off 1=on (smooth binary validity edge in direction space)
  float dynCube_coverage_width;                // validity smoothing cone half-angle in degrees [0..8], default 2
  float dynCube_ssr_isfast_enabled;            // 0=off (hash phase), 1=on (IS-FAST phase when master+texture allow)
  float dynCube_ssr_isfast_strength;           // [0..1] blend hash phase -> noise phase, default 1
  float dynCube_ssr_isfast_spatial;            // [0.25..4] noise spatial scale, default 1
  float dynCube_ssr_isfast_temporal;           // [0..5] noise animation speed, 0=frozen slice, default 1
  float dynCube_ssr_confidence_fallback;       // [0..0.9] AUTO SSR confidence fallback, 0 = today's weighting, higher = low-conf SSR yields to Dynamic sooner
  float dynCube_vertical_offset;               // TEST: vertical tilt of dyn cube lookup, degrees; 0 = no-op, + = slide content down
  float dynCube_worldbox_contrib;
  float dynCube_reserved_8;
  float dynCube_reserved_9;
  float dynCube_reserved_10;
  float dynCube_reserved_11;
  float dynCube_reserved_12;
  float dynCube_capture_soften;              // [0..1] variant soften for global pushes (lighting unaffected), default 0
  float dynCube_global_strength;             // [0..1] variant strength for global pushes (lighting unaffected), default 1
  float dynCube_ssr_replacement;             // 0=vanilla Sora SSR passes, 1=replace ssr1/ssr2 with DynCube composite (appended last: do not insert above)
  float dynCube_ssr_replacement_debug;       // 0=off, 1=ssr1 input, 2=ssr1 march result, 3=lighting SSR texture, 4=lighting color (appended last: do not insert above)
  float dynCube_game_ssr;                    // 0=skip vanilla ssr1 march (water gets dynamic+vanilla only), 1=run it as composite input (appended last: do not insert above)
  float dynCube_vanilla_refine_fix;          // 0=verbatim vanilla ssr1 refine (forward-only), 1=Kai-style backtrack when inside (appended last: do not insert above)
  float dynCube_vanilla_refine_threshold;    // hit threshold on the refine depth delta (0=inside/behind surface, higher=stricter); used only when refine fix is on (appended last: do not insert above)
  float dynCube_vanilla_history_fixed;       // 1=fixed history weight from slider, 0=motion-adaptive (Kai formula) (appended last: do not insert above)
  float dynCube_vanilla_history_weight;      // fixed history fraction 0.5-0.99 (0.9=vanilla); used only when fixed mode on (appended last: do not insert above)
  float dynCube_vanilla_disoc_reject;        // 0=off, 1=validate reprojected history vs current scene, use current on mismatch (appended last: do not insert above)
  float dynCube_vanilla_disoc_depth;         // depth-mismatch reject threshold, view units 0-2 (appended last: do not insert above)
  float dynCube_vanilla_disoc_uv;            // reprojection-motion reject threshold, 0-1 UV 0-0.25 (appended last: do not insert above)
  float dynCube_vanilla_isfast;              // 0=off, 1=blue-noise subpixel history distribution (reuses SSR IS-FAST spatial/strength) (appended last: do not insert above)
  float dynCube_vanilla_isfast_frame;        // runtime: frame_index % 64, or -1 when noise unusable (appended last: do not insert above)
  float dynCube_vanilla_ssr_enabled;         // master switch for Vanilla SSR Improvements: 0=all off (vanilla code only), 1=section active (appended last: do not insert above)
  float gtvbao_optimization;               // RETIRED: GTVBAO shader-side optimizations are always on; slot kept for ShaderInjectData layout (appended last: do not insert above)
  float custom_shader_logging;             // 0=Off, 1=On - throttled step log (GTVBAO + DynCube + SSR) for crash diagnosis (appended last: do not insert above)
  float dynCube_sparkle_rejection;         // 0=off, 1=reject isolated HDR spikes at depth edges + non-finite input, no HDR caps (appended last: do not insert above)
  float char_outline_intensity;            // [0..1], 1=vanilla outline strength, applied before saturate (appended last: do not insert above)
  // —— Custom TAA (Sora 1st/2nd, from-scratch replacement; vanilla TAA files stay untouched) ——
  float custom_taa_enabled;                // 0=Off (game TAA runs), 1=On (custom replacement serves the TAA hashes)
  float custom_taa_history_filter;         // 0=Bilinear, 1=Adaptive high-order (edge-gated bicubic), 2=Full bicubic
  float custom_taa_clip_mode;              // 0=None, 1=RGB AABB, 2=k-DOP
  float custom_taa_kdop_axes;              // 0=16-DOP General subset (first 8 axes of paper 32-DOP set), 1=22-DOP Paper lightweight, 2=32-DOP Paper full
  float custom_taa_dmin;                   // adaptive gate threshold on edge D-terms (paper start point ~0.048, tune per game)
  float custom_taa_kdop_epsilon;           // slab extent padding for k-DOP clipping (paper default 1e-5)
  float custom_taa_static_feedback;        // TAA-5 static history weight [0..0.99], default 0.9
  float custom_taa_dynamic_feedback;       // TAA-5 moving history weight [0..0.99], default 0.5
  float custom_taa_motion_scale;           // TAA-5 pixels-to-weight scale: weight = saturate(motionPixels * scale)
  float custom_taa_debug;                  // 0=Normal, 1=History only, 2=Clip factor, 3=|Adaptive-Bilinear|x20, 4=|Full-Bilinear|x20 (dev/Advanced only)
  float custom_taa_history_valid;          // runtime: 0=first custom frame (output current, no accumulate), 1=accumulate
  float custom_taa_overshoot_softness;       // selective-tolerance experiment: fb *= 1/(1+(r/k)^2), r = normalized hull overshoot; k=100 ~= prior behavior
  float custom_taa_silhouette_rejection;     // wrong-surface experiment: disocc = 1 - motionWeight*saturate(spread*k); 0 = prior behavior
  float custom_taa_squared_motion_response;  // 0=linear motion weight (baseline), 1=squared (more history at low/moderate motion)
  float custom_taa_detail_restore;           // 0=off (exact prior behavior); test 8.0: restore fb toward static on detailed+agreeing pixels
  float custom_taa_detail_target;            // selective above-static window for high-detail/low-motion/agreeing pixels; == staticFb = off (exact prior)
  // —— RCAS post-TAA sharpening (dedicated pass; never feeds TAA history) ——
  // Always on: strength 0 is the off position (passthrough, zero dispatch).
  float rcas_strength;                       // [0..1], default 0.2 — linear lobe multiplier (0 = off/passthrough)
  float rcas_enabled;                        // 0=forced 0 sharpening (stage still runs: hist copy + t2 path intact), 1=strength applies; default On
  float rcas_denoise;                        // 0=off (reference default), 1=apply nz grain reduction (lobe *= nz, 0.5..1.0 range)
  // —— RCAS Stage 2 motion-adaptive sharpening (base * multiplier, clamped to max) ——
  float rcas_motion_on;                      // 0=base strength exactly (previous behavior), 1=per-pixel interpolation; default On
  float rcas_motion_multiplier;              // [1..10], default 2.0 — motion value = min(base * multiplier, max)
  float rcas_motion_max;                     // [0..3], default 1.0 — clamp for the motion value (may exceed 1)
  float rcas_motion_threshold;               // px [0..3], default 1.0 — motion below this adds nothing (jitter floor)
  float rcas_motion_range;                   // px [0.25..8], default 2.0 — additional motion to reach the target
  float rcas_motion_response;                // [0.5..3], default 1.0 — pow curvature of the transition
  float rcas_debug;                          // 0=normal sharpen, 1=motion-strength heatmap (green/yellow/red); default Normal
  // —— FXAA post-TAA (TAA -> FXAA -> RCAS; never feeds TAA history) ——
  float fxaa_enabled;                        // 0=off (passthrough: RCAS reads hist as before), 1=on; default On
  float fxaa_quality;                        // 0=Standard (preset 12, reference default), 1=High (preset 29); default High
  float fxaa_subpix;                         // [0..1], default 0.75 — sub-pixel aliasing removal (sharpness tradeoff)
  float fxaa_edge_threshold;                 // default 0.166 — local-contrast gate (reference: 0.333 faster .. 0.063 overkill)
  float fxaa_edge_threshold_min;             // default 0.0625 — dark trim (0 = process darks fully)
  // —— GTVBAO half-resolution spatial pipeline (appended last: do not insert above) ——
  float gtvbao_resolution;                   // 0=Full (existing behavior), 1=Half (half AO/GI + full-res reconstruction)
  float gtvbao_upscale_plane_sigma;          // reconstruction plane edge-stop sigma, default 40
  float gtvbao_upscale_normal_power;         // reconstruction normal weight power, default 16
  float gtvbao_upscale_debug;                // 0=Final, others=reconstruction diagnostics
  // —— DOF IS-FAST rotated gather (appended last: do not insert above) ——
  float dof_isfast_enabled;                  // 0/1 — per-pixel rotated bokeh gather (Improved mode)
  float dof_isfast_noise_frame;              // runtime: frame_index % 64, or -1 when noise unusable
  // —— Motion Blur (Guertin et al. 2013, Sora 2nd) ——
  // Deploy: 0=Off, 1=Cutscene Only (DoF draw), 2=Always On (tonemap draw).
  // Motion blur runs in UV space; every length below is a 1080p-equivalent
  // pixel value and is converted by motionblur/motion_blur_common.hlsli, so a
  // 4K player sees the same blur as a 1080p player.
  // 0=Off, 1=Cutscene Only, 2=Always On. Read on the CPU as the single gate for
  // the whole chain; no shader reads it.
  float mb_mode;
  float mb_intensity;               // [0..2], default 1 — shutter scale on the gather's
                                    // integration DOMAIN (not on velocity); 0 is an exact
                                    // no-op, and the domain is capped at one tile
  float mb_sample_count;            // 12/16/20/24, default 16 (Medium) - CEILING on
                                    // taps (paper N). Written by the addon from the
                                    // Quality preset; not bound to a setting, because
                                    // the shader wants a tap count, not a preset index.
  float mb_max_radius_px;           // [8..80], default 40 — paper r; also drives the
                                    // square tile size (2x this in pixels)
  float mb_center_weight_k;         // [1..100], default 40 — paper k (centre weight bias)
  float mb_jitter_h;                // [0..4], default 0.95 — paper h, integration domain extension
  float mb_min_velocity_g;          // [0..8], default 1.5 — paper g, vc blend threshold
  float mb_neighbor_t;              // [0..4], default 1 — paper t, Section 4.2 falloff in TILES
  float mb_depth_tolerance;         // [0.01..1], default 0.1 — zCompare soft band
  float mb_velocity_format;         // DEPRECATED, unread: the full-resolution velocity
                                    // intermediate was removed, so there is nothing left
                                    // to quantise. Kept so later packoffsets do not shift.
  float mb_jitter_source;           // 0=Halton (paper), 1=IS-FAST volume
  float mb_jitter_ready;            // runtime: 0/1 — IS-FAST volume actually usable
  float mb_debug_view;              // 0=Off, 1=Velocity, 2=NeighborMax, 3=Tile grid,
                                    // 4=Linear depth, 5=Blur amount, 6=Sample count,
                                    // 7=Half-res partition, 8=Camera velocity,
                                    // 9=Object residual
  // Runtime, written by the addon immediately before the b13 push.
  float mb_working_w, mb_working_h; // gather/velocity resolution (colour source dims)
  float mb_depth_w, mb_depth_h;     // game depth / linear depth texture dims
  float mb_motion_w, mb_motion_h;   // game motion texture dims
  float mb_tiles_x, mb_tiles_y;     // tile grid dims
  float mb_tile_uv;                 // paper r in UV = mb_max_radius_px / 1080
  float mb_pass;                    // TileMax axis selector: 0=X, 1=Y
  float mb_frame_index;             // IS-FAST volume slice
  float mb_motion_valid;            // 0/1 — 0 when the motion buffer is unavailable
  // Diagnostic chain selector, branched on the CPU (not read by any shader):
  // 0 = Full (everything), 1 = Prep Only (the three prep dispatches, no gather),
  // 2 = Gather Only (gather alone, reusing the previous frame's prep). Isolate
  // per-stage GPU cost. Gather Only reads a FROZEN tile grid, so it is not a
  // valid cost measurement while the scene is moving.
  float mb_debug_chain;
  // 0/1, default 1. When 0 the gather stops loading the motion texture per tap and
  // weights by the composite direction instead: ~25 fewer loads/pixel. Measured to
  // cost visible foreground bleeding, so it is treated as a quality feature and this
  // is kept only as a hook for a future optimisation pass.
  float mb_local_velocity_weights;
  // 0/1, default 1. When 0 the gather skips the per-tap depth fetch and both cone
  // terms, weighting by the cylinder term alone: one fewer load per tap. Costs
  // foreground/background separation, and note MBZCompare is scaled by
  // mb_depth_tolerance, so a very low tolerance already suppresses the cone terms
  // while still paying for the fetch.
  float mb_depth_test;
  // 0/1, default 0. Gathers at half resolution and bilinearly reconstructs to the
  // blit target. Roughly quarters the gather's pixel count, which is ~80% of the
  // moving-frame cost, at the price of blending four pixels before the depth test
  // sees them: edges soften and can ghost on high-contrast depth discontinuities.
  // Off by default for that reason.
  float mb_halfres;
  // 0/1, default 0. DEPRECATED, unread: an earlier half-res design had the gather
  // report "did this pixel blur" in its output alpha so a composite could blend
  // untouched pixels back at full resolution. The split-resolution design selects
  // per tile instead, so nothing reads alpha. Kept so later packoffsets do not move.
  float mb_output_mask;
  // 0/1, default 0. Which motion class this gather dispatch owns: 0 = short
  // motion at full resolution, 1 = long motion at half resolution. The gather
  // shader is dispatched twice with this flipped when Half Resolution is on, and
  // the routing test itself is bypassed when it is off, in which case a single
  // full-res dispatch owns all motion.
  float mb_gather_side;
  float mb_halfres_px;           // [0..24], default 10 — 1080-reference pixels of
                                  // streak at which a tile moves to half resolution.
                                  // Sits where the sample ladder jumps to 16 taps, so
                                  // the most expensive rungs take the cheap path.
  float mb_frame_rate_reference;  // [0..240], default 0 — framerate the motion is
                                  // normalised TO, in fps. 0 = UE's
                                  // r.MotionBlurTargetFPS 0: track the measured
                                  // frame time with a 0.1 moving average, so the
                                  // shutter spans the real frame and the scale
                                  // converges to 1.0 at any steady rate.
  float mb_frame_scale;           // runtime: targetDt / dt, clamped to [0.25..4].
                                  // Motion vectors are per-frame displacements, so
                                  // without this the streak, the ladder buckets, the
                                  // early-out and the half-res split would all
                                  // change with framerate. Applying it once,
                                  // in MBGameMotionToUV, puts all of them into
                                  // reference-frame units together. 1.0 = no-op.
  // ---- camera term (appended last) ----
  // The object channel is gone; the streak LENGTH is now the camera's share of
  // the total magnitude, and that ratio is a compile-time constant in
  // motion_blur_resolve (gain = camLen / (camLen + objLen)). It used to arrive
  // here as two weights and
  //     out = game * (camLen*w_cam + objLen*w_obj) / (camLen + objLen)
  // with the sum evaluated per pixel. RETIRED, unread by any shader and now
  // written by nothing. Kept so later packoffsets do not move.
  float mb_reserved_length;
  // ---- convention probe (appended last) ----
  // The game's motion encoding is documented inconsistently in this repo:
  // motion_blur_common.hlsli says prevPixel - curPixel, while the decompiled
  // writer (staticfoliage_0xF1EC53A8:201-207) reads as cur - prev because of the
  // order of its arithmetic. Its v5/v6 varyings are never labelled, so which is
  // the previous clip position is an INFERENCE, and getting it backwards negates
  // the whole camera term. These two exist so that can be determined from the
  // image rather than argued: the correct combination is the one that renders
  // Debug View "Object Residual" black on a static scene.
  float mb_camera_sign;    // +1 = cur - prev, -1 = prev - cur. Default 1.
  float mb_camera_jitter;  // 1 = add jitterDiff_g, 0 = omit. Default 1.
  // ---- camera-cut rejection ----
  // 0/1 toggle. When on, motion_blur_resolve compares the current view-projection
  // against prevViewProj_g and writes ZERO velocity for that frame if the implied
  // screen displacement is implausibly large, which is what stops a teleport or a
  // cutscene transition from smearing the whole frame toward the warp point.
  // Separate from a threshold so it can be switched off wholesale for A/B.
  float mb_camera_cut;
  // [8..512], default 120 — that displacement limit in 1080-reference pixels,
  // measured at a nominal depth. A fast camera whip is roughly 30-80 px/frame at
  // 60 fps; a scene cut is hundreds. Read only when mb_camera_cut is on.
   float mb_camera_cut_px;
   // ---- DoF adaptive sampling (appended last: do not insert above) ----
   // 0/1 toggle, default 1. When on, the bokeh gather picks each pixel's tap
   // count from its OWN blur radius instead of using dof_sample_count
   // everywhere, and dof_sample_count becomes the CEILING rather than the count.
   // When off, the gather is identical to the pre-adaptive algorithm.
   //
   // Appending here rather than beside the other dof_* fields is deliberate:
   // this struct is pushed to every shader in the addon, so a mid-struct insert
   // would move the packoffset of everything after it.
   float dof_adaptive_samples;
   // 0=Off, 1=Tap Count. The tap-count view colours every pixel by the rung the
   // ladder gave it, and BLACK where the pixel early-outs and costs no taps at
   // all -- a different state from the 4-tap rung, and the distinction is the
   // whole point of the view. Read only when dof_adaptive_samples is on.
   float dof_debug_view;
   // ---- GTVBAO MRT normal / horizon diagnostics (appended last) ----
   // 0/1, set at runtime (not a user setting). The GTVBAO main pass writes its
   // MRT/horizon diagnostic views into the debug UAV, which the host pushes to
   // t23 for the lighting shader to display. That display is gated on this flag
   // rather than on vbgi_debug_view, because that field is a persistent user
   // setting (SSGIDebugView) and reusing it would clobber the user's choice --
   // which is why the earlier 22-25 views pushed the texture but rendered the
   // fallback instead. Set only while the push is active; reset every frame.
  float gtvbao_mrt_normal_debug;
  // ---- Contact Shadows + Micro Shadows compute passes (appended last: do not insert above) ----
  // Two independent screen-space shadow techniques computed in shadows/*.cs_5_0.hlsl
  // and consumed by the lighting shaders. They supersede the inline screen-space
  // shadow raymarchs (char_shadow_* / env_sss_* / local_sss_* above, now retired).
  //
  // Micro Shadows: an AO-driven aperture term (Uncharted 4) applied to NdotL.
  // Contact Shadows: a clip-space depth march toward the light, jittered with
  // IS-FAST blue noise and resolved by the game's TAA.
  //
  // The two passes are separate because they need different inputs and have very
  // different cost: micro is a handful of ALU ops, contact is a raymarch.

  // Micro Shadows
  float cs_micro_enabled;           // 0=Off, 1=On
  float cs_micro_strength;          // [0..1] global blend of the micro term
  float cs_micro_env_strength;      // [0..1] strength on non-character pixels
  float cs_micro_char_strength;     // [0..1] strength on character pixels
  float cs_micro_opacity;           // [0..1] lerp(1, microshadow, opacity) from the paper
  float cs_micro_aperture_scale;    // [0..2], default 1 — scales the 2*AO*AO aperture
  float cs_micro_debug;             // 0=Off, 1=raw micro term, 2=diagnostic channels
  // AO source for the micro pass: 0 = the game's deferred AO (a float channel),
  // 1 = the GTVBAO AO (visibility in byte 0 of an r32_uint). Both are bound every
  // frame and the shader decodes each in its own encoding, because reading either
  // as the wrong type yields plausible garbage rather than an error.
  // 1 is the default: the game AO capture is not reliably available every frame,
  // and with AO = 1 the micro term saturate(NdotL + 2*AO*AO - 1) is identically
  // 1 at every pixel, i.e. the effect does nothing at all.
  float cs_micro_ao_source;

  // Contact Shadows
  float cs_contact_enabled;         // 0=Off, 1=On
  float cs_contact_strength;        // [0..1] global blend of the contact term
  float cs_contact_env_strength;    // [0..1] strength on non-character pixels
  float cs_contact_char_strength;   // [0..1] strength on character pixels
  float cs_contact_sample_count;    // march steps; quality tiers 4/8/12/16, default 8
                                     // (Medium). 8 is the count the reference settled on
                                     // (same count UE uses).
  float cs_contact_ray_length;      // world units the ray travels [1..200], default 50
  float cs_contact_thickness;       // [0.001..4] world-space occluder thickness
  float cs_contact_bias;            // [0..0.2] minimum penetration before a hit counts
  float cs_contact_normal_bias;     // [0..1] world units to lift the origin along N
  // Early-out for the sky: a device depth linearizing beyond this is treated as
  // no geometry, so the march is skipped. Lower = cheaper, higher = less sky.
  float cs_contact_sky_depth;       // world units [100..1e6]
  float cs_contact_max_darkening;   // [0..1] floor on the contact term (0 = full black)
  float cs_contact_isfast_enabled;  // 0=Off, 1=On — IS-FAST jitter (no IGN fallback by
                                    // design: without it the march is unjittered)
  float cs_contact_debug;           // 0=Off, 1=raw contact term (white = lit)
  // Local (point/spot) contact shadows, evaluated inside the dynamic light loop.
  // Off by default: it is per-light work in a pixel shader, so the cost scales
  // with the per-pixel light count rather than being one full-screen pass.
  float cs_contact_local_enabled;      // 0=Off, 1=On
  float cs_contact_local_strength;     // [0..1]
  float cs_contact_local_sample_count;  // march steps per local light; quality tiers
                                        // 4/8/12/16, default 8 (Medium), matching the sun
  float cs_contact_local_ray_length;   // world units [0.1..20], default 2
  float cs_contact_local_max_lights;   // per-pixel light budget [1..16], default 4

  // Runtime, written by the addon immediately before the b13 push.
  float cs_noise_frame;            // IS-FAST volume slice (frame_index % 32), or -1 when
                                   // the volume is unusable and the caller must not sample
  float cs_working_w, cs_working_h; // shadow output / depth texture dims
  float cs_ao_bound;               // 0/1 — a usable AO capture was bound to the micro pass
  float cs_micro_dedicated_bound;  // 0/1 — t33 holds a valid micro term this frame
  float cs_contact_dedicated_bound;// 0/1 — t34 holds a valid contact term this frame
  // IS-FAST dither for the micro pass. Appended last, and OFF by default.
  //
  // The micro term is a closed-form function of NdotL and AO with no ray to march,
  // so there is no sample position to jitter. What this actually dithers is the AO
  // QUANTISATION: GTVBAO stores visibility in a single byte, so a slowly varying
  // surface steps through discrete AO cells and the aperture term turns that into
  // visible banding. Offsetting the AO read by the blue-noise value walks
  // neighbouring cells per pixel and lets TAA average them back out.
  //
  // Off by default because it trades visible noise for that, and most scenes do
  // not band. On for users who see stepping on large smooth surfaces under GTVBAO.
  float cs_micro_isfast_enabled;
  // Manual override for the sun-contact range gate, in world units. 0 = derive the
  // cutoff from the engine's own last-cascade split. The override exists because
  // that split is only read in-shader by Kai and Kai soft; the Sora shaders
  // declare the same global but never use it, so which component holds the
  // outermost split is an inference there and may need correcting per title.
  float cs_contact_sun_range;
  // Strength of the contact term, relative to the ORIGINAL 4-sample appearance.
  // Not a physical unit and not derived from geometry: 4 is the sample count whose
  // average response the pass is being asked to reproduce, so that raising Sample
  // Count buys noise reduction without also changing how dark the contact reads.
  // At Sample Count 4 with this at 4 the estimator is exactly the old binary
  // any-hit test, value for value, which is what makes it a safe default.
  float cs_contact_response_scale;
  // Which of the four local light loops the contact march runs on. All four
  // march identically -- each derives its direction from lightPos - worldPos --
  // so this is a cost/appearance selection, not a different technique. The spot
  // cone is not tested; the light's own angular attenuation already handles
  // that, and gating the march by it would only re-apply a term the engine has
  // computed.
  //
  // The engine evaluates each light class twice: once on the specular-enabled
  // material path (cs_contact_local_point / cs_contact_local_spot) and once on
  // the diffuse-only path whose specular comes from the environment cube
  // (cs_contact_local_env_point / cs_contact_local_env_spot).
  //
  // A disabled loop marches zero times and therefore spends NO budget, so the
  // loops left on get the whole Local Light Budget rather than sharing it with
  // loops that are gated off.
  float cs_contact_local_point;
  float cs_contact_local_spot;
  float cs_contact_local_env_point;
  float cs_contact_local_env_spot;

  // —— Character Shadowing: the custom character contact pass ——
  //
  // This does NOT extend the cs_contact_* block above, and it is not the same
  // feature. That block drives the full-screen compute pass, whose terms reach
  // characters only where the lighting shader's main composite runs -- and
  // characters leave before that composite in every one of the four lighting
  // shaders (kai/lighting_0x430ED091, lightingsoft_0xF6C55E5F,
  // sora1st/lighting_0xFDAAF80E, sora2nd/lighting_0xCA3D8596). So the compute
  // pass's character half has always been dead, and this pass is what actually
  // shadows a character.
  //
  // It runs in the character/SSAO pixel pass, along the engine's own camera-facing
  // axis (rayMarchShadowDir_g, normalised -- see FalcomCameraFacingAxis), and writes
  // the same AO-target .z channel the vanilla march wrote, so the lighting shaders are
  // unchanged and the engine's own temporal blend resolves the jitter for free.
  //
  // There was also a sun-facing character march here, in the lighting shader at the
  // character early return. It is gone. It could not be made to look right, and the
  // reason is structural rather than a matter of tuning -- see the note on
  // char_cam_ray_length for the same failure in miniature. Do not reintroduce it
  // without reading that note first.

  // Master mode, in char_shadow_mode above: 0 = Off, 1 = Vanilla, 2 = Custom.
  // Every field below is read only in mode 2, and the mode is what decides whether
  // the lighting shaders consume the AO .z channel at all.

  // Camera pass.
  float char_cam_enabled;             // 0/1 — run the camera-facing march
  float char_cam_strength;            // [0..1] blend of the resulting term
  float char_cam_sample_count;        // [1..32] march steps
  // World-space march length, in the same units as cs_contact_ray_length. 1 unit is
  // about 1 metre and a human head is ~0.18, so this is a SILHOUETTE reach and the
  // engine's own march is in the same neighbourhood. It is not, and cannot be, a real
  // contact shadow: this axis is the camera axis, so the ray always heads into the
  // receiver's own surface and a screen-space depth test cannot tell that from an
  // occluder. The engine's vanilla march has the identical degeneracy and survives it
  // only because it is a binary any-hit that the AO channel's temporal blend smooths.
  // Read this pass as a softened depth-gradient term, not a shadow.
  float char_cam_ray_length;
  // Assumed occluder thickness. MUST stay comparable to the ray length: a band much
  // larger than the ray makes the whole ray either in or out of the band, which
  // collapses the coverage estimator back to a binary any-hit test and is why the
  // first version of this pass had no usable Sample Count at all.
  float char_cam_thickness;
  // Extra penetration required beyond the normal lift, folded into the lift by the
  // shader (bias = lift + this) so the receiver can never occlude itself.
  float char_cam_bias;
  // Lift off the surface along the normal, before the shader folds it into the bias.
  // About a millimetre: this ray is 2 cm long, so a lift comparable to the ray would
  // start the march in free air beside the face.
  float char_cam_normal_bias;
  float char_cam_response_scale;      // see cs_contact_response_scale
  float char_cam_max_darkening;       // [0..1] cap on how dark a shadowed pixel gets
  float char_cam_isfast_enabled;      // 0/1 — IS-FAST blue-noise dither of the step offset
  // 0 = Off, 1 = Raw Term, 2 = Coverage, 3 = Axis Length. Axis Length shows
  // |rayMarchShadowDir_g| on a log scale, because nothing in the engine pins whether
  // that symbol is a unit direction (Daybreak 2's 39-step march reads as if it is) or
  // a pre-scaled vector (the Sora/Kai 10-step march needs it to be), and the Ray
  // Length default should be set against a measured number rather than a guess.
  float char_cam_debug;
  // The struct must end on a float4 boundary: the host declares this range as a D3D
  // constant buffer of sizeof(ShaderInjectData) bytes (CreateShadowsPipelinesIfNeeded
  // and RunShadows) while fxc sizes CB13 from the same struct, so a size that is not a
  // whole number of float4s makes the shader's own declaration and the host's range
  // disagree. It is 440 floats = 110 float4s = 1760 bytes, which already satisfies that,
  // so there is no trailing pad any more. Adding a field means the count has to stay a
  // multiple of 4: trade it against another float rather than appending past the boundary.
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
