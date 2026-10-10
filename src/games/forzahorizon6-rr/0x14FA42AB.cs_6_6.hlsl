// forzahorizon6-rr: bilateral resolve gather bypass — high RT-quality variant.
//
// Original (0x14FA42AB): the cheap apply stage of the same reconstruction
// family as 0x0B33C6D8 — reads the half-res resolve pair (t22/t23) at
// integer div (px / m[20]) i.e. nearest sampling, and reconstructs the
// full-res output through the same Co/Cg + normal lobe transform.
// Replaced with a bilinear reconstruction at the exact fractional position
// (pixel centers), so the raw resolve feed does not show 2x2 blocks.
// Depth guard and normal decode are exact copies of the original's path.

cbuffer cb : register(b0, space36)
{
    float4 m[50] : packoffset(c0);
};

Texture2D<uint4> g_normal : register(t0, space36);
Texture2D<float4> g_depth : register(t1, space36);
Texture2D<float4> g_rad_a : register(t22, space36);
Texture2D<float4> g_rad_b : register(t23, space36);
RWTexture2D<float4> g_out : register(u0, space36);

[numthreads(8, 8, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    const uint2 px = uint2(global_id.x, global_id.y);

    const float depth = g_depth.Load(int3(px, 0)).x;
    const float linear_depth = (depth <= 0.0f) ? 0.0f : (m[0].w / (depth - m[0].z));
    if (!(linear_depth > 9.9999999747524270787835121154785e-07f))
    {
        g_out[px] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    // Exact copy of the original's packed-normal decode (single normalize;
    // the original's second normalize on the unit vector was a no-op).
    const uint packed = g_normal.Load(int3(px, 0)).x;
    const float nx0 = (float((packed >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) - 1.0f;
    const float ny0 = (float(packed >> 20u) * 0.0004884005174972116947174072265625f) - 1.0f;
    const float nz0 = 1.0f - abs(nx0) - abs(ny0);
    const float neg = clamp(-nz0, 0.0f, 1.0f);
    const float nxn = ((nx0 >= 0.0f) ? -neg : neg) + nx0;
    const float nyn = ((ny0 >= 0.0f) ? -neg : neg) + ny0;
    const float nlen = rsqrt(dot(float3(nxn, nyn, nz0), float3(nxn, nyn, nz0)));
    const float3 n = float3(nxn, nyn, nz0) * nlen;

    // Bilinear read of the half-res resolve pair at pixel centers.
    // Original mapping: nearest at integer div px / m[20].xy.
    uint in_w, in_h;
    g_rad_a.GetDimensions(in_w, in_h);
    const float2 scale = max(float2(asuint(m[20]).xy), 1.0f.xx);
    const float2 in_max = float2(in_w, in_h) - 1.0f.xx;
    const float2 h = clamp((float2(px) + 0.5f) / scale - 0.5f, 0.0f.xx, in_max);
    const uint2 i0 = uint2(floor(h));
    const uint2 i1 = min(i0 + 1u, uint2(in_w - 1u, in_h - 1u));
    const float2 f = h - float2(i0);

    const float4 a00 = g_rad_a.Load(int3(i0, 0));
    const float4 a10 = g_rad_a.Load(int3(uint2(i1.x, i0.y), 0));
    const float4 a01 = g_rad_a.Load(int3(uint2(i0.x, i1.y), 0));
    const float4 a11 = g_rad_a.Load(int3(i1, 0));
    const float4 b00 = g_rad_b.Load(int3(i0, 0));
    const float4 b10 = g_rad_b.Load(int3(uint2(i1.x, i0.y), 0));
    const float4 b01 = g_rad_b.Load(int3(uint2(i0.x, i1.y), 0));
    const float4 b11 = g_rad_b.Load(int3(i1, 0));
    const float4 rad_a = lerp(lerp(a00, a10, f.x), lerp(a01, a11, f.x), f.y);
    const float4 rad_b = lerp(lerp(b00, b10, f.x), lerp(b01, b11, f.x), f.y);

    // Original output transform with the reconstructed tap.
    const float base = rad_b.w + dot(rad_b.xyz, n);
    g_out[px] = float4(
        rad_a.x - rad_a.y + base,
        base + rad_a.y,
        -rad_a.x - rad_a.y + base,
        rad_a.w);
}
