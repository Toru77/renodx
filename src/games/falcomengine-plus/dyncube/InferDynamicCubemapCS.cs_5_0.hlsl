// InferDynamicCubemapCS.cs_5_0.hlsl — Falcom Engine+ generic inference (Phase 2)
// Adapted from Skyrim InferCubemapCS, not verbatim. Walks coarser mips of the inference
// mip chain (alpha = validity coverage) to fill directions with insufficient history.
// History-authoritative: coverage >= threshold -> copy unchanged (source=1).
// Coarser mip with coverage >= minMipCoverage -> fill missing fraction (source=0.5).
// Nothing found -> black / invalid (source=0). No static fallback yet.
// Input : t0 infer_mips cube SRV (8 mips), s0 linear clamp, b13 {coverageThreshold, minMipCoverage}
// Output: u0 inferred (rgb, a=source), u1 srcview (source as grayscale)

cbuffer DynCubeInferCB : register(b13)
{
    float g_coverageThreshold;
    float g_minMipCoverage;
    float2 _pad0;
};

TextureCube<float4> g_historyMips : register(t0);
SamplerState        g_linearClamp : register(s0);

RWTexture2DArray<float4> g_outInferred : register(u0);
RWTexture2DArray<float4> g_outSrcView  : register(u1);

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
    g_outInferred.GetDimensions(w, h, el);
    if (dtid.x >= w || dtid.y >= h || dtid.z >= 6) return;

    float3 dir = GetSamplingVector(dtid, w, h);

    float4 color = g_historyMips.SampleLevel(g_linearClamp, dir, 0.0);
    float source = 1.0; // history / captured

    if (color.w < g_coverageThreshold)
    {
        source = 0.0;
        bool found = false;
        for (int mip = 1; mip < 8; ++mip)
        {
            float4 temp = g_historyMips.SampleLevel(g_linearClamp, dir, (float)mip);
            if (temp.w >= g_minMipCoverage)
            {
                // Fill the uncovered fraction from the coarse mip, normalized by its coverage.
                float missing = saturate(1.0 - color.w);
                float3 coarse = temp.rgb / max(temp.w, 1e-4);
                color.rgb = lerp(color.rgb, coarse, saturate(missing / max(temp.w, 1e-4)));
                color.w = 1.0;
                source = 0.5; // inferred
                found = true;
                break;
            }
        }
        if (!found)
        {
            color.rgb = 0.0;
            color.w = 0.0;
            source = 0.0; // invalid
        }
    }

    g_outInferred[dtid] = float4(max(0.0, color.rgb), source);
    g_outSrcView[dtid]  = float4(source, source, source, 1.0);
}