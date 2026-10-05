#pragma once

// Phase 1 recipe persistence.
//
// Auto Research establishes which families are runtime_verified and which
// transform recipe they use. Persisting that knowledge avoids re-running the
// full verification pass on every launch. Saving unions passing layouts into
// the existing file instead of overwriting it: a family can verify different
// layouts on different runs, and previously tested layouts must not disappear
// just because the latest run selected another one. The file carries
// schema/game metadata so incompatible recipes are rejected instead of
// silently trusted.

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "../../../../utils/path.hpp"
#include "../world_state.hpp"
#include "bvh_pool.hpp"

namespace falcom_world::bvh {

inline constexpr uint32_t kWorldRecipeSchema = 1u;
inline constexpr uint32_t kWorldRecipeMaxEvidence = 3u;
inline constexpr const char* kWorldRecipeGame = "sora_2nd";

struct RecipeState {
  uint32_t loaded_count = 0u;
  uint32_t saved_count = 0u;
  uint32_t saved_frame = 0u;
  bool auto_saved = false;
  std::string status = "not loaded";
};

inline RecipeState g_recipes;

struct MergedRecipeLayout {
  uint32_t kind = 0u;
  uint32_t slot = 0u;
  uint32_t offset = 0u;
  uint32_t stride = 0u;
  uint32_t base = 0u;
  uint32_t count = 0u;
};

struct MergedRecipeFamily {
  bool verified = false;
  bool depth_pass = false;
  bool cross_capture_pass = false;
  bool has_transform = false;
  uint32_t transform_kind = 0u;
  uint32_t transform_slot = 0u;
  uint32_t transform_offset = 0u;
  uint32_t transform_stride = 0u;
  int32_t transform_base = 0;
  std::map<std::string, MergedRecipeLayout> layouts;
};

inline std::filesystem::path RecipesPath() {
  return renodx::utils::path::GetOutputPath() / "falcomengine-plus" / "world" / "world_recipes.json";
}

inline std::string RecipeLayoutKey(uint32_t kind, uint32_t slot, uint32_t offset, uint32_t stride) {
  return std::to_string(kind) + ":" + std::to_string(slot) + ":" + std::to_string(offset) + ":" + std::to_string(stride);
}

inline void MergeRecipeLayout(MergedRecipeFamily* family, const MergedRecipeLayout& layout) {
  if (family == nullptr) return;
  const std::string key = RecipeLayoutKey(layout.kind, layout.slot, layout.offset, layout.stride);
  auto it = family->layouts.find(key);
  if (it == family->layouts.end()) {
    MergedRecipeLayout merged = layout;
    merged.count = (std::min)(merged.count, kWorldRecipeMaxEvidence);
    family->layouts.emplace(key, merged);
    return;
  }
  it->second.count = (std::min)(it->second.count + layout.count, kWorldRecipeMaxEvidence);
  it->second.base = layout.base;
}

inline void SaveWorldRecipes() {
  using json = nlohmann::json;
  std::vector<AutoFamilyResult> results;
  std::vector<FamilyStats> families;
  const uint32_t frame = g_state.frame.load();
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    results = g_state.auto_research.results;
    families.reserve(g_state.families.size());
    for (const auto& [hash, family] : g_state.families) {
      (void)hash;
      families.push_back(family);
    }
  }

  std::unordered_map<uint32_t, MergedRecipeFamily> merged;

  // Load existing evidence first so previously passing layouts are never lost.
  {
    std::ifstream file(RecipesPath());
    if (file.is_open()) {
      json existing;
      try {
        file >> existing;
      } catch (...) {
        existing = json();
      }
      if (existing.contains("schema") && existing["schema"].get<uint32_t>() == kWorldRecipeSchema
          && existing.contains("game") && existing["game"].get<std::string>() == kWorldRecipeGame
          && existing.contains("families") && existing["families"].is_array()) {
        for (const auto& entry : existing["families"]) {
          if (!entry.contains("vs_hash")) continue;
          MergedRecipeFamily family;
          family.verified = entry.value("verified", false);
          family.depth_pass = entry.value("depth_pass", false);
          family.cross_capture_pass = entry.value("cross_capture_pass", false);
          if (entry.contains("transform")) {
            const auto& transform = entry["transform"];
            family.has_transform = true;
            family.transform_kind = transform.value("kind", 0u);
            family.transform_slot = transform.value("slot", 0u);
            family.transform_offset = transform.value("offset", 0u);
            family.transform_stride = transform.value("stride", 0u);
            family.transform_base = transform.value("base", 0);
          }
          if (entry.contains("layouts") && entry["layouts"].is_array()) {
            for (const auto& layout_entry : entry["layouts"]) {
              MergedRecipeLayout layout;
              layout.kind = layout_entry.value("kind", 0u);
              layout.slot = layout_entry.value("slot", 0u);
              layout.offset = layout_entry.value("offset", 0u);
              layout.stride = layout_entry.value("stride", 0u);
              layout.base = layout_entry.value("base", 0u);
              layout.count = layout_entry.value("count", 1u);
              MergeRecipeLayout(&family, layout);
            }
          }
          merged[entry["vs_hash"].get<uint32_t>()] = std::move(family);
        }
      }
    }
  }

  // Merge the current run's evidence: every passing layout is retained, not
  // only the families that reached runtime_verified this run.
  for (const auto& result : results) {
    if (result.passing_layouts.empty() && !result.depth_pass) continue;
    MergedRecipeFamily& family = merged[result.vs_hash];
    family.depth_pass = family.depth_pass || result.depth_pass;
    family.cross_capture_pass = family.cross_capture_pass || result.cross_capture_pass;
    for (const auto& layout : result.passing_layouts) {
      MergedRecipeLayout merged_layout;
      merged_layout.kind = static_cast<uint32_t>(layout.kind);
      merged_layout.slot = layout.slot;
      merged_layout.offset = layout.matrix_offset;
      merged_layout.stride = layout.stride;
      merged_layout.base = layout.base_offset;
      merged_layout.count = layout.count;
      MergeRecipeLayout(&family, merged_layout);
    }
    if (!family.has_transform) {
      for (const auto& stats : families) {
        if (stats.vs_hash != result.vs_hash || !stats.transform_found) continue;
        family.has_transform = true;
        family.transform_kind = stats.transform_kind;
        family.transform_slot = stats.transform_slot;
        family.transform_offset = stats.transform_offset;
        family.transform_stride = stats.transform_stride;
        family.transform_base = stats.transform_base;
        break;
      }
    }
  }

  json root;
  root["schema"] = kWorldRecipeSchema;
  root["game"] = kWorldRecipeGame;
  root["generated_frame"] = frame;
  json family_array = json::array();
  uint32_t saved = 0u;
  for (auto& [vs_hash, family] : merged) {
    uint32_t best_count = 0u;
    for (const auto& [key, layout] : family.layouts) {
      (void)key;
      best_count = (std::max)(best_count, layout.count);
    }
    family.verified = family.verified || best_count >= 2u;

    json entry;
    entry["vs_hash"] = vs_hash;
    entry["verified"] = family.verified;
    entry["depth_pass"] = family.depth_pass;
    entry["cross_capture_pass"] = family.cross_capture_pass;
    if (family.has_transform) {
      entry["transform"] = {
          {"kind", family.transform_kind},
          {"slot", family.transform_slot},
          {"offset", family.transform_offset},
          {"stride", family.transform_stride},
          {"base", family.transform_base},
      };
    }
    std::vector<MergedRecipeLayout> layouts;
    layouts.reserve(family.layouts.size());
    for (const auto& [key, layout] : family.layouts) {
      (void)key;
      layouts.push_back(layout);
    }
    std::sort(layouts.begin(), layouts.end(), [](const MergedRecipeLayout& a, const MergedRecipeLayout& b) {
      if (a.count != b.count) return a.count > b.count;
      if (a.slot != b.slot) return a.slot < b.slot;
      return a.offset < b.offset;
    });
    json layout_array = json::array();
    for (const auto& layout : layouts) {
      layout_array.push_back({
          {"kind", layout.kind},
          {"slot", layout.slot},
          {"offset", layout.offset},
          {"stride", layout.stride},
          {"base", layout.base},
          {"count", layout.count},
      });
    }
    entry["layouts"] = layout_array;
    family_array.push_back(entry);
    saved += 1u;
  }
  root["families"] = family_array;

  std::ofstream file(RecipesPath());
  if (!file.is_open()) {
    g_recipes.status = "save failed: cannot open file";
    return;
  }
  file << root.dump(2);
  file.close();
  g_recipes.saved_count = saved;
  g_recipes.saved_frame = frame;
  g_recipes.status = "saved " + std::to_string(saved) + " families";
}

inline uint32_t LoadWorldRecipes() {
  using json = nlohmann::json;
  std::ifstream file(RecipesPath());
  if (!file.is_open()) {
    g_recipes.status = "no recipe file";
    return 0u;
  }
  json root;
  try {
    file >> root;
  } catch (...) {
    g_recipes.status = "parse failed";
    return 0u;
  }
  if (!root.contains("schema") || root["schema"].get<uint32_t>() != kWorldRecipeSchema) {
    g_recipes.status = "schema mismatch";
    return 0u;
  }
  if (!root.contains("game") || root["game"].get<std::string>() != kWorldRecipeGame) {
    g_recipes.status = "game mismatch";
    return 0u;
  }
  if (!root.contains("families") || !root["families"].is_array()) {
    g_recipes.status = "no families";
    return 0u;
  }

  std::lock_guard<std::mutex> lock(g_state.mutex);
  auto& state = g_state.auto_research;
  uint32_t loaded = 0u;
  for (const auto& entry : root["families"]) {
    if (!entry.contains("vs_hash")) continue;
    const uint32_t vs_hash = entry["vs_hash"].get<uint32_t>();
    AutoFamilyResult* result = nullptr;
    for (auto& candidate : state.results) {
      if (candidate.vs_hash == vs_hash) {
        result = &candidate;
        break;
      }
    }
    if (result == nullptr) {
      state.results.push_back(AutoFamilyResult{});
      result = &state.results.back();
    }
    result->vs_hash = vs_hash;
    result->runtime_verified = entry.value("verified", false);
    result->depth_pass = entry.value("depth_pass", false);
    result->cross_capture_pass = entry.value("cross_capture_pass", false);
    result->verdict = result->runtime_verified
                          ? static_cast<uint8_t>(AutoVerdict::Verified)
                          : (result->depth_pass ? static_cast<uint8_t>(AutoVerdict::VerifiedSingleCapture)
                                                : static_cast<uint8_t>(AutoVerdict::Failed));
    result->passing_layouts.clear();
    if (entry.contains("layouts") && entry["layouts"].is_array()) {
      for (const auto& layout_entry : entry["layouts"]) {
        PassingLayout layout;
        layout.kind = static_cast<CandidateKind>(layout_entry.value("kind", 0u));
        layout.slot = layout_entry.value("slot", 0u);
        layout.matrix_offset = layout_entry.value("offset", 0u);
        layout.stride = layout_entry.value("stride", 0u);
        layout.base_offset = layout_entry.value("base", 0u);
        layout.count = layout_entry.value("count", 1u);
        result->passing_layouts.push_back(layout);
      }
    }
    FamilyStats& family = g_state.families[vs_hash];
    family.vs_hash = vs_hash;
    family.verified = result->runtime_verified;
    family.transform_cross_capture = result->cross_capture_pass;
    family.transform_found = result->runtime_verified && !result->passing_layouts.empty();
    if (family.transform_found) {
      // Prefer the canonical instance layout so the pool's persistent fallback
      // resolves the engine's t15/stride-160 path instead of a coincidental
      // probe match.
      PassingLayout chosen;
      const PassingLayout* layout = nullptr;
      if (SelectPoolInstanceLayout(result->passing_layouts, &chosen)) {
        layout = &chosen;
      } else {
        layout = &result->passing_layouts[0];
      }
      family.transform_kind = static_cast<uint8_t>(layout->kind);
      family.transform_slot = layout->slot;
      family.transform_offset = layout->matrix_offset;
      family.transform_stride = layout->stride;
      family.transform_base = layout->base_offset;
    }
    loaded += 1u;
  }
  g_recipes.loaded_count = loaded;
  g_recipes.status = "loaded " + std::to_string(loaded) + " families";
  return loaded;
}

}  // namespace falcom_world::bvh
