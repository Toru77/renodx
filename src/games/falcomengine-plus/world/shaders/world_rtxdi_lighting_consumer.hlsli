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
  uint layers;
};

struct RtxdiCharacterLightResult
{
  bool resolved;
  float3 diffuse;
};

struct RtxdiDenoisedDiffuseResult
{
  bool valid;
  float3 radiance;
};

RtxdiDenoisedDiffuseResult RtxdiResolveDenoisedDiffuse(bool bound, RtxdiLightingSelection selection,
                                                        float4 published, uint expected_class)
{
  RtxdiDenoisedDiffuseResult result = (RtxdiDenoisedDiffuseResult)0;
  result.valid = bound && selection.valid && (expected_class == 1u || expected_class == 2u)
                 && isfinite(published.a) && published.a == (float)expected_class
                 && all(isfinite(published.rgb)) && all(published.rgb >= 0.0) && all(published.rgb <= 65504.0);
  if (result.valid) result.radiance = published.rgb;
  return result;
}

RtxdiSunLightingSelection RtxdiResolveSunLightingSelection(bool bound, uint word, float transmission)
{
  RtxdiSunLightingSelection result = (RtxdiSunLightingSelection)0;
  const uint status = word & 0xFFu;
  result.known = bound && (word & ~0xFFFu) == 0u && (status == RTXDI_SUN_STATUS_VISIBLE || status == RTXDI_SUN_STATUS_BLOCKED);
  result.layers = (word >> RTXDI_SUN_LAYER_SHIFT) & RTXDI_SUN_LAYER_MASK;
  result.visibility = status == RTXDI_SUN_STATUS_VISIBLE
                          ? (result.layers == 0u ? 1.0 : pow(saturate(transmission), result.layers))
                          : 0.0;
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

RtxdiCharacterLightResult RtxdiEvaluateCharacterLight(RtxdiLightingSelection selection, LightParam lamp,
                                                       float3 world_position, float3 normal)
{
  RtxdiCharacterLightResult result = (RtxdiCharacterLightResult)0;
  if (!selection.valid) return result;
  if ((selection.status != RTXDI_DI_STATUS_VISIBLE && selection.status != RTXDI_DI_STATUS_BLOCKED)
      || !all(isfinite(float4(world_position, 1.0))) || !all(isfinite(float4(normal, 0.0)))
      || !all(isfinite(float4(lamp.pos, lamp.radius))) || !all(isfinite(float4(lamp.charaColor, lamp.radiusInv)))
      || !all(isfinite(float4(lamp.vec, lamp.spotAngleInv))) || !isfinite(lamp.attenuation)
      || !isfinite(lamp.attenuationAngle) || !isfinite(lamp.translucency)
      || lamp.radius <= 0.0 || lamp.radiusInv <= 0.0
      || (selection.type == RTXDI_DI_LIGHT_TYPE_SPOT
          && (lamp.spotAngleInv <= 0.0 || lamp.attenuationAngle < 0.0 || dot(lamp.vec, lamp.vec) <= 1e-12)))
  {
    return result;
  }
  if (selection.status == RTXDI_DI_STATUS_BLOCKED)
  {
    result.resolved = true;
    return result;
  }

  const float3 to_lamp = lamp.pos - world_position;
  const float distance_squared = dot(to_lamp, to_lamp);
  if (!all(isfinite(to_lamp)) || !isfinite(distance_squared) || distance_squared <= 1e-12) return result;
  const float3 light_direction = to_lamp * rsqrt(distance_squared);
  const float attenuation = LampDistanceAttenuation(lamp, to_lamp);
  const float cone = selection.type == RTXDI_DI_LIGHT_TYPE_SPOT ? LampSpotCone(lamp, light_direction) : 1.0;
  const float lit = max(lamp.translucency, dot(normal, light_direction));
  const float3 diffuse = lamp.charaColor * (attenuation * cone * lit * selection.inverse_pdf);
  if (!all(isfinite(light_direction)) || !isfinite(lit) || !all(isfinite(diffuse)))
  {
    return result;
  }
  result.resolved = true;
  result.diffuse = diffuse;
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
