// ── Motion Vector Generation: Depth Reprojection ──
// Compute shader (cs_5_0) that generates R16G16_FLOAT velocity from
// depth + camera matrices passed via push constants.
//
// Input:  t0 = depth texture (R32_FLOAT or R32_TYPELESS)
// Output: u0 = velocity buffer (R16G16_FLOAT)
//
// Push constants (b13, 32 floats = 8 float4s):
//   c[0..3]  = prevViewProjection (4x4 row-major)
//   c[4..7]  = curViewProjInverse (4x4 row-major)
//   c[8]     = VP_WIDTH, VP_HEIGHT, VELOCITY_SCALE, DEBUG_VIEW
//   c[9]     = JITTER_X, JITTER_Y, 0, 0
//
// SPDX-License-Identifier: MIT

Texture2D<float> g_srcDepth : register(t0);
RWTexture2D<float2> g_outVelocity : register(u0);

cbuffer cb_push : register(b13) {
  float4x4 prevViewProj;
  float4x4 curViewProjInv;
  float4 params0;   // x=vp_w, y=vp_h, z=velocity_scale, w=debug_view
  float4 params1;   // x=jitter_x, y=jitter_y
};

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
  uint w = (uint)params0.x;
  uint h = (uint)params0.y;
  if (pix.x >= w || pix.y >= h) return;

  // Read depth at pixel center
  float depth = g_srcDepth.Load(int3(pix, 0));

  // NDC from pixel position
  float2 ndc = (float2(pix) + 0.5) / float2(w, h);
  ndc = ndc * 2.0 - 1.0;
  ndc.y = -ndc.y;

  // Unproject to world space
  float4 clipPos = float4(ndc, depth, 1.0);
  float4 worldPos = mul(clipPos, curViewProjInv);
  worldPos /= worldPos.w;

  // Reproject to previous screen
  float4 prevClip = mul(worldPos, prevViewProj);
  float2 prevNDC = prevClip.xy / prevClip.w;

  // Motion vector in output-res pixel units, subtract camera jitter
  float2 motion = (ndc - prevNDC) * float2(w, h) * 0.5;
  motion -= float2(params1.x * w, params1.y * h);

  float2 vel = motion * params0.z;

  if (params0.w > 0.5f) {
    float3 viz = float3(vel * 0.02 * 0.5 + 0.5, 0.0);
    viz = saturate(viz);
    if (length(vel) < 0.5) viz = float3(0.15, 0.15, 0.15);
    g_outVelocity[pix] = float2(viz.r, viz.g);
  } else {
    g_outVelocity[pix] = vel;
  }
}
