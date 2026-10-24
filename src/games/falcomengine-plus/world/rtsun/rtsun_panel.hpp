#pragma once

// RT Sun Shadows panel (Ray Tracing tab, RT Sun Shadows status row). Reads rtsun::g_rtsun_frame, the camera's sun cascades
// and the stats read on request (rtsun::g_rtsun_stats). Timings are on the Performance tab only.

#include <cstdint>

#include <deps/imgui/imgui.h>

#include "../rtao/rtao_state.hpp"
#include "../world_state.hpp"
#include "rtsun.hpp"

namespace falcom_world::rtsun {

inline void DrawRtSunPanel() {
  const RtSunFrameState& frame = g_rtsun_frame;
  if (!RtSunRequested()) {
    ImGui::TextDisabled("RT sun: off");
    return;
  }
  const int applied = g_state.rt_sun_cascades.load(std::memory_order_relaxed);
  if (frame.producing) {
    if (applied > 0) {
      ImGui::Text("RT sun: producing (traced this frame; applied to the image)");
    } else {
      ImGui::Text("RT sun: producing (traced, not applied (latch 0))");
    }
  } else if (frame.reason == Reason::InputsMissing) {
    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "RT sun: requested, not producing (%s / %s)", ReasonName(frame.reason),
                       rtao::ReasonName(frame.input_reason));
  } else {
    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "RT sun: requested, not producing (%s)", ReasonName(frame.reason));
  }

  const auto& camera = g_state.camera;
  if (frame.built_range <= 0.f) {
    ImGui::TextDisabled("RT sun covers: waiting for BVH");
  } else if (camera.has_sun) {
    const int effective = RtSunEffectiveCascades(static_cast<int>(g_rtsun_cascade_range), camera, frame.built_range);
    const float covered = RtSunBoundary(static_cast<uint32_t>(effective), camera.shadow_cascade_count, camera.shadow_split_distance,
                                        camera.shadow_fade_near, camera.shadow_fade_range_inv);
    ImGui::Text("RT sun covers %d of %d cascades, to %.1f m, limited by BVH Region %.0f m; applied: %d", effective,
                camera.shadow_cascade_count, static_cast<double>(covered), static_cast<double>(frame.built_range), applied);
  } else {
    ImGui::TextDisabled("RT sun: camera sun data not captured");
  }
  if (frame.width != 0u) {
    ImGui::Text("Trace: %u x %u (Full)", frame.width, frame.height);
  }
  ImGui::Text("Characters: casters %s (%u objects traced); not receivers", g_rtsun_trace_characters > 0.5f ? "on" : "off",
              frame.character_objects);
  if (frame.isfast_unavailable) {
    ImGui::TextDisabled("IS-FAST: not loaded; IGN noise used");
  }
  ImGui::Text("GPU: %.3f ms (Performance gathering must be on; '--' otherwise)", static_cast<double>(frame.gpu_ms));
  if (ImGui::Button("Read RT Sun Stats")) {
    g_rtsun_stats.requested.store(true, std::memory_order_relaxed);
  }
  if (g_rtsun_stats.valid) {
    const uint32_t* v = g_rtsun_stats.values;
    ImGui::Text("Pixels: traced %u of %u (sky %u, normal %u, character %u, scaled %u, region %u, beyond range %u)",
                v[kRtSunStatTraced], v[kRtSunStatPixels], v[kRtSunStatSky], v[kRtSunStatNormal], v[kRtSunStatCharacter],
                v[kRtSunStatScaled], v[kRtSunStatRegion], v[kRtSunStatBeyondRange]);
    const uint32_t rays = v[kRtSunStatRays];
    ImGui::Text("Rays: %u, blocked %.1f%%, blocked rays whose hit is a character %.1f%%, self-blocked %u", rays,
                rtao::SharePercent(v[kRtSunStatRaysBlocked], rays), rtao::SharePercent(v[kRtSunStatCharacterRays], rays),
                v[kRtSunStatSelfBlocked]);
    ImGui::Text("invalid_refs %u, stack_overflow %u", v[kRtSunStatInvalidRefs], v[kRtSunStatStackOverflow]);
  }
}

}  // namespace falcom_world::rtsun
