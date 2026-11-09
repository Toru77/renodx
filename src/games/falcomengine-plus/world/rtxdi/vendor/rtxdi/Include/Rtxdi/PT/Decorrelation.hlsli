/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
    10| * its affiliates is strictly prohibited.
 */

#ifndef RTXDI_PT_DECORRELATION_HLSLI
#define RTXDI_PT_DECORRELATION_HLSLI

#include "Rtxdi/PT/ReSTIRPTParameters.h"
#include "Rtxdi/PT/Reservoir.hlsli"

#ifdef RTXDI_ENABLE_BOILING_FILTER
#include "Rtxdi/Utils/BoilingFilter.hlsli"
#endif

// Stochastic final-shading decorrelation: with a Uniform or Stagnancy-driven probability
// (and always for boiling-filter-flagged fireflies) replace the resampled reservoir with
// the preserved, unresampled initial-sampling reservoir.
//
// Apply reads these application resource aliases (same set as DuplicationMap.hlsli):
//   RTXDI_PT_DUPLICATION_MAP           - RWTexture2D<float2>  .y = raw stagnancy
//   RTXDI_PT_SMOOTHED_DUPLICATION_MAP  - RWTexture2D<float>   smoothed stagnancy

bool RTXDI_PTNeedsDuplicationMap(
    RTXDI_PTTemporalResamplingParameters temporalResampling,
    RTXDI_PTDecorrelationParameters decorrelation)
{
    const bool stagnancyDecorrelation =
        decorrelation.decorrelationMode == RTXDI_PT_DECORRELATION_MODE_STAGNANCY &&
        decorrelation.decorrelationFactor > 0.0f;
    return temporalResampling.duplicationBasedHistoryReduction != 0 || stagnancyDecorrelation;
}

bool RTXDI_PTNeedsDuplicationInputs(uint outputBufferIndex, RTXDI_PTParameters restirPT)
{
    return outputBufferIndex == restirPT.bufferIndices.finalShadingInputBufferIndex &&
        RTXDI_PTNeedsDuplicationMap(restirPT.temporalResampling, restirPT.decorrelation);
}

bool RTXDI_PTNeedsPreservedInitialSample(RTXDI_PTDecorrelationParameters decorrelation)
{
    return decorrelation.decorrelationFactor > 0.0f;
}

void RTXDI_PTDetectDecorrelationFireflies(
    uint2 localIndex,
    inout RTXDI_PTReservoir reservoir,
    RTXDI_PTDecorrelationParameters decorrelation,
    RTXDI_PTBufferIndices bufferIndices)
{
    RTXDI_SetFireflyDetected(reservoir, false);
#ifdef RTXDI_ENABLE_BOILING_FILTER
    const bool temporalResamplingActive =
        bufferIndices.temporalResamplingInputBufferIndex !=
        bufferIndices.temporalResamplingOutputBufferIndex;
    if (temporalResamplingActive &&
        decorrelation.fireflyReplacementFilterEnable != 0 &&
        decorrelation.decorrelationMode != RTXDI_PT_DECORRELATION_MODE_NONE &&
        decorrelation.decorrelationFactor > 0.0f)
    {
        const float fireflyWeight = RTXDI_Luminance(reservoir.targetFunction) * reservoir.weightSum;
        RTXDI_SetFireflyDetected(reservoir, RTXDI_BoilingFilterInternal(
            localIndex, decorrelation.fireflyReplacementFilterStrength, fireflyWeight));
    }
#endif
}

// Returns the per-pixel decorrelation probability actually used (0 when inactive).
float RTXDI_PTApplyDecorrelation(
    uint2 pixelPosition,
    uint2 reservoirPosition,
    inout RTXDI_PTReservoir ptReservoir,
    RTXDI_PTDecorrelationParameters decorrelation,
    RTXDI_PTBufferIndices bufferIndices,
    RTXDI_ReservoirBufferParameters reservoirBuffer,
    uint frameIndex)
{
    const float decorrelationFactor = saturate(decorrelation.decorrelationFactor);
    const uint decorrelationMode = decorrelation.decorrelationMode;
    float scaledDecorrelationFactor = 0.0f;

    const bool temporalResamplingActive =
        bufferIndices.temporalResamplingInputBufferIndex !=
        bufferIndices.temporalResamplingOutputBufferIndex;

    if(decorrelationMode == RTXDI_PT_DECORRELATION_MODE_NONE ||
      decorrelationFactor <= 0.0f ||
      !temporalResamplingActive)
        return scaledDecorrelationFactor;

    if (decorrelationMode == RTXDI_PT_DECORRELATION_MODE_UNIFORM)
    {
        scaledDecorrelationFactor = decorrelationFactor;
    }
    else if (decorrelationMode == RTXDI_PT_DECORRELATION_MODE_STAGNANCY)
    {
        const float stagnancy = saturate(RTXDI_PT_SMOOTHED_DUPLICATION_MAP[pixelPosition]);
        const float stagnancyExponent = max(0.0f, decorrelation.decorrelationStagnancyExponent);
        const float stagnancyScale = pow(stagnancy, stagnancyExponent);
        scaledDecorrelationFactor = saturate(4 * decorrelationFactor * stagnancyScale);
    }

    if (decorrelation.fireflyReplacementFilterEnable != 0 &&
        RTXDI_GetFireflyDetected(ptReservoir))
    {
        scaledDecorrelationFactor = 1.f;
    }

    if (scaledDecorrelationFactor > 0.0f)
    {
        RTXDI_RandomSamplerState decorrelationRng = RTXDI_InitRandomSampler(pixelPosition, frameIndex, RTXDI_PT_FINAL_SHADING_RANDOM_SEED);
        float prevWeightSum = ptReservoir.weightSum;
        if (RTXDI_GetNextRandom(decorrelationRng) < scaledDecorrelationFactor)
        {
            ptReservoir = RTXDI_LoadPTReservoir(reservoirBuffer, reservoirPosition, bufferIndices.initialPathTracerPreservedBufferIndex);

            if (decorrelation.fireflyReplacementBiasReduction != 0)
            {
                float stagnancy = saturate(RTXDI_PT_DUPLICATION_MAP[pixelPosition].y);
                if (stagnancy == 0.f)
                {
                    ptReservoir.weightSum = min(decorrelation.fireflyReplacementMultiplyBound * prevWeightSum, ptReservoir.weightSum);
                }
            }
        }
    }

    return scaledDecorrelationFactor;
}

#endif // RTXDI_PT_DECORRELATION_HLSLI
