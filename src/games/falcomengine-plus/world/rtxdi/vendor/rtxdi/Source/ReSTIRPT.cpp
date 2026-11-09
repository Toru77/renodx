/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#include "Rtxdi/PT/ReSTIRPT.h"

namespace rtxdi
{

RTXDI_PTBufferIndices GetDefaultReSTIRPTBufferIndices()
{
    RTXDI_PTBufferIndices bufferIndices = {};
    bufferIndices.initialPathTracerOutputBufferIndex = 0;
    bufferIndices.temporalResamplingInputBufferIndex = 0;
    bufferIndices.temporalResamplingOutputBufferIndex = 0;
    bufferIndices.spatialResamplingInputBufferIndex = 0;
    bufferIndices.spatialResamplingOutputBufferIndex = 0;
    return bufferIndices;
}

RTXDI_PTInitialSamplingParameters GetDefaultReSTIRPTInitialSamplingParams()
{
    RTXDI_PTInitialSamplingParameters params = {};
    params.numInitialSamples = 1;
    params.maxBounceDepth = 3;
    params.maxRcVertexLength = 5;
    return params;
}

RTXDI_PTDecorrelationParameters GetDefaultReSTIRPTDecorrelationParams(bool dlssRREnabled)
{
    RTXDI_PTDecorrelationParameters params = {};
    params.decorrelationFactor = 0.4f;
    params.decorrelationStagnancyExponent = 0.5f;
    params.decorrelationEmaFactor = 0.2f;
    params.fireflyReplacementFilterStrength = 0.7f;
    params.fireflyReplacementBiasReduction = 1;
    params.fireflyReplacementMultiplyBound = 15.0f;

    params.decorrelationMode = dlssRREnabled ? RTXDI_PTDecorrelationMode::Stagnancy : RTXDI_PTDecorrelationMode::None;
    params.fireflyReplacementFilterEnable = dlssRREnabled ? 1 : 0;
    return params;
}

RTXDI_PTReconnectionParameters GetDefaultReSTIRPTReconnectionParameters()
{
    RTXDI_PTReconnectionParameters reconnectionParams = {};
    reconnectionParams.minConnectionFootprint = 0.02f;
    reconnectionParams.minConnectionFootprintSigma = 0.2f;
    reconnectionParams.minPdfRoughness = 0.1f;
    reconnectionParams.minPdfRoughnessSigma = 0.01f;
    reconnectionParams.roughnessThreshold = 0.1f;
    reconnectionParams.distanceThreshold = 0.0f;
    reconnectionParams.reconnectionMode = RTXDI_PTReconnectionMode::Footprint;
    return reconnectionParams;
}

RTXDI_PTHybridShiftPerFrameParameters GetDefaultReSTIRPTHybridShiftParams()
{
    RTXDI_PTInitialSamplingParameters iParams = GetDefaultReSTIRPTInitialSamplingParams();
    RTXDI_PTHybridShiftPerFrameParameters hybridShiftParams = {};
    hybridShiftParams.maxBounceDepth = iParams.maxBounceDepth;
    hybridShiftParams.maxRcVertexLength = iParams.maxRcVertexLength;
    return hybridShiftParams;
}

RTXDI_PTTemporalResamplingParameters GetDefaultReSTIRPTTemporalResamplingParams(bool dlssRREnabled)
{
    RTXDI_PTTemporalResamplingParameters params = {};
    params.depthThreshold = 0.1f;
    params.enableFallbackSampling = true;
    params.enablePermutationSampling = false;
    params.maxReservoirAge = 30;
    params.normalThreshold = 0.6f;
    params.historyReductionStrength = 0.5f;

    // DLSS-RR: longer history and duplication-based history reduction, with age-based rejection off
    // (the reservoir age doubles as the duplication-map stagnancy signal in that mode).
    params.maxHistoryLength = dlssRREnabled ? 20 : 8;
    params.duplicationBasedHistoryReduction = dlssRREnabled ? 1 : 0;
    params.enableAgeBasedRejection = dlssRREnabled ? 0 : 1;
    return params;
}

RTXDI_BoilingFilterParameters GetDefaultReSTIRPTBoilingFilterParams(bool dlssRREnabled)
{
    RTXDI_BoilingFilterParameters params = {};
    params.boilingFilterStrength = 0.2f;
    // DLSS-RR uses the final-shading firefly replacement instead of the temporal boiling filter.
    params.enableBoilingFilter = dlssRREnabled ? 0 : 1;
    return params;
}

RTXDI_PTSpatialResamplingParameters GetDefaultReSTIRPTSpatialResamplingParams(bool dlssRREnabled)
{
    RTXDI_PTSpatialResamplingParameters params = {};
    params.numSpatialSamples = 1;
    params.depthThreshold = 0.1f;
    params.normalThreshold = 0.6f;
    params.samplingRadius = 32.0f;
    params.numDisocclusionBoostSamples = 8;
    params.enableSpatialHeuristicMode = 0;

    params.duplicationBasedHistoryReduction = dlssRREnabled ? 1 : 0;
    params.maxTemporalHistory = dlssRREnabled ? 20 : 8;
    return params;
}

bool NeedsDuplicationMap(
    const RTXDI_PTTemporalResamplingParameters& temporalResampling,
    const RTXDI_PTDecorrelationParameters& decorrelation)
{
    const bool stagnancyDecorrelation =
        decorrelation.decorrelationMode == RTXDI_PTDecorrelationMode::Stagnancy &&
        decorrelation.decorrelationFactor > 0.0f;
    return temporalResampling.duplicationBasedHistoryReduction != 0 || stagnancyDecorrelation;
}

ReSTIRPTContext::ReSTIRPTContext(const ReSTIRPTStaticParameters& staticParams) :
    m_staticParams(staticParams),
    m_frameIndex(0),
    m_reservoirBufferParams(CalculateReservoirBufferParameters(staticParams.RenderWidth, staticParams.RenderHeight, staticParams.CheckerboardSamplingMode)),
    m_resamplingMode(ReSTIRPT_ResamplingMode::None),
    m_bufferIndices({}),
    m_initialSamplingParams(GetDefaultReSTIRPTInitialSamplingParams()),
    m_decorrelationParams(GetDefaultReSTIRPTDecorrelationParams()),
    m_hybridShiftParams(GetDefaultReSTIRPTHybridShiftParams()),
    m_reconnectionParams(GetDefaultReSTIRPTReconnectionParameters()),
    m_boilingFilterParameters(GetDefaultReSTIRPTBoilingFilterParams()),
    m_temporalResamplingParams(GetDefaultReSTIRPTTemporalResamplingParams()),
    m_spatialResamplingParams(GetDefaultReSTIRPTSpatialResamplingParams())
{
    UpdateBufferIndices();
}

ReSTIRPTStaticParameters ReSTIRPTContext::GetStaticParams() const
{
    return m_staticParams;
}

uint32_t ReSTIRPTContext::GetFrameIndex() const
{
    return m_frameIndex;
}

RTXDI_ReservoirBufferParameters ReSTIRPTContext::GetReservoirBufferParameters() const
{
    return m_reservoirBufferParams;
}

ReSTIRPT_ResamplingMode ReSTIRPTContext::GetResamplingMode() const
{
    return m_resamplingMode;
}

RTXDI_PTBufferIndices ReSTIRPTContext::GetBufferIndices() const
{
    return m_bufferIndices;
}

RTXDI_PTInitialSamplingParameters ReSTIRPTContext::GetInitialSamplingParameters() const
{
    return m_initialSamplingParams;
}

RTXDI_PTDecorrelationParameters ReSTIRPTContext::GetDecorrelationParameters() const
{
    return m_decorrelationParams;
}

RTXDI_PTHybridShiftPerFrameParameters ReSTIRPTContext::GetHybridShiftParameters() const
{
    return m_hybridShiftParams;
}

RTXDI_PTReconnectionParameters ReSTIRPTContext::GetReconnectionParameters() const
{
    return m_reconnectionParams;
}

RTXDI_PTTemporalResamplingParameters ReSTIRPTContext::GetTemporalResamplingParameters() const
{
    return m_temporalResamplingParams;
}

RTXDI_BoilingFilterParameters ReSTIRPTContext::GetBoilingFilterParameters() const
{
    return m_boilingFilterParameters;
}

RTXDI_PTSpatialResamplingParameters ReSTIRPTContext::GetSpatialResamplingParameters() const
{
    return m_spatialResamplingParams;
}

void ReSTIRPTContext::SetFrameIndex(uint32_t frameIndex)
{
    m_frameIndex = frameIndex;
    m_temporalResamplingParams.uniformRandomNumber = JenkinsHash(m_frameIndex);
    UpdateBufferIndices();
}

void ReSTIRPTContext::SetResamplingMode(ReSTIRPT_ResamplingMode resamplingMode)
{
    m_resamplingMode = resamplingMode;
    UpdateBufferIndices();
}

void ReSTIRPTContext::SetInitialSamplingParameters(const RTXDI_PTInitialSamplingParameters& initialSamplingParams)
{
    m_initialSamplingParams = initialSamplingParams;
}

void ReSTIRPTContext::SetDecorrelationParameters(const RTXDI_PTDecorrelationParameters& decorrelationParams)
{
    m_decorrelationParams = decorrelationParams;
}

void ReSTIRPTContext::SetHybridShiftParameters(const RTXDI_PTHybridShiftPerFrameParameters& hybridShiftParams)
{
    m_hybridShiftParams = hybridShiftParams;
}

void ReSTIRPTContext::SetReconnectionParameters(const RTXDI_PTReconnectionParameters& params)
{
    m_reconnectionParams = params;
}

void ReSTIRPTContext::SetTemporalResamplingParameters(const RTXDI_PTTemporalResamplingParameters& temporalResamplingParams)
{
    m_temporalResamplingParams = temporalResamplingParams;
    m_temporalResamplingParams.uniformRandomNumber = JenkinsHash(m_frameIndex);
}

void ReSTIRPTContext::SetBoilingFilterParameters(const RTXDI_BoilingFilterParameters& parameters)
{
    m_boilingFilterParameters = parameters;
}

void ReSTIRPTContext::SetSpatialResamplingParameters(const RTXDI_PTSpatialResamplingParameters& spatialResamplingParams)
{
    m_spatialResamplingParams = spatialResamplingParams;
}

void ReSTIRPTContext::UpdateBufferIndices()
{
    // reset all buffer indices to 0, such that inputBufferIndex != outputBufferIndex can be used to check if a pass is enabled
    m_bufferIndices = {};

    switch (m_resamplingMode)
    {
    case rtxdi::ReSTIRPT_ResamplingMode::None:
        m_bufferIndices.initialPathTracerOutputBufferIndex = 0;
        m_bufferIndices.finalShadingInputBufferIndex = 0;
        break;
    case rtxdi::ReSTIRPT_ResamplingMode::Temporal:
        m_bufferIndices.initialPathTracerOutputBufferIndex = m_frameIndex & 1;
        m_bufferIndices.temporalResamplingInputBufferIndex = 1-m_bufferIndices.initialPathTracerOutputBufferIndex;
        m_bufferIndices.temporalResamplingOutputBufferIndex = m_bufferIndices.initialPathTracerOutputBufferIndex;
        m_bufferIndices.finalShadingInputBufferIndex = m_bufferIndices.temporalResamplingOutputBufferIndex;
        break;
    case rtxdi::ReSTIRPT_ResamplingMode::Spatial:
        m_bufferIndices.initialPathTracerOutputBufferIndex = 0;
        m_bufferIndices.spatialResamplingInputBufferIndex = 0;
        m_bufferIndices.spatialResamplingOutputBufferIndex = 1;
        m_bufferIndices.finalShadingInputBufferIndex = 1;
        break;
    case rtxdi::ReSTIRPT_ResamplingMode::TemporalAndSpatial:
        m_bufferIndices.initialPathTracerOutputBufferIndex = 0;
        m_bufferIndices.temporalResamplingInputBufferIndex = 1;
        m_bufferIndices.temporalResamplingOutputBufferIndex = 0;
        m_bufferIndices.spatialResamplingInputBufferIndex = 0;
        m_bufferIndices.spatialResamplingOutputBufferIndex = 1;
        m_bufferIndices.finalShadingInputBufferIndex = 1;
        break;
    }

    // Slot 2 is reserved as a copy of the unresampled initial-sampling reservoir so final shading
    // can fall back to it when the decorrelation factor selects the initial sample.
    m_bufferIndices.initialPathTracerPreservedBufferIndex = 2;
}
}