#pragma once

// RT Local Shadows GPU resources: the trace pipeline (world_rtls), the stats buffer, the raw shadow texture (rgba16f at
// the trace size) and the debug texture (r8g8b8a8_unorm, created while RTLS Debug is 1 to 5). Created lazily from
// RunRtlsInline (rtls.hpp); no module mutex is held around any graphics call here.

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>

#include "../../../../utils/log.hpp"
#include "../rtgi/rtgi_resources.hpp"

namespace falcom_world::rtls {

// t0..t8 BVH, t9 depth, t10..t13 characters, t14..t16 alpha, t17 MRT0, t18 IS-FAST, t19 P2b records,
// t20 the lamps (dynamicLights_g of the lighting draw), t21 the tile lists (lightIndices_g).
inline constexpr uint32_t kRtlsSrvCount = 22u;
inline constexpr uint32_t kRtlsUavCount = 3u;            // u0 debug (rgba8, null unless RTLS Debug 1-5), u1 stats, u2 raw (rgba16f)
inline constexpr uint32_t kRtlsPushConstantCount = 28u;
inline constexpr uint32_t kRtlsStatsCount = 96u;
inline constexpr uint32_t kRtlsKindPoint = 1u;           // b12 c2.z bit
inline constexpr uint32_t kRtlsKindSpot = 2u;            // b12 c2.z bit
inline constexpr uint32_t kRtlsMaxPoints = 10u;          // Max Point Lights Per Pixel upper bound
inline constexpr uint32_t kRtlsMaxSpots = 8u;            // spot lamps traced per pixel (all contributing, capped)
inline constexpr uint32_t kRtlsPerLightCount = 32u;      // per-lamp counters for lamp index < 32
inline constexpr uint32_t kRtlsStatPixels = 0u;
inline constexpr uint32_t kRtlsStatTraced = 1u;
inline constexpr uint32_t kRtlsStatSky = 2u;
inline constexpr uint32_t kRtlsStatNormal = 3u;
inline constexpr uint32_t kRtlsStatRegion = 4u;
inline constexpr uint32_t kRtlsStatBeyondFade = 5u;
inline constexpr uint32_t kRtlsStatScaled = 6u;
inline constexpr uint32_t kRtlsStatCharacter = 7u;
inline constexpr uint32_t kRtlsStatLightsConsidered = 8u;
inline constexpr uint32_t kRtlsStatLightsTraced = 9u;
inline constexpr uint32_t kRtlsStatPointsOverBudget = 10u;
inline constexpr uint32_t kRtlsStatSpotsOverCap = 11u;
inline constexpr uint32_t kRtlsStatLightsBacklit = 12u;
inline constexpr uint32_t kRtlsStatRays = 13u;
inline constexpr uint32_t kRtlsStatRaysBlocked = 14u;
inline constexpr uint32_t kRtlsStatRaysNearLight = 15u;
inline constexpr uint32_t kRtlsStatRaysClipped = 16u;
inline constexpr uint32_t kRtlsStatInvalidRefs = 17u;
inline constexpr uint32_t kRtlsStatStackOverflow = 18u;
inline constexpr uint32_t kRtlsStatIndexOutOfRange = 19u;
inline constexpr uint32_t kRtlsStatCharacterTraced = 24u;       // character pixels traced (Receive on characters)
inline constexpr uint32_t kRtlsStatCharacterSelfBlocked = 25u;  // rays from character pixels blocked within 0.1 m
inline constexpr uint32_t kRtlsKindCharacters = 4u;             // b12 c2.z bit: trace character pixels
// 20..22: the RTGI temporal pass (M4b); 23..31: reserved (M3b: 24 character traced, 25 character self-blocked).
inline constexpr uint32_t kRtlsStatPerLightRays = 32u;   // + lamp index, 32 entries
inline constexpr uint32_t kRtlsStatPerLightNear = 64u;   // + lamp index, 32 entries

#if defined(__world_rtls_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtlsTraceShader = __world_rtls;
#else
inline constexpr std::span<const std::uint8_t> kRtlsTraceShader = {};
#endif

#if defined(__world_rtls_filter_EMBED_FILE)
inline constexpr std::span<const std::uint8_t> kRtlsFilterShader = __world_rtls_filter;
#else
inline constexpr std::span<const std::uint8_t> kRtlsFilterShader = {};
#endif

// b12 for the RTLS trace, matching cbuffer cb_rtls in world_rtls.cs_5_0.hlsl (seven float4 registers).
struct RtlsPushConstants {
  float params[4];    // c0: effective ray max (m), normal bias (m), samples per light, trace divisor (1 in M2)
  float sampling[4];  // c1: IS-FAST flag (1 loaded), slice base, IGN seed, debug mode
  float fade[4];      // c2: fade start_eff, fade end_eff, kind bits (kRtlsKind* as a float), P2b record count (w, uint bits)
  float region[4];    // c3: region min xyz, region size
  float size[4];      // c4: trace width, trace height, full width, full height
  float light[4];     // c5: light size (m), light clearance (m), max point lights, lamp count
  float extra[4];     // c6: character object count, character normal bias (m), 0, 0
};
static_assert(sizeof(RtlsPushConstants) == kRtlsPushConstantCount * sizeof(float), "b12 must be 28 floats");
static_assert(offsetof(RtlsPushConstants, fade) == 8 * sizeof(float), "b12 c2 offset");
static_assert(offsetof(RtlsPushConstants, light) == 20 * sizeof(float), "b12 c5 offset");
static_assert(offsetof(RtlsPushConstants, extra) == 24 * sizeof(float), "b12 c6 offset");

struct RtlsDeviceData {
  rtgi::RtgiPipeline trace_pipeline;
  reshade::api::resource stats_buffer = {0u};
  reshade::api::resource_view stats_uav = {0u};
  rtgi::RtgiTextureSet<1> raw;    // rgba16f, trace size: x point shadow, y spot shadow, z coverage (fade), a 1 traced / -1 not
  rtgi::RtgiTextureSet<1> debug;  // r8g8b8a8_unorm, full size; created while RTLS Debug is 1 to 8
  rtgi::RtgiDenoiseSet denoise;   // output: r8g8b8a8_unorm, full size, read by the lighting shader at t38
  bvh::GpuTimer timer;
};

inline std::unordered_map<reshade::api::device*, RtlsDeviceData> g_rtls_devices;

inline RtlsDeviceData& GetRtlsDeviceData(reshade::api::device* device) { return g_rtls_devices[device]; }

inline void DestroyRtlsDeviceData(reshade::api::device* device) {
  const auto found = g_rtls_devices.find(device);
  if (found == g_rtls_devices.end()) return;
  RtlsDeviceData& data = found->second;
  rtgi::DestroyRtgiTextureSet(device, &data.raw);
  rtgi::DestroyRtgiTextureSet(device, &data.debug);
  rtgi::DestroyRtgiDenoiseSet(device, &data.denoise);
  bvh::DestroyBuffer(device, &data.stats_uav, &data.stats_buffer);
  bvh::DestroyGpuTimer(&data.timer);
  rtgi::DestroyRtgiPipeline(device, data.trace_pipeline);
  g_rtls_devices.erase(found);
}

}  // namespace falcom_world::rtls
