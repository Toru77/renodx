#define SHARC_UPDATE 0
#include "world_pt_common.hlsli"
#include "world_pt_evaluate.hlsli"
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= g_size)) return;
  const bool water_pass = (g_refl_flags & 16u) != 0u;
  RtxdiSurfaceResult surface;
  if (!PtLoadSurface(id.xy, surface)) { if (!water_pass) g_pt_output[id.xy] = 0; if (g_refl_samples != 0u || water_pass) g_pt_reflection[id.xy] = 0; return; }
  if (water_pass && (g_pt_normal.Load(int3(id.xy, 0)).w & 2u) == 0u) { g_pt_reflection[id.xy] = 0; return; }
  if (!water_pass && !((g_refl_flags & 8u) != 0u && (g_pt_normal.Load(int3(id.xy, 0)).w & 2u) != 0u)
      && g_sparse_enabled != 0u && g_sparse_levels < 256u) {
    PtCount(8u);
    if (!PtSparseSelected(id.xy)) {
      g_pt_output[id.xy] = float4(0, 0, 0, 1);
      if ((g_refl_flags & 32u) != 0u) {
        if (g_refl_samples != 0u) {
          g_pt_reflection[id.xy] = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u
              ? float4(0, 0, 0, 0)
              : float4(0, 0, 0, 1e-3);
        }
        return;
      }
      if (g_refl_samples == 0u) return;
      if ((surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u) {
        g_pt_reflection[id.xy] = float4(0, 0, 0, 0);
        return;
      }
      PtEvaluate(id, surface, water_pass, false);
      return;
    }
    PtCount(7u);
  }
  PtEvaluate(id, surface, water_pass, true);
}
