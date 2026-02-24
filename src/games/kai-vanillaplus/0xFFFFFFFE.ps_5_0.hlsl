SamplerState samLinear_s : register(s0);
Texture2D<float4> sourceTexture : register(t0);
Texture2D<float4> ssgiTexture : register(t1);
Texture2D<uint4> mrtTexture0 : register(t2);
Texture2D<uint4> mrtTexture1 : register(t3);
Texture2D<float4> depthTexture : register(t4);
Texture2D<float4> ssaoTexture : register(t5);

static const uint CHARACTER_MASK_SHIFT = 8u;
static const float3 LUMA_WEIGHTS = float3(0.299, 0.587, 0.114);
static const float INV_U16 = 1.0 / 32767.0;
static const float INV_U8 = 1.0 / 255.0;
static const float PI = 3.14159274;

#if ((__SHADER_TARGET_MAJOR == 5 && __SHADER_TARGET_MINOR >= 1) || __SHADER_TARGET_MAJOR >= 6)
cbuffer character_ssgi_composite_settings : register(b13, space0) {
#else
cbuffer character_ssgi_composite_settings : register(b13) {
#endif
  // x=strength, y=alpha_scale, z=chroma_strength, w=luma_strength
  float4 characterGiParams0;
  // x=shadow_power, y=headroom_power, z=max_add, w=dark_boost
  float4 characterGiParams1;
  // x=debug_mode, y=debug_scale, z=debug_chars_only, w=bright_boost
  float4 characterGiParams2;
  // x=peak_luma_cap, y=depth_reject, z=normal_reject, w=ao_influence
  float4 characterGiParams3;
  // x=reject_strength, yzw=reserved
  float4 characterGiParams4;
}

uint2 ComputePixelCoord(float2 uv, uint width, uint height) {
  width = max(width, 1u);
  height = max(height, 1u);
  float2 pixel_f = saturate(uv) * float2((float)width, (float)height);
  return min((uint2)pixel_f, uint2(width - 1u, height - 1u));
}

float3 DecodeMrt0Normal(uint4 mrt) {
  float2 enc = float2((float)mrt.x, (float)mrt.y) * INV_U16 + float2(-1.0, -1.0);
  float sn, cs;
  sincos(PI * enc.x, sn, cs);
  float xy = sqrt(saturate(1.0 - enc.y * enc.y));
  float3 n = float3(cs * xy, sn * xy, enc.y);
  return normalize(n);
}

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
  float4 source = sourceTexture.SampleLevel(samLinear_s, uv, 0);
  float3 source_base = source.rgb;
  float4 ssgi = ssgiTexture.SampleLevel(samLinear_s, uv, 0);

  uint mrt0_width, mrt0_height;
  mrtTexture0.GetDimensions(mrt0_width, mrt0_height);
  uint2 mrt0_pixel = ComputePixelCoord(uv, mrt0_width, mrt0_height);
  uint4 mrt = mrtTexture0.Load(int3(mrt0_pixel, 0));
  uint2 mrt0_right_pixel = min(mrt0_pixel + uint2(1u, 0u), uint2(max(mrt0_width, 1u) - 1u, max(mrt0_height, 1u) - 1u));
  uint2 mrt0_down_pixel = min(mrt0_pixel + uint2(0u, 1u), uint2(max(mrt0_width, 1u) - 1u, max(mrt0_height, 1u) - 1u));
  uint4 mrt_right = mrtTexture0.Load(int3(mrt0_right_pixel, 0));
  uint4 mrt_down = mrtTexture0.Load(int3(mrt0_down_pixel, 0));

  uint mrt1_width, mrt1_height;
  mrtTexture1.GetDimensions(mrt1_width, mrt1_height);
  uint2 mrt1_pixel = ComputePixelCoord(uv, mrt1_width, mrt1_height);
  uint2 mrt1_right_pixel = min(mrt1_pixel + uint2(1u, 0u), uint2(max(mrt1_width, 1u) - 1u, max(mrt1_height, 1u) - 1u));
  uint2 mrt1_down_pixel = min(mrt1_pixel + uint2(0u, 1u), uint2(max(mrt1_width, 1u) - 1u, max(mrt1_height, 1u) - 1u));
  uint4 mrt1 = mrtTexture1.Load(int3(mrt1_pixel, 0));
  uint4 mrt1_right = mrtTexture1.Load(int3(mrt1_right_pixel, 0));
  uint4 mrt1_down = mrtTexture1.Load(int3(mrt1_down_pixel, 0));

  uint depth_width, depth_height;
  depthTexture.GetDimensions(depth_width, depth_height);
  uint2 depth_pixel = ComputePixelCoord(uv, depth_width, depth_height);
  uint2 depth_right_pixel = min(depth_pixel + uint2(1u, 0u), uint2(max(depth_width, 1u) - 1u, max(depth_height, 1u) - 1u));
  uint2 depth_down_pixel = min(depth_pixel + uint2(0u, 1u), uint2(max(depth_width, 1u) - 1u, max(depth_height, 1u) - 1u));
  float depth_center = max(depthTexture.Load(int3(depth_pixel, 0)).x, 1e-6);
  float depth_right = max(depthTexture.Load(int3(depth_right_pixel, 0)).x, 1e-6);
  float depth_down = max(depthTexture.Load(int3(depth_down_pixel, 0)).x, 1e-6);
  float depth_delta = max(abs(log2(depth_right) - log2(depth_center)), abs(log2(depth_down) - log2(depth_center)));

  float4 ssao_sample = ssaoTexture.SampleLevel(samLinear_s, uv, 0);
  float ao_raw = saturate(max(ssao_sample.x, ssao_sample.y * ssao_sample.z));

  uint material_flags = (mrt.z >> CHARACTER_MASK_SHIFT);
  uint is_character = (material_flags & 1u);
  float char_mask = (is_character != 0u) ? 1.0 : 0.0;

  float3 normal_center = DecodeMrt0Normal(mrt);
  float3 normal_right = DecodeMrt0Normal(mrt_right);
  float3 normal_down = DecodeMrt0Normal(mrt_down);
  float normal_edge = max(1.0 - saturate(dot(normal_center, normal_right)),
                          1.0 - saturate(dot(normal_center, normal_down)));

  float material_center = (float)(mrt1.y & 0xFFu) * INV_U8;
  float material_right = (float)(mrt1_right.y & 0xFFu) * INV_U8;
  float material_down = (float)(mrt1_down.y & 0xFFu) * INV_U8;
  float material_edge = max(abs(material_center - material_right), abs(material_center - material_down));
  normal_edge = max(normal_edge, material_edge);

  float depth_reject = max(characterGiParams3.y, 0.0);
  float normal_reject = max(characterGiParams3.z, 0.0);
  float ao_influence = saturate(characterGiParams3.w);

  float depth_factor = exp2(-depth_delta * depth_reject);
  float normal_factor = exp2(-normal_edge * normal_reject * 4.0);
  float ao_factor = lerp(1.0, ao_raw, ao_influence);
  float reject_strength = max(characterGiParams4.x, 0.0);
  float reject_factor = pow(saturate(depth_factor * normal_factor * ao_factor), reject_strength);

  float source_luma = dot(source_base, LUMA_WEIGHTS);
  float shadow_power = max(characterGiParams1.x, 0.1);
  float shadow_raw = pow(saturate(1.0 - source_luma), shadow_power);
  float bright_boost = max(characterGiParams2.w, 0.0);
  float bright_boost_scale = max(bright_boost - 1.0, 0.0);
  float bright_mask = saturate(1.0 - shadow_raw);
  float shadow_mask = saturate(shadow_raw + bright_mask * bright_boost_scale);
  float dark_boost = max(characterGiParams1.w, 0.0);
  float dark_factor = lerp(1.0, dark_boost, shadow_raw);

  float3 gi_color = max(ssgi.rgb, 0.0);
  float gi_luma = dot(gi_color, LUMA_WEIGHTS);
  float3 gi_chroma = gi_color - gi_luma.xxx;
  float chroma_strength = max(characterGiParams0.z, 0.0);
  float luma_strength = max(characterGiParams0.w, 0.0);
  float3 gi_filtered = gi_chroma * chroma_strength + (gi_luma * luma_strength).xxx;

  float alpha_scale = max(characterGiParams0.y, 0.0);
  float gi_weight = saturate(ssgi.a * alpha_scale) * shadow_mask * dark_factor;
  gi_weight *= reject_factor;

  float headroom_power = max(characterGiParams1.y, 0.1);
  float3 headroom = pow(saturate(1.0 - source_base), headroom_power);
  float bright_headroom_floor = saturate(bright_boost_scale * 0.35);
  float3 headroom_adjusted = max(headroom, bright_headroom_floor.xxx);

  float strength = max(characterGiParams0.x, 0.0);
  float max_add = max(characterGiParams1.z, 0.0);
  float max_add_boost = 1.0 + bright_boost_scale * bright_mask;
  float3 final_gain = gi_weight * strength * headroom_adjusted;
  float3 gi_contrib = gi_filtered * final_gain;
  float cap_scale = sqrt(max(reject_factor, 0.0));
  float gi_cap = max_add * max_add_boost * cap_scale;
  gi_contrib = min(gi_contrib, gi_cap.xxx);
  float peak_luma_cap = max(characterGiParams3.x, 0.0);
  if (peak_luma_cap > 0.0) {
    float contrib_luma = dot(max(gi_contrib, 0.0), LUMA_WEIGHTS);
    float luma_scale = min(1.0, peak_luma_cap / max(contrib_luma, 1e-6));
    gi_contrib *= luma_scale;
  }
  gi_contrib *= char_mask;
  source.rgb = source_base + gi_contrib;

  uint debug_mode = (uint)(max(characterGiParams2.x, 0.0) + 0.5);
  if (debug_mode != 0u) {
    float debug_scale = max(characterGiParams2.y, 0.001);
    bool debug_chars_only = characterGiParams2.z >= 0.5;
    float3 debug_color = source_base;

    if (debug_mode == 1u) {
      debug_color = (char_mask > 0.5) ? float3(1.0, 0.2, 0.2) : float3(0.1, 0.1, 0.1);
    } else if (debug_mode == 2u) {
      debug_color = gi_color * debug_scale;
    } else if (debug_mode == 3u) {
      debug_color = (ssgi.aaa * debug_scale);
    } else if (debug_mode == 4u) {
      debug_color = gi_filtered * debug_scale;
    } else if (debug_mode == 5u) {
      debug_color = (gi_weight.xxx * debug_scale);
    } else if (debug_mode == 6u) {
      debug_color = abs(gi_contrib) * debug_scale;
    } else if (debug_mode == 7u) {
      debug_color = source_base;
    } else if (debug_mode == 8u) {
      float contrib_luma = dot(abs(gi_contrib), LUMA_WEIGHTS) * debug_scale;
      debug_color = contrib_luma.xxx;
    } else if (debug_mode == 9u) {
      debug_color = headroom_adjusted * debug_scale;
    } else if (debug_mode == 10u) {
      float gain_luma = dot(final_gain, LUMA_WEIGHTS) * debug_scale;
      debug_color = gain_luma.xxx;
    } else if (debug_mode == 11u) {
      debug_color = shadow_mask.xxx * debug_scale;
    } else if (debug_mode == 12u) {
      debug_color = depth_factor.xxx * debug_scale;
    } else if (debug_mode == 13u) {
      debug_color = normal_factor.xxx * debug_scale;
    } else if (debug_mode == 14u) {
      debug_color = ao_factor.xxx * debug_scale;
    } else if (debug_mode == 15u) {
      debug_color = reject_factor.xxx * debug_scale;
    }

    if (debug_chars_only && char_mask < 0.5) {
      debug_color = float3(0.0, 0.0, 0.0);
    }

    return float4(saturate(debug_color), source.a);
  }

  return source;
}
