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

#include "XeGTAO.h"

#define XE_GTAO_PI                 (3.1415926535897932384626433832795)
#define XE_GTAO_PI_HALF            (1.5707963267948966192313216916398)

#ifndef XE_GTAO_USE_HALF_FLOAT_PRECISION
#define XE_GTAO_USE_HALF_FLOAT_PRECISION 0
#endif

#if (XE_GTAO_USE_HALF_FLOAT_PRECISION != 0)
    typedef min16float      lpfloat; 
    typedef min16float2     lpfloat2;
    typedef min16float3     lpfloat3;
    typedef min16float4     lpfloat4;
    typedef min16float3x3   lpfloat3x3;
#else
    typedef float           lpfloat;
    typedef float2          lpfloat2;
    typedef float3          lpfloat3;
    typedef float4          lpfloat4;
    typedef float3x3        lpfloat3x3;
#endif

float3 XeGTAO_R11G11B10_UNORM_to_FLOAT3(uint packedInput) {
    float3 unpackedOutput;
    unpackedOutput.x = (float)((packedInput) & 0x000007ff) / 2047.0f;
    unpackedOutput.y = (float)((packedInput >> 11) & 0x000007ff) / 2047.0f;
    unpackedOutput.z = (float)((packedInput >> 22) & 0x000003ff) / 1023.0f;
    return unpackedOutput;
}

uint XeGTAO_FLOAT3_to_R11G11B10_UNORM(float3 unpackedInput) {
    uint packedOutput;
    packedOutput = ((uint(saturate(unpackedInput.x) * 2047 + 0.5f)) |
        (uint(saturate(unpackedInput.y) * 2047 + 0.5f) << 11) |
        (uint(saturate(unpackedInput.z) * 1023 + 0.5f) << 22));
    return packedOutput;
}

lpfloat4 XeGTAO_R8G8B8A8_UNORM_to_FLOAT4(uint packedInput) {
    lpfloat4 unpackedOutput;
    unpackedOutput.x = (lpfloat)(packedInput & 0x000000ff) / (lpfloat)255;
    unpackedOutput.y = (lpfloat)(((packedInput >> 8) & 0x000000ff)) / (lpfloat)255;
    unpackedOutput.z = (lpfloat)(((packedInput >> 16) & 0x000000ff)) / (lpfloat)255;
    unpackedOutput.w = (lpfloat)(packedInput >> 24) / (lpfloat)255;
    return unpackedOutput;
}

uint XeGTAO_FLOAT4_to_R8G8B8A8_UNORM(lpfloat4 unpackedInput) {
    return ((uint(saturate(unpackedInput.x) * (lpfloat)255 + (lpfloat)0.5)) |
        (uint(saturate(unpackedInput.y) * (lpfloat)255 + (lpfloat)0.5) << 8) |
        (uint(saturate(unpackedInput.z) * (lpfloat)255 + (lpfloat)0.5) << 16) |
        (uint(saturate(unpackedInput.w) * (lpfloat)255 + (lpfloat)0.5) << 24));
}

float3 XeGTAO_ComputeViewspacePosition(const float2 screenPos, const float viewspaceDepth, const GTAOConstants consts) {
    float3 ret;
    ret.xy = (consts.NDCToViewMul * screenPos.xy + consts.NDCToViewAdd) * viewspaceDepth;
    ret.z = viewspaceDepth;
    return ret;
}

float XeGTAO_ScreenSpaceToViewSpaceDepth(const float screenDepth, const GTAOConstants consts) {
    float depthLinearizeMul = consts.DepthUnpackConsts.x;
    float depthLinearizeAdd = consts.DepthUnpackConsts.y;
    return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

lpfloat4 XeGTAO_CalculateEdges(const lpfloat centerZ, const lpfloat leftZ, const lpfloat rightZ, const lpfloat topZ, const lpfloat bottomZ) {
    lpfloat4 edgesLRTB = lpfloat4(leftZ, rightZ, topZ, bottomZ) - (lpfloat)centerZ;
    lpfloat slopeLR = (edgesLRTB.y - edgesLRTB.x) * 0.5;
    lpfloat slopeTB = (edgesLRTB.w - edgesLRTB.z) * 0.5;
    lpfloat4 edgesLRTBSlopeAdjusted = edgesLRTB + lpfloat4(slopeLR, -slopeLR, slopeTB, -slopeTB);
    edgesLRTB = min(abs(edgesLRTB), abs(edgesLRTBSlopeAdjusted));
    return lpfloat4(saturate((1.25 - edgesLRTB / (centerZ * 0.011))));
}

lpfloat XeGTAO_PackEdges(lpfloat4 edgesLRTB) {
    edgesLRTB = round(saturate(edgesLRTB) * 2.9);
    return dot(edgesLRTB, lpfloat4(64.0 / 255.0, 16.0 / 255.0, 4.0 / 255.0, 1.0 / 255.0));
}

float3 XeGTAO_CalculateNormal(const float4 edgesLRTB, float3 pixCenterPos, float3 pixLPos, float3 pixRPos, float3 pixTPos, float3 pixBPos) {
    float4 acceptedNormals = saturate(float4(edgesLRTB.x * edgesLRTB.z, edgesLRTB.z * edgesLRTB.y, edgesLRTB.y * edgesLRTB.w, edgesLRTB.w * edgesLRTB.x) + 0.01);
    pixLPos = normalize(pixLPos - pixCenterPos);
    pixRPos = normalize(pixRPos - pixCenterPos);
    pixTPos = normalize(pixTPos - pixCenterPos);
    pixBPos = normalize(pixBPos - pixCenterPos);
    float3 pixelNormal = acceptedNormals.x * cross(pixLPos, pixTPos) +
        +acceptedNormals.y * cross(pixTPos, pixRPos) +
        +acceptedNormals.z * cross(pixRPos, pixBPos) +
        +acceptedNormals.w * cross(pixBPos, pixLPos);
    pixelNormal = normalize(pixelNormal);
    return pixelNormal;
}

lpfloat XeGTAO_FastSqrt(float x) {
    return (lpfloat)(asfloat(0x1fbd1df5 + (asint(x) >> 1)));
}

lpfloat XeGTAO_FastACos(lpfloat inX) {
    const lpfloat PI = 3.141593;
    const lpfloat HALF_PI = 1.570796;
    lpfloat x = abs(inX);
    lpfloat res = -0.156583 * x + HALF_PI;
    res *= XeGTAO_FastSqrt(1.0 - x);
    return (inX >= 0) ? res : PI - res;
}

uint XeGTAO_EncodeVisibilityBentNormal(lpfloat visibility, lpfloat3 bentNormal) {
    return XeGTAO_FLOAT4_to_R8G8B8A8_UNORM(lpfloat4(bentNormal * 0.5 + 0.5, visibility));
}

void XeGTAO_DecodeVisibilityBentNormal(const uint packedValue, out lpfloat visibility, out lpfloat3 bentNormal) {
    lpfloat4 decoded = XeGTAO_R8G8B8A8_UNORM_to_FLOAT4(packedValue);
    bentNormal = decoded.xyz * lpfloat3(2.0, 2.0, 2.0) - lpfloat3(1.0, 1.0, 1.0);
    visibility = decoded.w;
}

void XeGTAO_OutputWorkingTerm(const uint2 pixCoord, lpfloat visibility, lpfloat3 bentNormal, RWTexture2D<uint> outWorkingAOTerm) {
    visibility = saturate(visibility / lpfloat(XE_GTAO_OCCLUSION_TERM_SCALE));
    outWorkingAOTerm[pixCoord] = XeGTAO_EncodeVisibilityBentNormal(visibility, bentNormal);
}

lpfloat3x3 XeGTAO_RotFromToMatrix(lpfloat3 from, lpfloat3 to) {
    const lpfloat e = dot(from, to);
    const lpfloat f = abs(e);
    if (f > lpfloat(1.0 - 0.0003))
        return lpfloat3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);
    const lpfloat3 v = cross(from, to);
    const lpfloat h = (1.0) / (1.0 + e);
    const lpfloat hvx = h * v.x;
    const lpfloat hvz = h * v.z;
    const lpfloat hvxy = hvx * v.y;
    const lpfloat hvxz = hvx * v.z;
    const lpfloat hvyz = hvz * v.y;
    lpfloat3x3 mtx;
    mtx[0][0] = e + hvx * v.x;
    mtx[0][1] = hvxy - v.z;
    mtx[0][2] = hvxz + v.y;
    mtx[1][0] = hvxy + v.z;
    mtx[1][1] = e + h * v.y * v.y;
    mtx[1][2] = hvyz - v.x;
    mtx[2][0] = hvxz - v.y;
    mtx[2][1] = hvyz + v.x;
    mtx[2][2] = e + hvz * v.z;
    return mtx;
}

void XeGTAO_MainPass(const uint2 pixCoord, lpfloat sliceCount, lpfloat stepsPerSlice, const lpfloat2 localNoise, lpfloat3 viewspaceNormal, const GTAOConstants consts,
    Texture2D<lpfloat> sourceViewspaceDepth, SamplerState depthSampler, RWTexture2D<uint> outWorkingAOTerm, RWTexture2D<unorm float> outWorkingEdges) {
    float2 normalizedScreenPos = (pixCoord + float2(0.5, 0.5)) * consts.ViewportPixelSize;

    lpfloat4 valuesUL = sourceViewspaceDepth.GatherRed(depthSampler, float2(pixCoord * consts.ViewportPixelSize));
    lpfloat4 valuesBR = sourceViewspaceDepth.GatherRed(depthSampler, float2(pixCoord * consts.ViewportPixelSize), int2(1, 1));

    lpfloat viewspaceZ = valuesUL.y;
    const lpfloat pixLZ = valuesUL.x;
    const lpfloat pixTZ = valuesUL.z;
    const lpfloat pixRZ = valuesBR.z;
    const lpfloat pixBZ = valuesBR.x;

    lpfloat4 edgesLRTB = XeGTAO_CalculateEdges((lpfloat)viewspaceZ, (lpfloat)pixLZ, (lpfloat)pixRZ, (lpfloat)pixTZ, (lpfloat)pixBZ);
    outWorkingEdges[pixCoord] = XeGTAO_PackEdges(edgesLRTB);

    viewspaceZ *= 0.99920;
    const float3 pixCenterPos = XeGTAO_ComputeViewspacePosition(normalizedScreenPos, viewspaceZ, consts);
    const lpfloat3 viewVec = (lpfloat3)normalize(-pixCenterPos);

    const lpfloat effectRadius = (lpfloat)consts.EffectRadius * (lpfloat)consts.RadiusMultiplier;
    const lpfloat sampleDistributionPower = (lpfloat)consts.SampleDistributionPower;
    const lpfloat thinOccluderCompensation = (lpfloat)consts.ThinOccluderCompensation;
    const lpfloat falloffRange = (lpfloat)consts.EffectFalloffRange * effectRadius;
    const lpfloat falloffFrom = effectRadius * ((lpfloat)1 - (lpfloat)consts.EffectFalloffRange);
    const lpfloat falloffMul = (lpfloat)-1.0 / (falloffRange);
    const lpfloat falloffAdd = falloffFrom / (falloffRange) + (lpfloat)1.0;

    lpfloat visibility = 0;
    lpfloat3 bentNormal = 0;

    {
        const lpfloat noiseSlice = (lpfloat)localNoise.x;
        const lpfloat noiseSample = (lpfloat)localNoise.y;
        const lpfloat pixelTooCloseThreshold = 1.3;
        const float2 pixelDirRBViewspaceSizeAtCenterZ = float2(viewspaceZ, viewspaceZ) * consts.NDCToViewMul_x_PixelSize;
        lpfloat screenspaceRadius = effectRadius / (lpfloat)pixelDirRBViewspaceSizeAtCenterZ.x;
        visibility += saturate((10 - screenspaceRadius) / 100) * 0.5;
        const lpfloat minS = (lpfloat)pixelTooCloseThreshold / screenspaceRadius;

        for (lpfloat slice = 0; slice < sliceCount; slice++) {
            lpfloat sliceK = (slice + noiseSlice) / sliceCount;
            lpfloat phi = sliceK * XE_GTAO_PI;
            lpfloat cosPhi = cos(phi);
            lpfloat sinPhi = sin(phi);
            lpfloat2 omega = lpfloat2(cosPhi, -sinPhi);
            omega *= screenspaceRadius;
            const lpfloat3 directionVec = lpfloat3(cosPhi, sinPhi, 0);
            const lpfloat3 orthoDirectionVec = directionVec - (dot(directionVec, viewVec) * viewVec);
            const lpfloat3 axisVec = normalize(cross(orthoDirectionVec, viewVec));
            lpfloat3 projectedNormalVec = viewspaceNormal - axisVec * dot(viewspaceNormal, axisVec);
            lpfloat signNorm = (lpfloat)sign(dot(orthoDirectionVec, projectedNormalVec));
            lpfloat projectedNormalVecLength = length(projectedNormalVec);
            lpfloat cosNorm = (lpfloat)saturate(dot(projectedNormalVec, viewVec) / projectedNormalVecLength);
            lpfloat n = signNorm * XeGTAO_FastACos(cosNorm);
            const lpfloat lowHorizonCos0 = cos(n + XE_GTAO_PI_HALF);
            const lpfloat lowHorizonCos1 = cos(n - XE_GTAO_PI_HALF);
            lpfloat horizonCos0 = lowHorizonCos0;
            lpfloat horizonCos1 = lowHorizonCos1;

            for (lpfloat step = 0; step < stepsPerSlice; step++) {
                const lpfloat stepBaseNoise = lpfloat(slice + step * stepsPerSlice) * 0.6180339887498948482;
                lpfloat stepNoise = frac(noiseSample + stepBaseNoise);
                lpfloat s = (step + stepNoise) / (stepsPerSlice);
                s = (lpfloat)pow(s, (lpfloat)sampleDistributionPower);
                s += minS;
                lpfloat2 sampleOffset = s * omega;
                lpfloat sampleOffsetLength = length(sampleOffset);
                const lpfloat mipLevel = (lpfloat)clamp(log2(sampleOffsetLength) - consts.DepthMIPSamplingOffset, 0, XE_GTAO_DEPTH_MIP_LEVELS);
                sampleOffset = round(sampleOffset) * (lpfloat2)consts.ViewportPixelSize;

                float2 sampleScreenPos0 = normalizedScreenPos + sampleOffset;
                float SZ0 = sourceViewspaceDepth.SampleLevel(depthSampler, sampleScreenPos0, mipLevel).x;
                float3 samplePos0 = XeGTAO_ComputeViewspacePosition(sampleScreenPos0, SZ0, consts);
                float2 sampleScreenPos1 = normalizedScreenPos - sampleOffset;
                float SZ1 = sourceViewspaceDepth.SampleLevel(depthSampler, sampleScreenPos1, mipLevel).x;
                float3 samplePos1 = XeGTAO_ComputeViewspacePosition(sampleScreenPos1, SZ1, consts);
                float3 sampleDelta0 = (samplePos0 - float3(pixCenterPos));
                float3 sampleDelta1 = (samplePos1 - float3(pixCenterPos));
                lpfloat sampleDist0 = (lpfloat)length(sampleDelta0);
                lpfloat sampleDist1 = (lpfloat)length(sampleDelta1);
                lpfloat3 sampleHorizonVec0 = (lpfloat3)(sampleDelta0 / sampleDist0);
                lpfloat3 sampleHorizonVec1 = (lpfloat3)(sampleDelta1 / sampleDist1);
                lpfloat falloffBase0 = length(lpfloat3(sampleDelta0.x, sampleDelta0.y, sampleDelta0.z * (1 + thinOccluderCompensation)));
                lpfloat falloffBase1 = length(lpfloat3(sampleDelta1.x, sampleDelta1.y, sampleDelta1.z * (1 + thinOccluderCompensation)));
                lpfloat weight0 = saturate(falloffBase0 * falloffMul + falloffAdd);
                lpfloat weight1 = saturate(falloffBase1 * falloffMul + falloffAdd);
                lpfloat shc0 = (lpfloat)dot(sampleHorizonVec0, viewVec);
                lpfloat shc1 = (lpfloat)dot(sampleHorizonVec1, viewVec);
                shc0 = lerp(lowHorizonCos0, shc0, weight0);
                shc1 = lerp(lowHorizonCos1, shc1, weight1);
                horizonCos0 = max(horizonCos0, shc0);
                horizonCos1 = max(horizonCos1, shc1);
            }

            projectedNormalVecLength = lerp(projectedNormalVecLength, 1, 0.05);
            lpfloat h0 = -XeGTAO_FastACos((lpfloat)horizonCos1);
            lpfloat h1 = XeGTAO_FastACos((lpfloat)horizonCos0);
            lpfloat iarc0 = ((lpfloat)cosNorm + (lpfloat)2 * (lpfloat)h0 * (lpfloat)sin(n) - (lpfloat)cos((lpfloat)2 * (lpfloat)h0 - n)) / (lpfloat)4;
            lpfloat iarc1 = ((lpfloat)cosNorm + (lpfloat)2 * (lpfloat)h1 * (lpfloat)sin(n) - (lpfloat)cos((lpfloat)2 * (lpfloat)h1 - n)) / (lpfloat)4;
            lpfloat localVisibility = (lpfloat)projectedNormalVecLength * (lpfloat)(iarc0 + iarc1);
            visibility += localVisibility;

            lpfloat t0 = (6 * sin(h0 - n) - sin(3 * h0 - n) + 6 * sin(h1 - n) - sin(3 * h1 - n) + 16 * sin(n) - 3 * (sin(h0 + n) + sin(h1 + n))) / 12;
            lpfloat t1 = (-cos(3 * h0 - n) - cos(3 * h1 - n) + 8 * cos(n) - 3 * (cos(h0 + n) + cos(h1 + n))) / 12;
            lpfloat3 localBentNormal = lpfloat3(directionVec.x * (lpfloat)t0, directionVec.y * (lpfloat)t0, -lpfloat(t1));
            localBentNormal = (lpfloat3)mul(XeGTAO_RotFromToMatrix(lpfloat3(0, 0, -1), viewVec), localBentNormal) * projectedNormalVecLength;
            bentNormal += localBentNormal;
        }
        visibility /= (lpfloat)sliceCount;
        visibility = pow(visibility, (lpfloat)consts.FinalValuePower);
        visibility = max((lpfloat)0.03, visibility);
        bentNormal = normalize(bentNormal);
    }

    XeGTAO_OutputWorkingTerm(pixCoord, visibility, bentNormal, outWorkingAOTerm);
}

lpfloat XeGTAO_DepthMIPFilter(lpfloat depth0, lpfloat depth1, lpfloat depth2, lpfloat depth3, const GTAOConstants consts) {
    lpfloat maxDepth = max(max(depth0, depth1), max(depth2, depth3));
    const lpfloat depthRangeScaleFactor = 0.75;
    const lpfloat effectRadius = depthRangeScaleFactor * (lpfloat)consts.EffectRadius * (lpfloat)consts.RadiusMultiplier;
    const lpfloat falloffRange = (lpfloat)consts.EffectFalloffRange * effectRadius;
    const lpfloat falloffFrom = effectRadius * ((lpfloat)1 - (lpfloat)consts.EffectFalloffRange);
    const lpfloat falloffMul = (lpfloat)-1.0 / (falloffRange);
    const lpfloat falloffAdd = falloffFrom / (falloffRange) + (lpfloat)1.0;
    lpfloat weight0 = saturate((maxDepth - depth0) * falloffMul + falloffAdd);
    lpfloat weight1 = saturate((maxDepth - depth1) * falloffMul + falloffAdd);
    lpfloat weight2 = saturate((maxDepth - depth2) * falloffMul + falloffAdd);
    lpfloat weight3 = saturate((maxDepth - depth3) * falloffMul + falloffAdd);
    lpfloat weightSum = weight0 + weight1 + weight2 + weight3;
    return (weight0 * depth0 + weight1 * depth1 + weight2 * depth2 + weight3 * depth3) / weightSum;
}

lpfloat XeGTAO_ClampDepth(float depth) {
    return clamp(depth, 0.0, 3.402823466e+38);
}

groupshared lpfloat g_scratchDepths[8][8];
void XeGTAO_PrefilterDepths16x16(uint2 dispatchThreadID, uint2 groupThreadID, const GTAOConstants consts, Texture2D<float> sourceNDCDepth, SamplerState depthSampler,
    RWTexture2D<lpfloat> outDepth0, RWTexture2D<lpfloat> outDepth1, RWTexture2D<lpfloat> outDepth2, RWTexture2D<lpfloat> outDepth3, RWTexture2D<lpfloat> outDepth4) {
    const uint2 baseCoord = dispatchThreadID;
    const uint2 pixCoord = baseCoord * 2;
    uint outDepth0Width, outDepth0Height;
    uint outDepth1Width, outDepth1Height;
    uint outDepth2Width, outDepth2Height;
    uint outDepth3Width, outDepth3Height;
    uint outDepth4Width, outDepth4Height;
    outDepth0.GetDimensions(outDepth0Width, outDepth0Height);
    outDepth1.GetDimensions(outDepth1Width, outDepth1Height);
    outDepth2.GetDimensions(outDepth2Width, outDepth2Height);
    outDepth3.GetDimensions(outDepth3Width, outDepth3Height);
    outDepth4.GetDimensions(outDepth4Width, outDepth4Height);
    const uint2 outDepth0SafeSize = uint2(max(outDepth0Width, 1u), max(outDepth0Height, 1u));
    const uint2 clampedPixCoord = min(pixCoord, outDepth0SafeSize - 1u);
    float4 depths4 = sourceNDCDepth.GatherRed(depthSampler, float2(clampedPixCoord * consts.ViewportPixelSize), int2(1, 1));
    lpfloat depth0 = XeGTAO_ClampDepth(XeGTAO_ScreenSpaceToViewSpaceDepth(depths4.w, consts));
    lpfloat depth1 = XeGTAO_ClampDepth(XeGTAO_ScreenSpaceToViewSpaceDepth(depths4.z, consts));
    lpfloat depth2 = XeGTAO_ClampDepth(XeGTAO_ScreenSpaceToViewSpaceDepth(depths4.x, consts));
    lpfloat depth3 = XeGTAO_ClampDepth(XeGTAO_ScreenSpaceToViewSpaceDepth(depths4.y, consts));
    if (pixCoord.x < outDepth0Width && pixCoord.y < outDepth0Height)
        outDepth0[pixCoord + uint2(0, 0)] = (lpfloat)depth0;
    if ((pixCoord.x + 1) < outDepth0Width && pixCoord.y < outDepth0Height)
        outDepth0[pixCoord + uint2(1, 0)] = (lpfloat)depth1;
    if (pixCoord.x < outDepth0Width && (pixCoord.y + 1) < outDepth0Height)
        outDepth0[pixCoord + uint2(0, 1)] = (lpfloat)depth2;
    if ((pixCoord.x + 1) < outDepth0Width && (pixCoord.y + 1) < outDepth0Height)
        outDepth0[pixCoord + uint2(1, 1)] = (lpfloat)depth3;
    lpfloat dm1 = XeGTAO_DepthMIPFilter(depth0, depth1, depth2, depth3, consts);
    if (baseCoord.x < outDepth1Width && baseCoord.y < outDepth1Height)
        outDepth1[baseCoord] = (lpfloat)dm1;
    g_scratchDepths[groupThreadID.x][groupThreadID.y] = dm1;
    GroupMemoryBarrierWithGroupSync();
    if (all((groupThreadID.xy % uint2(2, 2)) == uint2(0, 0))) {
        lpfloat inTL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 0];
        lpfloat inTR = g_scratchDepths[groupThreadID.x + 1][groupThreadID.y + 0];
        lpfloat inBL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 1];
        lpfloat inBR = g_scratchDepths[groupThreadID.x + 1][groupThreadID.y + 1];
        lpfloat dm2 = XeGTAO_DepthMIPFilter(inTL, inTR, inBL, inBR, consts);
        if ((baseCoord.x / 2) < outDepth2Width && (baseCoord.y / 2) < outDepth2Height)
            outDepth2[baseCoord / 2] = (lpfloat)dm2;
        g_scratchDepths[groupThreadID.x][groupThreadID.y] = dm2;
    }
    GroupMemoryBarrierWithGroupSync();
    if (all((groupThreadID.xy % uint2(4, 4)) == uint2(0, 0))) {
        lpfloat inTL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 0];
        lpfloat inTR = g_scratchDepths[groupThreadID.x + 2][groupThreadID.y + 0];
        lpfloat inBL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 2];
        lpfloat inBR = g_scratchDepths[groupThreadID.x + 2][groupThreadID.y + 2];
        lpfloat dm3 = XeGTAO_DepthMIPFilter(inTL, inTR, inBL, inBR, consts);
        if ((baseCoord.x / 4) < outDepth3Width && (baseCoord.y / 4) < outDepth3Height)
            outDepth3[baseCoord / 4] = (lpfloat)dm3;
        g_scratchDepths[groupThreadID.x][groupThreadID.y] = dm3;
    }
    GroupMemoryBarrierWithGroupSync();
    if (all((groupThreadID.xy % uint2(8, 8)) == uint2(0, 0))) {
        lpfloat inTL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 0];
        lpfloat inTR = g_scratchDepths[groupThreadID.x + 4][groupThreadID.y + 0];
        lpfloat inBL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 4];
        lpfloat inBR = g_scratchDepths[groupThreadID.x + 4][groupThreadID.y + 4];
        lpfloat dm4 = XeGTAO_DepthMIPFilter(inTL, inTR, inBL, inBR, consts);
        if ((baseCoord.x / 8) < outDepth4Width && (baseCoord.y / 8) < outDepth4Height)
            outDepth4[baseCoord / 8] = (lpfloat)dm4;
    }
}

lpfloat4 XeGTAO_UnpackEdges(lpfloat _packedVal) {
    uint packedVal = (uint)(_packedVal * 255.5);
    lpfloat4 edgesLRTB;
    edgesLRTB.x = lpfloat((packedVal >> 6) & 0x03) / 3.0;
    edgesLRTB.y = lpfloat((packedVal >> 4) & 0x03) / 3.0;
    edgesLRTB.z = lpfloat((packedVal >> 2) & 0x03) / 3.0;
    edgesLRTB.w = lpfloat((packedVal >> 0) & 0x03) / 3.0;
    return saturate(edgesLRTB);
}

typedef lpfloat4 AOTermType;

void XeGTAO_AddSample(AOTermType ssaoValue, lpfloat edgeValue, inout AOTermType sum, inout lpfloat sumWeight) {
    lpfloat weight = edgeValue;
    sum += (weight * ssaoValue);
    sumWeight += weight;
}

void XeGTAO_Output(uint2 pixCoord, RWTexture2D<uint> outputTexture, AOTermType outputValue, const uniform bool finalApply) {
    lpfloat visibility = outputValue.w * ((finalApply) ? ((lpfloat)XE_GTAO_OCCLUSION_TERM_SCALE) : (1));
    lpfloat3 bentNormal = normalize(outputValue.xyz);
    outputTexture[pixCoord.xy] = XeGTAO_EncodeVisibilityBentNormal(visibility, bentNormal);
}

void XeGTAO_DecodeGatherPartial(const uint4 packedValue, out AOTermType outDecoded[4]) {
    for (int i = 0; i < 4; i++)
        XeGTAO_DecodeVisibilityBentNormal(packedValue[i], outDecoded[i].w, outDecoded[i].xyz);
}

void XeGTAO_Denoise(const uint2 pixCoordBase, const GTAOConstants consts, Texture2D<uint> sourceAOTerm, Texture2D<lpfloat> sourceEdges,
    SamplerState texSampler, RWTexture2D<uint> outputTexture, const uniform bool finalApply) {
    uint sourceWidth, sourceHeight;
    uint outputWidth, outputHeight;
    sourceAOTerm.GetDimensions(sourceWidth, sourceHeight);
    outputTexture.GetDimensions(outputWidth, outputHeight);
    if (sourceWidth == 0 || sourceHeight == 0 || outputWidth == 0 || outputHeight == 0)
        return;
    if (pixCoordBase.x >= sourceWidth || pixCoordBase.y >= sourceHeight)
        return;

    const lpfloat blurAmount = (finalApply) ? ((lpfloat)consts.DenoiseBlurBeta) : ((lpfloat)consts.DenoiseBlurBeta / (lpfloat)5.0);
    const lpfloat diagWeight = 0.85 * 0.5;

    AOTermType aoTerm[2];
    lpfloat4 edgesC_LRTB[2];
    lpfloat weightTL[2], weightTR[2], weightBL[2], weightBR[2];

    const float2 gatherCenter = float2(pixCoordBase.x, pixCoordBase.y) * consts.ViewportPixelSize;
    lpfloat4 edgesQ0 = sourceEdges.GatherRed(texSampler, gatherCenter, int2(0, 0));
    lpfloat4 edgesQ1 = sourceEdges.GatherRed(texSampler, gatherCenter, int2(2, 0));
    lpfloat4 edgesQ2 = sourceEdges.GatherRed(texSampler, gatherCenter, int2(1, 2));

    AOTermType visQ0[4]; XeGTAO_DecodeGatherPartial(sourceAOTerm.GatherRed(texSampler, gatherCenter, int2(0, 0)), visQ0);
    AOTermType visQ1[4]; XeGTAO_DecodeGatherPartial(sourceAOTerm.GatherRed(texSampler, gatherCenter, int2(2, 0)), visQ1);
    AOTermType visQ2[4]; XeGTAO_DecodeGatherPartial(sourceAOTerm.GatherRed(texSampler, gatherCenter, int2(0, 2)), visQ2);
    AOTermType visQ3[4]; XeGTAO_DecodeGatherPartial(sourceAOTerm.GatherRed(texSampler, gatherCenter, int2(2, 2)), visQ3);

    for (int side = 0; side < 2; side++) {
        const int2 pixCoord = int2(pixCoordBase.x + side, pixCoordBase.y);
        if (pixCoord.x < 0 || pixCoord.y < 0 || uint(pixCoord.x) >= sourceWidth || uint(pixCoord.y) >= sourceHeight || uint(pixCoord.x) >= outputWidth || uint(pixCoord.y) >= outputHeight)
            continue;

        lpfloat4 edgesL_LRTB = XeGTAO_UnpackEdges((side == 0) ? (edgesQ0.x) : (edgesQ0.y));
        lpfloat4 edgesT_LRTB = XeGTAO_UnpackEdges((side == 0) ? (edgesQ0.z) : (edgesQ1.w));
        lpfloat4 edgesR_LRTB = XeGTAO_UnpackEdges((side == 0) ? (edgesQ1.x) : (edgesQ1.y));
        lpfloat4 edgesB_LRTB = XeGTAO_UnpackEdges((side == 0) ? (edgesQ2.w) : (edgesQ2.z));
        edgesC_LRTB[side] = XeGTAO_UnpackEdges((side == 0) ? (edgesQ0.y) : (edgesQ1.x));
        edgesC_LRTB[side] *= lpfloat4(edgesL_LRTB.y, edgesR_LRTB.x, edgesT_LRTB.w, edgesB_LRTB.z);
        const lpfloat leak_threshold = 2.5; const lpfloat leak_strength = 0.5;
        lpfloat edginess = (saturate(4.0 - leak_threshold - dot(edgesC_LRTB[side], lpfloat4(1.0, 1.0, 1.0, 1.0))) / (4 - leak_threshold)) * leak_strength;
        edgesC_LRTB[side] = saturate(edgesC_LRTB[side] + edginess);

        weightTL[side] = diagWeight * (edgesC_LRTB[side].x * edgesL_LRTB.z + edgesC_LRTB[side].z * edgesT_LRTB.x);
        weightTR[side] = diagWeight * (edgesC_LRTB[side].z * edgesT_LRTB.y + edgesC_LRTB[side].y * edgesR_LRTB.z);
        weightBL[side] = diagWeight * (edgesC_LRTB[side].w * edgesB_LRTB.x + edgesC_LRTB[side].x * edgesL_LRTB.w);
        weightBR[side] = diagWeight * (edgesC_LRTB[side].y * edgesR_LRTB.w + edgesC_LRTB[side].w * edgesB_LRTB.y);

        AOTermType ssaoValue = (side == 0) ? (visQ0[1]) : (visQ1[0]);
        AOTermType ssaoValueL = (side == 0) ? (visQ0[0]) : (visQ0[1]);
        AOTermType ssaoValueT = (side == 0) ? (visQ0[2]) : (visQ1[3]);
        AOTermType ssaoValueR = (side == 0) ? (visQ1[0]) : (visQ1[1]);
        AOTermType ssaoValueB = (side == 0) ? (visQ2[2]) : (visQ3[3]);
        AOTermType ssaoValueTL = (side == 0) ? (visQ0[3]) : (visQ0[2]);
        AOTermType ssaoValueBR = (side == 0) ? (visQ3[3]) : (visQ3[2]);
        AOTermType ssaoValueTR = (side == 0) ? (visQ1[3]) : (visQ1[2]);
        AOTermType ssaoValueBL = (side == 0) ? (visQ2[3]) : (visQ2[2]);

        lpfloat sumWeight = blurAmount;
        AOTermType sum = ssaoValue * sumWeight;
        XeGTAO_AddSample(ssaoValueL, edgesC_LRTB[side].x, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueR, edgesC_LRTB[side].y, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueT, edgesC_LRTB[side].z, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueB, edgesC_LRTB[side].w, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueTL, weightTL[side], sum, sumWeight);
        XeGTAO_AddSample(ssaoValueTR, weightTR[side], sum, sumWeight);
        XeGTAO_AddSample(ssaoValueBL, weightBL[side], sum, sumWeight);
        XeGTAO_AddSample(ssaoValueBR, weightBR[side], sum, sumWeight);
        aoTerm[side] = sum / sumWeight;
        XeGTAO_Output(pixCoord, outputTexture, aoTerm[side], finalApply);
    }
}

float3 XeGTAO_ComputeViewspaceNormal(const uint2 pixCoord, const GTAOConstants consts, Texture2D<float> sourceNDCDepth, SamplerState depthSampler) {
    float2 normalizedScreenPos = (pixCoord + float2(0.5, 0.5)) * consts.ViewportPixelSize;
    float4 valuesUL = sourceNDCDepth.GatherRed(depthSampler, float2(pixCoord * consts.ViewportPixelSize));
    float4 valuesBR = sourceNDCDepth.GatherRed(depthSampler, float2(pixCoord * consts.ViewportPixelSize), int2(1, 1));
    float viewspaceZ = XeGTAO_ScreenSpaceToViewSpaceDepth(valuesUL.y, consts);
    const float pixLZ = XeGTAO_ScreenSpaceToViewSpaceDepth(valuesUL.x, consts);
    const float pixTZ = XeGTAO_ScreenSpaceToViewSpaceDepth(valuesUL.z, consts);
    const float pixRZ = XeGTAO_ScreenSpaceToViewSpaceDepth(valuesBR.z, consts);
    const float pixBZ = XeGTAO_ScreenSpaceToViewSpaceDepth(valuesBR.x, consts);
    lpfloat4 edgesLRTB = XeGTAO_CalculateEdges((lpfloat)viewspaceZ, (lpfloat)pixLZ, (lpfloat)pixRZ, (lpfloat)pixTZ, (lpfloat)pixBZ);
    float3 CENTER = XeGTAO_ComputeViewspacePosition(normalizedScreenPos, viewspaceZ, consts);
    float3 LEFT = XeGTAO_ComputeViewspacePosition(normalizedScreenPos + float2(-1, 0) * consts.ViewportPixelSize, pixLZ, consts);
    float3 RIGHT = XeGTAO_ComputeViewspacePosition(normalizedScreenPos + float2(1, 0) * consts.ViewportPixelSize, pixRZ, consts);
    float3 TOP = XeGTAO_ComputeViewspacePosition(normalizedScreenPos + float2(0, -1) * consts.ViewportPixelSize, pixTZ, consts);
    float3 BOTTOM = XeGTAO_ComputeViewspacePosition(normalizedScreenPos + float2(0, 1) * consts.ViewportPixelSize, pixBZ, consts);
    return XeGTAO_CalculateNormal(edgesLRTB, CENTER, LEFT, RIGHT, TOP, BOTTOM);
}