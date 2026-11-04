// Bend sun setup: light coordinate and dispatch list for the sun, as float4(direction to light, 0) * viewProj_g, over
// the whole depth target. No sun this frame writes zero dispatches, so the cleared (lit) term stays.
#define RENODX_SHADOWS_DECLARE_CB_SCENE
#include "shadows_common.hlsli"
#include "bend_dispatch.hlsli"

Texture2D<float4> g_unusedDepth : register(t0);  // bound so the layout matches the shadow pass; not read
RWBuffer<uint> g_outDispatch : register(u0);

[numthreads(1, 1, 1)]
void main()
{
  const float3 toLight = FalcomDirectionToLight();
  const float2 viewport = float2(shader_injection_data.cs_working_w, shader_injection_data.cs_working_h);
  BendWriteDispatchList(g_outDispatch, 0u, FalcomNormalValid(toLight), mul(float4(toLight, 0.0), viewProj_g),
                        float4(toLight, 0.0), viewport, int2(0, 0), int2(viewport));
}
