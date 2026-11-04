// world_rtao_svgf_input.cs_5_0.hlsl - RTAO trace raw AO to the SVGF raw layout (SVGF M2b). t0: trace-size raw AO (r16_float,
// -1 = not traced). u0: rgba16f, rgb = visibility, a = 1 when traced and -1 when not (the reprojection skips a < 0).
// b12 c0.xy: trace width, height.
Texture2D<float> g_rtao_raw : register(t0);
RWTexture2D<float4> g_svgf_raw : register(u0);
cbuffer cb_svgf_input : register(b12) { float4 i_size : packoffset(c0); };
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= uint2(i_size.xy))) return;
    const float ao = g_rtao_raw[id.xy];
    g_svgf_raw[id.xy] = ao < 0.0 ? float4(1.0, 1.0, 1.0, -1.0) : float4(ao, ao, ao, 1.0);
}
