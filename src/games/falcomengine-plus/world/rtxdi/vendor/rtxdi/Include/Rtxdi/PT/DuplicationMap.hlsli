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

#ifndef RTXDI_PT_DUPLICATION_MAP_HLSLI
#define RTXDI_PT_DUPLICATION_MAP_HLSLI

#include "Rtxdi/PT/Reservoir.hlsli"

// The duplication map has two channels, both written from the final resampled reservoir:
//   .x - spatial duplication count (how many nearby pixels share the same sample ID), used for
//        duplication-based temporal history reduction. Filled by RTXDI_PTComputeDuplicationMap.
//   .y - temporal stagnancy (reservoir age / 40), fed through RTXDI_PTComputeSmoothedDuplicationMap
//        to drive the final-shading decorrelation probability.
//
// Requires the application to provide these resource aliases:
//   RTXDI_PT_SAMPLE_ID_TEXTURE            - RWTexture2D<uint>   per-pixel sample ID.
//   RTXDI_PT_DUPLICATION_MAP              - RWTexture2D<float2> the .x/.y map above.
//   RTXDI_PT_SMOOTHED_DUPLICATION_MAP     - RWTexture2D<float>  current-frame smoothed .y output.
//   RTXDI_PT_PREV_SMOOTHED_DUPLICATION_MAP- Texture2D<float>    previous-frame smoothed .y (for reprojection).
// The smoothed pass takes the previous-frame reprojection (prevPixel / prevValid) from its caller,
// since motion-vector reprojection is application-specific.

// Stores the per-pixel inputs the duplication map is later built from: the reservoir's sample ID
// (0 for empty reservoirs) and its normalized stagnancy (age / 40) in the map's .y channel.
void RTXDI_PTStoreDuplicationInputs(uint2 pixelPosition, RTXDI_PTReservoir reservoir)
{
    const uint sampleID = (reservoir.weightSum == 0.0) ? 0u : reservoir.randomSeed;
    RTXDI_PT_SAMPLE_ID_TEXTURE[pixelPosition] = sampleID;
    RTXDI_PT_DUPLICATION_MAP[pixelPosition].y = min(1.f, float(reservoir.age) / 40.f);
}

// À-trous box filter step for the reprojected previous-frame tap (1 = dense 5x5; >1 = dilated,
// larger footprint without more taps).
#define RTXDI_PT_SMOOTHED_DUPMAP_FILTER_STEP 2

// Temporally smooths the duplication map's .y (stagnancy) channel: blends the current-frame value
// with a reprojected, spatially-averaged previous-frame value via an exponential moving average.
void RTXDI_PTComputeSmoothedDuplicationMap(uint2 pixelPosition, uint2 viewportSize, float emaFactor, int2 prevPixel, bool prevValid)
{
    const int2 frameDim = int2(viewportSize);

    // Current-frame value = reservoir.age / 40, written by RTXDI_PTStoreDuplicationInputs during resampling.
    const float current = saturate(RTXDI_PT_DUPLICATION_MAP[pixelPosition].y);

    // prevPixel / prevValid are the caller-supplied reprojection into the previous frame.
    float smoothed;
    if (prevValid)
    {
        // 5x5 (à-trous) box average around the reprojected position. Weights are uniform; we divide
        // by the number of in-bounds taps so screen-edge taps don't bias the result.
        const int step = RTXDI_PT_SMOOTHED_DUPMAP_FILTER_STEP;
        float prevSum = 0.0f;
        float weightSum = 0.0f;
        const bool filterInBounds = all(prevPixel >= step * 2) && all(prevPixel < frameDim - step * 2);
        if (filterInBounds)
        {
            [unroll]
            for (int dy = -2; dy <= 2; ++dy)
            {
                [unroll]
                for (int dx = -2; dx <= 2; ++dx)
                {
                    const int2 tap = prevPixel + int2(dx, dy) * step;
                    prevSum += RTXDI_PT_PREV_SMOOTHED_DUPLICATION_MAP[tap];
                }
            }
            weightSum = 25.0f;
        }
        else
        {
            [unroll]
            for (int dy = -2; dy <= 2; ++dy)
            {
                [unroll]
                for (int dx = -2; dx <= 2; ++dx)
                {
                    const int2 tap = prevPixel + int2(dx, dy) * step;
                    if (all(tap >= 0) && all(tap < frameDim))
                    {
                        prevSum += RTXDI_PT_PREV_SMOOTHED_DUPLICATION_MAP[tap];
                        weightSum += 1.0f;
                    }
                }
            }
        }
        const float prev = (weightSum > 0.0f) ? (prevSum / weightSum) : current;

        // emaFactor == 1 -> fully use current sample; 0 -> keep history indefinitely.
        smoothed = saturate(emaFactor) * current + (1.0f - saturate(emaFactor)) * prev;
    }
    else
    {
        // No valid history (screen-edge disocclusion or first frame): bootstrap with current.
        smoothed = current;
    }

    RTXDI_PT_SMOOTHED_DUPLICATION_MAP[pixelPosition] = smoothed;
}

// The spatial duplication-count reduction uses group-shared memory, so it is only compiled into the
// dedicated compute pass that defines RTXDI_PT_ENABLE_DUPLICATION_MAP_COUNT (keeps groupshared out of
// the ray-generation resampling shaders that include this header only for the functions above).
#ifdef RTXDI_PT_ENABLE_DUPLICATION_MAP_COUNT

// Dispatched as 16x16 thread groups. Each group loads a 32x32 tile (16 + 2*radius) of sample IDs into
// LDS and counts, per pixel, how many of its 17x17 neighbors share the same (non-zero) sample ID.
#define RTXDI_PT_DUPMAP_GROUP_SIZE 16
#define RTXDI_PT_DUPMAP_RADIUS 8
#define RTXDI_PT_DUPMAP_TILE (RTXDI_PT_DUPMAP_GROUP_SIZE + 2 * RTXDI_PT_DUPMAP_RADIUS)
#define RTXDI_PT_DUPMAP_MAX_COUNT 255u

// Index as [y][x] so lane-varying x maps to different LDS banks (no bank conflicts).
groupshared uint s_RTXDI_PTDuplicationSampleIDs[RTXDI_PT_DUPMAP_TILE][RTXDI_PT_DUPMAP_TILE];

void RTXDI_PTComputeDuplicationMap(uint2 groupID, uint2 threadID, uint2 viewportSize)
{
    const int2 frameDim = int2(viewportSize);
    const int2 base = int2(groupID) * RTXDI_PT_DUPMAP_GROUP_SIZE - RTXDI_PT_DUPMAP_RADIUS;

    // Load the 32x32 tile into LDS (each of the 16x16 threads loads a 2x2 block).
    [unroll] for (int dy = 0; dy < 2; ++dy)
        [unroll] for (int dx = 0; dx < 2; ++dx)
        {
            int2 readPixel = base + int2(threadID.x * 2 + dx, threadID.y * 2 + dy);
            s_RTXDI_PTDuplicationSampleIDs[threadID.y * 2 + dy][threadID.x * 2 + dx] = RTXDI_PT_SAMPLE_ID_TEXTURE[readPixel];
        }
    GroupMemoryBarrierWithGroupSync();

    int2 pixel = base + int2(threadID.x + RTXDI_PT_DUPMAP_RADIUS, threadID.y + RTXDI_PT_DUPMAP_RADIUS);
    if (any(pixel < 0) || any(pixel >= frameDim))
        return;

    uint ownSample = s_RTXDI_PTDuplicationSampleIDs[threadID.y + RTXDI_PT_DUPMAP_RADIUS][threadID.x + RTXDI_PT_DUPMAP_RADIUS];
    uint count = 0u;
    if (ownSample != 0u)
    {
        [unroll] for (int ny = -RTXDI_PT_DUPMAP_RADIUS; ny <= RTXDI_PT_DUPMAP_RADIUS; ++ny)
        {
            [unroll] for (int nx = -RTXDI_PT_DUPMAP_RADIUS; nx <= RTXDI_PT_DUPMAP_RADIUS; ++nx)
            {
                uint n = s_RTXDI_PTDuplicationSampleIDs[threadID.y + RTXDI_PT_DUPMAP_RADIUS + ny][threadID.x + RTXDI_PT_DUPMAP_RADIUS + nx];
                count += (n == ownSample && (nx | ny) != 0) ? 1u : 0u;
            }
        }
    }
    RTXDI_PT_DUPLICATION_MAP[pixel].x = saturate(count / float(RTXDI_PT_DUPMAP_MAX_COUNT));
}

#endif // RTXDI_PT_ENABLE_DUPLICATION_MAP_COUNT

#endif // RTXDI_PT_DUPLICATION_MAP_HLSLI
