// ---- Created with 3Dmigoto v1.4.1 on Sat Feb 21 18:39:08 2026

cbuffer cb_scene : register(b0)
{
  float4x4 view_g : packoffset(c0);
  float4x4 viewInv_g : packoffset(c4);
  float4x4 proj_g : packoffset(c8);
  float4x4 projInv_g : packoffset(c12);
  float4x4 viewProj_g : packoffset(c16);
  float4x4 viewProjInv_g : packoffset(c20);
  float2 vpSize_g : packoffset(c24);
  float2 invVPSize_g : packoffset(c24.z);
  float3 lightColor_g : packoffset(c25);
  float ldotvXZ_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float gameTime_g : packoffset(c26.w);
  float3 sceneShadowColor_g : packoffset(c27);
  float chrLightIntensity_g : packoffset(c27.w);
  float3 windDirection_g : packoffset(c28);
  float sceneTime_g : packoffset(c28.w);
  float2 lightTileSizeInv_g : packoffset(c29);
  float fogNearDistance_g : packoffset(c29.z);
  float fogFadeRangeInv_g : packoffset(c29.w);
  float3 fogColor_g : packoffset(c30);
  float fogIntensity_g : packoffset(c30.w);
  float fogHeight_g : packoffset(c31);
  float fogHeightRangeInv_g : packoffset(c31.y);
  float windWaveTime_g : packoffset(c31.z);
  float windWaveFrequency_g : packoffset(c31.w);
  uint localLightProbeCount_g : packoffset(c32);
  float lightSpecularGlossiness_g : packoffset(c32.y);
  float lightSpecularIntensity_g : packoffset(c32.z);
  float disableMapObjNearFade_g : packoffset(c32.w);
  float4x4 ditherMtx_g : packoffset(c33);
  float4 lightProbe_g[9] : packoffset(c37);
  float3 chrLightDir_g : packoffset(c46);
  float windForce_g : packoffset(c46.w);
  float4 mapColor_g : packoffset(c47);
  float4 clipPlane_g : packoffset(c48);
  float2 resolutionScaling_g : packoffset(c49);
  float2 shadowSplitDistance_g : packoffset(c49.z);
  float4x4 shadowMtx_g[3] : packoffset(c50);
  float shadowFadeNear_g : packoffset(c62);
  float shadowFadeRangeInv_g : packoffset(c62.y);
  float2 invShadowSize_g : packoffset(c62.z);
  float4 frustumPlanes_g[6] : packoffset(c63);
  float4x4 prevView_g : packoffset(c69);
  float4x4 prevViewInv_g : packoffset(c73);
  float4x4 prevProj_g : packoffset(c77);
  float4x4 prevProjInv_g : packoffset(c81);
  float4x4 prevViewProj_g : packoffset(c85);
  float4x4 prevViewProjInv_g : packoffset(c89);
  float2 motionJitterOffset_g : packoffset(c93);
  float2 curJitterOffset_g : packoffset(c93.z);
  float prevSceneTime_g : packoffset(c94);
  uint enableMotionVectors_g : packoffset(c94.y);
  float prevWindWaveTime_g : packoffset(c94.z);
  float padding : packoffset(c94.w);
}

cbuffer cb_local : register(b2)
{
  int2 textureSize_g : packoffset(c0);
  float2 aoSize_g : packoffset(c0.z);
  float charaAOStrength_g : packoffset(c1);
  float charaAOCutOff_g : packoffset(c1.y);
  float mapAORadius_g : packoffset(c1.z);
  float mapAOFadeBeginDistance_g : packoffset(c1.w);
  float mapAOFadeRangeInv_g : packoffset(c2);
  float mapAOLimit_g : packoffset(c2.y);
  float mapAOBias_g : packoffset(c2.z);
  float mapAOIntensity_g : packoffset(c2.w);
  float2 texelSize_g : packoffset(c3);
  float2 prevAoScaling_g : packoffset(c3.z);
  float4 sphere_g[16] : packoffset(c4);
}

cbuffer cb_local2 : register(b3)
{
  float3 rayMarchShadowDir_g : packoffset(c0);
}

SamplerState samPoint_s : register(s0);
SamplerState samLinear_s : register(s1);
Texture2D<float4> depthTexture : register(t0);
Texture2D<uint4> mrtTexture0 : register(t2);
Texture2D<float4> prevTexture : register(t3);
Texture2D<float4> microShadowTex : register(t33);      // Micro Shadows term + diagnostics (rgba8_unorm)
Texture2D<float4> contactShadowTex : register(t34);    // Contact Shadows term + diagnostics (rgba8_unorm)
Texture3D<float2> csIsfastNoise : register(t35);       // IS-FAST volume, custom camera pass dither

#include "../../shared.h"
#include "../../shadows/shadows_common.hlsli"
#include "../../reference/rendering.hlsl"

// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  // Custom camera pass diagnostics, written inside the character branch and read by
  // the debug block at the very end of main. Declared at function scope because that
  // block is outside the branch; defaulted to lit/unmeasured so mode 0, mode 1 and
  // every early-out in the march report "nothing" rather than a value never computed.
  float charCamRawTerm = 1.0f;
  float charCamAxisLen = 0.0f;
  float charCamValid = 0.0f;
  // The reconstructed VIEW position, snapshotted at the projInv divide below. The
  // vanilla march below turns it into a world position in r3.xyz, but r3 is
  // sphere-loop scratch by the time control reaches the custom pass, so that value is
  // only readable from inside the branch that wrote it. The custom pass therefore
  // takes its own copy and does the same world transform itself.
  float3 charViewPos = float3(0.0, 0.0, 0.0);

  r0.z = depthTexture.SampleLevel(samPoint_s, v1.xy, 0).x;
  mrtTexture0.GetDimensions(0, fDest.x, fDest.y, fDest.z);
  r1.xy = fDest.xy;
  r1.xy = v1.xy * r1.xy + float2(-0.5,-0.5);
  r1.xy = max(float2(0,0), r1.xy);
  int2 mrt0_coord = (int2)r1.xy;
  uint4 mrt0_raw = mrtTexture0.Load(int3(mrt0_coord, 0));
  r1.xy = mrt0_raw.xy;
  uint mrt0_z_raw = mrt0_raw.z;  // Save for SSS target flag extraction
  const uint kFoliageMarkerBit = 0x80000000u;
  uint mrt0_z_class = mrt0_z_raw & ~kFoliageMarkerBit;
  r1.z = (float)((mrt0_z_class >> 8u) & 1u);
  if (r1.z != 0) {
    r0.xy = v1.zw * float2(2,-2) + float2(-1,1);
    r0.w = 1;
    r2.x = dot(r0.xyzw, projInv_g._m00_m10_m20_m30);
    r2.y = dot(r0.xyzw, projInv_g._m01_m11_m21_m31);
    r2.z = dot(r0.xyzw, projInv_g._m02_m12_m22_m32);
    r1.z = dot(r0.xyzw, projInv_g._m03_m13_m23_m33);
    r2.xyz = r2.xyz / r1.zzz;
    charViewPos = r2.xyz;
    r1.xy = (uint2)r1.xy;
    r1.zw = r1.xy * float2(3.05180438e-05,3.05180438e-05) + float2(-1,-1);
    r1.z = 3.14159274 * r1.z;
    sincos(r1.z, r3.x, r4.x);
    r1.z = -r1.w * r1.w + 1;
    r1.z = sqrt(r1.z);
    r1.x = r4.x * r1.z;
    r1.y = r3.x * r1.z;
    r1.z = dot(v1.zw, float2(12.9898005,78.2330017));
    r1.z = sin(r1.z);
    r1.z = 43758.5469 * r1.z;
    r1.z = frac(r1.z);
    r3.xyz = float3(5.39830017,5.44269991,6.93709993) * r1.zzz;
    r3.xyz = frac(r3.xyz);
    r4.xyz = float3(21.5351009,14.3136997,15.3219004) + r3.xyz;
    r1.z = dot(r3.yzx, r4.xyz);
    r3.xyz = r3.xyz + r1.zzz;
    r3.xyz = r3.xxy * r3.zyz;
    r3.xyz = float3(95.4337006,97.5970001,93.8365021) * r3.xyz;
    r3.xyz = frac(r3.xyz);
    r3.xyz = r3.xyz * float3(2,2,2) + float3(-1,-1,-1);
    r1.z = dot(r3.xyz, r3.xyz);
    r1.z = rsqrt(r1.z);
    r3.xyz = r3.xyz * r1.zzz;
    r1.z = charaAOStrength_g + -charaAOCutOff_g;
    r1.z = 1 / r1.z;
    r4.y = 1;
    r3.w = 0;
    r4.z = 0;
    while (true) {
      r4.w = cmp((int)r4.z >= 10);
      if (r4.w != 0) break;
      r4.w = dot(sphere_g[r4.z].xyz, r3.xyz);
      r4.w = r4.w + r4.w;
      r5.xyz = r3.xyz * -r4.www + sphere_g[r4.z].xyz;
      r4.w = dot(r5.xyz, r1.xyw);
      r5.z = cmp(0 < r4.w);
      r4.w = cmp(r4.w < 0);
      r4.w = (int)-r5.z + (int)r4.w;
      r4.w = (int)r4.w;
      r5.xy = r4.ww * r5.xy;
      r5.xy = aoSize_g.xy * r5.xy;
      r5.xy = r5.xy * float2(1,-1) + v1.zw;
      r5.xy = resolutionScaling_g.xy * r5.xy;
      r4.x = depthTexture.SampleLevel(samPoint_s, r5.xy, 0).x;
      r4.w = dot(projInv_g._m22_m32, r4.xy);
      r4.x = dot(projInv_g._m23_m33, r4.xy);
      r4.x = r4.w / r4.x;
      r4.x = r4.x + -r2.z;
      r4.w = cmp(r4.x >= charaAOCutOff_g);
      r4.w = r4.w ? 1.000000 : 0;
      r4.x = -charaAOCutOff_g + r4.x;
      r4.x = saturate(r4.x * r1.z);
      r5.x = r4.x * -2 + 3;
      r4.x = r4.x * r4.x;
      r4.x = -r5.x * r4.x + 1;
      r3.w = r4.w * r4.x + r3.w;
      r4.z = (int)r4.z + 1;
    }
    r1.z = 0.100000001 * r3.w;
    // 0 = off, 1 = the engine's own camera-facing march, 2 = our custom
    // camera-facing contact march. Modes 1 and 2 write the same channel and are
    // consumed by the same `char_shadow_mode >= 0.5f` gate in the lighting shader, so
    // nothing downstream has to know which one produced the value.
    int charShadowMode = clamp((int)round(shader_injection_data.char_shadow_mode), 0, 2);

    if (charShadowMode == 1) {
      // Vanilla: game's native 10-step ray-march camera shadow
      r2.w = 1;
      r3.x = dot(r2.xyzw, viewInv_g._m00_m10_m20_m30);
      r3.y = dot(r2.xyzw, viewInv_g._m01_m11_m21_m31);
      r3.z = dot(r2.xyzw, viewInv_g._m02_m12_m22_m32);
      r2.x = viewInv_g._m20;
      r2.y = viewInv_g._m21;
      r2.z = viewInv_g._m22;
      r2.x = dot(r1.xyw, r2.xyz);
      r2.xy = abs(r2.xx) * float2(-0.00249999994,0.00150000001) + float2(0.00749999983,0.000500000024);
      r1.xyw = r1.xyw * r2.xxx + r3.xyz;
      r2.xyz = rayMarchShadowDir_g.xyz * r2.yyy;
      r3.w = 1;
      r2.w = 0;
      r4.x = 0;
      while (true) {
        r4.y = cmp((int)r4.x >= 10);
        if (r4.y != 0) break;
        r4.y = (int)r4.x;
        r3.xyz = r2.xyz * r4.yyy + r1.xyw;
        r5.x = dot(r3.xyzw, viewProj_g._m00_m10_m20_m30);
        r5.y = dot(r3.xyzw, viewProj_g._m01_m11_m21_m31);
        r5.z = dot(r3.xyzw, viewProj_g._m02_m12_m22_m32);
        r3.x = dot(r3.xyzw, viewProj_g._m03_m13_m23_m33);
        r3.xyz = r5.xyz / r3.xxx;
        r5.xy = r3.xy * float2(0.5,0.5) + float2(0.5,0.5);
        r5.z = 1 + -r5.y;
        r3.xy = resolutionScaling_g.xy * r5.xz;
        r3.x = depthTexture.SampleLevel(samPoint_s, r3.xy, 0).x;
        r3.x = r3.x + -r3.z;
        r3.x = saturate(400000 * r3.x);
        r2.w = r3.x * 0.25 + r2.w;
        r4.x = (int)r4.x + 1;
      }
      r1.x = 1 + -r2.w;
      r1.x = max(0, r1.x);
    } else if (charShadowMode == 2 && shader_injection_data.char_cam_enabled >= 0.5f) {
      // Custom camera-facing contact march: the same family of shadow the vanilla
      // block above produces -- same axis, same normal lift, same output channel --
      // but through the shared clip-space marcher, so the sample count, response
      // scale, thickness, bias and IS-FAST dither are tunable instead of being
      // engine constants.
      //
      // Two register facts make this readable rather than a rewrite: r1.xyw is the
      // world-space MRT normal the vanilla branch also marches from, and charViewPos
      // is the view position that branch starts from. The world transform below is the
      // same one the vanilla branch performs on r3.xyz -- a register it writes itself,
      // and loop scratch besides, so it is not readable from here.
      const float3 camAxis = FalcomCameraFacingAxis(rayMarchShadowDir_g);
      const float3 charNormalWS = FalcomSafeNormalize3(r1.xyw, float3(0.0, 0.0, 0.0));
      charCamAxisLen = length(rayMarchShadowDir_g);
      if (FalcomNormalValid(camAxis) && FalcomNormalValid(charNormalWS)) {
        const float3 charPosWS = mul(float4(charViewPos, 1.0), viewInv_g).xyz
            + charNormalWS * max(0.0, shader_injection_data.char_cam_normal_bias);
        const float4 clipA = mul(float4(charPosWS, 1.0), viewProj_g);
        const float4 clipB = mul(float4(
            charPosWS + camAxis * max(0.0, shader_injection_data.char_cam_ray_length), 1.0),
            viewProj_g);
        if (abs(clipA.w) > 1e-6 && abs(clipB.w) > 1e-6) {
          const float camJitter = shader_injection_data.char_cam_isfast_enabled > 0.5f
              ? FalcomSampleISFAST(csIsfastNoise, samPoint_s, uint2(v0.xy),
                                   (uint)max(shader_injection_data.cs_noise_frame, 0.0f),
                                   128.0, 32.0,
                                   shader_injection_data.shadow_isfast_spatial_scale,
                                   shader_injection_data.shadow_isfast_temporal_speed,
                                   shader_injection_data.shadow_isfast_seed_offset,
                                   shader_injection_data.shadow_isfast_texture_loaded)
              : 0.5f;
          charCamRawTerm = FalcomContactShadowMarch(
              clipA, clipB, FalcomDepthUnpackConsts(), depthTexture, samPoint_s,
              max(0.0, shader_injection_data.char_cam_thickness),
              max(0.0, shader_injection_data.char_cam_bias),
              max(1.0, floor(shader_injection_data.char_cam_sample_count + 0.5)),
              camJitter,
              max(0.0, shader_injection_data.char_cam_response_scale),
              resolutionScaling_g.xy);
          charCamValid = 1.0f;
          r1.x = FalcomShadowStrength(
              charCamRawTerm, shader_injection_data.char_cam_strength,
              /*isCharacter=*/true, 1.0, 1.0,
              shader_injection_data.char_cam_max_darkening);
        } else {
          r1.x = 1;
        }
      } else {
        r1.x = 1;
      }
    } else {
      // Off, and Custom with the camera pass switched off. The environment
      // screen-space shadow raymarchs that used to run here are replaced by the
      // Contact / Micro Shadow compute passes, which the lighting shader reads at
      // t33/t34. This pass therefore no longer writes a shadow into the .z channel
      // on this path; Custom writes 1.0, which the lighting shader's
      // `char_shadow_mode >= 0.5f` gate still multiplies in as a no-op.
      r1.x = 1;
    }

    r1.yw = float2(0.5,0.5) + -v1.zw;
    r1.y = max(abs(r1.y), abs(r1.w));
    r1.y = -0.449999988 + r1.y;
    r1.y = max(0, r1.y);
    r1.y = 20 * r1.y;
    r1.w = 1 + -r1.x;
    r2.y = r1.y * r1.w + r1.x;
  } else {
    r1.z = 0;
    // The Env/foliage SSS raymarch that used to run here is replaced by the same
    // compute terms, so the shadow channel this pass handed to the lighting
    // shader is no longer written.
    r2.y = 1;
  }
  r2.x = 1 + -r1.z;
  r0.xy = v1.zw * float2(2,-2) + float2(-1,1);
  r0.w = 1;
  r1.x = dot(r0.xyzw, viewProjInv_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, viewProjInv_g._m01_m11_m21_m31);
  r1.z = dot(r0.xyzw, viewProjInv_g._m02_m12_m22_m32);
  r1.w = dot(r0.xyzw, viewProjInv_g._m03_m13_m23_m33);
  r0.xyzw = r1.xyzw / r1.wwww;
  r1.x = dot(r0.xyzw, prevViewProj_g._m00_m10_m20_m30);
  r1.y = dot(r0.xyzw, prevViewProj_g._m01_m11_m21_m31);
  r0.x = dot(r0.xyzw, prevViewProj_g._m03_m13_m23_m33);
  r0.xy = r1.xy / r0.xx;
  r0.xy = r0.xy * float2(0.5,0.5) + float2(0.5,0.5);
  r0.z = 1 + -r0.y;
  r0.xy = prevAoScaling_g.xy * r0.xz;
  r0.xy = resolutionScaling_g.xy * r0.xy;
  r0.xyz = prevTexture.SampleLevel(samLinear_s, r0.xy, 0).xyz;
  r2.z = 1;
  r1.xyz = r2.zxy + -r0.xyz;
  // The character pass output is the character AO/history blend, not a sun-lit
  // colour: there is no lightColor_g multiply and no ambient add to separate, so
  // contact has no sun term to attach to here. Micro still applies -- it is an
  // AO-driven aperture term and belongs on the resolved colour. This is also the
  // pass that writes the character shadow into the AO target's .z, which the
  // lighting shader's "Character Shadowing -> Mode" toggle consumes.
  o0.xyz = FalcomApplyShadowTerms(r1.xyz * float3(0.25,0.25,1) + r0.xyz, v1.zw,
                                  ((mrt0_z_class >> 8u) & 1u) != 0u, samPoint_s,
                                  microShadowTex, contactShadowTex);
  o0.w = 1;

  // --- Foliage / character mask debug in char shader ---
  int foliage_dbg_char = (int)shader_injection_data.foliage_debug_mode;
  bool dbg_is_foliage = (mrt0_z_class == 2303u || mrt0_z_class == 3327u);
  if (foliage_dbg_char == 1) {
    // Mask: green = foliage, red dim = character, black = other
    bool dbg_is_char = (((mrt0_z_class >> 8u) & 1u) != 0u);
    o0.xyz = dbg_is_foliage ? float3(0, 1, 0)
           : dbg_is_char    ? float3(0.3, 0, 0)
           :                  float3(0, 0, 0);
    o0.w = 1;
  } else if (foliage_dbg_char == 2) {
    // Was the SSS shadow channel. Now shows the Contact Shadow term, which is
    // what replaced it: white = lit, black = occluded.
    o0.xyz = dbg_is_foliage
        ? ((shader_injection_data.cs_contact_dedicated_bound >= 0.5)
               ? contactShadowTex.SampleLevel(samPoint_s, v1.zw, 0).xxx
               : float3(1.0, 1.0, 1.0))
        : float3(0.1, 0.1, 0.1);
    o0.w = 1;
  } else if (foliage_dbg_char == 6) {
    // Vanilla value detect (same as lighting debug)
    uint raw_z = mrt0_z_class;
    if (raw_z == 2303u)
      o0.xyz = float3(0, 1, 0);
    else if (raw_z == 3327u)
      o0.xyz = float3(0, 1, 1);
    else
      o0.xyz = float3(raw_z / 4096.0, 0, 0);
    o0.w = 1;
  }
  // --- End SSS Debug ---

  // --- Character Shadowing: custom camera pass debug ---
  // Placed after the block above so it wins: it is the narrower view of the two and
  // the one the user turned on deliberately. Only character pixels are touched --
  // this pass also writes the environment AO, and painting that with a
  // character-only diagnostic reads as a broken frame rather than as a debug view.
  float3 charCamDbgColour;
  if (((mrt0_z_class >> 8u) & 1u) != 0u
      && FalcomCharCamDebugView(charCamRawTerm, charCamAxisLen, charCamValid,
                                (int)shader_injection_data.char_cam_debug,
                                shader_injection_data.char_cam_response_scale,
                                charCamDbgColour)) {
    o0.xyz = charCamDbgColour;
    o0.w = 1;
  }
  // --- End Character Shadowing Debug ---

  return;
}
