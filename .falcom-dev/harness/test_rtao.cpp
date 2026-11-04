// Native harness: RTAO (world/rtao): pass A, the SVGF glue (rtao_svgf.hpp), the reset rule and the discovery index.
//
// TRANSCRIPTIONS (the test runs a copy of the shader logic, not the GPU code):
//   - gate table (16 combinations of T, Rd, G, V): the addon.cpp gate expressions written out again.
//   - ray generation, falloff, fade and strength (TestRayRules): world_rtao.cs_5_0.hlsl TracePixel on a wall
//     scene; the reference is a second sample sequence, not the BVH.
//   - pixel classes (sky, invalid normal, scaled, region): the shader's order, written out again.
//   - reset rule: the function in rtao_state.hpp is real, tested over all 512 input combinations against the
//     priority written out in the test.
//   - fade range and region margin: rtao_state.hpp / the shader rule written out again.
//
// REAL CODE RUN ON THE MOCK DEVICE (the addon's own functions):
//   - rtao::Dispatch: pass A bindings and nulls on the context; rtao_svgf.hpp: the SVGF dispatch order and bindings,
//     sizes, the output view, the release and destroy (TestDeviceLifecycle, TestSvgfDispatch).
//   - rtao::CheckMotionResources, rtao::TwoSidedDiscoveryIndex, ParameterSnapshot equality, ReasonName.
//   - contract::IsCameraViewVertex on real bytecode fixtures is covered in test_visibility, not here.
//
// NOT COVERED by this harness (addon.cpp or GPU only, checked by the addon build and in game):
//   - RunRtaoInline guard order (deferred list, startup and resize guard, once per frame, depth and normal
//     and scene CBV checks) and its reason codes.
//   - t22 is not pushed before the first successful write.
//   - the census histogram call (CommitDrawRecord) and the JSON text written by MaybeCaptureRtaoStats.
//   - the shader on the GPU (FXC compiles both passes; the output is not run).
//   - the barrier and the push nulls on a real D3D11 device (the mock models the slots only).
//
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstddef>
#include <map>
#include <span>
#include <string>
#include <random>
#include <vector>

#define __world_rtao_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao{__world_rtao_base};
#define __world_rtao_svgf_input_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_svgf_input_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao_svgf_input{__world_rtao_svgf_input_base};
#define __gtvbao_svgf_upsample_EMBED_FILE
inline constexpr std::uint8_t __gtvbao_svgf_upsample_base[] = {0};
inline constexpr std::span<const std::uint8_t> __gtvbao_svgf_upsample{__gtvbao_svgf_upsample_base};
#define __world_rtgi_EMBED_FILE
inline constexpr std::uint8_t __world_rtgi_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtgi{__world_rtgi_base};
#define __world_svgf_reproject_EMBED_FILE
inline constexpr std::uint8_t __world_svgf_reproject_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_svgf_reproject{__world_svgf_reproject_base};
#define __world_svgf_variance_EMBED_FILE
inline constexpr std::uint8_t __world_svgf_variance_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_svgf_variance{__world_svgf_variance_base};
#define __world_svgf_atrous_EMBED_FILE
inline constexpr std::uint8_t __world_svgf_atrous_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_svgf_atrous{__world_svgf_atrous_base};
#define __world_svgf_atrous_s1_EMBED_FILE
inline constexpr std::uint8_t __world_svgf_atrous_s1_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_svgf_atrous_s1{__world_svgf_atrous_s1_base};
#define __world_svgf_atrous_s2_EMBED_FILE
inline constexpr std::uint8_t __world_svgf_atrous_s2_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_svgf_atrous_s2{__world_svgf_atrous_s2_base};
#define __world_svgf_atrous_s4_EMBED_FILE
inline constexpr std::uint8_t __world_svgf_atrous_s4_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_svgf_atrous_s4{__world_svgf_atrous_s4_base};
#define __world_rtgi_lights_EMBED_FILE
inline constexpr std::uint8_t __world_rtgi_lights_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtgi_lights{__world_rtgi_lights_base};
#define __world_rtgi_upsample_EMBED_FILE
inline constexpr std::uint8_t __world_rtgi_upsample_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtgi_upsample{__world_rtgi_upsample_base};
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "src/games/falcomengine-plus/world/rtao/rtao.hpp"
#include "src/games/falcomengine-plus/world/rtao/rtao_svgf.hpp"
#include "../../src/games/falcomengine-plus/shared.h"  // REAL: compiled; ShaderInjectData.rtao_debug_show must exist


using namespace reshade::api;
namespace rtao = falcom_world::rtao;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static bool Near(float a, float b, float tol) { return std::fabs(a - b) <= tol; }

// ---- Gate (addon.cpp: RunRtaoInline, the T-gated GTVBAO/VBGI sites, the SSAO neutral condition) ----
struct GateResult {
  int rtao_active;   // shader_injection.rtao_active
  const char* ao;    // AO source at t22
  const char* gi;    // GI at t23
  const char* passes;
  bool neutral;      // SSAO shader neutral path
  bool vbgi_bound;   // shader_injection.gtvbao_vbgi_bound
};

static GateResult EvalGate(bool T, bool Rd, bool G, bool V) {
  GateResult r{};
  const bool producing = T && Rd;
  r.rtao_active = !T ? 0 : (Rd ? 2 : 1);
  const bool gtvbao_active = G && !T;
  r.vbgi_bound = gtvbao_active && V;
  r.ao = producing ? "RTAO" : (gtvbao_active ? "GTVBAO" : "vanilla");
  r.gi = r.vbgi_bound ? "VBGI" : "none";
  r.passes = producing ? "RTAO only" : (gtvbao_active ? (V ? "GTVBAO + VBGI" : "GTVBAO") : "none");
  // SSAO shader: (gtvbao_mode > 0.5 && rtao_active < 0.5) || vbgi_bound > 0.5 || rtao_active > 1.5
  r.neutral = (G && r.rtao_active < 0.5) || r.vbgi_bound || r.rtao_active > 1.5;
  return r;
}

static void TestGateTable() {
  for (int bits = 0; bits < 16; ++bits) {
    const bool T = (bits & 1) != 0, Rd = (bits & 2) != 0, G = (bits & 4) != 0, V = (bits & 8) != 0;
    const GateResult r = EvalGate(T, Rd, G, V);
    if (!T) {
      // Rd is meaningless with T=0: the plan's row says "-".
      CHECK(r.rtao_active == 0, "T0 rtao_active %d", r.rtao_active);
      CHECK(!r.vbgi_bound || (G && V), "T0 vbgi_bound");
      if (!G) {
        CHECK(std::string(r.ao) == "vanilla" && std::string(r.passes) == "none" && !r.neutral, "T0 G0 row");
      } else if (!V) {
        CHECK(std::string(r.ao) == "GTVBAO" && std::string(r.gi) == "none" && std::string(r.passes) == "GTVBAO" && r.neutral, "T0 G1 V0 row");
      } else {
        CHECK(std::string(r.ao) == "GTVBAO" && std::string(r.gi) == "VBGI" && std::string(r.passes) == "GTVBAO + VBGI" && r.neutral, "T0 G1 V1 row");
      }
    } else if (Rd) {
      CHECK(std::string(r.ao) == "RTAO" && std::string(r.gi) == "none" && std::string(r.passes) == "RTAO only", "T1 Rd1 row (G%d V%d)", G, V);
      CHECK(r.rtao_active == 2 && r.neutral && !r.vbgi_bound, "T1 Rd1 rtao_active/neutral/vbgi (G%d V%d)", G, V);
    } else {
      CHECK(std::string(r.ao) == "vanilla" && std::string(r.gi) == "none" && std::string(r.passes) == "none", "T1 Rd0 row (G%d V%d)", G, V);
      CHECK(r.rtao_active == 1 && !r.neutral && !r.vbgi_bound, "T1 Rd0 rtao_active/neutral/vbgi (G%d V%d)", G, V);
    }
  }
  std::printf("gate table: 16 combinations checked\n");
}

// ---- Fade range (rtao_state.hpp RtaoEffectiveFade) ----
struct FadeCase { float built, slider, start_pct, end_pct, start_req, end_req, start_eff, end_eff; };

// Percents of the slider range (RtaoFadeStartPct / RtaoFadeEndPct, defaults 15.625 / 31.25).
static void TestFade() {
  const FadeCase cases[] = {
      {256.f, 256.f, 15.625f, 31.25f, 40.f, 80.f, 40.f, 80.f},        // default range: today's 40 / 80 m
      {512.f, 512.f, 15.625f, 31.25f, 80.f, 160.f, 80.f, 160.f},      // default fade at the maximum range
      {256.f, 256.f, 100.f, 100.f, 256.f, 256.f, 256.f, 256.f},       // full range
      {512.f, 512.f, 100.f, 100.f, 512.f, 512.f, 512.f, 512.f},       // full range at the maximum
      {256.f, 256.f, 50.f, 100.f, 128.f, 256.f, 128.f, 256.f},
      {256.f, 512.f, 60.f, 100.f, 307.2f, 512.f, 256.f, 256.f},       // TLAS lags the slider: caps apply
      {512.f, 256.f, 100.f, 100.f, 256.f, 256.f, 256.f, 256.f},       // slider shrunk: built range is larger
      {256.f, 256.f, 90.f, 50.f, 230.4f, 128.f, 128.f, 128.f},        // start above end: start capped to end
  };
  for (const auto& c : cases) {
    const rtao::Fade f = rtao::RtaoEffectiveFade(c.built, c.slider, c.start_pct, c.end_pct);
    CHECK(Near(f.start_req, c.start_req, 1e-3f), "built %.0f slider %.0f start req %.3f want %.3f", c.built, c.slider, f.start_req, c.start_req);
    CHECK(Near(f.end_req, c.end_req, 1e-3f), "built %.0f slider %.0f end req %.3f want %.3f", c.built, c.slider, f.end_req, c.end_req);
    CHECK(Near(f.end, c.end_eff, 1e-3f), "built %.0f slider %.0f end_eff %.3f want %.3f", c.built, c.slider, f.end, c.end_eff);
    CHECK(Near(f.start, c.start_eff, 1e-3f), "built %.0f slider %.0f start_eff %.3f want %.3f", c.built, c.slider, f.start, c.start_eff);
    CHECK(Near(f.coverage, c.built, 1e-3f), "coverage is the built range");
    CHECK(f.end <= f.coverage + 1e-4f && f.start <= f.end + 1e-4f, "built %.0f ordering", c.built);
  }
  // No TLAS built yet: coverage below 1 m (the caller reports range_too_small).
  CHECK(rtao::RtaoEffectiveFade(0.f, 256.f, 15.625f, 31.25f).coverage < 1.f, "no built range is not ready");
  // Maximum slider and maximum percent: 100% of 512 m, and the 10 m ray length the slider allows is not a term here.
  CHECK(Near(rtao::RtaoEffectiveFade(512.f, 512.f, 100.f, 100.f).end, 512.f, 1e-3f), "max range end");
  std::printf("fade: %zu cases checked\n", sizeof(cases) / sizeof(cases[0]));
}

// ---- Per-pixel region margin (world_rtao.cs_5_0.hlsl TracePixel) ----
static bool InsideRegion(const float P[3], const float region_min[3], float region_size, float ray_max) {
  for (int i = 0; i < 3; ++i) {
    if (P[i] < region_min[i] + ray_max) return false;
    if (P[i] > region_min[i] + region_size - ray_max) return false;
  }
  return true;
}

static void TestRegionMargin() {
  const float lo[3] = {0.f, 0.f, 0.f};
  const float at_lo[3] = {2.f, 2.f, 2.f}, below_lo[3] = {1.9f, 2.f, 2.f};
  const float at_hi[3] = {62.f, 62.f, 62.f}, above_hi[3] = {62.1f, 2.f, 2.f};
  CHECK(InsideRegion(at_lo, lo, 64.f, 2.f), "pixel exactly ray_max from the low face is inside");
  CHECK(!InsideRegion(below_lo, lo, 64.f, 2.f), "pixel closer than ray_max to the low face is neutral");
  CHECK(InsideRegion(at_hi, lo, 64.f, 2.f), "pixel exactly ray_max from the high face is inside");
  CHECK(!InsideRegion(above_hi, lo, 64.f, 2.f), "pixel closer than ray_max to the high face is neutral");
}

// ---- b12 layout (rtao_resources.hpp, cb_rtao in world_rtao.cs_5_0.hlsl) ----
static void TestPushLayout() {
  CHECK(sizeof(rtao::RtaoPushConstants) == 24u * sizeof(float), "b12 size %zu", sizeof(rtao::RtaoPushConstants));
  CHECK(offsetof(rtao::RtaoPushConstants, params) == 0u, "c0 offset");
  CHECK(offsetof(rtao::RtaoPushConstants, sampling) == 16u, "c1 offset");
  CHECK(offsetof(rtao::RtaoPushConstants, fade) == 32u, "c2 offset");
  CHECK(offsetof(rtao::RtaoPushConstants, region) == 48u, "c3 offset");
  CHECK(offsetof(rtao::RtaoPushConstants, size) == 64u, "c4 offset");
  CHECK(rtao::kRtaoStatsCount == 24u && rtao::kRtaoStatScaled == 9u && rtao::kRtaoStatDenoiseBase == 20u, "stats count");
  CHECK(rtao::kRtaoSrvCount == 20u && rtao::kRtaoUavCount == 3u && rtao::kRtaoPushRegister == 12u, "table sizes");
}

// ---- Ray generation, falloff, fade and strength: transcription of world_rtao.cs_5_0.hlsl TracePixel ----
struct V3 { float x, y, z; };
static V3 Add(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static V3 Scale(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static float Smoothstep(float a, float b, float x) {
  const float t = std::fmin(std::fmax((x - a) / (b - a), 0.f), 1.f);
  return t * t * (3.f - 2.f * t);
}

// Cosine-hemisphere direction for N = (0,0,1): T = (0,-1,0), B = (1,0,0) as the shader builds them.
static V3 CosineDir(float u1, float u2) {
  const float r = std::sqrt(u1), phi = 6.2831853f * u2;
  return {r * std::cos(phi), r * std::sin(phi), std::sqrt(std::max(0.f, 1.f - u1))};
}

// Wall scene: the plane x = wall_x facing -x, in front of a pixel at the origin with N = (0,0,1).
// Returns the closest hit t along dir within (1e-3, ray_max], or -1 when there is none.
static float WallHit(V3 origin, V3 dir, float wall_x, float ray_max) {
  if (dir.x <= 1e-6f) return -1.f;
  const float t = (wall_x - origin.x) / dir.x;
  return (t > 1e-3f && t <= ray_max) ? t : -1.f;
}

static float ShaderAo(float wall_x, float radius, float ray_max, float strength, float fade, int spp, bool grid) {
  float occ = 0.f;
  const V3 origin = {0.f, 0.f, 0.f};
  for (int i = 0; i < spp; ++i) {
    float u1, u2;
    if (grid) {  // deterministic stratified sampling
      const int side = static_cast<int>(std::sqrt(static_cast<float>(spp)));
      u1 = (static_cast<float>(i % side) + 0.5f) / side;
      u2 = (static_cast<float>(i / side) + 0.5f) / side;
    } else {  // a different sequence (van der Corput style) for the reference
      u1 = std::fmod(0.5f + i * 0.6180339f, 1.f);
      u2 = std::fmod(0.5f + i * 0.7548776f, 1.f);
    }
    const V3 dir = CosineDir(u1, u2);
    const float t = WallHit(origin, dir, wall_x, ray_max);
    if (t >= 0.f) occ += 1.f - Smoothstep(radius, ray_max, t);
  }
  occ /= static_cast<float>(spp);
  const float raw = std::pow(std::fmax(1.f - occ, 0.f), strength);
  return (1.f - fade) * 1.f + fade * raw;
}

static void TestRayRules() {
  // Directions are unit length, in the hemisphere, with a cosine distribution (mean z = 2/3).
  double mean_z = 0.0;
  const int n = 64;
  bool unit = true, hemi = true;
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      const V3 d = CosineDir((i + 0.5f) / n, (j + 0.5f) / n);
      const float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
      unit = unit && Near(len, 1.f, 1e-4f);
      hemi = hemi && d.z >= 0.f;
      mean_z += d.z;
    }
  }
  mean_z /= n * n;
  CHECK(unit, "cosine directions are unit length");
  CHECK(hemi, "cosine directions stay in the hemisphere");
  CHECK(std::fabs(mean_z - 2.0 / 3.0) < 0.01, "cosine distribution mean z %.4f want 0.6667", mean_z);

  // Bias: origin = P + N * bias (N = +z).
  const V3 P = {1.f, 2.f, 3.f};
  const V3 origin = Add(P, Scale({0.f, 0.f, 1.f}, 0.05f));
  CHECK(Near(origin.z, 3.05f, 1e-6f) && Near(origin.x, 1.f, 1e-6f), "normal bias moves the origin along N");

  // Falloff: weight 1 inside radius, 0 at ray max, smooth in between.
  CHECK(Near(1.f - Smoothstep(1.f, 2.f, 0.5f), 1.f, 1e-6f), "weight is 1 inside the radius");
  CHECK(Near(1.f - Smoothstep(1.f, 2.f, 2.f), 0.f, 1e-6f), "weight is 0 at ray max");
  CHECK(Near(1.f - Smoothstep(1.f, 2.f, 1.5f), 0.5f, 1e-6f), "weight is 0.5 at the midpoint");

  // Wall at 0.3 m (inside radius for hits): AO < 1; reference with a different sample sequence agrees.
  const float ao_near = ShaderAo(0.3f, 1.f, 2.f, 1.f, 1.f, 256, true);
  const float ao_ref = ShaderAo(0.3f, 1.f, 2.f, 1.f, 1.f, 4096, false);
  CHECK(ao_near < 0.99f && ao_near > 0.2f, "near wall darkens: %.3f", ao_near);
  CHECK(Near(ao_near, ao_ref, 0.05f), "grid vs reference: %.3f vs %.3f", ao_near, ao_ref);
  // Wall beyond ray max: no hit, AO = 1 exactly.
  CHECK(ShaderAo(5.f, 1.f, 2.f, 1.f, 1.f, 256, true) == 1.f, "beyond ray max is neutral");
  // Strength 0: no darkening.
  CHECK(Near(ShaderAo(0.3f, 1.f, 2.f, 0.f, 1.f, 256, true), 1.f, 1e-6f), "strength 0 is neutral");
  // Fade: fully faded (0) is neutral; fade 1 is the raw AO.
  CHECK(ShaderAo(0.3f, 1.f, 2.f, 1.f, 0.f, 256, true) == 1.f, "fade 0 is neutral");
  CHECK(Near(ShaderAo(0.3f, 1.f, 2.f, 1.f, 0.5f, 256, true), 0.5f * (1.f + ao_near), 1e-5f), "fade 0.5 blends");

  // Fade factor: saturate((end_eff - d) / max(end_eff - start_eff, 1e-4)).
  const auto fade_at = [](float d, float start_eff, float end_eff) {
    return std::fmin(std::fmax((end_eff - d) / std::fmax(end_eff - start_eff, 1e-4f), 0.f), 1.f);
  };
  CHECK(fade_at(10.f, 40.f, 80.f) == 1.f, "closer than start: full AO");
  CHECK(fade_at(120.f, 40.f, 80.f) == 0.f, "beyond end: neutral");
  CHECK(Near(fade_at(60.f, 40.f, 80.f), 0.5f, 1e-6f), "midway: half");
  CHECK(fade_at(50.f, 50.f, 50.f) == 0.f, "start == end: step, neutral past the end");
}

// ---- Pixel classes: sky (depth 0), invalid MRT normal, scaled resolution, outside region ----
enum class PixelClass { Scaled, Sky, NormalInvalid, Region, Traced };
static PixelClass Classify(float depth, bool normal_zero, float scale_x, float scale_y, bool outside_region) {
  // Scaled first (the shader's order), then sky, normal, region.
  if (scale_x != 1.f || scale_y != 1.f) return PixelClass::Scaled;
  if (depth <= 0.f) return PixelClass::Sky;
  if (normal_zero) return PixelClass::NormalInvalid;
  if (outside_region) return PixelClass::Region;
  return PixelClass::Traced;
}

static void TestClasses() {
  CHECK(Classify(0.f, false, 1.f, 1.f, false) == PixelClass::Sky, "depth 0 is sky");
  CHECK(Classify(0.5f, true, 1.f, 1.f, false) == PixelClass::NormalInvalid, "zero normal is invalid");
  CHECK(Classify(0.5f, false, 0.75f, 0.75f, false) == PixelClass::Scaled, "scaled resolution");
  CHECK(Classify(0.5f, false, 1.f, 1.f, true) == PixelClass::Region, "outside region");
  CHECK(Classify(0.5f, false, 1.f, 1.f, false) == PixelClass::Traced, "normal pixel is traced");
}

// ---- Mock device lifecycle (rtao::EnsureRtaoPipeline, EnsureRtaoTraceTarget, EnsureRtaoStats, Dispatch, destroy) ----
struct Res { resource_desc desc; };
struct Device : mock::DeviceBase {
  std::map<uint64_t, Res> res;
  std::map<uint64_t, uint64_t> view_of;
  std::map<uint64_t, uint64_t> uav_of_table;  // descriptor table -> the UAV in its last binding (0 when null)
  std::map<uint64_t, std::vector<resource_view>> views_of_table;  // SRV and UAV descriptors per table
  uint64_t next = 0x1000;
  int texture_creates = 0;
  bool fail_texture_2d = false;  // S3: forced creation failure for texture_2d (the filter targets)
  device_api get_api() const override { return device_api::d3d11; }
  void update_descriptor_tables(uint32_t count, const descriptor_table_update *updates) override {
    for (uint32_t i = 0; i < count; ++i) {
      if (updates[i].type == descriptor_type::unordered_access_view) {
        uav_of_table[updates[i].table.handle] = static_cast<const resource_view *>(updates[i].descriptors)[0].handle;
      }
      if (updates[i].type == descriptor_type::shader_resource_view || updates[i].type == descriptor_type::unordered_access_view) {
        const auto* views = static_cast<const resource_view *>(updates[i].descriptors);
        views_of_table[updates[i].table.handle].assign(views, views + updates[i].count);
      }
      if (updates[i].type == descriptor_type::sampler) {
        const auto* samplers = static_cast<const sampler *>(updates[i].descriptors);
        samplers_of_table[updates[i].table.handle].assign(samplers, samplers + updates[i].count);
      }
    }
  }
  bool create_sampler(const sampler_desc &, sampler *out) override { out->handle = next; next += 0x100; sampler_creates += 1; return true; }
  int sampler_creates = 0;
  std::map<uint64_t, std::vector<sampler>> samplers_of_table;
  bool create_resource(const resource_desc &desc, const subresource_data *, resource_usage, resource *out, void ** = nullptr) override {
    if (desc.type == resource_type::texture_2d && fail_texture_2d) return false;
    if (desc.type == resource_type::texture_2d) texture_creates += 1;
    Res r; r.desc = desc;
    out->handle = next; next += 0x100; res[out->handle] = r;
    return true;
  }
  void destroy_resource(resource r) override { res.erase(r.handle); }
  bool create_resource_view(resource r, resource_usage, const resource_view_desc &, resource_view *out) override {
    out->handle = next; next += 0x100; view_of[out->handle] = r.handle; return true;
  }
  void destroy_resource_view(resource_view v) override { view_of.erase(v.handle); }
  bool create_pipeline_layout(uint32_t, const pipeline_layout_param *, pipeline_layout *out) override { out->handle = next; next += 0x100; return true; }
  void destroy_pipeline_layout(pipeline_layout) override {}
  bool create_pipeline(pipeline_layout, uint32_t, const pipeline_subobject *, pipeline *out) override { out->handle = next; next += 0x100; return true; }
  void destroy_pipeline(pipeline) override {}
  bool allocate_descriptor_tables(uint32_t count, pipeline_layout, uint32_t, descriptor_table *out) override {
    for (uint32_t i = 0; i < count; ++i) out[i].handle = next++;
    return true;
  }
  void free_descriptor_tables(uint32_t, const descriptor_table *) override {}
  void update_buffer_region(const void *, resource, uint64_t, uint64_t) override {}
};

struct CmdList : mock::CommandListBase {
  Device* dev = nullptr;
  int dispatches = 0;
  uint32_t groups_x = 0u, groups_y = 0u;
  int null_srv_pushes = 0, null_uav_pushes = 0;   // push_descriptors with pipeline_layout{0} that null all views
  uint32_t null_srv_count = 0u, null_uav_count = 0u;
  bool pushed_nonnull_uav = false;
  // Model of the compute slots on the context (set by binds and pushes, not by update_descriptor_tables).
  resource_view cs_srv[32] = {};
  resource_view cs_uav[8] = {};
  std::vector<float> rtao_push;  // the first 20-float push (pass A b12): fade[3] carries the P2b record count
  int dispatch_srv_bound = 0, dispatch_uav_bound = 0;  // non-null slots at the moment of the dispatch
  bool dispatch_t9 = false, dispatch_t17 = false, dispatch_t18 = false, dispatch_t19 = false, dispatch_u0 = false, dispatch_u1 = false;
  sampler cs_sampler[4] = {};
  // The slots as they were at each dispatch (one entry per pass: A, then B when it ran).
  struct Snap { resource_view srv[32]; resource_view uav[8]; sampler smp[4]; };
  std::vector<Snap> snaps;
  device *get_device() override { return dev; }
  void bind_pipeline(pipeline_stage, pipeline) override {}
  void bind_descriptor_tables(shader_stage, pipeline_layout layout, uint32_t first, uint32_t count, const descriptor_table *tables) override {
    if (layout.handle != 0u) {
      for (uint32_t i = 0; i < count; ++i) {
        const auto& views = dev->views_of_table[tables[i].handle];
        if (first + i == 1u) for (size_t j = 0; j < views.size() && j < 32; ++j) cs_srv[j] = views[j];
        if (first + i == 2u) for (size_t j = 0; j < views.size() && j < 8; ++j) cs_uav[j] = views[j];
        if (first + i == 3u) {
          const auto& smp = dev->samplers_of_table[tables[i].handle];
          for (size_t j = 0; j < smp.size() && j < 4; ++j) cs_sampler[j] = smp[j];
        }
      }
    }
  }
  void push_constants(shader_stage, pipeline_layout, uint32_t, uint32_t, uint32_t count, const void* values) override {
    if (count == rtao::kRtaoPushConstantCount && rtao_push.empty()) {
      const float* f = static_cast<const float *>(values);
      rtao_push.assign(f, f + count);
    }
  }
  void push_descriptors(shader_stage, pipeline_layout layout, uint32_t, const descriptor_table_update &update) override {
    const auto* views = static_cast<const resource_view *>(update.descriptors);
    if (layout.handle == 0u) {
      for (uint32_t i = 0; i < update.count; ++i) {
        if (update.type == descriptor_type::texture_shader_resource_view) cs_srv[update.binding + i] = views[i];
        if (update.type == descriptor_type::texture_unordered_access_view) cs_uav[update.binding + i] = views[i];
        if (update.type == descriptor_type::sampler) cs_sampler[update.binding + i] = static_cast<const sampler *>(update.descriptors)[i];
      }
    }
    if (layout.handle != 0u) return;
    bool all_null = true;
    for (uint32_t i = 0; i < update.count; ++i) all_null = all_null && views[i].handle == 0u;
    if (update.type == descriptor_type::texture_shader_resource_view && all_null) {
      null_srv_pushes += 1;
      null_srv_count = update.count;
    }
    if (update.type == descriptor_type::texture_unordered_access_view) {
      if (all_null) { null_uav_pushes += 1; null_uav_count = update.count; }
      else pushed_nonnull_uav = true;
    }
  }
  void dispatch(uint32_t x, uint32_t y, uint32_t) override {
    dispatches += 1; groups_x = x; groups_y = y;
    dispatch_srv_bound = 0;
    for (const auto& v : cs_srv) dispatch_srv_bound += v.handle != 0u ? 1 : 0;
    dispatch_uav_bound = 0;
    for (const auto& v : cs_uav) dispatch_uav_bound += v.handle != 0u ? 1 : 0;
    dispatch_t9 = cs_srv[9].handle != 0u;
    dispatch_t17 = cs_srv[17].handle != 0u;
    dispatch_t18 = cs_srv[18].handle != 0u;
    dispatch_t19 = cs_srv[19].handle != 0u;
    dispatch_u0 = cs_uav[0].handle != 0u;
    dispatch_u1 = cs_uav[1].handle != 0u;
    Snap snap = {};
    for (size_t j = 0; j < 32; ++j) snap.srv[j] = cs_srv[j];
    for (size_t j = 0; j < 8; ++j) snap.uav[j] = cs_uav[j];
    for (size_t j = 0; j < 4; ++j) snap.smp[j] = cs_sampler[j];
    snaps.push_back(snap);
  }
};

static void TestDeviceLifecycle() {
  Device dev;
  CmdList cl;
  cl.dev = &dev;
  CHECK(rtao::g_rtao_devices.find(&dev) == rtao::g_rtao_devices.end(), "no RTAO data before any RTAO call (T=0 path)");
  CHECK(dev.res.empty() && dev.texture_creates == 0, "T=0: no device objects");

  rtao::RtaoDeviceData& data = rtao::GetRtaoDeviceData(&dev);
  CHECK(data.pipeline.handle == 0u, "pipeline not created before the first call");
  CHECK(rtao::EnsureRtaoPipeline(&dev, &data), "pipeline created lazily");
  const uint64_t pipeline = data.pipeline.handle;
  CHECK(rtao::EnsureRtaoPipeline(&dev, &data) && data.pipeline.handle == pipeline, "pipeline created once");

  CHECK(rtao::EnsureRtaoTraceTarget(&dev, &data, 64u, 32u), "trace target created");
  CHECK(dev.texture_creates == 1, "one trace texture: %d", dev.texture_creates);
  CHECK(rtao::EnsureRtaoTraceTarget(&dev, &data, 64u, 32u) && dev.texture_creates == 1, "same size: no new texture");
  CHECK(rtao::EnsureRtaoTraceTarget(&dev, &data, 128u, 32u) && dev.texture_creates == 2, "resize creates a new texture");
  int textures = 0;
  for (const auto& entry : dev.res) textures += entry.second.desc.type == resource_type::texture_2d ? 1 : 0;
  CHECK(textures == 1, "resize destroyed the old trace texture (textures alive %d)", textures);
  CHECK(data.trace_width == 128u && data.trace_height == 32u, "target size tracked");
  CHECK(rtao::EnsureRtaoStats(&dev, &data) && data.stats_buffer.handle != 0u, "stats buffer created");

  rtao::RtaoDispatchInputs in = {};
  in.width = 200u;   // the captured depth size (addon.cpp sizes RTAO from captured_depth_w/h)
  in.height = 100u;
  in.depth_view = {0x9000u};
  in.mrt_normal_view = {0x9200u};
  in.isfast_view = {0x9300u};
  in.scene_cbv_view = {0x9100u};
  // P2b: the records view (t19) and the record count (fade[3] bits, the same way the trace sets its count).
  falcom_world::bvh::BvhDeviceData* bvh_data = falcom_world::bvh::GetBvhDeviceData(&dev);
  bvh_data->dynamic.instances_srv = {0x9400u};
  bvh_data->dynamic.instance_count = 3u;
  const bool dispatched = rtao::Dispatch(&dev, &cl, in).ok;
  CHECK(dispatched && cl.dispatches == 1, "dispatch issued (ok %d, dispatches %d)", dispatched, cl.dispatches);
  CHECK(data.trace_width == 200u && data.trace_height == 100u, "trace target sized from the depth input at Full (%u x %u)", data.trace_width, data.trace_height);
  CHECK(cl.groups_x == 25u && cl.groups_y == 13u, "8x8 groups over 200x100: %u x %u", cl.groups_x, cl.groups_y);
  CHECK(cl.null_srv_pushes == 1 && cl.null_srv_count == rtao::kRtaoSrvCount, "all 20 SRVs nulled on the context (pushes %d count %u)", cl.null_srv_pushes, cl.null_srv_count);
  CHECK(cl.dispatch_t19 && cl.snaps[0].srv[19].handle == 0x9400u, "t19 is the P2b records view at the dispatch (t19 %d handle %llx)", cl.dispatch_t19, (unsigned long long)cl.snaps[0].srv[19].handle);
  uint32_t fade_records = 0u;
  if (cl.rtao_push.size() == rtao::kRtaoPushConstantCount) std::memcpy(&fade_records, &cl.rtao_push[11], sizeof(uint32_t));
  CHECK(fade_records == 3u, "fade[3] bits are the P2b record count (%u)", fade_records);
  CHECK(cl.null_uav_pushes == 1 && cl.null_uav_count == rtao::kRtaoUavCount && !cl.pushed_nonnull_uav, "u0 and u1 nulled on the context, no UAV left bound");
  // The RTAO views were bound at dispatch (the model works), and every RTAO SRV/UAV slot is null afterwards.
  CHECK(cl.dispatch_t9 && cl.dispatch_t17 && cl.dispatch_t18 && cl.dispatch_t19 && !cl.dispatch_u0 && cl.dispatch_u1,
        "RTAO views were bound during the dispatch, u0 null (t9 %d t17 %d t18 %d t19 %d u0 %d u1 %d)",
        cl.dispatch_t9, cl.dispatch_t17, cl.dispatch_t18, cl.dispatch_t19, cl.dispatch_u0, cl.dispatch_u1);
  CHECK(cl.snaps[0].uav[0].handle == 0u && cl.snaps[0].uav[2].handle == data.trace_uav.handle, "pass A at Full: u0 null, u2 is the trace raw AO");
  CHECK(cl.rtao_push.size() == rtao::kRtaoPushConstantCount && cl.rtao_push[19] == 1.f, "pass A push size[3] = 1 (output_raw)");
  int srv_left = 0, uav_left = 0;
  for (uint32_t i = 0; i < rtao::kRtaoSrvCount; ++i) srv_left += cl.cs_srv[i].handle != 0u ? 1 : 0;
  for (uint32_t i = 0; i < rtao::kRtaoUavCount; ++i) uav_left += cl.cs_uav[i].handle != 0u ? 1 : 0;
  CHECK(srv_left == 0 && uav_left == 0, "after Dispatch: %d RTAO SRV slots and %d UAV slots still bound on the context", srv_left, uav_left);

  rtao::DestroyRtaoDeviceData(&dev);
  CHECK(rtao::g_rtao_devices.find(&dev) == rtao::g_rtao_devices.end(), "RTAO data removed with the device");
  CHECK(dev.res.empty(), "all RTAO device objects destroyed (%zu left)", dev.res.size());
}

// ---- Parameter snapshot (rtao_state.hpp): equality decides the reset rule (R2-S4) ----
static void TestParameterSnapshot() {
  const rtao::ParameterSnapshot base = {1.f, 2.f, 1.f, 2.f, 0.05f, 1.f, 1.f, 40.f, 80.f, 1.f, 0.9f, 0.05f, 0.9f, 0.f};
  CHECK(base == base, "snapshot equals itself");
  rtao::ParameterSnapshot changed = base;
  changed.radius = 1.5f; CHECK(!(changed == base), "radius change is seen");
  changed = base; changed.ray_max = 3.f; CHECK(!(changed == base), "ray max change is seen");
  changed = base; changed.strength = 0.5f; CHECK(!(changed == base), "strength change is seen");
  changed = base; changed.samples = 4.f; CHECK(!(changed == base), "samples change is seen");
  changed = base; changed.normal_bias = 0.01f; CHECK(!(changed == base), "normal bias change is seen");
  changed = base; changed.two_sided = 0.f; CHECK(!(changed == base), "two-sided change is seen");
  changed = base; changed.isfast = 0.f; CHECK(!(changed == base), "isfast change is seen");
  changed = base; changed.fade_start = 30.f; CHECK(!(changed == base), "fade start change is seen");
  changed = base; changed.fade_end = 90.f; CHECK(!(changed == base), "fade end change is seen");
  changed = base; changed.temporal_enabled = 0.f; CHECK(!(changed == base), "temporal change is seen");
  changed = base; changed.history_weight = 0.5f; CHECK(!(changed == base), "history weight change is seen");
  changed = base; changed.depth_rejection = 0.1f; CHECK(!(changed == base), "depth rejection change is seen");
  changed = base; changed.normal_rejection = 0.5f; CHECK(!(changed == base), "normal rejection change is seen");
  changed = base; changed.freeze_noise = 1.f; CHECK(!(changed == base), "freeze noise change is seen");
  changed = base; changed.debug = 1.f; CHECK(!(changed == base), "debug mode change is seen");
  CHECK(std::string(rtao::ReasonName(rtao::Reason::NoMotion)) == "no_motion", "no_motion name");
  CHECK(std::string(rtao::ReasonName(rtao::Reason::HistoryReset)) == "history_reset", "history_reset name");
  std::printf("parameter snapshot: 15 field checks\n");
}

// ---- Two-Sided discovery index (rtao_state.hpp TwoSidedDiscoveryIndex; shader: back*4 + toward*2 + alpha) ----
static void TestTwoSidedIndex() {
  CHECK(rtao::kRtaoStatsCount == 24u && rtao::kRtaoStatDiscoveryBase == 16u && rtao::kRtaoStatDiscoveryCount == 8u, "discovery stats layout");
  CHECK(rtao::TwoSidedDiscoveryIndex(false, false, false) == 0u, "front away opaque is 0");
  CHECK(rtao::TwoSidedDiscoveryIndex(false, false, true) == 1u, "front away alpha is 1");
  CHECK(rtao::TwoSidedDiscoveryIndex(false, true, false) == 2u, "front toward opaque is 2");
  CHECK(rtao::TwoSidedDiscoveryIndex(false, true, true) == 3u, "front toward alpha is 3");
  CHECK(rtao::TwoSidedDiscoveryIndex(true, false, false) == 4u, "back away opaque is 4");
  CHECK(rtao::TwoSidedDiscoveryIndex(true, false, true) == 5u, "back away alpha is 5");
  CHECK(rtao::TwoSidedDiscoveryIndex(true, true, false) == 6u, "back toward opaque is 6");
  CHECK(rtao::TwoSidedDiscoveryIndex(true, true, true) == 7u, "back toward alpha is 7");
  std::printf("two-sided index: 8 cases\n");
}

// ---- Motion source check (rtao_state.hpp CheckMotionResources): equal, different, unknown ----
static void TestMotionCheck() {
  CHECK(rtao::CheckMotionResources(7u, 7u) == rtao::MotionCheck::Ok, "same resource: ok");
  CHECK(rtao::CheckMotionResources(7u, 9u) == rtao::MotionCheck::Conflict, "different resources: conflict");
  CHECK(rtao::CheckMotionResources(0u, 9u) == rtao::MotionCheck::Ok, "TAA t3 not captured yet: no decision");
  CHECK(rtao::CheckMotionResources(7u, 0u) == rtao::MotionCheck::Ok, "RTV4 not captured yet: no decision");
  CHECK(rtao::CheckMotionResources(0u, 0u) == rtao::MotionCheck::Ok, "neither captured: no decision");
  std::printf("motion check: 5 cases\n");
}
// ---- Reset rule (rtao_state.hpp TemporalResetReason): exhaustive over the nine inputs ----
static void TestResetRule() {
  int checked = 0;
  for (unsigned bits = 0; bits < 512u; ++bits) {
    rtao::TemporalFrame f = {};
    f.temporal_on = (bits & 1u) != 0u;
    f.was_on = (bits & 2u) != 0u;
    f.resized = (bits & 4u) != 0u;
    f.guard = (bits & 8u) != 0u;
    f.scaled = (bits & 16u) != 0u;
    f.params_changed = (bits & 32u) != 0u;
    f.motion_ok = (bits & 64u) != 0u;
    const bool gap = (bits & 128u) != 0u;
    const bool never = (bits & 256u) != 0u;
    f.frame = 100u;
    f.last_frame = never ? UINT64_MAX : (gap ? 90u : 99u);
    // The specified priority, written out once more here.
    rtao::ResetReason want = rtao::ResetReason::None;
    if (!f.temporal_on) want = rtao::ResetReason::Disabled;
    else if (!f.was_on) want = rtao::ResetReason::TemporalEnabled;
    else if (f.resized) want = rtao::ResetReason::Resize;
    else if (gap || never) want = rtao::ResetReason::FrameGap;
    else if (f.guard) want = rtao::ResetReason::Guard;
    else if (f.scaled) want = rtao::ResetReason::Scaled;
    else if (f.params_changed) want = rtao::ResetReason::Parameters;
    else if (!f.motion_ok) want = rtao::ResetReason::NoMotion;
    CHECK(rtao::TemporalResetReason(f) == want, "reset rule bits %u: got %s want %s", bits,
          rtao::ResetReasonName(rtao::TemporalResetReason(f)), rtao::ResetReasonName(want));
    checked += 1;
  }
  // Boundary: one frame apart is not a gap, two frames apart is.
  rtao::TemporalFrame edge = {true, true, false, 100u, 99u, false, false, false, true};
  CHECK(rtao::TemporalResetReason(edge) == rtao::ResetReason::None, "one frame apart: history usable");
  edge.last_frame = 98u;
  CHECK(rtao::TemporalResetReason(edge) == rtao::ResetReason::FrameGap, "two frames apart: frame gap");
  std::printf("reset rule: %d input combinations checked\n", checked);
}

// REAL: rtao::RtaoNoiseSample. The reference is the round-1 expressions written out again (transcription).
static void TestNoiseSample() {
  bool same = true;
  for (uint32_t spp = 1u; spp <= 8u; ++spp) {
    for (uint64_t frame = 0u; frame < 2100u; frame += 7u) {
      const rtao::NoiseSample n = rtao::RtaoNoiseSample(frame, spp, false);
      same = same && n.slice_base == static_cast<float>((frame * spp) % 32u) && n.seed == static_cast<float>(frame % 1024u);
      const rtao::NoiseSample f = rtao::RtaoNoiseSample(frame, spp, true);
      same = same && f.slice_base == 0.f && f.seed == 0.f;
    }
  }
  CHECK(same, "freeze off: slice and seed equal the round-1 formulas; freeze on: 0 and 0");
  std::printf("noise sample: spp 1..8, frames 0..2100 (real function)\n");
}

// ---- Source-text guard for the RTAO shaders (NOT a shader compile): #define names, duplicates, and the index values
// against the C++ constants. Reads the harness copy of the shaders (refresh_world.ps1 copies world/*.hlsl there, CR
// bytes stripped), so run refresh_world first. ----
static std::string ReadText(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
struct HlslDefine { std::string name; long value; };
// Parses "#define NAME <digits>[u]" lines; other #define forms are skipped.
static std::vector<HlslDefine> ParseDefines(const std::string& text) {
  std::vector<HlslDefine> out;
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    const size_t p = line.find_first_not_of(" \t");
    if (p == std::string::npos || line.compare(p, 8, "#define ") != 0) continue;
    std::istringstream toks(line.substr(p + 8));
    std::string name, value;
    toks >> name >> value;
    if (!value.empty() && value.back() == 'u') value.pop_back();
    if (name.empty() || value.empty() || value.find_first_not_of("0123456789") != std::string::npos) continue;
    out.push_back({name, std::strtol(value.c_str(), nullptr, 10)});
  }
  return out;
}
static int CountDuplicates(const std::vector<HlslDefine>& defs) {
  std::map<std::string, int> seen;
  int dups = 0;
  for (const auto& d : defs) if (++seen[d.name] == 2) ++dups;
  return dups;
}
static bool FindDefine(const std::vector<HlslDefine>& defs, const char* name, long* value) {
  for (const auto& d : defs) if (d.name == name) { *value = d.value; return true; }
  return false;
}
static void TestHlslSources() {
  const std::string pass_a = ReadText("src/games/falcomengine-plus/world/shaders/world_rtao.cs_5_0.hlsl");
  const std::string svgf_input = ReadText("src/games/falcomengine-plus/world/shaders/world_rtao_svgf_input.cs_5_0.hlsl");
  const std::string upsample = ReadText("src/games/falcomengine-plus/world/shaders/gtvbao_svgf_upsample.cs_5_0.hlsl");
  CHECK(!pass_a.empty() && !svgf_input.empty() && !upsample.empty(), "RTAO and SVGF shader sources readable (run refresh_world first)");
  const std::vector<HlslDefine> defs = ParseDefines(pass_a);
  CHECK(CountDuplicates(defs) == 0, "no #define name appears twice in the RTAO pass A shader");
  CHECK(CountDuplicates(ParseDefines("#define X_TEST 1u\n#define X_TEST 2u\n")) == 1, "guard self-test: a duplicate is found");
  struct Pair { const char* name; uint32_t expected; };
  static const Pair pairs[] = {
      {"RTAO_STAT_RAYS", rtao::kRtaoStatRays}, {"RTAO_STAT_HITS", rtao::kRtaoStatHits},
      {"RTAO_STAT_PIXELS", rtao::kRtaoStatPixels}, {"RTAO_STAT_AO_SUM", rtao::kRtaoStatAoSum},
      {"RTAO_STAT_SKY", rtao::kRtaoStatSky}, {"RTAO_STAT_NORMAL", rtao::kRtaoStatNormal},
      {"RTAO_STAT_REGION", rtao::kRtaoStatRegion}, {"RTAO_STAT_INVALID_REFS", rtao::kRtaoStatInvalidRefs},
      {"RTAO_STAT_STACK_OVERFLOW", rtao::kRtaoStatStackOverflow}, {"RTAO_STAT_SCALED", rtao::kRtaoStatScaled},
      {"RTAO_DISC_BASE", rtao::kRtaoStatDiscoveryBase}, {"RTAO_DISC_COUNT", rtao::kRtaoStatDiscoveryCount},
  };
  for (const Pair& pr : pairs) {
    long value = -1;
    const bool found = FindDefine(defs, pr.name, &value);
    CHECK(found && value == static_cast<long>(pr.expected), "%s = %ld in the HLSL, %u in C++ (found %d)",
          pr.name, value, pr.expected, found ? 1 : 0);
  }
  long disc_base = 0, disc_count = 0;
  FindDefine(defs, "RTAO_DISC_BASE", &disc_base);
  FindDefine(defs, "RTAO_DISC_COUNT", &disc_count);
  CHECK(disc_base + disc_count <= static_cast<long>(rtao::kRtaoStatsCount), "discovery counters inside the stats buffer (%ld + %ld)", disc_base, disc_count);
  CHECK(svgf_input.find("float4(1.0, 1.0, 1.0, -1.0)") != std::string::npos, "SVGF input pass writes the not-traced texel (a = -1)");
  CHECK(upsample.find("#define RTGI_UPSAMPLE_AO_BYTE") != std::string::npos, "the shared upsample is the AO byte variant");
  std::printf("HLSL source guard: %zu defines, %zu index pairs (source text, not a shader compile)\n", defs.size(), sizeof(pairs) / sizeof(pairs[0]));
}

// ---- P3: the debug field (REAL compile check of shared.h) and source-text checks of the lighting shader ----
// REAL: the field is compiled (a missing name fails the build). TEXT: the old name is gone from shared.h.
// TEXT: the lighting shader is read from the repo (refresh_world does not copy sora2nd, so this reads the repo file).
static int CountOccurrences(const std::string& text, const std::string& needle) {
  int n = 0;
  for (size_t p = text.find(needle); p != std::string::npos; p = text.find(needle, p + needle.size())) ++n;
  return n;
}
static void TestDebugShowField() {
  ShaderInjectData injection = {};
  injection.rtao_debug_show = 1.f;
  CHECK(injection.rtao_debug_show == 1.f, "ShaderInjectData.rtao_debug_show compiles and holds its value");
  const std::string header = ReadText("../../src/games/falcomengine-plus/shared.h");
  CHECK(!header.empty(), "shared.h readable from the harness directory");
  CHECK(CountOccurrences(header, "rtao_debug_show") == 1, "shared.h declares rtao_debug_show exactly once (text)");
  CHECK(CountOccurrences(header, "char_shadow_surface_thickness") == 0, "the retired name is gone from shared.h (text)");
  const std::string lighting = ReadText("../../src/games/falcomengine-plus/sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl");
  CHECK(!lighting.empty(), "lighting shader readable from the repo (text)");
  CHECK(CountOccurrences(lighting, "shader_injection_data.gtvbao_debug_view > 0.5f || shader_injection_data.rtao_debug_show > 0.5f") == 1,
        "debug block condition includes rtao_debug_show (text)");
  CHECK(CountOccurrences(lighting, "int mode = shader_injection_data.rtao_debug_show > 0.5f ? 2 :") == 1,
        "mode selection line exists once and picks mode 2 for RTAO (text)");
  CHECK(CountOccurrences(lighting, "gtvbaoTexture.Load") == 4, "gtvbaoTexture.Load count is still 4 (text)");
  std::printf("P3 debug field: compiled field, 6 text checks (shared.h and the lighting shader)\n");
}

// ---- Step A: world_rtao.json is valid JSON and the root keys are top level. REAL: rtao::BuildRtaoJson. The JSON
// validator below is a recursive-descent parser written for this test (objects, arrays, strings with escapes,
// numbers with sign, fraction and exponent, true, false, null; any malformed or trailing content fails). ----
struct JNode {
  enum class Kind { Null, Bool, Number, String, Array, Object };
  Kind kind = Kind::Null;
  std::string text;
  std::vector<std::string> keys;  // object keys, in order
  std::vector<JNode> items;       // array items, or object values in key order
};
struct JsonParser {
  const std::string& s;
  size_t i = 0;
  explicit JsonParser(const std::string& text) : s(text) {}
  void Ws() {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) ++i;
  }
  bool Lit(const char* w) {
    const size_t n = std::strlen(w);
    if (s.compare(i, n, w) == 0) { i += n; return true; }
    return false;
  }
  bool Str(std::string* out) {
    if (i >= s.size() || s[i] != '"') return false;
    ++i;
    while (i < s.size()) {
      const char c = s[i++];
      if (c == '"') return true;
      if (static_cast<unsigned char>(c) < 0x20) return false;
      if (c == '\\') {
        if (i >= s.size()) return false;
        const char e = s[i++];
        if (e == 'u') {
          for (int k = 0; k < 4; ++k) {
            if (i >= s.size() || !std::isxdigit(static_cast<unsigned char>(s[i]))) return false;
            ++i;
          }
        } else if (e == '\0' || std::strchr("\"\\/bfnrt", e) == nullptr) {
          return false;
        }
        if (out) out->push_back('?');
      } else if (out) {
        out->push_back(c);
      }
    }
    return false;
  }
  bool Digits() {
    const size_t st = i;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
    return i > st;
  }
  bool Num() {
    if (i < s.size() && s[i] == '-') ++i;
    if (i < s.size() && s[i] == '0') ++i;
    else if (!Digits()) return false;
    if (i < s.size() && s[i] == '.') { ++i; if (!Digits()) return false; }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
      ++i;
      if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
      if (!Digits()) return false;
    }
    return true;
  }
  bool Value(JNode* n) {
    Ws();
    if (i >= s.size()) return false;
    const char c = s[i];
    if (c == '{') {
      ++i;
      n->kind = JNode::Kind::Object;
      Ws();
      if (i < s.size() && s[i] == '}') { ++i; return true; }
      while (true) {
        Ws();
        std::string key;
        if (!Str(&key)) return false;
        Ws();
        if (i >= s.size() || s[i] != ':') return false;
        ++i;
        JNode child;
        if (!Value(&child)) return false;
        n->keys.push_back(key);
        n->items.push_back(child);
        Ws();
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        if (i < s.size() && s[i] == '}') { ++i; return true; }
        return false;
      }
    }
    if (c == '[') {
      ++i;
      n->kind = JNode::Kind::Array;
      Ws();
      if (i < s.size() && s[i] == ']') { ++i; return true; }
      while (true) {
        JNode child;
        if (!Value(&child)) return false;
        n->items.push_back(child);
        Ws();
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        if (i < s.size() && s[i] == ']') { ++i; return true; }
        return false;
      }
    }
    if (c == '"') {
      n->kind = JNode::Kind::String;
      return Str(&n->text);
    }
    if (Lit("true")) { n->kind = JNode::Kind::Bool; n->text = "true"; return true; }
    if (Lit("false")) { n->kind = JNode::Kind::Bool; n->text = "false"; return true; }
    if (Lit("null")) { n->kind = JNode::Kind::Null; return true; }
    n->kind = JNode::Kind::Number;
    const size_t st = i;
    if (!Num()) return false;
    n->text = s.substr(st, i - st);
    return true;
  }
  bool Parse(JNode* root) {
    if (!Value(root)) return false;
    Ws();
    return i == s.size();
  }
};
static const JNode* Member(const JNode& obj, const char* key) {
  if (obj.kind != JNode::Kind::Object) return nullptr;
  for (size_t k = 0; k < obj.keys.size(); ++k) {
    if (obj.keys[k] == key) return &obj.items[k];
  }
  return nullptr;
}
// perf_gather follows the switch the dump was written with (the RTAO timings are only gathered while it is on).
static std::string WorldJsonWithGather(bool gather) {
  static rtao::RtaoFrameState f;
  f = rtao::RtaoFrameState{};
  static uint32_t values[rtao::kRtaoStatsCount];
  std::memset(values, 0, sizeof(values));
  rtao::RtaoJsonInput in = {};
  in.frame = &f;
  in.values = values;
  in.generated_frame = 7u;
  in.perf_gather = gather;
  return rtao::BuildRtaoJson(in);
}
static std::string WorldJsonForState(int which) {
  static rtao::RtaoFrameState f;
  f = rtao::RtaoFrameState{};
  static uint32_t values[rtao::kRtaoStatsCount];
  std::memset(values, 0, sizeof(values));
  rtao::RtaoJsonInput in = {};
  in.frame = &f;
  in.values = values;
  in.generated_frame = 42u;
  if (which >= 1) {
    f.temporal_ran = true;
    f.reset = rtao::ResetReason::Parameters;
    f.last_reset = rtao::ResetReason::FrameGap;
    f.dispatched_frame = 40u;
    f.captured.samples = 2.f;
    f.captured.history_weight = 0.9f;
    values[rtao::kRtaoStatDenoiseBase] = 1000u;
    in.temporal_on = true;
    in.freeze_noise = true;
  }
  if (which == 2) {
    for (size_t r = 0; r < rtao::kReasonCount; ++r) f.frames_without_ao_by_reason[r] = static_cast<uint64_t>(r + 1u);
    for (size_t r = 0; r < static_cast<size_t>(rtao::ResetReason::Count); ++r) f.reset_counts[r] = static_cast<uint64_t>(r + 1u);
    for (uint32_t cull = 0u; cull < 4u; ++cull) for (uint32_t ccw = 0u; ccw < 2u; ++ccw) in.cpu_draws[cull][ccw] = 3u;
  }
  return rtao::BuildRtaoJson(in);
}
static void CheckWorldJson(const char* name, const std::string& text) {
  JNode root;
  JsonParser parser(text);
  CHECK(parser.Parse(&root), "%s: world_rtao.json parses as JSON (recursive-descent check)", name);
  CHECK(root.kind == JNode::Kind::Object, "%s: root is an object", name);
  static const char* const root_keys[] = {"schema", "generated_frame", "perf_gather", "reason", "producing", "gpu_ms",
                                          "denoise_gpu_ms", "texture_bytes", "p2b_records", "character_objects",
                                          "frames_without_ao", "fade", "stats", "temporal", "svgf", "two_sided_discovery",
                                          "temporal_diagnostics", "test_modes", "note"};
  for (const char* key : root_keys) CHECK(Member(root, key) != nullptr, "%s: root key %s is top level", name, key);
  const JNode* perf_gather = Member(root, "perf_gather");
  CHECK(perf_gather && perf_gather->kind == JNode::Kind::Bool && perf_gather->text == "false", "%s: perf_gather is false (gathering off)", name);
  CHECK(WorldJsonWithGather(true).find("\"perf_gather\": true") != std::string::npos, "%s: perf_gather is true when gathering is on", name);
  const JNode* schema = Member(root, "schema");
  CHECK(schema && schema->text == "3", "%s: schema is 3", name);
  const JNode* temporal = Member(root, "temporal");
  CHECK(temporal && temporal->kind == JNode::Kind::Object && !temporal->keys.empty() && temporal->keys.back() == "reset_counts",
        "%s: temporal is an object whose last key is reset_counts", name);
  for (const char* key : {"svgf", "test_modes", "two_sided_discovery", "temporal_diagnostics", "note"}) {
    CHECK(temporal && Member(*temporal, key) == nullptr, "%s: %s is not inside temporal", name, key);
  }
  const JNode* svgf = Member(root, "svgf");
  CHECK(svgf && svgf->kind == JNode::Kind::Object && !svgf->keys.empty() && svgf->keys.back() == "reset_pixels",
        "%s: svgf is an object whose last key is reset_pixels", name);
}
static void TestWorldJson() {
  CheckWorldJson("default state", WorldJsonForState(0));
  CheckWorldJson("temporal on, test rows on", WorldJsonForState(1));
  CheckWorldJson("every reason count non-zero", WorldJsonForState(2));
  // The validator rejects malformed text (self-test).
  JNode bad;
  JsonParser trailing("{\"a\": 1} x");
  CHECK(!trailing.Parse(&bad), "validator self-test: trailing content fails");
  JNode nan_node;
  JsonParser nan_text("{\"a\": nan}");
  CHECK(!nan_text.Parse(&nan_node), "validator self-test: nan is not a JSON number");
  std::printf("world_rtao.json: 3 states, root keys, temporal closed, validator self-test (real builder, text parser)\n");
}

// ---- SVGF glue (rtao_svgf.hpp) on the mock device: dispatch order, bindings, sizes, output view, lifecycle ----
static void TestSvgfDispatch() {
  Device dev;
  CmdList cl;
  cl.dev = &dev;
  falcom_world::bvh::BvhDeviceData* bvh_data = falcom_world::bvh::GetBvhDeviceData(&dev);
  bvh_data->dynamic.instances_srv = {0x9400u};
  bvh_data->dynamic.instance_count = 0u;
  rtao::RtaoSvgfInputs svgf_in = {};
  svgf_in.depth_view = {0x9000u};
  svgf_in.mrt_normal_view = {0x9200u};
  svgf_in.scene_cbv_view = {0x9100u};
  svgf_in.motion_view = {0x9500u};
  svgf_in.full_width = 64u;
  svgf_in.full_height = 32u;
  svgf_in.trace_width = 32u;
  svgf_in.trace_height = 16u;
  svgf_in.divisor = 2u;
  svgf_in.temporal = true;
  svgf_in.spatial = true;
  svgf_in.stats = false;
  svgf_in.svgf = {0.8f, 0.2f, 0.f, true, 5.f, 4.f};
  rtao::RtaoDispatchInputs in = {};
  in.depth_view = svgf_in.depth_view;
  in.mrt_normal_view = svgf_in.mrt_normal_view;
  in.isfast_view = {0x9300u};
  in.scene_cbv_view = svgf_in.scene_cbv_view;
  in.width = 64u;
  in.height = 32u;
  in.trace_divisor = 2u;

  const falcom_world::rtgi::RtgiDenoiseReady ready = rtao::EnsureRtaoSvgf(&dev, svgf_in);
  CHECK(ready.ok && ready.temporal && ready.spatial, "chain ready with temporal and spatial");
  rtao::RtaoSvgfData& sd = rtao::GetRtaoSvgfData(&dev);
  rtao::RtaoDeviceData& rd = rtao::GetRtaoDeviceData(&dev);
  CHECK(sd.denoise.output.width == 64u && sd.denoise.output.height == 32u, "output is full size (%u x %u)", sd.denoise.output.width, sd.denoise.output.height);
  CHECK(sd.raw.width == 32u && sd.raw.height == 16u, "SVGF raw is trace size (%u x %u)", sd.raw.width, sd.raw.height);
  const auto& out_desc = dev.res[sd.denoise.output.texture[0].handle].desc.texture;
  const auto& raw_desc = dev.res[sd.raw.texture[0].handle].desc.texture;
  CHECK(out_desc.format == reshade::api::format::r32_uint && out_desc.width == 64u && out_desc.height == 32u, "output is r32_uint 64 x 32");
  CHECK(raw_desc.format == reshade::api::format::r16g16b16a16_float && raw_desc.width == 32u && raw_desc.height == 16u, "raw is rgba16f 32 x 16");

  const bool dispatched = rtao::Dispatch(&dev, &cl, in).ok;
  rtao::DispatchRtaoSvgf(&dev, &cl, svgf_in, ready);
  CHECK(dispatched, "pass A dispatched");
  CHECK(cl.dispatches == 9, "pass A, input, reproject, variance, four a-trous, upsample: %d dispatches", cl.dispatches);
  CHECK(sd.denoise.atrous_iterations == 4u, "Half: 4 a-trous iterations (%u)", sd.denoise.atrous_iterations);
  CHECK(cl.snaps[0].uav[0].handle == 0u && cl.snaps[0].uav[2].handle == rd.trace_uav.handle, "pass A: u0 null, u2 the trace AO");
  CHECK(cl.snaps[1].srv[0].handle == rd.trace_srv.handle && cl.snaps[1].uav[0].handle == sd.raw.uav[0].handle,
        "input pass: t0 the trace AO, u0 the SVGF raw");
  CHECK(cl.snaps[2].srv[0].handle == sd.raw.srv[0].handle, "reproject reads the SVGF raw at t0");
  CHECK(cl.snaps[2].uav[1].handle == 0u, "reproject: stats null with stats off");
  CHECK(cl.snaps[8].uav[0].handle == sd.denoise.output.uav[sd.denoise.output_index].handle, "upsample writes the newest output (u0)");
  CHECK(rtao::OutputSrv(&dev).handle == sd.denoise.output.srv[sd.denoise.output_index].handle && rtao::OutputSrv(&dev).handle != 0u,
        "OutputSrv is the newest output");
  int cs_left = 0;
  for (const auto& v : cl.cs_srv) cs_left += v.handle != 0u ? 1 : 0;
  for (const auto& v : cl.cs_uav) cs_left += v.handle != 0u ? 1 : 0;
  CHECK(cs_left == 0, "after the run no SRV or UAV slot is bound (%d left)", cs_left);

  CmdList cl_stats;
  cl_stats.dev = &dev;
  svgf_in.stats = true;
  rtao::DispatchRtaoSvgf(&dev, &cl_stats, svgf_in, ready);
  CHECK(cl_stats.dispatches == 8, "stats run: input, reproject, variance, four a-trous, upsample (%d)", cl_stats.dispatches);
  CHECK(cl_stats.snaps[1].uav[1].handle == rd.stats_uav.handle, "reproject: u1 the stats buffer with stats on");

  CmdList cl3;
  cl3.dev = &dev;
  svgf_in.stats = false;
  svgf_in.temporal = false;
  svgf_in.spatial = false;
  const falcom_world::rtgi::RtgiDenoiseReady ready3 = rtao::EnsureRtaoSvgf(&dev, svgf_in);
  CHECK(ready3.ok && !ready3.temporal && !ready3.spatial, "chain ready with temporal and spatial off");
  rtao::Dispatch(&dev, &cl3, in);
  rtao::DispatchRtaoSvgf(&dev, &cl3, svgf_in, ready3);
  CHECK(cl3.dispatches == 3, "temporal and spatial off: pass A, input, upsample (%d dispatches)", cl3.dispatches);
  CHECK(cl3.snaps[2].srv[2].handle == sd.raw.srv[0].handle, "upsample reads the raw AO at t2 with the filters off");

  CHECK(falcom_world::rtgi::SvgfEffectiveIterations(5.f, 1u) == 5u && falcom_world::rtgi::SvgfEffectiveIterations(5.f, 2u) == 4u
        && falcom_world::rtgi::SvgfEffectiveIterations(5.f, 4u) == 3u, "iterations 5, 4, 3 at Full, Half, Quarter");

  rtao::ReleaseRtaoTextures(&dev);
  int textures = 0;
  for (const auto& entry : dev.res) textures += entry.second.desc.type == resource_type::texture_2d ? 1 : 0;
  CHECK(textures == 0, "release leaves no texture_2d (%d alive)", textures);
  CHECK(sd.denoise.upsample_pipeline.pipeline.handle != 0u && rd.pipeline.handle != 0u, "release keeps the pipelines");

  rtao::DestroyRtaoSvgfData(&dev);
  rtao::DestroyRtaoDeviceData(&dev);
  CHECK(rtao::g_rtao_svgf_devices.find(&dev) == rtao::g_rtao_svgf_devices.end(), "SVGF data removed with the device");
  CHECK(rtao::g_rtao_devices.find(&dev) == rtao::g_rtao_devices.end(), "RTAO data removed with the device");
  CHECK(dev.res.empty(), "all device objects destroyed (%zu left)", dev.res.size());
}

int main() {
  HStage("gate table");
  TestGateTable();
  HStage("fade range");
  TestFade();
  HStage("region margin");
  TestRegionMargin();
  HStage("b12 layout");
  TestPushLayout();
  HStage("rays, falloff, fade");
  TestRayRules();
  HStage("pixel classes");
  TestClasses();
  HStage("parameter snapshot");
  TestParameterSnapshot();
  HStage("reset rule");
  TestResetRule();
  TestMotionCheck();
  TestTwoSidedIndex();
  TestNoiseSample();
  TestHlslSources();
  TestDebugShowField();
  TestWorldJson();
  HStage("device lifecycle (mock)");
  TestDeviceLifecycle();
  HStage("svgf dispatch (mock)");
  TestSvgfDispatch();
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
