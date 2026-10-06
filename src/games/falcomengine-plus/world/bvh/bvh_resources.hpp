#pragma once

// Phase 1 M2: GPU world pool.
//
// Uploads the validated M1 CPU pool (meshes, instances, region active list)
// into GPU buffers, verifies the upload once per pool revision with a
// readback checksum, and owns the resources shared by the BLAS/TLAS build and
// the trace debug views.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <d3d11.h>
#endif

#include "../../../../utils/data.hpp"
#include "../../../../utils/log.hpp"
#include "../../../../utils/scene.hpp"
#include "bvh_pool.hpp"

namespace falcom_world::bvh {

struct WorldMeshGPU {
  float header[4];  // x=vertex_offset y=vertex_count z=index_offset w=index_count
  float bbox_min[4];
  float bbox_max[4];
  float build[4];  // x=blas_node_offset y=blas_root z=leaf_offset w=leaf_count
};

struct WorldInstanceGPU {
  float header[4];  // x=mesh_id y=source
  float world[16];
  float inverse_world[16];
  float bounds_min[4];
  float bounds_max[4];
};

// Must match world_bvh_types.hlsli exactly.
static_assert(sizeof(WorldMeshGPU) == 64u, "WorldMeshGPU layout must match world_bvh_types.hlsli");
static_assert(sizeof(WorldInstanceGPU) == 176u, "WorldInstanceGPU layout must match world_bvh_types.hlsli");

struct BvhTraceStats {
  uint32_t rays = 0u;
  uint32_t hits = 0u;
  uint32_t misses = 0u;
  uint32_t invalid_refs = 0u;
  uint32_t stack_overflow = 0u;
  uint32_t triangle_tests = 0u;
  uint32_t max_stack_depth = 0u;
  // Depth Compare pixel classes (zero in the other views).
  uint32_t compare_match = 0u;
  uint32_t compare_missing = 0u;
  uint32_t compare_extra = 0u;
  uint32_t compare_extra_sky = 0u;
  uint32_t compare_far = 0u;
  uint32_t compare_sky = 0u;
  uint32_t compare_no_depth = 0u;
  bool valid = false;
  bool invariant_ok = false;
};

struct __declspec(uuid("b7a1c2d3-4e5f-4a6b-8c7d-9e0f1a2b3c4d")) BvhDeviceData {
  bool ready = false;
  uint64_t uploaded_revision = 0u;
  uint64_t region_revision = 0u;
  uint32_t active_count = 0u;
  uint32_t vertex_count = 0u;
  uint32_t index_count = 0u;
  uint32_t mesh_count = 0u;
  uint32_t instance_count = 0u;
  uint32_t max_mesh_vertices = 0u;
  float region_min[3] = {1e30f, 1e30f, 1e30f};

  reshade::api::resource vertex_buffer = {0u};
  reshade::api::resource_view vertex_srv = {0u};
  reshade::api::resource index_buffer = {0u};
  reshade::api::resource_view index_srv = {0u};
  reshade::api::resource mesh_buffer = {0u};
  reshade::api::resource_view mesh_srv = {0u};
  reshade::api::resource instance_buffer = {0u};
  reshade::api::resource_view instance_srv = {0u};
  reshade::api::resource active_buffer = {0u};
  reshade::api::resource_view active_srv = {0u};

  bool checksum_valid = false;
  bool checksum_match = false;
  uint64_t cpu_hash = 0u;
  uint64_t gpu_hash = 0u;

  reshade::api::resource debug_texture = {0u};
  reshade::api::resource_view debug_srv = {0u};
  reshade::api::resource_view debug_uav = {0u};
  uint32_t debug_width = 0u;
  uint32_t debug_height = 0u;

  reshade::api::resource blas_leaf_buffer = {0u};
  reshade::api::resource_view blas_leaf_srv = {0u};
  reshade::api::resource blas_node_buffer = {0u};
  reshade::api::resource_view blas_node_uav = {0u};
  reshade::api::resource_view blas_node_srv = {0u};
  reshade::api::resource tlas_leaf_buffer = {0u};
  reshade::api::resource_view tlas_leaf_srv = {0u};
  reshade::api::resource tlas_node_buffer = {0u};
  reshade::api::resource_view tlas_node_uav = {0u};
  reshade::api::resource_view tlas_node_srv = {0u};
  reshade::api::pipeline_layout build_layout = {0u};
  reshade::api::descriptor_table build_srv_table = {0u};
  reshade::api::descriptor_table build_uav_table = {0u};
  reshade::api::pipeline blas_build_pipeline = {0u};
  reshade::api::pipeline tlas_build_pipeline = {0u};

  reshade::api::pipeline_layout trace_layout = {0u};
  reshade::api::descriptor_table trace_srv_table = {0u};
  reshade::api::descriptor_table trace_uav_table = {0u};
  reshade::api::pipeline trace_pipeline = {0u};
  reshade::api::resource trace_stats_buffer = {0u};
  reshade::api::resource_view trace_stats_uav = {0u};
  BvhTraceStats trace_stats;
  bool trace_ready = false;

  bool bvh_ready = false;
  uint64_t bvh_built_revision = 0u;
  uint64_t tlas_built_revision = 0u;
  uint32_t blas_leaf_count = 0u;
  uint32_t blas_node_count = 0u;
  uint32_t tlas_leaf_count = 0u;
  uint32_t tlas_node_count = 0u;
  uint32_t degenerate_leaf_count = 0u;
  bool blas_valid = false;
  bool tlas_valid = false;
  std::string build_status = "not built";
};

inline uint64_t HashPoolBytes(uint64_t hash, const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    hash ^= data[i];
    hash *= 1099511628211ull;
  }
  return hash;
}

inline BvhDeviceData* GetBvhDeviceData(reshade::api::device* device) {
  if (device == nullptr) return nullptr;
  BvhDeviceData* data = nullptr;
  renodx::utils::data::CreateOrGet<BvhDeviceData>(device, data);
  return data;
}

inline void DestroyBuffer(reshade::api::device* device, reshade::api::resource_view* view, reshade::api::resource* resource) {
  if (view != nullptr && view->handle != 0u) {
    device->destroy_resource_view(*view);
    *view = {0u};
  }
  if (resource != nullptr && resource->handle != 0u) {
    device->destroy_resource(*resource);
    *resource = {0u};
  }
}

inline void DestroyBvhDeviceData(reshade::api::device* device) {
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return;
  DestroyBuffer(device, &data->vertex_srv, &data->vertex_buffer);
  DestroyBuffer(device, &data->index_srv, &data->index_buffer);
  DestroyBuffer(device, &data->mesh_srv, &data->mesh_buffer);
  DestroyBuffer(device, &data->instance_srv, &data->instance_buffer);
  DestroyBuffer(device, &data->active_srv, &data->active_buffer);
  DestroyBuffer(device, &data->blas_leaf_srv, &data->blas_leaf_buffer);
  DestroyBuffer(device, &data->blas_node_uav, &data->blas_node_buffer);
  DestroyBuffer(device, &data->blas_node_srv, nullptr);
  DestroyBuffer(device, &data->tlas_leaf_srv, &data->tlas_leaf_buffer);
  DestroyBuffer(device, &data->tlas_node_uav, &data->tlas_node_buffer);
  DestroyBuffer(device, &data->tlas_node_srv, nullptr);
  if (data->blas_build_pipeline.handle != 0u) {
    device->destroy_pipeline(data->blas_build_pipeline);
    data->blas_build_pipeline = {0u};
  }
  if (data->tlas_build_pipeline.handle != 0u) {
    device->destroy_pipeline(data->tlas_build_pipeline);
    data->tlas_build_pipeline = {0u};
  }
  if (data->build_srv_table.handle != 0u) {
    device->free_descriptor_table(data->build_srv_table);
    data->build_srv_table = {0u};
  }
  if (data->build_uav_table.handle != 0u) {
    device->free_descriptor_table(data->build_uav_table);
    data->build_uav_table = {0u};
  }
  if (data->build_layout.handle != 0u) {
    device->destroy_pipeline_layout(data->build_layout);
    data->build_layout = {0u};
  }
  if (data->trace_pipeline.handle != 0u) {
    device->destroy_pipeline(data->trace_pipeline);
    data->trace_pipeline = {0u};
  }
  if (data->trace_srv_table.handle != 0u) {
    device->free_descriptor_table(data->trace_srv_table);
    data->trace_srv_table = {0u};
  }
  if (data->trace_uav_table.handle != 0u) {
    device->free_descriptor_table(data->trace_uav_table);
    data->trace_uav_table = {0u};
  }
  if (data->trace_layout.handle != 0u) {
    device->destroy_pipeline_layout(data->trace_layout);
    data->trace_layout = {0u};
  }
  DestroyBuffer(device, &data->trace_stats_uav, &data->trace_stats_buffer);
  data->trace_stats = {};
  data->trace_ready = false;
  data->bvh_ready = false;
  data->blas_valid = false;
  data->tlas_valid = false;
  data->build_status = "not built";
  data->ready = false;
  data->checksum_valid = false;
  data->uploaded_revision = 0u;
  data->region_revision = 0u;
  data->active_count = 0u;
}

// Diagnostic-only: reproduce the same D3D11 buffer creation on the native
// device to recover the HRESULT when the ReShade path fails. The probe always
// passes null initial data, so a match with the ReShade result is expected
// once the caller's initial-data handling is correct.
inline uint32_t ProbeBufferCreateHr(reshade::api::device* device, const reshade::api::resource_desc& desc) {
#if defined(_WIN32)
  if (device == nullptr || desc.type != reshade::api::resource_type::buffer) return 0u;
  auto* native = reinterpret_cast<ID3D11Device*>(static_cast<uintptr_t>(device->get_native()));  // NOLINT(performance-no-int-to-ptr)
  if (native == nullptr) return 0u;
  D3D11_BUFFER_DESC native_desc = {};
  native_desc.ByteWidth = static_cast<UINT>(desc.buffer.size);
  native_desc.Usage = D3D11_USAGE_DEFAULT;
  native_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  if ((desc.usage & reshade::api::resource_usage::unordered_access) != 0u) {
    native_desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
  }
  if (desc.buffer.stride != 0u) {
    native_desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    native_desc.StructureByteStride = desc.buffer.stride;
  }
  ID3D11Buffer* probe = nullptr;
  const HRESULT hr = native->CreateBuffer(&native_desc, nullptr, &probe);
  if (probe != nullptr) probe->Release();
  return static_cast<uint32_t>(hr);
#else
  (void)device;
  (void)desc;
  return 0u;
#endif
}

inline std::string DescribePoolBufferFailure(
    reshade::api::device* device,
    const reshade::api::resource_desc& desc) {
  char hr_text[16] = {};
  std::snprintf(hr_text, sizeof(hr_text), "0x%08lX", static_cast<unsigned long>(ProbeBufferCreateHr(device, desc)));
  char size_text[32] = {};
  std::snprintf(
      size_text,
      sizeof(size_text),
      "%.2f MB",
      static_cast<double>(desc.buffer.size) / (1024.0 * 1024.0));
  return "type=buffer elems=" + std::to_string(desc.buffer.size / desc.buffer.stride)
         + " stride=" + std::to_string(desc.buffer.stride)
         + " bytes=" + std::to_string(desc.buffer.size)
         + " (" + size_text + ")"
         + " hr=" + hr_text;
}

inline bool CreatePoolBuffer(
    reshade::api::device* device,
    const void* bytes,
    uint64_t size,
    uint32_t element_size,
    reshade::api::resource* out_resource,
    reshade::api::resource_view* out_view,
    reshade::api::resource_usage usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source,
    std::string* out_error = nullptr) {
  if (device == nullptr || size == 0u || element_size == 0u) return false;
  reshade::api::resource_desc desc = {};
  desc.type = reshade::api::resource_type::buffer;
  desc.buffer.size = size;
  desc.buffer.stride = element_size;
  desc.heap = reshade::api::memory_heap::gpu_only;
  desc.usage = usage;
  // The view type must match how the buffer is bound: a UAV resource needs an
  // unordered_access view, never a shader_resource view bound as a UAV.
  const reshade::api::resource_usage view_usage =
      (usage & reshade::api::resource_usage::unordered_access) != 0u
          ? reshade::api::resource_usage::unordered_access
          : reshade::api::resource_usage::shader_resource;
  reshade::api::subresource_data initial = {};
  initial.data = const_cast<void*>(bytes);
  reshade::api::resource resource = {0u};
  if (!device->create_resource(desc, bytes != nullptr ? &initial : nullptr, view_usage, &resource)) {
    if (out_error != nullptr) *out_error = DescribePoolBufferFailure(device, desc);
    return false;
  }
  const uint64_t element_count = size / element_size;
  reshade::api::resource_view view = {0u};
  if (!device->create_resource_view(
          resource, view_usage,
          reshade::api::resource_view_desc(
              reshade::api::resource_view_type::buffer, reshade::api::format::unknown, 0, element_count),
          &view)) {
    device->destroy_resource(resource);
    if (out_error != nullptr) {
      *out_error = DescribePoolBufferFailure(device, desc) + " (resource created, view failed)";
    }
    return false;
  }
  *out_resource = resource;
  *out_view = view;
  return true;
}

// Creates an additional read-only view for a buffer that already exists (for
// example the node buffers, which are built through their UAV view and read by
// the trace pass through an SRV).
inline bool CreateBufferSrvView(
    reshade::api::device* device,
    reshade::api::resource resource,
    uint32_t element_size,
    reshade::api::resource_view* out_view) {
  if (device == nullptr || resource.handle == 0u || element_size == 0u || out_view == nullptr) return false;
  const auto desc = device->get_resource_desc(resource);
  if (desc.type != reshade::api::resource_type::buffer || desc.buffer.size == 0u) return false;
  const uint64_t element_count = desc.buffer.size / element_size;
  return device->create_resource_view(
      resource, reshade::api::resource_usage::shader_resource,
      reshade::api::resource_view_desc(
          reshade::api::resource_view_type::buffer, reshade::api::format::unknown, 0, element_count),
      out_view);
}

inline std::vector<uint32_t> ComputeActiveIndices(
    const std::vector<WorldInstance>& instances,
    const PoolRegion& region) {
  std::vector<uint32_t> active;
  active.reserve(instances.size());
  for (uint32_t i = 0; i < instances.size(); ++i) {
    if (PoolInstanceInRegion(instances[i], region)) active.push_back(i);
  }
  return active;
}

// Single source of truth for mesh descriptor construction: the vertex/index
// offsets must always match the flat pool buffers built by UploadWorldPoolToGpu
// (header.x is a float4 vertex offset, header.z an index offset).
inline void BuildMeshDescriptors(
    const std::vector<WorldMesh>& meshes,
    std::vector<WorldMeshGPU>* out_descriptors) {
  if (out_descriptors == nullptr) return;
  out_descriptors->assign(meshes.size(), WorldMeshGPU{});
  uint64_t vertex_offset = 0u;
  uint64_t index_offset = 0u;
  for (size_t i = 0; i < meshes.size(); ++i) {
    const WorldMesh& mesh = meshes[i];
    WorldMeshGPU& descriptor = (*out_descriptors)[i];
    descriptor.header[0] = static_cast<float>(vertex_offset);
    descriptor.header[1] = static_cast<float>(mesh.positions.size());
    descriptor.header[2] = static_cast<float>(index_offset);
    descriptor.header[3] = static_cast<float>(mesh.indices.size());
    for (int k = 0; k < 3; ++k) {
      descriptor.bbox_min[k] = mesh.bbox_min[k];
      descriptor.bbox_max[k] = mesh.bbox_max[k];
    }
    descriptor.bbox_min[3] = 0.f;
    descriptor.bbox_max[3] = 0.f;
    vertex_offset += mesh.positions.size();
    index_offset += mesh.indices.size();
  }
}

// The pool matrix is stored in the game's row-dot layout: world.x = dot(p, row0)
// with row0 = (m0, m1, m2, m3), and the engine only ever uses rows 0..2. The
// stored fourth row is not part of the transform (Inst4x4 captures often leave
// it zero or stale), so the canonical matrix is the affine 4x4 with last row
// (0,0,0,1); invert that with Gauss-Jordan so scale/shear stay safe.
inline void ComputeMatrixInverse(const float* matrix, float* out) {
  float m[4][4] = {};
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 4; ++col) {
      m[row][col] = matrix[row * 4 + col];
    }
  }
  m[3][0] = 0.f;
  m[3][1] = 0.f;
  m[3][2] = 0.f;
  m[3][3] = 1.f;
  float inv[4][4] = {};
  for (int i = 0; i < 4; ++i) inv[i][i] = 1.f;
  bool singular = false;
  for (int col = 0; col < 4 && !singular; ++col) {
    int pivot = col;
    for (int row = col + 1; row < 4; ++row) {
      if (std::fabs(m[row][col]) > std::fabs(m[pivot][col])) pivot = row;
    }
    if (std::fabs(m[pivot][col]) < 1e-12f) {
      singular = true;
      break;
    }
    if (pivot != col) {
      for (int k = 0; k < 4; ++k) {
        std::swap(m[col][k], m[pivot][k]);
        std::swap(inv[col][k], inv[pivot][k]);
      }
    }
    const float scale = 1.f / m[col][col];
    for (int k = 0; k < 4; ++k) {
      m[col][k] *= scale;
      inv[col][k] *= scale;
    }
    for (int row = 0; row < 4; ++row) {
      if (row == col) continue;
      const float factor = m[row][col];
      if (factor == 0.f) continue;
      for (int k = 0; k < 4; ++k) {
        m[row][k] -= factor * m[col][k];
        inv[row][k] -= factor * inv[col][k];
      }
    }
  }
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      out[row * 4 + col] = singular ? ((row == col) ? 1.f : 0.f) : inv[row][col];
    }
  }
}

inline bool UploadWorldPoolToGpu(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return false;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return false;

  std::vector<WorldMesh> meshes;
  std::vector<WorldInstance> instances;
  std::vector<uint32_t> active;
  PoolRegion region;
  uint64_t revision = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    meshes = g_pool.meshes;
    instances = g_pool.instances;
    region = CurrentPoolRegion();
    active = ComputeActiveIndices(instances, region);
    revision = g_pool.revision;
  }
  if (meshes.empty() || instances.empty()) {
    // An emptied pool (Reset, or every mesh retired after a map change) must
    // not leave the previous upload on screen or in the BVH.
    data->ready = false;
    data->bvh_ready = false;
    data->uploaded_revision = revision;
    data->build_status = "pool empty";
    return false;
  }

  std::vector<float> vertices;
  std::vector<uint32_t> indices;
  std::vector<WorldMeshGPU> mesh_descriptors;
  std::vector<WorldInstanceGPU> instance_descriptors(instances.size());
  vertices.reserve(1024u * 1024u);
  indices.reserve(1024u * 1024u);
  BuildMeshDescriptors(meshes, &mesh_descriptors);
  for (const auto& mesh : meshes) {
    for (const auto& position : mesh.positions) {
      vertices.push_back(position[0]);
      vertices.push_back(position[1]);
      vertices.push_back(position[2]);
      vertices.push_back(1.f);
    }
    indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());
  }
  for (size_t i = 0; i < instances.size(); ++i) {
    const WorldInstance& instance = instances[i];
    WorldInstanceGPU& descriptor = instance_descriptors[i];
    descriptor.header[0] = static_cast<float>(instance.mesh_id);
    descriptor.header[1] = static_cast<float>(instance.source);
    const uint32_t floats = instance.matrix_floats == 0u ? 12u : instance.matrix_floats;
    float matrix[16] = {};
    std::memcpy(matrix, instance.matrix, sizeof(float) * floats);
    // The engine's vertex transform uses only rows 0..2; make the descriptor
    // explicitly affine so it always matches the computed inverse.
    matrix[12] = 0.f;
    matrix[13] = 0.f;
    matrix[14] = 0.f;
    matrix[15] = 1.f;
    float inverse[16] = {};
    ComputeMatrixInverse(instance.matrix, inverse);
    std::memcpy(descriptor.world, matrix, sizeof(float) * 16u);
    std::memcpy(descriptor.inverse_world, inverse, sizeof(float) * 16u);
    for (int k = 0; k < 3; ++k) {
      descriptor.bounds_min[k] = instance.bounds_min[k];
      descriptor.bounds_max[k] = instance.bounds_max[k];
    }
    descriptor.bounds_min[3] = 0.f;
    descriptor.bounds_max[3] = 0.f;
  }

  uint64_t cpu_hash = 1469598103934665603ull;
  cpu_hash = HashPoolBytes(cpu_hash, reinterpret_cast<const uint8_t*>(vertices.data()), vertices.size() * sizeof(float));
  cpu_hash = HashPoolBytes(cpu_hash, reinterpret_cast<const uint8_t*>(indices.data()), indices.size() * sizeof(uint32_t));
  cpu_hash = HashPoolBytes(cpu_hash, reinterpret_cast<const uint8_t*>(mesh_descriptors.data()), mesh_descriptors.size() * sizeof(WorldMeshGPU));
  cpu_hash = HashPoolBytes(cpu_hash, reinterpret_cast<const uint8_t*>(instance_descriptors.data()), instance_descriptors.size() * sizeof(WorldInstanceGPU));
  cpu_hash = HashPoolBytes(cpu_hash, reinterpret_cast<const uint8_t*>(active.data()), active.size() * sizeof(uint32_t));

  DestroyBuffer(device, &data->vertex_srv, &data->vertex_buffer);
  DestroyBuffer(device, &data->index_srv, &data->index_buffer);
  DestroyBuffer(device, &data->mesh_srv, &data->mesh_buffer);
  DestroyBuffer(device, &data->instance_srv, &data->instance_buffer);
  DestroyBuffer(device, &data->active_srv, &data->active_buffer);

  if (!CreatePoolBuffer(device, vertices.data(), vertices.size() * sizeof(float), 16u, &data->vertex_buffer, &data->vertex_srv)) return false;
  if (!CreatePoolBuffer(device, indices.data(), indices.size() * sizeof(uint32_t), 4u, &data->index_buffer, &data->index_srv)) return false;
  if (!CreatePoolBuffer(device, mesh_descriptors.data(), mesh_descriptors.size() * sizeof(WorldMeshGPU), sizeof(WorldMeshGPU), &data->mesh_buffer, &data->mesh_srv)) return false;
  if (!CreatePoolBuffer(device, instance_descriptors.data(), instance_descriptors.size() * sizeof(WorldInstanceGPU), sizeof(WorldInstanceGPU), &data->instance_buffer, &data->instance_srv)) return false;
  if (!CreatePoolBuffer(device, active.data(), active.size() * sizeof(uint32_t), 4u, &data->active_buffer, &data->active_srv)) return false;

  data->vertex_count = static_cast<uint32_t>(vertices.size() / 4u);
  data->index_count = static_cast<uint32_t>(indices.size());
  data->mesh_count = static_cast<uint32_t>(mesh_descriptors.size());
  data->instance_count = static_cast<uint32_t>(instance_descriptors.size());
  data->active_count = static_cast<uint32_t>(active.size());
  data->uploaded_revision = revision;
  data->region_revision = revision;
  data->region_min[0] = region.min[0];
  data->region_min[1] = region.min[1];
  data->region_min[2] = region.min[2];
  data->max_mesh_vertices = 1u;
  for (const auto& mesh : meshes) {
    data->max_mesh_vertices = (std::max)(data->max_mesh_vertices, static_cast<uint32_t>(mesh.positions.size()));
  }
  data->ready = true;

  // One-time upload checksum per revision.
  uint64_t gpu_hash = 1469598103934665603ull;
  bool checksum_ok = true;
  const auto hash_resource = [&](reshade::api::resource resource, uint64_t size) {
    std::vector<uint8_t> bytes;
    if (!renodx::utils::scene::ReadbackBuffer(device, queue, resource, 0u, size, &bytes) || bytes.size() != size) {
      checksum_ok = false;
      return;
    }
    gpu_hash = HashPoolBytes(gpu_hash, bytes.data(), bytes.size());
  };
  hash_resource(data->vertex_buffer, vertices.size() * sizeof(float));
  hash_resource(data->index_buffer, indices.size() * sizeof(uint32_t));
  hash_resource(data->mesh_buffer, mesh_descriptors.size() * sizeof(WorldMeshGPU));
  hash_resource(data->instance_buffer, instance_descriptors.size() * sizeof(WorldInstanceGPU));
  hash_resource(data->active_buffer, active.size() * sizeof(uint32_t));
  data->cpu_hash = cpu_hash;
  data->gpu_hash = gpu_hash;
  data->checksum_valid = checksum_ok;
  data->checksum_match = checksum_ok && cpu_hash == gpu_hash;

  const char* checksum_text = !checksum_ok ? "unavailable" : (data->checksum_match ? "match" : "MISMATCH");
  renodx::utils::log::i(
      "[world-bvh] upload: rev=", revision,
      " meshes=", data->mesh_count,
      " vertices=", data->vertex_count,
      " indices=", data->index_count,
      " instances=", data->instance_count,
      " active=", data->active_count,
      " checksum=", checksum_text);
  if (checksum_ok && !data->checksum_match) {
    renodx::utils::log::w(
        "[world-bvh] upload checksum mismatch: cpu=", renodx::utils::log::AsHex(data->cpu_hash),
        " gpu=", renodx::utils::log::AsHex(data->gpu_hash));
  }

  // Diagnostic-only: bound-check every mesh descriptor against the global
  // vertex/index buffers (the float offsets are exact below 2^24).
  uint32_t bound_violations = 0u;
  for (const WorldMeshGPU& descriptor : mesh_descriptors) {
    const uint64_t vertex_offset = static_cast<uint64_t>(descriptor.header[0]);
    const uint64_t vertex_count = static_cast<uint64_t>(descriptor.header[1]);
    const uint64_t index_offset = static_cast<uint64_t>(descriptor.header[2]);
    const uint64_t index_count = static_cast<uint64_t>(descriptor.header[3]);
    if (vertex_offset + vertex_count > data->vertex_count || index_offset + index_count > data->index_count) {
      bound_violations += 1u;
    }
  }
  renodx::utils::log::i(
      "[world-bvh] mesh bounds: violations=", bound_violations,
      " meshes=", mesh_descriptors.size());
  return true;
}

inline bool UploadActiveInstancesToGpu(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return false;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr || !data->ready) return false;
  std::vector<WorldInstance> instances;
  std::vector<uint32_t> active;
  PoolRegion region;
  uint64_t revision = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    instances = g_pool.instances;
    region = CurrentPoolRegion();
    active = ComputeActiveIndices(instances, region);
    revision = g_pool.revision;
  }

  DestroyBuffer(device, &data->active_srv, &data->active_buffer);
  if (!active.empty()
      && !CreatePoolBuffer(device, active.data(), active.size() * sizeof(uint32_t), 4u, &data->active_buffer, &data->active_srv)) {
    return false;
  }
  data->active_count = static_cast<uint32_t>(active.size());
  data->region_revision = revision;
  data->region_min[0] = region.min[0];
  data->region_min[1] = region.min[1];
  data->region_min[2] = region.min[2];
  return true;
}

}  // namespace falcom_world::bvh
