#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include "../world_state.hpp"
#include <deps/imgui/imgui.h>
namespace falcom_world::pt {
inline float g_pt_enabled = 0.f;
inline float g_pt_regir_enabled = 0.f;
inline bool PtRequested() { return g_pt_enabled > 0.5f; }
inline float g_pt_sparse_reflections = 1.f;
inline float g_pt_sparse_enabled = 0.f, g_pt_sparse_adaptive = 1.f, g_pt_sparse_rate = 0.10f,
             g_pt_sparse_exponent = 0.91f, g_pt_sparse_quality = 0.30f, g_pt_sparse_max_rate = 0.75f;
enum class PtSparseDenseReason : uint32_t { None, Disabled, RateAboveMax, FullRate, RrNotRequested, RrNotActive, NoiseUnavailable };
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
inline uint32_t g_pt_sharc_reset_request = 0u;  // bumped by the Clear SHARC Cache button
inline constexpr uint32_t kPtSharcGapFrames = 5u;           // frames without a SHARC trace that clear it (addon.cpp:539 uses 5)
inline constexpr uint32_t kPtSharcMaxUpdatePaths = 82944u;  // OpenRemix options.h:346 (1920x1080 at downscale 5)
inline constexpr uint32_t kPtSharcAccumulationStride = 32u; // OpenRemix sharc_policy.h:9
inline constexpr uint32_t kPtSharcResolvedStride = 24u;     // OpenRemix sharc_policy.h:10
inline float g_pt_material_enabled = 1.f, g_pt_material_rate = 4.f, g_pt_material_cell_scale = 30.f,
             g_pt_material_capacity_log2 = 21.f, g_pt_char_roughness = 0.5f, g_pt_char_f0 = 0.04f;
inline constexpr uint32_t kPtMaterialValueStride = 16u;
inline float g_pt_samples = 1.f, g_pt_max_bounces = 16.f, g_pt_russian_roulette = 1.f,
             g_pt_roulette_min_bounce = 1.f, g_pt_intensity = 1.f, g_pt_char_intensity = 1.f, g_pt_sky_intensity = 1.f,
             g_pt_ray_max = 500.f, g_pt_env_bias = 0.02f, g_pt_char_bias = 0.02f, g_pt_lamp_clearance = 0.2f, g_pt_sun_casters = 0.f;
inline float g_pt_reflection_enabled = 0.f, g_pt_reflection_samples = 1.f, g_pt_reflection_intensity = 1.f,
             g_pt_reflection_roughness_scale = 1.f;
inline float g_pt_reflection_sample_clamp = 16.f;
inline float g_pt_reflection_clamp_enabled = 1.f;
inline float g_pt_gi_clamp_enabled = 0.f, g_pt_gi_clamp = 16.f;
inline float g_pt_sharc_firefly_clamp_enabled = 0.f, g_pt_sharc_firefly_clamp = 10.f;
inline float g_pt_reflection_cap_to_one = 0.f;
inline float g_pt_reflection_use_sharc = 1.f, g_pt_reflection_footprint = 1.f;
inline float g_pt_reflection_water_rule = 0.f;
inline float g_pt_reflection_water_surface = 0.f;
inline float g_pt_reflection_water_roughness = 0.05f;
enum class PtReason : uint32_t { Off, DeferredContext, NeedsRtxdi, InputNotReady, DxrOff, NotCreated, NoShaderModel66,
                                 Failed, SceneMissing, Stuck, SubmitFailed, TargetFailed, Tracing, Count };
inline const char* PtReasonName(PtReason r) {
  switch (r) {
    case PtReason::Off: return "off";
    case PtReason::DeferredContext: return "deferred_context";
    case PtReason::NeedsRtxdi: return "needs_rtxdi_local";
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
  bool water_on = false;
  uint32_t gpu_water_pixels = 0u, gpu_water_traces = 0u;
  const char* water_reason = "off";
  uint32_t gpu_exact = 0u, gpu_hits = 0u, gpu_misses = 0u, gpu_lock_drops = 0u, gpu_writes = 0u;
  uint32_t sparse_selected = 0u, sparse_valid = 0u;
  uint64_t gpu_frame = 0u;
  bool gpu_available = false, sparse_sample_active = false;
  uint32_t lamps = 0u, width = 0u, height = 0u;
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
  ImGui::Text("Water surface: %s, reason %s, pixels %u, traces %u", p.water_on ? "on" : "off", p.water_reason, p.gpu_water_pixels,
              p.gpu_water_traces);
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
  } else {
    ImGui::Text("GPU counters: unavailable");
  }
}
}  // namespace falcom_world::pt
