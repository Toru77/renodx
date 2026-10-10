static float _1737;
static uint _2886;
static float _2887;
static float _2888;
static float _2889;
static float _2890;
static float _2891;
static float _2892;
static uint _2893;
static uint _2894;
static float _2895;
static float _2896;
static float _2897;
static float _2898;
static float _2899;
static float _2905;
static uint _2906;
static float _2907;
static uint _2909;
static uint _2910;
static float _2915;

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

static RayQuery<RAY_FLAG_NONE> _2314;

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
                    float _1102;
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
                    if (_437 == 0u)
                    {
                        if (!((_428 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _818 = asfloat(_17.Load((_384 * 115u) + 33u).x);
                        uint4 _826 = _24[2u].Load(int3(uint2(_265, _266), 0u));
                        uint _828 = _826.y;
                        uint _829 = _427 & 128u;
                        uint _1090;
                        uint _1091;
                        uint _1092;
                        uint _1093;
                        if (_829 == 0u)
                        {
                            _1090 = uint(((_428 & 817889384u) | (_380 & 576u)) != 0u) | (((_428 >> 19u) & 1u) ^ 1u);
                            _1091 = uint(((_428 & 17825808u) | (_380 & 520u)) != 0u);
                            _1092 = uint(((_428 & 46137344u) | (_380 & 2564u)) != 0u);
                            _1093 = 0u;
                        }
                        else
                        {
                            _1090 = 1u;
                            _1091 = _427 & 1u;
                            _1092 = 1u;
                            _1093 = 1u;
                        }
                        precise float _1097 = float(_828 & 127u) * 0.0078740157186985015869140625f;
                        bool _1101 = (_428 & 4194304u) == 0u;
                        float _1229;
                        if (_1101)
                        {
                            _1229 = _1097;
                        }
                        else
                        {
                            _1229 = float(_828 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1379;
                        if ((_428 & 134217728u) == 0u)
                        {
                            uint frontier_phi_72_56_ladder;
                            if ((_829 != 0u) || ((_428 & 17825792u) == 1048576u))
                            {
                                frontier_phi_72_56_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_72_56_ladder = _1091;
                            }
                            _1379 = frontier_phi_72_56_ladder;
                        }
                        else
                        {
                            _1379 = _1091;
                        }
                        uint4 _1382 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _1384 = _1382.x;
                        float _1601;
                        float _1603;
                        float _1605;
                        if (_1090 == 0u)
                        {
                            _1601 = 0.0f;
                            _1603 = 0.0f;
                            _1605 = 0.0f;
                        }
                        else
                        {
                            float4 _1610 = _20[8u].Load(int3(uint2(_265, _266), 0u));
                            _1601 = _1610.x;
                            _1603 = _1610.y;
                            _1605 = _1610.z;
                        }
                        uint _1768;
                        if (_1379 == 0u)
                        {
                            _1768 = 0u;
                        }
                        else
                        {
                            _1768 = _24[9u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        uint _1823;
                        if (_1092 == 0u)
                        {
                            _1823 = 0u;
                        }
                        else
                        {
                            _1823 = _24[10u].Load(int3(uint2(_265, _266), 0u)).x;
                        }
                        float _1833 = (float((_1384 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1834 = (float(_1384 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1838 = (1.0f - abs(_1833)) - abs(_1834);
                        float _1840 = clamp((-0.0f) - _1838, 0.0f, 1.0f);
                        float _1841 = (-0.0f) - _1840;
                        float _1846 = ((_1833 >= 0.0f) ? _1841 : _1840) + _1833;
                        float _1847 = ((_1834 >= 0.0f) ? _1841 : _1840) + _1834;
                        float _1851 = rsqrt(dot(float3(_1846, _1847, _1838), float3(_1846, _1847, _1838)));
                        float _1852 = _1846 * _1851;
                        float _1853 = _1847 * _1851;
                        float _1854 = _1851 * _1838;
                        float _1103 = float(_1384 & 255u);
                        float _1858 = ((_428 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _2073;
                        float _2074;
                        float _2075;
                        float _2076;
                        uint _2077;
                        if ((_380 & 64u) == 0u)
                        {
                            float frontier_phi_138_130_ladder;
                            float frontier_phi_138_130_ladder_1;
                            float frontier_phi_138_130_ladder_2;
                            float frontier_phi_138_130_ladder_3;
                            uint frontier_phi_138_130_ladder_4;
                            if ((_428 & 276824064u) == 0u)
                            {
                                frontier_phi_138_130_ladder = 0.0f;
                                frontier_phi_138_130_ladder_1 = ((_428 & 8u) != 0u) ? _1603 : _1858;
                                frontier_phi_138_130_ladder_2 = 0.0f;
                                frontier_phi_138_130_ladder_3 = 0.0f;
                                frontier_phi_138_130_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_138_130_ladder = 0.0f;
                                frontier_phi_138_130_ladder_1 = _1858;
                                frontier_phi_138_130_ladder_2 = 0.0f;
                                frontier_phi_138_130_ladder_3 = 0.0f;
                                frontier_phi_138_130_ladder_4 = 0u;
                            }
                            _2073 = frontier_phi_138_130_ladder_1;
                            _2074 = frontier_phi_138_130_ladder_2;
                            _2075 = frontier_phi_138_130_ladder;
                            _2076 = frontier_phi_138_130_ladder_3;
                            _2077 = frontier_phi_138_130_ladder_4;
                        }
                        else
                        {
                            float _1977 = (_1603 * 2.0f) + (-1.0f);
                            float _1978 = (_1605 * 2.0f) + (-1.0f);
                            float _1982 = (1.0f - abs(_1977)) - abs(_1978);
                            float _1984 = clamp((-0.0f) - _1982, 0.0f, 1.0f);
                            float _1985 = (-0.0f) - _1984;
                            float _1990 = ((_1977 >= 0.0f) ? _1985 : _1984) + _1977;
                            float _1991 = ((_1978 >= 0.0f) ? _1985 : _1984) + _1978;
                            float _1995 = rsqrt(dot(float3(_1990, _1991, _1982), float3(_1990, _1991, _1982)));
                            _2073 = floor(round(_1601 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _2074 = _1990 * _1995;
                            _2075 = _1991 * _1995;
                            _2076 = _1995 * _1982;
                            _2077 = 1u;
                        }
                        float _1111;
                        if ((_428 & 32768u) == 0u)
                        {
                            _1111 = _2073;
                        }
                        else
                        {
                            float frontier_phi_144_145_ladder;
                            if (_17.Load((_384 * 115u) + 36u).x == 0u)
                            {
                                float _2271 = clamp((_1103 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _818;
                                frontier_phi_144_145_ladder = ((_428 & 131072u) != 0u) ? _2271 : ((((clamp((1.21000003814697265625f / (exp2((_1229 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_384 * 115u) + 32u).x)) + 1.0f) * _2271);
                            }
                            else
                            {
                                frontier_phi_144_145_ladder = _818;
                            }
                            _1111 = frontier_phi_144_145_ladder;
                        }
                        uint _2138 = _427 & 1u;
                        float _2214;
                        float _2216;
                        float _2218;
                        uint _2220;
                        if (((_428 & 16u) == 0u) || (((_2138 | (_380 & 8u)) | (_428 & 16777216u)) != 0u))
                        {
                            _2214 = _2074;
                            _2216 = _2075;
                            _2218 = _2076;
                            _2220 = _2077;
                        }
                        else
                        {
                            float _2230 = (float(_1768 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2231 = (float(_1768 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2235 = (1.0f - abs(_2230)) - abs(_2231);
                            float _2237 = clamp((-0.0f) - _2235, 0.0f, 1.0f);
                            float _2238 = (-0.0f) - _2237;
                            float _2243 = ((_2230 >= 0.0f) ? _2238 : _2237) + _2230;
                            float _2244 = ((_2231 >= 0.0f) ? _2238 : _2237) + _2231;
                            float _2248 = rsqrt(dot(float3(_2243, _2244, _2235), float3(_2243, _2244, _2235)));
                            _2214 = _2243 * _2248;
                            _2216 = _2244 * _2248;
                            _2218 = _2248 * _2235;
                            _2220 = 1u;
                        }
                        float _1105;
                        float _1107;
                        float _1109;
                        if (_2138 == 0u)
                        {
                            float frontier_phi_159_158_ladder;
                            float frontier_phi_159_158_ladder_1;
                            float frontier_phi_159_158_ladder_2;
                            if (((_427 & 64u) == 0u) && (_1093 != 0u))
                            {
                                float2 _2380 = spvUnpackHalf2x16((_1823 >> 17u) & 32736u);
                                float _2381 = _2380.x;
                                float _2384 = (spvUnpackHalf2x16((_1823 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2385 = (spvUnpackHalf2x16((_1823 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2389 = (1.0f - abs(_2384)) - abs(_2385);
                                float _2391 = clamp((-0.0f) - _2389, 0.0f, 1.0f);
                                float _2392 = (-0.0f) - _2391;
                                float _2397 = ((_2384 >= 0.0f) ? _2392 : _2391) + _2384;
                                float _2398 = ((_2385 >= 0.0f) ? _2392 : _2391) + _2385;
                                float _2402 = rsqrt(dot(float3(_2397, _2398, _2389), float3(_2397, _2398, _2389)));
                                float _2412 = (((_2397 * _2402) - _1852) * _2381) + _1852;
                                float _2413 = (((_2398 * _2402) - _1853) * _2381) + _1853;
                                float _2414 = (((_2402 * _2389) - _1854) * _2381) + _1854;
                                float _2418 = rsqrt(dot(float3(_2412, _2413, _2414), float3(_2412, _2413, _2414)));
                                frontier_phi_159_158_ladder = _2412 * _2418;
                                frontier_phi_159_158_ladder_1 = _2413 * _2418;
                                frontier_phi_159_158_ladder_2 = _2414 * _2418;
                            }
                            else
                            {
                                frontier_phi_159_158_ladder = _1852;
                                frontier_phi_159_158_ladder_1 = _1853;
                                frontier_phi_159_158_ladder_2 = _1854;
                            }
                            _1105 = frontier_phi_159_158_ladder;
                            _1107 = frontier_phi_159_158_ladder_1;
                            _1109 = frontier_phi_159_158_ladder_2;
                        }
                        else
                        {
                            _1105 = _1852;
                            _1107 = _1853;
                            _1109 = _1854;
                        }
                        float _1119;
                        float _1121;
                        float _1123;
                        float _1125;
                        if (_1101)
                        {
                            float frontier_phi_165_164_ladder;
                            float frontier_phi_165_164_ladder_1;
                            float frontier_phi_165_164_ladder_2;
                            float frontier_phi_165_164_ladder_3;
                            if (((_428 & 33554432u) == 0u) || (((_380 & 4u) != 0u) && ((_428 & 8388608u) == 0u)))
                            {
                                frontier_phi_165_164_ladder = 0.0f;
                                frontier_phi_165_164_ladder_1 = 0.0f;
                                frontier_phi_165_164_ladder_2 = 0.0f;
                                frontier_phi_165_164_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2450 = (spvUnpackHalf2x16((_1823 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2451 = (spvUnpackHalf2x16((_1823 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2455 = (1.0f - abs(_2450)) - abs(_2451);
                                float _2457 = clamp((-0.0f) - _2455, 0.0f, 1.0f);
                                float _2458 = (-0.0f) - _2457;
                                float _2463 = ((_2450 >= 0.0f) ? _2458 : _2457) + _2450;
                                float _2464 = ((_2451 >= 0.0f) ? _2458 : _2457) + _2451;
                                float _2468 = rsqrt(dot(float3(_2463, _2464, _2455), float3(_2463, _2464, _2455)));
                                float _2469 = _2463 * _2468;
                                float _2470 = _2464 * _2468;
                                float _2471 = _2468 * _2455;
                                float _2475 = rsqrt(dot(float3(_2469, _2470, _2471), float3(_2469, _2470, _2471)));
                                frontier_phi_165_164_ladder = _2475 * _2471;
                                frontier_phi_165_164_ladder_1 = _2475 * _2470;
                                frontier_phi_165_164_ladder_2 = _2475 * _2469;
                                frontier_phi_165_164_ladder_3 = spvUnpackHalf2x16((_1823 >> 17u) & 32736u).x;
                            }
                            _1119 = frontier_phi_165_164_ladder_3;
                            _1121 = frontier_phi_165_164_ladder_2;
                            _1123 = frontier_phi_165_164_ladder_1;
                            _1125 = frontier_phi_165_164_ladder;
                        }
                        else
                        {
                            _1119 = 0.0f;
                            _1121 = 0.0f;
                            _1123 = 0.0f;
                            _1125 = 0.0f;
                        }
                        bool _2432 = _2220 != 0u;
                        _1102 = _1103;
                        _1104 = _1105;
                        _1106 = _1107;
                        _1108 = _1109;
                        _1110 = _1111;
                        _1112 = _2432 ? _2214 : _1105;
                        _1114 = _2432 ? _2216 : _1107;
                        _1116 = _2432 ? _2218 : _1109;
                        _1118 = _1119;
                        _1120 = _1121;
                        _1122 = _1123;
                        _1124 = _1125;
                    }
                    else
                    {
                        uint4 _642 = _24[1u].Load(int3(uint2(_265, _266), 0u));
                        uint _644 = _642.x;
                        uint4 _648 = _24[9u].Load(int3(uint2(_265, _266), 0u));
                        uint _650 = _648.x;
                        float _1006;
                        float _1007;
                        float _1008;
                        if ((_428 & 33554432u) == 0u)
                        {
                            float _838 = (float((_644 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _839 = (float(_644 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _843 = (1.0f - abs(_838)) - abs(_839);
                            float _845 = clamp((-0.0f) - _843, 0.0f, 1.0f);
                            float _846 = (-0.0f) - _845;
                            float _851 = ((_838 >= 0.0f) ? _846 : _845) + _838;
                            float _852 = ((_839 >= 0.0f) ? _846 : _845) + _839;
                            float _856 = rsqrt(dot(float3(_851, _852, _843), float3(_851, _852, _843)));
                            _1006 = _851 * _856;
                            _1007 = _852 * _856;
                            _1008 = _856 * _843;
                        }
                        else
                        {
                            float _867 = (float((_650 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _868 = (float(_650 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _872 = (1.0f - abs(_867)) - abs(_868);
                            float _874 = clamp((-0.0f) - _872, 0.0f, 1.0f);
                            float _875 = (-0.0f) - _874;
                            float _880 = ((_867 >= 0.0f) ? _875 : _874) + _867;
                            float _881 = ((_868 >= 0.0f) ? _875 : _874) + _868;
                            float _885 = rsqrt(dot(float3(_880, _881, _872), float3(_880, _881, _872)));
                            _1006 = _880 * _885;
                            _1007 = _881 * _885;
                            _1008 = _885 * _872;
                        }
                        _1102 = float(_644 & 255u);
                        _1104 = _1006;
                        _1106 = _1007;
                        _1108 = _1008;
                        _1110 = 1.0f;
                        _1112 = _1006;
                        _1114 = _1007;
                        _1116 = _1008;
                        _1118 = 0.0f;
                        _1120 = 0.0f;
                        _1122 = 0.0f;
                        _1124 = 0.0f;
                    }
                    precise float _1126 = _1102 * 0.0039215688593685626983642578125f;
                    if ((_427 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1238 = ((_427 & 128u) | _437) != 0u;
                    uint _1291;
                    if (_1238)
                    {
                        _1291 = 1u;
                    }
                    else
                    {
                        _1291 = (((_428 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _392;
                    float _394;
                    float _396;
                    uint _398;
                    float _402;
                    if (((_428 & 33554432u) == 0u) || _1238)
                    {
                        bool _1386 = _77 != 0u;
                        uint _1393;
                        if ((_428 & 16u) == 0u)
                        {
                            _1393 = _1291;
                        }
                        else
                        {
                            _1393 = ((_428 & 268435456u) != 0u) ? _1291 : 2u;
                        }
                        _402 = _1110 * _1126;
                        _392 = _1386 ? _1112 : _1104;
                        _394 = _1386 ? _1114 : _1106;
                        _396 = _1386 ? _1116 : _1108;
                        _398 = _1393;
                    }
                    else
                    {
                        _402 = _1118;
                        _392 = _1120;
                        _394 = _1122;
                        _396 = _1124;
                        _398 = _1291;
                    }
                    uint _1394 = _398 + 102u;
                    float _1403 = clamp((_402 - _45_m0[_1394].x) / (_45_m0[_1394].y - _45_m0[_1394].x), 0.0f, 1.0f);
                    _391 = _392;
                    _393 = _394;
                    _395 = _396;
                    _397 = _398;
                    _399 = (_1403 * _1403) * (3.0f - (_1403 * 2.0f));
                    _401 = _402;
                    _403 = _45_m0[_1394].z;
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
                float _456 = _50_m0[50u].w + _50_m0[50u].y;
                uint _459 = _397 + 63u;
                float _468 = clamp(((_50_m0[50u].x / (_456 - (_50_m0[50u].y * _447))) - _50_m0[_459].y) / (_50_m0[_459].x - _50_m0[_459].y), 0.0f, 1.0f);
                float _473 = ((_468 * _468) * _399) * (3.0f - (_468 * 2.0f));
                float _484 = ((_254 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _485 = ((1.0f - (_50_m0[51u].w * _255)) * 2.0f) + (-1.0f);
                float _501 = mad(_144, _447, mad(_137, _485, _484 * _130)) + _151;
                float _502 = (mad(_141, _447, mad(_134, _485, _484 * _127)) + _148) / _501;
                float _503 = (mad(_142, _447, mad(_135, _485, _484 * _128)) + _149) / _501;
                float _504 = (mad(_143, _447, mad(_136, _485, _484 * _129)) + _150) / _501;
                float _508 = rsqrt(dot(float3(_502, _503, _504), float3(_502, _503, _504)));
                float _509 = _508 * _502;
                float _510 = _508 * _503;
                float _511 = _508 * _504;
                float _514 = mad(_93, _395, mad(_87, _393, _391 * _81));
                float _517 = mad(_94, _395, mad(_88, _393, _391 * _82));
                float _520 = mad(_95, _395, mad(_89, _393, _391 * _83));
                float _524 = dot(float3(_509, _510, _511), float3(_514, _517, _520)) * 2.0f;
                float _528 = _509 - (_524 * _514);
                float _529 = _510 - (_524 * _517);
                float _530 = _511 - (_524 * _520);
                bool _534 = dot(float3(_528, _529, _530), float3(_509, _510, _511)) < 0.0f;
                float _541 = sqrt(((_503 * _503) + (_502 * _502)) + (_504 * _504)) * 0.001000000047497451305389404296875f;
                float _552 = ((_541 * _514) + _502) + (_528 * _403);
                float _553 = ((_541 * _517) + _503) + (_529 * _403);
                float _554 = ((_541 * _520) + _504) + (_530 * _403);
                float _570 = mad(_116, _554, mad(_109, _553, _552 * _102)) + _123;
                float _573 = (mad(_115, _554, mad(_108, _553, _552 * _101)) + _122) / _570;
                float _576 = (((mad(_113, _554, mad(_106, _553, _552 * _99)) + _120) / _570) * 0.5f) + 0.5f;
                float _577 = 0.5f - (((mad(_114, _554, mad(_107, _553, _552 * _100)) + _121) / _570) * 0.5f);
                float _580 = _576 * _50_m0[51u].x;
                float _581 = _577 * _50_m0[51u].y;
                float _586 = _552 + (_528 * 0.100000001490116119384765625f);
                float _587 = _553 + (_529 * 0.100000001490116119384765625f);
                float _588 = _554 + (_530 * 0.100000001490116119384765625f);
                float _604 = mad(_116, _588, mad(_109, _587, _586 * _102)) + _123;
                float _613 = _50_m0[51u].x * (((((mad(_113, _588, mad(_106, _587, _586 * _99)) + _120) / _604) * 0.5f) + 0.5f) - _576);
                float _615 = _50_m0[51u].y * ((0.5f - (((mad(_114, _588, mad(_107, _587, _586 * _100)) + _121) / _604) * 0.5f)) - _577);
                float _616 = ((mad(_115, _588, mad(_108, _587, _586 * _101)) + _122) / _604) - _573;
                float _617 = _613 * 10.0f;
                float _619 = _615 * 10.0f;
                float _620 = _616 * 10.0f;
                bool _627 = _397 == 1u;
                uint _661;
                uint _663;
                float _665;
                float _667;
                float _669;
                float _671;
                float _673;
                float _675;
                uint _677;
                uint _679;
                float _681;
                float _683;
                float _685;
                if (_627 && (asuint(_50_m0[62u]).w != 0u))
                {
                    _661 = 0u;
                    _663 = 1u;
                    _665 = 0.0f;
                    _667 = 0.0f;
                    _669 = 1.0f;
                    _671 = 0.0f;
                    _673 = 0.0f;
                    _675 = 0.0f;
                    _677 = 0u;
                    _679 = 0u;
                    _681 = 0.0f;
                    _683 = 0.0f;
                    _685 = 0.0f;
                }
                else
                {
                    float _746 = float(_233);
                    float _747 = float(_234);
                    float _754 = (_617 != 0.0f) ? (0.100000001490116119384765625f / _613) : 3.4028234663852885981170418348452e+38f;
                    float _756 = (_619 != 0.0f) ? (0.100000001490116119384765625f / _615) : 3.4028234663852885981170418348452e+38f;
                    float _757 = (_620 != 0.0f) ? (0.100000001490116119384765625f / _616) : 3.4028234663852885981170418348452e+38f;
                    float _758 = 1.0f / _746;
                    float _759 = 1.0f / _747;
                    float _760 = 0.004999999888241291046142578125f / _746;
                    float _762 = 0.004999999888241291046142578125f / _747;
                    float _771 = float(_617 >= 0.0f);
                    float _772 = float(_619 >= 0.0f);
                    float _781 = ((_617 < 0.0f) ? ((-0.0f) - _760) : _760) - _580;
                    float _784 = ((_619 < 0.0f) ? ((-0.0f) - _762) : _762) - _581;
                    float _787 = min((((floor(_580 * _746) + _771) * _758) + _781) * _754, (((floor(_581 * _747) + _772) * _759) + _784) * _756);
                    float _791 = (_787 * _617) + _580;
                    float _792 = (_787 * _619) + _581;
                    float _793 = (_787 * _620) + _573;
                    float _796 = _50_m0[50u].x / (_456 - (_793 * _50_m0[50u].y));
                    float _807 = max(0.300000011920928955078125f, 10.0f / max(0.00999999977648258209228515625f, max(abs(_613 * 38400.0f), abs(_615 * 21600.0f))));
                    float _682;
                    float _684;
                    float _686;
                    uint _950;
                    uint _965;
                    uint _967;
                    uint _664;
                    float _666;
                    float _668;
                    float _670;
                    float _672;
                    float _674;
                    float _676;
                    float _955;
                    float _957;
                    float _959;
                    float _961;
                    float _963;
                    uint _949 = 0u;
                    float _951 = _793;
                    float _952 = _792;
                    float _953 = _791;
                    float _954 = _787;
                    float _956 = _759;
                    float _958 = _758;
                    float _960 = _747;
                    float _962 = _746;
                    uint _964 = 0u;
                    uint _966 = 0u;
                    float _968 = _793;
                    float _969 = _792;
                    float _970 = _791;
                    float _971 = 1.0f;
                    float _972 = 0.0f;
                    float _973 = 0.0f;
                    uint _974 = 1u;
                    float _975;
                    float _976;
                    uint _977;
                    uint _978;
                    bool _979;
                    for (;;)
                    {
                        _975 = _962 * _953;
                        _976 = _960 * _952;
                        _977 = uint(int(_975));
                        _978 = uint(int(_976));
                        _979 = _964 == 0u;
                        float _1185;
                        if (_979)
                        {
                            _1185 = _12.Load(int3(uint2(_977, _978), 0u)).x;
                        }
                        else
                        {
                            _1185 = _15.Load(int3(uint2(_977, _978), _964 + 4294967295u)).x;
                        }
                        float _1191 = ((_975 >= floor(_962)) || (_976 >= floor(_960))) ? 1.0f : _1185;
                        float _1205 = (_620 < 0.0f) ? ((_1191 - _573) * _757) : 3.4028234663852885981170418348452e+38f;
                        float _1207 = min(min((((floor(_975) + _771) * _958) + _781) * _754, (((floor(_976) + _772) * _956) + _784) * _756), _1205);
                        bool _1208 = _1191 < _951;
                        bool _1212 = _1208 && (asuint(_1207) != asuint(_1205));
                        float _1213 = _1208 ? _1207 : _954;
                        float _1217 = (_1213 * _617) + _580;
                        float _1218 = (_1213 * _619) + _581;
                        float _1219 = (_1213 * _620) + _573;
                        uint _1221 = (_1212 ? 1u : 4294967295u) + _964;
                        float _1222 = _1212 ? 0.5f : 2.0f;
                        float _1223 = _1222 * _962;
                        float _1224 = _1222 * _960;
                        float _1225 = _1212 ? 2.0f : 0.5f;
                        float _1226 = _1225 * _958;
                        float _1227 = _1225 * _956;
                        _950 = _949 + 1u;
                        uint _1371;
                        uint _1373;
                        if (int(_1221) < int(0u))
                        {
                            float _1269 = _50_m0[50u].w + _50_m0[50u].y;
                            float _1271 = _50_m0[50u].x / (_1269 - (_50_m0[50u].y * _1191));
                            float _1274 = _50_m0[50u].x / (_1269 - (_50_m0[50u].y * _1219));
                            float _1276 = abs(_796 - _1274);
                            float _1279 = _1274 - _1271;
                            float frontier_phi_71_54_ladder;
                            float frontier_phi_71_54_ladder_1;
                            float frontier_phi_71_54_ladder_2;
                            uint frontier_phi_71_54_ladder_3;
                            float frontier_phi_71_54_ladder_4;
                            float frontier_phi_71_54_ladder_5;
                            float frontier_phi_71_54_ladder_6;
                            float frontier_phi_71_54_ladder_7;
                            float frontier_phi_71_54_ladder_8;
                            uint frontier_phi_71_54_ladder_9;
                            uint frontier_phi_71_54_ladder_10;
                            float frontier_phi_71_54_ladder_11;
                            float frontier_phi_71_54_ladder_12;
                            float frontier_phi_71_54_ladder_13;
                            if (_1279 > max(0.00999999977648258209228515625f, _1276 * 0.00999999977648258209228515625f))
                            {
                                bool _1351 = _966 != 0u;
                                frontier_phi_71_54_ladder = _759;
                                frontier_phi_71_54_ladder_1 = _1213 + _807;
                                frontier_phi_71_54_ladder_2 = _758;
                                frontier_phi_71_54_ladder_3 = _974;
                                frontier_phi_71_54_ladder_4 = _1351 ? _973 : _1217;
                                frontier_phi_71_54_ladder_5 = _1351 ? _972 : _1218;
                                frontier_phi_71_54_ladder_6 = _971;
                                frontier_phi_71_54_ladder_7 = _970;
                                frontier_phi_71_54_ladder_8 = _969;
                                frontier_phi_71_54_ladder_9 = (_627 || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _966;
                                frontier_phi_71_54_ladder_10 = 0u;
                                frontier_phi_71_54_ladder_11 = _746;
                                frontier_phi_71_54_ladder_12 = _747;
                                frontier_phi_71_54_ladder_13 = _968;
                            }
                            else
                            {
                                float _1364 = max(0.100000001490116119384765625f, _1276 * 0.100000001490116119384765625f) * 0.5f;
                                float _1367 = clamp((abs(_1279) - _1364) / _1364, 0.0f, 1.0f);
                                uint _1369 = uint(_1271 < _796);
                                float frontier_phi_71_54_ladder_70_ladder;
                                float frontier_phi_71_54_ladder_70_ladder_1;
                                float frontier_phi_71_54_ladder_70_ladder_2;
                                uint frontier_phi_71_54_ladder_70_ladder_3;
                                float frontier_phi_71_54_ladder_70_ladder_4;
                                float frontier_phi_71_54_ladder_70_ladder_5;
                                float frontier_phi_71_54_ladder_70_ladder_6;
                                float frontier_phi_71_54_ladder_70_ladder_7;
                                float frontier_phi_71_54_ladder_70_ladder_8;
                                uint frontier_phi_71_54_ladder_70_ladder_9;
                                uint frontier_phi_71_54_ladder_70_ladder_10;
                                float frontier_phi_71_54_ladder_70_ladder_11;
                                float frontier_phi_71_54_ladder_70_ladder_12;
                                float frontier_phi_71_54_ladder_70_ladder_13;
                                if (_966 == 0u)
                                {
                                    frontier_phi_71_54_ladder_70_ladder = _1227;
                                    frontier_phi_71_54_ladder_70_ladder_1 = _1213;
                                    frontier_phi_71_54_ladder_70_ladder_2 = _1226;
                                    frontier_phi_71_54_ladder_70_ladder_3 = _1369;
                                    frontier_phi_71_54_ladder_70_ladder_4 = _973;
                                    frontier_phi_71_54_ladder_70_ladder_5 = _972;
                                    frontier_phi_71_54_ladder_70_ladder_6 = _1367;
                                    frontier_phi_71_54_ladder_70_ladder_7 = _970;
                                    frontier_phi_71_54_ladder_70_ladder_8 = _969;
                                    frontier_phi_71_54_ladder_70_ladder_9 = uint(_1367 > 0.0f);
                                    frontier_phi_71_54_ladder_70_ladder_10 = _1221;
                                    frontier_phi_71_54_ladder_70_ladder_11 = _1223;
                                    frontier_phi_71_54_ladder_70_ladder_12 = _1224;
                                    frontier_phi_71_54_ladder_70_ladder_13 = _968;
                                }
                                else
                                {
                                    frontier_phi_71_54_ladder_70_ladder = _1227;
                                    frontier_phi_71_54_ladder_70_ladder_1 = _1213;
                                    frontier_phi_71_54_ladder_70_ladder_2 = _1226;
                                    frontier_phi_71_54_ladder_70_ladder_3 = _1369;
                                    frontier_phi_71_54_ladder_70_ladder_4 = _973;
                                    frontier_phi_71_54_ladder_70_ladder_5 = _972;
                                    frontier_phi_71_54_ladder_70_ladder_6 = _1367;
                                    frontier_phi_71_54_ladder_70_ladder_7 = _970;
                                    frontier_phi_71_54_ladder_70_ladder_8 = _969;
                                    frontier_phi_71_54_ladder_70_ladder_9 = _966;
                                    frontier_phi_71_54_ladder_70_ladder_10 = _1221;
                                    frontier_phi_71_54_ladder_70_ladder_11 = _1223;
                                    frontier_phi_71_54_ladder_70_ladder_12 = _1224;
                                    frontier_phi_71_54_ladder_70_ladder_13 = _968;
                                }
                                frontier_phi_71_54_ladder = frontier_phi_71_54_ladder_70_ladder;
                                frontier_phi_71_54_ladder_1 = frontier_phi_71_54_ladder_70_ladder_1;
                                frontier_phi_71_54_ladder_2 = frontier_phi_71_54_ladder_70_ladder_2;
                                frontier_phi_71_54_ladder_3 = frontier_phi_71_54_ladder_70_ladder_3;
                                frontier_phi_71_54_ladder_4 = frontier_phi_71_54_ladder_70_ladder_4;
                                frontier_phi_71_54_ladder_5 = frontier_phi_71_54_ladder_70_ladder_5;
                                frontier_phi_71_54_ladder_6 = frontier_phi_71_54_ladder_70_ladder_6;
                                frontier_phi_71_54_ladder_7 = frontier_phi_71_54_ladder_70_ladder_7;
                                frontier_phi_71_54_ladder_8 = frontier_phi_71_54_ladder_70_ladder_8;
                                frontier_phi_71_54_ladder_9 = frontier_phi_71_54_ladder_70_ladder_9;
                                frontier_phi_71_54_ladder_10 = frontier_phi_71_54_ladder_70_ladder_10;
                                frontier_phi_71_54_ladder_11 = frontier_phi_71_54_ladder_70_ladder_11;
                                frontier_phi_71_54_ladder_12 = frontier_phi_71_54_ladder_70_ladder_12;
                                frontier_phi_71_54_ladder_13 = frontier_phi_71_54_ladder_70_ladder_13;
                            }
                            _664 = frontier_phi_71_54_ladder_3;
                            _666 = frontier_phi_71_54_ladder_4;
                            _668 = frontier_phi_71_54_ladder_5;
                            _670 = frontier_phi_71_54_ladder_6;
                            _672 = frontier_phi_71_54_ladder_7;
                            _674 = frontier_phi_71_54_ladder_8;
                            _676 = frontier_phi_71_54_ladder_13;
                            _1371 = frontier_phi_71_54_ladder_9;
                            _1373 = frontier_phi_71_54_ladder_10;
                            _963 = frontier_phi_71_54_ladder_11;
                            _961 = frontier_phi_71_54_ladder_12;
                            _959 = frontier_phi_71_54_ladder_2;
                            _957 = frontier_phi_71_54_ladder;
                            _955 = frontier_phi_71_54_ladder_1;
                        }
                        else
                        {
                            bool _1281 = _966 != 0u;
                            _664 = _974;
                            _666 = _973;
                            _668 = _972;
                            _670 = _971;
                            _672 = _1281 ? _970 : _1217;
                            _674 = _1281 ? _969 : _1218;
                            _676 = _1281 ? _968 : _1219;
                            _1371 = _966;
                            _1373 = _1221;
                            _963 = _1223;
                            _961 = _1224;
                            _959 = _1226;
                            _957 = _1227;
                            _955 = _1213;
                        }
                        float frontier_phi_106_pred;
                        uint frontier_phi_106_pred_1;
                        uint frontier_phi_106_pred_2;
                        float frontier_phi_106_pred_3;
                        float frontier_phi_106_pred_4;
                        bool _1376;
                        bool _1378;
                        for (;;)
                        {
                            _1376 = _1219 < 0.0f;
                            _1378 = _1376 || ((_1217 < 0.0f) || (_1218 < 0.0f));
                            if (!_1378)
                            {
                                if (!((_1219 > 1.0f) || ((_1217 > _50_m0[51u].x) || (_1218 > _50_m0[51u].y))))
                                {
                                    frontier_phi_106_pred = _1217;
                                    frontier_phi_106_pred_1 = _1371;
                                    frontier_phi_106_pred_2 = _1373;
                                    frontier_phi_106_pred_3 = _1218;
                                    frontier_phi_106_pred_4 = _1219;
                                    break;
                                }
                            }
                            if (!_1376)
                            {
                                frontier_phi_106_pred = _1217;
                                frontier_phi_106_pred_1 = 1u;
                                frontier_phi_106_pred_2 = 4294967295u;
                                frontier_phi_106_pred_3 = _1218;
                                frontier_phi_106_pred_4 = _1219;
                                break;
                            }
                            float _1758 = (-0.0f) - _1219;
                            float _1759 = _1758 / _620;
                            frontier_phi_106_pred = (_1759 * _617) + _1217;
                            frontier_phi_106_pred_1 = 1u;
                            frontier_phi_106_pred_2 = 4294967295u;
                            frontier_phi_106_pred_3 = (_1759 * _619) + _1218;
                            frontier_phi_106_pred_4 = _1758 + _1219;
                            break;
                        }
                        _682 = frontier_phi_106_pred;
                        _967 = frontier_phi_106_pred_1;
                        _965 = frontier_phi_106_pred_2;
                        _684 = frontier_phi_106_pred_3;
                        _686 = frontier_phi_106_pred_4;
                        if ((_950 < 128u) && (int(_965) > int(4294967295u)))
                        {
                            _949 = _950;
                            _951 = _686;
                            _952 = _684;
                            _953 = _682;
                            _954 = _955;
                            _956 = _957;
                            _958 = _959;
                            _960 = _961;
                            _962 = _963;
                            _964 = _965;
                            _966 = _967;
                            _968 = _676;
                            _969 = _674;
                            _970 = _672;
                            _971 = _670;
                            _972 = _668;
                            _973 = _666;
                            _974 = _664;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    bool _1820 = _950 > 127u;
                    _661 = uint(_1820);
                    _663 = _664;
                    _665 = _666;
                    _667 = _668;
                    _669 = _670;
                    _671 = _672;
                    _673 = _674;
                    _675 = _676;
                    _677 = _1820 ? 1u : _967;
                    _679 = uint(_950 < 129u);
                    _681 = _682;
                    _683 = _684;
                    _685 = _686;
                }
                float _693 = _50_m0[51u].z * 2.0f;
                float _696 = (_693 * _580) + (-1.0f);
                float _697 = ((1.0f - (_50_m0[51u].w * _581)) * 2.0f) + (-1.0f);
                float _713 = mad(_190, _573, mad(_183, _697, _696 * _176)) + _197;
                float _714 = (mad(_187, _573, mad(_180, _697, _696 * _173)) + _194) / _713;
                float _715 = (mad(_188, _573, mad(_181, _697, _696 * _174)) + _195) / _713;
                float _716 = (mad(_189, _573, mad(_182, _697, _696 * _175)) + _196) / _713;
                float _721 = (_693 * _681) + (-1.0f);
                float _722 = ((1.0f - (_50_m0[51u].w * _683)) * 2.0f) + (-1.0f);
                float _738 = mad(_190, _685, mad(_183, _722, _721 * _176)) + _197;
                float _742 = ((mad(_187, _685, mad(_180, _722, _721 * _173)) + _194) / _738) - _714;
                float _743 = ((mad(_188, _685, mad(_181, _722, _721 * _174)) + _195) / _738) - _715;
                float _744 = ((mad(_189, _685, mad(_182, _722, _721 * _175)) + _196) / _738) - _716;
                float _897;
                uint _899;
                float _901;
                if (_679 == 0u)
                {
                    _897 = 0.0f;
                    _899 = _677;
                    _901 = 0.0f;
                }
                else
                {
                    float _944 = float(_233);
                    float _945 = float(_234);
                    float frontier_phi_25_26_ladder;
                    uint frontier_phi_25_26_ladder_1;
                    float frontier_phi_25_26_ladder_2;
                    if ((_681 < 0.0f) || (_683 < 0.0f))
                    {
                        frontier_phi_25_26_ladder = 0.0f;
                        frontier_phi_25_26_ladder_1 = _677;
                        frontier_phi_25_26_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_25_26_ladder_33_ladder;
                        uint frontier_phi_25_26_ladder_33_ladder_1;
                        float frontier_phi_25_26_ladder_33_ladder_2;
                        if ((_685 >= 1.0f) || ((_681 > _50_m0[51u].x) || (_683 > _50_m0[51u].y)))
                        {
                            frontier_phi_25_26_ladder_33_ladder = 0.0f;
                            frontier_phi_25_26_ladder_33_ladder_1 = _677;
                            frontier_phi_25_26_ladder_33_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_25_26_ladder_33_ladder_42_ladder;
                            uint frontier_phi_25_26_ladder_33_ladder_42_ladder_1;
                            float frontier_phi_25_26_ladder_33_ladder_42_ladder_2;
                            for (;;)
                            {
                                if ((abs(_681 - _254) < (2.0f / _944)) && (abs(_683 - _255) < (2.0f / _945)))
                                {
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder = 0.0f;
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder_1 = _677;
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_86;
                                    float frontier_phi_86_pred;
                                    uint frontier_phi_86_pred_1;
                                    float frontier_phi_86_pred_2;
                                    uint _1260;
                                    uint _1261;
                                    bool _1262;
                                    for (;;)
                                    {
                                        _1260 = uint(int(_681 * _944));
                                        _1261 = uint(int(_683 * _945));
                                        _1262 = _627 && _534;
                                        if (!_1262)
                                        {
                                            if (!(dot(float3(_742, _743, _744), float3(_742, _743, _744)) < 0.000899999984540045261383056640625f))
                                            {
                                                ladder_phi_86 = false;
                                                frontier_phi_86_pred = 0.0f;
                                                frontier_phi_86_pred_1 = _677;
                                                frontier_phi_86_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1341 = _24[22u].Load(int3(uint2(_1260, _1261), 0u));
                                        uint _1343 = _1341.x;
                                        float _1744;
                                        float _1745;
                                        float _1746;
                                        if (_1343 == 0u)
                                        {
                                            uint4 _1496 = _24[1u].Load(int3(uint2(_1260, _1261), 0u));
                                            uint _1498 = _1496.x;
                                            float _1506 = (float((_1498 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1507 = (float(_1498 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1511 = (1.0f - abs(_1506)) - abs(_1507);
                                            float _1513 = clamp((-0.0f) - _1511, 0.0f, 1.0f);
                                            float _1514 = (-0.0f) - _1513;
                                            _1744 = ((_1506 >= 0.0f) ? _1514 : _1513) + _1506;
                                            _1745 = ((_1507 >= 0.0f) ? _1514 : _1513) + _1507;
                                            _1746 = _1511;
                                        }
                                        else
                                        {
                                            float _1528 = (float((_1343 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1529 = (float(_1343 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1533 = (1.0f - abs(_1528)) - abs(_1529);
                                            float _1535 = clamp((-0.0f) - _1533, 0.0f, 1.0f);
                                            float _1536 = (-0.0f) - _1535;
                                            _1744 = ((_1528 >= 0.0f) ? _1536 : _1535) + _1528;
                                            _1745 = ((_1529 >= 0.0f) ? _1536 : _1535) + _1529;
                                            _1746 = _1533;
                                        }
                                        float _1750 = rsqrt(dot(float3(_1744, _1745, _1746), float3(_1744, _1745, _1746)));
                                        if (dot(float3(_1750 * _1744, _1750 * _1745, _1750 * _1746), float3(_742, _743, _744)) > 0.0f)
                                        {
                                            ladder_phi_86 = true;
                                            frontier_phi_86_pred = 0.0f;
                                            frontier_phi_86_pred_1 = _677;
                                            frontier_phi_86_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_86 = false;
                                            frontier_phi_86_pred = 0.0f;
                                            frontier_phi_86_pred_1 = _677;
                                            frontier_phi_86_pred_2 = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_86)
                                    {
                                        frontier_phi_25_26_ladder_33_ladder_42_ladder = frontier_phi_86_pred;
                                        frontier_phi_25_26_ladder_33_ladder_42_ladder_1 = frontier_phi_86_pred_1;
                                        frontier_phi_25_26_ladder_33_ladder_42_ladder_2 = frontier_phi_86_pred_2;
                                        break;
                                    }
                                    float _1547 = _50_m0[51u].z * _681;
                                    float _1548 = _50_m0[51u].w * _683;
                                    float _1550 = (_945 / _944) * 0.0500000007450580596923828125f;
                                    float _1555 = clamp(_1547 / _1550, 0.0f, 1.0f);
                                    float _1556 = clamp(_1548 * 20.0f, 0.0f, 1.0f);
                                    float _1568 = clamp(((_1547 + (-1.0f)) + _1550) / _1550, 0.0f, 1.0f);
                                    float _1569 = clamp((_1548 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1580 = _1555 * _1556;
                                    precise float _1581 = _1580 * _1580;
                                    float _1585 = ((((3.0f - (_1556 * 2.0f)) * (3.0f - (_1555 * 2.0f))) * _1581) * (1.0f - ((_1568 * _1568) * (3.0f - (_1568 * 2.0f))))) * (1.0f - ((_1569 * _1569) * (3.0f - (_1569 * 2.0f))));
                                    bool _1588 = (_677 != 0u) || (_1585 >= 1.0f);
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder = _1585 * float(_473 > 0.0f);
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder_1 = _1588 ? _677 : 1u;
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder_2 = _1588 ? 0.0f : _1585;
                                    break;
                                }
                            }
                            frontier_phi_25_26_ladder_33_ladder = frontier_phi_25_26_ladder_33_ladder_42_ladder;
                            frontier_phi_25_26_ladder_33_ladder_1 = frontier_phi_25_26_ladder_33_ladder_42_ladder_1;
                            frontier_phi_25_26_ladder_33_ladder_2 = frontier_phi_25_26_ladder_33_ladder_42_ladder_2;
                        }
                        frontier_phi_25_26_ladder = frontier_phi_25_26_ladder_33_ladder;
                        frontier_phi_25_26_ladder_1 = frontier_phi_25_26_ladder_33_ladder_1;
                        frontier_phi_25_26_ladder_2 = frontier_phi_25_26_ladder_33_ladder_2;
                    }
                    _897 = frontier_phi_25_26_ladder_2;
                    _899 = frontier_phi_25_26_ladder_1;
                    _901 = frontier_phi_25_26_ladder;
                }
                uint _1056;
                uint _1058;
                float _942;
                for (;;)
                {
                    _942 = ((((exp2(log2(clamp((sqrt(((_715 * _715) + (_714 * _714)) + (_716 * _716)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _407) * exp2(log2(clamp((_715 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_393, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    if (_901 > 0.0f)
                    {
                        uint _1018 = uint((_202 * 0.999989986419677734375f) * clamp(_681, 0.0f, 1.0f));
                        uint _1019 = uint((_204 * 0.999989986419677734375f) * clamp(_683, 0.0f, 1.0f));
                        uint4 _1022 = _24[2u].Load(int3(uint2(_1018, _1019), 0u));
                        uint _1025 = _1022.w;
                        uint4 _1030 = _24[15u].Load(int3(uint2(_1018, _1019), 0u));
                        uint _1032 = _1030.y;
                        uint _1038 = ((_1032 & 64u) != 0u) ? uint((_1032 & 4294967167u) != 66u) : 4294967295u;
                        uint _1039 = _1025 & 128u;
                        uint _1041 = (_1039 != 0u) ? 1u : ((_1022.x << 7u) | _1025);
                        uint4 _1044 = _16.Load(_1041 * 4u);
                        uint _1045 = _1044.x;
                        uint _1052 = ((_1045 & 1u) != 0u) ? 0u : 18u;
                        uint _1054 = uint(min(int(uint(max(int(_1038), int(0u)))), int(1u)));
                        uint _1136;
                        if (_1039 == 0u)
                        {
                            _1136 = (((_1045 & 2097152u) != 0u) && (_1038 == _1054)) ? (_1052 | 128u) : _1052;
                        }
                        else
                        {
                            _1136 = _1025;
                        }
                        uint _1137 = _16.Load((_1041 * 4u) + 1u).x & 512u;
                        bool _1140 = (_1136 & 144u) == 0u;
                        if (_1137 == 0u)
                        {
                            if (_1140 || ((_1045 & 1u) != 0u))
                            {
                                _1056 = 0u;
                                _1058 = 0u;
                                break;
                            }
                        }
                        else
                        {
                            if (_1140)
                            {
                                _1056 = 0u;
                                _1058 = 0u;
                                break;
                            }
                        }
                        bool _1301 = ((_1136 & 128u) | _1137) != 0u;
                        uint _1057;
                        if (_1301)
                        {
                            _1057 = 1u;
                        }
                        else
                        {
                            _1057 = (((_1045 >> 14u) & 2u) ^ 2u) + 3u;
                        }
                        if (((_1045 & 268435472u) == 16u) && (((_1045 & 33554432u) == 0u) || _1301))
                        {
                            _1056 = 2u;
                            _1058 = 0u;
                            break;
                        }
                        _1056 = _1057;
                        _1058 = (_1057 == 1u) ? _1054 : 0u;
                        break;
                    }
                    else
                    {
                        _1056 = 0u;
                        _1058 = 0u;
                        break;
                    }
                }
                uint _1145;
                float _1146;
                float _1147;
                float _1148;
                float _1150;
                float _1062;
                float _1065;
                float _1068;
                float _1072;
                float _1073;
                for (;;)
                {
                    _1062 = mad(_167, _530, mad(_161, _529, _528 * _155));
                    _1065 = mad(_168, _530, mad(_162, _529, _528 * _156));
                    _1068 = mad(_169, _530, mad(_163, _529, _528 * _157));
                    bool _1071 = (_899 != 0u) || (_901 < 1.0f);
                    _1072 = _1071 ? 0.0f : 1.0f;
                    _1073 = _1071 ? 0.0f : 0.5f;
                    if (_1071)
                    {
                        uint4 _1144 = asuint(_50_m0[60u]);
                        if (_627)
                        {
                            if (int(_405) < int(1u))
                            {
                                if (_1144.x == 0u)
                                {
                                    _1145 = 0u;
                                    _1146 = 0.0f;
                                    _1147 = 0.0f;
                                    _1148 = 0.0f;
                                    _1150 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1144.y == 0u)
                                {
                                    _1145 = 0u;
                                    _1146 = 0.0f;
                                    _1147 = 0.0f;
                                    _1148 = 0.0f;
                                    _1150 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1144.z == 0u)
                            {
                                _1145 = 0u;
                                _1146 = 0.0f;
                                _1147 = 0.0f;
                                _1148 = 0.0f;
                                _1150 = 0.0f;
                                break;
                            }
                        }
                        if (_897 > 0.0f)
                        {
                            _1145 = 0u;
                            _1146 = 0.0f;
                            _1147 = 1.0f;
                            _1148 = 1000.0f;
                            _1150 = 0.5f;
                            break;
                        }
                        if (((_671 <= 0.0f) || (_673 <= 0.0f)) || (_675 <= 0.0f))
                        {
                            _1145 = 0u;
                            _1146 = 0.0f;
                            _1147 = 1.0f;
                            _1148 = 1000.0f;
                            _1150 = 0.5f;
                            break;
                        }
                        if ((_675 >= 1.0f) || ((_671 >= _50_m0[51u].x) || (_673 >= _50_m0[51u].y)))
                        {
                            _1145 = 0u;
                            _1146 = 0.0f;
                            _1147 = 1.0f;
                            _1148 = 1000.0f;
                            _1150 = 0.5f;
                            break;
                        }
                        uint _1999;
                        uint _2001;
                        uint _1779;
                        uint _1780;
                        bool _1786;
                        for (;;)
                        {
                            _1779 = uint(clamp(_665, 0.0f, 1.0f) * _202);
                            _1780 = uint(clamp(_667, 0.0f, 1.0f) * _204);
                            _1786 = _20[21u].Load(int3(uint2(_1779, _1780), 0u)).x > 0.0f;
                            if (_1786)
                            {
                                uint _1871 = _24[23u].Load(int3(uint2(_1779, _1780), 0u)).y + 4294967295u;
                                _1999 = (uint(int(_1871) >> int(31u)) & 3u) + 1u;
                                _2001 = (int(_1871) < int(0u)) ? 0u : _1871;
                                break;
                            }
                            else
                            {
                                uint4 _1879 = _24[2u].Load(int3(uint2(_1779, _1780), 0u));
                                uint _1882 = _1879.w;
                                uint4 _1887 = _24[15u].Load(int3(uint2(_1779, _1780), 0u));
                                uint _1889 = _1887.y;
                                uint _1895 = ((_1889 & 64u) != 0u) ? uint((_1889 & 4294967167u) != 66u) : 4294967295u;
                                uint _1896 = _1882 & 128u;
                                uint _1898 = (_1896 != 0u) ? 1u : ((_1879.x << 7u) | _1882);
                                uint4 _1901 = _16.Load(_1898 * 4u);
                                uint _1902 = _1901.x;
                                uint _1909 = ((_1902 & 1u) != 0u) ? 0u : 18u;
                                uint _1911 = uint(min(int(uint(max(int(_1895), int(0u)))), int(1u)));
                                uint _2012;
                                if (_1896 == 0u)
                                {
                                    _2012 = (((_1902 & 2097152u) != 0u) && (_1895 == _1911)) ? (_1909 | 128u) : _1909;
                                }
                                else
                                {
                                    _2012 = _1882;
                                }
                                uint _2013 = _16.Load((_1898 * 4u) + 1u).x & 512u;
                                bool _2016 = (_2012 & 144u) == 0u;
                                if (_2013 == 0u)
                                {
                                    if (_2016 || ((_1902 & 1u) != 0u))
                                    {
                                        _1999 = 0u;
                                        _2001 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2016)
                                    {
                                        _1999 = 0u;
                                        _2001 = 0u;
                                        break;
                                    }
                                }
                                bool _2169 = ((_2012 & 128u) | _2013) != 0u;
                                uint _2000;
                                if (_2169)
                                {
                                    _2000 = 1u;
                                }
                                else
                                {
                                    _2000 = (((_1902 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_1902 & 268435472u) == 16u) && (((_1902 & 33554432u) == 0u) || _2169))
                                {
                                    _1999 = 2u;
                                    _2001 = 0u;
                                    break;
                                }
                                _1999 = _2000;
                                _2001 = (_2000 == 1u) ? _1911 : 0u;
                                break;
                            }
                        }
                        if ((_397 != _1999) || (_405 != _2001))
                        {
                            _1145 = 0u;
                            _1146 = 0.0f;
                            _1147 = 1.0f;
                            _1148 = 1000.0f;
                            _1150 = 0.5f;
                            break;
                        }
                        float _2087 = _671 * 2.0f;
                        float _2090 = (_50_m0[51u].z * _2087) + (-1.0f);
                        float _2091 = ((1.0f - (_50_m0[51u].w * _673)) * 2.0f) + (-1.0f);
                        float _2107 = mad(_190, _675, mad(_183, _2091, _2090 * _176)) + _197;
                        float _2108 = (mad(_187, _675, mad(_180, _2091, _2090 * _173)) + _194) / _2107;
                        float _2109 = (mad(_188, _675, mad(_181, _2091, _2090 * _174)) + _195) / _2107;
                        float _2110 = (mad(_189, _675, mad(_182, _2091, _2090 * _175)) + _196) / _2107;
                        if (sqrt(((_2109 * _2109) + (_2108 * _2108)) + (_2110 * _2110)) > _50_m0[58u].w)
                        {
                            _1145 = 0u;
                            _1146 = 0.0f;
                            _1147 = 0.0f;
                            _1148 = 1000.0f;
                            _1150 = 0.5f;
                            break;
                        }
                        float _2149 = _2108 - _714;
                        float _2150 = _2109 - _715;
                        float _2151 = _2110 - _716;
                        float _2157 = sqrt(((_2150 * _2150) + (_2149 * _2149)) + (_2151 * _2151));
                        float _2165 = min(_50_m0[59u].y, max(0.0f, _2157 + (-0.001000000047497451305389404296875f)));
                        float _2275;
                        if (_627)
                        {
                            _2275 = min(_50_m0[59u].x, _2165 + 10.0f);
                        }
                        else
                        {
                            _2275 = _50_m0[59u].x;
                        }
                        float _2276 = _2275 - _2157;
                        if (!(_2276 > 0.0f))
                        {
                            _1145 = 0u;
                            _1146 = 1.0f;
                            _1147 = 1.0f;
                            _1148 = 0.0f;
                            _1150 = 0.5f;
                            break;
                        }
                        float _2309 = _2108 - (_2165 * _1062);
                        float _2310 = _2109 - (_2165 * _1065);
                        float _2311 = _2110 - (_2165 * _1068);
                        RayDesc _2ident = {float3(mad(_2311, _50_m0[46u].z, mad(_2310, _50_m0[46u].y, _50_m0[46u].x * _2309)) + _50_m0[46u].w, mad(_2311, _50_m0[47u].z, mad(_2310, _50_m0[47u].y, _50_m0[47u].x * _2309)) + _50_m0[47u].w, mad(_2311, _50_m0[48u].z, mad(_2310, _50_m0[48u].y, _50_m0[48u].x * _2309)) + _50_m0[48u].w), 0.0f, float3(mad(_1068, _50_m0[46u].z, mad(_1065, _50_m0[46u].y, _50_m0[46u].x * _1062)), mad(_1068, _50_m0[47u].z, mad(_1065, _50_m0[47u].y, _50_m0[47u].x * _1062)), mad(_1068, _50_m0[48u].z, mad(_1065, _50_m0[48u].y, _50_m0[48u].x * _1062))), _2276};
                        _2314.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2361 = _2314.Proceed();
                        uint _2362 = _2314.CommittedStatus();
                        if (!(_2362 == 1u))
                        {
                            _1145 = 0u;
                            _1146 = 1.0f;
                            _1147 = 0.0f;
                            _1148 = 0.0f;
                            _1150 = 0.5f;
                            break;
                        }
                        float _2433 = _2314.CommittedRayT();
                        if (!((_2433 < _2276) && (_2433 > 0.0f)))
                        {
                            _1145 = 0u;
                            _1146 = 1.0f;
                            _1147 = 0.0f;
                            _1148 = 0.0f;
                            _1150 = 0.5f;
                            break;
                        }
                        float _2484 = (_50_m0[51u].z * _2087) + (-1.0f);
                        float _2485 = ((1.0f - (_50_m0[51u].w * _673)) * 2.0f) + (-1.0f);
                        float _2501 = mad(_144, _675, mad(_137, _2485, _2484 * _130)) + _151;
                        float _2505 = _2433 - _2165;
                        float _2509 = ((mad(_141, _675, mad(_134, _2485, _2484 * _127)) + _148) / _2501) + (_2505 * _528);
                        float _2510 = ((mad(_142, _675, mad(_135, _2485, _2484 * _128)) + _149) / _2501) + (_2505 * _529);
                        float _2511 = ((mad(_143, _675, mad(_136, _2485, _2484 * _129)) + _150) / _2501) + (_2505 * _530);
                        float _2523 = mad(_116, _2511, mad(_109, _2510, _2509 * _102)) + _123;
                        float _2533 = (_50_m0[51u].z * _50_m0[51u].x) * ((((mad(_113, _2511, mad(_106, _2510, _2509 * _99)) + _120) / _2523) * 0.5f) + 0.5f);
                        float _2535 = (_50_m0[51u].w * _50_m0[51u].y) * (0.5f - (((mad(_114, _2511, mad(_107, _2510, _2509 * _100)) + _121) / _2523) * 0.5f));
                        float _2537 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2540 = clamp(_2533 / _2537, 0.0f, 1.0f);
                        float _2541 = clamp(_2535 * 20.0f, 0.0f, 1.0f);
                        float _2551 = clamp(((_2537 + (-1.0f)) + _2533) / _2537, 0.0f, 1.0f);
                        float _2552 = clamp((_2535 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2563 = _2540 * _2541;
                        precise float _2564 = _2563 * _2563;
                        if ((((((3.0f - (_2541 * 2.0f)) * (3.0f - (_2540 * 2.0f))) * _2564) * (1.0f - ((_2551 * _2551) * (3.0f - (_2551 * 2.0f))))) * (1.0f - ((_2552 * _2552) * (3.0f - (_2552 * 2.0f))))) < 1.0f)
                        {
                            _1145 = 0u;
                            _1146 = _1072;
                            _1147 = _1072;
                            _1148 = 0.0f;
                            _1150 = _1073;
                            break;
                        }
                        _1145 = 1u;
                        _1146 = 0.0f;
                        _1147 = 1.0f;
                        _1148 = 0.0f;
                        _1150 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1145 = 0u;
                        _1146 = 1.0f;
                        _1147 = 1.0f;
                        _1148 = 0.0f;
                        _1150 = 0.5f;
                        break;
                    }
                }
                uint4 _1167 = asuint(_50_m0[55u]);
                float _1172 = clamp(log2(_50_m0[61u].w * exp2(log2((1.0f - _401) * 0.75f) * 1.5f)) / float(_1167.x + 4294967295u), 0.0f, 1.0f);
                uint _1248;
                float _1249;
                float _1250;
                float _1252;
                float _1254;
                if (_627 && (_1167.z != 0u))
                {
                    uint frontier_phi_52_51_ladder;
                    float frontier_phi_52_51_ladder_1;
                    float frontier_phi_52_51_ladder_2;
                    float frontier_phi_52_51_ladder_3;
                    float frontier_phi_52_51_ladder_4;
                    if ((_1056 != 1u) || (_1058 != 0u))
                    {
                        float _1310 = _573 / max(9.9999999747524270787835121154785e-07f, (-0.0f) - _620);
                        float _1319 = ((_1310 * _617) + _580) / _50_m0[51u].x;
                        float _1320 = ((_1310 * _619) + _581) / _50_m0[51u].y;
                        float _1433;
                        if (_1320 < 0.300000011920928955078125f)
                        {
                            float _1425 = (0.300000011920928955078125f - _1320) * 3.3333332538604736328125f;
                            _1433 = 0.300000011920928955078125f - ((_1425 / sqrt((_1425 * _1425) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _1433 = _1320;
                        }
                        float _1634;
                        if (_1319 < 0.300000011920928955078125f)
                        {
                            float _1627 = (0.300000011920928955078125f - _1319) * 3.3333332538604736328125f;
                            _1634 = 0.300000011920928955078125f - ((_1627 / sqrt((_1627 * _1627) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _1634 = _1319;
                        }
                        float _1797;
                        if ((1.0f - _1634) < 0.300000011920928955078125f)
                        {
                            float _1789 = (_1634 + (-0.699999988079071044921875f)) * 3.3333332538604736328125f;
                            _1797 = ((_1789 / sqrt((_1789 * _1789) + 1.0f)) * 0.300000011920928955078125f) + 0.699999988079071044921875f;
                        }
                        else
                        {
                            _1797 = _1634;
                        }
                        frontier_phi_52_51_ladder = 0u;
                        frontier_phi_52_51_ladder_1 = 0.0f;
                        frontier_phi_52_51_ladder_2 = _1797 * _50_m0[51u].x;
                        frontier_phi_52_51_ladder_3 = 1.0f - clamp((_530 + (-0.25f)) * (-4.0f), 0.0f, 1.0f);
                        frontier_phi_52_51_ladder_4 = _1433 * _50_m0[51u].y;
                    }
                    else
                    {
                        frontier_phi_52_51_ladder = _661;
                        frontier_phi_52_51_ladder_1 = _669;
                        frontier_phi_52_51_ladder_2 = _681;
                        frontier_phi_52_51_ladder_3 = _901;
                        frontier_phi_52_51_ladder_4 = _683;
                    }
                    _1248 = frontier_phi_52_51_ladder;
                    _1249 = frontier_phi_52_51_ladder_1;
                    _1250 = frontier_phi_52_51_ladder_2;
                    _1252 = frontier_phi_52_51_ladder_4;
                    _1254 = frontier_phi_52_51_ladder_3;
                }
                else
                {
                    _1248 = _661;
                    _1249 = _669;
                    _1250 = _681;
                    _1252 = _683;
                    _1254 = _901;
                }
                bool _1256 = _397 != 1u;
                float _1653;
                float _1655;
                float _1657;
                float _1659;
                if (_1254 == 0.0f)
                {
                    float _1435;
                    float _1437;
                    float _1439;
                    float _1441;
                    if (_1145 == 0u)
                    {
                        float frontier_phi_81_80_ladder;
                        float frontier_phi_81_80_ladder_1;
                        float frontier_phi_81_80_ladder_2;
                        float frontier_phi_81_80_ladder_3;
                        if (_1256)
                        {
                            float _1637 = _1065 * _942;
                            float _1641 = rsqrt(dot(float3(_1062, _1637, _1068), float3(_1062, _1637, _1068)));
                            float4 _1651 = _28[4u].SampleLevel(_59, float3(_1641 * _1062, _1641 * _1637, _1641 * _1068), 0.0f);
                            frontier_phi_81_80_ladder = 1.0f;
                            frontier_phi_81_80_ladder_1 = _1651.z;
                            frontier_phi_81_80_ladder_2 = _1651.y;
                            frontier_phi_81_80_ladder_3 = _1651.x;
                        }
                        else
                        {
                            frontier_phi_81_80_ladder = 0.0f;
                            frontier_phi_81_80_ladder_1 = 0.0f;
                            frontier_phi_81_80_ladder_2 = 0.0f;
                            frontier_phi_81_80_ladder_3 = 0.0f;
                        }
                        _1435 = frontier_phi_81_80_ladder_3;
                        _1437 = frontier_phi_81_80_ladder_2;
                        _1439 = frontier_phi_81_80_ladder_1;
                        _1441 = frontier_phi_81_80_ladder;
                    }
                    else
                    {
                        _1435 = 0.0f;
                        _1437 = 0.0f;
                        _1439 = 0.0f;
                        _1441 = 1.0f;
                    }
                    _1653 = _1435 * _473;
                    _1655 = _1437 * _473;
                    _1657 = _1439 * _473;
                    _1659 = _1441 * _473;
                }
                else
                {
                    uint4 _1325 = asuint(_55_m0[0u]);
                    float _1734;
                    float _1736;
                    float _1739;
                    float _1741;
                    if (_12.Load(int3(uint2(uint(float(_1325.x) * _1250), uint(float(_1325.y) * _1252)), 0u)).x > 0.0f)
                    {
                        uint _1447_dummy_parameter;
                        uint2 _1447 = spvTextureSize(_14, 0u, _1447_dummy_parameter);
                        float4 _1456 = _14.Load(int3(uint2(uint(float(_1447.x) * _1250), uint(float(_1447.y) * _1252)), 0u));
                        float _1460 = _1456.x * 0.5f;
                        float _1461 = _1456.y * (-0.5f);
                        float4 _1480 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _1460) + (_50_m0[52u].x * _1250), (_50_m0[52u].w * _1461) + (_50_m0[52u].y * _1252)), 0.0f);
                        float _1490 = _50_m0[54u].x * _1480.x;
                        float _1491 = _50_m0[54u].x * _1480.y;
                        float _1492 = _50_m0[54u].x * _1480.z;
                        float _1721;
                        if (_627)
                        {
                            float frontier_phi_101_100_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _573))) < 5.0f)
                            {
                                float _1813 = sqrt((_1460 * _1460) + (_1461 * _1461));
                                float frontier_phi_101_100_ladder_114_ladder;
                                if (_1813 > 0.0500000007450580596923828125f)
                                {
                                    float _1920 = _1250 - _580;
                                    float _1921 = _1252 - _581;
                                    float frontier_phi_101_100_ladder_114_ladder_125_ladder;
                                    if (_1813 > sqrt((_1921 * _1921) + (_1920 * _1920)))
                                    {
                                        uint4 _2027 = asuint(_55_m0[0u]);
                                        uint _2034 = uint(float(_2027.x) * _1250);
                                        uint _2035 = uint(float(_2027.y) * _1252);
                                        uint4 _2038 = _24[2u].Load(int3(uint2(_2034, _2035), 0u));
                                        uint _2041 = _2038.w;
                                        uint4 _2046 = _24[15u].Load(int3(uint2(_2034, _2035), 0u));
                                        uint _2048 = _2046.y;
                                        uint _2054 = ((_2048 & 64u) != 0u) ? uint((_2048 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2055 = _2041 & 128u;
                                        uint _2057 = (_2055 != 0u) ? 1u : ((_2038.x << 7u) | _2041);
                                        uint4 _2060 = _16.Load(_2057 * 4u);
                                        uint _2061 = _2060.x;
                                        uint _2068 = ((_2061 & 1u) != 0u) ? 0u : 18u;
                                        uint _2132;
                                        if (_2055 == 0u)
                                        {
                                            _2132 = (((_2061 & 2097152u) != 0u) && (_2054 == uint(min(int(uint(max(int(_2054), int(0u)))), int(1u))))) ? (_2068 | 128u) : _2068;
                                        }
                                        else
                                        {
                                            _2132 = _2041;
                                        }
                                        float frontier_phi_101_100_ladder_114_ladder_125_ladder_143_ladder;
                                        if (((_2132 & 128u) | (_16.Load((_2057 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2172 = asuint(_55_m0[0u]);
                                            uint _2181 = uint(float(_2172.x) * (_1460 + _1250));
                                            uint _2182 = uint(float(_2172.y) * (_1461 + _1252));
                                            uint4 _2185 = _24[2u].Load(int3(uint2(_2181, _2182), 0u));
                                            uint _2188 = _2185.w;
                                            uint4 _2191 = _24[15u].Load(int3(uint2(_2181, _2182), 0u));
                                            uint _2193 = _2191.y;
                                            uint _2199 = ((_2193 & 64u) != 0u) ? uint((_2193 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2200 = _2188 & 128u;
                                            uint _2202 = (_2200 != 0u) ? 1u : ((_2185.x << 7u) | _2188);
                                            uint4 _2204 = _16.Load(_2202 * 4u);
                                            uint _2205 = _2204.x;
                                            uint _2212 = ((_2205 & 1u) != 0u) ? 0u : 18u;
                                            uint _2296;
                                            if (_2200 == 0u)
                                            {
                                                _2296 = (((_2205 & 2097152u) != 0u) && (_2199 == uint(min(int(uint(max(int(_2199), int(0u)))), int(1u))))) ? (_2212 | 128u) : _2212;
                                            }
                                            else
                                            {
                                                _2296 = _2188;
                                            }
                                            float frontier_phi_101_100_ladder_114_ladder_125_ladder_143_ladder_157_ladder;
                                            if ((_2296 & 128u) == 0u)
                                            {
                                                frontier_phi_101_100_ladder_114_ladder_125_ladder_143_ladder_157_ladder = ((_16.Load((_2202 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_101_100_ladder_114_ladder_125_ladder_143_ladder_157_ladder = 0.0f;
                                            }
                                            frontier_phi_101_100_ladder_114_ladder_125_ladder_143_ladder = frontier_phi_101_100_ladder_114_ladder_125_ladder_143_ladder_157_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_101_100_ladder_114_ladder_125_ladder_143_ladder = 1.0f;
                                        }
                                        frontier_phi_101_100_ladder_114_ladder_125_ladder = frontier_phi_101_100_ladder_114_ladder_125_ladder_143_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_101_100_ladder_114_ladder_125_ladder = 1.0f;
                                    }
                                    frontier_phi_101_100_ladder_114_ladder = frontier_phi_101_100_ladder_114_ladder_125_ladder;
                                }
                                else
                                {
                                    frontier_phi_101_100_ladder_114_ladder = 1.0f;
                                }
                                frontier_phi_101_100_ladder = frontier_phi_101_100_ladder_114_ladder;
                            }
                            else
                            {
                                frontier_phi_101_100_ladder = 1.0f;
                            }
                            _1721 = frontier_phi_101_100_ladder;
                        }
                        else
                        {
                            _1721 = 1.0f;
                        }
                        float frontier_phi_103_101_ladder;
                        float frontier_phi_103_101_ladder_1;
                        float frontier_phi_103_101_ladder_2;
                        float frontier_phi_103_101_ladder_3;
                        float _1723;
                        for (;;)
                        {
                            _1723 = _1721 * _1254;
                            if (_1249 > 0.0f)
                            {
                                float _1738;
                                float _1740;
                                float _1742;
                                if (_627)
                                {
                                    _1738 = 0.0f;
                                    _1740 = 0.0f;
                                    _1742 = 0.0f;
                                }
                                else
                                {
                                    if (!((_397 != 4u) || (_663 != 0u)))
                                    {
                                        frontier_phi_103_101_ladder = _1490;
                                        frontier_phi_103_101_ladder_1 = _1723;
                                        frontier_phi_103_101_ladder_2 = _1491;
                                        frontier_phi_103_101_ladder_3 = _1492;
                                        break;
                                    }
                                    _1738 = _1490;
                                    _1740 = _1491;
                                    _1742 = _1492;
                                }
                                frontier_phi_103_101_ladder = _1738;
                                frontier_phi_103_101_ladder_1 = (1.0f - exp2(log2(_1249) * 3.0f)) * _1723;
                                frontier_phi_103_101_ladder_2 = _1740;
                                frontier_phi_103_101_ladder_3 = _1742;
                                break;
                            }
                            else
                            {
                                frontier_phi_103_101_ladder = _1490;
                                frontier_phi_103_101_ladder_1 = _1723;
                                frontier_phi_103_101_ladder_2 = _1491;
                                frontier_phi_103_101_ladder_3 = _1492;
                                break;
                            }
                        }
                        _1734 = frontier_phi_103_101_ladder_1;
                        _1736 = frontier_phi_103_101_ladder;
                        _1739 = frontier_phi_103_101_ladder_2;
                        _1741 = frontier_phi_103_101_ladder_3;
                    }
                    else
                    {
                        float frontier_phi_103_83_ladder;
                        float frontier_phi_103_83_ladder_1;
                        float frontier_phi_103_83_ladder_2;
                        float frontier_phi_103_83_ladder_3;
                        if (_1248 == 0u)
                        {
                            float4 _1729 = _28[7u].SampleLevel(_59, float3(_1062, _1065, _1068), 0.0f);
                            frontier_phi_103_83_ladder = _1729.x;
                            frontier_phi_103_83_ladder_1 = _1254;
                            frontier_phi_103_83_ladder_2 = _1729.y;
                            frontier_phi_103_83_ladder_3 = _1729.z;
                        }
                        else
                        {
                            frontier_phi_103_83_ladder = _1737;
                            frontier_phi_103_83_ladder_1 = 0.0f;
                            frontier_phi_103_83_ladder_2 = _1737;
                            frontier_phi_103_83_ladder_3 = _1737;
                        }
                        _1734 = frontier_phi_103_83_ladder_1;
                        _1736 = frontier_phi_103_83_ladder;
                        _1739 = frontier_phi_103_83_ladder_2;
                        _1741 = frontier_phi_103_83_ladder_3;
                    }
                    float _1960;
                    float _1961;
                    float _1962;
                    float _1963;
                    if (_1145 == 0u)
                    {
                        float frontier_phi_129_116_ladder;
                        float frontier_phi_129_116_ladder_1;
                        float frontier_phi_129_116_ladder_2;
                        float frontier_phi_129_116_ladder_3;
                        if (_1256 && (_1734 < 1.0f))
                        {
                            float _1934 = _1065 * _942;
                            float _1938 = rsqrt(dot(float3(_1062, _1934, _1068), float3(_1062, _1934, _1068)));
                            float4 _1946 = _28[4u].SampleLevel(_59, float3(_1938 * _1062, _1938 * _1934, _1938 * _1068), 0.0f);
                            float _1948 = _1946.x;
                            float _1949 = _1946.y;
                            float _1950 = _1946.z;
                            frontier_phi_129_116_ladder = ((_1741 - _1950) * _1734) + _1950;
                            frontier_phi_129_116_ladder_1 = ((_1739 - _1949) * _1734) + _1949;
                            frontier_phi_129_116_ladder_2 = ((_1736 - _1948) * _1734) + _1948;
                            frontier_phi_129_116_ladder_3 = 1.0f;
                        }
                        else
                        {
                            frontier_phi_129_116_ladder = _1741;
                            frontier_phi_129_116_ladder_1 = _1739;
                            frontier_phi_129_116_ladder_2 = _1736;
                            frontier_phi_129_116_ladder_3 = _1734;
                        }
                        _1960 = frontier_phi_129_116_ladder_3;
                        _1961 = frontier_phi_129_116_ladder_2;
                        _1962 = frontier_phi_129_116_ladder_1;
                        _1963 = frontier_phi_129_116_ladder;
                    }
                    else
                    {
                        _1960 = 1.0f;
                        _1961 = _1736 * _897;
                        _1962 = _1739 * _897;
                        _1963 = _1741 * _897;
                    }
                    float _1660 = _1960 * _473;
                    _1653 = _1961 * _1660;
                    _1655 = _1962 * _1660;
                    _1657 = _1963 * _1660;
                    _1659 = _1660;
                }
                float _1664 = _50_m0[58u].z * _1150;
                float _1686 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _1688 = _1686 * ((_1664 * ((_1146 * 1000.0f) - _1653)) + _1653);
                float _1689 = _1686 * ((_1664 * ((_1147 * 1000.0f) - _1655)) + _1655);
                float _1690 = _1686 * ((_1664 * (_1148 - _1657)) + _1657);
                float _1696 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_1688, max(_1689, _1690)) + 1.0f);
                float _1700 = min(_1696 * _1688, 0.996078431606292724609375f);
                float _1702 = min(_1696 * _1689, 0.996078431606292724609375f);
                float _1703 = min(_1696 * _1690, 0.996078431606292724609375f);
                _35[uint2(_221, _224)] = float4(_1700, _1702, _1703, _1659);
                _39[uint2(_221, _224)] = float4(_1172, 0.0f, 0.0f, _1172);
                if (_228)
                {
                    uint _1803 = _221 + 1u;
                    _35[uint2(_1803, _224)] = float4(_1700, _1702, _1703, _1659);
                    _39[uint2(_1803, _224)] = float4(_1172, 0.0f, 0.0f, _1172);
                }
                if (_231)
                {
                    uint _1913 = _224 + 1u;
                    _35[uint2(_221, _1913)] = float4(_1700, _1702, _1703, _1659);
                    _39[uint2(_221, _1913)] = float4(_1172, 0.0f, 0.0f, _1172);
                }
                if (_232)
                {
                    uint _2017 = _221 + 1u;
                    uint _2018 = _224 + 1u;
                    _35[uint2(_2017, _2018)] = float4(_1700, _1702, _1703, _1659);
                    _39[uint2(_2017, _2018)] = float4(_1172, 0.0f, 0.0f, _1172);
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
                uint _654 = _224 + 1u;
                _35[uint2(_221, _654)] = 0.0f.xxxx;
                _39[uint2(_221, _654)] = 0.0f.xxxx;
            }
            if (!_232)
            {
                break;
            }
            uint _889 = _221 + 1u;
            uint _890 = _224 + 1u;
            _35[uint2(_889, _890)] = 0.0f.xxxx;
            _39[uint2(_889, _890)] = 0.0f.xxxx;
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
