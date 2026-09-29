// ── falcomengine-plus Micro Shadows (Uncharted 4 contact-hardening aperture) ──
//
// A very low-cost way to add direct-light detail: instead of tracing anything,
// reuse the occlusion that the G-buffer already carries and turn it into an
// aperture that closes as NdotL falls off.
//
//     aperture    = 2 * AO^2
//     microshadow = saturate(NdotL + aperture - 1)
//     result      = lerp(1, microshadow, opacity)
//
// Holes, crevices and other surface features in the authored occlusion term come
// out, and because the term is a function of NdotL it darkens the terminator
// first -- which is exactly where a shadow map is least able to help.
//
// NOTE the consequence of AO = 1: the aperture becomes 2, so the expression
// saturates to 1.0 at every pixel and the pass produces nothing. That is correct
// behaviour (an unoccluded surface has no micro shadow) but it makes a broken AO
// input indistinguishable from a working one, which is why the occlusion term
// that was actually read is written to .z for the AO Source debug view.
//
// Two AO inputs, because the two sources have genuinely different formats and
// reading either as the wrong type produces silent garbage rather than an error:
//   g_srcAoGtvbao  r32_uint, value = uint(visibility * 255)  -> .x / 255
//   g_srcAoSsao    the game's own deferred AO, a float channel
// cs_micro_ao_source selects between them.
//
// Bindings (root param 2 / 3 of the host layout):
//   t0 mrtTexture0   world normal + material flags
//   t1 GTVBAO AO     uint-encoded occlusion
//   t2 game SSAO     float occlusion
//   t3 IS-FAST       128x128x32 RG8 blue noise volume (optional, see below)
//   u0 output        rgba8_unorm: .x term, .y diagnostic, .z the AO read
//
// IS-FAST, cs_micro_isfast_enabled, OFF by default. There is no ray here to jitter,
// so this does NOT dither a march. It dithers the AO QUANTISATION instead: GTVBAO
// stores visibility in one byte, so a slowly varying surface steps through discrete
// AO cells and 2*AO*AO turns that stepping into visible banding. Reading the AO one
// texel off in a blue-noise direction walks neighbouring cells per pixel, and the
// game's TAA averages them back to the right mean.
//
//   t3 IS-FAST      128x128x32 RG8 blue noise volume
//   u0 output       rgba8_unorm: .x term, .y diagnostic, .z the AO read

#define RENODX_SHADOWS_DECLARE_CB_SCENE
#include "shadows_common.hlsli"

Texture2D<uint4>  g_srcMrtNormal : register(t0);
Texture2D<uint>   g_srcAoGtvbao  : register(t1);
Texture2D<float4> g_srcAoSsao    : register(t2);
Texture3D<float2> g_isfastNoise  : register(t3);
SamplerState g_samplerPointClamp : register(s0);

RWTexture2D<float4> g_outMicro : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  if (p.x >= (uint)shader_injection_data.cs_working_w) return;
  if (p.y >= (uint)shader_injection_data.cs_working_h) return;

  // An unwritten G-buffer texel has no normal and therefore no meaningful NdotL.
  // Returning fully lit is the conservative choice: it is the exact identity for
  // the consumer's blend and cannot introduce a shadow nothing can justify.
  float3 worldNormal = DecodeFalcomMrtNormal(g_srcMrtNormal.Load(int3(p, 0)).xy);
  if (!FalcomNormalValid(worldNormal)) {
    g_outMicro[p] = float4(1.0, kMicroDiagNoNormal, 0.0, 1.0);
    return;
  }

  // Negated: lightDirection_g is the direction light TRAVELS. See
  // FalcomDirectionToLight for the three engine-side confirmations.
  const float3 lightDir = FalcomDirectionToLight();
  if (!FalcomNormalValid(lightDir)) {
    g_outMicro[p] = float4(1.0, kMicroDiagNoLightDir, 0.0, 1.0);
    return;
  }

  // Where to read the AO from. Identical to p when the dither is off.
  const uint2 gridDim = uint2(max(1u, (uint)shader_injection_data.cs_working_w),
                              max(1u, (uint)shader_injection_data.cs_working_h));
  int2 aoTexel = int2(p);
  if (shader_injection_data.cs_micro_isfast_enabled > 0.5f) {
    const float2 noise = FalcomSampleISFAST2(g_isfastNoise, g_samplerPointClamp, p,
                                             (uint)max(shader_injection_data.cs_noise_frame, 0.0f),
                                             128.0, 32.0,
                                             shader_injection_data.shadow_isfast_spatial_scale,
                                             shader_injection_data.shadow_isfast_temporal_speed,
                                             shader_injection_data.shadow_isfast_seed_offset,
                                             shader_injection_data.shadow_isfast_texture_loaded);
    // noise is in [0,1); centre it so the walk is symmetric about this texel.
    const int2 walk = int2(round((noise - 0.5) * 2.0));
    // Clamped rather than wrapped: a wrapped read at the screen edge would pull in
    // occlusion from the opposite side of the frame, which is worse than banding.
    aoTexel = clamp(int2(p) + walk, int2(0, 0), int2(gridDim) - 1);
  }

  const bool use_gtvbao = shader_injection_data.cs_micro_ao_source > 0.5f;
  // GTVBAO quantises the visibility term into byte 0 of an r32_uint
  // (GTVBAO_OutputWorkingTerm), so the decode is a straight /255 on that byte --
  // reading the same resource as a float would give the integer bit pattern.
  const float ao = use_gtvbao
      ? saturate(float(g_srcAoGtvbao.Load(int3(aoTexel, 0)).x) / 255.0)
      : saturate(g_srcAoSsao.Load(int3(aoTexel, 0)).x);

  const float ndotl = saturate(dot(worldNormal, lightDir));

  const float aperture = 2.0 * ao * ao
                       * max(0.0, shader_injection_data.cs_micro_aperture_scale);
  const float microshadow = saturate(ndotl + aperture - 1.0);
  g_outMicro[p] = float4(lerp(1.0, microshadow, saturate(shader_injection_data.cs_micro_opacity)),
                         kMicroDiagOk, ao, 1.0);
}
