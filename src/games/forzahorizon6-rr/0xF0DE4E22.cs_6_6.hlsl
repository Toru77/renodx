// ssrfiltermiphorizontal_0xF0DE4E22 — SSR_FullScreen_FilterMip_Horizontal.
// Preserve each source mip texel while skipping the horizontal blur kernel.

Texture2D<float4> g_source : register(t0);
RWTexture2D<float4> g_output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    uint width;
    uint height;
    g_source.GetDimensions(width, height);
    if ((dispatch_id.x >= width) || (dispatch_id.y >= height))
    {
        return;
    }
    g_output[dispatch_id.xy] = g_source.Load(int3(dispatch_id.xy, 0));
}
