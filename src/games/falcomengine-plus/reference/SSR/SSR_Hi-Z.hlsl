// ============================================================================
// SSR_Hi-Z.hlsl — Hi-Z Screen Space Reflections (faithful to Sugu Lee Part 2)
// ----------------------------------------------------------------------------
// Goal: Match the traversal + Hi-Z build details described in:
//   "Screen Space Reflections : Implementation and optimization – Part 2 : HI-Z Tracing Method"
//   by Sugu Lee (Jan 19, 2021). :contentReference[oaicite:0]{index=0}
//
// This file fills the “gaps” the article implicitly leaves for a HLSL/engine
// implementation, while keeping the algorithm structure and intent the same.
//
// IMPORTANT ASSUMPTIONS (same as the article’s baseline):
// - Hi-Z stores per-cell MIN depth (standard device-depth direction). :contentReference[oaicite:1]{index=1}
// - Tracing operates in Texture Space (TS): ray.xy are UV in [0,1], ray.z is depth
//   in the same convention as the Hi-Z base level. :contentReference[oaicite:2]{index=2}
//
// If your game uses reversed-Z or linear view-Z for depth, you must adapt the
// compare logic + Hi-Z reduce operator accordingly (see switches below).
// ============================================================================


// ---------------------------
// 0) Compile-time switches
// ---------------------------
#ifndef SSR_USE_COMPUTE
  #define SSR_USE_COMPUTE 1
#endif

#ifndef SSR_REVERSED_Z
  // Article’s default is MIN-depth Hi-Z for “forward” depth convention. :contentReference[oaicite:3]{index=3}
  // Set to 1 only if your engine uses reversed-Z AND you also build Hi-Z as MAX.
  #define SSR_REVERSED_Z 0
#endif

#ifndef SSR_MAX_THICKNESS
  // Used only at mip 0 (stopLevel) to allow passing “behind” objects. :contentReference[oaicite:4]{index=4}
  // Tune per game depending on depth encoding.
  #define SSR_MAX_THICKNESS 0.005
#endif

#ifndef SSR_CROSS_EPS_DENOM
  // Article uses: crossOffset = crossStep / ViewSize / 128. :contentReference[oaicite:5]{index=5}
  #define SSR_CROSS_EPS_DENOM 128.0
#endif

#ifndef SSR_CROSS_OFFSET_NONPOW2_MULT
  // Article multiplies crossOffset by 64 for the initial “push to next cell”
  // to make non-power-of-two cases reliable. :contentReference[oaicite:6]{index=6}
  #define SSR_CROSS_OFFSET_NONPOW2_MULT 64.0
#endif

#ifndef SSR_START_LEVEL
  // Article example uses startLevel = 2. :contentReference[oaicite:7]{index=7}
  #define SSR_START_LEVEL 2
#endif

#ifndef SSR_STOP_LEVEL
  // Article uses stopLevel = 0. :contentReference[oaicite:8]{index=8}
  #define SSR_STOP_LEVEL 0
#endif

#ifndef SSR_MAX_ITER_FALLBACK
  #define SSR_MAX_ITER_FALLBACK 128
#endif

#ifndef SSR_DEBUG_MODE
  // 0 off, 1 hit mask, 2 visualize ray depth, 3 visualize Hi-Z mip used (requires extra output)
  #define SSR_DEBUG_MODE 0
#endif


// ---------------------------
// 1) Resources (bind as needed)
// ---------------------------
// Base depth (device depth in typical engines)
Texture2D<float>   gDepth        : register(t0);
// Normal.xyz + reflectionMask(w) (or roughness/mask)
Texture2D<float4>  gNormalMask   : register(t1);
// Scene color (prefer HDR if available)
Texture2D<float4>  gSceneColor   : register(t2);

// Hi-Z SRV with full mip chain
Texture2D<float>   gHiZ          : register(t3);

SamplerState gPointClamp : register(s0);
SamplerState gLinearClamp: register(s1);

#if SSR_USE_COMPUTE
  // Hi-Z build writes one mip per dispatch by rebinding this UAV to the target mip.
  RWTexture2D<float>  gHiZ_UAV : register(u0);

  // SSR output
  RWTexture2D<float4> gOut     : register(u1);
#endif


// ---------------------------
// 2) Constant buffers
// ---------------------------
cbuffer cbSSR : register(b0)
{
  float2 ViewSize;       // width,height in pixels
  float2 InvViewSize;    // 1/width, 1/height

  uint   HiZMaxLevel;    // MUST be (mipCount - 1). (article: tex.get_num_mip_levels()-1) :contentReference[oaicite:9]{index=9}
  uint   MaxIteration;   // if 0, fallback to SSR_MAX_ITER_FALLBACK
  float  _pad0;
  float  _pad1;

  // Any additional camera/depth parameters you need for ComputePosAndReflection()
  // (left generic because Part 2 assumes you already have Part 1’s setup). :contentReference[oaicite:10]{index=10}
};


// ============================================================================
// 3) Helpers: min/max reduce and “inside volume” test
// ============================================================================
float HiZReduce(float a, float b)
{
#if SSR_REVERSED_Z
  return max(a, b);
#else
  return min(a, b);
#endif
}

// Article logic uses MIN depth per cell (standard). :contentReference[oaicite:11]{index=11}
bool RayInsideCellVolume(float rayZ, float cellDepthPlane)
{
#if SSR_REVERSED_Z
  // reversed-Z + MAX Hi-Z would flip.
  return (rayZ <= cellDepthPlane);
#else
  return (rayZ >= cellDepthPlane);
#endif
}

float SafeRcp(float x) { return 1.0 / max(x, 1e-8); }


// ============================================================================
// 4) Hi-Z build (Compute): non-power-of-two fix (3x3 when needed)
// ----------------------------------------------------------------------------
// The article’s fix checks the ratio between src and dst mip dimensions and
// adds extra samples if ratio.x>2 or ratio.y>2, effectively covering 3x3. :contentReference[oaicite:12]{index=12}
// ============================================================================
#if SSR_USE_COMPUTE

cbuffer cbHiZBuild : register(b1)
{
  uint SrcMip;  // previous mip
  uint DstMip;  // current mip
  uint _padB0;
  uint _padB1;
};

float HiZLoad(Texture2D<float> tex, int2 p, uint mip)
{
  return tex.Load(int3(p, mip));
}

// Mip0 copy: copy scene depth into Hi-Z level 0. :contentReference[oaicite:13]{index=13}
[numthreads(8,8,1)]
void CS_HiZ_CopyMip0(uint3 tid : SV_DispatchThreadID)
{
  int2 p = int2(tid.xy);

  uint w,h;
  gHiZ.GetDimensions(0, w, h);
  if ((uint)p.x >= w || (uint)p.y >= h) return;

  gHiZ_UAV[p] = gDepth.Load(int3(p, 0));
}

// Reduce mip: from SrcMip -> DstMip using 2x2 or 3x3 depending on ratio. :contentReference[oaicite:14]{index=14}
[numthreads(8,8,1)]
void CS_HiZ_Reduce(uint3 tid : SV_DispatchThreadID)
{
  int2 dst = int2(tid.xy);

  uint srcW, srcH, dstW, dstH;
  gHiZ.GetDimensions(SrcMip, srcW, srcH);
  gHiZ.GetDimensions(DstMip, dstW, dstH);
  if ((uint)dst.x >= dstW || (uint)dst.y >= dstH) return;

  float2 ratio = float2((float)srcW / (float)dstW, (float)srcH / (float)dstH);

  int2 src = dst * 2;

  // Clamp sample coords (safe for edges)
  int2 maxP = int2((int)srcW - 1, (int)srcH - 1);
  int2 p00 = clamp(src + int2(0,0), int2(0,0), maxP);
  int2 p10 = clamp(src + int2(1,0), int2(0,0), maxP);
  int2 p01 = clamp(src + int2(0,1), int2(0,0), maxP);
  int2 p11 = clamp(src + int2(1,1), int2(0,0), maxP);

  float v = HiZReduce(HiZLoad(gHiZ, p00, SrcMip), HiZLoad(gHiZ, p10, SrcMip));
  v = HiZReduce(v, HiZLoad(gHiZ, p01, SrcMip));
  v = HiZReduce(v, HiZLoad(gHiZ, p11, SrcMip));

  bool needX = (ratio.x > 2.0);
  bool needY = (ratio.y > 2.0);

  if (needX)
  {
    int2 p20 = clamp(src + int2(2,0), int2(0,0), maxP);
    int2 p21 = clamp(src + int2(2,1), int2(0,0), maxP);
    v = HiZReduce(v, HiZLoad(gHiZ, p20, SrcMip));
    v = HiZReduce(v, HiZLoad(gHiZ, p21, SrcMip));
  }
  if (needY)
  {
    int2 p02 = clamp(src + int2(0,2), int2(0,0), maxP);
    int2 p12 = clamp(src + int2(1,2), int2(0,0), maxP);
    v = HiZReduce(v, HiZLoad(gHiZ, p02, SrcMip));
    v = HiZReduce(v, HiZLoad(gHiZ, p12, SrcMip));
  }
  if (needX && needY)
  {
    int2 p22 = clamp(src + int2(2,2), int2(0,0), maxP);
    v = HiZReduce(v, HiZLoad(gHiZ, p22, SrcMip));
  }

  gHiZ_UAV[dst] = v;
}

#endif // SSR_USE_COMPUTE


// ============================================================================
// 5) Hi-Z traversal utilities (as in the article)
// ----------------------------------------------------------------------------
// getCellCount(level) = mip dimensions :contentReference[oaicite:15]{index=15}
// getCell(pos, cellCount) = floor(pos * cellCount) :contentReference[oaicite:16]{index=16}
// getMinimumDepthPlane: sample Hi-Z at mip (we sample cell center to avoid boundary issues) :contentReference[oaicite:17]{index=17}
// intersectDepthPlane(o,d,t) = o + d*t :contentReference[oaicite:18]{index=18}
// intersectCellBoundary: cross into next cell using crossStep/crossOffset :contentReference[oaicite:19]{index=19}
// ============================================================================
float2 GetCellCount(int mip)
{
  uint w,h;
  gHiZ.GetDimensions(mip, w, h);
  return float2((float)w, (float)h);
}

float2 GetCell(float2 posUV, float2 cellCount)
{
  return floor(posUV * cellCount);
}

float GetMinimumDepthPlane_CellCenter(float2 cellIdx, float2 cellCount, int mip)
{
  // Article: sample Hi-Z at given mip; implementation chooses cell center to avoid boundary sampling errors. :contentReference[oaicite:20]{index=20}
  float2 uv = (cellIdx + 0.5) / cellCount;
  return gHiZ.SampleLevel(gPointClamp, uv, (float)mip);
}

float3 IntersectDepthPlane(float3 o, float3 d, float t)
{
  return o + d * t;
}

bool CrossedCellBoundary(float2 a, float2 b)
{
  return any(a != b);
}

// Robust version of intersectCellBoundary:
// - Same math as the article :contentReference[oaicite:21]{index=21}
// - Adds protection when d.x or d.y is ~0 (ray nearly axis-aligned in screen)
float3 IntersectCellBoundary(
  float3 o, float3 d,
  float2 cell, float2 cellCount,
  float2 crossStep01, float2 crossOffset)
{
  float2 index    = cell + crossStep01;
  float2 boundary = index / cellCount;
  boundary += crossOffset;

  float2 delta = boundary - o.xy;

  // Avoid INF/NaN on near-zero d.xy:
  float2 invDxy = float2(SafeRcp(d.x), SafeRcp(d.y));
  float2 t2     = delta * invDxy;

  float t = min(t2.x, t2.y);
  return IntersectDepthPlane(o, d, t);
}


// ============================================================================
// 6) FindIntersection_HiZ — faithful loop structure + forward/backward handling
// ----------------------------------------------------------------------------
// This follows the article’s core body (start push, then loop changing mip level,
// using temp ray, boundary crossing, and backward-ray special handling). :contentReference[oaicite:22]{index=22}
// ============================================================================
float FindIntersection_HiZ(
  float3 samplePosTS,
  float3 reflDirTS,
  float  maxTraceDistanceTS,      // distance to max trace point (in TS paramization below)
  out float3 intersectionTS)
{
  const int maxLevel  = (int)HiZMaxLevel;            // article: get_num_mip_levels()-1 :contentReference[oaicite:23]{index=23}
  const int startLevel= min(SSR_START_LEVEL, maxLevel);
  const int stopLevel = SSR_STOP_LEVEL;

  // crossStep/crossOffset setup from article :contentReference[oaicite:24]{index=24}
  float2 crossStepSign = float2(reflDirTS.x >= 0 ? 1 : -1, reflDirTS.y >= 0 ? 1 : -1);
  float2 crossOffset   = crossStepSign / ViewSize / SSR_CROSS_EPS_DENOM;  // /128 :contentReference[oaicite:25]{index=25}
  float2 crossStep01   = saturate(crossStepSign);                         // {-1,+1} -> {0,1} :contentReference[oaicite:26]{index=26}

  // Initialize ray, minZ/maxZ/deltaZ per article :contentReference[oaicite:27]{index=27}
  float3 ray   = samplePosTS;
  float  minZ  = ray.z;
  float  maxZ  = ray.z + reflDirTS.z * maxTraceDistanceTS;
  float  deltaZ= (maxZ - minZ);

  // IMPORTANT: article’s precision fix: o = ray origin, d = dir * maxTraceDistanceTS :contentReference[oaicite:28]{index=28}
  float3 o = ray;
  float3 d = reflDirTS * maxTraceDistanceTS;

  // Push to next cell at startLevel to avoid self-intersection :contentReference[oaicite:29]{index=29}
  float2 startCellCount = GetCellCount(startLevel);
  float2 rayCell        = GetCell(ray.xy, startCellCount);

  // Article multiplies crossOffset by 64 here for non-power-of-two robustness :contentReference[oaicite:30]{index=30}
  ray = IntersectCellBoundary(o, d, rayCell, startCellCount, crossStep01, crossOffset * SSR_CROSS_OFFSET_NONPOW2_MULT);

  int  level = startLevel;
  uint iter  = 0;
  uint maxIt = (MaxIteration != 0) ? MaxIteration : (uint)SSR_MAX_ITER_FALLBACK;

  bool  isBackwardRay = (reflDirTS.z < 0);  // article :contentReference[oaicite:31]{index=31}
  float rayDir        = isBackwardRay ? -1.0 : 1.0;

  while (level >= stopLevel &&
         (ray.z * rayDir) <= (maxZ * rayDir) &&          // termination flips for backward :contentReference[oaicite:32]{index=32}
         iter < maxIt)
  {
    float2 cellCount   = GetCellCount(level);
    float2 oldCellIdx  = GetCell(ray.xy, cellCount);

    float cell_minZ = GetMinimumDepthPlane_CellCenter(oldCellIdx, cellCount, level); // cell-center sampling :contentReference[oaicite:33]{index=33}

    float3 tmpRay = ray;

    if (!isBackwardRay)
    {
      // Forward ray case (article A/B logic) :contentReference[oaicite:34]{index=34}
      if (cell_minZ > ray.z)
      {
        // tmpRay = intersectDepthPlane(o,d,(cell_minZ-minZ)/deltaZ) :contentReference[oaicite:35]{index=35}
        // Keep exact form to stay faithful:
        float t = (cell_minZ - minZ) * SafeRcp(deltaZ);
        tmpRay = IntersectDepthPlane(o, d, t);
      }

      float2 newCellIdx = GetCell(tmpRay.xy, cellCount);

      float thickness = (level == 0) ? (tmpRay.z - cell_minZ) : 0.0;      // article: only at level 0 :contentReference[oaicite:36]{index=36}
      bool crossed =
        (thickness > SSR_MAX_THICKNESS) ||
        CrossedCellBoundary(oldCellIdx, newCellIdx);

      ray   = crossed ? IntersectCellBoundary(o, d, oldCellIdx, cellCount, crossStep01, crossOffset)
                      : tmpRay;
      level = crossed ? min(maxLevel, level + 1)
                      : (level - 1);
    }
    else
    {
      // Backward ray handling: article says logic is simpler :contentReference[oaicite:37]{index=37}
      // If inside cell volume, refine (go down mip); otherwise cross boundary and coarsen.
      bool inside = (cell_minZ > ray.z); // as in article’s crossed condition: (isBackwardRay && (cell_minZ > ray.z)) :contentReference[oaicite:38]{index=38}

      if (inside)
      {
        // “stay and go to higher resolution” (level-1)
        level = level - 1;
      }
      else
      {
        ray   = IntersectCellBoundary(o, d, oldCellIdx, cellCount, crossStep01, crossOffset);
        level = min(maxLevel, level + 1);
      }
    }

    ++iter;
  }

  bool intersected = (level < stopLevel);  // article :contentReference[oaicite:39]{index=39}
  intersectionTS = ray;
  return intersected ? 1.0 : 0.0;
}


// ============================================================================
// 7) Engine-dependent hook: ComputePosAndReflection (Part 1 responsibility)
// ----------------------------------------------------------------------------
// Part 2 explicitly reuses the “main body” from linear tracing; only swaps in
// Hi-Z and FindIntersection_HiZ. :contentReference[oaicite:40]{index=40}
//
// Therefore this template keeps this as an ENGINE-IMPLEMENTED function.
// You must output:
// - samplePosTS     : float3(uv, depth)
// - vReflDirTS      : ray direction in TS (xy in uv-space delta, z in depth-space delta)
// - maxTraceDistance: max distance until ray exits visible boundary (as in Part 1) :contentReference[oaicite:41]{index=41}
// ============================================================================
void ComputePosAndReflection(
  uint2  pix,
  out float3 samplePosTS,
  out float3 vReflDirTS,
  out float  maxTraceDistanceTS,
  out float  reflectionMask);


// ============================================================================
// 8) SSR Pass (Compute) — faithful structure
// ----------------------------------------------------------------------------
// Matches the article’s main SSR body shape: mask check, compute pos/dir/maxDist,
// call FindIntersection_HiZ, sample scene color at hit, add to base. :contentReference[oaicite:42]{index=42}
// ============================================================================
#if SSR_USE_COMPUTE

[numthreads(8,8,1)]
void CS_SSR_HiZ(uint3 tid : SV_DispatchThreadID)
{
  uint2 pix = tid.xy;
  if (pix.x >= (uint)ViewSize.x || pix.y >= (uint)ViewSize.y) return;

  float4 base = gSceneColor.Load(int3(pix, 0));

  float3 samplePosTS, dirTS;
  float  maxDistTS, mask;
  ComputePosAndReflection(pix, samplePosTS, dirTS, maxDistTS, mask);

  float4 outC = base;

  if (mask != 0.0)
  {
    float3 hitTS;
    float intensity = FindIntersection_HiZ(samplePosTS, dirTS, maxDistTS, hitTS);

    float3 refl = gSceneColor.SampleLevel(gLinearClamp, hitTS.xy, 0).rgb;

#if SSR_DEBUG_MODE == 1
    outC.rgb = intensity.xxx;
#elif SSR_DEBUG_MODE == 2
    outC.rgb = hitTS.zzz;
#else
    outC.rgb = base.rgb + refl * (intensity * mask);
#endif
  }

  gOut[pix] = outC;
}

#endif // SSR_USE_COMPUTE


// ============================================================================
// 9) Integration checklist (so you actually match the article in practice)
// ----------------------------------------------------------------------------
// (A) Hi-Z build pass order (per frame):
//   1) Bind gHiZ_UAV to mip 0. Dispatch CS_HiZ_CopyMip0 over full res. :contentReference[oaicite:43]{index=43}
//   2) For each mip i=1..HiZMaxLevel:
//        SrcMip=i-1, DstMip=i
//        Bind gHiZ_UAV to mip i
//        Dispatch CS_HiZ_Reduce over mip i dimensions :contentReference[oaicite:44]{index=44}
//
// (B) SSR pass:
//   - Ensure gHiZ has full mip chain built
//   - Dispatch CS_SSR_HiZ over full res
//
// (C) Critical “loyalty points” implemented here:
//   - Non-power-of-two Hi-Z reduce uses ratio>2 to include 3x3 extra samples :contentReference[oaicite:45]{index=45}
//   - Start push to next cell at startLevel with crossOffset * 64 workaround :contentReference[oaicite:46]{index=46}
//   - getMinimumDepthPlane samples cell center to avoid boundary ambiguity :contentReference[oaicite:47]{index=47}
//   - Forward vs backward ray treatment follows the article’s described logic :contentReference[oaicite:48]{index=48}
//
// ============================================================================