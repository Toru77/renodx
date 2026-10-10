static float _1768;
static uint _2624;
static float _2625;
static float _2626;
static uint _2627;
static float _2628;
static float _2629;
static float _2630;
static float _2631;
static uint _2632;
static uint _2633;
static float _2634;
static float _2635;
static float _2636;
static float _2637;
static float _2638;
static uint _2639;
static float _2645;
static uint _2646;
static float _2647;
static float _2652;
static float _2653;
static float _2654;

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

static RayQuery<RAY_FLAG_NONE> _2187;

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
        _208 = (((gl_WorkGroupID.y << 6u) + gl_WorkGroupID.x) << 6u) + gl_LocalInvocationIndex;
        if (_208 < _38[1u].xxxx.x)
        {
            bool ladder_phi_8;
            uint _220;
            uint _223;
            bool _227;
            bool _230;
            bool _231;
            uint _232;
            uint _233;
            float _253;
            float _254;
            float _261;
            bool _262;
            uint _264;
            uint _265;
            for (;;)
            {
                uint4 _218 = _8.Load(_208);
                uint _219 = _218.x;
                _220 = _219 & 32767u;
                uint _222 = _219 >> 15u;
                _223 = _222 & 16383u;
                _227 = (_219 & 536870912u) != 0u;
                _230 = (_219 & 1073741824u) != 0u;
                _231 = int(_219) < int(0u);
                _232 = uint(_201);
                _233 = uint(_203);
                bool _236 = ((_222 + _219) & 1u) == 0u;
                float _246 = float(int(_220));
                float _247 = float(int(_223));
                _253 = ((_246 + 0.5f) + (_236 ? _44_m0[58u].x : _44_m0[58u].z)) * (1.0f / _201);
                _254 = ((_247 + 0.5f) + (_236 ? _44_m0[58u].y : _44_m0[58u].w)) * (1.0f / _203);
                _261 = _20[21u].Load(int3(uint2(_220, _223), 0u)).x;
                _262 = _261 > 0.0f;
                _264 = uint(_246);
                _265 = uint(_247);
                float _390;
                float _392;
                float _394;
                uint _396;
                float _398;
                float _400;
                uint _402;
                float _404;
                if (_262)
                {
                    uint4 _269 = _24[22u].Load(int3(uint2(_264, _265), 0u));
                    uint _271 = _269.x;
                    float _284 = (float((_271 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _286 = (float(_271 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _291 = (1.0f - abs(_284)) - abs(_286);
                    float _294 = clamp((-0.0f) - _291, 0.0f, 1.0f);
                    float _295 = (-0.0f) - _294;
                    float _300 = ((_284 >= 0.0f) ? _295 : _294) + _284;
                    float _301 = ((_286 >= 0.0f) ? _295 : _294) + _286;
                    float _306 = rsqrt(dot(float3(_300, _301, _291), float3(_300, _301, _291)));
                    uint _318 = _24[23u].Load(int3(uint2(_264, _265), 0u)).y + 4294967295u;
                    uint _321 = uint(int(_318) >> int(31u)) & 3u;
                    uint _326 = _321 + 103u;
                    float _335 = clamp(((float(_271 & 255u) * 0.0039215688593685626983642578125f) - _44_m0[_326].x) / (_44_m0[_326].y - _44_m0[_326].x), 0.0f, 1.0f);
                    _390 = _300 * _306;
                    _392 = _301 * _306;
                    _394 = _306 * _291;
                    _396 = _321 + 1u;
                    _398 = (_335 * _335) * (3.0f - (_335 * 2.0f));
                    _400 = _44_m0[_326].z;
                    _402 = (int(_318) < int(0u)) ? 0u : _318;
                    _404 = 0.0f;
                }
                else
                {
                    uint4 _346 = _24[2u].Load(int3(uint2(_264, _265), 0u));
                    uint _348 = _346.x;
                    uint _349 = _346.w;
                    uint4 _355 = _24[15u].Load(int3(uint2(_264, _265), 0u));
                    uint _357 = _355.y;
                    uint _366 = ((_357 & 64u) != 0u) ? uint((_357 & 4294967167u) != 66u) : 4294967295u;
                    uint _367 = _349 & 128u;
                    uint _370 = (_367 != 0u) ? 1u : ((_348 << 7u) | _349);
                    uint4 _374 = _16.Load(_370 * 4u);
                    uint _375 = _374.x;
                    uint4 _378 = _16.Load((_370 * 4u) + 1u);
                    uint _379 = _378.x;
                    uint4 _382 = _16.Load((_370 * 4u) + 3u);
                    uint _383 = _382.x;
                    uint _386 = ((_375 & 1u) != 0u) ? 0u : 18u;
                    uint _388 = uint(min(int(uint(max(int(_366), int(0u)))), int(1u)));
                    uint _423;
                    uint _424;
                    if (_367 == 0u)
                    {
                        _423 = (((_375 & 2097152u) != 0u) && (_366 == _388)) ? (_386 | 128u) : _386;
                        _424 = _375;
                    }
                    else
                    {
                        _423 = _349;
                        _424 = _375 | ((_348 << 20u) & 134217728u);
                    }
                    uint _433 = _379 & 512u;
                    float _975;
                    float _977;
                    float _979;
                    float _981;
                    float _983;
                    float _985;
                    float _987;
                    float _989;
                    float _991;
                    float _993;
                    float _995;
                    float _997;
                    if (_433 == 0u)
                    {
                        if (!((_424 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _803 = asfloat(_17.Load((_383 * 115u) + 33u).x);
                        uint4 _811 = _24[2u].Load(int3(uint2(_264, _265), 0u));
                        uint _813 = _811.y;
                        uint _814 = _423 & 128u;
                        uint _963;
                        uint _964;
                        uint _965;
                        uint _966;
                        if (_814 == 0u)
                        {
                            _963 = uint(((_424 & 817889384u) | (_379 & 576u)) != 0u) | (((_424 >> 19u) & 1u) ^ 1u);
                            _964 = uint(((_424 & 17825808u) | (_379 & 520u)) != 0u);
                            _965 = uint(((_424 & 46137344u) | (_379 & 2564u)) != 0u);
                            _966 = 0u;
                        }
                        else
                        {
                            _963 = 1u;
                            _964 = _423 & 1u;
                            _965 = 1u;
                            _966 = 1u;
                        }
                        precise float _970 = float(_813 & 127u) * 0.0078740157186985015869140625f;
                        bool _974 = (_424 & 4194304u) == 0u;
                        float _1025;
                        if (_974)
                        {
                            _1025 = _970;
                        }
                        else
                        {
                            _1025 = float(_813 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1087;
                        if ((_424 & 134217728u) == 0u)
                        {
                            uint frontier_phi_49_40_ladder;
                            if ((_814 != 0u) || ((_424 & 17825792u) == 1048576u))
                            {
                                frontier_phi_49_40_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_49_40_ladder = _964;
                            }
                            _1087 = frontier_phi_49_40_ladder;
                        }
                        else
                        {
                            _1087 = _964;
                        }
                        uint4 _1090 = _24[1u].Load(int3(uint2(_264, _265), 0u));
                        uint _1092 = _1090.x;
                        float _1140;
                        float _1142;
                        float _1144;
                        if (_963 == 0u)
                        {
                            _1140 = 0.0f;
                            _1142 = 0.0f;
                            _1144 = 0.0f;
                        }
                        else
                        {
                            float4 _1149 = _20[8u].Load(int3(uint2(_264, _265), 0u));
                            _1140 = _1149.x;
                            _1142 = _1149.y;
                            _1144 = _1149.z;
                        }
                        uint _1222;
                        if (_1087 == 0u)
                        {
                            _1222 = 0u;
                        }
                        else
                        {
                            _1222 = _24[9u].Load(int3(uint2(_264, _265), 0u)).x;
                        }
                        uint _1292;
                        if (_965 == 0u)
                        {
                            _1292 = 0u;
                        }
                        else
                        {
                            _1292 = _24[10u].Load(int3(uint2(_264, _265), 0u)).x;
                        }
                        float _1302 = (float((_1092 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1303 = (float(_1092 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1307 = (1.0f - abs(_1302)) - abs(_1303);
                        float _1309 = clamp((-0.0f) - _1307, 0.0f, 1.0f);
                        float _1310 = (-0.0f) - _1309;
                        float _1315 = ((_1302 >= 0.0f) ? _1310 : _1309) + _1302;
                        float _1316 = ((_1303 >= 0.0f) ? _1310 : _1309) + _1303;
                        float _1320 = rsqrt(dot(float3(_1315, _1316, _1307), float3(_1315, _1316, _1307)));
                        float _1321 = _1315 * _1320;
                        float _1322 = _1316 * _1320;
                        float _1323 = _1320 * _1307;
                        float _976 = float(_1092 & 255u);
                        float _1327 = ((_424 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1413;
                        float _1414;
                        float _1415;
                        float _1416;
                        uint _1417;
                        if ((_379 & 64u) == 0u)
                        {
                            float frontier_phi_78_70_ladder;
                            float frontier_phi_78_70_ladder_1;
                            float frontier_phi_78_70_ladder_2;
                            float frontier_phi_78_70_ladder_3;
                            uint frontier_phi_78_70_ladder_4;
                            if ((_424 & 276824064u) == 0u)
                            {
                                frontier_phi_78_70_ladder = 0.0f;
                                frontier_phi_78_70_ladder_1 = ((_424 & 8u) != 0u) ? _1142 : _1327;
                                frontier_phi_78_70_ladder_2 = 0.0f;
                                frontier_phi_78_70_ladder_3 = 0.0f;
                                frontier_phi_78_70_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_78_70_ladder = 0.0f;
                                frontier_phi_78_70_ladder_1 = _1327;
                                frontier_phi_78_70_ladder_2 = 0.0f;
                                frontier_phi_78_70_ladder_3 = 0.0f;
                                frontier_phi_78_70_ladder_4 = 0u;
                            }
                            _1413 = frontier_phi_78_70_ladder_1;
                            _1414 = frontier_phi_78_70_ladder;
                            _1415 = frontier_phi_78_70_ladder_2;
                            _1416 = frontier_phi_78_70_ladder_3;
                            _1417 = frontier_phi_78_70_ladder_4;
                        }
                        else
                        {
                            float _1370 = (_1142 * 2.0f) + (-1.0f);
                            float _1371 = (_1144 * 2.0f) + (-1.0f);
                            float _1375 = (1.0f - abs(_1370)) - abs(_1371);
                            float _1377 = clamp((-0.0f) - _1375, 0.0f, 1.0f);
                            float _1378 = (-0.0f) - _1377;
                            float _1383 = ((_1370 >= 0.0f) ? _1378 : _1377) + _1370;
                            float _1384 = ((_1371 >= 0.0f) ? _1378 : _1377) + _1371;
                            float _1388 = rsqrt(dot(float3(_1383, _1384, _1375), float3(_1383, _1384, _1375)));
                            _1413 = floor(round(_1140 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1414 = _1383 * _1388;
                            _1415 = _1384 * _1388;
                            _1416 = _1388 * _1375;
                            _1417 = 1u;
                        }
                        float _984;
                        if ((_424 & 32768u) == 0u)
                        {
                            _984 = _1413;
                        }
                        else
                        {
                            float frontier_phi_87_88_ladder;
                            if (_17.Load((_383 * 115u) + 36u).x == 0u)
                            {
                                float _1602 = clamp((_976 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _803;
                                frontier_phi_87_88_ladder = ((_424 & 131072u) != 0u) ? _1602 : ((((clamp((1.21000003814697265625f / (exp2((_1025 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_383 * 115u) + 32u).x)) + 1.0f) * _1602);
                            }
                            else
                            {
                                frontier_phi_87_88_ladder = _803;
                            }
                            _984 = frontier_phi_87_88_ladder;
                        }
                        uint _1470 = _423 & 1u;
                        float _1545;
                        float _1547;
                        float _1549;
                        uint _1551;
                        if (((_424 & 16u) == 0u) || (((_1470 | (_379 & 8u)) | (_424 & 16777216u)) != 0u))
                        {
                            _1545 = _1414;
                            _1547 = _1415;
                            _1549 = _1416;
                            _1551 = _1417;
                        }
                        else
                        {
                            float _1561 = (float(_1222 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1562 = (float(_1222 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1566 = (1.0f - abs(_1561)) - abs(_1562);
                            float _1568 = clamp((-0.0f) - _1566, 0.0f, 1.0f);
                            float _1569 = (-0.0f) - _1568;
                            float _1574 = ((_1561 >= 0.0f) ? _1569 : _1568) + _1561;
                            float _1575 = ((_1562 >= 0.0f) ? _1569 : _1568) + _1562;
                            float _1579 = rsqrt(dot(float3(_1574, _1575, _1566), float3(_1574, _1575, _1566)));
                            _1545 = _1574 * _1579;
                            _1547 = _1575 * _1579;
                            _1549 = _1579 * _1566;
                            _1551 = 1u;
                        }
                        float _978;
                        float _980;
                        float _982;
                        if (_1470 == 0u)
                        {
                            float frontier_phi_111_110_ladder;
                            float frontier_phi_111_110_ladder_1;
                            float frontier_phi_111_110_ladder_2;
                            if (((_423 & 64u) == 0u) && (_966 != 0u))
                            {
                                float2 _1892 = spvUnpackHalf2x16((_1292 >> 17u) & 32736u);
                                float _1893 = _1892.x;
                                float _1896 = (spvUnpackHalf2x16((_1292 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1897 = (spvUnpackHalf2x16((_1292 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1901 = (1.0f - abs(_1896)) - abs(_1897);
                                float _1903 = clamp((-0.0f) - _1901, 0.0f, 1.0f);
                                float _1904 = (-0.0f) - _1903;
                                float _1909 = ((_1896 >= 0.0f) ? _1904 : _1903) + _1896;
                                float _1910 = ((_1897 >= 0.0f) ? _1904 : _1903) + _1897;
                                float _1914 = rsqrt(dot(float3(_1909, _1910, _1901), float3(_1909, _1910, _1901)));
                                float _1924 = (((_1909 * _1914) - _1321) * _1893) + _1321;
                                float _1925 = (((_1910 * _1914) - _1322) * _1893) + _1322;
                                float _1926 = (((_1914 * _1901) - _1323) * _1893) + _1323;
                                float _1930 = rsqrt(dot(float3(_1924, _1925, _1926), float3(_1924, _1925, _1926)));
                                frontier_phi_111_110_ladder = _1926 * _1930;
                                frontier_phi_111_110_ladder_1 = _1924 * _1930;
                                frontier_phi_111_110_ladder_2 = _1925 * _1930;
                            }
                            else
                            {
                                frontier_phi_111_110_ladder = _1323;
                                frontier_phi_111_110_ladder_1 = _1321;
                                frontier_phi_111_110_ladder_2 = _1322;
                            }
                            _978 = frontier_phi_111_110_ladder_1;
                            _980 = frontier_phi_111_110_ladder_2;
                            _982 = frontier_phi_111_110_ladder;
                        }
                        else
                        {
                            _978 = _1321;
                            _980 = _1322;
                            _982 = _1323;
                        }
                        float _992;
                        float _994;
                        float _996;
                        float _998;
                        if (_974)
                        {
                            float frontier_phi_120_119_ladder;
                            float frontier_phi_120_119_ladder_1;
                            float frontier_phi_120_119_ladder_2;
                            float frontier_phi_120_119_ladder_3;
                            if (((_424 & 33554432u) == 0u) || (((_379 & 4u) != 0u) && ((_424 & 8388608u) == 0u)))
                            {
                                frontier_phi_120_119_ladder = 0.0f;
                                frontier_phi_120_119_ladder_1 = 0.0f;
                                frontier_phi_120_119_ladder_2 = 0.0f;
                                frontier_phi_120_119_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2046 = (spvUnpackHalf2x16((_1292 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2047 = (spvUnpackHalf2x16((_1292 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2051 = (1.0f - abs(_2046)) - abs(_2047);
                                float _2053 = clamp((-0.0f) - _2051, 0.0f, 1.0f);
                                float _2054 = (-0.0f) - _2053;
                                float _2059 = ((_2046 >= 0.0f) ? _2054 : _2053) + _2046;
                                float _2060 = ((_2047 >= 0.0f) ? _2054 : _2053) + _2047;
                                float _2064 = rsqrt(dot(float3(_2059, _2060, _2051), float3(_2059, _2060, _2051)));
                                float _2065 = _2059 * _2064;
                                float _2066 = _2060 * _2064;
                                float _2067 = _2064 * _2051;
                                float _2071 = rsqrt(dot(float3(_2065, _2066, _2067), float3(_2065, _2066, _2067)));
                                frontier_phi_120_119_ladder = _2071 * _2067;
                                frontier_phi_120_119_ladder_1 = _2071 * _2066;
                                frontier_phi_120_119_ladder_2 = _2071 * _2065;
                                frontier_phi_120_119_ladder_3 = spvUnpackHalf2x16((_1292 >> 17u) & 32736u).x;
                            }
                            _992 = frontier_phi_120_119_ladder_3;
                            _994 = frontier_phi_120_119_ladder_2;
                            _996 = frontier_phi_120_119_ladder_1;
                            _998 = frontier_phi_120_119_ladder;
                        }
                        else
                        {
                            _992 = 0.0f;
                            _994 = 0.0f;
                            _996 = 0.0f;
                            _998 = 0.0f;
                        }
                        bool _1944 = _1551 != 0u;
                        _975 = _976;
                        _977 = _978;
                        _979 = _980;
                        _981 = _982;
                        _983 = _984;
                        _985 = _1944 ? _1545 : _978;
                        _987 = _1944 ? _1547 : _980;
                        _989 = _1944 ? _1549 : _982;
                        _991 = _992;
                        _993 = _994;
                        _995 = _996;
                        _997 = _998;
                    }
                    else
                    {
                        uint4 _733 = _24[1u].Load(int3(uint2(_264, _265), 0u));
                        uint _735 = _733.x;
                        uint4 _739 = _24[9u].Load(int3(uint2(_264, _265), 0u));
                        uint _741 = _739.x;
                        float _914;
                        float _915;
                        float _916;
                        if ((_424 & 33554432u) == 0u)
                        {
                            float _823 = (float((_735 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _824 = (float(_735 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _828 = (1.0f - abs(_823)) - abs(_824);
                            float _830 = clamp((-0.0f) - _828, 0.0f, 1.0f);
                            float _831 = (-0.0f) - _830;
                            float _836 = ((_823 >= 0.0f) ? _831 : _830) + _823;
                            float _837 = ((_824 >= 0.0f) ? _831 : _830) + _824;
                            float _841 = rsqrt(dot(float3(_836, _837, _828), float3(_836, _837, _828)));
                            _914 = _836 * _841;
                            _915 = _837 * _841;
                            _916 = _841 * _828;
                        }
                        else
                        {
                            float _852 = (float((_741 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _853 = (float(_741 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _857 = (1.0f - abs(_852)) - abs(_853);
                            float _859 = clamp((-0.0f) - _857, 0.0f, 1.0f);
                            float _860 = (-0.0f) - _859;
                            float _865 = ((_852 >= 0.0f) ? _860 : _859) + _852;
                            float _866 = ((_853 >= 0.0f) ? _860 : _859) + _853;
                            float _870 = rsqrt(dot(float3(_865, _866, _857), float3(_865, _866, _857)));
                            _914 = _865 * _870;
                            _915 = _866 * _870;
                            _916 = _870 * _857;
                        }
                        _975 = float(_735 & 255u);
                        _977 = _914;
                        _979 = _915;
                        _981 = _916;
                        _983 = 1.0f;
                        _985 = _914;
                        _987 = _915;
                        _989 = _916;
                        _991 = 0.0f;
                        _993 = 0.0f;
                        _995 = 0.0f;
                        _997 = 0.0f;
                    }
                    precise float _999 = _975 * 0.0039215688593685626983642578125f;
                    if ((_423 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1034 = ((_423 & 128u) | _433) != 0u;
                    uint _1065;
                    if (_1034)
                    {
                        _1065 = 1u;
                    }
                    else
                    {
                        _1065 = (((_424 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _391;
                    float _393;
                    float _395;
                    uint _397;
                    float _1100;
                    if (((_424 & 33554432u) == 0u) || _1034)
                    {
                        bool _1094 = _76 != 0u;
                        uint _1102;
                        if ((_424 & 16u) == 0u)
                        {
                            _1102 = _1065;
                        }
                        else
                        {
                            _1102 = ((_424 & 268435456u) != 0u) ? _1065 : 2u;
                        }
                        _1100 = _983 * _999;
                        _391 = _1094 ? _985 : _977;
                        _393 = _1094 ? _987 : _979;
                        _395 = _1094 ? _989 : _981;
                        _397 = _1102;
                    }
                    else
                    {
                        _1100 = _991;
                        _391 = _993;
                        _393 = _995;
                        _395 = _997;
                        _397 = _1065;
                    }
                    uint _1103 = _397 + 102u;
                    float _1112 = clamp((_1100 - _44_m0[_1103].x) / (_44_m0[_1103].y - _44_m0[_1103].x), 0.0f, 1.0f);
                    _390 = _391;
                    _392 = _393;
                    _394 = _395;
                    _396 = _397;
                    _398 = (_1112 * _1112) * (3.0f - (_1112 * 2.0f));
                    _400 = _44_m0[_1103].z;
                    _402 = (_397 == 1u) ? _388 : 0u;
                    _404 = asfloat(_17.Load((_383 * 115u) + 114u).x);
                }
                if (_398 == 0.0f)
                {
                    ladder_phi_8 = false;
                    break;
                }
                float _421 = float(_233);
                float _422 = float(_232);
                float _440;
                if (_262)
                {
                    _440 = _261;
                }
                else
                {
                    _440 = _12.Load(int3(uint2(uint(int(_253 * _422)), uint(int(_254 * _421))), 0u)).x;
                }
                float _449 = _49_m0[50u].w + _49_m0[50u].y;
                uint _452 = _396 + 63u;
                float _461 = clamp(((_49_m0[50u].x / (_449 - (_49_m0[50u].y * _440))) - _49_m0[_452].y) / (_49_m0[_452].x - _49_m0[_452].y), 0.0f, 1.0f);
                float _473 = ((_253 * 2.0f) * _49_m0[51u].z) + (-1.0f);
                float _474 = ((1.0f - (_49_m0[51u].w * _254)) * 2.0f) + (-1.0f);
                float _490 = mad(_143, _440, mad(_136, _474, _473 * _129)) + _150;
                float _491 = (mad(_140, _440, mad(_133, _474, _473 * _126)) + _147) / _490;
                float _492 = (mad(_141, _440, mad(_134, _474, _473 * _127)) + _148) / _490;
                float _493 = (mad(_142, _440, mad(_135, _474, _473 * _128)) + _149) / _490;
                float _497 = rsqrt(dot(float3(_491, _492, _493), float3(_491, _492, _493)));
                float _498 = _497 * _491;
                float _499 = _497 * _492;
                float _500 = _497 * _493;
                float _503 = mad(_92, _394, mad(_86, _392, _390 * _80));
                float _506 = mad(_93, _394, mad(_87, _392, _390 * _81));
                float _509 = mad(_94, _394, mad(_88, _392, _390 * _82));
                float _513 = dot(float3(_498, _499, _500), float3(_503, _506, _509)) * 2.0f;
                float _517 = _498 - (_513 * _503);
                float _518 = _499 - (_513 * _506);
                float _519 = _500 - (_513 * _509);
                float _529 = sqrt(((_492 * _492) + (_491 * _491)) + (_493 * _493)) * 0.001000000047497451305389404296875f;
                float _540 = ((_529 * _503) + _491) + (_517 * _400);
                float _541 = ((_529 * _506) + _492) + (_518 * _400);
                float _542 = ((_529 * _509) + _493) + (_519 * _400);
                float _558 = mad(_115, _542, mad(_108, _541, _540 * _101)) + _122;
                float _561 = (mad(_114, _542, mad(_107, _541, _540 * _100)) + _121) / _558;
                float _564 = (((mad(_112, _542, mad(_105, _541, _540 * _98)) + _119) / _558) * 0.5f) + 0.5f;
                float _565 = 0.5f - (((mad(_113, _542, mad(_106, _541, _540 * _99)) + _120) / _558) * 0.5f);
                float _568 = _564 * _49_m0[51u].x;
                float _569 = _565 * _49_m0[51u].y;
                float _574 = _540 + (_517 * 0.100000001490116119384765625f);
                float _575 = _541 + (_518 * 0.100000001490116119384765625f);
                float _576 = _542 + (_519 * 0.100000001490116119384765625f);
                float _592 = mad(_115, _576, mad(_108, _575, _574 * _101)) + _122;
                float _601 = _49_m0[51u].x * (((((mad(_112, _576, mad(_105, _575, _574 * _98)) + _119) / _592) * 0.5f) + 0.5f) - _564);
                float _603 = _49_m0[51u].y * ((0.5f - (((mad(_113, _576, mad(_106, _575, _574 * _99)) + _120) / _592) * 0.5f)) - _565);
                float _604 = ((mad(_114, _576, mad(_107, _575, _574 * _100)) + _121) / _592) - _561;
                float _605 = _601 * 10.0f;
                float _607 = _603 * 10.0f;
                float _608 = _604 * 10.0f;
                float _612 = 0.75f / dot(float3(_517, _518, _519), float3(_503, _506, _509));
                float _617 = (_612 * _517) + _540;
                float _618 = (_612 * _518) + _541;
                float _619 = (_612 * _519) + _542;
                float _635 = mad(_115, _619, mad(_108, _618, _617 * _101)) + _122;
                float _644 = _49_m0[51u].x * (((((mad(_112, _619, mad(_105, _618, _617 * _98)) + _119) / _635) * 0.5f) + 0.5f) - _564);
                float _646 = _49_m0[51u].y * ((0.5f - (((mad(_113, _619, mad(_106, _618, _617 * _99)) + _120) / _635) * 0.5f)) - _565);
                float _647 = ((mad(_114, _619, mad(_107, _618, _617 * _100)) + _121) / _635) - _561;
                float _660 = sqrt(((_644 * _644) + (_647 * _647)) + (_646 * _646)) / sqrt(((_605 * _605) + (_608 * _608)) + (_607 * _607));
                float _667 = (_605 != 0.0f) ? (0.100000001490116119384765625f / _601) : 3.4028234663852885981170418348452e+38f;
                float _669 = (_607 != 0.0f) ? (0.100000001490116119384765625f / _603) : 3.4028234663852885981170418348452e+38f;
                float _670 = (_608 != 0.0f) ? (0.100000001490116119384765625f / _604) : 3.4028234663852885981170418348452e+38f;
                float _671 = 1.0f / _422;
                float _672 = 1.0f / _421;
                float _673 = 0.004999999888241291046142578125f / _422;
                float _675 = 0.004999999888241291046142578125f / _421;
                float _684 = float(_605 >= 0.0f);
                float _685 = float(_607 >= 0.0f);
                float _694 = ((_605 < 0.0f) ? ((-0.0f) - _673) : _673) - _568;
                float _697 = ((_607 < 0.0f) ? ((-0.0f) - _675) : _675) - _569;
                float _700 = min((((floor(_568 * _422) + _684) * _671) + _694) * _667, (((floor(_569 * _421) + _685) * _672) + _697) * _669);
                float _704 = (_700 * _605) + _568;
                float _705 = (_700 * _607) + _569;
                float _706 = (_700 * _608) + _561;
                float _709 = _49_m0[50u].x / (_449 - (_706 * _49_m0[50u].y));
                float _720 = max(0.300000011920928955078125f, 10.0f / max(0.00999999977648258209228515625f, max(abs(_601 * 38400.0f), abs(_603 * 21600.0f))));
                uint _750;
                float _754;
                float _756;
                float _758;
                uint _770;
                uint _772;
                uint _752;
                float _760;
                float _762;
                float _764;
                float _766;
                float _768;
                float _774;
                float _776;
                float _778;
                float _780;
                uint _782;
                float _784;
                float _786;
                uint _788;
                uint _749 = 0u;
                uint _751 = 0u;
                float _753 = _706;
                float _755 = _705;
                float _757 = _704;
                float _759 = _700;
                float _761 = _672;
                float _763 = _671;
                float _765 = _421;
                float _767 = _422;
                uint _769 = 0u;
                uint _771 = 0u;
                float _773 = _706;
                float _775 = _705;
                float _777 = _704;
                float _779 = 1.0f;
                uint _781 = 0u;
                float _783 = 0.0f;
                float _785 = 0.0f;
                uint _787 = 1u;
                float _789;
                float _790;
                uint _791;
                uint _792;
                bool _793;
                for (;;)
                {
                    _789 = _767 * _757;
                    _790 = _765 * _755;
                    _791 = uint(int(_789));
                    _792 = uint(int(_790));
                    _793 = _769 == 0u;
                    float _919;
                    if (_793)
                    {
                        _919 = _12.Load(int3(uint2(_791, _792), 0u)).x;
                    }
                    else
                    {
                        _919 = _15.Load(int3(uint2(_791, _792), _769 + 4294967295u)).x;
                    }
                    float _925 = ((_789 >= floor(_767)) || (_790 >= floor(_765))) ? 1.0f : _919;
                    float _939 = (_608 < 0.0f) ? ((_925 - _561) * _670) : 3.4028234663852885981170418348452e+38f;
                    float _941 = min(min((((floor(_789) + _684) * _763) + _694) * _667, (((floor(_790) + _685) * _761) + _697) * _669), _939);
                    bool _942 = _925 < _753;
                    bool _946 = _942 && (asuint(_941) != asuint(_939));
                    float _947 = _942 ? _941 : _759;
                    float _951 = (_947 * _605) + _568;
                    float _952 = (_947 * _607) + _569;
                    float _953 = (_947 * _608) + _561;
                    uint _955 = (_946 ? 1u : 4294967295u) + _769;
                    float _956 = _946 ? 0.5f : 2.0f;
                    float _957 = _956 * _767;
                    float _958 = _956 * _765;
                    float _959 = _946 ? 2.0f : 0.5f;
                    float _960 = _959 * _763;
                    float _961 = _959 * _761;
                    _750 = _749 + 1u;
                    uint _1048;
                    uint _1051;
                    if (int(_955) < int(0u))
                    {
                        float frontier_phi_39_32_ladder;
                        uint frontier_phi_39_32_ladder_1;
                        float frontier_phi_39_32_ladder_2;
                        float frontier_phi_39_32_ladder_3;
                        uint frontier_phi_39_32_ladder_4;
                        float frontier_phi_39_32_ladder_5;
                        float frontier_phi_39_32_ladder_6;
                        float frontier_phi_39_32_ladder_7;
                        uint frontier_phi_39_32_ladder_8;
                        float frontier_phi_39_32_ladder_9;
                        float frontier_phi_39_32_ladder_10;
                        float frontier_phi_39_32_ladder_11;
                        float frontier_phi_39_32_ladder_12;
                        float frontier_phi_39_32_ladder_13;
                        uint frontier_phi_39_32_ladder_14;
                        uint frontier_phi_39_32_ladder_15;
                        float _1011;
                        float _1016;
                        float _1019;
                        bool _1020;
                        for (;;)
                        {
                            float _1009 = _49_m0[50u].w + _49_m0[50u].y;
                            _1011 = _49_m0[50u].x / (_1009 - (_49_m0[50u].y * _925));
                            float _1014 = _49_m0[50u].x / (_1009 - (_49_m0[50u].y * _953));
                            _1016 = abs(_709 - _1014);
                            _1019 = _1014 - _1011;
                            _1020 = _1019 > max(0.00999999977648258209228515625f, _1016 * 0.00999999977648258209228515625f);
                            if (_1020)
                            {
                                uint _1053;
                                if (_751 == 0u)
                                {
                                    uint frontier_phi_45_44_ladder;
                                    if ((_396 == 2u) || (_396 == 4u))
                                    {
                                        if ((_947 < _660) && (abs(_1019) < 2.0f))
                                        {
                                            frontier_phi_39_32_ladder = _777;
                                            frontier_phi_39_32_ladder_1 = _787;
                                            frontier_phi_39_32_ladder_2 = _951;
                                            frontier_phi_39_32_ladder_3 = _952;
                                            frontier_phi_39_32_ladder_4 = 1u;
                                            frontier_phi_39_32_ladder_5 = 0.0f;
                                            frontier_phi_39_32_ladder_6 = _775;
                                            frontier_phi_39_32_ladder_7 = _773;
                                            frontier_phi_39_32_ladder_8 = _955;
                                            frontier_phi_39_32_ladder_9 = _957;
                                            frontier_phi_39_32_ladder_10 = _958;
                                            frontier_phi_39_32_ladder_11 = _960;
                                            frontier_phi_39_32_ladder_12 = _961;
                                            frontier_phi_39_32_ladder_13 = _947;
                                            frontier_phi_39_32_ladder_14 = 1u;
                                            frontier_phi_39_32_ladder_15 = 1u;
                                            break;
                                        }
                                        frontier_phi_45_44_ladder = 1u;
                                    }
                                    else
                                    {
                                        frontier_phi_45_44_ladder = 1u;
                                    }
                                    _1053 = frontier_phi_45_44_ladder;
                                }
                                else
                                {
                                    _1053 = _751;
                                }
                                if (!(_781 == 0u))
                                {
                                    frontier_phi_39_32_ladder = _777;
                                    frontier_phi_39_32_ladder_1 = _787;
                                    frontier_phi_39_32_ladder_2 = _785;
                                    frontier_phi_39_32_ladder_3 = _783;
                                    frontier_phi_39_32_ladder_4 = _781;
                                    frontier_phi_39_32_ladder_5 = _779;
                                    frontier_phi_39_32_ladder_6 = _775;
                                    frontier_phi_39_32_ladder_7 = _773;
                                    frontier_phi_39_32_ladder_8 = _955;
                                    frontier_phi_39_32_ladder_9 = _957;
                                    frontier_phi_39_32_ladder_10 = _958;
                                    frontier_phi_39_32_ladder_11 = _960;
                                    frontier_phi_39_32_ladder_12 = _961;
                                    frontier_phi_39_32_ladder_13 = _947;
                                    frontier_phi_39_32_ladder_14 = _1053;
                                    frontier_phi_39_32_ladder_15 = _771;
                                    break;
                                }
                                bool _1121 = _771 != 0u;
                                frontier_phi_39_32_ladder = _777;
                                frontier_phi_39_32_ladder_1 = _787;
                                frontier_phi_39_32_ladder_2 = _1121 ? _785 : _951;
                                frontier_phi_39_32_ladder_3 = _1121 ? _783 : _952;
                                frontier_phi_39_32_ladder_4 = 0u;
                                frontier_phi_39_32_ladder_5 = _779;
                                frontier_phi_39_32_ladder_6 = _775;
                                frontier_phi_39_32_ladder_7 = _773;
                                frontier_phi_39_32_ladder_8 = 0u;
                                frontier_phi_39_32_ladder_9 = _422;
                                frontier_phi_39_32_ladder_10 = _421;
                                frontier_phi_39_32_ladder_11 = _671;
                                frontier_phi_39_32_ladder_12 = _672;
                                frontier_phi_39_32_ladder_13 = _947 + _720;
                                frontier_phi_39_32_ladder_14 = _1053;
                                frontier_phi_39_32_ladder_15 = ((_396 == 1u) || (asuint(_49_m0[62u]).z == 0u)) ? 1u : _771;
                                break;
                            }
                            else
                            {
                                float _1039 = max(0.100000001490116119384765625f, _1016 * 0.100000001490116119384765625f) * 0.5f;
                                float _1042 = clamp((abs(_1019) - _1039) / _1039, 0.0f, 1.0f);
                                uint _1044 = uint(_1011 < _709);
                                float frontier_phi_39_32_ladder_38_ladder;
                                uint frontier_phi_39_32_ladder_38_ladder_1;
                                float frontier_phi_39_32_ladder_38_ladder_2;
                                float frontier_phi_39_32_ladder_38_ladder_3;
                                uint frontier_phi_39_32_ladder_38_ladder_4;
                                float frontier_phi_39_32_ladder_38_ladder_5;
                                float frontier_phi_39_32_ladder_38_ladder_6;
                                float frontier_phi_39_32_ladder_38_ladder_7;
                                uint frontier_phi_39_32_ladder_38_ladder_8;
                                float frontier_phi_39_32_ladder_38_ladder_9;
                                float frontier_phi_39_32_ladder_38_ladder_10;
                                float frontier_phi_39_32_ladder_38_ladder_11;
                                float frontier_phi_39_32_ladder_38_ladder_12;
                                float frontier_phi_39_32_ladder_38_ladder_13;
                                uint frontier_phi_39_32_ladder_38_ladder_14;
                                uint frontier_phi_39_32_ladder_38_ladder_15;
                                if (_771 == 0u)
                                {
                                    frontier_phi_39_32_ladder_38_ladder = _777;
                                    frontier_phi_39_32_ladder_38_ladder_1 = _1044;
                                    frontier_phi_39_32_ladder_38_ladder_2 = _785;
                                    frontier_phi_39_32_ladder_38_ladder_3 = _783;
                                    frontier_phi_39_32_ladder_38_ladder_4 = _781;
                                    frontier_phi_39_32_ladder_38_ladder_5 = _1042;
                                    frontier_phi_39_32_ladder_38_ladder_6 = _775;
                                    frontier_phi_39_32_ladder_38_ladder_7 = _773;
                                    frontier_phi_39_32_ladder_38_ladder_8 = _955;
                                    frontier_phi_39_32_ladder_38_ladder_9 = _957;
                                    frontier_phi_39_32_ladder_38_ladder_10 = _958;
                                    frontier_phi_39_32_ladder_38_ladder_11 = _960;
                                    frontier_phi_39_32_ladder_38_ladder_12 = _961;
                                    frontier_phi_39_32_ladder_38_ladder_13 = _947;
                                    frontier_phi_39_32_ladder_38_ladder_14 = _751;
                                    frontier_phi_39_32_ladder_38_ladder_15 = uint(_1042 > 0.0f);
                                }
                                else
                                {
                                    frontier_phi_39_32_ladder_38_ladder = _777;
                                    frontier_phi_39_32_ladder_38_ladder_1 = _1044;
                                    frontier_phi_39_32_ladder_38_ladder_2 = _785;
                                    frontier_phi_39_32_ladder_38_ladder_3 = _783;
                                    frontier_phi_39_32_ladder_38_ladder_4 = _781;
                                    frontier_phi_39_32_ladder_38_ladder_5 = _1042;
                                    frontier_phi_39_32_ladder_38_ladder_6 = _775;
                                    frontier_phi_39_32_ladder_38_ladder_7 = _773;
                                    frontier_phi_39_32_ladder_38_ladder_8 = _955;
                                    frontier_phi_39_32_ladder_38_ladder_9 = _957;
                                    frontier_phi_39_32_ladder_38_ladder_10 = _958;
                                    frontier_phi_39_32_ladder_38_ladder_11 = _960;
                                    frontier_phi_39_32_ladder_38_ladder_12 = _961;
                                    frontier_phi_39_32_ladder_38_ladder_13 = _947;
                                    frontier_phi_39_32_ladder_38_ladder_14 = _751;
                                    frontier_phi_39_32_ladder_38_ladder_15 = _771;
                                }
                                frontier_phi_39_32_ladder = frontier_phi_39_32_ladder_38_ladder;
                                frontier_phi_39_32_ladder_1 = frontier_phi_39_32_ladder_38_ladder_1;
                                frontier_phi_39_32_ladder_2 = frontier_phi_39_32_ladder_38_ladder_2;
                                frontier_phi_39_32_ladder_3 = frontier_phi_39_32_ladder_38_ladder_3;
                                frontier_phi_39_32_ladder_4 = frontier_phi_39_32_ladder_38_ladder_4;
                                frontier_phi_39_32_ladder_5 = frontier_phi_39_32_ladder_38_ladder_5;
                                frontier_phi_39_32_ladder_6 = frontier_phi_39_32_ladder_38_ladder_6;
                                frontier_phi_39_32_ladder_7 = frontier_phi_39_32_ladder_38_ladder_7;
                                frontier_phi_39_32_ladder_8 = frontier_phi_39_32_ladder_38_ladder_8;
                                frontier_phi_39_32_ladder_9 = frontier_phi_39_32_ladder_38_ladder_9;
                                frontier_phi_39_32_ladder_10 = frontier_phi_39_32_ladder_38_ladder_10;
                                frontier_phi_39_32_ladder_11 = frontier_phi_39_32_ladder_38_ladder_11;
                                frontier_phi_39_32_ladder_12 = frontier_phi_39_32_ladder_38_ladder_12;
                                frontier_phi_39_32_ladder_13 = frontier_phi_39_32_ladder_38_ladder_13;
                                frontier_phi_39_32_ladder_14 = frontier_phi_39_32_ladder_38_ladder_14;
                                frontier_phi_39_32_ladder_15 = frontier_phi_39_32_ladder_38_ladder_15;
                                break;
                            }
                        }
                        _788 = frontier_phi_39_32_ladder_1;
                        _786 = frontier_phi_39_32_ladder_2;
                        _784 = frontier_phi_39_32_ladder_3;
                        _782 = frontier_phi_39_32_ladder_4;
                        _780 = frontier_phi_39_32_ladder_5;
                        _778 = frontier_phi_39_32_ladder;
                        _776 = frontier_phi_39_32_ladder_6;
                        _774 = frontier_phi_39_32_ladder_7;
                        _1048 = frontier_phi_39_32_ladder_15;
                        _1051 = frontier_phi_39_32_ladder_8;
                        _768 = frontier_phi_39_32_ladder_9;
                        _766 = frontier_phi_39_32_ladder_10;
                        _764 = frontier_phi_39_32_ladder_11;
                        _762 = frontier_phi_39_32_ladder_12;
                        _760 = frontier_phi_39_32_ladder_13;
                        _752 = frontier_phi_39_32_ladder_14;
                    }
                    else
                    {
                        bool _1021 = _771 != 0u;
                        _788 = _787;
                        _786 = _785;
                        _784 = _783;
                        _782 = _781;
                        _780 = _779;
                        _778 = _1021 ? _777 : _951;
                        _776 = _1021 ? _775 : _952;
                        _774 = _1021 ? _773 : _953;
                        _1048 = _771;
                        _1051 = _955;
                        _768 = _957;
                        _766 = _958;
                        _764 = _960;
                        _762 = _961;
                        _760 = _947;
                        _752 = _751;
                    }
                    float frontier_phi_55_pred;
                    uint frontier_phi_55_pred_1;
                    uint frontier_phi_55_pred_2;
                    float frontier_phi_55_pred_3;
                    float frontier_phi_55_pred_4;
                    bool _1056;
                    bool _1058;
                    for (;;)
                    {
                        _1056 = _953 < 0.0f;
                        _1058 = _1056 || ((_951 < 0.0f) || (_952 < 0.0f));
                        if (!_1058)
                        {
                            if (!((_953 > 1.0f) || ((_951 > _49_m0[51u].x) || (_952 > _49_m0[51u].y))))
                            {
                                frontier_phi_55_pred = _951;
                                frontier_phi_55_pred_1 = _1048;
                                frontier_phi_55_pred_2 = _1051;
                                frontier_phi_55_pred_3 = _952;
                                frontier_phi_55_pred_4 = _953;
                                break;
                            }
                        }
                        if (!_1056)
                        {
                            frontier_phi_55_pred = _951;
                            frontier_phi_55_pred_1 = 1u;
                            frontier_phi_55_pred_2 = 4294967295u;
                            frontier_phi_55_pred_3 = _952;
                            frontier_phi_55_pred_4 = _953;
                            break;
                        }
                        float _1130 = (-0.0f) - _953;
                        float _1131 = _1130 / _608;
                        frontier_phi_55_pred = (_1131 * _605) + _951;
                        frontier_phi_55_pred_1 = 1u;
                        frontier_phi_55_pred_2 = 4294967295u;
                        frontier_phi_55_pred_3 = (_1131 * _607) + _952;
                        frontier_phi_55_pred_4 = _1130 + _953;
                        break;
                    }
                    _758 = frontier_phi_55_pred;
                    _772 = frontier_phi_55_pred_1;
                    _770 = frontier_phi_55_pred_2;
                    _756 = frontier_phi_55_pred_3;
                    _754 = frontier_phi_55_pred_4;
                    if ((_750 < 128u) && (int(_770) > int(4294967295u)))
                    {
                        _749 = _750;
                        _751 = _752;
                        _753 = _754;
                        _755 = _756;
                        _757 = _758;
                        _759 = _760;
                        _761 = _762;
                        _763 = _764;
                        _765 = _766;
                        _767 = _768;
                        _769 = _770;
                        _771 = _772;
                        _773 = _774;
                        _775 = _776;
                        _777 = _778;
                        _779 = _780;
                        _781 = _782;
                        _783 = _784;
                        _785 = _786;
                        _787 = _788;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                float _1158 = ((_461 * _461) * _398) * (3.0f - (_461 * 2.0f));
                bool _1159 = dot(float3(_517, _518, _519), float3(_498, _499, _500)) < 0.0f;
                bool _1160 = _750 > 127u;
                uint _1161 = _1160 ? 1u : _772;
                float _1170 = _49_m0[51u].z * 2.0f;
                float _1173 = (_1170 * _568) + (-1.0f);
                float _1174 = ((1.0f - (_49_m0[51u].w * _569)) * 2.0f) + (-1.0f);
                float _1190 = mad(_189, _561, mad(_182, _1174, _1173 * _175)) + _196;
                float _1191 = (mad(_186, _561, mad(_179, _1174, _1173 * _172)) + _193) / _1190;
                float _1192 = (mad(_187, _561, mad(_180, _1174, _1173 * _173)) + _194) / _1190;
                float _1193 = (mad(_188, _561, mad(_181, _1174, _1173 * _174)) + _195) / _1190;
                float _1198 = (_1170 * _758) + (-1.0f);
                float _1199 = ((1.0f - (_49_m0[51u].w * _756)) * 2.0f) + (-1.0f);
                float _1215 = mad(_189, _754, mad(_182, _1199, _1198 * _175)) + _196;
                float _1219 = ((mad(_186, _754, mad(_179, _1199, _1198 * _172)) + _193) / _1215) - _1191;
                float _1220 = ((mad(_187, _754, mad(_180, _1199, _1198 * _173)) + _194) / _1215) - _1192;
                float _1221 = ((mad(_188, _754, mad(_181, _1199, _1198 * _174)) + _195) / _1215) - _1193;
                float _1232;
                uint _1234;
                float _1236;
                if (_750 < 129u)
                {
                    float frontier_phi_64_63_ladder;
                    uint frontier_phi_64_63_ladder_1;
                    float frontier_phi_64_63_ladder_2;
                    if ((_758 < 0.0f) || (_756 < 0.0f))
                    {
                        frontier_phi_64_63_ladder = 0.0f;
                        frontier_phi_64_63_ladder_1 = _1161;
                        frontier_phi_64_63_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_64_63_ladder_67_ladder;
                        uint frontier_phi_64_63_ladder_67_ladder_1;
                        float frontier_phi_64_63_ladder_67_ladder_2;
                        if ((_754 >= 1.0f) || ((_758 > _49_m0[51u].x) || (_756 > _49_m0[51u].y)))
                        {
                            frontier_phi_64_63_ladder_67_ladder = 0.0f;
                            frontier_phi_64_63_ladder_67_ladder_1 = _1161;
                            frontier_phi_64_63_ladder_67_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_64_63_ladder_67_ladder_72_ladder;
                            uint frontier_phi_64_63_ladder_67_ladder_72_ladder_1;
                            float frontier_phi_64_63_ladder_67_ladder_72_ladder_2;
                            for (;;)
                            {
                                if ((abs(_758 - _253) < (2.0f / _422)) && (abs(_756 - _254) < (2.0f / _421)))
                                {
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder = 0.0f;
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder_1 = _1161;
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_101;
                                    float frontier_phi_101_pred;
                                    uint frontier_phi_101_pred_1;
                                    float frontier_phi_101_pred_2;
                                    uint _1423;
                                    uint _1424;
                                    bool _1426;
                                    for (;;)
                                    {
                                        _1423 = uint(int(_758 * _422));
                                        _1424 = uint(int(_756 * _421));
                                        _1426 = (_396 == 1u) && _1159;
                                        if (!_1426)
                                        {
                                            if (!(dot(float3(_1219, _1220, _1221), float3(_1219, _1220, _1221)) < 0.000899999984540045261383056640625f))
                                            {
                                                ladder_phi_101 = false;
                                                frontier_phi_101_pred = 0.0f;
                                                frontier_phi_101_pred_1 = _1161;
                                                frontier_phi_101_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1483 = _24[22u].Load(int3(uint2(_1423, _1424), 0u));
                                        uint _1485 = _1483.x;
                                        float _1790;
                                        float _1791;
                                        float _1792;
                                        if (_1485 == 0u)
                                        {
                                            uint4 _1606 = _24[1u].Load(int3(uint2(_1423, _1424), 0u));
                                            uint _1608 = _1606.x;
                                            float _1616 = (float((_1608 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1617 = (float(_1608 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1621 = (1.0f - abs(_1616)) - abs(_1617);
                                            float _1623 = clamp((-0.0f) - _1621, 0.0f, 1.0f);
                                            float _1624 = (-0.0f) - _1623;
                                            _1790 = ((_1616 >= 0.0f) ? _1624 : _1623) + _1616;
                                            _1791 = ((_1617 >= 0.0f) ? _1624 : _1623) + _1617;
                                            _1792 = _1621;
                                        }
                                        else
                                        {
                                            float _1638 = (float((_1485 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1639 = (float(_1485 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1643 = (1.0f - abs(_1638)) - abs(_1639);
                                            float _1645 = clamp((-0.0f) - _1643, 0.0f, 1.0f);
                                            float _1646 = (-0.0f) - _1645;
                                            _1790 = ((_1638 >= 0.0f) ? _1646 : _1645) + _1638;
                                            _1791 = ((_1639 >= 0.0f) ? _1646 : _1645) + _1639;
                                            _1792 = _1643;
                                        }
                                        float _1796 = rsqrt(dot(float3(_1790, _1791, _1792), float3(_1790, _1791, _1792)));
                                        if (dot(float3(_1796 * _1790, _1796 * _1791, _1796 * _1792), float3(_1219, _1220, _1221)) > 0.0f)
                                        {
                                            ladder_phi_101 = true;
                                            frontier_phi_101_pred = 0.0f;
                                            frontier_phi_101_pred_1 = _1161;
                                            frontier_phi_101_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_101 = false;
                                            frontier_phi_101_pred = 0.0f;
                                            frontier_phi_101_pred_1 = _1161;
                                            frontier_phi_101_pred_2 = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_101)
                                    {
                                        frontier_phi_64_63_ladder_67_ladder_72_ladder = frontier_phi_101_pred;
                                        frontier_phi_64_63_ladder_67_ladder_72_ladder_1 = frontier_phi_101_pred_1;
                                        frontier_phi_64_63_ladder_67_ladder_72_ladder_2 = frontier_phi_101_pred_2;
                                        break;
                                    }
                                    float _1657 = _49_m0[51u].z * _758;
                                    float _1658 = _49_m0[51u].w * _756;
                                    float _1660 = (_421 / _422) * 0.0500000007450580596923828125f;
                                    float _1665 = clamp(_1657 / _1660, 0.0f, 1.0f);
                                    float _1666 = clamp(_1658 * 20.0f, 0.0f, 1.0f);
                                    float _1678 = clamp(((_1657 + (-1.0f)) + _1660) / _1660, 0.0f, 1.0f);
                                    float _1679 = clamp((_1658 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1690 = _1665 * _1666;
                                    precise float _1691 = _1690 * _1690;
                                    float _1695 = ((((3.0f - (_1666 * 2.0f)) * (3.0f - (_1665 * 2.0f))) * _1691) * (1.0f - ((_1678 * _1678) * (3.0f - (_1678 * 2.0f))))) * (1.0f - ((_1679 * _1679) * (3.0f - (_1679 * 2.0f))));
                                    bool _1698 = (_1161 != 0u) || (_1695 >= 1.0f);
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder = _1695 * float(_1158 > 0.0f);
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder_1 = _1698 ? _1161 : 1u;
                                    frontier_phi_64_63_ladder_67_ladder_72_ladder_2 = _1698 ? 0.0f : _1695;
                                    break;
                                }
                            }
                            frontier_phi_64_63_ladder_67_ladder = frontier_phi_64_63_ladder_67_ladder_72_ladder;
                            frontier_phi_64_63_ladder_67_ladder_1 = frontier_phi_64_63_ladder_67_ladder_72_ladder_1;
                            frontier_phi_64_63_ladder_67_ladder_2 = frontier_phi_64_63_ladder_67_ladder_72_ladder_2;
                        }
                        frontier_phi_64_63_ladder = frontier_phi_64_63_ladder_67_ladder;
                        frontier_phi_64_63_ladder_1 = frontier_phi_64_63_ladder_67_ladder_1;
                        frontier_phi_64_63_ladder_2 = frontier_phi_64_63_ladder_67_ladder_2;
                    }
                    _1232 = frontier_phi_64_63_ladder_2;
                    _1234 = frontier_phi_64_63_ladder_1;
                    _1236 = frontier_phi_64_63_ladder;
                }
                else
                {
                    _1232 = 0.0f;
                    _1234 = _1161;
                    _1236 = 0.0f;
                }
                uint _1347;
                float _1348;
                float _1351;
                float _1352;
                float _1353;
                float _1355;
                float _1277;
                float _1280;
                float _1283;
                float _1286;
                float _1290;
                float _1291;
                for (;;)
                {
                    _1277 = ((((exp2(log2(clamp((sqrt(((_1192 * _1192) + (_1191 * _1191)) + (_1193 * _1193)) - _44_m0[121u].y) * _44_m0[121u].z, 0.0f, 1.0f)) * _44_m0[121u].w) * _404) * exp2(log2(clamp((_1192 - _44_m0[122u].x) * _44_m0[122u].y, 0.0f, 1.0f)) * _44_m0[122u].z)) * (1.0f - clamp(_392, 0.0f, 1.0f))) * (max(_44_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    _1280 = mad(_166, _519, mad(_160, _518, _517 * _154));
                    _1283 = mad(_167, _519, mad(_161, _518, _517 * _155));
                    _1286 = mad(_168, _519, mad(_162, _518, _517 * _156));
                    bool _1289 = (_1234 != 0u) || (_1236 < 1.0f);
                    _1290 = _1289 ? 0.0f : 1.0f;
                    _1291 = _1289 ? 0.0f : 0.5f;
                    if (_1289)
                    {
                        bool _1342 = _396 == 1u;
                        uint4 _1346 = asuint(_49_m0[60u]);
                        if (_1342)
                        {
                            if (int(_402) < int(1u))
                            {
                                if (_1346.x == 0u)
                                {
                                    _1347 = 0u;
                                    _1348 = 9899999600270360182784.0f;
                                    _1351 = 0.0f;
                                    _1352 = 0.0f;
                                    _1353 = 0.0f;
                                    _1355 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1346.y == 0u)
                                {
                                    _1347 = 0u;
                                    _1348 = 9899999600270360182784.0f;
                                    _1351 = 0.0f;
                                    _1352 = 0.0f;
                                    _1353 = 0.0f;
                                    _1355 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1346.z == 0u)
                            {
                                _1347 = 0u;
                                _1348 = 9899999600270360182784.0f;
                                _1351 = 0.0f;
                                _1352 = 0.0f;
                                _1353 = 0.0f;
                                _1355 = 0.0f;
                                break;
                            }
                        }
                        if (_1232 > 0.0f)
                        {
                            _1347 = 0u;
                            _1348 = 9899999600270360182784.0f;
                            _1351 = 0.0f;
                            _1352 = 1.0f;
                            _1353 = 1000.0f;
                            _1355 = 0.5f;
                            break;
                        }
                        if (((_778 <= 0.0f) || (_776 <= 0.0f)) || (_774 <= 0.0f))
                        {
                            _1347 = 0u;
                            _1348 = 9899999600270360182784.0f;
                            _1351 = 0.0f;
                            _1352 = 1.0f;
                            _1353 = 1000.0f;
                            _1355 = 0.5f;
                            break;
                        }
                        if ((_774 >= 1.0f) || ((_778 >= _49_m0[51u].x) || (_776 >= _49_m0[51u].y)))
                        {
                            _1347 = 0u;
                            _1348 = 9899999600270360182784.0f;
                            _1351 = 0.0f;
                            _1352 = 1.0f;
                            _1353 = 1000.0f;
                            _1355 = 0.5f;
                            break;
                        }
                        uint _2072;
                        uint _2074;
                        uint _1808;
                        uint _1809;
                        bool _1815;
                        for (;;)
                        {
                            _1808 = uint(clamp(_786, 0.0f, 1.0f) * _201);
                            _1809 = uint(clamp(_784, 0.0f, 1.0f) * _203);
                            _1815 = _20[21u].Load(int3(uint2(_1808, _1809), 0u)).x > 0.0f;
                            if (_1815)
                            {
                                uint _1950 = _24[23u].Load(int3(uint2(_1808, _1809), 0u)).y + 4294967295u;
                                _2072 = (uint(int(_1950) >> int(31u)) & 3u) + 1u;
                                _2074 = (int(_1950) < int(0u)) ? 0u : _1950;
                                break;
                            }
                            else
                            {
                                uint4 _1958 = _24[2u].Load(int3(uint2(_1808, _1809), 0u));
                                uint _1961 = _1958.w;
                                uint4 _1966 = _24[15u].Load(int3(uint2(_1808, _1809), 0u));
                                uint _1968 = _1966.y;
                                uint _1974 = ((_1968 & 64u) != 0u) ? uint((_1968 & 4294967167u) != 66u) : 4294967295u;
                                uint _1975 = _1961 & 128u;
                                uint _1977 = (_1975 != 0u) ? 1u : ((_1958.x << 7u) | _1961);
                                uint4 _1980 = _16.Load(_1977 * 4u);
                                uint _1981 = _1980.x;
                                uint _1988 = ((_1981 & 1u) != 0u) ? 0u : 18u;
                                uint _1990 = uint(min(int(uint(max(int(_1974), int(0u)))), int(1u)));
                                uint _2085;
                                if (_1975 == 0u)
                                {
                                    _2085 = (((_1981 & 2097152u) != 0u) && (_1974 == _1990)) ? (_1988 | 128u) : _1988;
                                }
                                else
                                {
                                    _2085 = _1961;
                                }
                                uint _2086 = _16.Load((_1977 * 4u) + 1u).x & 512u;
                                bool _2089 = (_2085 & 144u) == 0u;
                                if (_2086 == 0u)
                                {
                                    if (_2089 || ((_1981 & 1u) != 0u))
                                    {
                                        _2072 = 0u;
                                        _2074 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_2089)
                                    {
                                        _2072 = 0u;
                                        _2074 = 0u;
                                        break;
                                    }
                                }
                                bool _2162 = ((_2085 & 128u) | _2086) != 0u;
                                uint _2073;
                                if (_2162)
                                {
                                    _2073 = 1u;
                                }
                                else
                                {
                                    _2073 = (((_1981 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_1981 & 268435472u) == 16u) && (((_1981 & 33554432u) == 0u) || _2162))
                                {
                                    _2072 = 2u;
                                    _2074 = 0u;
                                    break;
                                }
                                _2072 = _2073;
                                _2074 = (_2073 == 1u) ? _1990 : 0u;
                                break;
                            }
                        }
                        if ((_396 != _2072) || (_402 != _2074))
                        {
                            _1347 = 0u;
                            _1348 = 9899999600270360182784.0f;
                            _1351 = 0.0f;
                            _1352 = 1.0f;
                            _1353 = 1000.0f;
                            _1355 = 0.5f;
                            break;
                        }
                        float _2100 = _778 * 2.0f;
                        float _2103 = (_49_m0[51u].z * _2100) + (-1.0f);
                        float _2104 = ((1.0f - (_49_m0[51u].w * _776)) * 2.0f) + (-1.0f);
                        float _2120 = mad(_189, _774, mad(_182, _2104, _2103 * _175)) + _196;
                        float _2121 = (mad(_186, _774, mad(_179, _2104, _2103 * _172)) + _193) / _2120;
                        float _2122 = (mad(_187, _774, mad(_180, _2104, _2103 * _173)) + _194) / _2120;
                        float _2123 = (mad(_188, _774, mad(_181, _2104, _2103 * _174)) + _195) / _2120;
                        if (sqrt(((_2122 * _2122) + (_2121 * _2121)) + (_2123 * _2123)) > _49_m0[58u].w)
                        {
                            _1347 = 0u;
                            _1348 = 9899999600270360182784.0f;
                            _1351 = 0.0f;
                            _1352 = 0.0f;
                            _1353 = 1000.0f;
                            _1355 = 0.5f;
                            break;
                        }
                        float _2142 = _2121 - _1191;
                        float _2143 = _2122 - _1192;
                        float _2144 = _2123 - _1193;
                        float _2150 = sqrt(((_2143 * _2143) + (_2142 * _2142)) + (_2144 * _2144));
                        float _2158 = min(_49_m0[59u].y, max(0.0f, _2150 + (-0.001000000047497451305389404296875f)));
                        float _2165;
                        if (_1342)
                        {
                            _2165 = min(_49_m0[59u].x, _2158 + 10.0f);
                        }
                        else
                        {
                            _2165 = _49_m0[59u].x;
                        }
                        float _2166 = _2165 - _2150;
                        if (!(_2166 > 0.0f))
                        {
                            _1347 = 0u;
                            _1348 = 9899999600270360182784.0f;
                            _1351 = 1.0f;
                            _1352 = 1.0f;
                            _1353 = 0.0f;
                            _1355 = 0.5f;
                            break;
                        }
                        float _2182 = _2121 - (_2158 * _1280);
                        float _2183 = _2122 - (_2158 * _1283);
                        float _2184 = _2123 - (_2158 * _1286);
                        RayDesc _2ident = {float3(mad(_2184, _49_m0[46u].z, mad(_2183, _49_m0[46u].y, _49_m0[46u].x * _2182)) + _49_m0[46u].w, mad(_2184, _49_m0[47u].z, mad(_2183, _49_m0[47u].y, _49_m0[47u].x * _2182)) + _49_m0[47u].w, mad(_2184, _49_m0[48u].z, mad(_2183, _49_m0[48u].y, _49_m0[48u].x * _2182)) + _49_m0[48u].w), 0.0f, float3(mad(_1286, _49_m0[46u].z, mad(_1283, _49_m0[46u].y, _49_m0[46u].x * _1280)), mad(_1286, _49_m0[47u].z, mad(_1283, _49_m0[47u].y, _49_m0[47u].x * _1280)), mad(_1286, _49_m0[48u].z, mad(_1283, _49_m0[48u].y, _49_m0[48u].x * _1280))), _2166};
                        _2187.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2234 = _2187.Proceed();
                        uint _2235 = _2187.CommittedStatus();
                        if (!(_2235 == 1u))
                        {
                            _1347 = 0u;
                            _1348 = 9899999600270360182784.0f;
                            _1351 = 1.0f;
                            _1352 = 0.0f;
                            _1353 = 0.0f;
                            _1355 = 0.5f;
                            break;
                        }
                        float _2238 = _2187.CommittedRayT();
                        if (!((_2238 < _2166) && (_2238 > 0.0f)))
                        {
                            _1347 = 0u;
                            _1348 = 9899999600270360182784.0f;
                            _1351 = 1.0f;
                            _1352 = 0.0f;
                            _1353 = 0.0f;
                            _1355 = 0.5f;
                            break;
                        }
                        float _2250 = (_49_m0[51u].z * _2100) + (-1.0f);
                        float _2251 = ((1.0f - (_49_m0[51u].w * _776)) * 2.0f) + (-1.0f);
                        float _2267 = mad(_143, _774, mad(_136, _2251, _2250 * _129)) + _150;
                        float _2271 = _2238 - _2158;
                        float _2275 = ((mad(_140, _774, mad(_133, _2251, _2250 * _126)) + _147) / _2267) + (_2271 * _517);
                        float _2276 = ((mad(_141, _774, mad(_134, _2251, _2250 * _127)) + _148) / _2267) + (_2271 * _518);
                        float _2277 = ((mad(_142, _774, mad(_135, _2251, _2250 * _128)) + _149) / _2267) + (_2271 * _519);
                        float _2289 = mad(_115, _2277, mad(_108, _2276, _2275 * _101)) + _122;
                        float _2299 = (_49_m0[51u].z * _49_m0[51u].x) * ((((mad(_112, _2277, mad(_105, _2276, _2275 * _98)) + _119) / _2289) * 0.5f) + 0.5f);
                        float _2301 = (_49_m0[51u].w * _49_m0[51u].y) * (0.5f - (((mad(_113, _2277, mad(_106, _2276, _2275 * _99)) + _120) / _2289) * 0.5f));
                        float _2303 = (_203 / _201) * 0.0500000007450580596923828125f;
                        float _2306 = clamp(_2299 / _2303, 0.0f, 1.0f);
                        float _2307 = clamp(_2301 * 20.0f, 0.0f, 1.0f);
                        float _2317 = clamp(((_2303 + (-1.0f)) + _2299) / _2303, 0.0f, 1.0f);
                        float _2318 = clamp((_2301 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2329 = _2306 * _2307;
                        precise float _2330 = _2329 * _2329;
                        if ((((((3.0f - (_2307 * 2.0f)) * (3.0f - (_2306 * 2.0f))) * _2330) * (1.0f - ((_2317 * _2317) * (3.0f - (_2317 * 2.0f))))) * (1.0f - ((_2318 * _2318) * (3.0f - (_2318 * 2.0f))))) < 1.0f)
                        {
                            _1347 = 0u;
                            _1348 = 9899999600270360182784.0f;
                            _1351 = _1290;
                            _1352 = _1290;
                            _1353 = 0.0f;
                            _1355 = _1291;
                            break;
                        }
                        _1347 = 1u;
                        _1348 = (_2150 - _2158) + _2238;
                        _1351 = 0.0f;
                        _1352 = 1.0f;
                        _1353 = 0.0f;
                        _1355 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1347 = 0u;
                        _1348 = 9899999600270360182784.0f;
                        _1351 = 1.0f;
                        _1352 = 1.0f;
                        _1353 = 0.0f;
                        _1355 = 0.5f;
                        break;
                    }
                }
                bool _1356 = _396 == 1u;
                float _1405;
                if (_1356)
                {
                    float _1438;
                    if (_1347 == 0u)
                    {
                        _1438 = sqrt(((_1220 * _1220) + (_1219 * _1219)) + (_1221 * _1221));
                    }
                    else
                    {
                        _1438 = _1348;
                    }
                    float _1442 = clamp((_1438 + (-1.0f)) * 0.111111111938953399658203125f, 0.0f, 1.0f);
                    _1405 = (1.0f - ((_1442 * _1442) * (3.0f - (_1442 * 2.0f)))) * _49_m0[61u].z;
                }
                else
                {
                    _1405 = 1.0f;
                }
                float _1407 = _1405 * _1236;
                bool _1408 = _396 != 1u;
                float _1816;
                float _1818;
                float _1820;
                float _1822;
                if (_1407 == 0.0f)
                {
                    float _1729;
                    float _1731;
                    float _1733;
                    float _1735;
                    if (_1347 == 0u)
                    {
                        float frontier_phi_104_92_ladder;
                        float frontier_phi_104_92_ladder_1;
                        float frontier_phi_104_92_ladder_2;
                        float frontier_phi_104_92_ladder_3;
                        if (_1408)
                        {
                            float _1710 = _1283 * _1277;
                            float _1714 = rsqrt(dot(float3(_1280, _1710, _1286), float3(_1280, _1710, _1286)));
                            float4 _1724 = _28[4u].SampleLevel(_58, float3(_1714 * _1280, _1714 * _1710, _1714 * _1286), 0.0f);
                            frontier_phi_104_92_ladder = 1.0f;
                            frontier_phi_104_92_ladder_1 = _1724.z;
                            frontier_phi_104_92_ladder_2 = _1724.y;
                            frontier_phi_104_92_ladder_3 = _1724.x;
                        }
                        else
                        {
                            frontier_phi_104_92_ladder = 0.0f;
                            frontier_phi_104_92_ladder_1 = 0.0f;
                            frontier_phi_104_92_ladder_2 = 0.0f;
                            frontier_phi_104_92_ladder_3 = 0.0f;
                        }
                        _1729 = frontier_phi_104_92_ladder_3;
                        _1731 = frontier_phi_104_92_ladder_2;
                        _1733 = frontier_phi_104_92_ladder_1;
                        _1735 = frontier_phi_104_92_ladder;
                    }
                    else
                    {
                        float frontier_phi_104_93_ladder;
                        float frontier_phi_104_93_ladder_1;
                        float frontier_phi_104_93_ladder_2;
                        float frontier_phi_104_93_ladder_3;
                        if (_1408)
                        {
                            float _1740 = _1283 * _1277;
                            float _1744 = rsqrt(dot(float3(_1280, _1740, _1286), float3(_1280, _1740, _1286)));
                            float4 _1752 = _28[4u].SampleLevel(_58, float3(_1744 * _1280, _1744 * _1740, _1744 * _1286), 0.0f);
                            float _1754 = _1752.x;
                            float _1755 = _1752.y;
                            float _1756 = _1752.z;
                            frontier_phi_104_93_ladder = 1.0f;
                            frontier_phi_104_93_ladder_1 = _1756 - (_1756 * _1405);
                            frontier_phi_104_93_ladder_2 = _1755 - (_1755 * _1405);
                            frontier_phi_104_93_ladder_3 = _1754 - (_1754 * _1405);
                        }
                        else
                        {
                            frontier_phi_104_93_ladder = _1405;
                            frontier_phi_104_93_ladder_1 = 0.0f;
                            frontier_phi_104_93_ladder_2 = 0.0f;
                            frontier_phi_104_93_ladder_3 = 0.0f;
                        }
                        _1729 = frontier_phi_104_93_ladder_3;
                        _1731 = frontier_phi_104_93_ladder_2;
                        _1733 = frontier_phi_104_93_ladder_1;
                        _1735 = frontier_phi_104_93_ladder;
                    }
                    _1816 = _1729 * _1158;
                    _1818 = _1731 * _1158;
                    _1820 = _1733 * _1158;
                    _1822 = _1735 * _1158;
                }
                else
                {
                    uint4 _1455 = asuint(_54_m0[0u]);
                    float _1764;
                    float _1766;
                    float _1770;
                    float _1773;
                    if (_12.Load(int3(uint2(uint(float(_1455.x) * _758), uint(float(_1455.y) * _756)), 0u)).x > 0.0f)
                    {
                        uint _1498_dummy_parameter;
                        uint2 _1498 = spvTextureSize(_14, 0u, _1498_dummy_parameter);
                        float4 _1507 = _14.Load(int3(uint2(uint(float(_1498.x) * _758), uint(float(_1498.y) * _756)), 0u));
                        float4 _1531 = _13.SampleLevel(_57, float2(((_1507.x * 0.5f) * _49_m0[52u].z) + (_49_m0[52u].x * _758), ((_1507.y * (-0.5f)) * _49_m0[52u].w) + (_49_m0[52u].y * _756)), 0.0f);
                        float _1760;
                        float _1761;
                        float _1762;
                        if (_782 == 0u)
                        {
                            _1760 = _49_m0[54u].x * _1531.x;
                            _1761 = _49_m0[54u].x * _1531.y;
                            _1762 = _49_m0[54u].x * _1531.z;
                        }
                        else
                        {
                            _1760 = 0.0f;
                            _1761 = 0.0f;
                            _1762 = 0.0f;
                        }
                        float frontier_phi_108_106_ladder;
                        float frontier_phi_108_106_ladder_1;
                        float frontier_phi_108_106_ladder_2;
                        float frontier_phi_108_106_ladder_3;
                        for (;;)
                        {
                            if (_780 > 0.0f)
                            {
                                float _1769;
                                float _1772;
                                float _1775;
                                if (_1356)
                                {
                                    _1769 = 0.0f;
                                    _1772 = 0.0f;
                                    _1775 = 0.0f;
                                }
                                else
                                {
                                    if (!((_396 != 4u) || (_788 != 0u)))
                                    {
                                        frontier_phi_108_106_ladder = _1761;
                                        frontier_phi_108_106_ladder_1 = _1407;
                                        frontier_phi_108_106_ladder_2 = _1760;
                                        frontier_phi_108_106_ladder_3 = _1762;
                                        break;
                                    }
                                    _1769 = _1760;
                                    _1772 = _1761;
                                    _1775 = _1762;
                                }
                                frontier_phi_108_106_ladder = _1772;
                                frontier_phi_108_106_ladder_1 = (1.0f - exp2(log2(_780) * 3.0f)) * _1407;
                                frontier_phi_108_106_ladder_2 = _1769;
                                frontier_phi_108_106_ladder_3 = _1775;
                                break;
                            }
                            else
                            {
                                frontier_phi_108_106_ladder = _1761;
                                frontier_phi_108_106_ladder_1 = _1407;
                                frontier_phi_108_106_ladder_2 = _1760;
                                frontier_phi_108_106_ladder_3 = _1762;
                                break;
                            }
                        }
                        _1764 = frontier_phi_108_106_ladder_1;
                        _1766 = frontier_phi_108_106_ladder_2;
                        _1770 = frontier_phi_108_106_ladder;
                        _1773 = frontier_phi_108_106_ladder_3;
                    }
                    else
                    {
                        float frontier_phi_108_95_ladder;
                        float frontier_phi_108_95_ladder_1;
                        float frontier_phi_108_95_ladder_2;
                        float frontier_phi_108_95_ladder_3;
                        if (_1160)
                        {
                            frontier_phi_108_95_ladder = _1768;
                            frontier_phi_108_95_ladder_1 = 0.0f;
                            frontier_phi_108_95_ladder_2 = _1768;
                            frontier_phi_108_95_ladder_3 = _1768;
                        }
                        else
                        {
                            float4 _1781 = _28[7u].SampleLevel(_58, float3(_1280, _1283, _1286), 0.0f);
                            frontier_phi_108_95_ladder = _1781.y;
                            frontier_phi_108_95_ladder_1 = _1407;
                            frontier_phi_108_95_ladder_2 = _1781.x;
                            frontier_phi_108_95_ladder_3 = _1781.z;
                        }
                        _1764 = frontier_phi_108_95_ladder_1;
                        _1766 = frontier_phi_108_95_ladder_2;
                        _1770 = frontier_phi_108_95_ladder;
                        _1773 = frontier_phi_108_95_ladder_3;
                    }
                    float _1870;
                    float _1871;
                    float _1873;
                    float _1875;
                    if (_1347 == 0u)
                    {
                        _1870 = _1764;
                        _1871 = _1766;
                        _1873 = _1770;
                        _1875 = _1773;
                    }
                    else
                    {
                        _1870 = _1405;
                        _1871 = _1766 * _1232;
                        _1873 = _1770 * _1232;
                        _1875 = _1773 * _1232;
                    }
                    float _2029;
                    float _2030;
                    float _2031;
                    float _2032;
                    if (_1408 && (_1870 < 1.0f))
                    {
                        float _2003 = _1283 * _1277;
                        float _2007 = rsqrt(dot(float3(_1280, _2003, _1286), float3(_1280, _2003, _1286)));
                        float4 _2015 = _28[4u].SampleLevel(_58, float3(_2007 * _1280, _2007 * _2003, _2007 * _1286), 0.0f);
                        float _2017 = _2015.x;
                        float _2018 = _2015.y;
                        float _2019 = _2015.z;
                        _2029 = 1.0f;
                        _2030 = ((_1871 - _2017) * _1870) + _2017;
                        _2031 = ((_1873 - _2018) * _1870) + _2018;
                        _2032 = ((_1875 - _2019) * _1870) + _2019;
                    }
                    else
                    {
                        _2029 = _1870;
                        _2030 = _1871;
                        _2031 = _1873;
                        _2032 = _1875;
                    }
                    float _1823 = _2029 * _1158;
                    _1816 = _2030 * _1823;
                    _1818 = _2031 * _1823;
                    _1820 = _2032 * _1823;
                    _1822 = _1823;
                }
                float _1827 = _49_m0[58u].z * _1355;
                float _1849 = max(_49_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _1851 = _1849 * ((_1827 * ((_1351 * 1000.0f) - _1816)) + _1816);
                float _1852 = _1849 * ((_1827 * ((_1352 * 1000.0f) - _1818)) + _1818);
                float _1853 = _1849 * ((_1827 * (_1353 - _1820)) + _1820);
                float _1859 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_1851, max(_1852, _1853)) + 1.0f);
                float _1863 = min(_1859 * _1851, 0.996078431606292724609375f);
                float _1865 = min(_1859 * _1852, 0.996078431606292724609375f);
                float _1866 = min(_1859 * _1853, 0.996078431606292724609375f);
                _35[uint2(_220, _223)] = float4(_1863, _1865, _1866, _1822);
                if (_227)
                {
                    _35[uint2(_220 + 1u, _223)] = float4(_1863, _1865, _1866, _1822);
                }
                if (_230)
                {
                    _35[uint2(_220, _223 + 1u)] = float4(_1863, _1865, _1866, _1822);
                }
                if (_231)
                {
                    _35[uint2(_220 + 1u, _223 + 1u)] = float4(_1863, _1865, _1866, _1822);
                }
                ladder_phi_8 = true;
                break;
            }
            if (ladder_phi_8)
            {
                break;
            }
            _35[uint2(_220, _223)] = 0.0f.xxxx;
            if (_227)
            {
                _35[uint2(_220 + 1u, _223)] = 0.0f.xxxx;
            }
            if (_230)
            {
                _35[uint2(_220, _223 + 1u)] = 0.0f.xxxx;
            }
            if (!_231)
            {
                break;
            }
            _35[uint2(_220 + 1u, _223 + 1u)] = 0.0f.xxxx;
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
