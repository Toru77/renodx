// forzahorizon6-rr: resolve reconstruct bypass (0x0B33C6D8) - medium RT tier.
//
// Original: 16x8 pass over a 6x10 shared-memory tile that upsamples the
// half-res radiance pair (t22/t23) to the full-res resolve feed by blending
// nine taps with per-tap normal alignment and depth similarity (half-res
// guide t38), then reconstructing through the Co/Cg + normal-lobe transform.
// This replacement keeps the upscale - the pair is read with a plain bilinear
// 2x reconstruction at the original's own pixel centres, so the full-res
// output is still filled - and drops the bilateral denoise: no per-tap
// weighting, no tap rejection. Bounds, depth guard and normal decode are
// exact copies of the original's per-pixel path.

cbuffer cb : register(b0, space36)
{
    float4 m[50] : packoffset(c0);
};

Texture2D<uint4> g_normal : register(t0, space36);
Texture2D<float4> g_depth : register(t1, space36);
Texture2D<float4> g_rad_a : register(t22, space36);
Texture2D<float4> g_rad_b : register(t23, space36);
RWTexture2D<float4> g_out : register(u0, space36);

[numthreads(16, 8, 1)]
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

    // Exact copy of the original's packed-normal decode.
    const uint packed = g_normal.Load(int3(px, 0)).x;
    const float nx0 = (float((packed >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) - 1.0f;
    const float ny0 = (float(packed >> 20u) * 0.0004884005174972116947174072265625f) - 1.0f;
    const float nz0 = 1.0f - abs(nx0) - abs(ny0);
    const float neg = clamp(-nz0, 0.0f, 1.0f);
    const float nxn = ((nx0 >= 0.0f) ? -neg : neg) + nx0;
    const float nyn = ((ny0 >= 0.0f) ? -neg : neg) + ny0;
    const float nlen = rsqrt(dot(float3(nxn, nyn, nz0), float3(nxn, nyn, nz0)));
    const float3 n = float3(nxn, nyn, nz0) * nlen;

    // Bilinear 2x read of the half-res resolve pair at the original's own
    // pixel centres: output px reads the pair at (px + 0.5) * 0.5 - 0.5.
    uint in_w, in_h;
    g_rad_a.GetDimensions(in_w, in_h);
    const uint2 in_max = uint2(in_w - 1u, in_h - 1u);
    const float2 h = clamp((float2(px) + 0.5f) * 0.5f - 0.5f, 0.0f.xx, float2(in_max));
    const uint2 i0 = uint2(floor(h));
    const uint2 i1 = min(i0 + 1u, in_max);
    const float2 f = h - float2(i0);
    const float4 a = lerp(
        lerp(g_rad_a.Load(int3(i0, 0)), g_rad_a.Load(int3(uint2(i1.x, i0.y), 0)), f.x),
        lerp(g_rad_a.Load(int3(uint2(i0.x, i1.y), 0)), g_rad_a.Load(int3(i1, 0)), f.x),
        f.y);
    const float4 b = lerp(
        lerp(g_rad_b.Load(int3(i0, 0)), g_rad_b.Load(int3(uint2(i1.x, i0.y), 0)), f.x),
        lerp(g_rad_b.Load(int3(uint2(i0.x, i1.y), 0)), g_rad_b.Load(int3(i1, 0)), f.x),
        f.y);

    // Original output transform with the sampled tap.
    const float base = b.w + dot(b.xyz, n);
    g_out[px] = float4(
        a.x - a.y + base,
        base + a.y,
        -a.x - a.y + base,
        a.w);
}
