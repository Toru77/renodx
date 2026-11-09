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

#ifndef RTXDI_HYBRID_SHIFT_PATH_TRACING_CONTEXT_HLSLI
#define RTXDI_HYBRID_SHIFT_PATH_TRACING_CONTEXT_HLSLI

#include "PathReconnectibility.hlsli"
#include "PathTracerContextParameters.hlsli"
#include "PathTracerRandomContext.hlsli"
#include "PathTracerState.hlsli"

struct RTXDI_HybridShiftPathTracerContext
{
    void SetBrdfRaySample(RTXDI_BrdfRaySample raySample)
    {
        ptState.brdfRaySample = raySample;
        SetContinuationRayBrdfOverPdf(raySample.brdfTimesNoL / raySample.outPdf);
    }

    void SetMaxPathBounce(uint16_t newMax)
    {
        ptState.maxPathBounce = newMax;
    }

    void SetMaxRcVertexLengthIfUnset(uint16_t newMax)
    {
        // Not applicable to hybrid shift
    }

    bool ShouldRunRussianRoulette()
    {
        return false;
    }

    void RecordRussianRouletteProbability(float rrProb)
    {
    }

    void MultiplyPathThroughput(float3 multiplicationFactor)
    {
        ptState.pathThroughput *= multiplicationFactor;
    }

    void SetContinuationRay(RayDesc continuationRay)
    {
        ptState.continuationRay = continuationRay;
    }

    // Record data before tracing the continuation ray
    bool AnalyzePathReconnectibilityBeforeTrace()
    {
        if (ptState.IsLastVertexRough() && ptState.IsLastVertexFar())
        {
            bool isCurrentVertexRoughForConnection = false;
            if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FIXED_THRESHOLD)
            {
                isCurrentVertexRoughForConnection = RAB_GetSurfaceRoughness(ptState.intersectionSurface) > roughnessThreshold;
            }
            else if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
            {
                float inverseFootprint = RTXDI_CalculateRayFootprint(ptState.prevNormal, RAB_GetSurfaceViewDir(ptState.intersectionSurface), ptState.lastHitT, ptState.brdfRaySample);
                isCurrentVertexRoughForConnection = inverseFootprint > footprintThreshold;
            }

            if (isCurrentVertexRoughForConnection)
            {
                radiance = 0.f; // invalid shift
                ptState.SetPathTermination(true);
                return !IsPathTerminated();
            }
        }

        // mark the current vertex to be rough for next vertex's reconnection condition test
        // the current vertex is "last vertex" in the next vertex's eyes
        if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FIXED_THRESHOLD)
        {
            ptState.SetIsLastVertexRough(RAB_GetSurfaceRoughness(ptState.intersectionSurface) > roughnessThreshold);
        }
        else if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
        {
            ptState.SetIsLastVertexRough(ptState.brdfRaySample.outPdf <= pdfThreshold && !ptState.brdfRaySample.properties.IsDelta());
        }

        ptState.prevNormal = RAB_GetSurfaceNormal(ptState.intersectionSurface);

        return !IsPathTerminated();
    }

    void SetTraceResult(RAB_RayPayload rp)
    {
        ptState.traceResult = rp;
    }

    void RecordPathIntersection(const RAB_Surface intersectionSurface)
    {
        ptState.intersectionSurface = intersectionSurface;
        RAB_SetSurfaceNormal(ptState.intersectionSurface, normalize(RAB_GetSurfaceNormal(ptState.intersectionSurface)));

        if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FIXED_THRESHOLD)
        {
            ptState.SetIsLastVertexFar(RAB_RayPayloadGetCommittedHitT(ptState.traceResult) > distanceThreshold && !ptState.brdfRaySample.properties.IsDelta());
        }
        else if (reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
        {
            // compute the V_prev->V ray footprint, if it is above threshold, we mark last vertex as "far"
            // this is used when testing the current vertex is connectible
            const float rayFootprint = RTXDI_CalculateRayFootprint(RAB_GetSurfaceNormal(ptState.intersectionSurface), RAB_GetSurfaceViewDir(ptState.intersectionSurface), RAB_RayPayloadGetCommittedHitT(ptState.traceResult), ptState.brdfRaySample);
            ptState.SetIsLastVertexFar(rayFootprint > footprintThreshold);
        }

        // Check invertibility conditions since we have arrived at the last vertex in random replay
        // also populate some variables since we will about to terminate
        if (ptState.bounceDepth == ptState.maxPathBounce)
        {
            // there are two cases

            // 1. RcVertexLength <= PathLength: there is a rcVertex at RcVertexLength. We must check if our current vertex at RcVertexLength - 1
            // is non-connectible. If it is connectible, then the shift is non-invertible. Remember that the connectibility condition in this case
            // is that the previous vertex is "far" and "rough", and the current vertex is "far enough to be rough"
            // The current vertex is only possible to be connectible if the previous vertex is both "far" and "rough"
            // This means that we only need to check whether the inverse footprint is larger than the threshold ("far enough to be rough")
            // But we can't compute the inverse footprint yet, since it requires evaluting the BSDF sampling PDF of V_prev->V->V_next bounce
            // This requires computing the direction to the rcVertex, but we don't pass rcVertex as an argument for this function
            // Therefore, we store the partial terms of the inverse footprint and compute with full terms outside the random replay function

            // 2. RcVertexLength > PathLength: there is no rcVertex, this means that the last vertex of the base path is a light vertex
            // However, this light vertex is not connectible. Remember that the connectibility condition in this case is that the previous vertex is "far"
            // (the ray footprint V_prev->V is larger than the threshold)
            // Therefore, the base path doesn't satisfy this condition. So the offset path also shouldn't satisfy this condition. Otherwise, the light vertex
            // will be the rcVertex and the shift is not invertible

            if ((ptState.IsLastVertexRough() || (replayRcVertexLength > selectedPathLength)) && ptState.IsLastVertexFar())
            {
                // case 1
                // in this case, we need to check the inverse footprint using the scatterDir to rcVertex to determine invertbiiltiy
                if (replayRcVertexLength <= selectedPathLength)
                {
                    if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FIXED_THRESHOLD)
                    {
                        if(ptState.IsLastVertexFar() && ptState.IsLastVertexRough() && RAB_GetSurfaceRoughness(intersectionSurface) > roughnessThreshold)
                        {
                            radiance = float3(0.0f, 0.0f, 0.0f);
                            ptState.SetPathTermination(true);
                            return;
                        }
                    }
                    else if (reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
                    {
                        checkPreRcInverseGeoTerm = RAB_RayPayloadGetCommittedHitT(ptState.traceResult) * RAB_RayPayloadGetCommittedHitT(ptState.traceResult) / abs(dot(ptState.prevNormal, ptState.continuationRay.Direction));
                    }
                }
                else // case 2. This is non-invertible
                {
                    radiance = float3(0.0f, 0.0f, 0.0f);
                    ptState.SetPathTermination(true);
                    return;
                }
            }

            // If the path could be invertible, we store IntersectionSurface of the current vertex, which is the vertex before the rcVertex
            // these values are used for the reconnection operation, and evaluating the path contribution
            if (replayRcVertexLength <= selectedPathLength)
            {
                rcPrevSurface = ptState.intersectionSurface;
                radiance = ptState.pathThroughput; // partial path throughput

                ptState.SetPathTermination(true);
                return;
            }
        }
    }

    bool ShouldSampleEmissiveSurfaces()
    {
        // See RecordPathIntersection and RecordEmissiveLightSample
        return ptState.bounceDepth == ptState.maxPathBounce;
    }

    bool RecordEmissiveLightSample(float3 radianceFromEmissiveSurface, RAB_Surface prevSurface, inout RTXDI_RandomSamplerState rng)
    {
        // Check invertibility conditions since we have arrived at the last vertex in random replay
        // also populate some variables since we will about to terminate
        if (ptState.bounceDepth == ptState.maxPathBounce)
        {
            // This is the emissive radiance coming from the intersection the ray hits and the current surface where the ray is shot from
            float3 Le = radianceFromEmissiveSurface;
            Le = (ptState.bounceDepth == selectedPathLength) ? Le : 0.f;

            radiance += Le * ptState.pathThroughput;
        }
        return true;
    }

    bool ShouldSampleNee()
    {
        return false;
    }

    bool RecordNeeLightSample(in const RTXDI_SampledLightData sampledLightData,
                              in const float3 radianceFromLights,
                              in const float neePdf,
                              in const float scatterPdf,
                              in const RAB_LightSample lightSample,
                              inout RTXDI_RandomSamplerState rng)
    {
        return false;
    }

    void RecordPathRadianceMiss(inout RTXDI_RandomSamplerState rng)
    {
        ptState.SetPathTermination(true);
    }

    bool RecordEnvironmentMapLightSample(const float3 environmentMapRadiance,
                                         RAB_Surface prevSurface,
                                         inout RTXDI_RandomSamplerState rng)
    {
        const bool bSecondaryBounce = ptState.IsSecondaryBounce();
        if (ptState.bounceDepth == selectedPathLength && !ptState.IsLastVertexRough() && !bSecondaryBounce)
        {
            float3 Le = environmentMapRadiance;
            radiance += Le * ptState.pathThroughput;
        }
        return true;
    }

    bool IsPathTerminated()
    {
        return ptState.IsPathTermination();
    }

    void Init(const RTXDI_PathTracerContextParameters ptParams, const RAB_Surface primarySurface, inout RTXDI_PathTracerRandomContext ptRandContext)
    {
        ptState.initialBounceDepth = 2;
        ptState.bounceDepth = ptState.initialBounceDepth;
        ptState.maxPathBounce = (uint16_t)min(ptParams.rcVertexLength - 1, ptParams.selectedPathLength);
        ptState.prevNormal = float3(0.0f, 0.0f, 0.0f);
        ptState.pathThroughput = float3(1.0f, 1.0f, 1.0f);
        ptState.intersectionSurface = primarySurface;

        reconnectionMode = ptParams.rcParams.reconnectionMode;
        RTXDI_CalculateFootprintParameters(ptParams.rcParams, ptParams.rcrParams, primarySurface, ptRandContext, footprintThreshold, pdfThreshold);
        distanceThreshold = ptParams.rcParams.distanceThreshold;
        roughnessThreshold = ptParams.rcParams.roughnessThreshold;

        radiance = float3(0.0f, 0.0f, 0.0f);
        rcPrevSurface = RAB_EmptySurface();
    }

    //
    // Hybrid-shift specific functions
    //

    float GetCheckPreRcInverseGeoTerm()
    {
        return checkPreRcInverseGeoTerm;
    }

    //
    // Unused interface functions (i.e. InitialSampling version functions)
    //

    float CalculateRISWeight(float3 targetFunctionOverP)
    {
        return 0.0;
    }

    bool RISStreamPathSample(uint pathLength,
                             float3 currentLightRadiance,
                             inout RTXDI_RandomSamplerState rng,
                             uint rcVertexLength,
                             bool isNee,
                             float neePdf,
                             float3 packedLightData,
                             float partialJacobian)
    {
        return false;
    }

    //
    // Reconnection Parameters
    //

    RTXDI_PTReconnectionMode GetReconnectionMode()
    {
        return reconnectionMode;
    }

    // Fixed cutoff mode reconnection thrsholds
    float GetRoughnessThreshold()
    {
        return roughnessThreshold;
    }

    float GetDistanceThreshold()
    {
        return distanceThreshold;
    }

    // Footprint reconnection threshold
    float GetPdfThreshold()
    {
        return pdfThreshold;
    }

    float GetFootprintThreshold()
    {
        return footprintThreshold;
    }

    //
    // Hybrid-Shift only functions
    //

    RAB_Surface GetRcPrevSurface()
    {
        return rcPrevSurface;
    }

    void SetRcVertexLength(uint length)
    {
        replayRcVertexLength = length;
    }

    void SetSelectedPathLength(uint length)
    {
        selectedPathLength = length;
    }

    float3 GetSelectedTargetFunction()
    {
        return float3(0.0, 0.0, 0.0);
    }

    float3 GetRadiance()
    {
        return radiance;
    }

    //
    //
    // PTState functions
    //
    //

    //
    // Accessors for internal state
    //

    uint GetBounceDepth()
    {
        return ptState.bounceDepth;
    }

    uint GetMaxPathBounce()
    {
        return ptState.maxPathBounce;
    }

    RTXDI_BrdfRaySample GetBrdfRaySample()
    {
        return ptState.brdfRaySample;
    }

    RAB_Surface GetIntersectionSurface()
    {
        return ptState.intersectionSurface;
    }

    float3 GetContinuationRayBrdfOverPdf()
    {
        return ptState.continuationRayBrdfOverPdf;
    }

    void SetContinuationRayBrdfOverPdf(float3 brdfOverPdf)
    {
        ptState.continuationRayBrdfOverPdf = brdfOverPdf;
    }

    RayDesc GetContinuationRay()
    {
        return ptState.continuationRay;
    }

    float3 GetPathThroughput()
    {
        return ptState.pathThroughput;
    }

    RAB_RayPayload GetTraceResult()
    {
        return ptState.traceResult;
    }

    void IncreaseBounceDepth()
    {
        ptState.bounceDepth++;
    }

    bool ValidContinuationRayBrdfOverPdf()
    {
        float lum = RTXDI_Luminance(ptState.continuationRayBrdfOverPdf);
        return lum > 0 && !isinf(lum) && !isnan(lum);
    }

    uint16_t GetRcVertexLength()
    {
        return ptState.rcVertexLength;
    }

    //
    // Original Functions
    //

    bool IsSecondaryBounce()
    {
        return ptState.IsSecondaryBounce();
    }

    // Reset partial state for the next path
    // Merged implementation from FReSTIRPTPathTracerStateBase
    void BeginPathState()
    {
        ptState.BeginPathState();
    }

    // Hybrid shift functions

    void SetPathTermination(bool value)
    {
        ptState.SetPathTermination(value);
    }

    bool IsPathTermination()
    {
        return ptState.IsPathTermination();
    }

    // PT Base Functions

    void SetIsLastVertexFar(bool value)
    {
        ptState.SetIsLastVertexFar(value);
    }

    bool IsLastVertexFar()
    {
        return ptState.IsLastVertexFar();
    }

    void SetIsLastVertexRough(bool value)
    {
        ptState.SetIsLastVertexRough(value);
    }

    bool IsLastVertexRough()
    {
        return ptState.IsLastVertexRough();
    }

    // ReSTIR PT state from FReSTIRPTPathTracerState
    bool NoMainPathRcVertex()
    {
        return ptState.NoMainPathRcVertex();
    }

    // Totally radiance carried from the path tree to the primary intersection divided by
    // the pdfs of producing the path tree
    float3 radiance;

    uint selectedPathLength;

    uint replayRcVertexLength;

    // Secondary intersection
    RAB_Surface rcPrevSurface;

    float footprintThreshold;
    float pdfThreshold;

    float checkPreRcInverseGeoTerm;

    //
    // Initial Sampling only state
    //

    RAB_Surface selectedRcSurface;
    uint selectedRcVertexLength;
    float selectedRcWiPdf;
    float3 selectedRcWiTargetFunction;
    float runningWeightSum;
    float selectedPartialJacobian;

    RTXDI_PTReconnectionMode reconnectionMode;
    float distanceThreshold;
    float roughnessThreshold;

    RTXDI_PathTracerState ptState;
};

#endif // RTXDI_HYBRID_SHIFT_PATH_TRACING_CONTEXT_HLSLI