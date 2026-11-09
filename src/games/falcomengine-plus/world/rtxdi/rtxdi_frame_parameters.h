#pragma once

#ifdef __cplusplus
#include <cstdint>
#endif

#include <Rtxdi/DI/ReSTIRDIParameters.h>

struct RtxdiFrameParameters {
  uint32_t width;
  uint32_t height;
  uint32_t history_valid;
  uint32_t flags;
  RTXDI_RuntimeParameters runtime;
  RTXDI_LightBufferParameters lights;
  RTXDI_Parameters di;
};

#ifdef __cplusplus
#include <cstddef>
static_assert(offsetof(RtxdiFrameParameters, runtime) == 16u);
static_assert(offsetof(RtxdiFrameParameters, lights) == 32u);
static_assert(offsetof(RtxdiFrameParameters, di) == 80u);
static_assert(sizeof(RtxdiFrameParameters) == 352u);
inline constexpr size_t kRtxdiFrameCbvBytes = (sizeof(RtxdiFrameParameters) + 255u) & ~size_t{255u};
static_assert(kRtxdiFrameCbvBytes == 512u);
struct RtxdiFrameUpload {
  RtxdiFrameParameters parameters = {};
  uint8_t padding[kRtxdiFrameCbvBytes - sizeof(RtxdiFrameParameters)] = {};
};
static_assert(sizeof(RtxdiFrameUpload) == kRtxdiFrameCbvBytes);
#endif
