// forzahorizon6-rr: resolve reconstruct bypass (0x0B33C6D8) - medium RT tier.
//
// Original: 16x8 pass over a 6x10 shared-memory tile that upsamples the
// half-res radiance pair (t22/t23) to the full-res resolve feed by blending
// nine taps with per-tap normal alignment and depth similarity (half-res
// guide t38), then reconstructing through the Co/Cg + normal-lobe transform.
// This replacement keeps the raw bilinear 2x upscale (within a surface the
// pair is passed through, no averaging) but adds two anomaly controls:
//   - a per-tap guide gate matching the original's own weight zero-crossings:
//     a tap contributes only while its half-res guide normal agrees with this
//     pixel and its linear depth is within 5% of it, so the blend never mixes
//     across a surface edge;
//   - a bright-spike clamp: a tap whose luminance estimate dominates its
//     accepted neighbours (kSpikeLimit x) is scaled down before the blend, so
//     a single stray sample is not upscaled at full strength.
// If every tap fails the gate, the raw centre sample is used. Bounds, depth
// guard and normal decode are exact copies of the original's per-pixel path.

cbuffer cb : register(b0, space36)
{
    float4 m[50] : packoffset(c0);
};

Texture2D<uint4> g_normal : register(t0, space36);
Texture2D<float4> g_depth : register(t1, space36);
Texture2D<float4> g_rad_a : register(t22, space36);
Texture2D<float4> g_rad_b : register(t23, space36);
Texture2D<float2> g_guide : register(t38, space36);
RWTexture2D<float4> g_out : register(u0, space36);

// Relative linear-depth difference at which the original's depth weight
// reaches zero (its falloff is 1 - saturate(20 * ratio)).
static const float kGuideDepthRatio = 0.05f;
// A tap above this multiple of its accepted neighbours' luminance is clamped.
static const float kSpikeLimit = 16.0f;

float3 DecodePackedNormal(uint bits)
{
    const float nx = float((bits >> 8u) & 4095u) * 0.0004884005174972116947174072265625f - 1.0f;
    const float ny = float(bits >> 20u) * 0.0004884005174972116947174072265625f - 1.0f;
    const float nz = 1.0f - abs(nx) - abs(ny);
    const float negative = clamp(-nz, 0.0f, 1.0f);
    const float3 normal = float3(
        ((nx >= 0.0f) ? -negative : negative) + nx,
        ((ny >= 0.0f) ? -negative : negative) + ny,
        nz);
    return normal * rsqrt(dot(normal, normal));
}

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

    const float3 n = DecodePackedNormal(g_normal.Load(int3(px, 0)).x);

    // Bilinear 2x read of the half-res resolve pair at the original's own
    // pixel centres: output px reads the pair at (px + 0.5) * 0.5 - 0.5.
    uint in_w, in_h;
    g_rad_a.GetDimensions(in_w, in_h);
    const uint2 in_max = uint2(in_w - 1u, in_h - 1u);
    const float2 h = clamp((float2(px) + 0.5f) * 0.5f - 0.5f, 0.0f.xx, float2(in_max));
    const uint2 i0 = uint2(floor(h));
    const uint2 i1 = min(i0 + 1u, in_max);
    const float2 f = h - float2(i0);

    uint2 tap_px[4] = {i0, uint2(i1.x, i0.y), uint2(i0.x, i1.y), i1};
    float bilinear_w[4] = {
        (1.0f - f.x) * (1.0f - f.y),
        f.x * (1.0f - f.y),
        (1.0f - f.x) * f.y,
        f.x * f.y,
    };

    float4 a_tap[4];
    float4 b_tap[4];
    float l_tap[4];
    float tap_weight[4];
    float weight_sum = 0.0f;
    [unroll]
    for (int t = 0; t < 4; ++t)
    {
        a_tap[t] = g_rad_a.Load(int3(tap_px[t], 0));
        b_tap[t] = g_rad_b.Load(int3(tap_px[t], 0));
        l_tap[t] = b_tap[t].w + dot(b_tap[t].xyz, n);

        const float2 guide = g_guide.Load(int3(tap_px[t], 0));
        const float3 tile_n = DecodePackedNormal(asuint(guide.x));
        const float tile_d = guide.y;
        const float normal_gate = step(0.0f, dot(n, tile_n));
        const float depth_gate = step(
            abs(linear_depth - tile_d) / (tile_d + linear_depth),
            kGuideDepthRatio);
        tap_weight[t] = (normal_gate * depth_gate) * bilinear_w[t];
        weight_sum += tap_weight[t];
    }

    if (weight_sum <= 0.0f)
    {
        // Every tap failed the guide gate: fall back to the raw centre sample.
        const uint2 centre = min(px >> 1u, in_max);
        const float4 a = g_rad_a.Load(int3(centre, 0));
        const float4 b = g_rad_b.Load(int3(centre, 0));
        const float base = b.w + dot(b.xyz, n);
        g_out[px] = float4(a.x - a.y + base, base + a.y, -a.x - a.y + base, a.w);
        return;
    }

    float4 a_sum = 0.0f.xxxx;
    float4 b_sum = 0.0f.xxxx;
    float weight_used = 0.0f;
    [unroll]
    for (int t = 0; t < 4; ++t)
    {
        float w = tap_weight[t];
        if (w > 0.0f)
        {
            // Clamp an isolated bright spike against the other accepted taps.
            float others_max = 0.0f;
            [unroll]
            for (int o = 0; o < 4; ++o)
            {
                if ((o != t) && (tap_weight[o] > 0.0f))
                {
                    others_max = max(others_max, l_tap[o]);
                }
            }
            if ((others_max > 0.0f) && (l_tap[t] > kSpikeLimit * others_max))
            {
                w *= (kSpikeLimit * others_max) / l_tap[t];
            }
            a_sum += w * a_tap[t];
            b_sum += w * b_tap[t];
            weight_used += w;
        }
    }

    const float inv_weight = 1.0f / weight_used;
    const float4 a = a_sum * inv_weight;
    const float4 b = b_sum * inv_weight;

    // Original output transform with the reconstructed tap.
    const float base = b.w + dot(b.xyz, n);
    g_out[px] = float4(
        a.x - a.y + base,
        base + a.y,
        -a.x - a.y + base,
        a.w);
}
