// Experimental companion to the game's 0xD9289B6A motion producer.
// One mask texel corresponds to one packed motion texel. The game selects the
// minimum depth over a 3x3 neighborhood of 2x2 gathers before testing bit 3 of
// the selected full-resolution material flag. Keep the gather and tie ordering
// here so the mask can follow the producer at geometry boundaries.
Texture2D<float> t_linear_depth : register(t0);
Texture2D<float4> t_material_flags : register(t1);
RWTexture2D<float> u_object_mask : register(u0);

cbuffer MaskParams : register(b0) {
  uint2 g_input_size;
  uint2 g_motion_size;
};

groupshared float s_min_depth[100];
groupshared uint2 s_selected_position[100];

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID,
          uint3 group_id : SV_GroupID,
          uint3 group_thread_id : SV_GroupThreadID,
          uint group_index : SV_GroupIndex) {
  // GatherRed components are bottom-left, bottom-right, top-right, top-left.
  // The original shader's firstbitlow picks the first matching component.
  for (uint index = group_index; index < 100; index += 64) {
    const int2 tile = int2(index % 10, index / 10);
    const int2 origin = (int2(group_id.xy) * 8 + tile - 1) * 2;
    const int2 max_position = int2(g_input_size) - 1;
    const int2 positions[4] = {
        clamp(origin + int2(0, 1), int2(0, 0), max_position),
        clamp(origin + int2(1, 1), int2(0, 0), max_position),
        clamp(origin + int2(1, 0), int2(0, 0), max_position),
        clamp(origin, int2(0, 0), max_position),
    };
    float nearest = t_linear_depth.Load(int3(positions[0], 0));
    int2 selected = positions[0];
    [unroll] for (uint i = 1; i < 4; ++i) {
      const float depth = t_linear_depth.Load(int3(positions[i], 0));
      if (depth < nearest) {
        nearest = depth;
        selected = positions[i];
      }
    }
    s_min_depth[index] = nearest;
    s_selected_position[index] = uint2(selected);
  }
  GroupMemoryBarrierWithGroupSync();

  if (any(dispatch_id.xy >= g_motion_size)) return;
  const uint2 center = group_thread_id.xy + 1;
  const int2 offsets[9] = {
      int2(0, 0), int2(-1, -1), int2(0, -1), int2(1, -1),
      int2(-1, 0), int2(1, 0), int2(-1, 1), int2(0, 1), int2(1, 1),
  };
  uint selected_index = center.y * 10 + center.x;
  float nearest = s_min_depth[selected_index];
  [unroll] for (uint i = 1; i < 9; ++i) {
    const uint2 tile = uint2(int2(center) + offsets[i]);
    const uint index = tile.y * 10 + tile.x;
    if (s_min_depth[index] < nearest) {
      nearest = s_min_depth[index];
      selected_index = index;
    }
  }
  const uint2 selected = s_selected_position[selected_index];
  const float alpha = t_material_flags.Load(int3(selected, 0)).a;
  const uint flags = uint(alpha * 255.0f);
  u_object_mask[dispatch_id.xy] = (nearest < 1.0f && (flags & 8u) != 0u) ? 1.0f : 0.0f;
}
