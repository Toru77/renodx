/*
 * Copyright (C) 2026 Carlos Lopez
 * SPDX-License-Identifier: MIT
 *
 * Beyond The Falcom Engine: DX11 scene-extraction foundation.
 *
 * Scope is intentionally narrow (milestone 1):
 *   game DX11 draw calls -> DrawRecord database -> one DrawIndexed mesh -> OBJ + JSON
 *
 * Explicit non-goals in this file: BLAS/TLAS, DXR, ray/path tracing, materials,
 * lights, denoising, temporal accumulation, whole-scene auto-capture.
 */

#pragma once

#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <include/reshade.hpp>

#include "./command_action.hpp"
#include "./cross_addon.hpp"
#include "./data.hpp"
#include "./hash.hpp"
#include "./log.hpp"
#include "./path.hpp"
#include "./resource.hpp"
#include "./shader.hpp"
#include "./state.hpp"

namespace renodx::utils::scene {

// 64 MiB per-buffer capture safety limit (not a claim about max Falcom mesh size).
static constexpr uint64_t CAPTURE_SIZE_LIMIT = 64ull * 1024ull * 1024ull;
static constexpr size_t MAX_TRACKED_VERTEX_BUFFERS = 16u;

namespace falcom {
// Future Falcom-specific world-matrix heuristics live here so per-game patterns
// stay behind clearly named functions instead of hardcoded register numbers.
// Stubbed for milestone 1: geometry capture must not wait on transforms.
struct MatrixCandidate {
  uint32_t constant_buffer_slot = 0u;
  uint32_t row_offset = 0u;
  float score = 0.f;
};

static std::vector<MatrixCandidate> FindWorldMatrixCandidates() {
  return {};
}
}  // namespace falcom

// --- Format decoding (table-driven) ----------------------------------------

enum class AttributeKind : uint8_t {
  UNKNOWN = 0,
  FLOAT32,
  FLOAT16,
  UNORM8,
};

struct FormatInfo {
  reshade::api::format format = reshade::api::format::unknown;
  uint32_t byte_size = 0u;
  uint32_t component_count = 0u;
  AttributeKind kind = AttributeKind::UNKNOWN;
};

static const FormatInfo* FindFormatInfo(reshade::api::format format) {
  static const FormatInfo kTable[] = {
      {reshade::api::format::r32g32b32_float, 12u, 3u, AttributeKind::FLOAT32},
      {reshade::api::format::r32g32b32a32_float, 16u, 4u, AttributeKind::FLOAT32},
      {reshade::api::format::r32g32_float, 8u, 2u, AttributeKind::FLOAT32},
      {reshade::api::format::r16g16_float, 4u, 2u, AttributeKind::FLOAT16},
      {reshade::api::format::r16g16b16a16_float, 8u, 4u, AttributeKind::FLOAT16},
      {reshade::api::format::r8g8b8a8_unorm, 4u, 4u, AttributeKind::UNORM8},
  };
  for (const auto& entry : kTable) {
    if (entry.format == format) return &entry;
  }
  return nullptr;
}

static std::string FormatToString(reshade::api::format format) {
  switch (format) {
    case reshade::api::format::r32g32b32_float: return "R32G32B32_FLOAT";
    case reshade::api::format::r32g32b32a32_float: return "R32G32B32A32_FLOAT";
    case reshade::api::format::r32g32_float: return "R32G32_FLOAT";
    case reshade::api::format::r16g16_float: return "R16G16_FLOAT";
    case reshade::api::format::r16g16b16a16_float: return "R16G16B16A16_FLOAT";
    case reshade::api::format::r8g8b8a8_unorm: return "R8G8B8A8_UNORM";
    default: break;
  }
  std::ostringstream s;
  s << "format(" << static_cast<uint32_t>(format) << ")";
  return s.str();
}

static float HalfToFloat(uint16_t bits) {
  const uint32_t sign = (static_cast<uint32_t>(bits) & 0x8000u) << 16u;
  uint32_t exp = (static_cast<uint32_t>(bits) >> 10u) & 0x1Fu;
  uint32_t mantissa = static_cast<uint32_t>(bits) & 0x3FFu;
  uint32_t f32 = 0u;
  if (exp == 0u) {
    if (mantissa == 0u) {
      f32 = sign;
    } else {
      // Subnormal: normalize.
      exp = 1u;
      while ((mantissa & 0x400u) == 0u) {
        mantissa <<= 1u;
        exp++;
      }
      mantissa &= 0x3FFu;
      f32 = sign | ((exp + 112u) << 23u) | (mantissa << 13u);
    }
  } else if (exp == 31u) {
    f32 = sign | 0x7F800000u | (mantissa << 13u);
  } else {
    f32 = sign | ((exp + 112u) << 23u) | (mantissa << 13u);
  }
  float out = 0.f;
  std::memcpy(&out, &f32, sizeof(out));
  return out;
}

// Decodes up to 4 components at `src` into `out` (missing components -> 0, w -> 1).
static bool DecodeAttribute(const uint8_t* src, const FormatInfo& info, float* out) {
  if (src == nullptr || out == nullptr) return false;
  out[0] = 0.f;
  out[1] = 0.f;
  out[2] = 0.f;
  out[3] = 1.f;
  switch (info.kind) {
    case AttributeKind::FLOAT32: {
      const auto* f = reinterpret_cast<const float*>(src);
      for (uint32_t i = 0; i < info.component_count && i < 4u; ++i) {
        out[i] = f[i];
      }
      return true;
    }
    case AttributeKind::FLOAT16: {
      const auto* h = reinterpret_cast<const uint16_t*>(src);
      for (uint32_t i = 0; i < info.component_count && i < 4u; ++i) {
        out[i] = HalfToFloat(h[i]);
      }
      return true;
    }
    case AttributeKind::UNORM8: {
      for (uint32_t i = 0; i < info.component_count && i < 4u; ++i) {
        out[i] = static_cast<float>(src[i]) / 255.f;
      }
      return true;
    }
    default:
      return false;
  }
}

// --- MeshLayout --------------------------------------------------------------

struct MeshLayout {
  int32_t pos_off = -1;
  int32_t norm_off = -1;
  int32_t uv_off = -1;
  int32_t tangent_off = -1;
  reshade::api::format pos_format = reshade::api::format::unknown;
  reshade::api::format norm_format = reshade::api::format::unknown;
  reshade::api::format uv_format = reshade::api::format::unknown;
  reshade::api::format tangent_format = reshade::api::format::unknown;
  uint32_t stride = 0u;
  reshade::api::primitive_topology topology = reshade::api::primitive_topology::undefined;
  uint32_t index_size = 0u;
  bool heuristic = false;
};

// --- Records -----------------------------------------------------------------

struct VertexBufferBinding {
  reshade::api::resource handle = {0u};
  uint64_t offset = 0u;
  uint32_t stride = 0u;
};

struct IndexBufferBinding {
  reshade::api::resource handle = {0u};
  uint64_t offset = 0u;
  uint32_t index_size = 0u;
};

struct DrawRecord {
  reshade::api::device* device = nullptr;
  uint32_t frame_id = 0u;
  uint32_t draw_index = 0u;
  uint8_t method = 0u;  // 0 = draw, 1 = draw_indexed
  uint32_t vertex_count = 0u;
  uint32_t index_count = 0u;
  uint32_t instance_count = 0u;
  uint32_t first_vertex = 0u;
  uint32_t first_index = 0u;
  int32_t vertex_offset = 0;
  uint32_t first_instance = 0u;
  uint32_t vs_hash = 0u;
  uint32_t ps_hash = 0u;
  reshade::api::pipeline pipeline = {0u};
  // D3D11 input-layout handle from the bound input-assembler stage
  // (CreateInputLayout/init_pipeline handle). This is the lookup key for
  // input-layout decoding; `pipeline` above is the shader pipeline.
  reshade::api::pipeline input_layout = {0u};
  reshade::api::primitive_topology topology = reshade::api::primitive_topology::undefined;
  cross_addon::vector<VertexBufferBinding> vertex_buffers;
  IndexBufferBinding index_buffer;
  bool has_index_buffer = false;
  reshade::api::resource_view render_target = {0u};
  reshade::api::resource_view depth_stencil = {0u};
  reshade::api::viewport viewport = {};
  bool has_viewport = false;
  reshade::api::pipeline_layout layout = {0u};
  cross_addon::vector<reshade::api::descriptor_table> descriptor_tables;
};

struct InputElementCopy {
  cross_addon::string semantic;
  uint32_t semantic_index = 0u;
  reshade::api::format format = reshade::api::format::unknown;
  uint32_t buffer_binding = 0u;
  uint32_t offset = 0u;
  uint32_t stride = 0u;
};

struct InputLayoutInfo {
  cross_addon::vector<InputElementCopy> elements;
  reshade::api::primitive_topology topology = reshade::api::primitive_topology::undefined;
  bool has_topology = false;
};

struct __declspec(uuid("3f1a5c2e-7b48-4d9a-8e5f-1a2b3c4d5e6f")) SharedData {
  cross_addon::parallel_node_hash_map<uint64_t, DrawRecord, std::shared_mutex> draws;
  cross_addon::parallel_node_hash_map<uint64_t, InputLayoutInfo, std::shared_mutex> input_layouts;
  // Minimal staging pool: device handle -> free staging buffers.
  std::mutex staging_mutex;
  std::unordered_map<uint64_t, std::vector<std::pair<reshade::api::resource, uint64_t>>> staging_pool;
  std::atomic_uint32_t frame_id = 0u;
  std::atomic_uint32_t draw_counter = 0u;
  std::atomic_bool enabled = true;
  std::atomic_bool capture_pending = false;
  std::atomic_uint32_t capture_draw_index = 0u;
  std::atomic_uint32_t capture_frame_id = 0u;
  std::atomic_bool capture_busy = false;
  // Diagnostics for the overlay panel.
  std::atomic_uint32_t last_frame_draws = 0u;
  std::atomic_uint32_t layout_count = 0u;
  std::atomic_uint32_t layout_arrivals = 0u;
  std::mutex status_mutex;
  cross_addon::string last_status;
};

static cross_addon::Shared<SharedData> shared;

struct __declspec(uuid("7e2b6d3f-8c59-4eab-9f60-2b3c4d5e6f7a")) SceneCommandListData {
  cross_addon::vector<VertexBufferBinding> vertex_buffers;
  IndexBufferBinding index_buffer;
  bool has_index_buffer = false;
};

static void SetStatus(const std::string& status) {
  if (shared.data == nullptr) return;
  std::lock_guard<std::mutex> lock(shared.data->status_mutex);
  shared.data->last_status.assign(status.c_str());
}

static std::string GetStatus() {
  if (shared.data == nullptr) return "scene module not attached.";
  std::lock_guard<std::mutex> lock(shared.data->status_mutex);
  if (shared.data->last_status.empty()) return "idle — no capture requested yet.";
  return std::string(shared.data->last_status.c_str());
}

static uint64_t DrawKey(uint32_t frame_id, uint32_t draw_index) {
  return (static_cast<uint64_t>(frame_id) << 32u) | draw_index;
}

// --- Binding tracking (the missing VB/IB tracker) ----------------------------

static void OnInitCommandList(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;
  SceneCommandListData* data = nullptr;
  renodx::utils::data::CreateOrGet<SceneCommandListData>(cmd_list, data);
}

static void OnDestroyCommandList(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return;
  renodx::utils::data::Delete<SceneCommandListData>(cmd_list);
}

static void OnResetCommandList(reshade::api::command_list* cmd_list) {
  auto* data = renodx::utils::data::Get<SceneCommandListData>(cmd_list);
  if (data == nullptr) return;
  data->vertex_buffers.clear();
  data->has_index_buffer = false;
  data->index_buffer = {};
}

static void OnBindVertexBuffers(
    reshade::api::command_list* cmd_list,
    uint32_t first,
    uint32_t count,
    const reshade::api::resource* buffers,
    const uint64_t* offsets,
    const uint32_t* strides) {
  if (cmd_list == nullptr || buffers == nullptr) return;
  SceneCommandListData* data = nullptr;
  renodx::utils::data::CreateOrGet<SceneCommandListData>(cmd_list, data);
  if (data == nullptr) return;
  const uint64_t total = static_cast<uint64_t>(first) + count;
  if (data->vertex_buffers.size() < total) {
    data->vertex_buffers.resize(static_cast<size_t>(total));
  }
  for (uint32_t i = 0; i < count; ++i) {
    auto& slot = data->vertex_buffers[static_cast<size_t>(first) + i];
    slot.handle = buffers[i];
    slot.offset = (offsets != nullptr) ? offsets[i] : 0u;
    slot.stride = (strides != nullptr) ? strides[i] : 0u;
  }
}

static void OnBindIndexBuffer(
    reshade::api::command_list* cmd_list,
    reshade::api::resource buffer,
    uint64_t offset,
    uint32_t index_size) {
  if (cmd_list == nullptr) return;
  SceneCommandListData* data = nullptr;
  renodx::utils::data::CreateOrGet<SceneCommandListData>(cmd_list, data);
  if (data == nullptr) return;
  data->index_buffer.handle = buffer;
  data->index_buffer.offset = offset;
  data->index_buffer.index_size = index_size;
  data->has_index_buffer = buffer.handle != 0u;
}

static void OnInitPipeline(
    reshade::api::device* device,
    reshade::api::pipeline_layout layout,
    uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects,
    reshade::api::pipeline pipeline) {
  (void)device;
  (void)layout;
  if (subobjects == nullptr || pipeline.handle == 0u) return;
  if (shared.data == nullptr) return;
  InputLayoutInfo info;
  bool found_layout = false;
  for (uint32_t i = 0; i < subobject_count; ++i) {
    const auto& subobject = subobjects[i];
    if (subobject.type == reshade::api::pipeline_subobject_type::input_layout) {
      found_layout = true;
      const auto* elements = static_cast<const reshade::api::input_element*>(subobject.data);
      for (uint32_t j = 0; j < subobject.count; ++j) {
        InputElementCopy copy;
        copy.semantic = (elements[j].semantic != nullptr) ? elements[j].semantic : "";
        copy.semantic_index = elements[j].semantic_index;
        copy.format = elements[j].format;
        copy.buffer_binding = elements[j].buffer_binding;
        copy.offset = elements[j].offset;
        copy.stride = elements[j].stride;
        info.elements.push_back(std::move(copy));
      }
    } else if (subobject.type == reshade::api::pipeline_subobject_type::primitive_topology && subobject.count > 0u) {
      const auto* topologies = static_cast<const reshade::api::primitive_topology*>(subobject.data);
      info.topology = topologies[0];
      info.has_topology = true;
    }
  }
  if (!found_layout && !info.has_topology) return;
  shared.data->input_layouts.insert_or_assign(pipeline.handle, std::move(info));
  shared.data->layout_count.store(static_cast<uint32_t>(shared.data->input_layouts.size()));
  const uint32_t arrival = shared.data->layout_arrivals.fetch_add(1u);
  if (arrival < 5u) {
    size_t element_count = 0u;
    shared.data->input_layouts.if_contains(pipeline.handle, [&](const auto& pair) {
      element_count = pair.second.elements.size();
    });
    renodx::utils::log::i(
        "beyondthefalcom::scene: input layout #", arrival,
        " handle=", renodx::utils::log::AsHex(pipeline.handle),
        " elements=", static_cast<uint64_t>(element_count), ".");
  }
}

static void OnDestroyPipeline(
    reshade::api::device* device,
    reshade::api::pipeline pipeline) {
  (void)device;
  if (shared.data == nullptr || pipeline.handle == 0u) return;
  shared.data->input_layouts.erase(pipeline.handle);
  shared.data->layout_count.store(static_cast<uint32_t>(shared.data->input_layouts.size()));
}

// --- Draw recording ------------------------------------------------------------

static void FillCommonRecord(
    DrawRecord& record,
    reshade::api::command_list* cmd_list,
    reshade::api::device* device) {
  record.device = device;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state != nullptr) {
    record.vs_hash = renodx::utils::shader::GetCurrentVertexShaderHash(shader_state);
    record.ps_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
    record.pipeline = shader_state->last_pipeline;
    if (record.pipeline.handle == 0u) {
      record.pipeline = shader_state->stage_states[renodx::utils::shader::VERTEX_INDEX].pipeline;
    }
  }

  auto* render_state = renodx::utils::state::GetCurrentState(cmd_list);
  if (render_state != nullptr) {
    if (!render_state->render_targets.empty()) {
      record.render_target = render_state->render_targets[0];
    }
    record.depth_stencil = render_state->depth_stencil;
    record.topology = render_state->primitive_topology;
    // On D3D11 the input layout is bound per-stage (IASetInputLayout ->
    // bind_pipeline(input_assembler)), not as part of a draw-time pipeline.
    // Its handle matches the CreateInputLayout init_pipeline handle.
    const auto ia_it = render_state->pipelines.find(reshade::api::pipeline_stage::input_assembler);
    if (ia_it != render_state->pipelines.end()) {
      record.input_layout = ia_it->second;
    }
    if (!render_state->viewports.empty()) {
      record.viewport = render_state->viewports[0];
      record.has_viewport = true;
    }
    record.layout = render_state->graphics_pipeline_layout;
    record.descriptor_tables.assign(
        render_state->graphics_descriptor_tables.begin(),
        render_state->graphics_descriptor_tables.end());
  }

  auto* scene_state = renodx::utils::data::Get<SceneCommandListData>(cmd_list);
  if (scene_state != nullptr) {
    record.vertex_buffers.assign(
        scene_state->vertex_buffers.begin(),
        scene_state->vertex_buffers.end());
    record.index_buffer = scene_state->index_buffer;
    record.has_index_buffer = scene_state->has_index_buffer;
  }
}

struct SceneDrawCallback {
  static void MaybeLogSample(uint32_t frame, uint32_t index, const DrawRecord& record) {
    if (index >= 2u || (frame % 300u) != 0u) return;
    renodx::utils::log::i(
        "beyondthefalcom::scene: sample frame=", frame,
        " draw=", index,
        " method=", (record.method == 1u ? "indexed" : "draw"),
        " ia=", renodx::utils::log::AsHex(record.input_layout.handle),
        " vs=", renodx::utils::log::AsHex(record.vs_hash),
        " ps=", renodx::utils::log::AsHex(record.ps_hash),
        " vbs=", static_cast<uint64_t>(record.vertex_buffers.size()),
        " ib=", (record.has_index_buffer ? "yes" : "no"),
        " topo=", static_cast<uint64_t>(static_cast<uint32_t>(record.topology)),
        " layouts=", static_cast<uint64_t>((shared.data != nullptr) ? shared.data->layout_count.load() : 0u), ".");
  }

  renodx::utils::command_action::CallbackResult<renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawArguments>>
  operator()(renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawArguments>& context) const {
    using Result = renodx::utils::command_action::CallbackResult<renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawArguments>>;
    if (shared.data == nullptr || !shared.data->enabled.load()) return {};
    if (context.cmd_list == nullptr) return {};
    auto* device = context.cmd_list->get_device();
    if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return {};

    DrawRecord record;
    record.method = 0u;
    record.vertex_count = context.arguments.vertex_count;
    record.instance_count = context.arguments.instance_count;
    record.first_vertex = context.arguments.first_vertex;
    record.first_instance = context.arguments.first_instance;
    FillCommonRecord(record, context.cmd_list, device);

    const uint32_t frame = shared.data->frame_id.load();
    const uint32_t index = shared.data->draw_counter.fetch_add(1u);
    record.frame_id = frame;
    record.draw_index = index;
    MaybeLogSample(frame, index, record);
    shared.data->draws.insert_or_assign(DrawKey(frame, index), std::move(record));
    return {};
  }

  renodx::utils::command_action::CallbackResult<renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawIndexedArguments>>
  operator()(renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawIndexedArguments>& context) const {
    using Result = renodx::utils::command_action::CallbackResult<renodx::utils::command_action::CommandContext<renodx::utils::command_action::DrawIndexedArguments>>;
    if (shared.data == nullptr || !shared.data->enabled.load()) return {};
    if (context.cmd_list == nullptr) return {};
    auto* device = context.cmd_list->get_device();
    if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return {};

    DrawRecord record;
    record.method = 1u;
    record.index_count = context.arguments.index_count;
    record.instance_count = context.arguments.instance_count;
    record.first_index = context.arguments.first_index;
    record.vertex_offset = context.arguments.vertex_offset;
    record.first_instance = context.arguments.first_instance;
    FillCommonRecord(record, context.cmd_list, device);

    const uint32_t frame = shared.data->frame_id.load();
    const uint32_t index = shared.data->draw_counter.fetch_add(1u);
    record.frame_id = frame;
    record.draw_index = index;
    MaybeLogSample(frame, index, record);
    shared.data->draws.insert_or_assign(DrawKey(frame, index), std::move(record));
    return {};
  }
};

// --- Staging pool -----------------------------------------------------------------

static bool AcquireStaging(
    reshade::api::device* device,
    uint64_t size,
    reshade::api::resource* out_resource) {
  if (device == nullptr || out_resource == nullptr || shared.data == nullptr) return false;
  {
    std::lock_guard<std::mutex> lock(shared.data->staging_mutex);
    auto jt = shared.data->staging_pool.find(reinterpret_cast<uint64_t>(device));
    if (jt != shared.data->staging_pool.end()) {
      for (auto entry = jt->second.begin(); entry != jt->second.end(); ++entry) {
        if (entry->second >= size) {
          *out_resource = entry->first;
          jt->second.erase(entry);
          return true;
        }
      }
    }
  }
  reshade::api::resource_desc desc(size, reshade::api::memory_heap::gpu_to_cpu, reshade::api::resource_usage::copy_dest);
  reshade::api::resource staging = {0u};
  if (!device->create_resource(desc, nullptr, reshade::api::resource_usage::copy_dest, &staging)) {
    return false;
  }
  *out_resource = staging;
  return true;
}

static void ReleaseStaging(
    reshade::api::device* device,
    reshade::api::resource staging,
    uint64_t size) {
  if (device == nullptr || staging.handle == 0u || shared.data == nullptr) return;
  std::lock_guard<std::mutex> lock(shared.data->staging_mutex);
  auto& entries = shared.data->staging_pool[reinterpret_cast<uint64_t>(device)];
  if (entries.size() >= 4u) {
    device->destroy_resource(staging);
    return;
  }
  entries.emplace_back(staging, size);
}

static void PurgeStagingForDevice(reshade::api::device* device) {
  if (device == nullptr || shared.data == nullptr) return;
  std::lock_guard<std::mutex> lock(shared.data->staging_mutex);
  auto it = shared.data->staging_pool.find(reinterpret_cast<uint64_t>(device));
  if (it == shared.data->staging_pool.end()) return;
  for (const auto& [staging, size] : it->second) {
    (void)size;
    device->destroy_resource(staging);
  }
  shared.data->staging_pool.erase(it);
}

// Copies `size` bytes from `source` (offset `source_offset`) into a CPU-visible
// staging buffer and returns the bytes. Staging resource is pooled.
static bool ReadbackBuffer(
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    reshade::api::resource source,
    uint64_t source_offset,
    uint64_t size,
    std::vector<uint8_t>* out_bytes) {
  if (device == nullptr || queue == nullptr || source.handle == 0u || out_bytes == nullptr) return false;
  if (size == 0u || size > CAPTURE_SIZE_LIMIT) return false;

  reshade::api::resource staging = {0u};
  if (!AcquireStaging(device, size, &staging)) {
    renodx::utils::log::e("beyondthefalcom::scene: failed to create staging buffer (", size, " bytes).");
    return false;
  }

  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) {
    ReleaseStaging(device, staging, size);
    return false;
  }
  cmd_list->copy_buffer_region(source, source_offset, staging, 0u, size);
  queue->flush_immediate_command_list();
  queue->wait_idle();

  void* mapped = nullptr;
  if (!device->map_buffer_region(staging, 0u, size, reshade::api::map_access::read_only, &mapped) || mapped == nullptr) {
    device->unmap_buffer_region(staging);
    ReleaseStaging(device, staging, size);
    renodx::utils::log::e("beyondthefalcom::scene: failed to map staging buffer.");
    return false;
  }
  out_bytes->assign(static_cast<const uint8_t*>(mapped), static_cast<const uint8_t*>(mapped) + static_cast<size_t>(size));
  device->unmap_buffer_region(staging);
  ReleaseStaging(device, staging, size);
  return true;
}

// --- MeshLayout ---------------------------------------------------------------------

static bool BuildMeshLayout(const InputLayoutInfo& layout_info, uint32_t stride, uint32_t index_size, MeshLayout* out) {
  if (out == nullptr) return false;
  MeshLayout layout;
  layout.stride = stride;
  layout.index_size = index_size;
  if (layout_info.has_topology) {
    layout.topology = layout_info.topology;
  }

  for (const auto& element : layout_info.elements) {
    if (element.buffer_binding != 0u) continue;  // single-stream (slot 0) for milestone 1
    std::string semantic = element.semantic.c_str();
    for (auto& c : semantic) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
    const FormatInfo* info = FindFormatInfo(element.format);
    if (info == nullptr) continue;  // unsupported format: skip element, do not fail yet
    const bool is_position = (semantic == "POSITION" || semantic == "SV_POSITION" || semantic == "POSITIONT");
    const bool is_normal = (semantic == "NORMAL");
    const bool is_tangent = (semantic == "TANGENT" || semantic == "BINORMAL");
    const bool is_uv = (semantic == "TEXCOORD");
    if (is_position && layout.pos_off < 0) {
      layout.pos_off = static_cast<int32_t>(element.offset);
      layout.pos_format = element.format;
    } else if (is_normal && layout.norm_off < 0) {
      layout.norm_off = static_cast<int32_t>(element.offset);
      layout.norm_format = element.format;
    } else if (is_tangent && layout.tangent_off < 0) {
      layout.tangent_off = static_cast<int32_t>(element.offset);
      layout.tangent_format = element.format;
    } else if (is_uv && layout.uv_off < 0 && element.semantic_index == 0u) {
      // First TEXCOORD set feeds the initial OBJ export.
      layout.uv_off = static_cast<int32_t>(element.offset);
      layout.uv_format = element.format;
    }
  }

  if (layout.pos_off < 0) {
    // Controlled fallback: slot-0 stride-sized float3 at offset 0, never silent.
    layout.pos_off = 0;
    layout.pos_format = reshade::api::format::r32g32b32_float;
    layout.heuristic = true;
    renodx::utils::log::w("beyondthefalcom::scene: no POSITION semantic found; using heuristic float3 @ 0.");
  }
  *out = layout;
  return true;
}

// --- Capture --------------------------------------------------------------------------

struct CapturedMesh {
  MeshLayout layout;
  std::vector<std::array<float, 3>> positions;
  std::vector<std::array<float, 3>> normals;
  std::vector<std::array<float, 2>> uvs;
  std::vector<std::array<uint32_t, 3>> triangles;  // remapped vertex indices
  std::array<float, 3> bbox_min = {0.f, 0.f, 0.f};
  std::array<float, 3> bbox_max = {0.f, 0.f, 0.f};
  uint32_t mesh_hash = 0u;
  bool has_normals = false;
  bool has_uvs = false;
  uint64_t vertex_buffer_size = 0u;
  uint64_t index_buffer_size = 0u;
};

static bool CaptureDrawIndexed(
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    const DrawRecord& record,
    CapturedMesh* out_mesh,
    std::string* out_error) {
  const auto fail = [&](const std::string& message) {
    if (out_error != nullptr) *out_error = message;
    return false;
  };
  if (device == nullptr || queue == nullptr || out_mesh == nullptr) return fail("null device/queue/output.");
  if (record.method != 1u) return fail("only DrawIndexed capture is supported in milestone 1.");
  if (!record.has_index_buffer || record.index_buffer.handle.handle == 0u) return fail("draw has no index buffer.");
  if (record.vertex_buffers.empty() || record.vertex_buffers[0].handle.handle == 0u) return fail("draw has no vertex buffer.");
  if (record.index_count == 0u || (record.index_count % 3u) != 0u) return fail("index count is zero or not a triangle multiple.");
  if (record.index_buffer.index_size != 2u && record.index_buffer.index_size != 4u) return fail("unsupported index size.");

  InputLayoutInfo layout_info;
  bool found_layout = false;
  if (shared.data != nullptr && record.input_layout.handle != 0u) {
    shared.data->input_layouts.if_contains(record.input_layout.handle, [&](const auto& pair) {
      layout_info = pair.second;
      found_layout = true;
    });
  }
  if (!found_layout) {
    std::ostringstream missing;
    missing << "no input-layout information (ia handle=" << renodx::utils::log::AsHex(record.input_layout.handle)
            << ", shader pipeline=" << renodx::utils::log::AsHex(record.pipeline.handle)
            << ", vs=" << renodx::utils::log::AsHex(record.vs_hash)
            << ", layouts tracked=";
    missing << ((shared.data != nullptr) ? shared.data->layout_count.load() : 0u) << ").";
    return fail(missing.str());
  }

  const uint32_t stride = (record.vertex_buffers[0].stride != 0u) ? record.vertex_buffers[0].stride : 0u;
  if (stride == 0u) return fail("vertex stride is zero.");

  MeshLayout mesh_layout;
  if (!BuildMeshLayout(layout_info, stride, record.index_buffer.index_size, &mesh_layout)) {
    return fail("failed to decode input layout.");
  }
  if (mesh_layout.topology != reshade::api::primitive_topology::undefined
      && mesh_layout.topology != reshade::api::primitive_topology::triangle_list) {
    return fail("only triangle-list topology is supported in milestone 1.");
  }

  const auto vb_desc = device->get_resource_desc(record.vertex_buffers[0].handle);
  const auto ib_desc = device->get_resource_desc(record.index_buffer.handle);
  if (vb_desc.type != reshade::api::resource_type::buffer) return fail("vertex resource is not a buffer.");
  if (ib_desc.type != reshade::api::resource_type::buffer) return fail("index resource is not a buffer.");
  if (vb_desc.buffer.size == 0u || vb_desc.buffer.size > CAPTURE_SIZE_LIMIT) return fail("vertex buffer size out of range.");
  if (ib_desc.buffer.size == 0u || ib_desc.buffer.size > CAPTURE_SIZE_LIMIT) return fail("index buffer size out of range.");

  const uint64_t ib_begin = record.index_buffer.offset + static_cast<uint64_t>(record.first_index) * record.index_buffer.index_size;
  const uint64_t ib_end = ib_begin + static_cast<uint64_t>(record.index_count) * record.index_buffer.index_size;
  if (ib_end > ib_desc.buffer.size) return fail("draw index range exceeds index buffer.");

  std::vector<uint8_t> vb_bytes;
  std::vector<uint8_t> ib_bytes;
  if (!ReadbackBuffer(device, queue, record.vertex_buffers[0].handle, 0u, vb_desc.buffer.size, &vb_bytes)) {
    return fail("vertex buffer readback failed.");
  }
  if (!ReadbackBuffer(device, queue, record.index_buffer.handle, 0u, ib_desc.buffer.size, &ib_bytes)) {
    return fail("index buffer readback failed.");
  }

  // Decode indices (draw subrange only) and apply base-vertex offset.
  std::vector<uint32_t> indices;
  indices.reserve(record.index_count);
  const uint8_t* ib_ptr = ib_bytes.data() + static_cast<size_t>(ib_begin);
  const uint64_t vb_capacity = vb_bytes.size() / stride;
  for (uint32_t i = 0; i < record.index_count; ++i) {
    uint32_t raw = 0u;
    if (record.index_buffer.index_size == 2u) {
      uint16_t v = 0u;
      std::memcpy(&v, ib_ptr + static_cast<size_t>(i) * 2u, 2u);
      raw = v;
    } else {
      std::memcpy(&raw, ib_ptr + static_cast<size_t>(i) * 4u, 4u);
    }
    const int64_t absolute = static_cast<int64_t>(raw) + record.vertex_offset;
    if (absolute < 0 || static_cast<uint64_t>(absolute) >= vb_capacity) return fail("index references a vertex outside the buffer.");
    const uint64_t byte_offset = record.vertex_buffers[0].offset + static_cast<uint64_t>(absolute) * stride;
    if (byte_offset + stride > vb_bytes.size()) return fail("vertex byte range exceeds buffer.");
    indices.push_back(static_cast<uint32_t>(absolute));
  }

  const FormatInfo* pos_info = FindFormatInfo(mesh_layout.pos_format);
  if (pos_info == nullptr) return fail(std::string("unsupported POSITION format: ") + FormatToString(mesh_layout.pos_format));
  const FormatInfo* norm_info = (mesh_layout.norm_off >= 0) ? FindFormatInfo(mesh_layout.norm_format) : nullptr;
  const FormatInfo* uv_info = (mesh_layout.uv_off >= 0) ? FindFormatInfo(mesh_layout.uv_format) : nullptr;

  CapturedMesh mesh;
  mesh.layout = mesh_layout;
  mesh.vertex_buffer_size = vb_desc.buffer.size;
  mesh.index_buffer_size = ib_desc.buffer.size;

  std::unordered_map<uint32_t, uint32_t> remap;
  remap.reserve(indices.size());
  auto get_or_decode = [&](uint32_t absolute, uint32_t* out_new_index) {
    auto it = remap.find(absolute);
    if (it != remap.end()) {
      *out_new_index = it->second;
      return true;
    }
    const uint8_t* vtx = vb_bytes.data() + record.vertex_buffers[0].offset + static_cast<size_t>(absolute) * stride;
    float p[4] = {};
    if (!DecodeAttribute(vtx + mesh_layout.pos_off, *pos_info, p)) return false;
    if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) return false;
    const uint32_t fresh = static_cast<uint32_t>(mesh.positions.size());
    mesh.positions.push_back({p[0], p[1], p[2]});
    if (norm_info != nullptr) {
      float n[4] = {};
      if (DecodeAttribute(vtx + mesh_layout.norm_off, *norm_info, n)
          && std::isfinite(n[0]) && std::isfinite(n[1]) && std::isfinite(n[2])) {
        mesh.normals.push_back({n[0], n[1], n[2]});
      } else {
        mesh.normals.push_back({0.f, 0.f, 0.f});
      }
    }
    if (uv_info != nullptr) {
      float t[4] = {};
      if (DecodeAttribute(vtx + mesh_layout.uv_off, *uv_info, t)
          && std::isfinite(t[0]) && std::isfinite(t[1])) {
        mesh.uvs.push_back({t[0], t[1]});
      } else {
        mesh.uvs.push_back({0.f, 0.f});
      }
    }
    remap.emplace(absolute, fresh);
    *out_new_index = fresh;
    return true;
  };

  mesh.triangles.reserve(record.index_count / 3u);
  for (uint32_t i = 0; i < record.index_count; i += 3u) {
    std::array<uint32_t, 3> tri = {0u, 0u, 0u};
    for (int k = 0; k < 3; ++k) {
      if (!get_or_decode(indices[static_cast<size_t>(i) + k], &tri[k])) return fail("vertex attribute decode failed.");
    }
    mesh.triangles.push_back(tri);
  }
  mesh.has_normals = norm_info != nullptr;
  mesh.has_uvs = uv_info != nullptr;

  if (mesh.positions.empty()) return fail("no vertices decoded.");
  std::array<float, 3> bb_min = mesh.positions[0];
  std::array<float, 3> bb_max = mesh.positions[0];
  for (const auto& p : mesh.positions) {
    for (int k = 0; k < 3; ++k) {
      bb_min[k] = (std::min)(bb_min[k], p[k]);
      bb_max[k] = (std::max)(bb_max[k], p[k]);
    }
  }
  const bool degenerate = (bb_min[0] == bb_max[0]) && (bb_min[1] == bb_max[1]) && (bb_min[2] == bb_max[2]);
  if (degenerate) return fail("decoded bounding box is degenerate.");
  mesh.bbox_min = bb_min;
  mesh.bbox_max = bb_max;

  // Stable identity: layout + referenced vertex/index bytes.
  uint32_t crc = 0xFFFFFFFFu;
  const auto feed = [&](const void* ptr, size_t size) {
    crc = renodx::utils::hash::UpdateCRC32(crc, static_cast<const uint8_t*>(ptr), size);
  };
  int32_t layout_words[4] = {mesh_layout.pos_off, mesh_layout.norm_off, mesh_layout.uv_off, mesh_layout.tangent_off};
  feed(layout_words, sizeof(layout_words));
  feed(&mesh_layout.stride, sizeof(mesh_layout.stride));
  feed(ib_ptr, static_cast<size_t>(record.index_count) * record.index_buffer.index_size);
  for (const auto& [absolute, fresh] : remap) {
    (void)fresh;
    const uint8_t* vtx = vb_bytes.data() + record.vertex_buffers[0].offset + static_cast<size_t>(absolute) * stride;
    feed(vtx, stride);
  }
  mesh.mesh_hash = renodx::utils::hash::FinalizeCRC32(crc);

  *out_mesh = std::move(mesh);
  return true;
}

// --- Output ---------------------------------------------------------------------------

static std::filesystem::path OutputDir() {
  return renodx::utils::path::GetOutputPath() / "beyondthefalcom";
}

static bool WriteObj(const std::filesystem::path& path, const CapturedMesh& mesh) {
  std::ostringstream out;
  out << "# Beyond The Falcom Engine mesh capture\n";
  out.precision(9);
  for (const auto& p : mesh.positions) {
    out << "v " << p[0] << " " << p[1] << " " << p[2] << "\n";
  }
  if (mesh.has_uvs) {
    for (const auto& t : mesh.uvs) {
      out << "vt " << t[0] << " " << t[1] << "\n";
    }
  }
  if (mesh.has_normals) {
    for (const auto& n : mesh.normals) {
      out << "vn " << n[0] << " " << n[1] << " " << n[2] << "\n";
    }
  }
  for (const auto& tri : mesh.triangles) {
    out << "f";
    for (int k = 0; k < 3; ++k) {
      const uint32_t v = tri[k] + 1u;
      out << " " << v;
      if (mesh.has_uvs || mesh.has_normals) {
        out << "/";
        out << (mesh.has_uvs ? std::to_string(v) : "");
        if (mesh.has_normals) {
          out << "/" << v;
        }
      }
    }
    out << "\n";
  }
  std::string text = out.str();
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  std::vector<uint8_t> bytes(text.begin(), text.end());
  renodx::utils::path::WriteBinaryFile(path, bytes);
  return true;
}

static std::string BuildJson(
    const DrawRecord& record,
    const CapturedMesh& mesh,
    const InputLayoutInfo& layout_info) {
  std::ostringstream json;
  json.precision(9);
  json << "{\n";
  json << "  \"frame\": " << record.frame_id << ",\n";
  json << "  \"draw_index\": " << record.draw_index << ",\n";
  json << "  \"draw_method\": \"" << (record.method == 1u ? "draw_indexed" : "draw") << "\",\n";
  json << "  \"index_count\": " << record.index_count << ",\n";
  json << "  \"instance_count\": " << record.instance_count << ",\n";
  json << "  \"first_index\": " << record.first_index << ",\n";
  json << "  \"vertex_offset\": " << record.vertex_offset << ",\n";
  json << "  \"first_instance\": " << record.first_instance << ",\n";
  json << "  \"vs_hash\": \"" << renodx::utils::log::AsHex(record.vs_hash) << "\",\n";
  json << "  \"ps_hash\": \"" << renodx::utils::log::AsHex(record.ps_hash) << "\",\n";
  json << "  \"stride\": " << mesh.layout.stride << ",\n";
  json << "  \"index_size\": " << mesh.layout.index_size << ",\n";
  json << "  \"topology\": " << static_cast<uint32_t>(mesh.layout.topology) << ",\n";
  json << "  \"pos_off\": " << mesh.layout.pos_off << ",\n";
  json << "  \"norm_off\": " << mesh.layout.norm_off << ",\n";
  json << "  \"uv_off\": " << mesh.layout.uv_off << ",\n";
  json << "  \"tangent_off\": " << mesh.layout.tangent_off << ",\n";
  json << "  \"pos_format\": \"" << FormatToString(mesh.layout.pos_format) << "\",\n";
  json << "  \"heuristic_layout\": " << (mesh.layout.heuristic ? "true" : "false") << ",\n";
  json << "  \"input_elements\": [\n";
  for (size_t i = 0; i < layout_info.elements.size(); ++i) {
    const auto& e = layout_info.elements[i];
    json << "    {\"semantic\": \"" << e.semantic.c_str() << "\", \"index\": " << e.semantic_index
         << ", \"format\": \"" << FormatToString(e.format) << "\", \"binding\": " << e.buffer_binding
         << ", \"offset\": " << e.offset << ", \"stride\": " << e.stride << "}"
         << (i + 1u < layout_info.elements.size() ? "," : "") << "\n";
  }
  json << "  ],\n";
  json << "  \"mesh_hash\": \"" << renodx::utils::log::AsHex(mesh.mesh_hash) << "\",\n";
  json << "  \"vertices\": " << mesh.positions.size() << ",\n";
  json << "  \"triangles\": " << mesh.triangles.size() << ",\n";
  json << "  \"vertex_buffer_size\": " << mesh.vertex_buffer_size << ",\n";
  json << "  \"index_buffer_size\": " << mesh.index_buffer_size << ",\n";
  json << "  \"vb_handle\": \"" << renodx::utils::log::AsHex(record.vertex_buffers.empty() ? 0u : record.vertex_buffers[0].handle.handle) << "\",\n";
  json << "  \"ib_handle\": \"" << renodx::utils::log::AsHex(record.index_buffer.handle.handle) << "\",\n";
  json << "  \"input_layout_handle\": \"" << renodx::utils::log::AsHex(record.input_layout.handle) << "\",\n";
  json << "  \"pipeline_handle\": \"" << renodx::utils::log::AsHex(record.pipeline.handle) << "\",\n";
  json << "  \"world_transform\": \"missing\",\n";
  json << "  \"bbox_min\": [" << mesh.bbox_min[0] << ", " << mesh.bbox_min[1] << ", " << mesh.bbox_min[2] << "],\n";
  json << "  \"bbox_max\": [" << mesh.bbox_max[0] << ", " << mesh.bbox_max[1] << ", " << mesh.bbox_max[2] << "]\n";
  json << "}\n";
  return json.str();
}

static void LogCaptureBlock(uint32_t draw_index, const DrawRecord& record, const CapturedMesh& mesh) {
  const auto& layout = mesh.layout;
  renodx::utils::log::i(
      "beyondthefalcom::scene:\nDraw #", draw_index,
      "\nMeshHash: ", renodx::utils::log::AsHex(mesh.mesh_hash),
      "\nVertices: ", static_cast<uint64_t>(mesh.positions.size()),
      "\nTriangles: ", static_cast<uint64_t>(mesh.triangles.size()),
      "\nStride: ", static_cast<uint64_t>(layout.stride),
      "\nIndex: ", (layout.index_size == 2u ? "16-bit" : (layout.index_size == 4u ? "32-bit" : "unknown")),
      "\nPosition: float3 @ offset ", layout.pos_off,
      "\nNormal: ", (layout.norm_off >= 0 ? ("float3 @ offset " + std::to_string(layout.norm_off)) : std::string("absent")),
      "\nUV: ", (layout.uv_off >= 0 ? ("float2 @ offset " + std::to_string(layout.uv_off)) : std::string("absent")),
      "\nTangent: ", (layout.tangent_off >= 0 ? ("float4 @ offset " + std::to_string(layout.tangent_off)) : std::string("absent")),
      "\nWorldTransform: missing",
      "\nBBox:\n  min = (", mesh.bbox_min[0], ", ", mesh.bbox_min[1], ", ", mesh.bbox_min[2], ")",
      "\n  max = (", mesh.bbox_max[0], ", ", mesh.bbox_max[1], ", ", mesh.bbox_max[2], ")");
  (void)record;
}

// --- Capture orchestration ---------------------------------------------------------------

static bool RunPendingCapture(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (shared.data == nullptr || device == nullptr || queue == nullptr) return false;
  if (!shared.data->capture_pending.load() || shared.data->capture_busy.load()) return false;
  shared.data->capture_busy.store(true);

  const uint32_t draw_index = shared.data->capture_draw_index.load();
  const uint32_t frame_id = shared.data->frame_id.load();
  shared.data->capture_pending.store(false);

  bool found = false;
  bool ok = false;
  shared.data->draws.if_contains(DrawKey(frame_id, draw_index), [&](const auto& pair) {
    found = true;
    const DrawRecord& record = pair.second;
    if (device->get_api() != reshade::api::device_api::d3d11) {
      const std::string message = "capture device is not D3D11.";
      renodx::utils::log::e("beyondthefalcom::scene: Draw #", draw_index, " capture failed: ", message.c_str());
      SetStatus("Draw #" + std::to_string(draw_index) + " failed: " + message);
      return;
    }
    CapturedMesh mesh;
    std::string error;
    if (!CaptureDrawIndexed(device, queue, record, &mesh, &error)) {
      renodx::utils::log::e("beyondthefalcom::scene: Draw #", draw_index, " capture failed: ", error.c_str());
      SetStatus("Draw #" + std::to_string(draw_index) + " failed: " + error);
      return;
    }
    InputLayoutInfo layout_info;
    shared.data->input_layouts.if_contains(record.input_layout.handle, [&](const auto& layout_pair) {
      layout_info = layout_pair.second;
    });

    const auto dir = OutputDir();
    std::ostringstream name;
    name << "draw_" << draw_index;
    WriteObj(dir / (name.str() + ".obj"), mesh);
    const std::string json_text = BuildJson(record, mesh, layout_info);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    std::vector<uint8_t> json_bytes(json_text.begin(), json_text.end());
    renodx::utils::path::WriteBinaryFile(dir / (name.str() + ".json"), json_bytes);
    LogCaptureBlock(draw_index, record, mesh);
    SetStatus("Draw #" + std::to_string(draw_index) + " captured: " + std::to_string(mesh.positions.size())
              + " verts, " + std::to_string(mesh.triangles.size()) + " tris -> " + dir.string());
    ok = true;
  });

  if (!found) {
    std::ostringstream message;
    message << "no DrawRecord for Draw #" << draw_index << " in frame " << frame_id << " (frame has "
            << shared.data->draw_counter.load() << " draws).";
    renodx::utils::log::e("beyondthefalcom::scene: ", message.str().c_str());
    SetStatus(message.str());
  }
  shared.data->capture_busy.store(false);
  return ok;
}

static void RequestCapture(uint32_t draw_index) {
  if (shared.data == nullptr) return;
  shared.data->capture_draw_index.store(draw_index);
  shared.data->capture_pending.store(true);
  // capture_frame_id is informational; capture always targets the latest completed frame.
  shared.data->capture_frame_id.store(shared.data->frame_id.load());
  renodx::utils::log::i("beyondthefalcom::scene: capture queued for Draw #", draw_index, ".");
}

static void OnPresent(
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
  if (shared.data == nullptr) return;
  if (queue != nullptr && swapchain != nullptr) {
    RunPendingCapture(swapchain->get_device(), queue);
  }
  // Prune previous frames so the record map stays at ~1 frame.
  const uint32_t current = shared.data->frame_id.load();
  shared.data->last_frame_draws.store(shared.data->draw_counter.load());
  std::vector<uint64_t> stale;
  shared.data->draws.for_each([&](const auto& pair) {
    const uint32_t frame = static_cast<uint32_t>(pair.first >> 32u);
    if (frame < current) stale.push_back(pair.first);
  });
  for (uint64_t key : stale) {
    shared.data->draws.erase(key);
  }
  shared.data->draw_counter.store(0u);
  shared.data->frame_id.store(current + 1u);
}

static void OnDestroyDevice(reshade::api::device* device) {
  PurgeStagingForDevice(device);
}

// --- Module -------------------------------------------------------------------------------

static void Use(DWORD fdw_reason) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      shared.RegisterModule();
      // Register before command_action::Use: Use() snapshots registrations to
      // decide which ReShade draw events to activate.
      renodx::utils::command_action::Register(SceneDrawCallback{}, {
                                                                   .shader_hash = 0u,
                                                                   .command_types = renodx::utils::command_action::COMMAND_TYPE_DIRECT_DRAW,
                                                               });
      renodx::utils::command_action::Use(DLL_PROCESS_ATTACH);
      renodx::utils::shader::Use(DLL_PROCESS_ATTACH);
      renodx::utils::state::Use(DLL_PROCESS_ATTACH);
      renodx::utils::resource::Use(DLL_PROCESS_ATTACH);
      shared.RegisterEvent<reshade::addon_event::init_command_list>(OnInitCommandList);
      shared.RegisterEvent<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      shared.RegisterEvent<reshade::addon_event::reset_command_list>(OnResetCommandList);
      shared.RegisterEvent<reshade::addon_event::bind_vertex_buffers>(OnBindVertexBuffers);
      shared.RegisterEvent<reshade::addon_event::bind_index_buffer>(OnBindIndexBuffer);
      shared.RegisterEvent<reshade::addon_event::init_pipeline>(OnInitPipeline);
      shared.RegisterEvent<reshade::addon_event::destroy_pipeline>(OnDestroyPipeline);
      shared.RegisterEvent<reshade::addon_event::present>(OnPresent);
      shared.RegisterEvent<reshade::addon_event::destroy_device>(OnDestroyDevice);
      {
        std::lock_guard<std::mutex> lock(shared.data->status_mutex);
        if (shared.data->last_status.empty()) {
          shared.data->last_status.assign("idle — no capture requested yet.");
        }
      }
      renodx::utils::log::i("beyondthefalcom::scene attached (DX11 scene extraction, milestone 1).");
      break;
    case DLL_PROCESS_DETACH:
      shared.UnregisterEvent<reshade::addon_event::init_command_list>(OnInitCommandList);
      shared.UnregisterEvent<reshade::addon_event::destroy_command_list>(OnDestroyCommandList);
      shared.UnregisterEvent<reshade::addon_event::reset_command_list>(OnResetCommandList);
      shared.UnregisterEvent<reshade::addon_event::bind_vertex_buffers>(OnBindVertexBuffers);
      shared.UnregisterEvent<reshade::addon_event::bind_index_buffer>(OnBindIndexBuffer);
      shared.UnregisterEvent<reshade::addon_event::init_pipeline>(OnInitPipeline);
      shared.UnregisterEvent<reshade::addon_event::destroy_pipeline>(OnDestroyPipeline);
      shared.UnregisterEvent<reshade::addon_event::present>(OnPresent);
      shared.UnregisterEvent<reshade::addon_event::destroy_device>(OnDestroyDevice);
      renodx::utils::command_action::Unregister(SceneDrawCallback{});
      renodx::utils::resource::Use(DLL_PROCESS_DETACH);
      renodx::utils::state::Use(DLL_PROCESS_DETACH);
      renodx::utils::shader::Use(DLL_PROCESS_DETACH);
      renodx::utils::command_action::Use(DLL_PROCESS_DETACH);
      shared.UnregisterModule();
      break;
  }
}

}  // namespace renodx::utils::scene
