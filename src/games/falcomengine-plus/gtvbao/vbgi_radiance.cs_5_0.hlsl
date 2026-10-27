// falcomengine-plus VBGI - Radiance prepass (M7.4 step 3).
// One thread per radiance texel. Writes the sender radiance L_j = albedo * (ambient + lights + sun) with the emissive
// term, which the VBGI gather samples at the same texel. Sky writes 0. Multi-bounce (M7.6): last frame's denoised GI, reprojected with the game motion, is added as sender light when g_feedback_w > 0.
// Inputs are the lighting-draw captures (Sora 1st and Sora 2nd): see addon.cpp RunVbgiRadiance.
// Lamps follow the game's lamp tile lists (lightIndices_g) through world_rt_lamps.hlsli; shadow maps and specular are ignored.
// Kai (M7.7): material from mrtTexture1.x, emissive factor from mrtTexture0.z, sun from the game's cascaded shadow maps with saturate(dot(N, -L)); no mrtTexture2.

#include "../include/mrt_normal.hlsli"
#include "../world/shaders/world_rt_lamps.hlsli"
#include "../taa/taa_reproject.hlsli"

// The game's DeferredParam (144 bytes): emissive is head[0].w (byte 12), dynamicLightIntensity is mid.w (byte 76).
struct VbgiDeferredParam
{
  float4 head[4];
  float4 mid;
  float4 tail[4];
};

Texture2D<float4>                   g_color      : register(t0);  // colorTexture (G-buffer albedo)
Texture2D<uint4>                    g_mrt0       : register(t1);  // mrtTexture0 (world normal, Falcom encoding)
Texture2D<uint4>                    g_mrt1       : register(t2);  // mrtTexture1 (sun diffuse factor in .w)
Texture2D<uint2>                    g_mrt2       : register(t3);  // mrtTexture2 (material index in .x, emissive bits in .y)
Texture2D<float4>                   g_depth      : register(t4);  // depthTexture (NDC)
StructuredBuffer<VbgiDeferredParam> g_deferred   : register(t5);  // deferredParams_g
Texture2D<float4>                   g_shadow     : register(t6);  // shadowTexture (Sora 2nd t7, Sora 1st t9)
StructuredBuffer<LightParam>        g_lamps      : register(t7);  // dynamicLights_g
StructuredBuffer<LightIndexData>    g_lamp_tiles : register(t8);  // lightIndices_g
Texture2D<float4>                   g_prev_gi    : register(t9);   // previous frame's denoised VBGI (depth size)
Texture2D<float4>                   g_motion     : register(t10);  // RTV4 motion, texel units
Texture2DArray<float4>              g_csm        : register(t11);  // Kai cascade shadow maps (3 layers)
SamplerState                        g_samp_point : register(s0);

RWTexture2D<float4> g_radiance : register(u0);

cbuffer cb_scene : register(b0)
{
  float4x4 view_g        : packoffset(c0);
  float4x4 viewProjInv_g : packoffset(c20);
  float3 lightColor_g     : packoffset(c25);
  float3 sceneShadowColor_g : packoffset(c27);
  float2 lightTileSizeInv_g : packoffset(c29);
  float4 lightProbe_g[9] : packoffset(c37);
  float4x4 viewInv_g          : packoffset(c4);
  float3 lightDirection_g     : packoffset(c26);
  float2 kai_shadowSplit      : packoffset(c49.z);   // Kai shadowSplitDistance_g
  float4x4 kai_shadowMtx[3]   : packoffset(c50);     // Kai shadowMtx_g
  float kai_shadowFadeNear    : packoffset(c62);
  float kai_shadowFadeRangeInv : packoffset(c62.y);
};

// Push constants (addon.cpp RunVbgiRadiance fills these; the rest of the 48-float range is unused).
cbuffer cb_vbgi_radiance : register(b13)
{
  float g_ambient_w;      // c0
  float g_lights_w;       // c1
  float g_sun_w;          // c2
  float g_emissive_w;     // c3
  float g_radiance_clamp; // c4
  float g_divisor;        // c5 - domain divisor (1 Full, 2 Half)
  float g_feedback_w;     // c6 - Multi-Bounce Strength x Intensity capped at 0.9; 0 = off
  float g_game_profile;   // c7 - 0 Sora 1st, 1 Sora 2nd, 2 Kai
};

uint2 VbgiTexel(float2 uv, uint2 size)
{
  return min(uint2(uv * float2(size)), size - 1u);
}

static const float kKaiPenumbra = 2.0f;

float KaiCascadeVis(uint cascade, float3 P, float split_ref, float jitter)
{
  const float4 P4 = float4(P, 1.0f);
  float4 h;
  h.x = dot(P4, kai_shadowMtx[cascade]._m00_m10_m20_m30);
  h.y = dot(P4, kai_shadowMtx[cascade]._m01_m11_m21_m31);
  h.z = dot(P4, kai_shadowMtx[cascade]._m02_m12_m22_m32);
  h.w = dot(P4, kai_shadowMtx[cascade]._m03_m13_m23_m33);
  const float3 s = h.xyz / h.w;
  if (any(s.xy < 0.0f) || any(s.xy > 1.0f)) return 1.0f;
  uint sw, sh, sl;
  g_csm.GetDimensions(sw, sh, sl);
  const float2 radius = max(1.0f / float2(sw, sh), float2(0.00125f, 0.000625f) * (30.0f / split_ref) * kKaiPenumbra);
  float lit = 0.0f;
  [unroll]
  for (uint i = 0u; i < 8u; ++i) {
    float sn, cs;
    sincos((float)i * 2.4000001f + jitter, sn, cs);
    const float2 uv = s.xy + float2(cs, sn) * sqrt(((float)i + 0.5f) * 0.125f) * radius;
    lit += (s.z <= g_csm.SampleLevel(g_samp_point, float3(uv, (float)cascade), 0).x) ? 1.0f : 0.0f;
  }
  return lit * 0.125f;
}

// The game's cascade selection (kai lighting :1181-1500): radial distance, 5-unit blend regions, distance fade.
float KaiSunVisibility(float3 P, float jitter)
{
  const float dist = length(P - float3(viewInv_g._m30, viewInv_g._m31, viewInv_g._m32));
  float v;
  if (dist > kai_shadowSplit.y - 5.0f) {
    v = KaiCascadeVis(2u, P, kai_shadowSplit.y, jitter);
    if (dist < kai_shadowSplit.y) v = lerp(v, KaiCascadeVis(1u, P, kai_shadowSplit.y, jitter), 0.2f * (kai_shadowSplit.y - dist));
    v = lerp(v, 1.0f, saturate(kai_shadowFadeRangeInv * (dist - kai_shadowFadeNear)));
  } else {
    const bool near_c = dist < kai_shadowSplit.x;
    v = KaiCascadeVis(near_c ? 0u : 1u, P, kai_shadowSplit.x, jitter);
    if (near_c && dist > kai_shadowSplit.x - 5.0f) v = lerp(KaiCascadeVis(1u, P, kai_shadowSplit.x, jitter), v, 0.2f * (kai_shadowSplit.x - dist));
  }
  return v;
}

[numthreads(8, 8, 1)]
void main(uint2 p : SV_DispatchThreadID)
{
  uint ow, oh;
  g_radiance.GetDimensions(ow, oh);
  if (p.x >= ow || p.y >= oh) return;

  // Representative full-res pixel of this texel (block centre, clamped), as the RTGI and GTVBAO passes use.
  uint dw, dh;
  g_depth.GetDimensions(dw, dh);
  const uint d_div = (uint)max(g_divisor, 1.0f);
  const int2 pf = min(int2(p) * (int)d_div + (int)(d_div / 2u), int2(dw, dh) - 1);
  const float2 uv = (float2(pf) + 0.5f) / float2(dw, dh);

  const float depth = g_depth.Load(int3(pf, 0)).x;
  if (depth <= 0.0f) {
    g_radiance[p] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    return;
  }

  uint cw, ch;
  g_color.GetDimensions(cw, ch);
  const float3 albedo = g_color.Load(int3(VbgiTexel(uv, uint2(cw, ch)), 0)).rgb;

  uint m0w, m0h;
  g_mrt0.GetDimensions(m0w, m0h);
  const uint4 mrt0 = g_mrt0.Load(int3(VbgiTexel(uv, uint2(m0w, m0h)), 0));
  const float3 n = DecodeFalcomMrtNormalAsGame(mrt0.xy);

  // World position from the representative pixel's depth (same mapping as world_rtao).
  const float4 world_h = mul(float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, depth, 1.0f), viewProjInv_g);
  const float3 P = world_h.xyz / world_h.w;
  const float view_z = dot(view_g._m02_m12_m22_m32, float4(P, 1.0f));

  // Ambient: the game's SH probe at the sender normal.
  const float3 probe = max(float3(0.0f, 0.0f, 0.0f),
      lightProbe_g[0].xyz + lightProbe_g[1].xyz * n.x + lightProbe_g[2].xyz * n.y + lightProbe_g[3].xyz * n.z
      + lightProbe_g[4].xyz * (n.z * n.x) + lightProbe_g[5].xyz * (n.y * n.z) + lightProbe_g[6].xyz * (n.y * n.x)
      + lightProbe_g[7].xyz * (3.0f * n.z * n.z - 1.0f) + lightProbe_g[8].xyz * (n.x * n.x - n.y * n.y));

  // Game-specific G-buffer decode. Sora: sun byte in mrtTexture1.w, screen shadow mask, material and emissive bits in
  // mrtTexture2. Kai: material in mrtTexture1.x, emissive factor in mrtTexture0.z, cascaded shadow maps, no mrtTexture2.
  const bool is_kai = g_game_profile > 1.5f;
  uint m1w, m1h;
  g_mrt1.GetDimensions(m1w, m1h);
  const uint4 mrt1 = g_mrt1.Load(int3(VbgiTexel(uv, uint2(m1w, m1h)), 0));
  float3 sun;
  uint mat_index;
  float emissive_f;
  if (is_kai) {
    const float jitter = 6.28318548f * frac(52.9829189f * frac(dot(float2(pf), float2(0.06711056f, 0.00583715f))));
    sun = lightColor_g * (1.0f - min(float3(1.0f, 1.0f, 1.0f), sceneShadowColor_g))
        * (saturate(dot(n, -lightDirection_g)) * KaiSunVisibility(P, jitter));
    mat_index = mrt1.x;
    emissive_f = (float)(mrt0.z & 255u) / 255.0f;
  } else {
    const float k = (float)mrt1.w / 255.0f;
    uint sw, sh;
    g_shadow.GetDimensions(sw, sh);
    const float shadow = g_shadow.Load(int3(VbgiTexel(uv, uint2(sw, sh)), 0)).x;
    sun = lightColor_g * (1.0f - min(float3(1.0f, 1.0f, 1.0f), sceneShadowColor_g)) * (k * shadow);
    uint m2w, m2h;
    g_mrt2.GetDimensions(m2w, m2h);
    const uint2 mrt2 = g_mrt2.Load(int3(VbgiTexel(uv, uint2(m2w, m2h)), 0));
    mat_index = mrt2.x;
    emissive_f = (float)((mrt2.y >> 10) & 31u) / 31.0f;
  }
  const VbgiDeferredParam mat = g_deferred[min(0x4E1Fu, mat_index)];
  const float e = min(1.0f, mat.head[0].w * emissive_f);

  // Lamps: the game's tile lists at this pixel's tile cell (lighting :837-846, :689-693).
  const uint2 tile = (uint2)(lightTileSizeInv_g * (float2(pf) + 0.5f));
  const uint row = tile.y << 5;
  const uint slice = (uint)clamp(g_lamp_tiles[tile.x + row].tileDepthInv * -view_z, 0.0f, 7.0f);
  const uint cell = min(4607u, slice * 576u + row + tile.x);
  const LightIndexData lamp_cell = g_lamp_tiles[cell];
  float3 lamps = float3(0.0f, 0.0f, 0.0f);
  const uint point_count = min(63u, lamp_cell.pointLightCount);
  for (uint i = 0u; i < point_count; ++i) {
    const LightParam lamp = g_lamps[(uint)lamp_cell.pointLightIndices[i]];
    if (lamp.radius <= 0.0f) continue;
    const float3 to_lamp = lamp.pos - P;
    const float3 dir = normalize(to_lamp);
    lamps += lamp.color * LampDistanceAttenuation(lamp, to_lamp) * max(lamp.translucency, dot(n, dir));
  }
  const uint spot_count = min(63u, lamp_cell.spotLightCount);
  for (uint j = 0u; j < spot_count; ++j) {
    const LightParam lamp = g_lamps[(uint)lamp_cell.spotLightIndices[j]];
    if (lamp.radius <= 0.0f) continue;
    const float3 to_lamp = lamp.pos - P;
    const float3 dir = normalize(to_lamp);
    lamps += lamp.color * LampDistanceAttenuation(lamp, to_lamp) * LampSpotCone(lamp, dir)
           * max(lamp.translucency, dot(n, dir));
  }
  lamps *= mat.mid.w;

  // Multi-bounce: last frame's GI at this sender (reprojected), re-emitted as light.
  float3 prev_gi = float3(0.0f, 0.0f, 0.0f);
  if (g_feedback_w > 0.0f) {
    float motion_px, depth_spread;
    const float2 hist_uv = CustomTAA_Reproject(uv, g_depth, g_samp_point, g_motion, g_samp_point,
                                               1.0f / float2(dw, dh), float2(1.0f, 1.0f), motion_px, depth_spread);
    if (all(hist_uv >= 0.0f) && all(hist_uv <= 1.0f)) {
      uint gw, gh;
      g_prev_gi.GetDimensions(gw, gh);
      prev_gi = g_prev_gi.Load(int3(VbgiTexel(hist_uv, uint2(gw, gh)), 0)).rgb;
    }
  }

  const float3 light = g_ambient_w * probe + g_lights_w * lamps + g_sun_w * sun;
  const float3 lit = light + g_emissive_w * e * (1.0f - light);
  const float3 radiance = clamp(albedo * (lit + g_feedback_w * prev_gi), 0.0f, g_radiance_clamp);
  g_radiance[p] = float4(radiance, 1.0f);
}
