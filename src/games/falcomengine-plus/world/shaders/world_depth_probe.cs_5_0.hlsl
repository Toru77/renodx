// world_depth_probe.cs_5_0.hlsl — Phase 0 objective depth validation.
//
// Inputs : t0 game scene depth, t1 probe samples (uv.xy, expected_device_z, candidate)
// Outputs: u0 per-candidate counts (match, occluded, mismatch, out_of_screen)
//          u1 per-candidate sums (sum_abs_error, matched, expected_sum, samples)
//          u2 per-sample results (sampled_z, abs_error, classification, 0)
//          b13 { tol_rel, tol_abs, samples_per_candidate, candidate_count,
//                neighborhood, spare0, spare1, spare2 }
//
// Comparison is in device-depth space, so it does not depend on the constant
// buffer's matrix packing convention. The engine renders reversed-Z (nearer =
// larger device depth), so a sample that reads nearer than expected is counted
// as occluded (neutral), never as a failure; only a candidate surface that
// floats in front of the sampled depth is a mismatch.

Texture2D<float4> g_depth : register(t0);
StructuredBuffer<float4> g_samples : register(t1);
RWStructuredBuffer<float4> g_results : register(u0);
RWStructuredBuffer<float4> g_errors : register(u1);
RWStructuredBuffer<float4> g_sample_results : register(u2);

cbuffer cb_probe : register(b13)
{
    float g_tol_rel;
    float g_tol_abs;
    uint  g_samples_per_candidate;
    uint  g_candidate_count;
    uint  g_neighborhood;
    float g_spare0;
    float g_spare1;
    float g_spare2;
};

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint candidate = dtid.x;
    if (candidate >= g_candidate_count) return;

    uint w = 0;
    uint h = 0;
    g_depth.GetDimensions(w, h);
    const float2 size = float2(max(w, 1u), max(h, 1u));

    const uint base = candidate * g_samples_per_candidate;
    float match = 0.0;
    float occluded = 0.0;
    float mismatch = 0.0;
    float out_count = 0.0;
    float sum_err = 0.0;
    float matched = 0.0;
    float expected_sum = 0.0;
    float samples = 0.0;

    for (uint i = 0; i < g_samples_per_candidate; ++i)
    {
        const float4 s = g_samples[base + i];
        const float2 uv = s.xy;
        const float expected = s.z;
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        {
            out_count += 1.0;
            continue;
        }

        const int2 center = int2(uv * size);
        float nearest = -1.0;
        const int radius = (int)g_neighborhood;
        for (int oy = -radius; oy <= radius; ++oy)
        {
            for (int ox = -radius; ox <= radius; ++ox)
            {
                const int2 pix = center + int2(ox, oy);
                if (pix.x < 0 || pix.y < 0 || pix.x >= (int)w || pix.y >= (int)h) continue;
                const float sampled = g_depth.Load(int3(pix, 0)).x;
                // Reversed-Z: the nearest surface is the largest depth value.
                nearest = max(nearest, sampled);
            }
        }
        if (nearest < 0.0) nearest = 0.0;

        samples += 1.0;
        expected_sum += expected;
        const float tol = max(abs(expected) * g_tol_rel, g_tol_abs);
        const float err = abs(nearest - expected);
        float classification = 3.0;  // mismatch
        if (err <= tol)
        {
            classification = 1.0;  // match
            match += 1.0;
            sum_err += err;
            matched += 1.0;
        }
        else if (nearest > expected)
        {
            classification = 2.0;  // occluded (sampled surface is nearer)
            occluded += 1.0;
        }
        else
        {
            mismatch += 1.0;
        }
        g_sample_results[base + i] = float4(nearest, err, classification, 0.0);
    }

    g_results[candidate] = float4(match, occluded, mismatch, out_count);
    g_errors[candidate] = float4(sum_err, matched, expected_sum, samples);
}
