// Body of the contact denoise passes: contact_denoise.cs_5_0.hlsl (sun term) and contact_local_denoise.cs_5_0.hlsl
// (local term, CONTACT_DENOISE_LOCAL). One axis of a separable joint bilateral; RunShadows dispatches it horizontally
// into contact_denoise_texture and vertically back into the source target, once per pass.
//
// The sun target filters .x (the term); the local target filters .x (point share) and .z (spot share). The other
// channels are the centre's. A texel whose .y code is not kContactDiagMarched is copied through and is never a tap.
//
// Edge stops: the tap's distance to the centre's tangent plane over depth sigma times the centre's linear depth, and a
// Gaussian on 1 - dot(Nc, Nt) over normal sigma.
//
// Bindings: t0 depth, t1 mrtTexture0, t2 source target (rgba8_unorm), u0 output (same layout).

#define RENODX_SHADOWS_DECLARE_CB_SCENE
#include "shadows_common.hlsli"
#include "../world/shaders/world_rt_denoise.hlsli"
#if defined(CONTACT_DENOISE_LOCAL)
#define CONTACT_DENOISE_RADIUS cs_contact_local_denoise_radius
#define CONTACT_DENOISE_DEPTH_SIGMA cs_contact_local_denoise_depth_sigma
#define CONTACT_DENOISE_NORMAL_SIGMA cs_contact_local_denoise_normal_sigma
#else
#define CONTACT_DENOISE_RADIUS cs_contact_denoise_radius
#define CONTACT_DENOISE_DEPTH_SIGMA cs_contact_denoise_depth_sigma
#define CONTACT_DENOISE_NORMAL_SIGMA cs_contact_denoise_normal_sigma
#endif

Texture2D<float4> g_srcDepth     : register(t0);
Texture2D<uint4>  g_srcMrtNormal : register(t1);
Texture2D<float4> g_srcContact   : register(t2);

RWTexture2D<float4> g_outContact : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  const int2 size = int2((int)shader_injection_data.cs_working_w, (int)shader_injection_data.cs_working_h);
  if (any(int2(p) >= size)) return;

  const float4 centre = g_srcContact.Load(int3(p, 0));
  if ((uint)(centre.y * 255.0 + 0.5) != kContactDiagMarched) {
    g_outContact[p] = centre;
    return;
  }

  const float2 invSize = rcp(float2(size));
  const float centreDeviceDepth = g_srcDepth.Load(int3(p, 0)).x;
  const float centreLinearDepth = FalcomLinearDepth(centreDeviceDepth, FalcomDepthUnpackConsts());
  const float2 centreUv = (float2(p) + 0.5) * invSize;
  const float4 centreWorldH = mul(float4(centreUv.x * 2.0 - 1.0, 1.0 - centreUv.y * 2.0, centreDeviceDepth, 1.0),
                                  viewProjInv_g);
  const float3 centrePos = centreWorldH.xyz / centreWorldH.w;
  const float3 centreNormal = DecodeFalcomMrtNormal(g_srcMrtNormal.Load(int3(p, 0)).xy);

  const int radius = clamp((int)(shader_injection_data.CONTACT_DENOISE_RADIUS + 0.5), 1, 8);
  const float spatialSigma = FilterSeparableSigma((float)radius);
  const float planeScale = rcp(max(shader_injection_data.CONTACT_DENOISE_DEPTH_SIGMA, 1e-4) * centreLinearDepth);
  const float normalScale = rcp(max(shader_injection_data.CONTACT_DENOISE_NORMAL_SIGMA, 1e-3));
  const int2 axis = shader_injection_data.cs_denoise_axis > 0.5 ? int2(0, 1) : int2(1, 0);

  float2 sum = centre.xz;
  float weightSum = 1.0;
  [loop]
  for (int k = -radius; k <= radius; ++k) {
    if (k == 0) continue;
    const int2 q = int2(p) + axis * k;
    if (any(q < 0) || any(q >= size)) continue;
    const float4 tap = g_srcContact.Load(int3(q, 0));
    if ((uint)(tap.y * 255.0 + 0.5) != kContactDiagMarched) continue;

    const float tapDeviceDepth = g_srcDepth.Load(int3(q, 0)).x;
    const float2 tapUv = (float2(q) + 0.5) * invSize;
    const float4 tapWorldH = mul(float4(tapUv.x * 2.0 - 1.0, 1.0 - tapUv.y * 2.0, tapDeviceDepth, 1.0),
                                 viewProjInv_g);
    const float3 tapNormal = DecodeFalcomMrtNormal(g_srcMrtNormal.Load(int3(q, 0)).xy);
    const float plane = dot(centreNormal, tapWorldH.xyz / tapWorldH.w - centrePos) * planeScale;
    const float normalDistance = (1.0 - saturate(dot(centreNormal, tapNormal))) * normalScale;
    const float weight = FilterGaussianWeight(k, spatialSigma)
                       * exp(-0.5 * (plane * plane + normalDistance * normalDistance));
    sum += tap.xz * weight;
    weightSum += weight;
  }

#if defined(CONTACT_DENOISE_LOCAL)
  g_outContact[p] = float4(sum.x / weightSum, centre.y, sum.y / weightSum, centre.w);
#else
  g_outContact[p] = float4(sum.x / weightSum, centre.yzw);
#endif
}
