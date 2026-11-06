/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: RTGI and SSR denoiser replacements.
 *
 * On Medium and High RTGI, replaces the denoising stages so DLSS-RR receives
 * the current-frame ray signal. The High RTGI pointwise resolve is also
 * replaced with an equivalent implementation. Optional SSR switches disable
 * the checkerboard cleanup and the separable roughness-mip filters.
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

// Live RTGI shader hashes (CRC32 of the original DXIL). DevKit names the High
// Known shader tags from the DevKit PDB names:
//   temporalfilter_0xAF4E3D44 (High), temporalfilter_0x95FA8898 (Medium),
//   spatialfilter_0x480011D5 (shared), resolve_0x019CE131 (High), and
//   upscale2x_0x4067B14C (Medium).
inline constexpr uint32_t kSpatialFilterDiskHash = 0x480011D5;  // confirmed in High and Medium
inline constexpr uint32_t kTemporalFilterHighHash = 0xAF4E3D44;
inline constexpr uint32_t kResolveHighHash = 0x019CE131;       // RTGI_Resolve, draw 5441
inline constexpr uint32_t kTemporalFilterMediumHash = 0x95FA8898;
inline constexpr uint32_t kUpscale2xMediumHash = 0x4067B14C;
inline constexpr uint32_t kSsrCheckerboardTidyHash = 0x313E9A06;
inline constexpr uint32_t kSsrFilterMipHorizontalHash = 0xF0DE4E22;
inline constexpr uint32_t kSsrFilterMipVerticalHash = 0x24848E0B;

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

inline const Replacement kSpatialFilterDiskReplacements[] = {
    {kSpatialFilterDiskHash, __0x480011D5},
};

// These temporal-filter replacements remove the cross-frame/history weighting
// and keep only the current-frame sample reconstruction.
inline const Replacement kTemporalFilterHighReplacements[] = {
    {kTemporalFilterHighHash, __0xAF4E3D44},
};

// High RTGI resolve is pointwise; preserve its depth/normal/radiance transform
// while replacing the game hash with a source-controlled shader.
inline const Replacement kResolveHighReplacements[] = {
    {kResolveHighHash, __0x019CE131},
};

inline const Replacement kTemporalFilterMediumReplacements[] = {
    {kTemporalFilterMediumHash, __0x95FA8898},
};

// Medium's Upscale2x pass both denoises (nine-tap bilateral weighting) and
// upsamples the half-res pair to the full-res feed; the replacement keeps the
// bilinear 2x upscale but drops the weighting.
inline const Replacement kUpscale2xMediumReplacements[] = {
    {kUpscale2xMediumHash, __0x4067B14C},
};

// This pass clamps checkerboard samples in place. A no-op leaves the earlier
// current-frame reflection values untouched.
inline const Replacement kSsrCheckerboardTidyReplacements[] = {
    {kSsrCheckerboardTidyHash, __0x313E9A06},
};

// These hashes are the repeated separable roughness-mip blur passes. Their
// replacements copy each source mip texel directly to its destination.
inline const Replacement kSsrFilterMipReplacements[] = {
    {kSsrFilterMipHorizontalHash, __0xF0DE4E22},
    {kSsrFilterMipVerticalHash, __0x24848E0B},
};

inline BypassEntry bypasses[7] = {
    {kSpatialFilterDiskReplacements},
    {kTemporalFilterHighReplacements},
    {kTemporalFilterMediumReplacements},
    {kUpscale2xMediumReplacements},
    {kResolveHighReplacements},
    {kSsrCheckerboardTidyReplacements},
    {kSsrFilterMipReplacements},
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
