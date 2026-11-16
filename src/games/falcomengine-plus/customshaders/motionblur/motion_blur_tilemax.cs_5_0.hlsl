// Motion Blur P1 — TileMax, one pass.
//
// One thread group per tile, 16x16 threads. Each thread strides over the tile's
// texels, keeps the LONGEST velocity it has seen, and the group reduces those
// candidates through groupshared memory. Comparison is on length, so the result
// is the tile-based dominant velocity approach of McGuire et al. that the paper
// builds on. A tile with no velocity yields (0, 0).
//
// Input is the GAME's raw pixel motion; each fetch is converted to UV once here.

#include "motion_blur_common.hlsli"

Texture2D<float2>   g_srcMotion  : register(t0);
RWTexture2D<float2> g_outTileMax : register(u0);

groupshared float  gsMagSq[256];
groupshared float2 gsVec[256];

[numthreads(16, 16, 1)]
void main(uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID) {
  int motionW = (int)shader_injection_data.mb_motion_w;
  int motionH = (int)shader_injection_data.mb_motion_h;
  int tilesX  = (int)shader_injection_data.mb_tiles_x;
  int tilesY  = (int)shader_injection_data.mb_tiles_y;
  if (tilesX <= 0 || tilesY <= 0) return;

  int tileW = max(1, (motionW + tilesX - 1) / tilesX);
  int tileH = max(1, (motionH + tilesY - 1) / tilesY);

  int2 tileBase = int2(groupID.xy) * int2(tileW, tileH);
  int2 tileEnd  = min(tileBase + int2(tileW, tileH), int2(motionW, motionH));

  float2 best = float2(0.0, 0.0);
  float bestMagSq = -1.0;
  for (int y = tileBase.y + (int)threadID.y; y < tileEnd.y; y += 16) {
    for (int x = tileBase.x + (int)threadID.x; x < tileEnd.x; x += 16) {
      float2 v = MBGameMotionToUV(g_srcMotion[int2(x, y)]);
      float magSq = dot(v, v);
      if (magSq > bestMagSq) {
        bestMagSq = magSq;
        best = v;
      }
    }
  }

  uint t = threadID.y * 16u + threadID.x;
  gsMagSq[t] = bestMagSq;
  gsVec[t] = best;
  GroupMemoryBarrierWithGroupSync();

  [unroll]
  for (uint stride = 128u; stride > 0u; stride >>= 1u) {
    if (t < stride && gsMagSq[t + stride] > gsMagSq[t]) {
      gsMagSq[t] = gsMagSq[t + stride];
      gsVec[t] = gsVec[t + stride];
    }
    GroupMemoryBarrierWithGroupSync();
  }

  if (t == 0u) g_outTileMax[int2(groupID.xy)] = gsVec[0];
}
