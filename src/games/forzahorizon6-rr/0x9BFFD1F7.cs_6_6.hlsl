// forzahorizon6-rr: tile-refresh dispatch-args replacement for 0x9BFFD1F7.
//
// The original builds the three per-list expansion dispatch argument triples
// from the tile classifier's dirty-tile counts (list A rows = ceil(count/16)),
// pads each sparse list with sentinels and resets the counters. This
// replacement keeps the padding and the counter reset exactly as they were
// and only forces list A's expansion dispatch to cover a rotating quarter of
// the tile grid every frame (16 groups wide x 225 rows = 3600 tiles, the
// largest window that still fits the per-queue record capacity); the paired
// replacement of 0x7A3FD6D7 derives each tile from that dispatch index and
// rotates the window with the frame counter. Lists B and C keep their
// original counts. Experimental.

RWBuffer<uint> list_counts : register(u0, space0);
RWBuffer<uint4> list_a : register(u1, space0);
RWBuffer<uint4> list_b : register(u2, space0);
RWBuffer<uint4> list_c : register(u3, space0);
RWBuffer<uint4> expand_args : register(u4, space0);

// Writes one dispatch argument triple (16 groups wide, ceil(count/16) rows)
// or a zero triple when the list is empty, mirroring the original.
void WriteExpansionArgs(uint offset, uint count)
{
    if (count == 0u)
    {
        expand_args[offset + 0u] = uint4(0u, 0u, 0u, 0u);
        expand_args[offset + 1u] = uint4(0u, 0u, 0u, 0u);
        expand_args[offset + 2u] = uint4(0u, 0u, 0u, 0u);
        return;
    }
    const uint rows = (count + 15u) >> 4u;
    expand_args[offset + 0u] = uint4(16u, 16u, 16u, 16u);
    expand_args[offset + 1u] = uint4(rows, rows, rows, rows);
    expand_args[offset + 2u] = uint4(1u, 1u, 1u, 1u);
}

void PadList(RWBuffer<uint4> list, uint count)
{
    if (count == 0u) return;
    const uint padded = (count + 15u) & ~15u;
    for (uint i = count; i < padded; ++i)
    {
        list[i] = uint4(~0u, ~0u, ~0u, ~0u);
    }
}

[numthreads(1, 1, 1)]
void main()
{
    const uint count_a = list_counts[0u];
    const uint count_b = list_counts[1u];
    const uint count_c = list_counts[2u];

    // List A: a rotating 3600-tile window of the full grid, independent of
    // the classifier count (16 x 225 groups = 3600 tiles).
    expand_args[0u] = uint4(16u, 16u, 16u, 16u);
    expand_args[1u] = uint4(225u, 225u, 225u, 225u);
    expand_args[2u] = uint4(1u, 1u, 1u, 1u);
    PadList(list_a, count_a);

    WriteExpansionArgs(3u, count_b);
    PadList(list_b, count_b);

    WriteExpansionArgs(6u, count_c);
    PadList(list_c, count_c);

    list_counts[0u] = 0u;
    list_counts[1u] = 0u;
    list_counts[2u] = 0u;
}
