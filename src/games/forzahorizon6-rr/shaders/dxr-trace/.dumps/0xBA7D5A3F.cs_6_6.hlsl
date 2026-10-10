static float _1549;
static uint _2743;
static float _2744;
static float _2745;
static uint _2746;
static float _2747;
static float _2748;
static float _2749;
static float _2750;
static uint _2751;
static uint _2752;
static float _2753;
static float _2754;
static float _2755;
static float _2756;
static float _2757;
static uint _2758;
static float _2759;
static float _2765;
static uint _2766;
static float _2767;
static float _2772;
static float _2773;
static float _2774;

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

static RayQuery<RAY_FLAG_NONE> _2188;

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
                    uint _428;
                    uint _429;
                    if (_372 == 0u)
                    {
                        _428 = (((_380 & 2097152u) != 0u) && (_371 == _393)) ? (_391 | 128u) : _391;
                        _429 = _380;
                    }
                    else
                    {
                        _428 = _354;
                        _429 = _380 | ((_353 << 20u) & 134217728u);
                    }
                    uint _438 = _384 & 512u;
                    float _1089;
                    float _1091;
                    float _1093;
                    float _1095;
                    float _1097;
                    float _1099;
                    float _1101;
                    float _1103;
                    float _1105;
                    float _1107;
                    float _1109;
                    float _1111;
                    if (_438 == 0u)
                    {
                        if (!((_429 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _786 = asfloat(_17.Load((_388 * 115u) + 33u).x);
                        uint4 _794 = _24[2u].Load(int3(uint2(_269, _270), 0u));
                        uint _796 = _794.y;
                        uint _797 = _428 & 128u;
                        uint _1077;
                        uint _1078;
                        uint _1079;
                        uint _1080;
                        if (_797 == 0u)
                        {
                            _1077 = uint(((_429 & 817889384u) | (_384 & 576u)) != 0u) | (((_429 >> 19u) & 1u) ^ 1u);
                            _1078 = uint(((_429 & 17825808u) | (_384 & 520u)) != 0u);
                            _1079 = uint(((_429 & 46137344u) | (_384 & 2564u)) != 0u);
                            _1080 = 0u;
                        }
                        else
                        {
                            _1077 = 1u;
                            _1078 = _428 & 1u;
                            _1079 = 1u;
                            _1080 = 1u;
                        }
                        precise float _1084 = float(_796 & 127u) * 0.0078740157186985015869140625f;
                        bool _1088 = (_429 & 4194304u) == 0u;
                        float _1154;
                        if (_1088)
                        {
                            _1154 = _1084;
                        }
                        else
                        {
                            _1154 = float(_796 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1324;
                        if ((_429 & 134217728u) == 0u)
                        {
                            uint frontier_phi_63_49_ladder;
                            if ((_797 != 0u) || ((_429 & 17825792u) == 1048576u))
                            {
                                frontier_phi_63_49_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_63_49_ladder = _1078;
                            }
                            _1324 = frontier_phi_63_49_ladder;
                        }
                        else
                        {
                            _1324 = _1078;
                        }
                        uint4 _1327 = _24[1u].Load(int3(uint2(_269, _270), 0u));
                        uint _1329 = _1327.x;
                        float _1454;
                        float _1456;
                        float _1458;
                        if (_1077 == 0u)
                        {
                            _1454 = 0.0f;
                            _1456 = 0.0f;
                            _1458 = 0.0f;
                        }
                        else
                        {
                            float4 _1463 = _20[8u].Load(int3(uint2(_269, _270), 0u));
                            _1454 = _1463.x;
                            _1456 = _1463.y;
                            _1458 = _1463.z;
                        }
                        uint _1675;
                        if (_1324 == 0u)
                        {
                            _1675 = 0u;
                        }
                        else
                        {
                            _1675 = _24[9u].Load(int3(uint2(_269, _270), 0u)).x;
                        }
                        uint _1808;
                        if (_1079 == 0u)
                        {
                            _1808 = 0u;
                        }
                        else
                        {
                            _1808 = _24[10u].Load(int3(uint2(_269, _270), 0u)).x;
                        }
                        float _1818 = (float((_1329 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1819 = (float(_1329 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1823 = (1.0f - abs(_1818)) - abs(_1819);
                        float _1825 = clamp((-0.0f) - _1823, 0.0f, 1.0f);
                        float _1826 = (-0.0f) - _1825;
                        float _1831 = ((_1818 >= 0.0f) ? _1826 : _1825) + _1818;
                        float _1832 = ((_1819 >= 0.0f) ? _1826 : _1825) + _1819;
                        float _1836 = rsqrt(dot(float3(_1831, _1832, _1823), float3(_1831, _1832, _1823)));
                        float _1837 = _1831 * _1836;
                        float _1838 = _1832 * _1836;
                        float _1839 = _1836 * _1823;
                        float _1090 = float(_1329 & 255u);
                        float _1843 = ((_429 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _2006;
                        float _2007;
                        float _2008;
                        float _2009;
                        uint _2010;
                        if ((_384 & 64u) == 0u)
                        {
                            float frontier_phi_130_122_ladder;
                            float frontier_phi_130_122_ladder_1;
                            float frontier_phi_130_122_ladder_2;
                            float frontier_phi_130_122_ladder_3;
                            uint frontier_phi_130_122_ladder_4;
                            if ((_429 & 276824064u) == 0u)
                            {
                                frontier_phi_130_122_ladder = 0.0f;
                                frontier_phi_130_122_ladder_1 = ((_429 & 8u) != 0u) ? _1456 : _1843;
                                frontier_phi_130_122_ladder_2 = 0.0f;
                                frontier_phi_130_122_ladder_3 = 0.0f;
                                frontier_phi_130_122_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_130_122_ladder = 0.0f;
                                frontier_phi_130_122_ladder_1 = _1843;
                                frontier_phi_130_122_ladder_2 = 0.0f;
                                frontier_phi_130_122_ladder_3 = 0.0f;
                                frontier_phi_130_122_ladder_4 = 0u;
                            }
                            _2006 = frontier_phi_130_122_ladder_1;
                            _2007 = frontier_phi_130_122_ladder_2;
                            _2008 = frontier_phi_130_122_ladder;
                            _2009 = frontier_phi_130_122_ladder_3;
                            _2010 = frontier_phi_130_122_ladder_4;
                        }
                        else
                        {
                            float _1959 = (_1456 * 2.0f) + (-1.0f);
                            float _1960 = (_1458 * 2.0f) + (-1.0f);
                            float _1964 = (1.0f - abs(_1959)) - abs(_1960);
                            float _1966 = clamp((-0.0f) - _1964, 0.0f, 1.0f);
                            float _1967 = (-0.0f) - _1966;
                            float _1972 = ((_1959 >= 0.0f) ? _1967 : _1966) + _1959;
                            float _1973 = ((_1960 >= 0.0f) ? _1967 : _1966) + _1960;
                            float _1977 = rsqrt(dot(float3(_1972, _1973, _1964), float3(_1972, _1973, _1964)));
                            _2006 = floor(round(_1454 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _2007 = _1972 * _1977;
                            _2008 = _1973 * _1977;
                            _2009 = _1977 * _1964;
                            _2010 = 1u;
                        }
                        float _1098;
                        if ((_429 & 32768u) == 0u)
                        {
                            _1098 = _2006;
                        }
                        else
                        {
                            float frontier_phi_135_136_ladder;
                            if (_17.Load((_388 * 115u) + 36u).x == 0u)
                            {
                                float _2152 = clamp((_1090 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _786;
                                frontier_phi_135_136_ladder = ((_429 & 131072u) != 0u) ? _2152 : ((((clamp((1.21000003814697265625f / (exp2((_1154 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_388 * 115u) + 32u).x)) + 1.0f) * _2152);
                            }
                            else
                            {
                                frontier_phi_135_136_ladder = _786;
                            }
                            _1098 = frontier_phi_135_136_ladder;
                        }
                        uint _2063 = _428 & 1u;
                        float _2095;
                        float _2097;
                        float _2099;
                        uint _2101;
                        if (((_429 & 16u) == 0u) || (((_2063 | (_384 & 8u)) | (_429 & 16777216u)) != 0u))
                        {
                            _2095 = _2007;
                            _2097 = _2008;
                            _2099 = _2009;
                            _2101 = _2010;
                        }
                        else
                        {
                            float _2111 = (float(_1675 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2112 = (float(_1675 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2116 = (1.0f - abs(_2111)) - abs(_2112);
                            float _2118 = clamp((-0.0f) - _2116, 0.0f, 1.0f);
                            float _2119 = (-0.0f) - _2118;
                            float _2124 = ((_2111 >= 0.0f) ? _2119 : _2118) + _2111;
                            float _2125 = ((_2112 >= 0.0f) ? _2119 : _2118) + _2112;
                            float _2129 = rsqrt(dot(float3(_2124, _2125, _2116), float3(_2124, _2125, _2116)));
                            _2095 = _2124 * _2129;
                            _2097 = _2125 * _2129;
                            _2099 = _2129 * _2116;
                            _2101 = 1u;
                        }
                        float _1092;
                        float _1094;
                        float _1096;
                        if (_2063 == 0u)
                        {
                            float frontier_phi_147_146_ladder;
                            float frontier_phi_147_146_ladder_1;
                            float frontier_phi_147_146_ladder_2;
                            if (((_428 & 64u) == 0u) && (_1080 != 0u))
                            {
                                float2 _2252 = spvUnpackHalf2x16((_1808 >> 17u) & 32736u);
                                float _2253 = _2252.x;
                                float _2256 = (spvUnpackHalf2x16((_1808 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2257 = (spvUnpackHalf2x16((_1808 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2261 = (1.0f - abs(_2256)) - abs(_2257);
                                float _2263 = clamp((-0.0f) - _2261, 0.0f, 1.0f);
                                float _2264 = (-0.0f) - _2263;
                                float _2269 = ((_2256 >= 0.0f) ? _2264 : _2263) + _2256;
                                float _2270 = ((_2257 >= 0.0f) ? _2264 : _2263) + _2257;
                                float _2274 = rsqrt(dot(float3(_2269, _2270, _2261), float3(_2269, _2270, _2261)));
                                float _2284 = (((_2269 * _2274) - _1837) * _2253) + _1837;
                                float _2285 = (((_2270 * _2274) - _1838) * _2253) + _1838;
                                float _2286 = (((_2274 * _2261) - _1839) * _2253) + _1839;
                                float _2290 = rsqrt(dot(float3(_2284, _2285, _2286), float3(_2284, _2285, _2286)));
                                frontier_phi_147_146_ladder = _2284 * _2290;
                                frontier_phi_147_146_ladder_1 = _2285 * _2290;
                                frontier_phi_147_146_ladder_2 = _2286 * _2290;
                            }
                            else
                            {
                                frontier_phi_147_146_ladder = _1837;
                                frontier_phi_147_146_ladder_1 = _1838;
                                frontier_phi_147_146_ladder_2 = _1839;
                            }
                            _1092 = frontier_phi_147_146_ladder;
                            _1094 = frontier_phi_147_146_ladder_1;
                            _1096 = frontier_phi_147_146_ladder_2;
                        }
                        else
                        {
                            _1092 = _1837;
                            _1094 = _1838;
                            _1096 = _1839;
                        }
                        float _1106;
                        float _1108;
                        float _1110;
                        float _1112;
                        if (_1088)
                        {
                            float frontier_phi_152_151_ladder;
                            float frontier_phi_152_151_ladder_1;
                            float frontier_phi_152_151_ladder_2;
                            float frontier_phi_152_151_ladder_3;
                            if (((_429 & 33554432u) == 0u) || (((_384 & 4u) != 0u) && ((_429 & 8388608u) == 0u)))
                            {
                                frontier_phi_152_151_ladder = 0.0f;
                                frontier_phi_152_151_ladder_1 = 0.0f;
                                frontier_phi_152_151_ladder_2 = 0.0f;
                                frontier_phi_152_151_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2322 = (spvUnpackHalf2x16((_1808 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2323 = (spvUnpackHalf2x16((_1808 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2327 = (1.0f - abs(_2322)) - abs(_2323);
                                float _2329 = clamp((-0.0f) - _2327, 0.0f, 1.0f);
                                float _2330 = (-0.0f) - _2329;
                                float _2335 = ((_2322 >= 0.0f) ? _2330 : _2329) + _2322;
                                float _2336 = ((_2323 >= 0.0f) ? _2330 : _2329) + _2323;
                                float _2340 = rsqrt(dot(float3(_2335, _2336, _2327), float3(_2335, _2336, _2327)));
                                float _2341 = _2335 * _2340;
                                float _2342 = _2336 * _2340;
                                float _2343 = _2340 * _2327;
                                float _2347 = rsqrt(dot(float3(_2341, _2342, _2343), float3(_2341, _2342, _2343)));
                                frontier_phi_152_151_ladder = _2347 * _2343;
                                frontier_phi_152_151_ladder_1 = _2347 * _2342;
                                frontier_phi_152_151_ladder_2 = _2347 * _2341;
                                frontier_phi_152_151_ladder_3 = spvUnpackHalf2x16((_1808 >> 17u) & 32736u).x;
                            }
                            _1106 = frontier_phi_152_151_ladder_3;
                            _1108 = frontier_phi_152_151_ladder_2;
                            _1110 = frontier_phi_152_151_ladder_1;
                            _1112 = frontier_phi_152_151_ladder;
                        }
                        else
                        {
                            _1106 = 0.0f;
                            _1108 = 0.0f;
                            _1110 = 0.0f;
                            _1112 = 0.0f;
                        }
                        bool _2304 = _2101 != 0u;
                        _1089 = _1090;
                        _1091 = _1092;
                        _1093 = _1094;
                        _1095 = _1096;
                        _1097 = _1098;
                        _1099 = _2304 ? _2095 : _1092;
                        _1101 = _2304 ? _2097 : _1094;
                        _1103 = _2304 ? _2099 : _1096;
                        _1105 = _1106;
                        _1107 = _1108;
                        _1109 = _1110;
                        _1111 = _1112;
                    }
                    else
                    {
                        uint4 _756 = _24[1u].Load(int3(uint2(_269, _270), 0u));
                        uint _758 = _756.x;
                        uint4 _762 = _24[9u].Load(int3(uint2(_269, _270), 0u));
                        uint _764 = _762.x;
                        float _975;
                        float _976;
                        float _977;
                        if ((_429 & 33554432u) == 0u)
                        {
                            float _806 = (float((_758 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _807 = (float(_758 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _811 = (1.0f - abs(_806)) - abs(_807);
                            float _813 = clamp((-0.0f) - _811, 0.0f, 1.0f);
                            float _814 = (-0.0f) - _813;
                            float _819 = ((_806 >= 0.0f) ? _814 : _813) + _806;
                            float _820 = ((_807 >= 0.0f) ? _814 : _813) + _807;
                            float _824 = rsqrt(dot(float3(_819, _820, _811), float3(_819, _820, _811)));
                            _975 = _819 * _824;
                            _976 = _820 * _824;
                            _977 = _824 * _811;
                        }
                        else
                        {
                            float _835 = (float((_764 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _836 = (float(_764 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _840 = (1.0f - abs(_835)) - abs(_836);
                            float _842 = clamp((-0.0f) - _840, 0.0f, 1.0f);
                            float _843 = (-0.0f) - _842;
                            float _848 = ((_835 >= 0.0f) ? _843 : _842) + _835;
                            float _849 = ((_836 >= 0.0f) ? _843 : _842) + _836;
                            float _853 = rsqrt(dot(float3(_848, _849, _840), float3(_848, _849, _840)));
                            _975 = _848 * _853;
                            _976 = _849 * _853;
                            _977 = _853 * _840;
                        }
                        _1089 = float(_758 & 255u);
                        _1091 = _975;
                        _1093 = _976;
                        _1095 = _977;
                        _1097 = 1.0f;
                        _1099 = _975;
                        _1101 = _976;
                        _1103 = _977;
                        _1105 = 0.0f;
                        _1107 = 0.0f;
                        _1109 = 0.0f;
                        _1111 = 0.0f;
                    }
                    precise float _1113 = _1089 * 0.0039215688593685626983642578125f;
                    if ((_428 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1163 = ((_428 & 128u) | _438) != 0u;
                    uint _1237;
                    if (_1163)
                    {
                        _1237 = 1u;
                    }
                    else
                    {
                        _1237 = (((_429 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _396;
                    float _398;
                    float _400;
                    uint _402;
                    float _1337;
                    if (((_429 & 33554432u) == 0u) || _1163)
                    {
                        bool _1331 = _76 != 0u;
                        uint _1339;
                        if ((_429 & 16u) == 0u)
                        {
                            _1339 = _1237;
                        }
                        else
                        {
                            _1339 = ((_429 & 268435456u) != 0u) ? _1237 : 2u;
                        }
                        _1337 = _1097 * _1113;
                        _396 = _1331 ? _1099 : _1091;
                        _398 = _1331 ? _1101 : _1093;
                        _400 = _1331 ? _1103 : _1095;
                        _402 = _1339;
                    }
                    else
                    {
                        _1337 = _1105;
                        _396 = _1107;
                        _398 = _1109;
                        _400 = _1111;
                        _402 = _1237;
                    }
                    uint _1340 = _402 + 102u;
                    float _1349 = clamp((_1337 - _44_m0[_1340].x) / (_44_m0[_1340].y - _44_m0[_1340].x), 0.0f, 1.0f);
                    _395 = _396;
                    _397 = _398;
                    _399 = _400;
                    _401 = _402;
                    _403 = (_1349 * _1349) * (3.0f - (_1349 * 2.0f));
                    _405 = _44_m0[_1340].z;
                    _407 = (_402 == 1u) ? _393 : 0u;
                    _409 = asfloat(_17.Load((_388 * 115u) + 114u).x);
                }
                if (_403 == 0.0f)
                {
                    ladder_phi_8 = false;
                    break;
                }
                float _426 = float(_238);
                float _427 = float(_237);
                float _445;
                if (_267)
                {
                    _445 = _266;
                }
                else
                {
                    _445 = _12.Load(int3(uint2(uint(int(_258 * _427)), uint(int(_259 * _426))), 0u)).x;
                }
                float _454 = _49_m0[50u].w + _49_m0[50u].y;
                uint _457 = _401 + 63u;
                float _466 = clamp(((_49_m0[50u].x / (_454 - (_49_m0[50u].y * _445))) - _49_m0[_457].y) / (_49_m0[_457].x - _49_m0[_457].y), 0.0f, 1.0f);
                float _471 = ((_466 * _466) * _403) * (3.0f - (_466 * 2.0f));
                float _482 = ((_258 * 2.0f) * _49_m0[51u].z) + (-1.0f);
                float _483 = ((1.0f - (_49_m0[51u].w * _259)) * 2.0f) + (-1.0f);
                float _499 = mad(_143, _445, mad(_136, _483, _482 * _129)) + _150;
                float _500 = (mad(_140, _445, mad(_133, _483, _482 * _126)) + _147) / _499;
                float _501 = (mad(_141, _445, mad(_134, _483, _482 * _127)) + _148) / _499;
                float _502 = (mad(_142, _445, mad(_135, _483, _482 * _128)) + _149) / _499;
                float _506 = rsqrt(dot(float3(_500, _501, _502), float3(_500, _501, _502)));
                float _507 = _506 * _500;
                float _508 = _506 * _501;
                float _509 = _506 * _502;
                float _512 = mad(_92, _399, mad(_86, _397, _395 * _80));
                float _515 = mad(_93, _399, mad(_87, _397, _395 * _81));
                float _518 = mad(_94, _399, mad(_88, _397, _395 * _82));
                float _522 = dot(float3(_507, _508, _509), float3(_512, _515, _518)) * 2.0f;
                float _526 = _507 - (_522 * _512);
                float _527 = _508 - (_522 * _515);
                float _528 = _509 - (_522 * _518);
                bool _532 = dot(float3(_526, _527, _528), float3(_507, _508, _509)) < 0.0f;
                float _539 = sqrt(((_501 * _501) + (_500 * _500)) + (_502 * _502)) * 0.001000000047497451305389404296875f;
                float _550 = ((_539 * _512) + _500) + (_526 * _405);
                float _551 = ((_539 * _515) + _501) + (_527 * _405);
                float _552 = ((_539 * _518) + _502) + (_528 * _405);
                float _568 = mad(_115, _552, mad(_108, _551, _550 * _101)) + _122;
                float _571 = (mad(_114, _552, mad(_107, _551, _550 * _100)) + _121) / _568;
                float _574 = (((mad(_112, _552, mad(_105, _551, _550 * _98)) + _119) / _568) * 0.5f) + 0.5f;
                float _575 = 0.5f - (((mad(_113, _552, mad(_106, _551, _550 * _99)) + _120) / _568) * 0.5f);
                float _578 = _574 * _49_m0[51u].x;
                float _579 = _575 * _49_m0[51u].y;
                float _584 = _550 + (_526 * 0.100000001490116119384765625f);
                float _585 = _551 + (_527 * 0.100000001490116119384765625f);
                float _586 = _552 + (_528 * 0.100000001490116119384765625f);
                float _602 = mad(_115, _586, mad(_108, _585, _584 * _101)) + _122;
                float _611 = _49_m0[51u].x * (((((mad(_112, _586, mad(_105, _585, _584 * _98)) + _119) / _602) * 0.5f) + 0.5f) - _574);
                float _613 = _49_m0[51u].y * ((0.5f - (((mad(_113, _586, mad(_106, _585, _584 * _99)) + _120) / _602) * 0.5f)) - _575);
                float _614 = ((mad(_114, _586, mad(_107, _585, _584 * _100)) + _121) / _602) - _571;
                float _615 = _611 * 10.0f;
                float _617 = _613 * 10.0f;
                float _618 = _614 * 10.0f;
                float _625 = _49_m0[2u].w / dot(float3(_526, _527, _528), float3(_512, _515, _518));
                float _629 = (_625 * _526) + _550;
                float _630 = (_625 * _527) + _551;
                float _631 = (_625 * _528) + _552;
                float _647 = mad(_115, _631, mad(_108, _630, _629 * _101)) + _122;
                float _656 = _49_m0[51u].x * (((((mad(_112, _631, mad(_105, _630, _629 * _98)) + _119) / _647) * 0.5f) + 0.5f) - _574);
                float _658 = _49_m0[51u].y * ((0.5f - (((mad(_113, _631, mad(_106, _630, _629 * _99)) + _120) / _647) * 0.5f)) - _575);
                float _659 = ((mad(_114, _631, mad(_107, _630, _629 * _100)) + _121) / _647) - _571;
                float _672 = sqrt(((_656 * _656) + (_659 * _659)) + (_658 * _658)) / sqrt(((_615 * _615) + (_618 * _618)) + (_617 * _617));
                float _679 = (_615 != 0.0f) ? (0.100000001490116119384765625f / _611) : 3.4028234663852885981170418348452e+38f;
                float _681 = (_617 != 0.0f) ? (0.100000001490116119384765625f / _613) : 3.4028234663852885981170418348452e+38f;
                float _682 = (_618 != 0.0f) ? (0.100000001490116119384765625f / _614) : 3.4028234663852885981170418348452e+38f;
                float _683 = 1.0f / _427;
                float _684 = 1.0f / _426;
                float _685 = 0.004999999888241291046142578125f / _427;
                float _687 = 0.004999999888241291046142578125f / _426;
                float _696 = float(_615 >= 0.0f);
                float _697 = float(_617 >= 0.0f);
                float _706 = ((_615 < 0.0f) ? ((-0.0f) - _685) : _685) - _578;
                float _709 = ((_617 < 0.0f) ? ((-0.0f) - _687) : _687) - _579;
                float _712 = min((((floor(_578 * _427) + _696) * _683) + _706) * _679, (((floor(_579 * _426) + _697) * _684) + _709) * _681);
                float _716 = (_712 * _615) + _578;
                float _717 = (_712 * _617) + _579;
                float _718 = (_712 * _618) + _571;
                float _721 = _49_m0[50u].x / (_454 - (_718 * _49_m0[50u].y));
                float _731 = 1.0f / max(0.00999999977648258209228515625f, max(abs(_611 * 38400.0f), abs(_613 * 21600.0f)));
                float _739 = max(_49_m0[3u].z, _49_m0[5u].z * _731);
                float _772;
                if (asuint(_49_m0[5u]).y == 0u)
                {
                    _772 = _739;
                }
                else
                {
                    _772 = min(_739, _49_m0[5u].w * _731);
                }
                uint _862;
                float _864;
                float _866;
                float _868;
                uint _870;
                float _872;
                float _874;
                float _876;
                float _878;
                uint _880;
                float _882;
                float _884;
                uint _886;
                if (_208 == 0u)
                {
                    _862 = 0u;
                    _864 = _718;
                    _866 = _717;
                    _868 = _716;
                    _870 = 0u;
                    _872 = _718;
                    _874 = _717;
                    _876 = _716;
                    _878 = 1.0f;
                    _880 = 0u;
                    _882 = 0.0f;
                    _884 = 0.0f;
                    _886 = 1u;
                }
                else
                {
                    uint _863;
                    float _865;
                    float _867;
                    float _869;
                    uint _871;
                    uint _1062;
                    float _873;
                    float _875;
                    float _877;
                    float _879;
                    uint _881;
                    float _883;
                    float _885;
                    uint _887;
                    float _1045;
                    uint _1047;
                    float _1052;
                    float _1054;
                    float _1056;
                    float _1058;
                    float _1060;
                    uint _1043 = 0u;
                    float _1044 = _772;
                    uint _1046 = 0u;
                    float _1048 = _718;
                    float _1049 = _717;
                    float _1050 = _716;
                    float _1051 = _712;
                    float _1053 = _684;
                    float _1055 = _683;
                    float _1057 = _426;
                    float _1059 = _427;
                    uint _1061 = 0u;
                    uint _1063 = 0u;
                    float _1064 = _718;
                    float _1065 = _717;
                    float _1066 = _716;
                    float _1067 = 1.0f;
                    uint _1068 = 0u;
                    float _1069 = 0.0f;
                    float _1070 = 0.0f;
                    uint _1071 = 1u;
                    float _1072;
                    float _1073;
                    uint _1074;
                    uint _1075;
                    bool _1076;
                    for (;;)
                    {
                        _1072 = _1059 * _1050;
                        _1073 = _1057 * _1049;
                        _1074 = uint(int(_1072));
                        _1075 = uint(int(_1073));
                        _1076 = _1061 == 0u;
                        float _1187;
                        if (_1076)
                        {
                            _1187 = _12.Load(int3(uint2(_1074, _1075), 0u)).x;
                        }
                        else
                        {
                            _1187 = _15.Load(int3(uint2(_1074, _1075), _1061 + 4294967295u)).x;
                        }
                        float _1193 = ((_1072 >= floor(_1059)) || (_1073 >= floor(_1057))) ? 1.0f : _1187;
                        float _1207 = (_618 < 0.0f) ? ((_1193 - _571) * _682) : 3.4028234663852885981170418348452e+38f;
                        float _1209 = min(min((((floor(_1072) + _696) * _1055) + _706) * _679, (((floor(_1073) + _697) * _1053) + _709) * _681), _1207);
                        bool _1210 = _1193 < _1048;
                        bool _1214 = _1210 && (asuint(_1209) != asuint(_1207));
                        float _1215 = _1210 ? _1209 : _1051;
                        float _1219 = (_1215 * _615) + _578;
                        float _1220 = (_1215 * _617) + _579;
                        float _1221 = (_1215 * _618) + _571;
                        uint _1223 = (_1214 ? 1u : 4294967295u) + _1061;
                        float _1224 = _1214 ? 0.5f : 2.0f;
                        float _1225 = _1224 * _1059;
                        float _1226 = _1224 * _1057;
                        float _1227 = _1214 ? 2.0f : 0.5f;
                        float _1228 = _1227 * _1055;
                        float _1229 = _1227 * _1053;
                        _863 = _1043 + 1u;
                        uint _1442;
                        uint _1445;
                        if (int(_1223) < int(0u))
                        {
                            float frontier_phi_76_61_ladder;
                            uint frontier_phi_76_61_ladder_1;
                            float frontier_phi_76_61_ladder_2;
                            float frontier_phi_76_61_ladder_3;
                            uint frontier_phi_76_61_ladder_4;
                            float frontier_phi_76_61_ladder_5;
                            float frontier_phi_76_61_ladder_6;
                            float frontier_phi_76_61_ladder_7;
                            float frontier_phi_76_61_ladder_8;
                            uint frontier_phi_76_61_ladder_9;
                            uint frontier_phi_76_61_ladder_10;
                            float frontier_phi_76_61_ladder_11;
                            float frontier_phi_76_61_ladder_12;
                            float frontier_phi_76_61_ladder_13;
                            float frontier_phi_76_61_ladder_14;
                            float frontier_phi_76_61_ladder_15;
                            uint frontier_phi_76_61_ladder_16;
                            float _1301;
                            float4 _1306;
                            float _1309;
                            float _1312;
                            bool _1313;
                            for (;;)
                            {
                                float _1299 = _49_m0[50u].w + _49_m0[50u].y;
                                _1301 = _49_m0[50u].x / (_1299 - (_49_m0[50u].y * _1193));
                                float _1304 = _49_m0[50u].x / (_1299 - (_49_m0[50u].y * _1221));
                                _1306 = _49_m0[3u];
                                _1309 = abs(_721 - _1304);
                                _1312 = _1304 - _1301;
                                _1313 = _1312 > max(_1306.x, _1306.x * _1309);
                                if (_1313)
                                {
                                    uint _1447;
                                    if (_1046 == 0u)
                                    {
                                        uint frontier_phi_94_93_ladder;
                                        if ((_401 == 2u) || (_401 == 4u))
                                        {
                                            if ((_1215 < _672) && (abs(_1312) < _49_m0[2u].z))
                                            {
                                                frontier_phi_76_61_ladder = _1044;
                                                frontier_phi_76_61_ladder_1 = _1071;
                                                frontier_phi_76_61_ladder_2 = _1219;
                                                frontier_phi_76_61_ladder_3 = _1220;
                                                frontier_phi_76_61_ladder_4 = 1u;
                                                frontier_phi_76_61_ladder_5 = 0.0f;
                                                frontier_phi_76_61_ladder_6 = _1066;
                                                frontier_phi_76_61_ladder_7 = _1065;
                                                frontier_phi_76_61_ladder_8 = _1064;
                                                frontier_phi_76_61_ladder_9 = 1u;
                                                frontier_phi_76_61_ladder_10 = _1223;
                                                frontier_phi_76_61_ladder_11 = _1225;
                                                frontier_phi_76_61_ladder_12 = _1226;
                                                frontier_phi_76_61_ladder_13 = _1228;
                                                frontier_phi_76_61_ladder_14 = _1229;
                                                frontier_phi_76_61_ladder_15 = _1215;
                                                frontier_phi_76_61_ladder_16 = 1u;
                                                break;
                                            }
                                            frontier_phi_94_93_ladder = 1u;
                                        }
                                        else
                                        {
                                            frontier_phi_94_93_ladder = 1u;
                                        }
                                        _1447 = frontier_phi_94_93_ladder;
                                    }
                                    else
                                    {
                                        _1447 = _1046;
                                    }
                                    if (!(_1068 == 0u))
                                    {
                                        frontier_phi_76_61_ladder = _1044;
                                        frontier_phi_76_61_ladder_1 = _1071;
                                        frontier_phi_76_61_ladder_2 = _1070;
                                        frontier_phi_76_61_ladder_3 = _1069;
                                        frontier_phi_76_61_ladder_4 = _1068;
                                        frontier_phi_76_61_ladder_5 = _1067;
                                        frontier_phi_76_61_ladder_6 = _1066;
                                        frontier_phi_76_61_ladder_7 = _1065;
                                        frontier_phi_76_61_ladder_8 = _1064;
                                        frontier_phi_76_61_ladder_9 = _1063;
                                        frontier_phi_76_61_ladder_10 = _1223;
                                        frontier_phi_76_61_ladder_11 = _1225;
                                        frontier_phi_76_61_ladder_12 = _1226;
                                        frontier_phi_76_61_ladder_13 = _1228;
                                        frontier_phi_76_61_ladder_14 = _1229;
                                        frontier_phi_76_61_ladder_15 = _1215;
                                        frontier_phi_76_61_ladder_16 = _1447;
                                        break;
                                    }
                                    float _1446 = _1215 + _1044;
                                    bool _1784 = _1063 != 0u;
                                    float _1440 = _1784 ? _1070 : _1219;
                                    float _1441 = _1784 ? _1069 : _1220;
                                    uint _1443 = ((_401 == 1u) || (asuint(_49_m0[62u]).z == 0u)) ? 1u : _1063;
                                    if (asuint(_49_m0[5u]).y == 0u)
                                    {
                                        frontier_phi_76_61_ladder = _1044;
                                        frontier_phi_76_61_ladder_1 = _1071;
                                        frontier_phi_76_61_ladder_2 = _1440;
                                        frontier_phi_76_61_ladder_3 = _1441;
                                        frontier_phi_76_61_ladder_4 = 0u;
                                        frontier_phi_76_61_ladder_5 = _1067;
                                        frontier_phi_76_61_ladder_6 = _1066;
                                        frontier_phi_76_61_ladder_7 = _1065;
                                        frontier_phi_76_61_ladder_8 = _1064;
                                        frontier_phi_76_61_ladder_9 = _1443;
                                        frontier_phi_76_61_ladder_10 = 0u;
                                        frontier_phi_76_61_ladder_11 = _427;
                                        frontier_phi_76_61_ladder_12 = _426;
                                        frontier_phi_76_61_ladder_13 = _683;
                                        frontier_phi_76_61_ladder_14 = _684;
                                        frontier_phi_76_61_ladder_15 = _1446;
                                        frontier_phi_76_61_ladder_16 = _1447;
                                        break;
                                    }
                                    frontier_phi_76_61_ladder = min(_739, _49_m0[6u].x * _1044);
                                    frontier_phi_76_61_ladder_1 = _1071;
                                    frontier_phi_76_61_ladder_2 = _1440;
                                    frontier_phi_76_61_ladder_3 = _1441;
                                    frontier_phi_76_61_ladder_4 = 0u;
                                    frontier_phi_76_61_ladder_5 = _1067;
                                    frontier_phi_76_61_ladder_6 = _1066;
                                    frontier_phi_76_61_ladder_7 = _1065;
                                    frontier_phi_76_61_ladder_8 = _1064;
                                    frontier_phi_76_61_ladder_9 = _1443;
                                    frontier_phi_76_61_ladder_10 = 0u;
                                    frontier_phi_76_61_ladder_11 = _427;
                                    frontier_phi_76_61_ladder_12 = _426;
                                    frontier_phi_76_61_ladder_13 = _683;
                                    frontier_phi_76_61_ladder_14 = _684;
                                    frontier_phi_76_61_ladder_15 = _1446;
                                    frontier_phi_76_61_ladder_16 = _1447;
                                    break;
                                }
                                else
                                {
                                    float _1429 = max(_1306.y, _1306.y * _1309);
                                    float _1432 = _1429 * _1306.w;
                                    float _1436 = clamp((abs(_1312) - _1432) / (_1429 - _1432), 0.0f, 1.0f);
                                    uint _1438 = uint(_1301 < _721);
                                    float frontier_phi_76_61_ladder_75_ladder;
                                    uint frontier_phi_76_61_ladder_75_ladder_1;
                                    float frontier_phi_76_61_ladder_75_ladder_2;
                                    float frontier_phi_76_61_ladder_75_ladder_3;
                                    uint frontier_phi_76_61_ladder_75_ladder_4;
                                    float frontier_phi_76_61_ladder_75_ladder_5;
                                    float frontier_phi_76_61_ladder_75_ladder_6;
                                    float frontier_phi_76_61_ladder_75_ladder_7;
                                    float frontier_phi_76_61_ladder_75_ladder_8;
                                    uint frontier_phi_76_61_ladder_75_ladder_9;
                                    uint frontier_phi_76_61_ladder_75_ladder_10;
                                    float frontier_phi_76_61_ladder_75_ladder_11;
                                    float frontier_phi_76_61_ladder_75_ladder_12;
                                    float frontier_phi_76_61_ladder_75_ladder_13;
                                    float frontier_phi_76_61_ladder_75_ladder_14;
                                    float frontier_phi_76_61_ladder_75_ladder_15;
                                    uint frontier_phi_76_61_ladder_75_ladder_16;
                                    if (_1063 == 0u)
                                    {
                                        frontier_phi_76_61_ladder_75_ladder = _1044;
                                        frontier_phi_76_61_ladder_75_ladder_1 = _1438;
                                        frontier_phi_76_61_ladder_75_ladder_2 = _1070;
                                        frontier_phi_76_61_ladder_75_ladder_3 = _1069;
                                        frontier_phi_76_61_ladder_75_ladder_4 = _1068;
                                        frontier_phi_76_61_ladder_75_ladder_5 = _1436;
                                        frontier_phi_76_61_ladder_75_ladder_6 = _1066;
                                        frontier_phi_76_61_ladder_75_ladder_7 = _1065;
                                        frontier_phi_76_61_ladder_75_ladder_8 = _1064;
                                        frontier_phi_76_61_ladder_75_ladder_9 = uint(_1436 > 0.0f);
                                        frontier_phi_76_61_ladder_75_ladder_10 = _1223;
                                        frontier_phi_76_61_ladder_75_ladder_11 = _1225;
                                        frontier_phi_76_61_ladder_75_ladder_12 = _1226;
                                        frontier_phi_76_61_ladder_75_ladder_13 = _1228;
                                        frontier_phi_76_61_ladder_75_ladder_14 = _1229;
                                        frontier_phi_76_61_ladder_75_ladder_15 = _1215;
                                        frontier_phi_76_61_ladder_75_ladder_16 = _1046;
                                    }
                                    else
                                    {
                                        frontier_phi_76_61_ladder_75_ladder = _1044;
                                        frontier_phi_76_61_ladder_75_ladder_1 = _1438;
                                        frontier_phi_76_61_ladder_75_ladder_2 = _1070;
                                        frontier_phi_76_61_ladder_75_ladder_3 = _1069;
                                        frontier_phi_76_61_ladder_75_ladder_4 = _1068;
                                        frontier_phi_76_61_ladder_75_ladder_5 = _1436;
                                        frontier_phi_76_61_ladder_75_ladder_6 = _1066;
                                        frontier_phi_76_61_ladder_75_ladder_7 = _1065;
                                        frontier_phi_76_61_ladder_75_ladder_8 = _1064;
                                        frontier_phi_76_61_ladder_75_ladder_9 = _1063;
                                        frontier_phi_76_61_ladder_75_ladder_10 = _1223;
                                        frontier_phi_76_61_ladder_75_ladder_11 = _1225;
                                        frontier_phi_76_61_ladder_75_ladder_12 = _1226;
                                        frontier_phi_76_61_ladder_75_ladder_13 = _1228;
                                        frontier_phi_76_61_ladder_75_ladder_14 = _1229;
                                        frontier_phi_76_61_ladder_75_ladder_15 = _1215;
                                        frontier_phi_76_61_ladder_75_ladder_16 = _1046;
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
                            _887 = frontier_phi_76_61_ladder_1;
                            _885 = frontier_phi_76_61_ladder_2;
                            _883 = frontier_phi_76_61_ladder_3;
                            _881 = frontier_phi_76_61_ladder_4;
                            _879 = frontier_phi_76_61_ladder_5;
                            _877 = frontier_phi_76_61_ladder_6;
                            _875 = frontier_phi_76_61_ladder_7;
                            _873 = frontier_phi_76_61_ladder_8;
                            _1442 = frontier_phi_76_61_ladder_9;
                            _1445 = frontier_phi_76_61_ladder_10;
                            _1060 = frontier_phi_76_61_ladder_11;
                            _1058 = frontier_phi_76_61_ladder_12;
                            _1056 = frontier_phi_76_61_ladder_13;
                            _1054 = frontier_phi_76_61_ladder_14;
                            _1052 = frontier_phi_76_61_ladder_15;
                            _1047 = frontier_phi_76_61_ladder_16;
                            _1045 = frontier_phi_76_61_ladder;
                        }
                        else
                        {
                            bool _1314 = _1063 != 0u;
                            _887 = _1071;
                            _885 = _1070;
                            _883 = _1069;
                            _881 = _1068;
                            _879 = _1067;
                            _877 = _1314 ? _1066 : _1219;
                            _875 = _1314 ? _1065 : _1220;
                            _873 = _1314 ? _1064 : _1221;
                            _1442 = _1063;
                            _1445 = _1223;
                            _1060 = _1225;
                            _1058 = _1226;
                            _1056 = _1228;
                            _1054 = _1229;
                            _1052 = _1215;
                            _1047 = _1046;
                            _1045 = (asuint(_49_m0[5u]).y != 0u) ? _772 : _1044;
                        }
                        float frontier_phi_109_pred;
                        uint frontier_phi_109_pred_1;
                        uint frontier_phi_109_pred_2;
                        float frontier_phi_109_pred_3;
                        float frontier_phi_109_pred_4;
                        bool _1451;
                        bool _1453;
                        for (;;)
                        {
                            _1451 = _1221 < 0.0f;
                            _1453 = _1451 || ((_1219 < 0.0f) || (_1220 < 0.0f));
                            if (!_1453)
                            {
                                if (!((_1221 > 1.0f) || ((_1219 > _49_m0[51u].x) || (_1220 > _49_m0[51u].y))))
                                {
                                    frontier_phi_109_pred = _1219;
                                    frontier_phi_109_pred_1 = _1442;
                                    frontier_phi_109_pred_2 = _1445;
                                    frontier_phi_109_pred_3 = _1220;
                                    frontier_phi_109_pred_4 = _1221;
                                    break;
                                }
                            }
                            if (!_1451)
                            {
                                frontier_phi_109_pred = _1219;
                                frontier_phi_109_pred_1 = 1u;
                                frontier_phi_109_pred_2 = 4294967295u;
                                frontier_phi_109_pred_3 = _1220;
                                frontier_phi_109_pred_4 = _1221;
                                break;
                            }
                            float _1798 = (-0.0f) - _1221;
                            float _1799 = _1798 / _618;
                            frontier_phi_109_pred = (_1799 * _615) + _1219;
                            frontier_phi_109_pred_1 = 1u;
                            frontier_phi_109_pred_2 = 4294967295u;
                            frontier_phi_109_pred_3 = (_1799 * _617) + _1220;
                            frontier_phi_109_pred_4 = _1798 + _1221;
                            break;
                        }
                        _869 = frontier_phi_109_pred;
                        _871 = frontier_phi_109_pred_1;
                        _1062 = frontier_phi_109_pred_2;
                        _867 = frontier_phi_109_pred_3;
                        _865 = frontier_phi_109_pred_4;
                        if ((_863 < _208) && (int(_1062) > int(4294967295u)))
                        {
                            _1043 = _863;
                            _1044 = _1045;
                            _1046 = _1047;
                            _1048 = _865;
                            _1049 = _867;
                            _1050 = _869;
                            _1051 = _1052;
                            _1053 = _1054;
                            _1055 = _1056;
                            _1057 = _1058;
                            _1059 = _1060;
                            _1061 = _1062;
                            _1063 = _871;
                            _1064 = _873;
                            _1065 = _875;
                            _1066 = _877;
                            _1067 = _879;
                            _1068 = _881;
                            _1069 = _883;
                            _1070 = _885;
                            _1071 = _887;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    _862 = _863;
                    _864 = _865;
                    _866 = _867;
                    _868 = _869;
                    _870 = _871;
                    _872 = _873;
                    _874 = _875;
                    _876 = _877;
                    _878 = _879;
                    _880 = _881;
                    _882 = _883;
                    _884 = _885;
                    _886 = _887;
                }
                bool _888 = _862 >= _208;
                uint _889 = _888 ? 1u : _870;
                float _897 = _49_m0[51u].z * 2.0f;
                float _900 = (_897 * _578) + (-1.0f);
                float _901 = ((1.0f - (_49_m0[51u].w * _579)) * 2.0f) + (-1.0f);
                float _917 = mad(_189, _571, mad(_182, _901, _900 * _175)) + _196;
                float _918 = (mad(_186, _571, mad(_179, _901, _900 * _172)) + _193) / _917;
                float _919 = (mad(_187, _571, mad(_180, _901, _900 * _173)) + _194) / _917;
                float _920 = (mad(_188, _571, mad(_181, _901, _900 * _174)) + _195) / _917;
                float _925 = (_897 * _868) + (-1.0f);
                float _926 = ((1.0f - (_49_m0[51u].w * _866)) * 2.0f) + (-1.0f);
                float _942 = mad(_189, _864, mad(_182, _926, _925 * _175)) + _196;
                float _946 = ((mad(_186, _864, mad(_179, _926, _925 * _172)) + _193) / _942) - _918;
                float _947 = ((mad(_187, _864, mad(_180, _926, _925 * _173)) + _194) / _942) - _919;
                float _948 = ((mad(_188, _864, mad(_181, _926, _925 * _174)) + _195) / _942) - _920;
                float _980;
                uint _982;
                float _984;
                if (_862 > _208)
                {
                    _980 = 0.0f;
                    _982 = _889;
                    _984 = 0.0f;
                }
                else
                {
                    float frontier_phi_30_31_ladder;
                    uint frontier_phi_30_31_ladder_1;
                    float frontier_phi_30_31_ladder_2;
                    if ((_866 < 0.0f) || (_868 < 0.0f))
                    {
                        frontier_phi_30_31_ladder = 0.0f;
                        frontier_phi_30_31_ladder_1 = _889;
                        frontier_phi_30_31_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_30_31_ladder_37_ladder;
                        uint frontier_phi_30_31_ladder_37_ladder_1;
                        float frontier_phi_30_31_ladder_37_ladder_2;
                        if ((_864 >= 1.0f) || ((_868 > _49_m0[51u].x) || (_866 > _49_m0[51u].y)))
                        {
                            frontier_phi_30_31_ladder_37_ladder = 0.0f;
                            frontier_phi_30_31_ladder_37_ladder_1 = _889;
                            frontier_phi_30_31_ladder_37_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_30_31_ladder_37_ladder_47_ladder;
                            uint frontier_phi_30_31_ladder_37_ladder_47_ladder_1;
                            float frontier_phi_30_31_ladder_37_ladder_47_ladder_2;
                            for (;;)
                            {
                                if ((abs(_868 - _258) < (2.0f / _427)) && (abs(_866 - _259) < (2.0f / _426)))
                                {
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder = 0.0f;
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder_1 = _889;
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_92;
                                    float frontier_phi_92_pred;
                                    uint frontier_phi_92_pred_1;
                                    float frontier_phi_92_pred_2;
                                    uint _1289;
                                    uint _1290;
                                    bool _1292;
                                    for (;;)
                                    {
                                        _1289 = uint(int(_868 * _427));
                                        _1290 = uint(int(_866 * _426));
                                        _1292 = (_401 == 1u) && _532;
                                        if (!_1292)
                                        {
                                            if (!(dot(float3(_946, _947, _948), float3(_946, _947, _948)) < _49_m0[4u].w))
                                            {
                                                ladder_phi_92 = false;
                                                frontier_phi_92_pred = 0.0f;
                                                frontier_phi_92_pred_1 = _889;
                                                frontier_phi_92_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1415 = _24[22u].Load(int3(uint2(_1289, _1290), 0u));
                                        uint _1417 = _1415.x;
                                        float _1763;
                                        float _1764;
                                        float _1765;
                                        if (_1417 == 0u)
                                        {
                                            uint4 _1566 = _24[1u].Load(int3(uint2(_1289, _1290), 0u));
                                            uint _1568 = _1566.x;
                                            float _1576 = (float((_1568 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1577 = (float(_1568 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1581 = (1.0f - abs(_1576)) - abs(_1577);
                                            float _1583 = clamp((-0.0f) - _1581, 0.0f, 1.0f);
                                            float _1584 = (-0.0f) - _1583;
                                            _1763 = ((_1576 >= 0.0f) ? _1584 : _1583) + _1576;
                                            _1764 = ((_1577 >= 0.0f) ? _1584 : _1583) + _1577;
                                            _1765 = _1581;
                                        }
                                        else
                                        {
                                            float _1598 = (float((_1417 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1599 = (float(_1417 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1603 = (1.0f - abs(_1598)) - abs(_1599);
                                            float _1605 = clamp((-0.0f) - _1603, 0.0f, 1.0f);
                                            float _1606 = (-0.0f) - _1605;
                                            _1763 = ((_1598 >= 0.0f) ? _1606 : _1605) + _1598;
                                            _1764 = ((_1599 >= 0.0f) ? _1606 : _1605) + _1599;
                                            _1765 = _1603;
                                        }
                                        float _1769 = rsqrt(dot(float3(_1763, _1764, _1765), float3(_1763, _1764, _1765)));
                                        if (dot(float3(_1769 * _1763, _1769 * _1764, _1769 * _1765), float3(_946, _947, _948)) > 0.0f)
                                        {
                                            ladder_phi_92 = true;
                                            frontier_phi_92_pred = 0.0f;
                                            frontier_phi_92_pred_1 = _889;
                                            frontier_phi_92_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_92 = false;
                                            frontier_phi_92_pred = 0.0f;
                                            frontier_phi_92_pred_1 = _889;
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
                                    float _1617 = _49_m0[51u].z * _868;
                                    float _1618 = _49_m0[51u].w * _866;
                                    float _1620 = (_426 / _427) * 0.0500000007450580596923828125f;
                                    float _1625 = clamp(_1617 / _1620, 0.0f, 1.0f);
                                    float _1626 = clamp(_1618 * 20.0f, 0.0f, 1.0f);
                                    float _1638 = clamp(((_1617 + (-1.0f)) + _1620) / _1620, 0.0f, 1.0f);
                                    float _1639 = clamp((_1618 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1650 = _1625 * _1626;
                                    precise float _1651 = _1650 * _1650;
                                    float _1655 = ((((3.0f - (_1626 * 2.0f)) * (3.0f - (_1625 * 2.0f))) * _1651) * (1.0f - ((_1638 * _1638) * (3.0f - (_1638 * 2.0f))))) * (1.0f - ((_1639 * _1639) * (3.0f - (_1639 * 2.0f))));
                                    bool _1658 = (_889 != 0u) || (_1655 >= 1.0f);
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder = _1658 ? 0.0f : _1655;
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder_1 = _1658 ? _889 : 1u;
                                    frontier_phi_30_31_ladder_37_ladder_47_ladder_2 = _1655 * float(_471 > 0.0f);
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
                    _980 = frontier_phi_30_31_ladder;
                    _982 = frontier_phi_30_31_ladder_1;
                    _984 = frontier_phi_30_31_ladder_2;
                }
                uint _1122;
                float _1123;
                float _1125;
                float _1127;
                float _1129;
                float _1132;
                float _1133;
                float _1134;
                float _1136;
                float _1025;
                float _1028;
                float _1031;
                float _1034;
                float _1038;
                float _1039;
                for (;;)
                {
                    _1025 = ((((exp2(log2(clamp((sqrt(((_919 * _919) + (_918 * _918)) + (_920 * _920)) - _44_m0[121u].y) * _44_m0[121u].z, 0.0f, 1.0f)) * _44_m0[121u].w) * _409) * exp2(log2(clamp((_919 - _44_m0[122u].x) * _44_m0[122u].y, 0.0f, 1.0f)) * _44_m0[122u].z)) * (1.0f - clamp(_397, 0.0f, 1.0f))) * (max(_44_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    _1028 = mad(_166, _528, mad(_160, _527, _526 * _154));
                    _1031 = mad(_167, _528, mad(_161, _527, _526 * _155));
                    _1034 = mad(_168, _528, mad(_162, _527, _526 * _156));
                    bool _1037 = (_982 != 0u) || (_984 < 1.0f);
                    _1038 = _1037 ? 0.0f : 1.0f;
                    _1039 = _1037 ? 0.0f : 0.5f;
                    if (_1037)
                    {
                        bool _1117 = _401 == 1u;
                        uint4 _1121 = asuint(_49_m0[60u]);
                        if (_1117)
                        {
                            if (int(_407) < int(1u))
                            {
                                if (_1121.x == 0u)
                                {
                                    _1122 = 0u;
                                    _1123 = 0.0f;
                                    _1125 = 0.0f;
                                    _1127 = 0.0f;
                                    _1129 = 9899999600270360182784.0f;
                                    _1132 = 0.0f;
                                    _1133 = 0.0f;
                                    _1134 = 0.0f;
                                    _1136 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1121.y == 0u)
                                {
                                    _1122 = 0u;
                                    _1123 = 0.0f;
                                    _1125 = 0.0f;
                                    _1127 = 0.0f;
                                    _1129 = 9899999600270360182784.0f;
                                    _1132 = 0.0f;
                                    _1133 = 0.0f;
                                    _1134 = 0.0f;
                                    _1136 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1121.z == 0u)
                            {
                                _1122 = 0u;
                                _1123 = 0.0f;
                                _1125 = 0.0f;
                                _1127 = 0.0f;
                                _1129 = 9899999600270360182784.0f;
                                _1132 = 0.0f;
                                _1133 = 0.0f;
                                _1134 = 0.0f;
                                _1136 = 0.0f;
                                break;
                            }
                        }
                        if (_980 > 0.0f)
                        {
                            _1122 = 0u;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                            _1129 = 9899999600270360182784.0f;
                            _1132 = 0.0f;
                            _1133 = 1.0f;
                            _1134 = 1000.0f;
                            _1136 = 0.5f;
                            break;
                        }
                        if ((_872 <= 0.0f) || ((_874 <= 0.0f) || (_876 <= 0.0f)))
                        {
                            _1122 = 0u;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                            _1129 = 9899999600270360182784.0f;
                            _1132 = 0.0f;
                            _1133 = 1.0f;
                            _1134 = 1000.0f;
                            _1136 = 0.5f;
                            break;
                        }
                        if ((_872 >= 1.0f) || ((_876 >= _49_m0[51u].x) || (_874 >= _49_m0[51u].y)))
                        {
                            _1122 = 0u;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                            _1129 = 9899999600270360182784.0f;
                            _1132 = 0.0f;
                            _1133 = 1.0f;
                            _1134 = 1000.0f;
                            _1136 = 0.5f;
                            break;
                        }
                        uint _1981;
                        uint _1983;
                        uint _1686;
                        uint _1687;
                        bool _1693;
                        for (;;)
                        {
                            _1686 = uint(clamp(_884, 0.0f, 1.0f) * _201);
                            _1687 = uint(clamp(_882, 0.0f, 1.0f) * _203);
                            _1693 = _20[21u].Load(int3(uint2(_1686, _1687), 0u)).x > 0.0f;
                            if (_1693)
                            {
                                uint _1856 = _24[23u].Load(int3(uint2(_1686, _1687), 0u)).y + 4294967295u;
                                _1981 = (uint(int(_1856) >> int(31u)) & 3u) + 1u;
                                _1983 = (int(_1856) < int(0u)) ? 0u : _1856;
                                break;
                            }
                            else
                            {
                                uint4 _1864 = _24[2u].Load(int3(uint2(_1686, _1687), 0u));
                                uint _1867 = _1864.w;
                                uint4 _1872 = _24[15u].Load(int3(uint2(_1686, _1687), 0u));
                                uint _1874 = _1872.y;
                                uint _1880 = ((_1874 & 64u) != 0u) ? uint((_1874 & 4294967167u) != 66u) : 4294967295u;
                                uint _1881 = _1867 & 128u;
                                uint _1883 = (_1881 != 0u) ? 1u : ((_1864.x << 7u) | _1867);
                                uint4 _1886 = _16.Load(_1883 * 4u);
                                uint _1887 = _1886.x;
                                uint _1894 = ((_1887 & 1u) != 0u) ? 0u : 18u;
                                uint _1896 = uint(min(int(uint(max(int(_1880), int(0u)))), int(1u)));
                                uint _1994;
                                if (_1881 == 0u)
                                {
                                    _1994 = (((_1887 & 2097152u) != 0u) && (_1880 == _1896)) ? (_1894 | 128u) : _1894;
                                }
                                else
                                {
                                    _1994 = _1867;
                                }
                                uint _1995 = _16.Load((_1883 * 4u) + 1u).x & 512u;
                                bool _1998 = (_1994 & 144u) == 0u;
                                if (_1995 == 0u)
                                {
                                    if (_1998 || ((_1887 & 1u) != 0u))
                                    {
                                        _1981 = 0u;
                                        _1983 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_1998)
                                    {
                                        _1981 = 0u;
                                        _1983 = 0u;
                                        break;
                                    }
                                }
                                bool _2094 = ((_1994 & 128u) | _1995) != 0u;
                                uint _1982;
                                if (_2094)
                                {
                                    _1982 = 1u;
                                }
                                else
                                {
                                    _1982 = (((_1887 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_1887 & 268435472u) == 16u) && (((_1887 & 33554432u) == 0u) || _2094))
                                {
                                    _1981 = 2u;
                                    _1983 = 0u;
                                    break;
                                }
                                _1981 = _1982;
                                _1983 = (_1982 == 1u) ? _1896 : 0u;
                                break;
                            }
                        }
                        if ((_401 != _1981) || (_407 != _1983))
                        {
                            _1122 = 0u;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                            _1129 = 9899999600270360182784.0f;
                            _1132 = 0.0f;
                            _1133 = 1.0f;
                            _1134 = 1000.0f;
                            _1136 = 0.5f;
                            break;
                        }
                        float _2020 = _876 * 2.0f;
                        float _2023 = (_49_m0[51u].z * _2020) + (-1.0f);
                        float _2024 = ((1.0f - (_49_m0[51u].w * _874)) * 2.0f) + (-1.0f);
                        float _2040 = mad(_189, _872, mad(_182, _2024, _2023 * _175)) + _196;
                        float _2041 = (mad(_186, _872, mad(_179, _2024, _2023 * _172)) + _193) / _2040;
                        float _2042 = (mad(_187, _872, mad(_180, _2024, _2023 * _173)) + _194) / _2040;
                        float _2043 = (mad(_188, _872, mad(_181, _2024, _2023 * _174)) + _195) / _2040;
                        if (sqrt(((_2042 * _2042) + (_2041 * _2041)) + (_2043 * _2043)) > _49_m0[58u].w)
                        {
                            _1122 = 0u;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                            _1129 = 9899999600270360182784.0f;
                            _1132 = 0.0f;
                            _1133 = 0.0f;
                            _1134 = 1000.0f;
                            _1136 = 0.5f;
                            break;
                        }
                        float _2074 = _2041 - _918;
                        float _2075 = _2042 - _919;
                        float _2076 = _2043 - _920;
                        float _2082 = sqrt(((_2075 * _2075) + (_2074 * _2074)) + (_2076 * _2076));
                        float _2090 = min(_49_m0[59u].y, max(0.0f, _2082 + (-0.001000000047497451305389404296875f)));
                        float _2159;
                        if (_1117)
                        {
                            _2159 = min(_49_m0[59u].x, _49_m0[4u].z + _2090);
                        }
                        else
                        {
                            _2159 = _49_m0[59u].x;
                        }
                        float _2160 = _2159 - _2082;
                        if (!(_2160 > 0.0f))
                        {
                            _1122 = 0u;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                            _1129 = 9899999600270360182784.0f;
                            _1132 = 1.0f;
                            _1133 = 1.0f;
                            _1134 = 0.0f;
                            _1136 = 0.5f;
                            break;
                        }
                        float _2183 = _2041 - (_2090 * _1028);
                        float _2184 = _2042 - (_2090 * _1031);
                        float _2185 = _2043 - (_2090 * _1034);
                        RayDesc _2ident = {float3(mad(_2185, _49_m0[46u].z, mad(_2184, _49_m0[46u].y, _49_m0[46u].x * _2183)) + _49_m0[46u].w, mad(_2185, _49_m0[47u].z, mad(_2184, _49_m0[47u].y, _49_m0[47u].x * _2183)) + _49_m0[47u].w, mad(_2185, _49_m0[48u].z, mad(_2184, _49_m0[48u].y, _49_m0[48u].x * _2183)) + _49_m0[48u].w), 0.0f, float3(mad(_1034, _49_m0[46u].z, mad(_1031, _49_m0[46u].y, _49_m0[46u].x * _1028)), mad(_1034, _49_m0[47u].z, mad(_1031, _49_m0[47u].y, _49_m0[47u].x * _1028)), mad(_1034, _49_m0[48u].z, mad(_1031, _49_m0[48u].y, _49_m0[48u].x * _1028))), _2160};
                        _2188.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2235 = _2188.Proceed();
                        uint _2236 = _2188.CommittedStatus();
                        if (!(_2236 == 1u))
                        {
                            _1122 = 0u;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                            _1129 = 9899999600270360182784.0f;
                            _1132 = 1.0f;
                            _1133 = 0.0f;
                            _1134 = 0.0f;
                            _1136 = 0.5f;
                            break;
                        }
                        float _2305 = _2188.CommittedRayT();
                        if (!((_2305 < _2160) && (_2305 > 0.0f)))
                        {
                            _1122 = 0u;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                            _1129 = 9899999600270360182784.0f;
                            _1132 = 1.0f;
                            _1133 = 0.0f;
                            _1134 = 0.0f;
                            _1136 = 0.5f;
                            break;
                        }
                        float _2356 = (_49_m0[51u].z * _2020) + (-1.0f);
                        float _2357 = ((1.0f - (_49_m0[51u].w * _874)) * 2.0f) + (-1.0f);
                        float _2373 = mad(_143, _872, mad(_136, _2357, _2356 * _129)) + _150;
                        float _2377 = _2305 - _2090;
                        float _2381 = ((mad(_140, _872, mad(_133, _2357, _2356 * _126)) + _147) / _2373) + (_2377 * _526);
                        float _2382 = ((mad(_141, _872, mad(_134, _2357, _2356 * _127)) + _148) / _2373) + (_2377 * _527);
                        float _2383 = ((mad(_142, _872, mad(_135, _2357, _2356 * _128)) + _149) / _2373) + (_2377 * _528);
                        float _2395 = mad(_115, _2383, mad(_108, _2382, _2381 * _101)) + _122;
                        float _2405 = (_49_m0[51u].z * _49_m0[51u].x) * ((((mad(_112, _2383, mad(_105, _2382, _2381 * _98)) + _119) / _2395) * 0.5f) + 0.5f);
                        float _2407 = (_49_m0[51u].w * _49_m0[51u].y) * (0.5f - (((mad(_113, _2383, mad(_106, _2382, _2381 * _99)) + _120) / _2395) * 0.5f));
                        float _2409 = (_203 / _201) * 0.0500000007450580596923828125f;
                        float _2412 = clamp(_2405 / _2409, 0.0f, 1.0f);
                        float _2413 = clamp(_2407 * 20.0f, 0.0f, 1.0f);
                        float _2423 = clamp(((_2409 + (-1.0f)) + _2405) / _2409, 0.0f, 1.0f);
                        float _2424 = clamp((_2407 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2435 = _2412 * _2413;
                        precise float _2436 = _2435 * _2435;
                        if ((((((3.0f - (_2413 * 2.0f)) * (3.0f - (_2412 * 2.0f))) * _2436) * (1.0f - ((_2423 * _2423) * (3.0f - (_2423 * 2.0f))))) * (1.0f - ((_2424 * _2424) * (3.0f - (_2424 * 2.0f))))) < 1.0f)
                        {
                            _1122 = 0u;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                            _1127 = 0.0f;
                            _1129 = 9899999600270360182784.0f;
                            _1132 = _1038;
                            _1133 = _1038;
                            _1134 = 0.0f;
                            _1136 = _1039;
                            break;
                        }
                        _1122 = 1u;
                        _1123 = _49_m0[1u].w;
                        _1125 = _49_m0[2u].x;
                        _1127 = _49_m0[2u].y;
                        _1129 = (_2082 - _2090) + _2305;
                        _1132 = 0.0f;
                        _1133 = 1.0f;
                        _1134 = 0.0f;
                        _1136 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1122 = 0u;
                        _1123 = 0.0f;
                        _1125 = 0.0f;
                        _1127 = 0.0f;
                        _1129 = 9899999600270360182784.0f;
                        _1132 = 1.0f;
                        _1133 = 1.0f;
                        _1134 = 0.0f;
                        _1136 = 0.5f;
                        break;
                    }
                }
                bool _1137 = _401 == 1u;
                float _1173;
                if (_1137)
                {
                    float _1172 = _49_m0[4u].z - _49_m0[4u].y;
                    float _1359;
                    if (_1122 == 0u)
                    {
                        float _1258 = clamp((sqrt(((_947 * _947) + (_946 * _946)) + (_948 * _948)) - _49_m0[4u].y) / _1172, 0.0f, 1.0f);
                        _1359 = (_1258 * _1258) * (3.0f - (_1258 * 2.0f));
                    }
                    else
                    {
                        float _1265 = clamp((_1129 - _49_m0[4u].y) / _1172, 0.0f, 1.0f);
                        _1359 = (_1265 * _1265) * (3.0f - (_1265 * 2.0f));
                    }
                    _1173 = _49_m0[61u].z * (1.0f - _1359);
                }
                else
                {
                    _1173 = 1.0f;
                }
                float _1175 = _1173 * _984;
                bool _1176 = _401 != 1u;
                float _1694;
                float _1696;
                float _1698;
                float _1700;
                if (_1175 == 0.0f)
                {
                    float _1497;
                    float _1500;
                    float _1503;
                    float _1506;
                    if (_1122 == 0u)
                    {
                        float frontier_phi_83_68_ladder;
                        float frontier_phi_83_68_ladder_1;
                        float frontier_phi_83_68_ladder_2;
                        float frontier_phi_83_68_ladder_3;
                        if (_1176)
                        {
                            float _1478 = _1031 * _1025;
                            float _1482 = rsqrt(dot(float3(_1028, _1478, _1034), float3(_1028, _1478, _1034)));
                            float4 _1492 = _28[4u].SampleLevel(_58, float3(_1482 * _1028, _1482 * _1478, _1482 * _1034), 0.0f);
                            frontier_phi_83_68_ladder = 1.0f;
                            frontier_phi_83_68_ladder_1 = _1492.z;
                            frontier_phi_83_68_ladder_2 = _1492.y;
                            frontier_phi_83_68_ladder_3 = _1492.x;
                        }
                        else
                        {
                            frontier_phi_83_68_ladder = 0.0f;
                            frontier_phi_83_68_ladder_1 = 0.0f;
                            frontier_phi_83_68_ladder_2 = 0.0f;
                            frontier_phi_83_68_ladder_3 = 0.0f;
                        }
                        _1497 = frontier_phi_83_68_ladder_3;
                        _1500 = frontier_phi_83_68_ladder_2;
                        _1503 = frontier_phi_83_68_ladder_1;
                        _1506 = frontier_phi_83_68_ladder;
                    }
                    else
                    {
                        float frontier_phi_83_69_ladder;
                        float frontier_phi_83_69_ladder_1;
                        float frontier_phi_83_69_ladder_2;
                        float frontier_phi_83_69_ladder_3;
                        if (_1176)
                        {
                            float _1511 = _1031 * _1025;
                            float _1515 = rsqrt(dot(float3(_1028, _1511, _1034), float3(_1028, _1511, _1034)));
                            float4 _1523 = _28[4u].SampleLevel(_58, float3(_1515 * _1028, _1515 * _1511, _1515 * _1034), 0.0f);
                            float _1525 = _1523.x;
                            float _1526 = _1523.y;
                            float _1527 = _1523.z;
                            frontier_phi_83_69_ladder = 1.0f;
                            frontier_phi_83_69_ladder_1 = ((_1127 - _1527) * _1173) + _1527;
                            frontier_phi_83_69_ladder_2 = ((_1125 - _1526) * _1173) + _1526;
                            frontier_phi_83_69_ladder_3 = ((_1123 - _1525) * _1173) + _1525;
                        }
                        else
                        {
                            frontier_phi_83_69_ladder = _1173;
                            frontier_phi_83_69_ladder_1 = _1173 * _1127;
                            frontier_phi_83_69_ladder_2 = _1173 * _1125;
                            frontier_phi_83_69_ladder_3 = _1173 * _1123;
                        }
                        _1497 = frontier_phi_83_69_ladder_3;
                        _1500 = frontier_phi_83_69_ladder_2;
                        _1503 = frontier_phi_83_69_ladder_1;
                        _1506 = frontier_phi_83_69_ladder;
                    }
                    _1694 = _1497 * _471;
                    _1696 = _1500 * _471;
                    _1698 = _1503 * _471;
                    _1700 = _1506 * _471;
                }
                else
                {
                    uint4 _1273 = asuint(_54_m0[0u]);
                    float _1545;
                    float _1547;
                    float _1551;
                    float _1554;
                    if (_12.Load(int3(uint2(uint(float(_1273.x) * _868), uint(float(_1273.y) * _866)), 0u)).x > 0.0f)
                    {
                        uint _1366_dummy_parameter;
                        uint2 _1366 = spvTextureSize(_14, 0u, _1366_dummy_parameter);
                        float4 _1375 = _14.Load(int3(uint2(uint(float(_1366.x) * _868), uint(float(_1366.y) * _866)), 0u));
                        float4 _1399 = _13.SampleLevel(_57, float2(((_1375.x * 0.5f) * _49_m0[52u].z) + (_49_m0[52u].x * _868), ((_1375.y * (-0.5f)) * _49_m0[52u].w) + (_49_m0[52u].y * _866)), 0.0f);
                        float _1534;
                        float _1536;
                        float _1538;
                        if (_880 == 0u)
                        {
                            _1534 = _49_m0[54u].x * _1399.x;
                            _1536 = _49_m0[54u].x * _1399.y;
                            _1538 = _49_m0[54u].x * _1399.z;
                        }
                        else
                        {
                            _1534 = _49_m0[1u].w;
                            _1536 = _49_m0[2u].x;
                            _1538 = _49_m0[2u].y;
                        }
                        float frontier_phi_88_86_ladder;
                        float frontier_phi_88_86_ladder_1;
                        float frontier_phi_88_86_ladder_2;
                        float frontier_phi_88_86_ladder_3;
                        for (;;)
                        {
                            if (_878 > 0.0f)
                            {
                                float _1550;
                                float _1553;
                                float _1556;
                                if (_1137)
                                {
                                    _1550 = 0.0f;
                                    _1553 = 0.0f;
                                    _1556 = 0.0f;
                                }
                                else
                                {
                                    if (!((_401 != 4u) || (_886 != 0u)))
                                    {
                                        frontier_phi_88_86_ladder = _1538;
                                        frontier_phi_88_86_ladder_1 = _1536;
                                        frontier_phi_88_86_ladder_2 = _1534;
                                        frontier_phi_88_86_ladder_3 = _1175;
                                        break;
                                    }
                                    _1550 = _1534;
                                    _1553 = _1536;
                                    _1556 = _1538;
                                }
                                frontier_phi_88_86_ladder = _1556;
                                frontier_phi_88_86_ladder_1 = _1553;
                                frontier_phi_88_86_ladder_2 = _1550;
                                frontier_phi_88_86_ladder_3 = (1.0f - exp2(log2(_878) * _49_m0[4u].x)) * _1175;
                                break;
                            }
                            else
                            {
                                frontier_phi_88_86_ladder = _1538;
                                frontier_phi_88_86_ladder_1 = _1536;
                                frontier_phi_88_86_ladder_2 = _1534;
                                frontier_phi_88_86_ladder_3 = _1175;
                                break;
                            }
                        }
                        _1545 = frontier_phi_88_86_ladder_3;
                        _1547 = frontier_phi_88_86_ladder_2;
                        _1551 = frontier_phi_88_86_ladder_1;
                        _1554 = frontier_phi_88_86_ladder;
                    }
                    else
                    {
                        float frontier_phi_88_71_ladder;
                        float frontier_phi_88_71_ladder_1;
                        float frontier_phi_88_71_ladder_2;
                        float frontier_phi_88_71_ladder_3;
                        if (_888)
                        {
                            frontier_phi_88_71_ladder = _1549;
                            frontier_phi_88_71_ladder_1 = _1549;
                            frontier_phi_88_71_ladder_2 = _1549;
                            frontier_phi_88_71_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float4 _1562 = _28[7u].SampleLevel(_58, float3(_1028, _1031, _1034), 0.0f);
                            frontier_phi_88_71_ladder = _1562.z;
                            frontier_phi_88_71_ladder_1 = _1562.y;
                            frontier_phi_88_71_ladder_2 = _1562.x;
                            frontier_phi_88_71_ladder_3 = _1175;
                        }
                        _1545 = frontier_phi_88_71_ladder_3;
                        _1547 = frontier_phi_88_71_ladder_2;
                        _1551 = frontier_phi_88_71_ladder_1;
                        _1554 = frontier_phi_88_71_ladder;
                    }
                    float _1748;
                    float _1749;
                    float _1751;
                    float _1753;
                    if (_1122 == 0u)
                    {
                        _1748 = _1545;
                        _1749 = _1547;
                        _1751 = _1551;
                        _1753 = _1554;
                    }
                    else
                    {
                        _1748 = _1173;
                        _1749 = ((_1547 - _1123) * _980) + _1123;
                        _1751 = ((_1551 - _1125) * _980) + _1125;
                        _1753 = ((_1554 - _1127) * _980) + _1127;
                    }
                    float _1938;
                    float _1939;
                    float _1940;
                    float _1941;
                    if (_1176 && (_1748 < 1.0f))
                    {
                        float _1912 = _1031 * _1025;
                        float _1916 = rsqrt(dot(float3(_1028, _1912, _1034), float3(_1028, _1912, _1034)));
                        float4 _1924 = _28[4u].SampleLevel(_58, float3(_1916 * _1028, _1916 * _1912, _1916 * _1034), 0.0f);
                        float _1926 = _1924.x;
                        float _1927 = _1924.y;
                        float _1928 = _1924.z;
                        _1938 = 1.0f;
                        _1939 = ((_1749 - _1926) * _1748) + _1926;
                        _1940 = ((_1751 - _1927) * _1748) + _1927;
                        _1941 = ((_1753 - _1928) * _1748) + _1928;
                    }
                    else
                    {
                        _1938 = _1748;
                        _1939 = _1749;
                        _1940 = _1751;
                        _1941 = _1753;
                    }
                    float _1701 = _1938 * _471;
                    _1694 = _1939 * _1701;
                    _1696 = _1940 * _1701;
                    _1698 = _1941 * _1701;
                    _1700 = _1701;
                }
                float _1705 = _49_m0[58u].z * _1136;
                float _1727 = max(_49_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _1729 = _1727 * ((_1705 * ((_1132 * 1000.0f) - _1694)) + _1694);
                float _1730 = _1727 * ((_1705 * ((_1133 * 1000.0f) - _1696)) + _1696);
                float _1731 = _1727 * ((_1705 * (_1134 - _1698)) + _1698);
                float _1737 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_1729, max(_1730, _1731)) + 1.0f);
                float _1741 = min(_1737 * _1729, 0.996078431606292724609375f);
                float _1743 = min(_1737 * _1730, 0.996078431606292724609375f);
                float _1744 = min(_1737 * _1731, 0.996078431606292724609375f);
                _35[uint2(_225, _228)] = float4(_1741, _1743, _1744, _1700);
                if (_232)
                {
                    _35[uint2(_225 + 1u, _228)] = float4(_1741, _1743, _1744, _1700);
                }
                if (_235)
                {
                    _35[uint2(_225, _228 + 1u)] = float4(_1741, _1743, _1744, _1700);
                }
                if (_236)
                {
                    _35[uint2(_225 + 1u, _228 + 1u)] = float4(_1741, _1743, _1744, _1700);
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
