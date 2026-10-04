#pragma once

// Phase 0 research overlay.
//
// Draws every instance of the captured draw through the selected transform
// candidate, hue-coded per instance, so the instance indexing rule
// (SV_InstanceID + instanceOffset + stride) is validated as a set rather than
// one instance. Rendered with the ReShade/DevKit ImGui overlay's background
// draw list while the overlay is open.

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../world_state.hpp"
#include "../research/transform_candidates.hpp"

namespace falcom_world {

inline ImU32 InstanceHueColor(uint32_t instance) {
  const float hue = std::fmod(static_cast<float>(instance) * 0.618034f, 1.f);
  return ImGui::ColorConvertFloat4ToU32(ImColor::HSV(hue, 0.85f, 1.f));
}

inline ImU32 WorldPositionColor(const float* world) {
  const float hue = std::fmod(std::fabs(world[0] * 0.031f + world[1] * 0.017f + world[2] * 0.023f), 1.f);
  return ImGui::ColorConvertFloat4ToU32(ImColor::HSV(hue, 0.85f, 1.f));
}

inline void DrawDebugOverlay() {
  if (!g_state.supported) return;
  const uint32_t mode = OverlayMode();
  if (mode == 0u) return;

  std::lock_guard<std::mutex> lock(g_state.mutex);
  const CapturedDraw& captured = g_state.captured;
  if (!captured.mesh_valid || captured.mesh.positions.empty()) return;
  if (g_state.candidates.empty()) return;

  const int candidate_index = std::clamp(
      g_state.selected_candidate, 0, static_cast<int>(g_state.candidates.size()) - 1);
  const TransformCandidate& candidate = g_state.candidates[candidate_index];
  const CameraSnapshot& camera = g_state.camera;
  const bool show_prev = OverlayShowPrev() && candidate.has_prev;

  const ImVec2 display = ImGui::GetIO().DisplaySize;
  if (display.x <= 0.f || display.y <= 0.f) return;
  const float render_w = captured.rtv_w != 0u ? static_cast<float>(captured.rtv_w) : display.x;
  const float render_h = captured.rtv_h != 0u ? static_cast<float>(captured.rtv_h) : display.y;
  const float scale_x = display.x / render_w;
  const float scale_y = display.y / render_h;

  auto* draw_list = ImGui::GetBackgroundDrawList();
  const uint32_t stride = OverlayStride();
  const auto& positions = captured.mesh.positions;
  const bool structured = IsStructuredKind(candidate.kind);
  const uint32_t total_instances = structured
                                       ? (captured.draw.instance_count == 0u ? 1u : captured.draw.instance_count)
                                       : 1u;
  const uint32_t shown_instances = structured
                                       ? (std::min)(candidate.element_count, kMaxCandidateInstances)
                                       : 1u;

  const auto project = [&](uint32_t instance, const std::array<float, 3>& point, ImVec2* out_screen, float* out_world, ImU32* out_color, bool* out_ok) {
    float uv[2] = {};
    float world[3] = {};
    *out_ok = ProjectCandidateInstance(candidate, camera, instance, point, uv, world);
    if (!*out_ok) return;
    if (uv[0] < -0.05f || uv[0] > 1.05f || uv[1] < -0.05f || uv[1] > 1.05f) {
      *out_ok = false;
      return;
    }
    out_screen->x = uv[0] * render_w * scale_x;
    out_screen->y = uv[1] * render_h * scale_y;
    out_world[0] = world[0];
    out_world[1] = world[1];
    out_world[2] = world[2];
    if (mode == 4u) {
      *out_color = WorldPositionColor(world);
    } else {
      *out_color = InstanceHueColor(instance);
    }
  };

  const auto project_prev = [&](uint32_t instance, const std::array<float, 3>& point, ImVec2* out_screen, bool* out_ok) {
    float uv[2] = {};
    float world[3] = {};
    *out_ok = ProjectCandidatePrevInstance(candidate, camera, instance, point, uv, world);
    if (!*out_ok) return;
    if (uv[0] < -0.05f || uv[0] > 1.05f || uv[1] < -0.05f || uv[1] > 1.05f) {
      *out_ok = false;
      return;
    }
    out_screen->x = uv[0] * render_w * scale_x;
    out_screen->y = uv[1] * render_h * scale_y;
  };

  if (mode == 1u || mode == 4u) {
    for (uint32_t instance = 0; instance < shown_instances; ++instance) {
      for (size_t i = 0u; i < positions.size(); i += stride) {
        ImVec2 screen = {};
        float world[3] = {};
        ImU32 color = 0u;
        bool ok = false;
        project(instance, positions[i], &screen, world, &color, &ok);
        if (!ok) continue;
        draw_list->AddCircleFilled(screen, 2.0f, color);
        if (show_prev && i == 0u) {
          ImVec2 prev_screen = {};
          bool prev_ok = false;
          project_prev(instance, positions[i], &prev_screen, &prev_ok);
          if (prev_ok) {
            draw_list->AddLine(prev_screen, screen, IM_COL32(255, 255, 255, 90));
            draw_list->AddCircleFilled(prev_screen, 1.5f, IM_COL32(200, 200, 200, 120));
          }
        }
      }
    }
  } else if (mode == 2u) {
    const auto& triangles = captured.mesh.triangles;
    const size_t tri_stride = (triangles.size() > 30000u) ? (triangles.size() / 30000u) : 1u;
    for (uint32_t instance = 0; instance < shown_instances; ++instance) {
      const ImU32 color = InstanceHueColor(instance);
      for (size_t i = 0u; i < triangles.size(); i += tri_stride) {
        ImVec2 screen[3] = {};
        bool ok[3] = {};
        for (int k = 0; k < 3; ++k) {
          const uint32_t index = triangles[i][k];
          if (index >= positions.size()) {
            ok[k] = false;
            continue;
          }
          float world[3] = {};
          ImU32 point_color = 0u;
          project(instance, positions[index], &screen[k], world, &point_color, &ok[k]);
        }
        if (!ok[0] || !ok[1] || !ok[2]) continue;
        draw_list->AddLine(screen[0], screen[1], color);
        draw_list->AddLine(screen[1], screen[2], color);
        draw_list->AddLine(screen[2], screen[0], color);
      }
    }
  } else if (mode == 3u) {
    const std::array<float, 3> corners[8] = {
        {{captured.mesh.bbox_min[0], captured.mesh.bbox_min[1], captured.mesh.bbox_min[2]}},
        {{captured.mesh.bbox_max[0], captured.mesh.bbox_min[1], captured.mesh.bbox_min[2]}},
        {{captured.mesh.bbox_min[0], captured.mesh.bbox_max[1], captured.mesh.bbox_min[2]}},
        {{captured.mesh.bbox_max[0], captured.mesh.bbox_max[1], captured.mesh.bbox_min[2]}},
        {{captured.mesh.bbox_min[0], captured.mesh.bbox_min[1], captured.mesh.bbox_max[2]}},
        {{captured.mesh.bbox_max[0], captured.mesh.bbox_min[1], captured.mesh.bbox_max[2]}},
        {{captured.mesh.bbox_min[0], captured.mesh.bbox_max[1], captured.mesh.bbox_max[2]}},
        {{captured.mesh.bbox_max[0], captured.mesh.bbox_max[1], captured.mesh.bbox_max[2]}},
    };
    static constexpr int kEdges[12][2] = {
        {0, 1}, {2, 3}, {4, 5}, {6, 7},
        {0, 2}, {1, 3}, {4, 6}, {5, 7},
        {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };
    for (uint32_t instance = 0; instance < shown_instances; ++instance) {
      const ImU32 color = InstanceHueColor(instance);
      ImVec2 screen[8] = {};
      bool ok[8] = {};
      for (int i = 0; i < 8; ++i) {
        float world[3] = {};
        ImU32 point_color = 0u;
        project(instance, corners[i], &screen[i], world, &point_color, &ok[i]);
      }
      for (const auto& edge : kEdges) {
        if (!ok[edge[0]] || !ok[edge[1]]) continue;
        draw_list->AddLine(screen[edge[0]], screen[edge[1]], color, 2.0f);
      }
    }
  }

  const int64_t element_begin = static_cast<int64_t>(captured.draw.first_instance)
                                + static_cast<int64_t>(candidate.base_offset);
  char label[512] = {};
  std::snprintf(
      label, sizeof(label),
      "draw %u  vs 0x%08X  ps 0x%08X\n"
      "%s  %s b%u  offset %u  stride %u  base %d\n"
      "elements [%lld..%lld)  instances ok %u/%u  showing %u / %u%s",
      captured.draw.serial,
      captured.draw.vs_hash,
      captured.draw.ps_hash,
      CandidateKindName(candidate.kind),
      candidate.stage == 1u ? "VS" : (candidate.stage == 2u ? "PS" : "-"),
      candidate.slot,
      candidate.matrix_offset,
      candidate.stride,
      candidate.base_offset,
      static_cast<long long>(element_begin),
      static_cast<long long>(element_begin + total_instances),
      candidate.instances_ok,
      candidate.element_count,
      shown_instances,
      total_instances,
      show_prev ? "\nprev world shown" : "");
  draw_list->AddText(ImVec2(12.f, 12.f), IM_COL32(255, 255, 255, 230), label);
}

}  // namespace falcom_world
