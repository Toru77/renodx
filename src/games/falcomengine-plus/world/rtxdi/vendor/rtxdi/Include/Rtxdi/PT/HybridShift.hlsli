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

#ifndef RTXDI_PT_HYBRID_SHIFT_HLSLI
#define RTXDI_PT_HYBRID_SHIFT_HLSLI

#include "Rtxdi/PT/HybridShiftPathTracerContext.hlsli"
#include "Rtxdi/PT/PathTracerContext.hlsli"
#include "Rtxdi/PT/PathReconnectibility.hlsli"
#include "Rtxdi/PT/Reservoir.hlsli"

struct RTXDI_PTHybridShiftRuntimeParameters
{
    bool isPrevFrame;
    bool isBasePathInPrevFrame;

    float3 cameraPos;
    float3 prevCameraPos;
    float3 prevPrevCameraPos;
};

RTXDI_PathTracerContext<RTXDI_HybridShiftPathTracerContext> InitializePathTracerContext(const RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                                                      const RTXDI_PTHybridShiftRuntimeParameters hsrParams,
                                                      const RTXDI_PTReconnectionParameters rcParams,
                                                      const RTXDI_PTReservoir neighborSample,
                                                      const RAB_Surface surfaceForResampling,
                                                      inout RTXDI_PathTracerRandomContext ptRandContext)
{
    RTXDI_PathTracerContextParameters ptParams = (RTXDI_PathTracerContextParameters) 0;
    ptParams.maxBounces = (uint16_t) hspfParams.maxBounceDepth;
    ptParams.maxRcVertexLength = (uint16_t) hspfParams.maxRcVertexLength;
    ptParams.rcVertexLength = (uint16_t) neighborSample.rcVertexLength;
    ptParams.selectedPathLength = (uint16_t) neighborSample.pathLength;
    ptParams.rcParams = rcParams;
    ptParams.rcrParams.isPrevFrame = hsrParams.isPrevFrame;
    ptParams.rcrParams.cameraPos = hsrParams.cameraPos;
    ptParams.rcrParams.prevCameraPos = hsrParams.prevCameraPos;
    ptParams.rcrParams.prevPrevCameraPos = hsrParams.prevPrevCameraPos;

    RTXDI_PathTracerContext<RTXDI_HybridShiftPathTracerContext> ctx = RTXDI_InitializePathTracerContext<RTXDI_HybridShiftPathTracerContext>(ptParams, surfaceForResampling, ptRandContext);

    return ctx;
}

bool NeedToRunRandomReplayPathTracer(const RTXDI_PTReservoir neighborSample)
{
    return neighborSample.rcVertexLength > 2;
}

void RandomReplay(inout RTXDI_PathTracerContext<RTXDI_HybridShiftPathTracerContext> ctx,
                  const RTXDI_PTReservoir neighborSample,
                  inout RAB_Surface surfaceForResampling,
                  inout RTXDI_PathTracerRandomContext ptRandContext,
                  inout float3 targetFunction,
                  inout RAB_PathTracerUserData ptud)
{
    // set the RcVertexLength and the path length here
    // such that we know to which bounce we need to replay our path to
    ctx.SetRcVertexLength(neighborSample.rcVertexLength);
    ctx.SetSelectedPathLength(neighborSample.pathLength);

    RAB_PathTrace(ctx, ptRandContext, ptud);

    // surfaceForResampling is now the surface of the vertex before rcVertex
    surfaceForResampling = ctx.GetRcPrevSurface();

    // This is now the "prefix" throughput
    targetFunction = ctx.GetRadiance();
}

void CalculatePartialJacobian(const float3 receiverPos,
                              const float3 samplePos,
                              const float3 sampleNormal,
                              inout float distanceToSurfaceSqr,
                              inout float cosineEmissionAngle)
{
    const float3 vec = receiverPos - samplePos;

    distanceToSurfaceSqr = dot(vec, vec);
    cosineEmissionAngle = saturate(dot(sampleNormal, vec * rsqrt(distanceToSurfaceSqr)));
}

float CalculateJacobianWithCachedJacobian(const float3 receiverPos,
                                          inout RTXDI_PTReservoir neighborReservoir)
{
    const float originalInversePartialJacobian = neighborReservoir.partialJacobian;
    float newDistanceSqr = 0.0f;
    float newCosine = 0.0f;
    CalculatePartialJacobian(receiverPos, neighborReservoir.translatedWorldPosition, neighborReservoir.worldNormal, newDistanceSqr, newCosine);

    const float newPartialJacobian = newCosine / newDistanceSqr;
    float jacobian = newPartialJacobian * originalInversePartialJacobian;

    neighborReservoir.partialJacobian = 1.f / newPartialJacobian;

    if (isinf(jacobian) || isnan(jacobian))
    {
        jacobian = 0;
    }

    return jacobian;
}

bool ConnectsToNEELight(RTXDI_PTReservoir neighborSample)
{
    // neighborSample.radiance.x == INF is used as an indicator for NEE-sample lights
    return (neighborSample.rcVertexLength == neighborSample.pathLength) &&
            isinf(neighborSample.radiance.x);
}

bool ShouldComputeReconnectionJacobian(const RTXDI_PTReservoir neighborSample,
                                       const bool connectToRtxdiLight)
{
    // We only need to execute the reconnection code below if the rcVertex exists
    // Exception is that if the rcVertex is a NEE-sampled light vertex, it requires some different code
    // and we handle it in another code block below

    // IE only need to compute reconnection jacobian under conditions explained above and implemented here
    return neighborSample.rcVertexLength <= neighborSample.pathLength && !connectToRtxdiLight;
}

float ComputeReconnectionJacobian(const RTXDI_PathTracerContext<RTXDI_HybridShiftPathTracerContext> ctx,
                                  inout RTXDI_PTReservoir neighborSample,
                                  const RAB_Surface surfaceForResampling,
                                  inout float jacobian)
{
    // note that we don't need the RcPrevSurface of the base path, as the partial Jacobian (geometry term) is precomputed in neighborSample.partialJacobian
    if (neighborSample.rcVertexLength <= neighborSample.pathLength)
    {
        jacobian = CalculateJacobianWithCachedJacobian(RAB_GetSurfaceWorldPos(surfaceForResampling), neighborSample);
    }

    return jacobian;
}

void ValidateInvertibilityCondition(const RTXDI_PathTracerContext<RTXDI_HybridShiftPathTracerContext> ctx,
                                    const RTXDI_PTReservoir neighborSample,
                                    const RAB_Surface surfaceForResampling,
                                    inout float jacobian)
{
    // Test invertibility. We need to make sure that the RcVertex is connectible in the shifted path
    float3 reconnectionDir = neighborSample.translatedWorldPosition - RAB_GetSurfaceWorldPos(surfaceForResampling);
    float reconnectionDist = length(reconnectionDir);
    reconnectionDir /= reconnectionDist;

    if (ctx.GetReconnectionMode() == RTXDI_RESTIRPT_RECONNECTION_MODE_FIXED_THRESHOLD)
    {
        if (neighborSample.rcVertexLength == neighborSample.pathLength)
        {
            // Connects to an RTXDI light. Always reconnectible.
            if (isinf(neighborSample.radiance.x))
            {
                return;
            }
            // Connects to an emissive surface. Only reconnectible if it
            // passes the roughness and distance thresholds
            else if (reconnectionDist < ctx.GetDistanceThreshold() || RAB_GetSurfaceRoughness(surfaceForResampling) < ctx.GetRoughnessThreshold())
            {
                jacobian = 0.0f;
            }
        }
        // Mid-path reconnection.
        // If the previous vertex is not far or rough enough, then the current vertex is not reconnectible.
        else if (neighborSample.rcVertexLength < neighborSample.pathLength)
        {
            // If the previous vertex is neither far nor rough enough, this reconnection is not valid.
            if (reconnectionDist < ctx.GetDistanceThreshold() || RAB_GetSurfaceRoughness(surfaceForResampling) < ctx.GetRoughnessThreshold())
            {
                jacobian = 0.0f;
            }
        }
    }
    else if (ctx.GetReconnectionMode() == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
    {
        float scatterPdf = 0.f;
        if (neighborSample.rcVertexLength <= neighborSample.pathLength)
        {
            scatterPdf = RAB_SurfaceEvaluateBrdfPdf(surfaceForResampling, reconnectionDir);
        }

        float geometryFactor = abs(dot(neighborSample.worldNormal, reconnectionDir)) / (reconnectionDist * reconnectionDist);
        float rayfootprint = 1.f / (geometryFactor * scatterPdf);

        if (rayfootprint <= ctx.GetFootprintThreshold())
        {
            jacobian = 0.f;
        }

        // This is the missing check of whether RcPrevVertex is connectible (it must not be connectible for the shift to be invertible)
        // i.e. check the inverse footprint for the case 2 in RandomReplayTracing
        if (ctx.GetCheckPreRcInverseGeoTerm() > 0.f && ctx.GetCheckPreRcInverseGeoTerm() / scatterPdf > ctx.GetFootprintThreshold())
        {
            jacobian = 0.f;
        }

        // This checks the whether RcPrevVertex is "rough" enough such that rcVertex is connectible
        // Note that if rcVertex a light vertex sampled by bsdf, we don't need to check this as the we relax the connectibility condition in this case
        // to encourage connection to light vertices
        // (WARNING: if we support delta material, the scatterPdf it returns must be set to a large float)
        if (scatterPdf > ctx.GetPdfThreshold() && neighborSample.rcVertexLength < neighborSample.pathLength)
        {
            jacobian = 0.f;
        }

        // This checks whether the inverse footprint of the ray RcVertex -> RcPrevVertex is above the threshold
        // if it is not, then the shift is non-invertible
        if (neighborSample.rcVertexLength < neighborSample.pathLength)
        {
            // The BSDF sampling PDF of RcVertex->RcNextVertex will actually change due to reconnecting from a different direction
            // However, we assume they are the same to avoid the expensive BSDF evaluation at the rcVertex (we don't store the material reference anyway)
            // this is likely to be a reasonable approximation, consider that the rcVertex is "far enough to be rough" when we set it up in initial sampling
            float rcScatterPdf = neighborSample.rcWiPdf;
            float inverseGeometryFactor = abs(dot(RAB_GetSurfaceNormal(surfaceForResampling), reconnectionDir)) / (reconnectionDist * reconnectionDist);
            float inversefootprint = 1.f / (inverseGeometryFactor * rcScatterPdf);
            if (inversefootprint <= ctx.GetFootprintThreshold())
                jacobian = 0.f;
        }
    }
}

void ValidateLightIDAndReservoir(const RTXDI_PTHybridShiftRuntimeParameters hsrParams,
                                 inout RTXDI_PTReservoir neighborSample,
                                 inout RTXDI_SampledLightData sampledLightData,
                                 inout float3 targetFunction)
{
    if (RTXDI_SampledLightData_IsValidLightData(sampledLightData))
    {
        int mappedLightID = -1;

        bool remapped = false;

        if (hsrParams.isBasePathInPrevFrame)
        {
            mappedLightID = RAB_TranslateLightIndex(RTXDI_SampledLightData_GetLightIndex(sampledLightData), false);
            neighborSample.radiance.y = asfloat(RTXDI_LightIndexToLightData(mappedLightID));
            remapped = true;
        }

        if (remapped)
        {
            // invalid index
            if (mappedLightID == -1)
            {
                targetFunction *= 0.f;
                sampledLightData = RTXDI_SampledLightData_CreateInvalidData();
            }
            else
            {
                RTXDI_SampledLightData_SetLightData(sampledLightData, mappedLightID);
            }
        }
    }
}

// A helper used for pairwise MIS computations.  This might be able to simplify code elsewhere, too.
float3 RTXDI_ComputeTargetFunctionWithLightPdf(const RTXDI_PTHybridShiftRuntimeParameters hsrParams,
                                               const RTXDI_SampledLightData sampledLightData,
                                               const RAB_Surface surface,
                                               inout float lightPdf,
                                               inout float3 lightPos,
                                               inout RAB_LightSample lightSample)
{
    RAB_LightInfo lightInfo = RAB_LoadLightInfo(RTXDI_SampledLightData_GetLightIndex(sampledLightData), hsrParams.isPrevFrame);
    lightSample = RAB_SamplePolymorphicLight(lightInfo, surface, RTXDI_SampledLightData_GetUVDataFloat2(sampledLightData));

    const bool isVisible = RAB_GetConservativeVisibility(surface, lightSample);

    float3 targetFunction = isVisible ? RAB_GetReflectedBsdfRadianceForSurface(RAB_LightSamplePosition(lightSample), RAB_LightSampleRadiance(lightSample), surface) : float3(0.0f, 0.0f, 0.0f);

    float3 surfaceToLightVector = RAB_LightSamplePosition(lightSample) - RAB_GetSurfaceWorldPos(surface);
    float distanceToLight = length(surfaceToLightVector);
    float3 lightDir = surfaceToLightVector / distanceToLight;

    lightPdf = RAB_LightSampleSolidAnglePdf(lightSample); // pdf is inverse G
    lightPos = RAB_LightSamplePosition(lightSample);

    return targetFunction / RAB_LightSampleSolidAnglePdf(lightSample);
}

void UpdateReconnectionForRTXDIConnectedLight(const RTXDI_PTHybridShiftRuntimeParameters hsrParams,
                                              const RTXDI_PathTracerContext<RTXDI_HybridShiftPathTracerContext> ctx,
                                              const RAB_Surface surfaceForResampling,
                                              inout float jacobian,
                                              inout float3 targetFunction,
                                              inout RTXDI_PTReservoir neighborSample,
                                              inout RAB_PathTracerUserData ptud)
{
    // fetch sampledLightData packed in Radiance
    RTXDI_SampledLightData sampledLightData = RTXDI_GetSampledLightData(neighborSample);
	
    ValidateLightIDAndReservoir(hsrParams, neighborSample, sampledLightData, targetFunction);

    // Compute target function and Jacobian using some RTXDI functions
    // Since we are using solid angle measure, we need solid angle light sampling PDF (which contains a geomtery term)
    // to compute Jacobian of reconnection
    float invPartialJacobian = 1.f;
    float3 lightPos = float3(0.0f, 0.0f, 0.0f);
    RAB_LightSample lightSample = RAB_EmptyLightSample();
    float3 lightReflectedRadiance = RTXDI_ComputeTargetFunctionWithLightPdf(hsrParams, sampledLightData, surfaceForResampling, invPartialJacobian, lightPos, lightSample);
    
    const float originalInvPartialJacobian = neighborSample.partialJacobian; // yes, this stores the src sample light pdf
    targetFunction *= lightReflectedRadiance;
    jacobian = invPartialJacobian == 0.f ? 0.f : originalInvPartialJacobian / invPartialJacobian;
    neighborSample.partialJacobian = invPartialJacobian; // override this reservoir member with the shifted path's partial jacobian

    const float3 lightDirection = normalize(lightPos - RAB_GetSurfaceWorldPos(surfaceForResampling));
    
    RAB_LastBounceDenoiserCallback(lightPos, surfaceForResampling, ptud);

    float scatterPdf = 0.f;
    if (ctx.GetReconnectionMode() == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
        scatterPdf = RAB_SurfaceEvaluateBrdfPdf(surfaceForResampling, lightDirection);
        
    // invertibility check 
    // an NEE-sampled light vertex is by definition always connectible
    // We only need to ensure that the vertex before rcVertex is not connectible
    // basically we just need to complete the missing check of inverse footprint for case 2 in RandomReplayTracing
    if (ctx.GetReconnectionMode() == RTXDI_RESTIRPT_RECONNECTION_MODE_FOOTPRINT)
    {
        if (ctx.GetCheckPreRcInverseGeoTerm() > 0.f && ctx.GetCheckPreRcInverseGeoTerm() / scatterPdf > ctx.GetFootprintThreshold())
        {
            jacobian = 0.f;
        }
    }
    
    targetFunction *= RAB_GetMISWeightForNEE(RTXDI_SampledLightData_GetLightIndex(sampledLightData), lightSample, lightDirection, invPartialJacobian, scatterPdf);
}

void RTXDI_ComputeHybridShift(inout RAB_Surface surfaceForResampling,
                              inout RTXDI_PTReservoir neighborSample,
                              RTXDI_PTHybridShiftPerFrameParameters hspfParams,
                              RTXDI_PTHybridShiftRuntimeParameters hsrParams,
                              RTXDI_PTReconnectionParameters rcParams,
                              inout float3 targetFunction,
                              inout float jacobian,
                              inout RAB_PathTracerUserData ptud)
{
    jacobian = 1.f;
    targetFunction = float3(1.0f, 1.0f, 1.0f);

    RTXDI_PathTracerRandomContext ptrRandContext = (RTXDI_PathTracerRandomContext) 0;
    ptrRandContext.replayRandomSamplerState = RTXDI_CreateRandomSamplerFromDirectSeed(neighborSample.randomSeed, neighborSample.randomIndex);

    RTXDI_PathTracerContext<RTXDI_HybridShiftPathTracerContext> ctx = InitializePathTracerContext(hspfParams, hsrParams, rcParams, neighborSample, surfaceForResampling, ptrRandContext);

    bool needToRunRandomReplayPathTracer = NeedToRunRandomReplayPathTracer(neighborSample);

#if RTXDI_PATHTRACER_USE_REORDERING
    // this helps performance a lot. Basically all threads that don't need to do random replay will be grouped together
    NvReorderThread((needToRunRandomReplayPathTracer ? 1 : 0), 1);
#endif

    if (needToRunRandomReplayPathTracer)
    {
        RandomReplay(ctx, neighborSample, surfaceForResampling, ptrRandContext, targetFunction, ptud);
    }

    const bool connectToRtxdiLight = ConnectsToNEELight(neighborSample);
    if (ShouldComputeReconnectionJacobian(neighborSample, connectToRtxdiLight))
    {
        jacobian = ComputeReconnectionJacobian(ctx, neighborSample, surfaceForResampling, jacobian);
        ValidateInvertibilityCondition(ctx, neighborSample, surfaceForResampling, jacobian);

        RAB_ReconnectionDenoiserCallback(neighborSample, surfaceForResampling, ptud);
    }

    // Handle the nee-sampled light vertex reconnection here
    if (connectToRtxdiLight)
    {
        UpdateReconnectionForRTXDIConnectedLight(hsrParams, ctx, surfaceForResampling, jacobian, targetFunction, neighborSample, ptud);
    }
}

#endif // RTXDI_PT_HYBRID_SHIFT_HLSLI