#pragma once

// Alpha atlas. An alpha-tested material owns one slice of the atlas: 1024 x 256 alpha texels,
// four 8-bit texels per uint (texel x in byte x & 3), stored as one 256 x 256 uint layer of a
// Texture2DArray. AlphaSliceTable is the slice bookkeeping; AlphaGpu holds the GPU objects that
// alpha_live.hpp creates while alpha_foliage is on (none while it is off).

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace falcom_world {

inline constexpr uint32_t kAlphaAtlasSlices = 256u;
inline constexpr uint32_t kAlphaSliceWords = 256u;    // uints per row and per column of a slice
inline constexpr uint32_t kAlphaSliceTexelsX = 1024u;  // kAlphaSliceWords * 4 texels per uint
inline constexpr uint32_t kAlphaSliceTexelsY = 256u;
inline constexpr uint32_t kAlphaBlitsPerFrame = 4u;      // slices filled per present
inline constexpr uint32_t kAlphaCopiesPerFrame = 4u;     // source copies made per frame (pool, at draw time)
inline constexpr uint32_t kAlphaSourcesMax = 64u;        // live source copies
inline constexpr uint32_t kAlphaKeylessSourcesMax = 16u;  // live keyless copies (indirect draws awaiting their resolve)
inline constexpr uint64_t kAlphaSourceBytesMax = 256ull << 20;  // bytes of live source copies (mip 0)
// Atlas-only sources use their own budget, so alpha foliage cannot starve them. A copy is freed once its slice is filled.
inline constexpr uint32_t kAlbedoSourcesMax = 256u;
inline constexpr uint32_t kAlbedoCopiesPerFrame = 4u;
inline constexpr uint64_t kAlbedoSourceBytesMax = 64ull << 20;
inline constexpr uint32_t kAlbedoRetryFrames = 120u;  // an atlas-only capture is retried after this many presents
inline constexpr uint32_t kAlphaSliceQuarantineFrames = 16u;  // a released slice is reused after this many frames
inline constexpr uint32_t kAlphaOrphanFrames = 600u;  // a keyless copy (indirect draw) is held this many presents for its resolve

// Albedo atlas: one RGBA8 slice per unique t0 source texture, sampled at the hit's UV by the path tracer (A35).
inline constexpr uint32_t kAlbedoAtlasSlices = 512u;
inline constexpr uint32_t kAlbedoSliceSize = 128u;
inline constexpr uint32_t kAlbedoMaterials = 2048u;  // one entry per (slice, scroll, swizzle)

enum class AlphaUvRangeStatus { NotRequired,
                                Valid,
                                Invalid };

inline AlphaUvRangeStatus ValidateAlphaUvRange(uint32_t material, bool force_opaque, float uv_slot, uint32_t uv_count,
                                               uint32_t i0, uint32_t i1, uint32_t i2, uint32_t* uv_base) {
  if (material == 0u || force_opaque) return AlphaUvRangeStatus::NotRequired;
  if (uv_base == nullptr || !std::isfinite(uv_slot) || uv_slot < 1.f || uv_slot != std::floor(uv_slot)
      || uv_slot >= 4294967296.f) return AlphaUvRangeStatus::Invalid;
  const uint32_t base = static_cast<uint32_t>(uv_slot) - 1u;
  const uint32_t max_index = (std::max)({i0, i1, i2});
  if (base >= uv_count || max_index >= uv_count - base) return AlphaUvRangeStatus::Invalid;
  *uv_base = base;
  return AlphaUvRangeStatus::Valid;
}

inline bool ValidAlphaAtlasLayout(uint32_t word_width, uint32_t height, uint32_t slices) {
  return word_width == kAlphaSliceWords && height == kAlphaSliceWords && slices == kAlphaAtlasSlices;
}

inline bool ValidAlphaAtlasSlice(uint32_t slice, uint32_t layers) {
  return layers == kAlphaAtlasSlices && slice < layers;
}

// One per slice, read by the trace (material slot = slice). 32 bytes.
struct AlphaMaterialGPU {
  float threshold = 0.f;
  float scroll[2] = {};
  uint32_t swizzle = 0u;
  uint32_t slice = 0u;
  uint32_t pad[3] = {};
};
static_assert(sizeof(AlphaMaterialGPU) == 32u, "AlphaMaterialGPU is 32 bytes");

// One per material, read by the PT shaders through header.w - 1. 32 bytes, mirroring AlphaMaterialGPU.
struct AlbedoMaterialGPU {
  uint32_t slice = 0u;
  float scroll[2] = {};
  uint32_t swizzle = 0u;
  uint32_t uv_slot = 0u;  // unused by the shader: UVs come from the mesh's bbox_min.w
  uint32_t pad[3] = {};
};
static_assert(sizeof(AlbedoMaterialGPU) == 32u, "AlbedoMaterialGPU is 32 bytes");

// Same bookkeeping as AlphaSliceTable, sized for the albedo atlas. The owner is a source id, not a mesh uid.
struct AlbedoSliceTable {
  std::array<uint64_t, kAlbedoAtlasSlices> owner = {};
  uint32_t used = 0u;

  int32_t Acquire(uint64_t source_id) {
    for (uint32_t slice = 0u; slice < kAlbedoAtlasSlices; ++slice) {
      if (owner[slice] != 0u) continue;
      owner[slice] = source_id;
      used += 1u;
      return static_cast<int32_t>(slice);
    }
    return -1;
  }

  void Release(uint32_t slice) {
    if (slice >= kAlbedoAtlasSlices || owner[slice] == 0u) return;
    owner[slice] = 0u;
    used -= 1u;
  }
};

// One entry per distinct (slice, scroll, swizzle), addressed by material index + 1 in the instance header.
struct AlbedoMaterialTable {
  struct Entry {
    AlbedoMaterialGPU gpu = {};
    uint32_t refs = 0u;  // draw keys mapped to this entry
  };
  std::array<Entry, kAlbedoMaterials> entries = {};
  uint32_t used = 0u;

  // Adds a reference to a matching entry, or claims a free index for a new one; -1 when the table is full.
  int32_t FindOrInsert(uint32_t slice, const float scroll[2], uint32_t swizzle) {
    for (uint32_t i = 0u; i < kAlbedoMaterials; ++i) {
      const Entry& entry = entries[i];
      if (entry.refs != 0u && entry.gpu.slice == slice && entry.gpu.scroll[0] == scroll[0]
          && entry.gpu.scroll[1] == scroll[1] && entry.gpu.swizzle == swizzle) {
        entries[i].refs += 1u;
        return static_cast<int32_t>(i);
      }
    }
    for (uint32_t i = 0u; i < kAlbedoMaterials; ++i) {
      Entry& entry = entries[i];
      if (entry.refs != 0u) continue;
      entry.gpu = {.slice = slice, .scroll = {scroll[0], scroll[1]}, .swizzle = swizzle};
      entry.refs = 1u;
      used += 1u;
      return static_cast<int32_t>(i);
    }
    return -1;
  }

  void Release(uint32_t index) {
    if (index >= kAlbedoMaterials || entries[index].refs == 0u) return;
    entries[index].refs -= 1u;
    if (entries[index].refs == 0u) {
      entries[index] = {};
      used -= 1u;
    }
  }
};

// Four alpha bytes as the blit packs them (texel 0 in the low byte).
inline uint32_t PackAlphaTexels(const uint8_t* texels) {
  return static_cast<uint32_t>(texels[0]) | (static_cast<uint32_t>(texels[1]) << 8u)
         | (static_cast<uint32_t>(texels[2]) << 16u) | (static_cast<uint32_t>(texels[3]) << 24u);
}

struct AlphaSliceTable {
  std::array<uint64_t, kAlphaAtlasSlices> owner = {};  // mesh uid per slice, 0 = free
  uint32_t used = 0u;

  // Lowest free slice for a mesh uid, or -1 when every slice is taken.
  int32_t Acquire(uint64_t mesh_uid) {
    for (uint32_t slice = 0u; slice < kAlphaAtlasSlices; ++slice) {
      if (owner[slice] != 0u) continue;
      owner[slice] = mesh_uid;
      used += 1u;
      return static_cast<int32_t>(slice);
    }
    return -1;
  }

  void Release(uint32_t slice) {
    if (!ValidAlphaAtlasSlice(slice, kAlphaAtlasSlices) || owner[slice] == 0u) return;
    owner[slice] = 0u;
    used -= 1u;
  }
};

}  // namespace falcom_world
