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
    float3 _pad0;
};

Texture2D<float>       g_depthTex      : register(t0);
Texture2D<float4>      g_colorTex      : register(t1);
Texture2DArray<float4> g_prevColorTex  : register(t2);
Texture2DArray<float4> g_prevPosTex    : register(t3);
Texture2DArray<float>  g_prevContribTex: register(t4);
Texture2D<float4>      g_camPrevTex    : register(t5);
Texture2D<uint4>       g_mrt0Tex       : register(t6);
SamplerState           g_pointClamp    : register(s0);

RWTexture2DArray<float4> g_outColor   : register(u0);
RWTexture2DArray<float4> g_outPos     : register(u1);
RWTexture2DArray<float>  g_outContrib : register(u2);
RWTexture2D<float4>      g_camCurTex  : register(u3);
RWTexture2DArray<float4> g_outCharMask: register(u4);

float3 WorldToViewDir(float3 dir)
{
    return mul((float3x3)view_g, dir);
}

float2 ViewToUV(float3 viewDir)
{
    float4 clip = mul(proj_g, float4(viewDir, 0.0));
    if (abs(clip.w) < 1e-6) return float2(-2, -2);
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
    if (dtid.x >= w || dtid.y >= h || dtid.z >= 6) return;

    // Falcom game samples t17 with (1,-1,-1)*reflect(...); conjugate the reference
    // capture negation (-GetSamplingVector) through that flip -> negate X only.
    float3 worldDir = float3(-1.0, 1.0, 1.0) * GetSamplingVector(dtid, w, h);
    float3 viewDir = WorldToViewDir(worldDir);
    // Projection cull disabled for diagnostic (see Phase 0B). Keep IsOutside only.
    float2 uv = ViewToUV(viewDir);
    bool inside = !IsOutside(uv);

    // ── Current screen sample (Phase 0B Sub-step B path, unchanged) ──
    float3 curCol = 0.0;
    float3 curPos = 0.0;
    bool curValid = false;
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
            curValid = true;
        }
    }

    // ── Character mask (GTVBAO-style: mrtTexture0.w & 1) — sample at the projected
    //    screen UV (same coordinate as color/depth), not the cubemap texel. ──
    bool isCharacter = false;
    if (g_charMaskAvailable > 0.5f && inside)
    {
        uint mrtW, mrtH;
        g_mrt0Tex.GetDimensions(mrtW, mrtH);
        int2 mrtPixel = int2(min(uv * float2(mrtW, mrtH) + 0.25f, float2(mrtW - 1, mrtH - 1)));
        uint4 mrt0 = g_mrt0Tex.Load(int3(mrtPixel, 0));
        isCharacter = (mrt0.w & 1) != 0;
    }

    // ── Character mask exclusion (Phase 2) ──
    if (isCharacter && g_characterCapture < 0.5f)
    {
        // Character pixel but character capture disabled -> treat as invalid to preserve history
        curValid = false;
    }

    // ── Previous history (same face/texel layout — direct Load) ──
    float4 prevColor = g_prevColorTex.Load(int4(dtid, 0));
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

    float3 outCol;
    float3 outPos;
    float  outValid;
    float  outContrib;
    if (curValid)
    {
        float3 curPosScaled = curPos * g_posScale;
        float posDelta = length(prevPosComp - curPosScaled);
        bool compatible = (!prevValid) || (posDelta < (g_historyPosThreshold * g_posScale));
        if (compatible)
        {
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
    else if (prevValid)
    {
        // No current sample — preserve previous history
        outCol = prevColor.rgb;
        outPos = prevPosComp;
        outValid = prevPos.a;
        outContrib = prevContrib * 0.5;
    }
    else
    {
        outCol = 0.0;
        outPos = 0.0;
        outValid = 0.0;
        outContrib = 0.0;
    }

    g_outColor[dtid]   = float4(max(0.0, outCol), 1.0);
    g_outPos[dtid]     = float4(outPos, outValid);
    g_outContrib[dtid] = outContrib;

    // Write character mask (1 = character, 0 = non-character)
    float charMask = isCharacter ? 1.0 : 0.0;
    g_outCharMask[dtid] = float4(charMask, charMask, charMask, 1.0);

    // Record current camera position for next frame (thread 0 of face 0)
    if (dtid.x == 0 && dtid.y == 0 && dtid.z == 0)
        g_camCurTex[uint2(0, 0)] = float4(camCur, 1.0);
}