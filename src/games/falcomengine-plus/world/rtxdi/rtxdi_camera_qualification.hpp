#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "rtxdi_surface_layout.h"

namespace falcom_world::rtxdi {

using CameraMatrix = std::array<float, 16u>;

struct CameraCandidate {
  CameraMatrix world_to_view = {};
  CameraMatrix view_to_clip = {};
  CameraMatrix world_to_view_previous = {};
  CameraMatrix view_to_clip_previous = {};
  std::array<float, 2u> jitter_pixels = {};
  std::array<float, 2u> jitter_pixels_previous = {};
  std::array<std::array<float, 2u>, 2u> motion_correction_candidates = {};
  // Candidate 0 assumes clip matrices are non-jittered; candidate 1 removes the
  // jitter delta embedded in clip-space motion as well as the game's c79 term.
  bool clip_matrix_jitter_candidate = false;
  bool matrices_valid = false;
  bool absolute_jitter_valid = false;
};

inline bool ReadSceneMatrix(const void* scene_rows, uint32_t row, CameraMatrix* matrix) {
  if (scene_rows == nullptr || matrix == nullptr || row > RTXDI_SURFACE_SCENE_ROW_COUNT - 4u) return false;
  std::array<float, 16u> packed = {};
  std::memcpy(packed.data(), static_cast<const uint8_t*>(scene_rows) + row * 16u, sizeof(packed));
  // HLSL cbuffer matrices use column-major packing. Convert to game row-vector algebra.
  for (uint32_t y = 0u; y < 4u; ++y) {
    for (uint32_t x = 0u; x < 4u; ++x) (*matrix)[y * 4u + x] = packed[x * 4u + y];
  }
  return std::all_of(matrix->begin(), matrix->end(), [](float value) { return std::isfinite(value); });
}

inline CameraMatrix MultiplyCameraMatrices(const CameraMatrix& a, const CameraMatrix& b) {
  CameraMatrix result = {};
  for (uint32_t row = 0u; row < 4u; ++row) {
    for (uint32_t col = 0u; col < 4u; ++col) {
      for (uint32_t inner = 0u; inner < 4u; ++inner) {
        result[row * 4u + col] += a[row * 4u + inner] * b[inner * 4u + col];
      }
    }
  }
  return result;
}

inline bool CameraMatricesNear(const CameraMatrix& a, const CameraMatrix& b, float epsilon) {
  float scale = 1.0f;
  float error = 0.0f;
  for (uint32_t index = 0u; index < a.size(); ++index) {
    scale = (std::max)(scale, (std::max)(std::abs(a[index]), std::abs(b[index])));
    error = (std::max)(error, std::abs(a[index] - b[index]));
  }
  return error <= epsilon * scale;
}

inline bool InvertCameraMatrix(const CameraMatrix& matrix, CameraMatrix* inverse) {
  if (inverse == nullptr) return false;
  std::array<std::array<double, 8u>, 4u> augmented = {};
  for (uint32_t row = 0u; row < 4u; ++row) {
    for (uint32_t col = 0u; col < 4u; ++col) augmented[row][col] = matrix[row * 4u + col];
    augmented[row][row + 4u] = 1.0;
  }
  for (uint32_t pivot = 0u; pivot < 4u; ++pivot) {
    uint32_t best = pivot;
    for (uint32_t row = pivot + 1u; row < 4u; ++row) {
      if (std::abs(augmented[row][pivot]) > std::abs(augmented[best][pivot])) best = row;
    }
    if (!std::isfinite(augmented[best][pivot]) || std::abs(augmented[best][pivot]) < 1.0e-12) return false;
    if (best != pivot) std::swap(augmented[best], augmented[pivot]);
    const double divisor = augmented[pivot][pivot];
    for (double& value : augmented[pivot]) value /= divisor;
    for (uint32_t row = 0u; row < 4u; ++row) {
      if (row == pivot) continue;
      const double factor = augmented[row][pivot];
      for (uint32_t col = 0u; col < 8u; ++col) augmented[row][col] -= factor * augmented[pivot][col];
    }
  }
  for (uint32_t row = 0u; row < 4u; ++row) {
    for (uint32_t col = 0u; col < 4u; ++col) {
      (*inverse)[row * 4u + col] = static_cast<float>(augmented[row][col + 4u]);
      if (!std::isfinite((*inverse)[row * 4u + col])) return false;
    }
  }
  return true;
}

inline bool BuildCameraCandidate(const void* current_scene_rows, const void* previous_scene_rows,
                                 uint32_t width, uint32_t height, CameraCandidate* candidate) {
  if (candidate == nullptr || current_scene_rows == nullptr || previous_scene_rows == nullptr || width == 0u || height == 0u) return false;
  *candidate = {};
  CameraMatrix current_view = {}, current_view_inverse = {}, current_projection = {}, current_projection_inverse = {};
  CameraMatrix current_view_projection = {}, current_view_projection_inverse = {}, current_previous_view_projection = {};
  CameraMatrix previous_view = {}, previous_view_inverse = {}, previous_projection = {}, previous_projection_inverse = {};
  CameraMatrix previous_view_projection = {}, previous_view_projection_inverse = {};
  if (!ReadSceneMatrix(current_scene_rows, 0u, &current_view)
      || !ReadSceneMatrix(current_scene_rows, 4u, &current_view_inverse)
      || !ReadSceneMatrix(current_scene_rows, 8u, &current_projection)
      || !ReadSceneMatrix(current_scene_rows, 12u, &current_projection_inverse)
      || !ReadSceneMatrix(current_scene_rows, 16u, &current_view_projection)
      || !ReadSceneMatrix(current_scene_rows, 20u, &current_view_projection_inverse)
      || !ReadSceneMatrix(current_scene_rows, 75u, &current_previous_view_projection)
      || !ReadSceneMatrix(previous_scene_rows, 0u, &previous_view)
      || !ReadSceneMatrix(previous_scene_rows, 4u, &previous_view_inverse)
      || !ReadSceneMatrix(previous_scene_rows, 8u, &previous_projection)
      || !ReadSceneMatrix(previous_scene_rows, 12u, &previous_projection_inverse)
      || !ReadSceneMatrix(previous_scene_rows, 16u, &previous_view_projection)
      || !ReadSceneMatrix(previous_scene_rows, 20u, &previous_view_projection_inverse)) return false;

  const CameraMatrix identity = {1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f};
  const float epsilon = 1.0e-3f;
  if (!CameraMatricesNear(MultiplyCameraMatrices(current_view, current_view_inverse), identity, epsilon)
      || !CameraMatricesNear(MultiplyCameraMatrices(current_projection, current_projection_inverse), identity, epsilon)
      || !CameraMatricesNear(MultiplyCameraMatrices(current_view_projection, current_view_projection_inverse), identity, epsilon)
      || !CameraMatricesNear(MultiplyCameraMatrices(current_view, current_projection), current_view_projection, epsilon)
      || !CameraMatricesNear(MultiplyCameraMatrices(previous_view, previous_view_inverse), identity, epsilon)
      || !CameraMatricesNear(MultiplyCameraMatrices(previous_projection, previous_projection_inverse), identity, epsilon)
      || !CameraMatricesNear(MultiplyCameraMatrices(previous_view_projection, previous_view_projection_inverse), identity, epsilon)
      || !CameraMatricesNear(MultiplyCameraMatrices(previous_view, previous_projection), previous_view_projection, epsilon)
      || !CameraMatricesNear(current_previous_view_projection, previous_view_projection, epsilon)) return false;

  const auto standard_projection = [](const CameraMatrix& projection) {
    return std::abs(projection[11]) > 1.0e-6f && std::abs(projection[1]) <= 1.0e-5f
           && std::abs(projection[4]) <= 1.0e-5f && std::abs(projection[12]) <= 1.0e-5f
           && std::abs(projection[13]) <= 1.0e-5f && std::abs(projection[15]) <= 1.0e-5f;
  };
  if (!standard_projection(current_projection) || !standard_projection(previous_projection)) return false;

  const auto projection_jitter = [&](const CameraMatrix& projection, std::array<float, 2u>* jitter) {
    if (std::abs(projection[11]) < 1.0e-6f || jitter == nullptr) return false;
    const float ndc_x = projection[8] / projection[11];
    const float ndc_y = projection[9] / projection[11];
    (*jitter)[0] = 0.5f * width * ndc_x;
    (*jitter)[1] = -0.5f * height * ndc_y;
    return std::isfinite((*jitter)[0]) && std::isfinite((*jitter)[1])
           && std::abs((*jitter)[0]) <= 0.5f && std::abs((*jitter)[1]) <= 0.5f;
  };
  std::array<float, 2u> current_jitter = {}, previous_jitter = {};
  const uint8_t* current_bytes = static_cast<const uint8_t*>(current_scene_rows);
  std::array<float, 4u> jitter_delta = {};
  std::memcpy(jitter_delta.data(), current_bytes + 79u * 16u, sizeof(jitter_delta));
  if (!std::all_of(jitter_delta.begin(), jitter_delta.end(), [](float value) { return std::isfinite(value); })) return false;
  if (!projection_jitter(current_projection, &current_jitter)
      || !projection_jitter(previous_projection, &previous_jitter)) return false;
  CameraMatrix current_non_jittered = current_projection;
  CameraMatrix previous_non_jittered = previous_projection;
  current_non_jittered[8] = current_non_jittered[9] = 0.0f;
  previous_non_jittered[8] = previous_non_jittered[9] = 0.0f;
  if (!CameraMatricesNear(current_non_jittered, previous_non_jittered, 1.0e-4f)) return false;
  const std::array<float, 2u> difference = {previous_jitter[0] - current_jitter[0],
                                            previous_jitter[1] - current_jitter[1]};
  const bool delta_matches = std::abs(difference[0] - jitter_delta[0]) <= 0.05f
                             && std::abs(difference[1] - jitter_delta[1]) <= 0.05f;
  const bool opposite_matches = std::abs(difference[0] + jitter_delta[0]) <= 0.05f
                                && std::abs(difference[1] + jitter_delta[1]) <= 0.05f;
  if (!delta_matches && !opposite_matches) return false;

  // HLSL cbuffer registers hold game-matrix columns. ReadSceneMatrix creates
  // row-major game matrices; those bytes are column-major transposes for NRD.
  candidate->world_to_view = current_view;
  candidate->view_to_clip = current_non_jittered;
  candidate->world_to_view_previous = previous_view;
  candidate->view_to_clip_previous = previous_non_jittered;
  candidate->jitter_pixels = current_jitter;
  candidate->jitter_pixels_previous = previous_jitter;
  candidate->motion_correction_candidates[0] = {jitter_delta[0], jitter_delta[1]};
  candidate->motion_correction_candidates[1] = {jitter_delta[0] + difference[0],
                                                jitter_delta[1] + difference[1]};
  candidate->clip_matrix_jitter_candidate = std::abs(current_jitter[0]) > 1.0e-5f
                                            || std::abs(current_jitter[1]) > 1.0e-5f
                                            || std::abs(previous_jitter[0]) > 1.0e-5f
                                            || std::abs(previous_jitter[1]) > 1.0e-5f;
  candidate->matrices_valid = true;
  candidate->absolute_jitter_valid = true;
  return true;
}

struct CameraQualification {
  uint64_t owner_generation = 0u, configuration_generation = 0u, resource = 0u;
  uint32_t last_frame = UINT32_MAX, width = 0u, height = 0u, consecutive_passes = 0u;
  uint32_t motion_correction_index = UINT32_MAX;
  bool qualified = false;
};

inline bool CameraQualificationMatches(const CameraQualification& state, uint64_t owner_generation,
                                       uint64_t configuration_generation, uint64_t resource, uint32_t width,
                                       uint32_t height, bool candidate_valid, bool projection_compatible) {
  return state.qualified && owner_generation != 0u && configuration_generation != 0u && resource != 0u
         && width != 0u && height != 0u && candidate_valid && projection_compatible
         && state.owner_generation == owner_generation
         && state.configuration_generation == configuration_generation && state.resource == resource
         && state.width == width && state.height == height && state.motion_correction_index < 2u;
}

inline bool ShouldRunCameraProbe(const CameraQualification& state, uint64_t owner_generation,
                                 uint64_t configuration_generation, uint64_t resource, uint32_t width,
                                 uint32_t height, bool candidate_valid, bool projection_compatible) {
  return !CameraQualificationMatches(state, owner_generation, configuration_generation, resource, width, height,
                                     candidate_valid, projection_compatible);
}

inline bool ObserveCameraProbe(CameraQualification* state, uint32_t frame, uint64_t owner_generation,
                               uint64_t configuration_generation, uint64_t resource, uint32_t width,
                               uint32_t height, bool candidate_valid, bool probe_valid,
                               uint32_t motion_correction_index, uint32_t required_consecutive = 4u) {
  if (state == nullptr || width == 0u || height == 0u || owner_generation == 0u
      || configuration_generation == 0u || resource == 0u || !candidate_valid || !probe_valid) {
    if (state != nullptr) {
      state->consecutive_passes = 0u;
      state->qualified = false;
      state->last_frame = frame;
      state->motion_correction_index = UINT32_MAX;
    }
    return false;
  }
  if (state->last_frame != UINT32_MAX && frame <= state->last_frame) return state->qualified;
  const bool consecutive = state->last_frame != UINT32_MAX && state->last_frame != UINT32_MAX - 1u
                           && frame == state->last_frame + 1u && state->owner_generation == owner_generation
                           && state->configuration_generation == configuration_generation
                           && state->resource == resource && state->width == width && state->height == height
                           && state->motion_correction_index == motion_correction_index;
  state->consecutive_passes = consecutive ? state->consecutive_passes + 1u : 1u;
  state->owner_generation = owner_generation;
  state->configuration_generation = configuration_generation;
  state->resource = resource;
  state->width = width;
  state->height = height;
  state->last_frame = frame;
  state->motion_correction_index = motion_correction_index;
  state->qualified = state->consecutive_passes >= required_consecutive;
  return state->qualified;
}

}  // namespace falcom_world::rtxdi
