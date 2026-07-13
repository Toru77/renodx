///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Pass 2: Main GTAO (Medium quality — 18 spp)
//
// Kai-style: builds GTAOConstants in-shader, MRT normal from game g-buffer.
// Visibility Bitmask AO + optional GI (Therrien/Levesque/Gilet 2023).
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GT_VBAO_COMPUTE_GI
#include "gtvbao_common.hlsl"

Texture2D<lpfloat>     g_srcWorkingDepth   : register(t0);
Texture2D<uint4>       g_srcMrtNormal      : register(t1);
SamplerState           g_samplerPointClamp : register(s0);

// ── GI bindings (main pass: t2 = HDR light buffer) ──
Texture2D<float4>      g_srcLightBuffer    : register(t2);
SamplerState           g_samplerLightBuffer : register(s1);

// ── IS-FAST noise (t3 = 3D noise texture / unused when off) ──
Texture3D<float2>      g_isfastNoiseTexture : register(t3);

RWTexture2D<uint>          g_outWorkingAOTerm : register(u0);
RWTexture2D<float>         g_outWorkingEdges  : register(u1);
RWTexture2D<float4>        g_outGI            : register(u2);
RWTexture2D<float4>        g_outDebug         : register(u3);

// ── Interleaved Gradient Noise (IGN) — IS-FAST fallback ──
static float IGN(float2 p) {
  return frac(52.9829189 * frac(0.06711056 * p.x + 0.00583715 * p.y));
}

static lpfloat2 SpatioTemporalNoise_Hilbert(uint2 p, uint t)
{
  uint i = HilbertIndex(p.x % XE_HILBERT_WIDTH, p.y % XE_HILBERT_WIDTH) + 288u * (t % 64u);
  return lpfloat2(frac(0.5 + (float)i * 0.75487766624669276005),
                  frac(0.5 + (float)i * 0.5698402909980532659114));
}

static lpfloat2 SpatioTemporalNoise_ISFAST(uint2 p, uint t)
{
  if (g_isfast_texture_loaded > 0.5f) {
    float3 uvw = float3(
      (float)(p.x % 128u) / 128.0,
      (float)(p.y % 128u) / 128.0,
      (float)((t + (uint)g_isfast_seed_offset) % 32u) / 32.0);
    uvw.xy *= g_isfast_spatial_scale;
    uvw.z *= g_isfast_temporal_speed;
    float2 s = g_isfastNoiseTexture.SampleLevel(g_samplerPointClamp, uvw, 0);
    return (lpfloat2)(s * g_isfast_strength);
  } else {
    static const float R2_A1 = 0.7548776662466927;
    static const float R2_A2 = 0.5698402909980532;
    float b1 = IGN(float2(p) * g_isfast_spatial_scale + g_isfast_seed_offset);
    float b2 = IGN(float2(p) * g_isfast_spatial_scale + float2(47, 17) + g_isfast_seed_offset);
    return lpfloat2(frac(b1 + R2_A1 * (float)t * g_isfast_temporal_speed),
                    frac(b2 + R2_A2 * (float)t * g_isfast_temporal_speed))
         * g_isfast_strength;
  }
}

// ── Interleaved Gradient Noise + R2 temporal (standalone, no texture needed) ──
static lpfloat2 SpatioTemporalNoise_IGN(uint2 p, uint t)
{
  static const float R2_A1 = 0.7548776662466927;
  static const float R2_A2 = 0.5698402909980532;
  float b1 = IGN(float2(p) * g_isfast_spatial_scale + g_isfast_seed_offset);
  float b2 = IGN(float2(p) * g_isfast_spatial_scale + float2(47, 17) + g_isfast_seed_offset);
  return lpfloat2(frac(b1 + R2_A1 * (float)t * g_isfast_temporal_speed),
                  frac(b2 + R2_A2 * (float)t * g_isfast_temporal_speed))
       * g_isfast_strength;
}

float3 DepthNormal(uint2 p, float2 u, lpfloat z, lpfloat l, lpfloat r,
                   lpfloat t, lpfloat b, lpfloat4 e, GTAOConstants consts)
{
  float3 C = GTVBAO_ComputeViewspacePosition(u, z, consts);
  float3 L = GTVBAO_ComputeViewspacePosition(u + float2(-1, 0) * consts.ViewportPixelSize, l, consts);
  float3 R = GTVBAO_ComputeViewspacePosition(u + float2( 1, 0) * consts.ViewportPixelSize, r, consts);
  float3 T = GTVBAO_ComputeViewspacePosition(u + float2( 0,-1) * consts.ViewportPixelSize, t, consts);
  float3 B = GTVBAO_ComputeViewspacePosition(u + float2( 0, 1) * consts.ViewportPixelSize, b, consts);
  return (float3)GTVBAO_CalculateNormal(e, C, L, R, T, B);
}

float3 SafeNormalize3(float3 v, float3 fallback)
{
  float len2 = dot(v, v);
  return (len2 < 1e-5) ? fallback : v * rsqrt(len2);
}

float ComputeDepthEdgeMetric(uint2 pix, GTAOConstants consts)
{
  int2 size = max(consts.ViewportSize, int2(1, 1));
  int2 tc = int2(pix);
  float c = g_srcWorkingDepth.Load(int3(clamp(tc, int2(0,0), size-1), 0));
  float l = g_srcWorkingDepth.Load(int3(clamp(tc+int2(-1,0), int2(0,0), size-1), 0));
  float r = g_srcWorkingDepth.Load(int3(clamp(tc+int2(1,0), int2(0,0), size-1), 0));
  float t = g_srcWorkingDepth.Load(int3(clamp(tc+int2(0,-1), int2(0,0), size-1), 0));
  float b = g_srcWorkingDepth.Load(int3(clamp(tc+int2(0,1), int2(0,0), size-1), 0));
  float delta = max(max(abs(l-c), abs(r-c)), max(abs(t-c), abs(b-c)));
  return saturate(delta * 1.0);
}

float3 DecodeMrtNormalAsIs(uint2 texel)
{
  uint4 sample = g_srcMrtNormal.Load(int3(texel, 0));
  float2 enc = float2((float)sample.x, (float)sample.y) * (1.0 / 32767.5) + float2(-1.0, -1.0);
  float azimuth = 3.14159274 * enc.x;
  float sin_a, cos_a;
  sincos(azimuth, sin_a, cos_a);
  float ring = sqrt(saturate(1.0 - enc.y * enc.y));
  float3 n = float3(cos_a * ring, sin_a * ring, enc.y);
  return SafeNormalize3(n, float3(0, 0, 1));
}

float3 TransformNormalToView(float3 decoded)
{
  // 0=view_g (default), 1=viewInv_g, 2=passthrough
  float3x3 m = (float3x3)view_g;
  if (GTVBAO_normal_transform_mode > 1.5) return SafeNormalize3(decoded, float3(0, 0, 1));
  if (GTVBAO_normal_transform_mode > 0.5) m = (float3x3)viewInv_g;
  float3 vn = mul(m, decoded);
  return SafeNormalize3(vn, float3(0, 0, 1));
}

float3 BuildDepthFallbackNormal(uint2 pix, GTAOConstants consts)
{
  float2 u = (float2(pix) + 0.5) * consts.ViewportPixelSize;
  float4 ul = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pix) * consts.ViewportPixelSize);
  float4 br = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pix) * consts.ViewportPixelSize, int2(1,1));
  return DepthNormal(pix, u, ul.y, ul.x, br.z, ul.z, br.x,
      GTVBAO_CalculateEdges(ul.y, ul.x, br.z, ul.z, br.x), consts);
}

float3 BuildSelectedInputNormal(uint2 pix, uint2 working_size, GTAOConstants consts)
{
  float3 depth_fallback = BuildDepthFallbackNormal(pix, consts);
  float3 selected = depth_fallback;

  if (GTVBAO_normal_input_mode < 0.5) return selected;
  if (GTVBAO_mrt_normal_available < 0.5) return selected;

  uint mw, mh;
  g_srcMrtNormal.GetDimensions(mw, mh);
  if (mw == 0 || mh == 0) return selected;

  float2 scale = float2(mw, mh) / max(float2(working_size), 1.0.xx);
  int2 mrt_tc = min(int2(floor((float2(pix) + 0.5) * scale)), int2(mw-1, mh-1));

  float3 decoded = DecodeMrtNormalAsIs((uint2)mrt_tc);
  if (dot(decoded, decoded) < 1e-5) return selected;

  float3 mrt_normal = TransformNormalToView(decoded);
  float3 tuned = mrt_normal;
  tuned.xy *= max(0.0, GTVBAO_normal_influence);
  tuned.z  *= max(0.0, GTVBAO_normal_z_preservation);
  tuned = SafeNormalize3(tuned, mrt_normal);

  float sharpness = max(0.01, GTVBAO_normal_sharpness);
  float base_blend = pow(saturate(GTVBAO_normal_depth_blend), 1.0 / sharpness);
  float edge_metric = ComputeDepthEdgeMetric(pix, consts);
  float edge_att = 1.0 - saturate(edge_metric * max(0.0, GTVBAO_normal_edge_rejection));
  float normal_delta = 1.0 - saturate(dot(depth_fallback, tuned));
  float detail_response = max(0.01, GTVBAO_normal_detail_response);
  float detail_gain = lerp(0.35, 1.25, pow(normal_delta, 1.0 / detail_response));
  float final_blend = saturate(base_blend * edge_att * detail_gain);
  if (GTVBAO_normal_darkening_mode < 0.5)
    final_blend *= saturate(GTVBAO_normal_max_darkening);

  return SafeNormalize3(lerp(depth_fallback, tuned, final_blend), depth_fallback);
}

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(uint2 pixCoord : SV_DispatchThreadID)
{
  uint width, height;
  g_srcWorkingDepth.GetDimensions(width, height);
  if (pixCoord.x >= width || pixCoord.y >= height) return;

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  uint noise_idx = consts.NoiseIndex < 0 ? 0u : (uint)consts.NoiseIndex;
  lpfloat2 n;
  if (g_isfast_enabled > 0.5f) {
    if (GTVBAO_noise_type < 0.5f)      n = SpatioTemporalNoise_ISFAST(pixCoord, noise_idx);
    else if (GTVBAO_noise_type < 1.5f) n = SpatioTemporalNoise_IGN(pixCoord, noise_idx);
    else                               n = SpatioTemporalNoise_Hilbert(pixCoord, noise_idx);
  } else {
    n = SpatioTemporalNoise_Hilbert(pixCoord, noise_idx);
  }

  lpfloat3 normal = (lpfloat3)BuildSelectedInputNormal(pixCoord, uint2(width, height), consts);

  // Quality from push constant
  lpfloat slice_count = 3.0;
  lpfloat steps_per_slice = 3.0;
  uint q = (uint)round(clamp(GTVBAO_quality, 0.0, 3.0));
  if (q == 0u)      { slice_count = 3.0; steps_per_slice = 3.0; }  // Low
  else if (q == 1u) { slice_count = 5.0; steps_per_slice = 3.0; }  // Medium
  else if (q == 2u) { slice_count = 8.0; steps_per_slice = 3.0; }  // High
  else              { slice_count = 10.0; steps_per_slice = 3.0; }  // Ultra

  GTVBAO_MainPass(pixCoord, slice_count, steps_per_slice, n, normal,
      consts, g_srcWorkingDepth, g_samplerPointClamp,
      g_outWorkingAOTerm, g_outWorkingEdges,
      g_srcMrtNormal,
      (g_gi_enabled > 0.5f), g_gi_intensity,
      g_srcLightBuffer, g_samplerLightBuffer,
      g_outGI,
      g_outDebug);
}
