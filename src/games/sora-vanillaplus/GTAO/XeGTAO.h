///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2021, Intel Corporation 
// 
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XeGTAO is based on GTAO/GTSO "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion", 
// https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
// 
// Implementation:  Filip Strugar (filip.strugar@intel.com), Steve Mccalla <stephen.mccalla@intel.com>
// Version:         1.30
// Details:         https://github.com/GameTechDev/XeGTAO
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __XE_GTAO_TYPES_H__
#define __XE_GTAO_TYPES_H__

#ifdef __cplusplus

namespace XeGTAO {
    struct Matrix4x4 { float m[16]; };
    struct Vector3 { float x, y, z; };
    struct Vector2 { float x, y; };
    struct Vector2i { int x, y; };
    typedef unsigned int uint;
#else
    #define Matrix4x4 float4x4
    #define Vector3 float3
    #define Vector2 float2
    #define Vector2i int2
#endif

    #define XE_GTAO_DEPTH_MIP_LEVELS 5
    #define XE_GTAO_NUMTHREADS_X 8
    #define XE_GTAO_NUMTHREADS_Y 8

    struct GTAOConstants {
        Vector2i ViewportSize;
        Vector2 ViewportPixelSize;
        Vector2 DepthUnpackConsts;
        Vector2 CameraTanHalfFOV;
        Vector2 NDCToViewMul;
        Vector2 NDCToViewAdd;
        Vector2 NDCToViewMul_x_PixelSize;
        float EffectRadius;
        float EffectFalloffRange;
        float RadiusMultiplier;
        float Padding0;
        float FinalValuePower;
        float DenoiseBlurBeta;
        float SampleDistributionPower;
        float ThinOccluderCompensation;
        float DepthMIPSamplingOffset;
        int NoiseIndex;
    };

    #define XE_GTAO_USE_DEFAULT_CONSTANTS 0
    #define XE_GTAO_DEFAULT_RADIUS_MULTIPLIER (1.457f)
    #define XE_GTAO_DEFAULT_FALLOFF_RANGE (0.615f)
    #define XE_GTAO_DEFAULT_SAMPLE_DISTRIBUTION_POWER (2.0f)
    #define XE_GTAO_DEFAULT_THIN_OCCLUDER_COMPENSATION (0.0f)
    #define XE_GTAO_DEFAULT_FINAL_VALUE_POWER (2.2f)
    #define XE_GTAO_DEFAULT_DEPTH_MIP_SAMPLING_OFFSET (3.30f)
    #define XE_GTAO_OCCLUSION_TERM_SCALE (1.5f)

    #define XE_HILBERT_LEVEL 6U
    #define XE_HILBERT_WIDTH ( (1U << XE_HILBERT_LEVEL) )
    #define XE_HILBERT_AREA ( XE_HILBERT_WIDTH * XE_HILBERT_WIDTH )

    inline uint HilbertIndex(uint posX, uint posY) {
        uint index = 0U;
        for (uint curLevel = XE_HILBERT_WIDTH / 2U; curLevel > 0U; curLevel /= 2U) {
            uint regionX = (posX & curLevel) > 0U;
            uint regionY = (posY & curLevel) > 0U;
            index += curLevel * curLevel * ((3U * regionX) ^ regionY);
            if (regionY == 0U) {
                if (regionX == 1U) {
                    posX = uint((XE_HILBERT_WIDTH - 1U)) - posX;
                    posY = uint((XE_HILBERT_WIDTH - 1U)) - posY;
                }
                uint temp = posX;
                posX = posY;
                posY = temp;
            }
        }
        return index;
    }

#ifdef __cplusplus
}

#endif

#endif // __XE_GTAO_TYPES_H__