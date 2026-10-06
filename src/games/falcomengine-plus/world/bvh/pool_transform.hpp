#pragma once

// Instance transforms used by the world pool.
//
// Pool instances are Inst4x3Row: the 12 floats of InstanceParam.world as
// stored, world.x = dot(float4(p, 1), m[0..3]) (row-dot). Verified against the
// Sora 2nd instanced vertex shaders (t15, stride 160, b1 offset). The other
// forms stay for CandidateKind values other layouts may use.

#include <cstdint>
#include <cstring>

namespace falcom_world {

inline void TransformRowDot4(const float* m, float x, float y, float z, float w, float* out) {
  out[0] = m[0] * x + m[1] * y + m[2] * z + m[3] * w;
  out[1] = m[4] * x + m[5] * y + m[6] * z + m[7] * w;
  out[2] = m[8] * x + m[9] * y + m[10] * z + m[11] * w;
  out[3] = m[12] * x + m[13] * y + m[14] * z + m[15] * w;
}

inline void TransformInst4x3RowDot(const float* m, float x, float y, float z, float* out) {
  out[0] = m[0] * x + m[1] * y + m[2] * z + m[3];
  out[1] = m[4] * x + m[5] * y + m[6] * z + m[7];
  out[2] = m[8] * x + m[9] * y + m[10] * z + m[11];
}

// Column-dot alternative.
inline void TransformCol(const float* m, float x, float y, float z, float w, float* out) {
  out[0] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
  out[1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
  out[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
  out[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;
}

// 4x3 column-major alternative.
inline void TransformInst4x3Col(const float* m, float x, float y, float z, float* out) {
  out[0] = x * m[0] + y * m[3] + z * m[6] + m[9];
  out[1] = x * m[1] + y * m[4] + z * m[7] + m[10];
  out[2] = x * m[2] + y * m[5] + z * m[8] + m[11];
}

// Bitwise hash of a matrix: identical placements hash identically.
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

}  // namespace falcom_world
