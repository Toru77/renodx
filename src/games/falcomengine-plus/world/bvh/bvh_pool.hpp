#pragma once

// Phase 1 M1: persistent CPU world pool for the two-level world BVH.
//
// Admission is limited to Phase 0 runtime_verified families whose verified
// transform is a structured instance path (t15-style InstanceParam). Candidate
// draws are queued while the pool scan is active; instance ranges and meshes
// are read back at present, and an instance enters the pool after three stable
// observations (same mesh, same matrix). This is the extraction boundary
// between the Phase 0 research state and the BVH runtime.

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../../../utils/constants.hpp"
#include "../../../../utils/path.hpp"
#include "../../../../utils/scene.hpp"
#include "../capture/buffer_readback.hpp"
#include "../capture/cb_tracking.hpp"
#include "../capture/mesh_capture.hpp"
#include "../research/classification.hpp"
#include "../research/transform_candidates.hpp"
#include "../world_state.hpp"

namespace falcom_world::bvh {

enum class InstanceSource : uint8_t {
  CanonicalInstance = 0u,
  ConstantBuffer = 1u,
};

inline constexpr uint32_t kPoolStableObservations = 3u;
inline constexpr uint32_t kPoolMaxEntryAttempts = 16u;
inline constexpr uint32_t kPoolPruneAge = 600u;
inline constexpr size_t kPoolMaxMeshes = 4096u;
inline constexpr size_t kPoolMaxInstances = 2000000u;
inline constexpr uint32_t kPoolMaxCopiesPerFrame = 256u;
inline constexpr uint32_t kPoolMaxSnapshotsPerFrame = 64u;
inline constexpr uint64_t kPoolMaxResourceSnapshotBytes = 1ull * 1024ull * 1024ull;
inline constexpr uint64_t kPoolMaxCopyBytesPerFrame = 32ull * 1024ull * 1024ull;
inline constexpr uint32_t kPoolInstanceSlot = 15u;
inline constexpr uint32_t kPoolInstanceCbSlot = 1u;
inline constexpr uint32_t kPoolInstanceStride = 160u;
inline constexpr float kPoolRegionMinSize = 64.f;
inline constexpr float kPoolRegionMaxSize = 512.f;

struct WorldMesh {
  uint64_t mesh_key = 0u;
  uint32_t mesh_id = 0u;
  uint32_t source_vs_hash = 0u;
  uint32_t triangle_count = 0u;
  float bbox_min[3] = {};
  float bbox_max[3] = {};
  std::vector<std::array<float, 3>> positions;
  std::vector<uint32_t> indices;  // flat, three per triangle
};

struct WorldInstance {
  uint32_t mesh_id = 0u;
  uint32_t source_vs_hash = 0u;
  uint8_t kind = 0u;
  uint8_t source = static_cast<uint8_t>(InstanceSource::CanonicalInstance);
  uint32_t matrix_floats = 0u;
  float matrix[16] = {};
  float bounds_min[3] = {};
  float bounds_max[3] = {};
};

struct PoolScanEntry {
  uint32_t vs_hash = 0u;
  uint64_t mesh_key = 0u;
  DrawRecord draw;
  reshade::api::resource instance_buffer = {0u};
  reshade::api::resource instance_cb = {0u};
  uint8_t kind = 0u;
  uint32_t stride = 0u;
  uint32_t matrix_offset = 0u;
  uint32_t matrix_floats = 0u;
  uint32_t first_instance = 0u;
  uint32_t instance_count = 0u;
  int32_t base = 0;
};

struct InstanceKey {
  uint64_t mesh_key = 0u;
  uint64_t matrix_hash = 0u;
  uint8_t kind = 0u;
  bool operator==(const InstanceKey& other) const {
    return mesh_key == other.mesh_key && matrix_hash == other.matrix_hash && kind == other.kind;
  }
};

struct InstanceKeyHash {
  size_t operator()(const InstanceKey& key) const {
    uint64_t hash = 1469598103934665603ull;
    const auto mix = [&hash](uint64_t value) {
      hash ^= value;
      hash *= 1099511628211ull;
    };
    mix(key.mesh_key);
    mix(key.matrix_hash);
    mix(key.kind);
    return static_cast<size_t>(hash);
  }
};

struct ObservedInstance {
  uint32_t count = 0u;
  uint32_t last_frame = 0u;
  uint32_t vs_hash = 0u;
  uint8_t kind = 0u;
  uint32_t matrix_floats = 0u;
  int32_t base = 0;
  bool admitted = false;
  bool rejected = false;
  float matrix[16] = {};
};

struct PoolFamilyRecipe {
  bool initialized = false;
  uint8_t recorded_kind = 0u;
  uint32_t recorded_slot = 0u;
  uint32_t recorded_offset = 0u;
  uint32_t recorded_stride = 0u;
  int32_t recorded_base = 0;
  uint8_t used_kind = 0u;
  uint32_t used_slot = 0u;
  uint32_t used_offset = 0u;
  uint32_t used_stride = 0u;
  int32_t used_base = 0;
  bool canonical_override = false;
  uint32_t rejected_implausible = 0u;
  // Skip-reason counters so a pool dump names the gate that rejected a family
  // instead of leaving its absence unexplained.
  uint32_t skipped_not_candidate = 0u;
  uint32_t skipped_no_srv = 0u;
  uint32_t skipped_slot_invalid = 0u;
  uint32_t skipped_no_buffer = 0u;
  bool has_sample_matrix = false;
  float sample_matrix[16] = {};
  bool has_sample_base = false;
  int32_t sample_base = 0;
  uint32_t base_observed_count = 0u;
  int32_t base_min = 0;
  int32_t base_max = 0;
  float translation_min[3] = {1e30f, 1e30f, 1e30f};
  float translation_max[3] = {-1e30f, -1e30f, -1e30f};
};

struct PoolPendingCopy {
  PoolScanEntry entry;
  uint64_t snapshot_offset = 0u;
  uint64_t snapshot_size = 0u;
  uint64_t cb_staging_offset = 0u;
  uint32_t cb_size = 0u;
  bool has_cb = false;
  bool has_base_override = false;
};

struct PoolResourceSnapshot {
  reshade::api::resource resource = {0u};
  uint64_t staging_offset = 0u;
  uint64_t size = 0u;
};

struct PoolStats {
  size_t queued = 0u;
  size_t mesh_queue = 0u;
  size_t meshes = 0u;
  size_t observed = 0u;
  size_t admitted = 0u;
  size_t region = 0u;
  uint32_t skipped_unverified = 0u;
  uint32_t skipped_non_instance = 0u;
  uint32_t skipped_satisfied = 0u;
  uint32_t queue_drops = 0u;
  uint32_t copy_drops = 0u;
  uint32_t skipped_no_base = 0u;
  uint32_t skipped_beyond_snapshot = 0u;
  uint32_t mesh_failures = 0u;
  uint32_t skipped_not_candidate = 0u;
  uint32_t skipped_no_srv = 0u;
  uint32_t skipped_slot_invalid = 0u;
  uint32_t skipped_no_buffer = 0u;
  uint64_t scan_first_frame = 0u;
  uint64_t scan_last_frame = 0u;
};

// One draw identity's scheduling attempts plus the mesh it belongs to, so a
// late mesh capture can re-arm the attempts that expired while the mesh was
// still queued.
struct PoolEntryAttempt {
  uint32_t count = 0u;
  uint64_t mesh_key = 0u;
};

struct PoolState {
  std::atomic_bool scan_active{false};
  float region_size = 512.f;
  std::mutex mutex;
  std::vector<PoolPendingCopy> pending_snapshots;
  std::vector<PoolScanEntry> mesh_queue;
  std::unordered_set<uint64_t> mesh_queued;
  std::unordered_set<uint64_t> failed_meshes;
  std::unordered_map<uint64_t, uint32_t> mesh_by_key;
  std::vector<WorldMesh> meshes;
  std::unordered_map<InstanceKey, ObservedInstance, InstanceKeyHash> observations;
  std::vector<WorldInstance> instances;
  std::unordered_map<uint32_t, PoolFamilyRecipe> family_recipes;
  std::unordered_map<uint64_t, PoolEntryAttempt> entry_attempts;
  std::unordered_map<uint64_t, PoolResourceSnapshot> frame_snapshots;
  uint64_t revision = 0u;
  reshade::api::device* staging_device = nullptr;
  reshade::api::resource frame_staging = {0u};
  uint64_t frame_staging_capacity = 0u;
  uint64_t frame_staging_used = 0u;
  uint32_t frame_copy_count = 0u;
  uint32_t frame_snapshot_count = 0u;
  PoolStats stats;
};

inline PoolState g_pool;

inline bool IsPoolStructuredKind(uint8_t kind) {
  return kind == static_cast<uint8_t>(CandidateKind::Inst4x3Row)
         || kind == static_cast<uint8_t>(CandidateKind::Inst4x3Col)
         || kind == static_cast<uint8_t>(CandidateKind::Inst4x4Row)
         || kind == static_cast<uint8_t>(CandidateKind::Inst4x4Col);
}

// Chooses the instance layout the pool should use: the engine's canonical
// path (slot 15, stride 160) first, row kinds before col, then any other
// instance layout. Returns false when the family has no instance layout.
inline bool SelectPoolInstanceLayout(
    const std::vector<PassingLayout>& layouts,
    PassingLayout* out_layout) {
  if (out_layout == nullptr) return false;
  const PassingLayout* canonical_col = nullptr;
  const PassingLayout* fallback = nullptr;
  for (const auto& layout : layouts) {
    if (!IsPoolStructuredKind(static_cast<uint8_t>(layout.kind))) continue;
    const bool canonical_slot = layout.slot == kPoolInstanceSlot && layout.stride == kPoolInstanceStride;
    if (canonical_slot) {
      const uint8_t kind_value = static_cast<uint8_t>(layout.kind);
      const bool row_kind = kind_value == static_cast<uint8_t>(CandidateKind::Inst4x3Row)
                            || kind_value == static_cast<uint8_t>(CandidateKind::Inst4x4Row);
      if (row_kind) {
        *out_layout = layout;
        return true;
      }
      if (canonical_col == nullptr) canonical_col = &layout;
    } else if (fallback == nullptr) {
      fallback = &layout;
    }
  }
  if (canonical_col != nullptr) {
    *out_layout = *canonical_col;
    return true;
  }
  if (fallback != nullptr) {
    *out_layout = *fallback;
    return true;
  }
  return false;
}

inline void TransformPoolPoint(uint8_t kind, const float* matrix, const float* point, float* out) {
  switch (static_cast<CandidateKind>(kind)) {
    case CandidateKind::Inst4x3Row:
      TransformInst4x3RowDot(matrix, point[0], point[1], point[2], out);
      break;
    case CandidateKind::Inst4x3Col:
      TransformInst4x3Col(matrix, point[0], point[1], point[2], out);
      break;
    case CandidateKind::Inst4x4Row: {
      float out4[4] = {};
      TransformRowDot4(matrix, point[0], point[1], point[2], 1.f, out4);
      out[0] = out4[0];
      out[1] = out4[1];
      out[2] = out4[2];
      break;
    }
    case CandidateKind::Inst4x4Col: {
      float out4[4] = {};
      TransformCol(matrix, point[0], point[1], point[2], 1.f, out4);
      out[0] = out4[0];
      out[1] = out4[1];
      out[2] = out4[2];
      break;
    }
    default:
      out[0] = point[0];
      out[1] = point[1];
      out[2] = point[2];
      break;
  }
}

inline uint64_t PoolEntryIdentity(const PoolScanEntry& entry) {
  uint64_t hash = 1469598103934665603ull;
  const auto mix = [&hash](uint64_t value) {
    hash ^= value;
    hash *= 1099511628211ull;
  };
  // Stable per-draw identity. Runtime b1 must not participate: the game
  // repacks the instance buffer, so the same draw can use different
  // instanceOffset_g values across frames while still describing one object.
  mix(entry.vs_hash);
  mix(entry.mesh_key);
  mix(entry.instance_buffer.handle);
  mix(entry.kind);
  mix(entry.stride);
  mix(entry.matrix_offset);
  mix(entry.first_instance);
  mix(entry.instance_count);
  return hash;
}

struct PoolCameraInfo {
  bool valid = false;
  float row[3] = {};
  float legacy[3] = {};
  float position[3] = {};
};

inline PoolCameraInfo GetPoolCameraInfo() {
  PoolCameraInfo info;
  std::lock_guard<std::mutex> lock(g_state.mutex);
  if (!g_state.camera.valid) return info;
  const float* inv = g_state.camera.view_inv;
  info.row[0] = inv[3];
  info.row[1] = inv[7];
  info.row[2] = inv[11];
  info.legacy[0] = inv[12];
  info.legacy[1] = inv[13];
  info.legacy[2] = inv[14];
  const auto usable = [](const float* value) {
    return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2])
           && (value[0] != 0.f || value[1] != 0.f || value[2] != 0.f);
  };
  const float* selected = nullptr;
  if (usable(info.row)) {
    selected = info.row;
  } else if (usable(info.legacy)) {
    selected = info.legacy;
  }
  if (selected == nullptr) return info;
  info.position[0] = selected[0];
  info.position[1] = selected[1];
  info.position[2] = selected[2];
  info.valid = true;
  return info;
}

inline bool PoolCameraPosition(float* out) {
  if (out == nullptr) return false;
  const PoolCameraInfo info = GetPoolCameraInfo();
  if (!info.valid) return false;
  out[0] = info.position[0];
  out[1] = info.position[1];
  out[2] = info.position[2];
  return true;
}

struct PoolRegion {
  float size = 128.f;
  float min[3] = {};
  float max[3] = {};
};

inline PoolRegion CurrentPoolRegion() {
  PoolRegion region;
  region.size = g_pool.region_size;
  float camera[3] = {};
  if (!PoolCameraPosition(camera)) {
    camera[0] = camera[1] = camera[2] = 0.f;
  }
  // Snap the region center to half-size cells offset by a quarter: the camera
  // is then always at least size/4 away from every boundary (instead of being
  // able to sit right on one), while rebuilds still only happen when the
  // camera crosses a half-size cell.
  const float half = region.size * 0.5f;
  const float quarter = region.size * 0.25f;
  for (int i = 0; i < 3; ++i) {
    const float center = std::floor(camera[i] / half) * half + quarter;
    region.min[i] = center - half;
    region.max[i] = center + half;
  }
  return region;
}

inline bool PoolInstanceInRegion(const WorldInstance& instance, const PoolRegion& region) {
  for (int i = 0; i < 3; ++i) {
    if (instance.bounds_max[i] < region.min[i] || instance.bounds_min[i] > region.max[i]) return false;
  }
  return true;
}

inline void ComputePoolInstanceBounds(
    const WorldMesh& mesh,
    uint8_t kind,
    const float* matrix,
    float* out_min,
    float* out_max) {
  const float corners[8][3] = {
      {mesh.bbox_min[0], mesh.bbox_min[1], mesh.bbox_min[2]},
      {mesh.bbox_max[0], mesh.bbox_min[1], mesh.bbox_min[2]},
      {mesh.bbox_min[0], mesh.bbox_max[1], mesh.bbox_min[2]},
      {mesh.bbox_max[0], mesh.bbox_max[1], mesh.bbox_min[2]},
      {mesh.bbox_min[0], mesh.bbox_min[1], mesh.bbox_max[2]},
      {mesh.bbox_max[0], mesh.bbox_min[1], mesh.bbox_max[2]},
      {mesh.bbox_min[0], mesh.bbox_max[1], mesh.bbox_max[2]},
      {mesh.bbox_max[0], mesh.bbox_max[1], mesh.bbox_max[2]},
  };
  out_min[0] = out_min[1] = out_min[2] = 1e30f;
  out_max[0] = out_max[1] = out_max[2] = -1e30f;
  for (const auto& corner : corners) {
    float world[3] = {};
    TransformPoolPoint(kind, matrix, corner, world);
    for (int i = 0; i < 3; ++i) {
      out_min[i] = (std::min)(out_min[i], world[i]);
      out_max[i] = (std::max)(out_max[i], world[i]);
    }
  }
}

inline void PoolMatrixTranslation(uint8_t kind, const float* matrix, float* out) {
  switch (static_cast<CandidateKind>(kind)) {
    case CandidateKind::Inst4x3Row:
    case CandidateKind::Inst4x4Row:
      out[0] = matrix[3];
      out[1] = matrix[7];
      out[2] = matrix[11];
      break;
    case CandidateKind::Inst4x3Col:
      out[0] = matrix[9];
      out[1] = matrix[10];
      out[2] = matrix[11];
      break;
    case CandidateKind::Inst4x4Col:
      out[0] = matrix[12];
      out[1] = matrix[13];
      out[2] = matrix[14];
      break;
    default:
      out[0] = out[1] = out[2] = 0.f;
      break;
  }
}

inline bool PoolMatrixRigidPlausible(uint8_t kind, const float* matrix) {
  float rows[3][3] = {};
  switch (static_cast<CandidateKind>(kind)) {
    case CandidateKind::Inst4x3Row:
    case CandidateKind::Inst4x4Row:
      rows[0][0] = matrix[0]; rows[0][1] = matrix[1]; rows[0][2] = matrix[2];
      rows[1][0] = matrix[4]; rows[1][1] = matrix[5]; rows[1][2] = matrix[6];
      rows[2][0] = matrix[8]; rows[2][1] = matrix[9]; rows[2][2] = matrix[10];
      break;
    case CandidateKind::Inst4x3Col:
      rows[0][0] = matrix[0]; rows[0][1] = matrix[3]; rows[0][2] = matrix[6];
      rows[1][0] = matrix[1]; rows[1][1] = matrix[4]; rows[1][2] = matrix[7];
      rows[2][0] = matrix[2]; rows[2][1] = matrix[5]; rows[2][2] = matrix[8];
      break;
    case CandidateKind::Inst4x4Col:
      rows[0][0] = matrix[0]; rows[0][1] = matrix[4]; rows[0][2] = matrix[8];
      rows[1][0] = matrix[1]; rows[1][1] = matrix[5]; rows[1][2] = matrix[9];
      rows[2][0] = matrix[2]; rows[2][1] = matrix[6]; rows[2][2] = matrix[10];
      break;
    default:
      return true;
  }
  float lengths[3] = {};
  for (int r = 0; r < 3; ++r) {
    lengths[r] = std::sqrt(
        rows[r][0] * rows[r][0] + rows[r][1] * rows[r][1] + rows[r][2] * rows[r][2]);
    if (!(lengths[r] > 0.05f && lengths[r] < 50.f)) return false;
  }
  for (int a = 0; a < 3; ++a) {
    for (int b = a + 1; b < 3; ++b) {
      const float dot = (rows[a][0] * rows[b][0] + rows[a][1] * rows[b][1] + rows[a][2] * rows[b][2])
                        / (lengths[a] * lengths[b]);
      if (!(std::fabs(dot) < 0.995f)) return false;
    }
  }
  return true;
}

inline bool ComputePoolInstanceBoundsChecked(
    const WorldMesh& mesh,
    uint8_t kind,
    const float* matrix,
    float* out_min,
    float* out_max) {
  const uint32_t floats = KindMatrixFloats(static_cast<CandidateKind>(kind));
  for (uint32_t i = 0; i < floats; ++i) {
    if (!std::isfinite(matrix[i])) return false;
  }
  if (!PoolMatrixRigidPlausible(kind, matrix)) return false;
  ComputePoolInstanceBounds(mesh, kind, matrix, out_min, out_max);
  float extent = 0.f;
  for (int i = 0; i < 3; ++i) {
    if (!std::isfinite(out_min[i]) || !std::isfinite(out_max[i])) return false;
    extent = (std::max)(extent, out_max[i] - out_min[i]);
  }
  return extent > 1e-6f && extent < 1e6f;
}

inline void UpdatePoolStats() {
  g_pool.stats.queued = g_pool.pending_snapshots.size();
  g_pool.stats.mesh_queue = g_pool.mesh_queue.size();
  g_pool.stats.meshes = g_pool.meshes.size();
  g_pool.stats.observed = g_pool.observations.size();
  g_pool.stats.admitted = g_pool.instances.size();
  const PoolRegion region = CurrentPoolRegion();
  size_t in_region = 0u;
  for (const auto& instance : g_pool.instances) {
    if (PoolInstanceInRegion(instance, region)) in_region += 1u;
  }
  g_pool.stats.region = in_region;
}

inline bool AdmitPoolInstance(ObservedInstance& observed, uint32_t mesh_id) {
  if (mesh_id >= g_pool.meshes.size()) return false;
  float bounds_min[3] = {};
  float bounds_max[3] = {};
  if (!ComputePoolInstanceBoundsChecked(
          g_pool.meshes[mesh_id], observed.kind, observed.matrix, bounds_min, bounds_max)) {
    observed.rejected = true;
    g_pool.family_recipes[observed.vs_hash].rejected_implausible += 1u;
    return false;
  }
  observed.admitted = true;
  WorldInstance instance;
  instance.mesh_id = mesh_id;
  instance.source_vs_hash = observed.vs_hash;
  instance.kind = observed.kind;
  instance.source = static_cast<uint8_t>(InstanceSource::CanonicalInstance);
  instance.matrix_floats = observed.matrix_floats;
  std::memcpy(instance.matrix, observed.matrix, sizeof(float) * 16u);
  std::memcpy(instance.bounds_min, bounds_min, sizeof(float) * 3u);
  std::memcpy(instance.bounds_max, bounds_max, sizeof(float) * 3u);
  g_pool.instances.push_back(instance);
  g_pool.revision += 1u;
  PoolFamilyRecipe& recipe = g_pool.family_recipes[observed.vs_hash];
  if (!recipe.has_sample_matrix) {
    recipe.has_sample_matrix = true;
    std::memcpy(recipe.sample_matrix, observed.matrix, sizeof(float) * 16u);
    recipe.has_sample_base = true;
    recipe.sample_base = observed.base;
  }
  float translation[3] = {};
  PoolMatrixTranslation(observed.kind, observed.matrix, translation);
  for (int i = 0; i < 3; ++i) {
    recipe.translation_min[i] = (std::min)(recipe.translation_min[i], translation[i]);
    recipe.translation_max[i] = (std::max)(recipe.translation_max[i], translation[i]);
  }
  return true;
}

inline void AdmitPendingInstancesForMesh(uint64_t mesh_key, uint32_t mesh_id) {
  for (auto& [key, observed] : g_pool.observations) {
    if (key.mesh_key != mesh_key || observed.admitted || observed.rejected) continue;
    if (observed.count < kPoolStableObservations) continue;
    AdmitPoolInstance(observed, mesh_id);
  }
}

// A mesh can be captured many frames after its first draws (the mesh queue is
// drained one entry per frame). Re-arm the family's exhausted identities so
// the next draws re-observe and admit their instances now that the mesh
// exists. Caller holds g_pool.mutex.
inline void ClearPoolEntryAttemptsForMesh(uint64_t mesh_key) {
  for (auto it = g_pool.entry_attempts.begin(); it != g_pool.entry_attempts.end();) {
    if (it->second.mesh_key == mesh_key) {
      it = g_pool.entry_attempts.erase(it);
    } else {
      ++it;
    }
  }
}

inline void ExtractPoolInstances(
    const PoolScanEntry& entry,
    int32_t base,
    const uint8_t* bytes,
    uint64_t size,
    uint32_t frame) {
  const uint32_t count = entry.instance_count == 0u ? 1u : entry.instance_count;
  const uint32_t floats = entry.matrix_floats;
  if (floats == 0u || entry.stride == 0u) return;
  const int64_t element_base = base < 0 ? 0 : base;
  PoolFamilyRecipe& recipe = g_pool.family_recipes[entry.vs_hash];
  if (recipe.base_observed_count == 0u) {
    recipe.base_min = base;
    recipe.base_max = base;
  } else {
    recipe.base_min = (std::min)(recipe.base_min, base);
    recipe.base_max = (std::max)(recipe.base_max, base);
  }
  recipe.base_observed_count += 1u;
  bool any_unadmitted = false;
  for (uint32_t i = 0; i < count; ++i) {
    const uint64_t element = static_cast<uint64_t>(element_base) + entry.first_instance + i;
    const uint64_t offset = element * entry.stride + entry.matrix_offset;
    if (offset + static_cast<uint64_t>(floats) * 4u > size) {
      g_pool.stats.skipped_beyond_snapshot += 1u;
      continue;
    }
    float matrix[16] = {};
    std::memcpy(matrix, bytes + offset, sizeof(float) * floats);
    const InstanceKey key{entry.mesh_key, MatrixHash(matrix, floats), entry.kind};
    ObservedInstance& observed = g_pool.observations[key];
    if (observed.rejected) continue;
    if (observed.last_frame != frame) {
      observed.count += 1u;
      observed.last_frame = frame;
    }
    observed.vs_hash = entry.vs_hash;
    observed.kind = entry.kind;
    observed.matrix_floats = floats;
    observed.base = base;
    std::memcpy(observed.matrix, matrix, sizeof(float) * floats);
    if (!observed.admitted && observed.count >= kPoolStableObservations) {
      const auto mesh_it = g_pool.mesh_by_key.find(entry.mesh_key);
      if (mesh_it != g_pool.mesh_by_key.end()) {
        AdmitPoolInstance(observed, mesh_it->second);
      }
    }
    if (!observed.admitted && !observed.rejected) any_unadmitted = true;
  }
  if (!any_unadmitted) {
    PoolEntryAttempt& attempt = g_pool.entry_attempts[PoolEntryIdentity(entry)];
    attempt.count = kPoolMaxEntryAttempts;
    attempt.mesh_key = entry.mesh_key;
  }
}

inline void CaptureOnePoolMesh(reshade::api::device* device, reshade::api::command_queue* queue) {
  PoolScanEntry entry;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (g_pool.mesh_queue.empty()) return;
    entry = g_pool.mesh_queue.back();
    g_pool.mesh_queue.pop_back();
  }
  renodx::utils::scene::CapturedMesh mesh;
  std::string error;
  if (!renodx::utils::scene::CaptureDrawIndexed(device, queue, ToSceneRecord(entry.draw), &mesh, &error)
      || mesh.positions.empty() || mesh.triangles.empty()) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.failed_meshes.insert(entry.mesh_key);
    g_pool.stats.mesh_failures += 1u;
    return;
  }
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  if (g_pool.mesh_by_key.count(entry.mesh_key) != 0u) return;
  if (g_pool.meshes.size() >= kPoolMaxMeshes) return;
  WorldMesh world_mesh;
  world_mesh.mesh_key = entry.mesh_key;
  world_mesh.mesh_id = static_cast<uint32_t>(g_pool.meshes.size());
  world_mesh.source_vs_hash = entry.vs_hash;
  world_mesh.triangle_count = static_cast<uint32_t>(mesh.triangles.size());
  std::memcpy(world_mesh.bbox_min, mesh.bbox_min.data(), sizeof(float) * 3u);
  std::memcpy(world_mesh.bbox_max, mesh.bbox_max.data(), sizeof(float) * 3u);
  world_mesh.positions = std::move(mesh.positions);
  world_mesh.indices.reserve(mesh.triangles.size() * 3u);
  for (const auto& triangle : mesh.triangles) {
    world_mesh.indices.push_back(triangle[0]);
    world_mesh.indices.push_back(triangle[1]);
    world_mesh.indices.push_back(triangle[2]);
  }
  const uint32_t mesh_id = world_mesh.mesh_id;
  g_pool.meshes.push_back(std::move(world_mesh));
  g_pool.mesh_by_key[entry.mesh_key] = mesh_id;
  g_pool.revision += 1u;
  AdmitPendingInstancesForMesh(entry.mesh_key, mesh_id);
  ClearPoolEntryAttemptsForMesh(entry.mesh_key);
}

inline bool EnsurePoolFrameStaging(reshade::api::device* device, uint64_t needed) {
  if (device == nullptr || needed > kPoolMaxCopyBytesPerFrame) return false;
  if (g_pool.frame_staging.handle != 0u && g_pool.staging_device == device
      && g_pool.frame_staging_capacity >= needed) {
    return true;
  }
  if (g_pool.frame_staging.handle != 0u && g_pool.staging_device != nullptr) {
    g_pool.staging_device->destroy_resource(g_pool.frame_staging);
  }
  g_pool.frame_staging = {0u};
  g_pool.frame_staging_capacity = 0u;
  g_pool.staging_device = nullptr;
  const uint64_t capacity = (std::min)((std::max)(needed, static_cast<uint64_t>(1u << 20)), kPoolMaxCopyBytesPerFrame);
  if (capacity < needed) return false;
  reshade::api::resource_desc desc(
      capacity, reshade::api::memory_heap::gpu_to_cpu, reshade::api::resource_usage::copy_dest);
  reshade::api::resource staging = {0u};
  if (!device->create_resource(desc, nullptr, reshade::api::resource_usage::copy_dest, &staging)) return false;
  g_pool.frame_staging = staging;
  g_pool.frame_staging_capacity = capacity;
  g_pool.staging_device = device;
  return true;
}

inline void OnDestroyDevicePool(reshade::api::device* device) {
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  if (device != nullptr && g_pool.staging_device != device) return;
  if (g_pool.frame_staging.handle != 0u && g_pool.staging_device != nullptr) {
    g_pool.staging_device->destroy_resource(g_pool.frame_staging);
  }
  g_pool.frame_staging = {0u};
  g_pool.frame_staging_capacity = 0u;
  g_pool.staging_device = nullptr;
  g_pool.frame_staging_used = 0u;
  g_pool.frame_copy_count = 0u;
  g_pool.frame_snapshot_count = 0u;
  g_pool.frame_snapshots.clear();
  g_pool.pending_snapshots.clear();
}

inline void OnPoolScanDraw(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    const DrawRecord& draw,
    WorldCommandListData* cl_data,
    int32_t base_override = INT32_MIN) {
  if (!g_pool.scan_active.load(std::memory_order_relaxed)) return;
  if (cl_data == nullptr) return;
  // Pool geometry gate: the census gate in relaxed mode (no color target or
  // pixel shader required), because Auto Research verifies families from
  // depth-only prepass draws too. Duplicate geometry from shadow/prepass
  // passes dedupes on MeshKey/InstanceKey.
  if (!IsGeometryCandidate(draw, true)) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.skipped_not_candidate += 1u;
    g_pool.family_recipes[draw.vs_hash].skipped_not_candidate += 1u;
    return;
  }

  uint8_t kind = 0u;
  uint32_t slot = 0u;
  uint32_t recorded_offset = 0u;
  uint32_t stride = 0u;
  int32_t recorded_base = 0;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto family_it = g_state.families.find(draw.vs_hash);
    if (family_it == g_state.families.end()) return;
    const FamilyStats& family = family_it->second;
    if (!family.verified || !family.transform_found) {
      g_pool.stats.skipped_unverified += 1u;
      return;
    }
    const AutoFamilyResult* result = nullptr;
    for (const auto& candidate : g_state.auto_research.results) {
      if (candidate.vs_hash == draw.vs_hash) {
        result = &candidate;
        break;
      }
    }
    // Layout selection: prefer the engine's canonical instance path
    // (slot 15, stride 160, row kind) over coincidental probe matches. A
    // non-canonical layout is only used when it passed cross-capture
    // verification; otherwise the family is skipped rather than admitting
    // geometry with a wrong transform source.
    //
    // The transient Auto Research result list is cleared by every run and only
    // contains unverified families, so recipe-verified families fall back to
    // the persistent FamilyStats transform recorded by the recipe loader or
    // the verification pass.
    PassingLayout selected;
    bool have_layout = false;
    if (result != nullptr && SelectPoolInstanceLayout(result->passing_layouts, &selected)) {
      const bool canonical = selected.slot == kPoolInstanceSlot && selected.stride == kPoolInstanceStride;
      have_layout = canonical || result->cross_capture_pass;
    }
    if (!have_layout && family.transform_found && IsPoolStructuredKind(family.transform_kind)) {
      const bool canonical = family.transform_slot == kPoolInstanceSlot
                             && family.transform_stride == kPoolInstanceStride;
      if (canonical || family.transform_cross_capture) {
        selected.kind = static_cast<CandidateKind>(family.transform_kind);
        selected.slot = family.transform_slot;
        selected.matrix_offset = family.transform_offset;
        selected.stride = family.transform_stride;
        selected.base_offset = family.transform_base;
        have_layout = true;
      }
    }
    if (!have_layout) {
      g_pool.stats.skipped_non_instance += 1u;
      return;
    }
    kind = static_cast<uint8_t>(selected.kind);
    if (kind == static_cast<uint8_t>(CandidateKind::Inst4x3Col)) {
      kind = static_cast<uint8_t>(CandidateKind::Inst4x3Row);
    } else if (kind == static_cast<uint8_t>(CandidateKind::Inst4x4Col)) {
      kind = static_cast<uint8_t>(CandidateKind::Inst4x4Row);
    }
    slot = selected.slot;
    recorded_offset = selected.matrix_offset;
    stride = selected.stride;
    recorded_base = selected.base_offset;
  }
  if (slot >= kSrvSlotCapacity || stride == 0u) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.skipped_slot_invalid += 1u;
    g_pool.family_recipes[draw.vs_hash].skipped_slot_invalid += 1u;
    return;
  }
  const reshade::api::resource instance_buffer = cl_data->vs_srv[slot];
  if (instance_buffer.handle == 0u) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.skipped_no_srv += 1u;
    g_pool.family_recipes[draw.vs_hash].skipped_no_srv += 1u;
    return;
  }

  int32_t base = 0;
  const bool base_overridden = base_override != INT32_MIN;
  if (base_overridden) base = base_override;
  const reshade::api::resource instance_cb = cl_data->vs_cb[kPoolInstanceCbSlot];
  if (!base_overridden && instance_cb.handle != 0u && device != nullptr) {
    const auto bytes = renodx::utils::constants::GetResourceCache(device, instance_cb);
    if (bytes.size() >= sizeof(int32_t)) std::memcpy(&base, bytes.data(), sizeof(int32_t));
  }

  PoolScanEntry entry;
  entry.vs_hash = draw.vs_hash;
  entry.mesh_key = MeshKey(draw);
  entry.draw = draw;
  entry.instance_buffer = instance_buffer;
  entry.instance_cb = instance_cb;
  entry.kind = kind;
  entry.stride = stride;
  // Canonical recipe: the decompiled VS reads the world matrix at element
  // offset 0 and indexes with SV_InstanceID + runtime instanceOffset_g. The
  // Phase 0 selected offset can be a depth-coincidence pass (e.g. the whale
  // at offset 60), so it is recorded for audit but never used for extraction.
  entry.matrix_offset = 0u;
  entry.matrix_floats = KindMatrixFloats(static_cast<CandidateKind>(kind));
  entry.first_instance = draw.first_instance;
  entry.instance_count = draw.instance_count;
  entry.base = base;

  if (cmd_list == nullptr || device == nullptr) return;

  const uint64_t buffer_size = device->get_resource_desc(instance_buffer).buffer.size;
  if (buffer_size == 0u) {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.stats.skipped_no_buffer += 1u;
    g_pool.family_recipes[draw.vs_hash].skipped_no_buffer += 1u;
    return;
  }

  const uint64_t identity = PoolEntryIdentity(entry);
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  PoolFamilyRecipe& recipe = g_pool.family_recipes[draw.vs_hash];
  if (!recipe.initialized) {
    recipe.initialized = true;
    recipe.recorded_kind = kind;
    recipe.recorded_slot = slot;
    recipe.recorded_offset = recorded_offset;
    recipe.recorded_stride = stride;
    recipe.recorded_base = recorded_base;
    recipe.used_kind = kind;
    recipe.used_slot = slot;
    recipe.used_offset = 0u;
    recipe.used_stride = stride;
    recipe.used_base = base;
  }
  if (recorded_offset != 0u) recipe.canonical_override = true;
  const auto attempts_it = g_pool.entry_attempts.find(identity);
  if (attempts_it != g_pool.entry_attempts.end() && attempts_it->second.count >= kPoolMaxEntryAttempts) {
    g_pool.stats.skipped_satisfied += 1u;
    return;
  }

  // Whole-buffer snapshot, once per unique instance resource per frame: the
  // draw-time b1 selects an element far beyond the old cached-base range, so
  // the snapshot has to cover the buffer the draw actually indexed into.
  const uint64_t resource_key = instance_buffer.handle;
  auto snapshot_it = g_pool.frame_snapshots.find(resource_key);
  if (snapshot_it == g_pool.frame_snapshots.end()) {
    const uint64_t snapshot_size = (std::min)(buffer_size, kPoolMaxResourceSnapshotBytes);
    if (g_pool.frame_snapshot_count >= kPoolMaxSnapshotsPerFrame
        || g_pool.frame_staging_used + snapshot_size > kPoolMaxCopyBytesPerFrame) {
      g_pool.stats.copy_drops += 1u;
      return;
    }
    if (!EnsurePoolFrameStaging(device, g_pool.frame_staging_used + snapshot_size)) {
      g_pool.stats.copy_drops += 1u;
      return;
    }
    cmd_list->copy_buffer_region(
        instance_buffer, 0u, g_pool.frame_staging, g_pool.frame_staging_used, snapshot_size);
    PoolResourceSnapshot snapshot;
    snapshot.resource = instance_buffer;
    snapshot.staging_offset = g_pool.frame_staging_used;
    snapshot.size = snapshot_size;
    g_pool.frame_staging_used += snapshot_size;
    g_pool.frame_snapshot_count += 1u;
    snapshot_it = g_pool.frame_snapshots.emplace(resource_key, snapshot).first;
  }

  uint64_t cb_copy_size = 0u;
  if (!base_overridden && instance_cb.handle != 0u) {
    const uint64_t cb_buffer_size = device->get_resource_desc(instance_cb).buffer.size;
    cb_copy_size = (std::min)(cb_buffer_size, static_cast<uint64_t>(16u));
    if (cb_copy_size < sizeof(int32_t)) cb_copy_size = 0u;
  }
  if (g_pool.frame_copy_count >= kPoolMaxCopiesPerFrame
      || g_pool.frame_staging_used + cb_copy_size > kPoolMaxCopyBytesPerFrame) {
    g_pool.stats.copy_drops += 1u;
    return;
  }
  if (!EnsurePoolFrameStaging(device, g_pool.frame_staging_used + cb_copy_size)) {
    g_pool.stats.copy_drops += 1u;
    return;
  }
  if (g_pool.mesh_by_key.count(entry.mesh_key) == 0u
      && g_pool.failed_meshes.count(entry.mesh_key) == 0u
      && g_pool.mesh_queued.count(entry.mesh_key) == 0u) {
    g_pool.mesh_queued.insert(entry.mesh_key);
    g_pool.mesh_queue.push_back(entry);
  }
  PoolPendingCopy pending;
  pending.entry = entry;
  pending.snapshot_offset = snapshot_it->second.staging_offset;
  pending.snapshot_size = snapshot_it->second.size;
  pending.has_base_override = base_overridden;
  if (cb_copy_size != 0u) {
    // Draw-time b1 snapshot: the constants cache can miss this buffer
    // entirely, which would silently read element 0 for every family.
    cmd_list->copy_buffer_region(instance_cb, 0u, g_pool.frame_staging, g_pool.frame_staging_used, cb_copy_size);
    pending.cb_staging_offset = g_pool.frame_staging_used;
    pending.cb_size = static_cast<uint32_t>(cb_copy_size);
    pending.has_cb = true;
    g_pool.frame_staging_used += cb_copy_size;
  }
  g_pool.frame_copy_count += 1u;
  g_pool.pending_snapshots.push_back(std::move(pending));
}

inline void DrainPoolScan(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return;
  if (!g_pool.scan_active.load(std::memory_order_relaxed)) return;
  const uint32_t frame = g_state.frame.load();
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (g_pool.stats.scan_first_frame == 0u) g_pool.stats.scan_first_frame = frame;
    g_pool.stats.scan_last_frame = frame;
  }

  CaptureOnePoolMesh(device, queue);

  std::vector<PoolPendingCopy> pending;
  reshade::api::resource staging = {0u};
  uint64_t staging_used = 0u;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    pending.swap(g_pool.pending_snapshots);
    staging = g_pool.frame_staging;
    staging_used = g_pool.frame_staging_used;
    g_pool.stats.queued = 0u;
  }

  if (!pending.empty()) {
    void* mapped = nullptr;
    const bool mapped_ok = staging.handle != 0u && staging_used != 0u
                           && device->map_buffer_region(
                                  staging, 0u, staging_used, reshade::api::map_access::read_only, &mapped)
                           && mapped != nullptr;
    if (mapped_ok) {
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      for (const auto& copy : pending) {
        const auto* bytes = static_cast<const uint8_t*>(mapped) + copy.snapshot_offset;
        if (copy.has_base_override) {
          ExtractPoolInstances(copy.entry, copy.entry.base, bytes, copy.snapshot_size, frame);
          continue;
        }
        if (!copy.has_cb || copy.cb_size < sizeof(int32_t)) {
          g_pool.stats.skipped_no_base += 1u;
          continue;
        }
        int32_t base = 0;
        std::memcpy(&base, static_cast<const uint8_t*>(mapped) + copy.cb_staging_offset, sizeof(int32_t));
        ExtractPoolInstances(copy.entry, base, bytes, copy.snapshot_size, frame);
      }
      device->unmap_buffer_region(staging);
    }
  }

  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    // Attempts count scheduling opportunities, not copies: many draws of the
    // same structural identity in one frame must consume a single attempt so
    // each of their b1 elements still gets its three stable observations.
    std::unordered_set<uint64_t> attempted_identities;
    attempted_identities.reserve(pending.size());
    for (const auto& copy : pending) {
      const uint64_t identity = PoolEntryIdentity(copy.entry);
      if (!attempted_identities.insert(identity).second) continue;
      PoolEntryAttempt& attempt = g_pool.entry_attempts[identity];
      if (attempt.count < kPoolMaxEntryAttempts) attempt.count += 1u;
      attempt.mesh_key = copy.entry.mesh_key;
    }
    for (auto it = g_pool.observations.begin(); it != g_pool.observations.end();) {
      if ((it->second.rejected || !it->second.admitted) && frame > it->second.last_frame + kPoolPruneAge) {
        it = g_pool.observations.erase(it);
      } else {
        ++it;
      }
    }
    g_pool.frame_staging_used = 0u;
    g_pool.frame_copy_count = 0u;
    g_pool.frame_snapshot_count = 0u;
    g_pool.frame_snapshots.clear();
    UpdatePoolStats();
  }
}

inline void ResetWorldPool() {
  std::lock_guard<std::mutex> lock(g_pool.mutex);
  g_pool.pending_snapshots.clear();
  g_pool.mesh_queue.clear();
  g_pool.mesh_queued.clear();
  g_pool.failed_meshes.clear();
  g_pool.mesh_by_key.clear();
  g_pool.meshes.clear();
  g_pool.observations.clear();
  g_pool.instances.clear();
  g_pool.family_recipes.clear();
  g_pool.entry_attempts.clear();
  g_pool.frame_snapshots.clear();
  g_pool.frame_staging_used = 0u;
  g_pool.frame_copy_count = 0u;
  g_pool.frame_snapshot_count = 0u;
  g_pool.revision += 1u;
  g_pool.stats = {};
}

inline std::filesystem::path PoolOutputDir() {
  auto path = renodx::utils::path::GetOutputPath() / "falcomengine-plus" / "world";
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  return path;
}

inline void DumpWorldPool() {
  std::vector<WorldMesh> meshes;
  std::vector<WorldInstance> instances;
  std::unordered_map<uint32_t, PoolFamilyRecipe> recipes;
  PoolStats stats;
  PoolRegion region;
  const PoolCameraInfo camera = GetPoolCameraInfo();
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    UpdatePoolStats();
    meshes = g_pool.meshes;
    instances = g_pool.instances;
    recipes = g_pool.family_recipes;
    stats = g_pool.stats;
    region = CurrentPoolRegion();
  }

  struct FamilyAggregate {
    uint32_t meshes = 0u;
    uint32_t observed = 0u;
    uint32_t admitted = 0u;
    uint32_t in_region = 0u;
    float bounds_min[3] = {1e30f, 1e30f, 1e30f};
    float bounds_max[3] = {-1e30f, -1e30f, -1e30f};
  };
  std::unordered_map<uint32_t, FamilyAggregate> families;
  for (const auto& mesh : meshes) {
    families[mesh.source_vs_hash].meshes += 1u;
  }
  for (const auto& instance : instances) {
    auto& aggregate = families[instance.source_vs_hash];
    aggregate.admitted += 1u;
    if (PoolInstanceInRegion(instance, region)) aggregate.in_region += 1u;
    for (int i = 0; i < 3; ++i) {
      aggregate.bounds_min[i] = (std::min)(aggregate.bounds_min[i], instance.bounds_min[i]);
      aggregate.bounds_max[i] = (std::max)(aggregate.bounds_max[i], instance.bounds_max[i]);
    }
  }
  for (const auto& [vs_hash, recipe] : recipes) {
    (void)recipe;
    families[vs_hash];
  }

  std::ostringstream out;
  out << "{\n";
  out << "  \"generated_frame\": " << g_state.frame.load() << ",\n";
  out << "  \"camera_valid\": " << (camera.valid ? "true" : "false") << ",\n";
  out << "  \"view_inv_translation_row\": [" << camera.row[0] << ", " << camera.row[1] << ", " << camera.row[2] << "],\n";
  out << "  \"view_inv_translation_legacy\": [" << camera.legacy[0] << ", " << camera.legacy[1] << ", " << camera.legacy[2] << "],\n";
  out << "  \"camera_position\": [" << camera.position[0] << ", " << camera.position[1] << ", " << camera.position[2] << "],\n";
  out << "  \"region\": {\"size\": " << region.size
      << ", \"min\": [" << region.min[0] << ", " << region.min[1] << ", " << region.min[2]
      << "], \"max\": [" << region.max[0] << ", " << region.max[1] << ", " << region.max[2] << "]},\n";
  out << "  \"stats\": {\"meshes\": " << stats.meshes
      << ", \"observed_instances\": " << stats.observed
      << ", \"admitted_instances\": " << stats.admitted
      << ", \"region_instances\": " << stats.region
      << ", \"skipped_unverified\": " << stats.skipped_unverified
      << ", \"skipped_non_instance\": " << stats.skipped_non_instance
      << ", \"skipped_satisfied\": " << stats.skipped_satisfied
      << ", \"queue_drops\": " << stats.queue_drops
      << ", \"copy_drops\": " << stats.copy_drops
      << ", \"skipped_no_base\": " << stats.skipped_no_base
      << ", \"skipped_beyond_snapshot\": " << stats.skipped_beyond_snapshot
      << ", \"mesh_failures\": " << stats.mesh_failures
      << ", \"skipped_not_candidate\": " << stats.skipped_not_candidate
      << ", \"skipped_no_srv\": " << stats.skipped_no_srv
      << ", \"skipped_slot_invalid\": " << stats.skipped_slot_invalid
      << ", \"skipped_no_buffer\": " << stats.skipped_no_buffer
      << ", \"scan_first_frame\": " << stats.scan_first_frame
      << ", \"scan_last_frame\": " << stats.scan_last_frame
      << ", \"indirect_dropped_calls\": " << IndirectDroppedCalls() << "},\n";
  out << "  \"families\": [";
  bool first = true;
  for (const auto& [vs_hash, aggregate] : families) {
    if (!first) out << ",";
    first = false;
    char hash_text[16] = {};
    std::snprintf(hash_text, sizeof(hash_text), "0x%08X", vs_hash);
    const auto recipe_it = recipes.find(vs_hash);
    out << "\n    {\"vs_hash\": \"" << hash_text << "\""
        << ", \"meshes\": " << aggregate.meshes
        << ", \"admitted\": " << aggregate.admitted
        << ", \"region\": " << aggregate.in_region
        << ", \"bounds_min\": [" << aggregate.bounds_min[0] << ", " << aggregate.bounds_min[1] << ", " << aggregate.bounds_min[2] << "]"
        << ", \"bounds_max\": [" << aggregate.bounds_max[0] << ", " << aggregate.bounds_max[1] << ", " << aggregate.bounds_max[2] << "]";
    if (recipe_it != recipes.end()) {
      const PoolFamilyRecipe& recipe = recipe_it->second;
      out << ", \"skipped_not_candidate\": " << recipe.skipped_not_candidate
          << ", \"skipped_no_srv\": " << recipe.skipped_no_srv
          << ", \"skipped_slot_invalid\": " << recipe.skipped_slot_invalid
          << ", \"skipped_no_buffer\": " << recipe.skipped_no_buffer;
      if (!recipe.initialized) {
        out << "}";
        continue;
      }
      out << ", \"recorded_recipe\": {\"kind\": \"" << CandidateKindName(static_cast<CandidateKind>(recipe.recorded_kind))
          << "\", \"slot\": " << recipe.recorded_slot
          << ", \"offset\": " << recipe.recorded_offset
          << ", \"stride\": " << recipe.recorded_stride
          << ", \"base\": " << recipe.recorded_base << "}"
          << ", \"used_recipe\": {\"kind\": \"" << CandidateKindName(static_cast<CandidateKind>(recipe.used_kind))
          << "\", \"slot\": " << recipe.used_slot
          << ", \"offset\": " << recipe.used_offset
          << ", \"stride\": " << recipe.used_stride
          << ", \"base\": " << recipe.used_base << "}"
          << ", \"canonical_override\": " << (recipe.canonical_override ? "true" : "false")
          << ", \"rejected_implausible\": " << recipe.rejected_implausible
          << ", \"translation_min\": [" << recipe.translation_min[0] << ", " << recipe.translation_min[1] << ", " << recipe.translation_min[2] << "]"
          << ", \"translation_max\": [" << recipe.translation_max[0] << ", " << recipe.translation_max[1] << ", " << recipe.translation_max[2] << "]";
      if (recipe.has_sample_matrix) {
        float sample_translation[3] = {};
        PoolMatrixTranslation(recipe.used_kind, recipe.sample_matrix, sample_translation);
        out << ", \"sample_matrix\": [";
        for (int i = 0; i < 16; ++i) {
          if (i != 0) out << ", ";
          out << recipe.sample_matrix[i];
        }
        out << "]"
            << ", \"sample_translation\": [" << sample_translation[0] << ", " << sample_translation[1] << ", " << sample_translation[2] << "]";
      }
      out << ", \"base_min\": " << recipe.base_min
          << ", \"base_max\": " << recipe.base_max
          << ", \"base_observed_count\": " << recipe.base_observed_count;
      if (recipe.has_sample_base) out << ", \"sample_base\": " << recipe.sample_base;
    }
    out << "}";
  }
  out << "\n  ],\n";
  out << "  \"meshes\": [";
  first = true;
  for (const auto& mesh : meshes) {
    if (!first) out << ",";
    first = false;
    out << "\n    {\"mesh_id\": " << mesh.mesh_id
        << ", \"mesh_key\": \"" << mesh.mesh_key << "\""
        << ", \"vs_hash\": " << mesh.source_vs_hash
        << ", \"vertices\": " << mesh.positions.size()
        << ", \"triangles\": " << mesh.triangle_count
        << ", \"bbox_min\": [" << mesh.bbox_min[0] << ", " << mesh.bbox_min[1] << ", " << mesh.bbox_min[2] << "]"
        << ", \"bbox_max\": [" << mesh.bbox_max[0] << ", " << mesh.bbox_max[1] << ", " << mesh.bbox_max[2] << "]}";
  }
  out << "\n  ]\n}\n";

  std::string text = out.str();
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_pool.json", text);
}

inline void DumpWorldPoolObj(size_t max_vertices = 2000000u) {
  std::vector<WorldMesh> meshes;
  std::vector<WorldInstance> instances;
  PoolRegion region;
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    meshes = g_pool.meshes;
    instances = g_pool.instances;
    region = CurrentPoolRegion();
  }

  std::ostringstream out;
  out << "# falcomengine-plus world pool (region instances, world space)\n";
  size_t vertex_base = 1u;
  size_t written = 0u;
  for (const auto& instance : instances) {
    if (!PoolInstanceInRegion(instance, region)) continue;
    if (instance.mesh_id >= meshes.size()) continue;
    const WorldMesh& mesh = meshes[instance.mesh_id];
    if (written + mesh.positions.size() > max_vertices) break;
    for (const auto& position : mesh.positions) {
      float world[3] = {};
      TransformPoolPoint(instance.kind, instance.matrix, position.data(), world);
      out << "v " << world[0] << " " << world[1] << " " << world[2] << "\n";
    }
    for (size_t i = 0u; i + 2u < mesh.indices.size(); i += 3u) {
      out << "f " << (vertex_base + mesh.indices[i])
          << " " << (vertex_base + mesh.indices[i + 1u])
          << " " << (vertex_base + mesh.indices[i + 2u]) << "\n";
    }
    vertex_base += mesh.positions.size();
    written += mesh.positions.size();
  }
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_pool_region.obj", text);
}

}  // namespace falcom_world::bvh
