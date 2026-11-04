#pragma once

// RTAO panel (Ray Tracing tab, RTAO status row). Reads the per-frame state (rtao::g_rtao_frame) and the
// stats read on request (rtao::g_rtao_stats).

namespace falcom_world::rtao {

inline void DrawRtaoPanel() {
  const RtaoFrameState& frame = g_rtao_frame;
  if (g_test_freeze_noise > 0.5f) {
    ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "TEST MODE ACTIVE: frozen noise");
  }
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
  ImGui::Text("Frames without AO: %llu", static_cast<unsigned long long>(frame.frames_without_ao));
  {
    bvh::GcPoolReduceStats reduce;  // G5
    {
      std::lock_guard lock(bvh::g_gpu_capture.mutex);
      reduce = bvh::g_gpu_capture.pool_reduce;
    }
    if (bvh::g_gpu_capture.pool_reduced.load(std::memory_order_relaxed)) {
      ImGui::Text("CPU pool: reduced (DXR traces the GPU scene); entered %llu, left %llu, vanilla frames %llu",
                  static_cast<unsigned long long>(reduce.entered), static_cast<unsigned long long>(reduce.left),
                  static_cast<unsigned long long>(reduce.vanilla_frames));
    } else {
      ImGui::Text("CPU pool: full (%s, streak %u of %u); entered %llu, left %llu, vanilla frames %llu", reduce.full_reason,
                  reduce.streak, bvh::kGcPoolReduceStreak, static_cast<unsigned long long>(reduce.entered),
                  static_cast<unsigned long long>(reduce.left), static_cast<unsigned long long>(reduce.vanilla_frames));
    }
  }
  if (frame.fade_valid) {
    ImGui::Text("Fade: effective start %.0f m, end %.0f m (requested %.0f / %.0f m, BVH range %.0f m; stored %.1f%% / %.1f%%)",
                frame.fade.start, frame.fade.end, frame.fade.start_req, frame.fade.end_req, frame.fade.coverage,
                g_rtao_fade_start_pct, g_rtao_fade_end_pct);
  } else {
    ImGui::TextDisabled("Fade: no built BVH region yet");
  }
  ImGui::Text("Texture: %.2f MB (trace AO, SVGF raw, history and output, stats)",
              static_cast<double>(frame.texture_bytes) / (1024.0 * 1024.0));

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
    ImGui::Text(" %s: %llu", ResetReasonName(static_cast<ResetReason>(r)),
                static_cast<unsigned long long>(frame.reset_counts[r]));
  }
  if (frame.dispatched_frame != UINT64_MAX) {
    const ParameterSnapshot& c = frame.captured;
    ImGui::Text("Last dispatch: spp %.0f, history weight %.2f, depth rejection %.3f, normal rejection %.2f, IS-FAST %s",
                c.samples, c.history_weight, c.depth_rejection, c.normal_rejection,
                frame.captured_isfast_used ? "used" : "off");
    ImGui::Text("Last dispatch: radius %.2f, ray max %.2f, strength %.2f, normal bias %.4f, debug %.0f, "
                "tests freeze noise %s, foliage skip %s, trace characters %s (%u objects)",
                c.radius, c.ray_max, c.strength, c.normal_bias, c.debug, c.freeze_noise > 0.5f ? "on" : "off",
                c.foliage_skip > 0.5f ? "on" : "off", c.trace_characters > 0.5f ? "on" : "off", frame.character_objects);
  }
  if (frame.trace_divisor > 1u) {
    ImGui::Text("Trace: %u x %u (one per %u x %u block of %u x %u)", frame.trace_width, frame.trace_height,
                frame.trace_divisor, frame.trace_divisor, frame.depth_w, frame.depth_h);
    ImGui::TextDisabled("Counters are per trace pixel at Half and Quarter");
  }
  if (frame.temporal_ran || frame.motion_dims_ok == false) {
    ImGui::Text("Motion dims: RTV4 %ux%u, depth %ux%u (%s)", frame.motion_w, frame.motion_h,
                frame.depth_w, frame.depth_h, frame.motion_dims_ok ? "match" : "MISMATCH: no_motion");
  }
  if (frame.last_parameter_change[0] != '\0') ImGui::Text("Last parameter change: %s", frame.last_parameter_change);
  for (size_t r = 0; r < kReasonCount; ++r) {
    if (frame.frames_without_ao_by_reason[r] == 0u) continue;
    ImGui::Text("Frames without AO, %s: %llu", ReasonName(static_cast<Reason>(r)),
                static_cast<unsigned long long>(frame.frames_without_ao_by_reason[r]));
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
  ImGui::SeparatorText("Denoising (SVGF)");
  ImGui::Text("Spatial filter: %s, iterations %u (effective)", g_rtao_spatial_enabled > 0.5f ? "on" : "off", frame.svgf_iterations);
  ImGui::Text("Denoise GPU: %.3f ms", frame.denoise_gpu_ms);
  if (g_rtao_stats.valid && frame.svgf_stats) {
    const uint32_t* v = g_rtao_stats.values;
    ImGui::Text("Temporal pixels %u, no history %.1f%%, reset %.1f%%", v[kRtaoStatDenoiseBase],
                SharePercent(v[kRtaoStatDenoiseBase + 1u], v[kRtaoStatDenoiseBase]),
                SharePercent(v[kRtaoStatDenoiseBase + 2u], v[kRtaoStatDenoiseBase]));
  } else {
    ImGui::TextDisabled("Counters: press Read RTAO Stats (not with Two-Sided discovery on)");
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
