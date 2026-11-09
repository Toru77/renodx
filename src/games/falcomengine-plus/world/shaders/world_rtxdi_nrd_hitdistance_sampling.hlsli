#pragma once

uint NrdHitDistanceHash(uint value) {
  value ^= value >> 16u;
  value *= 0x7FEB352Du;
  value ^= value >> 15u;
  value *= 0x846CA68Bu;
  value ^= value >> 16u;
  return value;
}

float NrdHitDistanceRandom(inout uint state) {
  state = NrdHitDistanceHash(state + 0x9E3779B9u);
  return (state >> 8u) * (1.0 / 16777216.0);
}

float3 NrdCosineHemisphere(float3 normal, inout uint state) {
  const float u1 = NrdHitDistanceRandom(state);
  const float u2 = NrdHitDistanceRandom(state);
  const float radius = sqrt(u1);
  const float angle = 6.28318530718 * u2;
  const float3 helper = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);
  const float3 tangent = normalize(cross(helper, normal));
  const float3 bitangent = cross(normal, tangent);
  return normalize(tangent * (radius * cos(angle)) + bitangent * (radius * sin(angle))
                   + normal * sqrt(max(0.0, 1.0 - u1)));
}
