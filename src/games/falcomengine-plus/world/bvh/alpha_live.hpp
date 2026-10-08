#pragma once

// GPU stage of alpha-tested foliage. UpdateLiveBvh runs SyncLiveAlpha after the mesh sync and before the
// TLAS: each alpha mesh that is resident, has UVs and has a source copy (CapturePoolAlphaSource in
// bvh_pool.hpp) gets one atlas slice, blitted from its copy, and its material entry. With alpha_foliage
// off nothing here exists (DestroyAlphaGpu in bvh_resources.hpp).

#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#include "../../../../utils/log.hpp"
#include "alpha_atlas.hpp"
#include "bvh_pool.hpp"
#include "bvh_resources.hpp"

namespace falcom_world::bvh {

// Creates whatever of the atlas is missing (each object once). False when one cannot be created; the
// mesh then keeps its source copy and waits.
inline bool EnsureAlphaGpu(reshade::api::device* device, BvhDeviceData* data) {
  AlphaGpu& alpha = data->alpha;
  using RU = reshade::api::resource_usage;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using F = reshade::api::format;
  using DR = reshade::api::descriptor_range;
  using P = reshade::api::pipeline_layout_param;
  const reshade::api::resource_view_desc array_view(
      reshade::api::resource_view_type::texture_2d_array, F::r32_uint, 0u, 1u, 0u, kAlphaAtlasSlices);
  const std::vector<uint8_t> zeros(kAlphaAtlasSlices * sizeof(AlphaMaterialGPU));

  DR srv_range = {0, 0, 0, 1, DS::all_compute, 1, DT::shader_resource_view};
  DR sampler_range = {0, 0, 0, 1, DS::all_compute, 1, DT::sampler};
  DR uav_range = {0, 0, 0, 1, DS::all_compute, 1, DT::unordered_access_view};
  reshade::api::constant_range push_range = {};
  push_range.binding = 0;
  push_range.dx_register_index = 0;
  push_range.dx_register_space = 0;
  push_range.count = 1;
  push_range.visibility = DS::all_compute;
  P params[4] = {};
  params[0].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[0].descriptor_table.count = 1;
  params[0].descriptor_table.ranges = &srv_range;
  params[1].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[1].descriptor_table.count = 1;
  params[1].descriptor_table.ranges = &sampler_range;
  params[2].type = reshade::api::pipeline_layout_param_type::descriptor_table;
  params[2].descriptor_table.count = 1;
  params[2].descriptor_table.ranges = &uav_range;
  params[3].type = reshade::api::pipeline_layout_param_type::push_constants;
  params[3].push_constants = push_range;

  bool ready =
      (alpha.atlas.handle != 0u
       || device->create_resource(
              reshade::api::resource_desc(reshade::api::resource_type::texture_2d, kAlphaSliceWords, kAlphaSliceWords,
                                          static_cast<uint16_t>(kAlphaAtlasSlices), 1, F::r32_uint, 1,
                                          reshade::api::memory_heap::gpu_only, RU::shader_resource | RU::unordered_access),
              nullptr, RU::unordered_access, &alpha.atlas))
      && (alpha.atlas_srv.handle != 0u || device->create_resource_view(alpha.atlas, RU::shader_resource, array_view, &alpha.atlas_srv))
      && (alpha.atlas_uav.handle != 0u || device->create_resource_view(alpha.atlas, RU::unordered_access, array_view, &alpha.atlas_uav))
      && (alpha.materials.handle != 0u
          || CreatePoolBuffer(device, zeros.data(), zeros.size(), sizeof(AlphaMaterialGPU), &alpha.materials, &alpha.materials_srv))
      && (alpha.sampler.handle != 0u || device->create_sampler(reshade::api::sampler_desc{}, &alpha.sampler))
      && (alpha.blit_layout.handle != 0u || device->create_pipeline_layout(4, params, &alpha.blit_layout));
  for (uint32_t i = 0u; i < 3u && ready; ++i) {
    ready = alpha.blit_tables[i].handle != 0u || device->allocate_descriptor_table(alpha.blit_layout, i, &alpha.blit_tables[i]);
  }
#if defined(__world_alpha_blit_EMBED_FILE)
  if (ready && alpha.blit_pipeline.handle == 0u) {
    reshade::api::shader_desc shader = {};
    shader.code = __world_alpha_blit.data();
    shader.code_size = __world_alpha_blit.size();
    shader.entry_point = "main";
    reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    ready = device->create_pipeline(alpha.blit_layout, 1, &subobject, &alpha.blit_pipeline);
  }
#else
  ready = false;
#endif
  if (!ready) {
    if (!alpha.failure_logged) {  // retried every present: logged once
      alpha.failure_logged = true;
      renodx::utils::log::w("[world-bvh] alpha atlas: GPU objects not created (atlas ", alpha.atlas.handle != 0u,
                            ", blit ", alpha.blit_pipeline.handle != 0u, ")");
    }
    return false;
  }
  // The sampler and atlas bindings never change: only the source SRV is rewritten per blit.
  const reshade::api::descriptor_table_update updates[2] = {
      {alpha.blit_tables[1], 0, 0, 1, DT::sampler, &alpha.sampler},
      {alpha.blit_tables[2], 0, 0, 1, DT::unordered_access_view, &alpha.atlas_uav}};
  device->update_descriptor_tables(2, updates);
  return true;
}

struct AlphaBlitJob {
  uint64_t source = 0u;  // source texture handle of the copy (alpha_sources)
  uint64_t uid = 0u;     // mesh uid that gets the slice
  reshade::api::resource proxy = {0u};
  reshade::api::format format = reshade::api::format::unknown;
  PoolAlphaMaterial material;
};

// Fills the atlas for the alpha meshes of the store and keeps slot_of_uid (the TLAS reads it). Runs on the
// present thread: the pool is read under its lock, and every graphics call runs after the lock is released.
inline void SyncLiveAlpha(reshade::api::device* device, reshade::api::command_list* cmd_list, BvhDeviceData* data, uint32_t frame) {
  AlphaGpu& alpha = data->alpha;
  const bool on = g_pool.alpha_foliage.load(std::memory_order_relaxed);
  {
    // The numbers of the previous present: the blit timer resolves a frame late.
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (!on) {
      g_pool.alpha_sync_logged = false;
    } else if (!g_pool.alpha_sync_logged) {
      g_pool.alpha_sync_logged = true;
      renodx::utils::log::i("[world-bvh] alpha stage: first SyncLiveAlpha since alpha foliage was switched on");
    }
    PoolAlphaGpuStats& gpu = g_pool.alpha_gpu;
    gpu.slices_used = alpha.slices.used;
    gpu.blits = alpha.blits;
    gpu.blits_frame = alpha.blits_frame;
    gpu.proxies = static_cast<uint32_t>(g_pool.alpha_sources.size());
    gpu.proxy_bytes = PoolAlphaProxyBytes();
    gpu.cap_refused = alpha.cap_refused;
    gpu.tlas_instances = data->live.tlas_alpha_instances;
    gpu.waiting = data->live.tlas_alpha_waiting;
    gpu.blit_ms = alpha.timer.last_ms;
  }
  for (const reshade::api::resource proxy : TakePoolAlphaProxies(!on)) device->destroy_resource(proxy);
  if (!on) {
    DestroyAlphaGpu(device, data);
    return;
  }

  std::vector<AlphaBlitJob> jobs;
  std::vector<reshade::api::resource> stale;  // copies with no mesh to blit into (freed below)
  std::unordered_set<uint64_t> ready_uids;    // alive alpha meshes with UVs, without a conflict (pool state only)
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    for (const WorldMesh& mesh : g_pool.meshes) {
      if (mesh.alpha && !mesh.alpha_state.conflict && !mesh.uvs.empty()) ready_uids.insert(mesh.uid);
    }
    for (auto it = g_pool.alpha_sources.begin(); it != g_pool.alpha_sources.end();) {
      const PoolAlphaSource& source = it->second;
      if (source.proxy.handle == 0u) {  // its copy is being made (outside the lock)
        ++it;
        continue;
      }
      bool wanted = false;  // a draw key of the copy has a mesh or waits for one
      for (const uint64_t key : source.keys) {
        wanted = wanted || g_pool.mesh_by_key.count(key) != 0u || g_pool.mesh_queued.count(key) != 0u
                 || g_pool.mesh_requests.count(key) != 0u;
      }
      if (!wanted) {  // no key uses the copy: freed; an unblitted copy is not copied again until its key is drawn again
        stale.push_back(source.proxy);
        for (const uint64_t key : source.keys) {
          g_pool.alpha_key_source.erase(key);
          if (!source.blitted) g_pool.alpha_source_done.insert(key);
        }
        it = g_pool.alpha_sources.erase(it);
        continue;
      }
      for (const uint64_t key : source.keys) {
        const auto mesh_it = g_pool.mesh_by_key.find(key);
        if (mesh_it == g_pool.mesh_by_key.end() || mesh_it->second >= g_pool.meshes.size()) continue;
        const WorldMesh& mesh = g_pool.meshes[mesh_it->second];
        if (ready_uids.count(mesh.uid) == 0u || alpha.slot_of_uid.count(mesh.uid) != 0u) continue;
        bool chosen = false;
        for (const AlphaBlitJob& job : jobs) chosen = chosen || job.uid == mesh.uid;
        // Blit only into a mesh resident in the live store (slot_by_uid); the copy stays in the map until then.
        if (chosen || data->slot_by_uid.count(mesh.uid) == 0u || jobs.size() >= kAlphaBlitsPerFrame) continue;
        jobs.push_back({it->first, mesh.uid, source.proxy, source.format, mesh.alpha_state.material});
      }
      ++it;
    }
  }
  for (auto it = alpha.slot_of_uid.begin(); it != alpha.slot_of_uid.end();) {
    if (ready_uids.count(it->first) != 0u) {
      ++it;
      continue;
    }
    alpha.quarantine.emplace_back(it->second, frame);
    it = alpha.slot_of_uid.erase(it);
    data->store_version += 1u;
  }
  for (auto it = alpha.quarantine.begin(); it != alpha.quarantine.end();) {
    if (frame >= it->second + kAlphaSliceQuarantineFrames) {
      alpha.slices.Release(it->first);
      it = alpha.quarantine.erase(it);
    } else {
      ++it;
    }
  }
  for (const reshade::api::resource proxy : stale) device->destroy_resource(proxy);
  alpha.blits_frame = 0u;
  if (jobs.empty()) return;

  // A job that cannot run keeps its copy (still in the map): the mesh is retried at the next present.
  if (!EnsureAlphaGpu(device, data)) return;

  using RU = reshade::api::resource_usage;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  BeginGpuTimer(device, cmd_list, &alpha.timer);
  uint32_t blits = 0u;
  for (const AlphaBlitJob& job : jobs) {
    const int32_t slot = alpha.slices.Acquire(job.uid);
    if (slot < 0) {
      if (alpha.cap_refused_uids.insert(job.uid).second) alpha.cap_refused += 1u;  // counted once per mesh
      continue;
    }
    const uint32_t slice = static_cast<uint32_t>(slot);
    AlphaMaterialGPU material;
    material.threshold = job.material.threshold;
    material.scroll[0] = job.material.scroll[0];
    material.scroll[1] = job.material.scroll[1];
    material.swizzle = job.material.swizzle;
    material.slice = slice;
    std::string error;
    reshade::api::resource_view source = {0u};
    const bool ok = WriteBufferRange(device, cmd_list, alpha.materials, sizeof(AlphaMaterialGPU),
                                     static_cast<uint64_t>(slice) * sizeof(AlphaMaterialGPU), &material, sizeof(material), &error)
                    && device->create_resource_view(job.proxy, RU::shader_resource, reshade::api::resource_view_desc(job.format), &source);
    if (ok) {
      const reshade::api::descriptor_table_update source_update = {alpha.blit_tables[0], 0, 0, 1, DT::shader_resource_view, &source};
      device->update_descriptor_tables(1, &source_update);
      cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, alpha.blit_pipeline);
      cmd_list->bind_descriptor_tables(DS::all_compute, alpha.blit_layout, 0, 3, alpha.blit_tables);
      cmd_list->push_constants(DS::all_compute, alpha.blit_layout, 3, 0, 1, &slice);
      cmd_list->dispatch(kAlphaSliceWords / 8u, kAlphaSliceWords / 8u, 1u);
    }
    if (source.handle != 0u) device->destroy_resource_view(source);
    if (!ok) {
      alpha.slices.Release(slice);
      renodx::utils::log::w("[world-bvh] alpha atlas: slice ", slice, " for mesh uid ", job.uid, " not written: ", error);
      continue;
    }
    {
      // The copy stays (blitted) so the slice can be filled again; if its texture was invalidated meanwhile, the
      // invalidation already queued the proxy.
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      const auto source_it = g_pool.alpha_sources.find(job.source);
      if (source_it != g_pool.alpha_sources.end() && source_it->second.proxy.handle == job.proxy.handle) {
        source_it->second.blitted = true;
      }
    }
    alpha.slot_of_uid[job.uid] = slice;
    if (alpha.blits == 0u && blits == 0u) {
      renodx::utils::log::i("[world-bvh] alpha atlas: first slice filled (mesh uid ", job.uid, ", slice ", slice, ")");
    }
    blits += 1u;
  }
  EndGpuTimer(cmd_list, &alpha.timer);
  if (blits != 0u) {
    // Unbind the atlas UAV at u0: the trace binds the atlas as SRV t15, and D3D11 nulls an SRV that is still bound as a UAV.
    const reshade::api::resource_view null_uav = {0u};
    const reshade::api::descriptor_table_update unbind = {alpha.blit_tables[2], 0, 0, 1, DT::unordered_access_view, &null_uav};
    device->update_descriptor_tables(1, &unbind);
    cmd_list->bind_descriptor_tables(DS::all_compute, alpha.blit_layout, 2, 1, &alpha.blit_tables[2]);
    const reshade::api::resource atlas = alpha.atlas;
    const RU before = RU::unordered_access;
    const RU after = RU::shader_resource;
    cmd_list->barrier(1, &atlas, &before, &after);
    data->store_version += 1u;  // the TLAS can now use the new slices
  }
  alpha.blits += blits;
  alpha.blits_frame = blits;
}
}  // namespace falcom_world::bvh
