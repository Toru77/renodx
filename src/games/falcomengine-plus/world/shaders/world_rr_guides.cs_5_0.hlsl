// world_rr_guides.cs_5_0.hlsl - DLSS Ray Reconstruction guide buffers (world/docs/ROADMAP.md, Reference: DLSS Ray Reconstruction).
//
// One thread per pixel over the game depth size, dispatched inline in the lighting hook. Writes the RR guides:
// u0 NormalRoughness (view-space normal, roughness in w), u1 Albedo, u2 SpecularAlbedo (DLSS-RR Integration Guide,
// Specular Albedo Calculation). The lighting output is not changed.
//
// Camera: the game scene CBV bound as b0 (view_g, viewInv_g, viewProjInv_g).

#include "world_rt_lamps.hlsli"
#include "../../include/mrt_normal.hlsli"

Texture2D<float4> g_color : register(t0);
Texture2D<uint4> g_mrt0 : register(t1);
Texture2D<uint2> g_mrt2 : register(t2);
StructuredBuffer<GameDeferredParam> g_deferred : register(t3);
Texture2D<float4> g_depth : register(t4);

RWTexture2D<float4> g_normal_roughness : register(u0);
RWTexture2D<float4> g_albedo : register(u1);
RWTexture2D<float4> g_specular : register(u2);
RWTexture2D<float> g_depth_copy : register(u3);  // raw hardware depth, typed (the game's depth is typeless)

cbuffer cb_scene : register(b0)
{
    float4x4 view_g : packoffset(c0);
    float4x4 viewInv_g : packoffset(c4);
    float4x4 viewProjInv_g : packoffset(c20);
};

cbuffer cb_rr_guides : register(b12)
{
    float4 g_rr_size : packoffset(c0);  // width, height
};

// Copied from the DLSS-RR Integration Guide appendix (Specular Albedo Calculation).
float3 EnvBRDFApprox2(float3 SpecularColor, float alpha, float NoV)
{
    NoV = abs(NoV);
    // [Ray Tracing Gems, Chapter 32]
    float4 X;
    X.x = 1.f;
    X.y = NoV;
    X.z = NoV * NoV;
    X.w = NoV * X.z;
    float4 Y;
    Y.x = 1.f;
    Y.y = alpha;
    Y.z = alpha * alpha;
    Y.w = alpha * Y.z;
    float2x2 M1 = float2x2(0.99044f, -1.28514f, 1.29678f, -0.755907f);
    float3x3 M2 = float3x3(1.f, 2.92338f, 59.4188f, 20.3225f, -27.0302f, 222.592f, 121.563f, 626.13f, 316.627f);
    float2x2 M3 = float2x2(0.0365463f, 3.32707, 9.0632f, -9.04756);
    float3x3 M4 = float3x3(1.f, 3.59685f, -1.36772f, 9.04401f, -16.3174f, 9.22949f, 5.56589f, 19.7886f, -20.2123f);
    float bias = dot(mul(M1, X.xy), Y.xy) * rcp(dot(mul(M2, X.xyw), Y.xyw));
    float scale = dot(mul(M3, X.xy), Y.xy) * rcp(dot(mul(M4, X.xzw), Y.xyw));
    // This is a hack for specular reflectance of 0
    bias *= saturate(SpecularColor.g * 50);
    return mad(SpecularColor, max(0, scale), max(0, bias));
}

// The texel of a source of size dim at normalized uv, as the lighting shader maps it (lighting_0xCA3D8596.ps_5_0.hlsl:243-260).
uint2 UvTexel(float2 uv, uint2 dim)
{
    return min(uint2(uv * float2(dim)), dim - 1u);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint2 px = id.xy;
    if (px.x >= (uint)g_rr_size.x || px.y >= (uint)g_rr_size.y) return;
    g_depth_copy[px] = g_depth.Load(int3(px, 0)).x;

    const float2 uv = (float2(px) + 0.5) / g_rr_size.xy;
    uint2 dim = uint2(0u, 0u);
    g_mrt0.GetDimensions(dim.x, dim.y);
    const float depth = g_depth.Load(int3(px, 0)).x;
    const float3 n_world = DecodeFalcomMrtNormalAsGame(g_mrt0.Load(int3(UvTexel(uv, dim), 0)).xy);
    if (depth <= 0.0 || dot(n_world, n_world) < 0.5)
    {
        g_normal_roughness[px] = float4(0.0, 0.0, 1.0, 1.0);
        g_albedo[px] = float4(0.5, 0.5, 0.5, 1.0);
        g_specular[px] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    g_mrt2.GetDimensions(dim.x, dim.y);
    const GameDeferredParam mat = g_deferred[min(0x4e1fu, g_mrt2.Load(int3(UvTexel(uv, dim), 0)).x)];
    const float roughness = saturate(mat.tail[0].z);
    const float3 spec_color = mat.head[1].xyz;

    const float4 world_h = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), viewProjInv_g);
    const float3 P = world_h.xyz / world_h.w;
    const float3 camera = mul(float4(0.0, 0.0, 0.0, 1.0), viewInv_g).xyz;
    const float3 n = normalize(n_world);
    const float3 n_view = normalize(mul(float4(n, 0.0), view_g).xyz);

    g_normal_roughness[px] = float4(n_view, roughness);
    g_color.GetDimensions(dim.x, dim.y);
    g_albedo[px] = float4(saturate(g_color.Load(int3(UvTexel(uv, dim), 0)).rgb), 1.0);
    const float3 spec = EnvBRDFApprox2(spec_color, roughness * roughness, dot(n, normalize(camera - P)));
    g_specular[px] = float4(saturate(spec), 1.0);
}
