// gtvbao_svgf_input.cs_5_0.hlsl - GTVBAO AO byte to the SVGF raw layout (SVGF M2a). t0: main-pass byte of
// visibility / GT_VBAO_OCCLUSION_TERM_SCALE (final_pass 0). u0: rgba16f, rgb = visibility (the scale is applied
// here, once), a = 1 (every AO texel is traced). b12 c0.xy: AO width, height.
#include "../../gtvbao/GTVBAO.h"
Texture2D<uint> g_ao_byte : register(t0);
RWTexture2D<float4> g_svgf_raw : register(u0);
cbuffer cb_svgf_input : register(b12) { float4 i_size : packoffset(c0); };
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= uint2(i_size.xy))) return;
    const float ao = (float)g_ao_byte[id.xy] * (GT_VBAO_OCCLUSION_TERM_SCALE / 255.0);
    g_svgf_raw[id.xy] = float4(ao, ao, ao, 1.0);
}
