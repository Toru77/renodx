#include "world_rt_lamps.hlsli"
#include "../rtxdi/rtxdi_di_layout.h"
#include "../rtxdi/rtxdi_diag_layout.h"

StructuredBuffer<LightParam> g_current_lights : register(t0);
StructuredBuffer<uint> g_current_types : register(t1);
StructuredBuffer<LightParam> g_previous_lights : register(t2);
StructuredBuffer<RtxdiLightTableEntry> g_previous_table : register(t3);
StructuredBuffer<RtxdiLightTableEntry> g_current_table : register(t4);
RWStructuredBuffer<RtxdiLightTableEntry> g_light_table : register(u0);
RWStructuredBuffer<uint> g_previous_to_current : register(u1);
RWStructuredBuffer<uint> g_current_to_previous : register(u2);
RWStructuredBuffer<uint> g_diagnostics : register(u3);

cbuffer LightTableParams : register(b0) {
  uint g_current_count;
  uint g_previous_count;
  uint g_history_valid;
  uint g_mode;
};

bool LightTableQuantize(float value, float scale, out int quantized) {
  quantized = 0;
  if (!isfinite(value)) return false;
  const float scaled = value * scale;
  if (!isfinite(scaled)) return false;
  const float rounded = floor(scaled + 0.5);
  if (!isfinite(rounded) || rounded < -2147483648.0 || rounded >= 2147483648.0) return false;
  quantized = (int)rounded;
  return true;
}

bool LightTableQuantizeIdentity(LightParam light, out int values[7]) {
  bool valid = true;
  valid = LightTableQuantize(light.pos.x, 256.0, values[0]) && valid;
  valid = LightTableQuantize(light.pos.y, 256.0, values[1]) && valid;
  valid = LightTableQuantize(light.pos.z, 256.0, values[2]) && valid;
  valid = LightTableQuantize(light.radius, 256.0, values[3]) && valid;
  valid = LightTableQuantize(light.color.x, 1024.0, values[4]) && valid;
  valid = LightTableQuantize(light.color.y, 1024.0, values[5]) && valid;
  valid = LightTableQuantize(light.color.z, 1024.0, values[6]) && valid;
  return valid;
}

void umulExtended(uint left, uint right, out uint low, out uint high) {
  const uint low_product = (left & 0xffffu) * right;
  const uint high_product = (left >> 16u) * right;
  const uint shifted_high_product = high_product << 16u;
  low = low_product + shifted_high_product;
  high = (high_product >> 16u) + (low < shifted_high_product ? 1u : 0u);
}

uint2 LightTableHashWord(uint2 hash, uint word) {
  [unroll]
  for (uint byte_index = 0u; byte_index < 4u; ++byte_index) {
    const uint hash_x_pre = hash.x ^ ((word >> (byte_index * 8u)) & 0xffu);
    uint low;
    uint carry;
    umulExtended(hash_x_pre, 0x1b3u, low, carry);
    hash = uint2(low, carry + hash.y * 0x1b3u + hash_x_pre * 0x100u);
  }
  return hash;
}

uint2 LightTableHash(int values[7], uint type) {
  uint2 hash = uint2(0x84222325u, 0xcbf29ce4u);
  [unroll]
  for (uint i = 0u; i < 7u; ++i) hash = LightTableHashWord(hash, asuint(values[i]));
  return LightTableHashWord(hash, type);
}

bool LightTableIdentityEqual(LightParam a, LightParam b, uint a_type, uint b_type) {
  int a_values[7];
  int b_values[7];
  if (a_type != b_type || !LightTableQuantizeIdentity(a, a_values) || !LightTableQuantizeIdentity(b, b_values)) return false;
  [unroll]
  for (uint i = 0u; i < 7u; ++i) {
    if (a_values[i] != b_values[i]) return false;
  }
  return true;
}

bool LightTableValidForEvaluation(LightParam light, uint type);

bool LightTableCompatible(LightParam a, LightParam b, uint type) {
  if (!LightTableValidForEvaluation(a, type) || !LightTableValidForEvaluation(b, type)) return false;
  if (asuint(a.radiusInv) != asuint(b.radiusInv) || asuint(a.attenuation) != asuint(b.attenuation)) return false;
  return type != RTXDI_LIGHT_TABLE_TYPE_SPOT
      || (all(asuint(a.vec.xyz) == asuint(b.vec.xyz))
          && asuint(a.spotAngleInv) == asuint(b.spotAngleInv)
          && asuint(a.attenuationAngle) == asuint(b.attenuationAngle));
}

bool LightTableValidForEvaluation(LightParam light, uint type) {
  if ((type != RTXDI_LIGHT_TABLE_TYPE_POINT && type != RTXDI_LIGHT_TABLE_TYPE_SPOT)
      || !all(isfinite(float4(light.pos, light.radius))) || !all(isfinite(float4(light.color, light.radiusInv)))
      || !isfinite(light.attenuation) || light.radius <= 0.0 || light.radiusInv <= 0.0 || light.attenuation < 0.0) return false;
  if (type != RTXDI_LIGHT_TABLE_TYPE_SPOT) return true;
  return all(isfinite(float4(light.vec.xyz, light.spotAngleInv))) && isfinite(light.attenuationAngle)
      && light.spotAngleInv > 0.0 && light.attenuationAngle >= 0.0 && dot(light.vec.xyz, light.vec.xyz) > 1e-12;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  const uint index = dispatch_id.x;
  if (g_mode == 0u) {
    if (index >= g_current_count) return;
    const LightParam light = g_current_lights[index];
    const uint type = g_current_types[index];
    RtxdiLightTableEntry entry = (RtxdiLightTableEntry)0;
    if (type == 0u) {
      g_light_table[index] = entry;
      InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_TABLE_WRITTEN], 1u);
      return;
    }
    entry.position = light.pos;
    entry.radius = light.radius;
    entry.type = type;
    const float3 nonnegative_color = max(light.color, 0.0);
    const float power = dot(nonnegative_color, float3(0.2126, 0.7152, 0.0722)) * light.radius * light.radius;
    entry.power = isfinite(power) ? power : 0.0;
    int values[7];
    if (LightTableQuantizeIdentity(light, values) && isfinite(power) && light.radius > 0.0
        && isfinite(light.radiusInv) && light.radiusInv > 0.0) {
      const uint2 id = LightTableHash(values, type);
      entry.stable_id_lo = id.x;
      entry.stable_id_hi = id.y;
    } else {
      entry.power = 0.0;
      InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_INVALID_ID], 1u);
    }
    g_light_table[index] = entry;
    InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_TABLE_WRITTEN], 1u);
    return;
  }

  if (index < g_current_count && g_current_table[index].type != 0u) {
    InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_CURRENT_UNMATCHED], 1u);
  }
  if (index >= g_previous_count) return;
  const RtxdiLightTableEntry previous_entry = g_previous_table[index];
  if (previous_entry.type == 0u) return;
  if (g_history_valid == 0u) {
    InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_ID_UNMATCHED], 1u);
    return;
  }
  {
    const LightParam previous_light = g_previous_lights[index];
    int previous_values[7];
    if (!LightTableQuantizeIdentity(previous_light, previous_values)) {
      InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_ID_UNMATCHED], 1u);
    } else {
      uint occurrence = 0u;
      uint candidate_index = RTXDI_LIGHT_TABLE_INVALID_INDEX;
      uint equal_candidates = 0u;
      bool selected_compatible = false;
      [loop]
      for (uint previous_scan = 0u; previous_scan <= index; ++previous_scan) {
        const RtxdiLightTableEntry scanned = g_previous_table[previous_scan];
        if (scanned.stable_id_lo == previous_entry.stable_id_lo && scanned.stable_id_hi == previous_entry.stable_id_hi
            && scanned.type == previous_entry.type
            && LightTableIdentityEqual(previous_light, g_previous_lights[previous_scan], previous_entry.type, scanned.type)) {
          if (previous_scan < index) ++occurrence;
        }
      }
      [loop]
      for (uint current_scan = 0u; current_scan < g_current_count; ++current_scan) {
        const RtxdiLightTableEntry current_entry = g_current_table[current_scan];
        if (current_entry.stable_id_lo != previous_entry.stable_id_lo || current_entry.stable_id_hi != previous_entry.stable_id_hi
            || current_entry.type != previous_entry.type) continue;
        const LightParam current_light = g_current_lights[current_scan];
        if (!LightTableIdentityEqual(previous_light, current_light, previous_entry.type, current_entry.type)) {
          InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_HASH_COLLISIONS], 1u);
          continue;
        }
        if (equal_candidates == occurrence) {
          candidate_index = current_scan;
          selected_compatible = LightTableCompatible(previous_light, current_light, previous_entry.type);
        }
        ++equal_candidates;
      }
      if (equal_candidates > 1u) InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_ID_DUPLICATES], 1u);
      if (candidate_index != RTXDI_LIGHT_TABLE_INVALID_INDEX && selected_compatible) {
        g_previous_to_current[index] = candidate_index;
        g_current_to_previous[candidate_index] = index;
        InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_ID_MATCHED], 1u);
        InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_REMAP_HITS], 1u);
        InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_CURRENT_UNMATCHED], RTXDI_LIGHT_TABLE_INVALID_INDEX);
      } else {
        if (candidate_index != RTXDI_LIGHT_TABLE_INVALID_INDEX) {
          InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_INCOMPATIBLE], 1u);
        }
        InterlockedAdd(g_diagnostics[RTXDI_DIAG_LIGHT_ID_UNMATCHED], 1u);
      }
    }
  }
}
