#pragma once

// Instance matrix maths shared by world_dynamic_expand (P2b) and the G3 instance set.
// CheckPoolWorld and ComputeMatrixInverse (bvh_pool.hpp, pool_transform.hpp) are the CPU references.

// CheckPoolWorld (bvh_pool.hpp): finite; each row length >= 1e-6 (not zero scale) and within [min, max); each
// translation within max_position; no two rows nearly parallel (|cos| < 0.995).
bool WorldRowsPlausible(float4 r0, float4 r1, float4 r2, float min_scale, float max_scale, float max_position)
{
    if (!all(isfinite(r0)) || !all(isfinite(r1)) || !all(isfinite(r2))) return false;
    const float3 rows[3] = {r0.xyz, r1.xyz, r2.xyz};
    const float3 translation = float3(r0.w, r1.w, r2.w);
    if (!all(abs(translation) <= max_position)) return false;
    float lengths[3];
    for (uint r = 0; r < 3; ++r)
    {
        lengths[r] = length(rows[r]);
        if (!(lengths[r] >= 1e-6) || !(lengths[r] >= min_scale) || !(lengths[r] < max_scale)) return false;
    }
    for (uint a = 0; a < 3; ++a)
    {
        for (uint b = a + 1; b < 3; ++b)
        {
            if (!(abs(dot(rows[a], rows[b]) / (lengths[a] * lengths[b])) < 0.995)) return false;
        }
    }
    return true;
}

// Columns of the adjugate: rows of the inverse are (c0.k, c1.k, c2.k) / det.
float WorldAffineInverse(float4 r0, float4 r1, float4 r2, out float4 inverse_rows[3])
{
    const float3 a0 = r0.xyz;
    const float3 a1 = r1.xyz;
    const float3 a2 = r2.xyz;
    const float3 c0 = cross(a1, a2);
    const float3 c1 = cross(a2, a0);
    const float3 c2 = cross(a0, a1);
    const float det = dot(a0, c0);
    const float3 translation = float3(r0.w, r1.w, r2.w);
    for (uint r = 0; r < 3; ++r)
    {
        const float3 inverse_row = float3(c0[r], c1[r], c2[r]) / det;
        inverse_rows[r] = float4(inverse_row, -dot(inverse_row, translation));
    }
    return det;
}
