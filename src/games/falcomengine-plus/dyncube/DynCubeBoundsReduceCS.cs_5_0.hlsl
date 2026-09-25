cbuffer DynCubeBoundsCB : register(b13)
{
    float g_pass;
    float g_posScale;
    float g_reset;
    float g_scratchCount;
    float g_contribThreshold;
};

Texture2DArray<float4> g_posTex : register(t0);
Texture2DArray<float> g_contribTex : register(t1);
Texture2DArray<float4> g_charmaskTex : register(t2);
Texture2D<float4> g_camCurTex : register(t3);
RWStructuredBuffer<float4> g_scratch : register(u0);
RWStructuredBuffer<float4> g_bounds : register(u1);

static const float kDynCubeValidityMaxDist = 100.0;
groupshared uint s_valid[64];

bool IsValidCapture(uint x, uint y, uint face, uint w, uint h, float3 camCur)
{
    if (x >= w || y >= h) return false;
    float4 p = g_posTex.Load(int4(x, y, face, 0));
    float contrib = g_contribTex.Load(int4(x, y, face, 0));
    if (p.a <= 0.5f || contrib <= g_contribThreshold) return false;
    float4 mask = g_charmaskTex.Load(int4(x, y, face, 0));
    if (mask.x > 0.5f || !all(isfinite(p.xyz))) return false;
    float3 worldPos = p.xyz / max(g_posScale, 1e-9f);
    return all(isfinite(worldPos)) && distance(worldPos, camCur) <= kDynCubeValidityMaxDist;
}

[numthreads(8, 8, 1)]
void main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    uint w, h, el;
    g_posTex.GetDimensions(w, h, el);
    uint li = gtid.y * 8 + gtid.x;
    if (g_pass < 0.5f)
    {
        uint x = gid.x * 8 + gtid.x;
        uint y = gid.y * 8 + gtid.y;
        float3 camCur = g_camCurTex.Load(int3(0, 0, 0)).xyz;
        s_valid[li] = IsValidCapture(x, y, gid.z, w, h, camCur) ? 1u : 0u;
    }
    else
    {
        uint count = (uint)max(g_scratchCount, 1.0f);
        uint value = 0u;
        for (uint i = li; i < count; i += 64u)
            value += (g_scratch[i].x > 0.5f) ? 1u : 0u;
        s_valid[li] = value;
    }
    GroupMemoryBarrierWithGroupSync();
    for (uint stride = 32u; stride > 0u; stride >>= 1u)
    {
        if (li < stride) s_valid[li] += s_valid[li + stride];
        GroupMemoryBarrierWithGroupSync();
    }
    if (li == 0u)
    {
        if (g_pass < 0.5f)
        {
            uint gy = (h + 7u) / 8u;
            uint gx = (w + 7u) / 8u;
            uint groupIndex = (gid.z * gy + gid.y) * gx + gid.x;
            g_scratch[groupIndex] = float4((float)s_valid[0], 0.0f, 0.0f, 0.0f);
        }
        else
        {
            g_bounds[1] = float4(0.0f, 0.0f, 0.0f, (float)s_valid[0]);
        }
    }
}
