///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus GTVBAO — Pass 3: Denoise (final pass)
//
// Kai-style: builds GTAOConstants in-shader.
// Extended to denoise GI alongside AO when GT_VBAO_COMPUTE_GI is defined.
//
// Denoiser stages (dispatched via GTVBAO_denoise_stage, spatial-only):
//   stage 0 — spatial combined path (prefilter or 5×5 bilateral)
//   stage 2 — spatial-final only (runs LAST; applies OCCLUSION_TERM_SCALE, no temporal)
//   stage 3 — passthrough scale-back only (tail of the à-trous chain)
//   stage 4 — GI-only (à-trous tail for Spatial-only mode)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GT_VBAO_COMPUTE_GI
#include "gtvbao_common.hlsl"

Texture2D<uint>    g_srcWorkingAOTerm : register(t0);
Texture2D<lpfloat> g_srcWorkingEdges  : register(t1);
Texture2D<float4>  g_srcRawGI          : register(t2);  // raw GI from main pass
Texture2D<uint>    g_srcHistoryAO      : register(t3);  // unused (spatial only, bound to fallback)
Texture2D<float>   g_srcDepth          : register(t4);  // viewspace depth MIP0
Texture2D<uint4>   g_mrtNormalTexture  : register(t5);  // unused (spatial only, bound to fallback)
SamplerState       g_samplerPointClamp : register(s0);
RWTexture2D<uint>  g_outFinalAOTerm   : register(u0);
RWTexture2D<float4> g_outGI            : register(u1);
RWTexture2D<uint>  g_outHistoryAO     : register(u2);  // unused (spatial only, bound to fallback)

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
    // ── Stage 2: spatial-final only — NO temporal ──
    if (GTVBAO_prefilter_enabled > 0.5f) {
      uint pf_w, pf_h;
      g_srcWorkingAOTerm.GetDimensions(pf_w, pf_h);
      [unroll]
      for (int pf_side = 0; pf_side < 2; pf_side++) {
        int2 pc = int2(dt.x * 2 + pf_side, dt.y);
        if (pc.x >= (int)pf_w || pc.y >= (int)pf_h) continue;
        float centerAO = (float)g_srcWorkingAOTerm.Load(int3(pc, 0)) * (1.0f / 255.0f);
        float centerDepth = (GTVBAO_resolution > 0.5f)
            ? GTVBAO_LoadMappedDepth(g_srcDepth, pc, int2(pf_w, pf_h))
            : g_srcDepth.Load(int3(pc, 0));
        float filteredSum = centerAO, filteredW = 1.0f;
        [unroll]
        for (int dy = -1; dy <= 1; dy++) {
          [unroll]
          for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int2 npc = int2(pc.x + dx, pc.y + dy);
            if (npc.x < 0 || npc.y < 0 || npc.x >= (int)pf_w || npc.y >= (int)pf_h) continue;
            float nAO = (float)g_srcWorkingAOTerm.Load(int3(npc, 0)) * (1.0f / 255.0f);
            float nDepth = (GTVBAO_resolution > 0.5f)
                ? GTVBAO_LoadMappedDepth(g_srcDepth, npc, int2(pf_w, pf_h))
                : g_srcDepth.Load(int3(npc, 0));
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
  else
  {
    // ── Stage 0: spatial combined (prefilter or 5×5 bilateral) ──
    // Pre-filter: 3×3 depth-aware bilateral on raw AO
    if (GTVBAO_prefilter_enabled > 0.5f) {
      uint pf_w, pf_h;
      g_srcWorkingAOTerm.GetDimensions(pf_w, pf_h);
      [unroll]
      for (int pf_side = 0; pf_side < 2; pf_side++) {
        int2 pc = int2(dt.x * 2 + pf_side, dt.y);
        if (pc.x >= (int)pf_w || pc.y >= (int)pf_h) continue;
        float centerAO = (float)g_srcWorkingAOTerm.Load(int3(pc, 0)) * (1.0f / 255.0f);
        float centerDepth = (GTVBAO_resolution > 0.5f)
            ? GTVBAO_LoadMappedDepth(g_srcDepth, pc, int2(pf_w, pf_h))
            : g_srcDepth.Load(int3(pc, 0));
        float filteredSum = centerAO, filteredW = 1.0f;
        [unroll]
        for (int dy = -1; dy <= 1; dy++) {
          [unroll]
          for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int2 npc = int2(pc.x + dx, pc.y + dy);
            if (npc.x < 0 || npc.y < 0 || npc.x >= (int)pf_w || npc.y >= (int)pf_h) continue;
            float nAO = (float)g_srcWorkingAOTerm.Load(int3(npc, 0)) * (1.0f / 255.0f);
            float nDepth = (GTVBAO_resolution > 0.5f)
                ? GTVBAO_LoadMappedDepth(g_srcDepth, npc, int2(pf_w, pf_h))
                : g_srcDepth.Load(int3(npc, 0));
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
  } // end stage 0

  // GI always uses original 3×3 bilateral
  if (g_gi_enabled > 0.5f)
  {
      GTVBAO_DenoiseGI(dt * uint2(2, 1), consts,
          g_srcRawGI, g_srcWorkingEdges, g_samplerPointClamp,
          g_outGI);
  }
}
