// Copy structured records bit-for-bit into RGBA32_UINT shared carriers.
struct LightRecord { uint4 words[7]; };
struct TileRecord { uint4 words[36]; };
struct DeferredRecord { uint4 words[9]; };
StructuredBuffer<LightRecord> g_lights : register(t0);
StructuredBuffer<TileRecord> g_tiles : register(t1);
StructuredBuffer<DeferredRecord> g_deferred : register(t2);
RWTexture2D<uint4> g_out_lights : register(u0);
RWTexture2D<uint4> g_out_tiles : register(u1);
RWTexture2D<uint4> g_out_deferred : register(u2);
cbuffer cb_pack : register(b12) { uint4 g_counts; };
[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  uint t = id.x;
  uint2 xy = uint2(t % 4096u, t / 4096u);
  if (t < g_counts.x * 7u) g_out_lights[xy] = g_lights[t / 7u].words[t % 7u];
  if (t < g_counts.y * 36u) g_out_tiles[xy] = g_tiles[t / 36u].words[t % 36u];
  if (t < g_counts.z * 9u) g_out_deferred[xy] = g_deferred[t / 9u].words[t % 9u];
}
