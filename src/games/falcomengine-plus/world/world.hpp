#pragma once

// falcomengine-plus world module: draw census, world pool and world BVH.
//
// One entry point (Use) plus one settings hook (AddSettings). Everything is
// DevKit-gated: when DevKit is not present, Use and AddSettings do nothing and
// no events, capture state or per-frame work exist. The pool admits geometry
// by bytecode shader class (contract/), bvh/ builds and traces the BVH. There
// is no ray-traced consumer yet.

#include <Windows.h>

#include "world_state.hpp"
#include "capture/buffer_readback.hpp"
#include "capture/cb_tracking.hpp"
#include "capture/cb_value_tracker.hpp"
#include "contract/shader_registry.hpp"
#include "capture/state_capture.hpp"
#include "capture/draw_census.hpp"
#include "capture/mesh_capture.hpp"
#include "capture/depth_source.hpp"
#include "research/classification.hpp"
#include "research/reference_hints.hpp"
#include "research/transform_candidates.hpp"
#include "debug/crash_log.hpp"
#include "debug/overlay.hpp"
#include "world_settings.hpp"
#include "bvh/world_bvh.hpp"

#include "../../../utils/command_action.hpp"
#include "../../../utils/constants.hpp"
#include "../../../utils/scene.hpp"

namespace falcom_world {

inline void Use(DWORD fdw_reason, bool supported) {
  static bool attached = false;
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH: {
      if (attached) return;
      g_state.supported = supported && IsDevkitPresent();
      if (!g_state.supported) return;
      attached = true;

      g_state.ring.reserve(kCensusRingSize);

      bvh::Use(fdw_reason);

      renodx::utils::scene::Use(fdw_reason);
      if (renodx::utils::scene::shared.data != nullptr) {
        renodx::utils::scene::shared.data->enabled.store(false);
      }

      renodx::utils::constants::capture_constant_buffers = true;
      renodx::utils::constants::Use(fdw_reason);
      if (renodx::utils::constants::shared.data != nullptr) {
        renodx::utils::constants::shared.data->capture_constant_buffers = true;
      }

      // Bytecode classifier and b1 mirror: what the BVH pool admits on.
      contract::RegisterShaderRegistry();
      RegisterCbTracker();

      RegisterCensus();
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsWorld);
      reshade::register_event<reshade::addon_event::init_command_list>(OnInitWorldCommandList);
      reshade::register_event<reshade::addon_event::reset_command_list>(OnResetWorldCommandList);
      reshade::register_event<reshade::addon_event::destroy_command_list>(OnDestroyWorldCommandList);
      reshade::register_event<reshade::addon_event::init_pipeline>(OnInitPipelineState);
      reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineState);
      reshade::register_event<reshade::addon_event::present>(OnWorldPresent);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceWorld);
      reshade::register_event<reshade::addon_event::destroy_resource>(OnDestroyResourceWorld);
      reshade::register_event<reshade::addon_event::destroy_resource_view>(OnDestroyResourceViewWorld);
      reshade::register_event<reshade::addon_event::present>(OnCrashLogPresent);
      break;
    }
    case DLL_PROCESS_DETACH: {
      if (!attached) return;
      attached = false;
      // The crash handler lives in this module: remove it before unloading.
      SetCrashLogEnabled(false);
      bvh::Use(fdw_reason);
      contract::UnregisterShaderRegistry();
      UnregisterCbTracker();
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsWorld);
      reshade::unregister_event<reshade::addon_event::init_command_list>(OnInitWorldCommandList);
      reshade::unregister_event<reshade::addon_event::reset_command_list>(OnResetWorldCommandList);
      reshade::unregister_event<reshade::addon_event::destroy_command_list>(OnDestroyWorldCommandList);
      reshade::unregister_event<reshade::addon_event::init_pipeline>(OnInitPipelineState);
      reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineState);
      reshade::unregister_event<reshade::addon_event::present>(OnWorldPresent);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceWorld);
      reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResourceWorld);
      reshade::unregister_event<reshade::addon_event::destroy_resource_view>(OnDestroyResourceViewWorld);
      reshade::unregister_event<reshade::addon_event::present>(OnCrashLogPresent);
      renodx::utils::command_action::Unregister(WorldDrawCallback{});
      renodx::utils::constants::Use(fdw_reason);
      renodx::utils::scene::Use(fdw_reason);
      break;
    }
    default:
      break;
  }
}

}  // namespace falcom_world
