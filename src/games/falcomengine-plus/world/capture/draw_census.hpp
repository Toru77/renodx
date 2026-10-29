#pragma once

// Draw observation: the census and the lighting-pass inputs.
//
// While the census toggle is on, records a bounded ring of draw metadata plus
// per-VS-hash aggregates. Whenever draw observation is active (census, Pool
// Scan or a GPU debug view) every draw also feeds the world pool, and the
// lighting pass provides the camera (cb0, from utils::constants' CPU cache)
// and the game depth view.

#include <algorithm>
#include <array>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../world_state.hpp"
#include "../rtao/rtao_state.hpp"
#include "../rtsun/rtsun_state.hpp"
#include "../bvh/bvh_pool.hpp"
#include "../bvh/bvh_live.hpp"
#include "../bvh/deform_live.hpp"
#include "../bvh/deform_probe.hpp"
#include "../bvh/gpu_timer.hpp"
#include "../bvh/p2b_diag.hpp"
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
  thread_local uint32_t cache_generation = 0u;
  thread_local std::unordered_map<uint64_t, BufferInfo> cache;
  const uint32_t generation = g_record_cache_generation.load(std::memory_order_acquire);
  if (generation != cache_generation) {
    cache.clear();
    cache_generation = generation;
  }
  if (const auto cached = cache.find(resource.handle); cached != cache.end()) return cached->second;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.buffer_info.find(resource.handle);
    if (it != g_state.buffer_info.end()) {
      cache.insert_or_assign(resource.handle, it->second);
      return it->second;
    }
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
  g_record_cache_generation.fetch_add(1u, std::memory_order_release);
  g_state.next_serial.store(0u);
  g_state.status = "census reset";
}

inline bool LayoutHasSkinInputs(const reshade::api::pipeline& input_layout) {
  if (input_layout.handle == 0u || renodx::utils::scene::shared.data == nullptr) return false;
  thread_local uint32_t cache_generation = 0u;
  thread_local std::unordered_map<uint64_t, bool> cache;
  const uint32_t generation = g_record_cache_generation.load(std::memory_order_acquire);
  if (generation != cache_generation) {
    cache.clear();
    cache_generation = generation;
  }
  if (const auto cached = cache.find(input_layout.handle); cached != cache.end()) return cached->second;
  bool has_skin = false;
  renodx::utils::scene::shared.data->input_layouts.if_contains(input_layout.handle, [&](const auto& pair) {
    for (const auto& element : pair.second.elements) {
      const char* semantic = element.semantic.c_str();
      if (semantic != nullptr && std::strncmp(semantic, "BLEND", 5) == 0) {
        has_skin = true;
        break;
      }
    }
    cache.insert_or_assign(input_layout.handle, has_skin);
  });
  return has_skin;
}

// Render state read straight from the D3D11 context (M1). The handles are the native state objects, the keys of StateMaps.
inline void FillRenderStateFromNative(DrawRecord* record, reshade::api::command_list* cmd_list) {
  auto* native = reinterpret_cast<ID3D11DeviceContext*>(static_cast<uintptr_t>(cmd_list->get_native()));

  ID3D11RenderTargetView* rtv = nullptr;
  ID3D11DepthStencilView* dsv = nullptr;
  native->OMGetRenderTargets(1u, &rtv, &dsv);
  record->rtv0 = {reinterpret_cast<uintptr_t>(rtv)};
  record->dsv = {reinterpret_cast<uintptr_t>(dsv)};
  if (rtv != nullptr) rtv->Release();
  if (dsv != nullptr) dsv->Release();

  D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
  native->IAGetPrimitiveTopology(&topology);
  record->topology = static_cast<reshade::api::primitive_topology>(topology);

  std::array<D3D11_VIEWPORT, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> viewports = {};
  UINT viewport_count = static_cast<UINT>(viewports.size());
  native->RSGetViewports(&viewport_count, viewports.data());
  if (viewport_count != 0u) {
    const auto& viewport = viewports[0];
    record->viewport = {static_cast<float>(viewport.TopLeftX), static_cast<float>(viewport.TopLeftY),
                        static_cast<float>(viewport.Width), static_cast<float>(viewport.Height),
                        viewport.MinDepth, viewport.MaxDepth};
    record->has_viewport = true;
  }

  ID3D11InputLayout* input_layout = nullptr;
  native->IAGetInputLayout(&input_layout);
  record->input_layout = {reinterpret_cast<uintptr_t>(input_layout)};
  if (input_layout != nullptr) input_layout->Release();

  ID3D11BlendState* blend_state = nullptr;
  FLOAT blend_factor[4] = {};
  UINT sample_mask = 0u;
  native->OMGetBlendState(&blend_state, blend_factor, &sample_mask);
  if (blend_state != nullptr) {
    reshade::api::blend_desc desc = {};
    if (GetBlendDesc(reinterpret_cast<uintptr_t>(blend_state), &desc)) record->blend_enable = desc.blend_enable[0];
    blend_state->Release();
  }

  ID3D11DepthStencilState* depth_state = nullptr;
  UINT stencil_ref = 0u;
  native->OMGetDepthStencilState(&depth_state, &stencil_ref);
  if (depth_state != nullptr) {
    reshade::api::depth_stencil_desc desc = {};
    if (GetDepthStencilDesc(reinterpret_cast<uintptr_t>(depth_state), &desc)) {
      record->depth_enable = desc.depth_enable;
      record->depth_write = desc.depth_write_mask;
    } else {
      // D3D11 default depth-stencil state: test on, write all.
      record->depth_enable = true;
      record->depth_write = true;
    }
    depth_state->Release();
  } else {
    // D3D11 default depth-stencil state: test on, write all.
    record->depth_enable = true;
    record->depth_write = true;
  }

  ID3D11RasterizerState* rasterizer_state = nullptr;
  native->RSGetState(&rasterizer_state);
  if (rasterizer_state != nullptr) {
    reshade::api::rasterizer_desc desc = {};
    if (GetRasterizerDesc(reinterpret_cast<uintptr_t>(rasterizer_state), &desc)) {
      record->cull_mode = static_cast<uint32_t>(desc.cull_mode);
      record->front_counter_clockwise = desc.front_counter_clockwise;
    }
    rasterizer_state->Release();
  }
}

// Render state from the framework's tracked snapshot. Used only by the sampled parity check in FillCommonDrawRecord.
inline void FillRenderStateFromFramework(DrawRecord* record, reshade::api::command_list* cmd_list) {
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
}

inline void FillCommonDrawRecord(
    DrawRecord* record,
    reshade::api::command_list* cmd_list,
    reshade::api::device* device) {
  const auto shader_start = bvh::PerfNow();
  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state != nullptr) {
    record->vs_hash = renodx::utils::shader::GetCurrentVertexShaderHash(shader_state);
    record->ps_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
    record->vs_pipeline = shader_state->stage_states[renodx::utils::shader::VERTEX_INDEX].pipeline.handle;
    record->ps_pipeline = shader_state->stage_states[renodx::utils::shader::PIXEL_INDEX].pipeline.handle;
  }
  bvh::AddPerfBlockMs(bvh::kPerfBlockRecordShader, bvh::LiveElapsedMs(shader_start));

  const auto render_start = bvh::PerfNow();
  FillRenderStateFromNative(record, cmd_list);
  bvh::AddPerfBlockMs(bvh::kPerfBlockRecordRender, bvh::LiveElapsedMs(render_start));
  // Parity gate: while gathering, every 64th draw also reads the framework state and counts the render-state fields that differ.
  if (bvh::g_perf_gate.gather.load(std::memory_order_relaxed)
      && bvh::g_perf_gate.record_native_seq.fetch_add(1u, std::memory_order_relaxed) % 64u == 0u) {
    DrawRecord framework;
    FillRenderStateFromFramework(&framework, cmd_list);
    bvh::g_perf_gate.record_mismatch_rtv0.fetch_add(framework.rtv0.handle != record->rtv0.handle, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_dsv.fetch_add(framework.dsv.handle != record->dsv.handle, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_topology.fetch_add(framework.topology != record->topology, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_input_layout.fetch_add(framework.input_layout.handle != record->input_layout.handle, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_has_viewport.fetch_add(framework.has_viewport != record->has_viewport, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_viewport.fetch_add(std::memcmp(&framework.viewport, &record->viewport, sizeof(record->viewport)) != 0, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_blend_enable.fetch_add(framework.blend_enable != record->blend_enable, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_depth_enable.fetch_add(framework.depth_enable != record->depth_enable, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_depth_write.fetch_add(framework.depth_write != record->depth_write, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_cull_mode.fetch_add(framework.cull_mode != record->cull_mode, std::memory_order_relaxed);
    bvh::g_perf_gate.record_mismatch_front_counter_clockwise.fetch_add(framework.front_counter_clockwise != record->front_counter_clockwise, std::memory_order_relaxed);
  }

  const auto skin_start = bvh::PerfNow();
  record->has_skin_inputs = LayoutHasSkinInputs(record->input_layout);
  bvh::AddPerfBlockMs(bvh::kPerfBlockRecordSkin, bvh::LiveElapsedMs(skin_start));

  const auto scene_start = bvh::PerfNow();
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
  bvh::AddPerfBlockMs(bvh::kPerfBlockRecordScene, bvh::LiveElapsedMs(scene_start));
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

// Depth-target log (diagnostic only): for two frames after the world panel arms it, records each
// depth-target change seen by a draw and each depth clear, then writes depth_targets.log once.
enum class DepthRunKind : uint32_t { kOther, kCopy, kResolve, kBlur, kCaster };

// A run is a maximal sequence of draws with the same frame, kind and DSV. index is the GPU timer slot
// (-1: untimed).
struct DepthRun {
  uint32_t frame = 0u;
  int32_t index = -1;
  DepthRunKind kind = DepthRunKind::kOther;
  uint32_t draws = 0u;
  reshade::api::resource_view dsv = {};
  uint32_t first_vs = 0u;
};

struct DepthLogState {
  std::shared_mutex mutex;
  std::string text;
  uint64_t last_dsv = 0u;
  bool written = false;
  uint32_t serial = 0u;
  uint64_t shadow_dsv = 0u;
  bool shadow_dsv_sized = false;
  std::vector<DepthRun> runs;
  uint32_t run_frame = UINT32_MAX;
  uint32_t runs_in_frame = 0u;
  std::array<bvh::GpuTimer, 48> timers;
};

inline DepthLogState g_depth_log;

inline bool DepthLogActive() {
  const uint32_t start = g_state.depth_log_start_frame.load();
  if (start == UINT32_MAX) return false;
  const uint32_t frame = g_state.frame.load();
  return frame >= start && frame <= start + 1u;
}

inline std::string DescribeResource(reshade::api::device* device, reshade::api::resource resource) {
  std::ostringstream out;
  out << std::hex << "res=0x" << resource.handle << std::dec << " ";
  const auto resource_desc = device->get_resource_desc(resource);
  if (resource_desc.type == reshade::api::resource_type::buffer) {
    out << "buffer";
  } else {
    const auto& texture = resource_desc.texture;
    out << "texture " << texture.width << " " << texture.height << " " << texture.depth_or_layers << " "
        << texture.levels << " " << static_cast<uint32_t>(texture.format) << " " << texture.samples;
  }
  return out.str();
}

// Shadow, caster and grid targets all share this description (4096x4096, format 53).
inline bool IsShadowSizedTexture(reshade::api::device* device, reshade::api::resource resource) {
  if (resource.handle == 0u) return false;
  const auto desc = device->get_resource_desc(resource);
  return desc.type == reshade::api::resource_type::texture_2d && desc.texture.width == 4096u
         && desc.texture.height == 4096u && static_cast<uint32_t>(desc.texture.format) == 53u;
}

// [world-rtsun] One line per distinct (first_layer, layer_count) of the sun cascade DSVs. A layer_count other than 1 warns once.
inline void LogRtSunCascadeDsv(uint32_t first_layer, uint32_t layer_count) {
  static std::mutex mutex;
  static std::set<std::pair<uint32_t, uint32_t>> logged;
  static bool warned = false;
  std::lock_guard<std::mutex> lock(mutex);
  if (layer_count != 1u) {
    if (!warned) {
      warned = true;
      renodx::utils::log::w("[world-rtsun] cascade DSV first_layer=", first_layer, " layer_count=", layer_count, " is not 1; not skipped");
    }
    return;
  }
  if (logged.insert({first_layer, layer_count}).second) {
    renodx::utils::log::i("[world-rtsun] cascade DSV first_layer=", first_layer, " layer_count=", layer_count);
  }
}

inline std::string DescribeDepthTarget(reshade::api::device* device, reshade::api::resource_view view) {
  if (view.handle == 0u) return "dsv=0x0 res=0x0";
  const auto resource = device->get_resource_from_view(view);
  std::ostringstream out;
  out << std::hex << "dsv=0x" << view.handle << std::dec << " " << DescribeResource(device, resource);
  const auto view_desc = device->get_resource_view_desc(view);
  out << " view=" << static_cast<uint32_t>(view_desc.type) << " " << view_desc.texture.first_level << " "
      << view_desc.texture.level_count << " " << view_desc.texture.first_layer << " " << view_desc.texture.layer_count
      << " " << static_cast<uint32_t>(view_desc.format);
  return out.str();
}

inline void DepthLogAppend(const std::string& line) {
  std::lock_guard<std::shared_mutex> lock(g_depth_log.mutex);
  if (!DepthLogActive()) return;
  g_depth_log.text += line;
}

inline bool OnClearDepthStencilView(
    reshade::api::command_list* cmd_list,
    reshade::api::resource_view dsv,
    const float* depth,
    const uint8_t* stencil,
    uint32_t rect_count,
    const reshade::api::rect* rects) {
  (void)rects;
  if (DepthLogActive()) {
    std::ostringstream out;
    out << "CLEAR frame=" << g_state.frame.load() << " " << DescribeDepthTarget(cmd_list->get_device(), dsv) << " depth=";
    if (depth != nullptr) {
      out << *depth;
    } else {
      out << "none";
    }
    out << " stencil=";
    if (stencil != nullptr) {
      out << static_cast<uint32_t>(*stencil);
    } else {
      out << "none";
    }
    out << " rects=" << rect_count << "\n";
    DepthLogAppend(out.str());
  }
  return false;
}

inline bool OnCopyResourceDepthLog(reshade::api::command_list* cmd_list, reshade::api::resource source, reshade::api::resource dest) {
  if (DepthLogActive()) {
    auto* device = cmd_list->get_device();
    if (IsShadowSizedTexture(device, source) || IsShadowSizedTexture(device, dest)) {
      std::ostringstream out;
      out << "COPY frame=" << g_state.frame.load() << " src=" << DescribeResource(device, source)
          << " dst=" << DescribeResource(device, dest) << "\n";
      DepthLogAppend(out.str());
    }
  }
  return false;
}

inline bool OnCopyTextureRegionDepthLog(
    reshade::api::command_list* cmd_list,
    reshade::api::resource source,
    uint32_t source_subresource,
    const reshade::api::subresource_box*,
    reshade::api::resource dest,
    uint32_t dest_subresource,
    const reshade::api::subresource_box*,
    reshade::api::filter_mode) {
  if (DepthLogActive()) {
    auto* device = cmd_list->get_device();
    if (IsShadowSizedTexture(device, source) || IsShadowSizedTexture(device, dest)) {
      std::ostringstream out;
      out << "COPY_REGION frame=" << g_state.frame.load() << " src=" << DescribeResource(device, source)
          << " src_sub=" << source_subresource << " dst=" << DescribeResource(device, dest)
          << " dst_sub=" << dest_subresource << "\n";
      DepthLogAppend(out.str());
    }
  }
  return false;
}

struct WorldDrawCallback {
  template <typename Arguments>
  renodx::utils::command_action::CallbackResult<renodx::utils::command_action::CommandContext<Arguments>>
  Record(renodx::utils::command_action::CommandContext<Arguments>& context, uint8_t method) const {
    const bvh::PoolCpuTimer hook_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockDrawHookTotal]};
    if (context.cmd_list == nullptr) return {};
    if (!WorldDrawHookNeeded()) return {};
    auto* device = context.cmd_list->get_device();
    if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return {};

    // Depth-log runs and their GPU timers use the immediate command list only; draws on deferred lists are not logged.
    const uint64_t immediate = bvh::g_pool.immediate_cmd_list.load(std::memory_order_relaxed);
    const bool on_immediate = immediate != 0u && reinterpret_cast<uint64_t>(context.cmd_list) == immediate;
    if (on_immediate && DepthLogActive()) {
      auto* shader_state = renodx::utils::shader::GetCurrentState(context.cmd_list);
      auto* render_state = renodx::utils::state::GetCurrentState(context.cmd_list);
      const uint32_t frame = g_state.frame.load();
      const uint32_t ps_hash = shader_state != nullptr ? renodx::utils::shader::GetCurrentPixelShaderHash(shader_state) : 0u;
      const uint32_t vs_hash = shader_state != nullptr ? renodx::utils::shader::GetCurrentVertexShaderHash(shader_state) : 0u;
      const auto dsv = render_state != nullptr ? render_state->depth_stencil : reshade::api::resource_view{};
      if (IsLightingHash(ps_hash) || ps_hash == 0xF320152Cu) {
        const auto* cl_data = GetWorldCommandListData(context.cmd_list);
        if (cl_data != nullptr && cl_data->ps_srv_view[16].handle != 0u) {
          DepthLogAppend("t16 " + DescribeDepthTarget(device, cl_data->ps_srv_view[16]) + "\n");
        }
      }

      bool dsv_changed = false;
      uint32_t serial = 0u;
      {
        std::lock_guard<std::shared_mutex> lock(g_depth_log.mutex);
        if (g_depth_log.written) {
          g_depth_log.last_dsv = 0u;
          g_depth_log.written = false;
          g_depth_log.serial = 0u;
          g_depth_log.shadow_dsv = 0u;
          g_depth_log.shadow_dsv_sized = false;
          g_depth_log.run_frame = UINT32_MAX;
          g_depth_log.runs_in_frame = 0u;
        }
        serial = g_depth_log.serial++;
        dsv_changed = dsv.handle != g_depth_log.last_dsv;
        if (dsv_changed) g_depth_log.last_dsv = dsv.handle;
      }
      if (dsv_changed) {
        reshade::api::depth_stencil_desc ds_desc = {};
        bool ds_known = false;
        if (render_state != nullptr) {
          const auto ds_it = render_state->pipelines.find(reshade::api::pipeline_stage::depth_stencil);
          ds_known = ds_it != render_state->pipelines.end() && GetDepthStencilDesc(ds_it->second.handle, &ds_desc);
        }
        std::ostringstream out;
        out << "frame=" << frame << " serial=" << serial << " method=" << static_cast<uint32_t>(method) << " "
            << DescribeDepthTarget(device, dsv) << " ";
        if (ds_known) {
          out << "depth_enable=" << ds_desc.depth_enable << " depth_write=" << static_cast<uint32_t>(ds_desc.depth_write_mask)
              << " depth_func=" << static_cast<uint32_t>(ds_desc.depth_func);
        } else {
          out << "ds=unknown";
        }
        out << " ps=0x" << std::hex << ps_hash << std::dec << "\n";
        DepthLogAppend(out.str());
      }

      // Caster draws are indexed draws into a shadow-sized DSV; the DSV is evaluated only when its handle changes.
      bool dsv_shadow = false;
      if (method == 1u) {
        bool cached = false;
        {
          std::lock_guard<std::shared_mutex> lock(g_depth_log.mutex);
          cached = g_depth_log.shadow_dsv == dsv.handle;
          dsv_shadow = g_depth_log.shadow_dsv_sized;
        }
        if (!cached) {
          dsv_shadow = dsv.handle != 0u && IsShadowSizedTexture(device, device->get_resource_from_view(dsv));
          std::lock_guard<std::shared_mutex> lock(g_depth_log.mutex);
          g_depth_log.shadow_dsv = dsv.handle;
          g_depth_log.shadow_dsv_sized = dsv_shadow;
        }
      }
      DepthRunKind kind = DepthRunKind::kOther;
      if (method == 0u && (ps_hash == 0x7ee16ccbu || ps_hash == 0xdb25d13eu || ps_hash == 0x30190562u)) {
        kind = DepthRunKind::kCopy;
      } else if (method == 0u && ps_hash == 0xF320152Cu) {
        kind = DepthRunKind::kResolve;
      } else if (method == 0u && ps_hash == 0xF1575FE3u) {
        kind = DepthRunKind::kBlur;
      } else if (method == 1u && dsv_shadow) {
        kind = DepthRunKind::kCaster;
      }

      if (kind == DepthRunKind::kCopy) {
        const auto* cl_data = GetWorldCommandListData(context.cmd_list);
        std::ostringstream out;
        out << "COPYDRAW frame=" << frame << " serial=" << serial << " ps=0x" << std::hex << ps_hash
            << std::dec << " dst=" << DescribeDepthTarget(device, dsv) << " src0="
            << (cl_data != nullptr ? DescribeDepthTarget(device, cl_data->ps_srv_view[0]) : std::string("none")) << "\n";
        DepthLogAppend(out.str());
      }

      // Runs are timed on the immediate list only. Each timed run is one GPU timer slot: index is
      // (frame - start) * 24 + the run's position among timed runs in its frame, below 48.
      const uint32_t start = g_state.depth_log_start_frame.load();
      int32_t end_index = -1;
      int32_t begin_index = -1;
      {
        std::lock_guard<std::shared_mutex> lock(g_depth_log.mutex);
        auto& runs = g_depth_log.runs;
        const bool same_run = !runs.empty() && runs.back().frame == frame && runs.back().kind == kind
                              && runs.back().dsv.handle == dsv.handle;
        if (same_run) {
          runs.back().draws += 1u;
        } else {
          if (!runs.empty()) end_index = runs.back().index;
          if (frame != g_depth_log.run_frame) {
            g_depth_log.run_frame = frame;
            g_depth_log.runs_in_frame = 0u;
          }
          DepthRun run;
          run.frame = frame;
          run.kind = kind;
          run.draws = 1u;
          run.dsv = dsv;
          run.first_vs = vs_hash;
          const uint32_t index = (frame - start) * 24u + g_depth_log.runs_in_frame;
          if (kind != DepthRunKind::kOther && g_depth_log.runs_in_frame < 24u
              && index < g_depth_log.timers.size()) {
            run.index = static_cast<int32_t>(index);
            g_depth_log.runs_in_frame += 1u;
            begin_index = run.index;
          }
          runs.push_back(run);
        }
      }
      // Timer calls run outside the lock; the immediate list is the only thread that touches the timers.
      if (end_index >= 0) EndGpuTimer(context.cmd_list, &g_depth_log.timers[end_index]);
      if (begin_index >= 0) BeginGpuTimer(device, context.cmd_list, &g_depth_log.timers[begin_index]);
    } else if (on_immediate) {
      const uint32_t depth_log_start = g_state.depth_log_start_frame.load();
      if (depth_log_start != UINT32_MAX && g_state.frame.load() > depth_log_start + 3u) {
        std::vector<DepthRun> runs;
        bool pending = false;
        std::string text;
        {
          std::lock_guard<std::shared_mutex> lock(g_depth_log.mutex);
          pending = !g_depth_log.written;
          if (pending) {
            g_depth_log.written = true;
            text.swap(g_depth_log.text);
            runs.swap(g_depth_log.runs);
          }
        }
        if (pending) {
          for (const auto& run : runs) {
            if (run.index >= 0) bvh::PollGpuTimer(bvh::GpuTimerContext(context.cmd_list), &g_depth_log.timers[run.index]);
          }
          std::ostringstream out;
          out << "RUN note: immediate command list only; ms is GPU time from a run's first draw to the next run's first draw, "
                 "including non-draw work; needs Gather performance metrics; the final run of the window has no end marker "
                 "and shows n/a(unread); n/a(untimed) = not timed, n/a(unread) = timed but no result yet\n";
          for (const auto& run : runs) {
            out << "RUN frame=" << run.frame << " index=" << run.index << " kind="
                << (run.kind == DepthRunKind::kCopy       ? "copy"
                    : run.kind == DepthRunKind::kResolve  ? "resolve"
                    : run.kind == DepthRunKind::kBlur     ? "blur"
                    : run.kind == DepthRunKind::kCaster   ? "caster"
                                                          : "other")
                << " " << DescribeDepthTarget(device, run.dsv) << " draws=" << run.draws << " first_vs=0x" << std::hex
                << run.first_vs << std::dec << " ms=";
            if (run.index < 0) {
              out << "n/a(untimed)";
            } else if (g_depth_log.timers[run.index].last_ms >= 0.f) {
              out << g_depth_log.timers[run.index].last_ms;
            } else {
              out << "n/a(unread)";
            }
            out << "\n";
          }
          text += out.str();
          const auto path = bvh::PoolOutputDir() / "depth_targets.log";
          renodx::utils::path::WriteTextFile(path, text);
          renodx::utils::log::i("[world-depth] wrote ", path.string());
          for (auto& timer : g_depth_log.timers) bvh::DestroyGpuTimer(&timer);
          g_state.depth_log_start_frame.store(UINT32_MAX);
        }
      }
    }

    bool skip_cascade_draw = false;
    const int32_t rt_sun_cascades = g_state.rt_sun_cascades.load(std::memory_order_relaxed);
    if (on_immediate && rt_sun_cascades > 0) {
      ID3D11DepthStencilView* native_dsv = nullptr;
      bvh::GpuTimerContext(context.cmd_list)->OMGetRenderTargets(0u, nullptr, &native_dsv);
      if (native_dsv != nullptr) {
        const reshade::api::resource_view dsv = {reinterpret_cast<uintptr_t>(native_dsv)};
        native_dsv->Release();
        const auto resource = device->get_resource_from_view(dsv);
        if (IsShadowSizedTexture(device, resource) && device->get_resource_desc(resource).texture.depth_or_layers == 4u) {
          const auto view = device->get_resource_view_desc(dsv).texture;
          LogRtSunCascadeDsv(view.first_layer, view.layer_count);
          skip_cascade_draw = rtsun::g_rtsun_keep_cascade_maps < 0.5f && view.layer_count == 1u && view.first_layer < static_cast<uint32_t>(rt_sun_cascades);
          if (skip_cascade_draw) {
            auto& cleared_frame = g_state.rt_sun_cleared_frame[view.first_layer];
            const uint32_t frame_index = g_state.frame.load(std::memory_order_relaxed);
            if (cleared_frame.load(std::memory_order_relaxed) != frame_index) {
              cleared_frame.store(frame_index, std::memory_order_relaxed);
              const float far_depth = 1.0f;
              context.cmd_list->clear_depth_stencil_view(dsv, &far_depth, nullptr);
            }
          }
        }
      }
    }

    if (!CaptureActive()) return {.bypass = skip_cascade_draw};
    if (bvh::g_perf_gate.gather.load(std::memory_order_relaxed)) {
      bvh::g_perf_gate.hook_draws.fetch_add(1u, std::memory_order_relaxed);
      auto* shader_state = renodx::utils::shader::GetCurrentState(context.cmd_list);
      const uint64_t vs_pipeline = shader_state != nullptr ? shader_state->stage_states[renodx::utils::shader::VERTEX_INDEX].pipeline.handle : 0u;
      const uint32_t ps_hash = shader_state != nullptr ? renodx::utils::shader::GetCurrentPixelShaderHash(shader_state) : 0u;
      const auto vs_class = static_cast<contract::VsClass>(contract::LookupVertexTraits(vs_pipeline).cls);
      // M1 consumed rule (plan step 5): draws the census, RTAO discovery, lighting, pool or deform paths need. Count only in M0.
      const bool consumed = CensusEnabled() || rtao::g_rtao_discovery > 0.5f || IsLightingHash(ps_hash)
                            || vs_class == contract::VsClass::Rigid || vs_class == contract::VsClass::Wind || contract::IsDeformingClass(vs_class);
      if (!consumed) bvh::g_perf_gate.hook_draws_unconsumed.fetch_add(1u, std::memory_order_relaxed);
    }
    const auto record_start = bvh::PerfNow();

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
    const auto rest_start = bvh::PerfNow();
    record.is_candidate = IsGeometryCandidate(record);

    auto* cl_data = GetWorldCommandListData(context.cmd_list);
    FillDrawBindingMasks(&record, cl_data);

    CommitDrawRecord(record, device, cl_data);
    bvh::AddPerfBlockMs(bvh::kPerfBlockRecordRest, bvh::LiveElapsedMs(rest_start));
    bvh::AddPerfBlockMs(bvh::kPerfBlockDrawHookRecord, bvh::LiveElapsedMs(record_start));
    bvh::OnPoolScanDraw(device, context.cmd_list, record, cl_data);
    {
      const bvh::PoolCpuTimer probe_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockDeformProbeDraw]};
      bvh::OnDeformProbeDraw(device, context.cmd_list, record);
    }
    bvh::OnDeformCaptureDraw(device, context.cmd_list, record);
    return {.bypass = skip_cascade_draw};
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
    const bvh::PoolCpuTimer hook_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockDrawHookTotal]};
    if (!CaptureActive() || context.cmd_list == nullptr) return {};
    auto* device = context.cmd_list->get_device();
    if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return {};
    if (bvh::g_perf_gate.gather.load(std::memory_order_relaxed)) {
      bvh::g_perf_gate.hook_draws.fetch_add(1u, std::memory_order_relaxed);
      bvh::g_perf_gate.hook_draws_indirect.fetch_add(1u, std::memory_order_relaxed);
    }

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
    const auto record_start = bvh::PerfNow();
    FillCommonDrawRecord(&record, context.cmd_list, device);
    record.method = indexed ? 1u : 0u;
    record.frame = g_state.frame.load();
    FillDrawBindingMasks(&record, cl_data);
    bvh::AddPerfBlockMs(bvh::kPerfBlockDrawHookIndirectRecord, bvh::LiveElapsedMs(record_start));

    // The pool copies b1, the args and the instance window on the command list
    // here and reads them back later without waiting.
    bvh::OnPoolScanIndirectDraw(
        device, context.cmd_list, record, cl_data, args.buffer, args.offset, draw_count, stride);
    {
      const bvh::PoolCpuTimer probe_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockDeformProbeDraw]};
      bvh::OnDeformProbeIndirectDraw(device, context.cmd_list, record, args.buffer, args.offset, draw_count, args.stride);
    }

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
  const bvh::PoolCpuTimer resolve_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockResolveIndirect]};
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
  const bvh::PoolCpuTimer present_timer{&bvh::g_perf_gate.present_world_us};
  g_state.frame.fetch_add(1u);
  g_state.draw_counter = 0u;
  if (CaptureActive() && renodx::utils::constants::shared.data != nullptr) {
    renodx::utils::constants::shared.data->capture_constant_buffers = true;
  }
  if (queue == nullptr) return;
  ResolveIndirectDraws(queue);
  bvh::DrainPoolScan(queue->get_device(), queue);
  bvh::ExpandLiveDynamic(queue->get_device(), queue);
  bvh::RefitLiveTlasAfterDrain(queue->get_device(), queue);
  bvh::RunP2bDiagnostics(queue->get_device(), queue);
  bvh::DrainDeformProbe(queue->get_device(), queue);
}

}  // namespace falcom_world
