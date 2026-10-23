// world_rtls_filter.cs_5_0.hlsl - the RTGI spatial filter (world_rtgi_filter.cs_5_0.hlsl) built with the RT Local Shadows
// penumbra edge stop: each tap is also weighted by exp(-k * max |shadow - centre shadow|) over x (point) and y (spot),
// k = f_full.w. The filter input holds the shadow term in rgb (x point, y spot, z coverage).
#define WORLD_FILTER_VALUE_STOP
#include "world_rtgi_filter.cs_5_0.hlsl"
