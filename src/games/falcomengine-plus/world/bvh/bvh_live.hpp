#pragma once

// Live world BVH.
//
// Keeps the GPU BVH in step with the pool while the game runs, on the present
// thread, without waiting for the GPU:
//
// Mesh store. Every captured mesh (WorldMesh, keyed by its never-reused uid)
// is uploaded once into append-only GPU arenas (vertices, indices, BLAS leaves,
// BLAS nodes) with its BLAS built on the CPU (bvh_build.hpp). Uploads are
// budgeted to kLiveTrianglesPerFrame per present (always at least one mesh),
// so a full pool streams in over a few frames instead of one long stall. An
// arena that runs out of room is replaced by one twice as large and the old
// contents are copied on the GPU. A mesh the pool retires (its VB/IB was
// released) keeps its slot but its descriptor is zeroed at once; its ranges
// become garbage. When garbage passes half of the store (and
// kLiveResetGarbageBytes), no live mesh is left (a map change), or a mesh no
// longer fits below the float-exact offset limit while garbage exists, the
// store is reset and the pool's meshes stream in again.
//
// TLAS. When the pool's instances, the store or the region changed, at most
// every kLiveTlasInterval frames, the region's instances whose mesh is
// resident are snapshotted under the pool lock, their descriptors and TLAS are
// built on the CPU, validated, and uploaded as exact-size buffers (the trace
// reads their sizes with GetDimensions). A failed build or upload keeps the
// previous TLAS.
//
// Failures (GPU buffer creation, a tree that fails validation) keep the last
// working state, record a reason in LiveBvhStats, and are retried after
// kLiveRetryFrames; a store reset clears the wait.
//
// Lock rule (see bvh_pool.hpp): g_pool.mutex is only held to copy pool data;
// every graphics call (buffer creation, update, copy, destroy) happens after
// it is released. GPU readback happens only in the on-request GPU check,
// which waits for the GPU by design.
//
// Every tree is structurally validated on the CPU before upload; the GPU
// check compares the GPU buffers with what was uploaded (mesh data and BLAS
// rebuilt from the pool, descriptors, TLAS) and re-validates the read-back
// trees.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../../../utils/log.hpp"
#include "../../../../utils/scene.hpp"
#include "../debug/pool_stage.hpp"
#include "bvh_build.hpp"
#include "bvh_pool.hpp"
#include "bvh_resources.hpp"

namespace falcom_world::bvh {

inline constexpr uint64_t kLiveTrianglesPerFrame = 16384u;  // BLAS work per present (at least one mesh)
inline constexpr uint32_t kLiveMeshesPerFrame = 64u;
inline constexpr uint32_t kLiveTlasInterval = 8u;           // frames between TLAS rebuilds
inline constexpr uint64_t kLiveFloatExact = 1ull << 24u;    // mesh descriptor offsets are floats
inline constexpr uint64_t kLiveResetGarbageBytes = 16ull * 1024ull * 1024ull;
inline constexpr uint32_t kLiveRetryFrames = 120u;          // after a GPU buffer creation failure
inline constexpr uint64_t kLiveReadbackChunk = 32ull * 1024ull * 1024ull;
inline constexpr uint64_t kLiveArenaMinBytes = 1024ull * 1024ull;

struct LiveBvhSwitches {
  std::atomic_bool enabled{true};
  std::atomic_bool reset_requested{false};
  std::atomic_bool check_requested{false};
};

inline LiveBvhSwitches g_live_bvh;

inline float LiveElapsedMs(std::chrono::steady_clock::time_point start) {
  return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
}

inline uint64_t LiveArenaBytes(const LiveArena& arena, uint64_t elements) {
  return elements * arena.stride;
}

inline uint64_t LiveSlotBytes(const BvhDeviceData& data, const LiveMeshSlot& slot) {
  return LiveArenaBytes(data.vertices, slot.vertex_count) + LiveArenaBytes(data.indices, slot.index_count)
         + LiveArenaBytes(data.blas_leaves, slot.leaf_count) + LiveArenaBytes(data.blas_nodes, slot.node_count);
}

inline void UpdateLiveStoreStats(BvhDeviceData* data, uint32_t pool_meshes) {
  LiveBvhStats& stats = data->live;
  stats.pool_meshes = pool_meshes;
  stats.resident_meshes = 0u;
  stats.resident_triangles = 0u;
  stats.garbage_bytes = 0u;
  for (const LiveMeshSlot& slot : data->slots) {
    if (slot.live) {
      stats.resident_meshes += 1u;
      stats.resident_triangles += slot.leaf_count;
    } else {
      stats.garbage_bytes += LiveSlotBytes(*data, slot);
    }
  }
  stats.used_bytes = LiveArenaBytes(data->vertices, data->vertices.used) + LiveArenaBytes(data->indices, data->indices.used)
                     + LiveArenaBytes(data->blas_leaves, data->blas_leaves.used)
                     + LiveArenaBytes(data->blas_nodes, data->blas_nodes.used);
  stats.capacity_bytes = LiveArenaBytes(data->vertices, data->vertices.capacity)
                         + LiveArenaBytes(data->indices, data->indices.capacity)
                         + LiveArenaBytes(data->blas_leaves, data->blas_leaves.capacity)
                         + LiveArenaBytes(data->blas_nodes, data->blas_nodes.capacity);
  stats.failed_meshes = static_cast<uint32_t>(data->failed_uids.size());
}

inline void ResetLiveStore(reshade::api::device* device, BvhDeviceData* data, const char* reason) {
  const size_t meshes = data->slots.size();
  DestroyLiveStore(device, data);
  data->live.resets += 1u;
  data->live.last_reset_reason = reason;
  UpdateLiveStoreStats(data, data->live.pool_meshes);
  renodx::utils::log::i("[world-bvh] live store reset (", reason, "): ", meshes, " mesh slots dropped");
}

// ---------------------------------------------------------------------------
// Mesh store.

struct LiveMeshUpload {
  uint64_t uid = 0u;
  float bbox_min[3] = {};
  float bbox_max[3] = {};
  std::vector<std::array<float, 3>> positions;
  std::vector<uint32_t> indices;
};

struct LiveMeshScan {
  uint64_t revision = 0u;
  std::vector<uint64_t> live_uids;
  std::vector<LiveMeshUpload> uploads;
  uint32_t pending = 0u;  // not resident and not failed, the uploads included
};

// Under g_pool.mutex: the pool's mesh uids, and (when `collect_uploads`)
// copies of the next meshes to upload within the triangle budget (always at
// least one).
inline LiveMeshScan ScanLiveMeshes(const BvhDeviceData& data, bool collect_uploads) {
  LiveMeshScan scan;
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  scan.revision = g_pool.revision;
  scan.live_uids.reserve(g_pool.meshes.size());
  uint64_t budget = kLiveTrianglesPerFrame;
  for (const WorldMesh& mesh : g_pool.meshes) {
    scan.live_uids.push_back(mesh.uid);
    if (data.slot_by_uid.count(mesh.uid) != 0u || data.failed_uids.count(mesh.uid) != 0u) continue;
    scan.pending += 1u;
    if (!collect_uploads || scan.uploads.size() >= kLiveMeshesPerFrame) continue;
    const uint64_t triangles = mesh.indices.size() / 3u;
    if (!scan.uploads.empty() && triangles > budget) continue;
    budget = triangles >= budget ? 0u : budget - triangles;
    LiveMeshUpload& upload = scan.uploads.emplace_back();
    upload.uid = mesh.uid;
    std::memcpy(upload.bbox_min, mesh.bbox_min, sizeof(upload.bbox_min));
    std::memcpy(upload.bbox_max, mesh.bbox_max, sizeof(upload.bbox_max));
    upload.positions = mesh.positions;
    upload.indices = mesh.indices;
  }
  return scan;
}

// Zeroes the descriptors of slots whose mesh left the pool; returns how many.
inline uint32_t RetireLiveSlots(BvhDeviceData* data, const std::vector<uint64_t>& live_uids) {
  const std::unordered_set<uint64_t> live(live_uids.begin(), live_uids.end());
  uint32_t retired = 0u;
  for (size_t i = 0; i < data->slots.size(); ++i) {
    LiveMeshSlot& slot = data->slots[i];
    if (!slot.live || live.count(slot.uid) != 0u) continue;
    slot.live = false;
    data->slot_by_uid.erase(slot.uid);
    if (i < data->mesh_descriptors.size()) data->mesh_descriptors[i] = WorldMeshGPU{};
    retired += 1u;
  }
  for (auto it = data->failed_uids.begin(); it != data->failed_uids.end();) {
    if (live.count(*it) == 0u) {
      it = data->failed_uids.erase(it);
    } else {
      ++it;
    }
  }
  return retired;
}

inline const char* LiveStoreResetReason(const BvhDeviceData& data) {
  if (data.slots.empty()) return nullptr;
  if (data.live.resident_meshes == 0u) return "no live mesh left";
  if (data.live.garbage_bytes >= kLiveResetGarbageBytes && data.live.garbage_bytes * 2u > data.live.used_bytes) {
    return "retired meshes fill half of the store";
  }
  if (data.store_full && data.live.garbage_bytes != 0u) return "store full, reclaiming retired meshes";
  return nullptr;
}

// Grows `arena` to hold `needed` elements; the old contents are copied on the
// GPU. Returns false (and the reason) when the new buffer cannot be created.
inline bool EnsureLiveArena(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    BvhDeviceData* data,
    LiveArena* arena,
    uint64_t needed,
    std::string* error) {
  if (needed <= arena->capacity) return true;
  const uint64_t min_elements = (kLiveArenaMinBytes + arena->stride - 1u) / arena->stride;
  const uint64_t capacity = (std::max)({needed, arena->capacity * 2u, min_elements});
  LiveArena grown = MakeLiveArena(arena->stride);
  if (!CreatePoolBuffer(
          device, nullptr, capacity * arena->stride, arena->stride, &grown.buffer, &grown.srv,
          reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source
              | reshade::api::resource_usage::copy_dest,
          error)) {
    return false;
  }
  if (arena->used != 0u) {
    cmd_list->copy_buffer_region(arena->buffer, 0u, grown.buffer, 0u, arena->used * arena->stride);
  }
  DestroyBuffer(device, &arena->srv, &arena->buffer);
  arena->buffer = grown.buffer;
  arena->srv = grown.srv;
  arena->capacity = capacity;
  data->live.grows += 1u;
  return true;
}

struct LiveBuiltMesh {
  LiveMeshUpload* upload = nullptr;
  MeshBlas blas;
};

struct LiveUploadResult {
  uint32_t uploaded = 0u;  // meshes that became resident
  uint32_t failed = 0u;    // meshes without a usable BLAS
};

// Builds the BLAS of each upload and appends the successful ones that fit to
// the arenas. Mesh descriptors carry the offsets as floats, exact below 2^24:
// a mesh that would cross that limit stays pending and flags the store full
// (it is reset once retired data can be reclaimed).
inline LiveUploadResult UploadLiveMeshes(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    BvhDeviceData* data,
    std::vector<LiveMeshUpload>* uploads,
    uint32_t frame) {
  LiveBvhStats& stats = data->live;
  LiveUploadResult result;
  data->store_full = false;
  std::vector<LiveBuiltMesh> built;
  built.reserve(uploads->size());
  uint64_t vertex_total = 0u;
  uint64_t index_total = 0u;
  uint64_t leaf_total = 0u;
  uint64_t node_total = 0u;
  for (LiveMeshUpload& upload : *uploads) {
    LiveBuiltMesh mesh;
    mesh.upload = &upload;
    const char* error = BuildMeshBlas(upload.positions, upload.indices, &mesh.blas);
    if (error != nullptr) {
      data->failed_uids.insert(upload.uid);
      result.failed += 1u;
      stats.blas_invalid += 1u;
      stats.last_failure = std::string("mesh BLAS: ") + error;
      renodx::utils::log::w("[world-bvh] mesh uid ", upload.uid, " not uploaded: ", error, " (",
                            upload.indices.size() / 3u, " triangles, ", upload.positions.size(), " vertices)");
      continue;
    }
    if (data->vertices.used + vertex_total + upload.positions.size() > kLiveFloatExact
        || data->indices.used + index_total + upload.indices.size() > kLiveFloatExact
        || data->blas_leaves.used + leaf_total + mesh.blas.leaves.size() > kLiveFloatExact
        || data->blas_nodes.used + node_total + mesh.blas.nodes.size() > kLiveFloatExact) {
      data->store_full = true;
      continue;
    }
    vertex_total += upload.positions.size();
    index_total += upload.indices.size();
    leaf_total += mesh.blas.leaves.size();
    node_total += mesh.blas.nodes.size();
    built.push_back(std::move(mesh));
  }
  if (data->store_full) stats.last_failure = "store full: offsets would exceed 2^24 elements";
  if (built.empty()) return result;

  std::string error;
  if (!EnsureLiveArena(device, cmd_list, data, &data->vertices, data->vertices.used + vertex_total, &error)
      || !EnsureLiveArena(device, cmd_list, data, &data->indices, data->indices.used + index_total, &error)
      || !EnsureLiveArena(device, cmd_list, data, &data->blas_leaves, data->blas_leaves.used + leaf_total, &error)
      || !EnsureLiveArena(device, cmd_list, data, &data->blas_nodes, data->blas_nodes.used + node_total, &error)) {
    stats.upload_failures += 1u;
    stats.last_failure = "arena buffer creation failed: " + error;
    data->upload_retry_frame = frame + kLiveRetryFrames;
    renodx::utils::log::w("[world-bvh] live store: ", stats.last_failure);
    return result;
  }

  std::vector<float> vertex_batch;
  std::vector<uint32_t> index_batch;
  std::vector<BVHLeafGPU> leaf_batch;
  std::vector<BVHNodeGPU> node_batch;
  vertex_batch.reserve(vertex_total * 4u);
  index_batch.reserve(index_total);
  leaf_batch.reserve(leaf_total);
  node_batch.reserve(node_total);
  std::vector<LiveMeshSlot> new_slots;
  new_slots.reserve(built.size());
  for (const LiveBuiltMesh& mesh : built) {
    const LiveMeshUpload& upload = *mesh.upload;
    LiveMeshSlot slot;
    slot.uid = upload.uid;
    slot.vertex_offset = static_cast<uint32_t>(data->vertices.used + vertex_batch.size() / 4u);
    slot.vertex_count = static_cast<uint32_t>(upload.positions.size());
    slot.index_offset = static_cast<uint32_t>(data->indices.used + index_batch.size());
    slot.index_count = static_cast<uint32_t>(upload.indices.size());
    slot.leaf_offset = static_cast<uint32_t>(data->blas_leaves.used + leaf_batch.size());
    slot.leaf_count = static_cast<uint32_t>(mesh.blas.leaves.size());
    slot.node_offset = static_cast<uint32_t>(data->blas_nodes.used + node_batch.size());
    slot.node_count = static_cast<uint32_t>(mesh.blas.nodes.size());
    std::memcpy(slot.bbox_min, upload.bbox_min, sizeof(slot.bbox_min));
    std::memcpy(slot.bbox_max, upload.bbox_max, sizeof(slot.bbox_max));

    for (const auto& position : upload.positions) {
      vertex_batch.push_back(position[0]);
      vertex_batch.push_back(position[1]);
      vertex_batch.push_back(position[2]);
      vertex_batch.push_back(1.f);
    }
    index_batch.insert(index_batch.end(), upload.indices.begin(), upload.indices.end());
    leaf_batch.insert(leaf_batch.end(), mesh.blas.leaves.begin(), mesh.blas.leaves.end());
    node_batch.insert(node_batch.end(), mesh.blas.nodes.begin(), mesh.blas.nodes.end());
    new_slots.push_back(slot);
  }

  // All four appends or none: `used` only advances once every write was
  // issued, so a failure leaves no slot pointing at unwritten data.
  const auto append = [&](LiveArena* arena, const void* bytes, uint64_t elements) {
    return WriteBufferRange(device, cmd_list, arena->buffer, arena->stride, arena->used * arena->stride, bytes,
                            elements * arena->stride, &error);
  };
  if (!append(&data->vertices, vertex_batch.data(), vertex_batch.size() / 4u)
      || !append(&data->indices, index_batch.data(), index_batch.size())
      || !append(&data->blas_leaves, leaf_batch.data(), leaf_batch.size())
      || !append(&data->blas_nodes, node_batch.data(), node_batch.size())) {
    stats.upload_failures += 1u;
    stats.last_failure = "arena write failed: " + error;
    data->upload_retry_frame = frame + kLiveRetryFrames;
    renodx::utils::log::w("[world-bvh] live store: ", stats.last_failure);
    return result;
  }
  data->vertices.used += vertex_batch.size() / 4u;
  data->indices.used += index_batch.size();
  data->blas_leaves.used += leaf_batch.size();
  data->blas_nodes.used += node_batch.size();

  for (const LiveMeshSlot& slot : new_slots) {
    WorldMeshGPU descriptor = {};
    descriptor.header[0] = static_cast<float>(slot.vertex_offset);
    descriptor.header[1] = static_cast<float>(slot.vertex_count);
    descriptor.header[2] = static_cast<float>(slot.index_offset);
    descriptor.header[3] = static_cast<float>(slot.index_count);
    for (int k = 0; k < 3; ++k) {
      descriptor.bbox_min[k] = slot.bbox_min[k];
      descriptor.bbox_max[k] = slot.bbox_max[k];
    }
    descriptor.build[0] = static_cast<float>(slot.node_offset);
    descriptor.build[1] = 0.f;
    descriptor.build[2] = static_cast<float>(slot.leaf_offset);
    descriptor.build[3] = static_cast<float>(slot.leaf_count);
    data->slot_by_uid[slot.uid] = static_cast<uint32_t>(data->slots.size());
    data->slots.push_back(slot);
    data->mesh_descriptors.push_back(descriptor);
    stats.meshes_uploaded += 1u;
    stats.triangles_uploaded += slot.leaf_count;
  }
  result.uploaded = static_cast<uint32_t>(new_slots.size());
  return result;
}

// Replaces the mesh descriptor buffer (exact size). On failure the previous
// buffer stays (descriptor_count tells the TLAS which slots it covers) and
// the upload is retried after kLiveRetryFrames.
inline bool UploadLiveMeshDescriptors(reshade::api::device* device, BvhDeviceData* data, uint32_t frame) {
  if (data->mesh_descriptors.empty()) {
    data->descriptors_dirty = false;
    return true;
  }
  reshade::api::resource buffer = {0u};
  reshade::api::resource_view view = {0u};
  std::string error;
  if (!CreatePoolBuffer(
          device, data->mesh_descriptors.data(), data->mesh_descriptors.size() * sizeof(WorldMeshGPU),
          sizeof(WorldMeshGPU), &buffer, &view,
          reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source, &error)) {
    data->live.upload_failures += 1u;
    data->live.last_failure = "mesh descriptor buffer creation failed: " + error;
    data->upload_retry_frame = frame + kLiveRetryFrames;
    renodx::utils::log::w("[world-bvh] live store: ", data->live.last_failure);
    return false;
  }
  DestroyBuffer(device, &data->mesh_srv, &data->mesh_buffer);
  data->mesh_buffer = buffer;
  data->mesh_srv = view;
  data->descriptor_count = static_cast<uint32_t>(data->mesh_descriptors.size());
  data->descriptors_dirty = false;
  data->store_version += 1u;  // the TLAS can now use the new slots
  return true;
}

inline void SyncLiveMeshes(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    BvhDeviceData* data,
    uint32_t frame) {
  const auto start = std::chrono::steady_clock::now();
  const bool can_upload = frame >= data->upload_retry_frame;
  LiveMeshScan scan = ScanLiveMeshes(*data, can_upload);
  const uint32_t retired = RetireLiveSlots(data, scan.live_uids);
  data->live.meshes_retired += retired;
  UpdateLiveStoreStats(data, static_cast<uint32_t>(scan.live_uids.size()));
  if (const char* reason = LiveStoreResetReason(*data); reason != nullptr) {
    ResetLiveStore(device, data, reason);
    data->live.pending_meshes = static_cast<uint32_t>(scan.live_uids.size());
    data->live.mesh_ms_last = LiveElapsedMs(start);
    data->live.mesh_ms_max = (std::max)(data->live.mesh_ms_max, data->live.mesh_ms_last);
    return;
  }
  LiveUploadResult upload;
  if (!scan.uploads.empty()) upload = UploadLiveMeshes(device, cmd_list, data, &scan.uploads, frame);
  if (retired != 0u || upload.uploaded != 0u) data->descriptors_dirty = true;
  if (upload.failed != 0u) data->store_version += 1u;  // the TLAS recounts instances of failed meshes
  if (data->descriptors_dirty && frame >= data->upload_retry_frame) UploadLiveMeshDescriptors(device, data, frame);
  UpdateLiveStoreStats(data, static_cast<uint32_t>(scan.live_uids.size()));
  // A full store without retired data to reclaim: try again later.
  if (data->store_full && data->live.garbage_bytes == 0u) data->upload_retry_frame = frame + kLiveRetryFrames;
  // Meshes that failed this frame are no longer pending.
  uint32_t pending = 0u;
  for (const LiveMeshUpload& upload : scan.uploads) {
    if (data->slot_by_uid.count(upload.uid) == 0u && data->failed_uids.count(upload.uid) == 0u) pending += 1u;
  }
  pending += scan.pending - static_cast<uint32_t>(scan.uploads.size());
  data->live.pending_meshes = pending;
  data->mesh_revision = scan.revision;
  data->mesh_scan_needed = pending != 0u || data->descriptors_dirty;
  data->live.mesh_ms_last = LiveElapsedMs(start);
  data->live.mesh_ms_max = (std::max)(data->live.mesh_ms_max, data->live.mesh_ms_last);
}

// ---------------------------------------------------------------------------
// TLAS.

inline const char* LiveTlasChange(const BvhDeviceData& data, uint64_t revision, uint64_t visibility_revision,
                                  const PoolRegion& region) {
  if (data.live.tlas_rebuilds == 0u && !data.tlas_built && data.tlas_pool_revision == 0u) return "first build";
  if (data.tlas_store_version != data.store_version) return "store changed";
  if (data.tlas_pool_revision != revision) return "pool changed";
  if (data.tlas_visibility_revision != visibility_revision) return "visibility changed";
  if (data.tlas_region_size != region.size || data.tlas_region_min[0] != region.min[0]
      || data.tlas_region_min[1] != region.min[1] || data.tlas_region_min[2] != region.min[2]) {
    return "region moved";
  }
  return nullptr;
}

// The GPU form of an instance's views (WorldInstanceGPU::visibility, see
// kInstance* flags): seen by a camera VS, by a light VS, and the near fade,
// which applies to camera-visible instances whose camera draws mostly
// near-fade and whose inputs are finite.
inline bool CameraFadeApplies(const PoolCameraVisibility& visibility) {
  return visibility.camera_seen && visibility.near_fade && visibility.inputs.valid
         && CameraFadeInputsUsable(visibility.inputs.param[0], visibility.inputs.param[1]);
}

inline void FillInstanceVisibility(const PoolCameraVisibility& visibility, float out[4]) {
  out[0] = 0.f;
  out[1] = 0.f;
  out[3] = 0.f;
  uint32_t flags = 0u;
  if (visibility.camera_seen) flags |= kInstanceCameraVisible;
  if (visibility.light_seen) flags |= kInstanceShadowCaster;
  if (CameraFadeApplies(visibility)) {
    flags |= kInstanceNearFade;
    out[0] = visibility.inputs.param[0];
    out[1] = visibility.inputs.param[1];
  }
  out[2] = static_cast<float>(flags);
}

inline uint32_t InstanceVisibilityFlags(const WorldInstanceGPU& instance) {
  return static_cast<uint32_t>(instance.visibility[2] + 0.5f);
}

inline void RebuildLiveTlas(
    reshade::api::device* device,
    BvhDeviceData* data,
    const PoolRegion& region,
    uint32_t frame,
    const char* reason) {
  const auto start = std::chrono::steady_clock::now();
  LiveBvhStats& stats = data->live;
  std::vector<LiveTlasInstance> snapshot;
  uint64_t revision = 0u;
  uint64_t visibility_revision = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    revision = g_pool.revision;
    visibility_revision = g_pool.visibility_revision;
    for (const WorldInstance& instance : g_pool.instances) {
      if (!PoolInstanceInRegion(instance, region) || instance.mesh_id >= g_pool.meshes.size()) continue;
      LiveTlasInstance& copy = snapshot.emplace_back();
      copy.uid = g_pool.meshes[instance.mesh_id].uid;
      copy.mesh_key = instance.mesh_key;
      copy.vs_hash = instance.source_vs_hash;
      copy.first_frame = instance.first_frame;
      copy.admit_frame = instance.admit_frame;
      copy.admit_camera_valid = instance.admit_camera_valid;
      std::memcpy(copy.admit_camera, instance.admit_camera, sizeof(copy.admit_camera));
      copy.source = instance.source;
      copy.matrix_floats = instance.matrix_floats;
      std::memcpy(copy.matrix, instance.matrix, sizeof(copy.matrix));
      std::memcpy(copy.inverse_world, instance.inverse_world, sizeof(copy.inverse_world));
      std::memcpy(copy.bounds_min, instance.bounds_min, sizeof(copy.bounds_min));
      std::memcpy(copy.bounds_max, instance.bounds_max, sizeof(copy.bounds_max));
      copy.visibility = GetPoolCameraVisibility(instance);
    }
  }
  // What this TLAS reflects; recorded only once it is in place, so a failed
  // build is retried (after kLiveRetryFrames).
  const uint64_t store_version = data->store_version;
  const auto commit = [&]() {
    data->tlas_pool_revision = revision;
    data->tlas_visibility_revision = visibility_revision;
    data->tlas_store_version = store_version;
    data->tlas_region_size = region.size;
    std::memcpy(data->tlas_region_min, region.min, sizeof(data->tlas_region_min));
  };
  stats.tlas_frame = frame;
  stats.tlas_reason = reason;
  stats.tlas_rebuilds += 1u;

  std::vector<WorldInstanceGPU> instances;
  std::vector<LiveTlasInstance> info;
  std::vector<BVHLeafGPU> leaves;
  instances.reserve(snapshot.size());
  info.reserve(snapshot.size());
  leaves.reserve(snapshot.size());
  uint32_t waiting = 0u;
  uint32_t unusable = 0u;
  uint32_t near_fade = 0u;
  uint32_t camera_hidden = 0u;
  for (const LiveTlasInstance& instance : snapshot) {
    const auto slot_it = data->slot_by_uid.find(instance.uid);
    if (slot_it == data->slot_by_uid.end() || slot_it->second >= data->descriptor_count) {
      if (data->failed_uids.count(instance.uid) != 0u) {
        unusable += 1u;
      } else {
        waiting += 1u;
      }
      continue;
    }
    WorldInstanceGPU descriptor = {};
    descriptor.header[0] = static_cast<float>(slot_it->second);
    descriptor.header[1] = static_cast<float>(instance.source);
    const uint32_t floats = instance.matrix_floats == 0u ? 12u : (std::min)(instance.matrix_floats, 16u);
    float matrix[16] = {};
    std::memcpy(matrix, instance.matrix, sizeof(float) * floats);
    // The engine's vertex transform uses only rows 0..2; the descriptor is
    // explicitly affine so it always matches the computed inverse.
    matrix[12] = 0.f;
    matrix[13] = 0.f;
    matrix[14] = 0.f;
    matrix[15] = 1.f;
    std::memcpy(descriptor.world, matrix, sizeof(matrix));
    std::memcpy(descriptor.inverse_world, instance.inverse_world, sizeof(descriptor.inverse_world));
    for (int k = 0; k < 3; ++k) {
      descriptor.bounds_min[k] = instance.bounds_min[k];
      descriptor.bounds_max[k] = instance.bounds_max[k];
    }
    FillInstanceVisibility(instance.visibility, descriptor.visibility);
    const uint32_t flags = InstanceVisibilityFlags(descriptor);
    if ((flags & kInstanceNearFade) != 0u) near_fade += 1u;
    if ((flags & kInstanceCameraVisible) == 0u) camera_hidden += 1u;
    leaves.push_back(MakeLeaf(instance.bounds_min, instance.bounds_max, static_cast<uint32_t>(instances.size())));
    instances.push_back(descriptor);
    info.push_back(instance);
  }
  stats.tlas_waiting = waiting;
  stats.tlas_unusable = unusable;

  const auto finish = [&]() {
    stats.tlas_ms_last = LiveElapsedMs(start);
    stats.tlas_ms_max = (std::max)(stats.tlas_ms_max, stats.tlas_ms_last);
  };
  if (instances.empty()) {
    DestroyLiveTlas(device, data);
    commit();
    stats.tlas_instances = 0u;
    stats.tlas_near_fade = 0u;
    stats.tlas_camera_hidden = 0u;
    stats.tlas_nodes = 0u;
    finish();
    return;
  }

  SortBvhLeaves(&leaves);
  std::vector<BVHNodeGPU> nodes;
  const bool built = BuildBvhTree(leaves, &nodes);
  const char* invalid = built ? DescribeBvhValidation(ValidateBvh(nodes, leaves)) : "tree construction failed";
  if (invalid != nullptr) {
    stats.tlas_invalid += 1u;
    stats.last_failure = std::string("TLAS: ") + invalid + " (previous TLAS kept)";
    renodx::utils::log::w("[world-bvh] TLAS over ", leaves.size(), " instances not uploaded: ", invalid);
    data->tlas_retry_frame = frame + kLiveRetryFrames;
    finish();
    return;
  }

  std::vector<uint32_t> active(instances.size());
  for (uint32_t i = 0; i < active.size(); ++i) active[i] = i;
  const auto usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source;
  struct NewBuffer {
    reshade::api::resource buffer = {0u};
    reshade::api::resource_view view = {0u};
  };
  NewBuffer instance_buffer;
  NewBuffer active_buffer;
  NewBuffer leaf_buffer;
  NewBuffer node_buffer;
  std::string error;
  const bool created =
      CreatePoolBuffer(device, instances.data(), instances.size() * sizeof(WorldInstanceGPU), sizeof(WorldInstanceGPU),
                       &instance_buffer.buffer, &instance_buffer.view, usage, &error)
      && CreatePoolBuffer(device, active.data(), active.size() * sizeof(uint32_t), sizeof(uint32_t),
                          &active_buffer.buffer, &active_buffer.view, usage, &error)
      && CreatePoolBuffer(device, leaves.data(), leaves.size() * sizeof(BVHLeafGPU), sizeof(BVHLeafGPU),
                          &leaf_buffer.buffer, &leaf_buffer.view, usage, &error)
      && CreatePoolBuffer(device, nodes.data(), nodes.size() * sizeof(BVHNodeGPU), sizeof(BVHNodeGPU),
                          &node_buffer.buffer, &node_buffer.view, usage, &error);
  if (!created) {
    DestroyBuffer(device, &instance_buffer.view, &instance_buffer.buffer);
    DestroyBuffer(device, &active_buffer.view, &active_buffer.buffer);
    DestroyBuffer(device, &leaf_buffer.view, &leaf_buffer.buffer);
    DestroyBuffer(device, &node_buffer.view, &node_buffer.buffer);
    stats.upload_failures += 1u;
    stats.last_failure = "TLAS buffer creation failed: " + error + " (previous TLAS kept)";
    renodx::utils::log::w("[world-bvh] ", stats.last_failure);
    data->tlas_retry_frame = frame + kLiveRetryFrames;
    finish();
    return;
  }
  DestroyLiveTlas(device, data);
  data->instance_buffer = instance_buffer.buffer;
  data->instance_srv = instance_buffer.view;
  data->active_buffer = active_buffer.buffer;
  data->active_srv = active_buffer.view;
  data->tlas_leaf_buffer = leaf_buffer.buffer;
  data->tlas_leaf_srv = leaf_buffer.view;
  data->tlas_node_buffer = node_buffer.buffer;
  data->tlas_node_srv = node_buffer.view;
  data->active_count = static_cast<uint32_t>(instances.size());
  data->tlas_instances_cpu = std::move(instances);
  data->tlas_info = std::move(info);
  data->tlas_leaves_cpu = std::move(leaves);
  data->tlas_nodes_cpu = std::move(nodes);
  data->tlas_built = true;
  commit();
  data->bvh_ready = data->mesh_srv.handle != 0u && data->vertices.srv.handle != 0u && data->indices.srv.handle != 0u
                    && data->blas_leaves.srv.handle != 0u && data->blas_nodes.srv.handle != 0u;
  stats.tlas_instances = data->active_count;
  stats.tlas_near_fade = near_fade;
  stats.tlas_camera_hidden = camera_hidden;
  stats.tlas_nodes = static_cast<uint32_t>(data->tlas_nodes_cpu.size());
  finish();
}

// ---------------------------------------------------------------------------
// GPU contents check (on request; waits for the GPU).

inline bool ReadLiveBytes(
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    reshade::api::resource resource,
    uint64_t size,
    std::vector<uint8_t>* out) {
  out->clear();
  if (resource.handle == 0u) return size == 0u;
  out->reserve(size);
  std::vector<uint8_t> chunk;
  for (uint64_t offset = 0u; offset < size; offset += kLiveReadbackChunk) {
    const uint64_t bytes = (std::min)(kLiveReadbackChunk, size - offset);
    if (!renodx::utils::scene::ReadbackBuffer(device, queue, resource, offset, bytes, &chunk) || chunk.size() != bytes) {
      return false;
    }
    out->insert(out->end(), chunk.begin(), chunk.end());
  }
  return true;
}

template <typename T>
inline std::vector<T> LiveBytesAs(const std::vector<uint8_t>& bytes, uint64_t first, uint64_t count) {
  std::vector<T> values(count);
  if (count != 0u) std::memcpy(values.data(), bytes.data() + first * sizeof(T), count * sizeof(T));
  return values;
}

// Reads every live buffer back and compares it with what the store uploaded:
// each live mesh's vertices, indices, BLAS leaves and nodes are rebuilt from
// the pool's copy of the mesh (the build is deterministic), the descriptors
// and the TLAS with the CPU copies kept at upload; read-back trees are also
// validated structurally.
inline void CheckLiveBvhOnGpu(reshade::api::device* device, reshade::api::command_queue* queue, BvhDeviceData* data) {
  LiveBvhStats& stats = data->live;
  stats.gpu_checked = true;
  stats.gpu_ok = false;
  std::vector<uint8_t> vertices;
  std::vector<uint8_t> indices;
  std::vector<uint8_t> leaves;
  std::vector<uint8_t> nodes;
  std::vector<uint8_t> descriptors;
  std::vector<uint8_t> instances;
  std::vector<uint8_t> active;
  std::vector<uint8_t> tlas_leaves;
  std::vector<uint8_t> tlas_nodes;
  const bool read =
      ReadLiveBytes(device, queue, data->vertices.buffer, data->vertices.used * data->vertices.stride, &vertices)
      && ReadLiveBytes(device, queue, data->indices.buffer, data->indices.used * data->indices.stride, &indices)
      && ReadLiveBytes(device, queue, data->blas_leaves.buffer, data->blas_leaves.used * data->blas_leaves.stride, &leaves)
      && ReadLiveBytes(device, queue, data->blas_nodes.buffer, data->blas_nodes.used * data->blas_nodes.stride, &nodes)
      && ReadLiveBytes(device, queue, data->mesh_buffer, data->descriptor_count * sizeof(WorldMeshGPU), &descriptors)
      && ReadLiveBytes(device, queue, data->instance_buffer, data->active_count * sizeof(WorldInstanceGPU), &instances)
      && ReadLiveBytes(device, queue, data->active_buffer, data->active_count * sizeof(uint32_t), &active)
      && ReadLiveBytes(device, queue, data->tlas_leaf_buffer, data->tlas_leaves_cpu.size() * sizeof(BVHLeafGPU), &tlas_leaves)
      && ReadLiveBytes(device, queue, data->tlas_node_buffer, data->tlas_nodes_cpu.size() * sizeof(BVHNodeGPU), &tlas_nodes);
  if (!read) {
    stats.gpu_result = "readback failed";
    renodx::utils::log::w("[world-bvh] GPU check: readback failed");
    return;
  }

  // The pool's copy of every live mesh.
  std::unordered_map<uint64_t, LiveMeshUpload> expected;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    for (const WorldMesh& mesh : g_pool.meshes) {
      if (data->slot_by_uid.count(mesh.uid) == 0u) continue;
      LiveMeshUpload& copy = expected[mesh.uid];
      copy.uid = mesh.uid;
      copy.positions = mesh.positions;
      copy.indices = mesh.indices;
    }
  }

  const auto range_equal = [](const std::vector<uint8_t>& bytes, uint64_t offset, const void* values, uint64_t size) {
    return offset + size <= bytes.size() && (size == 0u || std::memcmp(bytes.data() + offset, values, size) == 0);
  };
  uint32_t meshes_checked = 0u;
  uint32_t meshes_bad = 0u;
  uint32_t trees_bad = 0u;
  uint32_t meshes_gone = 0u;  // left the pool since the last mesh scan
  for (const LiveMeshSlot& slot : data->slots) {
    if (!slot.live) continue;
    const auto source = expected.find(slot.uid);
    if (source == expected.end()) {
      meshes_gone += 1u;
      continue;
    }
    meshes_checked += 1u;
    MeshBlas blas;
    std::vector<float> vertex_values;
    vertex_values.reserve(source->second.positions.size() * 4u);
    for (const auto& position : source->second.positions) {
      vertex_values.insert(vertex_values.end(), {position[0], position[1], position[2], 1.f});
    }
    const bool equal =
        BuildMeshBlas(source->second.positions, source->second.indices, &blas) == nullptr
        && blas.leaves.size() == slot.leaf_count && blas.nodes.size() == slot.node_count
        && vertex_values.size() == static_cast<size_t>(slot.vertex_count) * 4u
        && source->second.indices.size() == slot.index_count
        && range_equal(vertices, static_cast<uint64_t>(slot.vertex_offset) * data->vertices.stride, vertex_values.data(),
                       vertex_values.size() * sizeof(float))
        && range_equal(indices, static_cast<uint64_t>(slot.index_offset) * data->indices.stride, source->second.indices.data(),
                       source->second.indices.size() * sizeof(uint32_t))
        && range_equal(leaves, static_cast<uint64_t>(slot.leaf_offset) * data->blas_leaves.stride, blas.leaves.data(),
                       blas.leaves.size() * sizeof(BVHLeafGPU))
        && range_equal(nodes, static_cast<uint64_t>(slot.node_offset) * data->blas_nodes.stride, blas.nodes.data(),
                       blas.nodes.size() * sizeof(BVHNodeGPU));
    if (!equal) meshes_bad += 1u;
    if (static_cast<uint64_t>(slot.node_offset) + slot.node_count <= data->blas_nodes.used
        && static_cast<uint64_t>(slot.leaf_offset) + slot.leaf_count <= data->blas_leaves.used) {
      const auto slot_nodes = LiveBytesAs<BVHNodeGPU>(nodes, slot.node_offset, slot.node_count);
      const auto slot_leaves = LiveBytesAs<BVHLeafGPU>(leaves, slot.leaf_offset, slot.leaf_count);
      if (!ValidateBvh(slot_nodes, slot_leaves).ok) trees_bad += 1u;
    } else {
      trees_bad += 1u;
    }
  }
  const bool descriptors_ok =
      data->descriptor_count <= data->mesh_descriptors.size()
      && range_equal(descriptors, 0u, data->mesh_descriptors.data(), data->descriptor_count * sizeof(WorldMeshGPU));
  bool active_ok = true;
  for (uint32_t i = 0; i < data->active_count && active_ok; ++i) {
    uint32_t value = 0u;
    std::memcpy(&value, active.data() + static_cast<size_t>(i) * sizeof(uint32_t), sizeof(value));
    active_ok = value == i;
  }
  const bool tlas_ok =
      active_ok && data->tlas_instances_cpu.size() == data->active_count
      && range_equal(instances, 0u, data->tlas_instances_cpu.data(), data->tlas_instances_cpu.size() * sizeof(WorldInstanceGPU))
      && range_equal(tlas_leaves, 0u, data->tlas_leaves_cpu.data(), data->tlas_leaves_cpu.size() * sizeof(BVHLeafGPU))
      && range_equal(tlas_nodes, 0u, data->tlas_nodes_cpu.data(), data->tlas_nodes_cpu.size() * sizeof(BVHNodeGPU));
  const bool tlas_tree_ok =
      ValidateBvh(LiveBytesAs<BVHNodeGPU>(tlas_nodes, 0u, tlas_nodes.size() / sizeof(BVHNodeGPU)),
                  LiveBytesAs<BVHLeafGPU>(tlas_leaves, 0u, tlas_leaves.size() / sizeof(BVHLeafGPU)))
          .ok;
  stats.gpu_ok = meshes_bad == 0u && trees_bad == 0u && descriptors_ok && tlas_ok && tlas_tree_ok;
  stats.gpu_result = std::string(stats.gpu_ok ? "match" : "MISMATCH") + ": meshes " + std::to_string(meshes_checked - meshes_bad)
                     + "/" + std::to_string(meshes_checked) + " equal, " + std::to_string(meshes_checked - trees_bad)
                     + " valid trees; descriptors " + (descriptors_ok ? "equal" : "DIFFER") + "; TLAS ("
                     + std::to_string(data->active_count) + " instances) " + (tlas_ok ? "equal" : "DIFFERS") + ", tree "
                     + (tlas_tree_ok ? "valid" : "INVALID");
  if (meshes_gone != 0u) stats.gpu_result += "; " + std::to_string(meshes_gone) + " meshes left the pool, not checked";
  renodx::utils::log::i("[world-bvh] GPU check: ", stats.gpu_result);
}

// ---------------------------------------------------------------------------
// Inspect (middle-click in a trace view): what the BVH hit at one pixel and
// where that instance and its mesh came from.

inline const char* LiveCompareClassName(uint32_t stat) {
  switch (stat) {
    case 7u:  return "match";
    case 8u:  return "missing";
    case 9u:  return "extra (BVH in front)";
    case 10u: return "extra on sky";
    case 11u: return "beyond range";
    case 12u: return "sky";
    case 13u: return "no game depth";
    default:  return nullptr;
  }
}

template <typename... Args>
inline std::string LiveFormat(const char* format, Args... args) {
  char text[512] = {};
  std::snprintf(text, sizeof(text), format, args...);
  return text;
}

// Inspect lines about the game camera's view of the hit instance, from what
// the TLAS was built with (GetPoolCameraVisibility, camera_fade.hpp).
inline void DescribeLiveCameraView(const LiveTlasInstance& info, BvhInspect& inspect, bool scene_fade, float near_fade_floor,
                                   float map_alpha) {
  const PoolCameraVisibility& visibility = info.visibility;
  if (!visibility.camera_seen) {
    inspect.lines.push_back(LiveFormat(
        "Game camera: NEVER drawn by a camera vertex shader (%s); hidden from camera views when hiding is on.",
        visibility.light_seen ? "only drawn into shadow maps: a shadow-only caster for the game, or its camera draws are "
                                "alpha-tested / blended, which the pool does not admit"
                              : "no view recorded"));
  } else {
    inspect.lines.push_back(LiveFormat("Game camera: drawn by camera vertex shaders (%u draws); shadow caster: %s.",
                                       visibility.camera_sightings, visibility.light_seen ? "yes" : "not seen"));
  }
  const std::string scene = scene_fade ? LiveFormat("scene near-fade floor %.2f, map alpha %.2f", near_fade_floor, map_alpha)
                                       : std::string("scene fade constants not captured yet (the trace applies no near fade until they are)");
  if (!visibility.inputs.valid) {
    inspect.lines.push_back("Near fade: no inputs (the drawing VS lacks the contract color/param layout); " + scene + ".");
  } else {
    const float* param = visibility.inputs.param;
    const float full = param[1] != 0.f ? param[0] + 1.f / param[1] : param[0];
    const std::string ps = !visibility.camera_seen
                               ? std::string("no camera draws")
                               : LiveFormat("%u of %u camera draws near-fade (latest PS 0x%08X)",
                                            visibility.camera_near_fade_sightings, visibility.camera_sightings,
                                            visibility.camera_ps_hash);
    inspect.lines.push_back(LiveFormat("Near fade: from %.2f m, full at %.2f m (1/range %.4g), opacity %.2f, dither flip %s; inputs from %s; ",
                                       param[0], full, param[1], visibility.inputs.color[3], param[2] > 0.f ? "on" : "off",
                                       visibility.source)
                            + ps + "; " + scene + ".");
    if (inspect.hit && visibility.camera_seen) {
      const float fade = CameraNearFade(inspect.t, param[0], param[1], scene_fade ? near_fade_floor : 0.f);
      const bool applies = CameraFadeApplies(visibility);
      inspect.lines.push_back(LiveFormat("At this hit (%.2f m from the camera) the near-fade factor is %.2f%s: the game %s this surface%s.",
                                         inspect.t, fade, scene_fade ? "" : " (assuming floor 0)",
                                         fade >= kCameraFadeShown ? "draws" : "HIDES",
                                         applies ? "" : " (the near fade does not apply to this instance)"));
    }
  }
  if (inspect.camera_hidden) {
    inspect.lines.push_back(inspect.hiding
                                ? "Trace: a surface the game camera does not show, in front of this hit, was skipped (hiding is on)."
                                : "Trace: the game camera does not show this surface (hiding is off).");
  }
}

inline void DescribeLiveInspect(BvhDeviceData* data) {
  BvhInspect& inspect = data->inspect;
  inspect.lines.clear();
  const uint32_t frame = g_state.frame.load();
  const char* compare = LiveCompareClassName(inspect.compare_class);
  const std::string compare_text = compare != nullptr ? std::string("  [depth compare: ") + compare + "]" : "";
  if (!inspect.valid) {
    inspect.lines.push_back(LiveFormat("Pixel (%u, %u): not traced.", inspect.x, inspect.y));
  } else if (!inspect.hit) {
    inspect.lines.push_back(LiveFormat("Pixel (%u, %u): no BVH hit.", inspect.x, inspect.y) + compare_text);
    if (inspect.camera_hidden) {
      inspect.lines.push_back("Trace: a surface the game camera does not show was skipped along this ray (hiding is on); turn hiding off and click again to inspect it.");
    }
  } else if (inspect.instance >= data->tlas_info.size() || inspect.instance >= data->tlas_instances_cpu.size()) {
    inspect.lines.push_back(LiveFormat("Pixel (%u, %u): hit TLAS instance %u, which the current TLAS no longer has (rebuilt since).",
                                       inspect.x, inspect.y, inspect.instance));
  } else {
    const LiveTlasInstance& info = data->tlas_info[inspect.instance];
    const float* m = info.matrix;
    float scale[3] = {};
    for (int k = 0; k < 3; ++k) scale[k] = std::sqrt(m[k] * m[k] + m[4 + k] * m[4 + k] + m[8 + k] * m[8 + k]);
    inspect.lines.push_back(LiveFormat("Pixel (%u, %u): hit at %.2f m, point (%.2f, %.2f, %.2f).", inspect.x, inspect.y,
                                       inspect.t, inspect.position[0], inspect.position[1], inspect.position[2])
                            + compare_text);
    inspect.lines.push_back(LiveFormat("Instance %u: VS 0x%08X, position (%.2f, %.2f, %.2f), axis scales (%.3f, %.3f, %.3f).",
                                       inspect.instance, info.vs_hash, m[3], m[7], m[11], scale[0], scale[1], scale[2]));

    bool scene_fade = false;
    float near_fade_floor = 0.f;
    float map_alpha = 1.f;
    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      scene_fade = g_state.camera.has_fade;
      near_fade_floor = g_state.camera.near_fade_floor;
      map_alpha = g_state.camera.map_alpha;
    }

    // Pool state now (the instance may have changed since the TLAS was built).
    bool observed = false;
    ObservedInstance observation;
    bool mesh_found = false;
    WorldMesh mesh;  // copied without geometry
    size_t vertex_count = 0u;
    PoolStats stats;
    {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      const auto it = g_pool.observations.find(InstanceKey{info.mesh_key, MatrixHash(info.matrix, kPoolWorldFloats)});
      if (it != g_pool.observations.end()) {
        observed = true;
        observation = it->second;
      }
      for (const WorldMesh& candidate : g_pool.meshes) {
        if (candidate.uid != info.uid) continue;
        mesh_found = true;
        mesh.uid = candidate.uid;
        mesh.source_vs_hash = candidate.source_vs_hash;
        mesh.triangle_count = candidate.triangle_count;
        std::memcpy(mesh.bbox_min, candidate.bbox_min, sizeof(mesh.bbox_min));
        std::memcpy(mesh.bbox_max, candidate.bbox_max, sizeof(mesh.bbox_max));
        mesh.live_keys = candidate.live_keys;
        mesh.capture_frame = candidate.capture_frame;
        mesh.source_vb = candidate.source_vb;
        mesh.source_ib = candidate.source_ib;
        mesh.from_indirect = candidate.from_indirect;
        mesh.writes_after_capture = candidate.writes_after_capture;
        mesh.camera_draws = candidate.camera_draws;
        mesh.camera_near_fade_draws = candidate.camera_near_fade_draws;
        mesh.light_draws = candidate.light_draws;
        mesh.camera_ps_hash = candidate.camera_ps_hash;
        vertex_count = candidate.positions.size();
        break;
      }
      stats = g_pool.stats;
    }
    if (observed) {
      inspect.lines.push_back(LiveFormat("Seen in %u frames: first %u, admitted %u, last %u (now %u, %u frames ago).",
                                         observation.count, info.first_frame, info.admit_frame, observation.last_frame, frame,
                                         frame >= observation.last_frame ? frame - observation.last_frame : 0u));
    } else {
      inspect.lines.push_back(LiveFormat("No longer observed by the pool (retired since). First seen %u, admitted %u.",
                                         info.first_frame, info.admit_frame));
    }
    if (observed) {
      inspect.lines.push_back(LiveFormat("Views now: camera VS %u frames (last %u; %u of %u draws with a near-fading PS, latest 0x%08X), light VS (shadow maps) %u frames (last %u).",
                                         observation.camera_frames, observation.last_camera_frame,
                                         observation.camera_near_fade_sightings, observation.camera_sightings,
                                         observation.camera_ps_hash, observation.light_frames, observation.last_light_frame));
    }
    DescribeLiveCameraView(info, inspect, scene_fade, near_fade_floor, map_alpha);
    if (info.admit_camera_valid) {
      const float d[3] = {m[3] - info.admit_camera[0], m[7] - info.admit_camera[1], m[11] - info.admit_camera[2]};
      inspect.lines.push_back(LiveFormat("Camera at admission (%.2f, %.2f, %.2f), %.2f m from the instance origin.",
                                         info.admit_camera[0], info.admit_camera[1], info.admit_camera[2],
                                         std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2])));
    } else {
      inspect.lines.push_back("Camera at admission unknown.");
    }
    if (mesh_found) {
      float smallest = 1e30f;
      for (int k = 0; k < 3; ++k) smallest = (std::min)(smallest, mesh.bbox_max[k] - mesh.bbox_min[k]);
      inspect.lines.push_back(LiveFormat("Mesh uid %llu (GPU slot %u): %u triangles, %llu vertices, local bbox (%.2f, %.2f, %.2f) .. (%.2f, %.2f, %.2f), thinnest extent %.3f.",
                                         static_cast<unsigned long long>(mesh.uid), inspect.mesh, mesh.triangle_count,
                                         static_cast<unsigned long long>(vertex_count), mesh.bbox_min[0], mesh.bbox_min[1],
                                         mesh.bbox_min[2], mesh.bbox_max[0], mesh.bbox_max[1], mesh.bbox_max[2], smallest));
      inspect.lines.push_back(LiveFormat("Mesh captured at frame %u from VB 0x%llX IB 0x%llX (%s draw, VS 0x%08X); draw keys %u; game writes to those buffers since: %u.",
                                         mesh.capture_frame, static_cast<unsigned long long>(mesh.source_vb),
                                         static_cast<unsigned long long>(mesh.source_ib), mesh.from_indirect ? "indirect" : "direct",
                                         mesh.source_vs_hash, mesh.live_keys, mesh.writes_after_capture));
      inspect.lines.push_back(LiveFormat("Mesh draws sampled (all its instances): camera VS %u (%u with a near-fading PS; latest PS 0x%08X), light VS %u.",
                                         mesh.camera_draws, mesh.camera_near_fade_draws, mesh.camera_ps_hash, mesh.light_draws));
    } else {
      inspect.lines.push_back(LiveFormat("Mesh uid %llu is no longer in the pool (retired since).",
                                         static_cast<unsigned long long>(info.uid)));
    }
    if (stats.mass_retirements != 0u) {
      inspect.lines.push_back(LiveFormat("Last map change (%u meshes retired at once): frame %u; instance admitted %s it.",
                                         stats.last_mass_retire_meshes, stats.last_mass_retire_frame,
                                         info.admit_frame < stats.last_mass_retire_frame ? "BEFORE" : "after"));
    } else {
      inspect.lines.push_back("No map change seen since the pool started.");
    }
  }
  for (const std::string& line : inspect.lines) renodx::utils::log::i("[world-bvh] inspect: ", line);
}

// ---------------------------------------------------------------------------
// Per present.

inline void UpdateLiveBvh(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return;
  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) return;
  PoolStageScope stage("live bvh: reset");
  if (g_live_bvh.reset_requested.exchange(false, std::memory_order_relaxed)) ResetLiveStore(device, data, "requested");

  if (g_live_bvh.enabled.load(std::memory_order_relaxed)) {
    const uint32_t frame = g_state.frame.load();
    uint64_t revision = 0u;
    uint64_t visibility_revision = 0u;
    {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      revision = g_pool.revision;
      visibility_revision = g_pool.visibility_revision;
    }
    stage.Set("live bvh: meshes");
    // Pool changes (retirements) are handled at once; pending uploads wait
    // out a GPU buffer failure.
    if (revision != data->mesh_revision || (data->mesh_scan_needed && frame >= data->upload_retry_frame)) {
      SyncLiveMeshes(device, cmd_list, data, frame);
    }
    stage.Set("live bvh: tlas");
    const PoolRegion region = CurrentPoolRegion();
    const char* change = LiveTlasChange(*data, revision, visibility_revision, region);
    if (change != nullptr && frame >= data->tlas_retry_frame
        && (data->live.tlas_rebuilds == 0u || frame - data->live.tlas_frame >= kLiveTlasInterval || frame < data->live.tlas_frame)) {
      RebuildLiveTlas(device, data, region, frame, change);
    }
  }

  if (g_live_bvh.check_requested.exchange(false, std::memory_order_relaxed)) {
    stage.Set("live bvh: gpu check");
    CheckLiveBvhOnGpu(device, queue, data);
  }
}

}  // namespace falcom_world::bvh
