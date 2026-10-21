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
#include <span>
#include <string>

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
  CHECK(rtgi::RtgiTraceDivisor(-1.f) == 1u, "setting -1 gives 1");
  CHECK(rtgi::RtgiTraceDivisor(0.f) == 1u, "setting 0 (Full) gives 1");
  CHECK(rtgi::RtgiTraceDivisor(1.f) == 2u, "setting 1 (Half) gives 2");
  CHECK(rtgi::RtgiTraceDivisor(2.f) == 4u, "setting 2 (Quarter) gives 4");
  CHECK(rtgi::RtgiTraceDivisor(3.f) == 4u, "setting 3 clamps to 4");
}

static void TestTraceExtent() {
  CHECK(rtgi::RtgiTraceExtent(1920u, 2u) == 960u, "1920 / 2");
  CHECK(rtgi::RtgiTraceExtent(1919u, 2u) == 960u, "1919 / 2 rounds up");
  CHECK(rtgi::RtgiTraceExtent(1080u, 4u) == 270u, "1080 / 4");
  CHECK(rtgi::RtgiTraceExtent(1081u, 4u) == 271u, "1081 / 4 rounds up");
  CHECK(rtgi::RtgiTraceExtent(7u, 1u) == 7u, "full resolution is unchanged");
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
  CHECK(sizeof(rtgi::RtgiPushConstants) == 28u * sizeof(float), "b12 is 28 floats");
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
  CHECK(text.find("#define RTGI_FLAG_DROP_FALLBACK " + std::to_string(rtgi::kRtgiFlagDropFallback) + "u")
            != std::string::npos,
        "drop fallback flag define matches kRtgiFlagDropFallback");
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
}

int main() {
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
