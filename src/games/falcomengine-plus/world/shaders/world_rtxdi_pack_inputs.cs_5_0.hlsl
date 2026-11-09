// Raw pixel inputs and cb_scene rows for the RTXDI input bridge.
Texture2D<float4> g_depth : register(t0);
Texture2D<uint4> g_normal : register(t1);
Texture2D<float4> g_color : register(t2);
Texture2D<uint4> g_mrt1 : register(t3);
Texture2D<uint2> g_mrt2 : register(t4);
Texture2D<float4> g_motion : register(t5);
RWTexture2D<float> g_out_depth : register(u0);
RWTexture2D<uint4> g_out_normal : register(u1);
RWTexture2D<float4> g_out_color : register(u2);
RWTexture2D<uint4> g_out_mrt1 : register(u3);
RWTexture2D<uint4> g_out_mrt2 : register(u4);
RWTexture2D<uint4> g_out_scene : register(u5);
RWTexture2D<float4> g_out_motion : register(u6);
cbuffer cb_scene : register(b0) { uint4 g_scene[96]; };
cbuffer cb_pack : register(b12) { uint4 g_dimensions; };
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (id.x == 0u && id.y == 0u) {
    [unroll] for (uint row = 0u; row < 96u; ++row) g_out_scene[uint2(row, 0u)] = g_scene[row];
  }
  if (id.x >= g_dimensions.x || id.y >= g_dimensions.y) return;
  g_out_depth[id.xy] = g_depth.Load(int3(id.xy, 0)).x;
  g_out_normal[id.xy] = g_normal.Load(int3(id.xy, 0));
  if (g_dimensions.w != 0u) g_out_motion[id.xy] = float4(g_motion.Load(int3(id.xy, 0)).xy, 0.0, 0.0);
  if (g_dimensions.z != 0u) {
    g_out_color[id.xy] = g_color.Load(int3(id.xy, 0));
    g_out_mrt1[id.xy] = g_mrt1.Load(int3(id.xy, 0));
    g_out_mrt2[id.xy] = uint4(g_mrt2.Load(int3(id.xy, 0)), 0u, 0u);
  }
}
