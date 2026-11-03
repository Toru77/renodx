#ifndef SRC_GAMES_FALCOMENGINE_PLUS_GTVBAO_VBGI_FAR_PASS_HLSLI_
#define SRC_GAMES_FALCOMENGINE_PLUS_GTVBAO_VBGI_FAR_PASS_HLSLI_
#define GTVBAO_GI 1
#define VBGI_FAR_PASS 1
#include "gtvbao_common.hlsl"
#include "vbgi_far.hlsli"
#include "../taa/taa_reproject.hlsli"
Texture2D<lpfloat> g_farDepth    : register(t0);   // linear view depth mips
Texture2D<uint4>   g_farMrt      : register(t1);   // MRT normal
Texture2D<float4>  g_farNearRaw  : register(t2);   // near raw GI (resolve reads .rgb and .a)
Texture2D<float4>  g_farNdcDepth : register(t3);   // game depth, full res (temporal reprojection)
// t4 = g_srcRadiance (gtvbao_common.hlsl)
Texture2D<float4>  g_farMotion   : register(t5);   // RTV4 motion, texel units
Texture2D<float4>  g_farIn0      : register(t6);   // input pos
Texture2D<float4>  g_farIn1      : register(t7);   // input rad
Texture2D<float4>  g_farIn2      : register(t8);   // input nrm
Texture2D<float4>  g_farPrev0    : register(t9);   // previous temporal reservoir pos
Texture2D<float4>  g_farPrev1    : register(t10);
Texture2D<float4>  g_farPrev2    : register(t11);
SamplerState       g_farSamp     : register(s0);   // point clamp
RWTexture2D<float4> g_farOut0    : register(u0);   // output pos (resolve: joint raw GI)
RWTexture2D<float4> g_farOut1    : register(u1);   // output rad (resolve: far raw GI)
RWTexture2D<float4> g_farOut2    : register(u2);   // output nrm
RWTexture2D<float4> g_farDebug   : register(u3);   // full-res debug target
// View-space visible point (main pass 0.9992 nudge), view normal and screen uv of trace texel t; false for sky or no normal.
bool VbgiFarSurface(int2 t, GTAOConstants consts, out float3 P, out float3 N, out float2 uv)
{
  uint fw, fh, mw, mh;
  g_farDepth.GetDimensions(fw, fh);
  g_farMrt.GetDimensions(mw, mh);
  uv = (float2(t) + 0.5f) * consts.ViewportPixelSize;
  const float z = g_farDepth.Load(int3(GTVBAO_RepresentativePixel(t, int2(fw, fh)), 0)).x;
  const float3 nw = DecodeFalcomMrtNormal(g_farMrt.Load(int3(GTVBAO_MrtTexel(uv * float2(mw, mh), float2(mw, mh)), 0)).xy);
  N = TransformNormalToView(nw);
  P = GTVBAO_ComputeViewspacePosition(uv, z * 0.99920f, consts);
  return z < 60000.0f && FalcomNormalValid(nw);
}
#endif
