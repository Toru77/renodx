void PtEvaluate(uint3 id, RtxdiSurfaceResult surface, bool water_pass) {
  if (water_pass) PtCount(5u);
  g_rt12_dynamic_mask = g_dynamic_on != 0u ? RT12_MASK_DYNAMIC : 0u;
  WorldTraceCounters pt_counters;  // required out argument of TraceWorldRayDxr; discarded
  const WorldCameraView bounce_view = {true, 1.0, 0u, 0u};
  const SharcParameters sharc = PtSharcParameters();
  if (!water_pass) {
    uint rng = PtHash(id.x + id.y * g_size.x + PtHash(g_frame));
    float3 L = float3(0, 0, 0);
    for (uint sample = 0u; sample < g_samples; ++sample) {
      float3 p = surface.world_position;
      float3 n = surface.shading_normal;
      float3 T = float3(1, 1, 1);
      float bias = (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) != 0u ? g_char_bias : g_env_bias;
      for (uint b = 0u; b < g_max_bounces; ++b) {
        const float3 dir = PtCosineDirection(n, rng);
        const WorldTraceHit hit = TraceWorldRayDxr(p + n * bias, dir, 1e-3, g_ray_max, false, bounce_view, pt_counters);
        if (hit.prim == 0xFFFFFFFFu) { L += T * PtSky(dir); break; }
        const float3 nh = dot(hit.normal, dir) > 0.0 ? -hit.normal : hit.normal;
        const float3 albedo = PtHitMaterial(hit.position, nh).albedo;
        if (g_sharc_enabled != 0u) {
          const SharcHitData data = PtSharcHit(hit.position, nh, dir, albedo);
          const float voxel = HashGridGetVoxelSize(HashGridGetLevel(data.positionWorld, sharc.hashGridParameters), sharc.hashGridParameters);
          float3 cached;
          if (hit.t > voxel * 1.732050808 && SharcGetCachedRadiance(sharc, data, cached, false) && all(isfinite(cached))) {
            L += PtClampLuminance(T * max(cached, 0), g_gi_clamp);
            break;
          }
        }
        L += PtClampLuminance(T * albedo * PtHitLight(hit.position, nh, rng), g_gi_clamp);
        T *= albedo;
        if (g_roulette != 0u && b + 1u >= g_roulette_min_bounce) {
          const float pr = max(0.05, pow(saturate(max(T.x, max(T.y, T.z))), 0.25));
          if (PtRandom(rng) >= pr) break;
          T /= pr;
        }
        p = hit.position;
        n = nh;
        bias = g_env_bias;
      }
    }
    float3 A = L / float(g_samples);
    if (any(isnan(A)) || any(isinf(A))) A = float3(0, 0, 0);
    g_pt_output[id.xy] = float4(max(A, 0), 1);
  }
  if (g_refl_samples != 0u) {
    float4 refl = float4(0, 0, 0, 0);
    if (water_pass || (surface.flags & RTXDI_SURFACE_FLAG_CHARACTER) == 0u) {
      // Roughness as the lighting shader's reflection mip: deferred roughness x mrt1.y / 255 (lighting :1184-1185).
      const bool water_origin = water_pass || ((g_refl_flags & 8u) != 0u && (g_pt_normal.Load(int3(id.xy, 0)).w & 2u) != 0u);
      const float rough = water_origin ? saturate(g_water_roughness * g_refl_roughness_scale) : saturate(PtGBufferMaterial(id.xy).roughness * ((g_pt_mrt1.Load(int3(id.xy, 0)).y & 255u) * (1.0 / 255.0)) * g_refl_roughness_scale);
      const float alpha = max(rough * rough, 1e-4);
      const float3 n = surface.shading_normal;
      const float3 v = surface.view_direction;
      const float3 up = abs(n.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
      const float3 t = normalize(cross(up, n));
      const float3 bt = cross(n, t);
      const float3 wo = float3(dot(v, t), dot(v, bt), max(dot(v, n), 1e-4));
      uint refl_rng = PtHash(id.x + id.y * g_size.x + PtHash(g_frame ^ 0x5245464Cu));
      float3 sum = float3(0, 0, 0);
      float distance = 0.0;
      for (uint sample = 0u; sample < g_refl_samples; ++sample) {
        if (water_pass) PtCount(6u);
        const float3 l =PtSampleGgx(wo, alpha, float2(PtRandom(refl_rng), PtRandom(refl_rng)));
        float3 dir = normalize(t * l.x + bt * l.y + n * l.z);
        if (dot(dir, n) <= 0.0) dir = reflect(-v, n);
        const WorldTraceHit hit = TraceWorldRayDxr(surface.world_position + n * g_env_bias, dir, 1e-3, g_ray_max, false, bounce_view, pt_counters);
        if (hit.prim == 0xFFFFFFFFu) { sum += PtSky(dir); distance += g_ray_max; continue; }
        if (water_origin) {
          const float4 hit_clip = mul(g_pt_view_proj, float4(hit.position, 1.0));
          if (hit_clip.w > 1e-6) {
            const float2 huv = float2(hit_clip.x / hit_clip.w * 0.5 + 0.5, 0.5 - hit_clip.y / hit_clip.w * 0.5);
            if (all(huv >= 0.0) && all(huv < 1.0)
                && (g_pt_normal.Load(int3(min(uint2(huv * float2(g_size)), g_size - 1u), 0)).w & 2u) != 0u) {
              sum += PtSky(reflect(-v, n));
              distance += g_ray_max;
              continue;
            }
          }
        }
        const float3 nh = dot(hit.normal, dir) > 0.0 ? -hit.normal : hit.normal;
        const float3 albedo = PtHitMaterial(hit.position, nh).albedo;
        const bool use_sharc = (g_refl_flags & 2u) != 0u && g_sharc_enabled != 0u;
        const SharcHitData sharc_hit = PtSharcHit(hit.position, nh, dir, albedo);
        const float voxel = HashGridGetVoxelSize(HashGridGetLevel(sharc_hit.positionWorld, sharc.hashGridParameters), sharc.hashGridParameters);
        const float spread = min(1.0, 1.41421356 * alpha / sqrt(max(1.0 - alpha * alpha, 1e-6)));
        const bool footprint_ok = (g_refl_flags & 4u) == 0u || hit.t * spread > voxel * 1.732050808;
        float3 cached;
        if (use_sharc && footprint_ok && SharcGetCachedRadiance(sharc, sharc_hit, cached, false) && all(isfinite(cached))) {
          sum += g_refl_sample_clamp > 0.0 ? min(max(cached, 0), g_refl_sample_clamp) : max(cached, 0);
        } else {
          const float3 lit = albedo * (PtHitLight(hit.position, nh, refl_rng) + PtSky(nh));
          sum += g_refl_sample_clamp > 0.0 ? min(lit, g_refl_sample_clamp) : lit;
        }
        distance += hit.t;
      }
      sum /= float(g_refl_samples);
      if (any(isnan(sum)) || any(isinf(sum))) sum = float3(0, 0, 0);
      float3 rgb = max(sum, 0) * g_refl_intensity;
      if ((g_refl_flags & 1u) != 0u) rgb = min(rgb, 1.0);
      refl = float4(rgb, max(distance / float(g_refl_samples), 1e-3));
    }
    g_pt_reflection[id.xy] = refl;
  }
}
