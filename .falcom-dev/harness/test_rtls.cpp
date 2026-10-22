// Native harness: RT Local Shadows (world/rtls): the trace and the stats JSON. Pure rules, the push layout, the shader
// text and the device lifecycle on a recording mock.
//
// REAL: RtlsPushConstants layout, BuildRtlsJson (the text written to world_rtls.json), the shared lamp helpers in
//       world_rt_lamps.hlsli (text), the lifecycle of rtls::Dispatch on the recording mock (one dispatch, groups,
//       texture formats, destroy).
// TRANSCRIPTION: the cell index (world_rtls.cs_5_0.hlsl main), the point top-k insertion and the spot list-order cap, the shadow ratio and the fade, the lighting visibility and the spot cover lerp.
// TEXT: the RTLS_STAT_* / RTLS_KIND_* defines of world_rtls.cs_5_0.hlsl against the kRtls* constants (harness copy); the M3 lighting shader edits (t38, RTLS_FLAG_* against kRtlsFlag*, the four colour sites, the two spot cover sites, the contact gate, no RTLS code in the character branch), shared.h fields.
// NOT COVERED: the shader on the GPU, RunRtlsInline (addon.cpp),
//       the per-lamp sample rules (backlit, budget, Light Clearance), the present blit.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <span>
#include <string>
#include <vector>

#define __world_rtls_EMBED_FILE
inline constexpr std::uint8_t __world_rtls_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtls{__world_rtls_base};
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
#define __world_rtgi_lights_EMBED_FILE
inline constexpr std::uint8_t __world_rtgi_lights_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtgi_lights{__world_rtgi_lights_base};
#define __world_rtao_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao{__world_rtao_base};
#define __world_rtao_temporal_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_temporal_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao_temporal{__world_rtao_temporal_base};
#define __world_rtao_filter_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_filter_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao_filter{__world_rtao_filter_base};

#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "json_check.hpp"
#include "src/games/falcomengine-plus/world/rtls/rtls.hpp"
#include "../../src/games/falcomengine-plus/shared.h"  // REAL: compiled; ShaderInjectData rtls_* fields and size

using namespace reshade::api;
namespace rtls = falcom_world::rtls;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static std::string ReadText(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static void TestPushLayout() {
  CHECK(sizeof(rtls::RtlsPushConstants) == 28u * sizeof(float), "push constants are 28 floats");
  CHECK(offsetof(rtls::RtlsPushConstants, fade) == 8u * sizeof(float), "c2 offset");
  CHECK(offsetof(rtls::RtlsPushConstants, light) == 20u * sizeof(float), "c5 offset");
  CHECK(offsetof(rtls::RtlsPushConstants, extra) == 24u * sizeof(float), "c6 offset");
}

static void TestShaderDefines() {
  const std::string text = ReadText("src/games/falcomengine-plus/world/shaders/world_rtls.cs_5_0.hlsl");
  CHECK(!text.empty(), "shader copy readable (run refresh_world.ps1 first)");
  const struct {
    const char* name;
    uint32_t value;
  } stats[] = {
      {"PIXELS", rtls::kRtlsStatPixels},
      {"TRACED", rtls::kRtlsStatTraced},
      {"SKY", rtls::kRtlsStatSky},
      {"NORMAL", rtls::kRtlsStatNormal},
      {"REGION", rtls::kRtlsStatRegion},
      {"BEYOND_FADE", rtls::kRtlsStatBeyondFade},
      {"SCALED", rtls::kRtlsStatScaled},
      {"CHARACTER", rtls::kRtlsStatCharacter},
      {"LIGHTS_CONSIDERED", rtls::kRtlsStatLightsConsidered},
      {"LIGHTS_TRACED", rtls::kRtlsStatLightsTraced},
      {"POINTS_OVER_BUDGET", rtls::kRtlsStatPointsOverBudget},
      {"SPOTS_OVER_CAP", rtls::kRtlsStatSpotsOverCap},
      {"LIGHTS_BACKLIT", rtls::kRtlsStatLightsBacklit},
      {"RAYS", rtls::kRtlsStatRays},
      {"RAYS_BLOCKED", rtls::kRtlsStatRaysBlocked},
      {"RAYS_NEAR_LIGHT", rtls::kRtlsStatRaysNearLight},
      {"RAYS_CLIPPED", rtls::kRtlsStatRaysClipped},
      {"INVALID_REFS", rtls::kRtlsStatInvalidRefs},
      {"STACK_OVERFLOW", rtls::kRtlsStatStackOverflow},
      {"INDEX_OUT_OF_RANGE", rtls::kRtlsStatIndexOutOfRange},
      {"PER_LIGHT_RAYS", rtls::kRtlsStatPerLightRays},
      {"PER_LIGHT_NEAR", rtls::kRtlsStatPerLightNear},
  };
  for (const auto& s : stats) {
    const std::string line = "#define RTLS_STAT_" + std::string(s.name) + " " + std::to_string(s.value) + "u";
    CHECK(text.find(line) != std::string::npos, "shader has %s", line.c_str());
  }
  const struct {
    const char* line;
  } consts[] = {
      {"#define RTLS_STAT_COUNT 96u"},
      {"#define RTLS_KIND_POINT 1u"},
      {"#define RTLS_KIND_SPOT 2u"},
      {"#define RTLS_MAX_POINTS 10u"},
      {"#define RTLS_MAX_SPOTS 8u"},
      {"#define RTLS_PER_LIGHT_COUNT 32u"},
  };
  for (const auto& c : consts) CHECK(text.find(c.line) != std::string::npos, "shader has %s", c.line);
  CHECK(rtls::kRtlsStatsCount == 96u && rtls::kRtlsKindPoint == 1u && rtls::kRtlsKindSpot == 2u, "kRtls constants");
  CHECK(rtls::kRtlsMaxPoints == 10u && rtls::kRtlsMaxSpots == 8u && rtls::kRtlsPerLightCount == 32u, "kRtls limits");
  CHECK(text.find("view_g : packoffset(c0);") != std::string::npos, "cb_scene view_g c0");
  CHECK(text.find("viewInv_g : packoffset(c4);") != std::string::npos, "cb_scene viewInv_g c4");
  CHECK(text.find("viewProjInv_g : packoffset(c20);") != std::string::npos, "cb_scene viewProjInv_g c20");
  CHECK(text.find("lightTileSizeInv_g : packoffset(c29);") != std::string::npos, "cb_scene lightTileSizeInv_g c29");
  CHECK(text.find("resolutionScaling_g : packoffset(c49);") != std::string::npos, "cb_scene resolutionScaling_g c49");
  CHECK(text.find("const float view_z = dot(view_g._m02_m12_m22_m32, float4(P, 1.0));") != std::string::npos, "cell line: view_z");
  CHECK(text.find("const uint2 tile = (uint2)(lightTileSizeInv_g * (float2(px) + 0.5));") != std::string::npos, "cell line: tile");
  CHECK(text.find("const uint row = tile.y << 5;") != std::string::npos, "cell line: row");
  CHECK(text.find("const uint slice = (uint)clamp(g_rtls_light_indices[tile.x + row].tileDepthInv * -view_z, 0.0, 7.0);") != std::string::npos,
        "cell line: slice");
  CHECK(text.find("const uint cell = min(4607u, slice * 576u + row + tile.x);") != std::string::npos, "cell line: cell");
  const std::string lighting = ReadText("../../src/games/falcomengine-plus/sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl");
  if (!lighting.empty()) {
    CHECK(lighting.find("view_g : packoffset(c0)") != std::string::npos, "lighting shader view_g c0");
    CHECK(lighting.find("lightTileSizeInv_g : packoffset(c29)") != std::string::npos, "lighting shader lightTileSizeInv_g c29");
  }
}

static void TestSharedLamps() {
  const std::string lamps = ReadText("src/games/falcomengine-plus/world/shaders/world_rt_lamps.hlsli");
  CHECK(!lamps.empty(), "lamp header readable (run refresh_world.ps1 first)");
  CHECK(lamps.find("struct LightParam") != std::string::npos, "lamp header: LightParam");
  CHECK(lamps.find("struct LightIndexData") != std::string::npos, "lamp header: LightIndexData");
  CHECK(lamps.find("float LampDistanceAttenuation(LightParam lamp, float3 to_lamp)") != std::string::npos, "lamp header: attenuation");
  CHECK(lamps.find("float LampSpotCone(LightParam lamp, float3 lamp_dir)") != std::string::npos, "lamp header: spot cone");
  const std::string rtgi = ReadText("src/games/falcomengine-plus/world/shaders/world_rtgi.cs_5_0.hlsl");
  const std::string rtgi_lights = ReadText("src/games/falcomengine-plus/world/shaders/world_rtgi_lights.cs_5_0.hlsl");
  const std::string rtls_text = ReadText("src/games/falcomengine-plus/world/shaders/world_rtls.cs_5_0.hlsl");
  CHECK(rtgi.find("struct LightParam") == std::string::npos, "rtgi shader has no LightParam of its own");
  CHECK(rtgi_lights.find("struct LightIndexData") == std::string::npos, "rtgi lights shader has no LightIndexData of its own");
  for (const std::string* s : {&rtgi, &rtls_text}) {
    CHECK(s->find("LampDistanceAttenuation(") != std::string::npos, "shader calls LampDistanceAttenuation");
    CHECK(s->find("LampSpotCone(") != std::string::npos, "shader calls LampSpotCone");
  }
  const std::string trace = ReadText("src/games/falcomengine-plus/world/shaders/world_bvh_trace.hlsli");
  CHECK(trace.find("float2 TraceIgnNoise2(float2 q)") != std::string::npos, "trace include defines TraceIgnNoise2");
  for (const std::string* s : {&rtgi, &rtls_text}) {
    CHECK(s->find("TraceIgnNoise2(q)") != std::string::npos, "shader uses TraceIgnNoise2");
    CHECK(s->find("52.9829189") == std::string::npos, "shader has no IGN copy of its own");
  }
}

// Transcription of the cell index in world_rtls.cs_5_0.hlsl main: k = tileDepthInv * -view_z.
static uint32_t CellIndex(uint32_t tile_x, uint32_t tile_y, float k) {
  const uint32_t row = tile_y << 5;
  const uint32_t slice = static_cast<uint32_t>(std::clamp(k, 0.f, 7.f));
  return std::min(4607u, slice * 576u + row + tile_x);
}

static void TestCellTranscription() {
  CHECK(CellIndex(3u, 2u, 2.7f) == 1219u, "tile (3,2) slice 2: %u", CellIndex(3u, 2u, 2.7f));
  CHECK(CellIndex(0u, 0u, 7.9f) == 7u * 576u, "7.9 gives slice 7");
  CHECK(CellIndex(0u, 0u, -1.f) == 0u, "-1 gives slice 0");
  CHECK(CellIndex(31u, 17u, 7.f) == 4607u, "tile (31,17) slice 7");
  CHECK(CellIndex(40u, 30u, 7.f) == 4607u, "tile (40,30) slice 7 clamps to 4607");
}

// Transcription of the point top-k insertion in world_rtls.cs_5_0.hlsl main (10, descending).
static void TopKInsert(float (&top_w)[10], uint32_t (&top_id)[10], float w, uint32_t id) {
  for (uint32_t k = 0u; k < 10u; ++k) {
    if (w > top_w[k]) {
      std::swap(top_w[k], w);
      std::swap(top_id[k], id);
    }
  }
}

static void TestTopK() {
  uint32_t seed = 12345u;
  const auto next = [&]() {
    seed = seed * 1664525u + 1013904223u;
    return seed;
  };
  int mismatches = 0;
  for (int trial = 0; trial < 1000; ++trial) {
    const uint32_t n = next() % 64u;
    std::vector<float> w(n);
    float top_w[10] = {};
    uint32_t top_id[10] = {};
    for (uint32_t i = 0u; i < n; ++i) {
      w[i] = static_cast<float>(next() % 1000u) / 7.f;
      TopKInsert(top_w, top_id, w[i], i);
    }
    std::vector<float> sorted = w;
    std::sort(sorted.begin(), sorted.end(), std::greater<float>());
    for (uint32_t k = 0u; k < 10u; ++k) {
      const float expect = k < n ? sorted[k] : 0.f;
      if (top_w[k] != expect) ++mismatches;
    }
  }
  CHECK(mismatches == 0, "top-k mismatches over 1000 lists: %d", mismatches);
}

// Transcription of the shadow ratio: every contributing lamp's weight is in the denominator; only traced lamps add shadow
// (an untraced lamp, over the point budget, counts as unblocked). The fade scales x and y.
static float ShadowRatio(const float* w, const float* visible, const bool* traced, uint32_t n) {
  float shadow = 0.f;
  float sum = 0.f;
  for (uint32_t i = 0u; i < n; ++i) {
    sum += w[i];
    if (traced[i]) shadow += w[i] * (1.f - visible[i]);
  }
  return sum > 0.f ? shadow / sum : 0.f;
}

static void TestRatio() {
  const float w2[2] = {1.f, 1.f};
  const bool traced2[2] = {true, true};
  const float all_visible[2] = {1.f, 1.f};
  const float all_blocked[2] = {0.f, 0.f};
  const float one_blocked[2] = {0.f, 1.f};
  CHECK(ShadowRatio(w2, all_visible, traced2, 2u) == 0.f, "all visible gives 0");
  CHECK(ShadowRatio(w2, all_blocked, traced2, 2u) == 1.f, "all blocked gives 1");
  CHECK(ShadowRatio(w2, one_blocked, traced2, 2u) == 0.5f, "two equal lamps, one blocked gives 0.5");
  // Over the point budget: three equal lamps, budget 2, both traced lamps blocked. The third stays in the denominator.
  const float w3[3] = {1.f, 1.f, 1.f};
  const float blocked3[3] = {0.f, 0.f, 0.f};
  const bool budget2[3] = {true, true, false};
  CHECK(std::fabs(ShadowRatio(w3, blocked3, budget2, 3u) - 2.f / 3.f) < 1e-6f, "over-budget lamp counts as unblocked");
  // A zero-weight point (NdotL <= 0, no translucency) that is blocked: the ratio is the first point's alone, and point_sum
  // (transcribed: only w > 0 adds, as the shader does) is the first point's weight.
  const float w_zero[2] = {1.f, 0.f};
  const float blocked_zero[2] = {0.f, 0.f};
  const float first_alone_w[1] = {1.f};
  const float first_alone_vis[1] = {0.f};
  const bool first_alone_traced[1] = {true};
  float point_sum = 0.f;
  for (uint32_t i = 0u; i < 2u; ++i) {
    if (w_zero[i] > 0.f) point_sum += w_zero[i];
  }
  CHECK(ShadowRatio(w_zero, blocked_zero, traced2, 2u) == ShadowRatio(first_alone_w, first_alone_vis, first_alone_traced, 1u),
        "zero-weight blocked point leaves the ratio unchanged");
  CHECK(point_sum == 1.f, "zero-weight point does not add to point_sum (%f)", point_sum);
  // Fade 0.5 halves x and y (raw = (point ratio x f, spot ratio x f, f, 1)).
  const float f = 0.5f;
  const float x = ShadowRatio(w2, all_blocked, traced2, 2u) * f;
  const float y = ShadowRatio(w2, one_blocked, traced2, 2u) * f;
  CHECK(x == 0.5f && y == 0.25f, "fade 0.5 halves x (%f) and y (%f)", x, y);
}

// Transcription of the spot cap: the first RTLS_MAX_SPOTS contributing spots in list order; the rest are over the cap.
static void TestSpotCap() {
  float top_sw[8] = {};
  uint32_t spot_n = 0u;
  const float weights[11] = {5.f, 1.f, 9.f, 2.f, 7.f, 3.f, 8.f, 4.f, 6.f, 10.f, 11.f};
  for (const float w : weights) {
    if (spot_n < 8u) top_sw[spot_n] = w;
    ++spot_n;
  }
  CHECK(top_sw[0] == 5.f && top_sw[7] == 4.f, "first 8 kept in list order");
  CHECK(spot_n - std::min(spot_n, 8u) == 3u, "3 spots over the cap");
}

// Counts non-overlapping occurrences of needle in text.
static int Count(const std::string& text, const std::string& needle) {
  int n = 0;
  for (size_t p = text.find(needle); p != std::string::npos; p = text.find(needle, p + needle.size())) ++n;
  return n;
}

// M3: the lighting shader edits (read from the repo; refresh_world does not copy sora2nd) and the shared.h fields.
static void TestLightingApply() {
  const std::string lighting = ReadText("../../src/games/falcomengine-plus/sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl");
  CHECK(!lighting.empty(), "lighting shader readable from the repo");
  CHECK(Count(lighting, "Texture2D<float4> rtlsTexture : register(t38);") == 1, "t38 declared once");
  CHECK(Count(lighting, "register(t38)") == 1, "t38 used by one declaration only");
  const struct {
    const char* name;
    uint32_t value;
  } flags[] = {
      {"REQUESTED", rtls::kRtlsFlagRequested}, {"BOUND", rtls::kRtlsFlagBound}, {"POINT", rtls::kRtlsFlagPoint},
      {"SPOT", rtls::kRtlsFlagSpot}, {"ENV_POINT", rtls::kRtlsFlagEnvPoint}, {"ENV_SPOT", rtls::kRtlsFlagEnvSpot},
  };
  for (const auto& f : flags) {
    const std::string line = "#define RTLS_FLAG_" + std::string(f.name) + " " + std::to_string(f.value) + "u";
    CHECK(Count(lighting, line) == 1, "lighting shader has %s", line.c_str());
  }
  CHECK(Count(lighting, "const bool csLocalOn = shader_injection_data.cs_contact_local_enabled > 0.5f && (rtlsFlags & RTLS_FLAG_REQUESTED) == 0u;") == 1,
        "contact march off while RTLS is requested");
  CHECK(Count(lighting, "if (r5.z != 0 && rtlsSpotCover < 1.0) {") == 2, "both spot shadow-map blocks skipped under full cover");
  CHECK(Count(lighting, "r5.z = lerp(r5.z, 1.0, rtlsSpotCover);") == 2, "both spot shadow-map results blend by cover");
  CHECK(Count(lighting, "r13.xyz *= rtlsPointVis;") == 1 && Count(lighting, "r11.xyz *= rtlsPointVis;") == 1, "point colour sites");
  CHECK(Count(lighting, "r19.xyz *= rtlsSpotVis;") == 1 && Count(lighting, "r12.xyz *= rtlsSpotVis;") == 1, "spot colour sites");
  CHECK(Count(lighting, "SampleCmpLevelZero") == 10, "the ten spot shadow-map taps are unchanged");
  // Character exclusion: no RTLS code in main before the first environment line after the character branch.
  const size_t main_at = lighting.find("void main(");
  const size_t env_at = lighting.find("r3.zw = r4.xz * cloudShadowScale_g + cloudShadowOffset_g.xy;");
  const size_t first_rtls_in_main = lighting.find("rtls", main_at);
  CHECK(main_at != std::string::npos && env_at != std::string::npos && first_rtls_in_main > env_at,
        "no RTLS code in the character branch (first use at %zu, environment path at %zu)", first_rtls_in_main, env_at);
  const std::string header = ReadText("../../src/games/falcomengine-plus/shared.h");
  CHECK(!header.empty(), "shared.h readable");
  for (const char* field : {"float rtls_flags;", "float rtls_strength;", "float rtls_max_darkening;"}) {
    CHECK(Count(header, field) == 1, "shared.h declares %s once", field);
  }
  for (const char* old_name : {"local_sss_enabled", "local_sss_strength", "local_sss_light_type"}) {
    CHECK(Count(header, old_name) == 0, "shared.h no longer has %s", old_name);
  }
  ShaderInjectData injection = {};
  injection.rtls_flags = 3.f;
  injection.rtls_strength = 0.5f;
  injection.rtls_max_darkening = 0.25f;
  CHECK(injection.rtls_flags == 3.f && injection.rtls_strength == 0.5f && injection.rtls_max_darkening == 0.25f,
        "ShaderInjectData rtls_* fields compile and hold their values");
  CHECK(sizeof(ShaderInjectData) == 1756u, "ShaderInjectData stays 1756 bytes (%zu)", sizeof(ShaderInjectData));
}

// Transcription of the lighting visibility (F2) and the spot cover lerp (F6, F10).
static float LampVisibility(float shadow, float strength, float max_darkening) {
  return 1.f - std::min(shadow * strength, max_darkening);
}

static void TestLightingMath() {
  CHECK(LampVisibility(0.f, 1.f, 1.f) == 1.f, "no shadow: visibility 1");
  CHECK(LampVisibility(0.75f, 1.f, 1.f) == 0.25f, "defaults: visibility = 1 - shadow");
  CHECK(LampVisibility(1.f, 1.f, 0.5f) == 0.5f, "Max Darkening 0.5 caps a full shadow at half");
  CHECK(LampVisibility(0.25f, 1.f, 0.5f) == 0.75f, "Max Darkening leaves lighter shadows unchanged");
  CHECK(LampVisibility(1.f, 0.f, 1.f) == 1.f, "Strength 0: no RT shadow");
  // lerp(a, 1, c) as HLSL computes it: a + c * (1 - a). Cover 0 must return the game's PCF value bit-exactly.
  const float pcf[4] = {0.f, 0.2f, 0.6f, 1.f};
  for (const float a : pcf) {
    CHECK(a + 0.f * (1.f - a) == a, "cover 0 keeps the game shadow %f", a);
    CHECK(a + 1.f * (1.f - a) == 1.f, "cover 1 removes the game shadow %f", a);
  }
}

static void TestJson() {
  const rtls::RtlsFrameState frame = {};
  const uint32_t values[rtls::kRtlsStatsCount] = {};
  const rtls::RtlsJsonInput in = {.frame = &frame, .values = values, .generated_frame = 0u};
  const std::string text = rtls::BuildRtlsJson(in);
  Validator validator(text);
  CHECK(validator.Run(), "json is valid: %s", validator.error().c_str());
  CHECK(text.find("\"schema\": 1") != std::string::npos, "schema key");
  CHECK(text.find("\"input_reason\"") != std::string::npos, "input_reason key");
  CHECK(text.find("\"stats\": {") != std::string::npos, "stats object");
  CHECK(text.find("\"per_light\": [") != std::string::npos, "per_light array");
  CHECK(text.find("\"ray_max\": {") != std::string::npos, "ray_max object");
  CHECK(text.find("\"fade\": {") != std::string::npos, "fade object");
  CHECK(text.find("\"temporal\": {") != std::string::npos && text.find("\"spatial_filter\": {") != std::string::npos,
        "temporal and spatial_filter objects");
  CHECK(text.find("\"divisor\": ") != std::string::npos && text.find("\"temporal_pixels\": ") != std::string::npos,
        "trace divisor and temporal counters");
}

// Recording mock (same shape as test_rtgi's golden device): resource creation, descriptor tables and dispatches.
struct RecDevice : mock::DeviceBase {
  std::map<uint64_t, resource_desc> res;
  std::map<uint64_t, uint64_t> view_of;
  uint64_t next = 0x1000;
  std::map<uint64_t, std::vector<resource_view>> views_of_table;  // SRV and UAV descriptors per table (last update)
  int texture_creates = 0;
  device_api get_api() const override { return device_api::d3d11; }
  resource_desc get_resource_desc(resource r) const override { return res.at(r.handle); }
  resource get_resource_from_view(resource_view view) const override { return {view_of.at(view.handle)}; }
  bool create_resource(const resource_desc &desc, const subresource_data *, resource_usage, resource *out, void ** = nullptr) override {
    if (desc.type == resource_type::texture_2d) texture_creates += 1;
    out->handle = next;
    next += 0x100;
    res[out->handle] = desc;
    return true;
  }
  bool create_resource_view(resource r, resource_usage, const resource_view_desc &, resource_view *out) override {
    out->handle = next;
    next += 0x100;
    view_of[out->handle] = r.handle;
    return true;
  }
  bool create_sampler(const sampler_desc &, sampler *out) override {
    out->handle = next;
    next += 0x100;
    return true;
  }
  void destroy_resource(resource r) override { res.erase(r.handle); }
  void destroy_resource_view(resource_view v) override { view_of.erase(v.handle); }
  void update_descriptor_tables(uint32_t count, const descriptor_table_update *updates) override {
    for (uint32_t i = 0; i < count; ++i) {
      if (updates[i].type == descriptor_type::shader_resource_view || updates[i].type == descriptor_type::unordered_access_view) {
        const auto* views = static_cast<const resource_view *>(updates[i].descriptors);
        views_of_table[updates[i].table.handle].assign(views, views + updates[i].count);
      }
    }
  }
  bool create_pipeline_layout(uint32_t, const pipeline_layout_param *, pipeline_layout *out) override {
    out->handle = next;
    next += 0x100;
    return true;
  }
  bool create_pipeline(pipeline_layout, uint32_t, const pipeline_subobject *, pipeline *out) override {
    out->handle = next;
    next += 0x100;
    return true;
  }
  bool allocate_descriptor_tables(uint32_t count, pipeline_layout, uint32_t, descriptor_table *out) override {
    for (uint32_t i = 0; i < count; ++i) out[i].handle = next++;
    return true;
  }
  void update_buffer_region(const void *, resource, uint64_t, uint64_t) override {}
};

struct RecCmdList : mock::CommandListBase {
  RecDevice* dev = nullptr;
  int dispatches = 0;
  uint32_t groups_x = 0u;
  uint32_t groups_y = 0u;
  resource_view cs_srv[32] = {};
  resource_view cs_uav[8] = {};
  struct Snap { resource_view srv[32]; resource_view uav[8]; };
  std::vector<Snap> snaps;   // the slots at each dispatch
  std::vector<float> push;   // the last push of kRtlsPushConstantCount floats
  std::vector<uint32_t> null_srv_counts;  // the count of each all-null SRV push, in order
  std::vector<uint32_t> null_uav_counts;
  device *get_device() override { return dev; }
  void bind_pipeline(pipeline_stage, pipeline) override {}
  void bind_descriptor_tables(shader_stage, pipeline_layout layout, uint32_t first, uint32_t count, const descriptor_table *tables) override {
    if (layout.handle == 0u) return;
    for (uint32_t i = 0; i < count; ++i) {
      const auto& views = dev->views_of_table[tables[i].handle];
      if (first + i == 1u) for (size_t j = 0; j < views.size() && j < 32; ++j) cs_srv[j] = views[j];
      if (first + i == 2u) for (size_t j = 0; j < views.size() && j < 8; ++j) cs_uav[j] = views[j];
    }
  }
  void push_constants(shader_stage, pipeline_layout, uint32_t, uint32_t, uint32_t count, const void *values) override {
    if (count != rtls::kRtlsPushConstantCount) return;
    const float* f = static_cast<const float *>(values);
    push.assign(f, f + count);
  }
  void push_descriptors(shader_stage, pipeline_layout layout, uint32_t, const descriptor_table_update &update) override {
    if (layout.handle != 0u) return;
    const auto* views = static_cast<const resource_view *>(update.descriptors);
    bool all_null = true;
    for (uint32_t i = 0; i < update.count; ++i) {
      all_null = all_null && views[i].handle == 0u;
      if (update.type == descriptor_type::texture_shader_resource_view) cs_srv[update.binding + i] = views[i];
      if (update.type == descriptor_type::texture_unordered_access_view) cs_uav[update.binding + i] = views[i];
    }
    if (all_null && update.type == descriptor_type::texture_shader_resource_view) null_srv_counts.push_back(update.count);
    if (all_null && update.type == descriptor_type::texture_unordered_access_view) null_uav_counts.push_back(update.count);
  }
  void barrier(uint32_t, const resource *, const resource_usage *, const resource_usage *) override {}
  void dispatch(uint32_t x, uint32_t y, uint32_t) override {
    dispatches += 1;
    groups_x = x;
    groups_y = y;
    Snap snap = {};
    for (size_t j = 0; j < 32; ++j) snap.srv[j] = cs_srv[j];
    for (size_t j = 0; j < 8; ++j) snap.uav[j] = cs_uav[j];
    snaps.push_back(snap);
  }
};

static void TestDeviceLifecycle() {
  RecDevice dev;
  RecCmdList cl;
  cl.dev = &dev;
  falcom_world::bvh::BvhDeviceData* bvh_data = falcom_world::bvh::GetBvhDeviceData(&dev);
  bvh_data->dynamic.instances_srv = {0x9400u};
  bvh_data->dynamic.instance_count = 3u;
  rtls::RtlsDispatchInputs in = {};
  in.depth_view = {0x9000u};
  in.mrt_normal_view = {0x9200u};
  in.scene_cbv_view = {0x9100u};
  in.lights_view = {0x7000u};
  in.light_indices_view = {0x7100u};
  in.full_width = 64u;
  in.full_height = 32u;
  in.trace_width = 64u;
  in.trace_height = 32u;
  in.push.params[3] = 1.f;
  in.push.params[0] = 16.f;
  in.push.light[2] = 6.f;
  in.push.light[3] = 2.f;
  in.dynamic.count = 2u;
  const rtls::RtlsDispatchResult result = rtls::Dispatch(&dev, &cl, in);
  CHECK(result.ok, "dispatch ok");
  CHECK(!result.debug_written, "debug 0: debug not written");
  CHECK(cl.dispatches == 2, "trace and upsample (%d)", cl.dispatches);
  CHECK(cl.groups_x == 8u && cl.groups_y == 4u, "groups 8 x 4 (%u x %u)", cl.groups_x, cl.groups_y);
  const rtls::RtlsDeviceData& data = rtls::GetRtlsDeviceData(&dev);
  CHECK(data.raw.width == 64u && data.raw.height == 32u, "raw texture at trace size");
  CHECK(dev.res[data.raw.texture[0].handle].texture.format == format::r16g16b16a16_float, "raw is rgba16f");
  CHECK(data.debug.texture[0].handle == 0u, "no debug texture at debug 0");
  CHECK(cl.snaps.size() == 2u, "two slot snapshots (%zu)", cl.snaps.size());
  const RecCmdList::Snap& s0 = cl.snaps[0];
  CHECK(s0.srv[17].handle == 0x9200u, "t17 is the MRT normal");
  CHECK(s0.srv[19].handle == 0x9400u, "t19 is the P2b records view");
  CHECK(s0.srv[20].handle == 0x7000u, "t20 is the lamp buffer");
  CHECK(s0.srv[21].handle == 0x7100u, "t21 is the tile lists");
  CHECK(s0.uav[2].handle == data.raw.uav[0].handle && data.raw.uav[0].handle != 0u, "u2 is the raw UAV");
  CHECK(s0.uav[0].handle == 0u, "u0 null at debug 0");
  CHECK(cl.null_srv_counts.size() == 2u && cl.null_srv_counts[0] == rtls::kRtlsSrvCount
            && cl.null_srv_counts[1] == falcom_world::rtgi::kRtgiUpsampleSrvCount,
        "trace then upsample SRVs nulled (%zu pushes)", cl.null_srv_counts.size());
  CHECK(cl.null_uav_counts.size() == 2u && cl.null_uav_counts[0] == rtls::kRtlsUavCount
            && cl.null_uav_counts[1] == falcom_world::rtgi::kRtgiUpsampleUavCount,
        "trace then upsample UAVs nulled (%zu pushes)", cl.null_uav_counts.size());
  int left_bound = 0;
  for (uint32_t i = 0; i < rtls::kRtlsSrvCount; ++i) left_bound += cl.cs_srv[i].handle != 0u ? 1 : 0;
  for (uint32_t i = 0; i < rtls::kRtlsUavCount; ++i) left_bound += cl.cs_uav[i].handle != 0u ? 1 : 0;
  CHECK(left_bound == 0, "no RTLS slot left bound after the dispatch (%d)", left_bound);
  CHECK(cl.push.size() == rtls::kRtlsPushConstantCount, "push captured (%zu floats)", cl.push.size());
  uint32_t p2b_records = 0u;
  if (cl.push.size() == rtls::kRtlsPushConstantCount) std::memcpy(&p2b_records, &cl.push[11], sizeof(uint32_t));
  CHECK(p2b_records == 3u, "fade[3] bits are the P2b record count (%u)", p2b_records);
  CHECK(cl.push.size() == rtls::kRtlsPushConstantCount && cl.push[24] == 2.f, "extra[0] is the character object count");
  CHECK(dev.texture_creates == 3, "raw and the two denoise outputs at debug 0 (%d)", dev.texture_creates);
  in.debug_mode = 2;
  const rtls::RtlsDispatchResult debug_result = rtls::Dispatch(&dev, &cl, in);
  CHECK(debug_result.ok && debug_result.debug_written, "debug 2 writes the debug texture");
  CHECK(dev.res[rtls::GetRtlsDeviceData(&dev).debug.texture[0].handle].texture.format == format::r8g8b8a8_unorm,
        "debug texture is rgba8");
  CHECK(cl.snaps.size() == 4u && cl.snaps[2].uav[0].handle == rtls::GetRtlsDeviceData(&dev).debug.uav[0].handle
            && cl.snaps[2].uav[0].handle != 0u,
        "u0 is the debug UAV at debug 2");
  CHECK(dev.texture_creates == 4, "debug 2 adds one texture, raw reused (%d)", dev.texture_creates);
  CHECK(dev.res[rtls::GetRtlsDeviceData(&dev).denoise.output.texture[0].handle].texture.format == format::r8g8b8a8_unorm,
        "denoise output is rgba8");
  rtls::DestroyRtlsDeviceData(&dev);
  CHECK(dev.res.empty(), "destroy releases every RTLS object (%zu left)", dev.res.size());
  CHECK(rtls::g_rtls_devices.find(&dev) == rtls::g_rtls_devices.end(), "device entry removed");
}

// REAL: rtls::Dispatch with the shared denoise chain on the recording mock (Half resolution, temporal and spatial).
static void TestDenoiseDispatch() {
  RecDevice dev;
  RecCmdList cl;
  cl.dev = &dev;
  falcom_world::bvh::BvhDeviceData* bvh_data = falcom_world::bvh::GetBvhDeviceData(&dev);
  bvh_data->dynamic.instances_srv = {0x9400u};
  bvh_data->dynamic.instance_count = 0u;
  rtls::RtlsDispatchInputs in = {};
  in.depth_view = {0x9000u};
  in.mrt_normal_view = {0x9200u};
  in.scene_cbv_view = {0x9100u};
  in.lights_view = {0x7000u};
  in.light_indices_view = {0x7100u};
  in.full_width = 64u;
  in.full_height = 32u;
  in.trace_width = 32u;
  in.trace_height = 16u;
  in.push.params[3] = 2.f;
  in.motion_view = {0x9800u};
  in.temporal = true;
  in.temporal_push.size[3] = 1.f;
  in.spatial = true;
  in.filter_type = 1;
  in.filter_radius = 2;
  in.filter_passes = 1;
  in.filter_taps = 5;
  const rtls::RtlsDispatchResult first = rtls::Dispatch(&dev, &cl, in);
  const rtls::RtlsDeviceData& data = rtls::GetRtlsDeviceData(&dev);
  CHECK(first.ok && first.temporal_ran && first.filter_ran, "Half: temporal and filter ran");
  CHECK(cl.dispatches == 4, "trace, temporal, filter, upsample (%d)", cl.dispatches);
  CHECK(cl.push.size() == rtls::kRtlsPushConstantCount && cl.push[16] == 32.f && cl.push[17] == 16.f && cl.push[18] == 64.f
            && cl.push[19] == 32.f && cl.push[3] == 2.f,
        "trace push: trace 32x16, full 64x32, divisor 2");
  CHECK(data.raw.width == 32u && data.raw.height == 16u, "raw at trace size");
  CHECK(data.denoise.history_color.width == 32u && data.denoise.history_color.height == 16u, "history at trace size");
  CHECK(data.denoise.output.width == 64u && data.denoise.output.height == 32u, "output at full size");
  CHECK(data.denoise.history_index == 1u && data.denoise.output_index == 1u, "first run writes history 1 and output 1");
  CHECK(cl.snaps.size() == 4u && cl.snaps[1].srv[0].handle == data.raw.srv[0].handle && cl.snaps[1].srv[3].handle == 0x9800u,
        "temporal reads the raw term and the motion view");
  CHECK(cl.snaps[3].srv[2].handle == data.denoise.filter.srv[2].handle, "upsample reads the filter output B");
  CHECK(cl.snaps[3].uav[0].handle == data.denoise.output.uav[1].handle, "upsample writes output 1");
  const rtls::RtlsDispatchResult second = rtls::Dispatch(&dev, &cl, in);
  CHECK(second.ok && data.denoise.history_index == 0u && data.denoise.output_index == 0u, "second run swaps back to 0");
  in.motion_view = {0u};
  const rtls::RtlsDispatchResult no_motion = rtls::Dispatch(&dev, &cl, in);
  CHECK(no_motion.ok && !no_motion.temporal_ran && cl.dispatches == 11, "no motion: trace, filter, upsample (%d)", cl.dispatches);
  in.motion_view = {0x9800u};
  in.debug_mode = 6;
  const size_t debug_at = cl.snaps.size();
  const rtls::RtlsDispatchResult debug6 = rtls::Dispatch(&dev, &cl, in);
  CHECK(debug6.ok && debug6.debug_written, "Debug 6 written by the temporal pass");
  CHECK(cl.snaps.size() == debug_at + 4u && cl.snaps[debug_at].uav[0].handle == 0u
            && cl.snaps[debug_at + 1u].uav[3].handle == data.debug.uav[0].handle && data.debug.uav[0].handle != 0u,
        "Debug 6: the trace has no debug UAV, the temporal pass has it at u3");
  rtls::DestroyRtlsDeviceData(&dev);
  CHECK(dev.res.empty(), "destroy releases every RTLS object (%zu left)", dev.res.size());
}

// Fields that must NOT be in rtls::ParameterSnapshot: they act after the history (lighting shader or spatial filter).
template <typename T> concept SnapshotHasStrength = requires(T t) { t.strength; };
template <typename T> concept SnapshotHasMaxDarkening = requires(T t) { t.max_darkening; };
template <typename T> concept SnapshotHasSpatial = requires(T t) { t.spatial_enabled; } || requires(T t) { t.filter_type; }
    || requires(T t) { t.filter_radius; } || requires(T t) { t.filter_passes; } || requires(T t) { t.filter_taps_separable; }
    || requires(T t) { t.filter_taps_atrous; };

static void TestParameterSnapshot() {
  rtls::ParameterSnapshot a = {};
  rtls::ParameterSnapshot b = {};
  CHECK(a == b, "equal snapshots compare equal");
  b.history_weight = 0.5f;
  CHECK(!(a == b), "a history weight change is a parameter change");
  b = a;
  b.trace_resolution = 2.f;
  CHECK(!(a == b), "a trace resolution change is a parameter change");
  CHECK(!SnapshotHasStrength<rtls::ParameterSnapshot>, "Strength is not in the snapshot (no history reset)");
  CHECK(!SnapshotHasMaxDarkening<rtls::ParameterSnapshot>, "Max Darkening is not in the snapshot (no history reset)");
  CHECK(!SnapshotHasSpatial<rtls::ParameterSnapshot>, "no spatial filter field is in the snapshot");
  // Exactly the 21 float fields of rtls_state.hpp; each one takes part in the comparison.
  CHECK(sizeof(rtls::ParameterSnapshot) == 21u * sizeof(float), "snapshot is 21 floats (%zu bytes)", sizeof(rtls::ParameterSnapshot));
  for (size_t i = 0u; i < 21u; ++i) {
    rtls::ParameterSnapshot changed = {};
    float fields[21] = {};
    std::memcpy(fields, &changed, sizeof(fields));
    fields[i] = 1.f;
    std::memcpy(&changed, fields, sizeof(fields));
    CHECK(!(changed == rtls::ParameterSnapshot{}), "field %zu takes part in the comparison", i);
  }
}

int main() {
  HStage("push layout");
  TestPushLayout();
  HStage("shader defines");
  TestShaderDefines();
  HStage("shared lamp helpers");
  TestSharedLamps();
  HStage("cell transcription");
  TestCellTranscription();
  HStage("top-k transcription");
  TestTopK();
  HStage("spot cap transcription");
  TestSpotCap();
  HStage("ratio transcription");
  TestRatio();
  HStage("lighting shader apply");
  TestLightingApply();
  HStage("lighting math transcription");
  TestLightingMath();
  HStage("parameter snapshot");
  TestParameterSnapshot();
  HStage("json");
  TestJson();
  HStage("device lifecycle");
  TestDeviceLifecycle();
  HStage("denoise dispatch (mock)");
  TestDenoiseDispatch();
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
