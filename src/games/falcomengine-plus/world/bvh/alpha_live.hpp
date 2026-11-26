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
#include "../rt12/rt12_mirror.hpp"

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

// Creates whatever of the albedo atlas is missing (each object once). False when one cannot be created; the
// meshes keep their source copies and are retried at the next present.
inline bool EnsureAlbedoGpu(reshade::api::device* device, BvhDeviceData* data) {
  AlbedoGpu& albedo = data->albedo;
  using RU = reshade::api::resource_usage;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using F = reshade::api::format;
  using DR = reshade::api::descriptor_range;
  using P = reshade::api::pipeline_layout_param;
  const reshade::api::resource_view_desc array_view(
      reshade::api::resource_view_type::texture_2d_array, F::r8g8b8a8_unorm, 0u, 1u, 0u, kAlbedoAtlasSlices);
  const std::vector<uint8_t> zeros(kAlbedoMaterials * sizeof(AlbedoMaterialGPU));

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
      (albedo.atlas.handle != 0u
       || device->create_resource(reshade::api::resource_desc(reshade::api::resource_type::texture_2d, kAlbedoSliceSize,
                                                             kAlbedoSliceSize, static_cast<uint16_t>(kAlbedoAtlasSlices), 1,
                                                             F::r8g8b8a8_unorm, 1, reshade::api::memory_heap::gpu_only,
                                                             RU::shader_resource | RU::unordered_access),
                                  nullptr, RU::unordered_access, &albedo.atlas))
      && (albedo.atlas_srv.handle != 0u
          || device->create_resource_view(albedo.atlas, RU::shader_resource, array_view, &albedo.atlas_srv))
      && (albedo.atlas_uav.handle != 0u
          || device->create_resource_view(albedo.atlas, RU::unordered_access, array_view, &albedo.atlas_uav))
      && (albedo.materials.handle != 0u
          || CreatePoolBuffer(device, zeros.data(), zeros.size(), sizeof(AlbedoMaterialGPU), &albedo.materials,
                              &albedo.materials_srv))
      && (albedo.sampler.handle != 0u || device->create_sampler(reshade::api::sampler_desc{}, &albedo.sampler))
      && (albedo.blit_layout.handle != 0u || device->create_pipeline_layout(4, params, &albedo.blit_layout));
  for (uint32_t i = 0u; i < 3u && ready; ++i) {
    ready = albedo.blit_tables[i].handle != 0u || device->allocate_descriptor_table(albedo.blit_layout, i, &albedo.blit_tables[i]);
  }
#if defined(__world_bvh_albedo_blit_EMBED_FILE)
  if (ready && albedo.blit_pipeline.handle == 0u) {
    reshade::api::shader_desc shader = {};
    shader.code = __world_bvh_albedo_blit.data();
    shader.code_size = __world_bvh_albedo_blit.size();
    shader.entry_point = "main";
    reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    ready = device->create_pipeline(albedo.blit_layout, 1, &subobject, &albedo.blit_pipeline);
  }
#else
  ready = false;
#endif
  if (!ready) {
    if (!albedo.failure_logged) {
      albedo.failure_logged = true;
      renodx::utils::log::w("[world-bvh] albedo atlas: GPU objects not created (atlas ", albedo.atlas.handle != 0u,
                            ", blit ", albedo.blit_pipeline.handle != 0u, ")");
    }
    return false;
  }
  const reshade::api::descriptor_table_update updates[2] = {
      {albedo.blit_tables[1], 0, 0, 1, DT::sampler, &albedo.sampler},
      {albedo.blit_tables[2], 0, 0, 1, DT::unordered_access_view, &albedo.atlas_uav}};
  device->update_descriptor_tables(2, updates);
  return true;
}

struct AlbedoKeyJob {
  uint64_t key = 0u;  // draw key admitted with this source's texture
  float scroll[2] = {};
  uint32_t swizzle = 0u;
};

struct AlbedoSourceJob {
  uint64_t source = 0u;  // source id of the copy (alpha_sources)
  uint64_t texture = 0u;  // game texture handle: the slice is keyed by it, not by the copy
  reshade::api::resource proxy = {0u};
  reshade::api::format format = reshade::api::format::unknown;
  uint32_t slice = 0u;
  bool blit = false;
  std::vector<AlbedoKeyJob> keys;  // ready draw keys of the source, with their material
};

// Fills one albedo atlas slice per source texture and maps every ready draw key to a material entry. Runs on the
// present thread after SyncLiveAlpha: alpha and albedo share the source copies, but own their slices. Dynamic and
// character meshes are excluded (they have no UV capture path here).
inline void SyncLiveAlbedo(reshade::api::device* device, reshade::api::command_list* cmd_list, BvhDeviceData* data, uint32_t frame) {
  AlbedoGpu& albedo = data->albedo;
  if (pt::g_pt_material_atlas <= 0.5f) {
    DestroyAlbedoGpu(device, data);
    return;
  }

  std::vector<AlbedoSourceJob> sources;
  std::unordered_set<uint64_t> ready_keys;    // draw keys whose mesh is ready for the atlas
  std::unordered_set<uint64_t> ready_uids;     // static meshes with UVs, not dynamic and not alpha-tested
  std::unordered_set<uint64_t> dead_textures;  // destroyed textures whose slice must be released
  std::vector<reshade::api::resource> stale;   // albedo-only proxies freed once their slice is filled
  const auto free_albedo_only_source = [&](uint64_t source_id, const PoolAlphaSource& source) {
    for (const uint64_t key : source.keys) g_pool.alpha_key_source.erase(key);
    UnlinkPoolAlphaTexture(source_id, source);
    ReleasePoolAlbedoBudget(source);
    stale.push_back(source.proxy);
  };
  {
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    for (uint64_t texture : g_pool.albedo_dead_textures) dead_textures.insert(texture);
    g_pool.albedo_dead_textures.clear();
    g_pool.stats.albedo_meshes_with_uvs = 0u;
    g_pool.stats.albedo_meshes_total = 0u;
    for (const WorldMesh& mesh : g_pool.meshes) {
      if (mesh.dynamic || mesh.alpha) continue;
      g_pool.stats.albedo_meshes_total += 1u;
      if (!mesh.uvs.empty()) {
        g_pool.stats.albedo_meshes_with_uvs += 1u;
        ready_uids.insert(mesh.uid);
      }
    }
    // A texture-keyed slice outlives its copy: keep the mapping alive while its mesh and slice are alive.
    for (const auto& [key, index] : albedo.material_of_key) {
      const auto mesh_it = g_pool.mesh_by_key.find(key);
      if (mesh_it == g_pool.mesh_by_key.end() || mesh_it->second >= g_pool.meshes.size()) continue;
      const WorldMesh& mesh = g_pool.meshes[mesh_it->second];
      if (ready_uids.count(mesh.uid) == 0u || data->slot_by_uid.count(mesh.uid) == 0u) continue;
      const auto key_texture = albedo.texture_of_key.find(key);
      if (key_texture == albedo.texture_of_key.end() || albedo.slice_of_texture.count(key_texture->second) == 0u) continue;
      ready_keys.insert(key);
    }
    for (const auto& [source_id, source] : g_pool.alpha_sources) {
      AlbedoSourceJob job;
      job.source = source_id;
      job.texture = source.texture;
      job.proxy = source.proxy;
      job.format = source.format;
      if (source.texture_dead) dead_textures.insert(source.texture);
      for (const uint64_t key : source.keys) {
        const auto mesh_it = g_pool.mesh_by_key.find(key);
        if (mesh_it == g_pool.mesh_by_key.end() || mesh_it->second >= g_pool.meshes.size()) continue;
        const WorldMesh& mesh = g_pool.meshes[mesh_it->second];
        if (ready_uids.count(mesh.uid) == 0u || data->slot_by_uid.count(mesh.uid) == 0u) continue;
        ready_keys.insert(key);
        if (source.proxy.handle == 0u) continue;  // its copy is being made (outside the lock)
        job.keys.push_back({key, {mesh.alpha_state.material.scroll[0], mesh.alpha_state.material.scroll[1]},
                            mesh.alpha_state.material.swizzle});
      }
      if (!job.keys.empty()) sources.push_back(job);
    }
    // An albedo-only copy whose keys never reach a ready static mesh holds a source budget slot forever.
    // Release it through the same unlink/free path as after a blit and give its keys a fresh retry stamp.
    for (auto it = g_pool.alpha_sources.begin(); it != g_pool.alpha_sources.end();) {
      PoolAlphaSource& source = it->second;
      if (!source.albedo_only || source.proxy.handle == 0u) {
        ++it;
        continue;
      }
      bool any_ready = false;
      for (const uint64_t key : source.keys) {
        const auto mesh_it = g_pool.mesh_by_key.find(key);
        if (mesh_it == g_pool.mesh_by_key.end() || mesh_it->second >= g_pool.meshes.size()) continue;
        const WorldMesh& mesh = g_pool.meshes[mesh_it->second];
        if (ready_uids.count(mesh.uid) != 0u && data->slot_by_uid.count(mesh.uid) != 0u) {
          any_ready = true;
          break;
        }
      }
      if (any_ready) {
        source.unready_since = 0u;
        ++it;
        continue;
      }
      if (source.unready_since == 0u) source.unready_since = frame;
      if (frame - source.unready_since < kAlbedoUnreadyFrames) {
        ++it;
        continue;
      }
      for (const uint64_t key : source.keys) g_pool.albedo_retry_at[key] = frame + kAlbedoRetryFrames;
      g_pool.stats.albedo_unready_freed += 1u;
      free_albedo_only_source(it->first, source);
      it = g_pool.alpha_sources.erase(it);
    }
    g_pool.albedo_sliced_textures.clear();
    for (const auto& entry : albedo.slice_of_texture) {
      if (dead_textures.count(entry.first) != 0u) continue;
      g_pool.albedo_sliced_textures.insert(entry.first);
    }
    for (auto it = g_pool.albedo_key_texture.begin(); it != g_pool.albedo_key_texture.end();) {
      if (dead_textures.count(it->second) != 0u) {
        it = g_pool.albedo_key_texture.erase(it);
      } else {
        ++it;
      }
    }
    PoolAlbedoGpuStats& gpu = g_pool.albedo_gpu;
    gpu.sources = static_cast<uint32_t>(albedo.slice_of_texture.size());
    g_pool.stats.albedo_live_sources = g_pool.albedo_live_sources;
    g_pool.stats.albedo_textures_sliced = static_cast<uint64_t>(albedo.slice_of_texture.size());
    gpu.slices_used = albedo.slices.used;
    gpu.materials_used = albedo.material_table.used;
    gpu.keys_mapped = static_cast<uint32_t>(albedo.material_of_key.size());
    gpu.cap_refused_slices = albedo.cap_refused_slices;
    gpu.cap_refused_materials = albedo.cap_refused_materials;
    gpu.copies = g_pool.stats.albedo_source_copies;
    gpu.refused = g_pool.stats.albedo_source_refused;
    gpu.tlas_albedo_instances = data->live.tlas_albedo_instances;
  }

  // A source whose GPU objects cannot be made keeps its copy (still in the map): it is retried at the next present.
  bool can_blit = true;
  for (const AlbedoSourceJob& job : sources) {
    if (albedo.slice_of_texture.count(job.texture) == 0u) {
      can_blit = EnsureAlbedoGpu(device, data);
      break;
    }
  }
  uint32_t pending = 0u;
  if (can_blit) {
    for (AlbedoSourceJob& job : sources) {
      if (albedo.slice_of_texture.count(job.texture) != 0u || dead_textures.count(job.texture) != 0u) continue;
      if (pending >= kAlphaBlitsPerFrame) break;
      const int32_t slot = albedo.slices.Acquire(job.texture);
      if (slot < 0) {
        albedo.cap_refused_slices += 1u;
        continue;
      }
      job.slice = static_cast<uint32_t>(slot);
      job.blit = true;
      albedo.slice_of_texture[job.texture] = job.slice;
      pending += 1u;
    }
  }

  bool mappings_changed = false;
  for (const AlbedoSourceJob& job : sources) {
    const auto slice_it = albedo.slice_of_texture.find(job.texture);
    if (slice_it == albedo.slice_of_texture.end()) continue;
    for (const AlbedoKeyJob& key : job.keys) {
      const auto mapped = albedo.material_of_key.find(key.key);
      if (mapped != albedo.material_of_key.end()) {
        // The slice of a key comes from its source's texture, which outlives the source copy.
        const auto key_texture = albedo.texture_of_key.find(key.key);
        uint32_t mapped_slice = UINT32_MAX;
        if (key_texture != albedo.texture_of_key.end()) {
          const auto mapped_slice_it = albedo.slice_of_texture.find(key_texture->second);
          if (mapped_slice_it != albedo.slice_of_texture.end()) mapped_slice = mapped_slice_it->second;
        }
        const AlbedoMaterialGPU& existing = albedo.material_table.entries[mapped->second].gpu;
        if (existing.slice == mapped_slice && existing.scroll[0] == key.scroll[0] && existing.scroll[1] == key.scroll[1]
            && existing.swizzle == key.swizzle) continue;
        albedo.material_table.Release(mapped->second);
        albedo.material_of_key.erase(mapped);
        if (key_texture != albedo.texture_of_key.end()) albedo.texture_of_key.erase(key_texture);
        mappings_changed = true;
      }
      const int32_t index = albedo.material_table.FindOrInsert(slice_it->second, key.scroll, key.swizzle);
      if (index < 0) {
        albedo.cap_refused_materials += 1u;
        continue;
      }
      albedo.material_of_key[key.key] = static_cast<uint32_t>(index);
      albedo.texture_of_key[key.key] = job.texture;
      rt12::MirrorAlbedoMaterial(static_cast<uint32_t>(index), albedo.material_table.entries[index].gpu);
      mappings_changed = true;
    }
  }
  for (auto it = albedo.material_of_key.begin(); it != albedo.material_of_key.end();) {
    if (ready_keys.count(it->first) != 0u) {
      ++it;
      continue;
    }
    albedo.material_table.Release(it->second);
    albedo.texture_of_key.erase(it->first);
    it = albedo.material_of_key.erase(it);
    mappings_changed = true;
  }
  std::vector<uint32_t> mirrored_materials;
  {
    // Keys mapped without a copy (their texture already has a slice) join the same material table and mirror.
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    for (auto it = g_pool.albedo_key_texture.begin(); it != g_pool.albedo_key_texture.end();) {
      const auto mesh_it = g_pool.mesh_by_key.find(it->first);
      if (mesh_it == g_pool.mesh_by_key.end() || mesh_it->second >= g_pool.meshes.size()) {
        it = g_pool.albedo_key_texture.erase(it);
        continue;
      }
      const WorldMesh& mesh = g_pool.meshes[mesh_it->second];
      if (ready_uids.count(mesh.uid) == 0u || data->slot_by_uid.count(mesh.uid) == 0u) {
        ++it;
        continue;
      }
      if (albedo.material_of_key.count(it->first) != 0u) {
        it = g_pool.albedo_key_texture.erase(it);
        continue;
      }
      const auto slice_it = albedo.slice_of_texture.find(it->second);
      if (slice_it == albedo.slice_of_texture.end()) {
        ++it;
        continue;
      }
      const int32_t index = albedo.material_table.FindOrInsert(slice_it->second, mesh.alpha_state.material.scroll,
                                                               mesh.alpha_state.material.swizzle);
      if (index < 0) {
        albedo.cap_refused_materials += 1u;
        ++it;
        continue;
      }
      albedo.material_of_key[it->first] = static_cast<uint32_t>(index);
      albedo.texture_of_key[it->first] = it->second;
      mirrored_materials.push_back(static_cast<uint32_t>(index));
      mappings_changed = true;
      it = g_pool.albedo_key_texture.erase(it);
    }
  }
  for (const uint32_t index : mirrored_materials) {
    rt12::MirrorAlbedoMaterial(index, albedo.material_table.entries[index].gpu);
  }
  for (uint64_t texture : dead_textures) {
    const auto slice_it = albedo.slice_of_texture.find(texture);
    if (slice_it == albedo.slice_of_texture.end()) continue;
    for (auto it = albedo.material_of_key.begin(); it != albedo.material_of_key.end();) {
      const auto key_texture = albedo.texture_of_key.find(it->first);
      if (key_texture == albedo.texture_of_key.end() || key_texture->second != texture) {
        ++it;
        continue;
      }
      albedo.material_table.Release(it->second);
      albedo.texture_of_key.erase(key_texture);
      it = albedo.material_of_key.erase(it);
      mappings_changed = true;
    }
    albedo.quarantine.emplace_back(slice_it->second, frame);
    albedo.slice_of_texture.erase(slice_it);
    mappings_changed = true;
  }
  std::unordered_set<uint32_t> slices_in_use;
  for (const auto& entry : albedo.material_of_key) {
    slices_in_use.insert(albedo.material_table.entries[entry.second].gpu.slice);
  }
  for (auto it = albedo.slice_of_texture.begin(); it != albedo.slice_of_texture.end();) {
    if (slices_in_use.count(it->second) != 0u) {
      ++it;
      continue;
    }
    albedo.quarantine.emplace_back(it->second, frame);
    it = albedo.slice_of_texture.erase(it);
    mappings_changed = true;
  }
  for (auto it = albedo.quarantine.begin(); it != albedo.quarantine.end();) {
    if (frame >= it->second + kAlphaSliceQuarantineFrames) {
      albedo.slices.Release(it->first);
      it = albedo.quarantine.erase(it);
    } else {
      ++it;
    }
  }
  {
    // The retry gate runs on draw threads under the pool lock; publish the mapped keys under that lock.
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    g_pool.albedo_material_keys.clear();
    for (const auto& [key, index] : albedo.material_of_key) g_pool.albedo_material_keys.insert(key);
    g_pool.stats.albedo_textures_sliced = static_cast<uint64_t>(albedo.slice_of_texture.size());
  }
  if (mappings_changed) data->store_version += 1u;
  {
    // A copy made for a texture that already has a slice never blits; its slot is free once its keys are mapped.
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    for (const AlbedoSourceJob& job : sources) {
      if (job.blit || albedo.slice_of_texture.count(job.texture) == 0u) continue;
      const auto source_it = g_pool.alpha_sources.find(job.source);
      if (source_it == g_pool.alpha_sources.end() || !source_it->second.albedo_only) continue;
      free_albedo_only_source(job.source, source_it->second);
      g_pool.stats.albedo_sliced_copies_freed += 1u;
      g_pool.alpha_sources.erase(source_it);
    }
  }
  for (const reshade::api::resource proxy : stale) device->destroy_resource(proxy);
  stale.clear();
  if (pending == 0u) return;

  using RU = reshade::api::resource_usage;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  BeginGpuTimer(device, cmd_list, &albedo.timer);
  uint32_t blits = 0u;
  for (const AlbedoSourceJob& job : sources) {
    if (!job.blit) continue;
    const uint32_t slice = job.slice;
    std::string error;
    bool ok = true;
    for (const AlbedoKeyJob& key : job.keys) {
      const auto mapped = albedo.material_of_key.find(key.key);
      if (mapped == albedo.material_of_key.end()) continue;
      const uint32_t index = mapped->second;
      const AlbedoMaterialGPU& material = albedo.material_table.entries[index].gpu;
      if (!WriteBufferRange(device, cmd_list, albedo.materials, sizeof(AlbedoMaterialGPU),
                            static_cast<uint64_t>(index) * sizeof(AlbedoMaterialGPU), &material, sizeof(material), &error)) {
        ok = false;
      }
    }
    reshade::api::resource_view source = {0u};
    if (!device->create_resource_view(job.proxy, RU::shader_resource, reshade::api::resource_view_desc(job.format), &source)) {
      ok = false;
    }
    if (ok) {
      const reshade::api::descriptor_table_update source_update = {albedo.blit_tables[0], 0, 0, 1, DT::shader_resource_view, &source};
      device->update_descriptor_tables(1, &source_update);
      cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, albedo.blit_pipeline);
      cmd_list->bind_descriptor_tables(DS::all_compute, albedo.blit_layout, 0, 3, albedo.blit_tables);
      cmd_list->push_constants(DS::all_compute, albedo.blit_layout, 3, 0, 1, &slice);
      cmd_list->dispatch(kAlbedoSliceSize / 8u, kAlbedoSliceSize / 8u, 1u);
    }
    if (source.handle != 0u) device->destroy_resource_view(source);
    if (!ok) {
      albedo.slices.Release(slice);
      const auto failed_slice = albedo.slice_of_texture.find(job.texture);
      if (failed_slice != albedo.slice_of_texture.end() && failed_slice->second == slice) {
        albedo.slice_of_texture.erase(failed_slice);
      }
      for (auto it = albedo.material_of_key.begin(); it != albedo.material_of_key.end();) {
        if (albedo.material_table.entries[it->second].gpu.slice != slice) {
          ++it;
          continue;
        }
        albedo.material_table.Release(it->second);
        albedo.texture_of_key.erase(it->first);
        it = albedo.material_of_key.erase(it);
        mappings_changed = true;
      }
      {
        std::lock_guard<std::mutex> lock(g_pool.mutex);
        g_pool.albedo_material_keys.clear();
        for (const auto& [key, index] : albedo.material_of_key) g_pool.albedo_material_keys.insert(key);
        g_pool.stats.albedo_textures_sliced = static_cast<uint64_t>(albedo.slice_of_texture.size());
      }
      renodx::utils::log::w("[world-bvh] albedo atlas: slice ", slice, " for source ", job.source, " not written: ", error);
      continue;
    }
    {
      // The slice is keyed by the game texture and outlives this copy. Free an albedo-only proxy through
      // the same unlink/dead-list path as SyncLiveAlpha, so its budget slot is available again.
      std::lock_guard<std::mutex> lock(g_pool.mutex);
      const auto source_it = g_pool.alpha_sources.find(job.source);
      if (source_it != g_pool.alpha_sources.end() && source_it->second.proxy.handle == job.proxy.handle
          && source_it->second.albedo_only) {
        free_albedo_only_source(job.source, source_it->second);
        g_pool.alpha_sources.erase(source_it);
      }
    }
    blits += 1u;
  }
  EndGpuTimer(cmd_list, &albedo.timer);
  if (mappings_changed) data->store_version += 1u;
  if (blits != 0u) {
    // Unbind the atlas UAV at u0: the PT binds it as SRV t58, and D3D11 nulls an SRV still bound as a UAV.
    const reshade::api::resource_view null_uav = {0u};
    const reshade::api::descriptor_table_update unbind = {albedo.blit_tables[2], 0, 0, 1, DT::unordered_access_view, &null_uav};
    device->update_descriptor_tables(1, &unbind);
    cmd_list->bind_descriptor_tables(DS::all_compute, albedo.blit_layout, 2, 1, &albedo.blit_tables[2]);
    const reshade::api::resource atlas = albedo.atlas;
    const RU before = RU::unordered_access;
    const RU after = RU::shader_resource;
    cmd_list->barrier(1, &atlas, &before, &after);
    data->store_version += 1u;
  }
  albedo.blits += blits;
  albedo.blits_frame = blits;
  for (const reshade::api::resource proxy : stale) device->destroy_resource(proxy);
}

struct AlphaBlitJob {
  uint64_t source = 0u;  // source id of the copy (alpha_sources)
  uint64_t uid = 0u;     // mesh uid that gets the slice
  reshade::api::resource proxy = {0u};
  reshade::api::format format = reshade::api::format::unknown;
  PoolAlphaMaterial material;
  bool indirect = false;  // the key is an indirect key (alpha_indirect_keys): the slice is dropped with the switch OFF
};

// Fills the atlas for the alpha meshes of the store and keeps slot_of_uid (the TLAS reads it). Runs on the
// present thread: the pool is read under its lock, and every graphics call runs after the lock is released.
inline void SyncLiveAlpha(reshade::api::device* device, reshade::api::command_list* cmd_list, BvhDeviceData* data, uint32_t frame) {
  const PoolCpuTimer cpu_timer{&g_perf_gate.alpha_sync_us};
  AlphaGpu& alpha = data->alpha;
  const bool on = g_pool.alpha_foliage.load(std::memory_order_relaxed);
  const bool indirect_on = g_pool.alpha_indirect_source.load(std::memory_order_relaxed);
  {
    // The numbers of the previous present: the blit timer resolves a frame late.
    std::lock_guard<std::mutex> lock(g_pool.mutex);
    if (!on) {
      g_pool.alpha_sync_logged = false;
      g_pool.alpha_maps = PoolAlphaDumpMaps{};  // alpha OFF: the dump shows no atlas slices
    } else if (!g_pool.alpha_sync_logged) {
      g_pool.alpha_sync_logged = true;
      renodx::utils::log::i("[world-bvh] alpha stage: first SyncLiveAlpha since alpha foliage was switched on");
    }
    PoolAlphaGpuStats& gpu = g_pool.alpha_gpu;
    PoolAlphaDumpMaps& maps = g_pool.alpha_maps;
    if (on && maps.store_version != data->store_version) {  // the dump's copies of the atlas maps: at each store change, alpha on
      maps.store_version = data->store_version;
      maps.slice_of_uid = alpha.slot_of_uid;
      maps.indirect_uids = alpha.indirect_uids;
      maps.resident_uids.clear();
      for (const auto& slot : data->slot_by_uid) maps.resident_uids.insert(slot.first);
    }
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
  for (const reshade::api::resource proxy : TakePoolAlphaProxies(!on && pt::g_pt_material_atlas <= 0.5f)) {
    device->destroy_resource(proxy);
  }
  if (!on) {
    DestroyAlphaGpu(device, data);
    SyncLiveAlbedo(device, cmd_list, data, frame);
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
      if (!wanted && source.keys.empty() && source.hold_until != 0u) {  // keyless: held for its indirect draw's resolve
        if (frame <= source.hold_until) {
          ++it;
          continue;
        }
        g_pool.stats.alpha_orphans_expired += 1u;
      }
      if (!wanted) {  // no key uses the copy: freed; an unblitted copy is not copied again until its key is drawn again
        stale.push_back(source.proxy);
        for (const uint64_t key : source.keys) {
          g_pool.alpha_key_source.erase(key);
          if (!source.blitted) g_pool.alpha_source_done.insert(key);
        }
        UnlinkPoolAlphaTexture(it->first, source);
        ReleasePoolAlbedoBudget(source);
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
        jobs.push_back({it->first, mesh.uid, source.proxy, source.format, mesh.alpha_state.material,
                        g_pool.alpha_indirect_keys.count(key) != 0u});
      }
      ++it;
    }
  }
  for (auto it = alpha.slot_of_uid.begin(); it != alpha.slot_of_uid.end();) {
    // With the indirect source switch OFF, a slice filled from an indirect key is dropped (the mesh waits again).
    const bool dropped = !indirect_on && alpha.indirect_uids.count(it->first) != 0u;
    if (ready_uids.count(it->first) != 0u && !dropped) {
      ++it;
      continue;
    }
    alpha.quarantine.emplace_back(it->second, frame);
    alpha.indirect_uids.erase(it->first);
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
      rt12::MirrorMaterial(slice, &material);
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
    if (job.indirect) alpha.indirect_uids.insert(job.uid);
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
  SyncLiveAlbedo(device, cmd_list, data, frame);
}
}  // namespace falcom_world::bvh
