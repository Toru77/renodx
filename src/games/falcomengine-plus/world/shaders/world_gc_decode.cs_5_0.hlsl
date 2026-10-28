// G2 GPU geometry cache: decode pass (FXC, D3D11). Transcription of falcom_world::bvh::GcDecodeJob in
// bvh/gpu_capture.hpp; the two must agree (the CPU HalfToFloat subnormal branch is copied on purpose). One dispatch per
// job, one thread per texel slot t < max(V, ceil(I/4)). Texel n goes to (n % 4096, n / 4096).

#define GC_FORMAT_NONE 0u
#define GC_FORMAT_F32X2 1u
#define GC_FORMAT_F32X3 2u
#define GC_FORMAT_F32X4 3u
#define GC_FORMAT_F16X2 4u
#define GC_FORMAT_F16X4 5u
#define GC_NO_UV 0xFFFFFFFFu

ByteAddressBuffer g_gc_staging : register(t0);
RWTexture2D<uint4> g_gc_carrier : register(u0);

cbuffer cb_gc : register(b0) {
  uint4 g_gc_c0;  // index_byte, index_size, index_count, asuint(base_vertex)
  uint4 g_gc_c1;  // vertex_byte, stride, pos_offset, pos_format
  uint4 g_gc_c2;  // uv_byte, uv_stride, uv_offset, uv_format
  uint4 g_gc_c3;  // vertex_count, min_vertex, vtx_texel, idx_texel
};

uint LoadU32(uint address) {
  const uint aligned = address & ~3u;
  const uint shift = (address & 3u) * 8u;
  const uint w0 = g_gc_staging.Load(aligned);  // out of range reads 0
  const uint w1 = g_gc_staging.Load(aligned + 4u);
  return shift == 0u ? w0 : ((w0 >> shift) | (w1 << (32u - shift)));
}

float HalfToFloat(uint bits) {
  const uint sign = (bits & 0x8000u) << 16u;
  uint exp = (bits >> 10u) & 0x1Fu;
  uint mantissa = bits & 0x3FFu;
  uint f32 = 0u;
  if (exp == 0u) {
    if (mantissa == 0u) {
      f32 = sign;
    } else {
      exp = 1u;
      [loop] while ((mantissa & 0x400u) == 0u) {
        mantissa <<= 1u;
        exp++;
      }
      mantissa &= 0x3FFu;
      f32 = sign | ((exp + 112u) << 23u) | (mantissa << 13u);
    }
  } else if (exp == 31u) {
    f32 = sign | 0x7F800000u | (mantissa << 13u);
  } else {
    f32 = sign | ((exp + 112u) << 23u) | (mantissa << 13u);
  }
  return asfloat(f32);
}

// {0, 0, 0, 1}, then the format's components at `address` (up to four).
float4 DecodeAttribute(uint address, uint code) {
  const bool half_float = code == GC_FORMAT_F16X2 || code == GC_FORMAT_F16X4;
  const uint count = (code == GC_FORMAT_F32X2 || code == GC_FORMAT_F16X2) ? 2u : (code == GC_FORMAT_F32X3 ? 3u : 4u);
  float4 value = float4(0.f, 0.f, 0.f, 1.f);
  if (half_float) {
    value.x = HalfToFloat(LoadU32(address) & 0xFFFFu);
    if (count > 1u) value.y = HalfToFloat(LoadU32(address + 2u) & 0xFFFFu);
    if (count > 2u) value.z = HalfToFloat(LoadU32(address + 4u) & 0xFFFFu);
    if (count > 3u) value.w = HalfToFloat(LoadU32(address + 6u) & 0xFFFFu);
  } else {
    value.x = asfloat(LoadU32(address));
    if (count > 1u) value.y = asfloat(LoadU32(address + 4u));
    if (count > 2u) value.z = asfloat(LoadU32(address + 8u));
    if (count > 3u) value.w = asfloat(LoadU32(address + 12u));
  }
  return value;
}

// The relative index i of the job (raw + base_vertex - min_vertex); 0 past the end or outside the vertices.
uint RelativeIndex(uint i) {
  const uint index_count = g_gc_c0.z;
  if (i >= index_count) return 0u;
  uint raw = LoadU32(g_gc_c0.x + i * g_gc_c0.y);
  if (g_gc_c0.y == 2u) raw &= 0xFFFFu;
  const uint relative = raw + g_gc_c0.w - g_gc_c3.y;
  return relative < g_gc_c3.x ? relative : 0u;
}

void StoreTexel(uint texel, uint4 value) {
  g_gc_carrier[uint2(texel % 4096u, texel / 4096u)] = value;
}

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  const uint t = id.y * (65535u * 64u) + id.x;
  const uint vertex_byte = g_gc_c1.x;
  const uint stride = g_gc_c1.y;
  const uint pos_offset = g_gc_c1.z;
  const uint pos_format = g_gc_c1.w;
  const uint uv_byte = g_gc_c2.x;
  const uint uv_stride = g_gc_c2.y;
  const uint uv_offset = g_gc_c2.z;
  const uint uv_format = g_gc_c2.w;
  const uint vertex_count = g_gc_c3.x;
  const uint vtx_texel = g_gc_c3.z;
  const uint idx_texel = g_gc_c3.w;
  const uint index_count = g_gc_c0.z;
  const uint index_texels = (index_count + 3u) / 4u;
  const uint uv_texel = idx_texel + index_texels;
  const bool has_uv = uv_offset != GC_NO_UV;
  if (t >= max(vertex_count, index_texels)) return;

  if (t < vertex_count) {
    const float4 position = DecodeAttribute(vertex_byte + t * stride + pos_offset, pos_format);
    StoreTexel(vtx_texel + t, asuint(float4(position.xyz, 1.f)));
  }
  if (t < index_texels) {
    const uint4 quad = uint4(RelativeIndex(4u * t), RelativeIndex(4u * t + 1u), RelativeIndex(4u * t + 2u), RelativeIndex(4u * t + 3u));
    StoreTexel(idx_texel + t, quad);
  }
  if (has_uv && t < (vertex_count + 1u) / 2u) {
    uint4 pair = uint4(0u, 0u, 0u, 0u);
    if (2u * t < vertex_count) {
      const float4 first = DecodeAttribute(uv_byte + (2u * t) * uv_stride + uv_offset, uv_format);
      pair.xy = asuint(first.xy);
    }
    if (2u * t + 1u < vertex_count) {
      const float4 second = DecodeAttribute(uv_byte + (2u * t + 1u) * uv_stride + uv_offset, uv_format);
      pair.zw = asuint(second.xy);
    }
    StoreTexel(uv_texel + t, pair);
  }
}
