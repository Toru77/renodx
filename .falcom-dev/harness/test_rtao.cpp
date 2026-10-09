// Native harness: RTAO round 1 (world/rtao). Covers the gate table (T x Rd x G x V), the distance-fade
// range rule, the per-pixel region margin, the b12 layout, the ray generation / falloff / fade rules
// against a brute-force reference on a wall scene (shader logic transcribed from world_rtao.cs_5_0.hlsl),
// the sky / invalid-normal / scaled classification, and the device lifecycle on the mock device (lazy
// creation, resize, null UAV after dispatch, destroy). The GPU trace itself is not run here.
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
  CHECK(rtao::kRtaoStatsCount == 10u && rtao::kRtaoStatScaled == 9u, "stats count");
  CHECK(rtao::kRtaoSrvCount == 19u && rtao::kRtaoUavCount == 2u && rtao::kRtaoPushRegister == 12u, "table sizes");
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
  uint64_t next = 0x1000;
  int texture_creates = 0;
  device_api get_api() const override { return device_api::d3d11; }
  void update_descriptor_tables(uint32_t count, const descriptor_table_update *updates) override {
    for (uint32_t i = 0; i < count; ++i) {
      if (updates[i].type == descriptor_type::unordered_access_view) {
        uav_of_table[updates[i].table.handle] = static_cast<const resource_view *>(updates[i].descriptors)[0].handle;
      }
    }
  }
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
  device *get_device() override { return dev; }
  void bind_pipeline(pipeline_stage, pipeline) override {}
  void bind_descriptor_tables(shader_stage, pipeline_layout, uint32_t, uint32_t, const descriptor_table *) override {}
  void push_constants(shader_stage, pipeline_layout, uint32_t, uint32_t, uint32_t, const void *) override {}
  void push_descriptors(shader_stage, pipeline_layout layout, uint32_t, const descriptor_table_update &update) override {
    if (layout.handle != 0u) return;
    const auto* views = static_cast<const resource_view *>(update.descriptors);
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
  void dispatch(uint32_t x, uint32_t y, uint32_t) override { dispatches += 1; groups_x = x; groups_y = y; }
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
  in.scene_cbv_view = {0x9100u};
  const bool dispatched = rtao::Dispatch(&dev, &cl, in);
  CHECK(dispatched && cl.dispatches == 1, "dispatch issued (ok %d, dispatches %d)", dispatched, cl.dispatches);
  CHECK(data.ao_width == 200u && data.ao_height == 100u, "AO target sized from the depth input (%u x %u)", data.ao_width, data.ao_height);
  CHECK(cl.groups_x == 25u && cl.groups_y == 13u, "8x8 groups over 200x100: %u x %u", cl.groups_x, cl.groups_y);
  CHECK(cl.null_srv_pushes == 1 && cl.null_srv_count == rtao::kRtaoSrvCount, "all 19 SRVs nulled on the context (pushes %d count %u)", cl.null_srv_pushes, cl.null_srv_count);
  CHECK(cl.null_uav_pushes == 1 && cl.null_uav_count == rtao::kRtaoUavCount && !cl.pushed_nonnull_uav, "u0 and u1 nulled on the context, no UAV left bound");

  rtao::DestroyRtaoDeviceData(&dev);
  CHECK(rtao::g_rtao_devices.find(&dev) == rtao::g_rtao_devices.end(), "RTAO data removed with the device");
  CHECK(dev.res.empty(), "all RTAO device objects destroyed (%zu left)", dev.res.size());
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
  HStage("device lifecycle (mock)");
  TestDeviceLifecycle();
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
