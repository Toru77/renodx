///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus — Custom SSR (Sora 2nd) — Hi-Z debug visualization
//
// Phase 1 validation pass. Reads the SSR Hi-Z chain and renders a debug view
// selected by SSR_debug_view:
//   1 = mip 0 linearized view-space depth (grayscale, normalized by max ray distance)
//   2 = selected mip (SSR_debug_mip) log-depth ramp; red = near, blue = far,
//       magenta = empty/invalid cells
//   3 = ADJACENT-LEVEL bound invariant: every texel at mip N must be NEARER-OR-
//       EQUAL (<=) to its parent footprint texel at mip N-1 (nested footprints,
//       since (p>>N)>>1 == p>>(N-1)). Red = bound violation. Green-blue ramp =
//       healthy absorbed-variation margin. Gray = invalid/sky OR pixel whose
//       coordinate falls outside the selected level (orphan strips at odd
//       dimensions have no containing texel and must not be compared).
//   4 = raw value classification of selected mip:
//       white  = finite > 0        (valid depth)
//       black  = exactly 0         (nulled/uninitialized binding)
//       magenta= >= FLT_MAX * 0.5  (legitimate empty/sky cell)
//       red    = negative          (corruption)
//       yellow = NaN               (corruption)
//
// Dispatched with ceil(w/8) x ceil(h/8) x 1 groups.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ssr_common.hlsl"

Texture2D<float>    g_hizChain : register(t0);  // full mip chain SRV
RWTexture2D<float4> g_outDebug : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
  uint width, height;
  g_outDebug.GetDimensions(width, height);
  if (dispatchThreadID.x >= width || dispatchThreadID.y >= height) return;

  uint chain_w, chain_h, chain_levels;
  g_hizChain.GetDimensions(0, chain_w, chain_h, chain_levels);

  int mode = (int)(SSR_debug_view + 0.5);
  float4 out_color = float4(0.0, 0.0, 0.0, 1.0);

  if (mode == 1) {
    // Mip 0 depth — should visually match the scene's depth layout.
    float z = g_hizChain.Load(int3(dispatchThreadID.xy, 0));
    float t = saturate(z / max(SSR_max_ray_distance, 1.0));
    out_color.rgb = t.xxx;
  } else if (mode == 2) {
    // Selected mip — coverage/validity of a coarse level.
    int mip = (int)clamp(SSR_debug_mip, 0.0, (float)(7));
    uint mw = max(chain_w >> mip, 1u);
    uint mh = max(chain_h >> mip, 1u);
    int2 tc = min(int2(dispatchThreadID.xy) >> mip, int2((int)mw - 1, (int)mh - 1));
    float z = g_hizChain.Load(int3(tc, mip));
    if (z <= 0.0 || z >= SSR_FLT_MAX * 0.5) {
      out_color.rgb = float3(1.0, 0.0, 1.0);   // empty cell
    } else {
      float t = saturate(log2(max(z, 1.0)) / 14.0);
      out_color.rgb = float3(1.0 - t, 0.15, t);
    }
  } else if (mode == 3) {
    // ADJACENT-LEVEL bound invariant: mip N texel must be <= its mip N-1 parent
    // footprint texel. Coordinates that fall outside either level's real
    // dimensions (orphan strips from odd dims) render gray — they have no true
    // containing texel and clamping them would compare unrelated footprints.
    int mip = (int)clamp(SSR_debug_mip, 1.0, (float)(7));
    uint mwN = max(chain_w >> mip, 1u);
    uint mhN = max(chain_h >> mip, 1u);
    uint mwP = max(chain_w >> (mip - 1), 1u);
    uint mhP = max(chain_h >> (mip - 1), 1u);
    int2 tcN = int2(dispatchThreadID.xy) >> mip;
    int2 tcP = int2(dispatchThreadID.xy) >> (mip - 1);
    bool oob = tcN.x >= (int)mwN || tcN.y >= (int)mhN
            || tcP.x >= (int)mwP || tcP.y >= (int)mhP;
    if (oob) {
      out_color.rgb = float3(0.35, 0.35, 0.35);   // no containing texel at this level
    } else {
      float z_coarse = g_hizChain.Load(int3(tcN, mip));
      float z_prev = g_hizChain.Load(int3(tcP, mip - 1));
      bool invalid = z_coarse <= 0.0 || z_coarse >= SSR_FLT_MAX * 0.5
                  || z_prev <= 0.0 || z_prev >= SSR_FLT_MAX * 0.5;
      if (invalid) {
        out_color.rgb = float3(0.5, 0.5, 0.5);   // sky/empty on either side
      } else if (z_coarse > z_prev * (1.0 + 1e-4) + 1e-3) {
        out_color.rgb = float3(4.0, 0.0, 0.0);   // violation: child lost the parent's nearest surface
      } else {
        float margin = saturate((z_prev - z_coarse) / max(z_prev, 1.0));
        out_color.rgb = float3(0.05 + 0.10 * (1.0 - margin),
                               0.20 + 0.65 * margin,
                               0.35 + 0.15 * (1.0 - margin));
      }
    }
  } else if (mode == 4) {
    // Raw value classification — diagnoses binding/copy failures numerically.
    int mip = (int)clamp(SSR_debug_mip, 0.0, (float)(7));
    uint mw = max(chain_w >> mip, 1u);
    uint mh = max(chain_h >> mip, 1u);
    int2 tc = min(int2(dispatchThreadID.xy) >> mip, int2((int)mw - 1, (int)mh - 1));
    float z = g_hizChain.Load(int3(tc, mip));
    if (isnan(z)) {
      out_color.rgb = float3(4.0, 4.0, 0.0);           // yellow: NaN
    } else if (z < 0.0) {
      out_color.rgb = float3(4.0, 0.0, 0.0);           // red: negative corruption
    } else if (z == 0.0) {
      out_color.rgb = float3(0.0, 0.0, 0.0);           // black: nulled read / unwritten
    } else if (z >= SSR_FLT_MAX * 0.5) {
      out_color.rgb = float3(1.0, 0.0, 1.0);           // magenta: legitimate empty cell
    } else {
      out_color.rgb = float3(1.0, 1.0, 1.0);           // white: valid finite depth
    }
  } else {
    out_color.rgb = float3(0.0, 0.0, 0.0);
  }

  g_outDebug[dispatchThreadID.xy] = out_color;
}
