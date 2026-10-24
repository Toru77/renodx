#pragma once

// RT Sun Shadows GPU resources: the trace pipeline (world_rtsun), the stats buffer, the raw sun term (rgba16f at the trace
// size), the denoise chain (shared rtgi; output full size) and the debug texture (r8g8b8a8_unorm, full size, created while
// RT Sun Debug is 1, 2 or 4 to 6). Created lazily from RunRtSunInline
// (rtsun.hpp); no module mutex is held around any graphics call here.

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>

#include "../rtgi/rtgi_resources.hpp"
#include "../rtls/rtls_resources.hpp"

namespace falcom_world::rtsun {

// t0..t8 BVH, t9 depth, t10..t13 characters, t14..t16 alpha, t17 MRT0, t18 IS-FAST, t19 P2b records (the dynamic instances).
inline constexpr uint32_t kRtSunSrvCount = 20u;
inline constexpr uint32_t kRtSunUavCount = 3u;            // u0 debug (rgba8, null unless RT Sun Debug 1-2), u1 stats, u2 raw (rgba16f)
inline constexpr uint32_t kRtSunPushConstantCount = 20u;
inline constexpr uint32_t kRtSunStatsCount = 32u;
inline constexpr uint32_t kRtSunStatPixels = 0u;
inline constexpr uint32_t kRtSunStatTraced = 1u;           // environment pixels inside the RT range and the BVH region
inline constexpr uint32_t kRtSunStatSky = 2u;
inline constexpr uint32_t kRtSunStatNormal = 3u;
inline constexpr uint32_t kRtSunStatRegion = 4u;           // outside the BVH region (shrunk by Max Ray Distance)
inline constexpr uint32_t kRtSunStatBeyondRange = 5u;      // beyond the RT cascade range (weight 0)
inline constexpr uint32_t kRtSunStatScaled = 6u;
inline constexpr uint32_t kRtSunStatCharacter = 7u;        // character pixels: skipped, not receivers in R3a
inline constexpr uint32_t kRtSunStatRays = 8u;
inline constexpr uint32_t kRtSunStatRaysBlocked = 9u;
inline constexpr uint32_t kRtSunStatCharacterRays = 10u;   // blocked rays whose reported hit is a character (dynamic instance flag)
inline constexpr uint32_t kRtSunStatSelfBlocked = 11u;     // blocked rays with the hit within 0.1 m of the origin
inline constexpr uint32_t kRtSunStatInvalidRefs = 12u;
inline constexpr uint32_t kRtSunStatStackOverflow = 13u;

#if defined(__world_rtsun_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtSunTraceShader = __world_rtsun;
#else
inline constexpr std::span<const std::uint8_t> kRtSunTraceShader = {};
#endif

// b12 for the RT sun trace, matching cbuffer cb_rtsun in world_rtsun.cs_5_0.hlsl (five float4 registers).
struct RtSunPushConstants {
  float params[4];    // c0: Max Ray Distance (m), normal bias (m), samples per pixel, effective RT cascades
  float sampling[4];  // c1: IS-FAST flag (1 loaded), slice base, IGN seed, debug mode
  float sun[4];       // c2: light size (degrees), character object count, P2b record count (z, uint bits), trace divisor (w)
  float region[4];    // c3: BVH region min xyz, region size
  float size[4];      // c4: full width, full height, region margin (m; kPoolRegionRayMargin), 0
};
static_assert(sizeof(RtSunPushConstants) == kRtSunPushConstantCount * sizeof(float), "b12 must be 20 floats");
static_assert(offsetof(RtSunPushConstants, sun) == 8 * sizeof(float), "b12 c2 offset");
static_assert(offsetof(RtSunPushConstants, region) == 12 * sizeof(float), "b12 c3 offset");
static_assert(offsetof(RtSunPushConstants, size) == 16 * sizeof(float), "b12 c4 offset");

struct RtSunDeviceData {
  rtgi::RtgiPipeline trace_pipeline;
  reshade::api::resource stats_buffer = {0u};
  reshade::api::resource_view stats_uav = {0u};
  rtgi::RtgiTextureSet<1> raw;    // rgba16f, trace size: x shadow x w, y 0, z w (RT cascade weight), a 1 traced / -1 not
  rtgi::RtgiDenoiseSet denoise;   // temporal, filter and upsample; output r8g8b8a8_unorm, full size (x = the denoised term)
  rtgi::RtgiTextureSet<1> debug;  // r8g8b8a8_unorm, full size; created while RT Sun Debug is 1, 2 or 4 to 6
  bvh::GpuTimer timer;
};

inline std::unordered_map<reshade::api::device*, RtSunDeviceData> g_rtsun_devices;

inline RtSunDeviceData& GetRtSunDeviceData(reshade::api::device* device) { return g_rtsun_devices[device]; }

inline void DestroyRtSunDeviceData(reshade::api::device* device) {
  const auto found = g_rtsun_devices.find(device);
  if (found == g_rtsun_devices.end()) return;
  RtSunDeviceData& data = found->second;
  rtgi::DestroyRtgiTextureSet(device, &data.raw);
  rtgi::DestroyRtgiTextureSet(device, &data.debug);
  rtgi::DestroyRtgiDenoiseSet(device, &data.denoise);
  bvh::DestroyBuffer(device, &data.stats_uav, &data.stats_buffer);
  bvh::DestroyGpuTimer(&data.timer);
  rtgi::DestroyRtgiPipeline(device, data.trace_pipeline);
  g_rtsun_devices.erase(found);
}

}  // namespace falcom_world::rtsun
