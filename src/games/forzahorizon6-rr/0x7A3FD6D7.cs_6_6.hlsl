// forzahorizon6-rr: tile-refresh expansion replacement for 0x7A3FD6D7.
//
// The original expands only the tiles present in the tile classifier's sparse
// worklist (mask-gated "dirty" tiles) into the per-pixel ray queues, which is
// what lets a standing-still view converge over several frames while a moving
// view keeps re-dirtying tiles. This replacement keeps the original's class
// decode, per-pixel checkerboard sub-sampling, route flag (space40 globals
// index 36) and queue record layout byte-for-byte, but derives the tile
// coordinates from the dispatch index instead of reading the sparse list.
//
// Each queue holds at most 230400 uint4 records (3686400 bytes), so the full
// 160x90 grid cannot be expanded every frame without overflowing it. The
// dispatch instead covers a rotating quarter of the tile grid (3600 tiles =
// up to 230400 records at full density) selected by the frame counter, so
// every tile is expanded within four frames at the engine's own maximum
// per-frame density. The paired replacement of 0x9BFFD1F7 forces the
// expansion dispatch to that window. Experimental.

cbuffer _33_35 : register(b3, space0)
{
    float4 _35_m0[69] : packoffset(c0);
};

Texture2D<uint4> _9[] : register(t0, space40);
Texture2D<uint4> _14 : register(t1, space0);
globallycoherent RWBuffer<uint> _17 : register(u1, space0);
RWBuffer<uint4> _20 : register(u2, space0);
globallycoherent RWBuffer<uint> _21 : register(u3, space0);
RWBuffer<uint4> _22 : register(u4, space0);
globallycoherent RWBuffer<uint> _23 : register(u5, space0);
RWBuffer<uint4> _24 : register(u6, space0);
globallycoherent RWBuffer<uint> _25 : register(u7, space0);
RWBuffer<uint4> _26 : register(u8, space0);

static const uint kTileGridWidth = 160u;
static const uint kGridTiles = kTileGridWidth * 90u;
static const uint kWindowTiles = 3600u;      // dispatch covers this many tiles
static const uint kQueueCapacity = 230400u;  // 3686400 bytes / 16 bytes per record

static uint3 gl_WorkGroupID;
static uint3 gl_LocalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint3 gl_LocalInvocationID : SV_GroupThreadID;
};

void comp_main()
{
    const uint entry = (gl_WorkGroupID.y << 4u) + gl_WorkGroupID.x;
    if (entry >= kWindowTiles) return;
    const uint window = asuint(_35_m0[54u].w) & 3u;
    const uint tile_index = entry + window * kWindowTiles;
    const uint tile_x = tile_index % kTileGridWidth;
    const uint tile_y = tile_index / kTileGridWidth;
    const uint px = (tile_x << 3u) + gl_LocalInvocationID.x;
    const uint py = (tile_y << 3u) + gl_LocalInvocationID.y;

    const uint cls = _14.Load(int3(uint2(tile_x, tile_y), 0u)).x;
    uint select_a = 0u;
    bool select_b = false;
    uint flag29 = 0u;
    bool flag30 = false;
    if ((cls & 2u) == 0u)
    {
        const bool no_y_parity = (cls & 4u) == 0u;
        const uint y_parity = (py & 1u) ^ 1u;
        const uint a_by_y = no_y_parity ? 1u : y_parity;
        const uint b_by_y = no_y_parity ? 0u : y_parity;
        if ((cls & 8u) == 0u)
        {
            select_a = a_by_y;
            flag29 = 0u;
            flag30 = b_by_y != 0u;
        }
        else
        {
            const uint x_parity = px & 1u;
            select_a = (x_parity != 0u) ? 0u : a_by_y;
            flag29 = x_parity ^ 1u;
            flag30 = b_by_y != 0u;
        }
    }
    else
    {
        select_a = 0u;
        select_b = (((px + py) + asuint(_35_m0[54u].w)) & 1u) == 0u;
    }

    const bool selected_a = select_a != 0u;
    const uint index_a = WavePrefixCountBits(selected_a);
    const uint count_a = WaveActiveCountBits(selected_a);
    const uint index_b = WavePrefixCountBits(select_b);
    const uint count_b = WaveActiveCountBits(select_b);

    const bool route_b = WaveActiveAnyTrue(
        _9[36u].Load(int3(uint2(tile_x, tile_y), 0u)).x != 0u);

    uint base_a = 0u;
    uint base_b = 0u;
    if (WaveIsFirstLane())
    {
        uint previous_a = 0u;
        uint previous_b = 0u;
        if (route_b)
        {
            InterlockedAdd(_23[0u], count_a, previous_a);
            InterlockedAdd(_25[0u], count_b, previous_b);
        }
        else
        {
            InterlockedAdd(_17[0u], count_a, previous_a);
            InterlockedAdd(_21[0u], count_b, previous_b);
        }
        base_a = previous_a;
        base_b = previous_b;
    }
    base_a = WaveReadLaneFirst(base_a);
    base_b = WaveReadLaneFirst(base_b);

    if (selected_a && (base_a + index_a) < kQueueCapacity)
    {
        const uint record_a =
            (((((py << 15u) & 536838144u) | (px & 32767u))
              | (flag29 << 29u)) | ((flag30 ? 1u : 0u) << 30u))
            | (((flag29 != 0u) && flag30) ? 2147483648u : 0u);
        if (route_b)
        {
            _24[base_a + index_a] = record_a.xxxx;
        }
        else
        {
            _20[base_a + index_a] = record_a.xxxx;
        }
    }
    if (select_b && (base_b + index_b) < kQueueCapacity)
    {
        const uint record_b = ((py << 15u) & 536838144u) | (px & 32767u);
        if (route_b)
        {
            _26[base_b + index_b] = record_b.xxxx;
        }
        else
        {
            _22[base_b + index_b] = record_b.xxxx;
        }
    }
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_WorkGroupID = stage_input.gl_WorkGroupID;
    gl_LocalInvocationID = stage_input.gl_LocalInvocationID;
    comp_main();
}
