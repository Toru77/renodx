// vbgi_upsample.cs_5_0.hlsl - the selectable shared RTGI upsample built for GTVBGI. The nearest-valid fallback is
// controlled at runtime; RTGI, RT Local Shadows and RT Sun build the shared source without fallback support.
#define RTGI_UPSAMPLE_NEAREST_FALLBACK
#include "world_rtgi_upsample.cs_5_0.hlsl"
