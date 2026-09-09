// DynCubeBoundsReduceCS.cs_5_0.hlsl — Falcom Engine+ world-fixed parallax bounds reduction.
// Accumulates a persistent world-space AABB over the dynamic cubemap's captured
// geometry so the parallax proxy stays fixed in the world instead of following
// the camera. Expand-only; reset only via recreation or the manual Reset button.
// Input : pos t0 (rgb=scaled pos, a=validity), contrib t1, charmask t2, camCur t3,
//         scratch u0 (per-group partials), bounds u1 ([0]=min+valid, [1]=max+spare),
//         push b13 (pass, posScale, reset, scratchCount).
// Output: pass 0 writes scratch[2*i+0]=partialMin, scratch[2*i+1]=partialMax;
//         pass 1 merges scratch + stored bounds + capture camera into bounds.
// D3D11/SM5 only: groupshared reduction, no wave ops, no atomics.

cbuffer DynCubeBoundsCB : register(b13)
{
    float g_pass;          // 0 = per-group partials, 1 = single-group merge
    float g_posScale;      // position storage scale (capture push constant, 0.001)
    float g_reset;         // 1 = ignore stored bounds this run (recreate / manual reset / toggle rising edge)
    float g_scratchCount;  // pass-0 group count this run (pass 1 loop bound)
};

// Beyond typical interior room scale; prevents sky/far-field reconstructed
// positions (depth ~= 1 or stale history) from expanding the persistent AABB
// indefinitely. Internal only, not a UI setting.
static const float kWorldBoxMaxDist = 100.0;

// Minimum probe-centered safety volume (world-unit half-extents). Unioned with
// the geometry bounds so near-field reflection points P stay inside the parallax
// AABB. Internal only, not a UI setting.
static const float kWorldBoxSafetyHalfExtentX = 4.0;
static const float kWorldBoxSafetyHalfExtentY = 3.0;
static const float kWorldBoxSafetyHalfExtentZ = 4.0;

Texture2DArray<float4> g_posTex      : register(t0);
Texture2DArray<float>  g_contribTex  : register(t1);
Texture2DArray<float4> g_charmaskTex : register(t2);
Texture2D<float4>      g_camCurTex   : register(t3);

RWStructuredBuffer<float4> g_scratch : register(u0);
RWStructuredBuffer<float4> g_bounds  : register(u1);

groupshared float4 s_min[64];
groupshared float4 s_max[64];

// Current-frame geometric position candidate for one capture texel.
// contrib > 0.9 selects exactly the fresh screen samples (capture writes 1.0 for
// fresh, <= 0.5 decaying for history-only), so the proxy represents actual world
// geometry rather than historical radiance. No raw-position texture exists; the
// capture-time pos-threshold compatibility test already bounds blend staleness.
bool BoundsCandidate(uint x, uint y, uint face, uint w, uint h, float3 camCur, out float3 wp)
{
    wp = 0.0;
    if (x >= w || y >= h) return false;
    float4 P = g_posTex.Load(int4(x, y, face, 0));
    float C = g_contribTex.Load(int4(x, y, face, 0));
    if (P.a <= 0.5) return false;      // invalid capture sample
    if (C <= 0.9) return false;        // history-only sample, not current-frame geometry
    float4 M = g_charmaskTex.Load(int4(x, y, face, 0));
    if (M.x > 0.5) return false;       // character texel
    if (!all(isfinite(P.xyz))) return false;
    float3 p = P.xyz / max(g_posScale, 1e-9);
    if (!all(isfinite(p))) return false;
    if (distance(p, camCur) > kWorldBoxMaxDist) return false;  // sky / far field
    wp = p;
    return true;
}

[numthreads(8, 8, 1)]
void main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    uint w, h, el;
    g_posTex.GetDimensions(w, h, el);
    uint gx = (w + 7) / 8;  // groups per face axis; matches the (sz+7)/8 dispatch
    float3 camCur = g_camCurTex.Load(int3(0, 0, 0)).xyz;

    if (g_pass < 0.5)
    {
        // ── Pass 0: one group per 8x8 tile per face → one min/max pair in scratch.
        // Dispatched as (gx, gy, 6) so the group ID maps directly to tile + face.
        uint tileX = gid.x;
        uint tileY = gid.y;
        uint x = tileX * 8 + gtid.x;
        uint y = tileY * 8 + gtid.y;
        uint face = gid.z;

        float3 wp;
        bool ok = BoundsCandidate(x, y, face, w, h, camCur, wp);
        uint li = gtid.y * 8 + gtid.x;
        s_min[li] = ok ? float4(wp, 0.0) : float4(3.402823466e+38, 3.402823466e+38, 3.402823466e+38, 0.0);
        s_max[li] = ok ? float4(wp, 0.0) : float4(-3.402823466e+38, -3.402823466e+38, -3.402823466e+38, 0.0);
        GroupMemoryBarrierWithGroupSync();
        for (uint s = 32; s > 0; s >>= 1)
        {
            if (li < s)
            {
                s_min[li] = min(s_min[li], s_min[li + s]);
                s_max[li] = max(s_max[li], s_max[li + s]);
            }
            GroupMemoryBarrierWithGroupSync();
        }
        if (li == 0)
        {
            uint gy = (h + 7) / 8;
            uint groupIdx = (face * gy + tileY) * gx + tileX;
            g_scratch[2 * groupIdx + 0] = s_min[0];
            g_scratch[2 * groupIdx + 1] = s_max[0];
        }
    }
    else
    {
        // ── Pass 1: single group merges all partials + stored bounds + camera.
        uint li = gtid.y * 8 + gtid.x;
        uint count = (uint)max(g_scratchCount, 1.0);
        float3 gmin = float3(3.402823466e+38, 3.402823466e+38, 3.402823466e+38);
        float3 gmax = float3(-3.402823466e+38, -3.402823466e+38, -3.402823466e+38);
        for (uint i = li; i < count; i += 64)
        {
            gmin = min(gmin, g_scratch[2 * i + 0].xyz);
            gmax = max(gmax, g_scratch[2 * i + 1].xyz);
        }
        s_min[li] = float4(gmin, 0.0);
        s_max[li] = float4(gmax, 0.0);
        GroupMemoryBarrierWithGroupSync();
        for (uint s = 32; s > 0; s >>= 1)
        {
            if (li < s)
            {
                s_min[li] = min(s_min[li], s_min[li + s]);
                s_max[li] = max(s_max[li], s_max[li + s]);
            }
            GroupMemoryBarrierWithGroupSync();
        }
        if (li == 0)
        {
            float3 mergedMin = s_min[0].xyz;
            float3 mergedMax = s_max[0].xyz;
            bool hasGeom = (mergedMin.x <= mergedMax.x);
            // Expand over the stored bounds only when they are usable: flagged
            // valid, finite, non-inverted, and no reset requested. This also
            // neutralizes any uninitialized buffer contents on the first run.
            float4 storedMin = g_bounds[0];
            float4 storedMax = g_bounds[1];
            bool storedOk = (storedMin.w > 0.5)
                && all(isfinite(storedMin.xyz)) && all(isfinite(storedMax.xyz))
                && (storedMin.x <= storedMax.x)
                && (storedMin.y <= storedMax.y)
                && (storedMin.z <= storedMax.z);
            bool useStored = storedOk && (g_reset < 0.5);
            if (useStored)
            {
                mergedMin = min(mergedMin, storedMin.xyz);
                mergedMax = max(mergedMax, storedMax.xyz);
            }
            float valid = (useStored || hasGeom) ? 1.0 : 0.0;
            // Probe containment: the capture camera is always inside its own
            // proxy volume. Unfiltered on purpose — independent of contrib,
            // charmask, and the geometry distance test.
            float4 cam = g_camCurTex.Load(int3(0, 0, 0));
            if (all(isfinite(cam.xyz)))
            {
                mergedMin = min(mergedMin, cam.xyz);
                mergedMax = max(mergedMax, cam.xyz);
                // Minimum probe-centered volume ensures near-field reflection points P stay inside the parallax AABB.
                float3 safeMin = cam.xyz - float3(kWorldBoxSafetyHalfExtentX, kWorldBoxSafetyHalfExtentY, kWorldBoxSafetyHalfExtentZ);
                float3 safeMax = cam.xyz + float3(kWorldBoxSafetyHalfExtentX, kWorldBoxSafetyHalfExtentY, kWorldBoxSafetyHalfExtentZ);
                mergedMin = min(mergedMin, safeMin);
                mergedMax = max(mergedMax, safeMax);
            }
            g_bounds[0] = float4(mergedMin, valid);
            // Spare channel carries the CURRENT-frame geometry result (NOT the latched
            // persistent flag): staged for delayed-validate commit decisions.
            g_bounds[1] = float4(mergedMax, hasGeom ? 1.0f : 0.0f);
        }
    }
}
