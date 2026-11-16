// dyncube_solid.cs_5_0.hlsl — Phase 0A solid-face test cube
// Writes per-face solid color: +X red, -X green, +Y blue, -Y yellow, +Z magenta, -Z cyan
// Dispatch: (size/8, size/8, 6) numthreads(8,8,1) ThreadID.z = face index
// Output: RWTexture2DArray<float4> 128x128x6 RGBA16F (or RGBA8)

RWTexture2DArray<float4> g_outCube : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint w, h, d;
    g_outCube.GetDimensions(w, h, d);
    if (dtid.x >= w || dtid.y >= h || dtid.z >= 6)
        return;

    float4 color;
    switch (dtid.z)
    {
        case 0: color = float4(1, 0, 0, 1); break; // +X red
        case 1: color = float4(0, 1, 0, 1); break; // -X green
        case 2: color = float4(0, 0, 1, 1); break; // +Y blue
        case 3: color = float4(1, 1, 0, 1); break; // -Y yellow
        case 4: color = float4(1, 0, 1, 1); break; // +Z magenta
        default: color = float4(0, 1, 1, 1); break; // -Z cyan
    }
    g_outCube[dtid] = color;
}
