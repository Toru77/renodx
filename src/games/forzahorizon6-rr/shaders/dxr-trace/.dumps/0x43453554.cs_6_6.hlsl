static float _1589;
static uint _2954;
static float _2955;
static float _2956;
static uint _2957;
static float _2958;
static float _2959;
static float _2960;
static float _2961;
static uint _2962;
static uint _2963;
static float _2964;
static float _2965;
static float _2966;
static float _2967;
static float _2968;
static uint _2969;
static float _2970;
static float _2976;
static uint _2977;
static float _2978;
static float _2983;
static float _2984;
static float _2985;

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

static RayQuery<RAY_FLAG_NONE> _2376;

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
                    uint _434;
                    uint _435;
                    if (_373 == 0u)
                    {
                        _434 = (((_381 & 2097152u) != 0u) && (_372 == _394)) ? (_392 | 128u) : _392;
                        _435 = _381;
                    }
                    else
                    {
                        _434 = _355;
                        _435 = _381 | ((_354 << 20u) & 134217728u);
                    }
                    uint _444 = _385 & 512u;
                    float _1104;
                    float _1106;
                    float _1108;
                    float _1110;
                    float _1112;
                    float _1114;
                    float _1116;
                    float _1118;
                    float _1120;
                    float _1122;
                    float _1124;
                    float _1126;
                    if (_444 == 0u)
                    {
                        if (!((_435 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _798 = asfloat(_17.Load((_389 * 115u) + 33u).x);
                        uint4 _806 = _24[2u].Load(int3(uint2(_270, _271), 0u));
                        uint _808 = _806.y;
                        uint _809 = _434 & 128u;
                        uint _1092;
                        uint _1093;
                        uint _1094;
                        uint _1095;
                        if (_809 == 0u)
                        {
                            _1092 = uint(((_435 & 817889384u) | (_385 & 576u)) != 0u) | (((_435 >> 19u) & 1u) ^ 1u);
                            _1093 = uint(((_435 & 17825808u) | (_385 & 520u)) != 0u);
                            _1094 = uint(((_435 & 46137344u) | (_385 & 2564u)) != 0u);
                            _1095 = 0u;
                        }
                        else
                        {
                            _1092 = 1u;
                            _1093 = _434 & 1u;
                            _1094 = 1u;
                            _1095 = 1u;
                        }
                        precise float _1099 = float(_808 & 127u) * 0.0078740157186985015869140625f;
                        bool _1103 = (_435 & 4194304u) == 0u;
                        float _1195;
                        if (_1103)
                        {
                            _1195 = _1099;
                        }
                        else
                        {
                            _1195 = float(_808 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1365;
                        if ((_435 & 134217728u) == 0u)
                        {
                            uint frontier_phi_63_49_ladder;
                            if ((_809 != 0u) || ((_435 & 17825792u) == 1048576u))
                            {
                                frontier_phi_63_49_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_63_49_ladder = _1093;
                            }
                            _1365 = frontier_phi_63_49_ladder;
                        }
                        else
                        {
                            _1365 = _1093;
                        }
                        uint4 _1368 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _1370 = _1368.x;
                        float _1490;
                        float _1492;
                        float _1494;
                        if (_1092 == 0u)
                        {
                            _1490 = 0.0f;
                            _1492 = 0.0f;
                            _1494 = 0.0f;
                        }
                        else
                        {
                            float4 _1499 = _20[8u].Load(int3(uint2(_270, _271), 0u));
                            _1490 = _1499.x;
                            _1492 = _1499.y;
                            _1494 = _1499.z;
                        }
                        uint _1718;
                        if (_1365 == 0u)
                        {
                            _1718 = 0u;
                        }
                        else
                        {
                            _1718 = _24[9u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        uint _1867;
                        if (_1094 == 0u)
                        {
                            _1867 = 0u;
                        }
                        else
                        {
                            _1867 = _24[10u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        float _1877 = (float((_1370 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1878 = (float(_1370 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1882 = (1.0f - abs(_1877)) - abs(_1878);
                        float _1884 = clamp((-0.0f) - _1882, 0.0f, 1.0f);
                        float _1885 = (-0.0f) - _1884;
                        float _1890 = ((_1877 >= 0.0f) ? _1885 : _1884) + _1877;
                        float _1891 = ((_1878 >= 0.0f) ? _1885 : _1884) + _1878;
                        float _1895 = rsqrt(dot(float3(_1890, _1891, _1882), float3(_1890, _1891, _1882)));
                        float _1896 = _1890 * _1895;
                        float _1897 = _1891 * _1895;
                        float _1898 = _1895 * _1882;
                        float _1105 = float(_1370 & 255u);
                        float _1902 = ((_435 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _2123;
                        float _2124;
                        float _2125;
                        float _2126;
                        uint _2127;
                        if ((_385 & 64u) == 0u)
                        {
                            float frontier_phi_135_124_ladder;
                            float frontier_phi_135_124_ladder_1;
                            float frontier_phi_135_124_ladder_2;
                            float frontier_phi_135_124_ladder_3;
                            uint frontier_phi_135_124_ladder_4;
                            if ((_435 & 276824064u) == 0u)
                            {
                                frontier_phi_135_124_ladder = 0.0f;
                                frontier_phi_135_124_ladder_1 = ((_435 & 8u) != 0u) ? _1492 : _1902;
                                frontier_phi_135_124_ladder_2 = 0.0f;
                                frontier_phi_135_124_ladder_3 = 0.0f;
                                frontier_phi_135_124_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_135_124_ladder = 0.0f;
                                frontier_phi_135_124_ladder_1 = _1902;
                                frontier_phi_135_124_ladder_2 = 0.0f;
                                frontier_phi_135_124_ladder_3 = 0.0f;
                                frontier_phi_135_124_ladder_4 = 0u;
                            }
                            _2123 = frontier_phi_135_124_ladder_1;
                            _2124 = frontier_phi_135_124_ladder_2;
                            _2125 = frontier_phi_135_124_ladder;
                            _2126 = frontier_phi_135_124_ladder_3;
                            _2127 = frontier_phi_135_124_ladder_4;
                        }
                        else
                        {
                            float _2018 = (_1492 * 2.0f) + (-1.0f);
                            float _2019 = (_1494 * 2.0f) + (-1.0f);
                            float _2023 = (1.0f - abs(_2018)) - abs(_2019);
                            float _2025 = clamp((-0.0f) - _2023, 0.0f, 1.0f);
                            float _2026 = (-0.0f) - _2025;
                            float _2031 = ((_2018 >= 0.0f) ? _2026 : _2025) + _2018;
                            float _2032 = ((_2019 >= 0.0f) ? _2026 : _2025) + _2019;
                            float _2036 = rsqrt(dot(float3(_2031, _2032, _2023), float3(_2031, _2032, _2023)));
                            _2123 = floor(round(_1490 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _2124 = _2031 * _2036;
                            _2125 = _2032 * _2036;
                            _2126 = _2036 * _2023;
                            _2127 = 1u;
                        }
                        float _1113;
                        if ((_435 & 32768u) == 0u)
                        {
                            _1113 = _2123;
                        }
                        else
                        {
                            float frontier_phi_142_143_ladder;
                            if (_17.Load((_389 * 115u) + 36u).x == 0u)
                            {
                                float _2329 = clamp((_1105 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _798;
                                frontier_phi_142_143_ladder = ((_435 & 131072u) != 0u) ? _2329 : ((((clamp((1.21000003814697265625f / (exp2((_1195 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_389 * 115u) + 32u).x)) + 1.0f) * _2329);
                            }
                            else
                            {
                                frontier_phi_142_143_ladder = _798;
                            }
                            _1113 = frontier_phi_142_143_ladder;
                        }
                        uint _2196 = _434 & 1u;
                        float _2272;
                        float _2274;
                        float _2276;
                        uint _2278;
                        if (((_435 & 16u) == 0u) || (((_2196 | (_385 & 8u)) | (_435 & 16777216u)) != 0u))
                        {
                            _2272 = _2124;
                            _2274 = _2125;
                            _2276 = _2126;
                            _2278 = _2127;
                        }
                        else
                        {
                            float _2288 = (float(_1718 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2289 = (float(_1718 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2293 = (1.0f - abs(_2288)) - abs(_2289);
                            float _2295 = clamp((-0.0f) - _2293, 0.0f, 1.0f);
                            float _2296 = (-0.0f) - _2295;
                            float _2301 = ((_2288 >= 0.0f) ? _2296 : _2295) + _2288;
                            float _2302 = ((_2289 >= 0.0f) ? _2296 : _2295) + _2289;
                            float _2306 = rsqrt(dot(float3(_2301, _2302, _2293), float3(_2301, _2302, _2293)));
                            _2272 = _2301 * _2306;
                            _2274 = _2302 * _2306;
                            _2276 = _2306 * _2293;
                            _2278 = 1u;
                        }
                        float _1107;
                        float _1109;
                        float _1111;
                        if (_2196 == 0u)
                        {
                            float frontier_phi_157_156_ladder;
                            float frontier_phi_157_156_ladder_1;
                            float frontier_phi_157_156_ladder_2;
                            if (((_434 & 64u) == 0u) && (_1095 != 0u))
                            {
                                float2 _2442 = spvUnpackHalf2x16((_1867 >> 17u) & 32736u);
                                float _2443 = _2442.x;
                                float _2446 = (spvUnpackHalf2x16((_1867 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2447 = (spvUnpackHalf2x16((_1867 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2451 = (1.0f - abs(_2446)) - abs(_2447);
                                float _2453 = clamp((-0.0f) - _2451, 0.0f, 1.0f);
                                float _2454 = (-0.0f) - _2453;
                                float _2459 = ((_2446 >= 0.0f) ? _2454 : _2453) + _2446;
                                float _2460 = ((_2447 >= 0.0f) ? _2454 : _2453) + _2447;
                                float _2464 = rsqrt(dot(float3(_2459, _2460, _2451), float3(_2459, _2460, _2451)));
                                float _2474 = (((_2459 * _2464) - _1896) * _2443) + _1896;
                                float _2475 = (((_2460 * _2464) - _1897) * _2443) + _1897;
                                float _2476 = (((_2464 * _2451) - _1898) * _2443) + _1898;
                                float _2480 = rsqrt(dot(float3(_2474, _2475, _2476), float3(_2474, _2475, _2476)));
                                frontier_phi_157_156_ladder = _2474 * _2480;
                                frontier_phi_157_156_ladder_1 = _2475 * _2480;
                                frontier_phi_157_156_ladder_2 = _2476 * _2480;
                            }
                            else
                            {
                                frontier_phi_157_156_ladder = _1896;
                                frontier_phi_157_156_ladder_1 = _1897;
                                frontier_phi_157_156_ladder_2 = _1898;
                            }
                            _1107 = frontier_phi_157_156_ladder;
                            _1109 = frontier_phi_157_156_ladder_1;
                            _1111 = frontier_phi_157_156_ladder_2;
                        }
                        else
                        {
                            _1107 = _1896;
                            _1109 = _1897;
                            _1111 = _1898;
                        }
                        float _1121;
                        float _1123;
                        float _1125;
                        float _1127;
                        if (_1103)
                        {
                            float frontier_phi_163_162_ladder;
                            float frontier_phi_163_162_ladder_1;
                            float frontier_phi_163_162_ladder_2;
                            float frontier_phi_163_162_ladder_3;
                            if (((_435 & 33554432u) == 0u) || (((_385 & 4u) != 0u) && ((_435 & 8388608u) == 0u)))
                            {
                                frontier_phi_163_162_ladder = 0.0f;
                                frontier_phi_163_162_ladder_1 = 0.0f;
                                frontier_phi_163_162_ladder_2 = 0.0f;
                                frontier_phi_163_162_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2512 = (spvUnpackHalf2x16((_1867 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2513 = (spvUnpackHalf2x16((_1867 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2517 = (1.0f - abs(_2512)) - abs(_2513);
                                float _2519 = clamp((-0.0f) - _2517, 0.0f, 1.0f);
                                float _2520 = (-0.0f) - _2519;
                                float _2525 = ((_2512 >= 0.0f) ? _2520 : _2519) + _2512;
                                float _2526 = ((_2513 >= 0.0f) ? _2520 : _2519) + _2513;
                                float _2530 = rsqrt(dot(float3(_2525, _2526, _2517), float3(_2525, _2526, _2517)));
                                float _2531 = _2525 * _2530;
                                float _2532 = _2526 * _2530;
                                float _2533 = _2530 * _2517;
                                float _2537 = rsqrt(dot(float3(_2531, _2532, _2533), float3(_2531, _2532, _2533)));
                                frontier_phi_163_162_ladder = _2537 * _2533;
                                frontier_phi_163_162_ladder_1 = _2537 * _2532;
                                frontier_phi_163_162_ladder_2 = _2537 * _2531;
                                frontier_phi_163_162_ladder_3 = spvUnpackHalf2x16((_1867 >> 17u) & 32736u).x;
                            }
                            _1121 = frontier_phi_163_162_ladder_3;
                            _1123 = frontier_phi_163_162_ladder_2;
                            _1125 = frontier_phi_163_162_ladder_1;
                            _1127 = frontier_phi_163_162_ladder;
                        }
                        else
                        {
                            _1121 = 0.0f;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                        }
                        bool _2494 = _2278 != 0u;
                        _1104 = _1105;
                        _1106 = _1107;
                        _1108 = _1109;
                        _1110 = _1111;
                        _1112 = _1113;
                        _1114 = _2494 ? _2272 : _1107;
                        _1116 = _2494 ? _2274 : _1109;
                        _1118 = _2494 ? _2276 : _1111;
                        _1120 = _1121;
                        _1122 = _1123;
                        _1124 = _1125;
                        _1126 = _1127;
                    }
                    else
                    {
                        uint4 _765 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _767 = _765.x;
                        uint4 _771 = _24[9u].Load(int3(uint2(_270, _271), 0u));
                        uint _773 = _771.x;
                        float _990;
                        float _991;
                        float _992;
                        if ((_435 & 33554432u) == 0u)
                        {
                            float _818 = (float((_767 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _819 = (float(_767 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _823 = (1.0f - abs(_818)) - abs(_819);
                            float _825 = clamp((-0.0f) - _823, 0.0f, 1.0f);
                            float _826 = (-0.0f) - _825;
                            float _831 = ((_818 >= 0.0f) ? _826 : _825) + _818;
                            float _832 = ((_819 >= 0.0f) ? _826 : _825) + _819;
                            float _836 = rsqrt(dot(float3(_831, _832, _823), float3(_831, _832, _823)));
                            _990 = _831 * _836;
                            _991 = _832 * _836;
                            _992 = _836 * _823;
                        }
                        else
                        {
                            float _847 = (float((_773 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _848 = (float(_773 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _852 = (1.0f - abs(_847)) - abs(_848);
                            float _854 = clamp((-0.0f) - _852, 0.0f, 1.0f);
                            float _855 = (-0.0f) - _854;
                            float _860 = ((_847 >= 0.0f) ? _855 : _854) + _847;
                            float _861 = ((_848 >= 0.0f) ? _855 : _854) + _848;
                            float _865 = rsqrt(dot(float3(_860, _861, _852), float3(_860, _861, _852)));
                            _990 = _860 * _865;
                            _991 = _861 * _865;
                            _992 = _865 * _852;
                        }
                        _1104 = float(_767 & 255u);
                        _1106 = _990;
                        _1108 = _991;
                        _1110 = _992;
                        _1112 = 1.0f;
                        _1114 = _990;
                        _1116 = _991;
                        _1118 = _992;
                        _1120 = 0.0f;
                        _1122 = 0.0f;
                        _1124 = 0.0f;
                        _1126 = 0.0f;
                    }
                    precise float _1128 = _1104 * 0.0039215688593685626983642578125f;
                    if ((_434 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1204 = ((_434 & 128u) | _444) != 0u;
                    uint _1278;
                    if (_1204)
                    {
                        _1278 = 1u;
                    }
                    else
                    {
                        _1278 = (((_435 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _397;
                    float _399;
                    float _401;
                    uint _403;
                    float _407;
                    if (((_435 & 33554432u) == 0u) || _1204)
                    {
                        bool _1372 = _77 != 0u;
                        uint _1379;
                        if ((_435 & 16u) == 0u)
                        {
                            _1379 = _1278;
                        }
                        else
                        {
                            _1379 = ((_435 & 268435456u) != 0u) ? _1278 : 2u;
                        }
                        _407 = _1112 * _1128;
                        _397 = _1372 ? _1114 : _1106;
                        _399 = _1372 ? _1116 : _1108;
                        _401 = _1372 ? _1118 : _1110;
                        _403 = _1379;
                    }
                    else
                    {
                        _407 = _1120;
                        _397 = _1122;
                        _399 = _1124;
                        _401 = _1126;
                        _403 = _1278;
                    }
                    uint _1380 = _403 + 102u;
                    float _1389 = clamp((_407 - _45_m0[_1380].x) / (_45_m0[_1380].y - _45_m0[_1380].x), 0.0f, 1.0f);
                    _396 = _397;
                    _398 = _399;
                    _400 = _401;
                    _402 = _403;
                    _404 = (_1389 * _1389) * (3.0f - (_1389 * 2.0f));
                    _406 = _407;
                    _408 = _45_m0[_1380].z;
                    _410 = (_403 == 1u) ? _394 : 0u;
                    _412 = asfloat(_17.Load((_389 * 115u) + 114u).x);
                }
                if (_404 == 0.0f)
                {
                    ladder_phi_8 = false;
                    break;
                }
                float _432 = float(_239);
                float _433 = float(_238);
                float _454;
                if (_268)
                {
                    _454 = _267;
                }
                else
                {
                    _454 = _12.Load(int3(uint2(uint(int(_259 * _433)), uint(int(_260 * _432))), 0u)).x;
                }
                float _463 = _50_m0[50u].w + _50_m0[50u].y;
                uint _466 = _402 + 63u;
                float _475 = clamp(((_50_m0[50u].x / (_463 - (_50_m0[50u].y * _454))) - _50_m0[_466].y) / (_50_m0[_466].x - _50_m0[_466].y), 0.0f, 1.0f);
                float _480 = ((_475 * _475) * _404) * (3.0f - (_475 * 2.0f));
                float _491 = ((_259 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _492 = ((1.0f - (_50_m0[51u].w * _260)) * 2.0f) + (-1.0f);
                float _508 = mad(_144, _454, mad(_137, _492, _491 * _130)) + _151;
                float _509 = (mad(_141, _454, mad(_134, _492, _491 * _127)) + _148) / _508;
                float _510 = (mad(_142, _454, mad(_135, _492, _491 * _128)) + _149) / _508;
                float _511 = (mad(_143, _454, mad(_136, _492, _491 * _129)) + _150) / _508;
                float _515 = rsqrt(dot(float3(_509, _510, _511), float3(_509, _510, _511)));
                float _516 = _515 * _509;
                float _517 = _515 * _510;
                float _518 = _515 * _511;
                float _521 = mad(_93, _400, mad(_87, _398, _396 * _81));
                float _524 = mad(_94, _400, mad(_88, _398, _396 * _82));
                float _527 = mad(_95, _400, mad(_89, _398, _396 * _83));
                float _531 = dot(float3(_516, _517, _518), float3(_521, _524, _527)) * 2.0f;
                float _535 = _516 - (_531 * _521);
                float _536 = _517 - (_531 * _524);
                float _537 = _518 - (_531 * _527);
                bool _541 = dot(float3(_535, _536, _537), float3(_516, _517, _518)) < 0.0f;
                float _548 = sqrt(((_510 * _510) + (_509 * _509)) + (_511 * _511)) * 0.001000000047497451305389404296875f;
                float _559 = ((_548 * _521) + _509) + (_535 * _408);
                float _560 = ((_548 * _524) + _510) + (_536 * _408);
                float _561 = ((_548 * _527) + _511) + (_537 * _408);
                float _577 = mad(_116, _561, mad(_109, _560, _559 * _102)) + _123;
                float _580 = (mad(_115, _561, mad(_108, _560, _559 * _101)) + _122) / _577;
                float _583 = (((mad(_113, _561, mad(_106, _560, _559 * _99)) + _120) / _577) * 0.5f) + 0.5f;
                float _584 = 0.5f - (((mad(_114, _561, mad(_107, _560, _559 * _100)) + _121) / _577) * 0.5f);
                float _587 = _583 * _50_m0[51u].x;
                float _588 = _584 * _50_m0[51u].y;
                float _593 = _559 + (_535 * 0.100000001490116119384765625f);
                float _594 = _560 + (_536 * 0.100000001490116119384765625f);
                float _595 = _561 + (_537 * 0.100000001490116119384765625f);
                float _611 = mad(_116, _595, mad(_109, _594, _593 * _102)) + _123;
                float _620 = _50_m0[51u].x * (((((mad(_113, _595, mad(_106, _594, _593 * _99)) + _120) / _611) * 0.5f) + 0.5f) - _583);
                float _622 = _50_m0[51u].y * ((0.5f - (((mad(_114, _595, mad(_107, _594, _593 * _100)) + _121) / _611) * 0.5f)) - _584);
                float _623 = ((mad(_115, _595, mad(_108, _594, _593 * _101)) + _122) / _611) - _580;
                float _624 = _620 * 10.0f;
                float _626 = _622 * 10.0f;
                float _627 = _623 * 10.0f;
                float _634 = _50_m0[2u].w / dot(float3(_535, _536, _537), float3(_521, _524, _527));
                float _638 = (_634 * _535) + _559;
                float _639 = (_634 * _536) + _560;
                float _640 = (_634 * _537) + _561;
                float _656 = mad(_116, _640, mad(_109, _639, _638 * _102)) + _123;
                float _665 = _50_m0[51u].x * (((((mad(_113, _640, mad(_106, _639, _638 * _99)) + _120) / _656) * 0.5f) + 0.5f) - _583);
                float _667 = _50_m0[51u].y * ((0.5f - (((mad(_114, _640, mad(_107, _639, _638 * _100)) + _121) / _656) * 0.5f)) - _584);
                float _668 = ((mad(_115, _640, mad(_108, _639, _638 * _101)) + _122) / _656) - _580;
                float _681 = sqrt(((_665 * _665) + (_668 * _668)) + (_667 * _667)) / sqrt(((_624 * _624) + (_627 * _627)) + (_626 * _626));
                float _688 = (_624 != 0.0f) ? (0.100000001490116119384765625f / _620) : 3.4028234663852885981170418348452e+38f;
                float _690 = (_626 != 0.0f) ? (0.100000001490116119384765625f / _622) : 3.4028234663852885981170418348452e+38f;
                float _691 = (_627 != 0.0f) ? (0.100000001490116119384765625f / _623) : 3.4028234663852885981170418348452e+38f;
                float _692 = 1.0f / _433;
                float _693 = 1.0f / _432;
                float _694 = 0.004999999888241291046142578125f / _433;
                float _696 = 0.004999999888241291046142578125f / _432;
                float _705 = float(_624 >= 0.0f);
                float _706 = float(_626 >= 0.0f);
                float _715 = ((_624 < 0.0f) ? ((-0.0f) - _694) : _694) - _587;
                float _718 = ((_626 < 0.0f) ? ((-0.0f) - _696) : _696) - _588;
                float _721 = min((((floor(_587 * _433) + _705) * _692) + _715) * _688, (((floor(_588 * _432) + _706) * _693) + _718) * _690);
                float _725 = (_721 * _624) + _587;
                float _726 = (_721 * _626) + _588;
                float _727 = (_721 * _627) + _580;
                float _730 = _50_m0[50u].x / (_463 - (_727 * _50_m0[50u].y));
                float _740 = 1.0f / max(0.00999999977648258209228515625f, max(abs(_620 * 38400.0f), abs(_622 * 21600.0f)));
                float _748 = max(_50_m0[3u].z, _50_m0[5u].z * _740);
                float _784;
                if (asuint(_50_m0[5u]).y == 0u)
                {
                    _784 = _748;
                }
                else
                {
                    _784 = min(_748, _50_m0[5u].w * _740);
                }
                uint _877;
                float _879;
                float _881;
                float _883;
                uint _885;
                float _887;
                float _889;
                float _891;
                float _893;
                uint _895;
                float _897;
                float _899;
                uint _901;
                if (_209 == 0u)
                {
                    _877 = 0u;
                    _879 = _727;
                    _881 = _726;
                    _883 = _725;
                    _885 = 0u;
                    _887 = _727;
                    _889 = _726;
                    _891 = _725;
                    _893 = 1.0f;
                    _895 = 0u;
                    _897 = 0.0f;
                    _899 = 0.0f;
                    _901 = 1u;
                }
                else
                {
                    uint _878;
                    float _880;
                    float _882;
                    float _884;
                    uint _886;
                    uint _1077;
                    float _888;
                    float _890;
                    float _892;
                    float _894;
                    uint _896;
                    float _898;
                    float _900;
                    uint _902;
                    float _1060;
                    uint _1062;
                    float _1067;
                    float _1069;
                    float _1071;
                    float _1073;
                    float _1075;
                    uint _1058 = 0u;
                    float _1059 = _784;
                    uint _1061 = 0u;
                    float _1063 = _727;
                    float _1064 = _726;
                    float _1065 = _725;
                    float _1066 = _721;
                    float _1068 = _693;
                    float _1070 = _692;
                    float _1072 = _432;
                    float _1074 = _433;
                    uint _1076 = 0u;
                    uint _1078 = 0u;
                    float _1079 = _727;
                    float _1080 = _726;
                    float _1081 = _725;
                    float _1082 = 1.0f;
                    uint _1083 = 0u;
                    float _1084 = 0.0f;
                    float _1085 = 0.0f;
                    uint _1086 = 1u;
                    float _1087;
                    float _1088;
                    uint _1089;
                    uint _1090;
                    bool _1091;
                    for (;;)
                    {
                        _1087 = _1074 * _1065;
                        _1088 = _1072 * _1064;
                        _1089 = uint(int(_1087));
                        _1090 = uint(int(_1088));
                        _1091 = _1076 == 0u;
                        float _1228;
                        if (_1091)
                        {
                            _1228 = _12.Load(int3(uint2(_1089, _1090), 0u)).x;
                        }
                        else
                        {
                            _1228 = _15.Load(int3(uint2(_1089, _1090), _1076 + 4294967295u)).x;
                        }
                        float _1234 = ((_1087 >= floor(_1074)) || (_1088 >= floor(_1072))) ? 1.0f : _1228;
                        float _1248 = (_627 < 0.0f) ? ((_1234 - _580) * _691) : 3.4028234663852885981170418348452e+38f;
                        float _1250 = min(min((((floor(_1087) + _705) * _1070) + _715) * _688, (((floor(_1088) + _706) * _1068) + _718) * _690), _1248);
                        bool _1251 = _1234 < _1063;
                        bool _1255 = _1251 && (asuint(_1250) != asuint(_1248));
                        float _1256 = _1251 ? _1250 : _1066;
                        float _1260 = (_1256 * _624) + _587;
                        float _1261 = (_1256 * _626) + _588;
                        float _1262 = (_1256 * _627) + _580;
                        uint _1264 = (_1255 ? 1u : 4294967295u) + _1076;
                        float _1265 = _1255 ? 0.5f : 2.0f;
                        float _1266 = _1265 * _1074;
                        float _1267 = _1265 * _1072;
                        float _1268 = _1255 ? 2.0f : 0.5f;
                        float _1269 = _1268 * _1070;
                        float _1270 = _1268 * _1068;
                        _878 = _1058 + 1u;
                        uint _1478;
                        uint _1481;
                        if (int(_1264) < int(0u))
                        {
                            float frontier_phi_76_61_ladder;
                            uint frontier_phi_76_61_ladder_1;
                            uint frontier_phi_76_61_ladder_2;
                            float frontier_phi_76_61_ladder_3;
                            float frontier_phi_76_61_ladder_4;
                            uint frontier_phi_76_61_ladder_5;
                            float frontier_phi_76_61_ladder_6;
                            float frontier_phi_76_61_ladder_7;
                            float frontier_phi_76_61_ladder_8;
                            float frontier_phi_76_61_ladder_9;
                            uint frontier_phi_76_61_ladder_10;
                            uint frontier_phi_76_61_ladder_11;
                            float frontier_phi_76_61_ladder_12;
                            float frontier_phi_76_61_ladder_13;
                            float frontier_phi_76_61_ladder_14;
                            float frontier_phi_76_61_ladder_15;
                            float frontier_phi_76_61_ladder_16;
                            float _1342;
                            float4 _1347;
                            float _1350;
                            float _1353;
                            bool _1354;
                            for (;;)
                            {
                                float _1340 = _50_m0[50u].w + _50_m0[50u].y;
                                _1342 = _50_m0[50u].x / (_1340 - (_50_m0[50u].y * _1234));
                                float _1345 = _50_m0[50u].x / (_1340 - (_50_m0[50u].y * _1262));
                                _1347 = _50_m0[3u];
                                _1350 = abs(_730 - _1345);
                                _1353 = _1345 - _1342;
                                _1354 = _1353 > max(_1347.x, _1347.x * _1350);
                                if (_1354)
                                {
                                    uint _1483;
                                    if (_1061 == 0u)
                                    {
                                        uint frontier_phi_94_93_ladder;
                                        if ((_402 == 2u) || (_402 == 4u))
                                        {
                                            if ((_1256 < _681) && (abs(_1353) < _50_m0[2u].z))
                                            {
                                                frontier_phi_76_61_ladder = _1059;
                                                frontier_phi_76_61_ladder_1 = 1u;
                                                frontier_phi_76_61_ladder_2 = _1086;
                                                frontier_phi_76_61_ladder_3 = _1260;
                                                frontier_phi_76_61_ladder_4 = _1261;
                                                frontier_phi_76_61_ladder_5 = 1u;
                                                frontier_phi_76_61_ladder_6 = 0.0f;
                                                frontier_phi_76_61_ladder_7 = _1081;
                                                frontier_phi_76_61_ladder_8 = _1080;
                                                frontier_phi_76_61_ladder_9 = _1079;
                                                frontier_phi_76_61_ladder_10 = 1u;
                                                frontier_phi_76_61_ladder_11 = _1264;
                                                frontier_phi_76_61_ladder_12 = _1266;
                                                frontier_phi_76_61_ladder_13 = _1267;
                                                frontier_phi_76_61_ladder_14 = _1269;
                                                frontier_phi_76_61_ladder_15 = _1270;
                                                frontier_phi_76_61_ladder_16 = _1256;
                                                break;
                                            }
                                            frontier_phi_94_93_ladder = 1u;
                                        }
                                        else
                                        {
                                            frontier_phi_94_93_ladder = 1u;
                                        }
                                        _1483 = frontier_phi_94_93_ladder;
                                    }
                                    else
                                    {
                                        _1483 = _1061;
                                    }
                                    if (!(_1083 == 0u))
                                    {
                                        frontier_phi_76_61_ladder = _1059;
                                        frontier_phi_76_61_ladder_1 = _1483;
                                        frontier_phi_76_61_ladder_2 = _1086;
                                        frontier_phi_76_61_ladder_3 = _1085;
                                        frontier_phi_76_61_ladder_4 = _1084;
                                        frontier_phi_76_61_ladder_5 = _1083;
                                        frontier_phi_76_61_ladder_6 = _1082;
                                        frontier_phi_76_61_ladder_7 = _1081;
                                        frontier_phi_76_61_ladder_8 = _1080;
                                        frontier_phi_76_61_ladder_9 = _1079;
                                        frontier_phi_76_61_ladder_10 = _1078;
                                        frontier_phi_76_61_ladder_11 = _1264;
                                        frontier_phi_76_61_ladder_12 = _1266;
                                        frontier_phi_76_61_ladder_13 = _1267;
                                        frontier_phi_76_61_ladder_14 = _1269;
                                        frontier_phi_76_61_ladder_15 = _1270;
                                        frontier_phi_76_61_ladder_16 = _1256;
                                        break;
                                    }
                                    float _1482 = _1256 + _1059;
                                    bool _1843 = _1078 != 0u;
                                    float _1476 = _1843 ? _1085 : _1260;
                                    float _1477 = _1843 ? _1084 : _1261;
                                    uint _1479 = ((_402 == 1u) || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1078;
                                    if (asuint(_50_m0[5u]).y == 0u)
                                    {
                                        frontier_phi_76_61_ladder = _1059;
                                        frontier_phi_76_61_ladder_1 = _1483;
                                        frontier_phi_76_61_ladder_2 = _1086;
                                        frontier_phi_76_61_ladder_3 = _1476;
                                        frontier_phi_76_61_ladder_4 = _1477;
                                        frontier_phi_76_61_ladder_5 = 0u;
                                        frontier_phi_76_61_ladder_6 = _1082;
                                        frontier_phi_76_61_ladder_7 = _1081;
                                        frontier_phi_76_61_ladder_8 = _1080;
                                        frontier_phi_76_61_ladder_9 = _1079;
                                        frontier_phi_76_61_ladder_10 = _1479;
                                        frontier_phi_76_61_ladder_11 = 0u;
                                        frontier_phi_76_61_ladder_12 = _433;
                                        frontier_phi_76_61_ladder_13 = _432;
                                        frontier_phi_76_61_ladder_14 = _692;
                                        frontier_phi_76_61_ladder_15 = _693;
                                        frontier_phi_76_61_ladder_16 = _1482;
                                        break;
                                    }
                                    frontier_phi_76_61_ladder = min(_748, _50_m0[6u].x * _1059);
                                    frontier_phi_76_61_ladder_1 = _1483;
                                    frontier_phi_76_61_ladder_2 = _1086;
                                    frontier_phi_76_61_ladder_3 = _1476;
                                    frontier_phi_76_61_ladder_4 = _1477;
                                    frontier_phi_76_61_ladder_5 = 0u;
                                    frontier_phi_76_61_ladder_6 = _1082;
                                    frontier_phi_76_61_ladder_7 = _1081;
                                    frontier_phi_76_61_ladder_8 = _1080;
                                    frontier_phi_76_61_ladder_9 = _1079;
                                    frontier_phi_76_61_ladder_10 = _1479;
                                    frontier_phi_76_61_ladder_11 = 0u;
                                    frontier_phi_76_61_ladder_12 = _433;
                                    frontier_phi_76_61_ladder_13 = _432;
                                    frontier_phi_76_61_ladder_14 = _692;
                                    frontier_phi_76_61_ladder_15 = _693;
                                    frontier_phi_76_61_ladder_16 = _1482;
                                    break;
                                }
                                else
                                {
                                    float _1465 = max(_1347.y, _1347.y * _1350);
                                    float _1468 = _1465 * _1347.w;
                                    float _1472 = clamp((abs(_1353) - _1468) / (_1465 - _1468), 0.0f, 1.0f);
                                    uint _1474 = uint(_1342 < _730);
                                    float frontier_phi_76_61_ladder_75_ladder;
                                    uint frontier_phi_76_61_ladder_75_ladder_1;
                                    uint frontier_phi_76_61_ladder_75_ladder_2;
                                    float frontier_phi_76_61_ladder_75_ladder_3;
                                    float frontier_phi_76_61_ladder_75_ladder_4;
                                    uint frontier_phi_76_61_ladder_75_ladder_5;
                                    float frontier_phi_76_61_ladder_75_ladder_6;
                                    float frontier_phi_76_61_ladder_75_ladder_7;
                                    float frontier_phi_76_61_ladder_75_ladder_8;
                                    float frontier_phi_76_61_ladder_75_ladder_9;
                                    uint frontier_phi_76_61_ladder_75_ladder_10;
                                    uint frontier_phi_76_61_ladder_75_ladder_11;
                                    float frontier_phi_76_61_ladder_75_ladder_12;
                                    float frontier_phi_76_61_ladder_75_ladder_13;
                                    float frontier_phi_76_61_ladder_75_ladder_14;
                                    float frontier_phi_76_61_ladder_75_ladder_15;
                                    float frontier_phi_76_61_ladder_75_ladder_16;
                                    if (_1078 == 0u)
                                    {
                                        frontier_phi_76_61_ladder_75_ladder = _1059;
                                        frontier_phi_76_61_ladder_75_ladder_1 = _1061;
                                        frontier_phi_76_61_ladder_75_ladder_2 = _1474;
                                        frontier_phi_76_61_ladder_75_ladder_3 = _1085;
                                        frontier_phi_76_61_ladder_75_ladder_4 = _1084;
                                        frontier_phi_76_61_ladder_75_ladder_5 = _1083;
                                        frontier_phi_76_61_ladder_75_ladder_6 = _1472;
                                        frontier_phi_76_61_ladder_75_ladder_7 = _1081;
                                        frontier_phi_76_61_ladder_75_ladder_8 = _1080;
                                        frontier_phi_76_61_ladder_75_ladder_9 = _1079;
                                        frontier_phi_76_61_ladder_75_ladder_10 = uint(_1472 > 0.0f);
                                        frontier_phi_76_61_ladder_75_ladder_11 = _1264;
                                        frontier_phi_76_61_ladder_75_ladder_12 = _1266;
                                        frontier_phi_76_61_ladder_75_ladder_13 = _1267;
                                        frontier_phi_76_61_ladder_75_ladder_14 = _1269;
                                        frontier_phi_76_61_ladder_75_ladder_15 = _1270;
                                        frontier_phi_76_61_ladder_75_ladder_16 = _1256;
                                    }
                                    else
                                    {
                                        frontier_phi_76_61_ladder_75_ladder = _1059;
                                        frontier_phi_76_61_ladder_75_ladder_1 = _1061;
                                        frontier_phi_76_61_ladder_75_ladder_2 = _1474;
                                        frontier_phi_76_61_ladder_75_ladder_3 = _1085;
                                        frontier_phi_76_61_ladder_75_ladder_4 = _1084;
                                        frontier_phi_76_61_ladder_75_ladder_5 = _1083;
                                        frontier_phi_76_61_ladder_75_ladder_6 = _1472;
                                        frontier_phi_76_61_ladder_75_ladder_7 = _1081;
                                        frontier_phi_76_61_ladder_75_ladder_8 = _1080;
                                        frontier_phi_76_61_ladder_75_ladder_9 = _1079;
                                        frontier_phi_76_61_ladder_75_ladder_10 = _1078;
                                        frontier_phi_76_61_ladder_75_ladder_11 = _1264;
                                        frontier_phi_76_61_ladder_75_ladder_12 = _1266;
                                        frontier_phi_76_61_ladder_75_ladder_13 = _1267;
                                        frontier_phi_76_61_ladder_75_ladder_14 = _1269;
                                        frontier_phi_76_61_ladder_75_ladder_15 = _1270;
                                        frontier_phi_76_61_ladder_75_ladder_16 = _1256;
                                    }
                                    frontier_phi_76_61_ladder = frontier_phi_76_61_ladder_75_ladder;
                                    frontier_phi_76_61_ladder_1 = frontier_phi_76_61_ladder_75_ladder_1;
                                    frontier_phi_76_61_ladder_2 = frontier_phi_76_61_ladder_75_ladder_2;
                                    frontier_phi_76_61_ladder_3 = frontier_phi_76_61_ladder_75_ladder_3;
                                    frontier_phi_76_61_ladder_4 = frontier_phi_76_61_ladder_75_ladder_4;
                                    frontier_phi_76_61_ladder_5 = frontier_phi_76_61_ladder_75_ladder_5;
                                    frontier_phi_76_61_ladder_6 = frontier_phi_76_61_ladder_75_ladder_6;
                                    frontier_phi_76_61_ladder_7 = frontier_phi_76_61_ladder_75_ladder_7;
                                    frontier_phi_76_61_ladder_8 = frontier_phi_76_61_ladder_75_ladder_8;
                                    frontier_phi_76_61_ladder_9 = frontier_phi_76_61_ladder_75_ladder_9;
                                    frontier_phi_76_61_ladder_10 = frontier_phi_76_61_ladder_75_ladder_10;
                                    frontier_phi_76_61_ladder_11 = frontier_phi_76_61_ladder_75_ladder_11;
                                    frontier_phi_76_61_ladder_12 = frontier_phi_76_61_ladder_75_ladder_12;
                                    frontier_phi_76_61_ladder_13 = frontier_phi_76_61_ladder_75_ladder_13;
                                    frontier_phi_76_61_ladder_14 = frontier_phi_76_61_ladder_75_ladder_14;
                                    frontier_phi_76_61_ladder_15 = frontier_phi_76_61_ladder_75_ladder_15;
                                    frontier_phi_76_61_ladder_16 = frontier_phi_76_61_ladder_75_ladder_16;
                                    break;
                                }
                            }
                            _902 = frontier_phi_76_61_ladder_2;
                            _900 = frontier_phi_76_61_ladder_3;
                            _898 = frontier_phi_76_61_ladder_4;
                            _896 = frontier_phi_76_61_ladder_5;
                            _894 = frontier_phi_76_61_ladder_6;
                            _892 = frontier_phi_76_61_ladder_7;
                            _890 = frontier_phi_76_61_ladder_8;
                            _888 = frontier_phi_76_61_ladder_9;
                            _1478 = frontier_phi_76_61_ladder_10;
                            _1481 = frontier_phi_76_61_ladder_11;
                            _1075 = frontier_phi_76_61_ladder_12;
                            _1073 = frontier_phi_76_61_ladder_13;
                            _1071 = frontier_phi_76_61_ladder_14;
                            _1069 = frontier_phi_76_61_ladder_15;
                            _1067 = frontier_phi_76_61_ladder_16;
                            _1062 = frontier_phi_76_61_ladder_1;
                            _1060 = frontier_phi_76_61_ladder;
                        }
                        else
                        {
                            bool _1355 = _1078 != 0u;
                            _902 = _1086;
                            _900 = _1085;
                            _898 = _1084;
                            _896 = _1083;
                            _894 = _1082;
                            _892 = _1355 ? _1081 : _1260;
                            _890 = _1355 ? _1080 : _1261;
                            _888 = _1355 ? _1079 : _1262;
                            _1478 = _1078;
                            _1481 = _1264;
                            _1075 = _1266;
                            _1073 = _1267;
                            _1071 = _1269;
                            _1069 = _1270;
                            _1067 = _1256;
                            _1062 = _1061;
                            _1060 = (asuint(_50_m0[5u]).y != 0u) ? _784 : _1059;
                        }
                        uint frontier_phi_111_pred;
                        uint frontier_phi_111_pred_1;
                        float frontier_phi_111_pred_2;
                        float frontier_phi_111_pred_3;
                        float frontier_phi_111_pred_4;
                        bool _1487;
                        bool _1489;
                        for (;;)
                        {
                            _1487 = _1262 < 0.0f;
                            _1489 = _1487 || ((_1260 < 0.0f) || (_1261 < 0.0f));
                            if (!_1489)
                            {
                                if (!((_1262 > 1.0f) || ((_1260 > _50_m0[51u].x) || (_1261 > _50_m0[51u].y))))
                                {
                                    frontier_phi_111_pred = _1478;
                                    frontier_phi_111_pred_1 = _1481;
                                    frontier_phi_111_pred_2 = _1260;
                                    frontier_phi_111_pred_3 = _1262;
                                    frontier_phi_111_pred_4 = _1261;
                                    break;
                                }
                            }
                            if (!_1487)
                            {
                                frontier_phi_111_pred = 1u;
                                frontier_phi_111_pred_1 = 4294967295u;
                                frontier_phi_111_pred_2 = _1260;
                                frontier_phi_111_pred_3 = _1262;
                                frontier_phi_111_pred_4 = _1261;
                                break;
                            }
                            float _1857 = (-0.0f) - _1262;
                            float _1858 = _1857 / _627;
                            frontier_phi_111_pred = 1u;
                            frontier_phi_111_pred_1 = 4294967295u;
                            frontier_phi_111_pred_2 = (_1858 * _624) + _1260;
                            frontier_phi_111_pred_3 = _1857 + _1262;
                            frontier_phi_111_pred_4 = (_1858 * _626) + _1261;
                            break;
                        }
                        _886 = frontier_phi_111_pred;
                        _1077 = frontier_phi_111_pred_1;
                        _884 = frontier_phi_111_pred_2;
                        _880 = frontier_phi_111_pred_3;
                        _882 = frontier_phi_111_pred_4;
                        if ((_878 < _209) && (int(_1077) > int(4294967295u)))
                        {
                            _1058 = _878;
                            _1059 = _1060;
                            _1061 = _1062;
                            _1063 = _880;
                            _1064 = _882;
                            _1065 = _884;
                            _1066 = _1067;
                            _1068 = _1069;
                            _1070 = _1071;
                            _1072 = _1073;
                            _1074 = _1075;
                            _1076 = _1077;
                            _1078 = _886;
                            _1079 = _888;
                            _1080 = _890;
                            _1081 = _892;
                            _1082 = _894;
                            _1083 = _896;
                            _1084 = _898;
                            _1085 = _900;
                            _1086 = _902;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    _877 = _878;
                    _879 = _880;
                    _881 = _882;
                    _883 = _884;
                    _885 = _886;
                    _887 = _888;
                    _889 = _890;
                    _891 = _892;
                    _893 = _894;
                    _895 = _896;
                    _897 = _898;
                    _899 = _900;
                    _901 = _902;
                }
                bool _903 = _877 >= _209;
                uint _904 = _903 ? 1u : _885;
                float _912 = _50_m0[51u].z * 2.0f;
                float _915 = (_912 * _587) + (-1.0f);
                float _916 = ((1.0f - (_50_m0[51u].w * _588)) * 2.0f) + (-1.0f);
                float _932 = mad(_190, _580, mad(_183, _916, _915 * _176)) + _197;
                float _933 = (mad(_187, _580, mad(_180, _916, _915 * _173)) + _194) / _932;
                float _934 = (mad(_188, _580, mad(_181, _916, _915 * _174)) + _195) / _932;
                float _935 = (mad(_189, _580, mad(_182, _916, _915 * _175)) + _196) / _932;
                float _940 = (_912 * _883) + (-1.0f);
                float _941 = ((1.0f - (_50_m0[51u].w * _881)) * 2.0f) + (-1.0f);
                float _957 = mad(_190, _879, mad(_183, _941, _940 * _176)) + _197;
                float _961 = ((mad(_187, _879, mad(_180, _941, _940 * _173)) + _194) / _957) - _933;
                float _962 = ((mad(_188, _879, mad(_181, _941, _940 * _174)) + _195) / _957) - _934;
                float _963 = ((mad(_189, _879, mad(_182, _941, _940 * _175)) + _196) / _957) - _935;
                float _995;
                uint _997;
                float _999;
                if (_877 > _209)
                {
                    _995 = 0.0f;
                    _997 = _904;
                    _999 = 0.0f;
                }
                else
                {
                    float frontier_phi_30_31_ladder;
                    uint frontier_phi_30_31_ladder_1;
                    float frontier_phi_30_31_ladder_2;
                    if ((_881 < 0.0f) || (_883 < 0.0f))
                    {
                        frontier_phi_30_31_ladder = 0.0f;
                        frontier_phi_30_31_ladder_1 = _904;
                        frontier_phi_30_31_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_30_31_ladder_37_ladder;
                        uint frontier_phi_30_31_ladder_37_ladder_1;
                        float frontier_phi_30_31_ladder_37_ladder_2;
                        if ((_879 >= 1.0f) || ((_883 > _50_m0[51u].x) || (_881 > _50_m0[51u].y)))
                        {
                            frontier_phi_30_31_ladder_37_ladder = 0.0f;
                            frontier_phi_30_31_ladder_37_ladder_1 = _904;
                            frontier_phi_30_31_ladder_37_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_30_31_ladder_37_ladder_47_ladder;
                            uint frontier_phi_30_31_ladder_37_ladder_47_ladder_1;
                            float frontier_phi_30_31_ladder_37_ladder_47_ladder_2;
                            for (;;)
                            {
                                if ((abs(_883 - _259) < (2.0f / _433)) && (abs(_881 - _260) < (2.0f / _432)))
                                {
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder = 0.0f;
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder_1 = _904;
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_92;
                                    float frontier_phi_92_pred;
                                    uint frontier_phi_92_pred_1;
                                    float frontier_phi_92_pred_2;
                                    uint _1330;
                                    uint _1331;
                                    bool _1333;
                                    for (;;)
                                    {
                                        _1330 = uint(int(_883 * _433));
                                        _1331 = uint(int(_881 * _432));
                                        _1333 = (_402 == 1u) && _541;
                                        if (!_1333)
                                        {
                                            if (!(dot(float3(_961, _962, _963), float3(_961, _962, _963)) < _50_m0[4u].w))
                                            {
                                                ladder_phi_92 = false;
                                                frontier_phi_92_pred = 0.0f;
                                                frontier_phi_92_pred_1 = _904;
                                                frontier_phi_92_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1451 = _24[22u].Load(int3(uint2(_1330, _1331), 0u));
                                        uint _1453 = _1451.x;
                                        float _1822;
                                        float _1823;
                                        float _1824;
                                        if (_1453 == 0u)
                                        {
                                            uint4 _1609 = _24[1u].Load(int3(uint2(_1330, _1331), 0u));
                                            uint _1611 = _1609.x;
                                            float _1619 = (float((_1611 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1620 = (float(_1611 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1624 = (1.0f - abs(_1619)) - abs(_1620);
                                            float _1626 = clamp((-0.0f) - _1624, 0.0f, 1.0f);
                                            float _1627 = (-0.0f) - _1626;
                                            _1822 = ((_1619 >= 0.0f) ? _1627 : _1626) + _1619;
                                            _1823 = ((_1620 >= 0.0f) ? _1627 : _1626) + _1620;
                                            _1824 = _1624;
                                        }
                                        else
                                        {
                                            float _1641 = (float((_1453 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1642 = (float(_1453 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1646 = (1.0f - abs(_1641)) - abs(_1642);
                                            float _1648 = clamp((-0.0f) - _1646, 0.0f, 1.0f);
                                            float _1649 = (-0.0f) - _1648;
                                            _1822 = ((_1641 >= 0.0f) ? _1649 : _1648) + _1641;
                                            _1823 = ((_1642 >= 0.0f) ? _1649 : _1648) + _1642;
                                            _1824 = _1646;
                                        }
                                        float _1828 = rsqrt(dot(float3(_1822, _1823, _1824), float3(_1822, _1823, _1824)));
                                        if (dot(float3(_1828 * _1822, _1828 * _1823, _1828 * _1824), float3(_961, _962, _963)) > 0.0f)
                                        {
                                            ladder_phi_92 = true;
                                            frontier_phi_92_pred = 0.0f;
                                            frontier_phi_92_pred_1 = _904;
                                            frontier_phi_92_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_92 = false;
                                            frontier_phi_92_pred = 0.0f;
                                            frontier_phi_92_pred_1 = _904;
                                            frontier_phi_92_pred_2 = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_92)
                                    {
                                        frontier_phi_30_31_ladder_37_ladder_47_ladder = frontier_phi_92_pred;
                                        frontier_phi_30_31_ladder_37_ladder_47_ladder_1 = frontier_phi_92_pred_1;
                                        frontier_phi_30_31_ladder_37_ladder_47_ladder_2 = frontier_phi_92_pred_2;
                                        break;
                                    }
                                    float _1660 = _50_m0[51u].z * _883;
                                    float _1661 = _50_m0[51u].w * _881;
                                    float _1663 = (_432 / _433) * 0.0500000007450580596923828125f;
                                    float _1668 = clamp(_1660 / _1663, 0.0f, 1.0f);
                                    float _1669 = clamp(_1661 * 20.0f, 0.0f, 1.0f);
                                    float _1681 = clamp(((_1660 + (-1.0f)) + _1663) / _1663, 0.0f, 1.0f);
                                    float _1682 = clamp((_1661 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1693 = _1668 * _1669;
                                    precise float _1694 = _1693 * _1693;
                                    float _1698 = ((((3.0f - (_1669 * 2.0f)) * (3.0f - (_1668 * 2.0f))) * _1694) * (1.0f - ((_1681 * _1681) * (3.0f - (_1681 * 2.0f))))) * (1.0f - ((_1682 * _1682) * (3.0f - (_1682 * 2.0f))));
                                    bool _1701 = (_904 != 0u) || (_1698 >= 1.0f);
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder = _1698 * float(_480 > 0.0f);
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder_1 = _1701 ? _904 : 1u;
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder_2 = _1701 ? 0.0f : _1698;
                                    break;
                                }
                            }
                            frontier_phi_30_31_ladder_37_ladder = frontier_phi_30_31_ladder_37_ladder_47_ladder;
                            frontier_phi_30_31_ladder_37_ladder_1 = frontier_phi_30_31_ladder_37_ladder_47_ladder_1;
                            frontier_phi_30_31_ladder_37_ladder_2 = frontier_phi_30_31_ladder_37_ladder_47_ladder_2;
                        }
                        frontier_phi_30_31_ladder = frontier_phi_30_31_ladder_37_ladder;
                        frontier_phi_30_31_ladder_1 = frontier_phi_30_31_ladder_37_ladder_1;
                        frontier_phi_30_31_ladder_2 = frontier_phi_30_31_ladder_37_ladder_2;
                    }
                    _995 = frontier_phi_30_31_ladder_2;
                    _997 = frontier_phi_30_31_ladder_1;
                    _999 = frontier_phi_30_31_ladder;
                }
                uint _1137;
                float _1138;
                float _1140;
                float _1142;
                float _1144;
                float _1147;
                float _1148;
                float _1149;
                float _1151;
                float _1040;
                float _1043;
                float _1046;
                float _1049;
                float _1053;
                float _1054;
                for (;;)
                {
                    _1040 = ((((exp2(log2(clamp((sqrt(((_934 * _934) + (_933 * _933)) + (_935 * _935)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _412) * exp2(log2(clamp((_934 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_398, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    _1043 = mad(_167, _537, mad(_161, _536, _535 * _155));
                    _1046 = mad(_168, _537, mad(_162, _536, _535 * _156));
                    _1049 = mad(_169, _537, mad(_163, _536, _535 * _157));
                    bool _1052 = (_997 != 0u) || (_999 < 1.0f);
                    _1053 = _1052 ? 0.0f : 1.0f;
                    _1054 = _1052 ? 0.0f : 0.5f;
                    if (_1052)
                    {
                        bool _1132 = _402 == 1u;
                        uint4 _1136 = asuint(_50_m0[60u]);
                        if (_1132)
                        {
                            if (int(_410) < int(1u))
                            {
                                if (_1136.x == 0u)
                                {
                                    _1137 = 0u;
                                    _1138 = 0.0f;
                                    _1140 = 0.0f;
                                    _1142 = 0.0f;
                                    _1144 = 9899999600270360182784.0f;
                                    _1147 = 0.0f;
                                    _1148 = 0.0f;
                                    _1149 = 0.0f;
                                    _1151 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1136.y == 0u)
                                {
                                    _1137 = 0u;
                                    _1138 = 0.0f;
                                    _1140 = 0.0f;
                                    _1142 = 0.0f;
                                    _1144 = 9899999600270360182784.0f;
                                    _1147 = 0.0f;
                                    _1148 = 0.0f;
                                    _1149 = 0.0f;
                                    _1151 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1136.z == 0u)
                            {
                                _1137 = 0u;
                                _1138 = 0.0f;
                                _1140 = 0.0f;
                                _1142 = 0.0f;
                                _1144 = 9899999600270360182784.0f;
                                _1147 = 0.0f;
                                _1148 = 0.0f;
                                _1149 = 0.0f;
                                _1151 = 0.0f;
                                break;
                            }
                        }
                        if (_995 > 0.0f)
                        {
                            _1137 = 0u;
                            _1138 = 0.0f;
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 9899999600270360182784.0f;
                            _1147 = 0.0f;
                            _1148 = 1.0f;
                            _1149 = 1000.0f;
                            _1151 = 0.5f;
                            break;
                        }
                        if ((_887 <= 0.0f) || ((_889 <= 0.0f) || (_891 <= 0.0f)))
                        {
                            _1137 = 0u;
                            _1138 = 0.0f;
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 9899999600270360182784.0f;
                            _1147 = 0.0f;
                            _1148 = 1.0f;
                            _1149 = 1000.0f;
                            _1151 = 0.5f;
                            break;
                        }
                        if ((_887 >= 1.0f) || ((_891 >= _50_m0[51u].x) || (_889 >= _50_m0[51u].y)))
                        {
                            _1137 = 0u;
                            _1138 = 0.0f;
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 9899999600270360182784.0f;
                            _1147 = 0.0f;
                            _1148 = 1.0f;
                            _1149 = 1000.0f;
                            _1151 = 0.5f;
                            break;
                        }
                        uint _2040;
                        uint _2042;
                        uint _1729;
                        uint _1730;
                        bool _1736;
                        for (;;)
                        {
                            _1729 = uint(clamp(_899, 0.0f, 1.0f) * _202);
                            _1730 = uint(clamp(_897, 0.0f, 1.0f) * _204);
                            _1736 = _20[21u].Load(int3(uint2(_1729, _1730), 0u)).x > 0.0f;
                            if (_1736)
                            {
                                uint _1915 = _24[23u].Load(int3(uint2(_1729, _1730), 0u)).y + 4294967295u;
                                _2040 = (uint(int(_1915) >> int(31u)) & 3u) + 1u;
                                _2042 = (int(_1915) < int(0u)) ? 0u : _1915;
                                break;
                            }
                            else
                            {
                                uint4 _1923 = _24[2u].Load(int3(uint2(_1729, _1730), 0u));
                                uint _1926 = _1923.w;
                                uint4 _1931 = _24[15u].Load(int3(uint2(_1729, _1730), 0u));
                                uint _1933 = _1931.y;
                                uint _1939 = ((_1933 & 64u) != 0u) ? uint((_1933 & 4294967167u) != 66u) : 4294967295u;
                                uint _1940 = _1926 & 128u;
                                uint _1942 = (_1940 != 0u) ? 1u : ((_1923.x << 7u) | _1926);
                                uint4 _1945 = _16.Load(_1942 * 4u);
                                uint _1946 = _1945.x;
                                uint _1953 = ((_1946 & 1u) != 0u) ? 0u : 18u;
                                uint _1955 = uint(min(int(uint(max(int(_1939), int(0u)))), int(1u)));
                                uint _2053;
                                if (_1940 == 0u)
                                {
                                    _2053 = (((_1946 & 2097152u) != 0u) && (_1939 == _1955)) ? (_1953 | 128u) : _1953;
                                }
                                else
                                {
                                    _2053 = _1926;
                                }
                                uint _2054 = _16.Load((_1942 * 4u) + 1u).x & 512u;
                                bool _2057 = (_2053 & 144u) == 0u;
                                if (_2054 == 0u)
                                {
                                    if (_2057 || ((_1946 & 1u) != 0u))
                                    {
                                        _2040 = 0u;
                                        _2042 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2057)
                                    {
                                        _2040 = 0u;
                                        _2042 = 0u;
                                        break;
                                    }
                                }
                                bool _2227 = ((_2053 & 128u) | _2054) != 0u;
                                uint _2041;
                                if (_2227)
                                {
                                    _2041 = 1u;
                                }
                                else
                                {
                                    _2041 = (((_1946 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_1946 & 268435472u) == 16u) && (((_1946 & 33554432u) == 0u) || _2227))
                                {
                                    _2040 = 2u;
                                    _2042 = 0u;
                                    break;
                                }
                                _2040 = _2041;
                                _2042 = (_2041 == 1u) ? _1955 : 0u;
                                break;
                            }
                        }
                        if ((_402 != _2040) || (_410 != _2042))
                        {
                            _1137 = 0u;
                            _1138 = 0.0f;
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 9899999600270360182784.0f;
                            _1147 = 0.0f;
                            _1148 = 1.0f;
                            _1149 = 1000.0f;
                            _1151 = 0.5f;
                            break;
                        }
                        float _2137 = _891 * 2.0f;
                        float _2140 = (_50_m0[51u].z * _2137) + (-1.0f);
                        float _2141 = ((1.0f - (_50_m0[51u].w * _889)) * 2.0f) + (-1.0f);
                        float _2157 = mad(_190, _887, mad(_183, _2141, _2140 * _176)) + _197;
                        float _2158 = (mad(_187, _887, mad(_180, _2141, _2140 * _173)) + _194) / _2157;
                        float _2159 = (mad(_188, _887, mad(_181, _2141, _2140 * _174)) + _195) / _2157;
                        float _2160 = (mad(_189, _887, mad(_182, _2141, _2140 * _175)) + _196) / _2157;
                        if (sqrt(((_2159 * _2159) + (_2158 * _2158)) + (_2160 * _2160)) > _50_m0[58u].w)
                        {
                            _1137 = 0u;
                            _1138 = 0.0f;
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 9899999600270360182784.0f;
                            _1147 = 0.0f;
                            _1148 = 0.0f;
                            _1149 = 1000.0f;
                            _1151 = 0.5f;
                            break;
                        }
                        float _2207 = _2158 - _933;
                        float _2208 = _2159 - _934;
                        float _2209 = _2160 - _935;
                        float _2215 = sqrt(((_2208 * _2208) + (_2207 * _2207)) + (_2209 * _2209));
                        float _2223 = min(_50_m0[59u].y, max(0.0f, _2215 + (-0.001000000047497451305389404296875f)));
                        float _2336;
                        if (_1132)
                        {
                            _2336 = min(_50_m0[59u].x, _50_m0[4u].z + _2223);
                        }
                        else
                        {
                            _2336 = _50_m0[59u].x;
                        }
                        float _2337 = _2336 - _2215;
                        if (!(_2337 > 0.0f))
                        {
                            _1137 = 0u;
                            _1138 = 0.0f;
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 9899999600270360182784.0f;
                            _1147 = 1.0f;
                            _1148 = 1.0f;
                            _1149 = 0.0f;
                            _1151 = 0.5f;
                            break;
                        }
                        float _2371 = _2158 - (_2223 * _1043);
                        float _2372 = _2159 - (_2223 * _1046);
                        float _2373 = _2160 - (_2223 * _1049);
                        RayDesc _2ident = {float3(mad(_2373, _50_m0[46u].z, mad(_2372, _50_m0[46u].y, _50_m0[46u].x * _2371)) + _50_m0[46u].w, mad(_2373, _50_m0[47u].z, mad(_2372, _50_m0[47u].y, _50_m0[47u].x * _2371)) + _50_m0[47u].w, mad(_2373, _50_m0[48u].z, mad(_2372, _50_m0[48u].y, _50_m0[48u].x * _2371)) + _50_m0[48u].w), 0.0f, float3(mad(_1049, _50_m0[46u].z, mad(_1046, _50_m0[46u].y, _50_m0[46u].x * _1043)), mad(_1049, _50_m0[47u].z, mad(_1046, _50_m0[47u].y, _50_m0[47u].x * _1043)), mad(_1049, _50_m0[48u].z, mad(_1046, _50_m0[48u].y, _50_m0[48u].x * _1043))), _2337};
                        _2376.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2423 = _2376.Proceed();
                        uint _2424 = _2376.CommittedStatus();
                        if (!(_2424 == 1u))
                        {
                            _1137 = 0u;
                            _1138 = 0.0f;
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 9899999600270360182784.0f;
                            _1147 = 1.0f;
                            _1148 = 0.0f;
                            _1149 = 0.0f;
                            _1151 = 0.5f;
                            break;
                        }
                        float _2495 = _2376.CommittedRayT();
                        if (!((_2495 < _2337) && (_2495 > 0.0f)))
                        {
                            _1137 = 0u;
                            _1138 = 0.0f;
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 9899999600270360182784.0f;
                            _1147 = 1.0f;
                            _1148 = 0.0f;
                            _1149 = 0.0f;
                            _1151 = 0.5f;
                            break;
                        }
                        float _2546 = (_50_m0[51u].z * _2137) + (-1.0f);
                        float _2547 = ((1.0f - (_50_m0[51u].w * _889)) * 2.0f) + (-1.0f);
                        float _2563 = mad(_144, _887, mad(_137, _2547, _2546 * _130)) + _151;
                        float _2567 = _2495 - _2223;
                        float _2571 = ((mad(_141, _887, mad(_134, _2547, _2546 * _127)) + _148) / _2563) + (_2567 * _535);
                        float _2572 = ((mad(_142, _887, mad(_135, _2547, _2546 * _128)) + _149) / _2563) + (_2567 * _536);
                        float _2573 = ((mad(_143, _887, mad(_136, _2547, _2546 * _129)) + _150) / _2563) + (_2567 * _537);
                        float _2585 = mad(_116, _2573, mad(_109, _2572, _2571 * _102)) + _123;
                        float _2595 = (_50_m0[51u].z * _50_m0[51u].x) * ((((mad(_113, _2573, mad(_106, _2572, _2571 * _99)) + _120) / _2585) * 0.5f) + 0.5f);
                        float _2597 = (_50_m0[51u].w * _50_m0[51u].y) * (0.5f - (((mad(_114, _2573, mad(_107, _2572, _2571 * _100)) + _121) / _2585) * 0.5f));
                        float _2599 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2602 = clamp(_2595 / _2599, 0.0f, 1.0f);
                        float _2603 = clamp(_2597 * 20.0f, 0.0f, 1.0f);
                        float _2613 = clamp(((_2599 + (-1.0f)) + _2595) / _2599, 0.0f, 1.0f);
                        float _2614 = clamp((_2597 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2625 = _2602 * _2603;
                        precise float _2626 = _2625 * _2625;
                        if ((((((3.0f - (_2603 * 2.0f)) * (3.0f - (_2602 * 2.0f))) * _2626) * (1.0f - ((_2613 * _2613) * (3.0f - (_2613 * 2.0f))))) * (1.0f - ((_2614 * _2614) * (3.0f - (_2614 * 2.0f))))) < 1.0f)
                        {
                            _1137 = 0u;
                            _1138 = 0.0f;
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 9899999600270360182784.0f;
                            _1147 = _1053;
                            _1148 = _1053;
                            _1149 = 0.0f;
                            _1151 = _1054;
                            break;
                        }
                        _1137 = 1u;
                        _1138 = _50_m0[1u].w;
                        _1140 = _50_m0[2u].x;
                        _1142 = _50_m0[2u].y;
                        _1144 = (_2215 - _2223) + _2495;
                        _1147 = 0.0f;
                        _1148 = 1.0f;
                        _1149 = 0.0f;
                        _1151 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1137 = 0u;
                        _1138 = 0.0f;
                        _1140 = 0.0f;
                        _1142 = 0.0f;
                        _1144 = 9899999600270360182784.0f;
                        _1147 = 1.0f;
                        _1148 = 1.0f;
                        _1149 = 0.0f;
                        _1151 = 0.5f;
                        break;
                    }
                }
                float _1177 = clamp(log2((exp2(log2(_50_m0[0u].x * (1.0f - _406)) * _50_m0[0u].y) * _50_m0[0u].z) * _50_m0[61u].w) / float(asuint(_50_m0[55u]).x + 4294967295u), 0.0f, 1.0f);
                bool _1178 = _402 == 1u;
                float _1214;
                if (_1178)
                {
                    float _1213 = _50_m0[4u].z - _50_m0[4u].y;
                    float _1399;
                    if (_1137 == 0u)
                    {
                        float _1299 = clamp((sqrt(((_962 * _962) + (_961 * _961)) + (_963 * _963)) - _50_m0[4u].y) / _1213, 0.0f, 1.0f);
                        _1399 = (_1299 * _1299) * (3.0f - (_1299 * 2.0f));
                    }
                    else
                    {
                        float _1306 = clamp((_1144 - _50_m0[4u].y) / _1213, 0.0f, 1.0f);
                        _1399 = (_1306 * _1306) * (3.0f - (_1306 * 2.0f));
                    }
                    _1214 = (1.0f - _1399) * _50_m0[61u].z;
                }
                else
                {
                    _1214 = 1.0f;
                }
                float _1216 = _1214 * _999;
                bool _1217 = _402 != 1u;
                float _1737;
                float _1739;
                float _1741;
                float _1743;
                if (_1216 == 0.0f)
                {
                    float _1533;
                    float _1536;
                    float _1539;
                    float _1542;
                    if (_1137 == 0u)
                    {
                        float frontier_phi_83_68_ladder;
                        float frontier_phi_83_68_ladder_1;
                        float frontier_phi_83_68_ladder_2;
                        float frontier_phi_83_68_ladder_3;
                        if (_1217)
                        {
                            float _1514 = _1046 * _1040;
                            float _1518 = rsqrt(dot(float3(_1043, _1514, _1049), float3(_1043, _1514, _1049)));
                            float4 _1528 = _28[4u].SampleLevel(_59, float3(_1518 * _1043, _1518 * _1514, _1518 * _1049), 0.0f);
                            frontier_phi_83_68_ladder = 1.0f;
                            frontier_phi_83_68_ladder_1 = _1528.z;
                            frontier_phi_83_68_ladder_2 = _1528.y;
                            frontier_phi_83_68_ladder_3 = _1528.x;
                        }
                        else
                        {
                            frontier_phi_83_68_ladder = 0.0f;
                            frontier_phi_83_68_ladder_1 = 0.0f;
                            frontier_phi_83_68_ladder_2 = 0.0f;
                            frontier_phi_83_68_ladder_3 = 0.0f;
                        }
                        _1533 = frontier_phi_83_68_ladder_3;
                        _1536 = frontier_phi_83_68_ladder_2;
                        _1539 = frontier_phi_83_68_ladder_1;
                        _1542 = frontier_phi_83_68_ladder;
                    }
                    else
                    {
                        float frontier_phi_83_69_ladder;
                        float frontier_phi_83_69_ladder_1;
                        float frontier_phi_83_69_ladder_2;
                        float frontier_phi_83_69_ladder_3;
                        if (_1217)
                        {
                            float _1547 = _1046 * _1040;
                            float _1551 = rsqrt(dot(float3(_1043, _1547, _1049), float3(_1043, _1547, _1049)));
                            float4 _1559 = _28[4u].SampleLevel(_59, float3(_1551 * _1043, _1551 * _1547, _1551 * _1049), 0.0f);
                            float _1561 = _1559.x;
                            float _1562 = _1559.y;
                            float _1563 = _1559.z;
                            frontier_phi_83_69_ladder = 1.0f;
                            frontier_phi_83_69_ladder_1 = ((_1142 - _1563) * _1214) + _1563;
                            frontier_phi_83_69_ladder_2 = ((_1140 - _1562) * _1214) + _1562;
                            frontier_phi_83_69_ladder_3 = ((_1138 - _1561) * _1214) + _1561;
                        }
                        else
                        {
                            frontier_phi_83_69_ladder = _1214;
                            frontier_phi_83_69_ladder_1 = _1214 * _1142;
                            frontier_phi_83_69_ladder_2 = _1214 * _1140;
                            frontier_phi_83_69_ladder_3 = _1214 * _1138;
                        }
                        _1533 = frontier_phi_83_69_ladder_3;
                        _1536 = frontier_phi_83_69_ladder_2;
                        _1539 = frontier_phi_83_69_ladder_1;
                        _1542 = frontier_phi_83_69_ladder;
                    }
                    _1737 = _1533 * _480;
                    _1739 = _1536 * _480;
                    _1741 = _1539 * _480;
                    _1743 = _1542 * _480;
                }
                else
                {
                    uint4 _1314 = asuint(_55_m0[0u]);
                    float _1585;
                    float _1587;
                    float _1592;
                    float _1596;
                    if (_12.Load(int3(uint2(uint(float(_1314.x) * _883), uint(float(_1314.y) * _881)), 0u)).x > 0.0f)
                    {
                        uint _1403_dummy_parameter;
                        uint2 _1403 = spvTextureSize(_14, 0u, _1403_dummy_parameter);
                        float4 _1412 = _14.Load(int3(uint2(uint(float(_1403.x) * _883), uint(float(_1403.y) * _881)), 0u));
                        float _1416 = _1412.x * 0.5f;
                        float _1417 = _1412.y * (-0.5f);
                        float4 _1436 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _1416) + (_50_m0[52u].x * _883), (_50_m0[52u].w * _1417) + (_50_m0[52u].y * _881)), 0.0f);
                        float _1581;
                        if (_1178)
                        {
                            float frontier_phi_87_86_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _580))) < 5.0f)
                            {
                                float _1797 = sqrt((_1416 * _1416) + (_1417 * _1417));
                                float frontier_phi_87_86_ladder_102_ladder;
                                if (_1797 > 0.0500000007450580596923828125f)
                                {
                                    float _1964 = _883 - _587;
                                    float _1965 = _881 - _588;
                                    float frontier_phi_87_86_ladder_102_ladder_118_ladder;
                                    if (_1797 > sqrt((_1964 * _1964) + (_1965 * _1965)))
                                    {
                                        uint4 _2067 = asuint(_55_m0[0u]);
                                        uint _2074 = uint(float(_2067.x) * _883);
                                        uint _2075 = uint(float(_2067.y) * _881);
                                        uint4 _2078 = _24[2u].Load(int3(uint2(_2074, _2075), 0u));
                                        uint _2081 = _2078.w;
                                        uint4 _2086 = _24[15u].Load(int3(uint2(_2074, _2075), 0u));
                                        uint _2088 = _2086.y;
                                        uint _2094 = ((_2088 & 64u) != 0u) ? uint((_2088 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2095 = _2081 & 128u;
                                        uint _2097 = (_2095 != 0u) ? 1u : ((_2078.x << 7u) | _2081);
                                        uint4 _2100 = _16.Load(_2097 * 4u);
                                        uint _2101 = _2100.x;
                                        uint _2108 = ((_2101 & 1u) != 0u) ? 0u : 18u;
                                        uint _2190;
                                        if (_2095 == 0u)
                                        {
                                            _2190 = (((_2101 & 2097152u) != 0u) && (_2094 == uint(min(int(uint(max(int(_2094), int(0u)))), int(1u))))) ? (_2108 | 128u) : _2108;
                                        }
                                        else
                                        {
                                            _2190 = _2081;
                                        }
                                        float frontier_phi_87_86_ladder_102_ladder_118_ladder_141_ladder;
                                        if (((_2190 & 128u) | (_16.Load((_2097 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2230 = asuint(_55_m0[0u]);
                                            uint _2239 = uint(float(_2230.x) * (_1416 + _883));
                                            uint _2240 = uint(float(_2230.y) * (_1417 + _881));
                                            uint4 _2243 = _24[2u].Load(int3(uint2(_2239, _2240), 0u));
                                            uint _2246 = _2243.w;
                                            uint4 _2249 = _24[15u].Load(int3(uint2(_2239, _2240), 0u));
                                            uint _2251 = _2249.y;
                                            uint _2257 = ((_2251 & 64u) != 0u) ? uint((_2251 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2258 = _2246 & 128u;
                                            uint _2260 = (_2258 != 0u) ? 1u : ((_2243.x << 7u) | _2246);
                                            uint4 _2262 = _16.Load(_2260 * 4u);
                                            uint _2263 = _2262.x;
                                            uint _2270 = ((_2263 & 1u) != 0u) ? 0u : 18u;
                                            uint _2358;
                                            if (_2258 == 0u)
                                            {
                                                _2358 = (((_2263 & 2097152u) != 0u) && (_2257 == uint(min(int(uint(max(int(_2257), int(0u)))), int(1u))))) ? (_2270 | 128u) : _2270;
                                            }
                                            else
                                            {
                                                _2358 = _2246;
                                            }
                                            float frontier_phi_87_86_ladder_102_ladder_118_ladder_141_ladder_155_ladder;
                                            if ((_2358 & 128u) == 0u)
                                            {
                                                frontier_phi_87_86_ladder_102_ladder_118_ladder_141_ladder_155_ladder = ((_16.Load((_2260 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_87_86_ladder_102_ladder_118_ladder_141_ladder_155_ladder = 0.0f;
                                            }
                                            frontier_phi_87_86_ladder_102_ladder_118_ladder_141_ladder = frontier_phi_87_86_ladder_102_ladder_118_ladder_141_ladder_155_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_87_86_ladder_102_ladder_118_ladder_141_ladder = 1.0f;
                                        }
                                        frontier_phi_87_86_ladder_102_ladder_118_ladder = frontier_phi_87_86_ladder_102_ladder_118_ladder_141_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_87_86_ladder_102_ladder_118_ladder = 1.0f;
                                    }
                                    frontier_phi_87_86_ladder_102_ladder = frontier_phi_87_86_ladder_102_ladder_118_ladder;
                                }
                                else
                                {
                                    frontier_phi_87_86_ladder_102_ladder = 1.0f;
                                }
                                frontier_phi_87_86_ladder = frontier_phi_87_86_ladder_102_ladder;
                            }
                            else
                            {
                                frontier_phi_87_86_ladder = 1.0f;
                            }
                            _1581 = frontier_phi_87_86_ladder;
                        }
                        else
                        {
                            _1581 = 1.0f;
                        }
                        float _1583 = _1581 * _1216;
                        float _1591;
                        float _1595;
                        float _1599;
                        if (_895 == 0u)
                        {
                            _1591 = _50_m0[54u].x * _1436.x;
                            _1595 = _50_m0[54u].x * _1436.y;
                            _1599 = _50_m0[54u].x * _1436.z;
                        }
                        else
                        {
                            _1591 = _50_m0[1u].w;
                            _1595 = _50_m0[2u].x;
                            _1599 = _50_m0[2u].y;
                        }
                        float frontier_phi_88_103_ladder;
                        float frontier_phi_88_103_ladder_1;
                        float frontier_phi_88_103_ladder_2;
                        float frontier_phi_88_103_ladder_3;
                        for (;;)
                        {
                            if (_893 > 0.0f)
                            {
                                float _1590;
                                float _1594;
                                float _1598;
                                if (_1178)
                                {
                                    _1590 = 0.0f;
                                    _1594 = 0.0f;
                                    _1598 = 0.0f;
                                }
                                else
                                {
                                    if (!((_402 != 4u) || (_901 != 0u)))
                                    {
                                        frontier_phi_88_103_ladder = _1599;
                                        frontier_phi_88_103_ladder_1 = _1595;
                                        frontier_phi_88_103_ladder_2 = _1591;
                                        frontier_phi_88_103_ladder_3 = _1583;
                                        break;
                                    }
                                    _1590 = _1591;
                                    _1594 = _1595;
                                    _1598 = _1599;
                                }
                                frontier_phi_88_103_ladder = _1598;
                                frontier_phi_88_103_ladder_1 = _1594;
                                frontier_phi_88_103_ladder_2 = _1590;
                                frontier_phi_88_103_ladder_3 = (1.0f - exp2(log2(_893) * _50_m0[4u].x)) * _1583;
                                break;
                            }
                            else
                            {
                                frontier_phi_88_103_ladder = _1599;
                                frontier_phi_88_103_ladder_1 = _1595;
                                frontier_phi_88_103_ladder_2 = _1591;
                                frontier_phi_88_103_ladder_3 = _1583;
                                break;
                            }
                        }
                        _1585 = frontier_phi_88_103_ladder_3;
                        _1587 = frontier_phi_88_103_ladder_2;
                        _1592 = frontier_phi_88_103_ladder_1;
                        _1596 = frontier_phi_88_103_ladder;
                    }
                    else
                    {
                        float frontier_phi_88_71_ladder;
                        float frontier_phi_88_71_ladder_1;
                        float frontier_phi_88_71_ladder_2;
                        float frontier_phi_88_71_ladder_3;
                        if (_903)
                        {
                            frontier_phi_88_71_ladder = _1589;
                            frontier_phi_88_71_ladder_1 = _1589;
                            frontier_phi_88_71_ladder_2 = _1589;
                            frontier_phi_88_71_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float4 _1605 = _28[7u].SampleLevel(_59, float3(_1043, _1046, _1049), 0.0f);
                            frontier_phi_88_71_ladder = _1605.z;
                            frontier_phi_88_71_ladder_1 = _1605.y;
                            frontier_phi_88_71_ladder_2 = _1605.x;
                            frontier_phi_88_71_ladder_3 = _1216;
                        }
                        _1585 = frontier_phi_88_71_ladder_3;
                        _1587 = frontier_phi_88_71_ladder_2;
                        _1592 = frontier_phi_88_71_ladder_1;
                        _1596 = frontier_phi_88_71_ladder;
                    }
                    float _1807;
                    float _1808;
                    float _1810;
                    float _1812;
                    if (_1137 == 0u)
                    {
                        _1807 = _1585;
                        _1808 = _1587;
                        _1810 = _1592;
                        _1812 = _1596;
                    }
                    else
                    {
                        _1807 = _1214;
                        _1808 = ((_1587 - _1138) * _995) + _1138;
                        _1810 = ((_1592 - _1140) * _995) + _1140;
                        _1812 = ((_1596 - _1142) * _995) + _1142;
                    }
                    float _1997;
                    float _1998;
                    float _1999;
                    float _2000;
                    if (_1217 && (_1807 < 1.0f))
                    {
                        float _1971 = _1046 * _1040;
                        float _1975 = rsqrt(dot(float3(_1043, _1971, _1049), float3(_1043, _1971, _1049)));
                        float4 _1983 = _28[4u].SampleLevel(_59, float3(_1975 * _1043, _1975 * _1971, _1975 * _1049), 0.0f);
                        float _1985 = _1983.x;
                        float _1986 = _1983.y;
                        float _1987 = _1983.z;
                        _1997 = 1.0f;
                        _1998 = ((_1808 - _1985) * _1807) + _1985;
                        _1999 = ((_1810 - _1986) * _1807) + _1986;
                        _2000 = ((_1812 - _1987) * _1807) + _1987;
                    }
                    else
                    {
                        _1997 = _1807;
                        _1998 = _1808;
                        _1999 = _1810;
                        _2000 = _1812;
                    }
                    float _1744 = _1997 * _480;
                    _1737 = _1998 * _1744;
                    _1739 = _1999 * _1744;
                    _1741 = _2000 * _1744;
                    _1743 = _1744;
                }
                float _1748 = _50_m0[58u].z * _1151;
                float _1770 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _1772 = _1770 * ((_1748 * ((_1147 * 1000.0f) - _1737)) + _1737);
                float _1773 = _1770 * ((_1748 * ((_1148 * 1000.0f) - _1739)) + _1739);
                float _1774 = _1770 * ((_1748 * (_1149 - _1741)) + _1741);
                float _1780 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_1772, max(_1773, _1774)) + 1.0f);
                float _1784 = min(_1780 * _1772, 0.996078431606292724609375f);
                float _1786 = min(_1780 * _1773, 0.996078431606292724609375f);
                float _1787 = min(_1780 * _1774, 0.996078431606292724609375f);
                _35[uint2(_226, _229)] = float4(_1784, _1786, _1787, _1743);
                _39[uint2(_226, _229)] = float4(_1177, 0.0f, 0.0f, _1177);
                if (_233)
                {
                    uint _1957 = _226 + 1u;
                    _35[uint2(_1957, _229)] = float4(_1784, _1786, _1787, _1743);
                    _39[uint2(_1957, _229)] = float4(_1177, 0.0f, 0.0f, _1177);
                }
                if (_236)
                {
                    uint _2058 = _229 + 1u;
                    _35[uint2(_226, _2058)] = float4(_1784, _1786, _1787, _1743);
                    _39[uint2(_226, _2058)] = float4(_1177, 0.0f, 0.0f, _1177);
                }
                if (_237)
                {
                    uint _2174 = _226 + 1u;
                    uint _2175 = _229 + 1u;
                    _35[uint2(_2174, _2175)] = float4(_1784, _1786, _1787, _1743);
                    _39[uint2(_2174, _2175)] = float4(_1177, 0.0f, 0.0f, _1177);
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
                uint _447 = _226 + 1u;
                _35[uint2(_447, _229)] = 0.0f.xxxx;
                _39[uint2(_447, _229)] = 0.0f.xxxx;
            }
            if (_236)
            {
                uint _777 = _229 + 1u;
                _35[uint2(_226, _777)] = 0.0f.xxxx;
                _39[uint2(_226, _777)] = 0.0f.xxxx;
            }
            if (!_237)
            {
                break;
            }
            uint _869 = _226 + 1u;
            uint _870 = _229 + 1u;
            _35[uint2(_869, _870)] = 0.0f.xxxx;
            _39[uint2(_869, _870)] = 0.0f.xxxx;
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
