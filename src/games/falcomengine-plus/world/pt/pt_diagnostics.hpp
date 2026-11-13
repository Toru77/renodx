#pragma once
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include "../../../../utils/log.hpp"
#include "pt_state.hpp"
namespace falcom_world::pt {
inline void SetPtDiagnosticCapture(bool enabled) {
  std::lock_guard lock(g_pt_event_mutex);
  const uint64_t previous = g_pt_capture_state.load();
  if (static_cast<bool>(previous & 1u) == enabled) return;
  if (enabled) {
    g_pt_event_next = 0u;
    g_pt_event_size = 0u;
    g_pt_event_overwritten = 0u;
  }
  g_pt_capture_state.store(((previous & ~uint64_t{1}) + 2u) | uint64_t{enabled});
}
inline bool PtDiagnosticCaptureEnabled() { return (g_pt_capture_state.load() & 1u) != 0u; }
inline void SyncPtDiagnosticCapture(bool enabled) {
  if (PtDiagnosticCaptureEnabled() != enabled) {
    SetPtDiagnosticCapture(enabled);
  }
}
inline void PtLogWarn(const std::string& text) {
  const uint64_t capture_state = g_pt_capture_state.load();
  if ((capture_state & 1u) != 0u) {
    RecordPtEvent(g_state.frame.load(std::memory_order_relaxed), text, capture_state);
  } else {
    renodx::utils::log::w(text);
  }
}
inline std::string SerializePtReport() {
  PtPanel p;
  {
    std::lock_guard lock(g_pt_panel_mutex);
    p = g_pt_panel;
  }
  std::ostringstream out;
  out << std::boolalpha;
  out << "reason: " << PtReasonName(p.reason) << "\n";
  out << "traced: " << p.frames[static_cast<uint32_t>(PtReason::Tracing)] << "\n";
  out << "lamps: " << p.lamps << "\n";
  out << "width: " << p.width << "\n";
  out << "height: " << p.height << "\n";
  out << "sparse requested mode: " << (!p.sparse_requested ? "off" : p.sparse_adaptive ? "adaptive" : "fixed") << "\n";
  out << "sparse reflections requested: " << p.sparse_reflections_requested << "\n";
  out << "sparse reflections effective: " << p.sparse_reflections_effective << "\n";
  out << "sparse effective levels: " << (p.sparse_policy.dense_reason == PtSparseDenseReason::None ? p.sparse_policy.levels : 256u) << " / 256\n";
  out << "sparse dense reason: " << PtSparseDenseReasonName(p.sparse_policy.dense_reason) << "\n";
  out << "sparse counter available: " << p.gpu_available << "\n";
  out << "sparse sample active: " << p.sparse_sample_active << "\n";
  out << "sparse counter frame: " << p.gpu_frame << "\n";
  out << "sparse counter age: " << (static_cast<uint32_t>(g_state.frame.load()) - static_cast<uint32_t>(p.gpu_frame)) << "\n";
  out << "sparse selected: " << p.sparse_selected << "\n";
  out << "sparse valid: " << p.sparse_valid << "\n";
  out << "slot waits: " << p.waits << "\n";
  out << "max wait ms: " << p.wait_ms_max << "\n";
  out << "sharc on: " << p.sharc_on << "\n";
  out << "sharc clears: " << p.sharc_clears << "\n";
  out << "sharc last reason: " << p.sharc_clear_reason << "\n";
  out << "sharc capacity log2: " << p.sharc_capacity_log2 << "\n";
  out << "sharc downscale: " << p.sharc_downscale << "\n";
  out << "sharc window: " << p.sharc_window << "\n";
  out << "material on: " << p.mat_on << "\n";
  out << "material clears: " << p.mat_clears << "\n";
  out << "material last reason: " << p.mat_clear_reason << "\n";
  out << "material capacity log2: " << p.mat_capacity_log2 << "\n";
  out << "material rate: " << p.mat_rate << "\n";
  out << "gpu_exact: " << p.gpu_exact << "\n";
  out << "gpu_hits: " << p.gpu_hits << "\n";
  out << "gpu_misses: " << p.gpu_misses << "\n";
  out << "gpu_lock_drops: " << p.gpu_lock_drops << "\n";
  out << "gpu_writes: " << p.gpu_writes << "\n";
  out << "gpu_frame: " << p.gpu_frame << "\n";
  out << "gpu_available: " << p.gpu_available << "\n";
  out << "material cell scale: " << g_pt_material_cell_scale << "\n";
  out << "samples: " << g_pt_samples << "\n";
  out << "max bounces: " << g_pt_max_bounces << "\n";
  out << "russian roulette: " << g_pt_russian_roulette << "\n";
  out << "roulette min bounce: " << g_pt_roulette_min_bounce << "\n";
  out << "ray max: " << g_pt_ray_max << "\n";
  out << "env bias: " << g_pt_env_bias << "\n";
  out << "char bias: " << g_pt_char_bias << "\n";
  out << "lamp clearance: " << g_pt_lamp_clearance << "\n";
  out << "sun casters: " << g_pt_sun_casters << "\n";
  out << "sky intensity: " << g_pt_sky_intensity << "\n";
  out << "intensity: " << g_pt_intensity << "\n";
  out << "char intensity: " << g_pt_char_intensity << "\n";
  out << "sharc enabled: " << g_pt_sharc_enabled << "\n";
  out << "sharc capacity log2 setting: " << g_pt_sharc_capacity_log2 << "\n";
  out << "accumulation frames: " << g_pt_sharc_accumulation_frames << "\n";
  out << "stale frames: " << g_pt_sharc_stale_frames << "\n";
  out << "scene scale: " << g_pt_sharc_scene_scale << "\n";
  out << "sharc downscale setting: " << g_pt_sharc_downscale << "\n";
  out << "sharc radiance scale: " << g_pt_sharc_radiance_scale << "\n";
  out << "sharc firefly clamp enabled: " << g_pt_sharc_firefly_clamp_enabled << "\n";
  out << "sharc firefly clamp: " << g_pt_sharc_firefly_clamp << "\n";
  out << "material enabled: " << g_pt_material_enabled << "\n";
  out << "material rate setting: " << g_pt_material_rate << "\n";
  out << "material cell scale setting: " << g_pt_material_cell_scale << "\n";
  out << "material capacity log2 setting: " << g_pt_material_capacity_log2 << "\n";
  out << "char roughness: " << g_pt_char_roughness << "\n";
  out << "char f0: " << g_pt_char_f0 << "\n";
  out << "reflection enabled: " << g_pt_reflection_enabled << "\n";
  out << "reflection on: " << (p.refl_on ? "true" : "false") << "\n";
  out << "reflection samples: " << g_pt_reflection_samples << "\n";
  out << "reflection intensity: " << g_pt_reflection_intensity << "\n";
  out << "reflection roughness scale: " << g_pt_reflection_roughness_scale << "\n";
  out << "reflection sample clamp: " << g_pt_reflection_sample_clamp << "\n";
  out << "reflection clamp enabled: " << g_pt_reflection_clamp_enabled << "\n";
  out << "gi clamp enabled: " << g_pt_gi_clamp_enabled << "\n";
  out << "gi clamp: " << g_pt_gi_clamp << "\n";
  out << "reflection cap to one: " << g_pt_reflection_cap_to_one << "\n";
  out << "reflection uses sharc: " << g_pt_reflection_use_sharc << "\n";
  out << "reflection footprint test: " << g_pt_reflection_footprint << "\n";
  out << "water reflection bed test: " << g_pt_reflection_water_rule << "\n";
  out << "water surface on: " << (p.water_on ? "true" : "false") << "\n";
  out << "water surface reason: " << p.water_reason << "\n";
  out << "water surface pixels: " << p.gpu_water_pixels << "\n";
  out << "water surface traces: " << p.gpu_water_traces << "\n";
  out << "water roughness: " << g_pt_reflection_water_roughness << "\n";
  out << "debug logging: " << PtDiagnosticCaptureEnabled() << "\n";
  std::vector<PtEvent> events;
  uint64_t overwritten = 0u;
  {
    std::lock_guard lock(g_pt_event_mutex);
    overwritten = g_pt_event_overwritten;
    const uint32_t oldest = (g_pt_event_next + kPtEventCapacity - g_pt_event_size) % kPtEventCapacity;
    for (uint32_t i = 0u; i < g_pt_event_size; ++i) {
      events.push_back(g_pt_events[(oldest + i) % kPtEventCapacity]);
    }
  }
  out << "overwritten events: " << overwritten << "\n";
  for (const PtEvent& e : events) {
    out << "event frame " << e.frame << ": " << e.text << "\n";
  }
  return out.str();
}
}  // namespace falcom_world::pt
