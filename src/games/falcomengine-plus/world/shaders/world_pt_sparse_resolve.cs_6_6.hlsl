#define SHARC_UPDATE 0
#include "world_pt_common.hlsli"
#include "world_pt_evaluate.hlsli"
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= g_size) || PtSparseSelected(id.xy)) return;
  RtxdiSurfaceResult surface;
  if (!PtLoadSurface(id.xy, surface)) return;
  bool found = false;
  float best_plane = 0.0;
  uint best_distance = 0u;
  uint2 best_pixel = 0u;
  for (int y = -2; y <= 2; ++y) {
    for (int x = -2; x <= 2; ++x) {
      if (x == 0 && y == 0) continue;
      const int2 candidate = int2(id.xy) + int2(x, y);
      if (any(candidate < 0) || any(candidate >= int2(g_size)) || !PtSparseSelected(uint2(candidate))) continue;
      RtxdiSurfaceResult other;
      if (!PtLoadSurface(uint2(candidate), other)) continue;
      if (((other.flags ^ surface.flags) & RTXDI_SURFACE_FLAG_CHARACTER) != 0u) continue;
      const float plane = abs(dot(other.world_position - surface.world_position, surface.shading_normal));
      if (plane > 0.02 * surface.linear_view_depth || dot(other.shading_normal, surface.shading_normal) < 0.8) continue;
      const uint distance = uint(x * x + y * y);
      if (!found || plane < best_plane || (plane == best_plane && distance < best_distance)) {
        found = true;
        best_plane = plane;
        best_distance = distance;
        best_pixel = uint2(candidate);
      }
    }
  }
  if (found) {
    g_pt_output[id.xy] = g_pt_output[best_pixel];
    if (g_refl_samples != 0u) g_pt_reflection[id.xy] = g_pt_reflection[best_pixel];
  } else {
    PtEvaluate(id, surface, false);
  }
}
