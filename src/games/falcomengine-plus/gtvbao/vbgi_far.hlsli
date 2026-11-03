#ifndef SRC_GAMES_FALCOMENGINE_PLUS_GTVBAO_VBGI_FAR_HLSLI_
#define SRC_GAMES_FALCOMENGINE_PLUS_GTVBAO_VBGI_FAR_HLSLI_
// VBGI far field (ReSTIR GI on the escape directions). Shared by the far main variant and the far passes.
// GTVBAO view space is the engine view with Z negated (gtvbao_common.hlsl TransformNormalToView). Far samples
// integrate in the cosine-weighted measure of the analytic VBGI output, so W x L adds directly to the raw GI.
static const uint kVbgiFarSteps = 16u;
float3 VbgiFarViewToWorld(float3 pv) { return mul(float4(pv.x, pv.y, -pv.z, 1.0f), viewInv_g).xyz; }
float3 VbgiFarWorldToView(float3 pw) { const float3 e = mul(float4(pw, 1.0f), view_g).xyz; return float3(e.x, e.y, -e.z); }
float2 VbgiFarViewToUv(float3 pv, GTAOConstants consts) { return (pv.xy / pv.z - consts.NDCToViewAdd) / consts.NDCToViewMul; }
float VbgiFarLuminance(float3 c) { return dot(c, float3(0.2126f, 0.7152f, 0.0722f)); }
#endif
