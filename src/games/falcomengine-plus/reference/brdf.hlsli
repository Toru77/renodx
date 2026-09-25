// ──────────────────────────────────────────────────────────────────────────────
// brdf.hlsli  —  Energy-conserving BRDF helpers (fxc-compatible HLSL SM5.0)
//
// Ported from rendering.hlsl (Slang) to plain HLSL.
// All `renodx::` namespaces stripped, `DivideSafe` local.
// ──────────────────────────────────────────────────────────────────────────────

#ifndef REFERENCE_BRDF_HLSLI_
#define REFERENCE_BRDF_HLSLI_

// ──────────────────────────────────────────────────────────────────────────────
// GGX Normal Distribution Function  (Trowbridge-Reitz)
// ──────────────────────────────────────────────────────────────────────────────
float GGX_NDF(float NdotH, float alpha)
{
  float a2    = alpha * alpha;
  float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
  denom = max(denom, 1e-4f);  // Guard against zero when NdotH→1 and a2→0
  return a2 / (3.14159265f * denom * denom);
}

// ──────────────────────────────────────────────────────────────────────────────
// Smith Height-Correlated Visibility  (V = G₂ / (4·NdotL·NdotV))
// ──────────────────────────────────────────────────────────────────────────────
float SmithGGX_Visibility(float NdotV, float NdotL, float alpha)
{
  float a2  = alpha * alpha;
  float ggxV = NdotL * sqrt(max(NdotV * NdotV * (1.0f - a2) + a2, 1e-8f));
  float ggxL = NdotV * sqrt(max(NdotL * NdotL * (1.0f - a2) + a2, 1e-8f));
  return 0.5f / max(ggxV + ggxL, 1e-6f);
}

// ──────────────────────────────────────────────────────────────────────────────
// Schlick Fresnel  (scalar + float3)
// ──────────────────────────────────────────────────────────────────────────────
float3 SchlickFresnel(float3 F0, float VdotH)
{
  float oneMinusVdotH = 1.0f - VdotH;
  float pow5 = oneMinusVdotH * oneMinusVdotH;
  pow5 *= pow5 * oneMinusVdotH;  // (1 - VdotH)^5
  return F0 + (1.0f - F0) * pow5;
}

float SchlickFresnel(float F0, float VdotH)
{
  float oneMinusVdotH = 1.0f - VdotH;
  float pow5 = oneMinusVdotH * oneMinusVdotH;
  pow5 *= pow5 * oneMinusVdotH;  // (1 - VdotH)^5
  return F0 + (1.0f - F0) * pow5;
}

// ──────────────────────────────────────────────────────────────────────────────
// Single GGX Specular Lobe  (D · V · F)
//
// Returns pre-NdotL specular.  Multiply by NdotL × lightColor for outgoing.
//   roughness – perceptual [0,1] (squared internally to alpha)
//   F0        – reflectance at normal incidence
// ──────────────────────────────────────────────────────────────────────────────
float3 GGX_Specular(
    float NdotH, float NdotV, float NdotL, float VdotH,
    float roughness, float3 F0)
{
  float alpha = roughness * roughness;
  // 5e-3 ≈ roughness 0.07 — caps D_max at ~64, wide enough to cover a pixel.
  // Below this, the NDF is narrower than a pixel and causes specular aliasing.
  alpha = max(alpha, 5e-3f);

  float  D = GGX_NDF(NdotH, alpha);
  float  V = SmithGGX_Visibility(NdotV, NdotL, alpha);
  float3 F = SchlickFresnel(F0, VdotH);

  return D * V * F;
}

// ──────────────────────────────────────────────────────────────────────────────
// Directional Albedo E(μ, α)  — Turquin 2019 rational polynomial fit
// ──────────────────────────────────────────────────────────────────────────────
float GGX_DirectionalAlbedo(float NdotV, float roughness)
{
  float mu  = NdotV;
  float a   = roughness;
  float a2  = a * a;
  float mu2 = mu * mu;

  float num = 1.0f
            + mu  * (-1.0816f + a * 0.0378f)
            + mu2 * ( 0.1696f + a * 0.0856f)
            + a   * (-0.6992f + a * (1.4424f + a * (-1.3616f + a * 0.4504f)));

  return saturate(num);
}

// ──────────────────────────────────────────────────────────────────────────────
// Average Albedo E_avg(α)  — hemisphere average for multi-scatter compensation
// ──────────────────────────────────────────────────────────────────────────────
float GGX_AverageAlbedo(float roughness)
{
  float a = roughness;
  return saturate(1.0f + a * (-0.7127f + a * (0.4364f + a * (-0.1188f))));
}

// ──────────────────────────────────────────────────────────────────────────────
// Multi-Scatter Compensation (Kulla-Conty 2017)
//
// Returns a multiplier (≥ 1.0) to apply to single-scatter specular,
// recovering inter-microfacet bounce energy.
// Clamped to 4.0× to prevent fireflies.
// ──────────────────────────────────────────────────────────────────────────────
float3 MultiScatterCompensation(
    float NdotV, float NdotL, float roughness, float3 F0)
{
  float Eo   = GGX_DirectionalAlbedo(NdotV, roughness);
  float Ei   = GGX_DirectionalAlbedo(NdotL, roughness);
  float Eavg = GGX_AverageAlbedo(roughness);

  // Average Fresnel: cosine-weighted hemisphere integral of Schlick
  float3 Favg = F0 + (1.0f / 21.0f) * (1.0f - F0);

  // Energy denominator
  float3 f_ms_denom = max(1.0f - Favg * (1.0f - Eavg), 1e-5f);

  // Directional energy loss from single-scatter
  float dirLoss = (1.0f - Eo) * (1.0f - Ei);
  float dirBase = max(Eo * Ei, 1e-5f);

  return min(1.0f + Favg * dirLoss / (dirBase * f_ms_denom), 4.0f);
}

float MultiScatterCompensation(float NdotV, float NdotL, float roughness, float F0)
{
  return MultiScatterCompensation(NdotV, NdotL, roughness, float3(F0, F0, F0)).x;
}

#endif // REFERENCE_BRDF_HLSLI_
