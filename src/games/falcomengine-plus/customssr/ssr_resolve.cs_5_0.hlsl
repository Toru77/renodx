///////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus — Custom SSR (Sora 2nd) — Phase R2A: dual-ring spatial reconstruction
//
// Edge-aware reconstruction over the 1-spp VNDF ray result.
//   RGB = weighted neighbor radiance (center included when its ray is valid;
//         neighbors may reconstruct a center miss)
//   A   = RAW center confidence passthrough — no coverage heuristics.
//
// R2A tap strategy: TWO 8-direction rings instead of one perimeter ring.
//   Ring 1: 8 dirs at 0.45 x radius (interior fill)
//   Ring 2: same dirs rotated 22.5 deg, at 0.90 x radius
// Rationale: a single perimeter ring at large radii leaves ~10px inter-sample
// gaps (8 taps over an 800px^2 disk); interior coverage is where variance is.
//
// REFERENCE DISTINCTION (deliberate adaptation, not a port):
//   AMD FFX-DNSR-Reflections uses a FIXED +-3px 15-tap Halton kernel and
//   never scales it with roughness (resolve_spatial.h:84-102); broader
//   integration is delegated to its temporal stage, and mirror/non-glossy
//   pixels bypass via tile classification (:140-143).
//   Frostbite SSSR represents roughness footprint through FILTERED RADIANCE
//   FETCH (prefiltered pyramid mip from roughness/distance/elongation),
//   not through neighbor-kernel radius.
//   Our temporal-free architecture therefore uses roughness-aware reach as
//   an interpolation between the two philosophies (planned R2B); R2A first
//   isolates pure sample-density effects at fixed radius.
///////////////////////////////////////////////////////////////////////////////////////

#include "ssr_common.hlsl"

Texture2D<float4>   g_rayResult     : register(t0);  // raw stochastic (rgb=rad, a=conf)
Texture2D<float>    g_hizChain      : register(t1);  // Hi-Z chain (mip0 for depth)
Texture2D<uint4>    g_srcMrtNormal  : register(t2);  // G-buffer normals
Texture2D<uint2>    g_srcMrtMatIdx  : register(t3);  // material index
StructuredBuffer<float4> g_deferredParams : register(t4); // material params
Texture2D<float4>   g_rayMeta       : register(t5);  // R2C: r=pdf_L g=viewDist ba=octa(dir)
Texture2D<uint4>    g_srcMrtSpec    : register(t6);  // MRT1 specular (F0)
Texture2D<float4>   g_radPyr        : register(t7);  // R3: radiance pyramid (8 mips)
Texture2D<float4>   g_motionVectors : register(t8);  // R3-MV: DLSS motion vectors (RG)

RWTexture2D<float4> g_outResolved   : register(u0);
RWTexture2D<uint>   g_stats         : register(u1);  // RvS compare row (y25)

SamplerState g_samplerPointClamp  : register(s0);
SamplerState g_samplerLinearClamp : register(s1);  // R3: pyramid trilinear fetch

// ── Phase R1 fixed initial constants ──
// Radius / depth-sigma / normal-sigma are wired to their existing sliders
// via cb_ssr c32/c33/c34 (SSR_resolve_radius_max / _depth_sigma / _normal_sigma).
// Gates remain fixed per the R1 spec.
static const float  kDepthThresh = 0.10;   // reject if |dz| > z_c * this
static const float  kNormalMin   = 0.80;   // reject if dot(n_n,n_c) < this
static const float  kRoughMax    = 0.25;   // reject if |r_n - r_c| > this
static const float  EPS          = 1e-6;
static const float  EPS_PDF      = 1e-4;   // R2C pdf denominator floor
// Phase R2D coverage experiment: bounded reconstruction-aware coverage.
// Evidence per accepted tap = conf × depthSim × normSim (no distance falloff,
// no estimator ratio — kernel taper must not shrink coverage). Two
// full-quality taps grant full coverage (COV_EVIDENCE_NORM = 2.0, baked).
static const float  COV_EVIDENCE_NORM = 2.0;

// y25: RvS compare at probe pixel: [0..2]=rawRGB [3..5]=resolvedRGB [6]=rawA [7]=resA
// y26: ResolveProbe diagnosis at probe pixel:
//      [0]validNbr [1]alphaRej [2]depthRej [3]normalRej [4]roughRej
//      [5]centerW  [6]neighborW [7]totalW
#define SSR_RVS_ROW    25u
#define SSR_RPROBE_ROW 26u
#define SSR_EST_ROW    28u

#define SSR_RESOLVE_MAX_TAPS 16u

// Ring scales relative to the active radius (Phase R2A).
static const float RING1_SCALE = 0.45f;   // interior ring
static const float RING2_SCALE = 0.90f;   // outer ring

// Base directions (unit-ish disk, as in R1).
static const float2 SSR_KERNEL[8] = {
    float2( 1.00,  0.00), float2(-1.00,  0.00),
    float2( 0.00,  1.00), float2( 0.00, -1.00),
    float2( 0.71,  0.71), float2(-0.71,  0.71),
    float2( 0.71, -0.71), float2(-0.71, -0.71),
};

// Ring-2 directions: base rotated by 22.5 degrees (precomputed literals).
static const float2 SSR_KERNEL_R2[8] = {
    float2( 0.92388,  0.38268), float2(-0.92388, -0.38268),
    float2(-0.38268,  0.92388), float2( 0.38268, -0.92388),
    float2( 0.38425,  0.92766), float2(-0.38425, -0.92766),
    float2( 0.92766, -0.38425), float2(-0.92766,  0.38425),
};

// ── Shared math (mirrors trace shader — same conventions) ──

float3 SSR_DecodeWorldNormalR(int2 px, int2 size)
{
  int2 tc = clamp(px, int2(0, 0), size - int2(1, 1));
  uint4 mrt = g_srcMrtNormal.Load(int3(tc, 0));
  float2 enc = float2(mrt.x, mrt.y) * (1.0 / 32767.5) - 1.0;
  float azimuth = 3.14159274 * enc.x;
  float ring = sqrt(saturate(1.0 - enc.y * enc.y));
  float3 n = float3(cos(azimuth) * ring, sin(azimuth) * ring, enc.y);
  if (dot(n, n) < 1e-6) n = float3(0.0, 0.0, -1.0);
  return normalize(n);
}

float SSR_FetchRoughness(int2 px, int2 size)
{
  int2 tc = clamp(px, int2(0, 0), size - int2(1, 1));
  uint mat_idx = min(g_srcMrtMatIdx.Load(int3(tc, 0)).x, 19999u);
  return g_deferredParams[mat_idx * 9u + 5u].z;
}

float3 SSR_ViewNormalR(float3 n_world)
{
  return normalize(mul(n_world, (float3x3)view_g));
}

// ── Phase R2C estimator helpers ─────────────────────────────────────────────
// FRESNEL CONVENTION (audited): the lighting PS blends our t25 RGB over the
// env sample and applies its specular modulation chain AFTER the blend point,
// identically to both paths. Mirror mode proves the host expects
// radiance-scale input. Therefore F appears ONLY inside the ratio weights
// (shaping which samples dominate); it never scales output magnitude —
// exactly one effective Fresnel application, supplied by the host.
static const float W_CLAMP = 50.0;   // BRDF/PDF ratio spike guard (baked)

float SSR_GGXD_R(float alpha, float n_h)
{
  float a2 = alpha * alpha;
  float d = (n_h * n_h) * (a2 - 1.0) + 1.0;
  return a2 / (3.14159265 * d * d);
}

float SSR_SmithG1_R(float alpha, float n_x)
{
  float a2 = alpha * alpha;
  return 2.0 * n_x / (n_x + sqrt(a2 + (1.0 - a2) * n_x * n_x));
}

float SSR_SchlickR(float f0, float v_h)
{
  float o = 1.0 - v_h;
  float p5 = o * o; p5 *= p5 * o;
  return saturate(f0 + (1.0 - f0) * p5);
}

float3 SSR_OctDecode(float2 e)
{
  e = e * 2.0 - 1.0;
  float3 n = float3(e.x, e.y, 1.0 - abs(e.x) - abs(e.y));
  float t = saturate(-n.z);
  n.x += (n.x >= 0.0) ? -t : t;
  n.y += (n.y >= 0.0) ? -t : t;
  return normalize(n);
}

// Unproject UV + raw hardware depth to view space (trace-identical pattern).
float3 SSR_UnprojectViewPos(float2 uv, float hw_depth)
{
  float ndc_x = uv.x * 2.0 - 1.0;
  float ndc_y = 1.0 - uv.y * 2.0;
  float4 clip = float4(ndc_x, ndc_y, hw_depth, 1.0);
  float4 vp = mul(clip, projInv_g);
  return vp.xyz / vp.w;
}

// Project a view-space position back to UV (mirror of SSR_ProjectToScreen).
float2 SSR_ProjectUV(float3 view_pos)
{
  float4 clip = mul(float4(view_pos, 1.0), proj_g);
  float2 ndc = clip.xy / clip.w;
  return float2(ndc.x * 0.5 + 0.5, 1.0 - (ndc.y * 0.5 + 0.5));
}

// ── Phase R3: filtered radiance mip selection ──────────────────────────────
// Lobe half-angle tangent = grazing-stretched α × Frostbite elongation shrink.
// Footprint radius at the hit = travel distance × cone tangent; projected to
// pixels with the vertical perspective scale; log₂ → mip. v1 heuristic
// (z_hit approximated by the receiver's depth) — experiment first.
static const float SSR_RAD_MAX_MIP = 7.0;

float SSR_RadianceMip(float alpha_r, float n_dot_v, float travel_dist,
                      float z_hit, float screen_h)
{
  const float a_e   = alpha_r / max(n_dot_v, 0.15);
  const float elong = lerp(saturate(n_dot_v * 2.0), 1.0, sqrt(alpha_r));
  const float cone  = saturate(a_e * elong);
  const float R_w   = travel_dist * cone;
  const float f_y   = proj_g[1][1] * 0.5 * screen_h;
  const float px    = R_w * f_y / max(abs(z_hit), EPS);
  return clamp(log2(max(px, 1.0)), 0.0, SSR_RAD_MAX_MIP);
}

// ── Phase R2C: receiver-side BRDF/PDF ratio for a reused direction ──
// Pure function (no side effects). Returns:
//   ratio > 0 : estimator weight B_receiver(l)/pdf_stored (unclamped)
//   ratio = 1 : sentinel pdf ≤ 0 (mirror/diagnostic ray) → neutral legacy weight
//   ratio < 0 : reused direction below receiver horizon → drop tap
// F0 scalarized as the mean of the MRT1 specular color; F shapes selection
// only — output magnitude is set by the host lighting chain (audited).
float SSR_EstRatio(float meta_pdf, float3 l_v,
                   float3 n_c, float3 V_c, float3 F0_c, float alpha_r)
{
  if (meta_pdf <= 0.0) return 1.0;
  const float nl = dot(n_c, l_v);
  if (nl <= 0.0) return -1.0;
  const float nv = saturate(dot(n_c, V_c));
  const float3 h = normalize(V_c + l_v);
  const float nh = saturate(dot(n_c, h));
  const float vh = saturate(dot(V_c, h));
  const float f0m = (F0_c.x + F0_c.y + F0_c.z > 0.0)
      ? dot(F0_c, float3(0.3333, 0.3334, 0.3333)) : 0.04;
  const float D = SSR_GGXD_R(alpha_r, nh);
  const float G2 = SSR_SmithG1_R(alpha_r, nv) * SSR_SmithG1_R(alpha_r, nl);
  const float F = SSR_SchlickR(f0m, vh);
  return D * G2 * F / (4.0 * max(nv, EPS) * max(meta_pdf, EPS_PDF));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
  uint2 screen_size;
  g_outResolved.GetDimensions(screen_size.x, screen_size.y);
  if (dispatchThreadID.x >= screen_size.x || dispatchThreadID.y >= screen_size.y) return;

  const int2 px = int2(dispatchThreadID.xy);

  // ── Wired resolve parameters (sliders) ──
  const float radius  = clamp(SSR_resolve_radius_max,   1.0f, 16.0f);   // "Resolve Radius"
  const float sigma_z = SSR_resolve_depth_sigma;                          // "Depth Sigma"
  const float sigma_n = SSR_resolve_normal_sigma;                         // "Normal Sigma"

  // ── Phase R3-MV: motion vector debug view ──
  // Slider index 38 translates to code 43 (38+5). Shows amplified DLSS MVs.
  if ((uint)(SSR_debug_view + 0.5) == 43u) {
    float2 mv = g_motionVectors.Load(int3(dispatchThreadID.xy, 0)).xy;
    g_outResolved[dispatchThreadID.xy] =
        float4(mv.x * 20.0 + 0.5, mv.y * 20.0 + 0.5, length(mv) * 10.0, 1.0);
    return;
  }

  // ── Center data ──
  const float4 center_ray = g_rayResult.Load(int3(px, 0));
  const float hw_depth = g_hizChain.Load(int3(px, 0));
  const float z_c = SSR_LinearizeDepth(hw_depth);
  const float3 n_view_c = SSR_ViewNormalR(SSR_DecodeWorldNormalR(px, int2(screen_size)));
  const float rough_c = SSR_FetchRoughness(px, int2(screen_size));

  // ── Phase R2C receiver setup ──
  const float2 uv = (float2(dispatchThreadID.xy) + 0.5) / float2(screen_size);
  const float3 P_view_c = SSR_UnprojectViewPos(uv, hw_depth);
  const float3 V_c = normalize(-P_view_c);
  uint4 spec_c = g_srcMrtSpec.Load(int3(px, 0));
  const float3 F0_c = float3(spec_c.w, spec_c.x, spec_c.y) / 255.0;
  float alpha_r = (SSR_rough_interp > 0.5) ? rough_c : rough_c * rough_c;
  alpha_r = max(alpha_r, 1e-3);

  // ── Accumulation ──
  float3 sum_rad = 0.0.xxx;
  float sum_w = 0.0;

  // ResolveProbe diagnosis counters (row y26, probe pixel only).
  uint   rp_valid = 0, rp_alpha = 0, rp_depth = 0, rp_norm = 0, rp_rough = 0;
  float  rp_nbr_w = 0.0;
  const float rp_center_w = (center_ray.a > 0.0) ? center_ray.a : 0.0;

  // R2C estimator diagnosis (row y28, probe pixel only).
  float rp_nbr_est = 0.0;      // Σ estimator ratio over accepted neighbor taps
  uint  rp_clamps  = 0;
  float rp_pdf_acc = 0.0;
  uint  rp_pdf_n   = 0;
  float rp_cov_ev  = 0.0;      // R2D: Σ accepted-tap credibility evidence
  float rp_mip_acc = 0.0;      // R3: Σ selected mip over pyramid-fetched taps
  uint  rp_mip_n   = 0;
  // Receiver-side BRDF/PDF ratio for reused directions uses the pure
  // SSR_EstRatio() helper above, parameterized with receiver data.
  // pdf ≤ 0 sentinel (mirror/diagnostic ray) → neutral legacy weight;
  // negative return = below receiver horizon → tap dropped.

  // Center contribution (only when the center ray was accepted).
  if (center_ray.a > 0.0) {
    const float4 cm = g_rayMeta.Load(int3(px, 0));
    float est_c = SSR_EstRatio(cm.r, SSR_OctDecode(cm.ba),
                               n_view_c, V_c, F0_c, alpha_r);
    if (est_c < 0.0) est_c = 0.0;
    if (est_c > W_CLAMP) { ++rp_clamps; est_c = W_CLAMP; }
    sum_rad += center_ray.rgb * center_ray.a * est_c;
    sum_w += center_ray.a * est_c;
  }

  // ── Neighborhood taps: dual-ring, 16 taps (Phase R2A) ──
  // Ring 1 (i<8):  base dirs at 0.45 x radius — interior coverage.
  // Ring 2 (i>=8): 22.5-deg-rotated dirs at 0.90 x radius.
  for (uint i = 0; i < SSR_RESOLVE_MAX_TAPS; ++i) {
    const bool outer = (i >= 8u);
    // i&7 keeps both sides of the ternary in-bounds under fxc's full unroll.
    const float2 dir = outer ? SSR_KERNEL_R2[i & 7u] : SSR_KERNEL[i & 7u];
    const float2 offset_px = dir * ((outer ? RING2_SCALE : RING1_SCALE) * radius);
    const int2 tap_px = px + int2(round(offset_px));
    if (tap_px.x < 0 || tap_px.y < 0 ||
        tap_px.x >= (int)screen_size.x || tap_px.y >= (int)screen_size.y)
      { ++rp_alpha; continue; }   // out-of-bounds counts as alpha-class

    const float4 nr = g_rayResult.Load(int3(tap_px, 0));
    if (nr.a <= 0.0) { ++rp_alpha; continue; }   // neighbor has no accepted ray

    const float hw_n = g_hizChain.Load(int3(tap_px, 0));
    const float z_n = SSR_LinearizeDepth(hw_n);
    const float3 n_v_n = SSR_ViewNormalR(SSR_DecodeWorldNormalR(tap_px, int2(screen_size)));
    const float r_n = SSR_FetchRoughness(tap_px, int2(screen_size));

    // Rejects (Phase R1 spec — gate-only roughness). First-fail counting.
    if (abs(z_n - z_c) > z_c * kDepthThresh) { ++rp_depth; continue; }
    if (dot(n_v_n, n_view_c) < kNormalMin)   { ++rp_norm;  continue; }
    if (abs(r_n - rough_c) > kRoughMax)      { ++rp_rough; continue; }

    // Weights: confidence × depth-similarity × normal-similarity × distance.
    // Phase R1 FIX: falloff domain = 2×radius. The old domain (== radius) sat
    // exactly at/below every tap distance (cardinals 2.000, diagonals 2.008),
    // making w_dist identically 0 for ALL taps — resolve equaled raw bit-exact.
    // With edge = 4px: cardinals ≈ 0.500, diagonals ≈ 0.497, all nonzero.
    const float w_conf = saturate(nr.a);
    const float w_depth = exp(-abs(z_n - z_c) / max(z_c * sigma_z, EPS));
    const float w_norm = pow(max(dot(n_v_n, n_view_c), 0.0), sigma_n);
    const float w_dist = 1.0 - smoothstep(0.0, radius * 2.0, length(offset_px));

    // R2D coverage evidence: quality credibility WITHOUT kernel taper /
    // estimator ratio, so coverage reflects independent corroboration.
    rp_cov_ev += w_conf * w_depth * saturate(dot(n_v_n, n_view_c));

    // ── Phase R2C: BRDF/PDF ratio estimator weight ──
    // Reused direction from the neighbor's metadata; receiver-side B_r via
    // SSR_EstRatio (pdf ≤ 0 sentinel → neutral 1.0; below-horizon → drop).
    const float4 nm = g_rayMeta.Load(int3(tap_px, 0));
    float est = SSR_EstRatio(nm.r, SSR_OctDecode(nm.ba),
                             n_view_c, V_c, F0_c, alpha_r);
    if (est < 0.0) { ++rp_alpha; continue; }        // below horizon → drop
    if (est > W_CLAMP) { ++rp_clamps; est = W_CLAMP; }
    if (nm.r > EPS_PDF) { rp_pdf_acc += nm.r; ++rp_pdf_n; }

    // ── Phase R3: filtered radiance fetch ──
    // Reconstruct the hit position from stored direction + travel distance,
    // project to UV, select mip from the receiver's lobe footprint, and fetch
    // pre-averaged radiance. Falls back to the raw texel when the pyramid is
    // inactive (radSrc != 0 / build skipped) via the c12 flag.
    float3 rad_src = nr.rgb;
    float mip_used = 0.0;
    if (SSR_half_res_trace > 0.5 && nm.r > EPS_PDF && nm.g > 0.0) {
      const float3 l_v = SSR_OctDecode(nm.ba);
      const float3 P_hit = P_view_c + l_v * nm.g;
      const float2 uv_hit = SSR_ProjectUV(P_hit);
      if (uv_hit.x >= 0.0 && uv_hit.x <= 1.0 && uv_hit.y >= 0.0 && uv_hit.y <= 1.0) {
        const float nv_r = saturate(dot(n_view_c, V_c));
        mip_used = SSR_RadianceMip(alpha_r, nv_r, nm.g, abs(P_hit.z), screen_size.y);
        rad_src = g_radPyr.SampleLevel(g_samplerLinearClamp, uv_hit, mip_used).rgb;
        rp_mip_acc += mip_used; ++rp_mip_n;
      }
    }

    const float w = w_conf * w_depth * w_norm * w_dist * est;
    sum_rad += rad_src * w;
    sum_w += w;
    rp_nbr_w += w;
    rp_nbr_est += est;
    ++rp_valid;
  }

  // ── Output ──
  const float4 raw_center = center_ray;
  float4 result;
  result.rgb = (sum_w > EPS) ? (sum_rad / sum_w) : float3(0.0, 0.0, 0.0);

  // ── Phase R2D: reconstruction-aware coverage (A/B experiment) ──
  // Bounded evidence signal: 0 = no credible accepted neighbors, rising with
  // independent corroborating taps, saturating at COV_EVIDENCE_NORM.
  // resolvedA = max(center confidence, reconstructed coverage). RGB untouched.
  const float cov = saturate(rp_cov_ev / COV_EVIDENCE_NORM);
  result.a = max(raw_center.a, cov);

  // ── RvS + ResolveProbe rows at the CompareProbe pixel ──
  if (px.x == (int)clamp(SSR_probe_pixel_x, 0.0, screen_size.x - 1.0) &&
      px.y == (int)clamp(SSR_probe_pixel_y, 0.0, screen_size.y - 1.0)) {
    g_stats[uint2(0u, SSR_RVS_ROW)] = asuint(raw_center.r);
    g_stats[uint2(1u, SSR_RVS_ROW)] = asuint(raw_center.g);
    g_stats[uint2(2u, SSR_RVS_ROW)] = asuint(raw_center.b);
    g_stats[uint2(3u, SSR_RVS_ROW)] = asuint(result.rgb.r);
    g_stats[uint2(4u, SSR_RVS_ROW)] = asuint(result.rgb.g);
    g_stats[uint2(5u, SSR_RVS_ROW)] = asuint(result.rgb.b);
    g_stats[uint2(6u, SSR_RVS_ROW)] = asuint(raw_center.a);
    g_stats[uint2(7u, SSR_RVS_ROW)] = asuint(result.a);

    g_stats[uint2(0u, SSR_RPROBE_ROW)] = rp_valid;
    g_stats[uint2(1u, SSR_RPROBE_ROW)] = rp_alpha;
    g_stats[uint2(2u, SSR_RPROBE_ROW)] = rp_depth;
    g_stats[uint2(3u, SSR_RPROBE_ROW)] = rp_norm;
    g_stats[uint2(4u, SSR_RPROBE_ROW)] = rp_rough;
    g_stats[uint2(5u, SSR_RPROBE_ROW)] = asuint(rp_center_w);
    g_stats[uint2(6u, SSR_RPROBE_ROW)] = asuint(rp_nbr_w);
    g_stats[uint2(7u, SSR_RPROBE_ROW)] = asuint(sum_w);

    // Phase R2C estimator diagnosis (y28).
    g_stats[uint2(0u, SSR_EST_ROW)] = asuint(rp_nbr_est);
    g_stats[uint2(1u, SSR_EST_ROW)] = asuint(sum_w);
    g_stats[uint2(2u, SSR_EST_ROW)] = asuint(rp_pdf_n > 0 ? rp_pdf_acc / rp_pdf_n : 0.0);
    g_stats[uint2(3u, SSR_EST_ROW)] = asuint(rp_clamps);
    g_stats[uint2(4u, SSR_EST_ROW)] = asuint(rp_mip_n > 0 ? rp_mip_acc / rp_mip_n : 0.0);
    g_stats[uint2(5u, SSR_EST_ROW)] = asuint(cov);
    g_stats[uint2(6u, SSR_EST_ROW)] = asuint(result.a);
  }

  // ── Raw-vs-Resolved split A/B (c11 SSR_denoise_taps repurposed as flag) ──
  // Left half = untouched raw ray result; right half = resolved radiance.
  // Alpha identical on both halves.
  if (SSR_denoise_taps > 0.5 && dispatchThreadID.x < screen_size.x / 2u) {
    result.rgb = raw_center.rgb;
  }

  // ── Phase R3-wiring: SSR Intensity ──
  // Scales the resolved contribution (0-2). Applied after the split override
  // so both halves scale identically. Requires Spatial Resolve ON — the
  // no-resolve production path bypasses this shader entirely.
  result.rgb *= max(SSR_intensity, 0.0);

  g_outResolved[dispatchThreadID.xy] = result;
}
