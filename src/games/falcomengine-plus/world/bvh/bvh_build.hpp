#pragma once

// Phase 1 M3a: GPU LBVH construction (correctness build).
//
// CPU computes triangle/instance AABBs, 30-bit Morton codes and sorts the
// leaves; the GPU builds the internal hierarchy per unique mesh (BLAS) and per
// active region instance set (TLAS). No ray traversal here: this stage only
// proves that a valid hierarchy can be constructed from the reconstructed
// world data, with structural validation and counters.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../../../../utils/log.hpp"
#include "bvh_pool.hpp"
#include "bvh_resources.hpp"

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
  uint32_t child_or_leaf = 0u;  // internal: left child; leaf: 0x80000000 | leaf index
  float bounds_max[3];
  uint32_t sibling_or_right = 0u;  // internal: right child
};

// Must match world_bvh_types.hlsli exactly.
static_assert(sizeof(BVHLeafGPU) == 48u, "BVHLeafGPU layout must match world_bvh_types.hlsli");
static_assert(sizeof(BVHNodeGPU) == 32u, "BVHNodeGPU layout must match world_bvh_types.hlsli");

inline constexpr uint32_t kBuildPushConstantCount = 8u;

struct BvhBuildInput {
  std::vector<WorldMesh> meshes;
  std::vector<WorldInstance> instances;
  std::vector<uint32_t> active;
  PoolRegion region;
  uint64_t revision = 0u;
};

inline BvhBuildInput SnapshotBvhBuildInput() {
  BvhBuildInput input;
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  input.meshes = g_pool.meshes;
  input.instances = g_pool.instances;
  input.region = CurrentPoolRegion();
  input.active = ComputeActiveIndices(input.instances, input.region);
  input.revision = g_pool.revision;
  return input;
}

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

inline bool BuildBlasLeaves(
    const BvhBuildInput& input,
    std::vector<BVHLeafGPU>* out_leaves,
    std::vector<WorldMeshGPU>* out_descriptors,
    uint32_t* out_degenerate) {
  if (out_leaves == nullptr || out_descriptors == nullptr) return false;
  out_leaves->clear();
  BuildMeshDescriptors(input.meshes, out_descriptors);
  uint32_t degenerate = 0u;
  uint64_t node_offset = 0u;

  std::vector<BVHLeafGPU> mesh_leaves;
  for (size_t mesh_index = 0; mesh_index < input.meshes.size(); ++mesh_index) {
    const WorldMesh& mesh = input.meshes[mesh_index];
    WorldMeshGPU& descriptor = (*out_descriptors)[mesh_index];

    float bounds_min[3] = {1e30f, 1e30f, 1e30f};
    float bounds_max[3] = {-1e30f, -1e30f, -1e30f};
    const size_t triangle_count = mesh.indices.size() / 3u;
    mesh_leaves.clear();
    mesh_leaves.reserve(triangle_count);
    for (size_t triangle = 0u; triangle < triangle_count; ++triangle) {
      const uint32_t i0 = mesh.indices[triangle * 3u + 0u];
      const uint32_t i1 = mesh.indices[triangle * 3u + 1u];
      const uint32_t i2 = mesh.indices[triangle * 3u + 2u];
      if (i0 >= mesh.positions.size() || i1 >= mesh.positions.size() || i2 >= mesh.positions.size()) continue;
      const auto& p0 = mesh.positions[i0];
      const auto& p1 = mesh.positions[i1];
      const auto& p2 = mesh.positions[i2];
      float tri_min[3];
      float tri_max[3];
      for (int k = 0; k < 3; ++k) {
        tri_min[k] = (std::min)({p0[k], p1[k], p2[k]});
        tri_max[k] = (std::max)({p0[k], p1[k], p2[k]});
        if (!std::isfinite(tri_min[k]) || !std::isfinite(tri_max[k])) {
          degenerate += 1u;
        }
        bounds_min[k] = (std::min)(bounds_min[k], tri_min[k]);
        bounds_max[k] = (std::max)(bounds_max[k], tri_max[k]);
      }
      mesh_leaves.push_back(MakeLeaf(tri_min, tri_max, static_cast<uint32_t>(triangle)));
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
    for (auto& leaf : mesh_leaves) {
      const float cx = (leaf.bounds_min[0] + leaf.bounds_max[0]) * 0.5f;
      const float cy = (leaf.bounds_min[1] + leaf.bounds_max[1]) * 0.5f;
      const float cz = (leaf.bounds_min[2] + leaf.bounds_max[2]) * 0.5f;
      leaf.code = Morton3D(
          (cx - bounds_min[0]) / extent[0],
          (cy - bounds_min[1]) / extent[1],
          (cz - bounds_min[2]) / extent[2]);
    }
    std::sort(mesh_leaves.begin(), mesh_leaves.end(), [](const BVHLeafGPU& a, const BVHLeafGPU& b) {
      if (a.code != b.code) return a.code < b.code;
      return a.prim < b.prim;
    });

    const uint32_t leaf_count = static_cast<uint32_t>(mesh_leaves.size());
    const uint32_t node_count = leaf_count == 0u ? 0u : (2u * leaf_count - 1u);
    descriptor.build[0] = static_cast<float>(node_offset);
    descriptor.build[1] = 0.f;
    descriptor.build[2] = static_cast<float>(out_leaves->size());
    descriptor.build[3] = static_cast<float>(leaf_count);
    out_leaves->insert(out_leaves->end(), mesh_leaves.begin(), mesh_leaves.end());
    node_offset += node_count;
  }
  if (out_degenerate != nullptr) *out_degenerate = degenerate;
  return true;
}

inline bool BuildTlasLeaves(
    const BvhBuildInput& input,
    std::vector<BVHLeafGPU>* out_leaves,
    uint32_t* out_degenerate) {
  if (out_leaves == nullptr) return false;
  out_leaves->clear();
  uint32_t degenerate = 0u;

  float bounds_min[3] = {1e30f, 1e30f, 1e30f};
  float bounds_max[3] = {-1e30f, -1e30f, -1e30f};
  for (const uint32_t index : input.active) {
    if (index >= input.instances.size()) continue;
    const WorldInstance& instance = input.instances[index];
    for (int k = 0; k < 3; ++k) {
      bounds_min[k] = (std::min)(bounds_min[k], instance.bounds_min[k]);
      bounds_max[k] = (std::max)(bounds_max[k], instance.bounds_max[k]);
    }
  }
  if (input.active.empty()) return true;
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
  out_leaves->reserve(input.active.size());
  for (uint32_t slot = 0u; slot < input.active.size(); ++slot) {
    const WorldInstance& instance = input.instances[input.active[slot]];
    BVHLeafGPU leaf = MakeLeaf(instance.bounds_min, instance.bounds_max, slot);
    const float cx = (leaf.bounds_min[0] + leaf.bounds_max[0]) * 0.5f;
    const float cy = (leaf.bounds_min[1] + leaf.bounds_max[1]) * 0.5f;
    const float cz = (leaf.bounds_min[2] + leaf.bounds_max[2]) * 0.5f;
    leaf.code = Morton3D(
        (cx - bounds_min[0]) / extent[0],
        (cy - bounds_min[1]) / extent[1],
        (cz - bounds_min[2]) / extent[2]);
    out_leaves->push_back(leaf);
  }
  std::sort(out_leaves->begin(), out_leaves->end(), [](const BVHLeafGPU& a, const BVHLeafGPU& b) {
    if (a.code != b.code) return a.code < b.code;
    return a.prim < b.prim;
  });
  if (out_degenerate != nullptr) *out_degenerate = degenerate;
  return true;
}

inline bool EnsureBuildPipelines(reshade::api::device* device, BvhDeviceData* data) {
  if (data == nullptr) return false;
  if (data->blas_build_pipeline.handle != 0u && data->tlas_build_pipeline.handle != 0u) return true;

#if defined(__world_bvh_build_blas_EMBED_FILE) && defined(__world_bvh_build_tlas_EMBED_FILE)
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;

  DR srv_range = {0, 0, 0, 2, DS::all_compute, 1, DT::shader_resource_view};
  DR uav_range = {0, 0, 0, 1, DS::all_compute, 1, DT::buffer_unordered_access_view};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = 13;
  push_range.dx_register_space = 0;
  push_range.count = kBuildPushConstantCount;
  push_range.visibility = DS::all_compute;
  P params[3] = {};
  params[0].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[0].descriptor_table.count = 1;
  params[0].descriptor_table.ranges = &srv_range;
  params[1].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[1].descriptor_table.count = 1;
  params[1].descriptor_table.ranges = &uav_range;
  params[2].type = reshade::api::pipeline_layout_param_type::push_constants;
  params[2].push_constants = push_range;
  if (data->build_layout.handle == 0u
      && !device->create_pipeline_layout(3, params, &data->build_layout)) {
    return false;
  }
  if (data->build_srv_table.handle == 0u
      && !device->allocate_descriptor_table(data->build_layout, 0, &data->build_srv_table)) {
    return false;
  }
  if (data->build_uav_table.handle == 0u
      && !device->allocate_descriptor_table(data->build_layout, 1, &data->build_uav_table)) {
    return false;
  }
  const auto make_shader = [](std::span<const uint8_t> code, reshade::api::shader_desc* out) {
    *out = {};
    out->code = code.data();
    out->code_size = code.size();
    out->entry_point = "main";
  };
  if (data->blas_build_pipeline.handle == 0u) {
    reshade::api::shader_desc shader = {};
    make_shader(__world_bvh_build_blas, &shader);
    reshade::api::pipeline_subobject subobject = {
        reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    if (!device->create_pipeline(data->build_layout, 1, &subobject, &data->blas_build_pipeline)) return false;
  }
  if (data->tlas_build_pipeline.handle == 0u) {
    reshade::api::shader_desc shader = {};
    make_shader(__world_bvh_build_tlas, &shader);
    reshade::api::pipeline_subobject subobject = {
        reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    if (!device->create_pipeline(data->build_layout, 1, &subobject, &data->tlas_build_pipeline)) return false;
  }
  return true;
#else
  return false;
#endif
}

inline void DispatchBvhBuild(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    BvhDeviceData* data,
    reshade::api::pipeline pipeline,
    reshade::api::resource_view srv0,
    reshade::api::resource_view srv1,
    reshade::api::resource_view uav,
    uint32_t count,
    uint32_t groups) {
  reshade::api::resource_view srvs[2] = {srv0, srv1};
  reshade::api::descriptor_table_update srv_update = {
      data->build_srv_table, 0, 0, 2, reshade::api::descriptor_type::shader_resource_view, srvs};
  device->update_descriptor_tables(1, &srv_update);
  reshade::api::resource_view uavs[1] = {uav};
  reshade::api::descriptor_table_update uav_update = {
      data->build_uav_table, 0, 0, 1, reshade::api::descriptor_type::buffer_unordered_access_view, uavs};
  device->update_descriptor_tables(1, &uav_update);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, pipeline);
  const reshade::api::descriptor_table tables[2] = {data->build_srv_table, data->build_uav_table};
  cmd_list->bind_descriptor_tables(
      reshade::api::shader_stage::all_compute, data->build_layout, 0, 2, tables);
  float constants[kBuildPushConstantCount] = {};
  constants[0] = *reinterpret_cast<const float*>(&count);
  cmd_list->push_constants(
      reshade::api::shader_stage::all_compute, data->build_layout, 2, 0, kBuildPushConstantCount, constants);
  cmd_list->dispatch(groups, 1u, 1u);
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
    if ((node.child_or_leaf & 0x80000000u) != 0u) {
      if ((node.child_or_leaf & 0x7FFFFFFFu) >= leaves.size()) result.bad_leaf += 1u;
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

inline bool ReadbackBytes(
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    reshade::api::resource resource,
    uint64_t offset,
    uint64_t size,
    std::vector<uint8_t>* out) {
  return renodx::utils::scene::ReadbackBuffer(device, queue, resource, offset, size, out);
}

inline bool ValidateTlas(reshade::api::device* device, reshade::api::command_queue* queue, BvhDeviceData* data) {
  if (data->tlas_node_buffer.handle == 0u || data->tlas_leaf_buffer.handle == 0u) return false;
  std::vector<uint8_t> node_bytes;
  std::vector<uint8_t> leaf_bytes;
  if (!ReadbackBytes(device, queue, data->tlas_node_buffer, 0u, static_cast<uint64_t>(data->tlas_node_count) * sizeof(BVHNodeGPU), &node_bytes)) return false;
  if (!ReadbackBytes(device, queue, data->tlas_leaf_buffer, 0u, static_cast<uint64_t>(data->tlas_leaf_count) * sizeof(BVHLeafGPU), &leaf_bytes)) return false;
  std::vector<BVHNodeGPU> nodes(data->tlas_node_count);
  std::vector<BVHLeafGPU> leaves(data->tlas_leaf_count);
  std::memcpy(nodes.data(), node_bytes.data(), node_bytes.size());
  std::memcpy(leaves.data(), leaf_bytes.data(), leaf_bytes.size());
  const BvhValidation validation = ValidateBvh(nodes, leaves);
  data->tlas_valid = validation.ok;
  return validation.ok;
}

inline bool ValidateBlasSamples(
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    BvhDeviceData* data,
    const std::vector<WorldMeshGPU>& descriptors,
    const std::vector<BVHLeafGPU>& leaves) {
  std::vector<size_t> order(descriptors.size());
  for (size_t i = 0u; i < order.size(); ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
    return descriptors[a].build[3] > descriptors[b].build[3];
  });
  bool all_ok = true;
  uint32_t checked = 0u;
  for (const size_t mesh_index : order) {
    if (checked >= 4u) break;
    const WorldMeshGPU& descriptor = descriptors[mesh_index];
    const uint32_t leaf_count = static_cast<uint32_t>(descriptor.build[3]);
    if (leaf_count == 0u) continue;
    const uint32_t node_count = 2u * leaf_count - 1u;
    const uint64_t node_offset = static_cast<uint64_t>(descriptor.build[0]);
    const uint64_t leaf_offset = static_cast<uint64_t>(descriptor.build[2]);
    std::vector<uint8_t> node_bytes;
    std::vector<uint8_t> leaf_bytes;
    if (!ReadbackBytes(device, queue, data->blas_node_buffer, node_offset * sizeof(BVHNodeGPU), static_cast<uint64_t>(node_count) * sizeof(BVHNodeGPU), &node_bytes)
        || !ReadbackBytes(device, queue, data->blas_leaf_buffer, leaf_offset * sizeof(BVHLeafGPU), static_cast<uint64_t>(leaf_count) * sizeof(BVHLeafGPU), &leaf_bytes)) {
      all_ok = false;
      break;
    }
    std::vector<BVHNodeGPU> nodes(node_count);
    std::vector<BVHLeafGPU> mesh_leaves(leaf_count);
    std::memcpy(nodes.data(), node_bytes.data(), node_bytes.size());
    std::memcpy(mesh_leaves.data(), leaf_bytes.data(), leaf_bytes.size());
    const BvhValidation validation = ValidateBvh(nodes, mesh_leaves);
    if (!validation.ok) all_ok = false;
    checked += 1u;
  }
  data->blas_valid = all_ok;
  return all_ok;
}

inline bool BuildWorldBvh(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return false;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr || !data->ready) return false;
  if (!EnsureBuildPipelines(device, data)) {
    data->build_status = "build pipelines unavailable";
    return false;
  }
  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) return false;

  const BvhBuildInput input = SnapshotBvhBuildInput();
  std::vector<BVHLeafGPU> blas_leaves;
  std::vector<WorldMeshGPU> descriptors;
  uint32_t degenerate = 0u;
  if (!BuildBlasLeaves(input, &blas_leaves, &descriptors, &degenerate)) return false;
  std::vector<BVHLeafGPU> tlas_leaves;
  uint32_t tlas_degenerate = 0u;
  BuildTlasLeaves(input, &tlas_leaves, &tlas_degenerate);
  data->degenerate_leaf_count = degenerate + tlas_degenerate;
  renodx::utils::log::i(
      "[world-bvh] build start: rev=", input.revision,
      " meshes=", descriptors.size(),
      " blas_leaves=", blas_leaves.size(),
      " tlas_leaves=", tlas_leaves.size());

  // BLAS buffers + refreshed mesh descriptors (now carrying build info).
  DestroyBuffer(device, &data->blas_leaf_srv, &data->blas_leaf_buffer);
  DestroyBuffer(device, &data->blas_node_uav, &data->blas_node_buffer);
  DestroyBuffer(device, &data->blas_node_srv, nullptr);
  std::string error;
  if (!blas_leaves.empty()) {
    const auto node_usage = reshade::api::resource_usage::unordered_access
                            | reshade::api::resource_usage::shader_resource
                            | reshade::api::resource_usage::copy_source;
    if (!CreatePoolBuffer(device, blas_leaves.data(), blas_leaves.size() * sizeof(BVHLeafGPU), sizeof(BVHLeafGPU), &data->blas_leaf_buffer, &data->blas_leaf_srv, reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source, &error)) {
      data->build_status = "blas leaf buffer creation failed: " + error;
      return false;
    }
    const uint32_t node_count = 2u * static_cast<uint32_t>(blas_leaves.size()) - 1u;
    if (!CreatePoolBuffer(device, nullptr, static_cast<uint64_t>(node_count) * sizeof(BVHNodeGPU), sizeof(BVHNodeGPU), &data->blas_node_buffer, &data->blas_node_uav, node_usage, &error)) {
      data->build_status = "blas node buffer creation failed: " + error;
      return false;
    }
    if (!CreateBufferSrvView(device, data->blas_node_buffer, sizeof(BVHNodeGPU), &data->blas_node_srv)) {
      data->build_status = "blas node srv creation failed";
      return false;
    }
    data->blas_leaf_count = static_cast<uint32_t>(blas_leaves.size());
    data->blas_node_count = node_count;
  } else {
    data->blas_leaf_count = 0u;
    data->blas_node_count = 0u;
  }
  DestroyBuffer(device, &data->mesh_srv, &data->mesh_buffer);
  if (!descriptors.empty()
      && !CreatePoolBuffer(device, descriptors.data(), descriptors.size() * sizeof(WorldMeshGPU), sizeof(WorldMeshGPU), &data->mesh_buffer, &data->mesh_srv, reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source, &error)) {
    data->build_status = "mesh descriptor refresh failed: " + error;
    return false;
  }

  // TLAS buffers.
  DestroyBuffer(device, &data->tlas_leaf_srv, &data->tlas_leaf_buffer);
  DestroyBuffer(device, &data->tlas_node_uav, &data->tlas_node_buffer);
  DestroyBuffer(device, &data->tlas_node_srv, nullptr);
  if (!tlas_leaves.empty()) {
    const auto node_usage = reshade::api::resource_usage::unordered_access
                            | reshade::api::resource_usage::shader_resource
                            | reshade::api::resource_usage::copy_source;
    if (!CreatePoolBuffer(device, tlas_leaves.data(), tlas_leaves.size() * sizeof(BVHLeafGPU), sizeof(BVHLeafGPU), &data->tlas_leaf_buffer, &data->tlas_leaf_srv, reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source, &error)) {
      data->build_status = "tlas leaf buffer creation failed: " + error;
      return false;
    }
    const uint32_t node_count = 2u * static_cast<uint32_t>(tlas_leaves.size()) - 1u;
    if (!CreatePoolBuffer(device, nullptr, static_cast<uint64_t>(node_count) * sizeof(BVHNodeGPU), sizeof(BVHNodeGPU), &data->tlas_node_buffer, &data->tlas_node_uav, node_usage, &error)) {
      data->build_status = "tlas node buffer creation failed: " + error;
      return false;
    }
    if (!CreateBufferSrvView(device, data->tlas_node_buffer, sizeof(BVHNodeGPU), &data->tlas_node_srv)) {
      data->build_status = "tlas node srv creation failed";
      return false;
    }
    data->tlas_leaf_count = static_cast<uint32_t>(tlas_leaves.size());
    data->tlas_node_count = node_count;
  } else {
    data->tlas_leaf_count = 0u;
    data->tlas_node_count = 0u;
  }

  // Dispatch the builds.
  renodx::utils::log::i(
      "[world-bvh] buffers ready: blas_nodes=", data->blas_node_count,
      " tlas_nodes=", data->tlas_node_count);
  if (data->blas_node_count != 0u) {
    DispatchBvhBuild(
        device, cmd_list, data, data->blas_build_pipeline,
        data->mesh_srv, data->blas_leaf_srv, data->blas_node_uav,
        static_cast<uint32_t>(descriptors.size()),
        (static_cast<uint32_t>(descriptors.size()) + 63u) / 64u);
  }
  if (data->tlas_node_count != 0u) {
    DispatchBvhBuild(
        device, cmd_list, data, data->tlas_build_pipeline,
        data->tlas_leaf_srv, data->blas_leaf_srv, data->tlas_node_uav,
        data->tlas_leaf_count, 1u);
  }
  if (data->blas_node_buffer.handle != 0u) {
    cmd_list->barrier(data->blas_node_buffer, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  if (data->tlas_node_buffer.handle != 0u) {
    cmd_list->barrier(data->tlas_node_buffer, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
  }
  queue->flush_immediate_command_list();
  renodx::utils::log::i("[world-bvh] dispatched, validating");

  // Structural validation.
  const bool tlas_ok = data->tlas_node_count != 0u ? ValidateTlas(device, queue, data) : true;
  const bool blas_ok = ValidateBlasSamples(device, queue, data, descriptors, blas_leaves);
  renodx::utils::log::i(
      "[world-bvh] validation: blas=", blas_ok ? "true" : "false",
      " tlas=", tlas_ok ? "true" : "false");
  data->bvh_ready = true;
  data->bvh_built_revision = input.revision;
  data->tlas_built_revision = input.revision;
  data->build_status = "blas " + std::to_string(data->blas_leaf_count) + " leaves/" + std::to_string(data->blas_node_count)
                       + " nodes, tlas " + std::to_string(data->tlas_leaf_count) + "/" + std::to_string(data->tlas_node_count)
                       + ", blas_valid=" + (blas_ok ? "true" : "false")
                       + ", tlas_valid=" + (tlas_ok ? "true" : "false")
                       + ", degenerate=" + std::to_string(data->degenerate_leaf_count);
  return blas_ok && tlas_ok;
}

inline bool BuildWorldTlas(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return false;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr || !data->ready || !data->bvh_ready) return false;
  if (!EnsureBuildPipelines(device, data)) return false;
  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) return false;

  const BvhBuildInput input = SnapshotBvhBuildInput();
  std::vector<BVHLeafGPU> tlas_leaves;
  uint32_t degenerate = 0u;
  BuildTlasLeaves(input, &tlas_leaves, &degenerate);

  DestroyBuffer(device, &data->tlas_leaf_srv, &data->tlas_leaf_buffer);
  DestroyBuffer(device, &data->tlas_node_uav, &data->tlas_node_buffer);
  DestroyBuffer(device, &data->tlas_node_srv, nullptr);
  if (!tlas_leaves.empty()) {
    const auto node_usage = reshade::api::resource_usage::unordered_access
                            | reshade::api::resource_usage::shader_resource
                            | reshade::api::resource_usage::copy_source;
    std::string error;
    if (!CreatePoolBuffer(device, tlas_leaves.data(), tlas_leaves.size() * sizeof(BVHLeafGPU), sizeof(BVHLeafGPU), &data->tlas_leaf_buffer, &data->tlas_leaf_srv, reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source, &error)) {
      data->build_status = "tlas leaf buffer creation failed: " + error;
      return false;
    }
    const uint32_t node_count = 2u * static_cast<uint32_t>(tlas_leaves.size()) - 1u;
    if (!CreatePoolBuffer(device, nullptr, static_cast<uint64_t>(node_count) * sizeof(BVHNodeGPU), sizeof(BVHNodeGPU), &data->tlas_node_buffer, &data->tlas_node_uav, node_usage, &error)) {
      data->build_status = "tlas node buffer creation failed: " + error;
      return false;
    }
    if (!CreateBufferSrvView(device, data->tlas_node_buffer, sizeof(BVHNodeGPU), &data->tlas_node_srv)) {
      data->build_status = "tlas node srv creation failed";
      return false;
    }
    data->tlas_leaf_count = static_cast<uint32_t>(tlas_leaves.size());
    data->tlas_node_count = node_count;
    DispatchBvhBuild(
        device, cmd_list, data, data->tlas_build_pipeline,
        data->tlas_leaf_srv, data->blas_leaf_srv, data->tlas_node_uav,
        data->tlas_leaf_count, 1u);
    cmd_list->barrier(data->tlas_node_buffer, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::shader_resource);
    queue->flush_immediate_command_list();
    const bool tlas_ok = ValidateTlas(device, queue, data);
    renodx::utils::log::i(
        "[world-bvh] tlas rebuild: leaves=", data->tlas_leaf_count,
        " nodes=", data->tlas_node_count,
        " valid=", tlas_ok ? "true" : "false");
    data->tlas_built_revision = input.revision;
    data->build_status = "tlas " + std::to_string(data->tlas_leaf_count) + "/" + std::to_string(data->tlas_node_count)
                         + ", tlas_valid=" + (tlas_ok ? "true" : "false")
                         + ", degenerate=" + std::to_string(data->degenerate_leaf_count + degenerate);
    return tlas_ok;
  }
  data->tlas_leaf_count = 0u;
  data->tlas_node_count = 0u;
  data->tlas_built_revision = input.revision;
  data->build_status = "tlas empty";
  return true;
}

}  // namespace falcom_world::bvh
