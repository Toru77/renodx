cbuffer _24_26 : register(b0, space0)
{
    float4 _26_m0[1] : packoffset(c0);
};

Texture2D<float4> _8 : register(t0, space0);
RWTexture2D<float4> _11 : register(u0, space0);
RWTexture2D<uint> _15 : register(u1, space0);
RWTexture2D<float> _20[12] : register(u2, space0);
SamplerState _29 : register(s0, space0);

static uint3 gl_WorkGroupID;
static uint gl_LocalInvocationIndex;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint gl_LocalInvocationIndex : SV_GroupIndex;
};

groupshared uint _31;
groupshared float _35[256];

uint2 spvTextureSize(Texture2D<float4> Tex, uint Level, out uint Param)
{
    uint2 ret;
    Tex.GetDimensions(Level, ret.x, ret.y, Param);
    return ret;
}

uint2 spvImageSize(RWTexture2D<float4> Tex, out uint Param)
{
    uint2 ret;
    Tex.GetDimensions(ret.x, ret.y);
    Param = 0u;
    return ret;
}

void comp_main()
{
    uint _49_dummy_parameter;
    uint2 _49 = spvImageSize(_11, _49_dummy_parameter);
    uint4 _63 = asuint(_26_m0[0u]);
    uint _64 = _63.x;
    uint _66 = gl_LocalInvocationIndex >> 2u;
    uint _70 = gl_LocalInvocationIndex >> 1u;
    uint _73 = gl_LocalInvocationIndex >> 3u;
    uint _79 = ((_66 & 6u) | (gl_LocalInvocationIndex & 1u)) | (_73 & 8u);
    uint _83 = ((_70 & 3u) | (_73 & 4u)) | ((gl_LocalInvocationIndex >> 7u) << 3u);
    uint _86 = _79 << 1u;
    uint _87 = _83 << 1u;
    uint _88 = _86 | (gl_WorkGroupID.x << 6u);
    uint _89 = _87 + (gl_WorkGroupID.y << 6u);
    uint _93 = _79 | (gl_WorkGroupID.x << 5u);
    uint _94 = _83 + (gl_WorkGroupID.y << 5u);
    uint _97 = _79 >> 1u;
    uint _98 = _83 >> 1u;
    uint _99 = _97 | (gl_WorkGroupID.x << 4u);
    uint _100 = _98 + (gl_WorkGroupID.y << 4u);
    uint _102_dummy_parameter;
    uint2 _102 = spvTextureSize(_8, 0u, _102_dummy_parameter);
    float _107 = 1.0f / float(int(_102.x));
    float _109 = 1.0f / float(int(_102.y));
    float _114 = (_107 * float(_88)) + _107;
    float _115 = (_109 * float(_89)) + _109;
    float4 _121 = _8.GatherRed(_29, float2(_114, _115));
    float _122 = _121.x;
    float _123 = _121.y;
    float _124 = _121.z;
    float _125 = _121.w;
    uint _126 = _88 | 32u;
    float _130 = (_107 * float(_126)) + _107;
    float4 _132 = _8.GatherRed(_29, float2(_130, _115));
    float _133 = _132.x;
    float _134 = _132.y;
    float _135 = _132.z;
    float _136 = _132.w;
    uint _137 = _89 + 32u;
    float _140 = (_109 * float(_137)) + _109;
    float4 _142 = _8.GatherRed(_29, float2(_114, _140));
    float _143 = _142.x;
    float _144 = _142.y;
    float _145 = _142.z;
    float _146 = _142.w;
    float4 _148 = _8.GatherRed(_29, float2(_130, _140));
    float _149 = _148.x;
    float _150 = _148.y;
    float _151 = _148.z;
    float _152 = _148.w;
    uint _153 = _89 | 1u;
    _11[uint2(_88, _153)] = _122.xxxx;
    uint _156 = _88 | 1u;
    _11[uint2(_156, _153)] = _123.xxxx;
    _11[uint2(_156, _89)] = _124.xxxx;
    _11[uint2(_88, _89)] = _125.xxxx;
    _11[uint2(_126, _153)] = _133.xxxx;
    uint _169 = _88 | 33u;
    _11[uint2(_169, _153)] = _134.xxxx;
    _11[uint2(_169, _89)] = _135.xxxx;
    _11[uint2(_126, _89)] = _136.xxxx;
    uint _180 = _89 + 33u;
    _11[uint2(_88, _180)] = _143.xxxx;
    _11[uint2(_156, _180)] = _144.xxxx;
    _11[uint2(_156, _137)] = _145.xxxx;
    _11[uint2(_88, _137)] = _146.xxxx;
    _11[uint2(_126, _180)] = _149.xxxx;
    _11[uint2(_169, _180)] = _150.xxxx;
    _11[uint2(_169, _137)] = _151.xxxx;
    _11[uint2(_126, _137)] = _152.xxxx;
    float _208 = max(_122, max(_123, max(_124, _125)));
    float _211 = max(_133, max(_134, max(_135, _136)));
    float _214 = max(_143, max(_144, max(_145, _146)));
    float _217 = max(_149, max(_150, max(_151, _152)));
    _20[2u][uint2(_93, _94)] = _208.x;
    uint _223 = _93 | 16u;
    _20[2u][uint2(_223, _94)] = _211.x;
    uint _229 = _94 + 16u;
    _20[2u][uint2(_93, _229)] = _214.x;
    _20[2u][uint2(_223, _229)] = _217.x;
    if (!(_64 < 2u))
    {
        float _240 = QuadReadAcrossX(_208);
        float _241 = QuadReadAcrossY(_208);
        float _242 = QuadReadAcrossDiagonal(_208);
        float _245 = max(_208, max(_240, max(_241, _242)));
        float _246 = QuadReadAcrossX(_211);
        float _247 = QuadReadAcrossY(_211);
        float _248 = QuadReadAcrossDiagonal(_211);
        float _251 = max(_211, max(_246, max(_247, _248)));
        float _252 = QuadReadAcrossX(_214);
        float _253 = QuadReadAcrossY(_214);
        float _254 = QuadReadAcrossDiagonal(_214);
        float _257 = max(_214, max(_252, max(_253, _254)));
        float _258 = QuadReadAcrossX(_217);
        float _259 = QuadReadAcrossY(_217);
        float _260 = QuadReadAcrossDiagonal(_217);
        float _263 = max(_217, max(_258, max(_259, _260)));
        bool _265 = (gl_LocalInvocationIndex & 3u) == 0u;
        if (_265)
        {
            _20[3u][uint2(_99, _100)] = _245.x;
            uint _270 = _99 | 8u;
            _20[3u][uint2(_270, _100)] = _251.x;
            uint _275 = _100 + 8u;
            _20[3u][uint2(_99, _275)] = _257.x;
            _20[3u][uint2(_270, _275)] = _263.x;
            _35[_98 + (_97 * 16u)] = _245;
            uint _288 = _97 | 8u;
            _35[_98 + (_288 * 16u)] = _251;
            uint _292 = _98 + 8u;
            _35[_292 + (_97 * 16u)] = _257;
            _35[_292 + (_288 * 16u)] = _263;
        }
        if (!(_64 < 3u))
        {
            GroupMemoryBarrierWithGroupSync();
            uint _302 = _83 + (_79 * 16u);
            float _305 = QuadReadAcrossX(_35[_302]);
            float _306 = QuadReadAcrossY(_35[_302]);
            float _307 = QuadReadAcrossDiagonal(_35[_302]);
            float _310 = max(_35[_302], max(_305, max(_306, _307)));
            if (_265)
            {
                _20[4u][uint2(_97 | (gl_WorkGroupID.x << 3u), _98 + (gl_WorkGroupID.y << 3u))] = _310.x;
                _35[_83 + ((_79 + (_66 & 1u)) * 16u)] = _310;
            }
            if (!(_64 < 4u))
            {
                GroupMemoryBarrierWithGroupSync();
                bool _325 = gl_LocalInvocationIndex < 64u;
                if (_325)
                {
                    uint _330 = _87 + ((_86 | (_70 & 1u)) * 16u);
                    float _333 = QuadReadAcrossX(_35[_330]);
                    float _334 = QuadReadAcrossY(_35[_330]);
                    float _335 = QuadReadAcrossDiagonal(_35[_330]);
                    float _338 = max(_35[_330], max(_333, max(_334, _335)));
                    if (_265)
                    {
                        _20[5u][uint2(_97 + (gl_WorkGroupID.x << 2u), _98 + (gl_WorkGroupID.y << 2u))] = _338.x;
                        _35[_87 + ((_86 + _98) * 16u)] = _338;
                    }
                }
                if (!(_64 < 5u))
                {
                    GroupMemoryBarrierWithGroupSync();
                    bool _352 = gl_LocalInvocationIndex < 16u;
                    if (_352)
                    {
                        uint _357 = (_83 << 2u) + (((_79 << 2u) + _83) * 16u);
                        float _360 = QuadReadAcrossX(_35[_357]);
                        float _361 = QuadReadAcrossY(_35[_357]);
                        float _362 = QuadReadAcrossDiagonal(_35[_357]);
                        float _365 = max(_35[_357], max(_360, max(_361, _362)));
                        if (_265)
                        {
                            _20[6u][uint2(_97 + (gl_WorkGroupID.x << 1u), _98 + (gl_WorkGroupID.y << 1u))] = _365.x;
                            _35[0u + ((_97 + _83) * 16u)] = _365;
                        }
                    }
                    if (!(_64 < 6u))
                    {
                        GroupMemoryBarrierWithGroupSync();
                        bool _379 = gl_LocalInvocationIndex < 4u;
                        if (_379)
                        {
                            uint _381 = 0u + (gl_LocalInvocationIndex * 16u);
                            float _384 = QuadReadAcrossX(_35[_381]);
                            float _385 = QuadReadAcrossY(_35[_381]);
                            float _386 = QuadReadAcrossDiagonal(_35[_381]);
                            if (_265)
                            {
                                _20[7u][uint2(gl_WorkGroupID.x, gl_WorkGroupID.y)] = max(_35[_381], max(_384, max(_385, _386))).x;
                            }
                        }
                        if (!(_64 < 7u))
                        {
                            if (gl_LocalInvocationIndex == 0u)
                            {
                                uint _400;
                                InterlockedAdd(_15[uint2(0u, 0u)], 1u, _400);
                                _31 = _400;
                            }
                            GroupMemoryBarrierWithGroupSync();
                            if (_31 == ((((_49.x + 63u) >> 6u) * ((_49.y + 63u) >> 6u)) + 4294967295u))
                            {
                                _15[uint2(0u, 0u)] = uint4(0u, 0u, 0u, 0u).x;
                                uint _408 = _79 << 2u;
                                uint _409 = _83 << 2u;
                                uint _410 = _408 | 1u;
                                uint _411 = _409 | 1u;
                                float4 _414 = _20[7u][uint2(_410, _411)].xxxx;
                                float4 _417 = _20[7u][uint2(_410, _409)].xxxx;
                                float4 _420 = _20[7u][uint2(_408, _411)].xxxx;
                                float4 _423 = _20[7u][uint2(_408, _409)].xxxx;
                                float _428 = max(_423.x, max(_420.x, max(_417.x, _414.x)));
                                _20[8u][uint2(_86, _87)] = _428.x;
                                uint _433 = _408 | 2u;
                                uint _434 = _86 | 1u;
                                uint _435 = _408 | 3u;
                                float4 _438 = _20[7u][uint2(_435, _411)].xxxx;
                                float4 _441 = _20[7u][uint2(_435, _409)].xxxx;
                                float4 _444 = _20[7u][uint2(_433, _411)].xxxx;
                                float4 _447 = _20[7u][uint2(_433, _409)].xxxx;
                                float _452 = max(_447.x, max(_444.x, max(_441.x, _438.x)));
                                _20[8u][uint2(_434, _87)] = _452.x;
                                uint _457 = _409 | 2u;
                                uint _458 = _87 | 1u;
                                uint _459 = _409 | 3u;
                                float4 _462 = _20[7u][uint2(_410, _459)].xxxx;
                                float4 _465 = _20[7u][uint2(_410, _457)].xxxx;
                                float4 _468 = _20[7u][uint2(_408, _459)].xxxx;
                                float4 _471 = _20[7u][uint2(_408, _457)].xxxx;
                                float _476 = max(_471.x, max(_468.x, max(_465.x, _462.x)));
                                _20[8u][uint2(_86, _458)] = _476.x;
                                float4 _483 = _20[7u][uint2(_435, _459)].xxxx;
                                float4 _486 = _20[7u][uint2(_435, _457)].xxxx;
                                float4 _489 = _20[7u][uint2(_433, _459)].xxxx;
                                float4 _492 = _20[7u][uint2(_433, _457)].xxxx;
                                float _497 = max(_492.x, max(_489.x, max(_486.x, _483.x)));
                                _20[8u][uint2(_434, _458)] = _497.x;
                                if (!(_64 < 8u))
                                {
                                    float _505 = max(_428, max(_452, max(_476, _497)));
                                    _20[9u][uint2(_79, _83)] = _505.x;
                                    _35[_302] = _505;
                                    if (!(_64 < 9u))
                                    {
                                        GroupMemoryBarrierWithGroupSync();
                                        float _513 = QuadReadAcrossX(_35[_302]);
                                        float _514 = QuadReadAcrossY(_35[_302]);
                                        float _515 = QuadReadAcrossDiagonal(_35[_302]);
                                        float _518 = max(_35[_302], max(_513, max(_514, _515)));
                                        if (_265)
                                        {
                                            _20[10u][uint2(_97, _98)] = _518.x;
                                            _35[_83 + ((_79 + (_66 & 1u)) * 16u)] = _518;
                                        }
                                        if (!(_64 < 10u))
                                        {
                                            GroupMemoryBarrierWithGroupSync();
                                            if (_325)
                                            {
                                                uint _533 = _87 + ((_86 | (_70 & 1u)) * 16u);
                                                float _536 = QuadReadAcrossX(_35[_533]);
                                                float _537 = QuadReadAcrossY(_35[_533]);
                                                float _538 = QuadReadAcrossDiagonal(_35[_533]);
                                                float _541 = max(_35[_533], max(_536, max(_537, _538)));
                                                if (_265)
                                                {
                                                    _20[11u][uint2(_97, _98)] = _541.x;
                                                    _35[_87 + ((_86 + _98) * 16u)] = _541;
                                                }
                                            }
                                            if (!(_64 < 11u))
                                            {
                                                GroupMemoryBarrierWithGroupSync();
                                                if (_352)
                                                {
                                                    uint _554 = _409 + ((_408 + _83) * 16u);
                                                    float _557 = QuadReadAcrossX(_35[_554]);
                                                    float _558 = QuadReadAcrossY(_35[_554]);
                                                    float _559 = QuadReadAcrossDiagonal(_35[_554]);
                                                    float _562 = max(_35[_554], max(_557, max(_558, _559)));
                                                    if (_265)
                                                    {
                                                        _20[12u][uint2(_97, _98)] = _562.x;
                                                        _35[0u + ((_97 + _83) * 16u)] = _562;
                                                    }
                                                }
                                                if (!(_64 < 12u))
                                                {
                                                    GroupMemoryBarrierWithGroupSync();
                                                    if (_379)
                                                    {
                                                        uint _573 = 0u + (gl_LocalInvocationIndex * 16u);
                                                        float _576 = QuadReadAcrossX(_35[_573]);
                                                        float _577 = QuadReadAcrossY(_35[_573]);
                                                        float _578 = QuadReadAcrossDiagonal(_35[_573]);
                                                        if (_265)
                                                        {
                                                            _20[13u][uint2(0u, 0u)] = max(_35[_573], max(_576, max(_577, _578))).x;
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
