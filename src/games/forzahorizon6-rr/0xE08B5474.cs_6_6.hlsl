// forzahorizon6-rr: bilateral resolve gather bypass — LOW RT-quality variant.
//
// Original (0xE08B5474): the low-tier member of the same reconstruction family
// as 0x0B33C6D8 — an 8x8 pass that stages the half-res resolve pair (t22/t23)
// and the half-res packed normal/depth (t38) into groupshared memory, then
// blends a nine-tap (3x3) normal/depth-weighted neighbourhood and reconstructs
// the output through the same Co/Cg + normal-lobe transform. This replacement
// takes a raw nearest point sample of the pair at the original's own center-tap
// coordinate (input = px >> 1, the half-res center tap) - no bilateral
// weighting, no tap mixing, so the resolve feed stays as raw as the dispatch
// allows. Bounds, depth guard and normal decode are exact copies of the
// original's per-pixel path (t0/t1).
//
// t38 is bound by the original's root signature but not needed here (its data
// is the half-res normal/depth staged for the gather); it is declared so the
// resource layout matches.

cbuffer cb : register(b0, space36)
{
    float4 m[50] : packoffset(c0);
};

Texture2D<uint4> g_normal : register(t0, space36);
Texture2D<float4> g_depth : register(t1, space36);
Texture2D<half4> g_rad_a : register(t22, space36);
Texture2D<half4> g_rad_b : register(t23, space36);
Texture2D<float2> g_nr_lowres : register(t38, space36);
RWTexture2D<float4> g_out : register(u0, space36);

[numthreads(8, 8, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    const uint2 px = uint2(global_id.x, global_id.y);
    const uint4 bounds = asuint(m[17]);
    if (px.x >= bounds.x || px.y >= bounds.y) return;

    const float depth = g_depth.Load(int3(px, 0)).x;
    const float linear_depth = (depth <= 0.0f) ? 0.0f : (m[0].w / (depth - m[0].z));
    if (!(linear_depth > 9.9999999747524270787835121154785e-07f))
    {
        g_out[px] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    // Exact copy of the original's packed-normal decode (single normalize).
    const uint packed = g_normal.Load(int3(px, 0)).x;
    const float nx0 = (float((packed >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) - 1.0f;
    const float ny0 = (float(packed >> 20u) * 0.0004884005174972116947174072265625f) - 1.0f;
    const float nz0 = 1.0f - abs(nx0) - abs(ny0);
    const float neg = clamp(-nz0, 0.0f, 1.0f);
    const float nxn = ((nx0 >= 0.0f) ? -neg : neg) + nx0;
    const float nyn = ((ny0 >= 0.0f) ? -neg : neg) + ny0;
    const float nlen = rsqrt(dot(float3(nxn, nyn, nz0), float3(nxn, nyn, nz0)));
    const float3 n = float3(nxn, nyn, nz0) * nlen;

    // Raw nearest sample of the half-res resolve pair: the original's center
    // tap is exactly the input texel at (px >> 1, py >> 1).
    uint in_w, in_h;
    g_rad_a.GetDimensions(in_w, in_h);
    const uint2 in_px = min(px >> 1u, uint2(in_w - 1u, in_h - 1u));
    const float4 rad_a = float4(g_rad_a.Load(int3(in_px, 0)));
    const float4 rad_b = float4(g_rad_b.Load(int3(in_px, 0)));

    // Original output transform with the sampled tap.
    const float base = rad_b.w + dot(rad_b.xyz, n);
    g_out[px] = float4(
        rad_a.x - rad_a.y + base,
        base + rad_a.y,
        -rad_a.x - rad_a.y + base,
        rad_a.w);
}
