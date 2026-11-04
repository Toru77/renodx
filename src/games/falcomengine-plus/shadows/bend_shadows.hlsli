// Copyright 2023 Sony Interactive Entertainment.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Modified for falcomengine-plus: D3D11 cs_5_0 port of ComputeWavefrontExtents() and WriteScreenSpaceShadow() from
// reference/bend_sss_gpu.h. Parameters come from the including wrapper's BEND_* macros. SAMPLE_COUNT is a runtime
// value (a multiple of 4, 16..BEND_MAX_SAMPLE_COUNT), so the per-read arrays are folded into the read loop and the
// main samples are taken four at a time (the same lanes as the reference's i & 3 when SAMPLE_COUNT % 4 == 0). The
// early-out (wave intrinsics, not in SM5) is removed; the write goes through BEND_WRITE. Not in Bend: a per-game
// character receiver mask and character bias, an optional world-space thickness and length, and an optional IS-FAST
// start-depth dither. Names follow the reference.
//
// The wrapper defines before including: BEND_SAMPLE_COUNT_VALUE, BEND_SURFACE_THICKNESS, BEND_BILINEAR_THRESHOLD,
// BEND_SHADOW_CONTRAST, BEND_IGNORE_EDGE_PIXELS, BEND_USE_PRECISION_OFFSET, BEND_BILINEAR_OFFSET_MODE,
// BEND_WORLD_SPACE, BEND_WORLD_THICKNESS, BEND_WORLD_LENGTH, BEND_ISFAST, BEND_ISFAST_AMOUNT, BEND_CHAR_RECEIVE,
// BEND_CHAR_BIAS, BEND_DEBUG_VIEW(slot), BEND_WRITE(slot, xy, result).

#define RENODX_SHADOWS_DECLARE_CB_SCENE
#include "shadows_common.hlsli"

#define WAVE_SIZE BEND_WAVE_SIZE
#define HARD_SHADOW_SAMPLES 4   // Bend recommended starting value
#define FADE_OUT_SAMPLES 8      // Bend recommended starting value
#define MAX_READ_COUNT (BEND_MAX_SAMPLE_COUNT / WAVE_SIZE + 2)
// Reversed-Z (see FalcomDepthUnpackConsts): far plane 0, near plane 1.
#define BEND_FAR_DEPTH 0.0
#define BEND_NEAR_DEPTH 1.0

Texture2D<float4> g_depth : register(t0);            // the game's depth view, device Z in .x
Buffer<uint> g_dispatch : register(t1);              // dispatch blocks, layout in shadows_common.hlsli
Texture2D<uint4> g_mrt0 : register(t2);              // MRT0, FalcomIsCharacterMrt0
Texture3D<float2> g_isfastNoise : register(t3);      // IS-FAST volume; a 1x1 stand-in when not loaded (never sampled then)
SamplerState g_pointBorderSampler : register(s0);    // point, border colour = BEND_FAR_DEPTH
SamplerState g_pointClampSampler : register(s1);     // FalcomSampleISFAST

groupshared float DepthData[MAX_READ_COUNT * WAVE_SIZE];

[numthreads(WAVE_SIZE, 1, 1)]
void main(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
  const int3 inGroupID = (int3)groupId;
  const int inGroupThreadID = (int)groupThreadId.x;
  // cs_bend_dispatch_index = slot * 8 + dispatch; the sun uses slot 0.
  const uint dispatchCode = (uint)shader_injection_data.cs_bend_dispatch_index;
  const uint slot = dispatchCode >> 3u;
  const uint blockBase = slot * BEND_DISPATCH_UINTS;
  const uint waveBase = blockBase + BEND_WAVE_OFFSET + (dispatchCode & 7u) * 2u;
  const float4 lightCoordinate = asfloat(uint4(
      g_dispatch[blockBase + BEND_LIGHT_OFFSET + 0u], g_dispatch[blockBase + BEND_LIGHT_OFFSET + 1u],
      g_dispatch[blockBase + BEND_LIGHT_OFFSET + 2u], g_dispatch[blockBase + BEND_LIGHT_OFFSET + 3u]));
  const float4 lightWorld = asfloat(uint4(
      g_dispatch[blockBase + BEND_LIGHT_WORLD_OFFSET + 0u], g_dispatch[blockBase + BEND_LIGHT_WORLD_OFFSET + 1u],
      g_dispatch[blockBase + BEND_LIGHT_WORLD_OFFSET + 2u], g_dispatch[blockBase + BEND_LIGHT_WORLD_OFFSET + 3u]));
  const int2 waveOffset = asint(uint2(g_dispatch[waveBase], g_dispatch[waveBase + 1u]));
  const float2 invDepthTextureSize =
      1.0 / float2(shader_injection_data.cs_working_w, shader_injection_data.cs_working_h);
  const uint sampleCount = clamp(((uint)(BEND_SAMPLE_COUNT_VALUE) / 4u) * 4u, 16u, (uint)BEND_MAX_SAMPLE_COUNT);
  const uint readCount = sampleCount / WAVE_SIZE + 2u;
  const int debugView = BEND_DEBUG_VIEW(slot);

  // ComputeWavefrontExtents (reference lines 185-231)
  int2 xy = inGroupID.yz * WAVE_SIZE + waveOffset;
  float2 light_xy = floor(lightCoordinate.xy) + 0.5;
  float2 light_xy_fraction = lightCoordinate.xy - light_xy;
  bool reverse_direction = lightCoordinate.w > 0.0f;
  int2 sign_xy = sign(xy);
  bool horizontal = abs(xy.x + sign_xy.y) < abs(xy.y - sign_xy.x);
  int2 axis;
  axis.x = horizontal ? (+sign_xy.y) : (0);
  axis.y = horizontal ? (0) : (-sign_xy.x);
  xy = axis * (int)inGroupID.x + xy;
  float2 xy_f = (float2)xy;
  bool x_axis_major = abs(xy_f.x) > abs(xy_f.y);
  float major_axis = x_axis_major ? xy_f.x : xy_f.y;
  float major_axis_start = abs(major_axis);
  float major_axis_end = abs(major_axis) - (float)WAVE_SIZE;
  float ma_light_frac = x_axis_major ? light_xy_fraction.x : light_xy_fraction.y;
  ma_light_frac = major_axis > 0 ? -ma_light_frac : ma_light_frac;
  float2 start_xy = xy_f + light_xy;
  float2 end_xy = lerp(lightCoordinate.xy, start_xy, (major_axis_end + ma_light_frac) / (major_axis_start + ma_light_frac));
  float2 xy_delta = (start_xy - end_xy);
  float thread_step = (float)((uint)inGroupThreadID ^ (reverse_direction ? 0u : (uint)(WAVE_SIZE - 1)));
  float2 pixel_xy = lerp(start_xy, end_xy, thread_step / (float)WAVE_SIZE);
  float pixel_distance = major_axis_start - thread_step + ma_light_frac;

  // WriteScreenSpaceShadow (reference lines 261-468); the read loop folds lines 269-337 and 376-391.
  const float direction = -lightCoordinate.w;
  const float z_sign = BEND_NEAR_DEPTH > BEND_FAR_DEPTH ? -1 : +1;
  bool is_edge = false;
  float2 write_xy = floor(pixel_xy);
  const bool receiver_character = FalcomIsCharacterMrt0(g_mrt0.Load(int3((int2)write_xy, 0)));
  float start_sampling_depth = 0.0;
  float start_depth_thickness_scale = 0.0;
  float start_sample_distance = 0.0;

  [loop] for (uint i = 0; i < readCount; i++)
  {
    float2 read_xy = floor(pixel_xy);
    float minor_axis = x_axis_major ? pixel_xy.y : pixel_xy.x;
    const float edge_skip = 1e20;
    float2 depths;
    float bilinear = frac(minor_axis) - 0.5;
    read_xy += 0.5;  // USE_HALF_PIXEL_OFFSET
    float bias = bilinear > 0 ? 1 : -1;  // USE_UV_PIXEL_BIAS
    float2 offset_xy = float2(x_axis_major ? 0 : bias, x_axis_major ? bias : 0);
    depths.x = g_depth.SampleLevel(g_pointBorderSampler, read_xy * invDepthTextureSize, 0).x;
    depths.y = g_depth.SampleLevel(g_pointBorderSampler, (read_xy + offset_xy) * invDepthTextureSize, 0).x;
    float depth_thickness_scale = abs(BEND_FAR_DEPTH - depths.x);
    bool use_point_filter = abs(depths.x - depths.y) > depth_thickness_scale * BEND_BILINEAR_THRESHOLD;
    float sampling_depth;
    float shadowing_depth;
    if (BEND_BILINEAR_OFFSET_MODE) {
      bilinear = use_point_filter ? 0 : bilinear;
      sampling_depth = lerp(depths.x, depths.y, abs(bilinear));
      shadowing_depth = (BEND_IGNORE_EDGE_PIXELS && use_point_filter) ? edge_skip : sampling_depth;
    } else {
      sampling_depth = depths.x;
      float edge_depth = BEND_IGNORE_EDGE_PIXELS ? edge_skip : depths.x;
      float shadow_depth = depths.x + abs(depths.x - depths.y) * z_sign;
      shadowing_depth = use_point_filter ? edge_depth : shadow_depth;
    }
    float sample_distance = pixel_distance + (WAVE_SIZE * i) * direction;
    float stored_depth = (shadowing_depth - lightCoordinate.z) / sample_distance;
    if (i == 0) {
      is_edge = use_point_filter;
      start_sampling_depth = sampling_depth;
      start_depth_thickness_scale = depth_thickness_scale;
      start_sample_distance = sample_distance;
    } else {
      stored_depth = sample_distance > 0 ? stored_depth : 1e10;
    }
    DepthData[i * WAVE_SIZE + (uint)inGroupThreadID] = stored_depth;
    pixel_xy += xy_delta * direction;
  }
  GroupMemoryBarrierWithGroupSync();

  const float2 unpack = FalcomDepthUnpackConsts();
  const float z0 = FalcomLinearDepth(start_sampling_depth, unpack);
  float surface_thickness = BEND_SURFACE_THICKNESS;
  uint pixel_sample_count = sampleCount;
  if (BEND_WORLD_SPACE && start_sampling_depth > 0.0) {
    // Bend's window is SurfaceThickness x |far - d| = T x d0 in device depth (reversed-Z), and dd/dz = mul / z^2, so
    // a world thickness W at the receiver needs T = W x |mul| / (z0^2 x d0).
    surface_thickness = BEND_WORLD_THICKNESS * abs(unpack.x) / max(z0 * z0 * start_sampling_depth, 1e-12);
    // World length: project the receiver and the point BEND_WORLD_LENGTH toward the light; that many pixels is this
    // pixel's sample count (a multiple of 4, at least the hard + fade samples, at most sampleCount).
    const float2 receiver_uv = (write_xy + 0.5) * invDepthTextureSize;
    const float4 receiver_h = mul(float4(receiver_uv.x * 2.0 - 1.0, 1.0 - receiver_uv.y * 2.0, start_sampling_depth, 1.0),
                                  viewProjInv_g);
    const float3 receiver_ws = receiver_h.xyz / receiver_h.w;
    const float3 to_light = lightWorld.w > 0.5 ? (lightWorld.xyz - receiver_ws) : lightWorld.xyz;
    const float length_ws = lightWorld.w > 0.5 ? min(BEND_WORLD_LENGTH, length(to_light)) : BEND_WORLD_LENGTH;
    const float4 end_h = mul(float4(receiver_ws + FalcomSafeNormalize3(to_light, float3(0.0, 0.0, 0.0)) * length_ws, 1.0),
                             viewProj_g);
    if (end_h.w > 1e-4) {
      const float2 end_px = float2(end_h.x / end_h.w * 0.5 + 0.5, end_h.y / end_h.w * -0.5 + 0.5) / invDepthTextureSize;
      const uint length_px = (uint)(length(end_px - (write_xy + 0.5)) / 4.0) * 4u;
      pixel_sample_count = clamp(length_px, (uint)(HARD_SHADOW_SAMPLES + FADE_OUT_SAMPLES), sampleCount);
    }
  }

  float start_depth = start_sampling_depth;
  if (BEND_CHAR_RECEIVE && receiver_character && start_sampling_depth > 0.0) {
    // Character bias (the RT passes' Character Normal Bias): move only this receiver's start depth toward the camera by
    // BEND_CHAR_BIAS world units. The depth this pixel shares with the wave is unchanged, so characters still cast.
    start_depth += BEND_CHAR_BIAS * abs(unpack.x) / max(z0 * z0, 1e-12);
  }
  if (BEND_USE_PRECISION_OFFSET) start_depth = lerp(start_depth, BEND_FAR_DEPTH, -1.0 / 0xFFFF);
  start_depth = (start_depth - lightCoordinate.z) / start_sample_distance;
  const uint sample_index = (uint)inGroupThreadID + 1u;
  float4 shadow_value = 1;
  float hard_shadow = 1;
  float depth_scale = min(start_sample_distance + direction, 1.0 / surface_thickness)
                    * start_sample_distance / start_depth_thickness_scale;
  start_depth = start_depth * depth_scale - z_sign;
  if (BEND_ISFAST) {
    // Start-depth dither in window units (the window is 1 wide here). The ray and its shared samples are not jittered.
    start_depth += (FalcomSampleISFAST(g_isfastNoise, g_pointClampSampler, (uint2)write_xy,
                                       (uint)max(shader_injection_data.cs_noise_frame, 0.0f), 128.0, 32.0,
                                       shader_injection_data.shadow_isfast_spatial_scale,
                                       shader_injection_data.shadow_isfast_temporal_speed,
                                       shader_injection_data.shadow_isfast_seed_offset,
                                       shader_injection_data.shadow_isfast_texture_loaded) - 0.5) * BEND_ISFAST_AMOUNT;
  }
  [unroll] for (uint h = 0; h < HARD_SHADOW_SAMPLES; h++) {
    float depth_delta = abs(start_depth - DepthData[sample_index + h] * depth_scale);
    hard_shadow = min(hard_shadow, depth_delta);
  }
  const uint main_end = pixel_sample_count - FADE_OUT_SAMPLES;
  [loop] for (uint g = HARD_SHADOW_SAMPLES; g < main_end; g += 4u) {
    const float4 depth_delta = abs(start_depth - float4(
        DepthData[sample_index + g], DepthData[sample_index + g + 1u],
        DepthData[sample_index + g + 2u], DepthData[sample_index + g + 3u]) * depth_scale);
    shadow_value = min(shadow_value, depth_delta);
  }
  [unroll] for (uint f = 0; f < FADE_OUT_SAMPLES; f++) {
    float depth_delta = abs(start_depth - DepthData[sample_index + main_end + f] * depth_scale);
    const float fade_out = (float)(f + 1) / (float)(FADE_OUT_SAMPLES + 1) * 0.75;
    shadow_value[f & 3] = min(shadow_value[f & 3], depth_delta + fade_out);
  }
  shadow_value = saturate(shadow_value * BEND_SHADOW_CONTRAST + (1 - BEND_SHADOW_CONTRAST));
  hard_shadow = saturate(hard_shadow * BEND_SHADOW_CONTRAST + (1 - BEND_SHADOW_CONTRAST));
  float result = dot(shadow_value, 0.25);
  result = min(hard_shadow, result);
  if (!BEND_CHAR_RECEIVE && receiver_character) result = 1;
  if (debugView == 2) result = is_edge ? 1 : 0;
  if (debugView == 3) result = (inGroupThreadID / (float)WAVE_SIZE);
  if (debugView == 4) result = frac(inGroupID.x / (float)WAVE_SIZE);
  if (debugView == 7) result = receiver_character ? 0 : 1;
  BEND_WRITE(slot, (int2)write_xy, result);
}
