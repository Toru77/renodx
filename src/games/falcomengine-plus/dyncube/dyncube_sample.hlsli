#ifndef __DYNCUBE_SAMPLE_HLSLI__
#define __DYNCUBE_SAMPLE_HLSLI__

void DynCubeSampleDynamic(
    TextureCube<float4> envTex, SamplerState cubeSampler,
    float3 R, float roughFactor,
    out float3 dynCol, out float3 finalDir,
    out uint outNumLevels, out float outSampleMip)
{
  uint sampW, sampH;
  envTex.GetDimensions(0, sampW, sampH, outNumLevels);
  float3 sampleDir = float3(1, -1, -1) * R;
  float flip = (shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_lookup_direction_flip > 0.5f) ? -1.0 : 1.0;
  sampleDir *= flip;
  if (shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && abs(shader_injection_data.dynCube_vertical_offset) > 1e-4) {
    sampleDir.y -= tan(radians(shader_injection_data.dynCube_vertical_offset));
    sampleDir = normalize(sampleDir);
  }
  float sampleMip = (float)(outNumLevels - 1) * roughFactor;
  if (shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_mip > -0.5f) {
    sampleMip = clamp(shader_injection_data.dynCube_force_mip, 0.0, (float)(outNumLevels - 1));
  }
  if (shader_injection_data.dynCube_enabled > 0.5f) {
    sampleMip += (shader_injection_data.dynCube_force_vanilla > 0.5f
        ? shader_injection_data.dynCube_vanilla_blur
        : shader_injection_data.dynCube_blur);
  }
  outSampleMip = sampleMip;
  finalDir = sampleDir;
  dynCol = envTex.SampleLevel(cubeSampler, finalDir, sampleMip).xyz;
  if (shader_injection_data.dynCube_enabled > 0.5f
      && shader_injection_data.dynCube_force_vanilla < 0.5f
      && shader_injection_data.dynCube_debug != 4.f) {
    float hdrLum = dot(max(dynCol, 0.0), float3(0.2126, 0.7152, 0.0722));
    if (isfinite(hdrLum) && hdrLum > 1.0) {
      const float hdrKnee = 1.0;
      const float hdrRange = 1.0;
      float hdrExcess = hdrLum - hdrKnee;
      float compressedLum = hdrKnee + hdrExcess / (1.0 + hdrExcess / hdrRange);
      dynCol *= compressedLum / hdrLum;
    }
    dynCol *= clamp(shader_injection_data.dynCube_capture_boost, 0.0, 8.0);
  }
}

#endif // __DYNCUBE_SAMPLE_HLSLI__
