// ── falcomengine-plus Screen-Space Contact Shadows ──
//
// Raymarches from each shaded surface point toward the light and asks whether
// the scene depth in between blocks it. Because the only inputs are the depth
// buffer the renderer already produced and a light direction, there is no shadow
// map to render and no second pass over the geometry -- which is the whole
// reason this technique is affordable.
//
// The march runs in clip space, not world space. Contact shadows are inherently
// a screen-space effect: the depth buffer lives in clip space and every occlusion
// test ultimately compares against it. Marching in world space instead means
// projecting a world position to screen, sampling depth, reconstructing a world
// position and taking two distances -- per step, inside the loop. Doing the walk
// directly in the space the data is already in removes all of that and is worth
// roughly 0.6 ms at 4K for a 32-sample ray.
//
// The ray start is offset along the surface normal. Without it the first sample
// sits on the receiver's own depth and reports a self-hit, which reads as a dark
// band over every lit surface.
//
// The step position is jittered with IS-FAST blue noise. Uniform steps make the
// step length directly visible as banding across a surface; randomising the
// offset decorrelates neighbouring pixels and lets the same sample count cover
// more ground. The jitter is only worth anything with a temporal resolve, which
// the game's TAA provides downstream. There is deliberately no IGN fallback: a
// low-discrepancy analytic pattern is cheaper but this pass needs the volume, and
// silently substituting a worse pattern would make the setting a lie.
//
// EVERY early-out writes its reason into the .y channel instead of silently
// producing a term of 1.0. A march that correctly finds nothing, a march whose
// MRT normal never bound, and a linearisation that swallowed the whole frame all
// look identical from outside the shader, and the whole point of this pass is to
// be diagnosable from a screenshot. See the kContactDiag* block in
// shadows_common.hlsli for the channel layout and the colour mapping.
//
// Bindings (root param 2 / 3 of the host layout):
//   t0 depth        scene depth, reversed-Z, device Z in .x
//   t1 mrtTexture0  world normal
//   t2 IS-FAST      128x128x32 blue noise volume
//   u0 output       rgba8_unorm: .x term, .y diagnostic, .z linear depth / 1000

#define RENODX_SHADOWS_DECLARE_CB_SCENE
#include "shadows_common.hlsli"

// Declared float4 to match the engine's own depthTexture declaration in the
// lighting shaders: the SRV handed to this pass IS the game's view, created for
// that float4 binding, so this is the faithful type for it rather than a
// convenient one.
Texture2D<float4> g_srcDepth     : register(t0);
Texture2D<uint4>  g_srcMrtNormal : register(t1);
Texture3D<float2> g_isfastNoise  : register(t2);
SamplerState g_samplerPointClamp : register(s0);

RWTexture2D<float4> g_outContact : register(u0);

// Every exit writes all four channels, so .w is a reliable "this texel was
// written" marker and no path can leave a stale value from a previous frame.
[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  if (p.x >= (uint)shader_injection_data.cs_working_w) return;
  if (p.y >= (uint)shader_injection_data.cs_working_h) return;

  const float2 unpack = FalcomDepthUnpackConsts();
  const float deviceDepth = g_srcDepth.Load(int3(p, 0)).x;

  // Depth that will not linearise, or an unpack pair that is not finite, means
  // every comparison below is meaningless. Report it rather than marching against
  // numbers that cannot be right.
  if (!isfinite(deviceDepth) || !isfinite(unpack.x) || !isfinite(unpack.y)
      || abs(unpack.x) < 1e-8) {
    g_outContact[p] = float4(1.0, kContactDiagBadDepth, 0.0, 1.0);
    return;
  }

  const float linearDepth = FalcomLinearDepth(deviceDepth, unpack);

  // Sky early-out. A sky pixel has no surface to shadow, no surface to march
  // from, and the march would run the full sample count before its own bounds
  // test could stop it. The threshold is a slider rather than a constant because
  // how much of the frame is sky varies enormously between scenes; the diagnostic
  // channel is what tells you when the threshold is set too high and is eating
  // real geometry.
  if (linearDepth > shader_injection_data.cs_contact_sky_depth) {
    g_outContact[p] = float4(1.0, kContactDiagSky, saturate(linearDepth * 0.001), 1.0);
    return;
  }

  float3 worldNormal = DecodeFalcomMrtNormal(g_srcMrtNormal.Load(int3(p, 0)).xy);
  if (!FalcomNormalValid(worldNormal)) {
    g_outContact[p] = float4(1.0, kContactDiagNoNormal, saturate(linearDepth * 0.001), 1.0);
    return;
  }

  // Negated: lightDirection_g is the direction light TRAVELS, so marching along it
  // un-negated walks AWAY from the light -- into the receiver's own back side and
  // into the floor behind it. That reports occlusion when looking one way and none
  // when looking the other. See FalcomDirectionToLight for the engine-side proof.
  const float3 lightDir = FalcomDirectionToLight();
  if (!FalcomNormalValid(lightDir)) {
    g_outContact[p] = float4(1.0, kContactDiagNoLightDir, saturate(linearDepth * 0.001), 1.0);
    return;
  }

  // Reconstruct the shaded point in world space, then lift it along the normal.
  // Working from the reconstructed world position (rather than interpolating a
  // world position from the vertex stage) is what keeps this a full-screen pass
  // that works for every draw in the deferred frame, not just geometry that
  // opted in.
  const float2 uv = (float2(p) + 0.5) / max(float2(shader_injection_data.cs_working_w,
                                                   shader_injection_data.cs_working_h),
                                           1.0.xx);
  const float4 clipHere = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, deviceDepth, 1.0);
  const float4 worldH = mul(clipHere, viewProjInv_g);
  if (abs(worldH.w) <= 1e-6 || !isfinite(worldH.w)) {
    g_outContact[p] = float4(1.0, kContactDiagBadProjection, saturate(linearDepth * 0.001), 1.0);
    return;
  }
  const float3 worldPos = worldH.xyz / worldH.w
                        + worldNormal * max(0.0, shader_injection_data.cs_contact_normal_bias);

  const float rayLength = max(0.0, shader_injection_data.cs_contact_ray_length);
  const float4 clipStart = mul(float4(worldPos, 1.0), viewProj_g);
  const float4 clipEnd = mul(float4(worldPos + lightDir * rayLength, 1.0), viewProj_g);
  if (abs(clipStart.w) <= 1e-6 || abs(clipEnd.w) <= 1e-6
      || !isfinite(clipStart.w) || !isfinite(clipEnd.w)) {
    g_outContact[p] = float4(1.0, kContactDiagBadProjection, saturate(linearDepth * 0.001), 1.0);
    return;
  }

  const float jitter = shader_injection_data.cs_contact_isfast_enabled > 0.5f
      ? FalcomSampleISFAST(g_isfastNoise, g_samplerPointClamp, p,
                           (uint)max(shader_injection_data.cs_noise_frame, 0.0f),
                           128.0, 32.0,
                           shader_injection_data.shadow_isfast_spatial_scale,
                           shader_injection_data.shadow_isfast_temporal_speed,
                           shader_injection_data.shadow_isfast_seed_offset,
                           shader_injection_data.shadow_isfast_texture_loaded)
      : 0.5f;

  g_outContact[p] = float4(
      FalcomContactShadowMarch(clipStart, clipEnd, unpack,
                              g_srcDepth, g_samplerPointClamp,
                              max(0.0, shader_injection_data.cs_contact_thickness),
                              max(0.0, shader_injection_data.cs_contact_bias),
                              max(1.0, floor(shader_injection_data.cs_contact_sample_count + 0.5)),
                              jitter,
                              max(0.0, shader_injection_data.cs_contact_response_scale)),
      kContactDiagMarched, saturate(linearDepth * 0.001), 1.0);
}
