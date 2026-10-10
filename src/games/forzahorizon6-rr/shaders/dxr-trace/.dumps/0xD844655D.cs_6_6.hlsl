static float _2292;
static uint _3188;
static float _3189;
static float _3190;
static uint _3191;
static float _3192;
static float _3193;
static float _3194;
static float _3195;
static float _3196;
static float _3197;
static float _3198;
static uint _3199;
static uint _3200;
static float _3201;
static float _3202;
static float _3203;
static float _3204;
static float _3205;
static uint _3206;
static float _3212;
static uint _3213;
static float _3214;
static float _3219;
static float _3220;
static float _3221;

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

static RayQuery<RAY_FLAG_NONE> _2702;

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
                        float _1107;
                        if (_792)
                        {
                            _1107 = _788;
                        }
                        else
                        {
                            _1107 = float(_589 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1191;
                        if ((_428 & 134217728u) == 0u)
                        {
                            uint frontier_phi_45_39_ladder;
                            if ((_590 != 0u) || ((_428 & 17825792u) == 1048576u))
                            {
                                frontier_phi_45_39_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_45_39_ladder = _782;
                            }
                            _1191 = frontier_phi_45_39_ladder;
                        }
                        else
                        {
                            _1191 = _782;
                        }
                        uint4 _1194 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _1196 = _1194.x;
                        float _1264;
                        float _1266;
                        float _1268;
                        if (_781 == 0u)
                        {
                            _1264 = 0.0f;
                            _1266 = 0.0f;
                            _1268 = 0.0f;
                        }
                        else
                        {
                            float4 _1273 = _20[8u].Load(int3(uint2(_265, _266), 0u));
                            _1264 = _1273.x;
                            _1266 = _1273.y;
                            _1268 = _1273.z;
                        }
                        uint _1301;
                        if (_1191 == 0u)
                        {
                            _1301 = 0u;
                        }
                        else
                        {
                            _1301 = _24[9u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        uint _1332;
                        if (_783 == 0u)
                        {
                            _1332 = 0u;
                        }
                        else
                        {
                            _1332 = _24[10u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        float _1342 = (float((_1196 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1343 = (float(_1196 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1347 = (1.0f - abs(_1342)) - abs(_1343);
                        float _1349 = clamp((-0.0f) - _1347, 0.0f, 1.0f);
                        float _1350 = (-0.0f) - _1349;
                        float _1355 = ((_1342 >= 0.0f) ? _1350 : _1349) + _1342;
                        float _1356 = ((_1343 >= 0.0f) ? _1350 : _1349) + _1343;
                        float _1360 = rsqrt(dot(float3(_1355, _1356, _1347), float3(_1355, _1356, _1347)));
                        float _1361 = _1355 * _1360;
                        float _1362 = _1356 * _1360;
                        float _1363 = _1360 * _1347;
                        float _794 = float(_1196 & 255u);
                        float _1367 = ((_428 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1450;
                        float _1451;
                        float _1452;
                        float _1453;
                        uint _1454;
                        if ((_380 & 64u) == 0u)
                        {
                            float frontier_phi_74_67_ladder;
                            float frontier_phi_74_67_ladder_1;
                            float frontier_phi_74_67_ladder_2;
                            float frontier_phi_74_67_ladder_3;
                            uint frontier_phi_74_67_ladder_4;
                            if ((_428 & 276824064u) == 0u)
                            {
                                frontier_phi_74_67_ladder = 0.0f;
                                frontier_phi_74_67_ladder_1 = ((_428 & 8u) != 0u) ? _1266 : _1367;
                                frontier_phi_74_67_ladder_2 = 0.0f;
                                frontier_phi_74_67_ladder_3 = 0.0f;
                                frontier_phi_74_67_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_74_67_ladder = 0.0f;
                                frontier_phi_74_67_ladder_1 = _1367;
                                frontier_phi_74_67_ladder_2 = 0.0f;
                                frontier_phi_74_67_ladder_3 = 0.0f;
                                frontier_phi_74_67_ladder_4 = 0u;
                            }
                            _1450 = frontier_phi_74_67_ladder_1;
                            _1451 = frontier_phi_74_67_ladder_2;
                            _1452 = frontier_phi_74_67_ladder;
                            _1453 = frontier_phi_74_67_ladder_3;
                            _1454 = frontier_phi_74_67_ladder_4;
                        }
                        else
                        {
                            float _1402 = (_1266 * 2.0f) + (-1.0f);
                            float _1403 = (_1268 * 2.0f) + (-1.0f);
                            float _1407 = (1.0f - abs(_1402)) - abs(_1403);
                            float _1409 = clamp((-0.0f) - _1407, 0.0f, 1.0f);
                            float _1410 = (-0.0f) - _1409;
                            float _1415 = ((_1402 >= 0.0f) ? _1410 : _1409) + _1402;
                            float _1416 = ((_1403 >= 0.0f) ? _1410 : _1409) + _1403;
                            float _1420 = rsqrt(dot(float3(_1415, _1416, _1407), float3(_1415, _1416, _1407)));
                            _1450 = floor(round(_1264 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1451 = _1415 * _1420;
                            _1452 = _1416 * _1420;
                            _1453 = _1420 * _1407;
                            _1454 = 1u;
                        }
                        float _802;
                        if ((_428 & 32768u) == 0u)
                        {
                            _802 = _1450;
                        }
                        else
                        {
                            float frontier_phi_76_77_ladder;
                            if (_17.Load((_384 * 115u) + 36u).x == 0u)
                            {
                                float _1655 = clamp((_794 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _579;
                                frontier_phi_76_77_ladder = ((_428 & 131072u) != 0u) ? _1655 : ((((clamp((1.21000003814697265625f / (exp2((_1107 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_384 * 115u) + 32u).x)) + 1.0f) * _1655);
                            }
                            else
                            {
                                frontier_phi_76_77_ladder = _579;
                            }
                            _802 = frontier_phi_76_77_ladder;
                        }
                        uint _1524 = _427 & 1u;
                        float _1598;
                        float _1600;
                        float _1602;
                        uint _1604;
                        if (((_428 & 16u) == 0u) || (((_1524 | (_380 & 8u)) | (_428 & 16777216u)) != 0u))
                        {
                            _1598 = _1451;
                            _1600 = _1452;
                            _1602 = _1453;
                            _1604 = _1454;
                        }
                        else
                        {
                            float _1614 = (float(_1301 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1615 = (float(_1301 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1619 = (1.0f - abs(_1614)) - abs(_1615);
                            float _1621 = clamp((-0.0f) - _1619, 0.0f, 1.0f);
                            float _1622 = (-0.0f) - _1621;
                            float _1627 = ((_1614 >= 0.0f) ? _1622 : _1621) + _1614;
                            float _1628 = ((_1615 >= 0.0f) ? _1622 : _1621) + _1615;
                            float _1632 = rsqrt(dot(float3(_1627, _1628, _1619), float3(_1627, _1628, _1619)));
                            _1598 = _1627 * _1632;
                            _1600 = _1628 * _1632;
                            _1602 = _1632 * _1619;
                            _1604 = 1u;
                        }
                        float _796;
                        float _798;
                        float _800;
                        if (_1524 == 0u)
                        {
                            float frontier_phi_87_86_ladder;
                            float frontier_phi_87_86_ladder_1;
                            float frontier_phi_87_86_ladder_2;
                            if (((_427 & 64u) == 0u) && (_784 != 0u))
                            {
                                float2 _1892 = spvUnpackHalf2x16((_1332 >> 17u) & 32736u);
                                float _1893 = _1892.x;
                                float _1896 = (spvUnpackHalf2x16((_1332 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1897 = (spvUnpackHalf2x16((_1332 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1901 = (1.0f - abs(_1896)) - abs(_1897);
                                float _1903 = clamp((-0.0f) - _1901, 0.0f, 1.0f);
                                float _1904 = (-0.0f) - _1903;
                                float _1909 = ((_1896 >= 0.0f) ? _1904 : _1903) + _1896;
                                float _1910 = ((_1897 >= 0.0f) ? _1904 : _1903) + _1897;
                                float _1914 = rsqrt(dot(float3(_1909, _1910, _1901), float3(_1909, _1910, _1901)));
                                float _1924 = (((_1909 * _1914) - _1361) * _1893) + _1361;
                                float _1925 = (((_1910 * _1914) - _1362) * _1893) + _1362;
                                float _1926 = (((_1914 * _1901) - _1363) * _1893) + _1363;
                                float _1930 = rsqrt(dot(float3(_1924, _1925, _1926), float3(_1924, _1925, _1926)));
                                frontier_phi_87_86_ladder = _1924 * _1930;
                                frontier_phi_87_86_ladder_1 = _1925 * _1930;
                                frontier_phi_87_86_ladder_2 = _1926 * _1930;
                            }
                            else
                            {
                                frontier_phi_87_86_ladder = _1361;
                                frontier_phi_87_86_ladder_1 = _1362;
                                frontier_phi_87_86_ladder_2 = _1363;
                            }
                            _796 = frontier_phi_87_86_ladder;
                            _798 = frontier_phi_87_86_ladder_1;
                            _800 = frontier_phi_87_86_ladder_2;
                        }
                        else
                        {
                            _796 = _1361;
                            _798 = _1362;
                            _800 = _1363;
                        }
                        float _810;
                        float _812;
                        float _814;
                        float _816;
                        if (_792)
                        {
                            float frontier_phi_95_94_ladder;
                            float frontier_phi_95_94_ladder_1;
                            float frontier_phi_95_94_ladder_2;
                            float frontier_phi_95_94_ladder_3;
                            if (((_428 & 33554432u) == 0u) || (((_380 & 4u) != 0u) && ((_428 & 8388608u) == 0u)))
                            {
                                frontier_phi_95_94_ladder = 0.0f;
                                frontier_phi_95_94_ladder_1 = 0.0f;
                                frontier_phi_95_94_ladder_2 = 0.0f;
                                frontier_phi_95_94_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _1974 = (spvUnpackHalf2x16((_1332 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1975 = (spvUnpackHalf2x16((_1332 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1979 = (1.0f - abs(_1974)) - abs(_1975);
                                float _1981 = clamp((-0.0f) - _1979, 0.0f, 1.0f);
                                float _1982 = (-0.0f) - _1981;
                                float _1987 = ((_1974 >= 0.0f) ? _1982 : _1981) + _1974;
                                float _1988 = ((_1975 >= 0.0f) ? _1982 : _1981) + _1975;
                                float _1992 = rsqrt(dot(float3(_1987, _1988, _1979), float3(_1987, _1988, _1979)));
                                float _1993 = _1987 * _1992;
                                float _1994 = _1988 * _1992;
                                float _1995 = _1992 * _1979;
                                float _1999 = rsqrt(dot(float3(_1993, _1994, _1995), float3(_1993, _1994, _1995)));
                                frontier_phi_95_94_ladder = _1999 * _1995;
                                frontier_phi_95_94_ladder_1 = _1999 * _1994;
                                frontier_phi_95_94_ladder_2 = _1999 * _1993;
                                frontier_phi_95_94_ladder_3 = spvUnpackHalf2x16((_1332 >> 17u) & 32736u).x;
                            }
                            _810 = frontier_phi_95_94_ladder_3;
                            _812 = frontier_phi_95_94_ladder_2;
                            _814 = frontier_phi_95_94_ladder_1;
                            _816 = frontier_phi_95_94_ladder;
                        }
                        else
                        {
                            _810 = 0.0f;
                            _812 = 0.0f;
                            _814 = 0.0f;
                            _816 = 0.0f;
                        }
                        bool _1944 = _1604 != 0u;
                        _793 = _794;
                        _795 = _796;
                        _797 = _798;
                        _799 = _800;
                        _801 = _802;
                        _803 = _1944 ? _1598 : _796;
                        _805 = _1944 ? _1600 : _798;
                        _807 = _1944 ? _1602 : _800;
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
                    bool _1116 = ((_427 & 128u) | _437) != 0u;
                    uint _1174;
                    if (_1116)
                    {
                        _1174 = 1u;
                    }
                    else
                    {
                        _1174 = (((_428 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _392;
                    float _394;
                    float _396;
                    uint _398;
                    float _402;
                    if (((_428 & 33554432u) == 0u) || _1116)
                    {
                        bool _1198 = _77 != 0u;
                        uint _1205;
                        if ((_428 & 16u) == 0u)
                        {
                            _1205 = _1174;
                        }
                        else
                        {
                            _1205 = ((_428 & 268435456u) != 0u) ? _1174 : 2u;
                        }
                        _402 = _801 * _817;
                        _392 = _1198 ? _803 : _795;
                        _394 = _1198 ? _805 : _797;
                        _396 = _1198 ? _807 : _799;
                        _398 = _1205;
                    }
                    else
                    {
                        _402 = _809;
                        _392 = _811;
                        _394 = _813;
                        _396 = _815;
                        _398 = _1174;
                    }
                    uint _1206 = _398 + 102u;
                    float _1215 = clamp((_402 - _45_m0[_1206].x) / (_45_m0[_1206].y - _45_m0[_1206].x), 0.0f, 1.0f);
                    _391 = _392;
                    _393 = _394;
                    _395 = _396;
                    _397 = _398;
                    _399 = (_1215 * _1215) * (3.0f - (_1215 * 2.0f));
                    _401 = _402;
                    _403 = _45_m0[_1206].z;
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
                float _889 = rsqrt(dot(float3(_764, _767, _770), float3(_764, _767, _770)));
                float _890 = _889 * _764;
                float _891 = _889 * _767;
                float _892 = _889 * _770;
                float _896 = dot(float3(_511, _512, _513), float3(_890, _891, _892)) * 2.0f;
                float _900 = _511 - (_896 * _890);
                float _901 = _512 - (_896 * _891);
                float _902 = _513 - (_896 * _892);
                float _912 = sqrt(((_505 * _505) + (_504 * _504)) + (_506 * _506)) * 0.001000000047497451305389404296875f;
                float _923 = ((_912 * _516) + _504) + (_900 * _403);
                float _924 = ((_912 * _519) + _505) + (_901 * _403);
                float _925 = ((_912 * _522) + _506) + (_902 * _403);
                float _941 = mad(_116, _925, mad(_109, _924, _923 * _102)) + _123;
                float _944 = (mad(_115, _925, mad(_108, _924, _923 * _101)) + _122) / _941;
                float _947 = (((mad(_113, _925, mad(_106, _924, _923 * _99)) + _120) / _941) * 0.5f) + 0.5f;
                float _948 = 0.5f - (((mad(_114, _925, mad(_107, _924, _923 * _100)) + _121) / _941) * 0.5f);
                float _951 = _947 * _50_m0[51u].x;
                float _952 = _948 * _50_m0[51u].y;
                float _957 = _923 + (_900 * 0.100000001490116119384765625f);
                float _958 = _924 + (_901 * 0.100000001490116119384765625f);
                float _959 = _925 + (_902 * 0.100000001490116119384765625f);
                float _975 = mad(_116, _959, mad(_109, _958, _957 * _102)) + _123;
                float _984 = _50_m0[51u].x * (((((mad(_113, _959, mad(_106, _958, _957 * _99)) + _120) / _975) * 0.5f) + 0.5f) - _947);
                float _986 = _50_m0[51u].y * ((0.5f - (((mad(_114, _959, mad(_107, _958, _957 * _100)) + _121) / _975) * 0.5f)) - _948);
                float _987 = ((mad(_115, _959, mad(_108, _958, _957 * _101)) + _122) / _975) - _944;
                float _988 = _984 * 10.0f;
                float _990 = _986 * 10.0f;
                float _991 = _987 * 10.0f;
                float _995 = 0.75f / dot(float3(_900, _901, _902), float3(_516, _519, _522));
                float _1000 = (_995 * _900) + _923;
                float _1001 = (_995 * _901) + _924;
                float _1002 = (_995 * _902) + _925;
                float _1018 = mad(_116, _1002, mad(_109, _1001, _1000 * _102)) + _123;
                float _1027 = _50_m0[51u].x * (((((mad(_113, _1002, mad(_106, _1001, _1000 * _99)) + _120) / _1018) * 0.5f) + 0.5f) - _947);
                float _1029 = _50_m0[51u].y * ((0.5f - (((mad(_114, _1002, mad(_107, _1001, _1000 * _100)) + _121) / _1018) * 0.5f)) - _948);
                float _1030 = ((mad(_115, _1002, mad(_108, _1001, _1000 * _101)) + _122) / _1018) - _944;
                float _1043 = sqrt(((_1027 * _1027) + (_1030 * _1030)) + (_1029 * _1029)) / sqrt(((_988 * _988) + (_991 * _991)) + (_990 * _990));
                float _1044 = float(_233);
                float _1045 = float(_234);
                float _1052 = (_988 != 0.0f) ? (0.100000001490116119384765625f / _984) : 3.4028234663852885981170418348452e+38f;
                float _1054 = (_990 != 0.0f) ? (0.100000001490116119384765625f / _986) : 3.4028234663852885981170418348452e+38f;
                float _1055 = (_991 != 0.0f) ? (0.100000001490116119384765625f / _987) : 3.4028234663852885981170418348452e+38f;
                float _1056 = 1.0f / _1044;
                float _1057 = 1.0f / _1045;
                float _1058 = 0.004999999888241291046142578125f / _1044;
                float _1060 = 0.004999999888241291046142578125f / _1045;
                float _1069 = float(_988 >= 0.0f);
                float _1070 = float(_990 >= 0.0f);
                float _1079 = ((_988 < 0.0f) ? ((-0.0f) - _1058) : _1058) - _951;
                float _1082 = ((_990 < 0.0f) ? ((-0.0f) - _1060) : _1060) - _952;
                float _1085 = min((((floor(_951 * _1044) + _1069) * _1056) + _1079) * _1052, (((floor(_952 * _1045) + _1070) * _1057) + _1082) * _1054);
                float _1089 = (_1085 * _988) + _951;
                float _1090 = (_1085 * _990) + _952;
                float _1091 = (_1085 * _991) + _944;
                float _1094 = _50_m0[50u].x / (_458 - (_1091 * _50_m0[50u].y));
                float _1105 = max(0.300000011920928955078125f, 10.0f / max(0.00999999977648258209228515625f, max(abs(_984 * 38400.0f), abs(_986 * 21600.0f))));
                uint _1118;
                float _1122;
                float _1124;
                float _1126;
                uint _1138;
                uint _1140;
                uint _1120;
                float _1128;
                float _1130;
                float _1132;
                float _1134;
                float _1136;
                float _1142;
                float _1144;
                float _1146;
                float _1148;
                float _1150;
                float _1152;
                float _1154;
                uint _1156;
                float _1158;
                float _1160;
                uint _1162;
                uint _1117 = 0u;
                uint _1119 = 0u;
                float _1121 = _1091;
                float _1123 = _1090;
                float _1125 = _1089;
                float _1127 = _1085;
                float _1129 = _1057;
                float _1131 = _1056;
                float _1133 = _1045;
                float _1135 = _1044;
                uint _1137 = 0u;
                uint _1139 = 0u;
                float _1141 = _944;
                float _1143 = _952;
                float _1145 = _951;
                float _1147 = _1091;
                float _1149 = _1090;
                float _1151 = _1089;
                float _1153 = 1.0f;
                uint _1155 = 0u;
                float _1157 = 0.0f;
                float _1159 = 0.0f;
                uint _1161 = 1u;
                float _1163;
                float _1164;
                uint _1165;
                uint _1166;
                bool _1167;
                for (;;)
                {
                    _1163 = _1135 * _1125;
                    _1164 = _1133 * _1123;
                    _1165 = uint(int(_1163));
                    _1166 = uint(int(_1164));
                    _1167 = _1137 == 0u;
                    float _1220;
                    if (_1167)
                    {
                        _1220 = _12.Load(int3(uint2(_1165, _1166), 0u)).x;
                    }
                    else
                    {
                        _1220 = _15.Load(int3(uint2(_1165, _1166), _1137 + 4294967295u)).x;
                    }
                    float _1226 = ((_1163 >= floor(_1135)) || (_1164 >= floor(_1133))) ? 1.0f : _1220;
                    float _1240 = (_991 < 0.0f) ? ((_1226 - _944) * _1055) : 3.4028234663852885981170418348452e+38f;
                    float _1242 = min(min((((floor(_1163) + _1069) * _1131) + _1079) * _1052, (((floor(_1164) + _1070) * _1129) + _1082) * _1054), _1240);
                    bool _1243 = _1226 < _1121;
                    bool _1247 = _1243 && (asuint(_1242) != asuint(_1240));
                    float _1248 = _1243 ? _1242 : _1127;
                    float _1252 = (_1248 * _988) + _951;
                    float _1253 = (_1248 * _990) + _952;
                    float _1254 = (_1248 * _991) + _944;
                    uint _1256 = (_1247 ? 1u : 4294967295u) + _1137;
                    float _1257 = _1247 ? 0.5f : 2.0f;
                    float _1258 = _1257 * _1135;
                    float _1259 = _1257 * _1133;
                    float _1260 = _1247 ? 2.0f : 0.5f;
                    float _1261 = _1260 * _1131;
                    float _1262 = _1260 * _1129;
                    _1118 = _1117 + 1u;
                    uint _1321;
                    uint _1324;
                    if (int(_1256) < int(0u))
                    {
                        float frontier_phi_59_53_ladder;
                        float frontier_phi_59_53_ladder_1;
                        float frontier_phi_59_53_ladder_2;
                        float frontier_phi_59_53_ladder_3;
                        float frontier_phi_59_53_ladder_4;
                        float frontier_phi_59_53_ladder_5;
                        uint frontier_phi_59_53_ladder_6;
                        float frontier_phi_59_53_ladder_7;
                        float frontier_phi_59_53_ladder_8;
                        uint frontier_phi_59_53_ladder_9;
                        uint frontier_phi_59_53_ladder_10;
                        uint frontier_phi_59_53_ladder_11;
                        float frontier_phi_59_53_ladder_12;
                        float frontier_phi_59_53_ladder_13;
                        float frontier_phi_59_53_ladder_14;
                        float frontier_phi_59_53_ladder_15;
                        float frontier_phi_59_53_ladder_16;
                        uint frontier_phi_59_53_ladder_17;
                        float frontier_phi_59_53_ladder_18;
                        float _1287;
                        float _1292;
                        float _1295;
                        bool _1296;
                        for (;;)
                        {
                            float _1285 = _50_m0[50u].w + _50_m0[50u].y;
                            _1287 = _50_m0[50u].x / (_1285 - (_50_m0[50u].y * _1226));
                            float _1290 = _50_m0[50u].x / (_1285 - (_50_m0[50u].y * _1254));
                            _1292 = abs(_1094 - _1290);
                            _1295 = _1290 - _1287;
                            _1296 = _1295 > max(0.00999999977648258209228515625f, _1292 * 0.00999999977648258209228515625f);
                            if (_1296)
                            {
                                uint _1326;
                                if (_1119 == 0u)
                                {
                                    uint frontier_phi_63_62_ladder;
                                    if ((_397 == 2u) || (_397 == 4u))
                                    {
                                        if ((_1248 < _1043) && (abs(_1295) < 2.0f))
                                        {
                                            frontier_phi_59_53_ladder = _1143;
                                            frontier_phi_59_53_ladder_1 = _1151;
                                            frontier_phi_59_53_ladder_2 = _1145;
                                            frontier_phi_59_53_ladder_3 = _1147;
                                            frontier_phi_59_53_ladder_4 = _1149;
                                            frontier_phi_59_53_ladder_5 = 0.0f;
                                            frontier_phi_59_53_ladder_6 = 1u;
                                            frontier_phi_59_53_ladder_7 = _1253;
                                            frontier_phi_59_53_ladder_8 = _1252;
                                            frontier_phi_59_53_ladder_9 = _1161;
                                            frontier_phi_59_53_ladder_10 = 1u;
                                            frontier_phi_59_53_ladder_11 = _1256;
                                            frontier_phi_59_53_ladder_12 = _1258;
                                            frontier_phi_59_53_ladder_13 = _1259;
                                            frontier_phi_59_53_ladder_14 = _1261;
                                            frontier_phi_59_53_ladder_15 = _1262;
                                            frontier_phi_59_53_ladder_16 = _1248;
                                            frontier_phi_59_53_ladder_17 = 1u;
                                            frontier_phi_59_53_ladder_18 = _1141;
                                            break;
                                        }
                                        frontier_phi_63_62_ladder = 1u;
                                    }
                                    else
                                    {
                                        frontier_phi_63_62_ladder = 1u;
                                    }
                                    _1326 = frontier_phi_63_62_ladder;
                                }
                                else
                                {
                                    _1326 = _1119;
                                }
                                if (!(_1155 == 0u))
                                {
                                    frontier_phi_59_53_ladder = _1143;
                                    frontier_phi_59_53_ladder_1 = _1151;
                                    frontier_phi_59_53_ladder_2 = _1145;
                                    frontier_phi_59_53_ladder_3 = _1147;
                                    frontier_phi_59_53_ladder_4 = _1149;
                                    frontier_phi_59_53_ladder_5 = _1153;
                                    frontier_phi_59_53_ladder_6 = _1155;
                                    frontier_phi_59_53_ladder_7 = _1157;
                                    frontier_phi_59_53_ladder_8 = _1159;
                                    frontier_phi_59_53_ladder_9 = _1161;
                                    frontier_phi_59_53_ladder_10 = _1139;
                                    frontier_phi_59_53_ladder_11 = _1256;
                                    frontier_phi_59_53_ladder_12 = _1258;
                                    frontier_phi_59_53_ladder_13 = _1259;
                                    frontier_phi_59_53_ladder_14 = _1261;
                                    frontier_phi_59_53_ladder_15 = _1262;
                                    frontier_phi_59_53_ladder_16 = _1248;
                                    frontier_phi_59_53_ladder_17 = _1326;
                                    frontier_phi_59_53_ladder_18 = _1141;
                                    break;
                                }
                                bool _1428 = _1139 != 0u;
                                frontier_phi_59_53_ladder = _1143;
                                frontier_phi_59_53_ladder_1 = _1151;
                                frontier_phi_59_53_ladder_2 = _1145;
                                frontier_phi_59_53_ladder_3 = _1147;
                                frontier_phi_59_53_ladder_4 = _1149;
                                frontier_phi_59_53_ladder_5 = _1153;
                                frontier_phi_59_53_ladder_6 = 0u;
                                frontier_phi_59_53_ladder_7 = _1428 ? _1157 : _1253;
                                frontier_phi_59_53_ladder_8 = _1428 ? _1159 : _1252;
                                frontier_phi_59_53_ladder_9 = _1161;
                                frontier_phi_59_53_ladder_10 = ((_397 == 1u) || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1139;
                                frontier_phi_59_53_ladder_11 = 0u;
                                frontier_phi_59_53_ladder_12 = _1044;
                                frontier_phi_59_53_ladder_13 = _1045;
                                frontier_phi_59_53_ladder_14 = _1056;
                                frontier_phi_59_53_ladder_15 = _1057;
                                frontier_phi_59_53_ladder_16 = _1248 + _1105;
                                frontier_phi_59_53_ladder_17 = _1326;
                                frontier_phi_59_53_ladder_18 = _1141;
                                break;
                            }
                            else
                            {
                                float _1312 = max(0.100000001490116119384765625f, _1292 * 0.100000001490116119384765625f) * 0.5f;
                                float _1315 = clamp((abs(_1295) - _1312) / _1312, 0.0f, 1.0f);
                                uint _1317 = uint(_1287 < _1094);
                                float frontier_phi_59_53_ladder_58_ladder;
                                float frontier_phi_59_53_ladder_58_ladder_1;
                                float frontier_phi_59_53_ladder_58_ladder_2;
                                float frontier_phi_59_53_ladder_58_ladder_3;
                                float frontier_phi_59_53_ladder_58_ladder_4;
                                float frontier_phi_59_53_ladder_58_ladder_5;
                                uint frontier_phi_59_53_ladder_58_ladder_6;
                                float frontier_phi_59_53_ladder_58_ladder_7;
                                float frontier_phi_59_53_ladder_58_ladder_8;
                                uint frontier_phi_59_53_ladder_58_ladder_9;
                                uint frontier_phi_59_53_ladder_58_ladder_10;
                                uint frontier_phi_59_53_ladder_58_ladder_11;
                                float frontier_phi_59_53_ladder_58_ladder_12;
                                float frontier_phi_59_53_ladder_58_ladder_13;
                                float frontier_phi_59_53_ladder_58_ladder_14;
                                float frontier_phi_59_53_ladder_58_ladder_15;
                                float frontier_phi_59_53_ladder_58_ladder_16;
                                uint frontier_phi_59_53_ladder_58_ladder_17;
                                float frontier_phi_59_53_ladder_58_ladder_18;
                                if (_1139 == 0u)
                                {
                                    frontier_phi_59_53_ladder_58_ladder = _1143;
                                    frontier_phi_59_53_ladder_58_ladder_1 = _1151;
                                    frontier_phi_59_53_ladder_58_ladder_2 = _1145;
                                    frontier_phi_59_53_ladder_58_ladder_3 = _1147;
                                    frontier_phi_59_53_ladder_58_ladder_4 = _1149;
                                    frontier_phi_59_53_ladder_58_ladder_5 = _1315;
                                    frontier_phi_59_53_ladder_58_ladder_6 = _1155;
                                    frontier_phi_59_53_ladder_58_ladder_7 = _1157;
                                    frontier_phi_59_53_ladder_58_ladder_8 = _1159;
                                    frontier_phi_59_53_ladder_58_ladder_9 = _1317;
                                    frontier_phi_59_53_ladder_58_ladder_10 = uint(_1315 > 0.0f);
                                    frontier_phi_59_53_ladder_58_ladder_11 = _1256;
                                    frontier_phi_59_53_ladder_58_ladder_12 = _1258;
                                    frontier_phi_59_53_ladder_58_ladder_13 = _1259;
                                    frontier_phi_59_53_ladder_58_ladder_14 = _1261;
                                    frontier_phi_59_53_ladder_58_ladder_15 = _1262;
                                    frontier_phi_59_53_ladder_58_ladder_16 = _1248;
                                    frontier_phi_59_53_ladder_58_ladder_17 = _1119;
                                    frontier_phi_59_53_ladder_58_ladder_18 = _1141;
                                }
                                else
                                {
                                    frontier_phi_59_53_ladder_58_ladder = _1143;
                                    frontier_phi_59_53_ladder_58_ladder_1 = _1151;
                                    frontier_phi_59_53_ladder_58_ladder_2 = _1145;
                                    frontier_phi_59_53_ladder_58_ladder_3 = _1147;
                                    frontier_phi_59_53_ladder_58_ladder_4 = _1149;
                                    frontier_phi_59_53_ladder_58_ladder_5 = _1315;
                                    frontier_phi_59_53_ladder_58_ladder_6 = _1155;
                                    frontier_phi_59_53_ladder_58_ladder_7 = _1157;
                                    frontier_phi_59_53_ladder_58_ladder_8 = _1159;
                                    frontier_phi_59_53_ladder_58_ladder_9 = _1317;
                                    frontier_phi_59_53_ladder_58_ladder_10 = _1139;
                                    frontier_phi_59_53_ladder_58_ladder_11 = _1256;
                                    frontier_phi_59_53_ladder_58_ladder_12 = _1258;
                                    frontier_phi_59_53_ladder_58_ladder_13 = _1259;
                                    frontier_phi_59_53_ladder_58_ladder_14 = _1261;
                                    frontier_phi_59_53_ladder_58_ladder_15 = _1262;
                                    frontier_phi_59_53_ladder_58_ladder_16 = _1248;
                                    frontier_phi_59_53_ladder_58_ladder_17 = _1119;
                                    frontier_phi_59_53_ladder_58_ladder_18 = _1141;
                                }
                                frontier_phi_59_53_ladder = frontier_phi_59_53_ladder_58_ladder;
                                frontier_phi_59_53_ladder_1 = frontier_phi_59_53_ladder_58_ladder_1;
                                frontier_phi_59_53_ladder_2 = frontier_phi_59_53_ladder_58_ladder_2;
                                frontier_phi_59_53_ladder_3 = frontier_phi_59_53_ladder_58_ladder_3;
                                frontier_phi_59_53_ladder_4 = frontier_phi_59_53_ladder_58_ladder_4;
                                frontier_phi_59_53_ladder_5 = frontier_phi_59_53_ladder_58_ladder_5;
                                frontier_phi_59_53_ladder_6 = frontier_phi_59_53_ladder_58_ladder_6;
                                frontier_phi_59_53_ladder_7 = frontier_phi_59_53_ladder_58_ladder_7;
                                frontier_phi_59_53_ladder_8 = frontier_phi_59_53_ladder_58_ladder_8;
                                frontier_phi_59_53_ladder_9 = frontier_phi_59_53_ladder_58_ladder_9;
                                frontier_phi_59_53_ladder_10 = frontier_phi_59_53_ladder_58_ladder_10;
                                frontier_phi_59_53_ladder_11 = frontier_phi_59_53_ladder_58_ladder_11;
                                frontier_phi_59_53_ladder_12 = frontier_phi_59_53_ladder_58_ladder_12;
                                frontier_phi_59_53_ladder_13 = frontier_phi_59_53_ladder_58_ladder_13;
                                frontier_phi_59_53_ladder_14 = frontier_phi_59_53_ladder_58_ladder_14;
                                frontier_phi_59_53_ladder_15 = frontier_phi_59_53_ladder_58_ladder_15;
                                frontier_phi_59_53_ladder_16 = frontier_phi_59_53_ladder_58_ladder_16;
                                frontier_phi_59_53_ladder_17 = frontier_phi_59_53_ladder_58_ladder_17;
                                frontier_phi_59_53_ladder_18 = frontier_phi_59_53_ladder_58_ladder_18;
                                break;
                            }
                        }
                        _1162 = frontier_phi_59_53_ladder_9;
                        _1160 = frontier_phi_59_53_ladder_8;
                        _1158 = frontier_phi_59_53_ladder_7;
                        _1156 = frontier_phi_59_53_ladder_6;
                        _1154 = frontier_phi_59_53_ladder_5;
                        _1152 = frontier_phi_59_53_ladder_1;
                        _1150 = frontier_phi_59_53_ladder_4;
                        _1148 = frontier_phi_59_53_ladder_3;
                        _1146 = frontier_phi_59_53_ladder_2;
                        _1144 = frontier_phi_59_53_ladder;
                        _1142 = frontier_phi_59_53_ladder_18;
                        _1321 = frontier_phi_59_53_ladder_10;
                        _1324 = frontier_phi_59_53_ladder_11;
                        _1136 = frontier_phi_59_53_ladder_12;
                        _1134 = frontier_phi_59_53_ladder_13;
                        _1132 = frontier_phi_59_53_ladder_14;
                        _1130 = frontier_phi_59_53_ladder_15;
                        _1128 = frontier_phi_59_53_ladder_16;
                        _1120 = frontier_phi_59_53_ladder_17;
                    }
                    else
                    {
                        bool _1297 = _1139 != 0u;
                        _1162 = _1161;
                        _1160 = _1159;
                        _1158 = _1157;
                        _1156 = _1155;
                        _1154 = _1153;
                        _1152 = _1297 ? _1151 : _1252;
                        _1150 = _1297 ? _1149 : _1253;
                        _1148 = _1297 ? _1147 : _1254;
                        _1146 = _1252;
                        _1144 = _1253;
                        _1142 = _1254;
                        _1321 = _1139;
                        _1324 = _1256;
                        _1136 = _1258;
                        _1134 = _1259;
                        _1132 = _1261;
                        _1130 = _1262;
                        _1128 = _1248;
                        _1120 = _1119;
                    }
                    uint frontier_phi_72_pred;
                    uint frontier_phi_72_pred_1;
                    float frontier_phi_72_pred_2;
                    float frontier_phi_72_pred_3;
                    float frontier_phi_72_pred_4;
                    bool _1329;
                    bool _1331;
                    for (;;)
                    {
                        _1329 = _1254 < 0.0f;
                        _1331 = _1329 || ((_1252 < 0.0f) || (_1253 < 0.0f));
                        if (!_1331)
                        {
                            if (!((_1254 > 1.0f) || ((_1252 > _50_m0[51u].x) || (_1253 > _50_m0[51u].y))))
                            {
                                frontier_phi_72_pred = _1324;
                                frontier_phi_72_pred_1 = _1321;
                                frontier_phi_72_pred_2 = _1252;
                                frontier_phi_72_pred_3 = _1253;
                                frontier_phi_72_pred_4 = _1254;
                                break;
                            }
                        }
                        if (!_1329)
                        {
                            frontier_phi_72_pred = 4294967295u;
                            frontier_phi_72_pred_1 = 1u;
                            frontier_phi_72_pred_2 = _1252;
                            frontier_phi_72_pred_3 = _1253;
                            frontier_phi_72_pred_4 = _1254;
                            break;
                        }
                        float _1437 = (-0.0f) - _1254;
                        float _1438 = _1437 / _991;
                        frontier_phi_72_pred = 4294967295u;
                        frontier_phi_72_pred_1 = 1u;
                        frontier_phi_72_pred_2 = (_1438 * _988) + _1252;
                        frontier_phi_72_pred_3 = (_1438 * _990) + _1253;
                        frontier_phi_72_pred_4 = _1437 + _1254;
                        break;
                    }
                    _1138 = frontier_phi_72_pred;
                    _1140 = frontier_phi_72_pred_1;
                    _1126 = frontier_phi_72_pred_2;
                    _1124 = frontier_phi_72_pred_3;
                    _1122 = frontier_phi_72_pred_4;
                    if ((_1118 < 128u) && (int(_1138) > int(4294967295u)))
                    {
                        _1117 = _1118;
                        _1119 = _1120;
                        _1121 = _1122;
                        _1123 = _1124;
                        _1125 = _1126;
                        _1127 = _1128;
                        _1129 = _1130;
                        _1131 = _1132;
                        _1133 = _1134;
                        _1135 = _1136;
                        _1137 = _1138;
                        _1139 = _1140;
                        _1141 = _1142;
                        _1143 = _1144;
                        _1145 = _1146;
                        _1147 = _1148;
                        _1149 = _1150;
                        _1151 = _1152;
                        _1153 = _1154;
                        _1155 = _1156;
                        _1157 = _1158;
                        _1159 = _1160;
                        _1161 = _1162;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                bool _1460 = dot(float3(_900, _901, _902), float3(_511, _512, _513)) < 0.0f;
                bool _1461 = _1118 > 127u;
                uint _1462 = _1461 ? 1u : _1140;
                float _1471 = _50_m0[51u].z * 2.0f;
                float _1474 = (_1471 * _951) + (-1.0f);
                float _1475 = ((1.0f - (_50_m0[51u].w * _952)) * 2.0f) + (-1.0f);
                float _1491 = mad(_190, _944, mad(_183, _1475, _1474 * _176)) + _197;
                float _1492 = (mad(_187, _944, mad(_180, _1475, _1474 * _173)) + _194) / _1491;
                float _1493 = (mad(_188, _944, mad(_181, _1475, _1474 * _174)) + _195) / _1491;
                float _1494 = (mad(_189, _944, mad(_182, _1475, _1474 * _175)) + _196) / _1491;
                float _1499 = (_1471 * _1126) + (-1.0f);
                float _1500 = ((1.0f - (_50_m0[51u].w * _1124)) * 2.0f) + (-1.0f);
                float _1516 = mad(_190, _1122, mad(_183, _1500, _1499 * _176)) + _197;
                float _1520 = ((mad(_187, _1122, mad(_180, _1500, _1499 * _173)) + _194) / _1516) - _1492;
                float _1521 = ((mad(_188, _1122, mad(_181, _1500, _1499 * _174)) + _195) / _1516) - _1493;
                float _1522 = ((mad(_189, _1122, mad(_182, _1500, _1499 * _175)) + _196) / _1516) - _1494;
                float _1538;
                uint _1540;
                float _1542;
                if (_1118 < 129u)
                {
                    float frontier_phi_79_78_ladder;
                    uint frontier_phi_79_78_ladder_1;
                    float frontier_phi_79_78_ladder_2;
                    if ((_1126 < 0.0f) || (_1124 < 0.0f))
                    {
                        frontier_phi_79_78_ladder = 0.0f;
                        frontier_phi_79_78_ladder_1 = _1462;
                        frontier_phi_79_78_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_79_78_ladder_83_ladder;
                        uint frontier_phi_79_78_ladder_83_ladder_1;
                        float frontier_phi_79_78_ladder_83_ladder_2;
                        if ((_1122 >= 1.0f) || ((_1126 > _50_m0[51u].x) || (_1124 > _50_m0[51u].y)))
                        {
                            frontier_phi_79_78_ladder_83_ladder = 0.0f;
                            frontier_phi_79_78_ladder_83_ladder_1 = _1462;
                            frontier_phi_79_78_ladder_83_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_79_78_ladder_83_ladder_88_ladder;
                            uint frontier_phi_79_78_ladder_83_ladder_88_ladder_1;
                            float frontier_phi_79_78_ladder_83_ladder_88_ladder_2;
                            for (;;)
                            {
                                if ((abs(_1126 - _254) < (2.0f / _1044)) && (abs(_1124 - _255) < (2.0f / _1045)))
                                {
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder = 0.0f;
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder_1 = _1462;
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_112;
                                    float frontier_phi_112_pred;
                                    uint frontier_phi_112_pred_1;
                                    float frontier_phi_112_pred_2;
                                    uint _1947;
                                    uint _1948;
                                    bool _1950;
                                    for (;;)
                                    {
                                        _1947 = uint(int(_1126 * _1044));
                                        _1948 = uint(int(_1124 * _1045));
                                        _1950 = (_397 == 1u) && _1460;
                                        if (!_1950)
                                        {
                                            if (!(dot(float3(_1520, _1521, _1522), float3(_1520, _1521, _1522)) < 0.000899999984540045261383056640625f))
                                            {
                                                ladder_phi_112 = false;
                                                frontier_phi_112_pred = 0.0f;
                                                frontier_phi_112_pred_1 = _1462;
                                                frontier_phi_112_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _2002 = _24[22u].Load(int3(uint2(_1947, _1948), 0u));
                                        uint _2004 = _2002.x;
                                        float _2197;
                                        float _2198;
                                        float _2199;
                                        if (_2004 == 0u)
                                        {
                                            uint4 _2047 = _24[1u].Load(int3(uint2(_1947, _1948), 0u));
                                            uint _2049 = _2047.x;
                                            float _2057 = (float((_2049 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2058 = (float(_2049 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2062 = (1.0f - abs(_2057)) - abs(_2058);
                                            float _2064 = clamp((-0.0f) - _2062, 0.0f, 1.0f);
                                            float _2065 = (-0.0f) - _2064;
                                            _2197 = ((_2057 >= 0.0f) ? _2065 : _2064) + _2057;
                                            _2198 = ((_2058 >= 0.0f) ? _2065 : _2064) + _2058;
                                            _2199 = _2062;
                                        }
                                        else
                                        {
                                            float _2079 = (float((_2004 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2080 = (float(_2004 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2084 = (1.0f - abs(_2079)) - abs(_2080);
                                            float _2086 = clamp((-0.0f) - _2084, 0.0f, 1.0f);
                                            float _2087 = (-0.0f) - _2086;
                                            _2197 = ((_2079 >= 0.0f) ? _2087 : _2086) + _2079;
                                            _2198 = ((_2080 >= 0.0f) ? _2087 : _2086) + _2080;
                                            _2199 = _2084;
                                        }
                                        float _2203 = rsqrt(dot(float3(_2197, _2198, _2199), float3(_2197, _2198, _2199)));
                                        if (dot(float3(_2203 * _2197, _2203 * _2198, _2203 * _2199), float3(_1520, _1521, _1522)) > 0.0f)
                                        {
                                            ladder_phi_112 = true;
                                            frontier_phi_112_pred = 0.0f;
                                            frontier_phi_112_pred_1 = _1462;
                                            frontier_phi_112_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_112 = false;
                                            frontier_phi_112_pred = 0.0f;
                                            frontier_phi_112_pred_1 = _1462;
                                            frontier_phi_112_pred_2 = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_112)
                                    {
                                        frontier_phi_79_78_ladder_83_ladder_88_ladder = frontier_phi_112_pred;
                                        frontier_phi_79_78_ladder_83_ladder_88_ladder_1 = frontier_phi_112_pred_1;
                                        frontier_phi_79_78_ladder_83_ladder_88_ladder_2 = frontier_phi_112_pred_2;
                                        break;
                                    }
                                    float _2098 = _50_m0[51u].z * _1126;
                                    float _2099 = _50_m0[51u].w * _1124;
                                    float _2101 = (_1045 / _1044) * 0.0500000007450580596923828125f;
                                    float _2106 = clamp(_2098 / _2101, 0.0f, 1.0f);
                                    float _2107 = clamp(_2099 * 20.0f, 0.0f, 1.0f);
                                    float _2119 = clamp(((_2098 + (-1.0f)) + _2101) / _2101, 0.0f, 1.0f);
                                    float _2120 = clamp((_2099 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _2131 = _2106 * _2107;
                                    precise float _2132 = _2131 * _2131;
                                    float _2136 = ((((3.0f - (_2107 * 2.0f)) * (3.0f - (_2106 * 2.0f))) * _2132) * (1.0f - ((_2119 * _2119) * (3.0f - (_2119 * 2.0f))))) * (1.0f - ((_2120 * _2120) * (3.0f - (_2120 * 2.0f))));
                                    bool _2139 = (_1462 != 0u) || (_2136 >= 1.0f);
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder = _2136 * float(_475 > 0.0f);
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder_1 = _2139 ? _1462 : 1u;
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder_2 = _2139 ? 0.0f : _2136;
                                    break;
                                }
                            }
                            frontier_phi_79_78_ladder_83_ladder = frontier_phi_79_78_ladder_83_ladder_88_ladder;
                            frontier_phi_79_78_ladder_83_ladder_1 = frontier_phi_79_78_ladder_83_ladder_88_ladder_1;
                            frontier_phi_79_78_ladder_83_ladder_2 = frontier_phi_79_78_ladder_83_ladder_88_ladder_2;
                        }
                        frontier_phi_79_78_ladder = frontier_phi_79_78_ladder_83_ladder;
                        frontier_phi_79_78_ladder_1 = frontier_phi_79_78_ladder_83_ladder_1;
                        frontier_phi_79_78_ladder_2 = frontier_phi_79_78_ladder_83_ladder_2;
                    }
                    _1538 = frontier_phi_79_78_ladder_2;
                    _1540 = frontier_phi_79_78_ladder_1;
                    _1542 = frontier_phi_79_78_ladder;
                }
                else
                {
                    _1538 = 0.0f;
                    _1540 = _1462;
                    _1542 = 0.0f;
                }
                bool _1669;
                float _1672;
                float _1675;
                float _1677;
                float _1679;
                float _1681;
                float _1682;
                float _1683;
                float _1685;
                float _1583;
                float _1586;
                float _1589;
                float _1592;
                float _1596;
                float _1597;
                for (;;)
                {
                    _1583 = ((((exp2(log2(clamp((sqrt(((_1493 * _1493) + (_1492 * _1492)) + (_1494 * _1494)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _407) * exp2(log2(clamp((_1493 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_393, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    _1586 = mad(_167, _902, mad(_161, _901, _900 * _155));
                    _1589 = mad(_168, _902, mad(_162, _901, _900 * _156));
                    _1592 = mad(_169, _902, mad(_163, _901, _900 * _157));
                    bool _1595 = (_1540 != 0u) || (_1542 < 1.0f);
                    _1596 = _1595 ? 0.0f : 1.0f;
                    _1597 = _1595 ? 0.0f : 0.5f;
                    if (_1595)
                    {
                        bool _1664 = _397 == 1u;
                        uint4 _1668 = asuint(_50_m0[60u]);
                        if (_1664)
                        {
                            if (int(_405) < int(1u))
                            {
                                if (_1668.x == 0u)
                                {
                                    _1669 = false;
                                    _1672 = 9899999600270360182784.0f;
                                    _1675 = _1146;
                                    _1677 = _1144;
                                    _1679 = _1142;
                                    _1681 = 0.0f;
                                    _1682 = 0.0f;
                                    _1683 = 0.0f;
                                    _1685 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1668.y == 0u)
                                {
                                    _1669 = false;
                                    _1672 = 9899999600270360182784.0f;
                                    _1675 = _1146;
                                    _1677 = _1144;
                                    _1679 = _1142;
                                    _1681 = 0.0f;
                                    _1682 = 0.0f;
                                    _1683 = 0.0f;
                                    _1685 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1668.z == 0u)
                            {
                                _1669 = false;
                                _1672 = 9899999600270360182784.0f;
                                _1675 = _1146;
                                _1677 = _1144;
                                _1679 = _1142;
                                _1681 = 0.0f;
                                _1682 = 0.0f;
                                _1683 = 0.0f;
                                _1685 = 0.0f;
                                break;
                            }
                        }
                        if (_1538 > 0.0f)
                        {
                            _1669 = false;
                            _1672 = 9899999600270360182784.0f;
                            _1675 = _1146;
                            _1677 = _1144;
                            _1679 = _1142;
                            _1681 = 0.0f;
                            _1682 = 1.0f;
                            _1683 = 1000.0f;
                            _1685 = 0.5f;
                            break;
                        }
                        if (((_1152 <= 0.0f) || (_1150 <= 0.0f)) || (_1148 <= 0.0f))
                        {
                            _1669 = false;
                            _1672 = 9899999600270360182784.0f;
                            _1675 = _1146;
                            _1677 = _1144;
                            _1679 = _1142;
                            _1681 = 0.0f;
                            _1682 = 1.0f;
                            _1683 = 1000.0f;
                            _1685 = 0.5f;
                            break;
                        }
                        if ((_1148 >= 1.0f) || ((_1152 >= _50_m0[51u].x) || (_1150 >= _50_m0[51u].y)))
                        {
                            _1669 = false;
                            _1672 = 9899999600270360182784.0f;
                            _1675 = _1146;
                            _1677 = _1144;
                            _1679 = _1142;
                            _1681 = 0.0f;
                            _1682 = 1.0f;
                            _1683 = 1000.0f;
                            _1685 = 0.5f;
                            break;
                        }
                        uint _2428;
                        uint _2430;
                        uint _2215;
                        uint _2216;
                        bool _2222;
                        for (;;)
                        {
                            _2215 = uint(clamp(_1160, 0.0f, 1.0f) * _202);
                            _2216 = uint(clamp(_1158, 0.0f, 1.0f) * _204);
                            _2222 = _20[21u].Load(int3(uint2(_2215, _2216), 0u)).x > 0.0f;
                            if (_2222)
                            {
                                uint _2314 = _24[23u].Load(int3(uint2(_2215, _2216), 0u)).y + 4294967295u;
                                _2428 = (uint(int(_2314) >> int(31u)) & 3u) + 1u;
                                _2430 = (int(_2314) < int(0u)) ? 0u : _2314;
                                break;
                            }
                            else
                            {
                                uint4 _2322 = _24[2u].Load(int3(uint2(_2215, _2216), 0u));
                                uint _2325 = _2322.w;
                                uint4 _2330 = _24[15u].Load(int3(uint2(_2215, _2216), 0u));
                                uint _2332 = _2330.y;
                                uint _2338 = ((_2332 & 64u) != 0u) ? uint((_2332 & 4294967167u) != 66u) : 4294967295u;
                                uint _2339 = _2325 & 128u;
                                uint _2341 = (_2339 != 0u) ? 1u : ((_2322.x << 7u) | _2325);
                                uint4 _2344 = _16.Load(_2341 * 4u);
                                uint _2345 = _2344.x;
                                uint _2352 = ((_2345 & 1u) != 0u) ? 0u : 18u;
                                uint _2354 = uint(min(int(uint(max(int(_2338), int(0u)))), int(1u)));
                                uint _2441;
                                if (_2339 == 0u)
                                {
                                    _2441 = (((_2345 & 2097152u) != 0u) && (_2338 == _2354)) ? (_2352 | 128u) : _2352;
                                }
                                else
                                {
                                    _2441 = _2325;
                                }
                                uint _2442 = _16.Load((_2341 * 4u) + 1u).x & 512u;
                                bool _2445 = (_2441 & 144u) == 0u;
                                if (_2442 == 0u)
                                {
                                    if (_2445 || ((_2345 & 1u) != 0u))
                                    {
                                        _2428 = 0u;
                                        _2430 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2445)
                                    {
                                        _2428 = 0u;
                                        _2430 = 0u;
                                        break;
                                    }
                                }
                                bool _2612 = ((_2441 & 128u) | _2442) != 0u;
                                uint _2429;
                                if (_2612)
                                {
                                    _2429 = 1u;
                                }
                                else
                                {
                                    _2429 = (((_2345 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_2345 & 268435472u) == 16u) && (((_2345 & 33554432u) == 0u) || _2612))
                                {
                                    _2428 = 2u;
                                    _2430 = 0u;
                                    break;
                                }
                                _2428 = _2429;
                                _2430 = (_2429 == 1u) ? _2354 : 0u;
                                break;
                            }
                        }
                        if ((_397 != _2428) || (_405 != _2430))
                        {
                            _1669 = false;
                            _1672 = 9899999600270360182784.0f;
                            _1675 = _1146;
                            _1677 = _1144;
                            _1679 = _1142;
                            _1681 = 0.0f;
                            _1682 = 1.0f;
                            _1683 = 1000.0f;
                            _1685 = 0.5f;
                            break;
                        }
                        float _2496 = _1152 * 2.0f;
                        float _2499 = (_50_m0[51u].z * _2496) + (-1.0f);
                        float _2500 = ((1.0f - (_50_m0[51u].w * _1150)) * 2.0f) + (-1.0f);
                        float _2516 = mad(_190, _1148, mad(_183, _2500, _2499 * _176)) + _197;
                        float _2517 = (mad(_187, _1148, mad(_180, _2500, _2499 * _173)) + _194) / _2516;
                        float _2518 = (mad(_188, _1148, mad(_181, _2500, _2499 * _174)) + _195) / _2516;
                        float _2519 = (mad(_189, _1148, mad(_182, _2500, _2499 * _175)) + _196) / _2516;
                        if (sqrt(((_2518 * _2518) + (_2517 * _2517)) + (_2519 * _2519)) > _50_m0[58u].w)
                        {
                            _1669 = false;
                            _1672 = 9899999600270360182784.0f;
                            _1675 = _1146;
                            _1677 = _1144;
                            _1679 = _1142;
                            _1681 = 0.0f;
                            _1682 = 0.0f;
                            _1683 = 1000.0f;
                            _1685 = 0.5f;
                            break;
                        }
                        float _2592 = _2517 - _1492;
                        float _2593 = _2518 - _1493;
                        float _2594 = _2519 - _1494;
                        float _2600 = sqrt(((_2593 * _2593) + (_2592 * _2592)) + (_2594 * _2594));
                        float _2608 = min(_50_m0[59u].y, max(0.0f, _2600 + (-0.001000000047497451305389404296875f)));
                        float _2636;
                        if (_1664)
                        {
                            _2636 = min(_50_m0[59u].x, _2608 + 10.0f);
                        }
                        else
                        {
                            _2636 = _50_m0[59u].x;
                        }
                        float _2637 = _2636 - _2600;
                        if (!(_2637 > 0.0f))
                        {
                            _1669 = false;
                            _1672 = 9899999600270360182784.0f;
                            _1675 = _1146;
                            _1677 = _1144;
                            _1679 = _1142;
                            _1681 = 1.0f;
                            _1682 = 1.0f;
                            _1683 = 0.0f;
                            _1685 = 0.5f;
                            break;
                        }
                        float _2697 = _2517 - (_2608 * _1586);
                        float _2698 = _2518 - (_2608 * _1589);
                        float _2699 = _2519 - (_2608 * _1592);
                        RayDesc _2ident = {float3(mad(_2699, _50_m0[46u].z, mad(_2698, _50_m0[46u].y, _50_m0[46u].x * _2697)) + _50_m0[46u].w, mad(_2699, _50_m0[47u].z, mad(_2698, _50_m0[47u].y, _50_m0[47u].x * _2697)) + _50_m0[47u].w, mad(_2699, _50_m0[48u].z, mad(_2698, _50_m0[48u].y, _50_m0[48u].x * _2697)) + _50_m0[48u].w), 0.0f, float3(mad(_1592, _50_m0[46u].z, mad(_1589, _50_m0[46u].y, _50_m0[46u].x * _1586)), mad(_1592, _50_m0[47u].z, mad(_1589, _50_m0[47u].y, _50_m0[47u].x * _1586)), mad(_1592, _50_m0[48u].z, mad(_1589, _50_m0[48u].y, _50_m0[48u].x * _1586))), _2637};
                        _2702.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2749 = _2702.Proceed();
                        uint _2750 = _2702.CommittedStatus();
                        if (!(_2750 == 1u))
                        {
                            _1669 = false;
                            _1672 = 9899999600270360182784.0f;
                            _1675 = _1146;
                            _1677 = _1144;
                            _1679 = _1142;
                            _1681 = 1.0f;
                            _1682 = 0.0f;
                            _1683 = 0.0f;
                            _1685 = 0.5f;
                            break;
                        }
                        float _2764 = _2702.CommittedRayT();
                        if (!((_2764 < _2637) && (_2764 > 0.0f)))
                        {
                            _1669 = false;
                            _1672 = 9899999600270360182784.0f;
                            _1675 = _1146;
                            _1677 = _1144;
                            _1679 = _1142;
                            _1681 = 1.0f;
                            _1682 = 0.0f;
                            _1683 = 0.0f;
                            _1685 = 0.5f;
                            break;
                        }
                        float _2778 = (_50_m0[51u].z * _2496) + (-1.0f);
                        float _2779 = ((1.0f - (_50_m0[51u].w * _1150)) * 2.0f) + (-1.0f);
                        float _2795 = mad(_144, _1148, mad(_137, _2779, _2778 * _130)) + _151;
                        float _2799 = _2764 - _2608;
                        float _2803 = ((mad(_141, _1148, mad(_134, _2779, _2778 * _127)) + _148) / _2795) + (_2799 * _900);
                        float _2804 = ((mad(_142, _1148, mad(_135, _2779, _2778 * _128)) + _149) / _2795) + (_2799 * _901);
                        float _2805 = ((mad(_143, _1148, mad(_136, _2779, _2778 * _129)) + _150) / _2795) + (_2799 * _902);
                        float _2821 = mad(_116, _2805, mad(_109, _2804, _2803 * _102)) + _123;
                        float _1680 = (mad(_115, _2805, mad(_108, _2804, _2803 * _101)) + _122) / _2821;
                        float _1676 = ((((mad(_113, _2805, mad(_106, _2804, _2803 * _99)) + _120) / _2821) * 0.5f) + 0.5f) * _50_m0[51u].x;
                        float _1678 = (0.5f - (((mad(_114, _2805, mad(_107, _2804, _2803 * _100)) + _121) / _2821) * 0.5f)) * _50_m0[51u].y;
                        float _2830 = _1676 * _50_m0[51u].z;
                        float _2831 = _1678 * _50_m0[51u].w;
                        float _2833 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2836 = clamp(_2830 / _2833, 0.0f, 1.0f);
                        float _2837 = clamp(_2831 * 20.0f, 0.0f, 1.0f);
                        float _2847 = clamp(((_2830 + (-1.0f)) + _2833) / _2833, 0.0f, 1.0f);
                        float _2848 = clamp((_2831 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2859 = _2836 * _2837;
                        precise float _2860 = _2859 * _2859;
                        if ((((((3.0f - (_2837 * 2.0f)) * (3.0f - (_2836 * 2.0f))) * _2860) * (1.0f - ((_2847 * _2847) * (3.0f - (_2847 * 2.0f))))) * (1.0f - ((_2848 * _2848) * (3.0f - (_2848 * 2.0f))))) < 1.0f)
                        {
                            _1669 = false;
                            _1672 = 9899999600270360182784.0f;
                            _1675 = _1676;
                            _1677 = _1678;
                            _1679 = _1680;
                            _1681 = _1596;
                            _1682 = _1596;
                            _1683 = 0.0f;
                            _1685 = _1597;
                            break;
                        }
                        _1669 = true;
                        _1672 = (_2600 - _2608) + _2764;
                        _1675 = _1676;
                        _1677 = _1678;
                        _1679 = _1680;
                        _1681 = 0.0f;
                        _1682 = 1.0f;
                        _1683 = 0.0f;
                        _1685 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1669 = false;
                        _1672 = 9899999600270360182784.0f;
                        _1675 = _1146;
                        _1677 = _1144;
                        _1679 = _1142;
                        _1681 = 1.0f;
                        _1682 = 1.0f;
                        _1683 = 0.0f;
                        _1685 = 0.5f;
                        break;
                    }
                }
                uint4 _1688 = asuint(_55_m0[0u]);
                float _1690 = float(_1688.x);
                float _1692 = float(_1688.y);
                float _1714;
                float _1716;
                float _1718;
                if ((_1542 >= 1.0f) || _1669)
                {
                    _1714 = _1675;
                    _1716 = _1677;
                    _1718 = _1679;
                }
                else
                {
                    float _1865 = (-0.0f) - _944;
                    float _1866 = _1865 / _991;
                    float _1869 = (_1866 * _988) + _951;
                    float _1870 = (_1866 * _990) + _952;
                    float _1871 = _1865 + _944;
                    _1714 = ((_1675 - _1869) * _1542) + _1869;
                    _1716 = ((_1677 - _1870) * _1542) + _1870;
                    _1718 = ((_1679 - _1871) * _1542) + _1871;
                }
                float _1729 = ((_1714 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _1730 = ((1.0f - (_50_m0[51u].w * _1716)) * 2.0f) + (-1.0f);
                float _1746 = mad(_144, _1718, mad(_137, _1730, _1729 * _130)) + _151;
                float _1750 = ((mad(_141, _1718, mad(_134, _1730, _1729 * _127)) + _148) / _1746) - _923;
                float _1751 = ((mad(_142, _1718, mad(_135, _1730, _1729 * _128)) + _149) / _1746) - _924;
                float _1752 = ((mad(_143, _1718, mad(_136, _1730, _1729 * _129)) + _150) / _1746) - _925;
                float _1758 = sqrt(((_1751 * _1751) + (_1750 * _1750)) + (_1752 * _1752));
                float _1759 = _1758 * _778;
                float _1760 = _1758 * _779;
                float _1761 = _1758 * _780;
                float _1762 = _1758 * (_879 / _882);
                float _1763 = _1758 * (_880 / _882);
                float _1764 = _1758 * (_881 / _882);
                float _1768 = dot(float3(_1759, _1760, _1761), float3(_764, _767, _770)) * 2.0f;
                float _1778 = dot(float3(_1762, _1763, _1764), float3(_764, _767, _770)) * 2.0f;
                float _1805 = (_1759 - (_1768 * _764)) + _923;
                float _1806 = (_1760 - (_1768 * _767)) + _924;
                float _1807 = (_1761 - (_1768 * _770)) + _925;
                float _1819 = mad(_50_m0[24u].w, _1807, mad(_50_m0[23u].w, _1806, _1805 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1824 = (_1762 - (_1778 * _764)) + _923;
                float _1825 = (_1763 - (_1778 * _767)) + _924;
                float _1826 = (_1764 - (_1778 * _770)) + _925;
                float _1838 = mad(_50_m0[24u].w, _1826, mad(_50_m0[23u].w, _1825, _1824 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1844 = (_50_m0[51u].x * ((((mad(_50_m0[24u].x, _1807, mad(_50_m0[23u].x, _1806, _1805 * _50_m0[22u].x)) + _50_m0[25u].x) / _1819) - ((mad(_50_m0[24u].x, _1826, mad(_50_m0[23u].x, _1825, _1824 * _50_m0[22u].x)) + _50_m0[25u].x) / _1838)) * 0.5f)) * _1690;
                float _1848 = (_50_m0[51u].y * ((((mad(_50_m0[24u].y, _1826, mad(_50_m0[23u].y, _1825, _1824 * _50_m0[22u].y)) + _50_m0[25u].y) / _1838) - ((mad(_50_m0[24u].y, _1807, mad(_50_m0[23u].y, _1806, _1805 * _50_m0[22u].y)) + _50_m0[25u].y) / _1819)) * 0.5f)) * _1692;
                float _1863 = clamp(log2(sqrt((_1848 * _1848) + (_1844 * _1844)) * 2.0f) / float(asuint(_50_m0[55u]).x + 4294967295u), 0.0f, 1.0f);
                bool _1864 = _397 == 1u;
                float _1956;
                if (_1864)
                {
                    float _2016;
                    if (_1669)
                    {
                        _2016 = _1672;
                    }
                    else
                    {
                        _2016 = sqrt(((_1521 * _1521) + (_1520 * _1520)) + (_1522 * _1522));
                    }
                    float _2021 = clamp((_2016 + (-1.0f)) * 0.111111111938953399658203125f, 0.0f, 1.0f);
                    _1956 = (1.0f - ((_2021 * _2021) * (3.0f - (_2021 * 2.0f)))) * _50_m0[61u].z;
                }
                else
                {
                    _1956 = 1.0f;
                }
                float _1958 = _1956 * _1542;
                bool _1959 = _397 != 1u;
                float _2356;
                float _2358;
                float _2360;
                float _2362;
                if (_1958 == 0.0f)
                {
                    float _2248;
                    float _2250;
                    float _2252;
                    float _2254;
                    if (_1669)
                    {
                        float frontier_phi_121_114_ladder;
                        float frontier_phi_121_114_ladder_1;
                        float frontier_phi_121_114_ladder_2;
                        float frontier_phi_121_114_ladder_3;
                        if (_1959)
                        {
                            float _2223 = _1589 * _1583;
                            float _2227 = rsqrt(dot(float3(_1586, _2223, _1592), float3(_1586, _2223, _1592)));
                            float4 _2237 = _28[4u].SampleLevel(_59, float3(_2227 * _1586, _2227 * _2223, _2227 * _1592), 0.0f);
                            float _2239 = _2237.x;
                            float _2240 = _2237.y;
                            float _2241 = _2237.z;
                            frontier_phi_121_114_ladder = 1.0f;
                            frontier_phi_121_114_ladder_1 = _2241 - (_2241 * _1956);
                            frontier_phi_121_114_ladder_2 = _2240 - (_2240 * _1956);
                            frontier_phi_121_114_ladder_3 = _2239 - (_2239 * _1956);
                        }
                        else
                        {
                            frontier_phi_121_114_ladder = _1956;
                            frontier_phi_121_114_ladder_1 = 0.0f;
                            frontier_phi_121_114_ladder_2 = 0.0f;
                            frontier_phi_121_114_ladder_3 = 0.0f;
                        }
                        _2248 = frontier_phi_121_114_ladder_3;
                        _2250 = frontier_phi_121_114_ladder_2;
                        _2252 = frontier_phi_121_114_ladder_1;
                        _2254 = frontier_phi_121_114_ladder;
                    }
                    else
                    {
                        float frontier_phi_121_115_ladder;
                        float frontier_phi_121_115_ladder_1;
                        float frontier_phi_121_115_ladder_2;
                        float frontier_phi_121_115_ladder_3;
                        if (_1959)
                        {
                            float _2259 = _1589 * _1583;
                            float _2263 = rsqrt(dot(float3(_1586, _2259, _1592), float3(_1586, _2259, _1592)));
                            float4 _2271 = _28[4u].SampleLevel(_59, float3(_2263 * _1586, _2263 * _2259, _2263 * _1592), 0.0f);
                            frontier_phi_121_115_ladder = 1.0f;
                            frontier_phi_121_115_ladder_1 = _2271.z;
                            frontier_phi_121_115_ladder_2 = _2271.y;
                            frontier_phi_121_115_ladder_3 = _2271.x;
                        }
                        else
                        {
                            frontier_phi_121_115_ladder = 0.0f;
                            frontier_phi_121_115_ladder_1 = 0.0f;
                            frontier_phi_121_115_ladder_2 = 0.0f;
                            frontier_phi_121_115_ladder_3 = 0.0f;
                        }
                        _2248 = frontier_phi_121_115_ladder_3;
                        _2250 = frontier_phi_121_115_ladder_2;
                        _2252 = frontier_phi_121_115_ladder_1;
                        _2254 = frontier_phi_121_115_ladder;
                    }
                    _2356 = _2248 * _475;
                    _2358 = _2250 * _475;
                    _2360 = _2252 * _475;
                    _2362 = _2254 * _475;
                }
                else
                {
                    float _2288;
                    float _2290;
                    float _2295;
                    float _2299;
                    if (_12.Load(int3(uint2(uint(_1690 * _1126), uint(_1692 * _1124)), 0u)).x > 0.0f)
                    {
                        uint _2152_dummy_parameter;
                        uint2 _2152 = spvTextureSize(_14, 0u, _2152_dummy_parameter);
                        float4 _2161 = _14.Load(int3(uint2(uint(float(_2152.x) * _1126), uint(float(_2152.y) * _1124)), 0u));
                        float _2165 = _2161.x * 0.5f;
                        float _2166 = _2161.y * (-0.5f);
                        float4 _2185 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _2165) + (_50_m0[52u].x * _1126), (_50_m0[52u].w * _2166) + (_50_m0[52u].y * _1124)), 0.0f);
                        float _2284;
                        if (_1864)
                        {
                            float frontier_phi_124_123_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _944))) < 5.0f)
                            {
                                float _2416 = sqrt((_2165 * _2165) + (_2166 * _2166));
                                float frontier_phi_124_123_ladder_130_ladder;
                                if (_2416 > 0.0500000007450580596923828125f)
                                {
                                    float _2453 = _1126 - _951;
                                    float _2454 = _1124 - _952;
                                    float frontier_phi_124_123_ladder_130_ladder_140_ladder;
                                    if (_2416 > sqrt((_2454 * _2454) + (_2453 * _2453)))
                                    {
                                        uint4 _2542 = asuint(_55_m0[0u]);
                                        uint _2549 = uint(float(_2542.x) * _1126);
                                        uint _2550 = uint(float(_2542.y) * _1124);
                                        uint4 _2553 = _24[2u].Load(int3(uint2(_2549, _2550), 0u));
                                        uint _2556 = _2553.w;
                                        uint4 _2561 = _24[15u].Load(int3(uint2(_2549, _2550), 0u));
                                        uint _2563 = _2561.y;
                                        uint _2569 = ((_2563 & 64u) != 0u) ? uint((_2563 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2570 = _2556 & 128u;
                                        uint _2572 = (_2570 != 0u) ? 1u : ((_2553.x << 7u) | _2556);
                                        uint4 _2575 = _16.Load(_2572 * 4u);
                                        uint _2576 = _2575.x;
                                        uint _2583 = ((_2576 & 1u) != 0u) ? 0u : 18u;
                                        uint _2629;
                                        if (_2570 == 0u)
                                        {
                                            _2629 = (((_2576 & 2097152u) != 0u) && (_2569 == uint(min(int(uint(max(int(_2569), int(0u)))), int(1u))))) ? (_2583 | 128u) : _2583;
                                        }
                                        else
                                        {
                                            _2629 = _2556;
                                        }
                                        float frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder;
                                        if (((_2629 & 128u) | (_16.Load((_2572 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2652 = asuint(_55_m0[0u]);
                                            uint _2661 = uint(float(_2652.x) * (_2165 + _1126));
                                            uint _2662 = uint(float(_2652.y) * (_2166 + _1124));
                                            uint4 _2665 = _24[2u].Load(int3(uint2(_2661, _2662), 0u));
                                            uint _2668 = _2665.w;
                                            uint4 _2671 = _24[15u].Load(int3(uint2(_2661, _2662), 0u));
                                            uint _2673 = _2671.y;
                                            uint _2679 = ((_2673 & 64u) != 0u) ? uint((_2673 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2680 = _2668 & 128u;
                                            uint _2682 = (_2680 != 0u) ? 1u : ((_2665.x << 7u) | _2668);
                                            uint4 _2684 = _16.Load(_2682 * 4u);
                                            uint _2685 = _2684.x;
                                            uint _2692 = ((_2685 & 1u) != 0u) ? 0u : 18u;
                                            uint _2761;
                                            if (_2680 == 0u)
                                            {
                                                _2761 = (((_2685 & 2097152u) != 0u) && (_2679 == uint(min(int(uint(max(int(_2679), int(0u)))), int(1u))))) ? (_2692 | 128u) : _2692;
                                            }
                                            else
                                            {
                                                _2761 = _2668;
                                            }
                                            float frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder_165_ladder;
                                            if ((_2761 & 128u) == 0u)
                                            {
                                                frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder_165_ladder = ((_16.Load((_2682 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder_165_ladder = 0.0f;
                                            }
                                            frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder = frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder_165_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder = 1.0f;
                                        }
                                        frontier_phi_124_123_ladder_130_ladder_140_ladder = frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_124_123_ladder_130_ladder_140_ladder = 1.0f;
                                    }
                                    frontier_phi_124_123_ladder_130_ladder = frontier_phi_124_123_ladder_130_ladder_140_ladder;
                                }
                                else
                                {
                                    frontier_phi_124_123_ladder_130_ladder = 1.0f;
                                }
                                frontier_phi_124_123_ladder = frontier_phi_124_123_ladder_130_ladder;
                            }
                            else
                            {
                                frontier_phi_124_123_ladder = 1.0f;
                            }
                            _2284 = frontier_phi_124_123_ladder;
                        }
                        else
                        {
                            _2284 = 1.0f;
                        }
                        float _2286 = _2284 * _1958;
                        float _2294;
                        float _2298;
                        float _2302;
                        if (_1156 == 0u)
                        {
                            _2294 = _50_m0[54u].x * _2185.x;
                            _2298 = _50_m0[54u].x * _2185.y;
                            _2302 = _50_m0[54u].x * _2185.z;
                        }
                        else
                        {
                            _2294 = 0.0f;
                            _2298 = 0.0f;
                            _2302 = 0.0f;
                        }
                        float frontier_phi_125_131_ladder;
                        float frontier_phi_125_131_ladder_1;
                        float frontier_phi_125_131_ladder_2;
                        float frontier_phi_125_131_ladder_3;
                        for (;;)
                        {
                            if (_1154 > 0.0f)
                            {
                                float _2293;
                                float _2297;
                                float _2301;
                                if (_1864)
                                {
                                    _2293 = 0.0f;
                                    _2297 = 0.0f;
                                    _2301 = 0.0f;
                                }
                                else
                                {
                                    if (!((_397 != 4u) || (_1162 != 0u)))
                                    {
                                        frontier_phi_125_131_ladder = _2302;
                                        frontier_phi_125_131_ladder_1 = _2298;
                                        frontier_phi_125_131_ladder_2 = _2294;
                                        frontier_phi_125_131_ladder_3 = _2286;
                                        break;
                                    }
                                    _2293 = _2294;
                                    _2297 = _2298;
                                    _2301 = _2302;
                                }
                                frontier_phi_125_131_ladder = _2301;
                                frontier_phi_125_131_ladder_1 = _2297;
                                frontier_phi_125_131_ladder_2 = _2293;
                                frontier_phi_125_131_ladder_3 = (1.0f - exp2(log2(_1154) * 3.0f)) * _2286;
                                break;
                            }
                            else
                            {
                                frontier_phi_125_131_ladder = _2302;
                                frontier_phi_125_131_ladder_1 = _2298;
                                frontier_phi_125_131_ladder_2 = _2294;
                                frontier_phi_125_131_ladder_3 = _2286;
                                break;
                            }
                        }
                        _2288 = frontier_phi_125_131_ladder_3;
                        _2290 = frontier_phi_125_131_ladder_2;
                        _2295 = frontier_phi_125_131_ladder_1;
                        _2299 = frontier_phi_125_131_ladder;
                    }
                    else
                    {
                        float frontier_phi_125_117_ladder;
                        float frontier_phi_125_117_ladder_1;
                        float frontier_phi_125_117_ladder_2;
                        float frontier_phi_125_117_ladder_3;
                        if (_1461)
                        {
                            frontier_phi_125_117_ladder = _2292;
                            frontier_phi_125_117_ladder_1 = _2292;
                            frontier_phi_125_117_ladder_2 = _2292;
                            frontier_phi_125_117_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float4 _2307 = _28[7u].SampleLevel(_59, float3(_1586, _1589, _1592), 0.0f);
                            frontier_phi_125_117_ladder = _2307.z;
                            frontier_phi_125_117_ladder_1 = _2307.y;
                            frontier_phi_125_117_ladder_2 = _2307.x;
                            frontier_phi_125_117_ladder_3 = _1958;
                        }
                        _2288 = frontier_phi_125_117_ladder_3;
                        _2290 = frontier_phi_125_117_ladder_2;
                        _2295 = frontier_phi_125_117_ladder_1;
                        _2299 = frontier_phi_125_117_ladder;
                    }
                    float _2422;
                    float _2423;
                    float _2424;
                    float _2425;
                    if (_1669)
                    {
                        _2422 = _1956;
                        _2423 = _2290 * _1538;
                        _2424 = _2295 * _1538;
                        _2425 = _2299 * _1538;
                    }
                    else
                    {
                        _2422 = _2288;
                        _2423 = _2290;
                        _2424 = _2295;
                        _2425 = _2299;
                    }
                    float _2486;
                    float _2487;
                    float _2488;
                    float _2489;
                    if (_1959 && (_2422 < 1.0f))
                    {
                        float _2460 = _1589 * _1583;
                        float _2464 = rsqrt(dot(float3(_1586, _2460, _1592), float3(_1586, _2460, _1592)));
                        float4 _2472 = _28[4u].SampleLevel(_59, float3(_2464 * _1586, _2464 * _2460, _2464 * _1592), 0.0f);
                        float _2474 = _2472.x;
                        float _2475 = _2472.y;
                        float _2476 = _2472.z;
                        _2486 = 1.0f;
                        _2487 = ((_2423 - _2474) * _2422) + _2474;
                        _2488 = ((_2424 - _2475) * _2422) + _2475;
                        _2489 = ((_2425 - _2476) * _2422) + _2476;
                    }
                    else
                    {
                        _2486 = _2422;
                        _2487 = _2423;
                        _2488 = _2424;
                        _2489 = _2425;
                    }
                    float _2363 = _2486 * _475;
                    _2356 = _2487 * _2363;
                    _2358 = _2488 * _2363;
                    _2360 = _2489 * _2363;
                    _2362 = _2363;
                }
                float _2367 = _50_m0[58u].z * _1685;
                float _2389 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _2391 = _2389 * ((_2367 * ((_1681 * 1000.0f) - _2356)) + _2356);
                float _2392 = _2389 * ((_2367 * ((_1682 * 1000.0f) - _2358)) + _2358);
                float _2393 = _2389 * ((_2367 * (_1683 - _2360)) + _2360);
                float _2399 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2391, max(_2392, _2393)) + 1.0f);
                float _2403 = min(_2399 * _2391, 0.996078431606292724609375f);
                float _2405 = min(_2399 * _2392, 0.996078431606292724609375f);
                float _2406 = min(_2399 * _2393, 0.996078431606292724609375f);
                _35[uint2(_221, _224)] = float4(_2403, _2405, _2406, _2362);
                _39[uint2(_221, _224)] = float4(_1863, 0.0f, 0.0f, _1863);
                if (_228)
                {
                    uint _2446 = _221 + 1u;
                    _35[uint2(_2446, _224)] = float4(_2403, _2405, _2406, _2362);
                    _39[uint2(_2446, _224)] = float4(_1863, 0.0f, 0.0f, _1863);
                }
                if (_231)
                {
                    uint _2533 = _224 + 1u;
                    _35[uint2(_221, _2533)] = float4(_2403, _2405, _2406, _2362);
                    _39[uint2(_221, _2533)] = float4(_1863, 0.0f, 0.0f, _1863);
                }
                if (_232)
                {
                    uint _2613 = _221 + 1u;
                    uint _2614 = _224 + 1u;
                    _35[uint2(_2613, _2614)] = float4(_2403, _2405, _2406, _2362);
                    _39[uint2(_2613, _2614)] = float4(_1863, 0.0f, 0.0f, _1863);
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
