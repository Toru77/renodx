#include "./shared.h"

struct CompositeToDisplayConstants {
  float4 nits;
  float4 uiScale;
  float4 huePreserveLerp;
  uint presentSurfaceBindlessIndex;
  uint uiSurfaceBindlessIndex;
  uint _unused_;
  uint linearComposite;
  float3 pipelineToOutputColorSpace[3];
};

Texture2D<float4> srv2DTextures[65536] : register(t0, space1);

cbuffer cbPostprocessSource : register(b3) {
  uint source0Tex2dIndex : packoffset(c000.x);
  uint source1Tex2dIndex : packoffset(c000.y);
  uint source2Tex2dIndex : packoffset(c000.z);
  uint source3Tex2dIndex : packoffset(c000.w);
  uint source4Tex2dIndex : packoffset(c001.x);
  uint source5Tex2dIndex : packoffset(c001.y);
  uint source6Tex2dIndex : packoffset(c001.z);
  uint source7Tex2dIndex : packoffset(c001.w);
};

cbuffer cbCompositeToDisplayConstants : register(b0) {
  CompositeToDisplayConstants cbCompositeToDisplayConstants : packoffset(c000.x);
};

SamplerState g_staticSampler_BilinearClamp : register(s18);

uint firstbithigh_msb(int value) { return (value == 0) ? 0xFFFFFFFF : (31u - firstbithigh(value)); }
uint firstbithigh_msb(uint value) { return (value == 0) ? 0xFFFFFFFF : (31u - firstbithigh(value)); }

static const uint  kBindlessIndexMask      = 0x000FFFFFu;
static const uint  kBindlessHeaderMask     = 0xFFF00000u;
static const uint  kValidBindlessHeader    = 0xC3500000u;
static const uint  kFallbackTextureIndex   = 17u;

static const float kPqInvM2                = 0.012683313339948654f;
static const float kPqC1                   = 0.8359375f;
static const float kPqC2                   = 18.8515625f;
static const float kPqC3                   = 18.6875f;
static const float kPqInvM1                = 6.277394771575928f;
static const float kPqMaxNits              = 10000.0f;

uint ResolveBindlessTextureIndex(uint packedTextureIndex) {
  const bool hasValidHeader = ((packedTextureIndex & kBindlessHeaderMask) == kValidBindlessHeader);
  return hasValidHeader ? (packedTextureIndex & kBindlessIndexMask) : kFallbackTextureIndex;
}

float3 DecodePqToNits(float3 pqColor) {
  const float3 pqPow = pow(pqColor, kPqInvM2);
  const float3 pqNumerator = max(float3(0.0f, 0.0f, 0.0f), pqPow - kPqC1);
  const float3 pqDenominator = kPqC2 - (pqPow * kPqC3);
  const float3 linearPq = exp2(log2(pqNumerator / pqDenominator) * kPqInvM1);
  return linearPq * kPqMaxNits;
}

float2 ScaleUiTexCoord(float2 texCoord) {
  return (cbCompositeToDisplayConstants.uiScale.xy * (texCoord - float2(0.5f, 0.5f))) + float2(0.5f, 0.5f);
}

float4 main(
  precise noperspective float4 SV_Position : SV_Position,
  linear float2 TEXCOORD : TEXCOORD
) : SV_Target {
  const uint presentTextureIndex = ResolveBindlessTextureIndex(source0Tex2dIndex);
  const uint uiTextureIndex      = ResolveBindlessTextureIndex(source1Tex2dIndex);

  float4 presentSample = srv2DTextures[presentTextureIndex].Sample(
    g_staticSampler_BilinearClamp,
    TEXCOORD
  );

  presentSample = max(presentSample, float4(0.0f, 0.0f, 0.0f, 0.0f));

  const float4 uiSample = srv2DTextures[uiTextureIndex].Sample(
    g_staticSampler_BilinearClamp,
    ScaleUiTexCoord(TEXCOORD)
  );

  const float  presentBlendWeight = 1.0f - uiSample.a;
  const float3 presentDisplayRgb  = DecodePqToNits(presentSample.rgb) / cbCompositeToDisplayConstants.nits.x;
  const float3 uiDisplayRgb       = uiSample.rgb * cbCompositeToDisplayConstants.nits.y;

  float4 outputColor;
  outputColor.rgb = (presentBlendWeight * presentDisplayRgb) + uiDisplayRgb;
  outputColor.a   = (presentBlendWeight * presentSample.a) + (uiSample.a * cbCompositeToDisplayConstants.nits.y);

  outputColor = max(outputColor, 0);
  outputColor.rgb = saturate(outputColor.rgb);

  return outputColor;
}
