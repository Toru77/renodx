#pragma once

// Kai hero light: zeroes the color of the one light that matches a PlayerLight config of the Kai light table
// (kai_player_light_table.h) when the light buffer is mapped. Matching reads the records before the patch, so the
// game's own writes are never fed back into the match.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>

#include "./kai_player_light_table.h"
#include "./world/rtgi/rtgi_resources.hpp"

namespace falcom_world::kai {

inline constexpr size_t kHeroConfigCount = std::size(kKaiPlayerLightTable);
inline constexpr float kHeroMatchTolerance = 0.006f;
inline constexpr uint16_t kHeroLatchFrames = 60u;
inline constexpr size_t kHeroLightCount = 512u;  // t11 dynamicLights_g records
inline constexpr uint64_t kHeroLightBufferBytes = kHeroLightCount * sizeof(rtgi::LightParamCpu);

struct KaiHeroLightState {
  std::array<uint16_t, kHeroConfigCount> run = {};           // consecutive frames with exactly one active match
  std::array<int32_t, kHeroConfigCount> latched_index = {};  // matched light index when latched, else -1
  std::array<int32_t, kHeroConfigCount> match_index = {};    // matched light index on the last evaluated frame
  int32_t first_latched_config = -1;                         // config_id of the first latched config, else -1
  uint64_t evaluated_frame = UINT64_MAX;
  void* mapped = nullptr;  // light buffer pointer of the current map
  uint64_t mapped_bytes = 0u;
  std::array<rtgi::LightParamCpu, kHeroLightCount> copy = {};  // records read from the mapped buffer before patching
  uint32_t maps_in_frame = 0u;
  uint32_t max_maps_per_frame = 0u;
  uint32_t patched = 0u;
  uint64_t eval_us = 0u;
  std::chrono::steady_clock::time_point last_log = {};
};

inline KaiHeroLightState g_kai_hero_light;

inline bool NearHeroValue(float value, float target) {
  return std::fabs(value - target) <= kHeroMatchTolerance;
}

// Index of the single active light (radius > 0) matching the config, or -1 when no light or two or more lights match.
inline int32_t FindUniqueHeroMatch(const rtgi::LightParamCpu* lights, size_t count, const KaiPlayerLightConfig& config) {
  int32_t match = -1;
  uint32_t matches = 0u;
  for (size_t i = 0; i < count; ++i) {
    const rtgi::LightParamCpu& light = lights[i];
    const bool same = light.radius > 0.f && NearHeroValue(light.radius, config.radius)
                      && NearHeroValue(light.color[0], config.color_r) && NearHeroValue(light.color[1], config.color_g)
                      && NearHeroValue(light.color[2], config.color_b) && NearHeroValue(light.attenuation, config.attenuation);
    if (same) {
      ++matches;
      match = static_cast<int32_t>(i);
    }
  }
  return matches == 1u ? match : -1;
}

// Runs on every unmap, before any patch. A new frame advances the run counters; further maps in the same frame only
// refresh the latched indices.
inline void EvaluateKaiHeroLights(const rtgi::LightParamCpu* lights, size_t count, bool new_frame) {
  g_kai_hero_light.first_latched_config = -1;
  for (size_t c = 0; c < kHeroConfigCount; ++c) {
    const int32_t match = FindUniqueHeroMatch(lights, count, kKaiPlayerLightTable[c]);
    if (new_frame) {
      g_kai_hero_light.run[c] = match < 0 ? 0u : (std::min)(static_cast<uint16_t>(g_kai_hero_light.run[c] + 1u), kHeroLatchFrames);
    }
    const bool latched = match >= 0 && g_kai_hero_light.run[c] >= kHeroLatchFrames;
    g_kai_hero_light.latched_index[c] = latched ? match : -1;
    g_kai_hero_light.match_index[c] = match;
    if (latched && g_kai_hero_light.first_latched_config < 0) {
      g_kai_hero_light.first_latched_config = static_cast<int32_t>(kKaiPlayerLightTable[c].config_id);
    }
  }
}

// Clears the run counters and latches. Called when the toggle turns off.
inline void ResetKaiHeroLight() {
  g_kai_hero_light.run = {};
  g_kai_hero_light.match_index = {};
  g_kai_hero_light.latched_index.fill(-1);
  g_kai_hero_light.first_latched_config = -1;
}

// Zeroes color, chara_color, scatter_color and specular_intensity of the latched lights only.
inline uint32_t PatchKaiHeroLights(rtgi::LightParamCpu* lights, size_t count) {
  uint32_t patched = 0u;
  for (size_t c = 0; c < kHeroConfigCount; ++c) {
    const int32_t index = g_kai_hero_light.latched_index[c];
    if (index >= 0 && static_cast<size_t>(index) < count) {
      rtgi::LightParamCpu& light = lights[index];
      std::memset(light.color, 0, sizeof(light.color));
      std::memset(light.chara_color, 0, sizeof(light.chara_color));
      std::memset(light.scatter_color, 0, sizeof(light.scatter_color));
      light.specular_intensity = 0.f;
      ++patched;
    }
  }
  return patched;
}

}  // namespace falcom_world::kai
