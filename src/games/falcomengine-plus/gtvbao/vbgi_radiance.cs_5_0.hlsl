// falcomengine-plus VBGI - Radiance prepass (M7.4 step 3).
// One thread per radiance texel. Writes the sender radiance L_j = albedo * (ambient + lights + sun) with the emissive
// term, which the VBGI gather samples at the same texel. Sky writes 0. Multi-bounce is not included yet.
// Inputs are the lighting-draw captures (Sora 1st and Sora 2nd): see addon.cpp RunVbgiRadiance.
// Lamps follow the game's lamp tile lists (lightIndices_g) through world_rt_lamps.hlsli; shadow maps and specular are ignored.

#include "../include/mrt_normal.hlsli"
#include "../world/shaders/world_rt_lamps.hlsli"

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

RWTexture2D<float4> g_radiance : register(u0);

cbuffer cb_scene : register(b0)
{
  float4x4 view_g        : packoffset(c0);
  float4x4 viewProjInv_g : packoffset(c20);
  float3 lightColor_g     : packoffset(c25);
  float3 sceneShadowColor_g : packoffset(c27);
  float2 lightTileSizeInv_g : packoffset(c29);
  float4 lightProbe_g[9] : packoffset(c37);
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
};

uint2 VbgiTexel(float2 uv, uint2 size)
{
  return min(uint2(uv * float2(size)), size - 1u);
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
  if (depth >= 1.0f) {
    g_radiance[p] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    return;
  }

  uint cw, ch;
  g_color.GetDimensions(cw, ch);
  const float3 albedo = g_color.Load(int3(VbgiTexel(uv, uint2(cw, ch)), 0)).rgb;

  uint m0w, m0h;
  g_mrt0.GetDimensions(m0w, m0h);
  const float3 n = DecodeFalcomMrtNormalAsGame(g_mrt0.Load(int3(VbgiTexel(uv, uint2(m0w, m0h)), 0)).xy);

  // World position from the representative pixel's depth (same mapping as world_rtao).
  const float4 world_h = mul(float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, depth, 1.0f), viewProjInv_g);
  const float3 P = world_h.xyz / world_h.w;
  const float view_z = dot(view_g._m02_m12_m22_m32, float4(P, 1.0f));

  // Ambient: the game's SH probe at the sender normal.
  const float3 probe = max(float3(0.0f, 0.0f, 0.0f),
      lightProbe_g[0].xyz + lightProbe_g[1].xyz * n.x + lightProbe_g[2].xyz * n.y + lightProbe_g[3].xyz * n.z
      + lightProbe_g[4].xyz * (n.z * n.x) + lightProbe_g[5].xyz * (n.y * n.z) + lightProbe_g[6].xyz * (n.y * n.x)
      + lightProbe_g[7].xyz * (3.0f * n.z * n.z - 1.0f) + lightProbe_g[8].xyz * (n.x * n.x - n.y * n.y));

  // Sun: the game's sun term minus its shadow floor (RTGI SunGiAt).
  uint m1w, m1h;
  g_mrt1.GetDimensions(m1w, m1h);
  const float k = (float)g_mrt1.Load(int3(VbgiTexel(uv, uint2(m1w, m1h)), 0)).w / 255.0f;
  uint sw, sh;
  g_shadow.GetDimensions(sw, sh);
  const float shadow = g_shadow.Load(int3(VbgiTexel(uv, uint2(sw, sh)), 0)).x;
  const float3 sun = lightColor_g * (1.0f - min(float3(1.0f, 1.0f, 1.0f), sceneShadowColor_g)) * (k * shadow);

  // Material: emissive amount and the dynamic light intensity of this pixel's material.
  uint m2w, m2h;
  g_mrt2.GetDimensions(m2w, m2h);
  const uint2 mat_texel = uint2(VbgiTexel(uv, uint2(m2w, m2h)));
  const uint2 mrt2 = g_mrt2.Load(int3(mat_texel, 0));
  const VbgiDeferredParam mat = g_deferred[min(0x4E1Fu, mrt2.x)];
  const float emissive_f = (float)((mrt2.y >> 10) & 31u) / 31.0f;
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

  const float3 light = g_ambient_w * probe + g_lights_w * lamps + g_sun_w * sun;
  const float3 lit = light + g_emissive_w * e * (1.0f - light);
  const float3 radiance = clamp(albedo * lit, 0.0f, g_radiance_clamp);
  g_radiance[p] = float4(radiance, 1.0f);
}
