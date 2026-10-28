/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "./streamline/include/sl_consts.h"

namespace farcry6_dlss::dlss {

// The provider stores the same camera constants the game uploads to its GPU
// buffer. Validate its viewport before accepting a sample for this frame.
template <typename CameraSample>
bool CaptureFrameGenerationCamera(const void* constants, uint32_t width, uint32_t height, uint64_t frame, CameraSample* output) {
  if (constants == nullptr || output == nullptr || width == 0u || height == 0u) return false;
  const auto* bytes = static_cast<const uint8_t*>(constants);
  float viewport[2] = {};
  std::memcpy(viewport, bytes + 0x200u, sizeof(viewport));
  if (viewport[0] != static_cast<float>(width) || viewport[1] != static_cast<float>(height)) return false;
  CameraSample sample = {};
  sample.frame = frame;
  std::memcpy(sample.viewport, viewport, sizeof(viewport));
  std::memcpy(sample.projection, bytes + 0x80u, sizeof(sample.projection));
  std::memcpy(sample.matrix, bytes + 0x140u, sizeof(sample.matrix));
  std::memcpy(sample.previous_matrix, bytes + 0x180u, sizeof(sample.previous_matrix));
  std::memcpy(sample.camera_position, bytes + 0x230u, sizeof(sample.camera_position));
  std::memcpy(sample.previous_camera_position, bytes + 0x240u, sizeof(sample.previous_camera_position));
  std::memcpy(sample.camera_right, bytes + 0x270u, sizeof(sample.camera_right));
  std::memcpy(sample.camera_up, bytes + 0x280u, sizeof(sample.camera_up));
  std::memcpy(sample.camera_direction, bytes + 0x290u, sizeof(sample.camera_direction));
  *output = sample;
  return true;
}

// The game's matrices multiply column vectors; Streamline uses row vectors.
// Build the transformation from two real, consecutive camera samples instead
// of guessing translation from the rotation-only previous matrix.
template <typename CameraSample>
bool BuildFrameGenerationCamera(
    const CameraSample& current, const CameraSample& previous,
    float jitter_x, float jitter_y, bool depth_inverted, bool motion_jittered,
    bool reset, sl::Constants* output) {
  if (output == nullptr || current.frame == 0u || previous.frame != current.frame - 1u
      || !std::isfinite(current.viewport[0]) || !std::isfinite(current.viewport[1])
      || current.viewport[0] < 1.f || current.viewport[1] < 1.f
      || current.viewport[0] != previous.viewport[0] || current.viewport[1] != previous.viewport[1]
      || !std::isfinite(jitter_x) || !std::isfinite(jitter_y)) return false;

  using Matrix = std::array<std::array<double, 4>, 4>;
  const auto inverse = [](const Matrix& input, Matrix* result) {
    double work[4][8] = {};
    for (uint32_t row = 0u; row < 4u; ++row) {
      for (uint32_t column = 0u; column < 4u; ++column) work[row][column] = input[row][column];
      work[row][row + 4u] = 1.0;
    }
    for (uint32_t column = 0u; column < 4u; ++column) {
      uint32_t pivot = column;
      for (uint32_t row = column + 1u; row < 4u; ++row) {
        if (std::abs(work[row][column]) > std::abs(work[pivot][column])) pivot = row;
      }
      if (!std::isfinite(work[pivot][column]) || std::abs(work[pivot][column]) < 1e-12) return false;
      if (pivot != column) {
        for (uint32_t i = 0u; i < 8u; ++i) std::swap(work[pivot][i], work[column][i]);
      }
      const double divisor = work[column][column];
      for (double& value : work[column]) value /= divisor;
      for (uint32_t row = 0u; row < 4u; ++row) {
        if (row == column) continue;
        const double scale = work[row][column];
        for (uint32_t i = 0u; i < 8u; ++i) work[row][i] -= scale * work[column][i];
      }
    }
    for (uint32_t row = 0u; row < 4u; ++row) {
      for (uint32_t column = 0u; column < 4u; ++column) {
        if (!std::isfinite(work[row][column + 4u])) return false;
        (*result)[row][column] = work[row][column + 4u];
      }
    }
    return true;
  };
  const auto multiply = [](const Matrix& left, const Matrix& right) {
    Matrix product = {};
    for (uint32_t row = 0u; row < 4u; ++row) {
      for (uint32_t column = 0u; column < 4u; ++column) {
        for (uint32_t i = 0u; i < 4u; ++i) product[row][column] += left[row][i] * right[i][column];
      }
    }
    return product;
  };
  const auto to_streamline = [](const Matrix& matrix, sl::float4x4* result) {
    for (uint32_t row = 0u; row < 4u; ++row) {
      result->setRow(row, sl::float4(static_cast<float>(matrix[0][row]), static_cast<float>(matrix[1][row]),
                                    static_cast<float>(matrix[2][row]), static_cast<float>(matrix[3][row])));
    }
  };
  const auto read_matrix = [](const float* values, Matrix* result) {
    for (uint32_t row = 0u; row < 4u; ++row) {
      for (uint32_t column = 0u; column < 4u; ++column) {
        const float value = values[row * 4u + column];
        if (!std::isfinite(value)) return false;
        (*result)[row][column] = value;
      }
    }
    return true;
  };

  Matrix projection = {}, current_view_projection = {}, previous_view_projection = {};
  if (!read_matrix(current.projection, &projection)
      || !read_matrix(current.matrix, &current_view_projection)
      || !read_matrix(previous.matrix, &previous_view_projection)) return false;
  if (projection[3][2] != -1.0 || projection[3][3] != 0.0
      || projection[1][1] <= 0.0 || projection[2][2] <= 0.0 || projection[2][3] <= 0.0) return false;
  for (uint32_t column = 0u; column < 4u; ++column) {
    current_view_projection[0][column] += current.projection[2] * current_view_projection[3][column];
    current_view_projection[1][column] += current.projection[6] * current_view_projection[3][column];
    previous_view_projection[0][column] += previous.projection[2] * previous_view_projection[3][column];
    previous_view_projection[1][column] += previous.projection[6] * previous_view_projection[3][column];
  }
  // The engine provides its own unjittered previous rotation-projection. A
  // consecutive present counter alone does not prove both samples are the same
  // gameplay camera, especially around menus, cuts and multiple views.
  for (uint32_t row = 0u; row < 4u; ++row) {
    for (uint32_t column = 0u; column < 4u; ++column) {
      const float expected = current.previous_matrix[row * 4u + column];
      if (!std::isfinite(expected)
          || std::abs(previous_view_projection[row][column] - expected) > 0.001 * (std::max)(1.0, std::abs(static_cast<double>(expected)))) return false;
    }
  }
  projection[0][2] = 0.0;
  projection[1][2] = 0.0;
  Matrix inverse_projection = {}, inverse_current = {}, inverse_previous = {};
  if (!inverse(projection, &inverse_projection) || !inverse(current_view_projection, &inverse_current)
      || !inverse(previous_view_projection, &inverse_previous)) return false;

  for (uint32_t i = 0u; i < 3u; ++i) {
    if (!std::isfinite(current.camera_position[i]) || !std::isfinite(current.camera_right[i])
        || !std::isfinite(current.camera_up[i]) || !std::isfinite(current.camera_direction[i])
        || !std::isfinite(previous.camera_position[i]) || !std::isfinite(current.previous_camera_position[i])
        || std::abs(current.previous_camera_position[i] - previous.camera_position[i]) > 0.05f) return false;
  }
  Matrix current_to_previous = {}, previous_to_current = {};
  for (uint32_t i = 0u; i < 4u; ++i) {
    current_to_previous[i][i] = 1.0;
    previous_to_current[i][i] = 1.0;
  }
  for (uint32_t i = 0u; i < 3u; ++i) {
    const double delta = static_cast<double>(current.camera_position[i]) - previous.camera_position[i];
    current_to_previous[i][3] = delta;
    previous_to_current[i][3] = -delta;
  }
  const double near_plane = projection[2][3] / (1.0 + projection[2][2]);
  const double far_plane = projection[2][3] / projection[2][2];
  if (!std::isfinite(near_plane) || !std::isfinite(far_plane) || near_plane <= 0.0 || far_plane <= near_plane) return false;

  sl::Constants constants = {};
  to_streamline(projection, &constants.cameraViewToClip);
  to_streamline(inverse_projection, &constants.clipToCameraView);
  to_streamline(multiply(multiply(previous_view_projection, current_to_previous), inverse_current), &constants.clipToPrevClip);
  to_streamline(multiply(multiply(current_view_projection, previous_to_current), inverse_previous), &constants.prevClipToClip);
  constants.jitterOffset = sl::float2(jitter_x, jitter_y);
  constants.mvecScale = sl::float2(1.f / current.viewport[0], 1.f / current.viewport[1]);
  constants.cameraPinholeOffset = sl::float2(0.f, 0.f);
  constants.cameraPos = sl::float3(current.camera_position[0], current.camera_position[1], current.camera_position[2]);
  constants.cameraRight = sl::float3(current.camera_right[0], current.camera_right[1], current.camera_right[2]);
  constants.cameraUp = sl::float3(current.camera_up[0], current.camera_up[1], current.camera_up[2]);
  constants.cameraFwd = sl::float3(current.camera_direction[0], current.camera_direction[1], current.camera_direction[2]);
  constants.cameraNear = static_cast<float>(near_plane);
  constants.cameraFar = static_cast<float>(far_plane);
  constants.cameraFOV = static_cast<float>(2.0 * std::atan(1.0 / projection[1][1]));
  constants.cameraAspectRatio = current.viewport[0] / current.viewport[1];
  constants.depthInverted = depth_inverted ? sl::Boolean::eTrue : sl::Boolean::eFalse;
  constants.cameraMotionIncluded = sl::Boolean::eTrue;
  constants.motionVectors3D = sl::Boolean::eFalse;
  constants.motionVectorsJittered = motion_jittered ? sl::Boolean::eTrue : sl::Boolean::eFalse;
  constants.reset = reset ? sl::Boolean::eTrue : sl::Boolean::eFalse;
  *output = constants;
  return true;
}

// The engine supplies an unjittered previous rotation-projection and the
// previous position in the current camera buffer. This avoids pairing camera
// providers from different views or CPU recording intervals.
template <typename CameraSample>
bool BuildFrameGenerationCameraFromProvider(
    const CameraSample& current, float jitter_x, float jitter_y,
    bool depth_inverted, bool motion_jittered, bool reset, sl::Constants* output) {
  if (current.frame == 0u) return false;
  CameraSample previous = current;
  previous.frame = current.frame - 1u;
  std::memcpy(previous.matrix, current.previous_matrix, sizeof(previous.matrix));
  previous.projection[2] = 0.f;
  previous.projection[6] = 0.f;
  std::memcpy(previous.camera_position, current.previous_camera_position, sizeof(previous.camera_position));
  return BuildFrameGenerationCamera(current, previous, jitter_x, jitter_y,
                                    depth_inverted, motion_jittered, reset, output);
}

}  // namespace farcry6_dlss::dlss
