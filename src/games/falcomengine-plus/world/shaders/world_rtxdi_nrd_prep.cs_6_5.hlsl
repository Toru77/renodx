#include "../../include/mrt_normal.hlsli"
#include "world_rt_lamps.hlsli"
#include "../rtxdi/rtxdi_di_layout.h"

Texture2D<float> g_depth : register(t0);
Texture2D<uint4> g_normal : register(t1);
Texture2D<uint4> g_scene : register(t2);
Texture2D<float4> g_motion : register(t3);
Texture2D<uint2> g_mrt2 : register(t4);
StructuredBuffer<GameDeferredParam> g_deferred : register(t5);
Texture2D<float4> g_n1_diffuse : register(t6);
Texture2D<uint> g_n1_status : register(t7);

RWTexture2D<float2> g_nrd_mv : register(u0);
RWTexture2D<float4> g_nrd_normal_roughness : register(u1);
RWTexture2D<float> g_nrd_viewz : register(u2);
RWTexture2D<float4> g_nrd_diffuse : register(u3);
RWTexture2D<float4> g_nrd_specular : register(u4);

cbuffer NrdPrepParams : register(b0) {
  uint2 g_dimensions;
  uint g_deferred_count;
  uint g_motion_correction_index;
  float2 g_motion_correction_0;
  float2 g_motion_correction_1;
  uint g_history_valid;
  uint g_padding;
};

float4 NrdPackNormalRoughness(float3 normal, float roughness, float material_id) {
  normal /= abs(normal.x) + abs(normal.y) + abs(normal.z);
  float3 packed;
  packed.y = normal.y * 0.5 + 0.5;
  packed.x = normal.x * 0.5 + packed.y;
  packed.y -= normal.x * 0.5;
  const float safe_roughness = max(saturate(roughness), 1.5 / 512.0);
  packed.z = (normal.z < 0.0 ? -safe_roughness : safe_roughness) * 0.5 + 0.5;
  return float4(packed, saturate(material_id / 3.0));
}

float4 NrdPackDiffuse(float3 radiance, float hit_distance) {
  const bool valid = all(isfinite(radiance)) && isfinite(hit_distance) && hit_distance >= 0.0;
  return valid ? float4(clamp(radiance, 0.0, 65504.0), min(hit_distance, 65504.0)) : 0.0;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id.xy >= g_dimensions)) return;
  const uint2 pixel = dispatch_id.xy;
  const float depth = g_depth.Load(int3(pixel, 0));
  const uint4 mrt0 = g_normal.Load(int3(pixel, 0));
  const float3 normal = DecodeFalcomMrtNormalAsGame(mrt0.xy);
  const float2 uv = (float2(pixel) + 0.5) / float2(g_dimensions);
  const float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
  const float4x4 inverse_view_projection = float4x4(asfloat(g_scene.Load(int3(20, 0, 0))),
                                                    asfloat(g_scene.Load(int3(21, 0, 0))),
                                                    asfloat(g_scene.Load(int3(22, 0, 0))),
                                                    asfloat(g_scene.Load(int3(23, 0, 0))));
  const float4 world_h = mul(clip, inverse_view_projection);
  const float3 world = world_h.xyz / world_h.w;
  const float4x4 view = float4x4(asfloat(g_scene.Load(int3(0, 0, 0))), asfloat(g_scene.Load(int3(1, 0, 0))),
                                 asfloat(g_scene.Load(int3(2, 0, 0))), asfloat(g_scene.Load(int3(3, 0, 0))));
  const float viewz = -mul(float4(world, 1.0), view).z;
  const uint status = g_n1_status.Load(int3(pixel, 0));
  const bool known = (status & RTXDI_N1_STATUS_KNOWN_BIT) != 0u
                     && ((status & 0x07u) == RTXDI_N1_STATUS_VISIBLE || (status & 0x07u) == RTXDI_N1_STATUS_BLOCKED);
  const bool character = (mrt0.w & 8u) == 0u;
  const bool valid = isfinite(depth) && depth > 0.0 && depth <= 1.0 && isfinite(world_h.w) && abs(world_h.w) > 1.0e-8
                     && all(isfinite(world)) && all(isfinite(normal)) && dot(normal, normal) > 0.5
                     && isfinite(viewz) && viewz > 0.0 && known;
  float roughness = 1.0;
  const uint material_index = g_mrt2.Load(int3(pixel, 0)).x;
  if (!character && material_index < g_deferred_count) {
    const float candidate_roughness = g_deferred[material_index].tail[0].z;
    if (isfinite(candidate_roughness)) roughness = saturate(candidate_roughness);
  }
  const float4 packed = valid ? g_n1_diffuse.Load(int3(pixel, 0)) : 0.0;
  const bool finite_input = all(isfinite(packed)) && all(packed >= 0.0) && all(packed <= 65504.0);
  const float2 motion = g_motion.Load(int3(pixel, 0)).xy;
  const float2 correction = g_motion_correction_index == 0u ? g_motion_correction_0 : g_motion_correction_1;
  g_nrd_mv[pixel] = g_history_valid != 0u && all(isfinite(motion))
                        ? (motion - correction) / float2(g_dimensions)
                        : 0.0;
  g_nrd_normal_roughness[pixel] = valid ? NrdPackNormalRoughness(normal, roughness, character ? 1.0 : 0.0)
                                         : NrdPackNormalRoughness(float3(0.0, 0.0, 1.0), 1.0, 0.0);
  g_nrd_viewz[pixel] = valid ? viewz : 0.0;
  g_nrd_diffuse[pixel] = valid && finite_input ? NrdPackDiffuse(packed.rgb, packed.a) : 0.0;
  g_nrd_specular[pixel] = 0.0;
}
