#pragma once

// Candidate world-transform discovery for Phase 0.
//
// Two sources are scanned, in this order:
//   1. Structured instance buffers (VS SRV snapshots), because the tagged
//      Sora 2nd geometry shaders read per-instance world matrices from one.
//   2. Constant buffers (legacy fallback for families that use CB transforms).
//
// The 160-byte InstanceParam layout is a hint, never a requirement. Structured
// buffers use the resource's actual stride; offsets, matrix size and vector
// convention are scanned; base offsets come from the runtime b1 int plus small
// ints found in captured CBs. Acceptance requires set-level agreement: many
// instances of the same draw must project correctly, not just one.

#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_set>

#include "../world_state.hpp"

namespace falcom_world {

inline void TransformRow(const float* m, float x, float y, float z, float w, float* out) {
  out[0] = x * m[0] + y * m[4] + z * m[8] + w * m[12];
  out[1] = x * m[1] + y * m[5] + z * m[9] + w * m[13];
  out[2] = x * m[2] + y * m[6] + z * m[10] + w * m[14];
  out[3] = x * m[3] + y * m[7] + z * m[11] + w * m[15];
}

inline void TransformCol(const float* m, float x, float y, float z, float w, float* out) {
  out[0] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
  out[1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
  out[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
  out[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;
}

// float4x3 row-major storage with row-vector multiply (matches the tagged
// instance shaders: translation in m[9..11]).
inline void TransformInst4x3Row(const float* m, float x, float y, float z, float* out) {
  out[0] = x * m[0] + y * m[3] + z * m[6] + m[9];
  out[1] = x * m[1] + y * m[4] + z * m[7] + m[10];
  out[2] = x * m[2] + y * m[5] + z * m[8] + m[11];
}

// float4x3 column-major storage with column-vector multiply.
inline void TransformInst4x3Col(const float* m, float x, float y, float z, float* out) {
  out[0] = m[0] * x + m[1] * y + m[2] * z + m[3];
  out[1] = m[4] * x + m[5] * y + m[6] * z + m[7];
  out[2] = m[8] * x + m[9] * y + m[10] * z + m[11];
}

inline bool IsCameraMatrix(const float* m, const CameraSnapshot& camera) {
  const auto same = [m](const float* other) {
    for (int i = 0; i < 16; ++i) {
      if (std::fabs(m[i] - other[i]) > 1e-6f) return false;
    }
    return true;
  };
  if (same(camera.view)) return true;
  if (same(camera.proj)) return true;
  if (same(camera.view_proj)) return true;
  if (same(camera.view_proj_inv)) return true;
  if (camera.has_prev && same(camera.prev_view_proj)) return true;
  return false;
}

inline uint64_t MatrixHash(const float* m, uint32_t count) {
  uint64_t hash = 1469598103934665603ull;
  for (uint32_t i = 0; i < count; ++i) {
    uint32_t bits = 0u;
    std::memcpy(&bits, m + i, sizeof(uint32_t));
    hash ^= bits;
    hash *= 1099511628211ull;
  }
  return hash;
}

struct SetScore {
  float set_score = 0.f;
  float mean_inside = 0.f;
  float bbox_area = 0.f;
  uint32_t instances_ok = 0u;
  uint32_t instances_tested = 0u;
  bool valid = false;
};

// Projects one instance of a candidate. For CB kinds the matrix maps straight
// to clip; for instance kinds the matrix maps model->world and the camera
// view-projection is applied afterwards.
inline bool ProjectCandidateInstance(
    const TransformCandidate& candidate,
    const CameraSnapshot& camera,
    uint32_t instance,
    const std::array<float, 3>& point,
    float* out_uv,
    float* out_world) {
  if (out_uv == nullptr || out_world == nullptr) return false;
  const uint32_t floats = KindMatrixFloats(candidate.kind);
  if (candidate.matrices.size() < (static_cast<size_t>(instance) + 1u) * floats) return false;
  const float* matrix = candidate.matrices.data() + static_cast<size_t>(instance) * floats;

  float clip[4] = {};
  const bool row_vector = IsRowVectorKind(candidate.kind);
  if (candidate.kind == CandidateKind::Cb4x4Row) {
    TransformRow(matrix, point[0], point[1], point[2], 1.f, clip);
    out_world[0] = point[0];
    out_world[1] = point[1];
    out_world[2] = point[2];
  } else if (candidate.kind == CandidateKind::Cb4x4Col) {
    TransformCol(matrix, point[0], point[1], point[2], 1.f, clip);
    out_world[0] = point[0];
    out_world[1] = point[1];
    out_world[2] = point[2];
  } else {
    float world[3] = {};
    if (candidate.kind == CandidateKind::Inst4x3Row) {
      TransformInst4x3Row(matrix, point[0], point[1], point[2], world);
    } else if (candidate.kind == CandidateKind::Inst4x3Col) {
      TransformInst4x3Col(matrix, point[0], point[1], point[2], world);
    } else {
      float world4[4] = {};
      if (candidate.kind == CandidateKind::Inst4x4Row) {
        TransformRow(matrix, point[0], point[1], point[2], 1.f, world4);
      } else {
        TransformCol(matrix, point[0], point[1], point[2], 1.f, world4);
      }
      world[0] = world4[0];
      world[1] = world4[1];
      world[2] = world4[2];
    }
    if (row_vector) {
      TransformRow(camera.view_proj, world[0], world[1], world[2], 1.f, clip);
    } else {
      TransformCol(camera.view_proj, world[0], world[1], world[2], 1.f, clip);
    }
    out_world[0] = world[0];
    out_world[1] = world[1];
    out_world[2] = world[2];
  }

  if (!std::isfinite(clip[0]) || !std::isfinite(clip[1]) || !std::isfinite(clip[2]) || !std::isfinite(clip[3])) return false;
  if (clip[3] <= 1e-6f) return false;
  const float inv_w = 1.f / clip[3];
  out_uv[0] = clip[0] * inv_w * 0.5f + 0.5f;
  out_uv[1] = -clip[1] * inv_w * 0.5f + 0.5f;
  return true;
}

inline bool ProjectCandidatePrevInstance(
    const TransformCandidate& candidate,
    const CameraSnapshot& camera,
    uint32_t instance,
    const std::array<float, 3>& point,
    float* out_uv,
    float* out_world) {
  if (out_uv == nullptr || out_world == nullptr) return false;
  if (!candidate.has_prev || !IsStructuredKind(candidate.kind)) return false;
  const size_t offset = static_cast<size_t>(instance) * 12u;
  if (candidate.prev_matrices.size() < offset + 12u) return false;
  const float* matrix = candidate.prev_matrices.data() + offset;

  float world[3] = {};
  if (candidate.kind == CandidateKind::Inst4x3Row || candidate.kind == CandidateKind::Inst4x4Row) {
    TransformInst4x3Row(matrix, point[0], point[1], point[2], world);
  } else {
    TransformInst4x3Col(matrix, point[0], point[1], point[2], world);
  }
  float clip[4] = {};
  if (IsRowVectorKind(candidate.kind)) {
    TransformRow(camera.view_proj, world[0], world[1], world[2], 1.f, clip);
  } else {
    TransformCol(camera.view_proj, world[0], world[1], world[2], 1.f, clip);
  }
  if (!std::isfinite(clip[0]) || !std::isfinite(clip[1]) || !std::isfinite(clip[2]) || !std::isfinite(clip[3])) return false;
  if (clip[3] <= 1e-6f) return false;
  const float inv_w = 1.f / clip[3];
  out_uv[0] = clip[0] * inv_w * 0.5f + 0.5f;
  out_uv[1] = -clip[1] * inv_w * 0.5f + 0.5f;
  out_world[0] = world[0];
  out_world[1] = world[1];
  out_world[2] = world[2];
  return true;
}

inline SetScore ScoreCandidateSet(
    const TransformCandidate& candidate,
    const CameraSnapshot& camera,
    const std::vector<std::array<float, 3>>& positions,
    uint32_t max_instances,
    uint32_t max_samples) {
  SetScore result = {};
  if (positions.empty()) return result;
  const bool needs_camera = IsStructuredKind(candidate.kind);
  if (needs_camera && !camera.valid) return result;

  const uint32_t instances = (std::min)(candidate.element_count, max_instances);
  if (instances == 0u) return result;
  result.instances_tested = instances;

  const size_t count = positions.size();
  const size_t stride = count > max_samples ? ((count + max_samples - 1u) / max_samples) : 1u;
  float sum_inside = 0.f;
  float min_u = 1e9f, max_u = -1e9f, min_v = 1e9f, max_v = -1e9f;

  for (uint32_t instance = 0; instance < instances; ++instance) {
    size_t sampled = 0u;
    size_t valid_count = 0u;
    size_t inside_count = 0u;
    for (size_t i = 0u; i < count; i += stride) {
      float uv[2] = {};
      float world[3] = {};
      sampled += 1u;
      if (!ProjectCandidateInstance(candidate, camera, instance, positions[i], uv, world)) continue;
      valid_count += 1u;
      if (uv[0] > -0.05f && uv[0] < 1.05f && uv[1] > -0.05f && uv[1] < 1.05f) {
        inside_count += 1u;
        min_u = (std::min)(min_u, uv[0]);
        max_u = (std::max)(max_u, uv[0]);
        min_v = (std::min)(min_v, uv[1]);
        max_v = (std::max)(max_v, uv[1]);
      }
    }
    if (sampled == 0u) continue;
    const float valid_fraction = static_cast<float>(valid_count) / static_cast<float>(sampled);
    const float inside_fraction = static_cast<float>(inside_count) / static_cast<float>(sampled);
    if (valid_fraction >= 0.9f && inside_fraction >= 0.8f) result.instances_ok += 1u;
    sum_inside += inside_fraction;
  }

  if (result.instances_tested == 0u) return result;
  result.mean_inside = sum_inside / static_cast<float>(result.instances_tested);
  result.bbox_area = (max_u - min_u) * (max_v - min_v);
  const float ok_fraction = static_cast<float>(result.instances_ok) / static_cast<float>(result.instances_tested);
  if (ok_fraction < 0.9f) return result;
  if (result.bbox_area <= 1e-4f || result.bbox_area >= 4.f) return result;
  result.set_score = ok_fraction
                     + (std::min)(result.mean_inside, 1.f) * 0.1f
                     + (std::min)(result.bbox_area * 2.f, 1.f) * 0.05f;
  result.valid = true;
  return result;
}

inline std::vector<int32_t> CollectBaseOffsets(const CapturedDraw& captured) {
  std::vector<int32_t> bases;
  const auto add = [&bases](int32_t value) {
    if (value < -256 || value > 4096) return;
    if (std::find(bases.begin(), bases.end(), value) != bases.end()) return;
    if (bases.size() < 16u) bases.push_back(value);
  };
  if (captured.instance_offset_found) add(captured.instance_offset_g);
  add(0);
  for (const auto& cb : captured.cbs) {
    const size_t words = cb.bytes.size() / sizeof(int32_t);
    const auto* data = reinterpret_cast<const int32_t*>(cb.bytes.data());
    for (size_t i = 0; i < words && bases.size() < 16u; ++i) {
      add(data[i]);
    }
  }
  return bases;
}

inline bool ExtractInstanceMatrices(
    const SrvBufferSnapshot& buffer,
    const DrawRecord& draw,
    uint32_t base_offset,
    uint32_t matrix_offset,
    uint32_t matrix_floats,
    uint32_t element_count,
    std::vector<float>* out_matrices,
    std::vector<float>* out_prev) {
  if (out_matrices == nullptr || out_prev == nullptr) return false;
  out_matrices->clear();
  out_prev->clear();
  if (buffer.stride == 0u) return false;

  const uint64_t matrix_bytes = static_cast<uint64_t>(matrix_floats) * sizeof(float);
  for (uint32_t i = 0; i < element_count; ++i) {
    const int64_t element = static_cast<int64_t>(draw.first_instance) + static_cast<int64_t>(base_offset) + i;
    if (element < 0) return false;
    const uint64_t byte = static_cast<uint64_t>(element) * buffer.stride + matrix_offset;
    if (byte < buffer.read_offset) return false;
    const uint64_t relative = byte - buffer.read_offset;
    if (relative + matrix_bytes > buffer.bytes.size()) return false;
    const auto* floats = reinterpret_cast<const float*>(buffer.bytes.data() + relative);
    out_matrices->insert(out_matrices->end(), floats, floats + matrix_floats);

    // prevWorld is recorded when the hinted 4x3 layout fits; it is display
    // evidence only and never affects candidate acceptance.
    if (matrix_floats == 12u && matrix_offset + 48u + 48u <= buffer.stride) {
      const uint64_t prev_byte = static_cast<uint64_t>(element) * buffer.stride + matrix_offset + 48u;
      if (prev_byte >= buffer.read_offset) {
        const uint64_t prev_relative = prev_byte - buffer.read_offset;
        if (prev_relative + 48u <= buffer.bytes.size()) {
          const auto* prev = reinterpret_cast<const float*>(buffer.bytes.data() + prev_relative);
          out_prev->insert(out_prev->end(), prev, prev + 12);
        }
      }
    }
  }
  return !out_matrices->empty();
}

inline void PushStructuredCandidate(
    std::vector<TransformCandidate>* candidates,
    const SrvBufferSnapshot& buffer,
    const DrawRecord& draw,
    const CameraSnapshot& camera,
    const std::vector<std::array<float, 3>>& positions,
    CandidateKind kind,
    uint32_t matrix_offset,
    uint32_t base_offset,
    int32_t preferred_base) {
  const uint32_t matrix_floats = KindMatrixFloats(kind);
  const uint32_t element_count = (std::min)(
      draw.instance_count == 0u ? 1u : draw.instance_count, kMaxCandidateInstances);
  TransformCandidate candidate;
  candidate.kind = kind;
  candidate.stage = buffer.stage;
  candidate.slot = buffer.slot;
  candidate.matrix_offset = matrix_offset;
  candidate.stride = buffer.stride;
  candidate.base_offset = base_offset;
  candidate.element_count = element_count;
  if (!ExtractInstanceMatrices(
          buffer, draw, base_offset, matrix_offset, matrix_floats, element_count,
          &candidate.matrices, &candidate.prev_matrices)) {
    return;
  }
  candidate.has_prev = !candidate.prev_matrices.empty();

  const SetScore score = ScoreCandidateSet(candidate, camera, positions, 32u, 32u);
  if (!score.valid) return;
  candidate.set_score = score.set_score;
  candidate.mean_inside = score.mean_inside;
  candidate.bbox_area = score.bbox_area;
  candidate.instances_ok = score.instances_ok;
  candidate.valid = true;

  // Ordering bonus for the runtime b1 offset; acceptance is unchanged.
  if (preferred_base >= 0 && candidate.base_offset == preferred_base) {
    candidate.set_score += 0.05f;
  }
  candidates->push_back(std::move(candidate));
}

inline void PushConstantBufferCandidate(
    std::vector<TransformCandidate>* candidates,
    const CbSnapshot& cb,
    const CameraSnapshot& camera,
    const std::vector<std::array<float, 3>>& positions,
    const float* matrix,
    CandidateKind kind) {
  TransformCandidate candidate;
  candidate.kind = kind;
  candidate.stage = cb.stage;
  candidate.slot = cb.slot;
  candidate.element_count = 1u;
  candidate.matrices.assign(matrix, matrix + 16u);
  const SetScore score = ScoreCandidateSet(candidate, camera, positions, 1u, 128u);
  if (!score.valid) return;
  candidate.set_score = score.set_score;
  candidate.mean_inside = score.mean_inside;
  candidate.bbox_area = score.bbox_area;
  candidate.instances_ok = score.instances_ok;
  candidate.valid = true;
  candidates->push_back(std::move(candidate));
}

inline void ScanCandidates() {
  std::lock_guard<std::mutex> lock(g_state.mutex);
  g_state.candidates.clear();
  g_state.selected_candidate = 0;

  const CapturedDraw& captured = g_state.captured;
  if (!captured.mesh_valid || captured.mesh.positions.empty()) {
    g_state.status = "scan failed: no captured mesh";
    return;
  }
  const CameraSnapshot& camera = g_state.camera;
  const auto& positions = captured.mesh.positions;

  std::vector<TransformCandidate> candidates;
  std::unordered_set<uint64_t> seen_matrices;

  // -- Structured instance buffers --
  const std::vector<int32_t> bases = CollectBaseOffsets(captured);
  const int32_t preferred_base = captured.instance_offset_found ? captured.instance_offset_g : -1;
  for (const auto& buffer : captured.srv_buffers) {
    if (!buffer.valid || buffer.stride == 0u || buffer.bytes.size() < 64u) continue;

    std::vector<uint32_t> offsets;
    offsets.push_back(0u);
    for (uint32_t offset = 4u; offset + 12u <= buffer.stride; offset += 4u) {
      offsets.push_back(offset);
    }

    for (const uint32_t offset : offsets) {
      const bool can_4x3 = (offset + 48u <= buffer.stride);
      const bool can_4x4 = (offset + 64u <= buffer.stride);
      for (const int32_t base : bases) {
        const uint32_t base_u = static_cast<uint32_t>(base < 0 ? 0 : base);
        if (can_4x3) {
          PushStructuredCandidate(&candidates, buffer, captured.draw, camera, positions, CandidateKind::Inst4x3Row, offset, base_u, preferred_base);
          PushStructuredCandidate(&candidates, buffer, captured.draw, camera, positions, CandidateKind::Inst4x3Col, offset, base_u, preferred_base);
        }
        if (can_4x4) {
          PushStructuredCandidate(&candidates, buffer, captured.draw, camera, positions, CandidateKind::Inst4x4Row, offset, base_u, preferred_base);
          PushStructuredCandidate(&candidates, buffer, captured.draw, camera, positions, CandidateKind::Inst4x4Col, offset, base_u, preferred_base);
        }
        if (candidates.size() >= 4096u) break;
      }
      if (candidates.size() >= 4096u) break;
    }
  }

  // -- Constant buffers (fallback for families that use CB transforms) --
  for (const auto& cb : captured.cbs) {
    const size_t float_count = cb.bytes.size() / sizeof(float);
    if (float_count < 16u) continue;
    const auto* data = reinterpret_cast<const float*>(cb.bytes.data());
    for (size_t offset = 0u; offset + 16u <= float_count; offset += 4u) {
      float matrix[16] = {};
      std::memcpy(matrix, data + offset, sizeof(float) * 16u);
      bool all_zero = true;
      for (int i = 0; i < 16; ++i) {
        if (matrix[i] != 0.f) {
          all_zero = false;
          break;
        }
      }
      if (all_zero) continue;
      if (IsCameraMatrix(matrix, camera)) continue;
      if (!seen_matrices.insert(MatrixHash(matrix, 16u)).second) continue;
      PushConstantBufferCandidate(&candidates, cb, camera, positions, matrix, CandidateKind::Cb4x4Row);
      PushConstantBufferCandidate(&candidates, cb, camera, positions, matrix, CandidateKind::Cb4x4Col);
    }
  }

  std::sort(candidates.begin(), candidates.end(), [](const TransformCandidate& a, const TransformCandidate& b) {
    if (a.set_score != b.set_score) return a.set_score > b.set_score;
    return a.base_offset < b.base_offset;
  });
  if (candidates.size() > kMaxCandidates) candidates.resize(kMaxCandidates);
  g_state.candidates = std::move(candidates);
  g_state.status = "scan complete: " + std::to_string(g_state.candidates.size()) + " candidates";
}

inline CandidateSummary MakeCandidateSummary(const TransformCandidate& candidate) {
  CandidateSummary summary;
  summary.kind = candidate.kind;
  summary.stage = candidate.stage;
  summary.slot = candidate.slot;
  summary.matrix_offset = candidate.matrix_offset;
  summary.stride = candidate.stride;
  summary.base_offset = candidate.base_offset;
  summary.element_count = candidate.element_count;
  summary.instances_ok = candidate.instances_ok;
  summary.has_prev = candidate.has_prev;
  summary.set_score = candidate.set_score;
  summary.mean_inside = candidate.mean_inside;
  return summary;
}

}  // namespace falcom_world
