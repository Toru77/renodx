static float _2201;
static uint _2669;
static uint _2670;
static uint _2671;
static float _2672;
static float _2673;
static float _2674;
static float _2675;
static uint _2676;
static float _2677;
static float _2678;
static float _2679;
static float _2680;
static float _2681;
static uint _2682;
static float _2688;
static float _2689;
static float _2690;

cbuffer _39_41 : register(b2, space0)
{
    float4 _41_m0[700] : packoffset(c0);
};

cbuffer _44_46 : register(b3, space0)
{
    float4 _46_m0[69] : packoffset(c0);
};

cbuffer _49_51 : register(b4, space0)
{
    float4 _51_m0[1] : packoffset(c0);
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
RWTexture2D<float4> _31 : register(u0, space0);
RWBuffer<uint> _34 : register(u1, space0);
RWTexture2D<float4> _35 : register(u2, space0);
SamplerState _54 : register(s0, space0);
SamplerState _55 : register(s5, space0);

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
    uint _73;
    float _77;
    float _78;
    float _79;
    float _83;
    float _84;
    float _85;
    float _89;
    float _90;
    float _91;
    float _95;
    float _96;
    float _97;
    float _98;
    float _102;
    float _103;
    float _104;
    float _105;
    float _109;
    float _110;
    float _111;
    float _112;
    float _116;
    float _117;
    float _118;
    float _119;
    float _123;
    float _124;
    float _125;
    float _126;
    float _130;
    float _131;
    float _132;
    float _133;
    float _137;
    float _138;
    float _139;
    float _140;
    float _144;
    float _145;
    float _146;
    float _147;
    float _151;
    float _152;
    float _153;
    float _157;
    float _158;
    float _159;
    float _163;
    float _164;
    float _165;
    float _169;
    float _170;
    float _171;
    float _172;
    float _176;
    float _177;
    float _178;
    float _179;
    float _183;
    float _184;
    float _185;
    float _186;
    float _190;
    float _191;
    float _192;
    float _193;
    float _198;
    float _200;
    uint _205;
    for (;;)
    {
        uint4 _72 = asuint(_41_m0[176u]);
        _73 = _72.z;
        _77 = _46_m0[14u].x;
        _78 = _46_m0[14u].y;
        _79 = _46_m0[14u].z;
        _83 = _46_m0[15u].x;
        _84 = _46_m0[15u].y;
        _85 = _46_m0[15u].z;
        _89 = _46_m0[16u].x;
        _90 = _46_m0[16u].y;
        _91 = _46_m0[16u].z;
        _95 = _46_m0[22u].x;
        _96 = _46_m0[22u].y;
        _97 = _46_m0[22u].z;
        _98 = _46_m0[22u].w;
        _102 = _46_m0[23u].x;
        _103 = _46_m0[23u].y;
        _104 = _46_m0[23u].z;
        _105 = _46_m0[23u].w;
        _109 = _46_m0[24u].x;
        _110 = _46_m0[24u].y;
        _111 = _46_m0[24u].z;
        _112 = _46_m0[24u].w;
        _116 = _46_m0[25u].x;
        _117 = _46_m0[25u].y;
        _118 = _46_m0[25u].z;
        _119 = _46_m0[25u].w;
        _123 = _46_m0[26u].x;
        _124 = _46_m0[26u].y;
        _125 = _46_m0[26u].z;
        _126 = _46_m0[26u].w;
        _130 = _46_m0[27u].x;
        _131 = _46_m0[27u].y;
        _132 = _46_m0[27u].z;
        _133 = _46_m0[27u].w;
        _137 = _46_m0[28u].x;
        _138 = _46_m0[28u].y;
        _139 = _46_m0[28u].z;
        _140 = _46_m0[28u].w;
        _144 = _46_m0[29u].x;
        _145 = _46_m0[29u].y;
        _146 = _46_m0[29u].z;
        _147 = _46_m0[29u].w;
        _151 = _46_m0[18u].x;
        _152 = _46_m0[18u].y;
        _153 = _46_m0[18u].z;
        _157 = _46_m0[19u].x;
        _158 = _46_m0[19u].y;
        _159 = _46_m0[19u].z;
        _163 = _46_m0[20u].x;
        _164 = _46_m0[20u].y;
        _165 = _46_m0[20u].z;
        _169 = _46_m0[30u].x;
        _170 = _46_m0[30u].y;
        _171 = _46_m0[30u].z;
        _172 = _46_m0[30u].w;
        _176 = _46_m0[31u].x;
        _177 = _46_m0[31u].y;
        _178 = _46_m0[31u].z;
        _179 = _46_m0[31u].w;
        _183 = _46_m0[32u].x;
        _184 = _46_m0[32u].y;
        _185 = _46_m0[32u].z;
        _186 = _46_m0[32u].w;
        _190 = _46_m0[33u].x;
        _191 = _46_m0[33u].y;
        _192 = _46_m0[33u].z;
        _193 = _46_m0[33u].w;
        uint4 _196 = asuint(_51_m0[0u]);
        _198 = float(_196.x);
        _200 = float(_196.y);
        _205 = (((gl_WorkGroupID.y << 6u) + gl_WorkGroupID.x) << 6u) + gl_LocalInvocationIndex;
        if (_205 < _34[1u].xxxx.x)
        {
            bool ladder_phi_8;
            float frontier_phi_8_pred;
            uint _217;
            uint _220;
            bool _224;
            bool _227;
            bool _228;
            uint _229;
            uint _230;
            float _250;
            float _251;
            float _258;
            bool _259;
            uint _261;
            uint _262;
            for (;;)
            {
                uint4 _215 = _8.Load(_205);
                uint _216 = _215.x;
                _217 = _216 & 32767u;
                uint _219 = _216 >> 15u;
                _220 = _219 & 16383u;
                _224 = (_216 & 536870912u) != 0u;
                _227 = (_216 & 1073741824u) != 0u;
                _228 = int(_216) < int(0u);
                _229 = uint(_198);
                _230 = uint(_200);
                bool _233 = ((_219 + _216) & 1u) == 0u;
                float _243 = float(int(_217));
                float _244 = float(int(_220));
                _250 = ((_243 + 0.5f) + (_233 ? _41_m0[58u].x : _41_m0[58u].z)) * (1.0f / _198);
                _251 = ((_244 + 0.5f) + (_233 ? _41_m0[58u].y : _41_m0[58u].w)) * (1.0f / _200);
                _258 = _20[21u].Load(int3(uint2(_217, _220), 0u)).x;
                _259 = _258 > 0.0f;
                _261 = uint(_243);
                _262 = uint(_244);
                float _383;
                float _385;
                float _387;
                uint _389;
                float _391;
                float _393;
                float _395;
                float _397;
                if (_259)
                {
                    uint4 _266 = _24[22u].Load(int3(uint2(_261, _262), 0u));
                    uint _268 = _266.x;
                    float _281 = (float((_268 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _283 = (float(_268 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _288 = (1.0f - abs(_281)) - abs(_283);
                    float _291 = clamp((-0.0f) - _288, 0.0f, 1.0f);
                    float _292 = (-0.0f) - _291;
                    float _297 = ((_281 >= 0.0f) ? _292 : _291) + _281;
                    float _298 = ((_283 >= 0.0f) ? _292 : _291) + _283;
                    float _303 = rsqrt(dot(float3(_297, _298, _288), float3(_297, _298, _288)));
                    float _308 = float(_268 & 255u) * 0.0039215688593685626983642578125f;
                    uint _318 = uint(int(_24[23u].Load(int3(uint2(_261, _262), 0u)).y + 4294967295u) >> int(31u)) & 3u;
                    uint _321 = _318 + 103u;
                    float _330 = clamp((_308 - _41_m0[_321].x) / (_41_m0[_321].y - _41_m0[_321].x), 0.0f, 1.0f);
                    _383 = _297 * _303;
                    _385 = _298 * _303;
                    _387 = _303 * _288;
                    _389 = _318 + 1u;
                    _391 = (_330 * _330) * (3.0f - (_330 * 2.0f));
                    _393 = _308;
                    _395 = _41_m0[_321].z;
                    _397 = 0.0f;
                }
                else
                {
                    uint4 _341 = _24[2u].Load(int3(uint2(_261, _262), 0u));
                    uint _343 = _341.x;
                    uint _344 = _341.w;
                    uint4 _350 = _24[15u].Load(int3(uint2(_261, _262), 0u));
                    uint _352 = _350.y;
                    uint _361 = ((_352 & 64u) != 0u) ? uint((_352 & 4294967167u) != 66u) : 4294967295u;
                    uint _362 = _344 & 128u;
                    uint _365 = (_362 != 0u) ? 1u : ((_343 << 7u) | _344);
                    uint4 _369 = _16.Load(_365 * 4u);
                    uint _370 = _369.x;
                    uint4 _373 = _16.Load((_365 * 4u) + 1u);
                    uint _374 = _373.x;
                    uint4 _377 = _16.Load((_365 * 4u) + 3u);
                    uint _378 = _377.x;
                    uint _381 = ((_370 & 1u) != 0u) ? 0u : 18u;
                    uint _420;
                    uint _421;
                    if (_362 == 0u)
                    {
                        _420 = (((_370 & 2097152u) != 0u) && (_361 == uint(min(int(uint(max(int(_361), int(0u)))), int(1u))))) ? (_381 | 128u) : _381;
                        _421 = _370;
                    }
                    else
                    {
                        _420 = _344;
                        _421 = _370 | ((_343 << 20u) & 134217728u);
                    }
                    uint _430 = _374 & 512u;
                    float _786;
                    float _788;
                    float _790;
                    float _792;
                    float _794;
                    float _796;
                    float _798;
                    float _800;
                    float _802;
                    float _804;
                    float _806;
                    float _808;
                    if (_430 == 0u)
                    {
                        if (!((_421 & 1u) == 0u))
                        {
                            ladder_phi_8 = false;
                            frontier_phi_8_pred = 0.0f;
                            break;
                        }
                        float _572 = asfloat(_17.Load((_378 * 115u) + 33u).x);
                        uint4 _580 = _24[2u].Load(int3(uint2(_261, _262), 0u));
                        uint _582 = _580.y;
                        uint _583 = _420 & 128u;
                        uint _774;
                        uint _775;
                        uint _776;
                        uint _777;
                        if (_583 == 0u)
                        {
                            _774 = uint(((_421 & 817889384u) | (_374 & 576u)) != 0u) | (((_421 >> 19u) & 1u) ^ 1u);
                            _775 = uint(((_421 & 17825808u) | (_374 & 520u)) != 0u);
                            _776 = uint(((_421 & 46137344u) | (_374 & 2564u)) != 0u);
                            _777 = 0u;
                        }
                        else
                        {
                            _774 = 1u;
                            _775 = _420 & 1u;
                            _776 = 1u;
                            _777 = 1u;
                        }
                        precise float _781 = float(_582 & 127u) * 0.0078740157186985015869140625f;
                        bool _785 = (_421 & 4194304u) == 0u;
                        float _1100;
                        if (_785)
                        {
                            _1100 = _781;
                        }
                        else
                        {
                            _1100 = float(_582 & 31u) * 0.0322580635547637939453125f;
                        }
                        uint _1174;
                        if ((_421 & 134217728u) == 0u)
                        {
                            uint frontier_phi_45_39_ladder;
                            if ((_583 != 0u) || ((_421 & 17825792u) == 1048576u))
                            {
                                frontier_phi_45_39_ladder = 1u;
                            }
                            else
                            {
                                frontier_phi_45_39_ladder = _775;
                            }
                            _1174 = frontier_phi_45_39_ladder;
                        }
                        else
                        {
                            _1174 = _775;
                        }
                        uint4 _1177 = _24[1u].Load(int3(uint2(_261, _262), 0u));
                        uint _1179 = _1177.x;
                        float _1246;
                        float _1248;
                        float _1250;
                        if (_774 == 0u)
                        {
                            _1246 = 0.0f;
                            _1248 = 0.0f;
                            _1250 = 0.0f;
                        }
                        else
                        {
                            float4 _1255 = _20[8u].Load(int3(uint2(_261, _262), 0u));
                            _1246 = _1255.x;
                            _1248 = _1255.y;
                            _1250 = _1255.z;
                        }
                        uint _1292;
                        if (_1174 == 0u)
                        {
                            _1292 = 0u;
                        }
                        else
                        {
                            _1292 = _24[9u].Load(int3(uint2(_261, _262), 0u)).x;
                        }
                        uint _1317;
                        if (_776 == 0u)
                        {
                            _1317 = 0u;
                        }
                        else
                        {
                            _1317 = _24[10u].Load(int3(uint2(_261, _262), 0u)).x;
                        }
                        float _1327 = (float((_1179 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1328 = (float(_1179 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1332 = (1.0f - abs(_1327)) - abs(_1328);
                        float _1334 = clamp((-0.0f) - _1332, 0.0f, 1.0f);
                        float _1335 = (-0.0f) - _1334;
                        float _1340 = ((_1327 >= 0.0f) ? _1335 : _1334) + _1327;
                        float _1341 = ((_1328 >= 0.0f) ? _1335 : _1334) + _1328;
                        float _1345 = rsqrt(dot(float3(_1340, _1341, _1332), float3(_1340, _1341, _1332)));
                        float _1346 = _1340 * _1345;
                        float _1347 = _1341 * _1345;
                        float _1348 = _1345 * _1332;
                        float _787 = float(_1179 & 255u);
                        float _1352 = ((_421 & 262144u) != 0u) ? 0.0f : 1.0f;
                        float _1489;
                        float _1490;
                        float _1491;
                        float _1492;
                        uint _1493;
                        if ((_374 & 64u) == 0u)
                        {
                            float frontier_phi_74_68_ladder;
                            float frontier_phi_74_68_ladder_1;
                            float frontier_phi_74_68_ladder_2;
                            float frontier_phi_74_68_ladder_3;
                            uint frontier_phi_74_68_ladder_4;
                            if ((_421 & 276824064u) == 0u)
                            {
                                frontier_phi_74_68_ladder = 0.0f;
                                frontier_phi_74_68_ladder_1 = ((_421 & 8u) != 0u) ? _1248 : _1352;
                                frontier_phi_74_68_ladder_2 = 0.0f;
                                frontier_phi_74_68_ladder_3 = 0.0f;
                                frontier_phi_74_68_ladder_4 = 0u;
                            }
                            else
                            {
                                frontier_phi_74_68_ladder = 0.0f;
                                frontier_phi_74_68_ladder_1 = _1352;
                                frontier_phi_74_68_ladder_2 = 0.0f;
                                frontier_phi_74_68_ladder_3 = 0.0f;
                                frontier_phi_74_68_ladder_4 = 0u;
                            }
                            _1489 = frontier_phi_74_68_ladder_1;
                            _1490 = frontier_phi_74_68_ladder;
                            _1491 = frontier_phi_74_68_ladder_2;
                            _1492 = frontier_phi_74_68_ladder_3;
                            _1493 = frontier_phi_74_68_ladder_4;
                        }
                        else
                        {
                            float _1388 = (_1248 * 2.0f) + (-1.0f);
                            float _1389 = (_1250 * 2.0f) + (-1.0f);
                            float _1393 = (1.0f - abs(_1388)) - abs(_1389);
                            float _1395 = clamp((-0.0f) - _1393, 0.0f, 1.0f);
                            float _1396 = (-0.0f) - _1395;
                            float _1401 = ((_1388 >= 0.0f) ? _1396 : _1395) + _1388;
                            float _1402 = ((_1389 >= 0.0f) ? _1396 : _1395) + _1389;
                            float _1406 = rsqrt(dot(float3(_1401, _1402, _1393), float3(_1401, _1402, _1393)));
                            _1489 = floor(round(_1246 * 255.0f) * 0.0625f) * 0.066666670143604278564453125f;
                            _1490 = _1401 * _1406;
                            _1491 = _1402 * _1406;
                            _1492 = _1406 * _1393;
                            _1493 = 1u;
                        }
                        float _795;
                        if ((_421 & 32768u) == 0u)
                        {
                            _795 = _1489;
                        }
                        else
                        {
                            float frontier_phi_77_78_ladder;
                            if (_17.Load((_378 * 115u) + 36u).x == 0u)
                            {
                                float _1799 = clamp((_787 * 0.02450981177389621734619140625f) + (-0.250000178813934326171875f), 0.0f, 1.0f) * _572;
                                frontier_phi_77_78_ladder = ((_421 & 131072u) != 0u) ? _1799 : ((((clamp((1.21000003814697265625f / (exp2((_1100 + (-0.4600000083446502685546875f)) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f) + (-1.0f)) * asfloat(_17.Load((_378 * 115u) + 32u).x)) + 1.0f) * _1799);
                            }
                            else
                            {
                                frontier_phi_77_78_ladder = _572;
                            }
                            _795 = frontier_phi_77_78_ladder;
                        }
                        uint _1560 = _420 & 1u;
                        float _1742;
                        float _1744;
                        float _1746;
                        uint _1748;
                        if (((_421 & 16u) == 0u) || (((_1560 | (_374 & 8u)) | (_421 & 16777216u)) != 0u))
                        {
                            _1742 = _1490;
                            _1744 = _1491;
                            _1746 = _1492;
                            _1748 = _1493;
                        }
                        else
                        {
                            float _1758 = (float(_1292 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1759 = (float(_1292 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                            float _1763 = (1.0f - abs(_1758)) - abs(_1759);
                            float _1765 = clamp((-0.0f) - _1763, 0.0f, 1.0f);
                            float _1766 = (-0.0f) - _1765;
                            float _1771 = ((_1758 >= 0.0f) ? _1766 : _1765) + _1758;
                            float _1772 = ((_1759 >= 0.0f) ? _1766 : _1765) + _1759;
                            float _1776 = rsqrt(dot(float3(_1771, _1772, _1763), float3(_1771, _1772, _1763)));
                            _1742 = _1771 * _1776;
                            _1744 = _1772 * _1776;
                            _1746 = _1776 * _1763;
                            _1748 = 1u;
                        }
                        float _789;
                        float _791;
                        float _793;
                        if (_1560 == 0u)
                        {
                            float frontier_phi_89_88_ladder;
                            float frontier_phi_89_88_ladder_1;
                            float frontier_phi_89_88_ladder_2;
                            if (((_420 & 64u) == 0u) && (_777 != 0u))
                            {
                                float2 _1870 = spvUnpackHalf2x16((_1317 >> 17u) & 32736u);
                                float _1871 = _1870.x;
                                float _1874 = (spvUnpackHalf2x16((_1317 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1875 = (spvUnpackHalf2x16((_1317 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _1879 = (1.0f - abs(_1874)) - abs(_1875);
                                float _1881 = clamp((-0.0f) - _1879, 0.0f, 1.0f);
                                float _1882 = (-0.0f) - _1881;
                                float _1887 = ((_1874 >= 0.0f) ? _1882 : _1881) + _1874;
                                float _1888 = ((_1875 >= 0.0f) ? _1882 : _1881) + _1875;
                                float _1892 = rsqrt(dot(float3(_1887, _1888, _1879), float3(_1887, _1888, _1879)));
                                float _1902 = (((_1887 * _1892) - _1346) * _1871) + _1346;
                                float _1903 = (((_1888 * _1892) - _1347) * _1871) + _1347;
                                float _1904 = (((_1892 * _1879) - _1348) * _1871) + _1348;
                                float _1908 = rsqrt(dot(float3(_1902, _1903, _1904), float3(_1902, _1903, _1904)));
                                frontier_phi_89_88_ladder = _1904 * _1908;
                                frontier_phi_89_88_ladder_1 = _1902 * _1908;
                                frontier_phi_89_88_ladder_2 = _1903 * _1908;
                            }
                            else
                            {
                                frontier_phi_89_88_ladder = _1348;
                                frontier_phi_89_88_ladder_1 = _1346;
                                frontier_phi_89_88_ladder_2 = _1347;
                            }
                            _789 = frontier_phi_89_88_ladder_1;
                            _791 = frontier_phi_89_88_ladder_2;
                            _793 = frontier_phi_89_88_ladder;
                        }
                        else
                        {
                            _789 = _1346;
                            _791 = _1347;
                            _793 = _1348;
                        }
                        float _803;
                        float _805;
                        float _807;
                        float _809;
                        if (_785)
                        {
                            float frontier_phi_95_94_ladder;
                            float frontier_phi_95_94_ladder_1;
                            float frontier_phi_95_94_ladder_2;
                            float frontier_phi_95_94_ladder_3;
                            if (((_421 & 33554432u) == 0u) || (((_374 & 4u) != 0u) && ((_421 & 8388608u) == 0u)))
                            {
                                frontier_phi_95_94_ladder = 0.0f;
                                frontier_phi_95_94_ladder_1 = 0.0f;
                                frontier_phi_95_94_ladder_2 = 0.0f;
                                frontier_phi_95_94_ladder_3 = 0.0f;
                            }
                            else
                            {
                                float _2020 = (spvUnpackHalf2x16((_1317 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2021 = (spvUnpackHalf2x16((_1317 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                                float _2025 = (1.0f - abs(_2020)) - abs(_2021);
                                float _2027 = clamp((-0.0f) - _2025, 0.0f, 1.0f);
                                float _2028 = (-0.0f) - _2027;
                                float _2033 = ((_2020 >= 0.0f) ? _2028 : _2027) + _2020;
                                float _2034 = ((_2021 >= 0.0f) ? _2028 : _2027) + _2021;
                                float _2038 = rsqrt(dot(float3(_2033, _2034, _2025), float3(_2033, _2034, _2025)));
                                float _2039 = _2033 * _2038;
                                float _2040 = _2034 * _2038;
                                float _2041 = _2038 * _2025;
                                float _2045 = rsqrt(dot(float3(_2039, _2040, _2041), float3(_2039, _2040, _2041)));
                                frontier_phi_95_94_ladder = _2045 * _2041;
                                frontier_phi_95_94_ladder_1 = _2045 * _2040;
                                frontier_phi_95_94_ladder_2 = _2045 * _2039;
                                frontier_phi_95_94_ladder_3 = spvUnpackHalf2x16((_1317 >> 17u) & 32736u).x;
                            }
                            _803 = frontier_phi_95_94_ladder_3;
                            _805 = frontier_phi_95_94_ladder_2;
                            _807 = frontier_phi_95_94_ladder_1;
                            _809 = frontier_phi_95_94_ladder;
                        }
                        else
                        {
                            _803 = 0.0f;
                            _805 = 0.0f;
                            _807 = 0.0f;
                            _809 = 0.0f;
                        }
                        bool _1922 = _1748 != 0u;
                        _786 = _787;
                        _788 = _789;
                        _790 = _791;
                        _792 = _793;
                        _794 = _795;
                        _796 = _1922 ? _1742 : _789;
                        _798 = _1922 ? _1744 : _791;
                        _800 = _1922 ? _1746 : _793;
                        _802 = _803;
                        _804 = _805;
                        _806 = _807;
                        _808 = _809;
                    }
                    else
                    {
                        uint4 _532 = _24[1u].Load(int3(uint2(_261, _262), 0u));
                        uint _534 = _532.x;
                        uint4 _538 = _24[9u].Load(int3(uint2(_261, _262), 0u));
                        uint _540 = _538.x;
                        float _714;
                        float _715;
                        float _716;
                        if ((_421 & 33554432u) == 0u)
                        {
                            float _592 = (float((_534 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _593 = (float(_534 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _597 = (1.0f - abs(_592)) - abs(_593);
                            float _599 = clamp((-0.0f) - _597, 0.0f, 1.0f);
                            float _600 = (-0.0f) - _599;
                            float _605 = ((_592 >= 0.0f) ? _600 : _599) + _592;
                            float _606 = ((_593 >= 0.0f) ? _600 : _599) + _593;
                            float _610 = rsqrt(dot(float3(_605, _606, _597), float3(_605, _606, _597)));
                            _714 = _605 * _610;
                            _715 = _606 * _610;
                            _716 = _610 * _597;
                        }
                        else
                        {
                            float _621 = (float((_540 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _622 = (float(_540 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _626 = (1.0f - abs(_621)) - abs(_622);
                            float _628 = clamp((-0.0f) - _626, 0.0f, 1.0f);
                            float _629 = (-0.0f) - _628;
                            float _634 = ((_621 >= 0.0f) ? _629 : _628) + _621;
                            float _635 = ((_622 >= 0.0f) ? _629 : _628) + _622;
                            float _639 = rsqrt(dot(float3(_634, _635, _626), float3(_634, _635, _626)));
                            _714 = _634 * _639;
                            _715 = _635 * _639;
                            _716 = _639 * _626;
                        }
                        _786 = float(_534 & 255u);
                        _788 = _714;
                        _790 = _715;
                        _792 = _716;
                        _794 = 1.0f;
                        _796 = _714;
                        _798 = _715;
                        _800 = _716;
                        _802 = 0.0f;
                        _804 = 0.0f;
                        _806 = 0.0f;
                        _808 = 0.0f;
                    }
                    precise float _810 = _786 * 0.0039215688593685626983642578125f;
                    if ((_420 & 144u) == 0u)
                    {
                        ladder_phi_8 = false;
                        frontier_phi_8_pred = 0.0f;
                        break;
                    }
                    bool _1109 = ((_420 & 128u) | _430) != 0u;
                    uint _1157;
                    if (_1109)
                    {
                        _1157 = 1u;
                    }
                    else
                    {
                        _1157 = (((_421 >> 14u) & 2u) ^ 2u) + 3u;
                    }
                    float _384;
                    float _386;
                    float _388;
                    uint _390;
                    float _394;
                    if (((_421 & 33554432u) == 0u) || _1109)
                    {
                        bool _1181 = _73 != 0u;
                        uint _1188;
                        if ((_421 & 16u) == 0u)
                        {
                            _1188 = _1157;
                        }
                        else
                        {
                            _1188 = ((_421 & 268435456u) != 0u) ? _1157 : 2u;
                        }
                        _394 = _794 * _810;
                        _384 = _1181 ? _796 : _788;
                        _386 = _1181 ? _798 : _790;
                        _388 = _1181 ? _800 : _792;
                        _390 = _1188;
                    }
                    else
                    {
                        _394 = _802;
                        _384 = _804;
                        _386 = _806;
                        _388 = _808;
                        _390 = _1157;
                    }
                    uint _1189 = _390 + 102u;
                    float _1198 = clamp((_394 - _41_m0[_1189].x) / (_41_m0[_1189].y - _41_m0[_1189].x), 0.0f, 1.0f);
                    _383 = _384;
                    _385 = _386;
                    _387 = _388;
                    _389 = _390;
                    _391 = (_1198 * _1198) * (3.0f - (_1198 * 2.0f));
                    _393 = _394;
                    _395 = _41_m0[_1189].z;
                    _397 = asfloat(_17.Load((_378 * 115u) + 114u).x);
                }
                if (_391 == 0.0f)
                {
                    ladder_phi_8 = false;
                    frontier_phi_8_pred = _393;
                    break;
                }
                float _440;
                if (_259)
                {
                    _440 = _258;
                }
                else
                {
                    _440 = _12.Load(int3(uint2(uint(int(_250 * float(_229))), uint(int(_251 * float(_230)))), 0u)).x;
                }
                float _442 = 1.0f - _393;
                float _443 = _442 * _442;
                float _451 = _46_m0[50u].w + _46_m0[50u].y;
                uint _454 = _389 + 63u;
                float _463 = clamp(((_46_m0[50u].x / (_451 - (_46_m0[50u].y * _440))) - _46_m0[_454].y) / (_46_m0[_454].x - _46_m0[_454].y), 0.0f, 1.0f);
                float _468 = ((_463 * _463) * _391) * (3.0f - (_463 * 2.0f));
                float _479 = ((_250 * 2.0f) * _46_m0[51u].z) + (-1.0f);
                float _480 = ((1.0f - (_46_m0[51u].w * _251)) * 2.0f) + (-1.0f);
                float _496 = mad(_140, _440, mad(_133, _480, _479 * _126)) + _147;
                float _497 = (mad(_137, _440, mad(_130, _480, _479 * _123)) + _144) / _496;
                float _498 = (mad(_138, _440, mad(_131, _480, _479 * _124)) + _145) / _496;
                float _499 = (mad(_139, _440, mad(_132, _480, _479 * _125)) + _146) / _496;
                float _503 = rsqrt(dot(float3(_497, _498, _499), float3(_497, _498, _499)));
                float _504 = _503 * _497;
                float _505 = _503 * _498;
                float _506 = _503 * _499;
                float _509 = mad(_89, _387, mad(_83, _385, _383 * _77));
                float _512 = mad(_90, _387, mad(_84, _385, _383 * _78));
                float _515 = mad(_91, _387, mad(_85, _385, _383 * _79));
                float _518 = _512 * _512;
                float _651;
                float _652;
                float _653;
                if (abs(_515) > 0.0f)
                {
                    float _553 = sqrt((_515 * _515) + _518);
                    _651 = 0.0f;
                    _652 = ((-0.0f) - _515) / _553;
                    _653 = _512 / _553;
                }
                else
                {
                    float _559 = sqrt(_518 + (_509 * _509));
                    _651 = _512 / _559;
                    _652 = ((-0.0f) - _509) / _559;
                    _653 = 0.0f;
                }
                float _656 = (_653 * _512) - (_652 * _515);
                float _659 = (_651 * _515) - (_653 * _509);
                float _662 = (_652 * _509) - (_651 * _512);
                float _663 = (-0.0f) - _504;
                float _664 = (-0.0f) - _505;
                float _665 = (-0.0f) - _506;
                float _674 = mad(_665, _515, mad(_664, _512, _509 * _663));
                float _675 = mad(_665, _653, mad(_664, _652, _651 * _663)) * _443;
                float _676 = mad(_665, _662, mad(_664, _659, _656 * _663)) * _443;
                float _680 = rsqrt(dot(float3(_675, _676, _674), float3(_675, _676, _674)));
                float _681 = _680 * _675;
                float _682 = _680 * _676;
                float _683 = _680 * _674;
                float _686 = (_681 * _681) + (_682 * _682);
                bool _687 = _686 > 0.0f;
                float _723;
                float _724;
                if (_687)
                {
                    float _719 = rsqrt(_686);
                    _723 = (-0.0f) - (_682 * _719);
                    _724 = _719 * _681;
                }
                else
                {
                    _723 = 1.0f;
                    _724 = 0.0f;
                }
                float _728 = _683 + 1.0f;
                float _730 = 1.0f - (_728 * 0.5f);
                float _731 = _730 * _683;
                float _738 = sqrt(max(0.0f, 1.0f - (_730 * _730)));
                float _745 = ((_738 * _681) - (_724 * _731)) * _443;
                float _746 = ((_738 * _682) + (_723 * _731)) * _443;
                float _747 = max(0.0f, (((_724 * _681) - (_723 * _682)) * _730) + (_738 * _683));
                float _751 = rsqrt(dot(float3(_745, _746, _747), float3(_745, _746, _747)));
                float _752 = _745 * _751;
                float _753 = _746 * _751;
                float _754 = _751 * _747;
                float _757 = mad(_754, _509, mad(_753, _656, _752 * _651));
                float _760 = mad(_754, _512, mad(_753, _659, _752 * _652));
                float _763 = mad(_754, _515, mad(_753, _662, _752 * _653));
                float _767 = dot(float3(_504, _505, _506), float3(_757, _760, _763)) * 2.0f;
                float _771 = _504 - (_767 * _757);
                float _772 = _505 - (_767 * _760);
                float _773 = _506 - (_767 * _763);
                float _818;
                float _819;
                if (_687)
                {
                    float _814 = rsqrt(_686);
                    _818 = (-0.0f) - (_682 * _814);
                    _819 = _814 * _681;
                }
                else
                {
                    _818 = 1.0f;
                    _819 = 0.0f;
                }
                float _825 = _730 + (_728 * 0.15811388194561004638671875f);
                float _830 = _825 * _683;
                float _839 = sqrt(max(0.0f, 1.0f - (_825 * _825)));
                float _846 = (((_818 * (-1.3822754496572997595649212598801e-08f)) - (_819 * _830)) + (_839 * _681)) * _443;
                float _847 = (((_818 * _830) - (_819 * 1.3822754496572997595649212598801e-08f)) + (_839 * _682)) * _443;
                float _848 = max(0.0f, (((_819 * _681) - (_818 * _682)) * _825) + (_839 * _683));
                float _852 = rsqrt(dot(float3(_846, _847, _848), float3(_846, _847, _848)));
                float _853 = _846 * _852;
                float _854 = _847 * _852;
                float _855 = _852 * _848;
                float _858 = mad(_855, _509, mad(_854, _656, _853 * _651));
                float _861 = mad(_855, _512, mad(_854, _659, _853 * _652));
                float _864 = mad(_855, _515, mad(_854, _662, _853 * _653));
                float _868 = dot(float3(_504, _505, _506), float3(_858, _861, _864)) * 2.0f;
                float _872 = _504 - (_868 * _858);
                float _873 = _505 - (_868 * _861);
                float _874 = _506 - (_868 * _864);
                float _875 = dot(float3(_872, _873, _874), float3(_771, _772, _773));
                float _882 = rsqrt(dot(float3(_757, _760, _763), float3(_757, _760, _763)));
                float _883 = _882 * _757;
                float _884 = _882 * _760;
                float _885 = _882 * _763;
                float _889 = dot(float3(_504, _505, _506), float3(_883, _884, _885)) * 2.0f;
                float _893 = _504 - (_889 * _883);
                float _894 = _505 - (_889 * _884);
                float _895 = _506 - (_889 * _885);
                float _905 = sqrt(((_498 * _498) + (_497 * _497)) + (_499 * _499)) * 0.001000000047497451305389404296875f;
                float _916 = ((_905 * _509) + _497) + (_893 * _395);
                float _917 = ((_905 * _512) + _498) + (_894 * _395);
                float _918 = ((_905 * _515) + _499) + (_895 * _395);
                float _934 = mad(_112, _918, mad(_105, _917, _916 * _98)) + _119;
                float _937 = (mad(_111, _918, mad(_104, _917, _916 * _97)) + _118) / _934;
                float _940 = (((mad(_109, _918, mad(_102, _917, _916 * _95)) + _116) / _934) * 0.5f) + 0.5f;
                float _941 = 0.5f - (((mad(_110, _918, mad(_103, _917, _916 * _96)) + _117) / _934) * 0.5f);
                float _944 = _940 * _46_m0[51u].x;
                float _945 = _941 * _46_m0[51u].y;
                float _950 = _916 + (_893 * 0.100000001490116119384765625f);
                float _951 = _917 + (_894 * 0.100000001490116119384765625f);
                float _952 = _918 + (_895 * 0.100000001490116119384765625f);
                float _968 = mad(_112, _952, mad(_105, _951, _950 * _98)) + _119;
                float _977 = _46_m0[51u].x * (((((mad(_109, _952, mad(_102, _951, _950 * _95)) + _116) / _968) * 0.5f) + 0.5f) - _940);
                float _979 = _46_m0[51u].y * ((0.5f - (((mad(_110, _952, mad(_103, _951, _950 * _96)) + _117) / _968) * 0.5f)) - _941);
                float _980 = ((mad(_111, _952, mad(_104, _951, _950 * _97)) + _118) / _968) - _937;
                float _981 = _977 * 10.0f;
                float _983 = _979 * 10.0f;
                float _984 = _980 * 10.0f;
                float _988 = 0.75f / dot(float3(_893, _894, _895), float3(_509, _512, _515));
                float _993 = (_988 * _893) + _916;
                float _994 = (_988 * _894) + _917;
                float _995 = (_988 * _895) + _918;
                float _1011 = mad(_112, _995, mad(_105, _994, _993 * _98)) + _119;
                float _1020 = _46_m0[51u].x * (((((mad(_109, _995, mad(_102, _994, _993 * _95)) + _116) / _1011) * 0.5f) + 0.5f) - _940);
                float _1022 = _46_m0[51u].y * ((0.5f - (((mad(_110, _995, mad(_103, _994, _993 * _96)) + _117) / _1011) * 0.5f)) - _941);
                float _1023 = ((mad(_111, _995, mad(_104, _994, _993 * _97)) + _118) / _1011) - _937;
                float _1036 = sqrt(((_1020 * _1020) + (_1023 * _1023)) + (_1022 * _1022)) / sqrt(((_981 * _981) + (_984 * _984)) + (_983 * _983));
                float _1037 = float(_229);
                float _1038 = float(_230);
                float _1045 = (_981 != 0.0f) ? (0.100000001490116119384765625f / _977) : 3.4028234663852885981170418348452e+38f;
                float _1047 = (_983 != 0.0f) ? (0.100000001490116119384765625f / _979) : 3.4028234663852885981170418348452e+38f;
                float _1048 = (_984 != 0.0f) ? (0.100000001490116119384765625f / _980) : 3.4028234663852885981170418348452e+38f;
                float _1049 = 1.0f / _1037;
                float _1050 = 1.0f / _1038;
                float _1051 = 0.004999999888241291046142578125f / _1037;
                float _1053 = 0.004999999888241291046142578125f / _1038;
                float _1062 = float(_981 >= 0.0f);
                float _1063 = float(_983 >= 0.0f);
                float _1072 = ((_981 < 0.0f) ? ((-0.0f) - _1051) : _1051) - _944;
                float _1075 = ((_983 < 0.0f) ? ((-0.0f) - _1053) : _1053) - _945;
                float _1078 = min((((floor(_944 * _1037) + _1062) * _1049) + _1072) * _1045, (((floor(_945 * _1038) + _1063) * _1050) + _1075) * _1047);
                float _1082 = (_1078 * _981) + _944;
                float _1083 = (_1078 * _983) + _945;
                float _1084 = (_1078 * _984) + _937;
                float _1087 = _46_m0[50u].x / (_451 - (_1084 * _46_m0[50u].y));
                float _1098 = max(0.300000011920928955078125f, 10.0f / max(0.00999999977648258209228515625f, max(abs(_977 * 38400.0f), abs(_979 * 21600.0f))));
                uint _1111;
                float _1115;
                float _1117;
                float _1119;
                uint _1131;
                uint _1143;
                uint _1113;
                float _1121;
                float _1123;
                float _1125;
                float _1127;
                float _1129;
                float _1133;
                float _1135;
                float _1137;
                float _1139;
                uint _1141;
                uint _1145;
                uint _1110 = 0u;
                uint _1112 = 0u;
                float _1114 = _1084;
                float _1116 = _1083;
                float _1118 = _1082;
                float _1120 = _1078;
                float _1122 = _1050;
                float _1124 = _1049;
                float _1126 = _1038;
                float _1128 = _1037;
                uint _1130 = 0u;
                float _1132 = _937;
                float _1134 = _945;
                float _1136 = _944;
                float _1138 = 1.0f;
                uint _1140 = 0u;
                uint _1142 = 0u;
                uint _1144 = 1u;
                float _1146;
                float _1147;
                uint _1148;
                uint _1149;
                bool _1150;
                for (;;)
                {
                    _1146 = _1128 * _1118;
                    _1147 = _1126 * _1116;
                    _1148 = uint(int(_1146));
                    _1149 = uint(int(_1147));
                    _1150 = _1130 == 0u;
                    float _1202;
                    if (_1150)
                    {
                        _1202 = _12.Load(int3(uint2(_1148, _1149), 0u)).x;
                    }
                    else
                    {
                        _1202 = _15.Load(int3(uint2(_1148, _1149), _1130 + 4294967295u)).x;
                    }
                    float _1208 = ((_1146 >= floor(_1128)) || (_1147 >= floor(_1126))) ? 1.0f : _1202;
                    float _1222 = (_984 < 0.0f) ? ((_1208 - _937) * _1048) : 3.4028234663852885981170418348452e+38f;
                    float _1224 = min(min((((floor(_1146) + _1062) * _1124) + _1072) * _1045, (((floor(_1147) + _1063) * _1122) + _1075) * _1047), _1222);
                    bool _1225 = _1208 < _1114;
                    bool _1229 = _1225 && (asuint(_1224) != asuint(_1222));
                    float _1230 = _1225 ? _1224 : _1120;
                    float _1234 = (_1230 * _981) + _944;
                    float _1235 = (_1230 * _983) + _945;
                    float _1236 = (_1230 * _984) + _937;
                    uint _1238 = (_1229 ? 1u : 4294967295u) + _1130;
                    float _1239 = _1229 ? 0.5f : 2.0f;
                    float _1240 = _1239 * _1128;
                    float _1241 = _1239 * _1126;
                    float _1242 = _1229 ? 2.0f : 0.5f;
                    float _1243 = _1242 * _1124;
                    float _1244 = _1242 * _1122;
                    _1111 = _1110 + 1u;
                    uint _1280;
                    uint _1284;
                    if (int(_1238) < int(0u))
                    {
                        uint frontier_phi_54_53_ladder;
                        uint frontier_phi_54_53_ladder_1;
                        uint frontier_phi_54_53_ladder_2;
                        uint frontier_phi_54_53_ladder_3;
                        float frontier_phi_54_53_ladder_4;
                        float frontier_phi_54_53_ladder_5;
                        float frontier_phi_54_53_ladder_6;
                        float frontier_phi_54_53_ladder_7;
                        uint frontier_phi_54_53_ladder_8;
                        float frontier_phi_54_53_ladder_9;
                        float frontier_phi_54_53_ladder_10;
                        float frontier_phi_54_53_ladder_11;
                        float frontier_phi_54_53_ladder_12;
                        float frontier_phi_54_53_ladder_13;
                        float _1269;
                        float _1274;
                        float _1277;
                        bool _1278;
                        for (;;)
                        {
                            float _1267 = _46_m0[50u].w + _46_m0[50u].y;
                            _1269 = _46_m0[50u].x / (_1267 - (_46_m0[50u].y * _1208));
                            float _1272 = _46_m0[50u].x / (_1267 - (_46_m0[50u].y * _1236));
                            _1274 = abs(_1087 - _1272);
                            _1277 = _1272 - _1269;
                            _1278 = _1277 > max(0.00999999977648258209228515625f, _1274 * 0.00999999977648258209228515625f);
                            if (_1278)
                            {
                                uint _1286;
                                if (_1112 == 0u)
                                {
                                    uint frontier_phi_64_63_ladder;
                                    if ((_389 == 2u) || (_389 == 4u))
                                    {
                                        if ((_1230 < _1036) && (abs(_1277) < 2.0f))
                                        {
                                            frontier_phi_54_53_ladder = 1u;
                                            frontier_phi_54_53_ladder_1 = _1144;
                                            frontier_phi_54_53_ladder_2 = 1u;
                                            frontier_phi_54_53_ladder_3 = 1u;
                                            frontier_phi_54_53_ladder_4 = 0.0f;
                                            frontier_phi_54_53_ladder_5 = _1136;
                                            frontier_phi_54_53_ladder_6 = _1134;
                                            frontier_phi_54_53_ladder_7 = _1132;
                                            frontier_phi_54_53_ladder_8 = _1238;
                                            frontier_phi_54_53_ladder_9 = _1240;
                                            frontier_phi_54_53_ladder_10 = _1241;
                                            frontier_phi_54_53_ladder_11 = _1243;
                                            frontier_phi_54_53_ladder_12 = _1244;
                                            frontier_phi_54_53_ladder_13 = _1230;
                                            break;
                                        }
                                        frontier_phi_64_63_ladder = 1u;
                                    }
                                    else
                                    {
                                        frontier_phi_64_63_ladder = 1u;
                                    }
                                    _1286 = frontier_phi_64_63_ladder;
                                }
                                else
                                {
                                    _1286 = _1112;
                                }
                                if (!(_1140 == 0u))
                                {
                                    frontier_phi_54_53_ladder = _1286;
                                    frontier_phi_54_53_ladder_1 = _1144;
                                    frontier_phi_54_53_ladder_2 = _1142;
                                    frontier_phi_54_53_ladder_3 = _1140;
                                    frontier_phi_54_53_ladder_4 = _1138;
                                    frontier_phi_54_53_ladder_5 = _1136;
                                    frontier_phi_54_53_ladder_6 = _1134;
                                    frontier_phi_54_53_ladder_7 = _1132;
                                    frontier_phi_54_53_ladder_8 = _1238;
                                    frontier_phi_54_53_ladder_9 = _1240;
                                    frontier_phi_54_53_ladder_10 = _1241;
                                    frontier_phi_54_53_ladder_11 = _1243;
                                    frontier_phi_54_53_ladder_12 = _1244;
                                    frontier_phi_54_53_ladder_13 = _1230;
                                    break;
                                }
                                frontier_phi_54_53_ladder = _1286;
                                frontier_phi_54_53_ladder_1 = _1144;
                                frontier_phi_54_53_ladder_2 = ((_389 == 1u) || (asuint(_46_m0[62u]).z == 0u)) ? 1u : _1142;
                                frontier_phi_54_53_ladder_3 = 0u;
                                frontier_phi_54_53_ladder_4 = _1138;
                                frontier_phi_54_53_ladder_5 = _1136;
                                frontier_phi_54_53_ladder_6 = _1134;
                                frontier_phi_54_53_ladder_7 = _1132;
                                frontier_phi_54_53_ladder_8 = 0u;
                                frontier_phi_54_53_ladder_9 = _1037;
                                frontier_phi_54_53_ladder_10 = _1038;
                                frontier_phi_54_53_ladder_11 = _1049;
                                frontier_phi_54_53_ladder_12 = _1050;
                                frontier_phi_54_53_ladder_13 = _1230 + _1098;
                                break;
                            }
                            else
                            {
                                float _1303 = max(0.100000001490116119384765625f, _1274 * 0.100000001490116119384765625f) * 0.5f;
                                float _1283 = clamp((abs(_1277) - _1303) / _1303, 0.0f, 1.0f);
                                uint _1279 = uint(_1269 < _1087);
                                uint frontier_phi_54_53_ladder_58_ladder;
                                uint frontier_phi_54_53_ladder_58_ladder_1;
                                uint frontier_phi_54_53_ladder_58_ladder_2;
                                uint frontier_phi_54_53_ladder_58_ladder_3;
                                float frontier_phi_54_53_ladder_58_ladder_4;
                                float frontier_phi_54_53_ladder_58_ladder_5;
                                float frontier_phi_54_53_ladder_58_ladder_6;
                                float frontier_phi_54_53_ladder_58_ladder_7;
                                uint frontier_phi_54_53_ladder_58_ladder_8;
                                float frontier_phi_54_53_ladder_58_ladder_9;
                                float frontier_phi_54_53_ladder_58_ladder_10;
                                float frontier_phi_54_53_ladder_58_ladder_11;
                                float frontier_phi_54_53_ladder_58_ladder_12;
                                float frontier_phi_54_53_ladder_58_ladder_13;
                                if (_1142 == 0u)
                                {
                                    frontier_phi_54_53_ladder_58_ladder = _1112;
                                    frontier_phi_54_53_ladder_58_ladder_1 = _1279;
                                    frontier_phi_54_53_ladder_58_ladder_2 = uint(_1283 > 0.0f);
                                    frontier_phi_54_53_ladder_58_ladder_3 = _1140;
                                    frontier_phi_54_53_ladder_58_ladder_4 = _1283;
                                    frontier_phi_54_53_ladder_58_ladder_5 = _1136;
                                    frontier_phi_54_53_ladder_58_ladder_6 = _1134;
                                    frontier_phi_54_53_ladder_58_ladder_7 = _1132;
                                    frontier_phi_54_53_ladder_58_ladder_8 = _1238;
                                    frontier_phi_54_53_ladder_58_ladder_9 = _1240;
                                    frontier_phi_54_53_ladder_58_ladder_10 = _1241;
                                    frontier_phi_54_53_ladder_58_ladder_11 = _1243;
                                    frontier_phi_54_53_ladder_58_ladder_12 = _1244;
                                    frontier_phi_54_53_ladder_58_ladder_13 = _1230;
                                }
                                else
                                {
                                    frontier_phi_54_53_ladder_58_ladder = _1112;
                                    frontier_phi_54_53_ladder_58_ladder_1 = _1279;
                                    frontier_phi_54_53_ladder_58_ladder_2 = _1142;
                                    frontier_phi_54_53_ladder_58_ladder_3 = _1140;
                                    frontier_phi_54_53_ladder_58_ladder_4 = _1283;
                                    frontier_phi_54_53_ladder_58_ladder_5 = _1136;
                                    frontier_phi_54_53_ladder_58_ladder_6 = _1134;
                                    frontier_phi_54_53_ladder_58_ladder_7 = _1132;
                                    frontier_phi_54_53_ladder_58_ladder_8 = _1238;
                                    frontier_phi_54_53_ladder_58_ladder_9 = _1240;
                                    frontier_phi_54_53_ladder_58_ladder_10 = _1241;
                                    frontier_phi_54_53_ladder_58_ladder_11 = _1243;
                                    frontier_phi_54_53_ladder_58_ladder_12 = _1244;
                                    frontier_phi_54_53_ladder_58_ladder_13 = _1230;
                                }
                                frontier_phi_54_53_ladder = frontier_phi_54_53_ladder_58_ladder;
                                frontier_phi_54_53_ladder_1 = frontier_phi_54_53_ladder_58_ladder_1;
                                frontier_phi_54_53_ladder_2 = frontier_phi_54_53_ladder_58_ladder_2;
                                frontier_phi_54_53_ladder_3 = frontier_phi_54_53_ladder_58_ladder_3;
                                frontier_phi_54_53_ladder_4 = frontier_phi_54_53_ladder_58_ladder_4;
                                frontier_phi_54_53_ladder_5 = frontier_phi_54_53_ladder_58_ladder_5;
                                frontier_phi_54_53_ladder_6 = frontier_phi_54_53_ladder_58_ladder_6;
                                frontier_phi_54_53_ladder_7 = frontier_phi_54_53_ladder_58_ladder_7;
                                frontier_phi_54_53_ladder_8 = frontier_phi_54_53_ladder_58_ladder_8;
                                frontier_phi_54_53_ladder_9 = frontier_phi_54_53_ladder_58_ladder_9;
                                frontier_phi_54_53_ladder_10 = frontier_phi_54_53_ladder_58_ladder_10;
                                frontier_phi_54_53_ladder_11 = frontier_phi_54_53_ladder_58_ladder_11;
                                frontier_phi_54_53_ladder_12 = frontier_phi_54_53_ladder_58_ladder_12;
                                frontier_phi_54_53_ladder_13 = frontier_phi_54_53_ladder_58_ladder_13;
                                break;
                            }
                        }
                        _1145 = frontier_phi_54_53_ladder_1;
                        _1280 = frontier_phi_54_53_ladder_2;
                        _1141 = frontier_phi_54_53_ladder_3;
                        _1139 = frontier_phi_54_53_ladder_4;
                        _1137 = frontier_phi_54_53_ladder_5;
                        _1135 = frontier_phi_54_53_ladder_6;
                        _1133 = frontier_phi_54_53_ladder_7;
                        _1284 = frontier_phi_54_53_ladder_8;
                        _1129 = frontier_phi_54_53_ladder_9;
                        _1127 = frontier_phi_54_53_ladder_10;
                        _1125 = frontier_phi_54_53_ladder_11;
                        _1123 = frontier_phi_54_53_ladder_12;
                        _1121 = frontier_phi_54_53_ladder_13;
                        _1113 = frontier_phi_54_53_ladder;
                    }
                    else
                    {
                        _1145 = _1144;
                        _1280 = _1142;
                        _1141 = _1140;
                        _1139 = _1138;
                        _1137 = _1234;
                        _1135 = _1235;
                        _1133 = _1236;
                        _1284 = _1238;
                        _1129 = _1240;
                        _1127 = _1241;
                        _1125 = _1243;
                        _1123 = _1244;
                        _1121 = _1230;
                        _1113 = _1112;
                    }
                    uint frontier_phi_67_pred;
                    uint frontier_phi_67_pred_1;
                    float frontier_phi_67_pred_2;
                    float frontier_phi_67_pred_3;
                    float frontier_phi_67_pred_4;
                    bool _1289;
                    bool _1291;
                    for (;;)
                    {
                        _1289 = _1236 < 0.0f;
                        _1291 = _1289 || ((_1234 < 0.0f) || (_1235 < 0.0f));
                        if (!_1291)
                        {
                            if (!((_1236 > 1.0f) || ((_1234 > _46_m0[51u].x) || (_1235 > _46_m0[51u].y))))
                            {
                                frontier_phi_67_pred = _1280;
                                frontier_phi_67_pred_1 = _1284;
                                frontier_phi_67_pred_2 = _1234;
                                frontier_phi_67_pred_3 = _1236;
                                frontier_phi_67_pred_4 = _1235;
                                break;
                            }
                        }
                        if (!_1289)
                        {
                            frontier_phi_67_pred = 1u;
                            frontier_phi_67_pred_1 = 4294967295u;
                            frontier_phi_67_pred_2 = _1234;
                            frontier_phi_67_pred_3 = _1236;
                            frontier_phi_67_pred_4 = _1235;
                            break;
                        }
                        float _1365 = (-0.0f) - _1236;
                        float _1366 = _1365 / _984;
                        frontier_phi_67_pred = 1u;
                        frontier_phi_67_pred_1 = 4294967295u;
                        frontier_phi_67_pred_2 = (_1366 * _981) + _1234;
                        frontier_phi_67_pred_3 = _1365 + _1236;
                        frontier_phi_67_pred_4 = (_1366 * _983) + _1235;
                        break;
                    }
                    _1143 = frontier_phi_67_pred;
                    _1131 = frontier_phi_67_pred_1;
                    _1119 = frontier_phi_67_pred_2;
                    _1115 = frontier_phi_67_pred_3;
                    _1117 = frontier_phi_67_pred_4;
                    if ((_1111 < 128u) && (int(_1131) > int(4294967295u)))
                    {
                        _1110 = _1111;
                        _1112 = _1113;
                        _1114 = _1115;
                        _1116 = _1117;
                        _1118 = _1119;
                        _1120 = _1121;
                        _1122 = _1123;
                        _1124 = _1125;
                        _1126 = _1127;
                        _1128 = _1129;
                        _1130 = _1131;
                        _1132 = _1133;
                        _1134 = _1135;
                        _1136 = _1137;
                        _1138 = _1139;
                        _1140 = _1141;
                        _1142 = _1143;
                        _1144 = _1145;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                bool _1424 = dot(float3(_893, _894, _895), float3(_504, _505, _506)) < 0.0f;
                float _1434 = _46_m0[51u].z * 2.0f;
                float _1437 = (_1434 * _944) + (-1.0f);
                float _1438 = ((1.0f - (_46_m0[51u].w * _945)) * 2.0f) + (-1.0f);
                float _1454 = mad(_186, _937, mad(_179, _1438, _1437 * _172)) + _193;
                float _1455 = (mad(_183, _937, mad(_176, _1438, _1437 * _169)) + _190) / _1454;
                float _1456 = (mad(_184, _937, mad(_177, _1438, _1437 * _170)) + _191) / _1454;
                float _1457 = (mad(_185, _937, mad(_178, _1438, _1437 * _171)) + _192) / _1454;
                float _1462 = (_1434 * _1119) + (-1.0f);
                float _1463 = ((1.0f - (_46_m0[51u].w * _1117)) * 2.0f) + (-1.0f);
                float _1479 = mad(_186, _1115, mad(_179, _1463, _1462 * _172)) + _193;
                float _1483 = ((mad(_183, _1115, mad(_176, _1463, _1462 * _169)) + _190) / _1479) - _1455;
                float _1484 = ((mad(_184, _1115, mad(_177, _1463, _1462 * _170)) + _191) / _1479) - _1456;
                float _1485 = ((mad(_185, _1115, mad(_178, _1463, _1462 * _171)) + _192) / _1479) - _1457;
                float _1500;
                if (_1111 < 129u)
                {
                    float frontier_phi_76_75_ladder;
                    if ((_1119 < 0.0f) || (_1117 < 0.0f))
                    {
                        frontier_phi_76_75_ladder = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_76_75_ladder_79_ladder;
                        if ((_1115 >= 1.0f) || ((_1119 > _46_m0[51u].x) || (_1117 > _46_m0[51u].y)))
                        {
                            frontier_phi_76_75_ladder_79_ladder = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_76_75_ladder_79_ladder_85_ladder;
                            for (;;)
                            {
                                if ((abs(_1119 - _250) < (2.0f / _1037)) && (abs(_1117 - _251) < (2.0f / _1038)))
                                {
                                    frontier_phi_76_75_ladder_79_ladder_85_ladder = 0.0f;
                                    break;
                                }
                                else
                                {
                                    bool ladder_phi_105;
                                    float frontier_phi_105_pred;
                                    uint _1843;
                                    uint _1844;
                                    bool _1846;
                                    for (;;)
                                    {
                                        _1843 = uint(int(_1119 * _1037));
                                        _1844 = uint(int(_1117 * _1038));
                                        _1846 = (_389 == 1u) && _1424;
                                        if (!_1846)
                                        {
                                            if (!(dot(float3(_1483, _1484, _1485), float3(_1483, _1484, _1485)) < 0.000899999984540045261383056640625f))
                                            {
                                                ladder_phi_105 = false;
                                                frontier_phi_105_pred = 0.0f;
                                                break;
                                            }
                                        }
                                        uint4 _1925 = _24[22u].Load(int3(uint2(_1843, _1844), 0u));
                                        uint _1927 = _1925.x;
                                        float _2220;
                                        float _2221;
                                        float _2222;
                                        if (_1927 == 0u)
                                        {
                                            uint4 _2048 = _24[1u].Load(int3(uint2(_1843, _1844), 0u));
                                            uint _2050 = _2048.x;
                                            float _2058 = (float((_2050 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2059 = (float(_2050 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2063 = (1.0f - abs(_2058)) - abs(_2059);
                                            float _2065 = clamp((-0.0f) - _2063, 0.0f, 1.0f);
                                            float _2066 = (-0.0f) - _2065;
                                            _2220 = ((_2058 >= 0.0f) ? _2066 : _2065) + _2058;
                                            _2221 = ((_2059 >= 0.0f) ? _2066 : _2065) + _2059;
                                            _2222 = _2063;
                                        }
                                        else
                                        {
                                            float _2080 = (float((_1927 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2081 = (float(_1927 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                            float _2085 = (1.0f - abs(_2080)) - abs(_2081);
                                            float _2087 = clamp((-0.0f) - _2085, 0.0f, 1.0f);
                                            float _2088 = (-0.0f) - _2087;
                                            _2220 = ((_2080 >= 0.0f) ? _2088 : _2087) + _2080;
                                            _2221 = ((_2081 >= 0.0f) ? _2088 : _2087) + _2081;
                                            _2222 = _2085;
                                        }
                                        float _2226 = rsqrt(dot(float3(_2220, _2221, _2222), float3(_2220, _2221, _2222)));
                                        if (dot(float3(_2226 * _2220, _2226 * _2221, _2226 * _2222), float3(_1483, _1484, _1485)) > 0.0f)
                                        {
                                            ladder_phi_105 = true;
                                            frontier_phi_105_pred = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            ladder_phi_105 = false;
                                            frontier_phi_105_pred = 0.0f;
                                            break;
                                        }
                                    }
                                    if (ladder_phi_105)
                                    {
                                        frontier_phi_76_75_ladder_79_ladder_85_ladder = frontier_phi_105_pred;
                                        break;
                                    }
                                    float _2099 = _46_m0[51u].z * _1119;
                                    float _2100 = _46_m0[51u].w * _1117;
                                    float _2102 = (_1038 / _1037) * 0.0500000007450580596923828125f;
                                    float _2107 = clamp(_2099 / _2102, 0.0f, 1.0f);
                                    float _2108 = clamp(_2100 * 20.0f, 0.0f, 1.0f);
                                    float _2120 = clamp(((_2099 + (-1.0f)) + _2102) / _2102, 0.0f, 1.0f);
                                    float _2121 = clamp((_2100 + (-0.949999988079071044921875f)) * 19.999996185302734375f, 0.0f, 1.0f);
                                    precise float _2134 = _2107 * _2108;
                                    precise float _2135 = _2134 * _2134;
                                    frontier_phi_76_75_ladder_79_ladder_85_ladder = (((((3.0f - (_2107 * 2.0f)) * float(_468 > 0.0f)) * (3.0f - (_2108 * 2.0f))) * _2135) * (1.0f - ((_2120 * _2120) * (3.0f - (_2120 * 2.0f))))) * (1.0f - ((_2121 * _2121) * (3.0f - (_2121 * 2.0f))));
                                    break;
                                }
                            }
                            frontier_phi_76_75_ladder_79_ladder = frontier_phi_76_75_ladder_79_ladder_85_ladder;
                        }
                        frontier_phi_76_75_ladder = frontier_phi_76_75_ladder_79_ladder;
                    }
                    _1500 = frontier_phi_76_75_ladder;
                }
                else
                {
                    _1500 = 0.0f;
                }
                float _1541 = ((((exp2(log2(clamp((sqrt(((_1456 * _1456) + (_1455 * _1455)) + (_1457 * _1457)) - _41_m0[121u].y) * _41_m0[121u].z, 0.0f, 1.0f)) * _41_m0[121u].w) * _397) * exp2(log2(clamp((_1456 - _41_m0[122u].x) * _41_m0[122u].y, 0.0f, 1.0f)) * _41_m0[122u].z)) * (1.0f - clamp(_385, 0.0f, 1.0f))) * (max(_41_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
                float _1544 = mad(_163, _895, mad(_157, _894, _893 * _151));
                float _1547 = mad(_164, _895, mad(_158, _894, _893 * _152));
                float _1550 = mad(_165, _895, mad(_159, _894, _893 * _153));
                uint4 _1553 = asuint(_51_m0[0u]);
                float _1555 = float(_1553.x);
                float _1557 = float(_1553.y);
                float _1594;
                float _1595;
                float _1596;
                if (_1500 < 1.0f)
                {
                    float _1578 = (-0.0f) - _937;
                    float _1579 = _1578 / _984;
                    float _1582 = (_1579 * _981) + _944;
                    float _1583 = (_1579 * _983) + _945;
                    float _1584 = _1578 + _937;
                    _1594 = ((_1137 - _1582) * _1500) + _1582;
                    _1595 = ((_1135 - _1583) * _1500) + _1583;
                    _1596 = ((_1133 - _1584) * _1500) + _1584;
                }
                else
                {
                    _1594 = _1137;
                    _1595 = _1135;
                    _1596 = _1133;
                }
                float _1606 = ((_1594 * 2.0f) * _46_m0[51u].z) + (-1.0f);
                float _1607 = ((1.0f - (_46_m0[51u].w * _1595)) * 2.0f) + (-1.0f);
                float _1623 = mad(_140, _1596, mad(_133, _1607, _1606 * _126)) + _147;
                float _1627 = ((mad(_137, _1596, mad(_130, _1607, _1606 * _123)) + _144) / _1623) - _916;
                float _1628 = ((mad(_138, _1596, mad(_131, _1607, _1606 * _124)) + _145) / _1623) - _917;
                float _1629 = ((mad(_139, _1596, mad(_132, _1607, _1606 * _125)) + _146) / _1623) - _918;
                float _1635 = sqrt(((_1628 * _1628) + (_1627 * _1627)) + (_1629 * _1629));
                float _1636 = _1635 * _771;
                float _1637 = _1635 * _772;
                float _1638 = _1635 * _773;
                float _1639 = _1635 * (_872 / _875);
                float _1640 = _1635 * (_873 / _875);
                float _1641 = _1635 * (_874 / _875);
                float _1645 = dot(float3(_1636, _1637, _1638), float3(_757, _760, _763)) * 2.0f;
                float _1655 = dot(float3(_1639, _1640, _1641), float3(_757, _760, _763)) * 2.0f;
                float _1682 = (_1636 - (_1645 * _757)) + _916;
                float _1683 = (_1637 - (_1645 * _760)) + _917;
                float _1684 = (_1638 - (_1645 * _763)) + _918;
                float _1696 = mad(_46_m0[24u].w, _1684, mad(_46_m0[23u].w, _1683, _1682 * _46_m0[22u].w)) + _46_m0[25u].w;
                float _1701 = (_1639 - (_1655 * _757)) + _916;
                float _1702 = (_1640 - (_1655 * _760)) + _917;
                float _1703 = (_1641 - (_1655 * _763)) + _918;
                float _1715 = mad(_46_m0[24u].w, _1703, mad(_46_m0[23u].w, _1702, _1701 * _46_m0[22u].w)) + _46_m0[25u].w;
                float _1721 = (_46_m0[51u].x * ((((mad(_46_m0[24u].x, _1684, mad(_46_m0[23u].x, _1683, _1682 * _46_m0[22u].x)) + _46_m0[25u].x) / _1696) - ((mad(_46_m0[24u].x, _1703, mad(_46_m0[23u].x, _1702, _1701 * _46_m0[22u].x)) + _46_m0[25u].x) / _1715)) * 0.5f)) * _1555;
                float _1725 = (_46_m0[51u].y * ((((mad(_46_m0[24u].y, _1703, mad(_46_m0[23u].y, _1702, _1701 * _46_m0[22u].y)) + _46_m0[25u].y) / _1715) - ((mad(_46_m0[24u].y, _1684, mad(_46_m0[23u].y, _1683, _1682 * _46_m0[22u].y)) + _46_m0[25u].y) / _1696)) * 0.5f)) * _1557;
                float _1740 = clamp(log2(sqrt((_1725 * _1725) + (_1721 * _1721)) * 2.0f) / float(asuint(_46_m0[55u]).x + 4294967295u), 0.0f, 1.0f);
                bool _1741 = _389 == 1u;
                float _1830;
                if (_1741)
                {
                    float _1819 = clamp((sqrt(((_1484 * _1484) + (_1483 * _1483)) + (_1485 * _1485)) + (-1.0f)) * 0.111111111938953399658203125f, 0.0f, 1.0f);
                    _1830 = (1.0f - ((_1819 * _1819) * (3.0f - (_1819 * 2.0f)))) * _46_m0[61u].z;
                }
                else
                {
                    _1830 = 1.0f;
                }
                float _1831 = _1830 * _1500;
                bool _1832 = _389 != 1u;
                float _2140;
                float _2142;
                float _2144;
                float _2146;
                if (_1831 == 0.0f)
                {
                    float _1953;
                    float _1954;
                    float _1955;
                    float _1956;
                    if (_1832)
                    {
                        float _1934 = _1547 * _1541;
                        float _1938 = rsqrt(dot(float3(_1544, _1934, _1550), float3(_1544, _1934, _1550)));
                        float4 _1948 = _28[4u].SampleLevel(_55, float3(_1938 * _1544, _1938 * _1934, _1938 * _1550), 0.0f);
                        _1953 = _1948.x;
                        _1954 = _1948.y;
                        _1955 = _1948.z;
                        _1956 = 1.0f;
                    }
                    else
                    {
                        _1953 = 0.0f;
                        _1954 = 0.0f;
                        _1955 = 0.0f;
                        _1956 = 0.0f;
                    }
                    _2140 = _1953 * _468;
                    _2142 = _1954 * _468;
                    _2144 = _1955 * _468;
                    _2146 = _1956 * _468;
                }
                else
                {
                    float _2197;
                    float _2199;
                    float _2204;
                    float _2208;
                    if (_12.Load(int3(uint2(uint(_1555 * _1119), uint(_1557 * _1117)), 0u)).x > 0.0f)
                    {
                        uint _1962_dummy_parameter;
                        uint2 _1962 = spvTextureSize(_14, 0u, _1962_dummy_parameter);
                        float4 _1971 = _14.Load(int3(uint2(uint(float(_1962.x) * _1119), uint(float(_1962.y) * _1117)), 0u));
                        float _1975 = _1971.x * 0.5f;
                        float _1976 = _1971.y * (-0.5f);
                        float4 _1995 = _28[7u].SampleLevel(_55, float3(_1544, _1547, _1550), 0.0f);
                        float _2193;
                        if (_1741)
                        {
                            float frontier_phi_108_107_ladder;
                            if ((_46_m0[50u].x / ((_46_m0[50u].w + _46_m0[50u].y) - (_46_m0[50u].y * _937))) < 5.0f)
                            {
                                float _2244 = sqrt((_1975 * _1975) + (_1976 * _1976));
                                float frontier_phi_108_107_ladder_114_ladder;
                                if (_2244 > 0.0500000007450580596923828125f)
                                {
                                    float _2284 = _1119 - _944;
                                    float _2285 = _1117 - _945;
                                    float frontier_phi_108_107_ladder_114_ladder_121_ladder;
                                    if (_2244 > sqrt((_2285 * _2285) + (_2284 * _2284)))
                                    {
                                        uint4 _2301 = asuint(_51_m0[0u]);
                                        uint _2308 = uint(float(_2301.x) * _1119);
                                        uint _2309 = uint(float(_2301.y) * _1117);
                                        uint4 _2312 = _24[2u].Load(int3(uint2(_2308, _2309), 0u));
                                        uint _2315 = _2312.w;
                                        uint4 _2320 = _24[15u].Load(int3(uint2(_2308, _2309), 0u));
                                        uint _2322 = _2320.y;
                                        uint _2328 = ((_2322 & 64u) != 0u) ? uint((_2322 & 4294967167u) != 66u) : 4294967295u;
                                        uint _2329 = _2315 & 128u;
                                        uint _2331 = (_2329 != 0u) ? 1u : ((_2312.x << 7u) | _2315);
                                        uint4 _2334 = _16.Load(_2331 * 4u);
                                        uint _2335 = _2334.x;
                                        uint _2342 = ((_2335 & 1u) != 0u) ? 0u : 18u;
                                        uint _2359;
                                        if (_2329 == 0u)
                                        {
                                            _2359 = (((_2335 & 2097152u) != 0u) && (_2328 == uint(min(int(uint(max(int(_2328), int(0u)))), int(1u))))) ? (_2342 | 128u) : _2342;
                                        }
                                        else
                                        {
                                            _2359 = _2315;
                                        }
                                        float frontier_phi_108_107_ladder_114_ladder_121_ladder_128_ladder;
                                        if (((_2359 & 128u) | (_16.Load((_2331 * 4u) + 1u).x & 512u)) == 0u)
                                        {
                                            uint4 _2366 = asuint(_51_m0[0u]);
                                            uint _2375 = uint(float(_2366.x) * (_1975 + _1119));
                                            uint _2376 = uint(float(_2366.y) * (_1976 + _1117));
                                            uint4 _2379 = _24[2u].Load(int3(uint2(_2375, _2376), 0u));
                                            uint _2382 = _2379.w;
                                            uint4 _2385 = _24[15u].Load(int3(uint2(_2375, _2376), 0u));
                                            uint _2387 = _2385.y;
                                            uint _2393 = ((_2387 & 64u) != 0u) ? uint((_2387 & 4294967167u) != 66u) : 4294967295u;
                                            uint _2394 = _2382 & 128u;
                                            uint _2396 = (_2394 != 0u) ? 1u : ((_2379.x << 7u) | _2382);
                                            uint4 _2398 = _16.Load(_2396 * 4u);
                                            uint _2399 = _2398.x;
                                            uint _2406 = ((_2399 & 1u) != 0u) ? 0u : 18u;
                                            uint _2416;
                                            if (_2394 == 0u)
                                            {
                                                _2416 = (((_2399 & 2097152u) != 0u) && (_2393 == uint(min(int(uint(max(int(_2393), int(0u)))), int(1u))))) ? (_2406 | 128u) : _2406;
                                            }
                                            else
                                            {
                                                _2416 = _2382;
                                            }
                                            float frontier_phi_108_107_ladder_114_ladder_121_ladder_128_ladder_131_ladder;
                                            if ((_2416 & 128u) == 0u)
                                            {
                                                frontier_phi_108_107_ladder_114_ladder_121_ladder_128_ladder_131_ladder = ((_16.Load((_2396 * 4u) + 1u).x & 512u) != 0u) ? 0.0f : 1.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_108_107_ladder_114_ladder_121_ladder_128_ladder_131_ladder = 0.0f;
                                            }
                                            frontier_phi_108_107_ladder_114_ladder_121_ladder_128_ladder = frontier_phi_108_107_ladder_114_ladder_121_ladder_128_ladder_131_ladder;
                                        }
                                        else
                                        {
                                            frontier_phi_108_107_ladder_114_ladder_121_ladder_128_ladder = 1.0f;
                                        }
                                        frontier_phi_108_107_ladder_114_ladder_121_ladder = frontier_phi_108_107_ladder_114_ladder_121_ladder_128_ladder;
                                    }
                                    else
                                    {
                                        frontier_phi_108_107_ladder_114_ladder_121_ladder = 1.0f;
                                    }
                                    frontier_phi_108_107_ladder_114_ladder = frontier_phi_108_107_ladder_114_ladder_121_ladder;
                                }
                                else
                                {
                                    frontier_phi_108_107_ladder_114_ladder = 1.0f;
                                }
                                frontier_phi_108_107_ladder = frontier_phi_108_107_ladder_114_ladder;
                            }
                            else
                            {
                                frontier_phi_108_107_ladder = 1.0f;
                            }
                            _2193 = frontier_phi_108_107_ladder;
                        }
                        else
                        {
                            _2193 = 1.0f;
                        }
                        float _2195 = _2193 * _1831;
                        float _2203;
                        float _2207;
                        float _2211;
                        if (_1141 == 0u)
                        {
                            _2203 = _46_m0[54u].x * _1995.x;
                            _2207 = _46_m0[54u].x * _1995.y;
                            _2211 = _46_m0[54u].x * _1995.z;
                        }
                        else
                        {
                            _2203 = 0.0f;
                            _2207 = 0.0f;
                            _2211 = 0.0f;
                        }
                        float frontier_phi_109_115_ladder;
                        float frontier_phi_109_115_ladder_1;
                        float frontier_phi_109_115_ladder_2;
                        float frontier_phi_109_115_ladder_3;
                        for (;;)
                        {
                            if (_1139 > 0.0f)
                            {
                                float _2202;
                                float _2206;
                                float _2210;
                                if (_1741)
                                {
                                    _2202 = 0.0f;
                                    _2206 = 0.0f;
                                    _2210 = 0.0f;
                                }
                                else
                                {
                                    if (!((_389 != 4u) || (_1145 != 0u)))
                                    {
                                        frontier_phi_109_115_ladder = _2211;
                                        frontier_phi_109_115_ladder_1 = _2207;
                                        frontier_phi_109_115_ladder_2 = _2203;
                                        frontier_phi_109_115_ladder_3 = _2195;
                                        break;
                                    }
                                    _2202 = _2203;
                                    _2206 = _2207;
                                    _2210 = _2211;
                                }
                                frontier_phi_109_115_ladder = _2210;
                                frontier_phi_109_115_ladder_1 = _2206;
                                frontier_phi_109_115_ladder_2 = _2202;
                                frontier_phi_109_115_ladder_3 = (1.0f - exp2(log2(_1139) * 3.0f)) * _2195;
                                break;
                            }
                            else
                            {
                                frontier_phi_109_115_ladder = _2211;
                                frontier_phi_109_115_ladder_1 = _2207;
                                frontier_phi_109_115_ladder_2 = _2203;
                                frontier_phi_109_115_ladder_3 = _2195;
                                break;
                            }
                        }
                        _2197 = frontier_phi_109_115_ladder_3;
                        _2199 = frontier_phi_109_115_ladder_2;
                        _2204 = frontier_phi_109_115_ladder_1;
                        _2208 = frontier_phi_109_115_ladder;
                    }
                    else
                    {
                        float frontier_phi_109_101_ladder;
                        float frontier_phi_109_101_ladder_1;
                        float frontier_phi_109_101_ladder_2;
                        float frontier_phi_109_101_ladder_3;
                        if (_1111 > 127u)
                        {
                            frontier_phi_109_101_ladder = _2201;
                            frontier_phi_109_101_ladder_1 = _2201;
                            frontier_phi_109_101_ladder_2 = _2201;
                            frontier_phi_109_101_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float4 _2218 = _28[7u].SampleLevel(_55, float3(_1544, _1547, _1550), 0.0f);
                            frontier_phi_109_101_ladder = _2218.z;
                            frontier_phi_109_101_ladder_1 = _2218.y;
                            frontier_phi_109_101_ladder_2 = _2218.x;
                            frontier_phi_109_101_ladder_3 = _1831;
                        }
                        _2197 = frontier_phi_109_101_ladder_3;
                        _2199 = frontier_phi_109_101_ladder_2;
                        _2204 = frontier_phi_109_101_ladder_1;
                        _2208 = frontier_phi_109_101_ladder;
                    }
                    float _2273;
                    float _2274;
                    float _2275;
                    float _2276;
                    if (_1832 && (_2197 < 1.0f))
                    {
                        float _2247 = _1547 * _1541;
                        float _2251 = rsqrt(dot(float3(_1544, _2247, _1550), float3(_1544, _2247, _1550)));
                        float4 _2259 = _28[4u].SampleLevel(_55, float3(_2251 * _1544, _2251 * _2247, _2251 * _1550), 0.0f);
                        float _2261 = _2259.x;
                        float _2262 = _2259.y;
                        float _2263 = _2259.z;
                        _2273 = 1.0f;
                        _2274 = ((_2199 - _2261) * _2197) + _2261;
                        _2275 = ((_2204 - _2262) * _2197) + _2262;
                        _2276 = ((_2208 - _2263) * _2197) + _2263;
                    }
                    else
                    {
                        _2273 = _2197;
                        _2274 = _2199;
                        _2275 = _2204;
                        _2276 = _2208;
                    }
                    float _2147 = _2273 * _468;
                    _2140 = _2274 * _2147;
                    _2142 = _2275 * _2147;
                    _2144 = _2276 * _2147;
                    _2146 = _2147;
                }
                float _2158 = max(_46_m0[54u].y * _20[39u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f);
                float _2160 = _2158 * _2140;
                float _2161 = _2158 * _2142;
                float _2162 = _2158 * _2144;
                float _2168 = 1.0f / max(9.9999997473787516355514526367188e-06f, max(_2160, max(_2161, _2162)) + 1.0f);
                float _2172 = min(_2168 * _2160, 0.996078431606292724609375f);
                float _2174 = min(_2168 * _2161, 0.996078431606292724609375f);
                float _2175 = min(_2168 * _2162, 0.996078431606292724609375f);
                _31[uint2(_217, _220)] = float4(_2172, _2174, _2175, _2146);
                _35[uint2(_217, _220)] = float4(_1740, _393, _2146, _1740);
                if (_224)
                {
                    uint _2234 = _217 + 1u;
                    _31[uint2(_2234, _220)] = float4(_2172, _2174, _2175, _2146);
                    _35[uint2(_2234, _220)] = float4(_1740, _393, _2146, _1740);
                }
                if (_227)
                {
                    uint _2277 = _220 + 1u;
                    _31[uint2(_217, _2277)] = float4(_2172, _2174, _2175, _2146);
                    _35[uint2(_217, _2277)] = float4(_1740, _393, _2146, _1740);
                }
                if (_228)
                {
                    uint _2291 = _217 + 1u;
                    uint _2292 = _220 + 1u;
                    _31[uint2(_2291, _2292)] = float4(_2172, _2174, _2175, _2146);
                    _35[uint2(_2291, _2292)] = float4(_1740, _393, _2146, _1740);
                }
                ladder_phi_8 = true;
                frontier_phi_8_pred = _393;
                break;
            }
            float _413 = frontier_phi_8_pred;
            if (ladder_phi_8)
            {
                break;
            }
            _31[uint2(_217, _220)] = 0.0f.xxxx;
            _35[uint2(_217, _220)] = float4(0.0f, _413, 0.0f, 0.0f);
            if (_224)
            {
                uint _433 = _217 + 1u;
                _31[uint2(_433, _220)] = 0.0f.xxxx;
                _35[uint2(_433, _220)] = float4(0.0f, _413, 0.0f, 0.0f);
            }
            if (_227)
            {
                uint _544 = _220 + 1u;
                _31[uint2(_217, _544)] = 0.0f.xxxx;
                _35[uint2(_217, _544)] = float4(0.0f, _413, 0.0f, 0.0f);
            }
            if (!_228)
            {
                break;
            }
            uint _643 = _217 + 1u;
            uint _644 = _220 + 1u;
            _31[uint2(_643, _644)] = 0.0f.xxxx;
            _35[uint2(_643, _644)] = float4(0.0f, _413, 0.0f, 0.0f);
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
