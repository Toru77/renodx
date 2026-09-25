// DynCubeBoundsReduceCS.cs_5_0.hlsl — Falcom Engine+ world-fixed parallax bounds reduction.
// Accumulates a persistent world-space AABB over the dynamic cubemap's captured
// geometry so the parallax proxy stays fixed in the world instead of following
// the camera. Expand-only; reset only via recreation or the manual Reset button.
// Input : pos t0 (rgb=scaled pos, a=validity), contrib t1, charmask t2, camCur t3,
//         scratch u0 (per-group partials), bounds u1 ([0]=min+valid, [1]=max+spare),
//         faceExt u2 ([0]=(+X,+Y,+Z,faceMask), [1]=(-X,-Y,-Z,spare)),
//         push b13 (pass, posScale, reset, scratchCount, contribThreshold).
// Output: pass 0 writes scratch[2*i+0]=partialMin, scratch[2*i+1]=partialMax;
//         pass 1 merges scratch + stored bounds + capture camera into bounds,
//         and writes per-face applied extents into faceExt.
// D3D11/SM5 only: groupshared reduction, no wave ops, no atomics.

cbuffer DynCubeBoundsCB : register(b13)
{
    float g_pass;          // 0 = per-group partials, 1 = single-group merge
    float g_posScale;      // position storage scale (capture push constant, 0.001)
    float g_reset;         // 1 = ignore stored bounds this run (recreate / manual reset / toggle rising edge)
    float g_scratchCount;  // pass-0 group count this run (pass 1 loop bound)
    float g_contribThreshold;  // bounds-candidate contrib cutoff (UI slider, default 0.25)
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
RWStructuredBuffer<float4> g_faceExt : register(u2);

groupshared float4 s_min[64];
groupshared float4 s_max[64];
groupshared float s_face[6 * 64];  // per-thread per-world-direction max displacement

// Texture face (gid.z) -> world principal axis. Capture stores
// GetSamplingVector(tid) flipped by (1,-1,-1) (capture :72-77,92), so the
// world-space content per face is: 0:+X, 1:-X, 2:-Y, 3:+Y, 4:-Z, 5:+Z.
// Extent slots below are world-ordered [+X,-X,+Y,-Y,+Z,-Z].

// Current-frame geometric position candidate for one capture texel.
// g_contribThreshold selects fresh screen samples (capture writes 1.0 for
// fresh, x0.5 decaying per missed capture for history-only), so the proxy
// represents actual world geometry rather than historical radiance. No
// raw-position texture exists; the capture-time pos-threshold compatibility
// test already bounds blend staleness.
bool BoundsCandidate(uint x, uint y, uint face, uint w, uint h, float3 camCur, out float3 wp)
{
    wp = 0.0;
    if (x >= w || y >= h) return false;
    float4 P = g_posTex.Load(int4(x, y, face, 0));
    float C = g_contribTex.Load(int4(x, y, face, 0));
    if (P.a <= 0.5) return false;      // invalid capture sample
    if (C <= g_contribThreshold) return false;  // stale history, not recent geometry
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
        uint gpf = max(gx * ((h + 7) / 8), 1u);  // groups per face; pass-0 groups are face-major
        bool camOk = all(isfinite(camCur));
        float3 gmin = float3(3.402823466e+38, 3.402823466e+38, 3.402823466e+38);
        float3 gmax = float3(-3.402823466e+38, -3.402823466e+38, -3.402823466e+38);
        // Per-thread per-world-direction max displacement from the capture
        // origin, in world order [+X,-X,+Y,-Y,+Z,-Z].
        float ePX = -3.402823466e+38;
        float eNX = -3.402823466e+38;
        float ePY = -3.402823466e+38;
        float eNY = -3.402823466e+38;
        float ePZ = -3.402823466e+38;
        float eNZ = -3.402823466e+38;
        for (uint i = li; i < count; i += 64)
        {
            float3 gmn = g_scratch[2 * i + 0].xyz;
            float3 gmx = g_scratch[2 * i + 1].xyz;
            gmin = min(gmin, gmn);
            gmax = max(gmax, gmx);
            // Per-group validity mirrors hasGeom: empty groups hold inverted sentinels.
            bool gok = (gmx.x >= gmn.x) && all(isfinite(gmn)) && all(isfinite(gmx));
            if (gok && camOk)
            {
                uint grpFace = min(i / gpf, 5u);
                float disp;
                if (grpFace == 0) disp = gmx.x - camCur.x;       // world +X
                else if (grpFace == 1) disp = camCur.x - gmn.x;  // world -X
                else if (grpFace == 2) disp = camCur.y - gmn.y;  // world -Y (face 2 holds -Y content)
                else if (grpFace == 3) disp = gmx.y - camCur.y;  // world +Y (face 3 holds +Y content)
                else if (grpFace == 4) disp = camCur.z - gmn.z;  // world -Z (face 4 holds -Z content)
                else disp = gmx.z - camCur.z;                    // world +Z (face 5 holds +Z content)
                if (grpFace == 0) ePX = max(ePX, disp);
                else if (grpFace == 1) eNX = max(eNX, disp);
                else if (grpFace == 3) ePY = max(ePY, disp);
                else if (grpFace == 2) eNY = max(eNY, disp);
                else if (grpFace == 5) ePZ = max(ePZ, disp);
                else eNZ = max(eNZ, disp);
            }
        }
        s_min[li] = float4(gmin, 0.0);
        s_max[li] = float4(gmax, 0.0);
        s_face[li * 6 + 0] = ePX;
        s_face[li * 6 + 1] = eNX;
        s_face[li * 6 + 2] = ePY;
        s_face[li * 6 + 3] = eNY;
        s_face[li * 6 + 4] = ePZ;
        s_face[li * 6 + 5] = eNZ;
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
            float fPX = -3.402823466e+38;
            float fNX = -3.402823466e+38;
            float fPY = -3.402823466e+38;
            float fNY = -3.402823466e+38;
            float fPZ = -3.402823466e+38;
            float fNZ = -3.402823466e+38;
            for (uint t = 0u; t < 64u; ++t)
            {
                fPX = max(fPX, s_face[t * 6 + 0]);
                fNX = max(fNX, s_face[t * 6 + 1]);
                fPY = max(fPY, s_face[t * 6 + 2]);
                fNY = max(fNY, s_face[t * 6 + 3]);
                fPZ = max(fPZ, s_face[t * 6 + 4]);
                fNZ = max(fNZ, s_face[t * 6 + 5]);
            }
            // Per-face extents -> AABB around the capture origin. A side with no
            // valid data (extent <= 0) falls back to the safety half-extent for
            // that side only. hasGeom keeps the global any-candidate semantics.
            bool hasGeom = (s_min[0].x <= s_max[0].x);
            uint faceMask = 0u;  // bit0:+X bit1:-X bit2:+Y bit3:-Y bit4:+Z bit5:-Z
            float pX = kWorldBoxSafetyHalfExtentX;
            float nX = kWorldBoxSafetyHalfExtentX;
            float pY = kWorldBoxSafetyHalfExtentY;
            float nY = kWorldBoxSafetyHalfExtentY;
            float pZ = kWorldBoxSafetyHalfExtentZ;
            float nZ = kWorldBoxSafetyHalfExtentZ;
            float3 mergedMin;
            float3 mergedMax;
            if (camOk)
            {
                if (fPX > 0.0f) { pX = fPX; faceMask |= 1u; }
                if (fNX > 0.0f) { nX = fNX; faceMask |= 2u; }
                if (fPY > 0.0f) { pY = fPY; faceMask |= 4u; }
                if (fNY > 0.0f) { nY = fNY; faceMask |= 8u; }
                if (fPZ > 0.0f) { pZ = fPZ; faceMask |= 16u; }
                if (fNZ > 0.0f) { nZ = fNZ; faceMask |= 32u; }
                mergedMin = camCur - float3(nX, nY, nZ);
                mergedMax = camCur + float3(pX, pY, pZ);
            }
            else
            {
                // Degenerate camera: behave exactly as before (global box only).
                mergedMin = s_min[0].xyz;
                mergedMax = s_max[0].xyz;
            }
            g_faceExt[0] = float4(pX, pY, pZ, asfloat(faceMask));
            g_faceExt[1] = float4(nX, nY, nZ, 0.0);
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
