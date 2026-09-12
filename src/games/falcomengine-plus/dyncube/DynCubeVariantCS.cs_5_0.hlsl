// DynCubeVariantCS.cs_5_0.hlsl — global-push variant of the filtered dynamic cube.
//
// Resamples the served cube (ggx_out[active]) at a fractional mip (soften) and
// scales it (strength) into a dedicated cube served ONLY on non-lighting t17
// pushes (glass and other global consumers). Lighting always samples the sharp
// cube plus its own sample-time controls, so this pass never affects it.
//
// Input : t0 source cube (ggx_out[active]), s0 trilinear clamp,
//         b13 { srcMip, strength }
// Output: u0 variant mip0 (2D-array UAV); remaining mips via GenerateMips.
cbuffer DynCubeVariantCB : register(b13)
{
    float g_srcMip;    // fractional source LOD = soften * maxBlur (0 = sharp copy)
    float g_strength;  // global reflection strength multiplier (1 = neutral)
};

TextureCube<float4> g_srcTex : register(t0);
SamplerState g_linearClamp : register(s0);

RWTexture2DArray<float4> g_outTex : register(u0);

// Address-space face map — identical layout to the capture GetSamplingVector
// MINUS the world-space (1,-1,-1) flip: resampling must preserve texel addresses
// (content already carries the baked convention). See DynamicCubemapCaptureCS.
float3 DynCubeVariantDir(uint3 tid, uint w, uint h)
{
    float2 st = float2(tid.xy) / float2(w, h);
    float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - 1.0;
    float3 v;
    switch (tid.z)
    {
    case 0: v = float3(1.0, uv.y, -uv.x); break;
    case 1: v = float3(-1.0, uv.y, uv.x); break;
    case 2: v = float3(uv.x, 1.0, -uv.y); break;
    case 3: v = float3(uv.x, -1.0, uv.y); break;
    case 4: v = float3(uv.x, uv.y, 1.0); break;
    default: v = float3(-uv.x, uv.y, -1.0); break;
    }
    return normalize(v);
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint w, h, el;
    g_outTex.GetDimensions(w, h, el);
    if (dtid.x >= w || dtid.y >= h || dtid.z >= 6) return;
    float3 D = DynCubeVariantDir(dtid, w, h);
    float4 tap = g_srcTex.SampleLevel(g_linearClamp, D, g_srcMip);
    // Validity rides along unscaled: dimming must never fake or erase validity.
    g_outTex[dtid] = float4(max(0.0, tap.rgb * g_strength), tap.a);
}
