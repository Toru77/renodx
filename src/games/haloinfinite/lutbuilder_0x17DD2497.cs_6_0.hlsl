#include "./common.hlsl"

struct LutParameters
{
    float4 scale;
    uint   texIndex;
    uint3  pad;
};

Texture3D<float4> srv3DTextures[65536] : register(t0, space2);
RWTexture3D<float4> hdrColorGrade : register(u0);

cbuffer cbPostprocessParameters : register(b5)
{
    float4 ps_pixel_size                     : packoffset(c0.x);
    float4 ps_scale                          : packoffset(c1.x);
    float4 ps_intensity                      : packoffset(c2.x);
    float4 psScreenSize                      : packoffset(c3.x);
    float4 psPreGradeTonemapAndPaperWhite    : packoffset(c4.x);
    float4 psHdrDebugParams                  : packoffset(c5.x);
    float4 psHdrDebugColor                   : packoffset(c6.x);
    float4 psViewportBounds                  : packoffset(c7.x);
};

cbuffer cbPostprocessColorGradingParameters : register(b2)
{
    LutParameters psLutParameters[8]        : packoffset(c0.x);
    float4 psHueSaturationMatrix[3]         : packoffset(c16.x);
    float4 psGamma                          : packoffset(c19.x);
    float  psLutLerp                        : packoffset(c20.x);
    float  psLutLerpDv                      : packoffset(c20.y);
    float  psSdrGamma                       : packoffset(c20.z);
    float  psPaperWhite                     : packoffset(c20.w);
    float  psBrightnessAdjustment           : packoffset(c21.x);
    float3 psPad                            : packoffset(c21.y);
    float3 psColorblindTransform[3]         : packoffset(c22.x);
};

cbuffer cbPostprocessDWSColorGradingParameters : register(b6)
{
    float  dwsGradingWeight        : packoffset(c0.x);
    float  dwsTwoToExposure        : packoffset(c0.y);
    float  dwsSaturation           : packoffset(c0.z);
    float  dwsContrast             : packoffset(c0.w);
    float3 dwsLift                 : packoffset(c1.x);
    float  dwsContrastMidpoint     : packoffset(c1.w);
    float3 dwsOneOverGamma         : packoffset(c2.x);
    float3 dwsGain                 : packoffset(c3.x);
};

SamplerState g_staticSampler_TrilinearClamp : register(s20);

// DXIL FirstbitHi: returns bit position counting from MSB (leading zeros count)
uint firstbithigh_msb(int value)  { return (value == 0) ? 0xFFFFFFFFu : (31u - firstbithigh(value)); }
uint firstbithigh_msb(uint value) { return (value == 0) ? 0xFFFFFFFFu : (31u - firstbithigh(value)); }

uint ResolveBindlessIndex(uint rawIndex, int selectFallback)
{
    int  heapBits   = (int)rawIndex & (-1048576);
    bool isSentinel = (heapBits == (-1018167296));
    int  localIndex = (int)rawIndex & 1048575;
    return select(isSentinel, localIndex, selectFallback);
}

[numthreads(8, 8, 1)]
void main(
    uint3 SV_DispatchThreadID : SV_DispatchThreadID,
    uint3 SV_GroupID : SV_GroupID,
    uint3 SV_GroupThreadID : SV_GroupThreadID,
    uint  SV_GroupIndex : SV_GroupIndex)
{
    // ---- LUT grid coordinate (32-cell grid, treated as a PQ value) -------

    float3 grid = float3(SV_DispatchThreadID) * 0.032258063554763794f;

    // ---- Decode grid coord (PQ) -> linear -> round-trip back through a   --
    // ---- full PQ encode, then decode again (preserved exactly as found) --

    float preGradePaperWhite = psPreGradeTonemapAndPaperWhite.z;

    float3 linear0 = renodx::color::pq::Decode(grid, preGradePaperWhite);

    float3 reencoded = renodx::color::pq::Encode(linear0, preGradePaperWhite);

    // ---- Exposure-adjusted linear values ----------------------------------

    float3 exposure = dwsTwoToExposure * renodx::color::pq::Decode(reencoded, preGradePaperWhite);

    float luma = dot(exposure, float3(0.25f, 0.5f, 0.25f));
    float lumaEps = luma + 9.999999747378752e-06f;

    // ---- DWS grading: saturation -> log contrast -> lift -> gain -> gamma --

    float3 diff = exposure - luma;
    float3 satAdj = lumaEps + dwsSaturation * diff;

    float3 ln = log2(satAdj) * 0.6931471824645996f;
    float3 contrasted0 = (ln - dwsContrastMidpoint) * dwsContrast + dwsContrastMidpoint;
    float3 exped = exp2(contrasted0 * 1.4426950216293335f) + -9.999999747378752e-06f;

    satAdj = pow(satAdj / dwsContrastMidpoint, dwsContrast) * dwsContrastMidpoint;

    float3 clamped = exped; //max(0.0f.xxx, exped);
    float3 lifted = (1.0f.xxx - dwsLift) * clamped + dwsLift;
    float3 gained = lifted * dwsGain;
    float3 gamma = pow(gained, dwsOneOverGamma);

    float scale = renodx::tonemap::neutwo::ComputeMaxChannelScale(gamma);
    gamma *= scale;

    // ---- Sample LUT slot 2: coords = ungraded value, blended by --------
    // ---- dwsGradingWeight toward the freshly graded+PQ-encoded value ---

    uint lut2TexIndex = ResolveBindlessIndex(psLutParameters[2].texIndex, 18);

    float3 gradedFinished = saturate(renodx::color::pq::Encode(gamma, preGradePaperWhite));

    float3 gradeDelta = (gradedFinished - reencoded) * dwsGradingWeight;
    float3 lut2Coord0 = (reencoded + -0.5f) + gradeDelta; // _280.._285
    float3 lut2Coord = lut2Coord0 * psLutParameters[2].scale.xyz + 0.5f;

    float4 lut2Sample = srv3DTextures[lut2TexIndex].SampleLevel(g_staticSampler_TrilinearClamp, lut2Coord, 0.0f);

    // ---- Sample LUT slot 4 using the slot-2 result as input coordinates --

    float3 lut4CoordOff = lut2Sample.rgb + -0.5f;

    uint lut4TexIndex = ResolveBindlessIndex(psLutParameters[4].texIndex, 18);

    float3 lut4Coord = lut4CoordOff * psLutParameters[4].scale.xyz + 0.5f;

    float4 lut4Sample = srv3DTextures[lut4TexIndex].SampleLevel(g_staticSampler_TrilinearClamp, lut4Coord, 0.0f);

    // ---- RenoDX PQ-decode LUT slot 4 result ------------------------------

    float3 linear1 = renodx::color::pq::Decode(lut4Sample.rgb, preGradePaperWhite);

    // ---- RGB -> CIE XYZ (Rec.709/sRGB primaries) --------------------------

    float xyzX = (linear1.g * 0.35760000348091125f + linear1.r * 0.4124000072479248f) + linear1.b * 0.18050000071525574f;
    float xyzY = (linear1.g * 0.7152000069618225f + linear1.r * 0.2125999927520752f) + linear1.b * 0.0722000002861023f;
    float xyzZ = (linear1.g * 0.11919999867677689f + linear1.r * 0.019300000742077827f) + linear1.b * 0.9505000114440918f;
    float xyzSum = ((xyzZ) + xyzY) + xyzX; // _379

    // ---- xyY chromaticity split, with a safe fallback near black ---------

    bool  xyzSumTiny = (xyzSum < 9.999999747378752e-06f);
    float safeChromaX = select(xyzSumTiny, 0.3127000033855438f, (xyzX / xyzSum));
    float safeChromaY = select(xyzSumTiny, 0.32899999618530273f, (xyzY / xyzSum));

    // ---- Gamma curve applied to luminance, then back through chromaticity -

    float gradedY = pow(max(0.0f, xyzY) * 0.4300000071525574f, psSdrGamma);
    float yOverChromaY = gradedY / safeChromaY; // _390
    float gradedX = yOverChromaY * safeChromaX; // _391
    float gradedZ = ((1.0f - safeChromaX) - safeChromaY) * yOverChromaY;

    // ---- CIE XYZ -> RGB (Rec.709/sRGB primaries) --------------------------

    float3 graded;
    graded.r = (gradedX * 3.240600109100342f - gradedY * 1.5371999740600586f) - gradedZ * 0.4986000061035156f; // _399
    graded.g = (gradedY * 1.8758000135421753f - gradedX * 0.9689000248908997f) + gradedZ * 0.04149999842047691f; // _404
    graded.b = (gradedX * 0.05570000037550926f - gradedY * 0.20399999618530273f) + gradedZ * 1.0570000410079956f; // _409

    // ---- Per-channel soft-knee rolloff above 0.65 -------------------------

    //bool3 belowKnee = (graded < 0.6499999761581421f); // _410/_420/_430
    //float3 rolledAboveKnee = (1.0f.xxx - exp2((graded + -0.6499999761581421f) * -3.205988645553589f)) * 0.4500000476837158f + 0.6499999761581421f;
    //float3 rolled = select(belowKnee, graded, rolledAboveKnee); // _419/_429/_439

    // ---- Clamp before PQ re-encode ---------------------------------------

    float3 gradedClamped = max(0.0f.xxx, graded);

    // ---- Sample LUT slot 6 using the SAME coords as LUT slot 4 (reused) --

    uint lut6TexIndex = ResolveBindlessIndex(psLutParameters[6].texIndex, 18);

    float3 lut6Coord = psLutParameters[6].scale.xyz * lut4CoordOff + 0.5f;

    float4 lut6Sample = srv3DTextures[lut6TexIndex].SampleLevel(g_staticSampler_TrilinearClamp, lut6Coord, 0.0f); // _474

    // ---- Blend: either a direct LUT6-vs-LUT4 lerp, or graded-vs-LUT4 -----

    float3 delta;

    bool useDirectLutLerp = (psLutLerp > 0.0f);
    if (useDirectLutLerp)
    {
        delta = psLutLerp * (lut6Sample.rgb - lut4Sample.rgb);
    }
    else
    {
        float3 pqFinal = renodx::color::pq::Encode(gradedClamped, preGradePaperWhite);

        float negLutLerp = -0.0f - psLutLerp;

        delta = (pqFinal - lut4Sample.rgb) * negLutLerp;
    }

    float3 final0 = delta + lut4Sample.rgb;

    // ---- HDR debug override: substitute the LUT slot 2 result -----------

    bool isHdrDebug = (psHdrDebugParams.w == 1.0f);
    float3 debug = select(isHdrDebug.xxx, lut2Sample.rgb, final0);

    // ---- Hue/saturation 3x4 matrix transform ------------------------------

    float3 hueSat;
    hueSat.r = mad(debug.b, psHueSaturationMatrix[0].z, mad(debug.g, psHueSaturationMatrix[0].y, debug.r * psHueSaturationMatrix[0].x)) + psHueSaturationMatrix[0].w;
    hueSat.g = mad(debug.b, psHueSaturationMatrix[1].z, mad(debug.g, psHueSaturationMatrix[1].y, debug.r * psHueSaturationMatrix[1].x)) + psHueSaturationMatrix[1].w;
    hueSat.b = mad(debug.b, psHueSaturationMatrix[2].z, mad(debug.g, psHueSaturationMatrix[2].y, psHueSaturationMatrix[2].x * debug.r)) + psHueSaturationMatrix[2].w;

    // ---- Luma-preserving final gamma normalization (psGamma.w) -----------

    float finalLuma = dot(hueSat, float3(0.3330000042915344f, 0.3330000042915344f, 0.3330000042915344f)); // _563
    float finalLumaClamped = max(9.999999747378752e-06f, finalLuma);
    float lumaGammaMul = pow(finalLumaClamped, psGamma.w);

    float3 out_ = lumaGammaMul * hueSat;

    out_ = renodx::color::pq::Decode(out_, preGradePaperWhite);
    out_ = out_ / scale;
    out_ = renodx::color::pq::Encode(out_, preGradePaperWhite);

    hdrColorGrade[int3((int)SV_DispatchThreadID.x, (int)SV_DispatchThreadID.y, (int)SV_DispatchThreadID.z)] = float4(out_, 1.0f);
}
