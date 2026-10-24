// Native harness: RT sun cascades (R2, world/rtsun). The C++ boundary, weight and effective-count rules that mirror
// include/rt_sun_range.hlsli and the draw hooks' per-slice count. Pure functions, no device.
//
// REAL: falcom_world::rtsun::RtSunBoundary, RtSunWeight and RtSunEffectiveCascades (compiled from the repo header).
// NOT COVERED: the HLSL functions on the GPU (sora2nd shadow resolve), the census skip and DSV clear (draw_census.hpp),
//       the panel rows (world_bvh.hpp), the camera capture of cb_scene c50.w and c57.w.
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "src/games/falcomengine-plus/world/rtsun/rtsun_range.hpp"

namespace rtsun = falcom_world::rtsun;

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

int main() {
  HStage("boundary transcription");
  TestBoundary();
  HStage("weight transcription");
  TestWeight();
  HStage("effective count");
  TestEffective();
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
