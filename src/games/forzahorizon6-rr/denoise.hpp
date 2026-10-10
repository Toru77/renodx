/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: RT resolve/denoise shader bypass.
 *
 * Replaces the confirmed-live resolve shaders with minimal single-sample
 * variants (see the 0x*.cs_6_6.hlsl files in this folder) so the raw ray
 * signal can be A/B-tested with and without the game's neighborhood
 * filtering. Each stage has its own switch, and covers every known RT-quality
 * permutation of that stage (medium and high tiers currently dispatch
 * different compiled hashes for the spatial filter and the reconstruction
 * stage; the add-on replaces whichever one the game actually runs).
 *
 * The swap is a live, per-device runtime replacement on the bind-time
 * replacement path (renodx utility shader framework), so a toggle applies
 * within a frame; the game's pipelines are never modified.
 */

#pragma once

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>

#include <include/reshade.hpp>
#include <embed/shaders.h>

#include "../../utils/shader.hpp"

namespace denoise {

// Live resolve shader hashes (CRC32 of the original DXIL), per stage and
// RT-quality permutation.
inline constexpr uint32_t kGatherHash = 0x209AB6A4;        // 4-tap stochastic reprojected gather
inline constexpr uint32_t kSpatialHash = 0x596D3E8F;       // spatial filter, medium RT tier
inline constexpr uint32_t kSpatialHighHash = 0x4DAF8A48;   // spatial filter, high RT tier
inline constexpr uint32_t kBilateralHash = 0x0B33C6D8;     // 9-tap bilateral reconstruct, medium RT tier
inline constexpr uint32_t kBilateralHighHash = 0x14FA42AB; // nearest reconstruct, high RT tier

struct Replacement {
  uint32_t hash;
  std::span<const uint8_t> data;
};

struct BypassEntry {
  std::span<const Replacement> replacements;
  const char* label;
  const char* short_label;
  const char* effect;
  std::atomic<bool> enabled{false};
  std::atomic<bool> applied{false};
};

inline const Replacement kGatherReplacements[] = {
    {kGatherHash, __0x209AB6A4},
};
inline const Replacement kSpatialReplacements[] = {
    {kSpatialHash, __0x596D3E8F},
    {kSpatialHighHash, __0x4DAF8A48},
};
inline const Replacement kBilateralReplacements[] = {
    {kBilateralHash, __0x0B33C6D8},
    {kBilateralHighHash, __0x14FA42AB},
};

inline BypassEntry bypasses[3] = {
    {kGatherReplacements, "0x209AB6A4 resolve-4tap", "4tap",
     "per-pixel passthrough (no 4-tap gather)"},
    {kSpatialReplacements, "0x596D3E8F/0x4DAF8A48 spatial filter", "spatial",
     "single-sample material path (no filter)"},
    {kBilateralReplacements, "0x0B33C6D8/0x14FA42AB reconstruct", "bilateral",
     "bilinear reconstruction (no bilateral/nearest blocking)"},
};

inline std::atomic<uint32_t> apply_count = 0;
inline std::atomic<reshade::api::device*> current_device = nullptr;

inline bool AnyEnabled() {
  for (const BypassEntry& entry : bypasses) {
    if (entry.enabled.load()) return true;
  }
  return false;
}

// Pushes each switch state onto the device's live runtime-replacement map for
// every known permutation hash. The bind-time replacement path picks changes
// up on the next bind of each pipeline (within a frame); entries already in
// the requested state are left alone so one switch never re-churns the other
// stages' pipelines.
inline void ApplyToDevice() {
  reshade::api::device* device = current_device.load();
  if (device == nullptr) {
    for (BypassEntry& entry : bypasses) entry.applied.store(false);
    return;
  }
  for (BypassEntry& entry : bypasses) {
    const bool enabled = entry.enabled.load();
    if (entry.applied.load() == enabled) continue;
    for (const Replacement& replacement : entry.replacements) {
      if (enabled) {
        renodx::utils::shader::AddRuntimeReplacement(device, replacement.hash, replacement.data);
      } else {
        renodx::utils::shader::RemoveRuntimeReplacements(device, {replacement.hash});
      }
    }
    entry.applied.store(enabled);
  }
  apply_count.fetch_add(1);
}

inline void SetEnabled(uint32_t hash, bool enabled) {
  for (BypassEntry& entry : bypasses) {
    for (const Replacement& replacement : entry.replacements) {
      if (replacement.hash == hash) entry.enabled.store(enabled);
    }
  }
  ApplyToDevice();
}

inline void OnInitDevice(reshade::api::device* device) {
  current_device.store(device);
  ApplyToDevice();
}

inline void OnDestroyDevice(reshade::api::device* device) {
  if (current_device.load() == device) {
    current_device.store(nullptr);
    for (BypassEntry& entry : bypasses) entry.applied.store(false);
  }
}

inline void Use(DWORD fdw_reason) {
  renodx::utils::shader::Use(fdw_reason);
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      break;
  }
}

inline std::string StatusLine() {
  std::stringstream s;
  s << "RT denoise bypass:";
  for (const BypassEntry& entry : bypasses) {
    s << " " << entry.short_label << ":" << (entry.enabled.load() ? "on" : "off");
  }
  return s.str();
}

inline std::string BuildReportSection() {
  std::stringstream s;
  s << "\n[RT denoise bypass]\n";
  for (const BypassEntry& entry : bypasses) {
    s << "  " << entry.label << ": " << (entry.enabled.load() ? "on" : "off")
      << ", applied: " << (entry.applied.load() ? "yes" : "no")
      << " -> " << entry.effect << "\n";
    for (const Replacement& replacement : entry.replacements) {
      char hash_text[16] = {};
      snprintf(hash_text, sizeof hash_text, "0x%08X", replacement.hash);
      s << "    - " << hash_text << "\n";
    }
  }
  s << "  apply count: " << apply_count.load() << "\n";
  return s.str();
}

}  // namespace denoise
