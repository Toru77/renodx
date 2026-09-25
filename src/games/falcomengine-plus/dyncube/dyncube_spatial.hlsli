// dyncube_spatial.hlsli — positional spatial reprojection (experimental).
//
// Searches a small tangent-space neighborhood around a cube-space reflection
// direction for the temporal-cubemap texel whose stored world position lies
// closest to the live world-space ray. Returns true + best cube-space
// direction on a match; otherwise false.
//
// Moved verbatim from the Sora 2nd lighting shader; the only change is that
// the history position texture and its sampler are explicit parameters
// instead of file-scope globals, so other games can bind their own slots.
// Stored positions use the capture convention world * 0.001.
// No resources, samplers, cbuffers, or settings are declared here; the
// caller supplies radius / candidate count / thresholds from its own config.

#ifndef __DYNCUBE_SPATIAL_HLSLI__
#define __DYNCUBE_SPATIAL_HLSLI__

bool DynCubeSpatialReproject(
    TextureCube<float4> histPosTex, SamplerState histPosSampler,
    float3 Rcubedir, float3 Pworld, float3 Rworld,
    float radius, int maxCand,
    float errThreshold, float minDist,
    out float3 bestDir)
{
  bestDir = Rcubedir;
  float3 T = normalize(cross(Rcubedir, abs(Rcubedir.y) > 0.99f ? float3(1, 0, 0) : float3(0, 1, 0)));
  float3 B = cross(Rcubedir, T);
  float bestRel = errThreshold;
  bool found = false;
  for (int i = 0; i < 9; ++i) {
    if (i >= maxCand) break;
    float3 off;
    if (i == 0) off = float3(0, 0, 0);
    else if (i == 1) off = T * radius;
    else if (i == 2) off = -T * radius;
    else if (i == 3) off = B * radius;
    else if (i == 4) off = -B * radius;
    else if (i == 5) off = (T + B) * radius;
    else if (i == 6) off = (T - B) * radius;
    else if (i == 7) off = (-T + B) * radius;
    else off = (-T - B) * radius;
    float3 D = normalize(Rcubedir + off);
    float4 Psample = histPosTex.SampleLevel(histPosSampler, D, 0);
    if (Psample.a <= 0.5f) continue;
    float3 Q = Psample.rgb * 1000.0f;  // stored pos is world * posScale (0.001)
    if (!all(isfinite(Q))) continue;
    float t = dot(Q - Pworld, Rworld);
    if (t <= minDist) continue;
    float rel = length(Q - (Pworld + Rworld * t)) / max(t, 1e-4f);
    if (rel < bestRel) {
      bestRel = rel;
      bestDir = D;
      found = true;
    }
  }
  return found;
}

#endif // __DYNCUBE_SPATIAL_HLSLI__
