// gtvbao_upscale.cs_5_0.hlsl - GTVBAO low-resolution upsample to the full AO domain (RTGI upsample structure).
//
// One thread per full-res pixel. Joint bilateral upsample of the AO texel grid (t0 is the final AO byte, 0 = not
// traced): the 2x2 AO texels around the pixel's position in AO space, each weighted by its bilinear weight times
// FilterPlaneNormalWeight (world_rt_denoise.hlsli) between this pixel and the tap's representative pixel
// GTVBAO_RepresentativePixel. With d = 1 it is a pass-through. Writes the full-res AO byte to u0.
// Layout matches the spatial filter: t0 AO, t1 linear depth, t2 view-space normals, u0 output, s0 point clamp (unused).
// Surface test uses FalcomNormalValid (dot > 1e-5), as the filter and temporal passes do, not RTGI's dot >= 0.5.

#include "gtvbao_common.hlsl"
#include "../world/shaders/world_rt_denoise.hlsli"

Texture2D<uint> g_in_ao : register(t0);
Texture2D<float> g_depth : register(t1);
Texture2D<float4> g_normal : register(t2);

RWTexture2D<uint> g_out_ao : register(u0);

SamplerState g_samp_point : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
  uint fw, fh;
  g_depth.GetDimensions(fw, fh);
  const uint2 full_size = uint2(fw, fh);
  if (id.x >= fw || id.y >= fh) return;

  uint aw, ah;
  g_in_ao.GetDimensions(aw, ah);
  const int2 ao_size = int2(aw, ah);
  const GTAOConstants consts = BuildGTAOConstants(full_size);
  const uint d = (uint)GTVBAO_Divisor();
  const int2 px = int2(id.xy);

  float3 Pc;
  float3 Nc;
  if (!GTVBAO_SurfaceAt(g_depth, g_normal, px, full_size, consts, Pc, Nc))
  {
    g_out_ao[id.xy] = 255u;
    return;
  }
  const float dist_c = length(Pc);
  // Without MRT normals (GTVBAO_mrt_normal_available < 0.5) both terms of FilterPlaneNormalWeight are unavailable (the plane term uses the centre normal). Only the surface test remains: taps that are not surfaces by depth are skipped, the rest are weighted bilinearly. Degraded fallback.
  const bool edge_stop = GTVBAO_mrt_normal_available > 0.5f;

  // Position in AO space: AO texel tq is centred on its representative pixel tq * d + d / 2.
  const float2 lp = (float2(px) - float(d / 2u)) / float(d);
  const int2 base = (int2)floor(lp);
  const float2 f = lp - floor(lp);
  float sum_w = 0.0;
  float sum_v = 0.0;
  float near_d = 1e30;
  uint near_byte = 255u;
  [unroll]
  for (int t = 0; t < 4; ++t)
  {
    const int2 offset = int2(t & 1, t >> 1);
    const float w = (offset.x == 1 ? f.x : 1.0 - f.x) * (offset.y == 1 ? f.y : 1.0 - f.y);
    const int2 tq = clamp(base + offset, int2(0, 0), ao_size - 1);
    const uint byte = g_in_ao[tq];
    if (byte == 0u) continue;
    const float2 dl = lp - float2(tq);
    const float dd = dot(dl, dl);
    if (dd < near_d)
    {
      near_d = dd;
      near_byte = byte;
    }
    float3 Pt;
    float3 Nt;
    if (!GTVBAO_RepresentativeSurface(g_depth, g_normal, tq, full_size, consts, Pt, Nt)) continue;
    const float wt = edge_stop ? w * FilterPlaneNormalWeight(Pc, Nc, dist_c, Pt, Nt) : w;
    sum_w += wt;
    sum_v += wt * ((float)byte * (1.0f / 255.0f));
  }

  float ao;
  if (sum_w > 1e-6)
  {
    ao = sum_v / sum_w;
  }
  else
  {
    // Nearest traced AO texel (same byte rule as the loop), so a silhouette does not turn white; 255 if none is traced.
    ao = (float)near_byte * (1.0f / 255.0f);
  }
  g_out_ao[id.xy] = (uint)(saturate(ao) * 255.0 + 0.5);
}
