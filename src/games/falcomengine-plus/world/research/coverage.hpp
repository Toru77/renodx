#pragma once

// Coverage metrics and CSV export for the Phase 0 gate.
//
// Three metrics are reported separately so a passing aggregate cannot hide a
// missing major family: weighted triangle coverage, unique-mesh coverage, and
// per-family verification for labeled major families (terrain/buildings/props).
// Glass/windows are reported separately and never determine the main gate.

#include <algorithm>
#include <filesystem>
#include <sstream>

#include "../world_state.hpp"
#include "reference_hints.hpp"
#include "../../../../utils/path.hpp"

namespace falcom_world {

struct CoverageMetrics {
  uint64_t candidate_triangles = 0u;
  uint64_t verified_triangles = 0u;
  uint64_t candidate_meshes = 0u;
  uint64_t verified_meshes = 0u;
  uint64_t candidate_instances = 0u;
  uint64_t verified_instances = 0u;
  float primary = 0.f;
  float secondary = 0.f;
  bool primary_pass = false;
  bool secondary_pass = false;
  bool major_family_missing = false;
  bool pass = false;
};

inline bool IsMajorLabel(uint8_t label) {
  switch (static_cast<FamilyLabel>(label)) {
    case FamilyLabel::Terrain:
    case FamilyLabel::Buildings:
    case FamilyLabel::Props:
      return true;
    default:
      return false;
  }
}

inline CoverageMetrics ComputeCoverage() {
  std::lock_guard<std::mutex> lock(g_state.mutex);
  CoverageMetrics metrics;
  for (const auto& [hash, family] : g_state.families) {
    (void)hash;
    if (!family.candidate) continue;
    metrics.candidate_triangles += family.triangles;
    metrics.candidate_meshes += family.meshes.size();
    metrics.candidate_instances += family.instances;
    if (family.verified) {
      metrics.verified_triangles += family.triangles;
      metrics.verified_meshes += family.meshes.size();
      metrics.verified_instances += family.instances;
    }
  }

  if (metrics.candidate_triangles > 0u) {
    metrics.primary = static_cast<float>(metrics.verified_triangles) / static_cast<float>(metrics.candidate_triangles);
  }
  if (metrics.candidate_meshes > 0u) {
    metrics.secondary = static_cast<float>(metrics.verified_meshes) / static_cast<float>(metrics.candidate_meshes);
  }

  for (const auto& [hash, family] : g_state.families) {
    (void)hash;
    if (!family.candidate || !IsMajorLabel(family.label)) continue;
    const uint64_t threshold = metrics.candidate_triangles / 100u;
    if (family.triangles < threshold) continue;
    if (!family.verified) metrics.major_family_missing = true;
  }

  metrics.primary_pass = metrics.primary >= 0.70f;
  metrics.secondary_pass = metrics.secondary >= 0.50f;
  metrics.pass = metrics.primary_pass && metrics.secondary_pass && !metrics.major_family_missing;
  return metrics;
}

inline std::filesystem::path WorldOutputDir() {
  auto path = renodx::utils::path::GetOutputPath() / "falcomengine-plus" / "world";
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  return path;
}

inline void DumpCensusCsv() {
  std::vector<FamilyStats> families;
  std::vector<DrawRecord> draws;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    families.reserve(g_state.families.size());
    for (const auto& [hash, family] : g_state.families) {
      (void)hash;
      families.push_back(family);
    }
    draws = g_state.ring;
  }

  std::sort(families.begin(), families.end(), [](const FamilyStats& a, const FamilyStats& b) {
    return a.triangles > b.triangles;
  });

  const auto dir = WorldOutputDir();

  std::ostringstream family_csv;
  family_csv << "vs_hash,hint_tag,hint_category,label,label_source,draws,unique_meshes,instances,"
                "weighted_triangles,indices,candidate,candidate_draws,verified,transform_kind,transform_slot,"
                "transform_offset,transform_stride,transform_base,transform_has_prev,"
                "depth_write_draws,blend_draws,skin_draws,ps_hashes\n";
  for (const auto& family : families) {
    const ReferenceHint* hint = FindReferenceHint(family.vs_hash);
    const bool has_transform = family.verified && family.transform_found;
    family_csv << "0x" << std::hex << family.vs_hash << std::dec << ","
               << (hint != nullptr ? hint->tag : "") << ","
               << (hint != nullptr ? hint->category : "") << ","
               << FamilyLabelName(family.label) << ","
               << LabelSourceName(family.label_source) << ","
               << family.draws << ","
               << family.meshes.size() << ","
               << family.instances << ","
               << family.triangles << ","
               << family.indices << ","
               << (family.candidate ? 1 : 0) << ","
               << family.candidate_draws << ","
               << (family.verified ? 1 : 0) << ","
               << (has_transform ? CandidateKindName(static_cast<CandidateKind>(family.transform_kind)) : "") << ","
               << (has_transform ? family.transform_slot : 0u) << ","
               << (has_transform ? family.transform_offset : 0u) << ","
               << (has_transform ? family.transform_stride : 0u) << ","
               << (has_transform ? family.transform_base : 0u) << ","
               << (has_transform && family.transform_has_prev ? 1 : 0) << ","
               << family.depth_write_draws << ","
               << family.blend_draws << ","
               << family.skin_draws << ","
               << family.ps_hashes.size() << "\n";
  }
  std::string family_text = family_csv.str();
  renodx::utils::path::WriteTextFile(dir / "census_families.csv", family_text);

  std::ostringstream draw_csv;
  draw_csv << "frame,serial,method,vs_hash,ps_hash,index_count,vertex_count,instance_count,first_index,"
              "vertex_offset,first_instance,vb,ib,vb_stride,index_size,blend,depth_write,candidate,"
              "skin_inputs,vs_cb_mask,ps_cb_mask,vs_srv_mask,input_layout,rtv,dsv,topology,vp_x,vp_y,vp_w,vp_h\n";
  for (const auto& draw : draws) {
    draw_csv << draw.frame << ","
             << draw.serial << ","
             << static_cast<uint32_t>(draw.method) << ","
             << "0x" << std::hex << draw.vs_hash << std::dec << ","
             << "0x" << std::hex << draw.ps_hash << std::dec << ","
             << draw.index_count << ","
             << draw.vertex_count << ","
             << draw.instance_count << ","
             << draw.first_index << ","
             << draw.vertex_offset << ","
             << draw.first_instance << ","
             << "0x" << std::hex << draw.vb.handle << std::dec << ","
             << "0x" << std::hex << draw.ib.handle << std::dec << ","
             << draw.vb_stride << ","
             << draw.index_size << ","
             << (draw.blend_enable ? 1 : 0) << ","
             << (draw.depth_write ? 1 : 0) << ","
             << (draw.is_candidate ? 1 : 0) << ","
             << (draw.has_skin_inputs ? 1 : 0) << ","
             << draw.vs_cb_mask << ","
             << draw.ps_cb_mask << ","
             << draw.vs_srv_mask << ","
             << "0x" << std::hex << draw.input_layout.handle << std::dec << ","
             << "0x" << std::hex << draw.rtv0.handle << std::dec << ","
             << "0x" << std::hex << draw.dsv.handle << std::dec << ","
             << static_cast<uint32_t>(draw.topology) << ","
             << draw.viewport.x << ","
             << draw.viewport.y << ","
             << draw.viewport.width << ","
             << draw.viewport.height << "\n";
  }
  std::string draw_text = draw_csv.str();
  renodx::utils::path::WriteTextFile(dir / "census_draws.csv", draw_text);

  const CoverageMetrics metrics = ComputeCoverage();
  std::ostringstream coverage;
  coverage << "metric,value,pass\n";
  coverage << "primary_triangle_coverage," << metrics.primary << "," << (metrics.primary_pass ? 1 : 0) << "\n";
  coverage << "secondary_mesh_coverage," << metrics.secondary << "," << (metrics.secondary_pass ? 1 : 0) << "\n";
  coverage << "candidate_instances," << metrics.candidate_instances << ",,\n";
  coverage << "verified_instances," << metrics.verified_instances << ",,\n";
  coverage << "major_family_missing," << (metrics.major_family_missing ? 1 : 0) << "," << (metrics.major_family_missing ? 0 : 1) << "\n";
  coverage << "overall," << (metrics.pass ? 1 : 0) << "," << (metrics.pass ? 1 : 0) << "\n";
  std::string coverage_text = coverage.str();
  renodx::utils::path::WriteTextFile(dir / "coverage.csv", coverage_text);

  SetStatus("csv dumped to " + dir.string());
}

}  // namespace falcom_world
