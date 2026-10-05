#pragma once

// Sora 2nd shader-family classification for the Phase 0 census.
//
// The PS hash lists are the hashes already identified in this mod (lighting,
// SSR, SSAO/character shadow, shadow maps, TAA, DoF, foliage, outline). They
// exist so the census does not propose post-process, UI or foliage passes as
// world geometry candidates.

#include "../world_state.hpp"

namespace falcom_world {

inline bool IsKnownNonGeometryPs(uint32_t hash) {
  static constexpr uint32_t kHashes[] = {
      0xCA3D8596u,  // lighting
      0xE2F406C7u,  // ssr1
      0x17F931DEu,  // ssr2
      0x752B2580u,  // ssao + character shadow
      0xF320152Cu,  // shadow
      0xF1575FE3u,  // shadow
      0x9D91FAC3u,  // TAA
      0x5BBEC5A3u,  // DoF 1
      0xCD6FC25Du,  // DoF 2
      0x42F7D5B9u,  // character outline
      0xF1EC53A8u,  // static foliage
      0x533C1853u,  // potflower
      0x2F107485u,  // plantpot
      0xF6733CDDu,  // moving foliage
      0x2DADE2B8u,  // leaves
      0xAA835FE0u,  // foliage
      0x882FADE1u,  // foliage
      0xA2E80908u,  // flower
      0x5C33E765u,  // flower
      0x46FCDC51u,  // clutter
      0xA8291F30u,  // clutter3
      0x37C0064Bu,  // clutter2
      0x73DC9D7Eu,  // bush
      0x5F527E52u,  // bush
  };
  for (const uint32_t known : kHashes) {
    if (known == hash) return true;
  }
  return false;
}

inline bool IsLightingHash(uint32_t hash) {
  return hash == kLightingHashSora2nd;
}

// relaxed=true is the pool gate: depth-only prepass draws have no color
// target and often no pixel shader, yet Auto Research verifies families from
// them too, and the pool dedupes the duplicate geometry on MeshKey/InstanceKey
// instead of excluding the pass.
inline bool IsGeometryCandidate(const DrawRecord& draw, bool relaxed = false) {
  if (draw.ps_hash != 0u && IsKnownNonGeometryPs(draw.ps_hash)) return false;
  if (!relaxed && draw.ps_hash == 0u) return false;
  if (draw.has_skin_inputs) return false;
  if (draw.vb.handle == 0u || draw.dsv.handle == 0u || draw.method != 1u || !draw.has_index_buffer) return false;
  if (!relaxed && draw.rtv0.handle == 0u) return false;
  if (draw.index_count < 3u || (draw.index_count % 3u) != 0u) return false;
  if (draw.blend_enable || !draw.depth_enable || !draw.depth_write) return false;
  if (draw.topology != reshade::api::primitive_topology::undefined
      && draw.topology != reshade::api::primitive_topology::triangle_list) {
    return false;
  }
  return true;
}

inline uint64_t MeshKey(const DrawRecord& draw) {
  uint64_t key = 1469598103934665603ull;
  const auto mix = [&key](uint64_t value) {
    key ^= value;
    key *= 1099511628211ull;
  };
  mix(draw.vb.handle);
  mix(draw.ib.handle);
  mix(draw.first_index);
  mix(draw.index_count);
  mix(static_cast<uint64_t>(static_cast<int64_t>(draw.vertex_offset)));
  mix(draw.vb_stride);
  mix(draw.input_layout.handle);
  return key;
}

inline uint64_t HintDrawKey(const DrawRecord& draw) {
  uint64_t key = MeshKey(draw);
  const auto mix = [&key](uint64_t value) {
    key ^= value;
    key *= 1099511628211ull;
  };
  mix(draw.first_instance);
  mix(draw.instance_count);
  mix(draw.vs_hash);
  mix(draw.ps_hash);
  mix(draw.method);
  return key;
}

}  // namespace falcom_world
