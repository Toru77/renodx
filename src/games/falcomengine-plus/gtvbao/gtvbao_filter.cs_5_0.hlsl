// gtvbao_filter.cs_5_0.hlsl - GTVBAO spatial filter (RTGI filter structure, GTVBAO AO domain).
//
// One dispatch per pass. Separable (filter_kind 0): one 1D Gaussian pass, horizontal (direction 0) or vertical (1).
// A-trous (filter_kind 1): one 2D pass of taps x taps binomial weights at spacing filter_step. Edge stops are
// FilterPlaneNormalWeight (world_rt_denoise.hlsli) on the depth plane and the view-space normal at each AO texel's
// representative full-res pixel min(tq * d + d / 2, full - 1). Input t0 (AO byte), output u0 (AO byte).

#include "gtvbao_common.hlsl"
#include "../world/shaders/world_rt_denoise.hlsli"

Texture2D<uint> g_in_ao : register(t0);
Texture2D<float> g_depth : register(t1);
Texture2D<float4> g_normal : register(t2);

RWTexture2D<uint> g_out_ao : register(u0);

SamplerState g_samp_point : register(s0);

// World-space (view) position and normal of AO texel tq at its representative pixel; false for sky or invalid normal.
bool RepresentativeSurface(int2 tq, uint2 full_size, GTAOConstants consts, out float3 P, out float3 N)
{
  const uint d = (uint)GTVBAO_Divisor();
  const uint2 px = min(uint2(tq) * d + d / 2u, full_size - 1u);
  const float z = g_depth.Load(int3(px, 0));
  N = g_normal.Load(int3(px, 0)).xyz;
  const float2 uv = (float2(px) + 0.5) / float2(full_size);
  P = GTVBAO_ComputeViewspacePosition(uv, z, consts);
  return z > 0.0 && FalcomNormalValid(N);
}

// One tap: kernel weight times the edge stop (1 at the centre). Taps outside the image or not a surface contribute 0.
void FilterTap(int2 q, float kernel_w, bool centre, int2 size, uint2 full_size, GTAOConstants consts,
               float3 Pc, float3 Nc, float dist_c, inout float sum_w, inout float sum_v)
{
  if (any(q < 0) || any(q >= size)) return;
  float edge = 1.0;
  if (!centre)
  {
    float3 Pt;
    float3 Nt;
    edge = RepresentativeSurface(q, full_size, consts, Pt, Nt) ? FilterPlaneNormalWeight(Pc, Nc, dist_c, Pt, Nt) : 0.0;
  }
  sum_w += kernel_w * edge;
  sum_v += kernel_w * edge * ((float)g_in_ao[q] * (1.0f / 255.0f));
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
  uint aw, ah;
  g_in_ao.GetDimensions(aw, ah);
  if (id.x >= aw || id.y >= ah) return;

  uint fw, fh;
  g_depth.GetDimensions(fw, fh);
  const uint2 full_size = uint2(fw, fh);
  const GTAOConstants consts = BuildGTAOConstants(full_size);
  const int2 size = int2(aw, ah);
  const int2 px = int2(id.xy);
  const bool final_pass = GTVBAO_final_pass > 0.5f;
  const float output_scale = final_pass ? GT_VBAO_OCCLUSION_TERM_SCALE : 1.0f;

  float result = (float)g_in_ao[px] * (1.0f / 255.0f);
  float3 Pc;
  float3 Nc;
  if (RepresentativeSurface(px, full_size, consts, Pc, Nc))
  {
    const float dist_c = length(Pc);
    const int taps = (int)GTVBAO_filter_taps;
    const int half_taps = (taps - 1) / 2;
    float sum_w = 0.0;
    float sum_v = 0.0;
    if (GTVBAO_filter_kind < 0.5f)
    {
      const float sigma = FilterSeparableSigma(GTVBAO_filter_radius);
      for (int k = -half_taps; k <= half_taps; ++k)
      {
        const int2 offset = GTVBAO_filter_direction < 0.5f ? int2(k, 0) : int2(0, k);
        FilterTap(px + offset, FilterGaussianWeight(k, sigma), k == 0, size, full_size, consts, Pc, Nc, dist_c, sum_w, sum_v);
      }
    }
    else
    {
      // A-trous binomial row (1 2 1, 1 4 6 4 1, ...), integer recurrence.
      float wk[9];
      int c = 1;
      wk[0] = (float)c;
      for (int t = 0; t < taps - 1; ++t)
      {
        c = c * (taps - 1 - t) / (t + 1);
        wk[t + 1] = (float)c;
      }
      const int step = (int)GTVBAO_filter_step;
      for (int j = -half_taps; j <= half_taps; ++j)
      {
        for (int i = -half_taps; i <= half_taps; ++i)
        {
          FilterTap(px + int2(i, j) * step, wk[i + half_taps] * wk[j + half_taps], i == 0 && j == 0,
                    size, full_size, consts, Pc, Nc, dist_c, sum_w, sum_v);
        }
      }
    }
    if (sum_w > 0.0) result = sum_v / sum_w;
  }
  g_out_ao[px] = (uint)(saturate(result * output_scale) * 255.0 + 0.5);
}
