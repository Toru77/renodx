#pragma once

// Phase 1 BVH construction (CPU).
//
// Every tree is a Morton-code LBVH built top-down: leaves are sorted by
// (Morton code, primitive), each leaf range is split at the highest differing
// Morton bit (equal codes: at the middle), the larger half is deferred while
// the smaller one is processed next, and a reverse index sweep fits the
// internal bounds (parents are allocated before their children). This is the
// algorithm and node layout the earlier GPU build ran with one GPU thread per
// tree; it now runs on the CPU so the live BVH (bvh_live.hpp) can build a
// mesh's BLAS once when the mesh enters the GPU store and rebuild the TLAS
// whenever the region's instance set changes, without GPU-side stalls. The
// trace shaders read the same BVHNodeGPU / BVHLeafGPU arrays as before.
//
// Tree layout: node 0 is the root; an internal node stores its left and right
// child indices (relative to the tree's first node); a leaf node stores
// kBvhLeafFlag | leaf index (relative to the tree's first leaf). A tree over n
// leaves has exactly 2n - 1 nodes. Every tree is structurally validated
// (ValidateBvh) before it is uploaded.

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace falcom_world::bvh {

struct BVHLeafGPU {
  float bounds_min[4];
  float bounds_max[4];
  uint32_t prim = 0u;
  uint32_t code = 0u;
  uint32_t pad0 = 0u;
  uint32_t pad1 = 0u;
};

struct BVHNodeGPU {
  float bounds_min[3];
  uint32_t child_or_leaf = 0u;  // internal: left child; leaf: kBvhLeafFlag | leaf index
  float bounds_max[3];
  uint32_t sibling_or_right = 0u;  // internal: right child
};

// Must match world_bvh_types.hlsli exactly.
static_assert(sizeof(BVHLeafGPU) == 48u, "BVHLeafGPU layout must match world_bvh_types.hlsli");
static_assert(sizeof(BVHNodeGPU) == 32u, "BVHNodeGPU layout must match world_bvh_types.hlsli");

inline constexpr uint32_t kBvhLeafFlag = 0x80000000u;  // BVH_LEAF_FLAG in world_bvh_types.hlsli

inline uint32_t ExpandMortonBits(uint32_t value) {
  value = (value * 0x00010001u) & 0xFF0000FFu;
  value = (value * 0x00000101u) & 0x0F00F00Fu;
  value = (value * 0x00000011u) & 0xC30C30C3u;
  value = (value * 0x00000005u) & 0x49249249u;
  return value;
}

inline uint32_t Morton3D(float x, float y, float z) {
  const auto quantize = [](float value) {
    value = (std::min)((std::max)(value, 0.f), 1.f);
    return (std::min)(static_cast<uint32_t>(value * 1024.f), 1023u);
  };
  return ExpandMortonBits(quantize(x)) * 4u
         + ExpandMortonBits(quantize(y)) * 2u
         + ExpandMortonBits(quantize(z));
}

inline BVHLeafGPU MakeLeaf(const float* bounds_min, const float* bounds_max, uint32_t prim) {
  BVHLeafGPU leaf;
  leaf.bounds_min[0] = bounds_min[0];
  leaf.bounds_min[1] = bounds_min[1];
  leaf.bounds_min[2] = bounds_min[2];
  leaf.bounds_min[3] = 0.f;
  leaf.bounds_max[0] = bounds_max[0];
  leaf.bounds_max[1] = bounds_max[1];
  leaf.bounds_max[2] = bounds_max[2];
  leaf.bounds_max[3] = 0.f;
  leaf.prim = prim;
  return leaf;
}

// Assigns Morton codes relative to the leaves' common bounds and sorts the
// leaves by (code, prim). Prims are unique within a tree, so the order is
// total and deterministic.
inline void SortBvhLeaves(std::vector<BVHLeafGPU>* leaves) {
  if (leaves->empty()) return;
  float bounds_min[3] = {1e30f, 1e30f, 1e30f};
  float bounds_max[3] = {-1e30f, -1e30f, -1e30f};
  for (const BVHLeafGPU& leaf : *leaves) {
    for (int k = 0; k < 3; ++k) {
      bounds_min[k] = (std::min)(bounds_min[k], leaf.bounds_min[k]);
      bounds_max[k] = (std::max)(bounds_max[k], leaf.bounds_max[k]);
    }
  }
  for (int k = 0; k < 3; ++k) {
    if (!(bounds_max[k] > bounds_min[k])) {
      bounds_min[k] -= 1e-3f;
      bounds_max[k] += 1e-3f;
    }
  }
  const float extent[3] = {
      bounds_max[0] - bounds_min[0],
      bounds_max[1] - bounds_min[1],
      bounds_max[2] - bounds_min[2]};
  for (BVHLeafGPU& leaf : *leaves) {
    const float cx = (leaf.bounds_min[0] + leaf.bounds_max[0]) * 0.5f;
    const float cy = (leaf.bounds_min[1] + leaf.bounds_max[1]) * 0.5f;
    const float cz = (leaf.bounds_min[2] + leaf.bounds_max[2]) * 0.5f;
    leaf.code = Morton3D(
        (cx - bounds_min[0]) / extent[0],
        (cy - bounds_min[1]) / extent[1],
        (cz - bounds_min[2]) / extent[2]);
  }
  // Sort 16-byte keys instead of 48-byte leaves, then gather.
  struct Key {
    uint64_t order;
    uint32_t index;
  };
  std::vector<Key> keys(leaves->size());
  for (uint32_t i = 0; i < keys.size(); ++i) {
    keys[i].order = (static_cast<uint64_t>((*leaves)[i].code) << 32u) | (*leaves)[i].prim;
    keys[i].index = i;
  }
  std::sort(keys.begin(), keys.end(), [](const Key& a, const Key& b) { return a.order < b.order; });
  std::vector<BVHLeafGPU> sorted(leaves->size());
  for (size_t i = 0; i < keys.size(); ++i) sorted[i] = (*leaves)[keys[i].index];
  leaves->swap(sorted);
}

// Last leaf of the left half of [first, last] (first < last): the split is at
// the highest Morton bit in which the range's first and last codes differ.
inline uint32_t SplitBvhRange(const std::vector<BVHLeafGPU>& leaves, uint32_t first, uint32_t last) {
  const uint32_t first_code = leaves[first].code;
  const uint32_t last_code = leaves[last].code;
  if (first_code == last_code) return (first + last) / 2u;
  const uint32_t bit = static_cast<uint32_t>(std::bit_width(first_code ^ last_code)) - 1u;
  uint32_t lo = first;
  uint32_t hi = last;
  while (lo + 1u < hi) {
    const uint32_t mid = (lo + hi) / 2u;
    if (((leaves[mid].code >> bit) & 1u) != 0u) {
      hi = mid;
    } else {
      lo = mid;
    }
  }
  return lo;
}

// Builds the tree over sorted `leaves` into `nodes` (2n - 1 nodes).
inline bool BuildBvhTree(const std::vector<BVHLeafGPU>& leaves, std::vector<BVHNodeGPU>* nodes) {
  nodes->clear();
  if (leaves.empty()) return true;
  const uint32_t leaf_count = static_cast<uint32_t>(leaves.size());
  const uint32_t node_total = 2u * leaf_count - 1u;
  nodes->resize(node_total);

  struct Range {
    uint32_t first;
    uint32_t last;
    uint32_t node;
  };
  std::vector<Range> stack;
  stack.reserve(64u);
  uint32_t node_count = 1u;
  uint32_t first = 0u;
  uint32_t last = leaf_count - 1u;
  uint32_t node = 0u;

  while (true) {
    if (node >= node_total) return false;
    if (first == last) {
      const BVHLeafGPU& leaf = leaves[first];
      BVHNodeGPU& out = (*nodes)[node];
      for (int k = 0; k < 3; ++k) {
        out.bounds_min[k] = leaf.bounds_min[k];
        out.bounds_max[k] = leaf.bounds_max[k];
      }
      out.child_or_leaf = kBvhLeafFlag | first;
      out.sibling_or_right = 0u;
      if (stack.empty()) break;
      first = stack.back().first;
      last = stack.back().last;
      node = stack.back().node;
      stack.pop_back();
      continue;
    }

    const uint32_t split = SplitBvhRange(leaves, first, last);
    const uint32_t left = node_count++;
    const uint32_t right = node_count++;
    BVHNodeGPU& out = (*nodes)[node];
    for (int k = 0; k < 3; ++k) {
      out.bounds_min[k] = 1e30f;
      out.bounds_max[k] = -1e30f;
    }
    out.child_or_leaf = left;
    out.sibling_or_right = right;

    const uint32_t left_size = split - first + 1u;
    const uint32_t right_size = last - split;
    if (left_size >= right_size) {
      stack.push_back({first, split, left});
      first = split + 1u;
      node = right;
    } else {
      stack.push_back({split + 1u, last, right});
      last = split;
      node = left;
    }
  }
  if (node_count != node_total) return false;

  // Parents are allocated before their children, so a reverse index sweep
  // fits every internal node from its already-finished children.
  for (uint32_t i = node_total; i-- > 0u;) {
    BVHNodeGPU& out = (*nodes)[i];
    if ((out.child_or_leaf & kBvhLeafFlag) != 0u) continue;
    const BVHNodeGPU& left = (*nodes)[out.child_or_leaf];
    const BVHNodeGPU& right = (*nodes)[out.sibling_or_right];
    for (int k = 0; k < 3; ++k) {
      out.bounds_min[k] = (std::min)(left.bounds_min[k], right.bounds_min[k]);
      out.bounds_max[k] = (std::max)(left.bounds_max[k], right.bounds_max[k]);
    }
  }
  return true;
}

struct BvhValidation {
  bool ok = true;
  uint32_t node_count = 0u;
  uint32_t expected_node_count = 0u;
  uint32_t bad_child = 0u;
  uint32_t bad_bounds = 0u;
  uint32_t bad_leaf = 0u;
  bool root_covers = false;
};

inline BvhValidation ValidateBvh(
    const std::vector<BVHNodeGPU>& nodes,
    const std::vector<BVHLeafGPU>& leaves) {
  BvhValidation result;
  result.node_count = static_cast<uint32_t>(nodes.size());
  result.expected_node_count = leaves.empty() ? 0u : (2u * static_cast<uint32_t>(leaves.size()) - 1u);
  if (result.node_count != result.expected_node_count) {
    result.ok = false;
    return result;
  }
  if (nodes.empty()) return result;

  for (uint32_t i = 0u; i < nodes.size(); ++i) {
    const BVHNodeGPU& node = nodes[i];
    for (int k = 0; k < 3; ++k) {
      if (!std::isfinite(node.bounds_min[k]) || !std::isfinite(node.bounds_max[k])
          || node.bounds_max[k] < node.bounds_min[k]) {
        result.bad_bounds += 1u;
      }
    }
    if ((node.child_or_leaf & kBvhLeafFlag) != 0u) {
      if ((node.child_or_leaf & ~kBvhLeafFlag) >= leaves.size()) result.bad_leaf += 1u;
      continue;
    }
    const uint32_t left = node.child_or_leaf;
    const uint32_t right = node.sibling_or_right;
    if (left >= nodes.size() || right >= nodes.size() || left == right) {
      result.bad_child += 1u;
      continue;
    }
    const BVHNodeGPU& left_node = nodes[left];
    const BVHNodeGPU& right_node = nodes[right];
    for (int k = 0; k < 3; ++k) {
      const float eps = 1e-3f * (1.f + std::fabs(node.bounds_max[k] - node.bounds_min[k]));
      if (left_node.bounds_min[k] < node.bounds_min[k] - eps
          || left_node.bounds_max[k] > node.bounds_max[k] + eps
          || right_node.bounds_min[k] < node.bounds_min[k] - eps
          || right_node.bounds_max[k] > node.bounds_max[k] + eps) {
        result.bad_bounds += 1u;
      }
    }
  }

  float union_min[3] = {1e30f, 1e30f, 1e30f};
  float union_max[3] = {-1e30f, -1e30f, -1e30f};
  for (const auto& leaf : leaves) {
    for (int k = 0; k < 3; ++k) {
      union_min[k] = (std::min)(union_min[k], leaf.bounds_min[k]);
      union_max[k] = (std::max)(union_max[k], leaf.bounds_max[k]);
    }
  }
  result.root_covers = true;
  for (int k = 0; k < 3; ++k) {
    const float eps = 1e-3f * (1.f + std::fabs(union_max[k] - union_min[k]));
    if (nodes[0].bounds_min[k] > union_min[k] + eps || nodes[0].bounds_max[k] < union_max[k] - eps) {
      result.root_covers = false;
    }
  }
  result.ok = result.bad_child == 0u && result.bad_bounds == 0u && result.bad_leaf == 0u && result.root_covers;
  return result;
}

inline const char* DescribeBvhValidation(const BvhValidation& validation) {
  if (validation.ok) return nullptr;
  if (validation.node_count != validation.expected_node_count) return "node count is not 2n - 1";
  if (validation.bad_child != 0u) return "bad child index";
  if (validation.bad_leaf != 0u) return "bad leaf index";
  if (validation.bad_bounds != 0u) return "child bounds outside the parent, or non-finite bounds";
  return "root does not cover the leaves";
}

// One mesh's BLAS: a leaf per triangle (prim = triangle index; triangles with
// an index outside the vertex list are left out and counted) and its tree.
struct MeshBlas {
  std::vector<BVHLeafGPU> leaves;
  std::vector<BVHNodeGPU> nodes;
  uint32_t skipped_triangles = 0u;
};

// Returns nullptr on success, else why the mesh has no usable BLAS.
inline const char* BuildMeshBlas(
    const std::vector<std::array<float, 3>>& positions,
    const std::vector<uint32_t>& indices,
    MeshBlas* out) {
  out->leaves.clear();
  out->nodes.clear();
  out->skipped_triangles = 0u;
  const size_t triangle_count = indices.size() / 3u;
  out->leaves.reserve(triangle_count);
  for (size_t triangle = 0u; triangle < triangle_count; ++triangle) {
    const uint32_t i0 = indices[triangle * 3u + 0u];
    const uint32_t i1 = indices[triangle * 3u + 1u];
    const uint32_t i2 = indices[triangle * 3u + 2u];
    if (i0 >= positions.size() || i1 >= positions.size() || i2 >= positions.size()) {
      out->skipped_triangles += 1u;
      continue;
    }
    const auto& p0 = positions[i0];
    const auto& p1 = positions[i1];
    const auto& p2 = positions[i2];
    float tri_min[3];
    float tri_max[3];
    bool finite = true;
    for (int k = 0; k < 3; ++k) {
      tri_min[k] = (std::min)({p0[k], p1[k], p2[k]});
      tri_max[k] = (std::max)({p0[k], p1[k], p2[k]});
      finite = finite && std::isfinite(tri_min[k]) && std::isfinite(tri_max[k]);
    }
    if (!finite) {
      out->skipped_triangles += 1u;
      continue;
    }
    out->leaves.push_back(MakeLeaf(tri_min, tri_max, static_cast<uint32_t>(triangle)));
  }
  if (out->leaves.empty()) return "no valid triangle";
  SortBvhLeaves(&out->leaves);
  if (!BuildBvhTree(out->leaves, &out->nodes)) return "tree construction failed";
  const char* invalid = DescribeBvhValidation(ValidateBvh(out->nodes, out->leaves));
  if (invalid != nullptr) return invalid;
  return nullptr;
}

}  // namespace falcom_world::bvh
