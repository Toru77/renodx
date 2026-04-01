#ifndef VA_SATURATE
#define VA_SATURATE(x) saturate(x)
#endif

#ifndef XE_GTAO_USE_DEFAULT_CONSTANTS
#define XE_GTAO_USE_DEFAULT_CONSTANTS 0
#endif

#ifndef XE_GTAO_USE_HALF_FLOAT_PRECISION
#define XE_GTAO_USE_HALF_FLOAT_PRECISION 0
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

Texture2D<uint> g_srcWorkingAOTerm : register(t0);
Texture2D<lpfloat> g_srcWorkingEdges : register(t1);
SamplerState g_samplerPointClamp : register(s0);

RWTexture2D<uint> g_outFinalAOTerm : register(u0);

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

[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void main(uint2 dispatch_thread_id : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_srcWorkingAOTerm.GetDimensions(width, height);

  const uint2 pix_coord_base = dispatch_thread_id * uint2(2, 1);
  if (pix_coord_base.x >= width || pix_coord_base.y >= height)
  {
    return;
  }

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));
  const bool is_last_pass = xegtao_denoise_is_last_pass >= 0.5;
  XeGTAO_Denoise(
      pix_coord_base,
      consts,
      g_srcWorkingAOTerm,
      g_srcWorkingEdges,
      g_samplerPointClamp,
      g_outFinalAOTerm,
      is_last_pass);
}

