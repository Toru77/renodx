/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: RT resolve/denoise bypass.
 *
 * On High RTGI, replaces the resolve shaders with minimal per-frame variants
 * (see the 0x*.cs_6_6.hlsl files in this folder) so DLSS-RR receives the raw
 * ray signal instead of the game's neighbourhood-filtered one. Medium and Low
 * keep the game's own resolve; the add-on detects the tier and gates the whole
 * feature (addon.cpp).
 *
 * The swap is a live, per-device runtime replacement on the bind-time
 * replacement path (renodx utility shader framework), so a toggle applies
 * within a frame; the game's pipelines are never modified.
 */

#pragma once

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <span>

#include <include/reshade.hpp>
#include <embed/shaders.h>

#include "../../utils/shader.hpp"

namespace denoise {

// Live High-RTGI resolve shader hashes (CRC32 of the original DXIL).
inline constexpr uint32_t kGatherHash = 0x209AB6A4;      // 4-tap stochastic reprojected gather
inline constexpr uint32_t kSpatialHighHash = 0x4DAF8A48;  // spatial filter, high RT tier

struct Replacement {
  uint32_t hash;
  std::span<const uint8_t> data;
};

struct BypassEntry {
  std::span<const Replacement> replacements;
  std::atomic<bool> enabled{false};
  // True while the replacements are pushed to the device.
  std::atomic<bool> applied{false};
};

inline const Replacement kGatherReplacements[] = {
    {kGatherHash, __0x209AB6A4},
};

// Centre-sample spatial payload for the High tier: no spatial neighbourhood
// and no cross-frame state.
inline const Replacement kSpatialReplacements[] = {
    {kSpatialHighHash, __0x4DAF8A48_both},
};

inline BypassEntry bypasses[2] = {
    {kGatherReplacements},
    {kSpatialReplacements},
};

inline std::atomic<reshade::api::device*> current_device = nullptr;

// Pushes each switch state onto the device's live runtime-replacement map. The
// bind-time replacement path picks changes up on the next bind of each pipeline
// (within a frame); entries already in the requested state are left alone.
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
      renodx::utils::shader::RemoveRuntimeReplacements(device, {replacement.hash});
    }
    if (enabled) {
      for (const Replacement& replacement : entry.replacements) {
        renodx::utils::shader::AddRuntimeReplacement(device, replacement.hash, replacement.data);
      }
    }
    entry.applied.store(enabled);
  }
}

// Turns the bypass that owns `hash` on or off.
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

}  // namespace denoise
