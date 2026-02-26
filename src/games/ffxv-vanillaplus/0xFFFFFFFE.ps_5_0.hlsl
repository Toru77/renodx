SamplerState samLinear_s : register(s0);
Texture2D<float4> sourceTexture : register(t0);
Texture2D<float4> albedoTexture : register(t1);
Texture2D<float4> normalTexture : register(t2);
Texture2D<float4> depthTexture : register(t3);
Texture2D<float4> aoTexture : register(t4);
Texture2D<float4> shadowTexture : register(t5);
Texture2D<float4> specularCandidateTexture : register(t6);
Texture2D<float4> ssrCandidateTexture : register(t7);
Texture2D<float4> envCandidateTexture : register(t8);

static const float3 LUMA_WEIGHTS = float3(0.299, 0.587, 0.114);

#if ((__SHADER_TARGET_MAJOR == 5 && __SHADER_TARGET_MINOR >= 1) || __SHADER_TARGET_MAJOR >= 6)
cbuffer ambient_gi_composite_settings : register(b13, space0) {
#else
cbuffer ambient_gi_composite_settings : register(b13) {
#endif
  // x=enabled, y=strength(0-100), z=tint saturation(0-100), w=shadow influence(0-100)
  float4 ambientGiParams0;
  // x=color boost(0-400), y=debug mode(0-14), z=debug scale(0-8), w=bleed radius(0-4)
  float4 ambientGiParams1;
  // x=tint source A id(0-7), y=tint source B id(0-7), z=tint blend(0-100), w=reserved
  float4 ambientGiParams2;
}

float3 SampleCandidate(uint source_id, float2 uv) {
  switch (source_id) {
    case 0u:
      return saturate(albedoTexture.SampleLevel(samLinear_s, uv, 0).rgb);
    case 1u:
      return saturate(specularCandidateTexture.SampleLevel(samLinear_s, uv, 0).rgb);
    case 2u:
      return saturate(sourceTexture.SampleLevel(samLinear_s, uv, 0).rgb);
    case 3u:
      return saturate(ssrCandidateTexture.SampleLevel(samLinear_s, uv, 0).rgb);
    case 4u: {
      float v = saturate(aoTexture.SampleLevel(samLinear_s, uv, 0).x);
      return v.xxx;
    }
    case 5u: {
      float v = saturate(shadowTexture.SampleLevel(samLinear_s, uv, 0).x);
      return v.xxx;
    }
    case 6u: {
      float3 n = normalTexture.SampleLevel(samLinear_s, uv, 0).xyz * 2.0 - 1.0;
      return saturate(abs(n));
    }
    case 7u:
      return saturate(envCandidateTexture.SampleLevel(samLinear_s, uv, 0).rgb);
    default:
      return saturate(sourceTexture.SampleLevel(samLinear_s, uv, 0).rgb);
  }
}

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
  float4 source = sourceTexture.SampleLevel(samLinear_s, uv, 0);
  if (ambientGiParams0.x < 0.5) return source;

  uint width, height;
  sourceTexture.GetDimensions(width, height);
  float2 texel = 1.0 / max(float2((float)width, (float)height), float2(1.0, 1.0));

  float2 uv_right = saturate(uv + float2(texel.x, 0.0));
  float2 uv_down = saturate(uv + float2(0.0, texel.y));

  float ao = saturate(aoTexture.SampleLevel(samLinear_s, uv, 0).x);
  float shadow = saturate(shadowTexture.SampleLevel(samLinear_s, uv, 0).x);
  float depth = saturate(depthTexture.SampleLevel(samLinear_s, uv, 0).x);

  float3 normal = normalTexture.SampleLevel(samLinear_s, uv, 0).xyz;
  float3 normal_right = normalTexture.SampleLevel(samLinear_s, uv_right, 0).xyz;
  float3 normal_down = normalTexture.SampleLevel(samLinear_s, uv_down, 0).xyz;
  float normal_edge = max(length(normal - normal_right), length(normal - normal_down));

  float depth_right = saturate(depthTexture.SampleLevel(samLinear_s, uv_right, 0).x);
  float depth_down = saturate(depthTexture.SampleLevel(samLinear_s, uv_down, 0).x);
  float depth_edge = max(abs(depth_right - depth), abs(depth_down - depth));

  float edge_reject = exp2(-(normal_edge * 16.0 + depth_edge * 128.0));

  float strength = saturate(ambientGiParams0.y * 0.01) * 0.15;
  float saturation = saturate(ambientGiParams0.z * 0.02);
  float shadow_influence = saturate(ambientGiParams0.w * 0.01);
  float color_boost = max(ambientGiParams1.x, 0.0) * 0.01;
  float bleed_radius = saturate(ambientGiParams1.w * 0.25) * 4.0;

  uint source_id_a = min((uint)(max(ambientGiParams2.x, 0.0) + 0.5), 7u);
  uint source_id_b = min((uint)(max(ambientGiParams2.y, 0.0) + 0.5), 7u);
  float source_blend = saturate(ambientGiParams2.z * 0.01);

  float ambient_mask = 1.0 - ao;
  float shadow_mask = 1.0 - shadow;
  float bounce_mask = lerp(ambient_mask, ambient_mask * shadow_mask, shadow_influence);

  float2 bleed_step = texel * max(bleed_radius, 1.0);
  float2 uv_l = saturate(uv + float2(-bleed_step.x, 0.0));
  float2 uv_r = saturate(uv + float2(bleed_step.x, 0.0));
  float2 uv_u = saturate(uv + float2(0.0, -bleed_step.y));
  float2 uv_d = saturate(uv + float2(0.0, bleed_step.y));

  float ao_l = saturate(aoTexture.SampleLevel(samLinear_s, uv_l, 0).x);
  float ao_r = saturate(aoTexture.SampleLevel(samLinear_s, uv_r, 0).x);
  float ao_u = saturate(aoTexture.SampleLevel(samLinear_s, uv_u, 0).x);
  float ao_d = saturate(aoTexture.SampleLevel(samLinear_s, uv_d, 0).x);

  float w_l = 1.0 - ao_l;
  float w_r = 1.0 - ao_r;
  float w_u = 1.0 - ao_u;
  float w_d = 1.0 - ao_d;
  float w_sum = max(w_l + w_r + w_u + w_d, 1e-4);

  float3 source_a = SampleCandidate(source_id_a, uv);
  float3 source_a_l = SampleCandidate(source_id_a, uv_l);
  float3 source_a_r = SampleCandidate(source_id_a, uv_r);
  float3 source_a_u = SampleCandidate(source_id_a, uv_u);
  float3 source_a_d = SampleCandidate(source_id_a, uv_d);
  float3 source_a_bleed = (source_a_l * w_l + source_a_r * w_r + source_a_u * w_u + source_a_d * w_d) / w_sum;

  float3 source_b = SampleCandidate(source_id_b, uv);
  float3 source_b_l = SampleCandidate(source_id_b, uv_l);
  float3 source_b_r = SampleCandidate(source_id_b, uv_r);
  float3 source_b_u = SampleCandidate(source_id_b, uv_u);
  float3 source_b_d = SampleCandidate(source_id_b, uv_d);
  float3 source_b_bleed = (source_b_l * w_l + source_b_r * w_r + source_b_u * w_u + source_b_d * w_d) / w_sum;

  float bleed_blend = saturate(bleed_radius * 0.25);
  float3 source_a_color = lerp(source_a, source_a_bleed, bleed_blend);
  float3 source_b_color = lerp(source_b, source_b_bleed, bleed_blend);
  float3 tint_source = lerp(source_a_color, source_b_color, source_blend);

  float tint_luma = dot(tint_source, LUMA_WEIGHTS);
  float3 tint_sat = lerp(tint_luma.xxx, tint_source, saturation);
  float tint_sat_luma = dot(tint_sat, LUMA_WEIGHTS);
  float3 tint_chroma = tint_sat - tint_sat_luma.xxx;
  float3 gi_tint = tint_sat_luma.xxx + tint_chroma * color_boost;

  float sky_reject = 1.0 - step(0.9995, depth);
  float3 gi = gi_tint * bounce_mask * edge_reject * sky_reject;
  gi *= strength;
  float3 gi_contrib = gi * saturate(1.0 - source.rgb);

  float debug_mode = max(ambientGiParams1.y, 0.0);
  if (debug_mode >= 0.5) {
    float debug_scale = max(ambientGiParams1.z, 0.001);
    uint mode = (uint)(debug_mode + 0.5);
    float3 debug_color = source.rgb;
    if (mode == 1u) {
      debug_color = gi_contrib * debug_scale;
    } else if (mode == 2u) {
      debug_color = gi_tint * debug_scale;
    } else if (mode == 3u) {
      debug_color = ambient_mask.xxx * debug_scale;
    } else if (mode == 4u) {
      debug_color = shadow_mask.xxx * debug_scale;
    } else if (mode == 5u) {
      debug_color = edge_reject.xxx * debug_scale;
    } else if (mode == 6u) {
      debug_color = bounce_mask.xxx * debug_scale;
    } else if (mode == 7u) {
      debug_color = ao.xxx * debug_scale;
    } else if (mode == 8u) {
      debug_color = shadow.xxx * debug_scale;
    } else if (mode == 9u) {
      debug_color = source_a_color * debug_scale;
    } else if (mode == 10u) {
      debug_color = source_b_color * debug_scale;
    } else if (mode == 11u) {
      debug_color = abs(tint_chroma) * debug_scale;
    } else if (mode == 12u) {
      debug_color = tint_source * debug_scale;
    } else if (mode == 13u) {
      debug_color = source_a_bleed * debug_scale;
    } else if (mode == 14u) {
      debug_color = source_b_bleed * debug_scale;
    }
    return float4(saturate(debug_color), source.a);
  }

  source.rgb += gi_contrib;
  return source;
}
