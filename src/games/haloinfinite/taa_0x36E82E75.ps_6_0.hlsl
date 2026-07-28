#include "./common.hlsl"

Texture2D<float4> srv2DTextures[65536] : register(t0, space1);

cbuffer cbTaaConstants : register(b0) {
  float4 tcHistoryTexelSize : packoffset(c000.x);
  float4 tcTargetTexelSize : packoffset(c001.x);
  float4 tcSourceTexelSize : packoffset(c002.x);
  float4 tcVelocityTexelSize : packoffset(c003.x);
  float4 tcJitterOffset : packoffset(c004.x);
  float tcBlendValue : packoffset(c005.x);
  float tcVariableBlending : packoffset(c005.y);
  float tcDepthOcclusion : packoffset(c005.z);
  float tcTransparentsMasking : packoffset(c005.w);
  float tcAntiFlicker : packoffset(c006.x);
  float tcHistorySharpness : packoffset(c006.y);
  int tcZoneClip : packoffset(c006.z);
  int tcDebugMode : packoffset(c006.w);
  float2 tcWindowSize : packoffset(c007.x);
  float2 tcWindowOffset : packoffset(c007.z);
  float2 tcInverseWindowSize : packoffset(c008.x);
  float2 tcScaledWindowOffset : packoffset(c008.z);
  uint4 tcVelocityBounds : packoffset(c009.x);
  int2 tcPixelOffset : packoffset(c010.x);
  uint2 tcPixelSize : packoffset(c010.z);
  int4 tcWindowClamp : packoffset(c011.x);
  uint texIndexDepth : packoffset(c012.x);
  uint texIndexVelocity : packoffset(c012.y);
  uint texIndexPresent : packoffset(c012.z);
  uint texIndexHistory : packoffset(c012.w);
  uint texIndexHistoryAux : packoffset(c013.x);
  uint texIndexCompressedDepth : packoffset(c013.y);
  uint texIndexStencil : packoffset(c013.z);
  uint uavIndexVelocity : packoffset(c013.w);
  uint uavIndexOutColor : packoffset(c014.x);
  uint uavIndexOutAux : packoffset(c014.y);
  uint uavIndexOutDebug : packoffset(c014.z);
  uint uavIndexCompressedDepth : packoffset(c014.w);
  float compressionScale : packoffset(c015.x);
  float tcAntiFlickerHistoryBias : packoffset(c015.y);
  float ditheredBlendValue : packoffset(c015.z);
  float ditheredAntiflicker : packoffset(c015.w);
  int ditheredZoneclip : packoffset(c016.x);
  uint texIndexDofTileData : packoffset(c016.y);
  float2 tileTexCoordMult : packoffset(c016.z);
  float dofHistoryDepthTolerance : packoffset(c017.x);
  float tcMovingBlendValue : packoffset(c017.y);
  float tcOccludedBlendValue : packoffset(c017.z);
  float tcMovingOccludedBlendValue : packoffset(c017.w);
  float velocityThresholdStart : packoffset(c018.x);
  float velocityThresholdRangeInv : packoffset(c018.y);
  float imageAspectRatio : packoffset(c018.z);
  float taaFovChange : packoffset(c018.w);
};

uint GetBindlessTextureIndex(uint packedIndex) {
  return (uint)(select(((packedIndex & -1048576) == -1018167296), (packedIndex & 1048575), 17)) + 0u;
}

struct OutputSignature {
  float4 SV_Target : SV_Target;
  float4 SV_Target_1 : SV_Target1;
};

OutputSignature main(
  precise noperspective float4 SV_Position : SV_Position,
  linear float2 TEXCOORD : TEXCOORD
) {
  const uint velocityTex = GetBindlessTextureIndex(texIndexVelocity);
  const uint presentTex = GetBindlessTextureIndex(texIndexPresent);
  const uint compressedDepthTex = GetBindlessTextureIndex(texIndexCompressedDepth);

  int2 targetPixel = int2(int(tcTargetTexelSize.z * TEXCOORD.x), int(tcTargetTexelSize.w * TEXCOORD.y));
  float2 targetUv = (float2(targetPixel) + 0.5f) * tcTargetTexelSize.xy;

  int2 initialVelocityPixel = int2(
    int((float(targetPixel.x) * tcTargetTexelSize.x) * tcSourceTexelSize.z),
    int((float(targetPixel.y) * tcTargetTexelSize.y) * tcSourceTexelSize.w)
  );
  float4 initialVelocity = srv2DTextures[velocityTex].Load(int3(initialVelocityPixel, 0));

  float transparentMask = select((initialVelocity.w == 0.0f), initialVelocity.z, 0.0f);
  float jitterx = tcJitterOffset.x * 2560;
  float jittery = tcJitterOffset.y * 1440;

  float2 unjitteredUv = targetUv - ((1.0f - transparentMask) * tcJitterOffset.xy);
  int2 sourcePixel = min(max(int2(unjitteredUv * tcSourceTexelSize.zw), tcWindowClamp.xy), tcWindowClamp.zw);

  float4 currentColor = srv2DTextures[presentTex].Load(int3(sourcePixel, 0));
  float4 velocityPacked = srv2DTextures[velocityTex].Load(int3(sourcePixel, 0));
  float4 compressedDepth = srv2DTextures[compressedDepthTex].Load(int3(sourcePixel, 0));

  OutputSignature output;
  output.SV_Target = float4(currentColor.rgb, 0.0f);
  output.SV_Target_1 = float4(1.0f, transparentMask, compressedDepth.y + 0.0004887585528194904f, select((velocityPacked.w == 0.3333333432674408f), 1.0099999904632568f, 0.0f));
  return output;
}
