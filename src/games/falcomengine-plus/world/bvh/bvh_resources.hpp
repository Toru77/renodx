#pragma once

// GPU resources of the world BVH.
//
// Layout structs shared with the shaders, the per-device data that the live
// BVH (bvh_live.hpp) maintains and the trace pass (bvh_trace.hpp) reads, and
// buffer helpers. The live store keeps every captured mesh in append-only GPU
// arenas (vertices, indices, BLAS leaves, BLAS nodes); the trace binds those
// arenas plus exact-size buffers rebuilt from the store and from each TLAS
// snapshot (mesh descriptors, region instances, active list, TLAS leaves and
// nodes). Shader register order: world_bvh_trace.hlsli t0..t8.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined(_WIN32)
#include <d3d11.h>
#endif

#include "../../../../utils/data.hpp"
#include "../../../../utils/log.hpp"
#include "../../../../utils/scene.hpp"
#include "bvh_build.hpp"
#include "alpha_atlas.hpp"
#include "bvh_pool.hpp"
#include "camera_fade.hpp"
#include "gpu_timer.hpp"

namespace falcom_world::bvh {

struct WorldMeshGPU {
  float header[4];  // x=vertex_offset y=vertex_count z=index_offset w=index_count
  float bbox_min[4];
  float bbox_max[4];
  float build[4];  // x=blas_node_offset y=blas_root z=leaf_offset w=leaf_count
};

// visibility: x = near-fade start (m), y = near-fade 1/range (camera_fade.hpp;
// both 0 unless kInstanceNearFade), z = kInstance* flags as a float, w = 0.
inline constexpr uint32_t kInstanceNearFade = 1u;       // camera rays apply the near fade
inline constexpr uint32_t kInstanceCameraVisible = 2u;  // drawn by a camera VS
inline constexpr uint32_t kInstanceShadowCaster = 4u;   // drawn by a light VS (shadow maps)

struct WorldInstanceGPU {
  float header[4];  // x=mesh_id (live store slot) y=source
  float world[16];
  float inverse_world[16];
  float bounds_min[4];
  float bounds_max[4];
  float visibility[4];
};

// Must match world_bvh_types.hlsli exactly.
static_assert(sizeof(WorldMeshGPU) == 64u, "WorldMeshGPU layout must match world_bvh_types.hlsli");
static_assert(sizeof(WorldInstanceGPU) == 192u, "WorldInstanceGPU layout must match world_bvh_types.hlsli");

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
  // Alpha-tested foliage: triangle hits tested against the atlas, and those cut (invalid reads count as invalid_refs).
  uint32_t alpha_tests = 0u;
  uint32_t alpha_cut = 0u;
  // Rays whose nearest BVH surface the game camera does not show: an instance
  // no camera VS drew, or a near-faded surface (shown in the view when hiding
  // is off; skipped when it is on).
  uint32_t camera_hidden = 0u;
  bool hiding = false;  // the dispatch hid them
  bool valid = false;
  bool invariant_ok = false;
};

// Append-only GPU buffer of the live store: elements [0, used) are valid.
// It grows by creating a larger buffer and copying [0, used) on the GPU.
struct LiveArena {
  reshade::api::resource buffer = {0u};
  reshade::api::resource_view srv = {0u};
  uint32_t stride = 0u;
  uint64_t capacity = 0u;  // elements
  uint64_t used = 0u;      // elements
};

inline LiveArena MakeLiveArena(uint32_t stride) {
  LiveArena arena;
  arena.stride = stride;
  return arena;
}

// One mesh in the live store. Its index is the GPU mesh id that instance
// descriptors carry; a slot is never reused until the store is reset.
struct LiveMeshSlot {
  uint64_t uid = 0u;  // WorldMesh::uid
  bool live = true;   // false once the pool retired the mesh (its ranges are garbage)
  uint32_t vertex_offset = 0u;
  uint32_t vertex_count = 0u;
  uint32_t index_offset = 0u;
  uint32_t index_count = 0u;
  uint32_t leaf_offset = 0u;
  uint32_t leaf_count = 0u;
  uint32_t node_offset = 0u;
  uint32_t node_count = 0u;
  float bbox_min[3] = {};
  float bbox_max[3] = {};
  // UV arena range (one float2 per vertex); uv_count 0 = the mesh has no UVs.
  uint32_t uv_offset = 0u;
  uint32_t uv_count = 0u;
};

struct LiveBvhStats {
  // Store.
  uint32_t pool_meshes = 0u;
  uint32_t resident_meshes = 0u;
  uint32_t pending_meshes = 0u;   // in the pool, not uploaded yet
  uint32_t failed_meshes = 0u;    // no usable BLAS (reason in last_failure)
  uint64_t meshes_uploaded = 0u;  // since the last reset of the counters
  uint64_t triangles_uploaded = 0u;
  uint32_t meshes_retired = 0u;
  uint32_t resets = 0u;
  uint32_t grows = 0u;
  uint32_t upload_failures = 0u;  // GPU buffer creation failures
  uint64_t used_bytes = 0u;
  uint64_t capacity_bytes = 0u;
  uint64_t garbage_bytes = 0u;    // ranges of retired meshes
  uint32_t resident_triangles = 0u;
  std::string last_reset_reason;
  std::string last_failure;
  // TLAS.
  uint32_t tlas_rebuilds = 0u;
  uint32_t tlas_instances = 0u;   // in the TLAS
  uint32_t tlas_waiting = 0u;     // in the region, mesh not uploaded yet
  uint32_t tlas_unusable = 0u;    // in the region, mesh failed (no BLAS)
  uint32_t tlas_alpha_instances = 0u;  // alpha-tested instances in the TLAS (with a material slot)
  uint32_t tlas_alpha_waiting = 0u;    // alpha-tested instances left out: their material is not on the GPU yet
  uint32_t tlas_near_fade = 0u;      // in the TLAS with the game camera's near fade applied
  uint32_t tlas_camera_hidden = 0u;  // in the TLAS, never drawn by a camera VS (shadow-only or no view)
  uint32_t tlas_nodes = 0u;
  uint32_t tlas_frame = 0u;
  const char* tlas_reason = "";
  // Every tree is validated on the CPU before upload.
  uint32_t blas_invalid = 0u;
  uint32_t tlas_invalid = 0u;
  // CPU time on the present thread.
  float mesh_ms_last = 0.f;
  float mesh_ms_max = 0.f;
  float tlas_ms_last = 0.f;
  float tlas_ms_max = 0.f;
  // TLAS refits (RefitLiveTlas): moved dynamic instances without a rebuild.
  uint32_t tlas_refits = 0u;
  uint32_t refit_instances = 0u;  // descriptors that changed in the last refit
  float refit_ms_last = 0.f;
  float refit_ms_max = 0.f;
  uint32_t refit_lag_last = 0u;   // frames since the oldest refit instance was followed
  uint32_t refit_lag_max = 0u;
  uint32_t refit_missing = 0u;    // refit instances no longer in the pool
  uint32_t refit_failures = 0u;
  // GPU contents check (on request; waits for the GPU).
  bool gpu_checked = false;
  bool gpu_ok = false;
  std::string gpu_result;
};

// One region instance as copied for a TLAS build, with its pool provenance
// (kept per TLAS instance for the inspect tool).
struct LiveTlasInstance {
  uint64_t uid = 0u;       // WorldMesh::uid
  bool alpha = false;      // alpha-tested mesh (copied under g_pool.mutex); traced through its atlas slice
  uint32_t material = 0u;  // atlas slice + 1 at the TLAS build (descriptor header.z); 0 = not alpha-tested
  uint64_t id = 0u;        // PoolState instance id (ascending in the pool)
  bool dynamic = false;    // moving instance: refitted, not rebuilt
  uint64_t mesh_key = 0u;  // draw key it was admitted through
  uint32_t vs_hash = 0u;
  uint8_t source = 0u;
  uint32_t matrix_floats = kPoolWorldFloats;
  float matrix[16] = {};
  float inverse_world[16] = {};
  float bounds_min[3] = {};
  float bounds_max[3] = {};
  uint32_t first_frame = 0u;
  uint32_t admit_frame = 0u;
  float admit_camera[3] = {};
  bool admit_camera_valid = false;
  PoolCameraVisibility visibility;  // at the TLAS build (GetPoolCameraVisibility)
};

// What the trace hit at one pixel (middle-click in a trace view), read back
// with the trace statistics.
struct BvhInspect {
  bool requested = false;  // the last dispatch carried an inspect pixel
  bool fresh = false;      // read back, description not built yet
  bool valid = false;      // the pixel was traced
  bool hit = false;
  uint32_t x = 0u;
  uint32_t y = 0u;
  uint32_t instance = 0u;  // TLAS instance (index into tlas_info / tlas_instances_cpu)
  uint32_t mesh = 0u;      // live store slot
  uint32_t prim = 0u;
  float t = 0.f;
  float position[3] = {};
  uint32_t compare_class = UINT32_MAX;  // Depth Compare pixel class (trace stats index), if that view
  bool camera_hidden = false;  // the nearest surface is one the game camera does not show (shown, or skipped when hiding)
  bool hiding = false;         // the dispatch hid such surfaces
  std::vector<std::string> lines;
};

// GPU side of the atlas, owned by BvhDeviceData::alpha. Everything is null until the first
// alpha mesh needs a slice, and is destroyed again when alpha_foliage goes off.
struct AlphaGpu {
  reshade::api::resource atlas = {0u};  // Texture2DArray<uint>, kAlphaAtlasSlices layers of 256 x 256
  reshade::api::resource_view atlas_srv = {0u};
  reshade::api::resource_view atlas_uav = {0u};
  reshade::api::resource materials = {0u};  // AlphaMaterialGPU per slice (zero-initialised)
  reshade::api::resource_view materials_srv = {0u};
  reshade::api::sampler sampler = {0u};
  reshade::api::pipeline_layout blit_layout = {0u};
  reshade::api::descriptor_table blit_tables[3] = {};  // t0 source, s0 sampler, u0 atlas
  reshade::api::pipeline blit_pipeline = {0u};
  AlphaSliceTable slices;
  std::unordered_map<uint64_t, uint32_t> slot_of_uid;     // mesh uid -> slice of its material (read by the TLAS)
  std::unordered_set<uint64_t> indirect_uids;  // mesh uids whose slice was filled from an indirect key (dropped with the switch OFF)
  std::vector<std::pair<uint32_t, uint32_t>> quarantine;  // (slice, frame released), reused after kAlphaSliceQuarantineFrames
  GpuTimer timer;
  uint64_t blits = 0u;       // slices filled since the atlas was created
  uint32_t blits_frame = 0u;  // slices filled at the last present
  uint64_t cap_refused = 0u;  // meshes refused a slice (the atlas is full), counted once per mesh
  std::unordered_set<uint64_t> cap_refused_uids;  // meshes counted in cap_refused (cleared with the GPU objects)
  bool failure_logged = false;  // EnsureAlphaGpu logs its failure once (re-armed with the GPU objects)
};

struct __declspec(uuid("b7a1c2d3-4e5f-4a6b-8c7d-9e0f1a2b3c4d")) BvhDeviceData {
  // Live store (bvh_live.hpp).
  LiveArena vertices = MakeLiveArena(sizeof(float) * 4u);  // float4, w = 1
  LiveArena indices = MakeLiveArena(sizeof(uint32_t));     // mesh-local
  LiveArena blas_leaves = MakeLiveArena(sizeof(BVHLeafGPU));
  LiveArena blas_nodes = MakeLiveArena(sizeof(BVHNodeGPU));
  LiveArena uvs = MakeLiveArena(sizeof(float) * 2u);  // float2, only meshes with UVs (alpha foliage)
  AlphaGpu alpha;  // atlas, materials and blit (alpha_live.hpp); empty while alpha_foliage is off
  std::vector<LiveMeshSlot> slots;
  std::unordered_map<uint64_t, uint32_t> slot_by_uid;
  std::unordered_set<uint64_t> failed_uids;
  uint64_t store_version = 0u;     // bumped whenever slots change (or a mesh failed)
  uint64_t mesh_revision = 0u;     // pool revision at the last mesh scan
  bool mesh_scan_needed = true;    // meshes were left pending at the last scan
  bool store_full = false;         // a mesh did not fit below 2^24 elements at the last upload
  bool descriptors_dirty = false;  // slots changed since the descriptor buffer was uploaded
  uint32_t upload_retry_frame = 0u;  // uploads wait until then after a GPU buffer failure
  uint32_t tlas_retry_frame = 0u;    // TLAS rebuilds wait until then after a failure

  // Mesh descriptors (one per slot, retired slots zeroed), exact size.
  // descriptor_count: slots the uploaded buffer covers (the TLAS only uses those).
  reshade::api::resource mesh_buffer = {0u};
  reshade::api::resource_view mesh_srv = {0u};
  std::vector<WorldMeshGPU> mesh_descriptors;
  uint32_t descriptor_count = 0u;

  // TLAS snapshot: region instances, identity active list, TLAS, exact size.
  reshade::api::resource instance_buffer = {0u};
  reshade::api::resource_view instance_srv = {0u};
  reshade::api::resource active_buffer = {0u};
  reshade::api::resource_view active_srv = {0u};
  reshade::api::resource tlas_leaf_buffer = {0u};
  reshade::api::resource_view tlas_leaf_srv = {0u};
  reshade::api::resource tlas_node_buffer = {0u};
  reshade::api::resource_view tlas_node_srv = {0u};
  std::vector<WorldInstanceGPU> tlas_instances_cpu;  // as uploaded (GPU check)
  std::vector<LiveTlasInstance> tlas_info;           // pool provenance per TLAS instance (inspect)
  std::vector<BVHLeafGPU> tlas_leaves_cpu;
  std::vector<BVHNodeGPU> tlas_nodes_cpu;
  std::vector<uint32_t> tlas_leaf_of_instance;  // TLAS instance -> its leaf (sorted leaves)
  uint32_t tlas_dynamic_first = 0u;             // instances [first, end) are dynamic
  uint32_t tlas_refits_since_rebuild = 0u;
  bool tlas_refit_failed = false;               // the next TLAS update is a full rebuild
  uint64_t tlas_pool_revision = 0u;
  uint64_t tlas_visibility_revision = 0u;  // g_pool.visibility_revision the TLAS reflects
  uint64_t tlas_dynamic_revision = 0u;     // g_pool.dynamic_revision the TLAS reflects
  uint64_t tlas_store_version = 0u;
  uint8_t tlas_alpha_mode = 0u;  // AlphaMode() at the last TLAS build (LiveTlasChange: a change rebuilds it)
  float tlas_region_min[3] = {1e30f, 1e30f, 1e30f};
  float tlas_region_size = 0.f;
  bool tlas_built = false;
  uint32_t active_count = 0u;  // instances in the uploaded TLAS
  bool bvh_ready = false;      // store and TLAS can be traced

  reshade::api::resource debug_texture = {0u};
  reshade::api::resource_view debug_srv = {0u};
  reshade::api::resource_view debug_uav = {0u};
  uint32_t debug_width = 0u;
  uint32_t debug_height = 0u;

  reshade::api::pipeline_layout trace_layout = {0u};
  reshade::api::descriptor_table trace_srv_table = {0u};
  reshade::api::descriptor_table trace_uav_table = {0u};
  reshade::api::pipeline trace_pipeline = {0u};
  reshade::api::resource trace_stats_buffer = {0u};
  reshade::api::resource_view trace_stats_uav = {0u};
  BvhTraceStats trace_stats;
  bool trace_hiding = false;  // camera-view hiding setting of the last dispatch
  uint32_t trace_dynamic_count = 0u;  // deforming objects bound to the last dispatch
  BvhInspect inspect;
  bool trace_ready = false;
  GpuTimer trace_timer;

  LiveBvhStats live;
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

inline void DestroyLiveArena(reshade::api::device* device, LiveArena* arena) {
  DestroyBuffer(device, &arena->srv, &arena->buffer);
  arena->capacity = 0u;
  arena->used = 0u;
}

// Drops the TLAS snapshot; the trace stops until the next TLAS is built.
inline void DestroyLiveTlas(reshade::api::device* device, BvhDeviceData* data) {
  DestroyBuffer(device, &data->instance_srv, &data->instance_buffer);
  DestroyBuffer(device, &data->active_srv, &data->active_buffer);
  DestroyBuffer(device, &data->tlas_leaf_srv, &data->tlas_leaf_buffer);
  DestroyBuffer(device, &data->tlas_node_srv, &data->tlas_node_buffer);
  data->tlas_instances_cpu.clear();
  data->tlas_info.clear();
  data->tlas_leaves_cpu.clear();
  data->tlas_nodes_cpu.clear();
  data->tlas_leaf_of_instance.clear();
  data->tlas_dynamic_first = 0u;
  data->tlas_refits_since_rebuild = 0u;
  data->tlas_built = false;
  data->active_count = 0u;
  data->bvh_ready = false;
}

// Drops every mesh of the store (and the TLAS, whose mesh ids it invalidates).
inline void DestroyLiveStore(reshade::api::device* device, BvhDeviceData* data) {
  DestroyLiveTlas(device, data);
  DestroyLiveArena(device, &data->vertices);
  DestroyLiveArena(device, &data->indices);
  DestroyLiveArena(device, &data->blas_leaves);
  DestroyLiveArena(device, &data->blas_nodes);
  DestroyLiveArena(device, &data->uvs);
  DestroyBuffer(device, &data->mesh_srv, &data->mesh_buffer);
  data->mesh_descriptors.clear();
  data->descriptor_count = 0u;
  data->slots.clear();
  data->slot_by_uid.clear();
  data->failed_uids.clear();
  data->store_version += 1u;
  data->mesh_scan_needed = true;
  data->store_full = false;
  data->descriptors_dirty = false;
  data->upload_retry_frame = 0u;
  data->tlas_retry_frame = 0u;
}

// Destroys the alpha atlas, its materials and the blit (alpha_foliage off, or the device goes). The TLAS drops
// the alpha instances (store_version). The counters are kept.
inline void DestroyAlphaGpu(reshade::api::device* device, BvhDeviceData* data) {
  AlphaGpu& alpha = data->alpha;
  if (!alpha.slot_of_uid.empty()) data->store_version += 1u;
  for (reshade::api::descriptor_table& table : alpha.blit_tables) {
    if (table.handle != 0u) device->free_descriptor_table(table);
    table = {0u};
  }
  if (alpha.blit_pipeline.handle != 0u) device->destroy_pipeline(alpha.blit_pipeline);
  if (alpha.blit_layout.handle != 0u) device->destroy_pipeline_layout(alpha.blit_layout);
  if (alpha.sampler.handle != 0u) device->destroy_sampler(alpha.sampler);
  DestroyBuffer(device, &alpha.atlas_srv, nullptr);
  DestroyBuffer(device, &alpha.atlas_uav, &alpha.atlas);
  DestroyBuffer(device, &alpha.materials_srv, &alpha.materials);
  if (alpha.timer.created) DestroyGpuTimer(&alpha.timer);
  alpha.blit_pipeline = {0u};
  alpha.blit_layout = {0u};
  alpha.sampler = {0u};
  alpha.slices = {};
  alpha.slot_of_uid.clear();
  alpha.indirect_uids.clear();
  alpha.quarantine.clear();
  alpha.cap_refused_uids.clear();
  alpha.failure_logged = false;
  alpha.blits_frame = 0u;
}

inline void DestroyBvhDeviceData(reshade::api::device* device) {
  for (const reshade::api::resource proxy : TakePoolAlphaProxies(true)) device->destroy_resource(proxy);
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return;
  DestroyAlphaGpu(device, data);
  DestroyLiveStore(device, data);
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
  DestroyGpuTimer(&data->trace_timer);
  data->trace_stats = {};
  data->trace_ready = false;
  data->mesh_revision = 0u;
  data->upload_retry_frame = 0u;
  data->tlas_pool_revision = 0u;
  data->tlas_visibility_revision = 0u;
  data->tlas_store_version = 0u;
  data->live = {};
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
  return "type=buffer elems=" + std::to_string(desc.buffer.stride != 0u ? desc.buffer.size / desc.buffer.stride : 0u)
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

// Writes `size` bytes at byte `offset` of a buffer whose elements are
// `stride` bytes. ReShade's D3D11 update_buffer_region passes no box to
// UpdateSubresource when the offset is 0
// (external/reshade/source/d3d11/d3d11_impl_device.cpp), so the driver then
// writes the whole buffer and reads the buffer's full size from `bytes`. A
// write at offset 0 that does not cover the whole buffer therefore goes
// through a temporary buffer of the same stride created with the data, and a
// GPU copy. (ReShade's get_resource_desc reports buffer stride 0, so the
// stride is passed in.) Caller must not hold g_pool.mutex (graphics calls).
inline bool WriteBufferRange(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    reshade::api::resource buffer,
    uint32_t stride,
    uint64_t offset,
    const void* bytes,
    uint64_t size,
    std::string* error) {
  if (size == 0u) return true;
  const reshade::api::resource_desc desc = device->get_resource_desc(buffer);
  if (desc.type != reshade::api::resource_type::buffer || offset + size > desc.buffer.size) {
    if (error != nullptr) *error = "write outside the buffer";
    return false;
  }
  if (offset != 0u || size == desc.buffer.size) {
    device->update_buffer_region(bytes, buffer, offset, size);
    return true;
  }
  reshade::api::resource_desc temp_desc = {};
  temp_desc.type = reshade::api::resource_type::buffer;
  temp_desc.buffer.size = size;
  temp_desc.buffer.stride = stride;
  temp_desc.heap = reshade::api::memory_heap::gpu_only;
  temp_desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source;
  reshade::api::subresource_data initial = {};
  initial.data = const_cast<void*>(bytes);
  reshade::api::resource temp = {0u};
  if (!device->create_resource(temp_desc, &initial, reshade::api::resource_usage::copy_source, &temp)) {
    if (error != nullptr) *error = "upload buffer creation failed: " + DescribePoolBufferFailure(device, temp_desc);
    return false;
  }
  cmd_list->copy_buffer_region(temp, 0u, buffer, 0u, size);
  device->destroy_resource(temp);  // D3D11 keeps it alive until the copy has run
  return true;
}

}  // namespace falcom_world::bvh
