// ── MV Debug: velocity -> readable visualization (4 modes) ──
//   mode 1 (HSV):       hue = direction, value = speed      (best all-around)
//   mode 2 (Arrows):    oriented arrows on a 32px grid
//   mode 3 (Magnitude): grayscale speed
//   mode 4 (Reproj):    reprojection error vs previous frame (black = perfect)
// Push constants (b13): (mode, scale, 0, 0)
// SPDX-License-Identifier: MIT

Texture2D<float2> g_srcVelocity : register(t0);
Texture2D<float4> g_srcCurrent : register(t1);
Texture2D<float4> g_srcPrev : register(t2);
Texture2D<float> g_srcDepth : register(t3);
SamplerState g_samplerLinear : register(s0);
RWTexture2D<float4> g_outColor : register(u0);

cbuffer cb_push : register(b13) {
  float4 params0;   // x = mode, y = scale
};

float3 HsvToRgb(float3 hsv) {
  float h = frac(hsv.x) * 6.0;
  float c = hsv.z * hsv.y;
  float x = c * (1.0 - abs(frac(h) * 2.0 - 1.0));
  float3 rgb = (h < 1.0) ? float3(c, x, 0)
            : (h < 2.0) ? float3(x, c, 0)
            : (h < 3.0) ? float3(0, c, x)
            : (h < 4.0) ? float3(0, x, c)
            : (h < 5.0) ? float3(x, 0, c)
            : float3(c, 0, x);
  return rgb + hsv.z - c;
}

// Direction -> hue, speed -> value (saturation = 1).
float3 DirectionColor(float2 vel, float scale) {
  float speed = length(vel);
  float value = saturate(speed * scale);
  float hue = 0.0;
  if (speed > 1e-4) hue = atan2(vel.y, vel.x) / 6.2831853 + 0.5;  // [0,1)
  return HsvToRgb(float3(hue, 1.0, value));
}

float DistToSegment(float2 p, float2 a, float2 b) {
  float2 ab = b - a;
  float t = saturate(dot(p - a, ab) / dot(ab, ab));
  return length((p - a) - ab * t);
}

[numthreads(8, 8, 1)]
void main(uint2 pix : SV_DispatchThreadID)
{
  float2 vel = g_srcVelocity[pix];
  float scale = max(params0.y, 0.001f);
  int mode = (int)params0.x;

  if (mode == 1) {
    // HSV: hue = direction, value = speed
    g_outColor[pix] = float4(DirectionColor(vel, scale), 1.0);
  } else if (mode == 2) {
    // Arrows on a 32px grid, oriented by the local velocity.
    const float GRID = 32.0;
    float2 anchor = (floor(pix / GRID) + 0.5) * GRID;
    float2 avel = g_srcVelocity.Load(int3((int2)anchor, 0));
    float3 col = float3(0, 0, 0);
    float aspeed = length(avel);
    if (aspeed > 1e-3) {
      float2 dir = avel / aspeed;
      float2 perp = float2(-dir.y, dir.x);
      float len = clamp(aspeed * scale, 2.0, GRID * 0.8);
      float2 tip = anchor + dir * len;
      float2 headA = tip - dir * (len * 0.25) + perp * (len * 0.18);
      float2 headB = tip - dir * (len * 0.25) - perp * (len * 0.18);
      float2 p = float2(pix) + 0.5;
      float d = min(DistToSegment(p, anchor, tip),
                min(DistToSegment(p, tip, headA), DistToSegment(p, tip, headB)));
      if (d < 1.5) {
        col = DirectionColor(avel, scale);
        col = col * 0.7 + 0.3;  // keep arrows visible
      }
    }
    g_outColor[pix] = float4(col, 1.0);
  } else if (mode == 3) {
    // Magnitude grayscale
    float m = saturate(length(vel) * scale);
    g_outColor[pix] = float4(m, m, m, 1.0);
  } else if (mode == 5) {
    // Depth: grayscale of the captured depth buffer (near=white for reverse-Z).
    // A correct camera depth looks like a smooth depth map (objects shaded by
    // distance). A flat/weird/linearized gradient means we're sampling the WRONG
    // depth (post-process/DOF/linear) instead of the raw Z/W camera depth.
    float d = saturate(g_srcDepth.Load(int3(pix, 0)));
    g_outColor[pix] = float4(d, d, d, 1.0);
  } else {
    // Reprojection error: black = perfect, white = error.
    uint w, h;
    g_srcCurrent.GetDimensions(w, h);
    float2 uv = (float2(pix) + 0.5) / float2(w, h);
    float2 prevUv = uv - vel / float2(w, h);
    float3 cur = g_srcCurrent.Load(int3(pix, 0)).rgb;
    float3 prev = g_srcPrev.SampleLevel(g_samplerLinear, prevUv, 0).rgb;
    float e = saturate(max(max(abs(cur.r - prev.r), abs(cur.g - prev.g)), abs(cur.b - prev.b)) * scale * 0.05f);
    g_outColor[pix] = float4(e, e, e, 1.0);
  }
}
