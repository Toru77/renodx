static float _2583;
static uint _3342;
static uint _3343;
static uint _3344;
static float _3345;
static float _3346;
static float _3347;
static float _3348;
static uint _3349;
static float _3350;
static float _3351;
static float _3352;
static float _3353;
static float _3354;
static uint _3355;
static float _3356;
static float _3362;
static float _3363;
static float _3364;
static float _3335;
static float _3336;

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


Buffer<uint4> _13 : register(t0, space0);
Texture2D<float4> _15 : register(t1, space0);
Texture2D<float4> _16 : register(t2, space0);
Texture2D<float4> _17 : register(t3, space0);
Texture2D<float4> _18 : register(t4, space0);
Buffer<uint4> _19 : register(t96, space0);
Buffer<uint4> _20 : register(t98, space0);
Texture2D<float4> _23[] : register(t0, space37);
Texture2D<uint4> _27[] : register(t0, space38);
TextureCube<float4> _31[] : register(t0, space41);
RWTexture2D<float4> _34 : register(u0, space0);
RWBuffer<uint> _37 : register(u1, space0);
RWTexture2D<float4> _38 : register(u2, space0);
SamplerState _57 : register(s0, space0);
SamplerState _58 : register(s5, space0);
SamplerState _59 : register(s11, space0);

static uint3 gl_WorkGroupID;
static uint gl_LocalInvocationIndex;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint gl_LocalInvocationIndex : SV_GroupIndex;
};

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
        uint4 _76 = asuint(_44_m0[176u]);
        _77 = _76.z;
        _81 = _49_m0[14u].x;
        _82 = _49_m0[14u].y;
        _83 = _49_m0[14u].z;
        _87 = _49_m0[15u].x;
        _88 = _49_m0[15u].y;
        _89 = _49_m0[15u].z;
        _93 = _49_m0[16u].x;
        _94 = _49_m0[16u].y;
        _95 = _49_m0[16u].z;
        _99 = _49_m0[22u].x;
        _100 = _49_m0[22u].y;
        _101 = _49_m0[22u].z;
        _102 = _49_m0[22u].w;
        _106 = _49_m0[23u].x;
        _107 = _49_m0[23u].y;
        _108 = _49_m0[23u].z;
        _109 = _49_m0[23u].w;
        _113 = _49_m0[24u].x;
        _114 = _49_m0[24u].y;
        _115 = _49_m0[24u].z;
        _116 = _49_m0[24u].w;
        _120 = _49_m0[25u].x;
        _121 = _49_m0[25u].y;
        _122 = _49_m0[25u].z;
        _123 = _49_m0[25u].w;
        _127 = _49_m0[26u].x;
        _128 = _49_m0[26u].y;
        _129 = _49_m0[26u].z;
        _130 = _49_m0[26u].w;
        _134 = _49_m0[27u].x;
        _135 = _49_m0[27u].y;
        _136 = _49_m0[27u].z;
        _137 = _49_m0[27u].w;
        _141 = _49_m0[28u].x;
        _142 = _49_m0[28u].y;
        _143 = _49_m0[28u].z;
        _144 = _49_m0[28u].w;
        _148 = _49_m0[29u].x;
        _149 = _49_m0[29u].y;
        _150 = _49_m0[29u].z;
        _151 = _49_m0[29u].w;
        _155 = _49_m0[18u].x;
        _156 = _49_m0[18u].y;
        _157 = _49_m0[18u].z;
        _161 = _49_m0[19u].x;
        _162 = _49_m0[19u].y;
        _163 = _49_m0[19u].z;
        _167 = _49_m0[20u].x;
        _168 = _49_m0[20u].y;
        _169 = _49_m0[20u].z;
        _173 = _49_m0[30u].x;
        _174 = _49_m0[30u].y;
        _175 = _49_m0[30u].z;
        _176 = _49_m0[30u].w;
        _180 = _49_m0[31u].x;
        _181 = _49_m0[31u].y;
        _182 = _49_m0[31u].z;
        _183 = _49_m0[31u].w;
        _187 = _49_m0[32u].x;
        _188 = _49_m0[32u].y;
        _189 = _49_m0[32u].z;
        _190 = _49_m0[32u].w;
        _194 = _49_m0[33u].x;
        _195 = _49_m0[33u].y;
        _196 = _49_m0[33u].z;
        _197 = _49_m0[33u].w;
        uint4 _200 = asuint(_54_m0[0u]);
        _202 = float(_200.x);
        _204 = float(_200.y);
        uint4 _208 = asuint(_49_m0[5u]);
        _209 = _208.x;
        _214 = (((gl_WorkGroupID.y << 6u) + gl_WorkGroupID.x) << 6u) + gl_LocalInvocationIndex;
        if (_214 < _37[1u].xxxx.x)
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
                uint4 _224 = _13.Load(_214);
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
                _259 = ((_252 + 0.5f) + (_242 ? _44_m0[58u].x : _44_m0[58u].z)) * (1.0f / _202);
                _260 = ((_253 + 0.5f) + (_242 ? _44_m0[58u].y : _44_m0[58u].w)) * (1.0f / _204);
                _267 = _23[21u].Load(int3(uint2(_226, _229), 0u)).x;
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
                    uint4 _275 = _27[22u].Load(int3(uint2(_270, _271), 0u));
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
                    uint _327 = uint(int(_27[23u].Load(int3(uint2(_270, _271), 0u)).y + 4294967295u) >> int(31u)) & 3u;
                    uint _330 = _327 + 103u;
                    float _339 = clamp((_317 - _44_m0[_330].x) / (_44_m0[_330].y - _44_m0[_330].x), 0.0f, 1.0f);
                    _396 = _306 * _312;
                    _398 = _307 * _312;
                    _400 = _312 * _297;
                    _402 = _327 + 1u;
                    _404 = (_339 * _339) * (3.0f - (_339 * 2.0f));
                    _406 = _317;
                    _408 = _44_m0[_330].z;
                    _410 = asuint(_44_m0[_330]).w;
                    _412 = 0.0f;
                }
                else
                {
                    uint4 _354 = _27[2u].Load(int3(uint2(_270, _271), 0u));
                    uint _356 = _354.x;
                    uint _357 = _354.w;
                    uint4 _363 = _27[15u].Load(int3(uint2(_270, _271), 0u));
                    uint _365 = _363.y;
                    uint _374 = ((_365 & 64u) != 0u) ? uint((_365 & 4294967167u) != 66u) : 4294967295u;
                    uint _375 = _357 & 128u;
                    uint _378 = (_375 != 0u) ? 1u : ((_356 << 7u) | _357);
                    uint4 _382 = _19.Load(_378 * 4u);
                    uint _383 = _382.x;
                    uint4 _386 = _19.Load((_378 * 4u) + 1u);
                    uint _387 = _386.x;
                    uint4 _390 = _19.Load((_378 * 4u) + 3u);
                    uint _391 = _390.x;
                    uint _394 = ((_383 & 1u) != 0u) ? 0u : 18u;
                    uint _435;
                    uint _436;
                    if (_375 == 0u)
                    {
                        _435 = (((_383 & 2097152u) != 0u) && (_374 == uint(min(int(uint(max(int(_374), int(0u)))), int(1u))))) ? (_394 | 128u) : _394;
                        _436 = _383;
                    }
                    else
                    {
                        _435 = _357;
                        _436 = _383 | ((_356 << 20u) & 134217728u);
                    }
                    uint _445 = _387 & 512u;
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
                    float _827;
                    if (_445 == 0u)
                    {
                        if (!((_436 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            frontier_phi_8_pred = 0.0f;
                            break;
                        }
                        float _587 = asfloat(_20.Load((_391 * 115u) + 33u).x);
                        uint4 _595 = _27[2u].Load(int3(uint2(_270, _271), 0u));
                        uint _597 = _595.y;
                        uint _598 = _435 & 128u;
                        uint _793;
                        uint _794;
                        uint _795;
                        uint _796;
                        if (_598 == 0u)
                        {
                            _793 = uint(((_436 & 817889384u) | (_387 & 576u)) != 0u) | (((_436 >> 19u) & 1u) ^ 1u);
                            _794 = uint(((_436 & 17825808u) | (_387 & 520u)) != 0u);
                            _795 = uint(((_436 & 46137344u) | (_387 & 2564u)) != 0u);
                            _796 = 0u;
                        }
                        else
                        {
                            _793 = 1u;
                            _794 = _435 & 1u;
                            _795 = 1u;
                            _796 = 1u;
                        }
                        precise float _800 = float(_597 & 127u) * 0.0078740157186985015869140625f;
                        bool _804 = (_436 & 4194304u) == 0u;
                        float _1105;
                        if (_804)
                        {
                            _1105 = _800;
                        }
                        else
                        {
                            _1105 = float(_597 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1371;
                        if ((_436 & 134217728u) == 0u)
                        {
                            uint frontier_phi_47_40_ladder;
                            if ((_598 != 0u) || ((_436 & 17825792u) == 1048576u))
                            {
                                frontier_phi_47_40_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_47_40_ladder = _794;
                            }
                            _1371 = frontier_phi_47_40_ladder;
                        }
                        else
                        {
                            _1371 = _794;
                        }
                        uint4 _1374 = _27[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _1376 = _1374.x;
                        float _1547;
                        float _1549;
                        float _1551;
                        if (_793 == 0u)
                        {
                            _1547 = 0.0f;
                            _1549 = 0.0f;
                            _1551 = 0.0f;
                        }
                        else
                        {
                            float4 _1556 = _23[8u].Load(int3(uint2(_270, _271), 0u));
                            _1547 = _1556.x;
                            _1549 = _1556.y;
                            _1551 = _1556.z;
                        }
                        uint _1811;
                        if (_1371 == 0u)
                        {
                            _1811 = 0u;
                        }
                        else
                        {
                            _1811 = _27[9u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        uint _1928;
                        if (_795 == 0u)
                        {
                            _1928 = 0u;
                        }
                        else
                        {
                            _1928 = _27[10u].Load(int3(uint2(_270, _271), 0u)).x;
                        }
                        float _1938 = (float((_1376 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1939 = (float(_1376 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1943 = (1.0f - abs(_1938)) - abs(_1939);
                        float _1945 = clamp((-0.0f) - _1943, 0.0f, 1.0f);
                        float _1946 = (-0.0f) - _1945;
                        float _1951 = ((_1938 >= 0.0f) ? _1946 : _1945) + _1938;
                        float _1952 = ((_1939 >= 0.0f) ? _1946 : _1945) + _1939;
                        float _1956 = rsqrt(dot(float3(_1951, _1952, _1943), float3(_1951, _1952, _1943)));
                        float _1957 = _1951 * _1956;
                        float _1958 = _1952 * _1956;
                        float _1959 = _1956 * _1943;
                        float _806 = float(_1376 & 255u);
                        float _1963 = ((_436 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _2288;
                        float _2289;
                        float _2290;
                        float _2291;
                        uint _2292;
                        if ((_387 & 64u) == 0u)
                        {
                            float frontier_phi_91_83_ladder;
                            float frontier_phi_91_83_ladder_1;
                            float frontier_phi_91_83_ladder_2;
                            float frontier_phi_91_83_ladder_3;
                            uint frontier_phi_91_83_ladder_4;
                            if ((_436 & 276824064u) == 0u)
                            {
                                frontier_phi_91_83_ladder = 0.0f;
                                frontier_phi_91_83_ladder_1 = ((_436 & 8u) != 0u) ? _1549 : _1963;
                                frontier_phi_91_83_ladder_2 = 0.0f;
                                frontier_phi_91_83_ladder_3 = 0.0f;
                                frontier_phi_91_83_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_91_83_ladder = 0.0f;
                                frontier_phi_91_83_ladder_1 = _1963;
                                frontier_phi_91_83_ladder_2 = 0.0f;
                                frontier_phi_91_83_ladder_3 = 0.0f;
                                frontier_phi_91_83_ladder_4 = 0u;
                            }
                            _2288 = frontier_phi_91_83_ladder_1;
                            _2289 = frontier_phi_91_83_ladder_2;
                            _2290 = frontier_phi_91_83_ladder;
                            _2291 = frontier_phi_91_83_ladder_3;
                            _2292 = frontier_phi_91_83_ladder_4;
                        }
                        else
                        {
                            float _2174 = (_1549 * 2.0f) + (-1.0f);
                            float _2175 = (_1551 * 2.0f) + (-1.0f);
                            float _2179 = (1.0f - abs(_2174)) - abs(_2175);
                            float _2181 = clamp((-0.0f) - _2179, 0.0f, 1.0f);
                            float _2182 = (-0.0f) - _2181;
                            float _2187 = ((_2174 >= 0.0f) ? _2182 : _2181) + _2174;
                            float _2188 = ((_2175 >= 0.0f) ? _2182 : _2181) + _2175;
                            float _2192 = rsqrt(dot(float3(_2187, _2188, _2179), float3(_2187, _2188, _2179)));
                            _2288 = floor(round(_1547 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _2289 = _2187 * _2192;
                            _2290 = _2188 * _2192;
                            _2291 = _2192 * _2179;
                            _2292 = 1u;
                        }
                        float _814;
                        if ((_436 & 32768u) == 0u)
                        {
                            _814 = _2288;
                        }
                        else
                        {
                            float frontier_phi_97_98_ladder;
                            if (_20.Load((_391 * 115u) + 36u).x == 0u)
                            {
                                float _2516 = clamp((_806 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _587;
                                frontier_phi_97_98_ladder = ((_436 & 131072u) != 0u) ? _2516 : ((((clamp((1.21000003814697265625f / (exp2((_1105 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_20.Load((_391 * 115u) + 32u).x)) + 1.0f) * _2516);
                            }
                            else
                            {
                                frontier_phi_97_98_ladder = _587;
                            }
                            _814 = frontier_phi_97_98_ladder;
                        }
                        uint _2339 = _435 & 1u;
                        float _2459;
                        float _2461;
                        float _2463;
                        uint _2465;
                        if (((_436 & 16u) == 0u) || (((_2339 | (_387 & 8u)) | (_436 & 16777216u)) != 0u))
                        {
                            _2459 = _2289;
                            _2461 = _2290;
                            _2463 = _2291;
                            _2465 = _2292;
                        }
                        else
                        {
                            float _2475 = (float(_1811 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2476 = (float(_1811 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2480 = (1.0f - abs(_2475)) - abs(_2476);
                            float _2482 = clamp((-0.0f) - _2480, 0.0f, 1.0f);
                            float _2483 = (-0.0f) - _2482;
                            float _2488 = ((_2475 >= 0.0f) ? _2483 : _2482) + _2475;
                            float _2489 = ((_2476 >= 0.0f) ? _2483 : _2482) + _2476;
                            float _2493 = rsqrt(dot(float3(_2488, _2489, _2480), float3(_2488, _2489, _2480)));
                            _2459 = _2488 * _2493;
                            _2461 = _2489 * _2493;
                            _2463 = _2493 * _2480;
                            _2465 = 1u;
                        }
                        float _808;
                        float _810;
                        float _812;
                        if (_2339 == 0u)
                        {
                            float frontier_phi_125_124_ladder;
                            float frontier_phi_125_124_ladder_1;
                            float frontier_phi_125_124_ladder_2;
                            if (((_435 & 64u) == 0u) && (_796 != 0u))
                            {
                                float2 _2814 = spvUnpackHalf2x16((_1928 >> 17u) & 32736u);
                                float _2815 = _2814.x;
                                float _2818 = (spvUnpackHalf2x16((_1928 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2819 = (spvUnpackHalf2x16((_1928 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2823 = (1.0f - abs(_2818)) - abs(_2819);
                                float _2825 = clamp((-0.0f) - _2823, 0.0f, 1.0f);
                                float _2826 = (-0.0f) - _2825;
                                float _2831 = ((_2818 >= 0.0f) ? _2826 : _2825) + _2818;
                                float _2832 = ((_2819 >= 0.0f) ? _2826 : _2825) + _2819;
                                float _2836 = rsqrt(dot(float3(_2831, _2832, _2823), float3(_2831, _2832, _2823)));
                                float _2846 = (((_2831 * _2836) - _1957) * _2815) + _1957;
                                float _2847 = (((_2832 * _2836) - _1958) * _2815) + _1958;
                                float _2848 = (((_2836 * _2823) - _1959) * _2815) + _1959;
                                float _2852 = rsqrt(dot(float3(_2846, _2847, _2848), float3(_2846, _2847, _2848)));
                                frontier_phi_125_124_ladder = _2846 * _2852;
                                frontier_phi_125_124_ladder_1 = _2847 * _2852;
                                frontier_phi_125_124_ladder_2 = _2848 * _2852;
                            }
                            else
                            {
                                frontier_phi_125_124_ladder = _1957;
                                frontier_phi_125_124_ladder_1 = _1958;
                                frontier_phi_125_124_ladder_2 = _1959;
                            }
                            _808 = frontier_phi_125_124_ladder;
                            _810 = frontier_phi_125_124_ladder_1;
                            _812 = frontier_phi_125_124_ladder_2;
                        }
                        else
                        {
                            _808 = _1957;
                            _810 = _1958;
                            _812 = _1959;
                        }
                        float _822;
                        float _824;
                        float _826;
                        float _828;
                        if (_804)
                        {
                            float frontier_phi_140_139_ladder;
                            float frontier_phi_140_139_ladder_1;
                            float frontier_phi_140_139_ladder_2;
                            float frontier_phi_140_139_ladder_3;
                            if (((_436 & 33554432u) == 0u) || (((_387 & 4u) != 0u) && ((_436 & 8388608u) == 0u)))
                            {
                                frontier_phi_140_139_ladder = 0.0f;
                                frontier_phi_140_139_ladder_1 = 0.0f;
                                frontier_phi_140_139_ladder_2 = 0.0f;
                                frontier_phi_140_139_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2898 = (spvUnpackHalf2x16((_1928 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2899 = (spvUnpackHalf2x16((_1928 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2903 = (1.0f - abs(_2898)) - abs(_2899);
                                float _2905 = clamp((-0.0f) - _2903, 0.0f, 1.0f);
                                float _2906 = (-0.0f) - _2905;
                                float _2911 = ((_2898 >= 0.0f) ? _2906 : _2905) + _2898;
                                float _2912 = ((_2899 >= 0.0f) ? _2906 : _2905) + _2899;
                                float _2916 = rsqrt(dot(float3(_2911, _2912, _2903), float3(_2911, _2912, _2903)));
                                float _2917 = _2911 * _2916;
                                float _2918 = _2912 * _2916;
                                float _2919 = _2916 * _2903;
                                float _2923 = rsqrt(dot(float3(_2917, _2918, _2919), float3(_2917, _2918, _2919)));
                                frontier_phi_140_139_ladder = _2923 * _2919;
                                frontier_phi_140_139_ladder_1 = _2923 * _2918;
                                frontier_phi_140_139_ladder_2 = _2923 * _2917;
                                frontier_phi_140_139_ladder_3 = spvUnpackHalf2x16((_1928 >> 17u) & 32736u).x;
                            }
                            _822 = frontier_phi_140_139_ladder_3;
                            _824 = frontier_phi_140_139_ladder_2;
                            _826 = frontier_phi_140_139_ladder_1;
                            _828 = frontier_phi_140_139_ladder;
                        }
                        else
                        {
                            _822 = 0.0f;
                            _824 = 0.0f;
                            _826 = 0.0f;
                            _828 = 0.0f;
                        }
                        bool _2866 = _2465 != 0u;
                        _805 = _806;
                        _807 = _808;
                        _809 = _810;
                        _811 = _812;
                        _813 = _814;
                        _815 = _2866 ? _2459 : _808;
                        _817 = _2866 ? _2461 : _810;
                        _819 = _2866 ? _2463 : _812;
                        _821 = _822;
                        _823 = _824;
                        _825 = _826;
                        _827 = _828;
                    }
                    else
                    {
                        uint4 _547 = _27[1u].Load(int3(uint2(_270, _271), 0u));
                        uint _549 = _547.x;
                        uint4 _553 = _27[9u].Load(int3(uint2(_270, _271), 0u));
                        uint _555 = _553.x;
                        float _729;
                        float _730;
                        float _731;
                        if ((_436 & 33554432u) == 0u)
                        {
                            float _607 = (float((_549 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _608 = (float(_549 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _612 = (1.0f - abs(_607)) - abs(_608);
                            float _614 = clamp((-0.0f) - _612, 0.0f, 1.0f);
                            float _615 = (-0.0f) - _614;
                            float _620 = ((_607 >= 0.0f) ? _615 : _614) + _607;
                            float _621 = ((_608 >= 0.0f) ? _615 : _614) + _608;
                            float _625 = rsqrt(dot(float3(_620, _621, _612), float3(_620, _621, _612)));
                            _729 = _620 * _625;
                            _730 = _621 * _625;
                            _731 = _625 * _612;
                        }
                        else
                        {
                            float _636 = (float((_555 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _637 = (float(_555 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _641 = (1.0f - abs(_636)) - abs(_637);
                            float _643 = clamp((-0.0f) - _641, 0.0f, 1.0f);
                            float _644 = (-0.0f) - _643;
                            float _649 = ((_636 >= 0.0f) ? _644 : _643) + _636;
                            float _650 = ((_637 >= 0.0f) ? _644 : _643) + _637;
                            float _654 = rsqrt(dot(float3(_649, _650, _641), float3(_649, _650, _641)));
                            _729 = _649 * _654;
                            _730 = _650 * _654;
                            _731 = _654 * _641;
                        }
                        _805 = float(_549 & 255u);
                        _807 = _729;
                        _809 = _730;
                        _811 = _731;
                        _813 = 1.0f;
                        _815 = _729;
                        _817 = _730;
                        _819 = _731;
                        _821 = 0.0f;
                        _823 = 0.0f;
                        _825 = 0.0f;
                        _827 = 0.0f;
                    }
                    precise float _829 = _805 * 0.0039215688593685626983642578125f;
                    if ((_435 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        frontier_phi_8_pred = 0.0f;
                        break;
                    }
                    bool _1114 = ((_435 & 128u) | _445) != 0u;
                    uint _1130;
                    if (_1114)
                    {
                        _1130 = 1u;
                    }
                    else
                    {
                        _1130 = (((_436 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _397;
                    float _399;
                    float _401;
                    uint _403;
                    float _407;
                    if (((_436 & 33554432u) == 0u) || _1114)
                    {
                        bool _1378 = _77 != 0u;
                        uint _1385;
                        if ((_436 & 16u) == 0u)
                        {
                            _1385 = _1130;
                        }
                        else
                        {
                            _1385 = ((_436 & 268435456u) != 0u) ? _1130 : 2u;
                        }
                        _407 = _813 * _829;
                        _397 = _1378 ? _815 : _807;
                        _399 = _1378 ? _817 : _809;
                        _401 = _1378 ? _819 : _811;
                        _403 = _1385;
                    }
                    else
                    {
                        _407 = _821;
                        _397 = _823;
                        _399 = _825;
                        _401 = _827;
                        _403 = _1130;
                    }
                    uint _1386 = _403 + 102u;
                    float _1395 = clamp((_407 - _44_m0[_1386].x) / (_44_m0[_1386].y - _44_m0[_1386].x), 0.0f, 1.0f);
                    _396 = _397;
                    _398 = _399;
                    _400 = _401;
                    _402 = _403;
                    _404 = (_1395 * _1395) * (3.0f - (_1395 * 2.0f));
                    _406 = _407;
                    _408 = _44_m0[_1386].z;
                    _410 = asuint(_44_m0[_1386]).w;
                    _412 = asfloat(_20.Load((_391 * 115u) + 114u).x);
                }
                if (_404 == 0.0f)
                {
                    ladder_phi_8 = false;
                    frontier_phi_8_pred = _406;
                    break;
                }
                float _455;
                if (_268)
                {
                    _455 = _267;
                }
                else
                {
                    _455 = _15.Load(int3(uint2(uint(int(_259 * float(_238))), uint(int(_260 * float(_239)))), 0u)).x;
                }
                float _457 = 1.0f - _406;
                float _458 = _457 * _457;
                float _466 = _49_m0[50u].w + _49_m0[50u].y;
                uint _469 = _402 + 63u;
                float _478 = clamp(((_49_m0[50u].x / (_466 - (_49_m0[50u].y * _455))) - _49_m0[_469].y) / (_49_m0[_469].x - _49_m0[_469].y), 0.0f, 1.0f);
                float _483 = ((_478 * _478) * _404) * (3.0f - (_478 * 2.0f));
                float _490 = 1.0f - (_49_m0[51u].w * _260);
                float _491 = _49_m0[51u].z * _259;
                float _494 = (_491 * 2.0f) + (-1.0f);
                float _495 = (_490 * 2.0f) + (-1.0f);
                float _497 = mad(_134, _495, _494 * _127);
                float _501 = mad(_135, _495, _494 * _128);
                float _505 = mad(_136, _495, _494 * _129);
                float _509 = mad(_137, _495, _494 * _130);
                float _511 = mad(_144, _455, _509) + _151;
                float _512 = (mad(_141, _455, _497) + _148) / _511;
                float _513 = (mad(_142, _455, _501) + _149) / _511;
                float _514 = (mad(_143, _455, _505) + _150) / _511;
                float _518 = rsqrt(dot(float3(_512, _513, _514), float3(_512, _513, _514)));
                float _519 = _518 * _512;
                float _520 = _518 * _513;
                float _521 = _518 * _514;
                float _524 = mad(_93, _400, mad(_87, _398, _396 * _81));
                float _527 = mad(_94, _400, mad(_88, _398, _396 * _82));
                float _530 = mad(_95, _400, mad(_89, _398, _396 * _83));
                float _533 = _527 * _527;
                float _666;
                float _667;
                float _668;
                if (abs(_530) > 0.0f)
                {
                    float _568 = sqrt((_530 * _530) + _533);
                    _666 = 0.0f;
                    _667 = ((-0.0f) - _530) / _568;
                    _668 = _527 / _568;
                }
                else
                {
                    float _574 = sqrt(_533 + (_524 * _524));
                    _666 = _527 / _574;
                    _667 = ((-0.0f) - _524) / _574;
                    _668 = 0.0f;
                }
                float _671 = (_668 * _527) - (_667 * _530);
                float _674 = (_666 * _530) - (_668 * _524);
                float _677 = (_667 * _524) - (_666 * _527);
                float _678 = (-0.0f) - _519;
                float _679 = (-0.0f) - _520;
                float _680 = (-0.0f) - _521;
                float _689 = mad(_680, _530, mad(_679, _527, _524 * _678));
                float _690 = mad(_680, _668, mad(_679, _667, _666 * _678)) * _458;
                float _691 = mad(_680, _677, mad(_679, _674, _671 * _678)) * _458;
                float _695 = rsqrt(dot(float3(_690, _691, _689), float3(_690, _691, _689)));
                float _696 = _695 * _690;
                float _697 = _695 * _691;
                float _698 = _695 * _689;
                float _701 = (_696 * _696) + (_697 * _697);
                bool _702 = _701 > 0.0f;
                float _738;
                float _739;
                if (_702)
                {
                    float _734 = rsqrt(_701);
                    _738 = (-0.0f) - (_697 * _734);
                    _739 = _734 * _696;
                }
                else
                {
                    _738 = 1.0f;
                    _739 = 0.0f;
                }
                float _744 = (_698 + 1.0f) * 0.5f;
                float _745 = 1.0f - _744;
                float _746 = _745 * _698;
                float _753 = sqrt(max(0.0f, 1.0f - (_745 * _745)));
                float _760 = ((_753 * _696) - (_739 * _746)) * _458;
                float _761 = ((_753 * _697) + (_738 * _746)) * _458;
                float _762 = max(0.0f, (((_739 * _696) - (_738 * _697)) * _745) + (_753 * _698));
                float _766 = rsqrt(dot(float3(_760, _761, _762), float3(_760, _761, _762)));
                float _767 = _760 * _766;
                float _768 = _761 * _766;
                float _769 = _766 * _762;
                float _772 = mad(_769, _524, mad(_768, _671, _767 * _666));
                float _775 = mad(_769, _527, mad(_768, _674, _767 * _667));
                float _778 = mad(_769, _530, mad(_768, _677, _767 * _668));
                float _782 = dot(float3(_519, _520, _521), float3(_772, _775, _778)) * 2.0f;
                float _786 = _519 - (_782 * _772);
                float _787 = _520 - (_782 * _775);
                float _788 = _521 - (_782 * _778);
                float _837;
                float _838;
                if (_702)
                {
                    float _833 = rsqrt(_701);
                    _837 = (-0.0f) - (_697 * _833);
                    _838 = _833 * _696;
                }
                else
                {
                    _837 = 1.0f;
                    _838 = 0.0f;
                }
                float _905;
                float _906;
                float _907;
                float _932;
                float _933;
                float _934;
                bool _938;
                float _956;
                float _957;
                float _958;
                float _977;
                float _982;
                float _983;
                float _984;
                float _985;
                float _1017;
                float _1019;
                float _1020;
                float _1021;
                float _1023;
                float _1024;
                float _1078;
                bool _1084;
                float4 _1099;
                bool _1103;
                for (;;)
                {
                    float _842 = sqrt(_49_m0[1u].x);
                    float _843 = _49_m0[1u].y * 1.57079637050628662109375f;
                    float _846 = cos(_843) * _842;
                    float _849 = 1.0f - (_846 * _846);
                    float _854 = (sqrt(_849) * _745) + ((_842 * _744) * sin(_843));
                    float _857 = _854 * _698;
                    float _864 = sqrt(max(0.0f, _849 - (_854 * _854)));
                    float _873 = (((_864 * _696) + (_846 * _837)) - (_857 * _838)) * _458;
                    float _874 = (((_864 * _697) + (_846 * _838)) + (_857 * _837)) * _458;
                    float _875 = max(0.0f, (_854 * ((_838 * _696) - (_837 * _697))) + (_864 * _698));
                    float _879 = rsqrt(dot(float3(_873, _874, _875), float3(_873, _874, _875)));
                    float _880 = _873 * _879;
                    float _881 = _874 * _879;
                    float _882 = _879 * _875;
                    float _885 = mad(_882, _524, mad(_881, _671, _880 * _666));
                    float _888 = mad(_882, _527, mad(_881, _674, _880 * _667));
                    float _891 = mad(_882, _530, mad(_881, _677, _880 * _668));
                    float _895 = dot(float3(_519, _520, _521), float3(_885, _888, _891)) * 2.0f;
                    float _899 = _519 - (_895 * _885);
                    float _900 = _520 - (_895 * _888);
                    float _901 = _521 - (_895 * _891);
                    float _902 = dot(float3(_899, _900, _901), float3(_786, _787, _788));
                    _905 = _899 / _902;
                    _906 = _900 / _902;
                    _907 = _901 / _902;
                    float _915 = (_49_m0[1u].z * (_524 - _772)) + _772;
                    float _916 = (_49_m0[1u].z * (_527 - _775)) + _775;
                    float _917 = (_49_m0[1u].z * (_530 - _778)) + _778;
                    float _921 = rsqrt(dot(float3(_915, _916, _917), float3(_915, _916, _917)));
                    float _922 = _921 * _915;
                    float _923 = _921 * _916;
                    float _924 = _921 * _917;
                    float _928 = dot(float3(_519, _520, _521), float3(_922, _923, _924)) * 2.0f;
                    _932 = _519 - (_928 * _922);
                    _933 = _520 - (_928 * _923);
                    _934 = _521 - (_928 * _924);
                    _938 = dot(float3(_932, _933, _934), float3(_519, _520, _521)) < 0.0f;
                    float _945 = sqrt(((_513 * _513) + (_512 * _512)) + (_514 * _514)) * 0.001000000047497451305389404296875f;
                    _956 = ((_945 * _524) + _512) + (_932 * _408);
                    _957 = ((_945 * _527) + _513) + (_933 * _408);
                    _958 = ((_945 * _530) + _514) + (_934 * _408);
                    float _974 = mad(_116, _958, mad(_109, _957, _956 * _102)) + _123;
                    _977 = (mad(_115, _958, mad(_108, _957, _956 * _101)) + _122) / _974;
                    float _980 = (((mad(_113, _958, mad(_106, _957, _956 * _99)) + _120) / _974) * 0.5f) + 0.5f;
                    float _981 = 0.5f - (((mad(_114, _958, mad(_107, _957, _956 * _100)) + _121) / _974) * 0.5f);
                    _982 = _49_m0[51u].x;
                    _983 = _49_m0[51u].y;
                    _984 = _980 * _982;
                    _985 = _981 * _983;
                    float _990 = _956 + (_932 * 0.100000001490116119384765625f);
                    float _991 = _957 + (_933 * 0.100000001490116119384765625f);
                    float _992 = _958 + (_934 * 0.100000001490116119384765625f);
                    float _1008 = mad(_116, _992, mad(_109, _991, _990 * _102)) + _123;
                    _1017 = _982 * (((((mad(_113, _992, mad(_106, _991, _990 * _99)) + _120) / _1008) * 0.5f) + 0.5f) - _980);
                    _1019 = _983 * ((0.5f - (((mad(_114, _992, mad(_107, _991, _990 * _100)) + _121) / _1008) * 0.5f)) - _981);
                    _1020 = ((mad(_115, _992, mad(_108, _991, _990 * _101)) + _122) / _1008) - _977;
                    _1021 = _1017 * 10.0f;
                    _1023 = _1019 * 10.0f;
                    _1024 = _1020 * 10.0f;
                    float _1031 = _49_m0[2u].w / dot(float3(_932, _933, _934), float3(_524, _527, _530));
                    float _1035 = (_1031 * _932) + _956;
                    float _1036 = (_1031 * _933) + _957;
                    float _1037 = (_1031 * _934) + _958;
                    float _1053 = mad(_116, _1037, mad(_109, _1036, _1035 * _102)) + _123;
                    float _1062 = _982 * (((((mad(_113, _1037, mad(_106, _1036, _1035 * _99)) + _120) / _1053) * 0.5f) + 0.5f) - _980);
                    float _1064 = _983 * ((0.5f - (((mad(_114, _1037, mad(_107, _1036, _1035 * _100)) + _121) / _1053) * 0.5f)) - _981);
                    float _1065 = ((mad(_115, _1037, mad(_108, _1036, _1035 * _101)) + _122) / _1053) - _977;
                    _1078 = sqrt(((_1062 * _1062) + (_1065 * _1065)) + (_1064 * _1064)) / sqrt(((_1021 * _1021) + (_1024 * _1024)) + (_1023 * _1023));
                    _1084 = asuint(_49_m0[62u]).y == 0u;
                    _1099 = _49_m0[9u];
                    _1103 = abs((((mad(_188, _455, mad(_181, _495, _494 * _174)) + _195) / (mad(_190, _455, mad(_183, _495, _494 * _176)) + _197)) + _44_m0[8u].y) - _1099.w) < 0.0500000007450580596923828125f;
                    if (_1103)
                    {
                        if (dot(float3(_1099.xyz), float3(_396, _398, _400)) > 0.999000012874603271484375f)
                        {
                            uint _1141 = asuint(_49_m0[7u]).x;
                            float4 _1153 = _23[asuint(_49_m0[8u]).x].SampleLevel(_59, float2(_491, _490), 0.0f);
                            float _1156 = _1153.x;
                            uint _1157_dummy_parameter;
                            uint2 _1157 = spvTextureSize(_23[_1141], 0u, _1157_dummy_parameter);
                            float _1159 = float(int(_1157.x));
                            float _1161 = float(int(_1157.y));
                            float _1169 = mad(_144, _1156, _509) + _151;
                            float _1173 = ((mad(_141, _1156, _497) + _148) / _1169) - _956;
                            float _1174 = ((mad(_142, _1156, _501) + _149) / _1169) - _957;
                            float _1175 = ((mad(_143, _1156, _505) + _150) / _1169) - _958;
                            float _1181 = sqrt(((_1174 * _1174) + (_1173 * _1173)) + (_1175 * _1175));
                            float _1182 = _1181 * _786;
                            float _1183 = _1181 * _787;
                            float _1184 = _1181 * _788;
                            float _1185 = _1181 * _905;
                            float _1186 = _1181 * _906;
                            float _1187 = _1181 * _907;
                            float _1191 = dot(float3(_1182, _1183, _1184), float3(_772, _775, _778)) * 2.0f;
                            float _1201 = dot(float3(_1185, _1186, _1187), float3(_772, _775, _778)) * 2.0f;
                            float _1228 = (_1182 - (_1191 * _772)) + _956;
                            float _1229 = (_1183 - (_1191 * _775)) + _957;
                            float _1230 = (_1184 - (_1191 * _778)) + _958;
                            float _1242 = mad(_49_m0[24u].w, _1230, mad(_49_m0[23u].w, _1229, _1228 * _49_m0[22u].w)) + _49_m0[25u].w;
                            float _1245 = (_1185 - (_1201 * _772)) + _956;
                            float _1246 = (_1186 - (_1201 * _775)) + _957;
                            float _1247 = (_1187 - (_1201 * _778)) + _958;
                            float _1259 = mad(_49_m0[24u].w, _1247, mad(_49_m0[23u].w, _1246, _1245 * _49_m0[22u].w)) + _49_m0[25u].w;
                            float _1265 = (_982 * ((((mad(_49_m0[24u].x, _1230, mad(_49_m0[23u].x, _1229, _1228 * _49_m0[22u].x)) + _49_m0[25u].x) / _1242) - ((mad(_49_m0[24u].x, _1247, mad(_49_m0[23u].x, _1246, _1245 * _49_m0[22u].x)) + _49_m0[25u].x) / _1259)) * 0.5f)) * _1159;
                            float _1269 = (_983 * ((((mad(_49_m0[24u].y, _1247, mad(_49_m0[23u].y, _1246, _1245 * _49_m0[22u].y)) + _49_m0[25u].y) / _1259) - ((mad(_49_m0[24u].y, _1230, mad(_49_m0[23u].y, _1229, _1228 * _49_m0[22u].y)) + _49_m0[25u].y) / _1242)) * 0.5f)) * _1161;
                            float _1286 = float(asuint(_49_m0[55u]).x + 4294967295u);
                            float _1288 = clamp(log2((sqrt((_1269 * _1269) + (_1265 * _1265)) * 2.0f) / _49_m0[0u].w) / _1286, 0.0f, 1.0f);
                            float _1289 = _1286 * _1288;
                            float _1562;
                            float _1564;
                            float _1566;
                            if (_1084)
                            {
                                float4 _1403 = _23[_1141].SampleLevel(_57, float2(_491, _490), _1289);
                                _1562 = _1403.x;
                                _1564 = _1403.y;
                                _1566 = _1403.z;
                            }
                            else
                            {
                                uint _1409 = uint(int(floor(_1289)));
                                uint _1411 = uint(int(ceil(_1289)));
                                float _1412 = float(int(_1409));
                                float _1413 = _1289 - _1412;
                                uint _1414 = uint(int(_1159));
                                uint _1415 = uint(int(_1161));
                                uint _1416 = _1409 & 31u;
                                float _1421 = float(int(uint(max(int(1u), int(uint(int(_1414) >> int(_1416)))))));
                                float _1422 = float(int(uint(max(int(1u), int(uint(int(_1415) >> int(_1416)))))));
                                float _1423 = 1.0f / _1421;
                                float _1424 = 1.0f / _1422;
                                float _1427 = (_1421 * _491) + (-0.5f);
                                float _1429 = (_1422 * _490) + (-0.5f);
                                float _1430 = floor(_1427);
                                float _1431 = floor(_1429);
                                float _1432 = _1427 - _1430;
                                float _1433 = _1429 - _1431;
                                float _1434 = _1432 * _1432;
                                float _1435 = _1434 * _1432;
                                float _1444 = mad(_1434, -1.0f, _1435 * 0.5f) + 0.666666686534881591796875f;
                                float _1449 = _1435 * 0.16666667163372039794921875f;
                                float _1450 = _1433 * _1433;
                                float _1451 = _1450 * _1433;
                                float _1458 = mad(_1450, -1.0f, _1451 * 0.5f) + 0.666666686534881591796875f;
                                float _1462 = _1451 * 0.16666667163372039794921875f;
                                float _1463 = (mad(_1432, -0.5f, mad(_1434, 0.5f, _1435 * (-0.16666667163372039794921875f))) + 0.16666667163372039794921875f) + _1444;
                                float _1464 = (mad(_1433, -0.5f, mad(_1450, 0.5f, _1451 * (-0.16666667163372039794921875f))) + 0.16666667163372039794921875f) + _1458;
                                float _1477 = ((_1430 + (-0.5f)) + (_1444 / _1463)) * _1423;
                                float _1478 = ((_1431 + (-0.5f)) + (_1458 / _1464)) * _1424;
                                float _1482 = ((_1430 + 1.5f) + (_1449 / ((_1449 + 0.16666667163372039794921875f) + mad(_1432, 0.5f, mad(_1434, 0.5f, _1435 * (-0.5f)))))) * _1423;
                                float _1485 = ((_1431 + 1.5f) + (_1462 / ((_1462 + 0.16666667163372039794921875f) + mad(_1433, 0.5f, mad(_1450, 0.5f, _1451 * (-0.5f)))))) * _1424;
                                float4 _1487 = _23[_1141].SampleLevel(_57, float2(_1477, _1478), _1412);
                                float4 _1492 = _23[_1141].SampleLevel(_57, float2(_1482, _1478), _1412);
                                float4 _1497 = _23[_1141].SampleLevel(_57, float2(_1477, _1485), _1412);
                                float _1499 = _1497.x;
                                float _1500 = _1497.y;
                                float _1501 = _1497.z;
                                float4 _1502 = _23[_1141].SampleLevel(_57, float2(_1482, _1485), _1412);
                                float _1504 = _1502.x;
                                float _1505 = _1502.y;
                                float _1506 = _1502.z;
                                float _1522 = ((_1492.x - _1504) * _1464) + _1504;
                                float _1523 = ((_1492.y - _1505) * _1464) + _1505;
                                float _1524 = ((_1492.z - _1506) * _1464) + _1506;
                                float _1531 = (((((_1487.x - _1499) * _1464) + _1499) - _1522) * _1463) + _1522;
                                float _1532 = (((((_1487.y - _1500) * _1464) + _1500) - _1523) * _1463) + _1523;
                                float _1533 = (((((_1487.z - _1501) * _1464) + _1501) - _1524) * _1463) + _1524;
                                float frontier_phi_60_51_ladder;
                                float frontier_phi_60_51_ladder_1;
                                float frontier_phi_60_51_ladder_2;
                                if (_1413 > 0.0f)
                                {
                                    uint _1604 = _1411 & 31u;
                                    float _1609 = float(int(uint(max(int(1u), int(uint(int(_1414) >> int(_1604)))))));
                                    float _1610 = float(int(uint(max(int(1u), int(uint(int(_1415) >> int(_1604)))))));
                                    float _1611 = 1.0f / _1609;
                                    float _1612 = 1.0f / _1610;
                                    float _1615 = (_1609 * _491) + (-0.5f);
                                    float _1616 = (_1610 * _490) + (-0.5f);
                                    float _1617 = floor(_1615);
                                    float _1618 = floor(_1616);
                                    float _1619 = _1615 - _1617;
                                    float _1620 = _1616 - _1618;
                                    float _1621 = _1619 * _1619;
                                    float _1622 = _1621 * _1619;
                                    float _1629 = mad(_1621, -1.0f, _1622 * 0.5f) + 0.666666686534881591796875f;
                                    float _1633 = _1622 * 0.16666667163372039794921875f;
                                    float _1634 = _1620 * _1620;
                                    float _1635 = _1634 * _1620;
                                    float _1642 = mad(_1634, -1.0f, _1635 * 0.5f) + 0.666666686534881591796875f;
                                    float _1646 = _1635 * 0.16666667163372039794921875f;
                                    float _1647 = (mad(_1619, -0.5f, mad(_1621, 0.5f, _1622 * (-0.16666667163372039794921875f))) + 0.16666667163372039794921875f) + _1629;
                                    float _1648 = (mad(_1620, -0.5f, mad(_1634, 0.5f, _1635 * (-0.16666667163372039794921875f))) + 0.16666667163372039794921875f) + _1642;
                                    float _1661 = ((_1617 + (-0.5f)) + (_1629 / _1647)) * _1611;
                                    float _1662 = ((_1618 + (-0.5f)) + (_1642 / _1648)) * _1612;
                                    float _1665 = ((_1617 + 1.5f) + (_1633 / ((_1633 + 0.16666667163372039794921875f) + mad(_1619, 0.5f, mad(_1621, 0.5f, _1622 * (-0.5f)))))) * _1611;
                                    float _1668 = ((_1618 + 1.5f) + (_1646 / ((_1646 + 0.16666667163372039794921875f) + mad(_1620, 0.5f, mad(_1634, 0.5f, _1635 * (-0.5f)))))) * _1612;
                                    float _1669 = float(int(_1411));
                                    float4 _1671 = _23[_1141].SampleLevel(_57, float2(_1661, _1662), _1669);
                                    float4 _1676 = _23[_1141].SampleLevel(_57, float2(_1665, _1662), _1669);
                                    float4 _1681 = _23[_1141].SampleLevel(_57, float2(_1661, _1668), _1669);
                                    float _1683 = _1681.x;
                                    float _1684 = _1681.y;
                                    float _1685 = _1681.z;
                                    float4 _1686 = _23[_1141].SampleLevel(_57, float2(_1665, _1668), _1669);
                                    float _1688 = _1686.x;
                                    float _1689 = _1686.y;
                                    float _1690 = _1686.z;
                                    float _1706 = ((_1676.x - _1688) * _1648) + _1688;
                                    float _1707 = ((_1676.y - _1689) * _1648) + _1689;
                                    float _1708 = ((_1676.z - _1690) * _1648) + _1690;
                                    frontier_phi_60_51_ladder = (((_1708 - _1533) + (((((_1671.z - _1685) * _1648) + _1685) - _1708) * _1647)) * _1413) + _1533;
                                    frontier_phi_60_51_ladder_1 = (((_1707 - _1532) + (((((_1671.y - _1684) * _1648) + _1684) - _1707) * _1647)) * _1413) + _1532;
                                    frontier_phi_60_51_ladder_2 = (((_1706 - _1531) + (((((_1671.x - _1683) * _1648) + _1683) - _1706) * _1647)) * _1413) + _1531;
                                }
                                else
                                {
                                    frontier_phi_60_51_ladder = _1533;
                                    frontier_phi_60_51_ladder_1 = _1532;
                                    frontier_phi_60_51_ladder_2 = _1531;
                                }
                                _1562 = frontier_phi_60_51_ladder_2;
                                _1564 = frontier_phi_60_51_ladder_1;
                                _1566 = frontier_phi_60_51_ladder;
                            }
                            float _1568 = (-0.0f) - _1288;
                            float _1580 = max(_49_m0[54u].y * _23[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                            float _1582 = _1580 * _1562;
                            float _1583 = _1580 * _1564;
                            float _1584 = _1580 * _1566;
                            float _1590 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_1582, max(_1583, _1584)) + 1.0f);
                            float _1594 = min(_1590 * _1582, 0.996078431606292724609375f);
                            float _1596 = min(_1590 * _1583, 0.996078431606292724609375f);
                            float _1597 = min(_1590 * _1584, 0.996078431606292724609375f);
                            _34[uint2(_226, _229)] = float4(_1594, _1596, _1597, 1.0f);
                            _38[uint2(_226, _229)] = float4(_1568, _406, 1.0f, _1568);
                            if (_233)
                            {
                                uint _1818 = _226 + 1u;
                                _34[uint2(_1818, _229)] = float4(_1594, _1596, _1597, 1.0f);
                                _38[uint2(_1818, _229)] = float4(_1568, _406, 1.0f, _1568);
                            }
                            if (_236)
                            {
                                uint _1971 = _229 + 1u;
                                _34[uint2(_226, _1971)] = float4(_1594, _1596, _1597, 1.0f);
                                _38[uint2(_226, _1971)] = float4(_1568, _406, 1.0f, _1568);
                            }
                            if (!_237)
                            {
                                break;
                            }
                            uint _2196 = _226 + 1u;
                            uint _2197 = _229 + 1u;
                            _34[uint2(_2196, _2197)] = float4(_1594, _1596, _1597, 1.0f);
                            _38[uint2(_2196, _2197)] = float4(_1568, _406, 1.0f, _1568);
                            break;
                        }
                    }
                    if (_410 == 0u)
                    {
                        float _1291 = float(_238);
                        float _1292 = float(_239);
                        float _1299 = (_1021 != 0.0f) ? (0.100000001490116119384765625f / _1017) : 3.4028234663852885981170418348452e+38f;
                        float _1301 = (_1023 != 0.0f) ? (0.100000001490116119384765625f / _1019) : 3.4028234663852885981170418348452e+38f;
                        float _1302 = (_1024 != 0.0f) ? (0.100000001490116119384765625f / _1020) : 3.4028234663852885981170418348452e+38f;
                        float _1303 = 1.0f / _1291;
                        float _1304 = 1.0f / _1292;
                        float _1305 = 0.004999999888241291046142578125f / _1291;
                        float _1307 = 0.004999999888241291046142578125f / _1292;
                        float _1316 = float(_1021 >= 0.0f);
                        float _1317 = float(_1023 >= 0.0f);
                        float _1326 = ((_1021 < 0.0f) ? ((-0.0f) - _1305) : _1305) - _984;
                        float _1329 = ((_1023 < 0.0f) ? ((-0.0f) - _1307) : _1307) - _985;
                        float _1332 = min((((floor(_984 * _1291) + _1316) * _1303) + _1326) * _1299, (((floor(_985 * _1292) + _1317) * _1304) + _1329) * _1301);
                        float _1336 = (_1332 * _1021) + _984;
                        float _1337 = (_1332 * _1023) + _985;
                        float _1338 = (_1332 * _1024) + _977;
                        float _1341 = _49_m0[50u].x / (_466 - (_1338 * _49_m0[50u].y));
                        float _1351 = 1.0f / max(0.00999999977648258209228515625f, max(abs(_1017 * 38400.0f), abs(_1019 * 21600.0f)));
                        float _1359 = max(_49_m0[3u].z, _49_m0[5u].z * _1351);
                        float _1535;
                        if (asuint(_49_m0[5u]).y == 0u)
                        {
                            _1535 = _1359;
                        }
                        else
                        {
                            _1535 = min(_1359, _49_m0[5u].w * _1351);
                        }
                        uint _1724;
                        float _1726;
                        float _1728;
                        float _1730;
                        float _1732;
                        float _1734;
                        float _1736;
                        float _1738;
                        uint _1740;
                        uint _1742;
                        if (_209 == 0u)
                        {
                            _1724 = 0u;
                            _1726 = _1338;
                            _1728 = _1337;
                            _1730 = _1336;
                            _1732 = _977;
                            _1734 = _985;
                            _1736 = _984;
                            _1738 = 1.0f;
                            _1740 = 0u;
                            _1742 = 1u;
                        }
                        else
                        {
                            uint _1725;
                            float _1727;
                            float _1729;
                            float _1731;
                            uint _1906;
                            uint _1913;
                            float _1733;
                            float _1735;
                            float _1737;
                            float _1739;
                            uint _1741;
                            uint _1743;
                            float _1889;
                            uint _1891;
                            float _1896;
                            float _1898;
                            float _1900;
                            float _1902;
                            float _1904;
                            uint _1887 = 0u;
                            float _1888 = _1535;
                            uint _1890 = 0u;
                            float _1892 = _1338;
                            float _1893 = _1337;
                            float _1894 = _1336;
                            float _1895 = _1332;
                            float _1897 = _1304;
                            float _1899 = _1303;
                            float _1901 = _1292;
                            float _1903 = _1291;
                            uint _1905 = 0u;
                            float _1907 = _977;
                            float _1908 = _985;
                            float _1909 = _984;
                            float _1910 = 1.0f;
                            uint _1911 = 0u;
                            uint _1912 = 0u;
                            uint _1914 = 1u;
                            float _1915;
                            float _1916;
                            uint _1917;
                            uint _1918;
                            bool _1919;
                            for (;;)
                            {
                                _1915 = _1903 * _1894;
                                _1916 = _1901 * _1893;
                                _1917 = uint(int(_1915));
                                _1918 = uint(int(_1916));
                                _1919 = _1905 == 0u;
                                float _2241;
                                if (_1919)
                                {
                                    _2241 = _15.Load(int3(uint2(_1917, _1918), 0u)).x;
                                }
                                else
                                {
                                    _2241 = _18.Load(int3(uint2(_1917, _1918), _1905 + 4294967295u)).x;
                                }
                                float _2247 = ((_1915 >= floor(_1903)) || (_1916 >= floor(_1901))) ? 1.0f : _2241;
                                float _2261 = (_1024 < 0.0f) ? ((_2247 - _977) * _1302) : 3.4028234663852885981170418348452e+38f;
                                float _2263 = min(min((((floor(_1915) + _1316) * _1899) + _1326) * _1299, (((floor(_1916) + _1317) * _1897) + _1329) * _1301), _2261);
                                bool _2264 = _2247 < _1892;
                                bool _2268 = _2264 && (asuint(_2263) != asuint(_2261));
                                float _2269 = _2264 ? _2263 : _1895;
                                float _2273 = (_2269 * _1021) + _984;
                                float _2274 = (_2269 * _1023) + _985;
                                float _2275 = (_2269 * _1024) + _977;
                                uint _2277 = (_2268 ? 1u : 4294967295u) + _1905;
                                float _2278 = _2268 ? 0.5f : 2.0f;
                                float _2279 = _2278 * _1903;
                                float _2280 = _2278 * _1901;
                                float _2281 = _2268 ? 2.0f : 0.5f;
                                float _2282 = _2281 * _1899;
                                float _2283 = _2281 * _1897;
                                _1725 = _1887 + 1u;
                                uint _2447;
                                uint _2450;
                                if (int(_2277) < int(0u))
                                {
                                    float frontier_phi_107_95_ladder;
                                    uint frontier_phi_107_95_ladder_1;
                                    uint frontier_phi_107_95_ladder_2;
                                    uint frontier_phi_107_95_ladder_3;
                                    float frontier_phi_107_95_ladder_4;
                                    float frontier_phi_107_95_ladder_5;
                                    float frontier_phi_107_95_ladder_6;
                                    float frontier_phi_107_95_ladder_7;
                                    uint frontier_phi_107_95_ladder_8;
                                    float frontier_phi_107_95_ladder_9;
                                    float frontier_phi_107_95_ladder_10;
                                    float frontier_phi_107_95_ladder_11;
                                    float frontier_phi_107_95_ladder_12;
                                    uint frontier_phi_107_95_ladder_13;
                                    float frontier_phi_107_95_ladder_14;
                                    float _2319;
                                    float4 _2324;
                                    float _2327;
                                    float _2330;
                                    bool _2331;
                                    for (;;)
                                    {
                                        float _2317 = _49_m0[50u].w + _49_m0[50u].y;
                                        _2319 = _49_m0[50u].x / (_2317 - (_49_m0[50u].y * _2247));
                                        float _2322 = _49_m0[50u].x / (_2317 - (_49_m0[50u].y * _2275));
                                        _2324 = _49_m0[3u];
                                        _2327 = abs(_1341 - _2322);
                                        _2330 = _2322 - _2319;
                                        _2331 = _2330 > max(_2324.x, _2324.x * _2327);
                                        if (_2331)
                                        {
                                            uint _2452;
                                            if (_1890 == 0u)
                                            {
                                                uint frontier_phi_120_119_ladder;
                                                if ((_402 == 2u) || (_402 == 4u))
                                                {
                                                    if ((_2269 < _1078) && (abs(_2330) < _49_m0[2u].z))
                                                    {
                                                        frontier_phi_107_95_ladder = _2279;
                                                        frontier_phi_107_95_ladder_1 = _1914;
                                                        frontier_phi_107_95_ladder_2 = 1u;
                                                        frontier_phi_107_95_ladder_3 = 1u;
                                                        frontier_phi_107_95_ladder_4 = 0.0f;
                                                        frontier_phi_107_95_ladder_5 = _1909;
                                                        frontier_phi_107_95_ladder_6 = _1908;
                                                        frontier_phi_107_95_ladder_7 = _1907;
                                                        frontier_phi_107_95_ladder_8 = _2277;
                                                        frontier_phi_107_95_ladder_9 = _2280;
                                                        frontier_phi_107_95_ladder_10 = _2282;
                                                        frontier_phi_107_95_ladder_11 = _2283;
                                                        frontier_phi_107_95_ladder_12 = _2269;
                                                        frontier_phi_107_95_ladder_13 = 1u;
                                                        frontier_phi_107_95_ladder_14 = _1888;
                                                        break;
                                                    }
                                                    frontier_phi_120_119_ladder = 1u;
                                                }
                                                else
                                                {
                                                    frontier_phi_120_119_ladder = 1u;
                                                }
                                                _2452 = frontier_phi_120_119_ladder;
                                            }
                                            else
                                            {
                                                _2452 = _1890;
                                            }
                                            if (!(_1911 == 0u))
                                            {
                                                frontier_phi_107_95_ladder = _2279;
                                                frontier_phi_107_95_ladder_1 = _1914;
                                                frontier_phi_107_95_ladder_2 = _1912;
                                                frontier_phi_107_95_ladder_3 = _1911;
                                                frontier_phi_107_95_ladder_4 = _1910;
                                                frontier_phi_107_95_ladder_5 = _1909;
                                                frontier_phi_107_95_ladder_6 = _1908;
                                                frontier_phi_107_95_ladder_7 = _1907;
                                                frontier_phi_107_95_ladder_8 = _2277;
                                                frontier_phi_107_95_ladder_9 = _2280;
                                                frontier_phi_107_95_ladder_10 = _2282;
                                                frontier_phi_107_95_ladder_11 = _2283;
                                                frontier_phi_107_95_ladder_12 = _2269;
                                                frontier_phi_107_95_ladder_13 = _2452;
                                                frontier_phi_107_95_ladder_14 = _1888;
                                                break;
                                            }
                                            float _2451 = _2269 + _1888;
                                            uint _2448 = ((_402 == 1u) || (asuint(_49_m0[62u]).z == 0u)) ? 1u : _1912;
                                            if (asuint(_49_m0[5u]).y == 0u)
                                            {
                                                frontier_phi_107_95_ladder = _1291;
                                                frontier_phi_107_95_ladder_1 = _1914;
                                                frontier_phi_107_95_ladder_2 = _2448;
                                                frontier_phi_107_95_ladder_3 = 0u;
                                                frontier_phi_107_95_ladder_4 = _1910;
                                                frontier_phi_107_95_ladder_5 = _1909;
                                                frontier_phi_107_95_ladder_6 = _1908;
                                                frontier_phi_107_95_ladder_7 = _1907;
                                                frontier_phi_107_95_ladder_8 = 0u;
                                                frontier_phi_107_95_ladder_9 = _1292;
                                                frontier_phi_107_95_ladder_10 = _1303;
                                                frontier_phi_107_95_ladder_11 = _1304;
                                                frontier_phi_107_95_ladder_12 = _2451;
                                                frontier_phi_107_95_ladder_13 = _2452;
                                                frontier_phi_107_95_ladder_14 = _1888;
                                                break;
                                            }
                                            frontier_phi_107_95_ladder = _1291;
                                            frontier_phi_107_95_ladder_1 = _1914;
                                            frontier_phi_107_95_ladder_2 = _2448;
                                            frontier_phi_107_95_ladder_3 = 0u;
                                            frontier_phi_107_95_ladder_4 = _1910;
                                            frontier_phi_107_95_ladder_5 = _1909;
                                            frontier_phi_107_95_ladder_6 = _1908;
                                            frontier_phi_107_95_ladder_7 = _1907;
                                            frontier_phi_107_95_ladder_8 = 0u;
                                            frontier_phi_107_95_ladder_9 = _1292;
                                            frontier_phi_107_95_ladder_10 = _1303;
                                            frontier_phi_107_95_ladder_11 = _1304;
                                            frontier_phi_107_95_ladder_12 = _2451;
                                            frontier_phi_107_95_ladder_13 = _2452;
                                            frontier_phi_107_95_ladder_14 = min(_1359, _49_m0[6u].x * _1888);
                                            break;
                                        }
                                        else
                                        {
                                            float _2436 = max(_2324.y, _2324.y * _2327);
                                            float _2439 = _2436 * _2324.w;
                                            float _2443 = clamp((abs(_2330) - _2439) / (_2436 - _2439), 0.0f, 1.0f);
                                            uint _2445 = uint(_2319 < _1341);
                                            float frontier_phi_107_95_ladder_106_ladder;
                                            uint frontier_phi_107_95_ladder_106_ladder_1;
                                            uint frontier_phi_107_95_ladder_106_ladder_2;
                                            uint frontier_phi_107_95_ladder_106_ladder_3;
                                            float frontier_phi_107_95_ladder_106_ladder_4;
                                            float frontier_phi_107_95_ladder_106_ladder_5;
                                            float frontier_phi_107_95_ladder_106_ladder_6;
                                            float frontier_phi_107_95_ladder_106_ladder_7;
                                            uint frontier_phi_107_95_ladder_106_ladder_8;
                                            float frontier_phi_107_95_ladder_106_ladder_9;
                                            float frontier_phi_107_95_ladder_106_ladder_10;
                                            float frontier_phi_107_95_ladder_106_ladder_11;
                                            float frontier_phi_107_95_ladder_106_ladder_12;
                                            uint frontier_phi_107_95_ladder_106_ladder_13;
                                            float frontier_phi_107_95_ladder_106_ladder_14;
                                            if (_1912 == 0u)
                                            {
                                                frontier_phi_107_95_ladder_106_ladder = _2279;
                                                frontier_phi_107_95_ladder_106_ladder_1 = _2445;
                                                frontier_phi_107_95_ladder_106_ladder_2 = uint(_2443 > 0.0f);
                                                frontier_phi_107_95_ladder_106_ladder_3 = _1911;
                                                frontier_phi_107_95_ladder_106_ladder_4 = _2443;
                                                frontier_phi_107_95_ladder_106_ladder_5 = _1909;
                                                frontier_phi_107_95_ladder_106_ladder_6 = _1908;
                                                frontier_phi_107_95_ladder_106_ladder_7 = _1907;
                                                frontier_phi_107_95_ladder_106_ladder_8 = _2277;
                                                frontier_phi_107_95_ladder_106_ladder_9 = _2280;
                                                frontier_phi_107_95_ladder_106_ladder_10 = _2282;
                                                frontier_phi_107_95_ladder_106_ladder_11 = _2283;
                                                frontier_phi_107_95_ladder_106_ladder_12 = _2269;
                                                frontier_phi_107_95_ladder_106_ladder_13 = _1890;
                                                frontier_phi_107_95_ladder_106_ladder_14 = _1888;
                                            }
                                            else
                                            {
                                                frontier_phi_107_95_ladder_106_ladder = _2279;
                                                frontier_phi_107_95_ladder_106_ladder_1 = _2445;
                                                frontier_phi_107_95_ladder_106_ladder_2 = _1912;
                                                frontier_phi_107_95_ladder_106_ladder_3 = _1911;
                                                frontier_phi_107_95_ladder_106_ladder_4 = _2443;
                                                frontier_phi_107_95_ladder_106_ladder_5 = _1909;
                                                frontier_phi_107_95_ladder_106_ladder_6 = _1908;
                                                frontier_phi_107_95_ladder_106_ladder_7 = _1907;
                                                frontier_phi_107_95_ladder_106_ladder_8 = _2277;
                                                frontier_phi_107_95_ladder_106_ladder_9 = _2280;
                                                frontier_phi_107_95_ladder_106_ladder_10 = _2282;
                                                frontier_phi_107_95_ladder_106_ladder_11 = _2283;
                                                frontier_phi_107_95_ladder_106_ladder_12 = _2269;
                                                frontier_phi_107_95_ladder_106_ladder_13 = _1890;
                                                frontier_phi_107_95_ladder_106_ladder_14 = _1888;
                                            }
                                            frontier_phi_107_95_ladder = frontier_phi_107_95_ladder_106_ladder;
                                            frontier_phi_107_95_ladder_1 = frontier_phi_107_95_ladder_106_ladder_1;
                                            frontier_phi_107_95_ladder_2 = frontier_phi_107_95_ladder_106_ladder_2;
                                            frontier_phi_107_95_ladder_3 = frontier_phi_107_95_ladder_106_ladder_3;
                                            frontier_phi_107_95_ladder_4 = frontier_phi_107_95_ladder_106_ladder_4;
                                            frontier_phi_107_95_ladder_5 = frontier_phi_107_95_ladder_106_ladder_5;
                                            frontier_phi_107_95_ladder_6 = frontier_phi_107_95_ladder_106_ladder_6;
                                            frontier_phi_107_95_ladder_7 = frontier_phi_107_95_ladder_106_ladder_7;
                                            frontier_phi_107_95_ladder_8 = frontier_phi_107_95_ladder_106_ladder_8;
                                            frontier_phi_107_95_ladder_9 = frontier_phi_107_95_ladder_106_ladder_9;
                                            frontier_phi_107_95_ladder_10 = frontier_phi_107_95_ladder_106_ladder_10;
                                            frontier_phi_107_95_ladder_11 = frontier_phi_107_95_ladder_106_ladder_11;
                                            frontier_phi_107_95_ladder_12 = frontier_phi_107_95_ladder_106_ladder_12;
                                            frontier_phi_107_95_ladder_13 = frontier_phi_107_95_ladder_106_ladder_13;
                                            frontier_phi_107_95_ladder_14 = frontier_phi_107_95_ladder_106_ladder_14;
                                            break;
                                        }
                                    }
                                    _1743 = frontier_phi_107_95_ladder_1;
                                    _2447 = frontier_phi_107_95_ladder_2;
                                    _1741 = frontier_phi_107_95_ladder_3;
                                    _1739 = frontier_phi_107_95_ladder_4;
                                    _1737 = frontier_phi_107_95_ladder_5;
                                    _1735 = frontier_phi_107_95_ladder_6;
                                    _1733 = frontier_phi_107_95_ladder_7;
                                    _2450 = frontier_phi_107_95_ladder_8;
                                    _1904 = frontier_phi_107_95_ladder;
                                    _1902 = frontier_phi_107_95_ladder_9;
                                    _1900 = frontier_phi_107_95_ladder_10;
                                    _1898 = frontier_phi_107_95_ladder_11;
                                    _1896 = frontier_phi_107_95_ladder_12;
                                    _1891 = frontier_phi_107_95_ladder_13;
                                    _1889 = frontier_phi_107_95_ladder_14;
                                }
                                else
                                {
                                    _1743 = _1914;
                                    _2447 = _1912;
                                    _1741 = _1911;
                                    _1739 = _1910;
                                    _1737 = _2273;
                                    _1735 = _2274;
                                    _1733 = _2275;
                                    _2450 = _2277;
                                    _1904 = _2279;
                                    _1902 = _2280;
                                    _1900 = _2282;
                                    _1898 = _2283;
                                    _1896 = _2269;
                                    _1891 = _1890;
                                    _1889 = (asuint(_49_m0[5u]).y != 0u) ? _1535 : _1888;
                                }
                                float frontier_phi_137_pred;
                                float frontier_phi_137_pred_1;
                                float frontier_phi_137_pred_2;
                                uint frontier_phi_137_pred_3;
                                uint frontier_phi_137_pred_4;
                                bool _2456;
                                bool _2458;
                                for (;;)
                                {
                                    _2456 = _2275 < 0.0f;
                                    _2458 = _2456 || ((_2273 < 0.0f) || (_2274 < 0.0f));
                                    if (!_2458)
                                    {
                                        if (!((_2275 > 1.0f) || ((_2273 > _49_m0[51u].x) || (_2274 > _49_m0[51u].y))))
                                        {
                                            frontier_phi_137_pred = _2275;
                                            frontier_phi_137_pred_1 = _2274;
                                            frontier_phi_137_pred_2 = _2273;
                                            frontier_phi_137_pred_3 = _2450;
                                            frontier_phi_137_pred_4 = _2447;
                                            break;
                                        }
                                    }
                                    if (!_2456)
                                    {
                                        frontier_phi_137_pred = _2275;
                                        frontier_phi_137_pred_1 = _2274;
                                        frontier_phi_137_pred_2 = _2273;
                                        frontier_phi_137_pred_3 = 4294967295u;
                                        frontier_phi_137_pred_4 = 1u;
                                        break;
                                    }
                                    float _2791 = (-0.0f) - _2275;
                                    float _2792 = _2791 / _1024;
                                    frontier_phi_137_pred = _2791 + _2275;
                                    frontier_phi_137_pred_1 = (_2792 * _1023) + _2274;
                                    frontier_phi_137_pred_2 = (_2792 * _1021) + _2273;
                                    frontier_phi_137_pred_3 = 4294967295u;
                                    frontier_phi_137_pred_4 = 1u;
                                    break;
                                }
                                _1727 = frontier_phi_137_pred;
                                _1729 = frontier_phi_137_pred_1;
                                _1731 = frontier_phi_137_pred_2;
                                _1906 = frontier_phi_137_pred_3;
                                _1913 = frontier_phi_137_pred_4;
                                if ((_1725 < _209) && (int(_1906) > int(4294967295u)))
                                {
                                    _1887 = _1725;
                                    _1888 = _1889;
                                    _1890 = _1891;
                                    _1892 = _1727;
                                    _1893 = _1729;
                                    _1894 = _1731;
                                    _1895 = _1896;
                                    _1897 = _1898;
                                    _1899 = _1900;
                                    _1901 = _1902;
                                    _1903 = _1904;
                                    _1905 = _1906;
                                    _1907 = _1733;
                                    _1908 = _1735;
                                    _1909 = _1737;
                                    _1910 = _1739;
                                    _1911 = _1741;
                                    _1912 = _1913;
                                    _1914 = _1743;
                                    continue;
                                }
                                else
                                {
                                    break;
                                }
                            }
                            _1724 = _1725;
                            _1726 = _1727;
                            _1728 = _1729;
                            _1730 = _1731;
                            _1732 = _1733;
                            _1734 = _1735;
                            _1736 = _1737;
                            _1738 = _1739;
                            _1740 = _1741;
                            _1742 = _1743;
                        }
                        float _1752 = _49_m0[51u].z * 2.0f;
                        float _1755 = (_1752 * _984) + (-1.0f);
                        float _1756 = ((1.0f - (_49_m0[51u].w * _985)) * 2.0f) + (-1.0f);
                        float _1772 = mad(_190, _977, mad(_183, _1756, _1755 * _176)) + _197;
                        float _1773 = (mad(_187, _977, mad(_180, _1756, _1755 * _173)) + _194) / _1772;
                        float _1774 = (mad(_188, _977, mad(_181, _1756, _1755 * _174)) + _195) / _1772;
                        float _1775 = (mad(_189, _977, mad(_182, _1756, _1755 * _175)) + _196) / _1772;
                        float _1780 = (_1752 * _1730) + (-1.0f);
                        float _1781 = ((1.0f - (_49_m0[51u].w * _1728)) * 2.0f) + (-1.0f);
                        float _1797 = mad(_190, _1726, mad(_183, _1781, _1780 * _176)) + _197;
                        float _1801 = ((mad(_187, _1726, mad(_180, _1781, _1780 * _173)) + _194) / _1797) - _1773;
                        float _1802 = ((mad(_188, _1726, mad(_181, _1781, _1780 * _174)) + _195) / _1797) - _1774;
                        float _1803 = ((mad(_189, _1726, mad(_182, _1781, _1780 * _175)) + _196) / _1797) - _1775;
                        float _1825;
                        if (_1724 > _209)
                        {
                            _1825 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_70_71_ladder;
                            if ((_1728 < 0.0f) || (_1730 < 0.0f))
                            {
                                frontier_phi_70_71_ladder = 0.0f;
                            }
                            else
                            {
                                float frontier_phi_70_71_ladder_80_ladder;
                                if ((_1726 >= 1.0f) || ((_1730 > _49_m0[51u].x) || (_1728 > _49_m0[51u].y)))
                                {
                                    frontier_phi_70_71_ladder_80_ladder = 0.0f;
                                }
                                else
                                {
                                    float frontier_phi_70_71_ladder_80_ladder_88_ladder;
                                    for (;;)
                                    {
                                        if ((abs(_1730 - _259) < (2.0f / _1291)) && (abs(_1728 - _260) < (2.0f / _1292)))
                                        {
                                            frontier_phi_70_71_ladder_80_ladder_88_ladder = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            bool ladder_phi_118;
                                            float frontier_phi_118_pred;
                                            uint _2307;
                                            uint _2308;
                                            bool _2310;
                                            for (;;)
                                            {
                                                _2307 = uint(int(_1730 * _1291));
                                                _2308 = uint(int(_1728 * _1292));
                                                _2310 = (_402 == 1u) && _938;
                                                if (!_2310)
                                                {
                                                    if (!(dot(float3(_1801, _1802, _1803), float3(_1801, _1802, _1803)) < _49_m0[4u].w))
                                                    {
                                                        ladder_phi_118 = false;
                                                        frontier_phi_118_pred = 0.0f;
                                                        break;
                                                    }
                                                }
                                                uint4 _2422 = _27[22u].Load(int3(uint2(_2307, _2308), 0u));
                                                uint _2424 = _2422.x;
                                                float _2758;
                                                float _2759;
                                                float _2760;
                                                if (_2424 == 0u)
                                                {
                                                    uint4 _2596 = _27[1u].Load(int3(uint2(_2307, _2308), 0u));
                                                    uint _2598 = _2596.x;
                                                    float _2606 = (float((_2598 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                                    float _2607 = (float(_2598 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                                    float _2611 = (1.0f - abs(_2606)) - abs(_2607);
                                                    float _2613 = clamp((-0.0f) - _2611, 0.0f, 1.0f);
                                                    float _2614 = (-0.0f) - _2613;
                                                    _2758 = ((_2606 >= 0.0f) ? _2614 : _2613) + _2606;
                                                    _2759 = ((_2607 >= 0.0f) ? _2614 : _2613) + _2607;
                                                    _2760 = _2611;
                                                }
                                                else
                                                {
                                                    float _2628 = (float((_2424 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                                    float _2629 = (float(_2424 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                                    float _2633 = (1.0f - abs(_2628)) - abs(_2629);
                                                    float _2635 = clamp((-0.0f) - _2633, 0.0f, 1.0f);
                                                    float _2636 = (-0.0f) - _2635;
                                                    _2758 = ((_2628 >= 0.0f) ? _2636 : _2635) + _2628;
                                                    _2759 = ((_2629 >= 0.0f) ? _2636 : _2635) + _2629;
                                                    _2760 = _2633;
                                                }
                                                float _2764 = rsqrt(dot(float3(_2758, _2759, _2760), float3(_2758, _2759, _2760)));
                                                if (dot(float3(_2764 * _2758, _2764 * _2759, _2764 * _2760), float3(_1801, _1802, _1803)) > 0.0f)
                                                {
                                                    ladder_phi_118 = true;
                                                    frontier_phi_118_pred = 0.0f;
                                                    break;
                                                }
                                                else
                                                {
                                                    ladder_phi_118 = false;
                                                    frontier_phi_118_pred = 0.0f;
                                                    break;
                                                }
                                            }
                                            if (ladder_phi_118)
                                            {
                                                frontier_phi_70_71_ladder_80_ladder_88_ladder = frontier_phi_118_pred;
                                                break;
                                            }
                                            float _2647 = _49_m0[51u].z * _1730;
                                            float _2648 = _49_m0[51u].w * _1728;
                                            float _2650 = (_1292 / _1291) * 0.0500000007450580596923828125f;
                                            float _2654 = clamp(_2647 / _2650, 0.0f, 1.0f);
                                            float _2655 = clamp(_2648 * 20.0f, 0.0f, 1.0f);
                                            float _2667 = clamp(((_2647 + (-1.0f)) + _2650) / _2650, 0.0f, 1.0f);
                                            float _2668 = clamp((_2648 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                            precise float _2681 = _2654 * _2655;
                                            precise float _2682 = _2681 * _2681;
                                            frontier_phi_70_71_ladder_80_ladder_88_ladder = (((((3.0f - (_2654 * 2.0f)) * float(_483 > 0.0f)) * (3.0f - (_2655 * 2.0f))) * _2682) * (1.0f - ((_2667 * _2667) * (3.0f - (_2667 * 2.0f))))) * (1.0f - ((_2668 * _2668) * (3.0f - (_2668 * 2.0f))));
                                            break;
                                        }
                                    }
                                    frontier_phi_70_71_ladder_80_ladder = frontier_phi_70_71_ladder_80_ladder_88_ladder;
                                }
                                frontier_phi_70_71_ladder = frontier_phi_70_71_ladder_80_ladder;
                            }
                            _1825 = frontier_phi_70_71_ladder;
                        }
                        float _1866 = ((((exp2(log2(clamp((sqrt(((_1774 * _1774) + (_1773 * _1773)) + (_1775 * _1775)) - _44_m0[121u].y) * _44_m0[121u].z, 0.0f, 1.0f)) * _44_m0[121u].w) * _412) * exp2(log2(clamp((_1774 - _44_m0[122u].x) * _44_m0[122u].y, 0.0f, 1.0f)) * _44_m0[122u].z)) * (1.0f - clamp(_398, 0.0f, 1.0f))) * (max(_44_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                        float _1869 = mad(_167, _934, mad(_161, _933, _932 * _155));
                        float _1872 = mad(_168, _934, mad(_162, _933, _932 * _156));
                        float _1875 = mad(_169, _934, mad(_163, _933, _932 * _157));
                        uint4 _1878 = asuint(_54_m0[0u]);
                        float _1880 = float(_1878.x);
                        float _1882 = float(_1878.y);
                        float _1994;
                        float _1995;
                        float _1996;
                        if (_1825 < 1.0f)
                        {
                            float _1978 = (-0.0f) - _977;
                            float _1979 = _1978 / _1024;
                            float _1982 = (_1979 * _1021) + _984;
                            float _1983 = (_1979 * _1023) + _985;
                            float _1984 = _1978 + _977;
                            _1994 = ((_1736 - _1982) * _1825) + _1982;
                            _1995 = ((_1734 - _1983) * _1825) + _1983;
                            _1996 = ((_1732 - _1984) * _1825) + _1984;
                        }
                        else
                        {
                            _1994 = _1736;
                            _1995 = _1734;
                            _1996 = _1732;
                        }
                        float _2006 = ((_1994 * 2.0f) * _49_m0[51u].z) + (-1.0f);
                        float _2007 = ((1.0f - (_49_m0[51u].w * _1995)) * 2.0f) + (-1.0f);
                        float _2023 = mad(_144, _1996, mad(_137, _2007, _2006 * _130)) + _151;
                        float _2027 = ((mad(_141, _1996, mad(_134, _2007, _2006 * _127)) + _148) / _2023) - _956;
                        float _2028 = ((mad(_142, _1996, mad(_135, _2007, _2006 * _128)) + _149) / _2023) - _957;
                        float _2029 = ((mad(_143, _1996, mad(_136, _2007, _2006 * _129)) + _150) / _2023) - _958;
                        float _2035 = sqrt(((_2028 * _2028) + (_2027 * _2027)) + (_2029 * _2029));
                        float _2036 = _2035 * _786;
                        float _2037 = _2035 * _787;
                        float _2038 = _2035 * _788;
                        float _2039 = _2035 * _905;
                        float _2040 = _2035 * _906;
                        float _2041 = _2035 * _907;
                        float _2045 = dot(float3(_2036, _2037, _2038), float3(_772, _775, _778)) * 2.0f;
                        float _2055 = dot(float3(_2039, _2040, _2041), float3(_772, _775, _778)) * 2.0f;
                        float _2082 = (_2036 - (_2045 * _772)) + _956;
                        float _2083 = (_2037 - (_2045 * _775)) + _957;
                        float _2084 = (_2038 - (_2045 * _778)) + _958;
                        float _2096 = mad(_49_m0[24u].w, _2084, mad(_49_m0[23u].w, _2083, _2082 * _49_m0[22u].w)) + _49_m0[25u].w;
                        float _2101 = (_2039 - (_2055 * _772)) + _956;
                        float _2102 = (_2040 - (_2055 * _775)) + _957;
                        float _2103 = (_2041 - (_2055 * _778)) + _958;
                        float _2115 = mad(_49_m0[24u].w, _2103, mad(_49_m0[23u].w, _2102, _2101 * _49_m0[22u].w)) + _49_m0[25u].w;
                        float _2121 = (_49_m0[51u].x * ((((mad(_49_m0[24u].x, _2084, mad(_49_m0[23u].x, _2083, _2082 * _49_m0[22u].x)) + _49_m0[25u].x) / _2096) - ((mad(_49_m0[24u].x, _2103, mad(_49_m0[23u].x, _2102, _2101 * _49_m0[22u].x)) + _49_m0[25u].x) / _2115)) * 0.5f)) * _1880;
                        float _2125 = (_49_m0[51u].y * ((((mad(_49_m0[24u].y, _2103, mad(_49_m0[23u].y, _2102, _2101 * _49_m0[22u].y)) + _49_m0[25u].y) / _2115) - ((mad(_49_m0[24u].y, _2084, mad(_49_m0[23u].y, _2083, _2082 * _49_m0[22u].y)) + _49_m0[25u].y) / _2096)) * 0.5f)) * _1882;
                        float _2143 = clamp(log2((sqrt((_2125 * _2125) + (_2121 * _2121)) * 2.0f) / _49_m0[0u].w) / float(asuint(_49_m0[55u]).x + 4294967295u), 0.0f, 1.0f);
                        bool _2144 = _402 == 1u;
                        float _2228;
                        if (_2144)
                        {
                            float _2217 = clamp((sqrt(((_1802 * _1802) + (_1801 * _1801)) + (_1803 * _1803)) - _49_m0[4u].y) / (_49_m0[4u].z - _49_m0[4u].y), 0.0f, 1.0f);
                            _2228 = (1.0f - ((_2217 * _2217) * (3.0f - (_2217 * 2.0f)))) * _49_m0[61u].z;
                        }
                        else
                        {
                            _2228 = 1.0f;
                        }
                        float _2229 = _2228 * _1825;
                        bool _2230 = _402 != 1u;
                        float _2518;
                        float _2520;
                        float _2522;
                        float _2524;
                        if (_2229 == 0.0f)
                        {
                            float _2369;
                            float _2370;
                            float _2371;
                            float _2372;
                            if (_2230)
                            {
                                float _2350 = _1872 * _1866;
                                float _2354 = rsqrt(dot(float3(_1869, _2350, _1875), float3(_1869, _2350, _1875)));
                                float4 _2364 = _31[4u].SampleLevel(_58, float3(_2354 * _1869, _2354 * _2350, _2354 * _1875), 0.0f);
                                _2369 = _2364.x;
                                _2370 = _2364.y;
                                _2371 = _2364.z;
                                _2372 = 1.0f;
                            }
                            else
                            {
                                _2369 = 0.0f;
                                _2370 = 0.0f;
                                _2371 = 0.0f;
                                _2372 = 0.0f;
                            }
                            _2518 = _2369 * _483;
                            _2520 = _2370 * _483;
                            _2522 = _2371 * _483;
                            _2524 = _2372 * _483;
                        }
                        else
                        {
                            float _2580;
                            float _2582;
                            float _2586;
                            float _2589;
                            if (_15.Load(int3(uint2(uint(_1880 * _1730), uint(_1882 * _1728)), 0u)).x > 0.0f)
                            {
                                uint _2378_dummy_parameter;
                                uint2 _2378 = spvTextureSize(_17, 0u, _2378_dummy_parameter);
                                float4 _2387 = _17.Load(int3(uint2(uint(float(_2378.x) * _1730), uint(float(_2378.y) * _1728)), 0u));
                                float _2391 = _2387.x * 0.5f;
                                float _2392 = _2387.y * (-0.5f);
                                float4 _2409 = _31[7u].SampleLevel(_58, float3(_1869, _1872, _1875), 0.0f);
                                float _2567;
                                if (_2144)
                                {
                                    float frontier_phi_113_112_ladder;
                                    if ((_49_m0[50u].x / ((_49_m0[50u].w + _49_m0[50u].y) - (_49_m0[50u].y * _977))) < 5.0f)
                                    {
                                        float _2718 = sqrt((_2391 * _2391) + (_2392 * _2392));
                                        float frontier_phi_113_112_ladder_128_ladder;
                                        if (_2718 > 0.0500000007450580596923828125f)
                                        {
                                            float _2874 = _1730 - _984;
                                            float _2875 = _1728 - _985;
                                            float frontier_phi_113_112_ladder_128_ladder_143_ladder;
                                            if (_2718 > sqrt((_2874 * _2874) + (_2875 * _2875)))
                                            {
                                                uint4 _2934 = asuint(_54_m0[0u]);
                                                uint _2941 = uint(float(_2934.x) * _1730);
                                                uint _2942 = uint(float(_2934.y) * _1728);
                                                uint4 _2945 = _27[2u].Load(int3(uint2(_2941, _2942), 0u));
                                                uint _2948 = _2945.w;
                                                uint4 _2953 = _27[15u].Load(int3(uint2(_2941, _2942), 0u));
                                                uint _2955 = _2953.y;
                                                uint _2961 = ((_2955 & 64u) != 0u) ? uint((_2955 & 4294967167u) != 66u) : 4294967295u;
                                                uint _2962 = _2948 & 128u;
                                                uint _2964 = (_2962 != 0u) ? 1u : ((_2945.x << 7u) | _2948);
                                                uint4 _2967 = _19.Load(_2964 * 4u);
                                                uint _2968 = _2967.x;
                                                uint _2975 = ((_2968 & 1u) != 0u) ? 0u : 18u;
                                                uint _2995;
                                                if (_2962 == 0u)
                                                {
                                                    _2995 = (((_2968 & 2097152u) != 0u) && (_2961 == uint(min(int(uint(max(int(_2961), int(0u)))), int(1u))))) ? (_2975 | 128u) : _2975;
                                                }
                                                else
                                                {
                                                    _2995 = _2948;
                                                }
                                                float frontier_phi_113_112_ladder_128_ladder_143_ladder_153_ladder;
                                                if (((_2995 & 128u) | (_19.Load((_2964 * 4u) + 1u).x & 512u)) == 0u)
                                                {
                                                    uint4 _3002 = asuint(_54_m0[0u]);
                                                    uint _3011 = uint(float(_3002.x) * (_2391 + _1730));
                                                    uint _3012 = uint(float(_3002.y) * (_2392 + _1728));
                                                    uint4 _3015 = _27[2u].Load(int3(uint2(_3011, _3012), 0u));
                                                    uint _3018 = _3015.w;
                                                    uint4 _3021 = _27[15u].Load(int3(uint2(_3011, _3012), 0u));
                                                    uint _3023 = _3021.y;
                                                    uint _3029 = ((_3023 & 64u) != 0u) ? uint((_3023 & 4294967167u) != 66u) : 4294967295u;
                                                    uint _3030 = _3018 & 128u;
                                                    uint _3032 = (_3030 != 0u) ? 1u : ((_3015.x << 7u) | _3018);
                                                    uint4 _3034 = _19.Load(_3032 * 4u);
                                                    uint _3035 = _3034.x;
                                                    uint _3042 = ((_3035 & 1u) != 0u) ? 0u : 18u;
                                                    uint _3052;
                                                    if (_3030 == 0u)
                                                    {
                                                        _3052 = (((_3035 & 2097152u) != 0u) && (_3029 == uint(min(int(uint(max(int(_3029), int(0u)))), int(1u))))) ? (_3042 | 128u) : _3042;
                                                    }
                                                    else
                                                    {
                                                        _3052 = _3018;
                                                    }
                                                    float frontier_phi_113_112_ladder_128_ladder_143_ladder_153_ladder_156_ladder;
                                                    if ((_3052 & 128u) == 0u)
                                                    {
                                                        frontier_phi_113_112_ladder_128_ladder_143_ladder_153_ladder_156_ladder = ((_19.Load((_3032 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                                    }
                                                    else
                                                    {
                                                        frontier_phi_113_112_ladder_128_ladder_143_ladder_153_ladder_156_ladder = 0.0f;
                                                    }
                                                    frontier_phi_113_112_ladder_128_ladder_143_ladder_153_ladder = frontier_phi_113_112_ladder_128_ladder_143_ladder_153_ladder_156_ladder;
                                                }
                                                else
                                                {
                                                    frontier_phi_113_112_ladder_128_ladder_143_ladder_153_ladder = 1.0f;
                                                }
                                                frontier_phi_113_112_ladder_128_ladder_143_ladder = frontier_phi_113_112_ladder_128_ladder_143_ladder_153_ladder;
                                            }
                                            else
                                            {
                                                frontier_phi_113_112_ladder_128_ladder_143_ladder = 1.0f;
                                            }
                                            frontier_phi_113_112_ladder_128_ladder = frontier_phi_113_112_ladder_128_ladder_143_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_113_112_ladder_128_ladder = 1.0f;
                                        }
                                        frontier_phi_113_112_ladder = frontier_phi_113_112_ladder_128_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_113_112_ladder = 1.0f;
                                    }
                                    _2567 = frontier_phi_113_112_ladder;
                                }
                                else
                                {
                                    _2567 = 1.0f;
                                }
                                float _2569 = _2567 * _2229;
                                float _2585;
                                float _2588;
                                float _2591;
                                if (_1740 == 0u)
                                {
                                    _2585 = _49_m0[54u].x * _2409.x;
                                    _2588 = _49_m0[54u].x * _2409.y;
                                    _2591 = _49_m0[54u].x * _2409.z;
                                }
                                else
                                {
                                    _2585 = _49_m0[1u].w;
                                    _2588 = _49_m0[2u].x;
                                    _2591 = _49_m0[2u].y;
                                }
                                float frontier_phi_115_129_ladder;
                                float frontier_phi_115_129_ladder_1;
                                float frontier_phi_115_129_ladder_2;
                                float frontier_phi_115_129_ladder_3;
                                for (;;)
                                {
                                    if (_1738 > 0.0f)
                                    {
                                        float _2584;
                                        float _2587;
                                        float _2590;
                                        if (_2144)
                                        {
                                            _2584 = 0.0f;
                                            _2587 = 0.0f;
                                            _2590 = 0.0f;
                                        }
                                        else
                                        {
                                            if (!((_402 != 4u) || (_1742 != 0u)))
                                            {
                                                frontier_phi_115_129_ladder = _2591;
                                                frontier_phi_115_129_ladder_1 = _2588;
                                                frontier_phi_115_129_ladder_2 = _2585;
                                                frontier_phi_115_129_ladder_3 = _2569;
                                                break;
                                            }
                                            _2584 = _2585;
                                            _2587 = _2588;
                                            _2590 = _2591;
                                        }
                                        frontier_phi_115_129_ladder = _2590;
                                        frontier_phi_115_129_ladder_1 = _2587;
                                        frontier_phi_115_129_ladder_2 = _2584;
                                        frontier_phi_115_129_ladder_3 = (1.0f - exp2(log2(_1738) * _49_m0[4u].x)) * _2569;
                                        break;
                                    }
                                    else
                                    {
                                        frontier_phi_115_129_ladder = _2591;
                                        frontier_phi_115_129_ladder_1 = _2588;
                                        frontier_phi_115_129_ladder_2 = _2585;
                                        frontier_phi_115_129_ladder_3 = _2569;
                                        break;
                                    }
                                }
                                _2580 = frontier_phi_115_129_ladder_3;
                                _2582 = frontier_phi_115_129_ladder_2;
                                _2586 = frontier_phi_115_129_ladder_1;
                                _2589 = frontier_phi_115_129_ladder;
                            }
                            else
                            {
                                float frontier_phi_115_102_ladder;
                                float frontier_phi_115_102_ladder_1;
                                float frontier_phi_115_102_ladder_2;
                                float frontier_phi_115_102_ladder_3;
                                if (_1724 < _209)
                                {
                                    float4 _2575 = _31[7u].SampleLevel(_58, float3(_1869, _1872, _1875), 0.0f);
                                    frontier_phi_115_102_ladder = _2575.z;
                                    frontier_phi_115_102_ladder_1 = _2575.y;
                                    frontier_phi_115_102_ladder_2 = _2575.x;
                                    frontier_phi_115_102_ladder_3 = _2229;
                                }
                                else
                                {
                                    frontier_phi_115_102_ladder = _2583;
                                    frontier_phi_115_102_ladder_1 = _2583;
                                    frontier_phi_115_102_ladder_2 = _2583;
                                    frontier_phi_115_102_ladder_3 = 0.0f;
                                }
                                _2580 = frontier_phi_115_102_ladder_3;
                                _2582 = frontier_phi_115_102_ladder_2;
                                _2586 = frontier_phi_115_102_ladder_1;
                                _2589 = frontier_phi_115_102_ladder;
                            }
                            float _2754;
                            float _2755;
                            float _2756;
                            float _2757;
                            if (_2230 && (_2580 < 1.0f))
                            {
                                float _2728 = _1872 * _1866;
                                float _2732 = rsqrt(dot(float3(_1869, _2728, _1875), float3(_1869, _2728, _1875)));
                                float4 _2740 = _31[4u].SampleLevel(_58, float3(_2732 * _1869, _2732 * _2728, _2732 * _1875), 0.0f);
                                float _2742 = _2740.x;
                                float _2743 = _2740.y;
                                float _2744 = _2740.z;
                                _2754 = 1.0f;
                                _2755 = ((_2582 - _2742) * _2580) + _2742;
                                _2756 = ((_2586 - _2743) * _2580) + _2743;
                                _2757 = ((_2589 - _2744) * _2580) + _2744;
                            }
                            else
                            {
                                _2754 = _2580;
                                _2755 = _2582;
                                _2756 = _2586;
                                _2757 = _2589;
                            }
                            float _2525 = _2754 * _483;
                            _2518 = _2755 * _2525;
                            _2520 = _2756 * _2525;
                            _2522 = _2757 * _2525;
                            _2524 = _2525;
                        }
                        float _2535 = max(_49_m0[54u].y * _23[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                        float _2536 = _2535 * _2518;
                        float _2537 = _2535 * _2520;
                        float _2538 = _2535 * _2522;
                        float _2543 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2536, max(_2537, _2538)) + 1.0f);
                        float _2547 = min(_2543 * _2536, 0.996078431606292724609375f);
                        float _2548 = min(_2543 * _2537, 0.996078431606292724609375f);
                        float _2549 = min(_2543 * _2538, 0.996078431606292724609375f);
                        _34[uint2(_226, _229)] = float4(_2547, _2548, _2549, _2524);
                        _38[uint2(_226, _229)] = float4(_2143, _406, _2524, _2143);
                        if (_233)
                        {
                            uint _2708 = _226 + 1u;
                            _34[uint2(_2708, _229)] = float4(_2547, _2548, _2549, _2524);
                            _38[uint2(_2708, _229)] = float4(_2143, _406, _2524, _2143);
                        }
                        if (_236)
                        {
                            uint _2867 = _229 + 1u;
                            _34[uint2(_226, _2867)] = float4(_2547, _2548, _2549, _2524);
                            _38[uint2(_226, _2867)] = float4(_2143, _406, _2524, _2143);
                        }
                        if (!_237)
                        {
                            break;
                        }
                        uint _2924 = _226 + 1u;
                        uint _2925 = _229 + 1u;
                        _34[uint2(_2924, _2925)] = float4(_2547, _2548, _2549, _2524);
                        _38[uint2(_2924, _2925)] = float4(_2143, _406, _2524, _2143);
                        break;
                    }
                    else
                    {
                        _34[uint2(_226, _229)] = 0.0f.xxxx;
                        _38[uint2(_226, _229)] = float4(0.0f, _406, 0.0f, 0.0f);
                        if (_233)
                        {
                            uint _1540 = _226 + 1u;
                            _34[uint2(_1540, _229)] = 0.0f.xxxx;
                            _38[uint2(_1540, _229)] = float4(0.0f, _406, 0.0f, 0.0f);
                        }
                        if (_236)
                        {
                            uint _1804 = _229 + 1u;
                            _34[uint2(_226, _1804)] = 0.0f.xxxx;
                            _38[uint2(_226, _1804)] = float4(0.0f, _406, 0.0f, 0.0f);
                        }
                        if (!_237)
                        {
                            break;
                        }
                        uint _1920 = _226 + 1u;
                        uint _1921 = _229 + 1u;
                        _34[uint2(_1920, _1921)] = 0.0f.xxxx;
                        _38[uint2(_1920, _1921)] = float4(0.0f, _406, 0.0f, 0.0f);
                        break;
                    }
                }
                ladder_phi_8 = true;
                frontier_phi_8_pred = _406;
                break;
            }
            float _428 = frontier_phi_8_pred;
            if (ladder_phi_8)
            {
                break;
            }
            _34[uint2(_226, _229)] = 0.0f.xxxx;
            _38[uint2(_226, _229)] = float4(0.0f, _428, 0.0f, 0.0f);
            if (_233)
            {
                uint _448 = _226 + 1u;
                _34[uint2(_448, _229)] = 0.0f.xxxx;
                _38[uint2(_448, _229)] = float4(0.0f, _428, 0.0f, 0.0f);
            }
            if (_236)
            {
                uint _559 = _229 + 1u;
                _34[uint2(_226, _559)] = 0.0f.xxxx;
                _38[uint2(_226, _559)] = float4(0.0f, _428, 0.0f, 0.0f);
            }
            if (!_237)
            {
                break;
            }
            uint _658 = _226 + 1u;
            uint _659 = _229 + 1u;
            _34[uint2(_658, _659)] = 0.0f.xxxx;
            _38[uint2(_658, _659)] = float4(0.0f, _428, 0.0f, 0.0f);
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
