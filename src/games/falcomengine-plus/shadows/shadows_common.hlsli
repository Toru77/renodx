#ifndef SRC_GAMES_FALCOMENGINE_PLUS_SHADOWS_SHADOWS_COMMON_HLSLI_
#define SRC_GAMES_FALCOMENGINE_PLUS_SHADOWS_SHADOWS_COMMON_HLSLI_

// ── falcomengine-plus screen-space shadow passes — shared core ──
//
// Two compute passes consume this (micro_shadows.cs_5_0.hlsl and
// contact_shadows.cs_5_0.hlsl), and the lighting pixel shaders include it again
// to run the same contact march per local light. Everything here is therefore
// free of stage-specific assumptions: no compute-only intrinsics, no
// pixel-only intrinsics, and no globals of its own -- every entry point takes
// the textures, samplers and parameters it needs.
//
// The scene constant buffer subset below is deliberately the MINIMUM both
// consumers need. Its packoffsets are identical in every Falcom title this mod
// supports (Sora 1st, Sora 2nd, Kai, Kyoto Xanadu, Daybreak 2), which is why
// there is no per-game #ifdef here -- unlike prevViewProj_g / sceneTime_g /
// resolutionScaling_g, which all differ and which nothing in this file reads.
// Anything added to that cbuffer must be re-verified against all five shaders
// before it is relied on.

// User settings via push constants at b13 (shared.h).
#include "../shared.h"

// Falcom MRT world-normal decode, shared with the GTVBAO passes.
#include "../include/mrt_normal.hlsli"

// ── Game's scene constant buffer (b0) ──
// Must match the register layout of the game's own lighting shader. See the
// table in src/games/AGENTS.md before changing any packoffset here.
//
// Declared ONLY for translation units that do not already have it. The lighting
// pixel shaders include this file to reuse the march and the strength blend, and
// they each declare their own cb_scene from the game's real layout -- a second
// declaration of the same register in one translation unit is a hard HLSL error,
// not a redefinition the compiler folds. The compute passes have no such
// declaration, so they define RENODX_SHADOWS_DECLARE_CB_SCENE before including
// this file. Every member used below (proj_g, viewProj_g, lightDirection_g) is
// declared at the same offset in all five games' lighting shaders, so either
// declaration satisfies the helpers.
#if defined(RENODX_SHADOWS_DECLARE_CB_SCENE)
cbuffer cb_scene : register(b0)
{
  float4x4 view_g          : packoffset(c0);
  float4x4 viewInv_g       : packoffset(c4);
  float4x4 proj_g          : packoffset(c8);
  float4x4 projInv_g       : packoffset(c12);
  float4x4 viewProj_g      : packoffset(c16);
  float4x4 viewProjInv_g   : packoffset(c20);
  float2 vpSize_g          : packoffset(c24);
  float2 invVPSize_g       : packoffset(c24.z);
  float3 lightColor_g      : packoffset(c25);
  float3 lightDirection_g  : packoffset(c26);
};
#endif

// ── Direction TOWARD the main light ──
//
// lightDirection_g is the direction the light TRAVELS, i.e. from the light toward
// the scene. Everything that needs "which way is the light" must negate it. Three
// independent confirmations in the engine's own code:
//
//   sora1st/lighting/lighting_0xFDAAF80E:811  dot(N, -lightDirection_g.xyz)
//   sora2nd/lighting/lighting_0xCA3D8596:859  dot(N, -lightDirection_g.xyz)
//   kai/lighting/lighting_0x430ED091:263-272
//     GetMainLightDirectionViewToLight(), whose comment reads "lightDirection_g
//     points from the light to the scene; invert to get direction TO light" and
//     which returns -light_dir_view * rsqrt(len2).
//
// This is a named function rather than a bare negation at each call site because
// the sun passes got it wrong: micro shaded with an inverted NdotL and contact
// marched AWAY from the light, which reported occlusion when looking one way and
// none when looking the other. The local-light path never had the bug because it
// derives the direction from lightPos - worldPos, where the sign is fixed by the
// definition of the subtraction.
//
// Returns a zero vector when there is no light this frame; callers must treat that
// as "no light", not as a direction.
float3 FalcomDirectionToLight()
{
  return -FalcomSafeNormalize3(lightDirection_g, float3(0.0, 0.0, 0.0));
}

// ── Device depth -> linear eye depth ──
// The engine renders reversed-Z, so a single formula covers both conventions:
// the handedness is entirely in proj_g, and the sign guard below normalises it.
// mul = -proj[3][2], add = proj[2][2], linear = mul / (add - deviceZ).
// This is the same pair gtvbao_common.hlsl (BuildGTAOConstants) and
// dyncube/dyncube_common.hlsli (DynCubeGetDepthUnpackConsts) derive; they
// cannot include this file because gtvbao declares its own b13 block.
float2 FalcomDepthUnpackConsts()
{
  const float mul_c = -proj_g[3][2];
  float add_c = proj_g[2][2];
  if (mul_c * add_c < 0.0) add_c = -add_c;
  return float2(mul_c, add_c);
}

// Returns a huge finite value for sky rather than inf/NaN, so every downstream
// comparison and lerp stays well defined without a per-call guard.
float FalcomLinearDepth(float deviceZ, float2 unpack)
{
  const float denom = unpack.y - deviceZ;
  const float z = (abs(denom) > 1e-8) ? (unpack.x / denom) : 0.0;
  if (!isfinite(z) || z <= 0.0) return 1.0e9;
  return z;
}

// ── IS-FAST blue noise ──
// One spatio-temporal lookup, in [0,1). The volume is 128x128x32 RG8_UNORM
// (fast_noise_ea.dds); the modulo is done here because the descriptor is bound
// to a point-CLAMP sampler, unlike the wrap sampler the pixel path uses.
//
// The temporal slice is frame_index % 32 so consecutive frames walk the volume
// and the game's TAA averages the jitter away. The spatial scale lets a user
// make the dither finer or coarser than one texel per pixel, and the temporal
// speed lets them freeze it (0) when TAA is off and the noise would otherwise
// crawl.
//
// The texture and sampler are parameters rather than globals so this works from
// both the compute passes and the lighting pixel shaders, which bind different
// volumes and samplers.
//
// volumeLoaded is the "is there a real volume here" flag. When it is 0 the
// function returns the fixed midpoint 0.5 rather than sampling: there is
// deliberately no IGN fallback, so a caller that ignores this flag and samples
// anyway would read an unbound 2D stand-in as a 3D texture.
float FalcomSampleISFAST(Texture3D<float2> volume, SamplerState noiseSampler,
                         uint2 pixel, uint frame, float volumeSize,
                         float temporalSlices, float spatialScale,
                         float temporalSpeed, float seedOffset,
                         float volumeLoaded)
{
  if (volumeLoaded < 0.5f) return 0.5f;
  const float3 uvw = float3(
      (float)(pixel.x % (uint)volumeSize) / volumeSize,
      (float)(pixel.y % (uint)volumeSize) / volumeSize,
      (float)((frame + (uint)seedOffset) % (uint)temporalSlices) / temporalSlices);
  return volume.SampleLevel(noiseSampler,
                            float3(uvw.xy * spatialScale, uvw.z * temporalSpeed), 0).x;
}

// The two-channel form, for a caller that needs an independent pair rather than
// one jitter value. The IS-FAST volume is RG8, so both channels are real noise and
// using them separately decorrelates two axes. The single-channel form above is
// this one's .x, so there is one definition of the uvw maths.
float2 FalcomSampleISFAST2(Texture3D<float2> volume, SamplerState noiseSampler,
                           uint2 pixel, uint frame, float volumeSize,
                           float temporalSlices, float spatialScale,
                           float temporalSpeed, float seedOffset,
                           float volumeLoaded)
{
  if (volumeLoaded < 0.5f) return float2(0.5, 0.5);
  const float3 uvw = float3(
      (float)(pixel.x % (uint)volumeSize) / volumeSize,
      (float)(pixel.y % (uint)volumeSize) / volumeSize,
      (float)((frame + (uint)seedOffset) % (uint)temporalSlices) / temporalSlices);
  return volume.SampleLevel(noiseSampler,
                            float3(uvw.xy * spatialScale, uvw.z * temporalSpeed), 0).xy;
}

// ── Screen-Space Contact Shadows, clip space ──
// Marches from a surface point toward the light, testing the reconstructed
// scene depth at each step. The whole march stays in the space the depth buffer
// is already in, so the per-step work is one texture fetch plus a compare --
// no world-to-screen projection and no position reconstruction inside the loop,
// which is what makes this affordable at full resolution.
//
// The march counts how much of the ray falls inside the thickness band, and scales
// that coverage into a shadow term. Jittering the starting offset by the IS-FAST
// value breaks the visible stepping that a fixed step produces; without a temporal
// resolve that trade would be a loss, so the consumer is expected to run under TAA.
//
// ── why coverage, and why a scale ──
//
// The previous form asked a yes/no question: "did ANY sample land inside the band?"
// and returned a hard 0.0 on the first one. If the band covers a fraction f of the
// ray, an N-sample lattice intersects it with probability 1-(1-f)^N, so the answer
// saturates towards yes as N rises. Measured at f=0.12: 0.49 of shadowed pixels at
// N=4, 0.96 at N=8, 1.00 at N=16. Two consequences, both reported from play. At low
// N roughly half the pixels that should be shadowed are missed and WHICH half changes
// every frame, which reads as noise. At high N every pixel with even a sliver of
// overlap goes fully black, so the contact region visibly grows simply because the
// sample count was raised. Both are artifacts of the combiner, not of the geometry.
//
// Counting the fraction removes the yes/no decision entirely: the result is a
// continuous function of f, and f is a property of the pixel that does not depend on
// N. Measured at f=0.12, the mean term is 0.458 / 0.452 / 0.451 / 0.451 for
// N=4/8/16/32 -- flat -- while the old form ran 0.458 / 0.903 / 0.969 / 0.984, i.e. it
// saturated almost everything to solid black.
//
// The coverage is then multiplied by `responseScale`. That factor is not arbitrary:
// 4 is the sample count whose average response the pass is being asked to reproduce.
// It gives the invariant term(N=4, scale=4) == the old binary test exactly, because
// 1 - saturate(4*hits/4) is 1 - saturate(hits), which is 0 or 1 precisely as before.
// So Sample Count becomes purely a quality/noise control and Response Scale purely a
// strength control, instead of one slider setting all three of noise, width and
// darkness. Measured at N=32, f=0.145: scale 3 gives a term of 0.597, 4 gives 0.463,
// 5 gives 0.329.
//
// The `inside` test is deliberately a hard 0/1 and NOT a penetration ramp. The band
// does two jobs -- nearness (the assumed occluder thickness) and validity (rejecting
// samples whose screen position has drifted out of register with the depth they are
// compared against, which can otherwise read a surface arbitrarily nearer than the
// ray point and register as full occlusion). A ramp softens the second job and
// measured out as "the average kills the shadows", because it also halves the weight
// of every legitimate hit. Keeping the test binary leaves the validity filter exactly
// as strict as it has always been.
//
// Note the early-out on leaving the screen under-counts coverage, since the remaining
// samples never contribute. That is the conservative direction and was already the
// documented behaviour of this loop: a ray that leaves the screen loses shadow rather
// than gaining it.
//
// `clipStart`/`clipEnd` are the ray's world endpoints already projected by the
// caller, which is also where the normal bias is applied: the biased point is
// part of the caller's world position (it also feeds NdotL and any local-light
// reuse), and threading it through separately would mean two subtly different
// notions of "the surface point".
//
// `unpack` comes from FalcomDepthUnpackConsts. The world-space ray length is
// deliberately NOT a parameter: in clip space the step size follows from the two
// endpoints, so passing it would only create a second thing to keep consistent.
//
// The depth texture is taken as Texture2D<float4>. The element type is irrelevant
// (only .x is read) but the games disagree on it -- Kai declares depthTexture as
// Texture2D<float> at t3, Sora and Kyoto declare Texture2D<float4> at t4 -- and
// fxc will not accept a template here, so one concrete type has to win. The
// consumer declares its OWN depth binding for the shadow path (t36) rather than
// reusing the game's, which is what makes a single type possible without editing
// a register the game owns. The host pushes the same SRV to both.
float FalcomContactShadowMarch(float4 clipStart, float4 clipEnd, float2 unpack,
                               Texture2D<float4> depthTex, SamplerState depthSampler,
                               float thickness, float bias,
                               float sampleCount, float jitter, float responseScale)
{
  if (sampleCount < 1.0) return 1.0;
  const float invSamples = rcp(sampleCount);

  const float3 ndcStart = clipStart.xyz / clipStart.w;
  const float3 ndcEnd = clipEnd.xyz / clipEnd.w;
  if (!isfinite(ndcStart.z) || !isfinite(ndcEnd.z)) return 1.0;

  const float rayLinearStart = FalcomLinearDepth(ndcStart.z, unpack);
  const float rayLinearEnd = FalcomLinearDepth(ndcEnd.z, unpack);

  // Jitter advances the first step by a fraction of the step size, so the whole
  // ray is offset rather than only its tail.
  float rayLinearDepth = rayLinearStart + (rayLinearEnd - rayLinearStart) * jitter * invSamples;
  const float rayLinearStep = (rayLinearEnd - rayLinearStart) * invSamples;

  // The engine's clip Y is inverted relative to the D3D UV convention, so the
  // y scale is negative. Every Falcom shader in this repo uses this exact form.
  const float2 uvScale = float2(0.5, -0.5);
  const float2 uvStep = (ndcEnd.xy - ndcStart.xy) * (invSamples * uvScale);
  float2 uv = mad(ndcStart.xy, uvScale, 0.5) + uvStep * jitter;

  float hits = 0.0;

  [loop]
  for (int i = 0; i < (int)sampleCount; i++) {
    // Break rather than clamp: past the screen edge there is no depth data, and
    // a clamped fetch would test against the frame's border geometry instead.
    if (any(uv < 0.0) || any(uv > 1.0)) break;

    const float sceneDepth = FalcomLinearDepth(depthTex.SampleLevel(depthSampler, uv, 0).x, unpack);
    const float penetration = rayLinearDepth - sceneDepth;
    hits += (penetration > bias && penetration < thickness) ? 1.0 : 0.0;

    rayLinearDepth += rayLinearStep;
    uv += uvStep;
  }

  return 1.0 - saturate(hits * invSamples * responseScale);
}

// ── Output channel layout ──
// Both passes write an rgba8_unorm target. RGBA8_UNORM is in the D3D11.0
// guaranteed UAV type-write set (R8_UNORM is not -- it only joins that set at
// feature level 11_1, so it cannot be relied on here), and the spare channels
// buy a self-diagnosis the console cannot give us.
//
//   .x  the term itself: 1 = lit, 0 = fully occluded. The only channel read in
//       the normal path.
//   .y  WHY the term is what it is. A march that finds nothing and a march that
//       never ran produce the same .x, and from outside the shader they are
//       indistinguishable -- which is exactly the trap this layout exists to
//       remove. See the decode in FalcomApplyShadowTerms.
//   .z  a pass-specific extra: the occlusion term that was actually read
//       (micro), or the linearised scene depth (contact).
//   .w  1 whenever the pass ran at all. 0 means the target was never written,
//       which points at resource creation rather than at the algorithm.

// Contact .y codes. kContactDiagMarched must stay 0 so a healthy frame is the
// default state rather than something the shader has to opt into.
#define kContactDiagMarched      0u  // the march ran to completion
#define kContactDiagNoNormal     1u  // MRT0 decoded to a zero vector
#define kContactDiagBadProjection 2u // viewProj_g produced a degenerate clip w
#define kContactDiagSky          3u  // linear depth beyond the sky cutoff
#define kContactDiagBadDepth     4u  // device depth or the unpack consts are not finite
#define kContactDiagNoLightDir  5u  // lightDirection_g was zero-length (no sun this frame)

// Micro .y codes.
#define kMicroDiagOk            0u
#define kMicroDiagNoNormal      1u  // MRT0 decoded to a zero vector
#define kMicroDiagNoLightDir    2u  // lightDirection_g was zero-length

// ── Per-pixel strength selection ──
// The two techniques share this because the environment/character split is a
// tuning decision, not a per-technique one, and every consumer (five lighting
// shaders plus the two character passes) needs the identical blend. The passes
// themselves emit the raw term; deciding how much of it applies to this pixel
// happens where the character mask is already decoded, which is the only place
// that can decode it correctly per game.
//
// The passes emit rawTerm = 1 for lit and 0 for fully occluded, so the dark
// fraction is (1 - rawTerm). The multiplier returned here is therefore built from
// that dark fraction, and max_darkening CAPS how dark a shadowed pixel may get:
// 1 permits fully black, 0 removes the effect.
//
// Note the sign: blending the raw term directly (lerp(1, rawTerm, s)) would make
// a fully lit pixel return 1 but an occluded one return 1 - s, i.e. a weak
// shadow, and blending 1 - rawTerm*maxDarkening returns 0 for a lit pixel, which
// renders the whole frame black. Both were tried; only the form below is right.
float FalcomShadowStrength(float rawTerm, float strength, bool isCharacter,
                           float envStrength, float charStrength, float maxDarkening)
{
  const float w = isCharacter ? charStrength : envStrength;
  const float s = saturate(strength) * saturate(w);
  // 1 - rawTerm is 0 for a lit pixel, so the multiplier below is exactly 1.0 there
  // and can never brighten anything.
  const float darkening = (1.0 - saturate(rawTerm)) * saturate(maxDarkening);
  return 1.0 - darkening * s;
}

// ── Micro Shadows: applied to the resolved colour ──
//
// Micro is an APERTURE term derived from ambient occlusion, so it belongs on the
// final colour: it models how much light a surface occludes, not where the sun is.
// Keeping it here is what makes it correct indoors, where the sun term is already
// ~0 and the ambient is the whole story.
//
// Contact Shadows are the opposite case -- a directional light march -- and are
// applied by FalcomApplyContactToSun instead. Multiplying contact by the final
// colour was the original bug: indoors the CSM has already zeroed the sun, so a
// sun contact shadow had no sun left to shadow and went on darkening the ambient
// instead, which is what made contact shadows appear in interiors.
float3 FalcomApplyMicroShadows(float3 lit, bool isCharacter, float4 micro)
{
  if (shader_injection_data.cs_micro_dedicated_bound < 0.5f) return lit;
  return lit * FalcomShadowStrength(
      micro.x, shader_injection_data.cs_micro_strength, isCharacter,
      shader_injection_data.cs_micro_env_strength,
      shader_injection_data.cs_micro_char_strength, 1.0);
}

// ── Sun-contact range gate ──
//
// Contact shadows are a sun technique, so they must only darken light the sun
// actually delivered. There are two distinct reasons a pixel must be skipped, and
// they are not the same test:
//
//   1. The CSM says this pixel is not sunlit. There is no sun term to shadow, so
//      contact is meaningless.
//   2. The pixel is BEYOND the CSM's coverage. Past the last cascade split the
//      engine clamps to the outermost cascade instead of falling off, so the
//      visibility it reports there is a clamp artefact, not a measurement. The
//      engine then treats the pixel as lit. This is the case that put contact
//      shadows on interior walls: an apartment sits at distances the CSM was
//      never fitted for, so every interior surface reads as "sunlit" and the
//      contact term darkens it.
//
// The engine measures cascade coverage against the RADIAL distance from the camera
// to the surface -- Kai/lighting_0x430ED091:1180-1193 takes the camera position
// from viewInv_g and does `length(worldPos - camPos)`. That is NOT the same
// quantity as the linear eye depth FalcomLinearDepth returns (which is the
// perpendicular distance along the view axis), so the reconstruction below is
// deliberately radial. Comparing eye depth against the split would underestimate
// distance by up to the field of view and let interior pixels through the gate.
//
// viewInv_g and viewProjInv_g are declared at the same packoffsets in all five
// titles' lighting shaders (c4 and c20 of cb_scene), which is what lets this
// shared file reference them with no per-game #ifdef.
float3 FalcomCameraWorldPosition()
{
  // Same indexing the engine uses at kai/lighting_0x430ED091:1180-1182.
  return float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32);
}

// Radial distance from the camera to whatever surface this pixel shades, matched
// to the engine's own cascade metric. Reconstructed from the depth buffer rather
// than read from a register: the registers that hold it are clobbered long before
// the sun composite in every one of the four call sites (Kai's r7.w is rewritten
// eight times between the cascade select and the contact call), so a register-based
// version would be correct on paper and wrong in practice.
float FalcomRadialDistance(Texture2D<float4> depthTex, SamplerState samp,
                           float2 screenUV, float2 unpack)
{
  const float deviceDepth = depthTex.SampleLevel(samp, screenUV, 0).x;
  if (!isfinite(deviceDepth)) return 1.0e9;
  const float4 clip = float4(screenUV.x * 2.0 - 1.0, 1.0 - screenUV.y * 2.0, deviceDepth, 1.0);
  const float4 worldH = mul(clip, viewProjInv_g);
  if (!isfinite(worldH.w) || abs(worldH.w) <= 1e-6) return 1.0e9;
  return length(worldH.xyz / worldH.w - FalcomCameraWorldPosition());
}

// 1 inside the CSM's coverage, falling to 0 across the last 15% of the range so the
// boundary does not draw a line across the scene. A range of 0 means "unknown" and
// passes through, which is the correct behaviour for a title whose split we could
// not read: gating on a wrong number would be worse than not gating.
float FalcomSunRangeFade(float radialDist, float rangeMax)
{
  if (rangeMax <= 0.0) return 1.0;
  return saturate((rangeMax - radialDist) / max(rangeMax * 0.15, 1.0));
}

// ── Contact Shadows: applied to the SUN'S OWN contribution ──
//
// Call this at the point where the sun diffuse is multiplied by the light colour
// and BEFORE ambient/GI is added. All three games share that shape:
//
//   Kai      r8.xyz = r8.xyz * lightColor_g.xyz + r12.xyz;
//   Sora 2nd r9.xyz = r9.xyz * lightColor_g.xyz + r13.xyz;
//   Sora 1st r7.xyz = r7.xyz * lightColor_g.xyz + r11.xyz;
//
// so each call site is a one-line change that scales the sun and leaves the
// ambient term untouched.
//
// `sunVis` is the CSM visibility already in a register at every one of those sites
// (Sora 1st r0.w, Sora 2nd r3.z, Kai and Kai soft r8.w); each was checked to be
// unwritten between its last read and the call. `csmRangeMax` is that game's
// outermost cascade split. Both gates are applied HERE rather than at the call
// sites so that all four stay identical and none can silently lose one.
//
// The gate is the whole point of this function. An earlier version applied contact
// only to the sun term and relied on the sun term being ~0 indoors, which it is
// not: the engine's directional term stays non-zero wherever the CSM fails to
// report occlusion, and indoors that is most surfaces. Scaling it by the contact
// term then darkened interiors. Gating on real sun visibility is what makes the
// multiply a no-op where there is no sun.
float3 FalcomApplyContactToSun(float3 sunDiffuse, bool isCharacter, float4 contact,
                               float sunVis, float csmRangeMax,
                               Texture2D<float4> depthTex, SamplerState pointSampler,
                               float2 screenUV)
{
  if (shader_injection_data.cs_contact_dedicated_bound < 0.5f) return sunDiffuse;

  float gate = saturate(sunVis);
  const float rangeMax = shader_injection_data.cs_contact_sun_range > 0.0
      ? shader_injection_data.cs_contact_sun_range : csmRangeMax;
  if (rangeMax > 0.0) {
    gate *= FalcomSunRangeFade(
        FalcomRadialDistance(depthTex, pointSampler, screenUV, FalcomDepthUnpackConsts()),
        rangeMax);
  }

  // lerp toward a fully lit term, so a gated pixel is a bit-exact no-op rather than
  // a multiply by zero that could still perturb a zero-valued register.
  contact.x = lerp(1.0, contact.x, gate);

  return sunDiffuse * FalcomShadowStrength(
      contact.x, shader_injection_data.cs_contact_strength, isCharacter,
      shader_injection_data.cs_contact_env_strength,
      shader_injection_data.cs_contact_char_strength,
      shader_injection_data.cs_contact_max_darkening);
}

// ── Diagnostic views + micro, for the final output site ──
//
// Runs at the end of the lighting shader, after every contribution is summed.
// The views override the colour outright so a broken pass can never hide behind a
// strength of 0, and each mode maps a diagnostic code to a colour that cannot be
// confused with any other, so one screenshot identifies the failing stage.
//
// The term textures are float4 rather than float for the same reason the passes
// write four channels: a pass that bails early, a pass whose inputs never bound
// and a pass that legitimately finds nothing all produce .x = 1.0, and only .y
// tells them apart. Without that, "the effect does nothing" and "the effect is
// correctly reporting no occlusion" are the same observation.
//
// SamplerState is a parameter because no new pixel-stage sampler may be bound:
// pushing one to the pixel stage is documented as causing heap corruption (see
// the IS-FAST push in OnBeforeLightingShaderDraw), so this reuses the game's own
// samPoint_s at s0.
float3 FalcomApplyShadowTerms(float3 lit, float2 screenUV, bool isCharacter,
                              SamplerState pointSampler,
                              Texture2D<float4> microTex, Texture2D<float4> contactTex)
{
  const bool micro_on = shader_injection_data.cs_micro_dedicated_bound > 0.5f;
  const bool contact_on = shader_injection_data.cs_contact_dedicated_bound > 0.5f;

  const float4 micro = micro_on ? microTex.SampleLevel(pointSampler, screenUV, 0) : float4(1, 0, 1, 1);
  const float4 contact = contact_on ? contactTex.SampleLevel(pointSampler, screenUV, 0) : float4(1, 0, 1, 1);

  // ---- diagnostic views ----
  const int contactDbg = (int)shader_injection_data.cs_contact_debug;
  if (contactDbg > 0 && contact_on) {
    if (contactDbg == 1) {
      // Stage ladder. See the kContactDiag* block above.
      const uint code = (uint)(contact.y * 255.0 + 0.5);
      if (contact.w < 0.5)        return float3(0.15, 0.15, 0.15);  // never written
      if (code == kContactDiagNoNormal)      return float3(1.0, 0.0, 1.0);  // magenta
      if (code == kContactDiagBadProjection) return float3(1.0, 1.0, 0.0);  // yellow
      if (code == kContactDiagSky)           return float3(0.0, 1.0, 1.0);  // cyan
      if (code == kContactDiagBadDepth)      return float3(1.0, 0.5, 0.0);  // orange
      if (code == kContactDiagNoLightDir)    return float3(0.0, 0.0, 1.0);  // blue
      if (contact.x < 0.5) return float3(1.0, 0.0, 0.0);                      // red = occluded
      return float3(contact.x, contact.x, contact.x);
    }
    if (contactDbg == 2) {
      // Linearised scene depth, 0..1 over 1000 world units. A uniformly white
      // frame means the linearisation is wrong; a uniformly black frame means it
      // is returning 0 for everything.
      return float3(contact.z, contact.z, contact.z);
    }
  }
  const int microDbg = (int)shader_injection_data.cs_micro_debug;
  if (microDbg > 0 && micro_on) {
    if (microDbg == 1) {
      // The occlusion term that was actually read. White means AO = 1, which
      // makes the micro term identically 1 at every pixel -- the pass is running
      // but cannot produce a shadow.
      return float3(micro.z, micro.z, micro.z);
    }
    if (microDbg == 2) {
      const uint code = (uint)(micro.y * 255.0 + 0.5);
      if (micro.w < 0.5)      return float3(0.15, 0.15, 0.15);
      if (code == kMicroDiagNoNormal)   return float3(1.0, 0.0, 1.0);
      if (code == kMicroDiagNoLightDir) return float3(0.0, 1.0, 1.0);
      return float3(micro.x, micro.x, micro.x);
    }
  }
  if (shader_injection_data.cs_contact_debug > 2.5f) {
    // Character mask, independent of either pass: the per-game flag-bit
    // convention is the one input here that no other shader in the repo can
    // corroborate for every game, and an all-one-colour frame is the unambiguous
    // symptom of getting it wrong.
    return isCharacter ? float3(1.0, 0.0, 1.0) : float3(0.0, 0.0, 0.0);
  }

  // Micro only. Contact was applied to the sun at its own composite site.
  return FalcomApplyMicroShadows(lit, isCharacter, micro);
}

// ── Local (point / spot) contact shadows ──
// The compute pass covers the sun once per pixel. Local lights are a different
// problem: they are evaluated inside the dynamic light loop, once per light, so
// the same march has to run there. That is also where it is worth the most --
// local lights in this engine mostly have shadow casting disabled, so the march
// is the only shadow detail they get.
struct FalcomContactParams
{
  float rayLength;    // world units toward the light
  float thickness;    // assumed occluder thickness
  float bias;         // minimum penetration before a hit counts
  float sampleCount;  // march steps
  float strength;     // blend of the resulting term
};

// Returns the light's colour scaled by its own contact shadow.
//
// Scaling the colour rather than a later attenuation term is deliberate: the
// shader applies that colour to both the diffuse and the specular lobe, and a
// shadow has to affect both. Anything applied to only one of them would make the
// shadowed surface change hue as the light's roughness moved.
//
// The direction is derived HERE from the light position rather than taken from
// the caller. Every one of these call sites sits in decompiled code where the
// register holding the normalised direction differs per loop and per game, and
// picking the wrong one would silently march along an unrelated axis. The light
// position and the world position are the two values every loop states
// explicitly (`lightPos - worldPos` is how NdotL is computed in all of them), so
// taking only those makes the call site immune to that.
//
// `lightPos == worldPos` and a degenerate projection both return the colour
// unchanged, so a light at the shading point cannot take its own contribution
// with it.
float3 FalcomApplyLocalContactShadow(float3 lightColor, float3 lightPos, float3 worldPos,
                                     FalcomContactParams p,
                                     float2 unpack, float jitter,
                                     Texture2D<float4> depthTex, SamplerState pointSampler)
{
  float3 toLight = lightPos - worldPos;
  const float len2 = dot(toLight, toLight);
  if (len2 < 1e-8) return lightColor;
  toLight *= rsqrt(len2);

  const float4 clipA = mul(float4(worldPos, 1.0), viewProj_g);
  const float4 clipB = mul(float4(worldPos + toLight * p.rayLength, 1.0), viewProj_g);
  if (abs(clipA.w) <= 1e-6 || abs(clipB.w) <= 1e-6) return lightColor;
  const float term = FalcomContactShadowMarch(clipA, clipB, unpack, depthTex, pointSampler,
                                              p.thickness, p.bias, p.sampleCount, jitter,
                                              shader_injection_data.cs_contact_response_scale);
  return lightColor * lerp(1.0, term, saturate(p.strength));
}

#endif  // SRC_GAMES_FALCOMENGINE_PLUS_SHADOWS_SHADOWS_COMMON_HLSLI_
