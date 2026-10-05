#pragma once

// Phase 0 automated research.
//
// Opportunistic set-arming: every eligible family is armed at once, so the next
// eligible draw of ANY family is captured as the user plays normally. A family
// is runtime-verified when at least two captures agree on the same structural
// transform signature and pass depth validation. Decompilation remains a
// separate, later confirmation.
//
// Hint diagnostics: for families with a manual structured-instance hint, the
// exact manual hypothesis (slot, offset, layout, prev) is forced into the probe
// set as one candidate among others. It never bypasses depth validation and is
// reported with source = hint_diagnostic.

#include <algorithm>
#include <numeric>
#include <string>
#include <unordered_map>

#include "../world_state.hpp"
#include "transform_candidates.hpp"
#include "classification.hpp"
#include "reference_hints.hpp"
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
  return CandidateSignature(temp);
}

inline AutoFamilyResult* FindAutoResult(AutoState* state, uint32_t vs_hash) {
  if (state == nullptr) return nullptr;
  for (auto& result : state->results) {
    if (result.vs_hash == vs_hash) return &result;
  }
  return nullptr;
}

inline uint32_t ComputeMinEligible(AutoState* state) {
  uint32_t min_captures = UINT32_MAX;
  for (const uint32_t vs_hash : state->pending) {
    if (state->deferred.count(vs_hash) != 0u) continue;
    const AutoFamilyResult* result = FindAutoResult(state, vs_hash);
    const uint32_t count = result != nullptr ? static_cast<uint32_t>(result->captures.size()) : 0u;
    min_captures = (std::min)(min_captures, count);
  }
  return min_captures;
}

inline bool HasEligiblePending(AutoState* state) {
  for (const uint32_t vs_hash : state->pending) {
    if (state->deferred.count(vs_hash) == 0u) return true;
  }
  return false;
}

inline void StartRound(AutoState* state, uint32_t min_captures) {
  state->round_active = min_captures != UINT32_MAX;
  state->round_target_count = state->round_active ? min_captures : 0u;
  state->round_budget = state->round_active ? static_cast<uint32_t>(state->pending.size()) : 0u;
  state->round_windows_used = 0u;
  state->round_cohort.clear();
  state->round_armed_draws.clear();
  if (!state->round_active) return;
  for (const uint32_t vs_hash : state->pending) {
    if (state->deferred.count(vs_hash) != 0u) continue;
    const AutoFamilyResult* result = FindAutoResult(state, vs_hash);
    const uint32_t count = result != nullptr ? static_cast<uint32_t>(result->captures.size()) : 0u;
    if (count == min_captures) state->round_cohort.push_back(vs_hash);
  }
  state->rounds_started += 1u;
}

// Defers a family that could not be observed. The deferral is temporary: after
// a cooldown the family becomes eligible again so a later camera position can
// observe its draws, up to kAutoDeferredMaxRetries times.
inline void DeferFamily(AutoState* state, uint32_t vs_hash) {
  if (state == nullptr) return;
  if (state->deferred.insert(vs_hash).second) {
    AutoFamilyResult* result = FindAutoResult(state, vs_hash);
    if (result != nullptr) result->deferred_no_draw = true;
  }
  state->deferred_until[vs_hash] = g_state.frame.load() + kAutoDeferredRetryCooldownFrames;
}

inline void EndRoundAndDefer(AutoState* state) {
  for (const uint32_t vs_hash : state->round_cohort) {
    if (state->deferred.count(vs_hash) != 0u) continue;
    AutoFamilyResult* result = FindAutoResult(state, vs_hash);
    if (result == nullptr) continue;
    if (result->captures.size() != state->round_target_count) continue;
    const auto armed_it = state->round_armed_draws.find(vs_hash);
    if (armed_it != state->round_armed_draws.end() && armed_it->second > 0u) continue;
    DeferFamily(state, vs_hash);
  }
  state->round_active = false;
  state->round_target_count = 0u;
  state->round_budget = 0u;
  state->round_windows_used = 0u;
  state->round_cohort.clear();
  state->round_armed_draws.clear();
}

inline void ArmMinimumCaptureCohort(AutoState* state) {
  g_state.arm_vs_set.clear();
  if (state == nullptr || !state->round_active) return;
  for (const uint32_t vs_hash : state->round_cohort) {
    if (state->deferred.count(vs_hash) != 0u) continue;
    const AutoFamilyResult* result = FindAutoResult(state, vs_hash);
    const uint32_t count = result != nullptr ? static_cast<uint32_t>(result->captures.size()) : 0u;
    if (count == state->round_target_count) g_state.arm_vs_set.push_back(vs_hash);
  }
}

// Builds the exact manual hypothesis as a candidate. No silent offset/layout
// changes: kind = 4x3 row, offset = hint.world_offset, stride = runtime stride.
inline bool BuildHintDiagnosticCandidate(
    const CapturedDraw& captured,
    const CameraSnapshot& camera,
    const ReferenceHint& hint,
    int32_t base,
    TransformCandidate* out) {
  if (out == nullptr || !hint.structured_instance || base < 0) return false;

  const SrvBufferSnapshot* buffer = nullptr;
  for (const auto& candidate_buffer : captured.srv_buffers) {
    if (!candidate_buffer.valid || candidate_buffer.slot != hint.instance_slot || candidate_buffer.stride == 0u) continue;
    if (buffer == nullptr || candidate_buffer.stride == 160u) buffer = &candidate_buffer;
    if (candidate_buffer.stride == 160u) break;
  }
  if (buffer == nullptr) return false;

  TransformCandidate candidate;
  candidate.kind = CandidateKind::Inst4x3Row;
  candidate.source = CandidateSource::HintDiagnostic;
  candidate.row_dot = true;  // audited row-dot convention from the captured VS
  candidate.stage = buffer->stage;
  candidate.slot = buffer->slot;
  candidate.matrix_offset = hint.world_offset;
  candidate.stride = buffer->stride;
  candidate.base_offset = static_cast<uint32_t>(base);
  candidate.element_count = (std::min)(
      captured.draw.instance_count == 0u ? 1u : captured.draw.instance_count, kMaxCandidateInstances);
  if (!ExtractInstanceMatrices(
          *buffer, captured.draw, candidate.base_offset, candidate.matrix_offset, 12u,
          candidate.element_count, &candidate.matrices, &candidate.prev_matrices)) {
    return false;
  }
  candidate.has_prev = !candidate.prev_matrices.empty();

  const SetScore score = ScoreCandidateSet(candidate, camera, captured.mesh.positions, 32u, 32u);
  candidate.set_score = score.set_score;
  candidate.mean_inside = score.mean_inside;
  candidate.bbox_area = score.bbox_area;
  candidate.ndc_z_valid_ratio = score.ndc_z_valid_ratio;
  candidate.projection_valid = score.valid;
  candidate.instances_ok = score.instances_ok;
  candidate.valid = true;
  *out = std::move(candidate);
  return true;
}

inline std::vector<int32_t> HintDiagnosticBases(const CapturedDraw& captured) {
  std::vector<int32_t> bases;
  const auto add = [&bases](int32_t base) {
    if (base < 0) return;
    if (std::find(bases.begin(), bases.end(), base) != bases.end()) return;
    bases.push_back(base);
  };
  if (captured.instance_offset_found) add(captured.instance_offset_g);
  add(0);
  return bases;
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
  state.deadline_frame = state.start_frame
                         + static_cast<uint64_t>(state.queue.size()) * state.rounds
                               * ((kAutoCaptureSpacingFrames + 1u) + kProbeRetryBudget + kNoDrawDeferralWindows)
                         + kAutoDeadlineMarginFrames;
  StartRound(&state, ComputeMinEligible(&state));
  ArmMinimumCaptureCohort(&state);
  g_state.arm_active = !g_state.arm_vs_set.empty();
  g_state.arm_vs_hash = 0u;
  g_state.arm_serial = 0u;
  g_state.arm_window_open = g_state.arm_active;
  if (g_state.arm_active) {
    state.round_windows_used += 1u;
    state.arm_windows += 1u;
  }
  state.arm_set_size = static_cast<uint32_t>(g_state.arm_vs_set.size());
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
        result.srv_buffers = capture.srv_buffers;
        result.instance_offset = capture.instance_offset_g;
        result.instance_offset_found = capture.instance_offset_found;
        result.hint_diagnostics = capture.hint_diagnostics;
      }
      if (passes == 0u && !result.captures.empty()) {
        result.srv_buffers = result.captures.back().srv_buffers;
        result.instance_offset = result.captures.back().instance_offset_g;
        result.instance_offset_found = result.captures.back().instance_offset_found;
        result.hint_diagnostics = result.captures.back().hint_diagnostics;
      }
      uint64_t best_signature = 0u;
      uint32_t best_count = 0u;
      for (const auto& [signature, count] : pass_counts) {
        if (count > best_count) {
          best_count = count;
          best_signature = signature;
        }
      }

      result.passing_layouts.clear();
      result.passing_layouts.reserve(pass_counts.size());
      for (const auto& [signature, count] : pass_counts) {
        const AutoCapture* capture = best_capture[signature];
        PassingLayout layout;
        layout.kind = capture->kind;
        layout.stage = capture->stage;
        layout.slot = capture->slot;
        layout.matrix_offset = capture->matrix_offset;
        layout.stride = capture->stride;
        layout.base_offset = capture->base_offset;
        layout.count = count;
        result.passing_layouts.push_back(layout);
      }
      std::sort(result.passing_layouts.begin(), result.passing_layouts.end(),
                [](const PassingLayout& a, const PassingLayout& b) {
                  if (a.count != b.count) return a.count > b.count;
                  if (a.slot != b.slot) return a.slot < b.slot;
                  if (a.matrix_offset != b.matrix_offset) return a.matrix_offset < b.matrix_offset;
                  return static_cast<uint8_t>(a.kind) < static_cast<uint8_t>(b.kind);
                });

      result.depth_pass = passes > 0u;
      result.cross_capture_pass = best_count >= 2u;
      // Relaxed verification: two passing captures regardless of structural
      // signature. cross_capture_pass remains the strict same-signature metric.
      result.runtime_verified = passes >= 2u;
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

    state.pending_at_end = static_cast<uint32_t>(state.pending.size());
    for (auto& result : state.results) {
      const bool was_pending =
          std::find(state.pending.begin(), state.pending.end(), result.vs_hash) != state.pending.end();
      result.pending_at_end = was_pending;
      const auto armed_it = state.armed_draws.find(result.vs_hash);
      result.armed_draws = armed_it != state.armed_draws.end() ? armed_it->second : 0u;
      const auto retry_it = state.probe_retry_totals.find(result.vs_hash);
      result.probe_retries = retry_it != state.probe_retry_totals.end() ? retry_it->second : 0u;
      if (result.captures.size() >= state.rounds) {
        result.skip_reason = "completed";
      } else if (result.deferred_no_draw) {
        result.skip_reason = "deferred_no_draw";
      } else if (was_pending && result.captures.empty() && result.armed_draws > 0u) {
        result.skip_reason = "starved_no_arm_win";
      } else if (was_pending && result.captures.empty()) {
        result.skip_reason = "no_draw_in_armed_windows";
      } else if (was_pending) {
        result.skip_reason = "partial_rounds";
      } else {
        result.skip_reason = "removed_early";
      }
    }
    if (state.finalize_reason.empty()) {
      state.finalize_reason = "unknown";
    }
    state.active = false;
    state.finalized = true;
    state.pending.clear();
    state.deadline_frame = 0u;
    g_state.arm_active = false;
    g_state.arm_window_open = false;
    g_state.arm_vs_set.clear();
    g_state.arm_vs_hash = 0u;
  }
  WriteAutoReport();
  SetStatus("auto research complete");
}

inline bool ProcessAutoCapture(
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
  bool instance_offset_found = false;
  std::vector<SrvBufferSummary> srv_summaries;
  uint32_t probe_retries = 0u;
  uint64_t preferred_signature = 0u;
  bool has_preferred_signature = false;
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
    instance_offset_found = g_state.captured.instance_offset_found;
    const auto retry_it = g_state.auto_research.probe_retry_counts.find(vs_hash);
    probe_retries = retry_it != g_state.auto_research.probe_retry_counts.end() ? retry_it->second : 0u;
    const auto* family_result = FindAutoResult(&g_state.auto_research, vs_hash);
    if (family_result != nullptr) {
      std::unordered_map<uint64_t, uint32_t> pass_counts;
      for (const auto& capture : family_result->captures) {
        if (!capture.pass) continue;
        pass_counts[CaptureSignature(capture)] += 1u;
      }
      uint32_t best_count = 0u;
      for (const auto& [signature, count] : pass_counts) {
        if (count > best_count) {
          best_count = count;
          preferred_signature = signature;
          has_preferred_signature = true;
        }
      }
    }
    for (const auto& buffer : g_state.captured.srv_buffers) {
      SrvBufferSummary summary;
      summary.slot = buffer.slot;
      summary.stride = buffer.stride;
      summary.size = buffer.size;
      summary.read_offset = buffer.read_offset;
      summary.read_size = buffer.read_size;
      summary.truncated = buffer.truncated;
      srv_summaries.push_back(summary);
    }
  }

  AutoCapture capture;
  capture.frame = draw.frame;
  capture.draw_serial = draw.serial;
  capture.camera_frame = camera.frame;
  capture.draw_instance_count = draw.instance_count;
  capture.instance_offset_g = instance_offset;
  capture.instance_offset_found = instance_offset_found;
  capture.instances_tested = draw.instance_count == 0u ? 1u : draw.instance_count;
  capture.srv_buffers = srv_summaries;
  bool capture_blind = false;
  if (draw.dsv.handle != 0u && depth.valid && queue != nullptr && queue->get_device() != nullptr) {
    const auto dsv_resource = queue->get_device()->get_resource_from_view(draw.dsv);
    capture.dsv_matches_depth_source = dsv_resource.handle != 0u && dsv_resource.handle == depth.resource.handle;
  }

  const float threshold = g_state.setting_probe_threshold;
  const float tol_rel = g_state.setting_probe_tol_rel;
  const float tol_abs = g_state.setting_probe_tol_abs;

  if (!mesh_valid) {
    capture.failure_reason = "mesh_capture_failed: " + mesh_status;
  } else if (candidates.empty()) {
    capture.failure_reason = "no_instance_candidates";
  } else {
    // Probe list: forced hint diagnostics first, then generic top candidates.
    std::vector<TransformCandidate> probe_candidates;
    std::vector<uint32_t> hint_positions;
    const ReferenceHint* hint = FindReferenceHint(vs_hash);
    if (hint != nullptr && hint->structured_instance) {
      for (const int32_t base : HintDiagnosticBases(g_state.captured)) {
        TransformCandidate diagnostic;
        if (BuildHintDiagnosticCandidate(g_state.captured, camera, *hint, base, &diagnostic)) {
          hint_positions.push_back(static_cast<uint32_t>(probe_candidates.size()));
          probe_candidates.push_back(std::move(diagnostic));
        }
      }
    }
    std::vector<uint32_t> generic_indices;
    CollectProbeCandidates(candidates, &generic_indices, kProbeTopCandidates);
    for (const uint32_t index : generic_indices) {
      if (probe_candidates.size() >= kProbeTopCandidates) break;
      probe_candidates.push_back(candidates[index]);
    }

    // Force the family's preferred passing signature back into the probe list
    // when the scanner ranked it below the generic cut, so cross-capture
    // verification can revisit it. Inclusion only; scoring is unchanged.
    if (has_preferred_signature) {
      bool present = false;
      for (const auto& candidate : probe_candidates) {
        if (CandidateSignature(candidate) == preferred_signature) {
          present = true;
          break;
        }
      }
      if (!present) {
        const TransformCandidate* forced = nullptr;
        for (const auto& candidate : candidates) {
          if (CandidateSignature(candidate) != preferred_signature) continue;
          if (instance_offset_found && candidate.base_offset == static_cast<uint32_t>(instance_offset)) {
            forced = &candidate;
            break;
          }
          if (forced == nullptr) forced = &candidate;
        }
        if (forced != nullptr) {
          TransformCandidate candidate = *forced;
          candidate.source = CandidateSource::PreferredSignature;
          if (probe_candidates.size() < kProbeTopCandidates) {
            probe_candidates.push_back(std::move(candidate));
          } else if (probe_candidates.size() > hint_positions.size()) {
            probe_candidates.back() = std::move(candidate);
          }
        }
      }
    }

    std::vector<uint32_t> probe_indices(probe_candidates.size());
    std::iota(probe_indices.begin(), probe_indices.end(), 0u);
    ProbeBuild build;
    BuildProbeForCandidates(probe_candidates, probe_indices, g_state.captured, camera, &build);

    std::vector<ProbeMetrics> metrics;
    bool depth_ok = false;
    if (depth.valid && depth.frame == draw.frame) {
      depth_ok = RunDepthProbe(queue->get_device(), cmd_list, queue, depth, &build, tol_rel, tol_abs, &metrics);
    }

    bool probe_viable = true;
    if (hint != nullptr && hint->structured_instance && !hint_positions.empty() && !metrics.empty()) {
      probe_viable = false;
      for (const uint32_t position : hint_positions) {
        if (position >= metrics.size()) continue;
        const ProbeMetrics& probe = metrics[position];
        if (probe.valid && probe.match + probe.mismatch > 0u) {
          probe_viable = true;
          break;
        }
      }
    }
    // Visibility-blind retry: when nothing passed and the top structural
    // candidate got no decisive samples (all occluded / off-screen), the draw
    // is inconclusive rather than wrong; reject it so a later draw of the same
    // family is probed instead. Draw-key skipping keeps the retry bounded.
    if (probe_viable && !metrics.empty()) {
      bool any_probe_pass = false;
      for (uint32_t i = 0; i < probe_candidates.size() && i < metrics.size(); ++i) {
        if (ProbeCandidatePass(metrics[i], threshold)) {
          any_probe_pass = true;
          break;
        }
      }
      if (!any_probe_pass) {
        int best_structural = -1;
        float best_structural_score = -1.f;
        for (uint32_t i = 0; i < probe_candidates.size() && i < metrics.size(); ++i) {
          if (!metrics[i].valid) continue;
          const TransformCandidate& candidate = probe_candidates[i];
          const bool projection_ok = candidate.projection_valid
                                     && candidate.instances_ok * 10u >= candidate.element_count * 9u;
          if (!projection_ok || candidate.ndc_z_valid_ratio < 0.5f) continue;
          if (candidate.set_score > best_structural_score) {
            best_structural_score = candidate.set_score;
            best_structural = static_cast<int>(i);
          }
        }
        if (best_structural >= 0) {
          const ProbeMetrics& probe = metrics[static_cast<size_t>(best_structural)];
          if (probe.match + probe.mismatch == 0u) probe_viable = false;
        }
      }
    }
    if (!probe_viable && probe_retries < kProbeRetryBudget) {
      return false;
    }
    capture_blind = !probe_viable;

    int best = -1;
    float best_score = -1.f;
    for (uint32_t i = 0; i < probe_candidates.size(); ++i) {
      const TransformCandidate& candidate = probe_candidates[i];
      const bool probe_pass = i < metrics.size() && ProbeCandidatePass(metrics[i], threshold);
      const float ratio = i < metrics.size() ? metrics[i].match_ratio : 0.f;
      float score = (probe_pass ? 1000.f : 0.f) + ratio * 100.f
                    + candidate.ndc_z_valid_ratio * 10.f + candidate.set_score;
      if (probe_pass && has_preferred_signature && CandidateSignature(candidate) == preferred_signature) {
        score += 500.f;
      }
      if (score > best_score) {
        best_score = score;
        best = static_cast<int>(i);
      }
    }

    for (uint32_t i = 0; i < probe_candidates.size(); ++i) {
      const TransformCandidate& candidate = probe_candidates[i];
      CandidateDiagnosticRecord record;
      record.source = candidate.source;
      record.kind = candidate.kind;
      record.stage = candidate.stage;
      record.slot = candidate.slot;
      record.matrix_offset = candidate.matrix_offset;
      record.stride = candidate.stride;
      record.base_offset = candidate.base_offset;
      record.projection_valid = candidate.projection_valid;
      record.projection_score = candidate.set_score;
      record.ndc_z_valid_ratio = candidate.ndc_z_valid_ratio;
      if (i < metrics.size()) {
        record.match = metrics[i].match;
        record.occluded = metrics[i].occluded;
        record.mismatch = metrics[i].mismatch;
        record.out_of_screen = metrics[i].out_of_screen;
        record.match_ratio = metrics[i].match_ratio;
      }
      const bool projection_ok = candidate.projection_valid
                                 && candidate.instances_ok * 10u >= candidate.element_count * 9u;
      record.pass = projection_ok && i < metrics.size() && metrics[i].valid
                    && ProbeCandidatePass(metrics[i], threshold);
      record.selected = best == static_cast<int>(i);
      capture.candidate_diagnostics.push_back(std::move(record));
    }

    for (const uint32_t position : hint_positions) {
      if (position >= probe_candidates.size()) continue;
      const TransformCandidate& candidate = probe_candidates[position];
      HintDiagnosticRecord record;
      record.valid = true;
      record.kind = candidate.kind;
      record.slot = candidate.slot;
      record.matrix_offset = candidate.matrix_offset;
      record.stride = candidate.stride;
      record.base = static_cast<int32_t>(candidate.base_offset);
      record.projection_valid = candidate.projection_valid;
      record.projection_score = candidate.set_score;
      record.mean_inside = candidate.mean_inside;
      record.ndc_z_valid_ratio = candidate.ndc_z_valid_ratio;
      record.instance_first = 0u;
      record.instance_count = candidate.element_count;
      record.element_first = draw.first_instance + candidate.base_offset;
      record.element_count = candidate.element_count;
      if (position < metrics.size()) record.probe = metrics[position];
      const bool projection_ok = candidate.projection_valid
                                 && candidate.instances_ok * 10u >= candidate.element_count * 9u;
      record.pass = projection_ok && record.probe.valid && ProbeCandidatePass(record.probe, threshold);
      capture.hint_diagnostics.push_back(std::move(record));
    }

    if (best >= 0) {
      const TransformCandidate& candidate = probe_candidates[static_cast<size_t>(best)];
      capture.kind = candidate.kind;
      capture.source = candidate.source;
      capture.stage = candidate.stage;
      capture.slot = candidate.slot;
      capture.matrix_offset = candidate.matrix_offset;
      capture.stride = candidate.stride;
      capture.base_offset = candidate.base_offset;
      capture.instances_tested = candidate.element_count;
      capture.instances_ok = candidate.instances_ok;
      capture.projection_score = candidate.set_score;
      capture.ndc_z_valid_ratio = candidate.ndc_z_valid_ratio;
      capture.candidate_valid = true;
      if (best < static_cast<int>(metrics.size())) capture.probe = metrics[static_cast<size_t>(best)];
      const bool projection_ok = candidate.projection_valid
                                 && candidate.instances_ok * 10u >= candidate.element_count * 9u;
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
    g_state.auto_research.last_capture_pass = capture.pass;
    auto* result = FindAutoResult(&g_state.auto_research, vs_hash);
    if (result != nullptr) {
      result->captures.push_back(std::move(capture));
      result->srv_mask = srv_mask;
      result->instance_offset = instance_offset;
      result->instance_offset_found = instance_offset_found;
      result->srv_buffers = srv_summaries;
    }
    g_state.auto_research.last_processed_serial = draw.serial;
    g_state.auto_research.families_done += 1u;
    g_state.auto_research.last_capture_blind = capture_blind;
  }
  return true;
}

// Manual controlled test: probe the exact manual hypothesis for the captured
// family and write a standalone report. Runs at present (needs the queue).
inline void ProbeHintCandidateNow(reshade::api::command_queue* queue) {
  if (queue == nullptr) return;

  uint32_t vs_hash = 0u;
  const ReferenceHint* hint = nullptr;
  CameraSnapshot camera;
  DepthSource depth;
  DrawRecord draw;
  bool instance_offset_found = false;
  int32_t instance_offset_g = 0;
  std::vector<SrvBufferSummary> srv_summaries;
  bool mesh_valid = false;
  std::string mesh_status;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    mesh_valid = g_state.captured.mesh_valid;
    mesh_status = g_state.captured.mesh_status;
    vs_hash = g_state.captured.draw.vs_hash;
    hint = FindReferenceHint(vs_hash);
    camera = g_state.camera;
    depth = g_state.depth_source;
    draw = g_state.captured.draw;
    instance_offset_found = g_state.captured.instance_offset_found;
    instance_offset_g = g_state.captured.instance_offset_g;
    for (const auto& buffer : g_state.captured.srv_buffers) {
      SrvBufferSummary summary;
      summary.slot = buffer.slot;
      summary.stride = buffer.stride;
      summary.size = buffer.size;
      summary.read_offset = buffer.read_offset;
      summary.read_size = buffer.read_size;
      summary.truncated = buffer.truncated;
      srv_summaries.push_back(summary);
    }
  }

  if (!mesh_valid) {
    SetStatus("hint probe: no captured mesh");
    return;
  }
  if (hint == nullptr || !hint->structured_instance) {
    SetStatus("hint probe: no structured instance hint for this family");
    return;
  }

  HintProbePipelineInfo pipeline;
  pipeline.frame_match = depth.valid && depth.frame == draw.frame;

  bool t15_available = false;
  for (const auto& buffer : g_state.captured.srv_buffers) {
    if (buffer.valid && buffer.slot == hint->instance_slot && buffer.stride > 0u) {
      t15_available = true;
      break;
    }
  }

  std::vector<TransformCandidate> diagnostics;
  for (const int32_t base : HintDiagnosticBases(g_state.captured)) {
    TransformCandidate diagnostic;
    if (BuildHintDiagnosticCandidate(g_state.captured, camera, *hint, base, &diagnostic)) {
      diagnostics.push_back(std::move(diagnostic));
    } else {
      pipeline.candidate_skipped += 1u;
      pipeline.skip_reasons.push_back(
          std::string(t15_available ? "extract_failed_base_" : "no_t15_buffer_base_") + std::to_string(base));
    }
  }
  pipeline.candidate_created = !diagnostics.empty();

  std::vector<ProbeMetrics> metrics;
  if (!diagnostics.empty()) {
    std::vector<uint32_t> indices(diagnostics.size());
    std::iota(indices.begin(), indices.end(), 0u);
    ProbeBuild build;
    BuildProbeForCandidates(diagnostics, indices, g_state.captured, camera, &build);
    if (pipeline.frame_match) {
      pipeline.probe_dispatched = true;
      pipeline.probe_readback = RunDepthProbe(
          queue->get_device(), queue->get_immediate_command_list(), queue, depth, &build,
          g_state.setting_probe_tol_rel, g_state.setting_probe_tol_abs, &metrics);
      if (!pipeline.probe_readback) pipeline.probe_skip_reason = "probe_readback_failed";
    } else {
      pipeline.probe_skip_reason = "depth_frame_mismatch";
    }
  } else {
    pipeline.probe_skip_reason = "no_candidate";
  }

  const float threshold = g_state.setting_probe_threshold;
  std::vector<uint8_t> passes(diagnostics.size(), 0u);
  for (size_t i = 0; i < diagnostics.size(); ++i) {
    const bool projection_ok = diagnostics[i].projection_valid
                               && diagnostics[i].instances_ok * 10u >= diagnostics[i].element_count * 9u;
    const bool probe_pass = i < metrics.size() && ProbeCandidatePass(metrics[i], threshold);
    passes[i] = (projection_ok && probe_pass) ? 1u : 0u;
  }

  WriteHintProbeReport(
      vs_hash, *hint, g_state.captured, camera, depth, srv_summaries, instance_offset_found, instance_offset_g,
      diagnostics, metrics, passes, pipeline);
  SetStatus("hint probe written: " + std::to_string(diagnostics.size()) + " diagnostics");
}

inline void RunAutoResearch(reshade::api::command_list* cmd_list, reshade::api::command_queue* queue) {
  if (cmd_list == nullptr || queue == nullptr) return;

  {
    bool probe_requested = false;
    uint32_t probe_vs = 0u;
    uint64_t probe_deadline = 0u;
    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      probe_requested = g_state.probe_hint_requested;
      probe_vs = g_state.probe_hint_vs;
      probe_deadline = g_state.probe_hint_deadline_frame;
    }
    if (probe_requested) {
      bool fresh = false;
      uint32_t serial = 0u;
      {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        fresh = g_state.captured.mesh_valid
                && g_state.captured.draw.vs_hash == probe_vs
                && g_state.depth_source.valid
                && g_state.captured.draw.frame == g_state.depth_source.frame
                && g_state.captured.draw.serial != g_state.probe_hint_last_serial;
        serial = g_state.captured.draw.serial;
      }
      if (fresh) {
        ProbeHintCandidateNow(queue);
        {
          std::lock_guard<std::mutex> lock(g_state.mutex);
          g_state.probe_hint_requested = false;
          g_state.probe_hint_last_serial = serial;
        }
        return;
      }
      const uint64_t frame = g_state.frame.load();
      if (frame > probe_deadline) {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.probe_hint_requested = false;
        if (g_state.arm_vs_hash == probe_vs) {
          g_state.arm_active = false;
          g_state.arm_vs_set.clear();
        }
        g_state.status = "hint probe timeout: no frame-aligned capture within 600 frames";
        return;
      }
      bool need_arm = false;
      {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        need_arm = !g_state.arm_active;
      }
      if (need_arm) ArmCapture(probe_vs, 0u);
      {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.status = "hint probe waiting for frame-aligned capture of 0x" + std::to_string(probe_vs) + "...";
      }
      return;
    }
  }

  bool stop = false;
  bool capture_pending = false;
  uint64_t frame = 0u;
  uint64_t deadline = 0u;
  bool ready = false;
  bool was_window_open = false;
  uint32_t ready_vs = 0u;
  uint32_t ready_serial = 0u;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto& state = g_state.auto_research;
    if (!state.active) return;
    was_window_open = g_state.arm_window_open;
    g_state.arm_window_open = false;
    if (!CensusEnabled()) {
      state.stop_requested = true;
      if (state.finalize_reason.empty()) state.finalize_reason = "census_disabled";
    }
    stop = state.stop_requested;
    capture_pending = g_state.mesh_capture_pending;
    frame = g_state.frame.load();
    deadline = state.deadline_frame;
    // Re-eligible deferred families once their cooldown expires (bounded
    // retries): a later camera position may finally observe their draws.
    if (!state.deferred.empty()) {
      for (auto it = state.deferred.begin(); it != state.deferred.end();) {
        const uint32_t vs_hash = *it;
        const auto retry_it = state.deferred_retries.find(vs_hash);
        const uint32_t retries = retry_it != state.deferred_retries.end() ? retry_it->second : 0u;
        const auto until_it = state.deferred_until.find(vs_hash);
        if (retries < kAutoDeferredMaxRetries
            && until_it != state.deferred_until.end()
            && frame >= until_it->second) {
          state.deferred_retries[vs_hash] = retries + 1u;
          state.no_draw_windows[vs_hash] = 0u;
          it = state.deferred.erase(it);
        } else {
          ++it;
        }
      }
    }
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
    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      auto& state = g_state.auto_research;
      if (state.finalize_reason.empty()) {
        state.finalize_reason = frame > state.deadline_frame ? "deadline" : "user_stop";
      }
    }
    FinalizeAutoResearch();
    return;
  }
  if (capture_pending) return;

  if (ready) {
    const bool accepted = ProcessAutoCapture(cmd_list, queue, ready_vs);
    bool finished = false;
    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      auto& state = g_state.auto_research;
      state.last_processed_serial = ready_serial;
      if (accepted) {
        state.last_vs = ready_vs;
        state.next_capture_frame = frame + kAutoCaptureSpacingFrames;
        const auto* result = FindAutoResult(&state, ready_vs);
        if (result == nullptr || result->captures.size() >= state.rounds) {
          state.pending.erase(std::remove(state.pending.begin(), state.pending.end(), ready_vs), state.pending.end());
        }
        state.probe_retry_counts.erase(ready_vs);
        state.no_draw_windows.erase(ready_vs);
        if (!state.last_capture_pass) {
          // Failing acceptance: retire this exact draw so later windows
          // advance to other draws of the same family. A passing draw stays
          // eligible so it can be captured again for cross-capture
          // verification.
          state.probe_rejected_draws[ready_vs][HintDrawKey(g_state.captured.draw)] = 0xFFFFFFFFu;
        }
        state.window_continuation = false;
        // Disarm after every capture; the spacing gate re-arms on a later present.
        g_state.arm_active = false;
        g_state.arm_vs_set.clear();
        if (state.round_active && state.round_windows_used >= state.round_budget) {
          EndRoundAndDefer(&state);
        }
      } else {
        ++state.probe_retry_counts[ready_vs];
        state.probe_retry_totals[ready_vs] += 1u;
        state.probe_rejected_draws[ready_vs][HintDrawKey(g_state.captured.draw)] = kProbeRetryBudget;
        state.window_continuation = true;
        g_state.status = "probe retry: rejected draw for 0x" + std::to_string(ready_vs);
        // The rejected draw does not consume the arm window; keep it open for the next draw.
        g_state.arm_active = true;
        g_state.arm_window_open = true;
      }
      if (!HasEligiblePending(&state)) {
        finished = true;
        if (state.finalize_reason.empty()) {
          state.finalize_reason = state.pending.empty() ? "pending_empty" : "deferred_remaining";
        }
      }
    }
    if (finished) FinalizeAutoResearch();
    return;
  }

  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto& state = g_state.auto_research;
    if (was_window_open && g_state.arm_active) {
      // A window continuation (probe rejection or skipped retired draw) is
      // part of the same scheduling opportunity: it must not consume a round
      // window or retire an arm window.
      if (!state.window_continuation) {
        // Genuine no-draw deferral: a family armed for several consecutive
        // windows without a single draw is culled, so the cohort moves on
        // instead of waiting for the whole round budget to expire.
        for (const uint32_t vs_hash : g_state.arm_vs_set) {
          const auto draw_it = state.window_armed_draws.find(vs_hash);
          const uint32_t draws = draw_it != state.window_armed_draws.end() ? draw_it->second : 0u;
          if (draws != 0u) {
            state.no_draw_windows[vs_hash] = 0u;
            continue;
          }
          const uint32_t misses = ++state.no_draw_windows[vs_hash];
          if (misses < kNoDrawDeferralWindows) continue;
          DeferFamily(&state, vs_hash);
        }
        state.windows_retired += 1u;
        if (state.round_active && state.round_windows_used >= state.round_budget) {
          EndRoundAndDefer(&state);
        }
      }
      g_state.arm_active = false;
      g_state.arm_vs_set.clear();
      state.next_capture_frame = frame;
    }
  }

  {
    bool nothing_eligible = false;
    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      auto& state = g_state.auto_research;
      nothing_eligible = !HasEligiblePending(&state);
      if (nothing_eligible && state.finalize_reason.empty()) {
        state.finalize_reason = state.pending.empty() ? "pending_empty" : "deferred_remaining";
      }
    }
    if (nothing_eligible) {
      FinalizeAutoResearch();
      return;
    }
  }

  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto& state = g_state.auto_research;
    if (!g_state.arm_active && HasEligiblePending(&state) && frame >= state.next_capture_frame) {
      const uint32_t min_captures = ComputeMinEligible(&state);
      if (!state.round_active || min_captures != state.round_target_count) {
        StartRound(&state, min_captures);
      }
      ArmMinimumCaptureCohort(&state);
      g_state.arm_active = !g_state.arm_vs_set.empty();
      g_state.arm_vs_hash = 0u;
      g_state.arm_serial = 0u;
      if (g_state.arm_active) {
        g_state.arm_window_open = true;
        if (!state.window_continuation) {
          state.round_windows_used += 1u;
          state.arm_windows += 1u;
          state.window_armed_draws.clear();
        }
        state.arm_set_size = static_cast<uint32_t>(g_state.arm_vs_set.size());
        state.window_continuation = false;
      }
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
