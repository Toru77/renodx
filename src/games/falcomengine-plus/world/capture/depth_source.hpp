#pragma once

// Lifetime of the captured game depth source.
//
// CaptureLightingInputs (draw_census.hpp) records the depth SRV the lighting
// pass reads (t4) once per frame. The BVH depth comparison binds that view, so
// it must be forgotten as soon as the game releases the view or its texture.

#include <mutex>

#include "../world_state.hpp"

namespace falcom_world {

inline void OnDestroyResourceWorld(reshade::api::device* device, reshade::api::resource resource) {
  (void)device;
  std::lock_guard<std::mutex> lock(g_state.mutex);
  if (g_state.depth_source.resource.handle == resource.handle) {
    g_state.depth_source = {};
  }
}

inline void OnDestroyResourceViewWorld(reshade::api::device* device, reshade::api::resource_view view) {
  (void)device;
  std::lock_guard<std::mutex> lock(g_state.mutex);
  if (g_state.depth_source.view.handle == view.handle) {
    g_state.depth_source = {};
  }
}

}  // namespace falcom_world
