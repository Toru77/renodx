// Falcom Engine+ extension (owner-approved, not part of CORTAO): cone-cone specular occlusion from the CORTAO AO and
// bent normal (GTAO, Jimenez et al. 2016; spherical-cap intersection as UE4 SphericalCapIntersectionAreaFast).
// Included by the Sora 1st, Sora 2nd and Kai lighting shaders after shadows_common.hlsli (FalcomSafeNormalize3).
#ifndef SRC_GAMES_FALCOMENGINE_PLUS_CUSTOMSHADERS_CORTAO_CORTAO_SPECULAR_OCCLUSION_HLSLI_
#define SRC_GAMES_FALCOMENGINE_PLUS_CUSTOMSHADERS_CORTAO_CORTAO_SPECULAR_OCCLUSION_HLSLI_
// visibility: CORTAO AO value (1 = unoccluded). coneAxis, refl: unit world-space vectors. roughness: [0, 1]. Returns [0, 1].
float FalcomCortaoSpecularOcclusion(float visibility, float3 coneAxis, float3 refl, float roughness) {
  const float r0 = acos(sqrt(saturate(1.0 - visibility)));
  const float cosSpec = exp2(-3.32193 * roughness * roughness);
  const float r1 = acos(saturate(cosSpec));
  const float d = acos(clamp(dot(coneAxis, refl), -1.0, 1.0));
  const float capMin = 6.2831853 - 6.2831853 * cos(min(r0, r1));
  float area;
  if (d <= max(r0, r1) - min(r0, r1)) area = capMin;
  else if (d >= r0 + r1) area = 0.0;
  else area = smoothstep(0.0, 1.0, 1.0 - saturate((d - abs(r0 - r1)) / (r0 + r1 - abs(r0 - r1)))) * capMin;
  return saturate(area / max(6.2831853 * (1.0 - cosSpec), 1e-4));
}
float3 FalcomCortaoBentWorld(Texture2D<float4> bentTex, float2 uv, float3x3 viewToWorld, float3 fallback) {
  uint w, h;
  bentTex.GetDimensions(w, h);
  const uint2 texel = min(uint2(saturate(uv) * float2(w, h)), uint2(w, h) - 1u);
  return FalcomSafeNormalize3(mul(bentTex.Load(int3(texel, 0)).yzw, viewToWorld), fallback);
}
#endif
