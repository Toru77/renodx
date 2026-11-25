#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include "../rtxdi/rtxdi_debug_layout.h"
#include "../world_state.hpp"
#include <deps/imgui/imgui.h>
namespace falcom_world::pt {
inline float g_pt_enabled = 0.f;
inline float g_pt_regir_enabled = 0.f;
inline bool PtRequested() { return g_pt_enabled > 0.5f; }
inline float g_pt_sparse_reflections = 1.f;
inline float g_pt_sparse_enabled = 0.f, g_pt_sparse_adaptive = 1.f, g_pt_sparse_rate = 0.10f,
             g_pt_sparse_exponent = 0.91f, g_pt_sparse_quality = 0.30f, g_pt_sparse_max_rate = 0.75f;
enum class PtSparseDenseReason : uint32_t { None, Disabled, RateAboveMax, FullRate, RrNotRequested, RrNotActive, NoiseUnavailable, ReflectionsDense };
struct PtSparsePolicy { uint32_t levels; PtSparseDenseReason dense_reason; };
inline const char* PtSparseDenseReasonName(PtSparseDenseReason reason) {
  switch (reason) {
    case PtSparseDenseReason::None: return "none";
    case PtSparseDenseReason::Disabled: return "disabled";
    case PtSparseDenseReason::RateAboveMax: return "rate_above_max";
    case PtSparseDenseReason::FullRate: return "full_rate";
    case PtSparseDenseReason::RrNotRequested: return "rr_not_requested";
    case PtSparseDenseReason::RrNotActive: return "rr_not_active";
    case PtSparseDenseReason::NoiseUnavailable: return "noise_unavailable";
    case PtSparseDenseReason::ReflectionsDense: return "reflections dense";
    default: return "unknown";
  }
}
inline PtSparsePolicy ComputePtSparsePolicy(bool enabled, bool adaptive, float base, float exponent, float quality,
                                            float max_rate, uint32_t width, uint32_t height) {
  if (!enabled) return {256u, PtSparseDenseReason::Disabled};
  base = std::clamp(std::isfinite(base) ? base : 0.10f, 0.0625f, 1.f);
  exponent = std::clamp(std::isfinite(exponent) ? exponent : 0.91f, 0.f, 1.f);
  quality = std::clamp(std::isfinite(quality) ? quality : 0.30f, 0.f, 1.f);
  max_rate = std::clamp(std::isfinite(max_rate) ? max_rate : 0.75f, 0.0625f, 1.f);
  double rate = base;
  if (adaptive) {
    const double pixels = std::max(static_cast<double>(width) * height, 1.0);
    const double scaled = std::clamp(rate * std::pow((2227.0 * 1238.0) / pixels, exponent), 0.0625, 1.0);
    const double bound = (quality >= 0.5f ? 1.0 : 0.0625);
    rate = scaled * std::pow(bound / scaled, std::abs(2.0 * quality - 1.0));
  }
  const uint32_t levels = std::clamp(static_cast<uint32_t>(std::round(rate * 256.0)), 1u, 256u);
  return {levels, rate > max_rate ? PtSparseDenseReason::RateAboveMax
                                  : (levels == 256u ? PtSparseDenseReason::FullRate : PtSparseDenseReason::None)};
}
inline float g_pt_sharc_enabled = 1.f, g_pt_sharc_accumulation_frames = 32.f, g_pt_sharc_stale_frames = 64.f,
             g_pt_sharc_scene_scale = 15.f, g_pt_sharc_downscale = 5.f, g_pt_sharc_capacity_log2 = 22.f;
inline float g_pt_sharc_radiance_scale = 100000.f;
inline float g_pt_sharc_min_bounce = 1.f;
inline float g_pt_debug_channel = static_cast<float>(PT_DEBUG_CHANNEL_UCW);  // session-only PT ReSTIR view channel
inline uint32_t g_pt_sharc_reset_request = 0u;  // bumped by the Clear SHARC Cache button
inline constexpr uint32_t kPtSharcGapFrames = 5u;           // frames without a SHARC trace that clear it (addon.cpp:539 uses 5)
inline constexpr uint32_t kPtSharcMaxUpdatePaths = 82944u;  // OpenRemix options.h:346 (1920x1080 at downscale 5)
inline constexpr uint32_t kPtSharcAccumulationStride = 32u; // OpenRemix sharc_policy.h:9
inline constexpr uint32_t kPtSharcResolvedStride = 24u;     // OpenRemix sharc_policy.h:10
inline float g_pt_material_enabled = 1.f, g_pt_material_rate = 4.f, g_pt_material_cell_scale = 80.f,
             g_pt_material_capacity_log2 = 22.f, g_pt_char_roughness = 0.5f, g_pt_char_f0 = 0.04f;
inline constexpr uint32_t kPtMaterialValueStride = 16u;
inline float g_pt_samples = 1.f, g_pt_max_bounces = 16.f, g_pt_russian_roulette = 1.f,
             g_pt_roulette_min_bounce = 1.f, g_pt_intensity = 1.f, g_pt_char_intensity = 1.f, g_pt_sky_intensity = 1.f,
             g_pt_ray_max = 500.f, g_pt_env_bias = 0.02f, g_pt_char_bias = 0.02f, g_pt_lamp_clearance = 0.2f, g_pt_sun_casters = 0.f;
inline float g_pt_reflection_enabled = 1.f, g_pt_reflection_intensity = 1.f,
             g_pt_reflection_roughness_scale = 0.f;
inline float g_pt_reflection_sample_clamp = 1.f;
inline float g_pt_reflection_clamp_enabled = 1.f;
inline float g_pt_gi_clamp_enabled = 1.f, g_pt_gi_clamp = 1.f, g_pt_gi_clamp_mode = 0.f;
inline float g_pt_sharc_firefly_clamp_enabled = 1.f, g_pt_sharc_firefly_clamp = 10.f;
inline float g_pt_reflection_cap_to_one = 1.f;
inline float g_pt_reflection_use_sharc = 1.f, g_pt_reflection_footprint = 1.f;
inline float g_pt_reflection_water_rule = 0.f;
inline float g_pt_reflection_water_surface = 1.f;
inline float g_pt_reflection_water_roughness = 0.f;
inline float g_restir_direct = 1.f;
inline float g_restir_direct_clamp_enabled = 1.f, g_restir_direct_clamp = 16.f;
inline float g_pt_lamp_wrap_scale = 1.f, g_pt_lamp_intensity = 1.5f, g_pt_lamp_material_scale = 1.f;
inline float g_pt_sun_toon = 1.f, g_pt_sun_intensity = 1.f, g_pt_sun_bands = 0.f, g_pt_sun_fill = 1.f,
             g_pt_sun_specular = 1.f;
inline float g_pt_lamp_toon = 1.f, g_pt_lamp_bands = 0.f;
inline float g_pt_char_sun_toon = 0.f, g_pt_char_lamp_toon = 0.f, g_pt_char_fill = 1.f;
inline float g_pt_char_terminator_fade = 1.f;
inline float g_pt_char_cast_shadow = 0.4f;
inline float g_pt_ao_rays = 8.f, g_pt_ao_radius = 0.20f, g_pt_ao_strength = 0.5f;
inline float g_pt_foliage_ao_strength = 0.f;
inline float g_pt_char_self_ao_strength = 0.5f, g_pt_char_self_ao_gap = 0.020f, g_pt_char_self_ao_radius = 0.05f;
inline float g_restir_game_sun_shadows = 1.f;
inline float g_restir_char_interior_sun_gate = 0.f;
inline float g_restir_max_rc_vertex_length = 5.f, g_restir_nee_local_samples = 1.f;
inline float g_restir_min_connection_footprint = 0.02f, g_restir_min_connection_footprint_sigma = 0.2f;
inline float g_restir_min_pdf_roughness = 0.1f, g_restir_min_pdf_roughness_sigma = 0.01f;
inline float g_restir_roughness_threshold = 0.1f, g_restir_distance_threshold = 0.f;
inline float g_restir_reconnection_mode = 1.f;
inline float g_restir_resampling = 3.f;
inline float g_restir_temporal_visibility = 1.f;
inline float g_restir_temporal_depth_threshold = 0.1f, g_restir_temporal_normal_threshold = 0.6f,
             g_restir_permutation_sampling = 1.f;
inline float g_restir_spatial_samples = 1.f, g_restir_disocclusion_boost_samples = 8.f, g_restir_spatial_radius = 32.f,
             g_restir_spatial_depth_threshold = 0.1f, g_restir_spatial_normal_threshold = 0.6f;
inline float g_restir_boiling_filter = 1.f, g_restir_boiling_filter_strength = 0.2f;
inline float g_restir_compatibility_neighbors = 1.f, g_restir_decorrelation_factor = 1.f;
inline float g_restir_stable_light_remap = 1.f;
enum class PtReason : uint32_t { Off, DeferredContext, InputNotReady, DxrOff, NotCreated, NoShaderModel66,
                                 Failed, SceneMissing, Stuck, SubmitFailed, TargetFailed, Tracing, Count };
inline const char* PtReasonName(PtReason r) {
  switch (r) {
    case PtReason::Off: return "off";
    case PtReason::DeferredContext: return "deferred_context";
    case PtReason::InputNotReady: return "input_not_ready";
    case PtReason::DxrOff: return "dxr_off";
    case PtReason::NotCreated: return "not_created";
    case PtReason::NoShaderModel66: return "no_shader_model_6_6";
    case PtReason::Failed: return "failed";
    case PtReason::SceneMissing: return "scene_missing";
    case PtReason::Stuck: return "stuck";
    case PtReason::SubmitFailed: return "submit_failed";
    case PtReason::TargetFailed: return "target_failed";
    case PtReason::Tracing: return "tracing";
    default: return "unknown";
  }
}
struct PtPanel {
  PtReason reason = PtReason::Off;
  PtSparsePolicy sparse_policy = {256u, PtSparseDenseReason::Disabled};
  bool sparse_requested = false, sparse_adaptive = false;
  bool sparse_reflections_requested = false, sparse_reflections_effective = false;
  uint64_t frames[static_cast<uint32_t>(PtReason::Count)] = {};
  uint64_t waits = 0u;
  float wait_ms_max = 0.f;
  bool sharc_on = false;
  uint32_t sharc_capacity_log2 = 0u, sharc_downscale = 0u, sharc_window = 0u;
  uint64_t sharc_clears = 0u;
  const char* sharc_clear_reason = "none";
  bool mat_on = false;
  uint32_t mat_capacity_log2 = 0u, mat_rate = 0u;
  uint64_t mat_clears = 0u;
  const char* mat_clear_reason = "none";
  bool refl_on = false;
  bool regir_requested = false, regir_active = false;
  const char* regir_fallback_reason = "off";
  uint32_t regir_frame = UINT32_MAX, regir_transfer_slot = UINT32_MAX, regir_light_count = 0u;
  uint64_t regir_owner_generation = 0u, regir_transfer_configuration = 0u;
  uint64_t regir_source_fence = 0u, regir_build_fence = 0u, regir_last_use_fence = 0u;
  bool regir_counts_available = false;
  uint32_t regir_counts_frame = 0u, regir_cell_count = 0u, regir_rejected_count = 0u, regir_overflow_count = 0u, regir_valid_count = 0u;
  uint64_t regir_counts_fence = 0u, regir_counts_generation = 0u;
  bool water_on = false;
  uint32_t gpu_water_pixels = 0u, gpu_water_traces = 0u;
  const char* water_reason = "off";
  uint32_t gpu_exact = 0u, gpu_hits = 0u, gpu_misses = 0u, gpu_lock_drops = 0u, gpu_writes = 0u;
  uint32_t regir_eligible = 0u, regir_lookups = 0u, regir_matches = 0u, regir_misses = 0u;
  uint32_t regir_invalid_headers = 0u, regir_invalid_slots = 0u, regir_selections = 0u;
  uint32_t gpu_initial_valid = 0u, gpu_path_len2 = 0u, gpu_path_len3plus = 0u, gpu_nee_connected = 0u;
  uint32_t gpu_sharc_terminated = 0u, gpu_sharc_terminal_rc = 0u, gpu_sharc_replay_rejected = 0u;
  uint32_t gpu_d2_lamp = 0u, gpu_d2_sun = 0u, gpu_shade_transmitted = 0u;
  uint32_t gpu_ao_rays = 0u, gpu_ao_occluded = 0u;
  uint32_t gpu_energy_len2 = 0u, gpu_energy_len3 = 0u, gpu_energy_len4_5 = 0u, gpu_energy_len6plus = 0u;
  uint32_t gpu_sharc_first_hit_cached_lum = 0u, gpu_first_hit_traced_lum = 0u;  // recorded only in the PT SHARC view
  uint32_t debug_view = RTXDI_DEBUG_VIEW_OFF, debug_channel = PT_DEBUG_CHANNEL_UCW;
  uint32_t temporal_selected = 0u, spatial_selected = 0u;
  uint32_t water_temporal_selected = 0u, water_spatial_selected = 0u;
  bool history_valid = false;
  const char* history_reason = "none";
  bool water_history_valid = false;
  const char* water_history_reason = "none";
  uint32_t regir_fallback_disabled = 0u, regir_fallback_key = 0u, regir_fallback_miss = 0u;
  uint32_t regir_fallback_header = 0u, regir_fallback_slots = 0u;
  uint32_t regir_water_eligible = 0u, regir_water_lookups = 0u, regir_water_matches = 0u, regir_water_misses = 0u;
  uint32_t regir_water_invalid_headers = 0u, regir_water_invalid_slots = 0u, regir_water_selections = 0u;
  uint32_t regir_water_fallback_disabled = 0u, regir_water_fallback_key = 0u, regir_water_fallback_miss = 0u;
  uint32_t regir_water_fallback_header = 0u, regir_water_fallback_slots = 0u;
  bool regir_water_counts_available = false;
  uint32_t regir_water_counts_frame = 0u, regir_water_submission_frame = UINT32_MAX;
  uint64_t regir_water_counts_owner_generation = 0u, regir_water_submission_owner_generation = 0u;
  uint32_t regir_water_pending_frame[3] = {};
  uint64_t regir_water_pending_owner_generation[3] = {};
  uint32_t regir_water_pending_owner[3] = {};
  float pt_interval_last_ms[2][4] = {}, pt_interval_average_ms[2][4] = {};
  uint8_t pt_interval_valid[2] = {};
  uint32_t sparse_selected = 0u, sparse_valid = 0u;
  uint64_t gpu_frame = 0u;
  bool gpu_available = false, sparse_sample_active = false;
  uint32_t lamps = 0u, width = 0u, height = 0u;
  uint32_t resampling_mode = 0u;
  bool direct_enabled = false;
  uint32_t light_table_status = 0u;
};
inline std::mutex g_pt_panel_mutex;
inline PtPanel g_pt_panel;
inline constexpr uint32_t kPtEventCapacity = 64u;
inline constexpr size_t kPtEventTextMax = 160u;
struct PtEvent {
  uint64_t frame = 0u;
  std::string text;
};
inline std::mutex g_pt_event_mutex;  // lock order: g_pt_panel_mutex, then g_pt_event_mutex
inline PtEvent g_pt_events[kPtEventCapacity];
inline uint32_t g_pt_event_next = 0u, g_pt_event_size = 0u;
inline uint64_t g_pt_event_overwritten = 0u;
inline std::atomic<uint64_t> g_pt_capture_state{0};  // bit 0: enabled; upper bits: epoch, +2 per change
inline float g_pt_debug_logging = 0.f;  // Settings binding only; render thread reads g_pt_capture_state
inline void RecordPtEvent(uint64_t frame, const std::string& text, uint64_t capture_state) {
  std::lock_guard lock(g_pt_event_mutex);
  if ((capture_state & 1u) == 0u || capture_state != g_pt_capture_state.load()) return;
  if (g_pt_event_size == kPtEventCapacity) {
    ++g_pt_event_overwritten;
  } else {
    ++g_pt_event_size;
  }
  PtEvent& slot = g_pt_events[g_pt_event_next];
  slot.frame = frame;
  slot.text = text.substr(0u, kPtEventTextMax);
  g_pt_event_next = (g_pt_event_next + 1u) % kPtEventCapacity;
}
inline void SetPtReason(PtReason r) {
  std::lock_guard lock(g_pt_panel_mutex);
  const uint64_t capture_state = g_pt_capture_state.load();
  if (g_pt_panel.reason != r && (capture_state & 1u) != 0u) {
    RecordPtEvent(g_state.frame.load(std::memory_order_relaxed), PtReasonName(r), capture_state);
  }
  g_pt_panel.reason = r;
  ++g_pt_panel.frames[static_cast<uint32_t>(r)];
}
inline void DrawPtPanel() {
  PtPanel p;
  { std::lock_guard lock(g_pt_panel_mutex); p = g_pt_panel; }
  const std::string lamps_text = p.lamps ? std::to_string(p.lamps) : std::string("off");
  ImGui::Text("Path Tracing: %s  traced %llu  lamps %s  %ux%u", PtReasonName(p.reason),
              static_cast<unsigned long long>(p.frames[static_cast<uint32_t>(PtReason::Tracing)]),
              lamps_text.c_str(), p.width, p.height);
  ImGui::Text("Slot waits %llu  max %.2f ms", static_cast<unsigned long long>(p.waits), p.wait_ms_max);
  ImGui::Text("Sparse policy: requested %s  effective rate %u/256  dense reason %s",
              !p.sparse_requested ? "off" : p.sparse_adaptive ? "adaptive" : "fixed",
              p.sparse_policy.dense_reason == PtSparseDenseReason::None ? p.sparse_policy.levels : 256u,
              PtSparseDenseReasonName(p.sparse_policy.dense_reason));
  ImGui::Text("Sparse reflections: requested %s  effective %s",
              p.sparse_reflections_requested ? "on" : "off", p.sparse_reflections_effective ? "on" : "off");
  ImGui::Text("SHARC: %s  capacity 2^%u  update 1/%u px  window %u frames  clears %llu (last: %s)", p.sharc_on ? "on" : "off",
              p.sharc_capacity_log2, p.sharc_downscale, p.sharc_window, static_cast<unsigned long long>(p.sharc_clears),
              p.sharc_clear_reason);
  ImGui::Text("Material cache: %s  capacity 2^%u  write 1/%u px  clears %llu (last: %s)", p.mat_on ? "on" : "off",
              p.mat_capacity_log2, p.mat_rate * p.mat_rate, static_cast<unsigned long long>(p.mat_clears), p.mat_clear_reason);
  ImGui::Text("Reflection: %s", p.refl_on ? "on" : "off");
  ImGui::Text("debug view: %s | channel: %s", kRtxdiDebugViewNames[(std::min)(p.debug_view, PT_DEBUG_VIEW_FINAL_SPECULAR)],
              kPtDebugChannelNames[(std::min)(p.debug_channel, PT_DEBUG_CHANNEL_INITIAL_VS_REUSED)]);
  static const char* interval_names[] = {"ReGIR register", "ReGIR weights", "ReGIR build", "PT trace"};
  for (uint32_t pass = 0u; pass < 2u; ++pass) {
    for (uint32_t interval = 0u; interval < 4u; ++interval) {
      if ((p.pt_interval_valid[pass] & (1u << interval)) == 0u) continue;
      ImGui::Text("PT %s %s: %.3f ms (avg %.3f ms)", pass == 0u ? "main" : "water", interval_names[interval],
                  p.pt_interval_last_ms[pass][interval], p.pt_interval_average_ms[pass][interval]);
    }
  }
  ImGui::Text("Water surface: %s, reason %s, pixels %u, traces %u", p.water_on ? "on" : "off", p.water_reason, p.gpu_water_pixels,
              p.gpu_water_traces);
  ImGui::Text("Water history: %s (%s)", p.water_history_valid ? "valid" : "invalid", p.water_history_reason);
  ImGui::Text("Water reuse: temporal %u  spatial %u", p.water_temporal_selected, p.water_spatial_selected);
  ImGui::Text("Water roughness: %.2f x scale %.2f", g_pt_reflection_water_roughness, g_pt_reflection_roughness_scale);
  if (p.gpu_available) {
    const uint32_t now = static_cast<uint32_t>(falcom_world::g_state.frame.load());
    const uint32_t gpu = static_cast<uint32_t>(p.gpu_frame);
    const uint32_t age = now - gpu;
    ImGui::Text("Sparse counters (available, active %s), frame %llu (age %u): selected %u, valid %u",
                p.sparse_sample_active ? "yes" : "no", static_cast<unsigned long long>(p.gpu_frame), age, p.sparse_selected, p.sparse_valid);
    ImGui::Text("GPU material lookups, frame %llu (age %u): exact %u, hits %u, grey misses %u; writes %u, lock drops %u",
                static_cast<unsigned long long>(p.gpu_frame), age,
                p.gpu_exact, p.gpu_hits, p.gpu_misses, p.gpu_writes, p.gpu_lock_drops);
    ImGui::Text("PT samples: initial valid %u, path len 2 %u, len 3+ %u, NEE %u, SHARC terminal %u/%u, replay rejected %u",
                p.gpu_initial_valid, p.gpu_path_len2, p.gpu_path_len3plus, p.gpu_nee_connected, p.gpu_sharc_terminated,
                p.gpu_sharc_terminal_rc, p.gpu_sharc_replay_rejected);
  ImGui::Text("PT direct: d=2 lamp %u, d=2 sun %u, shading transmitted %u", p.gpu_d2_lamp, p.gpu_d2_sun,
               p.gpu_shade_transmitted);
  const float ao_sky = p.gpu_ao_rays != 0u
      ? 100.0f * (1.0f - static_cast<float>(p.gpu_ao_occluded) / static_cast<float>(p.gpu_ao_rays)) : 100.0f;
  ImGui::Text("ao: rays %u | occluded %u | sky visibility %.1f%%", p.gpu_ao_rays, p.gpu_ao_occluded, ao_sky);
  ImGui::Text("energy by length: 2 / 3 / 4-5 / 6+: %u / %u / %u / %u", p.gpu_energy_len2, p.gpu_energy_len3,
              p.gpu_energy_len4_5, p.gpu_energy_len6plus);
  ImGui::Text("first hit (PT SHARC view): cached lum %u | traced lum %u", p.gpu_sharc_first_hit_cached_lum,
              p.gpu_first_hit_traced_lum);
    static const char* resampling_names[] = {"none", "temporal", "spatial", "temporal_and_spatial"};
    ImGui::Text("ReSTIR PT resampling: %s  history %s  temporal %u  spatial %u",
                resampling_names[(std::clamp)(static_cast<uint32_t>(g_restir_resampling), 0u, 3u)],
                p.history_valid ? "valid" : p.history_reason, p.temporal_selected, p.spatial_selected);
    ImGui::Text("ReGIR main hits: eligible %u, lookups %u, matches %u, misses %u, invalid header/slots %u/%u, selected %u",
                p.regir_eligible, p.regir_lookups, p.regir_matches, p.regir_misses, p.regir_invalid_headers,
                p.regir_invalid_slots, p.regir_selections);
    ImGui::Text("ReGIR main uniform fallback: disabled %u, key %u, miss %u, header %u, slots %u",
                p.regir_fallback_disabled, p.regir_fallback_key, p.regir_fallback_miss, p.regir_fallback_header,
                p.regir_fallback_slots);
    if (p.regir_water_counts_available) {
      ImGui::Text("ReGIR water hits (frame %u, owner generation %llu): eligible %u, lookups %u, matches %u, misses %u, invalid header/slots %u/%u, selected %u",
                  p.regir_water_counts_frame, static_cast<unsigned long long>(p.regir_water_counts_owner_generation),
                  p.regir_water_eligible, p.regir_water_lookups, p.regir_water_matches, p.regir_water_misses,
                  p.regir_water_invalid_headers, p.regir_water_invalid_slots, p.regir_water_selections);
      ImGui::Text("ReGIR water uniform fallback: disabled %u, key %u, miss %u, header %u, slots %u",
                  p.regir_water_fallback_disabled, p.regir_water_fallback_key, p.regir_water_fallback_miss,
                  p.regir_water_fallback_header, p.regir_water_fallback_slots);
    } else {
      ImGui::Text("ReGIR water hits: unavailable/stale");
    }
  } else {
    ImGui::Text("GPU counters: unavailable");
  }
}
}  // namespace falcom_world::pt
