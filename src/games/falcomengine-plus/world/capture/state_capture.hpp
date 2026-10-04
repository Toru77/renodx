#pragma once

// Blend/depth/rasterizer state-object capture.
//
// On D3D11 these are separate state objects (CreateBlendState etc.). ReShade
// surfaces each creation as init_pipeline with a single descriptor subobject
// and each OMSet*/RSSet* bind as bind_pipeline, which utils::state records in
// its per-stage pipeline map. Capturing the descriptors here turns the bound
// state handles into depth-write / blend-enable facts for census classification.

#include <mutex>
#include <unordered_map>

#include "../world_state.hpp"

namespace falcom_world {

struct PipelineStateMaps {
  std::mutex mutex;
  std::unordered_map<uint64_t, reshade::api::blend_desc> blend;
  std::unordered_map<uint64_t, reshade::api::depth_stencil_desc> depth_stencil;
  std::unordered_map<uint64_t, reshade::api::rasterizer_desc> rasterizer;
};

inline PipelineStateMaps& StateMaps() {
  static PipelineStateMaps maps;
  return maps;
}

inline void OnInitPipelineState(
    reshade::api::device* device,
    reshade::api::pipeline_layout layout,
    uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects,
    reshade::api::pipeline pipeline) {
  (void)device;
  (void)layout;
  if (subobjects == nullptr || pipeline.handle == 0u) return;

  auto& maps = StateMaps();
  std::lock_guard<std::mutex> lock(maps.mutex);
  for (uint32_t i = 0; i < subobject_count; ++i) {
    const auto& subobject = subobjects[i];
    if (subobject.count == 0u || subobject.data == nullptr) continue;
    switch (subobject.type) {
      case reshade::api::pipeline_subobject_type::blend_state:
        maps.blend.insert_or_assign(pipeline.handle, *static_cast<const reshade::api::blend_desc*>(subobject.data));
        break;
      case reshade::api::pipeline_subobject_type::depth_stencil_state:
        maps.depth_stencil.insert_or_assign(pipeline.handle, *static_cast<const reshade::api::depth_stencil_desc*>(subobject.data));
        break;
      case reshade::api::pipeline_subobject_type::rasterizer_state:
        maps.rasterizer.insert_or_assign(pipeline.handle, *static_cast<const reshade::api::rasterizer_desc*>(subobject.data));
        break;
      default:
        break;
    }
  }
}

inline void OnDestroyPipelineState(reshade::api::device* device, reshade::api::pipeline pipeline) {
  (void)device;
  if (pipeline.handle == 0u) return;
  auto& maps = StateMaps();
  std::lock_guard<std::mutex> lock(maps.mutex);
  maps.blend.erase(pipeline.handle);
  maps.depth_stencil.erase(pipeline.handle);
  maps.rasterizer.erase(pipeline.handle);
}

inline bool GetBlendDesc(uint64_t handle, reshade::api::blend_desc* out) {
  if (out == nullptr || handle == 0u) return false;
  auto& maps = StateMaps();
  std::lock_guard<std::mutex> lock(maps.mutex);
  const auto it = maps.blend.find(handle);
  if (it == maps.blend.end()) return false;
  *out = it->second;
  return true;
}

inline bool GetDepthStencilDesc(uint64_t handle, reshade::api::depth_stencil_desc* out) {
  if (out == nullptr || handle == 0u) return false;
  auto& maps = StateMaps();
  std::lock_guard<std::mutex> lock(maps.mutex);
  const auto it = maps.depth_stencil.find(handle);
  if (it == maps.depth_stencil.end()) return false;
  *out = it->second;
  return true;
}

inline bool GetRasterizerDesc(uint64_t handle, reshade::api::rasterizer_desc* out) {
  if (out == nullptr || handle == 0u) return false;
  auto& maps = StateMaps();
  std::lock_guard<std::mutex> lock(maps.mutex);
  const auto it = maps.rasterizer.find(handle);
  if (it == maps.rasterizer.end()) return false;
  *out = it->second;
  return true;
}

}  // namespace falcom_world
