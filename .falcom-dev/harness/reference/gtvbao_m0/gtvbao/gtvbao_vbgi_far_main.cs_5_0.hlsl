// falcomengine-plus VBGI far field - Pass 2 plus one far-field candidate per texel (GTVBAO_GI_FAR blocks in GTVBAO.hlsli).
#define GTVBAO_GI 1
#define GTVBAO_GI_FAR 1
RWTexture2D<float4> g_outFarPos : register(u2);  // xyz world sample point, w W (0 = none)
RWTexture2D<float4> g_outFarRad : register(u3);  // rgb sender radiance, a M (1)
RWTexture2D<float4> g_outFarNrm : register(u4);  // xyz world sender normal, w 1 = sector or round-trip mismatch
#include "gtvbao_main.cs_5_0.hlsl"
