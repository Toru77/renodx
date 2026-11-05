// world_restir_spatial.cs_5_0.hlsl - ReSTIR GI M3: biased spatial reuse (Ouyang et al. 2021, reference/restir/ReSTIR GI.md,
// Alg. 4 without the visibility lines 12-13 and 17-19; Sec. 4.2 similarity test; Eq. 11 Jacobian as world_restir_temporal;
// Sec. 5 adaptive radius, maxIterations and spatial M cap 500; Fig. 10 spatial-from-spatial at low sample count).
// Two dispatches per frame (c1.z). Mode 0: this pixel's temporal reservoir (this dispatch's M2 output, t3..t5) merged
// with 3 neighbours' temporal reservoirs, or 9 when its temporal M is below the low count (c1.w); writes the spatial
// reservoir (u2..u4). Mode 1: for pixels whose temporal M (t6) is below the low count, the mode-0 reservoir merged with
// 3 neighbours' mode-0 reservoirs (t3..t5); other pixels pass through. Both write the raw GI (u0, the trace contract;
// the input raw unchanged when no neighbour is merged). Spatial reservoirs live for this dispatch only and never feed
// the temporal history (Sec. 4.3, Fig. 10). Counters 28..31 (u1); debug view 14 (u5).

#include "../../include/mrt_normal.hlsli"
#include "world_rt_denoise.hlsli"

Texture2D<float4> g_raw_gi : register(t0);
Texture2D<float4> g_game_depth : register(t1);
Texture2D<uint4> g_mrt_normal : register(t2);
Texture2D<float4> g_res_sample : register(t3);    // x_s, W
Texture2D<float4> g_res_visible : register(t4);   // x_v, 0
Texture2D<uint4> g_res_data : register(t5);       // n_s, L_o rg, L_o b | M << 16, n_v
Texture2D<uint4> g_temporal_data : register(t6);  // this dispatch's temporal reservoir data (its M selects the low-count path)

RWTexture2D<float4> g_raw_out : register(u0);
RWStructuredBuffer<uint> g_rtgi_stats : register(u1);
RWTexture2D<float4> g_sample_write : register(u2);
RWTexture2D<float4> g_visible_write : register(u3);
RWTexture2D<uint4> g_data_write : register(u4);
RWTexture2D<float4> g_rtgi_debug : register(u5);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
};

// Push constants (RestirSpatialPushConstants in rtgi_resources.hpp, 16 floats).
cbuffer cb_restir_spatial : register(b12)
{
    float4 r_size : packoffset(c0);     // trace width, trace height, full width, full height
    float4 r_params : packoffset(c1);   // trace divisor d, debug mode, mode (0 or 1), low M
    float4 r_spatial : packoffset(c2);  // initial radius (trace px), minimum radius, normal cosine, relative depth tolerance
    float4 r_extra : packoffset(c3);    // noise seed, spatial M cap, fade start_eff, fade end_eff
};

#define RTGI_STAT_RESTIR_SPATIAL_PIXELS 28u
#define RTGI_STAT_RESTIR_SPATIAL_TRIED 29u
#define RTGI_STAT_RESTIR_SPATIAL_ACCEPTED 30u
#define RTGI_STAT_RESTIR_SPATIAL_LOW_M 31u
#define RTGI_SSTAT_COUNT 4u
static const float kRestirJacobianMax = 10.0;  // non-paper, as world_restir_temporal
static const float kRestirInvPi = 0.31830989;
static const float3 kRestirLuminance = float3(0.2126, 0.7152, 0.0722);  // p_hat = luminance of L_o (Eq. 10)

// Integer hash (the mixer of world_restir_temporal's selection number).
uint RestirHash(uint h)
{
    h ^= h >> 16u;
    h *= 0x7feb352du;
    h ^= h >> 15u;
    h *= 0x846ca68bu;
    h ^= h >> 16u;
    return h;
}

groupshared uint gs_sstats[RTGI_SSTAT_COUNT];

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    const uint group_index = gid.y * 8u + gid.x;
    if (group_index < RTGI_SSTAT_COUNT) gs_sstats[group_index] = 0u;
    GroupMemoryBarrierWithGroupSync();
    if (id.x < (uint)r_size.x && id.y < (uint)r_size.y)
    {
        const int2 tp = int2(id.xy);
        const uint d = max((uint)r_params.x, 1u);
        const uint2 full_size = uint2(r_size.zw);
        const int debug_mode = (int)r_params.y;
        const uint mode = (uint)r_params.z;
        const uint m_cap = (uint)r_extra.y;
        const float4 raw = g_raw_gi[tp];
        const float4 c_sample = g_res_sample[tp];
        const float4 c_visible = g_res_visible[tp];
        const uint4 c_data = g_res_data[tp];
        const uint c_m = min(c_data.z >> 16u, m_cap);
        const bool low = float(g_temporal_data[tp].z >> 16u) < r_params.w;
        float4 out_raw = raw;
        float4 res_sample = c_sample;
        float4 res_visible = c_visible;
        uint4 res_data = c_data;
        uint tried = 0u;
        uint accepted = 0u;
        const bool run = raw.a >= 0.0 && c_m > 0u && (mode == 0u || low);
        if (run)
        {
            InterlockedAdd(gs_sstats[mode == 0u ? 0u : 3u], 1u);
            // The receiver exactly as the trace and the temporal pass compute it (representative pixel).
            const uint2 rep = min(uint2(tp) * d + d / 2u, full_size - 1u);
            const float depth = g_game_depth.Load(int3(rep, 0)).x;
            const float2 uv = (float2(rep) + 0.5) / float2(full_size);
            const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
            const float3 xv = world_h.xyz / world_h.w;
            const float3 nv = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(rep, 0)).xy);
            const float3 camera = float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32);
            const float dist_q = length(xv - camera);
            const float fade = saturate((r_extra.w - dist_q) / max(r_extra.w - r_extra.z, 1e-4));

            // Canonical candidate: this pixel's own reservoir; MERGE weight p_hat x W x M (Alg. 1 line 13).
            const float3 l_c = float3(f16tof32(c_data.y & 0xFFFFu), f16tof32(c_data.y >> 16u), f16tof32(c_data.z & 0xFFFFu));
            const float p_c = dot(l_c, kRestirLuminance);
            float w_sum = c_sample.w > 0.0 ? p_c * c_sample.w * float(c_m) : 0.0;
            uint m_sum = c_m;
            float3 sel_xs = c_sample.xyz;
            uint3 sel_pack = uint3(c_data.x, c_data.y, c_data.z & 0xFFFFu);
            float3 sel_l = l_c;
            float sel_p = p_c;
            // Sec. 5: 9 neighbours while the sample count is low, otherwise 3; mode 1 uses 3.
            const uint iterations = mode == 0u ? (low ? 9u : 3u) : 3u;
            float radius = r_spatial.x;
            const uint base = (uint(tp.x) * 1973u + uint(tp.y) * 9277u + asuint(r_extra.x) * 26699u + mode * 0x68E31DA4u) | 1u;
            for (uint s = 0u; s < iterations; ++s)
            {
                const uint h1 = RestirHash(base + s * 0x9E3779B9u);
                const uint h2 = RestirHash(h1);
                const uint h3 = RestirHash(h2);
                const float u1 = float(h1 >> 8u) * (1.0 / 16777216.0);
                const float u2 = float(h2 >> 8u) * (1.0 / 16777216.0);
                const float u3 = float(h3 >> 8u) * (1.0 / 16777216.0);
                const float angle = 6.2831853 * u1;
                int2 off = int2(round(float2(cos(angle), sin(angle)) * (radius * sqrt(u2))));
                if (all(off == int2(0, 0))) off = int2(1, 0);
                const int2 qn = tp + off;
                tried += 1u;
                bool ok = all(qn >= 0) && all(qn < int2(r_size.xy));
                if (ok)
                {
                    const float4 n_sample = g_res_sample.Load(int3(qn, 0));
                    const float4 n_visible = g_res_visible.Load(int3(qn, 0));
                    const uint4 n_data = g_res_data.Load(int3(qn, 0));
                    const uint n_m = min(n_data.z >> 16u, m_cap);
                    const float3 n_nv = OctDecode(float2(f16tof32(n_data.w & 0xFFFFu), f16tof32(n_data.w >> 16u)));
                    // Sec. 4.2 similarity: normals within 25 degrees, depths within 0.05 (relative camera distance).
                    ok = n_m > 0u && n_sample.w > 0.0 && dot(n_nv, nv) >= r_spatial.z
                        && TemporalDepthAccept(length(n_visible.xyz - camera), dist_q, r_spatial.w);
                    if (ok)
                    {
                        // Reconnection Jacobian from the neighbour's visible point (Eq. 11; same form and guards as M2).
                        const float3 n_ns = OctDecode(float2(f16tof32(n_data.x & 0xFFFFu), f16tof32(n_data.x >> 16u)));
                        const float3 to_cur = xv - n_sample.xyz;
                        const float3 to_src = n_visible.xyz - n_sample.xyz;
                        const float d_cur2 = max(dot(to_cur, to_cur), 1e-12);
                        const float d_src2 = max(dot(to_src, to_src), 1e-12);
                        const float cos_cur = dot(n_ns, to_cur) * rsqrt(d_cur2);
                        const float cos_src = dot(n_ns, to_src) * rsqrt(d_src2);
                        const float cos_recv = -dot(nv, to_cur) * rsqrt(d_cur2);
                        const float J = (cos_cur * d_src2) / max(cos_src * d_cur2, 1e-12);
                        ok = cos_cur > 0.0 && cos_src > 0.0 && cos_recv > 0.0 && J <= kRestirJacobianMax
                            && J >= 1.0 / kRestirJacobianMax;
                        if (ok)
                        {
                            accepted += 1u;
                            const float3 hl = float3(f16tof32(n_data.y & 0xFFFFu), f16tof32(n_data.y >> 16u), f16tof32(n_data.z & 0xFFFFu));
                            const float hp_hat = dot(hl, kRestirLuminance);
                            const float hw = hp_hat * n_sample.w * float(n_m) * J;
                            w_sum += hw;
                            m_sum += n_m;
                            if (hw > 0.0 && u3 * w_sum < hw)
                            {
                                sel_xs = n_sample.xyz;
                                sel_pack = uint3(n_data.x, n_data.y, n_data.z & 0xFFFFu);
                                sel_l = hl;
                                sel_p = hp_hat;
                            }
                        }
                    }
                }
                // Sec. 5 adaptive radius: unchanged after a reused neighbour, otherwise halved down to the minimum.
                if (!ok) radius = max(radius * 0.5, r_spatial.y);
            }
            if (accepted > 0u)
            {
                // Biased normalisation: Z = the M of the canonical and every merged neighbour (Alg. 4 lines 16-20 without
                // visibility; with p_hat of Eq. 10 the line 18 test is always true). W = w_sum / (Z p_hat(z)).
                const float W = sel_p > 0.0 ? w_sum / (float(m_sum) * sel_p) : 0.0;
                const float3 to_s = sel_xs - xv;
                const float cos_s = max(0.0, dot(nv, to_s) * rsqrt(max(dot(to_s, to_s), 1e-12)));
                float3 gi = sel_l * (cos_s * kRestirInvPi) * W * fade;
                if (any(isnan(gi)) || any(isinf(gi)))
                {
                    gi = float3(0.0, 0.0, 0.0);
                }
                out_raw = float4(gi, 1.0);
                const float2 nv_oct = OctEncode(nv);
                res_sample = float4(sel_xs, W);
                res_visible = float4(xv, 0.0);
                res_data = uint4(sel_pack.x, sel_pack.y, sel_pack.z | (min(m_sum, 0xFFFFu) << 16u),
                                 f32tof16(nv_oct.x) | (f32tof16(nv_oct.y) << 16u));
            }
        }
        InterlockedAdd(gs_sstats[1], tried);
        InterlockedAdd(gs_sstats[2], accepted);
        g_raw_out[tp] = out_raw;
        if (mode == 0u)
        {
            // Mode 1's reservoir is not kept (it only feeds the raw GI); u2..u4 are null there.
            g_sample_write[tp] = res_sample;
            g_visible_write[tp] = res_visible;
            g_data_write[tp] = res_data;
        }
        // Debug 14: mode 0 writes every pixel (grey = merged / tried neighbours; dark purple = not run); mode 1 overwrites
        // the low-count pixels it ran (red, whiter with a larger merged share).
        if (debug_mode == 14 && (mode == 0u || run))
        {
            float3 debug_color = float3(0.25, 0.0, 0.25);
            if (run)
            {
                const float a = float(accepted) / max(float(tried), 1.0);
                debug_color = mode == 0u ? float3(a, a, a) : float3(1.0, a, a);
            }
            for (uint by = 0u; by < d; ++by)
            {
                for (uint bx = 0u; bx < d; ++bx)
                {
                    const uint2 block_px = uint2(tp) * d + uint2(bx, by);
                    if (all(block_px < full_size))
                    {
                        g_rtgi_debug[block_px] = float4(debug_color, 1.0);
                    }
                }
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (group_index < RTGI_SSTAT_COUNT) InterlockedAdd(g_rtgi_stats[RTGI_STAT_RESTIR_SPATIAL_PIXELS + group_index], gs_sstats[group_index]);
}
