#ifndef FALCOM_WORLD_PT_RESTIR_LAYOUT_H_
#define FALCOM_WORLD_PT_RESTIR_LAYOUT_H_

#ifdef __cplusplus
#include <cstdint>
#endif

#include <Rtxdi/PT/ReSTIRPTParameters.h>
#include <Rtxdi/DI/ReSTIRDIParameters.h>
#include <Rtxdi/RtxdiParameters.h>

struct PtRestirConstants {
  RTXDI_RuntimeParameters runtime;
  RTXDI_LightBufferParameters light_buffer;
  RTXDI_DIInitialSamplingParameters nee;
  RTXDI_PTParameters restir;
  uint32_t prev_light_count;
  uint32_t history_valid;
  uint32_t pad0;
  uint32_t pad1;
};

#ifdef __cplusplus
static_assert(sizeof(PtRestirConstants) % 16 == 0);
inline constexpr uint32_t kPtRestirConstantsBytes = (sizeof(PtRestirConstants) + 255u) & ~255u;
#endif

#endif  // FALCOM_WORLD_PT_RESTIR_LAYOUT_H_
