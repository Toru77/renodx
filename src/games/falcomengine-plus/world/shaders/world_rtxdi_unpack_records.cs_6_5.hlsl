// Restore the shared RGBA32_UINT carriers to private raw buffers.
Texture2D<uint4> g_scene : register(t0);
Texture2D<uint4> g_lights : register(t1);
Texture2D<uint4> g_tiles : register(t2);
Texture2D<uint4> g_deferred : register(t3);
RWByteAddressBuffer g_out_scene : register(u0);
RWByteAddressBuffer g_out_lights : register(u1);
RWByteAddressBuffer g_out_tiles : register(u2);
RWByteAddressBuffer g_out_deferred : register(u3);
cbuffer cb_unpack : register(b0) { uint4 g_counts; };
[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  uint t = id.x;
  uint2 xy = uint2(t % 4096u, t / 4096u);
  if (t < 96u) g_out_scene.Store4(t * 16u, g_scene.Load(int3(t, 0, 0)));
  if (t < g_counts.x * 7u) g_out_lights.Store4(t * 16u, g_lights.Load(int3(xy, 0)));
  if (t < g_counts.y * 36u) g_out_tiles.Store4(t * 16u, g_tiles.Load(int3(xy, 0)));
  if (t < g_counts.z * 9u) g_out_deferred.Store4(t * 16u, g_deferred.Load(int3(xy, 0)));
}
