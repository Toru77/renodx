#ifndef FALCOM_WORLD_RTXDI_LIGHTING_CONSUMER_HLSLI_
#define FALCOM_WORLD_RTXDI_LIGHTING_CONSUMER_HLSLI_

#include "../rtxdi/rtxdi_di_layout.h"
#include "../rtxdi/rtxdi_surface_layout.h"

struct RtxdiLightingSelection
{
  uint index;
  uint type;
  uint status;
  float inverse_pdf;
  bool valid;
};

struct RtxdiSunLightingSelection
{
  bool known;
  float visibility;
};

RtxdiSunLightingSelection RtxdiResolveSunLightingSelection(bool bound, uint status)
{
  RtxdiSunLightingSelection result = (RtxdiSunLightingSelection)0;
  result.known = bound && (status == RTXDI_SUN_STATUS_VISIBLE || status == RTXDI_SUN_STATUS_BLOCKED);
  result.visibility = status == RTXDI_SUN_STATUS_VISIBLE ? 1.0 : 0.0;
  return result;
}

RtxdiLightingSelection RtxdiResolveLightingSelection(bool bound, uint4 tuple, uint light_count, LightIndexData tile)
{
  RtxdiLightingSelection result = (RtxdiLightingSelection)0;
  result.index = tuple.x;
  result.type = tuple.y;
  result.status = tuple.z;
  result.inverse_pdf = asfloat(tuple.w);
  if (!bound || (result.status != RTXDI_DI_STATUS_VISIBLE && result.status != RTXDI_DI_STATUS_BLOCKED)
      || (result.type != RTXDI_DI_LIGHT_TYPE_POINT && result.type != RTXDI_DI_LIGHT_TYPE_SPOT)
      || result.index >= light_count || !isfinite(result.inverse_pdf) || result.inverse_pdf <= 0.0)
  {
    return result;
  }

  bool in_point_list = false;
  bool in_spot_list = false;
  const uint point_count = min(tile.pointLightCount, 63u);
  const uint spot_count = min(tile.spotLightCount, 63u);
  for (uint point_i = 0u; point_i < point_count; ++point_i)
  {
    const int listed_index = tile.pointLightIndices[point_i];
    in_point_list = in_point_list || (listed_index >= 0 && (uint)listed_index == result.index);
  }
  for (uint spot_i = 0u; spot_i < spot_count; ++spot_i)
  {
    const int listed_index = tile.spotLightIndices[spot_i];
    in_spot_list = in_spot_list || (listed_index >= 0 && (uint)listed_index == result.index);
  }
  result.valid = result.type == RTXDI_DI_LIGHT_TYPE_POINT ? (in_point_list && !in_spot_list)
                                                           : (in_spot_list && !in_point_list);
  return result;
}

void RtxdiEffectiveLightingLoopCounts(RtxdiLightingSelection selection, uint original_point_count,
                                      uint original_spot_count, out uint point_count, out uint spot_count)
{
  point_count = original_point_count;
  spot_count = original_spot_count;
  if (selection.valid)
  {
    point_count = selection.status == RTXDI_DI_STATUS_VISIBLE && selection.type == RTXDI_DI_LIGHT_TYPE_POINT ? 1u : 0u;
    spot_count = selection.status == RTXDI_DI_STATUS_VISIBLE && selection.type == RTXDI_DI_LIGHT_TYPE_SPOT ? 1u : 0u;
  }
}

#endif
