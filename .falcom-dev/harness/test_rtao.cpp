// Native harness: RTAO (world/rtao), rounds 1 and 2 and the discovery index.
//
// TRANSCRIPTIONS (the test runs a copy of the shader logic, not the GPU code):
//   - gate table (16 combinations of T, Rd, G, V): the addon.cpp gate expressions written out again.
//   - ray generation, falloff, fade and strength (TestRayRules): world_rtao.cs_5_0.hlsl TracePixel on a wall
//     scene; the reference is a second sample sequence, not the BVH.
//   - temporal blend (TestTemporalBlend): world_rtao_temporal.cs_5_0.hlsl TemporalPixel after the reprojection
//     taps; the reprojection (CustomTAA_Reproject) is an input, not run here.
//   - pixel classes (sky, invalid normal, scaled, region): the shader's order, written out again.
//   - reset rule: the function in rtao_state.hpp is real, tested over all 512 input combinations against the
//     priority written out in the test.
//   - fade range and region margin: rtao_state.hpp / the shader rule written out again.
//
// REAL CODE RUN ON THE MOCK DEVICE (the addon's own functions):
//   - rtao::Dispatch: pass A and pass B bindings, nulls on the context after each pass, the history swap, the
//     no-motion path, the samplers, destroy (TestDeviceLifecycle, TestTemporalDispatch).
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
#define __world_rtao_temporal_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_temporal_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao_temporal{__world_rtao_temporal_base};
#define __world_rtao_filter_EMBED_FILE
inline constexpr std::uint8_t __world_rtao_filter_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_rtao_filter{__world_rtao_filter_base};
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "src/games/falcomengine-plus/world/rtao/rtao.hpp"
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
struct FadeCase { float size, ray_max, start, end; float end_eff, start_eff; };

static void TestFade() {
  const FadeCase cases[] = {
      {64.f, 2.f, 40.f, 80.f, 14.f, 14.f},     // coverage 14: end capped
      {128.f, 2.f, 40.f, 80.f, 30.f, 30.f},    // coverage 30: end capped, start follows
      {256.f, 2.f, 40.f, 80.f, 62.f, 40.f},    // coverage 62: end capped only by the region
      {512.f, 2.f, 40.f, 80.f, 80.f, 40.f},    // coverage 126: stored values reach
      {512.f, 2.f, 90.f, 50.f, 50.f, 50.f},    // start > end: start capped to end
      {512.f, 10.f, 40.f, 128.f, 118.f, 40.f}, // ray max 10: coverage 118
  };
  for (const auto& c : cases) {
    const rtao::Fade f = rtao::RtaoEffectiveFade(c.size, c.ray_max, c.start, c.end);
    CHECK(Near(f.end, c.end_eff, 1e-4f), "size %.0f ray %.1f end_eff %.2f want %.2f", c.size, c.ray_max, f.end, c.end_eff);
    CHECK(Near(f.start, c.start_eff, 1e-4f), "size %.0f start_eff %.2f want %.2f", c.size, f.start, c.start_eff);
    CHECK(f.end <= f.coverage + 1e-4f && f.start <= f.end + 1e-4f, "size %.0f ordering", c.size);
    CHECK(Near(f.coverage, c.size * 0.25f - c.ray_max, 1e-4f), "coverage formula");
  }
  // Not ready: coverage below 1 m (the caller reports range_too_small).
  CHECK(rtao::RtaoEffectiveFade(4.f, 3.5f, 40.f, 80.f).coverage < 1.f, "small region is not ready");
  // The slider maximum is kPoolRegionMaxSize * 0.25 = 128 m; a 512 m region gives coverage 126 m, so the
  // stored End (up to 128) is capped there and never rewritten.
  const rtao::Fade big = rtao::RtaoEffectiveFade(512.f, 2.f, 128.f, 128.f);
  CHECK(Near(big.end, 126.f, 1e-4f) && Near(big.start, 126.f, 1e-4f), "max slider capped by coverage");
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
  CHECK(sizeof(rtao::RtaoPushConstants) == 20u * sizeof(float), "b12 size %zu", sizeof(rtao::RtaoPushConstants));
  CHECK(offsetof(rtao::RtaoPushConstants, params) == 0u, "c0 offset");
  CHECK(offsetof(rtao::RtaoPushConstants, sampling) == 16u, "c1 offset");
  CHECK(offsetof(rtao::RtaoPushConstants, fade) == 32u, "c2 offset");
  CHECK(offsetof(rtao::RtaoPushConstants, region) == 48u, "c3 offset");
  CHECK(offsetof(rtao::RtaoPushConstants, size) == 64u, "c4 offset");
  CHECK(rtao::kRtaoStatsCount == 71u && rtao::kRtaoStatScaled == 9u && rtao::kRtaoStatTemporalBase == 10u, "stats count");
  CHECK(rtao::kRtaoSrvCount == 19u && rtao::kRtaoUavCount == 3u && rtao::kRtaoPushRegister == 12u, "table sizes");
  CHECK(sizeof(rtao::RtaoTemporalPushConstants) == 20u * sizeof(float), "pass B b12 size");
  CHECK(offsetof(rtao::RtaoTemporalPushConstants, texel) == 16u && offsetof(rtao::RtaoTemporalPushConstants, size) == 32u, "pass B offsets");
  CHECK(rtao::kRtaoTemporalSrvCount == 6u && rtao::kRtaoTemporalUavCount == 3u && rtao::kRtaoTemporalSamplerCount == 2u, "pass B table sizes");
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

// ---- Mock device lifecycle (rtao::EnsureRtaoPipeline, EnsureRtaoTarget, EnsureRtaoStats, Dispatch, destroy) ----
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
  int dispatch_srv_bound = 0, dispatch_uav_bound = 0;  // non-null slots at the moment of the dispatch
  bool dispatch_t9 = false, dispatch_t17 = false, dispatch_t18 = false, dispatch_u0 = false, dispatch_u1 = false;
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
  void push_constants(shader_stage, pipeline_layout, uint32_t, uint32_t, uint32_t, const void *) override {}
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

  CHECK(rtao::EnsureRtaoTarget(&dev, &data, 64u, 32u), "AO target created");
  CHECK(dev.texture_creates == 1, "one AO texture: %d", dev.texture_creates);
  CHECK(rtao::EnsureRtaoTarget(&dev, &data, 64u, 32u) && dev.texture_creates == 1, "same size: no new texture");
  CHECK(rtao::EnsureRtaoTarget(&dev, &data, 128u, 32u) && dev.texture_creates == 2, "resize creates a new texture");
  int textures = 0;
  for (const auto& entry : dev.res) textures += entry.second.desc.type == resource_type::texture_2d ? 1 : 0;
  CHECK(textures == 1, "resize destroyed the old AO texture (textures alive %d)", textures);
  CHECK(data.ao_width == 128u && data.ao_height == 32u, "target size tracked");
  CHECK(rtao::EnsureRtaoStats(&dev, &data) && data.stats_buffer.handle != 0u, "stats buffer created");

  rtao::RtaoDispatchInputs in = {};
  in.width = 200u;   // the captured depth size (addon.cpp sizes RTAO from captured_depth_w/h)
  in.height = 100u;
  in.depth_view = {0x9000u};
  in.mrt_normal_view = {0x9200u};
  in.isfast_view = {0x9300u};
  in.scene_cbv_view = {0x9100u};
  const bool dispatched = rtao::Dispatch(&dev, &cl, in).ok;
  CHECK(dispatched && cl.dispatches == 1, "dispatch issued (ok %d, dispatches %d)", dispatched, cl.dispatches);
  CHECK(data.ao_width == 200u && data.ao_height == 100u, "AO target sized from the depth input (%u x %u)", data.ao_width, data.ao_height);
  CHECK(cl.groups_x == 25u && cl.groups_y == 13u, "8x8 groups over 200x100: %u x %u", cl.groups_x, cl.groups_y);
  CHECK(cl.null_srv_pushes == 1 && cl.null_srv_count == rtao::kRtaoSrvCount, "all 19 SRVs nulled on the context (pushes %d count %u)", cl.null_srv_pushes, cl.null_srv_count);
  CHECK(cl.null_uav_pushes == 1 && cl.null_uav_count == rtao::kRtaoUavCount && !cl.pushed_nonnull_uav, "u0 and u1 nulled on the context, no UAV left bound");
  // The RTAO views were bound at dispatch (the model works), and every RTAO SRV/UAV slot is null afterwards.
  CHECK(cl.dispatch_t9 && cl.dispatch_t17 && cl.dispatch_t18 && cl.dispatch_u0 && cl.dispatch_u1,
        "RTAO views were bound during the dispatch (t9 %d t17 %d t18 %d u0 %d u1 %d)",
        cl.dispatch_t9, cl.dispatch_t17, cl.dispatch_t18, cl.dispatch_u0, cl.dispatch_u1);
  int srv_left = 0, uav_left = 0;
  for (uint32_t i = 0; i < rtao::kRtaoSrvCount; ++i) srv_left += cl.cs_srv[i].handle != 0u ? 1 : 0;
  for (uint32_t i = 0; i < rtao::kRtaoUavCount; ++i) uav_left += cl.cs_uav[i].handle != 0u ? 1 : 0;
  CHECK(srv_left == 0 && uav_left == 0, "after Dispatch: %d RTAO SRV slots and %d UAV slots still bound on the context", srv_left, uav_left);

  rtao::DestroyRtaoDeviceData(&dev);
  CHECK(rtao::g_rtao_devices.find(&dev) == rtao::g_rtao_devices.end(), "RTAO data removed with the device");
  CHECK(dev.res.empty(), "all RTAO device objects destroyed (%zu left)", dev.res.size());
}

// ---- Pass B blend (world_rtao_temporal.cs_5_0.hlsl TemporalPixel): TRANSCRIPTION, not the GPU code ----
// Reprojection (CustomTAA_Reproject) is not transcribed here: its taps and weights are inputs.
struct Tap { float w; bool has_history; float ao; float dist; float dot; };
struct BlendResult { float ao; float valid_fraction; };
static BlendResult TemporalBlend(float raw, const Tap taps[4], float dist_cur, float history_weight, float depth_rejection,
                                 float normal_rejection, float clamp_sigma, const float neighbours[9], bool history_valid_frame) {
  float weight_total = 0.f, weight_accepted = 0.f, history_sum = 0.f;
  for (int i = 0; i < 4; ++i) {
    weight_total += taps[i].w;
    if (!taps[i].has_history) continue;
    const bool depth_ok = std::fabs(taps[i].dist - dist_cur) / std::fmax(dist_cur, 1e-4f) <= depth_rejection;
    if (!depth_ok) continue;
    if (!(taps[i].dot >= normal_rejection)) continue;
    weight_accepted += taps[i].w;
    history_sum += taps[i].w * taps[i].ao;
  }
  const float valid_fraction = (history_valid_frame && weight_total > 0.f && weight_accepted > 1e-4f)
      ? std::fmin(std::fmax(weight_accepted / weight_total, 0.f), 1.f) : 0.f;
  float history_ao = valid_fraction > 0.f ? history_sum / weight_accepted : raw;
  if (clamp_sigma > 0.f && valid_fraction > 0.f) {
    float s1 = 0.f, s2 = 0.f, n = 0.f;
    for (int i = 0; i < 9; ++i) {
      if (neighbours[i] < 0.f) continue;
      s1 += neighbours[i];
      s2 += neighbours[i] * neighbours[i];
      n += 1.f;
    }
    const float mean = s1 / n;
    const float sigma = std::sqrt(std::fmax(s2 / n - mean * mean, 0.f));
    history_ao = std::fmin(std::fmax(history_ao, mean - clamp_sigma * sigma), mean + clamp_sigma * sigma);
  }
  const float alpha = history_weight * valid_fraction;
  return {raw + (history_ao - raw) * alpha, valid_fraction};
}

static void TestTemporalBlend() {
  const float flat[9] = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f};
  const Tap good[4] = {{0.25f, true, 0.8f, 5.f, 1.f}, {0.25f, true, 0.8f, 5.f, 1.f}, {0.25f, true, 0.8f, 5.f, 1.f}, {0.25f, true, 0.8f, 5.f, 1.f}};
  BlendResult r = TemporalBlend(0.2f, good, 5.f, 0.9f, 0.05f, 0.9f, 0.f, flat, true);
  CHECK(Near(r.valid_fraction, 1.f, 1e-6f) && Near(r.ao, 0.74f, 1e-5f), "all taps valid: ao %.4f", r.ao);
  r = TemporalBlend(0.2f, good, 5.f, 0.9f, 0.05f, 0.9f, 0.f, flat, false);
  CHECK(r.ao == 0.2f && r.valid_fraction == 0.f, "history reset: raw AO");
  Tap depth_bad[4] = {good[0], good[1], good[2], good[3]};
  for (auto& t : depth_bad) t.dist = 50.f;
  r = TemporalBlend(0.2f, depth_bad, 5.f, 0.9f, 0.05f, 0.9f, 0.f, flat, true);
  CHECK(r.valid_fraction == 0.f && r.ao == 0.2f, "depth rejected on every tap: raw AO");
  Tap normal_half[4] = {good[0], good[1], good[2], good[3]};
  normal_half[0].dot = 0.f; normal_half[1].dot = 0.f;
  r = TemporalBlend(0.2f, normal_half, 5.f, 0.9f, 0.05f, 0.9f, 0.f, flat, true);
  CHECK(Near(r.valid_fraction, 0.5f, 1e-6f) && Near(r.ao, 0.2f + 0.9f * 0.5f * 0.6f, 1e-5f), "half the taps rejected by normal: alpha halves");
  Tap none[4] = {};
  r = TemporalBlend(0.2f, none, 5.f, 0.9f, 0.05f, 0.9f, 0.f, flat, true);
  CHECK(r.valid_fraction == 0.f && r.ao == 0.2f, "no history taps: raw AO");
  r = TemporalBlend(0.2f, good, 5.f, 0.f, 0.05f, 0.9f, 0.f, flat, true);
  CHECK(r.ao == 0.2f, "history weight 0: raw AO");
  const float spread[9] = {0.1f, 0.3f, 0.1f, 0.3f, 0.2f, 0.3f, 0.1f, 0.3f, 0.1f};
  r = TemporalBlend(0.2f, good, 5.f, 0.9f, 0.05f, 0.9f, 0.f, spread, true);
  CHECK(Near(r.ao, 0.74f, 1e-5f), "clamp off: history not limited");
  r = TemporalBlend(0.2f, good, 5.f, 0.9f, 0.05f, 0.9f, 1.f, spread, true);
  const float mean9 = (0.1f * 4 + 0.3f * 4 + 0.2f) / 9.f;
  const float sigma9 = std::sqrt(((0.1f * 0.1f) * 4 + (0.3f * 0.3f) * 4 + 0.04f) / 9.f - mean9 * mean9);
  const float clamped = std::fmin(0.8f, mean9 + sigma9);
  CHECK(Near(r.ao, 0.2f + 0.9f * (clamped - 0.2f), 1e-4f), "clamp k=1 limits history to mean+sigma: %.4f", r.ao);
  const float with_neutral[9] = {-1.f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f};
  r = TemporalBlend(0.2f, good, 5.f, 0.9f, 0.05f, 0.9f, 1.f, with_neutral, true);
  // Eight neighbours at 0.2 (sigma 0): the clamp pins the history to 0.2, so the blend returns 0.2.
  CHECK(Near(r.ao, 0.2f, 1e-5f), "neutral neighbours are ignored by the clamp: %.4f", r.ao);
  // Bounded sweep: the result lies between raw and the clamped history, and alpha stays in [0, history weight].
  unsigned seed = 12345u;
  auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return (seed >> 8) * (1.f / 16777216.f); };
  bool bounded = true;
  for (int k = 0; k < 2000; ++k) {
    Tap taps[4];
    for (auto& t : taps) t = {0.25f, next() < 0.8f, next(), 5.f * (0.9f + 0.2f * next()), next()};
    float nb[9];
    for (auto& v : nb) v = next() < 0.1f ? -1.f : next();
    const float raw = next();
    const BlendResult b = TemporalBlend(raw, taps, 5.f, 0.9f, 0.05f, 0.5f, 1.f, nb, true);
    const float lo = std::fmin(raw, 1.f), hi = std::fmax(raw, 1.f);
    bounded = bounded && b.ao >= std::fmin(lo, 0.f) - 1e-5f && b.ao <= hi + 1e-5f && b.valid_fraction >= 0.f && b.valid_fraction <= 1.f;
  }
  CHECK(bounded, "blend stays within [0, 1] for 2000 random inputs");
  std::printf("temporal blend: 9 explicit cases and a 2000-input bounded sweep (transcription)\n");
}

// ---- Parameter snapshot (rtao_state.hpp): equality decides the reset rule (R2-S4) ----
static void TestParameterSnapshot() {
  const rtao::ParameterSnapshot base = {1.f, 2.f, 1.f, 2.f, 0.05f, 1.f, 1.f, 40.f, 80.f, 1.f, 0.9f, 0.05f, 0.9f, 1.f, 0.f};
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
  changed = base; changed.history_clamp = 2.f; CHECK(!(changed == base), "history clamp change is seen");
  changed = base; changed.debug = 1.f; CHECK(!(changed == base), "debug mode change is seen");
  CHECK(std::string(rtao::ReasonName(rtao::Reason::NoMotion)) == "no_motion", "no_motion name");
  CHECK(std::string(rtao::ReasonName(rtao::Reason::HistoryReset)) == "history_reset", "history_reset name");
  std::printf("parameter snapshot: 16 field checks\n");
}

// ---- Two-Sided discovery index (rtao_state.hpp TwoSidedDiscoveryIndex; shader: back*4 + toward*2 + alpha) ----
static void TestTwoSidedIndex() {
  CHECK(rtao::kRtaoStatsCount == 71u && rtao::kRtaoStatDiscoveryBase == 16u && rtao::kRtaoStatDiscoveryCount == 8u, "discovery stats layout");
  CHECK(rtao::kRtaoStatTemporalFBase == 24u && rtao::kRtaoStatTemporalFCount == 16u, "pass B diagnostics layout");
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

// ---- D0: traced-pixel denominators (rtao_state.hpp ComputeTracedDenominators, SharePercent): real code ----
static void TestTracedDenominators() {
  rtao::TracedDenominators none = rtao::ComputeTracedDenominators(100u, 0u, 0u, 0u, 0u, 7u);
  CHECK(none.traced == 100u && none.taps == 400u && none.reset_traced == 7u, "no neutral pixels: traced 100, taps 400, reset 7");
  rtao::TracedDenominators all = rtao::ComputeTracedDenominators(50u, 50u, 0u, 0u, 0u, 50u);
  CHECK(all.traced == 0u && all.taps == 0u && all.reset_traced == 0u, "all neutral: traced 0, nothing to divide by");
  CHECK(rtao::SharePercent(3u, all.traced) == 0.f && rtao::SharePercent(3u, all.taps) == 0.f, "share with zero denominator is 0");
  rtao::TracedDenominators mixed = rtao::ComputeTracedDenominators(100u, 10u, 5u, 3u, 2u, 30u);
  CHECK(mixed.traced == 80u && mixed.taps == 320u && mixed.reset_traced == 10u, "mixed: traced 80, taps 320, reset 30 - 20 neutral = 10");
  CHECK(rtao::SharePercent(20u, mixed.traced) == 25.f, "share 20 of 80 is 25 percent");
  rtao::TracedDenominators under = rtao::ComputeTracedDenominators(5u, 10u, 0u, 0u, 0u, 3u);
  CHECK(under.traced == 0u && under.reset_traced == 0u, "neutral above pixels: no underflow");
  std::printf("traced denominators: 5 cases (real code)\n");
}

// ---- F1 diagnostics: index table, first changed parameter, reason count ----
static void TestDiagnosticsLayout() {
  static const char* const expected[16] = {"moving_pixels", "motion_max", "motion_sum", "clamp_active",
                                           "clamp_shift_sum", "alpha_sum", "abs_raw_ao_sum", "no_history", "texel_differs",
                                           "matrix_lt_0.1", "matrix_lt_0.25", "matrix_lt_0.5", "matrix_lt_1", "matrix_ge_1", "matrix_diff_sum", "matrix_diff_max"};
  for (uint32_t i = 0; i < 16u; ++i) {
    CHECK(std::string(rtao::kTemporalDiagnosticNames[i]) == expected[i], "diagnostic name %u is %s", i, rtao::kTemporalDiagnosticNames[i]);
  }
  rtao::ParameterSnapshot a = {1.f, 2.f, 1.f, 2.f, 0.05f, 1.f, 1.f, 40.f, 80.f, 1.f, 0.9f, 0.05f, 0.9f, 1.f, 0.f};
  rtao::ParameterSnapshot b = a;
  CHECK(std::string(rtao::ParameterFieldName(a, b)).empty(), "equal snapshots: no field");
  b.radius = 1.5f; CHECK(std::string(rtao::ParameterFieldName(a, b)) == "radius", "radius named");
  b = a; b.history_clamp = 2.f; CHECK(std::string(rtao::ParameterFieldName(a, b)) == "history_clamp", "history clamp named");
  b = a; b.debug = 1.f; CHECK(std::string(rtao::ParameterFieldName(a, b)) == "debug", "debug named");
  b = a; b.radius = 1.5f; b.debug = 1.f; CHECK(std::string(rtao::ParameterFieldName(a, b)) == "radius", "first changed field wins");
  CHECK(static_cast<size_t>(rtao::Reason::ScaledResolution) < rtao::kReasonCount, "reason histogram covers every code");
  std::printf("diagnostics layout: 8 names, 5 parameter cases, reason count\n");
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

// ---- Two-pass dispatch (rtao.hpp Dispatch): pass B binds, the nulls after both passes, the history swap ----
static void TestTemporalDispatch() {
  Device dev;
  CmdList cl;
  cl.dev = &dev;
  rtao::RtaoDispatchInputs in = {};
  in.width = 64u;
  in.height = 32u;
  in.depth_view = {0x9000u};
  in.mrt_normal_view = {0x9200u};
  in.isfast_view = {0x9300u};
  in.scene_cbv_view = {0x9100u};
  in.motion_view = {0x9400u};

  // Temporal off: pass A only, no raw or history objects, no samplers.
  const rtao::RtaoDispatchResult off = rtao::Dispatch(&dev, &cl, in);
  rtao::RtaoDeviceData& data = rtao::GetRtaoDeviceData(&dev);
  CHECK(off.ok && !off.temporal_ran, "temporal off: pass A only");
  CHECK(data.raw_texture[0].handle == 0u && data.raw_texture[1].handle == 0u && data.history_texture[0].handle == 0u && data.history_texture[1].handle == 0u,
        "temporal off: no raw or history objects");
  CHECK(cl.dispatches == 1 && dev.sampler_creates == 0, "temporal off: one dispatch, no samplers (%d dispatches)", cl.dispatches);

  // Temporal on with motion: pass A then pass B; the history index moves to 1 after both.
  in.temporal = true;
  const rtao::RtaoDispatchResult on = rtao::Dispatch(&dev, &cl, in);
  CHECK(on.ok && on.temporal_ran, "temporal on: both passes ran");
  CHECK(cl.dispatches == 3 && data.history_index == 1u, "two dispatches more, history index 1 (%d dispatches, index %u)", cl.dispatches, data.history_index);
  CHECK(data.raw_index == 1u, "raw index flips with the history after both passes");
  CHECK(dev.sampler_creates == 2, "samplers created once (%d)", dev.sampler_creates);
  CHECK(cl.snaps.size() == 3u, "snapshots per pass (%zu)", cl.snaps.size());
  const auto& pass_a = cl.snaps[1];
  const auto& pass_b = cl.snaps[2];
  CHECK(pass_a.uav[2].handle == data.raw_uav[0].handle && data.raw_uav[0].handle != 0u, "pass A binds the raw AO UAV at u2");
  CHECK(pass_b.srv[0].handle == data.raw_srv[0].handle && data.raw_srv[0].handle != 0u, "pass B reads the raw AO at t0");
  CHECK(pass_b.srv[5].handle == data.raw_srv[1].handle && data.raw_srv[1].handle != 0u, "pass B reads the previous raw AO at t5");
  CHECK(pass_b.srv[1].handle == 0x9000u, "pass B depth at t1");
  CHECK(pass_b.srv[3].handle == 0x9400u, "pass B motion at t3");
  CHECK(pass_b.srv[4].handle == data.history_srv[0].handle && data.history_srv[0].handle != 0u, "pass B reads history 0 at t4");
  CHECK(pass_b.uav[0].handle == data.ao_uav.handle && pass_b.uav[1].handle == data.stats_uav.handle, "pass B writes AO and stats");
  CHECK(pass_b.uav[2].handle == data.history_uav[1].handle && data.history_uav[1].handle != 0u, "pass B writes history 1 at u2");
  CHECK(pass_b.smp[0].handle != 0u && pass_b.smp[1].handle != 0u, "pass B binds both samplers");
  int left_srv = 0, left_uav = 0, left_smp = 0;
  for (uint32_t i = 0; i < rtao::kRtaoSrvCount; ++i) left_srv += cl.cs_srv[i].handle != 0u ? 1 : 0;
  for (uint32_t i = 0; i < rtao::kRtaoUavCount; ++i) left_uav += cl.cs_uav[i].handle != 0u ? 1 : 0;
  for (uint32_t i = 0; i < rtao::kRtaoTemporalSamplerCount; ++i) left_smp += cl.cs_sampler[i].handle != 0u ? 1 : 0;
  CHECK(left_srv == 0 && left_uav == 0 && left_smp == 0, "after both passes: %d SRV, %d UAV, %d sampler slots still bound", left_srv, left_uav, left_smp);

  // Second run: reads history 1 and writes history 0; the index goes back to 0.
  const rtao::RtaoDispatchResult again = rtao::Dispatch(&dev, &cl, in);
  CHECK(again.temporal_ran && data.history_index == 0u, "second run swaps back to index 0");
  CHECK(data.raw_index == 0u, "second run flips the raw index back");
  const auto& pass_b2 = cl.snaps.back();
  CHECK(pass_b2.srv[4].handle == data.history_srv[1].handle, "second run reads history 1");
  CHECK(pass_b2.uav[2].handle == data.history_uav[0].handle, "second run writes history 0");

  // No motion: pass B does not run and the history is not swapped.
  in.motion_view = {0u};
  const rtao::RtaoDispatchResult nomo = rtao::Dispatch(&dev, &cl, in);
  CHECK(nomo.ok && !nomo.temporal_ran && data.history_index == 0u, "no motion: pass B does not run, index unchanged");
  CHECK(data.raw_index == 0u, "no motion: the raw index does not flip");

  // Destroy: every RTAO object (AO, stats, raw, history) is released.
  rtao::DestroyRtaoDeviceData(&dev);
  CHECK(dev.res.empty(), "destroy releases every RTAO object (%zu left)", dev.res.size());
  CHECK(rtao::g_rtao_devices.find(&dev) == rtao::g_rtao_devices.end(), "destroy removes the per-device entry");
  std::printf("temporal dispatch: 2 passes, history swap, slot nulls, no-motion path, destroy\n");
}

// ---- D1 (transcriptions unless marked real): pass-through, noise sample, texel_differs layout, debug scales ----
// TRANSCRIPTION: the pass B blend (TemporalBlend above) with zero motion and frozen noise (constant raw, no clamp).
static void TestPassThrough() {
  const float raw = 0.3f;
  const float flat9[9] = {0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f};
  const float weights[2] = {0.5f, 0.9f};
  bool within = true;
  for (float hw : weights) {
    float ao = raw;
    for (int frame = 0; frame < 20; ++frame) {
      const bool no_history = frame == 0 || frame == 10;  // first frame, and a reset in the middle
      Tap taps[4];
      for (auto& t : taps) t = {0.25f, !no_history, ao, 5.f, 1.f};
      const BlendResult b = TemporalBlend(raw, taps, 5.f, hw, 0.05f, 0.9f, 0.f, flat9, !no_history);
      ao = b.ao;
      within = within && std::fabs(ao - raw) * 255.f <= 1.f;
    }
  }
  CHECK(within, "zero motion + frozen noise: output equals raw within 1 LSB over 20 frames (weights 0.5, 0.9; reset at 10)");
  std::printf("pass-through: 2 history weights x 20 frames (transcription)\n");
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

// REAL: the layout constants (the stat index is FBase + 8 = 32, and the name table).
static void TestTexelDiffersLayout() {
  CHECK(rtao::kRtaoStatTemporalFBase + 8u == 32u && rtao::kRtaoStatsCount == 71u, "texel_differs at index 32 of 33");
  CHECK(std::string(rtao::kTemporalDiagnosticNames[8]) == "texel_differs", "texel_differs name");
  std::printf("texel_differs layout (real constants)\n");
}

// TRANSCRIPTION: the debug value scales of modes 4 to 6 (world_rtao_temporal.cs_5_0.hlsl).
static void TestDebugScales() {
  const float motion_px[4] = {0.f, 2.f, 4.f, 8.f};
  const float want_motion[4] = {0.f, 0.5f, 1.f, 1.f};
  const float alpha[2] = {0.f, 0.9f};
  const float diff[4] = {0.f, 0.25f, 0.5f, 1.f};
  const float want_diff[4] = {0.f, 0.5f, 1.f, 1.f};
  bool ok = true;
  for (int i = 0; i < 4; ++i) {
    ok = ok && std::fabs(std::fmin(std::fmax(motion_px[i] / 4.f, 0.f), 1.f) - want_motion[i]) < 1e-6f;
    ok = ok && std::fabs(std::fmin(std::fmax(diff[i] * 2.f, 0.f), 1.f) - want_diff[i]) < 1e-6f;
  }
  for (int i = 0; i < 2; ++i) ok = ok && std::fabs(alpha[i] - alpha[i]) == 0.f;  // mode 5 writes alpha as it is
  CHECK(ok, "debug scales: motion saturate(px/4), alpha as is, diff saturate(diff*2)");
  std::printf("debug scales: 4 motion, 4 diff, 2 alpha values (transcription)\n");
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
  const std::string pass_b = ReadText("src/games/falcomengine-plus/world/shaders/world_rtao_temporal.cs_5_0.hlsl");
  const std::string pass_f = ReadText("src/games/falcomengine-plus/world/shaders/world_rtao_filter.cs_5_0.hlsl");
  CHECK(!pass_a.empty() && !pass_b.empty() && !pass_f.empty(), "RTAO shader sources readable (run refresh_world first)");
  std::vector<HlslDefine> defs = ParseDefines(pass_a);
  const std::vector<HlslDefine> defs_b = ParseDefines(pass_b);
  defs.insert(defs.end(), defs_b.begin(), defs_b.end());
  const std::vector<HlslDefine> defs_f = ParseDefines(pass_f);
  defs.insert(defs.end(), defs_f.begin(), defs_f.end());
  CHECK(CountDuplicates(defs) == 0, "no #define name appears twice in the two RTAO shaders");
  CHECK(CountDuplicates(ParseDefines("#define X_TEST 1u\n#define X_TEST 2u\n")) == 1, "guard self-test: a duplicate is found");
  struct Pair { const char* name; uint32_t expected; };
  static const Pair pairs[] = {
      {"RTAO_STAT_RAYS", rtao::kRtaoStatRays}, {"RTAO_STAT_HITS", rtao::kRtaoStatHits},
      {"RTAO_STAT_PIXELS", rtao::kRtaoStatPixels}, {"RTAO_STAT_AO_SUM", rtao::kRtaoStatAoSum},
      {"RTAO_STAT_SKY", rtao::kRtaoStatSky}, {"RTAO_STAT_NORMAL", rtao::kRtaoStatNormal},
      {"RTAO_STAT_REGION", rtao::kRtaoStatRegion}, {"RTAO_STAT_INVALID_REFS", rtao::kRtaoStatInvalidRefs},
      {"RTAO_STAT_STACK_OVERFLOW", rtao::kRtaoStatStackOverflow}, {"RTAO_STAT_SCALED", rtao::kRtaoStatScaled},
      {"RTAO_DISC_BASE", rtao::kRtaoStatDiscoveryBase}, {"RTAO_DISC_COUNT", rtao::kRtaoStatDiscoveryCount},
      {"RTAO_TSTAT_BASE", rtao::kRtaoStatTemporalBase},
      {"RTAO_TSTAT_PIXELS", 0}, {"RTAO_TSTAT_VALID_TAPS", 1}, {"RTAO_TSTAT_REJECTED_DEPTH", 2},
      {"RTAO_TSTAT_REJECTED_NORMAL", 3}, {"RTAO_TSTAT_OUT_OF_BOUNDS", 4}, {"RTAO_TSTAT_RESET_PIXELS", 5},
      {"RTAO_FSTAT_BASE", rtao::kRtaoStatTemporalFBase}, {"RTAO_FSTAT_COUNT", rtao::kRtaoStatTemporalFCount},
      {"RTAO_FSTAT_MOVING_PIXELS", 0}, {"RTAO_FSTAT_MOTION_MAX", 1}, {"RTAO_FSTAT_MOTION_SUM", 2},
      {"RTAO_FSTAT_CLAMP_ACTIVE", 3}, {"RTAO_FSTAT_CLAMP_SHIFT_SUM", 4}, {"RTAO_FSTAT_ALPHA_SUM", 5},
      {"RTAO_FSTAT_ABS_DIFF_SUM", 6}, {"RTAO_FSTAT_NO_HISTORY", 7}, {"RTAO_FSTAT_TEXEL_DIFFERS", 8},
      {"RTAO_FSTAT_MATRIX_DIFF_BIN", 9}, {"RTAO_FSTAT_MATRIX_DIFF_SUM", 14}, {"RTAO_FSTAT_MATRIX_DIFF_MAX", 15},
      {"RTAO_STAT_JITTER_X", rtao::kRtaoStatJitterX}, {"RTAO_STAT_JITTER_Y", rtao::kRtaoStatJitterY},
      {"RTAO_WSTAT_BASE", rtao::kRtaoStatWeightBase}, {"RTAO_WSTAT_COUNT", rtao::kRtaoStatWeightCount},
      {"RTAO_DSTAT_BASE", rtao::kRtaoStatDepthBase}, {"RTAO_DSTAT_COUNT", rtao::kRtaoStatDepthCount},
      {"RTAO_RSTAT_BASE", rtao::kRtaoStatRawBase}, {"RTAO_RSTAT_COUNT", rtao::kRtaoStatRawCount},
      {"RTAO_QSTAT_BASE", rtao::kRtaoStatQualityBase}, {"RTAO_QSTAT_COUNT", rtao::kRtaoStatQualityCount},
      {"RTAO_QSTAT_AO_SUM", 0}, {"RTAO_QSTAT_RAW_SUM", 1}, {"RTAO_QSTAT_ROUGH_PAIRS", 2},
      {"RTAO_QSTAT_ROUGH_RAW_SUM", 3}, {"RTAO_QSTAT_ROUGH_PREV_SUM", 4}, {"RTAO_QSTAT_CHANGE_PAIRS", 5},
      {"RTAO_QSTAT_CHANGE_SUM", 6},
      {"RTAO_NSTAT_BASE", rtao::kRtaoStatNormalBase}, {"RTAO_NSTAT_COUNT", rtao::kRtaoStatNormalCount},
      {"RTAO_FILTER_STAT_BASE", rtao::kRtaoStatFilterBase}, {"RTAO_FILTER_STAT_COUNT", rtao::kRtaoStatFilterCount},
      {"RTAO_FILTER_STAT_PIXELS", 0}, {"RTAO_FILTER_STAT_CHANGE_SUM", 1}, {"RTAO_FILTER_STAT_CHANGED_GT1LSB", 2},
  };
  for (const Pair& pr : pairs) {
    long value = -1;
    const bool found = FindDefine(defs, pr.name, &value);
    CHECK(found && value == static_cast<long>(pr.expected), "%s = %ld in the HLSL, %u in C++ (found %d)",
          pr.name, value, pr.expected, found ? 1 : 0);
  }
  // Every stat index the shaders name lies inside the buffer.
  long jitter_y = 0;
  FindDefine(defs, "RTAO_STAT_JITTER_Y", &jitter_y);
  CHECK(jitter_y < static_cast<long>(rtao::kRtaoStatsCount), "jitter index inside the stats buffer");
  std::printf("HLSL source guard: %zu defines, %zu index pairs (source text, not a shader compile)\n", defs.size(), sizeof(pairs) / sizeof(pairs[0]));
}

// ---- D1b (transcriptions marked; the rest real) ----
// TRANSCRIPTION: a static world point with a per-frame jitter. The game motion (texels) is (prev - cur) + jitterDiff,
// the motion lookup is cur + motion, the camera-matrix lookup is the previous position. diff_px must equal |jitterDiff|.
// The uv conversion (ndc to uv and back) is checked on random points.
static void TestMatrixDifference() {
  unsigned seed = 7u;
  auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return (seed >> 8) * (1.f / 16777216.f); };
  bool exact = true, roundtrip = true;
  const float base_px[2] = {20.3f, 31.7f};
  for (int f = 0; f < 200; ++f) {
    const float cur_j[2] = {next() - 0.5f, next() - 0.5f};
    const float prev_j[2] = {next() - 0.5f, next() - 0.5f};
    const float cur_px[2] = {base_px[0] + cur_j[0], base_px[1] + cur_j[1]};
    const float prev_px[2] = {base_px[0] + prev_j[0], base_px[1] + prev_j[1]};
    const float jitter_diff[2] = {cur_j[0] - prev_j[0], cur_j[1] - prev_j[1]};
    const float motion[2] = {(prev_px[0] - cur_px[0]) + jitter_diff[0], (prev_px[1] - cur_px[1]) + jitter_diff[1]};
    const float hist_motion[2] = {cur_px[0] + motion[0], cur_px[1] + motion[1]};
    const float d[2] = {hist_motion[0] - prev_px[0], hist_motion[1] - prev_px[1]};
    exact = exact && std::fabs(std::sqrt(d[0] * d[0] + d[1] * d[1]) - std::sqrt(jitter_diff[0] * jitter_diff[0] + jitter_diff[1] * jitter_diff[1])) < 1e-4f;
    const float ndc_x = next() * 2.f - 1.f, ndc_y = next() * 2.f - 1.f;
    const float uv_x = ndc_x * 0.5f + 0.5f, uv_y = 0.5f - ndc_y * 0.5f;
    roundtrip = roundtrip && std::fabs(uv_x * 2.f - 1.f - ndc_x) < 1e-5f && std::fabs(1.f - 2.f * uv_y - ndc_y) < 1e-5f;
  }
  CHECK(exact, "diff_px equals |jitterDiff| when the game motion includes the jitter (200 frames)");
  CHECK(roundtrip, "ndc to uv (x*0.5+0.5, 0.5-y*0.5) round-trips");
  std::printf("matrix difference: 200 frames, ndc round trip (transcription)\n");
}

// REAL: rtao::DiffBin bin edges, negative and NaN.
static void TestDiffBin() {
  CHECK(rtao::DiffBin(-1.f) == 0u && rtao::DiffBin(0.f) == 0u && rtao::DiffBin(0.05f) == 0u, "negative and small: bin 0");
  CHECK(rtao::DiffBin(0.1f) == 1u && rtao::DiffBin(0.24f) == 1u, "0.1 to 0.25: bin 1");
  CHECK(rtao::DiffBin(0.25f) == 2u && rtao::DiffBin(0.49f) == 2u, "0.25 to 0.5: bin 2");
  CHECK(rtao::DiffBin(0.5f) == 3u && rtao::DiffBin(0.99f) == 3u, "0.5 to 1: bin 3");
  CHECK(rtao::DiffBin(1.0f) == 4u && rtao::DiffBin(7.f) == 4u, "1 and above: bin 4");
  CHECK(rtao::DiffBin(std::nanf("")) == 4u, "NaN: bin 4, never bin 0");
  std::printf("diff bins: 11 values (real function)\n");
}

// REAL: rtao::CheckSceneMatrices on identity, a true inverse pair, and a perturbed pair.
static void TestMatrixSelfCheck() {
  const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  rtao::MatrixSelfCheck id = rtao::CheckSceneMatrices(I, I, I);
  CHECK(id.identity_err == 0.f && id.prev_diff == 0.f, "identity: no error");
  const float M[16] = {2, 0, 0, 1, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  const float Minv[16] = {0.5f, 0, 0, -0.5f, 0, 0.5f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  rtao::MatrixSelfCheck pair = rtao::CheckSceneMatrices(M, Minv, M);
  CHECK(pair.identity_err < 1e-6f && pair.prev_diff == 0.f, "true inverse pair: identity error %.2e", pair.identity_err);
  float bad[16];
  std::memcpy(bad, Minv, sizeof(bad));
  bad[1] += 0.1f;
  rtao::MatrixSelfCheck perturbed = rtao::CheckSceneMatrices(M, bad, M);
  CHECK(perturbed.identity_err > 0.05f, "perturbed inverse: identity error %.4f", perturbed.identity_err);
  float prev[16];
  std::memcpy(prev, M, sizeof(prev));
  prev[5] += 0.25f;
  rtao::MatrixSelfCheck moved = rtao::CheckSceneMatrices(M, Minv, prev);
  CHECK(std::fabs(moved.prev_diff - 0.25f) < 1e-6f, "previous matrix differs by 0.25: prev_diff %.4f", moved.prev_diff);
  std::printf("matrix self-check: identity, inverse pair, perturbed, moved previous (real)\n");
}

// REAL: names and the new test row in the snapshot.
static void TestD1bLayout() {
  CHECK(rtao::kRtaoStatJitterX == 40u && rtao::kRtaoStatJitterY == 41u, "jitter stats at 40 and 41");
  CHECK(rtao::kRtaoStatWeightBase == 42u && rtao::kRtaoStatWeightCount == 3u, "weighted tap stats at 42..44");
  CHECK(rtao::kRtaoStatDepthBase == 45u && rtao::kRtaoStatDepthCount == 7u, "depth-ratio stats at 45..51");
  CHECK(rtao::kRtaoStatRawBase == 52u && rtao::kRtaoStatRawCount == 3u, "frame-to-frame raw stats at 52..54");
  CHECK(rtao::kRtaoTemporalSrvCount == 6u, "pass B has six SRVs (t0..t5)");
  CHECK(std::string(rtao::kTemporalDiagnosticNames[9]) == "matrix_lt_0.1" && std::string(rtao::kTemporalDiagnosticNames[15]) == "matrix_diff_max", "matrix difference names 9..15");
  rtao::ParameterSnapshot a = {};
  rtao::ParameterSnapshot b = a;
  b.camera_matrix = 1.f;
  CHECK(std::string(rtao::ParameterFieldName(a, b)) == "test_camera_matrix", "camera matrix test row resets the history");
  std::printf("D1b layout (real constants and snapshot)\n");
}

// TRANSCRIPTION: debug mode 7 scale saturate(diff_px / 2).
static void TestDebugScale7() {
  const float d[4] = {0.f, 1.f, 2.f, 4.f};
  const float want[4] = {0.f, 0.5f, 1.f, 1.f};
  bool ok = true;
  for (int i = 0; i < 4; ++i) ok = ok && std::fabs(std::fmin(std::fmax(d[i] / 2.f, 0.f), 1.f) - want[i]) < 1e-6f;
  CHECK(ok, "debug mode 7: saturate(px / 2)");
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

// ---- P0-B: depth-ratio bin (REAL rtao::DepthRatioBin; the shader copy is a transcription of the same edges) ----
static void TestDepthRatioBin() {
  CHECK(rtao::DepthRatioBin(0.f, 0.f) == 0u && rtao::DepthRatioBin(-1.f, 0.01f) == 0u, "history distance 0 or negative: invalid bin");
  CHECK(rtao::DepthRatioBin(std::nanf(""), 0.01f) == 0u, "NaN history distance: invalid bin");
  CHECK(rtao::DepthRatioBin(5.f, 0.f) == 1u && rtao::DepthRatioBin(5.f, 0.0005f) == 1u, "below 0.1%: bin 1");
  CHECK(rtao::DepthRatioBin(5.f, 0.001f) == 2u && rtao::DepthRatioBin(5.f, 0.004f) == 2u, "0.1% to 0.5%: bin 2");
  CHECK(rtao::DepthRatioBin(5.f, 0.005f) == 3u && rtao::DepthRatioBin(5.f, 0.009f) == 3u, "0.5% to 1%: bin 3");
  CHECK(rtao::DepthRatioBin(5.f, 0.01f) == 4u && rtao::DepthRatioBin(5.f, 0.019f) == 4u, "1% to 2%: bin 4");
  CHECK(rtao::DepthRatioBin(5.f, 0.02f) == 5u && rtao::DepthRatioBin(5.f, 0.049f) == 5u, "2% to 5%: bin 5");
  CHECK(rtao::DepthRatioBin(5.f, 0.05f) == 6u && rtao::DepthRatioBin(5.f, 0.5f) == 6u, "5% and above: bin 6");
  CHECK(rtao::DepthRatioBin(5.f, std::nanf("")) == 6u, "NaN ratio: bin 6");
  std::printf("depth-ratio bins: 13 values (real function)\n");
}

// ---- P0-B2: frame-to-frame raw difference. TRANSCRIPTION of the statistic (world_rtao_temporal.cs_5_0.hlsl):
// sum |raw - previous raw| over pixels where both are traced and the history is valid. With two independent binary raws
// of probability p, E|X - Y| = 2p(1-p); identical raws give 0 (and every pair is below 0.01). ----
static void TestRawDifference() {
  unsigned seed = 2024u;
  auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return (seed >> 8) * (1.f / 16777216.f); };
  const double p = 0.217;
  double sum_independent = 0.0;
  const int n = 200000;
  for (int i = 0; i < n; ++i) {
    const int x = next() < static_cast<float>(p) ? 1 : 0;
    const int y = next() < static_cast<float>(p) ? 1 : 0;
    sum_independent += std::abs(x - y);
  }
  const double mean_independent = sum_independent / n;
  CHECK(std::fabs(mean_independent - 2.0 * p * (1.0 - p)) < 0.005, "independent binary raw: mean |diff| %.4f, expected 2p(1-p) = %.4f", mean_independent, 2.0 * p * (1.0 - p));
  double sum_identical = 0.0;
  int below = 0;
  for (int i = 0; i < n; ++i) {
    const int x = next() < static_cast<float>(p) ? 1 : 0;
    const double diff = std::abs(x - x);
    sum_identical += diff;
    below += diff < 0.01 ? 1 : 0;
  }
  CHECK(sum_identical == 0.0 && below == n, "identical raw: mean |diff| 0, every pair below 0.01");
  std::printf("raw difference: %d pairs, independent mean %.4f (transcription)\n", n, mean_independent);
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
    values[rtao::kRtaoStatTemporalBase] = 1000u;
    values[rtao::kRtaoStatTemporalFBase] = 7u;
    in.temporal_on = true;
    in.zero_motion = true;
    in.camera_matrix = true;
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
  static const char* const root_keys[] = {"schema", "generated_frame", "reason", "producing", "gpu_ms", "texture_bytes",
                                          "frames_without_ao", "fade", "stats", "temporal", "two_sided_discovery",
                                          "temporal_diagnostics", "test_modes", "note"};
  for (const char* key : root_keys) CHECK(Member(root, key) != nullptr, "%s: root key %s is top level", name, key);
  const JNode* schema = Member(root, "schema");
  CHECK(schema && schema->text == "2", "%s: schema is 2", name);
  const JNode* temporal = Member(root, "temporal");
  CHECK(temporal && temporal->kind == JNode::Kind::Object && !temporal->keys.empty() && temporal->keys.back() == "reset_counts",
        "%s: temporal is an object whose last key is reset_counts", name);
  for (const char* key : {"test_modes", "two_sided_discovery", "temporal_diagnostics", "note"}) {
    CHECK(temporal && Member(*temporal, key) == nullptr, "%s: %s is not inside temporal", name, key);
  }
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

// ---- Step B: output quality. TRANSCRIPTION: the roughness operator and the sums (same neighbours, validity and
// conditions as the HLSL, on small arrays). REAL: rtao::MeanFromSum, rtao::kQualityStatNames, the "quality" JSON key. ----
struct QualityField {
  int w = 0;
  int h = 0;
  std::vector<float> v;
  std::vector<bool> valid;
};
static QualityField MakeQualityField(int w, int h, const std::vector<float>& v) {
  QualityField f;
  f.w = w;
  f.h = h;
  f.v = v;
  f.valid.assign(v.size(), true);
  return f;
}
static float QualityRoughT(const QualityField& f, int x, int y, float centre, bool* defined) {
  const int dx[4] = {-1, 1, 0, 0};
  const int dy[4] = {0, 0, -1, 1};
  float sum = 0.f;
  float n = 0.f;
  for (int k = 0; k < 4; ++k) {
    const int qx = x + dx[k];
    const int qy = y + dy[k];
    if (qx < 0 || qy < 0 || qx >= f.w || qy >= f.h) continue;
    const size_t i = static_cast<size_t>(qy * f.w + qx);
    if (!f.valid[i]) continue;
    sum += f.v[i];
    n += 1.f;
  }
  *defined = n > 0.f;
  return *defined ? std::fabs(centre - sum / n) : 0.f;
}
static void TestOutputQuality() {
  bool defined = false;
  // Constant field: roughness 0 everywhere.
  const QualityField flat = MakeQualityField(4, 4, std::vector<float>(16, 0.5f));
  bool flat_ok = true;
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      const float r = QualityRoughT(flat, x, y, 0.5f, &defined);
      if (!defined || r != 0.f) flat_ok = false;
    }
  }
  CHECK(flat_ok, "output quality (transcription): a constant field has roughness 0");
  // Checkerboard 0 and 1 on 4x4: every interior pixel differs from all four neighbours, roughness 1.
  std::vector<float> cb(16, 0.f);
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) cb[static_cast<size_t>(y * 4 + x)] = ((x + y) & 1) ? 1.f : 0.f;
  }
  const QualityField board = MakeQualityField(4, 4, cb);
  bool board_ok = true;
  for (int y = 1; y <= 2; ++y) {
    for (int x = 1; x <= 2; ++x) {
      const float r = QualityRoughT(board, x, y, cb[static_cast<size_t>(y * 4 + x)], &defined);
      if (!defined || r != 1.f) board_ok = false;
    }
  }
  CHECK(board_ok, "output quality (transcription): checkerboard interior roughness is 1");
  // One outlier: 3x3 zeros with a 1 in the centre. Centre roughness 1; pixel (0,1) has neighbours 1, 0, 0 (right, up, down): 1/3.
  const QualityField outlier = MakeQualityField(3, 3, {0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f});
  const float r_centre = QualityRoughT(outlier, 1, 1, 1.f, &defined);
  CHECK(defined && r_centre == 1.f, "output quality (transcription): outlier centre roughness is 1 (got %f)", r_centre);
  const float r_side = QualityRoughT(outlier, 0, 1, 0.f, &defined);
  CHECK(defined && std::fabs(r_side - 1.f / 3.f) < 1e-6f, "output quality (transcription): pixel beside the outlier is 1/3 (got %f)", r_side);
  // Border: the corner of a 2x2 field uses only its right and down neighbours: |0.2 - (0.6 + 0.4) / 2| = 0.3.
  const QualityField corner = MakeQualityField(2, 2, {0.2f, 0.6f, 0.4f, 0.9f});
  const float r_corner = QualityRoughT(corner, 0, 0, 0.2f, &defined);
  CHECK(defined && std::fabs(r_corner - 0.3f) < 1e-6f, "output quality (transcription): corner uses only valid neighbours (got %f)", r_corner);
  // Undefined: no neighbour at all (1x1), and both neighbours invalid.
  const QualityField lone = MakeQualityField(1, 1, {0.5f});
  QualityRoughT(lone, 0, 0, 0.5f, &defined);
  CHECK(!defined, "output quality (transcription): a pixel with no neighbour is not defined");
  QualityField gap = MakeQualityField(3, 1, {0.f, 0.5f, 1.f});
  gap.valid[0] = false;
  gap.valid[2] = false;
  QualityRoughT(gap, 1, 0, 0.5f, &defined);
  CHECK(!defined, "output quality (transcription): a pixel with only invalid neighbours is not defined");
  // Sums on a 2-pixel frame (binary-exact values). px0: raw 0.75, ao 0.5, previous output 0.25 (valid).
  // px1: raw 0.25, ao 0.5, previous output invalid. Expected: ao 1000, raw 1000, change pairs 1, change sum 250, rough pairs 0.
  const QualityField raw_f = MakeQualityField(2, 1, {0.75f, 0.25f});
  QualityField prev_f = MakeQualityField(2, 1, {0.25f, 0.f});
  prev_f.valid[1] = false;
  const float ao_px[2] = {0.5f, 0.5f};
  uint32_t ao_sum = 0u, raw_sum = 0u, change_pairs = 0u, change_sum = 0u, rough_pairs = 0u;
  for (int i = 0; i < 2; ++i) {
    ao_sum += static_cast<uint32_t>(ao_px[i] * 1000.f);
    raw_sum += static_cast<uint32_t>(raw_f.v[i] * 1000.f);
    if (prev_f.valid[i]) {
      change_pairs += 1u;
      change_sum += static_cast<uint32_t>(std::fabs(ao_px[i] - prev_f.v[i]) * 1000.f);
      bool raw_def = false;
      bool prev_def = false;
      QualityRoughT(raw_f, i, 0, raw_f.v[i], &raw_def);
      QualityRoughT(prev_f, i, 0, prev_f.v[i], &prev_def);
      if (raw_def && prev_def) rough_pairs += 1u;
    }
  }
  CHECK(ao_sum == 1000u && raw_sum == 1000u && change_pairs == 1u && change_sum == 250u && rough_pairs == 0u,
        "output quality (transcription): sums ao %u raw %u change %u/%u rough %u", ao_sum, raw_sum, change_pairs, change_sum, rough_pairs);
  // Real: MeanFromSum (zero guard, x1000 scale).
  CHECK(rtao::MeanFromSum(0u, 0u) == 0.0, "MeanFromSum: count 0 gives 0");
  CHECK(rtao::MeanFromSum(3000u, 3u) == 1.0 && rtao::MeanFromSum(1000u, 4u) == 0.25, "MeanFromSum: sum x1000 over count");
  // Real: name table order.
  static const char* const names[7] = {"ao_sum", "raw_sum", "rough_pairs", "rough_raw_sum", "rough_prev_out_sum", "change_pairs", "change_sum"};
  bool names_ok = true;
  for (size_t i = 0; i < 7u; ++i) names_ok = names_ok && std::strcmp(rtao::kQualityStatNames[i], names[i]) == 0;
  CHECK(names_ok, "quality name table is in index order");
  // Real: the JSON "quality" key inside temporal_diagnostics, at depth 2.
  JNode root;
  const std::string quality_text = WorldJsonForState(1);
  JsonParser parser(quality_text);
  CHECK(parser.Parse(&root), "quality: world_rtao.json parses");
  const JNode* diag = Member(root, "temporal_diagnostics");
  const JNode* quality = diag ? Member(*diag, "quality") : nullptr;
  static const char* const quality_keys[] = {"mean_raw", "mean_output", "raw_minus_output", "roughness_raw",
                                             "roughness_previous_output", "roughness_ratio", "roughness_pixels",
                                             "output_change_mean", "output_change_pixels", "note"};
  bool keys_ok = quality && quality->kind == JNode::Kind::Object && quality->keys.size() == 10u;
  for (size_t i = 0; keys_ok && i < 10u; ++i) keys_ok = quality->keys[i] == quality_keys[i];
  CHECK(keys_ok, "quality: temporal_diagnostics.quality has the ten keys in order");
  std::printf("output quality: roughness and sums are transcriptions; MeanFromSum, names and the JSON key are real\n");
}

// ---- Step C: soft normal weight. TRANSCRIPTION: the ramp and the tap bookkeeping (same arithmetic as the HLSL, with
// the depth test kept hard). REAL: rtao::kNormalRampWidth, the layout and the guard pairs of the new stats. ----
static float NormalRampT(float dot, float threshold) {
  return std::fmin(std::fmax((dot - (threshold - rtao::kNormalRampWidth)) / rtao::kNormalRampWidth, 0.f), 1.f);
}
struct SoftBlend {
  float ao;
  float valid_fraction;
  float accepted;          // sum of w * wn
  float rejected_normal;   // sum of w * (1 - wn)
  uint32_t unweighted_rejected;  // taps with wn == 0
  uint32_t valid_taps;     // taps with wn > 0
};
// hard = true replaces the ramp with the old step function (dot >= T gives 1, otherwise 0).
static SoftBlend SoftTemporalBlend(float raw, const Tap taps[4], float dist_cur, float history_weight, float depth_rejection,
                                   float normal_rejection, bool hard) {
  float weight_total = 0.f, weight_accepted = 0.f, history_sum = 0.f, rejected_normal = 0.f;
  uint32_t unweighted_rejected = 0u, valid_taps = 0u;
  for (int i = 0; i < 4; ++i) {
    weight_total += taps[i].w;
    if (!taps[i].has_history) continue;
    const bool depth_ok = std::fabs(taps[i].dist - dist_cur) / std::fmax(dist_cur, 1e-4f) <= depth_rejection;
    if (!depth_ok) continue;
    const float wn = hard ? (taps[i].dot >= normal_rejection ? 1.f : 0.f) : NormalRampT(taps[i].dot, normal_rejection);
    if (wn == 0.f) unweighted_rejected += 1u;
    else valid_taps += 1u;
    rejected_normal += taps[i].w * (1.f - wn);
    weight_accepted += taps[i].w * wn;
    history_sum += taps[i].w * wn * taps[i].ao;
  }
  const float valid_fraction = (weight_total > 0.f && weight_accepted > 1e-4f)
      ? std::fmin(std::fmax(weight_accepted / weight_total, 0.f), 1.f) : 0.f;
  const float history_ao = valid_fraction > 0.f ? history_sum / weight_accepted : raw;
  const float alpha = history_weight * valid_fraction;
  return {raw + (history_ao - raw) * alpha, valid_fraction, weight_accepted, rejected_normal, unweighted_rejected, valid_taps};
}
static void TestNormalRamp() {
  // Real constants and layout.
  CHECK(rtao::kNormalRampWidth == 0.4f, "soft normal ramp width is 0.4");
  CHECK(rtao::kRtaoStatNormalBase == 62u && rtao::kRtaoStatNormalCount == 6u && rtao::kRtaoStatsCount == 71u,
        "normal-dot histogram at 62..67, stats count 71");
  CHECK(rtao::kRtaoStatNormalBase == rtao::kRtaoStatQualityBase + rtao::kRtaoStatQualityCount, "normal-dot histogram follows the quality block");
  // Ramp values at several thresholds (transcription).
  CHECK(std::fabs(NormalRampT(1.0f, 0.9f) - 1.f) < 1e-6f && std::fabs(NormalRampT(0.9f, 0.9f) - 1.f) < 1e-6f, "ramp T=0.9: full at 1.0 and 0.9");
  CHECK(std::fabs(NormalRampT(0.7f, 0.9f) - 0.5f) < 1e-5f, "ramp T=0.9: 0.5 at 0.7");
  CHECK(NormalRampT(0.5f, 0.9f) < 1e-5f && NormalRampT(0.3f, 0.9f) == 0.f, "ramp T=0.9: zero at 0.5 and below");
  CHECK(NormalRampT(0.5f, 0.5f) == 1.f && std::fabs(NormalRampT(0.3f, 0.5f) - 0.5f) < 1e-5f && NormalRampT(0.1f, 0.5f) < 1e-5f,
        "ramp T=0.5: 1 at 0.5, 0.5 at 0.3, 0 at 0.1");
  CHECK(NormalRampT(0.f, 0.f) == 1.f && std::fabs(NormalRampT(-0.2f, 0.f) - 0.5f) < 1e-5f && NormalRampT(-0.4f, 0.f) == 0.f,
        "ramp T=0: 1 at 0, 0.5 at -0.2, 0 at -0.4");
  bool monotonic = true;
  float previous = -1.f;
  for (int i = -20; i <= 20; ++i) {
    const float wn = NormalRampT(static_cast<float>(i) / 20.f, 0.9f);
    if (wn < previous) monotonic = false;
    previous = wn;
  }
  CHECK(monotonic, "ramp is monotonic in the dot product");
  // Tap bookkeeping, hand-computed: T 0.9, four taps with equal weight 0.25 and depth accepted.
  // Taps: dot 1.0 (wn 1, ao 0.8), dot 0.7 (wn 0.5, ao 0.4), dot 0.3 (wn 0, ao 0.2), dot 0.45 (wn 0, ao 0.6).
  // accepted = 0.25 + 0.125 = 0.375; history = 0.2 + 0.05 = 0.25; valid fraction 0.375; history ao = 0.25 / 0.375.
  // rejected by normal = 0.125 + 0.25 + 0.25 = 0.625; accepted + rejected = total weight (other 0%); unweighted rejected 2, valid taps 2.
  const Tap book[4] = {{0.25f, true, 0.8f, 5.f, 1.f}, {0.25f, true, 0.4f, 5.f, 0.7f}, {0.25f, true, 0.2f, 5.f, 0.3f}, {0.25f, true, 0.6f, 5.f, 0.45f}};
  const SoftBlend sb = SoftTemporalBlend(0.5f, book, 5.f, 0.9f, 0.1f, 0.9f, false);
  CHECK(std::fabs(sb.accepted - 0.375f) < 1e-5f && std::fabs(sb.valid_fraction - 0.375f) < 1e-5f,
        "bookkeeping: accepted and valid fraction 0.375 (got %f, %f)", sb.accepted, sb.valid_fraction);
  CHECK(std::fabs(sb.rejected_normal - 0.625f) < 1e-5f && std::fabs(sb.accepted + sb.rejected_normal - 1.f) < 1e-5f,
        "bookkeeping: rejected by normal 0.625, shares sum to 100%% (got %f)", sb.rejected_normal);
  CHECK(sb.unweighted_rejected == 2u && sb.valid_taps == 2u, "bookkeeping: 2 taps with wn 0, 2 valid taps (got %u, %u)", sb.unweighted_rejected, sb.valid_taps);
  const float expected_ao = 0.5f + (0.25f / 0.375f - 0.5f) * (0.9f * 0.375f);
  CHECK(std::fabs(sb.ao - expected_ao) < 1e-5f, "bookkeeping: blended AO %f (expected %f)", sb.ao, expected_ao);
  // Property: for one centre tap, the soft no-history share (wn == 0, dot <= T - 0.4) is never above the hard one (dot < T).
  std::mt19937 rng(1234u);
  std::uniform_real_distribution<float> dot_dist(-1.f, 1.f);
  bool property_ok = true;
  int soft_none = 0, hard_none = 0;
  for (int threshold_index = 0; threshold_index < 2; ++threshold_index) {
    const float threshold = threshold_index == 0 ? 0.5f : 0.9f;
    soft_none = 0;
    hard_none = 0;
    for (int i = 0; i < 10000; ++i) {
      const float d = dot_dist(rng);
      if (NormalRampT(d, threshold) == 0.f) soft_none += 1;
      if (d < threshold) hard_none += 1;
    }
    if (soft_none > hard_none) property_ok = false;
    std::printf("soft normal property T=%.1f: no-history soft %d, hard %d of 10000 (uniform dots)\n", threshold, soft_none, hard_none);
  }
  CHECK(property_ok, "property: soft no-history share is not above the hard share");
  // Regression: with the ramp replaced by the old step, the bookkeeping reproduces the old hard blend (no clamp).
  std::mt19937 rng2(777u);
  std::uniform_real_distribution<float> unit(0.f, 1.f);
  const float flat[9] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
  bool regression_ok = true;
  for (int trial = 0; trial < 500; ++trial) {
    Tap random_taps[4];
    for (int i = 0; i < 4; ++i) {
      random_taps[i] = {unit(rng2), unit(rng2) > 0.2f, unit(rng2), 5.f, unit(rng2) * 2.f - 1.f};
    }
    const float raw = unit(rng2);
    const float threshold = unit(rng2);
    const BlendResult old_result = TemporalBlend(raw, random_taps, 5.f, 0.9f, 0.1f, threshold, 0.f, flat, true);
    const SoftBlend hard_result = SoftTemporalBlend(raw, random_taps, 5.f, 0.9f, 0.1f, threshold, true);
    if (std::fabs(old_result.ao - hard_result.ao) > 1e-5f || std::fabs(old_result.valid_fraction - hard_result.valid_fraction) > 1e-6f) {
      regression_ok = false;
    }
  }
  CHECK(regression_ok, "regression: the step-function bookkeeping reproduces the old hard blend in 500 random cases");
  std::printf("normal ramp: ramp values, bookkeeping (hand-computed), property, regression (transcriptions); layout and guard real\n");
}

// ---- S1 spatial filter plan (REAL: rtao::MakeFilterPlan, constants, Reason, snapshot size) ----
static void TestSpatialPlan() {
  CHECK(rtao::kFilterPlaneSigma == 0.02f && rtao::kFilterNormalPower == 16.f && rtao::kFilterMinSigma == 0.5f && rtao::kFilterChangeScale == 8.f,
        "filter constants equal the spec (plane 0.02, normal power 16, min sigma 0.5, change scale 8)");
  CHECK(static_cast<size_t>(rtao::Reason::FilterFailed) < rtao::kReasonCount && std::strcmp(rtao::ReasonName(rtao::Reason::FilterFailed), "filter_failed") == 0,
        "FilterFailed: a reason code inside kReasonCount, named filter_failed");
  CHECK(sizeof(rtao::ParameterSnapshot) == 18u * sizeof(float), "ParameterSnapshot unchanged: 18 floats, no filter field (size %zu)", sizeof(rtao::ParameterSnapshot));
  // Explicit chains from the spec.
  const rtao::FilterPlan sep_high = rtao::MakeFilterPlan(0, 4, 2);
  CHECK(sep_high.pass_count == 4 && sep_high.passes[0].output == rtao::FilterSlot::F0 && sep_high.passes[1].output == rtao::FilterSlot::F1 &&
        sep_high.passes[2].output == rtao::FilterSlot::F0 && sep_high.passes[3].output == rtao::FilterSlot::B &&
        sep_high.passes[0].direction == rtao::FilterDirection::Horizontal && sep_high.passes[1].direction == rtao::FilterDirection::Vertical,
        "separable High: A->F0 (H), F0->F1 (V), F1->F0 (H), F0->B (V)");
  const rtao::FilterPlan atrous_high = rtao::MakeFilterPlan(1, 4, 2);
  CHECK(atrous_high.pass_count == 2 && atrous_high.passes[0].input == rtao::FilterSlot::A && atrous_high.passes[0].output == rtao::FilterSlot::F0 &&
        atrous_high.passes[1].output == rtao::FilterSlot::B && atrous_high.passes[0].direction == rtao::FilterDirection::Both,
        "a-trous High: A->F0, F0->B, both 2D");
  // Every type x radius 1..8 x quality 0..2.
  int bad_chain = 0, bad_taps = 0, bad_footprint = 0, bad_direction = 0, combinations = 0;
  for (int type = 0; type <= 1; ++type) {
    for (int radius = 1; radius <= 8; ++radius) {
      for (int quality = 0; quality <= 2; ++quality) {
        ++combinations;
        const rtao::FilterPlan plan = rtao::MakeFilterPlan(type, radius, quality);
        const bool separable = type == 0;
        const uint32_t iterations = quality == 2 ? 2u : 1u;
        const uint32_t expected_passes = separable ? 2u * iterations : iterations;
        if (plan.iterations != iterations || plan.pass_count != expected_passes) ++bad_chain;
        if (plan.kind != (separable ? rtao::FilterKind::Separable : rtao::FilterKind::ATrous)) ++bad_chain;
        if (plan.passes[0].input != rtao::FilterSlot::A || plan.passes[plan.pass_count - 1u].output != rtao::FilterSlot::B) ++bad_chain;
        const uint32_t r = static_cast<uint32_t>(radius);
        for (uint32_t i = 0; i < plan.pass_count; ++i) {
          const rtao::FilterPass& pass = plan.passes[i];
          if (pass.input == pass.output) ++bad_chain;  // never read and write one texture
          if (i > 0u && pass.input != plan.passes[i - 1u].output) ++bad_chain;
          const bool last = i + 1u == plan.pass_count;
          if (last ? pass.output != rtao::FilterSlot::B : pass.output == rtao::FilterSlot::B) ++bad_chain;
          if (separable) {
            if (pass.direction != (i % 2u == 0u ? rtao::FilterDirection::Horizontal : rtao::FilterDirection::Vertical)) ++bad_direction;
            const uint32_t step = quality == 0 ? 2u : 1u;
            if (pass.step != step || pass.taps != 2u * (r / step) + 1u) ++bad_taps;
            if (pass.footprint != (r / step) * step || pass.footprint > r) ++bad_footprint;
          } else {
            const uint32_t step = r / 2u > 1u ? r / 2u : 1u;
            if (pass.direction != rtao::FilterDirection::Both) ++bad_direction;
            if (pass.step != step || pass.taps != (quality == 0 ? 9u : 25u)) ++bad_taps;
            if (pass.footprint != 2u * step) ++bad_footprint;
            // Planner rule: +-R for even R, +-(R-1) for odd R (holds for R >= 3; R = 1 clamps the step to 1, so reach 2).
            if (r >= 2u && r % 2u == 0u && pass.footprint != r) ++bad_footprint;
            if (r >= 3u && r % 2u == 1u && pass.footprint != r - 1u) ++bad_footprint;
          }
        }
      }
    }
  }
  CHECK(bad_chain == 0, "filter chains: %d bad passes over %d type x radius x quality combinations", bad_chain, combinations);
  CHECK(bad_direction == 0, "separable passes alternate H, V; a-trous passes are 2D (%d bad)", bad_direction);
  CHECK(bad_taps == 0, "taps per pass: separable 2*floor(R/stride)+1, a-trous 9 (Low) or 25 (Med/High) (%d bad)", bad_taps);
  CHECK(bad_footprint == 0, "footprint: separable floor(R/stride)*stride, a-trous 2*max(1,floor(R/2)) (%d bad)", bad_footprint);
  std::printf("spatial filter plan: %d combinations (chain, slots, directions, taps, footprint), constants, reason, snapshot size (real)\n", combinations);
}

// ---- S2 spatial filter resources (REAL: lazy creation, size change, pipeline, destroy, on the mock device) ----
static void TestFilterResources() {
  CHECK(rtao::kRtaoStatsCount == 71u && rtao::kRtaoStatFilterBase == 68u && rtao::kRtaoStatFilterCount == 3u,
        "filter stats at 68..70, stats count 71");
  CHECK(rtao::kRtaoFilterPushCount == 12u && sizeof(rtao::RtaoFilterPushConstants) == 12u * sizeof(float), "filter b12 is 12 floats");
  Device dev;
  rtao::RtaoDeviceData& data = rtao::GetRtaoDeviceData(&dev);
  CHECK(dev.res.empty() && data.filter_texture[0].handle == 0u && data.filter_pipeline.handle == 0u,
        "spatial filter off: nothing created");
  CHECK(rtao::EnsureRtaoFilterTargets(&dev, &data, 64u, 32u), "filter targets created on first use");
  CHECK(data.filter_texture[0].handle != 0u && data.filter_texture[1].handle != 0u && data.filter_texture[2].handle != 0u,
        "F0, F1 and B exist");
  int textures = 0;
  for (const auto& entry : dev.res) textures += entry.second.desc.type == resource_type::texture_2d ? 1 : 0;
  CHECK(textures == 3, "three filter textures (%d)", textures);
  const int creates = dev.texture_creates;
  CHECK(rtao::EnsureRtaoFilterTargets(&dev, &data, 64u, 32u) && dev.texture_creates == creates, "same size: no new targets");
  CHECK(rtao::EnsureRtaoFilterTargets(&dev, &data, 32u, 16u) && dev.texture_creates == creates + 3, "size change replaces the three targets");
  textures = 0;
  for (const auto& entry : dev.res) textures += entry.second.desc.type == resource_type::texture_2d ? 1 : 0;
  CHECK(textures == 3, "after the size change: three filter textures (%d)", textures);
  CHECK(rtao::EnsureRtaoFilterPipeline(&dev, &data) && data.filter_pipeline.handle != 0u && !data.filter_failed, "filter pipeline created");
  rtao::DestroyRtaoDeviceData(&dev);
  CHECK(dev.res.empty(), "destroy leaves nothing (%zu left)", dev.res.size());
  std::printf("spatial filter resources: lazy targets, size change, pipeline, destroy (mock)\n");
}

// ---- S3 spatial filter dispatch (REAL: rtao::Dispatch on the mock; the mock records dispatches, null pushes, the
// slots bound at the last dispatch and the device objects). ----
static void SpatialFrame(Device* dev, CmdList* cl, rtao::RtaoDispatchInputs* in, bool spatial, int type, int quality, int debug_mode) {
  in->spatial = spatial;
  in->filter_type = type;
  in->filter_quality = quality;
  in->debug_mode = debug_mode;
  (void)dev;
  (void)cl;
}
static rtao::RtaoDispatchInputs SpatialInputs(CmdList* cl, Device* dev) {
  rtao::RtaoDispatchInputs in = {};
  cl->dev = dev;
  in.width = 64u;
  in.height = 32u;
  in.depth_view = {0x9000u};
  in.mrt_normal_view = {0x9200u};
  in.isfast_view = {0x9300u};
  in.scene_cbv_view = {0x9100u};
  return in;
}
static void TestSpatialDispatch() {
  // Filter passes per configuration: separable Low/Medium 2, High 4; a-trous Low/Medium 1, High 2.
  const int cases[6][3] = {{0, 0, 2}, {0, 1, 2}, {0, 2, 4}, {1, 0, 1}, {1, 1, 1}, {1, 2, 2}};
  for (const auto& c : cases) {
    Device dev;
    CmdList cl;
    rtao::RtaoDispatchInputs in = SpatialInputs(&cl, &dev);
    SpatialFrame(&dev, &cl, &in, true, c[0], c[1], 0);
    const rtao::RtaoDispatchResult r = rtao::Dispatch(&dev, &cl, in);
    rtao::RtaoDeviceData& data = rtao::GetRtaoDeviceData(&dev);
    const int expected = 1 + c[2];  // pass A + the filter passes
    CHECK(r.ok && data.filter_ran && cl.dispatches == expected, "filter type %d quality %d: %d dispatches, expected %d (ran %d)",
          c[0], c[1], cl.dispatches, expected, data.filter_ran ? 1 : 0);
    CHECK(cl.null_srv_pushes == cl.dispatches && cl.null_uav_pushes == cl.dispatches,
          "type %d quality %d: every dispatch is followed by the srv and uav nulls (%d, %d of %d)", c[0], c[1],
          cl.null_srv_pushes, cl.null_uav_pushes, cl.dispatches);
    int srv_left = 0, uav_left = 0;
    for (uint32_t i = 0; i < rtao::kRtaoSrvCount; ++i) srv_left += cl.cs_srv[i].handle != 0u ? 1 : 0;
    for (uint32_t i = 0; i < rtao::kRtaoUavCount; ++i) uav_left += cl.cs_uav[i].handle != 0u ? 1 : 0;
    CHECK(srv_left == 0 && uav_left == 0, "type %d quality %d: no slot left bound after the frame", c[0], c[1]);
    CHECK(rtao::OutputSrv(data).handle == data.filter_srv[2].handle && data.filter_srv[2].handle != 0u, "output view is B");
    CHECK(cl.snaps.back().uav[0].handle == data.filter_uav[2].handle, "type %d quality %d: the last pass writes B (u0)", c[0], c[1]);
    CHECK(cl.snaps.back().srv[4].handle == data.ao_srv.handle, "type %d quality %d: the last pass binds the unfiltered AO at t4", c[0], c[1]);
    CHECK(data.timer.open == -1 && data.filter_timer.open == -1, "main and filter timers closed after the frame (order not observable in the mock)");
  }
  // Debug modes 1..7 bypass the filter (one dispatch, the AO view); mode 8 runs it.
  for (int mode = 1; mode <= 7; ++mode) {
    Device dev;
    CmdList cl;
    rtao::RtaoDispatchInputs in = SpatialInputs(&cl, &dev);
    SpatialFrame(&dev, &cl, &in, true, 0, 1, mode);
    rtao::Dispatch(&dev, &cl, in);
    rtao::RtaoDeviceData& data = rtao::GetRtaoDeviceData(&dev);
    CHECK(cl.dispatches == 1 && !data.filter_ran && rtao::OutputSrv(data).handle == data.ao_srv.handle,
          "debug mode %d bypasses the filter (%d dispatches)", mode, cl.dispatches);
  }
  {
    Device dev;
    CmdList cl;
    rtao::RtaoDispatchInputs in = SpatialInputs(&cl, &dev);
    SpatialFrame(&dev, &cl, &in, true, 0, 1, 8);
    rtao::Dispatch(&dev, &cl, in);
    rtao::RtaoDeviceData& data = rtao::GetRtaoDeviceData(&dev);
    CHECK(cl.dispatches == 3 && data.filter_ran, "debug mode 8 runs the filter (%d dispatches)", cl.dispatches);
  }
  // Spatial Filter off: no filter dispatch, no filter objects, the AO view; mode 8 behaves like mode 0.
  {
    Device dev;
    CmdList cl;
    rtao::RtaoDispatchInputs in = SpatialInputs(&cl, &dev);
    SpatialFrame(&dev, &cl, &in, false, 0, 1, 8);
    const rtao::RtaoDispatchResult r = rtao::Dispatch(&dev, &cl, in);
    rtao::RtaoDeviceData& data = rtao::GetRtaoDeviceData(&dev);
    CHECK(r.ok && cl.dispatches == 1 && cl.null_srv_pushes == 1 && cl.null_uav_pushes == 1,
          "spatial off: one dispatch with its nulls (%d dispatches)", cl.dispatches);
    CHECK(data.filter_texture[0].handle == 0u && data.filter_texture[1].handle == 0u && data.filter_texture[2].handle == 0u
              && data.filter_pipeline.handle == 0u && data.filter_layout.handle == 0u && data.filter_cbv_table.handle == 0u
              && data.filter_srv_table.handle == 0u && data.filter_uav_table.handle == 0u && data.filter_width == 0u,
          "spatial off: no filter pipeline, layout, tables or targets created");
    CHECK(data.filter_timer.created == false && !data.filter_ran && rtao::OutputSrv(data).handle == data.ao_srv.handle,
          "spatial off: the timer is untouched and the output view is the AO");
    CHECK(dev.texture_creates == 1, "spatial off: only the AO texture was created (%d)", dev.texture_creates);
  }
  // Forced creation failure, then the off -> on retry: sticky while on; cleared once by off -> on.
  {
    Device dev;
    rtao::RtaoDeviceData* data_ptr = nullptr;
    auto frame = [&](bool spatial, bool fail) {
      CmdList cl;
      rtao::RtaoDispatchInputs in = SpatialInputs(&cl, &dev);
      SpatialFrame(&dev, &cl, &in, spatial, 0, 1, 0);
      dev.fail_texture_2d = fail;
      rtao::Dispatch(&dev, &cl, in);
      data_ptr = &rtao::GetRtaoDeviceData(&dev);
      return cl.dispatches;
    };
    frame(false, false);  // AO objects exist, filter off
    const int f1 = frame(true, true);
    CHECK(f1 == 1 && !data_ptr->filter_ran && data_ptr->filter_failed && rtao::OutputSrv(*data_ptr).handle == data_ptr->ao_srv.handle,
          "forced failure: AO only, filter_failed set, output is A (%d dispatches)", f1);
    const int f2 = frame(true, false);
    CHECK(f2 == 1 && !data_ptr->filter_ran && data_ptr->filter_failed, "failure stays sticky while Spatial Filter stays on");
    frame(false, false);
    CHECK(data_ptr->filter_failed, "off keeps the failure flag");
    const int f4 = frame(true, false);
    CHECK(f4 == 3 && data_ptr->filter_ran && !data_ptr->filter_failed,
          "off -> on clears the failure once: the filter runs again (%d dispatches)", f4);
  }
  std::printf("spatial dispatch: pass counts per type x quality, debug 1..7 bypass, mode 8, off path, failure and retry (mock)\n");
}

// ---- S3 filter math. TRANSCRIPTION of world_rtao_filter.cs_5_0.hlsl (edge stops, separable kernel, copy-through,
// rounding, stats). Labelled: these are not the GPU code. ----
struct FV3 { float x, y, z; };
static FV3 FSub(FV3 a, FV3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static float FDot(FV3 a, FV3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
struct FPix { FV3 P; FV3 N; float value; bool sky; };
static const FV3 kFilterCamera = {0.f, 0.f, -1000.f};
static float EdgeWeightT(const FPix& c, const FPix& t) {
  if (t.sky) return 0.f;
  const float dist_c = std::sqrt(FDot(FSub(c.P, kFilterCamera), FSub(c.P, kFilterCamera)));
  const float plane = FDot(c.N, FSub(t.P, c.P)) / (0.02f * dist_c);
  const float nd = std::fmin(std::fmax(FDot(c.N, t.N), 0.f), 1.f);
  return std::exp(-0.5f * plane * plane) * std::pow(nd, 16.f);
}
// Separable 1D pass around index c with stride 1: Gaussian in the offset, sigma = max(radius / 2, 0.5).
static float SeparableT(const std::vector<FPix>& px, int c, int radius) {
  const float sigma = std::fmax(radius * 0.5f, 0.5f);
  float sum_w = 0.f, sum_v = 0.f;
  for (int k = -radius; k <= radius; ++k) {
    const int idx = c + k;
    if (idx < 0 || idx >= static_cast<int>(px.size())) continue;
    const float d = static_cast<float>(k) / sigma;
    const float w = std::exp(-0.5f * d * d) * (k == 0 ? 1.f : EdgeWeightT(px[c], px[idx]));
    sum_w += w;
    sum_v += w * px[idx].value;
  }
  return sum_v / sum_w;
}
static FPix FlatPix(float x, float value) {
  FPix p;
  p.P = {x, 0.f, 0.f};
  p.N = {0.f, 0.f, 1.f};
  p.value = value;
  p.sky = false;
  return p;
}
static uint32_t QuantT(float v) {
  const float s = std::fmin(std::fmax(v, 0.f), 1.f);
  const uint32_t q = static_cast<uint32_t>(std::round(s * 255.f));
  return q < 255u ? q : 255u;
}
static void TestFilterMath() {
  std::vector<FPix> flat;
  for (int i = 0; i < 9; ++i) flat.push_back(FlatPix(static_cast<float>(i), 0.5f));
  CHECK(std::fabs(SeparableT(flat, 4, 2) - 0.5f) < 1e-6f, "filter math (transcription): flat region is unchanged");
  std::vector<FPix> step = flat;
  step[6].P.z = 100.f;  // depth step: the plane distance is 100 at view distance ~1000
  std::vector<FPix> step_hi = step;
  step_hi[6].value = 1.f;
  CHECK(std::fabs(SeparableT(step, 4, 2) - SeparableT(step_hi, 4, 2)) < 1e-4f,
        "filter math (transcription): a depth step isolates the far side (weight ~0)");
  std::vector<FPix> normal = flat;
  normal[5].N = {1.f, 0.f, 0.f};  // normal edge: dot 0 with the centre, weight 0
  std::vector<FPix> normal_hi = normal;
  normal_hi[5].value = 1.f;
  CHECK(std::fabs(SeparableT(normal, 4, 2) - SeparableT(normal_hi, 4, 2)) < 1e-6f,
        "filter math (transcription): a normal edge isolates the other side");
  std::vector<FPix> outlier = flat;
  outlier[4].value = 1.f;  // centre is an outlier on a flat neighbourhood: smoothed, not kept
  const float smoothed = SeparableT(outlier, 4, 2);
  CHECK(smoothed > 0.5f && smoothed < 1.f, "filter math (transcription): a single outlier is smoothed (%f)", smoothed);
  std::vector<FPix> hot = flat;
  hot[5].value = 1.f;  // neighbour outlier pulls the centre up, less than fully
  const float pulled = SeparableT(hot, 4, 2);
  CHECK(pulled > 0.5f && pulled < 1.f, "filter math (transcription): a neighbour outlier pulls the centre part way (%f)", pulled);
  std::vector<FPix> ones = flat;
  for (auto& p : ones) p.value = 1.f;
  CHECK(std::fabs(SeparableT(ones, 4, 3) - 1.f) < 1e-6f, "filter math (transcription): weights are normalised (flat 1 stays 1)");
  const float sigma = std::fmax(3 * 0.5f, 0.5f);
  CHECK(std::fabs(std::exp(-0.5f * (1.f / sigma) * (1.f / sigma)) - std::exp(-0.5f * (-1.f / sigma) * (-1.f / sigma))) < 1e-9f,
        "filter math (transcription): kernel weights are symmetric");
  // Tilted plane: neighbours on the same plane keep full weight.
  const float inv = 1.f / std::sqrt(1.25f);
  FPix tilt_c = {{0.f, 0.f, 0.f}, {-0.5f * inv, 0.f, inv}, 0.5f, false};
  FPix tilt_n = {{1.f, 0.f, 0.5f}, {-0.5f * inv, 0.f, inv}, 0.5f, false};
  CHECK(std::fabs(EdgeWeightT(tilt_c, tilt_n) - 1.f) < 1e-5f, "filter math (transcription): a tilted plane keeps full weight along it (%f)", EdgeWeightT(tilt_c, tilt_n));
  // Sky or invalid-normal tap: weight 0; sky centre: copied through.
  FPix sky_n = flat[5];
  sky_n.sky = true;
  CHECK(EdgeWeightT(flat[4], sky_n) == 0.f, "filter math (transcription): a sky tap has weight 0");
  // Rounding and the 255 cap.
  CHECK(QuantT(1.0f) == 255u && QuantT(1.7f) == 255u && QuantT(0.f) == 0u, "filter math (transcription): saturate and cap at 255");
  CHECK(QuantT(0.501f) == 128u && QuantT(0.499f) == 127u, "filter math (transcription): round to the nearest 1/255");
  // Stats: |out - AO| x1000 and the count above one LSB (1/255).
  const float change = std::fabs(0.5f - 0.25f);
  CHECK(static_cast<uint32_t>(change * 1000.f) == 250u && change > 1.f / 255.f, "filter math (transcription): change sum 250 and counted above 1 LSB");
  std::printf("filter math: transcriptions of the edge stops, separable kernel, rounding and stats (labelled)\n");
}

// Source guard: the pass A and pass B shaders are byte-identical to HEAD (recorded now; git diff empty for both).
static void TestPassShadersUnchanged() {
  const std::string pass_a = ReadText("src/games/falcomengine-plus/world/shaders/world_rtao.cs_5_0.hlsl");
  const std::string pass_b = ReadText("src/games/falcomengine-plus/world/shaders/world_rtao_temporal.cs_5_0.hlsl");
  auto fnv = [](const std::string& s) {
    uint64_t h = 1469598103934665603ull;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ull; }
    return h;
  };
  CHECK(fnv(pass_a) == 0x5e7d000039ab22cfull, "pass A harness copy equals the S3 record (repo file: git diff empty) (%016llx)", static_cast<unsigned long long>(fnv(pass_a)));
  CHECK(fnv(pass_b) == 0xba6fc5c95a80cf38ull, "pass B harness copy equals the S3 record (repo file: git diff empty) (%016llx)", static_cast<unsigned long long>(fnv(pass_b)));
}

// ---- S4 spatial_filter JSON (REAL: rtao::BuildRtaoJson; filter on/off and both types, the twelve keys in order) ----
static std::string SpatialJson(bool on, int type, int quality) {
  static rtao::RtaoFrameState f;
  f = rtao::RtaoFrameState{};
  static uint32_t values[rtao::kRtaoStatsCount];
  std::memset(values, 0, sizeof(values));
  const rtao::FilterPlan plan = rtao::MakeFilterPlan(type, 2, quality);
  f.filter_requested = on;
  f.filter_ran = on;
  f.filter_type = type;
  f.filter_radius = 2;
  f.filter_quality = quality;
  f.filter_iterations = plan.iterations;
  f.filter_passes = plan.pass_count;
  f.filter_taps = plan.passes[0].taps;
  f.filter_gpu_ms = 0.25f;
  values[rtao::kRtaoStatFilterBase] = 4u;
  values[rtao::kRtaoStatFilterBase + 1u] = 2000u;
  values[rtao::kRtaoStatFilterBase + 2u] = 1u;
  rtao::RtaoJsonInput in = {};
  in.frame = &f;
  in.values = values;
  in.generated_frame = 1u;
  return rtao::BuildRtaoJson(in);
}
static void TestSpatialJson() {
  struct Case { bool on; int type; int quality; int passes; int taps; };
  const Case cases[] = {{false, 0, 1, 2, 5}, {true, 0, 1, 2, 5}, {true, 1, 1, 1, 25}, {true, 0, 2, 4, 5}, {true, 1, 0, 1, 9}};
  for (const Case& c : cases) {
    const std::string text = SpatialJson(c.on, c.type, c.quality);
    JNode root;
    JsonParser parser(text);
    CHECK(parser.Parse(&root), "spatial json: parses (on %d, type %d, quality %d)", c.on ? 1 : 0, c.type, c.quality);
    const JNode* sf = Member(root, "spatial_filter");
    CHECK(sf && sf->kind == JNode::Kind::Object && sf->keys.size() == 12u, "spatial json: spatial_filter is top level with 12 keys");
    static const char* const keys[] = {"on", "type", "radius", "quality", "iterations", "passes", "taps_per_pass", "gpu_ms",
                                       "pixels", "mean_change", "changed_gt_1lsb_pct", "failed"};
    bool order_ok = sf != nullptr && sf->keys.size() == 12u;
    for (size_t i = 0; order_ok && i < 12u; ++i) order_ok = sf->keys[i] == keys[i];
    CHECK(order_ok, "spatial json: keys in order (on %d, type %d)", c.on ? 1 : 0, c.type);
    const JNode* on = sf ? Member(*sf, "on") : nullptr;
    CHECK(on && on->text == (c.on ? "true" : "false"), "spatial json: on = %s", c.on ? "true" : "false");
    if (c.on && sf) {
      CHECK(Member(*sf, "passes")->text == std::to_string(c.passes) && Member(*sf, "taps_per_pass")->text == std::to_string(c.taps),
            "spatial json: passes %d, taps per pass %d (type %d, quality %d)", c.passes, c.taps, c.type, c.quality);
      CHECK(Member(*sf, "mean_change")->text == "0.5" && Member(*sf, "changed_gt_1lsb_pct")->text == "25" && Member(*sf, "pixels")->text == "4",
            "spatial json: mean change 0.5 (MeanFromSum), 25%% above 1 LSB, 4 pixels");
      CHECK(Member(*sf, "failed")->text == "false", "spatial json: not failed");
    }
  }
  std::printf("spatial json: filter off and on for both types and three qualities, twelve keys in order (real builder, text parser)\n");
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
  HStage("temporal blend (transcription)");
  TestTemporalBlend();
  HStage("reset rule");
  TestResetRule();
  TestMotionCheck();
  TestTwoSidedIndex();
  TestDiagnosticsLayout();
  TestTracedDenominators();
  TestPassThrough();
  TestNoiseSample();
  TestTexelDiffersLayout();
  TestDebugScales();
  TestMatrixDifference();
  TestHlslSources();
  TestDebugShowField();
  TestDiffBin();
  TestMatrixSelfCheck();
  TestD1bLayout();
  TestDebugScale7();
  TestDepthRatioBin();
  TestRawDifference();
  TestWorldJson();
  TestOutputQuality();
  TestNormalRamp();
  TestSpatialPlan();
  TestFilterResources();
  TestSpatialDispatch();
  TestFilterMath();
  TestPassShadersUnchanged();
  TestSpatialJson();
  HStage("temporal dispatch (mock)");
  TestTemporalDispatch();
  HStage("device lifecycle (mock)");
  TestDeviceLifecycle();
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
