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

inline std::filesystem::path LatestReportPath() {
  return WorldOutputDir() / "world_auto_report_latest.json";
}

inline std::string EmitCaptureJson(const AutoCapture& capture) {
  std::ostringstream out;
  out << "{";
  out << "\"frame\":" << capture.frame << ",";
  out << "\"draw_serial\":" << capture.draw_serial << ",";
  out << "\"kind\":\"" << CandidateKindName(capture.kind) << "\",";
  out << "\"stage\":" << static_cast<uint32_t>(capture.stage) << ",";
  out << "\"slot\":" << capture.slot << ",";
  out << "\"matrix_offset\":" << capture.matrix_offset << ",";
  out << "\"stride\":" << capture.stride << ",";
  out << "\"base_offset\":" << capture.base_offset << ",";
  out << "\"instances_tested\":" << capture.instances_tested << ",";
  out << "\"instances_ok\":" << capture.instances_ok << ",";
  out << "\"projection_score\":" << JsonFloat(capture.projection_score) << ",";
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
  out << "}";
  return out.str();
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
    } else {
      out << "\"draws\":0,\"unique_meshes\":0,\"instances\":0,\"weighted_triangles\":0,";
    }
    out << "\"srv_mask\":" << result.srv_mask << ",";
    out << "\"instance_offset\":" << result.instance_offset << ",";
    out << "\"instance_stride\":" << result.read_stride << ",";
    out << "\"read_window\":{\"offset\":" << result.read_offset << ",\"size\":" << result.read_size << "},";
    out << "\"evidence\":{";
    out << "\"manual_reference\":" << (hint != nullptr ? "true" : "false") << ",";
    out << "\"runtime_census\":true,";
    out << "\"projection\":" << (result.cross_capture_pass ? "\"pass\"" : "\"inconclusive\"") << ",";
    out << "\"depth_validation\":" << (result.depth_pass ? "\"pass\"" : "\"fail\"") << ",";
    out << "\"decompilation\":\"pending\",\"final\":\"pending\"},";
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
  SetStatus("report written: " + report_name);
}

}  // namespace falcom_world
