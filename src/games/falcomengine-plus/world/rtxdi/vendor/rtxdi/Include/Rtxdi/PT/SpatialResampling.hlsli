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

#ifndef RTXDI_PT_SPATIAL_RESAMPLING_HLSLI
#define RTXDI_PT_SPATIAL_RESAMPLING_HLSLI

#include "Rtxdi/PT/HybridShift.hlsli"
#include "Rtxdi/PT/Reservoir.hlsli"
#include "Rtxdi/Utils/Math.hlsli"

// using uint mask to track samples, so absolute limit is 32
static const uint RTXDI_MAX_SPATIAL_RESAMPLING_COUNT = 32;

#ifndef SPATIAL_HEURISTIC_MAX_NEIGHBORS
#define SPATIAL_HEURISTIC_MAX_NEIGHBORS 4
#endif
#ifndef SPATIAL_HEURISTIC_INVALID_NEIGHBOR
#define SPATIAL_HEURISTIC_INVALID_NEIGHBOR 0xFFFFFFFFu
#endif

struct RTXDI_PTSpatialResamplingRuntimeParameters
{
    uint2 pixelPosition;
    uint2 reservoirPosition;

    float3 cameraPos;
    float3 prevCameraPos;
    float3 prevPrevCameraPos;

    uint2 viewportSize;
};

RTXDI_PTSpatialResamplingRuntimeParameters RTXDI_EmptyPTSpatialResamplingRuntimeParameters()
{
    return (RTXDI_PTSpatialResamplingRuntimeParameters)0;
}

uint2 CalculateNeighborSurfacePixelPosition(RTXDI_PTSpatialResamplingParameters spatialParams, RTXDI_RuntimeParameters rParams, uint2 pixelPosition, uint i, uint startIdx, uint2 viewportSize)
{
#ifdef RTXDI_SPATIAL_NEIGHBOR_SELECTION_BUFFER
    if (spatialParams.enableSpatialHeuristicMode && i < g_Const.restirPT.spatialResampling.numSpatialSamples)
    {
        uint linearPixelIndex = pixelPosition.y * viewportSize.x + pixelPosition.x;
        uint addr = (linearPixelIndex * SPATIAL_HEURISTIC_MAX_NEIGHBORS + i) * 4;
        uint packed = RTXDI_SPATIAL_NEIGHBOR_SELECTION_BUFFER.Load(addr);
        if (packed == SPATIAL_HEURISTIC_INVALID_NEIGHBOR)
            return uint2(0xFFFF, 0xFFFF);
        return uint2(packed & 0xFFFF, packed >> 16);
    }
#endif

    uint sampleIdx = (startIdx + i) & rParams.neighborOffsetMask;
    int2 spatialOffset = int2(float2(RTXDI_NEIGHBOR_OFFSETS_BUFFER[sampleIdx].xy) * spatialParams.samplingRadius);
    int2 idx = int2(pixelPosition) + spatialOffset;
    idx = RAB_ClampSamplePositionIntoView(idx, false);
    RTXDI_ActivateCheckerboardPixel(idx, false, rParams.activeCheckerboardField);
    return idx;
}

bool IsValidNeighborSurface(RTXDI_PTSpatialResamplingParameters spatialParams, RAB_Surface surface, RAB_Surface neighborSurface)
{
    if (!RAB_IsSurfaceValid(neighborSurface))
    {
        return false;
    }

    if (!RTXDI_IsValidNeighbor(RAB_GetSurfaceNormal(surface), RAB_GetSurfaceNormal(neighborSurface),
        RAB_GetSurfaceLinearDepth(surface), RAB_GetSurfaceLinearDepth(neighborSurface),
        spatialParams.normalThreshold, spatialParams.depthThreshold))
    {
        return false;
    }

    //if (spatialParams.enableMaterialSimilarityTest && !RAB_AreMaterialsSimilar(RAB_GetMaterial(centerSurface), RAB_GetMaterial(neighborSurface)))
    if (!RAB_AreMaterialsSimilar(RAB_GetMaterial(surface), RAB_GetMaterial(neighborSurface)))
    {
        return false;
    }

    return true;
}

bool ShouldApplyDisocclusionBoost(RTXDI_PTSpatialResamplingParameters spatialParams, RTXDI_PTReservoir reservoir, RTXDI_PTReservoir curSample)
{
    return (reservoir.rcVertexLength == 2 && ((!spatialParams.duplicationBasedHistoryReduction && spatialParams.maxTemporalHistory > curSample.M) || (spatialParams.duplicationBasedHistoryReduction && RTXDI_GetShouldBoostSpatialSamples(curSample))));
}

uint CalculateNumSamples(RTXDI_PTSpatialResamplingParameters spatialParams, RTXDI_PTReservoir reservoir, RTXDI_PTReservoir curSample)
{
    int numSamples = spatialParams.numSpatialSamples;

    // doing sample boost for random replay (RcVertexLength>2) is pretty expensive. Let's disable that for now.
    if (ShouldApplyDisocclusionBoost(spatialParams, reservoir, curSample))
    {
        numSamples = max(numSamples, spatialParams.numDisocclusionBoostSamples);
    }

    uint maxSamples = spatialParams.enableSpatialHeuristicMode
        ? min(RTXDI_MAX_SPATIAL_RESAMPLING_COUNT, SPATIAL_HEURISTIC_MAX_NEIGHBORS)
        : RTXDI_MAX_SPATIAL_RESAMPLING_COUNT;
    numSamples = min(numSamples, maxSamples);
    return numSamples;
}

RTXDI_PTHybridShiftRuntimeParameters BuildHSParams(const RTXDI_PTSpatialResamplingRuntimeParameters srrParams)
{
    RTXDI_PTHybridShiftRuntimeParameters hsrParams = (RTXDI_PTHybridShiftRuntimeParameters)0;
    hsrParams.isBasePathInPrevFrame = false;
    hsrParams.isPrevFrame = false;

    hsrParams.cameraPos = srrParams.cameraPos;
    hsrParams.prevCameraPos = srrParams.prevCameraPos;
    hsrParams.prevPrevCameraPos = srrParams.prevPrevCameraPos;
    return hsrParams;
}

#if RTXDI_DEBUG == 1
void SpatialRetrace(const RTXDI_PTSpatialResamplingRuntimeParameters srrParams,
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
    RAB_PathTracerUserDataSetPathType(ptud, RTXDI_PTPathTraceInvocationType_DebugSpatialRetrace);
    RTXDI_ComputeHybridShift(prevSurfCopy, prevSampleCopy, hspfParams, BuildHSParams(srrParams), rcParams, targetFunction, jacobian, ptud);
}
#endif

bool ShouldEvaluateSpatialReconnection(RTXDI_PTReservoir neighborReservoir)
{
    return !(((neighborReservoir.rcVertexLength == neighborReservoir.pathLength) && RTXDI_ConnectsToNeeLight(neighborReservoir)) || (neighborReservoir.rcVertexLength > neighborReservoir.pathLength));
}

void EvaluateReconnection(RTXDI_PTHybridShiftPerFrameParameters hspfParams, inout float3 targetFunction, RAB_Surface surface, RTXDI_PTReservoir neighborReservoir)
{
    targetFunction *= RAB_GetPTSampleTargetPdfForSurface(neighborReservoir.translatedWorldPosition, neighborReservoir.radiance, surface);

    // we don't want to skip this ReSTIR PT, otherwise, we have to do random replay in final shading
    // as well to fetch the vertex before reconnection to do a visibility test
    if (any(targetFunction > 0.f))
    {
        targetFunction *= RAB_GetConservativeVisibility(surface, neighborReservoir.translatedWorldPosition);
    }
}

bool ResampleNeighbors(const RTXDI_PTSpatialResamplingRuntimeParameters srrParams,
                       const RTXDI_PTSpatialResamplingParameters spatialParams,
                       const RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                       const RTXDI_PTReconnectionParameters rcParams,
                       const RTXDI_ReservoirBufferParameters reservoirBufferParams,
                       const RTXDI_PTBufferIndices bufferIndices,
                       RTXDI_RuntimeParameters rParams,
                       const uint startIdx,
                       inout uint selectedIndex,
                       inout RTXDI_RandomSamplerState rng,
                       const RAB_Surface surface,
                       inout RTXDI_PTReservoir targetReservoir,
                       inout float3 selectedTargetFunction,
                       const uint numSamples,
                       inout uint cachedResult,
                       inout RAB_PathTracerUserData ptud
)
{
    bool resampled = false;
    for (uint i = 0; i < numSamples; ++i)
    {
        uint2 neighborSurfacePos = CalculateNeighborSurfacePixelPosition(spatialParams, rParams, srrParams.pixelPosition, i, startIdx, srrParams.viewportSize);
        RAB_Surface neighborSurface = RAB_GetGBufferSurface(neighborSurfacePos, false);

        if (!IsValidNeighborSurface(spatialParams, surface, neighborSurface))
        {
            continue;
        }

        uint2 neighborReservoirPos = RTXDI_PixelPosToReservoirPos(neighborSurfacePos, rParams.activeCheckerboardField);
        RTXDI_PTReservoir neighborReservoir = RTXDI_LoadPTReservoir(reservoirBufferParams, neighborReservoirPos, bufferIndices.spatialResamplingInputBufferIndex);

#if RTXDI_DEBUG == 1
        SpatialRetrace(srrParams, hspfParams, rcParams, neighborSurface, neighborReservoir, ptud);
#endif

        float jacobian = RTXDI_CalculateJacobian(RAB_GetSurfaceWorldPos(surface), RAB_GetSurfaceWorldPos(neighborSurface), neighborReservoir.translatedWorldPosition, neighborReservoir.worldNormal);
        float3 targetFunction = float3(1.0f, 1.0f, 1.0f);

        RAB_Surface surfaceForResampling = surface;

        // The random replay (prefix tracing) pass taken by hybrid shift
        // this will update Surface to the vertex before reconnection
        // If the reconnection vertex is a NEE-sampled light vertex (UseRTXDILight), it will be handled inside this function
        bool shouldEvaluateReconnection = ShouldEvaluateSpatialReconnection(neighborReservoir);
        RAB_PathTracerUserDataSetPathType(ptud, RTXDI_PTPathTraceInvocationType_Spatial);
        RTXDI_ComputeHybridShift(surfaceForResampling, neighborReservoir, hspfParams, BuildHSParams(srrParams), rcParams, targetFunction, jacobian, ptud);

        if (shouldEvaluateReconnection)
        {
            EvaluateReconnection(hspfParams, targetFunction, surfaceForResampling, neighborReservoir);
        }

        neighborReservoir.weightSum *= jacobian;

        cachedResult |= (1u << uint(i));

        if (RTXDI_CombinePTReservoirs(targetReservoir, neighborReservoir, RTXDI_GetNextRandom(rng), targetFunction))
        {
            selectedTargetFunction = targetFunction;
            selectedIndex = i;
            resampled = true;
        }
    }
    return resampled;
}

void BiasCorrection(const RTXDI_PTSpatialResamplingRuntimeParameters srrParams,
                    const RTXDI_PTSpatialResamplingParameters spatialParams,
                    const RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                    const RTXDI_PTReconnectionParameters rcParams,
                    const RTXDI_ReservoirBufferParameters reservoirBufferParams,
                    const RTXDI_PTBufferIndices bufferIndices,
                    RTXDI_RuntimeParameters rParams,
                    const uint startIdx,
                    const uint selectedIndex,
                    const RAB_Surface surface,
                    inout RTXDI_PTReservoir targetReservoir,
                    const uint numSamples,
                    const uint cachedResult,
                    inout float pi,
                    inout float piSum,
                    inout RAB_PathTracerUserData ptud)
{
    for (uint i = 0; i < numSamples; ++i)
    {
        // If we skipped this neighbor above, do so again.
        if ((cachedResult & (1u << uint(i))) == 0)
            continue;

        uint2 neighborSurfacePos = CalculateNeighborSurfacePixelPosition(spatialParams, rParams, srrParams.pixelPosition, i, startIdx, srrParams.viewportSize);
        RAB_Surface neighborSurface = RAB_GetGBufferSurface(neighborSurfacePos, false);

        uint2 neighborReservoirPos = RTXDI_PixelPosToReservoirPos(neighborSurfacePos, rParams.activeCheckerboardField);
        RTXDI_PTReservoir neighborReservoir = RTXDI_LoadPTReservoir(reservoirBufferParams, neighborReservoirPos, bufferIndices.spatialResamplingInputBufferIndex);

        float3 targetFunction = float3(1.0f, 1.0f, 1.0f);

        float jacobian = RTXDI_CalculateJacobian(RAB_GetSurfaceWorldPos(neighborSurface), RAB_GetSurfaceWorldPos(surface), targetReservoir.translatedWorldPosition, targetReservoir.worldNormal);

        // The random replay (prefix tracing) pass taken by hybrid shift
        // this will update Surface to the vertex before reconnection
        // If the reconnection vertex is a NEE-sampled light vertex (UseRTXDILight), it will be handled inside this function
        float BackupPartialJacobian = targetReservoir.partialJacobian; // for MIS weight computation, we don't want Reservoir value to be eventually overwritten
        bool shouldEvaluateReconnection = ShouldEvaluateSpatialReconnection(targetReservoir);
        RAB_PathTracerUserDataSetPathType(ptud, RTXDI_PTPathTraceInvocationType_SpatialInverse);
        RTXDI_ComputeHybridShift(neighborSurface, targetReservoir, hspfParams, BuildHSParams(srrParams), rcParams, targetFunction, jacobian, ptud);
        targetReservoir.partialJacobian = BackupPartialJacobian;

        if (shouldEvaluateReconnection)
        {
            EvaluateReconnection(hspfParams, targetFunction, neighborSurface, targetReservoir);
        }

        float Ps = RTXDI_Luminance(targetFunction) * jacobian;

        // Select this sample for the (normalization) numerator if this particular neighbor pixel
        // was the one we selected via RIS in the first loop, above.
        pi = (selectedIndex == i) ? Ps : pi;

        // Add to the sums of weights for the (normalization) denominator
        piSum += Ps * neighborReservoir.M;
    }
}

void FinalizeResampling(inout RTXDI_PTReservoir targetReservoir, float3 selectedTargetFunction, float pi, float piSum)
{
    // "MIS-like" normalization
    // {wSum * (pi/piSum)} * 1/selectedTargetPdf
    const float normalizationNumerator = pi;
    const float normalizationDenominator = RTXDI_Luminance(selectedTargetFunction) * piSum;
    RTXDI_FinalizePTResampling(targetReservoir, normalizationNumerator, normalizationDenominator);
}

RTXDI_PTReservoir RTXDI_PTSpatialResampling(RTXDI_PTSpatialResamplingRuntimeParameters srrParams,
                                            RTXDI_PTSpatialResamplingParameters spatialParams,
                                            RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                                            RTXDI_PTReconnectionParameters rcParams,
                                            RTXDI_ReservoirBufferParameters reservoirBufferParams,
                                            RTXDI_PTBufferIndices bufferIndices,
                                            RTXDI_RuntimeParameters rParams,
                                            RTXDI_RandomSamplerState rng,
                                            inout bool resampled,
                                            inout RAB_PathTracerUserData ptud)
{
    RAB_Surface surface = RAB_GetGBufferSurface(srrParams.pixelPosition, false);
    if (!RAB_IsSurfaceValid(surface))
    {
        return RTXDI_EmptyPTReservoir();
    }

    RTXDI_PTReservoir targetReservoir = RTXDI_EmptyPTReservoir();
    RTXDI_PTReservoir curSample = RTXDI_LoadPTReservoir(reservoirBufferParams, srrParams.reservoirPosition, bufferIndices.spatialResamplingInputBufferIndex);
    float3 selectedTargetFunction = float3(0.0f, 0.0f, 0.0f);

    if (RTXDI_IsValidPTReservoir(curSample))
    {
        if (RTXDI_CombinePTReservoirs(targetReservoir, curSample, /* random = */ 0.5, curSample.targetFunction))
        {
            selectedTargetFunction = curSample.targetFunction;
        }
    }

    int numSamples = CalculateNumSamples(spatialParams, targetReservoir, curSample);

    const uint startIdx = RTXDI_GetNextRandom(rng) * rParams.neighborOffsetMask;
    uint selectedIndex = numSamples + 1;
    uint cachedResult = 0;

    resampled = ResampleNeighbors(srrParams, spatialParams, hspfParams, rcParams, reservoirBufferParams, bufferIndices, rParams, startIdx, selectedIndex, rng, surface, targetReservoir, selectedTargetFunction, numSamples, cachedResult, ptud);

    float pi = RTXDI_Luminance(selectedTargetFunction);
    float piSum = RTXDI_Luminance(selectedTargetFunction) * curSample.M;

    BiasCorrection(srrParams, spatialParams, hspfParams, rcParams, reservoirBufferParams, bufferIndices, rParams, startIdx, selectedIndex, surface, targetReservoir, numSamples, cachedResult, pi, piSum, ptud);

    FinalizeResampling(targetReservoir, selectedTargetFunction, pi, piSum);

    return targetReservoir;
}

#endif // RTXDI_PT_SPATIAL_RESAMPLING_HLSLI
