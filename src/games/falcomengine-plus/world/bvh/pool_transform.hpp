#pragma once

// Instance transforms used by the world pool.
//
// Pool instances are Inst4x3Row: the 12 floats of InstanceParam.world as
// stored, world.x = dot(float4(p, 1), m[0..3]) (row-dot). Verified against the
// Sora 2nd instanced vertex shaders (t15, stride 160, b1 offset). The other
// forms stay for CandidateKind values other layouts may use.

#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>

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

}  // namespace falcom_world
