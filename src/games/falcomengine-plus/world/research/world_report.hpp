#pragma once

// Phase 0 JSON report writer (manual emitter; no external JSON dependency).
//
// Writes world_auto_report_<YYYYMMDD_HHMMSS>.json and a full copy at
// world_auto_report_latest.json under renodx-dev/falcomengine-plus/world/.

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

#include "../world_state.hpp"
#include "../capture/draw_census.hpp"
#include "coverage.hpp"
#include "reference_hints.hpp"
#include "transform_candidates.hpp"

namespace falcom_world {

inline std::string JsonEscape(const std::string& value) {
  std::string out;
  out.reserve(value.size() + 8u);
  for (const char c : value) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

inline std::string JsonFloat(float value) {
  std::ostringstream out;
  out << std::setprecision(6) << value;
  return out.str();
}

inline std::string JsonFloatArray(const std::vector<float>& values) {
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < values.size(); ++i) {
    if (i != 0u) out << ",";
    out << JsonFloat(values[i]);
  }
  out << "]";
  return out.str();
}

inline std::string JsonFloatArrayRange(const std::vector<float>& values, size_t offset, size_t count) {
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < count && offset + i < values.size(); ++i) {
    if (i != 0u) out << ",";
    out << JsonFloat(values[offset + i]);
  }
  out << "]";
  return out.str();
}

inline std::filesystem::path LatestReportPath() {
  return WorldOutputDir() / "world_auto_report_latest.json";
}

inline std::filesystem::path LatestSummaryPath() {
  return WorldOutputDir() / "world_auto_summary_latest.txt";
}

inline std::string EmitSrvBuffersJson(const std::vector<SrvBufferSummary>& buffers) {
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < buffers.size(); ++i) {
    const SrvBufferSummary& buffer = buffers[i];
    if (i != 0u) out << ",";
    out << "{\"slot\":" << static_cast<uint32_t>(buffer.slot)
        << ",\"stride\":" << buffer.stride
        << ",\"size\":" << buffer.size
        << ",\"read_offset\":" << buffer.read_offset
        << ",\"read_size\":" << buffer.read_size
        << ",\"truncated\":" << (buffer.truncated ? "true" : "false") << "}";
  }
  out << "]";
  return out.str();
}

inline std::string EmitHintDiagnosticsJson(const std::vector<HintDiagnosticRecord>& diagnostics) {
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < diagnostics.size(); ++i) {
    const HintDiagnosticRecord& record = diagnostics[i];
    if (i != 0u) out << ",";
    out << "{";
    out << "\"kind\":\"" << CandidateKindName(record.kind) << "\",";
    out << "\"slot\":" << static_cast<uint32_t>(record.slot) << ",";
    out << "\"matrix_offset\":" << record.matrix_offset << ",";
    out << "\"stride\":" << record.stride << ",";
    out << "\"base\":" << record.base << ",";
    out << "\"projection_valid\":" << (record.projection_valid ? "true" : "false") << ",";
    out << "\"projection_score\":" << JsonFloat(record.projection_score) << ",";
    out << "\"mean_inside\":" << JsonFloat(record.mean_inside) << ",";
    out << "\"ndc_z_valid_ratio\":" << JsonFloat(record.ndc_z_valid_ratio) << ",";
    out << "\"instance_range\":[" << record.instance_first << ","
        << (record.instance_first + record.instance_count) << "],";
    out << "\"element_range\":[" << record.element_first << ","
        << (record.element_first + record.element_count) << "],";
    out << "\"depth\":{";
    out << "\"samples\":" << record.probe.samples << ",";
    out << "\"out_of_screen\":" << record.probe.out_of_screen << ",";
    out << "\"match\":" << record.probe.match << ",";
    out << "\"occluded\":" << record.probe.occluded << ",";
    out << "\"mismatch\":" << record.probe.mismatch << ",";
    out << "\"match_ratio\":" << JsonFloat(record.probe.match_ratio) << ",";
    out << "\"mean_abs_error\":" << JsonFloat(record.probe.mean_abs_error) << ",";
    out << "\"mean_expected\":" << JsonFloat(record.probe.mean_expected);
    out << "},";
    out << "\"pass\":" << (record.pass ? "true" : "false");
    out << "}";
  }
  out << "]";
  return out.str();
}

inline std::string EmitCandidateDiagnosticsJson(const std::vector<CandidateDiagnosticRecord>& records) {
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < records.size(); ++i) {
    const CandidateDiagnosticRecord& record = records[i];
    if (i != 0u) out << ",";
    out << "{\"source\":\"" << CandidateSourceName(static_cast<uint8_t>(record.source)) << "\",";
    out << "\"kind\":\"" << CandidateKindName(record.kind) << "\",";
    out << "\"slot\":" << record.slot << ",";
    out << "\"matrix_offset\":" << record.matrix_offset << ",";
    out << "\"stride\":" << record.stride << ",";
    out << "\"base\":" << record.base_offset << ",";
    out << "\"projection_valid\":" << (record.projection_valid ? "true" : "false") << ",";
    out << "\"projection_score\":" << JsonFloat(record.projection_score) << ",";
    out << "\"ndc_z_valid_ratio\":" << JsonFloat(record.ndc_z_valid_ratio) << ",";
    out << "\"match\":" << record.match << ",";
    out << "\"occluded\":" << record.occluded << ",";
    out << "\"mismatch\":" << record.mismatch << ",";
    out << "\"out_of_screen\":" << record.out_of_screen << ",";
    out << "\"match_ratio\":" << JsonFloat(record.match_ratio) << ",";
    out << "\"pass\":" << (record.pass ? "true" : "false") << ",";
    out << "\"selected\":" << (record.selected ? "true" : "false") << "}";
  }
  out << "]";
  return out.str();
}

inline std::string EmitPassingLayoutsJson(const std::vector<PassingLayout>& layouts) {
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < layouts.size(); ++i) {
    const PassingLayout& layout = layouts[i];
    if (i != 0u) out << ",";
    out << "{\"kind\":\"" << CandidateKindName(layout.kind) << "\",";
    out << "\"stage\":" << static_cast<uint32_t>(layout.stage) << ",";
    out << "\"slot\":" << layout.slot << ",";
    out << "\"matrix_offset\":" << layout.matrix_offset << ",";
    out << "\"stride\":" << layout.stride << ",";
    out << "\"base_offset\":" << layout.base_offset << ",";
    out << "\"count\":" << layout.count << "}";
  }
  out << "]";
  return out.str();
}

inline std::string EmitCaptureJson(const AutoCapture& capture) {
  std::ostringstream out;
  out << "{";
  out << "\"frame\":" << capture.frame << ",";
  out << "\"draw_serial\":" << capture.draw_serial << ",";
  out << "\"camera_frame\":" << capture.camera_frame << ",";
  out << "\"kind\":\"" << CandidateKindName(capture.kind) << "\",";
  out << "\"candidate_source\":\"" << CandidateSourceName(static_cast<uint8_t>(capture.source)) << "\",";
  out << "\"stage\":" << static_cast<uint32_t>(capture.stage) << ",";
  out << "\"slot\":" << capture.slot << ",";
  out << "\"matrix_offset\":" << capture.matrix_offset << ",";
  out << "\"stride\":" << capture.stride << ",";
  out << "\"base_offset\":" << capture.base_offset << ",";
  out << "\"instance_offset_g\":" << capture.instance_offset_g << ",";
  out << "\"instance_offset_found\":" << (capture.instance_offset_found ? "true" : "false") << ",";
  out << "\"dsv_matches_depth_source\":" << (capture.dsv_matches_depth_source ? "true" : "false") << ",";
  out << "\"draw_instance_count\":" << capture.draw_instance_count << ",";
  out << "\"instances_tested\":" << capture.instances_tested << ",";
  out << "\"instances_ok\":" << capture.instances_ok << ",";
  out << "\"projection_score\":" << JsonFloat(capture.projection_score) << ",";
  out << "\"ndc_z_valid_ratio\":" << JsonFloat(capture.ndc_z_valid_ratio) << ",";
  out << "\"depth\":{";
  out << "\"samples\":" << capture.probe.samples << ",";
  out << "\"out_of_screen\":" << capture.probe.out_of_screen << ",";
  out << "\"match\":" << capture.probe.match << ",";
  out << "\"occluded\":" << capture.probe.occluded << ",";
  out << "\"mismatch\":" << capture.probe.mismatch << ",";
  out << "\"match_ratio\":" << JsonFloat(capture.probe.match_ratio) << ",";
  out << "\"mean_abs_error\":" << JsonFloat(capture.probe.mean_abs_error) << ",";
  out << "\"mean_expected\":" << JsonFloat(capture.probe.mean_expected);
  out << "},";
  out << "\"pass\":" << (capture.pass ? "true" : "false") << ",";
  out << "\"failure_reason\":\"" << JsonEscape(capture.failure_reason) << "\"";
  if (!capture.matrices.empty()) {
    out << ",\"matrices\":" << JsonFloatArray(capture.matrices);
  }
  if (!capture.prev_matrices.empty()) {
    out << ",\"prev_matrices\":" << JsonFloatArray(capture.prev_matrices);
  }
  if (!capture.probe.diag.empty()) {
    out << ",\"samples\":[";
    for (size_t i = 0; i < capture.probe.diag.size(); ++i) {
      const ProbeSampleDiag& diag = capture.probe.diag[i];
      if (i != 0u) out << ",";
      out << "{\"instance\":" << diag.instance
          << ",\"uv\":[" << JsonFloat(diag.uv[0]) << "," << JsonFloat(diag.uv[1]) << "]"
          << ",\"expected\":" << JsonFloat(diag.expected_linear)
          << ",\"sampled\":" << JsonFloat(diag.sampled_linear)
          << ",\"error\":" << JsonFloat(diag.error)
          << ",\"class\":" << static_cast<uint32_t>(diag.classification) << "}";
    }
    out << "]";
  }
  if (!capture.hint_diagnostics.empty()) {
    out << ",\"hint_diagnostics\":" << EmitHintDiagnosticsJson(capture.hint_diagnostics);
  }
  if (!capture.candidate_diagnostics.empty()) {
    out << ",\"candidate_diagnostics\":" << EmitCandidateDiagnosticsJson(capture.candidate_diagnostics);
  }
  out << "}";
  return out.str();
}

inline const char* AutoFailureClassName(const AutoFamilyResult& result) {
  if (result.runtime_verified) return "pass";

  bool has_diagnostics = false;
  uint64_t preferred_signature = 0u;
  bool has_preferred = false;
  {
    std::unordered_map<uint64_t, uint32_t> pass_counts;
    for (const auto& capture : result.captures) {
      if (!capture.pass) continue;
      pass_counts[CandidateSignature(capture.kind, capture.slot, capture.matrix_offset, capture.stride)] += 1u;
    }
    uint32_t best_count = 0u;
    for (const auto& [signature, count] : pass_counts) {
      if (count > best_count) {
        best_count = count;
        preferred_signature = signature;
        has_preferred = true;
      }
    }
  }

  bool preferred_missing = false;
  const CandidateDiagnosticRecord* best_failing = nullptr;
  uint32_t best_decisive = 0u;
  uint32_t best_match = 0u;
  for (const auto& capture : result.captures) {
    if (!capture.candidate_diagnostics.empty()) has_diagnostics = true;
    if (!capture.pass && has_preferred) {
      bool present = false;
      for (const auto& record : capture.candidate_diagnostics) {
        if (CandidateSignature(record.kind, record.slot, record.matrix_offset, record.stride) == preferred_signature) {
          present = true;
          break;
        }
      }
      if (!present) preferred_missing = true;
    }
    for (const auto& record : capture.candidate_diagnostics) {
      if (capture.pass || !record.selected) continue;
      const uint32_t decisive = record.match + record.mismatch;
      if (best_failing == nullptr || decisive > best_decisive
          || (decisive == best_decisive && record.match > best_match)) {
        best_failing = &record;
        best_decisive = decisive;
        best_match = record.match;
      }
    }
  }

  if (preferred_missing) return "preferred_signature_missing";
  if (result.depth_pass) return "single_capture";
  if (result.deferred_no_draw) return "deferred_no_draw";
  if (!has_diagnostics) return "no_candidates";
  if (best_failing != nullptr) {
    if (best_failing->match == 0u && best_failing->mismatch == 0u && best_failing->occluded > 0u) {
      return "all_occluded";
    }
    if (best_failing->match + best_failing->mismatch > 0u) return "near_miss";
  }
  return "no_candidates";
}

inline void WriteAutoReport() {
  std::vector<FamilyStats> families;
  std::vector<AutoFamilyResult> results;
  AutoState auto_state;
  float threshold = 0.35f;
  uint64_t start_frame = 0u;
  uint64_t end_frame = 0u;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    families.reserve(g_state.families.size());
    for (const auto& [hash, family] : g_state.families) {
      (void)hash;
      families.push_back(family);
    }
    results = g_state.auto_research.results;
    auto_state = g_state.auto_research;
    threshold = g_state.setting_probe_threshold;
    start_frame = g_state.auto_research.start_frame;
    end_frame = g_state.frame.load();
  }

  const CoverageMetrics metrics = ComputeCoverage();

  uint32_t captures_done = 0u;
  for (const auto& result : results) {
    captures_done += static_cast<uint32_t>(result.captures.size());
  }
  const uint32_t captures_target = static_cast<uint32_t>(results.size()) * auto_state.rounds;

  const std::time_t now = std::time(nullptr);
  std::tm local = {};
  localtime_s(&local, &now);
  char timestamp[32] = {};
  char stamp[32] = {};
  std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local);
  std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &local);
  const std::string report_name = std::string("world_auto_report_") + stamp + ".json";

  std::ostringstream out;
  out << "{\n";
  out << "  \"report\": \"" << report_name << "\",\n";
  out << "  \"timestamp\": \"" << timestamp << "\",\n";
  out << "  \"game\": \"sora_2nd\",\n";
  out << "  \"frames\": {\"start\": " << start_frame << ", \"end\": " << end_frame << "},\n";
  out << "  \"thresholds\": {\"match_ratio\": " << JsonFloat(threshold)
      << ", \"min_matches\": " << kMinDepthMatches
      << ", \"rounds\": " << auto_state.rounds
      << ", \"depth_space\": \"device_ndc\"},\n";
  out << "  \"gate\": {\"primary\": " << JsonFloat(metrics.primary)
      << ", \"secondary\": " << JsonFloat(metrics.secondary)
      << ", \"primary_pass\": " << (metrics.primary_pass ? "true" : "false")
      << ", \"secondary_pass\": " << (metrics.secondary_pass ? "true" : "false")
      << ", \"major_family_missing\": " << (metrics.major_family_missing ? "true" : "false")
      << ", \"pass\": " << (metrics.pass ? "true" : "false") << "},\n";
  out << "  \"scheduler\": {"
      << "\"finalize_reason\": \"" << JsonEscape(auto_state.finalize_reason) << "\","
      << "\"queue\": " << auto_state.queue.size() << ","
      << "\"rounds\": " << auto_state.rounds << ","
      << "\"captures_done\": " << captures_done << ","
      << "\"captures_target\": " << captures_target << ","
      << "\"arm_windows\": " << auto_state.arm_windows << ","
      << "\"arm_set_size\": " << auto_state.arm_set_size << ","
      << "\"windows_retired\": " << auto_state.windows_retired << ","
      << "\"rounds_started\": " << auto_state.rounds_started << ","
      << "\"deferred\": " << auto_state.deferred.size() << ","
      << "\"pending_at_end\": " << auto_state.pending_at_end << ","
      << "\"start_frame\": " << auto_state.start_frame << ","
      << "\"end_frame\": " << end_frame << ","
      << "\"deadline_frame\": " << auto_state.deadline_frame << ","
      << "\"next_capture_frame\": " << auto_state.next_capture_frame << ","
      << "\"last_vs\": \"0x" << std::hex << auto_state.last_vs << std::dec << "\"},\n";
  out << "  \"families\": [\n";

  bool first_family = true;
  for (const auto& result : results) {
    const auto family_it = std::find_if(families.begin(), families.end(), [&](const FamilyStats& family) {
      return family.vs_hash == result.vs_hash;
    });
    const FamilyStats* family = family_it != families.end() ? &*family_it : nullptr;
    const ReferenceHint* hint = FindReferenceHint(result.vs_hash);

    if (!first_family) out << ",\n";
    first_family = false;
    out << "    {";
    out << "\"vs_hash\": \"0x" << std::hex << result.vs_hash << std::dec << "\",";
    out << "\"hint_tag\": \"" << (hint != nullptr ? hint->tag : "") << "\",";
    out << "\"hint_category\": \"" << (hint != nullptr ? hint->category : "") << "\",";
    out << "\"label\": \"" << (family != nullptr ? FamilyLabelName(family->label) : "Unknown") << "\",";
    out << "\"label_source\": \"" << (family != nullptr ? LabelSourceName(family->label_source) : "unknown") << "\",";
    if (family != nullptr) {
      out << "\"draws\":" << family->draws << ",";
      out << "\"unique_meshes\":" << family->meshes.size() << ",";
      out << "\"instances\":" << family->instances << ",";
      out << "\"weighted_triangles\":" << family->triangles << ",";
      out << "\"candidate_draws\":" << family->candidate_draws << ",";
    } else {
      out << "\"draws\":0,\"unique_meshes\":0,\"instances\":0,\"weighted_triangles\":0,\"candidate_draws\":0,";
    }
    out << "\"srv_mask\":" << result.srv_mask << ",";
    out << "\"instance_offset\":" << result.instance_offset << ",";
    out << "\"instance_offset_found\":" << (result.instance_offset_found ? "true" : "false") << ",";
    out << "\"selected_instance_stride\":" << result.read_stride << ",";
    out << "\"read_window\":{\"offset\":" << result.read_offset << ",\"size\":" << result.read_size << "},";
    out << "\"srv_buffers\":" << EmitSrvBuffersJson(result.srv_buffers) << ",";
    out << "\"hint_diagnostics\":" << EmitHintDiagnosticsJson(result.hint_diagnostics) << ",";
    uint32_t captures_ok = 0u;
    uint32_t hint_injected = 0u;
    uint32_t probes_run = 0u;
    for (const auto& capture : result.captures) {
      if (capture.pass) captures_ok += 1u;
      hint_injected += static_cast<uint32_t>(capture.hint_diagnostics.size());
      if (capture.probe.valid) probes_run += 1u;
    }
    out << "\"pipeline\":{"
        << "\"eligible\":true,"
        << "\"queued\":true,"
        << "\"pending_at_end\":" << (result.pending_at_end ? "true" : "false") << ","
        << "\"deferred_no_draw\":" << (result.deferred_no_draw ? "true" : "false") << ","
        << "\"armed_draws\":" << result.armed_draws << ","
        << "\"arm_wins\":" << result.captures.size() << ","
        << "\"capture_attempts\":" << result.captures.size() << ","
        << "\"captures_ok\":" << captures_ok << ","
        << "\"hint_injected\":" << hint_injected << ","
        << "\"probes_run\":" << probes_run << ","
        << "\"probe_retries\":" << result.probe_retries << ","
        << "\"skip_reason\":\"" << JsonEscape(result.skip_reason) << "\"},";
    out << "\"evidence\":{";
    out << "\"manual_reference\":" << (hint != nullptr ? "true" : "false") << ",";
    out << "\"runtime_census\":true,";
    out << "\"projection\":" << (result.cross_capture_pass ? "\"pass\"" : "\"inconclusive\"") << ",";
    out << "\"depth_validation\":" << (result.depth_pass ? "\"pass\"" : "\"fail\"") << ",";
    out << "\"decompilation\":\"pending\",\"final\":\"pending\"},";
    out << "\"passing_layouts\":" << EmitPassingLayoutsJson(result.passing_layouts) << ",";
    out << "\"captures\": [";
    for (size_t i = 0; i < result.captures.size(); ++i) {
      if (i != 0u) out << ",";
      out << EmitCaptureJson(result.captures[i]);
    }
    out << "],";
    out << "\"runtime_verified\":" << (result.runtime_verified ? "true" : "false") << ",";
    out << "\"depth_pass\":" << (result.depth_pass ? "true" : "false") << ",";
    out << "\"cross_capture_pass\":" << (result.cross_capture_pass ? "true" : "false") << ",";
    out << "\"verdict\":\"" << AutoVerdictName(result.verdict) << "\",";
    out << "\"failure_reason\":\"" << JsonEscape(result.failure_reason) << "\"";
    out << "}";
  }
  out << "\n  ]\n";
  out << "}\n";

  std::string text = out.str();
  const auto dir = WorldOutputDir();
  renodx::utils::path::WriteTextFile(dir / report_name, text);
  renodx::utils::path::WriteTextFile(LatestReportPath(), text);

  std::ostringstream summary;
  summary << "report: " << report_name << "\n";
  summary << "timestamp: " << timestamp << "\n";
  summary << "frames: " << start_frame << ".." << end_frame << "\n";
  summary << "gate: primary=" << JsonFloat(metrics.primary)
          << " secondary=" << JsonFloat(metrics.secondary)
          << " primary_pass=" << (metrics.primary_pass ? "true" : "false")
          << " secondary_pass=" << (metrics.secondary_pass ? "true" : "false")
          << " major_family_missing=" << (metrics.major_family_missing ? "true" : "false")
          << " pass=" << (metrics.pass ? "true" : "false") << "\n";
  summary << "scheduler: finalize_reason=" << auto_state.finalize_reason
          << " queue=" << auto_state.queue.size()
          << " rounds=" << auto_state.rounds
          << " captures=" << captures_done << "/" << captures_target
          << " arm_windows=" << auto_state.arm_windows
          << " windows_retired=" << auto_state.windows_retired
          << " rounds_started=" << auto_state.rounds_started
          << " deferred=" << auto_state.deferred.size()
          << " pending_at_end=" << auto_state.pending_at_end << "\n";
  uint32_t verified_count = 0u;
  uint32_t single_count = 0u;
  uint32_t failed_count = 0u;
  uint32_t not_observed_count = 0u;
  for (const auto& result : results) {
    switch (static_cast<AutoVerdict>(result.verdict)) {
      case AutoVerdict::Verified: verified_count += 1u; break;
      case AutoVerdict::VerifiedSingleCapture: single_count += 1u; break;
      case AutoVerdict::Failed: failed_count += 1u; break;
      default: not_observed_count += 1u; break;
    }
  }
  summary << "verdicts: Verified=" << verified_count
          << " single=" << single_count
          << " Failed=" << failed_count
          << " NotObserved=" << not_observed_count << "\n";
  summary << "families:\n";
  summary << std::left
          << std::setw(12) << "hash"
          << std::setw(15) << "label"
          << std::setw(12) << "tris"
          << std::setw(24) << "verdict"
          << std::setw(3) << "ok"
          << std::setw(6) << "rv"
          << std::setw(5) << "retry"
          << std::setw(38) << "selected"
          << "class" << "\n";
  std::vector<const AutoFamilyResult*> sorted_results;
  sorted_results.reserve(results.size());
  for (const auto& result : results) sorted_results.push_back(&result);
  std::sort(sorted_results.begin(), sorted_results.end(), [&](const AutoFamilyResult* a, const AutoFamilyResult* b) {
    const auto ta = std::find_if(families.begin(), families.end(), [&](const FamilyStats& family) {
      return family.vs_hash == a->vs_hash;
    });
    const auto tb = std::find_if(families.begin(), families.end(), [&](const FamilyStats& family) {
      return family.vs_hash == b->vs_hash;
    });
    const uint64_t va = ta != families.end() ? ta->triangles : 0u;
    const uint64_t vb = tb != families.end() ? tb->triangles : 0u;
    return va > vb;
  });
  for (const auto* result : sorted_results) {
    const auto family_it = std::find_if(families.begin(), families.end(), [&](const FamilyStats& family) {
      return family.vs_hash == result->vs_hash;
    });
    const FamilyStats* family = family_it != families.end() ? &*family_it : nullptr;
    uint32_t captures_ok = 0u;
    for (const auto& capture : result->captures) {
      if (capture.pass) captures_ok += 1u;
    }
    std::string selected = "-";
    if (!result->captures.empty()) {
      const AutoCapture& capture = result->captures.back();
      selected = std::string(CandidateKindName(capture.kind))
                 + " s" + std::to_string(capture.slot)
                 + " o" + std::to_string(capture.matrix_offset)
                 + " st" + std::to_string(capture.stride)
                 + " b" + std::to_string(capture.base_offset);
    }
    std::ostringstream hash_text;
    hash_text << "0x" << std::hex << result->vs_hash;
    summary << std::left
            << std::setw(12) << hash_text.str()
            << std::setw(15) << (family != nullptr ? FamilyLabelName(family->label) : "Unknown")
            << std::setw(12) << (family != nullptr ? family->triangles : 0u)
            << std::setw(24) << AutoVerdictName(result->verdict)
            << std::setw(3) << captures_ok
            << std::setw(6) << (result->runtime_verified ? "true" : "false")
            << std::setw(5) << result->probe_retries
            << std::setw(38) << selected
            << AutoFailureClassName(*result) << "\n";
  }
  std::string summary_text = summary.str();
  const std::string summary_name = std::string("world_auto_summary_") + stamp + ".txt";
  renodx::utils::path::WriteTextFile(dir / summary_name, summary_text);
  renodx::utils::path::WriteTextFile(LatestSummaryPath(), summary_text);
  SetStatus("report written: " + report_name);
}

inline void WriteHintProbeReport(
    uint32_t vs_hash,
    const ReferenceHint& hint,
    const CapturedDraw& captured,
    const CameraSnapshot& camera,
    const DepthSource& depth,
    const std::vector<SrvBufferSummary>& srv_buffers,
    bool instance_offset_found,
    int32_t instance_offset_g,
    const std::vector<TransformCandidate>& diagnostics,
    const std::vector<ProbeMetrics>& metrics,
    const std::vector<uint8_t>& passes,
    const HintProbePipelineInfo& pipeline) {
  const DrawRecord& draw = captured.draw;
  const std::time_t now = std::time(nullptr);
  std::tm local = {};
  localtime_s(&local, &now);
  char timestamp[32] = {};
  char stamp[32] = {};
  std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local);
  std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &local);
  const std::string report_name = std::string("world_hint_probe_") + stamp + ".json";

  std::ostringstream out;
  out << "{\n";
  out << "  \"report\": \"" << report_name << "\",\n";
  out << "  \"timestamp\": \"" << timestamp << "\",\n";
  out << "  \"game\": \"sora_2nd\",\n";
  out << "  \"vs_hash\": \"0x" << std::hex << vs_hash << std::dec << "\",\n";
  out << "  \"hint_tag\": \"" << hint.tag << "\",\n";
  out << "  \"pipeline\": {";
  out << "\"frame_match\": " << (pipeline.frame_match ? "true" : "false") << ",";
  out << "\"candidate_created\": " << (pipeline.candidate_created ? "true" : "false") << ",";
  out << "\"candidate_skipped\": " << pipeline.candidate_skipped << ",";
  out << "\"skip_reasons\": [";
  for (size_t i = 0; i < pipeline.skip_reasons.size(); ++i) {
    if (i != 0u) out << ",";
    out << "\"" << JsonEscape(pipeline.skip_reasons[i]) << "\"";
  }
  out << "],";
  out << "\"probe_dispatched\": " << (pipeline.probe_dispatched ? "true" : "false") << ",";
  out << "\"probe_readback\": " << (pipeline.probe_readback ? "true" : "false") << ",";
  out << "\"probe_skip_reason\": \"" << JsonEscape(pipeline.probe_skip_reason) << "\"},\n";
  out << "  \"manual_hypothesis\": {";
  out << "\"instance_slot\": " << static_cast<uint32_t>(hint.instance_slot) << ",";
  out << "\"matrix_offset\": " << hint.world_offset << ",";
  out << "\"prev_offset\": " << hint.prev_offset << ",";
  out << "\"layout\": \"Instance 4x3 Row\",";
  out << "\"expected_math\": \"world = row-dot float4x3; clip = row-dot float4x4\",";
  out << "\"pattern\": \"" << JsonEscape(hint.pattern) << "\",";
  out << "\"file\": \"" << JsonEscape(hint.file) << "\"},\n";
  bool b1_found = false;
  int32_t b1_value = 0;
  for (const auto& cb : captured.cbs) {
    if (cb.stage == 1u && cb.slot == 1u && cb.bytes.size() >= 4u) {
      std::memcpy(&b1_value, cb.bytes.data(), sizeof(int32_t));
      b1_found = true;
      break;
    }
  }
  out << "  \"runtime_facts\": {";
  out << "\"srv_buffers\": " << EmitSrvBuffersJson(srv_buffers) << ",";
  out << "\"instance_offset_found\": " << (instance_offset_found ? "true" : "false") << ",";
  out << "\"instance_offset_g\": " << instance_offset_g << ",";
  out << "\"b1_snapshot_found\": " << (b1_found ? "true" : "false") << ",";
  out << "\"b1_value\": " << b1_value << ",";
  out << "\"draw_frame\": " << draw.frame << ",";
  out << "\"draw_serial\": " << draw.serial << ",";
  out << "\"first_instance\": " << draw.first_instance << ",";
  out << "\"draw_instance_count\": " << draw.instance_count << ",";
  out << "\"vs_cb_mask\": " << draw.vs_cb_mask << ",";
  out << "\"ps_cb_mask\": " << draw.ps_cb_mask << ",";
  out << "\"vs_srv_mask\": " << draw.vs_srv_mask << ",";
  out << "\"cb_snapshots\": [";
  for (size_t i = 0; i < captured.cbs.size(); ++i) {
    const CbSnapshot& cb = captured.cbs[i];
    if (i != 0u) out << ",";
    out << "{\"stage\":" << static_cast<uint32_t>(cb.stage)
        << ",\"slot\":" << cb.slot
        << ",\"size\":" << cb.resource_size
        << ",\"snapshot_size\":" << cb.snapshot_size
        << ",\"gpu_snapshot\":" << (cb.gpu_snapshot ? "true" : "false") << "}";
  }
  out << "],";
  out << "\"camera_source\": \""
      << (camera.source == 2u ? "gpu_snapshot" : (camera.source == 1u ? "cache" : "unknown")) << "\",";
  out << "\"camera_view\": " << JsonFloatArray(std::vector<float>(camera.view, camera.view + 16u)) << ",";
  out << "\"camera_view_proj\": " << JsonFloatArray(std::vector<float>(camera.view_proj, camera.view_proj + 16u)) << ",";
  out << "\"mesh_vertices\": " << captured.mesh.positions.size() << ",";
  out << "\"mesh_triangles\": " << captured.mesh.triangles.size() << ",";
  out << "\"mesh_status\": \"" << JsonEscape(captured.mesh_status) << "\",";
  out << "\"position_format\": \"" << renodx::utils::scene::FormatToString(captured.mesh.layout.pos_format) << "\",";
  out << "\"camera_valid\": " << (camera.valid ? "true" : "false") << ",";
  out << "\"camera_has_prev\": " << (camera.has_prev ? "true" : "false") << ",";
  out << "\"depth_valid\": " << (depth.valid ? "true" : "false") << ",";
  out << "\"depth_frame\": " << depth.frame << ",";
  out << "\"depth_frame_match\": " << ((depth.valid && depth.frame == draw.frame) ? "true" : "false") << "},\n";
  const auto emit_projection = [&](const TransformCandidate& candidate, uint32_t instance,
                                   const std::array<float, 3>& point) {
    float uv[2] = {};
    float world[3] = {};
    float ndc_z = 0.f;
    float clip[4] = {};
    const bool ok = ProjectCandidateInstance(candidate, camera, instance, point, uv, world, &ndc_z, clip);
    out << "{";
    out << "\"ok\":" << (ok ? "true" : "false");
    out << ",\"clip\":[" << JsonFloat(clip[0]) << "," << JsonFloat(clip[1]) << ","
        << JsonFloat(clip[2]) << "," << JsonFloat(clip[3]) << "]";
    if (ok) {
      out << ",\"world\":[" << JsonFloat(world[0]) << "," << JsonFloat(world[1]) << ","
          << JsonFloat(world[2]) << "]";
      out << ",\"ndc\":[" << JsonFloat(clip[0] / clip[3]) << "," << JsonFloat(clip[1] / clip[3])
          << "," << JsonFloat(clip[2] / clip[3]) << "]";
      out << ",\"uv\":[" << JsonFloat(uv[0]) << "," << JsonFloat(uv[1]) << "]";
    }
    out << "}";
  };

  out << "  \"diagnostics\": [\n";
  for (size_t i = 0; i < diagnostics.size(); ++i) {
    const TransformCandidate& candidate = diagnostics[i];
    const ProbeMetrics* probe = i < metrics.size() ? &metrics[i] : nullptr;
    const bool pass = i < passes.size() && passes[i] != 0u;
    if (i != 0u) out << ",\n";
    out << "    {";
    out << "\"base\": " << candidate.base_offset << ",";
    out << "\"kind\": \"" << CandidateKindName(candidate.kind) << "\",";
    out << "\"convention\": \"" << (candidate.row_dot ? "row_dot" : "column_dot") << "\",";
    out << "\"slot\": " << static_cast<uint32_t>(candidate.slot) << ",";
    out << "\"matrix_offset\": " << candidate.matrix_offset << ",";
    out << "\"stride\": " << candidate.stride << ",";
    out << "\"projection_valid\": " << (candidate.projection_valid ? "true" : "false") << ",";
    out << "\"projection_score\": " << JsonFloat(candidate.set_score) << ",";
    out << "\"mean_inside\": " << JsonFloat(candidate.mean_inside) << ",";
    out << "\"ndc_z_valid_ratio\": " << JsonFloat(candidate.ndc_z_valid_ratio) << ",";
    const uint32_t real_samples = probe != nullptr ? probe->samples : 0u;
    const uint32_t total_slots = kProbeInstances * kProbeVertices;
    out << "\"real_samples\": " << real_samples << ",";
    out << "\"sentinel_samples_skipped\": "
        << (total_slots >= real_samples ? total_slots - real_samples : 0u) << ",";
    out << "\"instance_range\": [0," << candidate.element_count << "],";
    const uint32_t element_first = draw.first_instance + candidate.base_offset;
    out << "\"element_range\": [" << element_first << "," << (element_first + candidate.element_count) << "],";
    if (probe != nullptr) {
      out << "\"depth\": {";
      out << "\"samples\": " << probe->samples << ",";
      out << "\"out_of_screen\": " << probe->out_of_screen << ",";
      out << "\"match\": " << probe->match << ",";
      out << "\"occluded\": " << probe->occluded << ",";
      out << "\"mismatch\": " << probe->mismatch << ",";
      out << "\"match_ratio\": " << JsonFloat(probe->match_ratio) << ",";
      out << "\"mean_abs_error\": " << JsonFloat(probe->mean_abs_error) << ",";
      out << "\"mean_expected\": " << JsonFloat(probe->mean_expected) << "},";
      out << "\"samples\": [";
      for (size_t s = 0; s < probe->diag.size(); ++s) {
        const ProbeSampleDiag& diag = probe->diag[s];
        if (s != 0u) out << ",";
        out << "{\"instance\":" << diag.instance
            << ",\"uv\":[" << JsonFloat(diag.uv[0]) << "," << JsonFloat(diag.uv[1]) << "]"
            << ",\"expected\":" << JsonFloat(diag.expected_linear)
            << ",\"sampled\":" << JsonFloat(diag.sampled_linear)
            << ",\"error\":" << JsonFloat(diag.error)
            << ",\"class\":" << static_cast<uint32_t>(diag.classification) << "}";
      }
      out << "],";
    }
    if (candidate.stride > 0u && !candidate.matrices.empty()) {
      const uint32_t matrix_floats = KindMatrixFloats(candidate.kind);
      const uint32_t instance_count = (std::min)(candidate.element_count, 4u);
      out << "\"instances\": [";
      for (uint32_t i = 0; i < instance_count; ++i) {
        const uint64_t element_index = static_cast<uint64_t>(draw.first_instance)
                                       + candidate.base_offset + i;
        const uint64_t byte_offset = element_index * candidate.stride + candidate.matrix_offset;
        if (i != 0u) out << ",";
        out << "{\"instance\":" << i
            << ",\"first_instance\":" << draw.first_instance
            << ",\"base\":" << candidate.base_offset
            << ",\"resolved_element_index\":" << element_index
            << ",\"byte_offset\":" << byte_offset
            << ",\"world_rows\":"
            << JsonFloatArrayRange(candidate.matrices, static_cast<size_t>(i) * matrix_floats, matrix_floats);
        if (candidate.prev_matrices.size() >= (static_cast<size_t>(i) + 1u) * 12u) {
          out << ",\"prev_rows\":"
              << JsonFloatArrayRange(candidate.prev_matrices, static_cast<size_t>(i) * 12u, 12u);
        }
        const size_t base_index = static_cast<size_t>(i) * matrix_floats;
        const uint32_t tx = (matrix_floats == 16u) ? 12u : 3u;
        const std::array<float, 3> world_origin = {
            candidate.matrices[base_index + tx],
            candidate.matrices[base_index + (matrix_floats == 16u ? 13u : 7u)],
            candidate.matrices[base_index + (matrix_floats == 16u ? 14u : 11u)]};
        out << ",\"world_origin\":["
            << JsonFloat(world_origin[0]) << "," << JsonFloat(world_origin[1]) << ","
            << JsonFloat(world_origin[2]) << "],";
        out << "\"origin_projection\":";
        emit_projection(candidate, i, std::array<float, 3>{0.f, 0.f, 0.f});
        if (!captured.mesh.positions.empty()) {
          out << ",\"vertex_projection\":";
          emit_projection(candidate, i, captured.mesh.positions[0]);
        }
        out << "}";
      }
      out << "],";
    }
    out << "\"pass\": " << (pass ? "true" : "false");
    out << "}";
  }
  out << "\n  ]\n";
  out << "}\n";

  std::string text = out.str();
  const auto dir = WorldOutputDir();
  renodx::utils::path::WriteTextFile(dir / report_name, text);
  renodx::utils::path::WriteTextFile(dir / "world_hint_probe_latest.json", text);
}

}  // namespace falcom_world
