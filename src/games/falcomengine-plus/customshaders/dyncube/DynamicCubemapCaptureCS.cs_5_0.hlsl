// DynamicCubemapCaptureCS.cs_5_0.hlsl — Falcom Engine+ generic capture (Phase 1+2)
// Adapted from Skyrim UpdateCubemapCS, not verbatim. Replaces Skyrim FrameBuffer/SharedData
// helpers with Falcom equivalents using cb_scene view_g/proj_g.
// Phase 1: temporal accumulation. Phase 2: character mask exclusion.
// Input : depth t0, color t1, prevColor t2, prevPos t3, prevContrib t4, camPrev t5, mrt0 t6, cb_scene b0, point s0
// Output: curColor u0, curPos u1, curContrib u2, camCur u3, charmask u4

cbuffer cb_scene : register(b0)
{
    row_major float4x4 view_g        : packoffset(c0);
    row_major float4x4 viewInv_g     : packoffset(c4);
    row_major float4x4 proj_g        : packoffset(c8);
    row_major float4x4 projInv_g     : packoffset(c12);
    row_major float4x4 viewProj_g    : packoffset(c16);
    row_major float4x4 viewProjInv_g : packoffset(c20);
    // c24+ not used; packoffsets preserve layout.
};

cbuffer DynCubeCB : register(b13)
{
    float g_captureBoost;
    float g_historyBlend;
    float g_historyPosThreshold;   // world units, compared against scaled positions
    float g_posScale;              // position storage scale (Skyrim 0.001)
    float g_reset;                 // 1 = ignore history this frame (fresh capture / history off)
    float g_characterCapture;      // 1 = capture characters, 0 = exclude characters (default)
    float g_charMaskAvailable;     // 1 = character mask data available
    float g_charComp;              // 0 = char bit in mrt .w (Sora 1st/2nd), 1 = mrt .z shifted bit (Kai)
    float g_charShift;             // bit shift applied to the selected component (Sora 1st/2nd 3, Kai 8)
    float g_captureSoften;         // reserved (soften applies in the variant pass, not here); kept for push/CB alignment
    float g_charInvert;            // 0 = set bit means character (Kai), 1 = clear bit means character (Sora 1st/2nd: char = !(mrt.w & 8)); appended last, push count 10 -> 11
    float g_sparkleReject;         // 0 = off (default), 1 = reject isolated HDR spikes at depth edges + non-finite input; appended last, push count 11 -> 12
    float g_contribThreshold;      // world-box bounds candidate contrib cutoff (folded pass 0), push count 12 -> 13
    float g_writeCharmask;         // 1 = store character mask (debug view 7), 0 = skip store (folded bounds uses the in-register flag), push count 13 -> 14
};

#include "dyncube_common.hlsli"

Texture2D<float>       g_depthTex      : register(t0);
Texture2D<float4>      g_colorTex      : register(t1);
Texture2DArray<float4> g_prevColorTex  : register(t2);
Texture2DArray<float4> g_prevPosTex    : register(t3);
Texture2DArray<float>  g_prevContribTex: register(t4);
Texture2D<float4>      g_camPrevTex    : register(t5);
Texture2D<uint4>       g_mrt0Tex       : register(t6);
TextureCube<float4>  g_vanillaTex    : register(t7);  // game vanilla cube: painted where dynamic has no info (never black canvas)
SamplerState         g_pointClamp    : register(s0);

RWTexture2DArray<float4> g_outColor   : register(u0);
RWTexture2DArray<float4> g_outPos     : register(u1);
RWTexture2DArray<float>  g_outContrib : register(u2);
RWTexture2D<float4>      g_camCurTex  : register(u3);
RWTexture2DArray<float4> g_outCharMask: register(u4);
RWStructuredBuffer<float4> g_boundsScratch : register(u5);  // world-box pass-0 partials, folded here

// Folded world-box pass 0: per-group min/max candidates, reduced identically to
// DynCubeBoundsReduceCS pass 0 (same sentinels, same index layout) so the merge
// pass and the resulting bounds are unchanged.
groupshared float4 s_boundsMin[64];
groupshared float4 s_boundsMax[64];

float3 WorldToViewDir(float3 dir)
{
    return mul((float3x3)view_g, dir);
}

float2 ViewToUV(float3 viewDir)
{
    float4 clip = mul(proj_g, float4(viewDir, 0.0));
    if (abs(clip.w) < 1e-6) return float2(-2, -2);
    if (clip.w <= 0.0) return float2(-2, -2);
    float2 ndc = clip.xy / clip.w;
    return ndc * float2(0.5, -0.5) + 0.5;
}

bool IsOutside(float2 uv)
{
    return any(uv < 0.0) || any(uv > 1.0);
}

float3 GetSamplingVector(uint3 tid, uint w, uint h)
{
    float2 st = float2(tid.xy) / float2(w, h);
    float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - 1.0;
    float3 v = 0;
    switch (tid.z)
    {
        case 0: v = float3( 1.0,  uv.y, -uv.x); break;
        case 1: v = float3(-1.0,  uv.y,  uv.x); break;
        case 2: v = float3( uv.x,  1.0, -uv.y); break;
        case 3: v = float3( uv.x, -1.0,  uv.y); break;
        case 4: v = float3( uv.x,  uv.y,  1.0); break;
        case 5: v = float3(-uv.x,  uv.y, -1.0); break;
    }
    return normalize(v);
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint w, h, el;
    g_outColor.GetDimensions(w, h, el);
    // No bounds early-out: the dispatch is exactly (size+7)/8 and every supported
    // cube size is a multiple of 8, so every thread is in bounds. All threads must
    // reach the folded world-box barrier below (sync in uniform flow control).

    // Falcom game samples t17 with (1,-1,-1)*reflect(...); store physical
    // radiance R at the same TextureCube address the lookup samples, so the
    // capture transform must equal the lookup transform: (1,-1,-1).
    // Baked sampling convention: store physical radiance so that the game's bare
    // (1,-1,-1)*reflect(...) lookup fetches the correct texel with NO runtime
    // negation anywhere (lighting, glass, future consumers). The conjugate of the
    // lookup flip through the reference capture negation (-GetSamplingVector) is
    // X-only: float3(-1, 1, 1). Do NOT "fix" this to (1,-1,-1) without also
    // re-adding a lookup negate on every consumer (see git 5931055f).
    float3 worldDir = float3(-1.0, 1.0, 1.0) * GetSamplingVector(dtid, w, h);
    float3 viewDir = WorldToViewDir(worldDir);
    // Projection cull disabled for diagnostic (see Phase 0B). Keep IsOutside only.
    float2 uv = ViewToUV(viewDir);
    bool inside = !IsOutside(uv);

    // ── Current screen sample (Phase 0B Sub-step B path, unchanged) ──
    float3 curCol = 0.0;
    float3 curPos = 0.0;
    bool curValid = false;
    float3 forcedVanilla = 0.0;
    bool hasForcedVanilla = false;
    if (inside)
    {
        float rawDepth = g_depthTex.SampleLevel(g_pointClamp, uv, 0);
        if (rawDepth < 1.0 - 1e-5)
        {
            float2 ndc = float2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0);
            float4 clipPos = float4(ndc, rawDepth, 1.0);
            float4 worldH = mul(viewProjInv_g, clipPos);
            curPos = worldH.xyz / max(worldH.w, 1e-6);
            curCol = g_colorTex.SampleLevel(g_pointClamp, uv, 0).rgb;
            // NOTE: capture soften is intentionally NOT baked here (it would reach
            // lighting too); soften applies in the variant pass for global pushes.
            // NOTE: capture brightness is intentionally NOT baked here (sample-time
            // lighting multiply only).
            curValid = true;
            curValid = true;
        }

    float3 invalidCol = 0.0;
    bool hasInvalidCol = false;
    bool depthInvalid = !(rawDepth > 0.0 && rawDepth < 1.0 - 1e-5);
    if (g_sparkleReject > 0.5 && depthInvalid)
    {
        invalidCol = g_colorTex.SampleLevel(g_pointClamp, uv, 0).rgb;
        hasInvalidCol = true;
    }
    if (g_sparkleReject > 0.5 && curValid)
    {
        float curHeader = curCol.x + curCol.y + curCol.z + curPos.x + curPos.y + curPos.z;
        if (!isfinite(curHeader))
        {
            curValid = false;
        }
        else
        {
            uint cW, cH;
            g_colorTex.GetDimensions(cW, cH);
            float2 texel = 1.0 / float2(max(cW, 1u), max(cH, 1u));
            float centerLum = dot(curCol, float3(0.2126, 0.7152, 0.0722));
            if (centerLum > 1.0)
            {
                float unpackMul, unpackAdd;
                DynCubeGetDepthUnpackConsts(unpackMul, unpackAdd);
                float centerLin = DynCubeLinearizeDepth(rawDepth, unpackMul, unpackAdd);
                if (centerLin > 0.0 && centerLin < DynCubeFltMax)
                {
                    float depthTol = max(centerLin * 0.02, 0.05);
                    float brightFloor = max(centerLum * 0.25, 1.0);
                    uint validNeigh = 0u;
                    uint brightNeigh = 0u;
                    uint sameDepthNeigh = 0u;
                    uint nearerNeigh = 0u;
                    float neighSumLum = 0.0;
                    for (int oy = -1; oy <= 1; ++oy)
                    {
                        for (int ox = -1; ox <= 1; ++ox)
                        {
                            if (ox == 0 && oy == 0) continue;
                            float2 nuv = uv + float2((float)ox, (float)oy) * texel;
                            if (IsOutside(nuv)) continue;
                            float nDepth = g_depthTex.SampleLevel(g_pointClamp, nuv, 0);
                            if (!(nDepth > 0.0 && nDepth < 1.0 - 1e-5)) continue;
                            float3 nCol = g_colorTex.SampleLevel(g_pointClamp, nuv, 0).rgb;
                            float nHeader = nCol.x + nCol.y + nCol.z;
                            if (!isfinite(nHeader)) continue;
                            float nLin = DynCubeLinearizeDepth(nDepth, unpackMul, unpackAdd);
                            if (!(nLin > 0.0 && nLin < DynCubeFltMax)) continue;
                            float nLum = dot(nCol, float3(0.2126, 0.7152, 0.0722));
                            validNeigh++;
                            neighSumLum += nLum;
                            if (nLum >= brightFloor) brightNeigh++;
                            if (abs(nLin - centerLin) <= depthTol) sameDepthNeigh++;
                            else if (nLin < centerLin - depthTol) nearerNeigh++;
                        }
                    }
                    if (validNeigh == 0u)
                    {
                        float3 vanillaCol = g_vanillaTex.SampleLevel(g_pointClamp, GetSamplingVector(dtid, w, h), 0).rgb;
                        float vanillaHeader = vanillaCol.x + vanillaCol.y + vanillaCol.z;
                        if (isfinite(vanillaHeader))
                        {
                            float vanillaLum = dot(vanillaCol, float3(0.2126, 0.7152, 0.0722));
                            if (vanillaLum > 0.01 && centerLum > max(vanillaLum * 4.0, 1.0))
                            {
                                forcedVanilla = vanillaCol;
                                hasForcedVanilla = true;
                            }
                        }
                    }
                    else
                    {
                        bool depthOutlier = (sameDepthNeigh == 0u)
                            || (sameDepthNeigh * 2u <= validNeigh && nearerNeigh * 2u >= validNeigh);
                        if (brightNeigh <= 2u && depthOutlier
                            && centerLum > max((neighSumLum / (float)validNeigh) * 4.0, 1.0))
                        {
                            float3 vanillaReplace = g_vanillaTex.SampleLevel(g_pointClamp, GetSamplingVector(dtid, w, h), 0).rgb;
                            float replaceHeader = vanillaReplace.x + vanillaReplace.y + vanillaReplace.z;
                            if (isfinite(replaceHeader))
                            {
                                forcedVanilla = vanillaReplace;
                                hasForcedVanilla = true;
                            }
                            else
                            {
                                curValid = false;
                            }
                        }
                    }
                    if (!hasForcedVanilla)
                    {
                        float3 vanillaStable = g_vanillaTex.SampleLevel(g_pointClamp, GetSamplingVector(dtid, w, h), 0).rgb;
                        float vanillaStableHeader = vanillaStable.x + vanillaStable.y + vanillaStable.z;
                        if (isfinite(vanillaStableHeader))
                        {
                            float vanillaStableLum = dot(vanillaStable, float3(0.2126, 0.7152, 0.0722));
                            if (vanillaStableLum > 0.01 && centerLum > max(vanillaStableLum * 4.0, 1.0))
                            {
                                forcedVanilla = vanillaStable;
                                hasForcedVanilla = true;
                            }
                        }
                    }
                }
            }
        }
    }
    if (g_sparkleReject > 0.5 && !curValid && hasInvalidCol && !hasForcedVanilla)
    {
        float invalidHeader = invalidCol.x + invalidCol.y + invalidCol.z;
        if (!isfinite(invalidHeader))
        {
            float3 vanillaInvalid = g_vanillaTex.SampleLevel(g_pointClamp, GetSamplingVector(dtid, w, h), 0).rgb;
            float vanillaInvalidHeader = vanillaInvalid.x + vanillaInvalid.y + vanillaInvalid.z;
            if (isfinite(vanillaInvalidHeader))
            {
                forcedVanilla = vanillaInvalid;
                hasForcedVanilla = true;
            }
        }
        else
        {
            float invalidLum = dot(invalidCol, float3(0.2126, 0.7152, 0.0722));
            if (invalidLum > 1.0)
            {
                float3 vanillaSky = g_vanillaTex.SampleLevel(g_pointClamp, GetSamplingVector(dtid, w, h), 0).rgb;
                float vanillaSkyHeader = vanillaSky.x + vanillaSky.y + vanillaSky.z;
                if (isfinite(vanillaSkyHeader))
                {
                    float vanillaSkyLum = dot(vanillaSky, float3(0.2126, 0.7152, 0.0722));
                    if (vanillaSkyLum > 0.01 && invalidLum > max(vanillaSkyLum * 4.0, 1.0))
                    {
                        forcedVanilla = vanillaSky;
                        hasForcedVanilla = true;
                    }
                }
            }
        }
    }
    }

    // ── Character mask (game-specific bit: Kai (mrtTexture0.z >> 8) & 1 set,
    //    Sora 1st/2nd !(mrtTexture0.w & 8) via g_charComp/g_charShift/g_charInvert) — sample at the projected screen UV, not the cubemap texel. ──
    bool isCharacter = false;
    if (g_charMaskAvailable > 0.5f && inside)
    {
        uint mrtW, mrtH;
        g_mrt0Tex.GetDimensions(mrtW, mrtH);
        int2 mrtPixel = int2(min(uv * float2(mrtW, mrtH) + 0.25f, float2(mrtW - 1, mrtH - 1)));
        uint4 mrt0 = g_mrt0Tex.Load(int3(mrtPixel, 0));
        uint charWord = (g_charComp > 0.5f) ? mrt0.z : mrt0.w;
        isCharacter = ((((charWord >> (uint)g_charShift) & 1u) != 0u) != (g_charInvert > 0.5f));
    }

    // ── Character mask exclusion (Phase 2) ──
    if (isCharacter && g_characterCapture < 0.5f)
    {
        // Character pixel but character capture disabled -> treat as invalid to preserve history
        curValid = false;
    }

    // ── Previous history (same face/texel layout — direct Load) ──
    // prevColor is loaded lazily: the off-screen preservation path (below) skips
    // both its load and its store. Position is always loaded (camera comp).
    float4 prevPos   = g_prevPosTex.Load(int4(dtid, 0));
    float  prevContrib = g_prevContribTex.Load(int4(dtid, 0)).x;
    float4 camPrev   = g_camPrevTex.Load(int3(0, 0, 0));
    float3 camCur    = viewInv_g._m30_m31_m32;

    bool prevValid = (prevPos.a > 0.5);
    if (g_reset > 0.5) prevValid = false;

    // Camera-motion compensation: re-anchor stored (scaled) world position to current camera
    float3 prevPosComp = prevPos.xyz;
    if (prevValid && camPrev.w > 0.5)
    {
        prevPosComp += (camPrev.xyz - camCur.xyz) * g_posScale;
    }

    bool skipColorStore = false;
    float3 outCol;
    float3 outPos;
    float  outValid;
    float  outContrib;
    if (hasForcedVanilla)
    {
        outCol = forcedVanilla;
        outPos = 0.0;
        outValid = 0.0;
        outContrib = 0.0;
    }
    else if (curValid)
    {
        float3 curPosScaled = curPos * g_posScale;
        float posDelta = length(prevPosComp - curPosScaled);
        bool compatible = (!prevValid) || (posDelta < (g_historyPosThreshold * g_posScale));
        if (compatible)
        {
            float4 prevColor = g_prevColorTex.Load(int4(dtid, 0));
            outCol = lerp(prevColor.rgb, curCol, g_historyBlend);
            outPos = lerp(prevPosComp, curPosScaled, g_historyBlend);
        }
        else
        {
            outCol = curCol;
            outPos = curPosScaled;
        }
        outValid = 1.0;
        outContrib = 1.0;
    }
    else if (prevValid && !inside)
    {
        // Off-screen fast path: no current screen content. Only the camera-anchored
        // position and the aged contribution are stored. Color is copied forward
        // whenever the read set is fresher (contrib 1.0 = written by the previous
        // capture), so the two history sets can never diverge when a direction is
        // visible for only one capture before going off-screen. In steady-state
        // off-screen (both sets already agree) the color load/store is skipped.
        outPos = prevPosComp;
        outValid = prevPos.a;
        outContrib = prevContrib * 0.5;
        if (prevContrib > 0.75f)
        {
            outCol = g_prevColorTex.Load(int4(dtid, 0)).rgb;
        }
        else
        {
            outCol = 0.0;  // unused: color store skipped below
            skipColorStore = true;
        }
    }
    else if (prevValid)
    {
        // No current sample — preserve previous history
        float4 prevColor = g_prevColorTex.Load(int4(dtid, 0));
        outCol = prevColor.rgb;
        outPos = prevPosComp;
        outValid = prevPos.a;
        outContrib = prevContrib * 0.5;
    }
    else
    {
        // No live or history content: paint the vanilla cube instead of a black
        // canvas, so every consumer (lighting fallback AND raw t17 samplers like
        // glass) sees vanilla where dynamic has no info. Sampled at the texel's
        // address direction — the same convention the game uses — so dynamic and
        // vanilla texels agree. Validity/contrib/pos stay 0: the resolver still
        // blends to vanilla and worldbox still ignores these texels.
        outCol = g_vanillaTex.SampleLevel(g_pointClamp, GetSamplingVector(dtid, w, h), 0).rgb;
        outPos = 0.0;
        outValid = 0.0;
        outContrib = 0.0;
    }

    // Over-ceiling guard for the history store. The blend above cannot decay a
    // non-finite value -- lerp(Inf, Inf, t) == Inf and lerp(NaN, NaN, t) == NaN --
    // so a single over-range sample latches a texel until the history is manually
    // cleared. It arrives two ways: the scene target already holds Inf/NaN, or a
    // bright transient overflows fp16 on the store below (r16g16b16a16_float
    // maxes at 65504, so anything above commits as +Inf). Capping also heals
    // texels latched by an earlier capture, because reading back Inf and writing
    // min(Inf, ceiling) breaks the latch. NaN needs an explicit test: it compares
    // false against every ceiling and min() is undefined for it.
    const float kDynCubeCeiling = 1024.0;
    if (all(isfinite(outCol)))
    {
        outCol = min(outCol, kDynCubeCeiling);
    }
    else
    {
        // Repaint from vanilla with validity dropped, matching the no-content
        // branch above: raw t17 consumers (glass) read color without consulting
        // validity, so they need real radiance here rather than black.
        outCol = g_vanillaTex.SampleLevel(g_pointClamp, GetSamplingVector(dtid, w, h), 0).rgb;
        if (!all(isfinite(outCol))) outCol = 0.0;
        outPos = 0.0;
        outValid = 0.0;
        outContrib = 0.0;
    }

    if (!skipColorStore)
        g_outColor[dtid] = float4(max(0.0, outCol), outValid);
    g_outPos[dtid]     = float4(outPos, outValid);
    g_outContrib[dtid] = outContrib;

    // Character mask (1 = character, 0 = non-character). Only the debug preview
    // consumes this texture; the folded bounds reduction below uses the
    // in-register flag, so the store is skipped unless the debug view asks for it.
    if (g_writeCharmask > 0.5f)
    {
        float charMask = isCharacter ? 1.0 : 0.0;
        g_outCharMask[dtid] = float4(charMask, charMask, charMask, 1.0);
    }

    // ── Folded world-box pass 0 (per-group partials) ──
    // Mirrors DynCubeBoundsReduceCS BoundsCandidate + pass-0 reduction: same
    // validity/contrib/character gates (with the same <= NaN semantics), same
    // world-space conversion, distance cap, sentinels and scratch indexing.
    // Consumes the final in-register values (including the over-ceiling guard
    // resets); the former texture read saw the same values rounded through the
    // fp16 history store, so bounds can differ only by that store rounding.
    {
        const float kWorldBoxMaxDist = 100.0;
        const float kFltMax = 3.402823466e+38;
        bool boundsOk = !(outValid <= 0.5f) && !(outContrib <= g_contribThreshold) && !isCharacter;
        float3 boundsPos = 0.0;
        if (boundsOk)
        {
            if (!all(isfinite(outPos))) boundsOk = false;
            else
            {
                boundsPos = outPos / max(g_posScale, 1e-9);
                if (!all(isfinite(boundsPos))) boundsOk = false;
                else if (distance(boundsPos, camCur) > kWorldBoxMaxDist) boundsOk = false;
            }
        }
        uint li = (dtid.y & 7u) * 8u + (dtid.x & 7u);
        s_boundsMin[li] = boundsOk ? float4(boundsPos, 0.0) : float4(kFltMax, kFltMax, kFltMax, 0.0);
        s_boundsMax[li] = boundsOk ? float4(boundsPos, 0.0) : float4(-kFltMax, -kFltMax, -kFltMax, 0.0);
        GroupMemoryBarrierWithGroupSync();
        for (uint s = 32u; s > 0u; s >>= 1)
        {
            if (li < s)
            {
                s_boundsMin[li] = min(s_boundsMin[li], s_boundsMin[li + s]);
                s_boundsMax[li] = max(s_boundsMax[li], s_boundsMax[li + s]);
            }
            GroupMemoryBarrierWithGroupSync();
        }
        if (li == 0u)
        {
            uint boundsGx = (w + 7u) / 8u;
            uint boundsGy = (h + 7u) / 8u;
            uint groupIdx = (dtid.z * boundsGy + (dtid.y / 8u)) * boundsGx + (dtid.x / 8u);
            g_boundsScratch[2u * groupIdx + 0u] = s_boundsMin[0];
            g_boundsScratch[2u * groupIdx + 1u] = s_boundsMax[0];
        }
    }

    // Record current camera position for next frame (thread 0 of face 0)
    if (dtid.x == 0 && dtid.y == 0 && dtid.z == 0)
        g_camCurTex[uint2(0, 0)] = float4(camCur, 1.0);
}