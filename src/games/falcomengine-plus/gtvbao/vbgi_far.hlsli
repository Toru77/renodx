#ifndef SRC_GAMES_FALCOMENGINE_PLUS_GTVBAO_VBGI_FAR_HLSLI_
#define SRC_GAMES_FALCOMENGINE_PLUS_GTVBAO_VBGI_FAR_HLSLI_
// VBGI far field (ReSTIR GI on the escape directions). Shared by the far main variant and the far passes.
// GTVBAO view space is the engine view with Z negated (gtvbao_common.hlsl TransformNormalToView). Far samples
// integrate in the cosine-weighted measure of the analytic VBGI output, so W x L adds directly to the raw GI.
static const uint kVbgiFarSteps = 16u;
float3 VbgiFarViewToWorld(float3 pv) { return mul(float4(pv.x, pv.y, -pv.z, 1.0f), viewInv_g).xyz; }
float3 VbgiFarWorldToView(float3 pw) { const float3 e = mul(float4(pw, 1.0f), view_g).xyz; return float3(e.x, e.y, -e.z); }
float2 VbgiFarViewToUv(float3 pv, GTAOConstants consts) { return (pv.xy / pv.z - consts.NDCToViewAdd) / consts.NDCToViewMul; }
float VbgiFarLuminance(float3 c) { return dot(c, float3(0.2126f, 0.7152f, 0.0722f)); }
static const uint kVbgiFarNearSteps = 6u;
struct VbgiFarReservoir { float3 pos; float3 rad; float3 nrm; float phat; float wsum; float M; };
// Streaming RIS merge of a reservoir (or candidate) whose sample has target phat here, contribution weight W and count M.
void VbgiFarMerge(inout VbgiFarReservoir r, float3 pos, float3 rad, float3 nrm, float phat, float W, float M, float u)
{
  const float w = phat * W * M;
  r.wsum += w;
  r.M += M;
  if (w > 0.0f && u * r.wsum < w) { r.pos = pos; r.rad = rad; r.nrm = nrm; r.phat = phat; }
}
float VbgiFarFinalW(VbgiFarReservoir r) { return (r.wsum > 0.0f && r.phat > 0.0f && r.M > 0.0f) ? r.wsum / (r.M * r.phat) : 0.0f; }
float VbgiFarRand(uint2 p, uint salt)
{
  const float2 q = float2(p) + (float)((uint)GTVBAO_noise_index * 13u + salt * 7u) * float2(5.588238f, 3.137255f);
  return frac(52.9829189f * frac(dot(q, float2(0.06711056f, 0.00583715f))));
}
// Receiver P (normal N) and sender S (normal NS) can exchange light and S lies beyond the analytic near field.
bool VbgiFarUsable(float3 P, float3 N, float3 S, float3 NS, float radius, out float cosR, out float cosS, out float dist2)
{
  const float3 d = S - P;
  dist2 = dot(d, d);
  const float3 dir = d * rsqrt(max(dist2, 1e-12f));
  cosR = dot(N, dir);
  cosS = dot(NS, -dir);
  return dist2 >= radius * radius && cosR > 0.0f && cosS > 0.0f;
}
// Nothing in the depth buffer blocks the first `radius` view units from P toward S (the near field's region).
bool VbgiFarNearClear(float3 P, float3 S, float radius, float thickness, float2 uvP, GTAOConstants consts,
                      Texture2D<lpfloat> depth, SamplerState samp, float jitter)
{
  const float3 d = S - P;
  const float len = length(d);
  const float3 dir = d / max(len, 1e-6f);
  const float tEnd = min(radius, len);
  [loop] for (uint i = 0u; i < kVbgiFarNearSteps; ++i)
  {
    const float3 Q = P + dir * (tEnd * (((float)i + jitter) / (float)kVbgiFarNearSteps));
    if (Q.z <= 0.0f) return false;
    const float2 uv = VbgiFarViewToUv(Q, consts);
    if (any(uv < 0.0f) || any(uv > 1.0f)) return true;
    const float offsetPx = length((uv - uvP) / consts.ViewportPixelSize);
    if (offsetPx < 1.3f) continue;
    const float z = depth.SampleLevel(samp, uv, clamp(log2(offsetPx) - consts.DepthMIPSamplingOffset, 0.0f, (float)GT_VBAO_DEPTH_MIP_LEVELS)).x;
    if (Q.z > z && Q.z < z + thickness) return false;
  }
  return true;
}
#endif
