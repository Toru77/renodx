#pragma once

// RT Local Shadows panel (Ray Tracing tab, RT Local Shadows status row). Reads rtls::g_rtls_frame and the stats read on
// request (rtls::g_rtls_stats). Timings are on the Performance tab only.

#include <cstdint>

#include <deps/imgui/imgui.h>

#include "../rtao/rtao_state.hpp"
#include "../world_state.hpp"
#include "rtls.hpp"

namespace falcom_world::rtls {

inline void DrawRtlsPanel() {
  const RtlsFrameState& frame = g_rtls_frame;
  if (!RtlsRequested()) {
    ImGui::TextDisabled("RTLS: off");
  } else if (frame.producing) {
    ImGui::Text("RTLS: active (traced this frame; not applied to the image until M3)");
  } else if (frame.reason == Reason::InputsMissing) {
    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "RTLS: requested, not running (%s / %s)", ReasonName(frame.reason),
                       rtao::ReasonName(frame.input_reason));
  } else {
    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "RTLS: requested, not running (%s)", ReasonName(frame.reason));
  }
  if (frame.trace_width != 0u) {
    ImGui::Text("Trace: %u x %u", frame.trace_width, frame.trace_height);
  }
  ImGui::Text("Lamps: %u in the buffer; kinds traced: point %s, spot %s", frame.lamp_count,
              (frame.kinds & kRtlsKindPoint) != 0u ? "on" : "off", (frame.kinds & kRtlsKindSpot) != 0u ? "on" : "off");
  ImGui::Text("Characters: %s (%u objects traced)", g_rtls_trace_characters > 0.5f ? "on" : "off", frame.character_objects);
  if (frame.isfast_unavailable) {
    ImGui::TextDisabled("IS-FAST: not loaded; IGN noise used");
  }
  ImGui::Text("Max Ray Distance: requested %.1f m, effective %.1f m", frame.ray_max_requested, frame.ray_max_effective);
  if (frame.fade_valid) {
    ImGui::Text("Fade: effective start %.0f m, end %.0f m (requested %.0f / %.0f m, BVH range %.0f m; stored %.1f%% / %.1f%%)",
                frame.fade.start, frame.fade.end, frame.fade.start_req, frame.fade.end_req, frame.fade.coverage,
                g_rtls_fade_start_pct, g_rtls_fade_end_pct);
  } else {
    ImGui::TextDisabled("Fade: no built BVH region yet");
  }
  ImGui::Text("GPU: %.3f ms (Performance gathering must be on; '--' otherwise)", static_cast<double>(frame.gpu_ms));
  ImGui::Text("Texture: %.2f MB", static_cast<double>(frame.texture_bytes) / (1024.0 * 1024.0));
  if (ImGui::Button("Read RTLS Stats")) {
    g_rtls_stats.requested.store(true, std::memory_order_relaxed);
  }
  if (g_rtls_stats.valid) {
    const uint32_t* v = g_rtls_stats.values;
    ImGui::Text("Pixels: traced %u of %u (sky %u, normal %u, character %u, scaled %u, region %u, beyond Fade End %u)",
                v[kRtlsStatTraced], v[kRtlsStatPixels], v[kRtlsStatSky], v[kRtlsStatNormal], v[kRtlsStatCharacter],
                v[kRtlsStatScaled], v[kRtlsStatRegion], v[kRtlsStatBeyondFade]);
    ImGui::Text("Lamps: considered %u, traced %u, points over budget %u, spots over cap %u, back-lit %u, index out of range %u",
                v[kRtlsStatLightsConsidered], v[kRtlsStatLightsTraced], v[kRtlsStatPointsOverBudget], v[kRtlsStatSpotsOverCap],
                v[kRtlsStatLightsBacklit], v[kRtlsStatIndexOutOfRange]);
    const uint32_t rays = v[kRtlsStatRays];
    ImGui::Text("Rays: %u, blocked %.1f%%, blocked near the lamp %.1f%%, clipped by Max Ray Distance %.1f%%", rays,
                rtao::SharePercent(v[kRtlsStatRaysBlocked], rays), rtao::SharePercent(v[kRtlsStatRaysNearLight], rays),
                rtao::SharePercent(v[kRtlsStatRaysClipped], rays));
    ImGui::Text("invalid_refs %u, stack_overflow %u", v[kRtlsStatInvalidRefs], v[kRtlsStatStackOverflow]);
    ImGui::Text("Lamps blocked near the source (> 50%% of their rays): ");
    bool any = false;
    for (uint32_t i = 0u; i < kRtlsPerLightCount; ++i) {
      const uint32_t lamp_rays = v[kRtlsStatPerLightRays + i];
      const float share = rtao::SharePercent(v[kRtlsStatPerLightNear + i], lamp_rays);
      if (lamp_rays == 0u || share <= 50.f) continue;
      ImGui::SameLine();
      ImGui::Text("%u (%.0f%%)", i, static_cast<double>(share));
      any = true;
    }
    if (!any) {
      ImGui::SameLine();
      ImGui::Text("none");
    }
  }
}

}  // namespace falcom_world::rtls
