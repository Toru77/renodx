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

#ifndef RTXDI_PATH_RECONNECTIBILITY_HLSLI
#define RTXDI_PATH_RECONNECTIBILITY_HLSLI

#include "ReSTIRPTParameters.h"
#include "PathTracerRandomContext.hlsli"
#include "Rtxdi/Utils/BrdfRaySample.hlsli"
#include "Rtxdi/Utils/Math.hlsli"

struct RTXDI_PTReconnectionRuntimeParameters
{
    float3 cameraPos;
    float3 prevCameraPos;
    float3 prevPrevCameraPos;
    bool isPrevFrame;
};

float RTXDI_CalculatePrimaryRayFootprint(RTXDI_PTReconnectionRuntimeParameters rcrParams, RAB_Surface surfaceForResampling)
{
    // Note that the footprint threshold is a ratio of the primary ray's footprint
    // If we shift a sample to the previous frame, we need to know previous frame's primary ray's footprint

    float3 preViewTranslationOffset = (rcrParams.prevCameraPos - rcrParams.prevPrevCameraPos);
    float3 primaryHitDisp = RAB_GetSurfaceWorldPos(surfaceForResampling) - (rcrParams.isPrevFrame ? (rcrParams.prevCameraPos + preViewTranslationOffset) : rcrParams.cameraPos);

    return dot(primaryHitDisp, primaryHitDisp) * 4 * RTXDI_PI / abs(dot(RAB_GetSurfaceNormal(surfaceForResampling), RAB_GetSurfaceViewDir(surfaceForResampling)));
}

void RTXDI_CalculateFootprintParameters(
    const RTXDI_PTReconnectionParameters rcParams,
    const RTXDI_PTReconnectionRuntimeParameters rcrParams,
    const RAB_Surface surfaceForResampling,
    inout RTXDI_PathTracerRandomContext ptRandContext,
    inout float footprintThreshold,
    inout float pdfThreshold)
{
    const float localMinConnectionFootprint = RTXDI_GaussRand(rcParams.minConnectionFootprint, rcParams.minConnectionFootprintSigma, ptRandContext.replayRandomSamplerState);
    const float localMinPdfRoughness = RTXDI_GaussRand(rcParams.minPdfRoughness, rcParams.minPdfRoughnessSigma, ptRandContext.replayRandomSamplerState);
    float primaryRayFootprint = RTXDI_CalculatePrimaryRayFootprint(rcrParams, surfaceForResampling);

    footprintThreshold = primaryRayFootprint * localMinConnectionFootprint * localMinConnectionFootprint;
    pdfThreshold = 1.f / (localMinPdfRoughness * localMinPdfRoughness);
}


float RTXDI_CalculateRayFootprint(const float3 surfNormal,
                                  const float3 surfViewDir,
                                  const float rayDist, // committedRayT
                                  const RTXDI_BrdfRaySample brs) // const float pdf)
{
    if(brs.properties.IsDelta())
        return 0.0;    
    const float pdf = brs.outPdf;
    const float geometryFactor = abs(dot(surfNormal, surfViewDir)) / (rayDist * rayDist);
    const float rayFootprint = 1.f / (geometryFactor * pdf);
    return rayFootprint;
}

#endif // RTXDI_PATH_RECONNECTIBILITY_HLSLI