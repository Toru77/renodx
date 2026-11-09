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

#ifndef RTXDI_PT_SPATIAL_NEIGHBOR_SELECTION_HLSLI
#define RTXDI_PT_SPATIAL_NEIGHBOR_SELECTION_HLSLI

#include "Rtxdi/Utils/RandomSamplerState.hlsli"
#include "Rtxdi/Utils/RandomSamplerPerPassSeeds.hlsli"

// The spatial-heuristic mode pre-selects, for each pixel, up to SPATIAL_HEURISTIC_MAX_NEIGHBORS
// compatible neighbors via weighted reservoir sampling over a disk of candidates. The picks are
// packed (x | y<<16) into RTXDI_SPATIAL_NEIGHBOR_SELECTION_BUFFER and consumed by spatial resampling.
//
// Requires the application to provide:
//   RAB_GetNeighborSelectionSurface     - reads the neighbor-selection G-buffer surface for a pixel.
//   RTXDI_SPATIAL_NEIGHBOR_SELECTION_BUFFER - RWByteAddressBuffer that receives the packed picks.

#ifndef SPATIAL_HEURISTIC_MAX_NEIGHBORS
#define SPATIAL_HEURISTIC_MAX_NEIGHBORS 4
#endif
#ifndef SPATIAL_HEURISTIC_INVALID_NEIGHBOR
#define SPATIAL_HEURISTIC_INVALID_NEIGHBOR 0xFFFFFFFFu
#endif

static const uint RTXDI_PT_NEIGHBOR_SELECTION_CANDIDATE_COUNT = 32;
static const float RTXDI_PT_NEIGHBOR_SELECTION_SEARCH_RADIUS = 50.0;

// Higher when the surfaces face the same way and are close in world space (normalized by the
// center's view-space footprint), so nearby coplanar neighbors are favored.
float RTXDI_PTNeighborCompatibilityScore(float3 centerNormal, float3 centerPos, float centerDist,
                                         float3 neighborNormal, float3 neighborPos)
{
    float normalSim = pow(max(0.0, dot(centerNormal, neighborNormal)), 8);
    float s = sqrt(0.05 * centerDist * centerDist / 3.141592654);
    float posSim = exp(-distance(centerPos, neighborPos) / max(s, 1e-6));
    return normalSim * posSim;
}

void RTXDI_PTSpatialNeighborSelection(uint2 pixelPosition, uint2 viewportSize, uint frameIndex, uint numSpatialSamples)
{
    const uint neighborCount = min(numSpatialSamples, SPATIAL_HEURISTIC_MAX_NEIGHBORS);
    const uint linearPixelIndex = pixelPosition.y * viewportSize.x + pixelPosition.x;

    float3 centerNormal;
    float3 centerPos;
    float centerDist;
    if (!RAB_GetNeighborSelectionSurface(int2(pixelPosition), centerNormal, centerPos, centerDist))
    {
        for (uint k = 0; k < neighborCount; k++)
        {
            uint addr = (linearPixelIndex * SPATIAL_HEURISTIC_MAX_NEIGHBORS + k) * 4;
            RTXDI_SPATIAL_NEIGHBOR_SELECTION_BUFFER.Store(addr, SPATIAL_HEURISTIC_INVALID_NEIGHBOR);
        }
        return;
    }

    RTXDI_RandomSamplerState rng = RTXDI_InitRandomSampler(pixelPosition, frameIndex, RTXDI_PT_SPATIAL_NEIGHBOR_SELECTION_RANDOM_SEED);

    // Weighted Reservoir Sampling: K reservoirs running in parallel over the candidate stream
    float wSum[SPATIAL_HEURISTIC_MAX_NEIGHBORS];
    uint packedSelected[SPATIAL_HEURISTIC_MAX_NEIGHBORS];
    for (uint k = 0; k < neighborCount; k++)
    {
        wSum[k] = 0.0;
        packedSelected[k] = SPATIAL_HEURISTIC_INVALID_NEIGHBOR;
    }

    uint goodCandidateCount = 0;

    for (uint c = 0; c < RTXDI_PT_NEIGHBOR_SELECTION_CANDIDATE_COUNT; c++)
    {
        float rnd1 = RTXDI_GetNextRandom(rng);
        float rnd2 = RTXDI_GetNextRandom(rng);
        float angle = rnd1 * 2.0 * 3.141592654;
        float radius = sqrt(rnd2) * RTXDI_PT_NEIGHBOR_SELECTION_SEARCH_RADIUS;
        int2 candidatePixel = int2(pixelPosition) + int2(int(cos(angle) * radius), int(sin(angle) * radius));

        if (any(candidatePixel < 0) || any(candidatePixel >= int2(viewportSize)))
            continue;
        if (all(candidatePixel == int2(pixelPosition)))
            continue;

        float3 neighborNormal;
        float3 neighborPos;
        float neighborDist;
        if (!RAB_GetNeighborSelectionSurface(candidatePixel, neighborNormal, neighborPos, neighborDist))
            continue;

        float score = RTXDI_PTNeighborCompatibilityScore(centerNormal, centerPos, centerDist, neighborNormal, neighborPos);
        if (score <= 0.0)
            continue;

        if (score >= 0.5)
            goodCandidateCount++;

        uint packed = (uint(candidatePixel.x) & 0xFFFF) | (uint(candidatePixel.y) << 16);

        for (uint k = 0; k < neighborCount; k++)
        {
            wSum[k] += score;
            if (RTXDI_GetNextRandom(rng) < score / wSum[k])
            {
                packedSelected[k] = packed;
            }
        }

        if (goodCandidateCount >= neighborCount)
            break;
    }

    for (uint k = 0; k < neighborCount; k++)
    {
        uint addr = (linearPixelIndex * SPATIAL_HEURISTIC_MAX_NEIGHBORS + k) * 4;
        RTXDI_SPATIAL_NEIGHBOR_SELECTION_BUFFER.Store(addr, packedSelected[k]);
    }
}

#endif // RTXDI_PT_SPATIAL_NEIGHBOR_SELECTION_HLSLI
