static uint _1986;
static uint _1987;
static uint _1988;
static uint _1996;
static uint _1997;
static uint _2089;
static uint _2090;
static uint _2265;
static uint _2266;
static uint _2507;
static uint _2508;

cbuffer _41_43 : register(b2, space0)
{
    float4 _43_m0[700] : packoffset(c0);
};

cbuffer _46_48 : register(b3, space0)
{
    float4 _48_m0[1138] : packoffset(c0);
};

cbuffer _51_53 : register(b0, space0)
{
    float4 _53_m0[2] : packoffset(c0);
};

Texture2D<float4> _9[] : register(t0, space6);
Texture3D<float4> _13[] : register(t0, space7);
Texture2D<float4> _15 : register(t115, space0);
TextureCube<float4> _18 : register(t117, space0);
Buffer<uint4> _22 : register(t109, space0);
Texture3D<uint4> _25 : register(t0, space12);
Buffer<uint4> _26 : register(t1, space12);
Buffer<uint4> _27 : register(t2, space12);
Buffer<uint4> _28 : register(t3, space12);
Buffer<uint4> _29 : register(t9, space12);
Texture2DArray<float4> _32 : register(t4, space12);
TextureCube<float4> _33 : register(t0, space0);
Buffer<uint4> _34 : register(t16, space0);
RWBuffer<float4> _37 : register(u0, space0);
SamplerState _56 : register(s0, space0);
SamplerState _57 : register(s2, space0);
SamplerState _58 : register(s5, space0);
SamplerState _59 : register(s1, space0);
SamplerState _60 : register(s3, space0);
SamplerComparisonState _61 : register(s9, space0);
SamplerComparisonState _62 : register(s10, space0);

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
    uint4 _75 = asuint(_53_m0[1u]);
    if (gl_GlobalInvocationID.x < _75.x)
    {
        uint4 _82 = _34.Load(gl_GlobalInvocationID.x * 3u);
        uint _83 = _82.x;
        uint4 _86 = _34.Load((gl_GlobalInvocationID.x * 3u) + 1u);
        uint _87 = _86.x;
        uint4 _90 = _34.Load((gl_GlobalInvocationID.x * 3u) + 2u);
        uint _91 = _90.x;
        float _107 = float((_87 >> 8u) & 255u);
        float _109 = float(_87 & 255u);
        float _112 = float(_91 >> 24u);
        float _125 = (float((_91 >> 16u) & 255u) * 0.007843137718737125396728515625f) + (-1.0f);
        float _127 = (float((_91 >> 8u) & 255u) * 0.007843137718737125396728515625f) + (-1.0f);
        float _128 = (float(_91 & 255u) * 0.007843137718737125396728515625f) + (-1.0f);
        float _133 = rsqrt(dot(float3(_125, _127, _128), float3(_125, _127, _128)));
        float _134 = _125 * _133;
        float _135 = _127 * _133;
        float _136 = _128 * _133;
        float _142 = _53_m0[0u].x + spvUnpackHalf2x16(_83 >> 16u).x;
        float _143 = _53_m0[0u].y + spvUnpackHalf2x16(_83).x;
        float _144 = _53_m0[0u].z + spvUnpackHalf2x16(_87 >> 16u).x;
        uint _146 = _75.y;
        float _152 = _142 - _43_m0[8u].x;
        float _153 = _143 - _43_m0[8u].y;
        float _154 = _144 - _43_m0[8u].z;
        uint4 _163 = asuint(_43_m0[69u]);
        bool _165 = _163.w == 1u;
        uint _166 = _163.y;
        uint _167 = _166 + 4294967295u;
        float _177;
        if (_43_m0[30u].x > 0.0f)
        {
            float _374;
            if (_163.x == 1u)
            {
                float _281 = _48_m0[160u].y * (_142 - _43_m0[8u].x);
                float _284 = _48_m0[160u].y * (_144 - _43_m0[8u].z);
                float _293 = (_48_m0[160u].w + _48_m0[160u].x) + sqrt(_48_m0[160u].z - dot(float3(_281, _48_m0[160u].x, _284), float3(_281, _48_m0[160u].x, _284)));
                float _297 = rsqrt(dot(float3(_281, _293, _284), float3(_281, _293, _284)));
                float _299 = _293 * _297;
                float4 _305 = _18.SampleLevel(_59, float3(_297 * _281, _299, _297 * _284), 0.0f);
                float _307 = _305.x;
                float _314 = (_48_m0[164u].w - _143) / _48_m0[164u].y;
                _374 = (((1.0f - _307) + ((((1.0f - clamp(exp2(log2(clamp(1.0f - dot(float4(_15.SampleLevel(_59, float2((((_48_m0[164u].x * _314) + _142) * _48_m0[161u].x) + _48_m0[161u].z, (((_48_m0[164u].z * _314) + _144) * _48_m0[161u].x) + _48_m0[161u].w), 0.0f)), float4(_48_m0[163u])), 0.0f, 1.0f)) * _48_m0[162u].z), 0.0f, 1.0f)) * _48_m0[162u].w) + (-1.0f)) * _48_m0[161u].y)) * clamp((_299 - _48_m0[162u].x) * _48_m0[162u].y, 0.0f, 1.0f)) + _307;
            }
            else
            {
                _374 = 1.0f;
            }
            float _403;
            if (int(_166) < int(1u))
            {
                _403 = 1.0f;
            }
            else
            {
                float4 _430 = _9[584u].SampleLevel(_56, float2((_142 - _43_m0[111u].x) * _43_m0[111u].z, 1.0f - ((_144 - _43_m0[111u].y) * _43_m0[111u].w)), 0.0f);
                float _407 = 1.0f - (clamp((((_43_m0[112u].x - _143) - _43_m0[112u].w) + (_43_m0[112u].y * _430.x)) / _43_m0[112u].z, 0.0f, 1.0f) * clamp(_430.y, 0.0f, 1.0f));
                float frontier_phi_9_10_ladder;
                if (int(asuint(_43_m0[69u]).y) > int(0u))
                {
                    float frontier_phi_24;
                    uint _529 = 0u;
                    bool _531;
                    for (;;)
                    {
                        _531 = int(_529) > int(_167);
                        if (!_531)
                        {
                            uint _682 = _529 * 36u;
                            float3 _693 = asfloat(uint3(_22.Load(_682).x, _22.Load(_682 + 1u).x, _22.Load(_682 + 2u).x));
                            uint _698 = (_529 * 36u) + 4u;
                            float3 _708 = asfloat(uint3(_22.Load(_698).x, _22.Load(_698 + 1u).x, _22.Load(_698 + 2u).x));
                            uint _712 = (_529 * 36u) + 8u;
                            float3 _722 = asfloat(uint3(_22.Load(_712).x, _22.Load(_712 + 1u).x, _22.Load(_712 + 2u).x));
                            uint _727 = (_529 * 36u) + 12u;
                            float3 _737 = asfloat(uint3(_22.Load(_727).x, _22.Load(_727 + 1u).x, _22.Load(_727 + 2u).x));
                            uint _741 = (_529 * 36u) + 16u;
                            float3 _751 = asfloat(uint3(_22.Load(_741).x, _22.Load(_741 + 1u).x, _22.Load(_741 + 2u).x));
                            float _752 = _751.x;
                            float _753 = _751.y;
                            uint _756 = (_529 * 36u) + 20u;
                            float3 _766 = asfloat(uint3(_22.Load(_756).x, _22.Load(_756 + 1u).x, _22.Load(_756 + 2u).x));
                            uint _771 = (_529 * 36u) + 24u;
                            float3 _781 = asfloat(uint3(_22.Load(_771).x, _22.Load(_771 + 1u).x, _22.Load(_771 + 2u).x));
                            uint4 _788 = _22.Load((_529 * 36u) + 28u);
                            uint _789 = _788.x;
                            float _795 = asfloat(_22.Load((_529 * 36u) + 32u).x);
                            float _810 = (((_152 - _766.x) - _781.x) + _43_m0[23u].x) + _43_m0[24u].x;
                            float _814 = (((_153 - _766.y) - _781.y) + _43_m0[23u].y) + _43_m0[24u].y;
                            float _818 = (((_154 - _766.z) - _781.z) + _43_m0[23u].z) + _43_m0[24u].z;
                            float _822 = mad(_722.x, _818, mad(_708.x, _814, _810 * _693.x)) + _737.x;
                            float _826 = mad(_722.y, _818, mad(_708.y, _814, _810 * _693.y)) + _737.y;
                            if ((clamp(_822, 0.0f, 1.0f) == _822) && (clamp(_826, 0.0f, 1.0f) == _826))
                            {
                                float _945 = _43_m0[30u].z + (clamp(mad(_722.z, _818, mad(_708.z, _814, _810 * _693.z)) + _737.z, 0.0f, 1.0f) - (_751.z * _53_m0[0u].w));
                                uint _948 = (_529 + 40u) + 0u;
                                float _404;
                                if (int(_789) > int(15u))
                                {
                                    float _1020 = frac((_822 * _752) + 0.5f);
                                    float _1021 = frac((_826 * _752) + 0.5f);
                                    float _1022 = 1.0f - _1020;
                                    float _1028 = ((0.5f - _1020) * _753) + _822;
                                    float _1029 = ((0.5f - _1021) * _753) + _826;
                                    float4 _1038 = _9[NonUniformResourceIndex(_948)].GatherCmp(_61, float2(_1028, _1029), _945, int2(-1, 1));
                                    float4 _1046 = _9[NonUniformResourceIndex(_948)].GatherCmp(_61, float2(_1028, _1029), _945, int2(1, 1));
                                    float4 _1053 = _9[NonUniformResourceIndex(_948)].GatherCmp(_61, float2(_1028, _1029), _945, int2(1, -1));
                                    float4 _1060 = _9[NonUniformResourceIndex(_948)].GatherCmp(_61, float2(_1028, _1029), _945, int2(-1, -1));
                                    _404 = (((((((_1046.w + _1038.z) + _1053.x) + _1060.y) + ((_1053.y + _1046.z) * _1020)) + (((((_1038.x * _1022) + _1038.y) + _1046.x) + (_1046.y * _1020)) * _1021)) + ((_1060.x + _1038.w) * _1022)) + (((((_1053.z * _1020) + _1053.w) + _1060.z) + (_1060.w * _1022)) * (1.0f - _1021))) * 0.111111111938953399658203125f;
                                }
                                else
                                {
                                    float frontier_phi_36_31_ladder;
                                    if (_789 == 4u)
                                    {
                                        frontier_phi_36_31_ladder = _9[NonUniformResourceIndex(_948)].SampleCmpLevelZero(_62, float2(_822, _826), _945).xxxx.x;
                                    }
                                    else
                                    {
                                        frontier_phi_36_31_ladder = 1.0f;
                                    }
                                    _404 = frontier_phi_36_31_ladder;
                                }
                                if (_529 == (asuint(_43_m0[69u]).y + 4294967295u))
                                {
                                    frontier_phi_24 = (clamp(min(min(_822, _826), min(1.0f - _822, 1.0f - _826)) / _795, 0.0f, 1.0f) * (_404 + (-1.0f))) + 1.0f;
                                    break;
                                }
                                else
                                {
                                    if (!(_165 && (int(_167) > int(_529))))
                                    {
                                        frontier_phi_24 = _404;
                                        break;
                                    }
                                    float _1480 = clamp(min(min(_822, _826), min(1.0f - _822, 1.0f - _826)) / _795, 0.0f, 1.0f);
                                    if (!(_1480 < 1.0f))
                                    {
                                        frontier_phi_24 = _404;
                                        break;
                                    }
                                    uint _1576 = (_529 * 36u) + 36u;
                                    float3 _1586 = asfloat(uint3(_22.Load(_1576).x, _22.Load(_1576 + 1u).x, _22.Load(_1576 + 2u).x));
                                    uint _1591 = (_529 * 36u) + 40u;
                                    float3 _1601 = asfloat(uint3(_22.Load(_1591).x, _22.Load(_1591 + 1u).x, _22.Load(_1591 + 2u).x));
                                    uint _1607 = (_529 * 36u) + 44u;
                                    float3 _1617 = asfloat(uint3(_22.Load(_1607).x, _22.Load(_1607 + 1u).x, _22.Load(_1607 + 2u).x));
                                    uint _1622 = (_529 * 36u) + 48u;
                                    float3 _1632 = asfloat(uint3(_22.Load(_1622).x, _22.Load(_1622 + 1u).x, _22.Load(_1622 + 2u).x));
                                    uint _1638 = (_529 * 36u) + 52u;
                                    float3 _1648 = asfloat(uint3(_22.Load(_1638).x, _22.Load(_1638 + 1u).x, _22.Load(_1638 + 2u).x));
                                    float _1649 = _1648.x;
                                    float _1650 = _1648.y;
                                    uint _1654 = (_529 * 36u) + 56u;
                                    float3 _1664 = asfloat(uint3(_22.Load(_1654).x, _22.Load(_1654 + 1u).x, _22.Load(_1654 + 2u).x));
                                    uint _1670 = (_529 * 36u) + 60u;
                                    float3 _1680 = asfloat(uint3(_22.Load(_1670).x, _22.Load(_1670 + 1u).x, _22.Load(_1670 + 2u).x));
                                    uint4 _1687 = _22.Load((_529 * 36u) + 64u);
                                    uint _1688 = _1687.x;
                                    float _1702 = (((_152 - _1664.x) - _1680.x) + _43_m0[23u].x) + _43_m0[24u].x;
                                    float _1706 = (((_153 - _1664.y) - _1680.y) + _43_m0[23u].y) + _43_m0[24u].y;
                                    float _1710 = (((_154 - _1664.z) - _1680.z) + _43_m0[23u].z) + _43_m0[24u].z;
                                    float _1714 = mad(_1617.x, _1710, mad(_1601.x, _1706, _1702 * _1586.x)) + _1632.x;
                                    float _1718 = mad(_1617.y, _1710, mad(_1601.y, _1706, _1702 * _1586.y)) + _1632.y;
                                    float _1729 = _43_m0[30u].z + (clamp(mad(_1617.z, _1710, mad(_1601.z, _1706, _1702 * _1586.z)) + _1632.z, 0.0f, 1.0f) - (_1648.z * _53_m0[0u].w));
                                    uint _1732 = (_529 + 41u) + 0u;
                                    float _1936;
                                    if (int(_1688) > int(15u))
                                    {
                                        float _1814 = frac((_1714 * _1649) + 0.5f);
                                        float _1815 = frac((_1718 * _1649) + 0.5f);
                                        float _1816 = 1.0f - _1814;
                                        float _1822 = ((0.5f - _1814) * _1650) + _1714;
                                        float _1823 = ((0.5f - _1815) * _1650) + _1718;
                                        float4 _1829 = _9[NonUniformResourceIndex(_1732)].GatherCmp(_61, float2(_1822, _1823), _1729, int2(-1, 1));
                                        float4 _1835 = _9[NonUniformResourceIndex(_1732)].GatherCmp(_61, float2(_1822, _1823), _1729, int2(1, 1));
                                        float4 _1841 = _9[NonUniformResourceIndex(_1732)].GatherCmp(_61, float2(_1822, _1823), _1729, int2(1, -1));
                                        float4 _1847 = _9[NonUniformResourceIndex(_1732)].GatherCmp(_61, float2(_1822, _1823), _1729, int2(-1, -1));
                                        _1936 = (((((((_1835.w + _1829.z) + _1841.x) + _1847.y) + ((_1841.y + _1835.z) * _1814)) + (((((_1829.x * _1816) + _1829.y) + _1835.x) + (_1835.y * _1814)) * _1815)) + ((_1847.x + _1829.w) * _1816)) + (((((_1841.z * _1814) + _1841.w) + _1847.z) + (_1847.w * _1816)) * (1.0f - _1815))) * 0.111111111938953399658203125f;
                                    }
                                    else
                                    {
                                        float frontier_phi_70_65_ladder;
                                        if (_1688 == 4u)
                                        {
                                            frontier_phi_70_65_ladder = _9[NonUniformResourceIndex(_1732)].SampleCmpLevelZero(_62, float2(_1714, _1718), _1729).xxxx.x;
                                        }
                                        else
                                        {
                                            frontier_phi_70_65_ladder = 1.0f;
                                        }
                                        _1936 = frontier_phi_70_65_ladder;
                                    }
                                    frontier_phi_24 = ((_404 - _1936) * _1480) + _1936;
                                    break;
                                }
                            }
                        }
                        uint _530 = _529 + 1u;
                        if (int(_530) < int(asuint(_43_m0[69u]).y))
                        {
                            _529 = _530;
                            continue;
                        }
                        else
                        {
                            frontier_phi_24 = _407;
                            break;
                        }
                    }
                    frontier_phi_9_10_ladder = frontier_phi_24;
                }
                else
                {
                    frontier_phi_9_10_ladder = _407;
                }
                _403 = frontier_phi_9_10_ladder;
            }
            _177 = ((min(_403, 1.0f) * _374) * _43_m0[30u].x) + _43_m0[30u].y;
        }
        else
        {
            _177 = 1.0f;
        }
        float _191 = _43_m0[8u].x + _152;
        float _192 = _43_m0[8u].y + _153;
        float _193 = _43_m0[8u].z + _154;
        uint _194 = _146 + 422u;
        uint _202 = ((asuint(_43_m0[_194]).w >> 24u) + 8u) + 0u;
        uint4 _205 = asuint(_43_m0[_194]);
        uint _206 = _205.w;
        bool _208 = (_206 & 2u) != 0u;
        uint _211 = _205.y;
        uint _212 = _205.x;
        float _213 = _134 * _134;
        float _214 = _135 * _135;
        float _215 = _136 * _136;
        float _219 = float(_134 < 0.0f);
        float _220 = (_135 < 0.0f) ? 3.0f : 2.0f;
        float _223 = (_136 < 0.0f) ? 5.0f : 4.0f;
        float4 _244 = _9[584u].SampleLevel(_56, float2((_191 - _43_m0[111u].x) * _43_m0[111u].z, 1.0f - ((_193 - _43_m0[111u].y) * _43_m0[111u].w)), 0.0f);
        float _262 = clamp((((_43_m0[112u].x - _192) - _43_m0[112u].w) + (_43_m0[112u].y * _244.x)) / _43_m0[112u].z, 0.0f, 1.0f) * clamp(_244.y, 0.0f, 1.0f);
        float _274 = (_135 + 1.0f) * 0.5f;
        float _453;
        if (asuint(_43_m0[119u]).x == 0u)
        {
            _453 = ((_43_m0[166u].w - _43_m0[166u].z) * _262) + _43_m0[166u].z;
        }
        else
        {
            _453 = _9[586u].SampleLevel(_56, float2((_191 - _43_m0[118u].x) * _43_m0[118u].z, (_193 - _43_m0[118u].y) * _43_m0[118u].w), 0.0f).w;
        }
        precise float _457 = exp2(log2(_453 * _274)) + (-1.0f);
        precise float _458 = _457 * _43_m0[159u].y;
        float _459 = _458 + 1.0f;
        uint _460 = _212 * 6u;
        uint _462 = _460 + 457u;
        uint _469 = _460 + 458u;
        uint _476 = _460 + 459u;
        uint _483 = _460 + 455u;
        uint _490 = _460 + 456u;
        uint _496 = _460 + 454u;
        uint _526;
        float _527;
        if (_208)
        {
            float _517 = max(abs((_152 * 2.0f) * _43_m0[_483].x), max(abs((_43_m0[_483].y * 2.0f) * (_43_m0[158u].w + _153)), abs((_154 * 2.0f) * _43_m0[_483].z)));
            _526 = uint(min(max(floor(log2(_517 / _43_m0[_483].w)) + 1.0f, 0.0f), float(_211)));
            _527 = _517;
        }
        else
        {
            _526 = 0u;
            _527 = 0.0f;
        }
        float _532;
        if (_526 == _211)
        {
            _532 = _459;
        }
        else
        {
            uint _631 = (_211 == 1u) ? 0u : _526;
            float _635 = float(1u << (_631 & 31u));
            uint _637 = (_631 + _212) * 6u;
            uint _638 = _637 + 456u;
            uint _642 = _637 + 454u;
            float _648 = _43_m0[_483].x / _635;
            float _649 = _43_m0[_483].y / _635;
            float _650 = _43_m0[_483].z / _635;
            float _656 = (_648 * _43_m0[_462].x) * (_191 - _43_m0[_642].x);
            float _658 = (_649 * _43_m0[_462].y) * (_192 - _43_m0[_642].y);
            float _660 = ((_193 - _43_m0[_642].z) * _43_m0[_462].z) * _650;
            float _661 = _43_m0[_462].x * 0.5f;
            float _662 = _648 * _661;
            float _663 = _43_m0[_462].y * 0.5f;
            float _664 = _649 * _663;
            float _665 = _43_m0[_462].z * 0.5f;
            float _666 = _650 * _665;
            float _533;
            if (((_656 < ((-0.0f) - _662)) || (_658 < ((-0.0f) - _664))) || (_660 < ((-0.0f) - _666)))
            {
                _533 = _459;
            }
            else
            {
                float frontier_phi_22_23_ladder;
                if (((_656 > ((_43_m0[_476].x + _43_m0[_469].x) + _662)) || (_658 > ((_43_m0[_476].y + _43_m0[_469].y) + _664))) || (_660 > ((_43_m0[_476].z + _43_m0[_469].z) + _666)))
                {
                    frontier_phi_22_23_ladder = _459;
                }
                else
                {
                    float _984 = min(max(_656, _43_m0[_469].x), _43_m0[_476].x) + _43_m0[_638].x;
                    float _985 = min(max(_658, _43_m0[_469].y), _43_m0[_476].y) + _43_m0[_490].y;
                    float _986 = min(max(_660, _43_m0[_469].z), _43_m0[_476].z) + _43_m0[_490].z;
                    frontier_phi_22_23_ladder = (_43_m0[159u].y * (dot(float3(_213, _214, _215), float3(_13[NonUniformResourceIndex(_202)].SampleLevel(_57, float3(_984, _985, _986 + (_43_m0[_462].z * _219)), 0.0f).x, _13[NonUniformResourceIndex(_202)].SampleLevel(_57, float3(_984, _985, _986 + (_43_m0[_462].z * _220)), 0.0f).x, _13[NonUniformResourceIndex(_202)].SampleLevel(_57, float3(_984, _985, _986 + (_43_m0[_462].z * _223)), 0.0f).x)) + (-1.0f))) + 1.0f;
                }
                _533 = frontier_phi_22_23_ladder;
            }
            float frontier_phi_16_22_ladder;
            if ((_206 & 1u) == 0u)
            {
                frontier_phi_16_22_ladder = _533;
            }
            else
            {
                float _1203;
                if (_208)
                {
                    float _1196 = clamp(((_527 / _635) - _43_m0[_496].w) / (_43_m0[_483].w - _43_m0[_496].w), 0.0f, 1.0f);
                    _1203 = (1.0f - ((_1196 * _1196) * (3.0f - (_1196 * 2.0f)))) * _43_m0[_638].w;
                }
                else
                {
                    _1203 = _43_m0[_638].w;
                }
                float frontier_phi_16_22_ladder_35_ladder;
                if (_1203 < 0.9900000095367431640625f)
                {
                    uint _1287 = _631 + 1u;
                    float _1434;
                    if (_1287 == _211)
                    {
                        _1434 = _459;
                    }
                    else
                    {
                        float _1439 = _635 * 2.0f;
                        uint _1440 = (_1287 + _212) * 6u;
                        uint _1441 = _1440 + 454u;
                        float _1447 = _43_m0[_483].x / _1439;
                        float _1448 = _43_m0[_483].y / _1439;
                        float _1449 = _43_m0[_483].z / _1439;
                        float _1458 = (_1447 * _43_m0[_462].x) * (_191 - _43_m0[_1441].x);
                        float _1460 = (_1448 * _43_m0[_462].y) * (_192 - _43_m0[_1441].y);
                        float _1462 = ((_193 - _43_m0[_1441].z) * _43_m0[_462].z) * _1449;
                        float _1463 = _1447 * _661;
                        float _1464 = _1448 * _663;
                        float _1465 = _1449 * _665;
                        float frontier_phi_51_52_ladder;
                        if (((_1458 < ((-0.0f) - _1463)) || (_1460 < ((-0.0f) - _1464))) || (_1462 < ((-0.0f) - _1465)))
                        {
                            frontier_phi_51_52_ladder = _459;
                        }
                        else
                        {
                            float frontier_phi_51_52_ladder_59_ladder;
                            if (((_1458 > ((_43_m0[_476].x + _43_m0[_469].x) + _1463)) || (_1460 > ((_43_m0[_476].y + _43_m0[_469].y) + _1464))) || (_1462 > ((_43_m0[_476].z + _43_m0[_469].z) + _1465)))
                            {
                                frontier_phi_51_52_ladder_59_ladder = _459;
                            }
                            else
                            {
                                float _1780 = min(max(_1458, _43_m0[_469].x), _43_m0[_476].x) + _43_m0[_1440 + 456u].x;
                                float _1781 = min(max(_1460, _43_m0[_469].y), _43_m0[_476].y) + _43_m0[_490].y;
                                float _1782 = min(max(_1462, _43_m0[_469].z), _43_m0[_476].z) + _43_m0[_490].z;
                                frontier_phi_51_52_ladder_59_ladder = (_43_m0[159u].y * (dot(float3(_213, _214, _215), float3(_13[NonUniformResourceIndex(_202)].SampleLevel(_57, float3(_1780, _1781, _1782 + (_43_m0[_462].z * _219)), 0.0f).x, _13[NonUniformResourceIndex(_202)].SampleLevel(_57, float3(_1780, _1781, _1782 + (_43_m0[_462].z * _220)), 0.0f).x, _13[NonUniformResourceIndex(_202)].SampleLevel(_57, float3(_1780, _1781, _1782 + (_43_m0[_462].z * _223)), 0.0f).x)) + (-1.0f))) + 1.0f;
                            }
                            frontier_phi_51_52_ladder = frontier_phi_51_52_ladder_59_ladder;
                        }
                        _1434 = frontier_phi_51_52_ladder;
                    }
                    frontier_phi_16_22_ladder_35_ladder = ((_533 - _1434) * _1203) + _1434;
                }
                else
                {
                    frontier_phi_16_22_ladder_35_ladder = _533;
                }
                frontier_phi_16_22_ladder = frontier_phi_16_22_ladder_35_ladder;
            }
            _532 = frontier_phi_16_22_ladder;
        }
        float _545 = clamp((_192 - _43_m0[165u].x) / (_43_m0[165u].y - _43_m0[165u].x), 0.0f, 1.0f);
        float _549 = (_545 * _545) * (3.0f - (_545 * 2.0f));
        float _568 = ((_43_m0[159u].y * (min(_532, ((_549 * (1.0f - _43_m0[165u].w)) + _43_m0[165u].w) * exp2(((_549 * (1.0f - _43_m0[165u].z)) + _43_m0[165u].z) * log2(_274))) + (-1.0f))) + 1.0f) * (1.0f - _262);
        uint _569 = _146 + 198u;
        uint _577 = ((asuint(_43_m0[_569]).w >> 24u) + 24u) + 0u;
        uint4 _580 = asuint(_43_m0[_569]);
        uint _581 = _580.w;
        bool _583 = (_581 & 2u) != 0u;
        uint _586 = _580.y;
        uint _587 = _580.x;
        float4 _607 = _9[584u].SampleLevel(_56, float2((_191 - _43_m0[111u].x) * _43_m0[111u].z, 1.0f - ((_193 - _43_m0[111u].y) * _43_m0[111u].w)), 0.0f);
        float _624 = clamp((((_43_m0[112u].x - _192) - _43_m0[112u].w) + (_43_m0[112u].y * _607.x)) / _43_m0[112u].z, 0.0f, 1.0f) * clamp(_607.y, 0.0f, 1.0f);
        float _874;
        float _875;
        float _876;
        if (asuint(_43_m0[119u]).x == 0u)
        {
            float _856 = ((_48_m0[1u].x / _43_m0[59u].w) * 0.3183098733425140380859375f) * (((_43_m0[164u].y - _43_m0[160u].x) * _624) + _43_m0[160u].x);
            float _858 = ((_48_m0[1u].y / _43_m0[59u].w) * 0.3183098733425140380859375f) * (((_43_m0[164u].z - _43_m0[160u].y) * _624) + _43_m0[160u].y);
            float _860 = ((_48_m0[1u].z / _43_m0[59u].w) * 0.3183098733425140380859375f) * (((_43_m0[164u].w - _43_m0[160u].z) * _624) + _43_m0[160u].z);
            float _861 = clamp(_274, 0.0f, 1.0f);
            _874 = (_856 - (_856 * _861)) * _43_m0[159u].z;
            _875 = (_858 - (_858 * _861)) * _43_m0[159u].z;
            _876 = (_860 - (_860 * _861)) * _43_m0[159u].z;
        }
        else
        {
            _874 = 0.0f;
            _875 = 0.0f;
            _876 = 0.0f;
        }
        uint _877 = _587 * 6u;
        uint _878 = _877 + 233u;
        uint _885 = _877 + 234u;
        uint _892 = _877 + 235u;
        uint _899 = _877 + 231u;
        uint _906 = _877 + 232u;
        uint _912 = _877 + 230u;
        uint _975;
        float _976;
        if (_583)
        {
            float _965 = max(abs((_152 * 2.0f) * _43_m0[_899].x), max(abs((_43_m0[_899].y * 2.0f) * (_43_m0[158u].w + _153)), abs((_154 * 2.0f) * _43_m0[_899].z)));
            _975 = uint(min(max(floor(log2(_965 / _43_m0[_899].w)) + 1.0f, float(asuint(_43_m0[85u]).y)), float(_586)));
            _976 = _965;
        }
        else
        {
            _975 = 0u;
            _976 = 0.0f;
        }
        float _1092;
        float _1095;
        float _1098;
        if (_975 == _586)
        {
            _1092 = _874;
            _1095 = _875;
            _1098 = _876;
        }
        else
        {
            uint _1149 = (_586 == 1u) ? 0u : _975;
            float _1152 = float(1u << (_1149 & 31u));
            uint _1154 = (_1149 + _587) * 6u;
            uint _1155 = _1154 + 232u;
            uint _1159 = _1154 + 230u;
            float _1165 = _43_m0[_899].x / _1152;
            float _1166 = _43_m0[_899].y / _1152;
            float _1167 = _43_m0[_899].z / _1152;
            float _1173 = (_1165 * _43_m0[_878].x) * (_191 - _43_m0[_1159].x);
            float _1175 = (_1166 * _43_m0[_878].y) * (_192 - _43_m0[_1159].y);
            float _1177 = ((_193 - _43_m0[_1159].z) * _43_m0[_878].z) * _1167;
            float _1178 = _43_m0[_878].x * 0.5f;
            float _1179 = _1165 * _1178;
            float _1180 = _43_m0[_878].y * 0.5f;
            float _1181 = _1166 * _1180;
            float _1182 = _43_m0[_878].z * 0.5f;
            float _1183 = _1167 * _1182;
            float _1093;
            float _1096;
            float _1099;
            if (((_1173 < ((-0.0f) - _1179)) || (_1175 < ((-0.0f) - _1181))) || (_1177 < ((-0.0f) - _1183)))
            {
                _1093 = _874;
                _1096 = _875;
                _1099 = _876;
            }
            else
            {
                float frontier_phi_40_41_ladder;
                float frontier_phi_40_41_ladder_1;
                float frontier_phi_40_41_ladder_2;
                if (((_1173 > ((_43_m0[_892].x + _43_m0[_885].x) + _1179)) || (_1175 > ((_43_m0[_892].y + _43_m0[_885].y) + _1181))) || (_1177 > ((_43_m0[_892].z + _43_m0[_885].z) + _1183)))
                {
                    frontier_phi_40_41_ladder = _876;
                    frontier_phi_40_41_ladder_1 = _875;
                    frontier_phi_40_41_ladder_2 = _874;
                }
                else
                {
                    float _1388 = min(max(_1173, _43_m0[_885].x), _43_m0[_892].x) + _43_m0[_1155].x;
                    float _1389 = min(max(_1175, _43_m0[_885].y), _43_m0[_892].y) + _43_m0[_906].y;
                    float _1390 = min(max(_1177, _43_m0[_885].z), _43_m0[_892].z) + _43_m0[_906].z;
                    float4 _1401 = _13[NonUniformResourceIndex(_577)].SampleLevel(_57, float3(_1388, _1389, _1390 + (_43_m0[_878].z * _219)), 0.0f);
                    float4 _1406 = _13[NonUniformResourceIndex(_577)].SampleLevel(_57, float3(_1388, _1389, _1390 + (_43_m0[_878].z * _220)), 0.0f);
                    float4 _1411 = _13[NonUniformResourceIndex(_577)].SampleLevel(_57, float3(_1388, _1389, _1390 + (_43_m0[_878].z * _223)), 0.0f);
                    frontier_phi_40_41_ladder = (((_1406.z * _214) + (_1401.z * _213)) + (_1411.z * _215)) * _43_m0[159u].z;
                    frontier_phi_40_41_ladder_1 = (((_1406.y * _214) + (_1401.y * _213)) + (_1411.y * _215)) * _43_m0[159u].z;
                    frontier_phi_40_41_ladder_2 = (((_1406.x * _214) + (_1401.x * _213)) + (_1411.x * _215)) * _43_m0[159u].z;
                }
                _1093 = frontier_phi_40_41_ladder_2;
                _1096 = frontier_phi_40_41_ladder_1;
                _1099 = frontier_phi_40_41_ladder;
            }
            float frontier_phi_32_40_ladder;
            float frontier_phi_32_40_ladder_1;
            float frontier_phi_32_40_ladder_2;
            if ((_581 & 1u) == 0u)
            {
                frontier_phi_32_40_ladder = _1093;
                frontier_phi_32_40_ladder_1 = _1096;
                frontier_phi_32_40_ladder_2 = _1099;
            }
            else
            {
                float _1560;
                if (_583)
                {
                    float _1553 = clamp(((_976 / _1152) - _43_m0[_912].w) / (_43_m0[_899].w - _43_m0[_912].w), 0.0f, 1.0f);
                    _1560 = (1.0f - ((_1553 * _1553) * (3.0f - (_1553 * 2.0f)))) * _43_m0[_1155].w;
                }
                else
                {
                    _1560 = _43_m0[_1155].w;
                }
                float frontier_phi_32_40_ladder_58_ladder;
                float frontier_phi_32_40_ladder_58_ladder_1;
                float frontier_phi_32_40_ladder_58_ladder_2;
                if (_1560 < 0.9900000095367431640625f)
                {
                    uint _1772 = _1149 + 1u;
                    float _1888;
                    float _1890;
                    float _1892;
                    if (_1772 == _586)
                    {
                        _1888 = _874;
                        _1890 = _875;
                        _1892 = _876;
                    }
                    else
                    {
                        float _1901 = _1152 * 2.0f;
                        uint _1902 = (_1772 + _587) * 6u;
                        uint _1903 = _1902 + 230u;
                        float _1909 = _43_m0[_899].x / _1901;
                        float _1910 = _43_m0[_899].y / _1901;
                        float _1911 = _43_m0[_899].z / _1901;
                        float _1920 = (_1909 * _43_m0[_878].x) * (_191 - _43_m0[_1903].x);
                        float _1922 = (_1910 * _43_m0[_878].y) * (_192 - _43_m0[_1903].y);
                        float _1924 = ((_193 - _43_m0[_1903].z) * _43_m0[_878].z) * _1911;
                        float _1925 = _1909 * _1178;
                        float _1926 = _1910 * _1180;
                        float _1927 = _1911 * _1182;
                        float frontier_phi_68_69_ladder;
                        float frontier_phi_68_69_ladder_1;
                        float frontier_phi_68_69_ladder_2;
                        if (((_1920 < ((-0.0f) - _1925)) || (_1922 < ((-0.0f) - _1926))) || (_1924 < ((-0.0f) - _1927)))
                        {
                            frontier_phi_68_69_ladder = _876;
                            frontier_phi_68_69_ladder_1 = _875;
                            frontier_phi_68_69_ladder_2 = _874;
                        }
                        else
                        {
                            float frontier_phi_68_69_ladder_75_ladder;
                            float frontier_phi_68_69_ladder_75_ladder_1;
                            float frontier_phi_68_69_ladder_75_ladder_2;
                            if (((_1920 > ((_43_m0[_892].x + _43_m0[_885].x) + _1925)) || (_1922 > ((_43_m0[_892].y + _43_m0[_885].y) + _1926))) || (_1924 > ((_43_m0[_892].z + _43_m0[_885].z) + _1927)))
                            {
                                frontier_phi_68_69_ladder_75_ladder = _876;
                                frontier_phi_68_69_ladder_75_ladder_1 = _875;
                                frontier_phi_68_69_ladder_75_ladder_2 = _874;
                            }
                            else
                            {
                                float _2169 = min(max(_1920, _43_m0[_885].x), _43_m0[_892].x) + _43_m0[_1902 + 232u].x;
                                float _2170 = min(max(_1922, _43_m0[_885].y), _43_m0[_892].y) + _43_m0[_906].y;
                                float _2171 = min(max(_1924, _43_m0[_885].z), _43_m0[_892].z) + _43_m0[_906].z;
                                float4 _2182 = _13[NonUniformResourceIndex(_577)].SampleLevel(_57, float3(_2169, _2170, _2171 + (_43_m0[_878].z * _219)), 0.0f);
                                float4 _2187 = _13[NonUniformResourceIndex(_577)].SampleLevel(_57, float3(_2169, _2170, _2171 + (_43_m0[_878].z * _220)), 0.0f);
                                float4 _2192 = _13[NonUniformResourceIndex(_577)].SampleLevel(_57, float3(_2169, _2170, _2171 + (_43_m0[_878].z * _223)), 0.0f);
                                frontier_phi_68_69_ladder_75_ladder = (((_2187.z * _214) + (_2182.z * _213)) + (_2192.z * _215)) * _43_m0[159u].z;
                                frontier_phi_68_69_ladder_75_ladder_1 = (((_2187.y * _214) + (_2182.y * _213)) + (_2192.y * _215)) * _43_m0[159u].z;
                                frontier_phi_68_69_ladder_75_ladder_2 = (((_2187.x * _214) + (_2182.x * _213)) + (_2192.x * _215)) * _43_m0[159u].z;
                            }
                            frontier_phi_68_69_ladder = frontier_phi_68_69_ladder_75_ladder;
                            frontier_phi_68_69_ladder_1 = frontier_phi_68_69_ladder_75_ladder_1;
                            frontier_phi_68_69_ladder_2 = frontier_phi_68_69_ladder_75_ladder_2;
                        }
                        _1888 = frontier_phi_68_69_ladder_2;
                        _1890 = frontier_phi_68_69_ladder_1;
                        _1892 = frontier_phi_68_69_ladder;
                    }
                    frontier_phi_32_40_ladder_58_ladder = ((_1093 - _1888) * _1560) + _1888;
                    frontier_phi_32_40_ladder_58_ladder_1 = ((_1096 - _1890) * _1560) + _1890;
                    frontier_phi_32_40_ladder_58_ladder_2 = ((_1099 - _1892) * _1560) + _1892;
                }
                else
                {
                    frontier_phi_32_40_ladder_58_ladder = _1093;
                    frontier_phi_32_40_ladder_58_ladder_1 = _1096;
                    frontier_phi_32_40_ladder_58_ladder_2 = _1099;
                }
                frontier_phi_32_40_ladder = frontier_phi_32_40_ladder_58_ladder;
                frontier_phi_32_40_ladder_1 = frontier_phi_32_40_ladder_58_ladder_1;
                frontier_phi_32_40_ladder_2 = frontier_phi_32_40_ladder_58_ladder_2;
            }
            _1092 = frontier_phi_32_40_ladder;
            _1095 = frontier_phi_32_40_ladder_1;
            _1098 = frontier_phi_32_40_ladder_2;
        }
        float _1127 = _107 * 0.001198343117721378803253173828125f;
        float _1129 = _109 * 0.001198343117721378803253173828125f;
        float _1130 = _112 * 0.001198343117721378803253173828125f;
        float _1136 = clamp(clamp(dot(float3(_134, _135, _136), float3(_48_m0[0u].xyz)), 0.0f, 1.0f), 0.0f, 1.0f) * min(min(1.0f, _177), ((clamp(exp2(log2(_568) * _43_m0[162u].z), 0.0f, 1.0f) + (-1.0f)) * _43_m0[98u].y) + 1.0f);
        float _1138 = (_1136 * _1127) * _48_m0[1u].x;
        float _1140 = (_1136 * _1129) * _48_m0[1u].y;
        float _1142 = (_1136 * _1130) * _48_m0[1u].z;
        float _1229;
        float _1231;
        float _1233;
        if (_43_m0[66u].x > 0.0f)
        {
            float _1300;
            float _1303;
            float _1306;
            float _1309;
            float _1311;
            float _1313;
            if (asuint(_48_m0[59u]).w == 0u)
            {
                _1300 = 0.0f;
                _1303 = 0.0f;
                _1306 = 0.0f;
                _1309 = 0.0f;
                _1311 = 0.0f;
                _1313 = 0.0f;
            }
            else
            {
                float _1343 = (_43_m0[8u].x + _152) - _48_m0[59u].x;
                float _1344 = (_43_m0[8u].y + _153) - _48_m0[59u].y;
                float _1345 = (_43_m0[8u].z + _154) - _48_m0[59u].z;
                float _1366 = min(max(_1343 / _48_m0[88u].y, -1.0f), 1.0f);
                float _1367 = min(max(_1344 / _48_m0[88u].y, -1.0f), 1.0f);
                float _1368 = min(max(_1345 / _48_m0[88u].y, -1.0f), 1.0f);
                float _1482;
                float _1484;
                float _1486;
                if (_48_m0[88u].x < 0.001000000047497451305389404296875f)
                {
                    _1482 = _1366;
                    _1484 = _1367;
                    _1486 = _1368;
                }
                else
                {
                    float _1524 = log2(_48_m0[88u].x + 1.0f);
                    _1482 = (log2((abs(_1366) * _48_m0[88u].x) + 1.0f) / _1524) * float(int(uint(_1366 > 0.0f) - uint(_1366 < 0.0f)));
                    _1484 = (log2((abs(_1367) * _48_m0[88u].x) + 1.0f) / _1524) * float(int(uint(_1367 > 0.0f) - uint(_1367 < 0.0f)));
                    _1486 = (log2((abs(_1368) * _48_m0[88u].x) + 1.0f) / _1524) * float(int(uint(_1368 > 0.0f) - uint(_1368 < 0.0f)));
                }
                uint _1503 = uint(int(min(max((_48_m0[58u].x * 0.5f) * (_1482 + 1.0f), 0.0f), _48_m0[58u].x + (-1.0f))));
                uint _1504 = uint(int(min(max((_48_m0[58u].y * 0.5f) * (_1484 + 1.0f), 0.0f), _48_m0[58u].y + (-1.0f))));
                uint _1505 = uint(int(min(max((_48_m0[58u].z * 0.5f) * (_1486 + 1.0f), 0.0f), _48_m0[58u].z + (-1.0f))));
                uint4 _1507 = _25.Load(int4(uint3(_1503, _1504, _1505), 0u));
                uint _1509 = _1507.x;
                float frontier_phi_45_54_ladder;
                float frontier_phi_45_54_ladder_1;
                float frontier_phi_45_54_ladder_2;
                float frontier_phi_45_54_ladder_3;
                float frontier_phi_45_54_ladder_4;
                float frontier_phi_45_54_ladder_5;
                if (_1509 == 0u)
                {
                    frontier_phi_45_54_ladder = 0.0f;
                    frontier_phi_45_54_ladder_1 = 0.0f;
                    frontier_phi_45_54_ladder_2 = 0.0f;
                    frontier_phi_45_54_ladder_3 = 0.0f;
                    frontier_phi_45_54_ladder_4 = 0.0f;
                    frontier_phi_45_54_ladder_5 = 0.0f;
                }
                else
                {
                    uint4 _1737 = asuint(_48_m0[89u]);
                    uint _1748 = (((_1504 << (_1737.x & 31u)) + _1503) + (_1505 << (_1737.y & 31u))) << (_1737.z & 31u);
                    uint4 _1763 = _26.Load(_1748);
                    uint _1764 = _1763.x;
                    uint _1765 = _1748 + (_1509 & 127u);
                    uint _1766 = _1765 + ((_1509 >> 7u) & 127u);
                    uint _1767 = _1766 + ((_1509 >> 20u) & 63u);
                    uint _1768 = _1767 + ((_1509 >> 14u) & 63u);
                    uint _1769 = _1768 + (_1509 >> 26u);
                    uint _1770 = _1748 + 1u;
                    float _1877;
                    float _1879;
                    float _1881;
                    uint _1883;
                    uint _1885;
                    if (_1770 > _1765)
                    {
                        _1877 = 0.0f;
                        _1879 = 0.0f;
                        _1881 = 0.0f;
                        _1883 = _1770;
                        _1885 = _1764;
                    }
                    else
                    {
                        float _1878;
                        float _1880;
                        float _1882;
                        float _1958 = 0.0f;
                        float _1959 = 0.0f;
                        float _1960 = 0.0f;
                        uint _1961 = _1770;
                        uint _1962 = _1764;
                        uint _1884;
                        uint _1965;
                        uint _1980;
                        uint _1981;
                        float _2005;
                        float _2007;
                        float _2010;
                        float _2012;
                        uint _2014;
                        bool _2015;
                        float _2017;
                        bool _2023;
                        for (;;)
                        {
                            _1884 = _1961 + 1u;
                            _1965 = _26.Load(_1961).x;
                            uint _1967 = _1962 * 4u;
                            uint4 _1979 = uint4(_27.Load(_1967).x, _27.Load(_1967 + 1u).x, _27.Load(_1967 + 2u).x, _27.Load(_1967 + 3u).x);
                            _1980 = _1979.x;
                            _1981 = _1979.y;
                            uint _1982 = _1979.z;
                            uint _1983 = _1979.w;
                            _2005 = spvUnpackHalf2x16(_1981 >> 16u).x;
                            _2007 = spvUnpackHalf2x16(_1982).x;
                            _2010 = spvUnpackHalf2x16(_1982 >> 16u).x;
                            _2012 = spvUnpackHalf2x16(_1983).x;
                            _2014 = (_1983 >> 16u) & 7u;
                            _2015 = int(uint4(_1986, _1987, _1988, _28.Load((_1962 * 4u) + 3u).x).w) < int(0u);
                            _2017 = spvUnpackHalf2x16(uint3(_1996, _1997, _29.Load((_1962 * 4u) + 2u).x).z).x;
                            _2023 = (_1983 < 3221225472u) && ((_1983 & 4194304u) != 0u);
                            float frontier_phi_80_pred;
                            float frontier_phi_80_pred_1;
                            float frontier_phi_80_pred_2;
                            if (_2023)
                            {
                                float _2146 = spvUnpackHalf2x16(_1980).x - _1343;
                                float _2147 = spvUnpackHalf2x16(_1980 >> 16u).x - _1344;
                                float _2148 = spvUnpackHalf2x16(_1981).x - _1345;
                                float _2154 = sqrt(((_2147 * _2147) + (_2148 * _2148)) + (_2146 * _2146));
                                float _2155 = _2154 * _2005;
                                float frontier_phi_80_pred_79_ladder;
                                float frontier_phi_80_pred_79_ladder_1;
                                float frontier_phi_80_pred_79_ladder_2;
                                if ((_2005 <= 2.0f) && (_2155 < 1.0f))
                                {
                                    float _2334 = rsqrt(dot(float3(_2146, _2147, _2148), float3(_2146, _2147, _2148)));
                                    float _2338 = _2154 * _2154;
                                    float _2340 = (_2005 * _2005) * _2338;
                                    float _2343 = clamp(1.0f - (_2340 * _2340), 0.0f, 1.0f);
                                    float _2591;
                                    if (_2014 == 0u)
                                    {
                                        _2591 = (_2343 * _2343) * (1.0f / (max(_2338, 9.9999997473787516355514526367188e-05f) + ((_2017 * _2017) * 0.5f)));
                                    }
                                    else
                                    {
                                        _2591 = max((1.0f / dot(float3(1.0f, _2155, _2155 * _2155), float3(_48_m0[_2014 + 60u].xyz))) * (1.0f - _2155), 0.0f);
                                    }
                                    float _2600 = clamp(clamp(dot(float3(_134, _135, _136), float3(_2334 * _2146, _2334 * _2147, _2334 * _2148)), 0.0f, 1.0f), 0.0f, 1.0f);
                                    float _2604 = max(float(_2015) * 16.0f, 1.0f) * _2591;
                                    frontier_phi_80_pred_79_ladder = ((_2604 * _2012) * (_2600 * _1130)) + _1960;
                                    frontier_phi_80_pred_79_ladder_1 = ((_2604 * _2010) * (_2600 * _1129)) + _1959;
                                    frontier_phi_80_pred_79_ladder_2 = ((_2604 * _2007) * (_2600 * _1127)) + _1958;
                                }
                                else
                                {
                                    frontier_phi_80_pred_79_ladder = _1960;
                                    frontier_phi_80_pred_79_ladder_1 = _1959;
                                    frontier_phi_80_pred_79_ladder_2 = _1958;
                                }
                                frontier_phi_80_pred = frontier_phi_80_pred_79_ladder;
                                frontier_phi_80_pred_1 = frontier_phi_80_pred_79_ladder_1;
                                frontier_phi_80_pred_2 = frontier_phi_80_pred_79_ladder_2;
                            }
                            else
                            {
                                frontier_phi_80_pred = _1960;
                                frontier_phi_80_pred_1 = _1959;
                                frontier_phi_80_pred_2 = _1958;
                            }
                            _1882 = frontier_phi_80_pred;
                            _1880 = frontier_phi_80_pred_1;
                            _1878 = frontier_phi_80_pred_2;
                            if (_1884 > _1765)
                            {
                                break;
                            }
                            else
                            {
                                _1958 = _1878;
                                _1959 = _1880;
                                _1960 = _1882;
                                _1961 = _1884;
                                _1962 = _1965;
                                continue;
                            }
                        }
                        _1877 = _1878;
                        _1879 = _1880;
                        _1881 = _1882;
                        _1883 = _1884;
                        _1885 = _1965;
                    }
                    float _1947;
                    float _1949;
                    float _1951;
                    uint _1953;
                    uint _1955;
                    if (_1883 > _1766)
                    {
                        _1947 = _1877;
                        _1949 = _1879;
                        _1951 = _1881;
                        _1953 = _1883;
                        _1955 = _1885;
                    }
                    else
                    {
                        float _1948;
                        float _1950;
                        float _1952;
                        float _2043 = _1877;
                        float _2044 = _1879;
                        float _2045 = _1881;
                        uint _2046 = _1883;
                        uint _2047 = _1885;
                        uint _1954;
                        uint _2050;
                        uint _2065;
                        uint _2066;
                        float _2098;
                        float _2100;
                        float _2103;
                        float _2105;
                        uint _2107;
                        bool _2110;
                        float _2112;
                        float _2115;
                        float _2117;
                        float _2120;
                        float _2122;
                        float _2125;
                        float _2127;
                        float _2129;
                        uint _2130;
                        uint _2133;
                        uint _2134;
                        bool _2138;
                        for (;;)
                        {
                            _1954 = _2046 + 1u;
                            _2050 = _26.Load(_2046).x;
                            uint _2052 = _2047 * 4u;
                            uint4 _2064 = uint4(_27.Load(_2052).x, _27.Load(_2052 + 1u).x, _27.Load(_2052 + 2u).x, _27.Load(_2052 + 3u).x);
                            _2065 = _2064.x;
                            _2066 = _2064.y;
                            uint _2067 = _2064.z;
                            uint _2068 = _2064.w;
                            uint _2070 = _2047 * 4u;
                            uint4 _2082 = uint4(_28.Load(_2070).x, _28.Load(_2070 + 1u).x, _28.Load(_2070 + 2u).x, _28.Load(_2070 + 3u).x);
                            uint _2083 = _2082.x;
                            uint _2084 = _2082.y;
                            uint _2085 = _2082.z;
                            uint _2086 = _2082.w;
                            _2098 = spvUnpackHalf2x16(_2066 >> 16u).x;
                            _2100 = spvUnpackHalf2x16(_2067).x;
                            _2103 = spvUnpackHalf2x16(_2067 >> 16u).x;
                            _2105 = spvUnpackHalf2x16(_2068).x;
                            _2107 = (_2068 >> 16u) & 7u;
                            _2110 = (_2068 & 524288u) != 0u;
                            _2112 = spvUnpackHalf2x16(_2083).x;
                            _2115 = spvUnpackHalf2x16(_2083 >> 16u).x;
                            _2117 = spvUnpackHalf2x16(_2084).x;
                            _2120 = spvUnpackHalf2x16(_2084 >> 16u).x;
                            _2122 = spvUnpackHalf2x16(_2085).x;
                            _2125 = spvUnpackHalf2x16(_2085 >> 16u).x;
                            _2127 = spvUnpackHalf2x16(_2086).x;
                            _2129 = spvUnpackHalf2x16(uint3(_2089, _2090, _29.Load((_2047 * 4u) + 2u).x).z).x;
                            _2130 = _2086 & 8323072u;
                            _2133 = (_2086 >> 23u) & 31u;
                            _2134 = _2086 >> 28u;
                            _2138 = (_2068 < 3221225472u) && ((_2068 & 4194304u) != 0u);
                            float frontier_phi_86_pred;
                            float frontier_phi_86_pred_1;
                            float frontier_phi_86_pred_2;
                            if (_2138)
                            {
                                float _2314 = spvUnpackHalf2x16(_2065).x - _1343;
                                float _2315 = spvUnpackHalf2x16(_2065 >> 16u).x - _1344;
                                float _2316 = spvUnpackHalf2x16(_2066).x - _1345;
                                float _2322 = sqrt(((_2315 * _2315) + (_2316 * _2316)) + (_2314 * _2314));
                                float _2323 = _2322 * _2098;
                                float frontier_phi_86_pred_85_ladder;
                                float frontier_phi_86_pred_85_ladder_1;
                                float frontier_phi_86_pred_85_ladder_2;
                                if ((_2098 <= 2.0f) && (_2323 < 1.0f))
                                {
                                    float _2426 = rsqrt(dot(float3(_2314, _2315, _2316), float3(_2314, _2315, _2316)));
                                    float _2427 = _2426 * _2314;
                                    float _2428 = _2426 * _2315;
                                    float _2429 = _2426 * _2316;
                                    float _2430 = _2322 * _2322;
                                    float _2432 = (_2098 * _2098) * _2430;
                                    float _2435 = clamp(1.0f - (_2432 * _2432), 0.0f, 1.0f);
                                    float _2730;
                                    if (_2107 == 0u)
                                    {
                                        _2730 = (_2435 * _2435) * (1.0f / (max(_2430, 9.9999997473787516355514526367188e-05f) + ((_2129 * _2129) * 0.5f)));
                                    }
                                    else
                                    {
                                        _2730 = max((1.0f / dot(float3(1.0f, _2323, _2323 * _2323), float3(_48_m0[_2107 + 60u].xyz))) * (1.0f - _2323), 0.0f);
                                    }
                                    float _2731 = (-0.0f) - _2112;
                                    float _2755 = clamp((clamp(dot(float3(((_2120 * _2115) - (_2117 * _2731)) * 2.0f, ((_2117 * _2115) - (_2120 * _2112)) * 2.0f, (((_2112 * _2731) - (_2115 * _2115)) * 2.0f) + 1.0f), float3((-0.0f) - _2427, (-0.0f) - _2428, (-0.0f) - _2429)), 0.0f, 1.0f) - _2125) / (_2122 - _2125), 0.0f, 1.0f);
                                    float _2849;
                                    float _2851;
                                    float _2853;
                                    if ((_2133 | _2130) == 0u)
                                    {
                                        _2849 = _2100;
                                        _2851 = _2103;
                                        _2853 = _2105;
                                    }
                                    else
                                    {
                                        float _2882 = (-0.0f) - _2115;
                                        float _2883 = (-0.0f) - _2117;
                                        float _2896 = ((_2316 * _2882) - (_2315 * _2883)) + (_2314 * _2120);
                                        float _2897 = ((_2314 * _2883) - (_2316 * _2731)) + (_2315 * _2120);
                                        float _2898 = ((_2315 * _2731) - (_2314 * _2882)) + (_2316 * _2120);
                                        float _2905 = (1.0f / ((((_2897 * _2731) - (_2896 * _2882)) * 2.0f) + _2316)) * _2127;
                                        float frontier_phi_118_119_ladder;
                                        float frontier_phi_118_119_ladder_1;
                                        float frontier_phi_118_119_ladder_2;
                                        if (_2133 == 0u)
                                        {
                                            frontier_phi_118_119_ladder = _2105;
                                            frontier_phi_118_119_ladder_1 = _2103;
                                            frontier_phi_118_119_ladder_2 = _2100;
                                        }
                                        else
                                        {
                                            uint _2924 = _2134 + 72u;
                                            float4 _2942 = _32.SampleLevel(_58, float3((_48_m0[_2924].x * ((_2905 * ((((_2898 * _2882) - (_2897 * _2883)) * 2.0f) + _2314)) + 0.5f)) + _48_m0[_2924].z, (_48_m0[_2924].y * (0.5f - (_2905 * ((((_2896 * _2883) - (_2898 * _2731)) * 2.0f) + _2315)))) + _48_m0[_2924].w, float(_2133 + 4294967295u)), 0.0f);
                                            frontier_phi_118_119_ladder = _2942.z * _2105;
                                            frontier_phi_118_119_ladder_1 = _2942.y * _2103;
                                            frontier_phi_118_119_ladder_2 = _2942.x * _2100;
                                        }
                                        _2849 = frontier_phi_118_119_ladder_2;
                                        _2851 = frontier_phi_118_119_ladder_1;
                                        _2853 = frontier_phi_118_119_ladder;
                                    }
                                    float _2862 = (_2098 < 0.02857142873108386993408203125f) ? _48_m0[104u].w : 0.0f;
                                    float _2867 = 1.0f - _2862;
                                    float _2871 = clamp(((clamp(dot(float3(_134, _135, _136), float3(_2427, _2428, _2429)), 0.0f, 1.0f) * _2867) + _2862) * _2867, 0.0f, 1.0f);
                                    float _2872 = max(float(_2110) * 16.0f, 1.0f) * (((_2755 * _2755) * _2730) * (3.0f - (_2755 * 2.0f)));
                                    frontier_phi_86_pred_85_ladder = (((_2872 * _1130) * _2853) * _2871) + _2045;
                                    frontier_phi_86_pred_85_ladder_1 = (((_2872 * _1129) * _2851) * _2871) + _2044;
                                    frontier_phi_86_pred_85_ladder_2 = (((_2872 * _1127) * _2849) * _2871) + _2043;
                                }
                                else
                                {
                                    frontier_phi_86_pred_85_ladder = _2045;
                                    frontier_phi_86_pred_85_ladder_1 = _2044;
                                    frontier_phi_86_pred_85_ladder_2 = _2043;
                                }
                                frontier_phi_86_pred = frontier_phi_86_pred_85_ladder;
                                frontier_phi_86_pred_1 = frontier_phi_86_pred_85_ladder_1;
                                frontier_phi_86_pred_2 = frontier_phi_86_pred_85_ladder_2;
                            }
                            else
                            {
                                frontier_phi_86_pred = _2045;
                                frontier_phi_86_pred_1 = _2044;
                                frontier_phi_86_pred_2 = _2043;
                            }
                            _1952 = frontier_phi_86_pred;
                            _1950 = frontier_phi_86_pred_1;
                            _1948 = frontier_phi_86_pred_2;
                            if (_1954 > _1766)
                            {
                                break;
                            }
                            else
                            {
                                _2043 = _1948;
                                _2044 = _1950;
                                _2045 = _1952;
                                _2046 = _1954;
                                _2047 = _2050;
                                continue;
                            }
                        }
                        _1947 = _1948;
                        _1949 = _1950;
                        _1951 = _1952;
                        _1953 = _1954;
                        _1955 = _2050;
                    }
                    float _1301;
                    float _1304;
                    float _1307;
                    uint _2038;
                    uint _2040;
                    if (_1953 > _1767)
                    {
                        _1301 = _1947;
                        _1304 = _1949;
                        _1307 = _1951;
                        _2038 = _1953;
                        _2040 = _1955;
                    }
                    else
                    {
                        float _2035;
                        float _2036;
                        float _2037;
                        float _2223 = _1947;
                        float _2224 = _1949;
                        float _2225 = _1951;
                        uint _2226 = _1953;
                        uint _2227 = _1955;
                        uint _2039;
                        uint _2230;
                        uint _2245;
                        uint _2246;
                        float _2274;
                        float _2276;
                        float _2279;
                        float _2281;
                        uint _2283;
                        bool _2285;
                        float _2287;
                        float _2290;
                        float _2292;
                        float _2295;
                        float _2297;
                        float _2300;
                        float _2302;
                        bool _2306;
                        for (;;)
                        {
                            _2039 = _2226 + 1u;
                            _2230 = _26.Load(_2226).x;
                            uint _2232 = _2227 * 4u;
                            uint4 _2244 = uint4(_27.Load(_2232).x, _27.Load(_2232 + 1u).x, _27.Load(_2232 + 2u).x, _27.Load(_2232 + 3u).x);
                            _2245 = _2244.x;
                            _2246 = _2244.y;
                            uint _2247 = _2244.z;
                            uint _2248 = _2244.w;
                            uint _2250 = _2227 * 4u;
                            uint3 _2259 = uint3(_28.Load(_2250).x, _28.Load(_2250 + 1u).x, _28.Load(_2250 + 2u).x);
                            uint _2260 = _2259.x;
                            uint _2261 = _2259.y;
                            uint _2262 = _2259.z;
                            _2274 = spvUnpackHalf2x16(_2246 >> 16u).x;
                            _2276 = spvUnpackHalf2x16(_2247).x;
                            _2279 = spvUnpackHalf2x16(_2247 >> 16u).x;
                            _2281 = spvUnpackHalf2x16(_2248).x;
                            _2283 = (_2248 >> 16u) & 7u;
                            _2285 = (_2248 & 524288u) != 0u;
                            _2287 = spvUnpackHalf2x16(_2260).x;
                            _2290 = spvUnpackHalf2x16(_2260 >> 16u).x;
                            _2292 = spvUnpackHalf2x16(_2261).x;
                            _2295 = spvUnpackHalf2x16(_2261 >> 16u).x;
                            _2297 = spvUnpackHalf2x16(_2262).x;
                            _2300 = spvUnpackHalf2x16(_2262 >> 16u).x;
                            _2302 = spvUnpackHalf2x16(uint3(_2265, _2266, _29.Load((_2227 * 4u) + 2u).x).z).x;
                            _2306 = (_2248 < 3221225472u) && ((_2248 & 4194304u) != 0u);
                            float frontier_phi_92_pred;
                            float frontier_phi_92_pred_1;
                            float frontier_phi_92_pred_2;
                            if (_2306)
                            {
                                float _2406 = spvUnpackHalf2x16(_2245).x - _1343;
                                float _2407 = spvUnpackHalf2x16(_2245 >> 16u).x - _1344;
                                float _2408 = spvUnpackHalf2x16(_2246).x - _1345;
                                float _2414 = sqrt(((_2407 * _2407) + (_2408 * _2408)) + (_2406 * _2406));
                                float _2415 = _2414 * _2274;
                                float frontier_phi_92_pred_91_ladder;
                                float frontier_phi_92_pred_91_ladder_1;
                                float frontier_phi_92_pred_91_ladder_2;
                                if ((_2274 <= 2.0f) && (_2415 < 1.0f))
                                {
                                    float _2559 = rsqrt(dot(float3(_2406, _2407, _2408), float3(_2406, _2407, _2408)));
                                    float _2560 = _2559 * _2406;
                                    float _2561 = _2559 * _2407;
                                    float _2562 = _2559 * _2408;
                                    float _2563 = _2414 * _2414;
                                    float _2565 = (_2274 * _2274) * _2563;
                                    float _2568 = clamp(1.0f - (_2565 * _2565), 0.0f, 1.0f);
                                    float _2789;
                                    if (_2283 == 0u)
                                    {
                                        _2789 = (_2568 * _2568) * (1.0f / (max(_2563, 9.9999997473787516355514526367188e-05f) + ((_2302 * _2302) * 0.5f)));
                                    }
                                    else
                                    {
                                        _2789 = max((1.0f / dot(float3(1.0f, _2415, _2415 * _2415), float3(_48_m0[_2283 + 60u].xyz))) * (1.0f - _2415), 0.0f);
                                    }
                                    float _2790 = (-0.0f) - _2287;
                                    float _2814 = clamp((clamp(dot(float3(((_2295 * _2290) - (_2292 * _2790)) * 2.0f, ((_2292 * _2290) - (_2295 * _2287)) * 2.0f, (((_2287 * _2790) - (_2290 * _2290)) * 2.0f) + 1.0f), float3((-0.0f) - _2560, (-0.0f) - _2561, (-0.0f) - _2562)), 0.0f, 1.0f) - _2300) / (_2297 - _2300), 0.0f, 1.0f);
                                    float _2829 = (_2274 < 0.02857142873108386993408203125f) ? _48_m0[104u].w : 0.0f;
                                    float _2834 = 1.0f - _2829;
                                    float _2838 = clamp(((_2834 * clamp(dot(float3(_134, _135, _136), float3(_2560, _2561, _2562)), 0.0f, 1.0f)) + _2829) * _2834, 0.0f, 1.0f);
                                    float _2839 = (((_2814 * _2814) * _2789) * (3.0f - (_2814 * 2.0f))) * max(float(_2285) * 16.0f, 1.0f);
                                    frontier_phi_92_pred_91_ladder = (((_2839 * _1129) * _2279) * _2838) + _2224;
                                    frontier_phi_92_pred_91_ladder_1 = (((_2839 * _1127) * _2276) * _2838) + _2223;
                                    frontier_phi_92_pred_91_ladder_2 = (((_2839 * _1130) * _2281) * _2838) + _2225;
                                }
                                else
                                {
                                    frontier_phi_92_pred_91_ladder = _2224;
                                    frontier_phi_92_pred_91_ladder_1 = _2223;
                                    frontier_phi_92_pred_91_ladder_2 = _2225;
                                }
                                frontier_phi_92_pred = frontier_phi_92_pred_91_ladder;
                                frontier_phi_92_pred_1 = frontier_phi_92_pred_91_ladder_1;
                                frontier_phi_92_pred_2 = frontier_phi_92_pred_91_ladder_2;
                            }
                            else
                            {
                                frontier_phi_92_pred = _2224;
                                frontier_phi_92_pred_1 = _2223;
                                frontier_phi_92_pred_2 = _2225;
                            }
                            _2036 = frontier_phi_92_pred;
                            _2035 = frontier_phi_92_pred_1;
                            _2037 = frontier_phi_92_pred_2;
                            if (_2039 > _1767)
                            {
                                break;
                            }
                            else
                            {
                                _2223 = _2035;
                                _2224 = _2036;
                                _2225 = _2037;
                                _2226 = _2039;
                                _2227 = _2230;
                                continue;
                            }
                        }
                        _1301 = _2035;
                        _1304 = _2036;
                        _1307 = _2037;
                        _2038 = _2039;
                        _2040 = _2230;
                    }
                    float _1310;
                    float _1312;
                    float _1314;
                    uint _2218;
                    uint _2220;
                    if (_2038 > _1768)
                    {
                        _1310 = 0.0f;
                        _1312 = 0.0f;
                        _1314 = 0.0f;
                        _2218 = _2038;
                        _2220 = _2040;
                    }
                    else
                    {
                        float _2215;
                        float _2216;
                        float _2217;
                        float _2345 = 0.0f;
                        float _2346 = 0.0f;
                        float _2347 = 0.0f;
                        uint _2348 = _2038;
                        uint _2349 = _2040;
                        uint _2219;
                        uint _2352;
                        float _2382;
                        float _2385;
                        float _2387;
                        float _2390;
                        float _2394;
                        float _2397;
                        bool _2398;
                        for (;;)
                        {
                            _2219 = _2348 + 1u;
                            _2352 = _26.Load(_2348).x;
                            uint _2354 = _2349 * 4u;
                            uint4 _2366 = uint4(_27.Load(_2354).x, _27.Load(_2354 + 1u).x, _27.Load(_2354 + 2u).x, _27.Load(_2354 + 3u).x);
                            uint _2367 = _2366.x;
                            uint _2368 = _2366.y;
                            uint _2369 = _2366.z;
                            uint _2370 = _2366.w;
                            _2382 = spvUnpackHalf2x16(_2369).x;
                            _2385 = spvUnpackHalf2x16(_2369 >> 16u).x;
                            _2387 = spvUnpackHalf2x16(_2370).x;
                            _2390 = spvUnpackHalf2x16(_2370 >> 16u).x;
                            float _2391 = spvUnpackHalf2x16(_2367).x - _1343;
                            float _2392 = spvUnpackHalf2x16(_2367 >> 16u).x - _1344;
                            float _2393 = spvUnpackHalf2x16(_2368).x - _1345;
                            _2394 = dot(float3(_2391, _2392, _2393), float3(_2391, _2392, _2393));
                            _2397 = _2394 * spvUnpackHalf2x16(_2368 >> 16u).x;
                            _2398 = _2397 < 1.0f;
                            float frontier_phi_99_pred;
                            float frontier_phi_99_pred_1;
                            float frontier_phi_99_pred_2;
                            if (_2398)
                            {
                                float _2785;
                                if (_2390 != 0.0f)
                                {
                                    _2785 = (1.0f / ((_2397 * _2390) + 1.0f)) * (1.0f - _2397);
                                }
                                else
                                {
                                    float _2706 = clamp(1.0f - (_2397 * _2397), 0.0f, 1.0f);
                                    _2785 = (_2706 * _2706) * (1.0f / max(_2394, 9.9999997473787516355514526367188e-05f));
                                }
                                frontier_phi_99_pred = (_2785 * _2387) + _2347;
                                frontier_phi_99_pred_1 = (_2785 * _2382) + _2345;
                                frontier_phi_99_pred_2 = (_2785 * _2385) + _2346;
                            }
                            else
                            {
                                frontier_phi_99_pred = _2347;
                                frontier_phi_99_pred_1 = _2345;
                                frontier_phi_99_pred_2 = _2346;
                            }
                            _2217 = frontier_phi_99_pred;
                            _2215 = frontier_phi_99_pred_1;
                            _2216 = frontier_phi_99_pred_2;
                            if (_2219 > _1768)
                            {
                                break;
                            }
                            else
                            {
                                _2345 = _2215;
                                _2346 = _2216;
                                _2347 = _2217;
                                _2348 = _2219;
                                _2349 = _2352;
                                continue;
                            }
                        }
                        _1310 = _2215;
                        _1312 = _2216;
                        _1314 = _2217;
                        _2218 = _2219;
                        _2220 = _2352;
                    }
                    float frontier_phi_45_54_ladder_82_ladder;
                    float frontier_phi_45_54_ladder_82_ladder_1;
                    float frontier_phi_45_54_ladder_82_ladder_2;
                    float frontier_phi_45_54_ladder_82_ladder_3;
                    float frontier_phi_45_54_ladder_82_ladder_4;
                    float frontier_phi_45_54_ladder_82_ladder_5;
                    if (_2218 > _1769)
                    {
                        frontier_phi_45_54_ladder_82_ladder = _1314;
                        frontier_phi_45_54_ladder_82_ladder_1 = _1312;
                        frontier_phi_45_54_ladder_82_ladder_2 = _1310;
                        frontier_phi_45_54_ladder_82_ladder_3 = _1307;
                        frontier_phi_45_54_ladder_82_ladder_4 = _1304;
                        frontier_phi_45_54_ladder_82_ladder_5 = _1301;
                    }
                    else
                    {
                        float _1302;
                        float _1305;
                        float _1308;
                        float _2459 = _1301;
                        float _2460 = _1304;
                        float _2461 = _1307;
                        uint _2462 = _2218;
                        uint _2464 = _2220;
                        uint _2463;
                        uint _2468;
                        uint _2483;
                        uint _2484;
                        float _2516;
                        float _2518;
                        float _2521;
                        float _2523;
                        uint _2525;
                        bool _2527;
                        float _2529;
                        float _2532;
                        float _2534;
                        float _2537;
                        float _2539;
                        float _2542;
                        float _2544;
                        float _2546;
                        bool _2550;
                        for (;;)
                        {
                            _2463 = _2462 + 1u;
                            _2468 = _26.Load(_2462).x;
                            uint _2470 = _2464 * 4u;
                            uint4 _2482 = uint4(_27.Load(_2470).x, _27.Load(_2470 + 1u).x, _27.Load(_2470 + 2u).x, _27.Load(_2470 + 3u).x);
                            _2483 = _2482.x;
                            _2484 = _2482.y;
                            uint _2485 = _2482.z;
                            uint _2486 = _2482.w;
                            uint _2488 = _2464 * 4u;
                            uint4 _2500 = uint4(_28.Load(_2488).x, _28.Load(_2488 + 1u).x, _28.Load(_2488 + 2u).x, _28.Load(_2488 + 3u).x);
                            uint _2501 = _2500.x;
                            uint _2502 = _2500.y;
                            uint _2503 = _2500.z;
                            _2516 = spvUnpackHalf2x16(_2484 >> 16u).x;
                            _2518 = spvUnpackHalf2x16(_2485).x;
                            _2521 = spvUnpackHalf2x16(_2485 >> 16u).x;
                            _2523 = spvUnpackHalf2x16(_2486).x;
                            _2525 = (_2486 >> 16u) & 7u;
                            _2527 = (_2486 & 524288u) != 0u;
                            _2529 = spvUnpackHalf2x16(_2501).x;
                            _2532 = spvUnpackHalf2x16(_2501 >> 16u).x;
                            _2534 = spvUnpackHalf2x16(_2502).x;
                            _2537 = spvUnpackHalf2x16(_2502 >> 16u).x;
                            _2539 = spvUnpackHalf2x16(_2503).x;
                            _2542 = spvUnpackHalf2x16(_2503 >> 16u).x;
                            _2544 = spvUnpackHalf2x16(_2500.w).x;
                            _2546 = spvUnpackHalf2x16(uint3(_2507, _2508, _29.Load((_2464 * 4u) + 2u).x).z).x;
                            _2550 = (_2486 < 3221225472u) && ((_2486 & 4194304u) != 0u);
                            float frontier_phi_106_pred;
                            float frontier_phi_106_pred_1;
                            float frontier_phi_106_pred_2;
                            if (_2550)
                            {
                                float _2618 = (-0.0f) - _2534;
                                float _2624 = (_2534 * _2618) - (_2529 * _2529);
                                float _2625 = _2537 * _2529;
                                float _2630 = (-0.0f) - _2529;
                                float _2643 = _2544 * ((_2532 * _2529) - (_2537 * _2534));
                                float _2646 = _2544 * (_2625 - (_2532 * _2618));
                                float _2647 = spvUnpackHalf2x16(_2483).x - _2643;
                                float _2648 = spvUnpackHalf2x16(_2483 >> 16u).x - (_2544 * (_2624 + 0.5f));
                                float _2649 = spvUnpackHalf2x16(_2484).x - _2646;
                                float _2653 = _2643 * 2.0f;
                                float _2654 = _2544 * ((_2624 * 2.0f) + 1.0f);
                                float _2655 = _2646 * 2.0f;
                                float _2663 = clamp(dot(float3(_2653, _2654, _2655), float3(_1343 - _2647, _1344 - _2648, _1345 - _2649)) / dot(float3(_2653, _2654, _2655), float3(_2653, _2654, _2655)), 0.0f, 1.0f);
                                float _2668 = (_2663 * _2653) + (_2647 - _1343);
                                float _2670 = (_2663 * _2654) + (_2648 - _1344);
                                float _2672 = (_2663 * _2655) + (_2649 - _1345);
                                float _2676 = rsqrt(dot(float3(_2668, _2670, _2672), float3(_2668, _2670, _2672)));
                                float _2677 = _2668 * _2676;
                                float _2678 = _2670 * _2676;
                                float _2679 = _2672 * _2676;
                                float _2685 = sqrt(((_2668 * _2668) + (_2670 * _2670)) + (_2672 * _2672));
                                float _2686 = _2685 * _2685;
                                float _2688 = (_2516 * _2516) * _2686;
                                float _2691 = clamp(1.0f - (_2688 * _2688), 0.0f, 1.0f);
                                float _2907;
                                if (_2525 == 0u)
                                {
                                    _2907 = (_2691 * _2691) * (1.0f / (max(_2686, 9.9999997473787516355514526367188e-05f) + ((_2546 * _2546) * 0.5f)));
                                }
                                else
                                {
                                    float _2770 = _2685 * _2516;
                                    _2907 = max((1.0f / dot(float3(1.0f, _2770, _2770 * _2770), float3(_48_m0[_2525 + 60u].xyz))) * (1.0f - _2770), 0.0f);
                                }
                                float frontier_phi_106_pred_120_ladder;
                                float frontier_phi_106_pred_120_ladder_1;
                                float frontier_phi_106_pred_120_ladder_2;
                                if (_2907 < 9.9999997473787516355514526367188e-06f)
                                {
                                    frontier_phi_106_pred_120_ladder = _2461;
                                    frontier_phi_106_pred_120_ladder_1 = _2460;
                                    frontier_phi_106_pred_120_ladder_2 = _2459;
                                }
                                else
                                {
                                    float _2957 = clamp((clamp(dot(float3(((_2537 * _2532) - (_2534 * _2630)) * 2.0f, ((_2534 * _2532) - _2625) * 2.0f, (((_2529 * _2630) - (_2532 * _2532)) * 2.0f) + 1.0f), float3((-0.0f) - _2677, (-0.0f) - _2678, (-0.0f) - _2679)), 0.0f, 1.0f) - _2542) / (_2539 - _2542), 0.0f, 1.0f);
                                    float _2961 = (_2957 * _2957) * (3.0f - (_2957 * 2.0f));
                                    float _2963 = (_2961 * _2961) * _2907;
                                    float frontier_phi_106_pred_120_ladder_122_ladder;
                                    float frontier_phi_106_pred_120_ladder_122_ladder_1;
                                    float frontier_phi_106_pred_120_ladder_122_ladder_2;
                                    if (_2963 < 9.9999997473787516355514526367188e-06f)
                                    {
                                        frontier_phi_106_pred_120_ladder_122_ladder = _2461;
                                        frontier_phi_106_pred_120_ladder_122_ladder_1 = _2460;
                                        frontier_phi_106_pred_120_ladder_122_ladder_2 = _2459;
                                    }
                                    else
                                    {
                                        float _2972 = clamp(clamp(dot(float3(_134, _135, _136), float3(_2677, _2678, _2679)), 0.0f, 1.0f), 0.0f, 1.0f);
                                        float _2973 = max(float(_2527) * 16.0f, 1.0f) * _2963;
                                        frontier_phi_106_pred_120_ladder_122_ladder = (((_2973 * _1130) * _2523) * _2972) + _2461;
                                        frontier_phi_106_pred_120_ladder_122_ladder_1 = (((_2973 * _1129) * _2521) * _2972) + _2460;
                                        frontier_phi_106_pred_120_ladder_122_ladder_2 = (((_2973 * _1127) * _2518) * _2972) + _2459;
                                    }
                                    frontier_phi_106_pred_120_ladder = frontier_phi_106_pred_120_ladder_122_ladder;
                                    frontier_phi_106_pred_120_ladder_1 = frontier_phi_106_pred_120_ladder_122_ladder_1;
                                    frontier_phi_106_pred_120_ladder_2 = frontier_phi_106_pred_120_ladder_122_ladder_2;
                                }
                                frontier_phi_106_pred = frontier_phi_106_pred_120_ladder;
                                frontier_phi_106_pred_1 = frontier_phi_106_pred_120_ladder_1;
                                frontier_phi_106_pred_2 = frontier_phi_106_pred_120_ladder_2;
                            }
                            else
                            {
                                frontier_phi_106_pred = _2461;
                                frontier_phi_106_pred_1 = _2460;
                                frontier_phi_106_pred_2 = _2459;
                            }
                            _1308 = frontier_phi_106_pred;
                            _1305 = frontier_phi_106_pred_1;
                            _1302 = frontier_phi_106_pred_2;
                            if (_2463 > _1769)
                            {
                                break;
                            }
                            else
                            {
                                _2459 = _1302;
                                _2460 = _1305;
                                _2461 = _1308;
                                _2462 = _2463;
                                _2464 = _2468;
                                continue;
                            }
                        }
                        frontier_phi_45_54_ladder_82_ladder = _1314;
                        frontier_phi_45_54_ladder_82_ladder_1 = _1312;
                        frontier_phi_45_54_ladder_82_ladder_2 = _1310;
                        frontier_phi_45_54_ladder_82_ladder_3 = _1308;
                        frontier_phi_45_54_ladder_82_ladder_4 = _1305;
                        frontier_phi_45_54_ladder_82_ladder_5 = _1302;
                    }
                    frontier_phi_45_54_ladder = frontier_phi_45_54_ladder_82_ladder;
                    frontier_phi_45_54_ladder_1 = frontier_phi_45_54_ladder_82_ladder_1;
                    frontier_phi_45_54_ladder_2 = frontier_phi_45_54_ladder_82_ladder_2;
                    frontier_phi_45_54_ladder_3 = frontier_phi_45_54_ladder_82_ladder_3;
                    frontier_phi_45_54_ladder_4 = frontier_phi_45_54_ladder_82_ladder_4;
                    frontier_phi_45_54_ladder_5 = frontier_phi_45_54_ladder_82_ladder_5;
                }
                _1300 = frontier_phi_45_54_ladder_5;
                _1303 = frontier_phi_45_54_ladder_4;
                _1306 = frontier_phi_45_54_ladder_3;
                _1309 = frontier_phi_45_54_ladder_2;
                _1311 = frontier_phi_45_54_ladder_1;
                _1313 = frontier_phi_45_54_ladder;
            }
            float _1322 = _568 * 0.00376470596529543399810791015625f;
            _1229 = (((_1322 * _107) * _1309) + _1138) + (_43_m0[48u].x * _1300);
            _1231 = (((_1322 * _109) * _1311) + _1140) + (_43_m0[48u].x * _1303);
            _1233 = (((_1322 * _112) * _1313) + _1142) + (_43_m0[48u].x * _1306);
        }
        else
        {
            _1229 = _1138;
            _1231 = _1140;
            _1233 = _1142;
        }
        float _1235 = (1.0f - _624) * _43_m0[59u].w;
        float4 _1242 = _33.SampleLevel(_60, float3(_134, _135, _136), 0.0f);
        float _1248 = _43_m0[59u].y * _568;
        float _1266 = (((_107 * 0.00376470596529543399810791015625f) * ((_1242.x * _1248) + (_1092 * _1235))) + _1229) * _43_m0[59u].x;
        float _1267 = (((_109 * 0.00376470596529543399810791015625f) * ((_1242.y * _1248) + (_1095 * _1235))) + _1231) * _43_m0[59u].x;
        float _1268 = (((_112 * 0.00376470596529543399810791015625f) * ((_1242.z * _1248) + (_1098 * _1235))) + _1233) * _43_m0[59u].x;
        float _1371;
        float _1373;
        float _1375;
        if ((asuint(_1266) & 2139095040u) == 2139095040u)
        {
            _1371 = 0.0f;
            _1373 = 0.0f;
            _1375 = 0.0f;
        }
        else
        {
            float frontier_phi_47_48_ladder;
            float frontier_phi_47_48_ladder_1;
            float frontier_phi_47_48_ladder_2;
            if ((asuint(_1267) & 2139095040u) == 2139095040u)
            {
                frontier_phi_47_48_ladder = 0.0f;
                frontier_phi_47_48_ladder_1 = 0.0f;
                frontier_phi_47_48_ladder_2 = 0.0f;
            }
            else
            {
                bool _1548 = (asuint(_1268) & 2139095040u) == 2139095040u;
                frontier_phi_47_48_ladder = _1548 ? 0.0f : _1267;
                frontier_phi_47_48_ladder_1 = _1548 ? 0.0f : _1268;
                frontier_phi_47_48_ladder_2 = _1548 ? 0.0f : _1266;
            }
            _1371 = frontier_phi_47_48_ladder_2;
            _1373 = frontier_phi_47_48_ladder;
            _1375 = frontier_phi_47_48_ladder_1;
        }
        _37[gl_GlobalInvocationID.x] = float4(_1371, _1373, _1375, _1371);
    }
}

[numthreads(64, 1, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
