static float _2293;
static uint _3376;
static float _3377;
static float _3378;
static float _3379;
static float _3380;
static float _3381;
static float _3382;
static float _3383;
static float _3384;
static float _3385;
static uint _3386;
static uint _3387;
static float _3388;
static float _3389;
static float _3390;
static float _3391;
static float _3392;
static float _3393;
static float _3399;
static uint _3400;
static float _3401;
static uint _3403;
static uint _3404;
static float _3409;

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

static RayQuery<RAY_FLAG_NONE> _2852;

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
    uint _214;
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
        uint4 _208 = asuint(_50_m0[5u]);
        _209 = _208.x;
        _214 = (((gl_WorkGroupID.y << 6u) + gl_WorkGroupID.x) << 6u) + gl_LocalInvocationIndex;
        if (_214 < _38[1u].xxxx.x)
        {
            bool ladder_phi_8;
            float frontier_phi_8_pred;
            uint _226;
            uint _229;
            bool _233;
            bool _236;
            bool _237;
            uint _238;
            uint _239;
            float _259;
            float _260;
            float _267;
            bool _268;
            uint _270;
            uint _271;
            for (;;)
            {
                uint4 _224 = _8.Load(_214);
                uint _225 = _224.x;
                _226 = _225 & 32767u;
                uint _228 = _225 >> 15u;
                _229 = _228 & 16383u;
                _233 = (_225 & 536870912u) != 0u;
                _236 = (_225 & 1073741824u) != 0u;
                _237 = int(_225) < int(0u);
                _238 = uint(_202);
                _239 = uint(_204);
                bool _242 = ((_228 + _225) & 1u) == 0u;
                float _252 = float(int(_226));
                float _253 = float(int(_229));
                _259 = ((_252 + 0.5f) + (_242 ? _45_m0[58u].x : _45_m0[58u].z)) * (1.0f / _202);
                _260 = ((_253 + 0.5f) + (_242 ? _45_m0[58u].y : _45_m0[58u].w)) * (1.0f / _204);
                _267 = _20[21u].Load(int3(uint2(_226, _229), 0u)).x;
                _268 = _267 > 0.0f;
                _270 = uint(_252);
                _271 = uint(_253);
                float _396;
                float _398;
                float _400;
                uint _402;
                float _404;
                float _406;
                float _408;
                uint _410;
                float _412;
                if (_268)
                {
                    uint4 _275 = _24[22u].Load(int3(uint2(_270, _271), 0u));
                    uint _277 = _275.x;
                    float _290 = (float((_277 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _292 = (float(_277 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _297 = (1.0f - abs(_290)) - abs(_292);
                    float _300 = clamp((-0.0f) - _297, 0.0f, 1.0f);
                    float _301 = (-0.0f) - _300;
                    float _306 = ((_290 >= 0.0f) ? _301 : _300) + _290;
                    float _307 = ((_292 >= 0.0f) ? _301 : _300) + _292;
                    float _312 = rsqrt(dot(float3(_306, _307, _297), float3(_306, _307, _297)));
                    float _317 = float(_277 & 255u) * 0.0039215688593685626983642578125f;
                    uint _324 = _24[23u].Load(int3(uint2(_270, _271), 0u)).y + 4294967295u;
                    uint _327 = uint(int(_324) >> int(31u)) & 3u;
                    uint _332 = _327 + 103u;
                    float _341 = clamp((_317 - _45_m0[_332].x) / (_45_m0[_332].y - _45_m0[_332].x), 0.0f, 1.0f);
                    _396 = _306 * _312;
                    _398 = _307 * _312;
                    _400 = _312 * _297;
                    _402 = _327 + 1u;
                    _404 = (_341 * _341) * (3.0f - (_341 * 2.0f));
                    _406 = _317;
                    _408 = _45_m0[_332].z;
                    _410 = (int(_324) < int(0u)) ? 0u : _324;
                    _412 = 0.0f;
                }
                else
                {
                    uint4 _352 = _24[2u].Load(int3(uint2(_270, _271), 0u));
                    uint _354 = _352.x;
                    uint _355 = _352.w;
                    uint4 _361 = _24[15u].Load(int3(uint2(_270, _271), 0u));
                    uint _363 = _361.y;
                    uint _372 = ((_363 & 64u) != 0u) ? uint((_363 & 4294967167u) != 66u) : 4294967295u;
                    uint _373 = _355 & 128u;
                    uint _376 = (_373 != 0u) ? 1u : ((_354 << 7u) | _355);
                    uint4 _380 = _16.Load(_376 * 4u);
                    uint _381 = _380.x;
                    uint4 _384 = _16.Load((_376 * 4u) + 1u);
                    uint _385 = _384.x;
                    uint4 _388 = _16.Load((_376 * 4u) + 3u);
                    uint _389 = _388.x;
                    uint _392 = ((_381 & 1u) != 0u) ? 0u : 18u;
                    uint _394 = uint(min(int(uint(max(int(_372), int(0u)))), int(1u)));
                    uint _433;
                    uint _434;
                    if (_373 == 0u)
                    {
                        _433 = (((_381 & 2097152u) != 0u) && (_372 == _394)) ? (_392 | 128u) : _392;
                        _434 = _381;
                    }
                    else
                    {
                        _433 = _355;
                        _434 = _381 | ((_354 << 20u) & 134217728u);
                    }
                    uint _443 = _385 & 512u;
                    float _803;
                    float _805;
                    float _807;
                    float _809;
                    float _811;
                    float _813;
                    float _815;
                    float _817;
                    float _819;
                    float _821;
                    float _823;
                    float _825;
                    if (_443 == 0u)
                    {
                        if (!((_434 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            frontier_phi_8_pred = 0.0f;
                            break;
                        }
                        float _585 = asfloat(_17.Load((_389 * 115u) + 33u).x);
                        uint4 _593 = _24[2u].Load(int3(uint2(_270, _271), 0u));
                        uint _595 = _593.y;
                        uint _596 = _433 & 128u;
                        uint _791;
                        uint _792;
                        uint _793;
                        uint _794;
                        if (_596 == 0u)
                        {
                            _791 = uint(((_434 & 817889384u) | (_385 & 576u)) != 0u) | (((_434 >> 19u) & 1u) ^ 1u);
                            _792 = uint(((_434 & 17825808u) | (_385 & 520u)) != 0u);
                            _793 = uint(((_434 & 46137344u) | (_385 & 2564u)) != 0u);
                            _794 = 0u;
                        }
                        else
                        {
                            _791 = 1u;
                            _792 = _433 & 1u;
                            _793 = 1u;
                            _794 = 1u;
                        }
                        precise float _798 = float(_595 & 127u) * 0.0078740157186985015869140625f;
                        bool _802 = (_434 & 4194304u) == 0u;
                        float _1031;
                        if (_802)
                        {
                            _1031 = _798;
                        }
                        else
                        {
                            _1031 = float(_595 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1277;
                        if ((_434 & 134217728u) == 0u)
                        {
                            uint frontier_phi_48_40_ladder;
                            if ((_596 != 0u) || ((_434 & 17825792u) == 1048576u))
                            {
                                frontier_phi_48_40_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_48_40_ladder = _792;
                            }
                            _1277 = frontier_phi_48_40_ladder;
                        }
                        else
                        {
                            _1277 = _792;
                        }
                        uint4 _1280 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _1282 = _1280.x;
                        float _1395;
                        float _1397;
                        float _1399;
                        if (_791 == 0u)
                        {
                            _1395 = 0.0f;
                            _1397 = 0.0f;
                            _1399 = 0.0f;
                        }
                        else
                        {
                            float4 _1404 = _20[8u].Load(int3(uint2(_270, _271), 0u));
                            _1395 = _1404.x;
                            _1397 = _1404.y;
                            _1399 = _1404.z;
                        }
                        uint _1497;
                        if (_1277 == 0u)
                        {
                            _1497 = 0u;
                        }
                        else
                        {
                            _1497 = _24[9u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        uint _1694;
                        if (_793 == 0u)
                        {
                            _1694 = 0u;
                        }
                        else
                        {
                            _1694 = _24[10u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        float _1704 = (float((_1282 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1705 = (float(_1282 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1709 = (1.0f - abs(_1704)) - abs(_1705);
                        float _1711 = clamp((-0.0f) - _1709, 0.0f, 1.0f);
                        float _1712 = (-0.0f) - _1711;
                        float _1717 = ((_1704 >= 0.0f) ? _1712 : _1711) + _1704;
                        float _1718 = ((_1705 >= 0.0f) ? _1712 : _1711) + _1705;
                        float _1722 = rsqrt(dot(float3(_1717, _1718, _1709), float3(_1717, _1718, _1709)));
                        float _1723 = _1717 * _1722;
                        float _1724 = _1718 * _1722;
                        float _1725 = _1722 * _1709;
                        float _804 = float(_1282 & 255u);
                        float _1729 = ((_434 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _2018;
                        float _2019;
                        float _2020;
                        float _2021;
                        uint _2022;
                        if ((_385 & 64u) == 0u)
                        {
                            uint frontier_phi_102_88_ladder;
                            float frontier_phi_102_88_ladder_1;
                            float frontier_phi_102_88_ladder_2;
                            float frontier_phi_102_88_ladder_3;
                            float frontier_phi_102_88_ladder_4;
                            if ((_434 & 276824064u) == 0u)
                            {
                                frontier_phi_102_88_ladder = 0u;
                                frontier_phi_102_88_ladder_1 = 0.0f;
                                frontier_phi_102_88_ladder_2 = 0.0f;
                                frontier_phi_102_88_ladder_3 = 0.0f;
                                frontier_phi_102_88_ladder_4 = ((_434 & 8u) != 0u) ? _1397 : _1729;
                            }
                            else
                            {
                                frontier_phi_102_88_ladder = 0u;
                                frontier_phi_102_88_ladder_1 = 0.0f;
                                frontier_phi_102_88_ladder_2 = 0.0f;
                                frontier_phi_102_88_ladder_3 = 0.0f;
                                frontier_phi_102_88_ladder_4 = _1729;
                            }
                            _2018 = frontier_phi_102_88_ladder_4;
                            _2019 = frontier_phi_102_88_ladder_3;
                            _2020 = frontier_phi_102_88_ladder_2;
                            _2021 = frontier_phi_102_88_ladder_1;
                            _2022 = frontier_phi_102_88_ladder;
                        }
                        else
                        {
                            float _1828 = (_1397 * 2.0f) + (-1.0f);
                            float _1829 = (_1399 * 2.0f) + (-1.0f);
                            float _1833 = (1.0f - abs(_1828)) - abs(_1829);
                            float _1835 = clamp((-0.0f) - _1833, 0.0f, 1.0f);
                            float _1836 = (-0.0f) - _1835;
                            float _1841 = ((_1828 >= 0.0f) ? _1836 : _1835) + _1828;
                            float _1842 = ((_1829 >= 0.0f) ? _1836 : _1835) + _1829;
                            float _1846 = rsqrt(dot(float3(_1841, _1842, _1833), float3(_1841, _1842, _1833)));
                            _2018 = floor(round(_1395 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _2019 = _1841 * _1846;
                            _2020 = _1842 * _1846;
                            _2021 = _1846 * _1833;
                            _2022 = 1u;
                        }
                        float _812;
                        if ((_434 & 32768u) == 0u)
                        {
                            _812 = _2018;
                        }
                        else
                        {
                            float frontier_phi_115_116_ladder;
                            if (_17.Load((_389 * 115u) + 36u).x == 0u)
                            {
                                float _2370 = clamp((_804 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _585;
                                frontier_phi_115_116_ladder = ((_434 & 131072u) != 0u) ? _2370 : ((((clamp((1.21000003814697265625f / (exp2((_1031 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_389 * 115u) + 32u).x)) + 1.0f) * _2370);
                            }
                            else
                            {
                                frontier_phi_115_116_ladder = _585;
                            }
                            _812 = frontier_phi_115_116_ladder;
                        }
                        uint _2159 = _433 & 1u;
                        float _2313;
                        float _2315;
                        float _2317;
                        uint _2319;
                        if (((_434 & 16u) == 0u) || (((_2159 | (_385 & 8u)) | (_434 & 16777216u)) != 0u))
                        {
                            _2313 = _2019;
                            _2315 = _2020;
                            _2317 = _2021;
                            _2319 = _2022;
                        }
                        else
                        {
                            float _2329 = (float(_1497 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2330 = (float(_1497 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2334 = (1.0f - abs(_2329)) - abs(_2330);
                            float _2336 = clamp((-0.0f) - _2334, 0.0f, 1.0f);
                            float _2337 = (-0.0f) - _2336;
                            float _2342 = ((_2329 >= 0.0f) ? _2337 : _2336) + _2329;
                            float _2343 = ((_2330 >= 0.0f) ? _2337 : _2336) + _2330;
                            float _2347 = rsqrt(dot(float3(_2342, _2343, _2334), float3(_2342, _2343, _2334)));
                            _2313 = _2342 * _2347;
                            _2315 = _2343 * _2347;
                            _2317 = _2347 * _2334;
                            _2319 = 1u;
                        }
                        float _806;
                        float _808;
                        float _810;
                        if (_2159 == 0u)
                        {
                            float frontier_phi_146_145_ladder;
                            float frontier_phi_146_145_ladder_1;
                            float frontier_phi_146_145_ladder_2;
                            if (((_433 & 64u) == 0u) && (_794 != 0u))
                            {
                                float2 _2560 = spvUnpackHalf2x16((_1694 >> 17u) & 32736u);
                                float _2561 = _2560.x;
                                float _2564 = (spvUnpackHalf2x16((_1694 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2565 = (spvUnpackHalf2x16((_1694 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2569 = (1.0f - abs(_2564)) - abs(_2565);
                                float _2571 = clamp((-0.0f) - _2569, 0.0f, 1.0f);
                                float _2572 = (-0.0f) - _2571;
                                float _2577 = ((_2564 >= 0.0f) ? _2572 : _2571) + _2564;
                                float _2578 = ((_2565 >= 0.0f) ? _2572 : _2571) + _2565;
                                float _2582 = rsqrt(dot(float3(_2577, _2578, _2569), float3(_2577, _2578, _2569)));
                                float _2592 = (((_2577 * _2582) - _1723) * _2561) + _1723;
                                float _2593 = (((_2578 * _2582) - _1724) * _2561) + _1724;
                                float _2594 = (((_2582 * _2569) - _1725) * _2561) + _1725;
                                float _2598 = rsqrt(dot(float3(_2592, _2593, _2594), float3(_2592, _2593, _2594)));
                                frontier_phi_146_145_ladder = _2594 * _2598;
                                frontier_phi_146_145_ladder_1 = _2593 * _2598;
                                frontier_phi_146_145_ladder_2 = _2592 * _2598;
                            }
                            else
                            {
                                frontier_phi_146_145_ladder = _1725;
                                frontier_phi_146_145_ladder_1 = _1724;
                                frontier_phi_146_145_ladder_2 = _1723;
                            }
                            _806 = frontier_phi_146_145_ladder_2;
                            _808 = frontier_phi_146_145_ladder_1;
                            _810 = frontier_phi_146_145_ladder;
                        }
                        else
                        {
                            _806 = _1723;
                            _808 = _1724;
                            _810 = _1725;
                        }
                        float _820;
                        float _822;
                        float _824;
                        float _826;
                        if (_802)
                        {
                            float frontier_phi_160_159_ladder;
                            float frontier_phi_160_159_ladder_1;
                            float frontier_phi_160_159_ladder_2;
                            float frontier_phi_160_159_ladder_3;
                            if (((_434 & 33554432u) == 0u) || (((_385 & 4u) != 0u) && ((_434 & 8388608u) == 0u)))
                            {
                                frontier_phi_160_159_ladder = 0.0f;
                                frontier_phi_160_159_ladder_1 = 0.0f;
                                frontier_phi_160_159_ladder_2 = 0.0f;
                                frontier_phi_160_159_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2722 = (spvUnpackHalf2x16((_1694 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2723 = (spvUnpackHalf2x16((_1694 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2727 = (1.0f - abs(_2722)) - abs(_2723);
                                float _2729 = clamp((-0.0f) - _2727, 0.0f, 1.0f);
                                float _2730 = (-0.0f) - _2729;
                                float _2735 = ((_2722 >= 0.0f) ? _2730 : _2729) + _2722;
                                float _2736 = ((_2723 >= 0.0f) ? _2730 : _2729) + _2723;
                                float _2740 = rsqrt(dot(float3(_2735, _2736, _2727), float3(_2735, _2736, _2727)));
                                float _2741 = _2735 * _2740;
                                float _2742 = _2736 * _2740;
                                float _2743 = _2740 * _2727;
                                float _2747 = rsqrt(dot(float3(_2741, _2742, _2743), float3(_2741, _2742, _2743)));
                                frontier_phi_160_159_ladder = _2747 * _2743;
                                frontier_phi_160_159_ladder_1 = _2747 * _2742;
                                frontier_phi_160_159_ladder_2 = _2747 * _2741;
                                frontier_phi_160_159_ladder_3 = spvUnpackHalf2x16((_1694 >> 17u) & 32736u).x;
                            }
                            _820 = frontier_phi_160_159_ladder_3;
                            _822 = frontier_phi_160_159_ladder_2;
                            _824 = frontier_phi_160_159_ladder_1;
                            _826 = frontier_phi_160_159_ladder;
                        }
                        else
                        {
                            _820 = 0.0f;
                            _822 = 0.0f;
                            _824 = 0.0f;
                            _826 = 0.0f;
                        }
                        bool _2612 = _2319 != 0u;
                        _803 = _804;
                        _805 = _806;
                        _807 = _808;
                        _809 = _810;
                        _811 = _812;
                        _813 = _2612 ? _2313 : _806;
                        _815 = _2612 ? _2315 : _808;
                        _817 = _2612 ? _2317 : _810;
                        _819 = _820;
                        _821 = _822;
                        _823 = _824;
                        _825 = _826;
                    }
                    else
                    {
                        uint4 _545 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _547 = _545.x;
                        uint4 _551 = _24[9u].Load(int3(uint2(_270, _271), 0u));
                        uint _553 = _551.x;
                        float _727;
                        float _728;
                        float _729;
                        if ((_434 & 33554432u) == 0u)
                        {
                            float _605 = (float((_547 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _606 = (float(_547 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _610 = (1.0f - abs(_605)) - abs(_606);
                            float _612 = clamp((-0.0f) - _610, 0.0f, 1.0f);
                            float _613 = (-0.0f) - _612;
                            float _618 = ((_605 >= 0.0f) ? _613 : _612) + _605;
                            float _619 = ((_606 >= 0.0f) ? _613 : _612) + _606;
                            float _623 = rsqrt(dot(float3(_618, _619, _610), float3(_618, _619, _610)));
                            _727 = _618 * _623;
                            _728 = _619 * _623;
                            _729 = _623 * _610;
                        }
                        else
                        {
                            float _634 = (float((_553 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _635 = (float(_553 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _639 = (1.0f - abs(_634)) - abs(_635);
                            float _641 = clamp((-0.0f) - _639, 0.0f, 1.0f);
                            float _642 = (-0.0f) - _641;
                            float _647 = ((_634 >= 0.0f) ? _642 : _641) + _634;
                            float _648 = ((_635 >= 0.0f) ? _642 : _641) + _635;
                            float _652 = rsqrt(dot(float3(_647, _648, _639), float3(_647, _648, _639)));
                            _727 = _647 * _652;
                            _728 = _648 * _652;
                            _729 = _652 * _639;
                        }
                        _803 = float(_547 & 255u);
                        _805 = _727;
                        _807 = _728;
                        _809 = _729;
                        _811 = 1.0f;
                        _813 = _727;
                        _815 = _728;
                        _817 = _729;
                        _819 = 0.0f;
                        _821 = 0.0f;
                        _823 = 0.0f;
                        _825 = 0.0f;
                    }
                    precise float _827 = _803 * 0.0039215688593685626983642578125f;
                    if ((_433 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        frontier_phi_8_pred = 0.0f;
                        break;
                    }
                    bool _1040 = ((_433 & 128u) | _443) != 0u;
                    uint _1212;
                    if (_1040)
                    {
                        _1212 = 1u;
                    }
                    else
                    {
                        _1212 = (((_434 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _397;
                    float _399;
                    float _401;
                    uint _403;
                    float _407;
                    if (((_434 & 33554432u) == 0u) || _1040)
                    {
                        bool _1284 = _77 != 0u;
                        uint _1291;
                        if ((_434 & 16u) == 0u)
                        {
                            _1291 = _1212;
                        }
                        else
                        {
                            _1291 = ((_434 & 268435456u) != 0u) ? _1212 : 2u;
                        }
                        _407 = _811 * _827;
                        _397 = _1284 ? _813 : _805;
                        _399 = _1284 ? _815 : _807;
                        _401 = _1284 ? _817 : _809;
                        _403 = _1291;
                    }
                    else
                    {
                        _407 = _819;
                        _397 = _821;
                        _399 = _823;
                        _401 = _825;
                        _403 = _1212;
                    }
                    uint _1292 = _403 + 102u;
                    float _1301 = clamp((_407 - _45_m0[_1292].x) / (_45_m0[_1292].y - _45_m0[_1292].x), 0.0f, 1.0f);
                    _396 = _397;
                    _398 = _399;
                    _400 = _401;
                    _402 = _403;
                    _404 = (_1301 * _1301) * (3.0f - (_1301 * 2.0f));
                    _406 = _407;
                    _408 = _45_m0[_1292].z;
                    _410 = (_403 == 1u) ? _394 : 0u;
                    _412 = asfloat(_17.Load((_389 * 115u) + 114u).x);
                }
                if (_404 == 0.0f)
                {
                    ladder_phi_8 = false;
                    frontier_phi_8_pred = _406;
                    break;
                }
                float _453;
                if (_268)
                {
                    _453 = _267;
                }
                else
                {
                    _453 = _12.Load(int3(uint2(uint(int(_259 * float(_238))), uint(int(_260 * float(_239)))), 0u)).x;
                }
                float _455 = 1.0f - _406;
                float _456 = _455 * _455;
                float _464 = _50_m0[50u].w + _50_m0[50u].y;
                uint _467 = _402 + 63u;
                float _476 = clamp(((_50_m0[50u].x / (_464 - (_50_m0[50u].y * _453))) - _50_m0[_467].y) / (_50_m0[_467].x - _50_m0[_467].y), 0.0f, 1.0f);
                float _481 = ((_476 * _476) * _404) * (3.0f - (_476 * 2.0f));
                float _492 = ((_259 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _493 = ((1.0f - (_50_m0[51u].w * _260)) * 2.0f) + (-1.0f);
                float _509 = mad(_144, _453, mad(_137, _493, _492 * _130)) + _151;
                float _510 = (mad(_141, _453, mad(_134, _493, _492 * _127)) + _148) / _509;
                float _511 = (mad(_142, _453, mad(_135, _493, _492 * _128)) + _149) / _509;
                float _512 = (mad(_143, _453, mad(_136, _493, _492 * _129)) + _150) / _509;
                float _516 = rsqrt(dot(float3(_510, _511, _512), float3(_510, _511, _512)));
                float _517 = _516 * _510;
                float _518 = _516 * _511;
                float _519 = _516 * _512;
                float _522 = mad(_93, _400, mad(_87, _398, _396 * _81));
                float _525 = mad(_94, _400, mad(_88, _398, _396 * _82));
                float _528 = mad(_95, _400, mad(_89, _398, _396 * _83));
                float _531 = _525 * _525;
                float _664;
                float _665;
                float _666;
                if (abs(_528) > 0.0f)
                {
                    float _566 = sqrt((_528 * _528) + _531);
                    _664 = 0.0f;
                    _665 = ((-0.0f) - _528) / _566;
                    _666 = _525 / _566;
                }
                else
                {
                    float _572 = sqrt(_531 + (_522 * _522));
                    _664 = _525 / _572;
                    _665 = ((-0.0f) - _522) / _572;
                    _666 = 0.0f;
                }
                float _669 = (_666 * _525) - (_665 * _528);
                float _672 = (_664 * _528) - (_666 * _522);
                float _675 = (_665 * _522) - (_664 * _525);
                float _676 = (-0.0f) - _517;
                float _677 = (-0.0f) - _518;
                float _678 = (-0.0f) - _519;
                float _687 = mad(_678, _528, mad(_677, _525, _522 * _676));
                float _688 = mad(_678, _666, mad(_677, _665, _664 * _676)) * _456;
                float _689 = mad(_678, _675, mad(_677, _672, _669 * _676)) * _456;
                float _693 = rsqrt(dot(float3(_688, _689, _687), float3(_688, _689, _687)));
                float _694 = _693 * _688;
                float _695 = _693 * _689;
                float _696 = _693 * _687;
                float _699 = (_694 * _694) + (_695 * _695);
                bool _700 = _699 > 0.0f;
                float _736;
                float _737;
                if (_700)
                {
                    float _732 = rsqrt(_699);
                    _736 = (-0.0f) - (_695 * _732);
                    _737 = _732 * _694;
                }
                else
                {
                    _736 = 1.0f;
                    _737 = 0.0f;
                }
                float _742 = (_696 + 1.0f) * 0.5f;
                float _743 = 1.0f - _742;
                float _744 = _743 * _696;
                float _751 = sqrt(max(0.0f, 1.0f - (_743 * _743)));
                float _758 = ((_751 * _694) - (_737 * _744)) * _456;
                float _759 = ((_751 * _695) + (_736 * _744)) * _456;
                float _760 = max(0.0f, (((_737 * _694) - (_736 * _695)) * _743) + (_751 * _696));
                float _764 = rsqrt(dot(float3(_758, _759, _760), float3(_758, _759, _760)));
                float _765 = _758 * _764;
                float _766 = _759 * _764;
                float _767 = _764 * _760;
                float _770 = mad(_767, _522, mad(_766, _669, _765 * _664));
                float _773 = mad(_767, _525, mad(_766, _672, _765 * _665));
                float _776 = mad(_767, _528, mad(_766, _675, _765 * _666));
                float _780 = dot(float3(_517, _518, _519), float3(_770, _773, _776)) * 2.0f;
                float _784 = _517 - (_780 * _770);
                float _785 = _518 - (_780 * _773);
                float _786 = _519 - (_780 * _776);
                float _835;
                float _836;
                if (_700)
                {
                    float _831 = rsqrt(_699);
                    _835 = (-0.0f) - (_695 * _831);
                    _836 = _831 * _694;
                }
                else
                {
                    _835 = 1.0f;
                    _836 = 0.0f;
                }
                float _840 = sqrt(_50_m0[1u].x);
                float _841 = _50_m0[1u].y * 1.57079637050628662109375f;
                float _844 = cos(_841) * _840;
                float _847 = 1.0f - (_844 * _844);
                float _852 = (sqrt(_847) * _743) + ((_840 * _742) * sin(_841));
                float _855 = _852 * _696;
                float _862 = sqrt(max(0.0f, _847 - (_852 * _852)));
                float _871 = (((_862 * _694) + (_844 * _835)) - (_855 * _836)) * _456;
                float _872 = (((_862 * _695) + (_844 * _836)) + (_855 * _835)) * _456;
                float _873 = max(0.0f, (_852 * ((_836 * _694) - (_835 * _695))) + (_862 * _696));
                float _877 = rsqrt(dot(float3(_871, _872, _873), float3(_871, _872, _873)));
                float _878 = _871 * _877;
                float _879 = _872 * _877;
                float _880 = _877 * _873;
                float _883 = mad(_880, _522, mad(_879, _669, _878 * _664));
                float _886 = mad(_880, _525, mad(_879, _672, _878 * _665));
                float _889 = mad(_880, _528, mad(_879, _675, _878 * _666));
                float _893 = dot(float3(_517, _518, _519), float3(_883, _886, _889)) * 2.0f;
                float _897 = _517 - (_893 * _883);
                float _898 = _518 - (_893 * _886);
                float _899 = _519 - (_893 * _889);
                float _900 = dot(float3(_897, _898, _899), float3(_784, _785, _786));
                float _913 = (_50_m0[1u].z * (_522 - _770)) + _770;
                float _914 = (_50_m0[1u].z * (_525 - _773)) + _773;
                float _915 = (_50_m0[1u].z * (_528 - _776)) + _776;
                float _919 = rsqrt(dot(float3(_913, _914, _915), float3(_913, _914, _915)));
                float _920 = _919 * _913;
                float _921 = _919 * _914;
                float _922 = _919 * _915;
                float _926 = dot(float3(_517, _518, _519), float3(_920, _921, _922)) * 2.0f;
                float _930 = _517 - (_926 * _920);
                float _931 = _518 - (_926 * _921);
                float _932 = _519 - (_926 * _922);
                bool _936 = dot(float3(_930, _931, _932), float3(_517, _518, _519)) < 0.0f;
                float _943 = sqrt(((_511 * _511) + (_510 * _510)) + (_512 * _512)) * 0.001000000047497451305389404296875f;
                float _954 = ((_943 * _522) + _510) + (_930 * _408);
                float _955 = ((_943 * _525) + _511) + (_931 * _408);
                float _956 = ((_943 * _528) + _512) + (_932 * _408);
                float _972 = mad(_116, _956, mad(_109, _955, _954 * _102)) + _123;
                float _975 = (mad(_115, _956, mad(_108, _955, _954 * _101)) + _122) / _972;
                float _978 = (((mad(_113, _956, mad(_106, _955, _954 * _99)) + _120) / _972) * 0.5f) + 0.5f;
                float _979 = 0.5f - (((mad(_114, _956, mad(_107, _955, _954 * _100)) + _121) / _972) * 0.5f);
                float _982 = _978 * _50_m0[51u].x;
                float _983 = _979 * _50_m0[51u].y;
                float _988 = _954 + (_930 * 0.100000001490116119384765625f);
                float _989 = _955 + (_931 * 0.100000001490116119384765625f);
                float _990 = _956 + (_932 * 0.100000001490116119384765625f);
                float _1006 = mad(_116, _990, mad(_109, _989, _988 * _102)) + _123;
                float _1015 = _50_m0[51u].x * (((((mad(_113, _990, mad(_106, _989, _988 * _99)) + _120) / _1006) * 0.5f) + 0.5f) - _978);
                float _1017 = _50_m0[51u].y * ((0.5f - (((mad(_114, _990, mad(_107, _989, _988 * _100)) + _121) / _1006) * 0.5f)) - _979);
                float _1018 = ((mad(_115, _990, mad(_108, _989, _988 * _101)) + _122) / _1006) - _975;
                float _1019 = _1015 * 10.0f;
                float _1021 = _1017 * 10.0f;
                float _1022 = _1018 * 10.0f;
                bool _1029 = _402 == 1u;
                uint _1041;
                uint _1043;
                float _1045;
                float _1047;
                float _1049;
                float _1051;
                float _1053;
                float _1055;
                float _1057;
                float _1059;
                float _1061;
                uint _1063;
                uint _1065;
                float _1067;
                float _1069;
                float _1071;
                if (_1029 && (asuint(_50_m0[62u]).w != 0u))
                {
                    _1041 = 0u;
                    _1043 = 1u;
                    _1045 = 0.0f;
                    _1047 = 0.0f;
                    _1049 = 1.0f;
                    _1051 = 0.0f;
                    _1053 = 0.0f;
                    _1055 = 0.0f;
                    _1057 = 0.0f;
                    _1059 = 0.0f;
                    _1061 = 0.0f;
                    _1063 = 0u;
                    _1065 = 0u;
                    _1067 = 0.0f;
                    _1069 = 0.0f;
                    _1071 = 0.0f;
                }
                else
                {
                    float _1132 = float(_238);
                    float _1133 = float(_239);
                    float _1140 = (_1019 != 0.0f) ? (0.100000001490116119384765625f / _1015) : 3.4028234663852885981170418348452e+38f;
                    float _1142 = (_1021 != 0.0f) ? (0.100000001490116119384765625f / _1017) : 3.4028234663852885981170418348452e+38f;
                    float _1143 = (_1022 != 0.0f) ? (0.100000001490116119384765625f / _1018) : 3.4028234663852885981170418348452e+38f;
                    float _1144 = 1.0f / _1132;
                    float _1145 = 1.0f / _1133;
                    float _1146 = 0.004999999888241291046142578125f / _1132;
                    float _1148 = 0.004999999888241291046142578125f / _1133;
                    float _1157 = float(_1019 >= 0.0f);
                    float _1158 = float(_1021 >= 0.0f);
                    float _1167 = ((_1019 < 0.0f) ? ((-0.0f) - _1146) : _1146) - _982;
                    float _1170 = ((_1021 < 0.0f) ? ((-0.0f) - _1148) : _1148) - _983;
                    float _1173 = min((((floor(_982 * _1132) + _1157) * _1144) + _1167) * _1140, (((floor(_983 * _1133) + _1158) * _1145) + _1170) * _1142);
                    float _1177 = (_1173 * _1019) + _982;
                    float _1178 = (_1173 * _1021) + _983;
                    float _1179 = (_1173 * _1022) + _975;
                    float _1182 = _50_m0[50u].x / (_464 - (_1179 * _50_m0[50u].y));
                    float _1192 = 1.0f / max(0.00999999977648258209228515625f, max(abs(_1015 * 38400.0f), abs(_1017 * 21600.0f)));
                    float _1200 = max(_50_m0[3u].z, _50_m0[5u].z * _1192);
                    float _1272;
                    if (asuint(_50_m0[5u]).y == 0u)
                    {
                        _1272 = _1200;
                    }
                    else
                    {
                        _1272 = min(_1200, _50_m0[5u].w * _1192);
                    }
                    uint _1044;
                    float _1046;
                    float _1048;
                    float _1050;
                    float _1052;
                    float _1054;
                    float _1056;
                    float _1058;
                    float _1060;
                    float _1062;
                    float _1068;
                    float _1070;
                    float _1072;
                    uint _1376;
                    uint _1381;
                    if (_209 == 0u)
                    {
                        _1376 = 0u;
                        _1072 = _1179;
                        _1070 = _1178;
                        _1068 = _1177;
                        _1381 = 0u;
                        _1062 = _975;
                        _1060 = _983;
                        _1058 = _982;
                        _1056 = _1179;
                        _1054 = _1178;
                        _1052 = _1177;
                        _1050 = 1.0f;
                        _1048 = 0.0f;
                        _1046 = 0.0f;
                        _1044 = 1u;
                    }
                    else
                    {
                        uint _1377;
                        float _1378;
                        float _1379;
                        float _1380;
                        uint _1382;
                        uint _1480;
                        float _1383;
                        float _1384;
                        float _1385;
                        float _1386;
                        float _1387;
                        float _1388;
                        float _1389;
                        float _1390;
                        float _1391;
                        uint _1392;
                        float _1465;
                        float _1470;
                        float _1472;
                        float _1474;
                        float _1476;
                        float _1478;
                        uint _1463 = 0u;
                        float _1464 = _1272;
                        float _1466 = _1179;
                        float _1467 = _1178;
                        float _1468 = _1177;
                        float _1469 = _1173;
                        float _1471 = _1145;
                        float _1473 = _1144;
                        float _1475 = _1133;
                        float _1477 = _1132;
                        uint _1479 = 0u;
                        uint _1481 = 0u;
                        float _1482 = _975;
                        float _1483 = _983;
                        float _1484 = _982;
                        float _1485 = _1179;
                        float _1486 = _1178;
                        float _1487 = _1177;
                        float _1488 = 1.0f;
                        float _1489 = 0.0f;
                        float _1490 = 0.0f;
                        uint _1491 = 1u;
                        float _1492;
                        float _1493;
                        uint _1494;
                        uint _1495;
                        bool _1496;
                        for (;;)
                        {
                            _1492 = _1477 * _1468;
                            _1493 = _1475 * _1467;
                            _1494 = uint(int(_1492));
                            _1495 = uint(int(_1493));
                            _1496 = _1479 == 0u;
                            float _1771;
                            if (_1496)
                            {
                                _1771 = _12.Load(int3(uint2(_1494, _1495), 0u)).x;
                            }
                            else
                            {
                                _1771 = _15.Load(int3(uint2(_1494, _1495), _1479 + 4294967295u)).x;
                            }
                            float _1777 = ((_1492 >= floor(_1477)) || (_1493 >= floor(_1475))) ? 1.0f : _1771;
                            float _1791 = (_1022 < 0.0f) ? ((_1777 - _975) * _1143) : 3.4028234663852885981170418348452e+38f;
                            float _1793 = min(min((((floor(_1492) + _1157) * _1473) + _1167) * _1140, (((floor(_1493) + _1158) * _1471) + _1170) * _1142), _1791);
                            bool _1794 = _1777 < _1466;
                            bool _1798 = _1794 && (asuint(_1793) != asuint(_1791));
                            float _1799 = _1794 ? _1793 : _1469;
                            float _1803 = (_1799 * _1019) + _982;
                            float _1804 = (_1799 * _1021) + _983;
                            float _1805 = (_1799 * _1022) + _975;
                            uint _1807 = (_1798 ? 1u : 4294967295u) + _1479;
                            float _1808 = _1798 ? 0.5f : 2.0f;
                            float _1809 = _1808 * _1477;
                            float _1810 = _1808 * _1475;
                            float _1811 = _1798 ? 2.0f : 0.5f;
                            float _1812 = _1811 * _1473;
                            float _1813 = _1811 * _1471;
                            _1377 = _1463 + 1u;
                            uint _2149;
                            uint _2151;
                            if (int(_1807) < int(0u))
                            {
                                float _1990 = _50_m0[50u].w + _50_m0[50u].y;
                                float _1992 = _50_m0[50u].x / (_1990 - (_50_m0[50u].y * _1777));
                                float _1995 = _50_m0[50u].x / (_1990 - (_50_m0[50u].y * _1805));
                                float _2000 = abs(_1182 - _1995);
                                float _2003 = _1995 - _1992;
                                float frontier_phi_114_99_ladder;
                                float frontier_phi_114_99_ladder_1;
                                float frontier_phi_114_99_ladder_2;
                                float frontier_phi_114_99_ladder_3;
                                float frontier_phi_114_99_ladder_4;
                                float frontier_phi_114_99_ladder_5;
                                float frontier_phi_114_99_ladder_6;
                                uint frontier_phi_114_99_ladder_7;
                                uint frontier_phi_114_99_ladder_8;
                                float frontier_phi_114_99_ladder_9;
                                float frontier_phi_114_99_ladder_10;
                                float frontier_phi_114_99_ladder_11;
                                float frontier_phi_114_99_ladder_12;
                                float frontier_phi_114_99_ladder_13;
                                float frontier_phi_114_99_ladder_14;
                                float frontier_phi_114_99_ladder_15;
                                float frontier_phi_114_99_ladder_16;
                                uint frontier_phi_114_99_ladder_17;
                                if (_2003 > max(_50_m0[3u].x, _50_m0[3u].x * _2000))
                                {
                                    float _2120 = _1799 + _1464;
                                    bool _2121 = _1481 != 0u;
                                    float _2122 = _2121 ? _1490 : _1803;
                                    float _2123 = _2121 ? _1489 : _1804;
                                    uint _2130 = (_1029 || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1481;
                                    float frontier_phi_114_99_ladder_112_ladder;
                                    float frontier_phi_114_99_ladder_112_ladder_1;
                                    float frontier_phi_114_99_ladder_112_ladder_2;
                                    float frontier_phi_114_99_ladder_112_ladder_3;
                                    float frontier_phi_114_99_ladder_112_ladder_4;
                                    float frontier_phi_114_99_ladder_112_ladder_5;
                                    float frontier_phi_114_99_ladder_112_ladder_6;
                                    uint frontier_phi_114_99_ladder_112_ladder_7;
                                    uint frontier_phi_114_99_ladder_112_ladder_8;
                                    float frontier_phi_114_99_ladder_112_ladder_9;
                                    float frontier_phi_114_99_ladder_112_ladder_10;
                                    float frontier_phi_114_99_ladder_112_ladder_11;
                                    float frontier_phi_114_99_ladder_112_ladder_12;
                                    float frontier_phi_114_99_ladder_112_ladder_13;
                                    float frontier_phi_114_99_ladder_112_ladder_14;
                                    float frontier_phi_114_99_ladder_112_ladder_15;
                                    float frontier_phi_114_99_ladder_112_ladder_16;
                                    uint frontier_phi_114_99_ladder_112_ladder_17;
                                    if (asuint(_50_m0[5u]).y == 0u)
                                    {
                                        frontier_phi_114_99_ladder_112_ladder = _1485;
                                        frontier_phi_114_99_ladder_112_ladder_1 = _1464;
                                        frontier_phi_114_99_ladder_112_ladder_2 = _2120;
                                        frontier_phi_114_99_ladder_112_ladder_3 = _1145;
                                        frontier_phi_114_99_ladder_112_ladder_4 = _1144;
                                        frontier_phi_114_99_ladder_112_ladder_5 = _1133;
                                        frontier_phi_114_99_ladder_112_ladder_6 = _1132;
                                        frontier_phi_114_99_ladder_112_ladder_7 = 0u;
                                        frontier_phi_114_99_ladder_112_ladder_8 = _2130;
                                        frontier_phi_114_99_ladder_112_ladder_9 = _1482;
                                        frontier_phi_114_99_ladder_112_ladder_10 = _1483;
                                        frontier_phi_114_99_ladder_112_ladder_11 = _1484;
                                        frontier_phi_114_99_ladder_112_ladder_12 = _1486;
                                        frontier_phi_114_99_ladder_112_ladder_13 = _1487;
                                        frontier_phi_114_99_ladder_112_ladder_14 = _1488;
                                        frontier_phi_114_99_ladder_112_ladder_15 = _2123;
                                        frontier_phi_114_99_ladder_112_ladder_16 = _2122;
                                        frontier_phi_114_99_ladder_112_ladder_17 = _1491;
                                    }
                                    else
                                    {
                                        frontier_phi_114_99_ladder_112_ladder = _1485;
                                        frontier_phi_114_99_ladder_112_ladder_1 = min(_1200, _50_m0[6u].x * _1464);
                                        frontier_phi_114_99_ladder_112_ladder_2 = _2120;
                                        frontier_phi_114_99_ladder_112_ladder_3 = _1145;
                                        frontier_phi_114_99_ladder_112_ladder_4 = _1144;
                                        frontier_phi_114_99_ladder_112_ladder_5 = _1133;
                                        frontier_phi_114_99_ladder_112_ladder_6 = _1132;
                                        frontier_phi_114_99_ladder_112_ladder_7 = 0u;
                                        frontier_phi_114_99_ladder_112_ladder_8 = _2130;
                                        frontier_phi_114_99_ladder_112_ladder_9 = _1482;
                                        frontier_phi_114_99_ladder_112_ladder_10 = _1483;
                                        frontier_phi_114_99_ladder_112_ladder_11 = _1484;
                                        frontier_phi_114_99_ladder_112_ladder_12 = _1486;
                                        frontier_phi_114_99_ladder_112_ladder_13 = _1487;
                                        frontier_phi_114_99_ladder_112_ladder_14 = _1488;
                                        frontier_phi_114_99_ladder_112_ladder_15 = _2123;
                                        frontier_phi_114_99_ladder_112_ladder_16 = _2122;
                                        frontier_phi_114_99_ladder_112_ladder_17 = _1491;
                                    }
                                    frontier_phi_114_99_ladder = frontier_phi_114_99_ladder_112_ladder;
                                    frontier_phi_114_99_ladder_1 = frontier_phi_114_99_ladder_112_ladder_1;
                                    frontier_phi_114_99_ladder_2 = frontier_phi_114_99_ladder_112_ladder_2;
                                    frontier_phi_114_99_ladder_3 = frontier_phi_114_99_ladder_112_ladder_3;
                                    frontier_phi_114_99_ladder_4 = frontier_phi_114_99_ladder_112_ladder_4;
                                    frontier_phi_114_99_ladder_5 = frontier_phi_114_99_ladder_112_ladder_5;
                                    frontier_phi_114_99_ladder_6 = frontier_phi_114_99_ladder_112_ladder_6;
                                    frontier_phi_114_99_ladder_7 = frontier_phi_114_99_ladder_112_ladder_7;
                                    frontier_phi_114_99_ladder_8 = frontier_phi_114_99_ladder_112_ladder_8;
                                    frontier_phi_114_99_ladder_9 = frontier_phi_114_99_ladder_112_ladder_9;
                                    frontier_phi_114_99_ladder_10 = frontier_phi_114_99_ladder_112_ladder_10;
                                    frontier_phi_114_99_ladder_11 = frontier_phi_114_99_ladder_112_ladder_11;
                                    frontier_phi_114_99_ladder_12 = frontier_phi_114_99_ladder_112_ladder_12;
                                    frontier_phi_114_99_ladder_13 = frontier_phi_114_99_ladder_112_ladder_13;
                                    frontier_phi_114_99_ladder_14 = frontier_phi_114_99_ladder_112_ladder_14;
                                    frontier_phi_114_99_ladder_15 = frontier_phi_114_99_ladder_112_ladder_15;
                                    frontier_phi_114_99_ladder_16 = frontier_phi_114_99_ladder_112_ladder_16;
                                    frontier_phi_114_99_ladder_17 = frontier_phi_114_99_ladder_112_ladder_17;
                                }
                                else
                                {
                                    float _2138 = max(_50_m0[3u].y, _50_m0[3u].y * _2000);
                                    float _2141 = _2138 * _50_m0[3u].w;
                                    float _2145 = clamp((abs(_2003) - _2141) / (_2138 - _2141), 0.0f, 1.0f);
                                    uint _2147 = uint(_1992 < _1182);
                                    float frontier_phi_114_99_ladder_113_ladder;
                                    float frontier_phi_114_99_ladder_113_ladder_1;
                                    float frontier_phi_114_99_ladder_113_ladder_2;
                                    float frontier_phi_114_99_ladder_113_ladder_3;
                                    float frontier_phi_114_99_ladder_113_ladder_4;
                                    float frontier_phi_114_99_ladder_113_ladder_5;
                                    float frontier_phi_114_99_ladder_113_ladder_6;
                                    uint frontier_phi_114_99_ladder_113_ladder_7;
                                    uint frontier_phi_114_99_ladder_113_ladder_8;
                                    float frontier_phi_114_99_ladder_113_ladder_9;
                                    float frontier_phi_114_99_ladder_113_ladder_10;
                                    float frontier_phi_114_99_ladder_113_ladder_11;
                                    float frontier_phi_114_99_ladder_113_ladder_12;
                                    float frontier_phi_114_99_ladder_113_ladder_13;
                                    float frontier_phi_114_99_ladder_113_ladder_14;
                                    float frontier_phi_114_99_ladder_113_ladder_15;
                                    float frontier_phi_114_99_ladder_113_ladder_16;
                                    uint frontier_phi_114_99_ladder_113_ladder_17;
                                    if (_1481 == 0u)
                                    {
                                        frontier_phi_114_99_ladder_113_ladder = _1485;
                                        frontier_phi_114_99_ladder_113_ladder_1 = _1464;
                                        frontier_phi_114_99_ladder_113_ladder_2 = _1799;
                                        frontier_phi_114_99_ladder_113_ladder_3 = _1813;
                                        frontier_phi_114_99_ladder_113_ladder_4 = _1812;
                                        frontier_phi_114_99_ladder_113_ladder_5 = _1810;
                                        frontier_phi_114_99_ladder_113_ladder_6 = _1809;
                                        frontier_phi_114_99_ladder_113_ladder_7 = _1807;
                                        frontier_phi_114_99_ladder_113_ladder_8 = uint(_2145 > 0.0f);
                                        frontier_phi_114_99_ladder_113_ladder_9 = _1482;
                                        frontier_phi_114_99_ladder_113_ladder_10 = _1483;
                                        frontier_phi_114_99_ladder_113_ladder_11 = _1484;
                                        frontier_phi_114_99_ladder_113_ladder_12 = _1486;
                                        frontier_phi_114_99_ladder_113_ladder_13 = _1487;
                                        frontier_phi_114_99_ladder_113_ladder_14 = _2145;
                                        frontier_phi_114_99_ladder_113_ladder_15 = _1489;
                                        frontier_phi_114_99_ladder_113_ladder_16 = _1490;
                                        frontier_phi_114_99_ladder_113_ladder_17 = _2147;
                                    }
                                    else
                                    {
                                        frontier_phi_114_99_ladder_113_ladder = _1485;
                                        frontier_phi_114_99_ladder_113_ladder_1 = _1464;
                                        frontier_phi_114_99_ladder_113_ladder_2 = _1799;
                                        frontier_phi_114_99_ladder_113_ladder_3 = _1813;
                                        frontier_phi_114_99_ladder_113_ladder_4 = _1812;
                                        frontier_phi_114_99_ladder_113_ladder_5 = _1810;
                                        frontier_phi_114_99_ladder_113_ladder_6 = _1809;
                                        frontier_phi_114_99_ladder_113_ladder_7 = _1807;
                                        frontier_phi_114_99_ladder_113_ladder_8 = _1481;
                                        frontier_phi_114_99_ladder_113_ladder_9 = _1482;
                                        frontier_phi_114_99_ladder_113_ladder_10 = _1483;
                                        frontier_phi_114_99_ladder_113_ladder_11 = _1484;
                                        frontier_phi_114_99_ladder_113_ladder_12 = _1486;
                                        frontier_phi_114_99_ladder_113_ladder_13 = _1487;
                                        frontier_phi_114_99_ladder_113_ladder_14 = _2145;
                                        frontier_phi_114_99_ladder_113_ladder_15 = _1489;
                                        frontier_phi_114_99_ladder_113_ladder_16 = _1490;
                                        frontier_phi_114_99_ladder_113_ladder_17 = _2147;
                                    }
                                    frontier_phi_114_99_ladder = frontier_phi_114_99_ladder_113_ladder;
                                    frontier_phi_114_99_ladder_1 = frontier_phi_114_99_ladder_113_ladder_1;
                                    frontier_phi_114_99_ladder_2 = frontier_phi_114_99_ladder_113_ladder_2;
                                    frontier_phi_114_99_ladder_3 = frontier_phi_114_99_ladder_113_ladder_3;
                                    frontier_phi_114_99_ladder_4 = frontier_phi_114_99_ladder_113_ladder_4;
                                    frontier_phi_114_99_ladder_5 = frontier_phi_114_99_ladder_113_ladder_5;
                                    frontier_phi_114_99_ladder_6 = frontier_phi_114_99_ladder_113_ladder_6;
                                    frontier_phi_114_99_ladder_7 = frontier_phi_114_99_ladder_113_ladder_7;
                                    frontier_phi_114_99_ladder_8 = frontier_phi_114_99_ladder_113_ladder_8;
                                    frontier_phi_114_99_ladder_9 = frontier_phi_114_99_ladder_113_ladder_9;
                                    frontier_phi_114_99_ladder_10 = frontier_phi_114_99_ladder_113_ladder_10;
                                    frontier_phi_114_99_ladder_11 = frontier_phi_114_99_ladder_113_ladder_11;
                                    frontier_phi_114_99_ladder_12 = frontier_phi_114_99_ladder_113_ladder_12;
                                    frontier_phi_114_99_ladder_13 = frontier_phi_114_99_ladder_113_ladder_13;
                                    frontier_phi_114_99_ladder_14 = frontier_phi_114_99_ladder_113_ladder_14;
                                    frontier_phi_114_99_ladder_15 = frontier_phi_114_99_ladder_113_ladder_15;
                                    frontier_phi_114_99_ladder_16 = frontier_phi_114_99_ladder_113_ladder_16;
                                    frontier_phi_114_99_ladder_17 = frontier_phi_114_99_ladder_113_ladder_17;
                                }
                                _1392 = frontier_phi_114_99_ladder_17;
                                _1391 = frontier_phi_114_99_ladder_16;
                                _1390 = frontier_phi_114_99_ladder_15;
                                _1389 = frontier_phi_114_99_ladder_14;
                                _1388 = frontier_phi_114_99_ladder_13;
                                _1387 = frontier_phi_114_99_ladder_12;
                                _1386 = frontier_phi_114_99_ladder;
                                _1385 = frontier_phi_114_99_ladder_11;
                                _1384 = frontier_phi_114_99_ladder_10;
                                _1383 = frontier_phi_114_99_ladder_9;
                                _2149 = frontier_phi_114_99_ladder_8;
                                _2151 = frontier_phi_114_99_ladder_7;
                                _1478 = frontier_phi_114_99_ladder_6;
                                _1476 = frontier_phi_114_99_ladder_5;
                                _1474 = frontier_phi_114_99_ladder_4;
                                _1472 = frontier_phi_114_99_ladder_3;
                                _1470 = frontier_phi_114_99_ladder_2;
                                _1465 = frontier_phi_114_99_ladder_1;
                            }
                            else
                            {
                                bool _2005 = _1481 != 0u;
                                _1392 = _1491;
                                _1391 = _1490;
                                _1390 = _1489;
                                _1389 = _1488;
                                _1388 = _2005 ? _1487 : _1803;
                                _1387 = _2005 ? _1486 : _1804;
                                _1386 = _2005 ? _1485 : _1805;
                                _1385 = _1803;
                                _1384 = _1804;
                                _1383 = _1805;
                                _2149 = _1481;
                                _2151 = _1807;
                                _1478 = _1809;
                                _1476 = _1810;
                                _1474 = _1812;
                                _1472 = _1813;
                                _1470 = _1799;
                                _1465 = (asuint(_50_m0[5u]).y != 0u) ? _1272 : _1464;
                            }
                            float frontier_phi_144_pred;
                            uint frontier_phi_144_pred_1;
                            uint frontier_phi_144_pred_2;
                            float frontier_phi_144_pred_3;
                            float frontier_phi_144_pred_4;
                            bool _2155;
                            bool _2157;
                            for (;;)
                            {
                                _2155 = _1805 < 0.0f;
                                _2157 = _2155 || ((_1803 < 0.0f) || (_1804 < 0.0f));
                                if (!_2157)
                                {
                                    if (!((_1805 > 1.0f) || ((_1803 > _50_m0[51u].x) || (_1804 > _50_m0[51u].y))))
                                    {
                                        frontier_phi_144_pred = _1803;
                                        frontier_phi_144_pred_1 = _2149;
                                        frontier_phi_144_pred_2 = _2151;
                                        frontier_phi_144_pred_3 = _1804;
                                        frontier_phi_144_pred_4 = _1805;
                                        break;
                                    }
                                }
                                if (!_2155)
                                {
                                    frontier_phi_144_pred = _1803;
                                    frontier_phi_144_pred_1 = 1u;
                                    frontier_phi_144_pred_2 = 4294967295u;
                                    frontier_phi_144_pred_3 = _1804;
                                    frontier_phi_144_pred_4 = _1805;
                                    break;
                                }
                                float _2458 = (-0.0f) - _1805;
                                float _2459 = _2458 / _1022;
                                frontier_phi_144_pred = (_2459 * _1019) + _1803;
                                frontier_phi_144_pred_1 = 1u;
                                frontier_phi_144_pred_2 = 4294967295u;
                                frontier_phi_144_pred_3 = (_2459 * _1021) + _1804;
                                frontier_phi_144_pred_4 = _2458 + _1805;
                                break;
                            }
                            _1380 = frontier_phi_144_pred;
                            _1382 = frontier_phi_144_pred_1;
                            _1480 = frontier_phi_144_pred_2;
                            _1379 = frontier_phi_144_pred_3;
                            _1378 = frontier_phi_144_pred_4;
                            if ((_1377 < _209) && (int(_1480) > int(4294967295u)))
                            {
                                _1463 = _1377;
                                _1464 = _1465;
                                _1466 = _1378;
                                _1467 = _1379;
                                _1468 = _1380;
                                _1469 = _1470;
                                _1471 = _1472;
                                _1473 = _1474;
                                _1475 = _1476;
                                _1477 = _1478;
                                _1479 = _1480;
                                _1481 = _1382;
                                _1482 = _1383;
                                _1483 = _1384;
                                _1484 = _1385;
                                _1485 = _1386;
                                _1486 = _1387;
                                _1487 = _1388;
                                _1488 = _1389;
                                _1489 = _1390;
                                _1490 = _1391;
                                _1491 = _1392;
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        _1376 = _1377;
                        _1072 = _1378;
                        _1070 = _1379;
                        _1068 = _1380;
                        _1381 = _1382;
                        _1062 = _1383;
                        _1060 = _1384;
                        _1058 = _1385;
                        _1056 = _1386;
                        _1054 = _1387;
                        _1052 = _1388;
                        _1050 = _1389;
                        _1048 = _1390;
                        _1046 = _1391;
                        _1044 = _1392;
                    }
                    bool _1393 = _1376 >= _209;
                    _1041 = uint(_1393);
                    _1043 = _1044;
                    _1045 = _1046;
                    _1047 = _1048;
                    _1049 = _1050;
                    _1051 = _1052;
                    _1053 = _1054;
                    _1055 = _1056;
                    _1057 = _1058;
                    _1059 = _1060;
                    _1061 = _1062;
                    _1063 = _1393 ? 1u : _1381;
                    _1065 = uint(_1376 <= _209);
                    _1067 = _1068;
                    _1069 = _1070;
                    _1071 = _1072;
                }
                float _1079 = _50_m0[51u].z * 2.0f;
                float _1082 = (_1079 * _982) + (-1.0f);
                float _1083 = ((1.0f - (_50_m0[51u].w * _983)) * 2.0f) + (-1.0f);
                float _1099 = mad(_190, _975, mad(_183, _1083, _1082 * _176)) + _197;
                float _1100 = (mad(_187, _975, mad(_180, _1083, _1082 * _173)) + _194) / _1099;
                float _1101 = (mad(_188, _975, mad(_181, _1083, _1082 * _174)) + _195) / _1099;
                float _1102 = (mad(_189, _975, mad(_182, _1083, _1082 * _175)) + _196) / _1099;
                float _1107 = (_1079 * _1067) + (-1.0f);
                float _1108 = ((1.0f - (_50_m0[51u].w * _1069)) * 2.0f) + (-1.0f);
                float _1124 = mad(_190, _1071, mad(_183, _1108, _1107 * _176)) + _197;
                float _1128 = ((mad(_187, _1071, mad(_180, _1108, _1107 * _173)) + _194) / _1124) - _1100;
                float _1129 = ((mad(_188, _1071, mad(_181, _1108, _1107 * _174)) + _195) / _1124) - _1101;
                float _1130 = ((mad(_189, _1071, mad(_182, _1108, _1107 * _175)) + _196) / _1124) - _1102;
                float _1220;
                uint _1222;
                float _1224;
                if (_1065 == 0u)
                {
                    _1220 = 0.0f;
                    _1222 = _1063;
                    _1224 = 0.0f;
                }
                else
                {
                    float _1267 = float(_238);
                    float _1268 = float(_239);
                    float frontier_phi_44_45_ladder;
                    uint frontier_phi_44_45_ladder_1;
                    float frontier_phi_44_45_ladder_2;
                    if ((_1067 < 0.0f) || (_1069 < 0.0f))
                    {
                        frontier_phi_44_45_ladder = 0.0f;
                        frontier_phi_44_45_ladder_1 = _1063;
                        frontier_phi_44_45_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_44_45_ladder_53_ladder;
                        uint frontier_phi_44_45_ladder_53_ladder_1;
                        float frontier_phi_44_45_ladder_53_ladder_2;
                        if ((_1071 >= 1.0f) || ((_1067 > _50_m0[51u].x) || (_1069 > _50_m0[51u].y)))
                        {
                            frontier_phi_44_45_ladder_53_ladder = 0.0f;
                            frontier_phi_44_45_ladder_53_ladder_1 = _1063;
                            frontier_phi_44_45_ladder_53_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_44_45_ladder_53_ladder_64_ladder;
                            uint frontier_phi_44_45_ladder_53_ladder_64_ladder_1;
                            float frontier_phi_44_45_ladder_53_ladder_64_ladder_2;
                            for (;;)
                            {
                                if ((abs(_1067 - _259) < (2.0f / _1267)) && (abs(_1069 - _260) < (2.0f / _1268)))
                                {
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder = 0.0f;
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder_1 = _1063;
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_98;
                                    float frontier_phi_98_pred;
                                    uint frontier_phi_98_pred_1;
                                    float frontier_phi_98_pred_2;
                                    uint _1682;
                                    uint _1683;
                                    bool _1684;
                                    for (;;)
                                    {
                                        _1682 = uint(int(_1067 * _1267));
                                        _1683 = uint(int(_1069 * _1268));
                                        _1684 = _1029 && _936;
                                        if (!_1684)
                                        {
                                            if (!(dot(float3(_1128, _1129, _1130), float3(_1128, _1129, _1130)) < _50_m0[4u].w))
                                            {
                                                ladder_phi_98 = false;
                                                frontier_phi_98_pred = 0.0f;
                                                frontier_phi_98_pred_1 = _1063;
                                                frontier_phi_98_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1760 = _24[22u].Load(int3(uint2(_1682, _1683), 0u));
                                        uint _1762 = _1760.x;
                                        float _2106;
                                        float _2107;
                                        float _2108;
                                        if (_1762 == 0u)
                                        {
                                            uint4 _1889 = _24[1u].Load(int3(uint2(_1682, _1683), 0u));
                                            uint _1891 = _1889.x;
                                            float _1899 = (float((_1891 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1900 = (float(_1891 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1904 = (1.0f - abs(_1899)) - abs(_1900);
                                            float _1906 = clamp((-0.0f) - _1904, 0.0f, 1.0f);
                                            float _1907 = (-0.0f) - _1906;
                                            _2106 = ((_1899 >= 0.0f) ? _1907 : _1906) + _1899;
                                            _2107 = ((_1900 >= 0.0f) ? _1907 : _1906) + _1900;
                                            _2108 = _1904;
                                        }
                                        else
                                        {
                                            float _1921 = (float((_1762 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1922 = (float(_1762 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1926 = (1.0f - abs(_1921)) - abs(_1922);
                                            float _1928 = clamp((-0.0f) - _1926, 0.0f, 1.0f);
                                            float _1929 = (-0.0f) - _1928;
                                            _2106 = ((_1921 >= 0.0f) ? _1929 : _1928) + _1921;
                                            _2107 = ((_1922 >= 0.0f) ? _1929 : _1928) + _1922;
                                            _2108 = _1926;
                                        }
                                        float _2112 = rsqrt(dot(float3(_2106, _2107, _2108), float3(_2106, _2107, _2108)));
                                        if (dot(float3(_2112 * _2106, _2112 * _2107, _2112 * _2108), float3(_1128, _1129, _1130)) > 0.0f)
                                        {
                                            ladder_phi_98 = true;
                                            frontier_phi_98_pred = 0.0f;
                                            frontier_phi_98_pred_1 = _1063;
                                            frontier_phi_98_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_98 = false;
                                            frontier_phi_98_pred = 0.0f;
                                            frontier_phi_98_pred_1 = _1063;
                                            frontier_phi_98_pred_2 = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_98)
                                    {
                                        frontier_phi_44_45_ladder_53_ladder_64_ladder = frontier_phi_98_pred;
                                        frontier_phi_44_45_ladder_53_ladder_64_ladder_1 = frontier_phi_98_pred_1;
                                        frontier_phi_44_45_ladder_53_ladder_64_ladder_2 = frontier_phi_98_pred_2;
                                        break;
                                    }
                                    float _1940 = _50_m0[51u].z * _1067;
                                    float _1941 = _50_m0[51u].w * _1069;
                                    float _1943 = (_1268 / _1267) * 0.0500000007450580596923828125f;
                                    float _1948 = clamp(_1940 / _1943, 0.0f, 1.0f);
                                    float _1949 = clamp(_1941 * 20.0f, 0.0f, 1.0f);
                                    float _1961 = clamp(((_1940 + (-1.0f)) + _1943) / _1943, 0.0f, 1.0f);
                                    float _1962 = clamp((_1941 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1973 = _1948 * _1949;
                                    precise float _1974 = _1973 * _1973;
                                    float _1978 = ((((3.0f - (_1949 * 2.0f)) * (3.0f - (_1948 * 2.0f))) * _1974) * (1.0f - ((_1961 * _1961) * (3.0f - (_1961 * 2.0f))))) * (1.0f - ((_1962 * _1962) * (3.0f - (_1962 * 2.0f))));
                                    bool _1981 = (_1063 != 0u) || (_1978 >= 1.0f);
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder = _1978 * float(_481 > 0.0f);
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder_1 = _1981 ? _1063 : 1u;
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder_2 = _1981 ? 0.0f : _1978;
                                    break;
                                }
                            }
                            frontier_phi_44_45_ladder_53_ladder = frontier_phi_44_45_ladder_53_ladder_64_ladder;
                            frontier_phi_44_45_ladder_53_ladder_1 = frontier_phi_44_45_ladder_53_ladder_64_ladder_1;
                            frontier_phi_44_45_ladder_53_ladder_2 = frontier_phi_44_45_ladder_53_ladder_64_ladder_2;
                        }
                        frontier_phi_44_45_ladder = frontier_phi_44_45_ladder_53_ladder;
                        frontier_phi_44_45_ladder_1 = frontier_phi_44_45_ladder_53_ladder_1;
                        frontier_phi_44_45_ladder_2 = frontier_phi_44_45_ladder_53_ladder_2;
                    }
                    _1220 = frontier_phi_44_45_ladder_2;
                    _1222 = frontier_phi_44_45_ladder_1;
                    _1224 = frontier_phi_44_45_ladder;
                }
                uint _1351;
                uint _1353;
                float _1265;
                for (;;)
                {
                    _1265 = ((((exp2(log2(clamp((sqrt(((_1101 * _1101) + (_1100 * _1100)) + (_1102 * _1102)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _412) * exp2(log2(clamp((_1101 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_398, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    if (_1224 > 0.0f)
                    {
                        uint _1313 = uint((_202 * 0.999989986419677734375f) * clamp(_1067, 0.0f, 1.0f));
                        uint _1314 = uint((_204 * 0.999989986419677734375f) * clamp(_1069, 0.0f, 1.0f));
                        uint4 _1317 = _24[2u].Load(int3(uint2(_1313, _1314), 0u));
                        uint _1320 = _1317.w;
                        uint4 _1325 = _24[15u].Load(int3(uint2(_1313, _1314), 0u));
                        uint _1327 = _1325.y;
                        uint _1333 = ((_1327 & 64u) != 0u) ? uint((_1327 & 4294967167u) != 66u) : 4294967295u;
                        uint _1334 = _1320 & 128u;
                        uint _1336 = (_1334 != 0u) ? 1u : ((_1317.x << 7u) | _1320);
                        uint4 _1339 = _16.Load(_1336 * 4u);
                        uint _1340 = _1339.x;
                        uint _1347 = ((_1340 & 1u) != 0u) ? 0u : 18u;
                        uint _1349 = uint(min(int(uint(max(int(_1333), int(0u)))), int(1u)));
                        uint _1416;
                        if (_1334 == 0u)
                        {
                            _1416 = (((_1340 & 2097152u) != 0u) && (_1333 == _1349)) ? (_1347 | 128u) : _1347;
                        }
                        else
                        {
                            _1416 = _1320;
                        }
                        uint _1417 = _16.Load((_1336 * 4u) + 1u).x & 512u;
                        bool _1420 = (_1416 & 144u) == 0u;
                        if (_1417 == 0u)
                        {
                            if (_1420 || ((_1340 & 1u) != 0u))
                            {
                                _1351 = 0u;
                                _1353 = 0u;
                                break;
                            }
                        }
                        else
                        {
                            if (_1420)
                            {
                                _1351 = 0u;
                                _1353 = 0u;
                                break;
                            }
                        }
                        bool _1739 = ((_1416 & 128u) | _1417) != 0u;
                        uint _1352;
                        if (_1739)
                        {
                            _1352 = 1u;
                        }
                        else
                        {
                            _1352 = (((_1340 >> 14u) & 2u) ^ 2u) + 3u;
                        }
                        if (((_1340 & 268435472u) == 16u) && (((_1340 & 33554432u) == 0u) || _1739))
                        {
                            _1351 = 2u;
                            _1353 = 0u;
                            break;
                        }
                        _1351 = _1352;
                        _1353 = (_1352 == 1u) ? _1349 : 0u;
                        break;
                    }
                    else
                    {
                        _1351 = 0u;
                        _1353 = 0u;
                        break;
                    }
                }
                bool _1425;
                float _1428;
                float _1430;
                float _1432;
                float _1434;
                float _1436;
                float _1438;
                float _1440;
                float _1441;
                float _1442;
                float _1444;
                float _1357;
                float _1360;
                float _1363;
                float _1367;
                float _1368;
                for (;;)
                {
                    _1357 = mad(_167, _932, mad(_161, _931, _930 * _155));
                    _1360 = mad(_168, _932, mad(_162, _931, _930 * _156));
                    _1363 = mad(_169, _932, mad(_163, _931, _930 * _157));
                    bool _1366 = (_1222 != 0u) || (_1224 < 1.0f);
                    _1367 = _1366 ? 0.0f : 1.0f;
                    _1368 = _1366 ? 0.0f : 0.5f;
                    if (_1366)
                    {
                        uint4 _1424 = asuint(_50_m0[60u]);
                        if (_1029)
                        {
                            if (int(_410) < int(1u))
                            {
                                if (_1424.x == 0u)
                                {
                                    _1425 = false;
                                    _1428 = 0.0f;
                                    _1430 = 0.0f;
                                    _1432 = 0.0f;
                                    _1434 = _1057;
                                    _1436 = _1059;
                                    _1438 = _1061;
                                    _1440 = 0.0f;
                                    _1441 = 0.0f;
                                    _1442 = 0.0f;
                                    _1444 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1424.y == 0u)
                                {
                                    _1425 = false;
                                    _1428 = 0.0f;
                                    _1430 = 0.0f;
                                    _1432 = 0.0f;
                                    _1434 = _1057;
                                    _1436 = _1059;
                                    _1438 = _1061;
                                    _1440 = 0.0f;
                                    _1441 = 0.0f;
                                    _1442 = 0.0f;
                                    _1444 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1424.z == 0u)
                            {
                                _1425 = false;
                                _1428 = 0.0f;
                                _1430 = 0.0f;
                                _1432 = 0.0f;
                                _1434 = _1057;
                                _1436 = _1059;
                                _1438 = _1061;
                                _1440 = 0.0f;
                                _1441 = 0.0f;
                                _1442 = 0.0f;
                                _1444 = 0.0f;
                                break;
                            }
                        }
                        if (_1220 > 0.0f)
                        {
                            _1425 = false;
                            _1428 = 0.0f;
                            _1430 = 0.0f;
                            _1432 = 0.0f;
                            _1434 = _1057;
                            _1436 = _1059;
                            _1438 = _1061;
                            _1440 = 0.0f;
                            _1441 = 1.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        if (((_1051 <= 0.0f) || (_1053 <= 0.0f)) || (_1055 <= 0.0f))
                        {
                            _1425 = false;
                            _1428 = 0.0f;
                            _1430 = 0.0f;
                            _1432 = 0.0f;
                            _1434 = _1057;
                            _1436 = _1059;
                            _1438 = _1061;
                            _1440 = 0.0f;
                            _1441 = 1.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        if ((_1055 >= 1.0f) || ((_1051 >= _50_m0[51u].x) || (_1053 >= _50_m0[51u].y)))
                        {
                            _1425 = false;
                            _1428 = 0.0f;
                            _1430 = 0.0f;
                            _1432 = 0.0f;
                            _1434 = _1057;
                            _1436 = _1059;
                            _1438 = _1061;
                            _1440 = 0.0f;
                            _1441 = 1.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        uint _2475;
                        uint _2477;
                        uint _2174;
                        uint _2175;
                        bool _2181;
                        for (;;)
                        {
                            _2174 = uint(clamp(_1045, 0.0f, 1.0f) * _202);
                            _2175 = uint(clamp(_1047, 0.0f, 1.0f) * _204);
                            _2181 = _20[21u].Load(int3(uint2(_2174, _2175), 0u)).x > 0.0f;
                            if (_2181)
                            {
                                uint _2377 = _24[23u].Load(int3(uint2(_2174, _2175), 0u)).y + 4294967295u;
                                _2475 = (uint(int(_2377) >> int(31u)) & 3u) + 1u;
                                _2477 = (int(_2377) < int(0u)) ? 0u : _2377;
                                break;
                            }
                            else
                            {
                                uint4 _2385 = _24[2u].Load(int3(uint2(_2174, _2175), 0u));
                                uint _2388 = _2385.w;
                                uint4 _2393 = _24[15u].Load(int3(uint2(_2174, _2175), 0u));
                                uint _2395 = _2393.y;
                                uint _2401 = ((_2395 & 64u) != 0u) ? uint((_2395 & 4294967167u) != 66u) : 4294967295u;
                                uint _2402 = _2388 & 128u;
                                uint _2404 = (_2402 != 0u) ? 1u : ((_2385.x << 7u) | _2388);
                                uint4 _2407 = _16.Load(_2404 * 4u);
                                uint _2408 = _2407.x;
                                uint _2415 = ((_2408 & 1u) != 0u) ? 0u : 18u;
                                uint _2417 = uint(min(int(uint(max(int(_2401), int(0u)))), int(1u)));
                                uint _2488;
                                if (_2402 == 0u)
                                {
                                    _2488 = (((_2408 & 2097152u) != 0u) && (_2401 == _2417)) ? (_2415 | 128u) : _2415;
                                }
                                else
                                {
                                    _2488 = _2388;
                                }
                                uint _2489 = _16.Load((_2404 * 4u) + 1u).x & 512u;
                                bool _2492 = (_2488 & 144u) == 0u;
                                if (_2489 == 0u)
                                {
                                    if (_2492 || ((_2408 & 1u) != 0u))
                                    {
                                        _2475 = 0u;
                                        _2477 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2492)
                                    {
                                        _2475 = 0u;
                                        _2477 = 0u;
                                        break;
                                    }
                                }
                                bool _2768 = ((_2488 & 128u) | _2489) != 0u;
                                uint _2476;
                                if (_2768)
                                {
                                    _2476 = 1u;
                                }
                                else
                                {
                                    _2476 = (((_2408 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_2408 & 268435472u) == 16u) && (((_2408 & 33554432u) == 0u) || _2768))
                                {
                                    _2475 = 2u;
                                    _2477 = 0u;
                                    break;
                                }
                                _2475 = _2476;
                                _2477 = (_2476 == 1u) ? _2417 : 0u;
                                break;
                            }
                        }
                        if ((_402 != _2475) || (_410 != _2477))
                        {
                            _1425 = false;
                            _1428 = 0.0f;
                            _1430 = 0.0f;
                            _1432 = 0.0f;
                            _1434 = _1057;
                            _1436 = _1059;
                            _1438 = _1061;
                            _1440 = 0.0f;
                            _1441 = 1.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2619 = _1051 * 2.0f;
                        float _2622 = (_50_m0[51u].z * _2619) + (-1.0f);
                        float _2623 = ((1.0f - (_50_m0[51u].w * _1053)) * 2.0f) + (-1.0f);
                        float _2639 = mad(_190, _1055, mad(_183, _2623, _2622 * _176)) + _197;
                        float _2640 = (mad(_187, _1055, mad(_180, _2623, _2622 * _173)) + _194) / _2639;
                        float _2641 = (mad(_188, _1055, mad(_181, _2623, _2622 * _174)) + _195) / _2639;
                        float _2642 = (mad(_189, _1055, mad(_182, _2623, _2622 * _175)) + _196) / _2639;
                        if (sqrt(((_2641 * _2641) + (_2640 * _2640)) + (_2642 * _2642)) > _50_m0[58u].w)
                        {
                            _1425 = false;
                            _1428 = 0.0f;
                            _1430 = 0.0f;
                            _1432 = 0.0f;
                            _1434 = _1057;
                            _1436 = _1059;
                            _1438 = _1061;
                            _1440 = 0.0f;
                            _1441 = 0.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2748 = _2640 - _1100;
                        float _2749 = _2641 - _1101;
                        float _2750 = _2642 - _1102;
                        float _2756 = sqrt(((_2749 * _2749) + (_2748 * _2748)) + (_2750 * _2750));
                        float _2764 = min(_50_m0[59u].y, max(0.0f, _2756 + (-0.001000000047497451305389404296875f)));
                        float _2787;
                        if (_1029)
                        {
                            _2787 = min(_50_m0[59u].x, _50_m0[4u].z + _2764);
                        }
                        else
                        {
                            _2787 = _50_m0[59u].x;
                        }
                        float _2788 = _2787 - _2756;
                        if (!(_2788 > 0.0f))
                        {
                            _1425 = false;
                            _1428 = 0.0f;
                            _1430 = 0.0f;
                            _1432 = 0.0f;
                            _1434 = _1057;
                            _1436 = _1059;
                            _1438 = _1061;
                            _1440 = 1.0f;
                            _1441 = 1.0f;
                            _1442 = 0.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2847 = _2640 - (_2764 * _1357);
                        float _2848 = _2641 - (_2764 * _1360);
                        float _2849 = _2642 - (_2764 * _1363);
                        RayDesc _2ident = {float3(mad(_2849, _50_m0[46u].z, mad(_2848, _50_m0[46u].y, _50_m0[46u].x * _2847)) + _50_m0[46u].w, mad(_2849, _50_m0[47u].z, mad(_2848, _50_m0[47u].y, _50_m0[47u].x * _2847)) + _50_m0[47u].w, mad(_2849, _50_m0[48u].z, mad(_2848, _50_m0[48u].y, _50_m0[48u].x * _2847)) + _50_m0[48u].w), 0.0f, float3(mad(_1363, _50_m0[46u].z, mad(_1360, _50_m0[46u].y, _50_m0[46u].x * _1357)), mad(_1363, _50_m0[47u].z, mad(_1360, _50_m0[47u].y, _50_m0[47u].x * _1357)), mad(_1363, _50_m0[48u].z, mad(_1360, _50_m0[48u].y, _50_m0[48u].x * _1357))), _2788};
                        _2852.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2899 = _2852.Proceed();
                        uint _2900 = _2852.CommittedStatus();
                        if (!(_2900 == 1u))
                        {
                            _1425 = false;
                            _1428 = 0.0f;
                            _1430 = 0.0f;
                            _1432 = 0.0f;
                            _1434 = _1057;
                            _1436 = _1059;
                            _1438 = _1061;
                            _1440 = 1.0f;
                            _1441 = 0.0f;
                            _1442 = 0.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2914 = _2852.CommittedRayT();
                        if (!((_2914 < _2788) && (_2914 > 0.0f)))
                        {
                            _1425 = false;
                            _1428 = 0.0f;
                            _1430 = 0.0f;
                            _1432 = 0.0f;
                            _1434 = _1057;
                            _1436 = _1059;
                            _1438 = _1061;
                            _1440 = 1.0f;
                            _1441 = 0.0f;
                            _1442 = 0.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2928 = (_50_m0[51u].z * _2619) + (-1.0f);
                        float _2929 = ((1.0f - (_50_m0[51u].w * _1053)) * 2.0f) + (-1.0f);
                        float _2945 = mad(_144, _1055, mad(_137, _2929, _2928 * _130)) + _151;
                        float _2949 = _2914 - _2764;
                        float _2953 = ((mad(_141, _1055, mad(_134, _2929, _2928 * _127)) + _148) / _2945) + (_2949 * _930);
                        float _2954 = ((mad(_142, _1055, mad(_135, _2929, _2928 * _128)) + _149) / _2945) + (_2949 * _931);
                        float _2955 = ((mad(_143, _1055, mad(_136, _2929, _2928 * _129)) + _150) / _2945) + (_2949 * _932);
                        float _2971 = mad(_116, _2955, mad(_109, _2954, _2953 * _102)) + _123;
                        float _1439 = (mad(_115, _2955, mad(_108, _2954, _2953 * _101)) + _122) / _2971;
                        float _1435 = ((((mad(_113, _2955, mad(_106, _2954, _2953 * _99)) + _120) / _2971) * 0.5f) + 0.5f) * _50_m0[51u].x;
                        float _1437 = (0.5f - (((mad(_114, _2955, mad(_107, _2954, _2953 * _100)) + _121) / _2971) * 0.5f)) * _50_m0[51u].y;
                        float _2980 = _1435 * _50_m0[51u].z;
                        float _2981 = _1437 * _50_m0[51u].w;
                        float _2983 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2986 = clamp(_2980 / _2983, 0.0f, 1.0f);
                        float _2987 = clamp(_2981 * 20.0f, 0.0f, 1.0f);
                        float _2997 = clamp(((_2980 + (-1.0f)) + _2983) / _2983, 0.0f, 1.0f);
                        float _2998 = clamp((_2981 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _3009 = _2986 * _2987;
                        precise float _3010 = _3009 * _3009;
                        if ((((((3.0f - (_2987 * 2.0f)) * (3.0f - (_2986 * 2.0f))) * _3010) * (1.0f - ((_2997 * _2997) * (3.0f - (_2997 * 2.0f))))) * (1.0f - ((_2998 * _2998) * (3.0f - (_2998 * 2.0f))))) < 1.0f)
                        {
                            _1425 = false;
                            _1428 = 0.0f;
                            _1430 = 0.0f;
                            _1432 = 0.0f;
                            _1434 = _1435;
                            _1436 = _1437;
                            _1438 = _1439;
                            _1440 = _1367;
                            _1441 = _1367;
                            _1442 = 0.0f;
                            _1444 = _1368;
                            break;
                        }
                        _1425 = true;
                        _1428 = _50_m0[1u].w;
                        _1430 = _50_m0[2u].x;
                        _1432 = _50_m0[2u].y;
                        _1434 = _1435;
                        _1436 = _1437;
                        _1438 = _1439;
                        _1440 = 0.0f;
                        _1441 = 1.0f;
                        _1442 = 0.0f;
                        _1444 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1425 = false;
                        _1428 = 0.0f;
                        _1430 = 0.0f;
                        _1432 = 0.0f;
                        _1434 = _1057;
                        _1436 = _1059;
                        _1438 = _1061;
                        _1440 = 1.0f;
                        _1441 = 1.0f;
                        _1442 = 0.0f;
                        _1444 = 0.5f;
                        break;
                    }
                }
                uint4 _1447 = asuint(_55_m0[0u]);
                float _1449 = float(_1447.x);
                float _1451 = float(_1447.y);
                float _1510;
                float _1512;
                float _1514;
                if ((_1224 >= 1.0f) || _1425)
                {
                    _1510 = _1434;
                    _1512 = _1436;
                    _1514 = _1438;
                }
                else
                {
                    float _1667 = (-0.0f) - _975;
                    float _1668 = _1667 / _1022;
                    float _1671 = (_1668 * _1019) + _982;
                    float _1672 = (_1668 * _1021) + _983;
                    float _1673 = _1667 + _975;
                    _1510 = ((_1434 - _1671) * _1224) + _1671;
                    _1512 = ((_1436 - _1672) * _1224) + _1672;
                    _1514 = ((_1438 - _1673) * _1224) + _1673;
                }
                float _1525 = ((_1510 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _1526 = ((1.0f - (_50_m0[51u].w * _1512)) * 2.0f) + (-1.0f);
                float _1542 = mad(_144, _1514, mad(_137, _1526, _1525 * _130)) + _151;
                float _1546 = ((mad(_141, _1514, mad(_134, _1526, _1525 * _127)) + _148) / _1542) - _954;
                float _1547 = ((mad(_142, _1514, mad(_135, _1526, _1525 * _128)) + _149) / _1542) - _955;
                float _1548 = ((mad(_143, _1514, mad(_136, _1526, _1525 * _129)) + _150) / _1542) - _956;
                float _1554 = sqrt(((_1547 * _1547) + (_1546 * _1546)) + (_1548 * _1548));
                float _1555 = _1554 * _784;
                float _1556 = _1554 * _785;
                float _1557 = _1554 * _786;
                float _1558 = _1554 * (_897 / _900);
                float _1559 = _1554 * (_898 / _900);
                float _1560 = _1554 * (_899 / _900);
                float _1564 = dot(float3(_1555, _1556, _1557), float3(_770, _773, _776)) * 2.0f;
                float _1574 = dot(float3(_1558, _1559, _1560), float3(_770, _773, _776)) * 2.0f;
                float _1601 = (_1555 - (_1564 * _770)) + _954;
                float _1602 = (_1556 - (_1564 * _773)) + _955;
                float _1603 = (_1557 - (_1564 * _776)) + _956;
                float _1615 = mad(_50_m0[24u].w, _1603, mad(_50_m0[23u].w, _1602, _1601 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1620 = (_1558 - (_1574 * _770)) + _954;
                float _1621 = (_1559 - (_1574 * _773)) + _955;
                float _1622 = (_1560 - (_1574 * _776)) + _956;
                float _1634 = mad(_50_m0[24u].w, _1622, mad(_50_m0[23u].w, _1621, _1620 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1640 = (_50_m0[51u].x * ((((mad(_50_m0[24u].x, _1603, mad(_50_m0[23u].x, _1602, _1601 * _50_m0[22u].x)) + _50_m0[25u].x) / _1615) - ((mad(_50_m0[24u].x, _1622, mad(_50_m0[23u].x, _1621, _1620 * _50_m0[22u].x)) + _50_m0[25u].x) / _1634)) * 0.5f)) * _1449;
                float _1644 = (_50_m0[51u].y * ((((mad(_50_m0[24u].y, _1622, mad(_50_m0[23u].y, _1621, _1620 * _50_m0[22u].y)) + _50_m0[25u].y) / _1634) - ((mad(_50_m0[24u].y, _1603, mad(_50_m0[23u].y, _1602, _1601 * _50_m0[22u].y)) + _50_m0[25u].y) / _1615)) * 0.5f)) * _1451;
                uint4 _1658 = asuint(_50_m0[55u]);
                float _1663 = clamp(log2((sqrt((_1644 * _1644) + (_1640 * _1640)) * 2.0f) / _50_m0[0u].w) / float(_1658.x + 4294967295u), 0.0f, 1.0f);
                uint _1748;
                float _1749;
                float _1750;
                float _1752;
                float _1754;
                if (_1029 && (_1658.z != 0u))
                {
                    float frontier_phi_84_83_ladder;
                    uint frontier_phi_84_83_ladder_1;
                    float frontier_phi_84_83_ladder_2;
                    float frontier_phi_84_83_ladder_3;
                    float frontier_phi_84_83_ladder_4;
                    if ((_1351 != 1u) || (_1353 != 0u))
                    {
                        float _1869 = _975 / max(9.9999999747524270787835121154785e-07f, (-0.0f) - _1022);
                        float _1874 = ((_1869 * _1019) + _982) / _50_m0[51u].x;
                        float _1875 = ((_1869 * _1021) + _983) / _50_m0[51u].y;
                        float _2045;
                        if (_1875 < 0.300000011920928955078125f)
                        {
                            float _2037 = (0.300000011920928955078125f - _1875) * 3.3333332538604736328125f;
                            _2045 = 0.300000011920928955078125f - ((_2037 / sqrt((_2037 * _2037) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _2045 = _1875;
                        }
                        float _2190;
                        if (_1874 < 0.300000011920928955078125f)
                        {
                            float _2183 = (0.300000011920928955078125f - _1874) * 3.3333332538604736328125f;
                            _2190 = 0.300000011920928955078125f - ((_2183 / sqrt((_2183 * _2183) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _2190 = _1874;
                        }
                        float _2429;
                        if ((1.0f - _2190) < 0.300000011920928955078125f)
                        {
                            float _2421 = (_2190 + (-0.699999988079071044921875f)) * 3.3333332538604736328125f;
                            _2429 = ((_2421 / sqrt((_2421 * _2421) + 1.0f)) * 0.300000011920928955078125f) + 0.699999988079071044921875f;
                        }
                        else
                        {
                            _2429 = _2190;
                        }
                        frontier_phi_84_83_ladder = _2429 * _50_m0[51u].x;
                        frontier_phi_84_83_ladder_1 = 0u;
                        frontier_phi_84_83_ladder_2 = 0.0f;
                        frontier_phi_84_83_ladder_3 = _2045 * _50_m0[51u].y;
                        frontier_phi_84_83_ladder_4 = 1.0f - clamp((_932 + (-0.25f)) * (-4.0f), 0.0f, 1.0f);
                    }
                    else
                    {
                        frontier_phi_84_83_ladder = _1067;
                        frontier_phi_84_83_ladder_1 = _1041;
                        frontier_phi_84_83_ladder_2 = _1049;
                        frontier_phi_84_83_ladder_3 = _1069;
                        frontier_phi_84_83_ladder_4 = _1224;
                    }
                    _1748 = frontier_phi_84_83_ladder_1;
                    _1749 = frontier_phi_84_83_ladder_2;
                    _1750 = frontier_phi_84_83_ladder;
                    _1752 = frontier_phi_84_83_ladder_3;
                    _1754 = frontier_phi_84_83_ladder_4;
                }
                else
                {
                    _1748 = _1041;
                    _1749 = _1049;
                    _1750 = _1067;
                    _1752 = _1069;
                    _1754 = _1224;
                }
                bool _1756 = _402 != 1u;
                float _2193;
                float _2195;
                float _2197;
                float _2199;
                if (_1754 == 0.0f)
                {
                    float _2047;
                    float _2049;
                    float _2051;
                    float _2053;
                    if (_1425)
                    {
                        _2047 = _1428;
                        _2049 = _1430;
                        _2051 = _1432;
                        _2053 = 1.0f;
                    }
                    else
                    {
                        float frontier_phi_107_108_ladder;
                        float frontier_phi_107_108_ladder_1;
                        float frontier_phi_107_108_ladder_2;
                        float frontier_phi_107_108_ladder_3;
                        if (_1756)
                        {
                            float _2250 = _1360 * _1265;
                            float _2254 = rsqrt(dot(float3(_1357, _2250, _1363), float3(_1357, _2250, _1363)));
                            float4 _2264 = _28[4u].SampleLevel(_59, float3(_2254 * _1357, _2254 * _2250, _2254 * _1363), 0.0f);
                            frontier_phi_107_108_ladder = 1.0f;
                            frontier_phi_107_108_ladder_1 = _2264.z;
                            frontier_phi_107_108_ladder_2 = _2264.y;
                            frontier_phi_107_108_ladder_3 = _2264.x;
                        }
                        else
                        {
                            frontier_phi_107_108_ladder = 0.0f;
                            frontier_phi_107_108_ladder_1 = 0.0f;
                            frontier_phi_107_108_ladder_2 = 0.0f;
                            frontier_phi_107_108_ladder_3 = 0.0f;
                        }
                        _2047 = frontier_phi_107_108_ladder_3;
                        _2049 = frontier_phi_107_108_ladder_2;
                        _2051 = frontier_phi_107_108_ladder_1;
                        _2053 = frontier_phi_107_108_ladder;
                    }
                    _2193 = _2047 * _481;
                    _2195 = _2049 * _481;
                    _2197 = _2051 * _481;
                    _2199 = _2053 * _481;
                }
                else
                {
                    float _2290;
                    float _2292;
                    float _2295;
                    float _2297;
                    if (_12.Load(int3(uint2(uint(_1750 * _1449), uint(_1752 * _1451)), 0u)).x > 0.0f)
                    {
                        uint _2059_dummy_parameter;
                        uint2 _2059 = spvTextureSize(_14, 0u, _2059_dummy_parameter);
                        float4 _2068 = _14.Load(int3(uint2(uint(float(_2059.x) * _1750), uint(float(_2059.y) * _1752)), 0u));
                        float _2072 = _2068.x * 0.5f;
                        float _2073 = _2068.y * (-0.5f);
                        float4 _2092 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _2072) + (_50_m0[52u].x * _1750), (_50_m0[52u].w * _2073) + (_50_m0[52u].y * _1752)), 0.0f);
                        float _2102 = _50_m0[54u].x * _2092.x;
                        float _2103 = _50_m0[54u].x * _2092.y;
                        float _2104 = _50_m0[54u].x * _2092.z;
                        float _2277;
                        if (_1029)
                        {
                            float frontier_phi_123_122_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _975))) < 5.0f)
                            {
                                float _2445 = sqrt((_2072 * _2072) + (_2073 * _2073));
                                float frontier_phi_123_122_ladder_139_ladder;
                                if (_2445 > 0.0500000007450580596923828125f)
                                {
                                    float _2500 = _1750 - _982;
                                    float _2501 = _1752 - _983;
                                    float frontier_phi_123_122_ladder_139_ladder_152_ladder;
                                    if (_2445 > sqrt((_2501 * _2501) + (_2500 * _2500)))
                                    {
                                        uint4 _2666 = asuint(_55_m0[0u]);
                                        uint _2673 = uint(float(_2666.x) * _1750);
                                        uint _2674 = uint(float(_2666.y) * _1752);
                                        uint4 _2677 = _24[2u].Load(int3(uint2(_2673, _2674), 0u));
                                        uint _2680 = _2677.w;
                                        uint4 _2685 = _24[15u].Load(int3(uint2(_2673, _2674), 0u));
                                        uint _2687 = _2685.y;
                                        uint _2693 = ((_2687 & 64u) != 0u) ? uint((_2687 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2694 = _2680 & 128u;
                                        uint _2696 = (_2694 != 0u) ? 1u : ((_2677.x << 7u) | _2680);
                                        uint4 _2699 = _16.Load(_2696 * 4u);
                                        uint _2700 = _2699.x;
                                        uint _2707 = ((_2700 & 1u) != 0u) ? 0u : 18u;
                                        uint _2777;
                                        if (_2694 == 0u)
                                        {
                                            _2777 = (((_2700 & 2097152u) != 0u) && (_2693 == uint(min(int(uint(max(int(_2693), int(0u)))), int(1u))))) ? (_2707 | 128u) : _2707;
                                        }
                                        else
                                        {
                                            _2777 = _2680;
                                        }
                                        float frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder;
                                        if (((_2777 & 128u) | (_16.Load((_2696 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2802 = asuint(_55_m0[0u]);
                                            uint _2811 = uint(float(_2802.x) * (_2072 + _1750));
                                            uint _2812 = uint(float(_2802.y) * (_2073 + _1752));
                                            uint4 _2815 = _24[2u].Load(int3(uint2(_2811, _2812), 0u));
                                            uint _2818 = _2815.w;
                                            uint4 _2821 = _24[15u].Load(int3(uint2(_2811, _2812), 0u));
                                            uint _2823 = _2821.y;
                                            uint _2829 = ((_2823 & 64u) != 0u) ? uint((_2823 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2830 = _2818 & 128u;
                                            uint _2832 = (_2830 != 0u) ? 1u : ((_2815.x << 7u) | _2818);
                                            uint4 _2834 = _16.Load(_2832 * 4u);
                                            uint _2835 = _2834.x;
                                            uint _2842 = ((_2835 & 1u) != 0u) ? 0u : 18u;
                                            uint _2911;
                                            if (_2830 == 0u)
                                            {
                                                _2911 = (((_2835 & 2097152u) != 0u) && (_2829 == uint(min(int(uint(max(int(_2829), int(0u)))), int(1u))))) ? (_2842 | 128u) : _2842;
                                            }
                                            else
                                            {
                                                _2911 = _2818;
                                            }
                                            float frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder_179_ladder;
                                            if ((_2911 & 128u) == 0u)
                                            {
                                                frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder_179_ladder = ((_16.Load((_2832 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder_179_ladder = 0.0f;
                                            }
                                            frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder = frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder_179_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder = 1.0f;
                                        }
                                        frontier_phi_123_122_ladder_139_ladder_152_ladder = frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_123_122_ladder_139_ladder_152_ladder = 1.0f;
                                    }
                                    frontier_phi_123_122_ladder_139_ladder = frontier_phi_123_122_ladder_139_ladder_152_ladder;
                                }
                                else
                                {
                                    frontier_phi_123_122_ladder_139_ladder = 1.0f;
                                }
                                frontier_phi_123_122_ladder = frontier_phi_123_122_ladder_139_ladder;
                            }
                            else
                            {
                                frontier_phi_123_122_ladder = 1.0f;
                            }
                            _2277 = frontier_phi_123_122_ladder;
                        }
                        else
                        {
                            _2277 = 1.0f;
                        }
                        float frontier_phi_125_123_ladder;
                        float frontier_phi_125_123_ladder_1;
                        float frontier_phi_125_123_ladder_2;
                        float frontier_phi_125_123_ladder_3;
                        float _2279;
                        for (;;)
                        {
                            _2279 = _2277 * _1754;
                            if (_1749 > 0.0f)
                            {
                                float _2294;
                                float _2296;
                                float _2298;
                                if (_1029)
                                {
                                    _2294 = 0.0f;
                                    _2296 = 0.0f;
                                    _2298 = 0.0f;
                                }
                                else
                                {
                                    if (!((_402 != 4u) || (_1043 != 0u)))
                                    {
                                        frontier_phi_125_123_ladder = _2104;
                                        frontier_phi_125_123_ladder_1 = _2103;
                                        frontier_phi_125_123_ladder_2 = _2102;
                                        frontier_phi_125_123_ladder_3 = _2279;
                                        break;
                                    }
                                    _2294 = _2102;
                                    _2296 = _2103;
                                    _2298 = _2104;
                                }
                                frontier_phi_125_123_ladder = _2298;
                                frontier_phi_125_123_ladder_1 = _2296;
                                frontier_phi_125_123_ladder_2 = _2294;
                                frontier_phi_125_123_ladder_3 = (1.0f - exp2(log2(_1749) * _50_m0[4u].x)) * _2279;
                                break;
                            }
                            else
                            {
                                frontier_phi_125_123_ladder = _2104;
                                frontier_phi_125_123_ladder_1 = _2103;
                                frontier_phi_125_123_ladder_2 = _2102;
                                frontier_phi_125_123_ladder_3 = _2279;
                                break;
                            }
                        }
                        _2290 = frontier_phi_125_123_ladder_3;
                        _2292 = frontier_phi_125_123_ladder_2;
                        _2295 = frontier_phi_125_123_ladder_1;
                        _2297 = frontier_phi_125_123_ladder;
                    }
                    else
                    {
                        float frontier_phi_125_110_ladder;
                        float frontier_phi_125_110_ladder_1;
                        float frontier_phi_125_110_ladder_2;
                        float frontier_phi_125_110_ladder_3;
                        if (_1748 == 0u)
                        {
                            float4 _2285 = _28[7u].SampleLevel(_59, float3(_1357, _1360, _1363), 0.0f);
                            frontier_phi_125_110_ladder = _2285.z;
                            frontier_phi_125_110_ladder_1 = _2285.y;
                            frontier_phi_125_110_ladder_2 = _2285.x;
                            frontier_phi_125_110_ladder_3 = _1754;
                        }
                        else
                        {
                            frontier_phi_125_110_ladder = _2293;
                            frontier_phi_125_110_ladder_1 = _2293;
                            frontier_phi_125_110_ladder_2 = _2293;
                            frontier_phi_125_110_ladder_3 = 0.0f;
                        }
                        _2290 = frontier_phi_125_110_ladder_3;
                        _2292 = frontier_phi_125_110_ladder_2;
                        _2295 = frontier_phi_125_110_ladder_1;
                        _2297 = frontier_phi_125_110_ladder;
                    }
                    float _2517;
                    float _2518;
                    float _2520;
                    float _2522;
                    if (_1425)
                    {
                        _2517 = 1.0f;
                        _2518 = ((_2292 - _1428) * _1220) + _1428;
                        _2520 = ((_2295 - _1430) * _1220) + _1430;
                        _2522 = ((_2297 - _1432) * _1220) + _1432;
                    }
                    else
                    {
                        float frontier_phi_155_142_ladder;
                        float frontier_phi_155_142_ladder_1;
                        float frontier_phi_155_142_ladder_2;
                        float frontier_phi_155_142_ladder_3;
                        if (_1756 && (_2290 < 1.0f))
                        {
                            float _2524 = _1360 * _1265;
                            float _2528 = rsqrt(dot(float3(_1357, _2524, _1363), float3(_1357, _2524, _1363)));
                            float4 _2536 = _28[4u].SampleLevel(_59, float3(_2528 * _1357, _2528 * _2524, _2528 * _1363), 0.0f);
                            float _2538 = _2536.x;
                            float _2539 = _2536.y;
                            float _2540 = _2536.z;
                            frontier_phi_155_142_ladder = ((_2297 - _2540) * _2290) + _2540;
                            frontier_phi_155_142_ladder_1 = 1.0f;
                            frontier_phi_155_142_ladder_2 = ((_2292 - _2538) * _2290) + _2538;
                            frontier_phi_155_142_ladder_3 = ((_2295 - _2539) * _2290) + _2539;
                        }
                        else
                        {
                            frontier_phi_155_142_ladder = _2297;
                            frontier_phi_155_142_ladder_1 = _2290;
                            frontier_phi_155_142_ladder_2 = _2292;
                            frontier_phi_155_142_ladder_3 = _2295;
                        }
                        _2517 = frontier_phi_155_142_ladder_1;
                        _2518 = frontier_phi_155_142_ladder_2;
                        _2520 = frontier_phi_155_142_ladder_3;
                        _2522 = frontier_phi_155_142_ladder;
                    }
                    float _2200 = _2517 * _481;
                    _2193 = _2518 * _2200;
                    _2195 = _2520 * _2200;
                    _2197 = _2522 * _2200;
                    _2199 = _2200;
                }
                float _2204 = _50_m0[58u].z * _1444;
                float _2226 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _2228 = _2226 * ((_2204 * ((_1440 * 1000.0f) - _2193)) + _2193);
                float _2229 = _2226 * ((_2204 * ((_1441 * 1000.0f) - _2195)) + _2195);
                float _2230 = _2226 * ((_2204 * (_1442 - _2197)) + _2197);
                float _2236 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2228, max(_2229, _2230)) + 1.0f);
                float _2240 = min(_2236 * _2228, 0.996078431606292724609375f);
                float _2242 = min(_2236 * _2229, 0.996078431606292724609375f);
                float _2243 = min(_2236 * _2230, 0.996078431606292724609375f);
                _35[uint2(_226, _229)] = float4(_2240, _2242, _2243, _2199);
                _39[uint2(_226, _229)] = float4(_1663, _406, _2199, _1663);
                if (_233)
                {
                    uint _2435 = _226 + 1u;
                    _35[uint2(_2435, _229)] = float4(_2240, _2242, _2243, _2199);
                    _39[uint2(_2435, _229)] = float4(_1663, _406, _2199, _1663);
                }
                if (_236)
                {
                    uint _2493 = _229 + 1u;
                    _35[uint2(_226, _2493)] = float4(_2240, _2242, _2243, _2199);
                    _39[uint2(_226, _2493)] = float4(_1663, _406, _2199, _1663);
                }
                if (_237)
                {
                    uint _2656 = _226 + 1u;
                    uint _2657 = _229 + 1u;
                    _35[uint2(_2656, _2657)] = float4(_2240, _2242, _2243, _2199);
                    _39[uint2(_2656, _2657)] = float4(_1663, _406, _2199, _1663);
                }
                ladder_phi_8 = true;
                frontier_phi_8_pred = _406;
                break;
            }
            float _426 = frontier_phi_8_pred;
            if (ladder_phi_8)
            {
                break;
            }
            _35[uint2(_226, _229)] = 0.0f.xxxx;
            _39[uint2(_226, _229)] = float4(0.0f, _426, 0.0f, 0.0f);
            if (_233)
            {
                uint _446 = _226 + 1u;
                _35[uint2(_446, _229)] = 0.0f.xxxx;
                _39[uint2(_446, _229)] = float4(0.0f, _426, 0.0f, 0.0f);
            }
            if (_236)
            {
                uint _557 = _229 + 1u;
                _35[uint2(_226, _557)] = 0.0f.xxxx;
                _39[uint2(_226, _557)] = float4(0.0f, _426, 0.0f, 0.0f);
            }
            if (!_237)
            {
                break;
            }
            uint _656 = _226 + 1u;
            uint _657 = _229 + 1u;
            _35[uint2(_656, _657)] = 0.0f.xxxx;
            _39[uint2(_656, _657)] = float4(0.0f, _426, 0.0f, 0.0f);
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
