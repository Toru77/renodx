#ifndef FASTNOISE_HLSL
#define FASTNOISE_HLSL

/*
===============================================================================
FAST / IS-FAST NOISE DESIGN, USAGE AND IMPLEMENTATION
===============================================================================

This file documents and provides helper utilities for using FAST
(spatiotemporal blue noise) textures in real-time rendering and includes
IS-FAST helpers for importance-sampled, filter-adapted spatio-temporal noise.

Typical FAST texture
--------------------

Resolution: 128 x 128 x 32
Format:     RG8_UNORM
Channels:   two independent uniform random numbers

Example sample:

    float2 xi = noise.xy;

The noise texture is tiled spatially and temporally.

===============================================================================
INDEXING RULE
===============================================================================

If rendering pixel (px, py) at frame f with texture size W x H x D:

    noise(px % W, py % H, f % D)

Example for 128x128x32:

    float2 xi = noiseTex.SampleLevel(
        sampler,
        float3(px & 127, py & 127, frame & 31),
        0);

The texture is optimized in toroidal space so wrapping works well.

===============================================================================
MULTIPLE SAMPLES PER FRAME
===============================================================================

To obtain N samples per pixel per frame:

    noise((px,py) + R2(index), frame)

This simulates multiple independent noise textures.

===============================================================================
TEMPORAL SAMPLE EXTENSION
===============================================================================

If more than D temporal samples are needed:

    noise((x + offset) % W,
          (y + offset) % H,
          sample % D)

Where offset is generated from RNG(sample / D).

This introduces a discontinuity every D frames but increases sample count.

===============================================================================
DISTRIBUTION CHEAT TABLE
===============================================================================

Effect / Algorithm                Distribution
---------------------------------------------------------------
SSAO                              Cosine hemisphere
SSGI / diffuse GI                 Cosine hemisphere
Diffuse path tracing              Cosine hemisphere

SSR reflections                   GGX / VNDF
Glossy reflections                GGX
Specular reflections              GGX

Volumetric fog                    Henyey-Greenstein
Cloud rendering                   Henyey-Greenstein
Atmospheric scattering            Rayleigh + HG

Soft shadows (PCSS)               Disk / Poisson disk
PCF stochastic shadows            Uniform disk
Contact shadows                   Uniform disk

Depth of field                    Uniform disk
Motion blur                       Uniform time

Stochastic transparency           Uniform threshold
LOD dithering                     Blue noise

Raymarch jitter                   Uniform offset
ReSTIR sampling                   Light power distribution

===============================================================================
CONSTANTS
===============================================================================
*/

#define FAST_PI 3.14159265359
#define FAST_TAU 6.28318530718

/*
===============================================================================
LOW DISCREPANCY SEQUENCE
===============================================================================
Martin Roberts R2 sequence
Used for multi-sample offsets.

R2 source:
http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
*/

float2 FAST_R2(int index)
{
    const float g  = 1.32471795724474602596;
    const float a1 = 1.0 / g;
    const float a2 = 1.0 / (g * g);

    return float2(frac(index * a1), frac(index * a2));
}

/*
===============================================================================
SAMPLING HELPERS
===============================================================================
*/

float2 FAST_SampleDisk(float2 xi)
{
    float r = sqrt(xi.x);
    float theta = FAST_TAU * xi.y;

    return float2(
        r * cos(theta),
        r * sin(theta)
    );
}

float3 FAST_SampleUniformHemisphere(float2 xi)
{
    float phi = FAST_TAU * xi.x;

    float cosTheta = xi.y;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    return float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
}

float3 FAST_SampleCosineHemisphere(float2 xi)
{
    float phi = FAST_TAU * xi.x;

    float cosTheta = sqrt(1.0 - xi.y);
    float sinTheta = sqrt(xi.y);

    return float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
}

float3 FAST_SampleGGX(float2 xi, float roughness)
{
    float a = roughness * roughness;

    float phi = FAST_TAU * xi.x;

    float cosTheta =
        sqrt((1.0 - xi.y) /
        (1.0 + (a * a - 1.0) * xi.y));

    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    return float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
}

float3 FAST_SampleHenyeyGreenstein(float2 xi, float g)
{
    float cosTheta;

    if (abs(g) < 0.001)
        cosTheta = 1.0 - 2.0 * xi.x;
    else
    {
        float sqr =
            (1.0 - g * g) /
            (1.0 - g + 2.0 * g * xi.x);

        cosTheta =
            (1.0 + g * g - sqr * sqr) /
            (2.0 * g);
    }

    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float phi = FAST_TAU * xi.y;

    return float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
}

/*
===============================================================================
RESHADER / RENODX IMPLEMENTATION
===============================================================================

FAST noise is loaded and bound via the ReShade addon.

Texture file expected:

    fast_noise_ea.dds

Location:

    same directory as addon DLL / dxgi.dll

Format requirements:

    Texture3D
    128x128x32
    RG8_UNORM

-----------------------------------------------------------------------------
DX11
-----------------------------------------------------------------------------

Native binding is allowed:

    PSSetShaderResources
    CSSetShaderResources

-----------------------------------------------------------------------------
DX12 / VULKAN
-----------------------------------------------------------------------------

Must use ReShade API descriptors:

    command_list->push_descriptors()

-----------------------------------------------------------------------------
GLOBAL VARIABLES (addon.cpp)
-----------------------------------------------------------------------------

HMODULE g_hmodule
ID3D11Texture3D* g_isfast_texture
ID3D11ShaderResourceView* g_isfast_srv
ID3D11SamplerState* g_isfast_sampler

-----------------------------------------------------------------------------
SHARED DATA
-----------------------------------------------------------------------------

shared.h must define:

    float isfast_noise_bound;

Macro:

    #define ISFAST_NOISE_BOUND shader_injection.isfast_noise_bound

-----------------------------------------------------------------------------
SHADER DECLARATION
-----------------------------------------------------------------------------

Texture3D<float2> isfast_noise : register(t15);
SamplerState isfast_sampler : register(s15);

-----------------------------------------------------------------------------
SAMPLING EXAMPLE
-----------------------------------------------------------------------------

if (ISFAST_NOISE_BOUND > 0.5)
{
    float2 xi =
        isfast_noise.SampleLevel(
            isfast_sampler,
            float3(px & 127, py & 127, frame & 31),
            0);
}

-----------------------------------------------------------------------------
SLOT ASSIGNMENT
-----------------------------------------------------------------------------

Texture slot : t15
Sampler slot : s15

-----------------------------------------------------------------------------
BINDING FUNCTIONS
-----------------------------------------------------------------------------

Pixel shaders:

    PSSetShaderResources(15, 1, &srv);
    PSSetSamplers(15, 1, &sampler);

Compute shaders:

    CSSetShaderResources(15, 1, &srv);
    CSSetSamplers(15, 1, &sampler);

-----------------------------------------------------------------------------
DEBUGGING
-----------------------------------------------------------------------------

Several diagnostics are recommended.

1. DDS validation

Check:

    magic bytes
    DX10 header
    pixel size

2. Pixel inspection

Dump first bytes and count non-zero values.

3. Debug texture

Create fallback texture:

    1x1x1 RG8_UNORM
    value ≈ (0.75, 0.75)

This verifies binding path independent of DDS data.

4. Binding verification

Query resource binding after set:

    CSGetShaderResources

Compare returned SRV pointer with expected.

-----------------------------------------------------------------------------
TLDR IMPLEMENTATION
-----------------------------------------------------------------------------

addon.cpp

    g_hmodule
    g_isfast_texture
    g_isfast_srv
    g_isfast_sampler

shared.h

    float isfast_noise_bound

OnInitDevice

    load fast_noise_ea.dds
    create Texture3D (R8G8_UNORM)
    create SRV
    create sampler
    set isfast_noise_bound = 1

OnDestroyDevice

    release resources
    set isfast_noise_bound = 0

Binding

    PSSetShaderResources(15,...)
    CSSetShaderResources(15,...)

Shader usage

    Texture3D<float2> isfast_noise : register(t15);
    SamplerState isfast_sampler : register(s15);

Guard sampling

    if (ISFAST_NOISE_BOUND > 0.5)

===============================================================================
END OF FAST NOISE DOCUMENTATION
===============================================================================
*/


// ============================================================================
// 1.12  IS-FAST Noise Sampling
// ----------------------------------------------------------------------------
// Importance-Sampled Filter-Adapted Spatio-Temporal noise helpers.
//
// Reference:
//   "Importance-Sampled Filter-Adapted Spatio-Temporal Sampling"
//   JCGT Vol.14, No.1, 2025
//   https://jcgt.org/published/0014/01/08/
//
// Background:
//   Stochastic rendering effects (SSAO, SSR, soft shadows, volumetrics,
//   depth of field) all require random samples.  The quality of these
//   samples has a dramatic effect on both visual noise and convergence
//   speed under temporal accumulation (TAA).
//
//   Sample quality ladder (worst → best):
//     1. White noise       — clumpy, slow convergence
//     2. Blue noise (2D)   — perceptually pleasant, better coverage
//     3. STBN              — blue noise across space AND time
//     4. FAST              — blue noise optimized for the reconstruction
//                            filter (e.g. TAA's exponential moving average)
//     5. IS-FAST           — all of the above PLUS exact importance
//                            sampling for arbitrary distributions
//
//   IS-FAST extends FAST noise to support general distributions by warping
//   pre-computed uniform spatio-temporal blue noise samples through the
//   inverse CDF of the target distribution.  Because monotonic transforms
//   preserve rank ordering, the blue noise structure is maintained after
//   the warp.
//
// How to use with RenoDX:
//   1. Pre-compute an IS-FAST 3D noise volume offline (64×64×64,
//      R8_UNORM or RG8_UNORM for 1D/2D samples).  Ship as a DDS file.
//   2. In the RenoDX addon, create a Texture3D from the DDS and bind
//      it to an unused SRV slot (e.g. t15) via push_descriptors.
//   3. In replaced shaders, declare the Texture3D and sample using the
//      helpers below.
//   4. Warp the uniform [0,1] sample through the inverse CDF of the
//      target distribution (cosine hemisphere, GGX NDF, bokeh shape,
//      Henyey-Greenstein phase, etc.).
//
// Sections:
//   a) Core sampling — fetch from the 3D noise volume
//   b) Distribution warps — inverse CDF transforms for common
//      rendering distributions
//   c) Interleaved Gradient Noise fallback — when no external texture
//      is available, a self-contained analytic approximation
// ============================================================================


// ---------------------------------------------------------------------------
// 1.12a  Core IS-FAST Sampling
// ---------------------------------------------------------------------------
// Fetch a uniform [0,1] sample from a pre-computed IS-FAST 3D noise
// volume.  The volume is tiled spatially (wrapping in UV) and indexed
// temporally by frame number.
//
// The returned value has spatio-temporal blue noise properties: nearby
// pixels in the same frame get well-separated values, AND the same pixel
// across consecutive frames also gets well-separated values.  This means
// both spatial noise quality and TAA convergence are optimized.
//
// Parameters:
//   noiseVolume    – pre-computed 3D noise texture (Texture3D<float>)
//                    Format: R8_UNORM, 64×64×64 typical.
//                    Bound via RenoDX addon push_descriptors.
//   wrapSampler    – point-wrap sampler (tiles spatially, wraps in W)
//   pixelCoord     – integer pixel coordinate (SV_Position.xy)
//   frameIndex     – monotonically increasing frame counter (from
//                    RenoDX injected constant buffer)
//   volumeSize     – spatial resolution of the noise volume (default 64)
//   temporalSlices – number of temporal slices (default 64)
//
// Returns:  uniform [0,1] sample with IS-FAST blue noise properties.
// ---------------------------------------------------------------------------
float SampleISFAST(
    Texture3D<float> noiseVolume,
    SamplerState     wrapSampler,
    float2           pixelCoord,
    uint             frameIndex,
    float            volumeSize     = 64.0,
    float            temporalSlices = 64.0)
{
    float3 uvw = float3(
        (pixelCoord + 0.5) / volumeSize,
        ((float)(frameIndex % (uint)temporalSlices) + 0.5) / temporalSlices
    );
    return noiseVolume.SampleLevel(wrapSampler, uvw, 0);
}


// ---------------------------------------------------------------------------
// 1.12a′  Two-Channel IS-FAST Sampling (for 2D distributions)
// ---------------------------------------------------------------------------
// Returns two decorrelated uniform samples from the same volume by
// sampling at two spatially offset positions.  The offset values
// (37, 17) are co-prime with typical volume sizes (64, 128) ensuring
// the two channels are decorrelated.
//
// For RG8_UNORM volumes (two channels baked into one texture), use
// SampleISFAST_RG instead.
//
// Parameters:
//   noiseVolume    – Texture3D<float> (single-channel volume)
//   wrapSampler    – point-wrap sampler
//   pixelCoord     – pixel position
//   frameIndex     – frame counter
//   volumeSize     – spatial resolution (default 64)
//   temporalSlices – temporal slices (default 64)
//
// Returns:  float2 with two decorrelated uniform [0,1] samples.
// ---------------------------------------------------------------------------
float2 SampleISFAST_2D(
    Texture3D<float> noiseVolume,
    SamplerState     wrapSampler,
    float2           pixelCoord,
    uint             frameIndex,
    float            volumeSize     = 64.0,
    float            temporalSlices = 64.0)
{
    float xi1 = SampleISFAST(
        noiseVolume, wrapSampler,
        pixelCoord, frameIndex,
        volumeSize, temporalSlices);

    float xi2 = SampleISFAST(
        noiseVolume, wrapSampler,
        pixelCoord + float2(37.0, 17.0),
        frameIndex,
        volumeSize, temporalSlices);

    return float2(xi1, xi2);
}


// ---------------------------------------------------------------------------
// 1.12a″  Two-Channel IS-FAST Sampling (RG8 volume)
// ---------------------------------------------------------------------------
// For volumes stored as RG8_UNORM (two decorrelated channels baked
// together), this samples both channels in a single texture fetch.
//
// Parameters:
//   noiseVolume    – Texture3D<float2> (two-channel volume)
//   wrapSampler    – point-wrap sampler
//   pixelCoord     – pixel position
//   frameIndex     – frame counter
//   volumeSize     – spatial resolution (default 64)
//   temporalSlices – temporal slices (default 64)
//
// Returns:  float2 with two decorrelated uniform [0,1] samples.
// ---------------------------------------------------------------------------
float2 SampleISFAST_RG(
    Texture3D<float2> noiseVolume,
    SamplerState      wrapSampler,
    float2            pixelCoord,
    uint              frameIndex,
    float             volumeSize     = 64.0,
    float             temporalSlices = 64.0)
{
    float3 uvw = float3(
        (pixelCoord + 0.5) / volumeSize,
        ((float)(frameIndex % (uint)temporalSlices) + 0.5) / temporalSlices
    );
    return noiseVolume.SampleLevel(wrapSampler, uvw, 0);
}


// ---------------------------------------------------------------------------
// 1.12b  Distribution Warps — Inverse CDF Transforms
// ---------------------------------------------------------------------------
// These functions transform uniform [0,1] IS-FAST samples into
// importance-sampled directions for common rendering distributions.
//
// The key insight from the IS-FAST paper: monotonic transforms (inverse
// CDF) preserve the rank ordering of samples, which means the blue
// noise structure survives the warp.  The result is importance-sampled
// AND spatio-temporally well-distributed.
//
// Each function takes one or two uniform samples and returns a
// tangent-space direction (z-up).  The caller must rotate to world
// space using the local TBN.
// ---------------------------------------------------------------------------


// Cosine-weighted hemisphere (for diffuse / SSAO).
//
// Inverse CDF:  cos(θ) = √ξ₁,  φ = 2πξ₂
//
// Parameters:
//   xi1, xi2 – two uniform [0,1] IS-FAST samples
//
// Returns:  tangent-space direction (z = up = normal direction).
float3 ISFASTCosineHemisphere(float xi1, float xi2)
{
    float cosTheta = sqrt(xi1);
    float sinTheta = sqrt(1.0 - xi1);
    float phi = FAST_TAU * xi2;
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}


// GGX / Trowbridge-Reitz NDF importance sampling (for specular).
//
// Inverse CDF:  cos(θ_h) = √((1 - ξ₁) / (1 + (α⁴ - 1)ξ₁))
//               φ = 2πξ₂
//
// Parameters:
//   xi1, xi2  – two uniform [0,1] IS-FAST samples
//   roughness – perceptual roughness [0,1] (squared internally)
//
// Returns:  tangent-space half-vector direction.
float3 ISFASTImportanceSampleGGX(float xi1, float xi2, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;

    float cosTheta = sqrt((1.0 - xi1) / (1.0 + (a2 - 1.0) * xi1));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = FAST_TAU * xi2;

    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}


// Uniform hemisphere sampling (for unbiased AO / visibility tests).
//
// Inverse CDF:  cos(θ) = ξ₁,  φ = 2πξ₂
//
// Parameters:
//   xi1, xi2 – two uniform [0,1] IS-FAST samples
//
// Returns:  tangent-space direction (z = up).
float3 ISFASTUniformHemisphere(float xi1, float xi2)
{
    float cosTheta = xi1;
    float sinTheta = sqrt(1.0 - xi1 * xi1);
    float phi = FAST_TAU * xi2;
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}


// Uniform disc sampling (for depth of field / bokeh).
//
// Concentric disc mapping (Shirley & Chiu 1997).
//
// Parameters:
//   xi1, xi2 – two uniform [0,1] IS-FAST samples
//
// Returns:  float2 point on the unit disc [-1,1]^2.
float2 ISFASTUniformDisc(float xi1, float xi2)
{
    // Map [0,1]^2 -> [-1,1]^2
    float a = 2.0 * xi1 - 1.0;
    float b = 2.0 * xi2 - 1.0;

    float r, phi;
    if (a * a > b * b)
    {
        r   = a;
        phi = (FAST_PI / 4.0) * (b / (a + 1e-10));
    }
    else
    {
        r   = b;
        phi = (FAST_PI / 2.0) - (FAST_PI / 4.0) * (a / (b + 1e-10));
    }

    return float2(r * cos(phi), r * sin(phi));
}


// Henyey-Greenstein phase function importance sampling
// (for volumetric fog / participating media).
//
// Parameters:
//   xi1, xi2 – two uniform [0,1] IS-FAST samples
//   g        – asymmetry [-1,1]
//
// Returns:  scattering direction in frame where incident points +Z.
float3 ISFASTHenyeyGreenstein(float xi1, float xi2, float g)
{
    float cosTheta;

    if (abs(g) < 1e-4)
    {
        cosTheta = 1.0 - 2.0 * xi1; // isotropic (uniform sphere)
    }
    else
    {
        float s = (1.0 - g * g) / (1.0 + g - 2.0 * g * xi1);
        cosTheta = (1.0 + g * g - s * s) / (2.0 * g);
    }

    cosTheta = clamp(cosTheta, -1.0, 1.0);

    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float phi = FAST_TAU * xi2;

    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}


// Exponential distribution (for volumetric ray march free-path sampling).
//
// Inverse CDF:  t = -ln(1 - ξ) / σ_t
//
// Parameters:
//   xi         – uniform [0,1] IS-FAST sample
//   extinction – σ_t
//
// Returns:  free-path distance.
float ISFASTExponentialFreePath(float xi, float extinction)
{
    return -log(max(1.0 - xi, 1e-10)) / max(extinction, 1e-10);
}


// ---------------------------------------------------------------------------
// 1.12c  Interleaved Gradient Noise Fallback
// ---------------------------------------------------------------------------
// Use when external IS-FAST texture is not bound.
//
// Interleaved Gradient Noise (Jimenez 2014) + temporal variants.
// ---------------------------------------------------------------------------

float InterleavedGradientNoise(float2 pixelCoord)
{
    return frac(52.9829189 *
        frac(0.06711056 * pixelCoord.x + 0.00583715 * pixelCoord.y));
}

float InterleavedGradientNoiseTemporal(float2 pixelCoord, uint frameIndex)
{
    static const float R2_ALPHA = 0.7548776662466927;
    float base = InterleavedGradientNoise(pixelCoord);
    return frac(base + R2_ALPHA * (float)frameIndex);
}

float2 InterleavedGradientNoiseTemporal2D(float2 pixelCoord, uint frameIndex)
{
    static const float R2_ALPHA1 = 0.7548776662466927;
    static const float R2_ALPHA2 = 0.5698402909980532;

    float base1 = InterleavedGradientNoise(pixelCoord);
    float base2 = InterleavedGradientNoise(pixelCoord + float2(47.0, 17.0));

    return float2(
        frac(base1 + R2_ALPHA1 * (float)frameIndex),
        frac(base2 + R2_ALPHA2 * (float)frameIndex)
    );
}

#endif // FASTNOISE_HLSL