cbuffer _24_26 : register(b3, space0)
{
    float4 _26_m0[69] : packoffset(c0);
};

Texture2D<float4> _8 : register(t0, space0);
RWTexture2D<float4> _14[13] : register(u0, space0);
globallycoherent RWTexture2D<float> _17 : register(u14, space0);
globallycoherent RWBuffer<uint> _20 : register(u13, space0);

static uint3 gl_WorkGroupID;
static uint gl_LocalInvocationIndex;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint gl_LocalInvocationIndex : SV_GroupIndex;
};

groupshared uint _28;
groupshared float _32[256];

uint2 spvTextureSize(Texture2D<float4> Tex, uint Level, out uint Param)
{
    uint2 ret;
    Tex.GetDimensions(Level, ret.x, ret.y, Param);
    return ret;
}

void comp_main()
{
    uint _49_dummy_parameter;
    uint2 _49 = spvTextureSize(_8, 0u, _49_dummy_parameter);
    uint _59 = (uint(float(int(_49.x))) + 63u) >> 6u;
    uint _62 = gl_WorkGroupID.x >> 6u;
    uint _63 = gl_WorkGroupID.y >> 6u;
    uint _64 = _62 << 6u;
    uint _65 = _63 << 6u;
    uint _78 = (((_59 + 63u) >> 6u) * _63) + _62;
    uint4 _84 = asuint(_26_m0[55u]);
    uint _85 = _84.x;
    uint _86 = gl_LocalInvocationIndex >> 2u;
    uint _91 = gl_LocalInvocationIndex >> 1u;
    uint _94 = gl_LocalInvocationIndex >> 3u;
    uint _100 = ((_86 & 6u) | (gl_LocalInvocationIndex & 1u)) | (_94 & 8u);
    uint _104 = ((_91 & 3u) | (_94 & 4u)) | ((gl_LocalInvocationIndex >> 7u) << 3u);
    uint _106 = gl_WorkGroupID.y << 6u;
    uint _107 = _100 << 1u;
    uint _108 = _104 << 1u;
    uint _109 = _107 | (gl_WorkGroupID.x << 6u);
    uint _110 = _108 + _106;
    uint _114 = _100 | (gl_WorkGroupID.x << 5u);
    uint _115 = _104 + (gl_WorkGroupID.y << 5u);
    uint _120 = uint(float(int(_109)));
    uint _121 = uint(float(int(_110)));
    uint _126 = uint(float(int(_110 | 1u)));
    uint _131 = uint(float(int(_109 | 1u)));
    float _140 = max(max(_8.Load(int3(uint2(_120, _121), 0u)).x, _8.Load(int3(uint2(_120, _126), 0u)).x), max(_8.Load(int3(uint2(_131, _121), 0u)).x, _8.Load(int3(uint2(_131, _126), 0u)).x));
    _14[1u][uint2(_114, _115)] = _140.xxxx;
    uint _148 = _114 | 16u;
    uint _153 = uint(float(int(_109 | 32u)));
    uint _162 = uint(float(int(_109 | 33u)));
    float _171 = max(max(_8.Load(int3(uint2(_153, _121), 0u)).x, _8.Load(int3(uint2(_153, _126), 0u)).x), max(_8.Load(int3(uint2(_162, _121), 0u)).x, _8.Load(int3(uint2(_162, _126), 0u)).x));
    _14[1u][uint2(_148, _115)] = _171.xxxx;
    uint _177 = _108 + (_106 | 32u);
    uint _178 = _115 + 16u;
    uint _181 = uint(float(int(_177)));
    uint _187 = uint(float(int(_177 | 1u)));
    float _199 = max(max(_8.Load(int3(uint2(_120, _181), 0u)).x, _8.Load(int3(uint2(_120, _187), 0u)).x), max(_8.Load(int3(uint2(_131, _181), 0u)).x, _8.Load(int3(uint2(_131, _187), 0u)).x));
    _14[1u][uint2(_114, _178)] = _199.xxxx;
    float _219 = max(max(_8.Load(int3(uint2(_153, _181), 0u)).x, _8.Load(int3(uint2(_153, _187), 0u)).x), max(_8.Load(int3(uint2(_162, _181), 0u)).x, _8.Load(int3(uint2(_162, _187), 0u)).x));
    _14[1u][uint2(_148, _178)] = _219.xxxx;
    if (!(_85 < 2u))
    {
        uint _228 = WaveGetLaneIndex() & 4294967292u;
        float _231 = QuadReadLaneAt(_140, 1u);
        float _236 = QuadReadLaneAt(_140, 2u);
        float _241 = QuadReadLaneAt(_140, 3u);
        float _247 = max(max(_140, _231), max(_236, _241));
        uint _249 = WaveGetLaneIndex() & 4294967292u;
        float _251 = QuadReadLaneAt(_171, 1u);
        float _256 = QuadReadLaneAt(_171, 2u);
        float _261 = QuadReadLaneAt(_171, 3u);
        float _267 = max(max(_171, _251), max(_256, _261));
        uint _269 = WaveGetLaneIndex() & 4294967292u;
        float _271 = QuadReadLaneAt(_199, 1u);
        float _276 = QuadReadLaneAt(_199, 2u);
        float _281 = QuadReadLaneAt(_199, 3u);
        float _287 = max(max(_199, _271), max(_276, _281));
        uint _289 = WaveGetLaneIndex() & 4294967292u;
        float _291 = QuadReadLaneAt(_219, 1u);
        float _296 = QuadReadLaneAt(_219, 2u);
        float _301 = QuadReadLaneAt(_219, 3u);
        float _307 = max(max(_219, _291), max(_296, _301));
        bool _309 = (gl_LocalInvocationIndex & 3u) == 0u;
        if (_309)
        {
            uint _311 = gl_WorkGroupID.y << 4u;
            uint _312 = _100 >> 1u;
            uint _313 = _104 >> 1u;
            uint _314 = _312 | (gl_WorkGroupID.x << 4u);
            uint _315 = _313 + _311;
            _14[2u][uint2(_314, _315)] = _247.xxxx;
            _32[_313 + (_312 * 16u)] = _247;
            uint _324 = _314 | 8u;
            _14[2u][uint2(_324, _315)] = _267.xxxx;
            uint _329 = _312 | 8u;
            _32[_313 + (_329 * 16u)] = _267;
            uint _334 = _313 + (_311 | 8u);
            _14[2u][uint2(_314, _334)] = _287.xxxx;
            uint _339 = _313 + 8u;
            _32[_339 + (_312 * 16u)] = _287;
            _14[2u][uint2(_324, _334)] = _307.xxxx;
            _32[_339 + (_329 * 16u)] = _307;
        }
        if (!(_85 < 3u))
        {
            GroupMemoryBarrierWithGroupSync();
            uint _353 = _104 + (_100 * 16u);
            uint _357 = WaveGetLaneIndex() & 4294967292u;
            float _359 = QuadReadLaneAt(_32[_353], 1u);
            float _364 = QuadReadLaneAt(_32[_353], 2u);
            float _369 = QuadReadLaneAt(_32[_353], 3u);
            float _375 = max(max(_32[_353], _359), max(_364, _369));
            if (_309)
            {
                _14[3u][uint2((_100 >> 1u) | (gl_WorkGroupID.x << 3u), (_104 >> 1u) + (gl_WorkGroupID.y << 3u))] = _375.xxxx;
                _32[_104 + ((_100 + (_86 & 1u)) * 16u)] = _375;
            }
            if (!(_85 < 4u))
            {
                GroupMemoryBarrierWithGroupSync();
                bool _392 = gl_LocalInvocationIndex < 64u;
                if (_392)
                {
                    uint _396 = _108 + ((_107 | (_91 & 1u)) * 16u);
                    uint _400 = WaveGetLaneIndex() & 4294967292u;
                    float _402 = QuadReadLaneAt(_32[_396], 1u);
                    float _407 = QuadReadLaneAt(_32[_396], 2u);
                    float _412 = QuadReadLaneAt(_32[_396], 3u);
                    float _418 = max(max(_32[_396], _402), max(_407, _412));
                    if (_309)
                    {
                        uint _423 = _104 >> 1u;
                        _14[4u][uint2((_100 >> 1u) + (gl_WorkGroupID.x << 2u), _423 + (gl_WorkGroupID.y << 2u))] = _418.xxxx;
                        _32[_108 + ((_107 + _423) * 16u)] = _418;
                    }
                }
                if (!(_85 < 5u))
                {
                    GroupMemoryBarrierWithGroupSync();
                    bool _434 = gl_LocalInvocationIndex < 16u;
                    if (_434)
                    {
                        uint _439 = (_104 << 2u) + (((_100 << 2u) + _104) * 16u);
                        uint _443 = WaveGetLaneIndex() & 4294967292u;
                        float _445 = QuadReadLaneAt(_32[_439], 1u);
                        float _450 = QuadReadLaneAt(_32[_439], 2u);
                        float _455 = QuadReadLaneAt(_32[_439], 3u);
                        float _461 = max(max(_32[_439], _445), max(_450, _455));
                        if (_309)
                        {
                            uint _465 = _100 >> 1u;
                            _14[5u][uint2(_465 + (gl_WorkGroupID.x << 1u), (_104 >> 1u) + (gl_WorkGroupID.y << 1u))] = _461.xxxx;
                            _32[0u + ((_465 + _104) * 16u)] = _461;
                        }
                    }
                    if (!(_85 < 6u))
                    {
                        GroupMemoryBarrierWithGroupSync();
                        bool _477 = gl_LocalInvocationIndex < 4u;
                        if (_477)
                        {
                            uint _479 = 0u + (gl_LocalInvocationIndex * 16u);
                            uint _483 = WaveGetLaneIndex() & 4294967292u;
                            float _485 = QuadReadLaneAt(_32[_479], 1u);
                            float _490 = QuadReadLaneAt(_32[_479], 2u);
                            float _495 = QuadReadLaneAt(_32[_479], 3u);
                            if (_309)
                            {
                                _17[uint2(gl_WorkGroupID.x, gl_WorkGroupID.y)] = max(max(_32[_479], _485), max(_490, _495)).x;
                            }
                        }
                        if (!(_85 < 7u))
                        {
                            if (gl_LocalInvocationIndex == 0u)
                            {
                                uint _508;
                                InterlockedAdd(_20[_78], 1u, _508);
                                _28 = _508;
                            }
                            GroupMemoryBarrierWithGroupSync();
                            if (_28 == (((min((_65 + 64u), ((uint(float(int(_49.y))) + 63u) >> 6u)) - _65) * (min((_64 + 64u), _59) - _64)) + 4294967295u))
                            {
                                _20[_78] = uint4(0u, 0u, 0u, 0u).x;
                                uint _515 = _100 | (_62 << 4u);
                                uint _517 = _104 + (_63 << 4u);
                                uint _518 = _515 << 2u;
                                uint _519 = _517 << 2u;
                                uint _520 = _515 << 1u;
                                uint _521 = _517 << 1u;
                                uint _522 = _518 | 1u;
                                uint _523 = _519 | 1u;
                                float4 _524 = _17[uint2(_518, _519)].xxxx;
                                float4 _527 = _17[uint2(_518, _523)].xxxx;
                                float4 _530 = _17[uint2(_522, _519)].xxxx;
                                float4 _533 = _17[uint2(_522, _523)].xxxx;
                                float _538 = max(max(_524.x, _527.x), max(_530.x, _533.x));
                                _14[7u][uint2(_520, _521)] = _538.xxxx;
                                uint _543 = _518 | 2u;
                                uint _544 = _520 | 1u;
                                uint _545 = _518 | 3u;
                                float4 _546 = _17[uint2(_543, _519)].xxxx;
                                float4 _549 = _17[uint2(_543, _523)].xxxx;
                                float4 _552 = _17[uint2(_545, _519)].xxxx;
                                float4 _555 = _17[uint2(_545, _523)].xxxx;
                                float _560 = max(max(_546.x, _549.x), max(_552.x, _555.x));
                                _14[7u][uint2(_544, _521)] = _560.xxxx;
                                uint _565 = _519 | 2u;
                                uint _566 = _521 | 1u;
                                uint _567 = _519 | 3u;
                                float4 _568 = _17[uint2(_518, _565)].xxxx;
                                float4 _571 = _17[uint2(_518, _567)].xxxx;
                                float4 _574 = _17[uint2(_522, _565)].xxxx;
                                float4 _577 = _17[uint2(_522, _567)].xxxx;
                                float _582 = max(max(_568.x, _571.x), max(_574.x, _577.x));
                                _14[7u][uint2(_520, _566)] = _582.xxxx;
                                float4 _587 = _17[uint2(_543, _565)].xxxx;
                                float4 _590 = _17[uint2(_543, _567)].xxxx;
                                float4 _593 = _17[uint2(_545, _565)].xxxx;
                                float4 _596 = _17[uint2(_545, _567)].xxxx;
                                float _601 = max(max(_587.x, _590.x), max(_593.x, _596.x));
                                _14[7u][uint2(_544, _566)] = _601.xxxx;
                                if (!(_85 < 8u))
                                {
                                    float _609 = max(max(_538, _560), max(_582, _601));
                                    _14[8u][uint2(_515, _517)] = _609.xxxx;
                                    _32[_353] = _609;
                                    if (!(_85 < 9u))
                                    {
                                        GroupMemoryBarrierWithGroupSync();
                                        uint _618 = WaveGetLaneIndex() & 4294967292u;
                                        float _620 = QuadReadLaneAt(_32[_353], 1u);
                                        float _625 = QuadReadLaneAt(_32[_353], 2u);
                                        float _630 = QuadReadLaneAt(_32[_353], 3u);
                                        float _636 = max(max(_32[_353], _620), max(_625, _630));
                                        if (_309)
                                        {
                                            _14[9u][uint2((_100 >> 1u) | (_62 << 3u), (_104 >> 1u) + (_63 << 3u))] = _636.xxxx;
                                            _32[_104 + ((_100 + (_86 & 1u)) * 16u)] = _636;
                                        }
                                        if (!(_85 < 10u))
                                        {
                                            GroupMemoryBarrierWithGroupSync();
                                            if (_392)
                                            {
                                                uint _657 = _108 + ((_107 | (_91 & 1u)) * 16u);
                                                uint _661 = WaveGetLaneIndex() & 4294967292u;
                                                float _663 = QuadReadLaneAt(_32[_657], 1u);
                                                float _668 = QuadReadLaneAt(_32[_657], 2u);
                                                float _673 = QuadReadLaneAt(_32[_657], 3u);
                                                float _679 = max(max(_32[_657], _663), max(_668, _673));
                                                if (_309)
                                                {
                                                    uint _685 = _104 >> 1u;
                                                    _14[10u][uint2((_100 >> 1u) + (_62 << 2u), _685 + (_63 << 2u))] = _679.xxxx;
                                                    _32[_108 + ((_107 + _685) * 16u)] = _679;
                                                }
                                            }
                                            if (!(_85 < 11u))
                                            {
                                                GroupMemoryBarrierWithGroupSync();
                                                if (_434)
                                                {
                                                    uint _700 = (_104 << 2u) + (((_100 << 2u) + _104) * 16u);
                                                    uint _704 = WaveGetLaneIndex() & 4294967292u;
                                                    float _706 = QuadReadLaneAt(_32[_700], 1u);
                                                    float _711 = QuadReadLaneAt(_32[_700], 2u);
                                                    float _716 = QuadReadLaneAt(_32[_700], 3u);
                                                    float _722 = max(max(_32[_700], _706), max(_711, _716));
                                                    if (_309)
                                                    {
                                                        uint _727 = _100 >> 1u;
                                                        _14[11u][uint2(_727 + (_62 << 1u), (_104 >> 1u) + (_63 << 1u))] = _722.xxxx;
                                                        _32[0u + ((_727 + _104) * 16u)] = _722;
                                                    }
                                                }
                                                if (!(_85 < 12u))
                                                {
                                                    GroupMemoryBarrierWithGroupSync();
                                                    if (_477)
                                                    {
                                                        uint _740 = 0u + (gl_LocalInvocationIndex * 16u);
                                                        uint _744 = WaveGetLaneIndex() & 4294967292u;
                                                        float _746 = QuadReadLaneAt(_32[_740], 1u);
                                                        float _751 = QuadReadLaneAt(_32[_740], 2u);
                                                        float _756 = QuadReadLaneAt(_32[_740], 3u);
                                                        if (_309)
                                                        {
                                                            _14[12u][uint2(_62, _63)] = max(max(_32[_740], _746), max(_751, _756)).xxxx;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

[numthreads(256, 1, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_WorkGroupID = stage_input.gl_WorkGroupID;
    gl_LocalInvocationIndex = stage_input.gl_LocalInvocationIndex;
    comp_main();
}
