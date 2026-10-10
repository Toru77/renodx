/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: RT resolve/denoise shader bypass.
 *
 * Replaces the three confirmed-live resolve shaders with minimal
 * single-sample variants (see the 0x*.cs_6_6.hlsl files in this folder) so
 * the raw ray signal can be A/B-tested with and without the game's
 * neighborhood filtering. The swap is a live, per-device runtime replacement
 * on the bind-time replacement path (renodx utility shader framework), so the
 * toggle applies within a frame; the game's pipelines are never modified.
 */

#pragma once

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_set>

#include <include/reshade.hpp>
#include <embed/shaders.h>

#include "../../utils/shader.hpp"

namespace denoise {

// The three live resolve shaders (hashes are CRC32 of the original DXIL).
inline constexpr uint32_t kGatherHash = 0x209AB6A4;    // resolve-4tap: stochastic reprojected gather
inline constexpr uint32_t kFilterHash = 0x596D3E8F;    // resolve-spatial: shared-memory spatial filter
inline constexpr uint32_t kBilateralHash = 0x0B33C6D8; // resolve-bilateral: nine-tap bilateral gather

inline std::atomic<bool> bypass_enabled = false;
inline std::atomic<bool> bypass_applied = false;
inline std::atomic<uint32_t> apply_count = 0;
inline std::atomic<reshade::api::device*> current_device = nullptr;

// Pushes the current toggle state onto the device's live runtime-replacement
// map. The bind-time replacement path picks it up on the next bind of each
// pipeline (within a frame); nothing is applied while the toggle is off.
inline void ApplyToDevice() {
  reshade::api::device* device = current_device.load();
  const bool enabled = bypass_enabled.load();
  if (device == nullptr) {
    bypass_applied.store(false);
    return;
  }
  if (enabled) {
    renodx::utils::shader::AddRuntimeReplacement(device, kGatherHash, __0x209AB6A4);
    renodx::utils::shader::AddRuntimeReplacement(device, kFilterHash, __0x596D3E8F);
    renodx::utils::shader::AddRuntimeReplacement(device, kBilateralHash, __0x0B33C6D8);
  } else {
    renodx::utils::shader::RemoveRuntimeReplacements(
        device, {kGatherHash, kFilterHash, kBilateralHash});
  }
  bypass_applied.store(enabled);
  apply_count.fetch_add(1);
}

inline void SetEnabled(bool enabled) {
  bypass_enabled.store(enabled);
  ApplyToDevice();
}

inline void OnInitDevice(reshade::api::device* device) {
  current_device.store(device);
  ApplyToDevice();
}

inline void OnDestroyDevice(reshade::api::device* device) {
  if (current_device.load() == device) {
    current_device.store(nullptr);
    bypass_applied.store(false);
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
  s << "RT denoise bypass: " << (bypass_enabled.load() ? "on" : "off");
  if (bypass_applied.load() != bypass_enabled.load()) s << " (pending next bind)";
  return s.str();
}

inline std::string BuildReportSection() {
  std::stringstream s;
  s << "\n[RT denoise bypass]\n";
  s << "  setting: " << (bypass_enabled.load() ? "on" : "off") << "\n";
  s << "  applied: " << (bypass_applied.load() ? "yes" : "no")
    << " (apply count " << apply_count.load() << ")\n";
  s << "  0x209AB6A4 resolve-4tap      -> per-pixel passthrough (no 4-tap gather)\n";
  s << "  0x596D3E8F resolve-spatial   -> single-sample material path (no filter)\n";
  s << "  0x0B33C6D8 resolve-bilateral -> nearest tap (no 9-tap blend)\n";
  return s.str();
}

}  // namespace denoise
