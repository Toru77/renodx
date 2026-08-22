///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Pass 3: Denoise (final pass)
//
// Kai-style: builds GTAOConstants in-shader.
// Extended to denoise GI alongside AO when GT_VBAO_COMPUTE_GI is defined.
//
// Denoiser upgrades (dispatched via GTVBAO_denoise_stage):
//   stage 0 — legacy combined path (spatial/prefilter + temporal in one dispatch)
//   stage 1 — temporal-only (R2: runs FIRST on raw main-pass output)
//             • depth+normal+onscreen validity with smooth rejection ramps (R1b/c/d)
//             • distance-scaled depth threshold (R1c)
//             • variance clamp of history against current 3×3 neighborhood (R1a)
//             • 16-bit fixed-point history packing (R1e/R4) — removes 8-bit feedback quantization
//   stage 2 — spatial-final only (R2: runs LAST; applies OCCLUSION_TERM_SCALE, no temporal)
//   stage 3 — passthrough scale-back only (tail of the à-trous chain)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GT_VBAO_COMPUTE_GI
#include "gtvbao_common.hlsl"

Texture2D<uint>    g_srcWorkingAOTerm : register(t0);
Texture2D<lpfloat> g_srcWorkingEdges  : register(t1);
Texture2D<float4>  g_srcRawGI          : register(t2);  // raw GI from main pass
Texture2D<uint>    g_srcHistoryAO      : register(t3);  // previous frame accumulated AO (16-bit packed)
Texture2D<float>   g_srcDepth          : register(t4);  // viewspace depth MIP0 for reprojection
Texture2D<uint4>   g_mrtNormalTexture  : register(t5);  // MRT g-buffer normals (for Poisson / temporal normal reject)
SamplerState       g_samplerPointClamp : register(s0);
RWTexture2D<uint>  g_outFinalAOTerm   : register(u0);
RWTexture2D<float4> g_outGI            : register(u1);
RWTexture2D<uint>  g_outHistoryAO     : register(u2);  // write history for next frame

// ── R1e/R4: history precision helpers (16-bit fixed point over [0,1]) ──
uint GTVBAO_PackAO16(float v)
{
  return uint(saturate(v) * 65535.0 + 0.5);
}
float GTVBAO_UnpackAO16(uint e)
{
  return float(e & 0xFFFFu) / 65535.0;
}

// ── Local MRT normal decode (mirrors main wrappers; same encoding across games) ──
float3 GTVBAO_DenoiseDecodeMrtNormal(uint2 texel)
{
  uint4 sample = g_mrtNormalTexture.Load(int3(texel, 0));
  float2 enc = float2((float)sample.x, (float)sample.y) * (1.0 / 32767.5) + float2(-1.0, -1.0);
  float azimuth = 3.14159274 * enc.x;
  float sin_a, cos_a;
  sincos(azimuth, sin_a, cos_a);
  float ring = sqrt(saturate(1.0 - enc.y * enc.y));
  float3 n = float3(cos_a * ring, sin_a * ring, enc.y);
  float len = length(n);
  return (len > 1e-4) ? n / len : float3(0, 0, 1);
}

float3 GTVBAO_DenoiseNormalToView(float3 decoded)
{
  // 0=view_g (default), 1=viewInv_g, 2=passthrough
  float3x3 m = (float3x3)view_g;
  if (GTVBAO_normal_transform_mode > 1.5) {
    float3 vn = decoded; float l = length(vn); return (l > 1e-4) ? vn / l : float3(0, 0, 1);
  }
  if (GTVBAO_normal_transform_mode > 0.5) m = (float3x3)viewInv_g;
  float3 vn = mul(m, decoded);
  float l = length(vn);
  return (l > 1e-4) ? vn / l : float3(0, 0, 1);
}

// ── R1a: variance clamp of history against the current-frame 3×3 neighborhood ──
// Returns clamped history value. Neighborhood stats come from raw AO input (t0),
// scaled into the FINAL display domain (×OCCLUSION_TERM_SCALE) to match history.
float GTVBAO_ClampHistoryToNeighborhood(int2 pc, float historyAO)
{
  const float gamma = max(0.0, GTVBAO_ghost_clamp);
  if (gamma <= 0.0) return historyAO;

  uint w, h;
  g_srcWorkingAOTerm.GetDimensions(w, h);

  float mn = 1e9, mx = -1e9, sum = 0.0, sum2 = 0.0;
  [unroll]
  for (int dy = -1; dy <= 1; dy++) {
    [unroll]
    for (int dx = -1; dx <= 1; dx++) {
      int2 npc = clamp(pc + int2(dx, dy), int2(0, 0), int2(w - 1, h - 1));
      float v = (float)g_srcWorkingAOTerm.Load(int3(npc, 0)) * (1.0f / 255.0f);
      mn = min(mn, v); mx = max(mx, v);
      sum += v; sum2 += v * v;
    }
  }
  // Raw domain → final display domain
  const float scale = GT_VBAO_OCCLUSION_TERM_SCALE;
  mn *= scale; mx *= scale;
  const float invN = 1.0 / 9.0;
  float mean = sum * invN * scale;
  float variance = max(0.0, (sum2 * invN) * scale * scale - mean * mean);
  float sd = sqrt(variance);

  // Clamp extent = γ·σ intersected with hard min/max of the neighborhood.
  float lo = max(mn, mean - gamma * sd);
  float hi = min(mx, mean + gamma * sd);
  return clamp(historyAO, lo, hi);
}

// ── GI denoise helpers (unchanged) ──
float GTVBAO_DenoiseGI_EdgeWeight(float centerDepth, float neighborDepth)
{
    float diff = abs(centerDepth - neighborDepth);
    return exp(-diff * 10.0);
}

void GTVBAO_DenoiseGI(uint2 pixCoordBase, GTAOConstants consts,
    Texture2D<float4> srcGI, Texture2D<lpfloat> srcDepth,
    SamplerState samp, RWTexture2D<float4> outGI)
{
    uint w, h;
    srcGI.GetDimensions(w, h);

    for (int side = 0; side < 2; side++)
    {
        int2 pixCoord = int2(pixCoordBase.x + side, pixCoordBase.y);
        if (pixCoord.x >= w || pixCoord.y >= h) continue;

    float2 uv = (float2(pixCoord) + 0.5) * consts.ViewportPixelSize;
    float4 centerGI = srcGI.Load(int3(pixCoord, 0));
    float centerDepth = srcDepth.SampleLevel(samp, uv, 0);

    float4 sum = centerGI;
    float weightSum = 1.0;

    const int2 offsets[8] = {
        int2(-1,-1), int2(0,-1), int2(1,-1),
        int2(-1, 0),            int2(1, 0),
        int2(-1, 1), int2(0, 1), int2(1, 1)
    };

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        int2 nc = clamp(pixCoord + offsets[i], int2(0,0), int2(w-1, h-1));
        float4 neighborGI = srcGI.Load(int3(nc, 0));
        float2 nuv = (float2(nc) + 0.5) * consts.ViewportPixelSize;
        float neighborDepth = srcDepth.SampleLevel(samp, nuv, 0);

        float depthW = GTVBAO_DenoiseGI_EdgeWeight(centerDepth, neighborDepth);
        float colorDiff = length(neighborGI.rgb - centerGI.rgb) / max(length(centerGI.rgb), 0.001);
        float colorW = exp(-colorDiff * 2.0);
        float w = depthW * colorW;

        sum += neighborGI * w;
        weightSum += w;
    }

    outGI[pixCoord] = sum / max(weightSum, 0.001);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Stage 1 — Temporal-only (R2 first stage). Runs on RAW main-pass output.
// Blends current noisy AO with history using improved validity + variance clamp,
// then writes BOTH the working term (8-bit, feeds the spatial chain) and the
// 16-bit history for next frame.
// ─────────────────────────────────────────────────────────────────────────────
void GTVBAO_TemporalFirstStage(uint2 pixCoordBase, GTAOConstants consts)
{
  uint width, height;
  g_srcWorkingAOTerm.GetDimensions(width, height);
  float2 vpSize = float2(width, height);
  float2 invVPSize = 1.0.xx / vpSize;
  uint hw, hh;
  g_srcHistoryAO.GetDimensions(hw, hh);

  [unroll]
  for (int side = 0; side < 2; side++) {
    int2 pixCoord = int2(pixCoordBase.x + side, pixCoordBase.y);
    if (pixCoord.x >= (int)width || pixCoord.y >= (int)height) continue;

    // Current raw AO (/OCCLUSION_TERM_SCALE domain)
    float curRaw = (float)g_srcWorkingAOTerm.Load(int3(pixCoord, 0)) * (1.0f / 255.0f);
    // Final display domain ([0,1], scale applied) — history domain from here on.
    float curFinal = saturate(curRaw * GT_VBAO_OCCLUSION_TERM_SCALE);

    // Camera reprojection (existing math, verbatim)
    float2 uv = (float2(pixCoord) + 0.5) * invVPSize;
    float viewZ = g_srcDepth.SampleLevel(g_samplerPointClamp, uv, 0);
    float3 viewPos = GTVBAO_ComputeViewspacePosition(uv, viewZ, consts);
    float4 worldPos = mul(float4(viewPos, 1.0), viewInv_g);
    float4 prevClipPos = mul(worldPos, prevViewProj_g);
    float2 prevNDC = prevClipPos.xy / prevClipPos.w;
    float2 prevUV = prevNDC * float2(-0.5, 0.5) + 0.5;

    float validity = 1.0;

    // Off-screen → no history (kept binary: reprojection outside is meaningless)
    if (any(prevUV < 0.0 || prevUV > 1.0)) {
      validity = 0.0;
    } else {
      // ── R1c: distance-scaled depth threshold ──
      // Absolute base threshold near camera; grows with world-space size of one
      // pixel at this depth (×4 grazing-slope tolerance) to stop spurious
      // rejections on distant geometry.
      float baseThresh = max(0.0001, GTVBAO_disocclusion_threshold);
      float worldPx = abs(viewZ) * abs(consts.NDCToViewMul_x_PixelSize.y);
      float threshEff = max(baseThresh, worldPx * 4.0);

      float prevDepth = g_srcDepth.SampleLevel(g_samplerPointClamp, prevUV, 0);
      float dz = abs(viewZ - prevDepth);
      // ── R1d: smooth rejection ramp — full weight below threshEff, ramping to
      // zero at 2×threshEff instead of a binary cut.
      float depthValid = saturate(1.0 - (dz / threshEff - 1.0));

      // ── R1b: normal rejection via MRT normals ──
      float normalValid = 1.0;
      float nrThresh = saturate(GTVBAO_temporal_normal_reject);
      if (nrThresh > 0.001) {
        float3 curN = float3(0, 0, 1);
        float3 histN = curN;
        bool normalsOk = false;
        if (GTVBAO_mrt_normal_available > 0.5f) {
          uint mw, mh;
          g_mrtNormalTexture.GetDimensions(mw, mh);
          if (mw > 0 && mh > 0) {
            float2 scale = float2(mw, mh) / max(vpSize, 1.0.xx);
            int2 curTc = min(int2(floor((float2(pixCoord) + 0.5) * scale)), int2(mw - 1, mh - 1));
            float2 huv = saturate(prevUV);
            int2 histTc = min(int2(huv * float2(mw, mh)), int2(mw - 1, mh - 1));
            curN = GTVBAO_DenoiseNormalToView(GTVBAO_DenoiseDecodeMrtNormal((uint2)curTc));
            histN = GTVBAO_DenoiseNormalToView(GTVBAO_DenoiseDecodeMrtNormal((uint2)histTc));
            normalsOk = true;
          }
        }
        if (normalsOk) {
          float ndot = dot(curN, histN);
          // Full acceptance above nrThresh, smooth ramp below it.
          normalValid = smoothstep(nrThresh - 0.15, nrThresh, ndot);
        }
      }

      validity = depthValid * normalValid;
    }

    // Sample + decode 16-bit history
    float2 huv = saturate(prevUV);
    int2 historyCoord = min(int2(huv * float2(hw, hh)), int2(hw - 1, hh - 1));
    float historyAO = GTVBAO_UnpackAO16(g_srcHistoryAO.Load(int3(historyCoord, 0)));

    // Guard against uninitialized history (fallback texture decodes to 1.0 which
    // is indistinguishable from real data — keep legacy <0.001 guard for zeros).
    if (historyAO < 0.001) validity = 0.0;

    // ── R1a: variance clamp — bound stale history by the current neighborhood ──
    float historyClamped = GTVBAO_ClampHistoryToNeighborhood(pixCoord, historyAO);

    float blendedFinal = lerp(curFinal, historyClamped, saturate(GTVBAO_temporal_blend * validity));

    // Working term: back to /scale domain, 8-bit packed (spatial chain expects /255 decode)
    float blendedRaw = saturate(blendedFinal * (1.0f / GT_VBAO_OCCLUSION_TERM_SCALE));
    g_outFinalAOTerm[pixCoord] = (uint)(blendedRaw * 255.0f + 0.5f);
    // History: 16-bit packed final-domain accumulator
    g_outHistoryAO[pixCoord] = GTVBAO_PackAO16(blendedFinal);
  }
}

[numthreads(GT_VBAO_NUMTHREADS_X, GT_VBAO_NUMTHREADS_Y, 1)]
void main(uint2 dt : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_srcWorkingAOTerm.GetDimensions(width, height);

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  if (GTVBAO_denoise_stage > 3.5f)
  {
    // ── Stage 4: GI-only (à-trous tail for Spatial-only mode) ──
    // AO was fully processed by the à-trous chain (scale-back folded into its
    // last iteration). This dispatch only runs the GI bilateral so GI keeps
    // working when the regular denoise_last spatial pass is skipped.
  }
  else if (GTVBAO_denoise_stage > 2.5f)
  {
    // ── Stage 3: passthrough scale-back (à-trous tail) ──
    [unroll]
    for (int s3 = 0; s3 < 2; s3++) {
      int2 pc = int2(dt.x * 2 + s3, dt.y);
      if (pc.x >= (int)width || pc.y >= (int)height) continue;
      float v = (float)g_srcWorkingAOTerm.Load(int3(pc, 0)) * (1.0f / 255.0f);
      v = saturate(v * GT_VBAO_OCCLUSION_TERM_SCALE);
      g_outFinalAOTerm[pc] = (uint)(v * 255.0f + 0.5f);
    }
  }
  else if (GTVBAO_denoise_stage > 1.5f)
  {
    // ── Stage 2: spatial-final only (R2 last stage) — NO temporal ──
    if (GTVBAO_prefilter_enabled > 0.5f) {
      uint pf_w, pf_h;
      g_srcWorkingAOTerm.GetDimensions(pf_w, pf_h);
      [unroll]
      for (int pf_side = 0; pf_side < 2; pf_side++) {
        int2 pc = int2(dt.x * 2 + pf_side, dt.y);
        if (pc.x >= (int)pf_w || pc.y >= (int)pf_h) continue;
        float centerAO = (float)g_srcWorkingAOTerm.Load(int3(pc, 0)) * (1.0f / 255.0f);
        float centerDepth = g_srcDepth.Load(int3(pc, 0));
        float filteredSum = centerAO, filteredW = 1.0f;
        [unroll]
        for (int dy = -1; dy <= 1; dy++) {
          [unroll]
          for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int2 npc = int2(pc.x + dx, pc.y + dy);
            if (npc.x < 0 || npc.y < 0 || npc.x >= (int)pf_w || npc.y >= (int)pf_h) continue;
            float nAO = (float)g_srcWorkingAOTerm.Load(int3(npc, 0)) * (1.0f / 255.0f);
            float nDepth = g_srcDepth.Load(int3(npc, 0));
            float depthW = exp(-abs(centerDepth - nDepth) * 10.0f);
            filteredSum += nAO * depthW;
            filteredW += depthW;
          }
        }
        float filteredAO = filteredSum / max(filteredW, 1e-5f);
        filteredAO = saturate(filteredAO * GT_VBAO_OCCLUSION_TERM_SCALE);
        g_outFinalAOTerm[pc] = (uint)(filteredAO * 255.0f + 0.5f);
      }
    }
    else
    {
      GTVBAO_Denoise(dt * uint2(2, 1), consts,
          g_srcWorkingAOTerm, g_srcWorkingEdges, g_samplerPointClamp,
          g_outFinalAOTerm, true);
    }
  }
  else if (GTVBAO_denoise_stage > 0.5f)
  {
    // ── Stage 1: temporal-only (R2 first stage) ──
    GTVBAO_TemporalFirstStage(dt * uint2(2, 1), consts);
  }
  else if (GTVBAO_denoiser_type >= 1.5f)
  {
    // ── Legacy Poisson denoiser (denoiser_type = 2) — unchanged structure,
    //    history upgraded to 16-bit + improved validity/clamp. ──
    GTVBAO_DenoiseAO_Poisson(dt * uint2(2, 1), consts,
        g_srcWorkingAOTerm, g_srcDepth, g_mrtNormalTexture, g_samplerPointClamp,
        g_outFinalAOTerm, true);

    float2 vpSize = float2(width, height);
    float2 invVPSize = 1.0.xx / vpSize;
    uint hw, hh;
    g_srcHistoryAO.GetDimensions(hw, hh);

    [unroll]
    for (int side = 0; side < 2; side++) {
      int2 pixCoord = int2(dt.x * 2 + side, dt.y);
      if (pixCoord.x >= (int)width || pixCoord.y >= (int)height) continue;

      uint spatialPacked = g_outFinalAOTerm[pixCoord];
      float spatialAO = float(spatialPacked) / 255.0;

      float2 uv = (float2(pixCoord) + 0.5) * invVPSize;
      float viewZ = g_srcDepth.SampleLevel(g_samplerPointClamp, uv, 0);
      float3 viewPos = GTVBAO_ComputeViewspacePosition(uv, viewZ, consts);
      float4 worldPos = mul(float4(viewPos, 1.0), viewInv_g);
      float4 prevClipPos = mul(worldPos, prevViewProj_g);
      float2 prevNDC = prevClipPos.xy / prevClipPos.w;
      float2 prevUV = prevNDC * float2(-0.5, 0.5) + 0.5;

      float validity = 1.0;
      if (any(prevUV < 0.0 || prevUV > 1.0)) {
        validity = 0.0;
      } else {
        float baseThresh = max(0.0001, GTVBAO_disocclusion_threshold);
        float worldPx = abs(viewZ) * abs(consts.NDCToViewMul_x_PixelSize.y);
        float threshEff = max(baseThresh, worldPx * 4.0);
        float prevDepth = g_srcDepth.SampleLevel(g_samplerPointClamp, prevUV, 0);
        float dz = abs(viewZ - prevDepth);
        float depthValid = saturate(1.0 - (dz / threshEff - 1.0));

        float normalValid = 1.0;
        float nrThresh = saturate(GTVBAO_temporal_normal_reject);
        if (nrThresh > 0.001 && GTVBAO_mrt_normal_available > 0.5f) {
          uint mw, mh;
          g_mrtNormalTexture.GetDimensions(mw, mh);
          if (mw > 0 && mh > 0) {
            float2 scale = float2(mw, mh) / max(vpSize, 1.0.xx);
            int2 curTc = min(int2(floor(((float2(pixCoord) + 0.5) * invVPSize) * float2(mw, mh))), int2(mw - 1, mh - 1));
            int2 histTc = min(int2(saturate(prevUV) * float2(mw, mh)), int2(mw - 1, mh - 1));
            float3 curN = GTVBAO_DenoiseNormalToView(GTVBAO_DenoiseDecodeMrtNormal((uint2)curTc));
            float3 histN = GTVBAO_DenoiseNormalToView(GTVBAO_DenoiseDecodeMrtNormal((uint2)histTc));
            normalValid = smoothstep(nrThresh - 0.15, nrThresh, dot(curN, histN));
          }
        }
        validity = depthValid * normalValid;
      }

      int2 historyCoord = min(int2(saturate(prevUV) * float2(hw, hh)), int2(hw - 1, hh - 1));
      float historyAO = GTVBAO_UnpackAO16(g_srcHistoryAO.Load(int3(historyCoord, 0)));
      if (historyAO < 0.001) validity = 0.0;
      historyAO = GTVBAO_ClampHistoryToNeighborhood(pixCoord, historyAO);

      float blendedAO = lerp(spatialAO, historyAO, saturate(GTVBAO_temporal_blend * validity));
      g_outFinalAOTerm[pixCoord] = (uint)(saturate(blendedAO) * 255.0 + 0.5);
      g_outHistoryAO[pixCoord] = GTVBAO_PackAO16(blendedAO);
    }
  }
  else
  {
    // ── Legacy stage 0: combined spatial/prefilter + spatio-temporal ──
    // Pre-filter: 3×3 depth-aware bilateral on raw AO
    if (GTVBAO_prefilter_enabled > 0.5f) {
      uint pf_w, pf_h;
      g_srcWorkingAOTerm.GetDimensions(pf_w, pf_h);
      [unroll]
      for (int pf_side = 0; pf_side < 2; pf_side++) {
        int2 pc = int2(dt.x * 2 + pf_side, dt.y);
        if (pc.x >= (int)pf_w || pc.y >= (int)pf_h) continue;
        float centerAO = (float)g_srcWorkingAOTerm.Load(int3(pc, 0)) * (1.0f / 255.0f);
        float centerDepth = g_srcDepth.Load(int3(pc, 0));
        float filteredSum = centerAO, filteredW = 1.0f;
        [unroll]
        for (int dy = -1; dy <= 1; dy++) {
          [unroll]
          for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int2 npc = int2(pc.x + dx, pc.y + dy);
            if (npc.x < 0 || npc.y < 0 || npc.x >= (int)pf_w || npc.y >= (int)pf_h) continue;
            float nAO = (float)g_srcWorkingAOTerm.Load(int3(npc, 0)) * (1.0f / 255.0f);
            float nDepth = g_srcDepth.Load(int3(npc, 0));
            float depthW = exp(-abs(centerDepth - nDepth) * 10.0f);
            filteredSum += nAO * depthW;
            filteredW += depthW;
          }
        }
        float filteredAO = filteredSum / max(filteredW, 1e-5f);
        filteredAO = saturate(filteredAO * GT_VBAO_OCCLUSION_TERM_SCALE);
        g_outFinalAOTerm[pc] = (uint)(filteredAO * 255.0f + 0.5f);
      }
    }
    else
    {
      GTVBAO_Denoise(dt * uint2(2, 1), consts,
          g_srcWorkingAOTerm, g_srcWorkingEdges, g_samplerPointClamp,
          g_outFinalAOTerm, true);
    }

    if (GTVBAO_denoiser_type > 0.5f) {
    float2 vpSize = float2(width, height);
    float2 invVPSize = 1.0.xx / vpSize;
    uint hw, hh;
    g_srcHistoryAO.GetDimensions(hw, hh);

    [unroll]
    for (int side = 0; side < 2; side++) {
      int2 pixCoord = int2(dt.x * 2 + side, dt.y);
      if (pixCoord.x >= (int)width || pixCoord.y >= (int)height) continue;

      // Read spatial result
      uint spatialPacked = g_outFinalAOTerm[pixCoord];
      float spatialAO = float(spatialPacked) / 255.0;

      // Current UV
      float2 uv = (float2(pixCoord) + 0.5) * invVPSize;

      // Camera reprojection — reconstruct viewspace position from depth,
      // transform to previous frame's clip space via world space.
      float viewZ = g_srcDepth.SampleLevel(g_samplerPointClamp, uv, 0);
      float3 viewPos = GTVBAO_ComputeViewspacePosition(uv, viewZ, consts);
      float4 worldPos = mul(float4(viewPos, 1.0), viewInv_g);
      float4 prevClipPos = mul(worldPos, prevViewProj_g);
      float2 prevNDC = prevClipPos.xy / prevClipPos.w;
      float2 prevUV = prevNDC * float2(-0.5, 0.5) + 0.5;

      float validity = 1.0;
      if (any(prevUV < 0.0 || prevUV > 1.0)) {
        validity = 0.0;
      } else {
        float baseThresh = max(0.0001, GTVBAO_disocclusion_threshold);
        float worldPx = abs(viewZ) * abs(consts.NDCToViewMul_x_PixelSize.y);
        float threshEff = max(baseThresh, worldPx * 4.0);
        float prevDepth = g_srcDepth.SampleLevel(g_samplerPointClamp, prevUV, 0);
        float dz = abs(viewZ - prevDepth);
        float depthValid = saturate(1.0 - (dz / threshEff - 1.0));

        float normalValid = 1.0;
        float nrThresh = saturate(GTVBAO_temporal_normal_reject);
        if (nrThresh > 0.001 && GTVBAO_mrt_normal_available > 0.5f) {
          uint mw, mh;
          g_mrtNormalTexture.GetDimensions(mw, mh);
          if (mw > 0 && mh > 0) {
            float2 scale = float2(mw, mh) / max(vpSize, 1.0.xx);
            int2 curTc = min(int2(floor((float2(pixCoord) + 0.5) * scale)), int2(mw - 1, mh - 1));
            int2 histTc = min(int2(saturate(prevUV) * float2(mw, mh)), int2(mw - 1, mh - 1));
            float3 curN = GTVBAO_DenoiseNormalToView(GTVBAO_DenoiseDecodeMrtNormal((uint2)curTc));
            float3 histN = GTVBAO_DenoiseNormalToView(GTVBAO_DenoiseDecodeMrtNormal((uint2)histTc));
            normalValid = smoothstep(nrThresh - 0.15, nrThresh, dot(curN, histN));
          }
        }
        validity = depthValid * normalValid;
      }

      int2 historyCoord = min(int2(saturate(prevUV) * float2(hw, hh)), int2(hw - 1, hh - 1));
      float historyAO = GTVBAO_UnpackAO16(g_srcHistoryAO.Load(int3(historyCoord, 0)));
      if (historyAO < 0.001) validity = 0.0;
      historyAO = GTVBAO_ClampHistoryToNeighborhood(pixCoord, historyAO);

      float blendedAO = lerp(spatialAO, historyAO, saturate(GTVBAO_temporal_blend * validity));
      uint blendedPacked = (uint)(saturate(blendedAO) * 255.0 + 0.5);
      g_outFinalAOTerm[pixCoord] = blendedPacked;
      g_outHistoryAO[pixCoord] = GTVBAO_PackAO16(blendedAO);
    }
  }
  } // end legacy stage 0

  // GI always uses original 3×3 bilateral (runs for all denoiser types/stages)
  if (g_gi_enabled > 0.5f)
  {
      GTVBAO_DenoiseGI(dt * uint2(2, 1), consts,
          g_srcRawGI, g_srcWorkingEdges, g_samplerPointClamp,
          g_outGI);
  }
}
