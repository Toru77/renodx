// Native harness: RTGI (world/rtgi): the trace and the denoise passes. Pure rules and the JSON text only.
//
// REAL: rtgi_state.hpp functions (trace divisor and extent, effective ray max), RtgiPushConstants layout,
//       ParameterSnapshot equality, the M3b push layouts, the M4 ShaderInjectData fields, BuildRtgiJson (the text
//       written to world_rtgi.json).
// TEXT: the M4 composite lines in shared.h and the Sora 2nd lighting shader (read from the repo).
// SOURCE GUARD: the RTGI_STAT_* defines in world_rtgi.cs_5_0.hlsl match the kRtgiStat* constants (the harness copy).
// NOT COVERED: rtgi::Dispatch on a device, RunRtgiInline (addon.cpp), the shader on the GPU, the present blit.
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <map>
#include <span>
#include <string>
#include <vector>

#define __world_rtao_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao{__world_rtao_base};
#define __world_rtao_temporal_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_temporal_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao_temporal{__world_rtao_temporal_base};
#define __world_rtao_filter_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_filter_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao_filter{__world_rtao_filter_base};
#define __world_rtgi_EMBED_FILE
inline constexpr std::uint8_t __world_rtgi_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtgi{__world_rtgi_base};
#define __world_rtgi_temporal_EMBED_FILE
inline constexpr std::uint8_t __world_rtgi_temporal_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtgi_temporal{__world_rtgi_temporal_base};
#define __world_rtgi_filter_EMBED_FILE
inline constexpr std::uint8_t __world_rtgi_filter_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtgi_filter{__world_rtgi_filter_base};
#define __world_rtgi_upsample_EMBED_FILE
inline constexpr std::uint8_t __world_rtgi_upsample_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtgi_upsample{__world_rtgi_upsample_base};

#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "json_check.hpp"
#include "src/games/falcomengine-plus/world/rtgi/rtgi.hpp"
#include "../../src/games/falcomengine-plus/shared.h"  // REAL: compiled; the ShaderInjectData RTGI fields must exist

using namespace reshade::api;
namespace rtgi = falcom_world::rtgi;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static std::string ReadText(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static void TestTraceDivisor() {
  CHECK(falcom_world::rtao::RtgiTraceDivisor(-1.f) == 1u, "setting -1 gives 1");
  CHECK(falcom_world::rtao::RtgiTraceDivisor(0.f) == 1u, "setting 0 (Full) gives 1");
  CHECK(falcom_world::rtao::RtgiTraceDivisor(1.f) == 2u, "setting 1 (Half) gives 2");
  CHECK(falcom_world::rtao::RtgiTraceDivisor(2.f) == 4u, "setting 2 (Quarter) gives 4");
  CHECK(falcom_world::rtao::RtgiTraceDivisor(3.f) == 4u, "setting 3 clamps to 4");
}

static void TestTraceExtent() {
  CHECK(falcom_world::rtao::RtgiTraceExtent(1920u, 2u) == 960u, "1920 / 2");
  CHECK(falcom_world::rtao::RtgiTraceExtent(1919u, 2u) == 960u, "1919 / 2 rounds up");
  CHECK(falcom_world::rtao::RtgiTraceExtent(1080u, 4u) == 270u, "1080 / 4");
  CHECK(falcom_world::rtao::RtgiTraceExtent(1081u, 4u) == 271u, "1081 / 4 rounds up");
  CHECK(falcom_world::rtao::RtgiTraceExtent(7u, 1u) == 7u, "full resolution is unchanged");
}

static void TestEffectiveRayMax() {
  CHECK(rtgi::RtgiEffectiveRayMax(16.f, 256.f, 80.f, 10.f) == 16.f, "requested is kept when it fits");
  CHECK(rtgi::RtgiEffectiveRayMax(64.f, 256.f, 256.f, 10.f) == 10.f, "capped by built range + margin - fade end");
  CHECK(rtgi::RtgiEffectiveRayMax(64.f, 64.f, 20.f, 10.f) == 54.f, "small range");
  CHECK(rtgi::RtgiEffectiveRayMax(5.f, 100.f, 100.f, 10.f) == 5.f, "short request below the cap");
}

static void TestRtaoMaxDarkening() {
  CHECK(rtgi::RtgiRtaoMaxDarkening(0.5f) == 0.9f, "default 0.5 becomes 0.9 on bounce draws");
  CHECK(rtgi::RtgiRtaoMaxDarkening(0.f) == 0.f, "0 stays 0");
  CHECK(rtgi::RtgiRtaoMaxDarkening(0.6f) == 1.f, "1.08 is capped at 1");
  CHECK(rtgi::RtgiRtaoMaxDarkening(1.f) == 1.f, "1 stays 1");
}

static void TestPushLayout() {
  CHECK(sizeof(rtgi::RtgiPushConstants) == 32u * sizeof(float), "b12 is 32 floats");
  CHECK(offsetof(rtgi::RtgiPushConstants, shading) == 20u * sizeof(float), "b12 c5 offset");
  CHECK(offsetof(rtgi::RtgiPushConstants, sun) == 24u * sizeof(float), "b12 c6 offset");
  CHECK(sizeof(rtgi::RtgiTemporalPushConstants) == 20u * sizeof(float), "temporal b12 is 20 floats");
  CHECK(sizeof(rtgi::RtgiFilterPushConstants) == 12u * sizeof(float), "filter b12 is 12 floats");
  CHECK(sizeof(rtgi::RtgiUpsamplePushConstants) == 12u * sizeof(float), "upsample b12 is 12 floats");
}

static void TestParameterSnapshot() {
  rtgi::ParameterSnapshot a = {};
  rtgi::ParameterSnapshot b = {};
  CHECK(a == b, "equal snapshots compare equal");
  b.history_weight = 0.5f;
  CHECK(!(a == b), "a history weight change is a parameter change");
}

static void TestShaderDefines() {
  const std::string text = ReadText("src/games/falcomengine-plus/world/shaders/world_rtgi.cs_5_0.hlsl");
  CHECK(!text.empty(), "shader copy readable (run refresh_world.ps1 first)");
  CHECK(text.find("#define RTGI_FLAG_LAMP_SHADOWS " + std::to_string(rtgi::kRtgiFlagLampShadows) + "u") != std::string::npos,
        "lamp shadow flag define matches kRtgiFlagLampShadows");
  CHECK(text.find("LampDiffuseSum(hit.position, n, false)") != std::string::npos
            && text.find("LampDiffuseSum(hit.position, n, (flags & RTGI_FLAG_LAMP_SHADOWS) != 0u)") != std::string::npos,
        "on-screen estimate never traces; the fallback traces only under the flag");
  const struct {
    const char* name;
    uint32_t value;
  } stats[] = {
      {"RAYS", rtgi::kRtgiStatRays},
      {"HITS", rtgi::kRtgiStatHits},
      {"PIXELS", rtgi::kRtgiStatPixels},
      {"SKY", rtgi::kRtgiStatSky},
      {"NORMAL", rtgi::kRtgiStatNormal},
      {"REGION", rtgi::kRtgiStatRegion},
      {"BEYOND_FADE", rtgi::kRtgiStatBeyondFade},
      {"INVALID_REFS", rtgi::kRtgiStatInvalidRefs},
      {"STACK_OVERFLOW", rtgi::kRtgiStatStackOverflow},
      {"SCALED", rtgi::kRtgiStatScaled},
      {"HIT_ON_SCREEN", rtgi::kRtgiStatHitOnScreen},
      {"HIT_OCCLUDED", rtgi::kRtgiStatHitOccluded},
      {"HIT_OFF_SCREEN", rtgi::kRtgiStatHitOffScreen},
      {"HIT_DISTANCE_DM", rtgi::kRtgiStatHitDistanceDm},
      {"RADIANCE_LOOKUPS", rtgi::kRtgiStatRadianceLookups},
      {"SUN_RAYS", rtgi::kRtgiStatSunRays},
      {"SUN_VISIBLE", rtgi::kRtgiStatSunVisible},
      {"LUMINANCE_SUM", rtgi::kRtgiStatLuminanceSum},
      {"LUMINANCE_MAX", rtgi::kRtgiStatLuminanceMax},
      {"CLAMPED", rtgi::kRtgiStatClamped},
      {"LUMINANCE_SUM_HI", rtgi::kRtgiStatLuminanceSumHi},
  };
  for (const auto& stat : stats) {
    const std::string line = std::string("#define RTGI_STAT_") + stat.name + " " + std::to_string(stat.value) + "u";
    CHECK(text.find(line) != std::string::npos, "%s", line.c_str());
  }
  CHECK(text.find("#define RTGI_STAT_TRACE_COUNT 20u") != std::string::npos, "trace count 20");
  CHECK(text.find("#define RTGI_SCENE_ROWS_FIRST 50u") != std::string::npos, "scene rows first register 50");
  CHECK(text.find("#define RTGI_FLAG_SUBTRACT_PREVIOUS " + std::to_string(rtgi::kRtgiFlagSubtractPrevious) + "u") != std::string::npos,
        "subtract flag define matches kRtgiFlagSubtractPrevious");
  CHECK(text.find("#define RTGI_FLAG_SUN_CASTERS_ONLY " + std::to_string(rtgi::kRtgiFlagSunCastersOnly) + "u")
            != std::string::npos,
        "sun casters flag define matches kRtgiFlagSunCastersOnly");
  CHECK(text.find("#define RTGI_FLAG_FALLBACK_NO_ALBEDO " + std::to_string(rtgi::kRtgiFlagFallbackNoAlbedo) + "u")
            != std::string::npos,
        "fallback no albedo flag define matches kRtgiFlagFallbackNoAlbedo");
  CHECK(text.find("#define RTGI_FLAG_GAME_SUN " + std::to_string(rtgi::kRtgiFlagGameSun) + "u") != std::string::npos,
        "game sun flag define matches kRtgiFlagGameSun");
  CHECK(text.find("g_rtgi_sun : packoffset(c6)") != std::string::npos, "trace cb_rtgi has c6 (Sun Shadow Distance)");
  CHECK(text.find("RTGI_SUN_RAY_MAX") == std::string::npos, "the fixed sun ray length is gone");
  const std::string temporal = ReadText("src/games/falcomengine-plus/world/shaders/world_rtgi_temporal.cs_5_0.hlsl");
  CHECK(!temporal.empty(), "temporal shader copy readable");
  const struct {
    const char* name;
    uint32_t value;
  } temporal_stats[] = {
      {"TEMPORAL_PIXELS", rtgi::kRtgiStatTemporalPixels},
      {"NO_HISTORY", rtgi::kRtgiStatNoHistory},
      {"RESET_PIXELS", rtgi::kRtgiStatResetPixels},
  };
  for (const auto& stat : temporal_stats) {
    const std::string line = std::string("#define RTGI_STAT_") + stat.name + " " + std::to_string(stat.value) + "u";
    CHECK(temporal.find(line) != std::string::npos, "%s", line.c_str());
  }
  for (const char* path : {"src/games/falcomengine-plus/world/shaders/world_rtgi_temporal.cs_5_0.hlsl",
                           "src/games/falcomengine-plus/world/shaders/world_rtgi_filter.cs_5_0.hlsl",
                           "src/games/falcomengine-plus/world/shaders/world_rtgi_upsample.cs_5_0.hlsl"}) {
    CHECK(ReadText(path).find("#include \"world_rt_denoise.hlsli\"") != std::string::npos, "%s includes the shared denoise math", path);
  }
}

// Lighting shaders are outside world/ (not copied by refresh_world.ps1): read from the repo, two levels up from the
// harness folder, as test_rtao TestDebugShowField.
struct Variant {
  const char* path;
  rtgi::SceneLayout layout;
};
static const Variant variants[] = {
    {"../../src/games/falcomengine-plus/sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl", rtgi::SceneLayout::Sora2nd},
    {"../../src/games/falcomengine-plus/sora1st/lighting/lighting_0xFDAAF80E.ps_5_0.hlsl", rtgi::SceneLayout::Sora1st},
    {"../../src/games/falcomengine-plus/kai/lighting/lighting_0x430ED091.ps_5_0.hlsl", rtgi::SceneLayout::Kai},
    {"../../src/games/falcomengine-plus/kai/lighting/lightingsoft_0xF6C55E5F.ps_5_0.hlsl", rtgi::SceneLayout::Kai},
};
static const char* const shared_offsets[] = {
    "viewInv_g : packoffset(c4)", "viewProj_g : packoffset(c16)", "viewProjInv_g : packoffset(c20)",
    "lightColor_g : packoffset(c25)", "lightDirection_g : packoffset(c26)", "lightProbe_g[9] : packoffset(c37)",
    "resolutionScaling_g : packoffset(c49)"};

static void TestCbSceneOffsets() {
  const std::string rtgi_text = ReadText("src/games/falcomengine-plus/world/shaders/world_rtgi.cs_5_0.hlsl");
  CHECK(!rtgi_text.empty(), "RTGI shader readable");
  for (const char* offset : shared_offsets) {
    CHECK(rtgi_text.find(offset) != std::string::npos, "RTGI cb_scene has %s", offset);
  }
  CHECK(rtgi_text.find("g_scene_rows[46] : packoffset(c50)") != std::string::npos, "RTGI scene row window c50..c95");
  CHECK(rtgi_text.find("prevViewProj_g : packoffset") == std::string::npos, "world_rtgi.cs_5_0.hlsl has no prevViewProj_g cbuffer member (read through g_scene_rows)");
  for (const Variant& v : variants) {
    const std::string text = ReadText(v.path);
    CHECK(!text.empty(), "%s readable", v.path);
    for (const char* offset : shared_offsets) {
      CHECK(text.find(offset) != std::string::npos, "%s has %s", v.path, offset);
    }
    const std::string prev = "prevViewProj_g : packoffset(c" + std::to_string(rtgi::RtgiPrevViewProjRegister(v.layout)) + ")";
    CHECK(text.find(prev) != std::string::npos, "%s has %s", v.path, prev.c_str());
    const uint32_t reg = rtgi::RtgiPrevViewProjRegister(v.layout);
    CHECK(reg >= 50u && reg <= 92u, "%s register %u in c50..c92", v.path, reg);
  }
  CHECK(rtgi::RtgiPrevViewProjRegister(rtgi::SceneLayout::Unknown) == 0u, "unknown layout has no register");
}

// M4: the ShaderInjectData fields (REAL compile) and the composite text in shared.h and the Sora 2nd lighting shader.
static void TestComposite() {
  ShaderInjectData injection = {};
  injection.rtgi_gi_bound = 1.f;
  injection.rtgi_intensity = 2.f;
  injection.rtgi_char_intensity = 0.5f;
  CHECK(injection.rtgi_gi_bound == 1.f && injection.rtgi_intensity == 2.f && injection.rtgi_char_intensity == 0.5f,
        "ShaderInjectData RTGI fields compile and hold their values");
  const std::string header = ReadText("../../src/games/falcomengine-plus/shared.h");
  CHECK(!header.empty(), "shared.h readable from the harness directory");
  for (const char* field : {"float rtgi_gi_bound;", "float rtgi_intensity;", "float rtgi_char_intensity;", "float rtgi_reserved;"}) {
    CHECK(header.find(field) != std::string::npos && header.find(field) == header.rfind(field), "shared.h declares %s once", field);
  }
  for (const char* old_name : {"dynCube_reserved_3", "dynCube_reserved_4", "dynCube_reserved_5", "char_shadow_light_screen_fade_end", "float rtgi_debug_show"}) {
    CHECK(header.find(old_name) == std::string::npos, "shared.h no longer has %s", old_name);
  }
  const std::string lighting = ReadText(variants[0].path);
  CHECK(!lighting.empty(), "Sora 2nd lighting shader readable");
  for (const char* line : {"Texture2D<float4> rtgiTexture : register(t37);", "register(t37)", "float3 cachedRTGI = float3(0, 0, 0);",
                           "o0.xyz += cachedRTGI;",
                           "o0.xyz += cachedRTGI * (csCharacterShading ? saturate(shader_injection_data.rtgi_char_intensity) : 1.0);"}) {
    CHECK(lighting.find(line) != std::string::npos && lighting.find(line) == lighting.rfind(line), "lighting shader has %s once", line);
  }
  CHECK(lighting.find("rtgi_debug_show") == std::string::npos, "the lighting shader no longer reads rtgi_debug_show");
}

static void TestJson() {
  const rtgi::RtgiFrameState frame = {};
  const uint32_t values[rtgi::kRtgiStatsCount] = {};
  const rtgi::RtgiJsonInput in = {.frame = &frame, .values = values, .generated_frame = 0u, .perf_gather = false};
  const std::string text = rtgi::BuildRtgiJson(in);
  Validator validator(text);
  CHECK(validator.Run(), "json is valid: %s", validator.error().c_str());
  CHECK(text.find("\"schema\": 1") != std::string::npos, "schema key");
  CHECK(text.find("\"stats\": {") != std::string::npos, "stats object");
  CHECK(text.find("\"hit_off_screen\": ") != std::string::npos, "hit_off_screen key");
  CHECK(text.find("\"lamp_shadows\": ") != std::string::npos, "lamp_shadows key");
  CHECK(text.find("\"perf_gather\": false") != std::string::npos, "perf_gather key");
  CHECK(text.find("\"radiance\": {") != std::string::npos, "radiance object");
  CHECK(text.find("\"luminance_max\": ") != std::string::npos, "luminance_max key");
  CHECK(text.find("\"luminance_sum_x10\": ") != std::string::npos, "luminance_sum_x10 key");
  CHECK(text.find("\"luminance_stat_cap\": ") != std::string::npos, "luminance_stat_cap key");
  CHECK(text.find("\"prev_view_proj_register\": ") != std::string::npos, "prev_view_proj_register key");
  CHECK(text.find("\"sun_shadow_distance\": ") != std::string::npos, "sun_shadow_distance key");
  CHECK(text.find("\"temporal\": {") != std::string::npos, "temporal object");
  CHECK(text.find("\"spatial_filter\": {") != std::string::npos, "spatial_filter object");
  CHECK(text.find("\"temporal_pixels\": ") != std::string::npos, "temporal_pixels key");
  CHECK(text.find("\"denoise_gpu_ms\": ") != std::string::npos, "denoise_gpu_ms key");
  CHECK(text.find("\"composite\": {") != std::string::npos, "composite object");
  CHECK(text.find("\"subtract_previous\": false") != std::string::npos, "subtract_previous key");
  CHECK(text.find("\"saturation\": ") != std::string::npos, "saturation key");
}

// ---- Recording mock (rtgi::Dispatch on a device): every call appends its arguments to g_golden_log ----
static std::vector<uint64_t> g_golden_log;

static void LogWords(const void* values, uint32_t count) {
  const auto* words = static_cast<const uint32_t*>(values);
  for (uint32_t i = 0; i < count; ++i) g_golden_log.push_back(words[i]);
}

static uint64_t Fnv1a64(const std::vector<uint64_t>& words) {
  uint64_t hash = 1469598103934665603ull;
  for (const uint64_t word : words) {
    for (int shift = 0; shift < 64; shift += 8) {
      hash ^= (word >> shift) & 0xffu;
      hash *= 1099511628211ull;
    }
  }
  return hash;
}

struct GoldenDevice : mock::DeviceBase {
  std::map<uint64_t, resource_desc> res;
  std::map<uint64_t, uint64_t> view_of;  // resource view -> resource
  uint64_t next = 0x1000;
  device_api get_api() const override { return device_api::d3d11; }
  resource_desc get_resource_desc(resource r) const override { return res.at(r.handle); }
  resource get_resource_from_view(resource_view view) const override { return {view_of.at(view.handle)}; }
  bool create_sampler(const sampler_desc &, sampler *out) override {
    g_golden_log.push_back(7u);
    out->handle = next;
    next += 0x100;
    return true;
  }
  bool create_resource(const resource_desc &desc, const subresource_data *, resource_usage, resource *out, void ** = nullptr) override {
    g_golden_log.push_back(1u);
    g_golden_log.push_back(static_cast<uint64_t>(desc.type));
    if (desc.type == resource_type::buffer) {
      g_golden_log.push_back(desc.buffer.size);
    } else {
      g_golden_log.push_back(desc.texture.width);
      g_golden_log.push_back(desc.texture.height);
      g_golden_log.push_back(static_cast<uint64_t>(desc.texture.format));
    }
    out->handle = next;
    next += 0x100;
    res[out->handle] = desc;
    return true;
  }
  bool create_resource_view(resource r, resource_usage, const resource_view_desc &, resource_view *out) override {
    g_golden_log.push_back(2u);
    g_golden_log.push_back(r.handle);
    out->handle = next;
    next += 0x100;
    view_of[out->handle] = r.handle;
    return true;
  }
  bool create_pipeline_layout(uint32_t count, const pipeline_layout_param *params, pipeline_layout *out) override {
    g_golden_log.push_back(4u);
    g_golden_log.push_back(count);
    for (uint32_t i = 0; i < count; ++i) g_golden_log.push_back(static_cast<uint64_t>(params[i].type));
    out->handle = next;
    next += 0x100;
    return true;
  }
  bool create_pipeline(pipeline_layout layout, uint32_t count, const pipeline_subobject *, pipeline *out) override {
    g_golden_log.push_back(5u);
    g_golden_log.push_back(layout.handle);
    g_golden_log.push_back(count);
    out->handle = next;
    next += 0x100;
    return true;
  }
  bool allocate_descriptor_tables(uint32_t count, pipeline_layout layout, uint32_t param, descriptor_table *out) override {
    g_golden_log.push_back(6u);
    g_golden_log.push_back(count);
    g_golden_log.push_back(layout.handle);
    g_golden_log.push_back(param);
    for (uint32_t i = 0; i < count; ++i) out[i].handle = next++;
    return true;
  }
  void update_descriptor_tables(uint32_t count, const descriptor_table_update *updates) override {
    for (uint32_t i = 0; i < count; ++i) {
      const auto& update = updates[i];
      g_golden_log.push_back(8u);
      g_golden_log.push_back(update.table.handle);
      g_golden_log.push_back(update.binding);
      g_golden_log.push_back(update.count);
      g_golden_log.push_back(static_cast<uint64_t>(update.type));
      for (uint32_t j = 0; j < update.count; ++j) {
        if (update.type == descriptor_type::sampler) {
          g_golden_log.push_back(static_cast<const sampler *>(update.descriptors)[j].handle);
        } else {
          g_golden_log.push_back(static_cast<const resource_view *>(update.descriptors)[j].handle);
        }
      }
    }
  }
  void update_buffer_region(const void *, resource dest, uint64_t offset, uint64_t size) override {
    g_golden_log.push_back(9u);
    g_golden_log.push_back(dest.handle);
    g_golden_log.push_back(offset);
    g_golden_log.push_back(size);
  }
};

struct GoldenCmdList : mock::CommandListBase {
  GoldenDevice* dev = nullptr;
  int dispatches = 0;
  device *get_device() override { return dev; }
  void barrier(uint32_t count, const resource *resources, const resource_usage *old_states, const resource_usage *new_states) override {
    g_golden_log.push_back(15u);
    g_golden_log.push_back(count);
    for (uint32_t i = 0; i < count; ++i) {
      g_golden_log.push_back(resources[i].handle);
      g_golden_log.push_back(static_cast<uint64_t>(old_states[i]));
      g_golden_log.push_back(static_cast<uint64_t>(new_states[i]));
    }
  }
  void bind_pipeline(pipeline_stage, pipeline pipeline) override {
    g_golden_log.push_back(10u);
    g_golden_log.push_back(pipeline.handle);
  }
  void bind_descriptor_tables(shader_stage, pipeline_layout layout, uint32_t first, uint32_t count, const descriptor_table *tables) override {
    g_golden_log.push_back(11u);
    g_golden_log.push_back(layout.handle);
    g_golden_log.push_back(first);
    g_golden_log.push_back(count);
    for (uint32_t i = 0; i < count; ++i) g_golden_log.push_back(tables[i].handle);
  }
  void push_constants(shader_stage, pipeline_layout layout, uint32_t param, uint32_t first, uint32_t count, const void *values) override {
    g_golden_log.push_back(12u);
    g_golden_log.push_back(layout.handle);
    g_golden_log.push_back(param);
    g_golden_log.push_back(first);
    g_golden_log.push_back(count);
    LogWords(values, count);
  }
  void push_descriptors(shader_stage, pipeline_layout layout, uint32_t param, const descriptor_table_update &update) override {
    g_golden_log.push_back(13u);
    g_golden_log.push_back(layout.handle);
    g_golden_log.push_back(param);
    g_golden_log.push_back(update.binding);
    g_golden_log.push_back(update.count);
    g_golden_log.push_back(static_cast<uint64_t>(update.type));
    for (uint32_t i = 0; i < update.count; ++i) g_golden_log.push_back(static_cast<const resource_view *>(update.descriptors)[i].handle);
  }
  void dispatch(uint32_t x, uint32_t y, uint32_t z) override {
    g_golden_log.push_back(14u);
    g_golden_log.push_back(x);
    g_golden_log.push_back(y);
    g_golden_log.push_back(z);
    dispatches += 1;
  }
  void copy_buffer_region(resource source, uint64_t source_offset, resource dest, uint64_t dest_offset, uint64_t size) override {
    g_golden_log.push_back(16u);
    g_golden_log.push_back(source.handle);
    g_golden_log.push_back(source_offset);
    g_golden_log.push_back(dest.handle);
    g_golden_log.push_back(dest_offset);
    g_golden_log.push_back(size);
  }
};

// The command log and dispatch count of five fixed rtgi::Dispatch calls (trace, spatial, temporal, lamps with debug 4,
// trace debug 9) on the recording mock. Recorded at HEAD; the refactors must reproduce them exactly.
static constexpr uint64_t kDispatchGoldenHash = 0x22868adf1653e774ull;
static constexpr int kDispatchGoldenCount = 16;

static void TestDispatchGolden() {
  GoldenDevice dev;
  GoldenCmdList cl;
  cl.dev = &dev;
  falcom_world::bvh::BvhDeviceData* bvh_data = falcom_world::bvh::GetBvhDeviceData(&dev);
  bvh_data->dynamic.instances_srv = {0x9400u};
  bvh_data->dynamic.instance_count = 3u;
  rtgi::RtgiDispatchInputs in = {};
  in.depth_view = {0x9000u};
  in.mrt_normal_view = {0x9200u};
  in.isfast_view = {0x9300u};
  in.scene_cbv_view = {0x9100u};
  in.radiance_view = {0x9500u};
  in.albedo_view = {0x9600u};
  in.full_width = 128u;
  in.full_height = 64u;
  in.trace_width = 64u;
  in.trace_height = 32u;
  in.push.params[0] = 16.f;
  in.push.params[1] = 0.1f;
  in.push.params[2] = 2.f;
  in.push.params[3] = 2.f;
  in.push.sampling[1] = 0.5f;
  in.push.shading[0] = 0.25f;
  in.push.extra[0] = 1.f;
  // 1. Trace and upsample only.
  CHECK(rtgi::Dispatch(&dev, &cl, in).ok, "case 1: trace and upsample");
  // 2. Plus the spatial filter.
  in.spatial = true;
  CHECK(rtgi::Dispatch(&dev, &cl, in).ok, "case 2: spatial filter");
  // 3. Plus the temporal pass (motion view set).
  in.temporal = true;
  in.motion_view = {0x9700u};
  CHECK(rtgi::Dispatch(&dev, &cl, in).ok, "case 3: temporal and spatial");
  // 4. Lamps (type pass) with the temporal debug view (mode 4), no spatial filter.
  in.spatial = false;
  in.debug_mode = 4;
  in.lights_view = {0x7000u};
  in.light_indices_view = {0x7100u};
  dev.view_of[0x7000u] = 0x7200u;
  resource_desc lights_desc = {};
  lights_desc.type = resource_type::buffer;
  lights_desc.buffer.size = 40u * rtgi::kLightParamBytes;
  dev.res[0x7200u] = lights_desc;
  CHECK(rtgi::Dispatch(&dev, &cl, in).ok, "case 4: lamp types, temporal debug");
  // 5. Trace debug view (mode 9), temporal and lamps off.
  in.debug_mode = 9;
  in.temporal = false;
  in.motion_view = {0u};
  in.lights_view = {0u};
  in.light_indices_view = {0u};
  CHECK(rtgi::Dispatch(&dev, &cl, in).ok, "case 5: trace debug view");
  const uint64_t hash = Fnv1a64(g_golden_log);
  CHECK(cl.dispatches == kDispatchGoldenCount, "dispatch count %d (golden %d)", cl.dispatches, kDispatchGoldenCount);
  CHECK(hash == kDispatchGoldenHash, "command log hash %016llx over %zu words (golden %016llx)",
        static_cast<unsigned long long>(hash), g_golden_log.size(), static_cast<unsigned long long>(kDispatchGoldenHash));
  rtgi::DestroyRtgiDeviceData(&dev);
}

int main() {
  HStage("dispatch golden");
  TestDispatchGolden();
  HStage("trace divisor");
  TestTraceDivisor();
  HStage("trace extent");
  TestTraceExtent();
  HStage("effective ray max");
  TestEffectiveRayMax();
  HStage("RTAO max darkening on bounce draws");
  TestRtaoMaxDarkening();
  HStage("push layout");
  TestPushLayout();
  HStage("parameter snapshot");
  TestParameterSnapshot();
  HStage("shader defines");
  TestShaderDefines();
  HStage("cb_scene offsets per lighting variant");
  TestCbSceneOffsets();
  HStage("composite");
  TestComposite();
  HStage("json");
  TestJson();
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
