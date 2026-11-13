#include "world_regir_shared.hlsli"
#include "world_rtxdi_surface.hlsli"

Texture2D<float> g_depth : register(t0);
Texture2D<uint4> g_normal : register(t1);
Texture2D<uint4> g_scene : register(t2);
RWStructuredBuffer<PtReGirHeader> g_header : register(u0);
RWStructuredBuffer<PtReGirCell> g_cells : register(u1);
cbuffer ReGirRegisterParams : register(b0) {
  uint2 g_size;
};

[numthreads(1, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  if (any(dispatch_id != 0u)) return;
  uint header_count, header_stride, cell_capacity, cell_stride;
  g_header.GetDimensions(header_count, header_stride);
  g_cells.GetDimensions(cell_capacity, cell_stride);
  if (header_count == 0u) return;
  PtReGirHeader header = (PtReGirHeader)0;
  g_header[0] = header;
  uint depth_width, depth_height, normal_width, normal_height, scene_width, scene_height;
  g_depth.GetDimensions(depth_width, depth_height);
  g_normal.GetDimensions(normal_width, normal_height);
  g_scene.GetDimensions(scene_width, scene_height);
  if (any(g_size == 0u) || g_size.x > depth_width || g_size.y > depth_height
      || g_size.x > normal_width || g_size.y > normal_height
      || scene_width <= RTXDI_SURFACE_RESOLUTION_SCALING_ROW || scene_height == 0u) return;
  uint4 scene_rows[RTXDI_SURFACE_SCENE_ROW_COUNT];
  [unroll]
  for (uint row = 0u; row < 8u; ++row) scene_rows[row] = g_scene.Load(int3(row, 0, 0));
  [unroll]
  for (uint row = 0u; row < 4u; ++row) {
    scene_rows[RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row] =
        g_scene.Load(int3(RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 0, 0));
  }
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW - 1u] = g_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW - 1u, 0, 0));
  scene_rows[RTXDI_SURFACE_SUN_RESERVED_ROW] = g_scene.Load(int3(RTXDI_SURFACE_SUN_RESERVED_ROW, 0, 0));
  scene_rows[RTXDI_SURFACE_RESOLUTION_SCALING_ROW] = g_scene.Load(int3(RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0, 0));
  const uint64_t pixels = uint64_t(g_size.x) * uint64_t(g_size.y);
  const uint sample_count = uint(min(pixels, uint64_t(PT_REGIR_MAX_POSITIONS)));
  for (uint sample_index = 0u; sample_index < sample_count; ++sample_index) {
    const uint64_t linear_index = (pixels / sample_count) * sample_index + ((pixels % sample_count) * sample_index) / sample_count;
    const uint2 pixel = uint2(uint(linear_index % g_size.x), uint(linear_index / g_size.x));
    const RtxdiSurfaceResult surface = RtxdiReconstructSurface(g_depth.Load(int3(pixel, 0)), g_normal.Load(int3(pixel, 0)), pixel, g_size, scene_rows);
    int3 key;
    if ((surface.flags & RTXDI_SURFACE_FLAG_VALID) == 0u || !PtReGirCellKey(surface.world_position, key)) {
      ++header.rejected_position_count;
      continue;
    }
    uint cell_index = 0u;
    while (cell_index < header.cell_count && any(g_cells[cell_index].key != key)) ++cell_index;
    if (cell_index == header.cell_count) {
      if (header.cell_count >= min(cell_capacity, PT_REGIR_MAX_CELLS)) {
        ++header.overflow_position_count;
      } else {
        PtReGirCell cell = (PtReGirCell)0;
        cell.key = key;
        g_cells[header.cell_count++] = cell;
      }
    }
  }
  header.valid = 1u;
  g_header[0] = header;
}