// ssrcheckerboardtidy_0x313E9A06 — SSR_FullScreen_TidyCheckerboard_Rough_MipFiltered.
// This pass clamps selected checkerboard pixels in place. An empty dispatch
// preserves the preceding pass's unfiltered reflection values.

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
}
