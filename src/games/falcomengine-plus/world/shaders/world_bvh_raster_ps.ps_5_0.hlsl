// world_bvh_raster_ps.ps_5_0.hlsl - M2b debug coloring.
//
// Modes: 1 instance id, 2 mesh id, 3 world position, 4 flat normal,
// 6 shaded (fixed debug light, no real lighting).

cbuffer cb_raster : register(b13)
{
    float4x4 g_view_proj;
    uint g_mode;
    uint g_mesh_id;
    uint g_group_offset;
    uint g_spare0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 world : TEXCOORD0;
    nointerpolation uint instance_id : TEXCOORD1;
    nointerpolation uint mesh_id : TEXCOORD2;
};

static const float3 kDebugLightDirection = normalize(float3(0.4, 0.8, 0.45));

float3 HueToRgb(float hue)
{
    const float3 k = float3(1.0, 2.0 / 3.0, 1.0 / 3.0);
    const float3 p = abs(frac(hue + k) * 6.0 - 3.0);
    return saturate(p - 1.0);
}

float3 IdColor(uint id)
{
    return HueToRgb(frac(float(id) * 0.618034));
}

float3 PositionColor(float3 world)
{
    return HueToRgb(frac(abs(world.x * 0.031 + world.y * 0.017 + world.z * 0.023)));
}

float4 main(VSOutput input) : SV_Target
{
    const float3 normal = normalize(cross(ddx(input.world), ddy(input.world)));
    float3 color = 0.0;
    if (g_mode == 1u)
    {
        color = IdColor(input.instance_id);
    }
    else if (g_mode == 2u)
    {
        color = IdColor(input.mesh_id);
    }
    else if (g_mode == 3u)
    {
        color = PositionColor(input.world);
    }
    else if (g_mode == 6u)
    {
        const float lambert = 0.35 + 0.65 * saturate(dot(normal, kDebugLightDirection));
        color = 0.85 * lambert;
    }
    else
    {
        color = normal * 0.5 + 0.5;
    }
    return float4(color, 1.0);
}
