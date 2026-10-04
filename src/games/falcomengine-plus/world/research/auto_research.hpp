#pragma once

// Phase 0 automated research.
//
// Opportunistic set-arming: every eligible family for the current round is
// armed at once, so the next eligible draw of ANY family is captured as the
// user plays normally. No per-family camera positioning and no per-family
// waiting. A family is runtime-verified when at least two captures agree on the
// same transform signature and pass depth validation. Decompilation remains a
// separate, later confirmation.

#include <algorithm>
#include <string>
#include <unordered_map>

#include "../world_state.hpp"
#include "transform_candidates.hpp"
#include "../capture/depth_probe.hpp"
#include "coverage.hpp"
#include "world_report.hpp"

namespace falcom_world {

inline bool ProbeCandidatePass(const ProbeMetrics& probe, float threshold) {
  if (!probe.valid) return false;
  if (probe.match < kMinDepthMatches) return false;
  const uint32_t denom = probe.match + probe.mismatch;
  if (denom == 0u) return false;
  return probe.match_ratio >= threshold;
}

inline uint64_t CaptureSignature(const AutoCapture& capture) {
  TransformCandidate temp;
  temp.kind = capture.kind;
  temp.slot = capture.slot;
  temp.matrix_offset = capture.matrix_offset;
  temp.stride = capture.stride;
  temp.base_offset = capture.base_offset;
  return CandidateSignature(temp);
}

inline AutoFamilyResult* FindAutoResult(AutoState* state, uint32_t vs_hash) {
  if (state == nullptr) return nullptr;
  for (auto& result : state->results) {
    if (result.vs_hash == vs_hash) return &result;
  }
  return nullptr;
}

inline void StartAutoResearch() {
  if (!CensusEnabled()) return;
  std::lock_guard<std::mutex> lock(g_state.mutex);
  auto& state = g_state.auto_research;
  state = {};
  state.active = true;
  state.rerun_all = g_state.setting_auto_rerun > 0.5f;
  const int rounds = static_cast<int>(g_state.setting_auto_rounds + 0.5f);
  state.rounds = static_cast<uint32_t>(rounds < 1 ? 1 : (rounds > 10 ? 10 : rounds));
  state.start_frame = g_state.frame.load();
  state.deadline_frame = state.start_frame + kAutoRunTimeoutFrames;
  state.next_capture_frame = state.start_frame;

  struct Entry {
    uint32_t hash;
    uint64_t triangles;
  };
  std::vector<Entry> entries;
  for (const auto& [hash, family] : g_state.families) {
    if (!family.candidate) continue;
    if (family.verified && !state.rerun_all) continue;
    entries.push_back({hash, family.triangles});
  }
  std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
    return a.triangles > b.triangles;
  });
  for (const auto& entry : entries) {
    state.queue.push_back(entry.hash);
    AutoFamilyResult result;
    result.vs_hash = entry.hash;
    state.results.push_back(result);
  }
  state.pending = state.queue;
  state.last_vs = 0u;
  g_state.arm_active = !state.pending.empty();
  g_state.arm_vs_hash = 0u;
  g_state.arm_serial = 0u;
  g_state.arm_vs_set = state.pending;
  g_state.status = "auto research started: " + std::to_string(state.queue.size()) + " families";
}

inline void StopAutoResearch() {
  std::lock_guard<std::mutex> lock(g_state.mutex);
  g_state.auto_research.stop_requested = true;
}

inline void FinalizeAutoResearch() {
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto& state = g_state.auto_research;
    for (auto& result : state.results) {
      std::unordered_map<uint64_t, uint32_t> pass_counts;
      std::unordered_map<uint64_t, const AutoCapture*> best_capture;
      float best_ratio = 0.f;
      uint32_t passes = 0u;
      for (const auto& capture : result.captures) {
        if (!capture.pass) continue;
        passes += 1u;
        const uint64_t signature = CaptureSignature(capture);
        pass_counts[signature] += 1u;
        best_capture[signature] = &capture;
        best_ratio = (std::max)(best_ratio, capture.probe.match_ratio);
      }
      uint64_t best_signature = 0u;
      uint32_t best_count = 0u;
      for (const auto& [signature, count] : pass_counts) {
        if (count > best_count) {
          best_count = count;
          best_signature = signature;
        }
      }

      result.depth_pass = passes > 0u;
      result.cross_capture_pass = best_count >= 2u;
      result.runtime_verified = best_count >= 2u;
      if (result.runtime_verified) {
        result.verdict = static_cast<uint8_t>(AutoVerdict::Verified);
      } else if (passes > 0u) {
        result.verdict = static_cast<uint8_t>(AutoVerdict::VerifiedSingleCapture);
        result.failure_reason = "only one passing capture; re-run or observe more draws";
      } else if (!result.captures.empty()) {
        result.verdict = static_cast<uint8_t>(AutoVerdict::Failed);
        result.failure_reason = result.captures.back().failure_reason.empty()
                                    ? "all_candidates_failed_depth"
                                    : result.captures.back().failure_reason;
      } else {
        result.verdict = static_cast<uint8_t>(AutoVerdict::NotObserved);
        result.failure_reason = "no_draw_observed";
      }

      const auto family_it = g_state.families.find(result.vs_hash);
      if (family_it != g_state.families.end()) {
        FamilyStats& family = family_it->second;
        family.auto_verdict = result.verdict;
        family.captures_attempted = static_cast<uint32_t>(result.captures.size());
        family.captures_ok = passes;
        family.depth_match_ratio = best_ratio;
        if (result.runtime_verified && best_capture.count(best_signature) != 0u) {
          const AutoCapture* capture = best_capture[best_signature];
          family.verified = true;
          family.transform_found = true;
          family.transform_kind = static_cast<uint8_t>(capture->kind);
          family.transform_slot = capture->slot;
          family.transform_offset = capture->matrix_offset;
          family.transform_stride = capture->stride;
          family.transform_base = capture->base_offset;
          family.transform_has_prev = !capture->prev_matrices.empty();
        }
      }
    }
    state.active = false;
    state.finalized = true;
    state.pending.clear();
    state.deadline_frame = 0u;
    g_state.arm_active = false;
    g_state.arm_vs_set.clear();
    g_state.arm_vs_hash = 0u;
  }
  WriteAutoReport();
  SetStatus("auto research complete");
}

inline void ProcessAutoCapture(
    reshade::api::command_list* cmd_list,
    reshade::api::command_queue* queue,
    uint32_t vs_hash) {
  ScanCandidates();

  std::vector<TransformCandidate> candidates;
  DrawRecord draw;
  CameraSnapshot camera;
  DepthSource depth;
  bool mesh_valid = false;
  std::string mesh_status;
  uint32_t srv_mask = 0u;
  int32_t instance_offset = 0;
  uint32_t read_stride = 0u;
  uint64_t read_offset = 0u;
  uint64_t read_size = 0u;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    candidates = g_state.candidates;
    draw = g_state.captured.draw;
    camera = g_state.camera;
    depth = g_state.depth_source;
    mesh_valid = g_state.captured.mesh_valid;
    mesh_status = g_state.captured.mesh_status;
    srv_mask = draw.vs_srv_mask;
    instance_offset = g_state.captured.instance_offset_g;
    for (const auto& buffer : g_state.captured.srv_buffers) {
      read_stride = buffer.stride;
      read_offset = buffer.read_offset;
      read_size = buffer.read_size;
      break;
    }
  }

  AutoCapture capture;
  capture.frame = draw.frame;
  capture.draw_serial = draw.serial;
  capture.instances_tested = draw.instance_count == 0u ? 1u : draw.instance_count;

  const float threshold = g_state.setting_probe_threshold;
  const float tol_rel = g_state.setting_probe_tol_rel;
  const float tol_abs = g_state.setting_probe_tol_abs;

  if (!mesh_valid) {
    capture.failure_reason = "mesh_capture_failed: " + mesh_status;
  } else if (candidates.empty()) {
    capture.failure_reason = "no_instance_candidates";
  } else {
    std::vector<uint32_t> indices;
    CollectProbeCandidates(candidates, &indices, kProbeTopCandidates);
    ProbeBuild build;
    BuildProbeForCandidates(candidates, indices, g_state.captured, camera, &build);

    std::vector<ProbeMetrics> metrics;
    bool depth_ok = false;
    if (depth.valid && depth.frame == draw.frame) {
      depth_ok = RunDepthProbe(queue->get_device(), cmd_list, queue, depth, &build, tol_rel, tol_abs, &metrics);
    }

    int best = -1;
    float best_score = -1.f;
    for (uint32_t i = 0; i < indices.size(); ++i) {
      const TransformCandidate& candidate = candidates[indices[i]];
      const bool probe_pass = i < metrics.size() && ProbeCandidatePass(metrics[i], threshold);
      const float ratio = i < metrics.size() ? metrics[i].match_ratio : 0.f;
      const float score = (probe_pass ? 1000.f : 0.f) + ratio * 100.f + candidate.set_score;
      if (score > best_score) {
        best_score = score;
        best = static_cast<int>(i);
      }
    }

    if (best >= 0) {
      const TransformCandidate& candidate = candidates[indices[static_cast<size_t>(best)]];
      capture.kind = candidate.kind;
      capture.stage = candidate.stage;
      capture.slot = candidate.slot;
      capture.matrix_offset = candidate.matrix_offset;
      capture.stride = candidate.stride;
      capture.base_offset = candidate.base_offset;
      capture.instances_tested = candidate.element_count;
      capture.instances_ok = candidate.instances_ok;
      capture.projection_score = candidate.set_score;
      capture.candidate_valid = true;
      if (best < static_cast<int>(metrics.size())) capture.probe = metrics[static_cast<size_t>(best)];
      const bool projection_ok = candidate.instances_ok * 10u >= candidate.element_count * 9u;
      const bool probe_pass = capture.probe.valid && ProbeCandidatePass(capture.probe, threshold);
      capture.depth_ok = probe_pass;
      capture.pass = projection_ok && probe_pass;

      const uint32_t matrix_floats = KindMatrixFloats(candidate.kind);
      const uint32_t bounded_instances = (std::min)(candidate.element_count, 4u);
      for (uint32_t i = 0; i < bounded_instances; ++i) {
        const size_t offset = static_cast<size_t>(i) * matrix_floats;
        if (candidate.matrices.size() < offset + matrix_floats) break;
        capture.matrices.insert(
            capture.matrices.end(),
            candidate.matrices.begin() + static_cast<ptrdiff_t>(offset),
            candidate.matrices.begin() + static_cast<ptrdiff_t>(offset + matrix_floats));
      }
      const uint32_t prev_bounded = (std::min)(static_cast<uint32_t>(candidate.prev_matrices.size() / 12u), 4u);
      for (uint32_t i = 0; i < prev_bounded; ++i) {
        const size_t offset = static_cast<size_t>(i) * 12u;
        capture.prev_matrices.insert(
            capture.prev_matrices.end(),
            candidate.prev_matrices.begin() + static_cast<ptrdiff_t>(offset),
            candidate.prev_matrices.begin() + static_cast<ptrdiff_t>(offset + 12u));
      }
      if (!capture.pass) {
        if (!projection_ok) {
          capture.failure_reason = "projection_instances_low";
        } else if (!capture.probe.valid) {
          capture.failure_reason = "depth_validation_unavailable";
        } else if (capture.probe.match < kMinDepthMatches) {
          capture.failure_reason = "depth_matches_below_min";
        } else {
          capture.failure_reason = "depth_match_ratio_low";
        }
      }
    } else {
      capture.failure_reason = depth_ok ? "no_probe_candidates" : "depth_validation_unavailable";
    }
  }

  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto* result = FindAutoResult(&g_state.auto_research, vs_hash);
    if (result != nullptr) {
      result->captures.push_back(capture);
      result->srv_mask = srv_mask;
      result->instance_offset = instance_offset;
      result->read_stride = read_stride;
      result->read_offset = read_offset;
      result->read_size = read_size;
    }
    g_state.auto_research.last_processed_serial = draw.serial;
    g_state.auto_research.families_done += 1u;
  }
}

inline void RunAutoResearch(reshade::api::command_list* cmd_list, reshade::api::command_queue* queue) {
  if (cmd_list == nullptr || queue == nullptr) return;

  bool stop = false;
  bool capture_pending = false;
  uint64_t frame = 0u;
  uint64_t deadline = 0u;
  bool ready = false;
  uint32_t ready_vs = 0u;
  uint32_t ready_serial = 0u;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto& state = g_state.auto_research;
    if (!state.active) return;
    if (!CensusEnabled()) state.stop_requested = true;
    stop = state.stop_requested;
    capture_pending = g_state.mesh_capture_pending;
    frame = g_state.frame.load();
    deadline = state.deadline_frame;
    const uint32_t serial = g_state.captured.draw.serial;
    const uint32_t vs = g_state.captured.draw.vs_hash;
    if (serial != 0u && serial != state.last_processed_serial) {
      if (std::find(state.pending.begin(), state.pending.end(), vs) != state.pending.end()) {
        ready = true;
        ready_vs = vs;
        ready_serial = serial;
      }
    }
  }

  if (stop || frame > deadline) {
    FinalizeAutoResearch();
    return;
  }
  if (capture_pending) return;

  if (ready) {
    ProcessAutoCapture(cmd_list, queue, ready_vs);
    bool finished = false;
    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      auto& state = g_state.auto_research;
      state.last_vs = ready_vs;
      state.last_processed_serial = ready_serial;
      state.next_capture_frame = frame + kAutoCaptureSpacingFrames;
      const auto* result = FindAutoResult(&state, ready_vs);
      if (result == nullptr || result->captures.size() >= state.rounds) {
        state.pending.erase(std::remove(state.pending.begin(), state.pending.end(), ready_vs), state.pending.end());
      }
      if (state.pending.empty()) finished = true;
      // Disarm after every capture; the spacing gate re-arms on a later present.
      g_state.arm_active = false;
      g_state.arm_vs_set.clear();
    }
    if (finished) FinalizeAutoResearch();
    return;
  }

  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto& state = g_state.auto_research;
    if (!g_state.arm_active && !state.pending.empty() && frame >= state.next_capture_frame) {
      g_state.arm_active = true;
      g_state.arm_vs_set = state.pending;
      g_state.arm_vs_hash = 0u;
      g_state.arm_serial = 0u;
    }
  }
}

inline void OnWorldPresentAuto(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  (void)swapchain;
  (void)source_rect;
  (void)dest_rect;
  (void)dirty_rect_count;
  (void)dirty_rects;
  if (queue == nullptr) return;
  RunAutoResearch(queue->get_immediate_command_list(), queue);
}

}  // namespace falcom_world
