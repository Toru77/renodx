/*
 * Copyright (C) 2026 Carlos Lopez, speedlemur
 * SPDX-License-Identifier: MIT
 *
 * forzahorizon6-rr: RT resolve/denoise shader bypass.
 *
 * Replaces the confirmed-live resolve shaders with minimal per-frame
 * variants (see the 0x*.cs_6_6.hlsl files in this folder) so the raw ray
 * signal can be A/B-tested with and without the game's neighborhood
 * filtering. Each stage has its own switch, and covers every known RT-quality
 * permutation of that stage (medium and high tiers currently dispatch
 * different compiled hashes for the spatial filter and the reconstruction
 * stage; the add-on replaces whichever one the game actually runs).
 *
 * The fifth switch replaces the GI temporal resolve pair (0x087EDF0D and
 * 0xAD556EBA, one per ray-signal ping-pong member). The originals reproject
 * the previous frame's downsampled upscaled output with motion vectors and
 * blend it into the current GI estimate after material-ID validation; the
 * replacements substitute the current-frame probe estimate for the
 * reprojected history so the GI path stops accumulating across frames.
 *
 * The sixth switch targets the tile-refresh scheduler instead of a resolve
 * stage: 0x7A3FD6D7 expands the classifier's sparse dirty-tile worklist into
 * the per-pixel ray queues, and 0x9BFFD1F7 builds the expansion dispatch
 * args. The replacements expand the full 160x90 tile grid every frame
 * (engine per-pixel sub-sampling preserved), removing the tile cache that
 * lets standing-still views converge over several frames.
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

#include "../../utils/hash.hpp"
#include "../../utils/shader.hpp"
#include "pass_map.hpp"

namespace denoise {

// Live resolve shader hashes (CRC32 of the original DXIL), per stage and
// RT-quality permutation.
inline constexpr uint32_t kGatherHash = 0x209AB6A4;        // 4-tap stochastic reprojected gather
inline constexpr uint32_t kSpatialHash = 0x596D3E8F;       // spatial filter, medium RT tier
inline constexpr uint32_t kSpatialHighHash = 0x4DAF8A48;   // spatial filter, high RT tier
inline constexpr uint32_t kBilateralHash = 0x0B33C6D8;     // 9-tap bilateral reconstruct, medium RT tier
inline constexpr uint32_t kBilateralHighHash = 0x14FA42AB; // nearest reconstruct, high RT tier
inline constexpr uint32_t kProbeBlendHash = 0xD9CDA0AC;    // probe accumulation blend (out = A + w * B)
inline constexpr uint32_t kGiResolveHashA = 0x087EDF0D;    // GI temporal resolve, signal ping-pong member A
inline constexpr uint32_t kGiResolveHashB = 0xAD556EBA;    // GI temporal resolve, signal ping-pong member B
inline constexpr uint32_t kTileExpandHash = 0x7A3FD6D7;    // tile list -> per-pixel ray queue expansion
inline constexpr uint32_t kTileArgsHash = 0x9BFFD1F7;      // worklist pad + expansion dispatch args

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
inline const Replacement kProbeReplacements[] = {
    {kProbeBlendHash, __0xD9CDA0AC},
};
inline const Replacement kGiResolveReplacements[] = {
    {kGiResolveHashA, __0x087EDF0D},
    {kGiResolveHashB, __0xAD556EBA},
};
// Sixth switch: the tile-refresh scheduler. 0x7A3FD6D7 expands the tile
// classifier's sparse dirty-tile worklist into the per-pixel ray queues and
// 0x9BFFD1F7 builds the expansion dispatch args from the worklist counts. The
// replacements expand the full 160x90 tile grid every frame (the engine's own
// class-decode, per-pixel sub-sampling and queue routing are preserved), so
// the dirty-tile cache that makes standing-still views converge across frames
// is removed. This is the rawest per-frame ray signal the architecture
// exposes; expect a frame-rate cost.
inline const Replacement kTileRefreshReplacements[] = {
    {kTileExpandHash, __0x7A3FD6D7},
    {kTileArgsHash, __0x9BFFD1F7},
};

inline BypassEntry bypasses[6] = {
    {kGatherReplacements, "0x209AB6A4 resolve-4tap", "4tap",
     "per-pixel passthrough (no 4-tap gather)"},
    {kSpatialReplacements, "0x596D3E8F/0x4DAF8A48 spatial filter", "spatial",
     "current-frame reconstruction (no history, no cache, no wave smoothing)"},
    {kBilateralReplacements, "0x0B33C6D8/0x14FA42AB reconstruct", "bilateral",
     "raw nearest sample of the resolve pair (no bilateral weighting)"},
    {kProbeReplacements, "0xD9CDA0AC probe accumulation", "probe",
     "new contribution only (state term dropped)"},
    {kGiResolveReplacements, "0x087EDF0D/0xAD556EBA GI temporal resolve", "temporal",
     "current-frame only (reprojected history replaced by probe estimate)"},
    {kTileRefreshReplacements, "0x7A3FD6D7/0x9BFFD1F7 tile refresh gating", "tiles",
     "every tile expanded every frame (no dirty-tile cache)"},
};

inline std::atomic<uint32_t> apply_count = 0;
inline std::atomic<reshade::api::device*> current_device = nullptr;

// Per-replacement pipeline usage counters. The shader util rebuilds a game
// pipeline whenever it first binds a shader that has a live runtime
// replacement; when that rebuild fails it logs an error and the original
// pipeline keeps running. These counters make both outcomes visible in the
// report without reading the log: pipelines built with our replacement
// payload vs. pipelines built with the original shader.
struct UsageStat {
  uint32_t hash;                          // original game shader hash
  std::atomic<uint32_t> payload_crc{0};   // crc32 of the compiled replacement (0 = uncached)
  std::atomic<uint32_t> replaced_seen{0}; // pipelines created with the replacement payload
  std::atomic<uint32_t> original_seen{0}; // pipelines created with the original shader
};

inline UsageStat usage_stats[] = {
    {kGatherHash},
    {kSpatialHash},
    {kSpatialHighHash},
    {kBilateralHash},
    {kBilateralHighHash},
    {kProbeBlendHash},
    {kGiResolveHashA},
    {kGiResolveHashB},
    {kTileExpandHash},
    {kTileArgsHash},
};

inline void CachePayloadHashes() {
  for (const BypassEntry& entry : bypasses) {
    for (const Replacement& replacement : entry.replacements) {
      const uint32_t crc = renodx::utils::hash::ComputeCRC32(
          replacement.data.data(), replacement.data.size());
      for (UsageStat& stat : usage_stats) {
        if (stat.hash == replacement.hash) stat.payload_crc.store(crc);
      }
    }
  }
}

inline void OnInitPipeline(
    reshade::api::device*, reshade::api::pipeline_layout, uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects, reshade::api::pipeline) {
  if (subobjects == nullptr) return;
  for (uint32_t i = 0; i < subobject_count; ++i) {
    if (subobjects[i].type != reshade::api::pipeline_subobject_type::compute_shader) continue;
    const auto* desc = static_cast<const reshade::api::shader_desc*>(subobjects[i].data);
    if (desc == nullptr || desc->code == nullptr || desc->code_size == 0) break;
    const uint32_t crc = renodx::utils::hash::ComputeCRC32(
        static_cast<const uint8_t*>(desc->code), desc->code_size);
    for (UsageStat& stat : usage_stats) {
      if (crc == stat.hash) {
        stat.original_seen.fetch_add(1, std::memory_order_relaxed);
        break;
      }
      const uint32_t payload = stat.payload_crc.load(std::memory_order_relaxed);
      if (payload != 0 && crc == payload) {
        stat.replaced_seen.fetch_add(1, std::memory_order_relaxed);
        break;
      }
    }
  }
}

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

// Spatial stage on/off. Use this instead of SetEnabled for the spatial hash
// (kept as a dedicated entry point for the stage's experiment history).
inline void SetSpatialVariant(int variant) {
  bypasses[1].enabled.store(variant != 0);
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
      CachePayloadHashes();
      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::init_pipeline>(OnInitPipeline);
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::init_pipeline>(OnInitPipeline);
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
      char hash_text[24] = {};
      snprintf(hash_text, sizeof hash_text, "0x%08X", replacement.hash);
      s << "    - " << hash_text;
      for (const UsageStat& stat : usage_stats) {
        if (stat.hash != replacement.hash) continue;
        s << "  pipelines: replaced=" << stat.replaced_seen.load()
          << " original=" << stat.original_seen.load();
        const uint32_t payload = stat.payload_crc.load();
        if (payload != 0) {
          s << " payload=0x" << std::hex << payload << std::dec
            << " seen=" << (pass_map::WasSeen(payload) ? "yes" : "no");
        }
        break;
      }
      s << "\n";
    }
  }
  s << "  apply count: " << apply_count.load() << "\n";
  return s.str();
}

}  // namespace denoise
