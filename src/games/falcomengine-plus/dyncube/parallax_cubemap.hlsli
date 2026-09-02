// parallax_cubemap.hlsli — Falcom Engine+ generic parallax-corrected cubemap lookup.
// Corrects a camera-centered cubemap sample for the offset between the reflection
// surface point and the probe (capture) origin, using a finite axis-aligned probe
// box and standard slab ray/box intersection.
// Generic across Falcom Engine+ titles; no game-specific names or bindings.
// The corrected direction is computed in world space; the caller applies the
// game's existing cube-space flip before sampling.

#ifndef __DYNCUBE_PARALLAX_CUBEMAP_HLSL__
#define __DYNCUBE_PARALLAX_CUBEMAP_HLSL__

static const float kDynCubeParallaxEpsilon = 1e-6;
static const float kDynCubeParallaxMaxT = 3.402823466e+38;

// Standard slab intersection of ray (ro + t*rd, t>0) with AABB [bmin, bmax].
// Returns false when there is no positive intersection (parallel miss, ray behind box).
bool DynCubeRayBoxIntersect(float3 ro, float3 rd, float3 bmin, float3 bmax,
                            out float tmin, out float tmax)
{
    tmin = 0.0;
    tmax = kDynCubeParallaxMaxT;

    // X axis
    if (abs(rd.x) < kDynCubeParallaxEpsilon) {
        if (ro.x < bmin.x || ro.x > bmax.x) return false;
    } else {
        float inv = 1.0 / rd.x;
        float t1 = (bmin.x - ro.x) * inv;
        float t2 = (bmax.x - ro.x) * inv;
        tmin = max(tmin, min(t1, t2));
        tmax = min(tmax, max(t1, t2));
    }
    // Y axis
    if (abs(rd.y) < kDynCubeParallaxEpsilon) {
        if (ro.y < bmin.y || ro.y > bmax.y) return false;
    } else {
        float inv = 1.0 / rd.y;
        float t1 = (bmin.y - ro.y) * inv;
        float t2 = (bmax.y - ro.y) * inv;
        tmin = max(tmin, min(t1, t2));
        tmax = min(tmax, max(t1, t2));
    }
    // Z axis
    if (abs(rd.z) < kDynCubeParallaxEpsilon) {
        if (ro.z < bmin.z || ro.z > bmax.z) return false;
    } else {
        float inv = 1.0 / rd.z;
        float t1 = (bmin.z - ro.z) * inv;
        float t2 = (bmax.z - ro.z) * inv;
        tmin = max(tmin, min(t1, t2));
        tmax = min(tmax, max(t1, t2));
    }

    if (tmax < tmin) return false;  // no overlap
    if (tmax <= 0.0) return false;  // box entirely behind the ray
    return true;
}

// Parallax-correct the reflection ray at surface point P against a probe box of
// size boxSize centered at probeOrigin. On success outDir = normalize(Q - O) and
// outFace = probe-box exit face index (0..5); on failure outDir = R, outFace = -1.
bool DynCubeParallaxCorrect(float3 P, float3 R, float3 probeOrigin, float3 boxSize,
                            out float3 outDir, out int outFace)
{
    outDir = R;
    outFace = -1;

    float3 boxMin = probeOrigin - boxSize * 0.5;
    float3 boxMax = probeOrigin + boxSize * 0.5;

    float tmin, tmax;
    if (!DynCubeRayBoxIntersect(P, R, boxMin, boxMax, tmin, tmax)) return false;

    // First positive intersection; if P is inside the box (tmin <= 0), use the exit.
    float t = (tmin > 0.0) ? tmin : tmax;
    float3 Q = P + t * R;
    float3 d = Q - probeOrigin;
    if (dot(d, d) < 1e-12) return false;  // degenerate: ray passes through the origin

    outDir = normalize(d);

    // Exit face: axis with the largest offset from the box center.
    float ax = abs(d.x), ay = abs(d.y), az = abs(d.z);
    int axis = (ax >= ay && ax >= az) ? 0 : (ay >= az) ? 1 : 2;
    int sign = (axis == 0) ? (d.x > 0.0 ? 1 : 0)
             : (axis == 1) ? (d.y > 0.0 ? 1 : 0)
                           : (d.z > 0.0 ? 1 : 0);
    outFace = axis * 2 + sign;
    return true;
}

// Distinct color per probe-box exit face for the Parallax Debug view.
float3 DynCubeParallaxFaceColor(int face)
{
    switch (face) {
        case 0: return float3(1.0, 0.0, 0.0);  // +X red
        case 1: return float3(0.0, 1.0, 0.0);  // -X green
        case 2: return float3(0.0, 0.0, 1.0);  // +Y blue
        case 3: return float3(1.0, 1.0, 0.0);  // -Y yellow
        case 4: return float3(1.0, 0.0, 1.0);  // +Z magenta
        default: return float3(0.0, 1.0, 1.0); // -Z cyan
    }
}

// Position-aware dynamic cubemap lookup.
// Evaluates a small local set of candidate directions around the reflection direction,
// reading each candidate's stored world position (histPos) and scoring it against the
// reflection ray P + t*rayDirWorld. The original direction is the baseline candidate;
// the rest are distributed progressively around it (golden-angle spiral, small->large
// angular offsets). Candidates are rejected when their stored position is invalid,
// behind P, or farther from the ray than the current best. No brute-force search.
// On success outDirCube = the best candidate direction (defaults to rayDirCube) and the
// return value is true if any valid in-front candidate was found.
bool DynCubePosAwareLookup(TextureCube<float4> posTex, SamplerState posSmp,
                           float3 P, float3 rayDirWorld, float3 rayDirCube,
                           uint sampleCount, float spread,
                           out float3 outDirCube)
{
    outDirCube = rayDirCube;

    // Tangent basis around the cube-space reflection direction.
    float3 up = (abs(rayDirCube.y) < 0.999) ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T1 = normalize(cross(up, rayDirCube));
    float3 T2 = cross(rayDirCube, T1);

    const float goldenAngle = 2.399963229728653;  // 2*pi*(1 - 1/phi)

    float bestDist = kDynCubeParallaxMaxT;
    bool found = false;

    // Candidate 0: the original reflection direction (baseline).
    float4 hp0 = posTex.SampleLevel(posSmp, rayDirCube, 0);
    if (hp0.a > 0.5) {
        float3 w0 = hp0.xyz * 1000.0;  // un-scale stored pos (g_posScale = 0.001)
        float along0 = dot(w0 - P, rayDirWorld);
        if (along0 > 0.0) {
            float3 perp0 = w0 - P - rayDirWorld * along0;
            bestDist = length(perp0);
            found = true;
        }
    }

    // Progressive spiral candidates: small -> large angular offsets.
    uint denom = max(sampleCount - 1, 1u);
    for (uint i = 1; i < sampleCount; ++i) {
        float radius = spread * (float(i) / float(denom));
        float phi = goldenAngle * float(i);
        float3 D = normalize(rayDirCube + T1 * (cos(phi) * radius) + T2 * (sin(phi) * radius));
        float4 hp = posTex.SampleLevel(posSmp, D, 0);
        if (hp.a <= 0.5) continue;
        float3 w = hp.xyz * 1000.0;
        float along = dot(w - P, rayDirWorld);
        if (along <= 0.0) continue;  // behind the reflection surface
        float3 perp = w - P - rayDirWorld * along;
        float dist = length(perp);
        if (dist < bestDist) {
            bestDist = dist;
            outDirCube = D;
            found = true;
        }
    }
    return found;
}

#endif  // __DYNCUBE_PARALLAX_CUBEMAP_HLSL__