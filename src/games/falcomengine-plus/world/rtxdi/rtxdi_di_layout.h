#ifndef FALCOM_WORLD_RTXDI_DI_LAYOUT_H_
#define FALCOM_WORLD_RTXDI_DI_LAYOUT_H_

#define RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL 1u

#define RTXDI_DI_LIGHT_TYPE_POINT       1u
#define RTXDI_DI_LIGHT_TYPE_SPOT        2u
#define RTXDI_DI_STATUS_EMPTY           0u
#define RTXDI_DI_STATUS_VISIBLE         1u
#define RTXDI_DI_STATUS_BLOCKED         2u
#define RTXDI_DI_STATUS_UNKNOWN         3u
#define RTXDI_DI_REASON_NONE            0u
#define RTXDI_DI_REASON_INVALID_SURFACE 1u
#define RTXDI_DI_REASON_ZERO_LIGHTS     2u
#define RTXDI_DI_REASON_INVALID_SCENE   3u
#define RTXDI_DI_REASON_INVALID_SAMPLE  4u
#define RTXDI_DI_REASON_COVERAGE_MISS   5u
#define RTXDI_DI_REASON_INVALID_ALPHA   6u
#define RTXDI_DI_REASON_TRUNCATED       7u

#ifdef __cplusplus
#include <cstdint>
struct RtxdiDiRawResult {
  // Pre-material local diffuse; later shading applies base color and dynamic intensity once.
  float diffuse_radiance[3];
  uint32_t status;
  float distance;
  uint32_t reason;
  uint32_t flags;
  uint32_t padding;
  uint32_t selected_light_index;
  uint32_t selected_light_type;
  float inverse_pdf;
  uint32_t reserved;
};
static_assert(sizeof(RtxdiDiRawResult) == 48u);
#else
struct RtxdiDiRawResult {
  // Pre-material local diffuse; later shading applies base color and dynamic intensity once.
  float3 diffuse_radiance;
  uint status;
  float distance;
  uint reason;
  uint flags;
  uint padding;
  uint selected_light_index;
  uint selected_light_type;
  float inverse_pdf;
  uint reserved;
};
RtxdiDiRawResult RtxdiDiMakeVisibleRawResult(float3 diffuse_radiance, float distance, uint flags,
                                             uint light_index, uint light_type, float inverse_pdf) {
  RtxdiDiRawResult result = (RtxdiDiRawResult)0;
  result.selected_light_index = light_index;
  result.selected_light_type = light_type;
  result.inverse_pdf = inverse_pdf;
  if (!all(isfinite(diffuse_radiance)) || !isfinite(distance) || !isfinite(inverse_pdf) || inverse_pdf <= 0.0) {
    result.status = RTXDI_DI_STATUS_UNKNOWN;
    result.reason = RTXDI_DI_REASON_INVALID_SAMPLE;
    return result;
  }
  result.diffuse_radiance = diffuse_radiance;
  result.status = RTXDI_DI_STATUS_VISIBLE;
  result.distance = distance;
  result.flags = flags;
  return result;
}
#endif

#endif  // FALCOM_WORLD_RTXDI_DI_LAYOUT_H_
