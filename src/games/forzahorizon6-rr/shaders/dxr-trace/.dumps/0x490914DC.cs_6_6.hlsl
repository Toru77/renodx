static float _1680;
static uint _2680;
static float _2681;
static float _2682;
static float _2683;
static float _2684;
static float _2685;
static float _2686;
static uint _2687;
static uint _2688;
static float _2689;
static float _2690;
static float _2691;
static float _2692;
static float _2693;
static float _2699;
static uint _2700;
static float _2701;
static uint _2703;
static uint _2704;

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

static RayQuery<RAY_FLAG_NONE> _2131;

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
                    uint _421;
                    uint _422;
                    if (_367 == 0u)
                    {
                        _421 = (((_375 & 2097152u) != 0u) && (_366 == _388)) ? (_386 | 128u) : _386;
                        _422 = _375;
                    }
                    else
                    {
                        _421 = _349;
                        _422 = _375 | ((_348 << 20u) & 134217728u);
                    }
                    uint _431 = _379 & 512u;
                    float _1087;
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
                    if (_431 == 0u)
                    {
                        if (!((_422 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            break;
                        }
                        float _806 = asfloat(_17.Load((_383 * 115u) + 33u).x);
                        uint4 _814 = _24[2u].Load(int3(uint2(_264, _265), 0u));
                        uint _816 = _814.y;
                        uint _817 = _421 & 128u;
                        uint _1075;
                        uint _1076;
                        uint _1077;
                        uint _1078;
                        if (_817 == 0u)
                        {
                            _1075 = uint(((_422 & 817889384u) | (_379 & 576u)) != 0u) | (((_422 >> 19u) & 1u) ^ 1u);
                            _1076 = uint(((_422 & 17825808u) | (_379 & 520u)) != 0u);
                            _1077 = uint(((_422 & 46137344u) | (_379 & 2564u)) != 0u);
                            _1078 = 0u;
                        }
                        else
                        {
                            _1075 = 1u;
                            _1076 = _421 & 1u;
                            _1077 = 1u;
                            _1078 = 1u;
                        }
                        precise float _1082 = float(_816 & 127u) * 0.0078740157186985015869140625f;
                        bool _1086 = (_422 & 4194304u) == 0u;
                        float _1196;
                        if (_1086)
                        {
                            _1196 = _1082;
                        }
                        else
                        {
                            _1196 = float(_816 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1346;
                        if ((_422 & 134217728u) == 0u)
                        {
                            uint frontier_phi_72_56_ladder;
                            if ((_817 != 0u) || ((_422 & 17825792u) == 1048576u))
                            {
                                frontier_phi_72_56_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_72_56_ladder = _1076;
                            }
                            _1346 = frontier_phi_72_56_ladder;
                        }
                        else
                        {
                            _1346 = _1076;
                        }
                        uint4 _1349 = _24[1u].Load(int3(uint2(_264, _265), 0u));
                        uint _1351 = _1349.x;
                        float _1570;
                        float _1572;
                        float _1574;
                        if (_1075 == 0u)
                        {
                            _1570 = 0.0f;
                            _1572 = 0.0f;
                            _1574 = 0.0f;
                        }
                        else
                        {
                            float4 _1579 = _20[8u].Load(int3(uint2(_264, _265), 0u));
                            _1570 = _1579.x;
                            _1572 = _1579.y;
                            _1574 = _1579.z;
                        }
                        uint _1719;
                        if (_1346 == 0u)
                        {
                            _1719 = 0u;
                        }
                        else
                        {
                            _1719 = _24[9u].Load(int3(uint2(_264, _265), 0u)).x;
                        }
                        uint _1773;
                        if (_1077 == 0u)
                        {
                            _1773 = 0u;
                        }
                        else
                        {
                            _1773 = _24[10u].Load(int3(uint2(_264, _265), 0u)).x;
                        }
                        float _1783 = (float((_1351 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1784 = (float(_1351 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1788 = (1.0f - abs(_1783)) - abs(_1784);
                        float _1790 = clamp((-0.0f) - _1788, 0.0f, 1.0f);
                        float _1791 = (-0.0f) - _1790;
                        float _1796 = ((_1783 >= 0.0f) ? _1791 : _1790) + _1783;
                        float _1797 = ((_1784 >= 0.0f) ? _1791 : _1790) + _1784;
                        float _1801 = rsqrt(dot(float3(_1796, _1797, _1788), float3(_1796, _1797, _1788)));
                        float _1802 = _1796 * _1801;
                        float _1803 = _1797 * _1801;
                        float _1804 = _1801 * _1788;
                        float _1088 = float(_1351 & 255u);
                        float _1808 = ((_422 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1958;
                        float _1959;
                        float _1960;
                        float _1961;
                        uint _1962;
                        if ((_379 & 64u) == 0u)
                        {
                            float frontier_phi_133_126_ladder;
                            float frontier_phi_133_126_ladder_1;
                            float frontier_phi_133_126_ladder_2;
                            float frontier_phi_133_126_ladder_3;
                            uint frontier_phi_133_126_ladder_4;
                            if ((_422 & 276824064u) == 0u)
                            {
                                frontier_phi_133_126_ladder = 0.0f;
                                frontier_phi_133_126_ladder_1 = ((_422 & 8u) != 0u) ? _1572 : _1808;
                                frontier_phi_133_126_ladder_2 = 0.0f;
                                frontier_phi_133_126_ladder_3 = 0.0f;
                                frontier_phi_133_126_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_133_126_ladder = 0.0f;
                                frontier_phi_133_126_ladder_1 = _1808;
                                frontier_phi_133_126_ladder_2 = 0.0f;
                                frontier_phi_133_126_ladder_3 = 0.0f;
                                frontier_phi_133_126_ladder_4 = 0u;
                            }
                            _1958 = frontier_phi_133_126_ladder_1;
                            _1959 = frontier_phi_133_126_ladder_2;
                            _1960 = frontier_phi_133_126_ladder;
                            _1961 = frontier_phi_133_126_ladder_3;
                            _1962 = frontier_phi_133_126_ladder_4;
                        }
                        else
                        {
                            float _1910 = (_1572 * 2.0f) + (-1.0f);
                            float _1911 = (_1574 * 2.0f) + (-1.0f);
                            float _1915 = (1.0f - abs(_1910)) - abs(_1911);
                            float _1917 = clamp((-0.0f) - _1915, 0.0f, 1.0f);
                            float _1918 = (-0.0f) - _1917;
                            float _1923 = ((_1910 >= 0.0f) ? _1918 : _1917) + _1910;
                            float _1924 = ((_1911 >= 0.0f) ? _1918 : _1917) + _1911;
                            float _1928 = rsqrt(dot(float3(_1923, _1924, _1915), float3(_1923, _1924, _1915)));
                            _1958 = floor(round(_1570 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1959 = _1923 * _1928;
                            _1960 = _1924 * _1928;
                            _1961 = _1928 * _1915;
                            _1962 = 1u;
                        }
                        float _1096;
                        if ((_422 & 32768u) == 0u)
                        {
                            _1096 = _1958;
                        }
                        else
                        {
                            float frontier_phi_137_138_ladder;
                            if (_17.Load((_383 * 115u) + 36u).x == 0u)
                            {
                                float _2099 = clamp((_1088 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _806;
                                frontier_phi_137_138_ladder = ((_422 & 131072u) != 0u) ? _2099 : ((((clamp((1.21000003814697265625f / (exp2((_1196 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_383 * 115u) + 32u).x)) + 1.0f) * _2099);
                            }
                            else
                            {
                                frontier_phi_137_138_ladder = _806;
                            }
                            _1096 = frontier_phi_137_138_ladder;
                        }
                        uint _2010 = _421 & 1u;
                        float _2042;
                        float _2044;
                        float _2046;
                        uint _2048;
                        if (((_422 & 16u) == 0u) || (((_2010 | (_379 & 8u)) | (_422 & 16777216u)) != 0u))
                        {
                            _2042 = _1959;
                            _2044 = _1960;
                            _2046 = _1961;
                            _2048 = _1962;
                        }
                        else
                        {
                            float _2058 = (float(_1719 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2059 = (float(_1719 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _2063 = (1.0f - abs(_2058)) - abs(_2059);
                            float _2065 = clamp((-0.0f) - _2063, 0.0f, 1.0f);
                            float _2066 = (-0.0f) - _2065;
                            float _2071 = ((_2058 >= 0.0f) ? _2066 : _2065) + _2058;
                            float _2072 = ((_2059 >= 0.0f) ? _2066 : _2065) + _2059;
                            float _2076 = rsqrt(dot(float3(_2071, _2072, _2063), float3(_2071, _2072, _2063)));
                            _2042 = _2071 * _2076;
                            _2044 = _2072 * _2076;
                            _2046 = _2076 * _2063;
                            _2048 = 1u;
                        }
                        float _1090;
                        float _1092;
                        float _1094;
                        if (_2010 == 0u)
                        {
                            float frontier_phi_149_148_ladder;
                            float frontier_phi_149_148_ladder_1;
                            float frontier_phi_149_148_ladder_2;
                            if (((_421 & 64u) == 0u) && (_1078 != 0u))
                            {
                                float2 _2195 = spvUnpackHalf2x16((_1773 >> 17u) & 32736u);
                                float _2196 = _2195.x;
                                float _2199 = (spvUnpackHalf2x16((_1773 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2200 = (spvUnpackHalf2x16((_1773 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2204 = (1.0f - abs(_2199)) - abs(_2200);
                                float _2206 = clamp((-0.0f) - _2204, 0.0f, 1.0f);
                                float _2207 = (-0.0f) - _2206;
                                float _2212 = ((_2199 >= 0.0f) ? _2207 : _2206) + _2199;
                                float _2213 = ((_2200 >= 0.0f) ? _2207 : _2206) + _2200;
                                float _2217 = rsqrt(dot(float3(_2212, _2213, _2204), float3(_2212, _2213, _2204)));
                                float _2227 = (((_2212 * _2217) - _1802) * _2196) + _1802;
                                float _2228 = (((_2213 * _2217) - _1803) * _2196) + _1803;
                                float _2229 = (((_2217 * _2204) - _1804) * _2196) + _1804;
                                float _2233 = rsqrt(dot(float3(_2227, _2228, _2229), float3(_2227, _2228, _2229)));
                                frontier_phi_149_148_ladder = _2227 * _2233;
                                frontier_phi_149_148_ladder_1 = _2228 * _2233;
                                frontier_phi_149_148_ladder_2 = _2229 * _2233;
                            }
                            else
                            {
                                frontier_phi_149_148_ladder = _1802;
                                frontier_phi_149_148_ladder_1 = _1803;
                                frontier_phi_149_148_ladder_2 = _1804;
                            }
                            _1090 = frontier_phi_149_148_ladder;
                            _1092 = frontier_phi_149_148_ladder_1;
                            _1094 = frontier_phi_149_148_ladder_2;
                        }
                        else
                        {
                            _1090 = _1802;
                            _1092 = _1803;
                            _1094 = _1804;
                        }
                        float _1104;
                        float _1106;
                        float _1108;
                        float _1110;
                        if (_1086)
                        {
                            float frontier_phi_154_153_ladder;
                            float frontier_phi_154_153_ladder_1;
                            float frontier_phi_154_153_ladder_2;
                            float frontier_phi_154_153_ladder_3;
                            if (((_422 & 33554432u) == 0u) || (((_379 & 4u) != 0u) && ((_422 & 8388608u) == 0u)))
                            {
                                frontier_phi_154_153_ladder = 0.0f;
                                frontier_phi_154_153_ladder_1 = 0.0f;
                                frontier_phi_154_153_ladder_2 = 0.0f;
                                frontier_phi_154_153_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2265 = (spvUnpackHalf2x16((_1773 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2266 = (spvUnpackHalf2x16((_1773 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2270 = (1.0f - abs(_2265)) - abs(_2266);
                                float _2272 = clamp((-0.0f) - _2270, 0.0f, 1.0f);
                                float _2273 = (-0.0f) - _2272;
                                float _2278 = ((_2265 >= 0.0f) ? _2273 : _2272) + _2265;
                                float _2279 = ((_2266 >= 0.0f) ? _2273 : _2272) + _2266;
                                float _2283 = rsqrt(dot(float3(_2278, _2279, _2270), float3(_2278, _2279, _2270)));
                                float _2284 = _2278 * _2283;
                                float _2285 = _2279 * _2283;
                                float _2286 = _2283 * _2270;
                                float _2290 = rsqrt(dot(float3(_2284, _2285, _2286), float3(_2284, _2285, _2286)));
                                frontier_phi_154_153_ladder = _2290 * _2286;
                                frontier_phi_154_153_ladder_1 = _2290 * _2285;
                                frontier_phi_154_153_ladder_2 = _2290 * _2284;
                                frontier_phi_154_153_ladder_3 = spvUnpackHalf2x16((_1773 >> 17u) & 32736u).x;
                            }
                            _1104 = frontier_phi_154_153_ladder_3;
                            _1106 = frontier_phi_154_153_ladder_2;
                            _1108 = frontier_phi_154_153_ladder_1;
                            _1110 = frontier_phi_154_153_ladder;
                        }
                        else
                        {
                            _1104 = 0.0f;
                            _1106 = 0.0f;
                            _1108 = 0.0f;
                            _1110 = 0.0f;
                        }
                        bool _2247 = _2048 != 0u;
                        _1087 = _1088;
                        _1089 = _1090;
                        _1091 = _1092;
                        _1093 = _1094;
                        _1095 = _1096;
                        _1097 = _2247 ? _2042 : _1090;
                        _1099 = _2247 ? _2044 : _1092;
                        _1101 = _2247 ? _2046 : _1094;
                        _1103 = _1104;
                        _1105 = _1106;
                        _1107 = _1108;
                        _1109 = _1110;
                    }
                    else
                    {
                        uint4 _633 = _24[1u].Load(int3(uint2(_264, _265), 0u));
                        uint _635 = _633.x;
                        uint4 _639 = _24[9u].Load(int3(uint2(_264, _265), 0u));
                        uint _641 = _639.x;
                        float _991;
                        float _992;
                        float _993;
                        if ((_422 & 33554432u) == 0u)
                        {
                            float _826 = (float((_635 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _827 = (float(_635 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _831 = (1.0f - abs(_826)) - abs(_827);
                            float _833 = clamp((-0.0f) - _831, 0.0f, 1.0f);
                            float _834 = (-0.0f) - _833;
                            float _839 = ((_826 >= 0.0f) ? _834 : _833) + _826;
                            float _840 = ((_827 >= 0.0f) ? _834 : _833) + _827;
                            float _844 = rsqrt(dot(float3(_839, _840, _831), float3(_839, _840, _831)));
                            _991 = _839 * _844;
                            _992 = _840 * _844;
                            _993 = _844 * _831;
                        }
                        else
                        {
                            float _855 = (float((_641 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _856 = (float(_641 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _860 = (1.0f - abs(_855)) - abs(_856);
                            float _862 = clamp((-0.0f) - _860, 0.0f, 1.0f);
                            float _863 = (-0.0f) - _862;
                            float _868 = ((_855 >= 0.0f) ? _863 : _862) + _855;
                            float _869 = ((_856 >= 0.0f) ? _863 : _862) + _856;
                            float _873 = rsqrt(dot(float3(_868, _869, _860), float3(_868, _869, _860)));
                            _991 = _868 * _873;
                            _992 = _869 * _873;
                            _993 = _873 * _860;
                        }
                        _1087 = float(_635 & 255u);
                        _1089 = _991;
                        _1091 = _992;
                        _1093 = _993;
                        _1095 = 1.0f;
                        _1097 = _991;
                        _1099 = _992;
                        _1101 = _993;
                        _1103 = 0.0f;
                        _1105 = 0.0f;
                        _1107 = 0.0f;
                        _1109 = 0.0f;
                    }
                    precise float _1111 = _1087 * 0.0039215688593685626983642578125f;
                    if ((_421 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        break;
                    }
                    bool _1205 = ((_421 & 128u) | _431) != 0u;
                    uint _1258;
                    if (_1205)
                    {
                        _1258 = 1u;
                    }
                    else
                    {
                        _1258 = (((_422 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _391;
                    float _393;
                    float _395;
                    uint _397;
                    float _1359;
                    if (((_422 & 33554432u) == 0u) || _1205)
                    {
                        bool _1353 = _76 != 0u;
                        uint _1361;
                        if ((_422 & 16u) == 0u)
                        {
                            _1361 = _1258;
                        }
                        else
                        {
                            _1361 = ((_422 & 268435456u) != 0u) ? _1258 : 2u;
                        }
                        _1359 = _1095 * _1111;
                        _391 = _1353 ? _1097 : _1089;
                        _393 = _1353 ? _1099 : _1091;
                        _395 = _1353 ? _1101 : _1093;
                        _397 = _1361;
                    }
                    else
                    {
                        _1359 = _1103;
                        _391 = _1105;
                        _393 = _1107;
                        _395 = _1109;
                        _397 = _1258;
                    }
                    uint _1362 = _397 + 102u;
                    float _1371 = clamp((_1359 - _44_m0[_1362].x) / (_44_m0[_1362].y - _44_m0[_1362].x), 0.0f, 1.0f);
                    _390 = _391;
                    _392 = _393;
                    _394 = _395;
                    _396 = _397;
                    _398 = (_1371 * _1371) * (3.0f - (_1371 * 2.0f));
                    _400 = _44_m0[_1362].z;
                    _402 = (_397 == 1u) ? _388 : 0u;
                    _404 = asfloat(_17.Load((_383 * 115u) + 114u).x);
                }
                if (_398 == 0.0f)
                {
                    ladder_phi_8 = false;
                    break;
                }
                float _438;
                if (_262)
                {
                    _438 = _261;
                }
                else
                {
                    _438 = _12.Load(int3(uint2(uint(int(_253 * float(_232))), uint(int(_254 * float(_233)))), 0u)).x;
                }
                float _447 = _49_m0[50u].w + _49_m0[50u].y;
                uint _450 = _396 + 63u;
                float _459 = clamp(((_49_m0[50u].x / (_447 - (_49_m0[50u].y * _438))) - _49_m0[_450].y) / (_49_m0[_450].x - _49_m0[_450].y), 0.0f, 1.0f);
                float _464 = ((_459 * _459) * _398) * (3.0f - (_459 * 2.0f));
                float _475 = ((_253 * 2.0f) * _49_m0[51u].z) + (-1.0f);
                float _476 = ((1.0f - (_49_m0[51u].w * _254)) * 2.0f) + (-1.0f);
                float _492 = mad(_143, _438, mad(_136, _476, _475 * _129)) + _150;
                float _493 = (mad(_140, _438, mad(_133, _476, _475 * _126)) + _147) / _492;
                float _494 = (mad(_141, _438, mad(_134, _476, _475 * _127)) + _148) / _492;
                float _495 = (mad(_142, _438, mad(_135, _476, _475 * _128)) + _149) / _492;
                float _499 = rsqrt(dot(float3(_493, _494, _495), float3(_493, _494, _495)));
                float _500 = _499 * _493;
                float _501 = _499 * _494;
                float _502 = _499 * _495;
                float _505 = mad(_92, _394, mad(_86, _392, _390 * _80));
                float _508 = mad(_93, _394, mad(_87, _392, _390 * _81));
                float _511 = mad(_94, _394, mad(_88, _392, _390 * _82));
                float _515 = dot(float3(_500, _501, _502), float3(_505, _508, _511)) * 2.0f;
                float _519 = _500 - (_515 * _505);
                float _520 = _501 - (_515 * _508);
                float _521 = _502 - (_515 * _511);
                bool _525 = dot(float3(_519, _520, _521), float3(_500, _501, _502)) < 0.0f;
                float _532 = sqrt(((_494 * _494) + (_493 * _493)) + (_495 * _495)) * 0.001000000047497451305389404296875f;
                float _543 = ((_532 * _505) + _493) + (_519 * _400);
                float _544 = ((_532 * _508) + _494) + (_520 * _400);
                float _545 = ((_532 * _511) + _495) + (_521 * _400);
                float _561 = mad(_115, _545, mad(_108, _544, _543 * _101)) + _122;
                float _564 = (mad(_114, _545, mad(_107, _544, _543 * _100)) + _121) / _561;
                float _567 = (((mad(_112, _545, mad(_105, _544, _543 * _98)) + _119) / _561) * 0.5f) + 0.5f;
                float _568 = 0.5f - (((mad(_113, _545, mad(_106, _544, _543 * _99)) + _120) / _561) * 0.5f);
                float _571 = _567 * _49_m0[51u].x;
                float _572 = _568 * _49_m0[51u].y;
                float _577 = _543 + (_519 * 0.100000001490116119384765625f);
                float _578 = _544 + (_520 * 0.100000001490116119384765625f);
                float _579 = _545 + (_521 * 0.100000001490116119384765625f);
                float _595 = mad(_115, _579, mad(_108, _578, _577 * _101)) + _122;
                float _604 = _49_m0[51u].x * (((((mad(_112, _579, mad(_105, _578, _577 * _98)) + _119) / _595) * 0.5f) + 0.5f) - _567);
                float _606 = _49_m0[51u].y * ((0.5f - (((mad(_113, _579, mad(_106, _578, _577 * _99)) + _120) / _595) * 0.5f)) - _568);
                float _607 = ((mad(_114, _579, mad(_107, _578, _577 * _100)) + _121) / _595) - _564;
                float _608 = _604 * 10.0f;
                float _610 = _606 * 10.0f;
                float _611 = _607 * 10.0f;
                bool _618 = _396 == 1u;
                uint _649;
                uint _651;
                float _653;
                float _655;
                float _657;
                float _659;
                float _661;
                float _663;
                uint _665;
                uint _667;
                float _669;
                float _671;
                float _673;
                if (_618 && (asuint(_49_m0[62u]).w != 0u))
                {
                    _649 = 0u;
                    _651 = 1u;
                    _653 = 0.0f;
                    _655 = 0.0f;
                    _657 = 1.0f;
                    _659 = 0.0f;
                    _661 = 0.0f;
                    _663 = 0.0f;
                    _665 = 0u;
                    _667 = 0u;
                    _669 = 0.0f;
                    _671 = 0.0f;
                    _673 = 0.0f;
                }
                else
                {
                    float _734 = float(_232);
                    float _735 = float(_233);
                    float _742 = (_608 != 0.0f) ? (0.100000001490116119384765625f / _604) : 3.4028234663852885981170418348452e+38f;
                    float _744 = (_610 != 0.0f) ? (0.100000001490116119384765625f / _606) : 3.4028234663852885981170418348452e+38f;
                    float _745 = (_611 != 0.0f) ? (0.100000001490116119384765625f / _607) : 3.4028234663852885981170418348452e+38f;
                    float _746 = 1.0f / _734;
                    float _747 = 1.0f / _735;
                    float _748 = 0.004999999888241291046142578125f / _734;
                    float _750 = 0.004999999888241291046142578125f / _735;
                    float _759 = float(_608 >= 0.0f);
                    float _760 = float(_610 >= 0.0f);
                    float _769 = ((_608 < 0.0f) ? ((-0.0f) - _748) : _748) - _571;
                    float _772 = ((_610 < 0.0f) ? ((-0.0f) - _750) : _750) - _572;
                    float _775 = min((((floor(_571 * _734) + _759) * _746) + _769) * _742, (((floor(_572 * _735) + _760) * _747) + _772) * _744);
                    float _779 = (_775 * _608) + _571;
                    float _780 = (_775 * _610) + _572;
                    float _781 = (_775 * _611) + _564;
                    float _784 = _49_m0[50u].x / (_447 - (_781 * _49_m0[50u].y));
                    float _795 = max(0.300000011920928955078125f, 10.0f / max(0.00999999977648258209228515625f, max(abs(_604 * 38400.0f), abs(_606 * 21600.0f))));
                    float _670;
                    float _672;
                    float _674;
                    uint _935;
                    uint _950;
                    uint _952;
                    uint _652;
                    float _654;
                    float _656;
                    float _658;
                    float _660;
                    float _662;
                    float _664;
                    float _940;
                    float _942;
                    float _944;
                    float _946;
                    float _948;
                    uint _934 = 0u;
                    float _936 = _781;
                    float _937 = _780;
                    float _938 = _779;
                    float _939 = _775;
                    float _941 = _747;
                    float _943 = _746;
                    float _945 = _735;
                    float _947 = _734;
                    uint _949 = 0u;
                    uint _951 = 0u;
                    float _953 = _781;
                    float _954 = _780;
                    float _955 = _779;
                    float _956 = 1.0f;
                    float _957 = 0.0f;
                    float _958 = 0.0f;
                    uint _959 = 1u;
                    float _960;
                    float _961;
                    uint _962;
                    uint _963;
                    bool _964;
                    for (;;)
                    {
                        _960 = _947 * _938;
                        _961 = _945 * _937;
                        _962 = uint(int(_960));
                        _963 = uint(int(_961));
                        _964 = _949 == 0u;
                        float _1152;
                        if (_964)
                        {
                            _1152 = _12.Load(int3(uint2(_962, _963), 0u)).x;
                        }
                        else
                        {
                            _1152 = _15.Load(int3(uint2(_962, _963), _949 + 4294967295u)).x;
                        }
                        float _1158 = ((_960 >= floor(_947)) || (_961 >= floor(_945))) ? 1.0f : _1152;
                        float _1172 = (_611 < 0.0f) ? ((_1158 - _564) * _745) : 3.4028234663852885981170418348452e+38f;
                        float _1174 = min(min((((floor(_960) + _759) * _943) + _769) * _742, (((floor(_961) + _760) * _941) + _772) * _744), _1172);
                        bool _1175 = _1158 < _936;
                        bool _1179 = _1175 && (asuint(_1174) != asuint(_1172));
                        float _1180 = _1175 ? _1174 : _939;
                        float _1184 = (_1180 * _608) + _571;
                        float _1185 = (_1180 * _610) + _572;
                        float _1186 = (_1180 * _611) + _564;
                        uint _1188 = (_1179 ? 1u : 4294967295u) + _949;
                        float _1189 = _1179 ? 0.5f : 2.0f;
                        float _1190 = _1189 * _947;
                        float _1191 = _1189 * _945;
                        float _1192 = _1179 ? 2.0f : 0.5f;
                        float _1193 = _1192 * _943;
                        float _1194 = _1192 * _941;
                        _935 = _934 + 1u;
                        uint _1338;
                        uint _1340;
                        if (int(_1188) < int(0u))
                        {
                            float _1236 = _49_m0[50u].w + _49_m0[50u].y;
                            float _1238 = _49_m0[50u].x / (_1236 - (_49_m0[50u].y * _1158));
                            float _1241 = _49_m0[50u].x / (_1236 - (_49_m0[50u].y * _1186));
                            float _1243 = abs(_784 - _1241);
                            float _1246 = _1241 - _1238;
                            float frontier_phi_71_54_ladder;
                            float frontier_phi_71_54_ladder_1;
                            uint frontier_phi_71_54_ladder_2;
                            float frontier_phi_71_54_ladder_3;
                            float frontier_phi_71_54_ladder_4;
                            float frontier_phi_71_54_ladder_5;
                            float frontier_phi_71_54_ladder_6;
                            float frontier_phi_71_54_ladder_7;
                            uint frontier_phi_71_54_ladder_8;
                            uint frontier_phi_71_54_ladder_9;
                            float frontier_phi_71_54_ladder_10;
                            float frontier_phi_71_54_ladder_11;
                            float frontier_phi_71_54_ladder_12;
                            float frontier_phi_71_54_ladder_13;
                            if (_1246 > max(0.00999999977648258209228515625f, _1243 * 0.00999999977648258209228515625f))
                            {
                                bool _1318 = _951 != 0u;
                                frontier_phi_71_54_ladder = _1180 + _795;
                                frontier_phi_71_54_ladder_1 = _747;
                                frontier_phi_71_54_ladder_2 = _959;
                                frontier_phi_71_54_ladder_3 = _1318 ? _958 : _1184;
                                frontier_phi_71_54_ladder_4 = _1318 ? _957 : _1185;
                                frontier_phi_71_54_ladder_5 = _956;
                                frontier_phi_71_54_ladder_6 = _955;
                                frontier_phi_71_54_ladder_7 = _954;
                                frontier_phi_71_54_ladder_8 = (_618 || (asuint(_49_m0[62u]).z == 0u)) ? 1u : _951;
                                frontier_phi_71_54_ladder_9 = 0u;
                                frontier_phi_71_54_ladder_10 = _734;
                                frontier_phi_71_54_ladder_11 = _735;
                                frontier_phi_71_54_ladder_12 = _746;
                                frontier_phi_71_54_ladder_13 = _953;
                            }
                            else
                            {
                                float _1331 = max(0.100000001490116119384765625f, _1243 * 0.100000001490116119384765625f) * 0.5f;
                                float _1334 = clamp((abs(_1246) - _1331) / _1331, 0.0f, 1.0f);
                                uint _1336 = uint(_1238 < _784);
                                float frontier_phi_71_54_ladder_70_ladder;
                                float frontier_phi_71_54_ladder_70_ladder_1;
                                uint frontier_phi_71_54_ladder_70_ladder_2;
                                float frontier_phi_71_54_ladder_70_ladder_3;
                                float frontier_phi_71_54_ladder_70_ladder_4;
                                float frontier_phi_71_54_ladder_70_ladder_5;
                                float frontier_phi_71_54_ladder_70_ladder_6;
                                float frontier_phi_71_54_ladder_70_ladder_7;
                                uint frontier_phi_71_54_ladder_70_ladder_8;
                                uint frontier_phi_71_54_ladder_70_ladder_9;
                                float frontier_phi_71_54_ladder_70_ladder_10;
                                float frontier_phi_71_54_ladder_70_ladder_11;
                                float frontier_phi_71_54_ladder_70_ladder_12;
                                float frontier_phi_71_54_ladder_70_ladder_13;
                                if (_951 == 0u)
                                {
                                    frontier_phi_71_54_ladder_70_ladder = _1180;
                                    frontier_phi_71_54_ladder_70_ladder_1 = _1194;
                                    frontier_phi_71_54_ladder_70_ladder_2 = _1336;
                                    frontier_phi_71_54_ladder_70_ladder_3 = _958;
                                    frontier_phi_71_54_ladder_70_ladder_4 = _957;
                                    frontier_phi_71_54_ladder_70_ladder_5 = _1334;
                                    frontier_phi_71_54_ladder_70_ladder_6 = _955;
                                    frontier_phi_71_54_ladder_70_ladder_7 = _954;
                                    frontier_phi_71_54_ladder_70_ladder_8 = uint(_1334 > 0.0f);
                                    frontier_phi_71_54_ladder_70_ladder_9 = _1188;
                                    frontier_phi_71_54_ladder_70_ladder_10 = _1190;
                                    frontier_phi_71_54_ladder_70_ladder_11 = _1191;
                                    frontier_phi_71_54_ladder_70_ladder_12 = _1193;
                                    frontier_phi_71_54_ladder_70_ladder_13 = _953;
                                }
                                else
                                {
                                    frontier_phi_71_54_ladder_70_ladder = _1180;
                                    frontier_phi_71_54_ladder_70_ladder_1 = _1194;
                                    frontier_phi_71_54_ladder_70_ladder_2 = _1336;
                                    frontier_phi_71_54_ladder_70_ladder_3 = _958;
                                    frontier_phi_71_54_ladder_70_ladder_4 = _957;
                                    frontier_phi_71_54_ladder_70_ladder_5 = _1334;
                                    frontier_phi_71_54_ladder_70_ladder_6 = _955;
                                    frontier_phi_71_54_ladder_70_ladder_7 = _954;
                                    frontier_phi_71_54_ladder_70_ladder_8 = _951;
                                    frontier_phi_71_54_ladder_70_ladder_9 = _1188;
                                    frontier_phi_71_54_ladder_70_ladder_10 = _1190;
                                    frontier_phi_71_54_ladder_70_ladder_11 = _1191;
                                    frontier_phi_71_54_ladder_70_ladder_12 = _1193;
                                    frontier_phi_71_54_ladder_70_ladder_13 = _953;
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
                            _652 = frontier_phi_71_54_ladder_2;
                            _654 = frontier_phi_71_54_ladder_3;
                            _656 = frontier_phi_71_54_ladder_4;
                            _658 = frontier_phi_71_54_ladder_5;
                            _660 = frontier_phi_71_54_ladder_6;
                            _662 = frontier_phi_71_54_ladder_7;
                            _664 = frontier_phi_71_54_ladder_13;
                            _1338 = frontier_phi_71_54_ladder_8;
                            _1340 = frontier_phi_71_54_ladder_9;
                            _948 = frontier_phi_71_54_ladder_10;
                            _946 = frontier_phi_71_54_ladder_11;
                            _944 = frontier_phi_71_54_ladder_12;
                            _942 = frontier_phi_71_54_ladder_1;
                            _940 = frontier_phi_71_54_ladder;
                        }
                        else
                        {
                            bool _1248 = _951 != 0u;
                            _652 = _959;
                            _654 = _958;
                            _656 = _957;
                            _658 = _956;
                            _660 = _1248 ? _955 : _1184;
                            _662 = _1248 ? _954 : _1185;
                            _664 = _1248 ? _953 : _1186;
                            _1338 = _951;
                            _1340 = _1188;
                            _948 = _1190;
                            _946 = _1191;
                            _944 = _1193;
                            _942 = _1194;
                            _940 = _1180;
                        }
                        float frontier_phi_105_pred;
                        uint frontier_phi_105_pred_1;
                        uint frontier_phi_105_pred_2;
                        float frontier_phi_105_pred_3;
                        float frontier_phi_105_pred_4;
                        bool _1343;
                        bool _1345;
                        for (;;)
                        {
                            _1343 = _1186 < 0.0f;
                            _1345 = _1343 || ((_1184 < 0.0f) || (_1185 < 0.0f));
                            if (!_1345)
                            {
                                if (!((_1186 > 1.0f) || ((_1184 > _49_m0[51u].x) || (_1185 > _49_m0[51u].y))))
                                {
                                    frontier_phi_105_pred = _1184;
                                    frontier_phi_105_pred_1 = _1338;
                                    frontier_phi_105_pred_2 = _1340;
                                    frontier_phi_105_pred_3 = _1185;
                                    frontier_phi_105_pred_4 = _1186;
                                    break;
                                }
                            }
                            if (!_1343)
                            {
                                frontier_phi_105_pred = _1184;
                                frontier_phi_105_pred_1 = 1u;
                                frontier_phi_105_pred_2 = 4294967295u;
                                frontier_phi_105_pred_3 = _1185;
                                frontier_phi_105_pred_4 = _1186;
                                break;
                            }
                            float _1709 = (-0.0f) - _1186;
                            float _1710 = _1709 / _611;
                            frontier_phi_105_pred = (_1710 * _608) + _1184;
                            frontier_phi_105_pred_1 = 1u;
                            frontier_phi_105_pred_2 = 4294967295u;
                            frontier_phi_105_pred_3 = (_1710 * _610) + _1185;
                            frontier_phi_105_pred_4 = _1709 + _1186;
                            break;
                        }
                        _670 = frontier_phi_105_pred;
                        _952 = frontier_phi_105_pred_1;
                        _950 = frontier_phi_105_pred_2;
                        _672 = frontier_phi_105_pred_3;
                        _674 = frontier_phi_105_pred_4;
                        if ((_935 < 128u) && (int(_950) > int(4294967295u)))
                        {
                            _934 = _935;
                            _936 = _674;
                            _937 = _672;
                            _938 = _670;
                            _939 = _940;
                            _941 = _942;
                            _943 = _944;
                            _945 = _946;
                            _947 = _948;
                            _949 = _950;
                            _951 = _952;
                            _953 = _664;
                            _954 = _662;
                            _955 = _660;
                            _956 = _658;
                            _957 = _656;
                            _958 = _654;
                            _959 = _652;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    bool _1770 = _935 > 127u;
                    _649 = uint(_1770);
                    _651 = _652;
                    _653 = _654;
                    _655 = _656;
                    _657 = _658;
                    _659 = _660;
                    _661 = _662;
                    _663 = _664;
                    _665 = _1770 ? 1u : _952;
                    _667 = uint(_935 < 129u);
                    _669 = _670;
                    _671 = _672;
                    _673 = _674;
                }
                float _681 = _49_m0[51u].z * 2.0f;
                float _684 = (_681 * _571) + (-1.0f);
                float _685 = ((1.0f - (_49_m0[51u].w * _572)) * 2.0f) + (-1.0f);
                float _701 = mad(_189, _564, mad(_182, _685, _684 * _175)) + _196;
                float _702 = (mad(_186, _564, mad(_179, _685, _684 * _172)) + _193) / _701;
                float _703 = (mad(_187, _564, mad(_180, _685, _684 * _173)) + _194) / _701;
                float _704 = (mad(_188, _564, mad(_181, _685, _684 * _174)) + _195) / _701;
                float _709 = (_681 * _669) + (-1.0f);
                float _710 = ((1.0f - (_49_m0[51u].w * _671)) * 2.0f) + (-1.0f);
                float _726 = mad(_189, _673, mad(_182, _710, _709 * _175)) + _196;
                float _730 = ((mad(_186, _673, mad(_179, _710, _709 * _172)) + _193) / _726) - _702;
                float _731 = ((mad(_187, _673, mad(_180, _710, _709 * _173)) + _194) / _726) - _703;
                float _732 = ((mad(_188, _673, mad(_181, _710, _709 * _174)) + _195) / _726) - _704;
                float _882;
                uint _884;
                float _886;
                if (_667 == 0u)
                {
                    _882 = 0.0f;
                    _884 = _665;
                    _886 = 0.0f;
                }
                else
                {
                    float _929 = float(_232);
                    float _930 = float(_233);
                    float frontier_phi_25_26_ladder;
                    uint frontier_phi_25_26_ladder_1;
                    float frontier_phi_25_26_ladder_2;
                    if ((_669 < 0.0f) || (_671 < 0.0f))
                    {
                        frontier_phi_25_26_ladder = 0.0f;
                        frontier_phi_25_26_ladder_1 = _665;
                        frontier_phi_25_26_ladder_2 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_25_26_ladder_33_ladder;
                        uint frontier_phi_25_26_ladder_33_ladder_1;
                        float frontier_phi_25_26_ladder_33_ladder_2;
                        if ((_673 >= 1.0f) || ((_669 > _49_m0[51u].x) || (_671 > _49_m0[51u].y)))
                        {
                            frontier_phi_25_26_ladder_33_ladder = 0.0f;
                            frontier_phi_25_26_ladder_33_ladder_1 = _665;
                            frontier_phi_25_26_ladder_33_ladder_2 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_25_26_ladder_33_ladder_42_ladder;
                            uint frontier_phi_25_26_ladder_33_ladder_42_ladder_1;
                            float frontier_phi_25_26_ladder_33_ladder_42_ladder_2;
                            for (;;)
                            {
                                if ((abs(_669 - _253) < (2.0f / _929)) && (abs(_671 - _254) < (2.0f / _930)))
                                {
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder = 0.0f;
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder_1 = _665;
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder_2 = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_86;
                                    float frontier_phi_86_pred;
                                    uint frontier_phi_86_pred_1;
                                    float frontier_phi_86_pred_2;
                                    uint _1227;
                                    uint _1228;
                                    bool _1229;
                                    for (;;)
                                    {
                                        _1227 = uint(int(_669 * _929));
                                        _1228 = uint(int(_671 * _930));
                                        _1229 = _618 && _525;
                                        if (!_1229)
                                        {
                                            if (!(dot(float3(_730, _731, _732), float3(_730, _731, _732)) < 0.000899999984540045261383056640625f))
                                            {
                                                ladder_phi_86 = false;
                                                frontier_phi_86_pred = 0.0f;
                                                frontier_phi_86_pred_1 = _665;
                                                frontier_phi_86_pred_2 = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1308 = _24[22u].Load(int3(uint2(_1227, _1228), 0u));
                                        uint _1310 = _1308.x;
                                        float _1695;
                                        float _1696;
                                        float _1697;
                                        if (_1310 == 0u)
                                        {
                                            uint4 _1465 = _24[1u].Load(int3(uint2(_1227, _1228), 0u));
                                            uint _1467 = _1465.x;
                                            float _1475 = (float((_1467 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1476 = (float(_1467 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1480 = (1.0f - abs(_1475)) - abs(_1476);
                                            float _1482 = clamp((-0.0f) - _1480, 0.0f, 1.0f);
                                            float _1483 = (-0.0f) - _1482;
                                            _1695 = ((_1475 >= 0.0f) ? _1483 : _1482) + _1475;
                                            _1696 = ((_1476 >= 0.0f) ? _1483 : _1482) + _1476;
                                            _1697 = _1480;
                                        }
                                        else
                                        {
                                            float _1497 = (float((_1310 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1498 = (float(_1310 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _1502 = (1.0f - abs(_1497)) - abs(_1498);
                                            float _1504 = clamp((-0.0f) - _1502, 0.0f, 1.0f);
                                            float _1505 = (-0.0f) - _1504;
                                            _1695 = ((_1497 >= 0.0f) ? _1505 : _1504) + _1497;
                                            _1696 = ((_1498 >= 0.0f) ? _1505 : _1504) + _1498;
                                            _1697 = _1502;
                                        }
                                        float _1701 = rsqrt(dot(float3(_1695, _1696, _1697), float3(_1695, _1696, _1697)));
                                        if (dot(float3(_1701 * _1695, _1701 * _1696, _1701 * _1697), float3(_730, _731, _732)) > 0.0f)
                                        {
                                            ladder_phi_86 = true;
                                            frontier_phi_86_pred = 0.0f;
                                            frontier_phi_86_pred_1 = _665;
                                            frontier_phi_86_pred_2 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_86 = false;
                                            frontier_phi_86_pred = 0.0f;
                                            frontier_phi_86_pred_1 = _665;
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
                                    float _1516 = _49_m0[51u].z * _669;
                                    float _1517 = _49_m0[51u].w * _671;
                                    float _1519 = (_930 / _929) * 0.0500000007450580596923828125f;
                                    float _1524 = clamp(_1516 / _1519, 0.0f, 1.0f);
                                    float _1525 = clamp(_1517 * 20.0f, 0.0f, 1.0f);
                                    float _1537 = clamp(((_1516 + (-1.0f)) + _1519) / _1519, 0.0f, 1.0f);
                                    float _1538 = clamp((_1517 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _1549 = _1524 * _1525;
                                    precise float _1550 = _1549 * _1549;
                                    float _1554 = ((((3.0f - (_1525 * 2.0f)) * (3.0f - (_1524 * 2.0f))) * _1550) * (1.0f - ((_1537 * _1537) * (3.0f - (_1537 * 2.0f))))) * (1.0f - ((_1538 * _1538) * (3.0f - (_1538 * 2.0f))));
                                    bool _1557 = (_665 != 0u) || (_1554 >= 1.0f);
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder = _1554 * float(_464 > 0.0f);
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder_1 = _1557 ? _665 : 1u;
                                    frontier_phi_25_26_ladder_33_ladder_42_ladder_2 = _1557 ? 0.0f : _1554;
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
                    _882 = frontier_phi_25_26_ladder_2;
                    _884 = frontier_phi_25_26_ladder_1;
                    _886 = frontier_phi_25_26_ladder;
                }
                uint _1041;
                uint _1043;
                float _927;
                for (;;)
                {
                    _927 = ((((exp2(log2(clamp((sqrt(((_703 * _703) + (_702 * _702)) + (_704 * _704)) - _44_m0[121u].y) * _44_m0[121u].z, 0.0f, 1.0f)) * _44_m0[121u].w) * _404) * exp2(log2(clamp((_703 - _44_m0[122u].x) * _44_m0[122u].y, 0.0f, 1.0f)) * _44_m0[122u].z)) * (1.0f - clamp(_392, 0.0f, 1.0f))) * (max(_44_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                    if (_886 > 0.0f)
                    {
                        uint _1003 = uint((_201 * 0.999989986419677734375f) * clamp(_669, 0.0f, 1.0f));
                        uint _1004 = uint((_203 * 0.999989986419677734375f) * clamp(_671, 0.0f, 1.0f));
                        uint4 _1007 = _24[2u].Load(int3(uint2(_1003, _1004), 0u));
                        uint _1010 = _1007.w;
                        uint4 _1015 = _24[15u].Load(int3(uint2(_1003, _1004), 0u));
                        uint _1017 = _1015.y;
                        uint _1023 = ((_1017 & 64u) != 0u) ? uint((_1017 & 4294967167u) != 66u) : 4294967295u;
                        uint _1024 = _1010 & 128u;
                        uint _1026 = (_1024 != 0u) ? 1u : ((_1007.x << 7u) | _1010);
                        uint4 _1029 = _16.Load(_1026 * 4u);
                        uint _1030 = _1029.x;
                        uint _1037 = ((_1030 & 1u) != 0u) ? 0u : 18u;
                        uint _1039 = uint(min(int(uint(max(int(_1023), int(0u)))), int(1u)));
                        uint _1121;
                        if (_1024 == 0u)
                        {
                            _1121 = (((_1030 & 2097152u) != 0u) && (_1023 == _1039)) ? (_1037 | 128u) : _1037;
                        }
                        else
                        {
                            _1121 = _1010;
                        }
                        uint _1122 = _16.Load((_1026 * 4u) + 1u).x & 512u;
                        bool _1125 = (_1121 & 144u) == 0u;
                        if (_1122 == 0u)
                        {
                            if (_1125 || ((_1030 & 1u) != 0u))
                            {
                                _1041 = 0u;
                                _1043 = 0u;
                                break;
                            }
                        }
                        else
                        {
                            if (_1125)
                            {
                                _1041 = 0u;
                                _1043 = 0u;
                                break;
                            }
                        }
                        bool _1268 = ((_1121 & 128u) | _1122) != 0u;
                        uint _1042;
                        if (_1268)
                        {
                            _1042 = 1u;
                        }
                        else
                        {
                            _1042 = (((_1030 >> 14u) & 2u) ^ 2u) + 3u;
                        }
                        if (((_1030 & 268435472u) == 16u) && (((_1030 & 33554432u) == 0u) || _1268))
                        {
                            _1041 = 2u;
                            _1043 = 0u;
                            break;
                        }
                        _1041 = _1042;
                        _1043 = (_1042 == 1u) ? _1039 : 0u;
                        break;
                    }
                    else
                    {
                        _1041 = 0u;
                        _1043 = 0u;
                        break;
                    }
                }
                uint _1130;
                float _1131;
                float _1132;
                float _1133;
                float _1135;
                float _1047;
                float _1050;
                float _1053;
                float _1057;
                float _1058;
                for (;;)
                {
                    _1047 = mad(_166, _521, mad(_160, _520, _519 * _154));
                    _1050 = mad(_167, _521, mad(_161, _520, _519 * _155));
                    _1053 = mad(_168, _521, mad(_162, _520, _519 * _156));
                    bool _1056 = (_884 != 0u) || (_886 < 1.0f);
                    _1057 = _1056 ? 0.0f : 1.0f;
                    _1058 = _1056 ? 0.0f : 0.5f;
                    if (_1056)
                    {
                        uint4 _1129 = asuint(_49_m0[60u]);
                        if (_618)
                        {
                            if (int(_402) < int(1u))
                            {
                                if (_1129.x == 0u)
                                {
                                    _1130 = 0u;
                                    _1131 = 0.0f;
                                    _1132 = 0.0f;
                                    _1133 = 0.0f;
                                    _1135 = 0.0f;
                                    break;
                                }
                            }
                            else
                            {
                                if (_1129.y == 0u)
                                {
                                    _1130 = 0u;
                                    _1131 = 0.0f;
                                    _1132 = 0.0f;
                                    _1133 = 0.0f;
                                    _1135 = 0.0f;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (_1129.z == 0u)
                            {
                                _1130 = 0u;
                                _1131 = 0.0f;
                                _1132 = 0.0f;
                                _1133 = 0.0f;
                                _1135 = 0.0f;
                                break;
                            }
                        }
                        if (_882 > 0.0f)
                        {
                            _1130 = 0u;
                            _1131 = 0.0f;
                            _1132 = 1.0f;
                            _1133 = 1000.0f;
                            _1135 = 0.5f;
                            break;
                        }
                        if (((_659 <= 0.0f) || (_661 <= 0.0f)) || (_663 <= 0.0f))
                        {
                            _1130 = 0u;
                            _1131 = 0.0f;
                            _1132 = 1.0f;
                            _1133 = 1000.0f;
                            _1135 = 0.5f;
                            break;
                        }
                        if ((_663 >= 1.0f) || ((_659 >= _49_m0[51u].x) || (_661 >= _49_m0[51u].y)))
                        {
                            _1130 = 0u;
                            _1131 = 0.0f;
                            _1132 = 1.0f;
                            _1133 = 1000.0f;
                            _1135 = 0.5f;
                            break;
                        }
                        uint _1932;
                        uint _1934;
                        uint _1730;
                        uint _1731;
                        bool _1737;
                        for (;;)
                        {
                            _1730 = uint(clamp(_653, 0.0f, 1.0f) * _201);
                            _1731 = uint(clamp(_655, 0.0f, 1.0f) * _203);
                            _1737 = _20[21u].Load(int3(uint2(_1730, _1731), 0u)).x > 0.0f;
                            if (_1737)
                            {
                                uint _1821 = _24[23u].Load(int3(uint2(_1730, _1731), 0u)).y + 4294967295u;
                                _1932 = (uint(int(_1821) >> int(31u)) & 3u) + 1u;
                                _1934 = (int(_1821) < int(0u)) ? 0u : _1821;
                                break;
                            }
                            else
                            {
                                uint4 _1829 = _24[2u].Load(int3(uint2(_1730, _1731), 0u));
                                uint _1832 = _1829.w;
                                uint4 _1837 = _24[15u].Load(int3(uint2(_1730, _1731), 0u));
                                uint _1839 = _1837.y;
                                uint _1845 = ((_1839 & 64u) != 0u) ? uint((_1839 & 4294967167u) != 66u) : 4294967295u;
                                uint _1846 = _1832 & 128u;
                                uint _1848 = (_1846 != 0u) ? 1u : ((_1829.x << 7u) | _1832);
                                uint4 _1851 = _16.Load(_1848 * 4u);
                                uint _1852 = _1851.x;
                                uint _1859 = ((_1852 & 1u) != 0u) ? 0u : 18u;
                                uint _1861 = uint(min(int(uint(max(int(_1845), int(0u)))), int(1u)));
                                uint _1945;
                                if (_1846 == 0u)
                                {
                                    _1945 = (((_1852 & 2097152u) != 0u) && (_1845 == _1861)) ? (_1859 | 128u) : _1859;
                                }
                                else
                                {
                                    _1945 = _1832;
                                }
                                uint _1946 = _16.Load((_1848 * 4u) + 1u).x & 512u;
                                bool _1949 = (_1945 & 144u) == 0u;
                                if (_1946 == 0u)
                                {
                                    if (_1949 || ((_1852 & 1u) != 0u))
                                    {
                                        _1932 = 0u;
                                        _1934 = 0u;
                                        break;
                                    }
                                }
                                else
                                {
                                    if (_1949)
                                    {
                                        _1932 = 0u;
                                        _1934 = 0u;
                                        break;
                                    }
                                }
                                bool _2041 = ((_1945 & 128u) | _1946) != 0u;
                                uint _1933;
                                if (_2041)
                                {
                                    _1933 = 1u;
                                }
                                else
                                {
                                    _1933 = (((_1852 >> 14u) & 2u) ^ 2u) + 3u;
                                }
                                if (((_1852 & 268435472u) == 16u) && (((_1852 & 33554432u) == 0u) || _2041))
                                {
                                    _1932 = 2u;
                                    _1934 = 0u;
                                    break;
                                }
                                _1932 = _1933;
                                _1934 = (_1933 == 1u) ? _1861 : 0u;
                                break;
                            }
                        }
                        if ((_396 != _1932) || (_402 != _1934))
                        {
                            _1130 = 0u;
                            _1131 = 0.0f;
                            _1132 = 1.0f;
                            _1133 = 1000.0f;
                            _1135 = 0.5f;
                            break;
                        }
                        float _1972 = _659 * 2.0f;
                        float _1975 = (_49_m0[51u].z * _1972) + (-1.0f);
                        float _1976 = ((1.0f - (_49_m0[51u].w * _661)) * 2.0f) + (-1.0f);
                        float _1992 = mad(_189, _663, mad(_182, _1976, _1975 * _175)) + _196;
                        float _1993 = (mad(_186, _663, mad(_179, _1976, _1975 * _172)) + _193) / _1992;
                        float _1994 = (mad(_187, _663, mad(_180, _1976, _1975 * _173)) + _194) / _1992;
                        float _1995 = (mad(_188, _663, mad(_181, _1976, _1975 * _174)) + _195) / _1992;
                        if (sqrt(((_1994 * _1994) + (_1993 * _1993)) + (_1995 * _1995)) > _49_m0[58u].w)
                        {
                            _1130 = 0u;
                            _1131 = 0.0f;
                            _1132 = 0.0f;
                            _1133 = 1000.0f;
                            _1135 = 0.5f;
                            break;
                        }
                        float _2021 = _1993 - _702;
                        float _2022 = _1994 - _703;
                        float _2023 = _1995 - _704;
                        float _2029 = sqrt(((_2022 * _2022) + (_2021 * _2021)) + (_2023 * _2023));
                        float _2037 = min(_49_m0[59u].y, max(0.0f, _2029 + (-0.001000000047497451305389404296875f)));
                        float _2103;
                        if (_618)
                        {
                            _2103 = min(_49_m0[59u].x, _2037 + 10.0f);
                        }
                        else
                        {
                            _2103 = _49_m0[59u].x;
                        }
                        float _2104 = _2103 - _2029;
                        if (!(_2104 > 0.0f))
                        {
                            _1130 = 0u;
                            _1131 = 1.0f;
                            _1132 = 1.0f;
                            _1133 = 0.0f;
                            _1135 = 0.5f;
                            break;
                        }
                        float _2126 = _1993 - (_2037 * _1047);
                        float _2127 = _1994 - (_2037 * _1050);
                        float _2128 = _1995 - (_2037 * _1053);
                        RayDesc _2ident = {float3(mad(_2128, _49_m0[46u].z, mad(_2127, _49_m0[46u].y, _49_m0[46u].x * _2126)) + _49_m0[46u].w, mad(_2128, _49_m0[47u].z, mad(_2127, _49_m0[47u].y, _49_m0[47u].x * _2126)) + _49_m0[47u].w, mad(_2128, _49_m0[48u].z, mad(_2127, _49_m0[48u].y, _49_m0[48u].x * _2126)) + _49_m0[48u].w), 0.0f, float3(mad(_1053, _49_m0[46u].z, mad(_1050, _49_m0[46u].y, _49_m0[46u].x * _1047)), mad(_1053, _49_m0[47u].z, mad(_1050, _49_m0[47u].y, _49_m0[47u].x * _1047)), mad(_1053, _49_m0[48u].z, mad(_1050, _49_m0[48u].y, _49_m0[48u].x * _1047))), _2104};
                        _2131.TraceRayInline(_32, 537u, 4294967295u, _2ident);
                        bool _2178 = _2131.Proceed();
                        uint _2179 = _2131.CommittedStatus();
                        if (!(_2179 == 1u))
                        {
                            _1130 = 0u;
                            _1131 = 1.0f;
                            _1132 = 0.0f;
                            _1133 = 0.0f;
                            _1135 = 0.5f;
                            break;
                        }
                        float _2248 = _2131.CommittedRayT();
                        if (!((_2248 < _2104) && (_2248 > 0.0f)))
                        {
                            _1130 = 0u;
                            _1131 = 1.0f;
                            _1132 = 0.0f;
                            _1133 = 0.0f;
                            _1135 = 0.5f;
                            break;
                        }
                        float _2299 = (_49_m0[51u].z * _1972) + (-1.0f);
                        float _2300 = ((1.0f - (_49_m0[51u].w * _661)) * 2.0f) + (-1.0f);
                        float _2316 = mad(_143, _663, mad(_136, _2300, _2299 * _129)) + _150;
                        float _2320 = _2248 - _2037;
                        float _2324 = ((mad(_140, _663, mad(_133, _2300, _2299 * _126)) + _147) / _2316) + (_2320 * _519);
                        float _2325 = ((mad(_141, _663, mad(_134, _2300, _2299 * _127)) + _148) / _2316) + (_2320 * _520);
                        float _2326 = ((mad(_142, _663, mad(_135, _2300, _2299 * _128)) + _149) / _2316) + (_2320 * _521);
                        float _2338 = mad(_115, _2326, mad(_108, _2325, _2324 * _101)) + _122;
                        float _2348 = (_49_m0[51u].z * _49_m0[51u].x) * ((((mad(_112, _2326, mad(_105, _2325, _2324 * _98)) + _119) / _2338) * 0.5f) + 0.5f);
                        float _2350 = (_49_m0[51u].w * _49_m0[51u].y) * (0.5f - (((mad(_113, _2326, mad(_106, _2325, _2324 * _99)) + _120) / _2338) * 0.5f));
                        float _2352 = (_203 / _201) * 0.0500000007450580596923828125f;
                        float _2355 = clamp(_2348 / _2352, 0.0f, 1.0f);
                        float _2356 = clamp(_2350 * 20.0f, 0.0f, 1.0f);
                        float _2366 = clamp(((_2352 + (-1.0f)) + _2348) / _2352, 0.0f, 1.0f);
                        float _2367 = clamp((_2350 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                        precise float _2378 = _2355 * _2356;
                        precise float _2379 = _2378 * _2378;
                        if ((((((3.0f - (_2356 * 2.0f)) * (3.0f - (_2355 * 2.0f))) * _2379) * (1.0f - ((_2366 * _2366) * (3.0f - (_2366 * 2.0f))))) * (1.0f - ((_2367 * _2367) * (3.0f - (_2367 * 2.0f))))) < 1.0f)
                        {
                            _1130 = 0u;
                            _1131 = _1057;
                            _1132 = _1057;
                            _1133 = 0.0f;
                            _1135 = _1058;
                            break;
                        }
                        _1130 = 1u;
                        _1131 = 0.0f;
                        _1132 = 1.0f;
                        _1133 = 0.0f;
                        _1135 = 0.5f;
                        break;
                    }
                    else
                    {
                        _1130 = 0u;
                        _1131 = 1.0f;
                        _1132 = 1.0f;
                        _1133 = 0.0f;
                        _1135 = 0.5f;
                        break;
                    }
                }
                uint _1215;
                float _1216;
                float _1217;
                float _1219;
                float _1221;
                if (_618 && (asuint(_49_m0[55u]).z != 0u))
                {
                    uint frontier_phi_52_51_ladder;
                    float frontier_phi_52_51_ladder_1;
                    float frontier_phi_52_51_ladder_2;
                    float frontier_phi_52_51_ladder_3;
                    float frontier_phi_52_51_ladder_4;
                    if ((_1041 != 1u) || (_1043 != 0u))
                    {
                        float _1277 = _564 / max(9.9999999747524270787835121154785e-07f, (-0.0f) - _611);
                        float _1286 = ((_1277 * _608) + _571) / _49_m0[51u].x;
                        float _1287 = ((_1277 * _610) + _572) / _49_m0[51u].y;
                        float _1401;
                        if (_1287 < 0.300000011920928955078125f)
                        {
                            float _1393 = (0.300000011920928955078125f - _1287) * 3.3333332538604736328125f;
                            _1401 = 0.300000011920928955078125f - ((_1393 / sqrt((_1393 * _1393) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _1401 = _1287;
                        }
                        float _1603;
                        if (_1286 < 0.300000011920928955078125f)
                        {
                            float _1596 = (0.300000011920928955078125f - _1286) * 3.3333332538604736328125f;
                            _1603 = 0.300000011920928955078125f - ((_1596 / sqrt((_1596 * _1596) + 1.0f)) * 0.300000011920928955078125f);
                        }
                        else
                        {
                            _1603 = _1286;
                        }
                        float _1748;
                        if ((1.0f - _1603) < 0.300000011920928955078125f)
                        {
                            float _1740 = (_1603 + (-0.699999988079071044921875f)) * 3.3333332538604736328125f;
                            _1748 = ((_1740 / sqrt((_1740 * _1740) + 1.0f)) * 0.300000011920928955078125f) + 0.699999988079071044921875f;
                        }
                        else
                        {
                            _1748 = _1603;
                        }
                        frontier_phi_52_51_ladder = 0u;
                        frontier_phi_52_51_ladder_1 = 0.0f;
                        frontier_phi_52_51_ladder_2 = _1748 * _49_m0[51u].x;
                        frontier_phi_52_51_ladder_3 = 1.0f - clamp((_521 + (-0.25f)) * (-4.0f), 0.0f, 1.0f);
                        frontier_phi_52_51_ladder_4 = _1401 * _49_m0[51u].y;
                    }
                    else
                    {
                        frontier_phi_52_51_ladder = _649;
                        frontier_phi_52_51_ladder_1 = _657;
                        frontier_phi_52_51_ladder_2 = _669;
                        frontier_phi_52_51_ladder_3 = _886;
                        frontier_phi_52_51_ladder_4 = _671;
                    }
                    _1215 = frontier_phi_52_51_ladder;
                    _1216 = frontier_phi_52_51_ladder_1;
                    _1217 = frontier_phi_52_51_ladder_2;
                    _1219 = frontier_phi_52_51_ladder_4;
                    _1221 = frontier_phi_52_51_ladder_3;
                }
                else
                {
                    _1215 = _649;
                    _1216 = _657;
                    _1217 = _669;
                    _1219 = _671;
                    _1221 = _886;
                }
                bool _1223 = _396 != 1u;
                float _1622;
                float _1624;
                float _1626;
                float _1628;
                if (_1221 == 0.0f)
                {
                    float _1403;
                    float _1405;
                    float _1407;
                    float _1409;
                    if (_1130 == 0u)
                    {
                        float frontier_phi_81_80_ladder;
                        float frontier_phi_81_80_ladder_1;
                        float frontier_phi_81_80_ladder_2;
                        float frontier_phi_81_80_ladder_3;
                        if (_1223)
                        {
                            float _1606 = _1050 * _927;
                            float _1610 = rsqrt(dot(float3(_1047, _1606, _1053), float3(_1047, _1606, _1053)));
                            float4 _1620 = _28[4u].SampleLevel(_58, float3(_1610 * _1047, _1610 * _1606, _1610 * _1053), 0.0f);
                            frontier_phi_81_80_ladder = 1.0f;
                            frontier_phi_81_80_ladder_1 = _1620.z;
                            frontier_phi_81_80_ladder_2 = _1620.y;
                            frontier_phi_81_80_ladder_3 = _1620.x;
                        }
                        else
                        {
                            frontier_phi_81_80_ladder = 0.0f;
                            frontier_phi_81_80_ladder_1 = 0.0f;
                            frontier_phi_81_80_ladder_2 = 0.0f;
                            frontier_phi_81_80_ladder_3 = 0.0f;
                        }
                        _1403 = frontier_phi_81_80_ladder_3;
                        _1405 = frontier_phi_81_80_ladder_2;
                        _1407 = frontier_phi_81_80_ladder_1;
                        _1409 = frontier_phi_81_80_ladder;
                    }
                    else
                    {
                        _1403 = 0.0f;
                        _1405 = 0.0f;
                        _1407 = 0.0f;
                        _1409 = 1.0f;
                    }
                    _1622 = _1403 * _464;
                    _1624 = _1405 * _464;
                    _1626 = _1407 * _464;
                    _1628 = _1409 * _464;
                }
                else
                {
                    uint4 _1292 = asuint(_54_m0[0u]);
                    float _1676;
                    float _1678;
                    float _1682;
                    float _1685;
                    if (_12.Load(int3(uint2(uint(float(_1292.x) * _1217), uint(float(_1292.y) * _1219)), 0u)).x > 0.0f)
                    {
                        float frontier_phi_101_82_ladder;
                        float frontier_phi_101_82_ladder_1;
                        float frontier_phi_101_82_ladder_2;
                        float frontier_phi_101_82_ladder_3;
                        float _1458;
                        float _1459;
                        float _1460;
                        for (;;)
                        {
                            uint _1415_dummy_parameter;
                            uint2 _1415 = spvTextureSize(_14, 0u, _1415_dummy_parameter);
                            float4 _1424 = _14.Load(int3(uint2(uint(float(_1415.x) * _1217), uint(float(_1415.y) * _1219)), 0u));
                            float4 _1448 = _13.SampleLevel(_57, float2(((_1424.x * 0.5f) * _49_m0[52u].z) + (_49_m0[52u].x * _1217), ((_1424.y * (-0.5f)) * _49_m0[52u].w) + (_49_m0[52u].y * _1219)), 0.0f);
                            _1458 = _49_m0[54u].x * _1448.x;
                            _1459 = _49_m0[54u].x * _1448.y;
                            _1460 = _49_m0[54u].x * _1448.z;
                            if (_1216 > 0.0f)
                            {
                                float _1681;
                                float _1684;
                                float _1687;
                                if (_618)
                                {
                                    _1681 = 0.0f;
                                    _1684 = 0.0f;
                                    _1687 = 0.0f;
                                }
                                else
                                {
                                    if (!((_396 != 4u) || (_651 != 0u)))
                                    {
                                        frontier_phi_101_82_ladder = _1460;
                                        frontier_phi_101_82_ladder_1 = _1459;
                                        frontier_phi_101_82_ladder_2 = _1458;
                                        frontier_phi_101_82_ladder_3 = _1221;
                                        break;
                                    }
                                    _1681 = _1458;
                                    _1684 = _1459;
                                    _1687 = _1460;
                                }
                                frontier_phi_101_82_ladder = _1687;
                                frontier_phi_101_82_ladder_1 = _1684;
                                frontier_phi_101_82_ladder_2 = _1681;
                                frontier_phi_101_82_ladder_3 = (1.0f - exp2(log2(_1216) * 3.0f)) * _1221;
                                break;
                            }
                            else
                            {
                                frontier_phi_101_82_ladder = _1460;
                                frontier_phi_101_82_ladder_1 = _1459;
                                frontier_phi_101_82_ladder_2 = _1458;
                                frontier_phi_101_82_ladder_3 = _1221;
                                break;
                            }
                        }
                        _1676 = frontier_phi_101_82_ladder_3;
                        _1678 = frontier_phi_101_82_ladder_2;
                        _1682 = frontier_phi_101_82_ladder_1;
                        _1685 = frontier_phi_101_82_ladder;
                    }
                    else
                    {
                        float frontier_phi_101_83_ladder;
                        float frontier_phi_101_83_ladder_1;
                        float frontier_phi_101_83_ladder_2;
                        float frontier_phi_101_83_ladder_3;
                        if (_1215 == 0u)
                        {
                            float4 _1693 = _28[7u].SampleLevel(_58, float3(_1047, _1050, _1053), 0.0f);
                            frontier_phi_101_83_ladder = _1693.z;
                            frontier_phi_101_83_ladder_1 = _1693.y;
                            frontier_phi_101_83_ladder_2 = _1693.x;
                            frontier_phi_101_83_ladder_3 = _1221;
                        }
                        else
                        {
                            frontier_phi_101_83_ladder = _1680;
                            frontier_phi_101_83_ladder_1 = _1680;
                            frontier_phi_101_83_ladder_2 = _1680;
                            frontier_phi_101_83_ladder_3 = 0.0f;
                        }
                        _1676 = frontier_phi_101_83_ladder_3;
                        _1678 = frontier_phi_101_83_ladder_2;
                        _1682 = frontier_phi_101_83_ladder_1;
                        _1685 = frontier_phi_101_83_ladder;
                    }
                    float _1893;
                    float _1894;
                    float _1895;
                    float _1896;
                    if (_1130 == 0u)
                    {
                        float frontier_phi_125_115_ladder;
                        float frontier_phi_125_115_ladder_1;
                        float frontier_phi_125_115_ladder_2;
                        float frontier_phi_125_115_ladder_3;
                        if (_1223 && (_1676 < 1.0f))
                        {
                            float _1867 = _1050 * _927;
                            float _1871 = rsqrt(dot(float3(_1047, _1867, _1053), float3(_1047, _1867, _1053)));
                            float4 _1879 = _28[4u].SampleLevel(_58, float3(_1871 * _1047, _1871 * _1867, _1871 * _1053), 0.0f);
                            float _1881 = _1879.x;
                            float _1882 = _1879.y;
                            float _1883 = _1879.z;
                            frontier_phi_125_115_ladder = ((_1682 - _1882) * _1676) + _1882;
                            frontier_phi_125_115_ladder_1 = 1.0f;
                            frontier_phi_125_115_ladder_2 = ((_1678 - _1881) * _1676) + _1881;
                            frontier_phi_125_115_ladder_3 = ((_1685 - _1883) * _1676) + _1883;
                        }
                        else
                        {
                            frontier_phi_125_115_ladder = _1682;
                            frontier_phi_125_115_ladder_1 = _1676;
                            frontier_phi_125_115_ladder_2 = _1678;
                            frontier_phi_125_115_ladder_3 = _1685;
                        }
                        _1893 = frontier_phi_125_115_ladder_1;
                        _1894 = frontier_phi_125_115_ladder_2;
                        _1895 = frontier_phi_125_115_ladder;
                        _1896 = frontier_phi_125_115_ladder_3;
                    }
                    else
                    {
                        _1893 = 1.0f;
                        _1894 = _1678 * _882;
                        _1895 = _1682 * _882;
                        _1896 = _1685 * _882;
                    }
                    float _1629 = _1893 * _464;
                    _1622 = _1894 * _1629;
                    _1624 = _1895 * _1629;
                    _1626 = _1896 * _1629;
                    _1628 = _1629;
                }
                float _1633 = _49_m0[58u].z * _1135;
                float _1655 = max(_49_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _1657 = _1655 * ((_1633 * ((_1131 * 1000.0f) - _1622)) + _1622);
                float _1658 = _1655 * ((_1633 * ((_1132 * 1000.0f) - _1624)) + _1624);
                float _1659 = _1655 * ((_1633 * (_1133 - _1626)) + _1626);
                float _1665 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_1657, max(_1658, _1659)) + 1.0f);
                float _1669 = min(_1665 * _1657, 0.996078431606292724609375f);
                float _1671 = min(_1665 * _1658, 0.996078431606292724609375f);
                float _1672 = min(_1665 * _1659, 0.996078431606292724609375f);
                _35[uint2(_220, _223)] = float4(_1669, _1671, _1672, _1628);
                if (_227)
                {
                    _35[uint2(_220 + 1u, _223)] = float4(_1669, _1671, _1672, _1628);
                }
                if (_230)
                {
                    _35[uint2(_220, _223 + 1u)] = float4(_1669, _1671, _1672, _1628);
                }
                if (_231)
                {
                    _35[uint2(_220 + 1u, _223 + 1u)] = float4(_1669, _1671, _1672, _1628);
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
