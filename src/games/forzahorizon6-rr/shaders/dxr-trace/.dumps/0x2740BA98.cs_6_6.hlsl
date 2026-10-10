static float _2293;
static uint _3190;
static float _3191;
static float _3192;
static uint _3193;
static float _3194;
static float _3195;
static float _3196;
static float _3197;
static float _3198;
static float _3199;
static float _3200;
static uint _3201;
static uint _3202;
static float _3203;
static float _3204;
static float _3205;
static float _3206;
static float _3207;
static uint _3208;
static float _3214;
static uint _3215;
static float _3216;
static float _3221;
static float _3222;
static float _3223;

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

static RayQuery<RAY_FLAG_NONE> _2703;

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
                        float _1108;
                        if (_793)
                        {
                            _1108 = _789;
                        }
                        else
                        {
                            _1108 = float(_590 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1192;
                        if ((_429 & 134217728u) == 0u)
                        {
                            uint frontier_phi_45_39_ladder;
                            if ((_591 != 0u) || ((_429 & 17825792u) == 1048576u))
                            {
                                frontier_phi_45_39_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_45_39_ladder = _783;
                            }
                            _1192 = frontier_phi_45_39_ladder;
                        }
                        else
                        {
                            _1192 = _783;
                        }
                        uint4 _1195 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _1197 = _1195.x;
                        float _1265;
                        float _1267;
                        float _1269;
                        if (_782 == 0u)
                        {
                            _1265 = 0.0f;
                            _1267 = 0.0f;
                            _1269 = 0.0f;
                        }
                        else
                        {
                            float4 _1274 = _20[8u].Load(int3(uint2(_265, _266), 0u));
                            _1265 = _1274.x;
                            _1267 = _1274.y;
                            _1269 = _1274.z;
                        }
                        uint _1302;
                        if (_1192 == 0u)
                        {
                            _1302 = 0u;
                        }
                        else
                        {
                            _1302 = _24[9u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        uint _1333;
                        if (_784 == 0u)
                        {
                            _1333 = 0u;
                        }
                        else
                        {
                            _1333 = _24[10u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        float _1343 = (float((_1197 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1344 = (float(_1197 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1348 = (1.0f - abs(_1343)) - abs(_1344);
                        float _1350 = clamp((-0.0f) - _1348, 0.0f, 1.0f);
                        float _1351 = (-0.0f) - _1350;
                        float _1356 = ((_1343 >= 0.0f) ? _1351 : _1350) + _1343;
                        float _1357 = ((_1344 >= 0.0f) ? _1351 : _1350) + _1344;
                        float _1361 = rsqrt(dot(float3(_1356, _1357, _1348), float3(_1356, _1357, _1348)));
                        float _1362 = _1356 * _1361;
                        float _1363 = _1357 * _1361;
                        float _1364 = _1361 * _1348;
                        float _795 = float(_1197 & 255u);
                        float _1368 = ((_429 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1451;
                        float _1452;
                        float _1453;
                        float _1454;
                        uint _1455;
                        if ((_380 & 64u) == 0u)
                        {
                            float frontier_phi_74_67_ladder;
                            float frontier_phi_74_67_ladder_1;
                            float frontier_phi_74_67_ladder_2;
                            float frontier_phi_74_67_ladder_3;
                            uint frontier_phi_74_67_ladder_4;
                            if ((_429 & 276824064u) == 0u)
                            {
                                frontier_phi_74_67_ladder = 0.0f;
                                frontier_phi_74_67_ladder_1 = ((_429 & 8u) != 0u) ? _1267 : _1368;
                                frontier_phi_74_67_ladder_2 = 0.0f;
                                frontier_phi_74_67_ladder_3 = 0.0f;
                                frontier_phi_74_67_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_74_67_ladder = 0.0f;
                                frontier_phi_74_67_ladder_1 = _1368;
                                frontier_phi_74_67_ladder_2 = 0.0f;
                                frontier_phi_74_67_ladder_3 = 0.0f;
                                frontier_phi_74_67_ladder_4 = 0u;
                            }
                            _1451 = frontier_phi_74_67_ladder_1;
                            _1452 = frontier_phi_74_67_ladder_2;
                            _1453 = frontier_phi_74_67_ladder;
                            _1454 = frontier_phi_74_67_ladder_3;
                            _1455 = frontier_phi_74_67_ladder_4;
                        }
                        else
                        {
                            float _1403 = (_1267 * 2.0f) + (-1.0f);
                            float _1404 = (_1269 * 2.0f) + (-1.0f);
                            float _1408 = (1.0f - abs(_1403)) - abs(_1404);
                            float _1410 = clamp((-0.0f) - _1408, 0.0f, 1.0f);
                            float _1411 = (-0.0f) - _1410;
                            float _1416 = ((_1403 >= 0.0f) ? _1411 : _1410) + _1403;
                            float _1417 = ((_1404 >= 0.0f) ? _1411 : _1410) + _1404;
                            float _1421 = rsqrt(dot(float3(_1416, _1417, _1408), float3(_1416, _1417, _1408)));
                            _1451 = floor(round(_1265 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1452 = _1416 * _1421;
                            _1453 = _1417 * _1421;
                            _1454 = _1421 * _1408;
                            _1455 = 1u;
                        }
                        float _803;
                        if ((_429 & 32768u) == 0u)
                        {
                            _803 = _1451;
                        }
                        else
                        {
                            float frontier_phi_76_77_ladder;
                            if (_17.Load((_384 * 115u) + 36u).x == 0u)
                            {
                                float _1656 = clamp((_795 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _580;
                                frontier_phi_76_77_ladder = ((_429 & 131072u) != 0u) ? _1656 : ((((clamp((1.21000003814697265625f / (exp2((_1108 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_384 * 115u) + 32u).x)) + 1.0f) * _1656);
                            }
                            else
                            {
                                frontier_phi_76_77_ladder = _580;
                            }
                            _803 = frontier_phi_76_77_ladder;
                        }
                        uint _1525 = _428 & 1u;
                        float _1599;
                        float _1601;
                        float _1603;
                        uint _1605;
                        if (((_429 & 16u) == 0u) || (((_1525 | (_380 & 8u)) | (_429 & 16777216u)) != 0u))
                        {
                            _1599 = _1452;
                            _1601 = _1453;
                            _1603 = _1454;
                            _1605 = _1455;
                        }
                        else
                        {
                            float _1615 = (float(_1302 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1616 = (float(_1302 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1620 = (1.0f - abs(_1615)) - abs(_1616);
                            float _1622 = clamp((-0.0f) - _1620, 0.0f, 1.0f);
                            float _1623 = (-0.0f) - _1622;
                            float _1628 = ((_1615 >= 0.0f) ? _1623 : _1622) + _1615;
                            float _1629 = ((_1616 >= 0.0f) ? _1623 : _1622) + _1616;
                            float _1633 = rsqrt(dot(float3(_1628, _1629, _1620), float3(_1628, _1629, _1620)));
                            _1599 = _1628 * _1633;
                            _1601 = _1629 * _1633;
                            _1603 = _1633 * _1620;
                            _1605 = 1u;
                        }
                        float _797;
                        float _799;
                        float _801;
                        if (_1525 == 0u)
                        {
                            float frontier_phi_87_86_ladder;
                            float frontier_phi_87_86_ladder_1;
                            float frontier_phi_87_86_ladder_2;
                            if (((_428 & 64u) == 0u) && (_785 != 0u))
                            {
                                float2 _1893 = spvUnpackHalf2x16((_1333 >> 17u) & 32736u);
                                float _1894 = _1893.x;
                                float _1897 = (spvUnpackHalf2x16((_1333 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1898 = (spvUnpackHalf2x16((_1333 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1902 = (1.0f - abs(_1897)) - abs(_1898);
                                float _1904 = clamp((-0.0f) - _1902, 0.0f, 1.0f);
                                float _1905 = (-0.0f) - _1904;
                                float _1910 = ((_1897 >= 0.0f) ? _1905 : _1904) + _1897;
                                float _1911 = ((_1898 >= 0.0f) ? _1905 : _1904) + _1898;
                                float _1915 = rsqrt(dot(float3(_1910, _1911, _1902), float3(_1910, _1911, _1902)));
                                float _1925 = (((_1910 * _1915) - _1362) * _1894) + _1362;
                                float _1926 = (((_1911 * _1915) - _1363) * _1894) + _1363;
                                float _1927 = (((_1915 * _1902) - _1364) * _1894) + _1364;
                                float _1931 = rsqrt(dot(float3(_1925, _1926, _1927), float3(_1925, _1926, _1927)));
                                frontier_phi_87_86_ladder = _1925 * _1931;
                                frontier_phi_87_86_ladder_1 = _1926 * _1931;
                                frontier_phi_87_86_ladder_2 = _1927 * _1931;
                            }
                            else
                            {
                                frontier_phi_87_86_ladder = _1362;
                                frontier_phi_87_86_ladder_1 = _1363;
                                frontier_phi_87_86_ladder_2 = _1364;
                            }
                            _797 = frontier_phi_87_86_ladder;
                            _799 = frontier_phi_87_86_ladder_1;
                            _801 = frontier_phi_87_86_ladder_2;
                        }
                        else
                        {
                            _797 = _1362;
                            _799 = _1363;
                            _801 = _1364;
                        }
                        float _811;
                        float _813;
                        float _815;
                        float _817;
                        if (_793)
                        {
                            float frontier_phi_95_94_ladder;
                            float frontier_phi_95_94_ladder_1;
                            float frontier_phi_95_94_ladder_2;
                            float frontier_phi_95_94_ladder_3;
                            if (((_429 & 33554432u) == 0u) || (((_380 & 4u) != 0u) && ((_429 & 8388608u) == 0u)))
                            {
                                frontier_phi_95_94_ladder = 0.0f;
                                frontier_phi_95_94_ladder_1 = 0.0f;
                                frontier_phi_95_94_ladder_2 = 0.0f;
                                frontier_phi_95_94_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _1975 = (spvUnpackHalf2x16((_1333 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1976 = (spvUnpackHalf2x16((_1333 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1980 = (1.0f - abs(_1975)) - abs(_1976);
                                float _1982 = clamp((-0.0f) - _1980, 0.0f, 1.0f);
                                float _1983 = (-0.0f) - _1982;
                                float _1988 = ((_1975 >= 0.0f) ? _1983 : _1982) + _1975;
                                float _1989 = ((_1976 >= 0.0f) ? _1983 : _1982) + _1976;
                                float _1993 = rsqrt(dot(float3(_1988, _1989, _1980), float3(_1988, _1989, _1980)));
                                float _1994 = _1988 * _1993;
                                float _1995 = _1989 * _1993;
                                float _1996 = _1993 * _1980;
                                float _2000 = rsqrt(dot(float3(_1994, _1995, _1996), float3(_1994, _1995, _1996)));
                                frontier_phi_95_94_ladder = _2000 * _1996;
                                frontier_phi_95_94_ladder_1 = _2000 * _1995;
                                frontier_phi_95_94_ladder_2 = _2000 * _1994;
                                frontier_phi_95_94_ladder_3 = spvUnpackHalf2x16((_1333 >> 17u) & 32736u).x;
                            }
                            _811 = frontier_phi_95_94_ladder_3;
                            _813 = frontier_phi_95_94_ladder_2;
                            _815 = frontier_phi_95_94_ladder_1;
                            _817 = frontier_phi_95_94_ladder;
                        }
                        else
                        {
                            _811 = 0.0f;
                            _813 = 0.0f;
                            _815 = 0.0f;
                            _817 = 0.0f;
                        }
                        bool _1945 = _1605 != 0u;
                        _794 = _795;
                        _796 = _797;
                        _798 = _799;
                        _800 = _801;
                        _802 = _803;
                        _804 = _1945 ? _1599 : _797;
                        _806 = _1945 ? _1601 : _799;
                        _808 = _1945 ? _1603 : _801;
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
                    bool _1117 = ((_428 & 128u) | _438) != 0u;
                    uint _1175;
                    if (_1117)
                    {
                        _1175 = 1u;
                    }
                    else
                    {
                        _1175 = (((_429 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _392;
                    float _394;
                    float _396;
                    uint _398;
                    float _402;
                    if (((_429 & 33554432u) == 0u) || _1117)
                    {
                        bool _1199 = _77 != 0u;
                        uint _1206;
                        if ((_429 & 16u) == 0u)
                        {
                            _1206 = _1175;
                        }
                        else
                        {
                            _1206 = ((_429 & 268435456u) != 0u) ? _1175 : 2u;
                        }
                        _402 = _802 * _818;
                        _392 = _1199 ? _804 : _796;
                        _394 = _1199 ? _806 : _798;
                        _396 = _1199 ? _808 : _800;
                        _398 = _1206;
                    }
                    else
                    {
                        _402 = _810;
                        _392 = _812;
                        _394 = _814;
                        _396 = _816;
                        _398 = _1175;
                    }
                    uint _1207 = _398 + 102u;
                    float _1216 = clamp((_402 - _45_m0[_1207].x) / (_45_m0[_1207].y - _45_m0[_1207].x), 0.0f, 1.0f);
                    _391 = _392;
                    _393 = _394;
                    _395 = _396;
                    _397 = _398;
                    _399 = (_1216 * _1216) * (3.0f - (_1216 * 2.0f));
                    _401 = _402;
                    _403 = _45_m0[_1207].z;
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
                float _890 = rsqrt(dot(float3(_765, _768, _771), float3(_765, _768, _771)));
                float _891 = _890 * _765;
                float _892 = _890 * _768;
                float _893 = _890 * _771;
                float _897 = dot(float3(_512, _513, _514), float3(_891, _892, _893)) * 2.0f;
                float _901 = _512 - (_897 * _891);
                float _902 = _513 - (_897 * _892);
                float _903 = _514 - (_897 * _893);
                float _913 = sqrt(((_506 * _506) + (_505 * _505)) + (_507 * _507)) * 0.001000000047497451305389404296875f;
                float _924 = ((_913 * _517) + _505) + (_901 * _403);
                float _925 = ((_913 * _520) + _506) + (_902 * _403);
                float _926 = ((_913 * _523) + _507) + (_903 * _403);
                float _942 = mad(_116, _926, mad(_109, _925, _924 * _102)) + _123;
                float _945 = (mad(_115, _926, mad(_108, _925, _924 * _101)) + _122) / _942;
                float _948 = (((mad(_113, _926, mad(_106, _925, _924 * _99)) + _120) / _942) * 0.5f) + 0.5f;
                float _949 = 0.5f - (((mad(_114, _926, mad(_107, _925, _924 * _100)) + _121) / _942) * 0.5f);
                float _952 = _948 * _50_m0[51u].x;
                float _953 = _949 * _50_m0[51u].y;
                float _958 = _924 + (_901 * 0.100000001490116119384765625f);
                float _959 = _925 + (_902 * 0.100000001490116119384765625f);
                float _960 = _926 + (_903 * 0.100000001490116119384765625f);
                float _976 = mad(_116, _960, mad(_109, _959, _958 * _102)) + _123;
                float _985 = _50_m0[51u].x * (((((mad(_113, _960, mad(_106, _959, _958 * _99)) + _120) / _976) * 0.5f) + 0.5f) - _948);
                float _987 = _50_m0[51u].y * ((0.5f - (((mad(_114, _960, mad(_107, _959, _958 * _100)) + _121) / _976) * 0.5f)) - _949);
                float _988 = ((mad(_115, _960, mad(_108, _959, _958 * _101)) + _122) / _976) - _945;
                float _989 = _985 * 10.0f;
                float _991 = _987 * 10.0f;
                float _992 = _988 * 10.0f;
                float _996 = 0.75f / dot(float3(_901, _902, _903), float3(_517, _520, _523));
                float _1001 = (_996 * _901) + _924;
                float _1002 = (_996 * _902) + _925;
                float _1003 = (_996 * _903) + _926;
                float _1019 = mad(_116, _1003, mad(_109, _1002, _1001 * _102)) + _123;
                float _1028 = _50_m0[51u].x * (((((mad(_113, _1003, mad(_106, _1002, _1001 * _99)) + _120) / _1019) * 0.5f) + 0.5f) - _948);
                float _1030 = _50_m0[51u].y * ((0.5f - (((mad(_114, _1003, mad(_107, _1002, _1001 * _100)) + _121) / _1019) * 0.5f)) - _949);
                float _1031 = ((mad(_115, _1003, mad(_108, _1002, _1001 * _101)) + _122) / _1019) - _945;
                float _1044 = sqrt(((_1028 * _1028) + (_1031 * _1031)) + (_1030 * _1030)) / sqrt(((_989 * _989) + (_992 * _992)) + (_991 * _991));
                float _1045 = float(_233);
                float _1046 = float(_234);
                float _1053 = (_989 != 0.0f) ? (0.100000001490116119384765625f / _985) : 3.4028234663852885981170418348452e+38f;
                float _1055 = (_991 != 0.0f) ? (0.100000001490116119384765625f / _987) : 3.4028234663852885981170418348452e+38f;
                float _1056 = (_992 != 0.0f) ? (0.100000001490116119384765625f / _988) : 3.4028234663852885981170418348452e+38f;
                float _1057 = 1.0f / _1045;
                float _1058 = 1.0f / _1046;
                float _1059 = 0.004999999888241291046142578125f / _1045;
                float _1061 = 0.004999999888241291046142578125f / _1046;
                float _1070 = float(_989 >= 0.0f);
                float _1071 = float(_991 >= 0.0f);
                float _1080 = ((_989 < 0.0f) ? ((-0.0f) - _1059) : _1059) - _952;
                float _1083 = ((_991 < 0.0f) ? ((-0.0f) - _1061) : _1061) - _953;
                float _1086 = min((((floor(_952 * _1045) + _1070) * _1057) + _1080) * _1053, (((floor(_953 * _1046) + _1071) * _1058) + _1083) * _1055);
                float _1090 = (_1086 * _989) + _952;
                float _1091 = (_1086 * _991) + _953;
                float _1092 = (_1086 * _992) + _945;
                float _1095 = _50_m0[50u].x / (_459 - (_1092 * _50_m0[50u].y));
                float _1106 = max(0.300000011920928955078125f, 10.0f / max(0.00999999977648258209228515625f, max(abs(_985 * 38400.0f), abs(_987 * 21600.0f))));
                uint _1119;
                float _1123;
                float _1125;
                float _1127;
                uint _1139;
                uint _1141;
                uint _1121;
                float _1129;
                float _1131;
                float _1133;
                float _1135;
                float _1137;
                float _1143;
                float _1145;
                float _1147;
                float _1149;
                float _1151;
                float _1153;
                float _1155;
                uint _1157;
                float _1159;
                float _1161;
                uint _1163;
                uint _1118 = 0u;
                uint _1120 = 0u;
                float _1122 = _1092;
                float _1124 = _1091;
                float _1126 = _1090;
                float _1128 = _1086;
                float _1130 = _1058;
                float _1132 = _1057;
                float _1134 = _1046;
                float _1136 = _1045;
                uint _1138 = 0u;
                uint _1140 = 0u;
                float _1142 = _945;
                float _1144 = _953;
                float _1146 = _952;
                float _1148 = _1092;
                float _1150 = _1091;
                float _1152 = _1090;
                float _1154 = 1.0f;
                uint _1156 = 0u;
                float _1158 = 0.0f;
                float _1160 = 0.0f;
                uint _1162 = 1u;
                float _1164;
                float _1165;
                uint _1166;
                uint _1167;
                bool _1168;
                for (;;)
                {
                    _1164 = _1136 * _1126;
                    _1165 = _1134 * _1124;
                    _1166 = uint(int(_1164));
                    _1167 = uint(int(_1165));
                    _1168 = _1138 == 0u;
                    float _1221;
                    if (_1168)
                    {
                        _1221 = _12.Load(int3(uint2(_1166, _1167), 0u)).x;
                    }
                    else
                    {
                        _1221 = _15.Load(int3(uint2(_1166, _1167), _1138 + 4294967295u)).x;
                    }
                    float _1227 = ((_1164 >= floor(_1136)) || (_1165 >= floor(_1134))) ? 1.0f : _1221;
                    float _1241 = (_992 < 0.0f) ? ((_1227 - _945) * _1056) : 3.4028234663852885981170418348452e+38f;
                    float _1243 = min(min((((floor(_1164) + _1070) * _1132) + _1080) * _1053, (((floor(_1165) + _1071) * _1130) + _1083) * _1055), _1241);
                    bool _1244 = _1227 < _1122;
                    bool _1248 = _1244 && (asuint(_1243) != asuint(_1241));
                    float _1249 = _1244 ? _1243 : _1128;
                    float _1253 = (_1249 * _989) + _952;
                    float _1254 = (_1249 * _991) + _953;
                    float _1255 = (_1249 * _992) + _945;
                    uint _1257 = (_1248 ? 1u : 4294967295u) + _1138;
                    float _1258 = _1248 ? 0.5f : 2.0f;
                    float _1259 = _1258 * _1136;
                    float _1260 = _1258 * _1134;
                    float _1261 = _1248 ? 2.0f : 0.5f;
                    float _1262 = _1261 * _1132;
                    float _1263 = _1261 * _1130;
                    _1119 = _1118 + 1u;
                    uint _1322;
                    uint _1325;
                    if (int(_1257) < int(0u))
                    {
                        float frontier_phi_59_53_ladder;
                        uint frontier_phi_59_53_ladder_1;
                        float frontier_phi_59_53_ladder_2;
                        uint frontier_phi_59_53_ladder_3;
                        float frontier_phi_59_53_ladder_4;
                        float frontier_phi_59_53_ladder_5;
                        float frontier_phi_59_53_ladder_6;
                        float frontier_phi_59_53_ladder_7;
                        float frontier_phi_59_53_ladder_8;
                        float frontier_phi_59_53_ladder_9;
                        uint frontier_phi_59_53_ladder_10;
                        uint frontier_phi_59_53_ladder_11;
                        float frontier_phi_59_53_ladder_12;
                        float frontier_phi_59_53_ladder_13;
                        float frontier_phi_59_53_ladder_14;
                        float frontier_phi_59_53_ladder_15;
                        float frontier_phi_59_53_ladder_16;
                        uint frontier_phi_59_53_ladder_17;
                        float frontier_phi_59_53_ladder_18;
                        float _1288;
                        float _1293;
                        float _1296;
                        bool _1297;
                        for (;;)
                        {
                            float _1286 = _50_m0[50u].w + _50_m0[50u].y;
                            _1288 = _50_m0[50u].x / (_1286 - (_50_m0[50u].y * _1227));
                            float _1291 = _50_m0[50u].x / (_1286 - (_50_m0[50u].y * _1255));
                            _1293 = abs(_1095 - _1291);
                            _1296 = _1291 - _1288;
                            _1297 = _1296 > max(0.00999999977648258209228515625f, _1293 * 0.00999999977648258209228515625f);
                            if (_1297)
                            {
                                uint _1327;
                                if (_1120 == 0u)
                                {
                                    uint frontier_phi_63_62_ladder;
                                    if ((_397 == 2u) || (_397 == 4u))
                                    {
                                        if ((_1249 < _1044) && (abs(_1296) < 2.0f))
                                        {
                                            frontier_phi_59_53_ladder = _1254;
                                            frontier_phi_59_53_ladder_1 = _1162;
                                            frontier_phi_59_53_ladder_2 = _1253;
                                            frontier_phi_59_53_ladder_3 = 1u;
                                            frontier_phi_59_53_ladder_4 = 0.0f;
                                            frontier_phi_59_53_ladder_5 = _1152;
                                            frontier_phi_59_53_ladder_6 = _1150;
                                            frontier_phi_59_53_ladder_7 = _1148;
                                            frontier_phi_59_53_ladder_8 = _1146;
                                            frontier_phi_59_53_ladder_9 = _1144;
                                            frontier_phi_59_53_ladder_10 = 1u;
                                            frontier_phi_59_53_ladder_11 = _1257;
                                            frontier_phi_59_53_ladder_12 = _1259;
                                            frontier_phi_59_53_ladder_13 = _1260;
                                            frontier_phi_59_53_ladder_14 = _1262;
                                            frontier_phi_59_53_ladder_15 = _1263;
                                            frontier_phi_59_53_ladder_16 = _1249;
                                            frontier_phi_59_53_ladder_17 = 1u;
                                            frontier_phi_59_53_ladder_18 = _1142;
                                            break;
                                        }
                                        frontier_phi_63_62_ladder = 1u;
                                    }
                                    else
                                    {
                                        frontier_phi_63_62_ladder = 1u;
                                    }
                                    _1327 = frontier_phi_63_62_ladder;
                                }
                                else
                                {
                                    _1327 = _1120;
                                }
                                if (!(_1156 == 0u))
                                {
                                    frontier_phi_59_53_ladder = _1158;
                                    frontier_phi_59_53_ladder_1 = _1162;
                                    frontier_phi_59_53_ladder_2 = _1160;
                                    frontier_phi_59_53_ladder_3 = _1156;
                                    frontier_phi_59_53_ladder_4 = _1154;
                                    frontier_phi_59_53_ladder_5 = _1152;
                                    frontier_phi_59_53_ladder_6 = _1150;
                                    frontier_phi_59_53_ladder_7 = _1148;
                                    frontier_phi_59_53_ladder_8 = _1146;
                                    frontier_phi_59_53_ladder_9 = _1144;
                                    frontier_phi_59_53_ladder_10 = _1140;
                                    frontier_phi_59_53_ladder_11 = _1257;
                                    frontier_phi_59_53_ladder_12 = _1259;
                                    frontier_phi_59_53_ladder_13 = _1260;
                                    frontier_phi_59_53_ladder_14 = _1262;
                                    frontier_phi_59_53_ladder_15 = _1263;
                                    frontier_phi_59_53_ladder_16 = _1249;
                                    frontier_phi_59_53_ladder_17 = _1327;
                                    frontier_phi_59_53_ladder_18 = _1142;
                                    break;
                                }
                                bool _1429 = _1140 != 0u;
                                frontier_phi_59_53_ladder = _1429 ? _1158 : _1254;
                                frontier_phi_59_53_ladder_1 = _1162;
                                frontier_phi_59_53_ladder_2 = _1429 ? _1160 : _1253;
                                frontier_phi_59_53_ladder_3 = 0u;
                                frontier_phi_59_53_ladder_4 = _1154;
                                frontier_phi_59_53_ladder_5 = _1152;
                                frontier_phi_59_53_ladder_6 = _1150;
                                frontier_phi_59_53_ladder_7 = _1148;
                                frontier_phi_59_53_ladder_8 = _1146;
                                frontier_phi_59_53_ladder_9 = _1144;
                                frontier_phi_59_53_ladder_10 = ((_397 == 1u) || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1140;
                                frontier_phi_59_53_ladder_11 = 0u;
                                frontier_phi_59_53_ladder_12 = _1045;
                                frontier_phi_59_53_ladder_13 = _1046;
                                frontier_phi_59_53_ladder_14 = _1057;
                                frontier_phi_59_53_ladder_15 = _1058;
                                frontier_phi_59_53_ladder_16 = _1249 + _1106;
                                frontier_phi_59_53_ladder_17 = _1327;
                                frontier_phi_59_53_ladder_18 = _1142;
                                break;
                            }
                            else
                            {
                                float _1313 = max(0.100000001490116119384765625f, _1293 * 0.100000001490116119384765625f) * 0.5f;
                                float _1316 = clamp((abs(_1296) - _1313) / _1313, 0.0f, 1.0f);
                                uint _1318 = uint(_1288 < _1095);
                                float frontier_phi_59_53_ladder_58_ladder;
                                uint frontier_phi_59_53_ladder_58_ladder_1;
                                float frontier_phi_59_53_ladder_58_ladder_2;
                                uint frontier_phi_59_53_ladder_58_ladder_3;
                                float frontier_phi_59_53_ladder_58_ladder_4;
                                float frontier_phi_59_53_ladder_58_ladder_5;
                                float frontier_phi_59_53_ladder_58_ladder_6;
                                float frontier_phi_59_53_ladder_58_ladder_7;
                                float frontier_phi_59_53_ladder_58_ladder_8;
                                float frontier_phi_59_53_ladder_58_ladder_9;
                                uint frontier_phi_59_53_ladder_58_ladder_10;
                                uint frontier_phi_59_53_ladder_58_ladder_11;
                                float frontier_phi_59_53_ladder_58_ladder_12;
                                float frontier_phi_59_53_ladder_58_ladder_13;
                                float frontier_phi_59_53_ladder_58_ladder_14;
                                float frontier_phi_59_53_ladder_58_ladder_15;
                                float frontier_phi_59_53_ladder_58_ladder_16;
                                uint frontier_phi_59_53_ladder_58_ladder_17;
                                float frontier_phi_59_53_ladder_58_ladder_18;
                                if (_1140 == 0u)
                                {
                                    frontier_phi_59_53_ladder_58_ladder = _1158;
                                    frontier_phi_59_53_ladder_58_ladder_1 = _1318;
                                    frontier_phi_59_53_ladder_58_ladder_2 = _1160;
                                    frontier_phi_59_53_ladder_58_ladder_3 = _1156;
                                    frontier_phi_59_53_ladder_58_ladder_4 = _1316;
                                    frontier_phi_59_53_ladder_58_ladder_5 = _1152;
                                    frontier_phi_59_53_ladder_58_ladder_6 = _1150;
                                    frontier_phi_59_53_ladder_58_ladder_7 = _1148;
                                    frontier_phi_59_53_ladder_58_ladder_8 = _1146;
                                    frontier_phi_59_53_ladder_58_ladder_9 = _1144;
                                    frontier_phi_59_53_ladder_58_ladder_10 = uint(_1316 > 0.0f);
                                    frontier_phi_59_53_ladder_58_ladder_11 = _1257;
                                    frontier_phi_59_53_ladder_58_ladder_12 = _1259;
                                    frontier_phi_59_53_ladder_58_ladder_13 = _1260;
                                    frontier_phi_59_53_ladder_58_ladder_14 = _1262;
                                    frontier_phi_59_53_ladder_58_ladder_15 = _1263;
                                    frontier_phi_59_53_ladder_58_ladder_16 = _1249;
                                    frontier_phi_59_53_ladder_58_ladder_17 = _1120;
                                    frontier_phi_59_53_ladder_58_ladder_18 = _1142;
                                }
                                else
                                {
                                    frontier_phi_59_53_ladder_58_ladder = _1158;
                                    frontier_phi_59_53_ladder_58_ladder_1 = _1318;
                                    frontier_phi_59_53_ladder_58_ladder_2 = _1160;
                                    frontier_phi_59_53_ladder_58_ladder_3 = _1156;
                                    frontier_phi_59_53_ladder_58_ladder_4 = _1316;
                                    frontier_phi_59_53_ladder_58_ladder_5 = _1152;
                                    frontier_phi_59_53_ladder_58_ladder_6 = _1150;
                                    frontier_phi_59_53_ladder_58_ladder_7 = _1148;
                                    frontier_phi_59_53_ladder_58_ladder_8 = _1146;
                                    frontier_phi_59_53_ladder_58_ladder_9 = _1144;
                                    frontier_phi_59_53_ladder_58_ladder_10 = _1140;
                                    frontier_phi_59_53_ladder_58_ladder_11 = _1257;
                                    frontier_phi_59_53_ladder_58_ladder_12 = _1259;
                                    frontier_phi_59_53_ladder_58_ladder_13 = _1260;
                                    frontier_phi_59_53_ladder_58_ladder_14 = _1262;
                                    frontier_phi_59_53_ladder_58_ladder_15 = _1263;
                                    frontier_phi_59_53_ladder_58_ladder_16 = _1249;
                                    frontier_phi_59_53_ladder_58_ladder_17 = _1120;
                                    frontier_phi_59_53_ladder_58_ladder_18 = _1142;
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
                        _1163 = frontier_phi_59_53_ladder_1;
                        _1161 = frontier_phi_59_53_ladder_2;
                        _1159 = frontier_phi_59_53_ladder;
                        _1157 = frontier_phi_59_53_ladder_3;
                        _1155 = frontier_phi_59_53_ladder_4;
                        _1153 = frontier_phi_59_53_ladder_5;
                        _1151 = frontier_phi_59_53_ladder_6;
                        _1149 = frontier_phi_59_53_ladder_7;
                        _1147 = frontier_phi_59_53_ladder_8;
                        _1145 = frontier_phi_59_53_ladder_9;
                        _1143 = frontier_phi_59_53_ladder_18;
                        _1322 = frontier_phi_59_53_ladder_10;
                        _1325 = frontier_phi_59_53_ladder_11;
                        _1137 = frontier_phi_59_53_ladder_12;
                        _1135 = frontier_phi_59_53_ladder_13;
                        _1133 = frontier_phi_59_53_ladder_14;
                        _1131 = frontier_phi_59_53_ladder_15;
                        _1129 = frontier_phi_59_53_ladder_16;
                        _1121 = frontier_phi_59_53_ladder_17;
                    }
                    else
                    {
                        bool _1298 = _1140 != 0u;
                        _1163 = _1162;
                        _1161 = _1160;
                        _1159 = _1158;
                        _1157 = _1156;
                        _1155 = _1154;
                        _1153 = _1298 ? _1152 : _1253;
                        _1151 = _1298 ? _1150 : _1254;
                        _1149 = _1298 ? _1148 : _1255;
                        _1147 = _1253;
                        _1145 = _1254;
                        _1143 = _1255;
                        _1322 = _1140;
                        _1325 = _1257;
                        _1137 = _1259;
                        _1135 = _1260;
                        _1133 = _1262;
                        _1131 = _1263;
                        _1129 = _1249;
                        _1121 = _1120;
                    }
                    float frontier_phi_72_pred;
                    float frontier_phi_72_pred_1;
                    float frontier_phi_72_pred_2;
                    uint frontier_phi_72_pred_3;
                    uint frontier_phi_72_pred_4;
                    bool _1330;
                    bool _1332;
                    for (;;)
                    {
                        _1330 = _1255 < 0.0f;
                        _1332 = _1330 || ((_1253 < 0.0f) || (_1254 < 0.0f));
                        if (!_1332)
                        {
                            if (!((_1255 > 1.0f) || ((_1253 > _50_m0[51u].x) || (_1254 > _50_m0[51u].y))))
                            {
                                frontier_phi_72_pred = _1255;
                                frontier_phi_72_pred_1 = _1254;
                                frontier_phi_72_pred_2 = _1253;
                                frontier_phi_72_pred_3 = _1325;
                                frontier_phi_72_pred_4 = _1322;
                                break;
                            }
                        }
                        if (!_1330)
                        {
                            frontier_phi_72_pred = _1255;
                            frontier_phi_72_pred_1 = _1254;
                            frontier_phi_72_pred_2 = _1253;
                            frontier_phi_72_pred_3 = 4294967295u;
                            frontier_phi_72_pred_4 = 1u;
                            break;
                        }
                        float _1438 = (-0.0f) - _1255;
                        float _1439 = _1438 / _992;
                        frontier_phi_72_pred = _1438 + _1255;
                        frontier_phi_72_pred_1 = (_1439 * _991) + _1254;
                        frontier_phi_72_pred_2 = (_1439 * _989) + _1253;
                        frontier_phi_72_pred_3 = 4294967295u;
                        frontier_phi_72_pred_4 = 1u;
                        break;
                    }
                    _1123 = frontier_phi_72_pred;
                    _1125 = frontier_phi_72_pred_1;
                    _1127 = frontier_phi_72_pred_2;
                    _1139 = frontier_phi_72_pred_3;
                    _1141 = frontier_phi_72_pred_4;
                    if ((_1119 < 128u) && (int(_1139) > int(4294967295u)))
                    {
                        _1118 = _1119;
                        _1120 = _1121;
                        _1122 = _1123;
                        _1124 = _1125;
                        _1126 = _1127;
                        _1128 = _1129;
                        _1130 = _1131;
                        _1132 = _1133;
                        _1134 = _1135;
                        _1136 = _1137;
                        _1138 = _1139;
                        _1140 = _1141;
                        _1142 = _1143;
                        _1144 = _1145;
                        _1146 = _1147;
                        _1148 = _1149;
                        _1150 = _1151;
                        _1152 = _1153;
                        _1154 = _1155;
                        _1156 = _1157;
                        _1158 = _1159;
                        _1160 = _1161;
                        _1162 = _1163;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                bool _1461 = dot(float3(_901, _902, _903), float3(_512, _513, _514)) < 0.0f;
                bool _1462 = _1119 > 127u;
                uint _1463 = _1462 ? 1u : _1141;
                float _1472 = _50_m0[51u].z * 2.0f;
                float _1475 = (_1472 * _952) + (-1.0f);
                float _1476 = ((1.0f - (_50_m0[51u].w * _953)) * 2.0f) + (-1.0f);
                float _1492 = mad(_190, _945, mad(_183, _1476, _1475 * _176)) + _197;
                float _1493 = (mad(_187, _945, mad(_180, _1476, _1475 * _173)) + _194) / _1492;
                float _1494 = (mad(_188, _945, mad(_181, _1476, _1475 * _174)) + _195) / _1492;
                float _1495 = (mad(_189, _945, mad(_182, _1476, _1475 * _175)) + _196) / _1492;
                float _1500 = (_1472 * _1127) + (-1.0f);
                float _1501 = ((1.0f - (_50_m0[51u].w * _1125)) * 2.0f) + (-1.0f);
                float _1517 = mad(_190, _1123, mad(_183, _1501, _1500 * _176)) + _197;
                float _1521 = ((mad(_187, _1123, mad(_180, _1501, _1500 * _173)) + _194) / _1517) - _1493;
                float _1522 = ((mad(_188, _1123, mad(_181, _1501, _1500 * _174)) + _195) / _1517) - _1494;
                float _1523 = ((mad(_189, _1123, mad(_182, _1501, _1500 * _175)) + _196) / _1517) - _1495;
                float _1539;
                uint _1541;
                float _1543;
                if (_1119 < 129u)
                {
                    float frontier_phi_79_78_ladder;
                    uint frontier_phi_79_78_ladder_1;
                    float frontier_phi_79_78_ladder_2;
                    if ((_1127 < 0.0f) || (_1125 < 0.0f))
                    {
                        frontier_phi_79_78_ladder = 0.0f;
                        frontier_phi_79_78_ladder_1 = _1463;
                        frontier_phi_79_78_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_79_78_ladder_83_ladder;
                        uint frontier_phi_79_78_ladder_83_ladder_1;
                        float frontier_phi_79_78_ladder_83_ladder_2;
                        if ((_1123 >= 1.0f) || ((_1127 > _50_m0[51u].x) || (_1125 > _50_m0[51u].y)))
                        {
                            frontier_phi_79_78_ladder_83_ladder = 0.0f;
                            frontier_phi_79_78_ladder_83_ladder_1 = _1463;
                            frontier_phi_79_78_ladder_83_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_79_78_ladder_83_ladder_88_ladder;
                            uint frontier_phi_79_78_ladder_83_ladder_88_ladder_1;
                            float frontier_phi_79_78_ladder_83_ladder_88_ladder_2;
                            for (;;)
                            {
                                if ((abs(_1127 - _254) < (2.0f / _1045)) && (abs(_1125 - _255) < (2.0f / _1046)))
                                {
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder = 0.0f;
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder_1 = _1463;
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_112;
                                    float frontier_phi_112_pred;
                                    uint frontier_phi_112_pred_1;
                                    float frontier_phi_112_pred_2;
                                    uint _1948;
                                    uint _1949;
                                    bool _1951;
                                    for (;;)
                                    {
                                        _1948 = uint(int(_1127 * _1045));
                                        _1949 = uint(int(_1125 * _1046));
                                        _1951 = (_397 == 1u) && _1461;
                                        if (!_1951)
                                        {
                                            if (!(dot(float3(_1521, _1522, _1523), float3(_1521, _1522, _1523)) < 0.000899999984540045261383056640625f))
                                            {
                                                ladder_phi_112 = false;
                                                frontier_phi_112_pred = 0.0f;
                                                frontier_phi_112_pred_1 = _1463;
                                                frontier_phi_112_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _2003 = _24[22u].Load(int3(uint2(_1948, _1949), 0u));
                                        uint _2005 = _2003.x;
                                        float _2198;
                                        float _2199;
                                        float _2200;
                                        if (_2005 == 0u)
                                        {
                                            uint4 _2048 = _24[1u].Load(int3(uint2(_1948, _1949), 0u));
                                            uint _2050 = _2048.x;
                                            float _2058 = (float((_2050 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2059 = (float(_2050 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2063 = (1.0f - abs(_2058)) - abs(_2059);
                                            float _2065 = clamp((-0.0f) - _2063, 0.0f, 1.0f);
                                            float _2066 = (-0.0f) - _2065;
                                            _2198 = ((_2058 >= 0.0f) ? _2066 : _2065) + _2058;
                                            _2199 = ((_2059 >= 0.0f) ? _2066 : _2065) + _2059;
                                            _2200 = _2063;
                                        }
                                        else
                                        {
                                            float _2080 = (float((_2005 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2081 = (float(_2005 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2085 = (1.0f - abs(_2080)) - abs(_2081);
                                            float _2087 = clamp((-0.0f) - _2085, 0.0f, 1.0f);
                                            float _2088 = (-0.0f) - _2087;
                                            _2198 = ((_2080 >= 0.0f) ? _2088 : _2087) + _2080;
                                            _2199 = ((_2081 >= 0.0f) ? _2088 : _2087) + _2081;
                                            _2200 = _2085;
                                        }
                                        float _2204 = rsqrt(dot(float3(_2198, _2199, _2200), float3(_2198, _2199, _2200)));
                                        if (dot(float3(_2204 * _2198, _2204 * _2199, _2204 * _2200), float3(_1521, _1522, _1523)) > 0.0f)
                                        {
                                            ladder_phi_112 = true;
                                            frontier_phi_112_pred = 0.0f;
                                            frontier_phi_112_pred_1 = _1463;
                                            frontier_phi_112_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_112 = false;
                                            frontier_phi_112_pred = 0.0f;
                                            frontier_phi_112_pred_1 = _1463;
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
                                    float _2099 = _50_m0[51u].z * _1127;
                                    float _2100 = _50_m0[51u].w * _1125;
                                    float _2102 = (_1046 / _1045) * 0.0500000007450580596923828125f;
                                    float _2107 = clamp(_2099 / _2102, 0.0f, 1.0f);
                                    float _2108 = clamp(_2100 * 20.0f, 0.0f, 1.0f);
                                    float _2120 = clamp(((_2099 + (-1.0f)) + _2102) / _2102, 0.0f, 1.0f);
                                    float _2121 = clamp((_2100 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _2132 = _2107 * _2108;
                                    precise float _2133 = _2132 * _2132;
                                    float _2137 = ((((3.0f - (_2108 * 2.0f)) * (3.0f - (_2107 * 2.0f))) * _2133) * (1.0f - ((_2120 * _2120) * (3.0f - (_2120 * 2.0f))))) * (1.0f - ((_2121 * _2121) * (3.0f - (_2121 * 2.0f))));
                                    bool _2140 = (_1463 != 0u) || (_2137 >= 1.0f);
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder = _2137 * float(_476 > 0.0f);
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder_1 = _2140 ? _1463 : 1u;
                                    frontier_phi_79_78_ladder_83_ladder_88_ladder_2 = _2140 ? 0.0f : _2137;
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
                    _1539 = frontier_phi_79_78_ladder_2;
                    _1541 = frontier_phi_79_78_ladder_1;
                    _1543 = frontier_phi_79_78_ladder;
                }
                else
                {
                    _1539 = 0.0f;
                    _1541 = _1463;
                    _1543 = 0.0f;
                }
                bool _1670;
                float _1673;
                float _1676;
                float _1678;
                float _1680;
                float _1682;
                float _1683;
                float _1684;
                float _1686;
                float _1584;
                float _1587;
                float _1590;
                float _1593;
                float _1597;
                float _1598;
                for (;;)
                {
                    _1584 = ((((exp2(log2(clamp((sqrt(((_1494 * _1494) + (_1493 * _1493)) + (_1495 * _1495)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _407) * exp2(log2(clamp((_1494 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_393, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    _1587 = mad(_167, _903, mad(_161, _902, _901 * _155));
                    _1590 = mad(_168, _903, mad(_162, _902, _901 * _156));
                    _1593 = mad(_169, _903, mad(_163, _902, _901 * _157));
                    bool _1596 = (_1541 != 0u) || (_1543 < 1.0f);
                    _1597 = _1596 ? 0.0f : 1.0f;
                    _1598 = _1596 ? 0.0f : 0.5f;
                    if (_1596)
                    {
                        bool _1665 = _397 == 1u;
                        uint4 _1669 = asuint(_50_m0[60u]);
                        if (_1665)
                        {
                            if (int(_405) < int(1u))
                            {
                                if (_1669.x == 0u)
                                {
                                    _1670 = false;
                                    _1673 = 9899999600270360182784.0f;
                                    _1676 = _1147;
                                    _1678 = _1145;
                                    _1680 = _1143;
                                    _1682 = 0.0f;
                                    _1683 = 0.0f;
                                    _1684 = 0.0f;
                                    _1686 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1669.y == 0u)
                                {
                                    _1670 = false;
                                    _1673 = 9899999600270360182784.0f;
                                    _1676 = _1147;
                                    _1678 = _1145;
                                    _1680 = _1143;
                                    _1682 = 0.0f;
                                    _1683 = 0.0f;
                                    _1684 = 0.0f;
                                    _1686 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1669.z == 0u)
                            {
                                _1670 = false;
                                _1673 = 9899999600270360182784.0f;
                                _1676 = _1147;
                                _1678 = _1145;
                                _1680 = _1143;
                                _1682 = 0.0f;
                                _1683 = 0.0f;
                                _1684 = 0.0f;
                                _1686 = 0.0f;
                                break;
                            }
                        }
                        if (_1539 > 0.0f)
                        {
                            _1670 = false;
                            _1673 = 9899999600270360182784.0f;
                            _1676 = _1147;
                            _1678 = _1145;
                            _1680 = _1143;
                            _1682 = 0.0f;
                            _1683 = 1.0f;
                            _1684 = 1000.0f;
                            _1686 = 0.5f;
                            break;
                        }
                        if (((_1153 <= 0.0f) || (_1151 <= 0.0f)) || (_1149 <= 0.0f))
                        {
                            _1670 = false;
                            _1673 = 9899999600270360182784.0f;
                            _1676 = _1147;
                            _1678 = _1145;
                            _1680 = _1143;
                            _1682 = 0.0f;
                            _1683 = 1.0f;
                            _1684 = 1000.0f;
                            _1686 = 0.5f;
                            break;
                        }
                        if ((_1149 >= 1.0f) || ((_1153 >= _50_m0[51u].x) || (_1151 >= _50_m0[51u].y)))
                        {
                            _1670 = false;
                            _1673 = 9899999600270360182784.0f;
                            _1676 = _1147;
                            _1678 = _1145;
                            _1680 = _1143;
                            _1682 = 0.0f;
                            _1683 = 1.0f;
                            _1684 = 1000.0f;
                            _1686 = 0.5f;
                            break;
                        }
                        uint _2429;
                        uint _2431;
                        uint _2216;
                        uint _2217;
                        bool _2223;
                        for (;;)
                        {
                            _2216 = uint(clamp(_1161, 0.0f, 1.0f) * _202);
                            _2217 = uint(clamp(_1159, 0.0f, 1.0f) * _204);
                            _2223 = _20[21u].Load(int3(uint2(_2216, _2217), 0u)).x > 0.0f;
                            if (_2223)
                            {
                                uint _2315 = _24[23u].Load(int3(uint2(_2216, _2217), 0u)).y + 4294967295u;
                                _2429 = (uint(int(_2315) >> int(31u)) & 3u) + 1u;
                                _2431 = (int(_2315) < int(0u)) ? 0u : _2315;
                                break;
                            }
                            else
                            {
                                uint4 _2323 = _24[2u].Load(int3(uint2(_2216, _2217), 0u));
                                uint _2326 = _2323.w;
                                uint4 _2331 = _24[15u].Load(int3(uint2(_2216, _2217), 0u));
                                uint _2333 = _2331.y;
                                uint _2339 = ((_2333 & 64u) != 0u) ? uint((_2333 & 4294967167u) != 66u) : 4294967295u;
                                uint _2340 = _2326 & 128u;
                                uint _2342 = (_2340 != 0u) ? 1u : ((_2323.x << 7u) | _2326);
                                uint4 _2345 = _16.Load(_2342 * 4u);
                                uint _2346 = _2345.x;
                                uint _2353 = ((_2346 & 1u) != 0u) ? 0u : 18u;
                                uint _2355 = uint(min(int(uint(max(int(_2339), int(0u)))), int(1u)));
                                uint _2442;
                                if (_2340 == 0u)
                                {
                                    _2442 = (((_2346 & 2097152u) != 0u) && (_2339 == _2355)) ? (_2353 | 128u) : _2353;
                                }
                                else
                                {
                                    _2442 = _2326;
                                }
                                uint _2443 = _16.Load((_2342 * 4u) + 1u).x & 512u;
                                bool _2446 = (_2442 & 144u) == 0u;
                                if (_2443 == 0u)
                                {
                                    if (_2446 || ((_2346 & 1u) != 0u))
                                    {
                                        _2429 = 0u;
                                        _2431 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2446)
                                    {
                                        _2429 = 0u;
                                        _2431 = 0u;
                                        break;
                                    }
                                }
                                bool _2613 = ((_2442 & 128u) | _2443) != 0u;
                                uint _2430;
                                if (_2613)
                                {
                                    _2430 = 1u;
                                }
                                else
                                {
                                    _2430 = (((_2346 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_2346 & 268435472u) == 16u) && (((_2346 & 33554432u) == 0u) || _2613))
                                {
                                    _2429 = 2u;
                                    _2431 = 0u;
                                    break;
                                }
                                _2429 = _2430;
                                _2431 = (_2430 == 1u) ? _2355 : 0u;
                                break;
                            }
                        }
                        if ((_397 != _2429) || (_405 != _2431))
                        {
                            _1670 = false;
                            _1673 = 9899999600270360182784.0f;
                            _1676 = _1147;
                            _1678 = _1145;
                            _1680 = _1143;
                            _1682 = 0.0f;
                            _1683 = 1.0f;
                            _1684 = 1000.0f;
                            _1686 = 0.5f;
                            break;
                        }
                        float _2497 = _1153 * 2.0f;
                        float _2500 = (_50_m0[51u].z * _2497) + (-1.0f);
                        float _2501 = ((1.0f - (_50_m0[51u].w * _1151)) * 2.0f) + (-1.0f);
                        float _2517 = mad(_190, _1149, mad(_183, _2501, _2500 * _176)) + _197;
                        float _2518 = (mad(_187, _1149, mad(_180, _2501, _2500 * _173)) + _194) / _2517;
                        float _2519 = (mad(_188, _1149, mad(_181, _2501, _2500 * _174)) + _195) / _2517;
                        float _2520 = (mad(_189, _1149, mad(_182, _2501, _2500 * _175)) + _196) / _2517;
                        if (sqrt(((_2519 * _2519) + (_2518 * _2518)) + (_2520 * _2520)) > _50_m0[58u].w)
                        {
                            _1670 = false;
                            _1673 = 9899999600270360182784.0f;
                            _1676 = _1147;
                            _1678 = _1145;
                            _1680 = _1143;
                            _1682 = 0.0f;
                            _1683 = 0.0f;
                            _1684 = 1000.0f;
                            _1686 = 0.5f;
                            break;
                        }
                        float _2593 = _2518 - _1493;
                        float _2594 = _2519 - _1494;
                        float _2595 = _2520 - _1495;
                        float _2601 = sqrt(((_2594 * _2594) + (_2593 * _2593)) + (_2595 * _2595));
                        float _2609 = min(_50_m0[59u].y, max(0.0f, _2601 + (-0.001000000047497451305389404296875f)));
                        float _2637;
                        if (_1665)
                        {
                            _2637 = min(_50_m0[59u].x, _2609 + 10.0f);
                        }
                        else
                        {
                            _2637 = _50_m0[59u].x;
                        }
                        float _2638 = _2637 - _2601;
                        if (!(_2638 > 0.0f))
                        {
                            _1670 = false;
                            _1673 = 9899999600270360182784.0f;
                            _1676 = _1147;
                            _1678 = _1145;
                            _1680 = _1143;
                            _1682 = 1.0f;
                            _1683 = 1.0f;
                            _1684 = 0.0f;
                            _1686 = 0.5f;
                            break;
                        }
                        float _2698 = _2518 - (_2609 * _1587);
                        float _2699 = _2519 - (_2609 * _1590);
                        float _2700 = _2520 - (_2609 * _1593);
                        RayDesc _2ident = {float3(mad(_2700, _50_m0[46u].z, mad(_2699, _50_m0[46u].y, _50_m0[46u].x * _2698)) + _50_m0[46u].w, mad(_2700, _50_m0[47u].z, mad(_2699, _50_m0[47u].y, _50_m0[47u].x * _2698)) + _50_m0[47u].w, mad(_2700, _50_m0[48u].z, mad(_2699, _50_m0[48u].y, _50_m0[48u].x * _2698)) + _50_m0[48u].w), 0.0f, float3(mad(_1593, _50_m0[46u].z, mad(_1590, _50_m0[46u].y, _50_m0[46u].x * _1587)), mad(_1593, _50_m0[47u].z, mad(_1590, _50_m0[47u].y, _50_m0[47u].x * _1587)), mad(_1593, _50_m0[48u].z, mad(_1590, _50_m0[48u].y, _50_m0[48u].x * _1587))), _2638};
                        _2703.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2750 = _2703.Proceed();
                        uint _2751 = _2703.CommittedStatus();
                        if (!(_2751 == 1u))
                        {
                            _1670 = false;
                            _1673 = 9899999600270360182784.0f;
                            _1676 = _1147;
                            _1678 = _1145;
                            _1680 = _1143;
                            _1682 = 1.0f;
                            _1683 = 0.0f;
                            _1684 = 0.0f;
                            _1686 = 0.5f;
                            break;
                        }
                        float _2765 = _2703.CommittedRayT();
                        if (!((_2765 < _2638) && (_2765 > 0.0f)))
                        {
                            _1670 = false;
                            _1673 = 9899999600270360182784.0f;
                            _1676 = _1147;
                            _1678 = _1145;
                            _1680 = _1143;
                            _1682 = 1.0f;
                            _1683 = 0.0f;
                            _1684 = 0.0f;
                            _1686 = 0.5f;
                            break;
                        }
                        float _2779 = (_50_m0[51u].z * _2497) + (-1.0f);
                        float _2780 = ((1.0f - (_50_m0[51u].w * _1151)) * 2.0f) + (-1.0f);
                        float _2796 = mad(_144, _1149, mad(_137, _2780, _2779 * _130)) + _151;
                        float _2800 = _2765 - _2609;
                        float _2804 = ((mad(_141, _1149, mad(_134, _2780, _2779 * _127)) + _148) / _2796) + (_2800 * _901);
                        float _2805 = ((mad(_142, _1149, mad(_135, _2780, _2779 * _128)) + _149) / _2796) + (_2800 * _902);
                        float _2806 = ((mad(_143, _1149, mad(_136, _2780, _2779 * _129)) + _150) / _2796) + (_2800 * _903);
                        float _2822 = mad(_116, _2806, mad(_109, _2805, _2804 * _102)) + _123;
                        float _1681 = (mad(_115, _2806, mad(_108, _2805, _2804 * _101)) + _122) / _2822;
                        float _1677 = ((((mad(_113, _2806, mad(_106, _2805, _2804 * _99)) + _120) / _2822) * 0.5f) + 0.5f) * _50_m0[51u].x;
                        float _1679 = (0.5f - (((mad(_114, _2806, mad(_107, _2805, _2804 * _100)) + _121) / _2822) * 0.5f)) * _50_m0[51u].y;
                        float _2831 = _1677 * _50_m0[51u].z;
                        float _2832 = _1679 * _50_m0[51u].w;
                        float _2834 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2837 = clamp(_2831 / _2834, 0.0f, 1.0f);
                        float _2838 = clamp(_2832 * 20.0f, 0.0f, 1.0f);
                        float _2848 = clamp(((_2831 + (-1.0f)) + _2834) / _2834, 0.0f, 1.0f);
                        float _2849 = clamp((_2832 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2860 = _2837 * _2838;
                        precise float _2861 = _2860 * _2860;
                        if ((((((3.0f - (_2838 * 2.0f)) * (3.0f - (_2837 * 2.0f))) * _2861) * (1.0f - ((_2848 * _2848) * (3.0f - (_2848 * 2.0f))))) * (1.0f - ((_2849 * _2849) * (3.0f - (_2849 * 2.0f))))) < 1.0f)
                        {
                            _1670 = false;
                            _1673 = 9899999600270360182784.0f;
                            _1676 = _1677;
                            _1678 = _1679;
                            _1680 = _1681;
                            _1682 = _1597;
                            _1683 = _1597;
                            _1684 = 0.0f;
                            _1686 = _1598;
                            break;
                        }
                        _1670 = true;
                        _1673 = (_2601 - _2609) + _2765;
                        _1676 = _1677;
                        _1678 = _1679;
                        _1680 = _1681;
                        _1682 = 0.0f;
                        _1683 = 1.0f;
                        _1684 = 0.0f;
                        _1686 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1670 = false;
                        _1673 = 9899999600270360182784.0f;
                        _1676 = _1147;
                        _1678 = _1145;
                        _1680 = _1143;
                        _1682 = 1.0f;
                        _1683 = 1.0f;
                        _1684 = 0.0f;
                        _1686 = 0.5f;
                        break;
                    }
                }
                uint4 _1689 = asuint(_55_m0[0u]);
                float _1691 = float(_1689.x);
                float _1693 = float(_1689.y);
                float _1715;
                float _1717;
                float _1719;
                if ((_1543 >= 1.0f) || _1670)
                {
                    _1715 = _1676;
                    _1717 = _1678;
                    _1719 = _1680;
                }
                else
                {
                    float _1866 = (-0.0f) - _945;
                    float _1867 = _1866 / _992;
                    float _1870 = (_1867 * _989) + _952;
                    float _1871 = (_1867 * _991) + _953;
                    float _1872 = _1866 + _945;
                    _1715 = ((_1676 - _1870) * _1543) + _1870;
                    _1717 = ((_1678 - _1871) * _1543) + _1871;
                    _1719 = ((_1680 - _1872) * _1543) + _1872;
                }
                float _1730 = ((_1715 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _1731 = ((1.0f - (_50_m0[51u].w * _1717)) * 2.0f) + (-1.0f);
                float _1747 = mad(_144, _1719, mad(_137, _1731, _1730 * _130)) + _151;
                float _1751 = ((mad(_141, _1719, mad(_134, _1731, _1730 * _127)) + _148) / _1747) - _924;
                float _1752 = ((mad(_142, _1719, mad(_135, _1731, _1730 * _128)) + _149) / _1747) - _925;
                float _1753 = ((mad(_143, _1719, mad(_136, _1731, _1730 * _129)) + _150) / _1747) - _926;
                float _1759 = sqrt(((_1752 * _1752) + (_1751 * _1751)) + (_1753 * _1753));
                float _1760 = _1759 * _779;
                float _1761 = _1759 * _780;
                float _1762 = _1759 * _781;
                float _1763 = _1759 * (_880 / _883);
                float _1764 = _1759 * (_881 / _883);
                float _1765 = _1759 * (_882 / _883);
                float _1769 = dot(float3(_1760, _1761, _1762), float3(_765, _768, _771)) * 2.0f;
                float _1779 = dot(float3(_1763, _1764, _1765), float3(_765, _768, _771)) * 2.0f;
                float _1806 = (_1760 - (_1769 * _765)) + _924;
                float _1807 = (_1761 - (_1769 * _768)) + _925;
                float _1808 = (_1762 - (_1769 * _771)) + _926;
                float _1820 = mad(_50_m0[24u].w, _1808, mad(_50_m0[23u].w, _1807, _1806 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1825 = (_1763 - (_1779 * _765)) + _924;
                float _1826 = (_1764 - (_1779 * _768)) + _925;
                float _1827 = (_1765 - (_1779 * _771)) + _926;
                float _1839 = mad(_50_m0[24u].w, _1827, mad(_50_m0[23u].w, _1826, _1825 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1845 = (_50_m0[51u].x * ((((mad(_50_m0[24u].x, _1808, mad(_50_m0[23u].x, _1807, _1806 * _50_m0[22u].x)) + _50_m0[25u].x) / _1820) - ((mad(_50_m0[24u].x, _1827, mad(_50_m0[23u].x, _1826, _1825 * _50_m0[22u].x)) + _50_m0[25u].x) / _1839)) * 0.5f)) * _1691;
                float _1849 = (_50_m0[51u].y * ((((mad(_50_m0[24u].y, _1827, mad(_50_m0[23u].y, _1826, _1825 * _50_m0[22u].y)) + _50_m0[25u].y) / _1839) - ((mad(_50_m0[24u].y, _1808, mad(_50_m0[23u].y, _1807, _1806 * _50_m0[22u].y)) + _50_m0[25u].y) / _1820)) * 0.5f)) * _1693;
                float _1864 = clamp(log2(sqrt((_1849 * _1849) + (_1845 * _1845)) * 2.0f) / float(asuint(_50_m0[55u]).x + 4294967295u), 0.0f, 1.0f);
                bool _1865 = _397 == 1u;
                float _1957;
                if (_1865)
                {
                    float _2017;
                    if (_1670)
                    {
                        _2017 = _1673;
                    }
                    else
                    {
                        _2017 = sqrt(((_1522 * _1522) + (_1521 * _1521)) + (_1523 * _1523));
                    }
                    float _2022 = clamp((_2017 + (-1.0f)) * 0.111111111938953399658203125f, 0.0f, 1.0f);
                    _1957 = (1.0f - ((_2022 * _2022) * (3.0f - (_2022 * 2.0f)))) * _50_m0[61u].z;
                }
                else
                {
                    _1957 = 1.0f;
                }
                float _1959 = _1957 * _1543;
                bool _1960 = _397 != 1u;
                float _2357;
                float _2359;
                float _2361;
                float _2363;
                if (_1959 == 0.0f)
                {
                    float _2249;
                    float _2251;
                    float _2253;
                    float _2255;
                    if (_1670)
                    {
                        float frontier_phi_121_114_ladder;
                        float frontier_phi_121_114_ladder_1;
                        float frontier_phi_121_114_ladder_2;
                        float frontier_phi_121_114_ladder_3;
                        if (_1960)
                        {
                            float _2224 = _1590 * _1584;
                            float _2228 = rsqrt(dot(float3(_1587, _2224, _1593), float3(_1587, _2224, _1593)));
                            float4 _2238 = _28[4u].SampleLevel(_59, float3(_2228 * _1587, _2228 * _2224, _2228 * _1593), 0.0f);
                            float _2240 = _2238.x;
                            float _2241 = _2238.y;
                            float _2242 = _2238.z;
                            frontier_phi_121_114_ladder = 1.0f;
                            frontier_phi_121_114_ladder_1 = _2242 - (_2242 * _1957);
                            frontier_phi_121_114_ladder_2 = _2241 - (_2241 * _1957);
                            frontier_phi_121_114_ladder_3 = _2240 - (_2240 * _1957);
                        }
                        else
                        {
                            frontier_phi_121_114_ladder = _1957;
                            frontier_phi_121_114_ladder_1 = 0.0f;
                            frontier_phi_121_114_ladder_2 = 0.0f;
                            frontier_phi_121_114_ladder_3 = 0.0f;
                        }
                        _2249 = frontier_phi_121_114_ladder_3;
                        _2251 = frontier_phi_121_114_ladder_2;
                        _2253 = frontier_phi_121_114_ladder_1;
                        _2255 = frontier_phi_121_114_ladder;
                    }
                    else
                    {
                        float frontier_phi_121_115_ladder;
                        float frontier_phi_121_115_ladder_1;
                        float frontier_phi_121_115_ladder_2;
                        float frontier_phi_121_115_ladder_3;
                        if (_1960)
                        {
                            float _2260 = _1590 * _1584;
                            float _2264 = rsqrt(dot(float3(_1587, _2260, _1593), float3(_1587, _2260, _1593)));
                            float4 _2272 = _28[4u].SampleLevel(_59, float3(_2264 * _1587, _2264 * _2260, _2264 * _1593), 0.0f);
                            frontier_phi_121_115_ladder = 1.0f;
                            frontier_phi_121_115_ladder_1 = _2272.z;
                            frontier_phi_121_115_ladder_2 = _2272.y;
                            frontier_phi_121_115_ladder_3 = _2272.x;
                        }
                        else
                        {
                            frontier_phi_121_115_ladder = 0.0f;
                            frontier_phi_121_115_ladder_1 = 0.0f;
                            frontier_phi_121_115_ladder_2 = 0.0f;
                            frontier_phi_121_115_ladder_3 = 0.0f;
                        }
                        _2249 = frontier_phi_121_115_ladder_3;
                        _2251 = frontier_phi_121_115_ladder_2;
                        _2253 = frontier_phi_121_115_ladder_1;
                        _2255 = frontier_phi_121_115_ladder;
                    }
                    _2357 = _2249 * _476;
                    _2359 = _2251 * _476;
                    _2361 = _2253 * _476;
                    _2363 = _2255 * _476;
                }
                else
                {
                    float _2289;
                    float _2291;
                    float _2296;
                    float _2300;
                    if (_12.Load(int3(uint2(uint(_1691 * _1127), uint(_1693 * _1125)), 0u)).x > 0.0f)
                    {
                        uint _2153_dummy_parameter;
                        uint2 _2153 = spvTextureSize(_14, 0u, _2153_dummy_parameter);
                        float4 _2162 = _14.Load(int3(uint2(uint(float(_2153.x) * _1127), uint(float(_2153.y) * _1125)), 0u));
                        float _2166 = _2162.x * 0.5f;
                        float _2167 = _2162.y * (-0.5f);
                        float4 _2186 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _2166) + (_50_m0[52u].x * _1127), (_50_m0[52u].w * _2167) + (_50_m0[52u].y * _1125)), 0.0f);
                        float _2285;
                        if (_1865)
                        {
                            float frontier_phi_124_123_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _945))) < 5.0f)
                            {
                                float _2417 = sqrt((_2166 * _2166) + (_2167 * _2167));
                                float frontier_phi_124_123_ladder_130_ladder;
                                if (_2417 > 0.0500000007450580596923828125f)
                                {
                                    float _2454 = _1127 - _952;
                                    float _2455 = _1125 - _953;
                                    float frontier_phi_124_123_ladder_130_ladder_140_ladder;
                                    if (_2417 > sqrt((_2455 * _2455) + (_2454 * _2454)))
                                    {
                                        uint4 _2543 = asuint(_55_m0[0u]);
                                        uint _2550 = uint(float(_2543.x) * _1127);
                                        uint _2551 = uint(float(_2543.y) * _1125);
                                        uint4 _2554 = _24[2u].Load(int3(uint2(_2550, _2551), 0u));
                                        uint _2557 = _2554.w;
                                        uint4 _2562 = _24[15u].Load(int3(uint2(_2550, _2551), 0u));
                                        uint _2564 = _2562.y;
                                        uint _2570 = ((_2564 & 64u) != 0u) ? uint((_2564 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2571 = _2557 & 128u;
                                        uint _2573 = (_2571 != 0u) ? 1u : ((_2554.x << 7u) | _2557);
                                        uint4 _2576 = _16.Load(_2573 * 4u);
                                        uint _2577 = _2576.x;
                                        uint _2584 = ((_2577 & 1u) != 0u) ? 0u : 18u;
                                        uint _2630;
                                        if (_2571 == 0u)
                                        {
                                            _2630 = (((_2577 & 2097152u) != 0u) && (_2570 == uint(min(int(uint(max(int(_2570), int(0u)))), int(1u))))) ? (_2584 | 128u) : _2584;
                                        }
                                        else
                                        {
                                            _2630 = _2557;
                                        }
                                        float frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder;
                                        if (((_2630 & 128u) | (_16.Load((_2573 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2653 = asuint(_55_m0[0u]);
                                            uint _2662 = uint(float(_2653.x) * (_2166 + _1127));
                                            uint _2663 = uint(float(_2653.y) * (_2167 + _1125));
                                            uint4 _2666 = _24[2u].Load(int3(uint2(_2662, _2663), 0u));
                                            uint _2669 = _2666.w;
                                            uint4 _2672 = _24[15u].Load(int3(uint2(_2662, _2663), 0u));
                                            uint _2674 = _2672.y;
                                            uint _2680 = ((_2674 & 64u) != 0u) ? uint((_2674 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2681 = _2669 & 128u;
                                            uint _2683 = (_2681 != 0u) ? 1u : ((_2666.x << 7u) | _2669);
                                            uint4 _2685 = _16.Load(_2683 * 4u);
                                            uint _2686 = _2685.x;
                                            uint _2693 = ((_2686 & 1u) != 0u) ? 0u : 18u;
                                            uint _2762;
                                            if (_2681 == 0u)
                                            {
                                                _2762 = (((_2686 & 2097152u) != 0u) && (_2680 == uint(min(int(uint(max(int(_2680), int(0u)))), int(1u))))) ? (_2693 | 128u) : _2693;
                                            }
                                            else
                                            {
                                                _2762 = _2669;
                                            }
                                            float frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder_165_ladder;
                                            if ((_2762 & 128u) == 0u)
                                            {
                                                frontier_phi_124_123_ladder_130_ladder_140_ladder_156_ladder_165_ladder = ((_16.Load((_2683 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
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
                            _2285 = frontier_phi_124_123_ladder;
                        }
                        else
                        {
                            _2285 = 1.0f;
                        }
                        float _2287 = _2285 * _1959;
                        float _2295;
                        float _2299;
                        float _2303;
                        if (_1157 == 0u)
                        {
                            _2295 = _50_m0[54u].x * _2186.x;
                            _2299 = _50_m0[54u].x * _2186.y;
                            _2303 = _50_m0[54u].x * _2186.z;
                        }
                        else
                        {
                            _2295 = 0.0f;
                            _2299 = 0.0f;
                            _2303 = 0.0f;
                        }
                        float frontier_phi_125_131_ladder;
                        float frontier_phi_125_131_ladder_1;
                        float frontier_phi_125_131_ladder_2;
                        float frontier_phi_125_131_ladder_3;
                        for (;;)
                        {
                            if (_1155 > 0.0f)
                            {
                                float _2294;
                                float _2298;
                                float _2302;
                                if (_1865)
                                {
                                    _2294 = 0.0f;
                                    _2298 = 0.0f;
                                    _2302 = 0.0f;
                                }
                                else
                                {
                                    if (!((_397 != 4u) || (_1163 != 0u)))
                                    {
                                        frontier_phi_125_131_ladder = _2299;
                                        frontier_phi_125_131_ladder_1 = _2295;
                                        frontier_phi_125_131_ladder_2 = _2287;
                                        frontier_phi_125_131_ladder_3 = _2303;
                                        break;
                                    }
                                    _2294 = _2295;
                                    _2298 = _2299;
                                    _2302 = _2303;
                                }
                                frontier_phi_125_131_ladder = _2298;
                                frontier_phi_125_131_ladder_1 = _2294;
                                frontier_phi_125_131_ladder_2 = (1.0f - exp2(log2(_1155) * 3.0f)) * _2287;
                                frontier_phi_125_131_ladder_3 = _2302;
                                break;
                            }
                            else
                            {
                                frontier_phi_125_131_ladder = _2299;
                                frontier_phi_125_131_ladder_1 = _2295;
                                frontier_phi_125_131_ladder_2 = _2287;
                                frontier_phi_125_131_ladder_3 = _2303;
                                break;
                            }
                        }
                        _2289 = frontier_phi_125_131_ladder_2;
                        _2291 = frontier_phi_125_131_ladder_1;
                        _2296 = frontier_phi_125_131_ladder;
                        _2300 = frontier_phi_125_131_ladder_3;
                    }
                    else
                    {
                        float frontier_phi_125_117_ladder;
                        float frontier_phi_125_117_ladder_1;
                        float frontier_phi_125_117_ladder_2;
                        float frontier_phi_125_117_ladder_3;
                        if (_1462)
                        {
                            frontier_phi_125_117_ladder = _2293;
                            frontier_phi_125_117_ladder_1 = _2293;
                            frontier_phi_125_117_ladder_2 = 0.0f;
                            frontier_phi_125_117_ladder_3 = _2293;
                        }
                        else
                        {
                            float4 _2308 = _28[7u].SampleLevel(_59, float3(_1587, _1590, _1593), 0.0f);
                            frontier_phi_125_117_ladder = _2308.y;
                            frontier_phi_125_117_ladder_1 = _2308.x;
                            frontier_phi_125_117_ladder_2 = _1959;
                            frontier_phi_125_117_ladder_3 = _2308.z;
                        }
                        _2289 = frontier_phi_125_117_ladder_2;
                        _2291 = frontier_phi_125_117_ladder_1;
                        _2296 = frontier_phi_125_117_ladder;
                        _2300 = frontier_phi_125_117_ladder_3;
                    }
                    float _2423;
                    float _2424;
                    float _2425;
                    float _2426;
                    if (_1670)
                    {
                        _2423 = _1957;
                        _2424 = _2291 * _1539;
                        _2425 = _2296 * _1539;
                        _2426 = _2300 * _1539;
                    }
                    else
                    {
                        _2423 = _2289;
                        _2424 = _2291;
                        _2425 = _2296;
                        _2426 = _2300;
                    }
                    float _2487;
                    float _2488;
                    float _2489;
                    float _2490;
                    if (_1960 && (_2423 < 1.0f))
                    {
                        float _2461 = _1590 * _1584;
                        float _2465 = rsqrt(dot(float3(_1587, _2461, _1593), float3(_1587, _2461, _1593)));
                        float4 _2473 = _28[4u].SampleLevel(_59, float3(_2465 * _1587, _2465 * _2461, _2465 * _1593), 0.0f);
                        float _2475 = _2473.x;
                        float _2476 = _2473.y;
                        float _2477 = _2473.z;
                        _2487 = 1.0f;
                        _2488 = ((_2424 - _2475) * _2423) + _2475;
                        _2489 = ((_2425 - _2476) * _2423) + _2476;
                        _2490 = ((_2426 - _2477) * _2423) + _2477;
                    }
                    else
                    {
                        _2487 = _2423;
                        _2488 = _2424;
                        _2489 = _2425;
                        _2490 = _2426;
                    }
                    float _2364 = _2487 * _476;
                    _2357 = _2488 * _2364;
                    _2359 = _2489 * _2364;
                    _2361 = _2490 * _2364;
                    _2363 = _2364;
                }
                float _2368 = _50_m0[58u].z * _1686;
                float _2390 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _2392 = _2390 * ((_2368 * ((_1682 * 1000.0f) - _2357)) + _2357);
                float _2393 = _2390 * ((_2368 * ((_1683 * 1000.0f) - _2359)) + _2359);
                float _2394 = _2390 * ((_2368 * (_1684 - _2361)) + _2361);
                float _2400 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2392, max(_2393, _2394)) + 1.0f);
                float _2404 = min(_2400 * _2392, 0.996078431606292724609375f);
                float _2406 = min(_2400 * _2393, 0.996078431606292724609375f);
                float _2407 = min(_2400 * _2394, 0.996078431606292724609375f);
                _35[uint2(_221, _224)] = float4(_2404, _2406, _2407, _2363);
                _39[uint2(_221, _224)] = float4(_1864, _401, _2363, _1864);
                if (_228)
                {
                    uint _2447 = _221 + 1u;
                    _35[uint2(_2447, _224)] = float4(_2404, _2406, _2407, _2363);
                    _39[uint2(_2447, _224)] = float4(_1864, _401, _2363, _1864);
                }
                if (_231)
                {
                    uint _2534 = _224 + 1u;
                    _35[uint2(_221, _2534)] = float4(_2404, _2406, _2407, _2363);
                    _39[uint2(_221, _2534)] = float4(_1864, _401, _2363, _1864);
                }
                if (_232)
                {
                    uint _2614 = _221 + 1u;
                    uint _2615 = _224 + 1u;
                    _35[uint2(_2614, _2615)] = float4(_2404, _2406, _2407, _2363);
                    _39[uint2(_2614, _2615)] = float4(_1864, _401, _2363, _1864);
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
