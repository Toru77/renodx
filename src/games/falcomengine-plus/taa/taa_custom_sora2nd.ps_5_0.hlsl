// Custom TAA replacement for Sora 2nd (game hash 0x9D91FAC3).
//
// From-scratch implementation; the dumped vanilla reference at
// sora2nd/taa/taa_0x9D91FAC3.ps_5_0.hlsl is untouched. Only the verified
// external interface is shared: b2 cb_taa, s0/s1, t0 color, t1 depth,
// t2 history, t3 motion (texel units), texelSize/prevResolutionScale.
//
// OFF (Custom TAA toggle): this file never serves; the game draws vanilla.
// ON: reproject -> reconstruct -> validate -> fixed-feedback accumulate,
// with first-frame history reset (never accumulate stale vanilla history).
//
// NOTE: unique embed stem (taa_custom_sora2nd) so the vanilla reference file
// keeps compiling untouched; the runtime CRC registered in addon.cpp is the
// game hash above.

#include "taa_common.hlsli"

cbuffer cb_taa : register(b2) {
  float2 jitter_g : packoffset(c0);
  float2 texelSize_g : packoffset(c0.z);
  float staticBlending_g : packoffset(c1);
  float dynamicBlending_g : packoffset(c1.y);
  float motionSensitivity_g : packoffset(c1.z);
  float sharpness_g : packoffset(c1.w);
  float2 prevResolutionScale_g : packoffset(c2);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> colorTexture : register(t0);
Texture2D<float4> depthTexture : register(t1);
Texture2D<float4> historyTexture : register(t2);
Texture2D<float4> motionTexture : register(t3);

void main(
    float4 v0 : SV_Position0,
    float4 v1 : TEXCOORD0,
    out float4 o0 : SV_Target0) {
  float2 uv = v1.xy;
  float2 texelSize = texelSize_g.xy;

  // --- Reproject (game coordinate convention, custom code) ---
  float motionPixels = 0.0;
  float depthSpread = 0.0;
  float2 historyUV = CustomTAA_Reproject(
      uv, depthTexture, samPoint_s, motionTexture, samLinear_s,
      texelSize, prevResolutionScale_g.xy, motionPixels, depthSpread);

  float3 current = colorTexture.SampleLevel(samLinear_s, uv, 0).xyz;

  // --- History reset: first custom frame outputs current only ---
  if (shader_injection_data.custom_taa_history_valid < 0.5) {
    o0 = float4(clamp(current, 0.0, 65472.0), 1.0);
    return;
  }

  // --- Reconstruct history ---
  int filterMode = (int)(shader_injection_data.custom_taa_history_filter + 0.5);
  float3 history;
  if (filterMode == CUSTOM_TAA_FILTER_FULL) {
    history = CustomTAA_SampleBicubicFull(historyUV, texelSize, historyTexture, samPoint_s);
  } else if (filterMode == CUSTOM_TAA_FILTER_ADAPTIVE) {
    history = CustomTAA_SampleAdaptive(
        historyUV, texelSize, shader_injection_data.custom_taa_dmin,
        historyTexture, samPoint_s, samLinear_s);
  } else {
    history = CustomTAA_SampleBilinear(historyUV, historyTexture, samLinear_s);
  }
  if (!isfinite(dot(history, history))) {
    history = current;
  }

  float debugMode = shader_injection_data.custom_taa_debug;
  if (debugMode > 0.5 && debugMode < 1.5) {
    o0 = float4(clamp(history, 0.0, 65472.0), 1.0);
    return;
  }
  if (debugMode > 2.5) {
    // Filter-difference diagnostic: answers "are the filters producing
    // different samples, and where?" Compares raw history samples (same
    // historyUV, pre-clip, pre-accumulation). Relative normalized form so
    // small linear-HDR differences survive the downstream tonemap:
    // black = effectively identical, brighter = larger difference.
    // Fixed x8 gain (no user slider); 1% -> ~0.08, 5% -> ~0.4, >=12.5% -> 1.
    float3 bilinearRef = CustomTAA_SampleBilinear(historyUV, historyTexture, samLinear_s);
    float3 otherRef = (debugMode < 3.5)
        ? CustomTAA_SampleAdaptive(historyUV, texelSize, shader_injection_data.custom_taa_dmin,
                                   historyTexture, samPoint_s, samLinear_s)
        : CustomTAA_SampleBicubicFull(historyUV, texelSize, historyTexture, samPoint_s);
    float3 absDiff = abs(otherRef - bilinearRef);
    float3 denom = max(max(abs(otherRef), abs(bilinearRef)), float3(1e-4, 1e-4, 1e-4));
    float3 relDiff = absDiff / denom;
    float relMax = max(relDiff.x, max(relDiff.y, relDiff.z));
    o0 = float4(saturate(relMax * 8.0).xxx, 1.0);
    return;
  }

  // --- Validate against the current 3x3 neighborhood ---
  float clipT = 1.0;
  float overshootR = 0.0;
  // Detail gate for feedback restoration (experiment): HDR-safe relative
  // detail from the already-gathered 3x3 (zero new taps). Computed inside this
  // block where the neighborhood array is live; stays 0 when validation is
  // off (no agreement signal there). Default 0 = exact prior behavior.
  float detailGate = 0.0;
  int clipMode = (int)(shader_injection_data.custom_taa_clip_mode + 0.5);
  if (clipMode != CUSTOM_TAA_CLIP_NONE) {
    float3 neighborhood[9];
    int n = 0;
    [unroll]
    for (int oy = -1; oy <= 1; ++oy) {
      [unroll]
      for (int ox = -1; ox <= 1; ++ox) {
        float2 tapUV = saturate(uv + float2(ox, oy) * texelSize);
        neighborhood[n++] = colorTexture.SampleLevel(samPoint_s, tapUV, 0).xyz;
      }
    }
    if (clipMode == CUSTOM_TAA_CLIP_KDOP) {
      int setSel = (int)(shader_injection_data.custom_taa_kdop_axes + 0.5);
      history = CustomTAA_ClipKDOP(
          current, history, neighborhood, setSel,
          shader_injection_data.custom_taa_kdop_epsilon, clipT, overshootR);
    } else {
      float3 minC = neighborhood[0];
      float3 maxC = neighborhood[0];
      [unroll]
      for (int k = 1; k < 9; ++k) {
        minC = min(minC, neighborhood[k]);
        maxC = max(maxC, neighborhood[k]);
      }
      history = CustomTAA_ClipToExtents(current, history, minC, maxC, clipT, overshootR);
    }
    // Relative detail: max channel range over center luma (vanilla luma
    // weights, 1e-2 floor against near-black noise). Bright smooth regions
    // read low even when their absolute range is large; dark textured detail
    // reads high. Test value 8.0 opens fully at relativeDetail 0.125.
    float3 nMin = neighborhood[0];
    float3 nMax = neighborhood[0];
    [unroll]
    for (int q = 1; q < 9; ++q) {
      nMin = min(nMin, neighborhood[q]);
      nMax = max(nMax, neighborhood[q]);
    }
    float detailRange = max(nMax.x - nMin.x, max(nMax.y - nMin.y, nMax.z - nMin.z));
    float refLuma = max(dot(current, float3(0.299, 0.587, 0.114)), 1e-2);
    detailGate = saturate((detailRange / refLuma) * shader_injection_data.custom_taa_detail_restore);
  }

  if (shader_injection_data.custom_taa_debug > 1.5) {
    // CUSTOM_TAA_DEBUG_CLIP: visualize validated fraction (white = kept).
    o0 = float4(clipT.xxx, 1.0);
    return;
  }

  // --- Accumulate with motion-adaptive feedback (TAA-5) ---
  // Static pixels keep many frames of history; moving pixels fall toward
  // dynamicFeedback to cut trailing/temporal softness. Independent of the
  // vanilla static/dynamic/motionSensitivity equations by design.
  float motionWeight = saturate(motionPixels * shader_injection_data.custom_taa_motion_scale);
  // Squared-response A/B: same endpoints (0 -> static, saturated -> dynamic),
  // more history at low/moderate motion when enabled. Single variable, so
  // silhouette rejection softens mid-curve too -- judge ghosting with that
  // coupling in mind.
  if (shader_injection_data.custom_taa_squared_motion_response > 0.5) {
    motionWeight = motionWeight * motionWeight;
  }
  float adaptiveFeedback = lerp(shader_injection_data.custom_taa_static_feedback,
                                shader_injection_data.custom_taa_dynamic_feedback,
                                motionWeight);
  // Overshoot-softened feedback (selective-tolerance experiment): history just
  // outside the hull (flicker, r~0) keeps its feedback and averages out;
  // history far outside (genuine change, r>>1) loses feedback and the output
  // tracks the responsive current. k=100 approximates prior behavior (not
  // bit-exact: the divisor is never exactly 1). Clip boundary itself untouched.
  float softK = max(shader_injection_data.custom_taa_overshoot_softness, 1e-4);
  float overshootScale = 1.0 / (1.0 + (overshootR / softK) * (overshootR / softK));
  // Silhouette rejection (wrong-surface history experiment): depth
  // discontinuity alone never rejects (static detail keeps history); it only
  // attenuates feedback in proportion to motion. kSpread=0 reproduces prior
  // behavior exactly. Screen-edge term deliberately not included.
  float disocc = 1.0 - motionWeight * saturate(depthSpread * shader_injection_data.custom_taa_silhouette_rejection);
  // Restoration requires history agreement (overshootScale), so
  // detailed-but-changed pixels are NOT boosted.
  float restoreGate = detailGate * overshootScale;
  float feedback = clamp(adaptiveFeedback * overshootScale * disocc, 0.0, 0.99);
  feedback = lerp(feedback, shader_injection_data.custom_taa_static_feedback, saturate(restoreGate));
  // Selective above-static accumulation (experiment): high-detail, low-motion,
  // history-agreeing pixels approach detailTarget (~20-frame window) without
  // giving that window to the whole image. All gates must agree -- detail,
  // validation (overshootScale AND silhouette disocc), stillness -- and the
  // branch is skipped entirely at default (target == static), preserving exact
  // prior behavior. Silhouette rejection is never bypassed: disocc attenuates
  // the base feedback first and also closes this gate.
  float detailTarget = shader_injection_data.custom_taa_detail_target;
  if (detailTarget > shader_injection_data.custom_taa_static_feedback) {
    float boostGate = detailGate * overshootScale * disocc * (1.0 - motionWeight);
    feedback = lerp(feedback, min(detailTarget, 0.99), saturate(boostGate));
  }
  float3 result = lerp(current, history, feedback);
  o0 = float4(clamp(result, 0.0, 65472.0), 1.0);
}
