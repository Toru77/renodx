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

Texture2D<uint> g_srcDepthOnlyAOTerm : register(t0);
Texture2D<uint> g_srcNormalAOTerm : register(t1);
RWTexture2D<uint> g_outClampedAOTerm : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 dispatch_thread_id : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_srcNormalAOTerm.GetDimensions(width, height);
  if (dispatch_thread_id.x >= width || dispatch_thread_id.y >= height)
  {
    return;
  }

  const uint2 pix_coord = dispatch_thread_id;
  const uint packed_depth = g_srcDepthOnlyAOTerm.Load(int3(pix_coord, 0));
  const uint packed_normal = g_srcNormalAOTerm.Load(int3(pix_coord, 0));

  lpfloat depth_visibility;
  lpfloat3 depth_bent_normal;
  XeGTAO_DecodeVisibilityBentNormal(packed_depth, depth_visibility, depth_bent_normal);

  lpfloat normal_visibility;
  lpfloat3 normal_bent_normal;
  XeGTAO_DecodeVisibilityBentNormal(packed_normal, normal_visibility, normal_bent_normal);

  const lpfloat extra_darkening = max((lpfloat)0.0, depth_visibility - normal_visibility);
  lpfloat clamped_visibility = depth_visibility - extra_darkening * saturate((lpfloat)xegtao_normal_max_darkening);
  clamped_visibility = saturate(clamped_visibility);

  g_outClampedAOTerm[pix_coord] = XeGTAO_EncodeVisibilityBentNormal(clamped_visibility, normal_bent_normal);
}
