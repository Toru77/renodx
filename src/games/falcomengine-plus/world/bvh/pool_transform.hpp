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

// Wind sway approximation (CPU, rigid per instance). Transcribed from the wind
// vertex shader reference/.dumps/foliage_0xA77D908A.vs_5_0.hlsl (lines 153-196 of
// the decompile) with the vertex flexibility fixed to 1 (the instance origin
// moves with the full amplitude): cb_scene wind values, the b5 shake values
// (cb_local), and the instance param2 (InstanceParam @148).
struct WindSwayParams {
  float game_time = 0.f;         // gameTime_g
  float wave_time = 0.f;         // windWaveTime_g
  float wave_frequency = 0.f;    // windWaveFrequency_g
  float wind_force = 0.f;        // windForce_g
  float wind_dir[3] = {};        // windDirection_g
  float shake_scale = 0.f;       // shakeScale_g
  float shake_speed = 0.f;       // shakeSpeed_g
  float shake_freq = 0.f;        // shakeFreq_g
  float shake_wind_scale = 0.f;  // shakeWindScale_g
};

// Displacement of an instance whose origin (rest translation) is `origin`.
inline void WindSwayDisplacement(const WindSwayParams& w, const float* origin, const float* param2, float* out) {
  const float phase = w.wave_time - w.wave_frequency * (origin[0] * w.wind_dir[0] + origin[1] * w.wind_dir[1] + origin[2] * w.wind_dir[2]);
  const float gust = 1.f - std::pow(std::sin(phase) * 0.5f + 0.5f, 7.f);  // 0..1
  const float force = w.wind_force * gust;
  const float time = w.game_time * w.shake_speed + gust * w.wind_force;
  constexpr float kGravity[3] = {0.f, -0.00980000012f, 0.f};
  for (int k = 0; k < 3; ++k) {
    const float sway = std::sin(origin[k] * w.shake_freq + time);
    out[k] = force * (w.shake_scale * w.wind_dir[k] * sway + w.wind_dir[k] * w.shake_wind_scale) + kGravity[k] + param2[k];
  }
}

// Per-axis bound of |WindSwayDisplacement| over every origin and time (gust in 0..1).
inline void WindSwayBound(const WindSwayParams& w, const float* param2, float* out) {
  constexpr float kGravity[3] = {0.f, -0.00980000012f, 0.f};
  for (int k = 0; k < 3; ++k) {
    out[k] = std::fabs(w.wind_force) * (std::fabs(w.shake_scale) + std::fabs(w.shake_wind_scale)) * std::fabs(w.wind_dir[k])
             + std::fabs(kGravity[k]) + std::fabs(param2[k]);
  }
}

}  // namespace falcom_world
