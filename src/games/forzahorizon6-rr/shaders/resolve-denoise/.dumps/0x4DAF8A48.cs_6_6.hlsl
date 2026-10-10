static float _2881;
static float _2883;

cbuffer _42_44 : register(b0, space36)
{
    float4 _44_m0[50] : packoffset(c0);
};

Buffer<uint4> _8 : register(t96, space0);
Texture2D<uint4> _11 : register(t2, space36);
Texture2D<float4> _15 : register(t3, space36);
Texture2D<float4> _16 : register(t4, space36);
Texture2D<uint4> _17 : register(t6, space36);
Texture2D<float4> _18 : register(t7, space36);
Texture2D<float4> _19 : register(t20, space36);
Texture2D<float4> _20 : register(t26, space36);
Texture2D<float4> _21 : register(t27, space36);
Texture2D<int4> _25 : register(t28, space36);
Texture2D<float4> _26 : register(t37, space36);
Texture2D<uint4> _27 : register(t47, space36);
RWBuffer<uint> _30 : register(u30, space36);
RWBuffer<uint> _31 : register(u31, space36);
RWTexture2D<float4> _34 : register(u2, space36);
RWTexture2D<float4> _35 : register(u3, space36);
RWTexture2D<int4> _38 : register(u4, space36);

static uint3 gl_WorkGroupID;
static uint3 gl_LocalInvocationID;
static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint3 gl_LocalInvocationID : SV_GroupThreadID;
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

groupshared float _48[200];
groupshared float _52[400];
groupshared float _53[400];

void comp_main()
{
    uint _77 = (gl_GlobalInvocationID.x * 5u) + gl_GlobalInvocationID.y;
    uint _78 = _77 + 37u;
    uint _83 = ((_78 >> 8u) ^ _78) + 1759714724u;
    uint _87 = ((_83 << 8u) ^ _83) * 458671337u;
    uint _96 = _77 + 38u;
    uint _100 = ((_96 >> 8u) ^ _96) + 1759714724u;
    uint _103 = ((_100 << 8u) ^ _100) * 458671337u;
    uint _115 = uint(((float((_87 & 16777215u) ^ (_87 >> 8u)) * 5.9604644775390625e-08f) + float(gl_GlobalInvocationID.x)) * _44_m0[25u].x);
    uint _116 = uint(((float((_103 & 16777215u) ^ (_103 >> 8u)) * 5.9604644775390625e-08f) + float(gl_GlobalInvocationID.y)) * _44_m0[25u].y);
    uint _119 = (gl_LocalInvocationID.y << 3u) + gl_LocalInvocationID.x;
    if (_119 < 50u)
    {
        uint _129 = _119 << 1u;
        uint _130 = _129 % 10u;
        uint _132 = _129 / 10u;
        uint _133 = _130 + (gl_WorkGroupID.x << 3u);
        uint _137 = ((gl_WorkGroupID.y << 3u) + 4294967295u) + _132;
        uint4 _142 = asuint(_44_m0[14u]);
        uint _150 = uint(max(int(0u), int(uint(min(int(_142.x + 4294967295u), int(_133 + 4294967295u))))));
        uint _151 = uint(max(int(0u), int(uint(min(int(_142.y + 4294967295u), int(_137))))));
        uint _153 = (_150 * 5u) + _151;
        uint _154 = _153 + 37u;
        uint _157 = ((_154 >> 8u) ^ _154) + 1759714724u;
        uint _160 = ((_157 << 8u) ^ _157) * 458671337u;
        uint _166 = _153 + 38u;
        uint _169 = ((_166 >> 8u) ^ _166) + 1759714724u;
        uint _172 = ((_169 << 8u) ^ _169) * 458671337u;
        float4 _187 = _26.Load(int3(uint2(_150, _151), 0u));
        float _190 = _187.x;
        float _191 = _187.y;
        _48[0u + ((_132 + (_130 * 10u)) * 2u)] = _190;
        _48[1u + ((_132 + (_130 * 10u)) * 2u)] = _191;
        float4 _205 = _19.Load(int3(uint2(_150, _151), 0u));
        float _207 = _205.x;
        float _208 = _205.y;
        float _209 = _205.z;
        float _210 = _205.w;
        uint _211 = asuint(_190);
        float _222 = (float((_211 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _224 = (float(_211 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _229 = (1.0f - abs(_222)) - abs(_224);
        float _233 = clamp((-0.0f) - _229, 0.0f, 1.0f);
        float _234 = (-0.0f) - _233;
        float _239 = ((_222 >= 0.0f) ? _234 : _233) + _222;
        float _240 = ((_224 >= 0.0f) ? _234 : _233) + _224;
        float _245 = rsqrt(dot(float3(_239, _240, _229), float3(_239, _240, _229)));
        uint4 _252 = asuint(_44_m0[22u]);
        uint _253 = _252.x;
        uint _255 = _253 * 3039394381u;
        uint _259 = ((_255 >> 8u) ^ _255) + 1759714724u;
        uint _262 = ((_259 << 8u) ^ _259) * 458671337u;
        float4 _275 = _18.Load(int3(uint2(((_150 + 5u) + ((_262 >> 8u) ^ _262)) & 255u, ((_151 + 11u) + (_253 * 31060957u)) & 255u), 0u));
        float _291 = 1.0f - (((_275.x * 255.0f) + (_275.y * 0.99609375f)) * 0.0078125f);
        float _295 = sqrt(max(1.0f - (_291 * _291), 0.0f));
        float _296 = ((_275.z * 255.0f) + (_275.w * 0.99609375f)) * 0.02454369328916072845458984375f;
        float _300 = (cos(_296) * _295) + (_239 * _245);
        float _303 = (sin(_296) * _295) + (_240 * _245);
        float _304 = _291 + (_245 * _229);
        float _308 = rsqrt(dot(float3(_300, _303, _304), float3(_300, _303, _304)));
        float _309 = _308 * _300;
        float _310 = _303 * _308;
        float _311 = _304 * _308;
        uint4 _315 = asuint(_44_m0[17u]);
        float _337 = (((((float(uint(((float((_160 & 16777215u) ^ (_160 >> 8u)) * 5.9604644775390625e-08f) + float(_150)) * _44_m0[25u].x)) + 0.5f) / float(_315.x)) * 2.0f) + (-1.0f)) * _191) / _44_m0[0u].x;
        float _339 = ((1.0f - (((float(uint(((float((_172 & 16777215u) ^ (_172 >> 8u)) * 5.9604644775390625e-08f) + float(_151)) * _44_m0[25u].y)) + 0.5f) / float(_315.y)) * 2.0f)) * _191) / _44_m0[0u].y;
        float _371 = _44_m0[2u].x + mad(_44_m0[6u].x, _191, mad(_44_m0[5u].x, _339, _44_m0[4u].x * _337));
        float _372 = _44_m0[2u].y + mad(_44_m0[6u].y, _191, mad(_44_m0[5u].y, _339, _44_m0[4u].y * _337));
        float _373 = _44_m0[2u].z + mad(_44_m0[6u].z, _191, mad(_44_m0[5u].z, _339, _44_m0[4u].z * _337));
        bool _374 = _210 > 0.0f;
        float _421;
        if (_374)
        {
            _421 = _210;
        }
        else
        {
            _421 = _44_m0[28u].w;
        }
        float _643;
        float _644;
        float _645;
        uint _498;
        uint _499;
        uint _503;
        bool _504;
        for (;;)
        {
            float _445 = floor(min(max(log2(((_421 > 0.0f) ? _421 : _44_m0[28u].w) / (_44_m0[23u].w * 2.0f)), 0.0f), 8.0f));
            float _447 = exp2(_445) * _44_m0[23u].w;
            uint _467 = (uint(((_371 + (_421 * _309)) / _447) + 60000.0f) * 3039394381u) + uint(((_372 + (_421 * _310)) / _447) + 60000.0f);
            uint _470 = ((_467 >> 8u) ^ _467) + 1759714724u;
            uint _473 = ((_470 << 8u) ^ _470) * 458671337u;
            uint _477 = (((_473 >> 8u) ^ _473) * 3039394381u) + uint(((_373 + (_421 * _311)) / _447) + 60000.0f);
            uint _480 = ((_477 >> 8u) ^ _477) + 1759714724u;
            uint _483 = ((_480 << 8u) ^ _480) * 458671337u;
            uint _490 = (((_483 >> 8u) ^ _483) * 3039394381u) + (((uint(_309 > 0.0f) | ((_310 > 0.0f) ? 2u : 0u)) | ((_311 > 0.0f) ? 4u : 0u)) | (uint(_445) << 3u));
            uint _493 = ((_490 >> 8u) ^ _490) + 1759714724u;
            uint _496 = ((_493 << 8u) ^ _493) * 458671337u;
            _498 = (_496 >> 8u) ^ _496;
            _499 = _498 & 1048575u;
            _503 = _30[_499].xxxx.x;
            _504 = _503 == _498;
            uint _526;
            if (_504)
            {
                _526 = _499;
            }
            else
            {
                if (_503 == 4294967295u)
                {
                    _643 = -1.0f;
                    _644 = 0.0f;
                    _645 = 0.0f;
                    break;
                }
                uint _648 = _498 + 1u;
                uint _527 = _648 & 1048575u;
                uint4 _649 = _30[_527].xxxx;
                uint _650 = _649.x;
                uint frontier_phi_7_12_ladder;
                if (_650 == _648)
                {
                    frontier_phi_7_12_ladder = _527;
                }
                else
                {
                    if (_650 == 4294967295u)
                    {
                        _643 = -1.0f;
                        _644 = 0.0f;
                        _645 = 0.0f;
                        break;
                    }
                    uint _1138 = _498 + 2u;
                    uint _528 = _1138 & 1048575u;
                    uint4 _1139 = _30[_528].xxxx;
                    uint _1140 = _1139.x;
                    uint frontier_phi_7_12_ladder_22_ladder;
                    if (_1140 == _1138)
                    {
                        frontier_phi_7_12_ladder_22_ladder = _528;
                    }
                    else
                    {
                        if (_1140 == 4294967295u)
                        {
                            _643 = -1.0f;
                            _644 = 0.0f;
                            _645 = 0.0f;
                            break;
                        }
                        uint _1289 = _498 + 3u;
                        uint _529 = _1289 & 1048575u;
                        uint4 _1290 = _30[_529].xxxx;
                        uint _1291 = _1290.x;
                        uint frontier_phi_7_12_ladder_22_ladder_36_ladder;
                        if (_1291 == _1289)
                        {
                            frontier_phi_7_12_ladder_22_ladder_36_ladder = _529;
                        }
                        else
                        {
                            if (_1291 == 4294967295u)
                            {
                                _643 = -1.0f;
                                _644 = 0.0f;
                                _645 = 0.0f;
                                break;
                            }
                            uint _1499 = _498 + 4u;
                            uint _530 = _1499 & 1048575u;
                            if (!(_30[_530].xxxx.x == _1499))
                            {
                                _643 = -1.0f;
                                _644 = 0.0f;
                                _645 = 0.0f;
                                break;
                            }
                            frontier_phi_7_12_ladder_22_ladder_36_ladder = _530;
                        }
                        frontier_phi_7_12_ladder_22_ladder = frontier_phi_7_12_ladder_22_ladder_36_ladder;
                    }
                    frontier_phi_7_12_ladder = frontier_phi_7_12_ladder_22_ladder;
                }
                _526 = frontier_phi_7_12_ladder;
            }
            uint _531 = _526 << 2u;
            float _535 = float(_31[_531].xxxx.x);
            _643 = (float(_31[_531 | 1u].xxxx.x) * 0.000244140625f) / _535;
            _644 = (float(_31[_531 | 2u].xxxx.x) * 0.000244140625f) / _535;
            _645 = (float(_31[_531 | 3u].xxxx.x) * 0.000244140625f) / _535;
            break;
        }
        float _700;
        float _701;
        float _702;
        if (_374 && (_643 > 0.0f))
        {
            float _690 = clamp(_421, 0.0f, 1.0f);
            _700 = (_690 * (_643 - _207)) + _207;
            _701 = (_690 * (_644 - _208)) + _208;
            _702 = (_690 * (_645 - _209)) + _209;
        }
        else
        {
            _700 = _207;
            _701 = _208;
            _702 = _209;
        }
        _52[0u + ((_132 + (_130 * 10u)) * 4u)] = _700;
        _52[1u + ((_132 + (_130 * 10u)) * 4u)] = _701;
        _52[2u + ((_132 + (_130 * 10u)) * 4u)] = _702;
        _52[3u + ((_132 + (_130 * 10u)) * 4u)] = _210;
        _53[0u + ((_132 + (_130 * 10u)) * 4u)] = _309;
        _53[1u + ((_132 + (_130 * 10u)) * 4u)] = _310;
        _53[2u + ((_132 + (_130 * 10u)) * 4u)] = _311;
        _53[3u + ((_132 + (_130 * 10u)) * 4u)] = 0.0f;
        uint _743 = _130 + 1u;
        uint4 _746 = asuint(_44_m0[14u]);
        uint _753 = uint(max(int(0u), int(uint(min(int(_746.x + 4294967295u), int(_133))))));
        uint _754 = uint(max(int(0u), int(uint(min(int(_746.y + 4294967295u), int(_137))))));
        uint _760 = (_753 * 5u) + _754;
        uint _761 = _760 + 37u;
        uint _764 = ((_761 >> 8u) ^ _761) + 1759714724u;
        uint _767 = ((_764 << 8u) ^ _764) * 458671337u;
        uint _773 = _760 + 38u;
        uint _776 = ((_773 >> 8u) ^ _773) + 1759714724u;
        uint _779 = ((_776 << 8u) ^ _776) * 458671337u;
        float4 _794 = _26.Load(int3(uint2(_753, _754), 0u));
        float _796 = _794.x;
        float _797 = _794.y;
        _48[0u + ((_132 + (_743 * 10u)) * 2u)] = _796;
        _48[1u + ((_132 + (_743 * 10u)) * 2u)] = _797;
        float4 _809 = _19.Load(int3(uint2(_753, _754), 0u));
        float _811 = _809.x;
        float _812 = _809.y;
        float _813 = _809.z;
        float _814 = _809.w;
        uint _815 = asuint(_796);
        float _823 = (float((_815 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _824 = (float(_815 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _828 = (1.0f - abs(_823)) - abs(_824);
        float _830 = clamp((-0.0f) - _828, 0.0f, 1.0f);
        float _831 = (-0.0f) - _830;
        float _836 = ((_823 >= 0.0f) ? _831 : _830) + _823;
        float _837 = ((_824 >= 0.0f) ? _831 : _830) + _824;
        float _841 = rsqrt(dot(float3(_836, _837, _828), float3(_836, _837, _828)));
        uint4 _847 = asuint(_44_m0[22u]);
        uint _848 = _847.x;
        uint _850 = _848 * 3039394381u;
        uint _853 = ((_850 >> 8u) ^ _850) + 1759714724u;
        uint _856 = ((_853 << 8u) ^ _853) * 458671337u;
        float4 _866 = _18.Load(int3(uint2(((_753 + 5u) + ((_856 >> 8u) ^ _856)) & 255u, ((_754 + 11u) + (_848 * 31060957u)) & 255u), 0u));
        float _879 = 1.0f - (((_866.x * 255.0f) + (_866.y * 0.99609375f)) * 0.0078125f);
        float _883 = sqrt(max(1.0f - (_879 * _879), 0.0f));
        float _884 = ((_866.z * 255.0f) + (_866.w * 0.99609375f)) * 0.02454369328916072845458984375f;
        float _887 = (cos(_884) * _883) + (_836 * _841);
        float _890 = (sin(_884) * _883) + (_837 * _841);
        float _891 = _879 + (_841 * _828);
        float _895 = rsqrt(dot(float3(_887, _890, _891), float3(_887, _890, _891)));
        float _896 = _895 * _887;
        float _897 = _890 * _895;
        float _898 = _891 * _895;
        uint4 _901 = asuint(_44_m0[17u]);
        float _921 = (((((float(uint(((float((_767 & 16777215u) ^ (_767 >> 8u)) * 5.9604644775390625e-08f) + float(_753)) * _44_m0[25u].x)) + 0.5f) / float(_901.x)) * 2.0f) + (-1.0f)) * _797) / _44_m0[0u].x;
        float _923 = ((1.0f - (((float(uint(((float((_779 & 16777215u) ^ (_779 >> 8u)) * 5.9604644775390625e-08f) + float(_754)) * _44_m0[25u].y)) + 0.5f) / float(_901.y)) * 2.0f)) * _797) / _44_m0[0u].y;
        float _953 = _44_m0[2u].x + mad(_44_m0[6u].x, _797, mad(_44_m0[5u].x, _923, _44_m0[4u].x * _921));
        float _954 = _44_m0[2u].y + mad(_44_m0[6u].y, _797, mad(_44_m0[5u].y, _923, _44_m0[4u].y * _921));
        float _955 = _44_m0[2u].z + mad(_44_m0[6u].z, _797, mad(_44_m0[5u].z, _923, _44_m0[4u].z * _921));
        bool _956 = _814 > 0.0f;
        float _1057;
        if (_956)
        {
            _1057 = _814;
        }
        else
        {
            _1057 = _44_m0[28u].w;
        }
        float _1280;
        float _1281;
        float _1282;
        uint _1130;
        uint _1131;
        uint _1134;
        bool _1135;
        for (;;)
        {
            float _1078 = floor(min(max(log2(((_1057 > 0.0f) ? _1057 : _44_m0[28u].w) / (_44_m0[23u].w * 2.0f)), 0.0f), 8.0f));
            float _1080 = exp2(_1078) * _44_m0[23u].w;
            uint _1099 = (uint(((_953 + (_1057 * _896)) / _1080) + 60000.0f) * 3039394381u) + uint(((_954 + (_1057 * _897)) / _1080) + 60000.0f);
            uint _1102 = ((_1099 >> 8u) ^ _1099) + 1759714724u;
            uint _1105 = ((_1102 << 8u) ^ _1102) * 458671337u;
            uint _1109 = (((_1105 >> 8u) ^ _1105) * 3039394381u) + uint(((_955 + (_1057 * _898)) / _1080) + 60000.0f);
            uint _1112 = ((_1109 >> 8u) ^ _1109) + 1759714724u;
            uint _1115 = ((_1112 << 8u) ^ _1112) * 458671337u;
            uint _1122 = (((_1115 >> 8u) ^ _1115) * 3039394381u) + (((uint(_896 > 0.0f) | ((_897 > 0.0f) ? 2u : 0u)) | ((_898 > 0.0f) ? 4u : 0u)) | (uint(_1078) << 3u));
            uint _1125 = ((_1122 >> 8u) ^ _1122) + 1759714724u;
            uint _1128 = ((_1125 << 8u) ^ _1125) * 458671337u;
            _1130 = (_1128 >> 8u) ^ _1128;
            _1131 = _1130 & 1048575u;
            _1134 = _30[_1131].xxxx.x;
            _1135 = _1134 == _1130;
            uint _1148;
            if (_1135)
            {
                _1148 = _1131;
            }
            else
            {
                if (_1134 == 4294967295u)
                {
                    _1280 = -1.0f;
                    _1281 = 0.0f;
                    _1282 = 0.0f;
                    break;
                }
                uint _1285 = _1130 + 1u;
                uint _1149 = _1285 & 1048575u;
                uint4 _1286 = _30[_1149].xxxx;
                uint _1287 = _1286.x;
                uint frontier_phi_27_35_ladder;
                if (_1287 == _1285)
                {
                    frontier_phi_27_35_ladder = _1149;
                }
                else
                {
                    if (_1287 == 4294967295u)
                    {
                        _1280 = -1.0f;
                        _1281 = 0.0f;
                        _1282 = 0.0f;
                        break;
                    }
                    uint _1495 = _1130 + 2u;
                    uint _1150 = _1495 & 1048575u;
                    uint4 _1496 = _30[_1150].xxxx;
                    uint _1497 = _1496.x;
                    uint frontier_phi_27_35_ladder_49_ladder;
                    if (_1497 == _1495)
                    {
                        frontier_phi_27_35_ladder_49_ladder = _1150;
                    }
                    else
                    {
                        if (_1497 == 4294967295u)
                        {
                            _1280 = -1.0f;
                            _1281 = 0.0f;
                            _1282 = 0.0f;
                            break;
                        }
                        uint _1622 = _1130 + 3u;
                        uint _1151 = _1622 & 1048575u;
                        uint4 _1623 = _30[_1151].xxxx;
                        uint _1624 = _1623.x;
                        uint frontier_phi_27_35_ladder_49_ladder_60_ladder;
                        if (_1624 == _1622)
                        {
                            frontier_phi_27_35_ladder_49_ladder_60_ladder = _1151;
                        }
                        else
                        {
                            if (_1624 == 4294967295u)
                            {
                                _1280 = -1.0f;
                                _1281 = 0.0f;
                                _1282 = 0.0f;
                                break;
                            }
                            uint _1674 = _1130 + 4u;
                            uint _1152 = _1674 & 1048575u;
                            if (!(_30[_1152].xxxx.x == _1674))
                            {
                                _1280 = -1.0f;
                                _1281 = 0.0f;
                                _1282 = 0.0f;
                                break;
                            }
                            frontier_phi_27_35_ladder_49_ladder_60_ladder = _1152;
                        }
                        frontier_phi_27_35_ladder_49_ladder = frontier_phi_27_35_ladder_49_ladder_60_ladder;
                    }
                    frontier_phi_27_35_ladder = frontier_phi_27_35_ladder_49_ladder;
                }
                _1148 = frontier_phi_27_35_ladder;
            }
            uint _1153 = _1148 << 2u;
            float _1157 = float(_31[_1153].xxxx.x);
            _1280 = (float(_31[_1153 | 1u].xxxx.x) * 0.000244140625f) / _1157;
            _1281 = (float(_31[_1153 | 2u].xxxx.x) * 0.000244140625f) / _1157;
            _1282 = (float(_31[_1153 | 3u].xxxx.x) * 0.000244140625f) / _1157;
            break;
        }
        float _1339;
        float _1340;
        float _1341;
        if (_956 && (_1280 > 0.0f))
        {
            float _1329 = clamp(_1057, 0.0f, 1.0f);
            _1339 = (_1329 * (_1280 - _811)) + _811;
            _1340 = (_1329 * (_1281 - _812)) + _812;
            _1341 = (_1329 * (_1282 - _813)) + _813;
        }
        else
        {
            _1339 = _811;
            _1340 = _812;
            _1341 = _813;
        }
        _52[0u + ((_132 + (_743 * 10u)) * 4u)] = _1339;
        _52[1u + ((_132 + (_743 * 10u)) * 4u)] = _1340;
        _52[2u + ((_132 + (_743 * 10u)) * 4u)] = _1341;
        _52[3u + ((_132 + (_743 * 10u)) * 4u)] = _814;
        _53[0u + ((_132 + (_743 * 10u)) * 4u)] = _896;
        _53[1u + ((_132 + (_743 * 10u)) * 4u)] = _897;
        _53[2u + ((_132 + (_743 * 10u)) * 4u)] = _898;
        _53[3u + ((_132 + (_743 * 10u)) * 4u)] = 0.0f;
    }
    uint _376;
    uint _377;
    float _389;
    float _417;
    float _418;
    float _419;
    bool _420;
    for (;;)
    {
        GroupMemoryBarrierWithGroupSync();
        _376 = gl_LocalInvocationID.x + 1u;
        _377 = gl_LocalInvocationID.y + 1u;
        _389 = _48[1u + ((_377 + (_376 * 10u)) * 2u)];
        uint _390 = asuint(_48[0u + ((_377 + (_376 * 10u)) * 2u)]);
        float _398 = (float((_390 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _399 = (float(_390 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _403 = (1.0f - abs(_398)) - abs(_399);
        float _405 = clamp((-0.0f) - _403, 0.0f, 1.0f);
        float _406 = (-0.0f) - _405;
        float _411 = ((_398 >= 0.0f) ? _406 : _405) + _398;
        float _412 = ((_399 >= 0.0f) ? _406 : _405) + _399;
        float _416 = rsqrt(dot(float3(_411, _412, _403), float3(_411, _412, _403)));
        _417 = _411 * _416;
        _418 = _412 * _416;
        _419 = _416 * _403;
        _420 = _389 > 0.0f;
        if (_420)
        {
            uint4 _509 = asuint(_44_m0[14u]);
            if (!((gl_GlobalInvocationID.x >= _509.x) || (gl_GlobalInvocationID.y >= _509.y)))
            {
                uint4 _558 = asuint(_44_m0[17u]);
                float _578 = (((((float(int(_115)) + 0.5f) / float(_558.x)) * 2.0f) + (-1.0f)) * _389) / _44_m0[0u].x;
                float _580 = ((1.0f - (((float(int(_116)) + 0.5f) / float(_558.y)) * 2.0f)) * _389) / _44_m0[0u].y;
                uint4 _606 = _17.Load(int3(uint2(_115, _116), 0u));
                uint _609 = _606.w;
                uint4 _614 = _27.Load(int3(uint2(_115, _116), 0u));
                uint _616 = _614.y;
                uint _625 = ((_616 & 64u) != 0u) ? uint((_616 & 4294967167u) != 66u) : 4294967295u;
                uint _626 = _609 & 128u;
                uint _629 = (_626 != 0u) ? 1u : ((_606.x << 7u) | _609);
                uint4 _632 = _8.Load(_629 * 4u);
                uint _633 = _632.x;
                uint _640 = ((_633 & 1u) != 0u) ? 0u : 18u;
                uint _661;
                if (_626 == 0u)
                {
                    _661 = (((_633 & 2097152u) != 0u) && (_625 == uint(min(int(uint(max(int(_625), int(0u)))), int(1u))))) ? (_640 | 128u) : _640;
                }
                else
                {
                    _661 = _609;
                }
                bool _666 = ((_661 & 128u) | (_8.Load((_629 * 4u) + 1u).x & 512u)) == 0u;
                float _672 = _666 ? _44_m0[28u].x : _44_m0[28u].y;
                bool _678 = (asuint(_44_m0[22u]).y & 1u) != 0u;
                if (((_633 & 32770u) == 32768u) && (asuint(_44_m0[49u]).x != 0u))
                {
                    float4 _959 = _19.Load(int3(uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), 0u));
                    float _961 = _959.x;
                    float _964 = _959.w;
                    bool _965 = _964 > 0.0f;
                    float _1180;
                    if (_678)
                    {
                        float frontier_phi_31_23_ladder;
                        if (_965)
                        {
                            frontier_phi_31_23_ladder = clamp(_964 / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_31_23_ladder = 1.0f;
                        }
                        _1180 = frontier_phi_31_23_ladder;
                    }
                    else
                    {
                        _1180 = (_965 && (_964 < _672)) ? 0.0f : 1.0f;
                    }
                    float _1181 = dot(float3(_961, _959.yz), 1.0f.xxx);
                    float _1187 = clamp(_1181 * 0.03125f, 0.0f, 1.0f) * 32.0f;
                    float _1299;
                    float _1300;
                    float _1301;
                    if (_1181 > 0.0f)
                    {
                        _1299 = (_1187 * _961) / _1181;
                        _1300 = (_1187 * _959.y) / _1181;
                        _1301 = (_1187 * _959.z) / _1181;
                    }
                    else
                    {
                        _1299 = 0.0f;
                        _1300 = 0.0f;
                        _1301 = 0.0f;
                    }
                    float _1306 = dot(float3(0.5f, 0.0f, -0.5f), float3(_1299, _1300, _1301));
                    float _1310 = dot(float3(-0.25f, 0.5f, -0.25f), float3(_1299, _1300, _1301));
                    float _1314 = dot(float3(0.25f, 0.5f, 0.25f), float3(_1299, _1300, _1301)) * 0.5f;
                    float _1315 = _1314 * _417;
                    float _1316 = _1314 * _418;
                    float _1317 = _1314 * _419;
                    _38[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = int4(uint4(1u, 1u, 1u, 1u));
                    float _1384;
                    float _1386;
                    float _1388;
                    if ((asuint(_1306) & 2139095040u) == 2139095040u)
                    {
                        _1384 = 0.0f;
                        _1386 = 0.0f;
                        _1388 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_45_46_ladder;
                        float frontier_phi_45_46_ladder_1;
                        float frontier_phi_45_46_ladder_2;
                        if ((asuint(_1310) & 2139095040u) == 2139095040u)
                        {
                            frontier_phi_45_46_ladder = 0.0f;
                            frontier_phi_45_46_ladder_1 = 0.0f;
                            frontier_phi_45_46_ladder_2 = 0.0f;
                        }
                        else
                        {
                            bool _1519 = (asuint(_1180) & 2139095040u) == 2139095040u;
                            frontier_phi_45_46_ladder = _1519 ? 0.0f : _1180;
                            frontier_phi_45_46_ladder_1 = _1519 ? 0.0f : _1310;
                            frontier_phi_45_46_ladder_2 = _1519 ? 0.0f : _1306;
                        }
                        _1384 = frontier_phi_45_46_ladder_2;
                        _1386 = frontier_phi_45_46_ladder_1;
                        _1388 = frontier_phi_45_46_ladder;
                    }
                    _34[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = float4(_1384, _1386, 0.0f, _1388);
                    float _1503;
                    float _1505;
                    float _1507;
                    float _1509;
                    if ((asuint(_1315) & 2139095040u) == 2139095040u)
                    {
                        _1503 = 0.0f;
                        _1505 = 0.0f;
                        _1507 = 0.0f;
                        _1509 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_51_52_ladder;
                        float frontier_phi_51_52_ladder_1;
                        float frontier_phi_51_52_ladder_2;
                        float frontier_phi_51_52_ladder_3;
                        if ((asuint(_1316) & 2139095040u) == 2139095040u)
                        {
                            frontier_phi_51_52_ladder = 0.0f;
                            frontier_phi_51_52_ladder_1 = 0.0f;
                            frontier_phi_51_52_ladder_2 = 0.0f;
                            frontier_phi_51_52_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_51_52_ladder_57_ladder;
                            float frontier_phi_51_52_ladder_57_ladder_1;
                            float frontier_phi_51_52_ladder_57_ladder_2;
                            float frontier_phi_51_52_ladder_57_ladder_3;
                            if ((asuint(_1317) & 2139095040u) == 2139095040u)
                            {
                                frontier_phi_51_52_ladder_57_ladder = 0.0f;
                                frontier_phi_51_52_ladder_57_ladder_1 = 0.0f;
                                frontier_phi_51_52_ladder_57_ladder_2 = 0.0f;
                                frontier_phi_51_52_ladder_57_ladder_3 = 0.0f;
                            }
                            else
                            {
                                bool _1628 = (asuint(_1314) & 2139095040u) == 2139095040u;
                                frontier_phi_51_52_ladder_57_ladder = _1628 ? 0.0f : _1315;
                                frontier_phi_51_52_ladder_57_ladder_1 = _1628 ? 0.0f : _1314;
                                frontier_phi_51_52_ladder_57_ladder_2 = _1628 ? 0.0f : _1317;
                                frontier_phi_51_52_ladder_57_ladder_3 = _1628 ? 0.0f : _1316;
                            }
                            frontier_phi_51_52_ladder = frontier_phi_51_52_ladder_57_ladder;
                            frontier_phi_51_52_ladder_1 = frontier_phi_51_52_ladder_57_ladder_1;
                            frontier_phi_51_52_ladder_2 = frontier_phi_51_52_ladder_57_ladder_2;
                            frontier_phi_51_52_ladder_3 = frontier_phi_51_52_ladder_57_ladder_3;
                        }
                        _1503 = frontier_phi_51_52_ladder;
                        _1505 = frontier_phi_51_52_ladder_3;
                        _1507 = frontier_phi_51_52_ladder_2;
                        _1509 = frontier_phi_51_52_ladder_1;
                    }
                    _35[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = float4(_1503, _1505, _1507, _1509);
                    break;
                }
                else
                {
                    float _970 = clamp((-0.0f) - dot(float3(_44_m0[6u].xyz), float3(_417, _418, _419)), 0.0f, 1.0f);
                    uint _980 = 1u + ((gl_LocalInvocationID.y + (gl_LocalInvocationID.x * 10u)) * 2u);
                    uint _983 = asuint(_48[0u + ((gl_LocalInvocationID.y + (gl_LocalInvocationID.x * 10u)) * 2u)]);
                    float _991 = (float((_983 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _992 = (float(_983 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _996 = (1.0f - abs(_991)) - abs(_992);
                    float _998 = clamp((-0.0f) - _996, 0.0f, 1.0f);
                    float _999 = (-0.0f) - _998;
                    float _1004 = ((_991 >= 0.0f) ? _999 : _998) + _991;
                    float _1005 = ((_992 >= 0.0f) ? _999 : _998) + _992;
                    float _1009 = rsqrt(dot(float3(_1004, _1005, _996), float3(_1004, _1005, _996)));
                    float _1019 = (-0.0f) - _970;
                    float _1031 = (_48[_980] <= 0.0f) ? 0.0f : ((clamp(dot(float3(_417, _418, _419), float3(_1004 * _1009, _1005 * _1009, _1009 * _996)), 0.0f, 1.0f) * 0.3333333432674407958984375f) * clamp(exp2((abs(_389 - _48[_980]) * _1019) / max(0.001000000047497451305389404296875f, _48[_980] + _389)), 0.0f, 1.0f));
                    uint _1053 = 3u + ((gl_LocalInvocationID.y + (gl_LocalInvocationID.x * 10u)) * 4u);
                    bool _1056 = _52[_1053] > 0.0f;
                    float _1192;
                    if (_678)
                    {
                        float frontier_phi_33_25_ladder;
                        if (_1056)
                        {
                            frontier_phi_33_25_ladder = clamp(_52[_1053] / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_33_25_ladder = 1.0f;
                        }
                        _1192 = frontier_phi_33_25_ladder;
                    }
                    else
                    {
                        _1192 = (_1056 && (_52[_1053] < _672)) ? 0.0f : 1.0f;
                    }
                    uint _1206 = 1u + ((_377 + (gl_LocalInvocationID.x * 10u)) * 2u);
                    uint _1209 = asuint(_48[0u + ((_377 + (gl_LocalInvocationID.x * 10u)) * 2u)]);
                    float _1217 = (float((_1209 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1218 = (float(_1209 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1222 = (1.0f - abs(_1217)) - abs(_1218);
                    float _1224 = clamp((-0.0f) - _1222, 0.0f, 1.0f);
                    float _1225 = (-0.0f) - _1224;
                    float _1230 = ((_1217 >= 0.0f) ? _1225 : _1224) + _1217;
                    float _1231 = ((_1218 >= 0.0f) ? _1225 : _1224) + _1218;
                    float _1235 = rsqrt(dot(float3(_1230, _1231, _1222), float3(_1230, _1231, _1222)));
                    float _1254 = (_48[_1206] <= 0.0f) ? 0.0f : ((clamp(dot(float3(_417, _418, _419), float3(_1230 * _1235, _1231 * _1235, _1235 * _1222)), 0.0f, 1.0f) * 0.5f) * clamp(exp2((abs(_389 - _48[_1206]) * _1019) / max(0.001000000047497451305389404296875f, _48[_1206] + _389)), 0.0f, 1.0f));
                    uint _1276 = 3u + ((_377 + (gl_LocalInvocationID.x * 10u)) * 4u);
                    bool _1279 = _52[_1276] > 0.0f;
                    float _1401;
                    if (_678)
                    {
                        float frontier_phi_48_39_ladder;
                        if (_1279)
                        {
                            frontier_phi_48_39_ladder = clamp(_52[_1276] / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_48_39_ladder = 1.0f;
                        }
                        _1401 = frontier_phi_48_39_ladder;
                    }
                    else
                    {
                        _1401 = (_1279 && (_52[_1276] < _672)) ? 0.0f : 1.0f;
                    }
                    uint _1411 = gl_LocalInvocationID.y + 2u;
                    uint _1421 = 1u + ((_1411 + (gl_LocalInvocationID.x * 10u)) * 2u);
                    uint _1424 = asuint(_48[0u + ((_1411 + (gl_LocalInvocationID.x * 10u)) * 2u)]);
                    float _1432 = (float((_1424 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1433 = (float(_1424 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1437 = (1.0f - abs(_1432)) - abs(_1433);
                    float _1439 = clamp((-0.0f) - _1437, 0.0f, 1.0f);
                    float _1440 = (-0.0f) - _1439;
                    float _1445 = ((_1432 >= 0.0f) ? _1440 : _1439) + _1432;
                    float _1446 = ((_1433 >= 0.0f) ? _1440 : _1439) + _1433;
                    float _1450 = rsqrt(dot(float3(_1445, _1446, _1437), float3(_1445, _1446, _1437)));
                    float _1469 = (_48[_1421] <= 0.0f) ? 0.0f : ((clamp(dot(float3(_417, _418, _419), float3(_1445 * _1450, _1446 * _1450, _1450 * _1437)), 0.0f, 1.0f) * 0.3333333432674407958984375f) * clamp(exp2((abs(_389 - _48[_1421]) * _1019) / max(0.001000000047497451305389404296875f, _48[_1421] + _389)), 0.0f, 1.0f));
                    uint _1491 = 3u + ((_1411 + (gl_LocalInvocationID.x * 10u)) * 4u);
                    bool _1494 = _52[_1491] > 0.0f;
                    float _1529;
                    if (_678)
                    {
                        float frontier_phi_59_54_ladder;
                        if (_1494)
                        {
                            frontier_phi_59_54_ladder = clamp(_52[_1491] / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_59_54_ladder = 1.0f;
                        }
                        _1529 = frontier_phi_59_54_ladder;
                    }
                    else
                    {
                        _1529 = (_1494 && (_52[_1491] < _672)) ? 0.0f : 1.0f;
                    }
                    uint _1548 = 1u + ((gl_LocalInvocationID.y + (_376 * 10u)) * 2u);
                    uint _1551 = asuint(_48[0u + ((gl_LocalInvocationID.y + (_376 * 10u)) * 2u)]);
                    float _1559 = (float((_1551 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1560 = (float(_1551 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1564 = (1.0f - abs(_1559)) - abs(_1560);
                    float _1566 = clamp((-0.0f) - _1564, 0.0f, 1.0f);
                    float _1567 = (-0.0f) - _1566;
                    float _1572 = ((_1559 >= 0.0f) ? _1567 : _1566) + _1559;
                    float _1573 = ((_1560 >= 0.0f) ? _1567 : _1566) + _1560;
                    float _1577 = rsqrt(dot(float3(_1572, _1573, _1564), float3(_1572, _1573, _1564)));
                    float _1596 = (_48[_1548] <= 0.0f) ? 0.0f : ((clamp(dot(float3(_417, _418, _419), float3(_1572 * _1577, _1573 * _1577, _1577 * _1564)), 0.0f, 1.0f) * 0.5f) * clamp(exp2((abs(_389 - _48[_1548]) * _1019) / max(0.001000000047497451305389404296875f, _48[_1548] + _389)), 0.0f, 1.0f));
                    uint _1618 = 3u + ((gl_LocalInvocationID.y + (_376 * 10u)) * 4u);
                    bool _1621 = _52[_1618] > 0.0f;
                    float _1635;
                    if (_678)
                    {
                        float frontier_phi_66_62_ladder;
                        if (_1621)
                        {
                            frontier_phi_66_62_ladder = clamp(_52[_1618] / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_66_62_ladder = 1.0f;
                        }
                        _1635 = frontier_phi_66_62_ladder;
                    }
                    else
                    {
                        _1635 = (_1621 && (_52[_1618] < _672)) ? 0.0f : 1.0f;
                    }
                    float _1648 = clamp(dot(float3(_417, _418, _419), float3(_417, _418, _419)), 0.0f, 1.0f);
                    uint _1670 = 3u + ((_377 + (_376 * 10u)) * 4u);
                    bool _1673 = _52[_1670] > 0.0f;
                    float _1683;
                    if (_678)
                    {
                        float frontier_phi_71_68_ladder;
                        if (_1673)
                        {
                            frontier_phi_71_68_ladder = clamp(_52[_1670] / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_71_68_ladder = 1.0f;
                        }
                        _1683 = frontier_phi_71_68_ladder;
                    }
                    else
                    {
                        _1683 = (_1673 && (_52[_1670] < _672)) ? 0.0f : 1.0f;
                    }
                    uint _1702 = 1u + ((_1411 + (_376 * 10u)) * 2u);
                    uint _1705 = asuint(_48[0u + ((_1411 + (_376 * 10u)) * 2u)]);
                    float _1713 = (float((_1705 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1714 = (float(_1705 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1718 = (1.0f - abs(_1713)) - abs(_1714);
                    float _1720 = clamp((-0.0f) - _1718, 0.0f, 1.0f);
                    float _1721 = (-0.0f) - _1720;
                    float _1726 = ((_1713 >= 0.0f) ? _1721 : _1720) + _1713;
                    float _1727 = ((_1714 >= 0.0f) ? _1721 : _1720) + _1714;
                    float _1731 = rsqrt(dot(float3(_1726, _1727, _1718), float3(_1726, _1727, _1718)));
                    float _1750 = (_48[_1702] <= 0.0f) ? 0.0f : ((clamp(dot(float3(_417, _418, _419), float3(_1726 * _1731, _1727 * _1731, _1731 * _1718)), 0.0f, 1.0f) * 0.5f) * clamp(exp2((abs(_389 - _48[_1702]) * _1019) / max(0.001000000047497451305389404296875f, _48[_1702] + _389)), 0.0f, 1.0f));
                    uint _1772 = 3u + ((_1411 + (_376 * 10u)) * 4u);
                    bool _1775 = _52[_1772] > 0.0f;
                    float _1781;
                    if (_678)
                    {
                        float frontier_phi_75_72_ladder;
                        if (_1775)
                        {
                            frontier_phi_75_72_ladder = clamp(_52[_1772] / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_75_72_ladder = 1.0f;
                        }
                        _1781 = frontier_phi_75_72_ladder;
                    }
                    else
                    {
                        _1781 = (_1775 && (_52[_1772] < _672)) ? 0.0f : 1.0f;
                    }
                    uint _1791 = gl_LocalInvocationID.x + 2u;
                    uint _1801 = 1u + ((gl_LocalInvocationID.y + (_1791 * 10u)) * 2u);
                    uint _1804 = asuint(_48[0u + ((gl_LocalInvocationID.y + (_1791 * 10u)) * 2u)]);
                    float _1812 = (float((_1804 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1813 = (float(_1804 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1817 = (1.0f - abs(_1812)) - abs(_1813);
                    float _1819 = clamp((-0.0f) - _1817, 0.0f, 1.0f);
                    float _1820 = (-0.0f) - _1819;
                    float _1825 = ((_1812 >= 0.0f) ? _1820 : _1819) + _1812;
                    float _1826 = ((_1813 >= 0.0f) ? _1820 : _1819) + _1813;
                    float _1830 = rsqrt(dot(float3(_1825, _1826, _1817), float3(_1825, _1826, _1817)));
                    float _1849 = (_48[_1801] <= 0.0f) ? 0.0f : ((clamp(dot(float3(_417, _418, _419), float3(_1825 * _1830, _1826 * _1830, _1830 * _1817)), 0.0f, 1.0f) * 0.3333333432674407958984375f) * clamp(exp2((abs(_389 - _48[_1801]) * _1019) / max(0.001000000047497451305389404296875f, _48[_1801] + _389)), 0.0f, 1.0f));
                    uint _1871 = 3u + ((gl_LocalInvocationID.y + (_1791 * 10u)) * 4u);
                    bool _1874 = _52[_1871] > 0.0f;
                    float _1880;
                    if (_678)
                    {
                        float frontier_phi_79_76_ladder;
                        if (_1874)
                        {
                            frontier_phi_79_76_ladder = clamp(_52[_1871] / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_79_76_ladder = 1.0f;
                        }
                        _1880 = frontier_phi_79_76_ladder;
                    }
                    else
                    {
                        _1880 = (_1874 && (_52[_1871] < _672)) ? 0.0f : 1.0f;
                    }
                    uint _1899 = 1u + ((_377 + (_1791 * 10u)) * 2u);
                    uint _1902 = asuint(_48[0u + ((_377 + (_1791 * 10u)) * 2u)]);
                    float _1910 = (float((_1902 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1911 = (float(_1902 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1915 = (1.0f - abs(_1910)) - abs(_1911);
                    float _1917 = clamp((-0.0f) - _1915, 0.0f, 1.0f);
                    float _1918 = (-0.0f) - _1917;
                    float _1923 = ((_1910 >= 0.0f) ? _1918 : _1917) + _1910;
                    float _1924 = ((_1911 >= 0.0f) ? _1918 : _1917) + _1911;
                    float _1928 = rsqrt(dot(float3(_1923, _1924, _1915), float3(_1923, _1924, _1915)));
                    float _1947 = (_48[_1899] <= 0.0f) ? 0.0f : ((clamp(dot(float3(_417, _418, _419), float3(_1923 * _1928, _1924 * _1928, _1928 * _1915)), 0.0f, 1.0f) * 0.5f) * clamp(exp2((abs(_389 - _48[_1899]) * _1019) / max(0.001000000047497451305389404296875f, _48[_1899] + _389)), 0.0f, 1.0f));
                    uint _1969 = 3u + ((_377 + (_1791 * 10u)) * 4u);
                    bool _1972 = _52[_1969] > 0.0f;
                    float _1978;
                    if (_678)
                    {
                        float frontier_phi_83_80_ladder;
                        if (_1972)
                        {
                            frontier_phi_83_80_ladder = clamp(_52[_1969] / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_83_80_ladder = 1.0f;
                        }
                        _1978 = frontier_phi_83_80_ladder;
                    }
                    else
                    {
                        _1978 = (_1972 && (_52[_1969] < _672)) ? 0.0f : 1.0f;
                    }
                    float _1983 = (((((((_1254 * _52[0u + ((_377 + (gl_LocalInvocationID.x * 10u)) * 4u)]) + (_1031 * _52[0u + ((gl_LocalInvocationID.y + (gl_LocalInvocationID.x * 10u)) * 4u)])) + (_1469 * _52[0u + ((_1411 + (gl_LocalInvocationID.x * 10u)) * 4u)])) + (_1596 * _52[0u + ((gl_LocalInvocationID.y + (_376 * 10u)) * 4u)])) + (_52[0u + ((_377 + (_376 * 10u)) * 4u)] * _1648)) + (_1750 * _52[0u + ((_1411 + (_376 * 10u)) * 4u)])) + (_1849 * _52[0u + ((gl_LocalInvocationID.y + (_1791 * 10u)) * 4u)])) + (_1947 * _52[0u + ((_377 + (_1791 * 10u)) * 4u)]);
                    float _1984 = (((((((_1254 * _52[1u + ((_377 + (gl_LocalInvocationID.x * 10u)) * 4u)]) + (_1031 * _52[1u + ((gl_LocalInvocationID.y + (gl_LocalInvocationID.x * 10u)) * 4u)])) + (_1469 * _52[1u + ((_1411 + (gl_LocalInvocationID.x * 10u)) * 4u)])) + (_1596 * _52[1u + ((gl_LocalInvocationID.y + (_376 * 10u)) * 4u)])) + (_52[1u + ((_377 + (_376 * 10u)) * 4u)] * _1648)) + (_1750 * _52[1u + ((_1411 + (_376 * 10u)) * 4u)])) + (_1849 * _52[1u + ((gl_LocalInvocationID.y + (_1791 * 10u)) * 4u)])) + (_1947 * _52[1u + ((_377 + (_1791 * 10u)) * 4u)]);
                    float _1985 = (((((((_52[2u + ((_377 + (gl_LocalInvocationID.x * 10u)) * 4u)] * _1254) + (_52[2u + ((gl_LocalInvocationID.y + (gl_LocalInvocationID.x * 10u)) * 4u)] * _1031)) + (_52[2u + ((_1411 + (gl_LocalInvocationID.x * 10u)) * 4u)] * _1469)) + (_52[2u + ((gl_LocalInvocationID.y + (_376 * 10u)) * 4u)] * _1596)) + (_52[2u + ((_377 + (_376 * 10u)) * 4u)] * _1648)) + (_52[2u + ((_1411 + (_376 * 10u)) * 4u)] * _1750)) + (_52[2u + ((gl_LocalInvocationID.y + (_1791 * 10u)) * 4u)] * _1849)) + (_52[2u + ((_377 + (_1791 * 10u)) * 4u)] * _1947);
                    uint _1997 = 1u + ((_1411 + (_1791 * 10u)) * 2u);
                    uint _2000 = asuint(_48[0u + ((_1411 + (_1791 * 10u)) * 2u)]);
                    float _2008 = (float((_2000 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _2009 = (float(_2000 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _2013 = (1.0f - abs(_2008)) - abs(_2009);
                    float _2015 = clamp((-0.0f) - _2013, 0.0f, 1.0f);
                    float _2016 = (-0.0f) - _2015;
                    float _2021 = ((_2008 >= 0.0f) ? _2016 : _2015) + _2008;
                    float _2022 = ((_2009 >= 0.0f) ? _2016 : _2015) + _2009;
                    float _2026 = rsqrt(dot(float3(_2021, _2022, _2013), float3(_2021, _2022, _2013)));
                    float _2045 = (_48[_1997] <= 0.0f) ? 0.0f : ((clamp(dot(float3(_417, _418, _419), float3(_2021 * _2026, _2022 * _2026, _2026 * _2013)), 0.0f, 1.0f) * 0.3333333432674407958984375f) * clamp(exp2((abs(_389 - _48[_1997]) * _1019) / max(0.001000000047497451305389404296875f, _48[_1997] + _389)), 0.0f, 1.0f));
                    uint _2067 = 3u + ((_1411 + (_1791 * 10u)) * 4u);
                    bool _2070 = _52[_2067] > 0.0f;
                    float _2076;
                    if (_678)
                    {
                        float frontier_phi_87_84_ladder;
                        if (_2070)
                        {
                            frontier_phi_87_84_ladder = clamp(_52[_2067] / _672, 0.0f, 1.0f);
                        }
                        else
                        {
                            frontier_phi_87_84_ladder = 1.0f;
                        }
                        _2076 = frontier_phi_87_84_ladder;
                    }
                    else
                    {
                        _2076 = (_2070 && (_52[_2067] < _672)) ? 0.0f : 1.0f;
                    }
                    float _2077 = (((((((_1254 + _1031) + _1469) + _1596) + _1648) + _1750) + _1849) + _1947) + _2045;
                    float _2091;
                    float _2092;
                    float _2093;
                    float _2094;
                    if (_2077 > 0.0f)
                    {
                        _2091 = (_1983 + (_2045 * _52[0u + ((_1411 + (_1791 * 10u)) * 4u)])) / _2077;
                        _2092 = (_1984 + (_2045 * _52[1u + ((_1411 + (_1791 * 10u)) * 4u)])) / _2077;
                        _2093 = (_1985 + (_52[2u + ((_1411 + (_1791 * 10u)) * 4u)] * _2045)) / _2077;
                        _2094 = (((((((((_1401 * _1254) + (_1192 * _1031)) + (_1529 * _1469)) + (_1635 * _1596)) + (_1683 * _1648)) + (_1781 * _1750)) + (_1880 * _1849)) + (_1978 * _1947)) + (_2076 * _2045)) / _2077;
                    }
                    else
                    {
                        _2091 = 0.0f;
                        _2092 = 0.0f;
                        _2093 = 0.0f;
                        _2094 = 1.0f;
                    }
                    float _2095 = dot(float3(_2091, _2092, _2093), 1.0f.xxx);
                    float _2100 = clamp(_2095 * 0.03125f, 0.0f, 1.0f) * 32.0f;
                    float _2108;
                    float _2109;
                    float _2110;
                    if (_2095 > 0.0f)
                    {
                        _2108 = (_2100 * _2091) / _2095;
                        _2109 = (_2100 * _2092) / _2095;
                        _2110 = (_2100 * _2093) / _2095;
                    }
                    else
                    {
                        _2108 = 0.0f;
                        _2109 = 0.0f;
                        _2110 = 0.0f;
                    }
                    float _2114 = dot(float3(0.5f, 0.0f, -0.5f), float3(_2108, _2109, _2110));
                    float _2117 = dot(float3(-0.25f, 0.5f, -0.25f), float3(_2108, _2109, _2110));
                    float _2120 = dot(float3(0.25f, 0.5f, 0.25f), float3(_2108, _2109, _2110)) * 0.5f;
                    float _2121 = _2120 * _417;
                    float _2122 = _2120 * _418;
                    float _2123 = _2120 * _419;
                    float _2126 = abs(_2094 - (_2094 * _2094));
                    float4 _2128 = _16.Load(int3(uint2(_115, _116), 0u));
                    float _2130 = _2128.x;
                    float _2131 = _2128.y;
                    float _2133 = _2128.w;
                    float _2141;
                    if ((_44_m0[47u].y <= 9.9999997473787516355514526367188e-05f) || (!_666))
                    {
                        _2141 = 0.0f;
                    }
                    else
                    {
                        float _2291 = clamp((sqrt(((_578 * _578) + (_389 * _389)) + (_580 * _580)) - _44_m0[47u].w) / (_44_m0[47u].z - _44_m0[47u].w), 0.0f, 1.0f);
                        float _2296 = (_2291 * _2291) * (3.0f - (_2291 * 2.0f));
                        float frontier_phi_92_93_ladder;
                        if (_2296 > 0.0f)
                        {
                            float _2350 = clamp((_418 - _44_m0[48u].x) / (_44_m0[48u].y - _44_m0[48u].x), 0.0f, 1.0f);
                            float _2354 = (_2350 * _2350) * (3.0f - (_2350 * 2.0f));
                            float frontier_phi_92_93_ladder_96_ladder;
                            if ((_2133 == 0.0f) && (_2354 > 0.0f))
                            {
                                float _2420 = (_2130 * 1023.0f) + (-512.0f);
                                float _2421 = (_2131 * 1023.0f) + (-512.0f);
                                float _2422 = abs(_2420);
                                float _2423 = abs(_2421);
                                float _2438 = asfloat(asuint((_2422 * _2422) * 0.0009765625f) | (asuint(_2420) & 2147483648u)) * 0.00039062500582076609134674072265625f;
                                float _2439 = asfloat(asuint((_2423 * _2423) * 0.0009765625f) | (asuint(_2421) & 2147483648u)) * 0.0006944444612599909305572509765625f;
                                float _2450 = clamp(((sqrt((_2439 * _2439) + (_2438 * _2438)) / _44_m0[47u].y) - _44_m0[48u].z) / (_44_m0[48u].w - _44_m0[48u].z), 0.0f, 1.0f);
                                frontier_phi_92_93_ladder_96_ladder = ((_2450 * _2450) * (_2354 * _2296)) * (3.0f - (_2450 * 2.0f));
                            }
                            else
                            {
                                frontier_phi_92_93_ladder_96_ladder = 0.0f;
                            }
                            frontier_phi_92_93_ladder = frontier_phi_92_93_ladder_96_ladder;
                        }
                        else
                        {
                            frontier_phi_92_93_ladder = 0.0f;
                        }
                        _2141 = frontier_phi_92_93_ladder;
                    }
                    float _2143 = 1.0f - _2141;
                    uint4 _2152 = asuint(_44_m0[18u]);
                    uint4 _2157 = asuint(_44_m0[17u]);
                    float _2192 = (_44_m0[0u].z + (_44_m0[0u].w / _389)) - ((_2128.z + (-0.500488758087158203125f)) * 0.03125f);
                    float _2196 = (_2130 * 1023.0f) + (-512.0f);
                    float _2198 = (_2131 * 1023.0f) + (-512.0f);
                    float _2199 = abs(_2196);
                    float _2200 = abs(_2198);
                    float _2221 = _44_m0[12u].x + mad(_44_m0[6u].x, _389, mad(_44_m0[5u].x, _580, _44_m0[4u].x * _578));
                    float _2222 = _44_m0[12u].y + mad(_44_m0[6u].y, _389, mad(_44_m0[5u].y, _580, _44_m0[4u].y * _578));
                    float _2223 = _44_m0[12u].z + mad(_44_m0[6u].z, _389, mad(_44_m0[5u].z, _580, _44_m0[4u].z * _578));
                    float _2232 = mad(_2223, _44_m0[10u].z, mad(_2222, _44_m0[10u].y, _2221 * _44_m0[10u].x));
                    float _2239 = float(_2152.x);
                    float _2240 = float(_2152.y);
                    float _2249 = float(int(uint(int(floor((_2239 * 0.5f) * (((mad(_2223, _44_m0[8u].z, mad(_2222, _44_m0[8u].y, _2221 * _44_m0[8u].x)) * _44_m0[1u].x) / _2232) + 1.0f))))));
                    float _2250 = float(int(uint(int(floor((_2240 * 0.5f) * (1.0f - ((mad(_2223, _44_m0[9u].z, mad(_2222, _44_m0[9u].y, _2221 * _44_m0[9u].x)) * _44_m0[1u].y) / _2232)))))));
                    float _2263 = float(_2133 == 0.0f);
                    float _2274 = ((floor((((float(_115) + 0.5f) / float(_2157.x)) - (asfloat(asuint((_2199 * _2199) * 0.0009765625f) | (asuint(_2196) & 2147483648u)) * 0.00039062500582076609134674072265625f)) * _2239) - _2249) * _2263) + _2249;
                    float _2275 = ((floor((((float(_116) + 0.5f) / float(_2157.y)) - (asfloat(asuint((_2200 * _2200) * 0.0009765625f) | (asuint(_2198) & 2147483648u)) * 0.0006944444612599909305572509765625f)) * _2240) - _2250) * _2263) + _2250;
                    float _2278 = ((((_2192 <= 0.0f) ? 0.0f : (_44_m0[1u].w / (_2192 - _44_m0[1u].z))) - _2232) * _2263) + _2232;
                    float _2301;
                    float _2303;
                    float _2305;
                    float _2307;
                    float _2309;
                    float _2311;
                    float _2313;
                    float _2315;
                    float _2317;
                    if (_2278 > 0.0f)
                    {
                        float frontier_phi_95_94_ladder;
                        float frontier_phi_95_94_ladder_1;
                        float frontier_phi_95_94_ladder_2;
                        float frontier_phi_95_94_ladder_3;
                        float frontier_phi_95_94_ladder_4;
                        float frontier_phi_95_94_ladder_5;
                        float frontier_phi_95_94_ladder_6;
                        float frontier_phi_95_94_ladder_7;
                        float frontier_phi_95_94_ladder_8;
                        if ((_2274 > 0.0f) && (_2275 > 0.0f))
                        {
                            float frontier_phi_95_94_ladder_97_ladder;
                            float frontier_phi_95_94_ladder_97_ladder_1;
                            float frontier_phi_95_94_ladder_97_ladder_2;
                            float frontier_phi_95_94_ladder_97_ladder_3;
                            float frontier_phi_95_94_ladder_97_ladder_4;
                            float frontier_phi_95_94_ladder_97_ladder_5;
                            float frontier_phi_95_94_ladder_97_ladder_6;
                            float frontier_phi_95_94_ladder_97_ladder_7;
                            float frontier_phi_95_94_ladder_97_ladder_8;
                            if ((_2274 < _2239) && (_2275 < _2240))
                            {
                                uint _2456 = uint(_2274);
                                uint _2457 = uint(_2275);
                                uint4 _2459 = _11.Load(int3(uint2(_2456, _2457), 0u));
                                uint _2461 = _2459.x;
                                float _2469 = (float((_2461 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _2470 = (float(_2461 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _2474 = (1.0f - abs(_2469)) - abs(_2470);
                                float _2476 = clamp((-0.0f) - _2474, 0.0f, 1.0f);
                                float _2477 = (-0.0f) - _2476;
                                float _2482 = ((_2469 >= 0.0f) ? _2477 : _2476) + _2469;
                                float _2483 = ((_2470 >= 0.0f) ? _2477 : _2476) + _2470;
                                float _2487 = rsqrt(dot(float3(_2482, _2483, _2474), float3(_2482, _2483, _2474)));
                                float _2494 = clamp(dot(float3(_2482 * _2487, _2483 * _2487, _2487 * _2474), float3(_417, _418, _419)), 0.0f, 1.0f);
                                float4 _2498 = _15.Load(int3(uint2(_2456, _2457), 0u));
                                float _2500 = _2498.x;
                                float _2504 = (_2500 <= 0.0f) ? 0.0f : (_44_m0[1u].w / (_2500 - _44_m0[1u].z));
                                float _2512 = clamp(exp2(((_970 * (-16.0f)) * abs(_2504 - _2278)) / _2504), 0.0f, 1.0f);
                                bool _2513 = _2512 > 0.0f;
                                float frontier_phi_95_94_ladder_97_ladder_101_ladder;
                                float frontier_phi_95_94_ladder_97_ladder_101_ladder_1;
                                float frontier_phi_95_94_ladder_97_ladder_101_ladder_2;
                                float frontier_phi_95_94_ladder_97_ladder_101_ladder_3;
                                float frontier_phi_95_94_ladder_97_ladder_101_ladder_4;
                                float frontier_phi_95_94_ladder_97_ladder_101_ladder_5;
                                float frontier_phi_95_94_ladder_97_ladder_101_ladder_6;
                                float frontier_phi_95_94_ladder_97_ladder_101_ladder_7;
                                float frontier_phi_95_94_ladder_97_ladder_101_ladder_8;
                                if ((_2513 ? _2504 : 0.0f) > 0.0f)
                                {
                                    uint4 _2539 = asuint(_44_m0[20u]);
                                    uint4 _2549 = asuint(_44_m0[15u]);
                                    uint _2560 = uint(max(0.0f, min(float(_2549.x + 4294967295u), _2274 / float(_2539.x))));
                                    uint _2561 = uint(max(0.0f, min(float(_2549.y + 4294967295u), _2275 / float(_2539.y))));
                                    float4 _2563 = _20.Load(int3(uint2(_2560, _2561), 0u));
                                    float _2567 = _2563.z;
                                    float _2568 = _2563.w;
                                    float4 _2570 = _21.Load(int3(uint2(_2560, _2561), 0u));
                                    float _2607;
                                    if (_666)
                                    {
                                        _2607 = 1.0f - (clamp(_418 * 2.0f, 0.0f, 1.0f) * clamp((abs(_2568 - _2094) / (((_2567 + _2126) * 2.0f) + 0.100000001490116119384765625f)) + (-0.0625f), 0.0f, 1.0f));
                                    }
                                    else
                                    {
                                        _2607 = 1.0f;
                                    }
                                    float _2624 = float(min((asuint(_44_m0[21u]).w + 1u), uint4(_25.Load(int3(uint2(_2560, _2561), 0u))).x));
                                    float _2318;
                                    if (_44_m0[21u].y > 0.0f)
                                    {
                                        _2318 = exp2(log2(_2607) * _44_m0[21u].y) * _2624;
                                    }
                                    else
                                    {
                                        _2318 = _2624;
                                    }
                                    float _2639 = ((((_2513 ? _2512 : 0.0f) * (((_633 & 32768u) != 0u) ? 1.0f : ((_2494 > 0.0f) ? _2494 : 0.0f))) * _2318) / (_2318 + 1.0f)) * _2143;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder = _2318;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_1 = (_2639 * (_2570.w - _2120)) + _2120;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_2 = (_2639 * (_2570.z - _2123)) + _2123;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_3 = (_2639 * (_2570.y - _2122)) + _2122;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_4 = (_2639 * (_2570.x - _2121)) + _2121;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_5 = (_2639 * (_2568 - _2094)) + _2094;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_6 = (_2639 * (_2567 - _2126)) + _2126;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_7 = (_2639 * (_2563.y - _2117)) + _2117;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_8 = (_2639 * (_2563.x - _2114)) + _2114;
                                }
                                else
                                {
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder = 0.0f;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_1 = _2120;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_2 = _2123;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_3 = _2122;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_4 = _2121;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_5 = _2094;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_6 = _2126;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_7 = _2117;
                                    frontier_phi_95_94_ladder_97_ladder_101_ladder_8 = _2114;
                                }
                                frontier_phi_95_94_ladder_97_ladder = frontier_phi_95_94_ladder_97_ladder_101_ladder;
                                frontier_phi_95_94_ladder_97_ladder_1 = frontier_phi_95_94_ladder_97_ladder_101_ladder_1;
                                frontier_phi_95_94_ladder_97_ladder_2 = frontier_phi_95_94_ladder_97_ladder_101_ladder_2;
                                frontier_phi_95_94_ladder_97_ladder_3 = frontier_phi_95_94_ladder_97_ladder_101_ladder_3;
                                frontier_phi_95_94_ladder_97_ladder_4 = frontier_phi_95_94_ladder_97_ladder_101_ladder_4;
                                frontier_phi_95_94_ladder_97_ladder_5 = frontier_phi_95_94_ladder_97_ladder_101_ladder_5;
                                frontier_phi_95_94_ladder_97_ladder_6 = frontier_phi_95_94_ladder_97_ladder_101_ladder_6;
                                frontier_phi_95_94_ladder_97_ladder_7 = frontier_phi_95_94_ladder_97_ladder_101_ladder_7;
                                frontier_phi_95_94_ladder_97_ladder_8 = frontier_phi_95_94_ladder_97_ladder_101_ladder_8;
                            }
                            else
                            {
                                frontier_phi_95_94_ladder_97_ladder = 0.0f;
                                frontier_phi_95_94_ladder_97_ladder_1 = _2120;
                                frontier_phi_95_94_ladder_97_ladder_2 = _2123;
                                frontier_phi_95_94_ladder_97_ladder_3 = _2122;
                                frontier_phi_95_94_ladder_97_ladder_4 = _2121;
                                frontier_phi_95_94_ladder_97_ladder_5 = _2094;
                                frontier_phi_95_94_ladder_97_ladder_6 = _2126;
                                frontier_phi_95_94_ladder_97_ladder_7 = _2117;
                                frontier_phi_95_94_ladder_97_ladder_8 = _2114;
                            }
                            frontier_phi_95_94_ladder = frontier_phi_95_94_ladder_97_ladder;
                            frontier_phi_95_94_ladder_1 = frontier_phi_95_94_ladder_97_ladder_1;
                            frontier_phi_95_94_ladder_2 = frontier_phi_95_94_ladder_97_ladder_2;
                            frontier_phi_95_94_ladder_3 = frontier_phi_95_94_ladder_97_ladder_3;
                            frontier_phi_95_94_ladder_4 = frontier_phi_95_94_ladder_97_ladder_4;
                            frontier_phi_95_94_ladder_5 = frontier_phi_95_94_ladder_97_ladder_5;
                            frontier_phi_95_94_ladder_6 = frontier_phi_95_94_ladder_97_ladder_6;
                            frontier_phi_95_94_ladder_7 = frontier_phi_95_94_ladder_97_ladder_7;
                            frontier_phi_95_94_ladder_8 = frontier_phi_95_94_ladder_97_ladder_8;
                        }
                        else
                        {
                            frontier_phi_95_94_ladder = 0.0f;
                            frontier_phi_95_94_ladder_1 = _2120;
                            frontier_phi_95_94_ladder_2 = _2123;
                            frontier_phi_95_94_ladder_3 = _2122;
                            frontier_phi_95_94_ladder_4 = _2121;
                            frontier_phi_95_94_ladder_5 = _2094;
                            frontier_phi_95_94_ladder_6 = _2126;
                            frontier_phi_95_94_ladder_7 = _2117;
                            frontier_phi_95_94_ladder_8 = _2114;
                        }
                        _2301 = frontier_phi_95_94_ladder_8;
                        _2303 = frontier_phi_95_94_ladder_7;
                        _2305 = frontier_phi_95_94_ladder_6;
                        _2307 = frontier_phi_95_94_ladder_5;
                        _2309 = frontier_phi_95_94_ladder_4;
                        _2311 = frontier_phi_95_94_ladder_3;
                        _2313 = frontier_phi_95_94_ladder_2;
                        _2315 = frontier_phi_95_94_ladder_1;
                        _2317 = frontier_phi_95_94_ladder;
                    }
                    else
                    {
                        _2301 = _2114;
                        _2303 = _2117;
                        _2305 = _2126;
                        _2307 = _2094;
                        _2309 = _2121;
                        _2311 = _2122;
                        _2313 = _2123;
                        _2315 = _2120;
                        _2317 = 0.0f;
                    }
                    uint64_t _2320 = WaveActiveSum(1ull);
                    float _2323 = WaveActiveSum(_2309);
                    float _2324 = WaveActiveSum(_2311);
                    float _2325 = WaveActiveSum(_2313);
                    float _2326 = WaveActiveSum(_2315);
                    float _2327 = float(int(uint(_2320)));
                    float _2328 = _2323 / _2327;
                    float _2329 = _2324 / _2327;
                    float _2330 = _2325 / _2327;
                    float _2331 = _2326 / _2327;
                    float _2332 = WaveActiveSum(_2301);
                    float _2333 = WaveActiveSum(_2303);
                    float _2334 = WaveActiveSum(_2305);
                    float _2335 = WaveActiveSum(_2307);
                    float _2336 = _2332 / _2327;
                    float _2337 = _2333 / _2327;
                    float _2338 = _2334 / _2327;
                    float _2339 = _2335 / _2327;
                    float _2340 = WaveActiveSum(_389);
                    float _2361;
                    float _2363;
                    float _2365;
                    float _2367;
                    float _2369;
                    float _2371;
                    float _2373;
                    float _2375;
                    if (_2317 > 2.0f)
                    {
                        _2361 = _2301;
                        _2363 = _2303;
                        _2365 = _2305;
                        _2367 = _2307;
                        _2369 = _2309;
                        _2371 = _2311;
                        _2373 = _2313;
                        _2375 = _2315;
                    }
                    else
                    {
                        float _2401 = ((((1.0f - clamp(exp2((abs((_2340 / _2327) - _389) * (-32.0f)) / _389), 0.0f, 1.0f)) * clamp((_2317 * 0.5f) + 0.5f, 0.0f, 1.0f)) + (-1.0f)) * _2143) + 1.0f;
                        _2361 = (_2401 * (_2301 - _2336)) + _2336;
                        _2363 = (_2401 * (_2303 - _2337)) + _2337;
                        _2365 = (_2401 * (_2305 - _2338)) + _2338;
                        _2367 = (_2401 * (_2307 - _2339)) + _2339;
                        _2369 = (_2401 * (_2309 - _2328)) + _2328;
                        _2371 = (_2401 * (_2311 - _2329)) + _2329;
                        _2373 = (_2401 * (_2313 - _2330)) + _2330;
                        _2375 = (_2401 * (_2315 - _2331)) + _2331;
                    }
                    _38[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = int4(uint(int(_2317 + 1.0f)).xxxx);
                    float _2520;
                    float _2522;
                    float _2524;
                    float _2526;
                    if ((asuint(_2361) & 2139095040u) == 2139095040u)
                    {
                        _2520 = 0.0f;
                        _2522 = 0.0f;
                        _2524 = 0.0f;
                        _2526 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_102_103_ladder;
                        float frontier_phi_102_103_ladder_1;
                        float frontier_phi_102_103_ladder_2;
                        float frontier_phi_102_103_ladder_3;
                        if ((asuint(_2363) & 2139095040u) == 2139095040u)
                        {
                            frontier_phi_102_103_ladder = 0.0f;
                            frontier_phi_102_103_ladder_1 = 0.0f;
                            frontier_phi_102_103_ladder_2 = 0.0f;
                            frontier_phi_102_103_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_102_103_ladder_107_ladder;
                            float frontier_phi_102_103_ladder_107_ladder_1;
                            float frontier_phi_102_103_ladder_107_ladder_2;
                            float frontier_phi_102_103_ladder_107_ladder_3;
                            if ((asuint(_2365) & 2139095040u) == 2139095040u)
                            {
                                frontier_phi_102_103_ladder_107_ladder = 0.0f;
                                frontier_phi_102_103_ladder_107_ladder_1 = 0.0f;
                                frontier_phi_102_103_ladder_107_ladder_2 = 0.0f;
                                frontier_phi_102_103_ladder_107_ladder_3 = 0.0f;
                            }
                            else
                            {
                                bool _2630 = (asuint(_2367) & 2139095040u) == 2139095040u;
                                frontier_phi_102_103_ladder_107_ladder = _2630 ? 0.0f : _2367;
                                frontier_phi_102_103_ladder_107_ladder_1 = _2630 ? 0.0f : _2361;
                                frontier_phi_102_103_ladder_107_ladder_2 = _2630 ? 0.0f : _2363;
                                frontier_phi_102_103_ladder_107_ladder_3 = _2630 ? 0.0f : _2365;
                            }
                            frontier_phi_102_103_ladder = frontier_phi_102_103_ladder_107_ladder;
                            frontier_phi_102_103_ladder_1 = frontier_phi_102_103_ladder_107_ladder_1;
                            frontier_phi_102_103_ladder_2 = frontier_phi_102_103_ladder_107_ladder_2;
                            frontier_phi_102_103_ladder_3 = frontier_phi_102_103_ladder_107_ladder_3;
                        }
                        _2520 = frontier_phi_102_103_ladder_1;
                        _2522 = frontier_phi_102_103_ladder_2;
                        _2524 = frontier_phi_102_103_ladder_3;
                        _2526 = frontier_phi_102_103_ladder;
                    }
                    _34[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = float4(_2520, _2522, _2524, _2526);
                    float _2576;
                    float _2578;
                    float _2580;
                    float _2582;
                    if ((asuint(_2369) & 2139095040u) == 2139095040u)
                    {
                        _2576 = 0.0f;
                        _2578 = 0.0f;
                        _2580 = 0.0f;
                        _2582 = 0.0f;
                    }
                    else
                    {
                        float frontier_phi_105_106_ladder;
                        float frontier_phi_105_106_ladder_1;
                        float frontier_phi_105_106_ladder_2;
                        float frontier_phi_105_106_ladder_3;
                        if ((asuint(_2371) & 2139095040u) == 2139095040u)
                        {
                            frontier_phi_105_106_ladder = 0.0f;
                            frontier_phi_105_106_ladder_1 = 0.0f;
                            frontier_phi_105_106_ladder_2 = 0.0f;
                            frontier_phi_105_106_ladder_3 = 0.0f;
                        }
                        else
                        {
                            float frontier_phi_105_106_ladder_110_ladder;
                            float frontier_phi_105_106_ladder_110_ladder_1;
                            float frontier_phi_105_106_ladder_110_ladder_2;
                            float frontier_phi_105_106_ladder_110_ladder_3;
                            if ((asuint(_2373) & 2139095040u) == 2139095040u)
                            {
                                frontier_phi_105_106_ladder_110_ladder = 0.0f;
                                frontier_phi_105_106_ladder_110_ladder_1 = 0.0f;
                                frontier_phi_105_106_ladder_110_ladder_2 = 0.0f;
                                frontier_phi_105_106_ladder_110_ladder_3 = 0.0f;
                            }
                            else
                            {
                                bool _2658 = (asuint(_2375) & 2139095040u) == 2139095040u;
                                frontier_phi_105_106_ladder_110_ladder = _2658 ? 0.0f : _2375;
                                frontier_phi_105_106_ladder_110_ladder_1 = _2658 ? 0.0f : _2373;
                                frontier_phi_105_106_ladder_110_ladder_2 = _2658 ? 0.0f : _2371;
                                frontier_phi_105_106_ladder_110_ladder_3 = _2658 ? 0.0f : _2369;
                            }
                            frontier_phi_105_106_ladder = frontier_phi_105_106_ladder_110_ladder;
                            frontier_phi_105_106_ladder_1 = frontier_phi_105_106_ladder_110_ladder_1;
                            frontier_phi_105_106_ladder_2 = frontier_phi_105_106_ladder_110_ladder_2;
                            frontier_phi_105_106_ladder_3 = frontier_phi_105_106_ladder_110_ladder_3;
                        }
                        _2576 = frontier_phi_105_106_ladder_3;
                        _2578 = frontier_phi_105_106_ladder_2;
                        _2580 = frontier_phi_105_106_ladder_1;
                        _2582 = frontier_phi_105_106_ladder;
                    }
                    _35[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = float4(_2576, _2578, _2580, _2582);
                    break;
                }
            }
        }
        _38[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = int4(uint4(0u, 0u, 0u, 0u));
        _34[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        _35[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = 0.0f.xxxx;
        break;
    }
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_WorkGroupID = stage_input.gl_WorkGroupID;
    gl_LocalInvocationID = stage_input.gl_LocalInvocationID;
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
