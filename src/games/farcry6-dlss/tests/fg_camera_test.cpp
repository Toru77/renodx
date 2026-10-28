/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "../fg_camera.hpp"

struct CameraSample {
  uint64_t frame = 0;
  uint32_t offscreen = 0u;
  float viewport[2] = {1280.f, 720.f};
  float projection[16] = {};
  float matrix[16] = {};
  float previous_matrix[16] = {};
  float camera_position[3] = {};
  float previous_camera_position[3] = {};
  float camera_right[3] = {1.f, 0.f, 0.f};
  float camera_up[3] = {0.f, 1.f, 0.f};
  float camera_direction[3] = {0.f, 0.f, -1.f};
};

int main() {
  CameraSample current;
  current.frame = 7u;
  const float projection[16] = {
      0.795435965f, 0.f, 0.000390625f, 0.f,
      0.f, 1.41410816f, 0.00108024688f, 0.f,
      0.f, 0.f, 0.0000100001f, 0.100000992f,
      0.f, 0.f, -1.f, 0.f};
  for (uint32_t i = 0u; i < 16u; ++i) current.projection[i] = current.matrix[i] = current.previous_matrix[i] = projection[i];
  current.previous_matrix[2] = 0.f;
  current.previous_matrix[6] = 0.f;
  CameraSample previous = current;
  previous.frame = 6u;
  previous.projection[2] = previous.matrix[2] = -0.000390625f;
  previous.projection[6] = previous.matrix[6] = -0.000462963f;

  uint8_t camera_buffer[0x33c] = {};
  std::memcpy(camera_buffer + 0x80u, current.projection, sizeof(current.projection));
  std::memcpy(camera_buffer + 0x140u, current.matrix, sizeof(current.matrix));
  std::memcpy(camera_buffer + 0x180u, current.previous_matrix, sizeof(current.previous_matrix));
  std::memcpy(camera_buffer + 0x200u, current.viewport, sizeof(current.viewport));
  std::memcpy(camera_buffer + 0x270u, current.camera_right, sizeof(current.camera_right));
  std::memcpy(camera_buffer + 0x280u, current.camera_up, sizeof(current.camera_up));
  std::memcpy(camera_buffer + 0x290u, current.camera_direction, sizeof(current.camera_direction));
  CameraSample captured;
  if (!farcry6_dlss::dlss::CaptureFrameGenerationCamera(camera_buffer, 1280u, 720u, 7u, &captured)
      || captured.frame != 7u || captured.projection[0] != current.projection[0]
      || captured.camera_direction[2] != current.camera_direction[2]) return 12;
  if (farcry6_dlss::dlss::CaptureFrameGenerationCamera(camera_buffer, 2560u, 720u, 7u, &captured)) return 13;
  camera_buffer[0x338] = 1u;
  if (!farcry6_dlss::dlss::CaptureFrameGenerationCamera(camera_buffer, 1280u, 720u, 7u, &captured)) return 14;

  sl::Constants constants = {};
  if (!farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, -0.25f, 0.3889f, true, false, false, &constants)) return 1;
  for (uint32_t row = 0u; row < 4u; ++row) {
    const float* values = &constants.clipToPrevClip.row[row].x;
    for (uint32_t column = 0u; column < 4u; ++column) {
      if (std::abs(values[column] - (row == column ? 1.f : 0.f)) > 1e-4f) return 2;
    }
  }
  if (std::abs(constants.cameraNear - 0.1f) > 1e-4f || std::abs(constants.cameraFar - 10000.f) > 1.f) return 3;
  if (constants.cameraPinholeOffset.x != 0.f || constants.cameraPinholeOffset.y != 0.f) return 19;
  sl::Constants provider_constants = {};
  if (!farcry6_dlss::dlss::BuildFrameGenerationCameraFromProvider(current, -0.25f, 0.3889f, true, false, false, &provider_constants)) return 15;
  for (uint32_t row = 0u; row < 4u; ++row) {
    const float* expected = &constants.clipToPrevClip.row[row].x;
    const float* actual = &provider_constants.clipToPrevClip.row[row].x;
    for (uint32_t column = 0u; column < 4u; ++column) {
      if (std::abs(expected[column] - actual[column]) > 1e-4f) return 16;
    }
  }
  CameraSample missing_previous = current;
  std::memset(missing_previous.previous_matrix, 0, sizeof(missing_previous.previous_matrix));
  if (farcry6_dlss::dlss::BuildFrameGenerationCameraFromProvider(missing_previous, 0.f, 0.f, true, false, false, &provider_constants)) return 18;

  current.viewport[0] = previous.viewport[0] = std::numeric_limits<float>::infinity();
  if (farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, 0.f, 0.f, true, false, false, &constants)) return 11;
  current.viewport[0] = previous.viewport[0] = 1280.f;

  current.previous_matrix[0] *= 0.9f;
  if (farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, 0.f, 0.f, true, false, false, &constants)) return 10;
  current.previous_matrix[0] = projection[0];

  current.camera_position[0] = 1.f;
  if (!farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, 0.f, 0.f, true, false, false, &constants)) return 4;
  if (!farcry6_dlss::dlss::BuildFrameGenerationCameraFromProvider(current, 0.f, 0.f, true, false, false, &provider_constants)
      || std::abs(provider_constants.clipToPrevClip.row[3].x - constants.clipToPrevClip.row[3].x) > 1e-4f) return 17;
  const float previous_clip_x = constants.clipToPrevClip.row[2].x * 0.5f + constants.clipToPrevClip.row[3].x;
  if (std::abs(previous_clip_x - 3.977f) > 0.01f) return 5;
  current.previous_camera_position[0] = 2.f;
  if (farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, 0.f, 0.f, true, false, false, &constants)) return 6;
  current.previous_camera_position[0] = 0.f;
  previous.frame = 4u;
  if (farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, 0.f, 0.f, true, false, false, &constants)) return 7;

  current.camera_position[0] = 0.f;
  previous.frame = 6u;
  const float sine = std::sqrt(0.5f);
  const float rotation[16] = {sine, 0.f, sine, 0.f, 0.f, 1.f, 0.f, 0.f, -sine, 0.f, sine, 0.f, 0.f, 0.f, 0.f, 1.f};
  for (uint32_t row = 0u; row < 4u; ++row) {
    for (uint32_t column = 0u; column < 4u; ++column) {
      current.matrix[row * 4u + column] = 0.f;
      for (uint32_t i = 0u; i < 4u; ++i) {
        current.matrix[row * 4u + column] += current.projection[row * 4u + i] * rotation[i * 4u + column];
      }
    }
  }
  if (!farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, 0.f, 0.f, true, false, false, &constants)) return 8;
  const float rotated_x = constants.clipToPrevClip.row[2].x * 0.5f + constants.clipToPrevClip.row[3].x;
  const float rotated_w = constants.clipToPrevClip.row[2].w * 0.5f + constants.clipToPrevClip.row[3].w;
  if (std::abs(rotated_x / rotated_w - 0.795435965f) > 1e-4f) return 9;

  std::puts("FG camera conversion checks passed");
  return 0;
}
