///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2021, Intel Corporation 
// 
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GT-VBAO is based on GTAO/GTSO "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion", 
// https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
// 
// Implementation:  Filip Strugar (filip.strugar@intel.com), Steve Mccalla <stephen.mccalla@intel.com>         (\_/)
// Version:         1.02                                                                                      (='.'=)
// Details:         https://github.com/GameTechDev/GT-VBAO                                                     (")_(")
//
// Version history:
// 1.00 (2021-08-09): Initial release
// 1.01 (2021-09-02): Fix for depth going to inf for 'far' depth buffer values that are out of fp16 range
// 1.02 (2021-09-03): More fast_acos use and made final horizon cos clamping optional (off by default): 3-4% perf boost
// 1.10 (2021-09-03): Added a couple of heuristics to combat over-darkening errors in certain scenarios
// 1.20 (2021-09-06): Optional normal from depth generation is now a standalone pass: no longer integrated into 
//                    main GT-VBAO pass to reduce complexity and allow reuse; also quality of generated normals improved
// 1.21 (2021-09-28): Replaced 'groupshared'-based denoiser with a slightly slower multi-pass one where a 2-pass new
//                    equals 1-pass old. However, 1-pass new is faster than the 1-pass old and enough when TAA enabled.
// 1.22 (2021-09-28): Added 'GTVBAO_' prefix to all local functions to avoid name clashes with various user codebases.
// 1.30 (2021-10-10): Added support for directional component (bent normals).
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __GT_VBAO_TYPES_H__
#define __GT_VBAO_TYPES_H__

    // hlsl mapping
    #define Vector2         float2
    #define Vector2i        int2

    // Global consts that need to be visible from the shader side
    #define GT_VBAO_DEPTH_MIP_LEVELS                    5                   // this one is hard-coded to 5 for now
    #define GT_VBAO_NUMTHREADS_X                        8                   // these can be changed
    #define GT_VBAO_NUMTHREADS_Y                        8                   // these can be changed

    struct GTAOConstants
    {
        Vector2i                ViewportSize;
        Vector2                 ViewportPixelSize;                  // .zw == 1.0 / ViewportSize.xy

        Vector2                 DepthUnpackConsts;
        Vector2                 CameraTanHalfFOV;

        Vector2                 NDCToViewMul;
        Vector2                 NDCToViewAdd;

        Vector2                 NDCToViewMul_x_PixelSize;
        float                   EffectRadius;                       // world (viewspace) maximum size of the shadow

        float                   RadiusMultiplier;
        float                   FinalValuePower;
        float                   SampleDistributionPower;
        float                   BitmaskThickness;

        float                   DepthMIPSamplingOffset;
        int                     NoiseIndex;                         // frameIndex % 64 if using TAA or 0 otherwise
    };

    #define GT_VBAO_OCCLUSION_TERM_SCALE                    (1.5f)      // for packing in UNORM (because raw, pre-denoised occlusion term can overshoot 1 but will later average out to 1)

    // From https://www.shadertoy.com/view/3tB3z3 - except we're using R2 here
    #define XE_HILBERT_LEVEL    6U
    #define XE_HILBERT_WIDTH    ( (1U << XE_HILBERT_LEVEL) )
    #define XE_HILBERT_AREA     ( XE_HILBERT_WIDTH * XE_HILBERT_WIDTH )
    inline uint HilbertIndex( uint posX, uint posY )
    {   
        uint index = 0U;
        for( uint curLevel = XE_HILBERT_WIDTH/2U; curLevel > 0U; curLevel /= 2U )
        {
            uint regionX = ( posX & curLevel ) > 0U;
            uint regionY = ( posY & curLevel ) > 0U;
            index += curLevel * curLevel * ( (3U * regionX) ^ regionY);
            if( regionY == 0U )
            {
                if( regionX == 1U )
                {
                    posX = uint( (XE_HILBERT_WIDTH - 1U) ) - posX;
                    posY = uint( (XE_HILBERT_WIDTH - 1U) ) - posY;
                }

                uint temp = posX;
                posX = posY;
                posY = temp;
            }
        }
        return index;
    }

#endif // __GT_VBAO_TYPES_H__
