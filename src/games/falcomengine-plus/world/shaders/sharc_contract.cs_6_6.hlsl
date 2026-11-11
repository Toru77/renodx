#define SHARC_UPDATE 1
#define SHARC_ENABLE_RESPONSIVE_LIGHTING 0
#define SHARC_MATERIAL_DEMODULATION 1
#define SHARC_ENABLE_SH_ENCODING 1
#define SHARC_ENABLE_CACHE_RESAMPLING 1
#define SHARC_PROPAGATION_DEPTH 3
#define SHARC_RESAMPLING_DEPTH_MIN 2
#define SHARC_SAMPLE_NUM_THRESHOLD 3
#include "../sharc/vendor/include/SharcCommon.h"
RWStructuredBuffer<uint64_t> hashEntries : register(u0);
RWStructuredBuffer<SharcAccumulationData> accumulation : register(u1);
RWStructuredBuffer<SharcPackedData> resolved : register(u2);
cbuffer Params : register(b0) {
  float3 camera; uint capacity;
  float3 position; float random;
};
[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  SharcParameters parameters;
  parameters.hashGridParameters.cameraPosition = camera;
  parameters.hashGridParameters.logarithmBase = 2;
  parameters.hashGridParameters.sceneScale = 50;
  parameters.hashGridParameters.levelBias = 0;
  parameters.hashGridData.capacity = capacity;
  parameters.hashGridData.hashEntriesBuffer = hashEntries;
  parameters.radianceScale = 1000;
  parameters.accumulationBuffer = accumulation;
  parameters.resolvedBuffer = resolved;
  SharcState state;
  SharcInit(state);
  SharcHitData hit = (SharcHitData)0;
  hit.positionWorld = position;
  hit.normalWorld = float3(0, 1, 0);
  hit.materialDemodulation = 1;
  SharcUpdateHit(parameters, state, hit, 0, random);
  SharcSetThroughput(state, 1);
  SharcUpdateMiss(parameters, state, 0);
  SharcResolveParameters resolve;
  resolve.cameraPositionPrev = camera;
  resolve.accumulationFrameNum = 32;
  resolve.responsiveFrameNum = 4;
  resolve.staleFrameNumMax = 64;
  resolve.frameIndex = 0;
  SharcResolveEntry(id.x, parameters, resolve);
}
