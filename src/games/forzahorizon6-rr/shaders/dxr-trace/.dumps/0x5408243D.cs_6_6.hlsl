static float _2188;
static uint _3336;
static float _3337;
static float _3338;
static uint _3339;
static float _3340;
static float _3341;
static float _3342;
static float _3343;
static float _3344;
static float _3345;
static float _3346;
static uint _3347;
static uint _3348;
static float _3349;
static float _3350;
static float _3351;
static float _3352;
static float _3353;
static uint _3354;
static float _3355;
static float _3361;
static uint _3362;
static float _3363;
static float _3368;
static float _3369;
static float _3370;

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

static RayQuery<RAY_FLAG_NONE> _2837;

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
                        float _1150;
                        if (_801)
                        {
                            _1150 = _797;
                        }
                        else
                        {
                            _1150 = float(_594 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1272;
                        if ((_433 & 134217728u) == 0u)
                        {
                            uint frontier_phi_46_40_ladder;
                            if ((_595 != 0u) || ((_433 & 17825792u) == 1048576u))
                            {
                                frontier_phi_46_40_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_46_40_ladder = _791;
                            }
                            _1272 = frontier_phi_46_40_ladder;
                        }
                        else
                        {
                            _1272 = _791;
                        }
                        uint4 _1275 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _1277 = _1275.x;
                        float _1401;
                        float _1403;
                        float _1405;
                        if (_790 == 0u)
                        {
                            _1401 = 0.0f;
                            _1403 = 0.0f;
                            _1405 = 0.0f;
                        }
                        else
                        {
                            float4 _1410 = _20[8u].Load(int3(uint2(_270, _271), 0u));
                            _1401 = _1410.x;
                            _1403 = _1410.y;
                            _1405 = _1410.z;
                        }
                        uint _1469;
                        if (_1272 == 0u)
                        {
                            _1469 = 0u;
                        }
                        else
                        {
                            _1469 = _24[9u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        uint _1700;
                        if (_792 == 0u)
                        {
                            _1700 = 0u;
                        }
                        else
                        {
                            _1700 = _24[10u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        float _1710 = (float((_1277 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1711 = (float(_1277 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1715 = (1.0f - abs(_1710)) - abs(_1711);
                        float _1717 = clamp((-0.0f) - _1715, 0.0f, 1.0f);
                        float _1718 = (-0.0f) - _1717;
                        float _1723 = ((_1710 >= 0.0f) ? _1718 : _1717) + _1710;
                        float _1724 = ((_1711 >= 0.0f) ? _1718 : _1717) + _1711;
                        float _1728 = rsqrt(dot(float3(_1723, _1724, _1715), float3(_1723, _1724, _1715)));
                        float _1729 = _1723 * _1728;
                        float _1730 = _1724 * _1728;
                        float _1731 = _1728 * _1715;
                        float _803 = float(_1277 & 255u);
                        float _1735 = ((_433 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1908;
                        float _1909;
                        float _1910;
                        float _1911;
                        uint _1912;
                        if ((_385 & 64u) == 0u)
                        {
                            uint frontier_phi_92_79_ladder;
                            float frontier_phi_92_79_ladder_1;
                            float frontier_phi_92_79_ladder_2;
                            float frontier_phi_92_79_ladder_3;
                            float frontier_phi_92_79_ladder_4;
                            if ((_433 & 276824064u) == 0u)
                            {
                                frontier_phi_92_79_ladder = 0u;
                                frontier_phi_92_79_ladder_1 = 0.0f;
                                frontier_phi_92_79_ladder_2 = 0.0f;
                                frontier_phi_92_79_ladder_3 = 0.0f;
                                frontier_phi_92_79_ladder_4 = ((_433 & 8u) != 0u) ? _1403 : _1735;
                            }
                            else
                            {
                                frontier_phi_92_79_ladder = 0u;
                                frontier_phi_92_79_ladder_1 = 0.0f;
                                frontier_phi_92_79_ladder_2 = 0.0f;
                                frontier_phi_92_79_ladder_3 = 0.0f;
                                frontier_phi_92_79_ladder_4 = _1735;
                            }
                            _1908 = frontier_phi_92_79_ladder_4;
                            _1909 = frontier_phi_92_79_ladder_3;
                            _1910 = frontier_phi_92_79_ladder_2;
                            _1911 = frontier_phi_92_79_ladder_1;
                            _1912 = frontier_phi_92_79_ladder;
                        }
                        else
                        {
                            float _1808 = (_1403 * 2.0f) + (-1.0f);
                            float _1809 = (_1405 * 2.0f) + (-1.0f);
                            float _1813 = (1.0f - abs(_1808)) - abs(_1809);
                            float _1815 = clamp((-0.0f) - _1813, 0.0f, 1.0f);
                            float _1816 = (-0.0f) - _1815;
                            float _1821 = ((_1808 >= 0.0f) ? _1816 : _1815) + _1808;
                            float _1822 = ((_1809 >= 0.0f) ? _1816 : _1815) + _1809;
                            float _1826 = rsqrt(dot(float3(_1821, _1822, _1813), float3(_1821, _1822, _1813)));
                            _1908 = floor(round(_1401 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1909 = _1821 * _1826;
                            _1910 = _1822 * _1826;
                            _1911 = _1826 * _1813;
                            _1912 = 1u;
                        }
                        float _811;
                        if ((_433 & 32768u) == 0u)
                        {
                            _811 = _1908;
                        }
                        else
                        {
                            float frontier_phi_107_108_ladder;
                            if (_17.Load((_389 * 115u) + 36u).x == 0u)
                            {
                                float _2307 = clamp((_803 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _584;
                                frontier_phi_107_108_ladder = ((_433 & 131072u) != 0u) ? _2307 : ((((clamp((1.21000003814697265625f / (exp2((_1150 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_389 * 115u) + 32u).x)) + 1.0f) * _2307);
                            }
                            else
                            {
                                frontier_phi_107_108_ladder = _584;
                            }
                            _811 = frontier_phi_107_108_ladder;
                        }
                        uint _2090 = _432 & 1u;
                        float _2250;
                        float _2252;
                        float _2254;
                        uint _2256;
                        if (((_433 & 16u) == 0u) || (((_2090 | (_385 & 8u)) | (_433 & 16777216u)) != 0u))
                        {
                            _2250 = _1909;
                            _2252 = _1910;
                            _2254 = _1911;
                            _2256 = _1912;
                        }
                        else
                        {
                            float _2266 = (float(_1469 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2267 = (float(_1469 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2271 = (1.0f - abs(_2266)) - abs(_2267);
                            float _2273 = clamp((-0.0f) - _2271, 0.0f, 1.0f);
                            float _2274 = (-0.0f) - _2273;
                            float _2279 = ((_2266 >= 0.0f) ? _2274 : _2273) + _2266;
                            float _2280 = ((_2267 >= 0.0f) ? _2274 : _2273) + _2267;
                            float _2284 = rsqrt(dot(float3(_2279, _2280, _2271), float3(_2279, _2280, _2271)));
                            _2250 = _2279 * _2284;
                            _2252 = _2280 * _2284;
                            _2254 = _2284 * _2271;
                            _2256 = 1u;
                        }
                        float _805;
                        float _807;
                        float _809;
                        if (_2090 == 0u)
                        {
                            float frontier_phi_137_136_ladder;
                            float frontier_phi_137_136_ladder_1;
                            float frontier_phi_137_136_ladder_2;
                            if (((_432 & 64u) == 0u) && (_793 != 0u))
                            {
                                float2 _2527 = spvUnpackHalf2x16((_1700 >> 17u) & 32736u);
                                float _2528 = _2527.x;
                                float _2531 = (spvUnpackHalf2x16((_1700 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2532 = (spvUnpackHalf2x16((_1700 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2536 = (1.0f - abs(_2531)) - abs(_2532);
                                float _2538 = clamp((-0.0f) - _2536, 0.0f, 1.0f);
                                float _2539 = (-0.0f) - _2538;
                                float _2544 = ((_2531 >= 0.0f) ? _2539 : _2538) + _2531;
                                float _2545 = ((_2532 >= 0.0f) ? _2539 : _2538) + _2532;
                                float _2549 = rsqrt(dot(float3(_2544, _2545, _2536), float3(_2544, _2545, _2536)));
                                float _2559 = (((_2544 * _2549) - _1729) * _2528) + _1729;
                                float _2560 = (((_2545 * _2549) - _1730) * _2528) + _1730;
                                float _2561 = (((_2549 * _2536) - _1731) * _2528) + _1731;
                                float _2565 = rsqrt(dot(float3(_2559, _2560, _2561), float3(_2559, _2560, _2561)));
                                frontier_phi_137_136_ladder = _2561 * _2565;
                                frontier_phi_137_136_ladder_1 = _2560 * _2565;
                                frontier_phi_137_136_ladder_2 = _2559 * _2565;
                            }
                            else
                            {
                                frontier_phi_137_136_ladder = _1731;
                                frontier_phi_137_136_ladder_1 = _1730;
                                frontier_phi_137_136_ladder_2 = _1729;
                            }
                            _805 = frontier_phi_137_136_ladder_2;
                            _807 = frontier_phi_137_136_ladder_1;
                            _809 = frontier_phi_137_136_ladder;
                        }
                        else
                        {
                            _805 = _1729;
                            _807 = _1730;
                            _809 = _1731;
                        }
                        float _819;
                        float _821;
                        float _823;
                        float _825;
                        if (_801)
                        {
                            float frontier_phi_149_148_ladder;
                            float frontier_phi_149_148_ladder_1;
                            float frontier_phi_149_148_ladder_2;
                            float frontier_phi_149_148_ladder_3;
                            if (((_433 & 33554432u) == 0u) || (((_385 & 4u) != 0u) && ((_433 & 8388608u) == 0u)))
                            {
                                frontier_phi_149_148_ladder = 0.0f;
                                frontier_phi_149_148_ladder_1 = 0.0f;
                                frontier_phi_149_148_ladder_2 = 0.0f;
                                frontier_phi_149_148_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2698 = (spvUnpackHalf2x16((_1700 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2699 = (spvUnpackHalf2x16((_1700 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2703 = (1.0f - abs(_2698)) - abs(_2699);
                                float _2705 = clamp((-0.0f) - _2703, 0.0f, 1.0f);
                                float _2706 = (-0.0f) - _2705;
                                float _2711 = ((_2698 >= 0.0f) ? _2706 : _2705) + _2698;
                                float _2712 = ((_2699 >= 0.0f) ? _2706 : _2705) + _2699;
                                float _2716 = rsqrt(dot(float3(_2711, _2712, _2703), float3(_2711, _2712, _2703)));
                                float _2717 = _2711 * _2716;
                                float _2718 = _2712 * _2716;
                                float _2719 = _2716 * _2703;
                                float _2723 = rsqrt(dot(float3(_2717, _2718, _2719), float3(_2717, _2718, _2719)));
                                frontier_phi_149_148_ladder = _2723 * _2719;
                                frontier_phi_149_148_ladder_1 = _2723 * _2718;
                                frontier_phi_149_148_ladder_2 = _2723 * _2717;
                                frontier_phi_149_148_ladder_3 = spvUnpackHalf2x16((_1700 >> 17u) & 32736u).x;
                            }
                            _819 = frontier_phi_149_148_ladder_3;
                            _821 = frontier_phi_149_148_ladder_2;
                            _823 = frontier_phi_149_148_ladder_1;
                            _825 = frontier_phi_149_148_ladder;
                        }
                        else
                        {
                            _819 = 0.0f;
                            _821 = 0.0f;
                            _823 = 0.0f;
                            _825 = 0.0f;
                        }
                        bool _2579 = _2256 != 0u;
                        _802 = _803;
                        _804 = _805;
                        _806 = _807;
                        _808 = _809;
                        _810 = _811;
                        _812 = _2579 ? _2250 : _805;
                        _814 = _2579 ? _2252 : _807;
                        _816 = _2579 ? _2254 : _809;
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
                    bool _1159 = ((_432 & 128u) | _442) != 0u;
                    uint _1171;
                    if (_1159)
                    {
                        _1171 = 1u;
                    }
                    else
                    {
                        _1171 = (((_433 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _397;
                    float _399;
                    float _401;
                    uint _403;
                    float _407;
                    if (((_433 & 33554432u) == 0u) || _1159)
                    {
                        bool _1279 = _77 != 0u;
                        uint _1286;
                        if ((_433 & 16u) == 0u)
                        {
                            _1286 = _1171;
                        }
                        else
                        {
                            _1286 = ((_433 & 268435456u) != 0u) ? _1171 : 2u;
                        }
                        _407 = _810 * _826;
                        _397 = _1279 ? _812 : _804;
                        _399 = _1279 ? _814 : _806;
                        _401 = _1279 ? _816 : _808;
                        _403 = _1286;
                    }
                    else
                    {
                        _407 = _818;
                        _397 = _820;
                        _399 = _822;
                        _401 = _824;
                        _403 = _1171;
                    }
                    uint _1287 = _403 + 102u;
                    float _1296 = clamp((_407 - _45_m0[_1287].x) / (_45_m0[_1287].y - _45_m0[_1287].x), 0.0f, 1.0f);
                    _396 = _397;
                    _398 = _399;
                    _400 = _401;
                    _402 = _403;
                    _404 = (_1296 * _1296) * (3.0f - (_1296 * 2.0f));
                    _406 = _407;
                    _408 = _45_m0[_1287].z;
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
                float _1028 = _50_m0[2u].w / dot(float3(_929, _930, _931), float3(_521, _524, _527));
                float _1032 = (_1028 * _929) + _953;
                float _1033 = (_1028 * _930) + _954;
                float _1034 = (_1028 * _931) + _955;
                float _1050 = mad(_116, _1034, mad(_109, _1033, _1032 * _102)) + _123;
                float _1059 = _50_m0[51u].x * (((((mad(_113, _1034, mad(_106, _1033, _1032 * _99)) + _120) / _1050) * 0.5f) + 0.5f) - _977);
                float _1061 = _50_m0[51u].y * ((0.5f - (((mad(_114, _1034, mad(_107, _1033, _1032 * _100)) + _121) / _1050) * 0.5f)) - _978);
                float _1062 = ((mad(_115, _1034, mad(_108, _1033, _1032 * _101)) + _122) / _1050) - _974;
                float _1075 = sqrt(((_1059 * _1059) + (_1062 * _1062)) + (_1061 * _1061)) / sqrt(((_1018 * _1018) + (_1021 * _1021)) + (_1020 * _1020));
                float _1076 = float(_238);
                float _1077 = float(_239);
                float _1084 = (_1018 != 0.0f) ? (0.100000001490116119384765625f / _1014) : 3.4028234663852885981170418348452e+38f;
                float _1086 = (_1020 != 0.0f) ? (0.100000001490116119384765625f / _1016) : 3.4028234663852885981170418348452e+38f;
                float _1087 = (_1021 != 0.0f) ? (0.100000001490116119384765625f / _1017) : 3.4028234663852885981170418348452e+38f;
                float _1088 = 1.0f / _1076;
                float _1089 = 1.0f / _1077;
                float _1090 = 0.004999999888241291046142578125f / _1076;
                float _1092 = 0.004999999888241291046142578125f / _1077;
                float _1101 = float(_1018 >= 0.0f);
                float _1102 = float(_1020 >= 0.0f);
                float _1111 = ((_1018 < 0.0f) ? ((-0.0f) - _1090) : _1090) - _981;
                float _1114 = ((_1020 < 0.0f) ? ((-0.0f) - _1092) : _1092) - _982;
                float _1117 = min((((floor(_981 * _1076) + _1101) * _1088) + _1111) * _1084, (((floor(_982 * _1077) + _1102) * _1089) + _1114) * _1086);
                float _1121 = (_1117 * _1018) + _981;
                float _1122 = (_1117 * _1020) + _982;
                float _1123 = (_1117 * _1021) + _974;
                float _1126 = _50_m0[50u].x / (_463 - (_1123 * _50_m0[50u].y));
                float _1136 = 1.0f / max(0.00999999977648258209228515625f, max(abs(_1014 * 38400.0f), abs(_1016 * 21600.0f)));
                float _1144 = max(_50_m0[3u].z, _50_m0[5u].z * _1136);
                float _1160;
                if (asuint(_50_m0[5u]).y == 0u)
                {
                    _1160 = _1144;
                }
                else
                {
                    _1160 = min(_1144, _50_m0[5u].w * _1136);
                }
                uint _1179;
                float _1181;
                float _1183;
                float _1185;
                uint _1187;
                float _1189;
                float _1191;
                float _1193;
                float _1195;
                float _1197;
                float _1199;
                float _1201;
                uint _1203;
                float _1205;
                float _1207;
                uint _1209;
                if (_209 == 0u)
                {
                    _1179 = 0u;
                    _1181 = _1123;
                    _1183 = _1122;
                    _1185 = _1121;
                    _1187 = 0u;
                    _1189 = _974;
                    _1191 = _982;
                    _1193 = _981;
                    _1195 = _1123;
                    _1197 = _1122;
                    _1199 = _1121;
                    _1201 = 1.0f;
                    _1203 = 0u;
                    _1205 = 0.0f;
                    _1207 = 0.0f;
                    _1209 = 1u;
                }
                else
                {
                    uint _1180;
                    float _1182;
                    float _1184;
                    float _1186;
                    uint _1188;
                    uint _1383;
                    float _1190;
                    float _1192;
                    float _1194;
                    float _1196;
                    float _1198;
                    float _1200;
                    float _1202;
                    uint _1204;
                    float _1206;
                    float _1208;
                    uint _1210;
                    float _1366;
                    uint _1368;
                    float _1373;
                    float _1375;
                    float _1377;
                    float _1379;
                    float _1381;
                    uint _1364 = 0u;
                    float _1365 = _1160;
                    uint _1367 = 0u;
                    float _1369 = _1123;
                    float _1370 = _1122;
                    float _1371 = _1121;
                    float _1372 = _1117;
                    float _1374 = _1089;
                    float _1376 = _1088;
                    float _1378 = _1077;
                    float _1380 = _1076;
                    uint _1382 = 0u;
                    uint _1384 = 0u;
                    float _1385 = _974;
                    float _1386 = _982;
                    float _1387 = _981;
                    float _1388 = _1123;
                    float _1389 = _1122;
                    float _1390 = _1121;
                    float _1391 = 1.0f;
                    uint _1392 = 0u;
                    float _1393 = 0.0f;
                    float _1394 = 0.0f;
                    uint _1395 = 1u;
                    float _1396;
                    float _1397;
                    uint _1398;
                    uint _1399;
                    bool _1400;
                    for (;;)
                    {
                        _1396 = _1380 * _1371;
                        _1397 = _1378 * _1370;
                        _1398 = uint(int(_1396));
                        _1399 = uint(int(_1397));
                        _1400 = _1382 == 0u;
                        float _1656;
                        if (_1400)
                        {
                            _1656 = _12.Load(int3(uint2(_1398, _1399), 0u)).x;
                        }
                        else
                        {
                            _1656 = _15.Load(int3(uint2(_1398, _1399), _1382 + 4294967295u)).x;
                        }
                        float _1662 = ((_1396 >= floor(_1380)) || (_1397 >= floor(_1378))) ? 1.0f : _1656;
                        float _1676 = (_1021 < 0.0f) ? ((_1662 - _974) * _1087) : 3.4028234663852885981170418348452e+38f;
                        float _1678 = min(min((((floor(_1396) + _1101) * _1376) + _1111) * _1084, (((floor(_1397) + _1102) * _1374) + _1114) * _1086), _1676);
                        bool _1679 = _1662 < _1369;
                        bool _1683 = _1679 && (asuint(_1678) != asuint(_1676));
                        float _1684 = _1679 ? _1678 : _1372;
                        float _1688 = (_1684 * _1018) + _981;
                        float _1689 = (_1684 * _1020) + _982;
                        float _1690 = (_1684 * _1021) + _974;
                        uint _1692 = (_1683 ? 1u : 4294967295u) + _1382;
                        float _1693 = _1683 ? 0.5f : 2.0f;
                        float _1694 = _1693 * _1380;
                        float _1695 = _1693 * _1378;
                        float _1696 = _1683 ? 2.0f : 0.5f;
                        float _1697 = _1696 * _1376;
                        float _1698 = _1696 * _1374;
                        _1180 = _1364 + 1u;
                        uint _1893;
                        uint _1896;
                        if (int(_1692) < int(0u))
                        {
                            float frontier_phi_90_77_ladder;
                            uint frontier_phi_90_77_ladder_1;
                            float frontier_phi_90_77_ladder_2;
                            uint frontier_phi_90_77_ladder_3;
                            float frontier_phi_90_77_ladder_4;
                            float frontier_phi_90_77_ladder_5;
                            uint frontier_phi_90_77_ladder_6;
                            float frontier_phi_90_77_ladder_7;
                            float frontier_phi_90_77_ladder_8;
                            float frontier_phi_90_77_ladder_9;
                            float frontier_phi_90_77_ladder_10;
                            float frontier_phi_90_77_ladder_11;
                            float frontier_phi_90_77_ladder_12;
                            uint frontier_phi_90_77_ladder_13;
                            uint frontier_phi_90_77_ladder_14;
                            float frontier_phi_90_77_ladder_15;
                            float frontier_phi_90_77_ladder_16;
                            float frontier_phi_90_77_ladder_17;
                            float frontier_phi_90_77_ladder_18;
                            float frontier_phi_90_77_ladder_19;
                            float _1772;
                            float4 _1777;
                            float _1780;
                            float _1783;
                            bool _1784;
                            for (;;)
                            {
                                float _1770 = _50_m0[50u].w + _50_m0[50u].y;
                                _1772 = _50_m0[50u].x / (_1770 - (_50_m0[50u].y * _1662));
                                float _1775 = _50_m0[50u].x / (_1770 - (_50_m0[50u].y * _1690));
                                _1777 = _50_m0[3u];
                                _1780 = abs(_1126 - _1775);
                                _1783 = _1775 - _1772;
                                _1784 = _1783 > max(_1777.x, _1777.x * _1780);
                                if (_1784)
                                {
                                    uint _1898;
                                    if (_1367 == 0u)
                                    {
                                        uint frontier_phi_103_102_ladder;
                                        if ((_402 == 2u) || (_402 == 4u))
                                        {
                                            if ((_1684 < _1075) && (abs(_1783) < _50_m0[2u].z))
                                            {
                                                frontier_phi_90_77_ladder = _1365;
                                                frontier_phi_90_77_ladder_1 = 1u;
                                                frontier_phi_90_77_ladder_2 = _1684;
                                                frontier_phi_90_77_ladder_3 = _1395;
                                                frontier_phi_90_77_ladder_4 = _1688;
                                                frontier_phi_90_77_ladder_5 = _1689;
                                                frontier_phi_90_77_ladder_6 = 1u;
                                                frontier_phi_90_77_ladder_7 = 0.0f;
                                                frontier_phi_90_77_ladder_8 = _1390;
                                                frontier_phi_90_77_ladder_9 = _1389;
                                                frontier_phi_90_77_ladder_10 = _1388;
                                                frontier_phi_90_77_ladder_11 = _1387;
                                                frontier_phi_90_77_ladder_12 = _1385;
                                                frontier_phi_90_77_ladder_13 = 1u;
                                                frontier_phi_90_77_ladder_14 = _1692;
                                                frontier_phi_90_77_ladder_15 = _1694;
                                                frontier_phi_90_77_ladder_16 = _1695;
                                                frontier_phi_90_77_ladder_17 = _1697;
                                                frontier_phi_90_77_ladder_18 = _1698;
                                                frontier_phi_90_77_ladder_19 = _1386;
                                                break;
                                            }
                                            frontier_phi_103_102_ladder = 1u;
                                        }
                                        else
                                        {
                                            frontier_phi_103_102_ladder = 1u;
                                        }
                                        _1898 = frontier_phi_103_102_ladder;
                                    }
                                    else
                                    {
                                        _1898 = _1367;
                                    }
                                    if (!(_1392 == 0u))
                                    {
                                        frontier_phi_90_77_ladder = _1365;
                                        frontier_phi_90_77_ladder_1 = _1898;
                                        frontier_phi_90_77_ladder_2 = _1684;
                                        frontier_phi_90_77_ladder_3 = _1395;
                                        frontier_phi_90_77_ladder_4 = _1394;
                                        frontier_phi_90_77_ladder_5 = _1393;
                                        frontier_phi_90_77_ladder_6 = _1392;
                                        frontier_phi_90_77_ladder_7 = _1391;
                                        frontier_phi_90_77_ladder_8 = _1390;
                                        frontier_phi_90_77_ladder_9 = _1389;
                                        frontier_phi_90_77_ladder_10 = _1388;
                                        frontier_phi_90_77_ladder_11 = _1387;
                                        frontier_phi_90_77_ladder_12 = _1385;
                                        frontier_phi_90_77_ladder_13 = _1384;
                                        frontier_phi_90_77_ladder_14 = _1692;
                                        frontier_phi_90_77_ladder_15 = _1694;
                                        frontier_phi_90_77_ladder_16 = _1695;
                                        frontier_phi_90_77_ladder_17 = _1697;
                                        frontier_phi_90_77_ladder_18 = _1698;
                                        frontier_phi_90_77_ladder_19 = _1386;
                                        break;
                                    }
                                    float _1897 = _1684 + _1365;
                                    bool _2226 = _1384 != 0u;
                                    float _1891 = _2226 ? _1394 : _1688;
                                    float _1892 = _2226 ? _1393 : _1689;
                                    uint _1894 = ((_402 == 1u) || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1384;
                                    if (asuint(_50_m0[5u]).y == 0u)
                                    {
                                        frontier_phi_90_77_ladder = _1365;
                                        frontier_phi_90_77_ladder_1 = _1898;
                                        frontier_phi_90_77_ladder_2 = _1897;
                                        frontier_phi_90_77_ladder_3 = _1395;
                                        frontier_phi_90_77_ladder_4 = _1891;
                                        frontier_phi_90_77_ladder_5 = _1892;
                                        frontier_phi_90_77_ladder_6 = 0u;
                                        frontier_phi_90_77_ladder_7 = _1391;
                                        frontier_phi_90_77_ladder_8 = _1390;
                                        frontier_phi_90_77_ladder_9 = _1389;
                                        frontier_phi_90_77_ladder_10 = _1388;
                                        frontier_phi_90_77_ladder_11 = _1387;
                                        frontier_phi_90_77_ladder_12 = _1385;
                                        frontier_phi_90_77_ladder_13 = _1894;
                                        frontier_phi_90_77_ladder_14 = 0u;
                                        frontier_phi_90_77_ladder_15 = _1076;
                                        frontier_phi_90_77_ladder_16 = _1077;
                                        frontier_phi_90_77_ladder_17 = _1088;
                                        frontier_phi_90_77_ladder_18 = _1089;
                                        frontier_phi_90_77_ladder_19 = _1386;
                                        break;
                                    }
                                    frontier_phi_90_77_ladder = min(_1144, _50_m0[6u].x * _1365);
                                    frontier_phi_90_77_ladder_1 = _1898;
                                    frontier_phi_90_77_ladder_2 = _1897;
                                    frontier_phi_90_77_ladder_3 = _1395;
                                    frontier_phi_90_77_ladder_4 = _1891;
                                    frontier_phi_90_77_ladder_5 = _1892;
                                    frontier_phi_90_77_ladder_6 = 0u;
                                    frontier_phi_90_77_ladder_7 = _1391;
                                    frontier_phi_90_77_ladder_8 = _1390;
                                    frontier_phi_90_77_ladder_9 = _1389;
                                    frontier_phi_90_77_ladder_10 = _1388;
                                    frontier_phi_90_77_ladder_11 = _1387;
                                    frontier_phi_90_77_ladder_12 = _1385;
                                    frontier_phi_90_77_ladder_13 = _1894;
                                    frontier_phi_90_77_ladder_14 = 0u;
                                    frontier_phi_90_77_ladder_15 = _1076;
                                    frontier_phi_90_77_ladder_16 = _1077;
                                    frontier_phi_90_77_ladder_17 = _1088;
                                    frontier_phi_90_77_ladder_18 = _1089;
                                    frontier_phi_90_77_ladder_19 = _1386;
                                    break;
                                }
                                else
                                {
                                    float _1880 = max(_1777.y, _1777.y * _1780);
                                    float _1883 = _1880 * _1777.w;
                                    float _1887 = clamp((abs(_1783) - _1883) / (_1880 - _1883), 0.0f, 1.0f);
                                    uint _1889 = uint(_1772 < _1126);
                                    float frontier_phi_90_77_ladder_89_ladder;
                                    uint frontier_phi_90_77_ladder_89_ladder_1;
                                    float frontier_phi_90_77_ladder_89_ladder_2;
                                    uint frontier_phi_90_77_ladder_89_ladder_3;
                                    float frontier_phi_90_77_ladder_89_ladder_4;
                                    float frontier_phi_90_77_ladder_89_ladder_5;
                                    uint frontier_phi_90_77_ladder_89_ladder_6;
                                    float frontier_phi_90_77_ladder_89_ladder_7;
                                    float frontier_phi_90_77_ladder_89_ladder_8;
                                    float frontier_phi_90_77_ladder_89_ladder_9;
                                    float frontier_phi_90_77_ladder_89_ladder_10;
                                    float frontier_phi_90_77_ladder_89_ladder_11;
                                    float frontier_phi_90_77_ladder_89_ladder_12;
                                    uint frontier_phi_90_77_ladder_89_ladder_13;
                                    uint frontier_phi_90_77_ladder_89_ladder_14;
                                    float frontier_phi_90_77_ladder_89_ladder_15;
                                    float frontier_phi_90_77_ladder_89_ladder_16;
                                    float frontier_phi_90_77_ladder_89_ladder_17;
                                    float frontier_phi_90_77_ladder_89_ladder_18;
                                    float frontier_phi_90_77_ladder_89_ladder_19;
                                    if (_1384 == 0u)
                                    {
                                        frontier_phi_90_77_ladder_89_ladder = _1365;
                                        frontier_phi_90_77_ladder_89_ladder_1 = _1367;
                                        frontier_phi_90_77_ladder_89_ladder_2 = _1684;
                                        frontier_phi_90_77_ladder_89_ladder_3 = _1889;
                                        frontier_phi_90_77_ladder_89_ladder_4 = _1394;
                                        frontier_phi_90_77_ladder_89_ladder_5 = _1393;
                                        frontier_phi_90_77_ladder_89_ladder_6 = _1392;
                                        frontier_phi_90_77_ladder_89_ladder_7 = _1887;
                                        frontier_phi_90_77_ladder_89_ladder_8 = _1390;
                                        frontier_phi_90_77_ladder_89_ladder_9 = _1389;
                                        frontier_phi_90_77_ladder_89_ladder_10 = _1388;
                                        frontier_phi_90_77_ladder_89_ladder_11 = _1387;
                                        frontier_phi_90_77_ladder_89_ladder_12 = _1385;
                                        frontier_phi_90_77_ladder_89_ladder_13 = uint(_1887 > 0.0f);
                                        frontier_phi_90_77_ladder_89_ladder_14 = _1692;
                                        frontier_phi_90_77_ladder_89_ladder_15 = _1694;
                                        frontier_phi_90_77_ladder_89_ladder_16 = _1695;
                                        frontier_phi_90_77_ladder_89_ladder_17 = _1697;
                                        frontier_phi_90_77_ladder_89_ladder_18 = _1698;
                                        frontier_phi_90_77_ladder_89_ladder_19 = _1386;
                                    }
                                    else
                                    {
                                        frontier_phi_90_77_ladder_89_ladder = _1365;
                                        frontier_phi_90_77_ladder_89_ladder_1 = _1367;
                                        frontier_phi_90_77_ladder_89_ladder_2 = _1684;
                                        frontier_phi_90_77_ladder_89_ladder_3 = _1889;
                                        frontier_phi_90_77_ladder_89_ladder_4 = _1394;
                                        frontier_phi_90_77_ladder_89_ladder_5 = _1393;
                                        frontier_phi_90_77_ladder_89_ladder_6 = _1392;
                                        frontier_phi_90_77_ladder_89_ladder_7 = _1887;
                                        frontier_phi_90_77_ladder_89_ladder_8 = _1390;
                                        frontier_phi_90_77_ladder_89_ladder_9 = _1389;
                                        frontier_phi_90_77_ladder_89_ladder_10 = _1388;
                                        frontier_phi_90_77_ladder_89_ladder_11 = _1387;
                                        frontier_phi_90_77_ladder_89_ladder_12 = _1385;
                                        frontier_phi_90_77_ladder_89_ladder_13 = _1384;
                                        frontier_phi_90_77_ladder_89_ladder_14 = _1692;
                                        frontier_phi_90_77_ladder_89_ladder_15 = _1694;
                                        frontier_phi_90_77_ladder_89_ladder_16 = _1695;
                                        frontier_phi_90_77_ladder_89_ladder_17 = _1697;
                                        frontier_phi_90_77_ladder_89_ladder_18 = _1698;
                                        frontier_phi_90_77_ladder_89_ladder_19 = _1386;
                                    }
                                    frontier_phi_90_77_ladder = frontier_phi_90_77_ladder_89_ladder;
                                    frontier_phi_90_77_ladder_1 = frontier_phi_90_77_ladder_89_ladder_1;
                                    frontier_phi_90_77_ladder_2 = frontier_phi_90_77_ladder_89_ladder_2;
                                    frontier_phi_90_77_ladder_3 = frontier_phi_90_77_ladder_89_ladder_3;
                                    frontier_phi_90_77_ladder_4 = frontier_phi_90_77_ladder_89_ladder_4;
                                    frontier_phi_90_77_ladder_5 = frontier_phi_90_77_ladder_89_ladder_5;
                                    frontier_phi_90_77_ladder_6 = frontier_phi_90_77_ladder_89_ladder_6;
                                    frontier_phi_90_77_ladder_7 = frontier_phi_90_77_ladder_89_ladder_7;
                                    frontier_phi_90_77_ladder_8 = frontier_phi_90_77_ladder_89_ladder_8;
                                    frontier_phi_90_77_ladder_9 = frontier_phi_90_77_ladder_89_ladder_9;
                                    frontier_phi_90_77_ladder_10 = frontier_phi_90_77_ladder_89_ladder_10;
                                    frontier_phi_90_77_ladder_11 = frontier_phi_90_77_ladder_89_ladder_11;
                                    frontier_phi_90_77_ladder_12 = frontier_phi_90_77_ladder_89_ladder_12;
                                    frontier_phi_90_77_ladder_13 = frontier_phi_90_77_ladder_89_ladder_13;
                                    frontier_phi_90_77_ladder_14 = frontier_phi_90_77_ladder_89_ladder_14;
                                    frontier_phi_90_77_ladder_15 = frontier_phi_90_77_ladder_89_ladder_15;
                                    frontier_phi_90_77_ladder_16 = frontier_phi_90_77_ladder_89_ladder_16;
                                    frontier_phi_90_77_ladder_17 = frontier_phi_90_77_ladder_89_ladder_17;
                                    frontier_phi_90_77_ladder_18 = frontier_phi_90_77_ladder_89_ladder_18;
                                    frontier_phi_90_77_ladder_19 = frontier_phi_90_77_ladder_89_ladder_19;
                                    break;
                                }
                            }
                            _1210 = frontier_phi_90_77_ladder_3;
                            _1208 = frontier_phi_90_77_ladder_4;
                            _1206 = frontier_phi_90_77_ladder_5;
                            _1204 = frontier_phi_90_77_ladder_6;
                            _1202 = frontier_phi_90_77_ladder_7;
                            _1200 = frontier_phi_90_77_ladder_8;
                            _1198 = frontier_phi_90_77_ladder_9;
                            _1196 = frontier_phi_90_77_ladder_10;
                            _1194 = frontier_phi_90_77_ladder_11;
                            _1192 = frontier_phi_90_77_ladder_19;
                            _1190 = frontier_phi_90_77_ladder_12;
                            _1893 = frontier_phi_90_77_ladder_13;
                            _1896 = frontier_phi_90_77_ladder_14;
                            _1381 = frontier_phi_90_77_ladder_15;
                            _1379 = frontier_phi_90_77_ladder_16;
                            _1377 = frontier_phi_90_77_ladder_17;
                            _1375 = frontier_phi_90_77_ladder_18;
                            _1373 = frontier_phi_90_77_ladder_2;
                            _1368 = frontier_phi_90_77_ladder_1;
                            _1366 = frontier_phi_90_77_ladder;
                        }
                        else
                        {
                            bool _1785 = _1384 != 0u;
                            _1210 = _1395;
                            _1208 = _1394;
                            _1206 = _1393;
                            _1204 = _1392;
                            _1202 = _1391;
                            _1200 = _1785 ? _1390 : _1688;
                            _1198 = _1785 ? _1389 : _1689;
                            _1196 = _1785 ? _1388 : _1690;
                            _1194 = _1688;
                            _1192 = _1689;
                            _1190 = _1690;
                            _1893 = _1384;
                            _1896 = _1692;
                            _1381 = _1694;
                            _1379 = _1695;
                            _1377 = _1697;
                            _1375 = _1698;
                            _1373 = _1684;
                            _1368 = _1367;
                            _1366 = (asuint(_50_m0[5u]).y != 0u) ? _1160 : _1365;
                        }
                        float frontier_phi_122_pred;
                        uint frontier_phi_122_pred_1;
                        uint frontier_phi_122_pred_2;
                        float frontier_phi_122_pred_3;
                        float frontier_phi_122_pred_4;
                        bool _1902;
                        bool _1904;
                        for (;;)
                        {
                            _1902 = _1690 < 0.0f;
                            _1904 = _1902 || ((_1688 < 0.0f) || (_1689 < 0.0f));
                            if (!_1904)
                            {
                                if (!((_1690 > 1.0f) || ((_1688 > _50_m0[51u].x) || (_1689 > _50_m0[51u].y))))
                                {
                                    frontier_phi_122_pred = _1688;
                                    frontier_phi_122_pred_1 = _1893;
                                    frontier_phi_122_pred_2 = _1896;
                                    frontier_phi_122_pred_3 = _1689;
                                    frontier_phi_122_pred_4 = _1690;
                                    break;
                                }
                            }
                            if (!_1902)
                            {
                                frontier_phi_122_pred = _1688;
                                frontier_phi_122_pred_1 = 1u;
                                frontier_phi_122_pred_2 = 4294967295u;
                                frontier_phi_122_pred_3 = _1689;
                                frontier_phi_122_pred_4 = _1690;
                                break;
                            }
                            float _2240 = (-0.0f) - _1690;
                            float _2241 = _2240 / _1021;
                            frontier_phi_122_pred = (_2241 * _1018) + _1688;
                            frontier_phi_122_pred_1 = 1u;
                            frontier_phi_122_pred_2 = 4294967295u;
                            frontier_phi_122_pred_3 = (_2241 * _1020) + _1689;
                            frontier_phi_122_pred_4 = _2240 + _1690;
                            break;
                        }
                        _1186 = frontier_phi_122_pred;
                        _1188 = frontier_phi_122_pred_1;
                        _1383 = frontier_phi_122_pred_2;
                        _1184 = frontier_phi_122_pred_3;
                        _1182 = frontier_phi_122_pred_4;
                        if ((_1180 < _209) && (int(_1383) > int(4294967295u)))
                        {
                            _1364 = _1180;
                            _1365 = _1366;
                            _1367 = _1368;
                            _1369 = _1182;
                            _1370 = _1184;
                            _1371 = _1186;
                            _1372 = _1373;
                            _1374 = _1375;
                            _1376 = _1377;
                            _1378 = _1379;
                            _1380 = _1381;
                            _1382 = _1383;
                            _1384 = _1188;
                            _1385 = _1190;
                            _1386 = _1192;
                            _1387 = _1194;
                            _1388 = _1196;
                            _1389 = _1198;
                            _1390 = _1200;
                            _1391 = _1202;
                            _1392 = _1204;
                            _1393 = _1206;
                            _1394 = _1208;
                            _1395 = _1210;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    _1179 = _1180;
                    _1181 = _1182;
                    _1183 = _1184;
                    _1185 = _1186;
                    _1187 = _1188;
                    _1189 = _1190;
                    _1191 = _1192;
                    _1193 = _1194;
                    _1195 = _1196;
                    _1197 = _1198;
                    _1199 = _1200;
                    _1201 = _1202;
                    _1203 = _1204;
                    _1205 = _1206;
                    _1207 = _1208;
                    _1209 = _1210;
                }
                bool _1211 = _1179 >= _209;
                uint _1212 = _1211 ? 1u : _1187;
                float _1220 = _50_m0[51u].z * 2.0f;
                float _1223 = (_1220 * _981) + (-1.0f);
                float _1224 = ((1.0f - (_50_m0[51u].w * _982)) * 2.0f) + (-1.0f);
                float _1240 = mad(_190, _974, mad(_183, _1224, _1223 * _176)) + _197;
                float _1241 = (mad(_187, _974, mad(_180, _1224, _1223 * _173)) + _194) / _1240;
                float _1242 = (mad(_188, _974, mad(_181, _1224, _1223 * _174)) + _195) / _1240;
                float _1243 = (mad(_189, _974, mad(_182, _1224, _1223 * _175)) + _196) / _1240;
                float _1248 = (_1220 * _1185) + (-1.0f);
                float _1249 = ((1.0f - (_50_m0[51u].w * _1183)) * 2.0f) + (-1.0f);
                float _1265 = mad(_190, _1181, mad(_183, _1249, _1248 * _176)) + _197;
                float _1269 = ((mad(_187, _1181, mad(_180, _1249, _1248 * _173)) + _194) / _1265) - _1241;
                float _1270 = ((mad(_188, _1181, mad(_181, _1249, _1248 * _174)) + _195) / _1265) - _1242;
                float _1271 = ((mad(_189, _1181, mad(_182, _1249, _1248 * _175)) + _196) / _1265) - _1243;
                float _1301;
                uint _1303;
                float _1305;
                if (_1179 > _209)
                {
                    _1301 = 0.0f;
                    _1303 = _1212;
                    _1305 = 0.0f;
                }
                else
                {
                    float frontier_phi_49_50_ladder;
                    uint frontier_phi_49_50_ladder_1;
                    float frontier_phi_49_50_ladder_2;
                    if ((_1183 < 0.0f) || (_1185 < 0.0f))
                    {
                        frontier_phi_49_50_ladder = 0.0f;
                        frontier_phi_49_50_ladder_1 = _1212;
                        frontier_phi_49_50_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_49_50_ladder_58_ladder;
                        uint frontier_phi_49_50_ladder_58_ladder_1;
                        float frontier_phi_49_50_ladder_58_ladder_2;
                        if ((_1181 >= 1.0f) || ((_1185 > _50_m0[51u].x) || (_1183 > _50_m0[51u].y)))
                        {
                            frontier_phi_49_50_ladder_58_ladder = 0.0f;
                            frontier_phi_49_50_ladder_58_ladder_1 = _1212;
                            frontier_phi_49_50_ladder_58_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_49_50_ladder_58_ladder_67_ladder;
                            uint frontier_phi_49_50_ladder_58_ladder_67_ladder_1;
                            float frontier_phi_49_50_ladder_58_ladder_67_ladder_2;
                            for (;;)
                            {
                                if ((abs(_1185 - _259) < (2.0f / _1076)) && (abs(_1183 - _260) < (2.0f / _1077)))
                                {
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder = 0.0f;
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder_1 = _1212;
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_101;
                                    float frontier_phi_101_pred;
                                    uint frontier_phi_101_pred_1;
                                    float frontier_phi_101_pred_2;
                                    uint _1760;
                                    uint _1761;
                                    bool _1763;
                                    for (;;)
                                    {
                                        _1760 = uint(int(_1185 * _1076));
                                        _1761 = uint(int(_1183 * _1077));
                                        _1763 = (_402 == 1u) && _935;
                                        if (!_1763)
                                        {
                                            if (!(dot(float3(_1269, _1270, _1271), float3(_1269, _1270, _1271)) < _50_m0[4u].w))
                                            {
                                                ladder_phi_101 = false;
                                                frontier_phi_101_pred = 0.0f;
                                                frontier_phi_101_pred_1 = _1212;
                                                frontier_phi_101_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1866 = _24[22u].Load(int3(uint2(_1760, _1761), 0u));
                                        uint _1868 = _1866.x;
                                        float _2205;
                                        float _2206;
                                        float _2207;
                                        if (_1868 == 0u)
                                        {
                                            uint4 _1980 = _24[1u].Load(int3(uint2(_1760, _1761), 0u));
                                            uint _1982 = _1980.x;
                                            float _1990 = (float((_1982 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1991 = (float(_1982 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1995 = (1.0f - abs(_1990)) - abs(_1991);
                                            float _1997 = clamp((-0.0f) - _1995, 0.0f, 1.0f);
                                            float _1998 = (-0.0f) - _1997;
                                            _2205 = ((_1990 >= 0.0f) ? _1998 : _1997) + _1990;
                                            _2206 = ((_1991 >= 0.0f) ? _1998 : _1997) + _1991;
                                            _2207 = _1995;
                                        }
                                        else
                                        {
                                            float _2012 = (float((_1868 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2013 = (float(_1868 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2017 = (1.0f - abs(_2012)) - abs(_2013);
                                            float _2019 = clamp((-0.0f) - _2017, 0.0f, 1.0f);
                                            float _2020 = (-0.0f) - _2019;
                                            _2205 = ((_2012 >= 0.0f) ? _2020 : _2019) + _2012;
                                            _2206 = ((_2013 >= 0.0f) ? _2020 : _2019) + _2013;
                                            _2207 = _2017;
                                        }
                                        float _2211 = rsqrt(dot(float3(_2205, _2206, _2207), float3(_2205, _2206, _2207)));
                                        if (dot(float3(_2211 * _2205, _2211 * _2206, _2211 * _2207), float3(_1269, _1270, _1271)) > 0.0f)
                                        {
                                            ladder_phi_101 = true;
                                            frontier_phi_101_pred = 0.0f;
                                            frontier_phi_101_pred_1 = _1212;
                                            frontier_phi_101_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_101 = false;
                                            frontier_phi_101_pred = 0.0f;
                                            frontier_phi_101_pred_1 = _1212;
                                            frontier_phi_101_pred_2 = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_101)
                                    {
                                        frontier_phi_49_50_ladder_58_ladder_67_ladder = frontier_phi_101_pred;
                                        frontier_phi_49_50_ladder_58_ladder_67_ladder_1 = frontier_phi_101_pred_1;
                                        frontier_phi_49_50_ladder_58_ladder_67_ladder_2 = frontier_phi_101_pred_2;
                                        break;
                                    }
                                    float _2031 = _50_m0[51u].z * _1185;
                                    float _2032 = _50_m0[51u].w * _1183;
                                    float _2034 = (_1077 / _1076) * 0.0500000007450580596923828125f;
                                    float _2039 = clamp(_2031 / _2034, 0.0f, 1.0f);
                                    float _2040 = clamp(_2032 * 20.0f, 0.0f, 1.0f);
                                    float _2052 = clamp(((_2031 + (-1.0f)) + _2034) / _2034, 0.0f, 1.0f);
                                    float _2053 = clamp((_2032 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _2064 = _2039 * _2040;
                                    precise float _2065 = _2064 * _2064;
                                    float _2069 = ((((3.0f - (_2040 * 2.0f)) * (3.0f - (_2039 * 2.0f))) * _2065) * (1.0f - ((_2052 * _2052) * (3.0f - (_2052 * 2.0f))))) * (1.0f - ((_2053 * _2053) * (3.0f - (_2053 * 2.0f))));
                                    bool _2072 = (_1212 != 0u) || (_2069 >= 1.0f);
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder = _2069 * float(_480 > 0.0f);
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder_1 = _2072 ? _1212 : 1u;
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder_2 = _2072 ? 0.0f : _2069;
                                    break;
                                }
                            }
                            frontier_phi_49_50_ladder_58_ladder = frontier_phi_49_50_ladder_58_ladder_67_ladder;
                            frontier_phi_49_50_ladder_58_ladder_1 = frontier_phi_49_50_ladder_58_ladder_67_ladder_1;
                            frontier_phi_49_50_ladder_58_ladder_2 = frontier_phi_49_50_ladder_58_ladder_67_ladder_2;
                        }
                        frontier_phi_49_50_ladder = frontier_phi_49_50_ladder_58_ladder;
                        frontier_phi_49_50_ladder_1 = frontier_phi_49_50_ladder_58_ladder_1;
                        frontier_phi_49_50_ladder_2 = frontier_phi_49_50_ladder_58_ladder_2;
                    }
                    _1301 = frontier_phi_49_50_ladder_2;
                    _1303 = frontier_phi_49_50_ladder_1;
                    _1305 = frontier_phi_49_50_ladder;
                }
                bool _1421;
                float _1424;
                float _1426;
                float _1428;
                float _1430;
                float _1433;
                float _1435;
                float _1437;
                float _1439;
                float _1440;
                float _1441;
                float _1443;
                float _1346;
                float _1349;
                float _1352;
                float _1355;
                float _1359;
                float _1360;
                for (;;)
                {
                    _1346 = ((((exp2(log2(clamp((sqrt(((_1242 * _1242) + (_1241 * _1241)) + (_1243 * _1243)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _412) * exp2(log2(clamp((_1242 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_398, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    _1349 = mad(_167, _931, mad(_161, _930, _929 * _155));
                    _1352 = mad(_168, _931, mad(_162, _930, _929 * _156));
                    _1355 = mad(_169, _931, mad(_163, _930, _929 * _157));
                    bool _1358 = (_1303 != 0u) || (_1305 < 1.0f);
                    _1359 = _1358 ? 0.0f : 1.0f;
                    _1360 = _1358 ? 0.0f : 0.5f;
                    if (_1358)
                    {
                        bool _1416 = _402 == 1u;
                        uint4 _1420 = asuint(_50_m0[60u]);
                        if (_1416)
                        {
                            if (int(_410) < int(1u))
                            {
                                if (_1420.x == 0u)
                                {
                                    _1421 = false;
                                    _1424 = 0.0f;
                                    _1426 = 0.0f;
                                    _1428 = 0.0f;
                                    _1430 = 9899999600270360182784.0f;
                                    _1433 = _1193;
                                    _1435 = _1191;
                                    _1437 = _1189;
                                    _1439 = 0.0f;
                                    _1440 = 0.0f;
                                    _1441 = 0.0f;
                                    _1443 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1420.y == 0u)
                                {
                                    _1421 = false;
                                    _1424 = 0.0f;
                                    _1426 = 0.0f;
                                    _1428 = 0.0f;
                                    _1430 = 9899999600270360182784.0f;
                                    _1433 = _1193;
                                    _1435 = _1191;
                                    _1437 = _1189;
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
                            if (_1420.z == 0u)
                            {
                                _1421 = false;
                                _1424 = 0.0f;
                                _1426 = 0.0f;
                                _1428 = 0.0f;
                                _1430 = 9899999600270360182784.0f;
                                _1433 = _1193;
                                _1435 = _1191;
                                _1437 = _1189;
                                _1439 = 0.0f;
                                _1440 = 0.0f;
                                _1441 = 0.0f;
                                _1443 = 0.0f;
                                break;
                            }
                        }
                        if (_1301 > 0.0f)
                        {
                            _1421 = false;
                            _1424 = 0.0f;
                            _1426 = 0.0f;
                            _1428 = 0.0f;
                            _1430 = 9899999600270360182784.0f;
                            _1433 = _1193;
                            _1435 = _1191;
                            _1437 = _1189;
                            _1439 = 0.0f;
                            _1440 = 1.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        if ((_1195 <= 0.0f) || ((_1197 <= 0.0f) || (_1199 <= 0.0f)))
                        {
                            _1421 = false;
                            _1424 = 0.0f;
                            _1426 = 0.0f;
                            _1428 = 0.0f;
                            _1430 = 9899999600270360182784.0f;
                            _1433 = _1193;
                            _1435 = _1191;
                            _1437 = _1189;
                            _1439 = 0.0f;
                            _1440 = 1.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        if ((_1195 >= 1.0f) || ((_1199 >= _50_m0[51u].x) || (_1197 >= _50_m0[51u].y)))
                        {
                            _1421 = false;
                            _1424 = 0.0f;
                            _1426 = 0.0f;
                            _1428 = 0.0f;
                            _1430 = 9899999600270360182784.0f;
                            _1433 = _1193;
                            _1435 = _1191;
                            _1437 = _1189;
                            _1439 = 0.0f;
                            _1440 = 1.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        uint _2452;
                        uint _2454;
                        uint _2105;
                        uint _2106;
                        bool _2112;
                        for (;;)
                        {
                            _2105 = uint(clamp(_1207, 0.0f, 1.0f) * _202);
                            _2106 = uint(clamp(_1205, 0.0f, 1.0f) * _204);
                            _2112 = _20[21u].Load(int3(uint2(_2105, _2106), 0u)).x > 0.0f;
                            if (_2112)
                            {
                                uint _2314 = _24[23u].Load(int3(uint2(_2105, _2106), 0u)).y + 4294967295u;
                                _2452 = (uint(int(_2314) >> int(31u)) & 3u) + 1u;
                                _2454 = (int(_2314) < int(0u)) ? 0u : _2314;
                                break;
                            }
                            else
                            {
                                uint4 _2322 = _24[2u].Load(int3(uint2(_2105, _2106), 0u));
                                uint _2325 = _2322.w;
                                uint4 _2330 = _24[15u].Load(int3(uint2(_2105, _2106), 0u));
                                uint _2332 = _2330.y;
                                uint _2338 = ((_2332 & 64u) != 0u) ? uint((_2332 & 4294967167u) != 66u) : 4294967295u;
                                uint _2339 = _2325 & 128u;
                                uint _2341 = (_2339 != 0u) ? 1u : ((_2322.x << 7u) | _2325);
                                uint4 _2344 = _16.Load(_2341 * 4u);
                                uint _2345 = _2344.x;
                                uint _2352 = ((_2345 & 1u) != 0u) ? 0u : 18u;
                                uint _2354 = uint(min(int(uint(max(int(_2338), int(0u)))), int(1u)));
                                uint _2465;
                                if (_2339 == 0u)
                                {
                                    _2465 = (((_2345 & 2097152u) != 0u) && (_2338 == _2354)) ? (_2352 | 128u) : _2352;
                                }
                                else
                                {
                                    _2465 = _2325;
                                }
                                uint _2466 = _16.Load((_2341 * 4u) + 1u).x & 512u;
                                bool _2469 = (_2465 & 144u) == 0u;
                                if (_2466 == 0u)
                                {
                                    if (_2469 || ((_2345 & 1u) != 0u))
                                    {
                                        _2452 = 0u;
                                        _2454 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2469)
                                    {
                                        _2452 = 0u;
                                        _2454 = 0u;
                                        break;
                                    }
                                }
                                bool _2744 = ((_2465 & 128u) | _2466) != 0u;
                                uint _2453;
                                if (_2744)
                                {
                                    _2453 = 1u;
                                }
                                else
                                {
                                    _2453 = (((_2345 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_2345 & 268435472u) == 16u) && (((_2345 & 33554432u) == 0u) || _2744))
                                {
                                    _2452 = 2u;
                                    _2454 = 0u;
                                    break;
                                }
                                _2452 = _2453;
                                _2454 = (_2453 == 1u) ? _2354 : 0u;
                                break;
                            }
                        }
                        if ((_402 != _2452) || (_410 != _2454))
                        {
                            _1421 = false;
                            _1424 = 0.0f;
                            _1426 = 0.0f;
                            _1428 = 0.0f;
                            _1430 = 9899999600270360182784.0f;
                            _1433 = _1193;
                            _1435 = _1191;
                            _1437 = _1189;
                            _1439 = 0.0f;
                            _1440 = 1.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2586 = _1199 * 2.0f;
                        float _2589 = (_50_m0[51u].z * _2586) + (-1.0f);
                        float _2590 = ((1.0f - (_50_m0[51u].w * _1197)) * 2.0f) + (-1.0f);
                        float _2606 = mad(_190, _1195, mad(_183, _2590, _2589 * _176)) + _197;
                        float _2607 = (mad(_187, _1195, mad(_180, _2590, _2589 * _173)) + _194) / _2606;
                        float _2608 = (mad(_188, _1195, mad(_181, _2590, _2589 * _174)) + _195) / _2606;
                        float _2609 = (mad(_189, _1195, mad(_182, _2590, _2589 * _175)) + _196) / _2606;
                        if (sqrt(((_2608 * _2608) + (_2607 * _2607)) + (_2609 * _2609)) > _50_m0[58u].w)
                        {
                            _1421 = false;
                            _1424 = 0.0f;
                            _1426 = 0.0f;
                            _1428 = 0.0f;
                            _1430 = 9899999600270360182784.0f;
                            _1433 = _1193;
                            _1435 = _1191;
                            _1437 = _1189;
                            _1439 = 0.0f;
                            _1440 = 0.0f;
                            _1441 = 1000.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2724 = _2607 - _1241;
                        float _2725 = _2608 - _1242;
                        float _2726 = _2609 - _1243;
                        float _2732 = sqrt(((_2725 * _2725) + (_2724 * _2724)) + (_2726 * _2726));
                        float _2740 = min(_50_m0[59u].y, max(0.0f, _2732 + (-0.001000000047497451305389404296875f)));
                        float _2771;
                        if (_1416)
                        {
                            _2771 = min(_50_m0[59u].x, _50_m0[4u].z + _2740);
                        }
                        else
                        {
                            _2771 = _50_m0[59u].x;
                        }
                        float _2772 = _2771 - _2732;
                        if (!(_2772 > 0.0f))
                        {
                            _1421 = false;
                            _1424 = 0.0f;
                            _1426 = 0.0f;
                            _1428 = 0.0f;
                            _1430 = 9899999600270360182784.0f;
                            _1433 = _1193;
                            _1435 = _1191;
                            _1437 = _1189;
                            _1439 = 1.0f;
                            _1440 = 1.0f;
                            _1441 = 0.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2832 = _2607 - (_2740 * _1349);
                        float _2833 = _2608 - (_2740 * _1352);
                        float _2834 = _2609 - (_2740 * _1355);
                        RayDesc _2ident = {float3(mad(_2834, _50_m0[46u].z, mad(_2833, _50_m0[46u].y, _50_m0[46u].x * _2832)) + _50_m0[46u].w, mad(_2834, _50_m0[47u].z, mad(_2833, _50_m0[47u].y, _50_m0[47u].x * _2832)) + _50_m0[47u].w, mad(_2834, _50_m0[48u].z, mad(_2833, _50_m0[48u].y, _50_m0[48u].x * _2832)) + _50_m0[48u].w), 0.0f, float3(mad(_1355, _50_m0[46u].z, mad(_1352, _50_m0[46u].y, _50_m0[46u].x * _1349)), mad(_1355, _50_m0[47u].z, mad(_1352, _50_m0[47u].y, _50_m0[47u].x * _1349)), mad(_1355, _50_m0[48u].z, mad(_1352, _50_m0[48u].y, _50_m0[48u].x * _1349))), _2772};
                        _2837.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2884 = _2837.Proceed();
                        uint _2885 = _2837.CommittedStatus();
                        if (!(_2885 == 1u))
                        {
                            _1421 = false;
                            _1424 = 0.0f;
                            _1426 = 0.0f;
                            _1428 = 0.0f;
                            _1430 = 9899999600270360182784.0f;
                            _1433 = _1193;
                            _1435 = _1191;
                            _1437 = _1189;
                            _1439 = 1.0f;
                            _1440 = 0.0f;
                            _1441 = 0.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2899 = _2837.CommittedRayT();
                        if (!((_2899 < _2772) && (_2899 > 0.0f)))
                        {
                            _1421 = false;
                            _1424 = 0.0f;
                            _1426 = 0.0f;
                            _1428 = 0.0f;
                            _1430 = 9899999600270360182784.0f;
                            _1433 = _1193;
                            _1435 = _1191;
                            _1437 = _1189;
                            _1439 = 1.0f;
                            _1440 = 0.0f;
                            _1441 = 0.0f;
                            _1443 = 0.5f;
                            break;
                        }
                        float _2913 = (_50_m0[51u].z * _2586) + (-1.0f);
                        float _2914 = ((1.0f - (_50_m0[51u].w * _1197)) * 2.0f) + (-1.0f);
                        float _2930 = mad(_144, _1195, mad(_137, _2914, _2913 * _130)) + _151;
                        float _2934 = _2899 - _2740;
                        float _2938 = ((mad(_141, _1195, mad(_134, _2914, _2913 * _127)) + _148) / _2930) + (_2934 * _929);
                        float _2939 = ((mad(_142, _1195, mad(_135, _2914, _2913 * _128)) + _149) / _2930) + (_2934 * _930);
                        float _2940 = ((mad(_143, _1195, mad(_136, _2914, _2913 * _129)) + _150) / _2930) + (_2934 * _931);
                        float _2956 = mad(_116, _2940, mad(_109, _2939, _2938 * _102)) + _123;
                        float _1438 = (mad(_115, _2940, mad(_108, _2939, _2938 * _101)) + _122) / _2956;
                        float _1434 = ((((mad(_113, _2940, mad(_106, _2939, _2938 * _99)) + _120) / _2956) * 0.5f) + 0.5f) * _50_m0[51u].x;
                        float _1436 = (0.5f - (((mad(_114, _2940, mad(_107, _2939, _2938 * _100)) + _121) / _2956) * 0.5f)) * _50_m0[51u].y;
                        float _2965 = _1434 * _50_m0[51u].z;
                        float _2966 = _1436 * _50_m0[51u].w;
                        float _2968 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2971 = clamp(_2965 / _2968, 0.0f, 1.0f);
                        float _2972 = clamp(_2966 * 20.0f, 0.0f, 1.0f);
                        float _2982 = clamp(((_2965 + (-1.0f)) + _2968) / _2968, 0.0f, 1.0f);
                        float _2983 = clamp((_2966 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2994 = _2971 * _2972;
                        precise float _2995 = _2994 * _2994;
                        if ((((((3.0f - (_2972 * 2.0f)) * (3.0f - (_2971 * 2.0f))) * _2995) * (1.0f - ((_2982 * _2982) * (3.0f - (_2982 * 2.0f))))) * (1.0f - ((_2983 * _2983) * (3.0f - (_2983 * 2.0f))))) < 1.0f)
                        {
                            _1421 = false;
                            _1424 = 0.0f;
                            _1426 = 0.0f;
                            _1428 = 0.0f;
                            _1430 = 9899999600270360182784.0f;
                            _1433 = _1434;
                            _1435 = _1436;
                            _1437 = _1438;
                            _1439 = _1359;
                            _1440 = _1359;
                            _1441 = 0.0f;
                            _1443 = _1360;
                            break;
                        }
                        _1421 = true;
                        _1424 = _50_m0[1u].w;
                        _1426 = _50_m0[2u].x;
                        _1428 = _50_m0[2u].y;
                        _1430 = (_2732 - _2740) + _2899;
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
                        _1421 = false;
                        _1424 = 0.0f;
                        _1426 = 0.0f;
                        _1428 = 0.0f;
                        _1430 = 9899999600270360182784.0f;
                        _1433 = _1193;
                        _1435 = _1191;
                        _1437 = _1189;
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
                float _1479;
                float _1481;
                float _1483;
                if ((_1305 >= 1.0f) || _1421)
                {
                    _1479 = _1433;
                    _1481 = _1435;
                    _1483 = _1437;
                }
                else
                {
                    float _1634 = (-0.0f) - _974;
                    float _1635 = _1634 / _1021;
                    float _1638 = (_1635 * _1018) + _981;
                    float _1639 = (_1635 * _1020) + _982;
                    float _1640 = _1634 + _974;
                    _1479 = ((_1433 - _1638) * _1305) + _1638;
                    _1481 = ((_1435 - _1639) * _1305) + _1639;
                    _1483 = ((_1437 - _1640) * _1305) + _1640;
                }
                float _1494 = ((_1479 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _1495 = ((1.0f - (_50_m0[51u].w * _1481)) * 2.0f) + (-1.0f);
                float _1511 = mad(_144, _1483, mad(_137, _1495, _1494 * _130)) + _151;
                float _1515 = ((mad(_141, _1483, mad(_134, _1495, _1494 * _127)) + _148) / _1511) - _953;
                float _1516 = ((mad(_142, _1483, mad(_135, _1495, _1494 * _128)) + _149) / _1511) - _954;
                float _1517 = ((mad(_143, _1483, mad(_136, _1495, _1494 * _129)) + _150) / _1511) - _955;
                float _1523 = sqrt(((_1516 * _1516) + (_1515 * _1515)) + (_1517 * _1517));
                float _1524 = _1523 * _783;
                float _1525 = _1523 * _784;
                float _1526 = _1523 * _785;
                float _1527 = _1523 * (_896 / _899);
                float _1528 = _1523 * (_897 / _899);
                float _1529 = _1523 * (_898 / _899);
                float _1533 = dot(float3(_1524, _1525, _1526), float3(_769, _772, _775)) * 2.0f;
                float _1543 = dot(float3(_1527, _1528, _1529), float3(_769, _772, _775)) * 2.0f;
                float _1570 = (_1524 - (_1533 * _769)) + _953;
                float _1571 = (_1525 - (_1533 * _772)) + _954;
                float _1572 = (_1526 - (_1533 * _775)) + _955;
                float _1584 = mad(_50_m0[24u].w, _1572, mad(_50_m0[23u].w, _1571, _1570 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1589 = (_1527 - (_1543 * _769)) + _953;
                float _1590 = (_1528 - (_1543 * _772)) + _954;
                float _1591 = (_1529 - (_1543 * _775)) + _955;
                float _1603 = mad(_50_m0[24u].w, _1591, mad(_50_m0[23u].w, _1590, _1589 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1609 = (_50_m0[51u].x * ((((mad(_50_m0[24u].x, _1572, mad(_50_m0[23u].x, _1571, _1570 * _50_m0[22u].x)) + _50_m0[25u].x) / _1584) - ((mad(_50_m0[24u].x, _1591, mad(_50_m0[23u].x, _1590, _1589 * _50_m0[22u].x)) + _50_m0[25u].x) / _1603)) * 0.5f)) * _1448;
                float _1613 = (_50_m0[51u].y * ((((mad(_50_m0[24u].y, _1591, mad(_50_m0[23u].y, _1590, _1589 * _50_m0[22u].y)) + _50_m0[25u].y) / _1603) - ((mad(_50_m0[24u].y, _1572, mad(_50_m0[23u].y, _1571, _1570 * _50_m0[22u].y)) + _50_m0[25u].y) / _1584)) * 0.5f)) * _1450;
                float _1632 = clamp(log2((sqrt((_1613 * _1613) + (_1609 * _1609)) * 2.0f) / _50_m0[0u].w) / float(asuint(_50_m0[55u]).x + 4294967295u), 0.0f, 1.0f);
                bool _1633 = _402 == 1u;
                float _1753;
                if (_1633)
                {
                    float _1752 = _50_m0[4u].z - _50_m0[4u].y;
                    float _1925;
                    if (_1421)
                    {
                        float _1837 = clamp((_1430 - _50_m0[4u].y) / _1752, 0.0f, 1.0f);
                        _1925 = (_1837 * _1837) * (3.0f - (_1837 * 2.0f));
                    }
                    else
                    {
                        float _1850 = clamp((sqrt(((_1270 * _1270) + (_1269 * _1269)) + (_1271 * _1271)) - _50_m0[4u].y) / _1752, 0.0f, 1.0f);
                        _1925 = (_1850 * _1850) * (3.0f - (_1850 * 2.0f));
                    }
                    _1753 = _50_m0[61u].z * (1.0f - _1925);
                }
                else
                {
                    _1753 = 1.0f;
                }
                float _1755 = _1753 * _1305;
                bool _1756 = _402 != 1u;
                float _2356;
                float _2358;
                float _2360;
                float _2362;
                if (_1755 == 0.0f)
                {
                    float _2161;
                    float _2162;
                    float _2163;
                    float _2164;
                    if (_1421)
                    {
                        float frontier_phi_113_95_ladder;
                        float frontier_phi_113_95_ladder_1;
                        float frontier_phi_113_95_ladder_2;
                        float frontier_phi_113_95_ladder_3;
                        if (_1756)
                        {
                            float _2113 = _1352 * _1346;
                            float _2117 = rsqrt(dot(float3(_1349, _2113, _1355), float3(_1349, _2113, _1355)));
                            float4 _2127 = _28[4u].SampleLevel(_59, float3(_2117 * _1349, _2117 * _2113, _2117 * _1355), 0.0f);
                            float _2129 = _2127.x;
                            float _2130 = _2127.y;
                            float _2131 = _2127.z;
                            frontier_phi_113_95_ladder = 1.0f;
                            frontier_phi_113_95_ladder_1 = ((_1428 - _2131) * _1753) + _2131;
                            frontier_phi_113_95_ladder_2 = ((_1426 - _2130) * _1753) + _2130;
                            frontier_phi_113_95_ladder_3 = ((_1424 - _2129) * _1753) + _2129;
                        }
                        else
                        {
                            frontier_phi_113_95_ladder = _1753;
                            frontier_phi_113_95_ladder_1 = _1753 * _1428;
                            frontier_phi_113_95_ladder_2 = _1753 * _1426;
                            frontier_phi_113_95_ladder_3 = _1753 * _1424;
                        }
                        _2161 = frontier_phi_113_95_ladder_3;
                        _2162 = frontier_phi_113_95_ladder_2;
                        _2163 = frontier_phi_113_95_ladder_1;
                        _2164 = frontier_phi_113_95_ladder;
                    }
                    else
                    {
                        float frontier_phi_113_96_ladder;
                        float frontier_phi_113_96_ladder_1;
                        float frontier_phi_113_96_ladder_2;
                        float frontier_phi_113_96_ladder_3;
                        if (_1756)
                        {
                            float _2144 = _1352 * _1346;
                            float _2148 = rsqrt(dot(float3(_1349, _2144, _1355), float3(_1349, _2144, _1355)));
                            float4 _2156 = _28[4u].SampleLevel(_59, float3(_2148 * _1349, _2148 * _2144, _2148 * _1355), 0.0f);
                            frontier_phi_113_96_ladder = 1.0f;
                            frontier_phi_113_96_ladder_1 = _2156.z;
                            frontier_phi_113_96_ladder_2 = _2156.y;
                            frontier_phi_113_96_ladder_3 = _2156.x;
                        }
                        else
                        {
                            frontier_phi_113_96_ladder = 0.0f;
                            frontier_phi_113_96_ladder_1 = 0.0f;
                            frontier_phi_113_96_ladder_2 = 0.0f;
                            frontier_phi_113_96_ladder_3 = 0.0f;
                        }
                        _2161 = frontier_phi_113_96_ladder_3;
                        _2162 = frontier_phi_113_96_ladder_2;
                        _2163 = frontier_phi_113_96_ladder_1;
                        _2164 = frontier_phi_113_96_ladder;
                    }
                    _2356 = _2161 * _480;
                    _2358 = _2162 * _480;
                    _2360 = _2163 * _480;
                    _2362 = _2164 * _480;
                }
                else
                {
                    float _2184;
                    float _2186;
                    float _2191;
                    float _2195;
                    if (_12.Load(int3(uint2(uint(_1448 * _1185), uint(_1450 * _1183)), 0u)).x > 0.0f)
                    {
                        uint _1932_dummy_parameter;
                        uint2 _1932 = spvTextureSize(_14, 0u, _1932_dummy_parameter);
                        float4 _1941 = _14.Load(int3(uint2(uint(float(_1932.x) * _1185), uint(float(_1932.y) * _1183)), 0u));
                        float _1945 = _1941.x * 0.5f;
                        float _1946 = _1941.y * (-0.5f);
                        float4 _1965 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _1945) + (_50_m0[52u].x * _1185), (_50_m0[52u].w * _1946) + (_50_m0[52u].y * _1183)), 0.0f);
                        float _2180;
                        if (_1633)
                        {
                            float frontier_phi_115_114_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _974))) < 5.0f)
                            {
                                float _2416 = sqrt((_1945 * _1945) + (_1946 * _1946));
                                float frontier_phi_115_114_ladder_129_ladder;
                                if (_2416 > 0.0500000007450580596923828125f)
                                {
                                    float _2477 = _1185 - _981;
                                    float _2478 = _1183 - _982;
                                    float frontier_phi_115_114_ladder_129_ladder_143_ladder;
                                    if (_2416 > sqrt((_2477 * _2477) + (_2478 * _2478)))
                                    {
                                        uint4 _2632 = asuint(_55_m0[0u]);
                                        uint _2639 = uint(float(_2632.x) * _1185);
                                        uint _2640 = uint(float(_2632.y) * _1183);
                                        uint4 _2643 = _24[2u].Load(int3(uint2(_2639, _2640), 0u));
                                        uint _2646 = _2643.w;
                                        uint4 _2651 = _24[15u].Load(int3(uint2(_2639, _2640), 0u));
                                        uint _2653 = _2651.y;
                                        uint _2659 = ((_2653 & 64u) != 0u) ? uint((_2653 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2660 = _2646 & 128u;
                                        uint _2662 = (_2660 != 0u) ? 1u : ((_2643.x << 7u) | _2646);
                                        uint4 _2665 = _16.Load(_2662 * 4u);
                                        uint _2666 = _2665.x;
                                        uint _2673 = ((_2666 & 1u) != 0u) ? 0u : 18u;
                                        uint _2761;
                                        if (_2660 == 0u)
                                        {
                                            _2761 = (((_2666 & 2097152u) != 0u) && (_2659 == uint(min(int(uint(max(int(_2659), int(0u)))), int(1u))))) ? (_2673 | 128u) : _2673;
                                        }
                                        else
                                        {
                                            _2761 = _2646;
                                        }
                                        float frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder;
                                        if (((_2761 & 128u) | (_16.Load((_2662 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2787 = asuint(_55_m0[0u]);
                                            uint _2796 = uint(float(_2787.x) * (_1945 + _1185));
                                            uint _2797 = uint(float(_2787.y) * (_1946 + _1183));
                                            uint4 _2800 = _24[2u].Load(int3(uint2(_2796, _2797), 0u));
                                            uint _2803 = _2800.w;
                                            uint4 _2806 = _24[15u].Load(int3(uint2(_2796, _2797), 0u));
                                            uint _2808 = _2806.y;
                                            uint _2814 = ((_2808 & 64u) != 0u) ? uint((_2808 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2815 = _2803 & 128u;
                                            uint _2817 = (_2815 != 0u) ? 1u : ((_2800.x << 7u) | _2803);
                                            uint4 _2819 = _16.Load(_2817 * 4u);
                                            uint _2820 = _2819.x;
                                            uint _2827 = ((_2820 & 1u) != 0u) ? 0u : 18u;
                                            uint _2896;
                                            if (_2815 == 0u)
                                            {
                                                _2896 = (((_2820 & 2097152u) != 0u) && (_2814 == uint(min(int(uint(max(int(_2814), int(0u)))), int(1u))))) ? (_2827 | 128u) : _2827;
                                            }
                                            else
                                            {
                                                _2896 = _2803;
                                            }
                                            float frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder_172_ladder;
                                            if ((_2896 & 128u) == 0u)
                                            {
                                                frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder_172_ladder = ((_16.Load((_2817 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder_172_ladder = 0.0f;
                                            }
                                            frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder = frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder_172_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder = 1.0f;
                                        }
                                        frontier_phi_115_114_ladder_129_ladder_143_ladder = frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_115_114_ladder_129_ladder_143_ladder = 1.0f;
                                    }
                                    frontier_phi_115_114_ladder_129_ladder = frontier_phi_115_114_ladder_129_ladder_143_ladder;
                                }
                                else
                                {
                                    frontier_phi_115_114_ladder_129_ladder = 1.0f;
                                }
                                frontier_phi_115_114_ladder = frontier_phi_115_114_ladder_129_ladder;
                            }
                            else
                            {
                                frontier_phi_115_114_ladder = 1.0f;
                            }
                            _2180 = frontier_phi_115_114_ladder;
                        }
                        else
                        {
                            _2180 = 1.0f;
                        }
                        float _2182 = _2180 * _1755;
                        float _2190;
                        float _2194;
                        float _2198;
                        if (_1203 == 0u)
                        {
                            _2190 = _50_m0[54u].x * _1965.x;
                            _2194 = _50_m0[54u].x * _1965.y;
                            _2198 = _50_m0[54u].x * _1965.z;
                        }
                        else
                        {
                            _2190 = _50_m0[1u].w;
                            _2194 = _50_m0[2u].x;
                            _2198 = _50_m0[2u].y;
                        }
                        float frontier_phi_116_130_ladder;
                        float frontier_phi_116_130_ladder_1;
                        float frontier_phi_116_130_ladder_2;
                        float frontier_phi_116_130_ladder_3;
                        for (;;)
                        {
                            if (_1201 > 0.0f)
                            {
                                float _2189;
                                float _2193;
                                float _2197;
                                if (_1633)
                                {
                                    _2189 = 0.0f;
                                    _2193 = 0.0f;
                                    _2197 = 0.0f;
                                }
                                else
                                {
                                    if (!((_402 != 4u) || (_1209 != 0u)))
                                    {
                                        frontier_phi_116_130_ladder = _2198;
                                        frontier_phi_116_130_ladder_1 = _2194;
                                        frontier_phi_116_130_ladder_2 = _2190;
                                        frontier_phi_116_130_ladder_3 = _2182;
                                        break;
                                    }
                                    _2189 = _2190;
                                    _2193 = _2194;
                                    _2197 = _2198;
                                }
                                frontier_phi_116_130_ladder = _2197;
                                frontier_phi_116_130_ladder_1 = _2193;
                                frontier_phi_116_130_ladder_2 = _2189;
                                frontier_phi_116_130_ladder_3 = (1.0f - exp2(log2(_1201) * _50_m0[4u].x)) * _2182;
                                break;
                            }
                            else
                            {
                                frontier_phi_116_130_ladder = _2198;
                                frontier_phi_116_130_ladder_1 = _2194;
                                frontier_phi_116_130_ladder_2 = _2190;
                                frontier_phi_116_130_ladder_3 = _2182;
                                break;
                            }
                        }
                        _2184 = frontier_phi_116_130_ladder_3;
                        _2186 = frontier_phi_116_130_ladder_2;
                        _2191 = frontier_phi_116_130_ladder_1;
                        _2195 = frontier_phi_116_130_ladder;
                    }
                    else
                    {
                        float frontier_phi_116_98_ladder;
                        float frontier_phi_116_98_ladder_1;
                        float frontier_phi_116_98_ladder_2;
                        float frontier_phi_116_98_ladder_3;
                        if (_1211)
                        {
                            frontier_phi_116_98_ladder = _2188;
                            frontier_phi_116_98_ladder_1 = _2188;
                            frontier_phi_116_98_ladder_2 = _2188;
                            frontier_phi_116_98_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float4 _2203 = _28[7u].SampleLevel(_59, float3(_1349, _1352, _1355), 0.0f);
                            frontier_phi_116_98_ladder = _2203.z;
                            frontier_phi_116_98_ladder_1 = _2203.y;
                            frontier_phi_116_98_ladder_2 = _2203.x;
                            frontier_phi_116_98_ladder_3 = _1755;
                        }
                        _2184 = frontier_phi_116_98_ladder_3;
                        _2186 = frontier_phi_116_98_ladder_2;
                        _2191 = frontier_phi_116_98_ladder_1;
                        _2195 = frontier_phi_116_98_ladder;
                    }
                    float _2435;
                    float _2436;
                    float _2437;
                    float _2438;
                    if (_1421)
                    {
                        _2435 = _1753;
                        _2436 = ((_2186 - _1424) * _1301) + _1424;
                        _2437 = ((_2191 - _1426) * _1301) + _1426;
                        _2438 = ((_2195 - _1428) * _1301) + _1428;
                    }
                    else
                    {
                        _2435 = _2184;
                        _2436 = _2186;
                        _2437 = _2191;
                        _2438 = _2195;
                    }
                    float _2510;
                    float _2511;
                    float _2512;
                    float _2513;
                    if (_1756 && (_2435 < 1.0f))
                    {
                        float _2484 = _1352 * _1346;
                        float _2488 = rsqrt(dot(float3(_1349, _2484, _1355), float3(_1349, _2484, _1355)));
                        float4 _2496 = _28[4u].SampleLevel(_59, float3(_2488 * _1349, _2488 * _2484, _2488 * _1355), 0.0f);
                        float _2498 = _2496.x;
                        float _2499 = _2496.y;
                        float _2500 = _2496.z;
                        _2510 = 1.0f;
                        _2511 = ((_2436 - _2498) * _2435) + _2498;
                        _2512 = ((_2437 - _2499) * _2435) + _2499;
                        _2513 = ((_2438 - _2500) * _2435) + _2500;
                    }
                    else
                    {
                        _2510 = _2435;
                        _2511 = _2436;
                        _2512 = _2437;
                        _2513 = _2438;
                    }
                    float _2363 = _2510 * _480;
                    _2356 = _2511 * _2363;
                    _2358 = _2512 * _2363;
                    _2360 = _2513 * _2363;
                    _2362 = _2363;
                }
                float _2367 = _50_m0[58u].z * _1443;
                float _2389 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _2391 = _2389 * ((_2367 * ((_1439 * 1000.0f) - _2356)) + _2356);
                float _2392 = _2389 * ((_2367 * ((_1440 * 1000.0f) - _2358)) + _2358);
                float _2393 = _2389 * ((_2367 * (_1441 - _2360)) + _2360);
                float _2399 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2391, max(_2392, _2393)) + 1.0f);
                float _2403 = min(_2399 * _2391, 0.996078431606292724609375f);
                float _2405 = min(_2399 * _2392, 0.996078431606292724609375f);
                float _2406 = min(_2399 * _2393, 0.996078431606292724609375f);
                _35[uint2(_226, _229)] = float4(_2403, _2405, _2406, _2362);
                _39[uint2(_226, _229)] = float4(_1632, 0.0f, 0.0f, _1632);
                if (_233)
                {
                    uint _2470 = _226 + 1u;
                    _35[uint2(_2470, _229)] = float4(_2403, _2405, _2406, _2362);
                    _39[uint2(_2470, _229)] = float4(_1632, 0.0f, 0.0f, _1632);
                }
                if (_236)
                {
                    uint _2623 = _229 + 1u;
                    _35[uint2(_226, _2623)] = float4(_2403, _2405, _2406, _2362);
                    _39[uint2(_226, _2623)] = float4(_1632, 0.0f, 0.0f, _1632);
                }
                if (_237)
                {
                    uint _2745 = _226 + 1u;
                    uint _2746 = _229 + 1u;
                    _35[uint2(_2745, _2746)] = float4(_2403, _2405, _2406, _2362);
                    _39[uint2(_2745, _2746)] = float4(_1632, 0.0f, 0.0f, _1632);
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
