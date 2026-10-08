#pragma once

// Alpha atlas. An alpha-tested material owns one slice of the atlas: 1024 x 256 alpha texels,
// four 8-bit texels per uint (texel x in byte x & 3), stored as one 256 x 256 uint layer of a
// Texture2DArray. AlphaSliceTable is the slice bookkeeping; AlphaGpu holds the GPU objects that
// alpha_live.hpp creates while alpha_foliage is on (none while it is off).

#include <array>
#include <cstdint>

namespace falcom_world {

inline constexpr uint32_t kAlphaAtlasSlices = 256u;
inline constexpr uint32_t kAlphaSliceWords = 256u;    // uints per row and per column of a slice
inline constexpr uint32_t kAlphaSliceTexelsX = 1024u;  // kAlphaSliceWords * 4 texels per uint
inline constexpr uint32_t kAlphaSliceTexelsY = 256u;
inline constexpr uint32_t kAlphaBlitsPerFrame = 4u;      // slices filled per present
inline constexpr uint32_t kAlphaCopiesPerFrame = 4u;     // source copies made per frame (pool, at draw time)
inline constexpr uint32_t kAlphaSourcesMax = 64u;        // live source copies
inline constexpr uint64_t kAlphaSourceBytesMax = 256ull << 20;  // bytes of live source copies (mip 0)
inline constexpr uint32_t kAlphaSliceQuarantineFrames = 16u;  // a released slice is reused after this many frames

// One per slice, read by the trace (material slot = slice). 32 bytes.
struct AlphaMaterialGPU {
  float threshold = 0.f;
  float scroll[2] = {};
  uint32_t swizzle = 0u;
  uint32_t slice = 0u;
  uint32_t pad[3] = {};
};
static_assert(sizeof(AlphaMaterialGPU) == 32u, "AlphaMaterialGPU is 32 bytes");

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
    if (slice >= kAlphaAtlasSlices || owner[slice] == 0u) return;
    owner[slice] = 0u;
    used -= 1u;
  }
};

}  // namespace falcom_world
