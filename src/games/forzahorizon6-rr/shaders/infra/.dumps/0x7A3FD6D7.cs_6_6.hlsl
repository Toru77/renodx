static uint _123;

cbuffer _31_33 : register(b3, space0)
{
    float4 _33_m0[69] : packoffset(c0);
};

Texture2D<uint4> _9[] : register(t0, space40);
Buffer<uint4> _12 : register(t0, space0);
Texture2D<uint4> _14 : register(t1, space0);
globallycoherent RWBuffer<uint> _17 : register(u1, space0);
RWBuffer<uint4> _20 : register(u2, space0);
globallycoherent RWBuffer<uint> _21 : register(u3, space0);
RWBuffer<uint4> _22 : register(u4, space0);
globallycoherent RWBuffer<uint> _23 : register(u5, space0);
RWBuffer<uint4> _24 : register(u6, space0);
globallycoherent RWBuffer<uint> _25 : register(u7, space0);
RWBuffer<uint4> _26 : register(u8, space0);

static uint3 gl_WorkGroupID;
static uint3 gl_LocalInvocationID;
static uint4 gl_SubgroupLtMask;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint3 gl_LocalInvocationID : SV_GroupThreadID;
};

void comp_main()
{
    uint4 _54 = _12.Load((gl_WorkGroupID.y << 4u) + gl_WorkGroupID.x);
    uint _55 = _54.x;
    uint _56 = _55 & 65535u;
    uint _58 = _55 >> 16u;
    uint _63 = (_56 << 3u) + gl_LocalInvocationID.x;
    uint _64 = (_58 << 3u) + gl_LocalInvocationID.y;
    uint4 _75 = _14.Load(int3(uint2(_56, _58), 0u));
    uint _77 = _75.x;
    uint _100;
    bool _102;
    uint _104;
    uint _106;
    if ((_77 & 2u) == 0u)
    {
        bool _82 = (_77 & 4u) == 0u;
        uint _84 = (_64 & 1u) ^ 1u;
        uint _85 = _82 ? 1u : _84;
        uint _86 = _82 ? 0u : _84;
        uint frontier_phi_3_1_ladder;
        bool frontier_phi_3_1_ladder_1;
        uint frontier_phi_3_1_ladder_2;
        uint frontier_phi_3_1_ladder_3;
        if ((_77 & 8u) == 0u)
        {
            frontier_phi_3_1_ladder = _85;
            frontier_phi_3_1_ladder_1 = false;
            frontier_phi_3_1_ladder_2 = 0u;
            frontier_phi_3_1_ladder_3 = _86;
        }
        else
        {
            uint _118 = _63 & 1u;
            frontier_phi_3_1_ladder = (_118 != 0u) ? 0u : _85;
            frontier_phi_3_1_ladder_1 = false;
            frontier_phi_3_1_ladder_2 = _118 ^ 1u;
            frontier_phi_3_1_ladder_3 = _86;
        }
        _100 = frontier_phi_3_1_ladder;
        _102 = frontier_phi_3_1_ladder_1;
        _104 = frontier_phi_3_1_ladder_2;
        _106 = frontier_phi_3_1_ladder_3;
    }
    else
    {
        _100 = 0u;
        _102 = (((_63 + _64) + asuint(_33_m0[54u]).w) & 1u) == 0u;
        _104 = 0u;
        _106 = 0u;
    }
    bool _107 = _100 != 0u;
    uint4 _108 = WaveActiveBallot(_107);
    uint _109 = countbits(_108.x & gl_SubgroupLtMask.x) + countbits(_108.y & gl_SubgroupLtMask.y) + countbits(_108.z & gl_SubgroupLtMask.z) + countbits(_108.w & gl_SubgroupLtMask.w);
    uint4 _110 = WaveActiveBallot(_107);
    uint _111 = countbits(_110.x) + countbits(_110.y) + countbits(_110.z) + countbits(_110.w);
    uint4 _112 = WaveActiveBallot(_102);
    uint _113 = countbits(_112.x & gl_SubgroupLtMask.x) + countbits(_112.y & gl_SubgroupLtMask.y) + countbits(_112.z & gl_SubgroupLtMask.z) + countbits(_112.w & gl_SubgroupLtMask.w);
    uint4 _114 = WaveActiveBallot(_102);
    uint _115 = countbits(_114.x) + countbits(_114.y) + countbits(_114.z) + countbits(_114.w);
    bool _116 = WaveActiveAnyTrue(_9[36u].Load(int3(uint2(_56, _58), 0u)).x != 0u);
    uint _120;
    uint _124;
    if (WaveIsFirstLane())
    {
        uint frontier_phi_6_5_ladder;
        uint frontier_phi_6_5_ladder_1;
        if (_116)
        {
            uint _121;
            InterlockedAdd(_23[0u], _111, _121);
            uint _125;
            InterlockedAdd(_25[0u], _115, _125);
            frontier_phi_6_5_ladder = _121;
            frontier_phi_6_5_ladder_1 = _125;
        }
        else
        {
            uint _122;
            InterlockedAdd(_17[0u], _111, _122);
            uint _126;
            InterlockedAdd(_21[0u], _115, _126);
            frontier_phi_6_5_ladder = _122;
            frontier_phi_6_5_ladder_1 = _126;
        }
        _120 = frontier_phi_6_5_ladder;
        _124 = frontier_phi_6_5_ladder_1;
    }
    else
    {
        _120 = _123;
        _124 = _123;
    }
    uint _127 = WaveReadLaneFirst(_120);
    uint _128 = WaveReadLaneFirst(_124);
    if (!(_100 == 0u))
    {
        uint _139 = _127 + _109;
        bool _141 = _106 != 0u;
        uint _160 = (((((_64 << 15u) & 536838144u) | (_63 & 32767u)) | (_104 << 29u)) | (uint(_141) << 30u)) | (uint((_104 != 0u) && _141) << 31u);
        if (_116)
        {
            _24[_139] = _160.xxxx;
        }
        else
        {
            _20[_139] = _160.xxxx;
        }
    }
    if (_102)
    {
        uint _161 = _128 + _113;
        uint _165 = ((_64 << 15u) & 536838144u) | (_63 & 32767u);
        if (_116)
        {
            _26[_161] = _165.xxxx;
        }
        else
        {
            _22[_161] = _165.xxxx;
        }
    }
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_WorkGroupID = stage_input.gl_WorkGroupID;
    gl_LocalInvocationID = stage_input.gl_LocalInvocationID;
    gl_SubgroupLtMask = (1u << (WaveGetLaneIndex() - uint4(0, 32, 64, 96))) - 1u;
    if (WaveGetLaneIndex() >= 32) gl_SubgroupLtMask.x = ~0u;
    if (WaveGetLaneIndex() >= 64) gl_SubgroupLtMask.y = ~0u;
    if (WaveGetLaneIndex() >= 96) gl_SubgroupLtMask.z = ~0u;
    if (WaveGetLaneIndex() < 32) gl_SubgroupLtMask.y = 0u;
    if (WaveGetLaneIndex() < 64) gl_SubgroupLtMask.z = 0u;
    if (WaveGetLaneIndex() < 96) gl_SubgroupLtMask.w = 0u;
    comp_main();
}
