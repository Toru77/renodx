// ---- Created with 3Dmigoto v1.3.16 on Sun Aug 24 17:32:55 2025
#include "../common.hlsl"
struct InstanceParam
{
    float4x4 world;                // Offset:    0
    float4x4 world_view_inv;       // Offset:   64
    float4 color;                  // Offset:  128
    float4 specular;               // Offset:  144
    float4 uv[3];                  // Offset:  160
    float4 shape_param;            // Offset:  208
    float3 param2;                 // Offset:  224
    uint boneAddress;              // Offset:  236
    uint camera_type;              // Offset:  240
    uint uv_mask;                  // Offset:  244
    float rim_alpha;               // Offset:  248
    float depth_fade_width_inv;    // Offset:  252
    float4 camera_fade_param;      // Offset:  256
    float3 hsv;                    // Offset:  272
    float pad;                     // Offset:  284
};

cbuffer cb_scene : register(b2)
{
  float4x4 view_proj_[2] : packoffset(c0);
  float4x4 view_[2] : packoffset(c8);
  float4x4 view_inv_ : packoffset(c16);
  float4x4 proj_inv_ : packoffset(c20);
  float4x4 rain_mask_matrix_ : packoffset(c24);
  float2 inv_vp_size_ : packoffset(c28);
  float2 screen_uv_scale_ : packoffset(c28.z);
}

SamplerState colorSampler_s : register(s0);
SamplerState alphaSampler_s : register(s1);
SamplerState distortionSampler_s : register(s2);
SamplerState depthSampler_s : register(s5);
Texture2D<float4> colorMap : register(t0);
Texture2D<float4> alphaMap : register(t1);
Texture2D<float4> distortionMap : register(t2);
Texture2D<float4> depthTexture : register(t5);
StructuredBuffer<InstanceParam> vfx_instances_g : register(t10);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  float4 v2 : TEXCOORD1,
  float4 v3 : TEXCOORD2,
  float4 v4 : TEXCOORD3,
  float4 v5 : COLOR0,
  float4 v6 : COLOR1,
  float4 v7 : TEXCOORD4,
  nointerpolation uint v8 : TEXCOORD9,
  out float4 o0 : SV_Target0)
{
  float4 r0,r1,r2,r3,r4,r5;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = vfx_instances_g[v8.x].hsv.x;
  r0.y = vfx_instances_g[v8.x].hsv.y;
  r1.x = vfx_instances_g[v8.x].shape_param.z;
  r1.y = vfx_instances_g[v8.x].shape_param.w;
  r1.z = vfx_instances_g[v8.x].param2.x;
  r1.w = vfx_instances_g[v8.x].param2.y;
  r2.y = vfx_instances_g[v8.x].param2.z;
  r0.w = vfx_instances_g[v8.x].depth_fade_width_inv;
  r2.zw = float2(255.000015,255.000015) * v1.wz;
  r2.zw = (uint2)r2.zw;
  r2.w = (uint)r2.w << 8;
  r2.z = (int)r2.w | (int)r2.z;
  r3.xyzw = (int4)r2.zzzz & int4(4096,8192,1,8);
  r2.w = cmp(0 < r1.x);
  r4.xy = cmp(v1.xy < float2(0.5,0.5));
  r4.zw = float2(1,1) + -v1.xy;
  r4.xy = r4.xy ? v1.xy : r4.zw;
  r4.xy = r4.xy / r1.xx;
  r4.xy = min(float2(1,1), r4.xy);
  r4.xy = log2(r4.xy);
  r4.xy = r2.ww ? r4.xy : 0;
  r1.xy = r4.xy * r1.yy;
  r1.xy = exp2(r1.xy);
  r1.xy = min(float2(1,1), r1.xy);
  r1.xy = r3.xy ? r1.xy : float2(1,1);
  r1.x = r1.x * r1.y;
  r1.y = cmp(0 < r0.w);
  if (r1.y != 0) {
    r4.x = v0.z;
    r4.yw = float2(1,1);
    r1.y = dot(proj_inv_._m22_m32, r4.xy);
    r2.w = dot(proj_inv_._m23_m33, r4.xy);
    r1.y = r1.y / r2.w;
    r3.xy = inv_vp_size_.xy * v0.xy;
    r3.xy = screen_uv_scale_.xy * r3.xy;
    r4.z = depthTexture.SampleLevel(depthSampler_s, r3.xy, 0).x;
    r2.w = dot(proj_inv_._m22_m32, r4.zw);
    r3.x = dot(proj_inv_._m23_m33, r4.zw);
    r2.w = r2.w / r3.x;
    r1.y = -r2.w + r1.y;
    r0.w = saturate(r1.y * r0.w);
    r1.x = r1.x * r0.w;
  }
  r3.xy = v1.yx * float2(2,2) + float2(-1,-1);
  r0.w = dot(r3.xy, r3.xy);
  r0.w = sqrt(r0.w);
  r4.x = 1 + -r0.w;
  r0.w = min(abs(r3.x), abs(r3.y));
  r1.y = max(abs(r3.x), abs(r3.y));
  r1.y = 1 / r1.y;
  r0.w = r1.y * r0.w;
  r1.y = r0.w * r0.w;
  r2.w = r1.y * 0.0208350997 + -0.0851330012;
  r2.w = r1.y * r2.w + 0.180141002;
  r2.w = r1.y * r2.w + -0.330299497;
  r1.y = r1.y * r2.w + 0.999866009;
  r2.w = r1.y * r0.w;
  r4.z = cmp(abs(r3.y) < abs(r3.x));
  r2.w = r2.w * -2 + 1.57079637;
  r2.w = r4.z ? r2.w : 0;
  r0.w = r0.w * r1.y + r2.w;
  r1.y = cmp(r3.y < -r3.y);
  r1.y = r1.y ? -3.141593 : 0;
  r0.w = r1.y + r0.w;
  r1.y = min(-r3.x, r3.y);
  r2.w = max(-r3.x, r3.y);
  r1.y = cmp(r1.y < -r1.y);
  r2.w = cmp(r2.w >= -r2.w);
  r1.y = r1.y ? r2.w : 0;
  r0.w = r1.y ? -r0.w : r0.w;
  r0.w = 3.14159274 + r0.w;
  r4.y = 0.159154937 * r0.w;
  r3.xy = r3.zz ? r4.xy : v1.yx;
  r3.xy = r3.xy * v2.wz + v2.yx;
  r3.xy = r3.ww ? r3.xy : r3.yx;
  r5.xyzw = (int4)r2.zzzz & int4(64,512,2,16);
  r3.zw = float2(1,1) + -r3.xy;
  r3.xy = r5.xy ? r3.zw : r3.xy;
  r3.zw = r5.zz ? r4.xy : v1.yx;
  r3.zw = r3.zw * v3.wz + v3.yx;
  r3.zw = r5.ww ? r3.zw : r3.wz;
  r5.xyzw = (int4)r2.zzzz & int4(128,1024,4,32);
  r4.zw = float2(1,1) + -r3.zw;
  r3.zw = r5.xy ? r4.zw : r3.zw;
  r4.xy = r5.zz ? r4.xy : v1.yx;
  r4.xy = r4.xy * v4.wz + v4.yx;
  r4.xy = r5.ww ? r4.xy : r4.yx;
  r5.xyz = (int3)r2.zzz & int3(256,2048,0x8000);
  r2.zw = float2(1,1) + -r4.xy;
  r2.zw = r5.xy ? r2.zw : r4.xy;
  r4.xyz = distortionMap.Sample(distortionSampler_s, r2.zw).xyz;
  // r4.rgb = srgbDecode(r4.rgb);
  r2.zw = r4.xy * r4.zz;
  r2.x = r1.w;
  r1.yw = r2.zw * r2.xy;
  r2.xy = r1.yw * float2(0.0625,0.0625) + r3.xy;
  r2.xyzw = colorMap.Sample(colorSampler_s, r2.xy).xyzw;
  // r2.rgb = srgbDecode(r2.rgb);
  r1.yw = r1.yw * float2(0.0625,0.0625) + r3.zw;
  r3.xyzw = alphaMap.Sample(alphaSampler_s, r1.yw).xyzw;
  // r3.rgb = srgbDecode(r3.rgb);
  r0.w = -0.00100000005 + r3.w;
  r0.w = r0.w + -r1.z;
  r0.w = cmp(r0.w < 0);
  if (r0.w != 0) discard;
  r2.xyzw = r3.xyzw * r2.xyzw;
  r0.w = v5.w * r1.x;
  r1.xyz = v5.xyz;
  r0.w = r2.w * r0.w;
  r1.xyz = r2.xyz * r1.xyz + v6.xyz;
  r1.w = cmp(r1.y < r1.z);
  r2.xy = r1.zy;
  r2.zw = float2(-1,0.666666687);
  r3.xy = r2.yx;
  r3.zw = float2(0,-0.333333343);
  r2.xyzw = r1.wwww ? r2.xyzw : r3.xyzw;
  r1.w = cmp(r1.x < r2.x);
  r3.xyz = r2.xyw;
  r3.w = r1.x;
  r2.xyw = r3.wyx;
  r2.xyzw = r1.wwww ? r3.yxzw : r2.yxzw;
  r1.w = min(r2.w, r2.x);
  r1.w = r2.y + -r1.w;
  r2.w = r2.w + -r2.x;
  r3.x = r1.w * 6 + 1.00000001e-010;
  r2.w = r2.w / r3.x;
  r2.z = r2.z + r2.w;
  r2.w = 1.00000001e-010 + r2.y;
  r2.x = r1.w / r2.w;
  r0.x = abs(r2.z) + r0.x;
  r1.w = cmp(1 < r0.x);
  r2.z = -1 + r0.x;
  r0.x = r1.w ? r2.z : r0.x;
  r0.z = v6.w;
  r0.yz = r2.xy * r0.yz;
  r2.xyz = float3(1,0.666666687,0.333333343) + r0.xxx;
  r2.xyz = frac(r2.xyz);
  r2.xyz = r2.xyz * float3(6,6,6) + float3(-3,-3,-3);
  r2.xyz = saturate(float3(-1,-1,-1) + abs(r2.xyz));
  r2.xyz = float3(-1,-1,-1) + r2.xyz;
  r2.xyz = r0.yyy * r2.xyz + float3(1,1,1);
  r0.xyz = r2.xyz * r0.zzz;
  o0.xyz = r5.zzz ? r0.xyz : r1.xyz;
  o0.w = (r0.w);
  // o0.rgb = srgbEncode(o0.rgb);
  return;
}