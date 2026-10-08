// Native harness: the CPU LBVH (bvh_build.hpp) against a literal transcription
// of the removed GPU builder (world_bvh_build.hlsli BuildLbvh, 64-entry stack,
// firstbithigh), byte for byte, over random leaf sets including duplicate
// Morton codes, single leaves and pairs.
#include <cstdio>
#include <random>
#include "gen/mock_base.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_build.hpp"
namespace bvh = falcom_world::bvh;
static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

typedef uint32_t uint;
static uint firstbithigh(uint v) { uint b = 31; while (((v >> b) & 1u) == 0u) --b; return b; }
// ---- transcription of world_bvh_build.hlsli (as removed) ----
static uint BvhSplitByMorton(const std::vector<bvh::BVHLeafGPU>& leaves, uint leaf_base, uint first, uint last) {
  const uint first_code = leaves[leaf_base + first].code;
  const uint last_code = leaves[leaf_base + last].code;
  if (first_code == last_code) return (first + last) / 2u;
  const uint bit = firstbithigh(first_code ^ last_code);
  uint lo = first, hi = last;
  while (lo + 1u < hi) { const uint mid = (lo + hi) / 2u; if (((leaves[leaf_base + mid].code >> bit) & 1u) != 0u) hi = mid; else lo = mid; }
  return lo;
}
static void BuildLbvh(const std::vector<bvh::BVHLeafGPU>& leaves, uint leaf_base, uint leaf_count, std::vector<bvh::BVHNodeGPU>& nodes, uint node_base) {
  if (leaf_count == 0u) return;
  uint stack_first[64], stack_last[64], stack_node[64];
  uint top = 0u, node_count = 1u, first = 0u, last = leaf_count - 1u, node = 0u;
  while (true) {
    if (first == last) {
      const bvh::BVHLeafGPU leaf = leaves[leaf_base + first];
      bvh::BVHNodeGPU out_node;
      for (int k = 0; k < 3; ++k) { out_node.bounds_min[k] = leaf.bounds_min[k]; out_node.bounds_max[k] = leaf.bounds_max[k]; }
      out_node.child_or_leaf = 0x80000000u | first;
      out_node.sibling_or_right = 0u;
      nodes[node_base + node] = out_node;
      if (top == 0u) break;
      top--; first = stack_first[top]; last = stack_last[top]; node = stack_node[top];
      continue;
    }
    const uint split = BvhSplitByMorton(leaves, leaf_base, first, last);
    const uint left = node_count++, right = node_count++;
    bvh::BVHNodeGPU out_node;
    for (int k = 0; k < 3; ++k) { out_node.bounds_min[k] = 1e30f; out_node.bounds_max[k] = -1e30f; }
    out_node.child_or_leaf = left; out_node.sibling_or_right = right;
    nodes[node_base + node] = out_node;
    const uint left_size = split - first + 1u, right_size = last - split;
    if (left_size >= right_size) { stack_first[top] = first; stack_last[top] = split; stack_node[top] = left; top++; first = split + 1u; node = right; }
    else { stack_first[top] = split + 1u; stack_last[top] = last; stack_node[top] = right; top++; last = split; node = left; }
  }
  for (int i = (int)node_count - 1; i >= 0; --i) {
    bvh::BVHNodeGPU out_node = nodes[node_base + (uint)i];
    if ((out_node.child_or_leaf & 0x80000000u) != 0u) continue;
    const bvh::BVHNodeGPU l = nodes[node_base + out_node.child_or_leaf], r = nodes[node_base + out_node.sibling_or_right];
    for (int k = 0; k < 3; ++k) { out_node.bounds_min[k] = std::min(l.bounds_min[k], r.bounds_min[k]); out_node.bounds_max[k] = std::max(l.bounds_max[k], r.bounds_max[k]); }
    nodes[node_base + (uint)i] = out_node;
  }
}
// ---- end transcription ----

int main() {
  std::mt19937 rng(11);
  int trees = 0;
  for (int round = 0; round < 400; ++round) {
    const uint32_t n = round < 4 ? round + 1u : 1u + rng() % (round < 300 ? 300u : 20000u);
    const bool clustered = round % 3 == 0;  // many equal Morton codes
    std::uniform_real_distribution<float> u(clustered ? 0.f : -50.f, clustered ? 0.5f : 50.f);
    std::vector<bvh::BVHLeafGPU> leaves;
    for (uint32_t i = 0; i < n; ++i) {
      const float c[3] = {u(rng), u(rng) * (round % 5 == 0 ? 0.f : 1.f), u(rng)};
      const float mn[3] = {c[0] - 0.1f, c[1] - 0.1f, c[2] - 0.1f}, mx[3] = {c[0] + 0.1f, c[1] + 0.1f, c[2] + 0.1f};
      leaves.push_back(bvh::MakeLeaf(mn, mx, (i * 7919u) % 1000003u));
    }
    bvh::SortBvhLeaves(&leaves);
    for (size_t i = 1; i < leaves.size(); ++i) {
      CHECK(leaves[i - 1].code < leaves[i].code || (leaves[i - 1].code == leaves[i].code && leaves[i - 1].prim < leaves[i].prim), "sorted");
    }
    std::vector<bvh::BVHNodeGPU> cpu;
    CHECK(bvh::BuildBvhTree(leaves, &cpu), "built");
    std::vector<bvh::BVHNodeGPU> gpu(2 * n - 1);
    BuildLbvh(leaves, 0, n, gpu, 0);
    CHECK(cpu.size() == gpu.size() && std::memcmp(cpu.data(), gpu.data(), cpu.size() * sizeof(bvh::BVHNodeGPU)) == 0, "identical to the GPU builder (n=%u)", n);
    CHECK(bvh::ValidateBvh(cpu, leaves).ok, "valid (n=%u)", n);
    ++trees;
  }
  std::vector<bvh::BVHNodeGPU> none;
  CHECK(bvh::BuildBvhTree({}, &none) && none.empty(), "empty");
  bvh::MeshBlas blas;
  CHECK(bvh::BuildMeshBlas({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {0, 1, 2, 0, 1, 7}, &blas) == nullptr && blas.leaves.size() == 1 && blas.skipped_triangles == 1, "skips bad triangle");
  CHECK(std::string(bvh::BuildMeshBlas({{0, 0, 0}}, {0, 0, 9}, &blas)) == "no valid triangle", "no valid triangle");
  std::printf("%d trees identical to the GPU builder\n", trees);
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
