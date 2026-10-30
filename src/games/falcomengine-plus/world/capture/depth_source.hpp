#pragma once

// Lifetime of the captured game depth source and the resource size cache.
//
// CaptureLightingInputs (draw_census.hpp) records the depth SRV the lighting
// pass reads (t4) once per frame. The BVH depth comparison binds that view, so
// it must be forgotten as soon as the game releases the view or its texture.
// QueryBufferInfo (draw_census.hpp) caches buffer sizes and flags by handle; a
// handle reused by a new buffer must not report the old ones.

#include <mutex>

#include "../world_state.hpp"
#include "state_capture.hpp"

namespace falcom_world {

inline void OnDestroyResourceWorld(reshade::api::device* device, reshade::api::resource resource) {
  (void)device;
  std::lock_guard<std::mutex> lock(g_state.mutex);
  if (g_state.depth_source.resource.handle == resource.handle) {
    g_state.depth_source = {};
  }
  g_state.buffer_info.erase(resource.handle);
  g_record_cache_generation.fetch_add(1u, std::memory_order_release);
}

inline void OnDestroyResourceViewWorld(reshade::api::device* device, reshade::api::resource_view view) {
  (void)device;
  g_state.view_generation.fetch_add(1u, std::memory_order_release);
  std::lock_guard<std::mutex> lock(g_state.mutex);
  if (g_state.depth_source.view.handle == view.handle) {
    g_state.depth_source = {};
  }
}

}  // namespace falcom_world
