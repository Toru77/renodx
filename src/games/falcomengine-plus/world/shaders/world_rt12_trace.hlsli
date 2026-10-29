#pragma once

// world_rt12_trace.hlsli - shared DXR trace include for RTAO and RTGI: the world BVH trace plus the RT12 instance masks,
// the TLAS binding and TraceWorldClosestDxr (DXR 1.1 inline ray queries, rt12_backend.hpp).
#include "world_bvh_trace.hlsli"

// Instance masks (rt12_backend.hpp kRt12Mask*).
#define RT12_MASK_OPAQUE_VISIBLE 1u
#define RT12_MASK_FOLIAGE_VISIBLE 2u
#define RT12_MASK_SHADOW_ONLY 4u
#define RT12_MASK_DYNAMIC 8u
#define RT12_DYNAMIC_INSTANCE_ID 0xFFFFFFu
#ifndef RT12_TLAS_REGISTER
#define RT12_TLAS_REGISTER t20
#endif
RaytracingAccelerationStructure g_rt12_tlas : register(RT12_TLAS_REGISTER);
#ifndef RT12_DYNAMIC_VERTICES_REGISTER
#define RT12_DYNAMIC_VERTICES_REGISTER t30
#endif
StructuredBuffer<float4> g_rt12_dynamic_vertices : register(RT12_DYNAMIC_VERTICES_REGISTER);
static uint g_rt12_dynamic_mask = 0u;  // RT12_MASK_DYNAMIC while RTAO traces characters

WorldTraceHit TraceWorldClosestDxr(float3 origin, float3 direction, float t_min, float t_max, WorldCameraView view,
                                   out WorldTraceCounters counters)
{
    TraceResetCounters(counters);
    WorldTraceHit hit;
    hit.t = t_max;
    hit.instance = 0xFFFFFFFFu;
    hit.mesh = 0xFFFFFFFFu;
    hit.prim = 0xFFFFFFFFu;
    hit.position = float3(0.0, 0.0, 0.0);
    hit.normal = float3(0.0, 0.0, 0.0);
    hit.hidden = 0u;
    hit.facing = 0.0;
    hit.material = 0u;

    uint mesh_count;
    uint mesh_stride;
    g_trace_meshes.GetDimensions(mesh_count, mesh_stride);
    const bool skip_foliage = (view.skip_flags & (INSTANCE_FOLIAGE | INSTANCE_ALPHA_TESTED)) != 0u;
    const uint mask = RT12_MASK_OPAQUE_VISIBLE | (skip_foliage ? 0u : RT12_MASK_FOLIAGE_VISIBLE) | g_rt12_dynamic_mask;
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = t_min;
    ray.TMax = t_max;
    RayQuery<RAY_FLAG_NONE> query;
    query.TraceRayInline(g_rt12_tlas, RAY_FLAG_NONE, mask, ray);
    while (query.Proceed())
    {
        if (query.CandidateType() != CANDIDATE_NON_OPAQUE_TRIANGLE) continue;
        const WorldInstanceGPU instance = g_trace_instances[query.CandidateInstanceID()];
        const uint mesh_id = (uint)instance.header.x;
        if (mesh_id >= mesh_count)
        {
            counters.invalid_refs++;
            continue;
        }
        const WorldMeshGPU mesh = g_trace_meshes[mesh_id];
        const uint base = (uint)mesh.header.z + query.CandidatePrimitiveIndex() * 3u;
        const uint i0 = g_trace_indices[base];
        const uint i1 = g_trace_indices[base + 1u];
        const uint i2 = g_trace_indices[base + 2u];
        if (AlphaCutHit((uint)instance.header.z, mesh.bbox_min.w, i0, i1, i2, query.CandidateTriangleBarycentrics(), counters)) continue;
        query.CommitNonOpaqueTriangleHit();
    }
    if (query.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        counters.misses = 1u;
        return hit;
    }
    counters.hits = 1u;
    hit.t = query.CommittedRayT();
    hit.prim = query.CommittedPrimitiveIndex();
    hit.position = origin + direction * hit.t;
    if (query.CommittedInstanceID() == 0xFFFFFFu)
    {
        // A character (rt12 kRt12DynamicInstanceId): as the compute path's dynamic objects.
        hit.instance = DYNAMIC_INSTANCE_FLAG;
        hit.mesh = DYNAMIC_MESH_MARKER;
        hit.material = 0u;
    }
    else
    {
        hit.instance = query.CommittedInstanceID();
        const WorldInstanceGPU committed = g_trace_instances[hit.instance];
        hit.mesh = (uint)committed.header.x;
        hit.material = (uint)committed.header.z;
    }
    return hit;
}

// RTGI rays: TraceWorldClosestDxr with any_hit (first hit ends the search) and view.require_flags (hits only on
// instances with all of those INSTANCE_* bits, forced non-opaque). Characters commit as dynamic objects.
WorldTraceHit TraceWorldRayDxr(float3 origin, float3 direction, float t_min, float t_max, bool any_hit, WorldCameraView view,
                               out WorldTraceCounters counters)
{
    TraceResetCounters(counters);
    WorldTraceHit hit;
    hit.t = t_max;
    hit.instance = 0xFFFFFFFFu;
    hit.mesh = 0xFFFFFFFFu;
    hit.prim = 0xFFFFFFFFu;
    hit.position = float3(0.0, 0.0, 0.0);
    hit.normal = float3(0.0, 0.0, 0.0);
    hit.hidden = 0u;
    hit.facing = 0.0;
    hit.material = 0u;

    uint mesh_count;
    uint mesh_stride;
    g_trace_meshes.GetDimensions(mesh_count, mesh_stride);
    const bool skip_foliage = (view.skip_flags & (INSTANCE_FOLIAGE | INSTANCE_ALPHA_TESTED)) != 0u;
    const uint mask = RT12_MASK_OPAQUE_VISIBLE | (skip_foliage ? 0u : RT12_MASK_FOLIAGE_VISIBLE) |
                      (view.hide ? 0u : RT12_MASK_SHADOW_ONLY) | g_rt12_dynamic_mask;
    uint flags = any_hit ? RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH : RAY_FLAG_NONE;
    if (view.require_flags != 0u) flags |= RAY_FLAG_FORCE_NON_OPAQUE;
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = t_min;
    ray.TMax = t_max;
    RayQuery<RAY_FLAG_NONE> query;
    query.TraceRayInline(g_rt12_tlas, flags, mask, ray);
    while (query.Proceed())
    {
        if (query.CandidateType() != CANDIDATE_NON_OPAQUE_TRIANGLE) continue;
        if (query.CandidateInstanceID() == RT12_DYNAMIC_INSTANCE_ID)
        {
            query.CommitNonOpaqueTriangleHit();
            continue;
        }
        const WorldInstanceGPU instance = g_trace_instances[query.CandidateInstanceID()];
        const uint mesh_id = (uint)instance.header.x;
        if (mesh_id >= mesh_count)
        {
            counters.invalid_refs++;
            continue;
        }
        const uint visibility_flags = (uint)(instance.visibility.z + 0.5);
        if ((visibility_flags & view.require_flags) != view.require_flags) continue;
        const WorldMeshGPU mesh = g_trace_meshes[mesh_id];
        const uint base = (uint)mesh.header.z + query.CandidatePrimitiveIndex() * 3u;
        const uint i0 = g_trace_indices[base];
        const uint i1 = g_trace_indices[base + 1u];
        const uint i2 = g_trace_indices[base + 2u];
        if (AlphaCutHit((uint)instance.header.z, mesh.bbox_min.w, i0, i1, i2, query.CandidateTriangleBarycentrics(), counters)) continue;
        query.CommitNonOpaqueTriangleHit();
    }
    if (query.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        counters.misses = 1u;
        return hit;
    }
    counters.hits = 1u;
    hit.t = query.CommittedRayT();
    hit.prim = query.CommittedPrimitiveIndex();
    hit.position = origin + direction * hit.t;
    if (query.CommittedInstanceID() == RT12_DYNAMIC_INSTANCE_ID)
    {
        // A character: its triangle is in g_rt12_dynamic_vertices at the instance's contribution + prim * 3.
        const uint v = query.CommittedInstanceContributionToHitGroupIndex() + hit.prim * 3u;
        hit.normal = TriangleNormal(g_rt12_dynamic_vertices[v].xyz, g_rt12_dynamic_vertices[v + 1u].xyz, g_rt12_dynamic_vertices[v + 2u].xyz);
        hit.instance = DYNAMIC_INSTANCE_FLAG;
        hit.mesh = DYNAMIC_MESH_MARKER;
        hit.material = 0u;
        return hit;
    }
    hit.instance = query.CommittedInstanceID();
    const WorldInstanceGPU committed = g_trace_instances[hit.instance];
    const uint mesh_id = (uint)committed.header.x;
    hit.mesh = mesh_id;
    hit.material = (uint)committed.header.z;
    hit.normal = float3(0.0, 1.0, 0.0);
    if (mesh_id >= mesh_count)
    {
        counters.invalid_refs++;
        return hit;
    }
    const WorldMeshGPU mesh = g_trace_meshes[mesh_id];
    const uint vertex_offset = (uint)mesh.header.x;
    const uint vertex_count = (uint)mesh.header.y;
    const uint index_offset = (uint)mesh.header.z;
    const uint index_count = (uint)mesh.header.w;
    if (hit.prim * 3u + 2u >= index_count)
    {
        counters.invalid_refs++;
        return hit;
    }
    const uint i0 = g_trace_indices[index_offset + hit.prim * 3u];
    const uint i1 = g_trace_indices[index_offset + hit.prim * 3u + 1u];
    const uint i2 = g_trace_indices[index_offset + hit.prim * 3u + 2u];
    if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
    {
        counters.invalid_refs++;
        return hit;
    }
    const float3 v0 = g_trace_vertices[vertex_offset + i0].xyz;
    const float3 v1 = g_trace_vertices[vertex_offset + i1].xyz;
    const float3 v2 = g_trace_vertices[vertex_offset + i2].xyz;
    const float4 normal_h = float4(TriangleNormal(v0, v1, v2), 0.0);
    hit.normal = normalize(float3(
        dot(normal_h, committed.world[0]),
        dot(normal_h, committed.world[1]),
        dot(normal_h, committed.world[2])));
    return hit;
}
