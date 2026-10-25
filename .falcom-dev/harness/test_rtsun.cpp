// Native harness: RT sun cascades (R2, world/rtsun). The C++ boundary, weight and effective-count rules that mirror
// include/rt_sun_range.hlsli and the draw hooks' per-slice count. Pure functions, no device.
//
// REAL: falcom_world::rtsun::RtSunBoundary, RtSunWeight and RtSunEffectiveCascades (compiled from the repo header).
// NOT COVERED: the HLSL functions on the GPU (sora2nd shadow resolve), the census skip and DSV clear (draw_census.hpp),
//       the panel rows (world_bvh.hpp), the camera capture of cb_scene c50.w and c57.w.
// R3a (world/rtsun trace): TEXT: the world_rtsun.cs_5_0.hlsl defines (character casters in, shadow-only casters in: no
//       SKIP_HIDDEN), the stat indices and the range include against the kRtSun* constants (harness copy).
//       REAL: the kRtSun* constants and the RtSunPushConstants layout (rtsun.hpp).
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>

#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "src/games/falcomengine-plus/world/rtsun/rtsun.hpp"
#include "src/games/falcomengine-plus/world/rtsun/rtsun_range.hpp"

namespace rtsun = falcom_world::rtsun;
namespace rtao = falcom_world::rtao;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-3f; }

// count 4: splits 10/30/80 m, fade near 120 m, fade range inverse 0.05 (last boundary 140 m).
// count 3: splits 10/30 m (80 m is unused), fade near 60 m, fade range inverse 0.05 (last boundary 80 m).
static falcom_world::CameraSnapshot SunCamera(int count, float fade_near) {
  falcom_world::CameraSnapshot camera;
  camera.valid = true;
  camera.has_sun = true;
  camera.shadow_cascade_count = count;
  camera.shadow_split_distance[0] = 10.f;
  camera.shadow_split_distance[1] = 30.f;
  camera.shadow_split_distance[2] = 80.f;
  camera.shadow_fade_near = fade_near;
  camera.shadow_fade_range_inv = 0.05f;
  return camera;
}

static void TestBoundary() {
  const float splits[3] = {10.f, 30.f, 80.f};
  CHECK(Near(rtsun::RtSunBoundary(1, 4, splits, 120.f, 0.05f), 10.f), "count 4 cascade 1 ends at 10 m");
  CHECK(Near(rtsun::RtSunBoundary(2, 4, splits, 120.f, 0.05f), 30.f), "count 4 cascade 2 ends at 30 m");
  CHECK(Near(rtsun::RtSunBoundary(3, 4, splits, 120.f, 0.05f), 80.f), "count 4 cascade 3 ends at 80 m");
  CHECK(Near(rtsun::RtSunBoundary(4, 4, splits, 120.f, 0.05f), 140.f), "count 4 last cascade ends at fadeNear + 1/range = 140 m");
  CHECK(rtsun::RtSunBoundary(0, 4, splits, 120.f, 0.05f) == 0.f, "rt 0 has no boundary");
  CHECK(rtsun::RtSunBoundary(5, 4, splits, 120.f, 0.05f) == 0.f, "rt above count has no boundary");
  CHECK(Near(rtsun::RtSunBoundary(1, 3, splits, 60.f, 0.05f), 10.f), "count 3 cascade 1 ends at 10 m");
  CHECK(Near(rtsun::RtSunBoundary(2, 3, splits, 60.f, 0.05f), 30.f), "count 3 cascade 2 ends at 30 m");
  CHECK(Near(rtsun::RtSunBoundary(3, 3, splits, 60.f, 0.05f), 80.f), "count 3 last cascade ends at fadeNear + 1/range = 80 m");
  CHECK(rtsun::RtSunBoundary(4, 3, splits, 60.f, 0.05f) == 0.f, "count 3 has no rt 4");
  CHECK(rtsun::RtSunBoundary(1, 2, splits, 60.f, 0.05f) == 0.f, "count 2 is not a supported cascade count");
  CHECK(rtsun::RtSunBoundary(1, 0, splits, 60.f, 0.05f) == 0.f, "count 0 has no boundary");
  CHECK(rtsun::RtSunBoundary(4, 4, splits, 120.f, 0.f) > 1e5f, "zero fade range inverse is clamped (no division by zero)");
}

static void TestWeight() {
  const float splits[3] = {10.f, 30.f, 80.f};
  CHECK(Near(rtsun::RtSunWeight(0.f, 1, 4, splits, 120.f, 0.05f), 1.f), "inside cascade 1 the weight is 1");
  CHECK(Near(rtsun::RtSunWeight(8.f, 1, 4, splits, 120.f, 0.05f), 0.4f), "2 m inside the 10 m edge is 0.4 (0.2 inverse band)");
  CHECK(Near(rtsun::RtSunWeight(10.f, 1, 4, splits, 120.f, 0.05f), 0.f), "at the 10 m edge the weight is 0");
  CHECK(Near(rtsun::RtSunWeight(12.f, 1, 4, splits, 120.f, 0.05f), 0.f), "beyond the 10 m edge the weight is 0");
  CHECK(Near(rtsun::RtSunWeight(120.f, 4, 4, splits, 120.f, 0.05f), 1.f), "last cascade band starts at fadeNear");
  CHECK(Near(rtsun::RtSunWeight(130.f, 4, 4, splits, 120.f, 0.05f), 0.5f), "last cascade band at the midpoint is 0.5");
  CHECK(Near(rtsun::RtSunWeight(140.f, 4, 4, splits, 120.f, 0.05f), 0.f), "last cascade band ends at 140 m");
  CHECK(Near(rtsun::RtSunWeight(70.f, 4, 4, splits, 120.f, 0.05f), 1.f), "well inside the last cascade the weight is 1");
  CHECK(Near(rtsun::RtSunWeight(60.f, 3, 3, splits, 60.f, 0.05f), 1.f), "count 3 last cascade band starts at fadeNear");
  CHECK(Near(rtsun::RtSunWeight(78.f, 3, 3, splits, 60.f, 0.05f), 0.1f), "count 3 last cascade 2 m before the edge is 0.1");
  CHECK(Near(rtsun::RtSunWeight(0.f, 0, 4, splits, 120.f, 0.05f), 0.f), "rt 0 has weight 0");
  CHECK(Near(rtsun::RtSunWeight(0.f, 1, 2, splits, 120.f, 0.05f), 0.f), "count 2 has weight 0");
}

static void TestEffective() {
  falcom_world::CameraSnapshot none;
  CHECK(rtsun::RtSunEffectiveCascades(4, none, 0.f) == 0, "no camera snapshot gives 0");
  falcom_world::CameraSnapshot invalid = SunCamera(4, 120.f);
  invalid.valid = false;
  CHECK(rtsun::RtSunEffectiveCascades(4, invalid, 0.f) == 0, "invalid snapshot gives 0");
  falcom_world::CameraSnapshot no_sun = SunCamera(4, 120.f);
  no_sun.has_sun = false;
  CHECK(rtsun::RtSunEffectiveCascades(4, no_sun, 0.f) == 0, "snapshot without sun constants gives 0");
  CHECK(rtsun::RtSunEffectiveCascades(4, SunCamera(2, 120.f), 0.f) == 0, "camera with 2 cascades gives 0");
  CHECK(rtsun::RtSunEffectiveCascades(4, SunCamera(4, 120.f), 0.f) == 4, "count 4 requested 4 unlimited gives 4");
  CHECK(rtsun::RtSunEffectiveCascades(2, SunCamera(4, 120.f), 0.f) == 2, "requested 2 gives 2");
  CHECK(rtsun::RtSunEffectiveCascades(9, SunCamera(4, 120.f), 0.f) == 4, "requested 9 is capped at the camera count 4");
  CHECK(rtsun::RtSunEffectiveCascades(-1, SunCamera(4, 120.f), 0.f) == 0, "negative request gives 0");
  CHECK(rtsun::RtSunEffectiveCascades(4, SunCamera(3, 60.f), 0.f) == 3, "count 3 camera caps the request at 3");
  CHECK(rtsun::RtSunEffectiveCascades(4, SunCamera(4, 120.f), 100.f) == 3, "built range 100 m steps down past the 140 m boundary to 3");
  CHECK(rtsun::RtSunEffectiveCascades(4, SunCamera(4, 120.f), 50.f) == 2, "built range 50 m steps down to 2 (30 m boundary)");
  CHECK(rtsun::RtSunEffectiveCascades(4, SunCamera(4, 120.f), 5.f) == 0, "built range 5 m leaves no cascade");
  CHECK(rtsun::RtSunEffectiveCascades(4, SunCamera(4, 120.f), -1.f) == 4, "built range 0 or less is unlimited");
  CHECK(rtsun::RtSunEffectiveCascades(4, SunCamera(3, 60.f), 70.f) == 2, "count 3 built range 70 m steps down from 3 to 2");
}

static std::string ReadText(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static size_t Count(const std::string& text, const std::string& needle) {
  size_t n = 0;
  for (size_t p = text.find(needle); p != std::string::npos; p = text.find(needle, p + needle.size())) ++n;
  return n;
}

// R3a: the trace shader defines (character casters in, shadow-only casters in) and the C++ constants of the sun trace.
static void TestRtSunTrace() {
  CHECK(rtsun::kRtSunSrvCount == 20u && rtsun::kRtSunUavCount == 3u && rtsun::kRtSunPushConstantCount == 22u, "srv 20, uav 3, push 22 floats");
  CHECK(sizeof(rtsun::RtSunPushConstants) == 22u * sizeof(float), "b12 is 22 floats (%zu bytes)", sizeof(rtsun::RtSunPushConstants));
  CHECK(offsetof(rtsun::RtSunPushConstants, character) == 20u * sizeof(float), "b12 c5 offset (R4 receive flag and character bias)");
  CHECK(offsetof(rtsun::RtSunPushConstants, sun) == 8u * sizeof(float), "b12 c2 offset");
  CHECK(offsetof(rtsun::RtSunPushConstants, region) == 12u * sizeof(float), "b12 c3 offset");
  CHECK(offsetof(rtsun::RtSunPushConstants, size) == 16u * sizeof(float), "b12 c4 offset");
  CHECK(rtsun::kRtSunStatRaysBlocked == 9u && rtsun::kRtSunStatCharacterRays == 10u && rtsun::kRtSunStatSelfBlocked == 11u, "stats 9, 10, 11");
  CHECK(rtsun::kRtSunStatCharactersTraced == 14u && rtsun::kRtSunStatCharactersSelfBlocked == 15u && rtsun::kRtSunStatCharactersSelfBlocked < rtsun::kRtSunStatsCount, "R4 character stats 14, 15 inside the 32-uint buffer");
  CHECK(rtsun::kRtSunStatsCount == 32u, "stats buffer 32 uints");
  const std::string text = ReadText("src/games/falcomengine-plus/world/shaders/world_rtsun.cs_5_0.hlsl");
  CHECK(!text.empty(), "shader copy readable (run refresh_world.ps1 first)");
  CHECK(Count(text, "#define WORLD_TRACE_DYNAMIC_INSTANCES\n") == 1, "character casters: WORLD_TRACE_DYNAMIC_INSTANCES defined once");
  CHECK(Count(text, "WORLD_TRACE_DYNAMIC_SKIP_HIDDEN") == 0, "shadow-only casters included: no WORLD_TRACE_DYNAMIC_SKIP_HIDDEN");
  CHECK(Count(text, "#define WORLD_TRACE_DYNAMIC_SLOT t19") == 1, "P2b records at t19");
  CHECK(Count(text, "#include \"../../include/rt_sun_range.hlsli\"") == 1, "range header included once");
  CHECK(Count(text, "#define RTSUN_STAT_RAYS_BLOCKED 9u") == 1 && Count(text, "#define RTSUN_STAT_CHARACTER_RAYS 10u") == 1
            && Count(text, "#define RTSUN_STAT_SELF_BLOCKED 11u") == 1, "stat defines match the C++ indices");
  CHECK(Count(text, "#define RTSUN_STAT_COUNT 32u") == 1, "stat count define 32");
  CHECK(Count(text, "g_trace_dynamic_count = (uint)g_rtsun_sun.y;") == 1, "character object count from c2.y");
  CHECK(Count(text, "raw = float4(shadow * w, 0.0, w, 1.0);") == 1, "raw output (shadow * w, 0, w, 1)");
  CHECK(Count(text, "float2 g_rtsun_character : packoffset(c5);") == 1, "R4: cb_rtsun c5 holds the receive flag and character bias");
  CHECK(Count(text, "else if ((mrt0.w & 8u) == 0u && g_rtsun_character.x < 0.5)") == 1, "R4: character pixels skipped only while Receive on characters is off");
  CHECK(Count(text, "const bool receiver_character = (mrt0.w & 8u) == 0u;") == 1, "R4: character identification is MRT0.w bit 3 clear (as the lighting shader)");
  CHECK(Count(text, "#define RTSUN_STAT_CHARACTERS_TRACED 14u") == 1 && Count(text, "#define RTSUN_STAT_CHARACTERS_SELF_BLOCKED 15u") == 1, "R4: stats 14 and 15 defined");
}

// R3c: the sun denoise. Fields that must NOT be in rtsun::ParameterSnapshot: they act after the history (lighting shader
// or spatial filter).
template <typename T> concept SunSnapshotHasStrength = requires(T t) { t.strength; };
template <typename T> concept SunSnapshotHasMaxDarkening = requires(T t) { t.max_darkening; };
template <typename T> concept SunSnapshotHasSpatial = requires(T t) { t.spatial_enabled; } || requires(T t) { t.filter_type; }
    || requires(T t) { t.filter_radius; } || requires(T t) { t.filter_passes; } || requires(T t) { t.filter_taps_separable; }
    || requires(T t) { t.filter_taps_atrous; } || requires(T t) { t.filter_value_stop; };

static void TestSunParameterSnapshot() {
  rtsun::ParameterSnapshot a = {};
  rtsun::ParameterSnapshot b = {};
  CHECK(a == b, "equal snapshots compare equal");
  b.trace_resolution = 2.f;
  CHECK(!(a == b), "a trace resolution change is a parameter change");
  b = a;
  b.history_weight = 0.5f;
  CHECK(!(a == b), "a history weight change is a parameter change");
  CHECK(!SunSnapshotHasStrength<rtsun::ParameterSnapshot>, "Strength is not in the snapshot (no history reset)");
  CHECK(!SunSnapshotHasMaxDarkening<rtsun::ParameterSnapshot>, "Max Darkening is not in the snapshot (no history reset)");
  CHECK(!SunSnapshotHasSpatial<rtsun::ParameterSnapshot>, "no spatial filter field or edge stop is in the snapshot");
  // Exactly the 14 float fields listed in rtsun_state.hpp; each one takes part in the comparison.
  CHECK(sizeof(rtsun::ParameterSnapshot) == 16u * sizeof(float), "snapshot is 16 floats (%zu bytes)", sizeof(rtsun::ParameterSnapshot));
  for (size_t i = 0u; i < 16u; ++i) {
    rtsun::ParameterSnapshot changed = {};
    float fields[16] = {};
    std::memcpy(fields, &changed, sizeof(fields));
    fields[i] = 1.f;
    std::memcpy(&changed, fields, sizeof(fields));
    CHECK(!(changed == rtsun::ParameterSnapshot{}), "field %zu takes part in the comparison", i);
  }
}

// R3c: the trace resolution (divisor 1, 2, 4), the trace extent arithmetic shared by the shader and the C++ and the debug
// view numbering (1, 2 trace; 4 to 6 denoise; 3 unused).
static void TestSunDenoiseText() {
  CHECK(rtao::RtgiTraceDivisor(0.f) == 1u && rtao::RtgiTraceDivisor(1.f) == 2u && rtao::RtgiTraceDivisor(2.f) == 4u, "divisors 1, 2, 4");
  for (uint32_t d : {1u, 2u, 4u}) {
    for (uint32_t full : {1u, 7u, 1280u, 1921u}) {
      const uint32_t shader_extent = (full + d - 1u) / d;
      CHECK(rtao::RtgiTraceExtent(full, d) == shader_extent, "trace extent %u / %u matches the shader arithmetic", full, d);
    }
  }
  const std::string shader = ReadText("src/games/falcomengine-plus/world/shaders/world_rtsun.cs_5_0.hlsl");
  CHECK(Count(shader, "const uint2 trace = (full + d - 1u) / d;") == 1, "shader trace extent uses (full + d - 1) / d");
  CHECK(Count(shader, "const uint d = max((uint)g_rtsun_sun.w, 1u);") == 1, "divisor from sun.w, at least 1");
  CHECK(Count(shader, "g_rtsun_raw[tp] = raw;") == 1, "raw written at the trace pixel");
  CHECK(Count(shader, "if (debug_mode == 1 || debug_mode == 2)") == 1, "debug views 1 and 2 are written per block");
  CHECK(Count(shader, "debug_mode == 3") == 0, "debug view 3 is unused in the shader");
  const std::string rtao_state = ReadText("src/games/falcomengine-plus/world/rtao/rtao_state.hpp");
  CHECK(Count(rtao_state, "return (full + divisor - 1u) / divisor;") == 1, "RtgiTraceExtent arithmetic");
  const std::string sun = ReadText("src/games/falcomengine-plus/world/rtsun/rtsun.hpp");
  CHECK(Count(sun, "in.debug_mode >= 4 && in.debug_mode <= 6 ? in.debug_mode : 0") == 1, "denoise views 4 to 6 map to the chain");
  CHECK(Count(sun, "debug_mode == 3") == 0, "debug view 3 is unused in the C++");
  CHECK(Count(sun, "rtls::kRtlsFilterShader") == 1, "the spatial filter source is the RTLS filter (edge stop)");
  const std::string addon = ReadText("../../src/games/falcomengine-plus/addon.cpp");  // the harness copy has no addon.cpp
  // Scoped to RunRtSunInline (RTLS and RTGI have the same expressions).
  const size_t sun_begin = addon.find("static void RunRtSunInline(");
  const size_t sun_end = addon.find("static void RunRtgiInline(", sun_begin);
  const std::string sun_block = sun_begin != std::string::npos && sun_end != std::string::npos ? addon.substr(sun_begin, sun_end - sun_begin) : "";
  CHECK(!sun_block.empty(), "RunRtSunInline found in addon.cpp");
  CHECK(Count(sun_block, "rtao::RtgiTraceExtent(dd->captured_depth_w, divisor)") == 1, "addon trace width from the divisor");
  CHECK(Count(sun_block, "data.denoise.output.srv[data.denoise.output_index]") == 1, "lighting binds the denoised output");
  CHECK(Count(sun_block, "shader_injection.rtsun_spare = rtsun::g_rtsun_character_intensity;") == 1, "R4: Character Intensity goes to the rtsun_spare slot");
  // The snapshot block of RunRtSunInline must not read Strength or Max Darkening.
  const size_t begin = sun_block.find("const rtsun::ParameterSnapshot params = {");
  const size_t end = sun_block.find("const bool params_changed", begin);
  CHECK(begin != std::string::npos && end != std::string::npos, "RT sun snapshot block found in addon.cpp");
  if (begin != std::string::npos && end != std::string::npos) {
    const std::string block = sun_block.substr(begin, end - begin);
    CHECK(Count(block, "g_rtsun_strength") == 0 && Count(block, "g_rtsun_max_darkening") == 0, "snapshot block excludes Strength and Max Darkening");
    CHECK(Count(block, "g_rtsun_character_intensity") == 0, "R4: snapshot block excludes Character Intensity");
    CHECK(Count(block, "g_rtsun_receive_characters, rtsun::g_rtsun_character_normal_bias") == 1 || Count(block, "rtsun::g_rtsun_receive_characters, rtsun::g_rtsun_character_normal_bias") == 1, "R4: snapshot block has receive and character bias");
  }
  const std::string settings = ReadText("src/games/falcomengine-plus/world/world_settings.hpp");
  CHECK(Count(settings, "\"RtSunReceiveCharacters\"") == 1 && Count(settings, "\"RtSunCharacterNormalBias\"") == 1 && Count(settings, "\"RtSunCharacterIntensity\"") == 1, "R4 rows: receive, character bias, character intensity");
  CHECK(Count(settings, "\"RtSunFilterValueStop\"") == 1 && Count(settings, "\"RtSunTraceResolution\"") == 1, "RT sun rows: edge stop and trace resolution");
  // R5: Performance rows (RTLS pattern) and the GPU column sum by role.
  const std::string perf = ReadText("src/games/falcomengine-plus/world/bvh/perf_run.hpp");
  CHECK(Count(perf, "{\"gpu_rtsun\", true, kPerfGpu, true, nullptr}") == 1 && Count(perf, "{\"gpu_rtsun_denoise\", true, kPerfGpu, true, nullptr}") == 1,
        "R5: gpu_rtsun and gpu_rtsun_denoise rows are GPU rows");
  CHECK(Count(perf, "{\"rtsun_record_cpu\", true, kPerfDrawTime, false, nullptr}") == 1, "R5: rtsun_record_cpu row");
  CHECK(Count(perf, "c.cost[kPerfGpuRtsun] = extra.rtsun.ms_sum;") == 1 && Count(perf, "c.cost[kPerfGpuRtsunDenoise] = extra.rtsun_denoise.ms_sum;") == 1,
        "R5: both GPU timers feed the sample");
  CHECK(Count(perf, "} else if (role == kPerfGpu && !std::isnan(values[i])) {") == 1, "R5: GPU column sums every kPerfGpu row (rtsun included)");
}

// R4: the lighting character branch multiplies the RT sun term into the toon lerp factor, gated by rtsun_flags, before the lerp.
static void TestCharacterReceiveText() {
  const std::string lit = ReadText("../../src/games/falcomengine-plus/sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl");
  CHECK(!lit.empty(), "lighting shader readable");
  const size_t flag = lit.find("const float rtsunCharTerm = rtsunTexture.SampleLevel(samPoint_s, v1.xy, 0).x;");
  const size_t lerp = lit.find("      r5.yzw = -r9.xyz * r8.xyz + r0.xyz;");
  CHECK(flag != std::string::npos && lerp != std::string::npos && flag < lerp, "R4 character RT term sits before the toon lerp (r5.yzw)");
  CHECK(Count(lit, "#define RTSUN_FLAG_APPLIED 1u") == 1 && Count(lit, "#define RTSUN_FLAG_RECEIVE 2u") == 1, "R4: shader RTSUN_FLAG defines");
  CHECK(rtsun::kRtSunFlagApplied == 1u && rtsun::kRtSunFlagReceive == 2u, "R4: C++ kRtSunFlag bit values");
  CHECK(Count(lit, "#define RTSUN_FLAG_APPLIED " + std::to_string(rtsun::kRtSunFlagApplied) + "u") == 1
        && Count(lit, "#define RTSUN_FLAG_RECEIVE " + std::to_string(rtsun::kRtSunFlagReceive) + "u") == 1, "R4: C++ bits equal the shader defines");
  CHECK(Count(lit, "if (((uint)shader_injection_data.rtsun_flags & RTSUN_FLAG_APPLIED) != 0u) {") == 1, "R4: environment path gated by the applied bit only");
  CHECK(Count(lit, "if (((uint)shader_injection_data.rtsun_flags & (RTSUN_FLAG_APPLIED | RTSUN_FLAG_RECEIVE)) == (RTSUN_FLAG_APPLIED | RTSUN_FLAG_RECEIVE)) {") == 1,
        "R4: character path runs only with the applied and receive bits");
  CHECK(Count(lit, "const float rtsunCharVisibility = 1.0 - min(rtsunCharTerm * shader_injection_data.rtsun_strength, shader_injection_data.rtsun_max_darkening);") == 1,
        "R4: character visibility = 1 - min(term x Strength, Max Darkening)");
  CHECK(Count(lit, "r3.z *= lerp(1.0, rtsunCharVisibility, saturate(shader_injection_data.rtsun_spare));") == 1, "R4: Character Intensity lerps from CSM (0) to RT (1)");
  CHECK(Count(lit, "rtsunTexture.SampleLevel(samPoint_s, v1.xy, 0)") == 2, "R4: both RT sun samples use samPoint at v1.xy");
  // Transcription of the character visibility (lighting shader) and the character lerp.
  const float term = 0.8f, strength = 1.f, max_dark = 0.5f;
  const float vis = 1.f - std::fmin(term * strength, max_dark);
  const auto char_term = [&](float intensity) {
    const float i = std::fmin(std::fmax(intensity, 0.f), 1.f);
    return 1.f + (vis - 1.f) * i;
  };
  CHECK(std::fmin(term * strength, max_dark) < term, "R4 numeric: Max Darkening 0.5 cap is active at term 0.8");
  CHECK(std::fabs(vis - 0.5f) < 1e-6f, "R4 numeric: visibility at term 0.8, Max Darkening 0.5 is 0.5 (got %f)", vis);
  CHECK(std::fabs(char_term(0.f) - 1.f) < 1e-6f, "R4 numeric: Character Intensity 0 gives 1 (CSM only)");
  CHECK(std::fabs(char_term(1.f) - vis) < 1e-6f, "R4 numeric: Character Intensity 1 gives the RT visibility");
  CHECK(std::fabs(char_term(0.5f) - (1.f + vis) / 2.f) < 1e-6f, "R4 numeric: Character Intensity 0.5 gives (1 + vis) / 2");
}

int main() {
  HStage("boundary transcription");
  TestBoundary();
  HStage("weight transcription");
  TestWeight();
  HStage("effective count");
  TestEffective();
  HStage("sun trace text and constants");
  TestRtSunTrace();
  HStage("sun denoise snapshot and text");
  TestSunParameterSnapshot();
  TestSunDenoiseText();
  TestCharacterReceiveText();
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
