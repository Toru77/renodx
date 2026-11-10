#ifndef FALCOM_WORLD_RTXDI_DEBUG_HLSLI_
#define FALCOM_WORLD_RTXDI_DEBUG_HLSLI_

#include "../rtxdi/rtxdi_di_layout.h"
#include "../rtxdi/rtxdi_debug_layout.h"
#include "../rtxdi/rtxdi_surface_layout.h"

float3 RtxdiDebugUnavailable(uint2 pixel)
{
  const bool bright = (((pixel.x >> 3u) ^ (pixel.y >> 3u)) & 1u) != 0u;
  return (bright ? float3(0.65, 0.0, 0.65) : float3(0.05, 0.05, 0.05)) * 0.05;
}

float3 RtxdiDebugStatusColor(uint status_word)
{
  const uint status = status_word & 0x07u;
  if (status == RTXDI_N1_STATUS_VISIBLE) return float3(0.1, 1.0, 0.1) * 0.05;
  if (status == RTXDI_N1_STATUS_BLOCKED) return float3(1.0, 0.1, 0.1) * 0.05;
  if (status == RTXDI_N1_STATUS_UNKNOWN) return float3(1.0, 0.8, 0.0) * 0.05;
  if (status == RTXDI_N1_STATUS_EMPTY) return float3(0.1, 0.25, 1.0) * 0.05;
  return float3(1.0, 0.0, 1.0) * 0.05;
}

float3 RtxdiDebugRadiance(float3 radiance)
{
  if (!all(isfinite(radiance)) || any(radiance < 0.0)) return float3(1.0, 0.0, 1.0) * 0.05;
  return radiance / (1.0 + radiance) * 0.05;
}

float3 RtxdiDebugLight(uint index, uint type)
{
  if (type != RTXDI_DI_LIGHT_TYPE_POINT && type != RTXDI_DI_LIGHT_TYPE_SPOT) {
    return float3(1.0, 0.0, 1.0) * 0.05;
  }
  const float hash_brightness = 0.35 + 0.65 * ((float)((index * 1664525u + 1013904223u) >> 24u) / 255.0);
  const float3 color = type == RTXDI_DI_LIGHT_TYPE_POINT ? float3(0.0, 0.8, 1.0) : float3(1.0, 0.4, 0.0);
  return color * hash_brightness * 0.05;
}

float3 RtxdiDebugDistance(float4 carrier, bool magnitude)
{
  const bool finite_flags = isfinite(carrier.g);
  const uint flags = finite_flags ? (uint)carrier.g : 0xffffffffu;
  const bool flags_valid = finite_flags && carrier.g == (float)flags
                           && (flags == 0u || flags == 1u || flags == 3u || flags == 5u || flags == 7u);
  const bool valid = carrier.a == 1.0 && carrier.b == 0.0 && carrier.r >= 0.0
                     && isfinite(carrier.r) && flags_valid && (flags & 1u) != 0u
                     && (((flags & 4u) != 0u) ? carrier.r == 65504.0 : carrier.r > 0.0 && carrier.r < 65504.0);
  if (!valid) return float3(1.0, 0.0, 1.0) * 0.05;
  if ((flags & 4u) != 0u) return float3(0.0, 0.4, 1.0) * 0.05;
  if (!magnitude) return float3(0.1, 1.0, 0.1) * 0.05;
  const float value = log2(1.0 + carrier.r) / log2(1.0 + 65504.0);
  return value.xxx * 0.05;
}

float3 RtxdiDebugSunReasonColor(uint status, uint reason)
{
  if (reason == RTXDI_SUN_REASON_NONE) {
    if (status == RTXDI_SUN_STATUS_VISIBLE) return float3(1.0, 1.0, 1.0) * 0.05;
    if (status == RTXDI_SUN_STATUS_BLOCKED) return 0.0;
    return float3(1.0, 0.8, 0.0) * 0.05;
  }
  if (reason == RTXDI_SUN_REASON_INVALID_SURFACE) return float3(1.0, 0.0, 1.0) * 0.05;
  if (reason == RTXDI_SUN_REASON_INVALID_SCENE) return float3(1.0, 0.4, 0.0) * 0.05;
  if (reason == RTXDI_SUN_REASON_INVALID_ALPHA) return float3(1.0, 0.1, 0.1) * 0.05;
  if (reason == RTXDI_SUN_REASON_COVERAGE_MISS) return float3(0.0, 0.4, 1.0) * 0.05;
  if (reason == RTXDI_SUN_REASON_TRUNCATED) return float3(1.0, 0.8, 0.0) * 0.05;
  if (reason == RTXDI_SUN_REASON_INVALID_RAY) return float3(0.6, 0.0, 1.0) * 0.05;
  return float3(1.0, 0.0, 1.0) * 0.05;
}

float3 RtxdiDebugSunDetailColor(uint4 carrier, uint detail_view)
{
  const uint status = carrier.x;
  const uint reason = carrier.y;
  const float distance = asfloat(carrier.z);
  if (status > RTXDI_SUN_STATUS_BLOCKED || reason > RTXDI_SUN_REASON_INVALID_RAY) {
    return float3(1.0, 0.0, 1.0) * 0.05;
  }
  if (detail_view == RTXDI_SUN_DETAIL_REASON) return RtxdiDebugSunReasonColor(status, reason);
  if (detail_view != RTXDI_SUN_DETAIL_HIT_DISTANCE) return RtxdiDebugSunReasonColor(status, reason);
  if (status == RTXDI_SUN_STATUS_UNKNOWN) return float3(1.0, 0.8, 0.0) * 0.05;
  if (reason != RTXDI_SUN_REASON_NONE) return RtxdiDebugSunReasonColor(status, reason);
  if (!isfinite(distance) || distance < 0.0) return float3(1.0, 0.0, 1.0) * 0.05;
  return (log2(1.0 + min(distance, 65504.0)) / log2(1.0 + 65504.0)).xxx * 0.05;
}

float4 RtxdiDebugPackDistance(uint status_word, float distance, uint flags)
{
  const uint status = status_word & 0x07u;
  const bool receiver_valid = (status_word & RTXDI_N1_STATUS_KNOWN_BIT) != 0u
                              && (status == RTXDI_N1_STATUS_VISIBLE || status == RTXDI_N1_STATUS_BLOCKED);
  const bool allowed_flags = flags == 0u || flags == 1u || flags == 3u || flags == 5u || flags == 7u;
  const bool class_matches = (((status_word & RTXDI_N1_STATUS_CHARACTER_BIT) != 0u) == ((flags & 2u) != 0u));
  const bool valid_distance = (flags & 1u) != 0u && isfinite(distance) && distance > 0.0
                              && (((flags & 4u) != 0u) ? distance == 65504.0 : distance < 65504.0);
  return receiver_valid && allowed_flags && class_matches && valid_distance
             ? float4(distance, (float)flags, 0.0, 1.0)
             : float4(0.0, 0.0, 0.0, 1.0);
}

float3 RtxdiDebugValueColor(uint view, uint2 pixel, bool status_bound, uint status_word,
                            bool raw_bound, float3 raw_radiance, bool tuple_bound, uint4 tuple,
                            bool filtered_bound, float4 filtered_radiance, bool distance_bound,
                            float4 distance_carrier, bool sun_bound, uint sun_status,
                            uint consumer_decision)
{
  if (view == RTXDI_DEBUG_VIEW_OFF) return 0.0;
  if (view == RTXDI_DEBUG_VIEW_LOCAL_STATUS) {
    return status_bound ? RtxdiDebugStatusColor(status_word) : RtxdiDebugUnavailable(pixel);
  }
  if (view == RTXDI_DEBUG_VIEW_RECEIVER_CLASS) {
    if (!status_bound) return RtxdiDebugUnavailable(pixel);
    const uint status = status_word & 0x07u;
    const bool known = (status_word & RTXDI_N1_STATUS_KNOWN_BIT) != 0u
                       && (status == RTXDI_N1_STATUS_VISIBLE || status == RTXDI_N1_STATUS_BLOCKED);
    if (!known) return RtxdiDebugStatusColor(status_word);
    return (status_word & RTXDI_N1_STATUS_CHARACTER_BIT) != 0u
               ? float3(1.0, 0.4, 0.0) * 0.05
               : float3(0.0, 0.8, 1.0) * 0.05;
  }
  if (view == RTXDI_DEBUG_VIEW_SELECTED_LIGHT) {
    if (!tuple_bound) return RtxdiDebugUnavailable(pixel);
    if ((tuple.z != RTXDI_DI_STATUS_VISIBLE && tuple.z != RTXDI_DI_STATUS_BLOCKED)
        || (tuple.y != RTXDI_DI_LIGHT_TYPE_POINT && tuple.y != RTXDI_DI_LIGHT_TYPE_SPOT)
        || !isfinite(asfloat(tuple.w)) || asfloat(tuple.w) <= 0.0) {
      return RtxdiDebugStatusColor(tuple.z == RTXDI_DI_STATUS_EMPTY ? RTXDI_N1_STATUS_EMPTY
                                   : tuple.z == RTXDI_DI_STATUS_UNKNOWN ? RTXDI_N1_STATUS_UNKNOWN
                                                                       : RTXDI_N1_STATUS_INVALID);
    }
    return RtxdiDebugLight(tuple.x, tuple.y);
  }
  if (view == RTXDI_DEBUG_VIEW_RAW_DIFFUSE) {
    if (!status_bound) return RtxdiDebugUnavailable(pixel);
    const uint status = status_word & 0x07u;
    if (status != RTXDI_N1_STATUS_VISIBLE && status != RTXDI_N1_STATUS_BLOCKED) return RtxdiDebugStatusColor(status_word);
    return raw_bound ? RtxdiDebugRadiance(raw_radiance) : RtxdiDebugUnavailable(pixel);
  }
  if (view == RTXDI_DEBUG_VIEW_FILTERED_DIFFUSE) {
    if (!filtered_bound || !status_bound) return RtxdiDebugUnavailable(pixel);
    const uint status = status_word & 0x07u;
    const float expected_class = (status_word & RTXDI_N1_STATUS_CHARACTER_BIT) != 0u ? 2.0 : 1.0;
    if ((status != RTXDI_N1_STATUS_VISIBLE && status != RTXDI_N1_STATUS_BLOCKED)
        || !isfinite(filtered_radiance.a) || filtered_radiance.a != expected_class) {
      return float3(1.0, 0.0, 1.0) * 0.05;
    }
    return RtxdiDebugRadiance(filtered_radiance.rgb);
  }
  if (view == RTXDI_DEBUG_VIEW_DISTANCE_VALIDITY || view == RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE) {
    if (!distance_bound || !status_bound) return RtxdiDebugUnavailable(pixel);
    const uint status = status_word & 0x07u;
    const bool status_known = (status_word & RTXDI_N1_STATUS_KNOWN_BIT) != 0u
                              && (status == RTXDI_N1_STATUS_VISIBLE || status == RTXDI_N1_STATUS_BLOCKED);
    if (!status_known) return RtxdiDebugStatusColor(status_word);
    const uint flags = isfinite(distance_carrier.g) ? (uint)distance_carrier.g : 0xffffffffu;
    const bool class_matches = (((status_word & RTXDI_N1_STATUS_CHARACTER_BIT) != 0u)
                                == ((flags & 2u) != 0u));
    if (!class_matches) return float3(1.0, 0.0, 1.0) * 0.05;
    return RtxdiDebugDistance(distance_carrier, view == RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE);
  }
  if (view == RTXDI_DEBUG_VIEW_CONSUMER_SELECTION) {
    if (consumer_decision == RTXDI_DEBUG_CONSUMER_FILTERED) return float3(0.1, 1.0, 0.1) * 0.05;
    if (consumer_decision == RTXDI_DEBUG_CONSUMER_RAW) return float3(0.0, 0.8, 1.0) * 0.05;
    if (consumer_decision == RTXDI_DEBUG_CONSUMER_BLOCKED) return float3(1.0, 0.1, 0.1) * 0.05;
    if (consumer_decision == RTXDI_DEBUG_CONSUMER_EMPTY) return float3(0.1, 0.25, 1.0) * 0.05;
    if (consumer_decision == RTXDI_DEBUG_CONSUMER_INVALID) return float3(1.0, 0.0, 1.0) * 0.05;
    if (consumer_decision == RTXDI_DEBUG_CONSUMER_FALLBACK) return float3(1.0, 0.8, 0.0) * 0.05;
    return RtxdiDebugUnavailable(pixel);
  }
  if (view == RTXDI_DEBUG_VIEW_SUN_VISIBILITY) {
    if (!sun_bound) return RtxdiDebugUnavailable(pixel);
    if ((sun_status & ~0xFFFFu) != 0u) return float3(1.0, 0.8, 0.0) * 0.05;
    if ((sun_status & 0xFFu) == RTXDI_SUN_STATUS_VISIBLE) return (((sun_status >> RTXDI_SUN_LAYER_SHIFT) & 0xFFu) != 0u ? float3(0.0, 1.0, 0.0) : float3(1.0, 1.0, 1.0)) * 0.05;
    if ((sun_status & 0xFFu) == RTXDI_SUN_STATUS_BLOCKED) return 0.0;
    return float3(1.0, 0.8, 0.0) * 0.05;
  }
  return RtxdiDebugUnavailable(pixel);
}

#endif  // FALCOM_WORLD_RTXDI_DEBUG_HLSLI_
