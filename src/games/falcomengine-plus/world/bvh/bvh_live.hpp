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
// previous TLAS. Dynamic (moving) instances are stable-partitioned last. When
// only their poses changed, RefitLiveTlas rewrites their descriptors, leaf and
// node bounds in place (same leaves and topology); after kLiveTlasRefitsPerRebuild
// refits the TLAS is rebuilt. A failed refit keeps the previous TLAS and forces
// a rebuild on the next update.
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
#include "alpha_live.hpp"
#include "bvh_resources.hpp"
#include "cache_diag.hpp"
#include "perf_run.hpp"
#include "deform_live.hpp"

namespace falcom_world::bvh {

inline constexpr uint64_t kLiveTrianglesPerFrame = 16384u;  // BLAS work per present (at least one mesh)
inline constexpr uint32_t kLiveMeshesPerFrame = 64u;
inline constexpr uint32_t kLiveTlasInterval = 8u;           // frames between TLAS rebuilds
inline constexpr uint32_t kLiveTlasRefitsPerRebuild = 120u;  // refits before a full rebuild (tree quality)
inline constexpr uint64_t kLiveFloatExact = 1ull << 24u;    // mesh descriptor offsets are floats
inline constexpr uint64_t kLiveResetGarbageBytes = 16ull * 1024ull * 1024ull;
inline constexpr uint32_t kLiveRetryFrames = 120u;          // after a GPU buffer creation failure
inline constexpr uint64_t kLiveReadbackChunk = 32ull * 1024ull * 1024ull;
inline constexpr uint32_t kLiveSlackMinSpares = 256u;      // slack TLAS: holes per static block (at least)
inline constexpr uint32_t kLiveSlackSpareDivisor = 8u;    // ... and one per this many static instances
inline constexpr uint32_t kLiveSlackMinStatic = 1024u;    // fewer static instances: no holes
inline constexpr uint64_t kLiveSlackMaxOps = 512u;        // more changes than this: rebuild
inline constexpr uint32_t kLiveSlackMaintenanceFrames = 120u;  // full rebuild at least this often (tree quality)
inline constexpr uint32_t kLiveSlackSentinel = 0xFFFFFFFFu;    // active buffer value of a hole (shader: rejected)
inline constexpr uint64_t kLiveArenaMinBytes = 1024ull * 1024ull;

struct LiveBvhSwitches {
  std::atomic_bool enabled{true};
  std::atomic_bool reset_requested{false};
  std::atomic_bool check_requested{false};
  std::atomic_bool skip_noop{false};  // session only: a rebuild whose snapshot equals the TLAS is skipped (OFF = HEAD; default OFF until the in-game A/B)
  std::atomic_bool slack{false};     // session only: spare slots in the TLAS, changes written in place (OFF = HEAD)
};

inline LiveBvhSwitches g_live_bvh;

inline float LiveElapsedMs(std::chrono::steady_clock::time_point start) {
  if (start == std::chrono::steady_clock::time_point{}) return 0.f;  // gather off (PerfNow)
  return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
}

inline uint64_t LiveArenaBytes(const LiveArena& arena, uint64_t elements) {
  return elements * arena.stride;
}

inline uint64_t LiveSlotBytes(const BvhDeviceData& data, const LiveMeshSlot& slot) {
  return LiveArenaBytes(data.vertices, slot.vertex_count) + LiveArenaBytes(data.indices, slot.index_count)
         + LiveArenaBytes(data.blas_leaves, slot.leaf_count) + LiveArenaBytes(data.blas_nodes, slot.node_count)
         + LiveArenaBytes(data.uvs, slot.uv_count);
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
                     + LiveArenaBytes(data->blas_nodes, data->blas_nodes.used) + LiveArenaBytes(data->uvs, data->uvs.used);
  stats.capacity_bytes = LiveArenaBytes(data->vertices, data->vertices.capacity)
                         + LiveArenaBytes(data->indices, data->indices.capacity)
                         + LiveArenaBytes(data->blas_leaves, data->blas_leaves.capacity)
                         + LiveArenaBytes(data->blas_nodes, data->blas_nodes.capacity)
                         + LiveArenaBytes(data->uvs, data->uvs.capacity);
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
  std::vector<std::array<float, 2>> uvs;  // empty unless the mesh has UVs
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
    upload.uvs = mesh.uvs;
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
  uint64_t uv_total = 0u;
  for (LiveMeshUpload& upload : *uploads) {
    LiveBuiltMesh mesh;
    mesh.upload = &upload;
    const auto blas_start = PerfNow();
    const char* error = BuildMeshBlas(upload.positions, upload.indices, &mesh.blas);
    const float blas_ms = LiveElapsedMs(blas_start);
    stats.blas_builds += 1u;
    stats.blas_build_ms_sum += blas_ms;
    stats.blas_build_ms_max = (std::max)(stats.blas_build_ms_max, blas_ms);
    if (error == nullptr) stats.blas_built_triangles += upload.indices.size() / 3u;
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
        || data->blas_nodes.used + node_total + mesh.blas.nodes.size() > kLiveFloatExact
        || data->uvs.used + uv_total + upload.uvs.size() > kLiveFloatExact) {
      data->store_full = true;
      continue;
    }
    vertex_total += upload.positions.size();
    index_total += upload.indices.size();
    leaf_total += mesh.blas.leaves.size();
    node_total += mesh.blas.nodes.size();
    uv_total += upload.uvs.size();
    built.push_back(std::move(mesh));
  }
  if (data->store_full) stats.last_failure = "store full: offsets would exceed 2^24 elements";
  if (built.empty()) return result;

  std::string error;
  if (!EnsureLiveArena(device, cmd_list, data, &data->vertices, data->vertices.used + vertex_total, &error)
      || !EnsureLiveArena(device, cmd_list, data, &data->indices, data->indices.used + index_total, &error)
      || !EnsureLiveArena(device, cmd_list, data, &data->blas_leaves, data->blas_leaves.used + leaf_total, &error)
      || !EnsureLiveArena(device, cmd_list, data, &data->blas_nodes, data->blas_nodes.used + node_total, &error)
      || (uv_total != 0u && !EnsureLiveArena(device, cmd_list, data, &data->uvs, data->uvs.used + uv_total, &error))) {
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
  std::vector<std::array<float, 2>> uv_batch;
  uv_batch.reserve(uv_total);
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
    if (!upload.uvs.empty()) {
      slot.uv_offset = static_cast<uint32_t>(data->uvs.used + uv_batch.size());
      slot.uv_count = static_cast<uint32_t>(upload.uvs.size());
      uv_batch.insert(uv_batch.end(), upload.uvs.begin(), upload.uvs.end());
    }

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
      || !append(&data->blas_nodes, node_batch.data(), node_batch.size())
      || (!uv_batch.empty() && !append(&data->uvs, uv_batch.data(), uv_batch.size()))) {
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
  data->uvs.used += uv_batch.size();

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
    // bbox_min.w: uv_offset + 1 for a mesh with UVs, 0 for none (the trace reads UVs only when it is set).
    if (slot.uv_count != 0u) descriptor.bbox_min[3] = static_cast<float>(slot.uv_offset + 1u);
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
  const auto start = PerfNow();
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

// Refit every present while swayed wind instances are in the TLAS (the sway moves them every frame).
inline bool WindSwayPending(const BvhDeviceData& data) { return data.tlas_sway_on && data.tlas_wind_count != 0u; }

// P2b: flagged meshes leave the TLAS only with the switch on and their GPU objects created (draws record from then on).
inline bool LiveDynamicActive(const BvhDeviceData& data) {
  return g_pool.dynamic_per_frame.load(std::memory_order_relaxed) && data.dynamic.ready;
}

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
  if (data.tlas_alpha_mode != AlphaMode()) return "alpha switch";
  if (data.tlas_sway_on != g_pool.alpha_wind_sway.load(std::memory_order_relaxed)) return "sway switch";
  if (data.tlas_slack != g_live_bvh.slack.load(std::memory_order_relaxed)) return "slack switch";
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

// Caller holds g_pool.mutex. The trace pose of a wind rest-pose instance: its rest pose (copy already holds it)
// with the translation moved by the sway at its origin (WindSwayDisplacement, weighted 1/(flexibility+1)), capped at
// kWindSwayMaxFraction of the leaf extent or kWindSwayMaxMeters, and the leaf bounds inflated by the capped amplitude
// bound (WindSwayBound). The rest pose stays in WorldInstance (the observation key). Rest pose when the switch is off
// or its key has no sway inputs.
inline void ApplyPoolWindSway(LiveTlasInstance* copy, const WorldInstance& instance) {
  copy->wind = false;
  if (!g_pool.alpha_wind_sway.load(std::memory_order_relaxed)) return;
  const bool wind_key = g_pool.wind_keys.count(instance.mesh_key) != 0u;
  PoolWindPass& pass = g_pool.stats.wind_pass;
  if (wind_key) pass.wind_key_instances += 1u;
  const auto sway = g_pool.wind_sway.find(instance.mesh_key);
  if (wind_key && sway == g_pool.wind_sway.end()) {
    if (pass.no_inputs == 0u) pass.no_input_first_key = instance.mesh_key;
    pass.no_inputs += 1u;
  }
  if (sway == g_pool.wind_sway.end() || !wind_key) return;
  copy->wind = true;
  const float origin[3] = {instance.matrix[3], instance.matrix[7], instance.matrix[11]};
  float extent = 0.f;
  for (int k = 0; k < 3; ++k) extent = (std::max)(extent, copy->bounds_max[k] - copy->bounds_min[k]);
  const float cap = (std::min)(kWindSwayMaxFraction * extent, kWindSwayMaxMeters);
  float displacement[3] = {};
  float bound[3] = {};
  WindSwayDisplacement(sway->second, origin, instance.param2, displacement);
  WindSwayBound(sway->second, instance.param2, bound);
  const float raw_magnitude = std::sqrt(displacement[0] * displacement[0] + displacement[1] * displacement[1] + displacement[2] * displacement[2]);
  for (int k = 0; k < 3; ++k) {
    if (!std::isfinite(displacement[k]) || !std::isfinite(bound[k]) || !std::isfinite(origin[k] + displacement[k])) {
      g_pool.stats.wind_sway_skipped_nonfinite += 1u;
      pass.skipped_nonfinite += 1u;
      return;  // rest pose kept (the instance stays in the wind partition)
    }
    if (std::fabs(origin[k] + displacement[k]) > kPoolMaxPosition || bound[k] > kPoolMaxPosition) {
      g_pool.stats.wind_sway_skipped_range += 1u;
      pass.skipped_range += 1u;
      return;
    }
  }
  const float scale = raw_magnitude > cap ? cap / raw_magnitude : 1.f;
  for (int k = 0; k < 3; ++k) {
    displacement[k] *= scale;
    bound[k] = (std::min)(bound[k], cap);
    copy->matrix[3 + 4 * k] = origin[k] + displacement[k];
    copy->bounds_min[k] -= bound[k];
    copy->bounds_max[k] += bound[k];
  }
  ComputeMatrixInverse(copy->matrix, copy->inverse_world);
  const float magnitude = std::sqrt(displacement[0] * displacement[0] + displacement[1] * displacement[1] + displacement[2] * displacement[2]);
  g_pool.stats.wind_sway_instances += 1u;
  g_pool.stats.wind_sway_max_m = (std::max)(g_pool.stats.wind_sway_max_m, magnitude);
  const float weight = WindSwayWeight(sway->second);
  pass.applied += 1u;
  if (magnitude < 1e-4f) pass.zero_disp += 1u;
  if (raw_magnitude > cap) pass.clamped += 1u;
  if (cap > 0.f) pass.clamp_ratio_max = (std::max)(pass.clamp_ratio_max, raw_magnitude / cap);
  pass.weight_min = (std::min)(pass.weight_min, weight);
  pass.weight_max = (std::max)(pass.weight_max, weight);
  pass.raw_max_m = (std::max)(pass.raw_max_m, raw_magnitude);
  if (pass.applied <= std::size(pass.samples)) {
    PoolWindSample& sample = pass.samples[pass.applied - 1u];
    sample.id = instance.id;
    sample.mesh_key = instance.mesh_key;
    std::memcpy(sample.origin, origin, sizeof(origin));
    sample.params = sway->second;
    std::memcpy(sample.param2, instance.param2, sizeof(sample.param2));
    std::memcpy(sample.displacement, displacement, sizeof(displacement));
    sample.magnitude = magnitude;
    std::memcpy(sample.bound, bound, sizeof(bound));
    sample.weight = weight;
    sample.cap = cap;
    sample.raw_magnitude = raw_magnitude;
    sample.extent = extent;
  }
  for (int k = 0; k < 3; ++k) g_pool.stats.wind_sway_bound_m = (std::max)(g_pool.stats.wind_sway_bound_m, bound[k]);
}

// The TLAS descriptor of one instance, for the mesh slot its uid resolves to.
inline WorldInstanceGPU MakeLiveInstanceGPU(const LiveTlasInstance& instance, uint32_t slot) {
  WorldInstanceGPU descriptor = {};
  descriptor.header[0] = static_cast<float>(slot);
  descriptor.header[1] = static_cast<float>(instance.source);
  descriptor.header[2] = static_cast<float>(instance.material);
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
  return descriptor;
}

// Pure: true iff the TLAS in `data` holds `instances` (with `info`) as a rebuild would build it now: the same ids in the same order
// and the same descriptors. The static prefix is compared exactly; the dynamic suffix by its header and flags, or exactly when
// `dynamic_poses` (their poses are refitted every present, so a pose change alone is not a rebuild).
inline bool LiveSnapshotMatchesTlas(const BvhDeviceData& data, const std::vector<WorldInstanceGPU>& instances,
                                    const std::vector<LiveTlasInstance>& info, bool dynamic_poses) {
  if (!data.tlas_built || data.tlas_instances_cpu.size() != instances.size() || data.tlas_info.size() != info.size()) return false;
  const uint32_t df =
      static_cast<uint32_t>(std::count_if(info.begin(), info.end(), [](const LiveTlasInstance& instance) { return !instance.dynamic; }));
  if (df != data.tlas_dynamic_first) return false;
  for (size_t i = 0; i < info.size(); ++i) {
    if (info[i].id != data.tlas_info[i].id) return false;
    if (i < df || dynamic_poses) {
      if (std::memcmp(&instances[i], &data.tlas_instances_cpu[i], sizeof(WorldInstanceGPU)) != 0) return false;
    } else if (std::memcmp(instances[i].header, data.tlas_instances_cpu[i].header, 3u * sizeof(float)) != 0
               || std::memcmp(instances[i].visibility, data.tlas_instances_cpu[i].visibility, sizeof(instances[i].visibility)) != 0
               || info[i].dynamic != data.tlas_info[i].dynamic || info[i].wind != data.tlas_info[i].wind) {
      return false;
    }
  }
  return true;
}

// Slack (session switch): the static block of a slack TLAS has holes. A change of the static instances is written in place: a
// removed instance parks its slot (zero descriptor, degenerate leaf at its centre, sentinel in the active buffer), an added one
// takes the hole whose anchor is nearest, a changed one rewrites its descriptor and leaf box. The nodes are refitted and
// validated on the CPU before anything is written. Returns false with `why` set when a full rebuild must run instead. With
// `ops_out` 0 the static block already equals the TLAS (nothing to write: the caller counts a skip).
inline bool TryApplyLiveTlasSlack(reshade::api::device* device, reshade::api::command_list* cmd_list, BvhDeviceData* data, uint32_t frame,
                                  const std::vector<WorldInstanceGPU>& instances, const std::vector<LiveTlasInstance>& info,
                                  const char** why, uint32_t* ops_out) {
  const auto start = PerfNow();
  LiveBvhStats& stats = data->live;
  float phase_ms[4] = {};
  auto phase_start = start;
  const auto lap = [&](float* ms) {
    *ms = LiveElapsedMs(phase_start);
    phase_start = PerfNow();
  };
  const auto refuse = [&](const char* reason, uint32_t* counter) {
    *counter += 1u;
    *why = reason;
    return false;
  };
  if (frame < stats.tlas_frame || frame - stats.tlas_frame >= kLiveSlackMaintenanceFrames
      || data->tlas_refits_since_rebuild >= kLiveTlasRefitsPerRebuild) {
    return refuse("slack maintenance", &stats.slack_maintenance);
  }
  // The dynamic suffix is refitted, not rewritten: it must be the TLAS's own (same ids, flags and header).
  const uint32_t df = data->tlas_dynamic_first;
  const uint32_t static_count = static_cast<uint32_t>(
      std::count_if(info.begin(), info.end(), [](const LiveTlasInstance& instance) { return !instance.dynamic; }));
  const size_t suffix = instances.size() - static_count;
  if (suffix != data->tlas_info.size() - df) return refuse("slack structural", &stats.slack_structural);
  for (size_t i = 0; i < suffix; ++i) {
    const LiveTlasInstance& now = info[static_count + i];
    const LiveTlasInstance& was = data->tlas_info[df + i];
    if (now.id != was.id || now.dynamic != was.dynamic || now.wind != was.wind
        || std::memcmp(instances[static_count + i].header, data->tlas_instances_cpu[df + i].header, 3u * sizeof(float)) != 0
        || std::memcmp(instances[static_count + i].visibility, data->tlas_instances_cpu[df + i].visibility,
                       sizeof(instances[static_count + i].visibility)) != 0) {
      return refuse("slack structural", &stats.slack_structural);
    }
  }
  // The static block by id: removed (in the TLAS, not in the snapshot), added (not in the TLAS), changed (same id, other descriptor).
  std::unordered_set<uint64_t> snapshot_ids;
  for (uint32_t i = 0; i < static_count; ++i) snapshot_ids.insert(info[i].id);
  std::vector<uint32_t> removed;
  for (const auto& entry : data->tlas_slot_of_id) {
    if (snapshot_ids.count(entry.first) == 0u) removed.push_back(entry.second);
  }
  std::vector<uint32_t> added;
  std::vector<uint32_t> changed;
  for (uint32_t i = 0; i < static_count; ++i) {
    const auto it = data->tlas_slot_of_id.find(info[i].id);
    if (it == data->tlas_slot_of_id.end()) {
      added.push_back(i);
    } else if (std::memcmp(&instances[i], &data->tlas_instances_cpu[it->second], sizeof(WorldInstanceGPU)) != 0) {
      changed.push_back(i);
    }
  }
  lap(&phase_ms[0]);
  const size_t ops = removed.size() + added.size() + changed.size();
  if (ops > kLiveSlackMaxOps || added.size() > data->tlas_free_slots.size() + removed.size()) {
    return refuse("slack exhausted", &stats.slack_exhausted);
  }
  if (ops == 0u) {
    for (uint32_t i = 0; i < static_count; ++i) data->tlas_info[data->tlas_slot_of_id[info[i].id]] = info[i];  // provenance only
    data->tlas_check_frame = frame;
    *ops_out = 0u;
    return true;
  }

  // Edits on copies: nothing of the TLAS changes until every write has succeeded.
  std::vector<WorldInstanceGPU> next = data->tlas_instances_cpu;
  std::vector<BVHLeafGPU> leaves = data->tlas_leaves_cpu;
  std::vector<LiveTlasInstance> next_info = data->tlas_info;
  std::unordered_map<uint64_t, uint32_t> slot_of = data->tlas_slot_of_id;
  std::vector<uint32_t> free_slots = data->tlas_free_slots;
  std::vector<std::array<float, 3>> anchor = data->tlas_hole_anchor;
  uint32_t first = UINT32_MAX;  // touched slot span
  uint32_t last = 0u;
  const auto touch = [&](uint32_t slot) {
    first = (std::min)(first, slot);
    last = (std::max)(last, slot);
  };
  const auto set_leaf = [&](uint32_t slot, const float* bounds_min, const float* bounds_max) {
    BVHLeafGPU& leaf = leaves[data->tlas_leaf_of_instance[slot]];
    for (int k = 0; k < 3; ++k) {
      leaf.bounds_min[k] = bounds_min[k];
      leaf.bounds_max[k] = bounds_max[k];
    }
  };
  for (const uint32_t slot : removed) {
    const LiveTlasInstance& was = data->tlas_info[slot];
    std::array<float, 3> centre = {};
    for (int k = 0; k < 3; ++k) centre[k] = 0.5f * (was.bounds_min[k] + was.bounds_max[k]);
    set_leaf(slot, centre.data(), centre.data());
    anchor[slot] = centre;
    next[slot] = WorldInstanceGPU{};
    next_info[slot] = LiveTlasInstance{};
    slot_of.erase(was.id);
    free_slots.push_back(slot);
    touch(slot);
  }
  for (const uint32_t i : changed) {
    const uint32_t slot = slot_of[info[i].id];
    next[slot] = instances[i];
    next_info[slot] = info[i];
    set_leaf(slot, info[i].bounds_min, info[i].bounds_max);
    touch(slot);
  }
  for (const uint32_t i : added) {
    std::array<float, 3> centre = {};
    for (int k = 0; k < 3; ++k) centre[k] = 0.5f * (info[i].bounds_min[k] + info[i].bounds_max[k]);
    size_t pick = 0u;
    float best = (std::numeric_limits<float>::max)();
    for (size_t k = 0; k < free_slots.size(); ++k) {
      const std::array<float, 3>& at = anchor[free_slots[k]];
      const float d = (at[0] - centre[0]) * (at[0] - centre[0]) + (at[1] - centre[1]) * (at[1] - centre[1])
                      + (at[2] - centre[2]) * (at[2] - centre[2]);
      if (d < best) {
        best = d;
        pick = k;
      }
    }
    const uint32_t slot = free_slots[pick];
    free_slots[pick] = free_slots.back();
    free_slots.pop_back();
    next[slot] = instances[i];
    next_info[slot] = info[i];
    set_leaf(slot, info[i].bounds_min, info[i].bounds_max);
    slot_of[info[i].id] = slot;
    touch(slot);
  }
  lap(&phase_ms[1]);

  std::vector<BVHNodeGPU> nodes = data->tlas_nodes_cpu;
  RefitBvhNodes(leaves, &nodes);
  const char* invalid = DescribeBvhValidation(ValidateBvh(nodes, leaves));
  lap(&phase_ms[2]);
  if (invalid != nullptr) {
    stats.last_failure = std::string("TLAS slack: ") + invalid + " (full rebuild)";
    return refuse("slack invalid", &stats.tlas_invalid);
  }

  const uint32_t count = last - first + 1u;
  std::vector<uint32_t> active_values(count);
  for (uint32_t slot = first; slot <= last; ++slot) active_values[slot - first] = next_info[slot].id != 0u ? slot : kLiveSlackSentinel;
  std::string error;
  const bool written =
      WriteBufferRange(device, cmd_list, data->instance_buffer, sizeof(WorldInstanceGPU), first * sizeof(WorldInstanceGPU), &next[first],
                       count * sizeof(WorldInstanceGPU), &error)
      && WriteBufferRange(device, cmd_list, data->active_buffer, sizeof(uint32_t), first * sizeof(uint32_t), active_values.data(),
                          count * sizeof(uint32_t), &error)
      && WriteBufferRange(device, cmd_list, data->tlas_leaf_buffer, sizeof(BVHLeafGPU), 0u, leaves.data(),
                          leaves.size() * sizeof(BVHLeafGPU), &error)
      && WriteBufferRange(device, cmd_list, data->tlas_node_buffer, sizeof(BVHNodeGPU), 0u, nodes.data(),
                          nodes.size() * sizeof(BVHNodeGPU), &error);
  lap(&phase_ms[3]);
  if (!written) {
    stats.upload_failures += 1u;
    stats.last_failure = "TLAS slack upload: " + error + " (full rebuild)";
    data->tlas_refit_failed = true;
    *why = "slack upload";
    return false;
  }

  // Swapped only after every write succeeded.
  data->tlas_instances_cpu = std::move(next);
  data->tlas_leaves_cpu = std::move(leaves);
  data->tlas_nodes_cpu = std::move(nodes);
  data->tlas_info = std::move(next_info);
  data->tlas_slot_of_id = std::move(slot_of);
  data->tlas_free_slots = std::move(free_slots);
  data->tlas_hole_anchor = std::move(anchor);
  data->active_count = static_cast<uint32_t>(data->tlas_instances_cpu.size() - data->tlas_free_slots.size());
  data->tlas_refits_since_rebuild += 1u;
  data->tlas_check_frame = frame;
  stats.tlas_instances = data->active_count;
  stats.slack_applies += 1u;
  stats.slack_added += static_cast<uint32_t>(added.size());
  stats.slack_removed += static_cast<uint32_t>(removed.size());
  stats.slack_changed += static_cast<uint32_t>(changed.size());
  stats.slack_free_holes = static_cast<uint32_t>(data->tlas_free_slots.size());
  std::memcpy(stats.slack_phase_ms, phase_ms, sizeof(phase_ms));
  if (ops != 0u) {
    for (size_t p = 0; p < 4u; ++p) stats.slack_phase_ms_sum[p] += phase_ms[p];
  }
  *ops_out = static_cast<uint32_t>(ops);
  return true;
}

inline void RebuildLiveTlas(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    BvhDeviceData* data,
    const PoolRegion& region,
    uint32_t frame,
    const char* reason,
    bool allow_slack) {
  const PoolCpuTimer rebuild_timer{&g_perf_gate.block_us[kPerfBlockRebuildCall]};
  auto start = PerfNow();
  LiveBvhStats& stats = data->live;
  CacheRebuildExtra diag;  // cache diagnostics: phase times (laps), no-op test, cause and reason
  diag.reason = reason;
  auto phase_start = start;
  const auto lap = [&](float* ms) {
    *ms = LiveElapsedMs(phase_start);
    phase_start = PerfNow();
  };
  std::vector<LiveTlasInstance> snapshot;
  uint64_t revision = 0u;
  uint64_t visibility_revision = 0u;
  uint64_t dynamic_revision = 0u;
  uint64_t flag_revision = 0u;
  uint32_t sources = 0u;
  uint32_t p2b_skipped = 0u;
  const bool per_frame = LiveDynamicActive(*data);
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    revision = g_pool.revision;
    visibility_revision = g_pool.visibility_revision;
    dynamic_revision = g_pool.dynamic_revision;
    flag_revision = g_pool.dynamic_flag_revision;
    sources = g_pool.revision_sources_pending;
    g_pool.stats.wind_sway_instances = 0u;
    g_pool.stats.wind_sway_max_m = 0.f;
    g_pool.stats.wind_sway_bound_m = 0.f;
    g_pool.stats.wind_pass = {};
    g_pool.stats.wind_pass.frame = frame;
    g_pool.stats.wind_pass.switch_on = g_pool.alpha_wind_sway.load(std::memory_order_relaxed);
    for (const WorldInstance& instance : g_pool.instances) {
      if (!PoolInstanceInRegion(instance, region) || instance.mesh_id >= g_pool.meshes.size()) continue;
      if (per_frame && g_pool.meshes[instance.mesh_id].dynamic && !g_pool.meshes[instance.mesh_id].alpha) {  // P2b: drawn per frame, not in the TLAS (alpha: never recorded)
        p2b_skipped += 1u;
        continue;
      }
      LiveTlasInstance& copy = snapshot.emplace_back();
      copy.uid = g_pool.meshes[instance.mesh_id].uid;
      copy.alpha = g_pool.meshes[instance.mesh_id].alpha;
      copy.id = instance.id;
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
      std::memcpy(copy.param2, instance.param2, sizeof(copy.param2));
      ApplyPoolWindSway(&copy, instance);
      copy.dynamic = instance.dynamic || copy.wind;  // swayed wind instances are refitted every present
    }
  }
  std::stable_partition(snapshot.begin(), snapshot.end(), [](const LiveTlasInstance& instance) { return !instance.dynamic; });
  lap(&diag.phase_ms[0]);
  // The switch is read once: the TLAS reflects it (commit), so a later switch is seen as a change.
  const uint8_t alpha_mode = AlphaMode();
  const bool sway_on = g_pool.alpha_wind_sway.load(std::memory_order_relaxed);
  uint32_t wind_count = 0u;
  // What this TLAS reflects; recorded only once it is in place, so a failed
  // build is retried (after kLiveRetryFrames).
  const uint64_t store_version = data->store_version;
  const auto commit = [&](bool keep_dynamic_revision = false) {
    {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      g_pool.revision_sources_pending &= ~sources;  // cleared only once this TLAS is in place; a failed build keeps them for the retry
    }
    data->tlas_pool_revision = revision;
    data->tlas_visibility_revision = visibility_revision;
    if (!keep_dynamic_revision) data->tlas_dynamic_revision = dynamic_revision;
    data->tlas_per_frame = per_frame;
    data->tlas_p2b_skipped = p2b_skipped;
    data->tlas_flag_revision = flag_revision;
    data->tlas_store_version = store_version;
    data->tlas_alpha_mode = alpha_mode;
    data->tlas_sway_on = sway_on;
    data->tlas_wind_count = wind_count;
    data->tlas_region_size = region.size;
    data->tlas_region_range = region.range;
    std::memcpy(data->tlas_region_min, region.min, sizeof(data->tlas_region_min));
    data->tlas_refit_failed = false;
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.tlas_wind_frame = frame;
  };
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
  uint32_t alpha_instances = 0u;
  uint32_t alpha_waiting = 0u;
  for (LiveTlasInstance& instance : snapshot) {
    if (instance.alpha && alpha_mode == 0u) continue;  // alpha_foliage off: alpha-tested meshes are not traced
    const auto slot_it = data->slot_by_uid.find(instance.uid);
    if (slot_it == data->slot_by_uid.end() || slot_it->second >= data->descriptor_count) {
      if (data->failed_uids.count(instance.uid) != 0u) {
        unusable += 1u;
      } else {
        waiting += 1u;
      }
      continue;
    }
    if (instance.alpha) {
      // The slice is written by the GPU stage (SyncLiveAlpha, same thread): slot_of_uid is read here, not under the lock.
      const auto material_it = data->alpha.slot_of_uid.find(instance.uid);
      if (material_it == data->alpha.slot_of_uid.end()) {
        alpha_waiting += 1u;
        continue;
      }
      instance.material = material_it->second + 1u;
      alpha_instances += 1u;
    }
    const WorldInstanceGPU descriptor = MakeLiveInstanceGPU(instance, slot_it->second);
    const uint32_t flags = InstanceVisibilityFlags(descriptor);
    if ((flags & kInstanceNearFade) != 0u) near_fade += 1u;
    if ((flags & kInstanceCameraVisible) == 0u) camera_hidden += 1u;
    leaves.push_back(MakeLeaf(instance.bounds_min, instance.bounds_max, static_cast<uint32_t>(instances.size())));
    instances.push_back(descriptor);
    info.push_back(instance);
    if (instance.wind) wind_count += 1u;
  }
  lap(&diag.phase_ms[1]);
  stats.tlas_waiting = waiting;
  stats.tlas_unusable = unusable;
  stats.tlas_alpha_instances = alpha_instances;
  stats.tlas_alpha_waiting = alpha_waiting;

  std::vector<LiveTlasInstance> previous_info;  // cache diagnostics: the TLAS this build replaces
  bool replaced = false;
  const uint32_t retire_bits = (1u << static_cast<uint32_t>(PoolRevisionSource::Retire)) | (1u << static_cast<uint32_t>(PoolRevisionSource::Compact));
  const uint32_t admit_bits = (1u << static_cast<uint32_t>(PoolRevisionSource::Admit)) | (1u << static_cast<uint32_t>(PoolRevisionSource::NewMesh));
  diag.cause = (sources & retire_bits) != 0u ? 0u : (sources & admit_bits) != 0u ? 1u : 2u;
  // The skip is counted here; its time is measured above (skip_ms). The record (RecordCacheTlasSkip) is not timed.
  const auto count_skip = [&]() {
    const float skip_ms = LiveElapsedMs(start);
    stats.tlas_skips += 1u;
    stats.tlas_skip_ms_sum += skip_ms;
    stats.tlas_skip_ms_max = (std::max)(stats.tlas_skip_ms_max, skip_ms);
    RecordCacheTlasSkip(skip_ms, diag.cause);
  };
  if (g_live_bvh.skip_noop.load(std::memory_order_relaxed) && !instances.empty() && !data->tlas_refit_failed
      && std::strcmp(reason, "first build") != 0 && std::strcmp(reason, "refit quality") != 0 && std::strcmp(reason, "refit failed") != 0
      && data->tlas_slack == g_live_bvh.slack.load(std::memory_order_relaxed)  // a switch change rebuilds (the layout changes)
      && LiveSnapshotMatchesTlas(*data, instances, info, false)) {
    // The TLAS already holds these instances: nothing is built, uploaded or destroyed. The static prefix takes the new
    // provenance; the dynamic suffix is left to the refit, and its revision is not taken over (the refit still runs).
    data->tlas_check_frame = frame;
    std::copy(info.begin(), info.begin() + data->tlas_dynamic_first, data->tlas_info.begin());
    commit(true);
    count_skip();
    return;
  }
  if (allow_slack && data->tlas_slack && data->tlas_built && !data->tlas_refit_failed && !instances.empty()) {
    // Only the static block changed (no pool revision, visibility, region, alpha or sway change, which the fresh check below would hide).
    const char* change_now = LiveTlasChange(*data, data->tlas_pool_revision, data->tlas_visibility_revision, region);
    std::vector<LiveTlasInstance> applied_before;  // cache diagnostics: the TLAS the apply changes
    if (g_cache_diag.enabled.load(std::memory_order_relaxed)) applied_before = data->tlas_info;
    uint32_t ops = 0u;
    if ((change_now == nullptr || std::strcmp(change_now, "store changed") == 0)
        && TryApplyLiveTlasSlack(device, cmd_list, data, frame, instances, info, &reason, &ops)) {
      commit(true);
      const float apply_ms = LiveElapsedMs(start);
      if (ops == 0u) {
        stats.slack_zero_op_applies += 1u;
        count_skip();
      } else {
        stats.slack_apply_ms_sum += apply_ms;
        stats.slack_apply_ms_max = (std::max)(stats.slack_apply_ms_max, apply_ms);
        CacheRebuildExtra applied;
        applied.applied = true;
        RecordCacheTlasRebuild(applied_before, data->tlas_info, true, frame, apply_ms, applied);
      }
      return;
    }
  }
  diag.reason = reason;
  stats.tlas_frame = frame;
  stats.tlas_reason = reason;
  stats.tlas_rebuilds += 1u;
  const auto finish = [&]() {
    // Cache diagnostics (Phase 0). The empty-TLAS and failed-build returns also come through here: an empty TLAS is
    // counted, a failed build (not replaced) counts nothing.
    stats.tlas_ms_last = LiveElapsedMs(start);
    stats.tlas_ms_max = (std::max)(stats.tlas_ms_max, stats.tlas_ms_last);
    RecordCacheTlasRebuild(previous_info, data->tlas_info, replaced, frame, stats.tlas_ms_last, diag);  // after the timing: its cost is not TLAS time
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    PoolTlasTiming& timing = g_pool.tlas_timing;
    timing.rebuilds += 1u;
    timing.rebuilds_refit_quality += std::strcmp(reason, "refit quality") == 0 ? 1u : 0u;
    timing.rebuilds_refit_failed += std::strcmp(reason, "refit failed") == 0 ? 1u : 0u;
    timing.rebuild_ms_last = stats.tlas_ms_last;
    timing.rebuild_ms_max = stats.tlas_ms_max;
    timing.rebuild_ms_sum += stats.tlas_ms_last;
    timing.rebuilds_by_cause[diag.cause] += 1u;
    timing.rebuild_ms_by_cause[diag.cause] += stats.tlas_ms_last;
    for (size_t p = 0; p < 5u; ++p) timing.rebuild_phase_ms_sum[p] += diag.phase_ms[p];
    if ((sources & retire_bits) != 0u && (sources & admit_bits) != 0u) timing.rebuilds_mixed += 1u;
    timing.refits_since_rebuild = data->tlas_refits_since_rebuild;
    g_pool.stats.dynamic_tlas_per_frame = data->tlas_per_frame ? 1u : 0u;
    g_pool.stats.dynamic_tlas_partition = data->tlas_dynamic_first;
    g_pool.stats.dynamic_tlas_p2b_skipped = data->tlas_p2b_skipped;
    g_pool.stats.tlas_wind_count = data->tlas_wind_count;
    g_pool.stats.tlas_sway_on = data->tlas_sway_on;
  };
  if (instances.empty()) {
    previous_info = std::move(data->tlas_info);
    replaced = true;
    DestroyLiveTlas(device, data);
    commit();
    stats.tlas_instances = 0u;
    stats.tlas_near_fade = 0u;
    stats.tlas_camera_hidden = 0u;
    stats.tlas_nodes = 0u;
    finish();
    return;
  }

  // Slack (session switch): spare slots (holes) in the static block, each with a parked leaf at an anchor; otherwise HEAD.
  const bool slack_layout = g_live_bvh.slack.load(std::memory_order_relaxed);
  const uint32_t static_count = static_cast<uint32_t>(
      std::count_if(info.begin(), info.end(), [](const LiveTlasInstance& instance) { return !instance.dynamic; }));
  const uint32_t spares = slack_layout && static_count >= kLiveSlackMinStatic
                              ? (std::max)(kLiveSlackMinSpares, static_count / kLiveSlackSpareDivisor)
                              : 0u;
  std::vector<std::array<float, 3>> hole_anchor;
  if (spares != 0u) {
    instances.insert(instances.begin() + static_count, spares, WorldInstanceGPU{});
    info.insert(info.begin() + static_count, spares, LiveTlasInstance{});
    for (BVHLeafGPU& leaf : leaves) {
      if (leaf.prim >= static_count) leaf.prim += spares;
    }
    hole_anchor.assign(instances.size(), std::array<float, 3>{});
    for (uint32_t h = 0u; h < spares; ++h) {
      const uint32_t source = static_cast<uint32_t>(static_cast<uint64_t>(h) * static_count / spares);
      std::array<float, 3>& anchor = hole_anchor[static_count + h];
      for (int k = 0; k < 3; ++k) anchor[k] = 0.5f * (info[source].bounds_min[k] + info[source].bounds_max[k]);
      leaves.push_back(MakeLeaf(anchor.data(), anchor.data(), static_count + h));
    }
  }
  if (slack_layout) hole_anchor.resize(instances.size(), std::array<float, 3>{});  // a removal parks a slot even with no holes yet

  SortBvhLeaves(&leaves);
  std::vector<uint32_t> leaf_of(leaves.size());
  for (uint32_t l = 0; l < leaves.size(); ++l) leaf_of[leaves[l].prim] = l;
  const uint32_t dynamic_first =
      static_cast<uint32_t>(std::count_if(info.begin(), info.end(), [](const LiveTlasInstance& instance) { return !instance.dynamic; }));
  std::vector<BVHNodeGPU> nodes;
  const bool built = BuildBvhTree(leaves, &nodes);
  const char* invalid = built ? DescribeBvhValidation(ValidateBvh(nodes, leaves)) : "tree construction failed";
  lap(&diag.phase_ms[2]);
  if (invalid != nullptr) {
    stats.tlas_invalid += 1u;
    stats.last_failure = std::string("TLAS: ") + invalid + " (previous TLAS kept)";
    renodx::utils::log::w("[world-bvh] TLAS over ", leaves.size(), " instances not uploaded: ", invalid);
    data->tlas_retry_frame = frame + kLiveRetryFrames;
    finish();
    return;
  }

  if (g_cache_diag.enabled.load(std::memory_order_relaxed)) {  // diagnostics stay out of the TLAS time (the laps shift with it)
    const auto diag_start = std::chrono::steady_clock::now();
    diag.noop_full = LiveSnapshotMatchesTlas(*data, instances, info, true);
    diag.noop_static = LiveSnapshotMatchesTlas(*data, instances, info, false);
    const auto diag_time = std::chrono::steady_clock::now() - diag_start;
    AddPerfBlockMs(kPerfBlockRebuildNoop, std::chrono::duration<double, std::milli>(diag_time).count());
    if (start != std::chrono::steady_clock::time_point{}) {  // gathering on (PerfNow): the laps exclude the diagnostics
      start += diag_time;
      phase_start += diag_time;
    }
  }
  std::vector<uint32_t> active(instances.size());
  for (uint32_t i = 0; i < active.size(); ++i) active[i] = info[i].id != 0u ? i : kLiveSlackSentinel;
  const auto usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source
                     | reshade::api::resource_usage::copy_dest;
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
    lap(&diag.phase_ms[3]);
    stats.upload_failures += 1u;
    stats.last_failure = "TLAS buffer creation failed: " + error + " (previous TLAS kept)";
    renodx::utils::log::w("[world-bvh] ", stats.last_failure);
    data->tlas_retry_frame = frame + kLiveRetryFrames;
    finish();
    return;
  }
  lap(&diag.phase_ms[3]);
  previous_info = std::move(data->tlas_info);
  replaced = true;
  DestroyLiveTlas(device, data);
  data->instance_buffer = instance_buffer.buffer;
  data->instance_srv = instance_buffer.view;
  data->active_buffer = active_buffer.buffer;
  data->active_srv = active_buffer.view;
  data->tlas_leaf_buffer = leaf_buffer.buffer;
  data->tlas_leaf_srv = leaf_buffer.view;
  data->tlas_node_buffer = node_buffer.buffer;
  data->tlas_node_srv = node_buffer.view;
  data->active_count = static_cast<uint32_t>(instances.size() - spares);
  data->tlas_instances_cpu = std::move(instances);
  data->tlas_slot_of_id.clear();
  if (slack_layout) {
    for (uint32_t i = 0; i < static_count; ++i) data->tlas_slot_of_id[info[i].id] = i;
  }
  data->tlas_free_slots.clear();
  for (uint32_t h = 0u; h < spares; ++h) data->tlas_free_slots.push_back(static_count + h);
  data->tlas_hole_anchor = std::move(hole_anchor);
  data->tlas_slack = slack_layout;
  data->tlas_info = std::move(info);
  data->tlas_leaves_cpu = std::move(leaves);
  data->tlas_nodes_cpu = std::move(nodes);
  data->tlas_leaf_of_instance = std::move(leaf_of);
  data->tlas_dynamic_first = dynamic_first;
  data->tlas_built = true;
  lap(&diag.phase_ms[4]);
  commit();
  data->bvh_ready = data->mesh_srv.handle != 0u && data->vertices.srv.handle != 0u && data->indices.srv.handle != 0u
                    && data->blas_leaves.srv.handle != 0u && data->blas_nodes.srv.handle != 0u;
  stats.tlas_instances = data->active_count;
  stats.tlas_near_fade = near_fade;
  stats.tlas_camera_hidden = camera_hidden;
  stats.tlas_nodes = static_cast<uint32_t>(data->tlas_nodes_cpu.size());
  finish();
}

// Moves the dynamic instances of the TLAS to their pool poses without a rebuild:
// same instances, leaves and node topology; new descriptors, leaf bounds and node
// bounds. The pool is read under its lock, the writes follow after it is released.
// Any failure keeps the previous TLAS and CPU copies and makes the next update a
// full rebuild ("refit failed").
inline void RefitLiveTlas(reshade::api::device* device, reshade::api::command_list* cmd_list, BvhDeviceData* data,
                          uint32_t frame) {
  const auto start = PerfNow();
  LiveBvhStats& stats = data->live;
  std::vector<LiveTlasInstance> updated;
  uint64_t dynamic_revision = 0u;
  uint32_t missing = 0u;
  uint32_t lag = 0u;
  uint32_t on_screen_lag = 0u;
  const uint32_t df = data->tlas_dynamic_first;
  float phase_ms[4] = {};
  auto phase_start = start;
  const auto lap = [&](float* ms) {
    *ms = LiveElapsedMs(phase_start);
    phase_start = PerfNow();
  };
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    dynamic_revision = g_pool.dynamic_revision;
    g_pool.stats.wind_sway_instances = 0u;
    g_pool.stats.wind_sway_max_m = 0.f;
    g_pool.stats.wind_sway_bound_m = 0.f;
    if (WindSwayPending(*data)) {
      g_pool.stats.wind_pass = {};
      g_pool.stats.wind_pass.frame = frame;
      g_pool.stats.wind_pass.refit = true;
      g_pool.stats.wind_pass.switch_on = g_pool.alpha_wind_sway.load(std::memory_order_relaxed);
    }
    // No dynamic instance in the TLAS: nothing to follow. Only the revision is taken over (no copy, no refit, no count).
    if (data->tlas_dynamic_first >= data->tlas_info.size() && !WindSwayPending(*data)) {
      data->tlas_dynamic_revision = dynamic_revision;
      return;
    }
    updated.assign(data->tlas_info.begin() + df, data->tlas_info.end());
    for (uint32_t i = df; i < data->tlas_info.size(); ++i) {
      LiveTlasInstance& copy = updated[i - df];
      // Pool instances are kept sorted by ascending id (test_motion checks it).
      const auto it = std::lower_bound(g_pool.instances.begin(), g_pool.instances.end(), copy.id,
                                       [](const WorldInstance& instance, uint64_t id) { return instance.id < id; });
      if (it == g_pool.instances.end() || it->id != copy.id) {
        missing += 1u;
        continue;
      }
      const WorldInstance& instance = *it;
      std::memcpy(copy.matrix, instance.matrix, sizeof(copy.matrix));
      std::memcpy(copy.inverse_world, instance.inverse_world, sizeof(copy.inverse_world));
      std::memcpy(copy.bounds_min, instance.bounds_min, sizeof(copy.bounds_min));
      std::memcpy(copy.bounds_max, instance.bounds_max, sizeof(copy.bounds_max));
      copy.visibility = GetPoolCameraVisibility(instance);
      ApplyPoolWindSway(&copy, instance);
      if (frame > instance.last_follow_frame) {
        lag = (std::max)(lag, frame - instance.last_follow_frame);
        if (copy.visibility.camera_seen) on_screen_lag = (std::max)(on_screen_lag, frame - instance.last_follow_frame);
      }
    }
  }
  lap(&phase_ms[0]);
  const auto finish = [&]() {
    stats.refit_ms_last = LiveElapsedMs(start);
    stats.refit_ms_max = (std::max)(stats.refit_ms_max, stats.refit_ms_last);
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.tlas_timing.refit_ms_last = stats.refit_ms_last;
    g_pool.tlas_timing.refit_ms_max = stats.refit_ms_max;
    g_pool.tlas_timing.refit_ms_sum += stats.refit_ms_last;
    std::memcpy(g_pool.tlas_timing.refit_phase_ms, phase_ms, sizeof(phase_ms));
    for (size_t p = 0; p < 4u; ++p) g_pool.tlas_timing.refit_phase_ms_sum[p] += phase_ms[p];
    g_pool.stats.tlas_wind_count = data->tlas_wind_count;
    g_pool.stats.tlas_sway_on = data->tlas_sway_on;
  };
  const auto fail = [&](const std::string& why) {
    stats.refit_failures += 1u;
    stats.last_failure = "TLAS refit: " + why + " (previous TLAS kept)";
    data->tlas_refit_failed = true;
    finish();
  };
  stats.refit_missing += missing;
  if (missing != 0u) {
    fail("instance no longer in the pool");
    return;
  }

  std::vector<WorldInstanceGPU> instances(data->tlas_instances_cpu.begin() + df, data->tlas_instances_cpu.end());
  std::vector<BVHLeafGPU> leaves = data->tlas_leaves_cpu;
  uint32_t first = UINT32_MAX;
  uint32_t last = 0u;
  uint32_t changed = 0u;
  for (uint32_t i = df; i < data->tlas_info.size(); ++i) {
    const auto slot_it = data->slot_by_uid.find(updated[i - df].uid);
    if (slot_it == data->slot_by_uid.end() || slot_it->second >= data->descriptor_count) {
      fail("mesh slot no longer resident");
      return;
    }
    const WorldInstanceGPU descriptor = MakeLiveInstanceGPU(updated[i - df], slot_it->second);
    if (std::memcmp(&descriptor, &instances[i - df], sizeof(descriptor)) == 0) continue;
    instances[i - df] = descriptor;
    BVHLeafGPU& leaf = leaves[data->tlas_leaf_of_instance[i]];
    for (int k = 0; k < 3; ++k) {
      leaf.bounds_min[k] = updated[i - df].bounds_min[k];
      leaf.bounds_max[k] = updated[i - df].bounds_max[k];
    }
    first = (std::min)(first, i);
    last = i;
    changed += 1u;
  }
  lap(&phase_ms[1]);
  if (changed == 0u) {
    std::move(updated.begin(), updated.end(), data->tlas_info.begin() + df);
    data->tlas_dynamic_revision = dynamic_revision;
    finish();
    return;
  }

  std::vector<BVHNodeGPU> nodes = data->tlas_nodes_cpu;
  RefitBvhNodes(leaves, &nodes);
  const char* invalid = DescribeBvhValidation(ValidateBvh(nodes, leaves));
  lap(&phase_ms[2]);
  if (invalid != nullptr) {
    stats.tlas_invalid += 1u;
    fail(invalid);
    return;
  }

  std::string error;
  const bool written =
      WriteBufferRange(device, cmd_list, data->instance_buffer, sizeof(WorldInstanceGPU), first * sizeof(WorldInstanceGPU),
                       &instances[first - df], (last - first + 1u) * sizeof(WorldInstanceGPU), &error)
      && WriteBufferRange(device, cmd_list, data->tlas_leaf_buffer, sizeof(BVHLeafGPU), 0u, leaves.data(),
                          leaves.size() * sizeof(BVHLeafGPU), &error)
      && WriteBufferRange(device, cmd_list, data->tlas_node_buffer, sizeof(BVHNodeGPU), 0u, nodes.data(),
                          nodes.size() * sizeof(BVHNodeGPU), &error);
  if (!written) {
    stats.upload_failures += 1u;
    fail("upload: " + error);
    return;
  }
  lap(&phase_ms[3]);

  // Swapped only after every write succeeded.
  std::move(instances.begin(), instances.end(), data->tlas_instances_cpu.begin() + df);
  std::move(updated.begin(), updated.end(), data->tlas_info.begin() + df);
  data->tlas_leaves_cpu = std::move(leaves);
  data->tlas_nodes_cpu = std::move(nodes);
  data->tlas_dynamic_revision = dynamic_revision;
  data->tlas_refits_since_rebuild += 1u;
  stats.tlas_refits += 1u;
  stats.refit_instances = changed;
  stats.refit_lag_last = lag;
  stats.refit_lag_max = (std::max)(stats.refit_lag_max, lag);
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.on_screen_lag_last = on_screen_lag;
    g_pool.stats.on_screen_lag_max = (std::max)(g_pool.stats.on_screen_lag_max, on_screen_lag);
    g_pool.tlas_timing.refits += 1u;
    g_pool.tlas_timing.refits_since_rebuild = data->tlas_refits_since_rebuild;
  }
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
  stats.gpu_check_frame = g_state.frame.load();
  stats.gpu_ok = false;
  std::vector<uint8_t> vertices;
  std::vector<uint8_t> indices;
  std::vector<uint8_t> leaves;
  std::vector<uint8_t> nodes;
  std::vector<uint8_t> uvs;
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
      && ReadLiveBytes(device, queue, data->uvs.buffer, data->uvs.used * data->uvs.stride, &uvs)
      && ReadLiveBytes(device, queue, data->mesh_buffer, data->descriptor_count * sizeof(WorldMeshGPU), &descriptors)
      && ReadLiveBytes(device, queue, data->instance_buffer, data->tlas_instances_cpu.size() * sizeof(WorldInstanceGPU), &instances)
      && ReadLiveBytes(device, queue, data->active_buffer, data->tlas_instances_cpu.size() * sizeof(uint32_t), &active)
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
      copy.uvs = mesh.uvs;
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
                       blas.nodes.size() * sizeof(BVHNodeGPU))
        && source->second.uvs.size() == slot.uv_count
        && range_equal(uvs, static_cast<uint64_t>(slot.uv_offset) * data->uvs.stride, source->second.uvs.data(),
                       source->second.uvs.size() * sizeof(std::array<float, 2>));
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
  for (uint32_t i = 0; i < data->tlas_instances_cpu.size() && active_ok; ++i) {
    uint32_t value = 0u;
    std::memcpy(&value, active.data() + static_cast<size_t>(i) * sizeof(uint32_t), sizeof(value));
    active_ok = value == (data->tlas_info[i].id != 0u ? i : kLiveSlackSentinel);
  }
  const bool tlas_ok =
      active_ok
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

// Slab test of a ray against a box: true with the entry distance (>= 0) when the ray crosses the box.
inline bool RayBoxEntry(const float* origin, const float* direction, const float* bounds_min, const float* bounds_max, float* entry) {
  float t_min = 0.f;
  float t_max = 1e30f;
  for (int k = 0; k < 3; ++k) {
    const float inv = 1.f / direction[k];
    float a = (bounds_min[k] - origin[k]) * inv;
    float b = (bounds_max[k] - origin[k]) * inv;
    if (a > b) std::swap(a, b);
    t_min = (std::max)(t_min, a);
    t_max = (std::min)(t_max, b);
    if (!(t_min <= t_max)) return false;
  }
  *entry = t_min;
  return true;
}

// Inspect lines for a pixel with no BVH hit, or whose hit the depth compare calls missing or extra: the pool instances
// whose bounds the pixel's primary ray crosses (nearest first, up to 8), each with its state. The game depth point is
// not read back, so the ray (trace camera position and the trace shader's pixel direction) is the only geometry.
inline void DescribeLiveCandidates(BvhDeviceData* data) {
  constexpr size_t kCandidateLines = 8u;
  BvhInspect& inspect = data->inspect;
  const bool compare_differs = inspect.compare_class == 8u || inspect.compare_class == 9u;  // missing, extra
  if (!inspect.valid || data->debug_width == 0u || data->debug_height == 0u) return;
  float origin[3] = {};
  TraceCameraPosition(origin);
  const float ndc_x = (static_cast<float>(inspect.x) + 0.5f) / static_cast<float>(data->debug_width) * 2.f - 1.f;
  const float ndc_y = 1.f - (static_cast<float>(inspect.y) + 0.5f) / static_cast<float>(data->debug_height) * 2.f;
  float far_h[4] = {};
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const float* m = g_state.camera.view_proj_inv;  // mul(float4(ndc, 0, 1), view_proj_inv) as the trace shader does it
    for (uint32_t j = 0u; j < 4u; ++j) far_h[j] = ndc_x * m[j * 4u] + ndc_y * m[j * 4u + 1u] + m[j * 4u + 3u];
  }
  float direction[3] = {far_h[0] / far_h[3] - origin[0], far_h[1] / far_h[3] - origin[1], far_h[2] / far_h[3] - origin[2]};
  const float length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
  for (float& d : direction) d /= length;
  inspect.ray_valid = true;
  std::copy(std::begin(origin), std::end(origin), inspect.ray_origin.begin());
  std::copy(std::begin(direction), std::end(direction), inspect.ray_direction.begin());
  if (inspect.hit && !compare_differs) return;

  struct Crossing {
    float entry;
    size_t index;
  };
  std::vector<Crossing> crossings;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    for (size_t i = 0u; i < g_pool.instances.size(); ++i) {
      const WorldInstance& instance = g_pool.instances[i];
      float entry = 0.f;
      if (RayBoxEntry(origin, direction, instance.bounds_min, instance.bounds_max, &entry)) crossings.push_back({entry, i});
    }
  }
  std::sort(crossings.begin(), crossings.end(), [](const Crossing& a, const Crossing& b) { return a.entry < b.entry; });
  inspect.crossing_instances = crossings.size();
  if (crossings.empty()) {
    inspect.lines.push_back("No pool instance's bounds lie along this pixel's ray.");
    return;
  }
  inspect.lines.push_back(LiveFormat("Pool instances whose bounds this pixel's ray crosses: %u (nearest %u listed).",
                                     static_cast<uint32_t>(crossings.size()),
                                     static_cast<uint32_t>((std::min)(crossings.size(), kCandidateLines))));
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    for (size_t n = 0u; n < crossings.size() && n < kCandidateLines; ++n) {
      const WorldInstance& instance = g_pool.instances[crossings[n].index];
      if (instance.mesh_id >= g_pool.meshes.size()) continue;
      const WorldMesh& mesh = g_pool.meshes[instance.mesh_id];
      const bool camera_seen = GetPoolCameraVisibility(instance).camera_seen;
      const auto tlas = std::find_if(data->tlas_info.begin(), data->tlas_info.end(),
                                     [&](const LiveTlasInstance& entry) { return entry.id == instance.id; });
      PoolInspectCandidate& candidate = inspect.candidates.emplace_back();
      candidate.instance_id = instance.id;
      candidate.mesh_key = instance.mesh_key;
      candidate.mesh_uid = mesh.uid;
      candidate.vs_hash = instance.source_vs_hash;
      candidate.entry = crossings[n].entry;
      candidate.tlas_index = tlas != data->tlas_info.end() ? static_cast<int64_t>(tlas - data->tlas_info.begin()) : -1;
      if (g_pool.failed_meshes.count(instance.mesh_key) != 0u) {
        candidate.status = "failed_mesh";
        candidate.reason = "failed mesh";
      } else if (mesh.alpha && mesh.alpha_state.conflict) {
        candidate.status = "conflicted";
        candidate.reason = "conflicted";
      } else if (!camera_seen && inspect.hiding) {
        candidate.status = "hidden";
        candidate.reason = "hidden (the game camera does not show it)";
      } else if (LiveDynamicActive(*data) && mesh.dynamic && !mesh.alpha) {
        candidate.status = "p2b_record";
        candidate.reason = "per-frame record (P2b, not in the TLAS)";
      } else if (instance.dynamic) {
        candidate.status = "dynamic";
        candidate.reason = "dynamic";
      } else if (tlas != data->tlas_info.end()) {
        candidate.status = "in_tlas";
        candidate.reason = LiveFormat("in TLAS (index %u)", static_cast<uint32_t>(candidate.tlas_index));
      } else if (data->slot_by_uid.count(mesh.uid) == 0u) {
        candidate.status = "not_uploaded";
        candidate.reason = "not in TLAS (mesh not uploaded)";
      } else if (mesh.alpha && !g_pool.alpha_foliage.load(std::memory_order_relaxed)) {
        candidate.status = "alpha_foliage_off";
        candidate.reason = "not in TLAS (alpha_foliage off)";
      } else if (mesh.alpha && data->alpha.slot_of_uid.count(mesh.uid) == 0u) {
        candidate.status = "alpha_waiting";
        candidate.reason = "alpha waiting (no slice, tlas_waiting)";
      } else {
        candidate.status = "outside_region";
        candidate.reason = "not in TLAS (outside the region)";
      }
    }
  }
  for (const PoolInspectCandidate& candidate : inspect.candidates) {
    inspect.lines.push_back(LiveFormat("Candidate pool id %llu, mesh uid %llu, VS 0x%08X: %s.",
                                       static_cast<unsigned long long>(candidate.instance_id),
                                       static_cast<unsigned long long>(candidate.mesh_uid), candidate.vs_hash,
                                       candidate.reason.c_str()));
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
  } else if ((inspect.instance & kDynamicFlagsMask) == kDynamicDrawFlag) {
    const uint32_t record = inspect.instance & ~kDynamicFlagsMask;
    uint32_t slot = 0u;
    uint32_t flags = 0u;
    for (const DynamicDrawGPU& draw : data->dynamic.last_table) {
      if (record >= draw.info[2] && record < draw.info[2] + draw.info[1]) {
        slot = draw.extra[2];
        flags = draw.info[3];
        break;
      }
    }
    inspect.lines.push_back(LiveFormat("Pixel (%u, %u): hit per-frame record %u (mesh slot %u, flags %u).", inspect.x, inspect.y,
                                       record, slot, flags)
                            + compare_text);
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
    inspect.lines.push_back(LiveFormat("Instance %u: VS 0x%08X, position (%.2f, %.2f, %.2f), axis scales (%.3f, %.3f, %.3f)%s.",
                                       inspect.instance, info.vs_hash, m[3], m[7], m[11], scale[0], scale[1], scale[2],
                                       PoolOutsideLegacyScale(m) ? ", outside the legacy scale limits" : ""));

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
        mesh.captures = candidate.captures;
        mesh.verified = candidate.verified;
        mesh.source_vb_usage = candidate.source_vb_usage;
        mesh.source_vb_flags = candidate.source_vb_flags;
        mesh.source_ib_usage = candidate.source_ib_usage;
        mesh.source_ib_flags = candidate.source_ib_flags;
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
      inspect.lines.push_back(LiveFormat("Mesh buffers: VB %s, IB %s; captures taken %u (%s).",
                                         PoolBufferText(mesh.source_vb_usage, mesh.source_vb_flags).c_str(),
                                         PoolBufferText(mesh.source_ib_usage, mesh.source_ib_flags).c_str(), mesh.captures,
                                         mesh.verified ? "the last two were identical" : "not verified"));
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
  DescribeLiveCandidates(data);
  for (const std::string& line : inspect.lines) renodx::utils::log::i("[world-bvh] inspect: ", line);
}

// ---------------------------------------------------------------------------
// Per present.

// P2b GPU objects: the arena (the copy destination of the flagged draws, bvh_pool.hpp), the draw table, the records
// and the expansion pass. Created once; the pool gets the arena handle under its lock.
inline bool EnsureDynamicExpand(reshade::api::device* device, BvhDeviceData* data) {
  DynamicExpandGpu& dynamic = data->dynamic;
  using RU = reshade::api::resource_usage;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using DR = reshade::api::descriptor_range;
  using P = reshade::api::pipeline_layout_param;
  DR srv_range = {0, 0, 0, 2, DS::all_compute, 1, DT::shader_resource_view};
  DR uav_range = {0, 0, 0, 1, DS::all_compute, 1, DT::unordered_access_view};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = 0;
  push_range.dx_register_space = 0;
  push_range.count = 4;
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

  reshade::api::resource_desc arena_desc = {};
  arena_desc.type = reshade::api::resource_type::buffer;
  arena_desc.buffer.size = kPoolDynamicArenaBytes;  // no stride: a raw buffer (CreatePoolBuffer rejects stride 0)
  arena_desc.heap = reshade::api::memory_heap::gpu_only;
  arena_desc.usage = RU::shader_resource | RU::copy_dest | RU::copy_source;
  bool ready =
      (dynamic.arena.handle != 0u
       || device->create_resource(arena_desc, nullptr, RU::shader_resource, &dynamic.arena))
      && (dynamic.arena_srv.handle != 0u
          || device->create_resource_view(
                 dynamic.arena, RU::shader_resource,
                 reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer, reshade::api::format::r32_typeless, 0,
                                                  kPoolDynamicArenaBytes / 4u),
                 &dynamic.arena_srv))
      && (dynamic.draws.handle != 0u
          || CreatePoolBuffer(device, nullptr, kPoolMaxDynamicDraws * sizeof(DynamicDrawGPU), sizeof(DynamicDrawGPU),
                              &dynamic.draws, &dynamic.draws_srv, RU::shader_resource | RU::copy_dest))
      && (dynamic.instances.handle != 0u
          || CreatePoolBuffer(device, nullptr, kPoolDynamicInstances * sizeof(WorldInstanceGPU), sizeof(WorldInstanceGPU),
                              &dynamic.instances, &dynamic.instances_uav, RU::shader_resource | RU::unordered_access))
      && (dynamic.instances_srv.handle != 0u
          || device->create_resource_view(
                 dynamic.instances, RU::shader_resource,
                 reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer, reshade::api::format::unknown, 0,
                                                  kPoolDynamicInstances),
                 &dynamic.instances_srv))
      && (dynamic.layout.handle != 0u || device->create_pipeline_layout(3, params, &dynamic.layout));
  ready = ready && (dynamic.srv_table.handle != 0u || device->allocate_descriptor_table(dynamic.layout, 0, &dynamic.srv_table));
  ready = ready && (dynamic.uav_table.handle != 0u || device->allocate_descriptor_table(dynamic.layout, 1, &dynamic.uav_table));
#if defined(__world_dynamic_expand_EMBED_FILE)
  if (ready && dynamic.pipeline.handle == 0u) {
    reshade::api::shader_desc shader = {};
    shader.code = __world_dynamic_expand.data();
    shader.code_size = __world_dynamic_expand.size();
    shader.entry_point = "main";
    reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    ready = device->create_pipeline(dynamic.layout, 1, &subobject, &dynamic.pipeline);
  }
#else
  ready = false;
#endif
  if (!ready) {
    if (!dynamic.failure_logged) {  // retried every present: logged once
      dynamic.failure_logged = true;
      renodx::utils::log::w("[world-bvh] P2b: GPU objects not created (arena ", dynamic.arena.handle != 0u, ", pipeline ",
                            dynamic.pipeline.handle != 0u, ")");
    }
    return false;
  }
  const reshade::api::resource_view srvs[2] = {dynamic.arena_srv, dynamic.draws_srv};
  const reshade::api::descriptor_table_update srv_update = {dynamic.srv_table, 0, 0, 2, DT::shader_resource_view, srvs};
  const reshade::api::descriptor_table_update uav_update = {dynamic.uav_table, 0, 0, 1, DT::unordered_access_view,
                                                            &dynamic.instances_uav};
  device->update_descriptor_tables(1, &srv_update);
  device->update_descriptor_tables(1, &uav_update);
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.dynamic_arena = dynamic.arena;
  }
  if (!dynamic.ready) {
    // The arena is published above, so draws record from now on. The flagged meshes leave the TLAS at the next TLAS
    // rebuild, while their records come from the next expansion: a window of one present can have them in neither
    // (not avoided: the expansion runs in ExpandLiveDynamic, the rebuild in UpdateLiveBvh).
    dynamic.ready = true;
    dynamic.not_ready_logged = false;
    renodx::utils::log::i("[world-bvh] P2b: GPU objects ready (arena ", kPoolDynamicArenaBytes / 1024u, " KB, records ", kPoolDynamicInstances, ")");
  }
  return true;
}

// P2b, per present (OnWorldPresent, after DrainPoolScan): expands the flagged draws of the present into the records the
// trace reads (t17; their count goes in g_dynamic.y). Their slices and args were copied at the draws (bvh_pool.hpp). A
// draw whose mesh is not resident in the live store gets an invalid entry, so its records are skipped by the trace.
inline void ExpandLiveDynamic(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return;
  DynamicExpandGpu& dynamic = data->dynamic;
  dynamic.instance_count = 0u;
  const auto start = PerfNow();
  std::vector<DynamicDrawGPU> table;
  uint32_t records = 0u;
  uint32_t not_resident = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    std::vector<PoolDynamicDraw> draws;
    draws.swap(g_pool.dynamic_present);
    for (const PoolDynamicDraw& draw : draws) {
      DynamicDrawGPU entry = {};
      uint32_t slot = 0u;
      bool resident = false;
      const auto mesh = g_pool.mesh_by_key.find(draw.mesh_key);
      if (mesh != g_pool.mesh_by_key.end() && mesh->second < g_pool.meshes.size()) {
        const auto slot_it = data->slot_by_uid.find(g_pool.meshes[mesh->second].uid);
        resident = slot_it != data->slot_by_uid.end() && slot_it->second < data->descriptor_count
                   && slot_it->second < data->slots.size() && data->slots[slot_it->second].live;
        if (resident) slot = slot_it->second;
      }
      if (!resident) not_resident += 1u;
      entry.info[0] = static_cast<uint32_t>(draw.arena / 16u);
      entry.info[1] = draw.count;
      entry.info[2] = draw.out;
      entry.info[3] = (resident ? 1u : 0u)
                      | ((draw.pass & kPoolPassCamera) != 0u ? kInstanceCameraVisible : 0u)
                      | ((draw.pass & kPoolPassLight) != 0u ? kInstanceShadowCaster : 0u);
      entry.extra[0] = draw.indirect ? 1u : 0u;
      entry.extra[1] = static_cast<uint32_t>(draw.args / 16u);
      entry.extra[2] = slot;
      if (resident) {
        for (int k = 0; k < 3; ++k) {
          entry.bbox_min[k] = data->slots[slot].bbox_min[k];
          entry.bbox_max[k] = data->slots[slot].bbox_max[k];
        }
      }
      table.push_back(entry);
      records = draw.out + draw.count;  // records are reserved in draw order
    }
    g_pool.stats.dynamic_records = records;
    g_pool.stats.dynamic_not_resident = not_resident;
    g_pool.stats.dynamic_expand_cpu_ms = LiveElapsedMs(start);
    g_pool.stats.dynamic_expand_cpu_ms_sum += g_pool.stats.dynamic_expand_cpu_ms;
    g_pool.stats.dynamic_expand_gpu_ms = dynamic.timer.last_ms;
    g_pool.stats.dynamic_expand_presents += 1u;
    g_pool.stats.dynamic_expand_empty += table.empty() ? 1u : 0u;
    g_pool.stats.dynamic_ready = dynamic.ready;
    g_pool.stats.dynamic_instance_count = 0u;
    g_pool.stats.dynamic_flagged_total = 0u;
    g_pool.stats.dynamic_flagged_resident = 0u;
    for (const auto& mesh : g_pool.meshes) {
      if (!mesh.dynamic) continue;
      g_pool.stats.dynamic_flagged_total += 1u;
      const auto slot_it = data->slot_by_uid.find(mesh.uid);
      if (slot_it != data->slot_by_uid.end() && slot_it->second < data->slots.size() && data->slots[slot_it->second].live) {
        g_pool.stats.dynamic_flagged_resident += 1u;
      }
    }
    dynamic.last_draws = draws;
    dynamic.last_table = table;
    dynamic.last_frame = g_state.frame.load();
  }
  if (!g_live_bvh.enabled.load(std::memory_order_relaxed)) return;
  const PoolCpuTimer record_timer{&g_perf_gate.block_us[kPerfBlockP2bRecord]};
  if (!EnsureDynamicExpand(device, data)) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.dynamic_expand_failed += 1u;
    return;
  }
  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr || table.empty()) return;

  using RU = reshade::api::resource_usage;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  std::string error;
  if (!WriteBufferRange(device, cmd_list, dynamic.draws, sizeof(DynamicDrawGPU), 0u, table.data(),
                        table.size() * sizeof(DynamicDrawGPU), &error)) {
    renodx::utils::log::w("[world-bvh] P2b: draw table not written: ", error);
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.dynamic_expand_failed += 1u;
    return;
  }
  BeginGpuTimer(device, cmd_list, &dynamic.timer);
  cmd_list->barrier(dynamic.arena, RU::copy_dest, RU::shader_resource);
  cmd_list->barrier(dynamic.draws, RU::copy_dest, RU::shader_resource);
  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, dynamic.pipeline);
  const reshade::api::descriptor_table tables[2] = {dynamic.srv_table, dynamic.uav_table};
  cmd_list->bind_descriptor_tables(DS::all_compute, dynamic.layout, 0, 2, tables);
  const uint32_t draw_count = static_cast<uint32_t>(table.size());
  const bool legacy = g_pool.legacy_scale.load(std::memory_order_relaxed);
  float constants[4] = {};
  std::memcpy(&constants[0], &draw_count, sizeof(uint32_t));
  constants[1] = legacy ? kPoolLegacyMinScale : kPoolMinScale;
  constants[2] = legacy ? kPoolLegacyMaxScale : kPoolMaxScale;
  constants[3] = kPoolMaxPosition;
  cmd_list->push_constants(DS::all_compute, dynamic.layout, 2, 0, 4, constants);
  cmd_list->dispatch(draw_count, 1u, 1u);
  // Unbind the records' UAV: the trace binds them as SRV t17.
  const reshade::api::resource_view null_uav = {0u};
  const reshade::api::descriptor_table_update unbind = {dynamic.uav_table, 0, 0, 1, DT::unordered_access_view, &null_uav};
  device->update_descriptor_tables(1, &unbind);
  cmd_list->bind_descriptor_tables(DS::all_compute, dynamic.layout, 1, 1, &dynamic.uav_table);
  cmd_list->barrier(dynamic.instances, RU::unordered_access, RU::shader_resource);
  EndGpuTimer(cmd_list, &dynamic.timer);
  dynamic.instance_count = records;
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  g_pool.stats.dynamic_instance_count = records;
}

inline void UpdateLiveBvh(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr) return;
  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) return;
  PoolStageScope stage("live bvh: reset");
  if (g_live_bvh.reset_requested.exchange(false, std::memory_order_relaxed)) ResetLiveStore(device, data, "requested");

  const bool live_on = g_live_bvh.enabled.load(std::memory_order_relaxed);
  g_pool.live_on.store(live_on, std::memory_order_relaxed);
  if (!live_on) {
    // No source copy is made while the live BVH is off; the copies made before are freed here.
    for (const reshade::api::resource proxy : TakePoolAlphaProxies(true)) device->destroy_resource(proxy);
  }
  if (live_on) {
    const uint32_t frame = g_state.frame.load();
    uint64_t revision = 0u;
    uint64_t visibility_revision = 0u;
    uint64_t dynamic_revision = 0u;
    uint64_t flag_revision = 0u;
    const bool per_frame = LiveDynamicActive(*data);
    if (g_pool.dynamic_per_frame.load(std::memory_order_relaxed) && !data->dynamic.ready && !data->dynamic.not_ready_logged) {
      data->dynamic.not_ready_logged = true;
      renodx::utils::log::w("[world-bvh] P2b: flagged meshes stay in the TLAS (GPU objects not ready)");
    }
    {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      revision = g_pool.revision;
      visibility_revision = g_pool.visibility_revision;
      dynamic_revision = g_pool.dynamic_revision;
      flag_revision = g_pool.dynamic_flag_revision;
    }
    stage.Set("live bvh: meshes");
    // Pool changes (retirements) are handled at once; pending uploads wait
    // out a GPU buffer failure.
    if (revision != data->mesh_revision || (data->mesh_scan_needed && frame >= data->upload_retry_frame)) {
      const auto sync_start = PerfNow();
      const float blas_before = data->live.blas_build_ms_sum;
      SyncLiveMeshes(device, cmd_list, data, frame);
      AddPerfBlockMs(kPerfBlockSyncMeshes, LiveElapsedMs(sync_start) - (data->live.blas_build_ms_sum - blas_before));
    }
    stage.Set("live bvh: alpha");
    SyncLiveAlpha(device, cmd_list, data, frame);
    stage.Set("live bvh: tlas");
    const PoolRegion region = CurrentPoolRegion();
    const char* change = LiveTlasChange(*data, revision, visibility_revision, region);
    // P2b: the switch and a change of the flagged meshes change what the TLAS holds: applied at once, like alpha OFF.
    const bool per_frame_switched = data->tlas_per_frame != per_frame;
    const bool flags_changed = per_frame && data->tlas_flag_revision != flag_revision;
    if (change == nullptr && (per_frame_switched || flags_changed)) change = per_frame_switched ? "per-frame switch" : "flagged meshes";
    // OFF drops the alpha instances at once, whatever else changed (the reason is not the test).
    const bool alpha_off_now = (data->tlas_alpha_mode & ~AlphaMode()) != 0u;
    // The sway switch moves wind instances between their rest and sway poses (and partitions): applied at once, like alpha OFF.
    const bool sway_switched = data->tlas_sway_on != g_pool.alpha_wind_sway.load(std::memory_order_relaxed);
    const bool slack_switched = data->tlas_slack != g_live_bvh.slack.load(std::memory_order_relaxed);
    const bool allow_slack = !(alpha_off_now || sway_switched || per_frame_switched || flags_changed || slack_switched);
    // Flag changes wait for the rebuild interval (up to 8 presents of latency: a short ghost or duplicate in the debug view and RTAO for a flag set); switch changes are immediate.
    // A skipped rebuild (RebuildLiveTlas) counts as a check: the next one keeps the cadence it would have had.
    const uint32_t last_tlas = (std::max)(data->live.tlas_frame, data->tlas_check_frame);
    if (change != nullptr && frame >= data->tlas_retry_frame
        && (data->live.tlas_rebuilds == 0u || frame - last_tlas >= kLiveTlasInterval || frame < last_tlas
            || alpha_off_now || sway_switched || per_frame_switched || slack_switched)) {
      RebuildLiveTlas(device, cmd_list, data, region, frame, change, allow_slack);
      // A skipped rebuild (same instances) leaves the dynamic poses to the refit: it runs in this present, as after a rebuild.
      if (data->tlas_check_frame == frame && data->tlas_store_version == data->store_version
          && data->tlas_refits_since_rebuild < kLiveTlasRefitsPerRebuild
          && (data->tlas_dynamic_revision != dynamic_revision || WindSwayPending(*data))) {
        RefitLiveTlas(device, cmd_list, data, frame);
      }
    } else if (data->tlas_built && frame >= data->tlas_retry_frame && data->tlas_refit_failed) {
      RebuildLiveTlas(device, cmd_list, data, region, frame, "refit failed", false);
    } else if (data->tlas_built && frame >= data->tlas_retry_frame && data->tlas_store_version == data->store_version
               && (data->tlas_dynamic_revision != dynamic_revision || WindSwayPending(*data))) {
      if (data->tlas_refits_since_rebuild >= kLiveTlasRefitsPerRebuild) {
        RebuildLiveTlas(device, cmd_list, data, region, frame, "refit quality", false);
      } else {
        RefitLiveTlas(device, cmd_list, data, frame);
      }
    }
  }

  if (g_live_bvh.check_requested.exchange(false, std::memory_order_relaxed)) {
    stage.Set("live bvh: gpu check");
    const PoolCpuTimer gpu_check_timer{&g_perf_gate.block_us[kPerfBlockGpuCheck]};
    CheckLiveBvhOnGpu(device, queue, data);
  }
}

// Per present, after DrainPoolScan (OnWorldPresent): the dynamic-only refit, so the poses
// resolved by the drain reach the TLAS in this present. Same gates as the refit branch of
// UpdateLiveBvh; it never rebuilds (UpdateLiveBvh does, in the next present, when one is due).
inline void RefitLiveTlasAfterDrain(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return;
  BvhDeviceData* data = GetBvhDeviceData(device);
  if (data == nullptr || !g_live_bvh.enabled.load(std::memory_order_relaxed)) return;
  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) return;
  const uint32_t frame = g_state.frame.load();
  uint64_t dynamic_revision = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    dynamic_revision = g_pool.dynamic_revision;
  }
  // A WindSwayPending clause here would be dead: it sits under the revision check above, so it is always true.
  if (data->tlas_built && frame >= data->tlas_retry_frame && !data->tlas_refit_failed
      && data->tlas_store_version == data->store_version && data->tlas_dynamic_revision != dynamic_revision
      && data->tlas_refits_since_rebuild < kLiveTlasRefitsPerRebuild && (data->tlas_alpha_mode & ~AlphaMode()) == 0u) {
    const auto start = PerfNow();
    RefitLiveTlas(device, cmd_list, data, frame);
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    PoolTlasTiming& timing = g_pool.tlas_timing;
    timing.refits_after_drain += 1u;
    timing.after_drain_ms_last = LiveElapsedMs(start);
    timing.after_drain_ms_max = (std::max)(timing.after_drain_ms_max, timing.after_drain_ms_last);
    timing.after_drain_ms_sum += timing.after_drain_ms_last;
  } else if (data->tlas_built && data->tlas_store_version == data->store_version
             && data->tlas_dynamic_revision == dynamic_revision) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.tlas_timing.refit_after_drain_skipped += 1u;
  }
}
// Phase 0 cache diagnostics, once per present (bvh_debug.hpp). Diagnostic only: the tiers, histogram and self-check
// are computed every kCacheSampleInterval presents, or on the present after a Dump request, and read under g_pool.mutex.
// The lighting gap is a present that is dark: the lighting depth is not the depth of this frame's camera, or the
// lighting depth or the camera stamp is more than one frame old (no lighting pass ran).
// The switches as the panel and the dumps report them.
inline CacheSwitches ReadCacheSwitches() {
  CacheSwitches s;
  s.collect = g_cache_diag.enabled.load(std::memory_order_relaxed);
  s.skip_noop = g_live_bvh.skip_noop.load(std::memory_order_relaxed);
  s.slack = g_live_bvh.slack.load(std::memory_order_relaxed);
  s.live_bvh = g_live_bvh.enabled.load(std::memory_order_relaxed);
  return s;
}

// Present, before UpdateLiveBvh (OnWorldPresentBvh). Samples the performance rows of the previous present. Its brackets and
// InPresent rows are final by now, so every row of one sample is work of present N, sampled at the start of N+1, over one
// window (the frame interval ends here). The frame-value exchanges happen here too (gather only). Returns the present time
// for UpdateCacheDiag. Side effects: cache-diag capture_ms is one present fresher (it holds the previous drain), and perf start_frame is one present later.
inline std::chrono::steady_clock::time_point SamplePerfPresent(reshade::api::device* device, const PerfExtraGpu& extra = {}) {
  BvhDeviceData* data = device != nullptr ? GetBvhDeviceData(device) : nullptr;
  if (data == nullptr) return {};
  const PoolCpuTimer sample_timer{&g_perf_gate.block_us[kPerfBlockPerfSample]};
  const auto present_time = PerfNow();
  if (g_perf_gate.gather.load(std::memory_order_relaxed)) {
    g_pool.cpu_hook_frame_us = g_pool.cpu_hook_us.exchange(0u);
    g_pool.cpu_drain_frame_us = g_pool.cpu_drain_us.exchange(0u);
    g_pool.cpu_capture_frame_us = g_pool.cpu_capture_us.exchange(0u);
    g_deform_live.cpu_frame_us = g_deform_live.cpu_us.exchange(0u);
    PerfCounters counters;
    {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      counters = ReadPerfCounters(data, extra);
    }
    double frame_ms = std::numeric_limits<double>::quiet_NaN();
    {
      std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
      if (g_cache_diag.data.last_present != std::chrono::steady_clock::time_point{}) {
        frame_ms = std::chrono::duration<double, std::milli>(present_time - g_cache_diag.data.last_present).count();
      }
    }
    UpdatePerfTab(counters, frame_ms);
  }
  UpdatePerfRun(data, g_state.frame.load(), present_time, ReadCacheSwitches(), extra);
  return present_time;
}

inline void UpdateCacheDiag(reshade::api::device* device, std::chrono::steady_clock::time_point present_time = PerfNow()) {
  BvhDeviceData* data = device != nullptr ? GetBvhDeviceData(device) : nullptr;
  if (data == nullptr) return;
  const PoolCpuTimer diag_timer{&g_perf_gate.block_us[kPerfBlockCacheDiag]};
  if (g_cache_diag.reset_requested.exchange(false, std::memory_order_relaxed)) {
    if (g_perf.active.load(std::memory_order_relaxed)) {
      std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
      g_cache_diag.data.last_result = "Reset ignored: a performance test is running (the BLAS counters must not change mid-window)";
    } else {
      data->live.blas_builds = 0u;
      data->live.blas_built_triangles = 0u;
      data->live.blas_build_ms_sum = 0.f;
      data->live.blas_build_ms_max = 0.f;
      ResetCacheDiagCounters();
      std::lock_guard<std::mutex> pool_lock(g_pool.mutex);
      g_pool.resight.counters = PoolResightCounters{};
      g_pool.resight.by_signature.clear();
      g_pool.resight.shape_count.clear();
      g_pool.resight.sweep_frame = 0u;
    }
  }
  PoolStageScope stage("cache diag");
  const uint32_t frame = g_state.frame.load();
  const bool enabled = g_cache_diag.enabled.load(std::memory_order_relaxed);
  g_pool.resight.enabled.store(enabled);
  const bool force_tiers = g_cache_diag.force_tiers_requested.exchange(false, std::memory_order_relaxed);
  // Camera and region take g_state.mutex: read before the pool lock.
  float camera[3] = {};
  PoolCameraPosition(camera);
  const PoolRegion region = CurrentPoolRegion();
  bool dark = false;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    dark = !(g_state.camera.valid && g_state.depth_source.valid && g_state.depth_source.frame == g_state.camera.frame)
           || g_state.depth_source.frame + 1u < frame || g_state.camera.frame + 1u < frame;
  }
  bool sample = force_tiers;
  if (enabled) {
    std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
    sample = sample || (g_cache_diag.data.presents + 1u) % kCacheSampleInterval == 0u;
  }
  // Resident store (live slots by uid): built outside the pool lock.
  std::unordered_map<uint64_t, CacheResident> resident;
  if (sample) {
    for (const auto& [uid, index] : data->slot_by_uid) {
      if (index >= data->slots.size() || !data->slots[index].live) continue;
      resident[uid] = CacheResident{data->slots[index].leaf_count, LiveSlotBytes(*data, data->slots[index])};
    }
  }
  CacheTiers tiers;
  uint32_t serial = 0u;
  uint32_t pool_start = 0u;
  uint32_t pool_fresh_from = 0u;
  const char* pool_reason = "";
  uint64_t admitted = 0u;
  PoolResightCounters resight;
  size_t resight_tracked = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    serial = g_pool.episode_serial;
    pool_start = g_pool.episode_start_frame;
    pool_fresh_from = g_pool.episode_fresh_from;
    pool_reason = g_pool.episode_reason;
    admitted = g_pool.instances.size();
    resight = g_pool.resight.counters;
    resight_tracked = g_pool.resight.by_signature.size();
    if (sample) {
      const PoolCpuTimer tiers_timer{&g_perf_gate.block_us[kPerfBlockCacheTiers]};
      ComputeCacheTiers(g_pool.instances, g_pool.meshes, camera, region, resident, &tiers);
    }
  }
  std::lock_guard<std::mutex> lock(g_cache_diag.mutex);
  CacheDiagData& d = g_cache_diag.data;
  if (d.last_present != std::chrono::steady_clock::time_point{}) {
    d.frame_ms_last = std::chrono::duration<float, std::milli>(present_time - d.last_present).count();
    d.frame_ms_sum += d.frame_ms_last;
    d.frame_intervals += 1u;
  }
  d.last_present = present_time;

  const uint32_t previous_serial = d.serial_seen;
  d.serial_seen = serial;
  if (sample) {
    d.tiers = std::move(tiers);
    d.tiers_valid = true;
    d.tiers_frame = frame;
  }
  if (!enabled) {
    d.lighting = CacheLighting{};  // capture off: the lighting gap starts again when it is switched on
    d.prev.blas_builds = data->live.blas_builds;  // builds while off are not attributed to the first present on
    d.prev.blas_ms = data->live.blas_build_ms_sum;
    d.prev.capture_meshes = resight.capture_meshes;
    d.prev.capture_wait_sum = resight.capture_wait_sum;
    return;
  }
  d.presents += 1u;
  AddCacheLightingSample(&d.lighting, dark);
  // An episode starts at a map change or pool reset (serial changed), or at the first admitted instance.
  const bool triggered = serial != previous_serial;
  if (triggered || (!d.episode.ever && admitted != 0u)) {
    if (d.episode.open) FinishCacheEpisode(&d.episode, "interrupted");
    CacheEpisode next;
    next.ever = true;
    next.open = true;
    next.state = "open";
    next.reason = triggered ? pool_reason : "first admission";
    next.serial = serial;
    next.start_frame = triggered ? pool_start : frame;
    next.fresh_from = triggered ? pool_fresh_from : 0u;
    next.start_time = std::chrono::steady_clock::now();
    d.episode = std::move(next);
  } else if (d.episode.open) {
    d.episode.quiet_presents += 1u;
    if (d.episode.quiet_presents >= kCacheSettlePresents) FinishCacheEpisode(&d.episode, "settled");
  }
  d.resight = resight;
  d.resight_tracked = resight_tracked;
  const uint64_t blas_builds = data->live.blas_builds;
  const float blas_ms = data->live.blas_build_ms_sum;
  CacheWork delta;  // BLAS counters are cleared by a device destroy (now < previous: the delta is now)
  delta.blas_builds = blas_builds < d.prev.blas_builds ? blas_builds : blas_builds - d.prev.blas_builds;
  delta.blas_ms = blas_ms < d.prev.blas_ms ? blas_ms : blas_ms - d.prev.blas_ms;
  delta.capture_ms = static_cast<float>(g_pool.cpu_capture_frame_us.load(std::memory_order_relaxed)) / 1000.f;
  delta.capture_meshes = resight.capture_meshes - d.prev.capture_meshes;
  delta.capture_wait_sum = resight.capture_wait_sum - d.prev.capture_wait_sum;
  d.prev.blas_builds = blas_builds;
  d.prev.blas_ms = blas_ms;
  d.prev.capture_meshes = resight.capture_meshes;
  d.prev.capture_wait_sum = resight.capture_wait_sum;
  const auto add = [&delta](CacheWork* work) {
    work->blas_ms += delta.blas_ms;
    work->capture_ms += delta.capture_ms;
    work->blas_builds += delta.blas_builds;
    work->capture_meshes += delta.capture_meshes;
    work->capture_wait_sum += delta.capture_wait_sum;
  };
  add(&d.work);
  if (d.episode.open) add(&d.episode.work);
}

}  // namespace falcom_world::bvh
