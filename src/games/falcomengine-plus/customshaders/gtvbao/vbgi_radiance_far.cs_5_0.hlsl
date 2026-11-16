// falcomengine-plus VBGI - radiance prepass with far-field feedback (M4): splits last frame's joint denoised GI into
// near and far shares (far raw / joint raw luminance, 3x3) with separate weights, and firefly-clamps the fed-back GI.
#define VBGI_FAR_FEEDBACK 1
#include "vbgi_radiance.cs_5_0.hlsl"
