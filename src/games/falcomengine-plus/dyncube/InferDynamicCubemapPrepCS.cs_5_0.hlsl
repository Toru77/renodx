// InferDynamicCubemapPrepCS.cs_5_0.hlsl — Falcom Engine+ generic inference prep (Phase 2)
// Copies the current history set into mip0 of the inference mip chain so hardware
// GenerateMips produces blurred RGB + averaged validity coverage (.a).
// Input : t0 histColor (current set), t1 histPos (current set, .a = validity)
// Output: u0 infer_mips mip0 = float4(color.rgb, pos.a)
// Dispatch: (size/8, size/8, 6) numthreads(8,8,1)

Texture2DArray<float4> g_histColorTex : register(t0);
Texture2DArray<float4> g_histPosTex   : register(t1);

RWTexture2DArray<float4> g_outMip0 : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint w, h, el;
    g_outMip0.GetDimensions(w, h, el);
    if (dtid.x >= w || dtid.y >= h || dtid.z >= 6) return;

    float4 color = g_histColorTex.Load(int4(dtid, 0));
    float4 pos   = g_histPosTex.Load(int4(dtid, 0));
    g_outMip0[dtid] = float4(color.rgb, pos.a);
}