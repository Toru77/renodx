// FXAAHigh.cs_5_0.hlsl — FXAA 3.11 Quality, preset 29.
//
// Preset 29 is the highest non-extreme step table (FXAA_QUALITY__PS 12,
// longest edge-search radius). Same wrapper and reference contract as
// FXAAStandard.cs_5_0.hlsl; only the preset define differs. See
// fxaa_common.hlsli for the call-site contract and all deviations.
#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 29
#define FXAA_LUMA_SEPARATE_R8 1
#define FXAA_GATHER4_ALPHA 0
#include "../reference/taa/Fxaa3_11.h"
#include "fxaa_common.hlsli"
