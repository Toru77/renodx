/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#ifndef RTXDI_PT_TEMPORAL_RESAMPLING_HLSLI
#define RTXDI_PT_TEMPORAL_RESAMPLING_HLSLI

#include "Rtxdi/Utils/Math.hlsli"
#define RTXDI_RESTIR_PT_HYBRID_SHIFT
#include "Rtxdi/PT/HybridShift.hlsli"
#include "Rtxdi/Utils/Checkerboard.hlsli"

#define NEIGHBOR_FIND_MODE_GI 1
#define NEIGHBOR_FIND_MODE_IDENTITY 2

#define NEIGHBOR_FIND_MODE NEIGHBOR_FIND_MODE_GI

struct RTXDI_PTTemporalResamplingRuntimeParameters
{
    uint2 pixelPosition;
    uint2 reservoirPosition;
    float3 motionVector;

    float3 cameraPos;
    float3 prevCameraPos;
    float3 prevPrevCameraPos;
};

RTXDI_PTTemporalResamplingRuntimeParameters RTXDI_EmptyPTTemporalResamplingRuntimeParameters()
{
    return (RTXDI_PTTemporalResamplingRuntimeParameters)0;
}

bool IsValidNeighborSurface(RAB_Surface surfA, RAB_Surface surfB, float normalThreshold, float depthThreshold)
{
    return RTXDI_IsValidNeighbor(RAB_GetSurfaceNormal(surfA), RAB_GetSurfaceNormal(surfB), RAB_GetSurfaceLinearDepth(surfA), RAB_GetSurfaceLinearDepth(surfB), normalThreshold, depthThreshold);
}

#if NEIGHBOR_FIND_MODE == NEIGHBOR_FIND_MODE_GI

// Generates a pattern of offsets for looking closely around a given pixel.
// The pattern places 'sampleIdx' at the following locations in screen space around pixel (x):
//   0 4 3
//   6 x 7
//   2 5 1
int2 RTXDI_CalculateTemporalResamplingOffset(int sampleIdx, int radius)
{
    sampleIdx &= 7;

    int mask2 = sampleIdx >> 1 & 0x01;       // 0, 0, 1, 1, 0, 0, 1, 1
    int mask4 = 1 - (sampleIdx >> 2 & 0x01); // 1, 1, 1, 1, 0, 0, 0, 0
    int tmp0 = -1 + 2 * (sampleIdx & 0x01);  // -1, 1,....
    int tmp1 = 1 - 2 * mask2;                // 1, 1,-1,-1, 1, 1,-1,-1
    int tmp2 = mask4 | mask2;                // 1, 1, 1, 1, 0, 0, 1, 1
    int tmp3 = mask4 | (1 - mask2);          // 1, 1, 1, 1, 1, 1, 0, 0

    return int2(tmp0, tmp0 * tmp1) * int2(tmp2, tmp3) * radius;
}

int2 CalculateTemporalSurfacePosition(RTXDI_PTTemporalResamplingParameters tParams, RTXDI_RuntimeParameters rParams, int i, int temporalSampleStartIdx, int2 pixelPos, float radius, int2 prevPos, bool isFallbackSample)
{
    const bool isFirstSample = i == 0;
    int2 offset = int2(0, 0);
    if (isFallbackSample)
    {
        // Last sample is a fallback for disocclusion areas: use zero motion vector.
        prevPos = int2(pixelPos);
    }
    else if (!isFirstSample)
    {
        offset = RTXDI_CalculateTemporalResamplingOffset(temporalSampleStartIdx + i, radius);
    }

    int2 idx = prevPos + offset;
    if ((tParams.enablePermutationSampling && isFirstSample) || isFallbackSample)
    {
        // Apply permutation sampling for the first (non-jittered) sample,
        // also for the last (fallback) sample to prevent visible repeating patterns in disocclusions.
        RTXDI_ApplyPermutationSampling(idx, tParams.uniformRandomNumber);
    }

    RTXDI_ActivateCheckerboardPixel(idx, true, rParams.activeCheckerboardField);

    return idx;
}

bool ValidTemporalSurface(RTXDI_PTTemporalResamplingParameters tParams, RAB_Surface curSurface, RAB_Surface temporalSurface, float expectedPrevLinearDepth, bool isFallbackSample)
{
    if (!RAB_IsSurfaceValid(temporalSurface))
    {
        return false;
    }

    if (!isFallbackSample && !RTXDI_IsValidNeighbor(
        RAB_GetSurfaceNormal(curSurface), RAB_GetSurfaceNormal(temporalSurface),
        expectedPrevLinearDepth, RAB_GetSurfaceLinearDepth(temporalSurface),
        tParams.normalThreshold, tParams.depthThreshold))
    {
        return false;
    }

    if (!RAB_AreMaterialsSimilar(RAB_GetMaterial(curSurface), RAB_GetMaterial(temporalSurface)))
    {
        return false;
    }

    return true;
}

bool FindTemporalNeighbor_GI(const RTXDI_PTTemporalResamplingParameters tParams,
                             const RTXDI_RuntimeParameters rParams,
                             const RTXDI_ReservoirBufferParameters reservoirBufferParams,
                             const RTXDI_PTBufferIndices bufferIndices,
                             inout RTXDI_RandomSamplerState rng,
                             const int2 pixelPos,
                             const int2 prevPos,
                             const RAB_Surface curSurface,
                             const float expectedPrevLinearDepth,
                             inout RAB_Surface temporalSurface,
                             inout RTXDI_PTReservoir temporalReservoir)
{
    // Try to find a matching surface in the neighborhood of the reprojected pixel
    const int temporalSampleCount = 9;
    const int sampleCount = temporalSampleCount + (tParams.enableFallbackSampling ? 1 : 0);
    const int radius = (rParams.activeCheckerboardField == 0) ? 1 : 2;
    const int temporalSampleStartIdx = int(RTXDI_GetNextRandom(rng) * 8);
    for (int i = 0; i < sampleCount; i++)
    {
        const bool isFallbackSample = i == temporalSampleCount;

        int2 temporalSurfacePos = CalculateTemporalSurfacePosition(tParams, rParams, i, temporalSampleStartIdx, pixelPos, radius, prevPos, isFallbackSample);
        temporalSurface = RAB_GetGBufferSurface(temporalSurfacePos, true);

        if(!ValidTemporalSurface(tParams, curSurface, temporalSurface, expectedPrevLinearDepth, isFallbackSample))
        {
            continue;
        }

        uint2 prevReservoirPos = RTXDI_PixelPosToReservoirPos(temporalSurfacePos, rParams.activeCheckerboardField);
        temporalReservoir = RTXDI_LoadPTReservoir(reservoirBufferParams, prevReservoirPos, bufferIndices.temporalResamplingInputBufferIndex);

        if (!RTXDI_IsValidPTReservoir(temporalReservoir))
        {
            continue;
        }

        return true;
    }

    return false;
}

#elif NEIGHBOR_FIND_MODE == NEIGHBOR_FIND_MODE_IDENTITY

bool FindTemporalNeighbor_Identity(RTXDI_RuntimeParameters rParams,
                                   RTXDI_ReservoirBufferParameters reservoirBufferParams,
                                   RTXDI_PTBufferIndices bufferIndices,
                                   inout int2 prevReservoirPos,
                                   const int2 pixelPosition,
                                   const RAB_Surface curSurface,
                                   inout RAB_Surface temporalSurface,
                                   inout RTXDI_PTReservoir temporalReservoir)
{
    int2 temporalSampleIdx = pixelPosition;

    temporalSurface = RAB_GetGBufferSurface(temporalSampleIdx, true);
    if(!RAB_IsSurfaceValid(temporalSurface))
    {
        return false;
    }

    prevReservoirPos = RTXDI_PixelPosToReservoirPos(temporalSampleIdx, rParams.activeCheckerboardField);
    temporalReservoir = RTXDI_LoadPTReservoir(reservoirBufferParams, prevReservoirPos, bufferIndices.temporalResamplingInputBufferIndex);
    if (!RTXDI_IsValidPTReservoir(temporalReservoir))
    {
        return false;
    }

    return true;
}

#endif

bool FindTemporalNeighbor(RTXDI_PTTemporalResamplingParameters tParams,
                          RTXDI_RuntimeParameters rParams,
                          RTXDI_ReservoirBufferParameters bufferParams,
                          RTXDI_PTBufferIndices bufferIndices,
                          inout RTXDI_RandomSamplerState rng,
                          int2 pixelPos,
                          int2 prevPos,
                          RAB_Surface curSurface,
                          float expectedPrevLinearDepth,
                          inout RAB_Surface temporalSurface,
                          inout RTXDI_PTReservoir prevSample)
{
#if NEIGHBOR_FIND_MODE == NEIGHBOR_FIND_MODE_GI
    return FindTemporalNeighbor_GI(tParams, rParams, bufferParams, bufferIndices, rng, pixelPos, prevPos, curSurface, expectedPrevLinearDepth, temporalSurface, prevSample);
#elif NEIGHBOR_FIND_MODE == NEIGHBOR_FIND_MODE_IDENTITY
    return FindTemporalNeighbor_Identity(rParams, bufferParams, bufferIndices, prevPos, pixelPos, curSurface, temporalSurface, prevSample);
#endif
}

bool ValidateTemporalNeighbor(const RTXDI_PTTemporalResamplingParameters tParams,
                              inout RTXDI_PTReservoir prevSample,
                              const RAB_Surface curSurface,
                              const RAB_Surface prevSurface,
                              inout int reducedMaxTemporalHistory)
{
    prevSample.M = min(prevSample.M, min(RTXDI_PTReservoir::maxM, reducedMaxTemporalHistory));
    prevSample.age = min(RTXDI_PTRESERVOIR_AGE_MAX, prevSample.age + 1);

    bool foundNeighbor = true;
    if (tParams.enableAgeBasedRejection && prevSample.age > min(RTXDI_PTRESERVOIR_AGE_MAX, tParams.maxReservoirAge))
    {
        foundNeighbor = false;
    }

    return foundNeighbor;
}

bool ShouldEvaluateTemporalReconnection(RTXDI_PTReservoir prevSample)
{
    return !(((prevSample.rcVertexLength == prevSample.pathLength) && RTXDI_ConnectsToNeeLight(prevSample)) || (prevSample.rcVertexLength > prevSample.pathLength));
}

void EvaluateReconnection(const RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                          inout float3 targetFunction,
                          const RAB_Surface curSurface,
                          const RTXDI_PTReservoir prevSample,
                          const bool checkVisibility)
{
    targetFunction *= RAB_GetPTSampleTargetPdfForSurface(prevSample.translatedWorldPosition, prevSample.radiance, curSurface);

    if (checkVisibility && any(targetFunction > 0.f))
    {
        targetFunction *= RAB_GetConservativeVisibility(curSurface, prevSample.translatedWorldPosition);
    }
}

RTXDI_PTHybridShiftRuntimeParameters BuildHSRShiftParams(const RTXDI_PTTemporalResamplingRuntimeParameters trrParams)
{
    RTXDI_PTHybridShiftRuntimeParameters hsrParams = (RTXDI_PTHybridShiftRuntimeParameters)0;
    hsrParams.isBasePathInPrevFrame = true;
    hsrParams.isPrevFrame = false;
    hsrParams.cameraPos = trrParams.cameraPos;
    hsrParams.prevCameraPos = trrParams.prevCameraPos;
    hsrParams.prevPrevCameraPos = trrParams.prevPrevCameraPos;
    return hsrParams;
}

RTXDI_PTHybridShiftRuntimeParameters BuildHSRInverseShiftParams(const RTXDI_PTTemporalResamplingRuntimeParameters trrParams)
{
    RTXDI_PTHybridShiftRuntimeParameters hsrParams = (RTXDI_PTHybridShiftRuntimeParameters)0;
    hsrParams.isBasePathInPrevFrame = false;
    hsrParams.isPrevFrame = true;
    hsrParams.cameraPos = trrParams.cameraPos;
    hsrParams.prevCameraPos = trrParams.prevCameraPos;
    hsrParams.prevPrevCameraPos = trrParams.prevPrevCameraPos;
    return hsrParams;
}

#if RTXDI_DEBUG == 1
void TemporalRetrace(const RTXDI_PTTemporalResamplingRuntimeParameters trrParams,
                     const RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                     RTXDI_PTReconnectionParameters rcParams,
                     RAB_Surface prevSurfCopy,
                     RTXDI_PTReservoir prevSampleCopy,
                     inout RAB_PathTracerUserData ptud)
{
    float3 targetFunction = float3(0.0f, 0.0f, 0.0f);
    float jacobian = 0;
    prevSampleCopy.pathLength = max(prevSampleCopy.pathLength, hspfParams.maxBounceDepth);
    prevSampleCopy.rcVertexLength = prevSampleCopy.pathLength + 1;
    rcParams.roughnessThreshold = 1.0;
    rcParams.distanceThreshold = 1e6;
    rcParams.reconnectionMode = RTXDI_RESTIRPT_RECONNECTION_MODE_FIXED_THRESHOLD;
    RAB_PathTracerUserDataSetPathType(ptud, RTXDI_PTPathTraceInvocationType_DebugTemporalRetrace);
    RTXDI_ComputeHybridShift(prevSurfCopy, prevSampleCopy, hspfParams, BuildHSRShiftParams(trrParams), rcParams, targetFunction, jacobian, ptud);
}
#endif

// Shift previous sample - random replay to new location
void ResampleTemporalNeighbor(const RTXDI_PTTemporalResamplingParameters tParams,
                              const RTXDI_PTTemporalResamplingRuntimeParameters trrParams,
                              const RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                              const RTXDI_PTReconnectionParameters rcParams,
                              inout RTXDI_RandomSamplerState rng,
                              inout RTXDI_PTReservoir targetReservoir,
                              inout RAB_Surface curSurface,
                              inout RTXDI_PTReservoir prevSample,
                              inout float jacobian,
                              inout float3 selectedTargetFunction,
                              inout bool selectedPrevSample,
                              inout RAB_PathTracerUserData ptud)
{
    float3 targetFunction = float3(1.0f, 1.0f, 1.0f);
    bool shouldEvaluateReconnection = ShouldEvaluateTemporalReconnection(prevSample);

    // The random replay (prefix tracing) pass taken by hybrid shift
    // this will update curSurface to the vertex before reconnection
    // If the reconnection vertex is a NEE-sampled light vertex (UseRTXDILight), it will be handled inside this function
    RAB_PathTracerUserDataSetPathType(ptud, RTXDI_PTPathTraceInvocationType_Temporal);
    RTXDI_ComputeHybridShift(curSurface, prevSample, hspfParams, BuildHSRShiftParams(trrParams), rcParams, targetFunction, jacobian, ptud);

    prevSample.weightSum *= jacobian;

    if (shouldEvaluateReconnection)
    {
        EvaluateReconnection(hspfParams, targetFunction, curSurface, prevSample, tParams.enableVisibilityBeforeCombine);
    }

    if (RTXDI_CombinePTReservoirs(targetReservoir, prevSample, RTXDI_GetNextRandom(rng), targetFunction))
    {
        selectedPrevSample = true;
        selectedTargetFunction = targetFunction;
    }
}

void BiasCorrection(const RTXDI_PTTemporalResamplingRuntimeParameters trrParams,
                    const RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                    const RTXDI_PTReconnectionParameters rcParams,
                    inout RTXDI_RandomSamplerState rng,
                    inout RTXDI_PTReservoir targetReservoir,
                    inout RAB_Surface curSurface,
                    inout RAB_Surface prevSurface,
                    inout RTXDI_PTReservoir prevSample,
                    const bool selectedPrevSample,
                    inout float pi,
                    inout float piSum,
                    inout RAB_PathTracerUserData ptud)
{
    float3 targetFunction = float3(1.0f, 1.0f, 1.0f);
    bool shouldEvaluateReconnection = ShouldEvaluateTemporalReconnection(targetReservoir);

    float jacobian = RTXDI_CalculateJacobian(RAB_GetSurfaceWorldPos(prevSurface), RAB_GetSurfaceWorldPos(curSurface), targetReservoir.translatedWorldPosition, targetReservoir.worldNormal);
    float backupPartialJacobian = targetReservoir.partialJacobian; // for MIS weight computation, we don't want targetReservoir value to be eventually overwritten

    RAB_PathTracerUserDataSetPathType(ptud, RTXDI_PTPathTraceInvocationType_TemporalInverse);
    RTXDI_ComputeHybridShift(prevSurface, targetReservoir, hspfParams, BuildHSRInverseShiftParams(trrParams), rcParams, targetFunction, jacobian, ptud);

    targetReservoir.partialJacobian = backupPartialJacobian;

    if (shouldEvaluateReconnection)
    {
        EvaluateReconnection(hspfParams, targetFunction, prevSurface, targetReservoir, true);
    }

    float temporalP = RTXDI_Luminance(targetFunction) * jacobian;

    pi = selectedPrevSample ? temporalP : pi;
    piSum += temporalP * prevSample.M;
}

RTXDI_PTReservoir RTXDI_PTTemporalResampling(RTXDI_PTTemporalResamplingParameters tParams,
                                             RTXDI_PTTemporalResamplingRuntimeParameters trrParams,
                                             RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                                             RTXDI_PTReconnectionParameters rcParams,
                                             RTXDI_RuntimeParameters rParams,
                                             RTXDI_ReservoirBufferParameters bufferParams,
                                             RTXDI_RandomSamplerState rng,
                                             RTXDI_PTBufferIndices bufferIndices,
                                             inout bool selectedPrevSample,
                                             inout RAB_PathTracerUserData ptud)
{
    RAB_Surface curSurface = RAB_GetGBufferSurface(trrParams.pixelPosition, false);
    if (!RAB_IsSurfaceValid(curSurface))
    {
        return RTXDI_EmptyPTReservoir();
    }

    RTXDI_PTReservoir targetReservoir = RTXDI_EmptyPTReservoir();
    RTXDI_PTReservoir curSample = RTXDI_LoadPTReservoir(bufferParams, trrParams.reservoirPosition, bufferIndices.initialPathTracerOutputBufferIndex);
    float3 selectedTargetFunction = float3(0.0f, 0.0f, 0.0f);

    if (RTXDI_IsValidPTReservoir(curSample))
    {
        if (RTXDI_CombinePTReservoirs(targetReservoir, curSample, /* random = */ 0.5, curSample.targetFunction))
        {
            selectedTargetFunction = curSample.targetFunction;
        }
    }

    int2 prevPos = int2(round(float2(trrParams.pixelPosition) + trrParams.motionVector.xy));
    float expectedPrevLinearDepth = RAB_GetSurfaceLinearDepth(curSurface) + trrParams.motionVector.z;
    RTXDI_PTReservoir prevSample = RTXDI_EmptyPTReservoir();
    RAB_Surface prevSurface = RAB_EmptySurface();

    bool foundNeighbor = FindTemporalNeighbor(tParams, rParams, bufferParams, bufferIndices, rng, trrParams.pixelPosition, prevPos, curSurface, expectedPrevLinearDepth, prevSurface, prevSample);

#if RTXDI_DEBUG == 1
    if(foundNeighbor)
    {
        TemporalRetrace(trrParams, hspfParams, rcParams, prevSurface, prevSample, ptud);
    }
#endif

    int reducedMaxTemporalHistory = tParams.maxHistoryLength;
    // Duplication-based history reduction: reduce MCap where many pixels share the same sample ID
    if (tParams.duplicationBasedHistoryReduction && foundNeighbor && tParams.historyReductionStrength > 0.f)
    {
        uint dupCount = RAB_GetDuplicationMapCount(prevPos);
        static const float totalNeighborCountInDupmapRegion = 288.0;
        float impoverishment = saturate(float(dupCount) / totalNeighborCountInDupmapRegion);
        float powerFactor = 0.1 * pow(2,6 * (1.f - tParams.historyReductionStrength) - 3);
        float t = pow(impoverishment, powerFactor);
        reducedMaxTemporalHistory = max(1, (int)lerp((float)tParams.maxHistoryLength, 1.0, t));
    }
    float jacobian = 1.f;
    if (foundNeighbor)
    {
        jacobian = RTXDI_CalculateJacobian(RAB_GetSurfaceWorldPos(curSurface), RAB_GetSurfaceWorldPos(prevSurface), prevSample.translatedWorldPosition, prevSample.worldNormal);
        foundNeighbor = ValidateTemporalNeighbor(tParams, prevSample, curSurface, prevSurface, reducedMaxTemporalHistory);
    }

    selectedPrevSample = false;
    if (foundNeighbor)
    {
        ResampleTemporalNeighbor(tParams, trrParams, hspfParams, rcParams, rng, targetReservoir, curSurface, prevSample, jacobian, selectedTargetFunction, selectedPrevSample, ptud);
    }

    float pi = RTXDI_Luminance(selectedTargetFunction);
    float piSum = RTXDI_Luminance(selectedTargetFunction) * curSample.M;
    if (RTXDI_IsValidPTReservoir(targetReservoir) && foundNeighbor)
    {
        BiasCorrection(trrParams, hspfParams, rcParams, rng, targetReservoir, curSurface, prevSurface, prevSample, selectedPrevSample, pi, piSum, ptud);
    }

    const float normalizationNumerator = pi;
    const float normalizationDenominator = piSum * RTXDI_Luminance(selectedTargetFunction);
    RTXDI_FinalizePTResampling(targetReservoir, normalizationNumerator, normalizationDenominator);
    if (tParams.duplicationBasedHistoryReduction)
    {
        // cannot use ReducedMaxTemporalHistory > targetReservoir.M to test because targetReservoir.M fluctuates every frame
        RTXDI_SetShouldBoostSpatialSamples(targetReservoir, !foundNeighbor);
    }

    return targetReservoir;
}

#endif // RTXDI_PT_TEMPORAL_RESAMPLING_HLSLI