#pragma once

#include "../debug/crash_log.hpp"

// Shared compute-pass helpers: null-slot clearing, the D3D11 helper-pass layout used by the rt12 passes, and the temporal reset rule.

#include <cstdint>

#include "../bvh/bvh_trace.hpp"
#include "compute_resources.hpp"
#include "compute_state.hpp"

namespace falcom_world::compute {

// Null-array sizes: upper bounds of the SRV, UAV and sampler counts of every pass that calls NullComputeSlots.
// 8 is the D3D11 compute UAV slot count.
inline constexpr uint32_t kNullComputeSrvMax = 32u;
inline constexpr uint32_t kNullComputeUavMax = 8u;
inline constexpr uint32_t kNullComputeSamplerMax = 2u;
// NullComputeSlots: nulls the compute slots on the context with push_descriptors (pipeline_layout{0}),
// which binds directly at the register (table updates do not change the context).
inline void NullComputeSlots(reshade::api::command_list* cmd_list, uint32_t srv_count, uint32_t uav_count, uint32_t sampler_count) {
  const reshade::api::resource_view null_srvs[kNullComputeSrvMax] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, srv_count, reshade::api::descriptor_type::texture_shader_resource_view, null_srvs});
  const reshade::api::resource_view null_uavs[kNullComputeUavMax] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, uav_count, reshade::api::descriptor_type::texture_unordered_access_view, null_uavs});
  if (sampler_count == 0u) return;
  const reshade::api::sampler null_samplers[kNullComputeSamplerMax] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, sampler_count, reshade::api::descriptor_type::sampler, null_samplers});
}

#if !defined(FALCOM_WINDOWS_HARNESS)
}  // namespace falcom_world::compute
#include "../rt12/rt12_backend.hpp"
#include "../rt12/rt12_rtxdi.hpp"
#include "../rt12/rt12_rtxdi_transfer.hpp"
namespace falcom_world::compute {
#endif

}  // namespace falcom_world::compute
