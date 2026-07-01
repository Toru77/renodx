// ---- Created with 3Dmigoto v1.4.1 on Wed Jul  1 19:40:21 2026

struct InstanceParam
{
    float4x4 world;                // Offset:    0
    float4x4 prevWorld;            // Offset:   64
    float4 color;                  // Offset:  128
    float4 uv;                     // Offset:  144
    float4 param;                  // Offset:  160
    uint boneAddress;              // Offset:  176
    float3 param2;                 // Offset:  180
};

struct PlantCollider
{
    float3 pos;                    // Offset:    0
    float radius;                  // Offset:   12
    float pushStrength;            // Offset:   16
    float waveTime;                // Offset:   20
    float pad[2];                  // Offset:   24
};

struct InstanceInfoPlantGrow
{
    uint bufferOffset;             // Offset:    0
    uint instanceID;               // Offset:    4
    uint flags;                    // Offset:    8
    float nearClipDistance;        // Offset:   12
    float3 aabbMin;                // Offset:   16
    uint instanceCount;            // Offset:   28
    float3 aabbMax;                // Offset:   32
    float farClipDistance;         // Offset:   44
};

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
  float disableMapObjNearFade_g : packoffset(c25.w);
  float3 lightDirection_g : packoffset(c26);
  float gameTime_g : packoffset(c26.w);
  float3 sceneShadowColor_g : packoffset(c27);
  int shadowmapCascadeCount_g : packoffset(c27.w);
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
  float localShadowResolutionInv_g : packoffset(c32.w);
  float4x4 ditherMtx_g : packoffset(c33);
  float4 lightProbe_g[9] : packoffset(c37);
  float3 chrLightDir_g : packoffset(c46);
  float windForce_g : packoffset(c46.w);
  float4 mapColor_g : packoffset(c47);
  float4 clipPlane_g : packoffset(c48);
  float2 resolutionScaling_g : packoffset(c49);
  float2 invShadowSize_g : packoffset(c49.z);
  float3 chrShadowColor_g : packoffset(c50);
  float shadowFadeNear_g : packoffset(c50.w);
  float4 frustumPlanes_g[6] : packoffset(c51);
  float3 shadowSplitDistance_g : packoffset(c57);
  float shadowFadeRangeInv_g : packoffset(c57.w);
  float4x4 shadowMtx_g[4] : packoffset(c58);
  float4x4 prevViewProj_g : packoffset(c74);
  float2 jitterDiff_g : packoffset(c78);
}

cbuffer cb_instance_cull_plant : register(b2)
{

  struct
  {
    float4 planes[6];
  } frustum_g : packoffset(c0);

  uint polyCount_g : packoffset(c6);
  uint maxInstanceCount_g : packoffset(c6.y);
  uint meshCount_g : packoffset(c6.z);
  float growRadius_g : packoffset(c6.w);
  float3 basePos_g : packoffset(c7);
  float margin_g : packoffset(c7.w);
  float rayY_g : packoffset(c8);
  float rayLength_g : packoffset(c8.y);
  float scaleRandomMin_g : packoffset(c8.z);
  float scaleRandomMax_g : packoffset(c8.w);
  float randSeed_g : packoffset(c9);
  float marginInv_g : packoffset(c9.y);
  float modelRadius_g : packoffset(c9.z);
  uint plantColliderCount_g : packoffset(c9.w);
  float plantColliderStrengh_g : packoffset(c10);
  float plantColliderRadius_g : packoffset(c10.y);
  float4x4 mtx_g : packoffset(c11);
}

StructuredBuffer<InstanceParam> srcInstances_g : register(t4);
ByteAddressBuffer positionBuffer_g : register(t5);
ByteAddressBuffer colorBuffer_g : register(t6);
ByteAddressBuffer indexBuffer_g : register(t7);
StructuredBuffer<PlantCollider> colliders_g : register(t8);
RWStructuredBuffer<InstanceParam> destInstances_g : register(u0);
RWByteAddressBuffer indirectArg_g : register(u1);
RWStructuredBuffer<InstanceInfoPlantGrow> instanceInfos_g : register(u2);
RWByteAddressBuffer collisionFeedbackBuffer_g : register(u3);


// 3Dmigoto declarations
#define cmp -


[numthreads(64, 1, 1)]
void main(uint3 vThreadID : SV_DispatchThreadID)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31,r32,r33,r34,r35,r36,r37,r38,r39;
  uint4 bitmask, uiDest;
  float4 fDest;
  r0.x = cmp((uint)vThreadID.x >= polyCount_g);
  if (r0.x != 0) {
    return;
  }
  r0.x = (int)vThreadID.x * 12;
r0.xyz = asfloat(indexBuffer_g.Load3(asuint(r0.x)));
  r1.xyz = (int3)r0.xyz * int3(12,12,12);
r2.xyz = asfloat(positionBuffer_g.Load3(asuint(r1.x)));
  r2.w = 1;
  r3.z = dot(r2.xyzw, mtx_g._m00_m10_m20_m30);
  r3.x = dot(r2.xyzw, mtx_g._m01_m11_m21_m31);
  r3.y = dot(r2.xyzw, mtx_g._m02_m12_m22_m32);
r2.xyz = asfloat(positionBuffer_g.Load3(asuint(r1.y)));
  r2.w = 1;
  r4.x = dot(r2.xyzw, mtx_g._m00_m10_m20_m30);
  r4.y = dot(r2.xyzw, mtx_g._m01_m11_m21_m31);
  r4.z = dot(r2.xyzw, mtx_g._m02_m12_m22_m32);
r1.xyz = asfloat(positionBuffer_g.Load3(asuint(r1.z)));
  r1.w = 1;
  r2.x = dot(r1.xyzw, mtx_g._m00_m10_m20_m30);
  r2.y = dot(r1.xyzw, mtx_g._m01_m11_m21_m31);
  r2.z = dot(r1.xyzw, mtx_g._m02_m12_m22_m32);
  r1.xyz = min(r4.xyz, r2.xyz);
  r1.xyz = min(r3.zxy, r1.xyz);
  r5.xyz = max(r4.xyz, r2.xyz);
  r5.xyz = max(r5.xyz, r3.zxy);
  r6.xyz = r5.xyz + r1.xyz;
  r7.xyz = float3(0.5,0.5,0.5) * r6.xyz;
  r6.xyz = r6.xyz * float3(0.5,0.5,0.5) + -r1.xyz;
  r0.w = dot(r6.xyz, r6.xyz);
  r0.w = sqrt(r0.w);
  r1.w = 0;
  r2.w = 0;
  while (true) {
    r3.w = cmp((uint)r1.w >= 6);
    r2.w = 0;
    if (r3.w != 0) break;
    r3.w = dot(r7.xyz, frustum_g.planes[r1.w].xyz);
    r3.w = frustum_g.planes[r1.w].w + r3.w;
    r3.w = cmp(r3.w < -r0.w);
    if (r3.w != 0) {
      r2.w = -1;
      break;
    }
    r1.w = (int)r1.w + 1;
    r2.w = r3.w;
  }
  if (r2.w != 0) {
    return;
  }
  r0.w = cmp(viewInv_g._m30 < r1.x);
  r1.w = viewInv_g._m30 + -r1.x;
  r1.w = r1.w * r1.w;
  r0.w = r0.w ? r1.w : 0;
  r1.w = cmp(viewInv_g._m31 < r1.y);
  r1.y = viewInv_g._m31 + -r1.y;
  r1.y = r1.y * r1.y + r0.w;
  r0.w = r1.w ? r1.y : r0.w;
  r1.y = cmp(viewInv_g._m32 < r1.z);
  r1.w = viewInv_g._m32 + -r1.z;
  r1.w = r1.w * r1.w + r0.w;
  r0.w = r1.y ? r1.w : r0.w;
  r1.y = cmp(r5.x < viewInv_g._m30);
  r1.w = viewInv_g._m30 + -r5.x;
  r1.w = r1.w * r1.w + r0.w;
  r0.w = r1.y ? r1.w : r0.w;
  r1.y = cmp(r5.y < viewInv_g._m31);
  r1.w = viewInv_g._m31 + -r5.y;
  r1.w = r1.w * r1.w + r0.w;
  r0.w = r1.y ? r1.w : r0.w;
  r1.y = cmp(r5.z < viewInv_g._m32);
  r1.w = viewInv_g._m32 + -r5.z;
  r1.w = r1.w * r1.w + r0.w;
  r0.w = r1.y ? r1.w : r0.w;
  r1.y = growRadius_g * growRadius_g;
  r0.w = cmp(r1.y < r0.w);
  if (r0.w != 0) {
    return;
  }
  r0.xyz = (uint3)r0.xyz << int3(2,2,2);
r6.x = asfloat(colorBuffer_g.Load(asuint(r0.x)));
  r6.yzw = (uint3)r6.xxx >> int3(8,16,24);
  r6.xyzw = (int4)r6.xyzw & int4(255,255,255,255);
  r6.xyzw = (uint4)r6.xyzw;
  r6.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r6.xyzw;
r7.x = asfloat(colorBuffer_g.Load(asuint(r0.y)));
  r7.yzw = (uint3)r7.xxx >> int3(8,16,24);
  r7.xyzw = (int4)r7.xyzw & int4(255,255,255,255);
  r7.xyzw = (uint4)r7.xyzw;
  r7.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r7.xyzw;
r7.x = asfloat(colorBuffer_g.Load(asuint(r0.z)));
  r0.yzw = (uint3)r0.xxx >> int3(8,16,24);
  r0.xyzw = (int4)r0.xyzw & int4(255,255,255,255);
  r0.xyzw = (uint4)r0.xyzw;
  r0.xyzw = float4(0.00392156886,0.00392156886,0.00392156886,0.00392156886) * r0.xyzw;
  r8.xyz = r4.yzx + -r3.xyz;
  r9.xyz = r2.xyz + -r3.zxy;
  r10.xyz = r2.xyz + -r4.xyz;
  r1.y = dot(r8.xyz, r8.xyz);
  r11.x = sqrt(r1.y);
  r1.y = dot(r9.xyz, r9.xyz);
  r11.y = sqrt(r1.y);
  r1.y = dot(r10.xyz, r10.xyz);
  r11.z = sqrt(r1.y);
  r1.y = r11.x + r11.y;
  r1.y = r1.y + r11.z;
  r1.w = 0.5 * r1.y;
  r10.xyz = r1.yyy * float3(0.5,0.5,0.5) + -r11.xyz;
  r1.y = r10.x * r1.w;
  r1.y = r1.y * r10.y;
  r1.y = r1.y * r10.z;
  r1.y = sqrt(r1.y);
  r10.xyz = -r4.xyz + r3.zxy;
  r1.w = dot(r10.xyz, r10.xyz);
  r1.w = rsqrt(r1.w);
  r10.xyz = r10.xzy * r1.www;
  r12.xyz = r3.xyz + -r2.yzx;
  r1.w = dot(r12.xyz, r12.xyz);
  r1.w = rsqrt(r1.w);
  r12.xyz = r12.xyz * r1.www;
  r13.xyz = r12.xyz * r10.yxz;
  r12.xyz = r10.zyx * r12.yzx + -r13.xyz;
  r1.w = dot(r12.xyz, r12.xyz);
  r1.w = rsqrt(r1.w);
  r12.xyz = r12.xyz * r1.www;
  r13.xyz = r12.zxy * r10.zyx;
  r13.xyz = r12.yzx * r10.yxz + -r13.xyz;
  r1.w = dot(r13.xyz, r13.xyz);
  r1.w = rsqrt(r1.w);
  r13.xyz = r13.xyz * r1.www;
  r1.xz = -basePos_g.xz + r1.xz;
  r1.xz = marginInv_g * r1.xz;
  r1.xz = ceil(r1.xz);
  r1.xz = (int2)r1.xz;
  r5.xy = -basePos_g.xz + r5.xz;
  r5.xy = marginInv_g * r5.xy;
  r5.xy = ceil(r5.xy);
  r5.xy = (int2)r5.xy;
  r14.y = rayLength_g;
  r14.xz = float2(0,0);
  r5.zw = r14.zy * r9.yx;
  r5.zw = r14.yz * r9.zy + -r5.zw;
  r1.w = dot(r5.wz, r8.yz);
  r2.w = cmp(9.99999997e-07 < r1.w);
  r15.x = viewInv_g._m30;
  r15.y = viewInv_g._m31;
  r15.z = viewInv_g._m32;
  r3.w = growRadius_g * 0.699999988;
  r4.w = scaleRandomMax_g + -scaleRandomMin_g;
  r16.y = rayY_g * margin_g;
  r17.y = 0;
  r18.z = r11.y;
  r19.z = r11.x;
  r13.w = r10.x;
  r10.xw = r13.zy;
  r8.w = maxInstanceCount_g;
  r9.w = r1.x;
  while (true) {
    r11.z = cmp((int)r5.x < (int)r9.w);
    if (r11.z != 0) break;
    r20.x = (int)r9.w;
    r11.z = r8.w;
    r11.w = r1.z;
    while (true) {
      r12.w = cmp((int)r5.y < (int)r11.w);
      if (r12.w != 0) break;
      r20.y = (int)r11.w;
      r16.xz = margin_g * r20.xy;
      r16.xzw = basePos_g.xyz + r16.xyz;
      r13.yz = randSeed_g + r20.xy;
      r12.w = dot(r13.yz, float2(127.099998,311.700012));
      r14.w = dot(r13.yz, float2(269.5,183.300003));
      r21.x = sin(r12.w);
      r21.z = sin(r14.w);
      r20.yz = float2(43758.5469,43758.5469) * r21.xz;
      r20.yz = frac(r20.yz);
      r20.yz = r20.yz * float2(2,2) + float2(-1,-1);
      r20.yz = margin_g * r20.yz;
      r17.xz = float2(0.5,0.5) * r20.yz;
      r16.xzw = r17.xyz + r16.xzw;
      r17.xzw = r16.wxz + -r3.yzx;
      r12.w = dot(r5.wz, r17.xz);
      r14.w = cmp(r12.w >= 0);
      r15.w = cmp(r1.w >= r12.w);
      r14.w = r14.w ? r15.w : 0;
      r20.yzw = r17.xzw * r8.xyz;
      r17.xzw = r17.wxz * r8.yzx + -r20.yzw;
      r15.w = rayLength_g * r17.z;
      r15.w = cmp(r15.w >= 0);
      r12.w = r17.z * rayLength_g + r12.w;
      r12.w = cmp(r1.w >= r12.w);
      r18.w = r14.w ? r15.w : 0;
      r18.w = r2.w ? r18.w : 0;
      r18.w = r12.w ? r18.w : 0;
      if (r18.w == 0) {
        r18.w = (int)r11.w + 1;
        r11.w = r18.w;
        continue;
      }
      r12.w = r12.w ? r15.w : 0;
      r15.w = dot(r17.xzw, r9.xyz);
      r15.w = r15.w / r1.w;
      r17.xzw = r14.xyz * r15.www + r16.xzw;
      r20.yzw = r12.www ? r17.xzw : r16.xzw;
      r20.yzw = r14.www ? r20.yzw : r16.xzw;
      r16.xzw = r2.www ? r20.yzw : r16.xzw;
      r20.yzw = r16.xzw + -r15.xyz;
      r12.w = dot(r20.yzw, r20.yzw);
      r12.w = sqrt(r12.w);
      r14.w = cmp(growRadius_g < r12.w);
      if (r14.w != 0) {
        r14.w = (int)r11.w + 1;
        r11.w = r14.w;
        continue;
      }
      r13.y = dot(r13.yz, float2(12.9898005,78.2330017));
      r13.y = sin(r13.y);
      r13.y = 43758.5469 * r13.y;
      r13.y = frac(r13.y);
      r13.z = r13.y * r3.w + r12.w;
      r13.z = -growRadius_g + r13.z;
      r13.z = saturate(r13.z / modelRadius_g);
      r13.z = 1 + -r13.z;
      r14.w = cmp(0 >= r13.z);
      if (r14.w != 0) {
        r14.w = (int)r11.w + 1;
        r11.w = r14.w;
        continue;
      }
      r20.yzw = r17.xzw + -r3.zxy;
      r14.w = dot(r20.yzw, r20.yzw);
      r18.x = sqrt(r14.w);
      r20.yzw = r17.xzw + -r4.xyz;
      r14.w = dot(r20.yzw, r20.yzw);
      r19.y = sqrt(r14.w);
      r17.xzw = r17.xzw + -r2.xyz;
      r14.w = dot(r17.xzw, r17.xzw);
      r18.y = sqrt(r14.w);
      r14.w = r18.x + r18.y;
      r14.w = r14.w + r11.y;
      r15.w = 0.5 * r14.w;
      r17.xzw = r14.www * float3(0.5,0.5,0.5) + -r18.xyz;
      r14.w = r17.x * r15.w;
      r14.w = r14.w * r17.z;
      r14.w = r14.w * r17.w;
      r21.x = sqrt(r14.w);
      r14.w = r19.y + r18.x;
      r14.w = r14.w + r11.x;
      r15.w = 0.5 * r14.w;
      r19.x = r18.x;
      r17.xzw = r14.www * float3(0.5,0.5,0.5) + -r19.xyz;
      r14.w = r17.x * r15.w;
      r14.w = r14.w * r17.z;
      r14.w = r14.w * r17.w;
      r21.y = sqrt(r14.w);
      r17.xz = r21.xy / r1.yy;
      r14.w = 1 + -r17.x;
      r14.w = r14.w + -r17.z;
      r21.xyzw = r17.xxxx * r7.xyzw;
      r21.xyzw = r6.xyzw * r14.wwww + r21.xyzw;
      r21.xyzw = r0.xyzw * r17.zzzz + r21.xyzw;
      r14.w = 3.14159274 * r13.y;
      sincos(r14.w, r18.x, r17.x);
      r13.y = r13.y * r4.w + scaleRandomMin_g;
      r13.y = r13.y * r21.w;
      r18.y = r17.x;
      r22.xy = r18.xy * r13.yy;
      r22.z = -r22.x;
      r23.x = dot(r22.yz, r13.xw);
      r24.x = dot(r22.zy, r10.zw);
      r25.x = dot(r22.yz, r10.xy);
      r17.xzw = r13.yyy * r12.xyz;
      r23.z = dot(r22.xy, r13.xw);
      r24.z = dot(r22.yx, r10.zw);
      r25.z = dot(r22.xy, r10.xy);
      r23.y = r17.x;
      r23.w = r16.x;
      r24.y = r17.z;
      r24.w = r16.z;
      r25.y = r17.w;
      r25.w = r16.w;
      r14.w = r11.z;
      r15.w = 0;
      while (true) {
        r16.x = cmp((uint)r15.w >= meshCount_g);
        if (r16.x != 0) break;
        r16.x = instanceInfos_g[r15.w].nearClipDistance;
        r16.z = cmp(0 < r16.x);
        r16.x = r16.x + -r12.w;
        r16.x = saturate(r16.x / modelRadius_g);
        r16.x = 1 + -r16.x;
        r16.x = r16.x * r13.z;
        r16.x = r16.z ? r16.x : r13.z;
        r16.z = instanceInfos_g[r15.w].farClipDistance;
        r16.w = cmp(0 < r16.z);
        r16.z = r16.z + -r12.w;
        r16.z = saturate(r16.z / modelRadius_g);
        r16.z = r16.x * r16.z;
        r16.x = r16.w ? r16.z : r16.x;
        r16.z = cmp(0 >= r16.x);
        if (r16.z != 0) {
          r16.z = (int)r15.w + 1;
          r15.w = r16.z;
          continue;
        }
        r16.z = instanceInfos_g[r15.w].instanceID;
        r16.z = mad(20, (int)r16.z, 4);
        uint temp_r22x;
        indirectArg_g.InterlockedAdd(asuint(r16.z), 1, temp_r22x);
        r22.x = asfloat(temp_r22x);
        r16.z = cmp((uint)r22.x < (uint)r14.w);
        if (r16.z != 0) {
          r26.x = srcInstances_g[r15.w].world._m00;
          r26.y = srcInstances_g[r15.w].world._m10;
          r26.z = srcInstances_g[r15.w].world._m20;
          r26.w = srcInstances_g[r15.w].world._m30;
          r27.x = srcInstances_g[r15.w].world._m01;
          r27.y = srcInstances_g[r15.w].world._m11;
          r27.z = srcInstances_g[r15.w].world._m21;
          r27.w = srcInstances_g[r15.w].world._m31;
          r28.x = srcInstances_g[r15.w].world._m02;
          r28.y = srcInstances_g[r15.w].world._m12;
          r28.z = srcInstances_g[r15.w].world._m22;
          r28.w = srcInstances_g[r15.w].world._m32;
          r29.x = srcInstances_g[r15.w].world._m03;
          r29.y = srcInstances_g[r15.w].world._m13;
          r29.z = srcInstances_g[r15.w].world._m23;
          r29.w = srcInstances_g[r15.w].world._m33;
          r30.x = r26.x;
          r30.y = r27.x;
          r30.z = r28.x;
          r30.w = r29.x;
          r31.x = dot(r30.xyzw, r23.xyzw);
          r32.x = dot(r30.xyzw, r24.xyzw);
          r30.x = dot(r30.xyzw, r25.xyzw);
          r33.x = r26.y;
          r33.y = r27.y;
          r33.z = r28.y;
          r33.w = r29.y;
          r31.y = dot(r33.xyzw, r23.xyzw);
          r32.y = dot(r33.xyzw, r24.xyzw);
          r30.y = dot(r33.xyzw, r25.xyzw);
          r33.x = r26.z;
          r33.y = r27.z;
          r33.z = r28.z;
          r33.w = r29.z;
          r31.z = dot(r33.xyzw, r23.xyzw);
          r32.z = dot(r33.xyzw, r24.xyzw);
          r30.w = dot(r33.xyzw, r25.xyzw);
          r33.x = r26.w;
          r33.y = r27.w;
          r33.z = r28.w;
          r33.w = r29.w;
          r31.w = dot(r33.xyzw, r23.xyzw);
          r32.w = dot(r33.xyzw, r24.xyzw);
          r30.z = dot(r33.xyzw, r25.xyzw);
          r16.z = instanceInfos_g[r15.w].bufferOffset;
          r16.z = (int)r16.z + (int)r22.x;
          r22.x = srcInstances_g[r15.w].prevWorld._m00;
          r22.y = srcInstances_g[r15.w].prevWorld._m10;
          r22.z = srcInstances_g[r15.w].prevWorld._m20;
          r22.w = srcInstances_g[r15.w].prevWorld._m30;
          r33.x = srcInstances_g[r15.w].prevWorld._m01;
          r33.y = srcInstances_g[r15.w].prevWorld._m11;
          r33.z = srcInstances_g[r15.w].prevWorld._m21;
          r33.w = srcInstances_g[r15.w].prevWorld._m31;
          r34.x = srcInstances_g[r15.w].prevWorld._m02;
          r34.y = srcInstances_g[r15.w].prevWorld._m12;
          r34.z = srcInstances_g[r15.w].prevWorld._m22;
          r34.w = srcInstances_g[r15.w].prevWorld._m32;
          r35.x = srcInstances_g[r15.w].prevWorld._m03;
          r35.y = srcInstances_g[r15.w].prevWorld._m13;
          r35.z = srcInstances_g[r15.w].prevWorld._m23;
          r35.w = srcInstances_g[r15.w].prevWorld._m33;
          r36.x = srcInstances_g[r15.w].color.x;
          r36.y = srcInstances_g[r15.w].color.y;
          r36.z = srcInstances_g[r15.w].color.z;
          r36.w = srcInstances_g[r15.w].color.w;
          r37.x = srcInstances_g[r15.w].uv.x;
          r37.y = srcInstances_g[r15.w].uv.y;
          r37.z = srcInstances_g[r15.w].uv.z;
          r37.w = srcInstances_g[r15.w].uv.w;
          r38.x = srcInstances_g[r15.w].param.x;
          r38.y = srcInstances_g[r15.w].param.y;
          r38.z = srcInstances_g[r15.w].param.z;
          r38.w = srcInstances_g[r15.w].param.w;
          r39.x = srcInstances_g[r15.w].boneAddress;
          r39.y = srcInstances_g[r15.w].param2.x;
          r39.z = srcInstances_g[r15.w].param2.y;
          r39.w = srcInstances_g[r15.w].param2.z;
          destInstances_g[r16.z].world._m00 = r26.x;
          destInstances_g[r16.z].world._m10 = r26.y;
          destInstances_g[r16.z].world._m20 = r26.z;
          destInstances_g[r16.z].world._m30 = r26.w;
          destInstances_g[r16.z].world._m01 = r27.x;
          destInstances_g[r16.z].world._m11 = r27.y;
          destInstances_g[r16.z].world._m21 = r27.z;
          destInstances_g[r16.z].world._m31 = r27.w;
          destInstances_g[r16.z].world._m02 = r28.x;
          destInstances_g[r16.z].world._m12 = r28.y;
          destInstances_g[r16.z].world._m22 = r28.z;
          destInstances_g[r16.z].world._m32 = r28.w;
          destInstances_g[r16.z].world._m03 = r29.x;
          destInstances_g[r16.z].world._m13 = r29.y;
          destInstances_g[r16.z].world._m23 = r29.z;
          destInstances_g[r16.z].world._m33 = r29.w;
          destInstances_g[r16.z].prevWorld._m00 = r22.x;
          destInstances_g[r16.z].prevWorld._m10 = r22.y;
          destInstances_g[r16.z].prevWorld._m20 = r22.z;
          destInstances_g[r16.z].prevWorld._m30 = r22.w;
          destInstances_g[r16.z].prevWorld._m01 = r33.x;
          destInstances_g[r16.z].prevWorld._m11 = r33.y;
          destInstances_g[r16.z].prevWorld._m21 = r33.z;
          destInstances_g[r16.z].prevWorld._m31 = r33.w;
          destInstances_g[r16.z].prevWorld._m02 = r34.x;
          destInstances_g[r16.z].prevWorld._m12 = r34.y;
          destInstances_g[r16.z].prevWorld._m22 = r34.z;
          destInstances_g[r16.z].prevWorld._m32 = r34.w;
          destInstances_g[r16.z].prevWorld._m03 = r35.x;
          destInstances_g[r16.z].prevWorld._m13 = r35.y;
          destInstances_g[r16.z].prevWorld._m23 = r35.z;
          destInstances_g[r16.z].prevWorld._m33 = r35.w;
          destInstances_g[r16.z].color.x = r36.x;
          destInstances_g[r16.z].color.y = r36.y;
          destInstances_g[r16.z].color.z = r36.z;
          destInstances_g[r16.z].color.w = r36.w;
          destInstances_g[r16.z].uv.x = r37.x;
          destInstances_g[r16.z].uv.y = r37.y;
          destInstances_g[r16.z].uv.z = r37.z;
          destInstances_g[r16.z].uv.w = r37.w;
          destInstances_g[r16.z].param.x = r38.x;
          destInstances_g[r16.z].param.y = r38.y;
          destInstances_g[r16.z].param.z = r38.z;
          destInstances_g[r16.z].param.w = r38.w;
          destInstances_g[r16.z].boneAddress = r39.x;
          destInstances_g[r16.z].param2.x = r39.y;
          destInstances_g[r16.z].param2.y = r39.z;
          destInstances_g[r16.z].param2.z = r39.w;
          destInstances_g[r16.z].world._m00 = r31.x;
          destInstances_g[r16.z].world._m10 = r31.y;
          destInstances_g[r16.z].world._m20 = r31.z;
          destInstances_g[r16.z].world._m30 = r31.w;
          destInstances_g[r16.z].world._m01 = r32.x;
          destInstances_g[r16.z].world._m11 = r32.y;
          destInstances_g[r16.z].world._m21 = r32.z;
          destInstances_g[r16.z].world._m31 = r32.w;
          destInstances_g[r16.z].world._m02 = r30.x;
          destInstances_g[r16.z].world._m12 = r30.y;
          destInstances_g[r16.z].world._m22 = r30.w;
          destInstances_g[r16.z].world._m32 = r30.z;
          destInstances_g[r16.z].prevWorld._m00 = r31.x;
          destInstances_g[r16.z].prevWorld._m10 = r31.y;
          destInstances_g[r16.z].prevWorld._m20 = r31.z;
          destInstances_g[r16.z].prevWorld._m30 = r31.w;
          destInstances_g[r16.z].prevWorld._m01 = r32.x;
          destInstances_g[r16.z].prevWorld._m11 = r32.y;
          destInstances_g[r16.z].prevWorld._m21 = r32.z;
          destInstances_g[r16.z].prevWorld._m31 = r32.w;
          destInstances_g[r16.z].prevWorld._m02 = r30.x;
          destInstances_g[r16.z].prevWorld._m12 = r30.y;
          destInstances_g[r16.z].prevWorld._m22 = r30.w;
          destInstances_g[r16.z].prevWorld._m32 = r30.z;
          destInstances_g[r16.z].prevWorld._m03 = r29.x;
          destInstances_g[r16.z].prevWorld._m13 = r29.y;
          destInstances_g[r16.z].prevWorld._m23 = r29.z;
          destInstances_g[r16.z].prevWorld._m33 = r29.w;
          r22.xyzw = r36.xyzw * r21.xyzw;
          destInstances_g[r16.z].color.x = r22.x;
          destInstances_g[r16.z].color.y = r22.y;
          destInstances_g[r16.z].color.z = r22.z;
          destInstances_g[r16.z].color.w = r22.w;
          destInstances_g[r16.z].color.w = r16.x;
          r16.x = instanceInfos_g[r15.w].flags;
          r30.x = r31.w;
          r30.y = r32.w;
          r16.w = 0;
          while (true) {
            r17.x = cmp((uint)r16.w >= plantColliderCount_g);
            if (r17.x != 0) break;
            r22.x = colliders_g[r16.w].pos.x;
            r22.y = colliders_g[r16.w].pos.y;
            r22.z = colliders_g[r16.w].pos.z;
            r22.w = colliders_g[r16.w].radius;
            r17.xzw = r30.xyz + -r22.xyz;
            r18.x = dot(r17.xzw, r17.xzw);
            r18.y = sqrt(r18.x);
            r18.w = plantColliderRadius_g * r13.y + r22.w;
            r19.x = cmp(r18.y < r18.w);
            if (r19.x != 0) {
              r19.x = (uint)r16.w << 2;
              collisionFeedbackBuffer_g.InterlockedOr(asuint(r19.x), asuint(r16.x));
              r19.x = cmp(0 < r18.y);
              if (r19.x != 0) {
                r19.x = colliders_g[r16.w].pushStrength;
                r19.y = colliders_g[r16.w].waveTime;
                r19.y = r19.y + r18.y;
                r19.y = 0.159154937 * r19.y;
                r19.w = cmp(r19.y >= -r19.y);
                r19.y = frac(abs(r19.y));
                r19.y = r19.w ? r19.y : -r19.y;
                r19.y = 6.28318548 * r19.y;
                r19.y = sin(r19.y);
                r19.y = r19.y * 0.5 + 0.5;
                r18.x = rsqrt(r18.x);
                r17.xzw = r18.xxx * r17.xzw;
                r17.xzw = r17.xzw * r19.yyy;
                r18.x = r18.w + -r18.y;
                r17.xzw = r18.xxx * r17.xzw;
                r17.xzw = plantColliderStrengh_g * r17.xzw;
                r18.x = destInstances_g[r16.z].param2.x;
                r18.y = destInstances_g[r16.z].param2.y;
                r18.w = destInstances_g[r16.z].param2.z;
                r17.xzw = r17.xzw * r19.xxx + r18.xyw;
                destInstances_g[r16.z].param2.x = r17.x;
                destInstances_g[r16.z].param2.y = r17.z;
                destInstances_g[r16.z].param2.z = r17.w;
              }
            }
            r16.w = (int)r16.w + 1;
          }
        }
        r15.w = (int)r15.w + 1;
      }
      r11.w = (int)r11.w + 1;
    }
    r9.w = (int)r9.w + 1;
  }
  return;
}