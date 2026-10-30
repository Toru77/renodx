#pragma once

// Blend/depth/rasterizer state-object capture.
//
// On D3D11 these are separate state objects (CreateBlendState etc.). ReShade
// surfaces each creation as init_pipeline with a single descriptor subobject
// and each OMSet*/RSSet* bind as bind_pipeline, which utils::state records in
// its per-stage pipeline map. Capturing the descriptors here turns the bound
// state handles into depth-write / blend-enable facts for census classification.

#include <atomic>
#include <mutex>
#include <unordered_map>

#include "../world_state.hpp"

namespace falcom_world {

// Moves whenever a native handle may be reused (pipeline, buffer or census reset). The per-thread record-fill caches
// in draw_census.hpp drop their entries when it changes.
inline std::atomic<uint32_t> g_record_cache_generation{0u};

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

// Per-thread copy of one state map. A found description stays cached until g_record_cache_generation moves; a miss is never
// cached, so the callers' D3D11 default-state fallback is unchanged. The last handle is checked first.
template <typename Desc>
inline bool FindStateDesc(const std::unordered_map<uint64_t, Desc>& map, uint64_t handle, Desc* out) {
  thread_local uint32_t cache_generation = 0u;
  thread_local std::unordered_map<uint64_t, Desc> cache;
  thread_local uint64_t last_handle = 0u;
  thread_local Desc last_desc = {};
  const uint32_t generation = g_record_cache_generation.load(std::memory_order_acquire);
  if (generation != cache_generation) {
    cache.clear();
    last_handle = 0u;
    cache_generation = generation;
  }
  if (handle == last_handle) {
    *out = last_desc;
    return true;
  }
  if (const auto cached = cache.find(handle); cached != cache.end()) {
    const auto& [cached_handle, cached_desc] = *cached;
    last_desc = cached_desc;
  } else {
    Desc found = {};
    {
      std::lock_guard<std::mutex> lock(StateMaps().mutex);
      const auto it = map.find(handle);
      if (it == map.end()) return false;
      const auto& [map_handle, map_desc] = *it;
      found = map_desc;
    }
    cache.insert_or_assign(handle, found);
    last_desc = found;
  }
  last_handle = handle;
  *out = last_desc;
  return true;
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
  g_record_cache_generation.fetch_add(1u, std::memory_order_release);

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
  // Also bumped at init: a reused handle is never served a value cached for the destroyed object (scene.hpp erases in its own handler).
  g_record_cache_generation.fetch_add(1u, std::memory_order_release);
}

inline bool GetBlendDesc(uint64_t handle, reshade::api::blend_desc* out) {
  if (out == nullptr || handle == 0u) return false;
  return FindStateDesc(StateMaps().blend, handle, out);
}

inline bool GetDepthStencilDesc(uint64_t handle, reshade::api::depth_stencil_desc* out) {
  if (out == nullptr || handle == 0u) return false;
  return FindStateDesc(StateMaps().depth_stencil, handle, out);
}

inline bool GetRasterizerDesc(uint64_t handle, reshade::api::rasterizer_desc* out) {
  if (out == nullptr || handle == 0u) return false;
  return FindStateDesc(StateMaps().rasterizer, handle, out);
}

}  // namespace falcom_world
