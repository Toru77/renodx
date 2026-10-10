// forzahorizon6-rr: spatial resolve filter bypass (shared body).
//
// Serves both RT-quality variants of the same stage, which are dispatched 1x
// per frame:
//   - 0x596D3E8F (medium RT tier)
//   - 0x4DAF8A48 (high RT tier) — verified to have the identical bindings,
//     material-decode chain and per-pixel path as 0x596D3E8F.
//
// Original: large shared-memory spatial filter over the resolve pair (t19 and
// friends) with per-material paths, history weights, counters and NaN guards.
// This replacement skips all neighborhood accumulation and reproduces the
// original's own per-pixel "material path": every in-bounds depth-valid pixel
// writes its own single radiance sample through the original YCoCg + normal
// lobe transform. The material-decode chain (instance/material flags) and the
// original's NaN guards and zero-fallthrough are replicated exactly.

cbuffer cb : register(b0, space36)
{
    float4 m[50] : packoffset(c0);
};

Texture2D<uint4> g_gbuffer : register(t6, space36);
Texture2D<float4> g_radiance : register(t19, space36);
Texture2D<float4> g_normal : register(t26, space36);
Texture2D<uint4> g_material : register(t47, space36);
Buffer<uint4> g_instances : register(t96, space0);
RWTexture2D<float4> g_out_a : register(u2, space36);
RWTexture2D<float4> g_out_b : register(u3, space36);
RWTexture2D<int4> g_out_meta : register(u4, space36);

[numthreads(8, 8, 1)]
void main(uint3 global_id : SV_DispatchThreadID)
{
    const uint2 px = uint2(global_id.x, global_id.y);
    const uint4 bounds = asuint(m[14]);

    // Original: depth comes from the cache (t26.y); invalid depth or
    // out-of-bounds pixels fall through to the zero path.
    const float depth = g_normal.Load(int3(px, 0)).y;
    if (!(depth > 0.0f) || px.x >= bounds.x || px.y >= bounds.y)
    {
        g_out_meta[px] = int4(0, 0, 0, 0);
        g_out_a[px] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        g_out_b[px] = 0.0f.xxxx;
        return;
    }

    // Exact copy of the original material-decode chain.
    const uint4 gbuffer = g_gbuffer.Load(int3(px, 0));
    const uint instance_bits = gbuffer.w;
    const uint material_flags = g_material.Load(int3(px, 0)).y;
    const uint material_class = ((material_flags & 64u) != 0u)
        ? uint((material_flags & 4294967167u) != 66u)
        : 4294967295u;
    const uint is_override = instance_bits & 128u;
    const uint instance_index = (is_override != 0u) ? 1u : ((gbuffer.x << 7u) | instance_bits);
    const uint material_data = g_instances.Load(instance_index * 4u).x;
    const uint base_class = ((material_data & 1u) != 0u) ? 0u : 18u;
    uint resolved_class;
    if (is_override == 0u)
    {
        // Original condition: clamp(material_class, 0, 1) == material_class.
        resolved_class = (((material_data & 2097152u) != 0u) && (material_class <= 1u))
            ? (base_class | 128u)
            : base_class;
    }
    else
    {
        resolved_class = instance_bits;
    }
    const bool use_threshold_x = ((resolved_class & 128u)
        | (g_instances.Load((instance_index * 4u) + 1u).x & 512u)) == 0u;

    // Exact copy of the original's packed-normal decode (single normalize).
    const uint packed = asuint(g_normal.Load(int3(px, 0)).x);
    const float nx0 = (float((packed >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) - 1.0f;
    const float ny0 = (float(packed >> 20u) * 0.0004884005174972116947174072265625f) - 1.0f;
    const float nz0 = 1.0f - abs(nx0) - abs(ny0);
    const float neg = clamp(-nz0, 0.0f, 1.0f);
    const float nxn = ((nx0 >= 0.0f) ? -neg : neg) + nx0;
    const float nyn = ((ny0 >= 0.0f) ? -neg : neg) + ny0;
    const float nlen = rsqrt(dot(float3(nxn, nyn, nz0), float3(nxn, nyn, nz0)));
    const float3 n = float3(nxn, nyn, nz0) * nlen;

    // Original per-pixel path.
    const float4 radiance = g_radiance.Load(int3(px, 0));
    const float sample_weight = radiance.w;
    const float threshold = use_threshold_x ? m[28].x : m[28].y;
    float weight;
    if ((asuint(m[22]).y & 1u) != 0u)
    {
        weight = (sample_weight > 0.0f)
            ? clamp(sample_weight / threshold, 0.0f, 1.0f)
            : 1.0f;
    }
    else
    {
        weight = ((sample_weight > 0.0f) && (sample_weight < threshold)) ? 0.0f : 1.0f;
    }

    const float sum = radiance.x + radiance.y + radiance.z;
    float3 rgb = 0.0f.xxx;
    if (sum > 0.0f)
    {
        rgb = radiance.xyz * ((clamp(sum * 0.03125f, 0.0f, 1.0f) * 32.0f) / sum);
    }
    const float co = dot(float3(0.5f, 0.0f, -0.5f), rgb);
    const float cg = dot(float3(-0.25f, 0.5f, -0.25f), rgb);
    const float yv = dot(float3(0.25f, 0.5f, 0.25f), rgb) * 0.5f;
    const float3 yn = yv * n;

    g_out_meta[px] = int4(1, 1, 1, 1);
    if (isfinite(co) && isfinite(cg) && isfinite(weight))
    {
        g_out_a[px] = float4(co, cg, 0.0f, weight);
    }
    else
    {
        g_out_a[px] = 0.0f.xxxx;
    }
    if (isfinite(yn.x) && isfinite(yn.y) && isfinite(yn.z) && isfinite(yv))
    {
        g_out_b[px] = float4(yn, yv);
    }
    else
    {
        g_out_b[px] = 0.0f.xxxx;
    }
}
