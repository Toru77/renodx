static float _2292;
static uint _3374;
static float _3375;
static float _3376;
static float _3377;
static float _3378;
static float _3379;
static float _3380;
static float _3381;
static float _3382;
static float _3383;
static uint _3384;
static uint _3385;
static float _3386;
static float _3387;
static float _3388;
static float _3389;
static float _3390;
static float _3391;
static float _3397;
static uint _3398;
static float _3399;
static uint _3401;
static uint _3402;
static float _3407;

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

static RayQuery<RAY_FLAG_NONE> _2851;

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
                    uint _432;
                    uint _433;
                    if (_373 == 0u)
                    {
                        _432 = (((_381 & 2097152u) != 0u) && (_372 == _394)) ? (_392 | 128u) : _392;
                        _433 = _381;
                    }
                    else
                    {
                        _432 = _355;
                        _433 = _381 | ((_354 << 20u) & 134217728u);
                    }
                    uint _442 = _385 & 512u;
                    float _802;
                    float _804;
                    float _806;
                    float _808;
                    float _810;
                    float _812;
                    float _814;
                    float _816;
                    float _818;
                    float _820;
                    float _822;
                    float _824;
                    if (_442 == 0u)
                    {
                        if (!((_433 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _584 = asfloat(_17.Load((_389 * 115u) + 33u).x);
                        uint4 _592 = _24[2u].Load(int3(uint2(_270, _271), 0u));
                        uint _594 = _592.y;
                        uint _595 = _432 & 128u;
                        uint _790;
                        uint _791;
                        uint _792;
                        uint _793;
                        if (_595 == 0u)
                        {
                            _790 = uint(((_433 & 817889384u) | (_385 & 576u)) != 0u) | (((_433 >> 19u) & 1u) ^ 1u);
                            _791 = uint(((_433 & 17825808u) | (_385 & 520u)) != 0u);
                            _792 = uint(((_433 & 46137344u) | (_385 & 2564u)) != 0u);
                            _793 = 0u;
                        }
                        else
                        {
                            _790 = 1u;
                            _791 = _432 & 1u;
                            _792 = 1u;
                            _793 = 1u;
                        }
                        precise float _797 = float(_594 & 127u) * 0.0078740157186985015869140625f;
                        bool _801 = (_433 & 4194304u) == 0u;
                        float _1030;
                        if (_801)
                        {
                            _1030 = _797;
                        }
                        else
                        {
                            _1030 = float(_594 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1276;
                        if ((_433 & 134217728u) == 0u)
                        {
                            uint frontier_phi_48_40_ladder;
                            if ((_595 != 0u) || ((_433 & 17825792u) == 1048576u))
                            {
                                frontier_phi_48_40_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_48_40_ladder = _791;
                            }
                            _1276 = frontier_phi_48_40_ladder;
                        }
                        else
                        {
                            _1276 = _791;
                        }
                        uint4 _1279 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _1281 = _1279.x;
                        float _1394;
                        float _1396;
                        float _1398;
                        if (_790 == 0u)
                        {
                            _1394 = 0.0f;
                            _1396 = 0.0f;
                            _1398 = 0.0f;
                        }
                        else
                        {
                            float4 _1403 = _20[8u].Load(int3(uint2(_270, _271), 0u));
                            _1394 = _1403.x;
                            _1396 = _1403.y;
                            _1398 = _1403.z;
                        }
                        uint _1496;
                        if (_1276 == 0u)
                        {
                            _1496 = 0u;
                        }
                        else
                        {
                            _1496 = _24[9u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        uint _1693;
                        if (_792 == 0u)
                        {
                            _1693 = 0u;
                        }
                        else
                        {
                            _1693 = _24[10u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        float _1703 = (float((_1281 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1704 = (float(_1281 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1708 = (1.0f - abs(_1703)) - abs(_1704);
                        float _1710 = clamp((-0.0f) - _1708, 0.0f, 1.0f);
                        float _1711 = (-0.0f) - _1710;
                        float _1716 = ((_1703 >= 0.0f) ? _1711 : _1710) + _1703;
                        float _1717 = ((_1704 >= 0.0f) ? _1711 : _1710) + _1704;
                        float _1721 = rsqrt(dot(float3(_1716, _1717, _1708), float3(_1716, _1717, _1708)));
                        float _1722 = _1716 * _1721;
                        float _1723 = _1717 * _1721;
                        float _1724 = _1721 * _1708;
                        float _803 = float(_1281 & 255u);
                        float _1728 = ((_433 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _2017;
                        float _2018;
                        float _2019;
                        float _2020;
                        uint _2021;
                        if ((_385 & 64u) == 0u)
                        {
                            uint frontier_phi_102_88_ladder;
                            float frontier_phi_102_88_ladder_1;
                            float frontier_phi_102_88_ladder_2;
                            float frontier_phi_102_88_ladder_3;
                            float frontier_phi_102_88_ladder_4;
                            if ((_433 & 276824064u) == 0u)
                            {
                                frontier_phi_102_88_ladder = 0u;
                                frontier_phi_102_88_ladder_1 = 0.0f;
                                frontier_phi_102_88_ladder_2 = 0.0f;
                                frontier_phi_102_88_ladder_3 = 0.0f;
                                frontier_phi_102_88_ladder_4 = ((_433 & 8u) != 0u) ? _1396 : _1728;
                            }
                            else
                            {
                                frontier_phi_102_88_ladder = 0u;
                                frontier_phi_102_88_ladder_1 = 0.0f;
                                frontier_phi_102_88_ladder_2 = 0.0f;
                                frontier_phi_102_88_ladder_3 = 0.0f;
                                frontier_phi_102_88_ladder_4 = _1728;
                            }
                            _2017 = frontier_phi_102_88_ladder_4;
                            _2018 = frontier_phi_102_88_ladder_3;
                            _2019 = frontier_phi_102_88_ladder_2;
                            _2020 = frontier_phi_102_88_ladder_1;
                            _2021 = frontier_phi_102_88_ladder;
                        }
                        else
                        {
                            float _1827 = (_1396 * 2.0f) + (-1.0f);
                            float _1828 = (_1398 * 2.0f) + (-1.0f);
                            float _1832 = (1.0f - abs(_1827)) - abs(_1828);
                            float _1834 = clamp((-0.0f) - _1832, 0.0f, 1.0f);
                            float _1835 = (-0.0f) - _1834;
                            float _1840 = ((_1827 >= 0.0f) ? _1835 : _1834) + _1827;
                            float _1841 = ((_1828 >= 0.0f) ? _1835 : _1834) + _1828;
                            float _1845 = rsqrt(dot(float3(_1840, _1841, _1832), float3(_1840, _1841, _1832)));
                            _2017 = floor(round(_1394 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _2018 = _1840 * _1845;
                            _2019 = _1841 * _1845;
                            _2020 = _1845 * _1832;
                            _2021 = 1u;
                        }
                        float _811;
                        if ((_433 & 32768u) == 0u)
                        {
                            _811 = _2017;
                        }
                        else
                        {
                            float frontier_phi_115_116_ladder;
                            if (_17.Load((_389 * 115u) + 36u).x == 0u)
                            {
                                float _2369 = clamp((_803 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _584;
                                frontier_phi_115_116_ladder = ((_433 & 131072u) != 0u) ? _2369 : ((((clamp((1.21000003814697265625f / (exp2((_1030 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_389 * 115u) + 32u).x)) + 1.0f) * _2369);
                            }
                            else
                            {
                                frontier_phi_115_116_ladder = _584;
                            }
                            _811 = frontier_phi_115_116_ladder;
                        }
                        uint _2158 = _432 & 1u;
                        float _2312;
                        float _2314;
                        float _2316;
                        uint _2318;
                        if (((_433 & 16u) == 0u) || (((_2158 | (_385 & 8u)) | (_433 & 16777216u)) != 0u))
                        {
                            _2312 = _2018;
                            _2314 = _2019;
                            _2316 = _2020;
                            _2318 = _2021;
                        }
                        else
                        {
                            float _2328 = (float(_1496 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2329 = (float(_1496 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2333 = (1.0f - abs(_2328)) - abs(_2329);
                            float _2335 = clamp((-0.0f) - _2333, 0.0f, 1.0f);
                            float _2336 = (-0.0f) - _2335;
                            float _2341 = ((_2328 >= 0.0f) ? _2336 : _2335) + _2328;
                            float _2342 = ((_2329 >= 0.0f) ? _2336 : _2335) + _2329;
                            float _2346 = rsqrt(dot(float3(_2341, _2342, _2333), float3(_2341, _2342, _2333)));
                            _2312 = _2341 * _2346;
                            _2314 = _2342 * _2346;
                            _2316 = _2346 * _2333;
                            _2318 = 1u;
                        }
                        float _805;
                        float _807;
                        float _809;
                        if (_2158 == 0u)
                        {
                            float frontier_phi_146_145_ladder;
                            float frontier_phi_146_145_ladder_1;
                            float frontier_phi_146_145_ladder_2;
                            if (((_432 & 64u) == 0u) && (_793 != 0u))
                            {
                                float2 _2559 = spvUnpackHalf2x16((_1693 >> 17u) & 32736u);
                                float _2560 = _2559.x;
                                float _2563 = (spvUnpackHalf2x16((_1693 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2564 = (spvUnpackHalf2x16((_1693 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2568 = (1.0f - abs(_2563)) - abs(_2564);
                                float _2570 = clamp((-0.0f) - _2568, 0.0f, 1.0f);
                                float _2571 = (-0.0f) - _2570;
                                float _2576 = ((_2563 >= 0.0f) ? _2571 : _2570) + _2563;
                                float _2577 = ((_2564 >= 0.0f) ? _2571 : _2570) + _2564;
                                float _2581 = rsqrt(dot(float3(_2576, _2577, _2568), float3(_2576, _2577, _2568)));
                                float _2591 = (((_2576 * _2581) - _1722) * _2560) + _1722;
                                float _2592 = (((_2577 * _2581) - _1723) * _2560) + _1723;
                                float _2593 = (((_2581 * _2568) - _1724) * _2560) + _1724;
                                float _2597 = rsqrt(dot(float3(_2591, _2592, _2593), float3(_2591, _2592, _2593)));
                                frontier_phi_146_145_ladder = _2593 * _2597;
                                frontier_phi_146_145_ladder_1 = _2592 * _2597;
                                frontier_phi_146_145_ladder_2 = _2591 * _2597;
                            }
                            else
                            {
                                frontier_phi_146_145_ladder = _1724;
                                frontier_phi_146_145_ladder_1 = _1723;
                                frontier_phi_146_145_ladder_2 = _1722;
                            }
                            _805 = frontier_phi_146_145_ladder_2;
                            _807 = frontier_phi_146_145_ladder_1;
                            _809 = frontier_phi_146_145_ladder;
                        }
                        else
                        {
                            _805 = _1722;
                            _807 = _1723;
                            _809 = _1724;
                        }
                        float _819;
                        float _821;
                        float _823;
                        float _825;
                        if (_801)
                        {
                            float frontier_phi_160_159_ladder;
                            float frontier_phi_160_159_ladder_1;
                            float frontier_phi_160_159_ladder_2;
                            float frontier_phi_160_159_ladder_3;
                            if (((_433 & 33554432u) == 0u) || (((_385 & 4u) != 0u) && ((_433 & 8388608u) == 0u)))
                            {
                                frontier_phi_160_159_ladder = 0.0f;
                                frontier_phi_160_159_ladder_1 = 0.0f;
                                frontier_phi_160_159_ladder_2 = 0.0f;
                                frontier_phi_160_159_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2721 = (spvUnpackHalf2x16((_1693 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2722 = (spvUnpackHalf2x16((_1693 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2726 = (1.0f - abs(_2721)) - abs(_2722);
                                float _2728 = clamp((-0.0f) - _2726, 0.0f, 1.0f);
                                float _2729 = (-0.0f) - _2728;
                                float _2734 = ((_2721 >= 0.0f) ? _2729 : _2728) + _2721;
                                float _2735 = ((_2722 >= 0.0f) ? _2729 : _2728) + _2722;
                                float _2739 = rsqrt(dot(float3(_2734, _2735, _2726), float3(_2734, _2735, _2726)));
                                float _2740 = _2734 * _2739;
                                float _2741 = _2735 * _2739;
                                float _2742 = _2739 * _2726;
                                float _2746 = rsqrt(dot(float3(_2740, _2741, _2742), float3(_2740, _2741, _2742)));
                                frontier_phi_160_159_ladder = _2746 * _2742;
                                frontier_phi_160_159_ladder_1 = _2746 * _2741;
                                frontier_phi_160_159_ladder_2 = _2746 * _2740;
                                frontier_phi_160_159_ladder_3 = spvUnpackHalf2x16((_1693 >> 17u) & 32736u).x;
                            }
                            _819 = frontier_phi_160_159_ladder_3;
                            _821 = frontier_phi_160_159_ladder_2;
                            _823 = frontier_phi_160_159_ladder_1;
                            _825 = frontier_phi_160_159_ladder;
                        }
                        else
                        {
                            _819 = 0.0f;
                            _821 = 0.0f;
                            _823 = 0.0f;
                            _825 = 0.0f;
                        }
                        bool _2611 = _2318 != 0u;
                        _802 = _803;
                        _804 = _805;
                        _806 = _807;
                        _808 = _809;
                        _810 = _811;
                        _812 = _2611 ? _2312 : _805;
                        _814 = _2611 ? _2314 : _807;
                        _816 = _2611 ? _2316 : _809;
                        _818 = _819;
                        _820 = _821;
                        _822 = _823;
                        _824 = _825;
                    }
                    else
                    {
                        uint4 _544 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _546 = _544.x;
                        uint4 _550 = _24[9u].Load(int3(uint2(_270, _271), 0u));
                        uint _552 = _550.x;
                        float _726;
                        float _727;
                        float _728;
                        if ((_433 & 33554432u) == 0u)
                        {
                            float _604 = (float((_546 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _605 = (float(_546 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _609 = (1.0f - abs(_604)) - abs(_605);
                            float _611 = clamp((-0.0f) - _609, 0.0f, 1.0f);
                            float _612 = (-0.0f) - _611;
                            float _617 = ((_604 >= 0.0f) ? _612 : _611) + _604;
                            float _618 = ((_605 >= 0.0f) ? _612 : _611) + _605;
                            float _622 = rsqrt(dot(float3(_617, _618, _609), float3(_617, _618, _609)));
                            _726 = _617 * _622;
                            _727 = _618 * _622;
                            _728 = _622 * _609;
                        }
                        else
                        {
                            float _633 = (float((_552 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _634 = (float(_552 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _638 = (1.0f - abs(_633)) - abs(_634);
                            float _640 = clamp((-0.0f) - _638, 0.0f, 1.0f);
                            float _641 = (-0.0f) - _640;
                            float _646 = ((_633 >= 0.0f) ? _641 : _640) + _633;
                            float _647 = ((_634 >= 0.0f) ? _641 : _640) + _634;
                            float _651 = rsqrt(dot(float3(_646, _647, _638), float3(_646, _647, _638)));
                            _726 = _646 * _651;
                            _727 = _647 * _651;
                            _728 = _651 * _638;
                        }
                        _802 = float(_546 & 255u);
                        _804 = _726;
                        _806 = _727;
                        _808 = _728;
                        _810 = 1.0f;
                        _812 = _726;
                        _814 = _727;
                        _816 = _728;
                        _818 = 0.0f;
                        _820 = 0.0f;
                        _822 = 0.0f;
                        _824 = 0.0f;
                    }
                    precise float _826 = _802 * 0.0039215688593685626983642578125f;
                    if ((_432 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1039 = ((_432 & 128u) | _442) != 0u;
                    uint _1211;
                    if (_1039)
                    {
                        _1211 = 1u;
                    }
                    else
                    {
                        _1211 = (((_433 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _397;
                    float _399;
                    float _401;
                    uint _403;
                    float _407;
                    if (((_433 & 33554432u) == 0u) || _1039)
                    {
                        bool _1283 = _77 != 0u;
                        uint _1290;
                        if ((_433 & 16u) == 0u)
                        {
                            _1290 = _1211;
                        }
                        else
                        {
                            _1290 = ((_433 & 268435456u) != 0u) ? _1211 : 2u;
                        }
                        _407 = _810 * _826;
                        _397 = _1283 ? _812 : _804;
                        _399 = _1283 ? _814 : _806;
                        _401 = _1283 ? _816 : _808;
                        _403 = _1290;
                    }
                    else
                    {
                        _407 = _818;
                        _397 = _820;
                        _399 = _822;
                        _401 = _824;
                        _403 = _1211;
                    }
                    uint _1291 = _403 + 102u;
                    float _1300 = clamp((_407 - _45_m0[_1291].x) / (_45_m0[_1291].y - _45_m0[_1291].x), 0.0f, 1.0f);
                    _396 = _397;
                    _398 = _399;
                    _400 = _401;
                    _402 = _403;
                    _404 = (_1300 * _1300) * (3.0f - (_1300 * 2.0f));
                    _406 = _407;
                    _408 = _45_m0[_1291].z;
                    _410 = (_403 == 1u) ? _394 : 0u;
                    _412 = asfloat(_17.Load((_389 * 115u) + 114u).x);
                }
                if (_404 == 0.0f)
                {
                    ladder_phi_8 = false;
                    break;
                }
                float _452;
                if (_268)
                {
                    _452 = _267;
                }
                else
                {
                    _452 = _12.Load(int3(uint2(uint(int(_259 * float(_238))), uint(int(_260 * float(_239)))), 0u)).x;
                }
                float _454 = 1.0f - _406;
                float _455 = _454 * _454;
                float _463 = _50_m0[50u].w + _50_m0[50u].y;
                uint _466 = _402 + 63u;
                float _475 = clamp(((_50_m0[50u].x / (_463 - (_50_m0[50u].y * _452))) - _50_m0[_466].y) / (_50_m0[_466].x - _50_m0[_466].y), 0.0f, 1.0f);
                float _480 = ((_475 * _475) * _404) * (3.0f - (_475 * 2.0f));
                float _491 = ((_259 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _492 = ((1.0f - (_50_m0[51u].w * _260)) * 2.0f) + (-1.0f);
                float _508 = mad(_144, _452, mad(_137, _492, _491 * _130)) + _151;
                float _509 = (mad(_141, _452, mad(_134, _492, _491 * _127)) + _148) / _508;
                float _510 = (mad(_142, _452, mad(_135, _492, _491 * _128)) + _149) / _508;
                float _511 = (mad(_143, _452, mad(_136, _492, _491 * _129)) + _150) / _508;
                float _515 = rsqrt(dot(float3(_509, _510, _511), float3(_509, _510, _511)));
                float _516 = _515 * _509;
                float _517 = _515 * _510;
                float _518 = _515 * _511;
                float _521 = mad(_93, _400, mad(_87, _398, _396 * _81));
                float _524 = mad(_94, _400, mad(_88, _398, _396 * _82));
                float _527 = mad(_95, _400, mad(_89, _398, _396 * _83));
                float _530 = _524 * _524;
                float _663;
                float _664;
                float _665;
                if (abs(_527) > 0.0f)
                {
                    float _565 = sqrt((_527 * _527) + _530);
                    _663 = 0.0f;
                    _664 = ((-0.0f) - _527) / _565;
                    _665 = _524 / _565;
                }
                else
                {
                    float _571 = sqrt(_530 + (_521 * _521));
                    _663 = _524 / _571;
                    _664 = ((-0.0f) - _521) / _571;
                    _665 = 0.0f;
                }
                float _668 = (_665 * _524) - (_664 * _527);
                float _671 = (_663 * _527) - (_665 * _521);
                float _674 = (_664 * _521) - (_663 * _524);
                float _675 = (-0.0f) - _516;
                float _676 = (-0.0f) - _517;
                float _677 = (-0.0f) - _518;
                float _686 = mad(_677, _527, mad(_676, _524, _521 * _675));
                float _687 = mad(_677, _665, mad(_676, _664, _663 * _675)) * _455;
                float _688 = mad(_677, _674, mad(_676, _671, _668 * _675)) * _455;
                float _692 = rsqrt(dot(float3(_687, _688, _686), float3(_687, _688, _686)));
                float _693 = _692 * _687;
                float _694 = _692 * _688;
                float _695 = _692 * _686;
                float _698 = (_693 * _693) + (_694 * _694);
                bool _699 = _698 > 0.0f;
                float _735;
                float _736;
                if (_699)
                {
                    float _731 = rsqrt(_698);
                    _735 = (-0.0f) - (_694 * _731);
                    _736 = _731 * _693;
                }
                else
                {
                    _735 = 1.0f;
                    _736 = 0.0f;
                }
                float _741 = (_695 + 1.0f) * 0.5f;
                float _742 = 1.0f - _741;
                float _743 = _742 * _695;
                float _750 = sqrt(max(0.0f, 1.0f - (_742 * _742)));
                float _757 = ((_750 * _693) - (_736 * _743)) * _455;
                float _758 = ((_750 * _694) + (_735 * _743)) * _455;
                float _759 = max(0.0f, (((_736 * _693) - (_735 * _694)) * _742) + (_750 * _695));
                float _763 = rsqrt(dot(float3(_757, _758, _759), float3(_757, _758, _759)));
                float _764 = _757 * _763;
                float _765 = _758 * _763;
                float _766 = _763 * _759;
                float _769 = mad(_766, _521, mad(_765, _668, _764 * _663));
                float _772 = mad(_766, _524, mad(_765, _671, _764 * _664));
                float _775 = mad(_766, _527, mad(_765, _674, _764 * _665));
                float _779 = dot(float3(_516, _517, _518), float3(_769, _772, _775)) * 2.0f;
                float _783 = _516 - (_779 * _769);
                float _784 = _517 - (_779 * _772);
                float _785 = _518 - (_779 * _775);
                float _834;
                float _835;
                if (_699)
                {
                    float _830 = rsqrt(_698);
                    _834 = (-0.0f) - (_694 * _830);
                    _835 = _830 * _693;
                }
                else
                {
                    _834 = 1.0f;
                    _835 = 0.0f;
                }
                float _839 = sqrt(_50_m0[1u].x);
                float _840 = _50_m0[1u].y * 1.57079637050628662109375f;
                float _843 = cos(_840) * _839;
                float _846 = 1.0f - (_843 * _843);
                float _851 = (sqrt(_846) * _742) + ((_839 * _741) * sin(_840));
                float _854 = _851 * _695;
                float _861 = sqrt(max(0.0f, _846 - (_851 * _851)));
                float _870 = (((_861 * _693) + (_843 * _834)) - (_854 * _835)) * _455;
                float _871 = (((_861 * _694) + (_843 * _835)) + (_854 * _834)) * _455;
                float _872 = max(0.0f, (_851 * ((_835 * _693) - (_834 * _694))) + (_861 * _695));
                float _876 = rsqrt(dot(float3(_870, _871, _872), float3(_870, _871, _872)));
                float _877 = _870 * _876;
                float _878 = _871 * _876;
                float _879 = _876 * _872;
                float _882 = mad(_879, _521, mad(_878, _668, _877 * _663));
                float _885 = mad(_879, _524, mad(_878, _671, _877 * _664));
                float _888 = mad(_879, _527, mad(_878, _674, _877 * _665));
                float _892 = dot(float3(_516, _517, _518), float3(_882, _885, _888)) * 2.0f;
                float _896 = _516 - (_892 * _882);
                float _897 = _517 - (_892 * _885);
                float _898 = _518 - (_892 * _888);
                float _899 = dot(float3(_896, _897, _898), float3(_783, _784, _785));
                float _912 = (_50_m0[1u].z * (_521 - _769)) + _769;
                float _913 = (_50_m0[1u].z * (_524 - _772)) + _772;
                float _914 = (_50_m0[1u].z * (_527 - _775)) + _775;
                float _918 = rsqrt(dot(float3(_912, _913, _914), float3(_912, _913, _914)));
                float _919 = _918 * _912;
                float _920 = _918 * _913;
                float _921 = _918 * _914;
                float _925 = dot(float3(_516, _517, _518), float3(_919, _920, _921)) * 2.0f;
                float _929 = _516 - (_925 * _919);
                float _930 = _517 - (_925 * _920);
                float _931 = _518 - (_925 * _921);
                bool _935 = dot(float3(_929, _930, _931), float3(_516, _517, _518)) < 0.0f;
                float _942 = sqrt(((_510 * _510) + (_509 * _509)) + (_511 * _511)) * 0.001000000047497451305389404296875f;
                float _953 = ((_942 * _521) + _509) + (_929 * _408);
                float _954 = ((_942 * _524) + _510) + (_930 * _408);
                float _955 = ((_942 * _527) + _511) + (_931 * _408);
                float _971 = mad(_116, _955, mad(_109, _954, _953 * _102)) + _123;
                float _974 = (mad(_115, _955, mad(_108, _954, _953 * _101)) + _122) / _971;
                float _977 = (((mad(_113, _955, mad(_106, _954, _953 * _99)) + _120) / _971) * 0.5f) + 0.5f;
                float _978 = 0.5f - (((mad(_114, _955, mad(_107, _954, _953 * _100)) + _121) / _971) * 0.5f);
                float _981 = _977 * _50_m0[51u].x;
                float _982 = _978 * _50_m0[51u].y;
                float _987 = _953 + (_929 * 0.100000001490116119384765625f);
                float _988 = _954 + (_930 * 0.100000001490116119384765625f);
                float _989 = _955 + (_931 * 0.100000001490116119384765625f);
                float _1005 = mad(_116, _989, mad(_109, _988, _987 * _102)) + _123;
                float _1014 = _50_m0[51u].x * (((((mad(_113, _989, mad(_106, _988, _987 * _99)) + _120) / _1005) * 0.5f) + 0.5f) - _977);
                float _1016 = _50_m0[51u].y * ((0.5f - (((mad(_114, _989, mad(_107, _988, _987 * _100)) + _121) / _1005) * 0.5f)) - _978);
                float _1017 = ((mad(_115, _989, mad(_108, _988, _987 * _101)) + _122) / _1005) - _974;
                float _1018 = _1014 * 10.0f;
                float _1020 = _1016 * 10.0f;
                float _1021 = _1017 * 10.0f;
                bool _1028 = _402 == 1u;
                uint _1040;
                uint _1042;
                float _1044;
                float _1046;
                float _1048;
                float _1050;
                float _1052;
                float _1054;
                float _1056;
                float _1058;
                float _1060;
                uint _1062;
                uint _1064;
                float _1066;
                float _1068;
                float _1070;
                if (_1028 && (asuint(_50_m0[62u]).w != 0u))
                {
                    _1040 = 0u;
                    _1042 = 1u;
                    _1044 = 0.0f;
                    _1046 = 0.0f;
                    _1048 = 1.0f;
                    _1050 = 0.0f;
                    _1052 = 0.0f;
                    _1054 = 0.0f;
                    _1056 = 0.0f;
                    _1058 = 0.0f;
                    _1060 = 0.0f;
                    _1062 = 0u;
                    _1064 = 0u;
                    _1066 = 0.0f;
                    _1068 = 0.0f;
                    _1070 = 0.0f;
                }
                else
                {
                    float _1131 = float(_238);
                    float _1132 = float(_239);
                    float _1139 = (_1018 != 0.0f) ? (0.100000001490116119384765625f / _1014) : 3.4028234663852885981170418348452e+38f;
                    float _1141 = (_1020 != 0.0f) ? (0.100000001490116119384765625f / _1016) : 3.4028234663852885981170418348452e+38f;
                    float _1142 = (_1021 != 0.0f) ? (0.100000001490116119384765625f / _1017) : 3.4028234663852885981170418348452e+38f;
                    float _1143 = 1.0f / _1131;
                    float _1144 = 1.0f / _1132;
                    float _1145 = 0.004999999888241291046142578125f / _1131;
                    float _1147 = 0.004999999888241291046142578125f / _1132;
                    float _1156 = float(_1018 >= 0.0f);
                    float _1157 = float(_1020 >= 0.0f);
                    float _1166 = ((_1018 < 0.0f) ? ((-0.0f) - _1145) : _1145) - _981;
                    float _1169 = ((_1020 < 0.0f) ? ((-0.0f) - _1147) : _1147) - _982;
                    float _1172 = min((((floor(_981 * _1131) + _1156) * _1143) + _1166) * _1139, (((floor(_982 * _1132) + _1157) * _1144) + _1169) * _1141);
                    float _1176 = (_1172 * _1018) + _981;
                    float _1177 = (_1172 * _1020) + _982;
                    float _1178 = (_1172 * _1021) + _974;
                    float _1181 = _50_m0[50u].x / (_463 - (_1178 * _50_m0[50u].y));
                    float _1191 = 1.0f / max(0.00999999977648258209228515625f, max(abs(_1014 * 38400.0f), abs(_1016 * 21600.0f)));
                    float _1199 = max(_50_m0[3u].z, _50_m0[5u].z * _1191);
                    float _1271;
                    if (asuint(_50_m0[5u]).y == 0u)
                    {
                        _1271 = _1199;
                    }
                    else
                    {
                        _1271 = min(_1199, _50_m0[5u].w * _1191);
                    }
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
                    float _1067;
                    float _1069;
                    float _1071;
                    uint _1375;
                    uint _1380;
                    if (_209 == 0u)
                    {
                        _1375 = 0u;
                        _1071 = _1178;
                        _1069 = _1177;
                        _1067 = _1176;
                        _1380 = 0u;
                        _1061 = _974;
                        _1059 = _982;
                        _1057 = _981;
                        _1055 = _1178;
                        _1053 = _1177;
                        _1051 = _1176;
                        _1049 = 1.0f;
                        _1047 = 0.0f;
                        _1045 = 0.0f;
                        _1043 = 1u;
                    }
                    else
                    {
                        uint _1376;
                        float _1377;
                        float _1378;
                        float _1379;
                        uint _1381;
                        uint _1479;
                        float _1382;
                        float _1383;
                        float _1384;
                        float _1385;
                        float _1386;
                        float _1387;
                        float _1388;
                        float _1389;
                        float _1390;
                        uint _1391;
                        float _1464;
                        float _1469;
                        float _1471;
                        float _1473;
                        float _1475;
                        float _1477;
                        uint _1462 = 0u;
                        float _1463 = _1271;
                        float _1465 = _1178;
                        float _1466 = _1177;
                        float _1467 = _1176;
                        float _1468 = _1172;
                        float _1470 = _1144;
                        float _1472 = _1143;
                        float _1474 = _1132;
                        float _1476 = _1131;
                        uint _1478 = 0u;
                        uint _1480 = 0u;
                        float _1481 = _974;
                        float _1482 = _982;
                        float _1483 = _981;
                        float _1484 = _1178;
                        float _1485 = _1177;
                        float _1486 = _1176;
                        float _1487 = 1.0f;
                        float _1488 = 0.0f;
                        float _1489 = 0.0f;
                        uint _1490 = 1u;
                        float _1491;
                        float _1492;
                        uint _1493;
                        uint _1494;
                        bool _1495;
                        for (;;)
                        {
                            _1491 = _1476 * _1467;
                            _1492 = _1474 * _1466;
                            _1493 = uint(int(_1491));
                            _1494 = uint(int(_1492));
                            _1495 = _1478 == 0u;
                            float _1770;
                            if (_1495)
                            {
                                _1770 = _12.Load(int3(uint2(_1493, _1494), 0u)).x;
                            }
                            else
                            {
                                _1770 = _15.Load(int3(uint2(_1493, _1494), _1478 + 4294967295u)).x;
                            }
                            float _1776 = ((_1491 >= floor(_1476)) || (_1492 >= floor(_1474))) ? 1.0f : _1770;
                            float _1790 = (_1021 < 0.0f) ? ((_1776 - _974) * _1142) : 3.4028234663852885981170418348452e+38f;
                            float _1792 = min(min((((floor(_1491) + _1156) * _1472) + _1166) * _1139, (((floor(_1492) + _1157) * _1470) + _1169) * _1141), _1790);
                            bool _1793 = _1776 < _1465;
                            bool _1797 = _1793 && (asuint(_1792) != asuint(_1790));
                            float _1798 = _1793 ? _1792 : _1468;
                            float _1802 = (_1798 * _1018) + _981;
                            float _1803 = (_1798 * _1020) + _982;
                            float _1804 = (_1798 * _1021) + _974;
                            uint _1806 = (_1797 ? 1u : 4294967295u) + _1478;
                            float _1807 = _1797 ? 0.5f : 2.0f;
                            float _1808 = _1807 * _1476;
                            float _1809 = _1807 * _1474;
                            float _1810 = _1797 ? 2.0f : 0.5f;
                            float _1811 = _1810 * _1472;
                            float _1812 = _1810 * _1470;
                            _1376 = _1462 + 1u;
                            uint _2148;
                            uint _2150;
                            if (int(_1806) < int(0u))
                            {
                                float _1989 = _50_m0[50u].w + _50_m0[50u].y;
                                float _1991 = _50_m0[50u].x / (_1989 - (_50_m0[50u].y * _1776));
                                float _1994 = _50_m0[50u].x / (_1989 - (_50_m0[50u].y * _1804));
                                float _1999 = abs(_1181 - _1994);
                                float _2002 = _1994 - _1991;
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
                                if (_2002 > max(_50_m0[3u].x, _50_m0[3u].x * _1999))
                                {
                                    float _2119 = _1798 + _1463;
                                    bool _2120 = _1480 != 0u;
                                    float _2121 = _2120 ? _1489 : _1802;
                                    float _2122 = _2120 ? _1488 : _1803;
                                    uint _2129 = (_1028 || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1480;
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
                                        frontier_phi_114_99_ladder_112_ladder = _1483;
                                        frontier_phi_114_99_ladder_112_ladder_1 = _1463;
                                        frontier_phi_114_99_ladder_112_ladder_2 = _2119;
                                        frontier_phi_114_99_ladder_112_ladder_3 = _1144;
                                        frontier_phi_114_99_ladder_112_ladder_4 = _1143;
                                        frontier_phi_114_99_ladder_112_ladder_5 = _1132;
                                        frontier_phi_114_99_ladder_112_ladder_6 = _1131;
                                        frontier_phi_114_99_ladder_112_ladder_7 = 0u;
                                        frontier_phi_114_99_ladder_112_ladder_8 = _2129;
                                        frontier_phi_114_99_ladder_112_ladder_9 = _1481;
                                        frontier_phi_114_99_ladder_112_ladder_10 = _1482;
                                        frontier_phi_114_99_ladder_112_ladder_11 = _1484;
                                        frontier_phi_114_99_ladder_112_ladder_12 = _1485;
                                        frontier_phi_114_99_ladder_112_ladder_13 = _1486;
                                        frontier_phi_114_99_ladder_112_ladder_14 = _1487;
                                        frontier_phi_114_99_ladder_112_ladder_15 = _2122;
                                        frontier_phi_114_99_ladder_112_ladder_16 = _2121;
                                        frontier_phi_114_99_ladder_112_ladder_17 = _1490;
                                    }
                                    else
                                    {
                                        frontier_phi_114_99_ladder_112_ladder = _1483;
                                        frontier_phi_114_99_ladder_112_ladder_1 = min(_1199, _50_m0[6u].x * _1463);
                                        frontier_phi_114_99_ladder_112_ladder_2 = _2119;
                                        frontier_phi_114_99_ladder_112_ladder_3 = _1144;
                                        frontier_phi_114_99_ladder_112_ladder_4 = _1143;
                                        frontier_phi_114_99_ladder_112_ladder_5 = _1132;
                                        frontier_phi_114_99_ladder_112_ladder_6 = _1131;
                                        frontier_phi_114_99_ladder_112_ladder_7 = 0u;
                                        frontier_phi_114_99_ladder_112_ladder_8 = _2129;
                                        frontier_phi_114_99_ladder_112_ladder_9 = _1481;
                                        frontier_phi_114_99_ladder_112_ladder_10 = _1482;
                                        frontier_phi_114_99_ladder_112_ladder_11 = _1484;
                                        frontier_phi_114_99_ladder_112_ladder_12 = _1485;
                                        frontier_phi_114_99_ladder_112_ladder_13 = _1486;
                                        frontier_phi_114_99_ladder_112_ladder_14 = _1487;
                                        frontier_phi_114_99_ladder_112_ladder_15 = _2122;
                                        frontier_phi_114_99_ladder_112_ladder_16 = _2121;
                                        frontier_phi_114_99_ladder_112_ladder_17 = _1490;
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
                                    float _2137 = max(_50_m0[3u].y, _50_m0[3u].y * _1999);
                                    float _2140 = _2137 * _50_m0[3u].w;
                                    float _2144 = clamp((abs(_2002) - _2140) / (_2137 - _2140), 0.0f, 1.0f);
                                    uint _2146 = uint(_1991 < _1181);
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
                                    if (_1480 == 0u)
                                    {
                                        frontier_phi_114_99_ladder_113_ladder = _1483;
                                        frontier_phi_114_99_ladder_113_ladder_1 = _1463;
                                        frontier_phi_114_99_ladder_113_ladder_2 = _1798;
                                        frontier_phi_114_99_ladder_113_ladder_3 = _1812;
                                        frontier_phi_114_99_ladder_113_ladder_4 = _1811;
                                        frontier_phi_114_99_ladder_113_ladder_5 = _1809;
                                        frontier_phi_114_99_ladder_113_ladder_6 = _1808;
                                        frontier_phi_114_99_ladder_113_ladder_7 = _1806;
                                        frontier_phi_114_99_ladder_113_ladder_8 = uint(_2144 > 0.0f);
                                        frontier_phi_114_99_ladder_113_ladder_9 = _1481;
                                        frontier_phi_114_99_ladder_113_ladder_10 = _1482;
                                        frontier_phi_114_99_ladder_113_ladder_11 = _1484;
                                        frontier_phi_114_99_ladder_113_ladder_12 = _1485;
                                        frontier_phi_114_99_ladder_113_ladder_13 = _1486;
                                        frontier_phi_114_99_ladder_113_ladder_14 = _2144;
                                        frontier_phi_114_99_ladder_113_ladder_15 = _1488;
                                        frontier_phi_114_99_ladder_113_ladder_16 = _1489;
                                        frontier_phi_114_99_ladder_113_ladder_17 = _2146;
                                    }
                                    else
                                    {
                                        frontier_phi_114_99_ladder_113_ladder = _1483;
                                        frontier_phi_114_99_ladder_113_ladder_1 = _1463;
                                        frontier_phi_114_99_ladder_113_ladder_2 = _1798;
                                        frontier_phi_114_99_ladder_113_ladder_3 = _1812;
                                        frontier_phi_114_99_ladder_113_ladder_4 = _1811;
                                        frontier_phi_114_99_ladder_113_ladder_5 = _1809;
                                        frontier_phi_114_99_ladder_113_ladder_6 = _1808;
                                        frontier_phi_114_99_ladder_113_ladder_7 = _1806;
                                        frontier_phi_114_99_ladder_113_ladder_8 = _1480;
                                        frontier_phi_114_99_ladder_113_ladder_9 = _1481;
                                        frontier_phi_114_99_ladder_113_ladder_10 = _1482;
                                        frontier_phi_114_99_ladder_113_ladder_11 = _1484;
                                        frontier_phi_114_99_ladder_113_ladder_12 = _1485;
                                        frontier_phi_114_99_ladder_113_ladder_13 = _1486;
                                        frontier_phi_114_99_ladder_113_ladder_14 = _2144;
                                        frontier_phi_114_99_ladder_113_ladder_15 = _1488;
                                        frontier_phi_114_99_ladder_113_ladder_16 = _1489;
                                        frontier_phi_114_99_ladder_113_ladder_17 = _2146;
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
                                _1391 = frontier_phi_114_99_ladder_17;
                                _1390 = frontier_phi_114_99_ladder_16;
                                _1389 = frontier_phi_114_99_ladder_15;
                                _1388 = frontier_phi_114_99_ladder_14;
                                _1387 = frontier_phi_114_99_ladder_13;
                                _1386 = frontier_phi_114_99_ladder_12;
                                _1385 = frontier_phi_114_99_ladder_11;
                                _1384 = frontier_phi_114_99_ladder;
                                _1383 = frontier_phi_114_99_ladder_10;
                                _1382 = frontier_phi_114_99_ladder_9;
                                _2148 = frontier_phi_114_99_ladder_8;
                                _2150 = frontier_phi_114_99_ladder_7;
                                _1477 = frontier_phi_114_99_ladder_6;
                                _1475 = frontier_phi_114_99_ladder_5;
                                _1473 = frontier_phi_114_99_ladder_4;
                                _1471 = frontier_phi_114_99_ladder_3;
                                _1469 = frontier_phi_114_99_ladder_2;
                                _1464 = frontier_phi_114_99_ladder_1;
                            }
                            else
                            {
                                bool _2004 = _1480 != 0u;
                                _1391 = _1490;
                                _1390 = _1489;
                                _1389 = _1488;
                                _1388 = _1487;
                                _1387 = _2004 ? _1486 : _1802;
                                _1386 = _2004 ? _1485 : _1803;
                                _1385 = _2004 ? _1484 : _1804;
                                _1384 = _1802;
                                _1383 = _1803;
                                _1382 = _1804;
                                _2148 = _1480;
                                _2150 = _1806;
                                _1477 = _1808;
                                _1475 = _1809;
                                _1473 = _1811;
                                _1471 = _1812;
                                _1469 = _1798;
                                _1464 = (asuint(_50_m0[5u]).y != 0u) ? _1271 : _1463;
                            }
                            float frontier_phi_144_pred;
                            uint frontier_phi_144_pred_1;
                            uint frontier_phi_144_pred_2;
                            float frontier_phi_144_pred_3;
                            float frontier_phi_144_pred_4;
                            bool _2154;
                            bool _2156;
                            for (;;)
                            {
                                _2154 = _1804 < 0.0f;
                                _2156 = _2154 || ((_1802 < 0.0f) || (_1803 < 0.0f));
                                if (!_2156)
                                {
                                    if (!((_1804 > 1.0f) || ((_1802 > _50_m0[51u].x) || (_1803 > _50_m0[51u].y))))
                                    {
                                        frontier_phi_144_pred = _1802;
                                        frontier_phi_144_pred_1 = _2148;
                                        frontier_phi_144_pred_2 = _2150;
                                        frontier_phi_144_pred_3 = _1803;
                                        frontier_phi_144_pred_4 = _1804;
                                        break;
                                    }
                                }
                                if (!_2154)
                                {
                                    frontier_phi_144_pred = _1802;
                                    frontier_phi_144_pred_1 = 1u;
                                    frontier_phi_144_pred_2 = 4294967295u;
                                    frontier_phi_144_pred_3 = _1803;
                                    frontier_phi_144_pred_4 = _1804;
                                    break;
                                }
                                float _2457 = (-0.0f) - _1804;
                                float _2458 = _2457 / _1021;
                                frontier_phi_144_pred = (_2458 * _1018) + _1802;
                                frontier_phi_144_pred_1 = 1u;
                                frontier_phi_144_pred_2 = 4294967295u;
                                frontier_phi_144_pred_3 = (_2458 * _1020) + _1803;
                                frontier_phi_144_pred_4 = _2457 + _1804;
                                break;
                            }
                            _1379 = frontier_phi_144_pred;
                            _1381 = frontier_phi_144_pred_1;
                            _1479 = frontier_phi_144_pred_2;
                            _1378 = frontier_phi_144_pred_3;
                            _1377 = frontier_phi_144_pred_4;
                            if ((_1376 < _209) && (int(_1479) > int(4294967295u)))
                            {
                                _1462 = _1376;
                                _1463 = _1464;
                                _1465 = _1377;
                                _1466 = _1378;
                                _1467 = _1379;
                                _1468 = _1469;
                                _1470 = _1471;
                                _1472 = _1473;
                                _1474 = _1475;
                                _1476 = _1477;
                                _1478 = _1479;
                                _1480 = _1381;
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
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        _1375 = _1376;
                        _1071 = _1377;
                        _1069 = _1378;
                        _1067 = _1379;
                        _1380 = _1381;
                        _1061 = _1382;
                        _1059 = _1383;
                        _1057 = _1384;
                        _1055 = _1385;
                        _1053 = _1386;
                        _1051 = _1387;
                        _1049 = _1388;
                        _1047 = _1389;
                        _1045 = _1390;
                        _1043 = _1391;
                    }
                    bool _1392 = _1375 >= _209;
                    _1040 = uint(_1392);
                    _1042 = _1043;
                    _1044 = _1045;
                    _1046 = _1047;
                    _1048 = _1049;
                    _1050 = _1051;
                    _1052 = _1053;
                    _1054 = _1055;
                    _1056 = _1057;
                    _1058 = _1059;
                    _1060 = _1061;
                    _1062 = _1392 ? 1u : _1380;
                    _1064 = uint(_1375 <= _209);
                    _1066 = _1067;
                    _1068 = _1069;
                    _1070 = _1071;
                }
                float _1078 = _50_m0[51u].z * 2.0f;
                float _1081 = (_1078 * _981) + (-1.0f);
                float _1082 = ((1.0f - (_50_m0[51u].w * _982)) * 2.0f) + (-1.0f);
                float _1098 = mad(_190, _974, mad(_183, _1082, _1081 * _176)) + _197;
                float _1099 = (mad(_187, _974, mad(_180, _1082, _1081 * _173)) + _194) / _1098;
                float _1100 = (mad(_188, _974, mad(_181, _1082, _1081 * _174)) + _195) / _1098;
                float _1101 = (mad(_189, _974, mad(_182, _1082, _1081 * _175)) + _196) / _1098;
                float _1106 = (_1078 * _1066) + (-1.0f);
                float _1107 = ((1.0f - (_50_m0[51u].w * _1068)) * 2.0f) + (-1.0f);
                float _1123 = mad(_190, _1070, mad(_183, _1107, _1106 * _176)) + _197;
                float _1127 = ((mad(_187, _1070, mad(_180, _1107, _1106 * _173)) + _194) / _1123) - _1099;
                float _1128 = ((mad(_188, _1070, mad(_181, _1107, _1106 * _174)) + _195) / _1123) - _1100;
                float _1129 = ((mad(_189, _1070, mad(_182, _1107, _1106 * _175)) + _196) / _1123) - _1101;
                float _1219;
                uint _1221;
                float _1223;
                if (_1064 == 0u)
                {
                    _1219 = 0.0f;
                    _1221 = _1062;
                    _1223 = 0.0f;
                }
                else
                {
                    float _1266 = float(_238);
                    float _1267 = float(_239);
                    float frontier_phi_44_45_ladder;
                    uint frontier_phi_44_45_ladder_1;
                    float frontier_phi_44_45_ladder_2;
                    if ((_1066 < 0.0f) || (_1068 < 0.0f))
                    {
                        frontier_phi_44_45_ladder = 0.0f;
                        frontier_phi_44_45_ladder_1 = _1062;
                        frontier_phi_44_45_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_44_45_ladder_53_ladder;
                        uint frontier_phi_44_45_ladder_53_ladder_1;
                        float frontier_phi_44_45_ladder_53_ladder_2;
                        if ((_1070 >= 1.0f) || ((_1066 > _50_m0[51u].x) || (_1068 > _50_m0[51u].y)))
                        {
                            frontier_phi_44_45_ladder_53_ladder = 0.0f;
                            frontier_phi_44_45_ladder_53_ladder_1 = _1062;
                            frontier_phi_44_45_ladder_53_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_44_45_ladder_53_ladder_64_ladder;
                            uint frontier_phi_44_45_ladder_53_ladder_64_ladder_1;
                            float frontier_phi_44_45_ladder_53_ladder_64_ladder_2;
                            for (;;)
                            {
                                if ((abs(_1066 - _259) < (2.0f / _1266)) && (abs(_1068 - _260) < (2.0f / _1267)))
                                {
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder = 0.0f;
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder_1 = _1062;
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_98;
                                    float frontier_phi_98_pred;
                                    uint frontier_phi_98_pred_1;
                                    float frontier_phi_98_pred_2;
                                    uint _1681;
                                    uint _1682;
                                    bool _1683;
                                    for (;;)
                                    {
                                        _1681 = uint(int(_1066 * _1266));
                                        _1682 = uint(int(_1068 * _1267));
                                        _1683 = _1028 && _935;
                                        if (!_1683)
                                        {
                                            if (!(dot(float3(_1127, _1128, _1129), float3(_1127, _1128, _1129)) < _50_m0[4u].w))
                                            {
                                                ladder_phi_98 = false;
                                                frontier_phi_98_pred = 0.0f;
                                                frontier_phi_98_pred_1 = _1062;
                                                frontier_phi_98_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1759 = _24[22u].Load(int3(uint2(_1681, _1682), 0u));
                                        uint _1761 = _1759.x;
                                        float _2105;
                                        float _2106;
                                        float _2107;
                                        if (_1761 == 0u)
                                        {
                                            uint4 _1888 = _24[1u].Load(int3(uint2(_1681, _1682), 0u));
                                            uint _1890 = _1888.x;
                                            float _1898 = (float((_1890 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1899 = (float(_1890 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1903 = (1.0f - abs(_1898)) - abs(_1899);
                                            float _1905 = clamp((-0.0f) - _1903, 0.0f, 1.0f);
                                            float _1906 = (-0.0f) - _1905;
                                            _2105 = ((_1898 >= 0.0f) ? _1906 : _1905) + _1898;
                                            _2106 = ((_1899 >= 0.0f) ? _1906 : _1905) + _1899;
                                            _2107 = _1903;
                                        }
                                        else
                                        {
                                            float _1920 = (float((_1761 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1921 = (float(_1761 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1925 = (1.0f - abs(_1920)) - abs(_1921);
                                            float _1927 = clamp((-0.0f) - _1925, 0.0f, 1.0f);
                                            float _1928 = (-0.0f) - _1927;
                                            _2105 = ((_1920 >= 0.0f) ? _1928 : _1927) + _1920;
                                            _2106 = ((_1921 >= 0.0f) ? _1928 : _1927) + _1921;
                                            _2107 = _1925;
                                        }
                                        float _2111 = rsqrt(dot(float3(_2105, _2106, _2107), float3(_2105, _2106, _2107)));
                                        if (dot(float3(_2111 * _2105, _2111 * _2106, _2111 * _2107), float3(_1127, _1128, _1129)) > 0.0f)
                                        {
                                            ladder_phi_98 = true;
                                            frontier_phi_98_pred = 0.0f;
                                            frontier_phi_98_pred_1 = _1062;
                                            frontier_phi_98_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_98 = false;
                                            frontier_phi_98_pred = 0.0f;
                                            frontier_phi_98_pred_1 = _1062;
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
                                    float _1939 = _50_m0[51u].z * _1066;
                                    float _1940 = _50_m0[51u].w * _1068;
                                    float _1942 = (_1267 / _1266) * 0.0500000007450580596923828125f;
                                    float _1947 = clamp(_1939 / _1942, 0.0f, 1.0f);
                                    float _1948 = clamp(_1940 * 20.0f, 0.0f, 1.0f);
                                    float _1960 = clamp(((_1939 + (-1.0f)) + _1942) / _1942, 0.0f, 1.0f);
                                    float _1961 = clamp((_1940 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1972 = _1947 * _1948;
                                    precise float _1973 = _1972 * _1972;
                                    float _1977 = ((((3.0f - (_1948 * 2.0f)) * (3.0f - (_1947 * 2.0f))) * _1973) * (1.0f - ((_1960 * _1960) * (3.0f - (_1960 * 2.0f))))) * (1.0f - ((_1961 * _1961) * (3.0f - (_1961 * 2.0f))));
                                    bool _1980 = (_1062 != 0u) || (_1977 >= 1.0f);
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder = _1977 * float(_480 > 0.0f);
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder_1 = _1980 ? _1062 : 1u;
                                    frontier_phi_44_45_ladder_53_ladder_64_ladder_2 = _1980 ? 0.0f : _1977;
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
                    _1219 = frontier_phi_44_45_ladder_2;
                    _1221 = frontier_phi_44_45_ladder_1;
                    _1223 = frontier_phi_44_45_ladder;
                }
                uint _1350;
                uint _1352;
                float _1264;
                for (;;)
                {
                    _1264 = ((((exp2(log2(clamp((sqrt(((_1100 * _1100) + (_1099 * _1099)) + (_1101 * _1101)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _412) * exp2(log2(clamp((_1100 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_398, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    if (_1223 > 0.0f)
                    {
                        uint _1312 = uint((_202 * 0.999989986419677734375f) * clamp(_1066, 0.0f, 1.0f));
                        uint _1313 = uint((_204 * 0.999989986419677734375f) * clamp(_1068, 0.0f, 1.0f));
                        uint4 _1316 = _24[2u].Load(int3(uint2(_1312, _1313), 0u));
                        uint _1319 = _1316.w;
                        uint4 _1324 = _24[15u].Load(int3(uint2(_1312, _1313), 0u));
                        uint _1326 = _1324.y;
                        uint _1332 = ((_1326 & 64u) != 0u) ? uint((_1326 & 4294967167u) != 66u) : 4294967295u;
                        uint _1333 = _1319 & 128u;
                        uint _1335 = (_1333 != 0u) ? 1u : ((_1316.x << 7u) | _1319);
                        uint4 _1338 = _16.Load(_1335 * 4u);
                        uint _1339 = _1338.x;
                        uint _1346 = ((_1339 & 1u) != 0u) ? 0u : 18u;
                        uint _1348 = uint(min(int(uint(max(int(_1332), int(0u)))), int(1u)));
                        uint _1415;
                        if (_1333 == 0u)
                        {
                            _1415 = (((_1339 & 2097152u) != 0u) && (_1332 == _1348)) ? (_1346 | 128u) : _1346;
                        }
                        else
                        {
                            _1415 = _1319;
                        }
                        uint _1416 = _16.Load((_1335 * 4u) + 1u).x & 512u;
                        bool _1419 = (_1415 & 144u) == 0u;
                        if (_1416 == 0u)
                        {
                            if (_1419 || ((_1339 & 1u) != 0u))
                            {
                                _1350 = 0u;
                                _1352 = 0u;
                                break;
                            }
                        }
                        else
                        {
                            if (_1419)
                            {
                                _1350 = 0u;
                                _1352 = 0u;
                                break;
                            }
                        }
                        bool _1738 = ((_1415 & 128u) | _1416) != 0u;
                        uint _1351;
                        if (_1738)
                        {
                            _1351 = 1u;
                        }
                        else
                        {
                            _1351 = (((_1339 >> 14u) & 2u) ^ 2u) + 3u;
                        }
                        if (((_1339 & 268435472u) == 16u) && (((_1339 & 33554432u) == 0u) || _1738))
                        {
                            _1350 = 2u;
                            _1352 = 0u;
                            break;
                        }
                        _1350 = _1351;
                        _1352 = (_1351 == 1u) ? _1348 : 0u;
                        break;
                    }
                    else
                    {
                        _1350 = 0u;
                        _1352 = 0u;
                        break;
                    }
                }
                bool _1424;
                float _1427;
                float _1429;
                float _1431;
                float _1433;
                float _1435;
                float _1437;
                float _1439;
                float _1440;
                float _1441;
                float _1443;
                float _1356;
                float _1359;
                float _1362;
                float _1366;
                float _1367;
                for (;;)
                {
                    _1356 = mad(_167, _931, mad(_161, _930, _929 * _155));
                    _1359 = mad(_168, _931, mad(_162, _930, _929 * _156));
                    _1362 = mad(_169, _931, mad(_163, _930, _929 * _157));
                    bool _1365 = (_1221 != 0u) || (_1223 < 1.0f);
                    _1366 = _1365 ? 0.0f : 1.0f;
                    _1367 = _1365 ? 0.0f : 0.5f;
                    if (_1365)
                    {
                        uint4 _1423 = asuint(_50_m0[60u]);
                        if (_1028)
                        {
                            if (int(_410) < int(1u))
                            {
                                if (_1423.x == 0u)
                                {
                                    _1424 = false;
                                    _1427 = 0.0f;
                                    _1429 = 0.0f;
                                    _1431 = 0.0f;
                                    _1433 = _1056;
                                    _1435 = _1058;
                                    _1437 = _1060;
                                    _1439 = 0.0f;
                                    _1440 = 0.0f;
                                    _1441 = 0.0f;
                                    _1443 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1423.y == 0u)
                                {
                                    _1424 = false;
                                    _1427 = 0.0f;
                                    _1429 = 0.0f;
                                    _1431 = 0.0f;
                                    _1433 = _1056;
                                    _1435 = _1058;
                                    _1437 = _1060;
                                    _1439 = 0.0f;
                                    _1440 = 0.0f;
                                    _1441 = 0.0f;
                                    _1443 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1423.z == 0u)
                            {
                                _1424 = false;
                                _1427 = 0.0f;
                                _1429 = 0.0f;
                                _1431 = 0.0f;
                                _1433 = _1056;
                                _1435 = _1058;
                                _1437 = _1060;
                                _1439 = 0.0f;
                                _1440 = 0.0f;
                                _1441 = 0.0f;
                                _1443 = 0.0f;
                                break;
                            }
                        }
                        if (_1219 > 0.0f)
                        {
                            _1424 = false;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 0.0f;
                            _1433 = _1056;
                            _1435 = _1058;
                            _1437 = _1060;
                            _1439 = 0.0f;
                            _1440 = 1.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        if (((_1050 <= 0.0f) || (_1052 <= 0.0f)) || (_1054 <= 0.0f))
                        {
                            _1424 = false;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 0.0f;
                            _1433 = _1056;
                            _1435 = _1058;
                            _1437 = _1060;
                            _1439 = 0.0f;
                            _1440 = 1.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        if ((_1054 >= 1.0f) || ((_1050 >= _50_m0[51u].x) || (_1052 >= _50_m0[51u].y)))
                        {
                            _1424 = false;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 0.0f;
                            _1433 = _1056;
                            _1435 = _1058;
                            _1437 = _1060;
                            _1439 = 0.0f;
                            _1440 = 1.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        uint _2474;
                        uint _2476;
                        uint _2173;
                        uint _2174;
                        bool _2180;
                        for (;;)
                        {
                            _2173 = uint(clamp(_1044, 0.0f, 1.0f) * _202);
                            _2174 = uint(clamp(_1046, 0.0f, 1.0f) * _204);
                            _2180 = _20[21u].Load(int3(uint2(_2173, _2174), 0u)).x > 0.0f;
                            if (_2180)
                            {
                                uint _2376 = _24[23u].Load(int3(uint2(_2173, _2174), 0u)).y + 4294967295u;
                                _2474 = (uint(int(_2376) >> int(31u)) & 3u) + 1u;
                                _2476 = (int(_2376) < int(0u)) ? 0u : _2376;
                                break;
                            }
                            else
                            {
                                uint4 _2384 = _24[2u].Load(int3(uint2(_2173, _2174), 0u));
                                uint _2387 = _2384.w;
                                uint4 _2392 = _24[15u].Load(int3(uint2(_2173, _2174), 0u));
                                uint _2394 = _2392.y;
                                uint _2400 = ((_2394 & 64u) != 0u) ? uint((_2394 & 4294967167u) != 66u) : 4294967295u;
                                uint _2401 = _2387 & 128u;
                                uint _2403 = (_2401 != 0u) ? 1u : ((_2384.x << 7u) | _2387);
                                uint4 _2406 = _16.Load(_2403 * 4u);
                                uint _2407 = _2406.x;
                                uint _2414 = ((_2407 & 1u) != 0u) ? 0u : 18u;
                                uint _2416 = uint(min(int(uint(max(int(_2400), int(0u)))), int(1u)));
                                uint _2487;
                                if (_2401 == 0u)
                                {
                                    _2487 = (((_2407 & 2097152u) != 0u) && (_2400 == _2416)) ? (_2414 | 128u) : _2414;
                                }
                                else
                                {
                                    _2487 = _2387;
                                }
                                uint _2488 = _16.Load((_2403 * 4u) + 1u).x & 512u;
                                bool _2491 = (_2487 & 144u) == 0u;
                                if (_2488 == 0u)
                                {
                                    if (_2491 || ((_2407 & 1u) != 0u))
                                    {
                                        _2474 = 0u;
                                        _2476 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2491)
                                    {
                                        _2474 = 0u;
                                        _2476 = 0u;
                                        break;
                                    }
                                }
                                bool _2767 = ((_2487 & 128u) | _2488) != 0u;
                                uint _2475;
                                if (_2767)
                                {
                                    _2475 = 1u;
                                }
                                else
                                {
                                    _2475 = (((_2407 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_2407 & 268435472u) == 16u) && (((_2407 & 33554432u) == 0u) || _2767))
                                {
                                    _2474 = 2u;
                                    _2476 = 0u;
                                    break;
                                }
                                _2474 = _2475;
                                _2476 = (_2475 == 1u) ? _2416 : 0u;
                                break;
                            }
                        }
                        if ((_402 != _2474) || (_410 != _2476))
                        {
                            _1424 = false;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 0.0f;
                            _1433 = _1056;
                            _1435 = _1058;
                            _1437 = _1060;
                            _1439 = 0.0f;
                            _1440 = 1.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2618 = _1050 * 2.0f;
                        float _2621 = (_50_m0[51u].z * _2618) + (-1.0f);
                        float _2622 = ((1.0f - (_50_m0[51u].w * _1052)) * 2.0f) + (-1.0f);
                        float _2638 = mad(_190, _1054, mad(_183, _2622, _2621 * _176)) + _197;
                        float _2639 = (mad(_187, _1054, mad(_180, _2622, _2621 * _173)) + _194) / _2638;
                        float _2640 = (mad(_188, _1054, mad(_181, _2622, _2621 * _174)) + _195) / _2638;
                        float _2641 = (mad(_189, _1054, mad(_182, _2622, _2621 * _175)) + _196) / _2638;
                        if (sqrt(((_2640 * _2640) + (_2639 * _2639)) + (_2641 * _2641)) > _50_m0[58u].w)
                        {
                            _1424 = false;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 0.0f;
                            _1433 = _1056;
                            _1435 = _1058;
                            _1437 = _1060;
                            _1439 = 0.0f;
                            _1440 = 0.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2747 = _2639 - _1099;
                        float _2748 = _2640 - _1100;
                        float _2749 = _2641 - _1101;
                        float _2755 = sqrt(((_2748 * _2748) + (_2747 * _2747)) + (_2749 * _2749));
                        float _2763 = min(_50_m0[59u].y, max(0.0f, _2755 + (-0.001000000047497451305389404296875f)));
                        float _2786;
                        if (_1028)
                        {
                            _2786 = min(_50_m0[59u].x, _50_m0[4u].z + _2763);
                        }
                        else
                        {
                            _2786 = _50_m0[59u].x;
                        }
                        float _2787 = _2786 - _2755;
                        if (!(_2787 > 0.0f))
                        {
                            _1424 = false;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 0.0f;
                            _1433 = _1056;
                            _1435 = _1058;
                            _1437 = _1060;
                            _1439 = 1.0f;
                            _1440 = 1.0f;
                            _1441 = 0.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2846 = _2639 - (_2763 * _1356);
                        float _2847 = _2640 - (_2763 * _1359);
                        float _2848 = _2641 - (_2763 * _1362);
                        RayDesc _2ident = {float3(mad(_2848, _50_m0[46u].z, mad(_2847, _50_m0[46u].y, _50_m0[46u].x * _2846)) + _50_m0[46u].w, mad(_2848, _50_m0[47u].z, mad(_2847, _50_m0[47u].y, _50_m0[47u].x * _2846)) + _50_m0[47u].w, mad(_2848, _50_m0[48u].z, mad(_2847, _50_m0[48u].y, _50_m0[48u].x * _2846)) + _50_m0[48u].w), 0.0f, float3(mad(_1362, _50_m0[46u].z, mad(_1359, _50_m0[46u].y, _50_m0[46u].x * _1356)), mad(_1362, _50_m0[47u].z, mad(_1359, _50_m0[47u].y, _50_m0[47u].x * _1356)), mad(_1362, _50_m0[48u].z, mad(_1359, _50_m0[48u].y, _50_m0[48u].x * _1356))), _2787};
                        _2851.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2898 = _2851.Proceed();
                        uint _2899 = _2851.CommittedStatus();
                        if (!(_2899 == 1u))
                        {
                            _1424 = false;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 0.0f;
                            _1433 = _1056;
                            _1435 = _1058;
                            _1437 = _1060;
                            _1439 = 1.0f;
                            _1440 = 0.0f;
                            _1441 = 0.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2913 = _2851.CommittedRayT();
                        if (!((_2913 < _2787) && (_2913 > 0.0f)))
                        {
                            _1424 = false;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 0.0f;
                            _1433 = _1056;
                            _1435 = _1058;
                            _1437 = _1060;
                            _1439 = 1.0f;
                            _1440 = 0.0f;
                            _1441 = 0.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2927 = (_50_m0[51u].z * _2618) + (-1.0f);
                        float _2928 = ((1.0f - (_50_m0[51u].w * _1052)) * 2.0f) + (-1.0f);
                        float _2944 = mad(_144, _1054, mad(_137, _2928, _2927 * _130)) + _151;
                        float _2948 = _2913 - _2763;
                        float _2952 = ((mad(_141, _1054, mad(_134, _2928, _2927 * _127)) + _148) / _2944) + (_2948 * _929);
                        float _2953 = ((mad(_142, _1054, mad(_135, _2928, _2927 * _128)) + _149) / _2944) + (_2948 * _930);
                        float _2954 = ((mad(_143, _1054, mad(_136, _2928, _2927 * _129)) + _150) / _2944) + (_2948 * _931);
                        float _2970 = mad(_116, _2954, mad(_109, _2953, _2952 * _102)) + _123;
                        float _1438 = (mad(_115, _2954, mad(_108, _2953, _2952 * _101)) + _122) / _2970;
                        float _1434 = ((((mad(_113, _2954, mad(_106, _2953, _2952 * _99)) + _120) / _2970) * 0.5f) + 0.5f) * _50_m0[51u].x;
                        float _1436 = (0.5f - (((mad(_114, _2954, mad(_107, _2953, _2952 * _100)) + _121) / _2970) * 0.5f)) * _50_m0[51u].y;
                        float _2979 = _1434 * _50_m0[51u].z;
                        float _2980 = _1436 * _50_m0[51u].w;
                        float _2982 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2985 = clamp(_2979 / _2982, 0.0f, 1.0f);
                        float _2986 = clamp(_2980 * 20.0f, 0.0f, 1.0f);
                        float _2996 = clamp(((_2979 + (-1.0f)) + _2982) / _2982, 0.0f, 1.0f);
                        float _2997 = clamp((_2980 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _3008 = _2985 * _2986;
                        precise float _3009 = _3008 * _3008;
                        if ((((((3.0f - (_2986 * 2.0f)) * (3.0f - (_2985 * 2.0f))) * _3009) * (1.0f - ((_2996 * _2996) * (3.0f - (_2996 * 2.0f))))) * (1.0f - ((_2997 * _2997) * (3.0f - (_2997 * 2.0f))))) < 1.0f)
                        {
                            _1424 = false;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 0.0f;
                            _1433 = _1434;
                            _1435 = _1436;
                            _1437 = _1438;
                            _1439 = _1366;
                            _1440 = _1366;
                            _1441 = 0.0f;
                            _1443 = _1367;
                            break;
                        }
                        _1424 = true;
                        _1427 = _50_m0[1u].w;
                        _1429 = _50_m0[2u].x;
                        _1431 = _50_m0[2u].y;
                        _1433 = _1434;
                        _1435 = _1436;
                        _1437 = _1438;
                        _1439 = 0.0f;
                        _1440 = 1.0f;
                        _1441 = 0.0f;
                        _1443 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1424 = false;
                        _1427 = 0.0f;
                        _1429 = 0.0f;
                        _1431 = 0.0f;
                        _1433 = _1056;
                        _1435 = _1058;
                        _1437 = _1060;
                        _1439 = 1.0f;
                        _1440 = 1.0f;
                        _1441 = 0.0f;
                        _1443 = 0.5f;
                        break;
                    }
                }
                uint4 _1446 = asuint(_55_m0[0u]);
                float _1448 = float(_1446.x);
                float _1450 = float(_1446.y);
                float _1509;
                float _1511;
                float _1513;
                if ((_1223 >= 1.0f) || _1424)
                {
                    _1509 = _1433;
                    _1511 = _1435;
                    _1513 = _1437;
                }
                else
                {
                    float _1666 = (-0.0f) - _974;
                    float _1667 = _1666 / _1021;
                    float _1670 = (_1667 * _1018) + _981;
                    float _1671 = (_1667 * _1020) + _982;
                    float _1672 = _1666 + _974;
                    _1509 = ((_1433 - _1670) * _1223) + _1670;
                    _1511 = ((_1435 - _1671) * _1223) + _1671;
                    _1513 = ((_1437 - _1672) * _1223) + _1672;
                }
                float _1524 = ((_1509 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _1525 = ((1.0f - (_50_m0[51u].w * _1511)) * 2.0f) + (-1.0f);
                float _1541 = mad(_144, _1513, mad(_137, _1525, _1524 * _130)) + _151;
                float _1545 = ((mad(_141, _1513, mad(_134, _1525, _1524 * _127)) + _148) / _1541) - _953;
                float _1546 = ((mad(_142, _1513, mad(_135, _1525, _1524 * _128)) + _149) / _1541) - _954;
                float _1547 = ((mad(_143, _1513, mad(_136, _1525, _1524 * _129)) + _150) / _1541) - _955;
                float _1553 = sqrt(((_1546 * _1546) + (_1545 * _1545)) + (_1547 * _1547));
                float _1554 = _1553 * _783;
                float _1555 = _1553 * _784;
                float _1556 = _1553 * _785;
                float _1557 = _1553 * (_896 / _899);
                float _1558 = _1553 * (_897 / _899);
                float _1559 = _1553 * (_898 / _899);
                float _1563 = dot(float3(_1554, _1555, _1556), float3(_769, _772, _775)) * 2.0f;
                float _1573 = dot(float3(_1557, _1558, _1559), float3(_769, _772, _775)) * 2.0f;
                float _1600 = (_1554 - (_1563 * _769)) + _953;
                float _1601 = (_1555 - (_1563 * _772)) + _954;
                float _1602 = (_1556 - (_1563 * _775)) + _955;
                float _1614 = mad(_50_m0[24u].w, _1602, mad(_50_m0[23u].w, _1601, _1600 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1619 = (_1557 - (_1573 * _769)) + _953;
                float _1620 = (_1558 - (_1573 * _772)) + _954;
                float _1621 = (_1559 - (_1573 * _775)) + _955;
                float _1633 = mad(_50_m0[24u].w, _1621, mad(_50_m0[23u].w, _1620, _1619 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1639 = (_50_m0[51u].x * ((((mad(_50_m0[24u].x, _1602, mad(_50_m0[23u].x, _1601, _1600 * _50_m0[22u].x)) + _50_m0[25u].x) / _1614) - ((mad(_50_m0[24u].x, _1621, mad(_50_m0[23u].x, _1620, _1619 * _50_m0[22u].x)) + _50_m0[25u].x) / _1633)) * 0.5f)) * _1448;
                float _1643 = (_50_m0[51u].y * ((((mad(_50_m0[24u].y, _1621, mad(_50_m0[23u].y, _1620, _1619 * _50_m0[22u].y)) + _50_m0[25u].y) / _1633) - ((mad(_50_m0[24u].y, _1602, mad(_50_m0[23u].y, _1601, _1600 * _50_m0[22u].y)) + _50_m0[25u].y) / _1614)) * 0.5f)) * _1450;
                uint4 _1657 = asuint(_50_m0[55u]);
                float _1662 = clamp(log2((sqrt((_1643 * _1643) + (_1639 * _1639)) * 2.0f) / _50_m0[0u].w) / float(_1657.x + 4294967295u), 0.0f, 1.0f);
                uint _1747;
                float _1748;
                float _1749;
                float _1751;
                float _1753;
                if (_1028 && (_1657.z != 0u))
                {
                    float frontier_phi_84_83_ladder;
                    uint frontier_phi_84_83_ladder_1;
                    float frontier_phi_84_83_ladder_2;
                    float frontier_phi_84_83_ladder_3;
                    float frontier_phi_84_83_ladder_4;
                    if ((_1350 != 1u) || (_1352 != 0u))
                    {
                        float _1868 = _974 / max(9.9999999747524270787835121154785e-07f, (-0.0f) - _1021);
                        float _1873 = ((_1868 * _1018) + _981) / _50_m0[51u].x;
                        float _1874 = ((_1868 * _1020) + _982) / _50_m0[51u].y;
                        float _2044;
                        if (_1874 < 0.300000011920928955078125f)
                        {
                            float _2036 = (0.300000011920928955078125f - _1874) * 3.3333332538604736328125f;
                            _2044 = 0.300000011920928955078125f - ((_2036 / sqrt((_2036 * _2036) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _2044 = _1874;
                        }
                        float _2189;
                        if (_1873 < 0.300000011920928955078125f)
                        {
                            float _2182 = (0.300000011920928955078125f - _1873) * 3.3333332538604736328125f;
                            _2189 = 0.300000011920928955078125f - ((_2182 / sqrt((_2182 * _2182) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _2189 = _1873;
                        }
                        float _2428;
                        if ((1.0f - _2189) < 0.300000011920928955078125f)
                        {
                            float _2420 = (_2189 + (-0.699999988079071044921875f)) * 3.3333332538604736328125f;
                            _2428 = ((_2420 / sqrt((_2420 * _2420) + 1.0f)) * 0.300000011920928955078125f) + 0.699999988079071044921875f;
                        }
                        else
                        {
                            _2428 = _2189;
                        }
                        frontier_phi_84_83_ladder = _2044 * _50_m0[51u].y;
                        frontier_phi_84_83_ladder_1 = 0u;
                        frontier_phi_84_83_ladder_2 = 0.0f;
                        frontier_phi_84_83_ladder_3 = _2428 * _50_m0[51u].x;
                        frontier_phi_84_83_ladder_4 = 1.0f - clamp((_931 + (-0.25f)) * (-4.0f), 0.0f, 1.0f);
                    }
                    else
                    {
                        frontier_phi_84_83_ladder = _1068;
                        frontier_phi_84_83_ladder_1 = _1040;
                        frontier_phi_84_83_ladder_2 = _1048;
                        frontier_phi_84_83_ladder_3 = _1066;
                        frontier_phi_84_83_ladder_4 = _1223;
                    }
                    _1747 = frontier_phi_84_83_ladder_1;
                    _1748 = frontier_phi_84_83_ladder_2;
                    _1749 = frontier_phi_84_83_ladder_3;
                    _1751 = frontier_phi_84_83_ladder;
                    _1753 = frontier_phi_84_83_ladder_4;
                }
                else
                {
                    _1747 = _1040;
                    _1748 = _1048;
                    _1749 = _1066;
                    _1751 = _1068;
                    _1753 = _1223;
                }
                bool _1755 = _402 != 1u;
                float _2192;
                float _2194;
                float _2196;
                float _2198;
                if (_1753 == 0.0f)
                {
                    float _2046;
                    float _2048;
                    float _2050;
                    float _2052;
                    if (_1424)
                    {
                        _2046 = _1427;
                        _2048 = _1429;
                        _2050 = _1431;
                        _2052 = 1.0f;
                    }
                    else
                    {
                        float frontier_phi_107_108_ladder;
                        float frontier_phi_107_108_ladder_1;
                        float frontier_phi_107_108_ladder_2;
                        float frontier_phi_107_108_ladder_3;
                        if (_1755)
                        {
                            float _2249 = _1359 * _1264;
                            float _2253 = rsqrt(dot(float3(_1356, _2249, _1362), float3(_1356, _2249, _1362)));
                            float4 _2263 = _28[4u].SampleLevel(_59, float3(_2253 * _1356, _2253 * _2249, _2253 * _1362), 0.0f);
                            frontier_phi_107_108_ladder = 1.0f;
                            frontier_phi_107_108_ladder_1 = _2263.z;
                            frontier_phi_107_108_ladder_2 = _2263.y;
                            frontier_phi_107_108_ladder_3 = _2263.x;
                        }
                        else
                        {
                            frontier_phi_107_108_ladder = 0.0f;
                            frontier_phi_107_108_ladder_1 = 0.0f;
                            frontier_phi_107_108_ladder_2 = 0.0f;
                            frontier_phi_107_108_ladder_3 = 0.0f;
                        }
                        _2046 = frontier_phi_107_108_ladder_3;
                        _2048 = frontier_phi_107_108_ladder_2;
                        _2050 = frontier_phi_107_108_ladder_1;
                        _2052 = frontier_phi_107_108_ladder;
                    }
                    _2192 = _2046 * _480;
                    _2194 = _2048 * _480;
                    _2196 = _2050 * _480;
                    _2198 = _2052 * _480;
                }
                else
                {
                    float _2289;
                    float _2291;
                    float _2294;
                    float _2296;
                    if (_12.Load(int3(uint2(uint(_1749 * _1448), uint(_1751 * _1450)), 0u)).x > 0.0f)
                    {
                        uint _2058_dummy_parameter;
                        uint2 _2058 = spvTextureSize(_14, 0u, _2058_dummy_parameter);
                        float4 _2067 = _14.Load(int3(uint2(uint(float(_2058.x) * _1749), uint(float(_2058.y) * _1751)), 0u));
                        float _2071 = _2067.x * 0.5f;
                        float _2072 = _2067.y * (-0.5f);
                        float4 _2091 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _2071) + (_50_m0[52u].x * _1749), (_50_m0[52u].w * _2072) + (_50_m0[52u].y * _1751)), 0.0f);
                        float _2101 = _50_m0[54u].x * _2091.x;
                        float _2102 = _50_m0[54u].x * _2091.y;
                        float _2103 = _50_m0[54u].x * _2091.z;
                        float _2276;
                        if (_1028)
                        {
                            float frontier_phi_123_122_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _974))) < 5.0f)
                            {
                                float _2444 = sqrt((_2071 * _2071) + (_2072 * _2072));
                                float frontier_phi_123_122_ladder_139_ladder;
                                if (_2444 > 0.0500000007450580596923828125f)
                                {
                                    float _2499 = _1749 - _981;
                                    float _2500 = _1751 - _982;
                                    float frontier_phi_123_122_ladder_139_ladder_152_ladder;
                                    if (_2444 > sqrt((_2500 * _2500) + (_2499 * _2499)))
                                    {
                                        uint4 _2665 = asuint(_55_m0[0u]);
                                        uint _2672 = uint(float(_2665.x) * _1749);
                                        uint _2673 = uint(float(_2665.y) * _1751);
                                        uint4 _2676 = _24[2u].Load(int3(uint2(_2672, _2673), 0u));
                                        uint _2679 = _2676.w;
                                        uint4 _2684 = _24[15u].Load(int3(uint2(_2672, _2673), 0u));
                                        uint _2686 = _2684.y;
                                        uint _2692 = ((_2686 & 64u) != 0u) ? uint((_2686 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2693 = _2679 & 128u;
                                        uint _2695 = (_2693 != 0u) ? 1u : ((_2676.x << 7u) | _2679);
                                        uint4 _2698 = _16.Load(_2695 * 4u);
                                        uint _2699 = _2698.x;
                                        uint _2706 = ((_2699 & 1u) != 0u) ? 0u : 18u;
                                        uint _2776;
                                        if (_2693 == 0u)
                                        {
                                            _2776 = (((_2699 & 2097152u) != 0u) && (_2692 == uint(min(int(uint(max(int(_2692), int(0u)))), int(1u))))) ? (_2706 | 128u) : _2706;
                                        }
                                        else
                                        {
                                            _2776 = _2679;
                                        }
                                        float frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder;
                                        if (((_2776 & 128u) | (_16.Load((_2695 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2801 = asuint(_55_m0[0u]);
                                            uint _2810 = uint(float(_2801.x) * (_2071 + _1749));
                                            uint _2811 = uint(float(_2801.y) * (_2072 + _1751));
                                            uint4 _2814 = _24[2u].Load(int3(uint2(_2810, _2811), 0u));
                                            uint _2817 = _2814.w;
                                            uint4 _2820 = _24[15u].Load(int3(uint2(_2810, _2811), 0u));
                                            uint _2822 = _2820.y;
                                            uint _2828 = ((_2822 & 64u) != 0u) ? uint((_2822 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2829 = _2817 & 128u;
                                            uint _2831 = (_2829 != 0u) ? 1u : ((_2814.x << 7u) | _2817);
                                            uint4 _2833 = _16.Load(_2831 * 4u);
                                            uint _2834 = _2833.x;
                                            uint _2841 = ((_2834 & 1u) != 0u) ? 0u : 18u;
                                            uint _2910;
                                            if (_2829 == 0u)
                                            {
                                                _2910 = (((_2834 & 2097152u) != 0u) && (_2828 == uint(min(int(uint(max(int(_2828), int(0u)))), int(1u))))) ? (_2841 | 128u) : _2841;
                                            }
                                            else
                                            {
                                                _2910 = _2817;
                                            }
                                            float frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder_179_ladder;
                                            if ((_2910 & 128u) == 0u)
                                            {
                                                frontier_phi_123_122_ladder_139_ladder_152_ladder_170_ladder_179_ladder = ((_16.Load((_2831 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
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
                            _2276 = frontier_phi_123_122_ladder;
                        }
                        else
                        {
                            _2276 = 1.0f;
                        }
                        float frontier_phi_125_123_ladder;
                        float frontier_phi_125_123_ladder_1;
                        float frontier_phi_125_123_ladder_2;
                        float frontier_phi_125_123_ladder_3;
                        float _2278;
                        for (;;)
                        {
                            _2278 = _2276 * _1753;
                            if (_1748 > 0.0f)
                            {
                                float _2293;
                                float _2295;
                                float _2297;
                                if (_1028)
                                {
                                    _2293 = 0.0f;
                                    _2295 = 0.0f;
                                    _2297 = 0.0f;
                                }
                                else
                                {
                                    if (!((_402 != 4u) || (_1042 != 0u)))
                                    {
                                        frontier_phi_125_123_ladder = _2101;
                                        frontier_phi_125_123_ladder_1 = _2278;
                                        frontier_phi_125_123_ladder_2 = _2102;
                                        frontier_phi_125_123_ladder_3 = _2103;
                                        break;
                                    }
                                    _2293 = _2101;
                                    _2295 = _2102;
                                    _2297 = _2103;
                                }
                                frontier_phi_125_123_ladder = _2293;
                                frontier_phi_125_123_ladder_1 = (1.0f - exp2(log2(_1748) * _50_m0[4u].x)) * _2278;
                                frontier_phi_125_123_ladder_2 = _2295;
                                frontier_phi_125_123_ladder_3 = _2297;
                                break;
                            }
                            else
                            {
                                frontier_phi_125_123_ladder = _2101;
                                frontier_phi_125_123_ladder_1 = _2278;
                                frontier_phi_125_123_ladder_2 = _2102;
                                frontier_phi_125_123_ladder_3 = _2103;
                                break;
                            }
                        }
                        _2289 = frontier_phi_125_123_ladder_1;
                        _2291 = frontier_phi_125_123_ladder;
                        _2294 = frontier_phi_125_123_ladder_2;
                        _2296 = frontier_phi_125_123_ladder_3;
                    }
                    else
                    {
                        float frontier_phi_125_110_ladder;
                        float frontier_phi_125_110_ladder_1;
                        float frontier_phi_125_110_ladder_2;
                        float frontier_phi_125_110_ladder_3;
                        if (_1747 == 0u)
                        {
                            float4 _2284 = _28[7u].SampleLevel(_59, float3(_1356, _1359, _1362), 0.0f);
                            frontier_phi_125_110_ladder = _2284.x;
                            frontier_phi_125_110_ladder_1 = _1753;
                            frontier_phi_125_110_ladder_2 = _2284.y;
                            frontier_phi_125_110_ladder_3 = _2284.z;
                        }
                        else
                        {
                            frontier_phi_125_110_ladder = _2292;
                            frontier_phi_125_110_ladder_1 = 0.0f;
                            frontier_phi_125_110_ladder_2 = _2292;
                            frontier_phi_125_110_ladder_3 = _2292;
                        }
                        _2289 = frontier_phi_125_110_ladder_1;
                        _2291 = frontier_phi_125_110_ladder;
                        _2294 = frontier_phi_125_110_ladder_2;
                        _2296 = frontier_phi_125_110_ladder_3;
                    }
                    float _2516;
                    float _2517;
                    float _2519;
                    float _2521;
                    if (_1424)
                    {
                        _2516 = 1.0f;
                        _2517 = ((_2291 - _1427) * _1219) + _1427;
                        _2519 = ((_2294 - _1429) * _1219) + _1429;
                        _2521 = ((_2296 - _1431) * _1219) + _1431;
                    }
                    else
                    {
                        float frontier_phi_155_142_ladder;
                        float frontier_phi_155_142_ladder_1;
                        float frontier_phi_155_142_ladder_2;
                        float frontier_phi_155_142_ladder_3;
                        if (_1755 && (_2289 < 1.0f))
                        {
                            float _2523 = _1359 * _1264;
                            float _2527 = rsqrt(dot(float3(_1356, _2523, _1362), float3(_1356, _2523, _1362)));
                            float4 _2535 = _28[4u].SampleLevel(_59, float3(_2527 * _1356, _2527 * _2523, _2527 * _1362), 0.0f);
                            float _2537 = _2535.x;
                            float _2538 = _2535.y;
                            float _2539 = _2535.z;
                            frontier_phi_155_142_ladder = ((_2296 - _2539) * _2289) + _2539;
                            frontier_phi_155_142_ladder_1 = ((_2294 - _2538) * _2289) + _2538;
                            frontier_phi_155_142_ladder_2 = ((_2291 - _2537) * _2289) + _2537;
                            frontier_phi_155_142_ladder_3 = 1.0f;
                        }
                        else
                        {
                            frontier_phi_155_142_ladder = _2296;
                            frontier_phi_155_142_ladder_1 = _2294;
                            frontier_phi_155_142_ladder_2 = _2291;
                            frontier_phi_155_142_ladder_3 = _2289;
                        }
                        _2516 = frontier_phi_155_142_ladder_3;
                        _2517 = frontier_phi_155_142_ladder_2;
                        _2519 = frontier_phi_155_142_ladder_1;
                        _2521 = frontier_phi_155_142_ladder;
                    }
                    float _2199 = _2516 * _480;
                    _2192 = _2517 * _2199;
                    _2194 = _2519 * _2199;
                    _2196 = _2521 * _2199;
                    _2198 = _2199;
                }
                float _2203 = _50_m0[58u].z * _1443;
                float _2225 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _2227 = _2225 * ((_2203 * ((_1439 * 1000.0f) - _2192)) + _2192);
                float _2228 = _2225 * ((_2203 * ((_1440 * 1000.0f) - _2194)) + _2194);
                float _2229 = _2225 * ((_2203 * (_1441 - _2196)) + _2196);
                float _2235 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2227, max(_2228, _2229)) + 1.0f);
                float _2239 = min(_2235 * _2227, 0.996078431606292724609375f);
                float _2241 = min(_2235 * _2228, 0.996078431606292724609375f);
                float _2242 = min(_2235 * _2229, 0.996078431606292724609375f);
                _35[uint2(_226, _229)] = float4(_2239, _2241, _2242, _2198);
                _39[uint2(_226, _229)] = float4(_1662, 0.0f, 0.0f, _1662);
                if (_233)
                {
                    uint _2434 = _226 + 1u;
                    _35[uint2(_2434, _229)] = float4(_2239, _2241, _2242, _2198);
                    _39[uint2(_2434, _229)] = float4(_1662, 0.0f, 0.0f, _1662);
                }
                if (_236)
                {
                    uint _2492 = _229 + 1u;
                    _35[uint2(_226, _2492)] = float4(_2239, _2241, _2242, _2198);
                    _39[uint2(_226, _2492)] = float4(_1662, 0.0f, 0.0f, _1662);
                }
                if (_237)
                {
                    uint _2655 = _226 + 1u;
                    uint _2656 = _229 + 1u;
                    _35[uint2(_2655, _2656)] = float4(_2239, _2241, _2242, _2198);
                    _39[uint2(_2655, _2656)] = float4(_1662, 0.0f, 0.0f, _1662);
                }
                ladder_phi_8 = true;
                break;
            }
            if (ladder_phi_8)
            {
                break;
            }
            _35[uint2(_226, _229)] = 0.0f.xxxx;
            _39[uint2(_226, _229)] = 0.0f.xxxx;
            if (_233)
            {
                uint _445 = _226 + 1u;
                _35[uint2(_445, _229)] = 0.0f.xxxx;
                _39[uint2(_445, _229)] = 0.0f.xxxx;
            }
            if (_236)
            {
                uint _556 = _229 + 1u;
                _35[uint2(_226, _556)] = 0.0f.xxxx;
                _39[uint2(_226, _556)] = 0.0f.xxxx;
            }
            if (!_237)
            {
                break;
            }
            uint _655 = _226 + 1u;
            uint _656 = _229 + 1u;
            _35[uint2(_655, _656)] = 0.0f.xxxx;
            _39[uint2(_655, _656)] = 0.0f.xxxx;
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
