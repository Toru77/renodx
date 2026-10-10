static float _771;

cbuffer _23_25 : register(b0, space0)
{
    float4 _25_m0[42] : packoffset(c0);
};

Buffer<uint4> _8 : register(t0, space0);
Buffer<uint4> _9 : register(t1, space0);
Buffer<uint4> _10 : register(t2, space0);
RWBuffer<uint> _13 : register(u0, space0);
RWBuffer<uint> _14 : register(u1, space0);
RWBuffer<uint> _15 : register(u2, space0);
RWBuffer<uint> _16 : register(u3, space0);
RWBuffer<uint> _17 : register(u4, space0);
RWBuffer<uint> _18 : register(u5, space0);

static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

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
    if (gl_GlobalInvocationID.x < asuint(_25_m0[27u]).w)
    {
        if (gl_GlobalInvocationID.x == 0u)
        {
            _16[asuint(_25_m0[28u]).w * 5u] = uint4(0u, 0u, 0u, 0u).x;
            _16[(asuint(_25_m0[28u]).w * 5u) + 1u] = uint4(0u, 0u, 0u, 0u).x;
            _16[(asuint(_25_m0[28u]).w * 5u) + 2u] = uint4(0u, 0u, 0u, 0u).x;
            _16[(asuint(_25_m0[28u]).w * 5u) + 3u] = uint4(0u, 0u, 0u, 0u).x;
            _16[(asuint(_25_m0[28u]).w * 5u) + 4u] = uint4(0u, 0u, 0u, 0u).x;
        }
        float _96 = asfloat(_8.Load(gl_GlobalInvocationID.x * 18u).x);
        uint _98 = (gl_GlobalInvocationID.x * 18u) + 1u;
        float3 _109 = asfloat(uint3(_8.Load(_98).x, _8.Load(_98 + 1u).x, _8.Load(_98 + 2u).x));
        float _110 = _109.x;
        float _111 = _109.y;
        float _112 = _109.z;
        uint _114 = (gl_GlobalInvocationID.x * 18u) + 4u;
        float4 _127 = asfloat(uint4(_8.Load(_114).x, _8.Load(_114 + 1u).x, _8.Load(_114 + 2u).x, _8.Load(_114 + 3u).x));
        float _128 = _127.x;
        float _129 = _127.y;
        float _130 = _127.z;
        float _131 = _127.w;
        uint4 _135 = _8.Load((gl_GlobalInvocationID.x * 18u) + 8u);
        uint _136 = _135.x;
        uint4 _140 = _8.Load((gl_GlobalInvocationID.x * 18u) + 10u);
        uint _141 = _140.x;
        uint4 _145 = _8.Load((gl_GlobalInvocationID.x * 18u) + 11u);
        uint _146 = _145.x;
        float _152 = asfloat(_8.Load((gl_GlobalInvocationID.x * 18u) + 12u).x);
        uint _155 = (gl_GlobalInvocationID.x * 18u) + 13u;
        float3 _165 = asfloat(uint3(_8.Load(_155).x, _8.Load(_155 + 1u).x, _8.Load(_155 + 2u).x));
        float _166 = _165.x;
        float _167 = _165.y;
        float _168 = _165.z;
        float _174 = asfloat(_8.Load((gl_GlobalInvocationID.x * 18u) + 16u).x);
        float _180 = asfloat(_8.Load((gl_GlobalInvocationID.x * 18u) + 17u).x);
        uint _182 = _136 * 24u;
        float3 _193 = asfloat(uint3(_9.Load(_182).x, _9.Load(_182 + 1u).x, _9.Load(_182 + 2u).x));
        float _194 = _193.x;
        float _195 = _193.y;
        float _196 = _193.z;
        float _201 = asfloat(_9.Load((_136 * 24u) + 3u).x);
        float _206 = asfloat(_9.Load((_136 * 24u) + 4u).x);
        float _211 = asfloat(_9.Load((_136 * 24u) + 5u).x);
        float _217 = asfloat(_9.Load((_136 * 24u) + 6u).x);
        float _223 = asfloat(_9.Load((_136 * 24u) + 7u).x);
        float _228 = asfloat(_9.Load((_136 * 24u) + 8u).x);
        float _234 = asfloat(_9.Load((_136 * 24u) + 9u).x);
        float _239 = asfloat(_9.Load((_136 * 24u) + 10u).x);
        uint4 _242 = _9.Load((_136 * 24u) + 11u);
        uint _243 = _242.x;
        uint4 _250 = _9.Load((_136 * 24u) + 13u);
        uint _251 = _250.x;
        float _257 = asfloat(_9.Load((_136 * 24u) + 15u).x);
        float _262 = asfloat(_9.Load((_136 * 24u) + 16u).x);
        uint4 _265 = _9.Load((_136 * 24u) + 17u);
        uint _266 = _265.x;
        float _272 = asfloat(_9.Load((_136 * 24u) + 21u).x);
        float _278 = asfloat(_9.Load((_136 * 24u) + 22u).x);
        uint _281 = (_251 >> 8u) & (_146 >> 24u);
        uint4 _293 = asuint(_25_m0[28u]);
        uint _294 = _293.y;
        if ((uint((((_146 >> 5u) & 255u) & _294) == 0u) | uint(_266 >= asuint(_25_m0[38u]).x)) == 0u)
        {
            bool _306 = _266 >= asuint(_25_m0[37u]).w;
            float _332;
            if (_228 > 0.0f)
            {
                float _315 = _228 * 0.5f;
                float _321 = clamp(_223 - _315, 0.0f, 1.0f);
                float _325 = clamp((_25_m0[28u].x - _321) / (clamp(_315 + _223, 0.0f, 1.0f) - _321), 0.0f, 1.0f);
                _332 = (_325 * _325) * (3.0f - (_325 * 2.0f));
            }
            else
            {
                _332 = float(_25_m0[28u].x >= _223);
            }
            float _337 = _25_m0[29u].z * _332;
            bool _342 = (_234 != 0.0f) || (_239 != 0.0f);
            float _343 = _342 ? _239 : _25_m0[29u].y;
            float _344 = _342 ? _234 : _25_m0[29u].x;
            float _350 = _110 - _25_m0[27u].x;
            float _351 = _111 - _25_m0[27u].y;
            float _352 = _112 - _25_m0[27u].z;
            float _358 = sqrt(((_350 * _350) + (_351 * _351)) + (_352 * _352));
            uint _361 = (_146 >> 20u) & 15u;
            uint4 _365 = asuint(_25_m0[32u]);
            uint4 _383 = asuint(_25_m0[33u]);
            if (!((((((((_365.y << 2u) & 4u) | (_365.x & 1u)) | ((_365.z << 1u) & 2u)) | ((_365.w << 3u) & 8u)) | ((_383.x << 4u) & 16u)) & (1u << _361)) == 0u))
            {
                switch (_361)
                {
                    case 0u:
                    case 2u:
                    {
                        float _399 = _25_m0[24u].w - _25_m0[18u].w;
                        float _402 = clamp((max(0.0f, _358 - _262) - _25_m0[18u].w) / _399, 0.0f, 1.0f);
                        float _408 = clamp((_358 - _343) / (_344 - _343), 0.0f, 1.0f);
                        bool _412 = (_146 & 16u) != 0u;
                        precise float _414 = _402 * _408;
                        precise float _415 = _414 * _414;
                        float _417 = (3.0f - (_408 * 2.0f)) * (_337 * _96);
                        float _425 = clamp((max(0.0f, _358 - _201) - _25_m0[18u].w) / _399, 0.0f, 1.0f);
                        precise float _429 = _408 * _425;
                        precise float _430 = _429 * _429;
                        float _433 = ((_417 * (_412 ? _152 : _206)) * (3.0f - (_425 * 2.0f))) * _430;
                        if (!(max(((_417 * (_412 ? _152 : _257)) * (3.0f - (_402 * 2.0f))) * _415, _433) > 0.0f))
                        {
                            break;
                        }
                        uint _539 = _293.z;
                        uint _984;
                        uint _985;
                        uint _987;
                        uint _989;
                        uint _991;
                        uint _993;
                        uint _994;
                        uint _995;
                        uint _996;
                        uint _997;
                        float _999;
                        float _1001;
                        float _1002;
                        float _1003;
                        if (_361 == 0u)
                        {
                            uint _555;
                            InterlockedAdd(_16[_539 * 5u], 1u, _555);
                            if (!(int(_555) < int(asuint(_25_m0[33u]).y)))
                            {
                                uint _652;
                                InterlockedAdd(_16[asuint(_25_m0[28u]).z * 5u], 4294967295u, _652);
                                break;
                            }
                            uint _627 = asuint(_25_m0[34u]).z + _555;
                            float _633 = _110 - _25_m0[27u].x;
                            float _634 = _111 - _25_m0[27u].y;
                            float _635 = _112 - _25_m0[27u].z;
                            float _639 = _433 * (_412 ? _166 : _194);
                            float _640 = _433 * (_412 ? _167 : _195);
                            float _641 = _433 * (_412 ? _168 : _196);
                            float _717;
                            float _719;
                            float _721;
                            uint _723;
                            if (max(_639, max(_640, _641)) < 65504.0f)
                            {
                                _717 = _639;
                                _719 = _640;
                                _721 = _641;
                                _723 = 0u;
                            }
                            else
                            {
                                float _784 = _639 * 0.0625f;
                                float _785 = _640 * 0.0625f;
                                float _786 = _641 * 0.0625f;
                                float _788 = max(_784, max(_785, _786));
                                float _790 = min(_788, 65504.0f) / _788;
                                _717 = _790 * _784;
                                _719 = _790 * _785;
                                _721 = _790 * _786;
                                _723 = 32768u;
                            }
                            uint _727 = _141 + 1u;
                            uint _748 = (spvPackHalf2x16(float2(_634, 0.0f)) << 16u) | spvPackHalf2x16(float2(_633, 0.0f));
                            uint _754 = (spvPackHalf2x16(float2(1.0f / _201, 0.0f)) << 16u) | spvPackHalf2x16(float2(_635, 0.0f));
                            uint _760 = (spvPackHalf2x16(float2(_719, 0.0f)) << 16u) | spvPackHalf2x16(float2(_717, 0.0f));
                            uint _764 = spvPackHalf2x16(float2(_721, 0.0f)) | (((((((_146 >> 4u) & 49152u) | ((_146 >> 12u) & 48u)) | (_251 & 7u)) | (_281 << 6u)) | (_306 ? 0u : ((_146 << 1u) & 8u))) << 16u);
                            uint _768 = (_723 | (_727 & 127u)) << 16u;
                            uint _772 = spvPackHalf2x16(float2(_771, 0.0f));
                            uint _775 = (_772 << 16u) | spvPackHalf2x16(float2(_272, 0.0f));
                            uint _781 = (spvPackHalf2x16(0.0f.xx) << 16u) | spvPackHalf2x16(float2(_278, 0.0f));
                            float _782 = _201 * _201;
                            uint frontier_phi_40_32_ladder;
                            uint frontier_phi_40_32_ladder_1;
                            uint frontier_phi_40_32_ladder_2;
                            uint frontier_phi_40_32_ladder_3;
                            uint frontier_phi_40_32_ladder_4;
                            uint frontier_phi_40_32_ladder_5;
                            uint frontier_phi_40_32_ladder_6;
                            uint frontier_phi_40_32_ladder_7;
                            uint frontier_phi_40_32_ladder_8;
                            uint frontier_phi_40_32_ladder_9;
                            float frontier_phi_40_32_ladder_10;
                            float frontier_phi_40_32_ladder_11;
                            float frontier_phi_40_32_ladder_12;
                            float frontier_phi_40_32_ladder_13;
                            if (_727 == 0u)
                            {
                                frontier_phi_40_32_ladder = _748;
                                frontier_phi_40_32_ladder_1 = _627;
                                frontier_phi_40_32_ladder_2 = _754;
                                frontier_phi_40_32_ladder_3 = _760;
                                frontier_phi_40_32_ladder_4 = _764;
                                frontier_phi_40_32_ladder_5 = _768;
                                frontier_phi_40_32_ladder_6 = _775;
                                frontier_phi_40_32_ladder_7 = _772;
                                frontier_phi_40_32_ladder_8 = _781;
                                frontier_phi_40_32_ladder_9 = 4294967295u;
                                frontier_phi_40_32_ladder_10 = _782;
                                frontier_phi_40_32_ladder_11 = _633;
                                frontier_phi_40_32_ladder_12 = _634;
                                frontier_phi_40_32_ladder_13 = _635;
                            }
                            else
                            {
                                frontier_phi_40_32_ladder = _748;
                                frontier_phi_40_32_ladder_1 = _627;
                                frontier_phi_40_32_ladder_2 = _754;
                                frontier_phi_40_32_ladder_3 = _760;
                                frontier_phi_40_32_ladder_4 = _764;
                                frontier_phi_40_32_ladder_5 = _768;
                                frontier_phi_40_32_ladder_6 = _775;
                                frontier_phi_40_32_ladder_7 = _772;
                                frontier_phi_40_32_ladder_8 = _781;
                                frontier_phi_40_32_ladder_9 = _10.Load((_141 * 28u) + 2u).x;
                                frontier_phi_40_32_ladder_10 = _782;
                                frontier_phi_40_32_ladder_11 = _633;
                                frontier_phi_40_32_ladder_12 = _634;
                                frontier_phi_40_32_ladder_13 = _635;
                            }
                            _984 = frontier_phi_40_32_ladder_1;
                            _985 = frontier_phi_40_32_ladder;
                            _987 = frontier_phi_40_32_ladder_2;
                            _989 = frontier_phi_40_32_ladder_3;
                            _991 = frontier_phi_40_32_ladder_4;
                            _993 = frontier_phi_40_32_ladder_5;
                            _994 = frontier_phi_40_32_ladder_6;
                            _995 = frontier_phi_40_32_ladder_7;
                            _996 = frontier_phi_40_32_ladder_8;
                            _997 = frontier_phi_40_32_ladder_9;
                            _999 = frontier_phi_40_32_ladder_10;
                            _1001 = frontier_phi_40_32_ladder_11;
                            _1002 = frontier_phi_40_32_ladder_12;
                            _1003 = frontier_phi_40_32_ladder_13;
                        }
                        else
                        {
                            uint _564;
                            InterlockedAdd(_16[(_539 * 5u) + 3u], 1u, _564);
                            if (!(int(_564) < int(asuint(_25_m0[34u]).x)))
                            {
                                uint _684;
                                InterlockedAdd(_16[(asuint(_25_m0[28u]).z * 5u) + 3u], 4294967295u, _684);
                                break;
                            }
                            float _664 = _110 - _25_m0[27u].x;
                            float _665 = _111 - _25_m0[27u].y;
                            float _666 = _112 - _25_m0[27u].z;
                            float _670 = _433 * (_412 ? _166 : _194);
                            float _671 = _433 * (_412 ? _167 : _195);
                            float _672 = _433 * (_412 ? _168 : _196);
                            float _791;
                            float _793;
                            float _795;
                            if (max(_670, max(_671, _672)) < 65504.0f)
                            {
                                _791 = _670;
                                _793 = _671;
                                _795 = _672;
                            }
                            else
                            {
                                float _799 = _670 * 0.0625f;
                                float _800 = _671 * 0.0625f;
                                float _801 = _672 * 0.0625f;
                                float _803 = max(_799, max(_800, _801));
                                float _805 = min(_803, 65504.0f) / _803;
                                _791 = _805 * _799;
                                _793 = _805 * _800;
                                _795 = _805 * _801;
                            }
                            uint _797 = _251 & 7u;
                            float _1010;
                            if (_797 == 0u)
                            {
                                _1010 = 0.0f;
                            }
                            else
                            {
                                uint _1037 = ((_797 + 4294967295u) >> 2u) + 30u;
                                float _29[4];
                                _29[0u] = _25_m0[_1037].x;
                                _29[1u] = _25_m0[_1037].y;
                                _29[2u] = _25_m0[_1037].z;
                                _29[3u] = _25_m0[_1037].w;
                                _1010 = _29[(_251 + 3u) & 3u];
                            }
                            float _1000 = _201 * _201;
                            _984 = asuint(_25_m0[35u]).y + _564;
                            _985 = (spvPackHalf2x16(float2(_665, 0.0f)) << 16u) | spvPackHalf2x16(float2(_664, 0.0f));
                            _987 = (spvPackHalf2x16(float2(1.0f / _1000, 0.0f)) << 16u) | spvPackHalf2x16(float2(_666, 0.0f));
                            _989 = (spvPackHalf2x16(float2(_793, 0.0f)) << 16u) | spvPackHalf2x16(float2(_791, 0.0f));
                            _991 = (spvPackHalf2x16(float2(_1010, 0.0f)) << 16u) | spvPackHalf2x16(float2(_795, 0.0f));
                            _993 = 0u;
                            _994 = 0u;
                            _995 = 0u;
                            _996 = 0u;
                            _997 = 4294967295u;
                            _999 = _1000;
                            _1001 = _664;
                            _1002 = _665;
                            _1003 = _666;
                        }
                        if (!(int(_984) > int(4294967295u)))
                        {
                            break;
                        }
                        uint _1064 = _984 * 4u;
                        _13[_1064] = _985.x;
                        _13[_1064 + 1u] = _987.x;
                        _13[_1064 + 2u] = _989.x;
                        _13[_1064 + 3u] = _991.x;
                        uint _1073 = _984 * 4u;
                        _14[_1073] = uint4(0u, 0u, 0u, 0u).x;
                        _14[_1073 + 1u] = uint4(0u, 0u, 0u, 0u).x;
                        _14[_1073 + 2u] = uint4(0u, 0u, 0u, 0u).x;
                        _14[_1073 + 3u] = _993.x;
                        uint _1082 = _984 * 4u;
                        _15[_1082] = _994.x;
                        _15[_1082 + 1u] = _995.x;
                        _15[_1082 + 2u] = _996.x;
                        _15[_1082 + 3u] = uint4(0u, 0u, 0u, 0u).x;
                        uint _1091 = _984 * 5u;
                        _17[_1091] = asuint(_1001).x;
                        _17[_1091 + 1u] = asuint(_1002).x;
                        _17[_1091 + 2u] = asuint(_1003).x;
                        _17[(_984 * 5u) + 3u] = asuint(_999).x;
                        _17[(_984 * 5u) + 4u] = _997.x;
                        break;
                    }
                    case 1u:
                    case 3u:
                    {
                        float _444 = _25_m0[24u].w - _25_m0[18u].w;
                        float _447 = clamp((max(0.0f, _358 - _262) - _25_m0[18u].w) / _444, 0.0f, 1.0f);
                        float _453 = clamp((_358 - _343) / (_344 - _343), 0.0f, 1.0f);
                        bool _457 = (_146 & 16u) != 0u;
                        precise float _459 = _447 * _453;
                        precise float _460 = _459 * _459;
                        float _462 = (3.0f - (_453 * 2.0f)) * (_337 * _96);
                        float _470 = clamp((max(0.0f, _358 - _201) - _25_m0[18u].w) / _444, 0.0f, 1.0f);
                        precise float _474 = _453 * _470;
                        precise float _475 = _474 * _474;
                        float _478 = ((_462 * (_457 ? _152 : _206)) * (3.0f - (_470 * 2.0f))) * _475;
                        bool _481 = _243 != 4294967295u;
                        float _576;
                        float _579;
                        uint _582;
                        float _583;
                        if ((_294 == 4u) || (_294 == 32u))
                        {
                            float frontier_phi_19_13_ladder;
                            uint frontier_phi_19_13_ladder_1;
                            float frontier_phi_19_13_ladder_2;
                            float frontier_phi_19_13_ladder_3;
                            if (_481)
                            {
                                if (!(asuint(_25_m0[29u]).w == 0u))
                                {
                                    break;
                                }
                                float frontier_phi_19_13_ladder_27_ladder;
                                uint frontier_phi_19_13_ladder_27_ladder_1;
                                float frontier_phi_19_13_ladder_27_ladder_2;
                                float frontier_phi_19_13_ladder_27_ladder_3;
                                if ((_146 & 8u) == 0u)
                                {
                                    frontier_phi_19_13_ladder_27_ladder = _478;
                                    frontier_phi_19_13_ladder_27_ladder_1 = 3u;
                                    frontier_phi_19_13_ladder_27_ladder_2 = _217;
                                    frontier_phi_19_13_ladder_27_ladder_3 = _201;
                                }
                                else
                                {
                                    frontier_phi_19_13_ladder_27_ladder = _25_m0[38u].y * _478;
                                    frontier_phi_19_13_ladder_27_ladder_1 = 3u;
                                    frontier_phi_19_13_ladder_27_ladder_2 = _25_m0[38u].w * _217;
                                    frontier_phi_19_13_ladder_27_ladder_3 = _25_m0[38u].z * _201;
                                }
                                frontier_phi_19_13_ladder = frontier_phi_19_13_ladder_27_ladder;
                                frontier_phi_19_13_ladder_1 = frontier_phi_19_13_ladder_27_ladder_1;
                                frontier_phi_19_13_ladder_2 = frontier_phi_19_13_ladder_27_ladder_2;
                                frontier_phi_19_13_ladder_3 = frontier_phi_19_13_ladder_27_ladder_3;
                            }
                            else
                            {
                                frontier_phi_19_13_ladder = _478;
                                frontier_phi_19_13_ladder_1 = 3u;
                                frontier_phi_19_13_ladder_2 = _217;
                                frontier_phi_19_13_ladder_3 = _201;
                            }
                            _576 = frontier_phi_19_13_ladder_3;
                            _579 = frontier_phi_19_13_ladder_2;
                            _582 = frontier_phi_19_13_ladder_1;
                            _583 = frontier_phi_19_13_ladder;
                        }
                        else
                        {
                            float frontier_phi_19_14_ladder;
                            uint frontier_phi_19_14_ladder_1;
                            float frontier_phi_19_14_ladder_2;
                            float frontier_phi_19_14_ladder_3;
                            if (_294 == 2u)
                            {
                                frontier_phi_19_14_ladder = _478;
                                frontier_phi_19_14_ladder_1 = 3u;
                                frontier_phi_19_14_ladder_2 = _217;
                                frontier_phi_19_14_ladder_3 = _201;
                            }
                            else
                            {
                                float frontier_phi_19_14_ladder_20_ladder;
                                uint frontier_phi_19_14_ladder_20_ladder_1;
                                float frontier_phi_19_14_ladder_20_ladder_2;
                                float frontier_phi_19_14_ladder_20_ladder_3;
                                if ((_361 == 3u) && _481)
                                {
                                    frontier_phi_19_14_ladder_20_ladder = _25_m0[41u].y * _478;
                                    frontier_phi_19_14_ladder_20_ladder_1 = 3u;
                                    frontier_phi_19_14_ladder_20_ladder_2 = _25_m0[41u].w * _217;
                                    frontier_phi_19_14_ladder_20_ladder_3 = _25_m0[41u].z * _201;
                                }
                                else
                                {
                                    frontier_phi_19_14_ladder_20_ladder = _478;
                                    frontier_phi_19_14_ladder_20_ladder_1 = _361;
                                    frontier_phi_19_14_ladder_20_ladder_2 = _217;
                                    frontier_phi_19_14_ladder_20_ladder_3 = _201;
                                }
                                frontier_phi_19_14_ladder = frontier_phi_19_14_ladder_20_ladder;
                                frontier_phi_19_14_ladder_1 = frontier_phi_19_14_ladder_20_ladder_1;
                                frontier_phi_19_14_ladder_2 = frontier_phi_19_14_ladder_20_ladder_2;
                                frontier_phi_19_14_ladder_3 = frontier_phi_19_14_ladder_20_ladder_3;
                            }
                            _576 = frontier_phi_19_14_ladder_3;
                            _579 = frontier_phi_19_14_ladder_2;
                            _582 = frontier_phi_19_14_ladder_1;
                            _583 = frontier_phi_19_14_ladder;
                        }
                        if (!(max(((_462 * (_457 ? _152 : _257)) * (3.0f - (_447 * 2.0f))) * _460, _478) > 0.0f))
                        {
                            break;
                        }
                        float _690 = _583 * (_457 ? _166 : _194);
                        float _691 = _583 * (_457 ? _167 : _195);
                        float _692 = _583 * (_457 ? _168 : _196);
                        float _811;
                        float _813;
                        float _815;
                        uint _817;
                        if (max(_690, max(_691, _692)) < 65504.0f)
                        {
                            _811 = _690;
                            _813 = _691;
                            _815 = _692;
                            _817 = 0u;
                        }
                        else
                        {
                            float _839 = _690 * 0.0625f;
                            float _840 = _691 * 0.0625f;
                            float _841 = _692 * 0.0625f;
                            float _843 = max(_839, max(_840, _841));
                            float _845 = min(_843, 65504.0f) / _843;
                            _811 = _845 * _839;
                            _813 = _845 * _840;
                            _815 = _845 * _841;
                            _817 = 8u;
                        }
                        float _824 = _579 * 0.008726646192371845245361328125f;
                        float _825 = cos(_824);
                        float _826 = 1.0f / _825;
                        uint _833 = _141 + 1u;
                        uint _837 = _293.z;
                        uint _1135;
                        if ((_582 == 1u) && (!_306))
                        {
                            uint _1054;
                            InterlockedAdd(_16[(_837 * 5u) + 1u], 1u, _1054);
                            if (!(int(_1054) < int(_383.z)))
                            {
                                uint _1120;
                                InterlockedAdd(_16[(asuint(_25_m0[28u]).z * 5u) + 1u], 4294967295u, _1120);
                                break;
                            }
                            _1135 = asuint(_25_m0[34u]).w + _1054;
                        }
                        else
                        {
                            uint _1060;
                            InterlockedAdd(_16[(_837 * 5u) + 2u], 1u, _1060);
                            if (!(int(_1060) < int(_383.w)))
                            {
                                uint _1134;
                                InterlockedAdd(_16[(asuint(_25_m0[28u]).z * 5u) + 2u], 4294967295u, _1134);
                                break;
                            }
                            _1135 = asuint(_25_m0[35u]).x + _1060;
                        }
                        if (!(int(_1135) > int(4294967295u)))
                        {
                            break;
                        }
                        uint _1201 = spvPackHalf2x16(float2(_771, 0.0f));
                        uint _1212 = _1135 * 4u;
                        _13[_1212] = ((spvPackHalf2x16(float2(_351, 0.0f)) << 16u) | spvPackHalf2x16(float2(_350, 0.0f))).x;
                        _13[_1212 + 1u] = ((spvPackHalf2x16(float2(1.0f / _576, 0.0f)) << 16u) | spvPackHalf2x16(float2(_352, 0.0f))).x;
                        _13[_1212 + 2u] = ((spvPackHalf2x16(float2(_813, 0.0f)) << 16u) | spvPackHalf2x16(float2(_811, 0.0f))).x;
                        _13[_1212 + 3u] = (spvPackHalf2x16(float2(_815, 0.0f)) | (((((((_146 >> 4u) & 49152u) | ((_146 >> 12u) & 48u)) | (_251 & 7u)) | (_281 << 6u)) | _817) << 16u)).x;
                        uint _1221 = _1135 * 4u;
                        _14[_1221] = ((spvPackHalf2x16(float2(_129, 0.0f)) << 16u) | spvPackHalf2x16(float2(_128, 0.0f))).x;
                        _14[_1221 + 1u] = ((spvPackHalf2x16(float2(_131, 0.0f)) << 16u) | spvPackHalf2x16(float2(_130, 0.0f))).x;
                        _14[_1221 + 2u] = ((spvPackHalf2x16(float2(_825, 0.0f)) << 16u) | spvPackHalf2x16(float2(cos(max(_579 - _211, 0.0f) * 0.008726646192371845245361328125f), 0.0f))).x;
                        _14[_1221 + 3u] = (spvPackHalf2x16(float2(rsqrt(abs(1.0f - (_826 * _826))) * 0.49500000476837158203125f, 0.0f)) | ((((_9.Load((_136 * 24u) + 12u).x << 12u) | (_833 & 127u)) | (((_243 << 7u) + 128u) & 3968u)) << 16u)).x;
                        uint _1230 = _1135 * 4u;
                        _15[_1230] = ((_1201 << 16u) | spvPackHalf2x16(float2(_272, 0.0f))).x;
                        _15[_1230 + 1u] = _1201.x;
                        _15[_1230 + 2u] = ((spvPackHalf2x16(0.0f.xx) << 16u) | spvPackHalf2x16(float2(_278, 0.0f))).x;
                        _15[_1230 + 3u] = uint4(0u, 0u, 0u, 0u).x;
                        uint _1240;
                        if (_833 == 0u)
                        {
                            _1240 = 4294967295u;
                        }
                        else
                        {
                            _1240 = _10.Load((_141 * 28u) + 2u).x;
                        }
                        float _1242 = (-0.0f) - _129;
                        float _1244 = _129 * _1242;
                        float _1247 = _130 * _131;
                        float _1249 = _1247 - (_128 * _1242);
                        float _1250 = _128 * _130;
                        float _1251 = _129 * _131;
                        float _1252 = _1250 - _1251;
                        float _1254 = _1249 * 2.0f;
                        float _1255 = _1252 * 2.0f;
                        float _1256 = ((_1244 - (_130 * _130)) * 2.0f) + 1.0f;
                        float _1257 = (-0.0f) - _130;
                        float _1261 = _128 * _128;
                        float _1263 = _128 * _131;
                        float _1268 = (-0.0f) - _128;
                        float _1275 = _579 * 0.0043633230961859226226806640625f;
                        float _1277 = sin(_1275);
                        float _1278 = _1277 * 2.0f;
                        float _1279 = _1278 * ((_129 * _128) - _1247);
                        float _1280 = _1277 * ((((_130 * _1257) - _1261) * 2.0f) + 1.0f);
                        float _1281 = _1278 * (_1263 - (_129 * _1257));
                        float _1282 = cos(_1275);
                        float _1295 = ((_1280 * _1255) - (_1281 * _1254)) + (_1282 * _1256);
                        float _1296 = ((_1281 * _1256) - (_1279 * _1255)) + (_1282 * _1254);
                        float _1297 = ((_1279 * _1254) - (_1280 * _1256)) + (_1282 * _1255);
                        bool _1314 = _824 > 1.3962633609771728515625f;
                        float _1332;
                        float _1333;
                        float _1334;
                        if (_1314)
                        {
                            float _1321 = (-0.0f) - _131;
                            _1332 = ((_129 * _1321) - _1250) * 2.0f;
                            _1333 = (_130 * _1242) - (_128 * _1321);
                            _1334 = ((_1261 - _1244) * 2.0f) + (-1.0f);
                        }
                        else
                        {
                            _1332 = (((_1297 * _1280) - (_1296 * _1281)) * 2.0f) + _1256;
                            _1333 = (_1249 - (_1297 * _1279)) + (_1295 * _1281);
                            _1334 = (((_1296 * _1279) + _1252) - (_1295 * _1280)) * 2.0f;
                        }
                        precise float _1335 = _1333 * 2.0f;
                        uint _1338 = _1135 * 5u;
                        _17[_1338] = asuint(_350).x;
                        _17[_1338 + 1u] = asuint(_351).x;
                        _17[_1338 + 2u] = asuint(_352).x;
                        _17[(_1135 * 5u) + 3u] = asuint(_201 * _201).x;
                        _17[(_1135 * 5u) + 4u] = _1240.x;
                        _18[_1135 * 8u] = uint(_1314).x;
                        uint _1358 = (_1135 * 8u) + 1u;
                        _18[_1358] = asuint(_1332).x;
                        _18[_1358 + 1u] = asuint(_1335).x;
                        _18[_1358 + 2u] = asuint(_1334).x;
                        uint _1368 = (_1135 * 8u) + 4u;
                        _18[_1368] = asuint(_1251 - (_130 * _1268)).x;
                        _18[_1368 + 1u] = asuint((_130 * _129) - _1263).x;
                        _18[_1368 + 2u] = asuint((0.5f - (_129 * _129)) + (_128 * _1268)).x;
                        _18[_1368 + 3u] = asuint(0.866025388240814208984375f).x;
                        break;
                    }
                    case 4u:
                    {
                        float _487 = (_174 * 0.5f) + _180;
                        float _493 = max(1.0f, 1500.0f / _25_m0[18u].w);
                        float _497 = _493 * _25_m0[18u].w;
                        float _501 = (_25_m0[24u].w - _25_m0[18u].w) * _493;
                        float _504 = clamp((max(0.0f, _358 - _262) - _497) / _501, 0.0f, 1.0f);
                        float _510 = clamp((_358 - _343) / (_344 - _343), 0.0f, 1.0f);
                        bool _514 = (_146 & 16u) != 0u;
                        precise float _516 = _504 * _510;
                        precise float _517 = _516 * _516;
                        float _519 = (3.0f - (_510 * 2.0f)) * (_337 * _96);
                        float _527 = clamp((max(0.0f, _358 - _201) - _497) / _501, 0.0f, 1.0f);
                        precise float _531 = _510 * _527;
                        precise float _532 = _531 * _531;
                        float _535 = ((_519 * (_514 ? _152 : _206)) * (3.0f - (_527 * 2.0f))) * _532;
                        if (!(max(((_519 * (_514 ? _152 : _257)) * (3.0f - (_504 * 2.0f))) * _517, _535) > 0.0f))
                        {
                            break;
                        }
                        float _545 = _535 * (_514 ? _166 : _194);
                        float _546 = _535 * (_514 ? _167 : _195);
                        float _547 = _535 * (_514 ? _168 : _196);
                        float _588;
                        float _590;
                        float _592;
                        uint _594;
                        if (max(_545, max(_546, _547)) < 65504.0f)
                        {
                            _588 = _545;
                            _590 = _546;
                            _592 = _547;
                            _594 = 0u;
                        }
                        else
                        {
                            float _615 = _545 * 0.0625f;
                            float _617 = _546 * 0.0625f;
                            float _618 = _547 * 0.0625f;
                            float _620 = max(_615, max(_617, _618));
                            float _622 = min(_620, 65504.0f) / _620;
                            _588 = _622 * _615;
                            _590 = _622 * _617;
                            _592 = _622 * _618;
                            _594 = 8u;
                        }
                        uint _609;
                        InterlockedAdd(_16[(_293.z * 5u) + 4u], 1u, _609);
                        if (int(_609) < int(asuint(_25_m0[34u]).y))
                        {
                            uint _706 = asuint(_25_m0[35u]).z + _609;
                            if (!(int(_706) > int(4294967295u)))
                            {
                                break;
                            }
                            uint _903 = spvPackHalf2x16(float2(_771, 0.0f));
                            uint _914 = _706 * 4u;
                            _13[_914] = ((spvPackHalf2x16(float2(_351, 0.0f)) << 16u) | spvPackHalf2x16(float2(_350, 0.0f))).x;
                            _13[_914 + 1u] = ((spvPackHalf2x16(float2(2.0f / _180, 0.0f)) << 16u) | spvPackHalf2x16(float2(_352, 0.0f))).x;
                            _13[_914 + 2u] = ((spvPackHalf2x16(float2(_590, 0.0f)) << 16u) | spvPackHalf2x16(float2(_588, 0.0f))).x;
                            _13[_914 + 3u] = (spvPackHalf2x16(float2(_592, 0.0f)) | (((((((_146 >> 4u) & 49152u) | ((_146 >> 12u) & 48u)) | (_251 & 7u)) | (_281 << 6u)) | _594) << 16u)).x;
                            uint _923 = _706 * 4u;
                            _14[_923] = ((spvPackHalf2x16(float2(_129, 0.0f)) << 16u) | spvPackHalf2x16(float2(_128, 0.0f))).x;
                            _14[_923 + 1u] = ((spvPackHalf2x16(float2(_131, 0.0f)) << 16u) | spvPackHalf2x16(float2(_130, 0.0f))).x;
                            _14[_923 + 2u] = ((spvPackHalf2x16(float2(cos(_217 * 0.008726646192371845245361328125f), 0.0f)) << 16u) | spvPackHalf2x16(float2(cos(max(_217 - _211, 0.0f) * 0.008726646192371845245361328125f), 0.0f))).x;
                            _14[_923 + 3u] = ((spvPackHalf2x16(float2(_180, 0.0f)) << 16u) | spvPackHalf2x16(float2(_174, 0.0f))).x;
                            uint _932 = _706 * 4u;
                            _15[_932] = ((_903 << 16u) | spvPackHalf2x16(float2(_272, 0.0f))).x;
                            _15[_932 + 1u] = _903.x;
                            _15[_932 + 2u] = ((spvPackHalf2x16(0.0f.xx) << 16u) | spvPackHalf2x16(float2(_278, 0.0f))).x;
                            _15[_932 + 3u] = uint4(0u, 0u, 0u, 0u).x;
                            uint _942 = _706 * 5u;
                            _17[_942] = asuint(_350).x;
                            _17[_942 + 1u] = asuint(_351).x;
                            _17[_942 + 2u] = asuint(_352).x;
                            _17[(_706 * 5u) + 3u] = asuint(_487 * _487).x;
                            _17[(_706 * 5u) + 4u] = uint4(0u, 0u, 0u, 0u).x;
                            _18[_706 * 8u] = uint4(0u, 0u, 0u, 0u).x;
                            uint _962 = (_706 * 8u) + 1u;
                            _18[_962] = asuint(_180).x;
                            _18[_962 + 1u] = asuint(_174).x;
                            _18[_962 + 2u] = asuint(0.0f).x;
                            uint _972 = (_706 * 8u) + 4u;
                            _18[_972] = asuint(0.0f).x;
                            _18[_972 + 1u] = asuint(0.0f).x;
                            _18[_972 + 2u] = asuint(0.0f).x;
                            _18[_972 + 3u] = asuint(0.0f).x;
                            break;
                        }
                        else
                        {
                            uint _716;
                            InterlockedAdd(_16[(asuint(_25_m0[28u]).z * 5u) + 4u], 4294967295u, _716);
                            break;
                        }
                        break; // unreachable workaround
                    }
                }
            }
        }
    }
}

[numthreads(64, 1, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
