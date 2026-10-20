// Native harness: RTGI M1 (world/rtgi), the discovery trace. Pure rules and the JSON text only.
//
// REAL: rtgi_state.hpp functions (trace divisor and extent, effective ray max), RtgiPushConstants layout,
//       BuildRtgiJson (the text written to world_rtgi.json).
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

#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "json_check.hpp"
#include "src/games/falcomengine-plus/world/rtgi/rtgi.hpp"

using namespace reshade::api;
namespace rtgi = falcom_world::rtgi;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

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

static void TestPushLayout() {
  CHECK(sizeof(rtgi::RtgiPushConstants) == 24u * sizeof(float), "b12 is 24 floats");
  CHECK(offsetof(rtgi::RtgiPushConstants, shading) == 20u * sizeof(float), "b12 c5 offset");
}

static void TestShaderDefines() {
  std::ifstream file("src/games/falcomengine-plus/world/shaders/world_rtgi.cs_5_0.hlsl", std::ios::binary);
  const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
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
  };
  for (const auto& stat : stats) {
    const std::string line = std::string("#define RTGI_STAT_") + stat.name + " " + std::to_string(stat.value) + "u";
    CHECK(text.find(line) != std::string::npos, "%s", line.c_str());
  }
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
}

int main() {
  HStage("trace divisor");
  TestTraceDivisor();
  HStage("trace extent");
  TestTraceExtent();
  HStage("effective ray max");
  TestEffectiveRayMax();
  HStage("push layout");
  TestPushLayout();
  HStage("shader defines");
  TestShaderDefines();
  HStage("json");
  TestJson();
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
