#ifndef VA_SATURATE
#define VA_SATURATE(x) saturate(x)
#endif

#ifndef XE_GTAO_USE_DEFAULT_CONSTANTS
#define XE_GTAO_USE_DEFAULT_CONSTANTS 0
#endif

#ifndef XE_GTAO_USE_HALF_FLOAT_PRECISION
#define XE_GTAO_USE_HALF_FLOAT_PRECISION 0
#endif

#include "./GTAO/XeGTAO.h"

cbuffer cb_scene : register(b0)
{
  float4x4 view_g;
  float4x4 viewInv_g;
  float4x4 proj_g;
};

cbuffer cb_xegtao : register(b13)
{
  float xegtao_quality;
  float xegtao_denoise_passes;
  float xegtao_radius;
  float xegtao_falloff_range;
  float xegtao_radius_multiplier;
  float xegtao_final_value_power;
  float xegtao_sample_distribution_power;
  float xegtao_thin_occluder_compensation;
  float xegtao_depth_mip_sampling_offset;
  float xegtao_denoise_blur_beta;
  float xegtao_noise_index;
  float xegtao_debug_mode;
  float xegtao_denoise_is_last_pass;
  float xegtao_normal_input_mode;
  float xegtao_isfast_jitter_amount;
  float xegtao_normal_influence;
  float xegtao_normal_depth_blend;
  float xegtao_normal_sharpness;
  float xegtao_normal_edge_rejection;
  float xegtao_normal_z_preservation;
  float xegtao_normal_detail_response;
  float xegtao_normal_max_darkening;
  float xegtao_normal_darkening_mode;
  float xegtao_denoiser_mode;
  float xegtao_isfast_jitter_mode;
  float xegtao_isfast_passes;
  float xegtao_isfast_samples;
  float xegtao_isfast_radius;
  float xegtao_isfast_edge_sensitivity;
  float xegtao_isfast_spatial_sigma;
  float xegtao_isfast_hybrid_blend;
  float xegtao_isfast_noise_available;
};

#ifndef XE_GTAO_COMPUTE_BENT_NORMALS
#define XE_GTAO_COMPUTE_BENT_NORMALS 1
#endif

#include "./GTAO/XeGTAO.hlsli"

Texture2D<uint> g_srcWorkingAOTerm : register(t0);
Texture2D<lpfloat> g_srcWorkingEdges : register(t1);
Texture3D<float2> g_srcIsFastNoise : register(t2);
SamplerState g_samplerPointClamp : register(s0);

RWTexture2D<uint> g_outFinalAOTerm : register(u0);

float InterleavedGradientNoise(float2 pixelCoord)
{
  return frac(52.9829189 * frac(0.06711056 * pixelCoord.x + 0.00583715 * pixelCoord.y));
}

float2 InterleavedGradientNoiseTemporal2D(float2 pixelCoord, uint frameIndex)
{
  static const float kR2Alpha1 = 0.7548776662466927;
  static const float kR2Alpha2 = 0.5698402909980532;

  const float base1 = InterleavedGradientNoise(pixelCoord);
  const float base2 = InterleavedGradientNoise(pixelCoord + float2(47.0, 17.0));
  return float2(
      frac(base1 + kR2Alpha1 * (float)frameIndex),
      frac(base2 + kR2Alpha2 * (float)frameIndex));
}

float2 SampleISFASTRGLoad(uint2 pixelCoord, uint frameIndex)
{
  uint width;
  uint height;
  uint depth;
  g_srcIsFastNoise.GetDimensions(width, height, depth);
  if (width == 0u || height == 0u || depth == 0u)
  {
    return InterleavedGradientNoiseTemporal2D((float2)pixelCoord, frameIndex);
  }

  const uint3 wrappedCoord = uint3(
      pixelCoord.x % width,
      pixelCoord.y % height,
      frameIndex % depth);
  return saturate(g_srcIsFastNoise.Load(int4(wrappedCoord, 0)));
}

float2 LoadISFASTNoiseForFrame(uint2 sampleCoord, uint sampleFrame)
{
  float2 noise = InterleavedGradientNoiseTemporal2D((float2)sampleCoord, sampleFrame);
  if (xegtao_isfast_noise_available >= 0.5)
  {
    noise = SampleISFASTRGLoad(sampleCoord, sampleFrame);
  }
  return noise;
}

float2 GetISFASTNoise2D(uint2 pixelCoord, uint sampleIndex, uint frameIndex, float temporalBlend)
{
  const uint2 sampleCoord = pixelCoord + uint2(sampleIndex * 37u, sampleIndex * 17u);
  const uint sampleFrameStatic = sampleIndex * 13u;
  const uint sampleFrameAnimated = frameIndex + sampleIndex * 13u;
  const float2 staticNoise = LoadISFASTNoiseForFrame(sampleCoord, sampleFrameStatic);
  const float2 animatedNoise = LoadISFASTNoiseForFrame(sampleCoord, sampleFrameAnimated);
  return lerp(staticNoise, animatedNoise, saturate(temporalBlend));
}

lpfloat4 LoadDecodedAOTerm(int2 coord, int2 size)
{
  const int2 clamped = clamp(coord, int2(0, 0), size - int2(1, 1));
  const uint packed = g_srcWorkingAOTerm.Load(int3(clamped, 0));
  lpfloat visibility;
  lpfloat3 bentNormal;
  XeGTAO_DecodeVisibilityBentNormal(packed, visibility, bentNormal);
  return lpfloat4(bentNormal, visibility);
}

lpfloat LoadEdgeValue(int2 coord, int2 size)
{
  const int2 clamped = clamp(coord, int2(0, 0), size - int2(1, 1));
  return g_srcWorkingEdges.Load(int3(clamped, 0));
}

float3 SafeNormalize3(float3 value, float3 fallbackValue)
{
  const float lenSq = dot(value, value);
  if (lenSq < 1e-6)
  {
    return fallbackValue;
  }
  return value * rsqrt(lenSq);
}

[numthreads(8, 8, 1)]
void main(uint2 dispatchThreadId : SV_DispatchThreadID)
{
  uint width;
  uint height;
  g_srcWorkingAOTerm.GetDimensions(width, height);
  if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
  {
    return;
  }

  const int2 size = int2(max(width, 1u), max(height, 1u));
  const int2 centerCoord = int2(dispatchThreadId);
  const lpfloat4 centerTerm = LoadDecodedAOTerm(centerCoord, size);
  const lpfloat centerEdge = LoadEdgeValue(centerCoord, size);

  const uint sampleCount = max(1u, (uint)round(clamp(xegtao_isfast_samples, 1.0, 32.0)));
  const float filterRadius = max(0.01, xegtao_isfast_radius);
  const float sigma = max(0.01, xegtao_isfast_spatial_sigma);
  const float sigma2 = 2.0 * sigma * sigma;
  const float edgeSensitivity = max(0.0, xegtao_isfast_edge_sensitivity);
  const bool temporalJitterEnabled = xegtao_isfast_jitter_mode >= 0.5;
  const float jitterAmount = temporalJitterEnabled ? saturate(xegtao_isfast_jitter_amount) : 0.0;
  const uint noiseFrame = (uint)max(xegtao_noise_index, 0.0);

  float visibilitySum = centerTerm.w;
  float3 bentNormalSum = centerTerm.xyz;
  float weightSum = 1.0;

  [loop]
  for (uint sampleIndex = 0u; sampleIndex < sampleCount; ++sampleIndex)
  {
    const float2 xi = GetISFASTNoise2D(dispatchThreadId, sampleIndex, noiseFrame, jitterAmount);
    const float angle = xi.x * 6.28318530718;
    const float radial = sqrt(saturate(xi.y)) * filterRadius;
    const float2 offset = float2(cos(angle), sin(angle)) * radial;
    const int2 sampleCoord = int2(round((float2)dispatchThreadId + offset));

    const lpfloat4 sampleTerm = LoadDecodedAOTerm(sampleCoord, size);
    const lpfloat sampleEdge = LoadEdgeValue(sampleCoord, size);

    const float spatialWeight = exp(-dot(offset, offset) / sigma2);
    const float edgeDelta = abs(sampleEdge - centerEdge);
    const float edgeWeight = exp(-edgeDelta * edgeSensitivity * 8.0);
    const float sampleWeight = spatialWeight * edgeWeight;

    visibilitySum += sampleTerm.w * sampleWeight;
    bentNormalSum += sampleTerm.xyz * sampleWeight;
    weightSum += sampleWeight;
  }

  const float invWeight = rcp(max(weightSum, 1e-5));
  float filteredVisibility = saturate(visibilitySum * invWeight);
  float3 filteredBentNormal = SafeNormalize3(bentNormalSum * invWeight, float3(0.0, 0.0, 1.0));

  if (xegtao_denoiser_mode >= 1.5)
  {
    const float hybridBlend = saturate(xegtao_isfast_hybrid_blend);
    filteredVisibility = lerp(centerTerm.w, filteredVisibility, hybridBlend);
    filteredBentNormal = SafeNormalize3(
        lerp(centerTerm.xyz, filteredBentNormal, hybridBlend),
        float3(0.0, 0.0, 1.0));
  }

  const bool finalApply = xegtao_denoise_is_last_pass >= 0.5;
  if (finalApply)
  {
    filteredVisibility *= XE_GTAO_OCCLUSION_TERM_SCALE;
  }

  g_outFinalAOTerm[dispatchThreadId] = XeGTAO_EncodeVisibilityBentNormal(
      (lpfloat)filteredVisibility,
      (lpfloat3)filteredBentNormal);
}
