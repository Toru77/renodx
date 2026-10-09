#pragma once

// RTAO round 1 dispatch (world_rtao.cs_5_0.hlsl). Called inline from the lighting
// hook (addon.cpp RunRtaoInline), which owns the guards. No module mutex is held
// around any graphics call here.

#include <atomic>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <vector>

#include "../bvh/bvh_trace.hpp"
#include "rtao_resources.hpp"
#include "rtao_state.hpp"

namespace falcom_world::rtao {

struct RtaoFrameState {
  Reason reason = Reason::Off;       // why AO is not produced this frame
  Reason logged_reason = Reason::Off;
  Reason note = Reason::Off;         // non-fatal note (isfast_unavailable: IGN used)
  bool producing = false;            // AO written this frame (rtao_active == 2)
  uint64_t dispatched_frame = UINT64_MAX;
  uint64_t frames_without_ao = 0u;   // RTAO requested but no AO this frame
  float gpu_ms = -1.f;               // newest completed RTAO dispatch (negative: none yet)
  uint64_t texture_bytes = 0u;       // AO target plus stats buffer
  bool fade_valid = false;           // fade is computed from the built region
  Fade fade = {};
};

inline RtaoFrameState g_rtao_frame;

// Stats readback runs only when requested (panel "Read RTAO Stats", handled in OnWorldPresentBvh).
struct RtaoStatsState {
  std::atomic_bool requested{false};
  bool valid = false;
  uint32_t values[kRtaoStatsCount] = {};
};

inline RtaoStatsState g_rtao_stats;

struct RtaoDispatchInputs {
  reshade::api::resource_view depth_view = {0u};
  reshade::api::resource_view mrt_normal_view = {0u};
  reshade::api::resource_view isfast_view = {0u};
  reshade::api::resource_view scene_cbv_view = {0u};
  uint32_t width = 0u;
  uint32_t height = 0u;
  RtaoPushConstants push = {};
};

// Binds, dispatches and unbinds. Returns true when the AO texture was written.
inline bool Dispatch(reshade::api::device* device, reshade::api::command_list* cmd_list, const RtaoDispatchInputs& in) {
  auto* bvh_data = bvh::GetBvhDeviceData(device);
  if (bvh_data == nullptr || in.width == 0u || in.height == 0u) return false;
  RtaoDeviceData& data = GetRtaoDeviceData(device);
  if (!EnsureRtaoPipeline(device, &data) || !EnsureRtaoTarget(device, &data, in.width, in.height)
      || !EnsureRtaoStats(device, &data)) {
    return false;
  }

  RtaoPushConstants push = in.push;
  push.size[0] = static_cast<float>(in.width);
  push.size[1] = static_cast<float>(in.height);
  push.size[2] = 0.f;  // round 1: deforming objects are not traced (t10-t13 null)
  push.size[3] = 0.f;

  // Whole-buffer write (offset 0, full size): see WriteBufferRange in bvh_resources.hpp.
  const uint32_t zeros[kRtaoStatsCount] = {};
  device->update_buffer_region(zeros, data.stats_buffer, 0u, sizeof(zeros));

  reshade::api::resource_view srvs[kRtaoSrvCount] = {};
  bvh::FillBvhSceneSrvs(*bvh_data, in.depth_view, bvh::DynamicTraceInputs{}, srvs);
  srvs[17] = in.mrt_normal_view;
  srvs[18] = in.isfast_view;
  const reshade::api::resource_view uavs[kRtaoUavCount] = {data.ao_uav, data.stats_uav};
  reshade::api::descriptor_table_update updates[3] = {
      {data.cbv_table, 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &in.scene_cbv_view},
      {data.srv_table, 0, 0, kRtaoSrvCount, reshade::api::descriptor_type::shader_resource_view, srvs},
      {data.uav_table, 0, 0, kRtaoUavCount, reshade::api::descriptor_type::unordered_access_view, uavs},
  };
  device->update_descriptor_tables(3, updates);

  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, data.pipeline);
  const reshade::api::descriptor_table tables[3] = {data.cbv_table, data.srv_table, data.uav_table};
  cmd_list->bind_descriptor_tables(reshade::api::shader_stage::all_compute, data.layout, 0, 3, tables);
  cmd_list->push_constants(
      reshade::api::shader_stage::all_compute, data.layout, 3, 0, kRtaoPushConstantCount, &push);
  bvh::BeginGpuTimer(device, cmd_list, &data.timer);
  cmd_list->dispatch((in.width + 7u) / 8u, (in.height + 7u) / 8u, 1u);
  bvh::EndGpuTimer(cmd_list, &data.timer);

  // Unbind on the context. update_descriptor_tables only changes the app-side table; bound views stay
  // on the D3D11 context until a bind or push replaces them. push_descriptors with pipeline_layout{0}
  // binds directly at the register (t0 = binding 0, u0 = binding 0), so nulls take effect here.
  const reshade::api::resource_view null_srvs[kRtaoSrvCount] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, kRtaoSrvCount, reshade::api::descriptor_type::texture_shader_resource_view, null_srvs});
  const reshade::api::resource_view null_uavs[kRtaoUavCount] = {};
  cmd_list->push_descriptors(
      reshade::api::shader_stage::all_compute, reshade::api::pipeline_layout{0}, 0,
      reshade::api::descriptor_table_update{
          {}, 0, 0, kRtaoUavCount, reshade::api::descriptor_type::texture_unordered_access_view, null_uavs});
  return true;
}

// Reads the stats buffer and writes world_rtao.json. Called once per present; no lock held.
inline void MaybeCaptureRtaoStats(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (!g_rtao_stats.requested.exchange(false, std::memory_order_relaxed)) return;
  const auto found = g_rtao_devices.find(device);
  if (found == g_rtao_devices.end() || found->second.stats_buffer.handle == 0u) return;
  const RtaoDeviceData& data = found->second;
  std::vector<uint8_t> bytes;
  if (!renodx::utils::scene::ReadbackBuffer(device, queue, data.stats_buffer, 0u, sizeof(g_rtao_stats.values), &bytes)
      || bytes.size() < sizeof(g_rtao_stats.values)) {
    renodx::utils::log::w("[world-rtao] stats readback failed");
    return;
  }
  std::memcpy(g_rtao_stats.values, bytes.data(), sizeof(g_rtao_stats.values));
  g_rtao_stats.valid = true;
  const uint32_t* v = g_rtao_stats.values;
  const float occluded = v[kRtaoStatRays] > 0u ? 100.f * v[kRtaoStatHits] / v[kRtaoStatRays] : 0.f;
  const float mean_ao = v[kRtaoStatPixels] > 0u ? static_cast<float>(v[kRtaoStatAoSum]) / (255.f * v[kRtaoStatPixels]) : 1.f;
  renodx::utils::log::i("[world-rtao] stats: rays=", v[kRtaoStatRays], " hits=", v[kRtaoStatHits],
                        " occluded_pct=", occluded, " mean_ao=", mean_ao, " pixels=", v[kRtaoStatPixels],
                        " sky=", v[kRtaoStatSky], " normal=", v[kRtaoStatNormal], " region=", v[kRtaoStatRegion],
                        " scaled=", v[kRtaoStatScaled], " invalid_refs=", v[kRtaoStatInvalidRefs],
                        " stack_overflow=", v[kRtaoStatStackOverflow], " gpu_ms=", g_rtao_frame.gpu_ms);

  std::ostringstream out;
  out << "{\n  \"schema\": 1,\n  \"generated_frame\": " << g_state.frame.load()
      << ",\n  \"reason\": \"" << ReasonName(g_rtao_frame.reason) << "\""
      << ",\n  \"producing\": " << (g_rtao_frame.producing ? "true" : "false")
      << ",\n  \"gpu_ms\": " << g_rtao_frame.gpu_ms
      << ",\n  \"texture_bytes\": " << g_rtao_frame.texture_bytes
      << ",\n  \"frames_without_ao\": " << g_rtao_frame.frames_without_ao
      << ",\n  \"fade\": {\"start_eff\": " << g_rtao_frame.fade.start << ", \"end_eff\": " << g_rtao_frame.fade.end
      << ", \"coverage\": " << g_rtao_frame.fade.coverage << "}"
      << ",\n  \"stats\": {\"rays\": " << v[kRtaoStatRays] << ", \"hits\": " << v[kRtaoStatHits]
      << ", \"pixels\": " << v[kRtaoStatPixels] << ", \"ao_sum\": " << v[kRtaoStatAoSum]
      << ", \"sky\": " << v[kRtaoStatSky] << ", \"normal\": " << v[kRtaoStatNormal]
      << ", \"region\": " << v[kRtaoStatRegion] << ", \"scaled\": " << v[kRtaoStatScaled]
      << ", \"invalid_refs\": " << v[kRtaoStatInvalidRefs]
      << ", \"stack_overflow\": " << v[kRtaoStatStackOverflow] << "}"
      << ",\n  \"note\": \"dynamic objects not traced (round 3); scaled pixels are counted by the shader\"\n}\n";
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(bvh::PoolOutputDir() / "world_rtao.json", text);
}

}  // namespace falcom_world::rtao
