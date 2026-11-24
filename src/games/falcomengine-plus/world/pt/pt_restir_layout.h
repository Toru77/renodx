#ifndef FALCOM_WORLD_PT_RESTIR_LAYOUT_H_
#define FALCOM_WORLD_PT_RESTIR_LAYOUT_H_

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#endif

#include <Rtxdi/PT/ReSTIRPTParameters.h>
#include <Rtxdi/DI/ReSTIRDIParameters.h>
#include <Rtxdi/RtxdiParameters.h>

// No arrays: HLSL cbuffer packing puts each array element in its own 16-byte register, which breaks the C++ mirror.
struct PtRestirConstants {
  RTXDI_RuntimeParameters runtime;
  RTXDI_LightBufferParameters light_buffer;
  RTXDI_DIInitialSamplingParameters nee;
  RTXDI_PTParameters restir;
  uint32_t prev_light_count;
  uint32_t history_valid;
  uint32_t prev_deferred_count;
  uint32_t stable_light_remap;
  // Unified PT direct-light settings (M6). All scalars keep identical C++/HLSL packing; the two padding floats
  // align foliage_tint to a 16-byte register in both languages (HLSL would insert that padding implicitly).
  float sun_angular_radius;
  float lamp_radius;
  uint32_t isfast_light_jitter;
  float local_normal_bias, local_char_normal_bias, local_clearance;
  float sun_normal_bias, sun_char_normal_bias, sun_max_distance;
  uint32_t sun_casters_only;
  float local_clamp;
  uint32_t local_clamp_enabled;
  float char_lamp_intensity;
  float local_char_self_softening, sun_char_self_softening;
  float foliage_transmission, wind_transmission, wind_softening;
  float foliage_tint_padding0, foliage_tint_padding1;
#ifdef __cplusplus
  float foliage_tint[3];
#else
  float3 foliage_tint;
#endif
  float sun_darkening, sun_char_darkening;
  float pt_intensity, pt_char_intensity;
  uint32_t direct_enabled, tile_count, debug_view, sun_detail_view;
  uint32_t direct_padding;
  // Prepared-scene coverage AABB for the shading-time sun ray (rt12_rtxdi_transfer.hpp:1996-2000).
#ifdef __cplusplus
  float coverage_min[3];
#else
  float3 coverage_min;
#endif
  float coverage_size;
  uint32_t coverage_valid;
  uint32_t coverage_padding0, coverage_padding1, coverage_padding2;
};

#ifdef __cplusplus
static_assert(sizeof(PtRestirConstants) % 16 == 0);
static_assert(offsetof(PtRestirConstants, sun_angular_radius) % 16 == 0);
static_assert(offsetof(PtRestirConstants, foliage_tint) % 16 == 0);
static_assert(offsetof(PtRestirConstants, coverage_min) % 16 == 0);
inline constexpr uint32_t kPtRestirConstantsBytes = (sizeof(PtRestirConstants) + 255u) & ~255u;
#endif

#endif  // FALCOM_WORLD_PT_RESTIR_LAYOUT_H_
