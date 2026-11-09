bool NrdHitDistanceCoverageEnd(float3 origin, float3 direction, float max_distance, float3 coverage_min,
                               float coverage_size, out float coverage_end) {
  coverage_end = 0.0;
  if (!all(isfinite(origin)) || !all(isfinite(direction)) || !all(isfinite(coverage_min))
      || !isfinite(coverage_size) || coverage_size <= 0.0 || !isfinite(max_distance) || max_distance <= 0.0
      || any(origin < coverage_min) || any(origin > coverage_min + coverage_size)) return false;
  float exit_distance = max_distance;
  for (uint axis = 0u; axis < 3u; ++axis) {
    const float component = direction[axis];
    if (component == 0.0) continue;
    if (abs(component) <= 1.0e-8) return false;
    const float boundary = component > 0.0 ? coverage_min[axis] + coverage_size : coverage_min[axis];
    exit_distance = min(exit_distance, (boundary - origin[axis]) / component);
  }
  coverage_end = max(exit_distance, 0.0);
  return isfinite(coverage_end) && coverage_end > 0.0;
}

bool NrdHitDistanceFullyCovered(float coverage_end, float max_distance) {
  return isfinite(coverage_end) && isfinite(max_distance) && max_distance > 0.0
         && coverage_end >= max_distance;
}
