// ── falcomengine-plus Local (point / spot) Contact Shadows ──
//
// One thread per depth texel. For the point and spot lamps of the pixel's light tile cell, keeps the strongest lamps
// (diffuse weight, as world/shaders/world_rtls.cs_5_0.hlsl weighs them), runs the shared contact march toward each and
// writes, per kind, the diffuse-weighted share of the kind's lamp light the march blocks. Lamps past the Local Light
// Budget count as unblocked. The lighting shaders scale every lamp of a kind by 1 - Local Strength x share (t40).
//
// Only the kinds the pixel's material path uses are marched. The path is the lighting shaders' own test, bit 2 of the
// pixel's DeferredParam.flag: set is the specular path (Point / Spot), clear the diffuse-only path (Env Point / Env
// Spot). A pixel with neither kind on writes kContactDiagNoLocalKind and marches nothing.
//
// The origin is lifted along the MRT normal by Local Normal Bias, and along the negated normal for a lamp behind the
// surface (lit through translucency), as RT Local Shadows does.
//
// Runs on Sora 2nd only (IsSora2nd in LocalContactComputeRunning, addon.cpp). It reads lightTileSizeInv_g at cb_scene c29
// and LightIndexData.tileDepthInv, which Sora 1st and Kai lay out the same way. Kyoto keeps the inline march in its lighting shader.
//
// Bindings (root param 2 / 3 of the host layout):
//   t0 depth        scene depth, reversed-Z, device Z in .x
//   t1 mrtTexture0  world normal
//   t2 IS-FAST      128x128x32 blue noise volume
//   t3 lamps        the game's dynamicLights_g (LightParam)
//   t4 light cells  the game's lightIndices_g (LightIndexData)
//   t5 material     the game's material index texture, index in .x (Sora mrtTexture2, Kai mrtTexture1)
//   t6 materials    the game's deferredParams_g (DeferredParam, 144 bytes)
//   u0 output       rgba8_unorm: .x point share, .y diagnostic code / 255, .z spot share, .w 1 = written

#include "../world/shaders/world_rt_lamps.hlsli"

// The cb_scene subset of shadows_common.hlsli plus lightTileSizeInv_g, which Sora 1st, Sora 2nd and Kai declare at c29.
cbuffer cb_scene : register(b0)
{
  float4x4 view_g            : packoffset(c0);
  float4x4 viewInv_g         : packoffset(c4);
  float4x4 proj_g            : packoffset(c8);
  float4x4 projInv_g         : packoffset(c12);
  float4x4 viewProj_g        : packoffset(c16);
  float4x4 viewProjInv_g     : packoffset(c20);
  float2 vpSize_g            : packoffset(c24);
  float2 invVPSize_g         : packoffset(c24.z);
  float3 lightColor_g        : packoffset(c25);
  float3 lightDirection_g    : packoffset(c26);
  float2 lightTileSizeInv_g  : packoffset(c29);
};

#include "shadows_common.hlsli"

Texture2D<float4> g_srcDepth                    : register(t0);
Texture2D<uint4>  g_srcMrtNormal                : register(t1);
Texture3D<float2> g_isfastNoise                 : register(t2);
StructuredBuffer<LightParam> g_lamps            : register(t3);
StructuredBuffer<LightIndexData> g_lampCells    : register(t4);
Texture2D<uint2>  g_srcMaterialIndex            : register(t5);
StructuredBuffer<GameDeferredParam> g_materials : register(t6);
SamplerState g_samplerPointClamp                : register(s0);

RWTexture2D<float4> g_outLocal : register(u0);

// Must equal the Local Light Budget maximum (ContactShadowsLocalMaxLights in addon.cpp).
#define kLocalContactMaxLamps 16u
#define kLocalContactSpotBit 0x80000000u

[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  if (p.x >= (uint)shader_injection_data.cs_working_w) return;
  if (p.y >= (uint)shader_injection_data.cs_working_h) return;

  const float2 unpack = FalcomDepthUnpackConsts();
  const float deviceDepth = g_srcDepth.Load(int3(p, 0)).x;
  if (!isfinite(deviceDepth) || !isfinite(unpack.x) || !isfinite(unpack.y) || abs(unpack.x) < 1e-8) {
    g_outLocal[p] = float4(0.0, kContactDiagBadDepth / 255.0, 0.0, 1.0);
    return;
  }
  if (FalcomLinearDepth(deviceDepth, unpack) > shader_injection_data.cs_contact_sky_depth) {
    g_outLocal[p] = float4(0.0, kContactDiagSky / 255.0, 0.0, 1.0);
    return;
  }
  const float3 N = DecodeFalcomMrtNormal(g_srcMrtNormal.Load(int3(p, 0)).xy);
  if (!FalcomNormalValid(N)) {
    g_outLocal[p] = float4(0.0, kContactDiagNoNormal / 255.0, 0.0, 1.0);
    return;
  }
  const float2 pixelCentre = float2(p) + 0.5;
  const float2 uv = pixelCentre / max(float2(shader_injection_data.cs_working_w, shader_injection_data.cs_working_h), 1.0.xx);

  // Material path: the lighting shaders' own test (sora2nd lighting_0xCA3D8596 lines 262, 771, 804 and 849): the texel
  // is uv x the material texture size, truncated, and the index is clamped to 0x4E1F, both as they do it.
  uint materialW, materialH;
  g_srcMaterialIndex.GetDimensions(materialW, materialH);
  const uint2 materialTexel = min(uint2(uv * float2(materialW, materialH)), uint2(materialW, materialH) - 1u);
  const uint materialIndex = min(0x4E1Fu, g_srcMaterialIndex.Load(int3(materialTexel, 0)).x);
  const bool specPath = (asuint(g_materials[materialIndex].tail[3].z) & 2u) != 0u;
  const bool pointsOn = (specPath ? shader_injection_data.cs_contact_local_point
                                  : shader_injection_data.cs_contact_local_env_point) > 0.5f;
  const bool spotsOn = (specPath ? shader_injection_data.cs_contact_local_spot
                                 : shader_injection_data.cs_contact_local_env_spot) > 0.5f;
  if (!pointsOn && !spotsOn) {
    g_outLocal[p] = float4(0.0, kContactDiagNoLocalKind / 255.0, 0.0, 1.0);
    return;
  }

  const float4 worldH = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, deviceDepth, 1.0), viewProjInv_g);
  if (abs(worldH.w) <= 1e-6 || !isfinite(worldH.w)) {
    g_outLocal[p] = float4(0.0, kContactDiagBadProjection / 255.0, 0.0, 1.0);
    return;
  }
  const float3 P = worldH.xyz / worldH.w;

  // Cell index: the lighting shaders' expression (sora2nd lighting_0xCA3D8596 lines 812-823), the pixel centre standing
  // in for SV_Position.
  const float viewZ = dot(view_g._m02_m12_m22_m32, float4(P, 1.0));
  const uint2 tile = (uint2)(lightTileSizeInv_g * pixelCentre);
  const uint row = tile.y << 5;
  const uint slice = (uint)clamp(g_lampCells[tile.x + row].tileDepthInv * -viewZ, 0.0, 7.0);
  const uint cell = min(4607u, slice * 576u + row + tile.x);

  uint lampCount, lampStride;
  g_lamps.GetDimensions(lampCount, lampStride);
  const uint listPoints = pointsOn ? min(63u, g_lampCells[cell].pointLightCount) : 0u;
  const uint listSpots = spotsOn ? min(63u, g_lampCells[cell].spotLightCount) : 0u;

  // Candidates: every lamp of the cell that lights P, weighted as the game's loops weight it: luminance of the colour x
  // distance falloff x [cone] x max(NdotL, translucency). The largest kLocalContactMaxLamps are kept in descending order.
  float topWeight[kLocalContactMaxLamps];
  uint topId[kLocalContactMaxLamps];
  [unroll] for (uint s = 0u; s < kLocalContactMaxLamps; ++s) {
    topWeight[s] = 0.0;
    topId[s] = 0u;
  }
  float pointSum = 0.0;
  float spotSum = 0.0;
  uint candidates = 0u;
  [loop] for (uint i = 0u; i < listPoints + listSpots; ++i) {
    const bool spot = i >= listPoints;
    const uint li = spot ? (uint)g_lampCells[cell].spotLightIndices[i - listPoints]
                         : (uint)g_lampCells[cell].pointLightIndices[i];
    if (li >= lampCount) continue;
    const LightParam lamp = g_lamps[li];
    const float3 toLamp = lamp.pos - P;
    if (lamp.radius <= 0.0 || dot(toLamp, toLamp) < 1e-8) continue;
    const float3 dir = normalize(toLamp);
    float att = LampDistanceAttenuation(lamp, toLamp);
    if (spot) {
      att *= LampSpotCone(lamp, dir);
    }
    if (att <= 0.0) continue;
    float w = dot(lamp.color, float3(0.2126, 0.7152, 0.0722)) * att * max(dot(N, dir), lamp.translucency);
    if (w <= 0.0) continue;
    if (spot) {
      spotSum += w;
    } else {
      pointSum += w;
    }
    candidates++;
    uint id = li | (spot ? kLocalContactSpotBit : 0u);
    [unroll] for (uint j = 0u; j < kLocalContactMaxLamps; ++j) {
      if (w > topWeight[j]) {
        const float tw = topWeight[j];
        const uint ti = topId[j];
        topWeight[j] = w;
        topId[j] = id;
        w = tw;
        id = ti;
      }
    }
  }

  FalcomContactParams params;
  params.rayLength = max(0.0, shader_injection_data.cs_contact_local_ray_length);
  params.thickness = max(0.0, shader_injection_data.cs_contact_thickness);
  params.bias = max(0.0, shader_injection_data.cs_contact_bias);
  params.sampleCount = max(1.0, floor(shader_injection_data.cs_contact_local_sample_count + 0.5));
  params.strength = 1.0;
  const float jitter = shader_injection_data.cs_contact_isfast_enabled > 0.5f
      ? FalcomSampleISFAST(g_isfastNoise, g_samplerPointClamp, p,
                           (uint)max(shader_injection_data.cs_noise_frame, 0.0f),
                           128.0, 32.0,
                           shader_injection_data.shadow_isfast_spatial_scale,
                           shader_injection_data.shadow_isfast_temporal_speed,
                           shader_injection_data.shadow_isfast_seed_offset,
                           shader_injection_data.shadow_isfast_texture_loaded)
      : 0.5f;
  const float3 lift = N * max(0.0, shader_injection_data.cs_contact_local_normal_bias);
  const uint budget = clamp((uint)(shader_injection_data.cs_contact_local_max_lights + 0.5), 1u, kLocalContactMaxLamps);

  float pointBlocked = 0.0;
  float spotBlocked = 0.0;
  [loop] for (uint k = 0u; k < min(candidates, budget); ++k) {
    const float3 lampPos = g_lamps[topId[k] & ~kLocalContactSpotBit].pos;
    const float3 origin = P + (dot(N, lampPos - P) > 0.0 ? lift : -lift);
    // A unit colour makes the returned colour the march term itself (1 = unblocked).
    const float blocked = 1.0 - FalcomApplyLocalContactShadow(float3(1.0, 1.0, 1.0), lampPos, origin, params, unpack,
                                                              jitter, g_srcDepth, g_samplerPointClamp).x;
    if ((topId[k] & kLocalContactSpotBit) != 0u) {
      spotBlocked += topWeight[k] * blocked;
    } else {
      pointBlocked += topWeight[k] * blocked;
    }
  }

  g_outLocal[p] = float4(pointSum > 0.0 ? pointBlocked / pointSum : 0.0, kContactDiagMarched / 255.0,
                         spotSum > 0.0 ? spotBlocked / spotSum : 0.0, 1.0);
}
