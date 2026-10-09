#pragma once

// RTAO panel (Ray Tracing tab, RTAO status row). Reads the per-frame state (rtao::g_rtao_frame) and the
// stats read on request (rtao::g_rtao_stats).

namespace falcom_world::rtao {

inline void DrawRtaoPanel() {
  const RtaoFrameState& frame = g_rtao_frame;
  if (g_test_zero_motion > 0.5f || g_test_freeze_noise > 0.5f || g_test_camera_matrix > 0.5f) {
    std::string modes;
    if (g_test_zero_motion > 0.5f) modes += "zero motion";
    if (g_test_camera_matrix > 0.5f) modes += modes.empty() ? "camera matrix" : " / camera matrix";
    if (g_test_freeze_noise > 0.5f) modes += modes.empty() ? "frozen noise" : " / frozen noise";
    ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "TEST MODE ACTIVE: %s", modes.c_str());
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
    const TracedDenominators den = ComputeTracedDenominators(
        v[kRtaoStatPixels], v[kRtaoStatSky], v[kRtaoStatNormal], v[kRtaoStatRegion], v[kRtaoStatScaled],
        v[kRtaoStatTemporalBase + 5u]);
    ImGui::Text("Traced pixels: %u of %u (sky, normal, region and scaled are not traced)",
                den.traced, v[kRtaoStatPixels]);
    ImGui::Text("Temporal taps (unweighted): valid %.1f%%, rejected depth %.1f%%, rejected normal %.1f%%",
                SharePercent(v[kRtaoStatTemporalBase + 1u], den.taps),
                SharePercent(v[kRtaoStatTemporalBase + 2u], den.taps),
                SharePercent(v[kRtaoStatTemporalBase + 3u], den.taps));
    if (den.traced > 0u) {
      const double wden = 1000.0 * den.traced;
      const double w_acc = 100.0 * v[kRtaoStatWeightBase] / wden;
      const double w_dep = 100.0 * v[kRtaoStatWeightBase + 1u] / wden;
      const double w_nor = 100.0 * v[kRtaoStatWeightBase + 2u] / wden;
      ImGui::Text("Weighted taps (bilinear weights; share of traced pixels; not counted on reset frames): accepted %.1f%%, rejected depth %.1f%%, rejected normal %.1f%%",
                  w_acc, w_dep, w_nor);
      ImGui::Text("Weighted other (no history, out of bounds or reset): %.1f%%", 100.0 - w_acc - w_dep - w_nor);
      const double dn = static_cast<double>(den.traced);
      ImGui::Text("Depth ratio of the highest-weight tap (valid history, in bounds): invalid %.1f%%, <0.1%% %.1f%%, <0.5%% %.1f%%, <1%% %.1f%%, <2%% %.1f%%, <5%% %.1f%%, >=5%% %.1f%%",
                  100.0 * v[kRtaoStatDepthBase] / dn, 100.0 * v[kRtaoStatDepthBase + 1u] / dn, 100.0 * v[kRtaoStatDepthBase + 2u] / dn,
                  100.0 * v[kRtaoStatDepthBase + 3u] / dn, 100.0 * v[kRtaoStatDepthBase + 4u] / dn, 100.0 * v[kRtaoStatDepthBase + 5u] / dn,
                  100.0 * v[kRtaoStatDepthBase + 6u] / dn);
      ImGui::Text("Normal dot of the highest-weight tap (valid history, in bounds): <0 %.1f%%, <0.5 %.1f%%, <0.7 %.1f%%, <0.9 %.1f%%, <0.97 %.1f%%, >=0.97 %.1f%%",
                  100.0 * v[kRtaoStatNormalBase] / dn, 100.0 * v[kRtaoStatNormalBase + 1u] / dn, 100.0 * v[kRtaoStatNormalBase + 2u] / dn,
                  100.0 * v[kRtaoStatNormalBase + 3u] / dn, 100.0 * v[kRtaoStatNormalBase + 4u] / dn, 100.0 * v[kRtaoStatNormalBase + 5u] / dn);
      if (v[kRtaoStatRawBase + 1u] > 0u) {
        const double rn = static_cast<double>(v[kRtaoStatRawBase + 1u]);
        ImGui::Text("Frame-to-frame raw AO (history valid): mean |raw - previous raw| %.3f, identical %.1f%% (independent binary raw at p 0.217 gives about 0.34)",
                    v[kRtaoStatRawBase] / 1000.0 / rn, 100.0 * v[kRtaoStatRawBase + 2u] / rn);
      }
      const uint32_t* qv = &v[kRtaoStatQualityBase];
      ImGui::SeparatorText("Output quality (still camera only)");
      ImGui::SameLine();
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("These numbers compare this frame with the previous one at the same pixel. They are meaningful only when the camera and the scene are still (motion 0). With motion, the previous output is not aligned to the pixels and the roughness and change values include real movement.");
      }
      ImGui::Text("Mean AO: raw %.4f, output %.4f, raw - output %+.4f (positive = the temporal output is darker than raw)",
                  MeanFromSum(qv[1], den.traced), MeanFromSum(qv[0], den.traced), MeanFromSum(qv[1], den.traced) - MeanFromSum(qv[0], den.traced));
      if (qv[2] > 0u) {
        ImGui::Text("Spatial roughness (mean |pixel - mean of its 4 neighbours|, lower = smoother): raw %.4f, previous output %.4f, ratio %.2f",
                    MeanFromSum(qv[3], qv[2]), MeanFromSum(qv[4], qv[2]),
                    MeanFromSum(qv[3], qv[2]) > 0.0 ? MeanFromSum(qv[4], qv[2]) / MeanFromSum(qv[3], qv[2]) : 0.0);
      }
      if (qv[5] > 0u) {
        ImGui::Text("Output change per frame (mean |output - previous output|): %.4f over %u pixels; raw changes by %.4f (see the raw difference above)",
                    MeanFromSum(qv[6], qv[5]), qv[5], MeanFromSum(v[kRtaoStatRawBase], v[kRtaoStatRawBase + 1u]));
      }
    }
    ImGui::Text("Per traced pixel: out of bounds %.1f%%, reset %.1f%% (neutral pixels removed)",
                SharePercent(v[kRtaoStatTemporalBase + 4u], den.traced),
                SharePercent(den.reset_traced, den.traced));
    const uint32_t* f = &v[kRtaoStatTemporalFBase];
    if (den.traced > 0u) {
      const float px = static_cast<float>(den.traced);
      ImGui::Text("Motion: mean |motion| %.3f px, moving %.1f%%, max %.2f px",
                  f[2] / 100.f / px, 100.f * f[0] / px, f[1] / 100.f);
      ImGui::Text("Clamp: active %.1f%% of traced pixels, mean shift %.4f", 100.f * f[3] / px, f[4] / 1000.f / px);
      ImGui::Text("Blend: mean alpha %.3f, mean |raw - AO| %.4f, no history %.1f%%",
                  f[5] / 1000.f / px, f[6] / 1000.f / px, 100.f * f[7] / px);
    }
    ImGui::Text("Output differs from raw by more than 1 LSB: %.1f%% of traced pixels (valid with Debug Off only)",
                SharePercent(f[8], den.traced));
    const uint32_t* dv = &v[kRtaoStatTemporalFBase + 9u];
    ImGui::Text("Motion vs camera-matrix difference (valid for static geometry only: moving objects show real motion here, not a jitter error)");
    ImGui::Text("Within 0.25 px: %.1f%%, mean %.3f px, max %.2f px", SharePercent(dv[0] + dv[1], den.traced),
                den.traced > 0u ? dv[5] / 100.f / den.traced : 0.f, dv[6] / 100.f);
    float jitter_x = 0.f, jitter_y = 0.f;
    std::memcpy(&jitter_x, &v[kRtaoStatJitterX], sizeof(float));
    std::memcpy(&jitter_y, &v[kRtaoStatJitterY], sizeof(float));
    ImGui::Text("jitterDiff_g: (%.3f, %.3f) px", jitter_x, jitter_y);
    if (frame.matrix_valid) {
      ImGui::Text("Matrix self-check (indicative): |VP*VPinv-I| %.2e, |VP-prevVP| %.4f",
                  frame.matrix_identity_err, frame.matrix_prev_diff);
    }
  } else {
    ImGui::TextDisabled("Temporal percentages: press Read RTAO Stats");
  }
  if (frame.dispatched_frame != UINT64_MAX) {
    const ParameterSnapshot& c = frame.captured;
    ImGui::Text("Last dispatch: spp %.0f, history weight %.2f, depth rejection %.3f, normal rejection %.2f, history clamp %.2f, IS-FAST %s",
                c.samples, c.history_weight, c.depth_rejection, c.normal_rejection, c.history_clamp,
                frame.captured_isfast_used ? "used" : "off");
    ImGui::Text("Last dispatch: radius %.2f, ray max %.2f, strength %.2f, normal bias %.4f, debug %.0f, tests zero motion %s, freeze noise %s, camera matrix %s",
                c.radius, c.ray_max, c.strength, c.normal_bias, c.debug,
                c.zero_motion > 0.5f ? "on" : "off", c.freeze_noise > 0.5f ? "on" : "off", c.camera_matrix > 0.5f ? "on" : "off");
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
  if (frame.filter_requested) {
    const uint32_t* v = g_rtao_stats.values;
    ImGui::SeparatorText("Denoising");
    ImGui::Text("Spatial filter: on, %s, radius %d, quality %s (iterations %u, passes %u, taps per pass %u)",
                frame.filter_type == 0 ? "separable bilateral" : "a-trous 5x5", frame.filter_radius,
                frame.filter_quality == 0 ? "Low" : (frame.filter_quality == 1 ? "Medium" : "High"),
                frame.filter_iterations, frame.filter_passes, frame.filter_taps);
    if (frame.filter_gpu_ms < 0.f) {
      ImGui::Text("Filter GPU: no measurement yet");
    } else {
      ImGui::Text("Filter GPU: %.3f ms", frame.filter_gpu_ms);
    }
    const uint32_t pixels = v[kRtaoStatFilterBase];
    ImGui::Text("Mean change: %.4f (share of pixels changed by more than 1 LSB: %.1f%%) over %u pixels",
                MeanFromSum(v[kRtaoStatFilterBase + 1u], pixels),
                pixels > 0u ? 100.0 * v[kRtaoStatFilterBase + 2u] / pixels : 0.0, pixels);
    ImGui::TextDisabled("Debug modes 1 to 7 bypass the spatial filter; mode 8 shows the filter change.");
    if (frame.filter_failed) {
      ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "filter_failed: using the unfiltered AO");
    }
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
