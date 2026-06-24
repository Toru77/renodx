// ============================================================================
// DOF.hlsl — Generic Depth of Field Template for Modding / Injection
// Supports: Pixel Shader full-screen passes OR Compute Shader passes
// Methods:  (1) CSC DoF (Circular Separable Convolution, Garcia/Frostbite)
//           (2) Low-Rank Linear Filters (McGraw)
//           (3) Classic Gather (GPU Gems-style baseline)
//
// This is a TEMPLATE: it compiles once you wire resources + pass dispatch.
// For CSC + LowRank, you must provide method-specific coefficients/tables.
// ============================================================================

// ---------------------------
// 0) Compile-time switches
// ---------------------------
#ifndef DOF_METHOD
  // 1 = CSC, 2 = LOWRANK, 3 = GATHER
  #define DOF_METHOD 1
#endif

#ifndef DOF_USE_COMPUTE
  // 0 = Pixel Shader full-screen passes (PS/VS)
  // 1 = Compute Shader passes (CS + UAV ping-pong)
  #define DOF_USE_COMPUTE 0
#endif

#ifndef DOF_HALF_RES
  #define DOF_HALF_RES 1
#endif

#ifndef DOF_ENABLE_NEAR
  #define DOF_ENABLE_NEAR 1
#endif
#ifndef DOF_ENABLE_FAR
  #define DOF_ENABLE_FAR 1
#endif

#ifndef DOF_TILE_CLASSIFY
  // Optional optimization; template includes interfaces, not a full impl.
  #define DOF_TILE_CLASSIFY 0
#endif

#ifndef DOF_DEPTH_REJECT
  // Depth-aware rejection to reduce bleeding/halos
  #define DOF_DEPTH_REJECT 1
#endif

#ifndef DOF_HIGHLIGHT_BOOST
  // Optional prefilter shaping for HDR highlights (simple version)
  #define DOF_HIGHLIGHT_BOOST 0
#endif

#ifndef DOF_DEBUG_MODE
  // 0 off, 1 coc, 2 layers, 3 tiles (if implemented)
  #define DOF_DEBUG_MODE 0
#endif

// ---------------------------
// 1) Quality knobs
// ---------------------------
#ifndef DOF_MAX_BLUR_PX
  // Max blur radius IN PIXELS at processing resolution (half-res if DOF_HALF_RES=1)
  #define DOF_MAX_BLUR_PX 20.0
#endif

#ifndef DOF_COC_EPS
  #define DOF_COC_EPS 0.5   // px threshold to treat as in-focus at processing res
#endif

// Gather settings
#ifndef DOF_GATHER_TAPS
  #define DOF_GATHER_TAPS 16
#endif

// Low-rank settings (you must provide tables)
#ifndef DOF_LOWRANK_RANKS
  #define DOF_LOWRANK_RANKS 6
#endif
#ifndef DOF_LOWRANK_TAPCOUNT
  #define DOF_LOWRANK_TAPCOUNT 12
#endif

// CSC settings (you must provide coefficients)
#ifndef DOF_CSC_COMPONENTS
  // 1 = low quality; 2 = higher quality (sum of components)
  #define DOF_CSC_COMPONENTS 2
#endif
#ifndef DOF_CSC_TAPCOUNT
  // Per 1D pass. Increase for smoother large bokeh.
  #define DOF_CSC_TAPCOUNT 17
#endif

// ---------------------------
// 2) Depth conventions (pick ONE)
// ---------------------------
#ifndef DOF_DEPTH_DEVICE
  // 1 = device depth (0..1, non-linear), needs linearization parameters
  // 0 = already linear depth in "Depth" texture
  #define DOF_DEPTH_DEVICE 1
#endif

#ifndef DOF_DEPTH_REVERSED_Z
  // If using device depth, indicate reversed-Z if needed.
  #define DOF_DEPTH_REVERSED_Z 0
#endif

// ---------------------------
// 3) CoC model (pick ONE)
// ---------------------------
#ifndef DOF_COC_ARTIST
  // Simple ramp-based CoC in depth units -> blur radius in pixels
  #define DOF_COC_ARTIST 1
#endif

#ifndef DOF_COC_THINLENS
  #define DOF_COC_THINLENS 0
#endif

// ============================================================================
// 4) Common types / utilities
// ============================================================================
struct DOFParams
{
  float FocalDistance;   // depth units of your LINEAR depth (or view distance)
  float NearTransition;  // depth units: distance over which near blur ramps to max
  float FarTransition;   // depth units: distance over which far blur ramps to max
  float MaxBlurPx;       // overrides DOF_MAX_BLUR_PX if > 0, else use compile-time
  float2 InvFullRes;     // 1/W, 1/H of full-res
  float2 InvProcRes;     // 1/w, 1/h of processing res (half or full)
  float4 DepthParams0;   // engine-specific linearization constants (device depth)
  float4 DepthParams1;   // engine-specific linearization constants (device depth)
  float HighlightThreshold; // HDR highlight shaping (optional)
  float HighlightGain;      // HDR highlight shaping (optional)
};

cbuffer cbDOF : register(b0)
{
  DOFParams gDof;
}

// Replace register slots as needed.
Texture2D    gSceneColor   : register(t0);
Texture2D    gSceneDepth   : register(t1);
SamplerState gLinearClamp  : register(s0);

// Optional: if you store CoC in a separate RT between passes:
Texture2D    gCoC          : register(t2);

// If you do compute: output UAVs
#if DOF_USE_COMPUTE
RWTexture2D<float4> gOut0 : register(u0);
RWTexture2D<float4> gOut1 : register(u1);
#endif

// ---------------------------------
// Math helpers
// ---------------------------------
float SafeRcp(float x) { return 1.0 / max(x, 1e-8); }
float MaxBlurPx()
{
  return (gDof.MaxBlurPx > 0.0) ? gDof.MaxBlurPx : (float)DOF_MAX_BLUR_PX;
}

// ---------------------------------
// Depth decode (generic placeholders)
// ---------------------------------
// NOTE: There is no universal device-depth linearization.
// You must set DepthParams0/1 per game/engine.
// Provide multiple options and switch at runtime if possible.

float LinearizeDeviceDepth_OptionA(float depth01)
{
  // Common pattern: linearDepth = 1 / (A * depth + B)
  return SafeRcp(gDof.DepthParams0.x * depth01 + gDof.DepthParams0.y);
}

float LinearizeDeviceDepth_OptionB(float depth01)
{
  // Common pattern: viewZ = C / (depth - D)
  return gDof.DepthParams0.z * SafeRcp(depth01 - gDof.DepthParams0.w);
}

float GetLinearDepth(float2 uv)
{
  float d = gSceneDepth.SampleLevel(gLinearClamp, uv, 0).r;

#if DOF_DEPTH_DEVICE
  // Pick one option; you can expose a runtime selector via DepthParams1.x etc.
  // If reversed-Z, you may need d = 1 - d depending on the title.
  #if DOF_DEPTH_REVERSED_Z
    // Some engines store reversed Z; if needed:
    // d = 1.0 - d;
  #endif
  // Default to OptionA:
  return LinearizeDeviceDepth_OptionA(d);
#else
  return d; // already linear
#endif
}

// ============================================================================
// 5) CoC computation
// ============================================================================
float ComputeCoC_Artist(float linearDepth)
{
  float d = linearDepth - gDof.FocalDistance;
  float trans = (d < 0.0) ? gDof.NearTransition : gDof.FarTransition;

  float coc01 = saturate(abs(d) * SafeRcp(max(trans, 1e-4))); // 0..1
  float cocS  = (d < 0.0) ? -coc01 : +coc01;                  // signed
  return cocS * MaxBlurPx();                                  // signed px radius @ proc res
}

// Thin-lens placeholder; requires proper camera optics.
// Kept here for completeness; you can implement if you can source params reliably.
float ComputeCoC_ThinLens(float linearDepth, float focalLength, float fNumber, float sensorScale)
{
  // TEMPLATE ONLY. Provide a real thin-lens CoC derivation if you have consistent camera params.
  // Return signed px radius at proc res.
  float cocPx = 0.0;
  return cocPx;
}

float ComputeCoC(float2 uv)
{
  float z = GetLinearDepth(uv);

#if DOF_COC_ARTIST
  return ComputeCoC_Artist(z);
#elif DOF_COC_THINLENS
  return ComputeCoC_ThinLens(z, /*focalLength*/ gDof.DepthParams1.x, /*fNumber*/ gDof.DepthParams1.y, /*sensorScale*/ gDof.DepthParams1.z);
#else
  return ComputeCoC_Artist(z);
#endif
}

// ============================================================================
// 6) Layer masks and rejection
// ============================================================================
float FarMask(float cocPx)  { return saturate( ( cocPx - DOF_COC_EPS) * SafeRcp(MaxBlurPx()) ); }
float NearMask(float cocPx) { return saturate( (-cocPx - DOF_COC_EPS) * SafeRcp(MaxBlurPx()) ); }

float LayerCompat(float cocCenter, float cocSample)
{
  // Prevent mixing near/far blur contributions
  return (sign(cocCenter) == sign(cocSample)) ? 1.0 : 0.0;
}

float DepthCompat(float zCenter, float zSample, float cocCenter)
{
#if !DOF_DEPTH_REJECT
  return 1.0;
#else
  // Generic: for FAR blur, discourage samples that are closer than center.
  // for NEAR blur, discourage samples that are farther than center.
  // Strength can be tuned. This is intentionally conservative and engine-agnostic.
  float dz = zSample - zCenter;

  if (cocCenter > 0.0)  // far layer
    return (dz >= 0.0) ? 1.0 : 0.0;
  else                 // near layer
    return (dz <= 0.0) ? 1.0 : 0.0;
#endif
}

// ============================================================================
// 7) Prefilter (optional HDR highlight shaping)
// ============================================================================
float3 DOF_Prefilter(float3 colorHDR)
{
#if !DOF_HIGHLIGHT_BOOST
  return colorHDR;
#else
  // Simple shaping: lift highlights above threshold so bokeh “pops”.
  // Keep conservative to avoid blowing energy.
  float luma = dot(colorHDR, float3(0.2126, 0.7152, 0.0722));
  float t = saturate((luma - gDof.HighlightThreshold) * SafeRcp(max(gDof.HighlightThreshold, 1e-4)));
  return lerp(colorHDR, colorHDR * (1.0 + gDof.HighlightGain), t);
#endif
}

// ============================================================================
// 8) Method 3 — GATHER (baseline)
// ============================================================================
static const float2 kGatherTaps16[16] =
{
  float2( 1, 0), float2(-1, 0), float2(0, 1), float2(0,-1),
  float2( 1, 1), float2(-1, 1), float2(1,-1), float2(-1,-1),
  float2( 2, 0), float2(-2, 0), float2(0, 2), float2(0,-2),
  float2( 2, 1), float2(-2, 1), float2(2,-1), float2(-2,-1)
};

static const float kGatherW16[16] =
{
  1,1,1,1,
  0.9,0.9,0.9,0.9,
  0.7,0.7,0.7,0.7,
  0.6,0.6,0.6,0.6
};

float3 GatherBlur(float2 uv, float cocCenterPx)
{
  float r = abs(cocCenterPx);
  if (r < DOF_COC_EPS) return gSceneColor.SampleLevel(gLinearClamp, uv, 0).rgb;

  float zCenter = GetLinearDepth(uv);

  float3 sum = 0;
  float  wsum = 0;

  [unroll]
  for (int i = 0; i < 16; i++)
  {
    float2 duv = kGatherTaps16[i] * (r * gDof.InvProcRes);
    float2 suv = uv + duv;

    float cocS = gCoC.SampleLevel(gLinearClamp, suv, 0).r;
    float zS   = GetLinearDepth(suv);

    float w = kGatherW16[i];
    w *= LayerCompat(cocCenterPx, cocS);
    w *= DepthCompat(zCenter, zS, cocCenterPx);

    float3 c = gSceneColor.SampleLevel(gLinearClamp, suv, 0).rgb;
    sum  += c * w;
    wsum += w;
  }

  return sum * SafeRcp(max(wsum, 1e-5));
}

// ============================================================================
// 9) Method 2 — LOW-RANK LINEAR FILTERS (McGraw)
// ============================================================================
// You must supply per-rank 1D weights and offsets (for H and V).
// Suggested storage:
// - CPU-generated constant buffers
// - or a small LUT texture
// The paper uses low-rank decomposition to approximate an aperture PSF as sum of separable 1D filters.

struct LowRank1D
{
  // tapCount entries used; remaining ignored
  float w[DOF_LOWRANK_TAPCOUNT];
  float o[DOF_LOWRANK_TAPCOUNT];
};

// You must fill these from your chosen kernel decomposition.
cbuffer cbLowRank : register(b1)
{
  LowRank1D gLR_H[DOF_LOWRANK_RANKS];
  LowRank1D gLR_V[DOF_LOWRANK_RANKS];
  int       gLR_TapCount; // <= DOF_LOWRANK_TAPCOUNT
  int       gLR_Ranks;    // <= DOF_LOWRANK_RANKS
  float2    _padLR;
}

float3 LowRank1DPass(float2 uv, float cocCenterPx, float2 axis, LowRank1D filt)
{
  float r = abs(cocCenterPx);
  float3 sum = 0;
  float  wsum = 0;

  int tapCount = min(gLR_TapCount, DOF_LOWRANK_TAPCOUNT);

  [unroll]
  for (int i = 0; i < DOF_LOWRANK_TAPCOUNT; i++)
  {
    if (i >= tapCount) break;

    float2 duv = axis * (filt.o[i] * r) * gDof.InvProcRes;
    float2 suv = uv + duv;

    float3 c = gSceneColor.SampleLevel(gLinearClamp, suv, 0).rgb;
    float  w = filt.w[i];

    sum  += c * w;
    wsum += w;
  }

  return sum * SafeRcp(max(wsum, 1e-5));
}

// Typical usage:
// For each rank r:
//   tmp += V( H(scene, gLR_H[r]), gLR_V[r] )

// ============================================================================
// 10) Method 1 — CSC DOF (Garcia / Frostbite)
// ============================================================================
// CSC uses a complex-valued 1D filter:
//   F(x) = exp(-a x^2) * (cos(b x^2) + i sin(b x^2))
// Then uses separable passes and takes magnitude / recombination.
// You must supply (a,b) coefficients for 1 or more components; the talk lists values.

// Minimal complex type
struct cfloat { float re; float im; };

cfloat cadd(cfloat A, cfloat B) { cfloat r; r.re = A.re + B.re; r.im = A.im + B.im; return r; }
cfloat cmul(cfloat A, cfloat B)
{
  cfloat r;
  r.re = A.re * B.re - A.im * B.im;
  r.im = A.re * B.im + A.im * B.re;
  return r;
}
cfloat cscale(cfloat A, float s) { cfloat r; r.re = A.re * s; r.im = A.im * s; return r; }
float  cmag(cfloat A) { return sqrt(A.re*A.re + A.im*A.im); }

struct CSCComp
{
  float a; // Gaussian envelope
  float b; // phasor frequency
};

// Fill these from the Garcia talk/table (1 component = low quality, 2 components = higher quality).
cbuffer cbCSC : register(b2)
{
  CSCComp gCSC[DOF_CSC_COMPONENTS];
  int     gCSC_TapCount; // <= DOF_CSC_TAPCOUNT (odd recommended)
  int     gCSC_Comps;    // <= DOF_CSC_COMPONENTS
  float2  _padCSC;
}

// Compute complex filter at normalized sample position x (in pixels, along 1D axis).
cfloat CSC_Filter(float x, CSCComp p)
{
  float x2 = x * x;
  float e  = exp(-p.a * x2);

  float ang = p.b * x2;
  cfloat r;
  r.re = e * cos(ang);
  r.im = e * sin(ang);
  return r;
}

// CSC 1D pass returns complex accum for each color channel.
// For convenience, do RGB separately (3 complex accumulators).
void CSC_1DPass(in float2 uv, in float cocCenterPx, in float2 axis, out cfloat outR, out cfloat outG, out cfloat outB)
{
  outR = (cfloat)0;
  outG = (cfloat)0;
  outB = (cfloat)0;

  float r = abs(cocCenterPx);
  if (r < DOF_COC_EPS)
  {
    float3 c = gSceneColor.SampleLevel(gLinearClamp, uv, 0).rgb;
    outR.re = c.r; outG.re = c.g; outB.re = c.b;
    return;
  }

  int taps = min(gCSC_TapCount, DOF_CSC_TAPCOUNT);
  // Expect odd number (center tap at 0). If even, behavior is still defined but less symmetric.
  int halfN = taps / 2;

  // Sum components (1 or 2). For 2 components, add their results.
  int comps = min(gCSC_Comps, DOF_CSC_COMPONENTS);

  [unroll]
  for (int ci = 0; ci < DOF_CSC_COMPONENTS; ci++)
  {
    if (ci >= comps) break;

    CSCComp P = gCSC[ci];

    [unroll]
    for (int ti = 0; ti < DOF_CSC_TAPCOUNT; ti++)
    {
      if (ti >= taps) break;

      int  k  = ti - halfN;           // symmetric around 0
      float x = (float)k;             // tap position in pixels along axis (scaled by r below)

      float2 suv = uv + axis * (x * r) * gDof.InvProcRes;

      float3 c = gSceneColor.SampleLevel(gLinearClamp, suv, 0).rgb;

      // Complex weight
      cfloat w = CSC_Filter(x, P);

      // Accumulate: out += c * w  (real/imag separately)
      outR = cadd(outR, cscale(w, c.r));
      outG = cadd(outG, cscale(w, c.g));
      outB = cadd(outB, cscale(w, c.b));
    }
  }
}

// After horizontal then vertical, you typically use magnitude (or other recombine) per channel.
float3 CSC_ReconstructColor(cfloat R, cfloat G, cfloat B)
{
  // Simplest generic: magnitude per channel.
  // Production implementations may renormalize / bias to preserve energy.
  return float3(cmag(R), cmag(G), cmag(B));
}

// ============================================================================
// 11) Recombine Near/Far + Composite
// ============================================================================
float3 RecombineDOF(float2 uv, float cocPx, float3 sharp, float3 blurFar, float3 blurNear)
{
  float fm = DOF_ENABLE_FAR  ? FarMask(cocPx)  : 0.0;
  float nm = DOF_ENABLE_NEAR ? NearMask(cocPx) : 0.0;

  // Far first, then near over it (near wins).
  float3 c = lerp(sharp, blurFar, fm);
  c = lerp(c, blurNear, nm);
  return c;
}

// ============================================================================
// 12) Debug output
// ============================================================================
float4 DebugOut(float2 uv, float cocPx)
{
#if DOF_DEBUG_MODE == 1
  // Signed CoC visualization: near=blue, far=red
  float n = saturate((-cocPx) / MaxBlurPx());
  float f = saturate(( cocPx) / MaxBlurPx());
  return float4(f, 0, n, 1);
#elif DOF_DEBUG_MODE == 2
  float fm = FarMask(cocPx);
  float nm = NearMask(cocPx);
  return float4(fm, nm, 0, 1);
#else
  return float4(0,0,0,1);
#endif
}

// ============================================================================
// 13) Pixel Shader integration (full-screen pass)
// ============================================================================
#if !DOF_USE_COMPUTE

struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };

// NOTE: You must supply a full-screen triangle VS elsewhere.
// This file focuses on DOF logic.

float4 PS_CoC(VSOut i) : SV_Target
{
  float cocPx = ComputeCoC(i.uv);
  return float4(cocPx, 0, 0, 1);
}

// For methods requiring multiple passes, you should implement:
// - Prefilter/downsample pass (optional)
// - Blur passes (H/V or gather)
// - Recombine/composite pass
//
// In modding/injection contexts you often only get 1-2 hooks;
// if you only have one pass, choose DOF_METHOD=3 (GATHER) at half-res.

float4 PS_DOF_SinglePass(VSOut i) : SV_Target
{
  float2 uv = i.uv;
  float cocPx = (gCoC) ? gCoC.SampleLevel(gLinearClamp, uv, 0).r : ComputeCoC(uv);

  float3 sharp = gSceneColor.SampleLevel(gLinearClamp, uv, 0).rgb;
  sharp = DOF_Prefilter(sharp);

  // Baseline single-pass fallback:
  // - far blur computed with positive coc, near blur computed with negative coc
  float3 blurFar = sharp;
  float3 blurNear = sharp;

#if DOF_METHOD == 3
  // Gather once using sign of coc
  blurFar  = (cocPx >  DOF_COC_EPS) ? GatherBlur(uv, +abs(cocPx)) : sharp;
  blurNear = (cocPx < -DOF_COC_EPS) ? GatherBlur(uv, -abs(cocPx)) : sharp;

  float3 outC = RecombineDOF(uv, cocPx, sharp, blurFar, blurNear);

  #if DOF_DEBUG_MODE != 0
    return DebugOut(uv, cocPx);
  #endif
  return float4(outC, 1);

#else
  // CSC / LowRank require multi-pass ping-pong for quality.
  // Provide a minimal fallback so the shader “does something” in a single hook.
  float3 outC = sharp;

  #if DOF_DEBUG_MODE != 0
    return DebugOut(uv, cocPx);
  #endif
  return float4(outC, 1);
#endif
}

#endif // !DOF_USE_COMPUTE

// ============================================================================
// 14) Compute Shader integration (UAV ping-pong)
// ============================================================================
#if DOF_USE_COMPUTE

// Dispatch in tiles matching your processing resolution.
// Typically: [numthreads(8,8,1)] or [numthreads(16,16,1)].
[numthreads(8,8,1)]
void CS_CoC(uint3 tid : SV_DispatchThreadID)
{
  // You must map tid -> uv for processing resolution.
  // Template assumes you dispatch exactly over processing res, so:
  // uv = (tid.xy + 0.5) * InvProcRes
  float2 uv = (float2(tid.xy) + 0.5) * gDof.InvProcRes;

  float cocPx = ComputeCoC(uv);
  gOut0[tid.xy] = float4(cocPx, 0, 0, 1);
}

// Example compute blur stage for Gather (single pass).
// For CSC/LowRank, you’ll typically do:
// - CS_Horizontal into gOut1
// - CS_Vertical into gOut0
// - CS_Recombine into gOut1 (full-res composite if needed)

[numthreads(8,8,1)]
void CS_Gather(uint3 tid : SV_DispatchThreadID)
{
  float2 uv = (float2(tid.xy) + 0.5) * gDof.InvProcRes;

  float cocPx = gCoC.SampleLevel(gLinearClamp, uv, 0).r;
  float3 sharp = DOF_Prefilter(gSceneColor.SampleLevel(gLinearClamp, uv, 0).rgb);

  float3 blurFar  = (cocPx >  DOF_COC_EPS) ? GatherBlur(uv, +abs(cocPx)) : sharp;
  float3 blurNear = (cocPx < -DOF_COC_EPS) ? GatherBlur(uv, -abs(cocPx)) : sharp;

  float3 outC = RecombineDOF(uv, cocPx, sharp, blurFar, blurNear);

#if DOF_DEBUG_MODE != 0
  gOut0[tid.xy] = DebugOut(uv, cocPx);
#else
  gOut0[tid.xy] = float4(outC, 1);
#endif
}

// CSC horizontal pass (writes complex accum packed in RGBA; you likely need multiple RTs/UAVs).
// TEMPLATE: shows structure; you must decide packing format.
// One option: write magnitude only (cheaper, but not “true CSC”).
// Another: write complex accum for R and G in one UAV and B + padding in another.

[numthreads(8,8,1)]
void CS_CSC_H(uint3 tid : SV_DispatchThreadID)
{
#if DOF_METHOD != 1
  return;
#else
  float2 uv = (float2(tid.xy) + 0.5) * gDof.InvProcRes;
  float cocPx = gCoC.SampleLevel(gLinearClamp, uv, 0).r;

  cfloat rC, gC, bC;
  CSC_1DPass(uv, cocPx, float2(1,0), rC, gC, bC);

  // TEMPLATE PACK: store magnitude only (debug-quality).
  // For real CSC, ping-pong complex values between passes.
  float3 mag = CSC_ReconstructColor(rC, gC, bC);
  gOut0[tid.xy] = float4(mag, cocPx);
#endif
}

[numthreads(8,8,1)]
void CS_CSC_V(uint3 tid : SV_DispatchThreadID)
{
#if DOF_METHOD != 1
  return;
#else
  float2 uv = (float2(tid.xy) + 0.5) * gDof.InvProcRes;

  // TEMPLATE: if you did real CSC, you would read complex accum from a UAV/texture produced by CS_CSC_H.
  // Here we show a simplified "repeat CSC along Y" using scene again (not equivalent to proper 2-pass CSC).
  float cocPx = gCoC.SampleLevel(gLinearClamp, uv, 0).r;

  cfloat rC, gC, bC;
  CSC_1DPass(uv, cocPx, float2(0,1), rC, gC, bC);

  float3 blur = CSC_ReconstructColor(rC, gC, bC);
  float3 sharp = DOF_Prefilter(gSceneColor.SampleLevel(gLinearClamp, uv, 0).rgb);

  // Split into near/far by sign (simple; better if you run separate passes/masks)
  float3 blurFar  = (cocPx >  DOF_COC_EPS) ? blur : sharp;
  float3 blurNear = (cocPx < -DOF_COC_EPS) ? blur : sharp;

  float3 outC = RecombineDOF(uv, cocPx, sharp, blurFar, blurNear);
  gOut0[tid.xy] = float4(outC, 1);
#endif
}

#endif // DOF_USE_COMPUTE

// ============================================================================
// 15) “Pass plan” reference (copy into your injector notes)
//
// A) Minimal (1 hook):
//    - DOF_METHOD=GATHER, run at half-res if possible, composite to full.
//
// B) Standard production (recommended):
//    Pass 1: CoC (proc res)
//    Pass 2: Prefilter + Downsample (proc res, optional)
//    Pass 3: Blur Far  (H then V or gather) at proc res
//    Pass 4: Blur Near (H then V or gather) at proc res
//    Pass 5: Recombine + Composite to full-res
//
// C) CSC (high quality circular bokeh):
//    Pass 1: CoC
//    Pass 2: Prefilter/downsample
//    Pass 3: CSC horizontal (complex accum)
//    Pass 4: CSC vertical   (complex accum)
//    Pass 5: magnitude + near/far resolve + composite
//
// D) Low-rank (arbitrary bokeh shapes):
//    Pass 1: CoC
//    Pass 2: Downsample
//    Pass 3..: For each rank: H then V, sum
//    Pass N: Recombine
// ============================================================================
