#pragma once

// RTGI panel (Ray Tracing tab, RTGI status row). Reads rtgi::g_rtgi_frame and the stats read on request (rtgi::g_rtgi_stats).
// Timings are on the Performance tab only.

#include <cstdint>
#include <cstring>

#include <deps/imgui/imgui.h>

#include "../rtao/rtao_state.hpp"
#include "rtgi.hpp"

namespace falcom_world::rtgi {

inline void DrawRtgiPanel() {
  const RtgiFrameState& frame = g_rtgi_frame;
  if (g_rtgi_enabled < 0.5f) {
    ImGui::TextDisabled("RTGI: off");
  } else if (frame.producing) {
    ImGui::Text("RTGI: active (raw GI traced this frame; the image is unchanged)");
  } else {
    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "RTGI: requested, not running (%s)", ReasonName(frame.reason));
  }
  if (frame.trace_width != 0u) {
    ImGui::Text("Trace: %u x %u (one per %u x %u block of %u x %u)", frame.trace_width, frame.trace_height,
                frame.divisor, frame.divisor, frame.full_width, frame.full_height);
  }
  ImGui::Text("Scene layout: prevViewProj_g at c%u", frame.prev_view_proj_register);
  ImGui::Text("Max Ray Distance: requested %.1f m, effective %.1f m", frame.ray_max_requested, frame.ray_max_effective);
  if (frame.fade_valid) {
    ImGui::Text("Fade: effective start %.0f m, end %.0f m (requested %.0f / %.0f m, BVH range %.0f m; stored %.1f%% / %.1f%%)",
                frame.fade.start, frame.fade.end, frame.fade.start_req, frame.fade.end_req, frame.fade.coverage,
                g_rtgi_fade_start_pct, g_rtgi_fade_end_pct);
  } else {
    ImGui::TextDisabled("Fade: no built BVH region yet");
  }
  ImGui::Text("Texture: %.2f MB", static_cast<double>(frame.texture_bytes) / (1024.0 * 1024.0));
  ImGui::Text("Radiance source: %s, %u x %u, format %u",
              frame.radiance_valid ? "previous frame's lighting output" : "not available (hidden and off-screen shading only)",
              frame.radiance_width, frame.radiance_height, frame.radiance_format);
  if (ImGui::Button("Read RTGI Stats")) {
    g_rtgi_stats.requested.store(true, std::memory_order_relaxed);
  }
  if (g_rtgi_stats.valid) {
    const uint32_t* v = g_rtgi_stats.values;
    const uint32_t hits = v[kRtgiStatHits];
    const float mean_distance = hits > 0u ? static_cast<float>(v[kRtgiStatHitDistanceDm]) / 10.f / static_cast<float>(hits) : 0.f;
    ImGui::Text("Rays %u, hits %.1f%%, mean hit distance %.2f m", v[kRtgiStatRays],
                rtao::SharePercent(hits, v[kRtgiStatRays]), mean_distance);
    ImGui::Text("Hits: on screen %.1f%%, hidden on screen %.1f%%, off screen %.1f%%",
                rtao::SharePercent(v[kRtgiStatHitOnScreen], hits), rtao::SharePercent(v[kRtgiStatHitOccluded], hits),
                rtao::SharePercent(v[kRtgiStatHitOffScreen], hits));
    ImGui::Text("invalid_refs %u, stack_overflow %u", v[kRtgiStatInvalidRefs], v[kRtgiStatStackOverflow]);
    ImGui::Text("Not traced: sky %u, normal %u, scaled %u, region %u, beyond Fade End %u (of %u trace pixels)",
                v[kRtgiStatSky], v[kRtgiStatNormal], v[kRtgiStatScaled], v[kRtgiStatRegion], v[kRtgiStatBeyondFade],
                v[kRtgiStatPixels]);
    float luminance_max = 0.f;
    std::memcpy(&luminance_max, &v[kRtgiStatLuminanceMax], sizeof(luminance_max));
    const float mean_luminance = hits > 0u ? static_cast<float>(static_cast<double>((static_cast<uint64_t>(v[kRtgiStatLuminanceSumHi]) << 32) | v[kRtgiStatLuminanceSum]) / 10.0 / hits) : 0.f;
    ImGui::Text("Radiance: on-screen lookups %u, sun rays %u (visible %.1f%%), mean sample luminance (capped at 1000 per sample) %.3f, max %.3f, clamped %.1f%%",
                v[kRtgiStatRadianceLookups], v[kRtgiStatSunRays], rtao::SharePercent(v[kRtgiStatSunVisible], v[kRtgiStatSunRays]),
                mean_luminance, luminance_max, rtao::SharePercent(v[kRtgiStatClamped], hits));
  }
}

}  // namespace falcom_world::rtgi
