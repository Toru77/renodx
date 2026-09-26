// Motion Blur P1 — separable TileMax.
//
// Paper Section 3: "The TileMax pass can be computed in two passes, one per
// image dimension." This file is dispatched twice with mb_pass selecting the
// axis, which keeps the work linear in tile size and keeps every load coalesced
// along the scan axis. Comparison is on length, so the reduction keeps the
// LONGEST velocity in each segment — the tile-based dominant velocity approach
// of McGuire et al. that the paper builds on.
//
// Input is the GAME's motion texture on the X pass, and this pass's own output
// on the Y pass. The pixels->UV conversion, the shutter scale and the tile-size
// clamp run per fetch on the X pass only, which is a few ALU against a load the
// pass was issuing anyway, so the fetch count is unchanged. That removes a
// full-resolution intermediate and its per-frame write, which measured as the
// dominant cost because it evicted the gather's working set from L2.
//
//   mb_pass 0 (X): (motion_w, motion_h) -> (tiles_x, motion_h)   raw pixels in
//   mb_pass 1 (Y): (tiles_x, motion_h)   -> (tiles_x, tiles_y)   UV already in

#include "motion_blur_common.hlsli"

Texture2D<float2>   g_srcMotion  : register(t0);
RWTexture2D<float2> g_outTileMax : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
  int motionW = (int)shader_injection_data.mb_motion_w;
  int motionH = (int)shader_injection_data.mb_motion_h;
  int tilesX  = (int)shader_injection_data.mb_tiles_x;
  int tilesY  = (int)shader_injection_data.mb_tiles_y;
  if (tilesX <= 0 || tilesY <= 0) return;

  bool vertical = shader_injection_data.mb_pass > 0.5;
  int tileW = max(1, (motionW + tilesX - 1) / tilesX);
  int tileH = max(1, (motionH + tilesY - 1) / tilesY);

  int2 inDims  = vertical ? int2(tilesX, motionH) : int2(motionW, motionH);
  int2 outDims = vertical ? int2(tilesX, tilesY)   : int2(tilesX, motionH);

  int2 p = int2(dispatchThreadID.xy);
  if (p.x >= outDims.x || p.y >= outDims.y) return;

  int2 base  = vertical ? int2(p.x, p.y * tileH) : int2(p.x * tileW, p.y);
  int  range = vertical ? tileH : tileW;

  float2 best = float2(0.0, 0.0);
  float bestMagSq = -1.0;
  for (int i = 0; i < range; ++i) {
    int2 s = vertical ? int2(base.x, base.y + i) : int2(base.x + i, base.y);
    if (s.x >= inDims.x || s.y >= inDims.y) break;
    // t0 is the game's raw pixel motion on the X pass, but this pass's OWN
    // already-converted UV output on the Y pass. Converting both would divide by
    // the motion dimensions twice, leaving velocity/dims^2 (~1e-5 for a 20px
    // flick), which sits far below the gather's fixed minVelocityUV threshold
    // (0.5/1080). The result is a correctly shaped but microscopic vmax: the
    // early-out fires on every pixel, Blur Amount renders black, and Neighbor
    // Max still shows a valid direction because the vector is non-zero. It
    // silently disabled the whole filter.
    float2 raw = g_srcMotion[s];
    float2 v = vertical ? raw : MBGameMotionToUV(raw);
    float magSq = dot(v, v);
    if (magSq > bestMagSq) {
      bestMagSq = magSq;
      best = v;
    }
  }

  g_outTileMax[p] = best;
}
