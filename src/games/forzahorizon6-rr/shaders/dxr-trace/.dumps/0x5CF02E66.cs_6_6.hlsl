static float _1696;
static uint _2784;
static float _2785;
static float _2786;
static float _2787;
static float _2788;
static float _2789;
static float _2790;
static uint _2791;
static uint _2792;
static float _2793;
static float _2794;
static float _2795;
static float _2796;
static float _2797;
static float _2798;
static float _2804;
static uint _2805;
static float _2806;
static uint _2808;
static uint _2809;

cbuffer _42_44 : register(b2, space0)
{
    float4 _44_m0[700] : packoffset(c0);
};

cbuffer _47_49 : register(b3, space0)
{
    float4 _49_m0[69] : packoffset(c0);
};

cbuffer _52_54 : register(b4, space0)
{
    float4 _54_m0[1] : packoffset(c0);
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
SamplerState _57 : register(s0, space0);
SamplerState _58 : register(s5, space0);

static uint3 gl_WorkGroupID;
static uint gl_LocalInvocationIndex;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint gl_LocalInvocationIndex : SV_GroupIndex;
};

static RayQuery<RAY_FLAG_NONE> _2208;

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
    uint _76;
    float _80;
    float _81;
    float _82;
    float _86;
    float _87;
    float _88;
    float _92;
    float _93;
    float _94;
    float _98;
    float _99;
    float _100;
    float _101;
    float _105;
    float _106;
    float _107;
    float _108;
    float _112;
    float _113;
    float _114;
    float _115;
    float _119;
    float _120;
    float _121;
    float _122;
    float _126;
    float _127;
    float _128;
    float _129;
    float _133;
    float _134;
    float _135;
    float _136;
    float _140;
    float _141;
    float _142;
    float _143;
    float _147;
    float _148;
    float _149;
    float _150;
    float _154;
    float _155;
    float _156;
    float _160;
    float _161;
    float _162;
    float _166;
    float _167;
    float _168;
    float _172;
    float _173;
    float _174;
    float _175;
    float _179;
    float _180;
    float _181;
    float _182;
    float _186;
    float _187;
    float _188;
    float _189;
    float _193;
    float _194;
    float _195;
    float _196;
    float _201;
    float _203;
    uint _208;
    uint _213;
    for (;;)
    {
        uint4 _75 = asuint(_44_m0[176u]);
        _76 = _75.z;
        _80 = _49_m0[14u].x;
        _81 = _49_m0[14u].y;
        _82 = _49_m0[14u].z;
        _86 = _49_m0[15u].x;
        _87 = _49_m0[15u].y;
        _88 = _49_m0[15u].z;
        _92 = _49_m0[16u].x;
        _93 = _49_m0[16u].y;
        _94 = _49_m0[16u].z;
        _98 = _49_m0[22u].x;
        _99 = _49_m0[22u].y;
        _100 = _49_m0[22u].z;
        _101 = _49_m0[22u].w;
        _105 = _49_m0[23u].x;
        _106 = _49_m0[23u].y;
        _107 = _49_m0[23u].z;
        _108 = _49_m0[23u].w;
        _112 = _49_m0[24u].x;
        _113 = _49_m0[24u].y;
        _114 = _49_m0[24u].z;
        _115 = _49_m0[24u].w;
        _119 = _49_m0[25u].x;
        _120 = _49_m0[25u].y;
        _121 = _49_m0[25u].z;
        _122 = _49_m0[25u].w;
        _126 = _49_m0[26u].x;
        _127 = _49_m0[26u].y;
        _128 = _49_m0[26u].z;
        _129 = _49_m0[26u].w;
        _133 = _49_m0[27u].x;
        _134 = _49_m0[27u].y;
        _135 = _49_m0[27u].z;
        _136 = _49_m0[27u].w;
        _140 = _49_m0[28u].x;
        _141 = _49_m0[28u].y;
        _142 = _49_m0[28u].z;
        _143 = _49_m0[28u].w;
        _147 = _49_m0[29u].x;
        _148 = _49_m0[29u].y;
        _149 = _49_m0[29u].z;
        _150 = _49_m0[29u].w;
        _154 = _49_m0[18u].x;
        _155 = _49_m0[18u].y;
        _156 = _49_m0[18u].z;
        _160 = _49_m0[19u].x;
        _161 = _49_m0[19u].y;
        _162 = _49_m0[19u].z;
        _166 = _49_m0[20u].x;
        _167 = _49_m0[20u].y;
        _168 = _49_m0[20u].z;
        _172 = _49_m0[30u].x;
        _173 = _49_m0[30u].y;
        _174 = _49_m0[30u].z;
        _175 = _49_m0[30u].w;
        _179 = _49_m0[31u].x;
        _180 = _49_m0[31u].y;
        _181 = _49_m0[31u].z;
        _182 = _49_m0[31u].w;
        _186 = _49_m0[32u].x;
        _187 = _49_m0[32u].y;
        _188 = _49_m0[32u].z;
        _189 = _49_m0[32u].w;
        _193 = _49_m0[33u].x;
        _194 = _49_m0[33u].y;
        _195 = _49_m0[33u].z;
        _196 = _49_m0[33u].w;
        uint4 _199 = asuint(_54_m0[0u]);
        _201 = float(_199.x);
        _203 = float(_199.y);
        uint4 _207 = asuint(_49_m0[5u]);
        _208 = _207.x;
        _213 = (((gl_WorkGroupID.y << 6u) + gl_WorkGroupID.x) << 6u) + gl_LocalInvocationIndex;
        if (_213 < _38[1u].xxxx.x)
        {
            bool ladder_phi_8;
            uint _225;
            uint _228;
            bool _232;
            bool _235;
            bool _236;
            uint _237;
            uint _238;
            float _258;
            float _259;
            float _266;
            bool _267;
            uint _269;
            uint _270;
            for (;;)
            {
                uint4 _223 = _8.Load(_213);
                uint _224 = _223.x;
                _225 = _224 & 32767u;
                uint _227 = _224 >> 15u;
                _228 = _227 & 16383u;
                _232 = (_224 & 536870912u) != 0u;
                _235 = (_224 & 1073741824u) != 0u;
                _236 = int(_224) < int(0u);
                _237 = uint(_201);
                _238 = uint(_203);
                bool _241 = ((_227 + _224) & 1u) == 0u;
                float _251 = float(int(_225));
                float _252 = float(int(_228));
                _258 = ((_251 + 0.5f) + (_241 ? _44_m0[58u].x : _44_m0[58u].z)) * (1.0f / _201);
                _259 = ((_252 + 0.5f) + (_241 ? _44_m0[58u].y : _44_m0[58u].w)) * (1.0f / _203);
                _266 = _20[21u].Load(int3(uint2(_225, _228), 0u)).x;
                _267 = _266 > 0.0f;
                _269 = uint(_251);
                _270 = uint(_252);
                float _395;
                float _397;
                float _399;
                uint _401;
                float _403;
                float _405;
                uint _407;
                float _409;
                if (_267)
                {
                    uint4 _274 = _24[22u].Load(int3(uint2(_269, _270), 0u));
                    uint _276 = _274.x;
                    float _289 = (float((_276 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _291 = (float(_276 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _296 = (1.0f - abs(_289)) - abs(_291);
                    float _299 = clamp((-0.0f) - _296, 0.0f, 1.0f);
                    float _300 = (-0.0f) - _299;
                    float _305 = ((_289 >= 0.0f) ? _300 : _299) + _289;
                    float _306 = ((_291 >= 0.0f) ? _300 : _299) + _291;
                    float _311 = rsqrt(dot(float3(_305, _306, _296), float3(_305, _306, _296)));
                    uint _323 = _24[23u].Load(int3(uint2(_269, _270), 0u)).y + 4294967295u;
                    uint _326 = uint(int(_323) >> int(31u)) & 3u;
                    uint _331 = _326 + 103u;
                    float _340 = clamp(((float(_276 & 255u) * 0.0039215688593685626983642578125f) - _44_m0[_331].x) / (_44_m0[_331].y - _44_m0[_331].x), 0.0f, 1.0f);
                    _395 = _305 * _311;
                    _397 = _306 * _311;
                    _399 = _311 * _296;
                    _401 = _326 + 1u;
                    _403 = (_340 * _340) * (3.0f - (_340 * 2.0f));
                    _405 = _44_m0[_331].z;
                    _407 = (int(_323) < int(0u)) ? 0u : _323;
                    _409 = 0.0f;
                }
                else
                {
                    uint4 _351 = _24[2u].Load(int3(uint2(_269, _270), 0u));
                    uint _353 = _351.x;
                    uint _354 = _351.w;
                    uint4 _360 = _24[15u].Load(int3(uint2(_269, _270), 0u));
                    uint _362 = _360.y;
                    uint _371 = ((_362 & 64u) != 0u) ? uint((_362 & 4294967167u) != 66u) : 4294967295u;
                    uint _372 = _354 & 128u;
                    uint _375 = (_372 != 0u) ? 1u : ((_353 << 7u) | _354);
                    uint4 _379 = _16.Load(_375 * 4u);
                    uint _380 = _379.x;
                    uint4 _383 = _16.Load((_375 * 4u) + 1u);
                    uint _384 = _383.x;
                    uint4 _387 = _16.Load((_375 * 4u) + 3u);
                    uint _388 = _387.x;
                    uint _391 = ((_380 & 1u) != 0u) ? 0u : 18u;
                    uint _393 = uint(min(int(uint(max(int(_371), int(0u)))), int(1u)));
                    uint _426;
                    uint _427;
                    if (_372 == 0u)
                    {
                        _426 = (((_380 & 2097152u) != 0u) && (_371 == _393)) ? (_391 | 128u) : _391;
                        _427 = _380;
                    }
                    else
                    {
                        _426 = _354;
                        _427 = _380 | ((_353 << 20u) & 134217728u);
                    }
                    uint _436 = _384 & 512u;
                    float _1084;
                    float _1086;
                    float _1088;
                    float _1090;
                    float _1092;
                    float _1094;
                    float _1096;
                    float _1098;
                    float _1100;
                    float _1102;
                    float _1104;
                    float _1106;
                    if (_436 == 0u)
                    {
                        if (!((_427 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _822 = asfloat(_17.Load((_388 * 115u) + 33u).x);
                        uint4 _830 = _24[2u].Load(int3(uint2(_269, _270), 0u));
                        uint _832 = _830.y;
                        uint _833 = _426 & 128u;
                        uint _1072;
                        uint _1073;
                        uint _1074;
                        uint _1075;
                        if (_833 == 0u)
                        {
                            _1072 = uint(((_427 & 817889384u) | (_384 & 576u)) != 0u) | (((_427 >> 19u) & 1u) ^ 1u);
                            _1073 = uint(((_427 & 17825808u) | (_384 & 520u)) != 0u);
                            _1074 = uint(((_427 & 46137344u) | (_384 & 2564u)) != 0u);
                            _1075 = 0u;
                        }
                        else
                        {
                            _1072 = 1u;
                            _1073 = _426 & 1u;
                            _1074 = 1u;
                            _1075 = 1u;
                        }
                        precise float _1079 = float(_832 & 127u) * 0.0078740157186985015869140625f;
                        bool _1083 = (_427 & 4194304u) == 0u;
                        float _1186;
                        if (_1083)
                        {
                            _1186 = _1079;
                        }
                        else
                        {
                            _1186 = float(_832 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1341;
                        if ((_427 & 134217728u) == 0u)
                        {
                            uint frontier_phi_71_57_ladder;
                            if ((_833 != 0u) || ((_427 & 17825792u) == 1048576u))
                            {
                                frontier_phi_71_57_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_71_57_ladder = _1073;
                            }
                            _1341 = frontier_phi_71_57_ladder;
                        }
                        else
                        {
                            _1341 = _1073;
                        }
                        uint4 _1344 = _24[1u].Load(int3(uint2(_269, _270), 0u));
                        uint _1346 = _1344.x;
                        float _1586;
                        float _1588;
                        float _1590;
                        if (_1072 == 0u)
                        {
                            _1586 = 0.0f;
                            _1588 = 0.0f;
                            _1590 = 0.0f;
                        }
                        else
                        {
                            float4 _1595 = _20[8u].Load(int3(uint2(_269, _270), 0u));
                            _1586 = _1595.x;
                            _1588 = _1595.y;
                            _1590 = _1595.z;
                        }
                        uint _1763;
                        if (_1341 == 0u)
                        {
                            _1763 = 0u;
                        }
                        else
                        {
                            _1763 = _24[9u].Load(int3(uint2(_269, _270), 0u)).x;
                        }
                        uint _1837;
                        if (_1074 == 0u)
                        {
                            _1837 = 0u;
                        }
                        else
                        {
                            _1837 = _24[10u].Load(int3(uint2(_269, _270), 0u)).x;
                        }
                        float _1847 = (float((_1346 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1848 = (float(_1346 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1852 = (1.0f - abs(_1847)) - abs(_1848);
                        float _1854 = clamp((-0.0f) - _1852, 0.0f, 1.0f);
                        float _1855 = (-0.0f) - _1854;
                        float _1860 = ((_1847 >= 0.0f) ? _1855 : _1854) + _1847;
                        float _1861 = ((_1848 >= 0.0f) ? _1855 : _1854) + _1848;
                        float _1865 = rsqrt(dot(float3(_1860, _1861, _1852), float3(_1860, _1861, _1852)));
                        float _1866 = _1860 * _1865;
                        float _1867 = _1861 * _1865;
                        float _1868 = _1865 * _1852;
                        float _1085 = float(_1346 & 255u);
                        float _1872 = ((_427 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _2032;
                        float _2033;
                        float _2034;
                        float _2035;
                        uint _2036;
                        if ((_384 & 64u) == 0u)
                        {
                            float frontier_phi_138_130_ladder;
                            float frontier_phi_138_130_ladder_1;
                            float frontier_phi_138_130_ladder_2;
                            float frontier_phi_138_130_ladder_3;
                            uint frontier_phi_138_130_ladder_4;
                            if ((_427 & 276824064u) == 0u)
                            {
                                frontier_phi_138_130_ladder = 0.0f;
                                frontier_phi_138_130_ladder_1 = ((_427 & 8u) != 0u) ? _1588 : _1872;
                                frontier_phi_138_130_ladder_2 = 0.0f;
                                frontier_phi_138_130_ladder_3 = 0.0f;
                                frontier_phi_138_130_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_138_130_ladder = 0.0f;
                                frontier_phi_138_130_ladder_1 = _1872;
                                frontier_phi_138_130_ladder_2 = 0.0f;
                                frontier_phi_138_130_ladder_3 = 0.0f;
                                frontier_phi_138_130_ladder_4 = 0u;
                            }
                            _2032 = frontier_phi_138_130_ladder_1;
                            _2033 = frontier_phi_138_130_ladder_2;
                            _2034 = frontier_phi_138_130_ladder;
                            _2035 = frontier_phi_138_130_ladder_3;
                            _2036 = frontier_phi_138_130_ladder_4;
                        }
                        else
                        {
                            float _1984 = (_1588 * 2.0f) + (-1.0f);
                            float _1985 = (_1590 * 2.0f) + (-1.0f);
                            float _1989 = (1.0f - abs(_1984)) - abs(_1985);
                            float _1991 = clamp((-0.0f) - _1989, 0.0f, 1.0f);
                            float _1992 = (-0.0f) - _1991;
                            float _1997 = ((_1984 >= 0.0f) ? _1992 : _1991) + _1984;
                            float _1998 = ((_1985 >= 0.0f) ? _1992 : _1991) + _1985;
                            float _2002 = rsqrt(dot(float3(_1997, _1998, _1989), float3(_1997, _1998, _1989)));
                            _2032 = floor(round(_1586 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _2033 = _1997 * _2002;
                            _2034 = _1998 * _2002;
                            _2035 = _2002 * _1989;
                            _2036 = 1u;
                        }
                        float _1093;
                        if ((_427 & 32768u) == 0u)
                        {
                            _1093 = _2032;
                        }
                        else
                        {
                            float frontier_phi_142_143_ladder;
                            if (_17.Load((_388 * 115u) + 36u).x == 0u)
                            {
                                float _2173 = clamp((_1085 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _822;
                                frontier_phi_142_143_ladder = ((_427 & 131072u) != 0u) ? _2173 : ((((clamp((1.21000003814697265625f / (exp2((_1186 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_388 * 115u) + 32u).x)) + 1.0f) * _2173);
                            }
                            else
                            {
                                frontier_phi_142_143_ladder = _822;
                            }
                            _1093 = frontier_phi_142_143_ladder;
                        }
                        uint _2084 = _426 & 1u;
                        float _2116;
                        float _2118;
                        float _2120;
                        uint _2122;
                        if (((_427 & 16u) == 0u) || (((_2084 | (_384 & 8u)) | (_427 & 16777216u)) != 0u))
                        {
                            _2116 = _2033;
                            _2118 = _2034;
                            _2120 = _2035;
                            _2122 = _2036;
                        }
                        else
                        {
                            float _2132 = (float(_1763 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2133 = (float(_1763 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2137 = (1.0f - abs(_2132)) - abs(_2133);
                            float _2139 = clamp((-0.0f) - _2137, 0.0f, 1.0f);
                            float _2140 = (-0.0f) - _2139;
                            float _2145 = ((_2132 >= 0.0f) ? _2140 : _2139) + _2132;
                            float _2146 = ((_2133 >= 0.0f) ? _2140 : _2139) + _2133;
                            float _2150 = rsqrt(dot(float3(_2145, _2146, _2137), float3(_2145, _2146, _2137)));
                            _2116 = _2145 * _2150;
                            _2118 = _2146 * _2150;
                            _2120 = _2150 * _2137;
                            _2122 = 1u;
                        }
                        float _1087;
                        float _1089;
                        float _1091;
                        if (_2084 == 0u)
                        {
                            float frontier_phi_154_153_ladder;
                            float frontier_phi_154_153_ladder_1;
                            float frontier_phi_154_153_ladder_2;
                            if (((_426 & 64u) == 0u) && (_1075 != 0u))
                            {
                                float2 _2272 = spvUnpackHalf2x16((_1837 >> 17u) & 32736u);
                                float _2273 = _2272.x;
                                float _2276 = (spvUnpackHalf2x16((_1837 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2277 = (spvUnpackHalf2x16((_1837 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2281 = (1.0f - abs(_2276)) - abs(_2277);
                                float _2283 = clamp((-0.0f) - _2281, 0.0f, 1.0f);
                                float _2284 = (-0.0f) - _2283;
                                float _2289 = ((_2276 >= 0.0f) ? _2284 : _2283) + _2276;
                                float _2290 = ((_2277 >= 0.0f) ? _2284 : _2283) + _2277;
                                float _2294 = rsqrt(dot(float3(_2289, _2290, _2281), float3(_2289, _2290, _2281)));
                                float _2304 = (((_2289 * _2294) - _1866) * _2273) + _1866;
                                float _2305 = (((_2290 * _2294) - _1867) * _2273) + _1867;
                                float _2306 = (((_2294 * _2281) - _1868) * _2273) + _1868;
                                float _2310 = rsqrt(dot(float3(_2304, _2305, _2306), float3(_2304, _2305, _2306)));
                                frontier_phi_154_153_ladder = _2304 * _2310;
                                frontier_phi_154_153_ladder_1 = _2305 * _2310;
                                frontier_phi_154_153_ladder_2 = _2306 * _2310;
                            }
                            else
                            {
                                frontier_phi_154_153_ladder = _1866;
                                frontier_phi_154_153_ladder_1 = _1867;
                                frontier_phi_154_153_ladder_2 = _1868;
                            }
                            _1087 = frontier_phi_154_153_ladder;
                            _1089 = frontier_phi_154_153_ladder_1;
                            _1091 = frontier_phi_154_153_ladder_2;
                        }
                        else
                        {
                            _1087 = _1866;
                            _1089 = _1867;
                            _1091 = _1868;
                        }
                        float _1101;
                        float _1103;
                        float _1105;
                        float _1107;
                        if (_1083)
                        {
                            float frontier_phi_159_158_ladder;
                            float frontier_phi_159_158_ladder_1;
                            float frontier_phi_159_158_ladder_2;
                            float frontier_phi_159_158_ladder_3;
                            if (((_427 & 33554432u) == 0u) || (((_384 & 4u) != 0u) && ((_427 & 8388608u) == 0u)))
                            {
                                frontier_phi_159_158_ladder = 0.0f;
                                frontier_phi_159_158_ladder_1 = 0.0f;
                                frontier_phi_159_158_ladder_2 = 0.0f;
                                frontier_phi_159_158_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2342 = (spvUnpackHalf2x16((_1837 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2343 = (spvUnpackHalf2x16((_1837 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2347 = (1.0f - abs(_2342)) - abs(_2343);
                                float _2349 = clamp((-0.0f) - _2347, 0.0f, 1.0f);
                                float _2350 = (-0.0f) - _2349;
                                float _2355 = ((_2342 >= 0.0f) ? _2350 : _2349) + _2342;
                                float _2356 = ((_2343 >= 0.0f) ? _2350 : _2349) + _2343;
                                float _2360 = rsqrt(dot(float3(_2355, _2356, _2347), float3(_2355, _2356, _2347)));
                                float _2361 = _2355 * _2360;
                                float _2362 = _2356 * _2360;
                                float _2363 = _2360 * _2347;
                                float _2367 = rsqrt(dot(float3(_2361, _2362, _2363), float3(_2361, _2362, _2363)));
                                frontier_phi_159_158_ladder = _2367 * _2363;
                                frontier_phi_159_158_ladder_1 = _2367 * _2362;
                                frontier_phi_159_158_ladder_2 = _2367 * _2361;
                                frontier_phi_159_158_ladder_3 = spvUnpackHalf2x16((_1837 >> 17u) & 32736u).x;
                            }
                            _1101 = frontier_phi_159_158_ladder_3;
                            _1103 = frontier_phi_159_158_ladder_2;
                            _1105 = frontier_phi_159_158_ladder_1;
                            _1107 = frontier_phi_159_158_ladder;
                        }
                        else
                        {
                            _1101 = 0.0f;
                            _1103 = 0.0f;
                            _1105 = 0.0f;
                            _1107 = 0.0f;
                        }
                        bool _2324 = _2122 != 0u;
                        _1084 = _1085;
                        _1086 = _1087;
                        _1088 = _1089;
                        _1090 = _1091;
                        _1092 = _1093;
                        _1094 = _2324 ? _2116 : _1087;
                        _1096 = _2324 ? _2118 : _1089;
                        _1098 = _2324 ? _2120 : _1091;
                        _1100 = _1101;
                        _1102 = _1103;
                        _1104 = _1105;
                        _1106 = _1107;
                    }
                    else
                    {
                        uint4 _638 = _24[1u].Load(int3(uint2(_269, _270), 0u));
                        uint _640 = _638.x;
                        uint4 _644 = _24[9u].Load(int3(uint2(_269, _270), 0u));
                        uint _646 = _644.x;
                        float _981;
                        float _982;
                        float _983;
                        if ((_427 & 33554432u) == 0u)
                        {
                            float _842 = (float((_640 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _843 = (float(_640 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _847 = (1.0f - abs(_842)) - abs(_843);
                            float _849 = clamp((-0.0f) - _847, 0.0f, 1.0f);
                            float _850 = (-0.0f) - _849;
                            float _855 = ((_842 >= 0.0f) ? _850 : _849) + _842;
                            float _856 = ((_843 >= 0.0f) ? _850 : _849) + _843;
                            float _860 = rsqrt(dot(float3(_855, _856, _847), float3(_855, _856, _847)));
                            _981 = _855 * _860;
                            _982 = _856 * _860;
                            _983 = _860 * _847;
                        }
                        else
                        {
                            float _871 = (float((_646 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _872 = (float(_646 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _876 = (1.0f - abs(_871)) - abs(_872);
                            float _878 = clamp((-0.0f) - _876, 0.0f, 1.0f);
                            float _879 = (-0.0f) - _878;
                            float _884 = ((_871 >= 0.0f) ? _879 : _878) + _871;
                            float _885 = ((_872 >= 0.0f) ? _879 : _878) + _872;
                            float _889 = rsqrt(dot(float3(_884, _885, _876), float3(_884, _885, _876)));
                            _981 = _884 * _889;
                            _982 = _885 * _889;
                            _983 = _889 * _876;
                        }
                        _1084 = float(_640 & 255u);
                        _1086 = _981;
                        _1088 = _982;
                        _1090 = _983;
                        _1092 = 1.0f;
                        _1094 = _981;
                        _1096 = _982;
                        _1098 = _983;
                        _1100 = 0.0f;
                        _1102 = 0.0f;
                        _1104 = 0.0f;
                        _1106 = 0.0f;
                    }
                    precise float _1108 = _1084 * 0.0039215688593685626983642578125f;
                    if ((_426 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1195 = ((_426 & 128u) | _436) != 0u;
                    uint _1235;
                    if (_1195)
                    {
                        _1235 = 1u;
                    }
                    else
                    {
                        _1235 = (((_427 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _396;
                    float _398;
                    float _400;
                    uint _402;
                    float _1354;
                    if (((_427 & 33554432u) == 0u) || _1195)
                    {
                        bool _1348 = _76 != 0u;
                        uint _1356;
                        if ((_427 & 16u) == 0u)
                        {
                            _1356 = _1235;
                        }
                        else
                        {
                            _1356 = ((_427 & 268435456u) != 0u) ? _1235 : 2u;
                        }
                        _1354 = _1092 * _1108;
                        _396 = _1348 ? _1094 : _1086;
                        _398 = _1348 ? _1096 : _1088;
                        _400 = _1348 ? _1098 : _1090;
                        _402 = _1356;
                    }
                    else
                    {
                        _1354 = _1100;
                        _396 = _1102;
                        _398 = _1104;
                        _400 = _1106;
                        _402 = _1235;
                    }
                    uint _1357 = _402 + 102u;
                    float _1366 = clamp((_1354 - _44_m0[_1357].x) / (_44_m0[_1357].y - _44_m0[_1357].x), 0.0f, 1.0f);
                    _395 = _396;
                    _397 = _398;
                    _399 = _400;
                    _401 = _402;
                    _403 = (_1366 * _1366) * (3.0f - (_1366 * 2.0f));
                    _405 = _44_m0[_1357].z;
                    _407 = (_402 == 1u) ? _393 : 0u;
                    _409 = asfloat(_17.Load((_388 * 115u) + 114u).x);
                }
                if (_403 == 0.0f)
                {
                    ladder_phi_8 = false;
                    break;
                }
                float _443;
                if (_267)
                {
                    _443 = _266;
                }
                else
                {
                    _443 = _12.Load(int3(uint2(uint(int(_258 * float(_237))), uint(int(_259 * float(_238)))), 0u)).x;
                }
                float _452 = _49_m0[50u].w + _49_m0[50u].y;
                uint _455 = _401 + 63u;
                float _464 = clamp(((_49_m0[50u].x / (_452 - (_49_m0[50u].y * _443))) - _49_m0[_455].y) / (_49_m0[_455].x - _49_m0[_455].y), 0.0f, 1.0f);
                float _469 = ((_464 * _464) * _403) * (3.0f - (_464 * 2.0f));
                float _480 = ((_258 * 2.0f) * _49_m0[51u].z) + (-1.0f);
                float _481 = ((1.0f - (_49_m0[51u].w * _259)) * 2.0f) + (-1.0f);
                float _497 = mad(_143, _443, mad(_136, _481, _480 * _129)) + _150;
                float _498 = (mad(_140, _443, mad(_133, _481, _480 * _126)) + _147) / _497;
                float _499 = (mad(_141, _443, mad(_134, _481, _480 * _127)) + _148) / _497;
                float _500 = (mad(_142, _443, mad(_135, _481, _480 * _128)) + _149) / _497;
                float _504 = rsqrt(dot(float3(_498, _499, _500), float3(_498, _499, _500)));
                float _505 = _504 * _498;
                float _506 = _504 * _499;
                float _507 = _504 * _500;
                float _510 = mad(_92, _399, mad(_86, _397, _395 * _80));
                float _513 = mad(_93, _399, mad(_87, _397, _395 * _81));
                float _516 = mad(_94, _399, mad(_88, _397, _395 * _82));
                float _520 = dot(float3(_505, _506, _507), float3(_510, _513, _516)) * 2.0f;
                float _524 = _505 - (_520 * _510);
                float _525 = _506 - (_520 * _513);
                float _526 = _507 - (_520 * _516);
                bool _530 = dot(float3(_524, _525, _526), float3(_505, _506, _507)) < 0.0f;
                float _537 = sqrt(((_499 * _499) + (_498 * _498)) + (_500 * _500)) * 0.001000000047497451305389404296875f;
                float _548 = ((_537 * _510) + _498) + (_524 * _405);
                float _549 = ((_537 * _513) + _499) + (_525 * _405);
                float _550 = ((_537 * _516) + _500) + (_526 * _405);
                float _566 = mad(_115, _550, mad(_108, _549, _548 * _101)) + _122;
                float _569 = (mad(_114, _550, mad(_107, _549, _548 * _100)) + _121) / _566;
                float _572 = (((mad(_112, _550, mad(_105, _549, _548 * _98)) + _119) / _566) * 0.5f) + 0.5f;
                float _573 = 0.5f - (((mad(_113, _550, mad(_106, _549, _548 * _99)) + _120) / _566) * 0.5f);
                float _576 = _572 * _49_m0[51u].x;
                float _577 = _573 * _49_m0[51u].y;
                float _582 = _548 + (_524 * 0.100000001490116119384765625f);
                float _583 = _549 + (_525 * 0.100000001490116119384765625f);
                float _584 = _550 + (_526 * 0.100000001490116119384765625f);
                float _600 = mad(_115, _584, mad(_108, _583, _582 * _101)) + _122;
                float _609 = _49_m0[51u].x * (((((mad(_112, _584, mad(_105, _583, _582 * _98)) + _119) / _600) * 0.5f) + 0.5f) - _572);
                float _611 = _49_m0[51u].y * ((0.5f - (((mad(_113, _584, mad(_106, _583, _582 * _99)) + _120) / _600) * 0.5f)) - _573);
                float _612 = ((mad(_114, _584, mad(_107, _583, _582 * _100)) + _121) / _600) - _569;
                float _613 = _609 * 10.0f;
                float _615 = _611 * 10.0f;
                float _616 = _612 * 10.0f;
                bool _623 = _401 == 1u;
                uint _654;
                uint _656;
                float _658;
                float _660;
                float _662;
                float _664;
                float _666;
                float _668;
                uint _670;
                uint _672;
                float _674;
                float _676;
                float _678;
                if (_623 && (asuint(_49_m0[62u]).w != 0u))
                {
                    _654 = 0u;
                    _656 = 1u;
                    _658 = 0.0f;
                    _660 = 0.0f;
                    _662 = 1.0f;
                    _664 = 0.0f;
                    _666 = 0.0f;
                    _668 = 0.0f;
                    _670 = 0u;
                    _672 = 0u;
                    _674 = 0.0f;
                    _676 = 0.0f;
                    _678 = 0.0f;
                }
                else
                {
                    float _739 = float(_237);
                    float _740 = float(_238);
                    float _747 = (_613 != 0.0f) ? (0.100000001490116119384765625f / _609) : 3.4028234663852885981170418348452e+38f;
                    float _749 = (_615 != 0.0f) ? (0.100000001490116119384765625f / _611) : 3.4028234663852885981170418348452e+38f;
                    float _750 = (_616 != 0.0f) ? (0.100000001490116119384765625f / _612) : 3.4028234663852885981170418348452e+38f;
                    float _751 = 1.0f / _739;
                    float _752 = 1.0f / _740;
                    float _753 = 0.004999999888241291046142578125f / _739;
                    float _755 = 0.004999999888241291046142578125f / _740;
                    float _764 = float(_613 >= 0.0f);
                    float _765 = float(_615 >= 0.0f);
                    float _774 = ((_613 < 0.0f) ? ((-0.0f) - _753) : _753) - _576;
                    float _777 = ((_615 < 0.0f) ? ((-0.0f) - _755) : _755) - _577;
                    float _780 = min((((floor(_576 * _739) + _764) * _751) + _774) * _747, (((floor(_577 * _740) + _765) * _752) + _777) * _749);
                    float _784 = (_780 * _613) + _576;
                    float _785 = (_780 * _615) + _577;
                    float _786 = (_780 * _616) + _569;
                    float _789 = _49_m0[50u].x / (_452 - (_786 * _49_m0[50u].y));
                    float _799 = 1.0f / max(0.00999999977648258209228515625f, max(abs(_609 * 38400.0f), abs(_611 * 21600.0f)));
                    float _807 = max(_49_m0[3u].z, _49_m0[5u].z * _799);
                    float _950;
                    if (asuint(_49_m0[5u]).y == 0u)
                    {
                        _950 = _807;
                    }
                    else
                    {
                        _950 = min(_807, _49_m0[5u].w * _799);
                    }
                    uint _657;
                    float _659;
                    float _661;
                    float _663;
                    float _665;
                    float _667;
                    float _669;
                    float _675;
                    float _677;
                    float _679;
                    uint _1056;
                    uint _1061;
                    if (_208 == 0u)
                    {
                        _1056 = 0u;
                        _679 = _786;
                        _677 = _785;
                        _675 = _784;
                        _1061 = 0u;
                        _669 = _786;
                        _667 = _785;
                        _665 = _784;
                        _663 = 1.0f;
                        _661 = 0.0f;
                        _659 = 0.0f;
                        _657 = 1u;
                    }
                    else
                    {
                        uint _1057;
                        float _1058;
                        float _1059;
                        float _1060;
                        uint _1062;
                        uint _1172;
                        float _1063;
                        float _1064;
                        float _1065;
                        float _1066;
                        float _1067;
                        float _1068;
                        uint _1069;
                        float _1157;
                        float _1162;
                        float _1164;
                        float _1166;
                        float _1168;
                        float _1170;
                        uint _1155 = 0u;
                        float _1156 = _950;
                        float _1158 = _786;
                        float _1159 = _785;
                        float _1160 = _784;
                        float _1161 = _780;
                        float _1163 = _752;
                        float _1165 = _751;
                        float _1167 = _740;
                        float _1169 = _739;
                        uint _1171 = 0u;
                        uint _1173 = 0u;
                        float _1174 = _786;
                        float _1175 = _785;
                        float _1176 = _784;
                        float _1177 = 1.0f;
                        float _1178 = 0.0f;
                        float _1179 = 0.0f;
                        uint _1180 = 1u;
                        float _1181;
                        float _1182;
                        uint _1183;
                        uint _1184;
                        bool _1185;
                        for (;;)
                        {
                            _1181 = _1169 * _1160;
                            _1182 = _1167 * _1159;
                            _1183 = uint(int(_1181));
                            _1184 = uint(int(_1182));
                            _1185 = _1171 == 0u;
                            float _1297;
                            if (_1185)
                            {
                                _1297 = _12.Load(int3(uint2(_1183, _1184), 0u)).x;
                            }
                            else
                            {
                                _1297 = _15.Load(int3(uint2(_1183, _1184), _1171 + 4294967295u)).x;
                            }
                            float _1303 = ((_1181 >= floor(_1169)) || (_1182 >= floor(_1167))) ? 1.0f : _1297;
                            float _1317 = (_616 < 0.0f) ? ((_1303 - _569) * _750) : 3.4028234663852885981170418348452e+38f;
                            float _1319 = min(min((((floor(_1181) + _764) * _1165) + _774) * _747, (((floor(_1182) + _765) * _1163) + _777) * _749), _1317);
                            bool _1320 = _1303 < _1158;
                            bool _1324 = _1320 && (asuint(_1319) != asuint(_1317));
                            float _1325 = _1320 ? _1319 : _1161;
                            float _1329 = (_1325 * _613) + _576;
                            float _1330 = (_1325 * _615) + _577;
                            float _1331 = (_1325 * _616) + _569;
                            uint _1333 = (_1324 ? 1u : 4294967295u) + _1171;
                            float _1334 = _1324 ? 0.5f : 2.0f;
                            float _1335 = _1334 * _1169;
                            float _1336 = _1334 * _1167;
                            float _1337 = _1324 ? 2.0f : 0.5f;
                            float _1338 = _1337 * _1165;
                            float _1339 = _1337 * _1163;
                            _1057 = _1155 + 1u;
                            uint _1754;
                            uint _1756;
                            if (int(_1333) < int(0u))
                            {
                                float _1561 = _49_m0[50u].w + _49_m0[50u].y;
                                float _1563 = _49_m0[50u].x / (_1561 - (_49_m0[50u].y * _1303));
                                float _1566 = _49_m0[50u].x / (_1561 - (_49_m0[50u].y * _1331));
                                float _1571 = abs(_789 - _1566);
                                float _1574 = _1566 - _1563;
                                uint frontier_phi_104_86_ladder;
                                float frontier_phi_104_86_ladder_1;
                                float frontier_phi_104_86_ladder_2;
                                float frontier_phi_104_86_ladder_3;
                                float frontier_phi_104_86_ladder_4;
                                float frontier_phi_104_86_ladder_5;
                                float frontier_phi_104_86_ladder_6;
                                uint frontier_phi_104_86_ladder_7;
                                float frontier_phi_104_86_ladder_8;
                                float frontier_phi_104_86_ladder_9;
                                float frontier_phi_104_86_ladder_10;
                                float frontier_phi_104_86_ladder_11;
                                float frontier_phi_104_86_ladder_12;
                                float frontier_phi_104_86_ladder_13;
                                uint frontier_phi_104_86_ladder_14;
                                if (_1574 > max(_49_m0[3u].x, _49_m0[3u].x * _1571))
                                {
                                    float _1725 = _1325 + _1156;
                                    bool _1726 = _1173 != 0u;
                                    float _1727 = _1726 ? _1179 : _1329;
                                    float _1728 = _1726 ? _1178 : _1330;
                                    uint _1735 = (_623 || (asuint(_49_m0[62u]).z == 0u)) ? 1u : _1173;
                                    uint frontier_phi_104_86_ladder_102_ladder;
                                    float frontier_phi_104_86_ladder_102_ladder_1;
                                    float frontier_phi_104_86_ladder_102_ladder_2;
                                    float frontier_phi_104_86_ladder_102_ladder_3;
                                    float frontier_phi_104_86_ladder_102_ladder_4;
                                    float frontier_phi_104_86_ladder_102_ladder_5;
                                    float frontier_phi_104_86_ladder_102_ladder_6;
                                    uint frontier_phi_104_86_ladder_102_ladder_7;
                                    float frontier_phi_104_86_ladder_102_ladder_8;
                                    float frontier_phi_104_86_ladder_102_ladder_9;
                                    float frontier_phi_104_86_ladder_102_ladder_10;
                                    float frontier_phi_104_86_ladder_102_ladder_11;
                                    float frontier_phi_104_86_ladder_102_ladder_12;
                                    float frontier_phi_104_86_ladder_102_ladder_13;
                                    uint frontier_phi_104_86_ladder_102_ladder_14;
                                    if (asuint(_49_m0[5u]).y == 0u)
                                    {
                                        frontier_phi_104_86_ladder_102_ladder = 0u;
                                        frontier_phi_104_86_ladder_102_ladder_1 = _739;
                                        frontier_phi_104_86_ladder_102_ladder_2 = _740;
                                        frontier_phi_104_86_ladder_102_ladder_3 = _751;
                                        frontier_phi_104_86_ladder_102_ladder_4 = _752;
                                        frontier_phi_104_86_ladder_102_ladder_5 = _1725;
                                        frontier_phi_104_86_ladder_102_ladder_6 = _1156;
                                        frontier_phi_104_86_ladder_102_ladder_7 = _1735;
                                        frontier_phi_104_86_ladder_102_ladder_8 = _1174;
                                        frontier_phi_104_86_ladder_102_ladder_9 = _1175;
                                        frontier_phi_104_86_ladder_102_ladder_10 = _1176;
                                        frontier_phi_104_86_ladder_102_ladder_11 = _1177;
                                        frontier_phi_104_86_ladder_102_ladder_12 = _1728;
                                        frontier_phi_104_86_ladder_102_ladder_13 = _1727;
                                        frontier_phi_104_86_ladder_102_ladder_14 = _1180;
                                    }
                                    else
                                    {
                                        frontier_phi_104_86_ladder_102_ladder = 0u;
                                        frontier_phi_104_86_ladder_102_ladder_1 = _739;
                                        frontier_phi_104_86_ladder_102_ladder_2 = _740;
                                        frontier_phi_104_86_ladder_102_ladder_3 = _751;
                                        frontier_phi_104_86_ladder_102_ladder_4 = _752;
                                        frontier_phi_104_86_ladder_102_ladder_5 = _1725;
                                        frontier_phi_104_86_ladder_102_ladder_6 = min(_807, _49_m0[6u].x * _1156);
                                        frontier_phi_104_86_ladder_102_ladder_7 = _1735;
                                        frontier_phi_104_86_ladder_102_ladder_8 = _1174;
                                        frontier_phi_104_86_ladder_102_ladder_9 = _1175;
                                        frontier_phi_104_86_ladder_102_ladder_10 = _1176;
                                        frontier_phi_104_86_ladder_102_ladder_11 = _1177;
                                        frontier_phi_104_86_ladder_102_ladder_12 = _1728;
                                        frontier_phi_104_86_ladder_102_ladder_13 = _1727;
                                        frontier_phi_104_86_ladder_102_ladder_14 = _1180;
                                    }
                                    frontier_phi_104_86_ladder = frontier_phi_104_86_ladder_102_ladder;
                                    frontier_phi_104_86_ladder_1 = frontier_phi_104_86_ladder_102_ladder_1;
                                    frontier_phi_104_86_ladder_2 = frontier_phi_104_86_ladder_102_ladder_2;
                                    frontier_phi_104_86_ladder_3 = frontier_phi_104_86_ladder_102_ladder_3;
                                    frontier_phi_104_86_ladder_4 = frontier_phi_104_86_ladder_102_ladder_4;
                                    frontier_phi_104_86_ladder_5 = frontier_phi_104_86_ladder_102_ladder_5;
                                    frontier_phi_104_86_ladder_6 = frontier_phi_104_86_ladder_102_ladder_6;
                                    frontier_phi_104_86_ladder_7 = frontier_phi_104_86_ladder_102_ladder_7;
                                    frontier_phi_104_86_ladder_8 = frontier_phi_104_86_ladder_102_ladder_8;
                                    frontier_phi_104_86_ladder_9 = frontier_phi_104_86_ladder_102_ladder_9;
                                    frontier_phi_104_86_ladder_10 = frontier_phi_104_86_ladder_102_ladder_10;
                                    frontier_phi_104_86_ladder_11 = frontier_phi_104_86_ladder_102_ladder_11;
                                    frontier_phi_104_86_ladder_12 = frontier_phi_104_86_ladder_102_ladder_12;
                                    frontier_phi_104_86_ladder_13 = frontier_phi_104_86_ladder_102_ladder_13;
                                    frontier_phi_104_86_ladder_14 = frontier_phi_104_86_ladder_102_ladder_14;
                                }
                                else
                                {
                                    float _1743 = max(_49_m0[3u].y, _49_m0[3u].y * _1571);
                                    float _1746 = _1743 * _49_m0[3u].w;
                                    float _1750 = clamp((abs(_1574) - _1746) / (_1743 - _1746), 0.0f, 1.0f);
                                    uint _1752 = uint(_1563 < _789);
                                    uint frontier_phi_104_86_ladder_103_ladder;
                                    float frontier_phi_104_86_ladder_103_ladder_1;
                                    float frontier_phi_104_86_ladder_103_ladder_2;
                                    float frontier_phi_104_86_ladder_103_ladder_3;
                                    float frontier_phi_104_86_ladder_103_ladder_4;
                                    float frontier_phi_104_86_ladder_103_ladder_5;
                                    float frontier_phi_104_86_ladder_103_ladder_6;
                                    uint frontier_phi_104_86_ladder_103_ladder_7;
                                    float frontier_phi_104_86_ladder_103_ladder_8;
                                    float frontier_phi_104_86_ladder_103_ladder_9;
                                    float frontier_phi_104_86_ladder_103_ladder_10;
                                    float frontier_phi_104_86_ladder_103_ladder_11;
                                    float frontier_phi_104_86_ladder_103_ladder_12;
                                    float frontier_phi_104_86_ladder_103_ladder_13;
                                    uint frontier_phi_104_86_ladder_103_ladder_14;
                                    if (_1173 == 0u)
                                    {
                                        frontier_phi_104_86_ladder_103_ladder = _1333;
                                        frontier_phi_104_86_ladder_103_ladder_1 = _1335;
                                        frontier_phi_104_86_ladder_103_ladder_2 = _1336;
                                        frontier_phi_104_86_ladder_103_ladder_3 = _1338;
                                        frontier_phi_104_86_ladder_103_ladder_4 = _1339;
                                        frontier_phi_104_86_ladder_103_ladder_5 = _1325;
                                        frontier_phi_104_86_ladder_103_ladder_6 = _1156;
                                        frontier_phi_104_86_ladder_103_ladder_7 = uint(_1750 > 0.0f);
                                        frontier_phi_104_86_ladder_103_ladder_8 = _1174;
                                        frontier_phi_104_86_ladder_103_ladder_9 = _1175;
                                        frontier_phi_104_86_ladder_103_ladder_10 = _1176;
                                        frontier_phi_104_86_ladder_103_ladder_11 = _1750;
                                        frontier_phi_104_86_ladder_103_ladder_12 = _1178;
                                        frontier_phi_104_86_ladder_103_ladder_13 = _1179;
                                        frontier_phi_104_86_ladder_103_ladder_14 = _1752;
                                    }
                                    else
                                    {
                                        frontier_phi_104_86_ladder_103_ladder = _1333;
                                        frontier_phi_104_86_ladder_103_ladder_1 = _1335;
                                        frontier_phi_104_86_ladder_103_ladder_2 = _1336;
                                        frontier_phi_104_86_ladder_103_ladder_3 = _1338;
                                        frontier_phi_104_86_ladder_103_ladder_4 = _1339;
                                        frontier_phi_104_86_ladder_103_ladder_5 = _1325;
                                        frontier_phi_104_86_ladder_103_ladder_6 = _1156;
                                        frontier_phi_104_86_ladder_103_ladder_7 = _1173;
                                        frontier_phi_104_86_ladder_103_ladder_8 = _1174;
                                        frontier_phi_104_86_ladder_103_ladder_9 = _1175;
                                        frontier_phi_104_86_ladder_103_ladder_10 = _1176;
                                        frontier_phi_104_86_ladder_103_ladder_11 = _1750;
                                        frontier_phi_104_86_ladder_103_ladder_12 = _1178;
                                        frontier_phi_104_86_ladder_103_ladder_13 = _1179;
                                        frontier_phi_104_86_ladder_103_ladder_14 = _1752;
                                    }
                                    frontier_phi_104_86_ladder = frontier_phi_104_86_ladder_103_ladder;
                                    frontier_phi_104_86_ladder_1 = frontier_phi_104_86_ladder_103_ladder_1;
                                    frontier_phi_104_86_ladder_2 = frontier_phi_104_86_ladder_103_ladder_2;
                                    frontier_phi_104_86_ladder_3 = frontier_phi_104_86_ladder_103_ladder_3;
                                    frontier_phi_104_86_ladder_4 = frontier_phi_104_86_ladder_103_ladder_4;
                                    frontier_phi_104_86_ladder_5 = frontier_phi_104_86_ladder_103_ladder_5;
                                    frontier_phi_104_86_ladder_6 = frontier_phi_104_86_ladder_103_ladder_6;
                                    frontier_phi_104_86_ladder_7 = frontier_phi_104_86_ladder_103_ladder_7;
                                    frontier_phi_104_86_ladder_8 = frontier_phi_104_86_ladder_103_ladder_8;
                                    frontier_phi_104_86_ladder_9 = frontier_phi_104_86_ladder_103_ladder_9;
                                    frontier_phi_104_86_ladder_10 = frontier_phi_104_86_ladder_103_ladder_10;
                                    frontier_phi_104_86_ladder_11 = frontier_phi_104_86_ladder_103_ladder_11;
                                    frontier_phi_104_86_ladder_12 = frontier_phi_104_86_ladder_103_ladder_12;
                                    frontier_phi_104_86_ladder_13 = frontier_phi_104_86_ladder_103_ladder_13;
                                    frontier_phi_104_86_ladder_14 = frontier_phi_104_86_ladder_103_ladder_14;
                                }
                                _1069 = frontier_phi_104_86_ladder_14;
                                _1068 = frontier_phi_104_86_ladder_13;
                                _1067 = frontier_phi_104_86_ladder_12;
                                _1066 = frontier_phi_104_86_ladder_11;
                                _1065 = frontier_phi_104_86_ladder_10;
                                _1064 = frontier_phi_104_86_ladder_9;
                                _1063 = frontier_phi_104_86_ladder_8;
                                _1754 = frontier_phi_104_86_ladder_7;
                                _1756 = frontier_phi_104_86_ladder;
                                _1170 = frontier_phi_104_86_ladder_1;
                                _1168 = frontier_phi_104_86_ladder_2;
                                _1166 = frontier_phi_104_86_ladder_3;
                                _1164 = frontier_phi_104_86_ladder_4;
                                _1162 = frontier_phi_104_86_ladder_5;
                                _1157 = frontier_phi_104_86_ladder_6;
                            }
                            else
                            {
                                bool _1576 = _1173 != 0u;
                                _1069 = _1180;
                                _1068 = _1179;
                                _1067 = _1178;
                                _1066 = _1177;
                                _1065 = _1576 ? _1176 : _1329;
                                _1064 = _1576 ? _1175 : _1330;
                                _1063 = _1576 ? _1174 : _1331;
                                _1754 = _1173;
                                _1756 = _1333;
                                _1170 = _1335;
                                _1168 = _1336;
                                _1166 = _1338;
                                _1164 = _1339;
                                _1162 = _1325;
                                _1157 = (asuint(_49_m0[5u]).y != 0u) ? _950 : _1156;
                            }
                            float frontier_phi_129_pred;
                            uint frontier_phi_129_pred_1;
                            uint frontier_phi_129_pred_2;
                            float frontier_phi_129_pred_3;
                            float frontier_phi_129_pred_4;
                            bool _1760;
                            bool _1762;
                            for (;;)
                            {
                                _1760 = _1331 < 0.0f;
                                _1762 = _1760 || ((_1329 < 0.0f) || (_1330 < 0.0f));
                                if (!_1762)
                                {
                                    if (!((_1331 > 1.0f) || ((_1329 > _49_m0[51u].x) || (_1330 > _49_m0[51u].y))))
                                    {
                                        frontier_phi_129_pred = _1330;
                                        frontier_phi_129_pred_1 = _1754;
                                        frontier_phi_129_pred_2 = _1756;
                                        frontier_phi_129_pred_3 = _1329;
                                        frontier_phi_129_pred_4 = _1331;
                                        break;
                                    }
                                }
                                if (!_1760)
                                {
                                    frontier_phi_129_pred = _1330;
                                    frontier_phi_129_pred_1 = 1u;
                                    frontier_phi_129_pred_2 = 4294967295u;
                                    frontier_phi_129_pred_3 = _1329;
                                    frontier_phi_129_pred_4 = _1331;
                                    break;
                                }
                                float _1961 = (-0.0f) - _1331;
                                float _1962 = _1961 / _616;
                                frontier_phi_129_pred = (_1962 * _615) + _1330;
                                frontier_phi_129_pred_1 = 1u;
                                frontier_phi_129_pred_2 = 4294967295u;
                                frontier_phi_129_pred_3 = (_1962 * _613) + _1329;
                                frontier_phi_129_pred_4 = _1961 + _1331;
                                break;
                            }
                            _1059 = frontier_phi_129_pred;
                            _1062 = frontier_phi_129_pred_1;
                            _1172 = frontier_phi_129_pred_2;
                            _1060 = frontier_phi_129_pred_3;
                            _1058 = frontier_phi_129_pred_4;
                            if ((_1057 < _208) && (int(_1172) > int(4294967295u)))
                            {
                                _1155 = _1057;
                                _1156 = _1157;
                                _1158 = _1058;
                                _1159 = _1059;
                                _1160 = _1060;
                                _1161 = _1162;
                                _1163 = _1164;
                                _1165 = _1166;
                                _1167 = _1168;
                                _1169 = _1170;
                                _1171 = _1172;
                                _1173 = _1062;
                                _1174 = _1063;
                                _1175 = _1064;
                                _1176 = _1065;
                                _1177 = _1066;
                                _1178 = _1067;
                                _1179 = _1068;
                                _1180 = _1069;
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        _1056 = _1057;
                        _679 = _1058;
                        _677 = _1059;
                        _675 = _1060;
                        _1061 = _1062;
                        _669 = _1063;
                        _667 = _1064;
                        _665 = _1065;
                        _663 = _1066;
                        _661 = _1067;
                        _659 = _1068;
                        _657 = _1069;
                    }
                    bool _1070 = _1056 >= _208;
                    _654 = uint(_1070);
                    _656 = _657;
                    _658 = _659;
                    _660 = _661;
                    _662 = _663;
                    _664 = _665;
                    _666 = _667;
                    _668 = _669;
                    _670 = _1070 ? 1u : _1061;
                    _672 = uint(_1056 <= _208);
                    _674 = _675;
                    _676 = _677;
                    _678 = _679;
                }
                float _686 = _49_m0[51u].z * 2.0f;
                float _689 = (_686 * _576) + (-1.0f);
                float _690 = ((1.0f - (_49_m0[51u].w * _577)) * 2.0f) + (-1.0f);
                float _706 = mad(_189, _569, mad(_182, _690, _689 * _175)) + _196;
                float _707 = (mad(_186, _569, mad(_179, _690, _689 * _172)) + _193) / _706;
                float _708 = (mad(_187, _569, mad(_180, _690, _689 * _173)) + _194) / _706;
                float _709 = (mad(_188, _569, mad(_181, _690, _689 * _174)) + _195) / _706;
                float _714 = (_686 * _674) + (-1.0f);
                float _715 = ((1.0f - (_49_m0[51u].w * _676)) * 2.0f) + (-1.0f);
                float _731 = mad(_189, _678, mad(_182, _715, _714 * _175)) + _196;
                float _735 = ((mad(_186, _678, mad(_179, _715, _714 * _172)) + _193) / _731) - _707;
                float _736 = ((mad(_187, _678, mad(_180, _715, _714 * _173)) + _194) / _731) - _708;
                float _737 = ((mad(_188, _678, mad(_181, _715, _714 * _174)) + _195) / _731) - _709;
                float _898;
                uint _900;
                float _902;
                if (_672 == 0u)
                {
                    _898 = 0.0f;
                    _900 = _670;
                    _902 = 0.0f;
                }
                else
                {
                    float _945 = float(_237);
                    float _946 = float(_238);
                    float frontier_phi_25_26_ladder;
                    uint frontier_phi_25_26_ladder_1;
                    float frontier_phi_25_26_ladder_2;
                    if ((_674 < 0.0f) || (_676 < 0.0f))
                    {
                        frontier_phi_25_26_ladder = 0.0f;
                        frontier_phi_25_26_ladder_1 = _670;
                        frontier_phi_25_26_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_25_26_ladder_34_ladder;
                        uint frontier_phi_25_26_ladder_34_ladder_1;
                        float frontier_phi_25_26_ladder_34_ladder_2;
                        if ((_678 >= 1.0f) || ((_674 > _49_m0[51u].x) || (_676 > _49_m0[51u].y)))
                        {
                            frontier_phi_25_26_ladder_34_ladder = 0.0f;
                            frontier_phi_25_26_ladder_34_ladder_1 = _670;
                            frontier_phi_25_26_ladder_34_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_25_26_ladder_34_ladder_43_ladder;
                            uint frontier_phi_25_26_ladder_34_ladder_43_ladder_1;
                            float frontier_phi_25_26_ladder_34_ladder_43_ladder_2;
                            for (;;)
                            {
                                if ((abs(_674 - _258) < (2.0f / _945)) && (abs(_676 - _259) < (2.0f / _946)))
                                {
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder = 0.0f;
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder_1 = _670;
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_85;
                                    float frontier_phi_85_pred;
                                    uint frontier_phi_85_pred_1;
                                    float frontier_phi_85_pred_2;
                                    uint _1217;
                                    uint _1218;
                                    bool _1219;
                                    for (;;)
                                    {
                                        _1217 = uint(int(_674 * _945));
                                        _1218 = uint(int(_676 * _946));
                                        _1219 = _623 && _530;
                                        if (!_1219)
                                        {
                                            if (!(dot(float3(_735, _736, _737), float3(_735, _736, _737)) < _49_m0[4u].w))
                                            {
                                                ladder_phi_85 = false;
                                                frontier_phi_85_pred = 0.0f;
                                                frontier_phi_85_pred_1 = _670;
                                                frontier_phi_85_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1286 = _24[22u].Load(int3(uint2(_1217, _1218), 0u));
                                        uint _1288 = _1286.x;
                                        float _1711;
                                        float _1712;
                                        float _1713;
                                        if (_1288 == 0u)
                                        {
                                            uint4 _1460 = _24[1u].Load(int3(uint2(_1217, _1218), 0u));
                                            uint _1462 = _1460.x;
                                            float _1470 = (float((_1462 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1471 = (float(_1462 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1475 = (1.0f - abs(_1470)) - abs(_1471);
                                            float _1477 = clamp((-0.0f) - _1475, 0.0f, 1.0f);
                                            float _1478 = (-0.0f) - _1477;
                                            _1711 = ((_1470 >= 0.0f) ? _1478 : _1477) + _1470;
                                            _1712 = ((_1471 >= 0.0f) ? _1478 : _1477) + _1471;
                                            _1713 = _1475;
                                        }
                                        else
                                        {
                                            float _1492 = (float((_1288 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1493 = (float(_1288 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1497 = (1.0f - abs(_1492)) - abs(_1493);
                                            float _1499 = clamp((-0.0f) - _1497, 0.0f, 1.0f);
                                            float _1500 = (-0.0f) - _1499;
                                            _1711 = ((_1492 >= 0.0f) ? _1500 : _1499) + _1492;
                                            _1712 = ((_1493 >= 0.0f) ? _1500 : _1499) + _1493;
                                            _1713 = _1497;
                                        }
                                        float _1717 = rsqrt(dot(float3(_1711, _1712, _1713), float3(_1711, _1712, _1713)));
                                        if (dot(float3(_1717 * _1711, _1717 * _1712, _1717 * _1713), float3(_735, _736, _737)) > 0.0f)
                                        {
                                            ladder_phi_85 = true;
                                            frontier_phi_85_pred = 0.0f;
                                            frontier_phi_85_pred_1 = _670;
                                            frontier_phi_85_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_85 = false;
                                            frontier_phi_85_pred = 0.0f;
                                            frontier_phi_85_pred_1 = _670;
                                            frontier_phi_85_pred_2 = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_85)
                                    {
                                        frontier_phi_25_26_ladder_34_ladder_43_ladder = frontier_phi_85_pred;
                                        frontier_phi_25_26_ladder_34_ladder_43_ladder_1 = frontier_phi_85_pred_1;
                                        frontier_phi_25_26_ladder_34_ladder_43_ladder_2 = frontier_phi_85_pred_2;
                                        break;
                                    }
                                    float _1511 = _49_m0[51u].z * _674;
                                    float _1512 = _49_m0[51u].w * _676;
                                    float _1514 = (_946 / _945) * 0.0500000007450580596923828125f;
                                    float _1519 = clamp(_1511 / _1514, 0.0f, 1.0f);
                                    float _1520 = clamp(_1512 * 20.0f, 0.0f, 1.0f);
                                    float _1532 = clamp(((_1511 + (-1.0f)) + _1514) / _1514, 0.0f, 1.0f);
                                    float _1533 = clamp((_1512 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1544 = _1519 * _1520;
                                    precise float _1545 = _1544 * _1544;
                                    float _1549 = ((((3.0f - (_1520 * 2.0f)) * (3.0f - (_1519 * 2.0f))) * _1545) * (1.0f - ((_1532 * _1532) * (3.0f - (_1532 * 2.0f))))) * (1.0f - ((_1533 * _1533) * (3.0f - (_1533 * 2.0f))));
                                    bool _1552 = (_670 != 0u) || (_1549 >= 1.0f);
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder = _1552 ? 0.0f : _1549;
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder_1 = _1552 ? _670 : 1u;
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder_2 = _1549 * float(_469 > 0.0f);
                                    break;
                                }
                            }
                            frontier_phi_25_26_ladder_34_ladder = frontier_phi_25_26_ladder_34_ladder_43_ladder;
                            frontier_phi_25_26_ladder_34_ladder_1 = frontier_phi_25_26_ladder_34_ladder_43_ladder_1;
                            frontier_phi_25_26_ladder_34_ladder_2 = frontier_phi_25_26_ladder_34_ladder_43_ladder_2;
                        }
                        frontier_phi_25_26_ladder = frontier_phi_25_26_ladder_34_ladder;
                        frontier_phi_25_26_ladder_1 = frontier_phi_25_26_ladder_34_ladder_1;
                        frontier_phi_25_26_ladder_2 = frontier_phi_25_26_ladder_34_ladder_2;
                    }
                    _898 = frontier_phi_25_26_ladder;
                    _900 = frontier_phi_25_26_ladder_1;
                    _902 = frontier_phi_25_26_ladder_2;
                }
                uint _1031;
                uint _1033;
                float _943;
                for (;;)
                {
                    _943 = ((((exp2(log2(clamp((sqrt(((_708 * _708) + (_707 * _707)) + (_709 * _709)) - _44_m0[121u].y) * _44_m0[121u].z, 0.0f, 1.0f)) * _44_m0[121u].w) * _409) * exp2(log2(clamp((_708 - _44_m0[122u].x) * _44_m0[122u].y, 0.0f, 1.0f)) * _44_m0[122u].z)) * (1.0f - clamp(_397, 0.0f, 1.0f))) * (max(_44_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    if (_902 > 0.0f)
                    {
                        uint _993 = uint((_201 * 0.999989986419677734375f) * clamp(_674, 0.0f, 1.0f));
                        uint _994 = uint((_203 * 0.999989986419677734375f) * clamp(_676, 0.0f, 1.0f));
                        uint4 _997 = _24[2u].Load(int3(uint2(_993, _994), 0u));
                        uint _1000 = _997.w;
                        uint4 _1005 = _24[15u].Load(int3(uint2(_993, _994), 0u));
                        uint _1007 = _1005.y;
                        uint _1013 = ((_1007 & 64u) != 0u) ? uint((_1007 & 4294967167u) != 66u) : 4294967295u;
                        uint _1014 = _1000 & 128u;
                        uint _1016 = (_1014 != 0u) ? 1u : ((_997.x << 7u) | _1000);
                        uint4 _1019 = _16.Load(_1016 * 4u);
                        uint _1020 = _1019.x;
                        uint _1027 = ((_1020 & 1u) != 0u) ? 0u : 18u;
                        uint _1029 = uint(min(int(uint(max(int(_1013), int(0u)))), int(1u)));
                        uint _1118;
                        if (_1014 == 0u)
                        {
                            _1118 = (((_1020 & 2097152u) != 0u) && (_1013 == _1029)) ? (_1027 | 128u) : _1027;
                        }
                        else
                        {
                            _1118 = _1000;
                        }
                        uint _1119 = _16.Load((_1016 * 4u) + 1u).x & 512u;
                        bool _1122 = (_1118 & 144u) == 0u;
                        if (_1119 == 0u)
                        {
                            if (_1122 || ((_1020 & 1u) != 0u))
                            {
                                _1031 = 0u;
                                _1033 = 0u;
                                break;
                            }
                        }
                        else
                        {
                            if (_1122)
                            {
                                _1031 = 0u;
                                _1033 = 0u;
                                break;
                            }
                        }
                        bool _1245 = ((_1118 & 128u) | _1119) != 0u;
                        uint _1032;
                        if (_1245)
                        {
                            _1032 = 1u;
                        }
                        else
                        {
                            _1032 = (((_1020 >> 14u) & 2u) ^ 2u) + 3u;
                        }
                        if (((_1020 & 268435472u) == 16u) && (((_1020 & 33554432u) == 0u) || _1245))
                        {
                            _1031 = 2u;
                            _1033 = 0u;
                            break;
                        }
                        _1031 = _1032;
                        _1033 = (_1032 == 1u) ? _1029 : 0u;
                        break;
                    }
                    else
                    {
                        _1031 = 0u;
                        _1033 = 0u;
                        break;
                    }
                }
                uint _1127;
                float _1128;
                float _1130;
                float _1132;
                float _1134;
                float _1135;
                float _1136;
                float _1138;
                float _1037;
                float _1040;
                float _1043;
                float _1047;
                float _1048;
                for (;;)
                {
                    _1037 = mad(_166, _526, mad(_160, _525, _524 * _154));
                    _1040 = mad(_167, _526, mad(_161, _525, _524 * _155));
                    _1043 = mad(_168, _526, mad(_162, _525, _524 * _156));
                    bool _1046 = (_900 != 0u) || (_902 < 1.0f);
                    _1047 = _1046 ? 0.0f : 1.0f;
                    _1048 = _1046 ? 0.0f : 0.5f;
                    if (_1046)
                    {
                        uint4 _1126 = asuint(_49_m0[60u]);
                        if (_623)
                        {
                            if (int(_407) < int(1u))
                            {
                                if (_1126.x == 0u)
                                {
                                    _1127 = 0u;
                                    _1128 = 0.0f;
                                    _1130 = 0.0f;
                                    _1132 = 0.0f;
                                    _1134 = 0.0f;
                                    _1135 = 0.0f;
                                    _1136 = 0.0f;
                                    _1138 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1126.y == 0u)
                                {
                                    _1127 = 0u;
                                    _1128 = 0.0f;
                                    _1130 = 0.0f;
                                    _1132 = 0.0f;
                                    _1134 = 0.0f;
                                    _1135 = 0.0f;
                                    _1136 = 0.0f;
                                    _1138 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1126.z == 0u)
                            {
                                _1127 = 0u;
                                _1128 = 0.0f;
                                _1130 = 0.0f;
                                _1132 = 0.0f;
                                _1134 = 0.0f;
                                _1135 = 0.0f;
                                _1136 = 0.0f;
                                _1138 = 0.0f;
                                break;
                            }
                        }
                        if (_898 > 0.0f)
                        {
                            _1127 = 0u;
                            _1128 = 0.0f;
                            _1130 = 0.0f;
                            _1132 = 0.0f;
                            _1134 = 0.0f;
                            _1135 = 1.0f;
                            _1136 = 1000.0f;
                            _1138 = 0.5f;
                            break;
                        }
                        if (((_664 <= 0.0f) || (_666 <= 0.0f)) || (_668 <= 0.0f))
                        {
                            _1127 = 0u;
                            _1128 = 0.0f;
                            _1130 = 0.0f;
                            _1132 = 0.0f;
                            _1134 = 0.0f;
                            _1135 = 1.0f;
                            _1136 = 1000.0f;
                            _1138 = 0.5f;
                            break;
                        }
                        if ((_668 >= 1.0f) || ((_664 >= _49_m0[51u].x) || (_666 >= _49_m0[51u].y)))
                        {
                            _1127 = 0u;
                            _1128 = 0.0f;
                            _1130 = 0.0f;
                            _1132 = 0.0f;
                            _1134 = 0.0f;
                            _1135 = 1.0f;
                            _1136 = 1000.0f;
                            _1138 = 0.5f;
                            break;
                        }
                        uint _2006;
                        uint _2008;
                        uint _1774;
                        uint _1775;
                        bool _1781;
                        for (;;)
                        {
                            _1774 = uint(clamp(_658, 0.0f, 1.0f) * _201);
                            _1775 = uint(clamp(_660, 0.0f, 1.0f) * _203);
                            _1781 = _20[21u].Load(int3(uint2(_1774, _1775), 0u)).x > 0.0f;
                            if (_1781)
                            {
                                uint _1885 = _24[23u].Load(int3(uint2(_1774, _1775), 0u)).y + 4294967295u;
                                _2006 = (uint(int(_1885) >> int(31u)) & 3u) + 1u;
                                _2008 = (int(_1885) < int(0u)) ? 0u : _1885;
                                break;
                            }
                            else
                            {
                                uint4 _1893 = _24[2u].Load(int3(uint2(_1774, _1775), 0u));
                                uint _1896 = _1893.w;
                                uint4 _1901 = _24[15u].Load(int3(uint2(_1774, _1775), 0u));
                                uint _1903 = _1901.y;
                                uint _1909 = ((_1903 & 64u) != 0u) ? uint((_1903 & 4294967167u) != 66u) : 4294967295u;
                                uint _1910 = _1896 & 128u;
                                uint _1912 = (_1910 != 0u) ? 1u : ((_1893.x << 7u) | _1896);
                                uint4 _1915 = _16.Load(_1912 * 4u);
                                uint _1916 = _1915.x;
                                uint _1923 = ((_1916 & 1u) != 0u) ? 0u : 18u;
                                uint _1925 = uint(min(int(uint(max(int(_1909), int(0u)))), int(1u)));
                                uint _2019;
                                if (_1910 == 0u)
                                {
                                    _2019 = (((_1916 & 2097152u) != 0u) && (_1909 == _1925)) ? (_1923 | 128u) : _1923;
                                }
                                else
                                {
                                    _2019 = _1896;
                                }
                                uint _2020 = _16.Load((_1912 * 4u) + 1u).x & 512u;
                                bool _2023 = (_2019 & 144u) == 0u;
                                if (_2020 == 0u)
                                {
                                    if (_2023 || ((_1916 & 1u) != 0u))
                                    {
                                        _2006 = 0u;
                                        _2008 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2023)
                                    {
                                        _2006 = 0u;
                                        _2008 = 0u;
                                        break;
                                    }
                                }
                                bool _2115 = ((_2019 & 128u) | _2020) != 0u;
                                uint _2007;
                                if (_2115)
                                {
                                    _2007 = 1u;
                                }
                                else
                                {
                                    _2007 = (((_1916 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_1916 & 268435472u) == 16u) && (((_1916 & 33554432u) == 0u) || _2115))
                                {
                                    _2006 = 2u;
                                    _2008 = 0u;
                                    break;
                                }
                                _2006 = _2007;
                                _2008 = (_2007 == 1u) ? _1925 : 0u;
                                break;
                            }
                        }
                        if ((_401 != _2006) || (_407 != _2008))
                        {
                            _1127 = 0u;
                            _1128 = 0.0f;
                            _1130 = 0.0f;
                            _1132 = 0.0f;
                            _1134 = 0.0f;
                            _1135 = 1.0f;
                            _1136 = 1000.0f;
                            _1138 = 0.5f;
                            break;
                        }
                        float _2046 = _664 * 2.0f;
                        float _2049 = (_49_m0[51u].z * _2046) + (-1.0f);
                        float _2050 = ((1.0f - (_49_m0[51u].w * _666)) * 2.0f) + (-1.0f);
                        float _2066 = mad(_189, _668, mad(_182, _2050, _2049 * _175)) + _196;
                        float _2067 = (mad(_186, _668, mad(_179, _2050, _2049 * _172)) + _193) / _2066;
                        float _2068 = (mad(_187, _668, mad(_180, _2050, _2049 * _173)) + _194) / _2066;
                        float _2069 = (mad(_188, _668, mad(_181, _2050, _2049 * _174)) + _195) / _2066;
                        if (sqrt(((_2068 * _2068) + (_2067 * _2067)) + (_2069 * _2069)) > _49_m0[58u].w)
                        {
                            _1127 = 0u;
                            _1128 = 0.0f;
                            _1130 = 0.0f;
                            _1132 = 0.0f;
                            _1134 = 0.0f;
                            _1135 = 0.0f;
                            _1136 = 1000.0f;
                            _1138 = 0.5f;
                            break;
                        }
                        float _2095 = _2067 - _707;
                        float _2096 = _2068 - _708;
                        float _2097 = _2069 - _709;
                        float _2103 = sqrt(((_2096 * _2096) + (_2095 * _2095)) + (_2097 * _2097));
                        float _2111 = min(_49_m0[59u].y, max(0.0f, _2103 + (-0.001000000047497451305389404296875f)));
                        float _2180;
                        if (_623)
                        {
                            _2180 = min(_49_m0[59u].x, _49_m0[4u].z + _2111);
                        }
                        else
                        {
                            _2180 = _49_m0[59u].x;
                        }
                        float _2181 = _2180 - _2103;
                        if (!(_2181 > 0.0f))
                        {
                            _1127 = 0u;
                            _1128 = 0.0f;
                            _1130 = 0.0f;
                            _1132 = 0.0f;
                            _1134 = 1.0f;
                            _1135 = 1.0f;
                            _1136 = 0.0f;
                            _1138 = 0.5f;
                            break;
                        }
                        float _2203 = _2067 - (_2111 * _1037);
                        float _2204 = _2068 - (_2111 * _1040);
                        float _2205 = _2069 - (_2111 * _1043);
                        RayDesc _2ident = {float3(mad(_2205, _49_m0[46u].z, mad(_2204, _49_m0[46u].y, _49_m0[46u].x * _2203)) + _49_m0[46u].w, mad(_2205, _49_m0[47u].z, mad(_2204, _49_m0[47u].y, _49_m0[47u].x * _2203)) + _49_m0[47u].w, mad(_2205, _49_m0[48u].z, mad(_2204, _49_m0[48u].y, _49_m0[48u].x * _2203)) + _49_m0[48u].w), 0.0f, float3(mad(_1043, _49_m0[46u].z, mad(_1040, _49_m0[46u].y, _49_m0[46u].x * _1037)), mad(_1043, _49_m0[47u].z, mad(_1040, _49_m0[47u].y, _49_m0[47u].x * _1037)), mad(_1043, _49_m0[48u].z, mad(_1040, _49_m0[48u].y, _49_m0[48u].x * _1037))), _2181};
                        _2208.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2255 = _2208.Proceed();
                        uint _2256 = _2208.CommittedStatus();
                        if (!(_2256 == 1u))
                        {
                            _1127 = 0u;
                            _1128 = 0.0f;
                            _1130 = 0.0f;
                            _1132 = 0.0f;
                            _1134 = 1.0f;
                            _1135 = 0.0f;
                            _1136 = 0.0f;
                            _1138 = 0.5f;
                            break;
                        }
                        float _2325 = _2208.CommittedRayT();
                        if (!((_2325 < _2181) && (_2325 > 0.0f)))
                        {
                            _1127 = 0u;
                            _1128 = 0.0f;
                            _1130 = 0.0f;
                            _1132 = 0.0f;
                            _1134 = 1.0f;
                            _1135 = 0.0f;
                            _1136 = 0.0f;
                            _1138 = 0.5f;
                            break;
                        }
                        float _2376 = (_49_m0[51u].z * _2046) + (-1.0f);
                        float _2377 = ((1.0f - (_49_m0[51u].w * _666)) * 2.0f) + (-1.0f);
                        float _2393 = mad(_143, _668, mad(_136, _2377, _2376 * _129)) + _150;
                        float _2397 = _2325 - _2111;
                        float _2401 = ((mad(_140, _668, mad(_133, _2377, _2376 * _126)) + _147) / _2393) + (_2397 * _524);
                        float _2402 = ((mad(_141, _668, mad(_134, _2377, _2376 * _127)) + _148) / _2393) + (_2397 * _525);
                        float _2403 = ((mad(_142, _668, mad(_135, _2377, _2376 * _128)) + _149) / _2393) + (_2397 * _526);
                        float _2415 = mad(_115, _2403, mad(_108, _2402, _2401 * _101)) + _122;
                        float _2425 = (_49_m0[51u].z * _49_m0[51u].x) * ((((mad(_112, _2403, mad(_105, _2402, _2401 * _98)) + _119) / _2415) * 0.5f) + 0.5f);
                        float _2427 = (_49_m0[51u].w * _49_m0[51u].y) * (0.5f - (((mad(_113, _2403, mad(_106, _2402, _2401 * _99)) + _120) / _2415) * 0.5f));
                        float _2429 = (_203 / _201) * 0.0500000007450580596923828125f;
                        float _2432 = clamp(_2425 / _2429, 0.0f, 1.0f);
                        float _2433 = clamp(_2427 * 20.0f, 0.0f, 1.0f);
                        float _2443 = clamp(((_2429 + (-1.0f)) + _2425) / _2429, 0.0f, 1.0f);
                        float _2444 = clamp((_2427 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2455 = _2432 * _2433;
                        precise float _2456 = _2455 * _2455;
                        if ((((((3.0f - (_2433 * 2.0f)) * (3.0f - (_2432 * 2.0f))) * _2456) * (1.0f - ((_2443 * _2443) * (3.0f - (_2443 * 2.0f))))) * (1.0f - ((_2444 * _2444) * (3.0f - (_2444 * 2.0f))))) < 1.0f)
                        {
                            _1127 = 0u;
                            _1128 = 0.0f;
                            _1130 = 0.0f;
                            _1132 = 0.0f;
                            _1134 = _1047;
                            _1135 = _1047;
                            _1136 = 0.0f;
                            _1138 = _1048;
                            break;
                        }
                        _1127 = 1u;
                        _1128 = _49_m0[1u].w;
                        _1130 = _49_m0[2u].x;
                        _1132 = _49_m0[2u].y;
                        _1134 = 0.0f;
                        _1135 = 1.0f;
                        _1136 = 0.0f;
                        _1138 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1127 = 0u;
                        _1128 = 0.0f;
                        _1130 = 0.0f;
                        _1132 = 0.0f;
                        _1134 = 1.0f;
                        _1135 = 1.0f;
                        _1136 = 0.0f;
                        _1138 = 0.5f;
                        break;
                    }
                }
                uint _1205;
                float _1206;
                float _1207;
                float _1209;
                float _1211;
                if (_623 && (asuint(_49_m0[55u]).z != 0u))
                {
                    float frontier_phi_53_52_ladder;
                    float frontier_phi_53_52_ladder_1;
                    float frontier_phi_53_52_ladder_2;
                    float frontier_phi_53_52_ladder_3;
                    uint frontier_phi_53_52_ladder_4;
                    if ((_1031 != 1u) || (_1033 != 0u))
                    {
                        float _1254 = _569 / max(9.9999999747524270787835121154785e-07f, (-0.0f) - _616);
                        float _1263 = ((_1254 * _613) + _576) / _49_m0[51u].x;
                        float _1264 = ((_1254 * _615) + _577) / _49_m0[51u].y;
                        float _1396;
                        if (_1264 < 0.300000011920928955078125f)
                        {
                            float _1388 = (0.300000011920928955078125f - _1264) * 3.3333332538604736328125f;
                            _1396 = 0.300000011920928955078125f - ((_1388 / sqrt((_1388 * _1388) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _1396 = _1264;
                        }
                        float _1619;
                        if (_1263 < 0.300000011920928955078125f)
                        {
                            float _1612 = (0.300000011920928955078125f - _1263) * 3.3333332538604736328125f;
                            _1619 = 0.300000011920928955078125f - ((_1612 / sqrt((_1612 * _1612) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _1619 = _1263;
                        }
                        float _1792;
                        if ((1.0f - _1619) < 0.300000011920928955078125f)
                        {
                            float _1784 = (_1619 + (-0.699999988079071044921875f)) * 3.3333332538604736328125f;
                            _1792 = ((_1784 / sqrt((_1784 * _1784) + 1.0f)) * 0.300000011920928955078125f) + 0.699999988079071044921875f;
                        }
                        else
                        {
                            _1792 = _1619;
                        }
                        frontier_phi_53_52_ladder = 1.0f - clamp((_526 + (-0.25f)) * (-4.0f), 0.0f, 1.0f);
                        frontier_phi_53_52_ladder_1 = _1396 * _49_m0[51u].y;
                        frontier_phi_53_52_ladder_2 = _1792 * _49_m0[51u].x;
                        frontier_phi_53_52_ladder_3 = 0.0f;
                        frontier_phi_53_52_ladder_4 = 0u;
                    }
                    else
                    {
                        frontier_phi_53_52_ladder = _902;
                        frontier_phi_53_52_ladder_1 = _676;
                        frontier_phi_53_52_ladder_2 = _674;
                        frontier_phi_53_52_ladder_3 = _662;
                        frontier_phi_53_52_ladder_4 = _654;
                    }
                    _1205 = frontier_phi_53_52_ladder_4;
                    _1206 = frontier_phi_53_52_ladder_3;
                    _1207 = frontier_phi_53_52_ladder_2;
                    _1209 = frontier_phi_53_52_ladder_1;
                    _1211 = frontier_phi_53_52_ladder;
                }
                else
                {
                    _1205 = _654;
                    _1206 = _662;
                    _1207 = _674;
                    _1209 = _676;
                    _1211 = _902;
                }
                bool _1213 = _401 != 1u;
                float _1638;
                float _1640;
                float _1642;
                float _1644;
                if (_1211 == 0.0f)
                {
                    float _1398;
                    float _1400;
                    float _1402;
                    float _1404;
                    if (_1127 == 0u)
                    {
                        float frontier_phi_80_79_ladder;
                        float frontier_phi_80_79_ladder_1;
                        float frontier_phi_80_79_ladder_2;
                        float frontier_phi_80_79_ladder_3;
                        if (_1213)
                        {
                            float _1622 = _1040 * _943;
                            float _1626 = rsqrt(dot(float3(_1037, _1622, _1043), float3(_1037, _1622, _1043)));
                            float4 _1636 = _28[4u].SampleLevel(_58, float3(_1626 * _1037, _1626 * _1622, _1626 * _1043), 0.0f);
                            frontier_phi_80_79_ladder = 1.0f;
                            frontier_phi_80_79_ladder_1 = _1636.z;
                            frontier_phi_80_79_ladder_2 = _1636.y;
                            frontier_phi_80_79_ladder_3 = _1636.x;
                        }
                        else
                        {
                            frontier_phi_80_79_ladder = 0.0f;
                            frontier_phi_80_79_ladder_1 = 0.0f;
                            frontier_phi_80_79_ladder_2 = 0.0f;
                            frontier_phi_80_79_ladder_3 = 0.0f;
                        }
                        _1398 = frontier_phi_80_79_ladder_3;
                        _1400 = frontier_phi_80_79_ladder_2;
                        _1402 = frontier_phi_80_79_ladder_1;
                        _1404 = frontier_phi_80_79_ladder;
                    }
                    else
                    {
                        _1398 = _1128;
                        _1400 = _1130;
                        _1402 = _1132;
                        _1404 = 1.0f;
                    }
                    _1638 = _1398 * _469;
                    _1640 = _1400 * _469;
                    _1642 = _1402 * _469;
                    _1644 = _1404 * _469;
                }
                else
                {
                    uint4 _1270 = asuint(_54_m0[0u]);
                    float _1692;
                    float _1694;
                    float _1698;
                    float _1701;
                    if (_12.Load(int3(uint2(uint(float(_1270.x) * _1207), uint(float(_1270.y) * _1209)), 0u)).x > 0.0f)
                    {
                        float frontier_phi_99_81_ladder;
                        float frontier_phi_99_81_ladder_1;
                        float frontier_phi_99_81_ladder_2;
                        float frontier_phi_99_81_ladder_3;
                        float _1453;
                        float _1454;
                        float _1455;
                        for (;;)
                        {
                            uint _1410_dummy_parameter;
                            uint2 _1410 = spvTextureSize(_14, 0u, _1410_dummy_parameter);
                            float4 _1419 = _14.Load(int3(uint2(uint(float(_1410.x) * _1207), uint(float(_1410.y) * _1209)), 0u));
                            float4 _1443 = _13.SampleLevel(_57, float2(((_1419.x * 0.5f) * _49_m0[52u].z) + (_49_m0[52u].x * _1207), ((_1419.y * (-0.5f)) * _49_m0[52u].w) + (_49_m0[52u].y * _1209)), 0.0f);
                            _1453 = _49_m0[54u].x * _1443.x;
                            _1454 = _49_m0[54u].x * _1443.y;
                            _1455 = _49_m0[54u].x * _1443.z;
                            if (_1206 > 0.0f)
                            {
                                float _1697;
                                float _1700;
                                float _1703;
                                if (_623)
                                {
                                    _1697 = 0.0f;
                                    _1700 = 0.0f;
                                    _1703 = 0.0f;
                                }
                                else
                                {
                                    if (!((_401 != 4u) || (_656 != 0u)))
                                    {
                                        frontier_phi_99_81_ladder = _1453;
                                        frontier_phi_99_81_ladder_1 = _1211;
                                        frontier_phi_99_81_ladder_2 = _1454;
                                        frontier_phi_99_81_ladder_3 = _1455;
                                        break;
                                    }
                                    _1697 = _1453;
                                    _1700 = _1454;
                                    _1703 = _1455;
                                }
                                frontier_phi_99_81_ladder = _1697;
                                frontier_phi_99_81_ladder_1 = (1.0f - exp2(log2(_1206) * _49_m0[4u].x)) * _1211;
                                frontier_phi_99_81_ladder_2 = _1700;
                                frontier_phi_99_81_ladder_3 = _1703;
                                break;
                            }
                            else
                            {
                                frontier_phi_99_81_ladder = _1453;
                                frontier_phi_99_81_ladder_1 = _1211;
                                frontier_phi_99_81_ladder_2 = _1454;
                                frontier_phi_99_81_ladder_3 = _1455;
                                break;
                            }
                        }
                        _1692 = frontier_phi_99_81_ladder_1;
                        _1694 = frontier_phi_99_81_ladder;
                        _1698 = frontier_phi_99_81_ladder_2;
                        _1701 = frontier_phi_99_81_ladder_3;
                    }
                    else
                    {
                        float frontier_phi_99_82_ladder;
                        float frontier_phi_99_82_ladder_1;
                        float frontier_phi_99_82_ladder_2;
                        float frontier_phi_99_82_ladder_3;
                        if (_1205 == 0u)
                        {
                            float4 _1709 = _28[7u].SampleLevel(_58, float3(_1037, _1040, _1043), 0.0f);
                            frontier_phi_99_82_ladder = _1709.x;
                            frontier_phi_99_82_ladder_1 = _1211;
                            frontier_phi_99_82_ladder_2 = _1709.y;
                            frontier_phi_99_82_ladder_3 = _1709.z;
                        }
                        else
                        {
                            frontier_phi_99_82_ladder = _1696;
                            frontier_phi_99_82_ladder_1 = 0.0f;
                            frontier_phi_99_82_ladder_2 = _1696;
                            frontier_phi_99_82_ladder_3 = _1696;
                        }
                        _1692 = frontier_phi_99_82_ladder_1;
                        _1694 = frontier_phi_99_82_ladder;
                        _1698 = frontier_phi_99_82_ladder_2;
                        _1701 = frontier_phi_99_82_ladder_3;
                    }
                    float _1957;
                    float _1958;
                    float _1959;
                    float _1960;
                    if (_1127 == 0u)
                    {
                        float frontier_phi_127_114_ladder;
                        float frontier_phi_127_114_ladder_1;
                        float frontier_phi_127_114_ladder_2;
                        float frontier_phi_127_114_ladder_3;
                        if (_1213 && (_1692 < 1.0f))
                        {
                            float _1931 = _1040 * _943;
                            float _1935 = rsqrt(dot(float3(_1037, _1931, _1043), float3(_1037, _1931, _1043)));
                            float4 _1943 = _28[4u].SampleLevel(_58, float3(_1935 * _1037, _1935 * _1931, _1935 * _1043), 0.0f);
                            float _1945 = _1943.x;
                            float _1946 = _1943.y;
                            float _1947 = _1943.z;
                            frontier_phi_127_114_ladder = ((_1701 - _1947) * _1692) + _1947;
                            frontier_phi_127_114_ladder_1 = ((_1698 - _1946) * _1692) + _1946;
                            frontier_phi_127_114_ladder_2 = ((_1694 - _1945) * _1692) + _1945;
                            frontier_phi_127_114_ladder_3 = 1.0f;
                        }
                        else
                        {
                            frontier_phi_127_114_ladder = _1701;
                            frontier_phi_127_114_ladder_1 = _1698;
                            frontier_phi_127_114_ladder_2 = _1694;
                            frontier_phi_127_114_ladder_3 = _1692;
                        }
                        _1957 = frontier_phi_127_114_ladder_3;
                        _1958 = frontier_phi_127_114_ladder_2;
                        _1959 = frontier_phi_127_114_ladder_1;
                        _1960 = frontier_phi_127_114_ladder;
                    }
                    else
                    {
                        _1957 = 1.0f;
                        _1958 = ((_1694 - _1128) * _898) + _1128;
                        _1959 = ((_1698 - _1130) * _898) + _1130;
                        _1960 = ((_1701 - _1132) * _898) + _1132;
                    }
                    float _1645 = _1957 * _469;
                    _1638 = _1958 * _1645;
                    _1640 = _1959 * _1645;
                    _1642 = _1960 * _1645;
                    _1644 = _1645;
                }
                float _1649 = _49_m0[58u].z * _1138;
                float _1671 = max(_49_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _1673 = _1671 * ((_1649 * ((_1134 * 1000.0f) - _1638)) + _1638);
                float _1674 = _1671 * ((_1649 * ((_1135 * 1000.0f) - _1640)) + _1640);
                float _1675 = _1671 * ((_1649 * (_1136 - _1642)) + _1642);
                float _1681 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_1673, max(_1674, _1675)) + 1.0f);
                float _1685 = min(_1681 * _1673, 0.996078431606292724609375f);
                float _1687 = min(_1681 * _1674, 0.996078431606292724609375f);
                float _1688 = min(_1681 * _1675, 0.996078431606292724609375f);
                _35[uint2(_225, _228)] = float4(_1685, _1687, _1688, _1644);
                if (_232)
                {
                    _35[uint2(_225 + 1u, _228)] = float4(_1685, _1687, _1688, _1644);
                }
                if (_235)
                {
                    _35[uint2(_225, _228 + 1u)] = float4(_1685, _1687, _1688, _1644);
                }
                if (_236)
                {
                    _35[uint2(_225 + 1u, _228 + 1u)] = float4(_1685, _1687, _1688, _1644);
                }
                ladder_phi_8 = true;
                break;
            }
            if (ladder_phi_8)
            {
                break;
            }
            _35[uint2(_225, _228)] = 0.0f.xxxx;
            if (_232)
            {
                _35[uint2(_225 + 1u, _228)] = 0.0f.xxxx;
            }
            if (_235)
            {
                _35[uint2(_225, _228 + 1u)] = 0.0f.xxxx;
            }
            if (!_236)
            {
                break;
            }
            _35[uint2(_225 + 1u, _228 + 1u)] = 0.0f.xxxx;
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
