#include "./GTAO/XeGTAO.h"

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
  float xegtao_bent_normals;
  float xegtao_reserved0;
  float xegtao_reserved1;
};

Texture2D<float4> g_srcOriginalAO : register(t0);
Texture2D<uint> g_srcGTAOTerm : register(t1);
RWTexture2D<float4> g_outComposite : register(u0);

float4 UnpackR8G8B8A8(uint packed_value)
{
  float4 unpacked;
  unpacked.x = (float)(packed_value & 0x000000FFu) * (1.0 / 255.0);
  unpacked.y = (float)((packed_value >> 8u) & 0x000000FFu) * (1.0 / 255.0);
  unpacked.z = (float)((packed_value >> 16u) & 0x000000FFu) * (1.0 / 255.0);
  unpacked.w = (float)((packed_value >> 24u) & 0x000000FFu) * (1.0 / 255.0);
  return unpacked;
}

float4 DecodeAOTerm(uint packed_term)
{
  // Final denoise output is already encoded in display-space visibility and bent-normal channels.
  return saturate(UnpackR8G8B8A8(packed_term));
}

[numthreads(8, 8, 1)]
void main(uint2 dispatch_thread_id : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_outComposite.GetDimensions(width, height);
  if (dispatch_thread_id.x >= width || dispatch_thread_id.y >= height)
  {
    return;
  }

  const int3 load_coord = int3(dispatch_thread_id, 0);
  float4 original_ao = g_srcOriginalAO.Load(load_coord);
  float4 gtao_term = DecodeAOTerm(g_srcGTAOTerm.Load(load_coord).x);
  // Contract:
  //   x = visibility (decoded .w),
  //   yz = bent normal payload (decoded .xy),
  //   w = original alpha.
  g_outComposite[dispatch_thread_id] = float4(gtao_term.w, gtao_term.x, gtao_term.y, original_ao.w);
}

