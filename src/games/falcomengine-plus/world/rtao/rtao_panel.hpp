#pragma once

// RTAO panel (Ray Tracing tab, RTAO status row). Reads the per-frame state (rtao::g_rtao_frame) and the
// stats read on request (rtao::g_rtao_stats).

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
    ImGui::Text("GPU: %.3f ms (pass A%s)", frame.gpu_ms, frame.temporal_ran ? " + pass B" : " only");
  }
  ImGui::Text("Frames without AO: %llu", static_cast<unsigned long long>(frame.frames_without_ao));
  if (frame.fade_valid) {
    ImGui::Text("Fade: effective start %.0f m, end %.0f m (coverage %.0f m; stored %.0f / %.0f m)",
                frame.fade.start, frame.fade.end, frame.fade.coverage, g_rtao_fade_start, g_rtao_fade_end);
  } else {
    ImGui::TextDisabled("Fade: no built BVH region yet");
  }
  ImGui::Text("Texture: %.2f MB (AO, stats, raw and history: %.2f MB temporal)",
              static_cast<double>(frame.texture_bytes) / (1024.0 * 1024.0),
              static_cast<double>(frame.temporal_bytes) / (1024.0 * 1024.0));

  ImGui::SeparatorText("Temporal");
  if (g_rtao_temporal_enabled < 0.5f) {
    ImGui::TextDisabled("Temporal: off (AO is the round-1 trace)");
  } else if (frame.temporal_ran) {
    ImGui::Text("Temporal: on, running (this frame reset: %s)", ResetReasonName(frame.reset));
  } else {
    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "Temporal: on, not running (%s)", ResetReasonName(frame.reset));
  }
  switch (frame.motion_source) {
    case MotionSource::Rtv4: ImGui::Text("Motion source: RTV4 owned SRV"); break;
    case MotionSource::Conflict:
      ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
                         "Motion source: conflict (TAA t3 and RTV4 differ; Temporal off this session)");
      break;
    case MotionSource::None: ImGui::TextDisabled("Motion source: none"); break;
  }
  ImGui::Text("rcas_motion_res %llu, mb_rtv4_res %llu%s",
              static_cast<unsigned long long>(frame.rcas_res), static_cast<unsigned long long>(frame.rtv4_res),
              frame.rcas_res != 0u && frame.rtv4_res != 0u && frame.rcas_res == frame.rtv4_res ? " (same)" : "");
  ImGui::Text("Last reset: %s", ResetReasonName(frame.last_reset));
  for (size_t r = 1; r < static_cast<size_t>(ResetReason::Count); ++r) {
    if (frame.reset_counts[r] == 0u) continue;
    ImGui::Text("  %s: %llu", ResetReasonName(static_cast<ResetReason>(r)),
                static_cast<unsigned long long>(frame.reset_counts[r]));
  }
  if (g_rtao_stats.valid) {
    const uint32_t* v = g_rtao_stats.values;
    const float pixels = static_cast<float>(v[kRtaoStatTemporalBase + 0u]);
    if (pixels > 0.f) {
      const float taps = 4.f * pixels;
      ImGui::Text("Temporal taps: valid %.1f%%, rejected depth %.1f%%, rejected normal %.1f%%",
                  100.f * v[kRtaoStatTemporalBase + 1u] / taps,
                  100.f * v[kRtaoStatTemporalBase + 2u] / taps,
                  100.f * v[kRtaoStatTemporalBase + 3u] / taps);
      ImGui::Text("Per pixel: out of bounds %.1f%%, reset %.1f%%",
                  100.f * v[kRtaoStatTemporalBase + 4u] / pixels,
                  100.f * v[kRtaoStatTemporalBase + 5u] / pixels);
    }
  } else {
    ImGui::TextDisabled("Temporal percentages: press Read RTAO Stats");
  }

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
  ImGui::SeparatorText("Two-Sided discovery (diagnostic, no culling)");
  bool any_cpu = false;
  for (uint32_t cull = 0u; cull < 4u; ++cull) {
    for (uint32_t ccw = 0u; ccw < 2u; ++ccw) {
      const uint32_t draws = g_rtao_cull_hist[cull][ccw].load(std::memory_order_relaxed);
      if (draws == 0u) continue;
      any_cpu = true;
      ImGui::Text("Camera opaque candidate draws: cull %u, %s: %u", cull, ccw ? "front CCW" : "front CW", draws);
    }
  }
  if (!any_cpu) ImGui::TextDisabled("CPU counts: none yet (turn Two-Sided discovery on)");
  if (g_rtao_stats.valid && g_rtao_discovery > 0.5f) {
    static const char* const kDiscoveryNames[kRtaoStatDiscoveryCount] = {
        "front away opaque", "front away alpha", "front toward opaque", "front toward alpha",
        "back away opaque", "back away alpha", "back toward opaque", "back toward alpha"};
    for (uint32_t i = 0u; i < kRtaoStatDiscoveryCount; ++i) {
      ImGui::Text("GPU %s: %u", kDiscoveryNames[i], g_rtao_stats.values[kRtaoStatDiscoveryBase + i]);
    }
  }
  ImGui::TextDisabled("Dynamic objects not traced (round 3): characters and water do not occlude RTAO.");
}

}  // namespace falcom_world::rtao
