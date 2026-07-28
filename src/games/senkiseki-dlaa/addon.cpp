/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Senkiseki3 DLAA — Replaces FXAA with NVIDIA DLAA (NGX SDK).
 * Features: camera jitter via VS injection, per-object motion vectors,
 *           depth-reprojection velocity compute, debug visualization.
 */

#define ImTextureID ImU64
#define DEBUG_LEVEL_0

#include <array>
#include <cstdint>
#include <string>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>
#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/settings.hpp"
#include "./shared.h"
#include "./dlss/dlss.hpp"

namespace {

ShaderInjectData shader_injection = {
    .dlaa_enabled = 0.f, .dlaa_preset = 0.f,
    .dlaa_jitter_enabled = 0.f, .dlaa_per_object_motion = 0.f,
    .dlaa_velocity_scale = 1.f, .dlaa_debug_view = 0.f,
    .dlaa_debug_logging = 0.f,
    .jitter_offset_x = 0.f, .jitter_offset_y = 0.f,
};

// ── Descriptor table helpers ──
constexpr uint32_t kTableParamCount = 4u;
using DTSet = std::array<reshade::api::descriptor_table, kTableParamCount>;

static void FreeTables(reshade::api::device* dev, DTSet& t) {
  for (auto& tb : t) { if (tb.handle) { dev->free_descriptor_table(tb); tb = {}; } }
}

// ── Per-device state ──
struct __declspec(uuid("b1a2c3d4-e5f6-7890-abcd-ef1234567890")) DeviceData {
  reshade::api::resource velocity_texture = {};
  reshade::api::resource_view velocity_srv = {};
  reshade::api::resource_view velocity_uav = {};
  reshade::api::pipeline_layout velocity_layout = {};
  reshade::api::pipeline velocity_pipeline = {};
  DTSet velocity_tables = {};

  reshade::api::resource_view captured_scene_cbv = {};  // _Globals cbuffer at b0
  reshade::api::resource_view captured_depth_srv = {};
  reshade::api::resource captured_depth_res = {};
  reshade::api::resource_view captured_color_srv = {};
  reshade::api::resource captured_color_res = {};

  std::array<float, 16> prev_view_proj = {};
  std::array<float, 16> curr_view_proj = {};
  float viewport_w = 2560.f, viewport_h = 1440.f;
  float jitter_x = 0.f, jitter_y = 0.f;
  uint32_t frame_index = 0u;
  bool resources_created = false;

  reshade::api::sampler point_sampler = {};
};

// ── Halton jitter ──
static float Halton(uint32_t n, uint32_t base) {
  float r = 0.f, inv = 1.f / (float)base, f = 1.f;
  while (n) { f *= inv; r += f * (float)(n % base); n /= base; }
  return r;
}

// ── Destroy ──
static void Destroy(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d) return;
  auto dr = [&](reshade::api::resource& r) { if(r.handle) dev->destroy_resource(r); r={}; };
  auto dv = [&](reshade::api::resource_view& v) { if(v.handle) dev->destroy_resource_view(v); v={}; };
  auto dp = [&](reshade::api::pipeline& p) { if(p.handle) dev->destroy_pipeline(p); p={}; };
  auto dl = [&](reshade::api::pipeline_layout& l) { if(l.handle) dev->destroy_pipeline_layout(l); l={}; };

  dv(d->velocity_srv); dv(d->velocity_uav); dr(d->velocity_texture);
  dp(d->velocity_pipeline); dl(d->velocity_layout); FreeTables(dev, d->velocity_tables);
  dv(d->captured_scene_cbv); dv(d->captured_depth_srv); dv(d->captured_color_srv);
  d->captured_depth_res = {}; d->captured_color_res = {};
  if (d->point_sampler.handle) dev->destroy_sampler(d->point_sampler);
  d->point_sampler = {};
  d->resources_created = false;
}

// ── Create velocity pipeline ──
static bool CreateVelocityPipeline(reshade::api::device* dev, DeviceData* d) {
  if (!dev || !d || d->velocity_pipeline.handle) return true;

  reshade::api::sampler_desc sd = {};
  sd.filter = reshade::api::filter_mode::min_mag_mip_point;
  sd.address_u = sd.address_v = sd.address_w = reshade::api::texture_address_mode::clamp;
  dev->create_sampler(sd, &d->point_sampler);

  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  using DR = reshade::api::descriptor_range;
  using P  = reshade::api::pipeline_layout_param;
  using PT = reshade::api::pipeline_layout_param_type;

  DR sampler_r = {0,0,0,1, DS::all_compute, 1, DT::sampler};
  DR cbv_r     = {0,0,0,1, DS::all_compute, 1, DT::constant_buffer};
  DR srv_r     = {0,0,0,1, DS::all_compute, 1, DT::texture_shader_resource_view};
  DR uav_r     = {0,0,0,1, DS::all_compute, 1, DT::texture_unordered_access_view};
  reshade::api::constant_range pc_range = {};
  pc_range.binding = 0;
  pc_range.dx_register_index = 13;
  pc_range.count = 32;
  pc_range.visibility = DS::all_compute;

  P params[5];
  params[0].type = PT::descriptor_table; params[0].descriptor_table = {1, &sampler_r};
  params[1].type = PT::descriptor_table; params[1].descriptor_table = {1, &cbv_r};
  params[2].type = PT::descriptor_table; params[2].descriptor_table = {1, &srv_r};
  params[3].type = PT::descriptor_table; params[3].descriptor_table = {1, &uav_r};
  params[4].type = PT::push_constants;   params[4].push_constants = pc_range;

  if (!dev->create_pipeline_layout(5, params, &d->velocity_layout)) return false;
  for (uint32_t i = 0; i < 4u; ++i)
    if (!dev->allocate_descriptor_table(d->velocity_layout, i, &d->velocity_tables[i])) return false;

  reshade::api::shader_desc cs_desc = {};
  cs_desc.code = __motion_velocity.data();
  cs_desc.code_size = __motion_velocity.size();
  cs_desc.entry_point = "main";
  reshade::api::pipeline_subobject so = {reshade::api::pipeline_subobject_type::compute_shader, 1, &cs_desc};
  return dev->create_pipeline(d->velocity_layout, 1, &so, &d->velocity_pipeline);
}

// ── Event: capture scene CBV (b0 _Globals) from push_descriptors ──
static void OnPushDescriptorsCaptureCBV(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage, reshade::api::pipeline_layout,
    uint32_t param_index, const reshade::api::descriptor_table_update& update) {
  if (param_index != 1) return;
  const auto* updates = &update;
  auto* dev = cmd_list->get_device();
  if (!dev) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d || d->captured_scene_cbv.handle) return;

  if (updates[0].type == reshade::api::descriptor_type::constant_buffer) {
    auto* cbv = static_cast<const reshade::api::buffer_range*>(updates[0].descriptors);
    if (cbv && cbv->buffer.handle) {
      reshade::api::resource_view_desc vd(reshade::api::resource_view_type::buffer, reshade::api::format::unknown,
                                          cbv->offset, cbv->size);
      if (dev->create_resource_view(cbv->buffer, reshade::api::resource_usage::shader_resource, vd,
                                    &d->captured_scene_cbv)) {
        reshade::log::message(reshade::log::level::info, "[DLAA] Scene CBV captured");
      }
    }
  }
  static int cbv_log = 0;
  if (++cbv_log <= 5) reshade::log::message(reshade::log::level::info,
    (std::string("[DLAA] CBV probe param=") + std::to_string(param_index) +
     " type=" + std::to_string((int)update.type)).c_str());
}

// ── Event: capture depth SRV from push_descriptors ──
static void OnPushDescriptorsCaptureDepth(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage, reshade::api::pipeline_layout,
    uint32_t param_index, const reshade::api::descriptor_table_update& update) {
  if (param_index != 2) return;
  const auto* updates = &update;
  auto* dev = cmd_list->get_device();
  if (!dev) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d || d->captured_depth_srv.handle) return;

  const auto& u = updates[0];
  if (u.type == reshade::api::descriptor_type::texture_shader_resource_view && u.count >= 1 && u.descriptors) {
    auto* srvs = static_cast<const reshade::api::resource_view*>(u.descriptors);
    auto res = dev->get_resource_from_view(srvs[0]);
    auto rd = dev->get_resource_desc(res);
    if (rd.type == reshade::api::resource_type::texture_2d) {
      d->captured_depth_srv = srvs[0];
      d->captured_depth_res = res;
      d->viewport_w = (float)rd.texture.width;
      d->viewport_h = (float)rd.texture.height;
      reshade::log::message(reshade::log::level::info,
        (std::string("[DLAA] Depth captured: ") + std::to_string(rd.texture.width) + "x" +
         std::to_string(rd.texture.height)).c_str());
    }
  }
  static int dlog = 0;
  if (++dlog <= 5) reshade::log::message(reshade::log::level::info,
    (std::string("[DLAA] Depth probe param=") + std::to_string(param_index) +
     " type=" + std::to_string((int)update.type) + " count=" + std::to_string(update.count)).c_str());
}

// ── Event: capture color SRV from FXAA T0 ──
static void OnPushDescriptorsCaptureColor(
    reshade::api::command_list* cmd_list, reshade::api::shader_stage, reshade::api::pipeline_layout,
    uint32_t param_index, const reshade::api::descriptor_table_update& update) {
  if (param_index != 2) return;
  const auto* updates = &update;
  auto* dev = cmd_list->get_device();
  if (!dev) return;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d || d->captured_color_srv.handle) return;

  const auto& u = updates[0];
  if (u.type == reshade::api::descriptor_type::texture_shader_resource_view && u.count >= 1 && u.descriptors) {
    auto* srvs = static_cast<const reshade::api::resource_view*>(u.descriptors);
    d->captured_color_srv = srvs[0];
    d->captured_color_res = dev->get_resource_from_view(srvs[0]);
    reshade::log::message(reshade::log::level::info, "[DLAA] Color captured from FXAA T0");
  }
  static int clog = 0;
  if (++clog <= 5) reshade::log::message(reshade::log::level::info,
    (std::string("[DLAA] Color probe param=") + std::to_string(param_index) +
     " type=" + std::to_string((int)update.type)).c_str());
}

// ── Jitter update ──
static void UpdateJitter(DeviceData* d) {
  if (!d || shader_injection.dlaa_jitter_enabled < 0.5f) {
    d->jitter_x = 0.f; d->jitter_y = 0.f;
    shader_injection.jitter_offset_x = 0.f;
    shader_injection.jitter_offset_y = 0.f;
    return;
  }
  uint32_t f = d->frame_index;
  float jx = (Halton(f + 1u, 2u) - 0.5f) * 2.f / d->viewport_w;
  float jy = (Halton(f + 1u, 3u) - 0.5f) * 2.f / d->viewport_h;
  d->jitter_x = jx; d->jitter_y = jy;
  shader_injection.jitter_offset_x = jx;
  shader_injection.jitter_offset_y = jy;
}

// ── Velocity push constants ──
static std::array<float, 32> BuildVelocityPC(DeviceData* d) {
  std::array<float, 32> c = {};
  if (!d) return c;
  memcpy(&c[0],  d->prev_view_proj.data(), 64);
  memcpy(&c[16], d->curr_view_proj.data(), 64);
  c[24] = d->viewport_w;  c[25] = d->viewport_h;
  c[26] = shader_injection.dlaa_velocity_scale;
  c[27] = shader_injection.dlaa_debug_view;
  c[28] = d->jitter_x;    c[29] = d->jitter_y;
  return c;
}

// ── DLAA dispatch ──
static bool RunDLAA(reshade::api::command_list* cmd_list) {
  auto* dev = cmd_list->get_device();
  if (!dev) return false;
  auto* d = dev->get_private_data<DeviceData>();
  if (!d) return false;

  if (!d->captured_depth_srv.handle || !d->captured_scene_cbv.handle || !d->captured_color_srv.handle) {
    std::string miss = "[DLAA] Missing:";
    if (!d->captured_depth_srv.handle) miss += " depth";
    if (!d->captured_scene_cbv.handle) miss += " cbv";
    if (!d->captured_color_srv.handle) miss += " color";
    miss += " — skipping";
    reshade::log::message(reshade::log::level::warning, miss.c_str());
    return false;
  }

  uint32_t w = (uint32_t)d->viewport_w, h = (uint32_t)d->viewport_h;
  if (w < 64u) w = 64u; if (h < 64u) h = 64u;

  if (!d->velocity_texture.handle) {
    reshade::api::resource_desc rd = {};
    rd.type = reshade::api::resource_type::texture_2d;
    rd.texture = {w, h, 1, 1, reshade::api::format::r16g16_float, 1};
    rd.heap = reshade::api::memory_heap::gpu_only;
    rd.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::unordered_access;
    dev->create_resource(rd, nullptr, reshade::api::resource_usage::shader_resource, &d->velocity_texture);
    reshade::api::resource_view_desc vd(reshade::api::resource_view_type::texture_2d, reshade::api::format::r16g16_float, 0,1,0,1);
    dev->create_resource_view(d->velocity_texture, reshade::api::resource_usage::shader_resource, vd, &d->velocity_srv);
    dev->create_resource_view(d->velocity_texture, reshade::api::resource_usage::unordered_access, vd, &d->velocity_uav);
    d->resources_created = true;
    if (shader_injection.dlaa_debug_logging > 0.5f)
      reshade::log::message(reshade::log::level::info, (std::string("[DLAA] Velocity texture created: ") + std::to_string(w) + "x" + std::to_string(h)).c_str());
  }

  if (!d->velocity_pipeline.handle && !CreateVelocityPipeline(dev, d)) return false;

  const auto UA = reshade::api::resource_usage::unordered_access;
  const auto SR = reshade::api::resource_usage::shader_resource;
  const auto CS = reshade::api::shader_stage::all_compute;
  const auto AC = reshade::api::pipeline_stage::all_compute;

  UpdateJitter(d);

  // Pass: Velocity compute
  cmd_list->bind_pipeline(AC, d->velocity_pipeline);

  reshade::api::descriptor_table_update u[4] = {
    { d->velocity_tables[0], 0, 0, 1, reshade::api::descriptor_type::sampler, &d->point_sampler },
    { d->velocity_tables[1], 0, 0, 1, reshade::api::descriptor_type::constant_buffer, &d->captured_scene_cbv },
    { d->velocity_tables[2], 0, 0, 1, reshade::api::descriptor_type::texture_shader_resource_view, &d->captured_depth_srv },
    { d->velocity_tables[3], 0, 0, 1, reshade::api::descriptor_type::texture_unordered_access_view, &d->velocity_uav },
  };
  dev->update_descriptor_tables(4, u);

  reshade::api::descriptor_table tables[4] = { d->velocity_tables[0], d->velocity_tables[1], d->velocity_tables[2], d->velocity_tables[3] };
  cmd_list->bind_descriptor_tables(CS, d->velocity_layout, 0, 4, tables);

  auto pc = BuildVelocityPC(d);
  cmd_list->push_constants(CS, d->velocity_layout, 4, 0, 32, pc.data());
  cmd_list->dispatch((w + 7) / 8, (h + 7) / 8, 1);
  cmd_list->barrier(d->velocity_texture, UA, SR);

  // DLAA evaluate (when resources are available)
  if (senkiseki3::dlss::dlss_initialized && d->captured_color_res.handle) {
    auto* cl = reinterpret_cast<ID3D12GraphicsCommandList*>(cmd_list->get_native());
    auto* src = reinterpret_cast<ID3D12Resource*>(d->captured_color_res.handle);
    auto* dst = src;
    auto* mv  = reinterpret_cast<ID3D12Resource*>(d->velocity_texture.handle);
    auto* dep = reinterpret_cast<ID3D12Resource*>(d->captured_depth_res.handle);
    if (cl && src && mv)
      senkiseki3::dlss::EvaluateDLSS(cl, src, dst, mv, dep, d->jitter_x, d->jitter_y, 1.f, 1.f);
  }

  d->frame_index++;
  return true;
}

// ── Settings ──
renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "DLAAEnabled", .binding = &shader_injection.dlaa_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "DLAA", .section = "Antialiasing",
        .tooltip = "Replace FXAA with NVIDIA DLAA. Requires nvngx_dlss.dll.", .labels = {"FXAA", "DLAA"},
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPreset", .binding = &shader_injection.dlaa_preset,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f, .label = "DLSS Preset", .section = "Antialiasing",
        .labels = {"Default","F-CNN","J-T1","K-T1","L-T2","M-T2"},
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAJitter", .binding = &shader_injection.dlaa_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Camera Jitter", .section = "Antialiasing",
        .labels = {"Off","On"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAPerObjectMotion", .binding = &shader_injection.dlaa_per_object_motion,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Per-Object Motion", .section = "Antialiasing",
        .labels = {"Camera Only","Per-Object"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAAVelocityScale", .binding = &shader_injection.dlaa_velocity_scale,
        .value_type = renodx::utils::settings::SettingValueType::FLOAT,
        .default_value = 1.f, .label = "Velocity Scale", .section = "Antialiasing",
        .min = 0.1f, .max = 5.f, .format = "%.2f",
        .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAADebugView", .binding = &shader_injection.dlaa_debug_view,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "MV Debug", .section = "Antialiasing",
        .labels = {"Off","Velocity"}, .is_enabled = []{ return shader_injection.dlaa_enabled > 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLAADebugLogging", .binding = &shader_injection.dlaa_debug_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f, .label = "Debug Logging", .section = "Antialiasing",
        .labels = {"Off","On"},
    },
};

// ── Draw hook: FXAA replacement ──
// Called when the game is about to draw with the FXAA shader (0x96BB8CFF).
// We intercept and instead run the velocity compute + DLAA evaluate.
static bool OnBeforeFxaaDraw(reshade::api::command_list* cmd_list) {
  if (shader_injection.dlaa_enabled < 0.5f) return true;  // let FXAA run normally
  return !RunDLAA(cmd_list);  // if DLAA succeeds, skip original FXAA draw
}

renodx::mods::shader::CustomShaders custom_shaders = {
    {
        0x96BB8CFFu,
        renodx::mods::shader::CustomShader{
            .crc32 = 0x96BB8CFFu,
            .on_draw = OnBeforeFxaaDraw,
        },
    },
#ifdef __ALL_CUSTOM_SHADERS
    __ALL_CUSTOM_SHADERS
#endif
};

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Senkiseki3 DLAA";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION =
    "NVIDIA DLAA for Senkiseki3. Requires nvngx_dlss.dll.";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      renodx::utils::settings::use_presets = false;
      renodx::mods::shader::force_pipeline_cloning = true;
      renodx::mods::shader::allow_multiple_push_constants = true;
      renodx::mods::shader::expected_constant_buffer_index = 13;
      renodx::mods::shader::expected_constant_buffer_space = 0;

      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureCBV);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureDepth);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureColor);

      reshade::register_event<reshade::addon_event::present>(
          [](reshade::api::command_queue*, reshade::api::swapchain* swapchain,
             const reshade::api::rect*, const reshade::api::rect*, uint32_t, const reshade::api::rect*) {
        if (shader_injection.dlaa_enabled < 0.5f) return;
        auto* dev = swapchain->get_device();
        if (!dev) return;
        if (!senkiseki3::dlss::dlss_initialized) {
          auto* nd = reinterpret_cast<ID3D12Device*>(dev->get_native());
          reshade::api::resource_desc bbd = dev->get_resource_desc(swapchain->get_current_back_buffer());
          senkiseki3::dlss::InitDLSS(nd, bbd.texture.width, bbd.texture.height);
        }
        static int fc = 0;
        if (++fc <= 3 || (fc % 300 == 0))
          reshade::log::message(reshade::log::level::info,
            (std::string("[DLAA] Present frame ") + std::to_string(fc)).c_str());
      });

      reshade::log::message(reshade::log::level::info, "[Senkiseki3 DLAA] Addon loaded");

      reshade::register_event<reshade::addon_event::init_device>([](reshade::api::device* dev) {
        dev->create_private_data<DeviceData>();
      });
      reshade::register_event<reshade::addon_event::destroy_device>([](reshade::api::device* dev) {
        auto* d = dev->get_private_data<DeviceData>();
        if (d) { Destroy(dev, d); dev->destroy_private_data<DeviceData>(); }
      });
      break;
    case DLL_PROCESS_DETACH:
      senkiseki3::dlss::ShutdownDLSS();
      reshade::unregister_addon(h_module);
      break;
  }
  renodx::utils::settings::Use(fdw_reason, &settings);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);
  return TRUE;
}
