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
#include <array>
#include <cmath>
#include <cstdio>
#include <cstddef>
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
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "src/games/falcomengine-plus/world/rtao/rtao.hpp"


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
  CHECK(rtao::kRtaoStatsCount == 24u && rtao::kRtaoStatScaled == 9u && rtao::kRtaoStatTemporalBase == 10u, "stats count");
  CHECK(rtao::kRtaoSrvCount == 19u && rtao::kRtaoUavCount == 3u && rtao::kRtaoPushRegister == 12u, "table sizes");
  CHECK(sizeof(rtao::RtaoTemporalPushConstants) == 20u * sizeof(float), "pass B b12 size");
  CHECK(offsetof(rtao::RtaoTemporalPushConstants, texel) == 16u && offsetof(rtao::RtaoTemporalPushConstants, size) == 32u, "pass B offsets");
  CHECK(rtao::kRtaoTemporalSrvCount == 5u && rtao::kRtaoTemporalUavCount == 3u && rtao::kRtaoTemporalSamplerCount == 2u, "pass B table sizes");
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
  CHECK(data.raw_texture.handle == 0u && data.history_texture[0].handle == 0u && data.history_texture[1].handle == 0u,
        "temporal off: no raw or history objects");
  CHECK(cl.dispatches == 1 && dev.sampler_creates == 0, "temporal off: one dispatch, no samplers (%d dispatches)", cl.dispatches);

  // Temporal on with motion: pass A then pass B; the history index moves to 1 after both.
  in.temporal = true;
  const rtao::RtaoDispatchResult on = rtao::Dispatch(&dev, &cl, in);
  CHECK(on.ok && on.temporal_ran, "temporal on: both passes ran");
  CHECK(cl.dispatches == 3 && data.history_index == 1u, "two dispatches more, history index 1 (%d dispatches, index %u)", cl.dispatches, data.history_index);
  CHECK(dev.sampler_creates == 2, "samplers created once (%d)", dev.sampler_creates);
  CHECK(cl.snaps.size() == 3u, "snapshots per pass (%zu)", cl.snaps.size());
  const auto& pass_a = cl.snaps[1];
  const auto& pass_b = cl.snaps[2];
  CHECK(pass_a.uav[2].handle == data.raw_uav.handle && data.raw_uav.handle != 0u, "pass A binds the raw AO UAV at u2");
  CHECK(pass_b.srv[0].handle == data.raw_srv.handle && data.raw_srv.handle != 0u, "pass B reads the raw AO at t0");
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
  const auto& pass_b2 = cl.snaps.back();
  CHECK(pass_b2.srv[4].handle == data.history_srv[1].handle, "second run reads history 1");
  CHECK(pass_b2.uav[2].handle == data.history_uav[0].handle, "second run writes history 0");

  // No motion: pass B does not run and the history is not swapped.
  in.motion_view = {0u};
  const rtao::RtaoDispatchResult nomo = rtao::Dispatch(&dev, &cl, in);
  CHECK(nomo.ok && !nomo.temporal_ran && data.history_index == 0u, "no motion: pass B does not run, index unchanged");

  // Destroy: every RTAO object (AO, stats, raw, history) is released.
  rtao::DestroyRtaoDeviceData(&dev);
  CHECK(dev.res.empty(), "destroy releases every RTAO object (%zu left)", dev.res.size());
  CHECK(rtao::g_rtao_devices.find(&dev) == rtao::g_rtao_devices.end(), "destroy removes the per-device entry");
  std::printf("temporal dispatch: 2 passes, history swap, slot nulls, no-motion path, destroy\n");
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
  HStage("temporal dispatch (mock)");
  TestTemporalDispatch();
  HStage("device lifecycle (mock)");
  TestDeviceLifecycle();
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
