// Motion Blur P2 — NeighborMax: the one-ring tile maximum.
//
// Paper Section 3 / McGuire et al.: a single dominant neighbourhood velocity per
// tile, taken over the 3x3 tile neighbourhood so a pixel's 1-ring tiles are
// covered.
//
// Section 4.4 refinement, implemented faithfully: an off-axis (diagonal)
// neighbour is only considered when its maximum blur direction would actually
// affect this tile. "if the tile to the top-left of the central tile has a
// maximum velocity that does not point towards the middle, it is not
// considered in the vmax computation". On-axis neighbours are never culled,
// matching the paper's reasoning that small deviations there still produce
// overlapping blurs while the corners need a much larger deviation to matter.
// This is what keeps the estimate from over-blurring, at the cost of the
// "single velocity" artifacts the paper's multi-direction sampling then fixes
// in the gather.

#include "motion_blur_common.hlsli"

Texture2D<float2>   g_srcTileMax     : register(t0);
RWTexture2D<float2> g_outNeighborMax : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
  int2 p = int2(dispatchThreadID.xy);
  int tilesX = (int)shader_injection_data.mb_tiles_x;
  int tilesY = (int)shader_injection_data.mb_tiles_y;
  if (tilesX <= 0 || tilesY <= 0) return;
  if (p.x >= tilesX || p.y >= tilesY) return;

  float2 best = g_srcTileMax[p];
  float bestMagSq = dot(best, best);

  [unroll]
  for (int dy = -1; dy <= 1; ++dy) {
    [unroll]
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) continue;
      int2 n = p + int2(dx, dy);
      if (n.x < 0 || n.x >= tilesX || n.y < 0 || n.y >= tilesY) continue;

      float2 v = g_srcTileMax[n];

      if (dx != 0 && dy != 0) {
        // Section 4.4: the diagonal tile blurs along its own velocity, so it
        // only matters here if that direction points back at this tile.
        if (dot(MBNorm(v), MBNorm(-float2(dx, dy))) <= 0.0) continue;
      }

      float magSq = dot(v, v);
      if (magSq > bestMagSq) {
        bestMagSq = magSq;
        best = v;
      }
    }
  }

  g_outNeighborMax[p] = best;
}
