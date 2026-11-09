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

#ifndef RTXDI_PT_RESERVOIR_HLSLI
#define RTXDI_PT_RESERVOIR_HLSLI

#ifndef ENABLE_RESTIR_PT_PATH_REPLAY
#define ENABLE_RESTIR_PT_PATH_REPLAY 0
#endif

#define TARGET_FUNCTION_TYPE float3

#include "../Utils/Color.hlsli"
#include "../Utils/RandomSamplerState.hlsli"
#include "../Utils/ReservoirAddressing.hlsli"
#include "../Utils/SampledLightData.hlsli"
#include "../Utils/Math.hlsli"

#ifndef RTXDI_PT_RESERVOIR_BUFFER
#error "RTXDI_PT_RESERVOIR_BUFFER must be defined to point to a RWStructuredBuffer<RTXDI_PackedPTReservoir> type resource"
#endif

#define RTXDI_PTRESERVOIR_AGE_MAX 31

// This structure represents a indirect lighting reservoir that stores the radiance and weight
// as well as its the position where the radiane come from.
struct RTXDI_PTReservoir
{
#ifdef __cplusplus
    using float3 = float[3];
    using uint = uint32_t;
#endif

    static const uint maxM = 0xffff;

    // Position of the 2nd bounce surface (OR env map direction on miss?)
    float3 translatedWorldPosition;

    // Overloaded: represents RIS weight sum during streaming,
    // then reservoir weight (inverse PDF) after FinalizeResampling
    float weightSum;

    // Normal vector of the 2nd bounce surface.
    float3 worldNormal;

    // Number of samples considered for this reservoir
    float M;

    // Incoming radiance from the 2nd bounce surface.
    float3 radiance;

    // Number of frames the chosen sample has survived.
    // when used for age-based sample rejection, it does not reset to 0 when the sample is replaced by a neighbor's sample in spatial reuse
    // when used for temporal duplication map, it resets to 0 when the sample is replaced by a neighbor's sample in spatial reuse
    uint age;

    // General-purpose single-bit scratch flag packed into the reservoir. It carries different meaning
    // at different pipeline stages, so always access it through the semantic alias getters/setters
    // below rather than touching it directly:
    //   RTXDI_Get/SetShouldBoostSpatialSamples - temporal->spatial.
    //   RTXDI_Get/SetFireflyDetected           - spatial->FinalShading.
    int auxFlag;

    float rcWiPdf;

    float partialJacobian;

    // Reconnection vertex length
    uint rcVertexLength;

    // Length of path from primary surface to the final light
    uint pathLength;

    uint randomSeed;

    uint randomIndex;

    float3 targetFunction;
};

RTXDI_PTReservoir RTXDI_EmptyPTReservoir()
{
    RTXDI_PTReservoir reservoir = (RTXDI_PTReservoir)0;
    return reservoir;
}

bool RTXDI_IsValidPTReservoir(const RTXDI_PTReservoir reservoir)
{
    return reservoir.M > 0;
}

// Semantic aliases over the reused single-bit auxFlag (see the struct comment). Both alias the same
// storage; only one meaning is live at a time depending on the pipeline stage.

// Set when temporal resampling doesn't find a neighbor: written by temporal resampling, read by spatial resampling.
bool RTXDI_GetShouldBoostSpatialSamples(const RTXDI_PTReservoir reservoir)
{
    return reservoir.auxFlag != 0;
}
void RTXDI_SetShouldBoostSpatialSamples(inout RTXDI_PTReservoir reservoir, bool value)
{
    reservoir.auxFlag = value ? 1 : 0;
}

// Firefly-replacement flag: written at the end of spatial resampling (boiling filter), read by
// FinalShading. Safe to reuse the same bit because MCap has already consumed it by then.
bool RTXDI_GetFireflyDetected(const RTXDI_PTReservoir reservoir)
{
    return reservoir.auxFlag != 0;
}
void RTXDI_SetFireflyDetected(inout RTXDI_PTReservoir reservoir, bool value)
{
    reservoir.auxFlag = value ? 1 : 0;
}

// Adds `newReservoir` into `reservoir`, returns true if the new reservoir's sample was selected.
// This is a very general form, allowing input parameters to specify normalization and target function
// rather than computing them from `newReservoir`.  Named "internal" since these parameters take
// different meanings (e.g., in RTXDI_CombinePTReservoirs())
bool RTXDI_InternalSimplePTResample(inout RTXDI_PTReservoir targetReservoir,
                            const RTXDI_PTReservoir newReservoir,
                            float random,
                            TARGET_FUNCTION_TYPE newTargetFunction, // Usually closely related to the sample normalization,
                            float sampleNormalization, //     typically off by some multiplicative factor
                            float sampleM // In its most basic form, should be newReservoir.M
)
{
    // What's the current weight (times any prior-step RIS normalization factor)
    float risWeight = RTXDI_Luminance(newTargetFunction) * sampleNormalization;
    risWeight = (isnan(risWeight) || isinf(risWeight)) ? 0.0f : risWeight;

    // Our *effective* candidate pool is the sum of our candidates plus those of our neighbors
    targetReservoir.M += sampleM;

    // Update the weight sum
    targetReservoir.weightSum += risWeight;

    // Decide if we will randomly pick this sample
    const bool selectSample = (random * targetReservoir.weightSum < risWeight);

    // If we did select this sample, update the relevant data
    if (selectSample)
    {
        targetReservoir.translatedWorldPosition = newReservoir.translatedWorldPosition;
        targetReservoir.radiance = newReservoir.radiance;
        targetReservoir.worldNormal = newReservoir.worldNormal;
        targetReservoir.age = newReservoir.age;

        targetReservoir.targetFunction = newTargetFunction;
        targetReservoir.randomSeed = newReservoir.randomSeed;
        targetReservoir.randomIndex = newReservoir.randomIndex;
        targetReservoir.rcVertexLength = newReservoir.rcVertexLength;
        targetReservoir.pathLength = newReservoir.pathLength;
        targetReservoir.partialJacobian = newReservoir.partialJacobian;
        targetReservoir.rcWiPdf = newReservoir.rcWiPdf;
    }

    return selectSample;
}

// Adds a reservoir with one sample into this reservoir.
// Algorithm (4) from the ReSTIR paper, Combining the streams of multiple reservoirs.
// Normalization - Equation (6) - is postponed until all reservoirs are combined.
bool RTXDI_CombinePTReservoirs(inout RTXDI_PTReservoir targetReservoir, RTXDI_PTReservoir newReservoir, float random, TARGET_FUNCTION_TYPE newTargetFunction)
{
    return RTXDI_InternalSimplePTResample(targetReservoir, newReservoir, random, newTargetFunction, newReservoir.weightSum * newReservoir.M, newReservoir.M);
}

void RTXDI_FinalizePTResampling(inout RTXDI_PTReservoir reservoir, in float numerator, in float denominator)
{
    reservoir.weightSum = (denominator > 0.0) ? (numerator * reservoir.weightSum / denominator) : 0.f;
}

bool RTXDI_ConnectsToNeeLight(RTXDI_PTReservoir reservoir)
{
    return isinf(reservoir.radiance.x);
}

RTXDI_SampledLightData RTXDI_GetSampledLightData(RTXDI_PTReservoir reservoir)
{
    RTXDI_SampledLightData sampledLightData;
    sampledLightData.lightData = asuint(reservoir.radiance.y);
    sampledLightData.uvData = asuint(reservoir.radiance.z);
    return sampledLightData;
}

/*
 * The RNG stored in the reservoir is used for fully replaying its path
 * Because the path tracer context uses it to generate reconnection parameters
 * before tracing, the RNG state is not the same state used for generating
 * the outgoing ray from the primary surface.
 * This function advances returns RNG that has been advanced to that state
 * so that its next use in sampling the BSDF for an outgoing ray as
 * done in RAB_PathTrace will produce the correct output.
 */
RTXDI_RandomSamplerState RTXDI_GetRngForShading(RTXDI_PTReservoir reservoir)
{
    return RTXDI_CreateRandomSamplerFromDirectSeed(reservoir.randomSeed, reservoir.randomIndex + 4);
}

// Creates a PT reservoir from a raw light sample.
// Note: the original sample PDF can be embedded into sampleRadiance, in which case the samplePdf parameter should be set to 1.0.
RTXDI_PTReservoir RTXDI_MakePTReservoir(const float3 targetFunction,
                                        const uint randomSeed,
                                        const uint randomIndex,
                                        const uint rcVertexLength,
                                        const uint pathLength,
                                        const float partialJacobian,
                                        const float rcWiPdf,

                                        const float3 translatedWorldPosition,
                                        const float3 worldNormal,
                                        const float3 radiance,
                                        const float samplePdf)
{
    RTXDI_PTReservoir reservoir = (RTXDI_PTReservoir)0;
    reservoir.translatedWorldPosition = translatedWorldPosition;
    reservoir.worldNormal = worldNormal;
    reservoir.radiance = radiance;
    reservoir.weightSum = samplePdf > 0.0 ? 1.0 / samplePdf : 0.0;
    reservoir.M = 1;
    reservoir.age = 0;
    reservoir.auxFlag = 0;

    reservoir.targetFunction = targetFunction;
    reservoir.randomSeed = randomSeed;
    reservoir.randomIndex = randomIndex;
    reservoir.partialJacobian = partialJacobian;
    reservoir.rcVertexLength = rcVertexLength;
    reservoir.pathLength = pathLength;
    reservoir.rcWiPdf = rcWiPdf;

    return reservoir;
}

//
// Packing and Unpacking
//

uint2 EncodeSnorm3x16(float3 value)
{
    const uint x = uint(round(clamp(value.x, -1.0, 1.0) * 32767.0) + 32767.0);
    const uint y = uint(round(clamp(value.y, -1.0, 1.0) * 32767.0) + 32767.0);
    const uint z = uint(round(clamp(value.z, -1.0, 1.0) * 32767.0) + 32767.0);
    return uint2((x & 0x0000ffff) | (y << 16), (z & 0x0000ffff));
}

float3 DecodeSnorm3x16(uint2 packedValue)
{
    return float3(clamp((float(packedValue.x & 0xffff) - 32767.0f) * rcp(32767.0f), -1.0, 1.0), clamp((float(packedValue.x >> 16u) - 32767.0f) * rcp(32767.0f), -1.0, 1.0),
                  clamp((float(packedValue.y & 0xffff) - 32767.0f) * rcp(32767.0f), -1.0, 1.0));
}

RTXDI_PackedPTReservoir RTXDI_PackPTReservoir(RTXDI_PTReservoir reservoir)
{
    RTXDI_PackedPTReservoir packedData = (RTXDI_PackedPTReservoir)0;
    packedData.Data0.xyz = asuint(reservoir.translatedWorldPosition);
    packedData.Data0.w = asuint(reservoir.weightSum);
    packedData.Data2.xyz = asuint(reservoir.radiance);
    packedData.Data2.w = (reservoir.age & RTXDI_PTRESERVOIR_AGE_MAX);
    packedData.Data2.w |= reservoir.auxFlag ? 0x80 : 0x0;

    packedData.Data1.xy = EncodeSnorm3x16(reservoir.worldNormal);
    packedData.Data1.y |= f32tof16(reservoir.M) << 16;
    packedData.Data1.z = asuint(reservoir.partialJacobian);
    packedData.Data1.w = asuint(reservoir.rcWiPdf);
    packedData.Data2.w = (packedData.Data2.w & 0xff) | ((reservoir.rcVertexLength & 0xff) << 8) | ((reservoir.pathLength & 0xff) << 16) | ((reservoir.randomIndex & 0xff) << 24);
    packedData.Data3.xyz = asuint(reservoir.targetFunction);
    packedData.Data3.w = reservoir.randomSeed;

    return packedData;
}

// RTXDI_PTReservoir RTXDI_PTReservoirFromPackedData(in const RTXDI_PackedPTReservoir PackedData)
RTXDI_PTReservoir RTXDI_UnpackPTReservoir(in const RTXDI_PackedPTReservoir packedData)
{
    RTXDI_PTReservoir unpacked = (RTXDI_PTReservoir)0;

    unpacked.translatedWorldPosition = asfloat(packedData.Data0.xyz);
    unpacked.weightSum = asfloat(packedData.Data0.w);
    unpacked.radiance = asfloat(packedData.Data2.xyz);
    unpacked.age = packedData.Data2.w & RTXDI_PTRESERVOIR_AGE_MAX;

    unpacked.worldNormal = normalize(DecodeSnorm3x16(packedData.Data1.xy));
    unpacked.M = f16tof32(packedData.Data1.y >> 16);
    unpacked.auxFlag = ((packedData.Data2.w & 0x80) != 0x0);
    unpacked.targetFunction = asfloat(packedData.Data3.xyz);
    unpacked.rcWiPdf = asfloat(packedData.Data1.w);
    unpacked.partialJacobian = asfloat(packedData.Data1.z);
    unpacked.rcVertexLength = (packedData.Data2.w >> 8) & 0xff;
    unpacked.pathLength = (packedData.Data2.w >> 16) & 0xff;
    unpacked.randomIndex = (packedData.Data2.w >> 24) & 0xff;
    unpacked.randomSeed = packedData.Data3.w;

    return unpacked;
}

//
// Loading and Storing
//

uint ComputePTReservoirAddress(uint2 pixelCoord, int2 bufferDim, int slice)
{
    static const uint tileSize = 4;

    const int2 paddedBufferDim = (bufferDim + tileSize - 1) / tileSize * tileSize;

    const uint rowStride = paddedBufferDim.x * tileSize;

    int2 tile = pixelCoord / tileSize;
    int2 tileCoord = pixelCoord % tileSize;

    uint address = slice * paddedBufferDim.x * paddedBufferDim.y;
    address += tile.y * rowStride;
    address += tile.x * tileSize * tileSize;
    address += tileCoord.y * tileSize + tileCoord.x;

    return address;
}

void RTXDI_StorePackedPTReservoir(const RTXDI_PackedPTReservoir packedPTReservoir, RTXDI_ReservoirBufferParameters reservoirParams, uint2 reservoirPosition, uint reservoirArrayIndex)
{
    uint pointer = RTXDI_ReservoirPositionToPointer(reservoirParams, reservoirPosition, reservoirArrayIndex);
    RTXDI_PT_RESERVOIR_BUFFER[pointer] = packedPTReservoir;
}

void RTXDI_StorePTReservoir(const RTXDI_PTReservoir reservoir, RTXDI_ReservoirBufferParameters reservoirParams, uint2 reservoirPosition, uint reservoirArrayIndex)
{
    RTXDI_PackedPTReservoir packedReservoir = RTXDI_PackPTReservoir(reservoir);
    RTXDI_StorePackedPTReservoir(packedReservoir, reservoirParams, reservoirPosition, reservoirArrayIndex);
}

RTXDI_PackedPTReservoir RTXDI_LoadPackedPTReservoir(RTXDI_ReservoirBufferParameters reservoirParams, uint2 reservoirPosition, uint reservoirArrayIndex)
{
    uint pointer = RTXDI_ReservoirPositionToPointer(reservoirParams, reservoirPosition, reservoirArrayIndex);
    return RTXDI_PT_RESERVOIR_BUFFER[pointer];
}

RTXDI_PTReservoir RTXDI_LoadPTReservoir(RTXDI_ReservoirBufferParameters reservoirParams, uint2 reservoirPosition, uint reservoirArrayIndex)
{
    RTXDI_PackedPTReservoir packedReservoir = RTXDI_LoadPackedPTReservoir(reservoirParams, reservoirPosition, reservoirArrayIndex);
    return RTXDI_UnpackPTReservoir(packedReservoir);
}

#endif // RTXDI_PT_RESERVOIR_HLSLI