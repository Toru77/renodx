// ---- Created with 3Dmigoto v1.4.1 on Tue Oct  6 16:15:31 2026

struct Particle
{
    float3 position;               // Offset:    0
    float duration;                // Offset:   12
    float3 direction;              // Offset:   16
    float speed;                   // Offset:   28
    float4 next_direction;         // Offset:   32
    float3 animation_offset_position;// Offset:   48
    float elapsed_time;            // Offset:   60
    float3 animation_offset_rotation;// Offset:   64
    float prev_elapsed_time;       // Offset:   76
    float3 animation_offset_scale; // Offset:   80
    float total_gravity;           // Offset:   92
    float3 animation_offset_orbit_rotation;// Offset:   96
    uint random_seed;              // Offset:  108
    float3 animation_offset_position_scale;// Offset:  112
    uint flags;                    // Offset:  124
    float4x4 world;                // Offset:  128
    float4 color;                  // Offset:  192
    float4 specular;               // Offset:  208
    float4 uv[3];                  // Offset:  224
    float4 shape_param;            // Offset:  272
    float3 param2;                 // Offset:  288
    uint boneAddress;              // Offset:  300
    uint camera_type;              // Offset:  304
    uint uv_mask;                  // Offset:  308
    float rim_alpha;               // Offset:  312
    float depth_fade_width_inv;    // Offset:  316
    float4 camera_fade_param;      // Offset:  320
    float3 hsv;                    // Offset:  336
    float pad;                     // Offset:  348
};

struct ParticleHeader
{
    uint tag;                      // Offset:    0
    float depth;                   // Offset:    4
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

cbuffer cb_local : register(b3)
{
  uint offset_index_ : packoffset(c0);
}

StructuredBuffer<Particle> particles_g : register(t11);
StructuredBuffer<ParticleHeader> particle_headers_g : register(t12);
StructuredBuffer<uint> instance_offsets_g : register(t13);


// 3Dmigoto declarations
#define cmp -


void main(
  float3 v0 : POSITION0,
  float3 v1 : NORMAL0,
  float2 v2 : TEXCOORD0,
  uint v3 : SV_InstanceID0,
  out float4 o0 : SV_Position0,
  out float4 o1 : TEXCOORD0,
  out float4 o2 : TEXCOORD1,
  out float4 o3 : COLOR0,
  out float4 o4 : COLOR1,
  out float4 o5 : TEXCOORD4,
  out uint o6 : TEXCOORD9)
{
// Needs manual fix for instruction:
// unknown dcl_: dcl_input_sgv v3.x, instance_id
  float4 r0,r1,r2,r3;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = 1 + -v2.y;
  r0.y = instance_offsets_g[offset_index_].x;
  r0.y = (int)r0.y + (int)v3.x;
  r0.z = particle_headers_g[r0.y].tag;
  o6.x = r0.y;
  r0.y = (int)r0.z & 0x000fffff;
  r0.z = particles_g[r0.y].shape_param.x;
  r0.w = particles_g[r0.y].shape_param.y;
  r0.w = r0.w + -r0.z;
  r0.x = r0.x * r0.w + r0.z;
  r1.xz = v0.xz * r0.xx;
  r1.y = v0.y;
  r1.w = 1;
  r2.x = particles_g[r0.y].world._m03;
  r2.y = particles_g[r0.y].world._m13;
  r2.z = particles_g[r0.y].world._m23;
  r2.w = particles_g[r0.y].world._m33;
  r2.w = dot(r1.xyzw, r2.xyzw);
  r3.x = particles_g[r0.y].world._m00;
  r3.y = particles_g[r0.y].world._m10;
  r3.z = particles_g[r0.y].world._m20;
  r3.w = particles_g[r0.y].world._m30;
  r2.x = dot(r1.xyzw, r3.xyzw);
  r3.x = particles_g[r0.y].world._m01;
  r3.y = particles_g[r0.y].world._m11;
  r3.z = particles_g[r0.y].world._m21;
  r3.w = particles_g[r0.y].world._m31;
  r2.y = dot(r1.xyzw, r3.xyzw);
  r3.x = particles_g[r0.y].world._m02;
  r3.y = particles_g[r0.y].world._m12;
  r3.z = particles_g[r0.y].world._m22;
  r3.w = particles_g[r0.y].world._m32;
  r2.z = dot(r1.xyzw, r3.xyzw);
  r0.x = particles_g[r0.y].camera_type;
  r0.z = particles_g[r0.y].uv_mask;
  r0.w = particles_g[r0.y].rim_alpha;
  r0.x = (uint)r0.x << 2;
  o0.x = dot(r2.xyzw, view_proj_[r0.x/4]._m00_m10_m20_m30);
  o0.y = dot(r2.xyzw, view_proj_[r0.x/4]._m01_m11_m21_m31);
  o0.z = dot(r2.xyzw, view_proj_[r0.x/4]._m02_m12_m22_m32);
  o0.w = dot(r2.xyzw, view_proj_[r0.x/4]._m03_m13_m23_m33);
  r0.x = dot(view_[r0.x/4]._m02_m12_m22_m32, r2.xyzw);
  o5.xyzw = r2.xyzw;
  r1.x = view_inv_._m30 + -r2.x;
  r1.y = view_inv_._m31 + -r2.y;
  r1.z = view_inv_._m32 + -r2.z;
  o1.xy = v2.xy * float2(1,-1) + float2(0,1);
  if (8 == 0) r1.w = 0; else if (8+8 < 32) {   r1.w = (uint)r0.z << (32-(8 + 8)); r1.w = (uint)r1.w >> (32-8);  } else r1.w = (uint)r0.z >> 8;
  r1.w = (uint)r1.w;
  o1.z = r1.w * 0.00392156886 + 9.99999975e-06;
  r0.z = (int)r0.z & 255;
  r0.z = (uint)r0.z;
  o1.w = r0.z * 0.00392156886 + 9.99999975e-06;
  o2.x = particles_g[r0.y].uv[0].x;
  o2.y = particles_g[r0.y].uv[0].y;
  o2.z = particles_g[r0.y].uv[0].z;
  o2.w = particles_g[r0.y].uv[0].w;
  r0.z = dot(r1.xyz, r1.xyz);
  r0.z = rsqrt(r0.z);
  r1.xyz = r1.xyz * r0.zzz;
  r2.x = particles_g[r0.y].world._m00;
  r2.y = particles_g[r0.y].world._m10;
  r2.z = particles_g[r0.y].world._m20;
  r2.x = dot(v1.xyz, r2.xyz);
  r3.x = particles_g[r0.y].world._m01;
  r3.y = particles_g[r0.y].world._m11;
  r3.z = particles_g[r0.y].world._m21;
  r2.y = dot(v1.xyz, r3.xyz);
  r3.x = particles_g[r0.y].world._m02;
  r3.y = particles_g[r0.y].world._m12;
  r3.z = particles_g[r0.y].world._m22;
  r2.z = dot(v1.xyz, r3.xyz);
  r0.z = dot(r2.xyz, r2.xyz);
  r0.z = rsqrt(r0.z);
  r2.xyz = r2.xyz * r0.zzz;
  r0.z = dot(r2.xyz, r1.xyz);
  r0.z = log2(abs(r0.z));
  r0.z = abs(r0.w) * r0.z;
  r0.z = exp2(r0.z);
  r0.z = 1 + -r0.z;
  r0.z = r0.z * r0.z;
  r0.z = r0.z * r0.z;
  r1.x = cmp(0 < abs(r0.w));
  r0.w = cmp(r0.w < 0);
  r0.z = r1.x ? r0.z : 0;
  r1.x = 1 + -r0.z;
  r0.z = r0.w ? r0.z : r1.x;
  r1.x = particles_g[r0.y].camera_fade_param.x;
  r1.y = particles_g[r0.y].camera_fade_param.y;
  r1.z = particles_g[r0.y].camera_fade_param.z;
  r1.w = particles_g[r0.y].camera_fade_param.w;
  r0.x = -r1.x + -r0.x;
  r1.xz = r1.zw + -r1.xy;
  r0.x = saturate(r0.x / r1.x);
  r0.x = r0.x * r1.z + r1.y;
  r0.x = r0.z * r0.x;
  r1.x = particles_g[r0.y].color.x;
  r1.y = particles_g[r0.y].color.y;
  r1.z = particles_g[r0.y].color.z;
  r1.w = particles_g[r0.y].color.w;
  o3.w = r1.w * r0.x;
  o3.xyz = r1.xyz;
  r1.x = particles_g[r0.y].specular.x;
  r1.y = particles_g[r0.y].specular.y;
  r1.z = particles_g[r0.y].specular.z;
  r1.w = particles_g[r0.y].specular.w;
  o4.w = particles_g[r0.y].hsv.z;
  o4.xyz = r1.xyz * r1.www;
  return;
}