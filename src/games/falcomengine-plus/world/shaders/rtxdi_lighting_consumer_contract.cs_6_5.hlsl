#include "world_rt_lamps.hlsli"
#include "world_rtxdi_lighting_consumer.hlsli"

struct LightingConsumerCase
{
  uint4 tuple;
  uint light_count;
  uint bound;
  uint2 padding;
  uint original_point_count;
  uint original_spot_count;
  float lamp_radiance;
  float base_color;
  float dynamic_intensity;
  float padding2;
  LightIndexData tile;
  uint sun_bound;
  uint sun_status;
  float csm_visibility;
  uint padding3;
};

struct LightingConsumerResult
{
  uint mode;
  uint point_count;
  uint spot_count;
  float inverse_pdf;
  float raw_local_term;
  float shaded_local_term;
  uint selected_index;
  uint selected_type;
  uint sun_known;
  float environment_sun_visibility;
  float character_sun_multiplier;
  uint padding;
};

StructuredBuffer<LightingConsumerCase> g_cases : register(t0);
RWStructuredBuffer<LightingConsumerResult> g_results : register(u0);

cbuffer ConsumerConstants : register(b0)
{
  uint g_case_count;
};

[numthreads(64, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
  const uint index = dispatch_thread_id.x;
  if (index >= g_case_count) return;

  const LightingConsumerCase input = g_cases[index];
  const RtxdiLightingSelection selection =
      RtxdiResolveLightingSelection(input.bound != 0u, input.tuple, input.light_count, input.tile);
  uint point_count, spot_count;
  RtxdiEffectiveLightingLoopCounts(selection, min(input.original_point_count, 63u),
                                   min(input.original_spot_count, 63u), point_count, spot_count);
  const bool visible = selection.valid && selection.status == RTXDI_DI_STATUS_VISIBLE;
  const bool blocked = selection.valid && selection.status == RTXDI_DI_STATUS_BLOCKED;
  const float raw_local_term = visible ? input.lamp_radiance * selection.inverse_pdf
                              : blocked ? 0.0
                                        : input.original_point_count + input.original_spot_count > 0u
                                              ? input.lamp_radiance
                                              : 0.0;

  LightingConsumerResult result;
  result.mode = visible ? 1u : blocked ? 2u : 0u;
  result.point_count = point_count;
  result.spot_count = spot_count;
  result.inverse_pdf = visible ? selection.inverse_pdf : 0.0;
  result.raw_local_term = raw_local_term;
  result.shaded_local_term = raw_local_term * input.base_color * input.dynamic_intensity;
  result.selected_index = visible ? selection.index : 0xffffffffu;
  result.selected_type = visible ? selection.type : 0u;
  const RtxdiSunLightingSelection sun_selection = RtxdiResolveSunLightingSelection(input.sun_bound != 0u, input.sun_status);
  result.sun_known = sun_selection.known ? 1u : 0u;
  result.environment_sun_visibility = sun_selection.known ? sun_selection.visibility : input.csm_visibility;
  result.character_sun_multiplier = sun_selection.known ? sun_selection.visibility : 1.0;
  result.padding = 0u;
  g_results[index] = result;
}
