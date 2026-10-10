static float _2189;
static uint _3338;
static float _3339;
static float _3340;
static uint _3341;
static float _3342;
static float _3343;
static float _3344;
static float _3345;
static float _3346;
static float _3347;
static float _3348;
static uint _3349;
static uint _3350;
static float _3351;
static float _3352;
static float _3353;
static float _3354;
static float _3355;
static uint _3356;
static float _3357;
static float _3363;
static uint _3364;
static float _3365;
static float _3370;
static float _3371;
static float _3372;

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

static RayQuery<RAY_FLAG_NONE> _2838;

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
                        float _1151;
                        if (_802)
                        {
                            _1151 = _798;
                        }
                        else
                        {
                            _1151 = float(_595 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1273;
                        if ((_434 & 134217728u) == 0u)
                        {
                            uint frontier_phi_46_40_ladder;
                            if ((_596 != 0u) || ((_434 & 17825792u) == 1048576u))
                            {
                                frontier_phi_46_40_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_46_40_ladder = _792;
                            }
                            _1273 = frontier_phi_46_40_ladder;
                        }
                        else
                        {
                            _1273 = _792;
                        }
                        uint4 _1276 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _1278 = _1276.x;
                        float _1402;
                        float _1404;
                        float _1406;
                        if (_791 == 0u)
                        {
                            _1402 = 0.0f;
                            _1404 = 0.0f;
                            _1406 = 0.0f;
                        }
                        else
                        {
                            float4 _1411 = _20[8u].Load(int3(uint2(_270, _271), 0u));
                            _1402 = _1411.x;
                            _1404 = _1411.y;
                            _1406 = _1411.z;
                        }
                        uint _1470;
                        if (_1273 == 0u)
                        {
                            _1470 = 0u;
                        }
                        else
                        {
                            _1470 = _24[9u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        uint _1701;
                        if (_793 == 0u)
                        {
                            _1701 = 0u;
                        }
                        else
                        {
                            _1701 = _24[10u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        float _1711 = (float((_1278 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1712 = (float(_1278 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1716 = (1.0f - abs(_1711)) - abs(_1712);
                        float _1718 = clamp((-0.0f) - _1716, 0.0f, 1.0f);
                        float _1719 = (-0.0f) - _1718;
                        float _1724 = ((_1711 >= 0.0f) ? _1719 : _1718) + _1711;
                        float _1725 = ((_1712 >= 0.0f) ? _1719 : _1718) + _1712;
                        float _1729 = rsqrt(dot(float3(_1724, _1725, _1716), float3(_1724, _1725, _1716)));
                        float _1730 = _1724 * _1729;
                        float _1731 = _1725 * _1729;
                        float _1732 = _1729 * _1716;
                        float _804 = float(_1278 & 255u);
                        float _1736 = ((_434 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1909;
                        float _1910;
                        float _1911;
                        float _1912;
                        uint _1913;
                        if ((_385 & 64u) == 0u)
                        {
                            uint frontier_phi_92_79_ladder;
                            float frontier_phi_92_79_ladder_1;
                            float frontier_phi_92_79_ladder_2;
                            float frontier_phi_92_79_ladder_3;
                            float frontier_phi_92_79_ladder_4;
                            if ((_434 & 276824064u) == 0u)
                            {
                                frontier_phi_92_79_ladder = 0u;
                                frontier_phi_92_79_ladder_1 = 0.0f;
                                frontier_phi_92_79_ladder_2 = 0.0f;
                                frontier_phi_92_79_ladder_3 = 0.0f;
                                frontier_phi_92_79_ladder_4 = ((_434 & 8u) != 0u) ? _1404 : _1736;
                            }
                            else
                            {
                                frontier_phi_92_79_ladder = 0u;
                                frontier_phi_92_79_ladder_1 = 0.0f;
                                frontier_phi_92_79_ladder_2 = 0.0f;
                                frontier_phi_92_79_ladder_3 = 0.0f;
                                frontier_phi_92_79_ladder_4 = _1736;
                            }
                            _1909 = frontier_phi_92_79_ladder_4;
                            _1910 = frontier_phi_92_79_ladder_3;
                            _1911 = frontier_phi_92_79_ladder_2;
                            _1912 = frontier_phi_92_79_ladder_1;
                            _1913 = frontier_phi_92_79_ladder;
                        }
                        else
                        {
                            float _1809 = (_1404 * 2.0f) + (-1.0f);
                            float _1810 = (_1406 * 2.0f) + (-1.0f);
                            float _1814 = (1.0f - abs(_1809)) - abs(_1810);
                            float _1816 = clamp((-0.0f) - _1814, 0.0f, 1.0f);
                            float _1817 = (-0.0f) - _1816;
                            float _1822 = ((_1809 >= 0.0f) ? _1817 : _1816) + _1809;
                            float _1823 = ((_1810 >= 0.0f) ? _1817 : _1816) + _1810;
                            float _1827 = rsqrt(dot(float3(_1822, _1823, _1814), float3(_1822, _1823, _1814)));
                            _1909 = floor(round(_1402 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1910 = _1822 * _1827;
                            _1911 = _1823 * _1827;
                            _1912 = _1827 * _1814;
                            _1913 = 1u;
                        }
                        float _812;
                        if ((_434 & 32768u) == 0u)
                        {
                            _812 = _1909;
                        }
                        else
                        {
                            float frontier_phi_107_108_ladder;
                            if (_17.Load((_389 * 115u) + 36u).x == 0u)
                            {
                                float _2308 = clamp((_804 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _585;
                                frontier_phi_107_108_ladder = ((_434 & 131072u) != 0u) ? _2308 : ((((clamp((1.21000003814697265625f / (exp2((_1151 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_389 * 115u) + 32u).x)) + 1.0f) * _2308);
                            }
                            else
                            {
                                frontier_phi_107_108_ladder = _585;
                            }
                            _812 = frontier_phi_107_108_ladder;
                        }
                        uint _2091 = _433 & 1u;
                        float _2251;
                        float _2253;
                        float _2255;
                        uint _2257;
                        if (((_434 & 16u) == 0u) || (((_2091 | (_385 & 8u)) | (_434 & 16777216u)) != 0u))
                        {
                            _2251 = _1910;
                            _2253 = _1911;
                            _2255 = _1912;
                            _2257 = _1913;
                        }
                        else
                        {
                            float _2267 = (float(_1470 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2268 = (float(_1470 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2272 = (1.0f - abs(_2267)) - abs(_2268);
                            float _2274 = clamp((-0.0f) - _2272, 0.0f, 1.0f);
                            float _2275 = (-0.0f) - _2274;
                            float _2280 = ((_2267 >= 0.0f) ? _2275 : _2274) + _2267;
                            float _2281 = ((_2268 >= 0.0f) ? _2275 : _2274) + _2268;
                            float _2285 = rsqrt(dot(float3(_2280, _2281, _2272), float3(_2280, _2281, _2272)));
                            _2251 = _2280 * _2285;
                            _2253 = _2281 * _2285;
                            _2255 = _2285 * _2272;
                            _2257 = 1u;
                        }
                        float _806;
                        float _808;
                        float _810;
                        if (_2091 == 0u)
                        {
                            float frontier_phi_137_136_ladder;
                            float frontier_phi_137_136_ladder_1;
                            float frontier_phi_137_136_ladder_2;
                            if (((_433 & 64u) == 0u) && (_794 != 0u))
                            {
                                float2 _2528 = spvUnpackHalf2x16((_1701 >> 17u) & 32736u);
                                float _2529 = _2528.x;
                                float _2532 = (spvUnpackHalf2x16((_1701 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2533 = (spvUnpackHalf2x16((_1701 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2537 = (1.0f - abs(_2532)) - abs(_2533);
                                float _2539 = clamp((-0.0f) - _2537, 0.0f, 1.0f);
                                float _2540 = (-0.0f) - _2539;
                                float _2545 = ((_2532 >= 0.0f) ? _2540 : _2539) + _2532;
                                float _2546 = ((_2533 >= 0.0f) ? _2540 : _2539) + _2533;
                                float _2550 = rsqrt(dot(float3(_2545, _2546, _2537), float3(_2545, _2546, _2537)));
                                float _2560 = (((_2545 * _2550) - _1730) * _2529) + _1730;
                                float _2561 = (((_2546 * _2550) - _1731) * _2529) + _1731;
                                float _2562 = (((_2550 * _2537) - _1732) * _2529) + _1732;
                                float _2566 = rsqrt(dot(float3(_2560, _2561, _2562), float3(_2560, _2561, _2562)));
                                frontier_phi_137_136_ladder = _2562 * _2566;
                                frontier_phi_137_136_ladder_1 = _2561 * _2566;
                                frontier_phi_137_136_ladder_2 = _2560 * _2566;
                            }
                            else
                            {
                                frontier_phi_137_136_ladder = _1732;
                                frontier_phi_137_136_ladder_1 = _1731;
                                frontier_phi_137_136_ladder_2 = _1730;
                            }
                            _806 = frontier_phi_137_136_ladder_2;
                            _808 = frontier_phi_137_136_ladder_1;
                            _810 = frontier_phi_137_136_ladder;
                        }
                        else
                        {
                            _806 = _1730;
                            _808 = _1731;
                            _810 = _1732;
                        }
                        float _820;
                        float _822;
                        float _824;
                        float _826;
                        if (_802)
                        {
                            float frontier_phi_149_148_ladder;
                            float frontier_phi_149_148_ladder_1;
                            float frontier_phi_149_148_ladder_2;
                            float frontier_phi_149_148_ladder_3;
                            if (((_434 & 33554432u) == 0u) || (((_385 & 4u) != 0u) && ((_434 & 8388608u) == 0u)))
                            {
                                frontier_phi_149_148_ladder = 0.0f;
                                frontier_phi_149_148_ladder_1 = 0.0f;
                                frontier_phi_149_148_ladder_2 = 0.0f;
                                frontier_phi_149_148_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2699 = (spvUnpackHalf2x16((_1701 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2700 = (spvUnpackHalf2x16((_1701 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2704 = (1.0f - abs(_2699)) - abs(_2700);
                                float _2706 = clamp((-0.0f) - _2704, 0.0f, 1.0f);
                                float _2707 = (-0.0f) - _2706;
                                float _2712 = ((_2699 >= 0.0f) ? _2707 : _2706) + _2699;
                                float _2713 = ((_2700 >= 0.0f) ? _2707 : _2706) + _2700;
                                float _2717 = rsqrt(dot(float3(_2712, _2713, _2704), float3(_2712, _2713, _2704)));
                                float _2718 = _2712 * _2717;
                                float _2719 = _2713 * _2717;
                                float _2720 = _2717 * _2704;
                                float _2724 = rsqrt(dot(float3(_2718, _2719, _2720), float3(_2718, _2719, _2720)));
                                frontier_phi_149_148_ladder = _2724 * _2720;
                                frontier_phi_149_148_ladder_1 = _2724 * _2719;
                                frontier_phi_149_148_ladder_2 = _2724 * _2718;
                                frontier_phi_149_148_ladder_3 = spvUnpackHalf2x16((_1701 >> 17u) & 32736u).x;
                            }
                            _820 = frontier_phi_149_148_ladder_3;
                            _822 = frontier_phi_149_148_ladder_2;
                            _824 = frontier_phi_149_148_ladder_1;
                            _826 = frontier_phi_149_148_ladder;
                        }
                        else
                        {
                            _820 = 0.0f;
                            _822 = 0.0f;
                            _824 = 0.0f;
                            _826 = 0.0f;
                        }
                        bool _2580 = _2257 != 0u;
                        _803 = _804;
                        _805 = _806;
                        _807 = _808;
                        _809 = _810;
                        _811 = _812;
                        _813 = _2580 ? _2251 : _806;
                        _815 = _2580 ? _2253 : _808;
                        _817 = _2580 ? _2255 : _810;
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
                    bool _1160 = ((_433 & 128u) | _443) != 0u;
                    uint _1172;
                    if (_1160)
                    {
                        _1172 = 1u;
                    }
                    else
                    {
                        _1172 = (((_434 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _397;
                    float _399;
                    float _401;
                    uint _403;
                    float _407;
                    if (((_434 & 33554432u) == 0u) || _1160)
                    {
                        bool _1280 = _77 != 0u;
                        uint _1287;
                        if ((_434 & 16u) == 0u)
                        {
                            _1287 = _1172;
                        }
                        else
                        {
                            _1287 = ((_434 & 268435456u) != 0u) ? _1172 : 2u;
                        }
                        _407 = _811 * _827;
                        _397 = _1280 ? _813 : _805;
                        _399 = _1280 ? _815 : _807;
                        _401 = _1280 ? _817 : _809;
                        _403 = _1287;
                    }
                    else
                    {
                        _407 = _819;
                        _397 = _821;
                        _399 = _823;
                        _401 = _825;
                        _403 = _1172;
                    }
                    uint _1288 = _403 + 102u;
                    float _1297 = clamp((_407 - _45_m0[_1288].x) / (_45_m0[_1288].y - _45_m0[_1288].x), 0.0f, 1.0f);
                    _396 = _397;
                    _398 = _399;
                    _400 = _401;
                    _402 = _403;
                    _404 = (_1297 * _1297) * (3.0f - (_1297 * 2.0f));
                    _406 = _407;
                    _408 = _45_m0[_1288].z;
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
                float _1029 = _50_m0[2u].w / dot(float3(_930, _931, _932), float3(_522, _525, _528));
                float _1033 = (_1029 * _930) + _954;
                float _1034 = (_1029 * _931) + _955;
                float _1035 = (_1029 * _932) + _956;
                float _1051 = mad(_116, _1035, mad(_109, _1034, _1033 * _102)) + _123;
                float _1060 = _50_m0[51u].x * (((((mad(_113, _1035, mad(_106, _1034, _1033 * _99)) + _120) / _1051) * 0.5f) + 0.5f) - _978);
                float _1062 = _50_m0[51u].y * ((0.5f - (((mad(_114, _1035, mad(_107, _1034, _1033 * _100)) + _121) / _1051) * 0.5f)) - _979);
                float _1063 = ((mad(_115, _1035, mad(_108, _1034, _1033 * _101)) + _122) / _1051) - _975;
                float _1076 = sqrt(((_1060 * _1060) + (_1063 * _1063)) + (_1062 * _1062)) / sqrt(((_1019 * _1019) + (_1022 * _1022)) + (_1021 * _1021));
                float _1077 = float(_238);
                float _1078 = float(_239);
                float _1085 = (_1019 != 0.0f) ? (0.100000001490116119384765625f / _1015) : 3.4028234663852885981170418348452e+38f;
                float _1087 = (_1021 != 0.0f) ? (0.100000001490116119384765625f / _1017) : 3.4028234663852885981170418348452e+38f;
                float _1088 = (_1022 != 0.0f) ? (0.100000001490116119384765625f / _1018) : 3.4028234663852885981170418348452e+38f;
                float _1089 = 1.0f / _1077;
                float _1090 = 1.0f / _1078;
                float _1091 = 0.004999999888241291046142578125f / _1077;
                float _1093 = 0.004999999888241291046142578125f / _1078;
                float _1102 = float(_1019 >= 0.0f);
                float _1103 = float(_1021 >= 0.0f);
                float _1112 = ((_1019 < 0.0f) ? ((-0.0f) - _1091) : _1091) - _982;
                float _1115 = ((_1021 < 0.0f) ? ((-0.0f) - _1093) : _1093) - _983;
                float _1118 = min((((floor(_982 * _1077) + _1102) * _1089) + _1112) * _1085, (((floor(_983 * _1078) + _1103) * _1090) + _1115) * _1087);
                float _1122 = (_1118 * _1019) + _982;
                float _1123 = (_1118 * _1021) + _983;
                float _1124 = (_1118 * _1022) + _975;
                float _1127 = _50_m0[50u].x / (_464 - (_1124 * _50_m0[50u].y));
                float _1137 = 1.0f / max(0.00999999977648258209228515625f, max(abs(_1015 * 38400.0f), abs(_1017 * 21600.0f)));
                float _1145 = max(_50_m0[3u].z, _50_m0[5u].z * _1137);
                float _1161;
                if (asuint(_50_m0[5u]).y == 0u)
                {
                    _1161 = _1145;
                }
                else
                {
                    _1161 = min(_1145, _50_m0[5u].w * _1137);
                }
                uint _1180;
                float _1182;
                float _1184;
                float _1186;
                uint _1188;
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
                if (_209 == 0u)
                {
                    _1180 = 0u;
                    _1182 = _1124;
                    _1184 = _1123;
                    _1186 = _1122;
                    _1188 = 0u;
                    _1190 = _975;
                    _1192 = _983;
                    _1194 = _982;
                    _1196 = _1124;
                    _1198 = _1123;
                    _1200 = _1122;
                    _1202 = 1.0f;
                    _1204 = 0u;
                    _1206 = 0.0f;
                    _1208 = 0.0f;
                    _1210 = 1u;
                }
                else
                {
                    uint _1181;
                    float _1183;
                    float _1185;
                    float _1187;
                    uint _1189;
                    uint _1384;
                    float _1191;
                    float _1193;
                    float _1195;
                    float _1197;
                    float _1199;
                    float _1201;
                    float _1203;
                    uint _1205;
                    float _1207;
                    float _1209;
                    uint _1211;
                    float _1367;
                    uint _1369;
                    float _1374;
                    float _1376;
                    float _1378;
                    float _1380;
                    float _1382;
                    uint _1365 = 0u;
                    float _1366 = _1161;
                    uint _1368 = 0u;
                    float _1370 = _1124;
                    float _1371 = _1123;
                    float _1372 = _1122;
                    float _1373 = _1118;
                    float _1375 = _1090;
                    float _1377 = _1089;
                    float _1379 = _1078;
                    float _1381 = _1077;
                    uint _1383 = 0u;
                    uint _1385 = 0u;
                    float _1386 = _975;
                    float _1387 = _983;
                    float _1388 = _982;
                    float _1389 = _1124;
                    float _1390 = _1123;
                    float _1391 = _1122;
                    float _1392 = 1.0f;
                    uint _1393 = 0u;
                    float _1394 = 0.0f;
                    float _1395 = 0.0f;
                    uint _1396 = 1u;
                    float _1397;
                    float _1398;
                    uint _1399;
                    uint _1400;
                    bool _1401;
                    for (;;)
                    {
                        _1397 = _1381 * _1372;
                        _1398 = _1379 * _1371;
                        _1399 = uint(int(_1397));
                        _1400 = uint(int(_1398));
                        _1401 = _1383 == 0u;
                        float _1657;
                        if (_1401)
                        {
                            _1657 = _12.Load(int3(uint2(_1399, _1400), 0u)).x;
                        }
                        else
                        {
                            _1657 = _15.Load(int3(uint2(_1399, _1400), _1383 + 4294967295u)).x;
                        }
                        float _1663 = ((_1397 >= floor(_1381)) || (_1398 >= floor(_1379))) ? 1.0f : _1657;
                        float _1677 = (_1022 < 0.0f) ? ((_1663 - _975) * _1088) : 3.4028234663852885981170418348452e+38f;
                        float _1679 = min(min((((floor(_1397) + _1102) * _1377) + _1112) * _1085, (((floor(_1398) + _1103) * _1375) + _1115) * _1087), _1677);
                        bool _1680 = _1663 < _1370;
                        bool _1684 = _1680 && (asuint(_1679) != asuint(_1677));
                        float _1685 = _1680 ? _1679 : _1373;
                        float _1689 = (_1685 * _1019) + _982;
                        float _1690 = (_1685 * _1021) + _983;
                        float _1691 = (_1685 * _1022) + _975;
                        uint _1693 = (_1684 ? 1u : 4294967295u) + _1383;
                        float _1694 = _1684 ? 0.5f : 2.0f;
                        float _1695 = _1694 * _1381;
                        float _1696 = _1694 * _1379;
                        float _1697 = _1684 ? 2.0f : 0.5f;
                        float _1698 = _1697 * _1377;
                        float _1699 = _1697 * _1375;
                        _1181 = _1365 + 1u;
                        uint _1894;
                        uint _1897;
                        if (int(_1693) < int(0u))
                        {
                            uint frontier_phi_90_77_ladder;
                            float frontier_phi_90_77_ladder_1;
                            float frontier_phi_90_77_ladder_2;
                            float frontier_phi_90_77_ladder_3;
                            uint frontier_phi_90_77_ladder_4;
                            float frontier_phi_90_77_ladder_5;
                            float frontier_phi_90_77_ladder_6;
                            uint frontier_phi_90_77_ladder_7;
                            float frontier_phi_90_77_ladder_8;
                            float frontier_phi_90_77_ladder_9;
                            float frontier_phi_90_77_ladder_10;
                            float frontier_phi_90_77_ladder_11;
                            float frontier_phi_90_77_ladder_12;
                            float frontier_phi_90_77_ladder_13;
                            uint frontier_phi_90_77_ladder_14;
                            uint frontier_phi_90_77_ladder_15;
                            float frontier_phi_90_77_ladder_16;
                            float frontier_phi_90_77_ladder_17;
                            float frontier_phi_90_77_ladder_18;
                            float frontier_phi_90_77_ladder_19;
                            float _1773;
                            float4 _1778;
                            float _1781;
                            float _1784;
                            bool _1785;
                            for (;;)
                            {
                                float _1771 = _50_m0[50u].w + _50_m0[50u].y;
                                _1773 = _50_m0[50u].x / (_1771 - (_50_m0[50u].y * _1663));
                                float _1776 = _50_m0[50u].x / (_1771 - (_50_m0[50u].y * _1691));
                                _1778 = _50_m0[3u];
                                _1781 = abs(_1127 - _1776);
                                _1784 = _1776 - _1773;
                                _1785 = _1784 > max(_1778.x, _1778.x * _1781);
                                if (_1785)
                                {
                                    uint _1899;
                                    if (_1368 == 0u)
                                    {
                                        uint frontier_phi_103_102_ladder;
                                        if ((_402 == 2u) || (_402 == 4u))
                                        {
                                            if ((_1685 < _1076) && (abs(_1784) < _50_m0[2u].z))
                                            {
                                                frontier_phi_90_77_ladder = 1u;
                                                frontier_phi_90_77_ladder_1 = _1366;
                                                frontier_phi_90_77_ladder_2 = _1685;
                                                frontier_phi_90_77_ladder_3 = _1699;
                                                frontier_phi_90_77_ladder_4 = _1396;
                                                frontier_phi_90_77_ladder_5 = _1689;
                                                frontier_phi_90_77_ladder_6 = _1690;
                                                frontier_phi_90_77_ladder_7 = 1u;
                                                frontier_phi_90_77_ladder_8 = 0.0f;
                                                frontier_phi_90_77_ladder_9 = _1391;
                                                frontier_phi_90_77_ladder_10 = _1390;
                                                frontier_phi_90_77_ladder_11 = _1389;
                                                frontier_phi_90_77_ladder_12 = _1387;
                                                frontier_phi_90_77_ladder_13 = _1386;
                                                frontier_phi_90_77_ladder_14 = 1u;
                                                frontier_phi_90_77_ladder_15 = _1693;
                                                frontier_phi_90_77_ladder_16 = _1695;
                                                frontier_phi_90_77_ladder_17 = _1696;
                                                frontier_phi_90_77_ladder_18 = _1698;
                                                frontier_phi_90_77_ladder_19 = _1388;
                                                break;
                                            }
                                            frontier_phi_103_102_ladder = 1u;
                                        }
                                        else
                                        {
                                            frontier_phi_103_102_ladder = 1u;
                                        }
                                        _1899 = frontier_phi_103_102_ladder;
                                    }
                                    else
                                    {
                                        _1899 = _1368;
                                    }
                                    if (!(_1393 == 0u))
                                    {
                                        frontier_phi_90_77_ladder = _1899;
                                        frontier_phi_90_77_ladder_1 = _1366;
                                        frontier_phi_90_77_ladder_2 = _1685;
                                        frontier_phi_90_77_ladder_3 = _1699;
                                        frontier_phi_90_77_ladder_4 = _1396;
                                        frontier_phi_90_77_ladder_5 = _1395;
                                        frontier_phi_90_77_ladder_6 = _1394;
                                        frontier_phi_90_77_ladder_7 = _1393;
                                        frontier_phi_90_77_ladder_8 = _1392;
                                        frontier_phi_90_77_ladder_9 = _1391;
                                        frontier_phi_90_77_ladder_10 = _1390;
                                        frontier_phi_90_77_ladder_11 = _1389;
                                        frontier_phi_90_77_ladder_12 = _1387;
                                        frontier_phi_90_77_ladder_13 = _1386;
                                        frontier_phi_90_77_ladder_14 = _1385;
                                        frontier_phi_90_77_ladder_15 = _1693;
                                        frontier_phi_90_77_ladder_16 = _1695;
                                        frontier_phi_90_77_ladder_17 = _1696;
                                        frontier_phi_90_77_ladder_18 = _1698;
                                        frontier_phi_90_77_ladder_19 = _1388;
                                        break;
                                    }
                                    float _1898 = _1685 + _1366;
                                    bool _2227 = _1385 != 0u;
                                    float _1892 = _2227 ? _1395 : _1689;
                                    float _1893 = _2227 ? _1394 : _1690;
                                    uint _1895 = ((_402 == 1u) || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1385;
                                    if (asuint(_50_m0[5u]).y == 0u)
                                    {
                                        frontier_phi_90_77_ladder = _1899;
                                        frontier_phi_90_77_ladder_1 = _1366;
                                        frontier_phi_90_77_ladder_2 = _1898;
                                        frontier_phi_90_77_ladder_3 = _1090;
                                        frontier_phi_90_77_ladder_4 = _1396;
                                        frontier_phi_90_77_ladder_5 = _1892;
                                        frontier_phi_90_77_ladder_6 = _1893;
                                        frontier_phi_90_77_ladder_7 = 0u;
                                        frontier_phi_90_77_ladder_8 = _1392;
                                        frontier_phi_90_77_ladder_9 = _1391;
                                        frontier_phi_90_77_ladder_10 = _1390;
                                        frontier_phi_90_77_ladder_11 = _1389;
                                        frontier_phi_90_77_ladder_12 = _1387;
                                        frontier_phi_90_77_ladder_13 = _1386;
                                        frontier_phi_90_77_ladder_14 = _1895;
                                        frontier_phi_90_77_ladder_15 = 0u;
                                        frontier_phi_90_77_ladder_16 = _1077;
                                        frontier_phi_90_77_ladder_17 = _1078;
                                        frontier_phi_90_77_ladder_18 = _1089;
                                        frontier_phi_90_77_ladder_19 = _1388;
                                        break;
                                    }
                                    frontier_phi_90_77_ladder = _1899;
                                    frontier_phi_90_77_ladder_1 = min(_1145, _50_m0[6u].x * _1366);
                                    frontier_phi_90_77_ladder_2 = _1898;
                                    frontier_phi_90_77_ladder_3 = _1090;
                                    frontier_phi_90_77_ladder_4 = _1396;
                                    frontier_phi_90_77_ladder_5 = _1892;
                                    frontier_phi_90_77_ladder_6 = _1893;
                                    frontier_phi_90_77_ladder_7 = 0u;
                                    frontier_phi_90_77_ladder_8 = _1392;
                                    frontier_phi_90_77_ladder_9 = _1391;
                                    frontier_phi_90_77_ladder_10 = _1390;
                                    frontier_phi_90_77_ladder_11 = _1389;
                                    frontier_phi_90_77_ladder_12 = _1387;
                                    frontier_phi_90_77_ladder_13 = _1386;
                                    frontier_phi_90_77_ladder_14 = _1895;
                                    frontier_phi_90_77_ladder_15 = 0u;
                                    frontier_phi_90_77_ladder_16 = _1077;
                                    frontier_phi_90_77_ladder_17 = _1078;
                                    frontier_phi_90_77_ladder_18 = _1089;
                                    frontier_phi_90_77_ladder_19 = _1388;
                                    break;
                                }
                                else
                                {
                                    float _1881 = max(_1778.y, _1778.y * _1781);
                                    float _1884 = _1881 * _1778.w;
                                    float _1888 = clamp((abs(_1784) - _1884) / (_1881 - _1884), 0.0f, 1.0f);
                                    uint _1890 = uint(_1773 < _1127);
                                    uint frontier_phi_90_77_ladder_89_ladder;
                                    float frontier_phi_90_77_ladder_89_ladder_1;
                                    float frontier_phi_90_77_ladder_89_ladder_2;
                                    float frontier_phi_90_77_ladder_89_ladder_3;
                                    uint frontier_phi_90_77_ladder_89_ladder_4;
                                    float frontier_phi_90_77_ladder_89_ladder_5;
                                    float frontier_phi_90_77_ladder_89_ladder_6;
                                    uint frontier_phi_90_77_ladder_89_ladder_7;
                                    float frontier_phi_90_77_ladder_89_ladder_8;
                                    float frontier_phi_90_77_ladder_89_ladder_9;
                                    float frontier_phi_90_77_ladder_89_ladder_10;
                                    float frontier_phi_90_77_ladder_89_ladder_11;
                                    float frontier_phi_90_77_ladder_89_ladder_12;
                                    float frontier_phi_90_77_ladder_89_ladder_13;
                                    uint frontier_phi_90_77_ladder_89_ladder_14;
                                    uint frontier_phi_90_77_ladder_89_ladder_15;
                                    float frontier_phi_90_77_ladder_89_ladder_16;
                                    float frontier_phi_90_77_ladder_89_ladder_17;
                                    float frontier_phi_90_77_ladder_89_ladder_18;
                                    float frontier_phi_90_77_ladder_89_ladder_19;
                                    if (_1385 == 0u)
                                    {
                                        frontier_phi_90_77_ladder_89_ladder = _1368;
                                        frontier_phi_90_77_ladder_89_ladder_1 = _1366;
                                        frontier_phi_90_77_ladder_89_ladder_2 = _1685;
                                        frontier_phi_90_77_ladder_89_ladder_3 = _1699;
                                        frontier_phi_90_77_ladder_89_ladder_4 = _1890;
                                        frontier_phi_90_77_ladder_89_ladder_5 = _1395;
                                        frontier_phi_90_77_ladder_89_ladder_6 = _1394;
                                        frontier_phi_90_77_ladder_89_ladder_7 = _1393;
                                        frontier_phi_90_77_ladder_89_ladder_8 = _1888;
                                        frontier_phi_90_77_ladder_89_ladder_9 = _1391;
                                        frontier_phi_90_77_ladder_89_ladder_10 = _1390;
                                        frontier_phi_90_77_ladder_89_ladder_11 = _1389;
                                        frontier_phi_90_77_ladder_89_ladder_12 = _1387;
                                        frontier_phi_90_77_ladder_89_ladder_13 = _1386;
                                        frontier_phi_90_77_ladder_89_ladder_14 = uint(_1888 > 0.0f);
                                        frontier_phi_90_77_ladder_89_ladder_15 = _1693;
                                        frontier_phi_90_77_ladder_89_ladder_16 = _1695;
                                        frontier_phi_90_77_ladder_89_ladder_17 = _1696;
                                        frontier_phi_90_77_ladder_89_ladder_18 = _1698;
                                        frontier_phi_90_77_ladder_89_ladder_19 = _1388;
                                    }
                                    else
                                    {
                                        frontier_phi_90_77_ladder_89_ladder = _1368;
                                        frontier_phi_90_77_ladder_89_ladder_1 = _1366;
                                        frontier_phi_90_77_ladder_89_ladder_2 = _1685;
                                        frontier_phi_90_77_ladder_89_ladder_3 = _1699;
                                        frontier_phi_90_77_ladder_89_ladder_4 = _1890;
                                        frontier_phi_90_77_ladder_89_ladder_5 = _1395;
                                        frontier_phi_90_77_ladder_89_ladder_6 = _1394;
                                        frontier_phi_90_77_ladder_89_ladder_7 = _1393;
                                        frontier_phi_90_77_ladder_89_ladder_8 = _1888;
                                        frontier_phi_90_77_ladder_89_ladder_9 = _1391;
                                        frontier_phi_90_77_ladder_89_ladder_10 = _1390;
                                        frontier_phi_90_77_ladder_89_ladder_11 = _1389;
                                        frontier_phi_90_77_ladder_89_ladder_12 = _1387;
                                        frontier_phi_90_77_ladder_89_ladder_13 = _1386;
                                        frontier_phi_90_77_ladder_89_ladder_14 = _1385;
                                        frontier_phi_90_77_ladder_89_ladder_15 = _1693;
                                        frontier_phi_90_77_ladder_89_ladder_16 = _1695;
                                        frontier_phi_90_77_ladder_89_ladder_17 = _1696;
                                        frontier_phi_90_77_ladder_89_ladder_18 = _1698;
                                        frontier_phi_90_77_ladder_89_ladder_19 = _1388;
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
                            _1211 = frontier_phi_90_77_ladder_4;
                            _1209 = frontier_phi_90_77_ladder_5;
                            _1207 = frontier_phi_90_77_ladder_6;
                            _1205 = frontier_phi_90_77_ladder_7;
                            _1203 = frontier_phi_90_77_ladder_8;
                            _1201 = frontier_phi_90_77_ladder_9;
                            _1199 = frontier_phi_90_77_ladder_10;
                            _1197 = frontier_phi_90_77_ladder_11;
                            _1195 = frontier_phi_90_77_ladder_19;
                            _1193 = frontier_phi_90_77_ladder_12;
                            _1191 = frontier_phi_90_77_ladder_13;
                            _1894 = frontier_phi_90_77_ladder_14;
                            _1897 = frontier_phi_90_77_ladder_15;
                            _1382 = frontier_phi_90_77_ladder_16;
                            _1380 = frontier_phi_90_77_ladder_17;
                            _1378 = frontier_phi_90_77_ladder_18;
                            _1376 = frontier_phi_90_77_ladder_3;
                            _1374 = frontier_phi_90_77_ladder_2;
                            _1369 = frontier_phi_90_77_ladder;
                            _1367 = frontier_phi_90_77_ladder_1;
                        }
                        else
                        {
                            bool _1786 = _1385 != 0u;
                            _1211 = _1396;
                            _1209 = _1395;
                            _1207 = _1394;
                            _1205 = _1393;
                            _1203 = _1392;
                            _1201 = _1786 ? _1391 : _1689;
                            _1199 = _1786 ? _1390 : _1690;
                            _1197 = _1786 ? _1389 : _1691;
                            _1195 = _1689;
                            _1193 = _1690;
                            _1191 = _1691;
                            _1894 = _1385;
                            _1897 = _1693;
                            _1382 = _1695;
                            _1380 = _1696;
                            _1378 = _1698;
                            _1376 = _1699;
                            _1374 = _1685;
                            _1369 = _1368;
                            _1367 = (asuint(_50_m0[5u]).y != 0u) ? _1161 : _1366;
                        }
                        float frontier_phi_122_pred;
                        uint frontier_phi_122_pred_1;
                        uint frontier_phi_122_pred_2;
                        float frontier_phi_122_pred_3;
                        float frontier_phi_122_pred_4;
                        bool _1903;
                        bool _1905;
                        for (;;)
                        {
                            _1903 = _1691 < 0.0f;
                            _1905 = _1903 || ((_1689 < 0.0f) || (_1690 < 0.0f));
                            if (!_1905)
                            {
                                if (!((_1691 > 1.0f) || ((_1689 > _50_m0[51u].x) || (_1690 > _50_m0[51u].y))))
                                {
                                    frontier_phi_122_pred = _1689;
                                    frontier_phi_122_pred_1 = _1894;
                                    frontier_phi_122_pred_2 = _1897;
                                    frontier_phi_122_pred_3 = _1690;
                                    frontier_phi_122_pred_4 = _1691;
                                    break;
                                }
                            }
                            if (!_1903)
                            {
                                frontier_phi_122_pred = _1689;
                                frontier_phi_122_pred_1 = 1u;
                                frontier_phi_122_pred_2 = 4294967295u;
                                frontier_phi_122_pred_3 = _1690;
                                frontier_phi_122_pred_4 = _1691;
                                break;
                            }
                            float _2241 = (-0.0f) - _1691;
                            float _2242 = _2241 / _1022;
                            frontier_phi_122_pred = (_2242 * _1019) + _1689;
                            frontier_phi_122_pred_1 = 1u;
                            frontier_phi_122_pred_2 = 4294967295u;
                            frontier_phi_122_pred_3 = (_2242 * _1021) + _1690;
                            frontier_phi_122_pred_4 = _2241 + _1691;
                            break;
                        }
                        _1187 = frontier_phi_122_pred;
                        _1189 = frontier_phi_122_pred_1;
                        _1384 = frontier_phi_122_pred_2;
                        _1185 = frontier_phi_122_pred_3;
                        _1183 = frontier_phi_122_pred_4;
                        if ((_1181 < _209) && (int(_1384) > int(4294967295u)))
                        {
                            _1365 = _1181;
                            _1366 = _1367;
                            _1368 = _1369;
                            _1370 = _1183;
                            _1371 = _1185;
                            _1372 = _1187;
                            _1373 = _1374;
                            _1375 = _1376;
                            _1377 = _1378;
                            _1379 = _1380;
                            _1381 = _1382;
                            _1383 = _1384;
                            _1385 = _1189;
                            _1386 = _1191;
                            _1387 = _1193;
                            _1388 = _1195;
                            _1389 = _1197;
                            _1390 = _1199;
                            _1391 = _1201;
                            _1392 = _1203;
                            _1393 = _1205;
                            _1394 = _1207;
                            _1395 = _1209;
                            _1396 = _1211;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    _1180 = _1181;
                    _1182 = _1183;
                    _1184 = _1185;
                    _1186 = _1187;
                    _1188 = _1189;
                    _1190 = _1191;
                    _1192 = _1193;
                    _1194 = _1195;
                    _1196 = _1197;
                    _1198 = _1199;
                    _1200 = _1201;
                    _1202 = _1203;
                    _1204 = _1205;
                    _1206 = _1207;
                    _1208 = _1209;
                    _1210 = _1211;
                }
                bool _1212 = _1180 >= _209;
                uint _1213 = _1212 ? 1u : _1188;
                float _1221 = _50_m0[51u].z * 2.0f;
                float _1224 = (_1221 * _982) + (-1.0f);
                float _1225 = ((1.0f - (_50_m0[51u].w * _983)) * 2.0f) + (-1.0f);
                float _1241 = mad(_190, _975, mad(_183, _1225, _1224 * _176)) + _197;
                float _1242 = (mad(_187, _975, mad(_180, _1225, _1224 * _173)) + _194) / _1241;
                float _1243 = (mad(_188, _975, mad(_181, _1225, _1224 * _174)) + _195) / _1241;
                float _1244 = (mad(_189, _975, mad(_182, _1225, _1224 * _175)) + _196) / _1241;
                float _1249 = (_1221 * _1186) + (-1.0f);
                float _1250 = ((1.0f - (_50_m0[51u].w * _1184)) * 2.0f) + (-1.0f);
                float _1266 = mad(_190, _1182, mad(_183, _1250, _1249 * _176)) + _197;
                float _1270 = ((mad(_187, _1182, mad(_180, _1250, _1249 * _173)) + _194) / _1266) - _1242;
                float _1271 = ((mad(_188, _1182, mad(_181, _1250, _1249 * _174)) + _195) / _1266) - _1243;
                float _1272 = ((mad(_189, _1182, mad(_182, _1250, _1249 * _175)) + _196) / _1266) - _1244;
                float _1302;
                uint _1304;
                float _1306;
                if (_1180 > _209)
                {
                    _1302 = 0.0f;
                    _1304 = _1213;
                    _1306 = 0.0f;
                }
                else
                {
                    float frontier_phi_49_50_ladder;
                    uint frontier_phi_49_50_ladder_1;
                    float frontier_phi_49_50_ladder_2;
                    if ((_1184 < 0.0f) || (_1186 < 0.0f))
                    {
                        frontier_phi_49_50_ladder = 0.0f;
                        frontier_phi_49_50_ladder_1 = _1213;
                        frontier_phi_49_50_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_49_50_ladder_58_ladder;
                        uint frontier_phi_49_50_ladder_58_ladder_1;
                        float frontier_phi_49_50_ladder_58_ladder_2;
                        if ((_1182 >= 1.0f) || ((_1186 > _50_m0[51u].x) || (_1184 > _50_m0[51u].y)))
                        {
                            frontier_phi_49_50_ladder_58_ladder = 0.0f;
                            frontier_phi_49_50_ladder_58_ladder_1 = _1213;
                            frontier_phi_49_50_ladder_58_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_49_50_ladder_58_ladder_67_ladder;
                            uint frontier_phi_49_50_ladder_58_ladder_67_ladder_1;
                            float frontier_phi_49_50_ladder_58_ladder_67_ladder_2;
                            for (;;)
                            {
                                if ((abs(_1186 - _259) < (2.0f / _1077)) && (abs(_1184 - _260) < (2.0f / _1078)))
                                {
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder = 0.0f;
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder_1 = _1213;
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_101;
                                    float frontier_phi_101_pred;
                                    uint frontier_phi_101_pred_1;
                                    float frontier_phi_101_pred_2;
                                    uint _1761;
                                    uint _1762;
                                    bool _1764;
                                    for (;;)
                                    {
                                        _1761 = uint(int(_1186 * _1077));
                                        _1762 = uint(int(_1184 * _1078));
                                        _1764 = (_402 == 1u) && _936;
                                        if (!_1764)
                                        {
                                            if (!(dot(float3(_1270, _1271, _1272), float3(_1270, _1271, _1272)) < _50_m0[4u].w))
                                            {
                                                ladder_phi_101 = false;
                                                frontier_phi_101_pred = 0.0f;
                                                frontier_phi_101_pred_1 = _1213;
                                                frontier_phi_101_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1867 = _24[22u].Load(int3(uint2(_1761, _1762), 0u));
                                        uint _1869 = _1867.x;
                                        float _2206;
                                        float _2207;
                                        float _2208;
                                        if (_1869 == 0u)
                                        {
                                            uint4 _1981 = _24[1u].Load(int3(uint2(_1761, _1762), 0u));
                                            uint _1983 = _1981.x;
                                            float _1991 = (float((_1983 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1992 = (float(_1983 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1996 = (1.0f - abs(_1991)) - abs(_1992);
                                            float _1998 = clamp((-0.0f) - _1996, 0.0f, 1.0f);
                                            float _1999 = (-0.0f) - _1998;
                                            _2206 = ((_1991 >= 0.0f) ? _1999 : _1998) + _1991;
                                            _2207 = ((_1992 >= 0.0f) ? _1999 : _1998) + _1992;
                                            _2208 = _1996;
                                        }
                                        else
                                        {
                                            float _2013 = (float((_1869 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2014 = (float(_1869 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2018 = (1.0f - abs(_2013)) - abs(_2014);
                                            float _2020 = clamp((-0.0f) - _2018, 0.0f, 1.0f);
                                            float _2021 = (-0.0f) - _2020;
                                            _2206 = ((_2013 >= 0.0f) ? _2021 : _2020) + _2013;
                                            _2207 = ((_2014 >= 0.0f) ? _2021 : _2020) + _2014;
                                            _2208 = _2018;
                                        }
                                        float _2212 = rsqrt(dot(float3(_2206, _2207, _2208), float3(_2206, _2207, _2208)));
                                        if (dot(float3(_2212 * _2206, _2212 * _2207, _2212 * _2208), float3(_1270, _1271, _1272)) > 0.0f)
                                        {
                                            ladder_phi_101 = true;
                                            frontier_phi_101_pred = 0.0f;
                                            frontier_phi_101_pred_1 = _1213;
                                            frontier_phi_101_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_101 = false;
                                            frontier_phi_101_pred = 0.0f;
                                            frontier_phi_101_pred_1 = _1213;
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
                                    float _2032 = _50_m0[51u].z * _1186;
                                    float _2033 = _50_m0[51u].w * _1184;
                                    float _2035 = (_1078 / _1077) * 0.0500000007450580596923828125f;
                                    float _2040 = clamp(_2032 / _2035, 0.0f, 1.0f);
                                    float _2041 = clamp(_2033 * 20.0f, 0.0f, 1.0f);
                                    float _2053 = clamp(((_2032 + (-1.0f)) + _2035) / _2035, 0.0f, 1.0f);
                                    float _2054 = clamp((_2033 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _2065 = _2040 * _2041;
                                    precise float _2066 = _2065 * _2065;
                                    float _2070 = ((((3.0f - (_2041 * 2.0f)) * (3.0f - (_2040 * 2.0f))) * _2066) * (1.0f - ((_2053 * _2053) * (3.0f - (_2053 * 2.0f))))) * (1.0f - ((_2054 * _2054) * (3.0f - (_2054 * 2.0f))));
                                    bool _2073 = (_1213 != 0u) || (_2070 >= 1.0f);
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder = _2070 * float(_481 > 0.0f);
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder_1 = _2073 ? _1213 : 1u;
                                    frontier_phi_49_50_ladder_58_ladder_67_ladder_2 = _2073 ? 0.0f : _2070;
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
                    _1302 = frontier_phi_49_50_ladder_2;
                    _1304 = frontier_phi_49_50_ladder_1;
                    _1306 = frontier_phi_49_50_ladder;
                }
                bool _1422;
                float _1425;
                float _1427;
                float _1429;
                float _1431;
                float _1434;
                float _1436;
                float _1438;
                float _1440;
                float _1441;
                float _1442;
                float _1444;
                float _1347;
                float _1350;
                float _1353;
                float _1356;
                float _1360;
                float _1361;
                for (;;)
                {
                    _1347 = ((((exp2(log2(clamp((sqrt(((_1243 * _1243) + (_1242 * _1242)) + (_1244 * _1244)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _412) * exp2(log2(clamp((_1243 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_398, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    _1350 = mad(_167, _932, mad(_161, _931, _930 * _155));
                    _1353 = mad(_168, _932, mad(_162, _931, _930 * _156));
                    _1356 = mad(_169, _932, mad(_163, _931, _930 * _157));
                    bool _1359 = (_1304 != 0u) || (_1306 < 1.0f);
                    _1360 = _1359 ? 0.0f : 1.0f;
                    _1361 = _1359 ? 0.0f : 0.5f;
                    if (_1359)
                    {
                        bool _1417 = _402 == 1u;
                        uint4 _1421 = asuint(_50_m0[60u]);
                        if (_1417)
                        {
                            if (int(_410) < int(1u))
                            {
                                if (_1421.x == 0u)
                                {
                                    _1422 = false;
                                    _1425 = 0.0f;
                                    _1427 = 0.0f;
                                    _1429 = 0.0f;
                                    _1431 = 9899999600270360182784.0f;
                                    _1434 = _1194;
                                    _1436 = _1192;
                                    _1438 = _1190;
                                    _1440 = 0.0f;
                                    _1441 = 0.0f;
                                    _1442 = 0.0f;
                                    _1444 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1421.y == 0u)
                                {
                                    _1422 = false;
                                    _1425 = 0.0f;
                                    _1427 = 0.0f;
                                    _1429 = 0.0f;
                                    _1431 = 9899999600270360182784.0f;
                                    _1434 = _1194;
                                    _1436 = _1192;
                                    _1438 = _1190;
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
                            if (_1421.z == 0u)
                            {
                                _1422 = false;
                                _1425 = 0.0f;
                                _1427 = 0.0f;
                                _1429 = 0.0f;
                                _1431 = 9899999600270360182784.0f;
                                _1434 = _1194;
                                _1436 = _1192;
                                _1438 = _1190;
                                _1440 = 0.0f;
                                _1441 = 0.0f;
                                _1442 = 0.0f;
                                _1444 = 0.0f;
                                break;
                            }
                        }
                        if (_1302 > 0.0f)
                        {
                            _1422 = false;
                            _1425 = 0.0f;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 9899999600270360182784.0f;
                            _1434 = _1194;
                            _1436 = _1192;
                            _1438 = _1190;
                            _1440 = 0.0f;
                            _1441 = 1.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        if ((_1196 <= 0.0f) || ((_1198 <= 0.0f) || (_1200 <= 0.0f)))
                        {
                            _1422 = false;
                            _1425 = 0.0f;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 9899999600270360182784.0f;
                            _1434 = _1194;
                            _1436 = _1192;
                            _1438 = _1190;
                            _1440 = 0.0f;
                            _1441 = 1.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        if ((_1196 >= 1.0f) || ((_1200 >= _50_m0[51u].x) || (_1198 >= _50_m0[51u].y)))
                        {
                            _1422 = false;
                            _1425 = 0.0f;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 9899999600270360182784.0f;
                            _1434 = _1194;
                            _1436 = _1192;
                            _1438 = _1190;
                            _1440 = 0.0f;
                            _1441 = 1.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        uint _2453;
                        uint _2455;
                        uint _2106;
                        uint _2107;
                        bool _2113;
                        for (;;)
                        {
                            _2106 = uint(clamp(_1208, 0.0f, 1.0f) * _202);
                            _2107 = uint(clamp(_1206, 0.0f, 1.0f) * _204);
                            _2113 = _20[21u].Load(int3(uint2(_2106, _2107), 0u)).x > 0.0f;
                            if (_2113)
                            {
                                uint _2315 = _24[23u].Load(int3(uint2(_2106, _2107), 0u)).y + 4294967295u;
                                _2453 = (uint(int(_2315) >> int(31u)) & 3u) + 1u;
                                _2455 = (int(_2315) < int(0u)) ? 0u : _2315;
                                break;
                            }
                            else
                            {
                                uint4 _2323 = _24[2u].Load(int3(uint2(_2106, _2107), 0u));
                                uint _2326 = _2323.w;
                                uint4 _2331 = _24[15u].Load(int3(uint2(_2106, _2107), 0u));
                                uint _2333 = _2331.y;
                                uint _2339 = ((_2333 & 64u) != 0u) ? uint((_2333 & 4294967167u) != 66u) : 4294967295u;
                                uint _2340 = _2326 & 128u;
                                uint _2342 = (_2340 != 0u) ? 1u : ((_2323.x << 7u) | _2326);
                                uint4 _2345 = _16.Load(_2342 * 4u);
                                uint _2346 = _2345.x;
                                uint _2353 = ((_2346 & 1u) != 0u) ? 0u : 18u;
                                uint _2355 = uint(min(int(uint(max(int(_2339), int(0u)))), int(1u)));
                                uint _2466;
                                if (_2340 == 0u)
                                {
                                    _2466 = (((_2346 & 2097152u) != 0u) && (_2339 == _2355)) ? (_2353 | 128u) : _2353;
                                }
                                else
                                {
                                    _2466 = _2326;
                                }
                                uint _2467 = _16.Load((_2342 * 4u) + 1u).x & 512u;
                                bool _2470 = (_2466 & 144u) == 0u;
                                if (_2467 == 0u)
                                {
                                    if (_2470 || ((_2346 & 1u) != 0u))
                                    {
                                        _2453 = 0u;
                                        _2455 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2470)
                                    {
                                        _2453 = 0u;
                                        _2455 = 0u;
                                        break;
                                    }
                                }
                                bool _2745 = ((_2466 & 128u) | _2467) != 0u;
                                uint _2454;
                                if (_2745)
                                {
                                    _2454 = 1u;
                                }
                                else
                                {
                                    _2454 = (((_2346 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_2346 & 268435472u) == 16u) && (((_2346 & 33554432u) == 0u) || _2745))
                                {
                                    _2453 = 2u;
                                    _2455 = 0u;
                                    break;
                                }
                                _2453 = _2454;
                                _2455 = (_2454 == 1u) ? _2355 : 0u;
                                break;
                            }
                        }
                        if ((_402 != _2453) || (_410 != _2455))
                        {
                            _1422 = false;
                            _1425 = 0.0f;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 9899999600270360182784.0f;
                            _1434 = _1194;
                            _1436 = _1192;
                            _1438 = _1190;
                            _1440 = 0.0f;
                            _1441 = 1.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2587 = _1200 * 2.0f;
                        float _2590 = (_50_m0[51u].z * _2587) + (-1.0f);
                        float _2591 = ((1.0f - (_50_m0[51u].w * _1198)) * 2.0f) + (-1.0f);
                        float _2607 = mad(_190, _1196, mad(_183, _2591, _2590 * _176)) + _197;
                        float _2608 = (mad(_187, _1196, mad(_180, _2591, _2590 * _173)) + _194) / _2607;
                        float _2609 = (mad(_188, _1196, mad(_181, _2591, _2590 * _174)) + _195) / _2607;
                        float _2610 = (mad(_189, _1196, mad(_182, _2591, _2590 * _175)) + _196) / _2607;
                        if (sqrt(((_2609 * _2609) + (_2608 * _2608)) + (_2610 * _2610)) > _50_m0[58u].w)
                        {
                            _1422 = false;
                            _1425 = 0.0f;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 9899999600270360182784.0f;
                            _1434 = _1194;
                            _1436 = _1192;
                            _1438 = _1190;
                            _1440 = 0.0f;
                            _1441 = 0.0f;
                            _1442 = 1000.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2725 = _2608 - _1242;
                        float _2726 = _2609 - _1243;
                        float _2727 = _2610 - _1244;
                        float _2733 = sqrt(((_2726 * _2726) + (_2725 * _2725)) + (_2727 * _2727));
                        float _2741 = min(_50_m0[59u].y, max(0.0f, _2733 + (-0.001000000047497451305389404296875f)));
                        float _2772;
                        if (_1417)
                        {
                            _2772 = min(_50_m0[59u].x, _50_m0[4u].z + _2741);
                        }
                        else
                        {
                            _2772 = _50_m0[59u].x;
                        }
                        float _2773 = _2772 - _2733;
                        if (!(_2773 > 0.0f))
                        {
                            _1422 = false;
                            _1425 = 0.0f;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 9899999600270360182784.0f;
                            _1434 = _1194;
                            _1436 = _1192;
                            _1438 = _1190;
                            _1440 = 1.0f;
                            _1441 = 1.0f;
                            _1442 = 0.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2833 = _2608 - (_2741 * _1350);
                        float _2834 = _2609 - (_2741 * _1353);
                        float _2835 = _2610 - (_2741 * _1356);
                        RayDesc _2ident = {float3(mad(_2835, _50_m0[46u].z, mad(_2834, _50_m0[46u].y, _50_m0[46u].x * _2833)) + _50_m0[46u].w, mad(_2835, _50_m0[47u].z, mad(_2834, _50_m0[47u].y, _50_m0[47u].x * _2833)) + _50_m0[47u].w, mad(_2835, _50_m0[48u].z, mad(_2834, _50_m0[48u].y, _50_m0[48u].x * _2833)) + _50_m0[48u].w), 0.0f, float3(mad(_1356, _50_m0[46u].z, mad(_1353, _50_m0[46u].y, _50_m0[46u].x * _1350)), mad(_1356, _50_m0[47u].z, mad(_1353, _50_m0[47u].y, _50_m0[47u].x * _1350)), mad(_1356, _50_m0[48u].z, mad(_1353, _50_m0[48u].y, _50_m0[48u].x * _1350))), _2773};
                        _2838.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2885 = _2838.Proceed();
                        uint _2886 = _2838.CommittedStatus();
                        if (!(_2886 == 1u))
                        {
                            _1422 = false;
                            _1425 = 0.0f;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 9899999600270360182784.0f;
                            _1434 = _1194;
                            _1436 = _1192;
                            _1438 = _1190;
                            _1440 = 1.0f;
                            _1441 = 0.0f;
                            _1442 = 0.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2900 = _2838.CommittedRayT();
                        if (!((_2900 < _2773) && (_2900 > 0.0f)))
                        {
                            _1422 = false;
                            _1425 = 0.0f;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 9899999600270360182784.0f;
                            _1434 = _1194;
                            _1436 = _1192;
                            _1438 = _1190;
                            _1440 = 1.0f;
                            _1441 = 0.0f;
                            _1442 = 0.0f;
                            _1444 = 0.5f;
                            break;
                        }
                        float _2914 = (_50_m0[51u].z * _2587) + (-1.0f);
                        float _2915 = ((1.0f - (_50_m0[51u].w * _1198)) * 2.0f) + (-1.0f);
                        float _2931 = mad(_144, _1196, mad(_137, _2915, _2914 * _130)) + _151;
                        float _2935 = _2900 - _2741;
                        float _2939 = ((mad(_141, _1196, mad(_134, _2915, _2914 * _127)) + _148) / _2931) + (_2935 * _930);
                        float _2940 = ((mad(_142, _1196, mad(_135, _2915, _2914 * _128)) + _149) / _2931) + (_2935 * _931);
                        float _2941 = ((mad(_143, _1196, mad(_136, _2915, _2914 * _129)) + _150) / _2931) + (_2935 * _932);
                        float _2957 = mad(_116, _2941, mad(_109, _2940, _2939 * _102)) + _123;
                        float _1439 = (mad(_115, _2941, mad(_108, _2940, _2939 * _101)) + _122) / _2957;
                        float _1435 = ((((mad(_113, _2941, mad(_106, _2940, _2939 * _99)) + _120) / _2957) * 0.5f) + 0.5f) * _50_m0[51u].x;
                        float _1437 = (0.5f - (((mad(_114, _2941, mad(_107, _2940, _2939 * _100)) + _121) / _2957) * 0.5f)) * _50_m0[51u].y;
                        float _2966 = _1435 * _50_m0[51u].z;
                        float _2967 = _1437 * _50_m0[51u].w;
                        float _2969 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2972 = clamp(_2966 / _2969, 0.0f, 1.0f);
                        float _2973 = clamp(_2967 * 20.0f, 0.0f, 1.0f);
                        float _2983 = clamp(((_2966 + (-1.0f)) + _2969) / _2969, 0.0f, 1.0f);
                        float _2984 = clamp((_2967 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2995 = _2972 * _2973;
                        precise float _2996 = _2995 * _2995;
                        if ((((((3.0f - (_2973 * 2.0f)) * (3.0f - (_2972 * 2.0f))) * _2996) * (1.0f - ((_2983 * _2983) * (3.0f - (_2983 * 2.0f))))) * (1.0f - ((_2984 * _2984) * (3.0f - (_2984 * 2.0f))))) < 1.0f)
                        {
                            _1422 = false;
                            _1425 = 0.0f;
                            _1427 = 0.0f;
                            _1429 = 0.0f;
                            _1431 = 9899999600270360182784.0f;
                            _1434 = _1435;
                            _1436 = _1437;
                            _1438 = _1439;
                            _1440 = _1360;
                            _1441 = _1360;
                            _1442 = 0.0f;
                            _1444 = _1361;
                            break;
                        }
                        _1422 = true;
                        _1425 = _50_m0[1u].w;
                        _1427 = _50_m0[2u].x;
                        _1429 = _50_m0[2u].y;
                        _1431 = (_2733 - _2741) + _2900;
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
                        _1422 = false;
                        _1425 = 0.0f;
                        _1427 = 0.0f;
                        _1429 = 0.0f;
                        _1431 = 9899999600270360182784.0f;
                        _1434 = _1194;
                        _1436 = _1192;
                        _1438 = _1190;
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
                float _1480;
                float _1482;
                float _1484;
                if ((_1306 >= 1.0f) || _1422)
                {
                    _1480 = _1434;
                    _1482 = _1436;
                    _1484 = _1438;
                }
                else
                {
                    float _1635 = (-0.0f) - _975;
                    float _1636 = _1635 / _1022;
                    float _1639 = (_1636 * _1019) + _982;
                    float _1640 = (_1636 * _1021) + _983;
                    float _1641 = _1635 + _975;
                    _1480 = ((_1434 - _1639) * _1306) + _1639;
                    _1482 = ((_1436 - _1640) * _1306) + _1640;
                    _1484 = ((_1438 - _1641) * _1306) + _1641;
                }
                float _1495 = ((_1480 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _1496 = ((1.0f - (_50_m0[51u].w * _1482)) * 2.0f) + (-1.0f);
                float _1512 = mad(_144, _1484, mad(_137, _1496, _1495 * _130)) + _151;
                float _1516 = ((mad(_141, _1484, mad(_134, _1496, _1495 * _127)) + _148) / _1512) - _954;
                float _1517 = ((mad(_142, _1484, mad(_135, _1496, _1495 * _128)) + _149) / _1512) - _955;
                float _1518 = ((mad(_143, _1484, mad(_136, _1496, _1495 * _129)) + _150) / _1512) - _956;
                float _1524 = sqrt(((_1517 * _1517) + (_1516 * _1516)) + (_1518 * _1518));
                float _1525 = _1524 * _784;
                float _1526 = _1524 * _785;
                float _1527 = _1524 * _786;
                float _1528 = _1524 * (_897 / _900);
                float _1529 = _1524 * (_898 / _900);
                float _1530 = _1524 * (_899 / _900);
                float _1534 = dot(float3(_1525, _1526, _1527), float3(_770, _773, _776)) * 2.0f;
                float _1544 = dot(float3(_1528, _1529, _1530), float3(_770, _773, _776)) * 2.0f;
                float _1571 = (_1525 - (_1534 * _770)) + _954;
                float _1572 = (_1526 - (_1534 * _773)) + _955;
                float _1573 = (_1527 - (_1534 * _776)) + _956;
                float _1585 = mad(_50_m0[24u].w, _1573, mad(_50_m0[23u].w, _1572, _1571 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1590 = (_1528 - (_1544 * _770)) + _954;
                float _1591 = (_1529 - (_1544 * _773)) + _955;
                float _1592 = (_1530 - (_1544 * _776)) + _956;
                float _1604 = mad(_50_m0[24u].w, _1592, mad(_50_m0[23u].w, _1591, _1590 * _50_m0[22u].w)) + _50_m0[25u].w;
                float _1610 = (_50_m0[51u].x * ((((mad(_50_m0[24u].x, _1573, mad(_50_m0[23u].x, _1572, _1571 * _50_m0[22u].x)) + _50_m0[25u].x) / _1585) - ((mad(_50_m0[24u].x, _1592, mad(_50_m0[23u].x, _1591, _1590 * _50_m0[22u].x)) + _50_m0[25u].x) / _1604)) * 0.5f)) * _1449;
                float _1614 = (_50_m0[51u].y * ((((mad(_50_m0[24u].y, _1592, mad(_50_m0[23u].y, _1591, _1590 * _50_m0[22u].y)) + _50_m0[25u].y) / _1604) - ((mad(_50_m0[24u].y, _1573, mad(_50_m0[23u].y, _1572, _1571 * _50_m0[22u].y)) + _50_m0[25u].y) / _1585)) * 0.5f)) * _1451;
                float _1633 = clamp(log2((sqrt((_1614 * _1614) + (_1610 * _1610)) * 2.0f) / _50_m0[0u].w) / float(asuint(_50_m0[55u]).x + 4294967295u), 0.0f, 1.0f);
                bool _1634 = _402 == 1u;
                float _1754;
                if (_1634)
                {
                    float _1753 = _50_m0[4u].z - _50_m0[4u].y;
                    float _1926;
                    if (_1422)
                    {
                        float _1838 = clamp((_1431 - _50_m0[4u].y) / _1753, 0.0f, 1.0f);
                        _1926 = (_1838 * _1838) * (3.0f - (_1838 * 2.0f));
                    }
                    else
                    {
                        float _1851 = clamp((sqrt(((_1271 * _1271) + (_1270 * _1270)) + (_1272 * _1272)) - _50_m0[4u].y) / _1753, 0.0f, 1.0f);
                        _1926 = (_1851 * _1851) * (3.0f - (_1851 * 2.0f));
                    }
                    _1754 = _50_m0[61u].z * (1.0f - _1926);
                }
                else
                {
                    _1754 = 1.0f;
                }
                float _1756 = _1754 * _1306;
                bool _1757 = _402 != 1u;
                float _2357;
                float _2359;
                float _2361;
                float _2363;
                if (_1756 == 0.0f)
                {
                    float _2162;
                    float _2163;
                    float _2164;
                    float _2165;
                    if (_1422)
                    {
                        float frontier_phi_113_95_ladder;
                        float frontier_phi_113_95_ladder_1;
                        float frontier_phi_113_95_ladder_2;
                        float frontier_phi_113_95_ladder_3;
                        if (_1757)
                        {
                            float _2114 = _1353 * _1347;
                            float _2118 = rsqrt(dot(float3(_1350, _2114, _1356), float3(_1350, _2114, _1356)));
                            float4 _2128 = _28[4u].SampleLevel(_59, float3(_2118 * _1350, _2118 * _2114, _2118 * _1356), 0.0f);
                            float _2130 = _2128.x;
                            float _2131 = _2128.y;
                            float _2132 = _2128.z;
                            frontier_phi_113_95_ladder = 1.0f;
                            frontier_phi_113_95_ladder_1 = ((_1429 - _2132) * _1754) + _2132;
                            frontier_phi_113_95_ladder_2 = ((_1427 - _2131) * _1754) + _2131;
                            frontier_phi_113_95_ladder_3 = ((_1425 - _2130) * _1754) + _2130;
                        }
                        else
                        {
                            frontier_phi_113_95_ladder = _1754;
                            frontier_phi_113_95_ladder_1 = _1754 * _1429;
                            frontier_phi_113_95_ladder_2 = _1754 * _1427;
                            frontier_phi_113_95_ladder_3 = _1754 * _1425;
                        }
                        _2162 = frontier_phi_113_95_ladder_3;
                        _2163 = frontier_phi_113_95_ladder_2;
                        _2164 = frontier_phi_113_95_ladder_1;
                        _2165 = frontier_phi_113_95_ladder;
                    }
                    else
                    {
                        float frontier_phi_113_96_ladder;
                        float frontier_phi_113_96_ladder_1;
                        float frontier_phi_113_96_ladder_2;
                        float frontier_phi_113_96_ladder_3;
                        if (_1757)
                        {
                            float _2145 = _1353 * _1347;
                            float _2149 = rsqrt(dot(float3(_1350, _2145, _1356), float3(_1350, _2145, _1356)));
                            float4 _2157 = _28[4u].SampleLevel(_59, float3(_2149 * _1350, _2149 * _2145, _2149 * _1356), 0.0f);
                            frontier_phi_113_96_ladder = 1.0f;
                            frontier_phi_113_96_ladder_1 = _2157.z;
                            frontier_phi_113_96_ladder_2 = _2157.y;
                            frontier_phi_113_96_ladder_3 = _2157.x;
                        }
                        else
                        {
                            frontier_phi_113_96_ladder = 0.0f;
                            frontier_phi_113_96_ladder_1 = 0.0f;
                            frontier_phi_113_96_ladder_2 = 0.0f;
                            frontier_phi_113_96_ladder_3 = 0.0f;
                        }
                        _2162 = frontier_phi_113_96_ladder_3;
                        _2163 = frontier_phi_113_96_ladder_2;
                        _2164 = frontier_phi_113_96_ladder_1;
                        _2165 = frontier_phi_113_96_ladder;
                    }
                    _2357 = _2162 * _481;
                    _2359 = _2163 * _481;
                    _2361 = _2164 * _481;
                    _2363 = _2165 * _481;
                }
                else
                {
                    float _2185;
                    float _2187;
                    float _2192;
                    float _2196;
                    if (_12.Load(int3(uint2(uint(_1449 * _1186), uint(_1451 * _1184)), 0u)).x > 0.0f)
                    {
                        uint _1933_dummy_parameter;
                        uint2 _1933 = spvTextureSize(_14, 0u, _1933_dummy_parameter);
                        float4 _1942 = _14.Load(int3(uint2(uint(float(_1933.x) * _1186), uint(float(_1933.y) * _1184)), 0u));
                        float _1946 = _1942.x * 0.5f;
                        float _1947 = _1942.y * (-0.5f);
                        float4 _1966 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _1946) + (_50_m0[52u].x * _1186), (_50_m0[52u].w * _1947) + (_50_m0[52u].y * _1184)), 0.0f);
                        float _2181;
                        if (_1634)
                        {
                            float frontier_phi_115_114_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _975))) < 5.0f)
                            {
                                float _2417 = sqrt((_1946 * _1946) + (_1947 * _1947));
                                float frontier_phi_115_114_ladder_129_ladder;
                                if (_2417 > 0.0500000007450580596923828125f)
                                {
                                    float _2478 = _1186 - _982;
                                    float _2479 = _1184 - _983;
                                    float frontier_phi_115_114_ladder_129_ladder_143_ladder;
                                    if (_2417 > sqrt((_2478 * _2478) + (_2479 * _2479)))
                                    {
                                        uint4 _2633 = asuint(_55_m0[0u]);
                                        uint _2640 = uint(float(_2633.x) * _1186);
                                        uint _2641 = uint(float(_2633.y) * _1184);
                                        uint4 _2644 = _24[2u].Load(int3(uint2(_2640, _2641), 0u));
                                        uint _2647 = _2644.w;
                                        uint4 _2652 = _24[15u].Load(int3(uint2(_2640, _2641), 0u));
                                        uint _2654 = _2652.y;
                                        uint _2660 = ((_2654 & 64u) != 0u) ? uint((_2654 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2661 = _2647 & 128u;
                                        uint _2663 = (_2661 != 0u) ? 1u : ((_2644.x << 7u) | _2647);
                                        uint4 _2666 = _16.Load(_2663 * 4u);
                                        uint _2667 = _2666.x;
                                        uint _2674 = ((_2667 & 1u) != 0u) ? 0u : 18u;
                                        uint _2762;
                                        if (_2661 == 0u)
                                        {
                                            _2762 = (((_2667 & 2097152u) != 0u) && (_2660 == uint(min(int(uint(max(int(_2660), int(0u)))), int(1u))))) ? (_2674 | 128u) : _2674;
                                        }
                                        else
                                        {
                                            _2762 = _2647;
                                        }
                                        float frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder;
                                        if (((_2762 & 128u) | (_16.Load((_2663 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2788 = asuint(_55_m0[0u]);
                                            uint _2797 = uint(float(_2788.x) * (_1946 + _1186));
                                            uint _2798 = uint(float(_2788.y) * (_1947 + _1184));
                                            uint4 _2801 = _24[2u].Load(int3(uint2(_2797, _2798), 0u));
                                            uint _2804 = _2801.w;
                                            uint4 _2807 = _24[15u].Load(int3(uint2(_2797, _2798), 0u));
                                            uint _2809 = _2807.y;
                                            uint _2815 = ((_2809 & 64u) != 0u) ? uint((_2809 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2816 = _2804 & 128u;
                                            uint _2818 = (_2816 != 0u) ? 1u : ((_2801.x << 7u) | _2804);
                                            uint4 _2820 = _16.Load(_2818 * 4u);
                                            uint _2821 = _2820.x;
                                            uint _2828 = ((_2821 & 1u) != 0u) ? 0u : 18u;
                                            uint _2897;
                                            if (_2816 == 0u)
                                            {
                                                _2897 = (((_2821 & 2097152u) != 0u) && (_2815 == uint(min(int(uint(max(int(_2815), int(0u)))), int(1u))))) ? (_2828 | 128u) : _2828;
                                            }
                                            else
                                            {
                                                _2897 = _2804;
                                            }
                                            float frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder_172_ladder;
                                            if ((_2897 & 128u) == 0u)
                                            {
                                                frontier_phi_115_114_ladder_129_ladder_143_ladder_163_ladder_172_ladder = ((_16.Load((_2818 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
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
                            _2181 = frontier_phi_115_114_ladder;
                        }
                        else
                        {
                            _2181 = 1.0f;
                        }
                        float _2183 = _2181 * _1756;
                        float _2191;
                        float _2195;
                        float _2199;
                        if (_1204 == 0u)
                        {
                            _2191 = _50_m0[54u].x * _1966.x;
                            _2195 = _50_m0[54u].x * _1966.y;
                            _2199 = _50_m0[54u].x * _1966.z;
                        }
                        else
                        {
                            _2191 = _50_m0[1u].w;
                            _2195 = _50_m0[2u].x;
                            _2199 = _50_m0[2u].y;
                        }
                        float frontier_phi_116_130_ladder;
                        float frontier_phi_116_130_ladder_1;
                        float frontier_phi_116_130_ladder_2;
                        float frontier_phi_116_130_ladder_3;
                        for (;;)
                        {
                            if (_1202 > 0.0f)
                            {
                                float _2190;
                                float _2194;
                                float _2198;
                                if (_1634)
                                {
                                    _2190 = 0.0f;
                                    _2194 = 0.0f;
                                    _2198 = 0.0f;
                                }
                                else
                                {
                                    if (!((_402 != 4u) || (_1210 != 0u)))
                                    {
                                        frontier_phi_116_130_ladder = _2199;
                                        frontier_phi_116_130_ladder_1 = _2195;
                                        frontier_phi_116_130_ladder_2 = _2191;
                                        frontier_phi_116_130_ladder_3 = _2183;
                                        break;
                                    }
                                    _2190 = _2191;
                                    _2194 = _2195;
                                    _2198 = _2199;
                                }
                                frontier_phi_116_130_ladder = _2198;
                                frontier_phi_116_130_ladder_1 = _2194;
                                frontier_phi_116_130_ladder_2 = _2190;
                                frontier_phi_116_130_ladder_3 = (1.0f - exp2(log2(_1202) * _50_m0[4u].x)) * _2183;
                                break;
                            }
                            else
                            {
                                frontier_phi_116_130_ladder = _2199;
                                frontier_phi_116_130_ladder_1 = _2195;
                                frontier_phi_116_130_ladder_2 = _2191;
                                frontier_phi_116_130_ladder_3 = _2183;
                                break;
                            }
                        }
                        _2185 = frontier_phi_116_130_ladder_3;
                        _2187 = frontier_phi_116_130_ladder_2;
                        _2192 = frontier_phi_116_130_ladder_1;
                        _2196 = frontier_phi_116_130_ladder;
                    }
                    else
                    {
                        float frontier_phi_116_98_ladder;
                        float frontier_phi_116_98_ladder_1;
                        float frontier_phi_116_98_ladder_2;
                        float frontier_phi_116_98_ladder_3;
                        if (_1212)
                        {
                            frontier_phi_116_98_ladder = _2189;
                            frontier_phi_116_98_ladder_1 = _2189;
                            frontier_phi_116_98_ladder_2 = _2189;
                            frontier_phi_116_98_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float4 _2204 = _28[7u].SampleLevel(_59, float3(_1350, _1353, _1356), 0.0f);
                            frontier_phi_116_98_ladder = _2204.z;
                            frontier_phi_116_98_ladder_1 = _2204.y;
                            frontier_phi_116_98_ladder_2 = _2204.x;
                            frontier_phi_116_98_ladder_3 = _1756;
                        }
                        _2185 = frontier_phi_116_98_ladder_3;
                        _2187 = frontier_phi_116_98_ladder_2;
                        _2192 = frontier_phi_116_98_ladder_1;
                        _2196 = frontier_phi_116_98_ladder;
                    }
                    float _2436;
                    float _2437;
                    float _2438;
                    float _2439;
                    if (_1422)
                    {
                        _2436 = _1754;
                        _2437 = ((_2187 - _1425) * _1302) + _1425;
                        _2438 = ((_2192 - _1427) * _1302) + _1427;
                        _2439 = ((_2196 - _1429) * _1302) + _1429;
                    }
                    else
                    {
                        _2436 = _2185;
                        _2437 = _2187;
                        _2438 = _2192;
                        _2439 = _2196;
                    }
                    float _2511;
                    float _2512;
                    float _2513;
                    float _2514;
                    if (_1757 && (_2436 < 1.0f))
                    {
                        float _2485 = _1353 * _1347;
                        float _2489 = rsqrt(dot(float3(_1350, _2485, _1356), float3(_1350, _2485, _1356)));
                        float4 _2497 = _28[4u].SampleLevel(_59, float3(_2489 * _1350, _2489 * _2485, _2489 * _1356), 0.0f);
                        float _2499 = _2497.x;
                        float _2500 = _2497.y;
                        float _2501 = _2497.z;
                        _2511 = 1.0f;
                        _2512 = ((_2437 - _2499) * _2436) + _2499;
                        _2513 = ((_2438 - _2500) * _2436) + _2500;
                        _2514 = ((_2439 - _2501) * _2436) + _2501;
                    }
                    else
                    {
                        _2511 = _2436;
                        _2512 = _2437;
                        _2513 = _2438;
                        _2514 = _2439;
                    }
                    float _2364 = _2511 * _481;
                    _2357 = _2512 * _2364;
                    _2359 = _2513 * _2364;
                    _2361 = _2514 * _2364;
                    _2363 = _2364;
                }
                float _2368 = _50_m0[58u].z * _1444;
                float _2390 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _2392 = _2390 * ((_2368 * ((_1440 * 1000.0f) - _2357)) + _2357);
                float _2393 = _2390 * ((_2368 * ((_1441 * 1000.0f) - _2359)) + _2359);
                float _2394 = _2390 * ((_2368 * (_1442 - _2361)) + _2361);
                float _2400 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2392, max(_2393, _2394)) + 1.0f);
                float _2404 = min(_2400 * _2392, 0.996078431606292724609375f);
                float _2406 = min(_2400 * _2393, 0.996078431606292724609375f);
                float _2407 = min(_2400 * _2394, 0.996078431606292724609375f);
                _35[uint2(_226, _229)] = float4(_2404, _2406, _2407, _2363);
                _39[uint2(_226, _229)] = float4(_1633, _406, _2363, _1633);
                if (_233)
                {
                    uint _2471 = _226 + 1u;
                    _35[uint2(_2471, _229)] = float4(_2404, _2406, _2407, _2363);
                    _39[uint2(_2471, _229)] = float4(_1633, _406, _2363, _1633);
                }
                if (_236)
                {
                    uint _2624 = _229 + 1u;
                    _35[uint2(_226, _2624)] = float4(_2404, _2406, _2407, _2363);
                    _39[uint2(_226, _2624)] = float4(_1633, _406, _2363, _1633);
                }
                if (_237)
                {
                    uint _2746 = _226 + 1u;
                    uint _2747 = _229 + 1u;
                    _35[uint2(_2746, _2747)] = float4(_2404, _2406, _2407, _2363);
                    _39[uint2(_2746, _2747)] = float4(_1633, _406, _2363, _1633);
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
