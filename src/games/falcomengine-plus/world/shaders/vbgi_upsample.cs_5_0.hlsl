// vbgi_upsample.cs_5_0.hlsl - the RTGI upsample (world_rtgi_upsample.cs_5_0.hlsl) built for GTVBGI with the nearest valid
// tap fallback: where the edge stop rejects all four taps (summed weight <= 1e-6), the valid tap with the largest bilinear
// weight is used instead of 0. RTGI, RT Local Shadows and RT Sun build the shared source without the define.
#define RTGI_UPSAMPLE_NEAREST_FALLBACK
#include "world_rtgi_upsample.cs_5_0.hlsl"
