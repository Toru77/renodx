///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// falcomengine-plus — Custom SSR (Sora 2nd) — Pass 2: stochastic/mirror trace
//
// Phase 3: Heitz GGX VNDF importance sampling + IS-FAST noise + radiance fetch.
// The hierarchical traversal, validation funnel, positive-Z contract and every
// previously validated code path are preserved BYTE-FOR-BYTE — the march and
// funnel were extracted verbatim into SSR_TraceRay() so the deterministic
// Mirror regression mode (Stochastic OFF, Forced Ray = Mirror) reproduces the
// Phase 2.9 behavior exactly.
//
// Ray modes (SSR_forced_ray_mode):
//   0 Production   — Stochastic ON: VNDF rays; OFF: deterministic mirror
//   4 Mirror       — forced deterministic mirror regardless of toggle
//   1 Fixed normal  2 Screen diagonal  3 Depth/Floor normal  (diagnostics)
//
// Multi-ray (SSR_ray_count 1..4): heuristic VNDF-weighted accumulation —
//   rgb = sum(radiance_i * w_i) / max(sum(w_i), eps),  w_i = confidence_i * F_i
//   a   = mean(confidence_i) over accepted rays
// This is NOT yet a physically unbiased Monte Carlo estimator (no PDF division);
// it is a stable stochastic signal for Phase 4 spatial reconstruction.
//
// ═══════════════════════════════════════════════════════════════════════════
// POSITIVE-Z CONTRACT — read before touching any depth comparison
// ═══════════════════════════════════════════════════════════════════════════
// The engine's view space has NEGATIVE Z in front of the camera. The Hi-Z
// pyramid stores POSITIVE distance (= -viewZ), built by ssr_hiz_base.
// ALL traversal math operates on positive distance. The ONLY conversions:
//     distance = -viewPos.z      dir.posZ = -dirView.z
// Do NOT mix raw viewPos.z with Hi-Z values anywhere else.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ssr_common.hlsl"

Texture2D<float>    g_hizChain     : register(t0);  // SSR min-Z chain
Texture2D<float>    g_srcDepth     : register(t1);  // game's depthTexture (captured t4)
Texture2D<uint4>    g_srcMrtNormal : register(t2);  // game's mrtTexture0 (captured t1)
Texture2D<uint4>    g_srcMrtSpec   : register(t3);  // game's mrtTexture1 (F0/specular source)
Texture2D<uint2>    g_srcMrtMatIdx : register(t4);  // game's mrtTexture2 (.x = material index)
Texture2D<float4>   g_radiance     : register(t5);  // selected radiance source (bind-time choice)
Texture3D<float2>   g_isfastNoise  : register(t6);  // IS-FAST 128x128x32 RG8
StructuredBuffer<float4> g_deferredParams : register(t7); // DeferredParam stream (9 float4 per material)

SamplerState g_samplerPointClamp  : register(s0);
SamplerState g_samplerLinearClamp : register(s1);

RWTexture2D<float4> g_outRays      : register(u0);  // RGBA16F: rgb=weighted radiance/debug, a=confidence
RWTexture2D<uint>   g_stats        : register(u1);  // 8x13 atomic funnel counters
RWTexture2D<float4> g_rayMeta      : register(u2);  // Phase R2C: r=pdf_L g=viewDist ba=octa(dir)

#define SSR_MAX_MIP 7u

// ── STATS CELLS ──────────────────────────────────────────────────────────
void StatsAdd(uint2 cell, uint v) { InterlockedAdd(g_stats[cell], v); }
void StatsMax(uint2 cell, uint v) { InterlockedMax(g_stats[cell], v); }

static const uint2 CELL_PIXELS        = uint2(0, 0);
static const uint2 CELL_SKY_ORIGIN    = uint2(1, 0);
static const uint2 CELL_DEGENERATE    = uint2(2, 0);
static const uint2 CELL_TRAVERSED     = uint2(3, 0);
static const uint2 CELL_CANDIDATES    = uint2(4, 0);
static const uint2 CELL_REJ_INBOUNDS  = uint2(5, 0);
static const uint2 CELL_REJ_FINITE    = uint2(6, 0);
static const uint2 CELL_REJ_SELF      = uint2(7, 0);
static const uint2 CELL_REJ_SKY       = uint2(0, 1);
static const uint2 CELL_REJ_BACKFACE  = uint2(1, 1);
static const uint2 CELL_REJ_THICKNESS = uint2(2, 1);
static const uint2 CELL_REJ_VIGNETTE  = uint2(3, 1);
static const uint2 CELL_ACCEPTED      = uint2(4, 1);
static const uint2 CELL_BYPASS_OK     = uint2(5, 1);
static const uint2 CELL_RAW_HITS      = uint2(6, 1);
static const uint2 CELL_ITERS_SUM     = uint2(0, 2);
static const uint2 CELL_ITERS_MAX     = uint2(1, 2);
static const uint2 CELL_FINALMIP_SUM  = uint2(2, 2);
static const uint2 CELL_COARSEST_SUM  = uint2(3, 2);
static const uint2 CELL_TERM_CANDIDATE= uint2(0, 3);
static const uint2 CELL_TERM_BUDGET   = uint2(1, 3);
static const uint2 CELL_TERM_DEGEN    = uint2(2, 3);
static const uint2 CELL_TERM_SKYORIGIN= uint2(3, 3);
static const uint2 CELL_DIST_0_1      = uint2(0, 4);
static const uint2 CELL_DIST_1_2      = uint2(1, 4);
static const uint2 CELL_DIST_2_8      = uint2(2, 4);
static const uint2 CELL_DIST_8_32     = uint2(3, 4);
static const uint2 CELL_DIST_32_UP    = uint2(4, 4);
static const uint2 CELL_Z_BEHIND      = uint2(5, 4);
static const uint2 CELL_Z_FRONT       = uint2(6, 4);
static const uint2 CELL_Z_EPS         = uint2(7, 4);
static const uint2 CELL_DOT_M075      = uint2(0, 5);
static const uint2 CELL_DOT_M025      = uint2(1, 5);
static const uint2 CELL_DOT_M000      = uint2(2, 5);
static const uint2 CELL_DOT_P000      = uint2(3, 5);
static const uint2 CELL_DOT_P025      = uint2(4, 5);
  static const uint2 CELL_DOT_P075      = uint2(5, 5);
  // ── Phase 3.Fix17: below-horizon regeneration funnel (row y8) ──
  static const uint2 CELL_BELOW_HORIZON_INIT = uint2(0, 8);  // attempt-0 samples with NdotL<=0
  static const uint2 CELL_REGENERATED        = uint2(1, 8);  // second samples taken
  static const uint2 CELL_REGEN_REJECTED     = uint2(2, 8);  // both attempts below horizon → ray rejected
// Row map (Phase 2.6-3): y7 x0..x2 mrtLen(exact/near/off) x3..x7 mrtDot(>=.99,.95-.99,.90-.95,.80-.90,<.80)
//   y8 x0..x2 reflDelta(<10,10-30,>=30) x3..x6 vGrazing(<=-.2,-.2..0,0..+.2,>+.2)
//   y10 Euclidean thick CDF | y11 Perpendicular thick CDF
//   y12 x0 perpFallback x1 rayExec x2 candidateExec x3 acceptedExec

// ── Projection / reconstruction helpers (unchanged, validated) ──
float3 SSR_ProjectToScreen(float3 view_pos_negz)
{
  float4 clip = mul(float4(view_pos_negz, 1.0), proj_g);
  float2 ndc = clip.xy / clip.w;
  return float3(ndc.x * 0.5 + 0.5,
                1.0 - (ndc.y * 0.5 + 0.5),
                -view_pos_negz.z);
}

float3 SSR_ScreenToViewPos(float3 uv_dist)
{
  float mul_c, add_c;
  SSR_GetDepthUnpackConsts(mul_c, add_c);
  float ndc_z = add_c - mul_c / max(uv_dist.z, 1e-6);
  float4 clip = float4(uv_dist.x * 2.0 - 1.0,
                       (1.0 - uv_dist.y) * 2.0 - 1.0,
                       ndc_z, 1.0);
  float4 vp = mul(clip, projInv_g);
  return vp.xyz / vp.w;
}

void SSR_DecodeMrtNormalRaw(int2 px, int2 size, out float3 n_unit, out float raw_len)
{
  int2 tc = clamp(px, int2(0, 0), size - int2(1, 1));
  uint4 mrt = g_srcMrtNormal.Load(int3(tc, 0));
  float2 enc = float2(mrt.x, mrt.y) * (1.0 / 32767.5) - 1.0;
  float azimuth = 3.14159274 * enc.x;
  float ring = sqrt(saturate(1.0 - enc.y * enc.y));
  float3 n = float3(cos(azimuth) * ring, sin(azimuth) * ring, enc.y);
  raw_len = length(n);
  if (dot(n, n) < 1e-6) n = float3(0.0, 0.0, -1.0);
  n_unit = normalize(n);
}

float3 SSR_DecodeWorldNormal(int2 px, int2 size)
{
  float3 n_unit; float raw_len;
  SSR_DecodeMrtNormalRaw(px, size, n_unit, raw_len);
  return n_unit;
}

bool SSR_DepthNormalView(int2 px, uint2 screen_size, out float3 n_view, out float cross_mag)
{
  n_view = 0.0.xxx;
  cross_mag = 0.0;
  const float2 ts = 1.0 / float2(screen_size);
  const float2 uv_c = (float2(px) + 0.5) * ts;

  float dC = g_srcDepth.Load(int3(px, 0));
  float dL = g_srcDepth.Load(int3(clamp(px - int2(1, 0), int2(0, 0), int2(screen_size) - 1), 0));
  float dR = g_srcDepth.Load(int3(clamp(px + int2(1, 0), int2(0, 0), int2(screen_size) - 1), 0));
  float dT = g_srcDepth.Load(int3(clamp(px - int2(0, 1), int2(0, 0), int2(screen_size) - 1), 0));
  float dB = g_srcDepth.Load(int3(clamp(px + int2(0, 1), int2(0, 0), int2(screen_size) - 1), 0));

  float zC = SSR_LinearizeDepth(dC);
  float zL = SSR_LinearizeDepth(dL);
  float zR = SSR_LinearizeDepth(dR);
  float zT = SSR_LinearizeDepth(dT);
  float zB = SSR_LinearizeDepth(dB);
  if (zC >= SSR_FLT_MAX * 0.5 || zC <= 0.0) return false;
  if (zL >= SSR_FLT_MAX * 0.5 || zL <= 0.0) return false;
  if (zR >= SSR_FLT_MAX * 0.5 || zR <= 0.0) return false;
  if (zT >= SSR_FLT_MAX * 0.5 || zT <= 0.0) return false;
  if (zB >= SSR_FLT_MAX * 0.5 || zB <= 0.0) return false;

  float3 pC = SSR_ScreenToViewPos(float3(uv_c, zC));
  float3 pL = SSR_ScreenToViewPos(float3(uv_c - float2(ts.x, 0), zL));
  float3 pR = SSR_ScreenToViewPos(float3(uv_c + float2(ts.x, 0), zR));
  float3 pT = SSR_ScreenToViewPos(float3(uv_c - float2(0, ts.y), zT));
  float3 pB = SSR_ScreenToViewPos(float3(uv_c + float2(0, ts.y), zB));

  float3 dx = pR - pL;
  float3 dy = pB - pT;
  cross_mag = length(cross(dy, dx));
  if (cross_mag < 1e-12) return false;
  float3 n = normalize(cross(dy, dx));
  float3 V = normalize(-pC);
  if (dot(n, V) < 0.0) n = -n;
  n_view = n;
  return true;
}

float3 SSR_ViewNormalToWorld(float3 n_view)
{
  return mul(n_view, (float3x3)viewInv_g);
}

uint2 SSR_MipResolution(uint2 screen_size, uint mip)
{
  return max(screen_size >> mip, uint2(1, 1));
}

float SSR_LoadHiz(uint2 px, uint mip, uint2 screen_size)
{
  uint2 res = SSR_MipResolution(screen_size, mip);
  uint2 tc = min(px, res - uint2(1, 1));
  return g_hizChain.Load(int3(tc, mip));
}

void SSR_InitialAdvanceRay(float3 o, float3 d, float3 inv_d,
                           float2 mip_res, float2 mip_res_inv,
                           float2 floor_offset, float2 uv_offset,
                           out float3 position, out float current_t)
{
  float2 mip_position = mip_res * o.xy;
  float2 xy_plane = floor(mip_position) + floor_offset;
  xy_plane = xy_plane * mip_res_inv + uv_offset;
  float2 t_xy = (xy_plane - o.xy) * inv_d.xy;
  current_t = min(t_xy.x, t_xy.y);
  position = o + current_t * d;
}

bool SSR_AdvanceRay(float3 o, float3 d, float3 inv_d,
                    float2 mip_position, float2 mip_res_inv,
                    float2 floor_offset, float2 uv_offset,
                    float surface_z, inout float3 position, inout float current_t)
{
  float2 xy_plane = floor(mip_position) + floor_offset;
  xy_plane = xy_plane * mip_res_inv + uv_offset;
  float3 boundary_planes = float3(xy_plane, surface_z);

  float3 t_all = (boundary_planes - o) * inv_d;
  t_all.z = d.z > 0 ? t_all.z : SSR_FLT_MAX;

  float t_min = min(min(t_all.x, t_all.y), t_all.z);

  bool above_surface = surface_z > position.z;
  bool skipped_tile = t_min != t_all.z && above_surface;

  current_t = above_surface ? t_min : current_t;
  position = o + current_t * d;
  return skipped_tile;
}

float3 SSR_Heat3(float t)
{
  float tt = saturate(t);
  return float3(saturate(tt * 2.0),
                saturate(1.0 - abs(tt * 2.0 - 1.0)),
                saturate(2.0 - tt * 2.0));
}

// ── Heitz 2018 GGX VNDF sampling (verbatim port, tangent space z-up) ──
float3 SSR_SampleGGXVNDF(float3 Ve, float alpha_x, float alpha_y, float U1, float U2)
{
  float3 Vh = normalize(float3(alpha_x * Ve.x, alpha_y * Ve.y, Ve.z));
  float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
  float3 T1 = lensq > 0 ? float3(-Vh.y, Vh.x, 0) * rsqrt(lensq) : float3(1, 0, 0);
  float3 T2 = cross(Vh, T1);
  float r = sqrt(U1);
  float phi = 2.0 * 3.14159265 * U2;
  float t1 = r * cos(phi);
  float t2 = r * sin(phi);
  float s = 0.5 * (1.0 + Vh.z);
  t2 = (1.0 - s) * sqrt(max(0.0, 1.0 - t1 * t1)) + s * t2;
  float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;
  return normalize(float3(alpha_x * Nh.x, alpha_y * Nh.y, max(0.0, Nh.z)));
}

// IS-FAST 2D sample with IGN fallback (same pattern as GTVBAO main pass).
float2 SSR_Noise2(uint2 p, uint frame)
{
  float3 uvw = float3((float)(p.x % 128u) / 128.0,
                      (float)(p.y % 128u) / 128.0,
                      (float)(frame % 32u) / 32.0);
  return g_isfastNoise.SampleLevel(g_samplerPointClamp, uvw, 0);
}
float SSR_IGN(uint2 p)
{
  return frac(52.9829189 * frac(0.06711056 * p.x + 0.00583715 * p.y));
}

float SchlickScalar(float3 F0, float VdotH)
{
  float o = 1.0 - VdotH;
  float p5 = o * o; p5 *= p5 * o;
  return saturate(F0.x + (1.0 - F0.x) * p5);
}

// Phase R1 Probe B: mirrors the Fix11 same-surface classifier for probe rays.
// Returns 1.0 = hit lies on the origin's own planar surface, 0.0 = different
// surface, -1.0 = ray not accepted.
float SSR_ProbeSameSurface(float2 hit_uv, float3 origin_v, float3 n_view, uint2 screen_size)
{
  int2 hp = clamp((int2)(hit_uv * float2(screen_size)), int2(0, 0), (int2)screen_size - int2(1, 1));
  float3 nh_w = SSR_DecodeWorldNormal(hp, int2(screen_size));
  float3 nh_v = normalize(mul(nh_w, (float3x3)view_g));
  float sz = g_hizChain.Load(int3(hp, 0)).r;
  float3 ph_v = SSR_ScreenToViewPos(float3(hit_uv, max(sz, 0.001)));
  float n_sim = dot(n_view, nh_v);
  float plane_d = abs(dot(ph_v - origin_v, n_view));
  return (n_sim >= 0.95 && plane_d <= max(SSR_plane_delta_threshold, 0.001)) ? 1.0 : 0.0;
}

// Material parameter fetches through the deferred-parameter stream.
// DeferredParam stride = 144 bytes = 9 float4s. Roughness @ offset 88
// => float4 #5, component .z. Specular F0 comes from MRT1 (see below).
float SSR_FetchMaterialRoughness(uint mat_idx)
{
  return g_deferredParams[mat_idx * 9u + 5u].z;
}

// ── Phase R2C estimator helpers ──
// GGX normal distribution (Walter). α = perceptual².
float SSR_GGXD(float alpha, float n_h)
{
  float a2 = alpha * alpha;
  float d = (n_h * n_h) * (a2 - 1.0) + 1.0;
  return a2 / (3.14159265 * d * d);
}

// Smith masking, monodirectional G1(x = N·V or N·L).
float SSR_SmithG1(float alpha, float n_x)
{
  float a2 = alpha * alpha;
  return 2.0 * n_x / (n_x + sqrt(a2 + (1.0 - a2) * n_x * n_x));
}

// Octahedral encode of a unit view-space direction → [0,1]².
float2 SSR_OctEncode(float3 n)
{
  n /= (abs(n.x) + abs(n.y) + abs(n.z));
  float2 e = n.xy;
  if (n.z < 0.0)
    e = (1.0 - abs(e.yx)) * float2(e.x >= 0.0 ? 1.0 : -1.0,
                                   e.y >= 0.0 ? 1.0 : -1.0);
  return e * 0.5 + 0.5;
}

struct SSR_RayOut
{
  float3 hit_pos;
  bool   raw_hit;
  bool   accepted;
  bool   exhausted;
  float  confidence;
  int    reject_reason;     // 0 pass 1 offscreen/vig 2 self 3 finite/sky 4 bf 5 thickness 6 miss
  uint   iterations;
  uint   final_mip;
  uint   coarsest_mip;
};

// Per-ray march + validation funnel — extracted VERBATIM from the validated
// Phase 2.9 inline implementation. Do not "optimize" without re-running the
// deterministic regression protocol.
void SSR_TraceRay(float3 origin_pz, float3 dir_pz, uint2 screen_size,
                  bool sky_pixel, bool ray_valid_geom,
                  float3 r_view, float2 uv, bool stats,
                  out SSR_RayOut R, out float first_disp_px)
{
  R.hit_pos = origin_pz;
  R.raw_hit = false; R.accepted = false; R.exhausted = false; R.confidence = 0.0;
  R.reject_reason = 6; R.iterations = 0; R.final_mip = 0; R.coarsest_mip = 0;
  first_disp_px = 0.0;

  // Execution counters are never gated — they count every logical ray.
  StatsAdd(uint2(1u, 12u), 1u);

  if (!sky_pixel && ray_valid_geom) {
    const bool S = stats;   // legacy population/funnel counters: ray 0 only
    if (S) StatsAdd(CELL_TRAVERSED, 1u);

    const float3 inv_d = dir_pz != 0.0 ? 1.0 / dir_pz : SSR_FLT_MAX.xxx;
    int current_mip = 0;
    float2 mip_res = float2(screen_size);
    float2 mip_res_inv = 1.0 / mip_res;
    float2 uv_offset = 0.005 / float2(screen_size);
    uv_offset = dir_pz.xy < 0.0 ? -uv_offset : uv_offset;
    float2 floor_offset = dir_pz.xy < 0.0 ? 0.0.xx : 1.0.xx;

    float t_current;
    SSR_InitialAdvanceRay(origin_pz, dir_pz, inv_d,
                          mip_res, mip_res_inv, floor_offset, uv_offset,
                          R.hit_pos, t_current);

    // ── Phase 2.4 initial-advance minimum displacement (screen-space px) ──
    {
      float speed_px = length(dir_pz.xy * float2(screen_size));
      float bias_px = max(SSR_initial_advance_bias, 0.0);
      if (bias_px > 0.0 && speed_px > 1e-6) {
        float t_need = bias_px / speed_px;
        if (t_current < t_need) {
          t_current = t_need;
          R.hit_pos = origin_pz + t_current * dir_pz;
        }
      }
      first_disp_px = length((R.hit_pos.xy - uv) * float2(screen_size));
    }

    const uint max_steps = (uint)max(SSR_max_traversal_steps, 1.0);
    bool exhausted = false;
    [loop]
    while (true) {
      if (R.iterations >= max_steps) { exhausted = true; break; }
      if (current_mip < 0) break;
      R.final_mip = min((uint)max(current_mip, 0), SSR_MAX_MIP);
      float2 mip_position_px = mip_res * R.hit_pos.xy;
      float surface_z = SSR_LoadHiz((uint2)floor(mip_position_px),
                                    (uint)max(current_mip, 0), screen_size);
      bool skipped_tile = SSR_AdvanceRay(origin_pz, dir_pz, inv_d,
                                         mip_position_px, mip_res_inv,
                                         floor_offset, uv_offset, surface_z,
                                         R.hit_pos, t_current);
      R.coarsest_mip = max(R.coarsest_mip, (uint)max(current_mip, 0));
      current_mip += skipped_tile ? 1 : -1;
      mip_res *= skipped_tile ? 0.5 : 2.0;
      mip_res_inv *= skipped_tile ? 2.0 : 0.5;
      ++R.iterations;
    }
    R.exhausted = exhausted;

    R.raw_hit = !exhausted && (current_mip < 0);
    if (R.raw_hit) StatsAdd(uint2(2u, 12u), 1u);   // candidateExecution
    if (S) {
      if (R.raw_hit) {
        StatsAdd(CELL_CANDIDATES, 1u);
        StatsAdd(CELL_TERM_CANDIDATE, 1u);
      } else if (exhausted) {
        StatsAdd(CELL_TERM_BUDGET, 1u);
      }
    }

    // ── Validation funnel (verbatim Phase 2.9 logic) ──
    float conf_chain = 0.0;
    if (!R.raw_hit) {
      R.reject_reason = 6;
    } else {
      if (S) StatsAdd(CELL_RAW_HITS, 1u);
      const float2 hit_uv = R.hit_pos.xy;
      const bool in_bounds = all(hit_uv >= 0.0) && all(hit_uv <= 1.0);
      if (!in_bounds) { StatsAdd(CELL_REJ_INBOUNDS, 1u); R.reject_reason = 1; }
      else {
        float surf_z_at_hit = SSR_LoadHiz((uint2)(hit_uv * float2(screen_size)),
                                          0u, screen_size);
        const bool finite_depth = (surf_z_at_hit < SSR_FLT_MAX * 0.5) &&
                                  (surf_z_at_hit > 0.0);
        if (!finite_depth) { if (S) StatsAdd(CELL_REJ_FINITE, 1u); R.reject_reason = 3; }
        else {
          const float self_thr = max(SSR_self_hit_threshold_px, 0.0);
          const bool gate_self = all(abs(hit_uv - uv) * float2(screen_size) < self_thr);
          if (gate_self && S) StatsAdd(CELL_REJ_SELF, 1u);

          bool gate_bf = false, gate_thick = false, gate_vig = false;
          conf_chain = 1.0;
          if (!gate_self) {
            float3 r_world = normalize(mul((float3x3)viewInv_g, r_view));
            float3 n_hit_world = SSR_DecodeWorldNormal(
                (int2)(hit_uv * float2(screen_size)), int2(screen_size));
            float bf_dot = dot(n_hit_world, r_world);
            if (bf_dot > 0.0 && SSR_backface_gate > 0.5) {
              gate_bf = true; if (S) StatsAdd(CELL_REJ_BACKFACE, 1u);
            } else {
              float3 view_surf = SSR_ScreenToViewPos(float3(hit_uv, surf_z_at_hit));
              float3 view_hit_vs = SSR_ScreenToViewPos(R.hit_pos);
              float3 delta_vs = view_surf - view_hit_vs;
              float dist_eucl = length(delta_vs);
            {
              uint tb = dist_eucl < 0.01 ? 0u : dist_eucl < 0.025 ? 1u
                      : dist_eucl < 0.05 ? 2u : dist_eucl < 0.1 ? 3u
                      : dist_eucl < 0.15 ? 4u : dist_eucl < 0.25 ? 5u
                      : dist_eucl < 0.5 ? 6u : 7u;
              if (S) StatsAdd(uint2(tb, 10u), 1u);
            }
              bool perp_ok = false;
              float d_perp = 0.0;
              {
                int2 hit_px = (int2)(hit_uv * float2(screen_size));
                float3 dn_hit; float cm_unused;
                if (SSR_DepthNormalView(hit_px, screen_size, dn_hit, cm_unused)) {
                  perp_ok = true;
                  d_perp = abs(dot(delta_vs, dn_hit));
                  uint tb = d_perp < 0.01 ? 0u : d_perp < 0.025 ? 1u
                          : d_perp < 0.05 ? 2u : d_perp < 0.1 ? 3u
                          : d_perp < 0.15 ? 4u : d_perp < 0.25 ? 5u
                          : d_perp < 0.5 ? 6u : 7u;
                  if (S) StatsAdd(uint2(tb, 11u), 1u);
                } else {
                  StatsAdd(uint2(0u, 12u), 1u);
                }
              }
              const bool use_perp = (SSR_thickness_mode > 0.5) && perp_ok;
              const float metric = use_perp ? d_perp : dist_eucl;

              float thick = max(SSR_thickness, 1e-4);
              float c_sel = 1.0 - smoothstep(0.0, thick, metric);
              c_sel *= c_sel;
              float2 fov = 0.05 * float2((float)screen_size.y / (float)screen_size.x, 1.0);
              float2 border = smoothstep(0.0.xx, fov, hit_uv)
                            * (1.0.xx - smoothstep(1.0.xx - fov, 1.0.xx, hit_uv));
              float vignette = border.x * border.y;
              const bool thick_gate_on = SSR_thickness_gate > 0.5;
              if (c_sel < 0.01 && S) StatsAdd(CELL_REJ_THICKNESS, 1u);
              if (thick_gate_on && c_sel < 0.01) {
                gate_thick = true;
              } else if (vignette < 0.01) {
                gate_vig = true; if (S) StatsAdd(CELL_REJ_VIGNETTE, 1u);
              }
              conf_chain = thick_gate_on ? saturate(vignette * c_sel)
                                         : saturate(vignette);
            }
          }

          if (SSR_bypass_validation > 0.5) {
            R.confidence = 1.0;
            R.accepted = true;
            R.reject_reason = 0;
            if (S) StatsAdd(CELL_BYPASS_OK, 1u);
          } else if (gate_vig)        { R.reject_reason = 1; }
          else if (gate_thick)        { R.reject_reason = 5; }
          else { R.confidence = conf_chain; R.accepted = true; R.reject_reason = 0; }

          // acceptedExecution counts REAL acceptances only (unconditional).
          if (R.accepted) StatsAdd(uint2(3u, 12u), 1u);
        }
      }
      if (R.accepted && S) StatsAdd(CELL_ACCEPTED, 1u);
      if (S) {
        StatsAdd(CELL_ITERS_SUM, R.iterations);
        StatsMax(CELL_ITERS_MAX, R.iterations);
        if (R.raw_hit) {
          StatsAdd(CELL_FINALMIP_SUM, R.final_mip);
          StatsAdd(CELL_COARSEST_SUM, R.coarsest_mip);
        }
      }
    }
}

}

// MAIN_ANCHOR_A
// ── Per-pixel entry point ────────────────────────────────────────────────
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
  uint2 screen_size;
  g_outRays.GetDimensions(screen_size.x, screen_size.y);
  if (dispatchThreadID.x >= screen_size.x || dispatchThreadID.y >= screen_size.y) return;

  const int debug_mode = (int)(SSR_debug_view + 0.5);
  const bool diagnostics_on = SSR_diagnostics > 0.5;
  const bool stochastic = SSR_stochastic > 0.5;
  const uint forced_mode = (uint)clamp(SSR_forced_ray_mode, 0.0, 7.0);
  const float2 uv = (float2(dispatchThreadID.xy) + 0.5) / float2(screen_size);
  const int2 px = int2(dispatchThreadID.xy);

  // ── Phase R2C.1: default metadata = sentinel ──
  // Written unconditionally BEFORE any early-return (forced debug modes 5/6,
  // sky, ineligibility, ray rejection) so no stale pdf/direction from a
  // previous stochastic frame can ever leak into Mirror/diagnostic modes or
  // the resolve estimator. The accepted-stochastic path overwrites this with
  // real values further down.
  g_rayMeta[dispatchThreadID.xy] = float4(-1.0, 0.0, 0.0, 0.0);

  float4 out_color = float4(0.0, 0.0, 0.0, 0.0);

  StatsAdd(CELL_PIXELS, 1u);

  // ── Ray origin ──
  float hw_depth = g_srcDepth.Load(int3(px, 0));
  float origin_dist = SSR_LinearizeDepth(hw_depth);
  const bool sky_pixel = (hw_depth <= 0.0) || (hw_depth >= 1.0) ||
                         (origin_dist >= SSR_FLT_MAX * 0.5) || (origin_dist <= 0.0);

  float3 view_origin = 0.0.xxx;
  float3 n_view = 0.0.xxx;
  float3 r_mirror = 0.0.xxx;         // deterministic mirror (regression rung)
  float3 origin_pz = float3(uv, 0.0);
  bool ray_valid_geom = false;
  float nv = 0.0;

  if (!sky_pixel) {
    float ndc_x = uv.x * 2.0 - 1.0;
    float ndc_y = 1.0 - uv.y * 2.0;
    float4 clip_pt = float4(ndc_x, ndc_y, hw_depth, 1.0);
    float4 vp = mul(clip_pt, projInv_g);
    view_origin = vp.xyz / vp.w;

    // ══ POSITIVE-Z CONTRACT: single conversion point ══
    origin_pz = float3(uv, -view_origin.z);
    ray_valid_geom = true;
  }

  // ── Normal acquisition + mirror direction ──
  float3 V = 0.0.xxx;
  if (!sky_pixel) {
    V = normalize(-view_origin);
    if (forced_mode == 1u) {
      n_view = normalize(float3(0.0, -0.707, 0.707));
    } else if (forced_mode == 3u) {
      float cm_unused;
      if (!SSR_DepthNormalView(px, screen_size, n_view, cm_unused))
        ray_valid_geom = false;
    } else if (forced_mode == 4u) {
      // Fixed Plane Mirror: known world-space floor normal through canonical
      // path — isolates normal acquisition from reflection-formula correctness.
      n_view = normalize(mul(float3(0, 1, 0), (float3x3)view_g));
    } else {
      float3 n_world = SSR_DecodeWorldNormal(px, int2(screen_size));
      if (SSR_normal_convention > 0.5)
        n_view = normalize(mul(n_world, (float3x3)view_g));   // empirical canonical
      else
        n_view = normalize(mul((float3x3)view_g, n_world));
    }
    nv = dot(n_view, V);
    if (nv <= 0.0) ray_valid_geom = false;                    // incl. forced modes
    r_mirror = 2.0 * nv * n_view - V;                          // reflect(-V, N)
  }

  // ── Material parameters ──
  float rough_used = 0.0;
  float3 F0 = 0.0.xxx;
  uint mat_idx = 19999u;   // Phase 3.Fix14: hoisted — eligibility flag fetch below
  if (!sky_pixel) {
    // F0 from MRT1: identical decode to lighting PS (r9.xyz = mrt1.wzy / 255).
    uint4 spec = g_srcMrtSpec.Load(int3(px, 0));
    F0 = float3(spec.w, spec.x, spec.y) / 255.0;
    mat_idx = min(g_srcMrtMatIdx.Load(int3(px, 0)).x, 19999u);
    rough_used = clamp(SSR_FetchMaterialRoughness(mat_idx),
                       max(SSR_rough_min, 1e-3), max(SSR_rough_max, 1e-3));
  }

  // ── Per-ray execution ─────────────────────────────────────────────────
  // Ray 0 carries diagnostics/counters (comparable to prior phases).
  // Regression: Stochastic OFF or forced != Production => single deterministic
  // mirror ray with byte-equivalent geometry to Phase 2.9 (NO eligibility gate
  // in regression mode — it must reproduce Phase 2 exactly).
  const bool production_stoch = (forced_mode == 0u) && stochastic;
  uint ray_count_eff = production_stoch ? (uint)clamp(SSR_ray_count, 1.0, 4.0) : 1u;

    // ── Mode 5 (Proj Endpoint) + Mode 6 (Linear March): neutral diagnostics ──
    // Both bypass Hi-Z traversal and show simple hit/miss on a neutral
    // background. No arbitrary colour encodings.
    if ((forced_mode == 5u || forced_mode == 6u) && !sky_pixel && ray_valid_geom) {
      const float probe_len = clamp(SSR_max_ray_distance, 1.0, 100000.0);
      float3 end_scr = SSR_ProjectToScreen(view_origin + r_mirror * probe_len);
      float3 dir_diag = end_scr - origin_pz;

      if (forced_mode == 5u) {
        // Proj Endpoint: neutral background with endpoint crosshair marker.
        float ar = (float)screen_size.x / (float)screen_size.y;
        float2 this_px = float2(dispatchThreadID.xy) * float2(1.0, ar);
        float2 euv_px = saturate(end_scr.xy) * float2(screen_size)
                      * float2(1.0, ar);
        float d = length(this_px - euv_px);
        if (d < 2.0)        out_color.rgb = float3(1, 1, 1);       // white: endpoint
        else if (d < 5.0)   out_color.rgb = float3(0.5, 0.5, 0.5); // grey: vicinity
        else                out_color.rgb = float3(0.05, 0.05, 0.05); // dark bg
        out_color.a = 1.0;
        return;
      }

      // Mode 6: linear march — hit/miss with position shown as white dot.
      const uint max_lin = (uint)max(SSR_max_traversal_steps * 4u, 64u);
      const float step_t = probe_len / (float)max_lin;
      float3 lpos = origin_pz;
      bool lin_hit = false;
      [loop]
      for (uint s = 0u; s < max_lin; ++s) {
        lpos += dir_diag * step_t;
        int2 lp = (int2)(lpos.xy * float2(screen_size));
        lp = max(lp, int2(0, 0));
        lp = min(lp, int2(screen_size) - int2(1, 1));
        float sz = g_hizChain.Load(int3(lp, 0));
        if (sz > 0.0 && sz < SSR_FLT_MAX * 0.5 && lpos.z >= sz - 1e-4) {
          lin_hit = true;
          break;
        }
      }
      float ar6 = (float)screen_size.x / (float)screen_size.y;
      float2 tap_uv_px = saturate(lpos.xy) * float2(screen_size) * float2(1.0, ar6);
      float d_tap = length(float2(dispatchThreadID.xy) * float2(1.0, ar6) - tap_uv_px);
      if (lin_hit && d_tap < 2.0)
        out_color.rgb = float3(0.1, 0.9, 0.1);   // green dot: linear march hit
      else if (!lin_hit)
        out_color.rgb = float3(0.15, 0.0, 0.0);  // dark red: no linear hit
      else
        out_color.rgb = float3(0.05, 0.05, 0.08); // near-black background
      out_color.a = lin_hit ? 1.0 : 0.0;
      return;
    }

  // ── Custom Material eligibility (Production path only) ──
  // AMD-SSSR analogue: roughness threshold decides which surfaces spawn rays.
  // All-mode additionally excludes characters (bit 3) and foliage (bit 15).
  // ── Phase 3.Fix14: flag source corrected ──
  // The game's lighting shader reads feature flags from the deferred
  // parameter stream: deferredParams[mat].flag, struct Offset 136 =
  // float4 #8.z (same stride arithmetic as roughness @88 = #5.z, proven).
  // The previous predicate read MRT0.w — a different resource whose bit
  // pattern matched these masks on ~87% of world pixels, falsely excluding
  // them under All-mode. Regression/diagnostic modes trace everything.
  if (production_stoch && ray_count_eff > 0u && !sky_pixel) {
    const uint mat_flag = asuint(g_deferredParams[mat_idx * 9u + 8u].z);
    const bool is_character = (mat_flag & 0x0008u) != 0u;
    const bool is_foliage   = (mat_flag & 0x8000u) != 0u;
    uint elig = (uint)clamp(SSR_eligibility_mode, 0.0, 2.0);
    if (elig == 1u) {
      if (rough_used > max(SSR_eligibility_rough, 0.0)) ray_count_eff = 0u;
    } else if (elig == 2u) {
      if (is_character || is_foliage) ray_count_eff = 0u;
    }
  }

  float3 acc_rad = 0.0.xxx;
  float acc_w = 0.0, acc_conf = 0.0;
  uint acc_hits = 0;
  float3 last_accepted_pos = 0.0.xxx;

  SSR_RayOut r0 = (SSR_RayOut)0;
  r0.reject_reason = 6;
  float r0_disp = 0.0;
  float3 r_vndf_stored = 0.0.xxx;
  float ndotl_sampled = 0.0;
  float rough_viz = rough_used;
  bool probe_flip = false;   // Phase 3.Fix17 vndfProbe: ray REJECTED (both samples below horizon)
  bool ray0_dead = false;    // R2C: ray-0 rejected pre-traversal (both samples below horizon)
  // ── Phase R2E Test 1: per-ray directional BRDF weight (production only) ──
  // Exact-VNDF self ratio collapses to F(V·H)·G1(α,N·L); direction-dependent
  // terms restore the BRDF's falloff across the lobe (Defect-B fix).
  float alpha_used = max((SSR_rough_interp > 0.5) ? rough_used
                                                  : rough_used * rough_used, 1e-3);
  // ── Phase R2C: ray-0 estimator metadata (pdf of the accepted pre-bias sample) ──
  float3 H_acc = 0.0.xxx;    // half-vector of the accepted VNDF sample (view space)
  bool   have_H0 = false;
  float  pdf0 = 0.0;         // p_L at generating pixel, 0 = invalid/sentinel
  // ── Phase 3.Fix17: per-attempt VNDF diagnostics (ray 0, dumped at center) ──
  float probe_a0_dlm = -2.0, probe_a0_ang = -1.0, probe_a0_ndotl = 0.0;
  bool  probe_a0_below = false;
  bool  probe_a1_ran = false;
  float probe_a1_dlm = -2.0, probe_a1_ang = -1.0, probe_a1_ndotl = 0.0;
  bool  probe_a1_below = false;

  [unroll]
  for (uint ri = 0; ri < 4u; ++ri) {
    if (ri >= ray_count_eff) break;
    if (!(ray_valid_geom && !sky_pixel)) break;

    float3 dir_pz = 0.0.xxx;
    float3 r_ray = 0.0.xxx;
    bool ray_dead = false;

    if (forced_mode == 2u) {
      float3 diag = normalize(float3(0.45, 0.60, 0.55));
      r_ray = diag;
      dir_pz = float3(diag.xy, -diag.z);        // ══ positive-Z conversion ══
    } else if (production_stoch) {
      // ── Phase 3.Fix17: regenerate-once, reject-twice below-horizon policy ──
      // Frostbite/Heitz semantics: a sampled L with NdotL<=0 is discarded and
      // re-sampled ONCE with fresh randoms; if the second sample is still
      // below the horizon the ray is rejected outright. No direction flipping
      // (the old flip produced horizon-grazing directions that marched off
      // into nothing — CompareProbe V-side angDeg=162° population).
      float alpha = (SSR_rough_interp > 0.5) ? rough_used
                                             : rough_used * rough_used;
      alpha = max(alpha, 1e-3);
      const float3 T1 = nv < 0.999f ? normalize(cross(n_view, float3(0, 0, 1)))
                                    : float3(1, 0, 0);
      const float3 T2 = cross(n_view, T1);
      const float3 Ve_l = float3(dot(V, T1), dot(V, T2), dot(V, n_view));

      float3 L = 0.0.xxx;
      bool valid_sample = false;
      [loop]
      for (uint att = 0u; att < 2u; ++att) {
        // Attempt 1 uses a fixed noise offset → independent second sample
        // in both IS-FAST and IGN paths; frame scramble preserved.
        const uint2 noff = (att == 0u) ? uint2(0u, 0u) : uint2(137u, 149u);
      float2 U;
      // ── Phase R2E.2: RNG source decoupled from Diagnostics ──
      // Production sampling always prefers the animated IS-FAST volume;
      // IGN is the static-pattern fallback, now with optional golden-ratio
      // per-frame jitter so both paths share temporal animation.
      // (Previously gated on diagnostics_on — a diagnostics flag must never
      // alter rendering output.)
      if (SSR_isfast_active > 0.5) {
        U = SSR_Noise2(dispatchThreadID.xy + noff + uint2(ri * 37u, ri * 17u),
                       (uint)SSR_frame_index);
      } else {
        const float g_jit = float(uint(SSR_frame_index) & 255u) * 0.6180339887;
        U = frac(float2(
            SSR_IGN(dispatchThreadID.xy + noff + uint2(ri * 61u, ri * 29u)),
            SSR_IGN(dispatchThreadID.xy + noff + uint2(ri * 13u + 7u, ri * 7u + 3u))) + g_jit);
      }
        const float3 H_l = SSR_SampleGGXVNDF(Ve_l, alpha, alpha, U.x, U.y);
        const float3 H = T1 * H_l.x + T2 * H_l.y + n_view * H_l.z;
        const float3 Lc = 2.0 * dot(H, V) * H - V;   // reflect(-V, H)
        const bool lc_below = (dot(Lc, n_view) <= 0.0);

        if (lc_below) {
          if (att == 0u) {
            StatsAdd(CELL_BELOW_HORIZON_INIT, 1u);
            StatsAdd(CELL_REGENERATED, 1u);
          } else {
            StatsAdd(CELL_REGEN_REJECTED, 1u);
          }
        } else {
          L = Lc;
          H_acc = H;                       // R2C: pre-bias half-vector of the accepted sample
          valid_sample = true;
        }

        // Ray-0 attempt capture for the center-pixel vndfProbe dump.
        if (ri == 0u &&
            px.x == (int)screen_size.x / 2 && px.y == (int)screen_size.y / 2) {
          const float dlm_c = dot(normalize(Lc), r_mirror);
          const float ang_c = degrees(acos(clamp(dlm_c, -1.0, 1.0)));
          if (att == 0u) {
            probe_a0_dlm = dlm_c; probe_a0_ang = ang_c;
            probe_a0_ndotl = saturate(dot(Lc, n_view));
            probe_a0_below = lc_below;
          } else {
            probe_a1_ran = true;
            probe_a1_dlm = dlm_c; probe_a1_ang = ang_c;
            probe_a1_ndotl = saturate(dot(Lc, n_view));
            probe_a1_below = lc_below;
          }
        }

        if (valid_sample) break;
      }

      if (!valid_sample) {
        ray_dead = true;
        if (ri == 0u) { probe_flip = true; ray0_dead = true; }
      }

      // ── Phase R2C: PDF of the accepted pre-bias sample (ray 0) ──
      // p_L = D(H)·G1(V,H)/(4·NdotV). Exact for the unbiased VNDF sample.
      // Mirror Bias > 0 warps the distribution without a matching PDF —
      // estimator is valid/tested only at Bias = 0 (documented limitation).
      if (valid_sample && ri == 0u && !have_H0) {
        have_H0 = true;
        const float nh_h = saturate(dot(H_acc, n_view));
        const float vh_h = saturate(dot(V, H_acc));
        pdf0 = SSR_GGXD(alpha, nh_h) * SSR_SmithG1(alpha, vh_h)
             / (4.0 * max(nv, 1e-4));
      }

      if (!ray_dead) {
        // Mirror bias: filtered importance sampling (Stachowiak 2015).
        // Applied AFTER a valid above-horizon sample exists; both endpoints
        // are in the positive hemisphere so the blend cannot go below.
        if (SSR_mirror_bias > 0.001) {
          const float3 r_mir = 2.0 * nv * n_view - V;   // reflect(-V, N)
          L = normalize(lerp(L, r_mir, saturate(SSR_mirror_bias)));
        }
        L = normalize(L);
        ndotl_sampled = saturate(dot(n_view, L));
        r_ray = L;
        // ── Phase 3.Fix21 experiment (one-line): project the FINAL VNDF
        // direction through the same AMD ProjectDirection unit-step
        // construction the Mirror path uses (Fix12). View-space components
        // are not screen-space rates in perspective; the raw float3(L.xy,-L.z)
        // form traced a different screen line — catastrophic at grazing
        // angles (floor), benign on frontal walls. Sampling untouched.
        dir_pz = SSR_ProjectToScreen(view_origin + L)
               - float3(uv, -view_origin.z);
      }
    } else {
      r_ray = r_mirror;
      // ── Phase 3.Fix12: AMD-faithful unit-step screen-space direction ──
      // AMD Common.hlsl ProjectDirection: Project(origin + UNIT dir) - origin.
      // Direction construction is decoupled from SSR_max_ray_distance — the
      // marcher terminates via hit / off-screen / mip-exhaustion / budget,
      // exactly like the reference (Intersect.hlsl:209-213).
      float3 o_scr = float3(uv, -view_origin.z);            // positive-Z origin
      float3 u_scr = SSR_ProjectToScreen(view_origin + r_mirror);  // 1 world-unit step
      dir_pz = u_scr - o_scr;

      // Defensive guard only — unit-step deltas are O(0.001..0.5), so this
      // is arithmetically unreachable in valid states.
      if (any(isnan(dir_pz)) || any(abs(dir_pz) > 4.0)) {
        dir_pz = float3(0.0, 0.0, 0.0);
      }
    }
    r_ray = normalize(r_ray);

    SSR_RayOut R;
    float disp;
    if (!ray_dead) {
      SSR_TraceRay(origin_pz, dir_pz, screen_size,
                   sky_pixel, ray_valid_geom, r_ray, uv, ri == 0u,
                   R, disp);
    } else {
      // Phase 3.Fix17: both VNDF samples below horizon → ray rejected
      // before traversal (no march, no counters, no accumulation).
      R = (SSR_RayOut)0;
      R.reject_reason = 6;
      disp = 0.0;
    }

    if (ri == 0) { r0 = R; r0_disp = disp; r_vndf_stored = ray_dead ? 0.0.xxx : r_ray; }
    if (R.raw_hit) {
      // raw_hits counted inside TraceRay (stats-gated) — no duplicate here.
    }

    // ── Phase 3.Fix11: same-surface classification (post-traversal) ──
    // A candidate that hits the same planar surface as the origin pixel is
    // geometrically valid but produces no visually useful reflection content.
    // Classify using normal similarity and perpendicular plane distance;
    // optionally reject to let the cubemap show through on those pixels.
    bool is_same_surface = false;
    if (R.accepted && !sky_pixel && ray_valid_geom) {
      float2 hit_uv = R.hit_pos.xy;
      int2 hit_px = (int2)(hit_uv * float2(screen_size));
      hit_px = clamp(hit_px, int2(0, 0), int2(screen_size) - int2(1, 1));

      // Hit-texel world normal → view space via empirical canonical path.
      float3 n_hit_w = SSR_DecodeWorldNormal(hit_px, int2(screen_size));
      float3 n_hit_v = normalize(mul(n_hit_w, (float3x3)view_g));

      // Hit view-space position from Hi-Z mip0 depth at hit texel.
      float surf_z = g_hizChain.Load(int3(hit_px, 0)).r;
      float3 p_hit_v = SSR_ScreenToViewPos(float3(hit_uv, max(surf_z, 0.001)));

      float3 p_delta = p_hit_v - view_origin;
      float n_sim = dot(n_view, n_hit_v);
      float plane_d = abs(dot(p_delta, n_view));

      const float ssr_plane_thr = max(SSR_plane_delta_threshold, 0.001);
      is_same_surface = (n_sim >= 0.95) && (plane_d <= ssr_plane_thr);

      if (SSR_same_surface_reject > 0.5f && is_same_surface) {
        R.confidence = 0.0;   // reject: cubemap shows through instead
        R.accepted = false;
      }
    }

    // Radiance fetch + accumulation.
    // ── Phase R2E Test 1: direction-dependent BRDF/PDF-consistent weight ──
    // VNDF self-evaluation ratio collapses exactly to F(V·H)·G1(α,N·L):
    //   [D·G2·F/(4nv)] / [D·G1(V,H)/(4nv)] = F · G2/G1 ≈ F · G1(α,N·L)
    // (separable Smith G2 = G1(V,H)·G1(L,H)). This restores the BRDF's
    // angular falloff across the sampled lobe — far-from-mirror /
    // grazing-floor taps are downweighted exactly as the physical integrand
    // demands (Defect-B fix). Mirror/diagnostic modes keep the legacy
    // receiver-Fresnel weight, unchanged.
    if (R.accepted) {
      float3 rad = g_radiance.SampleLevel(g_samplerLinearClamp,
                                          R.hit_pos.xy, 0).rgb;
      float w;
      if (production_stoch) {
        const float3 h_dir  = normalize(V + r_ray);
        const float  nl_r   = saturate(dot(n_view, r_ray));
        const float  vh_r   = saturate(dot(V, h_dir));
        const float  Fr     = SchlickScalar(F0, vh_r);
        const float  Gl     = SSR_SmithG1(alpha_used, nl_r);
        w = saturate(R.confidence) * Fr * Gl;
      } else {
        const float Fold = SchlickScalar(F0, nv);
        w = saturate(R.confidence) * Fold;
      }
      acc_rad += rad * w;
      acc_w += w;
      acc_conf += R.confidence;
      ++acc_hits;
      last_accepted_pos = R.hit_pos;
    }
  }

  // ── Phase R2C: per-pixel ray metadata ──
  // Real values only for accepted stochastic ray-0 samples; sentinel pdf=-1
  // otherwise (mirror/diagnostic/dead). Resolve treats pdf<=0 taps as
  // legacy geometric-weight-only contributions.
  {
    float4 meta = float4(-1.0, 0.0, 0.0, 0.0);
    if (production_stoch && !sky_pixel && ray_valid_geom && have_H0 &&
        dot(r_vndf_stored, r_vndf_stored) > 0.25 && !ray0_dead) {
      const float3 p_hit_v = SSR_ScreenToViewPos(r0.hit_pos);
      meta.x = pdf0;
      meta.y = length(p_hit_v - view_origin);
      meta.zw = SSR_OctEncode(normalize(r_vndf_stored));
    }
    g_rayMeta[dispatchThreadID.xy] = meta;
  }

  // Termination classification for non-marching pixels (once per pixel).
  if (sky_pixel) StatsAdd(CELL_TERM_SKYORIGIN, 1u);
  else if (!ray_valid_geom) StatsAdd(CELL_TERM_DEGEN, 1u);

  // ── Phase 3.Fix9: deterministic geometry probe (screen center, diag ON) ──
  // Encodes probe values as bit-cast floats into stats cells y13-y14.
  // CPU readback decodes with memcpy — lossless float precision.
  if (diagnostics_on && !sky_pixel &&
      px.x == (int)screen_size.x / 2 && px.y == (int)screen_size.y / 2) {
    // Reconstruct view-space position from NDC + hardware depth.
    float ndc_x = uv.x * 2.0 - 1.0;
    float ndc_y = 1.0 - uv.y * 2.0;
    float4 clip_pt = float4(ndc_x, ndc_y, hw_depth, 1.0);
    float4 vp = mul(clip_pt, projInv_g);
    float3 P_view = vp.xyz / vp.w;

    // Known floor normal: world-up through empirical canonical transform.
    float3 N_view = normalize(mul(float3(0, 1, 0), (float3x3)view_g));

    // View vector: surface → camera.
    float3 Vv = normalize(-P_view);

    // Both reflection sign conventions for diagnosis.
    float nv_d = dot(N_view, Vv);
    float3 R_minus = reflect(-Vv, N_view);   // standard: I = -V (into surface)
    float3 R_plus  = reflect(Vv, N_view);    // opposite sign convention

    // Project reflected endpoint through engine proj_g.
    const float probe_dist = 5.0;
    float3 end_scr = SSR_ProjectToScreen(P_view + R_minus * probe_dist);

    // Dot products — sign tells us which reflection is physically valid.
    float d_VN = dot(Vv, N_view);
    float d_RM = dot(R_minus, N_view);
    float d_RP = dot(R_plus, N_view);

    // Encode as bit-cast floats (lossless).
    g_stats[uint2(0, 13)] = asuint(P_view.x);
    g_stats[uint2(1, 13)] = asuint(P_view.y);
    g_stats[uint2(2, 13)] = asuint(P_view.z);
    g_stats[uint2(3, 13)] = asuint(Vv.x);
    g_stats[uint2(4, 13)] = asuint(Vv.y);
    g_stats[uint2(5, 13)] = asuint(Vv.z);
    g_stats[uint2(6, 13)] = asuint(hw_depth);
    g_stats[uint2(7, 13)] = asuint(nv_d);

    g_stats[uint2(0, 14)] = asuint(R_minus.x);
    g_stats[uint2(1, 14)] = asuint(R_minus.y);
    g_stats[uint2(2, 14)] = asuint(R_minus.z);
    g_stats[uint2(3, 14)] = asuint(end_scr.x);
    g_stats[uint2(4, 14)] = asuint(end_scr.y);
    g_stats[uint2(5, 14)] = asuint(d_VN);
    g_stats[uint2(6, 14)] = asuint(d_RM);
    g_stats[uint2(7, 14)] = asuint(d_RP);

    // ── Phase 3.Fix12 dirProbe (y15) — permanent regression sanity check ──
    // Old (endpoint-projection) construction vs new AMD unit-step:
    // old |Δuv| grows nonlinearly with L; unitStep is a constant.
    float3 o_scr_p = float3(uv, -view_origin.z);
    #define SSR_DIRPROBE_LEN(L) length((SSR_ProjectToScreen(view_origin + r_mirror * (L)) - o_scr_p).xy)
    g_stats[uint2(0, 15)] = asuint(SSR_NearPlaneDistance());
    g_stats[uint2(1, 15)] = asuint(SSR_DIRPROBE_LEN(1.0));
    g_stats[uint2(2, 15)] = asuint(SSR_DIRPROBE_LEN(10.0));
    g_stats[uint2(3, 15)] = asuint(SSR_DIRPROBE_LEN(100.0));
    g_stats[uint2(4, 15)] = asuint(SSR_DIRPROBE_LEN(300.0));
    g_stats[uint2(5, 15)] = asuint(length((SSR_ProjectToScreen(view_origin + r_mirror) - o_scr_p).xy));
    #undef SSR_DIRPROBE_LEN

    // ── Phase 3.Fix13 vndfProbe (y16) — numeric VNDF direction probe ──
    // Ray-0 sampled direction vs deterministic mirror target:
    //   dotLM=1/angDeg=0 → sample landed exactly on mirror dir (also the
    //   deterministic-mode baseline). Sentinels: dotLM=-2 / angDeg=-1 when
    //   no ray was stored (ineligible/degenerate pixel).
    float vp_alpha = (SSR_rough_interp > 0.5) ? rough_used : rough_used * rough_used;
    vp_alpha = max(vp_alpha, 1e-3);
    float vp_dlm = -2.0;
    float vp_ang = -1.0;
    if (dot(r_vndf_stored, r_vndf_stored) > 0.25) {
      vp_dlm = dot(normalize(r_vndf_stored), normalize(r_mirror));
      vp_ang = degrees(acos(clamp(vp_dlm, -1.0, 1.0)));
    }
    g_stats[uint2(0, 16)] = asuint(rough_used);
    g_stats[uint2(1, 16)] = asuint(vp_alpha);
    g_stats[uint2(2, 16)] = asuint(vp_dlm);
    g_stats[uint2(3, 16)] = asuint(vp_ang);
    g_stats[uint2(4, 16)] = asuint(ndotl_sampled);
    g_stats[uint2(5, 16)] = asuint(probe_flip ? 1.0 : 0.0);   // 1 = ray rejected (both samples below horizon)
    // ── Phase 3.Fix17 (Fix18 relocation): per-attempt VNDF rows ──
    // Captured at THIS center pixel inside the sampling loop; previously
    // mis-wired to the CompareProbe pixel gate which dumped its defaults.
    // Columns: [0]=dotLM [1]=angDeg [2]=ndotl [3]=belowHorizon; y20[4]=ran.
    g_stats[uint2(0, 20)] = asuint(probe_a0_dlm);
    g_stats[uint2(1, 20)] = asuint(probe_a0_ang);
    g_stats[uint2(2, 20)] = asuint(probe_a0_ndotl);
    g_stats[uint2(3, 20)] = asuint(probe_a0_below ? 1.0 : 0.0);
    g_stats[uint2(4, 20)] = asuint(probe_a1_ran ? 1.0 : 0.0);
    g_stats[uint2(0, 21)] = asuint(probe_a1_dlm);
    g_stats[uint2(1, 21)] = asuint(probe_a1_ang);
    g_stats[uint2(2, 21)] = asuint(probe_a1_ndotl);
    g_stats[uint2(3, 21)] = asuint(probe_a1_below ? 1.0 : 0.0);
    g_stats[uint2(6, 16)] = asuint(saturate(SSR_mirror_bias));
    g_stats[uint2(7, 16)] = asuint((float)ray_count_eff);
  }

  // ── Phase 2.5/2.6 normal-family probes (ray 0 only, diagnostics-gated) ──
  if (diagnostics_on && !sky_pixel && ray_valid_geom) {
    float3 dn_v; float d_cross;
    if (SSR_DepthNormalView(px, screen_size, dn_v, d_cross)) {
      // Decode sanity: pre-normalize length of the spherical formula output.
      float3 n_unit_mrt; float len_mrt;
      SSR_DecodeMrtNormalRaw(px, int2(screen_size), n_unit_mrt, len_mrt);
      float dl = abs(len_mrt - 1.0);
      uint lb = (dl < 0.01) ? 0u : (dl < 0.05) ? 1u : 2u;
      StatsAdd(uint2(lb, 7u), 1u);
      // Same-space comparison: both normals taken to view space.
      float3 nv_canon = mul(n_unit_mrt, (float3x3)view_g);
      float wdot = dot(nv_canon, dn_v);
      uint db = (wdot >= 0.99) ? 3u : (wdot >= 0.95) ? 4u
              : (wdot >= 0.90) ? 5u : (wdot >= 0.80) ? 6u : 7u;
      StatsAdd(uint2(db, 7u), 1u);
      float a_diff = acos(clamp(dot(nv_canon, dn_v), -1.0, 1.0));
      if (a_diff > 30.0 * (3.14159265 / 180.0)) StatsAdd(uint2(6u, 6u), 1u);
    } else {
      StatsAdd(uint2(7u, 6u), 1u);
    }
  }

  // ── Debug payloads (ray-0 state) ────────────────────────────────────────
  const bool validated_any = acc_hits > 0;
  if (debug_mode == 10) {
    out_color.rgb = float3(r0.raw_hit ? 0.9 : 0.05,
                           validated_any ? 0.9 : 0.05, 0.0);
  } else if (debug_mode == 11) {
    // Three-state Hit UV classification:
    //   black         = traversal produced no candidate
    //   red           = candidate found but rejected by validation
    //   green checker = accepted hit (passed all active validation)
    if (!r0.raw_hit) {
      out_color.rgb = float3(0.0, 0.0, 0.0);              // no candidate
    } else if (!validated_any) {
      out_color.rgb = float3(0.8, 0.05, 0.05);            // rejected
    } else {
      float2 cell = floor(saturate(r0.hit_pos.xy) * 16.0);
      float checker = fmod(cell.x + cell.y, 2.0);
      out_color.rgb = lerp(float3(0.1, 0.55, 0.1),
                           float3(0.35, 1.0, 0.35), checker);
    }
  } else if (debug_mode == 12) {
    float fr = saturate((float)r0.final_mip / (float)SSR_MAX_MIP);
    float co = saturate((float)r0.coarsest_mip / (float)SSR_MAX_MIP);
    out_color.rgb = r0.raw_hit ? float3(fr, co, 0.9) : 0.03.xxx;
  } else if (debug_mode == 13) {
    float tt = saturate((float)r0.iterations / max(SSR_max_traversal_steps, 1.0));
    out_color.rgb = (!sky_pixel && ray_valid_geom) ? SSR_Heat3(tt)
                                                   : SSR_Heat3(tt) * 0.15;
  } else if (debug_mode == 14) {
    if (r0.reject_reason == 0)      out_color.rgb = float3(1, 1, 1) * lerp(0.25, 1.0, r0.confidence);
    else if (r0.reject_reason == 1) out_color.rgb = float3(1.00, 0.15, 0.10);
    else if (r0.reject_reason == 2) out_color.rgb = float3(1.00, 1.00, 0.10);
    else if (r0.reject_reason == 3) out_color.rgb = float3(0.60, 0.60, 0.65);
    else if (r0.reject_reason == 4) out_color.rgb = float3(0.20, 0.40, 1.00);
    else                            out_color.rgb = float3(0.25, 0.02, 0.02);
  } else if (debug_mode == 15) {
    out_color.rgb = r_vndf_stored * 0.5 + 0.5;
  } else if (debug_mode == 16) {
    if (!sky_pixel && !ray_valid_geom)      out_color.rgb = float3(0.90, 0.10, 0.90);
    else if (r0.raw_hit)                    out_color.rgb = float3(0.10, 0.90, 0.20);
    else if (r0.exhausted)                  out_color.rgb = float3(1.00, 0.45, 0.05);
    else                                    out_color.rgb = float3(0.05, 0.05, 0.08);
  } else if (debug_mode == 32) {
    // Raw Radiance — preview of the BOUND radiance source at 4x gain
    // (values are scene-linear and dark; PS debug display further dims x0.3).
    out_color.rgb = min(g_radiance.SampleLevel(g_samplerLinearClamp, uv, 0).rgb * 4.0,
                        64.0.xxx);
    out_color.a = 1.0;
  } else if (debug_mode == 17) {
    float2 pa = uv * float2(screen_size)
              * float2(1.0, (float)screen_size.x / (float)screen_size.y);
    float2 pb = r0.hit_pos.xy * float2(screen_size)
              * float2(1.0, (float)screen_size.x / (float)screen_size.y);
    float2 pc = pa;
    float2 seg = pb - pa;
    float len2 = max(dot(seg, seg), 1e-6);
    float tt2 = saturate(dot(pc - pa, seg) / len2);
    float dist = length(pc - (pa + tt2 * seg));
    float3 base = 0.04.xxx;
    if (!sky_pixel && ray_valid_geom) base += SSR_Heat3(saturate((float)r0.iterations /
        max(SSR_max_traversal_steps, 1.0))) * 0.18;
    float line_w = (dist < 0.85) ? 1.0 : smoothstep(1.6, 0.6, dist) * 0.35;
    float3 col = base;
    if (!sky_pixel && ray_valid_geom) {
      col = lerp(col, r0.raw_hit ? float3(1, 1, 0.85) : float3(1, 0.4, 0.25), line_w);
      if (length(pc - pa) < 2.2) col = float3(0.2, 1.0, 0.3);
      if (length(pc - pb) < 2.2) col = r0.raw_hit ? float3(0.2, 1.0, 0.3)
                                                  : float3(1.0, 0.15, 0.15);
    }
    out_color.rgb = col;
  } else if (debug_mode == 19) {
    if (r0.raw_hit) {
      float dpx = length((r0.hit_pos.xy - uv) * float2(screen_size));
      if      (dpx < 1.0)  out_color.rgb = float3(0.15, 0.35, 1.00);
      else if (dpx < 2.0)  out_color.rgb = float3(0.20, 0.90, 1.00);
      else if (dpx < 8.0)  out_color.rgb = float3(0.20, 1.00, 0.35);
      else if (dpx < 32.0) out_color.rgb = float3(1.00, 0.95, 0.20);
      else                 out_color.rgb = float3(1.00, 0.25, 0.20);
    } else out_color.rgb = 0.0.xxx;
  } else if (debug_mode == 22) {
    float3 n_w = SSR_DecodeWorldNormal(px, int2(screen_size));
    float3 nv_c = mul(n_w, (float3x3)view_g);
    float3 dn_v; float dcm;
    if (SSR_DepthNormalView(px, screen_size, dn_v, dcm)) {
      float a = acos(clamp(dot(normalize(nv_c), dn_v), -1.0, 1.0));
      out_color.rgb = SSR_Heat3(saturate(a / (45.0 * (3.14159265 / 180.0))));
    } else out_color.rgb = float3(0.35, 0.35, 0.4);
  } else if (debug_mode == 23) {
    float3 dn_v; float dcm;
    out_color.rgb = SSR_DepthNormalView(px, screen_size, dn_v, dcm)
        ? mul(dn_v, (float3x3)viewInv_g) * 0.5 + 0.5
        : float3(0.35, 0.35, 0.4);
  } else if (debug_mode == 24) {
    out_color.rgb = SSR_DecodeWorldNormal(px, int2(screen_size)) * 0.5 + 0.5;
  } else if (debug_mode == 25) {
    out_color.rgb = !sky_pixel ? V * 0.5 + 0.5 : float3(0.2, 0.2, 0.25);
  } else if (debug_mode == 26) {
    out_color.rgb = !sky_pixel ? n_view * 0.5 + 0.5 : float3(0.2, 0.2, 0.25);
  } else if (debug_mode == 27) {
    if (!sky_pixel && ray_valid_geom) {
      float3 dn_v; float dcm;
      if (SSR_DepthNormalView(px, screen_size, dn_v, dcm)) {
        float3 rr = 2.0 * dot(dn_v, V) * dn_v - V;
        float a = acos(clamp(dot(normalize(rr), normalize(r_vndf_stored)), -1.0, 1.0));
        out_color.rgb = SSR_Heat3(saturate(a / (60.0 * (3.14159265 / 180.0))));
      } else out_color.rgb = float3(0.35, 0.35, 0.4);
    } else out_color.rgb = 0.05.xxx;
  } else if (debug_mode == 28) {
    // Accepted Hit UV — three states (four-class validation instrument).
    if (!r0.raw_hit) {
      out_color.rgb = (!sky_pixel && ray_valid_geom) ? float3(0.02, 0.02, 0.03)
                                                     : 0.0.xxx;
    } else if (!validated_any) {
      out_color.rgb = float3(0.45, 0.05, 0.45);       // rejected candidate
    } else {
      float2 cell = floor(saturate(last_accepted_pos.xy) * 16.0);
      float checker = fmod(cell.x + cell.y, 2.0);
      out_color.rgb = lerp(float3(0.25, 0.25, 0.25), float3(1, 1, 1), checker);
    }
  } else if (debug_mode == 29) {
    // Validation Class — merged termination + rejection classification.
    if (sky_pixel)                              out_color.rgb = float3(0.03, 0.03, 0.04);
    else if (!ray_valid_geom)                   out_color.rgb = float3(0.30, 0.10, 0.40);
    else if (!r0.raw_hit)                       out_color.rgb = float3(1.00, 0.50, 0.05);
    else if (validated_any)                     out_color.rgb = float3(0.15, 0.95, 0.25);
    else if (r0.reject_reason == 5)             out_color.rgb = float3(1.00, 0.20, 0.15);
    else if (r0.reject_reason == 4)             out_color.rgb = float3(0.25, 0.45, 1.00);
    else if (r0.reject_reason == 2)             out_color.rgb = float3(1.00, 0.95, 0.15);
    else if (r0.reject_reason == 1)             out_color.rgb = float3(0.20, 0.90, 0.90);
    else if (r0.reject_reason == 3)             out_color.rgb = float3(0.45, 0.55, 0.70);
    else                                        out_color.rgb = float3(0.35, 0.08, 0.08);
  } else if (debug_mode == 30) {
    // VNDF vs Mirror: angular difference between stochastic and mirror dirs.
    if (production_stoch && !sky_pixel && ray_valid_geom) {
      float a = acos(clamp(dot(normalize(r_vndf_stored),
                               normalize(r_mirror)), -1.0, 1.0));
      out_color.rgb = SSR_Heat3(saturate(a / (60.0 * (3.14159265 / 180.0))));
    } else out_color.rgb = 0.03.xxx;
  } else if (debug_mode == 38) {
    // Projection Position: reconstruct view-space position from NDC+depth,
    // project back through engine proj_g. Tests Project∘Reconstruct ≡ identity.
    // r = reconstructed U, g = reconstructed V, b = positive view-Z / 100.
    if (!sky_pixel) {
      float z_pos = SSR_LinearizeDepth(hw_depth);
      float3 vp = SSR_ScreenToViewPos(float3(uv, z_pos));
      float3 reproj = SSR_ProjectToScreen(vp);
      out_color.rgb = float3(reproj.x, reproj.y,
                             saturate(-vp.z / 100.0));
    } else {
      out_color.rgb = float3(0.1, 0.1, 0.15);
    }
  } else if (debug_mode == 39) {
    // Mirror Hit Class — clean three-state classification.
    //   black = no candidate · red = rejected · green = accepted
    if (!sky_pixel && ray_valid_geom && r0.raw_hit) {
      if (validated_any)
        out_color.rgb = float3(0.1, 0.9, 0.1);       // green: accepted
      else
        out_color.rgb = float3(0.9, 0.05, 0.05);     // red: rejected
    } else {
      out_color.rgb = float3(0.0, 0.0, 0.0);          // black: no candidate
    }
  } else if (debug_mode == 40) {
    // Same Surface classification — identifies grazing self-intersections:
    //   white = same-surface candidate (normalSim ≥ 0.95)
    //   blue  = different-surface candidate (useful reflection content)
    //   black = no candidate
    //   gray  = missing normals for classification
    if (!sky_pixel && ray_valid_geom && r0.raw_hit) {
      int2 hit_px2 = clamp((int2)(saturate(r0.hit_pos.xy) * float2(screen_size)),
                           int2(0, 0), int2(screen_size) - int2(1, 1));
      float3 n_hit_w = SSR_DecodeWorldNormal(hit_px2, int2(screen_size));
      float3 n_hit_v = normalize(mul(n_hit_w, (float3x3)view_g));
      float n_sim = dot(n_view, n_hit_v);
      if (n_sim >= 0.95)
        out_color.rgb = float3(1, 1, 1);               // white: same surface
      else
        out_color.rgb = float3(0.15, 0.35, 0.9);       // blue: different surface
    } else {
      out_color.rgb = float3(0.05, 0.05, 0.08);        // dark: no candidate/sky
    }
  } else if (debug_mode == 41) {
    // Compare Probe Pixel marker — locates the active probe pixel
    // (auto-selected frozen coords or manual X/Y sliders).
    const float2 ppx = float2(SSR_probe_pixel_x, SSR_probe_pixel_y);
    const float dpx = length((float2(dispatchThreadID.xy) + 0.5) - ppx);
    if (dpx < 3.0)       out_color.rgb = float3(1.0, 1.0, 1.0);   // white core
    else if (dpx < 6.0)  out_color.rgb = float3(0.6, 0.6, 0.2);   // yellow ring
    else                 out_color.rgb = float3(0.02, 0.02, 0.03);
    out_color.a = (dpx < 6.0) ? 1.0 : 0.0;
  } else if (debug_mode == 37) {
    // Raw Hit Radiance — direct radiance-source sample at accepted hit UV.
    if (r0.raw_hit && validated_any) {
      out_color.rgb = g_radiance.SampleLevel(g_samplerLinearClamp,
                                             r0.hit_pos.xy, 0).rgb;
    } else {
      out_color.rgb = float3(0.0, 0.0, 0.0);
    }
  } else {
    // Default: heuristic VNDF-weighted radiance accumulation (production).
    out_color.rgb = acc_rad / max(acc_w, 1e-4);
    if (acc_hits == 0) out_color.rgb = 0.0.xxx;
  }

  out_color.a = validated_any ? saturate(acc_conf / (float)acc_hits) : 0.0;

  // ── Phase 3.Fix15: Mirror-vs-VNDF CompareProbe (read-only, one pixel) ──
  // Traces BOTH direction sets at the configured pixel through the exact
  // production constructions (Fix12 unit-step mirror; Heitz VNDF + bias),
  // with stats=false so traversal counters are untouched. Local state only;
  // the production loop above is not modified. When Stochastic is ON, the
  // V-side uses the same ray-0 noise as production, so Rv must reproduce r0.
  if (diagnostics_on &&
      px.x == (int)clamp(SSR_probe_pixel_x, 0.0, screen_size.x - 1.0) &&
      px.y == (int)clamp(SSR_probe_pixel_y, 0.0, screen_size.y - 1.0)) {
    // ---- Mirror side ----
    SSR_RayOut Rm;
    float dm_unused;
    float3 d_m = SSR_ProjectToScreen(view_origin + r_mirror)
               - float3(uv, -view_origin.z);
    if (any(isnan(d_m)) || any(abs(d_m) > 4.0)) d_m = 0.0.xxx;
    SSR_TraceRay(origin_pz, d_m, screen_size, sky_pixel, ray_valid_geom,
                 r_mirror, uv, false, Rm, dm_unused);
    float3 rad_m = 0.0.xxx;
    if (Rm.accepted)
      rad_m = g_radiance.SampleLevel(g_samplerLinearClamp, Rm.hit_pos.xy, 0).rgb;

    // ---- VNDF side (verbatim production sampling copy, ray-0 noise) ----
    // Phase 3.Fix17: same regenerate-once/reject-twice policy; no funnel
    // counter writes here (probe must not pollute production statistics).
    float alpha_p = (SSR_rough_interp > 0.5) ? rough_used : rough_used * rough_used;
    alpha_p = max(alpha_p, 1e-3);
    const float3 T1p = nv < 0.999f ? normalize(cross(n_view, float3(0, 0, 1)))
                                   : float3(1, 0, 0);
    const float3 T2p = cross(n_view, T1p);
    const float3 Vep = float3(dot(V, T1p), dot(V, T2p), dot(V, n_view));
    float3 Lp = 0.0.xxx;
    bool valid_p = false;
    [loop]
    for (uint attp = 0u; attp < 2u; ++attp) {
      const uint2 noffp = (attp == 0u) ? uint2(0u, 0u) : uint2(137u, 149u);
      const float2 Up = SSR_Noise2(dispatchThreadID.xy + noffp,
                                   (uint)SSR_frame_index);
      const float3 Hlp = SSR_SampleGGXVNDF(Vep, alpha_p, alpha_p, Up.x, Up.y);
      const float3 Hp = T1p * Hlp.x + T2p * Hlp.y + n_view * Hlp.z;
      const float3 Lcp = 2.0 * dot(Hp, V) * Hp - V;
      if (dot(Lcp, n_view) > 0.0) { Lp = Lcp; valid_p = true; break; }
    }
    SSR_RayOut Rv;
    float dv_unused;
    float cmp_ang = -1.0;   // sentinel: no valid sample
    if (!valid_p) {
      Rv = (SSR_RayOut)0;
      Rv.reject_reason = 6;
      dv_unused = 0.0;
    } else {
      if (SSR_mirror_bias > 0.001)
        Lp = normalize(lerp(Lp, r_mirror, saturate(SSR_mirror_bias)));
      Lp = normalize(Lp);
      SSR_TraceRay(origin_pz, float3(Lp.xy, -Lp.z), screen_size,
                   sky_pixel, ray_valid_geom, Lp, uv, false, Rv, dv_unused);
      cmp_ang = degrees(acos(clamp(dot(Lp, r_mirror), -1.0, 1.0)));
    }
    float3 rad_v = 0.0.xxx;
    if (Rv.accepted)
      rad_v = g_radiance.SampleLevel(g_samplerLinearClamp, Rv.hit_pos.xy, 0).rgb;

    g_stats[uint2(0, 17)] = asuint(Rm.hit_pos.x);
    g_stats[uint2(1, 17)] = asuint(Rm.hit_pos.y);
    g_stats[uint2(2, 17)] = asuint(rad_m.r);
    g_stats[uint2(3, 17)] = asuint(rad_m.g);
    g_stats[uint2(4, 17)] = asuint(rad_m.b);
    g_stats[uint2(5, 17)] = asuint(Rm.accepted ? saturate(Rm.confidence) : 0.0);
    g_stats[uint2(6, 17)] = asuint(Rm.accepted ? 1.0 : 0.0);
    g_stats[uint2(7, 17)] = asuint(SSR_probe_pixel_x);

    g_stats[uint2(0, 18)] = asuint(Rv.hit_pos.x);
    g_stats[uint2(1, 18)] = asuint(Rv.hit_pos.y);
    g_stats[uint2(2, 18)] = asuint(rad_v.r);
    g_stats[uint2(3, 18)] = asuint(rad_v.g);
    g_stats[uint2(4, 18)] = asuint(rad_v.b);
    g_stats[uint2(5, 18)] = asuint(Rv.accepted ? saturate(Rv.confidence) : 0.0);
    g_stats[uint2(6, 18)] = asuint(cmp_ang);
    g_stats[uint2(7, 18)] = asuint(Rv.accepted ? 1.0 : 0.0);

    // ── Phase R1 Probe B: hit classification row (y27) ──
    // [0]=V_sameSurface [1]=V_rejectReason [2]=M_sameSurface
    // [3]=V_ndotl_final [4]=M_rejectReason
    float v_ss = -1.0, m_ss = -1.0;
    if (Rv.accepted) v_ss = SSR_ProbeSameSurface(Rv.hit_pos.xy, view_origin, n_view, screen_size);
    if (Rm.accepted) m_ss = SSR_ProbeSameSurface(Rm.hit_pos.xy, view_origin, n_view, screen_size);
    g_stats[uint2(0, 27)] = asuint(v_ss);
    g_stats[uint2(1, 27)] = asuint((float)Rv.reject_reason);
    g_stats[uint2(2, 27)] = asuint(m_ss);
    g_stats[uint2(3, 27)] = asuint(valid_p ? dot(Lp, n_view) : -1.0);
    g_stats[uint2(4, 27)] = asuint((float)Rm.reject_reason);

    // ── Phase 3.Fix18: radiance footprint probe around the V hit UV ──
    // Read-only texel statistics of the radiance source at the stochastic
    // hit: center RGB, 3x3 average, 5x5 average, 5x5 maximum (per-channel,
    // max selected by luminance). Diagnoses whether a dull V radiance is
    // local content (small patch inside a bright neighborhood) or a
    // uniformly dark region. Point Loads — no sampler involvement.
    float3 fp_c = 0.0.xxx, fp_a3 = 0.0.xxx, fp_a5 = 0.0.xxx, fp_mx = 0.0.xxx;
    float fp_ml = -1.0;
    if (Rv.accepted) {
      const int2 vpx = (int2)(Rv.hit_pos.xy * float2(screen_size));
      float3 s3 = 0.0.xxx, s5 = 0.0.xxx;
      [unroll]
      for (int dy = -2; dy <= 2; ++dy) {
        [unroll]
        for (int dx = -2; dx <= 2; ++dx) {
          const int2 tp = clamp(vpx + int2(dx, dy), int2(0, 0),
                                int2(screen_size) - int2(1, 1));
          const float3 c = g_radiance.Load(int3(tp, 0)).rgb;
          s5 += c;
          fp_c = (dx == 0 && dy == 0) ? c : fp_c;
          const bool in3 = (abs(dx) <= 1) && (abs(dy) <= 1);
          s3 += in3 ? c : 0.0.xxx;
          const float lum = dot(c, float3(0.299, 0.587, 0.114));
          if (lum > fp_ml) { fp_ml = lum; fp_mx = c; }
        }
      }
      fp_a3 = s3 / 9.0;
      fp_a5 = s5 / 25.0;
    }

    // Row y22: center RGB + 3x3 average (+ valid flag).
    g_stats[uint2(0, 22)] = asuint(fp_c.r);
    g_stats[uint2(1, 22)] = asuint(fp_c.g);
    g_stats[uint2(2, 22)] = asuint(fp_c.b);
    g_stats[uint2(3, 22)] = asuint(fp_a3.r);
    g_stats[uint2(4, 22)] = asuint(fp_a3.g);
    g_stats[uint2(5, 22)] = asuint(fp_a3.b);
    g_stats[uint2(6, 22)] = asuint(Rv.accepted ? 1.0 : 0.0);
    // Row y23: 5x5 average RGB + 5x5 max RGB (by luminance) + max luma.
    g_stats[uint2(0, 23)] = asuint(fp_a5.r);
    g_stats[uint2(1, 23)] = asuint(fp_a5.g);
    g_stats[uint2(2, 23)] = asuint(fp_a5.b);
    g_stats[uint2(3, 23)] = asuint(fp_mx.r);
    g_stats[uint2(4, 23)] = asuint(fp_mx.g);
    g_stats[uint2(5, 23)] = asuint(fp_mx.b);
    g_stats[uint2(6, 23)] = asuint(fp_ml);

    g_stats[uint2(0, 19)] = asuint(r0.confidence);   // ray-0 raw confidence
    g_stats[uint2(1, 19)] = asuint(out_color.a);     // final t25 alpha
    g_stats[uint2(2, 19)] = asuint(-1.0);            // resolve alpha: pass undispatched (audit)
    g_stats[uint2(3, 19)] = asuint(SSR_probe_pixel_y);
    g_stats[uint2(7, 19)] = asuint(1.0);             // wrote-this-frame flag
  }
  g_outRays[dispatchThreadID.xy] = out_color;
}
