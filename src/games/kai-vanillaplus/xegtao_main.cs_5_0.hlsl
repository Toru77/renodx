#ifndef VA_SATURATE
#define VA_SATURATE(x) saturate(x)
#endif

#ifndef XE_GTAO_USE_DEFAULT_CONSTANTS
#define XE_GTAO_USE_DEFAULT_CONSTANTS 0
#endif

#include "./GTAO/XeGTAO.h"

cbuffer cb_scene : register(b0)
{
  float4x4 view_g;
  float4x4 viewInv_g;
  float4x4 proj_g;
};

cbuffer cb_xegtao : register(b13)
{
  float xegtao_quality;
  float xegtao_denoise_passes;
  float xegtao_radius;
  float xegtao_falloff_range;
  float xegtao_radius_multiplier;
  float xegtao_final_value_power;
  float xegtao_sample_distribution_power;
  float xegtao_thin_occluder_compensation;
  float xegtao_depth_mip_sampling_offset;
  float xegtao_denoise_blur_beta;
  float xegtao_noise_index;
  float xegtao_debug_mode;
  float xegtao_denoise_is_last_pass;
  float xegtao_normal_input_mode;
  float xegtao_mrt_normal_available;
  float xegtao_normal_influence;
  float xegtao_normal_depth_blend;
  float xegtao_normal_sharpness;
  float xegtao_normal_edge_rejection;
  float xegtao_normal_z_preservation;
  float xegtao_normal_detail_response;
  float xegtao_normal_max_darkening;
  float xegtao_normal_darkening_mode;
  float xegtao_denoiser_mode;
  float xegtao_copyback_preserve_yzw;
  float xegtao_isfast_passes;
  float xegtao_isfast_samples;
  float xegtao_isfast_radius;
  float xegtao_isfast_edge_sensitivity;
  float xegtao_isfast_spatial_sigma;
  float xegtao_isfast_hybrid_blend;
  float xegtao_isfast_noise_available;
};

#ifndef XE_GTAO_COMPUTE_BENT_NORMALS
#define XE_GTAO_COMPUTE_BENT_NORMALS 1
#endif

#include "./GTAO/XeGTAO.hlsli"

Texture2D<lpfloat> g_srcWorkingDepth : register(t0);
Texture2D<uint4> g_srcMrtNormal : register(t1);
SamplerState g_samplerPointClamp : register(s0);

RWTexture2D<uint> g_outWorkingAOTerm : register(u0);
RWTexture2D<unorm float> g_outWorkingEdges : register(u1);

GTAOConstants BuildGTAOConstants(uint2 viewport_size)
{
  GTAOConstants consts = (GTAOConstants)0;

  const float2 viewport_size_f = max(float2(viewport_size), 1.0.xx);
  consts.ViewportSize = int2(viewport_size);
  consts.ViewportPixelSize = 1.0.xx / viewport_size_f;

  float depth_linearize_mul = -proj_g[3][2];
  float depth_linearize_add = proj_g[2][2];
  if (depth_linearize_mul * depth_linearize_add < 0.0)
  {
    depth_linearize_add = -depth_linearize_add;
  }
  consts.DepthUnpackConsts = float2(depth_linearize_mul, depth_linearize_add);

  const float tan_half_fov_y = 1.0 / proj_g[1][1];
  const float tan_half_fov_x = 1.0 / proj_g[0][0];
  consts.CameraTanHalfFOV = float2(tan_half_fov_x, tan_half_fov_y);

  consts.NDCToViewMul = float2(consts.CameraTanHalfFOV.x * 2.0, consts.CameraTanHalfFOV.y * -2.0);
  consts.NDCToViewAdd = float2(-consts.CameraTanHalfFOV.x, consts.CameraTanHalfFOV.y);
  consts.NDCToViewMul_x_PixelSize = consts.NDCToViewMul * consts.ViewportPixelSize;

  consts.EffectRadius = max(0.001, xegtao_radius);
  consts.EffectFalloffRange = saturate(xegtao_falloff_range);
  consts.RadiusMultiplier = max(0.3, xegtao_radius_multiplier);
  consts.FinalValuePower = max(0.5, xegtao_final_value_power);
  consts.DenoiseBlurBeta = max(0.01, xegtao_denoise_blur_beta);
  consts.SampleDistributionPower = max(1.0, xegtao_sample_distribution_power);
  consts.ThinOccluderCompensation = max(0.0, xegtao_thin_occluder_compensation);
  consts.DepthMIPSamplingOffset = max(0.0, xegtao_depth_mip_sampling_offset);
  consts.NoiseIndex = (int)xegtao_noise_index;
  consts.Padding0 = 0.0;

  return consts;
}

lpfloat2 SpatioTemporalNoise(uint2 pix_coord, uint temporal_index)
{
  uint2 wrapped = pix_coord % XE_HILBERT_WIDTH;
  uint index = HilbertIndex(wrapped.x, wrapped.y);
  index += 288u * (temporal_index % 64u);
  return lpfloat2(frac(0.5 + index * float2(0.75487766624669276005, 0.5698402909980532659114)));
}

void GetQualityParameters(out lpfloat slice_count, out lpfloat steps_per_slice)
{
  uint quality_level = (uint)round(clamp(xegtao_quality, 0.0, 2.0));
  if (quality_level == 0u)
  {
    // High
    slice_count = 3.0;
    steps_per_slice = 3.0;
  }
  else if (quality_level == 1u)
  {
    // Very High (between High and Ultra)
    slice_count = 6.0;
    steps_per_slice = 3.0;
  }
  else
  {
    // Ultra
    slice_count = 9.0;
    steps_per_slice = 3.0;
  }
}

bool TryMapWorkingPixelToMrtTexel(uint2 working_pix_coord, uint2 working_size, out uint2 mapped_mrt_texel)
{
  mapped_mrt_texel = uint2(0u, 0u);

  uint mrt_width;
  uint mrt_height;
  g_srcMrtNormal.GetDimensions(mrt_width, mrt_height);
  if (mrt_width == 0u || mrt_height == 0u)
  {
    return false;
  }

  uint2 mrt_size = uint2(mrt_width, mrt_height);
  float2 safe_working_size = max(float2(working_size), 1.0.xx);
  float2 scale = float2(mrt_size) / safe_working_size;
  float2 mapped = floor((float2(working_pix_coord) + 0.5.xx) * scale);
  mapped_mrt_texel = min((uint2)mapped, mrt_size - 1u);
  return true;
}

float3 DecodeMrtNormalAsIs(uint2 mrt_texel)
{
  const uint4 mrt_sample = g_srcMrtNormal.Load(int3(mrt_texel, 0));
  float2 encoded = float2((float)mrt_sample.x, (float)mrt_sample.y) * (1.0 / 32767.5) + float2(-1.0, -1.0);
  float azimuth = 3.14159274 * encoded.x;
  float sin_azimuth;
  float cos_azimuth;
  sincos(azimuth, sin_azimuth, cos_azimuth);
  float ring = sqrt(saturate(1.0 - encoded.y * encoded.y));
  float3 normal = float3(cos_azimuth * ring, sin_azimuth * ring, encoded.y);
  float len2 = dot(normal, normal);
  if (len2 < 1e-5)
  {
    return float3(0.0, 0.0, 1.0);
  }
  return normal * rsqrt(len2);
}

float3 TransformNormalToView(float3 decoded_normal)
{
  float3 view_normal = mul((float3x3)view_g, decoded_normal);
  float len2 = dot(view_normal, view_normal);
  if (len2 < 1e-5)
  {
    return float3(0.0, 0.0, 1.0);
  }
  return view_normal * rsqrt(len2);
}

float3 SafeNormalize3(float3 value, float3 fallback_value)
{
  float len2 = dot(value, value);
  if (len2 < 1e-5)
  {
    return fallback_value;
  }
  return value * rsqrt(len2);
}

lpfloat LoadWorkingDepthClamped(int2 texel, int2 size)
{
  int2 clamped = clamp(texel, int2(0, 0), size - int2(1, 1));
  return g_srcWorkingDepth.Load(int3(clamped, 0));
}

float ComputeDepthEdgeMetric(uint2 pix_coord, GTAOConstants consts)
{
  int2 texel = int2(pix_coord);
  int2 size = max(consts.ViewportSize, int2(1, 1));

  lpfloat center = LoadWorkingDepthClamped(texel, size);
  lpfloat left = LoadWorkingDepthClamped(texel + int2(-1, 0), size);
  lpfloat right = LoadWorkingDepthClamped(texel + int2(1, 0), size);
  lpfloat top = LoadWorkingDepthClamped(texel + int2(0, -1), size);
  lpfloat bottom = LoadWorkingDepthClamped(texel + int2(0, 1), size);

  float max_abs_neighbor_delta = max(
      max(abs(left - center), abs(right - center)),
      max(abs(top - center), abs(bottom - center)));
  return saturate(max_abs_neighbor_delta * 4.0);
}

float3 BuildDepthFallbackNormal(uint2 pix_coord, GTAOConstants consts)
{
  float2 normalized_screen_pos = (pix_coord + float2(0.5, 0.5)) * consts.ViewportPixelSize;

  lpfloat4 values_ul = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pix_coord * consts.ViewportPixelSize));
  lpfloat4 values_br = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pix_coord * consts.ViewportPixelSize), int2(1, 1));

  lpfloat viewspace_z = values_ul.y;
  lpfloat pix_lz = values_ul.x;
  lpfloat pix_tz = values_ul.z;
  lpfloat pix_rz = values_br.z;
  lpfloat pix_bz = values_br.x;
  lpfloat4 edges_lrtb = XeGTAO_CalculateEdges((lpfloat)viewspace_z, (lpfloat)pix_lz, (lpfloat)pix_rz, (lpfloat)pix_tz, (lpfloat)pix_bz);

  float3 center = XeGTAO_ComputeViewspacePosition(normalized_screen_pos, viewspace_z, consts);
  float3 left = XeGTAO_ComputeViewspacePosition(normalized_screen_pos + float2(-1.0, 0.0) * consts.ViewportPixelSize, pix_lz, consts);
  float3 right = XeGTAO_ComputeViewspacePosition(normalized_screen_pos + float2(1.0, 0.0) * consts.ViewportPixelSize, pix_rz, consts);
  float3 top = XeGTAO_ComputeViewspacePosition(normalized_screen_pos + float2(0.0, -1.0) * consts.ViewportPixelSize, pix_tz, consts);
  float3 bottom = XeGTAO_ComputeViewspacePosition(normalized_screen_pos + float2(0.0, 1.0) * consts.ViewportPixelSize, pix_bz, consts);

  float3 depth_normal = XeGTAO_CalculateNormal(edges_lrtb, center, left, right, top, bottom);
  float len2 = dot(depth_normal, depth_normal);
  if (len2 < 1e-5)
  {
    return float3(0.0, 0.0, 1.0);
  }
  return depth_normal * rsqrt(len2);
}

float3 BuildSelectedInputNormal(uint2 pix_coord, uint2 working_size, GTAOConstants consts)
{
  float3 depth_fallback_normal = BuildDepthFallbackNormal(pix_coord, consts);
  float3 selected = depth_fallback_normal;
  if (xegtao_normal_input_mode < 0.5)
  {
    return selected;
  }

  if (xegtao_mrt_normal_available >= 0.5)
  {
    uint2 mapped_mrt_texel = uint2(0u, 0u);
    if (!TryMapWorkingPixelToMrtTexel(pix_coord, working_size, mapped_mrt_texel))
    {
      return selected;
    }

    float3 decoded_normal = DecodeMrtNormalAsIs(mapped_mrt_texel);
    float decoded_len2 = dot(decoded_normal, decoded_normal);
    if (decoded_len2 >= 1e-5)
    {
      float3 mrt_normal = TransformNormalToView(decoded_normal);
      float3 tuned_mrt = mrt_normal;
      tuned_mrt.xy *= max(0.0, xegtao_normal_influence);
      tuned_mrt.z *= max(0.0, xegtao_normal_z_preservation);
      tuned_mrt = SafeNormalize3(tuned_mrt, mrt_normal);

      float sharpness = max(0.01, xegtao_normal_sharpness);
      float base_blend = pow(saturate(xegtao_normal_depth_blend), 1.0 / sharpness);
      float edge_metric = ComputeDepthEdgeMetric(pix_coord, consts);
      float edge_attenuation = 1.0 - saturate(edge_metric * max(0.0, xegtao_normal_edge_rejection));
      float normal_delta = 1.0 - saturate(dot(depth_fallback_normal, tuned_mrt));
      float detail_response = max(0.01, xegtao_normal_detail_response);
      float detail_curve = pow(normal_delta, 1.0 / detail_response);
      float detail_gain = lerp(0.35, 1.25, detail_curve);
      float final_blend = saturate(base_blend * edge_attenuation * detail_gain);
      if (xegtao_normal_darkening_mode < 0.5)
      {
        final_blend *= saturate(xegtao_normal_max_darkening);
      }
      selected = SafeNormalize3(lerp(depth_fallback_normal, tuned_mrt, final_blend), depth_fallback_normal);
    }
  }
  return selected;
}

[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void main(uint2 pix_coord : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_srcWorkingDepth.GetDimensions(width, height);

  if (pix_coord.x >= width || pix_coord.y >= height)
  {
    return;
  }

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));
  lpfloat slice_count;
  lpfloat steps_per_slice;
  GetQualityParameters(slice_count, steps_per_slice);

  uint noise_index = consts.NoiseIndex < 0 ? 0u : (uint)consts.NoiseIndex;
  lpfloat3 selected_normal = (lpfloat3)BuildSelectedInputNormal(pix_coord, uint2(width, height), consts);

  XeGTAO_MainPass(
      pix_coord,
      slice_count,
      steps_per_slice,
      SpatioTemporalNoise(pix_coord, noise_index),
      selected_normal,
      consts,
      g_srcWorkingDepth,
      g_samplerPointClamp,
      g_outWorkingAOTerm,
      g_outWorkingEdges);
}

