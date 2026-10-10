static float _1757;
static uint _2994;
static float _2995;
static float _2996;
static float _2997;
static float _2998;
static float _2999;
static float _3000;
static uint _3001;
static uint _3002;
static float _3003;
static float _3004;
static float _3005;
static float _3006;
static float _3007;
static float _3008;
static float _3014;
static uint _3015;
static float _3016;
static uint _3018;
static uint _3019;
static float _3024;

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

static RayQuery<RAY_FLAG_NONE> _2395;

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
                    float _1099;
                    float _1101;
                    float _1103;
                    float _1105;
                    float _1107;
                    float _1109;
                    float _1111;
                    float _1113;
                    float _1115;
                    float _1117;
                    float _1119;
                    float _1121;
                    if (_442 == 0u)
                    {
                        if (!((_433 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _834 = asfloat(_17.Load((_389 * 115u) + 33u).x);
                        uint4 _842 = _24[2u].Load(int3(uint2(_270, _271), 0u));
                        uint _844 = _842.y;
                        uint _845 = _432 & 128u;
                        uint _1087;
                        uint _1088;
                        uint _1089;
                        uint _1090;
                        if (_845 == 0u)
                        {
                            _1087 = uint(((_433 & 817889384u) | (_385 & 576u)) != 0u) | (((_433 >> 19u) & 1u) ^ 1u);
                            _1088 = uint(((_433 & 17825808u) | (_385 & 520u)) != 0u);
                            _1089 = uint(((_433 & 46137344u) | (_385 & 2564u)) != 0u);
                            _1090 = 0u;
                        }
                        else
                        {
                            _1087 = 1u;
                            _1088 = _432 & 1u;
                            _1089 = 1u;
                            _1090 = 1u;
                        }
                        precise float _1094 = float(_844 & 127u) * 0.0078740157186985015869140625f;
                        bool _1098 = (_433 & 4194304u) == 0u;
                        float _1223;
                        if (_1098)
                        {
                            _1223 = _1094;
                        }
                        else
                        {
                            _1223 = float(_844 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1378;
                        if ((_433 & 134217728u) == 0u)
                        {
                            uint frontier_phi_71_57_ladder;
                            if ((_845 != 0u) || ((_433 & 17825792u) == 1048576u))
                            {
                                frontier_phi_71_57_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_71_57_ladder = _1088;
                            }
                            _1378 = frontier_phi_71_57_ladder;
                        }
                        else
                        {
                            _1378 = _1088;
                        }
                        uint4 _1381 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _1383 = _1381.x;
                        float _1621;
                        float _1623;
                        float _1625;
                        if (_1087 == 0u)
                        {
                            _1621 = 0.0f;
                            _1623 = 0.0f;
                            _1625 = 0.0f;
                        }
                        else
                        {
                            float4 _1630 = _20[8u].Load(int3(uint2(_270, _271), 0u));
                            _1621 = _1630.x;
                            _1623 = _1630.y;
                            _1625 = _1630.z;
                        }
                        uint _1816;
                        if (_1378 == 0u)
                        {
                            _1816 = 0u;
                        }
                        else
                        {
                            _1816 = _24[9u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        uint _1888;
                        if (_1089 == 0u)
                        {
                            _1888 = 0u;
                        }
                        else
                        {
                            _1888 = _24[10u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        float _1898 = (float((_1383 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1899 = (float(_1383 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1903 = (1.0f - abs(_1898)) - abs(_1899);
                        float _1905 = clamp((-0.0f) - _1903, 0.0f, 1.0f);
                        float _1906 = (-0.0f) - _1905;
                        float _1911 = ((_1898 >= 0.0f) ? _1906 : _1905) + _1898;
                        float _1912 = ((_1899 >= 0.0f) ? _1906 : _1905) + _1899;
                        float _1916 = rsqrt(dot(float3(_1911, _1912, _1903), float3(_1911, _1912, _1903)));
                        float _1917 = _1911 * _1916;
                        float _1918 = _1912 * _1916;
                        float _1919 = _1916 * _1903;
                        float _1100 = float(_1383 & 255u);
                        float _1923 = ((_433 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _2151;
                        float _2152;
                        float _2153;
                        float _2154;
                        uint _2155;
                        if ((_385 & 64u) == 0u)
                        {
                            uint frontier_phi_143_134_ladder;
                            float frontier_phi_143_134_ladder_1;
                            float frontier_phi_143_134_ladder_2;
                            float frontier_phi_143_134_ladder_3;
                            float frontier_phi_143_134_ladder_4;
                            if ((_433 & 276824064u) == 0u)
                            {
                                frontier_phi_143_134_ladder = 0u;
                                frontier_phi_143_134_ladder_1 = 0.0f;
                                frontier_phi_143_134_ladder_2 = 0.0f;
                                frontier_phi_143_134_ladder_3 = 0.0f;
                                frontier_phi_143_134_ladder_4 = ((_433 & 8u) != 0u) ? _1623 : _1923;
                            }
                            else
                            {
                                frontier_phi_143_134_ladder = 0u;
                                frontier_phi_143_134_ladder_1 = 0.0f;
                                frontier_phi_143_134_ladder_2 = 0.0f;
                                frontier_phi_143_134_ladder_3 = 0.0f;
                                frontier_phi_143_134_ladder_4 = _1923;
                            }
                            _2151 = frontier_phi_143_134_ladder_4;
                            _2152 = frontier_phi_143_134_ladder_3;
                            _2153 = frontier_phi_143_134_ladder_2;
                            _2154 = frontier_phi_143_134_ladder_1;
                            _2155 = frontier_phi_143_134_ladder;
                        }
                        else
                        {
                            float _2055 = (_1623 * 2.0f) + (-1.0f);
                            float _2056 = (_1625 * 2.0f) + (-1.0f);
                            float _2060 = (1.0f - abs(_2055)) - abs(_2056);
                            float _2062 = clamp((-0.0f) - _2060, 0.0f, 1.0f);
                            float _2063 = (-0.0f) - _2062;
                            float _2068 = ((_2055 >= 0.0f) ? _2063 : _2062) + _2055;
                            float _2069 = ((_2056 >= 0.0f) ? _2063 : _2062) + _2056;
                            float _2073 = rsqrt(dot(float3(_2068, _2069, _2060), float3(_2068, _2069, _2060)));
                            _2151 = floor(round(_1621 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _2152 = _2068 * _2073;
                            _2153 = _2069 * _2073;
                            _2154 = _2073 * _2060;
                            _2155 = 1u;
                        }
                        float _1108;
                        if ((_433 & 32768u) == 0u)
                        {
                            _1108 = _2151;
                        }
                        else
                        {
                            float frontier_phi_149_150_ladder;
                            if (_17.Load((_389 * 115u) + 36u).x == 0u)
                            {
                                float _2349 = clamp((_1100 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _834;
                                frontier_phi_149_150_ladder = ((_433 & 131072u) != 0u) ? _2349 : ((((clamp((1.21000003814697265625f / (exp2((_1223 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_389 * 115u) + 32u).x)) + 1.0f) * _2349);
                            }
                            else
                            {
                                frontier_phi_149_150_ladder = _834;
                            }
                            _1108 = frontier_phi_149_150_ladder;
                        }
                        uint _2216 = _432 & 1u;
                        float _2292;
                        float _2294;
                        float _2296;
                        uint _2298;
                        if (((_433 & 16u) == 0u) || (((_2216 | (_385 & 8u)) | (_433 & 16777216u)) != 0u))
                        {
                            _2292 = _2152;
                            _2294 = _2153;
                            _2296 = _2154;
                            _2298 = _2155;
                        }
                        else
                        {
                            float _2308 = (float(_1816 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2309 = (float(_1816 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2313 = (1.0f - abs(_2308)) - abs(_2309);
                            float _2315 = clamp((-0.0f) - _2313, 0.0f, 1.0f);
                            float _2316 = (-0.0f) - _2315;
                            float _2321 = ((_2308 >= 0.0f) ? _2316 : _2315) + _2308;
                            float _2322 = ((_2309 >= 0.0f) ? _2316 : _2315) + _2309;
                            float _2326 = rsqrt(dot(float3(_2321, _2322, _2313), float3(_2321, _2322, _2313)));
                            _2292 = _2321 * _2326;
                            _2294 = _2322 * _2326;
                            _2296 = _2326 * _2313;
                            _2298 = 1u;
                        }
                        float _1102;
                        float _1104;
                        float _1106;
                        if (_2216 == 0u)
                        {
                            float frontier_phi_164_163_ladder;
                            float frontier_phi_164_163_ladder_1;
                            float frontier_phi_164_163_ladder_2;
                            if (((_432 & 64u) == 0u) && (_1090 != 0u))
                            {
                                float2 _2461 = spvUnpackHalf2x16((_1888 >> 17u) & 32736u);
                                float _2462 = _2461.x;
                                float _2465 = (spvUnpackHalf2x16((_1888 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2466 = (spvUnpackHalf2x16((_1888 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2470 = (1.0f - abs(_2465)) - abs(_2466);
                                float _2472 = clamp((-0.0f) - _2470, 0.0f, 1.0f);
                                float _2473 = (-0.0f) - _2472;
                                float _2478 = ((_2465 >= 0.0f) ? _2473 : _2472) + _2465;
                                float _2479 = ((_2466 >= 0.0f) ? _2473 : _2472) + _2466;
                                float _2483 = rsqrt(dot(float3(_2478, _2479, _2470), float3(_2478, _2479, _2470)));
                                float _2493 = (((_2478 * _2483) - _1917) * _2462) + _1917;
                                float _2494 = (((_2479 * _2483) - _1918) * _2462) + _1918;
                                float _2495 = (((_2483 * _2470) - _1919) * _2462) + _1919;
                                float _2499 = rsqrt(dot(float3(_2493, _2494, _2495), float3(_2493, _2494, _2495)));
                                frontier_phi_164_163_ladder = _2495 * _2499;
                                frontier_phi_164_163_ladder_1 = _2494 * _2499;
                                frontier_phi_164_163_ladder_2 = _2493 * _2499;
                            }
                            else
                            {
                                frontier_phi_164_163_ladder = _1919;
                                frontier_phi_164_163_ladder_1 = _1918;
                                frontier_phi_164_163_ladder_2 = _1917;
                            }
                            _1102 = frontier_phi_164_163_ladder_2;
                            _1104 = frontier_phi_164_163_ladder_1;
                            _1106 = frontier_phi_164_163_ladder;
                        }
                        else
                        {
                            _1102 = _1917;
                            _1104 = _1918;
                            _1106 = _1919;
                        }
                        float _1116;
                        float _1118;
                        float _1120;
                        float _1122;
                        if (_1098)
                        {
                            float frontier_phi_170_169_ladder;
                            float frontier_phi_170_169_ladder_1;
                            float frontier_phi_170_169_ladder_2;
                            float frontier_phi_170_169_ladder_3;
                            if (((_433 & 33554432u) == 0u) || (((_385 & 4u) != 0u) && ((_433 & 8388608u) == 0u)))
                            {
                                frontier_phi_170_169_ladder = 0.0f;
                                frontier_phi_170_169_ladder_1 = 0.0f;
                                frontier_phi_170_169_ladder_2 = 0.0f;
                                frontier_phi_170_169_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2531 = (spvUnpackHalf2x16((_1888 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2532 = (spvUnpackHalf2x16((_1888 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2536 = (1.0f - abs(_2531)) - abs(_2532);
                                float _2538 = clamp((-0.0f) - _2536, 0.0f, 1.0f);
                                float _2539 = (-0.0f) - _2538;
                                float _2544 = ((_2531 >= 0.0f) ? _2539 : _2538) + _2531;
                                float _2545 = ((_2532 >= 0.0f) ? _2539 : _2538) + _2532;
                                float _2549 = rsqrt(dot(float3(_2544, _2545, _2536), float3(_2544, _2545, _2536)));
                                float _2550 = _2544 * _2549;
                                float _2551 = _2545 * _2549;
                                float _2552 = _2549 * _2536;
                                float _2556 = rsqrt(dot(float3(_2550, _2551, _2552), float3(_2550, _2551, _2552)));
                                frontier_phi_170_169_ladder = _2556 * _2552;
                                frontier_phi_170_169_ladder_1 = _2556 * _2551;
                                frontier_phi_170_169_ladder_2 = _2556 * _2550;
                                frontier_phi_170_169_ladder_3 = spvUnpackHalf2x16((_1888 >> 17u) & 32736u).x;
                            }
                            _1116 = frontier_phi_170_169_ladder_3;
                            _1118 = frontier_phi_170_169_ladder_2;
                            _1120 = frontier_phi_170_169_ladder_1;
                            _1122 = frontier_phi_170_169_ladder;
                        }
                        else
                        {
                            _1116 = 0.0f;
                            _1118 = 0.0f;
                            _1120 = 0.0f;
                            _1122 = 0.0f;
                        }
                        bool _2513 = _2298 != 0u;
                        _1099 = _1100;
                        _1101 = _1102;
                        _1103 = _1104;
                        _1105 = _1106;
                        _1107 = _1108;
                        _1109 = _2513 ? _2292 : _1102;
                        _1111 = _2513 ? _2294 : _1104;
                        _1113 = _2513 ? _2296 : _1106;
                        _1115 = _1116;
                        _1117 = _1118;
                        _1119 = _1120;
                        _1121 = _1122;
                    }
                    else
                    {
                        uint4 _647 = _24[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _649 = _647.x;
                        uint4 _653 = _24[9u].Load(int3(uint2(_270, _271), 0u));
                        uint _655 = _653.x;
                        float _996;
                        float _997;
                        float _998;
                        if ((_433 & 33554432u) == 0u)
                        {
                            float _854 = (float((_649 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _855 = (float(_649 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _859 = (1.0f - abs(_854)) - abs(_855);
                            float _861 = clamp((-0.0f) - _859, 0.0f, 1.0f);
                            float _862 = (-0.0f) - _861;
                            float _867 = ((_854 >= 0.0f) ? _862 : _861) + _854;
                            float _868 = ((_855 >= 0.0f) ? _862 : _861) + _855;
                            float _872 = rsqrt(dot(float3(_867, _868, _859), float3(_867, _868, _859)));
                            _996 = _867 * _872;
                            _997 = _868 * _872;
                            _998 = _872 * _859;
                        }
                        else
                        {
                            float _883 = (float((_655 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _884 = (float(_655 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _888 = (1.0f - abs(_883)) - abs(_884);
                            float _890 = clamp((-0.0f) - _888, 0.0f, 1.0f);
                            float _891 = (-0.0f) - _890;
                            float _896 = ((_883 >= 0.0f) ? _891 : _890) + _883;
                            float _897 = ((_884 >= 0.0f) ? _891 : _890) + _884;
                            float _901 = rsqrt(dot(float3(_896, _897, _888), float3(_896, _897, _888)));
                            _996 = _896 * _901;
                            _997 = _897 * _901;
                            _998 = _901 * _888;
                        }
                        _1099 = float(_649 & 255u);
                        _1101 = _996;
                        _1103 = _997;
                        _1105 = _998;
                        _1107 = 1.0f;
                        _1109 = _996;
                        _1111 = _997;
                        _1113 = _998;
                        _1115 = 0.0f;
                        _1117 = 0.0f;
                        _1119 = 0.0f;
                        _1121 = 0.0f;
                    }
                    precise float _1123 = _1099 * 0.0039215688593685626983642578125f;
                    if ((_432 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1232 = ((_432 & 128u) | _442) != 0u;
                    uint _1272;
                    if (_1232)
                    {
                        _1272 = 1u;
                    }
                    else
                    {
                        _1272 = (((_433 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _397;
                    float _399;
                    float _401;
                    uint _403;
                    float _407;
                    if (((_433 & 33554432u) == 0u) || _1232)
                    {
                        bool _1385 = _77 != 0u;
                        uint _1392;
                        if ((_433 & 16u) == 0u)
                        {
                            _1392 = _1272;
                        }
                        else
                        {
                            _1392 = ((_433 & 268435456u) != 0u) ? _1272 : 2u;
                        }
                        _407 = _1107 * _1123;
                        _397 = _1385 ? _1109 : _1101;
                        _399 = _1385 ? _1111 : _1103;
                        _401 = _1385 ? _1113 : _1105;
                        _403 = _1392;
                    }
                    else
                    {
                        _407 = _1115;
                        _397 = _1117;
                        _399 = _1119;
                        _401 = _1121;
                        _403 = _1272;
                    }
                    uint _1393 = _403 + 102u;
                    float _1402 = clamp((_407 - _45_m0[_1393].x) / (_45_m0[_1393].y - _45_m0[_1393].x), 0.0f, 1.0f);
                    _396 = _397;
                    _398 = _399;
                    _400 = _401;
                    _402 = _403;
                    _404 = (_1402 * _1402) * (3.0f - (_1402 * 2.0f));
                    _406 = _407;
                    _408 = _45_m0[_1393].z;
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
                float _461 = _50_m0[50u].w + _50_m0[50u].y;
                uint _464 = _402 + 63u;
                float _473 = clamp(((_50_m0[50u].x / (_461 - (_50_m0[50u].y * _452))) - _50_m0[_464].y) / (_50_m0[_464].x - _50_m0[_464].y), 0.0f, 1.0f);
                float _478 = ((_473 * _473) * _404) * (3.0f - (_473 * 2.0f));
                float _489 = ((_259 * 2.0f) * _50_m0[51u].z) + (-1.0f);
                float _490 = ((1.0f - (_50_m0[51u].w * _260)) * 2.0f) + (-1.0f);
                float _506 = mad(_144, _452, mad(_137, _490, _489 * _130)) + _151;
                float _507 = (mad(_141, _452, mad(_134, _490, _489 * _127)) + _148) / _506;
                float _508 = (mad(_142, _452, mad(_135, _490, _489 * _128)) + _149) / _506;
                float _509 = (mad(_143, _452, mad(_136, _490, _489 * _129)) + _150) / _506;
                float _513 = rsqrt(dot(float3(_507, _508, _509), float3(_507, _508, _509)));
                float _514 = _513 * _507;
                float _515 = _513 * _508;
                float _516 = _513 * _509;
                float _519 = mad(_93, _400, mad(_87, _398, _396 * _81));
                float _522 = mad(_94, _400, mad(_88, _398, _396 * _82));
                float _525 = mad(_95, _400, mad(_89, _398, _396 * _83));
                float _529 = dot(float3(_514, _515, _516), float3(_519, _522, _525)) * 2.0f;
                float _533 = _514 - (_529 * _519);
                float _534 = _515 - (_529 * _522);
                float _535 = _516 - (_529 * _525);
                bool _539 = dot(float3(_533, _534, _535), float3(_514, _515, _516)) < 0.0f;
                float _546 = sqrt(((_508 * _508) + (_507 * _507)) + (_509 * _509)) * 0.001000000047497451305389404296875f;
                float _557 = ((_546 * _519) + _507) + (_533 * _408);
                float _558 = ((_546 * _522) + _508) + (_534 * _408);
                float _559 = ((_546 * _525) + _509) + (_535 * _408);
                float _575 = mad(_116, _559, mad(_109, _558, _557 * _102)) + _123;
                float _578 = (mad(_115, _559, mad(_108, _558, _557 * _101)) + _122) / _575;
                float _581 = (((mad(_113, _559, mad(_106, _558, _557 * _99)) + _120) / _575) * 0.5f) + 0.5f;
                float _582 = 0.5f - (((mad(_114, _559, mad(_107, _558, _557 * _100)) + _121) / _575) * 0.5f);
                float _585 = _581 * _50_m0[51u].x;
                float _586 = _582 * _50_m0[51u].y;
                float _591 = _557 + (_533 * 0.100000001490116119384765625f);
                float _592 = _558 + (_534 * 0.100000001490116119384765625f);
                float _593 = _559 + (_535 * 0.100000001490116119384765625f);
                float _609 = mad(_116, _593, mad(_109, _592, _591 * _102)) + _123;
                float _618 = _50_m0[51u].x * (((((mad(_113, _593, mad(_106, _592, _591 * _99)) + _120) / _609) * 0.5f) + 0.5f) - _581);
                float _620 = _50_m0[51u].y * ((0.5f - (((mad(_114, _593, mad(_107, _592, _591 * _100)) + _121) / _609) * 0.5f)) - _582);
                float _621 = ((mad(_115, _593, mad(_108, _592, _591 * _101)) + _122) / _609) - _578;
                float _622 = _618 * 10.0f;
                float _624 = _620 * 10.0f;
                float _625 = _621 * 10.0f;
                bool _632 = _402 == 1u;
                uint _666;
                uint _668;
                float _670;
                float _672;
                float _674;
                float _676;
                float _678;
                float _680;
                uint _682;
                uint _684;
                float _686;
                float _688;
                float _690;
                if (_632 && (asuint(_50_m0[62u]).w != 0u))
                {
                    _666 = 0u;
                    _668 = 1u;
                    _670 = 0.0f;
                    _672 = 0.0f;
                    _674 = 1.0f;
                    _676 = 0.0f;
                    _678 = 0.0f;
                    _680 = 0.0f;
                    _682 = 0u;
                    _684 = 0u;
                    _686 = 0.0f;
                    _688 = 0.0f;
                    _690 = 0.0f;
                }
                else
                {
                    float _751 = float(_238);
                    float _752 = float(_239);
                    float _759 = (_622 != 0.0f) ? (0.100000001490116119384765625f / _618) : 3.4028234663852885981170418348452e+38f;
                    float _761 = (_624 != 0.0f) ? (0.100000001490116119384765625f / _620) : 3.4028234663852885981170418348452e+38f;
                    float _762 = (_625 != 0.0f) ? (0.100000001490116119384765625f / _621) : 3.4028234663852885981170418348452e+38f;
                    float _763 = 1.0f / _751;
                    float _764 = 1.0f / _752;
                    float _765 = 0.004999999888241291046142578125f / _751;
                    float _767 = 0.004999999888241291046142578125f / _752;
                    float _776 = float(_622 >= 0.0f);
                    float _777 = float(_624 >= 0.0f);
                    float _786 = ((_622 < 0.0f) ? ((-0.0f) - _765) : _765) - _585;
                    float _789 = ((_624 < 0.0f) ? ((-0.0f) - _767) : _767) - _586;
                    float _792 = min((((floor(_585 * _751) + _776) * _763) + _786) * _759, (((floor(_586 * _752) + _777) * _764) + _789) * _761);
                    float _796 = (_792 * _622) + _585;
                    float _797 = (_792 * _624) + _586;
                    float _798 = (_792 * _625) + _578;
                    float _801 = _50_m0[50u].x / (_461 - (_798 * _50_m0[50u].y));
                    float _811 = 1.0f / max(0.00999999977648258209228515625f, max(abs(_618 * 38400.0f), abs(_620 * 21600.0f)));
                    float _819 = max(_50_m0[3u].z, _50_m0[5u].z * _811);
                    float _965;
                    if (asuint(_50_m0[5u]).y == 0u)
                    {
                        _965 = _819;
                    }
                    else
                    {
                        _965 = min(_819, _50_m0[5u].w * _811);
                    }
                    uint _669;
                    float _671;
                    float _673;
                    float _675;
                    float _677;
                    float _679;
                    float _681;
                    float _687;
                    float _689;
                    float _691;
                    uint _1071;
                    uint _1076;
                    if (_209 == 0u)
                    {
                        _1071 = 0u;
                        _691 = _798;
                        _689 = _797;
                        _687 = _796;
                        _1076 = 0u;
                        _681 = _798;
                        _679 = _797;
                        _677 = _796;
                        _675 = 1.0f;
                        _673 = 0.0f;
                        _671 = 0.0f;
                        _669 = 1u;
                    }
                    else
                    {
                        uint _1072;
                        float _1073;
                        float _1074;
                        float _1075;
                        uint _1077;
                        uint _1209;
                        float _1078;
                        float _1079;
                        float _1080;
                        float _1081;
                        float _1082;
                        float _1083;
                        uint _1084;
                        float _1194;
                        float _1199;
                        float _1201;
                        float _1203;
                        float _1205;
                        float _1207;
                        uint _1192 = 0u;
                        float _1193 = _965;
                        float _1195 = _798;
                        float _1196 = _797;
                        float _1197 = _796;
                        float _1198 = _792;
                        float _1200 = _764;
                        float _1202 = _763;
                        float _1204 = _752;
                        float _1206 = _751;
                        uint _1208 = 0u;
                        uint _1210 = 0u;
                        float _1211 = _798;
                        float _1212 = _797;
                        float _1213 = _796;
                        float _1214 = 1.0f;
                        float _1215 = 0.0f;
                        float _1216 = 0.0f;
                        uint _1217 = 1u;
                        float _1218;
                        float _1219;
                        uint _1220;
                        uint _1221;
                        bool _1222;
                        for (;;)
                        {
                            _1218 = _1206 * _1197;
                            _1219 = _1204 * _1196;
                            _1220 = uint(int(_1218));
                            _1221 = uint(int(_1219));
                            _1222 = _1208 == 0u;
                            float _1334;
                            if (_1222)
                            {
                                _1334 = _12.Load(int3(uint2(_1220, _1221), 0u)).x;
                            }
                            else
                            {
                                _1334 = _15.Load(int3(uint2(_1220, _1221), _1208 + 4294967295u)).x;
                            }
                            float _1340 = ((_1218 >= floor(_1206)) || (_1219 >= floor(_1204))) ? 1.0f : _1334;
                            float _1354 = (_625 < 0.0f) ? ((_1340 - _578) * _762) : 3.4028234663852885981170418348452e+38f;
                            float _1356 = min(min((((floor(_1218) + _776) * _1202) + _786) * _759, (((floor(_1219) + _777) * _1200) + _789) * _761), _1354);
                            bool _1357 = _1340 < _1195;
                            bool _1361 = _1357 && (asuint(_1356) != asuint(_1354));
                            float _1362 = _1357 ? _1356 : _1198;
                            float _1366 = (_1362 * _622) + _585;
                            float _1367 = (_1362 * _624) + _586;
                            float _1368 = (_1362 * _625) + _578;
                            uint _1370 = (_1361 ? 1u : 4294967295u) + _1208;
                            float _1371 = _1361 ? 0.5f : 2.0f;
                            float _1372 = _1371 * _1206;
                            float _1373 = _1371 * _1204;
                            float _1374 = _1361 ? 2.0f : 0.5f;
                            float _1375 = _1374 * _1202;
                            float _1376 = _1374 * _1200;
                            _1072 = _1192 + 1u;
                            uint _1807;
                            uint _1809;
                            if (int(_1370) < int(0u))
                            {
                                float _1596 = _50_m0[50u].w + _50_m0[50u].y;
                                float _1598 = _50_m0[50u].x / (_1596 - (_50_m0[50u].y * _1340));
                                float _1601 = _50_m0[50u].x / (_1596 - (_50_m0[50u].y * _1368));
                                float _1606 = abs(_801 - _1601);
                                float _1609 = _1601 - _1598;
                                float frontier_phi_105_86_ladder;
                                float frontier_phi_105_86_ladder_1;
                                float frontier_phi_105_86_ladder_2;
                                float frontier_phi_105_86_ladder_3;
                                float frontier_phi_105_86_ladder_4;
                                float frontier_phi_105_86_ladder_5;
                                float frontier_phi_105_86_ladder_6;
                                uint frontier_phi_105_86_ladder_7;
                                uint frontier_phi_105_86_ladder_8;
                                float frontier_phi_105_86_ladder_9;
                                float frontier_phi_105_86_ladder_10;
                                float frontier_phi_105_86_ladder_11;
                                float frontier_phi_105_86_ladder_12;
                                float frontier_phi_105_86_ladder_13;
                                uint frontier_phi_105_86_ladder_14;
                                if (_1609 > max(_50_m0[3u].x, _50_m0[3u].x * _1606))
                                {
                                    float _1778 = _1362 + _1193;
                                    bool _1779 = _1210 != 0u;
                                    float _1780 = _1779 ? _1216 : _1366;
                                    float _1781 = _1779 ? _1215 : _1367;
                                    uint _1788 = (_632 || (asuint(_50_m0[62u]).z == 0u)) ? 1u : _1210;
                                    float frontier_phi_105_86_ladder_103_ladder;
                                    float frontier_phi_105_86_ladder_103_ladder_1;
                                    float frontier_phi_105_86_ladder_103_ladder_2;
                                    float frontier_phi_105_86_ladder_103_ladder_3;
                                    float frontier_phi_105_86_ladder_103_ladder_4;
                                    float frontier_phi_105_86_ladder_103_ladder_5;
                                    float frontier_phi_105_86_ladder_103_ladder_6;
                                    uint frontier_phi_105_86_ladder_103_ladder_7;
                                    uint frontier_phi_105_86_ladder_103_ladder_8;
                                    float frontier_phi_105_86_ladder_103_ladder_9;
                                    float frontier_phi_105_86_ladder_103_ladder_10;
                                    float frontier_phi_105_86_ladder_103_ladder_11;
                                    float frontier_phi_105_86_ladder_103_ladder_12;
                                    float frontier_phi_105_86_ladder_103_ladder_13;
                                    uint frontier_phi_105_86_ladder_103_ladder_14;
                                    if (asuint(_50_m0[5u]).y == 0u)
                                    {
                                        frontier_phi_105_86_ladder_103_ladder = _1211;
                                        frontier_phi_105_86_ladder_103_ladder_1 = _1193;
                                        frontier_phi_105_86_ladder_103_ladder_2 = _1778;
                                        frontier_phi_105_86_ladder_103_ladder_3 = _764;
                                        frontier_phi_105_86_ladder_103_ladder_4 = _763;
                                        frontier_phi_105_86_ladder_103_ladder_5 = _752;
                                        frontier_phi_105_86_ladder_103_ladder_6 = _751;
                                        frontier_phi_105_86_ladder_103_ladder_7 = 0u;
                                        frontier_phi_105_86_ladder_103_ladder_8 = _1788;
                                        frontier_phi_105_86_ladder_103_ladder_9 = _1212;
                                        frontier_phi_105_86_ladder_103_ladder_10 = _1213;
                                        frontier_phi_105_86_ladder_103_ladder_11 = _1214;
                                        frontier_phi_105_86_ladder_103_ladder_12 = _1781;
                                        frontier_phi_105_86_ladder_103_ladder_13 = _1780;
                                        frontier_phi_105_86_ladder_103_ladder_14 = _1217;
                                    }
                                    else
                                    {
                                        frontier_phi_105_86_ladder_103_ladder = _1211;
                                        frontier_phi_105_86_ladder_103_ladder_1 = min(_819, _50_m0[6u].x * _1193);
                                        frontier_phi_105_86_ladder_103_ladder_2 = _1778;
                                        frontier_phi_105_86_ladder_103_ladder_3 = _764;
                                        frontier_phi_105_86_ladder_103_ladder_4 = _763;
                                        frontier_phi_105_86_ladder_103_ladder_5 = _752;
                                        frontier_phi_105_86_ladder_103_ladder_6 = _751;
                                        frontier_phi_105_86_ladder_103_ladder_7 = 0u;
                                        frontier_phi_105_86_ladder_103_ladder_8 = _1788;
                                        frontier_phi_105_86_ladder_103_ladder_9 = _1212;
                                        frontier_phi_105_86_ladder_103_ladder_10 = _1213;
                                        frontier_phi_105_86_ladder_103_ladder_11 = _1214;
                                        frontier_phi_105_86_ladder_103_ladder_12 = _1781;
                                        frontier_phi_105_86_ladder_103_ladder_13 = _1780;
                                        frontier_phi_105_86_ladder_103_ladder_14 = _1217;
                                    }
                                    frontier_phi_105_86_ladder = frontier_phi_105_86_ladder_103_ladder;
                                    frontier_phi_105_86_ladder_1 = frontier_phi_105_86_ladder_103_ladder_1;
                                    frontier_phi_105_86_ladder_2 = frontier_phi_105_86_ladder_103_ladder_2;
                                    frontier_phi_105_86_ladder_3 = frontier_phi_105_86_ladder_103_ladder_3;
                                    frontier_phi_105_86_ladder_4 = frontier_phi_105_86_ladder_103_ladder_4;
                                    frontier_phi_105_86_ladder_5 = frontier_phi_105_86_ladder_103_ladder_5;
                                    frontier_phi_105_86_ladder_6 = frontier_phi_105_86_ladder_103_ladder_6;
                                    frontier_phi_105_86_ladder_7 = frontier_phi_105_86_ladder_103_ladder_7;
                                    frontier_phi_105_86_ladder_8 = frontier_phi_105_86_ladder_103_ladder_8;
                                    frontier_phi_105_86_ladder_9 = frontier_phi_105_86_ladder_103_ladder_9;
                                    frontier_phi_105_86_ladder_10 = frontier_phi_105_86_ladder_103_ladder_10;
                                    frontier_phi_105_86_ladder_11 = frontier_phi_105_86_ladder_103_ladder_11;
                                    frontier_phi_105_86_ladder_12 = frontier_phi_105_86_ladder_103_ladder_12;
                                    frontier_phi_105_86_ladder_13 = frontier_phi_105_86_ladder_103_ladder_13;
                                    frontier_phi_105_86_ladder_14 = frontier_phi_105_86_ladder_103_ladder_14;
                                }
                                else
                                {
                                    float _1796 = max(_50_m0[3u].y, _50_m0[3u].y * _1606);
                                    float _1799 = _1796 * _50_m0[3u].w;
                                    float _1803 = clamp((abs(_1609) - _1799) / (_1796 - _1799), 0.0f, 1.0f);
                                    uint _1805 = uint(_1598 < _801);
                                    float frontier_phi_105_86_ladder_104_ladder;
                                    float frontier_phi_105_86_ladder_104_ladder_1;
                                    float frontier_phi_105_86_ladder_104_ladder_2;
                                    float frontier_phi_105_86_ladder_104_ladder_3;
                                    float frontier_phi_105_86_ladder_104_ladder_4;
                                    float frontier_phi_105_86_ladder_104_ladder_5;
                                    float frontier_phi_105_86_ladder_104_ladder_6;
                                    uint frontier_phi_105_86_ladder_104_ladder_7;
                                    uint frontier_phi_105_86_ladder_104_ladder_8;
                                    float frontier_phi_105_86_ladder_104_ladder_9;
                                    float frontier_phi_105_86_ladder_104_ladder_10;
                                    float frontier_phi_105_86_ladder_104_ladder_11;
                                    float frontier_phi_105_86_ladder_104_ladder_12;
                                    float frontier_phi_105_86_ladder_104_ladder_13;
                                    uint frontier_phi_105_86_ladder_104_ladder_14;
                                    if (_1210 == 0u)
                                    {
                                        frontier_phi_105_86_ladder_104_ladder = _1211;
                                        frontier_phi_105_86_ladder_104_ladder_1 = _1193;
                                        frontier_phi_105_86_ladder_104_ladder_2 = _1362;
                                        frontier_phi_105_86_ladder_104_ladder_3 = _1376;
                                        frontier_phi_105_86_ladder_104_ladder_4 = _1375;
                                        frontier_phi_105_86_ladder_104_ladder_5 = _1373;
                                        frontier_phi_105_86_ladder_104_ladder_6 = _1372;
                                        frontier_phi_105_86_ladder_104_ladder_7 = _1370;
                                        frontier_phi_105_86_ladder_104_ladder_8 = uint(_1803 > 0.0f);
                                        frontier_phi_105_86_ladder_104_ladder_9 = _1212;
                                        frontier_phi_105_86_ladder_104_ladder_10 = _1213;
                                        frontier_phi_105_86_ladder_104_ladder_11 = _1803;
                                        frontier_phi_105_86_ladder_104_ladder_12 = _1215;
                                        frontier_phi_105_86_ladder_104_ladder_13 = _1216;
                                        frontier_phi_105_86_ladder_104_ladder_14 = _1805;
                                    }
                                    else
                                    {
                                        frontier_phi_105_86_ladder_104_ladder = _1211;
                                        frontier_phi_105_86_ladder_104_ladder_1 = _1193;
                                        frontier_phi_105_86_ladder_104_ladder_2 = _1362;
                                        frontier_phi_105_86_ladder_104_ladder_3 = _1376;
                                        frontier_phi_105_86_ladder_104_ladder_4 = _1375;
                                        frontier_phi_105_86_ladder_104_ladder_5 = _1373;
                                        frontier_phi_105_86_ladder_104_ladder_6 = _1372;
                                        frontier_phi_105_86_ladder_104_ladder_7 = _1370;
                                        frontier_phi_105_86_ladder_104_ladder_8 = _1210;
                                        frontier_phi_105_86_ladder_104_ladder_9 = _1212;
                                        frontier_phi_105_86_ladder_104_ladder_10 = _1213;
                                        frontier_phi_105_86_ladder_104_ladder_11 = _1803;
                                        frontier_phi_105_86_ladder_104_ladder_12 = _1215;
                                        frontier_phi_105_86_ladder_104_ladder_13 = _1216;
                                        frontier_phi_105_86_ladder_104_ladder_14 = _1805;
                                    }
                                    frontier_phi_105_86_ladder = frontier_phi_105_86_ladder_104_ladder;
                                    frontier_phi_105_86_ladder_1 = frontier_phi_105_86_ladder_104_ladder_1;
                                    frontier_phi_105_86_ladder_2 = frontier_phi_105_86_ladder_104_ladder_2;
                                    frontier_phi_105_86_ladder_3 = frontier_phi_105_86_ladder_104_ladder_3;
                                    frontier_phi_105_86_ladder_4 = frontier_phi_105_86_ladder_104_ladder_4;
                                    frontier_phi_105_86_ladder_5 = frontier_phi_105_86_ladder_104_ladder_5;
                                    frontier_phi_105_86_ladder_6 = frontier_phi_105_86_ladder_104_ladder_6;
                                    frontier_phi_105_86_ladder_7 = frontier_phi_105_86_ladder_104_ladder_7;
                                    frontier_phi_105_86_ladder_8 = frontier_phi_105_86_ladder_104_ladder_8;
                                    frontier_phi_105_86_ladder_9 = frontier_phi_105_86_ladder_104_ladder_9;
                                    frontier_phi_105_86_ladder_10 = frontier_phi_105_86_ladder_104_ladder_10;
                                    frontier_phi_105_86_ladder_11 = frontier_phi_105_86_ladder_104_ladder_11;
                                    frontier_phi_105_86_ladder_12 = frontier_phi_105_86_ladder_104_ladder_12;
                                    frontier_phi_105_86_ladder_13 = frontier_phi_105_86_ladder_104_ladder_13;
                                    frontier_phi_105_86_ladder_14 = frontier_phi_105_86_ladder_104_ladder_14;
                                }
                                _1084 = frontier_phi_105_86_ladder_14;
                                _1083 = frontier_phi_105_86_ladder_13;
                                _1082 = frontier_phi_105_86_ladder_12;
                                _1081 = frontier_phi_105_86_ladder_11;
                                _1080 = frontier_phi_105_86_ladder_10;
                                _1079 = frontier_phi_105_86_ladder_9;
                                _1078 = frontier_phi_105_86_ladder;
                                _1807 = frontier_phi_105_86_ladder_8;
                                _1809 = frontier_phi_105_86_ladder_7;
                                _1207 = frontier_phi_105_86_ladder_6;
                                _1205 = frontier_phi_105_86_ladder_5;
                                _1203 = frontier_phi_105_86_ladder_4;
                                _1201 = frontier_phi_105_86_ladder_3;
                                _1199 = frontier_phi_105_86_ladder_2;
                                _1194 = frontier_phi_105_86_ladder_1;
                            }
                            else
                            {
                                bool _1611 = _1210 != 0u;
                                _1084 = _1217;
                                _1083 = _1216;
                                _1082 = _1215;
                                _1081 = _1214;
                                _1080 = _1611 ? _1213 : _1366;
                                _1079 = _1611 ? _1212 : _1367;
                                _1078 = _1611 ? _1211 : _1368;
                                _1807 = _1210;
                                _1809 = _1370;
                                _1207 = _1372;
                                _1205 = _1373;
                                _1203 = _1375;
                                _1201 = _1376;
                                _1199 = _1362;
                                _1194 = (asuint(_50_m0[5u]).y != 0u) ? _965 : _1193;
                            }
                            float frontier_phi_133_pred;
                            uint frontier_phi_133_pred_1;
                            uint frontier_phi_133_pred_2;
                            float frontier_phi_133_pred_3;
                            float frontier_phi_133_pred_4;
                            bool _1813;
                            bool _1815;
                            for (;;)
                            {
                                _1813 = _1368 < 0.0f;
                                _1815 = _1813 || ((_1366 < 0.0f) || (_1367 < 0.0f));
                                if (!_1815)
                                {
                                    if (!((_1368 > 1.0f) || ((_1366 > _50_m0[51u].x) || (_1367 > _50_m0[51u].y))))
                                    {
                                        frontier_phi_133_pred = _1367;
                                        frontier_phi_133_pred_1 = _1807;
                                        frontier_phi_133_pred_2 = _1809;
                                        frontier_phi_133_pred_3 = _1366;
                                        frontier_phi_133_pred_4 = _1368;
                                        break;
                                    }
                                }
                                if (!_1813)
                                {
                                    frontier_phi_133_pred = _1367;
                                    frontier_phi_133_pred_1 = 1u;
                                    frontier_phi_133_pred_2 = 4294967295u;
                                    frontier_phi_133_pred_3 = _1366;
                                    frontier_phi_133_pred_4 = _1368;
                                    break;
                                }
                                float _2032 = (-0.0f) - _1368;
                                float _2033 = _2032 / _625;
                                frontier_phi_133_pred = (_2033 * _624) + _1367;
                                frontier_phi_133_pred_1 = 1u;
                                frontier_phi_133_pred_2 = 4294967295u;
                                frontier_phi_133_pred_3 = (_2033 * _622) + _1366;
                                frontier_phi_133_pred_4 = _2032 + _1368;
                                break;
                            }
                            _1074 = frontier_phi_133_pred;
                            _1077 = frontier_phi_133_pred_1;
                            _1209 = frontier_phi_133_pred_2;
                            _1075 = frontier_phi_133_pred_3;
                            _1073 = frontier_phi_133_pred_4;
                            if ((_1072 < _209) && (int(_1209) > int(4294967295u)))
                            {
                                _1192 = _1072;
                                _1193 = _1194;
                                _1195 = _1073;
                                _1196 = _1074;
                                _1197 = _1075;
                                _1198 = _1199;
                                _1200 = _1201;
                                _1202 = _1203;
                                _1204 = _1205;
                                _1206 = _1207;
                                _1208 = _1209;
                                _1210 = _1077;
                                _1211 = _1078;
                                _1212 = _1079;
                                _1213 = _1080;
                                _1214 = _1081;
                                _1215 = _1082;
                                _1216 = _1083;
                                _1217 = _1084;
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        _1071 = _1072;
                        _691 = _1073;
                        _689 = _1074;
                        _687 = _1075;
                        _1076 = _1077;
                        _681 = _1078;
                        _679 = _1079;
                        _677 = _1080;
                        _675 = _1081;
                        _673 = _1082;
                        _671 = _1083;
                        _669 = _1084;
                    }
                    bool _1085 = _1071 >= _209;
                    _666 = uint(_1085);
                    _668 = _669;
                    _670 = _671;
                    _672 = _673;
                    _674 = _675;
                    _676 = _677;
                    _678 = _679;
                    _680 = _681;
                    _682 = _1085 ? 1u : _1076;
                    _684 = uint(_1071 <= _209);
                    _686 = _687;
                    _688 = _689;
                    _690 = _691;
                }
                float _698 = _50_m0[51u].z * 2.0f;
                float _701 = (_698 * _585) + (-1.0f);
                float _702 = ((1.0f - (_50_m0[51u].w * _586)) * 2.0f) + (-1.0f);
                float _718 = mad(_190, _578, mad(_183, _702, _701 * _176)) + _197;
                float _719 = (mad(_187, _578, mad(_180, _702, _701 * _173)) + _194) / _718;
                float _720 = (mad(_188, _578, mad(_181, _702, _701 * _174)) + _195) / _718;
                float _721 = (mad(_189, _578, mad(_182, _702, _701 * _175)) + _196) / _718;
                float _726 = (_698 * _686) + (-1.0f);
                float _727 = ((1.0f - (_50_m0[51u].w * _688)) * 2.0f) + (-1.0f);
                float _743 = mad(_190, _690, mad(_183, _727, _726 * _176)) + _197;
                float _747 = ((mad(_187, _690, mad(_180, _727, _726 * _173)) + _194) / _743) - _719;
                float _748 = ((mad(_188, _690, mad(_181, _727, _726 * _174)) + _195) / _743) - _720;
                float _749 = ((mad(_189, _690, mad(_182, _727, _726 * _175)) + _196) / _743) - _721;
                float _913;
                uint _915;
                float _917;
                if (_684 == 0u)
                {
                    _913 = 0.0f;
                    _915 = _682;
                    _917 = 0.0f;
                }
                else
                {
                    float _960 = float(_238);
                    float _961 = float(_239);
                    float frontier_phi_25_26_ladder;
                    uint frontier_phi_25_26_ladder_1;
                    float frontier_phi_25_26_ladder_2;
                    if ((_686 < 0.0f) || (_688 < 0.0f))
                    {
                        frontier_phi_25_26_ladder = 0.0f;
                        frontier_phi_25_26_ladder_1 = _682;
                        frontier_phi_25_26_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_25_26_ladder_34_ladder;
                        uint frontier_phi_25_26_ladder_34_ladder_1;
                        float frontier_phi_25_26_ladder_34_ladder_2;
                        if ((_690 >= 1.0f) || ((_686 > _50_m0[51u].x) || (_688 > _50_m0[51u].y)))
                        {
                            frontier_phi_25_26_ladder_34_ladder = 0.0f;
                            frontier_phi_25_26_ladder_34_ladder_1 = _682;
                            frontier_phi_25_26_ladder_34_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_25_26_ladder_34_ladder_43_ladder;
                            uint frontier_phi_25_26_ladder_34_ladder_43_ladder_1;
                            float frontier_phi_25_26_ladder_34_ladder_43_ladder_2;
                            for (;;)
                            {
                                if ((abs(_686 - _259) < (2.0f / _960)) && (abs(_688 - _260) < (2.0f / _961)))
                                {
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder = 0.0f;
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder_1 = _682;
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_85;
                                    float frontier_phi_85_pred;
                                    uint frontier_phi_85_pred_1;
                                    float frontier_phi_85_pred_2;
                                    uint _1254;
                                    uint _1255;
                                    bool _1256;
                                    for (;;)
                                    {
                                        _1254 = uint(int(_686 * _960));
                                        _1255 = uint(int(_688 * _961));
                                        _1256 = _632 && _539;
                                        if (!_1256)
                                        {
                                            if (!(dot(float3(_747, _748, _749), float3(_747, _748, _749)) < _50_m0[4u].w))
                                            {
                                                ladder_phi_85 = false;
                                                frontier_phi_85_pred = 0.0f;
                                                frontier_phi_85_pred_1 = _682;
                                                frontier_phi_85_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1323 = _24[22u].Load(int3(uint2(_1254, _1255), 0u));
                                        uint _1325 = _1323.x;
                                        float _1764;
                                        float _1765;
                                        float _1766;
                                        if (_1325 == 0u)
                                        {
                                            uint4 _1495 = _24[1u].Load(int3(uint2(_1254, _1255), 0u));
                                            uint _1497 = _1495.x;
                                            float _1505 = (float((_1497 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1506 = (float(_1497 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1510 = (1.0f - abs(_1505)) - abs(_1506);
                                            float _1512 = clamp((-0.0f) - _1510, 0.0f, 1.0f);
                                            float _1513 = (-0.0f) - _1512;
                                            _1764 = ((_1505 >= 0.0f) ? _1513 : _1512) + _1505;
                                            _1765 = ((_1506 >= 0.0f) ? _1513 : _1512) + _1506;
                                            _1766 = _1510;
                                        }
                                        else
                                        {
                                            float _1527 = (float((_1325 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1528 = (float(_1325 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1532 = (1.0f - abs(_1527)) - abs(_1528);
                                            float _1534 = clamp((-0.0f) - _1532, 0.0f, 1.0f);
                                            float _1535 = (-0.0f) - _1534;
                                            _1764 = ((_1527 >= 0.0f) ? _1535 : _1534) + _1527;
                                            _1765 = ((_1528 >= 0.0f) ? _1535 : _1534) + _1528;
                                            _1766 = _1532;
                                        }
                                        float _1770 = rsqrt(dot(float3(_1764, _1765, _1766), float3(_1764, _1765, _1766)));
                                        if (dot(float3(_1770 * _1764, _1770 * _1765, _1770 * _1766), float3(_747, _748, _749)) > 0.0f)
                                        {
                                            ladder_phi_85 = true;
                                            frontier_phi_85_pred = 0.0f;
                                            frontier_phi_85_pred_1 = _682;
                                            frontier_phi_85_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_85 = false;
                                            frontier_phi_85_pred = 0.0f;
                                            frontier_phi_85_pred_1 = _682;
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
                                    float _1546 = _50_m0[51u].z * _686;
                                    float _1547 = _50_m0[51u].w * _688;
                                    float _1549 = (_961 / _960) * 0.0500000007450580596923828125f;
                                    float _1554 = clamp(_1546 / _1549, 0.0f, 1.0f);
                                    float _1555 = clamp(_1547 * 20.0f, 0.0f, 1.0f);
                                    float _1567 = clamp(((_1546 + (-1.0f)) + _1549) / _1549, 0.0f, 1.0f);
                                    float _1568 = clamp((_1547 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1579 = _1554 * _1555;
                                    precise float _1580 = _1579 * _1579;
                                    float _1584 = ((((3.0f - (_1555 * 2.0f)) * (3.0f - (_1554 * 2.0f))) * _1580) * (1.0f - ((_1567 * _1567) * (3.0f - (_1567 * 2.0f))))) * (1.0f - ((_1568 * _1568) * (3.0f - (_1568 * 2.0f))));
                                    bool _1587 = (_682 != 0u) || (_1584 >= 1.0f);
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder = _1584 * float(_478 > 0.0f);
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder_1 = _1587 ? _682 : 1u;
                                    frontier_phi_25_26_ladder_34_ladder_43_ladder_2 = _1587 ? 0.0f : _1584;
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
                    _913 = frontier_phi_25_26_ladder_2;
                    _915 = frontier_phi_25_26_ladder_1;
                    _917 = frontier_phi_25_26_ladder;
                }
                uint _1046;
                uint _1048;
                float _958;
                for (;;)
                {
                    _958 = ((((exp2(log2(clamp((sqrt(((_720 * _720) + (_719 * _719)) + (_721 * _721)) - _45_m0[121u].y) * _45_m0[121u].z, 0.0f, 1.0f)) * _45_m0[121u].w) * _412) * exp2(log2(clamp((_720 - _45_m0[122u].x) * _45_m0[122u].y, 0.0f, 1.0f)) * _45_m0[122u].z)) * (1.0f - clamp(_398, 0.0f, 1.0f))) * (max(_45_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    if (_917 > 0.0f)
                    {
                        uint _1008 = uint((_202 * 0.999989986419677734375f) * clamp(_686, 0.0f, 1.0f));
                        uint _1009 = uint((_204 * 0.999989986419677734375f) * clamp(_688, 0.0f, 1.0f));
                        uint4 _1012 = _24[2u].Load(int3(uint2(_1008, _1009), 0u));
                        uint _1015 = _1012.w;
                        uint4 _1020 = _24[15u].Load(int3(uint2(_1008, _1009), 0u));
                        uint _1022 = _1020.y;
                        uint _1028 = ((_1022 & 64u) != 0u) ? uint((_1022 & 4294967167u) != 66u) : 4294967295u;
                        uint _1029 = _1015 & 128u;
                        uint _1031 = (_1029 != 0u) ? 1u : ((_1012.x << 7u) | _1015);
                        uint4 _1034 = _16.Load(_1031 * 4u);
                        uint _1035 = _1034.x;
                        uint _1042 = ((_1035 & 1u) != 0u) ? 0u : 18u;
                        uint _1044 = uint(min(int(uint(max(int(_1028), int(0u)))), int(1u)));
                        uint _1133;
                        if (_1029 == 0u)
                        {
                            _1133 = (((_1035 & 2097152u) != 0u) && (_1028 == _1044)) ? (_1042 | 128u) : _1042;
                        }
                        else
                        {
                            _1133 = _1015;
                        }
                        uint _1134 = _16.Load((_1031 * 4u) + 1u).x & 512u;
                        bool _1137 = (_1133 & 144u) == 0u;
                        if (_1134 == 0u)
                        {
                            if (_1137 || ((_1035 & 1u) != 0u))
                            {
                                _1046 = 0u;
                                _1048 = 0u;
                                break;
                            }
                        }
                        else
                        {
                            if (_1137)
                            {
                                _1046 = 0u;
                                _1048 = 0u;
                                break;
                            }
                        }
                        bool _1282 = ((_1133 & 128u) | _1134) != 0u;
                        uint _1047;
                        if (_1282)
                        {
                            _1047 = 1u;
                        }
                        else
                        {
                            _1047 = (((_1035 >> 14u) & 2u) ^ 2u) + 3u;
                        }
                        if (((_1035 & 268435472u) == 16u) && (((_1035 & 33554432u) == 0u) || _1282))
                        {
                            _1046 = 2u;
                            _1048 = 0u;
                            break;
                        }
                        _1046 = _1047;
                        _1048 = (_1047 == 1u) ? _1044 : 0u;
                        break;
                    }
                    else
                    {
                        _1046 = 0u;
                        _1048 = 0u;
                        break;
                    }
                }
                uint _1142;
                float _1143;
                float _1145;
                float _1147;
                float _1149;
                float _1150;
                float _1151;
                float _1153;
                float _1052;
                float _1055;
                float _1058;
                float _1062;
                float _1063;
                for (;;)
                {
                    _1052 = mad(_167, _535, mad(_161, _534, _533 * _155));
                    _1055 = mad(_168, _535, mad(_162, _534, _533 * _156));
                    _1058 = mad(_169, _535, mad(_163, _534, _533 * _157));
                    bool _1061 = (_915 != 0u) || (_917 < 1.0f);
                    _1062 = _1061 ? 0.0f : 1.0f;
                    _1063 = _1061 ? 0.0f : 0.5f;
                    if (_1061)
                    {
                        uint4 _1141 = asuint(_50_m0[60u]);
                        if (_632)
                        {
                            if (int(_410) < int(1u))
                            {
                                if (_1141.x == 0u)
                                {
                                    _1142 = 0u;
                                    _1143 = 0.0f;
                                    _1145 = 0.0f;
                                    _1147 = 0.0f;
                                    _1149 = 0.0f;
                                    _1150 = 0.0f;
                                    _1151 = 0.0f;
                                    _1153 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1141.y == 0u)
                                {
                                    _1142 = 0u;
                                    _1143 = 0.0f;
                                    _1145 = 0.0f;
                                    _1147 = 0.0f;
                                    _1149 = 0.0f;
                                    _1150 = 0.0f;
                                    _1151 = 0.0f;
                                    _1153 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1141.z == 0u)
                            {
                                _1142 = 0u;
                                _1143 = 0.0f;
                                _1145 = 0.0f;
                                _1147 = 0.0f;
                                _1149 = 0.0f;
                                _1150 = 0.0f;
                                _1151 = 0.0f;
                                _1153 = 0.0f;
                                break;
                            }
                        }
                        if (_913 > 0.0f)
                        {
                            _1142 = 0u;
                            _1143 = 0.0f;
                            _1145 = 0.0f;
                            _1147 = 0.0f;
                            _1149 = 0.0f;
                            _1150 = 1.0f;
                            _1151 = 1000.0f;
                            _1153 = 0.5f;
                            break;
                        }
                        if (((_676 <= 0.0f) || (_678 <= 0.0f)) || (_680 <= 0.0f))
                        {
                            _1142 = 0u;
                            _1143 = 0.0f;
                            _1145 = 0.0f;
                            _1147 = 0.0f;
                            _1149 = 0.0f;
                            _1150 = 1.0f;
                            _1151 = 1000.0f;
                            _1153 = 0.5f;
                            break;
                        }
                        if ((_680 >= 1.0f) || ((_676 >= _50_m0[51u].x) || (_678 >= _50_m0[51u].y)))
                        {
                            _1142 = 0u;
                            _1143 = 0.0f;
                            _1145 = 0.0f;
                            _1147 = 0.0f;
                            _1149 = 0.0f;
                            _1150 = 1.0f;
                            _1151 = 1000.0f;
                            _1153 = 0.5f;
                            break;
                        }
                        uint _2077;
                        uint _2079;
                        uint _1827;
                        uint _1828;
                        bool _1834;
                        for (;;)
                        {
                            _1827 = uint(clamp(_670, 0.0f, 1.0f) * _202);
                            _1828 = uint(clamp(_672, 0.0f, 1.0f) * _204);
                            _1834 = _20[21u].Load(int3(uint2(_1827, _1828), 0u)).x > 0.0f;
                            if (_1834)
                            {
                                uint _1936 = _24[23u].Load(int3(uint2(_1827, _1828), 0u)).y + 4294967295u;
                                _2077 = (uint(int(_1936) >> int(31u)) & 3u) + 1u;
                                _2079 = (int(_1936) < int(0u)) ? 0u : _1936;
                                break;
                            }
                            else
                            {
                                uint4 _1944 = _24[2u].Load(int3(uint2(_1827, _1828), 0u));
                                uint _1947 = _1944.w;
                                uint4 _1952 = _24[15u].Load(int3(uint2(_1827, _1828), 0u));
                                uint _1954 = _1952.y;
                                uint _1960 = ((_1954 & 64u) != 0u) ? uint((_1954 & 4294967167u) != 66u) : 4294967295u;
                                uint _1961 = _1947 & 128u;
                                uint _1963 = (_1961 != 0u) ? 1u : ((_1944.x << 7u) | _1947);
                                uint4 _1966 = _16.Load(_1963 * 4u);
                                uint _1967 = _1966.x;
                                uint _1974 = ((_1967 & 1u) != 0u) ? 0u : 18u;
                                uint _1976 = uint(min(int(uint(max(int(_1960), int(0u)))), int(1u)));
                                uint _2090;
                                if (_1961 == 0u)
                                {
                                    _2090 = (((_1967 & 2097152u) != 0u) && (_1960 == _1976)) ? (_1974 | 128u) : _1974;
                                }
                                else
                                {
                                    _2090 = _1947;
                                }
                                uint _2091 = _16.Load((_1963 * 4u) + 1u).x & 512u;
                                bool _2094 = (_2090 & 144u) == 0u;
                                if (_2091 == 0u)
                                {
                                    if (_2094 || ((_1967 & 1u) != 0u))
                                    {
                                        _2077 = 0u;
                                        _2079 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2094)
                                    {
                                        _2077 = 0u;
                                        _2079 = 0u;
                                        break;
                                    }
                                }
                                bool _2247 = ((_2090 & 128u) | _2091) != 0u;
                                uint _2078;
                                if (_2247)
                                {
                                    _2078 = 1u;
                                }
                                else
                                {
                                    _2078 = (((_1967 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_1967 & 268435472u) == 16u) && (((_1967 & 33554432u) == 0u) || _2247))
                                {
                                    _2077 = 2u;
                                    _2079 = 0u;
                                    break;
                                }
                                _2077 = _2078;
                                _2079 = (_2078 == 1u) ? _1976 : 0u;
                                break;
                            }
                        }
                        if ((_402 != _2077) || (_410 != _2079))
                        {
                            _1142 = 0u;
                            _1143 = 0.0f;
                            _1145 = 0.0f;
                            _1147 = 0.0f;
                            _1149 = 0.0f;
                            _1150 = 1.0f;
                            _1151 = 1000.0f;
                            _1153 = 0.5f;
                            break;
                        }
                        float _2165 = _676 * 2.0f;
                        float _2168 = (_50_m0[51u].z * _2165) + (-1.0f);
                        float _2169 = ((1.0f - (_50_m0[51u].w * _678)) * 2.0f) + (-1.0f);
                        float _2185 = mad(_190, _680, mad(_183, _2169, _2168 * _176)) + _197;
                        float _2186 = (mad(_187, _680, mad(_180, _2169, _2168 * _173)) + _194) / _2185;
                        float _2187 = (mad(_188, _680, mad(_181, _2169, _2168 * _174)) + _195) / _2185;
                        float _2188 = (mad(_189, _680, mad(_182, _2169, _2168 * _175)) + _196) / _2185;
                        if (sqrt(((_2187 * _2187) + (_2186 * _2186)) + (_2188 * _2188)) > _50_m0[58u].w)
                        {
                            _1142 = 0u;
                            _1143 = 0.0f;
                            _1145 = 0.0f;
                            _1147 = 0.0f;
                            _1149 = 0.0f;
                            _1150 = 0.0f;
                            _1151 = 1000.0f;
                            _1153 = 0.5f;
                            break;
                        }
                        float _2227 = _2186 - _719;
                        float _2228 = _2187 - _720;
                        float _2229 = _2188 - _721;
                        float _2235 = sqrt(((_2228 * _2228) + (_2227 * _2227)) + (_2229 * _2229));
                        float _2243 = min(_50_m0[59u].y, max(0.0f, _2235 + (-0.001000000047497451305389404296875f)));
                        float _2356;
                        if (_632)
                        {
                            _2356 = min(_50_m0[59u].x, _50_m0[4u].z + _2243);
                        }
                        else
                        {
                            _2356 = _50_m0[59u].x;
                        }
                        float _2357 = _2356 - _2235;
                        if (!(_2357 > 0.0f))
                        {
                            _1142 = 0u;
                            _1143 = 0.0f;
                            _1145 = 0.0f;
                            _1147 = 0.0f;
                            _1149 = 1.0f;
                            _1150 = 1.0f;
                            _1151 = 0.0f;
                            _1153 = 0.5f;
                            break;
                        }
                        float _2390 = _2186 - (_2243 * _1052);
                        float _2391 = _2187 - (_2243 * _1055);
                        float _2392 = _2188 - (_2243 * _1058);
                        RayDesc _2ident = {float3(mad(_2392, _50_m0[46u].z, mad(_2391, _50_m0[46u].y, _50_m0[46u].x * _2390)) + _50_m0[46u].w, mad(_2392, _50_m0[47u].z, mad(_2391, _50_m0[47u].y, _50_m0[47u].x * _2390)) + _50_m0[47u].w, mad(_2392, _50_m0[48u].z, mad(_2391, _50_m0[48u].y, _50_m0[48u].x * _2390)) + _50_m0[48u].w), 0.0f, float3(mad(_1058, _50_m0[46u].z, mad(_1055, _50_m0[46u].y, _50_m0[46u].x * _1052)), mad(_1058, _50_m0[47u].z, mad(_1055, _50_m0[47u].y, _50_m0[47u].x * _1052)), mad(_1058, _50_m0[48u].z, mad(_1055, _50_m0[48u].y, _50_m0[48u].x * _1052))), _2357};
                        _2395.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2442 = _2395.Proceed();
                        uint _2443 = _2395.CommittedStatus();
                        if (!(_2443 == 1u))
                        {
                            _1142 = 0u;
                            _1143 = 0.0f;
                            _1145 = 0.0f;
                            _1147 = 0.0f;
                            _1149 = 1.0f;
                            _1150 = 0.0f;
                            _1151 = 0.0f;
                            _1153 = 0.5f;
                            break;
                        }
                        float _2514 = _2395.CommittedRayT();
                        if (!((_2514 < _2357) && (_2514 > 0.0f)))
                        {
                            _1142 = 0u;
                            _1143 = 0.0f;
                            _1145 = 0.0f;
                            _1147 = 0.0f;
                            _1149 = 1.0f;
                            _1150 = 0.0f;
                            _1151 = 0.0f;
                            _1153 = 0.5f;
                            break;
                        }
                        float _2565 = (_50_m0[51u].z * _2165) + (-1.0f);
                        float _2566 = ((1.0f - (_50_m0[51u].w * _678)) * 2.0f) + (-1.0f);
                        float _2582 = mad(_144, _680, mad(_137, _2566, _2565 * _130)) + _151;
                        float _2586 = _2514 - _2243;
                        float _2590 = ((mad(_141, _680, mad(_134, _2566, _2565 * _127)) + _148) / _2582) + (_2586 * _533);
                        float _2591 = ((mad(_142, _680, mad(_135, _2566, _2565 * _128)) + _149) / _2582) + (_2586 * _534);
                        float _2592 = ((mad(_143, _680, mad(_136, _2566, _2565 * _129)) + _150) / _2582) + (_2586 * _535);
                        float _2604 = mad(_116, _2592, mad(_109, _2591, _2590 * _102)) + _123;
                        float _2614 = (_50_m0[51u].z * _50_m0[51u].x) * ((((mad(_113, _2592, mad(_106, _2591, _2590 * _99)) + _120) / _2604) * 0.5f) + 0.5f);
                        float _2616 = (_50_m0[51u].w * _50_m0[51u].y) * (0.5f - (((mad(_114, _2592, mad(_107, _2591, _2590 * _100)) + _121) / _2604) * 0.5f));
                        float _2618 = (_204 / _202) * 0.0500000007450580596923828125f;
                        float _2621 = clamp(_2614 / _2618, 0.0f, 1.0f);
                        float _2622 = clamp(_2616 * 20.0f, 0.0f, 1.0f);
                        float _2632 = clamp(((_2618 + (-1.0f)) + _2614) / _2618, 0.0f, 1.0f);
                        float _2633 = clamp((_2616 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2644 = _2621 * _2622;
                        precise float _2645 = _2644 * _2644;
                        if ((((((3.0f - (_2622 * 2.0f)) * (3.0f - (_2621 * 2.0f))) * _2645) * (1.0f - ((_2632 * _2632) * (3.0f - (_2632 * 2.0f))))) * (1.0f - ((_2633 * _2633) * (3.0f - (_2633 * 2.0f))))) < 1.0f)
                        {
                            _1142 = 0u;
                            _1143 = 0.0f;
                            _1145 = 0.0f;
                            _1147 = 0.0f;
                            _1149 = _1062;
                            _1150 = _1062;
                            _1151 = 0.0f;
                            _1153 = _1063;
                            break;
                        }
                        _1142 = 1u;
                        _1143 = _50_m0[1u].w;
                        _1145 = _50_m0[2u].x;
                        _1147 = _50_m0[2u].y;
                        _1149 = 0.0f;
                        _1150 = 1.0f;
                        _1151 = 0.0f;
                        _1153 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1142 = 0u;
                        _1143 = 0.0f;
                        _1145 = 0.0f;
                        _1147 = 0.0f;
                        _1149 = 1.0f;
                        _1150 = 1.0f;
                        _1151 = 0.0f;
                        _1153 = 0.5f;
                        break;
                    }
                }
                uint4 _1174 = asuint(_50_m0[55u]);
                float _1179 = clamp(log2((exp2(log2(_50_m0[0u].x * (1.0f - _406)) * _50_m0[0u].y) * _50_m0[0u].z) * _50_m0[61u].w) / float(_1174.x + 4294967295u), 0.0f, 1.0f);
                uint _1242;
                float _1243;
                float _1244;
                float _1246;
                float _1248;
                if (_632 && (_1174.z != 0u))
                {
                    uint frontier_phi_53_52_ladder;
                    float frontier_phi_53_52_ladder_1;
                    float frontier_phi_53_52_ladder_2;
                    float frontier_phi_53_52_ladder_3;
                    float frontier_phi_53_52_ladder_4;
                    if ((_1046 != 1u) || (_1048 != 0u))
                    {
                        float _1291 = _578 / max(9.9999999747524270787835121154785e-07f, (-0.0f) - _625);
                        float _1300 = ((_1291 * _622) + _585) / _50_m0[51u].x;
                        float _1301 = ((_1291 * _624) + _586) / _50_m0[51u].y;
                        float _1432;
                        if (_1301 < 0.300000011920928955078125f)
                        {
                            float _1424 = (0.300000011920928955078125f - _1301) * 3.3333332538604736328125f;
                            _1432 = 0.300000011920928955078125f - ((_1424 / sqrt((_1424 * _1424) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _1432 = _1301;
                        }
                        float _1654;
                        if (_1300 < 0.300000011920928955078125f)
                        {
                            float _1647 = (0.300000011920928955078125f - _1300) * 3.3333332538604736328125f;
                            _1654 = 0.300000011920928955078125f - ((_1647 / sqrt((_1647 * _1647) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _1654 = _1300;
                        }
                        float _1845;
                        if ((1.0f - _1654) < 0.300000011920928955078125f)
                        {
                            float _1837 = (_1654 + (-0.699999988079071044921875f)) * 3.3333332538604736328125f;
                            _1845 = ((_1837 / sqrt((_1837 * _1837) + 1.0f)) * 0.300000011920928955078125f) + 0.699999988079071044921875f;
                        }
                        else
                        {
                            _1845 = _1654;
                        }
                        frontier_phi_53_52_ladder = 0u;
                        frontier_phi_53_52_ladder_1 = 0.0f;
                        frontier_phi_53_52_ladder_2 = _1845 * _50_m0[51u].x;
                        frontier_phi_53_52_ladder_3 = 1.0f - clamp((_535 + (-0.25f)) * (-4.0f), 0.0f, 1.0f);
                        frontier_phi_53_52_ladder_4 = _1432 * _50_m0[51u].y;
                    }
                    else
                    {
                        frontier_phi_53_52_ladder = _666;
                        frontier_phi_53_52_ladder_1 = _674;
                        frontier_phi_53_52_ladder_2 = _686;
                        frontier_phi_53_52_ladder_3 = _917;
                        frontier_phi_53_52_ladder_4 = _688;
                    }
                    _1242 = frontier_phi_53_52_ladder;
                    _1243 = frontier_phi_53_52_ladder_1;
                    _1244 = frontier_phi_53_52_ladder_2;
                    _1246 = frontier_phi_53_52_ladder_4;
                    _1248 = frontier_phi_53_52_ladder_3;
                }
                else
                {
                    _1242 = _666;
                    _1243 = _674;
                    _1244 = _686;
                    _1246 = _688;
                    _1248 = _917;
                }
                bool _1250 = _402 != 1u;
                float _1673;
                float _1675;
                float _1677;
                float _1679;
                if (_1248 == 0.0f)
                {
                    float _1434;
                    float _1436;
                    float _1438;
                    float _1440;
                    if (_1142 == 0u)
                    {
                        float frontier_phi_80_79_ladder;
                        float frontier_phi_80_79_ladder_1;
                        float frontier_phi_80_79_ladder_2;
                        float frontier_phi_80_79_ladder_3;
                        if (_1250)
                        {
                            float _1657 = _1055 * _958;
                            float _1661 = rsqrt(dot(float3(_1052, _1657, _1058), float3(_1052, _1657, _1058)));
                            float4 _1671 = _28[4u].SampleLevel(_59, float3(_1661 * _1052, _1661 * _1657, _1661 * _1058), 0.0f);
                            frontier_phi_80_79_ladder = 1.0f;
                            frontier_phi_80_79_ladder_1 = _1671.z;
                            frontier_phi_80_79_ladder_2 = _1671.y;
                            frontier_phi_80_79_ladder_3 = _1671.x;
                        }
                        else
                        {
                            frontier_phi_80_79_ladder = 0.0f;
                            frontier_phi_80_79_ladder_1 = 0.0f;
                            frontier_phi_80_79_ladder_2 = 0.0f;
                            frontier_phi_80_79_ladder_3 = 0.0f;
                        }
                        _1434 = frontier_phi_80_79_ladder_3;
                        _1436 = frontier_phi_80_79_ladder_2;
                        _1438 = frontier_phi_80_79_ladder_1;
                        _1440 = frontier_phi_80_79_ladder;
                    }
                    else
                    {
                        _1434 = _1143;
                        _1436 = _1145;
                        _1438 = _1147;
                        _1440 = 1.0f;
                    }
                    _1673 = _1434 * _478;
                    _1675 = _1436 * _478;
                    _1677 = _1438 * _478;
                    _1679 = _1440 * _478;
                }
                else
                {
                    uint4 _1307 = asuint(_55_m0[0u]);
                    float _1754;
                    float _1756;
                    float _1759;
                    float _1761;
                    if (_12.Load(int3(uint2(uint(float(_1307.x) * _1244), uint(float(_1307.y) * _1246)), 0u)).x > 0.0f)
                    {
                        uint _1446_dummy_parameter;
                        uint2 _1446 = spvTextureSize(_14, 0u, _1446_dummy_parameter);
                        float4 _1455 = _14.Load(int3(uint2(uint(float(_1446.x) * _1244), uint(float(_1446.y) * _1246)), 0u));
                        float _1459 = _1455.x * 0.5f;
                        float _1460 = _1455.y * (-0.5f);
                        float4 _1479 = _13.SampleLevel(_58, float2((_50_m0[52u].z * _1459) + (_50_m0[52u].x * _1244), (_50_m0[52u].w * _1460) + (_50_m0[52u].y * _1246)), 0.0f);
                        float _1489 = _50_m0[54u].x * _1479.x;
                        float _1490 = _50_m0[54u].x * _1479.y;
                        float _1491 = _50_m0[54u].x * _1479.z;
                        float _1741;
                        if (_632)
                        {
                            float frontier_phi_99_98_ladder;
                            if ((_50_m0[50u].x / ((_50_m0[50u].w + _50_m0[50u].y) - (_50_m0[50u].y * _578))) < 5.0f)
                            {
                                float _1861 = sqrt((_1459 * _1459) + (_1460 * _1460));
                                float frontier_phi_99_98_ladder_113_ladder;
                                if (_1861 > 0.0500000007450580596923828125f)
                                {
                                    float _1985 = _1244 - _585;
                                    float _1986 = _1246 - _586;
                                    float frontier_phi_99_98_ladder_113_ladder_127_ladder;
                                    if (_1861 > sqrt((_1986 * _1986) + (_1985 * _1985)))
                                    {
                                        uint4 _2105 = asuint(_55_m0[0u]);
                                        uint _2112 = uint(float(_2105.x) * _1244);
                                        uint _2113 = uint(float(_2105.y) * _1246);
                                        uint4 _2116 = _24[2u].Load(int3(uint2(_2112, _2113), 0u));
                                        uint _2119 = _2116.w;
                                        uint4 _2124 = _24[15u].Load(int3(uint2(_2112, _2113), 0u));
                                        uint _2126 = _2124.y;
                                        uint _2132 = ((_2126 & 64u) != 0u) ? uint((_2126 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2133 = _2119 & 128u;
                                        uint _2135 = (_2133 != 0u) ? 1u : ((_2116.x << 7u) | _2119);
                                        uint4 _2138 = _16.Load(_2135 * 4u);
                                        uint _2139 = _2138.x;
                                        uint _2146 = ((_2139 & 1u) != 0u) ? 0u : 18u;
                                        uint _2210;
                                        if (_2133 == 0u)
                                        {
                                            _2210 = (((_2139 & 2097152u) != 0u) && (_2132 == uint(min(int(uint(max(int(_2132), int(0u)))), int(1u))))) ? (_2146 | 128u) : _2146;
                                        }
                                        else
                                        {
                                            _2210 = _2119;
                                        }
                                        float frontier_phi_99_98_ladder_113_ladder_127_ladder_148_ladder;
                                        if (((_2210 & 128u) | (_16.Load((_2135 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2250 = asuint(_55_m0[0u]);
                                            uint _2259 = uint(float(_2250.x) * (_1459 + _1244));
                                            uint _2260 = uint(float(_2250.y) * (_1460 + _1246));
                                            uint4 _2263 = _24[2u].Load(int3(uint2(_2259, _2260), 0u));
                                            uint _2266 = _2263.w;
                                            uint4 _2269 = _24[15u].Load(int3(uint2(_2259, _2260), 0u));
                                            uint _2271 = _2269.y;
                                            uint _2277 = ((_2271 & 64u) != 0u) ? uint((_2271 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2278 = _2266 & 128u;
                                            uint _2280 = (_2278 != 0u) ? 1u : ((_2263.x << 7u) | _2266);
                                            uint4 _2282 = _16.Load(_2280 * 4u);
                                            uint _2283 = _2282.x;
                                            uint _2290 = ((_2283 & 1u) != 0u) ? 0u : 18u;
                                            uint _2377;
                                            if (_2278 == 0u)
                                            {
                                                _2377 = (((_2283 & 2097152u) != 0u) && (_2277 == uint(min(int(uint(max(int(_2277), int(0u)))), int(1u))))) ? (_2290 | 128u) : _2290;
                                            }
                                            else
                                            {
                                                _2377 = _2266;
                                            }
                                            float frontier_phi_99_98_ladder_113_ladder_127_ladder_148_ladder_162_ladder;
                                            if ((_2377 & 128u) == 0u)
                                            {
                                                frontier_phi_99_98_ladder_113_ladder_127_ladder_148_ladder_162_ladder = ((_16.Load((_2280 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_99_98_ladder_113_ladder_127_ladder_148_ladder_162_ladder = 0.0f;
                                            }
                                            frontier_phi_99_98_ladder_113_ladder_127_ladder_148_ladder = frontier_phi_99_98_ladder_113_ladder_127_ladder_148_ladder_162_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_99_98_ladder_113_ladder_127_ladder_148_ladder = 1.0f;
                                        }
                                        frontier_phi_99_98_ladder_113_ladder_127_ladder = frontier_phi_99_98_ladder_113_ladder_127_ladder_148_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_99_98_ladder_113_ladder_127_ladder = 1.0f;
                                    }
                                    frontier_phi_99_98_ladder_113_ladder = frontier_phi_99_98_ladder_113_ladder_127_ladder;
                                }
                                else
                                {
                                    frontier_phi_99_98_ladder_113_ladder = 1.0f;
                                }
                                frontier_phi_99_98_ladder = frontier_phi_99_98_ladder_113_ladder;
                            }
                            else
                            {
                                frontier_phi_99_98_ladder = 1.0f;
                            }
                            _1741 = frontier_phi_99_98_ladder;
                        }
                        else
                        {
                            _1741 = 1.0f;
                        }
                        float frontier_phi_101_99_ladder;
                        float frontier_phi_101_99_ladder_1;
                        float frontier_phi_101_99_ladder_2;
                        float frontier_phi_101_99_ladder_3;
                        float _1743;
                        for (;;)
                        {
                            _1743 = _1741 * _1248;
                            if (_1243 > 0.0f)
                            {
                                float _1758;
                                float _1760;
                                float _1762;
                                if (_632)
                                {
                                    _1758 = 0.0f;
                                    _1760 = 0.0f;
                                    _1762 = 0.0f;
                                }
                                else
                                {
                                    if (!((_402 != 4u) || (_668 != 0u)))
                                    {
                                        frontier_phi_101_99_ladder = _1743;
                                        frontier_phi_101_99_ladder_1 = _1489;
                                        frontier_phi_101_99_ladder_2 = _1490;
                                        frontier_phi_101_99_ladder_3 = _1491;
                                        break;
                                    }
                                    _1758 = _1489;
                                    _1760 = _1490;
                                    _1762 = _1491;
                                }
                                frontier_phi_101_99_ladder = (1.0f - exp2(log2(_1243) * _50_m0[4u].x)) * _1743;
                                frontier_phi_101_99_ladder_1 = _1758;
                                frontier_phi_101_99_ladder_2 = _1760;
                                frontier_phi_101_99_ladder_3 = _1762;
                                break;
                            }
                            else
                            {
                                frontier_phi_101_99_ladder = _1743;
                                frontier_phi_101_99_ladder_1 = _1489;
                                frontier_phi_101_99_ladder_2 = _1490;
                                frontier_phi_101_99_ladder_3 = _1491;
                                break;
                            }
                        }
                        _1754 = frontier_phi_101_99_ladder;
                        _1756 = frontier_phi_101_99_ladder_1;
                        _1759 = frontier_phi_101_99_ladder_2;
                        _1761 = frontier_phi_101_99_ladder_3;
                    }
                    else
                    {
                        float frontier_phi_101_82_ladder;
                        float frontier_phi_101_82_ladder_1;
                        float frontier_phi_101_82_ladder_2;
                        float frontier_phi_101_82_ladder_3;
                        if (_1242 == 0u)
                        {
                            float4 _1749 = _28[7u].SampleLevel(_59, float3(_1052, _1055, _1058), 0.0f);
                            frontier_phi_101_82_ladder = _1248;
                            frontier_phi_101_82_ladder_1 = _1749.x;
                            frontier_phi_101_82_ladder_2 = _1749.y;
                            frontier_phi_101_82_ladder_3 = _1749.z;
                        }
                        else
                        {
                            frontier_phi_101_82_ladder = 0.0f;
                            frontier_phi_101_82_ladder_1 = _1757;
                            frontier_phi_101_82_ladder_2 = _1757;
                            frontier_phi_101_82_ladder_3 = _1757;
                        }
                        _1754 = frontier_phi_101_82_ladder;
                        _1756 = frontier_phi_101_82_ladder_1;
                        _1759 = frontier_phi_101_82_ladder_2;
                        _1761 = frontier_phi_101_82_ladder_3;
                    }
                    float _2028;
                    float _2029;
                    float _2030;
                    float _2031;
                    if (_1142 == 0u)
                    {
                        float frontier_phi_131_115_ladder;
                        float frontier_phi_131_115_ladder_1;
                        float frontier_phi_131_115_ladder_2;
                        float frontier_phi_131_115_ladder_3;
                        if (_1250 && (_1754 < 1.0f))
                        {
                            float _2002 = _1055 * _958;
                            float _2006 = rsqrt(dot(float3(_1052, _2002, _1058), float3(_1052, _2002, _1058)));
                            float4 _2014 = _28[4u].SampleLevel(_59, float3(_2006 * _1052, _2006 * _2002, _2006 * _1058), 0.0f);
                            float _2016 = _2014.x;
                            float _2017 = _2014.y;
                            float _2018 = _2014.z;
                            frontier_phi_131_115_ladder = ((_1761 - _2018) * _1754) + _2018;
                            frontier_phi_131_115_ladder_1 = ((_1759 - _2017) * _1754) + _2017;
                            frontier_phi_131_115_ladder_2 = ((_1756 - _2016) * _1754) + _2016;
                            frontier_phi_131_115_ladder_3 = 1.0f;
                        }
                        else
                        {
                            frontier_phi_131_115_ladder = _1761;
                            frontier_phi_131_115_ladder_1 = _1759;
                            frontier_phi_131_115_ladder_2 = _1756;
                            frontier_phi_131_115_ladder_3 = _1754;
                        }
                        _2028 = frontier_phi_131_115_ladder_3;
                        _2029 = frontier_phi_131_115_ladder_2;
                        _2030 = frontier_phi_131_115_ladder_1;
                        _2031 = frontier_phi_131_115_ladder;
                    }
                    else
                    {
                        _2028 = 1.0f;
                        _2029 = ((_1756 - _1143) * _913) + _1143;
                        _2030 = ((_1759 - _1145) * _913) + _1145;
                        _2031 = ((_1761 - _1147) * _913) + _1147;
                    }
                    float _1680 = _2028 * _478;
                    _1673 = _2029 * _1680;
                    _1675 = _2030 * _1680;
                    _1677 = _2031 * _1680;
                    _1679 = _1680;
                }
                float _1684 = _50_m0[58u].z * _1153;
                float _1706 = max(_50_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _1708 = _1706 * ((_1684 * ((_1149 * 1000.0f) - _1673)) + _1673);
                float _1709 = _1706 * ((_1684 * ((_1150 * 1000.0f) - _1675)) + _1675);
                float _1710 = _1706 * ((_1684 * (_1151 - _1677)) + _1677);
                float _1716 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_1708, max(_1709, _1710)) + 1.0f);
                float _1720 = min(_1716 * _1708, 0.996078431606292724609375f);
                float _1722 = min(_1716 * _1709, 0.996078431606292724609375f);
                float _1723 = min(_1716 * _1710, 0.996078431606292724609375f);
                _35[uint2(_226, _229)] = float4(_1720, _1722, _1723, _1679);
                _39[uint2(_226, _229)] = float4(_1179, 0.0f, 0.0f, _1179);
                if (_233)
                {
                    uint _1851 = _226 + 1u;
                    _35[uint2(_1851, _229)] = float4(_1720, _1722, _1723, _1679);
                    _39[uint2(_1851, _229)] = float4(_1179, 0.0f, 0.0f, _1179);
                }
                if (_236)
                {
                    uint _1978 = _229 + 1u;
                    _35[uint2(_226, _1978)] = float4(_1720, _1722, _1723, _1679);
                    _39[uint2(_226, _1978)] = float4(_1179, 0.0f, 0.0f, _1179);
                }
                if (_237)
                {
                    uint _2095 = _226 + 1u;
                    uint _2096 = _229 + 1u;
                    _35[uint2(_2095, _2096)] = float4(_1720, _1722, _1723, _1679);
                    _39[uint2(_2095, _2096)] = float4(_1179, 0.0f, 0.0f, _1179);
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
                uint _659 = _229 + 1u;
                _35[uint2(_226, _659)] = 0.0f.xxxx;
                _39[uint2(_226, _659)] = 0.0f.xxxx;
            }
            if (!_237)
            {
                break;
            }
            uint _905 = _226 + 1u;
            uint _906 = _229 + 1u;
            _35[uint2(_905, _906)] = 0.0f.xxxx;
            _39[uint2(_905, _906)] = 0.0f.xxxx;
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
