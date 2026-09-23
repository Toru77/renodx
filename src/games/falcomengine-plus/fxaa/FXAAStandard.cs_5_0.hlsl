// FXAAStandard.cs_5_0.hlsl — FXAA 3.11 Quality, preset 12.
//
// Preset 12 is the reference default (FXAA_QUALITY__PS 5). The wrapper,
// call-site contract, and all deviations from the reference are documented
// in fxaa_common.hlsli. The reference file reference/taa/Fxaa3_11.h is
// included untouched.
#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 12
#define FXAA_LUMA_SEPARATE_R8 1
#define FXAA_GATHER4_ALPHA 0
#include "../reference/taa/Fxaa3_11.h"
#include "fxaa_common.hlsli"
