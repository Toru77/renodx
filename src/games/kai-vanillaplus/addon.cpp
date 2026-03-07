/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <array>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <span>
#include <string>
#include <vector>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../mods/shader.hpp"
#include "../../utils/descriptor.hpp"
#include "../../utils/pipeline_layout.hpp"
#include "../../utils/resource.hpp"
#include "../../utils/settings.hpp"
#include "../../utils/shader.hpp"
#include "../../utils/swapchain.hpp"
#include "./kai-vanillaplus.h"

namespace {

constexpr uint32_t kLightingShader = 0x430ED091u;
constexpr uint32_t kLightingSoftShader = 0xF6C55E5Fu;
constexpr uint32_t kCharacterShader = 0x445A1838u;
constexpr uint32_t kVolFogShader = 0xBD7DFE49u;
constexpr uint32_t kIsFastTextureBinding = 15u;
constexpr uint32_t kIsFastSamplerBinding = 15u;

bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list);
bool OnBeforeVolFogShaderDraw(reshade::api::command_list* cmd_list);
bool BindISFastNoisePixel(reshade::api::command_list* cmd_list);
void OnInitDevice(reshade::api::device* device);
void OnDestroyDevice(reshade::api::device* device);
void ReloadIsFastResources(reshade::api::device* device, const char* reason);

renodx::mods::shader::CustomShaders custom_shaders = {
    {
        kLightingShader,
        {
            .crc32 = kLightingShader,
            .code = __0x430ED091,
            .on_draw = &OnBeforeLightingShaderDraw,
        },
    },                                             // lighting
    {
        kLightingSoftShader,
        {
            .crc32 = kLightingSoftShader,
            .code = __0xF6C55E5F,
            .on_draw = &OnBeforeLightingShaderDraw,
        },
    },                                             // lighting soft shadows
    CustomShaderEntry(0x445A1838),                 // character lighting
    CustomShaderEntry(0x209125C1),                 // SSR
    CustomShaderEntry(0xB1CCBCAE),                 // glass
    CustomShaderEntry(0x1A17A133),                 // glass
    CustomShaderEntry(0xCA715B78),                 // glass
    CustomShaderEntry(0xE1E0ACBB),                 // glass
    CustomShaderEntry(0xF237E72F),                 // glass
    CustomShaderEntry(0x8337B262),                 // floor
    CustomShaderEntry(0x534E54EA),                 // sss source pass
    CustomShaderEntry(0xAB6DBF4D),                 // dof coc
    CustomShaderEntry(0x2734F870),                 // dof blur composite
    {
        kVolFogShader,
        {
            .crc32 = kVolFogShader,
            .code = __0xBD7DFE49,
            .on_draw = &OnBeforeVolFogShaderDraw,
        },
    },  // volumetric fog composite
};

SssInjectData shader_injection = {
    .char_shadow_mode = 2.f,
    .char_shadow_sample_count = 32.f,
    .char_shadow_hard_shadow_samples = 4.f,
    .char_shadow_fade_out_samples = 16.f,
    .char_shadow_surface_thickness = 0.09f,
    .char_shadow_contrast = 9.f,
    .char_shadow_light_screen_fade_start = 0.f,
    .char_shadow_light_screen_fade_end = 0.f,
    .char_shadow_min_occluder_depth_scale = 0.f,
    .char_shadow_jitter_enabled = 1.f,
    .char_shadow_strength = 1.f,
    .shadow_pcss_jitter_enabled = 1.f,
    .shadow_pcss_sample_mode = 1.f,
    .shadow_isfast_jitter_amount = 1.f,
    .shadow_isfast_jitter_speed = 237.f,
    .ssgi_mod_enabled = 1.f,
    .ssgi_color_boost = 1.f,
    .ssgi_alpha_boost = 1.f,
    .ssgi_pow = 1.f,
    .ssr_mode = 1.f,
    .ssr_ray_count_scale = 2.f,
    .ssr_temporal_clamp_enable = 1.f,
    .ssr_temporal_clamp_radius = 1.f,
    .ssr_temporal_clamp_strength = 1.f,
    .ssr_temporal_jitter_enable = 1.f,
    .ssr_temporal_jitter_amount = 1.f,
    .cubemap_improvements_enabled = 1.f,
    .cubemap_lighting_mip_boost = 1.5f,
    .floor_cubemap_mip_scale = 4.f,
    .shadow_base_softness = 0.2f,
    .fog_color_correction_enabled = 1.f,
    .fog_hue = 0.f,
    .fog_chrominance = 0.f,
    .fog_avg_brightness = 0.85f,
    .fog_min_brightness = 0.f,
    .fog_min_chroma_change = 0.f,
    .fog_max_chroma_change = 0.f,
    .fog_lightness_strength = 1.f,
    .fog_color_correction_strength = 0.5f,
    .volfog_tricubic_enabled = 1.f,
    .volfog_is_fast_enabled = 1.f,
    .isfast_noise_bound = 0.f,
    .volfog_color_correction_strength = 0.5f,
    .dof_mode = 1.f,
    .dof_strength = 1.f,
    .dof_radius_scale = 1.33f,
    .dof_sample_count = 24.f,
    .dof_near_scale = 1.f,
    .dof_far_scale = 1.f,
    .dof_coc_curve = 1.f,
    .dof_edge_threshold = 0.25f,
    .foliage_translucency_scale = 1.f,
    .foliage_opacity_scale = 1.f,
    .foliage_ssao_scale = 1.f,
    .foliage_sss_enabled = 1.f,
    .foliage_sss_strength = 1.0f,
    .foliage_sss_sample_count = 24.f,
    .foliage_sss_surface_thickness = 0.005f,
    .foliage_sss_contrast = 2.f,
    .foliage_sss_jitter_enabled = 1.f,
    .foliage_sss_height_enabled = 1.f,
    .foliage_sss_height_min = 0.f,
    .foliage_sss_height_max = 1.f,
    .foliage_sss_height_fade = 0.1f,
    .foliage_sss_vertical_reject = 0.3f,
    .foliage_sss_max_darkening = 0.50f,
    .foliage_sss_bright_reject_threshold = 0.19f,
    .foliage_sss_bright_reject_fade = 0.5f,
    .char_gi_enabled = 1.f,
    .char_gi_strength = 3.0f,
    .char_gi_alpha_scale = 1.0f,
    .char_gi_chroma_strength = 0.50f,
    .char_gi_luma_strength = 0.0f,
    .char_gi_shadow_power = 1.25f,
    .char_gi_headroom_power = 1.25f,
    .char_gi_max_add = 0.020f,
    .char_gi_dark_boost = 0.0f,
    .char_gi_debug_mode = 0.f,
    .char_gi_debug_scale = 1.f,
    .char_gi_debug_chars_only = 1.f,
    .char_gi_bright_boost = 3.0f,
    .char_gi_peak_luma_cap = 0.0f,
    .char_gi_depth_reject = 2.0f,
    .char_gi_normal_reject = 0.15f,
    .char_gi_ao_influence = 0.66f,
    .char_gi_reject_strength = 8.0f,
};

float settings_mode = 0.f;
float char_ssgi_composite_method = 1.f;  // 0=Off, 1=On

std::atomic_uint64_t g_lighting_mrt0_view{0u};
std::atomic_uint64_t g_character_mrt0_view{0u};
HMODULE g_hmodule = nullptr;
reshade::api::resource g_isfast_texture = {0u};
reshade::api::resource_view g_isfast_reshade_srv = {0u};
reshade::api::sampler g_isfast_reshade_sampler = {0u};
bool g_isfast_bind_logged = false;
bool g_isfast_using_debug_texture = false;
bool g_isfast_bind_failed_logged = false;
bool g_isfast_compute_bind_logged = false;
bool g_isfast_mode_use_fast_texture = true;
float isfast_texture_source_enabled = 1.f;  // 0=Off(debug), 1=On(real DDS)

#pragma pack(push, 1)
struct DdsPixelFormat {
  uint32_t size;
  uint32_t flags;
  uint32_t four_cc;
  uint32_t rgb_bit_count;
  uint32_t r_bit_mask;
  uint32_t g_bit_mask;
  uint32_t b_bit_mask;
  uint32_t a_bit_mask;
};

struct DdsHeader {
  uint32_t size;
  uint32_t flags;
  uint32_t height;
  uint32_t width;
  uint32_t pitch_or_linear_size;
  uint32_t depth;
  uint32_t mip_map_count;
  uint32_t reserved1[11];
  DdsPixelFormat ddspf;
  uint32_t caps;
  uint32_t caps2;
  uint32_t caps3;
  uint32_t caps4;
  uint32_t reserved2;
};

struct DdsHeaderDx10 {
  uint32_t dxgi_format;
  uint32_t resource_dimension;
  uint32_t misc_flag;
  uint32_t array_size;
  uint32_t misc_flags2;
};
#pragma pack(pop)

constexpr uint32_t kDdsMagic = 0x20534444u;  // "DDS "
constexpr uint32_t kDdsFourCCDx10 = 0x30315844u;  // "DX10"
constexpr uint32_t kDxgiFormatR8G8Unorm = 49u;
constexpr uint32_t kD3D10ResourceDimensionTexture3D = 4u;
constexpr uint32_t kExpectedIsFastWidth = 128u;
constexpr uint32_t kExpectedIsFastHeight = 128u;
constexpr uint32_t kExpectedIsFastDepth = 32u;
constexpr uint32_t kIsFastBytesPerTexel = 2u;

void LogIsFast(reshade::log::level level, const std::string& message) {
  if (level == reshade::log::level::debug) return;
  const std::string tagged = "IS-FAST: " + message;
  reshade::log::message(level, tagged.c_str());
}

std::filesystem::path GetModuleDirectory() {
  if (g_hmodule == nullptr) return std::filesystem::current_path();

  std::array<wchar_t, MAX_PATH> module_path = {};
  const DWORD length = GetModuleFileNameW(g_hmodule, module_path.data(), static_cast<DWORD>(module_path.size()));
  if (length == 0 || length >= module_path.size()) return std::filesystem::current_path();
  return std::filesystem::path(module_path.data()).parent_path();
}

bool ReadFileBytes(const std::filesystem::path& path, std::vector<uint8_t>* out_bytes) {
  if (out_bytes == nullptr) return false;
  out_bytes->clear();

  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) return false;

  const auto file_size = file.tellg();
  if (file_size <= 0) return false;
  const size_t file_size_bytes = static_cast<size_t>(file_size);
  file.seekg(0, std::ios::beg);

  out_bytes->resize(file_size_bytes);
  if (!file.read(reinterpret_cast<char*>(out_bytes->data()), static_cast<std::streamsize>(file_size_bytes))) {
    out_bytes->clear();
    return false;
  }

  return true;
}

bool ValidateAndExtractIsFastDds(
    const std::vector<uint8_t>& file_bytes,
    std::span<const uint8_t>* out_pixel_data) {
  if (out_pixel_data == nullptr) return false;

  LogIsFast(reshade::log::level::debug, "DDS HEADER VALIDATION begin.");

  constexpr size_t kHeaderSize = sizeof(uint32_t) + sizeof(DdsHeader) + sizeof(DdsHeaderDx10);
  if (file_bytes.size() < kHeaderSize) {
    LogIsFast(reshade::log::level::error, "DDS too small to contain required headers.");
    return false;
  }

  uint32_t magic = 0u;
  std::memcpy(&magic, file_bytes.data(), sizeof(uint32_t));
  if (magic != kDdsMagic) {
    LogIsFast(reshade::log::level::error, "DDS magic mismatch (expected 'DDS ').");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS magic bytes OK.");

  DdsHeader header = {};
  std::memcpy(&header, file_bytes.data() + sizeof(uint32_t), sizeof(DdsHeader));
  if (header.size != 124u || header.ddspf.size != 32u) {
    LogIsFast(reshade::log::level::error, "DDS header size mismatch.");
    return false;
  }
  if (header.ddspf.four_cc != kDdsFourCCDx10) {
    LogIsFast(reshade::log::level::error, "DDS does not use required DX10 extended header.");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS DX10 extended header tag OK.");

  DdsHeaderDx10 header_dx10 = {};
  std::memcpy(&header_dx10, file_bytes.data() + sizeof(uint32_t) + sizeof(DdsHeader), sizeof(DdsHeaderDx10));
  if (header_dx10.dxgi_format != kDxgiFormatR8G8Unorm) {
    LogIsFast(reshade::log::level::error, "DDS format is not R8G8_UNORM.");
    return false;
  }
  if (header_dx10.resource_dimension != kD3D10ResourceDimensionTexture3D || header_dx10.array_size != 1u) {
    LogIsFast(reshade::log::level::error, "DDS is not a Texture3D with array_size=1.");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS texture type/dimension OK (Texture3D).");
  if (header.width != kExpectedIsFastWidth || header.height != kExpectedIsFastHeight || header.depth != kExpectedIsFastDepth) {
    LogIsFast(reshade::log::level::error, "DDS dimensions are invalid (expected 128x128x32).");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS dimensions OK (128x128x32).");

  const size_t pixel_data_offset = sizeof(uint32_t) + sizeof(DdsHeader) + sizeof(DdsHeaderDx10);
  const size_t expected_pixel_bytes =
      static_cast<size_t>(header.width) * static_cast<size_t>(header.height) * static_cast<size_t>(header.depth) * kIsFastBytesPerTexel;

  if (file_bytes.size() < (pixel_data_offset + expected_pixel_bytes)) {
    LogIsFast(reshade::log::level::error, "DDS file is truncated (pixel data too small).");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "DDS file size check OK.");

  const uint8_t* pixel_data_ptr = file_bytes.data() + pixel_data_offset;
  std::span<const uint8_t> pixel_data(pixel_data_ptr, expected_pixel_bytes);

  const size_t inspect_bytes_count = std::min<size_t>(32u, pixel_data.size());
  std::ostringstream inspect;
  inspect << "Pixel[0..31]:";
  for (size_t i = 0; i < inspect_bytes_count; ++i) {
    inspect << ' ' << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<uint32_t>(pixel_data[i]);
  }
  LogIsFast(reshade::log::level::debug, inspect.str());

  const size_t non_zero_count = std::count_if(
      pixel_data.begin(), pixel_data.end(),
      [](uint8_t value) { return value != 0u; });
  {
    std::ostringstream non_zero_message;
    non_zero_message << "Pixel non-zero bytes: " << non_zero_count << "/" << pixel_data.size();
    LogIsFast(reshade::log::level::debug, non_zero_message.str());
  }

  *out_pixel_data = pixel_data;
  LogIsFast(reshade::log::level::debug, "DDS HEADER VALIDATION passed.");
  return true;
}

void DestroyIsFastResources(reshade::api::device* device) {
  if (device == nullptr) return;

  LogIsFast(reshade::log::level::debug, "Destroying IS-FAST resources.");

  if (g_isfast_reshade_sampler.handle != 0u) {
    device->destroy_sampler(g_isfast_reshade_sampler);
    g_isfast_reshade_sampler = {0u};
  }
  if (g_isfast_reshade_srv.handle != 0u) {
    device->destroy_resource_view(g_isfast_reshade_srv);
    g_isfast_reshade_srv = {0u};
  }
  if (g_isfast_texture.handle != 0u) {
    device->destroy_resource(g_isfast_texture);
    g_isfast_texture = {0u};
  }

  shader_injection.isfast_noise_bound = 0.f;
  g_isfast_using_debug_texture = false;
  g_isfast_bind_logged = false;
  g_isfast_bind_failed_logged = false;
  g_isfast_compute_bind_logged = false;
}

bool CreateIsFastTexture(
    reshade::api::device* device,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    void* pixel_data,
    uint32_t row_pitch,
    uint32_t slice_pitch,
    bool debug_texture) {
  if (device == nullptr || pixel_data == nullptr) return false;

  {
    std::ostringstream message;
    message << "Create Texture3D begin (" << width << "x" << height << "x" << depth
            << ", " << (debug_texture ? "debug" : "dds") << ").";
    LogIsFast(reshade::log::level::debug, message.str());
  }

  reshade::api::resource_desc desc = {};
  desc.type = reshade::api::resource_type::texture_3d;
  desc.texture = {
      width,
      height,
      static_cast<uint16_t>(depth),
      1,
      reshade::api::format::r8g8_unorm,
      1,
  };
  desc.heap = reshade::api::memory_heap::gpu_only;
  desc.usage = reshade::api::resource_usage::shader_resource;
  desc.flags = reshade::api::resource_flags::none;

  reshade::api::subresource_data initial_data = {
      pixel_data,
      row_pitch,
      slice_pitch,
  };

  if (!device->create_resource(desc, &initial_data, reshade::api::resource_usage::shader_resource, &g_isfast_texture)) {
    LogIsFast(reshade::log::level::error, "Create Texture3D failed.");
    return false;
  }
  LogIsFast(reshade::log::level::debug, "Create Texture3D OK.");

  reshade::api::resource_view_desc view_desc(
      reshade::api::resource_view_type::texture_3d,
      reshade::api::format::r8g8_unorm,
      0,
      UINT32_MAX,
      0,
      UINT32_MAX);
  if (!device->create_resource_view(
          g_isfast_texture,
          reshade::api::resource_usage::shader_resource,
          view_desc,
          &g_isfast_reshade_srv)) {
    LogIsFast(reshade::log::level::error, "Create SRV failed.");
    device->destroy_resource(g_isfast_texture);
    g_isfast_texture = {0u};
    return false;
  }
  LogIsFast(reshade::log::level::debug, "Create SRV OK.");

  reshade::api::sampler_desc sampler_desc = {};
  sampler_desc.filter = reshade::api::filter_mode::min_mag_mip_point;
  sampler_desc.address_u = reshade::api::texture_address_mode::wrap;
  sampler_desc.address_v = reshade::api::texture_address_mode::wrap;
  sampler_desc.address_w = reshade::api::texture_address_mode::wrap;

  if (!device->create_sampler(sampler_desc, &g_isfast_reshade_sampler)) {
    LogIsFast(reshade::log::level::error, "Create sampler failed.");
    device->destroy_resource_view(g_isfast_reshade_srv);
    device->destroy_resource(g_isfast_texture);
    g_isfast_reshade_srv = {0u};
    g_isfast_texture = {0u};
    return false;
  }
  LogIsFast(reshade::log::level::debug, "Create point-wrap sampler OK.");

  g_isfast_using_debug_texture = debug_texture;
  g_isfast_bind_logged = false;
  return true;
}

void OnInitDevice(reshade::api::device* device) {
  if (device == nullptr) return;
  ReloadIsFastResources(device, "init_device");
}

void OnDestroyDevice(reshade::api::device* device) {
  LogIsFast(reshade::log::level::debug, "OnDestroyDevice begin.");
  DestroyIsFastResources(device);
  LogIsFast(reshade::log::level::debug, "OnDestroyDevice complete. isfast_noise_bound=0.");
}

void ReloadIsFastResources(reshade::api::device* device, const char* reason) {
  if (device == nullptr) return;

  const bool use_fast_texture = isfast_texture_source_enabled >= 0.5f;
  g_isfast_mode_use_fast_texture = use_fast_texture;

  {
    std::ostringstream message;
    message << "Reload requested (" << (reason != nullptr ? reason : "unknown")
            << "), IS-FAST texture mode=" << (use_fast_texture ? "On" : "Off");
    LogIsFast(reshade::log::level::info, message.str());
  }

  DestroyIsFastResources(device);

  if (!use_fast_texture) {
    LogIsFast(reshade::log::level::info, "IS-FAST texture mode Off: forcing debug 1x1x1 test texture.");

    std::array<uint8_t, 2> debug_texel = {
        static_cast<uint8_t>(0.75f * 255.0f + 0.5f),
        static_cast<uint8_t>(0.75f * 255.0f + 0.5f),
    };
    const bool debug_created = CreateIsFastTexture(
        device,
        1u,
        1u,
        1u,
        debug_texel.data(),
        2u,
        2u,
        true);
    if (debug_created) {
      shader_injection.isfast_noise_bound = 1.f;
      LogIsFast(reshade::log::level::info, "Debug 1x1x1 test texture created OK.");
    } else {
      shader_injection.isfast_noise_bound = 0.f;
      LogIsFast(reshade::log::level::error, "Failed to create debug 1x1x1 test texture.");
    }
    return;
  }

  const std::filesystem::path dds_path = GetModuleDirectory() / L"fast_noise_ea.dds";
  {
    std::ostringstream message;
    message << "Searching for fast_noise_ea.dds at: " << dds_path.string();
    LogIsFast(reshade::log::level::debug, message.str());
  }

  std::vector<uint8_t> dds_bytes;
  std::span<const uint8_t> pixel_data;
  bool loaded_noise = false;

  if (ReadFileBytes(dds_path, &dds_bytes)) {
    {
      std::ostringstream message;
      message << "Read DDS file OK (" << dds_bytes.size() << " bytes).";
      LogIsFast(reshade::log::level::debug, message.str());
    }

    if (ValidateAndExtractIsFastDds(dds_bytes, &pixel_data)) {
      const uint32_t row_pitch = kExpectedIsFastWidth * kIsFastBytesPerTexel;
      const uint32_t slice_pitch = kExpectedIsFastWidth * kExpectedIsFastHeight * kIsFastBytesPerTexel;
      loaded_noise = CreateIsFastTexture(
          device,
          kExpectedIsFastWidth,
          kExpectedIsFastHeight,
          kExpectedIsFastDepth,
          const_cast<uint8_t*>(pixel_data.data()),
          row_pitch,
          slice_pitch,
          false);
      if (loaded_noise) {
        shader_injection.isfast_noise_bound = 1.f;
        LogIsFast(reshade::log::level::info, "Loaded noise texture (128x128x32 RG8_UNORM).");
      } else {
        shader_injection.isfast_noise_bound = 0.f;
        LogIsFast(reshade::log::level::error, "Failed to create GPU resources from validated DDS.");
      }
    } else {
      shader_injection.isfast_noise_bound = 0.f;
      LogIsFast(reshade::log::level::warning, "DDS validation failed. IS-FAST noise is disabled.");
    }
  } else {
    shader_injection.isfast_noise_bound = 0.f;
    LogIsFast(reshade::log::level::warning, "Could not read fast_noise_ea.dds. IS-FAST noise is disabled.");
  }

  if (!loaded_noise) {
    std::array<uint8_t, 2> debug_texel = {
        static_cast<uint8_t>(0.75f * 255.0f + 0.5f),
        static_cast<uint8_t>(0.75f * 255.0f + 0.5f),
    };
    const bool debug_created = CreateIsFastTexture(
        device,
        1u,
        1u,
        1u,
        debug_texel.data(),
        2u,
        2u,
        true);
    if (debug_created) {
      LogIsFast(reshade::log::level::info, "Debug 1x1x1 test texture created OK.");
    } else {
      LogIsFast(reshade::log::level::error, "Failed to create debug 1x1x1 test texture.");
    }
  }
}

bool TryResolveTextureRegister(
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    uint32_t binding,
    uint32_t* out_register,
    uint32_t* out_space) {
  if (out_register == nullptr || out_space == nullptr) return false;
  auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr) return false;
  if (layout_param >= layout_data->params.size()) return false;

  const auto& param = layout_data->params.at(layout_param);
  auto resolve_range = [binding, out_register, out_space](const reshade::api::descriptor_range& range) -> bool {
    if (range.count == UINT32_MAX) return false;
    if (binding < range.binding || binding >= (range.binding + range.count)) return false;
    *out_register = range.dx_register_index + (binding - range.binding);
    *out_space = range.dx_register_space;
    return true;
  };

  switch (param.type) {
    case reshade::api::pipeline_layout_param_type::push_descriptors: {
      return resolve_range(param.push_descriptors);
    }
    case reshade::api::pipeline_layout_param_type::push_descriptors_with_ranges: {
      for (uint32_t i = 0; i < param.descriptor_table.count; ++i) {
        if (resolve_range(param.descriptor_table.ranges[i])) return true;
      }
      return false;
    }
    case reshade::api::pipeline_layout_param_type::descriptor_table: {
      for (uint32_t i = 0; i < param.descriptor_table.count; ++i) {
        if (resolve_range(param.descriptor_table.ranges[i])) return true;
      }
      return false;
    }
    case reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers: {
#if RESHADE_API_VERSION >= 13
      for (uint32_t i = 0; i < param.descriptor_table_with_static_samplers.count; ++i) {
        if (resolve_range(param.descriptor_table_with_static_samplers.ranges[i])) return true;
      }
#endif
      return false;
    }
    default:
      return false;
  }
}

void CaptureTrackedTextureView(
    const uint32_t shader_hash,
    const uint32_t reg,
    const uint32_t space,
    const reshade::api::resource_view view) {
  if (view.handle == 0u) return;
  if (space != 0u) return;

  if (shader_hash == kCharacterShader && reg == 2u) {
    // Character pass uses mrtTexture0 at t2.
    g_character_mrt0_view.store(view.handle, std::memory_order_relaxed);
  } else if ((shader_hash == kLightingShader || shader_hash == kLightingSoftShader) && reg == 1u) {
    // Keep lighting mrt0 as fallback in case character pass capture is unavailable.
    g_lighting_mrt0_view.store(view.handle, std::memory_order_relaxed);
  }
}

void OnPushDescriptorsCaptureLightingTextures(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t layout_param,
    const reshade::api::descriptor_table_update& update) {
  if (cmd_list == nullptr) return;
  if ((static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) == 0u) return;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return;
  const auto shader_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  const bool is_tracked_shader =
      shader_hash == kLightingShader || shader_hash == kLightingSoftShader || shader_hash == kCharacterShader;

  for (uint32_t i = 0; i < update.count; ++i) {
    uint32_t reg = 0u;
    uint32_t space = 0u;
    if (!TryResolveTextureRegister(layout, layout_param, update.binding + i, &reg, &space)) {
      reg = update.binding + i;
      space = 0u;
    }
    if (update.type == reshade::api::descriptor_type::constant_buffer) continue;
    if (!is_tracked_shader) continue;
    const auto view = renodx::utils::descriptor::GetResourceViewFromDescriptorUpdate(update, i);
    CaptureTrackedTextureView(shader_hash, reg, space, view);
  }
}

bool TryGetResourceViewFromBoundDescriptorTable(
    reshade::api::device* device,
    const reshade::api::descriptor_table& table,
    const reshade::api::descriptor_range& range,
    const uint32_t descriptor_index,
    reshade::api::resource_view* out_view) {
  if (device == nullptr || out_view == nullptr) return false;
  if (table.handle == 0u) return false;
  if (range.count == UINT32_MAX) return false;
  if (descriptor_index >= range.count) return false;

  auto* descriptor_data = renodx::utils::data::Get<renodx::utils::descriptor::DeviceData>(device);
  if (descriptor_data == nullptr) return false;

  uint32_t base_offset = 0u;
  reshade::api::descriptor_heap heap = {0u};
  device->get_descriptor_heap_offset(table, range.binding, 0, &heap, &base_offset);
  if (heap.handle == 0u) return false;

  const std::shared_lock lock(descriptor_data->mutex);

  auto heap_pair = descriptor_data->heaps.find(heap.handle);
  if (heap_pair == descriptor_data->heaps.end()) return false;

  auto known_pair = descriptor_data->resource_view_heap_locations.find(heap.handle);
  if (known_pair == descriptor_data->resource_view_heap_locations.end()) return false;

  const uint32_t offset = base_offset + descriptor_index;
  const auto& heap_entries = heap_pair->second;
  if (offset >= heap_entries.size()) return false;
  if (!known_pair->second.contains(offset)) return false;

  const auto& [descriptor_type, descriptor_value] = heap_entries[offset];
  reshade::api::resource_view view = {};
  switch (descriptor_type) {
    case reshade::api::descriptor_type::sampler_with_resource_view:
      view = std::get<reshade::api::sampler_with_resource_view>(descriptor_value).view;
      break;
    case reshade::api::descriptor_type::buffer_shader_resource_view:
    case reshade::api::descriptor_type::texture_shader_resource_view:
    case reshade::api::descriptor_type::buffer_unordered_access_view:
    case reshade::api::descriptor_type::texture_unordered_access_view:
      view = std::get<reshade::api::resource_view>(descriptor_value);
      break;
    default:
      return false;
  }
  if (view.handle == 0u) return false;

  *out_view = view;
  return true;
}

void OnBindDescriptorTablesCaptureLightingTextures(
    reshade::api::command_list* cmd_list,
    reshade::api::shader_stage stages,
    reshade::api::pipeline_layout layout,
    uint32_t first,
    uint32_t count,
    const reshade::api::descriptor_table* tables) {
  if (cmd_list == nullptr || tables == nullptr || count == 0u) return;
  if ((static_cast<uint32_t>(stages) & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) == 0u) return;

  auto* shader_state = renodx::utils::shader::GetCurrentState(cmd_list);
  if (shader_state == nullptr) return;
  const auto shader_hash = renodx::utils::shader::GetCurrentPixelShaderHash(shader_state);
  if (shader_hash != kLightingShader && shader_hash != kLightingSoftShader && shader_hash != kCharacterShader) return;

  auto* layout_data = renodx::utils::pipeline_layout::GetPipelineLayoutData(layout);
  if (layout_data == nullptr) return;

  auto* device = cmd_list->get_device();
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t param_index = first + i;
    if (param_index >= layout_data->params.size()) continue;

    const auto& param = layout_data->params.at(param_index);
    const auto& table = tables[i];
    if (table.handle == 0u) continue;

    uint32_t range_count = 0u;
    const reshade::api::descriptor_range* ranges = nullptr;
    if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table) {
      range_count = param.descriptor_table.count;
      ranges = param.descriptor_table.ranges;
    } else if (param.type == reshade::api::pipeline_layout_param_type::descriptor_table_with_static_samplers) {
      range_count = param.descriptor_table_with_static_samplers.count;
      ranges = param.descriptor_table_with_static_samplers.ranges;
    } else {
      continue;
    }

    for (uint32_t j = 0; j < range_count; ++j) {
      const auto& range = ranges[j];
      if (range.count == UINT32_MAX) continue;

      switch (range.type) {
        case reshade::api::descriptor_type::sampler_with_resource_view:
        case reshade::api::descriptor_type::buffer_shader_resource_view:
        case reshade::api::descriptor_type::texture_shader_resource_view:
          break;
        default:
          continue;
      }

      if ((static_cast<uint32_t>(range.visibility) & static_cast<uint32_t>(reshade::api::shader_stage::pixel)) == 0u) {
        continue;
      }

      for (uint32_t k = 0; k < range.count; ++k) {
        reshade::api::resource_view view = {};
        if (!TryGetResourceViewFromBoundDescriptorTable(device, table, range, k, &view)) continue;
        CaptureTrackedTextureView(shader_hash, range.dx_register_index + k, range.dx_register_space, view);
      }
    }
  }
}

bool OnBeforeLightingShaderDraw(reshade::api::command_list* cmd_list) {
  // Bind character shader's MRT texture to t10 so the lighting shader
  // can read the character mask from it (the lighting shader's own t1
  // may not contain the character flag bits).
  if (cmd_list == nullptr) return true;
  if (shader_injection.char_gi_enabled < 0.5f) return true;

  reshade::api::resource_view char_mrt0 = {g_character_mrt0_view.load(std::memory_order_relaxed)};
  if (char_mrt0.handle == 0u) {
    char_mrt0 = {g_lighting_mrt0_view.load(std::memory_order_relaxed)};
  }
  if (char_mrt0.handle == 0u) return true;

  auto* info = renodx::utils::resource::GetResourceViewInfo(char_mrt0);
  if (info == nullptr || info->destroyed) return true;

  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0},
      0,
      reshade::api::descriptor_table_update{
          {},
          10,
          0,
          1,
          reshade::api::descriptor_type::texture_shader_resource_view,
          &char_mrt0,
      });
  return true;
}

bool BindISFastNoisePixel(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return true;
  if (g_isfast_reshade_srv.handle == 0u || g_isfast_reshade_sampler.handle == 0u) {
    if (!g_isfast_bind_failed_logged) {
      LogIsFast(reshade::log::level::warning, "BindISFASTNoisePixel match=NO (SRV/sampler unavailable).");
      g_isfast_bind_failed_logged = true;
    }
    return true;
  }

  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0},
      0,
      reshade::api::descriptor_table_update{
          {},
          kIsFastTextureBinding,
          0,
          1,
          reshade::api::descriptor_type::texture_shader_resource_view,
          &g_isfast_reshade_srv,
      });

  cmd_list->push_descriptors(
      reshade::api::shader_stage::pixel,
      reshade::api::pipeline_layout{0},
      0,
      reshade::api::descriptor_table_update{
          {},
          kIsFastSamplerBinding,
          0,
          1,
          reshade::api::descriptor_type::sampler,
          &g_isfast_reshade_sampler,
      });

  if (!g_isfast_bind_logged) {
    if (shader_injection.isfast_noise_bound > 0.5f) {
      LogIsFast(
          reshade::log::level::debug,
          g_isfast_using_debug_texture
              ? "BindISFASTNoisePixel match=YES (debug texture bound)"
              : "BindISFASTNoisePixel match=YES");
    } else {
      LogIsFast(
          reshade::log::level::debug,
          g_isfast_using_debug_texture
              ? "BindISFASTNoisePixel match=YES (debug texture bound, sampling disabled)"
              : "BindISFASTNoisePixel match=YES (sampling disabled)");
    }
    if (!g_isfast_compute_bind_logged) {
      LogIsFast(reshade::log::level::debug, "BindISFASTNoiseCompute match=YES (N/A, no compute target in this addon).");
      g_isfast_compute_bind_logged = true;
    }
    g_isfast_bind_logged = true;
  }

  return true;
}

bool OnBeforeVolFogShaderDraw(reshade::api::command_list* cmd_list) {
  return BindISFastNoisePixel(cmd_list);
}

void OnPresentAdvanceFrame(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  if (queue != nullptr) {
    const bool desired_fast_texture = isfast_texture_source_enabled >= 0.5f;
    if (desired_fast_texture != g_isfast_mode_use_fast_texture) {
      ReloadIsFastResources(queue->get_device(), "settings_change");
    }
  }
  (void)swapchain;
  (void)source_rect;
  (void)dest_rect;
  (void)dirty_rect_count;
  (void)dirty_rects;

  // Sync char_gi_enabled: 1 when On (method>=0.5), 0 when Off
  shader_injection.char_gi_enabled =
      (char_ssgi_composite_method >= 0.5f) ? 1.f : 0.f;

  // Basic mode: force all char_gi params to their defaults so sliders have no effect
  if (settings_mode < 0.5f) {
    shader_injection.char_gi_strength = 3.0f;
    shader_injection.char_gi_alpha_scale = 1.0f;
    shader_injection.char_gi_chroma_strength = 0.50f;
    shader_injection.char_gi_luma_strength = 0.0f;
    shader_injection.char_gi_shadow_power = 1.25f;
    shader_injection.char_gi_headroom_power = 1.25f;
    shader_injection.char_gi_max_add = 0.020f;
    shader_injection.char_gi_dark_boost = 0.0f;
    shader_injection.char_gi_debug_mode = 0.f;
    shader_injection.char_gi_debug_scale = 1.f;
    shader_injection.char_gi_debug_chars_only = 1.f;
    shader_injection.char_gi_bright_boost = 3.0f;
    shader_injection.char_gi_peak_luma_cap = 0.0f;
    shader_injection.char_gi_depth_reject = 2.0f;
    shader_injection.char_gi_normal_reject = 0.15f;
    shader_injection.char_gi_ao_influence = 0.66f;
    shader_injection.char_gi_reject_strength = 8.0f;
  }
}

bool IsAdvancedSettingsMode() {
  return settings_mode >= 0.5f;
}

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "SettingsMode",
        .binding = &settings_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .can_reset = false,
        .label = "Settings Mode",
        .section = "Settings",
        .labels = {"Basic", "Intermediate"},
        .is_global = true,
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowPCSSJitter",
        .binding = &shader_injection.shadow_pcss_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "IS-FAST jitter",
        .section = "Shadows",
        .tooltip = "Enables IS-FAST temporal jitter for PCSS shadow filtering.",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowISFASTJitterSpeed",
        .binding = &shader_injection.shadow_isfast_jitter_speed,
        .default_value = 237.f,
        .label = "IS-FAST jitter speed",
        .section = "Shadows",
        .tooltip = "Controls temporal progression speed for IS-FAST jitter in PCSS shadows.",
        .min = 0.f,
        .max = 480.f,
        .format = "%.0f",
        .is_enabled = []() { return shader_injection.shadow_pcss_jitter_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "ShadowBaseSoftness",
        .binding = &shader_injection.shadow_base_softness,
        .default_value = 0.2f,
        .label = "Base Softness",
        .section = "Shadows",
        .tooltip = "Adds a constant to the PCSS filter radius. 0.0 is Vanilla.",
        .min = 0.f,
        .max = 0.5f,
        .format = "%.2f",
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CubemapImprovements",
        .binding = &shader_injection.cubemap_improvements_enabled,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Mode",
        .section = "Cubemap",
        .labels = {"Vanilla", "Improved"},
    },
    new renodx::utils::settings::Setting{
        .key = "LightingCubemapMipBoost",
        .binding = &shader_injection.cubemap_lighting_mip_boost,
        .default_value = 1.5f,
        .label = "Lighting Mip Boost",
        .section = "Cubemap",
        .tooltip = "Lighting shader cubemap mip scale. Default is 1.5x.",
        .min = 0.5f,
        .max = 4.f,
        .format = "%.1fx",
        .is_enabled = []() { return shader_injection.cubemap_improvements_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
      new renodx::utils::settings::Setting{
        .key = "FloorCubemapMipScale",
        .binding = &shader_injection.floor_cubemap_mip_scale,
        .default_value = 4.f,
        .label = "Floor Mip Scale",
        .section = "Cubemap",
        .tooltip = "Scales floor reflection roughness/mip response. 1.0 = Vanilla.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
    new renodx::utils::settings::Setting{
        .key = "SSGIEnable",
        .binding = &shader_injection.ssgi_mod_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Enable",
        .section = "SSGI",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIColorBoost",
        .binding = &shader_injection.ssgi_color_boost,
        .default_value = 1.f,
        .label = "Color Boost",
        .section = "SSGI",
        .tooltip = "Scales SSGI RGB contribution before power shaping.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIAlphaBoost",
        .binding = &shader_injection.ssgi_alpha_boost,
        .default_value = 1.f,
        .label = "Alpha Boost",
        .section = "SSGI",
        .tooltip = "Scales SSGI alpha before saturate.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSGIPower",
        .binding = &shader_injection.ssgi_pow,
        .default_value = 1.f,
        .label = "Power",
        .section = "SSGI",
        .tooltip = "Applies pow(abs(color), Power) to shape bounce response.",
        .min = 0.1f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.ssgi_mod_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "SSRMode",
        .binding = &shader_injection.ssr_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Mode",
        .section = "SSR",
        .labels = {"Vanilla", "Improved"},
    },
    new renodx::utils::settings::Setting{
        .key = "SSRRayCountScale",
        .binding = &shader_injection.ssr_ray_count_scale,
        .default_value = 2.5f,
        .label = "Ray Count Scale",
        .section = "SSR",
        .tooltip = "Scales the game's SSR ray count budget. 1.0x is Vanilla.",
        .min = 0.5f,
        .max = 8.f,
        .format = "%.1fx",
        .is_enabled = []() { return shader_injection.ssr_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFMode",
        .binding = &shader_injection.dof_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Mode",
        .section = "Depth of Field",
        .tooltip = "Vanilla keeps the original blur shader. Improved uses DOF method 3 (gather).",
        .labels = {"Vanilla", "Improved"},
    },
    new renodx::utils::settings::Setting{
        .key = "DOFStrength",
        .binding = &shader_injection.dof_strength,
        .default_value = 1.f,
        .label = "Strength",
        .section = "Depth of Field",
        .tooltip = "Overall blend strength for improved DOF output.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFRadiusScale",
        .binding = &shader_injection.dof_radius_scale,
        .default_value = 1.33f,
        .label = "Radius Scale",
        .section = "Depth of Field",
        .tooltip = "Scales blur radius derived from game CoC.",
        .min = 0.25f,
        .max = 2.5f,
        .format = "%.2fx",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFSampleCount",
        .binding = &shader_injection.dof_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 24.f,
        .label = "Sample Count",
        .section = "Depth of Field",
        .tooltip = "Higher values produce smoother bokeh at higher cost.",
        .min = 4.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFNearScale",
        .binding = &shader_injection.dof_near_scale,
        .default_value = 1.f,
        .label = "Near Scale",
        .section = "Depth of Field",
        .tooltip = "Scales near-field CoC response.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFFarScale",
        .binding = &shader_injection.dof_far_scale,
        .default_value = 1.f,
        .label = "Far Scale",
        .section = "Depth of Field",
        .tooltip = "Scales far-field CoC response.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFCoCCurve",
        .binding = &shader_injection.dof_coc_curve,
        .default_value = 1.f,
        .label = "CoC Curve",
        .section = "Depth of Field",
        .tooltip = "Applies pow(CoC, Curve) before blur; >1 tightens focus transition.",
        .min = 0.25f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DOFEdgeThreshold",
        .binding = &shader_injection.dof_edge_threshold,
        .default_value = 0.25f,
        .label = "Edge Threshold",
        .section = "Depth of Field",
        .tooltip = "Rejects CoC-mismatched taps to reduce foreground/background bleeding.",
        .min = 0.02f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.dof_mode >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowMode",
        .binding = &shader_injection.char_shadow_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 2.f,
        .label = "Mode",
        .section = "Character Shadowing",
        .labels = {"Off", "Vanilla", "Bend_SSS"},
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowStrength",
        .binding = &shader_injection.char_shadow_strength,
        .default_value = 100.f,
        .label = "Strength",
        .section = "Character Shadowing",
        .tooltip = "Blend strength for character sun screen-space shadowing.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.char_shadow_mode >= 1.f; },
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowSampleCount",
        .binding = &shader_injection.char_shadow_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 32.f,
        .label = "Sample Count",
        .section = "Character Shadowing",
        .tooltip = "Higher values increase shadow reach/quality at higher cost.",
        .min = 1.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowHardSamples",
        .binding = &shader_injection.char_shadow_hard_shadow_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 4.f,
        .label = "Hard Samples",
        .section = "Character Shadowing",
        .tooltip = "Near-contact samples that preserve harder shadows.",
        .min = 0.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowFadeSamples",
        .binding = &shader_injection.char_shadow_fade_out_samples,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 16.f,
        .label = "Fade Samples",
        .section = "Character Shadowing",
        .tooltip = "Tail samples used to soften the far shadow cutoff.",
        .min = 0.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowSurfaceThickness",
        .binding = &shader_injection.char_shadow_surface_thickness,
        .default_value = 0.09f,
        .label = "Surface Thickness",
        .section = "Character Shadowing",
        .tooltip = "Depth thickness assumption for occluder matching.",
        .min = 0.001f,
        .max = 0.2f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowContrast",
        .binding = &shader_injection.char_shadow_contrast,
        .default_value = 9.f,
        .label = "Shadow Contrast",
        .section = "Character Shadowing",
        .tooltip = "Higher values darken/crisp the character shadow transition.",
        .min = 0.f,
        .max = 12.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowLightFadeStart",
        .binding = &shader_injection.char_shadow_light_screen_fade_start,
        .default_value = 0.f,
        .label = "Light Fade Start",
        .section = "Character Shadowing",
        .tooltip = "Minimum projected sun length before shadows ramp in.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowLightFadeEnd",
        .binding = &shader_injection.char_shadow_light_screen_fade_end,
        .default_value = 0.f,
        .label = "Light Fade End",
        .section = "Character Shadowing",
        .tooltip = "Projected sun length where shadows reach full strength.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowMinOccluderDepthScale",
        .binding = &shader_injection.char_shadow_min_occluder_depth_scale,
        .default_value = 0.f,
        .label = "Occluder Depth Scale",
        .section = "Character Shadowing",
        .tooltip = "Rejects tiny depth deltas to reduce self-shadowing noise.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharShadowJitter",
        .binding = &shader_injection.char_shadow_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "IS-FAST jitter",
        .section = "Character Shadowing",
        .tooltip = "Enables IS-FAST temporal jitter for character shadow sampling.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.char_shadow_mode == 2.f; },
    },
    // â”€â”€ SSS â”€â”€
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSEnabled",
        .binding = &shader_injection.foliage_sss_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Bend SSS",
        .section = "Screen Space Shadows",
        .tooltip = "Screen-space shadowing effect (Bend Studio algorithm).",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSStrength",
        .binding = &shader_injection.foliage_sss_strength,
      .default_value = 100.f,
        .label = "Strength",
        .section = "Screen Space Shadows",
        .tooltip = "Blend strength for screen-space shadows.",
        .min = 0.f,
        .max = 100.f,
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .parse = [](float value) { return value * 0.01f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSSampleCount",
        .binding = &shader_injection.foliage_sss_sample_count,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
      .default_value = 24.f,
        .label = "Sample Count",
        .section = "Screen Space Shadows",
        .tooltip = "Higher values increase shadow reach/quality at higher cost.",
        .min = 1.f,
        .max = 64.f,
        .format = "%d",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSSurfaceThickness",
        .binding = &shader_injection.foliage_sss_surface_thickness,
        .default_value = 0.005f,
        .label = "Surface Thickness",
        .section = "Screen Space Shadows",
        .tooltip = "Depth thickness assumption for occluder matching.",
        .min = 0.001f,
        .max = 0.2f,
        .format = "%.4f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSContrast",
        .binding = &shader_injection.foliage_sss_contrast,
      .default_value = 2.f,
        .label = "Shadow Contrast",
        .section = "Screen Space Shadows",
        .tooltip = "Higher values darken/crisp the shadow transition.",
        .min = 0.f,
        .max = 12.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FoliageSSSJitter",
        .binding = &shader_injection.foliage_sss_jitter_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "IS-FAST jitter",
        .section = "Screen Space Shadows",
        .tooltip = "Enables IS-FAST temporal jitter for foliage screen-space shadow sampling.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
    },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSHeightEnable",
        .binding = &shader_injection.foliage_sss_height_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Height Above Ground",
        .section = "Screen Space Shadows",
        .tooltip = "Only apply SSS to pixels above a certain height from the ground surface below them. Adapts to any map elevation.",
        .labels = {"Off", "On"},
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSHeightMin",
        .binding = &shader_injection.foliage_sss_height_min,
        .default_value = 0.f,
        .label = "Min Height",
        .section = "Screen Space Shadows",
        .tooltip = "Minimum height above the ground (world units) before SSS starts. Pixels closer to the ground get no SSS.",
        .min = 0.f,
        .max = 10.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.foliage_sss_enabled >= 0.5f &&
             shader_injection.foliage_sss_height_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSHeightMax",
        .binding = &shader_injection.foliage_sss_height_max,
        .default_value = 1.f,
        .label = "Ground Search",
        .section = "Screen Space Shadows",
        .tooltip = "How many pixels downward on-screen to search for the ground surface. Higher = works for taller geometry.",
        .min = 1.f,
        .max = 200.f,
        .format = "%.0f",
        .is_enabled = []() {
          return shader_injection.foliage_sss_enabled >= 0.5f &&
             shader_injection.foliage_sss_height_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSHeightFade",
        .binding = &shader_injection.foliage_sss_height_fade,
        .default_value = 0.10f,
        .label = "Height Fade",
        .section = "Screen Space Shadows",
        .tooltip = "Smooth transition range (world units) above the min height threshold.",
        .min = 0.f,
        .max = 5.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.foliage_sss_enabled >= 0.5f &&
             shader_injection.foliage_sss_height_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSVerticalReject",
        .binding = &shader_injection.foliage_sss_vertical_reject,
        .default_value = 0.30f,
        .label = "Vertical Reject",
        .section = "Screen Space Shadows",
        .tooltip = "Rejects vertical surfaces (walls, pillars). 0 = off, higher = stricter. Based on how upward-facing the normal is.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSSMaxDarkening",
        .binding = &shader_injection.foliage_sss_max_darkening,
        .default_value = 0.50f,
        .label = "Max Darkening",
        .section = "Screen Space Shadows",
        .tooltip = "Limits how dark shadows can get. 1.0 = full darkening allowed, 0.0 = no darkening at all.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSBrightRejectThreshold",
        .binding = &shader_injection.foliage_sss_bright_reject_threshold,
        .default_value = 0.19f,
        .label = "Brightness Reject",
        .section = "Screen Space Shadows",
        .tooltip = "Pixels brighter than this luminance will resist SSS darkening. Protects lamps and emissive surfaces.",
        .min = 0.f,
        .max = 5.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
      new renodx::utils::settings::Setting{
        .key = "FoliageSSBrightRejectFade",
        .binding = &shader_injection.foliage_sss_bright_reject_fade,
        .default_value = 0.5f,
        .label = "Brightness Fade",
        .section = "Screen Space Shadows",
        .tooltip = "How gradual the brightness rejection transition is. Lower = sharper cutoff.",
        .min = 0.01f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return shader_injection.foliage_sss_enabled >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
      },
    // -- SSS Debug --
    new renodx::utils::settings::Setting{
        .key = "FoliageDebugMode",
        .binding = &shader_injection.foliage_debug_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug Mode",
        .section = "SSS Debug",
        .tooltip = "Debug visualization for the SSS pipeline.",
        .labels = {"Off", "SSS Mask", "Shadow Value", "SSAO Texture", "MRT Bits", "Bit9 Raw", "Vanilla Detect", "Raw Bytes"},
      .is_visible = []() { return IsAdvancedSettingsMode(); },
    },

    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeMethod",
        .binding = &char_ssgi_composite_method,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "Apply Game SSGI",
        .section = "Character SSGI Composite",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeStrength",
        .binding = &shader_injection.char_gi_strength,
        .default_value = 3.0f,
        .label = "Strength",
        .section = "Character SSGI Composite",
        .tooltip = "Overall contribution scale for character GI.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeAlphaScale",
        .binding = &shader_injection.char_gi_alpha_scale,
        .default_value = 1.0f,
        .label = "Alpha Scale",
        .section = "Character SSGI Composite",
        .tooltip = "Scales sampled SSGI alpha before blending.",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeChroma",
        .binding = &shader_injection.char_gi_chroma_strength,
        .default_value = 0.50f,
        .label = "Chroma",
        .section = "Character SSGI Composite",
        .tooltip = "Scales colorful GI component; lower values reduce tinting.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeLuma",
        .binding = &shader_injection.char_gi_luma_strength,
        .default_value = 0.0f,
        .label = "Luma",
        .section = "Character SSGI Composite",
        .tooltip = "Scales neutral GI brightness; keep low to avoid white haze.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeShadowPower",
        .binding = &shader_injection.char_gi_shadow_power,
        .default_value = 1.25f,
        .label = "Shadow Power",
        .section = "Character SSGI Composite",
        .tooltip = "Higher values concentrate GI toward darker areas.",
        .min = 0.1f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDarkBoost",
        .binding = &shader_injection.char_gi_dark_boost,
        .default_value = 0.0f,
        .label = "Dark Boost",
        .section = "Character SSGI Composite",
        .tooltip = "Extra GI multiplier in darker regions (after shadow mask).",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeBrightBoost",
        .binding = &shader_injection.char_gi_bright_boost,
        .default_value = 3.0f,
        .label = "Bright Boost",
        .section = "Character SSGI Composite",
        .tooltip = "Boosts GI on brighter regions (values above 1.0 increase bright-side contribution).",
        .min = 0.f,
        .max = 3.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeHeadroomPower",
        .binding = &shader_injection.char_gi_headroom_power,
        .default_value = 1.25f,
        .label = "Headroom Power",
        .section = "Character SSGI Composite",
        .tooltip = "Controls how strongly bright pixels reject additional GI.",
        .min = 0.1f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeMaxAdd",
        .binding = &shader_injection.char_gi_max_add,
        .default_value = 0.020f,
        .label = "Max Add",
        .section = "Character SSGI Composite",
        .tooltip = "Per-channel cap for added GI to prevent haze/bloomy washout.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositePeakLumaCap",
        .binding = &shader_injection.char_gi_peak_luma_cap,
        .default_value = 0.0f,
        .label = "Peak Luma Cap",
        .section = "Character SSGI Composite",
        .tooltip = "Caps peak GI brightness on characters after blending weights. Set 0 to disable.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.3f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDepthReject",
        .binding = &shader_injection.char_gi_depth_reject,
        .default_value = 2.0f,
        .label = "Depth Reject",
        .section = "Character SSGI Composite",
        .tooltip = "Higher values suppress GI across depth discontinuities and silhouette edges.",
        .min = 0.f,
        .max = 16.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeNormalReject",
        .binding = &shader_injection.char_gi_normal_reject,
        .default_value = 0.15f,
        .label = "Normal Reject",
        .section = "Character SSGI Composite",
        .tooltip = "Higher values suppress GI across normal/material edges.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeAOInfluence",
        .binding = &shader_injection.char_gi_ao_influence,
        .default_value = 0.66f,
        .label = "AO Influence",
        .section = "Character SSGI Composite",
        .tooltip = "Uses game's AO to dampen character GI in occluded areas.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeRejectStrength",
        .binding = &shader_injection.char_gi_reject_strength,
        .default_value = 8.0f,
        .label = "Reject Strength",
        .section = "Character SSGI Composite",
        .tooltip = "Amplifies depth/normal/AO rejection visibility without changing base GI sliders.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDebugMode",
        .binding = &shader_injection.char_gi_debug_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Debug View",
        .section = "Character SSGI Composite",
        .labels = {
            "Off",
            "Character Mask",
            "Raw SSGI RGB",
            "Raw SSGI Alpha",
            "Filtered GI",
            "GI Weight (Pre-Headroom)",
            "GI Contribution Abs",
            "Source Color",
            "GI Contribution Luma",
            "Headroom",
            "Final Gain",
            "Shadow Mask (Adj)",
            "Depth Factor",
            "Normal Factor",
            "AO Factor",
            "Reject Factor",
        },
        .is_enabled = []() { return char_ssgi_composite_method >= 0.5f; },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDebugScale",
        .binding = &shader_injection.char_gi_debug_scale,
        .default_value = 1.f,
        .label = "Debug Scale",
        .section = "Character SSGI Composite",
        .tooltip = "Scales debug intensity for RGB/alpha/weight/contribution views.",
        .min = 0.1f,
        .max = 32.f,
        .format = "%.2f",
        .is_enabled = []() {
          return char_ssgi_composite_method >= 0.5f
                 && shader_injection.char_gi_debug_mode >= 1.f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "CharacterSSGICompositeDebugCharsOnly",
        .binding = &shader_injection.char_gi_debug_chars_only,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Debug Characters Only",
        .section = "Character SSGI Composite",
        .labels = {"Off", "On"},
        .is_enabled = []() {
          return char_ssgi_composite_method >= 0.5f
                 && shader_injection.char_gi_debug_mode >= 1.f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogTricubic",
        .binding = &shader_injection.volfog_tricubic_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Tricubic Sampling",
        .section = "Volumetric Fog",
        .tooltip = "Replaces trilinear with tricubic B-spline filtering on the fog volume, eliminating blocky voxel boundaries.",
        .labels = {"Off", "On"},
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogISFAST",
        .binding = &shader_injection.volfog_is_fast_enabled,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "IS-FAST jitter",
        .section = "Volumetric Fog",
        .tooltip = "Enables IS-FAST temporal jitter for volumetric fog sampling. Vanilla keeps the original sample coordinates.",
        .labels = {"Vanilla", "Enabled"},
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogISFASTTexture",
        .binding = &isfast_texture_source_enabled,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "IS-FAST Texture",
        .section = "Volumetric Fog",
        .tooltip = "Off forces the 1x1 debug texture. On uses fast_noise_ea.dds if it validates.",
        .labels = {"Off", "On"},
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "VolFogColorCorrectionStrength",
        .binding = &shader_injection.volfog_color_correction_strength,
        .default_value = 0.5f,
        .label = "Color Correction Strength",
        .section = "Volumetric Fog",
        .tooltip = "Independent strength for volumetric fog color correction.",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogColorCorrection",
        .binding = &shader_injection.fog_color_correction_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Mode",
        .section = "Fog Color Correction",
        .labels = {"Vanilla", "Improved"},
    },
    new renodx::utils::settings::Setting{
        .key = "FogHue",
        .binding = &shader_injection.fog_hue,
        .default_value = 0.f,
        .label = "Fog Hue",
        .section = "Fog Color Correction",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogChrominance",
        .binding = &shader_injection.fog_chrominance,
        .default_value = 0.f,
        .label = "Fog Chroma",
        .section = "Fog Color Correction",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogAvgBrightness",
        .binding = &shader_injection.fog_avg_brightness,
        .default_value = 0.85f,
        .label = "Fog Avg Bright",
        .section = "Fog Color Correction",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogMinBrightness",
        .binding = &shader_injection.fog_min_brightness,
        .default_value = 0.f,
        .label = "Fog Min Bright",
        .section = "Fog Color Correction",
        .min = -0.5f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogMinChroma",
        .binding = &shader_injection.fog_min_chroma_change,
        .default_value = 0.f,
        .label = "Fog Min Chroma",
        .section = "Fog Color Correction",
        .tooltip = "Minimum chroma ratio applied during fog hue/chroma restoration.",
        .min = 0.f,
        .max = 4.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogMaxChroma",
        .binding = &shader_injection.fog_max_chroma_change,
        .default_value = 0.f,
        .label = "Fog Max Chroma",
        .section = "Fog Color Correction",
        .tooltip = "Maximum chroma ratio applied during fog hue/chroma restoration.",
        .min = 0.f,
        .max = 8.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogLightnessStrength",
        .binding = &shader_injection.fog_lightness_strength,
        .default_value = 1.f,
        .label = "Fog Lightness",
        .section = "Fog Color Correction",
        .tooltip = "Scales fog lightness restoration amount.",
        .min = 0.f,
        .max = 2.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FogColorCorrectionStrength",
        .binding = &shader_injection.fog_color_correction_strength,
      .default_value = 0.5f,
        .label = "2D Fog Correction Strength",
        .section = "Fog Color Correction",
        .min = 0.f,
        .max = 1.f,
        .format = "%.2f",
        .is_enabled = []() {
          return shader_injection.fog_color_correction_enabled >= 0.5f;
        },
        .is_visible = []() { return IsAdvancedSettingsMode(); },
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Addon made by Toru.",
        .section = "Info",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Thanks to Shortfuse for RenoDX.",
        .section = "Info",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Disable IS-FAST jitter options if you are not using TAA or upscalers!",
        .section = "Info",
    },
    new renodx::utils::settings::Setting{
        .value_type = renodx::utils::settings::SettingValueType::TEXT,
        .label = "Enable Soft/PCSS Shadows and Ultra SSR.",
        .section = "Info",
    },
};

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Kai Vanilla+";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "Kai Vanilla+";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      g_hmodule = h_module;

      reshade::register_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::register_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::register_event<reshade::addon_event::present>(OnPresentAdvanceFrame);
      reshade::register_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureLightingTextures);
      reshade::register_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTablesCaptureLightingTextures);

      renodx::utils::settings::overlay_title = "Kai Vanilla+";
      renodx::utils::settings::global_name = "kai-vanilla-plus";
      renodx::utils::settings::use_presets = false;

      renodx::mods::shader::force_pipeline_cloning = true;
      renodx::mods::shader::allow_multiple_push_constants = true;
      renodx::mods::shader::expected_constant_buffer_index = 13;
      renodx::mods::shader::expected_constant_buffer_space = 0;

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDevice);
      reshade::unregister_event<reshade::addon_event::destroy_device>(OnDestroyDevice);
      reshade::unregister_event<reshade::addon_event::present>(OnPresentAdvanceFrame);
      reshade::unregister_event<reshade::addon_event::push_descriptors>(OnPushDescriptorsCaptureLightingTextures);
      reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(OnBindDescriptorTablesCaptureLightingTextures);
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::resource::Use(fdw_reason);
  renodx::utils::swapchain::Use(fdw_reason);
  renodx::utils::descriptor::trace_descriptor_tables = true;
  renodx::utils::descriptor::Use(fdw_reason);
  renodx::utils::settings::Use(fdw_reason, &settings);
  renodx::mods::shader::Use(fdw_reason, custom_shaders, &shader_injection);

  return TRUE;
}
