///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Per-blade AO gradient for foliage (Ghost of Tsushima §1.5e-ii).
// Darkens base, brightens tip — replaces noisy SSAO/GTAO with a stable vertical gradient.
//
// Usage:
//   o0.rgb = ApplyFoliageAO(o0.rgb, bladeHeight);
//
//   bladeHeight: 0 = root, 1 = tip.
//     For billboard foliage this is typically 1.0 - v3.y (UV V inverted by the shader).
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __cplusplus

// Returns diffuseColor * AO factor (scalar only — no tint, uses foliage's own color).
static float3 ApplyFoliageAO(float3 diffuseColor, float bladeHeight)
{
  if (shader_injection_data.foliage_grass_ao_enabled < 0.5f)
    return diffuseColor;

  // Power curve: concentrates darkening near the base.
  float t = pow(saturate(bladeHeight),
                shader_injection_data.foliage_grass_ao_curve);

  // Scalar AO intensity along the blade.
  float ao = lerp(shader_injection_data.foliage_grass_ao_base,
                  shader_injection_data.foliage_grass_ao_tip,
                  t);

  return diffuseColor * ao;
}

#endif
