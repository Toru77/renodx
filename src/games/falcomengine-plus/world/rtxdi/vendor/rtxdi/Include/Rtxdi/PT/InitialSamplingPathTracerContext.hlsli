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

#ifndef RTXDI_INITIAL_SAMPLING_PATH_TRACING_CONTEXT_HLSLI
#define RTXDI_INITIAL_SAMPLING_PATH_TRACING_CONTEXT_HLSLI

#include "PathReconnectibility.hlsli"
#include "PathTracerContextParameters.hlsli"
#include "PathTracerState.hlsli"
#include "Rtxdi/Utils/Color.hlsli"

struct RTXDI_InitialSamplingPathTracerContext
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
        if(!foundRcVertex)
            ptState.rcVertexLength = newMax;
    }

    bool ShouldRunRussianRoulette()
    {
        return ptState.bounceDepth != ptState.initialBounceDepth;
    }

    void RecordRussianRouletteProbability(float rrProb)
    {
        russianRoulettePdf *= rrProb;
    }
    
    void MultiplyPathThroughput(float3 multiplicationFactor)
    {
        ptState.pathThroughput *= multiplicationFactor;
    }

    void SetContinuationRay(RayDesc continuationRay)
    {
        ptState.continuationRay = continuationRay;
    }

    bool AnalyzePathReconnectibilityBeforeTrace()
    {
        if (ptState.NoMainPathRcVertex())
        {
            bool isCurrentVertexRoughForConnection = false;
            if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FIXED_THRESHOLD)
            {
                isCurrentVertexRoughForConnection = RAB_GetSurfaceRoughness(ptState.intersectionSurface) > roughnessThreshold;
            }
            else if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
            {
                const float inverseFootprint = RTXDI_CalculateRayFootprint(RAB_GetSurfaceNormal(mainPathRcVertexSurface), RAB_GetSurfaceViewDir(ptState.intersectionSurface), ptState.lastHitT, ptState.brdfRaySample);
                isCurrentVertexRoughForConnection = inverseFootprint > footprintThreshold;
            }
            // If the conditions are meet, we set the current vertex to be the rcVertex by marking ptState.rcVertexLength = BouceDepth - 1
            // It is BouceDepth - 1, because we haven't traced the ray yet
            if(isCurrentVertexRoughForConnection && ptState.IsLastVertexRough() && ptState.IsLastVertexFar())
            {
                ptState.rcVertexLength = ptState.bounceDepth - 1;
                foundRcVertex = true;
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

        // We memorize scatterPdf because we need to check invertibility during spatial/temporal reuse (this can change which affects the inverse footprint of the shifted path)
        if (ptState.rcVertexLength == ptState.bounceDepth - 1)
        {
            mainPathRcWiPdf = ptState.brdfRaySample.outPdf;
            mainPathPartialJacobian = RTXDI_CalculatePartialJacobian(ptState.lastHitT, -RAB_GetSurfaceViewDir(ptState.intersectionSurface), RAB_GetSurfaceNormal(ptState.intersectionSurface));
        }

        // update MainPathRcVertex (main path RcVertex)
        // we also update this before finding the MainPathRcVertex because we use it to get previous vertex normal
        if (ptState.rcVertexLength >= ptState.bounceDepth - 1)
        {
            mainPathRcVertexSurface = ptState.intersectionSurface;
        }

        // Memorize BrdfPdf before RcVertex to convert path parameterization
        // This is useful even if MainPathRcVertex is not found (connecting directly to a light)
        if (ptState.rcVertexLength > ptState.bounceDepth - 1)
        {
            brdfPdfBeforeRcVertex = ptState.brdfRaySample.outPdf;
        }

        // the "suffix" path throughput only happens after rcVertexLength
        if (ptState.bounceDepth > ptState.rcVertexLength)
        {
            ptState.rcPathThroughput *= ptState.continuationRayBrdfOverPdf;
        }

        return !IsPathTerminated();
    }

    void SetTraceResult(RAB_RayPayload rp)
    {
        ptState.traceResult = rp;
    }

    void SetIntersectionSurface(const RAB_Surface intersectionSurface)
    {
        ptState.intersectionSurface = intersectionSurface;
        RAB_SetSurfaceNormal(ptState.intersectionSurface, normalize(RAB_GetSurfaceNormal(ptState.intersectionSurface)));
    }

    void UpdateReconnectionStateForPathIntersection()
    {
        // compute the V_prev->V ray footprint, if it is above threshold, we mark last vertex as "far"
        // this is used when testing the current vertex is connectible given the location on the path sample
        if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FIXED_THRESHOLD)
        {
            ptState.SetIsLastVertexFar(RAB_RayPayloadGetCommittedHitT(ptState.traceResult) > distanceThreshold && !ptState.brdfRaySample.properties.IsDelta());
        }
        else if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
        {
            const float rayFootprint = RTXDI_CalculateRayFootprint(RAB_GetSurfaceNormal(ptState.intersectionSurface), RAB_GetSurfaceViewDir(ptState.intersectionSurface), RAB_RayPayloadGetCommittedHitT(ptState.traceResult), ptState.brdfRaySample);
            ptState.SetIsLastVertexFar(rayFootprint > footprintThreshold);
        }
    }

    void RecordPathIntersection(const RAB_Surface intersectionSurface)
    {
        SetIntersectionSurface(intersectionSurface);
        UpdateReconnectionStateForPathIntersection();
    }

    bool ShouldSampleEmissiveSurfaces()
    {
        return true;
    }

    bool RecordEmissiveLightSample(float3 radianceFromEmissiveSurface, RAB_Surface prevSurface, inout RTXDI_RandomSamplerState rng)
    {
        const uint candidateRcVertexLength = (ptState.NoMainPathRcVertex() && ptState.IsLastVertexFar()) ? ptState.bounceDepth : ptState.rcVertexLength;

        float partialJacobianToLight = 0.f;
        // if the current vertex is the rcVertex, we need to compute a partial Jacobian (which is different from the main path partial Jacobian)
        if (candidateRcVertexLength == ptState.bounceDepth)
        {
            partialJacobianToLight = RTXDI_CalculatePartialJacobian(RAB_RayPayloadGetCommittedHitT(ptState.traceResult), ptState.continuationRay.Direction, RAB_GetSurfaceNormal(ptState.intersectionSurface));
        }
        
        
        // RIS stream the current path sample, which is only the emissive portion of the intersection surface.
        float3 Le = radianceFromEmissiveSurface;

        bool accepted = RISStreamPathSample(ptState.bounceDepth, Le, rng, candidateRcVertexLength, false, 1.f, ptState.brdfRaySample.outDirection, partialJacobianToLight);
        return accepted;
    }

    bool ShouldSampleNee()
    {
        return true;
    }

    bool RecordNeeLightSample(in const RTXDI_SampledLightData sampledLightData,
                              in const float3 radianceFromLights,
                              in const float neePdf,
                              in const float scatterPdf,
                              in const RAB_LightSample lightSample,
                              inout RTXDI_RandomSamplerState rng)
    {
        bool selected = false;
        ptState.sampledLightDataForDI = sampledLightData;
        ptState.lightSampleForDI = lightSample;
        if (any(radianceFromLights > 0.f) && RTXDI_SampledLightData_IsValidLightData(ptState.sampledLightDataForDI))
        {
            RTXDI_SampledLightData lightData = ptState.sampledLightDataForDI; // UV and index
            RAB_LightSample lightSample = ptState.lightSampleForDI; // Orientation/location info for the light, plus the radiance solidAnglePdf, and polymorphicLightType

            const uint pathLength = ptState.bounceDepth + 1; // Include light vertex

            // make sure bounce count match between NEE and emissive modes
            if (pathLength > GetMaxPathBounce()) return false;
            
            // whether the current vertex is connectible (Case 2)
            bool isCurrentVertexRoughForConnection = false;
            if (ptState.NoMainPathRcVertex())
            {
                if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FIXED_THRESHOLD)
                {
                    isCurrentVertexRoughForConnection = RAB_GetSurfaceRoughness(ptState.intersectionSurface) > roughnessThreshold;
                }
                else if(reconnectionMode == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
                {
                    RTXDI_BrdfRaySample brs = (RTXDI_BrdfRaySample)0;
                    brs.outPdf = scatterPdf;
                    brs.properties.SetContinuous();
                    float inverseFootprint = RTXDI_CalculateRayFootprint(RAB_GetSurfaceNormal(mainPathRcVertexSurface), RAB_GetSurfaceViewDir(ptState.intersectionSurface), RAB_RayPayloadGetCommittedHitT(ptState.traceResult), brs);
                    isCurrentVertexRoughForConnection = inverseFootprint > footprintThreshold;
                }
            }

            // Three cases
            // 1) there is a MainPathRcVertex before
            // 2) not 1) and the current vertex is connectible
            // 3) not 1) and not 2) and the NEE-sampled light vertex is connectible
            // we don't allow random replay to the NEE sample (NEE sample is always reconnected if the path is not reconnected before)
            const uint rcVertexLength = ptState.NoMainPathRcVertex() ?
                (ptState.IsLastVertexRough() && ptState.IsLastVertexFar() && isCurrentVertexRoughForConnection ? pathLength - 1 : pathLength) :
                ptState.rcVertexLength;

            // in case 3), partial Jacobian is the light sample's solid angle PDF
            // in case 2), partial Jacobian is the ordinary G term

            float pathPartialJacobian = rcVertexLength == pathLength ? RAB_LightSampleSolidAnglePdf(lightSample) : 0.f;
            if (rcVertexLength == pathLength - 1)
            {
                pathPartialJacobian = RTXDI_CalculatePartialJacobian(RAB_RayPayloadGetCommittedHitT(ptState.traceResult), ptState.continuationRay.Direction, RAB_GetSurfaceNormal(ptState.intersectionSurface));
            }
            
            float3 Le = radianceFromLights;

            // if the rcVertex is the light vertex, we packed lightData to store in the "radiance" member in the reservoir (x is indicated by INF)
            // (we can re-evaluate the radiance by using RTXDI functions)

            selected = RISStreamPathSample(pathLength, Le, rng,
                rcVertexLength, true, neePdf,
                float3((rcVertexLength == pathLength ? 1.f / 0.f : 1.f), asfloat(ptState.sampledLightDataForDI.lightData), asfloat(ptState.sampledLightDataForDI.uvData)),
                pathPartialJacobian);

            // we memorize scatterPdf because we need to check invertibility during spatial/temporal reuse (this can change which affects the inverse footprint of the shifted path)
            if (selected && rcVertexLength == pathLength - 1)
            {
                selectedRcWiPdf = scatterPdf;
            }
        }
        
        return selected;
    }

    void RecordPathRadianceMiss(inout RTXDI_RandomSamplerState rng)
    {
        ptState.SetPathTermination(true);
    }

    bool RecordEnvironmentMapLightSample(const float3 environmentMapRadiance,
                                         RAB_Surface prevSurface,
                                         inout RTXDI_RandomSamplerState rng)
    {
        const float FakeDistanceHitT = RAB_DISTANT_LIGHT_DISTANCE;
        const float3 FakeWorldPosition = RAB_GetSurfaceWorldPos(ptState.intersectionSurface) + FakeDistanceHitT * ptState.continuationRay.Direction;

        float3 Le = environmentMapRadiance;

        const uint envRcVertexLength = (ptState.NoMainPathRcVertex() && ptState.IsLastVertexRough()) ? ptState.bounceDepth : ptState.rcVertexLength;

        // if the current vertex is the rcVertex, we need to compute a partial Jacobian (which is different from the main path partial Jacobian)
        float partialJacobianToLightEnv = 0.f;
        if (envRcVertexLength == ptState.bounceDepth)
        {
            RAB_SetSurfaceWorldPos(ptState.intersectionSurface, FakeWorldPosition);
            RAB_SetSurfaceNormal(ptState.intersectionSurface, -ptState.continuationRay.Direction);
            partialJacobianToLightEnv = RTXDI_CalculatePartialJacobian(FakeDistanceHitT, ptState.continuationRay.Direction, RAB_GetSurfaceNormal(ptState.intersectionSurface));
        }

        bool envSelected = RISStreamPathSample(ptState.bounceDepth, Le, rng, envRcVertexLength, false, 1.f, ptState.continuationRay.Direction, partialJacobianToLightEnv);
        return envSelected;
    }

    void Init(RTXDI_PathTracerContextParameters ptParams, const RAB_Surface primarySurface, inout RTXDI_PathTracerRandomContext ptRandContext)
    {
        ptState.initialBounceDepth = 2;
        ptState.bounceDepth = ptState.initialBounceDepth;
        ptState.maxPathBounce = (uint16_t)ptParams.maxBounces;
        ptState.pathThroughput = float3(1.0f, 1.0f, 1.0f);
        ptState.intersectionSurface = primarySurface;

        runningWeightSum = 0.f;
        selectedPathLength = 0;
        brdfPdfBeforeRcVertex = 1.f;
        russianRoulettePdf = 1.f;
        mainPathRcVertexSurface = RAB_EmptySurface();
        selectedRcSurface = RAB_EmptySurface();

        // "suffix" throughput to compute the radiance coming out of the reconnection vertex
        ptState.rcPathThroughput = float3(1.f, 1.f, 1.f);

        // the reconnection vertex length. Initially we set it to maximum, as we find the proper vertex we reduce the length
        ptState.rcVertexLength = ptParams.maxRcVertexLength;

        foundRcVertex = false;

        reconnectionMode = ptParams.rcParams.reconnectionMode;
        RTXDI_CalculateFootprintParameters(ptParams.rcParams, ptParams.rcrParams, primarySurface, ptRandContext, footprintThreshold, pdfThreshold);
        distanceThreshold = ptParams.rcParams.distanceThreshold;
        roughnessThreshold = ptParams.rcParams.roughnessThreshold;
    }

    float CalculateRISWeight(float3 targetFunctionOverP)
    {
        float risWeight = RTXDI_Luminance(targetFunctionOverP);
        risWeight = (isnan(risWeight) || isinf(risWeight)) ? 0.f : risWeight;
        return risWeight;
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
        const float3 targetFunctionOverP = currentLightRadiance * ptState.pathThroughput;

        float risWeight = RTXDI_Luminance(targetFunctionOverP);

        risWeight = (isnan(risWeight) || isinf(risWeight)) ? 0.f : risWeight;

        runningWeightSum += risWeight;
        
        if (runningWeightSum * RTXDI_GetNextRandom(rng) < risWeight)
        {
            bool connectBeforeLight = rcVertexLength == pathLength - 1; //connect to vertex prior to the light vertex
            bool connectToLight = rcVertexLength == pathLength; //connect to the light vertex

            selectedPathLength = pathLength;
            radiance = currentLightRadiance * ptState.rcPathThroughput;

            // Russian roulette PDF and the BSDF sampling PDF before RcVertex is baked in the path throughput
            // we want to exclude it from our target function
            // In ReSTIR PT, the target function is the path contribution in the mixed PSS-solid angle measure
            // where only the bounce V_prev_prev->V_prev->rcVertex uses solid angle measure (only f)
            // all other bounces uses PSS (f/p)
            // if rcVertexLength > pathLength, there is no rcVertex on this path, so "BSDF sampling PDF before RcVertex" should be 1
            float pdfToBeExcludedFromTargetFunction = russianRoulettePdf * ((connectToLight && isNee) ? neePdf : (rcVertexLength > pathLength ? 1.f : brdfPdfBeforeRcVertex));
            selectedTargetFunction = pdfToBeExcludedFromTargetFunction * currentLightRadiance * ptState.pathThroughput; //pHat

            selectedRcVertexLength = rcVertexLength;
            selectedRcWiPdf = mainPathRcWiPdf;
            
            // for these two scenarios, the rcVertex can be different from the main path
            if ((connectToLight && !isNee) || (connectBeforeLight && isNee))
            {
                selectedRcSurface = ptState.intersectionSurface;
            }
            else
            {
                selectedRcSurface = mainPathRcVertexSurface;
            }

            selectedPartialJacobian = partialJacobian == 0.f ? mainPathPartialJacobian : partialJacobian;

            radiance = isinf(packedLightData.x) ? packedLightData : radiance; // store RTXDI sampleRef

            return true;
        }

        return false;
    }


    // Offer a chance for early termination in RAB_PathTracer.hlsli
    bool IsPathTerminated()
    {
        return ptState.IsPathTermination();
    }

    float3 GetSelectedTargetFunction()
    {
        return selectedTargetFunction;
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

    // Footprint mode reconnection thresholds
    float GetFootprintThreshold()
    {
        return footprintThreshold;
    }

    float GetPdfThreshold()
    {
        return pdfThreshold;
    }

    //
    // Hybrid-shift only functions
    //

    float GetCheckPreRcInverseGeoTerm()
    {
        return 0.0;
    }

    RAB_Surface GetRcPrevSurface()
    {
        return RAB_EmptySurface();
    }

    void SetRcVertexLength(uint length)
    {

    }

    void SetSelectedPathLength(uint length)
    {

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


    void SetContinuationRayBrdfOverPdf(float3 brdfOverPdf)
    {
        ptState.continuationRayBrdfOverPdf = brdfOverPdf;
    }

    float3 GetContinuationRayBrdfOverPdf()
    {
        return ptState.continuationRayBrdfOverPdf;
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

    // Total radiance carried from the path tree to the primary intersection divided by 
    // the pdfs of producing the path tree
    float3              radiance;

    // RC vertex surface
    RAB_Surface         mainPathRcVertexSurface;
    float               mainPathRcWiPdf;
    float               mainPathPartialJacobian;

    // The brdf pdf w.r.t solid angle prior to the rc vertex
    float               brdfPdfBeforeRcVertex;

    float               runningWeightSum;
    float3              selectedTargetFunction;
    float               selectedRcWiPdf;
    uint                selectedPathLength;
    uint                selectedRcVertexLength;
    RAB_Surface         selectedRcSurface;
    float               selectedPartialJacobian;
    float               russianRoulettePdf;
    float               footprintThreshold;
    float               pdfThreshold;

    RTXDI_PTReconnectionMode reconnectionMode;
    float               roughnessThreshold;
    float               distanceThreshold;

    bool                evaluateEnvMapOnMiss;
    uint                lightSamplingMode;

    bool                foundRcVertex;

    RTXDI_PathTracerState ptState;
};

#endif // RTXDI_INITIAL_SAMPLING_PATH_TRACING_CONTEXT_HLSLI