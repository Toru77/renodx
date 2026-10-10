cbuffer _35_37 : register(b1, space0)
{
    float4 _37_m0[87] : packoffset(c0);
};

cbuffer _40_42 : register(b2, space0)
{
    float4 _42_m0[24] : packoffset(c0);
};

cbuffer _45_47 : register(b3, space0)
{
    float4 _47_m0[1136] : packoffset(c0);
};

Buffer<uint4> _8 : register(t24, space0);
Buffer<uint4> _9 : register(t25, space0);
Texture2D<float4> _13 : register(t0, space0);
Texture2D<uint4> _16 : register(t5, space0);
Texture2D<float4> _19[] : register(t0, space6);
Texture2DArray<float4> _23[] : register(t0, space9);
Texture3D<uint4> _26 : register(t8, space0);
Buffer<uint4> _27 : register(t5, space12);
Buffer<uint4> _28 : register(t8, space12);
RWTexture2D<float4> _31 : register(u0, space0);
SamplerComparisonState _50 : register(s7, space0);

static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

void comp_main()
{
    float _76 = float(gl_GlobalInvocationID.x);
    float _77 = float(gl_GlobalInvocationID.y);
    float _78 = _76 + 0.5f;
    float _80 = _77 + 0.5f;
    uint _84 = uint(_78);
    uint _85 = uint(_80);
    float _86 = float(_84);
    float _87 = float(_85);
    float _104;
    float _108;
    float _110;
    float _112;
    if (_26.Load(int4(uint3(uint(int(_47_m0[58u].x * _86)), uint(int(_47_m0[58u].y * _87)), 0u), 0u)).x == 0u)
    {
        _104 = 1.0f;
        _108 = 1.0f;
        _110 = 1.0f;
        _112 = 1.0f;
    }
    else
    {
        float _124 = ((_78 * 2.0f) * _42_m0[13u].z) + (-1.0f);
        float _128 = ((1.0f - (_42_m0[13u].w * _80)) * 2.0f) + (-1.0f);
        float4 _141 = _13.Load(int3(uint2(uint(int((_42_m0[14u].x * _76) + 0.5f)), uint(int((_42_m0[14u].y * _77) + 0.5f))), 0u));
        float _143 = _141.x;
        uint4 _145 = _16.Load(int3(uint2(_84, _85), 0u));
        uint _148 = _145.w;
        float frontier_phi_1_2_ladder;
        float frontier_phi_1_2_ladder_1;
        float frontier_phi_1_2_ladder_2;
        float frontier_phi_1_2_ladder_3;
        if ((_143 != 0.0f) && (_9.Load((_8.Load(((((_148 & 128u) == 0u) ? ((_145.x << 7u) | _148) : 1u) * 4u) + 2u).x * 57u) + 12u).x != 0u))
        {
            float _212 = mad(_143, _42_m0[4u].z, mad(_128, _42_m0[4u].y, _42_m0[4u].x * _124)) + _42_m0[4u].w;
            float _213 = (mad(_143, _42_m0[1u].z, mad(_128, _42_m0[1u].y, _42_m0[1u].x * _124)) + _42_m0[1u].w) / _212;
            float _214 = (mad(_143, _42_m0[2u].z, mad(_128, _42_m0[2u].y, _42_m0[2u].x * _124)) + _42_m0[2u].w) / _212;
            float _215 = (mad(_143, _42_m0[3u].z, mad(_128, _42_m0[3u].y, _42_m0[3u].x * _124)) + _42_m0[3u].w) / _212;
            float _239 = (clamp(((_37_m0[63u].x / (_37_m0[63u].w - (_37_m0[63u].y * _143))) - _42_m0[11u].z) / (_42_m0[11u].w + 1.0000000116860974230803549289703e-07f), 0.0f, 1.0f) * (_42_m0[11u].y - _42_m0[11u].x)) + _42_m0[11u].x;
            uint _240 = uint(_42_m0[7u].y);
            float _241 = _239 + _42_m0[7u].z;
            float _242 = _239 + _42_m0[6u].y;
            uint4 _267 = _26.Load(int4(uint3(uint(int(_47_m0[58u].x * _86)), uint(int(_47_m0[58u].y * _87)), uint(int(max(_47_m0[60u].y - (_47_m0[60u].x * log2((_47_m0[58u].z * _143) + _47_m0[58u].w)), 0.0f))) + 1u), 0u));
            uint _269 = _267.x;
            float _107;
            float _109;
            float _111;
            float _113;
            float _64[3];
            float _270 = 0.0f;
            float _271 = 0.0f;
            float _272 = 0.0f;
            float _273 = 0.0f;
            uint _274 = 0u;
            bool _278;
            for (;;)
            {
                _278 = ((1u << _274) & _269) == 0u;
                float frontier_phi_5_pred;
                float frontier_phi_5_pred_1;
                float frontier_phi_5_pred_2;
                float frontier_phi_5_pred_3;
                if (_278)
                {
                    frontier_phi_5_pred = _273;
                    frontier_phi_5_pred_1 = _272;
                    frontier_phi_5_pred_2 = _271;
                    frontier_phi_5_pred_3 = _270;
                }
                else
                {
                    float _285 = float(_274 == 0u);
                    float _287 = float(_274 == 1u);
                    float _289 = float(_274 == 2u);
                    float _291 = float(_274 == 3u);
                    uint4 _294 = asuint(_42_m0[0u]);
                    uint _306 = uint(dot(float4(_285, _287, _289, _291), float4(float(_294.x), float(_294.y), float(_294.z), float(_294.w))));
                    float _309;
                    if (_306 == 4294967295u)
                    {
                        _309 = 1.0f;
                    }
                    else
                    {
                        float frontier_phi_8_9_ladder;
                        if (_28.Load((_306 * 28u) + 1u).x > 1u)
                        {
                            uint _329 = (_306 * 28u) + 5u;
                            float3 _340 = asfloat(uint3(_28.Load(_329).x, _28.Load(_329 + 1u).x, _28.Load(_329 + 2u).x));
                            uint _343 = (_306 * 28u) + 8u;
                            float3 _353 = asfloat(uint3(_28.Load(_343).x, _28.Load(_343 + 1u).x, _28.Load(_343 + 2u).x));
                            float _370 = asfloat(_28.Load((_306 * 28u) + 20u).x);
                            float _381 = asfloat(_28.Load((_306 * 28u) + 22u).x);
                            float frontier_phi_8_9_ladder_10_ladder;
                            if (_28.Load((_306 * 28u) + 2u).x < 4u)
                            {
                                float _481 = (_213 - _353.x) - _340.x;
                                float _483 = (_214 - _353.y) - _340.y;
                                float _485 = (_215 - _353.z) - _340.z;
                                _64[0u] = _481;
                                _64[1u] = _483;
                                _64[2u] = _485;
                                float _490 = abs(_481);
                                float _491 = abs(_483);
                                float _492 = abs(_485);
                                uint _498 = (_490 > _491) ? ((_490 > _492) ? 0u : 2u) : ((_491 > _492) ? 1u : 2u);
                                uint _504 = (_498 << 1u) | uint(_64[_498] < 0.0f);
                                uint _505 = _504 + _28.Load(_306 * 28u).x;
                                uint _507 = _505 * 16u;
                                float4 _520 = asfloat(uint4(_27.Load(_507).x, _27.Load(_507 + 1u).x, _27.Load(_507 + 2u).x, _27.Load(_507 + 3u).x));
                                uint _526 = (_505 * 16u) + 4u;
                                float4 _539 = asfloat(uint4(_27.Load(_526).x, _27.Load(_526 + 1u).x, _27.Load(_526 + 2u).x, _27.Load(_526 + 3u).x));
                                uint _545 = (_505 * 16u) + 8u;
                                float4 _558 = asfloat(uint4(_27.Load(_545).x, _27.Load(_545 + 1u).x, _27.Load(_545 + 2u).x, _27.Load(_545 + 3u).x));
                                uint _564 = (_505 * 16u) + 12u;
                                float4 _577 = asfloat(uint4(_27.Load(_564).x, _27.Load(_564 + 1u).x, _27.Load(_564 + 2u).x, _27.Load(_564 + 3u).x));
                                float _597 = mad(_485, _577.z, mad(_483, _577.y, _577.x * _481)) + _577.w;
                                float _601 = ((mad(_485, _520.z, mad(_483, _520.y, _520.x * _481)) + _520.w) / _597) * 0.5f;
                                float _602 = ((mad(_485, _539.z, mad(_483, _539.y, _539.x * _481)) + _539.w) / _597) * (-0.5f);
                                float _604 = _601 + 0.5f;
                                float _605 = _602 + 0.5f;
                                float _606 = clamp((mad(_485, _558.z, mad(_483, _558.y, _558.x * _481)) + _558.w) / _597, 0.0f, 1.0f);
                                float _609 = ((_606 * asfloat(_28.Load((_306 * 28u) + 16u).x)) + asfloat(_28.Load((_306 * 28u) + 17u).x)) * _42_m0[7u].x;
                                float frontier_phi_8_9_ladder_10_ladder_12_ladder;
                                if (((_604 < 0.0f) || (_604 > 1.0f)) || ((_605 < 0.0f) || (_605 > 1.0f)))
                                {
                                    frontier_phi_8_9_ladder_10_ladder_12_ladder = 1.0f;
                                }
                                else
                                {
                                    float _754 = float(int(_504));
                                    uint _757 = (_306 + 80u) + 0u;
                                    float _846;
                                    if (asuint(_42_m0[12u]).x == 1u)
                                    {
                                        float _765 = _606 - _241;
                                        float _776 = (float(int(uint(int(_87)))) * 0.005837149918079376220703125f) + (float(int(uint(int(_86)))) * 0.067110560834407806396484375f);
                                        float _781 = frac(abs(_776));
                                        float _784 = ((_776 >= ((-0.0f) - _776)) ? _781 : ((-0.0f) - _781)) * 52.98291778564453125f;
                                        float _789 = frac(abs(_784));
                                        float _791 = (_784 >= ((-0.0f) - _784)) ? _789 : ((-0.0f) - _789);
                                        float _792 = _791 * 6.283185482025146484375f;
                                        float _796 = frac(_791 * 16.0f);
                                        float frontier_phi_22_16_ladder;
                                        if (_240 > 15u)
                                        {
                                            float _854;
                                            float _856;
                                            float _858;
                                            uint _860;
                                            _854 = _604;
                                            _856 = _605;
                                            _858 = 0.0f;
                                            _860 = 0u;
                                            float _859;
                                            for (;;)
                                            {
                                                float _862 = float(int(_860));
                                                float _865 = (_862 * 2.3999631404876708984375f) + _792;
                                                float _872 = (_609 * 0.25f) * sqrt(_862 + _796);
                                                float _855 = (_872 * cos(_865)) + _854;
                                                float _857 = (_872 * sin(_865)) + _856;
                                                _859 = _23[NonUniformResourceIndex(_757)].SampleCmpLevelZero(_50, float3(_855, _857, _754), _765).xxxx.x + _858;
                                                uint _861 = _860 + 1u;
                                                if (_861 == 16u)
                                                {
                                                    break;
                                                }
                                                else
                                                {
                                                    _854 = _855;
                                                    _856 = _857;
                                                    _858 = _859;
                                                    _860 = _861;
                                                }
                                            }
                                            frontier_phi_22_16_ladder = _859 * 0.0625f;
                                        }
                                        else
                                        {
                                            float _884;
                                            float _886;
                                            float _888;
                                            uint _890;
                                            _884 = _604;
                                            _886 = _605;
                                            _888 = 0.0f;
                                            _890 = 0u;
                                            float _889;
                                            for (;;)
                                            {
                                                float _892 = float(int(_890));
                                                float _894 = (_892 * 2.3999631404876708984375f) + _792;
                                                float _900 = (_609 * 0.5f) * sqrt(_892 + _796);
                                                float _885 = (_900 * cos(_894)) + _884;
                                                float _887 = (_900 * sin(_894)) + _886;
                                                _889 = _23[NonUniformResourceIndex(_757)].SampleCmpLevelZero(_50, float3(_885, _887, _754), _765).xxxx.x + _888;
                                                uint _891 = _890 + 1u;
                                                if (_891 == 4u)
                                                {
                                                    break;
                                                }
                                                else
                                                {
                                                    _884 = _885;
                                                    _886 = _887;
                                                    _888 = _889;
                                                    _890 = _891;
                                                }
                                            }
                                            frontier_phi_22_16_ladder = _889 * 0.25f;
                                        }
                                        _846 = frontier_phi_22_16_ladder;
                                    }
                                    else
                                    {
                                        _846 = _23[NonUniformResourceIndex(_757)].SampleCmpLevelZero(_50, float3(_604, _605, _754), _606 - _242).xxxx.x;
                                    }
                                    float _912;
                                    if (_28.Load((_306 * 28u) + 19u).x == 0u)
                                    {
                                        _912 = 1.0f;
                                    }
                                    else
                                    {
                                        float _925 = clamp(((max(abs((-0.0f) - _601), abs((-0.0f) - _602)) * 2.0f) - _370) / (0.999000012874603271484375f - _370), 0.0f, 1.0f);
                                        _912 = 1.0f - ((_925 * _925) * (3.0f - (_925 * 2.0f)));
                                    }
                                    float _1001;
                                    if (_28.Load((_306 * 28u) + 21u).x == 0u)
                                    {
                                        _1001 = _912;
                                    }
                                    else
                                    {
                                        float _1012 = clamp((((-0.0f) - (_485 * asfloat(_28.Load((_306 * 28u) + 24u).x))) - _381) / (asfloat(_28.Load((_306 * 28u) + 23u).x) - _381), 0.0f, 1.0f);
                                        _1001 = (1.0f - ((_1012 * _1012) * (3.0f - (_1012 * 2.0f)))) * _912;
                                    }
                                    frontier_phi_8_9_ladder_10_ladder_12_ladder = ((_1001 * (1.0f - asfloat(_28.Load((_306 * 28u) + 18u).x))) * (_846 + (-1.0f))) + 1.0f;
                                }
                                frontier_phi_8_9_ladder_10_ladder = frontier_phi_8_9_ladder_10_ladder_12_ladder;
                            }
                            else
                            {
                                frontier_phi_8_9_ladder_10_ladder = 1.0f;
                            }
                            frontier_phi_8_9_ladder = frontier_phi_8_9_ladder_10_ladder;
                        }
                        else
                        {
                            uint4 _395 = _28.Load(_306 * 28u);
                            uint _396 = _395.x;
                            uint _402 = (_306 * 28u) + 5u;
                            float3 _412 = asfloat(uint3(_28.Load(_402).x, _28.Load(_402 + 1u).x, _28.Load(_402 + 2u).x));
                            uint _414 = (_306 * 28u) + 8u;
                            float3 _424 = asfloat(uint3(_28.Load(_414).x, _28.Load(_414 + 1u).x, _28.Load(_414 + 2u).x));
                            float _438 = asfloat(_28.Load((_306 * 28u) + 20u).x);
                            float _447 = asfloat(_28.Load((_306 * 28u) + 22u).x);
                            float frontier_phi_8_9_ladder_11_ladder;
                            if (_28.Load((_306 * 28u) + 2u).x < 4u)
                            {
                                float _634 = (_213 - _424.x) - _412.x;
                                float _636 = (_214 - _424.y) - _412.y;
                                float _638 = (_215 - _424.z) - _412.z;
                                uint _640 = _396 * 16u;
                                float4 _653 = asfloat(uint4(_27.Load(_640).x, _27.Load(_640 + 1u).x, _27.Load(_640 + 2u).x, _27.Load(_640 + 3u).x));
                                uint _659 = (_396 * 16u) + 4u;
                                float4 _672 = asfloat(uint4(_27.Load(_659).x, _27.Load(_659 + 1u).x, _27.Load(_659 + 2u).x, _27.Load(_659 + 3u).x));
                                uint _678 = (_396 * 16u) + 8u;
                                float4 _691 = asfloat(uint4(_27.Load(_678).x, _27.Load(_678 + 1u).x, _27.Load(_678 + 2u).x, _27.Load(_678 + 3u).x));
                                uint _697 = (_396 * 16u) + 12u;
                                float4 _710 = asfloat(uint4(_27.Load(_697).x, _27.Load(_697 + 1u).x, _27.Load(_697 + 2u).x, _27.Load(_697 + 3u).x));
                                float _730 = mad(_638, _710.z, mad(_636, _710.y, _710.x * _634)) + _710.w;
                                float _734 = ((mad(_638, _653.z, mad(_636, _653.y, _653.x * _634)) + _653.w) / _730) * 0.5f;
                                float _735 = ((mad(_638, _672.z, mad(_636, _672.y, _672.x * _634)) + _672.w) / _730) * (-0.5f);
                                float _736 = _734 + 0.5f;
                                float _737 = _735 + 0.5f;
                                float _738 = clamp((mad(_638, _691.z, mad(_636, _691.y, _691.x * _634)) + _691.w) / _730, 0.0f, 1.0f);
                                float _741 = ((_738 * asfloat(_28.Load((_306 * 28u) + 16u).x)) + asfloat(_28.Load((_306 * 28u) + 17u).x)) * _42_m0[7u].x;
                                float frontier_phi_8_9_ladder_11_ladder_13_ladder;
                                if (((_736 < 0.0f) || (_736 > 1.0f)) || ((_737 < 0.0f) || (_737 > 1.0f)))
                                {
                                    frontier_phi_8_9_ladder_11_ladder_13_ladder = 1.0f;
                                }
                                else
                                {
                                    uint _764 = (_306 + 80u) + 0u;
                                    float _850;
                                    if (asuint(_42_m0[12u]).x == 1u)
                                    {
                                        float _809 = _738 - _241;
                                        float _817 = (float(int(uint(int(_87)))) * 0.005837149918079376220703125f) + (float(int(uint(int(_86)))) * 0.067110560834407806396484375f);
                                        float _821 = frac(abs(_817));
                                        float _824 = ((_817 >= ((-0.0f) - _817)) ? _821 : ((-0.0f) - _821)) * 52.98291778564453125f;
                                        float _828 = frac(abs(_824));
                                        float _830 = (_824 >= ((-0.0f) - _824)) ? _828 : ((-0.0f) - _828);
                                        float _831 = _830 * 6.283185482025146484375f;
                                        float _833 = frac(_830 * 16.0f);
                                        float frontier_phi_25_18_ladder;
                                        if (_240 > 15u)
                                        {
                                            float _931;
                                            uint _933;
                                            _931 = 0.0f;
                                            _933 = 0u;
                                            float _932;
                                            for (;;)
                                            {
                                                float _935 = float(int(_933));
                                                float _937 = (_935 * 2.3999631404876708984375f) + _831;
                                                float _943 = (_741 * 0.25f) * sqrt(_935 + _833);
                                                _932 = _19[NonUniformResourceIndex(_764)].SampleCmpLevelZero(_50, float2((_943 * cos(_937)) + _736, (_943 * sin(_937)) + _737), _809).xxxx.x + _931;
                                                uint _934 = _933 + 1u;
                                                if (_934 == 16u)
                                                {
                                                    break;
                                                }
                                                else
                                                {
                                                    _931 = _932;
                                                    _933 = _934;
                                                }
                                            }
                                            frontier_phi_25_18_ladder = _932 * 0.0625f;
                                        }
                                        else
                                        {
                                            float _957;
                                            uint _959;
                                            _957 = 0.0f;
                                            _959 = 0u;
                                            float _958;
                                            for (;;)
                                            {
                                                float _961 = float(int(_959));
                                                float _963 = (_961 * 2.3999631404876708984375f) + _831;
                                                float _969 = (_741 * 0.5f) * sqrt(_961 + _833);
                                                _958 = _19[NonUniformResourceIndex(_764)].SampleCmpLevelZero(_50, float2((_969 * cos(_963)) + _736, (_969 * sin(_963)) + _737), _809).xxxx.x + _957;
                                                uint _960 = _959 + 1u;
                                                if (_960 == 4u)
                                                {
                                                    break;
                                                }
                                                else
                                                {
                                                    _957 = _958;
                                                    _959 = _960;
                                                }
                                            }
                                            frontier_phi_25_18_ladder = _958 * 0.25f;
                                        }
                                        _850 = frontier_phi_25_18_ladder;
                                    }
                                    else
                                    {
                                        _850 = _19[NonUniformResourceIndex(_764)].SampleCmpLevelZero(_50, float2(_736, _737), _738 - _242).xxxx.x;
                                    }
                                    float _983;
                                    if (_28.Load((_306 * 28u) + 19u).x == 0u)
                                    {
                                        _983 = 1.0f;
                                    }
                                    else
                                    {
                                        float _995 = clamp(((max(abs((-0.0f) - _734), abs((-0.0f) - _735)) * 2.0f) - _438) / (0.999000012874603271484375f - _438), 0.0f, 1.0f);
                                        _983 = 1.0f - ((_995 * _995) * (3.0f - (_995 * 2.0f)));
                                    }
                                    float _1018;
                                    if (_28.Load((_306 * 28u) + 21u).x == 0u)
                                    {
                                        _1018 = _983;
                                    }
                                    else
                                    {
                                        float _1029 = clamp((((-0.0f) - (_638 * asfloat(_28.Load((_306 * 28u) + 24u).x))) - _447) / (asfloat(_28.Load((_306 * 28u) + 23u).x) - _447), 0.0f, 1.0f);
                                        _1018 = (1.0f - ((_1029 * _1029) * (3.0f - (_1029 * 2.0f)))) * _983;
                                    }
                                    frontier_phi_8_9_ladder_11_ladder_13_ladder = ((_1018 * (1.0f - asfloat(_28.Load((_306 * 28u) + 18u).x))) * (_850 + (-1.0f))) + 1.0f;
                                }
                                frontier_phi_8_9_ladder_11_ladder = frontier_phi_8_9_ladder_11_ladder_13_ladder;
                            }
                            else
                            {
                                frontier_phi_8_9_ladder_11_ladder = 1.0f;
                            }
                            frontier_phi_8_9_ladder = frontier_phi_8_9_ladder_11_ladder;
                        }
                        _309 = frontier_phi_8_9_ladder;
                    }
                    frontier_phi_5_pred = (_309 * _291) + _273;
                    frontier_phi_5_pred_1 = (_309 * _289) + _272;
                    frontier_phi_5_pred_2 = (_309 * _287) + _271;
                    frontier_phi_5_pred_3 = (_309 * _285) + _270;
                }
                _113 = frontier_phi_5_pred;
                _111 = frontier_phi_5_pred_1;
                _109 = frontier_phi_5_pred_2;
                _107 = frontier_phi_5_pred_3;
                uint _275 = _274 + 1u;
                if (_275 == 4u)
                {
                    break;
                }
                else
                {
                    _270 = _107;
                    _271 = _109;
                    _272 = _111;
                    _273 = _113;
                    _274 = _275;
                    continue;
                }
            }
            frontier_phi_1_2_ladder = _107;
            frontier_phi_1_2_ladder_1 = _113;
            frontier_phi_1_2_ladder_2 = _111;
            frontier_phi_1_2_ladder_3 = _109;
        }
        else
        {
            frontier_phi_1_2_ladder = 0.0f;
            frontier_phi_1_2_ladder_1 = 0.0f;
            frontier_phi_1_2_ladder_2 = 0.0f;
            frontier_phi_1_2_ladder_3 = 0.0f;
        }
        _104 = frontier_phi_1_2_ladder;
        _108 = frontier_phi_1_2_ladder_3;
        _110 = frontier_phi_1_2_ladder_2;
        _112 = frontier_phi_1_2_ladder_1;
    }
    _31[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = float4(_104, _108, _110, _112);
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
