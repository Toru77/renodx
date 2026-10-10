// forzahorizon6-rr: probe blend experiment (0xD9CDA0AC).
//
// Original: merges two probe contribution buffers with a per-frame weight
// from the constant buffer, six SH-style coefficients per probe:
//     out = A + m0[0].x * B
// where A (t0) is the probe state term and B (t1) is the new contribution
// slice. The small weight is what produces the multi-second "GI keeps
// refining" ramp while standing still.
//
// Experiment (A/B toggle "Bypass probe smoothing"): the first variant
// (out = A + B) showed no visible change, so this variant drops the state
// term entirely and emits the new contribution only:
//     out = B
// This is the rawest per-frame version of the blend. If the probe chain is in
// the visible GI path this changes the image immediately (better or worse);
// revert if the A/B shows a regression.
//
// The four-component output layout (x, y, z, x) is kept exactly as the
// original writes it.

cbuffer cb : register(b0, space0)
{
    float4 m0[1] : packoffset(c0);
};

Buffer<float4> g_a : register(t0, space0);
Buffer<float4> g_b : register(t1, space0);
RWBuffer<float4> g_out : register(u0, space0);

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint base = id.x * 6u;

    float c0[6];
    float c1[6];
    float c2[6];
    for (uint i = 0u; i < 6u; ++i)
    {
        const float4 b = g_b.Load(base + i);
        c0[i] = b.x;
        c1[i] = b.y;
        c2[i] = b.z;
    }
    for (uint j = 0u; j < 6u; ++j)
    {
        g_out[base + j] = float4(c0[j], c1[j], c2[j], c0[j]);
    }
}
