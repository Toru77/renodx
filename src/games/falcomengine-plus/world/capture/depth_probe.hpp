#pragma once

// GPU depth probe for Phase 0 candidate validation.
//
// Builds bounded probe samples on the CPU (candidate world -> clip -> uv +
// expected device depth), dispatches world_depth_probe against the captured
// scene depth, and reads back per-candidate match/occlusion/mismatch counts.
// Comparison is in device-depth space so it is independent of matrix packing.

#include <array>
#include <cstring>
#include <vector>

#include "../world_state.hpp"
#include "../research/transform_candidates.hpp"

namespace falcom_world {

struct __declspec(uuid("a3f1c2d4-6b7e-4f90-8a1b-2c3d4e5f6a70")) WorldDeviceData {
  reshade::api::pipeline_layout layout = {0u};
  reshade::api::pipeline pipeline = {0u};
  std::array<reshade::api::descriptor_table, 2> tables = {};
  reshade::api::resource sample_buffer = {0u};
  reshade::api::resource_view sample_srv = {0u};
  reshade::api::resource result_buffer = {0u};
  reshade::api::resource_view result_uav = {0u};
  reshade::api::resource error_buffer = {0u};
  reshade::api::resource_view error_uav = {0u};
  reshade::api::resource sample_result_buffer = {0u};
  reshade::api::resource_view sample_result_uav = {0u};
  bool resources_ready = false;
};

struct ProbeBuild {
  std::vector<float> samples;  // 4 floats per sample: uv.x, uv.y, expected, candidate
  std::vector<ProbeSampleDiag> diag;
  std::vector<uint32_t> candidate_indices;
  uint32_t candidate_count = 0u;
  uint32_t samples_per_candidate = 0u;
};

inline bool EnsureProbeResources(reshade::api::device* device) {
  if (device == nullptr) return false;
  WorldDeviceData* data = nullptr;
  renodx::utils::data::CreateOrGet<WorldDeviceData>(device, data);
  if (data == nullptr) return false;
  if (data->resources_ready) return true;

#ifdef __world_depth_probe_EMBED_FILE
  using DR = reshade::api::descriptor_range;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using P = reshade::api::pipeline_layout_param;

  DR srv_r = {0, 0, 0, 2, DS::all_compute, 1, DT::shader_resource_view};
  DR uav_r = {0, 0, 0, 3, DS::all_compute, 1, DT::buffer_unordered_access_view};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = 13;
  push_range.dx_register_space = 0;
  push_range.count = 8;
  push_range.visibility = DS::all_compute;
  P p0, p1, pPush;
  p0.type = reshade::api::pipeline_layout_param_type::descriptor_table;
  p0.descriptor_table.count = 1;
  p0.descriptor_table.ranges = &srv_r;
  p1.type = reshade::api::pipeline_layout_param_type::descriptor_table;
  p1.descriptor_table.count = 1;
  p1.descriptor_table.ranges = &uav_r;
  pPush.type = reshade::api::pipeline_layout_param_type::push_constants;
  pPush.push_constants = push_range;
  P params[3] = {p0, p1, pPush};
  if (!device->create_pipeline_layout(3, params, &data->layout)) return false;
  if (!device->allocate_descriptor_table(data->layout, 0, &data->tables[0])) return false;
  if (!device->allocate_descriptor_table(data->layout, 1, &data->tables[1])) return false;

  if (!__world_depth_probe.empty()) {
    reshade::api::shader_desc sd = {};
    sd.code = __world_depth_probe.data();
    sd.code_size = __world_depth_probe.size();
    sd.entry_point = "main";
    reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &sd};
    if (!device->create_pipeline(data->layout, 1, &so, &data->pipeline)) return false;
  }
  if (data->pipeline.handle == 0u) return false;

  const uint32_t sample_elements = kProbeTopCandidates * kProbeMaxSamples;
  const auto make_buffer = [&](uint32_t elements, reshade::api::resource_usage usage, reshade::api::resource_usage initial, reshade::api::resource* out) {
    reshade::api::resource_desc desc = {};
    desc.type = reshade::api::resource_type::buffer;
    desc.buffer.size = static_cast<uint64_t>(elements) * 16u;
    desc.buffer.stride = 16u;
    desc.heap = reshade::api::memory_heap::gpu_only;
    desc.usage = usage;
    return device->create_resource(desc, nullptr, initial, out);
  };
  const auto make_view = [&](reshade::api::resource resource, reshade::api::resource_usage usage, uint32_t elements, reshade::api::resource_view* out) {
    return device->create_resource_view(
        resource, usage,
        reshade::api::resource_view_desc(reshade::api::resource_view_type::buffer, reshade::api::format::unknown, 0, elements),
        out);
  };

  if (!make_buffer(sample_elements, reshade::api::resource_usage::shader_resource, reshade::api::resource_usage::shader_resource, &data->sample_buffer)) return false;
  if (!make_view(data->sample_buffer, reshade::api::resource_usage::shader_resource, sample_elements, &data->sample_srv)) return false;
  const auto result_usage = reshade::api::resource_usage::unordered_access | reshade::api::resource_usage::copy_source;
  if (!make_buffer(kProbeTopCandidates, result_usage, reshade::api::resource_usage::unordered_access, &data->result_buffer)) return false;
  if (!make_view(data->result_buffer, reshade::api::resource_usage::unordered_access, kProbeTopCandidates, &data->result_uav)) return false;
  if (!make_buffer(kProbeTopCandidates, result_usage, reshade::api::resource_usage::unordered_access, &data->error_buffer)) return false;
  if (!make_view(data->error_buffer, reshade::api::resource_usage::unordered_access, kProbeTopCandidates, &data->error_uav)) return false;
  if (!make_buffer(sample_elements, result_usage, reshade::api::resource_usage::unordered_access, &data->sample_result_buffer)) return false;
  if (!make_view(data->sample_result_buffer, reshade::api::resource_usage::unordered_access, sample_elements, &data->sample_result_uav)) return false;

  data->resources_ready = true;
  return true;
#else
  return false;
#endif
}

inline void BuildProbeForCandidates(
    const std::vector<TransformCandidate>& candidates,
    const std::vector<uint32_t>& indices,
    const CapturedDraw& captured,
    const CameraSnapshot& camera,
    ProbeBuild* out) {
  if (out == nullptr) return;
  out->samples.clear();
  out->diag.clear();
  out->candidate_indices = indices;
  out->candidate_count = static_cast<uint32_t>(indices.size());
  out->samples_per_candidate = kProbeInstances * kProbeVertices;

  const auto& positions = captured.mesh.positions;
  for (uint32_t c = 0; c < out->candidate_count; ++c) {
    const uint32_t index = indices[c];
    const TransformCandidate* candidate = index < candidates.size() ? &candidates[index] : nullptr;
    const uint32_t instances = candidate != nullptr
                                   ? (std::min)(candidate->element_count, kProbeInstances)
                                   : 0u;
    std::vector<std::array<float, 3>> verts;
    if (candidate != nullptr && !positions.empty()) {
      const size_t stride = positions.size() > kProbeVertices ? (positions.size() / kProbeVertices) : 1u;
      for (size_t i = 0; i < positions.size() && verts.size() < kProbeVertices; i += stride) {
        verts.push_back(positions[i]);
      }
    }
    for (uint32_t inst = 0; inst < kProbeInstances; ++inst) {
      for (uint32_t v = 0; v < kProbeVertices; ++v) {
        float uv[2] = {-1.f, -1.f};
        float world[3] = {};
        float ndc_z = 0.f;
        bool ok = false;
        if (candidate != nullptr && inst < instances && v < verts.size()) {
          ok = ProjectCandidateInstance(*candidate, camera, inst, verts[v], uv, world, &ndc_z);
        }
        out->samples.push_back(uv[0]);
        out->samples.push_back(uv[1]);
        out->samples.push_back(ok ? ndc_z : 0.f);
        out->samples.push_back(static_cast<float>(c));
        ProbeSampleDiag diag;
        diag.instance = inst;
        diag.uv[0] = uv[0];
        diag.uv[1] = uv[1];
        diag.expected_linear = ok ? ndc_z : 0.f;
        out->diag.push_back(diag);
      }
    }
  }
}

inline void ParseProbeResults(
    const std::vector<uint8_t>& result_bytes,
    const std::vector<uint8_t>& error_bytes,
    const std::vector<uint8_t>& sample_bytes,
    uint32_t candidate_count,
    uint32_t samples_per_candidate,
    ProbeBuild* build,
    std::vector<ProbeMetrics>* out_metrics) {
  if (out_metrics == nullptr) return;
  out_metrics->assign(candidate_count, ProbeMetrics{});
  const auto* results = reinterpret_cast<const float*>(result_bytes.data());
  const auto* errors = reinterpret_cast<const float*>(error_bytes.data());
  const auto* samples = reinterpret_cast<const float*>(sample_bytes.data());
  for (uint32_t c = 0; c < candidate_count; ++c) {
    ProbeMetrics& metrics = (*out_metrics)[c];
    if (result_bytes.size() >= (static_cast<size_t>(c) + 1u) * 16u) {
      metrics.match = static_cast<uint32_t>(results[c * 4u + 0u] + 0.5f);
      metrics.occluded = static_cast<uint32_t>(results[c * 4u + 1u] + 0.5f);
      metrics.mismatch = static_cast<uint32_t>(results[c * 4u + 2u] + 0.5f);
      metrics.out_of_screen = static_cast<uint32_t>(results[c * 4u + 3u] + 0.5f);
      metrics.samples = metrics.match + metrics.occluded + metrics.mismatch + metrics.out_of_screen;
      const uint32_t denom = metrics.match + metrics.mismatch;
      metrics.match_ratio = denom > 0u ? static_cast<float>(metrics.match) / static_cast<float>(denom) : 0.f;
      metrics.valid = metrics.samples > 0u;
    }
    if (error_bytes.size() >= (static_cast<size_t>(c) + 1u) * 16u) {
      const float sum_err = errors[c * 4u + 0u];
      const float matched = errors[c * 4u + 1u];
      const float expected_sum = errors[c * 4u + 2u];
      const float total = errors[c * 4u + 3u];
      metrics.mean_abs_error = matched > 0.f ? sum_err / matched : 0.f;
      metrics.mean_expected = total > 0.f ? expected_sum / total : 0.f;
    }
    if (build != nullptr && sample_bytes.size() >= (static_cast<size_t>(c) + 1u) * samples_per_candidate * 16u) {
      metrics.diag.clear();
      for (uint32_t s = 0; s < samples_per_candidate && metrics.diag.size() < kProbeMaxSamples; ++s) {
        const size_t base = (static_cast<size_t>(c) * samples_per_candidate + s) * 4u;
        ProbeSampleDiag diag = build->diag[static_cast<size_t>(c) * samples_per_candidate + s];
        diag.sampled_linear = samples[base + 0u];
        diag.error = samples[base + 1u];
        diag.classification = static_cast<uint8_t>(samples[base + 2u] + 0.5f);
        metrics.diag.push_back(diag);
      }
    }
  }
}

inline void OnDestroyDeviceProbe(reshade::api::device* device) {
  if (device == nullptr) return;
  renodx::utils::data::Delete<WorldDeviceData>(device);
}

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

inline bool RunDepthProbe(
    reshade::api::device* device,
    reshade::api::command_list* cmd_list,
    reshade::api::command_queue* queue,
    const DepthSource& depth,
    ProbeBuild* build,
    float tol_rel,
    float tol_abs,
    std::vector<ProbeMetrics>* out_metrics) {
  if (device == nullptr || cmd_list == nullptr || queue == nullptr || build == nullptr) return false;
  if (!depth.valid || depth.view.handle == 0u || depth.resource.handle == 0u) return false;
  if (build->candidate_count == 0u || build->samples.empty()) return false;
  if (!EnsureProbeResources(device)) return false;

  WorldDeviceData* data = renodx::utils::data::Get<WorldDeviceData>(device);
  if (data == nullptr || !data->resources_ready) return false;

  const uint32_t candidate_count = (std::min)(build->candidate_count, kProbeTopCandidates);
  const uint32_t samples_per_candidate = build->samples_per_candidate;
  if (candidate_count == 0u || samples_per_candidate == 0u) return false;

  device->update_buffer_region(
      build->samples.data(), data->sample_buffer, 0u,
      static_cast<uint64_t>(build->samples.size()) * sizeof(float));

  reshade::api::resource_view srvs[2] = {depth.view, data->sample_srv};
  reshade::api::descriptor_table_update upd0 = {data->tables[0], 0, 0, 2, reshade::api::descriptor_type::shader_resource_view, srvs};
  device->update_descriptor_tables(1, &upd0);
  reshade::api::resource_view uavs[3] = {data->result_uav, data->error_uav, data->sample_result_uav};
  reshade::api::descriptor_table_update upd1 = {data->tables[1], 0, 0, 3, reshade::api::descriptor_type::buffer_unordered_access_view, uavs};
  device->update_descriptor_tables(1, &upd1);

  cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, data->pipeline);
  cmd_list->bind_descriptor_tables(
      reshade::api::shader_stage::all_compute, data->layout, 0, 2, data->tables.data());

  uint32_t push[8] = {};
  std::memcpy(&push[0], &tol_rel, sizeof(float));
  std::memcpy(&push[1], &tol_abs, sizeof(float));
  push[2] = samples_per_candidate;
  push[3] = candidate_count;
  push[4] = 1u;  // neighborhood radius (3x3)
  cmd_list->push_constants(reshade::api::shader_stage::all_compute, data->layout, 2, 0, 8, push);

  cmd_list->dispatch((candidate_count + 63u) / 64u, 1u, 1u);
  cmd_list->barrier(data->result_buffer, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::copy_source);
  cmd_list->barrier(data->error_buffer, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::copy_source);
  cmd_list->barrier(data->sample_result_buffer, reshade::api::resource_usage::unordered_access, reshade::api::resource_usage::copy_source);

  std::vector<uint8_t> result_bytes;
  std::vector<uint8_t> error_bytes;
  std::vector<uint8_t> sample_bytes;
  if (!renodx::utils::scene::ReadbackBuffer(device, queue, data->result_buffer, 0u, candidate_count * 16u, &result_bytes)) return false;
  if (!renodx::utils::scene::ReadbackBuffer(device, queue, data->error_buffer, 0u, candidate_count * 16u, &error_bytes)) return false;
  if (!renodx::utils::scene::ReadbackBuffer(
          device, queue, data->sample_result_buffer, 0u,
          static_cast<uint64_t>(candidate_count) * samples_per_candidate * 16u, &sample_bytes)) {
    return false;
  }

  ParseProbeResults(result_bytes, error_bytes, sample_bytes, candidate_count, samples_per_candidate, build, out_metrics);
  return true;
}

}  // namespace falcom_world
