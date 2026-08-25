///////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus — Custom SSR (Sora 2nd) — Phase R3: filtered radiance pyramid
//
// Pass R3a: base level. Copies the captured HDR scene-color texture (lighting
// input ColorTexture t0, radSrc==0 only) into mip 0 of the RGBA16F radiance
// pyramid, and pre-seeds the reduction scratch buffer (mirrors ssr_hiz_base).
//
// Dispatched with ceil(w/8) x ceil(h/8) x 1 groups.
///////////////////////////////////////////////////////////////////////////////////////

#include "ssr_common.hlsl"

Texture2D<float4>   g_srcColor : register(t0);
RWTexture2D<float4> g_outPyrMip0 : register(u0);
RWTexture2D<float4> g_outScratch : register(u1);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
  uint width, height;
  g_outPyrMip0.GetDimensions(width, height);
  if (dispatchThreadID.x >= width || dispatchThreadID.y >= height) return;

  const float4 c = g_srcColor.Load(int3(dispatchThreadID.xy, 0));
  g_outPyrMip0[dispatchThreadID.xy] = c;
  g_outScratch[dispatchThreadID.xy] = c;
}
