// G2 GPU geometry cache: index range pass (FXC, D3D11). Transcription of falcom_world::bvh::GcRangeJob in
// bvh/gpu_capture.hpp; the two must agree. One dispatch per job, one thread per index. Results per job (c1.x):
// x = InterlockedMax of ~abs (min = ~x), y = InterlockedMax of abs (max), z = count of invalid indices.

ByteAddressBuffer g_gc_staging : register(t0);
RWStructuredBuffer<uint4> g_gc_results : register(u1);

cbuffer cb_gc : register(b0) {
  uint4 g_gc_c0;  // index_byte, index_size, index_count, asuint(base_vertex)
  uint4 g_gc_c1;  // job
  uint4 g_gc_c2;
  uint4 g_gc_c3;
};

uint LoadU32(uint address) {
  const uint aligned = address & ~3u;
  const uint shift = (address & 3u) * 8u;
  const uint w0 = g_gc_staging.Load(aligned);  // out of range reads 0
  const uint w1 = g_gc_staging.Load(aligned + 4u);
  return shift == 0u ? w0 : ((w0 >> shift) | (w1 << (32u - shift)));
}

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  const uint i = id.y * (65535u * 64u) + id.x;
  const uint index_count = g_gc_c0.z;
  if (i >= index_count) return;
  const uint job = g_gc_c1.x;
  const uint index_size = g_gc_c0.y;
  const int base_vertex = asint(g_gc_c0.w);
  uint raw = LoadU32(g_gc_c0.x + i * index_size);
  if (index_size == 2u) raw &= 0xFFFFu;
  if (raw >= 0x80000000u) {
    InterlockedAdd(g_gc_results[job].z, 1u);
    return;
  }
  // The absolute index is raw + base_vertex (exact in 32 bits: a negative base is subtracted from raw).
  uint abs_index = 0u;
  if (base_vertex >= 0) {
    abs_index = raw + (uint)base_vertex;
  } else {
    const uint below = (uint)(-base_vertex);
    if (raw < below) {
      InterlockedAdd(g_gc_results[job].z, 1u);
      return;
    }
    abs_index = raw - below;
  }
  InterlockedMax(g_gc_results[job].x, ~abs_index);
  InterlockedMax(g_gc_results[job].y, abs_index);
}
