/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace farcry6_dlss::dlss {

// These are the projection and current clip matrices observed in the main-view
// buffer. The adjustment is applied to a copy, before a replacement CBV is bound.
inline bool AdjustCameraJitter(
    std::array<float, 16>* projection, std::array<float, 16>* clip,
    float width, float height, float target_x, float target_y) {
  if (projection == nullptr || clip == nullptr || !std::isfinite(width) || !std::isfinite(height)
      || width < 1.f || height < 1.f || !std::isfinite(target_x) || !std::isfinite(target_y)
      || std::abs(target_x) > 0.5f || std::abs(target_y) > 0.5f
      || projection->at(14) != -1.f || projection->at(15) != 0.f) return false;
  for (float value : *projection) {
    if (!std::isfinite(value)) return false;
  }
  for (float value : *clip) {
    if (!std::isfinite(value)) return false;
  }
  float norm = 0.f;
  float clip_x = 0.f;
  float clip_y = 0.f;
  for (uint32_t i = 0u; i < 3u; ++i) {
    norm += (*clip)[12u + i] * (*clip)[12u + i];
    clip_x += (*clip)[i] * (*clip)[12u + i];
    clip_y += (*clip)[4u + i] * (*clip)[12u + i];
  }
  if (!std::isfinite(norm) || !std::isfinite(clip_x) || !std::isfinite(clip_y)
      || norm < 0.9f || norm > 1.1f) return false;
  const float current_x = -(*projection)[2] * width * 0.5f;
  const float current_y = (*projection)[6] * height * 0.5f;
  if (std::abs(clip_x / norm * width * 0.5f - current_x) > 0.01f
      || std::abs(-clip_y / norm * height * 0.5f - current_y) > 0.01f) return false;
  const float delta_x = (target_x - current_x) * 2.f / width;
  const float delta_y = (current_y - target_y) * 2.f / height;
  auto next_projection = *projection;
  auto next_clip = *clip;
  next_projection[2] -= delta_x;
  next_projection[6] -= delta_y;
  for (uint32_t i = 0u; i < 4u; ++i) {
    next_clip[i] += delta_x * next_clip[12u + i];
    next_clip[4u + i] += delta_y * next_clip[12u + i];
  }
  *projection = next_projection;
  *clip = next_clip;
  return true;
}

}  // namespace farcry6_dlss::dlss
