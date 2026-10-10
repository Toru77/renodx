static float _2220;
static uint _3240;
static float _3241;
static float _3242;
static float _3243;
static float _3244;
static float _3245;
static float _3246;
static float _3247;
static float _3248;
static float _3249;
static uint _3250;
static uint _3251;
static float _3252;
static float _3253;
static float _3254;
static float _3255;
static float _3256;
static float _3262;
static uint _3263;
static float _3264;
static uint _3266;
static uint _3267;
static float _3272;

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

static RayQuery<RAY_FLAG_NONE> _2746;

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
            float frontier_phi_8_pred;
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
                    uint _428;
                    uint _429;
                    if (_368 == 0u)
                    {
                        _428 = (((_376 & 2097152u) != 0u) && (_367 == _389)) ? (_387 | 128u) : _387;
                        _429 = _376;
                    }
                    else
                    {
                        _428 = _350;
                        _429 = _376 | ((_349 << 20u) & 134217728u);
                    }
                    uint _438 = _380 & 512u;
                    float _794;
                    float _796;
                    float _798;
                    float _800;
                    float _802;
                    float _804;
                    float _806;
                    float _808;
                    float _810;
                    float _812;
                    float _814;
                    float _816;
                    if (_438 == 0u)
                    {
                        if (!((_429 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            frontier_phi_8_pred = 0.0f;
                            break;
                        }
                        float _580 = asfloat(_17.Load((_384 * 115u) + 33u).x);
                        uint4 _588 = _24[2u].Load(int3(uint2(_265, _266), 0u));
                        uint _590 = _588.y;
                        uint _591 = _428 & 128u;
                        uint _782;
                        uint _783;
                        uint _784;
                        uint _785;
                        if (_591 == 0u)
                        {
                            _782 = uint(((_429 & 817889384u) | (_380 & 576u)) != 0u) | (((_429 >> 19u) & 1u) ^ 1u);
                            _783 = uint(((_429 & 17825808u) | (_380 & 520u)) != 0u);
                            _784 = uint(((_429 & 46137344u) | (_380 & 2564u)) != 0u);
                            _785 = 0u;
                        }
                        else
                        {
                            _782 = 1u;
                            _783 = _428 & 1u;
                            _784 = 1u;
                            _785 = 1u;
                        }
                        precise float _789 = float(_590 & 127u) * 0.0078740157186985015869140625f;
                        bool _793 = (_429 & 4194304u) == 0u;
                        float _1004;
                        if (_793)
                        {
                            _1004 = _789;
                        }
                        else
                        {
                            _1004 = float(_590 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1268;
                        if ((_429 & 134217728u) == 0u)
                        {
                            uint frontier_phi_47_40_ladder;
                            if ((_591 != 0u) || ((_429 & 17825792u) == 1048576u))
                            {
                                frontier_phi_47_40_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_47_40_ladder = _783;
                            }
                            _1268 = frontier_phi_47_40_ladder;
                        }
                        else
                        {
                            _1268 = _783;
                        }
                        uint4 _1271 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _1273 = _1271.x;
                        float _1376;
                        float _1378;
                        float _1380;
                        if (_782 == 0u)
                        {
                            _1376 = 0.0f;
                            _1378 = 0.0f;
                            _1380 = 0.0f;
                        }
                        else
                        {
                            float4 _1385 = _20[8u].Load(int3(uint2(_265, _266), 0u));
                            _1376 = _1385.x;
                            _1378 = _1385.y;
                            _1380 = _1385.z;
                        }
                        uint _1482;
                        if (_1268 == 0u)
                        {
                            _1482 = 0u;
                        }
                        else
                        {
                            _1482 = _24[9u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        uint _1688;
                        if (_784 == 0u)
                        {
                            _1688 = 0u;
                        }
                        else
                        {
                            _1688 = _24[10u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        float _1698 = (float((_1273 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1699 = (float(_1273 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1703 = (1.0f - abs(_1698)) - abs(_1699);
                        float _1705 = clamp((-0.0f) - _1703, 0.0f, 1.0f);
                        float _1706 = (-0.0f) - _1705;
                        float _1711 = ((_1698 >= 0.0f) ? _1706 : _1705) + _1698;
                        float _1712 = ((_1699 >= 0.0f) ? _1706 : _1705) + _1699;
                        float _1716 = rsqrt(dot(float3(_1711, _1712, _1703), float3(_1711, _1712, _1703)));
                        float _1717 = _1711 * _1716;
                        float _1718 = _1712 * _1716;
                        float _1719 = _1716 * _1703;
                        float _795 = float(_1273 & 255u);
                        float _1723 = ((_429 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1973;
                        float _1974;
                        float _1975;
                        float _1976;
                        uint _1977;
                        if ((_380 & 64u) == 0u)
                        {
                            uint frontier_phi_104_89_ladder;
                            float frontier_phi_104_89_ladder_1;
                            float frontier_phi_104_89_ladder_2;
                            float frontier_phi_104_89_ladder_3;
                            float frontier_phi_104_89_ladder_4;
                            if ((_429 & 276824064u) == 0u)
                            {
                                frontier_phi_104_89_ladder = 0u;
                                frontier_phi_104_89_ladder_1 = 0.0f;
                                frontier_phi_104_89_ladder_2 = 0.0f;
                                frontier_phi_104_89_ladder_3 = 0.0f;
                                frontier_phi_104_89_ladder_4 = ((_429 & 8u) != 0u) ? _1378 : _1723;
                            }
                            else
                            {
                                frontier_phi_104_89_ladder = 0u;
                                frontier_phi_104_89_ladder_1 = 0.0f;
                                frontier_phi_104_89_ladder_2 = 0.0f;
                                frontier_phi_104_89_ladder_3 = 0.0f;
                                frontier_phi_104_89_ladder_4 = _1723;
                            }
                            _1973 = frontier_phi_104_89_ladder_4;
                            _1974 = frontier_phi_104_89_ladder_3;
                            _1975 = frontier_phi_104_89_ladder_2;
                            _1976 = frontier_phi_104_89_ladder_1;
                            _1977 = frontier_phi_104_89_ladder;
                        }
                        else
                        {
                            float _1805 = (_1378 * 2.0f) + (-1.0f);
                            float _1806 = (_1380 * 2.0f) + (-1.0f);
                            float _1810 = (1.0f - abs(_1805)) - abs(_1806);
                            float _1812 = clamp((-0.0f) - _1810, 0.0f, 1.0f);
                            float _1813 = (-0.0f) - _1812;
                            float _1818 = ((_1805 >= 0.0f) ? _1813 : _1812) + _1805;
                            float _1819 = ((_1806 >= 0.0f) ? _1813 : _1812) + _1806;
                            float _1823 = rsqrt(dot(float3(_1818, _1819, _1810), float3(_1818, _1819, _1810)));
                            _1973 = floor(round(_1376 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1974 = _1818 * _1823;
                            _1975 = _1819 * _1823;
                            _1976 = _1823 * _1810;
                            _1977 = 1u;
                        }
                        float _803;
                        if ((_429 & 32768u) == 0u)
                        {
                            _803 = _1973;
                        }
                        else
                        {
                            float frontier_phi_116_117_ladder;
                            if (_17.Load((_384 * 115u) + 36u).x == 0u)
                            {
                                float _2286 = clamp((_795 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _580;
                                frontier_phi_116_117_ladder = ((_429 & 131072u) != 0u) ? _2286 : ((((clamp((1.21000003814697265625f / (exp2((_1004 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_384 * 115u) + 32u).x)) + 1.0f) * _2286);
                            }
                            else
                            {
                                frontier_phi_116_117_ladder = _580;
                            }
                            _803 = frontier_phi_116_117_ladder;
                        }
                        uint _2086 = _428 & 1u;
                        float _2229;
                        float _2231;
                        float _2233;
                        uint _2235;
                        if (((_429 & 16u) == 0u) || (((_2086 | (_380 & 8u)) | (_429 & 16777216u)) != 0u))
                        {
                            _2229 = _1974;
                            _2231 = _1975;
                            _2233 = _1976;
                            _2235 = _1977;
                        }
                        else
                        {
                            float _2245 = (float(_1482 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2246 = (float(_1482 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2250 = (1.0f - abs(_2245)) - abs(_2246);
                            float _2252 = clamp((-0.0f) - _2250, 0.0f, 1.0f);
                            float _2253 = (-0.0f) - _2252;
                            float _2258 = ((_2245 >= 0.0f) ? _2253 : _2252) + _2245;
                            float _2259 = ((_2246 >= 0.0f) ? _2253 : _2252) + _2246;
                            float _2263 = rsqrt(dot(float3(_2258, _2259, _2250), float3(_2258, _2259, _2250)));
                            _2229 = _2258 * _2263;
                            _2231 = _2259 * _2263;
                            _2233 = _2263 * _2250;
                            _2235 = 1u;
                        }
                        float _797;
                        float _799;
                        float _801;
                        if (_2086 == 0u)
                        {
                            float frontier_phi_142_141_ladder;
                            float frontier_phi_142_141_ladder_1;
                            float frontier_phi_142_141_ladder_2;
                            if (((_428 & 64u) == 0u) && (_785 != 0u))
                            {
                                float2 _2457 = spvUnpackHalf2x16((_1688 >> 17u) & 32736u);
                                float _2458 = _2457.x;
                                float _2461 = (spvUnpackHalf2x16((_1688 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2462 = (spvUnpackHalf2x16((_1688 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2466 = (1.0f - abs(_2461)) - abs(_2462);
                                float _2468 = clamp((-0.0f) - _2466, 0.0f, 1.0f);
                                float _2469 = (-0.0f) - _2468;
                                float _2474 = ((_2461 >= 0.0f) ? _2469 : _2468) + _2461;
                                float _2475 = ((_2462 >= 0.0f) ? _2469 : _2468) + _2462;
                                float _2479 = rsqrt(dot(float3(_2474, _2475, _2466), float3(_2474, _2475, _2466)));
                                float _2489 = (((_2474 * _2479) - _1717) * _2458) + _1717;
                                float _2490 = (((_2475 * _2479) - _1718) * _2458) + _1718;
                                float _2491 = (((_2479 * _2466) - _1719) * _2458) + _1719;
                                float _2495 = rsqrt(dot(float3(_2489, _2490, _2491), float3(_2489, _2490, _2491)));
                                frontier_phi_142_141_ladder = _2491 * _2495;
                                frontier_phi_142_141_ladder_1 = _2490 * _2495;
                                frontier_phi_142_141_ladder_2 = _2489 * _2495;
                            }
                            else
                            {
                                frontier_phi_142_141_ladder = _1719;
                                frontier_phi_142_141_ladder_1 = _1718;
                                frontier_phi_142_141_ladder_2 = _1717;
                            }
                            _797 = frontier_phi_142_141_ladder_2;
                            _799 = frontier_phi_142_141_ladder_1;
                            _801 = frontier_phi_142_141_ladder;
                        }
                        else
                        {
                            _797 = _1717;
                            _799 = _1718;
                            _801 = _1719;
                        }
                        float _811;
                        float _813;
                        float _815;
                        float _817;
                        if (_793)
                        {
                            float frontier_phi_155_154_ladder;
                            float frontier_phi_155_154_ladder_1;
                            float frontier_phi_155_154_ladder_2;
                            float frontier_phi_155_154_ladder_3;
                            if (((_429 & 33554432u) == 0u) || (((_380 & 4u) != 0u) && ((_429 & 8388608u) == 0u)))
                            {
                                frontier_phi_155_154_ladder = 0.0f;
                                frontier_phi_155_154_ladder_1 = 0.0f;
                                frontier_phi_155_154_ladder_2 = 0.0f;
                                frontier_phi_155_154_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2619 = (spvUnpackHalf2x16((_1688 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2620 = (spvUnpackHalf2x16((_1688 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2624 = (1.0f - abs(_2619)) - abs(_2620);
                                float _2626 = clamp((-0.0f) - _2624, 0.0f, 1.0f);
                                float _2627 = (-0.0f) - _2626;
                                float _2632 = ((_2619 >= 0.0f) ? _2627 : _2626) + _2619;
                                float _2633 = ((_2620 >= 0.0f) ? _2627 : _2626) + _2620;
                                float _2637 = rsqrt(dot(float3(_2632, _2633, _2624), float3(_2632, _2633, _2624)));
                                float _2638 = _2632 * _2637;
                                float _2639 = _2633 * _2637;
                                float _2640 = _2637 * _2624;
                                float _2644 = rsqrt(dot(float3(_2638, _2639, _2640), float3(_2638, _2639, _2640)));
                                frontier_phi_155_154_ladder = _2644 * _2640;
                                frontier_phi_155_154_ladder_1 = _2644 * _2639;
                                frontier_phi_155_154_ladder_2 = _2644 * _2638;
                                frontier_phi_155_154_ladder_3 = spvUnpackHalf2x16((_1688 >> 17u) & 32736u).x;
                            }
                            _811 = frontier_phi_155_154_ladder_3;
                            _813 = frontier_phi_155_154_ladder_2;
                            _815 = frontier_phi_155_154_ladder_1;
                            _817 = frontier_phi_155_154_ladder;
                        }
                        else
                        {
                            _811 = 0.0f;
                            _813 = 0.0f;
                            _815 = 0.0f;
                            _817 = 0.0f;
                        }
                        bool _2509 = _2235 != 0u;
                        _794 = _795;
                        _796 = _797;
                        _798 = _799;
                        _800 = _801;
                        _802 = _803;
                        _804 = _2509 ? _2229 : _797;
                        _806 = _2509 ? _2231 : _799;
                        _808 = _2509 ? _2233 : _801;
                        _810 = _811;
                        _812 = _813;
                        _814 = _815;
                        _816 = _817;
                    }
                    else
                    {
                        uint4 _540 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _542 = _540.x;
                        uint4 _546 = _24[9u].Load(int3(uint2(_265, _266), 0u));
                        uint _548 = _546.x;
                        float _722;
                        float _723;
                        float _724;
                        if ((_429 & 33554432u) == 0u)
                        {
                            float _600 = (float((_542 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _601 = (float(_542 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _605 = (1.0f - abs(_600)) - abs(_601);
                            float _607 = clamp((-0.0f) - _605, 0.0f, 1.0f);
                            float _608 = (-0.0f) - _607;
                            float _613 = ((_600 >= 0.0f) ? _608 : _607) + _600;
                            float _614 = ((_601 >= 0.0f) ? _608 : _607) + _601;
                            float _618 = rsqrt(dot(float3(_613, _614, _605), float3(_613, _614, _605)));
                            _722 = _613 * _618;
                            _723 = _614 * _618;
                            _724 = _618 * _605;
                        }
                        else
                        {
                            float _629 = (float((_548 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _630 = (float(_548 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _634 = (1.0f - abs(_629)) - abs(_630);
                            float _636 = clamp((-0.0f) - _634, 0.0f, 1.0f);
                            float _637 = (-0.0f) - _636;
                            float _642 = ((_629 >= 0.0f) ? _637 : _636) + _629;
                            float _643 = ((_630 >= 0.0f) ? _637 : _636) + _630;
                            float _647 = rsqrt(dot(float3(_642, _643, _634), float3(_642, _643, _634)));
                            _722 = _642 * _647;
                            _723 = _643 * _647;
                            _724 = _647 * _634;
                        }
                        _794 = float(_542 & 255u);
                        _796 = _722;
                        _798 = _723;
                        _800 = _724;
                        _802 = 1.0f;
                        _804 = _722;
                        _806 = _723;
                        _808 = _724;
                        _810 = 0.0f;
                        _812 = 0.0f;
                        _814 = 0.0f;
                        _816 = 0.0f;
                    }
                    precise float _818 = _794 * 0.0039215688593685626983642578125f;
                    if ((_428 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        frontier_phi_8_pred = 0.0f;
                        break;
                    }
                    bool _1013 = ((_428 & 128u) | _438) != 0u;
                    uint _1174;
                    if (_1013)
                    {
                        _1174 = 1u;
                    }
                    else
                    {
                        _1174 = (((_429 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _392;
                    float _394;
                    float _396;
                    uint _398;
                    float _402;
                    if (((_429 & 33554432u) == 0u) || _1013)
                    {
                        bool _1275 = _77 != 0u;
                        uint _1282;
                        if ((_429 & 16u) == 0u)
                        {
                            _1282 = _1174;
                        }
                        else
                        {
                            _1282 = ((_429 & 268435456u) != 0u) ? _1174 : 2u;
                        }
                        _402 = _802 * _818;
                        _392 = _1275 ? _804 : _796;
                        _394 = _1275 ? _806 : _798;
                        _396 = _1275 ? _808 : _800;
                        _398 = _1282;
                    }
                    else
                    {
                        _402 = _810;
                        _392 = _812;
                        _394 = _814;
                        _396 = _816;
                        _398 = _1174;
                    }
                    uint _1283 = _398 + 102u;
                    float _1292 = clamp((_402 - _45_m0[_1283].x) / (_45_m0[_1283].y - _45_m0[_1283].x), 0.0f, 1.0f);
                    _391 = _392;
                    _393 = _394;
                    _395 = _396;
                    _397 = _398;
                    _399 = (_1292 * _1292) * (3.0f - (_1292 * 2.0f));
                    _401 = _402;
                    _403 = _45_m0[_1283].z;
                    _405 = (_398 == 1u) ? _389 : 0u;
                    _407 = asfloat(_17.Load((_384 * 115u) + 114u).x);
                }
                if (_399 == 0.0f)
                {
                    ladder_phi_8 = false;
                    frontier_phi_8_pred = _401;
                    break;
                }
                float _448;
                if (_263)
                {
                    _448 = _262;
                }
                else
                {
                    _448 = _12.Load(int3(uint2(uint(int(_254 * float(_233))), uint(int(_255 * float(_234)))), 0u)).x;
                }
                float _450 = 1.0f - _401;
                float _451 = _450 * _450;
                float _459 = _50_m0[50u].w + _50_m0[50u].y;
                uint _462 = _397 + 63u;
                float _471 = clamp(((_50_m0[50u].x / (_459 - (_50_m0[50u].y * _448))) - _50_m0[_462].y) / (_50_m0[_462].x - _50_m0[_462].y), 0.0f, 1.0f);
                float _476 = ((_471 * _471) * _399) * (3.0f - (_471 * 2.0f));
                float _487 = ((_254 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _488 = ((1.0f - (_50_m0[51u].w * _255)) * 2.0f) + (-1.0f);
                float _504 = mad(_144, _448, mad(_137, _488, _487 * _130)) + _151;
                float _505 = (mad(_141, _448, mad(_134, _488, _487 * _127)) + _148) / _504;
                float _506 = (mad(_142, _448, mad(_135, _488, _487 * _128)) + _149) / _504;
                float _507 = (mad(_143, _448, mad(_136, _488, _487 * _129)) + _150) / _504;
                float _511 = rsqrt(dot(float3(_505, _506, _507), float3(_505, _506, _507)));
                float _512 = _511 * _505;
                float _513 = _511 * _506;
                float _514 = _511 * _507;
                float _517 = mad(_93, _395, mad(_87, _393, _391 * _81));
                float _520 = mad(_94, _395, mad(_88, _393, _391 * _82));
                float _523 = mad(_95, _395, mad(_89, _393, _391 * _83));
                float _526 = _520 * _520;
                float _659;
                float _660;
                float _661;
                if (abs(_523) > 0.0f)
                {
                    float _561 = sqrt((_523 * _523) + _526);
                    _659 = 0.0f;
                    _660 = ((-0.0f) - _523) / _561;
                    _661 = _520 / _561;
                }
                else
                {
                    float _567 = sqrt(_526 + (_517 * _517));
                    _659 = _520 / _567;
                    _660 = ((-0.0f) - _517) / _567;
                    _661 = 0.0f;
                }
                float _664 = (_661 * _520) - (_660 * _523);
                float _667 = (_659 * _523) - (_661 * _517);
                float _670 = (_660 * _517) - (_659 * _520);
                float _671 = (-0.0f) - _512;
                float _672 = (-0.0f) - _513;
                float _673 = (-0.0f) - _514;
                float _682 = mad(_673, _523, mad(_672, _520, _517 * _671));
                float _683 = mad(_673, _661, mad(_672, _660, _659 * _671)) * _451;
                float _684 = mad(_673, _670, mad(_672, _667, _664 * _671)) * _451;
                float _688 = rsqrt(dot(float3(_683, _684, _682), float3(_683, _684, _682)));
                float _689 = _688 * _683;
                float _690 = _688 * _684;
                float _691 = _688 * _682;
                float _694 = (_689 * _689) + (_690 * _690);
                bool _695 = _694 > 0.0f;
                float _731;
                float _732;
                if (_695)
                {
                    float _727 = rsqrt(_694);
                    _731 = (-0.0f) - (_690 * _727);
                    _732 = _727 * _689;
                }
                else
                {
                    _731 = 1.0f;
                    _732 = 0.0f;
                }
                float _736 = _691 + 1.0f;
                float _738 = 1.0f - (_736 * 0.5f);
                float _739 = _738 * _691;
                float _746 = sqrt(max(0.0f, 1.0f - (_738 * _738)));
                float _753 = ((_746 * _689) - (_732 * _739)) * _451;
                float _754 = ((_746 * _690) + (_731 * _739)) * _451;
                float _755 = max(0.0f, (((_732 * _689) - (_731 * _690)) * _738) + (_746 * _691));
                float _759 = rsqrt(dot(float3(_753, _754, _755), float3(_753, _754, _755)));
                float _760 = _753 * _759;
                float _761 = _754 * _759;
                float _762 = _759 * _755;
                float _765 = mad(_762, _517, mad(_761, _664, _760 * _659));
                float _768 = mad(_762, _520, mad(_761, _667, _760 * _660));
                float _771 = mad(_762, _523, mad(_761, _670, _760 * _661));
                float _775 = dot(float3(_512, _513, _514), float3(_765, _768, _771)) * 2.0f;
                float _779 = _512 - (_775 * _765);
                float _780 = _513 - (_775 * _768);
                float _781 = _514 - (_775 * _771);
                float _826;
                float _827;
                if (_695)
                {
                    float _822 = rsqrt(_694);
                    _826 = (-0.0f) - (_690 * _822);
                    _827 = _822 * _689;
                }
                else
                {
                    _826 = 1.0f;
                    _827 = 0.0f;
                }
                float _833 = _738 + (_736 * 0.15811388194561004638671875f);
                float _838 = _833 * _691;
                float _847 = sqrt(max(0.0f, 1.0f - (_833 * _833)));
                float _854 = (((_826 * (-1.3822754496572997595649212598801e-08f)) - (_827 * _838)) + (_847 * _689)) * _451;
                float _855 = (((_826 * _838) - (_827 * 1.3822754496572997595649212598801e-08f)) + (_847 * _690)) * _451;
                float _856 = max(0.0f, (((_827 * _689) - (_826 * _690)) * _833) + (_847 * _691));
                float _860 = rsqrt(dot(float3(_854, _855, _856), float3(_854, _855, _856)));
                float _861 = _854 * _860;
                float _862 = _855 * _860;
                float _863 = _860 * _856;
                float _866 = mad(_863, _517, mad(_862, _664, _861 * _659));
                float _869 = mad(_863, _520, mad(_862, _667, _861 * _660));
                float _872 = mad(_863, _523, mad(_862, _670, _861 * _661));
                float _876 = dot(float3(_512, _513, _514), float3(_866, _869, _872)) * 2.0f;
                float _880 = _512 - (_876 * _866);
                float _881 = _513 - (_876 * _869);
                float _882 = _514 - (_876 * _872);
                float _883 = dot(float3(_880, _881, _882), float3(_779, _780, _781));
                float _892 = rsqrt(dot(float3(_765, _768, _771), float3(_765, _768, _771)));
                float _893 = _892 * _765;
                float _894 = _892 * _768;
                float _895 = _892 * _771;
                float _899 = dot(float3(_512, _513, _514), float3(_893, _894, _895)) * 2.0f;
                float _903 = _512 - (_899 * _893);
                float _904 = _513 - (_899 * _894);
                float _905 = _514 - (_899 * _895);
                bool _909 = dot(float3(_903, _904, _905), float3(_512, _513, _514)) < 0.0f;
                float _916 = sqrt(((_506 * _506) + (_505 * _505)) + (_507 * _507)) * 0.001000000047497451305389404296875f;
                float _927 = ((_916 * _517) + _505) + (_903 * _403);
                float _928 = ((_916 * _520) + _506) + (_904 * _403);
                float _929 = ((_916 * _523) + _507) + (_905 * _403);
                float _945 = mad(_116, _929, mad(_109, _928, _927 * _102)) + _123;
                float _948 = (mad(_115, _929, mad(_108, _928, _927 * _101)) + _122) / _945;
                float _951 = (((mad(_113, _929, mad(_106, _928, _927 * _99)) + _120) / _945) * 0.5f) + 0.5f;
                float _952 = 0.5f - (((mad(_114, _929, mad(_107, _928, _927 * _100)) + _121) / _945) * 0.5f);
                float _955 = _951 * _50_m0[51u].x;
                float _956 = _952 * _50_m0[51u].y;
                float _961 = _927 + (_903 * 0.100000001490116119384765625f);
                float _962 = _928 + (_904 * 0.100000001490116119384765625f);
                float _963 = _929 + (_905 * 0.100000001490116119384765625f);
                float _979 = mad(_116, _963, mad(_109, _962, _961 * _102)) + _123;
                float _988 = _50_m0[51u].x * (((((mad(_113, _963, mad(_106, _962, _961 * _99)) + _120) / _979) * 0.5f) + 0.5f) - _951);
                float _990 = _50_m0[51u].y * ((0.5f - (((mad(_114, _963, mad(_107, _962, _961 * _100)) + _121) / _979) * 0.5f)) - _952);
                float _991 = ((mad(_115, _963, mad(_108, _962, _961 * _101)) + _122) / _979) - _948;
                float _992 = _988 * 10.0f;
                float _994 = _990 * 10.0f;
                float _995 = _991 * 10.0f;
                bool _1002 = _397 == 1u;
                uint _1014;
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
                uint _1036;
                uint _1038;
                float _1040;
                float _1042;
                float _1044;
                if (_1002 && (asuint(_50_m0[62u]).w != 0u))
                {
                    _1014 = 0u;
                    _1016 = 1u;
                    _1018 = 0.0f;
                    _1020 = 0.0f;
                    _1022 = 1.0f;
                    _1024 = 0.0f;
                    _1026 = 0.0f;
                    _1028 = 0.0f;
                    _1030 = 0.0f;
                    _1032 = 0.0f;
                    _1034 = 0.0f;
                    _1036 = 0u;
                    _1038 = 0u;
                    _1040 = 0.0f;
                    _1042 = 0.0f;
                    _1044 = 0.0f;
                }
                else
                {
                    float _1105 = float(_233);
                    float _1106 = float(_234);
                    float _1113 = (_992 != 0.0f) ? (0.100000001490116119384765625f / _988) : 3.4028234663852885981170418348452e+38f;
                    float _1115 = (_994 != 0.0f) ? (0.100000001490116119384765625f / _990) : 3.4028234663852885981170418348452e+38f;
                    float _1116 = (_995 != 0.0f) ? (0.100000001490116119384765625f / _991) : 3.4028234663852885981170418348452e+38f;
                    float _1117 = 1.0f / _1105;
                    float _1118 = 1.0f / _1106;
                    float _1119 = 0.004999999888241291046142578125f / _1105;
                    float _1121 = 0.004999999888241291046142578125f / _1106;
                    float _1130 = float(_992 >= 0.0f);
                    float _1131 = float(_994 >= 0.0f);
                    float _1140 = ((_992 < 0.0f) ? ((-0.0f) - _1119) : _1119) - _955;
                    float _1143 = ((_994 < 0.0f) ? ((-0.0f) - _1121) : _1121) - _956;
                    float _1146 = min((((floor(_955 * _1105) + _1130) * _1117) + _1140) * _1113, (((floor(_956 * _1106) + _1131) * _1118) + _1143) * _1115);
                    float _1150 = (_1146 * _992) + _955;
                    float _1151 = (_1146 * _994) + _956;
                    float _1152 = (_1146 * _995) + _948;
                    float _1155 = _50_m0[50u].x / (_459 - (_1152 * _50_m0[50u].y));
                    float _1166 = max(0.300000011920928955078125f, 10.0f / max(0.00999999977648258209228515625f, max(abs(_988 * 38400.0f), abs(_990 * 21600.0f))));
                    float _1041;
                    float _1043;
                    float _1045;
                    uint _1235;
                    uint _1250;
                    uint _1252;
                    uint _1017;
                    float _1019;
                    float _1021;
                    float _1023;
                    float _1025;
                    float _1027;
                    float _1029;
                    float _1031;
                    float _1033;
                    float _1035;
                    float _1240;
                    float _1242;
                    float _1244;
                    float _1246;
                    float _1248;
                    uint _1234 = 0u;
                    float _1236 = _1152;
                    float _1237 = _1151;
                    float _1238 = _1150;
                    float _1239 = _1146;
                    float _1241 = _1118;
                    float _1243 = _1117;
                    float _1245 = _1106;
                    float _1247 = _1105;
                    uint _1249 = 0u;
                    uint _1251 = 0u;
                    float _1253 = _948;
                    float _1254 = _956;
                    float _1255 = _955;
                    float _1256 = _1152;
                    float _1257 = _1151;
                    float _1258 = _1150;
                    float _1259 = 1.0f;
                    float _1260 = 0.0f;
                    float _1261 = 0.0f;
                    uint _1262 = 1u;
                    float _1263;
                    float _1264;
                    uint _1265;
                    uint _1266;
                    bool _1267;
                    for (;;)
                    {
                        _1263 = _1247 * _1238;
                        _1264 = _1245 * _1237;
                        _1265 = uint(int(_1263));
                        _1266 = uint(int(_1264));
                        _1267 = _1249 == 0u;
                        float _1438;
                        if (_1267)
                        {
                            _1438 = _12.Load(int3(uint2(_1265, _1266), 0u)).x;
                        }
                        else
                        {
                            _1438 = _15.Load(int3(uint2(_1265, _1266), _1249 + 4294967295u)).x;
                        }
                        float _1444 = ((_1263 >= floor(_1247)) || (_1264 >= floor(_1245))) ? 1.0f : _1438;
                        float _1458 = (_995 < 0.0f) ? ((_1444 - _948) * _1116) : 3.4028234663852885981170418348452e+38f;
                        float _1460 = min(min((((floor(_1263) + _1130) * _1243) + _1140) * _1113, (((floor(_1264) + _1131) * _1241) + _1143) * _1115), _1458);
                        bool _1461 = _1444 < _1236;
                        bool _1465 = _1461 && (asuint(_1460) != asuint(_1458));
                        float _1466 = _1461 ? _1460 : _1239;
                        float _1470 = (_1466 * _992) + _955;
                        float _1471 = (_1466 * _994) + _956;
                        float _1472 = (_1466 * _995) + _948;
                        uint _1474 = (_1465 ? 1u : 4294967295u) + _1249;
                        float _1475 = _1465 ? 0.5f : 2.0f;
                        float _1476 = _1475 * _1247;
                        float _1477 = _1475 * _1245;
                        float _1478 = _1465 ? 2.0f : 0.5f;
                        float _1479 = _1478 * _1243;
                        float _1480 = _1478 * _1241;
                        _1235 = _1234 + 1u;
                        uint _1784;
                        uint _1786;
                        if (int(_1474) < int(0u))
                        {
                            float _1672 = _50_m0[50u].w + _50_m0[50u].y;
                            float _1674 = _50_m0[50u].x / (_1672 - (_50_m0[50u].y * _1444));
                            float _1677 = _50_m0[50u].x / (_1672 - (_50_m0[50u].y * _1472));
                            float _1679 = abs(_1155 - _1677);
                            float _1682 = _1677 - _1674;
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
                            if (_1682 > max(0.00999999977648258209228515625f, _1679 * 0.00999999977648258209228515625f))
                            {
                                bool _1764 = _1251 != 0u;
                                frontier_phi_88_74_ladder = _1117;
                                frontier_phi_88_74_ladder_1 = _1466 + _1166;
                                frontier_phi_88_74_ladder_2 = _1118;
                                frontier_phi_88_74_ladder_3 = _1106;
                                frontier_phi_88_74_ladder_4 = _1262;
                                frontier_phi_88_74_ladder_5 = _1764 ? _1261 : _1470;
                                frontier_phi_88_74_ladder_6 = _1764 ? _1260 : _1471;
                                frontier_phi_88_74_ladder_7 = _1259;
                                frontier_phi_88_74_ladder_8 = _1258;
                                frontier_phi_88_74_ladder_9 = _1257;
                                frontier_phi_88_74_ladder_10 = _1256;
                                frontier_phi_88_74_ladder_11 = _1254;
                                frontier_phi_88_74_ladder_12 = _1253;
                                frontier_phi_88_74_ladder_13 = (_1002 || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1251;
                                frontier_phi_88_74_ladder_14 = 0u;
                                frontier_phi_88_74_ladder_15 = _1105;
                                frontier_phi_88_74_ladder_16 = _1255;
                            }
                            else
                            {
                                float _1777 = max(0.100000001490116119384765625f, _1679 * 0.100000001490116119384765625f) * 0.5f;
                                float _1780 = clamp((abs(_1682) - _1777) / _1777, 0.0f, 1.0f);
                                uint _1782 = uint(_1674 < _1155);
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
                                if (_1251 == 0u)
                                {
                                    frontier_phi_88_74_ladder_87_ladder = _1479;
                                    frontier_phi_88_74_ladder_87_ladder_1 = _1466;
                                    frontier_phi_88_74_ladder_87_ladder_2 = _1480;
                                    frontier_phi_88_74_ladder_87_ladder_3 = _1477;
                                    frontier_phi_88_74_ladder_87_ladder_4 = _1782;
                                    frontier_phi_88_74_ladder_87_ladder_5 = _1261;
                                    frontier_phi_88_74_ladder_87_ladder_6 = _1260;
                                    frontier_phi_88_74_ladder_87_ladder_7 = _1780;
                                    frontier_phi_88_74_ladder_87_ladder_8 = _1258;
                                    frontier_phi_88_74_ladder_87_ladder_9 = _1257;
                                    frontier_phi_88_74_ladder_87_ladder_10 = _1256;
                                    frontier_phi_88_74_ladder_87_ladder_11 = _1254;
                                    frontier_phi_88_74_ladder_87_ladder_12 = _1253;
                                    frontier_phi_88_74_ladder_87_ladder_13 = uint(_1780 > 0.0f);
                                    frontier_phi_88_74_ladder_87_ladder_14 = _1474;
                                    frontier_phi_88_74_ladder_87_ladder_15 = _1476;
                                    frontier_phi_88_74_ladder_87_ladder_16 = _1255;
                                }
                                else
                                {
                                    frontier_phi_88_74_ladder_87_ladder = _1479;
                                    frontier_phi_88_74_ladder_87_ladder_1 = _1466;
                                    frontier_phi_88_74_ladder_87_ladder_2 = _1480;
                                    frontier_phi_88_74_ladder_87_ladder_3 = _1477;
                                    frontier_phi_88_74_ladder_87_ladder_4 = _1782;
                                    frontier_phi_88_74_ladder_87_ladder_5 = _1261;
                                    frontier_phi_88_74_ladder_87_ladder_6 = _1260;
                                    frontier_phi_88_74_ladder_87_ladder_7 = _1780;
                                    frontier_phi_88_74_ladder_87_ladder_8 = _1258;
                                    frontier_phi_88_74_ladder_87_ladder_9 = _1257;
                                    frontier_phi_88_74_ladder_87_ladder_10 = _1256;
                                    frontier_phi_88_74_ladder_87_ladder_11 = _1254;
                                    frontier_phi_88_74_ladder_87_ladder_12 = _1253;
                                    frontier_phi_88_74_ladder_87_ladder_13 = _1251;
                                    frontier_phi_88_74_ladder_87_ladder_14 = _1474;
                                    frontier_phi_88_74_ladder_87_ladder_15 = _1476;
                                    frontier_phi_88_74_ladder_87_ladder_16 = _1255;
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
                            _1017 = frontier_phi_88_74_ladder_4;
                            _1019 = frontier_phi_88_74_ladder_5;
                            _1021 = frontier_phi_88_74_ladder_6;
                            _1023 = frontier_phi_88_74_ladder_7;
                            _1025 = frontier_phi_88_74_ladder_8;
                            _1027 = frontier_phi_88_74_ladder_9;
                            _1029 = frontier_phi_88_74_ladder_10;
                            _1031 = frontier_phi_88_74_ladder_16;
                            _1033 = frontier_phi_88_74_ladder_11;
                            _1035 = frontier_phi_88_74_ladder_12;
                            _1784 = frontier_phi_88_74_ladder_13;
                            _1786 = frontier_phi_88_74_ladder_14;
                            _1248 = frontier_phi_88_74_ladder_15;
                            _1246 = frontier_phi_88_74_ladder_3;
                            _1244 = frontier_phi_88_74_ladder;
                            _1242 = frontier_phi_88_74_ladder_2;
                            _1240 = frontier_phi_88_74_ladder_1;
                        }
                        else
                        {
                            bool _1684 = _1251 != 0u;
                            _1017 = _1262;
                            _1019 = _1261;
                            _1021 = _1260;
                            _1023 = _1259;
                            _1025 = _1684 ? _1258 : _1470;
                            _1027 = _1684 ? _1257 : _1471;
                            _1029 = _1684 ? _1256 : _1472;
                            _1031 = _1470;
                            _1033 = _1471;
                            _1035 = _1472;
                            _1784 = _1251;
                            _1786 = _1474;
                            _1248 = _1476;
                            _1246 = _1477;
                            _1244 = _1479;
                            _1242 = _1480;
                            _1240 = _1466;
                        }
                        uint frontier_phi_115_pred;
                        uint frontier_phi_115_pred_1;
                        float frontier_phi_115_pred_2;
                        float frontier_phi_115_pred_3;
                        float frontier_phi_115_pred_4;
                        bool _1789;
                        bool _1791;
                        for (;;)
                        {
                            _1789 = _1472 < 0.0f;
                            _1791 = _1789 || ((_1470 < 0.0f) || (_1471 < 0.0f));
                            if (!_1791)
                            {
                                if (!((_1472 > 1.0f) || ((_1470 > _50_m0[51u].x) || (_1471 > _50_m0[51u].y))))
                                {
                                    frontier_phi_115_pred = _1786;
                                    frontier_phi_115_pred_1 = _1784;
                                    frontier_phi_115_pred_2 = _1470;
                                    frontier_phi_115_pred_3 = _1471;
                                    frontier_phi_115_pred_4 = _1472;
                                    break;
                                }
                            }
                            if (!_1789)
                            {
                                frontier_phi_115_pred = 4294967295u;
                                frontier_phi_115_pred_1 = 1u;
                                frontier_phi_115_pred_2 = _1470;
                                frontier_phi_115_pred_3 = _1471;
                                frontier_phi_115_pred_4 = _1472;
                                break;
                            }
                            float _2075 = (-0.0f) - _1472;
                            float _2076 = _2075 / _995;
                            frontier_phi_115_pred = 4294967295u;
                            frontier_phi_115_pred_1 = 1u;
                            frontier_phi_115_pred_2 = (_2076 * _992) + _1470;
                            frontier_phi_115_pred_3 = (_2076 * _994) + _1471;
                            frontier_phi_115_pred_4 = _2075 + _1472;
                            break;
                        }
                        _1250 = frontier_phi_115_pred;
                        _1252 = frontier_phi_115_pred_1;
                        _1041 = frontier_phi_115_pred_2;
                        _1043 = frontier_phi_115_pred_3;
                        _1045 = frontier_phi_115_pred_4;
                        if ((_1235 < 128u) && (int(_1250) > int(4294967295u)))
                        {
                            _1234 = _1235;
                            _1236 = _1045;
                            _1237 = _1043;
                            _1238 = _1041;
                            _1239 = _1240;
                            _1241 = _1242;
                            _1243 = _1244;
                            _1245 = _1246;
                            _1247 = _1248;
                            _1249 = _1250;
                            _1251 = _1252;
                            _1253 = _1035;
                            _1254 = _1033;
                            _1255 = _1031;
                            _1256 = _1029;
                            _1257 = _1027;
                            _1258 = _1025;
                            _1259 = _1023;
                            _1260 = _1021;
                            _1261 = _1019;
                            _1262 = _1017;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    bool _2226 = _1235 > 127u;
                    _1014 = uint(_2226);
                    _1016 = _1017;
                    _1018 = _1019;
                    _1020 = _1021;
                    _1022 = _1023;
                    _1024 = _1025;
                    _1026 = _1027;
                    _1028 = _1029;
                    _1030 = _1031;
                    _1032 = _1033;
                    _1034 = _1035;
                    _1036 = _2226 ? 1u : _1252;
                    _1038 = uint(_1235 < 129u);
                    _1040 = _1041;
                    _1042 = _1043;
                    _1044 = _1045;
                }
                float _1052 = _50_m0[51u].z * 2.0f;
                float _1055 = (_1052 * _955) + (-1.0f);
                float _1056 = ((1.0f - (_50_m0[51u].w * _956)) * 2.0f) + (-1.0f);
                float _1072 = mad(_190, _948, mad(_183, _1056, _1055 * _176)) + _197;
                float _1073 = (mad(_187, _948, mad(_180, _1056, _1055 * _173)) + _194) / _1072;
                float _1074 = (mad(_188, _948, mad(_181, _1056, _1055 * _174)) + _195) / _1072;
                float _1075 = (mad(_189, _948, mad(_182, _1056, _1055 * _175)) + _196) / _1072;
                float _1080 = (_1052 * _1040) + (-1.0f);
                float _1081 = ((1.0f - (_50_m0[51u].w * _1042)) * 2.0f) + (-1.0f);
                float _1097 = mad(_190, _1044, mad(_183, _1081, _1080 * _176)) + _197;
                float _1101 = ((mad(_187, _1044, mad(_180, _1081, _1080 * _173)) + _194) / _1097) - _1073;
                float _1102 = ((mad(_188, _1044, mad(_181, _1081, _1080 * _174)) + _195) / _1097) - _1074;
                float _1103 = ((mad(_189, _1044, mad(_182, _1081, _1080 * _175)) + _196) / _1097) - _1075;
                float _1182;
                uint _1184;
                float _1186;
                if (_1038 == 0u)
                {
                    _1182 = 0.0f;
                    _1184 = _1036;
                    _1186 = 0.0f;
                }
                else
                {
                    float _1229 = float(_233);
                    float _1230 = float(_234);
                    float frontier_phi_44_45_ladder;
                    uint frontier_phi_44_45_ladder_1;
                    float frontier_phi_44_45_ladder_2;
                    if ((_1040 < 0.0f) || (_1042 < 0.0f))
                    {
                        frontier_phi_44_45_ladder = 0.0f;
                        frontier_phi_44_45_ladder_1 = _1036;
                        frontier_phi_44_45_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_44_45_ladder_52_ladder;
                        uint frontier_phi_44_45_ladder_52_ladder_1;
                        float frontier_phi_44_45_ladder_52_ladder_2;
                        if ((_1044 >= 1.0f) || ((_1040 > _50_m0[51u].x) || (_1042 > _50_m0[51u].y)))
                        {
                            frontier_phi_44_45_ladder_52_ladder = 0.0f;
                            frontier_phi_44_45_ladder_52_ladder_1 = _1036;
                            frontier_phi_44_45_ladder_52_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_44_45_ladder_52_ladder_63_ladder;
                            uint frontier_phi_44_45_ladder_52_ladder_63_ladder_1;
                            float frontier_phi_44_45_ladder_52_ladder_63_ladder_2;
                            for (;;)
                            {
                                if ((abs(_1040 - _254) < (2.0f / _1229)) && (abs(_1042 - _255) < (2.0f / _1230)))
                                {
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder = 0.0f;
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder_1 = _1036;
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_99;
                                    float frontier_phi_99_pred;
                                    uint frontier_phi_99_pred_1;
                                    float frontier_phi_99_pred_2;
                                    uint _1663;
                                    uint _1664;
                                    bool _1665;
                                    for (;;)
                                    {
                                        _1663 = uint(int(_1040 * _1229));
                                        _1664 = uint(int(_1042 * _1230));
                                        _1665 = _1002 && _909;
                                        if (!_1665)
                                        {
                                            if (!(dot(float3(_1101, _1102, _1103), float3(_1101, _1102, _1103)) < 0.000899999984540045261383056640625f))
                                            {
                                                ladder_phi_99 = false;
                                                frontier_phi_99_pred = 0.0f;
                                                frontier_phi_99_pred_1 = _1036;
                                                frontier_phi_99_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1754 = _24[22u].Load(int3(uint2(_1663, _1664), 0u));
                                        uint _1756 = _1754.x;
                                        float _2061;
                                        float _2062;
                                        float _2063;
                                        if (_1756 == 0u)
                                        {
                                            uint4 _1865 = _24[1u].Load(int3(uint2(_1663, _1664), 0u));
                                            uint _1867 = _1865.x;
                                            float _1875 = (float((_1867 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1876 = (float(_1867 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1880 = (1.0f - abs(_1875)) - abs(_1876);
                                            float _1882 = clamp((-0.0f) - _1880, 0.0f, 1.0f);
                                            float _1883 = (-0.0f) - _1882;
                                            _2061 = ((_1875 >= 0.0f) ? _1883 : _1882) + _1875;
                                            _2062 = ((_1876 >= 0.0f) ? _1883 : _1882) + _1876;
                                            _2063 = _1880;
                                        }
                                        else
                                        {
                                            float _1897 = (float((_1756 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1898 = (float(_1756 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1902 = (1.0f - abs(_1897)) - abs(_1898);
                                            float _1904 = clamp((-0.0f) - _1902, 0.0f, 1.0f);
                                            float _1905 = (-0.0f) - _1904;
                                            _2061 = ((_1897 >= 0.0f) ? _1905 : _1904) + _1897;
                                            _2062 = ((_1898 >= 0.0f) ? _1905 : _1904) + _1898;
                                            _2063 = _1902;
                                        }
                                        float _2067 = rsqrt(dot(float3(_2061, _2062, _2063), float3(_2061, _2062, _2063)));
                                        if (dot(float3(_2067 * _2061, _2067 * _2062, _2067 * _2063), float3(_1101, _1102, _1103)) > 0.0f)
                                        {
                                            ladder_phi_99 = true;
                                            frontier_phi_99_pred = 0.0f;
                                            frontier_phi_99_pred_1 = _1036;
                                            frontier_phi_99_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_99 = false;
                                            frontier_phi_99_pred = 0.0f;
                                            frontier_phi_99_pred_1 = _1036;
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
                                    float _1916 = _50_m0[51u].z * _1040;
                                    float _1917 = _50_m0[51u].w * _1042;
                                    float _1919 = (_1230 / _1229) * 0.0500000007450580596923828125f;
                                    float _1924 = clamp(_1916 / _1919, 0.0f, 1.0f);
                                    float _1925 = clamp(_1917 * 20.0f, 0.0f, 1.0f);
                                    float _1937 = clamp(((_1916 + (-1.0f)) + _1919) / _1919, 0.0f, 1.0f);
                                    float _1938 = clamp((_1917 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1949 = _1924 * _1925;
                                    precise float _1950 = _1949 * _1949;
                                    float _1954 = ((((3.0f - (_1925 * 2.0f)) * (3.0f - (_1924 * 2.0f))) * _1950) * (1.0f - ((_1937 * _1937) * (3.0f - (_1937 * 2.0f))))) * (1.0f - ((_1938 * _1938) * (3.0f - (_1938 * 2.0f))));
                                    bool _1957 = (_1036 != 0u) || (_1954 >= 1.0f);
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder = _1954 * float(_476 > 0.0f);
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder_1 = _1957 ? _1036 : 1u;
                                    frontier_phi_44_45_ladder_52_ladder_63_ladder_2 = _1957 ? 0.0f : _1954;
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
                    _1182 = frontier_phi_44_45_ladder_2;
                    _1184 = frontier_phi_44_45_ladder_1;
                    _1186 = frontier_phi_44_45_ladder;
                }
                uint _1342;
                uint _1344;
                float _1227;
                for (;;)
                {
                    _1227 = ((((exp2(log2(clamp((sqrt(((_1074 * _1074) + (_1073 * _1073)) + (_1075 * _1075)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _407) * exp2(log2(clamp((_1074 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_393, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    if (_1186 > 0.0f)
                    {
                        uint _1304 = uint((_202 * 0.999989986419677734375f) * clamp(_1040, 0.0f, 1.0f));
                        uint _1305 = uint((_204 * 0.999989986419677734375f) * clamp(_1042, 0.0f, 1.0f));
                        uint4 _1308 = _24[2u].Load(int3(uint2(_1304, _1305), 0u));
                        uint _1311 = _1308.w;
                        uint4 _1316 = _24[15u].Load(int3(uint2(_1304, _1305), 0u));
                        uint _1318 = _1316.y;
                        uint _1324 = ((_1318 & 64u) != 0u) ? uint((_1318 & 4294967167u) != 66u) : 4294967295u;
                        uint _1325 = _1311 & 128u;
                        uint _1327 = (_1325 != 0u) ? 1u : ((_1308.x << 7u) | _1311);
                        uint4 _1330 = _16.Load(_1327 * 4u);
                        uint _1331 = _1330.x;
                        uint _1338 = ((_1331 & 1u) != 0u) ? 0u : 18u;
                        uint _1340 = uint(min(int(uint(max(int(_1324), int(0u)))), int(1u)));
                        uint _1397;
                        if (_1325 == 0u)
                        {
                            _1397 = (((_1331 & 2097152u) != 0u) && (_1324 == _1340)) ? (_1338 | 128u) : _1338;
                        }
                        else
                        {
                            _1397 = _1311;
                        }
                        uint _1398 = _16.Load((_1327 * 4u) + 1u).x & 512u;
                        bool _1401 = (_1397 & 144u) == 0u;
                        if (_1398 == 0u)
                        {
                            if (_1401 || ((_1331 & 1u) != 0u))
                            {
                                _1342 = 0u;
                                _1344 = 0u;
                                break;
                            }
                        }
                        else
                        {
                            if (_1401)
                            {
                                _1342 = 0u;
                                _1344 = 0u;
                                break;
                            }
                        }
                        bool _1733 = ((_1397 & 128u) | _1398) != 0u;
                        uint _1343;
                        if (_1733)
                        {
                            _1343 = 1u;
                        }
                        else
                        {
                            _1343 = (((_1331 >> 14u) & 2u) ^ 2u) + 3u;
                        }
                        if (((_1331 & 268435472u) == 16u) && (((_1331 & 33554432u) == 0u) || _1733))
                        {
                            _1342 = 2u;
                            _1344 = 0u;
                            break;
                        }
                        _1342 = _1343;
                        _1344 = (_1343 == 1u) ? _1340 : 0u;
                        break;
                    }
                    else
                    {
                        _1342 = 0u;
                        _1344 = 0u;
                        break;
                    }
                }
                bool _1406;
                float _1409;
                float _1411;
                float _1413;
                float _1415;
                float _1416;
                float _1417;
                float _1419;
                float _1348;
                float _1351;
                float _1354;
                float _1358;
                float _1359;
                for (;;)
                {
                    _1348 = mad(_167, _905, mad(_161, _904, _903 * _155));
                    _1351 = mad(_168, _905, mad(_162, _904, _903 * _156));
                    _1354 = mad(_169, _905, mad(_163, _904, _903 * _157));
                    bool _1357 = (_1184 != 0u) || (_1186 < 1.0f);
                    _1358 = _1357 ? 0.0f : 1.0f;
                    _1359 = _1357 ? 0.0f : 0.5f;
                    if (_1357)
                    {
                        uint4 _1405 = asuint(_50_m0[60u]);
                        if (_1002)
                        {
                            if (int(_405) < int(1u))
                            {
                                if (_1405.x == 0u)
                                {
                                    _1406 = false;
                                    _1409 = _1030;
                                    _1411 = _1032;
                                    _1413 = _1034;
                                    _1415 = 0.0f;
                                    _1416 = 0.0f;
                                    _1417 = 0.0f;
                                    _1419 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1405.y == 0u)
                                {
                                    _1406 = false;
                                    _1409 = _1030;
                                    _1411 = _1032;
                                    _1413 = _1034;
                                    _1415 = 0.0f;
                                    _1416 = 0.0f;
                                    _1417 = 0.0f;
                                    _1419 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1405.z == 0u)
                            {
                                _1406 = false;
                                _1409 = _1030;
                                _1411 = _1032;
                                _1413 = _1034;
                                _1415 = 0.0f;
                                _1416 = 0.0f;
                                _1417 = 0.0f;
                                _1419 = 0.0f;
                                break;
                            }
                        }
                        if (_1182 > 0.0f)
                        {
                            _1406 = false;
                            _1409 = _1030;
                            _1411 = _1032;
                            _1413 = _1034;
                            _1415 = 0.0f;
                            _1416 = 1.0f;
                            _1417 = 1000.0f;
                            _1419 = 0.5f;
                            break;
                        }
                        if (((_1024 <= 0.0f) || (_1026 <= 0.0f)) || (_1028 <= 0.0f))
                        {
                            _1406 = false;
                            _1409 = _1030;
                            _1411 = _1032;
                            _1413 = _1034;
                            _1415 = 0.0f;
                            _1416 = 1.0f;
                            _1417 = 1000.0f;
                            _1419 = 0.5f;
                            break;
                        }
                        if ((_1028 >= 1.0f) || ((_1024 >= _50_m0[51u].x) || (_1026 >= _50_m0[51u].y)))
                        {
                            _1406 = false;
                            _1409 = _1030;
                            _1411 = _1032;
                            _1413 = _1034;
                            _1415 = 0.0f;
                            _1416 = 1.0f;
                            _1417 = 1000.0f;
                            _1419 = 0.5f;
                            break;
                        }
                        uint _2375;
                        uint _2377;
                        uint _2101;
                        uint _2102;
                        bool _2108;
                        for (;;)
                        {
                            _2101 = uint(clamp(_1018, 0.0f, 1.0f) * _202);
                            _2102 = uint(clamp(_1020, 0.0f, 1.0f) * _204);
                            _2108 = _20[21u].Load(int3(uint2(_2101, _2102), 0u)).x > 0.0f;
                            if (_2108)
                            {
                                uint _2293 = _24[23u].Load(int3(uint2(_2101, _2102), 0u)).y + 4294967295u;
                                _2375 = (uint(int(_2293) >> int(31u)) & 3u) + 1u;
                                _2377 = (int(_2293) < int(0u)) ? 0u : _2293;
                                break;
                            }
                            else
                            {
                                uint4 _2301 = _24[2u].Load(int3(uint2(_2101, _2102), 0u));
                                uint _2304 = _2301.w;
                                uint4 _2309 = _24[15u].Load(int3(uint2(_2101, _2102), 0u));
                                uint _2311 = _2309.y;
                                uint _2317 = ((_2311 & 64u) != 0u) ? uint((_2311 & 4294967167u) != 66u) : 4294967295u;
                                uint _2318 = _2304 & 128u;
                                uint _2320 = (_2318 != 0u) ? 1u : ((_2301.x << 7u) | _2304);
                                uint4 _2323 = _16.Load(_2320 * 4u);
                                uint _2324 = _2323.x;
                                uint _2331 = ((_2324 & 1u) != 0u) ? 0u : 18u;
                                uint _2333 = uint(min(int(uint(max(int(_2317), int(0u)))), int(1u)));
                                uint _2388;
                                if (_2318 == 0u)
                                {
                                    _2388 = (((_2324 & 2097152u) != 0u) && (_2317 == _2333)) ? (_2331 | 128u) : _2331;
                                }
                                else
                                {
                                    _2388 = _2304;
                                }
                                uint _2389 = _16.Load((_2320 * 4u) + 1u).x & 512u;
                                bool _2392 = (_2388 & 144u) == 0u;
                                if (_2389 == 0u)
                                {
                                    if (_2392 || ((_2324 & 1u) != 0u))
                                    {
                                        _2375 = 0u;
                                        _2377 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2392)
                                    {
                                        _2375 = 0u;
                                        _2377 = 0u;
                                        break;
                                    }
                                }
                                bool _2665 = ((_2388 & 128u) | _2389) != 0u;
                                uint _2376;
                                if (_2665)
                                {
                                    _2376 = 1u;
                                }
                                else
                                {
                                    _2376 = (((_2324 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_2324 & 268435472u) == 16u) && (((_2324 & 33554432u) == 0u) || _2665))
                                {
                                    _2375 = 2u;
                                    _2377 = 0u;
                                    break;
                                }
                                _2375 = _2376;
                                _2377 = (_2376 == 1u) ? _2333 : 0u;
                                break;
                            }
                        }
                        if ((_397 != _2375) || (_405 != _2377))
                        {
                            _1406 = false;
                            _1409 = _1030;
                            _1411 = _1032;
                            _1413 = _1034;
                            _1415 = 0.0f;
                            _1416 = 1.0f;
                            _1417 = 1000.0f;
                            _1419 = 0.5f;
                            break;
                        }
                        float _2516 = _1024 * 2.0f;
                        float _2519 = (_50_m0[51u].z * _2516) + (-1.0f);
                        float _2520 = ((1.0f - (_50_m0[51u].w * _1026)) * 2.0f) + (-1.0f);
                        float _2536 = mad(_190, _1028, mad(_183, _2520, _2519 * _176)) + _197;
                        float _2537 = (mad(_187, _1028, mad(_180, _2520, _2519 * _173)) + _194) / _2536;
                        float _2538 = (mad(_188, _1028, mad(_181, _2520, _2519 * _174)) + _195) / _2536;
                        float _2539 = (mad(_189, _1028, mad(_182, _2520, _2519 * _175)) + _196) / _2536;
                        if (sqrt(((_2538 * _2538) + (_2537 * _2537)) + (_2539 * _2539)) > _50_m0[58u].w)
                        {
                            _1406 = false;
                            _1409 = _1030;
                            _1411 = _1032;
                            _1413 = _1034;
                            _1415 = 0.0f;
                            _1416 = 0.0f;
                            _1417 = 1000.0f;
                            _1419 = 0.5f;
                            break;
                        }
                        float _2645 = _2537 - _1073;
                        float _2646 = _2538 - _1074;
                        float _2647 = _2539 - _1075;
                        float _2653 = sqrt(((_2646 * _2646) + (_2645 * _2645)) + (_2647 * _2647));
                        float _2661 = min(_50_m0[59u].y, max(0.0f, _2653 + (-0.001000000047497451305389404296875f)));
                        float _2681;
                        if (_1002)
                        {
                            _2681 = min(_50_m0[59u].x, _2661 + 10.0f);
                        }
                        else
                        {
                            _2681 = _50_m0[59u].x;
                        }
                        float _2682 = _2681 - _2653;
                        if (!(_2682 > 0.0f))
                        {
                            _1406 = false;
                            _1409 = _1030;
                            _1411 = _1032;
                            _1413 = _1034;
                            _1415 = 1.0f;
                            _1416 = 1.0f;
                            _1417 = 0.0f;
                            _1419 = 0.5f;
                            break;
                        }
                        float _2741 = _2537 - (_2661 * _1348);
                        float _2742 = _2538 - (_2661 * _1351);
                        float _2743 = _2539 - (_2661 * _1354);
                        RayDesc _2ident = {float3(mad(_2743, _50_m0[46u].z, mad(_2742, _50_m0[46u].y, _50_m0[46u].x * _2741)) + _50_m0[46u].w, mad(_2743, _50_m0[47u].z, mad(_2742, _50_m0[47u].y, _50_m0[47u].x * _2741)) + _50_m0[47u].w, mad(_2743, _50_m0[48u].z, mad(_2742, _50_m0[48u].y, _50_m0[48u].x * _2741)) + _50_m0[48u].w), 0.0f, float3(mad(_1354, _50_m0[46u].z, mad(_1351, _50_m0[46u].y, _50_m0[46u].x * _1348)), mad(_1354, _50_m0[47u].z, mad(_1351, _50_m0[47u].y, _50_m0[47u].x * _1348)), mad(_1354, _50_m0[48u].z, mad(_1351, _50_m0[48u].y, _50_m0[48u].x * _1348))), _2682};
                        _2746.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2793 = _2746.Proceed();
                        uint _2794 = _2746.CommittedStatus();
                        if (!(_2794 == 1u))
                        {
                            _1406 = false;
                            _1409 = _1030;
                            _1411 = _1032;
                            _1413 = _1034;
                            _1415 = 1.0f;
                            _1416 = 0.0f;
                            _1417 = 0.0f;
                            _1419 = 0.5f;
                            break;
                        }
                        float _2808 = _2746.CommittedRayT();
                        if (!((_2808 < _2682) && (_2808 > 0.0f)))
                        {
                            _1406 = false;
                            _1409 = _1030;
                            _1411 = _1032;
                            _1413 = _1034;
                            _1415 = 1.0f;
                            _1416 = 0.0f;
                            _1417 = 0.0f;
                            _1419 = 0.5f;
                            break;
                        }
                        float _2822 = (_50_m0[51u].z * _2516) + (-1.0f);
                        float _2823 = ((1.0f - (_50_m0[51u].w * _1026)) * 2.0f) + (-1.0f);
                        float _2839 = mad(_144, _1028, mad(_137, _2823, _2822 * _130)) + _151;
                        float _2843 = _2808 - _2661;
                        float _2847 = ((mad(_141, _1028, mad(_134, _2823, _2822 * _127)) + _148) / _2839) + (_2843 * _903);
                        float _2848 = ((mad(_142, _1028, mad(_135, _2823, _2822 * _128)) + _149) / _2839) + (_2843 * _904);
                        float _2849 = ((mad(_143, _1028, mad(_136, _2823, _2822 * _129)) + _150) / _2839) + (_2843 * _905);
                        float _2865 = mad(_116, _2849, mad(_109, _2848, _2847 * _102)) + _123;
                        float _1414 = (mad(_115, _2849, mad(_108, _2848, _2847 * _101)) + _122) / _2865;
                        float _1410 = ((((mad(_113, _2849, mad(_106, _2848, _2847 * _99)) + _120) / _2865) * 0.5f) + 0.5f) * _50_m0[51u].x;
                        float _1412 = (0.5f - (((mad(_114, _2849, mad(_107, _2848, _2847 * _100)) + _121) / _2865) * 0.5f)) * _50_m0[51u].y;
                        float _2874 = _1410 * _50_m0[51u].z;
                        float _2875 = _1412 * _50_m0[51u].w;
                        float _2877 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2880 = clamp(_2874 / _2877, 0.0f, 1.0f);
                        float _2881 = clamp(_2875 * 20.0f, 0.0f, 1.0f);
                        float _2891 = clamp(((_2874 + (-1.0f)) + _2877) / _2877, 0.0f, 1.0f);
                        float _2892 = clamp((_2875 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2903 = _2880 * _2881;
                        precise float _2904 = _2903 * _2903;
                        if ((((((3.0f - (_2881 * 2.0f)) * (3.0f - (_2880 * 2.0f))) * _2904) * (1.0f - ((_2891 * _2891) * (3.0f - (_2891 * 2.0f))))) * (1.0f - ((_2892 * _2892) * (3.0f - (_2892 * 2.0f))))) < 1.0f)
                        {
                            _1406 = false;
                            _1409 = _1410;
                            _1411 = _1412;
                            _1413 = _1414;
                            _1415 = _1358;
                            _1416 = _1358;
                            _1417 = 0.0f;
                            _1419 = _1359;
                            break;
                        }
                        _1406 = true;
                        _1409 = _1410;
                        _1411 = _1412;
                        _1413 = _1414;
                        _1415 = 0.0f;
                        _1416 = 1.0f;
                        _1417 = 0.0f;
                        _1419 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1406 = false;
                        _1409 = _1030;
                        _1411 = _1032;
                        _1413 = _1034;
                        _1415 = 1.0f;
                        _1416 = 1.0f;
                        _1417 = 0.0f;
                        _1419 = 0.5f;
                        break;
                    }
                }
                uint4 _1422 = asuint(_55_m0[0u]);
                float _1424 = float(_1422.x);
                float _1426 = float(_1422.y);
                float _1495;
                float _1497;
                float _1499;
                if ((_1186 >= 1.0f) || _1406)
                {
                    _1495 = _1409;
                    _1497 = _1411;
                    _1499 = _1413;
                }
                else
                {
                    float _1648 = (-0.0f) - _948;
                    float _1649 = _1648 / _995;
                    float _1652 = (_1649 * _992) + _955;
                    float _1653 = (_1649 * _994) + _956;
                    float _1654 = _1648 + _948;
                    _1495 = ((_1409 - _1652) * _1186) + _1652;
                    _1497 = ((_1411 - _1653) * _1186) + _1653;
                    _1499 = ((_1413 - _1654) * _1186) + _1654;
                }
                float _1510 = ((_1495 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _1511 = ((1.0f - (_50_m0[51u].w * _1497)) * 2.0f) + (-1.0f);
                float _1527 = mad(_144, _1499, mad(_137, _1511, _1510 * _130)) + _151;
                float _1531 = ((mad(_141, _1499, mad(_134, _1511, _1510 * _127)) + _148) / _1527) - _927;
                float _1532 = ((mad(_142, _1499, mad(_135, _1511, _1510 * _128)) + _149) / _1527) - _928;
                float _1533 = ((mad(_143, _1499, mad(_136, _1511, _1510 * _129)) + _150) / _1527) - _929;
                float _1539 = sqrt(((_1532 * _1532) + (_1531 * _1531)) + (_1533 * _1533));
                float _1540 = _1539 * _779;
                float _1541 = _1539 * _780;
                float _1542 = _1539 * _781;
                float _1543 = _1539 * (_880 / _883);
                float _1544 = _1539 * (_881 / _883);
                float _1545 = _1539 * (_882 / _883);
                float _1549 = dot(float3(_1540, _1541, _1542), float3(_765, _768, _771)) * 2.0f;
                float _1559 = dot(float3(_1543, _1544, _1545), float3(_765, _768, _771)) * 2.0f;
                float _1586 = (_1540 - (_1549 * _765)) + _927;
                float _1587 = (_1541 - (_1549 * _768)) + _928;
                float _1588 = (_1542 - (_1549 * _771)) + _929;
                float _1600 = mad(_50_m0[24u].w, _1588, mad(_50_m0[23u].w, _1587, _1586 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1605 = (_1543 - (_1559 * _765)) + _927;
                float _1606 = (_1544 - (_1559 * _768)) + _928;
                float _1607 = (_1545 - (_1559 * _771)) + _929;
                float _1619 = mad(_50_m0[24u].w, _1607, mad(_50_m0[23u].w, _1606, _1605 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1625 = (_50_m0[51u].x * ((((mad(_50_m0[24u].x, _1588, mad(_50_m0[23u].x, _1587, _1586 * _50_m0[22u].x)) + _50_m0[25u].x) / _1600) - ((mad(_50_m0[24u].x, _1607, mad(_50_m0[23u].x, _1606, _1605 * _50_m0[22u].x)) + _50_m0[25u].x) / _1619)) * 0.5f)) * _1424;
                float _1629 = (_50_m0[51u].y * ((((mad(_50_m0[24u].y, _1607, mad(_50_m0[23u].y, _1606, _1605 * _50_m0[22u].y)) + _50_m0[25u].y) / _1619) - ((mad(_50_m0[24u].y, _1588, mad(_50_m0[23u].y, _1587, _1586 * _50_m0[22u].y)) + _50_m0[25u].y) / _1600)) * 0.5f)) * _1426;
                uint4 _1639 = asuint(_50_m0[55u]);
                float _1644 = clamp(log2(sqrt((_1629 * _1629) + (_1625 * _1625)) * 2.0f) / float(_1639.x + 4294967295u), 0.0f, 1.0f);
                uint _1742;
                float _1743;
                float _1744;
                float _1746;
                float _1748;
                if (_1002 && (_1639.z != 0u))
                {
                    uint frontier_phi_83_82_ladder;
                    float frontier_phi_83_82_ladder_1;
                    float frontier_phi_83_82_ladder_2;
                    float frontier_phi_83_82_ladder_3;
                    float frontier_phi_83_82_ladder_4;
                    if ((_1342 != 1u) || (_1344 != 0u))
                    {
                        float _1846 = _948 / max(9.9999999747524270787835121154785e-07f, (-0.0f) - _995);
                        float _1851 = ((_1846 * _992) + _955) / _50_m0[51u].x;
                        float _1852 = ((_1846 * _994) + _956) / _50_m0[51u].y;
                        float _2000;
                        if (_1852 < 0.300000011920928955078125f)
                        {
                            float _1992 = (0.300000011920928955078125f - _1852) * 3.3333332538604736328125f;
                            _2000 = 0.300000011920928955078125f - ((_1992 / sqrt((_1992 * _1992) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _2000 = _1852;
                        }
                        float _2117;
                        if (_1851 < 0.300000011920928955078125f)
                        {
                            float _2110 = (0.300000011920928955078125f - _1851) * 3.3333332538604736328125f;
                            _2117 = 0.300000011920928955078125f - ((_2110 / sqrt((_2110 * _2110) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _2117 = _1851;
                        }
                        float _2345;
                        if ((1.0f - _2117) < 0.300000011920928955078125f)
                        {
                            float _2337 = (_2117 + (-0.699999988079071044921875f)) * 3.3333332538604736328125f;
                            _2345 = ((_2337 / sqrt((_2337 * _2337) + 1.0f)) * 0.300000011920928955078125f) + 0.699999988079071044921875f;
                        }
                        else
                        {
                            _2345 = _2117;
                        }
                        frontier_phi_83_82_ladder = 0u;
                        frontier_phi_83_82_ladder_1 = 0.0f;
                        frontier_phi_83_82_ladder_2 = _2345 * _50_m0[51u].x;
                        frontier_phi_83_82_ladder_3 = 1.0f - clamp((_905 + (-0.25f)) * (-4.0f), 0.0f, 1.0f);
                        frontier_phi_83_82_ladder_4 = _2000 * _50_m0[51u].y;
                    }
                    else
                    {
                        frontier_phi_83_82_ladder = _1014;
                        frontier_phi_83_82_ladder_1 = _1022;
                        frontier_phi_83_82_ladder_2 = _1040;
                        frontier_phi_83_82_ladder_3 = _1186;
                        frontier_phi_83_82_ladder_4 = _1042;
                    }
                    _1742 = frontier_phi_83_82_ladder;
                    _1743 = frontier_phi_83_82_ladder_1;
                    _1744 = frontier_phi_83_82_ladder_2;
                    _1746 = frontier_phi_83_82_ladder_4;
                    _1748 = frontier_phi_83_82_ladder_3;
                }
                else
                {
                    _1742 = _1014;
                    _1743 = _1022;
                    _1744 = _1040;
                    _1746 = _1042;
                    _1748 = _1186;
                }
                bool _1750 = _397 != 1u;
                float _2120;
                float _2122;
                float _2124;
                float _2126;
                if (_1748 == 0.0f)
                {
                    float _2002;
                    float _2004;
                    float _2006;
                    float _2008;
                    if (_1406)
                    {
                        _2002 = 0.0f;
                        _2004 = 0.0f;
                        _2006 = 0.0f;
                        _2008 = 1.0f;
                    }
                    else
                    {
                        float frontier_phi_109_110_ladder;
                        float frontier_phi_109_110_ladder_1;
                        float frontier_phi_109_110_ladder_2;
                        float frontier_phi_109_110_ladder_3;
                        if (_1750)
                        {
                            float _2177 = _1351 * _1227;
                            float _2181 = rsqrt(dot(float3(_1348, _2177, _1354), float3(_1348, _2177, _1354)));
                            float4 _2191 = _28[4u].SampleLevel(_59, float3(_2181 * _1348, _2181 * _2177, _2181 * _1354), 0.0f);
                            frontier_phi_109_110_ladder = 1.0f;
                            frontier_phi_109_110_ladder_1 = _2191.z;
                            frontier_phi_109_110_ladder_2 = _2191.y;
                            frontier_phi_109_110_ladder_3 = _2191.x;
                        }
                        else
                        {
                            frontier_phi_109_110_ladder = 0.0f;
                            frontier_phi_109_110_ladder_1 = 0.0f;
                            frontier_phi_109_110_ladder_2 = 0.0f;
                            frontier_phi_109_110_ladder_3 = 0.0f;
                        }
                        _2002 = frontier_phi_109_110_ladder_3;
                        _2004 = frontier_phi_109_110_ladder_2;
                        _2006 = frontier_phi_109_110_ladder_1;
                        _2008 = frontier_phi_109_110_ladder;
                    }
                    _2120 = _2002 * _476;
                    _2122 = _2004 * _476;
                    _2124 = _2006 * _476;
                    _2126 = _2008 * _476;
                }
                else
                {
                    float _2217;
                    float _2219;
                    float _2222;
                    float _2224;
                    if (_12.Load(int3(uint2(uint(_1744 * _1424), uint(_1746 * _1426)), 0u)).x > 0.0f)
                    {
                        uint _2014_dummy_parameter;
                        uint2 _2014 = spvTextureSize(_14, 0u, _2014_dummy_parameter);
                        float4 _2023 = _14.Load(int3(uint2(uint(float(_2014.x) * _1744), uint(float(_2014.y) * _1746)), 0u));
                        float _2027 = _2023.x * 0.5f;
                        float _2028 = _2023.y * (-0.5f);
                        float4 _2047 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _2027) + (_50_m0[52u].x * _1744), (_50_m0[52u].w * _2028) + (_50_m0[52u].y * _1746)), 0.0f);
                        float _2057 = _50_m0[54u].x * _2047.x;
                        float _2058 = _50_m0[54u].x * _2047.y;
                        float _2059 = _50_m0[54u].x * _2047.z;
                        float _2204;
                        if (_1002)
                        {
                            float frontier_phi_124_123_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _948))) < 5.0f)
                            {
                                float _2361 = sqrt((_2027 * _2027) + (_2028 * _2028));
                                float frontier_phi_124_123_ladder_137_ladder;
                                if (_2361 > 0.0500000007450580596923828125f)
                                {
                                    float _2400 = _1744 - _955;
                                    float _2401 = _1746 - _956;
                                    float frontier_phi_124_123_ladder_137_ladder_148_ladder;
                                    if (_2361 > sqrt((_2401 * _2401) + (_2400 * _2400)))
                                    {
                                        uint4 _2563 = asuint(_55_m0[0u]);
                                        uint _2570 = uint(float(_2563.x) * _1744);
                                        uint _2571 = uint(float(_2563.y) * _1746);
                                        uint4 _2574 = _24[2u].Load(int3(uint2(_2570, _2571), 0u));
                                        uint _2577 = _2574.w;
                                        uint4 _2582 = _24[15u].Load(int3(uint2(_2570, _2571), 0u));
                                        uint _2584 = _2582.y;
                                        uint _2590 = ((_2584 & 64u) != 0u) ? uint((_2584 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2591 = _2577 & 128u;
                                        uint _2593 = (_2591 != 0u) ? 1u : ((_2574.x << 7u) | _2577);
                                        uint4 _2596 = _16.Load(_2593 * 4u);
                                        uint _2597 = _2596.x;
                                        uint _2604 = ((_2597 & 1u) != 0u) ? 0u : 18u;
                                        uint _2674;
                                        if (_2591 == 0u)
                                        {
                                            _2674 = (((_2597 & 2097152u) != 0u) && (_2590 == uint(min(int(uint(max(int(_2590), int(0u)))), int(1u))))) ? (_2604 | 128u) : _2604;
                                        }
                                        else
                                        {
                                            _2674 = _2577;
                                        }
                                        float frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder;
                                        if (((_2674 & 128u) | (_16.Load((_2593 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2696 = asuint(_55_m0[0u]);
                                            uint _2705 = uint(float(_2696.x) * (_2027 + _1744));
                                            uint _2706 = uint(float(_2696.y) * (_2028 + _1746));
                                            uint4 _2709 = _24[2u].Load(int3(uint2(_2705, _2706), 0u));
                                            uint _2712 = _2709.w;
                                            uint4 _2715 = _24[15u].Load(int3(uint2(_2705, _2706), 0u));
                                            uint _2717 = _2715.y;
                                            uint _2723 = ((_2717 & 64u) != 0u) ? uint((_2717 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2724 = _2712 & 128u;
                                            uint _2726 = (_2724 != 0u) ? 1u : ((_2709.x << 7u) | _2712);
                                            uint4 _2728 = _16.Load(_2726 * 4u);
                                            uint _2729 = _2728.x;
                                            uint _2736 = ((_2729 & 1u) != 0u) ? 0u : 18u;
                                            uint _2805;
                                            if (_2724 == 0u)
                                            {
                                                _2805 = (((_2729 & 2097152u) != 0u) && (_2723 == uint(min(int(uint(max(int(_2723), int(0u)))), int(1u))))) ? (_2736 | 128u) : _2736;
                                            }
                                            else
                                            {
                                                _2805 = _2712;
                                            }
                                            float frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder_174_ladder;
                                            if ((_2805 & 128u) == 0u)
                                            {
                                                frontier_phi_124_123_ladder_137_ladder_148_ladder_165_ladder_174_ladder = ((_16.Load((_2726 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
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
                            _2204 = frontier_phi_124_123_ladder;
                        }
                        else
                        {
                            _2204 = 1.0f;
                        }
                        float frontier_phi_126_124_ladder;
                        float frontier_phi_126_124_ladder_1;
                        float frontier_phi_126_124_ladder_2;
                        float frontier_phi_126_124_ladder_3;
                        float _2206;
                        for (;;)
                        {
                            _2206 = _2204 * _1748;
                            if (_1743 > 0.0f)
                            {
                                float _2221;
                                float _2223;
                                float _2225;
                                if (_1002)
                                {
                                    _2221 = 0.0f;
                                    _2223 = 0.0f;
                                    _2225 = 0.0f;
                                }
                                else
                                {
                                    if (!((_397 != 4u) || (_1016 != 0u)))
                                    {
                                        frontier_phi_126_124_ladder = _2059;
                                        frontier_phi_126_124_ladder_1 = _2058;
                                        frontier_phi_126_124_ladder_2 = _2057;
                                        frontier_phi_126_124_ladder_3 = _2206;
                                        break;
                                    }
                                    _2221 = _2057;
                                    _2223 = _2058;
                                    _2225 = _2059;
                                }
                                frontier_phi_126_124_ladder = _2225;
                                frontier_phi_126_124_ladder_1 = _2223;
                                frontier_phi_126_124_ladder_2 = _2221;
                                frontier_phi_126_124_ladder_3 = (1.0f - exp2(log2(_1743) * 3.0f)) * _2206;
                                break;
                            }
                            else
                            {
                                frontier_phi_126_124_ladder = _2059;
                                frontier_phi_126_124_ladder_1 = _2058;
                                frontier_phi_126_124_ladder_2 = _2057;
                                frontier_phi_126_124_ladder_3 = _2206;
                                break;
                            }
                        }
                        _2217 = frontier_phi_126_124_ladder_3;
                        _2219 = frontier_phi_126_124_ladder_2;
                        _2222 = frontier_phi_126_124_ladder_1;
                        _2224 = frontier_phi_126_124_ladder;
                    }
                    else
                    {
                        float frontier_phi_126_112_ladder;
                        float frontier_phi_126_112_ladder_1;
                        float frontier_phi_126_112_ladder_2;
                        float frontier_phi_126_112_ladder_3;
                        if (_1742 == 0u)
                        {
                            float4 _2212 = _28[7u].SampleLevel(_59, float3(_1348, _1351, _1354), 0.0f);
                            frontier_phi_126_112_ladder = _2212.z;
                            frontier_phi_126_112_ladder_1 = _2212.y;
                            frontier_phi_126_112_ladder_2 = _2212.x;
                            frontier_phi_126_112_ladder_3 = _1748;
                        }
                        else
                        {
                            frontier_phi_126_112_ladder = _2220;
                            frontier_phi_126_112_ladder_1 = _2220;
                            frontier_phi_126_112_ladder_2 = _2220;
                            frontier_phi_126_112_ladder_3 = 0.0f;
                        }
                        _2217 = frontier_phi_126_112_ladder_3;
                        _2219 = frontier_phi_126_112_ladder_2;
                        _2222 = frontier_phi_126_112_ladder_1;
                        _2224 = frontier_phi_126_112_ladder;
                    }
                    float _2414;
                    float _2415;
                    float _2417;
                    float _2419;
                    if (_1406)
                    {
                        _2414 = 1.0f;
                        _2415 = _2219 * _1182;
                        _2417 = _2222 * _1182;
                        _2419 = _2224 * _1182;
                    }
                    else
                    {
                        float frontier_phi_151_140_ladder;
                        float frontier_phi_151_140_ladder_1;
                        float frontier_phi_151_140_ladder_2;
                        float frontier_phi_151_140_ladder_3;
                        if (_1750 && (_2217 < 1.0f))
                        {
                            float _2421 = _1351 * _1227;
                            float _2425 = rsqrt(dot(float3(_1348, _2421, _1354), float3(_1348, _2421, _1354)));
                            float4 _2433 = _28[4u].SampleLevel(_59, float3(_2425 * _1348, _2425 * _2421, _2425 * _1354), 0.0f);
                            float _2435 = _2433.x;
                            float _2436 = _2433.y;
                            float _2437 = _2433.z;
                            frontier_phi_151_140_ladder = ((_2224 - _2437) * _2217) + _2437;
                            frontier_phi_151_140_ladder_1 = 1.0f;
                            frontier_phi_151_140_ladder_2 = ((_2219 - _2435) * _2217) + _2435;
                            frontier_phi_151_140_ladder_3 = ((_2222 - _2436) * _2217) + _2436;
                        }
                        else
                        {
                            frontier_phi_151_140_ladder = _2224;
                            frontier_phi_151_140_ladder_1 = _2217;
                            frontier_phi_151_140_ladder_2 = _2219;
                            frontier_phi_151_140_ladder_3 = _2222;
                        }
                        _2414 = frontier_phi_151_140_ladder_1;
                        _2415 = frontier_phi_151_140_ladder_2;
                        _2417 = frontier_phi_151_140_ladder_3;
                        _2419 = frontier_phi_151_140_ladder;
                    }
                    float _2127 = _2414 * _476;
                    _2120 = _2415 * _2127;
                    _2122 = _2417 * _2127;
                    _2124 = _2419 * _2127;
                    _2126 = _2127;
                }
                float _2131 = _50_m0[58u].z * _1419;
                float _2153 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _2155 = _2153 * ((_2131 * ((_1415 * 1000.0f) - _2120)) + _2120);
                float _2156 = _2153 * ((_2131 * ((_1416 * 1000.0f) - _2122)) + _2122);
                float _2157 = _2153 * ((_2131 * (_1417 - _2124)) + _2124);
                float _2163 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2155, max(_2156, _2157)) + 1.0f);
                float _2167 = min(_2163 * _2155, 0.996078431606292724609375f);
                float _2169 = min(_2163 * _2156, 0.996078431606292724609375f);
                float _2170 = min(_2163 * _2157, 0.996078431606292724609375f);
                _35[uint2(_221, _224)] = float4(_2167, _2169, _2170, _2126);
                _39[uint2(_221, _224)] = float4(_1644, _401, _2126, _1644);
                if (_228)
                {
                    uint _2351 = _221 + 1u;
                    _35[uint2(_2351, _224)] = float4(_2167, _2169, _2170, _2126);
                    _39[uint2(_2351, _224)] = float4(_1644, _401, _2126, _1644);
                }
                if (_231)
                {
                    uint _2393 = _224 + 1u;
                    _35[uint2(_221, _2393)] = float4(_2167, _2169, _2170, _2126);
                    _39[uint2(_221, _2393)] = float4(_1644, _401, _2126, _1644);
                }
                if (_232)
                {
                    uint _2553 = _221 + 1u;
                    uint _2554 = _224 + 1u;
                    _35[uint2(_2553, _2554)] = float4(_2167, _2169, _2170, _2126);
                    _39[uint2(_2553, _2554)] = float4(_1644, _401, _2126, _1644);
                }
                ladder_phi_8 = true;
                frontier_phi_8_pred = _401;
                break;
            }
            float _421 = frontier_phi_8_pred;
            if (ladder_phi_8)
            {
                break;
            }
            _35[uint2(_221, _224)] = 0.0f.xxxx;
            _39[uint2(_221, _224)] = float4(0.0f, _421, 0.0f, 0.0f);
            if (_228)
            {
                uint _441 = _221 + 1u;
                _35[uint2(_441, _224)] = 0.0f.xxxx;
                _39[uint2(_441, _224)] = float4(0.0f, _421, 0.0f, 0.0f);
            }
            if (_231)
            {
                uint _552 = _224 + 1u;
                _35[uint2(_221, _552)] = 0.0f.xxxx;
                _39[uint2(_221, _552)] = float4(0.0f, _421, 0.0f, 0.0f);
            }
            if (!_232)
            {
                break;
            }
            uint _651 = _221 + 1u;
            uint _652 = _224 + 1u;
            _35[uint2(_651, _652)] = 0.0f.xxxx;
            _39[uint2(_651, _652)] = float4(0.0f, _421, 0.0f, 0.0f);
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
