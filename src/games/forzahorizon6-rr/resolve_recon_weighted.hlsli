// forzahorizon6-rr: normal/depth-weighted reconstruct (raw, current-frame).
//
// The medium (0x0B33C6D8) and low (0xE08B5474) resolve reconstructs upsample
// the sub-res radiance pair (t22/t23) to full resolution with a 3x3
// normal/depth-weighted filter: per tap they read a packed normal + linear
// depth from t38 and weight by the normal alignment and the depth similarity.
// This body reproduces that upsample exactly (kernel scales and both weight
// terms), with no cross-frame state — the output is the current frame's
// weighted estimate only.
//
// The pair-to-output ratio is read from the game's own input scale (m[20]), so
// no per-tier constant is needed.
#ifndef FH6_RECON_TX
#define FH6_RECON_TX 16
#endif
#ifndef FH6_RECON_TY
#define FH6_RECON_TY 8
#endif

cbuffer cb : register(b0, space36)
{
    float4 m[50] : packoffset(c0);
};

Texture2D<uint4> g_normal : register(t0, space36);
Texture2D<float4> g_depth : register(t1, space36);
Texture2D<half4> g_rad_a : register(t22, space36);
Texture2D<half4> g_rad_b : register(t23, space36);
Texture2D<float2> g_nr : register(t38, space36);
RWTexture2D<float4> g_out : register(u0, space36);

// Original packed-normal decode (12/12-bit fields, octahedral fold). Takes the
// raw packed bits: pass the uint texel directly for t0 (uint4) and
// asuint(t38.x) for the float2 t38, matching the original's asint(t38.x).
float3 DecodePackedNormal(uint bits)
{
    const float nx = float((bits >> 8u) & 4095u) * 0.0004884005174972116947174072265625f - 1.0f;
    const float ny = float(bits >> 20u) * 0.0004884005174972116947174072265625f - 1.0f;
    const float nz = 1.0f - abs(nx) - abs(ny);
    const float negative = clamp(-nz, 0.0f, 1.0f);
    const float3 n = float3(
        ((nx >= 0.0f) ? -negative : negative) + nx,
        ((ny >= 0.0f) ? -negative : negative) + ny,
        nz);
    return n * rsqrt(dot(n, n));
}

[numthreads(FH6_RECON_TX, FH6_RECON_TY, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    const uint2 px = global_id.xy;
    const uint4 bounds = asuint(m[17]);
    if (px.x >= bounds.x || px.y >= bounds.y) return;

    const float depth = g_depth.Load(int3(px, 0)).x;
    const float linear_depth = (depth <= 0.0f) ? 0.0f : (m[0].w / (depth - m[0].z));
    if (!(linear_depth > 9.9999999747524270787835121154785e-07f))
    {
        g_out[px] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    const float3 n = DecodePackedNormal(g_normal.Load(int3(px, 0)).x);

    uint in_w, in_h;
    g_rad_a.GetDimensions(in_w, in_h);
    const int2 in_max = int2(in_w - 1u, in_h - 1u);
    // Map the output pixel to the pair by the pair's own dimensions against the
    // output bounds, so the ratio is whatever the game actually uses (half,
    // quarter or full res) without relying on a constant or m[20].
    const uint2 out_dims = max(bounds.xy, uint2(1u, 1u));
    const int2 in_px =
        int2(min(px * uint2(in_w, in_h) / out_dims, uint2(in_w - 1u, in_h - 1u)));

    // 3x3 normal/depth-weighted gather of the pair (original kernel: center
    // 5.0, edges 2.5, corners 5/3).
    float3 rad_a = 0.0f.xxx;
    float3 rad_b = 0.0f.xxx;
    float rad_b_w = 0.0f;
    float rad_a_w = 0.0f;
    float weight_sum = 0.0f;
    [unroll] for (int dy = -1; dy <= 1; ++dy)
    {
        [unroll] for (int dx = -1; dx <= 1; ++dx)
        {
            const int2 q = clamp(in_px + int2(dx, dy), int2(0, 0), in_max);
            const float2 nr = g_nr.Load(int3(q, 0));
            const float3 tile_n = DecodePackedNormal(asuint(nr.x));
            const float tile_d = nr.y;
            const float axis =
                (dx == 0 && dy == 0) ? 5.0f : ((dx == 0 || dy == 0) ? 2.5f : 1.6666666269302368f);
            const float depth_w =
                1.0f - saturate((abs(linear_depth - tile_d) / (tile_d + linear_depth)) * 20.0f);
            const float w = saturate(dot(n, tile_n)) * axis * depth_w;
            const float4 a = float4(g_rad_a.Load(int3(q, 0)));
            const float4 b = float4(g_rad_b.Load(int3(q, 0)));
            rad_a += w * a.xyz;
            rad_b += w * b.xyz;
            rad_b_w += w * b.w;
            rad_a_w += w * a.w;
            weight_sum += w;
        }
    }

    const float divisor = (weight_sum != 0.0f) ? (1.0f / weight_sum) : 1.0f;
    rad_a *= divisor;
    rad_b *= divisor;
    rad_b_w *= divisor;
    rad_a_w *= divisor;

    const float base = rad_b_w + dot(rad_b, n);
    g_out[px] = float4(
        rad_a.x - rad_a.y + base,
        base + rad_a.y,
        -rad_a.x - rad_a.y + base,
        rad_a_w);
}
