static float _1810;
static uint _2830;
static float _2831;
static float _2832;
static uint _2833;
static float _2834;
static float _2835;
static float _2836;
static float _2837;
static uint _2838;
static uint _2839;
static float _2840;
static float _2841;
static float _2842;
static float _2843;
static float _2844;
static uint _2845;
static float _2851;
static uint _2852;
static float _2853;
static float _2858;
static float _2859;
static float _2860;

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

static RayQuery<RAY_FLAG_NONE> _2370;

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
                    uint _429;
                    uint _430;
                    if (_368 == 0u)
                    {
                        _429 = (((_376 & 2097152u) != 0u) && (_367 == _389)) ? (_387 | 128u) : _387;
                        _430 = _376;
                    }
                    else
                    {
                        _429 = _350;
                        _430 = _376 | ((_349 << 20u) & 134217728u);
                    }
                    uint _439 = _380 & 512u;
                    float _990;
                    float _992;
                    float _994;
                    float _996;
                    float _998;
                    float _1000;
                    float _1002;
                    float _1004;
                    float _1006;
                    float _1008;
                    float _1010;
                    float _1012;
                    if (_439 == 0u)
                    {
                        if (!((_430 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _815 = asfloat(_17.Load((_384 * 115u) + 33u).x);
                        uint4 _823 = _24[2u].Load(int3(uint2(_265, _266), 0u));
                        uint _825 = _823.y;
                        uint _826 = _429 & 128u;
                        uint _978;
                        uint _979;
                        uint _980;
                        uint _981;
                        if (_826 == 0u)
                        {
                            _978 = uint(((_430 & 817889384u) | (_380 & 576u)) != 0u) | (((_430 >> 19u) & 1u) ^ 1u);
                            _979 = uint(((_430 & 17825808u) | (_380 & 520u)) != 0u);
                            _980 = uint(((_430 & 46137344u) | (_380 & 2564u)) != 0u);
                            _981 = 0u;
                        }
                        else
                        {
                            _978 = 1u;
                            _979 = _429 & 1u;
                            _980 = 1u;
                            _981 = 1u;
                        }
                        precise float _985 = float(_825 & 127u) * 0.0078740157186985015869140625f;
                        bool _989 = (_430 & 4194304u) == 0u;
                        float _1040;
                        if (_989)
                        {
                            _1040 = _985;
                        }
                        else
                        {
                            _1040 = float(_825 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1102;
                        if ((_430 & 134217728u) == 0u)
                        {
                            uint frontier_phi_49_40_ladder;
                            if ((_826 != 0u) || ((_430 & 17825792u) == 1048576u))
                            {
                                frontier_phi_49_40_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_49_40_ladder = _979;
                            }
                            _1102 = frontier_phi_49_40_ladder;
                        }
                        else
                        {
                            _1102 = _979;
                        }
                        uint4 _1105 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _1107 = _1105.x;
                        float _1154;
                        float _1156;
                        float _1158;
                        if (_978 == 0u)
                        {
                            _1154 = 0.0f;
                            _1156 = 0.0f;
                            _1158 = 0.0f;
                        }
                        else
                        {
                            float4 _1163 = _20[8u].Load(int3(uint2(_265, _266), 0u));
                            _1154 = _1163.x;
                            _1156 = _1163.y;
                            _1158 = _1163.z;
                        }
                        uint _1236;
                        if (_1102 == 0u)
                        {
                            _1236 = 0u;
                        }
                        else
                        {
                            _1236 = _24[9u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        uint _1306;
                        if (_980 == 0u)
                        {
                            _1306 = 0u;
                        }
                        else
                        {
                            _1306 = _24[10u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        float _1316 = (float((_1107 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1317 = (float(_1107 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1321 = (1.0f - abs(_1316)) - abs(_1317);
                        float _1323 = clamp((-0.0f) - _1321, 0.0f, 1.0f);
                        float _1324 = (-0.0f) - _1323;
                        float _1329 = ((_1316 >= 0.0f) ? _1324 : _1323) + _1316;
                        float _1330 = ((_1317 >= 0.0f) ? _1324 : _1323) + _1317;
                        float _1334 = rsqrt(dot(float3(_1329, _1330, _1321), float3(_1329, _1330, _1321)));
                        float _1335 = _1329 * _1334;
                        float _1336 = _1330 * _1334;
                        float _1337 = _1334 * _1321;
                        float _991 = float(_1107 & 255u);
                        float _1341 = ((_430 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1448;
                        float _1449;
                        float _1450;
                        float _1451;
                        uint _1452;
                        if ((_380 & 64u) == 0u)
                        {
                            float frontier_phi_78_70_ladder;
                            float frontier_phi_78_70_ladder_1;
                            float frontier_phi_78_70_ladder_2;
                            float frontier_phi_78_70_ladder_3;
                            uint frontier_phi_78_70_ladder_4;
                            if ((_430 & 276824064u) == 0u)
                            {
                                frontier_phi_78_70_ladder = 0.0f;
                                frontier_phi_78_70_ladder_1 = ((_430 & 8u) != 0u) ? _1156 : _1341;
                                frontier_phi_78_70_ladder_2 = 0.0f;
                                frontier_phi_78_70_ladder_3 = 0.0f;
                                frontier_phi_78_70_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_78_70_ladder = 0.0f;
                                frontier_phi_78_70_ladder_1 = _1341;
                                frontier_phi_78_70_ladder_2 = 0.0f;
                                frontier_phi_78_70_ladder_3 = 0.0f;
                                frontier_phi_78_70_ladder_4 = 0u;
                            }
                            _1448 = frontier_phi_78_70_ladder_1;
                            _1449 = frontier_phi_78_70_ladder;
                            _1450 = frontier_phi_78_70_ladder_2;
                            _1451 = frontier_phi_78_70_ladder_3;
                            _1452 = frontier_phi_78_70_ladder_4;
                        }
                        else
                        {
                            float _1405 = (_1156 * 2.0f) + (-1.0f);
                            float _1406 = (_1158 * 2.0f) + (-1.0f);
                            float _1410 = (1.0f - abs(_1405)) - abs(_1406);
                            float _1412 = clamp((-0.0f) - _1410, 0.0f, 1.0f);
                            float _1413 = (-0.0f) - _1412;
                            float _1418 = ((_1405 >= 0.0f) ? _1413 : _1412) + _1405;
                            float _1419 = ((_1406 >= 0.0f) ? _1413 : _1412) + _1406;
                            float _1423 = rsqrt(dot(float3(_1418, _1419, _1410), float3(_1418, _1419, _1410)));
                            _1448 = floor(round(_1154 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1449 = _1418 * _1423;
                            _1450 = _1419 * _1423;
                            _1451 = _1423 * _1410;
                            _1452 = 1u;
                        }
                        float _999;
                        if ((_430 & 32768u) == 0u)
                        {
                            _999 = _1448;
                        }
                        else
                        {
                            float frontier_phi_87_88_ladder;
                            if (_17.Load((_384 * 115u) + 36u).x == 0u)
                            {
                                float _1633 = clamp((_991 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _815;
                                frontier_phi_87_88_ladder = ((_430 & 131072u) != 0u) ? _1633 : ((((clamp((1.21000003814697265625f / (exp2((_1040 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_384 * 115u) + 32u).x)) + 1.0f) * _1633);
                            }
                            else
                            {
                                frontier_phi_87_88_ladder = _815;
                            }
                            _999 = frontier_phi_87_88_ladder;
                        }
                        uint _1502 = _429 & 1u;
                        float _1576;
                        float _1578;
                        float _1580;
                        uint _1582;
                        if (((_430 & 16u) == 0u) || (((_1502 | (_380 & 8u)) | (_430 & 16777216u)) != 0u))
                        {
                            _1576 = _1449;
                            _1578 = _1450;
                            _1580 = _1451;
                            _1582 = _1452;
                        }
                        else
                        {
                            float _1592 = (float(_1236 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1593 = (float(_1236 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1597 = (1.0f - abs(_1592)) - abs(_1593);
                            float _1599 = clamp((-0.0f) - _1597, 0.0f, 1.0f);
                            float _1600 = (-0.0f) - _1599;
                            float _1605 = ((_1592 >= 0.0f) ? _1600 : _1599) + _1592;
                            float _1606 = ((_1593 >= 0.0f) ? _1600 : _1599) + _1593;
                            float _1610 = rsqrt(dot(float3(_1605, _1606, _1597), float3(_1605, _1606, _1597)));
                            _1576 = _1605 * _1610;
                            _1578 = _1606 * _1610;
                            _1580 = _1610 * _1597;
                            _1582 = 1u;
                        }
                        float _993;
                        float _995;
                        float _997;
                        if (_1502 == 0u)
                        {
                            float frontier_phi_111_110_ladder;
                            float frontier_phi_111_110_ladder_1;
                            float frontier_phi_111_110_ladder_2;
                            if (((_429 & 64u) == 0u) && (_981 != 0u))
                            {
                                float2 _1946 = spvUnpackHalf2x16((_1306 >> 17u) & 32736u);
                                float _1947 = _1946.x;
                                float _1950 = (spvUnpackHalf2x16((_1306 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1951 = (spvUnpackHalf2x16((_1306 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1955 = (1.0f - abs(_1950)) - abs(_1951);
                                float _1957 = clamp((-0.0f) - _1955, 0.0f, 1.0f);
                                float _1958 = (-0.0f) - _1957;
                                float _1963 = ((_1950 >= 0.0f) ? _1958 : _1957) + _1950;
                                float _1964 = ((_1951 >= 0.0f) ? _1958 : _1957) + _1951;
                                float _1968 = rsqrt(dot(float3(_1963, _1964, _1955), float3(_1963, _1964, _1955)));
                                float _1978 = (((_1963 * _1968) - _1335) * _1947) + _1335;
                                float _1979 = (((_1964 * _1968) - _1336) * _1947) + _1336;
                                float _1980 = (((_1968 * _1955) - _1337) * _1947) + _1337;
                                float _1984 = rsqrt(dot(float3(_1978, _1979, _1980), float3(_1978, _1979, _1980)));
                                frontier_phi_111_110_ladder = _1980 * _1984;
                                frontier_phi_111_110_ladder_1 = _1978 * _1984;
                                frontier_phi_111_110_ladder_2 = _1979 * _1984;
                            }
                            else
                            {
                                frontier_phi_111_110_ladder = _1337;
                                frontier_phi_111_110_ladder_1 = _1335;
                                frontier_phi_111_110_ladder_2 = _1336;
                            }
                            _993 = frontier_phi_111_110_ladder_1;
                            _995 = frontier_phi_111_110_ladder_2;
                            _997 = frontier_phi_111_110_ladder;
                        }
                        else
                        {
                            _993 = _1335;
                            _995 = _1336;
                            _997 = _1337;
                        }
                        float _1007;
                        float _1009;
                        float _1011;
                        float _1013;
                        if (_989)
                        {
                            float frontier_phi_122_121_ladder;
                            float frontier_phi_122_121_ladder_1;
                            float frontier_phi_122_121_ladder_2;
                            float frontier_phi_122_121_ladder_3;
                            if (((_430 & 33554432u) == 0u) || (((_380 & 4u) != 0u) && ((_430 & 8388608u) == 0u)))
                            {
                                frontier_phi_122_121_ladder = 0.0f;
                                frontier_phi_122_121_ladder_1 = 0.0f;
                                frontier_phi_122_121_ladder_2 = 0.0f;
                                frontier_phi_122_121_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2103 = (spvUnpackHalf2x16((_1306 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2104 = (spvUnpackHalf2x16((_1306 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2108 = (1.0f - abs(_2103)) - abs(_2104);
                                float _2110 = clamp((-0.0f) - _2108, 0.0f, 1.0f);
                                float _2111 = (-0.0f) - _2110;
                                float _2116 = ((_2103 >= 0.0f) ? _2111 : _2110) + _2103;
                                float _2117 = ((_2104 >= 0.0f) ? _2111 : _2110) + _2104;
                                float _2121 = rsqrt(dot(float3(_2116, _2117, _2108), float3(_2116, _2117, _2108)));
                                float _2122 = _2116 * _2121;
                                float _2123 = _2117 * _2121;
                                float _2124 = _2121 * _2108;
                                float _2128 = rsqrt(dot(float3(_2122, _2123, _2124), float3(_2122, _2123, _2124)));
                                frontier_phi_122_121_ladder = _2128 * _2124;
                                frontier_phi_122_121_ladder_1 = _2128 * _2123;
                                frontier_phi_122_121_ladder_2 = _2128 * _2122;
                                frontier_phi_122_121_ladder_3 = spvUnpackHalf2x16((_1306 >> 17u) & 32736u).x;
                            }
                            _1007 = frontier_phi_122_121_ladder_3;
                            _1009 = frontier_phi_122_121_ladder_2;
                            _1011 = frontier_phi_122_121_ladder_1;
                            _1013 = frontier_phi_122_121_ladder;
                        }
                        else
                        {
                            _1007 = 0.0f;
                            _1009 = 0.0f;
                            _1011 = 0.0f;
                            _1013 = 0.0f;
                        }
                        bool _1998 = _1582 != 0u;
                        _990 = _991;
                        _992 = _993;
                        _994 = _995;
                        _996 = _997;
                        _998 = _999;
                        _1000 = _1998 ? _1576 : _993;
                        _1002 = _1998 ? _1578 : _995;
                        _1004 = _1998 ? _1580 : _997;
                        _1006 = _1007;
                        _1008 = _1009;
                        _1010 = _1011;
                        _1012 = _1013;
                    }
                    else
                    {
                        uint4 _742 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _744 = _742.x;
                        uint4 _748 = _24[9u].Load(int3(uint2(_265, _266), 0u));
                        uint _750 = _748.x;
                        float _929;
                        float _930;
                        float _931;
                        if ((_430 & 33554432u) == 0u)
                        {
                            float _835 = (float((_744 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _836 = (float(_744 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _840 = (1.0f - abs(_835)) - abs(_836);
                            float _842 = clamp((-0.0f) - _840, 0.0f, 1.0f);
                            float _843 = (-0.0f) - _842;
                            float _848 = ((_835 >= 0.0f) ? _843 : _842) + _835;
                            float _849 = ((_836 >= 0.0f) ? _843 : _842) + _836;
                            float _853 = rsqrt(dot(float3(_848, _849, _840), float3(_848, _849, _840)));
                            _929 = _848 * _853;
                            _930 = _849 * _853;
                            _931 = _853 * _840;
                        }
                        else
                        {
                            float _864 = (float((_750 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _865 = (float(_750 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _869 = (1.0f - abs(_864)) - abs(_865);
                            float _871 = clamp((-0.0f) - _869, 0.0f, 1.0f);
                            float _872 = (-0.0f) - _871;
                            float _877 = ((_864 >= 0.0f) ? _872 : _871) + _864;
                            float _878 = ((_865 >= 0.0f) ? _872 : _871) + _865;
                            float _882 = rsqrt(dot(float3(_877, _878, _869), float3(_877, _878, _869)));
                            _929 = _877 * _882;
                            _930 = _878 * _882;
                            _931 = _882 * _869;
                        }
                        _990 = float(_744 & 255u);
                        _992 = _929;
                        _994 = _930;
                        _996 = _931;
                        _998 = 1.0f;
                        _1000 = _929;
                        _1002 = _930;
                        _1004 = _931;
                        _1006 = 0.0f;
                        _1008 = 0.0f;
                        _1010 = 0.0f;
                        _1012 = 0.0f;
                    }
                    precise float _1014 = _990 * 0.0039215688593685626983642578125f;
                    if ((_429 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1049 = ((_429 & 128u) | _439) != 0u;
                    uint _1080;
                    if (_1049)
                    {
                        _1080 = 1u;
                    }
                    else
                    {
                        _1080 = (((_430 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _392;
                    float _394;
                    float _396;
                    uint _398;
                    float _402;
                    if (((_430 & 33554432u) == 0u) || _1049)
                    {
                        bool _1109 = _77 != 0u;
                        uint _1116;
                        if ((_430 & 16u) == 0u)
                        {
                            _1116 = _1080;
                        }
                        else
                        {
                            _1116 = ((_430 & 268435456u) != 0u) ? _1080 : 2u;
                        }
                        _402 = _998 * _1014;
                        _392 = _1109 ? _1000 : _992;
                        _394 = _1109 ? _1002 : _994;
                        _396 = _1109 ? _1004 : _996;
                        _398 = _1116;
                    }
                    else
                    {
                        _402 = _1006;
                        _392 = _1008;
                        _394 = _1010;
                        _396 = _1012;
                        _398 = _1080;
                    }
                    uint _1117 = _398 + 102u;
                    float _1126 = clamp((_402 - _45_m0[_1117].x) / (_45_m0[_1117].y - _45_m0[_1117].x), 0.0f, 1.0f);
                    _391 = _392;
                    _393 = _394;
                    _395 = _396;
                    _397 = _398;
                    _399 = (_1126 * _1126) * (3.0f - (_1126 * 2.0f));
                    _401 = _402;
                    _403 = _45_m0[_1117].z;
                    _405 = (_398 == 1u) ? _389 : 0u;
                    _407 = asfloat(_17.Load((_384 * 115u) + 114u).x);
                }
                if (_399 == 0.0f)
                {
                    ladder_phi_8 = false;
                    break;
                }
                float _427 = float(_234);
                float _428 = float(_233);
                float _449;
                if (_263)
                {
                    _449 = _262;
                }
                else
                {
                    _449 = _12.Load(int3(uint2(uint(int(_254 * _428)), uint(int(_255 * _427))), 0u)).x;
                }
                float _458 = _50_m0[50u].w + _50_m0[50u].y;
                uint _461 = _397 + 63u;
                float _470 = clamp(((_50_m0[50u].x / (_458 - (_50_m0[50u].y * _449))) - _50_m0[_461].y) / (_50_m0[_461].x - _50_m0[_461].y), 0.0f, 1.0f);
                float _482 = ((_254 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _483 = ((1.0f - (_50_m0[51u].w * _255)) * 2.0f) + (-1.0f);
                float _499 = mad(_144, _449, mad(_137, _483, _482 * _130)) + _151;
                float _500 = (mad(_141, _449, mad(_134, _483, _482 * _127)) + _148) / _499;
                float _501 = (mad(_142, _449, mad(_135, _483, _482 * _128)) + _149) / _499;
                float _502 = (mad(_143, _449, mad(_136, _483, _482 * _129)) + _150) / _499;
                float _506 = rsqrt(dot(float3(_500, _501, _502), float3(_500, _501, _502)));
                float _507 = _506 * _500;
                float _508 = _506 * _501;
                float _509 = _506 * _502;
                float _512 = mad(_93, _395, mad(_87, _393, _391 * _81));
                float _515 = mad(_94, _395, mad(_88, _393, _391 * _82));
                float _518 = mad(_95, _395, mad(_89, _393, _391 * _83));
                float _522 = dot(float3(_507, _508, _509), float3(_512, _515, _518)) * 2.0f;
                float _526 = _507 - (_522 * _512);
                float _527 = _508 - (_522 * _515);
                float _528 = _509 - (_522 * _518);
                float _538 = sqrt(((_501 * _501) + (_500 * _500)) + (_502 * _502)) * 0.001000000047497451305389404296875f;
                float _549 = ((_538 * _512) + _500) + (_526 * _403);
                float _550 = ((_538 * _515) + _501) + (_527 * _403);
                float _551 = ((_538 * _518) + _502) + (_528 * _403);
                float _567 = mad(_116, _551, mad(_109, _550, _549 * _102)) + _123;
                float _570 = (mad(_115, _551, mad(_108, _550, _549 * _101)) + _122) / _567;
                float _573 = (((mad(_113, _551, mad(_106, _550, _549 * _99)) + _120) / _567) * 0.5f) + 0.5f;
                float _574 = 0.5f - (((mad(_114, _551, mad(_107, _550, _549 * _100)) + _121) / _567) * 0.5f);
                float _577 = _573 * _50_m0[51u].x;
                float _578 = _574 * _50_m0[51u].y;
                float _583 = _549 + (_526 * 0.100000001490116119384765625f);
                float _584 = _550 + (_527 * 0.100000001490116119384765625f);
                float _585 = _551 + (_528 * 0.100000001490116119384765625f);
                float _601 = mad(_116, _585, mad(_109, _584, _583 * _102)) + _123;
                float _610 = _50_m0[51u].x * (((((mad(_113, _585, mad(_106, _584, _583 * _99)) + _120) / _601) * 0.5f) + 0.5f) - _573);
                float _612 = _50_m0[51u].y * ((0.5f - (((mad(_114, _585, mad(_107, _584, _583 * _100)) + _121) / _601) * 0.5f)) - _574);
                float _613 = ((mad(_115, _585, mad(_108, _584, _583 * _101)) + _122) / _601) - _570;
                float _614 = _610 * 10.0f;
                float _616 = _612 * 10.0f;
                float _617 = _613 * 10.0f;
                float _621 = 0.75f / dot(float3(_526, _527, _528), float3(_512, _515, _518));
                float _626 = (_621 * _526) + _549;
                float _627 = (_621 * _527) + _550;
                float _628 = (_621 * _528) + _551;
                float _644 = mad(_116, _628, mad(_109, _627, _626 * _102)) + _123;
                float _653 = _50_m0[51u].x * (((((mad(_113, _628, mad(_106, _627, _626 * _99)) + _120) / _644) * 0.5f) + 0.5f) - _573);
                float _655 = _50_m0[51u].y * ((0.5f - (((mad(_114, _628, mad(_107, _627, _626 * _100)) + _121) / _644) * 0.5f)) - _574);
                float _656 = ((mad(_115, _628, mad(_108, _627, _626 * _101)) + _122) / _644) - _570;
                float _669 = sqrt(((_653 * _653) + (_656 * _656)) + (_655 * _655)) / sqrt(((_614 * _614) + (_617 * _617)) + (_616 * _616));
                float _676 = (_614 != 0.0f) ? (0.100000001490116119384765625f / _610) : 3.4028234663852885981170418348452e+38f;
                float _678 = (_616 != 0.0f) ? (0.100000001490116119384765625f / _612) : 3.4028234663852885981170418348452e+38f;
                float _679 = (_617 != 0.0f) ? (0.100000001490116119384765625f / _613) : 3.4028234663852885981170418348452e+38f;
                float _680 = 1.0f / _428;
                float _681 = 1.0f / _427;
                float _682 = 0.004999999888241291046142578125f / _428;
                float _684 = 0.004999999888241291046142578125f / _427;
                float _693 = float(_614 >= 0.0f);
                float _694 = float(_616 >= 0.0f);
                float _703 = ((_614 < 0.0f) ? ((-0.0f) - _682) : _682) - _577;
                float _706 = ((_616 < 0.0f) ? ((-0.0f) - _684) : _684) - _578;
                float _709 = min((((floor(_577 * _428) + _693) * _680) + _703) * _676, (((floor(_578 * _427) + _694) * _681) + _706) * _678);
                float _713 = (_709 * _614) + _577;
                float _714 = (_709 * _616) + _578;
                float _715 = (_709 * _617) + _570;
                float _718 = _50_m0[50u].x / (_458 - (_715 * _50_m0[50u].y));
                float _729 = max(0.300000011920928955078125f, 10.0f / max(0.00999999977648258209228515625f, max(abs(_610 * 38400.0f), abs(_612 * 21600.0f))));
                uint _762;
                float _766;
                float _768;
                float _770;
                uint _782;
                uint _784;
                uint _764;
                float _772;
                float _774;
                float _776;
                float _778;
                float _780;
                float _786;
                float _788;
                float _790;
                float _792;
                uint _794;
                float _796;
                float _798;
                uint _800;
                uint _761 = 0u;
                uint _763 = 0u;
                float _765 = _715;
                float _767 = _714;
                float _769 = _713;
                float _771 = _709;
                float _773 = _681;
                float _775 = _680;
                float _777 = _427;
                float _779 = _428;
                uint _781 = 0u;
                uint _783 = 0u;
                float _785 = _715;
                float _787 = _714;
                float _789 = _713;
                float _791 = 1.0f;
                uint _793 = 0u;
                float _795 = 0.0f;
                float _797 = 0.0f;
                uint _799 = 1u;
                float _801;
                float _802;
                uint _803;
                uint _804;
                bool _805;
                for (;;)
                {
                    _801 = _779 * _769;
                    _802 = _777 * _767;
                    _803 = uint(int(_801));
                    _804 = uint(int(_802));
                    _805 = _781 == 0u;
                    float _934;
                    if (_805)
                    {
                        _934 = _12.Load(int3(uint2(_803, _804), 0u)).x;
                    }
                    else
                    {
                        _934 = _15.Load(int3(uint2(_803, _804), _781 + 4294967295u)).x;
                    }
                    float _940 = ((_801 >= floor(_779)) || (_802 >= floor(_777))) ? 1.0f : _934;
                    float _954 = (_617 < 0.0f) ? ((_940 - _570) * _679) : 3.4028234663852885981170418348452e+38f;
                    float _956 = min(min((((floor(_801) + _693) * _775) + _703) * _676, (((floor(_802) + _694) * _773) + _706) * _678), _954);
                    bool _957 = _940 < _765;
                    bool _961 = _957 && (asuint(_956) != asuint(_954));
                    float _962 = _957 ? _956 : _771;
                    float _966 = (_962 * _614) + _577;
                    float _967 = (_962 * _616) + _578;
                    float _968 = (_962 * _617) + _570;
                    uint _970 = (_961 ? 1u : 4294967295u) + _781;
                    float _971 = _961 ? 0.5f : 2.0f;
                    float _972 = _971 * _779;
                    float _973 = _971 * _777;
                    float _974 = _961 ? 2.0f : 0.5f;
                    float _975 = _974 * _775;
                    float _976 = _974 * _773;
                    _762 = _761 + 1u;
                    uint _1063;
                    uint _1066;
                    if (int(_970) < int(0u))
                    {
                        float frontier_phi_39_32_ladder;
                        uint frontier_phi_39_32_ladder_1;
                        float frontier_phi_39_32_ladder_2;
                        float frontier_phi_39_32_ladder_3;
                        uint frontier_phi_39_32_ladder_4;
                        float frontier_phi_39_32_ladder_5;
                        float frontier_phi_39_32_ladder_6;
                        float frontier_phi_39_32_ladder_7;
                        uint frontier_phi_39_32_ladder_8;
                        float frontier_phi_39_32_ladder_9;
                        float frontier_phi_39_32_ladder_10;
                        float frontier_phi_39_32_ladder_11;
                        float frontier_phi_39_32_ladder_12;
                        float frontier_phi_39_32_ladder_13;
                        uint frontier_phi_39_32_ladder_14;
                        uint frontier_phi_39_32_ladder_15;
                        float _1026;
                        float _1031;
                        float _1034;
                        bool _1035;
                        for (;;)
                        {
                            float _1024 = _50_m0[50u].w + _50_m0[50u].y;
                            _1026 = _50_m0[50u].x / (_1024 - (_50_m0[50u].y * _940));
                            float _1029 = _50_m0[50u].x / (_1024 - (_50_m0[50u].y * _968));
                            _1031 = abs(_718 - _1029);
                            _1034 = _1029 - _1026;
                            _1035 = _1034 > max(0.00999999977648258209228515625f, _1031 * 0.00999999977648258209228515625f);
                            if (_1035)
                            {
                                uint _1068;
                                if (_763 == 0u)
                                {
                                    uint frontier_phi_45_44_ladder;
                                    if ((_397 == 2u) || (_397 == 4u))
                                    {
                                        if ((_962 < _669) && (abs(_1034) < 2.0f))
                                        {
                                            frontier_phi_39_32_ladder = 0.0f;
                                            frontier_phi_39_32_ladder_1 = _799;
                                            frontier_phi_39_32_ladder_2 = _966;
                                            frontier_phi_39_32_ladder_3 = _967;
                                            frontier_phi_39_32_ladder_4 = 1u;
                                            frontier_phi_39_32_ladder_5 = _789;
                                            frontier_phi_39_32_ladder_6 = _787;
                                            frontier_phi_39_32_ladder_7 = _785;
                                            frontier_phi_39_32_ladder_8 = _970;
                                            frontier_phi_39_32_ladder_9 = _972;
                                            frontier_phi_39_32_ladder_10 = _973;
                                            frontier_phi_39_32_ladder_11 = _975;
                                            frontier_phi_39_32_ladder_12 = _976;
                                            frontier_phi_39_32_ladder_13 = _962;
                                            frontier_phi_39_32_ladder_14 = 1u;
                                            frontier_phi_39_32_ladder_15 = 1u;
                                            break;
                                        }
                                        frontier_phi_45_44_ladder = 1u;
                                    }
                                    else
                                    {
                                        frontier_phi_45_44_ladder = 1u;
                                    }
                                    _1068 = frontier_phi_45_44_ladder;
                                }
                                else
                                {
                                    _1068 = _763;
                                }
                                if (!(_793 == 0u))
                                {
                                    frontier_phi_39_32_ladder = _791;
                                    frontier_phi_39_32_ladder_1 = _799;
                                    frontier_phi_39_32_ladder_2 = _797;
                                    frontier_phi_39_32_ladder_3 = _795;
                                    frontier_phi_39_32_ladder_4 = _793;
                                    frontier_phi_39_32_ladder_5 = _789;
                                    frontier_phi_39_32_ladder_6 = _787;
                                    frontier_phi_39_32_ladder_7 = _785;
                                    frontier_phi_39_32_ladder_8 = _970;
                                    frontier_phi_39_32_ladder_9 = _972;
                                    frontier_phi_39_32_ladder_10 = _973;
                                    frontier_phi_39_32_ladder_11 = _975;
                                    frontier_phi_39_32_ladder_12 = _976;
                                    frontier_phi_39_32_ladder_13 = _962;
                                    frontier_phi_39_32_ladder_14 = _1068;
                                    frontier_phi_39_32_ladder_15 = _783;
                                    break;
                                }
                                bool _1135 = _783 != 0u;
                                frontier_phi_39_32_ladder = _791;
                                frontier_phi_39_32_ladder_1 = _799;
                                frontier_phi_39_32_ladder_2 = _1135 ? _797 : _966;
                                frontier_phi_39_32_ladder_3 = _1135 ? _795 : _967;
                                frontier_phi_39_32_ladder_4 = 0u;
                                frontier_phi_39_32_ladder_5 = _789;
                                frontier_phi_39_32_ladder_6 = _787;
                                frontier_phi_39_32_ladder_7 = _785;
                                frontier_phi_39_32_ladder_8 = 0u;
                                frontier_phi_39_32_ladder_9 = _428;
                                frontier_phi_39_32_ladder_10 = _427;
                                frontier_phi_39_32_ladder_11 = _680;
                                frontier_phi_39_32_ladder_12 = _681;
                                frontier_phi_39_32_ladder_13 = _962 + _729;
                                frontier_phi_39_32_ladder_14 = _1068;
                                frontier_phi_39_32_ladder_15 = ((_397 == 1u) || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _783;
                                break;
                            }
                            else
                            {
                                float _1054 = max(0.100000001490116119384765625f, _1031 * 0.100000001490116119384765625f) * 0.5f;
                                float _1057 = clamp((abs(_1034) - _1054) / _1054, 0.0f, 1.0f);
                                uint _1059 = uint(_1026 < _718);
                                float frontier_phi_39_32_ladder_38_ladder;
                                uint frontier_phi_39_32_ladder_38_ladder_1;
                                float frontier_phi_39_32_ladder_38_ladder_2;
                                float frontier_phi_39_32_ladder_38_ladder_3;
                                uint frontier_phi_39_32_ladder_38_ladder_4;
                                float frontier_phi_39_32_ladder_38_ladder_5;
                                float frontier_phi_39_32_ladder_38_ladder_6;
                                float frontier_phi_39_32_ladder_38_ladder_7;
                                uint frontier_phi_39_32_ladder_38_ladder_8;
                                float frontier_phi_39_32_ladder_38_ladder_9;
                                float frontier_phi_39_32_ladder_38_ladder_10;
                                float frontier_phi_39_32_ladder_38_ladder_11;
                                float frontier_phi_39_32_ladder_38_ladder_12;
                                float frontier_phi_39_32_ladder_38_ladder_13;
                                uint frontier_phi_39_32_ladder_38_ladder_14;
                                uint frontier_phi_39_32_ladder_38_ladder_15;
                                if (_783 == 0u)
                                {
                                    frontier_phi_39_32_ladder_38_ladder = _1057;
                                    frontier_phi_39_32_ladder_38_ladder_1 = _1059;
                                    frontier_phi_39_32_ladder_38_ladder_2 = _797;
                                    frontier_phi_39_32_ladder_38_ladder_3 = _795;
                                    frontier_phi_39_32_ladder_38_ladder_4 = _793;
                                    frontier_phi_39_32_ladder_38_ladder_5 = _789;
                                    frontier_phi_39_32_ladder_38_ladder_6 = _787;
                                    frontier_phi_39_32_ladder_38_ladder_7 = _785;
                                    frontier_phi_39_32_ladder_38_ladder_8 = _970;
                                    frontier_phi_39_32_ladder_38_ladder_9 = _972;
                                    frontier_phi_39_32_ladder_38_ladder_10 = _973;
                                    frontier_phi_39_32_ladder_38_ladder_11 = _975;
                                    frontier_phi_39_32_ladder_38_ladder_12 = _976;
                                    frontier_phi_39_32_ladder_38_ladder_13 = _962;
                                    frontier_phi_39_32_ladder_38_ladder_14 = _763;
                                    frontier_phi_39_32_ladder_38_ladder_15 = uint(_1057 > 0.0f);
                                }
                                else
                                {
                                    frontier_phi_39_32_ladder_38_ladder = _1057;
                                    frontier_phi_39_32_ladder_38_ladder_1 = _1059;
                                    frontier_phi_39_32_ladder_38_ladder_2 = _797;
                                    frontier_phi_39_32_ladder_38_ladder_3 = _795;
                                    frontier_phi_39_32_ladder_38_ladder_4 = _793;
                                    frontier_phi_39_32_ladder_38_ladder_5 = _789;
                                    frontier_phi_39_32_ladder_38_ladder_6 = _787;
                                    frontier_phi_39_32_ladder_38_ladder_7 = _785;
                                    frontier_phi_39_32_ladder_38_ladder_8 = _970;
                                    frontier_phi_39_32_ladder_38_ladder_9 = _972;
                                    frontier_phi_39_32_ladder_38_ladder_10 = _973;
                                    frontier_phi_39_32_ladder_38_ladder_11 = _975;
                                    frontier_phi_39_32_ladder_38_ladder_12 = _976;
                                    frontier_phi_39_32_ladder_38_ladder_13 = _962;
                                    frontier_phi_39_32_ladder_38_ladder_14 = _763;
                                    frontier_phi_39_32_ladder_38_ladder_15 = _783;
                                }
                                frontier_phi_39_32_ladder = frontier_phi_39_32_ladder_38_ladder;
                                frontier_phi_39_32_ladder_1 = frontier_phi_39_32_ladder_38_ladder_1;
                                frontier_phi_39_32_ladder_2 = frontier_phi_39_32_ladder_38_ladder_2;
                                frontier_phi_39_32_ladder_3 = frontier_phi_39_32_ladder_38_ladder_3;
                                frontier_phi_39_32_ladder_4 = frontier_phi_39_32_ladder_38_ladder_4;
                                frontier_phi_39_32_ladder_5 = frontier_phi_39_32_ladder_38_ladder_5;
                                frontier_phi_39_32_ladder_6 = frontier_phi_39_32_ladder_38_ladder_6;
                                frontier_phi_39_32_ladder_7 = frontier_phi_39_32_ladder_38_ladder_7;
                                frontier_phi_39_32_ladder_8 = frontier_phi_39_32_ladder_38_ladder_8;
                                frontier_phi_39_32_ladder_9 = frontier_phi_39_32_ladder_38_ladder_9;
                                frontier_phi_39_32_ladder_10 = frontier_phi_39_32_ladder_38_ladder_10;
                                frontier_phi_39_32_ladder_11 = frontier_phi_39_32_ladder_38_ladder_11;
                                frontier_phi_39_32_ladder_12 = frontier_phi_39_32_ladder_38_ladder_12;
                                frontier_phi_39_32_ladder_13 = frontier_phi_39_32_ladder_38_ladder_13;
                                frontier_phi_39_32_ladder_14 = frontier_phi_39_32_ladder_38_ladder_14;
                                frontier_phi_39_32_ladder_15 = frontier_phi_39_32_ladder_38_ladder_15;
                                break;
                            }
                        }
                        _800 = frontier_phi_39_32_ladder_1;
                        _798 = frontier_phi_39_32_ladder_2;
                        _796 = frontier_phi_39_32_ladder_3;
                        _794 = frontier_phi_39_32_ladder_4;
                        _792 = frontier_phi_39_32_ladder;
                        _790 = frontier_phi_39_32_ladder_5;
                        _788 = frontier_phi_39_32_ladder_6;
                        _786 = frontier_phi_39_32_ladder_7;
                        _1063 = frontier_phi_39_32_ladder_15;
                        _1066 = frontier_phi_39_32_ladder_8;
                        _780 = frontier_phi_39_32_ladder_9;
                        _778 = frontier_phi_39_32_ladder_10;
                        _776 = frontier_phi_39_32_ladder_11;
                        _774 = frontier_phi_39_32_ladder_12;
                        _772 = frontier_phi_39_32_ladder_13;
                        _764 = frontier_phi_39_32_ladder_14;
                    }
                    else
                    {
                        bool _1036 = _783 != 0u;
                        _800 = _799;
                        _798 = _797;
                        _796 = _795;
                        _794 = _793;
                        _792 = _791;
                        _790 = _1036 ? _789 : _966;
                        _788 = _1036 ? _787 : _967;
                        _786 = _1036 ? _785 : _968;
                        _1063 = _783;
                        _1066 = _970;
                        _780 = _972;
                        _778 = _973;
                        _776 = _975;
                        _774 = _976;
                        _772 = _962;
                        _764 = _763;
                    }
                    float frontier_phi_55_pred;
                    uint frontier_phi_55_pred_1;
                    uint frontier_phi_55_pred_2;
                    float frontier_phi_55_pred_3;
                    float frontier_phi_55_pred_4;
                    bool _1071;
                    bool _1073;
                    for (;;)
                    {
                        _1071 = _968 < 0.0f;
                        _1073 = _1071 || ((_966 < 0.0f) || (_967 < 0.0f));
                        if (!_1073)
                        {
                            if (!((_968 > 1.0f) || ((_966 > _50_m0[51u].x) || (_967 > _50_m0[51u].y))))
                            {
                                frontier_phi_55_pred = _966;
                                frontier_phi_55_pred_1 = _1063;
                                frontier_phi_55_pred_2 = _1066;
                                frontier_phi_55_pred_3 = _967;
                                frontier_phi_55_pred_4 = _968;
                                break;
                            }
                        }
                        if (!_1071)
                        {
                            frontier_phi_55_pred = _966;
                            frontier_phi_55_pred_1 = 1u;
                            frontier_phi_55_pred_2 = 4294967295u;
                            frontier_phi_55_pred_3 = _967;
                            frontier_phi_55_pred_4 = _968;
                            break;
                        }
                        float _1144 = (-0.0f) - _968;
                        float _1145 = _1144 / _617;
                        frontier_phi_55_pred = (_1145 * _614) + _966;
                        frontier_phi_55_pred_1 = 1u;
                        frontier_phi_55_pred_2 = 4294967295u;
                        frontier_phi_55_pred_3 = (_1145 * _616) + _967;
                        frontier_phi_55_pred_4 = _1144 + _968;
                        break;
                    }
                    _770 = frontier_phi_55_pred;
                    _784 = frontier_phi_55_pred_1;
                    _782 = frontier_phi_55_pred_2;
                    _768 = frontier_phi_55_pred_3;
                    _766 = frontier_phi_55_pred_4;
                    if ((_762 < 128u) && (int(_782) > int(4294967295u)))
                    {
                        _761 = _762;
                        _763 = _764;
                        _765 = _766;
                        _767 = _768;
                        _769 = _770;
                        _771 = _772;
                        _773 = _774;
                        _775 = _776;
                        _777 = _778;
                        _779 = _780;
                        _781 = _782;
                        _783 = _784;
                        _785 = _786;
                        _787 = _788;
                        _789 = _790;
                        _791 = _792;
                        _793 = _794;
                        _795 = _796;
                        _797 = _798;
                        _799 = _800;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                float _1172 = ((_470 * _470) * _399) * (3.0f - (_470 * 2.0f));
                bool _1173 = dot(float3(_526, _527, _528), float3(_507, _508, _509)) < 0.0f;
                bool _1174 = _762 > 127u;
                uint _1175 = _1174 ? 1u : _784;
                float _1184 = _50_m0[51u].z * 2.0f;
                float _1187 = (_1184 * _577) + (-1.0f);
                float _1188 = ((1.0f - (_50_m0[51u].w * _578)) * 2.0f) + (-1.0f);
                float _1204 = mad(_190, _570, mad(_183, _1188, _1187 * _176)) + _197;
                float _1205 = (mad(_187, _570, mad(_180, _1188, _1187 * _173)) + _194) / _1204;
                float _1206 = (mad(_188, _570, mad(_181, _1188, _1187 * _174)) + _195) / _1204;
                float _1207 = (mad(_189, _570, mad(_182, _1188, _1187 * _175)) + _196) / _1204;
                float _1212 = (_1184 * _770) + (-1.0f);
                float _1213 = ((1.0f - (_50_m0[51u].w * _768)) * 2.0f) + (-1.0f);
                float _1229 = mad(_190, _766, mad(_183, _1213, _1212 * _176)) + _197;
                float _1233 = ((mad(_187, _766, mad(_180, _1213, _1212 * _173)) + _194) / _1229) - _1205;
                float _1234 = ((mad(_188, _766, mad(_181, _1213, _1212 * _174)) + _195) / _1229) - _1206;
                float _1235 = ((mad(_189, _766, mad(_182, _1213, _1212 * _175)) + _196) / _1229) - _1207;
                float _1246;
                uint _1248;
                float _1250;
                if (_762 < 129u)
                {
                    float frontier_phi_64_63_ladder;
                    uint frontier_phi_64_63_ladder_1;
                    float frontier_phi_64_63_ladder_2;
                    if ((_770 < 0.0f) || (_768 < 0.0f))
                    {
                        frontier_phi_64_63_ladder = 0.0f;
                        frontier_phi_64_63_ladder_1 = _1175;
                        frontier_phi_64_63_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_64_63_ladder_67_ladder;
                        uint frontier_phi_64_63_ladder_67_ladder_1;
                        float frontier_phi_64_63_ladder_67_ladder_2;
                        if ((_766 >= 1.0f) || ((_770 > _50_m0[51u].x) || (_768 > _50_m0[51u].y)))
                        {
                            frontier_phi_64_63_ladder_67_ladder = 0.0f;
                            frontier_phi_64_63_ladder_67_ladder_1 = _1175;
                            frontier_phi_64_63_ladder_67_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_64_63_ladder_67_ladder_72_ladder;
                            uint frontier_phi_64_63_ladder_67_ladder_72_ladder_1;
                            float frontier_phi_64_63_ladder_67_ladder_72_ladder_2;
                            for (;;)
                            {
                                if ((abs(_770 - _254) < (2.0f / _428)) && (abs(_768 - _255) < (2.0f / _427)))
                                {
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder = 0.0f;
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder_1 = _1175;
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_101;
                                    float frontier_phi_101_pred;
                                    uint frontier_phi_101_pred_1;
                                    float frontier_phi_101_pred_2;
                                    uint _1458;
                                    uint _1459;
                                    bool _1461;
                                    for (;;)
                                    {
                                        _1458 = uint(int(_770 * _428));
                                        _1459 = uint(int(_768 * _427));
                                        _1461 = (_397 == 1u) && _1173;
                                        if (!_1461)
                                        {
                                            if (!(dot(float3(_1233, _1234, _1235), float3(_1233, _1234, _1235)) < 0.000899999984540045261383056640625f))
                                            {
                                                ladder_phi_101 = false;
                                                frontier_phi_101_pred = 0.0f;
                                                frontier_phi_101_pred_1 = _1175;
                                                frontier_phi_101_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1515 = _24[22u].Load(int3(uint2(_1458, _1459), 0u));
                                        uint _1517 = _1515.x;
                                        float _1835;
                                        float _1836;
                                        float _1837;
                                        if (_1517 == 0u)
                                        {
                                            uint4 _1637 = _24[1u].Load(int3(uint2(_1458, _1459), 0u));
                                            uint _1639 = _1637.x;
                                            float _1647 = (float((_1639 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1648 = (float(_1639 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1652 = (1.0f - abs(_1647)) - abs(_1648);
                                            float _1654 = clamp((-0.0f) - _1652, 0.0f, 1.0f);
                                            float _1655 = (-0.0f) - _1654;
                                            _1835 = ((_1647 >= 0.0f) ? _1655 : _1654) + _1647;
                                            _1836 = ((_1648 >= 0.0f) ? _1655 : _1654) + _1648;
                                            _1837 = _1652;
                                        }
                                        else
                                        {
                                            float _1669 = (float((_1517 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1670 = (float(_1517 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1674 = (1.0f - abs(_1669)) - abs(_1670);
                                            float _1676 = clamp((-0.0f) - _1674, 0.0f, 1.0f);
                                            float _1677 = (-0.0f) - _1676;
                                            _1835 = ((_1669 >= 0.0f) ? _1677 : _1676) + _1669;
                                            _1836 = ((_1670 >= 0.0f) ? _1677 : _1676) + _1670;
                                            _1837 = _1674;
                                        }
                                        float _1841 = rsqrt(dot(float3(_1835, _1836, _1837), float3(_1835, _1836, _1837)));
                                        if (dot(float3(_1841 * _1835, _1841 * _1836, _1841 * _1837), float3(_1233, _1234, _1235)) > 0.0f)
                                        {
                                            ladder_phi_101 = true;
                                            frontier_phi_101_pred = 0.0f;
                                            frontier_phi_101_pred_1 = _1175;
                                            frontier_phi_101_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_101 = false;
                                            frontier_phi_101_pred = 0.0f;
                                            frontier_phi_101_pred_1 = _1175;
                                            frontier_phi_101_pred_2 = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_101)
                                    {
                                        frontier_phi_64_63_ladder_67_ladder_72_ladder = frontier_phi_101_pred;
                                        frontier_phi_64_63_ladder_67_ladder_72_ladder_1 = frontier_phi_101_pred_1;
                                        frontier_phi_64_63_ladder_67_ladder_72_ladder_2 = frontier_phi_101_pred_2;
                                        break;
                                    }
                                    float _1688 = _50_m0[51u].z * _770;
                                    float _1689 = _50_m0[51u].w * _768;
                                    float _1691 = (_427 / _428) * 0.0500000007450580596923828125f;
                                    float _1696 = clamp(_1688 / _1691, 0.0f, 1.0f);
                                    float _1697 = clamp(_1689 * 20.0f, 0.0f, 1.0f);
                                    float _1709 = clamp(((_1688 + (-1.0f)) + _1691) / _1691, 0.0f, 1.0f);
                                    float _1710 = clamp((_1689 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1721 = _1696 * _1697;
                                    precise float _1722 = _1721 * _1721;
                                    float _1726 = ((((3.0f - (_1697 * 2.0f)) * (3.0f - (_1696 * 2.0f))) * _1722) * (1.0f - ((_1709 * _1709) * (3.0f - (_1709 * 2.0f))))) * (1.0f - ((_1710 * _1710) * (3.0f - (_1710 * 2.0f))));
                                    bool _1729 = (_1175 != 0u) || (_1726 >= 1.0f);
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder = _1726 * float(_1172 > 0.0f);
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder_1 = _1729 ? _1175 : 1u;
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder_2 = _1729 ? 0.0f : _1726;
                                    break;
                                }
                            }
                            frontier_phi_64_63_ladder_67_ladder = frontier_phi_64_63_ladder_67_ladder_72_ladder;
                            frontier_phi_64_63_ladder_67_ladder_1 = frontier_phi_64_63_ladder_67_ladder_72_ladder_1;
                            frontier_phi_64_63_ladder_67_ladder_2 = frontier_phi_64_63_ladder_67_ladder_72_ladder_2;
                        }
                        frontier_phi_64_63_ladder = frontier_phi_64_63_ladder_67_ladder;
                        frontier_phi_64_63_ladder_1 = frontier_phi_64_63_ladder_67_ladder_1;
                        frontier_phi_64_63_ladder_2 = frontier_phi_64_63_ladder_67_ladder_2;
                    }
                    _1246 = frontier_phi_64_63_ladder_2;
                    _1248 = frontier_phi_64_63_ladder_1;
                    _1250 = frontier_phi_64_63_ladder;
                }
                else
                {
                    _1246 = 0.0f;
                    _1248 = _1175;
                    _1250 = 0.0f;
                }
                uint _1361;
                float _1362;
                float _1365;
                float _1366;
                float _1367;
                float _1369;
                float _1291;
                float _1294;
                float _1297;
                float _1300;
                float _1304;
                float _1305;
                for (;;)
                {
                    _1291 = ((((exp2(log2(clamp((sqrt(((_1206 * _1206) + (_1205 * _1205)) + (_1207 * _1207)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _407) * exp2(log2(clamp((_1206 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_393, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    _1294 = mad(_167, _528, mad(_161, _527, _526 * _155));
                    _1297 = mad(_168, _528, mad(_162, _527, _526 * _156));
                    _1300 = mad(_169, _528, mad(_163, _527, _526 * _157));
                    bool _1303 = (_1248 != 0u) || (_1250 < 1.0f);
                    _1304 = _1303 ? 0.0f : 1.0f;
                    _1305 = _1303 ? 0.0f : 0.5f;
                    if (_1303)
                    {
                        bool _1356 = _397 == 1u;
                        uint4 _1360 = asuint(_50_m0[60u]);
                        if (_1356)
                        {
                            if (int(_405) < int(1u))
                            {
                                if (_1360.x == 0u)
                                {
                                    _1361 = 0u;
                                    _1362 = 9899999600270360182784.0f;
                                    _1365 = 0.0f;
                                    _1366 = 0.0f;
                                    _1367 = 0.0f;
                                    _1369 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1360.y == 0u)
                                {
                                    _1361 = 0u;
                                    _1362 = 9899999600270360182784.0f;
                                    _1365 = 0.0f;
                                    _1366 = 0.0f;
                                    _1367 = 0.0f;
                                    _1369 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1360.z == 0u)
                            {
                                _1361 = 0u;
                                _1362 = 9899999600270360182784.0f;
                                _1365 = 0.0f;
                                _1366 = 0.0f;
                                _1367 = 0.0f;
                                _1369 = 0.0f;
                                break;
                            }
                        }
                        if (_1246 > 0.0f)
                        {
                            _1361 = 0u;
                            _1362 = 9899999600270360182784.0f;
                            _1365 = 0.0f;
                            _1366 = 1.0f;
                            _1367 = 1000.0f;
                            _1369 = 0.5f;
                            break;
                        }
                        if (((_790 <= 0.0f) || (_788 <= 0.0f)) || (_786 <= 0.0f))
                        {
                            _1361 = 0u;
                            _1362 = 9899999600270360182784.0f;
                            _1365 = 0.0f;
                            _1366 = 1.0f;
                            _1367 = 1000.0f;
                            _1369 = 0.5f;
                            break;
                        }
                        if ((_786 >= 1.0f) || ((_790 >= _50_m0[51u].x) || (_788 >= _50_m0[51u].y)))
                        {
                            _1361 = 0u;
                            _1362 = 9899999600270360182784.0f;
                            _1365 = 0.0f;
                            _1366 = 1.0f;
                            _1367 = 1000.0f;
                            _1369 = 0.5f;
                            break;
                        }
                        uint _2129;
                        uint _2131;
                        uint _1853;
                        uint _1854;
                        bool _1860;
                        for (;;)
                        {
                            _1853 = uint(clamp(_798, 0.0f, 1.0f) * _202);
                            _1854 = uint(clamp(_796, 0.0f, 1.0f) * _204);
                            _1860 = _20[21u].Load(int3(uint2(_1853, _1854), 0u)).x > 0.0f;
                            if (_1860)
                            {
                                uint _2004 = _24[23u].Load(int3(uint2(_1853, _1854), 0u)).y + 4294967295u;
                                _2129 = (uint(int(_2004) >> int(31u)) & 3u) + 1u;
                                _2131 = (int(_2004) < int(0u)) ? 0u : _2004;
                                break;
                            }
                            else
                            {
                                uint4 _2012 = _24[2u].Load(int3(uint2(_1853, _1854), 0u));
                                uint _2015 = _2012.w;
                                uint4 _2020 = _24[15u].Load(int3(uint2(_1853, _1854), 0u));
                                uint _2022 = _2020.y;
                                uint _2028 = ((_2022 & 64u) != 0u) ? uint((_2022 & 4294967167u) != 66u) : 4294967295u;
                                uint _2029 = _2015 & 128u;
                                uint _2031 = (_2029 != 0u) ? 1u : ((_2012.x << 7u) | _2015);
                                uint4 _2034 = _16.Load(_2031 * 4u);
                                uint _2035 = _2034.x;
                                uint _2042 = ((_2035 & 1u) != 0u) ? 0u : 18u;
                                uint _2044 = uint(min(int(uint(max(int(_2028), int(0u)))), int(1u)));
                                uint _2142;
                                if (_2029 == 0u)
                                {
                                    _2142 = (((_2035 & 2097152u) != 0u) && (_2028 == _2044)) ? (_2042 | 128u) : _2042;
                                }
                                else
                                {
                                    _2142 = _2015;
                                }
                                uint _2143 = _16.Load((_2031 * 4u) + 1u).x & 512u;
                                bool _2146 = (_2142 & 144u) == 0u;
                                if (_2143 == 0u)
                                {
                                    if (_2146 || ((_2035 & 1u) != 0u))
                                    {
                                        _2129 = 0u;
                                        _2131 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2146)
                                    {
                                        _2129 = 0u;
                                        _2131 = 0u;
                                        break;
                                    }
                                }
                                bool _2290 = ((_2142 & 128u) | _2143) != 0u;
                                uint _2130;
                                if (_2290)
                                {
                                    _2130 = 1u;
                                }
                                else
                                {
                                    _2130 = (((_2035 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_2035 & 268435472u) == 16u) && (((_2035 & 33554432u) == 0u) || _2290))
                                {
                                    _2129 = 2u;
                                    _2131 = 0u;
                                    break;
                                }
                                _2129 = _2130;
                                _2131 = (_2130 == 1u) ? _2044 : 0u;
                                break;
                            }
                        }
                        if ((_397 != _2129) || (_405 != _2131))
                        {
                            _1361 = 0u;
                            _1362 = 9899999600270360182784.0f;
                            _1365 = 0.0f;
                            _1366 = 1.0f;
                            _1367 = 1000.0f;
                            _1369 = 0.5f;
                            break;
                        }
                        float _2212 = _790 * 2.0f;
                        float _2215 = (_50_m0[51u].z * _2212) + (-1.0f);
                        float _2216 = ((1.0f - (_50_m0[51u].w * _788)) * 2.0f) + (-1.0f);
                        float _2232 = mad(_190, _786, mad(_183, _2216, _2215 * _176)) + _197;
                        float _2233 = (mad(_187, _786, mad(_180, _2216, _2215 * _173)) + _194) / _2232;
                        float _2234 = (mad(_188, _786, mad(_181, _2216, _2215 * _174)) + _195) / _2232;
                        float _2235 = (mad(_189, _786, mad(_182, _2216, _2215 * _175)) + _196) / _2232;
                        if (sqrt(((_2234 * _2234) + (_2233 * _2233)) + (_2235 * _2235)) > _50_m0[58u].w)
                        {
                            _1361 = 0u;
                            _1362 = 9899999600270360182784.0f;
                            _1365 = 0.0f;
                            _1366 = 0.0f;
                            _1367 = 1000.0f;
                            _1369 = 0.5f;
                            break;
                        }
                        float _2270 = _2233 - _1205;
                        float _2271 = _2234 - _1206;
                        float _2272 = _2235 - _1207;
                        float _2278 = sqrt(((_2271 * _2271) + (_2270 * _2270)) + (_2272 * _2272));
                        float _2286 = min(_50_m0[59u].y, max(0.0f, _2278 + (-0.001000000047497451305389404296875f)));
                        float _2337;
                        if (_1356)
                        {
                            _2337 = min(_50_m0[59u].x, _2286 + 10.0f);
                        }
                        else
                        {
                            _2337 = _50_m0[59u].x;
                        }
                        float _2338 = _2337 - _2278;
                        if (!(_2338 > 0.0f))
                        {
                            _1361 = 0u;
                            _1362 = 9899999600270360182784.0f;
                            _1365 = 1.0f;
                            _1366 = 1.0f;
                            _1367 = 0.0f;
                            _1369 = 0.5f;
                            break;
                        }
                        float _2365 = _2233 - (_2286 * _1294);
                        float _2366 = _2234 - (_2286 * _1297);
                        float _2367 = _2235 - (_2286 * _1300);
                        RayDesc _2ident = {float3(mad(_2367, _50_m0[46u].z, mad(_2366, _50_m0[46u].y, _50_m0[46u].x * _2365)) + _50_m0[46u].w, mad(_2367, _50_m0[47u].z, mad(_2366, _50_m0[47u].y, _50_m0[47u].x * _2365)) + _50_m0[47u].w, mad(_2367, _50_m0[48u].z, mad(_2366, _50_m0[48u].y, _50_m0[48u].x * _2365)) + _50_m0[48u].w), 0.0f, float3(mad(_1300, _50_m0[46u].z, mad(_1297, _50_m0[46u].y, _50_m0[46u].x * _1294)), mad(_1300, _50_m0[47u].z, mad(_1297, _50_m0[47u].y, _50_m0[47u].x * _1294)), mad(_1300, _50_m0[48u].z, mad(_1297, _50_m0[48u].y, _50_m0[48u].x * _1294))), _2338};
                        _2370.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2417 = _2370.Proceed();
                        uint _2418 = _2370.CommittedStatus();
                        if (!(_2418 == 1u))
                        {
                            _1361 = 0u;
                            _1362 = 9899999600270360182784.0f;
                            _1365 = 1.0f;
                            _1366 = 0.0f;
                            _1367 = 0.0f;
                            _1369 = 0.5f;
                            break;
                        }
                        float _2423 = _2370.CommittedRayT();
                        if (!((_2423 < _2338) && (_2423 > 0.0f)))
                        {
                            _1361 = 0u;
                            _1362 = 9899999600270360182784.0f;
                            _1365 = 1.0f;
                            _1366 = 0.0f;
                            _1367 = 0.0f;
                            _1369 = 0.5f;
                            break;
                        }
                        float _2435 = (_50_m0[51u].z * _2212) + (-1.0f);
                        float _2436 = ((1.0f - (_50_m0[51u].w * _788)) * 2.0f) + (-1.0f);
                        float _2452 = mad(_144, _786, mad(_137, _2436, _2435 * _130)) + _151;
                        float _2456 = _2423 - _2286;
                        float _2460 = ((mad(_141, _786, mad(_134, _2436, _2435 * _127)) + _148) / _2452) + (_2456 * _526);
                        float _2461 = ((mad(_142, _786, mad(_135, _2436, _2435 * _128)) + _149) / _2452) + (_2456 * _527);
                        float _2462 = ((mad(_143, _786, mad(_136, _2436, _2435 * _129)) + _150) / _2452) + (_2456 * _528);
                        float _2474 = mad(_116, _2462, mad(_109, _2461, _2460 * _102)) + _123;
                        float _2484 = (_50_m0[51u].z * _50_m0[51u].x) * ((((mad(_113, _2462, mad(_106, _2461, _2460 * _99)) + _120) / _2474) * 0.5f) + 0.5f);
                        float _2486 = (_50_m0[51u].w * _50_m0[51u].y) * (0.5f - (((mad(_114, _2462, mad(_107, _2461, _2460 * _100)) + _121) / _2474) * 0.5f));
                        float _2488 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2491 = clamp(_2484 / _2488, 0.0f, 1.0f);
                        float _2492 = clamp(_2486 * 20.0f, 0.0f, 1.0f);
                        float _2502 = clamp(((_2488 + (-1.0f)) + _2484) / _2488, 0.0f, 1.0f);
                        float _2503 = clamp((_2486 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2514 = _2491 * _2492;
                        precise float _2515 = _2514 * _2514;
                        if ((((((3.0f - (_2492 * 2.0f)) * (3.0f - (_2491 * 2.0f))) * _2515) * (1.0f - ((_2502 * _2502) * (3.0f - (_2502 * 2.0f))))) * (1.0f - ((_2503 * _2503) * (3.0f - (_2503 * 2.0f))))) < 1.0f)
                        {
                            _1361 = 0u;
                            _1362 = 9899999600270360182784.0f;
                            _1365 = _1304;
                            _1366 = _1304;
                            _1367 = 0.0f;
                            _1369 = _1305;
                            break;
                        }
                        _1361 = 1u;
                        _1362 = (_2278 - _2286) + _2423;
                        _1365 = 0.0f;
                        _1366 = 1.0f;
                        _1367 = 0.0f;
                        _1369 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1361 = 0u;
                        _1362 = 9899999600270360182784.0f;
                        _1365 = 1.0f;
                        _1366 = 1.0f;
                        _1367 = 0.0f;
                        _1369 = 0.5f;
                        break;
                    }
                }
                float _1390 = clamp(log2(_50_m0[61u].w * exp2(log2((1.0f - _401) * 0.75f) * 1.5f)) / float(asuint(_50_m0[55u]).x + 4294967295u), 0.0f, 1.0f);
                bool _1391 = _397 == 1u;
                float _1440;
                if (_1391)
                {
                    float _1473;
                    if (_1361 == 0u)
                    {
                        _1473 = sqrt(((_1234 * _1234) + (_1233 * _1233)) + (_1235 * _1235));
                    }
                    else
                    {
                        _1473 = _1362;
                    }
                    float _1477 = clamp((_1473 + (-1.0f)) * 0.111111111938953399658203125f, 0.0f, 1.0f);
                    _1440 = (1.0f - ((_1477 * _1477) * (3.0f - (_1477 * 2.0f)))) * _50_m0[61u].z;
                }
                else
                {
                    _1440 = 1.0f;
                }
                float _1442 = _1440 * _1250;
                bool _1443 = _397 != 1u;
                float _1861;
                float _1863;
                float _1865;
                float _1867;
                if (_1442 == 0.0f)
                {
                    float _1760;
                    float _1762;
                    float _1764;
                    float _1766;
                    if (_1361 == 0u)
                    {
                        float frontier_phi_104_92_ladder;
                        float frontier_phi_104_92_ladder_1;
                        float frontier_phi_104_92_ladder_2;
                        float frontier_phi_104_92_ladder_3;
                        if (_1443)
                        {
                            float _1741 = _1297 * _1291;
                            float _1745 = rsqrt(dot(float3(_1294, _1741, _1300), float3(_1294, _1741, _1300)));
                            float4 _1755 = _28[4u].SampleLevel(_59, float3(_1745 * _1294, _1745 * _1741, _1745 * _1300), 0.0f);
                            frontier_phi_104_92_ladder = 1.0f;
                            frontier_phi_104_92_ladder_1 = _1755.z;
                            frontier_phi_104_92_ladder_2 = _1755.y;
                            frontier_phi_104_92_ladder_3 = _1755.x;
                        }
                        else
                        {
                            frontier_phi_104_92_ladder = 0.0f;
                            frontier_phi_104_92_ladder_1 = 0.0f;
                            frontier_phi_104_92_ladder_2 = 0.0f;
                            frontier_phi_104_92_ladder_3 = 0.0f;
                        }
                        _1760 = frontier_phi_104_92_ladder_3;
                        _1762 = frontier_phi_104_92_ladder_2;
                        _1764 = frontier_phi_104_92_ladder_1;
                        _1766 = frontier_phi_104_92_ladder;
                    }
                    else
                    {
                        float frontier_phi_104_93_ladder;
                        float frontier_phi_104_93_ladder_1;
                        float frontier_phi_104_93_ladder_2;
                        float frontier_phi_104_93_ladder_3;
                        if (_1443)
                        {
                            float _1771 = _1297 * _1291;
                            float _1775 = rsqrt(dot(float3(_1294, _1771, _1300), float3(_1294, _1771, _1300)));
                            float4 _1783 = _28[4u].SampleLevel(_59, float3(_1775 * _1294, _1775 * _1771, _1775 * _1300), 0.0f);
                            float _1785 = _1783.x;
                            float _1786 = _1783.y;
                            float _1787 = _1783.z;
                            frontier_phi_104_93_ladder = 1.0f;
                            frontier_phi_104_93_ladder_1 = _1787 - (_1787 * _1440);
                            frontier_phi_104_93_ladder_2 = _1786 - (_1786 * _1440);
                            frontier_phi_104_93_ladder_3 = _1785 - (_1785 * _1440);
                        }
                        else
                        {
                            frontier_phi_104_93_ladder = _1440;
                            frontier_phi_104_93_ladder_1 = 0.0f;
                            frontier_phi_104_93_ladder_2 = 0.0f;
                            frontier_phi_104_93_ladder_3 = 0.0f;
                        }
                        _1760 = frontier_phi_104_93_ladder_3;
                        _1762 = frontier_phi_104_93_ladder_2;
                        _1764 = frontier_phi_104_93_ladder_1;
                        _1766 = frontier_phi_104_93_ladder;
                    }
                    _1861 = _1760 * _1172;
                    _1863 = _1762 * _1172;
                    _1865 = _1764 * _1172;
                    _1867 = _1766 * _1172;
                }
                else
                {
                    uint4 _1487 = asuint(_55_m0[0u]);
                    float _1806;
                    float _1808;
                    float _1813;
                    float _1817;
                    if (_12.Load(int3(uint2(uint(float(_1487.x) * _770), uint(float(_1487.y) * _768)), 0u)).x > 0.0f)
                    {
                        uint _1530_dummy_parameter;
                        uint2 _1530 = spvTextureSize(_14, 0u, _1530_dummy_parameter);
                        float4 _1539 = _14.Load(int3(uint2(uint(float(_1530.x) * _770), uint(float(_1530.y) * _768)), 0u));
                        float _1543 = _1539.x * 0.5f;
                        float _1544 = _1539.y * (-0.5f);
                        float4 _1563 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _1543) + (_50_m0[52u].x * _770), (_50_m0[52u].w * _1544) + (_50_m0[52u].y * _768)), 0.0f);
                        float _1802;
                        if (_1391)
                        {
                            float frontier_phi_107_106_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _570))) < 5.0f)
                            {
                                float _1921 = sqrt((_1543 * _1543) + (_1544 * _1544));
                                float frontier_phi_107_106_ladder_115_ladder;
                                if (_1921 > 0.0500000007450580596923828125f)
                                {
                                    float _2053 = _770 - _577;
                                    float _2054 = _768 - _578;
                                    float frontier_phi_107_106_ladder_115_ladder_127_ladder;
                                    if (_1921 > sqrt((_2054 * _2054) + (_2053 * _2053)))
                                    {
                                        uint4 _2156 = asuint(_55_m0[0u]);
                                        uint _2163 = uint(float(_2156.x) * _770);
                                        uint _2164 = uint(float(_2156.y) * _768);
                                        uint4 _2167 = _24[2u].Load(int3(uint2(_2163, _2164), 0u));
                                        uint _2170 = _2167.w;
                                        uint4 _2175 = _24[15u].Load(int3(uint2(_2163, _2164), 0u));
                                        uint _2177 = _2175.y;
                                        uint _2183 = ((_2177 & 64u) != 0u) ? uint((_2177 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2184 = _2170 & 128u;
                                        uint _2186 = (_2184 != 0u) ? 1u : ((_2167.x << 7u) | _2170);
                                        uint4 _2189 = _16.Load(_2186 * 4u);
                                        uint _2190 = _2189.x;
                                        uint _2197 = ((_2190 & 1u) != 0u) ? 0u : 18u;
                                        uint _2265;
                                        if (_2184 == 0u)
                                        {
                                            _2265 = (((_2190 & 2097152u) != 0u) && (_2183 == uint(min(int(uint(max(int(_2183), int(0u)))), int(1u))))) ? (_2197 | 128u) : _2197;
                                        }
                                        else
                                        {
                                            _2265 = _2170;
                                        }
                                        float frontier_phi_107_106_ladder_115_ladder_127_ladder_145_ladder;
                                        if (((_2265 & 128u) | (_16.Load((_2186 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2293 = asuint(_55_m0[0u]);
                                            uint _2302 = uint(float(_2293.x) * (_1543 + _770));
                                            uint _2303 = uint(float(_2293.y) * (_1544 + _768));
                                            uint4 _2306 = _24[2u].Load(int3(uint2(_2302, _2303), 0u));
                                            uint _2309 = _2306.w;
                                            uint4 _2312 = _24[15u].Load(int3(uint2(_2302, _2303), 0u));
                                            uint _2314 = _2312.y;
                                            uint _2320 = ((_2314 & 64u) != 0u) ? uint((_2314 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2321 = _2309 & 128u;
                                            uint _2323 = (_2321 != 0u) ? 1u : ((_2306.x << 7u) | _2309);
                                            uint4 _2325 = _16.Load(_2323 * 4u);
                                            uint _2326 = _2325.x;
                                            uint _2333 = ((_2326 & 1u) != 0u) ? 0u : 18u;
                                            uint _2359;
                                            if (_2321 == 0u)
                                            {
                                                _2359 = (((_2326 & 2097152u) != 0u) && (_2320 == uint(min(int(uint(max(int(_2320), int(0u)))), int(1u))))) ? (_2333 | 128u) : _2333;
                                            }
                                            else
                                            {
                                                _2359 = _2309;
                                            }
                                            float frontier_phi_107_106_ladder_115_ladder_127_ladder_145_ladder_154_ladder;
                                            if ((_2359 & 128u) == 0u)
                                            {
                                                frontier_phi_107_106_ladder_115_ladder_127_ladder_145_ladder_154_ladder = ((_16.Load((_2323 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_107_106_ladder_115_ladder_127_ladder_145_ladder_154_ladder = 0.0f;
                                            }
                                            frontier_phi_107_106_ladder_115_ladder_127_ladder_145_ladder = frontier_phi_107_106_ladder_115_ladder_127_ladder_145_ladder_154_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_107_106_ladder_115_ladder_127_ladder_145_ladder = 1.0f;
                                        }
                                        frontier_phi_107_106_ladder_115_ladder_127_ladder = frontier_phi_107_106_ladder_115_ladder_127_ladder_145_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_107_106_ladder_115_ladder_127_ladder = 1.0f;
                                    }
                                    frontier_phi_107_106_ladder_115_ladder = frontier_phi_107_106_ladder_115_ladder_127_ladder;
                                }
                                else
                                {
                                    frontier_phi_107_106_ladder_115_ladder = 1.0f;
                                }
                                frontier_phi_107_106_ladder = frontier_phi_107_106_ladder_115_ladder;
                            }
                            else
                            {
                                frontier_phi_107_106_ladder = 1.0f;
                            }
                            _1802 = frontier_phi_107_106_ladder;
                        }
                        else
                        {
                            _1802 = 1.0f;
                        }
                        float _1804 = _1802 * _1442;
                        float _1812;
                        float _1816;
                        float _1820;
                        if (_794 == 0u)
                        {
                            _1812 = _50_m0[54u].x * _1563.x;
                            _1816 = _50_m0[54u].x * _1563.y;
                            _1820 = _50_m0[54u].x * _1563.z;
                        }
                        else
                        {
                            _1812 = 0.0f;
                            _1816 = 0.0f;
                            _1820 = 0.0f;
                        }
                        float frontier_phi_108_116_ladder;
                        float frontier_phi_108_116_ladder_1;
                        float frontier_phi_108_116_ladder_2;
                        float frontier_phi_108_116_ladder_3;
                        for (;;)
                        {
                            if (_792 > 0.0f)
                            {
                                float _1811;
                                float _1815;
                                float _1819;
                                if (_1391)
                                {
                                    _1811 = 0.0f;
                                    _1815 = 0.0f;
                                    _1819 = 0.0f;
                                }
                                else
                                {
                                    if (!((_397 != 4u) || (_800 != 0u)))
                                    {
                                        frontier_phi_108_116_ladder = _1820;
                                        frontier_phi_108_116_ladder_1 = _1816;
                                        frontier_phi_108_116_ladder_2 = _1812;
                                        frontier_phi_108_116_ladder_3 = _1804;
                                        break;
                                    }
                                    _1811 = _1812;
                                    _1815 = _1816;
                                    _1819 = _1820;
                                }
                                frontier_phi_108_116_ladder = _1819;
                                frontier_phi_108_116_ladder_1 = _1815;
                                frontier_phi_108_116_ladder_2 = _1811;
                                frontier_phi_108_116_ladder_3 = (1.0f - exp2(log2(_792) * 3.0f)) * _1804;
                                break;
                            }
                            else
                            {
                                frontier_phi_108_116_ladder = _1820;
                                frontier_phi_108_116_ladder_1 = _1816;
                                frontier_phi_108_116_ladder_2 = _1812;
                                frontier_phi_108_116_ladder_3 = _1804;
                                break;
                            }
                        }
                        _1806 = frontier_phi_108_116_ladder_3;
                        _1808 = frontier_phi_108_116_ladder_2;
                        _1813 = frontier_phi_108_116_ladder_1;
                        _1817 = frontier_phi_108_116_ladder;
                    }
                    else
                    {
                        float frontier_phi_108_95_ladder;
                        float frontier_phi_108_95_ladder_1;
                        float frontier_phi_108_95_ladder_2;
                        float frontier_phi_108_95_ladder_3;
                        if (_1174)
                        {
                            frontier_phi_108_95_ladder = _1810;
                            frontier_phi_108_95_ladder_1 = _1810;
                            frontier_phi_108_95_ladder_2 = _1810;
                            frontier_phi_108_95_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float4 _1826 = _28[7u].SampleLevel(_59, float3(_1294, _1297, _1300), 0.0f);
                            frontier_phi_108_95_ladder = _1826.z;
                            frontier_phi_108_95_ladder_1 = _1826.y;
                            frontier_phi_108_95_ladder_2 = _1826.x;
                            frontier_phi_108_95_ladder_3 = _1442;
                        }
                        _1806 = frontier_phi_108_95_ladder_3;
                        _1808 = frontier_phi_108_95_ladder_2;
                        _1813 = frontier_phi_108_95_ladder_1;
                        _1817 = frontier_phi_108_95_ladder;
                    }
                    float _1924;
                    float _1925;
                    float _1927;
                    float _1929;
                    if (_1361 == 0u)
                    {
                        _1924 = _1806;
                        _1925 = _1808;
                        _1927 = _1813;
                        _1929 = _1817;
                    }
                    else
                    {
                        _1924 = _1440;
                        _1925 = _1808 * _1246;
                        _1927 = _1813 * _1246;
                        _1929 = _1817 * _1246;
                    }
                    float _2086;
                    float _2087;
                    float _2088;
                    float _2089;
                    if (_1443 && (_1924 < 1.0f))
                    {
                        float _2060 = _1297 * _1291;
                        float _2064 = rsqrt(dot(float3(_1294, _2060, _1300), float3(_1294, _2060, _1300)));
                        float4 _2072 = _28[4u].SampleLevel(_59, float3(_2064 * _1294, _2064 * _2060, _2064 * _1300), 0.0f);
                        float _2074 = _2072.x;
                        float _2075 = _2072.y;
                        float _2076 = _2072.z;
                        _2086 = 1.0f;
                        _2087 = ((_1925 - _2074) * _1924) + _2074;
                        _2088 = ((_1927 - _2075) * _1924) + _2075;
                        _2089 = ((_1929 - _2076) * _1924) + _2076;
                    }
                    else
                    {
                        _2086 = _1924;
                        _2087 = _1925;
                        _2088 = _1927;
                        _2089 = _1929;
                    }
                    float _1868 = _2086 * _1172;
                    _1861 = _2087 * _1868;
                    _1863 = _2088 * _1868;
                    _1865 = _2089 * _1868;
                    _1867 = _1868;
                }
                float _1872 = _50_m0[58u].z * _1369;
                float _1894 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _1896 = _1894 * ((_1872 * ((_1365 * 1000.0f) - _1861)) + _1861);
                float _1897 = _1894 * ((_1872 * ((_1366 * 1000.0f) - _1863)) + _1863);
                float _1898 = _1894 * ((_1872 * (_1367 - _1865)) + _1865);
                float _1904 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_1896, max(_1897, _1898)) + 1.0f);
                float _1908 = min(_1904 * _1896, 0.996078431606292724609375f);
                float _1910 = min(_1904 * _1897, 0.996078431606292724609375f);
                float _1911 = min(_1904 * _1898, 0.996078431606292724609375f);
                _35[uint2(_221, _224)] = float4(_1908, _1910, _1911, _1867);
                _39[uint2(_221, _224)] = float4(_1390, 0.0f, 0.0f, _1390);
                if (_228)
                {
                    uint _2046 = _221 + 1u;
                    _35[uint2(_2046, _224)] = float4(_1908, _1910, _1911, _1867);
                    _39[uint2(_2046, _224)] = float4(_1390, 0.0f, 0.0f, _1390);
                }
                if (_231)
                {
                    uint _2147 = _224 + 1u;
                    _35[uint2(_221, _2147)] = float4(_1908, _1910, _1911, _1867);
                    _39[uint2(_221, _2147)] = float4(_1390, 0.0f, 0.0f, _1390);
                }
                if (_232)
                {
                    uint _2249 = _221 + 1u;
                    uint _2250 = _224 + 1u;
                    _35[uint2(_2249, _2250)] = float4(_1908, _1910, _1911, _1867);
                    _39[uint2(_2249, _2250)] = float4(_1390, 0.0f, 0.0f, _1390);
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
                uint _442 = _221 + 1u;
                _35[uint2(_442, _224)] = 0.0f.xxxx;
                _39[uint2(_442, _224)] = 0.0f.xxxx;
            }
            if (_231)
            {
                uint _754 = _224 + 1u;
                _35[uint2(_221, _754)] = 0.0f.xxxx;
                _39[uint2(_221, _754)] = 0.0f.xxxx;
            }
            if (!_232)
            {
                break;
            }
            uint _886 = _221 + 1u;
            uint _887 = _224 + 1u;
            _35[uint2(_886, _887)] = 0.0f.xxxx;
            _39[uint2(_886, _887)] = 0.0f.xxxx;
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
