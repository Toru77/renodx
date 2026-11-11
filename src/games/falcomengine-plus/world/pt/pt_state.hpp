#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <deps/imgui/imgui.h>
namespace falcom_world::pt {
inline float g_pt_enabled = 0.f;
inline bool PtRequested() { return g_pt_enabled > 0.5f; }
inline float g_pt_samples = 1.f, g_pt_max_bounces = 16.f, g_pt_russian_roulette = 1.f,
             g_pt_roulette_min_bounce = 1.f, g_pt_intensity = 1.f, g_pt_char_intensity = 1.f, g_pt_sky_intensity = 1.f,
             g_pt_ray_max = 500.f, g_pt_env_bias = 0.02f, g_pt_char_bias = 0.02f, g_pt_lamp_clearance = 0.2f, g_pt_sun_casters = 0.f;
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
  uint64_t frames[static_cast<uint32_t>(PtReason::Count)] = {};
  uint64_t waits = 0u;
  float wait_ms_max = 0.f;
  uint32_t lamps = 0u, width = 0u, height = 0u;
};
inline std::mutex g_pt_panel_mutex;
inline PtPanel g_pt_panel;
inline void SetPtReason(PtReason r) {
  std::lock_guard lock(g_pt_panel_mutex);
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
}
}  // namespace falcom_world::pt
