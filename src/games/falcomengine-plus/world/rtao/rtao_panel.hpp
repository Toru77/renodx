#pragma once

// RTAO round 1 panel (Ray Tracing tab, RTAO section): the per-frame state and the
// stats read on request (Read RTAO Stats). Drawn from world_settings.hpp.

namespace falcom_world::rtao {

inline void DrawRtaoPanel() {
  const RtaoFrameState& frame = g_rtao_frame;
  if (!RtaoRequested()) {
    ImGui::TextDisabled("RTAO: off (GTVBAO and VBGI under their own toggles)");
  } else if (frame.producing) {
    ImGui::Text("RTAO: active (AO produced this frame)");
  } else {
    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f),
                       "RTAO: requested, not producing (%s). Vanilla SSAO.", ReasonName(frame.reason));
  }
  if (frame.note == Reason::IsfastUnavailable) {
    ImGui::TextDisabled("IS-FAST not loaded: IGN noise (isfast_unavailable)");
  }
  if (frame.gpu_ms < 0.f) {
    ImGui::Text("GPU: no measurement yet");
  } else {
    ImGui::Text("GPU: %.3f ms", frame.gpu_ms);
  }
  ImGui::Text("Frames without AO: %llu", static_cast<unsigned long long>(frame.frames_without_ao));
  if (frame.fade_valid) {
    ImGui::Text("Fade: effective start %.0f m, end %.0f m (coverage %.0f m; stored %.0f / %.0f m)",
                frame.fade.start, frame.fade.end, frame.fade.coverage, g_rtao_fade_start, g_rtao_fade_end);
  } else {
    ImGui::TextDisabled("Fade: no built BVH region yet");
  }
  ImGui::Text("Texture: %.2f MB (AO r32_uint and stats)", static_cast<double>(frame.texture_bytes) / (1024.0 * 1024.0));
  if (ImGui::Button("Read RTAO Stats")) g_rtao_stats.requested.store(true, std::memory_order_relaxed);
  if (g_rtao_stats.valid) {
    const uint32_t* v = g_rtao_stats.values;
    const float occluded = v[kRtaoStatRays] > 0u ? 100.f * v[kRtaoStatHits] / v[kRtaoStatRays] : 0.f;
    const float mean_ao = v[kRtaoStatPixels] > 0u ? static_cast<float>(v[kRtaoStatAoSum]) / (255.f * v[kRtaoStatPixels]) : 1.f;
    ImGui::Text("Rays %u, occluded %.1f%%, mean AO %.3f (%u pixels)", v[kRtaoStatRays], occluded, mean_ao, v[kRtaoStatPixels]);
    ImGui::Text("invalid_refs %u, stack_overflow %u", v[kRtaoStatInvalidRefs], v[kRtaoStatStackOverflow]);
    ImGui::Text("Skipped pixels: sky %u, normal %u, scaled %u (shader), region %u",
                v[kRtaoStatSky], v[kRtaoStatNormal], v[kRtaoStatScaled], v[kRtaoStatRegion]);
  }
  ImGui::TextDisabled("Dynamic objects not traced (round 3): characters and water do not occlude RTAO.");
}

}  // namespace falcom_world::rtao
