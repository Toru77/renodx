#pragma once

// Draw observation: the census and the lighting-pass inputs.
//
// While the census toggle is on, records a bounded ring of draw metadata plus
// per-VS-hash aggregates. Whenever draw observation is active (census, Pool
// Scan or a GPU debug view) every draw also feeds the world pool, and the
// lighting pass provides the camera (cb0, from utils::constants' CPU cache)
// and the game depth view.

#include <algorithm>
#include <type_traits>

#include "../world_state.hpp"
#include "../rtao/rtao_state.hpp"
#include "../bvh/bvh_pool.hpp"
#include "../bvh/deform_live.hpp"
#include "../bvh/deform_probe.hpp"
#include "../capture/buffer_readback.hpp"
#include "../capture/cb_tracking.hpp"
#include "../capture/camera_capture.hpp"
#include "../capture/state_capture.hpp"
#include "../research/classification.hpp"
#include "../../../../utils/command_action.hpp"
#include "../../../../utils/constants.hpp"

namespace falcom_world {

inline BufferInfo QueryBufferInfo(reshade::api::device* device, reshade::api::resource resource) {
  if (device == nullptr || resource.handle == 0u) return {};
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.buffer_info.find(resource.handle);
    if (it != g_state.buffer_info.end()) return it->second;
  }
  const auto desc = device->get_resource_desc(resource);
  BufferInfo info;
  if (desc.type == reshade::api::resource_type::buffer) {
    info.size = desc.buffer.size;
    info.usage = static_cast<uint32_t>(desc.usage);
    info.flags = static_cast<uint32_t>(desc.flags);
  }
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.buffer_info.insert_or_assign(resource.handle, info);
  }
  return info;
}

inline void SetStatus(const std::string& status) {
  std::lock_guard<std::mutex> lock(g_state.mutex);
  g_state.status = status;
}

inline void ResetCensus() {
  std::lock_guard<std::mutex> lock(g_state.mutex);
  g_state.ring.clear();
  g_state.ring_head = 0u;
  g_state.families.clear();
  g_state.buffer_info.clear();
  g_state.next_serial.store(0u);
  g_state.status = "census reset";
}

inline bool LayoutHasSkinInputs(const reshade::api::pipeline& input_layout) {
  if (input_layout.handle == 0u || renodx::utils::scene::shared.data == nullptr) return false;
  bool has_skin = false;
  renodx::utils::scene::shared.data->input_layouts.if_contains(input_layout.handle, [&](const auto& pair) {
    for (const auto& element : pair.second.elements) {
      const char* semantic = element.semantic.c_str();
      if (semantic != nullptr && std::strncmp(semantic, "BLEND", 5) == 0) {
        has_skin = true;
        break;
      }
    }
  });
  return has_skin;
}

inline void FillCommonDrawRecord(
    DrawRecord* record,
    reshade::api::command_list* cmd_list,
    reshade::api::device* device) {
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state != nullptr) {
    record->vs_hash = renodx::utils::shader::GetCurrentVertexShaderHash(shader_state);
    record->ps_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
    record->vs_pipeline = shader_state->stage_states[renodx::utils::shader::VERTEX_INDEX].pipeline.handle;
    record->ps_pipeline = shader_state->stage_states[renodx::utils::shader::PIXEL_INDEX].pipeline.handle;
  }

  auto* render_state = renodx::utils::state::GetCurrentState(cmd_list);
  if (render_state != nullptr) {
    if (!render_state->render_targets.empty()) record->rtv0 = render_state->render_targets[0];
    record->dsv = render_state->depth_stencil;
    record->topology = render_state->primitive_topology;
    if (!render_state->viewports.empty()) {
      record->viewport = render_state->viewports[0];
      record->has_viewport = true;
    }
    const auto ia_it = render_state->pipelines.find(reshade::api::pipeline_stage::input_assembler);
    if (ia_it != render_state->pipelines.end()) record->input_layout = ia_it->second;

    const auto blend_it = render_state->pipelines.find(reshade::api::pipeline_stage::output_merger);
    if (blend_it != render_state->pipelines.end()) {
      reshade::api::blend_desc desc = {};
      if (GetBlendDesc(blend_it->second.handle, &desc)) record->blend_enable = desc.blend_enable[0];
    }
    const auto ds_it = render_state->pipelines.find(reshade::api::pipeline_stage::depth_stencil);
    if (ds_it != render_state->pipelines.end()) {
      reshade::api::depth_stencil_desc desc = {};
      if (GetDepthStencilDesc(ds_it->second.handle, &desc)) {
        record->depth_enable = desc.depth_enable;
        record->depth_write = desc.depth_write_mask;
      } else {
        // D3D11 default depth-stencil state: test on, write all.
        record->depth_enable = true;
        record->depth_write = true;
      }
    }
    const auto rs_it = render_state->pipelines.find(reshade::api::pipeline_stage::rasterizer);
    if (rs_it != render_state->pipelines.end()) {
      reshade::api::rasterizer_desc desc = {};
      if (GetRasterizerDesc(rs_it->second.handle, &desc)) {
        record->cull_mode = static_cast<uint32_t>(desc.cull_mode);
        record->front_counter_clockwise = desc.front_counter_clockwise;
      }
    }
  }

  record->has_skin_inputs = LayoutHasSkinInputs(record->input_layout);

  auto* scene_state = renodx::utils::data::Get<renodx::utils::scene::SceneCommandListData>(cmd_list);
  if (scene_state != nullptr) {
    if (!scene_state->vertex_buffers.empty()) {
      const auto& vb = scene_state->vertex_buffers[0];
      record->vb = vb.handle;
      record->vb_offset = vb.offset;
      record->vb_stride = vb.stride;
      const BufferInfo vb_info = QueryBufferInfo(device, vb.handle);
      record->vb_size = vb_info.size;
      record->vb_usage = vb_info.usage;
      record->vb_flags = vb_info.flags;
    }
    record->ib = scene_state->index_buffer.handle;
    record->ib_offset = scene_state->index_buffer.offset;
    record->index_size = scene_state->index_buffer.index_size;
    record->has_index_buffer = scene_state->has_index_buffer;
    const BufferInfo ib_info = QueryBufferInfo(device, record->ib);
    record->ib_size = ib_info.size;
    record->ib_usage = ib_info.usage;
    record->ib_flags = ib_info.flags;
  }
}

// Camera and lighting depth come from the lighting pass; the pool region and
// the GPU debug views read them.
inline void CaptureLightingInputs(
    const DrawRecord& record,
    reshade::api::device* device,
    WorldCommandListData* cl_data) {
  if (!IsLightingHash(record.ps_hash) || cl_data == nullptr) return;
  const reshade::api::resource cb = cl_data->ps_cb[0].handle != 0u ? cl_data->ps_cb[0] : cl_data->vs_cb[0];
  if (cb.handle != 0u) {
    const auto bytes = renodx::utils::constants::GetResourceCache(device, cb);
    if (!bytes.empty()) {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      CaptureCameraFromBytes(bytes, record.frame, 1u);
    }
  }
  const reshade::api::resource_view depth_view = cl_data->ps_srv_view[kLightingDepthRegisterSora2nd];
  const reshade::api::resource depth_resource = cl_data->ps_srv[kLightingDepthRegisterSora2nd];
  if (depth_view.handle != 0u && depth_resource.handle != 0u) {
    uint32_t width = 0u;
    uint32_t height = 0u;
    const auto desc = device->get_resource_desc(depth_resource);
    if (desc.type == reshade::api::resource_type::texture_2d) {
      width = desc.texture.width;
      height = desc.texture.height;
    }
    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.depth_source.valid = true;
    g_state.depth_source.frame = record.frame;
    g_state.depth_source.view = depth_view;
    g_state.depth_source.resource = depth_resource;
    g_state.depth_source.width = width;
    g_state.depth_source.height = height;
    g_state.depth_source.viewport_x = record.has_viewport ? record.viewport.x : 0.f;
    g_state.depth_source.viewport_y = record.has_viewport ? record.viewport.y : 0.f;
    g_state.depth_source.viewport_width = record.has_viewport ? record.viewport.width : 0.f;
    g_state.depth_source.viewport_height = record.has_viewport ? record.viewport.height : 0.f;
  }
}

inline void CommitDrawRecord(
    const DrawRecord& record,
    reshade::api::device* device,
    WorldCommandListData* cl_data) {
  // Two-Sided discovery (diagnostic): camera-view opaque candidate draws by (cull mode, front face). Only while on.
  if (rtao::g_rtao_discovery > 0.5f && record.is_candidate && !record.blend_enable
      && contract::IsCameraViewVertex(record.vs_pipeline)) {
    rtao::g_rtao_cull_hist[std::min(record.cull_mode, 3u)][record.front_counter_clockwise ? 1 : 0].fetch_add(1u, std::memory_order_relaxed);
  }
  const bool census = CensusEnabled();
  if (census) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (g_state.ring.size() < kCensusRingSize) {
      g_state.ring.push_back(record);
    } else {
      g_state.ring[g_state.ring_head] = record;
    }
    g_state.ring_head = (g_state.ring_head + 1u) % kCensusRingSize;

    auto& family = g_state.families[record.vs_hash];
    family.vs_hash = record.vs_hash;
    family.draws += 1u;
    family.indices += record.index_count;
    family.triangles += static_cast<uint64_t>(record.index_count / 3u)
                        * (record.instance_count == 0u ? 1u : record.instance_count);
    family.instances += (record.instance_count == 0u ? 1u : record.instance_count);
    if (record.depth_write) family.depth_write_draws += 1u;
    if (record.blend_enable) family.blend_draws += 1u;
    if (record.has_skin_inputs) family.skin_draws += 1u;
    if (record.is_candidate) {
      family.candidate = true;
      family.candidate_draws += 1u;
    }
    family.meshes.insert(MeshKey(record));
    family.ps_hashes.insert(record.ps_hash);
  }

  CaptureLightingInputs(record, device, cl_data);
}

// Records which constant-buffer and VS SRV slots are bound for a draw so the
// census/report can reproduce the draw's binding footprint.
inline void FillDrawBindingMasks(DrawRecord* record, const WorldCommandListData* cl_data) {
  if (record == nullptr || cl_data == nullptr) return;
  for (uint32_t slot = 0; slot < kCbSlotCapacity; ++slot) {
    if (cl_data->vs_cb[slot].handle != 0u) {
      record->vs_cb_mask |= (1u << slot);
      if (slot == 0u) record->vs_cb0 = cl_data->vs_cb[slot];
    }
    if (cl_data->ps_cb[slot].handle != 0u) {
      record->ps_cb_mask |= (1u << slot);
      if (slot == 0u) record->ps_cb0 = cl_data->ps_cb[slot];
    }
  }
  for (uint32_t slot = 0; slot < kSrvSlotCapacity; ++slot) {
    if (cl_data->vs_srv[slot].handle != 0u) {
      record->vs_srv_mask |= (1u << slot);
      if (record->vs_srv0.handle == 0u) record->vs_srv0 = cl_data->vs_srv[slot];
    }
  }
}

struct WorldDrawCallback {
  template <typename Arguments>
  renodx::utils::command_action::CallbackResult<renodx::utils::command_action::CommandContext<Arguments>>
  Record(renodx::utils::command_action::CommandContext<Arguments>& context, uint8_t method) const {
    if (!CaptureActive() || context.cmd_list == nullptr) return {};
    auto* device = context.cmd_list->get_device();
    if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return {};

    DrawRecord record;
    record.method = method;
    record.frame = g_state.frame.load();
    record.serial = g_state.next_serial.fetch_add(1u);
    if constexpr (std::is_same_v<Arguments, renodx::utils::command_action::DrawArguments>) {
      record.vertex_count = context.arguments.vertex_count;
      record.instance_count = context.arguments.instance_count;
      record.first_vertex = context.arguments.first_vertex;
      record.first_instance = context.arguments.first_instance;
    } else {
      record.index_count = context.arguments.index_count;
      record.instance_count = context.arguments.instance_count;
      record.first_index = context.arguments.first_index;
      record.vertex_offset = context.arguments.vertex_offset;
      record.first_instance = context.arguments.first_instance;
    }

    FillCommonDrawRecord(&record, context.cmd_list, device);
    record.is_candidate = IsGeometryCandidate(record);

    auto* cl_data = GetWorldCommandListData(context.cmd_list);
    FillDrawBindingMasks(&record, cl_data);

    CommitDrawRecord(record, device, cl_data);
    bvh::OnPoolScanDraw(device, context.cmd_list, record, cl_data);
    bvh::OnDeformProbeDraw(device, context.cmd_list, record);
    bvh::OnDeformCaptureDraw(device, context.cmd_list, record);
    return {};
  }

  renodx::utils::command_action::CallbackResult<renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawArguments>>
  operator()(renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawArguments>& context) const {
    return Record(context, 0u);
  }

  renodx::utils::command_action::CallbackResult<renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawIndexedArguments>>
  operator()(renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawIndexedArguments>& context) const {
    return Record(context, 1u);
  }

  // Indirect draws carry their counts in a GPU buffer; the copy is issued on
  // the game's command list here and resolved at present, so the args match
  // the draw (the buffer can be rewritten later in the frame).
  renodx::utils::command_action::CallbackResult<renodx::utils::command_action::CommandContext<renodx::utils::command_action::IndirectArguments>>
  operator()(renodx::utils::command_action::CommandContext<renodx::utils::command_action::IndirectArguments>& context) const {
    if (!CaptureActive() || context.cmd_list == nullptr) return {};
    auto* device = context.cmd_list->get_device();
    if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return {};

    const auto& args = context.arguments;
    if (args.buffer.handle == 0u) return {};
    switch (args.command) {
      case reshade::api::indirect_command::unknown:
        if (args.unknown_command_is_dispatch) return {};
        break;
      case reshade::api::indirect_command::draw:
      case reshade::api::indirect_command::draw_indexed:
        break;
      default:
        return {};
    }
    const bool indexed = args.command != reshade::api::indirect_command::draw;
    // D3D11 reports a single draw with stride 0 (the args layout is implied):
    // 20 bytes for indexed draws, 16 for non-indexed. Other APIs pass the
    // actual stride/count.
    const uint32_t stride = args.stride != 0u ? args.stride : (indexed ? 20u : 16u);
    uint32_t draw_count = args.draw_count != 0u ? args.draw_count : 1u;

    auto* cl_data = GetWorldCommandListData(context.cmd_list);
    DrawRecord record;
    FillCommonDrawRecord(&record, context.cmd_list, device);
    record.method = indexed ? 1u : 0u;
    record.frame = g_state.frame.load();
    FillDrawBindingMasks(&record, cl_data);

    // The pool copies b1, the args and the instance window on the command list
    // here and reads them back later without waiting.
    bvh::OnPoolScanIndirectDraw(
        device, context.cmd_list, record, cl_data, args.buffer, args.offset, draw_count, stride);
    bvh::OnDeformProbeIndirectDraw(device, context.cmd_list, record, args.buffer, args.offset, draw_count, args.stride);

    // The census reads the args back at present with a GPU wait, so it only
    // runs while the census itself is on.
    if (!CensusEnabled()) return {};

    auto& pending = PendingIndirectDraws();
    if (pending.size() >= kMaxIndirectCallsPerFrame) {
      IndirectDroppedCalls() += 1u;
      return {};
    }

    if (draw_count > kMaxIndirectDrawsPerCall) draw_count = kMaxIndirectDrawsPerCall;
    const uint64_t read_size = static_cast<uint64_t>(draw_count) * stride;
    if (read_size == 0u) return {};

    if (IndirectStagingUsed() + read_size > kMaxIndirectReadBytes) {
      IndirectDroppedCalls() += 1u;
      return {};
    }
    if (!EnsureIndirectStaging(device, IndirectStagingUsed() + read_size)) {
      IndirectDroppedCalls() += 1u;
      return {};
    }

    PendingIndirectDraw draw;
    draw.indexed = indexed;
    draw.args_buffer = args.buffer;
    draw.args_offset = args.offset;
    draw.draw_count = draw_count;
    draw.stride = stride;
    draw.read_size = read_size;
    draw.staging_offset = IndirectStagingUsed();
    context.cmd_list->copy_buffer_region(
        args.buffer, args.offset, IndirectStagingResource(), IndirectStagingUsed(), read_size);
    IndirectStagingUsed() += read_size;

    draw.record = record;
    if (cl_data != nullptr) draw.cl_data = *cl_data;
    pending.push_back(std::move(draw));
    return {};
  }
};

// Resolves queued indirect draws at present: the args were copied on the game's
// command list at the draw, so the counts match what the GPU executed. Each
// sub-draw runs through the same classification/commit pipeline as a direct
// draw, using the bindings captured at draw time. The pool handles indirect
// draws itself (OnPoolScanIndirectDraw) and does not use this path.
inline void ResolveIndirectDraws(reshade::api::command_queue* queue) {
  if (queue == nullptr) return;
  auto& pending = PendingIndirectDraws();
  if (pending.empty()) return;
  auto* device = queue->get_device();
  if (device == nullptr) {
    pending.clear();
    IndirectStagingUsed() = 0u;
    return;
  }
  queue->flush_immediate_command_list();
  queue->wait_idle();

  void* mapped = nullptr;
  const uint64_t map_size = IndirectStagingUsed();
  const bool mapped_ok =
      map_size != 0u && IndirectStagingResource().handle != 0u
      && device->map_buffer_region(
             IndirectStagingResource(), 0u, map_size, reshade::api::map_access::read_only, &mapped)
      && mapped != nullptr;
  if (mapped_ok) {
    const auto* base_bytes = static_cast<const uint8_t*>(mapped);
    for (auto& draw : pending) {
      const uint8_t* args_bytes = base_bytes + draw.staging_offset;
      for (uint32_t i = 0; i < draw.draw_count; ++i) {
        DrawRecord record = draw.record;
        const uint8_t* element = args_bytes + static_cast<uint64_t>(i) * draw.stride;
        if (draw.indexed) {
          if (draw.stride < 20u) break;
          std::memcpy(&record.index_count, element + 0u, sizeof(uint32_t));
          std::memcpy(&record.instance_count, element + 4u, sizeof(uint32_t));
          std::memcpy(&record.first_index, element + 8u, sizeof(uint32_t));
          std::memcpy(&record.vertex_offset, element + 12u, sizeof(uint32_t));
          std::memcpy(&record.first_instance, element + 16u, sizeof(uint32_t));
        } else {
          if (draw.stride < 16u) break;
          std::memcpy(&record.vertex_count, element + 0u, sizeof(uint32_t));
          std::memcpy(&record.instance_count, element + 4u, sizeof(uint32_t));
          std::memcpy(&record.first_vertex, element + 8u, sizeof(uint32_t));
          std::memcpy(&record.first_instance, element + 12u, sizeof(uint32_t));
        }
        record.serial = g_state.next_serial.fetch_add(1u);
        record.is_candidate = IsGeometryCandidate(record);
        CommitDrawRecord(record, device, &draw.cl_data);
      }
    }
    device->unmap_buffer_region(IndirectStagingResource());
  }
  pending.clear();
  IndirectStagingUsed() = 0u;
}

inline void RegisterCensus() {
  renodx::utils::command_action::RegisterCallback(
      WorldDrawCallback{},
      {.shader_hash = 0u,
       .command_types = renodx::utils::command_action::COMMAND_TYPE_DIRECT_DRAW
                        | renodx::utils::command_action::COMMAND_TYPE_INDIRECT});
}

inline void OnWorldPresent(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  (void)swapchain;
  (void)source_rect;
  (void)dest_rect;
  (void)dirty_rect_count;
  (void)dirty_rects;
  g_state.frame.fetch_add(1u);
  g_state.draw_counter = 0u;
  if (CaptureActive() && renodx::utils::constants::shared.data != nullptr) {
    renodx::utils::constants::shared.data->capture_constant_buffers = true;
  }
  if (queue == nullptr) return;
  ResolveIndirectDraws(queue);
  bvh::DrainPoolScan(queue->get_device(), queue);
  bvh::DrainDeformProbe(queue->get_device(), queue);
}

}  // namespace falcom_world
