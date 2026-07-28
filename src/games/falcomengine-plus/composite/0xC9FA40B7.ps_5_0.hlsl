// ── Dummy passthrough for 0xC9FA40B7 (UI-less composite shader) ──
// This shader is never executed — the on_replace callback runs motion blur
// compute and returns false to skip the draw entirely.
// This file exists only to satisfy the build system's embedding requirement.
//
// SPDX-License-Identifier: MIT

Texture2D<float4> tex0 : register(t0);

float4 main(float4 pos : SV_Position) : SV_Target0
{
    return tex0.Load(int3(pos.xy, 0));
}
