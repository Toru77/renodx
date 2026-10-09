#pragma once

// world_bvh_trace.hlsli - M4 read-only BVH traversal.
//
// TraceWorldClosest / TraceWorldAny trace a world-space ray through the region
// TLAS, transform it into each candidate mesh's local space with the stored
// inverse world matrix (row-dot convention; the affine transform preserves the
// ray parameterization, so hit t stays in world units), then walk the mesh
// BLAS and test triangles with Moller-Trumbore.
//
// All references are bounds-checked and clamped: malformed data increments
// invalid_refs instead of dereferencing out of range, and the stack never
// overflows (stack_overflow is counted instead). Per-thread counters are
// accumulated by the caller.
//
// Game camera view (rays that start at the camera). A surface is hidden from
// the game camera when its instance was never drawn by a camera vertex shader
// (no INSTANCE_CAMERA_VISIBLE: shadow-only casters), or when its instance has
// INSTANCE_NEAR_FADE and its t lies outside CameraFadeInterval: the distance
// along a camera ray is the distance the game's map-object pixel shaders fade
// by (camera_fade.hpp). With view.hide hidden surfaces are skipped (traversal
// continues behind them); either way camera_hidden reports whether the
// nearest BVH surface along the ray is one.
//
// Deforming meshes (characters, water) are traced after the TLAS: each
// object's root bounds are tested, then its BLAS (already in world space, no
// transform). The game camera drew them this frame, so they are always shown.

#include "world_bvh_types.hlsli"

#define TRACE_STACK_SIZE 64u

StructuredBuffer<float4> g_trace_vertices : register(t0);
StructuredBuffer<uint> g_trace_indices : register(t1);
StructuredBuffer<WorldMeshGPU> g_trace_meshes : register(t2);
StructuredBuffer<WorldInstanceGPU> g_trace_instances : register(t3);
StructuredBuffer<uint> g_trace_active : register(t4);
StructuredBuffer<BVHNodeGPU> g_trace_blas_nodes : register(t5);
StructuredBuffer<BVHLeafGPU> g_trace_blas_leaves : register(t6);
StructuredBuffer<BVHNodeGPU> g_trace_tlas_nodes : register(t7);
StructuredBuffer<BVHLeafGPU> g_trace_tlas_leaves : register(t8);
// Deforming meshes of this frame (deform_live.hpp): captured world-space
// vertices (float3 as three floats), the objects, their refit BLAS.
Buffer<float> g_trace_dynamic_vertices : register(t10);
StructuredBuffer<DynamicObjectGPU> g_trace_dynamic_objects : register(t11);
StructuredBuffer<BVHNodeGPU> g_trace_dynamic_nodes : register(t12);
StructuredBuffer<BVHLeafGPU> g_trace_dynamic_leaves : register(t13);
// Alpha-tested foliage (alpha_live.hpp): UVs per vertex (a mesh with UVs has bbox_min.w = uv_offset + 1),
// the atlas (one slice per material) and the material table.
StructuredBuffer<float2> g_trace_uvs : register(t14);
Texture2DArray<uint> g_alpha_atlas : register(t15);
StructuredBuffer<AlphaMaterialGPU> g_alpha_materials : register(t16);
// Objects in g_trace_dynamic_objects; set by the including shader before tracing.
static uint g_trace_dynamic_count = 0u;

struct WorldTraceHit
{
    float t;
    uint instance;
    uint mesh;
    uint prim;
    float3 position;
    float3 normal;
    uint hidden;           // 1: the game camera does not show the hit surface (only when view.hide is off)
    float facing;          // Round 2 discovery: dot(local normal, local direction) of the winning triangle; 0 for dynamic objects
    uint material;         // Round 2 discovery: instance.header.z of the winning instance (alpha-tested material when != 0); 0 for dynamic objects
};

struct WorldTraceCounters
{
    uint hits;
    uint misses;
    uint invalid_refs;
    uint stack_overflow;
    uint triangle_tests;
    uint max_stack_depth;
    uint camera_hidden;    // 1: the game camera does not show the nearest BVH surface along the ray
    uint alpha_tests;      // triangle hits of alpha-tested materials tested against the atlas
    uint alpha_cut;        // of those, the ones cut (alpha below the threshold)
};

// Ray-wide game camera settings (rays from the camera only).
struct WorldCameraView
{
    bool hide;             // skip surfaces the game camera does not show
    float floor_value;     // cb_scene.disableMapObjNearFade_g
};

void TraceResetCounters(out WorldTraceCounters counters)
{
    counters.hits = 0u;
    counters.misses = 0u;
    counters.invalid_refs = 0u;
    counters.stack_overflow = 0u;
    counters.triangle_tests = 0u;
    counters.max_stack_depth = 0u;
    counters.camera_hidden = 0u;
    counters.alpha_tests = 0u;
    counters.alpha_cut = 0u;
}

// Mirrors camera_fade.hpp CameraFadeInterval: the distances [lo, hi] at which
// the near fade draws at least half of the dither pattern.
#define CAMERA_FADE_SHOWN 0.5
#define CAMERA_FADE_NEVER 3.0e38

bool CameraFadeInterval(float start, float inv_range, float floor_value, out float lo, out float hi)
{
    lo = 0.0;
    hi = CAMERA_FADE_NEVER;
    if (floor_value >= CAMERA_FADE_SHOWN) return true;
    if (inv_range > 0.0)
    {
        lo = start + CAMERA_FADE_SHOWN / inv_range;
        return true;
    }
    if (inv_range < 0.0)
    {
        hi = start + CAMERA_FADE_SHOWN / inv_range;
        if (hi >= 0.0) return true;
    }
    lo = CAMERA_FADE_NEVER;
    hi = -1.0;
    return false;
}

bool AabbIntersect(
    float3 origin,
    float3 direction,
    float3 bounds_min,
    float3 bounds_max,
    float t_min,
    float t_max,
    out float t_near,
    out float t_far)
{
    const float3 inv_direction = 1.0 / direction;
    const float3 t0 = (bounds_min - origin) * inv_direction;
    const float3 t1 = (bounds_max - origin) * inv_direction;
    const float3 t_small = min(t0, t1);
    const float3 t_big = max(t0, t1);
    t_near = max(max(t_small.x, t_small.y), max(t_small.z, t_min));
    t_far = min(min(t_big.x, t_big.y), min(t_big.z, t_max));
    return t_near <= t_far;
}

bool IntersectTriangle(
    float3 origin,
    float3 direction,
    float3 v0,
    float3 v1,
    float3 v2,
    out float t,
    out float3 normal,
    out float2 bary)
{
    t = 0.0;
    normal = float3(0.0, 1.0, 0.0);
    bary = float2(0.0, 0.0);
    const float3 edge1 = v1 - v0;
    const float3 edge2 = v2 - v0;
    const float3 p = cross(direction, edge2);
    const float det = dot(edge1, p);
    if (abs(det) < 1e-12) return false;
    const float inv_det = 1.0 / det;
    const float3 tv = origin - v0;
    const float u = dot(tv, p) * inv_det;
    if (u < 0.0 || u > 1.0) return false;
    const float3 q = cross(tv, edge1);
    const float v = dot(direction, q) * inv_det;
    if (v < 0.0 || u + v > 1.0) return false;
    const float hit_t = dot(edge2, q) * inv_det;
    if (hit_t <= 0.0) return false;
    const float3 raw_normal = cross(edge1, edge2);
    const float normal_length = length(raw_normal);
    bary = float2(u, v);
    t = hit_t;
    if (normal_length > 1e-12) normal = raw_normal / normal_length;
    return true;
}

// Alpha cutout of a triangle hit (alpha-tested foliage): true when the material's alpha at the hit's UV is
// below its threshold. The UV is interpolated from the mesh's UVs (uv_slot = bbox_min.w, uv_offset + 1) and
// transformed as the game's pixel shader does: (u + scroll.x, 1 - (v + scroll.y)), wrapped. The atlas is point
// sampled at LOD 0. A material or UV that cannot be read counts as invalid_refs and keeps the surface solid.
bool AlphaCutHit(uint material, float uv_slot, uint i0, uint i1, uint i2, float2 bary, inout WorldTraceCounters counters)
{
    if (material == 0u) return false;
    uint material_count;
    uint material_stride;
    g_alpha_materials.GetDimensions(material_count, material_stride);
    uint uv_count;
    uint uv_stride;
    g_trace_uvs.GetDimensions(uv_count, uv_stride);
    if (material - 1u >= material_count || uv_slot < 1.0)
    {
        counters.invalid_refs++;
        return false;
    }
    const AlphaMaterialGPU entry = g_alpha_materials[material - 1u];
    if ((entry.swizzle & 1u) != 0u) return false;  // alpha forced to 1: solid
    const uint uv_base = (uint)uv_slot - 1u;
    if (uv_base + max(i0, max(i1, i2)) >= uv_count)
    {
        counters.invalid_refs++;
        return false;
    }
    counters.alpha_tests++;
    const float2 uv0 = g_trace_uvs[uv_base + i0];
    const float2 uv1 = g_trace_uvs[uv_base + i1];
    const float2 uv2 = g_trace_uvs[uv_base + i2];
    const float2 uv = (1.0 - bary.x - bary.y) * uv0 + bary.x * uv1 + bary.y * uv2;
    const uint x = min((uint)floor(frac(uv.x + entry.scroll.x) * 1024.0), 1023u);
    const uint y = min((uint)floor(frac(1.0 - (uv.y + entry.scroll.y)) * 256.0), 255u);
    const uint word = g_alpha_atlas.Load(int4(x >> 2u, y, entry.slice, 0));
    const uint texel = (word >> ((x & 3u) * 8u)) & 0xFFu;
    const bool cut = (float)texel * (1.0 / 255.0) - entry.threshold < 0.0;
    if (cut) counters.alpha_cut++;
    return cut;
}

// shown_min / shown_max: the t range in which the game camera shows this
// instance's surfaces. A hit outside it is skipped when hide_hidden (its t
// goes to hidden_t), otherwise accepted with out_hidden = 1.
bool TraceBlas(
    uint mesh_id,
    uint material,
    float3 origin,
    float3 direction,
    float t_min,
    inout float t_max,
    float shown_min,
    float shown_max,
    bool hide_hidden,
    inout float hidden_t,
    inout WorldTraceCounters counters,
    out uint out_prim,
    out float3 out_normal,
    out uint out_hidden)
{
    out_prim = 0xFFFFFFFFu;
    out_normal = float3(0.0, 0.0, 0.0);
    out_hidden = 0u;

    const WorldMeshGPU mesh = g_trace_meshes[mesh_id];
    const uint leaf_count = (uint)mesh.build.w;
    if (leaf_count == 0u) return false;
    const uint node_count = 2u * leaf_count - 1u;
    const uint node_base = (uint)mesh.build.x;
    const uint leaf_base = (uint)mesh.build.z;
    const uint vertex_offset = (uint)mesh.header.x;
    const uint vertex_count = (uint)mesh.header.y;
    const uint index_offset = (uint)mesh.header.z;
    const uint index_count = (uint)mesh.header.w;

    uint stack[TRACE_STACK_SIZE];
    uint stack_size = 0u;
    uint node_index = 0u;
    bool found = false;

    while (true)
    {
        if (node_index >= node_count)
        {
            counters.invalid_refs++;
        }
        else
        {
            const BVHNodeGPU node = g_trace_blas_nodes[node_base + node_index];
            float t_near;
            float t_far;
            if (AabbIntersect(origin, direction, node.bounds_min, node.bounds_max, t_min, t_max, t_near, t_far))
            {
                if ((node.child_or_leaf & BVH_LEAF_FLAG) != 0u)
                {
                    const uint leaf_index = node.child_or_leaf & 0x7FFFFFFFu;
                    if (leaf_index >= leaf_count)
                    {
                        counters.invalid_refs++;
                    }
                    else
                    {
                        const BVHLeafGPU leaf = g_trace_blas_leaves[leaf_base + leaf_index];
                        const uint prim = leaf.prim;
                        if (prim * 3u + 2u >= index_count)
                        {
                            counters.invalid_refs++;
                        }
                        else
                        {
                            const uint i0 = g_trace_indices[index_offset + prim * 3u + 0u];
                            const uint i1 = g_trace_indices[index_offset + prim * 3u + 1u];
                            const uint i2 = g_trace_indices[index_offset + prim * 3u + 2u];
                            if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
                            {
                                counters.invalid_refs++;
                            }
                            else
                            {
                                const float3 v0 = g_trace_vertices[vertex_offset + i0].xyz;
                                const float3 v1 = g_trace_vertices[vertex_offset + i1].xyz;
                                const float3 v2 = g_trace_vertices[vertex_offset + i2].xyz;
                                float tri_t;
                                float3 tri_normal;
                                float2 tri_bary;
                                counters.triangle_tests++;
                                if (IntersectTriangle(origin, direction, v0, v1, v2, tri_t, tri_normal, tri_bary)
                                    && tri_t >= t_min && tri_t < t_max
                                    && !AlphaCutHit(material, mesh.bbox_min.w, i0, i1, i2, tri_bary, counters))
                                {
                                    const bool shown = tri_t >= shown_min && tri_t <= shown_max;
                                    if (shown || !hide_hidden)
                                    {
                                        t_max = tri_t;
                                        out_prim = prim;
                                        out_normal = tri_normal;
                                        out_hidden = shown ? 0u : 1u;
                                        found = true;
                                    }
                                    else
                                    {
                                        hidden_t = min(hidden_t, tri_t);
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    const uint left = node.child_or_leaf;
                    const uint right = node.sibling_or_right;
                    if (left >= node_count || right >= node_count || left == right)
                    {
                        counters.invalid_refs++;
                    }
                    else if (stack_size < TRACE_STACK_SIZE)
                    {
                        stack[stack_size] = right;
                        stack_size++;
                        counters.max_stack_depth = max(counters.max_stack_depth, stack_size);
                        node_index = left;
                        continue;
                    }
                    else
                    {
                        counters.stack_overflow++;
                    }
                }
            }
        }
        if (stack_size == 0u) break;
        stack_size--;
        node_index = stack[stack_size];
    }
    return found;
}

// One deforming object's BLAS; its triangles are world-space float3 triples.
bool TraceDynamicBlas(
    DynamicObjectGPU object,
    float3 origin,
    float3 direction,
    float t_min,
    inout float t_max,
    inout WorldTraceCounters counters,
    out uint out_prim,
    out float3 out_normal)
{
    out_prim = 0xFFFFFFFFu;
    out_normal = float3(0.0, 0.0, 0.0);
    const uint node_count = object.node_count;
    if (node_count == 0u) return false;
    const uint leaf_count = (node_count + 1u) / 2u;

    uint stack[TRACE_STACK_SIZE];
    uint stack_size = 0u;
    uint node_index = 0u;
    bool found = false;

    while (true)
    {
        if (node_index >= node_count)
        {
            counters.invalid_refs++;
        }
        else
        {
            const BVHNodeGPU node = g_trace_dynamic_nodes[object.node_offset + node_index];
            float t_near;
            float t_far;
            if (AabbIntersect(origin, direction, node.bounds_min, node.bounds_max, t_min, t_max, t_near, t_far))
            {
                if ((node.child_or_leaf & BVH_LEAF_FLAG) != 0u)
                {
                    const uint leaf_index = node.child_or_leaf & 0x7FFFFFFFu;
                    const uint prim = leaf_index < leaf_count ? g_trace_dynamic_leaves[object.leaf_offset + leaf_index].prim : 0xFFFFFFFFu;
                    if (prim >= object.triangle_count)
                    {
                        counters.invalid_refs++;
                    }
                    else
                    {
                        const uint base = (object.vertex_base + prim * 3u) * 3u;
                        const float3 v0 = float3(g_trace_dynamic_vertices[base + 0u], g_trace_dynamic_vertices[base + 1u], g_trace_dynamic_vertices[base + 2u]);
                        const float3 v1 = float3(g_trace_dynamic_vertices[base + 3u], g_trace_dynamic_vertices[base + 4u], g_trace_dynamic_vertices[base + 5u]);
                        const float3 v2 = float3(g_trace_dynamic_vertices[base + 6u], g_trace_dynamic_vertices[base + 7u], g_trace_dynamic_vertices[base + 8u]);
                        float tri_t;
                        float3 tri_normal;
                        float2 tri_bary;
                        counters.triangle_tests++;
                        if (IntersectTriangle(origin, direction, v0, v1, v2, tri_t, tri_normal, tri_bary)
                            && tri_t >= t_min && tri_t < t_max)
                        {
                            t_max = tri_t;
                            out_prim = prim;
                            out_normal = tri_normal;
                            found = true;
                        }
                    }
                }
                else
                {
                    const uint left = node.child_or_leaf;
                    const uint right = node.sibling_or_right;
                    if (left >= node_count || right >= node_count || left == right)
                    {
                        counters.invalid_refs++;
                    }
                    else if (stack_size < TRACE_STACK_SIZE)
                    {
                        stack[stack_size] = right;
                        stack_size++;
                        counters.max_stack_depth = max(counters.max_stack_depth, stack_size);
                        node_index = left;
                        continue;
                    }
                    else
                    {
                        counters.stack_overflow++;
                    }
                }
            }
        }
        if (stack_size == 0u) break;
        stack_size--;
        node_index = stack[stack_size];
    }
    return found;
}

WorldTraceHit TraceWorldRay(
    float3 origin,
    float3 direction,
    float t_min,
    float t_max,
    bool any_hit,
    WorldCameraView view,
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

    uint tlas_leaf_count = 0u;
    uint tlas_stride = 0u;
    g_trace_tlas_leaves.GetDimensions(tlas_leaf_count, tlas_stride);
    if (tlas_leaf_count == 0u)
    {
        counters.misses = 1u;
        return hit;
    }
    const uint tlas_node_count = 2u * tlas_leaf_count - 1u;

    uint active_count = 0u;
    uint active_stride = 0u;
    g_trace_active.GetDimensions(active_count, active_stride);
    uint instance_count = 0u;
    uint instance_stride = 0u;
    g_trace_instances.GetDimensions(instance_count, instance_stride);
    uint mesh_count = 0u;
    uint mesh_stride = 0u;
    g_trace_meshes.GetDimensions(mesh_count, mesh_stride);

    float best_t = t_max;
    uint best_instance = 0xFFFFFFFFu;
    uint best_mesh = 0xFFFFFFFFu;
    uint best_prim = 0xFFFFFFFFu;
    float3 best_normal = float3(0.0, 0.0, 0.0);
    uint best_hidden = 0u;
    float best_facing = 0.0;
    uint best_material = 0u;
    float hidden_t = CAMERA_FADE_NEVER;  // nearest skipped hidden hit (view.hide)

    uint stack[TRACE_STACK_SIZE];
    uint stack_size = 0u;
    uint node_index = 0u;

    while (true)
    {
        if (node_index >= tlas_node_count)
        {
            counters.invalid_refs++;
        }
        else
        {
            const BVHNodeGPU node = g_trace_tlas_nodes[node_index];
            float t_near;
            float t_far;
            if (AabbIntersect(origin, direction, node.bounds_min, node.bounds_max, t_min, best_t, t_near, t_far))
            {
                if ((node.child_or_leaf & BVH_LEAF_FLAG) != 0u)
                {
                    const uint slot = node.child_or_leaf & 0x7FFFFFFFu;
                    if (slot >= tlas_leaf_count)
                    {
                        counters.invalid_refs++;
                    }
                    else
                    {
                        const BVHLeafGPU leaf = g_trace_tlas_leaves[slot];
                        const uint instance_slot = leaf.prim;
                        if (instance_slot >= active_count)
                        {
                            counters.invalid_refs++;
                        }
                        else
                        {
                            const uint resolved = g_trace_active[instance_slot];
                            if (resolved >= instance_count)
                            {
                                counters.invalid_refs++;
                            }
                            else
                            {
                                const WorldInstanceGPU instance = g_trace_instances[resolved];
                                const uint mesh_id = (uint)instance.header.x;
                                if (mesh_id >= mesh_count)
                                {
                                    counters.invalid_refs++;
                                }
                                else
                                {
                                    const float4 origin_h = float4(origin, 1.0);
                                    const float4 direction_h = float4(direction, 0.0);
                                    const float3 local_origin = float3(
                                        dot(origin_h, instance.inverse_world[0]),
                                        dot(origin_h, instance.inverse_world[1]),
                                        dot(origin_h, instance.inverse_world[2]));
                                    const float3 local_direction = float3(
                                        dot(direction_h, instance.inverse_world[0]),
                                        dot(direction_h, instance.inverse_world[1]),
                                        dot(direction_h, instance.inverse_world[2]));
                                    // Where the game camera shows this instance's surfaces.
                                    const uint visibility_flags = (uint)(instance.visibility.z + 0.5);
                                    float shown_min = 0.0;
                                    float shown_max = CAMERA_FADE_NEVER;
                                    if ((visibility_flags & INSTANCE_CAMERA_VISIBLE) == 0u)
                                    {
                                        shown_min = CAMERA_FADE_NEVER;
                                        shown_max = -1.0;
                                    }
                                    else if ((visibility_flags & INSTANCE_NEAR_FADE) != 0u)
                                    {
                                        CameraFadeInterval(instance.visibility.x, instance.visibility.y, view.floor_value, shown_min, shown_max);
                                    }
                                    float local_best_t = best_t;
                                    uint local_prim = 0xFFFFFFFFu;
                                    float3 local_normal = float3(0.0, 0.0, 0.0);
                                    uint local_hidden = 0u;
                                    if (TraceBlas(mesh_id, (uint)instance.header.z, local_origin, local_direction, t_min, local_best_t,
                                                  shown_min, shown_max, view.hide, hidden_t,
                                                  counters, local_prim, local_normal, local_hidden))
                                    {
                                        best_t = local_best_t;
                                        best_instance = resolved;
                                        best_mesh = mesh_id;
                                        best_prim = local_prim;
                                        best_hidden = local_hidden;
                                        best_facing = dot(local_normal, local_direction);
                                        best_material = (uint)instance.header.z;
                                        const float4 normal_h = float4(local_normal, 0.0);
                                        best_normal = normalize(float3(
                                            dot(normal_h, instance.world[0]),
                                            dot(normal_h, instance.world[1]),
                                            dot(normal_h, instance.world[2])));
                                        if (any_hit)
                                        {
                                            counters.hits = 1u;
                                            counters.camera_hidden = (best_hidden != 0u || hidden_t < best_t) ? 1u : 0u;
                                            hit.t = best_t;
                                            hit.instance = best_instance;
                                            hit.mesh = best_mesh;
                                            hit.prim = best_prim;
                                            hit.position = origin + direction * best_t;
                                            hit.normal = best_normal;
                                            hit.hidden = best_hidden;
                                            hit.facing = best_facing;
                                            hit.material = best_material;
                                            return hit;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    const uint left = node.child_or_leaf;
                    const uint right = node.sibling_or_right;
                    if (left >= tlas_node_count || right >= tlas_node_count || left == right)
                    {
                        counters.invalid_refs++;
                    }
                    else if (stack_size < TRACE_STACK_SIZE)
                    {
                        stack[stack_size] = right;
                        stack_size++;
                        counters.max_stack_depth = max(counters.max_stack_depth, stack_size);
                        node_index = left;
                        continue;
                    }
                    else
                    {
                        counters.stack_overflow++;
                    }
                }
            }
        }
        if (stack_size == 0u) break;
        stack_size--;
        node_index = stack[stack_size];
    }

    // Deforming meshes of this frame.
    for (uint object_index = 0u; object_index < g_trace_dynamic_count; ++object_index)
    {
        const DynamicObjectGPU object = g_trace_dynamic_objects[object_index];
        if (object.node_count == 0u) continue;
        const BVHNodeGPU root = g_trace_dynamic_nodes[object.node_offset];
        float root_near;
        float root_far;
        if (!AabbIntersect(origin, direction, root.bounds_min, root.bounds_max, t_min, best_t, root_near, root_far)) continue;
        float object_t = best_t;
        uint object_prim = 0xFFFFFFFFu;
        float3 object_normal = float3(0.0, 0.0, 0.0);
        if (TraceDynamicBlas(object, origin, direction, t_min, object_t, counters, object_prim, object_normal))
        {
            best_t = object_t;
            best_instance = DYNAMIC_INSTANCE_FLAG | object_index;
            best_mesh = DYNAMIC_MESH_MARKER;
            best_prim = object_prim;
            best_normal = object_normal;
            best_hidden = 0u;
            best_facing = 0.0;
            best_material = 0u;
            if (any_hit) break;
        }
    }

    // A skipped hidden hit closer than the result (or than nothing) means the
    // game camera does not show the nearest BVH surface.
    counters.camera_hidden = (best_hidden != 0u || hidden_t < best_t) ? 1u : 0u;
    if (best_prim == 0xFFFFFFFFu)
    {
        counters.misses = 1u;
        return hit;
    }
    counters.hits = 1u;
    hit.t = best_t;
    hit.instance = best_instance;
    hit.mesh = best_mesh;
    hit.prim = best_prim;
    hit.position = origin + direction * best_t;
    hit.normal = best_normal;
    hit.hidden = best_hidden;
    hit.facing = best_facing;
    hit.material = best_material;
    return hit;
}

WorldTraceHit TraceWorldClosest(float3 origin, float3 direction, float t_min, float t_max, WorldCameraView view, out WorldTraceCounters counters)
{
    return TraceWorldRay(origin, direction, t_min, t_max, false, view, counters);
}

WorldTraceHit TraceWorldAny(float3 origin, float3 direction, float t_min, float t_max, WorldCameraView view, out WorldTraceCounters counters)
{
    return TraceWorldRay(origin, direction, t_min, t_max, true, view, counters);
}
