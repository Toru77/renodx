#pragma once

// Mesh capture + camera snapshot helpers for Phase 0.
//
// Mesh capture reuses utils::scene's decoder (input-layout lookup, VB/IB
// readback, attribute decode) and keeps the result in memory instead of
// writing OBJ/JSON. It runs once per armed draw at present, so the one-shot
// GPU readback stall is acceptable research cost.

#include "../world_state.hpp"
#include "../../../../utils/constants.hpp"

namespace falcom_world {

inline renodx::utils::scene::DrawRecord ToSceneRecord(const DrawRecord& draw) {
  renodx::utils::scene::DrawRecord record;
  record.device = nullptr;
  record.frame_id = draw.frame;
  record.draw_index = draw.serial;
  record.method = draw.method;
  record.vertex_count = draw.vertex_count;
  record.index_count = draw.index_count;
  record.instance_count = draw.instance_count;
  record.first_vertex = draw.first_vertex;
  record.first_index = draw.first_index;
  record.vertex_offset = draw.vertex_offset;
  record.first_instance = draw.first_instance;
  record.vs_hash = draw.vs_hash;
  record.ps_hash = draw.ps_hash;
  record.input_layout = draw.input_layout;
  record.topology = draw.topology;
  record.vertex_buffers.push_back({draw.vb, draw.vb_offset, draw.vb_stride});
  record.index_buffer = {draw.ib, draw.ib_offset, draw.index_size};
  record.has_index_buffer = draw.has_index_buffer;
  record.render_target = draw.rtv0;
  record.depth_stencil = draw.dsv;
  record.viewport = draw.viewport;
  record.has_viewport = draw.has_viewport;
  return record;
}

inline void CaptureCameraFromBytes(const std::vector<uint8_t>& bytes, uint32_t frame, uint8_t source = 1u) {
  const size_t float_count = bytes.size() / sizeof(float);
  if (float_count < 96u) return;

  const auto* data = reinterpret_cast<const float*>(bytes.data());
  auto& camera = g_state.camera;
  std::memcpy(camera.view, data + 0u, sizeof(float) * 16u);
  std::memcpy(camera.view_inv, data + 16u, sizeof(float) * 16u);
  std::memcpy(camera.proj, data + 32u, sizeof(float) * 16u);
  std::memcpy(camera.view_proj, data + 64u, sizeof(float) * 16u);
  std::memcpy(camera.view_proj_inv, data + 80u, sizeof(float) * 16u);
  camera.has_prev = false;
  if (float_count >= 316u) {
    std::memcpy(camera.prev_view_proj, data + 300u, sizeof(float) * 16u);
    camera.has_prev = true;
  }
  camera.valid = true;
  camera.frame = frame;
  camera.source = source;
}

inline void CaptureCameraFromLighting(
    reshade::api::device* device,
    reshade::api::resource cb,
    uint32_t frame) {
  if (device == nullptr || cb.handle == 0u) return;
  const auto bytes = renodx::utils::constants::GetResourceCache(device, cb);
  if (bytes.empty()) return;
  CaptureCameraFromBytes(bytes, frame);
}

inline void RunPendingMeshCapture(reshade::api::command_queue* queue) {
  if (queue == nullptr) return;

  DrawRecord draw;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.mesh_capture_pending) return;
    g_state.mesh_capture_pending = false;
    draw = g_state.captured.draw;
  }

  auto* device = queue->get_device();
  renodx::utils::scene::CapturedMesh mesh;
  std::string error;
  const bool ok = renodx::utils::scene::CaptureDrawIndexed(
      device, queue, ToSceneRecord(draw), &mesh, &error);

  std::lock_guard<std::mutex> lock(g_state.mutex);
  if (ok) {
    g_state.captured.mesh = std::move(mesh);
    g_state.captured.mesh_valid = true;
    g_state.captured.mesh_status = "ok";
  } else {
    g_state.captured.mesh_valid = false;
    g_state.captured.mesh_status = error.empty() ? "capture failed" : error;
  }
}

}  // namespace falcom_world
