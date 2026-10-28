/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#include <cmath>
#include <cstdint>
#include <cstdio>

#include "../fg_camera.hpp"

struct CameraSample {
  uint64_t frame = 0;
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

  sl::Constants constants = {};
  if (!farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, -0.25f, 0.3889f, true, false, false, &constants)) return 1;
  for (uint32_t row = 0u; row < 4u; ++row) {
    const float* values = &constants.clipToPrevClip.row[row].x;
    for (uint32_t column = 0u; column < 4u; ++column) {
      if (std::abs(values[column] - (row == column ? 1.f : 0.f)) > 1e-4f) return 2;
    }
  }
  if (std::abs(constants.cameraNear - 0.1f) > 1e-4f || std::abs(constants.cameraFar - 10000.f) > 1.f) return 3;

  current.previous_matrix[0] *= 0.9f;
  if (farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, 0.f, 0.f, true, false, false, &constants)) return 10;
  current.previous_matrix[0] = projection[0];

  current.camera_position[0] = 1.f;
  if (!farcry6_dlss::dlss::BuildFrameGenerationCamera(current, previous, 0.f, 0.f, true, false, false, &constants)) return 4;
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
