// world_rt12_rtao.cs_6_5.hlsl - M3: RTAO pass A on D3D12 (DXR 1.1 inline ray queries, rt12_backend.hpp RunRtaoPassA).
// The per-pixel code is world_rtao.cs_5_0.hlsl TracePixel, included with RTAO_DXR: its inputs are the shared copies made
// by world_rt12_pack.cs_5_0.hlsl (one texel per trace pixel, so the loads divide the representative pixel by the trace
// divisor) and its rays go through TraceWorldClosestDxr. Valid for RTAO's view only: hide on, near fade off
// (floor_value 1), skip_flags 0 or INSTANCE_FOLIAGE | INSTANCE_ALPHA_TESTED. Characters and P2b records are not traced.
#define RTAO_DXR
#include "world_rt12_trace.hlsli"

Texture2D<uint4> g_rt12_normal : register(t17);
Texture2D<uint> g_rt12_depth : register(t21);
Texture2D<uint4> g_rt12_scene : register(t22);
Texture2D<uint4> g_rt12_noise : register(t23);
RWTexture2D<uint> g_rtao_ao : register(u0);
RWTexture2D<uint> g_rt12_stats : register(u1);
RWTexture2D<float> g_rtao_raw : register(u2);

static uint g_rt12_d = 1u;

struct Rt12Depth
{
    uint unused;
    float4 Load(int3 p) { return asfloat(g_rt12_depth.Load(int3(p.xy / (int)g_rt12_d, 0))).xxxx; }
};
struct Rt12Normal
{
    uint unused;
    uint4 Load(int3 p) { return g_rt12_normal.Load(int3(p.xy / (int)g_rt12_d, 0)); }
};
struct Rt12Noise
{
    uint unused;
    float2 Load(int4 p) { return asfloat(g_rt12_noise.Load(int3(p.x, p.y + 128 * p.z, 0)).xy); }
};
static Rt12Depth g_game_depth;
static Rt12Normal g_mrt_normal;
static Rt12Noise g_isfast_noise;
static float4x4 viewInv_g;
static float4x4 viewProjInv_g;
static float2 resolutionScaling_g;

#define RTAO_TRACE_CLOSEST TraceWorldClosestDxr
#include "world_rtao.cs_5_0.hlsl"

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID)
{
    g_rt12_d = max((uint)g_rtao_trace.x, 1u);
    g_rt12_dynamic_mask = (uint)g_rtao_size.z > 0u ? RT12_MASK_DYNAMIC : 0u;
    viewInv_g = float4x4(asfloat(g_rt12_scene.Load(int3(0, 0, 0))), asfloat(g_rt12_scene.Load(int3(1, 0, 0))),
                         asfloat(g_rt12_scene.Load(int3(2, 0, 0))), asfloat(g_rt12_scene.Load(int3(3, 0, 0))));
    viewProjInv_g = float4x4(asfloat(g_rt12_scene.Load(int3(4, 0, 0))), asfloat(g_rt12_scene.Load(int3(5, 0, 0))),
                             asfloat(g_rt12_scene.Load(int3(6, 0, 0))), asfloat(g_rt12_scene.Load(int3(7, 0, 0))));
    resolutionScaling_g = asfloat(g_rt12_scene.Load(int3(8, 0, 0)).xy);
    const uint group_index = gid.y * 8u + gid.x;
    if (group_index < RTAO_STAT_COUNT) gs_stats[group_index] = 0u;
    if (group_index < RTAO_DISC_COUNT) gs_disc[group_index] = 0u;
    GroupMemoryBarrierWithGroupSync();

    const uint2 trace_size = (uint2(g_rtao_size.xy) + g_rt12_d - 1u) / g_rt12_d;
    if (all(id.xy < trace_size)) TracePixel(id.xy);

    GroupMemoryBarrierWithGroupSync();
    if (group_index < RTAO_STAT_COUNT) InterlockedAdd(g_rt12_stats[uint2(group_index, 0u)], gs_stats[group_index]);
}
