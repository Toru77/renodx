/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: RT resolve/denoise bypass.
 *
 * Replaces the confirmed-live resolve shaders with minimal per-frame variants
 * (see the 0x*.cs_6_6.hlsl files in this folder) so DLSS-RR receives the raw
 * ray signal instead of the game's neighbourhood-filtered one. Each stage has
 * its own switch and covers every known RT-quality permutation of that stage
 * (medium and high tiers dispatch different compiled hashes for the spatial
 * filter and the reconstruction stage; the add-on replaces whichever one the
 * game actually runs).
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

// Live resolve shader hashes (CRC32 of the original DXIL), per stage and
// RT-quality permutation.
inline constexpr uint32_t kGatherHash = 0x209AB6A4;        // 4-tap stochastic reprojected gather
inline constexpr uint32_t kSpatialHash = 0x596D3E8F;       // spatial filter, medium RT tier
inline constexpr uint32_t kSpatialHighHash = 0x4DAF8A48;   // spatial filter, high RT tier
inline constexpr uint32_t kBilateralHash = 0x0B33C6D8;     // 9-tap bilateral reconstruct, medium RT tier
inline constexpr uint32_t kBilateralHighHash = 0x14FA42AB; // nearest reconstruct, high RT tier
inline constexpr uint32_t kBilateralLowHash = 0xE08B5474;  // 3x3 bilateral reconstruct, low RT tier

struct Replacement {
  uint32_t hash;
  std::span<const uint8_t> data;
};

struct BypassEntry {
  std::span<const Replacement> replacements;
  // Optional payload sets for the same original hashes, selected by the
  // overlay (spatial stage). Empty for entries that only use `replacements`.
  std::span<const std::span<const Replacement>> payloads{};
  // Selects the active payload index when `payloads` is non-empty; null means
  // index 0.
  std::atomic<int>* selector = nullptr;
  std::atomic<bool> enabled{false};
  // Last payload pushed to the device: -1 = none, otherwise the payload index.
  std::atomic<int> applied_payload{-1};
};

inline const Replacement kGatherReplacements[] = {
    {kGatherHash, __0x209AB6A4},
};

// Spatial stage payloads, indexed by the selector:
//   0 = bypass temporal only (3x3 reconstruction, no cross-frame state)
//   1 = bypass both          (centre sample, no cross-frame state)
//   2 = bypass spatial only  (centre sample, cross-frame state kept)
inline const Replacement kSpatialTemporalOnlyReplacements[] = {
    {kSpatialHash, __0x596D3E8F},
    {kSpatialHighHash, __0x4DAF8A48},
};
inline const Replacement kSpatialBothReplacements[] = {
    {kSpatialHash, __0x596D3E8F_both},
    {kSpatialHighHash, __0x4DAF8A48_both},
};
inline const Replacement kSpatialSpatialOnlyReplacements[] = {
    {kSpatialHash, __0x596D3E8F_spatial},
    {kSpatialHighHash, __0x4DAF8A48_spatial},
};
inline const std::span<const Replacement> kSpatialPayloads[] = {
    kSpatialTemporalOnlyReplacements,
    kSpatialBothReplacements,
    kSpatialSpatialOnlyReplacements,
};

inline const Replacement kBilateralReplacements[] = {
    {kBilateralHash, __0x0B33C6D8},
    {kBilateralHighHash, __0x14FA42AB},
    {kBilateralLowHash, __0xE08B5474},
};

// Payload selectors (declared before `bypasses`, which stores their address).
inline std::atomic<int> spatial_payload = 0;

inline BypassEntry bypasses[3] = {
    {kGatherReplacements},
    {kSpatialTemporalOnlyReplacements, kSpatialPayloads, &spatial_payload},
    {kBilateralReplacements},
};

inline std::atomic<reshade::api::device*> current_device = nullptr;

// Pushes each switch state onto the device's live runtime-replacement map for
// every known permutation hash. The bind-time replacement path picks changes
// up on the next bind of each pipeline (within a frame); entries already in
// the requested state are left alone so one switch never re-churns the other
// stages' pipelines. Entries with payload sets pick their index from their
// selector.
inline void ApplyToDevice() {
  reshade::api::device* device = current_device.load();
  if (device == nullptr) {
    for (BypassEntry& entry : bypasses) entry.applied_payload.store(-1);
    return;
  }
  for (BypassEntry& entry : bypasses) {
    const bool enabled = entry.enabled.load();
    int want = -1;
    std::span<const Replacement> set;
    if (enabled) {
      if (!entry.payloads.empty()) {
        int index = (entry.selector != nullptr) ? entry.selector->load() : 0;
        if (index < 0 || index >= static_cast<int>(entry.payloads.size())) index = 0;
        want = index;
        set = entry.payloads[index];
      } else {
        want = 0;
        set = entry.replacements;
      }
    }
    if (entry.applied_payload.load() == want) continue;
    for (const Replacement& replacement : entry.replacements) {
      renodx::utils::shader::RemoveRuntimeReplacements(device, {replacement.hash});
    }
    for (const std::span<const Replacement>& payload : entry.payloads) {
      for (const Replacement& replacement : payload) {
        renodx::utils::shader::RemoveRuntimeReplacements(device, {replacement.hash});
      }
    }
    if (want >= 0) {
      for (const Replacement& replacement : set) {
        renodx::utils::shader::AddRuntimeReplacement(device, replacement.hash, replacement.data);
      }
    }
    entry.applied_payload.store(want);
  }
}

inline void SetEnabled(uint32_t hash, bool enabled) {
  for (BypassEntry& entry : bypasses) {
    for (const Replacement& replacement : entry.replacements) {
      if (replacement.hash == hash) entry.enabled.store(enabled);
    }
  }
  ApplyToDevice();
}

// Spatial stage selector: 0 = original, 1 = bypass temporal only, 2 = bypass
// both, 3 = bypass spatial only. Dedicated entry point so the two tiers'
// payloads can never be registered together.
inline void SetSpatialVariant(int variant) {
  if (variant < 0) variant = 0;
  if (variant > 3) variant = 3;
  bypasses[1].enabled.store(variant != 0);
  spatial_payload.store(variant == 0 ? 0 : variant - 1);
  ApplyToDevice();
}

inline void OnInitDevice(reshade::api::device* device) {
  current_device.store(device);
  ApplyToDevice();
}

inline void OnDestroyDevice(reshade::api::device* device) {
  if (current_device.load() == device) {
    current_device.store(nullptr);
    for (BypassEntry& entry : bypasses) entry.applied_payload.store(-1);
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
