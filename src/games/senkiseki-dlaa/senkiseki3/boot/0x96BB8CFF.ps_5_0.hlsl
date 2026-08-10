// ── Modified 0x96BB8CFF (FXAA) — the game's OWN native luma FXAA, fixed ──
// Faithful transcription of the game's ORIGINAL FXAA (0x96BB8CFF_ref.ps_5_0.hlsl):
// same contrast gate, same edge gradient, same 9-step edge search, same final
// blend. The ONLY change is the LUMA SOURCE.
//
// The game's composite pass (0xE8C7EBA2) writes luma to the ALPHA channel:
//     o0.w = dot(rgb, float3(0.299, 0.587, 0.114))
// The game's FXAA is DESIGNED to read that native luma from `.w`. The original
// bytecode broke because its 4 neighbor samples came from Gather() = RED channel
// while the center/diagonal samples came from ALPHA — a mixed red/alpha luma
// that produced a meaningless gradient (≈ passthrough). Here every one of the 7
// luma values is read from the ALPHA channel (`.w`), keeping the game's exact
// math and making it consistent, so the contrast check fires and real edges are
// found and smoothed.
//
// 3-way AA mode (shader_injection b13):
//   mode 0 (Off)  → passthrough (copy t0 → RTV0, no AA)
//   mode 1 (FXAA) → the game's native luma FXAA
//   mode 2 (DLAA) → passthrough (DLAA already anti-aliased t0)
//
// SPDX-License-Identifier: MIT

#include "../../shared.h"

SamplerState LinearClampSampler_s : register(s0);
Texture2D<float4> ColorBuffer : register(t0);

static const float kEdgeThresholdMin = 0.0000001;   // game: max(0.0833, 0.1*maxLuma)
static const float kEdgeThresholdMax = 0.1;
static const float kSubpixLumaScale = 1;            // game (Senkiseki3): 0.25*((3-2s)s²)²

// Native luma: the game's composite 0xE8C7EBA2 writes luma to the ALPHA channel.
float LumaAt(float2 uv) {
  return ColorBuffer.SampleLevel(LinearClampSampler_s, uv, 0).w;
}

void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  // ── 3-way AA mode: Off (0) and DLAA (2) → passthrough ──
  float mode = DLAA_ENABLED;
  if (mode < 0.5f || mode > 1.5f) {
    o0 = ColorBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0);
    o0.w = 1;
    return;
  }

  uint w, h;
  ColorBuffer.GetDimensions(w, h);
  float2 rcpFrame = float2(1.0 / float(w), 1.0 / float(h));

  // ── 7 native luma samples (alpha) at the same texels the game reads ──
  float lumaM  = LumaAt(v1.xy);
  float lumaE  = LumaAt(v1.xy + float2( rcpFrame.x,  0.0));
  float lumaW  = LumaAt(v1.xy + float2(-rcpFrame.x,  0.0));
  float lumaN  = LumaAt(v1.xy + float2( 0.0, -rcpFrame.y));
  float lumaS  = LumaAt(v1.xy + float2( 0.0,  rcpFrame.y));
  float lumaNE = LumaAt(v1.xy + float2( rcpFrame.x, -rcpFrame.y));
  float lumaSW = LumaAt(v1.xy + float2(-rcpFrame.x,  rcpFrame.y));

  // ── Contrast gate (fire on real edges) ──
  float maxLuma = max(lumaM, max(lumaE, max(lumaS, max(lumaN, lumaW))));
  float minLuma = min(lumaM, min(lumaE, min(lumaS, min(lumaN, lumaW))));
  float rangeLuma = maxLuma - minLuma;
  if (rangeLuma < max(kEdgeThresholdMin, maxLuma * kEdgeThresholdMax)) {
    o0 = ColorBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0);
    o0.w = 1;
    return;
  }

  // ── Edge gradient (game: diagonal second-derivative terms) ──
  float h2 = (lumaE + lumaW) - 2.0 * lumaM;   // horizontal 2nd derivative
  float v2 = (lumaN + lumaS) - 2.0 * lumaM;   // vertical 2nd derivative
  float ne_m = lumaNE + lumaM;
  float sw_m = lumaSW + lumaM;
  float gradH = 2.0 * abs(h2) + abs(ne_m - 2.0 * lumaS) + abs(sw_m - 2.0 * lumaN);
  float gradV = 2.0 * abs(v2) + abs(ne_m - 2.0 * lumaW) + abs(sw_m - 2.0 * lumaE);
  float walkX = (gradH >= gradV) ? 1.0 : 0.0;   // walk along X when horizontal gradient dominates

  // ── Subpixel base: center deviation from the 12-tap local average ──
  float sumAll = 2.0 * ((lumaE + lumaW) + (lumaN + lumaS)) + (lumaSW + lumaNE + 2.0 * lumaM);
  float avg12 = sumAll * 0.0833333358;          // /12
  float subpixBase = saturate(abs(avg12 - lumaM) / max(rangeLuma, 1e-6));

  // ── Edge sides on the walk axis ──
  float lumaL = walkX ? lumaW : lumaN;
  float lumaR = walkX ? lumaE : lumaS;
  float deltaL = lumaL - lumaM;
  float deltaR = lumaR - lumaM;
  float lumaBiggerSide = (abs(deltaL) >= abs(deltaR)) ? 1.0 : 0.0;
  float maxDelta = max(abs(deltaL), abs(deltaR));
  float lumaBigger = lumaBiggerSide ? lumaL : lumaR;
  float lumaMid = 0.5 * (lumaBigger + lumaM);   // edge midpoint luma
  float edgeThreshold = 0.25 * maxDelta;
  float sidePositive = (lumaBigger > lumaM) ? 1.0 : 0.0;

  // ── Walk setup: 1-texel step along the edge + 0.5-texel perpendicular bias ──
  float dirScale = walkX ? rcpFrame.y : rcpFrame.x;
  dirScale = lumaBiggerSide ? -dirScale : dirScale;
  float2 stepVec = walkX ? float2(rcpFrame.x, 0.0) : float2(0.0, rcpFrame.y);
  float2 posBase = v1.xy + (walkX ? float2(0.0, 0.5 * dirScale) : float2(0.5 * dirScale, 0.0));
  float2 posN = posBase - stepVec;
  float2 posP = posBase + stepVec;

  // ── Subpixel luma term: 0.25 * ((3-2s) * s²)² ──
  float subpixLuma = (3.0 - 2.0 * subpixBase) * (subpixBase * subpixBase);

  // ── First crossing test at the initial 1-texel positions ──
  float relN = LumaAt(posN) - lumaMid;
  float relP = LumaAt(posP) - lumaMid;
  float crossN = (abs(relN) >= edgeThreshold) ? 1.0 : 0.0;
  float crossP = (abs(relP) >= edgeThreshold) ? 1.0 : 0.0;

  // ── 9-step edge search: 1.5, 2,2,2,2,2, 4,4, 8 ──
  // Advance only the side(s) that haven't crossed the edge midpoint yet; sample
  // after advancing, stop once both sides have crossed (game behavior).
  const float kSteps[9] = { 1.5, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0, 4.0, 8.0 };
  for (int i = 0; i < 9; i++) {
    if (crossN == 0.0) posN -= kSteps[i] * stepVec;
    if (crossP == 0.0) posP += kSteps[i] * stepVec;
    if (crossN != 0.0 && crossP != 0.0) break;
    if (crossN == 0.0) { relN = LumaAt(posN) - lumaMid; crossN = (abs(relN) >= edgeThreshold) ? 1.0 : 0.0; }
    if (crossP == 0.0) { relP = LumaAt(posP) - lumaMid; crossP = (abs(relP) >= edgeThreshold) ? 1.0 : 0.0; }
  }

  // ── Final blend: gated search term vs subpixel luma term ──
  float distN = walkX ? (v1.x - posN.x) : (v1.y - posN.y);
  float distP = walkX ? (posP.x - v1.x) : (posP.y - v1.y);
  float span = distN + distP;
  float minDist = min(distN, distP);
  float relNSign = (relN < 0.0) ? 1.0 : 0.0;
  float relPSign = (relP < 0.0) ? 1.0 : 0.0;
  float dirCheckN = (sidePositive != relNSign) ? 1.0 : 0.0;
  float dirCheckP = (sidePositive != relPSign) ? 1.0 : 0.0;
  float dirBit = (distN < distP) ? dirCheckN : dirCheckP;
  float searchSubpix = (dirBit != 0.0) ? (0.5 - minDist / max(span, 1e-6)) : 0.0;
  float subpix = max(searchSubpix, kSubpixLumaScale * (subpixLuma * subpixLuma));

  // ── Final sample: center + subpix*dirScale (perpendicular to the edge) ──
  float2 uvFinal = v1.xy + (walkX ? float2(0.0, subpix * dirScale) : float2(subpix * dirScale, 0.0));
  o0 = ColorBuffer.SampleLevel(LinearClampSampler_s, uvFinal, 0);
  o0.w = 1;
  return;
}
