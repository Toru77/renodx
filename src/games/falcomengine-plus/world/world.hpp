#pragma once

// falcomengine-plus Phase 0: world-space research module.
//
// One entry point (Use) plus one settings hook (AddSettings). Everything is
// DevKit-gated: when DevKit is not present, Use and AddSettings do nothing and
// no events, capture state or per-frame work exist. This phase only researches
// whether Sora 2nd static opaque geometry can be captured with a correct world
// transform; there is no mesh pool, BVH, TLAS or ray consumer here.

#include <Windows.h>

#include "world_state.hpp"
#include "capture/buffer_readback.hpp"
#include "capture/cb_tracking.hpp"
#include "capture/state_capture.hpp"
#include "capture/draw_census.hpp"
#include "capture/mesh_capture.hpp"
#include "capture/depth_probe.hpp"
#include "research/classification.hpp"
#include "research/coverage.hpp"
#include "research/reference_hints.hpp"
#include "research/transform_candidates.hpp"
#include "research/world_report.hpp"
#include "research/auto_research.hpp"
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

      RegisterCensus();
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsWorld);
      reshade::register_event<reshade::addon_event::init_command_list>(OnInitWorldCommandList);
      reshade::register_event<reshade::addon_event::destroy_command_list>(OnDestroyWorldCommandList);
      reshade::register_event<reshade::addon_event::init_pipeline>(OnInitPipelineState);
      reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineState);
      reshade::register_event<reshade::addon_event::present>(OnWorldPresent);
      reshade::register_event<reshade::addon_event::present>(OnWorldPresentAuto);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceWorld);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDeviceProbe);
      reshade::register_event<reshade::addon_event::destroy_resource>(OnDestroyResourceWorld);
      reshade::register_event<reshade::addon_event::destroy_resource_view>(OnDestroyResourceViewWorld);
      break;
    }
    case DLL_PROCESS_DETACH: {
      if (!attached) return;
      attached = false;
      bvh::Use(fdw_reason);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsWorld);
      reshade::unregister_event<reshade::addon_event::init_command_list>(OnInitWorldCommandList);
      reshade::unregister_event<reshade::addon_event::destroy_command_list>(OnDestroyWorldCommandList);
      reshade::unregister_event<reshade::addon_event::init_pipeline>(OnInitPipelineState);
      reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineState);
      reshade::unregister_event<reshade::addon_event::present>(OnWorldPresent);
      reshade::unregister_event<reshade::addon_event::present>(OnWorldPresentAuto);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceWorld);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDeviceProbe);
      reshade::unregister_event<reshade::addon_event::destroy_resource>(OnDestroyResourceWorld);
      reshade::unregister_event<reshade::addon_event::destroy_resource_view>(OnDestroyResourceViewWorld);
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
