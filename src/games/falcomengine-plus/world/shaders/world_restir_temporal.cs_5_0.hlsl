// world_restir_temporal.cs_5_0.hlsl - ReSTIR GI M2: temporal reuse (Ouyang et al. 2021, reference/restir/ReSTIR GI.md,
// Alg. 1 MERGE and Alg. 3; temporal M cap 30 of Sec. 5). Biased: no visibility test and no sample validation (M4, M5).
// One thread per trace pixel tp; its representative pixel is min(tp * d + d / 2, full - 1), as in the RTGI trace.
// The canonical candidate is this frame's initial reservoir (t4, t5; world_rtgi.cs_5_0.hlsl RTGI_FLAG_RESTIR layout);
// the other is last frame's temporal reservoir (t6..t8) at the reprojected trace pixel (CustomTAA_Reproject, nearest).
// Writes the raw GI (u0, the trace's contract: rgb GI x fade, a = 1 valid, -1 not traced; the trace's value unchanged
// when no history is merged), this frame's temporal reservoir (u2..u4), counters 24..27 (u1) and debug view 13 (u5).

#include "../../taa/taa_common.hlsli"
#include "../../include/mrt_normal.hlsli"
#include "world_rt_denoise.hlsli"

Texture2D<float4> g_raw_gi : register(t0);
Texture2D<float4> g_game_depth : register(t1);
Texture2D<uint4> g_mrt_normal : register(t2);
Texture2D<float4> g_motion : register(t3);
Texture2D<float4> g_initial_pos : register(t4);   // x_s, W
Texture2D<uint4> g_initial_data : register(t5);   // n_s, L_o rg, L_o b | M << 16, flags
Texture2D<float4> g_prev_sample : register(t6);   // x_s, W
Texture2D<float4> g_prev_visible : register(t7);  // x_v, 0
Texture2D<uint4> g_prev_data : register(t8);      // n_s, L_o rg, L_o b | M << 16, n_v

RWTexture2D<float4> g_raw_out : register(u0);
RWStructuredBuffer<uint> g_rtgi_stats : register(u1);
RWTexture2D<float4> g_sample_write : register(u2);
RWTexture2D<float4> g_visible_write : register(u3);
RWTexture2D<uint4> g_data_write : register(u4);
RWTexture2D<float4> g_rtgi_debug : register(u5);

SamplerState g_samp_point : register(s0);
SamplerState g_samp_linear : register(s1);

cbuffer cb_scene : register(b0)
{
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
};

// Push constants (RestirTemporalPushConstants in rtgi_resources.hpp, 16 floats).
cbuffer cb_restir_temporal : register(b12)
{
    float4 r_size : packoffset(c0);    // trace width, trace height, full width, full height
    float4 r_params : packoffset(c1);  // trace divisor d, debug mode, history valid (1, 0 = reset), M cap
    float4 r_reject : packoffset(c2);  // depth rejection, normal rejection, fade start_eff, fade end_eff
    float4 r_seed : packoffset(c3);    // noise seed, unused x3
};

#define RTGI_STAT_RESTIR_PIXELS 24u
#define RTGI_STAT_RESTIR_ACCEPTED 25u
#define RTGI_STAT_RESTIR_FROM_HISTORY 26u
#define RTGI_STAT_RESTIR_JACOBIAN_REJECTED 27u
#define RTGI_RSTAT_COUNT 4u
#define RTGI_RESTIR_VALID 1u
#define RTGI_RESTIR_SURFACE 2u
static const float kRestirJacobianMax = 10.0;  // non-paper: history dropped when J > 10 or J < 1 / 10 (singular reconnections)
static const float kRestirInvPi = 0.31830989;
static const float3 kRestirLuminance = float3(0.2126, 0.7152, 0.0722);  // p_hat = luminance of L_o (Eq. 10), as the trace

groupshared uint gs_rstats[RTGI_RSTAT_COUNT];

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    const uint group_index = gid.y * 8u + gid.x;
    if (group_index < RTGI_RSTAT_COUNT) gs_rstats[group_index] = 0u;
    GroupMemoryBarrierWithGroupSync();
    if (id.x < (uint)r_size.x && id.y < (uint)r_size.y)
    {
        const int2 tp = int2(id.xy);
        const uint d = max((uint)r_params.x, 1u);
        const uint2 full_size = uint2(r_size.zw);
        const int debug_mode = (int)r_params.y;
        const float4 raw = g_raw_gi[tp];
        const float4 s0_pos = g_initial_pos[tp];
        const uint4 s0_data = g_initial_data[tp];
        const uint m0 = s0_data.z >> 16u;
        float4 out_raw = raw;
        float4 res_sample = float4(0.0, 0.0, 0.0, 0.0);
        float4 res_visible = float4(0.0, 0.0, 0.0, 0.0);
        uint4 res_data = uint4(0u, 0u, 0u, 0u);
        float3 debug_color = float3(0.25, 0.0, 0.25);
        if (raw.a >= 0.0 && (s0_data.w & RTGI_RESTIR_SURFACE) != 0u && m0 > 0u)
        {
            InterlockedAdd(gs_rstats[0], 1u);
            // The visible point and normal exactly as the trace computes them (world_rtgi.cs_5_0.hlsl, representative pixel).
            const uint2 rep = min(uint2(tp) * d + d / 2u, full_size - 1u);
            const float depth = g_game_depth.Load(int3(rep, 0)).x;
            const float2 uv = (float2(rep) + 0.5) / float2(full_size);
            const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
            const float3 xv = world_h.xyz / world_h.w;
            const float3 nv = DecodeFalcomMrtNormalAsGame(g_mrt_normal.Load(int3(rep, 0)).xy);
            const float3 camera = float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32);
            const float dist_cur = length(xv - camera);
            const float fade = saturate((r_reject.w - dist_cur) / max(r_reject.w - r_reject.z, 1e-4));

            // Canonical candidate: the initial reservoir (z0, W0, M0); MERGE weight p_hat x W x M (Alg. 1 line 13).
            const bool s0_valid = (s0_data.w & RTGI_RESTIR_VALID) != 0u && s0_pos.w > 0.0;
            const float3 l0 = float3(f16tof32(s0_data.y & 0xFFFFu), f16tof32(s0_data.y >> 16u), f16tof32(s0_data.z & 0xFFFFu));
            const float p0 = dot(l0, kRestirLuminance);
            float w_sum = s0_valid ? p0 * s0_pos.w * float(m0) : 0.0;
            uint m_sum = m0;
            bool accepted = false;
            bool from_history = false;
            float3 sel_xs = s0_pos.xyz;
            uint3 sel_pack = uint3(s0_data.x, s0_data.y, s0_data.z & 0xFFFFu);  // n_s, L_o rg, L_o b
            float3 sel_l = l0;
            float sel_p = p0;
            if (r_params.z > 0.5)
            {
                float motion_px = 0.0;
                float depth_spread = 0.0;
                const float2 hist_uv = CustomTAA_Reproject(uv, g_game_depth, g_samp_point, g_motion, g_samp_linear,
                    float2(1.0, 1.0) / float2(full_size), float2(1.0, 1.0), motion_px, depth_spread);
                // Trace position of the reprojected representative pixel (as world_svgf_reproject), nearest trace pixel.
                const float2 hp = (hist_uv * float2(full_size) - 0.5 - float(d / 2u)) / float(d);
                const int2 q = (int2)floor(hp + 0.5);
                if (all(q >= 0) && all(q < int2(r_size.xy)))
                {
                    const float4 h_sample = g_prev_sample.Load(int3(q, 0));
                    const float4 h_visible = g_prev_visible.Load(int3(q, 0));
                    const uint4 h_data = g_prev_data.Load(int3(q, 0));
                    const uint h_m = min(h_data.z >> 16u, (uint)r_params.w);
                    const float3 h_nv = OctDecode(float2(f16tof32(h_data.w & 0xFFFFu), f16tof32(h_data.w >> 16u)));
                    // Reprojection test: the reservoir's visible point lies within Depth Rejection x distance of this one
                    // and its normal passes Normal Rejection (the RTGI rows the SVGF reprojection uses).
                    if (h_m > 0u && h_sample.w > 0.0 && length(h_visible.xyz - xv) <= r_reject.x * dist_cur
                        && dot(h_nv, nv) >= r_reject.y)
                    {
                        // Reconnection Jacobian (GI Eq. 11; the GRIS weight |dT/dX| of PT Enhanced Sec. 2.1):
                        // J = (cos phi_cur / |x_v - x_s|^2) / (cos phi_prev / |x_v,prev - x_s|^2), phi at the sample normal.
                        const float3 h_ns = OctDecode(float2(f16tof32(h_data.x & 0xFFFFu), f16tof32(h_data.x >> 16u)));
                        const float3 to_cur = xv - h_sample.xyz;
                        const float3 to_prev = h_visible.xyz - h_sample.xyz;
                        const float d_cur2 = max(dot(to_cur, to_cur), 1e-12);
                        const float d_prev2 = max(dot(to_prev, to_prev), 1e-12);
                        const float cos_cur = dot(h_ns, to_cur) * rsqrt(d_cur2);
                        const float cos_prev = dot(h_ns, to_prev) * rsqrt(d_prev2);
                        const float cos_recv = -dot(nv, to_cur) * rsqrt(d_cur2);
                        const float J = (cos_cur * d_prev2) / max(cos_prev * d_cur2, 1e-12);
                        if (cos_cur > 0.0 && cos_prev > 0.0 && cos_recv > 0.0 && J <= kRestirJacobianMax
                            && J >= 1.0 / kRestirJacobianMax)
                        {
                            accepted = true;
                            InterlockedAdd(gs_rstats[1], 1u);
                            const float3 hl = float3(f16tof32(h_data.y & 0xFFFFu), f16tof32(h_data.y >> 16u), f16tof32(h_data.z & 0xFFFFu));
                            const float hp_hat = dot(hl, kRestirLuminance);
                            const float hw = hp_hat * h_sample.w * float(h_m) * J;
                            w_sum += hw;
                            m_sum += h_m;
                            // Selection random number (integer hash of the pixel and the frame seed).
                            uint h = (uint(tp.x) * 1973u + uint(tp.y) * 9277u + asuint(r_seed.x) * 26699u) | 1u;
                            h ^= h >> 16u;
                            h *= 0x7feb352du;
                            h ^= h >> 15u;
                            h *= 0x846ca68bu;
                            h ^= h >> 16u;
                            const float u = float(h >> 8u) * (1.0 / 16777216.0);
                            if (hw > 0.0 && u * w_sum < hw)
                            {
                                from_history = true;
                                sel_xs = h_sample.xyz;
                                sel_pack = uint3(h_data.x, h_data.y, h_data.z & 0xFFFFu);
                                sel_l = hl;
                                sel_p = hp_hat;
                            }
                        }
                        else
                        {
                            InterlockedAdd(gs_rstats[3], 1u);
                        }
                    }
                }
            }
            const float2 nv_oct = OctEncode(nv);
            const uint nv_pack = f32tof16(nv_oct.x) | (f32tof16(nv_oct.y) << 16u);
            if (accepted)
            {
                // W = w_sum / (M p_hat(z)) (Alg. 3 line 6, Eq. 7); f(z) = L_o x cos / pi at this pixel (albedo: the composite).
                const float W = sel_p > 0.0 ? w_sum / (float(m_sum) * sel_p) : 0.0;
                const float3 to_s = sel_xs - xv;
                const float cos_s = max(0.0, dot(nv, to_s) * rsqrt(max(dot(to_s, to_s), 1e-12)));
                float3 gi = sel_l * (cos_s * kRestirInvPi) * W * fade;
                if (any(isnan(gi)) || any(isinf(gi)))
                {
                    gi = float3(0.0, 0.0, 0.0);
                }
                out_raw = float4(gi, 1.0);
                if (from_history)
                {
                    InterlockedAdd(gs_rstats[2], 1u);
                }
                res_sample = float4(sel_xs, W);
                res_data = uint4(sel_pack.x, sel_pack.y, sel_pack.z | (min(m_sum, 0xFFFFu) << 16u), nv_pack);
            }
            else
            {
                // No history merged: the trace's raw GI unchanged (M1), and the initial reservoir becomes the temporal one.
                res_sample = float4(s0_pos.xyz, s0_valid ? s0_pos.w : 0.0);
                res_data = uint4(s0_data.x, s0_data.y, s0_data.z, nv_pack);
            }
            res_visible = float4(xv, 0.0);
            if (debug_mode == 13)
            {
                // History length: M / (M cap + 4) (4 = the Samples Per Pixel maximum).
                const float m = saturate(float(res_data.z >> 16u) / (r_params.w + 4.0));
                debug_color = float3(m, m, m);
            }
        }
        g_raw_out[tp] = out_raw;
        g_sample_write[tp] = res_sample;
        g_visible_write[tp] = res_visible;
        g_data_write[tp] = res_data;
        if (debug_mode == 13)
        {
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
    if (group_index < RTGI_RSTAT_COUNT) InterlockedAdd(g_rtgi_stats[RTGI_STAT_RESTIR_PIXELS + group_index], gs_rstats[group_index]);
}
