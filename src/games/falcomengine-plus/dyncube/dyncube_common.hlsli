// dyncube_common.hlsli — shared Dynamic Cubemap / SSR scalar helpers.
//
// Inclusion contract: the including translation unit must declare
// `float4x4 proj_g` (cb_scene, packoffset c8) before including this file.
// The functions below reference it directly so moved call sites keep
// character-identical bodies (see FalcomSSRCS / FalcomSSRBlurCS).
// No resources, samplers, or cbuffers are declared here.

#ifndef __DYNCUBE_COMMON_HLSLI__
#define __DYNCUBE_COMMON_HLSLI__

static const float DynCubeFltMax = 3.402823466e+38;

// ── Depth linearization (identical math to the validated SSR/GTVBAO path; handles
//    standard and reversed Z by sign guard; gives positive view-space distance). ──
void DynCubeGetDepthUnpackConsts(out float mul_c, out float add_c)
{
    mul_c = -proj_g[3][2];
    add_c =  proj_g[2][2];
    if (mul_c * add_c < 0.0) add_c = -add_c;
}

float DynCubeLinearizeDepth(float ndc_depth, float mul_c, float add_c)
{
    float denom = add_c - ndc_depth;
    float z = (abs(denom) > 1e-8) ? (mul_c / denom) : 0.0;
    z = max(z, 0.0);
    return isfinite(z) && z > 0.0 ? z : DynCubeFltMax;
}

// SSR depth validity convention (matches FalcomSSRCS sky test).
bool DynCubeIsSceneDepthValid(float hw_depth)
{
    return hw_depth > 0.0 && hw_depth < 1.0;
}

#endif // __DYNCUBE_COMMON_HLSLI__
