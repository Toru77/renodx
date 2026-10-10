static float _2219;
static uint _3238;
static float _3239;
static float _3240;
static float _3241;
static float _3242;
static float _3243;
static float _3244;
static float _3245;
static float _3246;
static float _3247;
static uint _3248;
static uint _3249;
static float _3250;
static float _3251;
static float _3252;
static float _3253;
static float _3254;
static float _3260;
static uint _3261;
static float _3262;
static uint _3264;
static uint _3265;
static float _3270;

cbuffer _43_45 : register(b2, space0)
{
    float4 _45_m0[700] : packoffset(c0);
};

cbuffer _48_50 : register(b3, space0)
{
    float4 _50_m0[69] : packoffset(c0);
};

cbuffer _53_55 : register(b4, space0)
{
    float4 _55_m0[1] : packoffset(c0);
};

Buffer<uint4> _8 : register(t0, space0);
Texture2D<float4> _12 : register(t1, space0);
Texture2D<float4> _13 : register(t2, space0);
Texture2D<float4> _14 : register(t3, space0);
Texture2D<float4> _15 : register(t4, space0);
Buffer<uint4> _16 : register(t96, space0);
Buffer<uint4> _17 : register(t98, space0);
Texture2D<float4> _20[] : register(t0, space37);
Texture2D<uint4> _24[] : register(t0, space38);
TextureCube<float4> _28[] : register(t0, space41);
uniform RaytracingAccelerationStructure _32 : register(t6, space0);
RWTexture2D<float4> _35 : register(u0, space0);
RWBuffer<uint> _38 : register(u1, space0);
RWTexture2D<float4> _39 : register(u2, space0);
SamplerState _58 : register(s0, space0);
SamplerState _59 : register(s5, space0);

static uint3 gl_WorkGroupID;
static uint gl_LocalInvocationIndex;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint gl_LocalInvocationIndex : SV_GroupIndex;
};

static RayQuery<RAY_FLAG_NONE> _2745;

uint2 spvTextureSize(Texture2D<float4> Tex, uint Level, out uint Param)
{
    uint2 ret;
    Tex.GetDimensions(Level, ret.x, ret.y, Param);
    return ret;
}

uint spvPackHalf2x16(float2 value)
{
    uint2 Packed = f32tof16(value);
    return Packed.x | (Packed.y << 16);
}

float2 spvUnpackHalf2x16(uint value)
{
    return f16tof32(uint2(value & 0xffff, value >> 16));
}

void comp_main()
{
    uint _77;
    float _81;
    float _82;
    float _83;
    float _87;
    float _88;
    float _89;
    float _93;
    float _94;
    float _95;
    float _99;
    float _100;
    float _101;
    float _102;
    float _106;
    float _107;
    float _108;
    float _109;
    float _113;
    float _114;
    float _115;
    float _116;
    float _120;
    float _121;
    float _122;
    float _123;
    float _127;
    float _128;
    float _129;
    float _130;
    float _134;
    float _135;
    float _136;
    float _137;
    float _141;
    float _142;
    float _143;
    float _144;
    float _148;
    float _149;
    float _150;
    float _151;
    float _155;
    float _156;
    float _157;
    float _161;
    float _162;
    float _163;
    float _167;
    float _168;
    float _169;
    float _173;
    float _174;
    float _175;
    float _176;
    float _180;
    float _181;
    float _182;
    float _183;
    float _187;
    float _188;
    float _189;
    float _190;
    float _194;
    float _195;
    float _196;
    float _197;
    float _202;
    float _204;
    uint _209;
    for (;;)
    {
        uint4 _76 = asuint(_45_m0[176u]);
        _77 = _76.z;
        _81 = _50_m0[14u].x;
        _82 = _50_m0[14u].y;
        _83 = _50_m0[14u].z;
        _87 = _50_m0[15u].x;
        _88 = _50_m0[15u].y;
        _89 = _50_m0[15u].z;
        _93 = _50_m0[16u].x;
        _94 = _50_m0[16u].y;
        _95 = _50_m0[16u].z;
        _99 = _50_m0[22u].x;
        _100 = _50_m0[22u].y;
        _101 = _50_m0[22u].z;
        _102 = _50_m0[22u].w;
        _106 = _50_m0[23u].x;
        _107 = _50_m0[23u].y;
        _108 = _50_m0[23u].z;
        _109 = _50_m0[23u].w;
        _113 = _50_m0[24u].x;
        _114 = _50_m0[24u].y;
        _115 = _50_m0[24u].z;
        _116 = _50_m0[24u].w;
        _120 = _50_m0[25u].x;
        _121 = _50_m0[25u].y;
        _122 = _50_m0[25u].z;
        _123 = _50_m0[25u].w;
        _127 = _50_m0[26u].x;
        _128 = _50_m0[26u].y;
        _129 = _50_m0[26u].z;
        _130 = _50_m0[26u].w;
        _134 = _50_m0[27u].x;
        _135 = _50_m0[27u].y;
        _136 = _50_m0[27u].z;
        _137 = _50_m0[27u].w;
        _141 = _50_m0[28u].x;
        _142 = _50_m0[28u].y;
        _143 = _50_m0[28u].z;
        _144 = _50_m0[28u].w;
        _148 = _50_m0[29u].x;
        _149 = _50_m0[29u].y;
        _150 = _50_m0[29u].z;
        _151 = _50_m0[29u].w;
        _155 = _50_m0[18u].x;
        _156 = _50_m0[18u].y;
        _157 = _50_m0[18u].z;
        _161 = _50_m0[19u].x;
        _162 = _50_m0[19u].y;
        _163 = _50_m0[19u].z;
        _167 = _50_m0[20u].x;
        _168 = _50_m0[20u].y;
        _169 = _50_m0[20u].z;
        _173 = _50_m0[30u].x;
        _174 = _50_m0[30u].y;
        _175 = _50_m0[30u].z;
        _176 = _50_m0[30u].w;
        _180 = _50_m0[31u].x;
        _181 = _50_m0[31u].y;
        _182 = _50_m0[31u].z;
        _183 = _50_m0[31u].w;
        _187 = _50_m0[32u].x;
        _188 = _50_m0[32u].y;
        _189 = _50_m0[32u].z;
        _190 = _50_m0[32u].w;
        _194 = _50_m0[33u].x;
        _195 = _50_m0[33u].y;
        _196 = _50_m0[33u].z;
        _197 = _50_m0[33u].w;
        uint4 _200 = asuint(_55_m0[0u]);
        _202 = float(_200.x);
        _204 = float(_200.y);
        _209 = (((gl_WorkGroupID.y << 6u) + gl_WorkGroupID.x) << 6u) + gl_LocalInvocationIndex;
        if (_209 < _38[1u].xxxx.x)
        {
            bool ladder_phi_8;
            uint _221;
            uint _224;
            bool _228;
            bool _231;
            bool _232;
            uint _233;
            uint _234;
            float _254;
            float _255;
            float _262;
            bool _263;
            uint _265;
            uint _266;
            for (;;)
            {
                uint4 _219 = _8.Load(_209);
                uint _220 = _219.x;
                _221 = _220 & 32767u;
                uint _223 = _220 >> 15u;
                _224 = _223 & 16383u;
                _228 = (_220 & 536870912u) != 0u;
                _231 = (_220 & 1073741824u) != 0u;
                _232 = int(_220) < int(0u);
                _233 = uint(_202);
                _234 = uint(_204);
                bool _237 = ((_223 + _220) & 1u) == 0u;
                float _247 = float(int(_221));
                float _248 = float(int(_224));
                _254 = ((_247 + 0.5f) + (_237 ? _45_m0[58u].x : _45_m0[58u].z)) * (1.0f / _202);
                _255 = ((_248 + 0.5f) + (_237 ? _45_m0[58u].y : _45_m0[58u].w)) * (1.0f / _204);
                _262 = _20[21u].Load(int3(uint2(_221, _224), 0u)).x;
                _263 = _262 > 0.0f;
                _265 = uint(_247);
                _266 = uint(_248);
                float _391;
                float _393;
                float _395;
                uint _397;
                float _399;
                float _401;
                float _403;
                uint _405;
                float _407;
                if (_263)
                {
                    uint4 _270 = _24[22u].Load(int3(uint2(_265, _266), 0u));
                    uint _272 = _270.x;
                    float _285 = (float((_272 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _287 = (float(_272 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _292 = (1.0f - abs(_285)) - abs(_287);
                    float _295 = clamp((-0.0f) - _292, 0.0f, 1.0f);
                    float _296 = (-0.0f) - _295;
                    float _301 = ((_285 >= 0.0f) ? _296 : _295) + _285;
                    float _302 = ((_287 >= 0.0f) ? _296 : _295) + _287;
                    float _307 = rsqrt(dot(float3(_301, _302, _292), float3(_301, _302, _292)));
                    float _312 = float(_272 & 255u) * 0.0039215688593685626983642578125f;
                    uint _319 = _24[23u].Load(int3(uint2(_265, _266), 0u)).y + 4294967295u;
                    uint _322 = uint(int(_319) >> int(31u)) & 3u;
                    uint _327 = _322 + 103u;
                    float _336 = clamp((_312 - _45_m0[_327].x) / (_45_m0[_327].y - _45_m0[_327].x), 0.0f, 1.0f);
                    _391 = _301 * _307;
                    _393 = _302 * _307;
                    _395 = _307 * _292;
                    _397 = _322 + 1u;
                    _399 = (_336 * _336) * (3.0f - (_336 * 2.0f));
                    _401 = _312;
                    _403 = _45_m0[_327].z;
                    _405 = (int(_319) < int(0u)) ? 0u : _319;
                    _407 = 0.0f;
                }
                else
                {
                    uint4 _347 = _24[2u].Load(int3(uint2(_265, _266), 0u));
                    uint _349 = _347.x;
                    uint _350 = _347.w;
                    uint4 _356 = _24[15u].Load(int3(uint2(_265, _266), 0u));
                    uint _358 = _356.y;
                    uint _367 = ((_358 & 64u) != 0u) ? uint((_358 & 4294967167u) != 66u) : 4294967295u;
                    uint _368 = _350 & 128u;
                    uint _371 = (_368 != 0u) ? 1u : ((_349 << 7u) | _350);
                    uint4 _375 = _16.Load(_371 * 4u);
                    uint _376 = _375.x;
                    uint4 _379 = _16.Load((_371 * 4u) + 1u);
                    uint _380 = _379.x;
                    uint4 _383 = _16.Load((_371 * 4u) + 3u);
                    uint _384 = _383.x;
                    uint _387 = ((_376 & 1u) != 0u) ? 0u : 18u;
                    uint _389 = uint(min(int(uint(max(int(_367), int(0u)))), int(1u)));
                    uint _427;
                    uint _428;
                    if (_368 == 0u)
                    {
                        _427 = (((_376 & 2097152u) != 0u) && (_367 == _389)) ? (_387 | 128u) : _387;
                        _428 = _376;
                    }
                    else
                    {
                        _427 = _350;
                        _428 = _376 | ((_349 << 20u) & 134217728u);
                    }
                    uint _437 = _380 & 512u;
                    float _793;
                    float _795;
                    float _797;
                    float _799;
                    float _801;
                    float _803;
                    float _805;
                    float _807;
                    float _809;
                    float _811;
                    float _813;
                    float _815;
                    if (_437 == 0u)
                    {
                        if (!((_428 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _579 = asfloat(_17.Load((_384 * 115u) + 33u).x);
                        uint4 _587 = _24[2u].Load(int3(uint2(_265, _266), 0u));
                        uint _589 = _587.y;
                        uint _590 = _427 & 128u;
                        uint _781;
                        uint _782;
                        uint _783;
                        uint _784;
                        if (_590 == 0u)
                        {
                            _781 = uint(((_428 & 817889384u) | (_380 & 576u)) != 0u) | (((_428 >> 19u) & 1u) ^ 1u);
                            _782 = uint(((_428 & 17825808u) | (_380 & 520u)) != 0u);
                            _783 = uint(((_428 & 46137344u) | (_380 & 2564u)) != 0u);
                            _784 = 0u;
                        }
                        else
                        {
                            _781 = 1u;
                            _782 = _427 & 1u;
                            _783 = 1u;
                            _784 = 1u;
                        }
                        precise float _788 = float(_589 & 127u) * 0.0078740157186985015869140625f;
                        bool _792 = (_428 & 4194304u) == 0u;
                        float _1003;
                        if (_792)
                        {
                            _1003 = _788;
                        }
                        else
                        {
                            _1003 = float(_589 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1267;
                        if ((_428 & 134217728u) == 0u)
                        {
                            uint frontier_phi_47_40_ladder;
                            if ((_590 != 0u) || ((_428 & 17825792u) == 1048576u))
                            {
                                frontier_phi_47_40_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_47_40_ladder = _782;
                            }
                            _1267 = frontier_phi_47_40_ladder;
                        }
                        else
                        {
                            _1267 = _782;
                        }
                        uint4 _1270 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _1272 = _1270.x;
                        float _1375;
                        float _1377;
                        float _1379;
                        if (_781 == 0u)
                        {
                            _1375 = 0.0f;
                            _1377 = 0.0f;
                            _1379 = 0.0f;
                        }
                        else
                        {
                            float4 _1384 = _20[8u].Load(int3(uint2(_265, _266), 0u));
                            _1375 = _1384.x;
                            _1377 = _1384.y;
                            _1379 = _1384.z;
                        }
                        uint _1481;
                        if (_1267 == 0u)
                        {
                            _1481 = 0u;
                        }
                        else
                        {
                            _1481 = _24[9u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        uint _1687;
                        if (_783 == 0u)
                        {
                            _1687 = 0u;
                        }
                        else
                        {
                            _1687 = _24[10u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        float _1697 = (float((_1272 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1698 = (float(_1272 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1702 = (1.0f - abs(_1697)) - abs(_1698);
                        float _1704 = clamp((-0.0f) - _1702, 0.0f, 1.0f);
                        float _1705 = (-0.0f) - _1704;
                        float _1710 = ((_1697 >= 0.0f) ? _1705 : _1704) + _1697;
                        float _1711 = ((_1698 >= 0.0f) ? _1705 : _1704) + _1698;
                        float _1715 = rsqrt(dot(float3(_1710, _1711, _1702), float3(_1710, _1711, _1702)));
                        float _1716 = _1710 * _1715;
                        float _1717 = _1711 * _1715;
                        float _1718 = _1715 * _1702;
                        float _794 = float(_1272 & 255u);
                        float _1722 = ((_428 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1972;
                        float _1973;
                        float _1974;
                        float _1975;
                        uint _1976;
                        if ((_380 & 64u) == 0u)
                        {
                            uint frontier_phi_104_89_ladder;
                            float frontier_phi_104_89_ladder_1;
                            float frontier_phi_104_89_ladder_2;
                            float frontier_phi_104_89_ladder_3;
                            float frontier_phi_104_89_ladder_4;
                            if ((_428 & 276824064u) == 0u)
                            {
                                frontier_phi_104_89_ladder = 0u;
                                frontier_phi_104_89_ladder_1 = 0.0f;
                                frontier_phi_104_89_ladder_2 = 0.0f;
                                frontier_phi_104_89_ladder_3 = 0.0f;
                                frontier_phi_104_89_ladder_4 = ((_428 & 8u) != 0u) ? _1377 : _1722;
                            }
                            else
                            {
                                frontier_phi_104_89_ladder = 0u;
                                frontier_phi_104_89_ladder_1 = 0.0f;
                                frontier_phi_104_89_ladder_2 = 0.0f;
                                frontier_phi_104_89_ladder_3 = 0.0f;
                                frontier_phi_104_89_ladder_4 = _1722;
                            }
                            _1972 = frontier_phi_104_89_ladder_4;
                            _1973 = frontier_phi_104_89_ladder_3;
                            _1974 = frontier_phi_104_89_ladder_2;
                            _1975 = frontier_phi_104_89_ladder_1;
                            _1976 = frontier_phi_104_89_ladder;
                        }
                        else
                        {
                            float _1804 = (_1377 * 2.0f) + (-1.0f);
                            float _1805 = (_1379 * 2.0f) + (-1.0f);
                            float _1809 = (1.0f - abs(_1804)) - abs(_1805);
                            float _1811 = clamp((-0.0f) - _1809, 0.0f, 1.0f);
                            float _1812 = (-0.0f) - _1811;
                            float _1817 = ((_1804 >= 0.0f) ? _1812 : _1811) + _1804;
                            float _1818 = ((_1805 >= 0.0f) ? _1812 : _1811) + _1805;
                            float _1822 = rsqrt(dot(float3(_1817, _1818, _1809), float3(_1817, _1818, _1809)));
                            _1972 = floor(round(_1375 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1973 = _1817 * _1822;
                            _1974 = _1818 * _1822;
                            _1975 = _1822 * _1809;
                            _1976 = 1u;
                        }
                        float _802;
                        if ((_428 & 32768u) == 0u)
                        {
                            _802 = _1972;
                        }
                        else
                        {
                            float frontier_phi_116_117_ladder;
                            if (_17.Load((_384 * 115u) + 36u).x == 0u)
                            {
                                float _2285 = clamp((_794 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _579;
                                frontier_phi_116_117_ladder = ((_428 & 131072u) != 0u) ? _2285 : ((((clamp((1.21000003814697265625f / (exp2((_1003 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_384 * 115u) + 32u).x)) + 1.0f) * _2285);
                            }
                            else
                            {
                                frontier_phi_116_117_ladder = _579;
                            }
                            _802 = frontier_phi_116_117_ladder;
                        }
                        uint _2085 = _427 & 1u;
                        float _2228;
                        float _2230;
                        float _2232;
                        uint _2234;
                        if (((_428 & 16u) == 0u) || (((_2085 | (_380 & 8u)) | (_428 & 16777216u)) != 0u))
                        {
                            _2228 = _1973;
                            _2230 = _1974;
                            _2232 = _1975;
                            _2234 = _1976;
                        }
                        else
                        {
                            float _2244 = (float(_1481 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2245 = (float(_1481 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2249 = (1.0f - abs(_2244)) - abs(_2245);
                            float _2251 = clamp((-0.0f) - _2249, 0.0f, 1.0f);
                            float _2252 = (-0.0f) - _2251;
                            float _2257 = ((_2244 >= 0.0f) ? _2252 : _2251) + _2244;
                            float _2258 = ((_2245 >= 0.0f) ? _2252 : _2251) + _2245;
                            float _2262 = rsqrt(dot(float3(_2257, _2258, _2249), float3(_2257, _2258, _2249)));
                            _2228 = _2257 * _2262;
                            _2230 = _2258 * _2262;
                            _2232 = _2262 * _2249;
                            _2234 = 1u;
                        }
                        float _796;
                        float _798;
                        float _800;
                        if (_2085 == 0u)
                        {
                            float frontier_phi_142_141_ladder;
                            float frontier_phi_142_141_ladder_1;
                            float frontier_phi_142_141_ladder_2;
                            if (((_427 & 64u) == 0u) && (_784 != 0u))
                            {
                                float2 _2456 = spvUnpackHalf2x16((_1687 >> 17u) & 32736u);
                                float _2457 = _2456.x;
                                float _2460 = (spvUnpackHalf2x16((_1687 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2461 = (spvUnpackHalf2x16((_1687 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2465 = (1.0f - abs(_2460)) - abs(_2461);
                                float _2467 = clamp((-0.0f) - _2465, 0.0f, 1.0f);
                                float _2468 = (-0.0f) - _2467;
                                float _2473 = ((_2460 >= 0.0f) ? _2468 : _2467) + _2460;
                                float _2474 = ((_2461 >= 0.0f) ? _2468 : _2467) + _2461;
                                float _2478 = rsqrt(dot(float3(_2473, _2474, _2465), float3(_2473, _2474, _2465)));
                                float _2488 = (((_2473 * _2478) - _1716) * _2457) + _1716;
                                float _2489 = (((_2474 * _2478) - _1717) * _2457) + _1717;
                                float _2490 = (((_2478 * _2465) - _1718) * _2457) + _1718;
                                float _2494 = rsqrt(dot(float3(_2488, _2489, _2490), float3(_2488, _2489, _2490)));
                                frontier_phi_142_141_ladder = _2490 * _2494;
                                frontier_phi_142_141_ladder_1 = _2489 * _2494;
                                frontier_phi_142_141_ladder_2 = _2488 * _2494;
                            }
                            else
                            {
                                frontier_phi_142_141_ladder = _1718;
                                frontier_phi_142_141_ladder_1 = _1717;
                                frontier_phi_142_141_ladder_2 = _1716;
                            }
                            _796 = frontier_phi_142_141_ladder_2;
                            _798 = frontier_phi_142_141_ladder_1;
                            _800 = frontier_phi_142_141_ladder;
                        }
                        else
                        {
                            _796 = _1716;
                            _798 = _1717;
                            _800 = _1718;
                        }
                        float _810;
                        float _812;
                        float _814;
                        float _816;
                        if (_792)
                        {
                            float frontier_phi_155_154_ladder;
                            float frontier_phi_155_154_ladder_1;
                            float frontier_phi_155_154_ladder_2;
                            float frontier_phi_155_154_ladder_3;
                            if (((_428 & 33554432u) == 0u) || (((_380 & 4u) != 0u) && ((_428 & 8388608u) == 0u)))
                            {
                                frontier_phi_155_154_ladder = 0.0f;
                                frontier_phi_155_154_ladder_1 = 0.0f;
                                frontier_phi_155_154_ladder_2 = 0.0f;
                                frontier_phi_155_154_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2618 = (spvUnpackHalf2x16((_1687 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2619 = (spvUnpackHalf2x16((_1687 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2623 = (1.0f - abs(_2618)) - abs(_2619);
                                float _2625 = clamp((-0.0f) - _2623, 0.0f, 1.0f);
                                float _2626 = (-0.0f) - _2625;
                                float _2631 = ((_2618 >= 0.0f) ? _2626 : _2625) + _2618;
                                float _2632 = ((_2619 >= 0.0f) ? _2626 : _2625) + _2619;
                                float _2636 = rsqrt(dot(float3(_2631, _2632, _2623), float3(_2631, _2632, _2623)));
                                float _2637 = _2631 * _2636;
                                float _2638 = _2632 * _2636;
                                float _2639 = _2636 * _2623;
                                float _2643 = rsqrt(dot(float3(_2637, _2638, _2639), float3(_2637, _2638, _2639)));
                                frontier_phi_155_154_ladder = _2643 * _2639;
                                frontier_phi_155_154_ladder_1 = _2643 * _2638;
                                frontier_phi_155_154_ladder_2 = _2643 * _2637;
                                frontier_phi_155_154_ladder_3 = spvUnpackHalf2x16((_1687 >> 17u) & 32736u).x;
                            }
                            _810 = frontier_phi_155_154_ladder_3;
                            _812 = frontier_phi_155_154_ladder_2;
                            _814 = frontier_phi_155_154_ladder_1;
                            _816 = frontier_phi_155_154_ladder;
                        }
                        else
                        {
                            _810 = 0.0f;
                            _812 = 0.0f;
                            _814 = 0.0f;
                            _816 = 0.0f;
                        }
                        bool _2508 = _2234 != 0u;
                        _793 = _794;
                        _795 = _796;
                        _797 = _798;
                        _799 = _800;
                        _801 = _802;
                        _803 = _2508 ? _2228 : _796;
                        _805 = _2508 ? _2230 : _798;
                        _807 = _2508 ? _2232 : _800;
                        _809 = _810;
                        _811 = _812;
                        _813 = _814;
                        _815 = _816;
                    }
                    else
                    {
                        uint4 _539 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _541 = _539.x;
                        uint4 _545 = _24[9u].Load(int3(uint2(_265, _266), 0u));
                        uint _547 = _545.x;
                        float _721;
                        float _722;
                        float _723;
                        if ((_428 & 33554432u) == 0u)
                        {
                            float _599 = (float((_541 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _600 = (float(_541 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _604 = (1.0f - abs(_599)) - abs(_600);
                            float _606 = clamp((-0.0f) - _604, 0.0f, 1.0f);
                            float _607 = (-0.0f) - _606;
                            float _612 = ((_599 >= 0.0f) ? _607 : _606) + _599;
                            float _613 = ((_600 >= 0.0f) ? _607 : _606) + _600;
                            float _617 = rsqrt(dot(float3(_612, _613, _604), float3(_612, _613, _604)));
                            _721 = _612 * _617;
                            _722 = _613 * _617;
                            _723 = _617 * _604;
                        }
                        else
                        {
                            float _628 = (float((_547 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _629 = (float(_547 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _633 = (1.0f - abs(_628)) - abs(_629);
                            float _635 = clamp((-0.0f) - _633, 0.0f, 1.0f);
                            float _636 = (-0.0f) - _635;
                            float _641 = ((_628 >= 0.0f) ? _636 : _635) + _628;
                            float _642 = ((_629 >= 0.0f) ? _636 : _635) + _629;
                            float _646 = rsqrt(dot(float3(_641, _642, _633), float3(_641, _642, _633)));
                            _721 = _641 * _646;
                            _722 = _642 * _646;
                            _723 = _646 * _633;
                        }
                        _793 = float(_541 & 255u);
                        _795 = _721;
                        _797 = _722;
                        _799 = _723;
                        _801 = 1.0f;
                        _803 = _721;
                        _805 = _722;
                        _807 = _723;
                        _809 = 0.0f;
                        _811 = 0.0f;
                        _813 = 0.0f;
                        _815 = 0.0f;
                    }
                    precise float _817 = _793 * 0.0039215688593685626983642578125f;
                    if ((_427 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1012 = ((_427 & 128u) | _437) != 0u;
                    uint _1173;
                    if (_1012)
                    {
                        _1173 = 1u;
                    }
                    else
                    {
                        _1173 = (((_428 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _392;
                    float _394;
                    float _396;
                    uint _398;
                    float _402;
                    if (((_428 & 33554432u) == 0u) || _1012)
                    {
                        bool _1274 = _77 != 0u;
                        uint _1281;
                        if ((_428 & 16u) == 0u)
                        {
                            _1281 = _1173;
                        }
                        else
                        {
                            _1281 = ((_428 & 268435456u) != 0u) ? _1173 : 2u;
                        }
                        _402 = _801 * _817;
                        _392 = _1274 ? _803 : _795;
                        _394 = _1274 ? _805 : _797;
                        _396 = _1274 ? _807 : _799;
                        _398 = _1281;
                    }
                    else
                    {
                        _402 = _809;
                        _392 = _811;
                        _394 = _813;
                        _396 = _815;
                        _398 = _1173;
                    }
                    uint _1282 = _398 + 102u;
                    float _1291 = clamp((_402 - _45_m0[_1282].x) / (_45_m0[_1282].y - _45_m0[_1282].x), 0.0f, 1.0f);
                    _391 = _392;
                    _393 = _394;
                    _395 = _396;
                    _397 = _398;
                    _399 = (_1291 * _1291) * (3.0f - (_1291 * 2.0f));
                    _401 = _402;
                    _403 = _45_m0[_1282].z;
                    _405 = (_398 == 1u) ? _389 : 0u;
                    _407 = asfloat(_17.Load((_384 * 115u) + 114u).x);
                }
                if (_399 == 0.0f)
                {
                    ladder_phi_8 = false;
                    break;
                }
                float _447;
                if (_263)
                {
                    _447 = _262;
                }
                else
                {
                    _447 = _12.Load(int3(uint2(uint(int(_254 * float(_233))), uint(int(_255 * float(_234)))), 0u)).x;
                }
                float _449 = 1.0f - _401;
                float _450 = _449 * _449;
                float _458 = _50_m0[50u].w + _50_m0[50u].y;
                uint _461 = _397 + 63u;
                float _470 = clamp(((_50_m0[50u].x / (_458 - (_50_m0[50u].y * _447))) - _50_m0[_461].y) / (_50_m0[_461].x - _50_m0[_461].y), 0.0f, 1.0f);
                float _475 = ((_470 * _470) * _399) * (3.0f - (_470 * 2.0f));
                float _486 = ((_254 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _487 = ((1.0f - (_50_m0[51u].w * _255)) * 2.0f) + (-1.0f);
                float _503 = mad(_144, _447, mad(_137, _487, _486 * _130)) + _151;
                float _504 = (mad(_141, _447, mad(_134, _487, _486 * _127)) + _148) / _503;
                float _505 = (mad(_142, _447, mad(_135, _487, _486 * _128)) + _149) / _503;
                float _506 = (mad(_143, _447, mad(_136, _487, _486 * _129)) + _150) / _503;
                float _510 = rsqrt(dot(float3(_504, _505, _506), float3(_504, _505, _506)));
                float _511 = _510 * _504;
                float _512 = _510 * _505;
                float _513 = _510 * _506;
                float _516 = mad(_93, _395, mad(_87, _393, _391 * _81));
                float _519 = mad(_94, _395, mad(_88, _393, _391 * _82));
                float _522 = mad(_95, _395, mad(_89, _393, _391 * _83));
                float _525 = _519 * _519;
                float _658;
                float _659;
                float _660;
                if (abs(_522) > 0.0f)
                {
                    float _560 = sqrt((_522 * _522) + _525);
                    _658 = 0.0f;
                    _659 = ((-0.0f) - _522) / _560;
                    _660 = _519 / _560;
                }
                else
                {
                    float _566 = sqrt(_525 + (_516 * _516));
                    _658 = _519 / _566;
                    _659 = ((-0.0f) - _516) / _566;
                    _660 = 0.0f;
                }
                float _663 = (_660 * _519) - (_659 * _522);
                float _666 = (_658 * _522) - (_660 * _516);
                float _669 = (_659 * _516) - (_658 * _519);
                float _670 = (-0.0f) - _511;
                float _671 = (-0.0f) - _512;
                float _672 = (-0.0f) - _513;
                float _681 = mad(_672, _522, mad(_671, _519, _516 * _670));
                float _682 = mad(_672, _660, mad(_671, _659, _658 * _670)) * _450;
                float _683 = mad(_672, _669, mad(_671, _666, _663 * _670)) * _450;
                float _687 = rsqrt(dot(float3(_682, _683, _681), float3(_682, _683, _681)));
                float _688 = _687 * _682;
                float _689 = _687 * _683;
                float _690 = _687 * _681;
                float _693 = (_688 * _688) + (_689 * _689);
                bool _694 = _693 > 0.0f;
                float _730;
                float _731;
                if (_694)
                {
                    float _726 = rsqrt(_693);
                    _730 = (-0.0f) - (_689 * _726);
                    _731 = _726 * _688;
                }
                else
                {
                    _730 = 1.0f;
                    _731 = 0.0f;
                }
                float _735 = _690 + 1.0f;
                float _737 = 1.0f - (_735 * 0.5f);
                float _738 = _737 * _690;
                float _745 = sqrt(max(0.0f, 1.0f - (_737 * _737)));
                float _752 = ((_745 * _688) - (_731 * _738)) * _450;
                float _753 = ((_745 * _689) + (_730 * _738)) * _450;
                float _754 = max(0.0f, (((_731 * _688) - (_730 * _689)) * _737) + (_745 * _690));
                float _758 = rsqrt(dot(float3(_752, _753, _754), float3(_752, _753, _754)));
                float _759 = _752 * _758;
                float _760 = _753 * _758;
                float _761 = _758 * _754;
                float _764 = mad(_761, _516, mad(_760, _663, _759 * _658));
                float _767 = mad(_761, _519, mad(_760, _666, _759 * _659));
                float _770 = mad(_761, _522, mad(_760, _669, _759 * _660));
                float _774 = dot(float3(_511, _512, _513), float3(_764, _767, _770)) * 2.0f;
                float _778 = _511 - (_774 * _764);
                float _779 = _512 - (_774 * _767);
                float _780 = _513 - (_774 * _770);
                float _825;
                float _826;
                if (_694)
                {
                    float _821 = rsqrt(_693);
                    _825 = (-0.0f) - (_689 * _821);
                    _826 = _821 * _688;
                }
                else
                {
                    _825 = 1.0f;
                    _826 = 0.0f;
                }
                float _832 = _737 + (_735 * 0.15811388194561004638671875f);
                float _837 = _832 * _690;
                float _846 = sqrt(max(0.0f, 1.0f - (_832 * _832)));
                float _853 = (((_825 * (-1.3822754496572997595649212598801e-08f)) - (_826 * _837)) + (_846 * _688)) * _450;
                float _854 = (((_825 * _837) - (_826 * 1.3822754496572997595649212598801e-08f)) + (_846 * _689)) * _450;
                float _855 = max(0.0f, (((_826 * _688) - (_825 * _689)) * _832) + (_846 * _690));
                float _859 = rsqrt(dot(float3(_853, _854, _855), float3(_853, _854, _855)));
                float _860 = _853 * _859;
                float _861 = _854 * _859;
                float _862 = _859 * _855;
                float _865 = mad(_862, _516, mad(_861, _663, _860 * _658));
                float _868 = mad(_862, _519, mad(_861, _666, _860 * _659));
                float _871 = mad(_862, _522, mad(_861, _669, _860 * _660));
                float _875 = dot(float3(_511, _512, _513), float3(_865, _868, _871)) * 2.0f;
                float _879 = _511 - (_875 * _865);
                float _880 = _512 - (_875 * _868);
                float _881 = _513 - (_875 * _871);
                float _882 = dot(float3(_879, _880, _881), float3(_778, _779, _780));
                float _891 = rsqrt(dot(float3(_764, _767, _770), float3(_764, _767, _770)));
                float _892 = _891 * _764;
                float _893 = _891 * _767;
                float _894 = _891 * _770;
                float _898 = dot(float3(_511, _512, _513), float3(_892, _893, _894)) * 2.0f;
                float _902 = _511 - (_898 * _892);
                float _903 = _512 - (_898 * _893);
                float _904 = _513 - (_898 * _894);
                bool _908 = dot(float3(_902, _903, _904), float3(_511, _512, _513)) < 0.0f;
                float _915 = sqrt(((_505 * _505) + (_504 * _504)) + (_506 * _506)) * 0.001000000047497451305389404296875f;
                float _926 = ((_915 * _516) + _504) + (_902 * _403);
                float _927 = ((_915 * _519) + _505) + (_903 * _403);
                float _928 = ((_915 * _522) + _506) + (_904 * _403);
                float _944 = mad(_116, _928, mad(_109, _927, _926 * _102)) + _123;
                float _947 = (mad(_115, _928, mad(_108, _927, _926 * _101)) + _122) / _944;
                float _950 = (((mad(_113, _928, mad(_106, _927, _926 * _99)) + _120) / _944) * 0.5f) + 0.5f;
                float _951 = 0.5f - (((mad(_114, _928, mad(_107, _927, _926 * _100)) + _121) / _944) * 0.5f);
                float _954 = _950 * _50_m0[51u].x;
                float _955 = _951 * _50_m0[51u].y;
                float _960 = _926 + (_902 * 0.100000001490116119384765625f);
                float _961 = _927 + (_903 * 0.100000001490116119384765625f);
                float _962 = _928 + (_904 * 0.100000001490116119384765625f);
                float _978 = mad(_116, _962, mad(_109, _961, _960 * _102)) + _123;
                float _987 = _50_m0[51u].x * (((((mad(_113, _962, mad(_106, _961, _960 * _99)) + _120) / _978) * 0.5f) + 0.5f) - _950);
                float _989 = _50_m0[51u].y * ((0.5f - (((mad(_114, _962, mad(_107, _961, _960 * _100)) + _121) / _978) * 0.5f)) - _951);
                float _990 = ((mad(_115, _962, mad(_108, _961, _960 * _101)) + _122) / _978) - _947;
                float _991 = _987 * 10.0f;
                float _993 = _989 * 10.0f;
                float _994 = _990 * 10.0f;
                bool _1001 = _397 == 1u;
                uint _1013;
                uint _1015;
                float _1017;
                float _1019;
                float _1021;
                float _1023;
                float _1025;
                float _1027;
                float _1029;
                float _1031;
                float _1033;
                uint _1035;
                uint _1037;
                float _1039;
                float _1041;
                float _1043;
                if (_1001 && (asuint(_50_m0[62u]).w != 0u))
                {
                    _1013 = 0u;
                    _1015 = 1u;
                    _1017 = 0.0f;
                    _1019 = 0.0f;
                    _1021 = 1.0f;
                    _1023 = 0.0f;
                    _1025 = 0.0f;
                    _1027 = 0.0f;
                    _1029 = 0.0f;
                    _1031 = 0.0f;
                    _1033 = 0.0f;
                    _1035 = 0u;
                    _1037 = 0u;
                    _1039 = 0.0f;
                    _1041 = 0.0f;
                    _1043 = 0.0f;
                }
                else
                {
                    float _1104 = float(_233);
                    float _1105 = float(_234);
                    float _1112 = (_991 != 0.0f) ? (0.100000001490116119384765625f / _987) : 3.4028234663852885981170418348452e+38f;
                    float _1114 = (_993 != 0.0f) ? (0.100000001490116119384765625f / _989) : 3.4028234663852885981170418348452e+38f;
                    float _1115 = (_994 != 0.0f) ? (0.100000001490116119384765625f / _990) : 3.4028234663852885981170418348452e+38f;
                    float _1116 = 1.0f / _1104;
                    float _1117 = 1.0f / _1105;
                    float _1118 = 0.004999999888241291046142578125f / _1104;
                    float _1120 = 0.004999999888241291046142578125f / _1105;
                    float _1129 = float(_991 >= 0.0f);
                    float _1130 = float(_993 >= 0.0f);
                    float _1139 = ((_991 < 0.0f) ? ((-0.0f) - _1118) : _1118) - _954;
                    float _1142 = ((_993 < 0.0f) ? ((-0.0f) - _1120) : _1120) - _955;
                    float _1145 = min((((floor(_954 * _1104) + _1129) * _1116) + _1139) * _1112, (((floor(_955 * _1105) + _1130) * _1117) + _1142) * _1114);
                    float _1149 = (_1145 * _991) + _954;
                    float _1150 = (_1145 * _993) + _955;
                    float _1151 = (_1145 * _994) + _947;
                    float _1154 = _50_m0[50u].x / (_458 - (_1151 * _50_m0[50u].y));
                    float _1165 = max(0.300000011920928955078125f, 10.0f / max(0.00999999977648258209228515625f, max(abs(_987 * 38400.0f), abs(_989 * 21600.0f))));
                    float _1040;
                    float _1042;
                    float _1044;
                    uint _1234;
                    uint _1249;
                    uint _1251;
                    uint _1016;
                    float _1018;
                    float _1020;
                    float _1022;
                    float _1024;
                    float _1026;
                    float _1028;
                    float _1030;
                    float _1032;
                    float _1034;
                    float _1239;
                    float _1241;
                    float _1243;
                    float _1245;
                    float _1247;
                    uint _1233 = 0u;
                    float _1235 = _1151;
                    float _1236 = _1150;
                    float _1237 = _1149;
                    float _1238 = _1145;
                    float _1240 = _1117;
                    float _1242 = _1116;
                    float _1244 = _1105;
                    float _1246 = _1104;
                    uint _1248 = 0u;
                    uint _1250 = 0u;
                    float _1252 = _947;
                    float _1253 = _955;
                    float _1254 = _954;
                    float _1255 = _1151;
                    float _1256 = _1150;
                    float _1257 = _1149;
                    float _1258 = 1.0f;
                    float _1259 = 0.0f;
                    float _1260 = 0.0f;
                    uint _1261 = 1u;
                    float _1262;
                    float _1263;
                    uint _1264;
                    uint _1265;
                    bool _1266;
                    for (;;)
                    {
                        _1262 = _1246 * _1237;
                        _1263 = _1244 * _1236;
                        _1264 = uint(int(_1262));
                        _1265 = uint(int(_1263));
                        _1266 = _1248 == 0u;
                        float _1437;
                        if (_1266)
                        {
                            _1437 = _12.Load(int3(uint2(_1264, _1265), 0u)).x;
                        }
                        else
                        {
                            _1437 = _15.Load(int3(uint2(_1264, _1265), _1248 + 4294967295u)).x;
                        }
                        float _1443 = ((_1262 >= floor(_1246)) || (_1263 >= floor(_1244))) ? 1.0f : _1437;
                        float _1457 = (_994 < 0.0f) ? ((_1443 - _947) * _1115) : 3.4028234663852885981170418348452e+38f;
                        float _1459 = min(min((((floor(_1262) + _1129) * _1242) + _1139) * _1112, (((floor(_1263) + _1130) * _1240) + _1142) * _1114), _1457);
                        bool _1460 = _1443 < _1235;
                        bool _1464 = _1460 && (asuint(_1459) != asuint(_1457));
                        float _1465 = _1460 ? _1459 : _1238;
                        float _1469 = (_1465 * _991) + _954;
                        float _1470 = (_1465 * _993) + _955;
                        float _1471 = (_1465 * _994) + _947;
                        uint _1473 = (_1464 ? 1u : 4294967295u) + _1248;
                        float _1474 = _1464 ? 0.5f : 2.0f;
                        float _1475 = _1474 * _1246;
                        float _1476 = _1474 * _1244;
                        float _1477 = _1464 ? 2.0f : 0.5f;
                        float _1478 = _1477 * _1242;
                        float _1479 = _1477 * _1240;
                        _1234 = _1233 + 1u;
                        uint _1783;
                        uint _1785;
                        if (int(_1473) < int(0u))
                        {
                            float _1671 = _50_m0[50u].w + _50_m0[50u].y;
                            float _1673 = _50_m0[50u].x / (_1671 - (_50_m0[50u].y * _1443));
                            float _1676 = _50_m0[50u].x / (_1671 - (_50_m0[50u].y * _1471));
                            float _1678 = abs(_1154 - _1676);
                            float _1681 = _1676 - _1673;
                            float frontier_phi_88_74_ladder;
                            float frontier_phi_88_74_ladder_1;
                            float frontier_phi_88_74_ladder_2;
                            float frontier_phi_88_74_ladder_3;
                            uint frontier_phi_88_74_ladder_4;
                            float frontier_phi_88_74_ladder_5;
                            float frontier_phi_88_74_ladder_6;
                            float frontier_phi_88_74_ladder_7;
                            float frontier_phi_88_74_ladder_8;
                            float frontier_phi_88_74_ladder_9;
                            float frontier_phi_88_74_ladder_10;
                            float frontier_phi_88_74_ladder_11;
                            float frontier_phi_88_74_ladder_12;
                            uint frontier_phi_88_74_ladder_13;
                            uint frontier_phi_88_74_ladder_14;
                            float frontier_phi_88_74_ladder_15;
                            float frontier_phi_88_74_ladder_16;
                            if (_1681 > max(0.00999999977648258209228515625f, _1678 * 0.00999999977648258209228515625f))
                            {
                                bool _1763 = _1250 != 0u;
                                frontier_phi_88_74_ladder = _1116;
                                frontier_phi_88_74_ladder_1 = _1465 + _1165;
                                frontier_phi_88_74_ladder_2 = _1117;
                                frontier_phi_88_74_ladder_3 = _1105;
                                frontier_phi_88_74_ladder_4 = _1261;
                                frontier_phi_88_74_ladder_5 = _1763 ? _1260 : _1469;
                                frontier_phi_88_74_ladder_6 = _1763 ? _1259 : _1470;
                                frontier_phi_88_74_ladder_7 = _1258;
                                frontier_phi_88_74_ladder_8 = _1257;
                                frontier_phi_88_74_ladder_9 = _1256;
                                frontier_phi_88_74_ladder_10 = _1255;
                                frontier_phi_88_74_ladder_11 = _1253;
                                frontier_phi_88_74_ladder_12 = _1252;
                                frontier_phi_88_74_ladder_13 = (_1001 || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1250;
                                frontier_phi_88_74_ladder_14 = 0u;
                                frontier_phi_88_74_ladder_15 = _1104;
                                frontier_phi_88_74_ladder_16 = _1254;
                            }
                            else
                            {
                                float _1776 = max(0.100000001490116119384765625f, _1678 * 0.100000001490116119384765625f) * 0.5f;
                                float _1779 = clamp((abs(_1681) - _1776) / _1776, 0.0f, 1.0f);
                                uint _1781 = uint(_1673 < _1154);
                                float frontier_phi_88_74_ladder_87_ladder;
                                float frontier_phi_88_74_ladder_87_ladder_1;
                                float frontier_phi_88_74_ladder_87_ladder_2;
                                float frontier_phi_88_74_ladder_87_ladder_3;
                                uint frontier_phi_88_74_ladder_87_ladder_4;
                                float frontier_phi_88_74_ladder_87_ladder_5;
                                float frontier_phi_88_74_ladder_87_ladder_6;
                                float frontier_phi_88_74_ladder_87_ladder_7;
                                float frontier_phi_88_74_ladder_87_ladder_8;
                                float frontier_phi_88_74_ladder_87_ladder_9;
                                float frontier_phi_88_74_ladder_87_ladder_10;
                                float frontier_phi_88_74_ladder_87_ladder_11;
                                float frontier_phi_88_74_ladder_87_ladder_12;
                                uint frontier_phi_88_74_ladder_87_ladder_13;
                                uint frontier_phi_88_74_ladder_87_ladder_14;
                                float frontier_phi_88_74_ladder_87_ladder_15;
                                float frontier_phi_88_74_ladder_87_ladder_16;
                                if (_1250 == 0u)
                                {
                                    frontier_phi_88_74_ladder_87_ladder = _1478;
                                    frontier_phi_88_74_ladder_87_ladder_1 = _1465;
                                    frontier_phi_88_74_ladder_87_ladder_2 = _1479;
                                    frontier_phi_88_74_ladder_87_ladder_3 = _1476;
                                    frontier_phi_88_74_ladder_87_ladder_4 = _1781;
                                    frontier_phi_88_74_ladder_87_ladder_5 = _1260;
                                    frontier_phi_88_74_ladder_87_ladder_6 = _1259;
                                    frontier_phi_88_74_ladder_87_ladder_7 = _1779;
                                    frontier_phi_88_74_ladder_87_ladder_8 = _1257;
                                    frontier_phi_88_74_ladder_87_ladder_9 = _1256;
                                    frontier_phi_88_74_ladder_87_ladder_10 = _1255;
                                    frontier_phi_88_74_ladder_87_ladder_11 = _1253;
                                    frontier_phi_88_74_ladder_87_ladder_12 = _1252;
                                    frontier_phi_88_74_ladder_87_ladder_13 = uint(_1779 > 0.0f);
                                    frontier_phi_88_74_ladder_87_ladder_14 = _1473;
                                    frontier_phi_88_74_ladder_87_ladder_15 = _1475;
                                    frontier_phi_88_74_ladder_87_ladder_16 = _1254;
                                }
                                else
                                {
                                    frontier_phi_88_74_ladder_87_ladder = _1478;
                                    frontier_phi_88_74_ladder_87_ladder_1 = _1465;
                                    frontier_phi_88_74_ladder_87_ladder_2 = _1479;
                                    frontier_phi_88_74_ladder_87_ladder_3 = _1476;
                                    frontier_phi_88_74_ladder_87_ladder_4 = _1781;
                                    frontier_phi_88_74_ladder_87_ladder_5 = _1260;
                                    frontier_phi_88_74_ladder_87_ladder_6 = _1259;
                                    frontier_phi_88_74_ladder_87_ladder_7 = _1779;
                                    frontier_phi_88_74_ladder_87_ladder_8 = _1257;
                                    frontier_phi_88_74_ladder_87_ladder_9 = _1256;
                                    frontier_phi_88_74_ladder_87_ladder_10 = _1255;
                                    frontier_phi_88_74_ladder_87_ladder_11 = _1253;
                                    frontier_phi_88_74_ladder_87_ladder_12 = _1252;
                                    frontier_phi_88_74_ladder_87_ladder_13 = _1250;
                                    frontier_phi_88_74_ladder_87_ladder_14 = _1473;
                                    frontier_phi_88_74_ladder_87_ladder_15 = _1475;
                                    frontier_phi_88_74_ladder_87_ladder_16 = _1254;
                                }
                                frontier_phi_88_74_ladder = frontier_phi_88_74_ladder_87_ladder;
                                frontier_phi_88_74_ladder_1 = frontier_phi_88_74_ladder_87_ladder_1;
                                frontier_phi_88_74_ladder_2 = frontier_phi_88_74_ladder_87_ladder_2;
                                frontier_phi_88_74_ladder_3 = frontier_phi_88_74_ladder_87_ladder_3;
                                frontier_phi_88_74_ladder_4 = frontier_phi_88_74_ladder_87_ladder_4;
                                frontier_phi_88_74_ladder_5 = frontier_phi_88_74_ladder_87_ladder_5;
                                frontier_phi_88_74_ladder_6 = frontier_phi_88_74_ladder_87_ladder_6;
                                frontier_phi_88_74_ladder_7 = frontier_phi_88_74_ladder_87_ladder_7;
                                frontier_phi_88_74_ladder_8 = frontier_phi_88_74_ladder_87_ladder_8;
                                frontier_phi_88_74_ladder_9 = frontier_phi_88_74_ladder_87_ladder_9;
                                frontier_phi_88_74_ladder_10 = frontier_phi_88_74_ladder_87_ladder_10;
                                frontier_phi_88_74_ladder_11 = frontier_phi_88_74_ladder_87_ladder_11;
                                frontier_phi_88_74_ladder_12 = frontier_phi_88_74_ladder_87_ladder_12;
                                frontier_phi_88_74_ladder_13 = frontier_phi_88_74_ladder_87_ladder_13;
                                frontier_phi_88_74_ladder_14 = frontier_phi_88_74_ladder_87_ladder_14;
                                frontier_phi_88_74_ladder_15 = frontier_phi_88_74_ladder_87_ladder_15;
                                frontier_phi_88_74_ladder_16 = frontier_phi_88_74_ladder_87_ladder_16;
                            }
                            _1016 = frontier_phi_88_74_ladder_4;
                            _1018 = frontier_phi_88_74_ladder_5;
                            _1020 = frontier_phi_88_74_ladder_6;
                            _1022 = frontier_phi_88_74_ladder_7;
                            _1024 = frontier_phi_88_74_ladder_8;
                            _1026 = frontier_phi_88_74_ladder_9;
                            _1028 = frontier_phi_88_74_ladder_10;
                            _1030 = frontier_phi_88_74_ladder_16;
                            _1032 = frontier_phi_88_74_ladder_11;
                            _1034 = frontier_phi_88_74_ladder_12;
                            _1783 = frontier_phi_88_74_ladder_13;
                            _1785 = frontier_phi_88_74_ladder_14;
                            _1247 = frontier_phi_88_74_ladder_15;
                            _1245 = frontier_phi_88_74_ladder_3;
                            _1243 = frontier_phi_88_74_ladder;
                            _1241 = frontier_phi_88_74_ladder_2;
                            _1239 = frontier_phi_88_74_ladder_1;
                        }
                        else
                        {
                            bool _1683 = _1250 != 0u;
                            _1016 = _1261;
                            _1018 = _1260;
                            _1020 = _1259;
                            _1022 = _1258;
                            _1024 = _1683 ? _1257 : _1469;
                            _1026 = _1683 ? _1256 : _1470;
                            _1028 = _1683 ? _1255 : _1471;
                            _1030 = _1469;
                            _1032 = _1470;
                            _1034 = _1471;
                            _1783 = _1250;
                            _1785 = _1473;
                            _1247 = _1475;
                            _1245 = _1476;
                            _1243 = _1478;
                            _1241 = _1479;
                            _1239 = _1465;
                        }
                        uint frontier_phi_115_pred;
                        uint frontier_phi_115_pred_1;
                        float frontier_phi_115_pred_2;
                        float frontier_phi_115_pred_3;
                        float frontier_phi_115_pred_4;
                        bool _1788;
                        bool _1790;
                        for (;;)
                        {
                            _1788 = _1471 < 0.0f;
                            _1790 = _1788 || ((_1469 < 0.0f) || (_1470 < 0.0f));
                            if (!_1790)
                            {
                                if (!((_1471 > 1.0f) || ((_1469 > _50_m0[51u].x) || (_1470 > _50_m0[51u].y))))
                                {
                                    frontier_phi_115_pred = _1785;
                                    frontier_phi_115_pred_1 = _1783;
                                    frontier_phi_115_pred_2 = _1469;
                                    frontier_phi_115_pred_3 = _1470;
                                    frontier_phi_115_pred_4 = _1471;
                                    break;
                                }
                            }
                            if (!_1788)
                            {
                                frontier_phi_115_pred = 4294967295u;
                                frontier_phi_115_pred_1 = 1u;
                                frontier_phi_115_pred_2 = _1469;
                                frontier_phi_115_pred_3 = _1470;
                                frontier_phi_115_pred_4 = _1471;
                                break;
                            }
                            float _2074 = (-0.0f) - _1471;
                            float _2075 = _2074 / _994;
                            frontier_phi_115_pred = 4294967295u;
                            frontier_phi_115_pred_1 = 1u;
                            frontier_phi_115_pred_2 = (_2075 * _991) + _1469;
                            frontier_phi_115_pred_3 = (_2075 * _993) + _1470;
                            frontier_phi_115_pred_4 = _2074 + _1471;
                            break;
                        }
                        _1249 = frontier_phi_115_pred;
                        _1251 = frontier_phi_115_pred_1;
                        _1040 = frontier_phi_115_pred_2;
                        _1042 = frontier_phi_115_pred_3;
                        _1044 = frontier_phi_115_pred_4;
                        if ((_1234 < 128u) && (int(_1249) > int(4294967295u)))
                        {
                            _1233 = _1234;
                            _1235 = _1044;
                            _1236 = _1042;
                            _1237 = _1040;
                            _1238 = _1239;
                            _1240 = _1241;
                            _1242 = _1243;
                            _1244 = _1245;
                            _1246 = _1247;
                            _1248 = _1249;
                            _1250 = _1251;
                            _1252 = _1034;
                            _1253 = _1032;
                            _1254 = _1030;
                            _1255 = _1028;
                            _1256 = _1026;
                            _1257 = _1024;
                            _1258 = _1022;
                            _1259 = _1020;
                            _1260 = _1018;
                            _1261 = _1016;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    bool _2225 = _1234 > 127u;
                    _1013 = uint(_2225);
                    _1015 = _1016;
                    _1017 = _1018;
                    _1019 = _1020;
                    _1021 = _1022;
                    _1023 = _1024;
                    _1025 = _1026;
                    _1027 = _1028;
                    _1029 = _1030;
                    _1031 = _1032;
                    _1033 = _1034;
                    _1035 = _2225 ? 1u : _1251;
                    _1037 = uint(_1234 < 129u);
                    _1039 = _1040;
                    _1041 = _1042;
                    _1043 = _1044;
                }
                float _1051 = _50_m0[51u].z * 2.0f;
                float _1054 = (_1051 * _954) + (-1.0f);
                float _1055 = ((1.0f - (_50_m0[51u].w * _955)) * 2.0f) + (-1.0f);
                float _1071 = mad(_190, _947, mad(_183, _1055, _1054 * _176)) + _197;
                float _1072 = (mad(_187, _947, mad(_180, _1055, _1054 * _173)) + _194) / _1071;
                float _1073 = (mad(_188, _947, mad(_181, _1055, _1054 * _174)) + _195) / _1071;
                float _1074 = (mad(_189, _947, mad(_182, _1055, _1054 * _175)) + _196) / _1071;
                float _1079 = (_1051 * _1039) + (-1.0f);
                float _1080 = ((1.0f - (_50_m0[51u].w * _1041)) * 2.0f) + (-1.0f);
                float _1096 = mad(_190, _1043, mad(_183, _1080, _1079 * _176)) + _197;
                float _1100 = ((mad(_187, _1043, mad(_180, _1080, _1079 * _173)) + _194) / _1096) - _1072;
                float _1101 = ((mad(_188, _1043, mad(_181, _1080, _1079 * _174)) + _195) / _1096) - _1073;
                float _1102 = ((mad(_189, _1043, mad(_182, _1080, _1079 * _175)) + _196) / _1096) - _1074;
                float _1181;
                uint _1183;
                float _1185;
                if (_1037 == 0u)
                {
                    _1181 = 0.0f;
                    _1183 = _1035;
                    _1185 = 0.0f;
                }
                else
                {
                    float _1228 = float(_233);
                    float _1229 = float(_234);
                    float frontier_phi_44_45_ladder;
                    uint frontier_phi_44_45_ladder_1;
                    float frontier_phi_44_45_ladder_2;
                    if ((_1039 < 0.0f) || (_1041 < 0.0f))
                    {
                        frontier_phi_44_45_ladder = 0.0f;
                        frontier_phi_44_45_ladder_1 = _1035;
                        frontier_phi_44_45_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_44_45_ladder_52_ladder;
                        uint frontier_phi_44_45_ladder_52_ladder_1;
                        float frontier_phi_44_45_ladder_52_ladder_2;
                        if ((_1043 >= 1.0f) || ((_1039 > _50_m0[51u].x) || (_1041 > _50_m0[51u].y)))
                        {
                            frontier_phi_44_45_ladder_52_ladder = 0.0f;
                            frontier_phi_44_45_ladder_52_ladder_1 = _1035;
                            frontier_phi_44_45_ladder_52_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_44_45_ladder_52_ladder_63_ladder;
                            uint frontier_phi_44_45_ladder_52_ladder_63_ladder_1;
                            float frontier_phi_44_45_ladder_52_ladder_63_ladder_2;
                            for (;;)
                            {
                                if ((abs(_1039 - _254) < (2.0f / _1228)) && (abs(_1041 - _255) < (2.0f / _1229)))
                                {
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder = 0.0f;
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder_1 = _1035;
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_99;
                                    float frontier_phi_99_pred;
                                    uint frontier_phi_99_pred_1;
                                    float frontier_phi_99_pred_2;
                                    uint _1662;
                                    uint _1663;
                                    bool _1664;
                                    for (;;)
                                    {
                                        _1662 = uint(int(_1039 * _1228));
                                        _1663 = uint(int(_1041 * _1229));
                                        _1664 = _1001 && _908;
                                        if (!_1664)
                                        {
                                            if (!(dot(float3(_1100, _1101, _1102), float3(_1100, _1101, _1102)) < 0.000899999984540045261383056640625f))
                                            {
                                                ladder_phi_99 = false;
                                                frontier_phi_99_pred = 0.0f;
                                                frontier_phi_99_pred_1 = _1035;
                                                frontier_phi_99_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1753 = _24[22u].Load(int3(uint2(_1662, _1663), 0u));
                                        uint _1755 = _1753.x;
                                        float _2060;
                                        float _2061;
                                        float _2062;
                                        if (_1755 == 0u)
                                        {
                                            uint4 _1864 = _24[1u].Load(int3(uint2(_1662, _1663), 0u));
                                            uint _1866 = _1864.x;
                                            float _1874 = (float((_1866 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1875 = (float(_1866 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1879 = (1.0f - abs(_1874)) - abs(_1875);
                                            float _1881 = clamp((-0.0f) - _1879, 0.0f, 1.0f);
                                            float _1882 = (-0.0f) - _1881;
                                            _2060 = ((_1874 >= 0.0f) ? _1882 : _1881) + _1874;
                                            _2061 = ((_1875 >= 0.0f) ? _1882 : _1881) + _1875;
                                            _2062 = _1879;
                                        }
                                        else
                                        {
                                            float _1896 = (float((_1755 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1897 = (float(_1755 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1901 = (1.0f - abs(_1896)) - abs(_1897);
                                            float _1903 = clamp((-0.0f) - _1901, 0.0f, 1.0f);
                                            float _1904 = (-0.0f) - _1903;
                                            _2060 = ((_1896 >= 0.0f) ? _1904 : _1903) + _1896;
                                            _2061 = ((_1897 >= 0.0f) ? _1904 : _1903) + _1897;
                                            _2062 = _1901;
                                        }
                                        float _2066 = rsqrt(dot(float3(_2060, _2061, _2062), float3(_2060, _2061, _2062)));
                                        if (dot(float3(_2066 * _2060, _2066 * _2061, _2066 * _2062), float3(_1100, _1101, _1102)) > 0.0f)
                                        {
                                            ladder_phi_99 = true;
                                            frontier_phi_99_pred = 0.0f;
                                            frontier_phi_99_pred_1 = _1035;
                                            frontier_phi_99_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_99 = false;
                                            frontier_phi_99_pred = 0.0f;
                                            frontier_phi_99_pred_1 = _1035;
                                            frontier_phi_99_pred_2 = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_99)
                                    {
                                        frontier_phi_44_45_ladder_52_ladder_63_ladder = frontier_phi_99_pred;
                                        frontier_phi_44_45_ladder_52_ladder_63_ladder_1 = frontier_phi_99_pred_1;
                                        frontier_phi_44_45_ladder_52_ladder_63_ladder_2 = frontier_phi_99_pred_2;
                                        break;
                                    }
                                    float _1915 = _50_m0[51u].z * _1039;
                                    float _1916 = _50_m0[51u].w * _1041;
                                    float _1918 = (_1229 / _1228) * 0.0500000007450580596923828125f;
                                    float _1923 = clamp(_1915 / _1918, 0.0f, 1.0f);
                                    float _1924 = clamp(_1916 * 20.0f, 0.0f, 1.0f);
                                    float _1936 = clamp(((_1915 + (-1.0f)) + _1918) / _1918, 0.0f, 1.0f);
                                    float _1937 = clamp((_1916 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1948 = _1923 * _1924;
                                    precise float _1949 = _1948 * _1948;
                                    float _1953 = ((((3.0f - (_1924 * 2.0f)) * (3.0f - (_1923 * 2.0f))) * _1949) * (1.0f - ((_1936 * _1936) * (3.0f - (_1936 * 2.0f))))) * (1.0f - ((_1937 * _1937) * (3.0f - (_1937 * 2.0f))));
                                    bool _1956 = (_1035 != 0u) || (_1953 >= 1.0f);
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder = _1953 * float(_475 > 0.0f);
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder_1 = _1956 ? _1035 : 1u;
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder_2 = _1956 ? 0.0f : _1953;
                                    break;
                                }
                            }
                            frontier_phi_44_45_ladder_52_ladder = frontier_phi_44_45_ladder_52_ladder_63_ladder;
                            frontier_phi_44_45_ladder_52_ladder_1 = frontier_phi_44_45_ladder_52_ladder_63_ladder_1;
                            frontier_phi_44_45_ladder_52_ladder_2 = frontier_phi_44_45_ladder_52_ladder_63_ladder_2;
                        }
                        frontier_phi_44_45_ladder = frontier_phi_44_45_ladder_52_ladder;
                        frontier_phi_44_45_ladder_1 = frontier_phi_44_45_ladder_52_ladder_1;
                        frontier_phi_44_45_ladder_2 = frontier_phi_44_45_ladder_52_ladder_2;
                    }
                    _1181 = frontier_phi_44_45_ladder_2;
                    _1183 = frontier_phi_44_45_ladder_1;
                    _1185 = frontier_phi_44_45_ladder;
                }
                uint _1341;
                uint _1343;
                float _1226;
                for (;;)
                {
                    _1226 = ((((exp2(log2(clamp((sqrt(((_1073 * _1073) + (_1072 * _1072)) + (_1074 * _1074)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _407) * exp2(log2(clamp((_1073 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_393, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    if (_1185 > 0.0f)
                    {
                        uint _1303 = uint((_202 * 0.999989986419677734375f) * clamp(_1039, 0.0f, 1.0f));
                        uint _1304 = uint((_204 * 0.999989986419677734375f) * clamp(_1041, 0.0f, 1.0f));
                        uint4 _1307 = _24[2u].Load(int3(uint2(_1303, _1304), 0u));
                        uint _1310 = _1307.w;
                        uint4 _1315 = _24[15u].Load(int3(uint2(_1303, _1304), 0u));
                        uint _1317 = _1315.y;
                        uint _1323 = ((_1317 & 64u) != 0u) ? uint((_1317 & 4294967167u) != 66u) : 4294967295u;
                        uint _1324 = _1310 & 128u;
                        uint _1326 = (_1324 != 0u) ? 1u : ((_1307.x << 7u) | _1310);
                        uint4 _1329 = _16.Load(_1326 * 4u);
                        uint _1330 = _1329.x;
                        uint _1337 = ((_1330 & 1u) != 0u) ? 0u : 18u;
                        uint _1339 = uint(min(int(uint(max(int(_1323), int(0u)))), int(1u)));
                        uint _1396;
                        if (_1324 == 0u)
                        {
                            _1396 = (((_1330 & 2097152u) != 0u) && (_1323 == _1339)) ? (_1337 | 128u) : _1337;
                        }
                        else
                        {
                            _1396 = _1310;
                        }
                        uint _1397 = _16.Load((_1326 * 4u) + 1u).x & 512u;
                        bool _1400 = (_1396 & 144u) == 0u;
                        if (_1397 == 0u)
                        {
                            if (_1400 || ((_1330 & 1u) != 0u))
                            {
                                _1341 = 0u;
                                _1343 = 0u;
                                break;
                            }
                        }
                        else
                        {
                            if (_1400)
                            {
                                _1341 = 0u;
                                _1343 = 0u;
                                break;
                            }
                        }
                        bool _1732 = ((_1396 & 128u) | _1397) != 0u;
                        uint _1342;
                        if (_1732)
                        {
                            _1342 = 1u;
                        }
                        else
                        {
                            _1342 = (((_1330 >> 14u) & 2u) ^ 2u) + 3u;
                        }
                        if (((_1330 & 268435472u) == 16u) && (((_1330 & 33554432u) == 0u) || _1732))
                        {
                            _1341 = 2u;
                            _1343 = 0u;
                            break;
                        }
                        _1341 = _1342;
                        _1343 = (_1342 == 1u) ? _1339 : 0u;
                        break;
                    }
                    else
                    {
                        _1341 = 0u;
                        _1343 = 0u;
                        break;
                    }
                }
                bool _1405;
                float _1408;
                float _1410;
                float _1412;
                float _1414;
                float _1415;
                float _1416;
                float _1418;
                float _1347;
                float _1350;
                float _1353;
                float _1357;
                float _1358;
                for (;;)
                {
                    _1347 = mad(_167, _904, mad(_161, _903, _902 * _155));
                    _1350 = mad(_168, _904, mad(_162, _903, _902 * _156));
                    _1353 = mad(_169, _904, mad(_163, _903, _902 * _157));
                    bool _1356 = (_1183 != 0u) || (_1185 < 1.0f);
                    _1357 = _1356 ? 0.0f : 1.0f;
                    _1358 = _1356 ? 0.0f : 0.5f;
                    if (_1356)
                    {
                        uint4 _1404 = asuint(_50_m0[60u]);
                        if (_1001)
                        {
                            if (int(_405) < int(1u))
                            {
                                if (_1404.x == 0u)
                                {
                                    _1405 = false;
                                    _1408 = _1029;
                                    _1410 = _1031;
                                    _1412 = _1033;
                                    _1414 = 0.0f;
                                    _1415 = 0.0f;
                                    _1416 = 0.0f;
                                    _1418 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1404.y == 0u)
                                {
                                    _1405 = false;
                                    _1408 = _1029;
                                    _1410 = _1031;
                                    _1412 = _1033;
                                    _1414 = 0.0f;
                                    _1415 = 0.0f;
                                    _1416 = 0.0f;
                                    _1418 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1404.z == 0u)
                            {
                                _1405 = false;
                                _1408 = _1029;
                                _1410 = _1031;
                                _1412 = _1033;
                                _1414 = 0.0f;
                                _1415 = 0.0f;
                                _1416 = 0.0f;
                                _1418 = 0.0f;
                                break;
                            }
                        }
                        if (_1181 > 0.0f)
                        {
                            _1405 = false;
                            _1408 = _1029;
                            _1410 = _1031;
                            _1412 = _1033;
                            _1414 = 0.0f;
                            _1415 = 1.0f;
                            _1416 = 1000.0f;
                            _1418 = 0.5f;
                            break;
                        }
                        if (((_1023 <= 0.0f) || (_1025 <= 0.0f)) || (_1027 <= 0.0f))
                        {
                            _1405 = false;
                            _1408 = _1029;
                            _1410 = _1031;
                            _1412 = _1033;
                            _1414 = 0.0f;
                            _1415 = 1.0f;
                            _1416 = 1000.0f;
                            _1418 = 0.5f;
                            break;
                        }
                        if ((_1027 >= 1.0f) || ((_1023 >= _50_m0[51u].x) || (_1025 >= _50_m0[51u].y)))
                        {
                            _1405 = false;
                            _1408 = _1029;
                            _1410 = _1031;
                            _1412 = _1033;
                            _1414 = 0.0f;
                            _1415 = 1.0f;
                            _1416 = 1000.0f;
                            _1418 = 0.5f;
                            break;
                        }
                        uint _2374;
                        uint _2376;
                        uint _2100;
                        uint _2101;
                        bool _2107;
                        for (;;)
                        {
                            _2100 = uint(clamp(_1017, 0.0f, 1.0f) * _202);
                            _2101 = uint(clamp(_1019, 0.0f, 1.0f) * _204);
                            _2107 = _20[21u].Load(int3(uint2(_2100, _2101), 0u)).x > 0.0f;
                            if (_2107)
                            {
                                uint _2292 = _24[23u].Load(int3(uint2(_2100, _2101), 0u)).y + 4294967295u;
                                _2374 = (uint(int(_2292) >> int(31u)) & 3u) + 1u;
                                _2376 = (int(_2292) < int(0u)) ? 0u : _2292;
                                break;
                            }
                            else
                            {
                                uint4 _2300 = _24[2u].Load(int3(uint2(_2100, _2101), 0u));
                                uint _2303 = _2300.w;
                                uint4 _2308 = _24[15u].Load(int3(uint2(_2100, _2101), 0u));
                                uint _2310 = _2308.y;
                                uint _2316 = ((_2310 & 64u) != 0u) ? uint((_2310 & 4294967167u) != 66u) : 4294967295u;
                                uint _2317 = _2303 & 128u;
                                uint _2319 = (_2317 != 0u) ? 1u : ((_2300.x << 7u) | _2303);
                                uint4 _2322 = _16.Load(_2319 * 4u);
                                uint _2323 = _2322.x;
                                uint _2330 = ((_2323 & 1u) != 0u) ? 0u : 18u;
                                uint _2332 = uint(min(int(uint(max(int(_2316), int(0u)))), int(1u)));
                                uint _2387;
                                if (_2317 == 0u)
                                {
                                    _2387 = (((_2323 & 2097152u) != 0u) && (_2316 == _2332)) ? (_2330 | 128u) : _2330;
                                }
                                else
                                {
                                    _2387 = _2303;
                                }
                                uint _2388 = _16.Load((_2319 * 4u) + 1u).x & 512u;
                                bool _2391 = (_2387 & 144u) == 0u;
                                if (_2388 == 0u)
                                {
                                    if (_2391 || ((_2323 & 1u) != 0u))
                                    {
                                        _2374 = 0u;
                                        _2376 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2391)
                                    {
                                        _2374 = 0u;
                                        _2376 = 0u;
                                        break;
                                    }
                                }
                                bool _2664 = ((_2387 & 128u) | _2388) != 0u;
                                uint _2375;
                                if (_2664)
                                {
                                    _2375 = 1u;
                                }
                                else
                                {
                                    _2375 = (((_2323 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_2323 & 268435472u) == 16u) && (((_2323 & 33554432u) == 0u) || _2664))
                                {
                                    _2374 = 2u;
                                    _2376 = 0u;
                                    break;
                                }
                                _2374 = _2375;
                                _2376 = (_2375 == 1u) ? _2332 : 0u;
                                break;
                            }
                        }
                        if ((_397 != _2374) || (_405 != _2376))
                        {
                            _1405 = false;
                            _1408 = _1029;
                            _1410 = _1031;
                            _1412 = _1033;
                            _1414 = 0.0f;
                            _1415 = 1.0f;
                            _1416 = 1000.0f;
                            _1418 = 0.5f;
                            break;
                        }
                        float _2515 = _1023 * 2.0f;
                        float _2518 = (_50_m0[51u].z * _2515) + (-1.0f);
                        float _2519 = ((1.0f - (_50_m0[51u].w * _1025)) * 2.0f) + (-1.0f);
                        float _2535 = mad(_190, _1027, mad(_183, _2519, _2518 * _176)) + _197;
                        float _2536 = (mad(_187, _1027, mad(_180, _2519, _2518 * _173)) + _194) / _2535;
                        float _2537 = (mad(_188, _1027, mad(_181, _2519, _2518 * _174)) + _195) / _2535;
                        float _2538 = (mad(_189, _1027, mad(_182, _2519, _2518 * _175)) + _196) / _2535;
                        if (sqrt(((_2537 * _2537) + (_2536 * _2536)) + (_2538 * _2538)) > _50_m0[58u].w)
                        {
                            _1405 = false;
                            _1408 = _1029;
                            _1410 = _1031;
                            _1412 = _1033;
                            _1414 = 0.0f;
                            _1415 = 0.0f;
                            _1416 = 1000.0f;
                            _1418 = 0.5f;
                            break;
                        }
                        float _2644 = _2536 - _1072;
                        float _2645 = _2537 - _1073;
                        float _2646 = _2538 - _1074;
                        float _2652 = sqrt(((_2645 * _2645) + (_2644 * _2644)) + (_2646 * _2646));
                        float _2660 = min(_50_m0[59u].y, max(0.0f, _2652 + (-0.001000000047497451305389404296875f)));
                        float _2680;
                        if (_1001)
                        {
                            _2680 = min(_50_m0[59u].x, _2660 + 10.0f);
                        }
                        else
                        {
                            _2680 = _50_m0[59u].x;
                        }
                        float _2681 = _2680 - _2652;
                        if (!(_2681 > 0.0f))
                        {
                            _1405 = false;
                            _1408 = _1029;
                            _1410 = _1031;
                            _1412 = _1033;
                            _1414 = 1.0f;
                            _1415 = 1.0f;
                            _1416 = 0.0f;
                            _1418 = 0.5f;
                            break;
                        }
                        float _2740 = _2536 - (_2660 * _1347);
                        float _2741 = _2537 - (_2660 * _1350);
                        float _2742 = _2538 - (_2660 * _1353);
                        RayDesc _2ident = {float3(mad(_2742, _50_m0[46u].z, mad(_2741, _50_m0[46u].y, _50_m0[46u].x * _2740)) + _50_m0[46u].w, mad(_2742, _50_m0[47u].z, mad(_2741, _50_m0[47u].y, _50_m0[47u].x * _2740)) + _50_m0[47u].w, mad(_2742, _50_m0[48u].z, mad(_2741, _50_m0[48u].y, _50_m0[48u].x * _2740)) + _50_m0[48u].w), 0.0f, float3(mad(_1353, _50_m0[46u].z, mad(_1350, _50_m0[46u].y, _50_m0[46u].x * _1347)), mad(_1353, _50_m0[47u].z, mad(_1350, _50_m0[47u].y, _50_m0[47u].x * _1347)), mad(_1353, _50_m0[48u].z, mad(_1350, _50_m0[48u].y, _50_m0[48u].x * _1347))), _2681};
                        _2745.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2792 = _2745.Proceed();
                        uint _2793 = _2745.CommittedStatus();
                        if (!(_2793 == 1u))
                        {
                            _1405 = false;
                            _1408 = _1029;
                            _1410 = _1031;
                            _1412 = _1033;
                            _1414 = 1.0f;
                            _1415 = 0.0f;
                            _1416 = 0.0f;
                            _1418 = 0.5f;
                            break;
                        }
                        float _2807 = _2745.CommittedRayT();
                        if (!((_2807 < _2681) && (_2807 > 0.0f)))
                        {
                            _1405 = false;
                            _1408 = _1029;
                            _1410 = _1031;
                            _1412 = _1033;
                            _1414 = 1.0f;
                            _1415 = 0.0f;
                            _1416 = 0.0f;
                            _1418 = 0.5f;
                            break;
                        }
                        float _2821 = (_50_m0[51u].z * _2515) + (-1.0f);
                        float _2822 = ((1.0f - (_50_m0[51u].w * _1025)) * 2.0f) + (-1.0f);
                        float _2838 = mad(_144, _1027, mad(_137, _2822, _2821 * _130)) + _151;
                        float _2842 = _2807 - _2660;
                        float _2846 = ((mad(_141, _1027, mad(_134, _2822, _2821 * _127)) + _148) / _2838) + (_2842 * _902);
                        float _2847 = ((mad(_142, _1027, mad(_135, _2822, _2821 * _128)) + _149) / _2838) + (_2842 * _903);
                        float _2848 = ((mad(_143, _1027, mad(_136, _2822, _2821 * _129)) + _150) / _2838) + (_2842 * _904);
                        float _2864 = mad(_116, _2848, mad(_109, _2847, _2846 * _102)) + _123;
                        float _1413 = (mad(_115, _2848, mad(_108, _2847, _2846 * _101)) + _122) / _2864;
                        float _1409 = ((((mad(_113, _2848, mad(_106, _2847, _2846 * _99)) + _120) / _2864) * 0.5f) + 0.5f) * _50_m0[51u].x;
                        float _1411 = (0.5f - (((mad(_114, _2848, mad(_107, _2847, _2846 * _100)) + _121) / _2864) * 0.5f)) * _50_m0[51u].y;
                        float _2873 = _1409 * _50_m0[51u].z;
                        float _2874 = _1411 * _50_m0[51u].w;
                        float _2876 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2879 = clamp(_2873 / _2876, 0.0f, 1.0f);
                        float _2880 = clamp(_2874 * 20.0f, 0.0f, 1.0f);
                        float _2890 = clamp(((_2873 + (-1.0f)) + _2876) / _2876, 0.0f, 1.0f);
                        float _2891 = clamp((_2874 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2902 = _2879 * _2880;
                        precise float _2903 = _2902 * _2902;
                        if ((((((3.0f - (_2880 * 2.0f)) * (3.0f - (_2879 * 2.0f))) * _2903) * (1.0f - ((_2890 * _2890) * (3.0f - (_2890 * 2.0f))))) * (1.0f - ((_2891 * _2891) * (3.0f - (_2891 * 2.0f))))) < 1.0f)
                        {
                            _1405 = false;
                            _1408 = _1409;
                            _1410 = _1411;
                            _1412 = _1413;
                            _1414 = _1357;
                            _1415 = _1357;
                            _1416 = 0.0f;
                            _1418 = _1358;
                            break;
                        }
                        _1405 = true;
                        _1408 = _1409;
                        _1410 = _1411;
                        _1412 = _1413;
                        _1414 = 0.0f;
                        _1415 = 1.0f;
                        _1416 = 0.0f;
                        _1418 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1405 = false;
                        _1408 = _1029;
                        _1410 = _1031;
                        _1412 = _1033;
                        _1414 = 1.0f;
                        _1415 = 1.0f;
                        _1416 = 0.0f;
                        _1418 = 0.5f;
                        break;
                    }
                }
                uint4 _1421 = asuint(_55_m0[0u]);
                float _1423 = float(_1421.x);
                float _1425 = float(_1421.y);
                float _1494;
                float _1496;
                float _1498;
                if ((_1185 >= 1.0f) || _1405)
                {
                    _1494 = _1408;
                    _1496 = _1410;
                    _1498 = _1412;
                }
                else
                {
                    float _1647 = (-0.0f) - _947;
                    float _1648 = _1647 / _994;
                    float _1651 = (_1648 * _991) + _954;
                    float _1652 = (_1648 * _993) + _955;
                    float _1653 = _1647 + _947;
                    _1494 = ((_1408 - _1651) * _1185) + _1651;
                    _1496 = ((_1410 - _1652) * _1185) + _1652;
                    _1498 = ((_1412 - _1653) * _1185) + _1653;
                }
                float _1509 = ((_1494 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _1510 = ((1.0f - (_50_m0[51u].w * _1496)) * 2.0f) + (-1.0f);
                float _1526 = mad(_144, _1498, mad(_137, _1510, _1509 * _130)) + _151;
                float _1530 = ((mad(_141, _1498, mad(_134, _1510, _1509 * _127)) + _148) / _1526) - _926;
                float _1531 = ((mad(_142, _1498, mad(_135, _1510, _1509 * _128)) + _149) / _1526) - _927;
                float _1532 = ((mad(_143, _1498, mad(_136, _1510, _1509 * _129)) + _150) / _1526) - _928;
                float _1538 = sqrt(((_1531 * _1531) + (_1530 * _1530)) + (_1532 * _1532));
                float _1539 = _1538 * _778;
                float _1540 = _1538 * _779;
                float _1541 = _1538 * _780;
                float _1542 = _1538 * (_879 / _882);
                float _1543 = _1538 * (_880 / _882);
                float _1544 = _1538 * (_881 / _882);
                float _1548 = dot(float3(_1539, _1540, _1541), float3(_764, _767, _770)) * 2.0f;
                float _1558 = dot(float3(_1542, _1543, _1544), float3(_764, _767, _770)) * 2.0f;
                float _1585 = (_1539 - (_1548 * _764)) + _926;
                float _1586 = (_1540 - (_1548 * _767)) + _927;
                float _1587 = (_1541 - (_1548 * _770)) + _928;
                float _1599 = mad(_50_m0[24u].w, _1587, mad(_50_m0[23u].w, _1586, _1585 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1604 = (_1542 - (_1558 * _764)) + _926;
                float _1605 = (_1543 - (_1558 * _767)) + _927;
                float _1606 = (_1544 - (_1558 * _770)) + _928;
                float _1618 = mad(_50_m0[24u].w, _1606, mad(_50_m0[23u].w, _1605, _1604 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1624 = (_50_m0[51u].x * ((((mad(_50_m0[24u].x, _1587, mad(_50_m0[23u].x, _1586, _1585 * _50_m0[22u].x)) + _50_m0[25u].x) / _1599) - ((mad(_50_m0[24u].x, _1606, mad(_50_m0[23u].x, _1605, _1604 * _50_m0[22u].x)) + _50_m0[25u].x) / _1618)) * 0.5f)) * _1423;
                float _1628 = (_50_m0[51u].y * ((((mad(_50_m0[24u].y, _1606, mad(_50_m0[23u].y, _1605, _1604 * _50_m0[22u].y)) + _50_m0[25u].y) / _1618) - ((mad(_50_m0[24u].y, _1587, mad(_50_m0[23u].y, _1586, _1585 * _50_m0[22u].y)) + _50_m0[25u].y) / _1599)) * 0.5f)) * _1425;
                uint4 _1638 = asuint(_50_m0[55u]);
                float _1643 = clamp(log2(sqrt((_1628 * _1628) + (_1624 * _1624)) * 2.0f) / float(_1638.x + 4294967295u), 0.0f, 1.0f);
                uint _1741;
                float _1742;
                float _1743;
                float _1745;
                float _1747;
                if (_1001 && (_1638.z != 0u))
                {
                    float frontier_phi_83_82_ladder;
                    uint frontier_phi_83_82_ladder_1;
                    float frontier_phi_83_82_ladder_2;
                    float frontier_phi_83_82_ladder_3;
                    float frontier_phi_83_82_ladder_4;
                    if ((_1341 != 1u) || (_1343 != 0u))
                    {
                        float _1845 = _947 / max(9.9999999747524270787835121154785e-07f, (-0.0f) - _994);
                        float _1850 = ((_1845 * _991) + _954) / _50_m0[51u].x;
                        float _1851 = ((_1845 * _993) + _955) / _50_m0[51u].y;
                        float _1999;
                        if (_1851 < 0.300000011920928955078125f)
                        {
                            float _1991 = (0.300000011920928955078125f - _1851) * 3.3333332538604736328125f;
                            _1999 = 0.300000011920928955078125f - ((_1991 / sqrt((_1991 * _1991) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _1999 = _1851;
                        }
                        float _2116;
                        if (_1850 < 0.300000011920928955078125f)
                        {
                            float _2109 = (0.300000011920928955078125f - _1850) * 3.3333332538604736328125f;
                            _2116 = 0.300000011920928955078125f - ((_2109 / sqrt((_2109 * _2109) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _2116 = _1850;
                        }
                        float _2344;
                        if ((1.0f - _2116) < 0.300000011920928955078125f)
                        {
                            float _2336 = (_2116 + (-0.699999988079071044921875f)) * 3.3333332538604736328125f;
                            _2344 = ((_2336 / sqrt((_2336 * _2336) + 1.0f)) * 0.300000011920928955078125f) + 0.699999988079071044921875f;
                        }
                        else
                        {
                            _2344 = _2116;
                        }
                        frontier_phi_83_82_ladder = _1999 * _50_m0[51u].y;
                        frontier_phi_83_82_ladder_1 = 0u;
                        frontier_phi_83_82_ladder_2 = 0.0f;
                        frontier_phi_83_82_ladder_3 = _2344 * _50_m0[51u].x;
                        frontier_phi_83_82_ladder_4 = 1.0f - clamp((_904 + (-0.25f)) * (-4.0f), 0.0f, 1.0f);
                    }
                    else
                    {
                        frontier_phi_83_82_ladder = _1041;
                        frontier_phi_83_82_ladder_1 = _1013;
                        frontier_phi_83_82_ladder_2 = _1021;
                        frontier_phi_83_82_ladder_3 = _1039;
                        frontier_phi_83_82_ladder_4 = _1185;
                    }
                    _1741 = frontier_phi_83_82_ladder_1;
                    _1742 = frontier_phi_83_82_ladder_2;
                    _1743 = frontier_phi_83_82_ladder_3;
                    _1745 = frontier_phi_83_82_ladder;
                    _1747 = frontier_phi_83_82_ladder_4;
                }
                else
                {
                    _1741 = _1013;
                    _1742 = _1021;
                    _1743 = _1039;
                    _1745 = _1041;
                    _1747 = _1185;
                }
                bool _1749 = _397 != 1u;
                float _2119;
                float _2121;
                float _2123;
                float _2125;
                if (_1747 == 0.0f)
                {
                    float _2001;
                    float _2003;
                    float _2005;
                    float _2007;
                    if (_1405)
                    {
                        _2001 = 0.0f;
                        _2003 = 0.0f;
                        _2005 = 0.0f;
                        _2007 = 1.0f;
                    }
                    else
                    {
                        float frontier_phi_109_110_ladder;
                        float frontier_phi_109_110_ladder_1;
                        float frontier_phi_109_110_ladder_2;
                        float frontier_phi_109_110_ladder_3;
                        if (_1749)
                        {
                            float _2176 = _1350 * _1226;
                            float _2180 = rsqrt(dot(float3(_1347, _2176, _1353), float3(_1347, _2176, _1353)));
                            float4 _2190 = _28[4u].SampleLevel(_59, float3(_2180 * _1347, _2180 * _2176, _2180 * _1353), 0.0f);
                            frontier_phi_109_110_ladder = 1.0f;
                            frontier_phi_109_110_ladder_1 = _2190.z;
                            frontier_phi_109_110_ladder_2 = _2190.y;
                            frontier_phi_109_110_ladder_3 = _2190.x;
                        }
                        else
                        {
                            frontier_phi_109_110_ladder = 0.0f;
                            frontier_phi_109_110_ladder_1 = 0.0f;
                            frontier_phi_109_110_ladder_2 = 0.0f;
                            frontier_phi_109_110_ladder_3 = 0.0f;
                        }
                        _2001 = frontier_phi_109_110_ladder_3;
                        _2003 = frontier_phi_109_110_ladder_2;
                        _2005 = frontier_phi_109_110_ladder_1;
                        _2007 = frontier_phi_109_110_ladder;
                    }
                    _2119 = _2001 * _475;
                    _2121 = _2003 * _475;
                    _2123 = _2005 * _475;
                    _2125 = _2007 * _475;
                }
                else
                {
                    float _2216;
                    float _2218;
                    float _2221;
                    float _2223;
                    if (_12.Load(int3(uint2(uint(_1743 * _1423), uint(_1745 * _1425)), 0u)).x > 0.0f)
                    {
                        uint _2013_dummy_parameter;
                        uint2 _2013 = spvTextureSize(_14, 0u, _2013_dummy_parameter);
                        float4 _2022 = _14.Load(int3(uint2(uint(float(_2013.x) * _1743), uint(float(_2013.y) * _1745)), 0u));
                        float _2026 = _2022.x * 0.5f;
                        float _2027 = _2022.y * (-0.5f);
                        float4 _2046 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _2026) + (_50_m0[52u].x * _1743), (_50_m0[52u].w * _2027) + (_50_m0[52u].y * _1745)), 0.0f);
                        float _2056 = _50_m0[54u].x * _2046.x;
                        float _2057 = _50_m0[54u].x * _2046.y;
                        float _2058 = _50_m0[54u].x * _2046.z;
                        float _2203;
                        if (_1001)
                        {
                            float frontier_phi_124_123_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _947))) < 5.0f)
                            {
                                float _2360 = sqrt((_2026 * _2026) + (_2027 * _2027));
                                float frontier_phi_124_123_ladder_137_ladder;
                                if (_2360 > 0.0500000007450580596923828125f)
                                {
                                    float _2399 = _1743 - _954;
                                    float _2400 = _1745 - _955;
                                    float frontier_phi_124_123_ladder_137_ladder_148_ladder;
                                    if (_2360 > sqrt((_2400 * _2400) + (_2399 * _2399)))
                                    {
                                        uint4 _2562 = asuint(_55_m0[0u]);
                                        uint _2569 = uint(float(_2562.x) * _1743);
                                        uint _2570 = uint(float(_2562.y) * _1745);
                                        uint4 _2573 = _24[2u].Load(int3(uint2(_2569, _2570), 0u));
                                        uint _2576 = _2573.w;
                                        uint4 _2581 = _24[15u].Load(int3(uint2(_2569, _2570), 0u));
                                        uint _2583 = _2581.y;
                                        uint _2589 = ((_2583 & 64u) != 0u) ? uint((_2583 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2590 = _2576 & 128u;
                                        uint _2592 = (_2590 != 0u) ? 1u : ((_2573.x << 7u) | _2576);
                                        uint4 _2595 = _16.Load(_2592 * 4u);
                                        uint _2596 = _2595.x;
                                        uint _2603 = ((_2596 & 1u) != 0u) ? 0u : 18u;
                                        uint _2673;
                                        if (_2590 == 0u)
                                        {
                                            _2673 = (((_2596 & 2097152u) != 0u) && (_2589 == uint(min(int(uint(max(int(_2589), int(0u)))), int(1u))))) ? (_2603 | 128u) : _2603;
                                        }
                                        else
                                        {
                                            _2673 = _2576;
                                        }
                                        float frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder;
                                        if (((_2673 & 128u) | (_16.Load((_2592 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2695 = asuint(_55_m0[0u]);
                                            uint _2704 = uint(float(_2695.x) * (_2026 + _1743));
                                            uint _2705 = uint(float(_2695.y) * (_2027 + _1745));
                                            uint4 _2708 = _24[2u].Load(int3(uint2(_2704, _2705), 0u));
                                            uint _2711 = _2708.w;
                                            uint4 _2714 = _24[15u].Load(int3(uint2(_2704, _2705), 0u));
                                            uint _2716 = _2714.y;
                                            uint _2722 = ((_2716 & 64u) != 0u) ? uint((_2716 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2723 = _2711 & 128u;
                                            uint _2725 = (_2723 != 0u) ? 1u : ((_2708.x << 7u) | _2711);
                                            uint4 _2727 = _16.Load(_2725 * 4u);
                                            uint _2728 = _2727.x;
                                            uint _2735 = ((_2728 & 1u) != 0u) ? 0u : 18u;
                                            uint _2804;
                                            if (_2723 == 0u)
                                            {
                                                _2804 = (((_2728 & 2097152u) != 0u) && (_2722 == uint(min(int(uint(max(int(_2722), int(0u)))), int(1u))))) ? (_2735 | 128u) : _2735;
                                            }
                                            else
                                            {
                                                _2804 = _2711;
                                            }
                                            float frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder_174_ladder;
                                            if ((_2804 & 128u) == 0u)
                                            {
                                                frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder_174_ladder = ((_16.Load((_2725 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder_174_ladder = 0.0f;
                                            }
                                            frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder = frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder_174_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder = 1.0f;
                                        }
                                        frontier_phi_124_123_ladder_137_ladder_148_ladder = frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_124_123_ladder_137_ladder_148_ladder = 1.0f;
                                    }
                                    frontier_phi_124_123_ladder_137_ladder = frontier_phi_124_123_ladder_137_ladder_148_ladder;
                                }
                                else
                                {
                                    frontier_phi_124_123_ladder_137_ladder = 1.0f;
                                }
                                frontier_phi_124_123_ladder = frontier_phi_124_123_ladder_137_ladder;
                            }
                            else
                            {
                                frontier_phi_124_123_ladder = 1.0f;
                            }
                            _2203 = frontier_phi_124_123_ladder;
                        }
                        else
                        {
                            _2203 = 1.0f;
                        }
                        float frontier_phi_126_124_ladder;
                        float frontier_phi_126_124_ladder_1;
                        float frontier_phi_126_124_ladder_2;
                        float frontier_phi_126_124_ladder_3;
                        float _2205;
                        for (;;)
                        {
                            _2205 = _2203 * _1747;
                            if (_1742 > 0.0f)
                            {
                                float _2220;
                                float _2222;
                                float _2224;
                                if (_1001)
                                {
                                    _2220 = 0.0f;
                                    _2222 = 0.0f;
                                    _2224 = 0.0f;
                                }
                                else
                                {
                                    if (!((_397 != 4u) || (_1015 != 0u)))
                                    {
                                        frontier_phi_126_124_ladder = _2057;
                                        frontier_phi_126_124_ladder_1 = _2205;
                                        frontier_phi_126_124_ladder_2 = _2056;
                                        frontier_phi_126_124_ladder_3 = _2058;
                                        break;
                                    }
                                    _2220 = _2056;
                                    _2222 = _2057;
                                    _2224 = _2058;
                                }
                                frontier_phi_126_124_ladder = _2222;
                                frontier_phi_126_124_ladder_1 = (1.0f - exp2(log2(_1742) * 3.0f)) * _2205;
                                frontier_phi_126_124_ladder_2 = _2220;
                                frontier_phi_126_124_ladder_3 = _2224;
                                break;
                            }
                            else
                            {
                                frontier_phi_126_124_ladder = _2057;
                                frontier_phi_126_124_ladder_1 = _2205;
                                frontier_phi_126_124_ladder_2 = _2056;
                                frontier_phi_126_124_ladder_3 = _2058;
                                break;
                            }
                        }
                        _2216 = frontier_phi_126_124_ladder_1;
                        _2218 = frontier_phi_126_124_ladder_2;
                        _2221 = frontier_phi_126_124_ladder;
                        _2223 = frontier_phi_126_124_ladder_3;
                    }
                    else
                    {
                        float frontier_phi_126_112_ladder;
                        float frontier_phi_126_112_ladder_1;
                        float frontier_phi_126_112_ladder_2;
                        float frontier_phi_126_112_ladder_3;
                        if (_1741 == 0u)
                        {
                            float4 _2211 = _28[7u].SampleLevel(_59, float3(_1347, _1350, _1353), 0.0f);
                            frontier_phi_126_112_ladder = _2211.y;
                            frontier_phi_126_112_ladder_1 = _1747;
                            frontier_phi_126_112_ladder_2 = _2211.x;
                            frontier_phi_126_112_ladder_3 = _2211.z;
                        }
                        else
                        {
                            frontier_phi_126_112_ladder = _2219;
                            frontier_phi_126_112_ladder_1 = 0.0f;
                            frontier_phi_126_112_ladder_2 = _2219;
                            frontier_phi_126_112_ladder_3 = _2219;
                        }
                        _2216 = frontier_phi_126_112_ladder_1;
                        _2218 = frontier_phi_126_112_ladder_2;
                        _2221 = frontier_phi_126_112_ladder;
                        _2223 = frontier_phi_126_112_ladder_3;
                    }
                    float _2413;
                    float _2414;
                    float _2416;
                    float _2418;
                    if (_1405)
                    {
                        _2413 = 1.0f;
                        _2414 = _2218 * _1181;
                        _2416 = _2221 * _1181;
                        _2418 = _2223 * _1181;
                    }
                    else
                    {
                        float frontier_phi_151_140_ladder;
                        float frontier_phi_151_140_ladder_1;
                        float frontier_phi_151_140_ladder_2;
                        float frontier_phi_151_140_ladder_3;
                        if (_1749 && (_2216 < 1.0f))
                        {
                            float _2420 = _1350 * _1226;
                            float _2424 = rsqrt(dot(float3(_1347, _2420, _1353), float3(_1347, _2420, _1353)));
                            float4 _2432 = _28[4u].SampleLevel(_59, float3(_2424 * _1347, _2424 * _2420, _2424 * _1353), 0.0f);
                            float _2434 = _2432.x;
                            float _2435 = _2432.y;
                            float _2436 = _2432.z;
                            frontier_phi_151_140_ladder = ((_2223 - _2436) * _2216) + _2436;
                            frontier_phi_151_140_ladder_1 = ((_2221 - _2435) * _2216) + _2435;
                            frontier_phi_151_140_ladder_2 = ((_2218 - _2434) * _2216) + _2434;
                            frontier_phi_151_140_ladder_3 = 1.0f;
                        }
                        else
                        {
                            frontier_phi_151_140_ladder = _2223;
                            frontier_phi_151_140_ladder_1 = _2221;
                            frontier_phi_151_140_ladder_2 = _2218;
                            frontier_phi_151_140_ladder_3 = _2216;
                        }
                        _2413 = frontier_phi_151_140_ladder_3;
                        _2414 = frontier_phi_151_140_ladder_2;
                        _2416 = frontier_phi_151_140_ladder_1;
                        _2418 = frontier_phi_151_140_ladder;
                    }
                    float _2126 = _2413 * _475;
                    _2119 = _2414 * _2126;
                    _2121 = _2416 * _2126;
                    _2123 = _2418 * _2126;
                    _2125 = _2126;
                }
                float _2130 = _50_m0[58u].z * _1418;
                float _2152 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _2154 = _2152 * ((_2130 * ((_1414 * 1000.0f) - _2119)) + _2119);
                float _2155 = _2152 * ((_2130 * ((_1415 * 1000.0f) - _2121)) + _2121);
                float _2156 = _2152 * ((_2130 * (_1416 - _2123)) + _2123);
                float _2162 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2154, max(_2155, _2156)) + 1.0f);
                float _2166 = min(_2162 * _2154, 0.996078431606292724609375f);
                float _2168 = min(_2162 * _2155, 0.996078431606292724609375f);
                float _2169 = min(_2162 * _2156, 0.996078431606292724609375f);
                _35[uint2(_221, _224)] = float4(_2166, _2168, _2169, _2125);
                _39[uint2(_221, _224)] = float4(_1643, 0.0f, 0.0f, _1643);
                if (_228)
                {
                    uint _2350 = _221 + 1u;
                    _35[uint2(_2350, _224)] = float4(_2166, _2168, _2169, _2125);
                    _39[uint2(_2350, _224)] = float4(_1643, 0.0f, 0.0f, _1643);
                }
                if (_231)
                {
                    uint _2392 = _224 + 1u;
                    _35[uint2(_221, _2392)] = float4(_2166, _2168, _2169, _2125);
                    _39[uint2(_221, _2392)] = float4(_1643, 0.0f, 0.0f, _1643);
                }
                if (_232)
                {
                    uint _2552 = _221 + 1u;
                    uint _2553 = _224 + 1u;
                    _35[uint2(_2552, _2553)] = float4(_2166, _2168, _2169, _2125);
                    _39[uint2(_2552, _2553)] = float4(_1643, 0.0f, 0.0f, _1643);
                }
                ladder_phi_8 = true;
                break;
            }
            if (ladder_phi_8)
            {
                break;
            }
            _35[uint2(_221, _224)] = 0.0f.xxxx;
            _39[uint2(_221, _224)] = 0.0f.xxxx;
            if (_228)
            {
                uint _440 = _221 + 1u;
                _35[uint2(_440, _224)] = 0.0f.xxxx;
                _39[uint2(_440, _224)] = 0.0f.xxxx;
            }
            if (_231)
            {
                uint _551 = _224 + 1u;
                _35[uint2(_221, _551)] = 0.0f.xxxx;
                _39[uint2(_221, _551)] = 0.0f.xxxx;
            }
            if (!_232)
            {
                break;
            }
            uint _650 = _221 + 1u;
            uint _651 = _224 + 1u;
            _35[uint2(_650, _651)] = 0.0f.xxxx;
            _39[uint2(_650, _651)] = 0.0f.xxxx;
            break;
        }
        else
        {
            break;
        }
    }
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_WorkGroupID = stage_input.gl_WorkGroupID;
    gl_LocalInvocationIndex = stage_input.gl_LocalInvocationIndex;
    comp_main();
}
