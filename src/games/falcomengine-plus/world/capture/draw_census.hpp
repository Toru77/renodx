#pragma once

// Phase 0 draw census.
//
// Records a bounded ring of draw metadata plus per-VS-hash aggregates while
// the research toggle is on. The census never reads back GPU data; the armed
// capture snapshots CB bytes from utils::constants' CPU cache and issues a
// split structured-buffer copy on the game's command list (resolved at
// present) so dynamic buffers cannot alias the captured draw.

#include <algorithm>
#include <type_traits>

#include "../world_state.hpp"
#include "../bvh/bvh_pool.hpp"
#include "../capture/buffer_readback.hpp"
#include "../capture/cb_tracking.hpp"
#include "../capture/mesh_capture.hpp"
#include "../capture/state_capture.hpp"
#include "../research/classification.hpp"
#include "../../../../utils/command_action.hpp"
#include "../../../../utils/constants.hpp"

namespace falcom_world {

inline uint64_t QueryResourceSize(reshade::api::device* device, reshade::api::resource resource) {
  if (device == nullptr || resource.handle == 0u) return 0u;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.resource_sizes.find(resource.handle);
    if (it != g_state.resource_sizes.end()) return it->second;
  }
  const auto desc = device->get_resource_desc(resource);
  const uint64_t size = (desc.type == reshade::api::resource_type::buffer) ? desc.buffer.size : 0u;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.resource_sizes.insert_or_assign(resource.handle, size);
  }
  return size;
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
  g_state.resource_sizes.clear();
  g_state.next_serial.store(0u);
  g_state.arm_active = false;
  g_state.arm_window_open = false;
  g_state.captured = {};
  g_state.candidates.clear();
  g_state.selected_family = 0u;
  g_state.selected_candidate = 0;
  g_state.status = "census reset";
}

inline void ArmCapture(uint32_t vs_hash, uint32_t serial) {
  std::lock_guard<std::mutex> lock(g_state.mutex);
  g_state.arm_active = true;
  g_state.arm_vs_hash = vs_hash;
  g_state.arm_serial = serial;
  g_state.arm_vs_set.clear();
  g_state.status = "armed for next matching draw";
}

inline void SnapshotDrawConstants(
    reshade::api::device* device,
    WorldCommandListData* cl_data,
    CapturedDraw* out) {
  if (device == nullptr || cl_data == nullptr || out == nullptr) return;
  const auto snapshot = [&](uint8_t stage, const std::array<reshade::api::resource, kCbSlotCapacity>& slots) {
    for (uint32_t slot = 0; slot < kCbSlotCapacity; ++slot) {
      if (out->cbs.size() >= kMaxCbSnapshots) return;
      const reshade::api::resource resource = slots[slot];
      if (resource.handle == 0u) continue;
      auto bytes = renodx::utils::constants::GetResourceCache(device, resource);
      if (bytes.empty() || bytes.size() > kMaxCbBytes) continue;
      CbSnapshot cb;
      cb.stage = stage;
      cb.slot = slot;
      cb.bytes = std::move(bytes);
      out->cbs.push_back(std::move(cb));
    }
  };
  snapshot(1u, cl_data->vs_cb);
  snapshot(2u, cl_data->ps_cb);
}

inline void ParseInstanceOffset(CapturedDraw* out) {
  if (out == nullptr) return;
  out->instance_offset_found = false;
  out->instance_offset_g = 0;
  for (const auto& cb : out->cbs) {
    if (cb.stage != 1u || cb.slot != 1u || cb.bytes.size() < 4u) continue;
    int32_t value = 0;
    std::memcpy(&value, cb.bytes.data(), sizeof(int32_t));
    out->instance_offset_g = value;
    out->instance_offset_found = true;
    return;
  }
}

inline void TryCaptureArmedDraw(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    const DrawRecord& draw,
    WorldCommandListData* cl_data) {
  std::lock_guard<std::mutex> lock(g_state.mutex);
  const bool in_arm_set =
      !g_state.arm_vs_set.empty()
      && std::find(g_state.arm_vs_set.begin(), g_state.arm_vs_set.end(), draw.vs_hash) != g_state.arm_vs_set.end();
  if (g_state.auto_research.active && g_state.arm_window_open && in_arm_set) {
    g_state.auto_research.armed_draws[draw.vs_hash] += 1u;
    g_state.auto_research.round_armed_draws[draw.vs_hash] += 1u;
    g_state.auto_research.window_armed_draws[draw.vs_hash] += 1u;
  }
  if (!g_state.arm_active) return;
  if (!g_state.arm_vs_set.empty()) {
    if (!in_arm_set) return;
  } else if (g_state.arm_vs_hash != 0u && g_state.arm_vs_hash != draw.vs_hash) {
    return;
  }
  if (g_state.arm_serial != 0u && g_state.arm_serial != draw.serial) return;

  if (g_state.auto_research.active && in_arm_set) {
    const auto retry_it = g_state.auto_research.probe_retry_counts.find(draw.vs_hash);
    const uint32_t retries =
        retry_it != g_state.auto_research.probe_retry_counts.end() ? retry_it->second : 0u;
    if (retries < kProbeRetryBudget) {
      const auto rejected_it = g_state.auto_research.probe_rejected_draws.find(draw.vs_hash);
      if (rejected_it != g_state.auto_research.probe_rejected_draws.end()) {
        auto& rejected = rejected_it->second;
        const auto key_it = rejected.find(HintDrawKey(draw));
        if (key_it != rejected.end()) {
          if (key_it->second > 0u) {
            key_it->second -= 1u;
            // A skipped retired draw continues the same scheduling window; it
            // must not be counted as a fresh arm window or round window.
            g_state.auto_research.window_continuation = true;
            return;
          }
          rejected.erase(key_it);
        }
      }
    }
  }

  g_state.arm_active = false;
  g_state.captured = {};
  g_state.captured.draw = draw;
  g_state.candidates.clear();
  g_state.selected_candidate = 0;
  SnapshotDrawConstants(device, cl_data, &g_state.captured);
  ParseInstanceOffset(&g_state.captured);
  IssueStructuredBufferCopies(cmd_list, device, cl_data, draw, &g_state.captured);
  IssueCbCopies(cmd_list, device, cl_data, draw);

  if (draw.rtv0.handle != 0u) {
    const auto rtv_resource = device->get_resource_from_view(draw.rtv0);
    if (rtv_resource.handle != 0u) {
      const auto desc = device->get_resource_desc(rtv_resource);
      if (desc.type == reshade::api::resource_type::texture_2d) {
        g_state.captured.rtv_w = desc.texture.width;
        g_state.captured.rtv_h = desc.texture.height;
      }
    }
  }

  g_state.mesh_capture_pending = true;
  g_state.status = "captured draw serial " + std::to_string(draw.serial)
                   + " vs=0x" + std::to_string(draw.vs_hash)
                   + " cbs=" + std::to_string(g_state.captured.cbs.size())
                   + " srvs=" + std::to_string(g_state.captured.srv_buffers.size());
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
      if (GetRasterizerDesc(rs_it->second.handle, &desc)) record->cull_mode = static_cast<uint32_t>(desc.cull_mode);
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
      record->vb_size = QueryResourceSize(device, vb.handle);
    }
    record->ib = scene_state->index_buffer.handle;
    record->ib_offset = scene_state->index_buffer.offset;
    record->index_size = scene_state->index_buffer.index_size;
    record->has_index_buffer = scene_state->has_index_buffer;
    record->ib_size = QueryResourceSize(device, record->ib);
  }
}

inline void CommitDrawRecord(
    const DrawRecord& record,
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    WorldCommandListData* cl_data) {
  {
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

  if (IsLightingHash(record.ps_hash) && cl_data != nullptr) {
    bool captured_frame_matches = false;
    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      captured_frame_matches = g_state.captured.draw.vs_hash != 0u
                               && g_state.captured.draw.frame == record.frame;
    }
    const reshade::api::resource cb = cl_data->ps_cb[0].handle != 0u ? cl_data->ps_cb[0] : cl_data->vs_cb[0];
    if (cb.handle != 0u) {
      const auto bytes = renodx::utils::constants::GetResourceCache(device, cb);
      if (!bytes.empty()) {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        CaptureCameraFromBytes(bytes, record.frame, 1u);
      }
    }
    if (captured_frame_matches && cl_data->ps_cb[0].handle != 0u) {
      IssueCameraCbCopy(cmd_list, device, cl_data, record.frame);
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
    }
  }

  TryCaptureArmedDraw(device, cmd_list, record, cl_data);
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
    if (!CensusEnabled() || context.cmd_list == nullptr) return {};
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

    CommitDrawRecord(record, device, context.cmd_list, cl_data);
    bvh::OnPoolScanDraw(device, context.cmd_list, record, cl_data);
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
    if (!CensusEnabled() || context.cmd_list == nullptr) return {};
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

    auto& pending = PendingIndirectDraws();
    if (pending.size() >= kMaxIndirectCallsPerFrame) {
      IndirectDroppedCalls() += 1u;
      return {};
    }

    if (draw_count > kMaxIndirectDrawsPerCall) draw_count = kMaxIndirectDrawsPerCall;
    const uint64_t read_size = static_cast<uint64_t>(draw_count) * stride;
    if (read_size == 0u) return {};

    auto* cl_data = GetWorldCommandListData(context.cmd_list);
    reshade::api::resource instance_cb = {0u};
    uint64_t cb_size = 0u;
    if (cl_data != nullptr) {
      instance_cb = cl_data->vs_cb[bvh::kPoolInstanceCbSlot];
      if (instance_cb.handle != 0u) {
        const auto desc = device->get_resource_desc(instance_cb);
        if (desc.type == reshade::api::resource_type::buffer) {
          cb_size = (std::min)(desc.buffer.size, static_cast<uint64_t>(16u));
        }
        if (cb_size < sizeof(int32_t)) cb_size = 0u;
      }
    }

    if (IndirectStagingUsed() + read_size + cb_size > kMaxIndirectReadBytes) {
      IndirectDroppedCalls() += 1u;
      return {};
    }
    if (!EnsureIndirectStaging(device, IndirectStagingUsed() + read_size + cb_size)) {
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
    if (cb_size != 0u) {
      draw.cb_size = static_cast<uint32_t>(cb_size);
      draw.cb_staging_offset = IndirectStagingUsed();
      context.cmd_list->copy_buffer_region(
          instance_cb, 0u, IndirectStagingResource(), IndirectStagingUsed(), cb_size);
      IndirectStagingUsed() += cb_size;
    }

    FillCommonDrawRecord(&draw.record, context.cmd_list, device);
    draw.record.method = indexed ? 1u : 0u;
    draw.record.frame = g_state.frame.load();
    FillDrawBindingMasks(&draw.record, cl_data);
    if (cl_data != nullptr) draw.cl_data = *cl_data;
    pending.push_back(std::move(draw));
    return {};
  }
};

// Resolves queued indirect draws at present: the args were copied on the game's
// command list at the draw, so the counts match what the GPU executed. Each
// sub-draw runs through the same classification/commit/pool-scan pipeline as a
// direct draw, using the bindings captured at draw time.
inline void ResolveIndirectDraws(reshade::api::command_queue* queue) {
  if (queue == nullptr) return;
  auto& pending = PendingIndirectDraws();
  if (pending.empty()) return;
  auto* device = queue->get_device();
  auto* cmd_list = queue->get_immediate_command_list();
  if (device == nullptr || cmd_list == nullptr) {
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
      int32_t base = 0;
      const bool has_base = draw.cb_size >= sizeof(int32_t);
      if (has_base) {
        std::memcpy(&base, base_bytes + draw.cb_staging_offset, sizeof(int32_t));
      }
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
        CommitDrawRecord(record, device, cmd_list, &draw.cl_data);
        bvh::OnPoolScanDraw(device, cmd_list, record, &draw.cl_data, has_base ? base : INT32_MIN);
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
  if (CensusEnabled() && renodx::utils::constants::shared.data != nullptr) {
    renodx::utils::constants::shared.data->capture_constant_buffers = true;
  }
  if (queue == nullptr) return;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    ResolveStructuredBufferCopies(queue, &g_state.captured);
    ResolveCbCopies(queue, &g_state.captured);
    ParseInstanceOffset(&g_state.captured);
  }
  ResolveIndirectDraws(queue);
  RunPendingMeshCapture(queue);
  bvh::DrainPoolScan(queue->get_device(), queue);
}

}  // namespace falcom_world
