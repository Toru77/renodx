static uint _3327;
static uint _3328;
static uint _3329;
static uint _3337;
static uint _3338;
static uint _3425;
static uint _3426;
static uint _3552;
static uint _3553;
static uint _3797;
static uint _3798;

cbuffer _46_48 : register(b2, space0)
{
    float4 _48_m0[700] : packoffset(c0);
};

cbuffer _51_53 : register(b0, space0)
{
    float4 _53_m0[1165] : packoffset(c0);
};

Buffer<uint4> _8 : register(t96, space0);
Buffer<uint4> _9 : register(t97, space0);
Buffer<uint4> _10 : register(t98, space0);
Texture2D<float4> _15[] : register(t0, space6);
Texture3D<float4> _18 : register(t123, space0);
TextureCube<float4> _21 : register(t118, space0);
Texture2D<float4> _24[] : register(t0, space37);
Texture2D<uint4> _28[] : register(t0, space38);
TextureCube<float4> _31[] : register(t0, space41);
Texture3D<uint4> _34 : register(t0, space12);
Buffer<uint4> _35 : register(t1, space12);
Buffer<uint4> _36 : register(t2, space12);
Buffer<uint4> _37 : register(t3, space12);
Buffer<uint4> _38 : register(t9, space12);
Texture2DArray<float4> _41 : register(t4, space12);
Texture3D<float4> _42 : register(t124, space0);
SamplerState _56 : register(s0, space0);
SamplerState _57 : register(s2, space0);
SamplerState _58 : register(s5, space0);
SamplerState _59 : register(s1, space0);
SamplerState _60 : register(s3, space0);
SamplerState _61 : register(s11, space0);

static float4 gl_FragCoord;
static float4 SV_Target;
static float4 SV_Target_3;

struct SPIRV_Cross_Input
{
    float4 gl_FragCoord : SV_Position;
};

struct SPIRV_Cross_Output
{
    float4 SV_Target : SV_Target0;
    float4 SV_Target_3 : SV_Target3;
};

static uint _70[26];
static uint _71[26];
static uint _72[26];
static uint _73[26];

uint spvPackHalf2x16(float2 value)
{
    uint2 Packed = f32tof16(value);
    return Packed.x | (Packed.y << 16);
}

float2 spvUnpackHalf2x16(uint value)
{
    return f16tof32(uint2(value & 0xffff, value >> 16));
}

void frag_main()
{
    _70[0u] = 2097152u;
    _71[0u] = 0u;
    _72[0u] = 4228890621u;
    _73[0u] = 4294963171u;
    _70[1u] = 67108864u;
    _71[1u] = 0u;
    _72[1u] = 4226793471u;
    _73[1u] = 4294967295u;
    _70[2u] = 0u;
    _71[2u] = 0u;
    _72[2u] = 4294950911u;
    _73[2u] = 4294967295u;
    _70[3u] = 0u;
    _71[3u] = 0u;
    _72[3u] = 4294950911u;
    _73[3u] = 4294967295u;
    _70[4u] = 134234112u;
    _71[4u] = 512u;
    _72[4u] = 4127178751u;
    _73[4u] = 4294965755u;
    _70[5u] = 134234112u;
    _71[5u] = 512u;
    _72[5u] = 4127178751u;
    _73[5u] = 4294963711u;
    _70[6u] = 0u;
    _71[6u] = 0u;
    _72[6u] = 4294950901u;
    _73[6u] = 4294963167u;
    _70[7u] = 512u;
    _71[7u] = 0u;
    _72[7u] = 4294950389u;
    _73[7u] = 4294963167u;
    _70[8u] = 49228u;
    _71[8u] = 256u;
    _72[8u] = 4294852529u;
    _73[8u] = 4294962943u;
    _70[9u] = 180300u;
    _71[9u] = 0u;
    _72[9u] = 4294721201u;
    _73[9u] = 4294963199u;
    _70[10u] = 33092u;
    _71[10u] = 0u;
    _72[10u] = 4294590129u;
    _73[10u] = 4294963071u;
    _70[11u] = 2072u;
    _71[11u] = 0u;
    _72[11u] = 4294965221u;
    _73[11u] = 4294963199u;
    _70[12u] = 2072u;
    _71[12u] = 1u;
    _72[12u] = 4294965221u;
    _73[12u] = 4294963198u;
    _70[13u] = 2072u;
    _71[13u] = 2u;
    _72[13u] = 4294965221u;
    _73[13u] = 4294963197u;
    _70[14u] = 2147485784u;
    _71[14u] = 0u;
    _72[14u] = 2147481509u;
    _73[14u] = 4294963199u;
    _70[15u] = 0u;
    _71[15u] = 64u;
    _72[15u] = 4294950901u;
    _73[15u] = 4294963103u;
    _70[16u] = 27279360u;
    _71[16u] = 8212u;
    _72[16u] = 4267687933u;
    _73[16u] = 4294954987u;
    _70[17u] = 0u;
    _71[17u] = 0u;
    _72[17u] = 467664896u;
    _73[17u] = 4294962712u;
    _70[18u] = 0u;
    _71[18u] = 0u;
    _72[18u] = 159383552u;
    _73[18u] = 4294962688u;
    _70[19u] = 0u;
    _71[19u] = 2097152u;
    _72[19u] = 4294967292u;
    _73[19u] = 4292849631u;
    _70[20u] = 0u;
    _71[20u] = 0u;
    _72[20u] = 4292853756u;
    _73[20u] = 4288638927u;
    _70[21u] = 0u;
    _71[21u] = 8388608u;
    _72[21u] = 4292853756u;
    _73[21u] = 3212767183u;
    _70[22u] = 0u;
    _71[22u] = 0u;
    _72[22u] = 4292853756u;
    _73[22u] = 1023274975u;
    _70[23u] = 0u;
    _71[23u] = 0u;
    _72[23u] = 4292853756u;
    _73[23u] = 3206426575u;
    _70[24u] = 0u;
    _71[24u] = 0u;
    _72[24u] = 4292853756u;
    _73[24u] = 941109199u;
    _70[25u] = 0u;
    _71[25u] = 0u;
    _72[25u] = 2145367972u;
    _73[25u] = 403713740u;
    float _268 = floor(gl_FragCoord.x);
    float _269 = floor(gl_FragCoord.y);
    uint _270 = uint(_268);
    uint _271 = uint(_269);
    float4 _275 = _24[0u].Load(int3(uint2(_270, _271), 0u));
    float _278 = _275.x;
    float _279 = _268 + 0.5f;
    float _281 = _269 + 0.5f;
    float _296 = (((float(_270) + 0.5f) / _48_m0[691u].x) * 2.0f) + (-1.0f);
    float _298 = 1.0f - (((float(_271) + 0.5f) / _48_m0[691u].y) * 2.0f);
    float _343 = mad(_278, _53_m0[1139u].z, mad(_298, _53_m0[1139u].y, _53_m0[1139u].x * _296)) + _53_m0[1139u].w;
    float _344 = (mad(_278, _53_m0[1136u].z, mad(_298, _53_m0[1136u].y, _53_m0[1136u].x * _296)) + _53_m0[1136u].w) / _343;
    float _345 = (mad(_278, _53_m0[1137u].z, mad(_298, _53_m0[1137u].y, _53_m0[1137u].x * _296)) + _53_m0[1137u].w) / _343;
    float _346 = (mad(_278, _53_m0[1138u].z, mad(_298, _53_m0[1138u].y, _53_m0[1138u].x * _296)) + _53_m0[1138u].w) / _343;
    float _352 = _48_m0[8u].x + _344;
    float _353 = _48_m0[8u].y + _345;
    float _354 = _48_m0[8u].z + _346;
    uint4 _359 = _28[2u].Load(int3(uint2(_270, _271), 0u));
    uint _362 = _359.w;
    uint4 _367 = _28[15u].Load(int3(uint2(_270, _271), 0u));
    uint _369 = _367.y;
    uint _378 = ((_369 & 64u) != 0u) ? uint((_369 & 4294967167u) != 66u) : 4294967295u;
    uint _379 = _362 & 128u;
    uint _382 = (_379 != 0u) ? 1u : ((_359.x << 7u) | _362);
    uint4 _385 = _8.Load(_382 * 4u);
    uint _386 = _385.x;
    uint4 _389 = _8.Load((_382 * 4u) + 1u);
    uint _390 = _389.x;
    uint4 _397 = _8.Load((_382 * 4u) + 3u);
    uint _398 = _397.x;
    uint _401 = ((_386 & 1u) != 0u) ? 0u : 18u;
    uint _403 = uint(min(int(uint(max(int(_378), int(0u)))), int(1u)));
    uint _411;
    if (_379 == 0u)
    {
        _411 = (((_386 & 2097152u) != 0u) && (_378 == _403)) ? (_401 | 128u) : _401;
    }
    else
    {
        _411 = _362;
    }
    float _431 = asfloat(_10.Load((_398 * 115u) + 1u).x);
    uint4 _435 = _10.Load((_398 * 115u) + 104u);
    uint _436 = _435.x;
    float _442 = asfloat(_10.Load((_398 * 115u) + 105u).x);
    uint _450 = _390 & 65536u;
    float _505;
    float _507;
    float _509;
    float _511;
    float _513;
    float _515;
    float _517;
    float _519;
    float _521;
    uint _523;
    float _524;
    float _527;
    float _530;
    float _533;
    float _535;
    float _537;
    float _539;
    float _542;
    float _545;
    float _548;
    float _551;
    float _553;
    float _555;
    float _558;
    float _560;
    float _561;
    float _563;
    float _565;
    float _567;
    float _569;
    float _571;
    float _573;
    float _574;
    float _575;
    float _576;
    uint _578;
    if (((_386 & 1u) | _450) == 0u)
    {
        uint4 _456 = _28[2u].Load(int3(uint2(_270, _271), 0u));
        uint _459 = _456.z;
        uint _460 = _411 & 128u;
        bool _461 = _460 == 0u;
        float _463;
        uint _466;
        uint _468;
        if (_461)
        {
            _463 = 0.0f;
            _466 = 0u;
            _468 = 0u;
        }
        else
        {
            _463 = float(_456.x & 127u) * 0.0078740157186985015869140625f;
            _466 = _411 & 1u;
            _468 = 1u;
        }
        precise float _476 = float(_459 & 127u) * 0.0078740157186985015869140625f;
        precise float _478 = float(_456.y & 127u) * 0.0078740157186985015869140625f;
        float4 _481 = _24[3u].Load(int3(uint2(_270, _271), 0u));
        float _483 = _481.x;
        float _484 = _481.y;
        float _485 = _481.z;
        float _486 = _481.w;
        uint4 _489 = _28[1u].Load(int3(uint2(_270, _271), 0u));
        uint _491 = _489.x;
        float4 _494 = _24[8u].Load(int3(uint2(_270, _271), 0u));
        uint _647;
        if ((_460 | _466) == 0u)
        {
            _647 = 0u;
        }
        else
        {
            _647 = _28[9u].Load(int3(uint2(_270, _271), 0u)).x;
        }
        uint _744;
        if (_468 == 0u)
        {
            _744 = 0u;
        }
        else
        {
            _744 = _28[10u].Load(int3(uint2(_270, _271), 0u)).x;
        }
        float _754 = (float((_491 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _755 = (float(_491 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _759 = (1.0f - abs(_754)) - abs(_755);
        float _761 = clamp((-0.0f) - _759, 0.0f, 1.0f);
        float _762 = (-0.0f) - _761;
        float _767 = ((_754 >= 0.0f) ? _762 : _761) + _754;
        float _768 = ((_755 >= 0.0f) ? _762 : _761) + _755;
        float _772 = rsqrt(dot(float3(_767, _768, _759), float3(_767, _768, _759)));
        float _526 = _767 * _772;
        float _529 = _768 * _772;
        float _532 = _772 * _759;
        float _557 = float(_491 & 255u) * 0.0039215688593685626983642578125f;
        float _811;
        float _812;
        float _815;
        float _817;
        float _819;
        if (_461)
        {
            _811 = _486;
            _812 = 0.039999999105930328369140625f;
            _815 = 0.039999999105930328369140625f;
            _817 = 0.039999999105930328369140625f;
            _819 = 0.039999999105930328369140625f;
        }
        else
        {
            float _820 = ((_411 & 32u) != 0u) ? _486 : 0.039999999105930328369140625f;
            float frontier_phi_19_20_ladder;
            float frontier_phi_19_20_ladder_1;
            float frontier_phi_19_20_ladder_2;
            float frontier_phi_19_20_ladder_3;
            float frontier_phi_19_20_ladder_4;
            if ((_411 & 1u) == 0u)
            {
                frontier_phi_19_20_ladder = 0.0f;
                frontier_phi_19_20_ladder_1 = spvUnpackHalf2x16((_647 << 4u) & 32752u).x;
                frontier_phi_19_20_ladder_2 = spvUnpackHalf2x16((_647 >> 7u) & 32752u).x;
                frontier_phi_19_20_ladder_3 = spvUnpackHalf2x16((_647 >> 17u) & 32736u).x;
                frontier_phi_19_20_ladder_4 = _820;
            }
            else
            {
                frontier_phi_19_20_ladder = 0.0f;
                frontier_phi_19_20_ladder_1 = 0.039999999105930328369140625f;
                frontier_phi_19_20_ladder_2 = 0.039999999105930328369140625f;
                frontier_phi_19_20_ladder_3 = 0.039999999105930328369140625f;
                frontier_phi_19_20_ladder_4 = _820;
            }
            _811 = frontier_phi_19_20_ladder;
            _812 = frontier_phi_19_20_ladder_1;
            _815 = frontier_phi_19_20_ladder_2;
            _817 = frontier_phi_19_20_ladder_3;
            _819 = frontier_phi_19_20_ladder_4;
        }
        bool _822 = (_411 & 64u) != 0u;
        float _512;
        float _514;
        float _516;
        if ((_411 & 1u) == 0u)
        {
            _512 = 0.0f;
            _514 = 0.0f;
            _516 = 0.0f;
        }
        else
        {
            _512 = spvUnpackHalf2x16((_647 << 4u) & 32752u).x;
            _514 = spvUnpackHalf2x16((_647 >> 7u) & 32752u).x;
            _516 = spvUnpackHalf2x16((_647 >> 17u) & 32736u).x;
        }
        float _518;
        float _520;
        float _522;
        float _534;
        float _536;
        float _538;
        if ((_411 & 65u) == 0u)
        {
            float frontier_phi_33_28_ladder;
            float frontier_phi_33_28_ladder_1;
            float frontier_phi_33_28_ladder_2;
            float frontier_phi_33_28_ladder_3;
            float frontier_phi_33_28_ladder_4;
            float frontier_phi_33_28_ladder_5;
            if (_468 == 0u)
            {
                frontier_phi_33_28_ladder = 0.0f;
                frontier_phi_33_28_ladder_1 = 0.0f;
                frontier_phi_33_28_ladder_2 = 0.0f;
                frontier_phi_33_28_ladder_3 = _532;
                frontier_phi_33_28_ladder_4 = _529;
                frontier_phi_33_28_ladder_5 = _526;
            }
            else
            {
                float2 _1019 = spvUnpackHalf2x16((_744 >> 17u) & 32736u);
                float _1020 = _1019.x;
                float _1023 = (spvUnpackHalf2x16((_744 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                float _1024 = (spvUnpackHalf2x16((_744 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                float _1028 = (1.0f - abs(_1023)) - abs(_1024);
                float _1030 = clamp((-0.0f) - _1028, 0.0f, 1.0f);
                float _1031 = (-0.0f) - _1030;
                float _1036 = ((_1023 >= 0.0f) ? _1031 : _1030) + _1023;
                float _1037 = ((_1024 >= 0.0f) ? _1031 : _1030) + _1024;
                float _1041 = rsqrt(dot(float3(_1036, _1037, _1028), float3(_1036, _1037, _1028)));
                float _1051 = (((_1036 * _1041) - _526) * _1020) + _526;
                float _1052 = (((_1037 * _1041) - _529) * _1020) + _529;
                float _1053 = (((_1041 * _1028) - _532) * _1020) + _532;
                float _1057 = rsqrt(dot(float3(_1051, _1052, _1053), float3(_1051, _1052, _1053)));
                frontier_phi_33_28_ladder = 0.0f;
                frontier_phi_33_28_ladder_1 = 0.0f;
                frontier_phi_33_28_ladder_2 = 0.0f;
                frontier_phi_33_28_ladder_3 = _1053 * _1057;
                frontier_phi_33_28_ladder_4 = _1052 * _1057;
                frontier_phi_33_28_ladder_5 = _1051 * _1057;
            }
            _534 = frontier_phi_33_28_ladder_5;
            _536 = frontier_phi_33_28_ladder_4;
            _538 = frontier_phi_33_28_ladder_3;
            _518 = frontier_phi_33_28_ladder_2;
            _520 = frontier_phi_33_28_ladder_1;
            _522 = frontier_phi_33_28_ladder;
        }
        else
        {
            _534 = _526;
            _536 = _529;
            _538 = _532;
            _518 = spvUnpackHalf2x16((_744 << 4u) & 32752u).x;
            _520 = spvUnpackHalf2x16((_744 >> 7u) & 32752u).x;
            _522 = spvUnpackHalf2x16((_744 >> 17u) & 32736u).x;
        }
        float _1007 = 1.0f - _811;
        float _541 = _1007 * _483;
        float _544 = _1007 * _484;
        float _547 = _1007 * _485;
        float _1135;
        float _1137;
        float _1139;
        if (_431 != 0.0f)
        {
            float _1132 = max(max(_483, _484), _485);
            float frontier_phi_38_37_ladder;
            float frontier_phi_38_37_ladder_1;
            float frontier_phi_38_37_ladder_2;
            if (_1132 > 0.001000000047497451305389404296875f)
            {
                float _1280 = _431 / min(_431, _1132);
                frontier_phi_38_37_ladder = _1280 * _484;
                frontier_phi_38_37_ladder_1 = _1280 * _483;
                frontier_phi_38_37_ladder_2 = _1280 * _485;
            }
            else
            {
                frontier_phi_38_37_ladder = _431;
                frontier_phi_38_37_ladder_1 = _431;
                frontier_phi_38_37_ladder_2 = _431;
            }
            _1135 = frontier_phi_38_37_ladder_1;
            _1137 = frontier_phi_38_37_ladder;
            _1139 = frontier_phi_38_37_ladder_2;
        }
        else
        {
            _1135 = _483;
            _1137 = _484;
            _1139 = _485;
        }
        float _550;
        float _552;
        float _554;
        if ((_411 & 32u) == 0u)
        {
            _550 = ((_1135 - _812) * _811) + _812;
            _552 = ((_1137 - _815) * _811) + _815;
            _554 = ((_1139 - _817) * _811) + _817;
        }
        else
        {
            _550 = _819;
            _552 = _819;
            _554 = _819;
        }
        _505 = _822 ? _494.x : 0.0f;
        _507 = _822 ? _494.y : 0.0f;
        _509 = _822 ? _494.z : 0.0f;
        _511 = _512;
        _513 = _514;
        _515 = _516;
        _517 = _518;
        _519 = _520;
        _521 = _522;
        _523 = (_459 >> 7u) & 1u;
        _524 = _526;
        _527 = _529;
        _530 = _532;
        _533 = _534;
        _535 = _536;
        _537 = _538;
        _539 = _541;
        _542 = _544;
        _545 = _547;
        _548 = _550;
        _551 = _552;
        _553 = _554;
        _555 = _557;
        _558 = _476;
        _560 = _494.w;
        _561 = _478;
        _563 = 0.0f;
        _565 = 0.0f;
        _567 = 0.0f;
        _569 = 0.0f;
        _571 = _461 ? 0.0f : _463;
        _573 = _541;
        _574 = _544;
        _575 = _547;
        _576 = 1.0f - _557;
        _578 = _390;
    }
    else
    {
        float frontier_phi_7_4_ladder;
        float frontier_phi_7_4_ladder_1;
        float frontier_phi_7_4_ladder_2;
        float frontier_phi_7_4_ladder_3;
        float frontier_phi_7_4_ladder_4;
        float frontier_phi_7_4_ladder_5;
        float frontier_phi_7_4_ladder_6;
        float frontier_phi_7_4_ladder_7;
        float frontier_phi_7_4_ladder_8;
        uint frontier_phi_7_4_ladder_9;
        float frontier_phi_7_4_ladder_10;
        float frontier_phi_7_4_ladder_11;
        float frontier_phi_7_4_ladder_12;
        float frontier_phi_7_4_ladder_13;
        float frontier_phi_7_4_ladder_14;
        float frontier_phi_7_4_ladder_15;
        float frontier_phi_7_4_ladder_16;
        float frontier_phi_7_4_ladder_17;
        float frontier_phi_7_4_ladder_18;
        float frontier_phi_7_4_ladder_19;
        uint frontier_phi_7_4_ladder_20;
        float frontier_phi_7_4_ladder_21;
        float frontier_phi_7_4_ladder_22;
        float frontier_phi_7_4_ladder_23;
        float frontier_phi_7_4_ladder_24;
        float frontier_phi_7_4_ladder_25;
        float frontier_phi_7_4_ladder_26;
        float frontier_phi_7_4_ladder_27;
        float frontier_phi_7_4_ladder_28;
        float frontier_phi_7_4_ladder_29;
        float frontier_phi_7_4_ladder_30;
        float frontier_phi_7_4_ladder_31;
        float frontier_phi_7_4_ladder_32;
        float frontier_phi_7_4_ladder_33;
        float frontier_phi_7_4_ladder_34;
        float frontier_phi_7_4_ladder_35;
        if (_450 == 0u)
        {
            frontier_phi_7_4_ladder = 0.0f;
            frontier_phi_7_4_ladder_1 = 0.0f;
            frontier_phi_7_4_ladder_2 = 0.0f;
            frontier_phi_7_4_ladder_3 = 0.0f;
            frontier_phi_7_4_ladder_4 = 0.0f;
            frontier_phi_7_4_ladder_5 = 0.0f;
            frontier_phi_7_4_ladder_6 = 0.0f;
            frontier_phi_7_4_ladder_7 = 0.0f;
            frontier_phi_7_4_ladder_8 = 0.0f;
            frontier_phi_7_4_ladder_9 = 0u;
            frontier_phi_7_4_ladder_10 = 0.0f;
            frontier_phi_7_4_ladder_11 = 0.0f;
            frontier_phi_7_4_ladder_12 = 0.0f;
            frontier_phi_7_4_ladder_13 = 0.0f;
            frontier_phi_7_4_ladder_14 = 0.0f;
            frontier_phi_7_4_ladder_15 = 0.0f;
            frontier_phi_7_4_ladder_16 = 0.0f;
            frontier_phi_7_4_ladder_17 = 0.0f;
            frontier_phi_7_4_ladder_18 = 0.0f;
            frontier_phi_7_4_ladder_19 = 0.0f;
            frontier_phi_7_4_ladder_20 = 0u;
            frontier_phi_7_4_ladder_21 = 0.0f;
            frontier_phi_7_4_ladder_22 = 0.0f;
            frontier_phi_7_4_ladder_23 = 0.0f;
            frontier_phi_7_4_ladder_24 = 0.0f;
            frontier_phi_7_4_ladder_25 = 0.0f;
            frontier_phi_7_4_ladder_26 = 0.0f;
            frontier_phi_7_4_ladder_27 = 0.0f;
            frontier_phi_7_4_ladder_28 = 0.0f;
            frontier_phi_7_4_ladder_29 = 0.0f;
            frontier_phi_7_4_ladder_30 = 1.0f;
            frontier_phi_7_4_ladder_31 = 1.0f;
            frontier_phi_7_4_ladder_32 = 1.0f;
            frontier_phi_7_4_ladder_33 = 0.0f;
            frontier_phi_7_4_ladder_34 = 0.0f;
            frontier_phi_7_4_ladder_35 = 0.0f;
        }
        else
        {
            uint4 _590 = _28[2u].Load(int3(uint2(_270, _271), 0u));
            float4 _596 = _24[3u].Load(int3(uint2(_270, _271), 0u));
            uint4 _600 = _28[1u].Load(int3(uint2(_270, _271), 0u));
            uint _602 = _600.x;
            float4 _605 = _24[8u].Load(int3(uint2(_270, _271), 0u));
            float _618 = (float((_602 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
            float _619 = (float(_602 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
            float _623 = (1.0f - abs(_618)) - abs(_619);
            float _626 = clamp((-0.0f) - _623, 0.0f, 1.0f);
            float _627 = (-0.0f) - _626;
            float _632 = ((_618 >= 0.0f) ? _627 : _626) + _618;
            float _633 = ((_619 >= 0.0f) ? _627 : _626) + _619;
            float _638 = rsqrt(dot(float3(_632, _633, _623), float3(_632, _633, _623)));
            float _525 = _632 * _638;
            float _528 = _633 * _638;
            float _531 = _638 * _623;
            float _646 = asfloat(_10.Load(_398 * 115u).x);
            frontier_phi_7_4_ladder = _596.y;
            frontier_phi_7_4_ladder_1 = _646;
            frontier_phi_7_4_ladder_2 = _596.x;
            frontier_phi_7_4_ladder_3 = _531;
            frontier_phi_7_4_ladder_4 = _528;
            frontier_phi_7_4_ladder_5 = _525;
            frontier_phi_7_4_ladder_6 = _531;
            frontier_phi_7_4_ladder_7 = _528;
            frontier_phi_7_4_ladder_8 = _525;
            frontier_phi_7_4_ladder_9 = 0u;
            frontier_phi_7_4_ladder_10 = 0.0f;
            frontier_phi_7_4_ladder_11 = 0.0f;
            frontier_phi_7_4_ladder_12 = 0.0f;
            frontier_phi_7_4_ladder_13 = 0.0f;
            frontier_phi_7_4_ladder_14 = 0.0f;
            frontier_phi_7_4_ladder_15 = 0.0f;
            frontier_phi_7_4_ladder_16 = 0.0f;
            frontier_phi_7_4_ladder_17 = 0.0f;
            frontier_phi_7_4_ladder_18 = 0.0f;
            frontier_phi_7_4_ladder_19 = _596.z;
            frontier_phi_7_4_ladder_20 = _390;
            frontier_phi_7_4_ladder_21 = 0.0f;
            frontier_phi_7_4_ladder_22 = 0.0f;
            frontier_phi_7_4_ladder_23 = 0.0f;
            frontier_phi_7_4_ladder_24 = 0.0f;
            frontier_phi_7_4_ladder_25 = 0.0f;
            frontier_phi_7_4_ladder_26 = _605.z;
            frontier_phi_7_4_ladder_27 = _605.y;
            frontier_phi_7_4_ladder_28 = _605.x;
            frontier_phi_7_4_ladder_29 = _596.w;
            frontier_phi_7_4_ladder_30 = float(_590.z) * 0.0039215688593685626983642578125f;
            frontier_phi_7_4_ladder_31 = 1.0f;
            frontier_phi_7_4_ladder_32 = float(_590.y) * 0.0039215688593685626983642578125f;
            frontier_phi_7_4_ladder_33 = float(_602 & 255u) * 0.0039215688593685626983642578125f;
            frontier_phi_7_4_ladder_34 = _646;
            frontier_phi_7_4_ladder_35 = _646;
        }
        _505 = frontier_phi_7_4_ladder_18;
        _507 = frontier_phi_7_4_ladder_17;
        _509 = frontier_phi_7_4_ladder_16;
        _511 = frontier_phi_7_4_ladder_15;
        _513 = frontier_phi_7_4_ladder_14;
        _515 = frontier_phi_7_4_ladder_13;
        _517 = frontier_phi_7_4_ladder_12;
        _519 = frontier_phi_7_4_ladder_11;
        _521 = frontier_phi_7_4_ladder_10;
        _523 = frontier_phi_7_4_ladder_9;
        _524 = frontier_phi_7_4_ladder_8;
        _527 = frontier_phi_7_4_ladder_7;
        _530 = frontier_phi_7_4_ladder_6;
        _533 = frontier_phi_7_4_ladder_5;
        _535 = frontier_phi_7_4_ladder_4;
        _537 = frontier_phi_7_4_ladder_3;
        _539 = frontier_phi_7_4_ladder_2;
        _542 = frontier_phi_7_4_ladder;
        _545 = frontier_phi_7_4_ladder_19;
        _548 = frontier_phi_7_4_ladder_1;
        _551 = frontier_phi_7_4_ladder_35;
        _553 = frontier_phi_7_4_ladder_34;
        _555 = frontier_phi_7_4_ladder_33;
        _558 = frontier_phi_7_4_ladder_32;
        _560 = frontier_phi_7_4_ladder_31;
        _561 = frontier_phi_7_4_ladder_30;
        _563 = frontier_phi_7_4_ladder_29;
        _565 = frontier_phi_7_4_ladder_28;
        _567 = frontier_phi_7_4_ladder_27;
        _569 = frontier_phi_7_4_ladder_26;
        _571 = frontier_phi_7_4_ladder_25;
        _573 = frontier_phi_7_4_ladder_24;
        _574 = frontier_phi_7_4_ladder_23;
        _575 = frontier_phi_7_4_ladder_22;
        _576 = frontier_phi_7_4_ladder_21;
        _578 = frontier_phi_7_4_ladder_20;
    }
    bool _580 = (_411 & 128u) == 0u;
    bool _581 = !_580;
    bool _584 = (_411 & 32u) != 0u;
    float _654;
    float _656;
    float _658;
    float _660;
    if ((_411 & 144u) == 0u)
    {
        _654 = 0.0f;
        _656 = 0.0f;
        _658 = 0.0f;
        _660 = 0.0f;
    }
    else
    {
        float _685 = _279 / _53_m0[1140u].x;
        float _686 = _281 / _53_m0[1140u].y;
        float _692 = sqrt(((_345 * _345) + (_344 * _344)) + (_346 * _346));
        uint _693 = _403 * 3u;
        uint _694 = _693 + 1155u;
        uint _703 = _693 + 1157u;
        float _722 = clamp(((_692 / _53_m0[1162u].z) - _53_m0[1161u].x) / (_53_m0[1161u].y - _53_m0[1161u].x), 0.0f, 1.0f);
        float _832;
        float _833;
        float _834;
        float _835;
        float _836;
        float _837;
        if (_580)
        {
            _832 = _53_m0[107u].w;
            _833 = _53_m0[106u].z;
            _834 = _53_m0[1u].x;
            _835 = _53_m0[1u].y;
            _836 = _53_m0[1u].z;
            _837 = _48_m0[66u].z;
        }
        else
        {
            _832 = ((_53_m0[1161u].w - _53_m0[1161u].z) * _722) + _53_m0[1161u].z;
            _833 = _53_m0[_703].y;
            _834 = _53_m0[_694].x;
            _835 = _53_m0[_694].y;
            _836 = _53_m0[_694].z;
            _837 = _48_m0[66u].y;
        }
        uint4 _841 = asuint(_48_m0[72u]);
        bool _845 = (_580 ? _841.y : _841.z) != 0u;
        float _846 = (-0.0f) - _344;
        float _847 = (-0.0f) - _345;
        float _848 = (-0.0f) - _346;
        float _852 = rsqrt(dot(float3(_846, _847, _848), float3(_846, _847, _848)));
        float _853 = _852 * _846;
        float _854 = _852 * _847;
        float _855 = _852 * _848;
        float _886;
        float _887;
        uint _888;
        if ((_523 == 0u) || (_48_m0[178u].w == 0.0f))
        {
            _886 = 0.0f;
            _887 = 0.0f;
            _888 = 0u;
        }
        else
        {
            uint frontier_phi_26_27_ladder;
            float frontier_phi_26_27_ladder_1;
            float frontier_phi_26_27_ladder_2;
            if (_24[21u].SampleLevel(_61, float2(_685, _686), 0.0f).x < _278)
            {
                frontier_phi_26_27_ladder = 1u;
                frontier_phi_26_27_ladder_1 = _686;
                frontier_phi_26_27_ladder_2 = _685;
            }
            else
            {
                frontier_phi_26_27_ladder = 0u;
                frontier_phi_26_27_ladder_1 = 0.0f;
                frontier_phi_26_27_ladder_2 = 0.0f;
            }
            _886 = frontier_phi_26_27_ladder_2;
            _887 = frontier_phi_26_27_ladder_1;
            _888 = frontier_phi_26_27_ladder;
        }
        float _897 = clamp((_692 - _53_m0[1164u].y) / (_53_m0[1164u].z - _53_m0[1164u].y), 0.0f, 1.0f);
        float _908 = _53_m0[1163u].w - (((_897 * _897) * (3.0f - (_897 * 2.0f))) * _53_m0[1163u].w);
        float4 _921 = _15[585u].SampleLevel(_56, float2(_279 / _53_m0[1140u].x, _281 / _53_m0[1140u].y), 0.0f);
        float _940 = (_921.w + (-1.0f)) * _908;
        float _941 = _940 + 1.0f;
        float4 _946 = _24[14u].SampleLevel(_56, float2(_685, _686), 0.0f);
        float _948 = _946.x;
        float4 _953 = _24[13u].SampleLevel(_56, float2(_685, _686), 0.0f);
        float _961 = _953.x / _48_m0[59u].x;
        float _962 = _953.y / _48_m0[59u].x;
        float _963 = _953.z / _48_m0[59u].x;
        float _968 = _961 - (_961 * _442);
        float _969 = _962 - (_962 * _442);
        float _970 = _963 - (_963 * _442);
        float _972 = ((_948 * _948) + (-1.0f)) * asfloat(_9.Load((_8.Load((_382 * 4u) + 2u).x * 57u) + 19u).x);
        float _973 = _972 + 1.0f;
        float _996;
        if (asuint(_53_m0[1154u]).z == 0u)
        {
            _996 = 1.0f;
        }
        else
        {
            _996 = (_48_m0[160u].w * _940) + 1.0f;
        }
        float _998 = _941 * _558;
        float _1095;
        if (_580)
        {
            float _1087 = _352 - _53_m0[114u].x;
            float _1088 = _353 - _53_m0[114u].y;
            float _1089 = _354 - _53_m0[114u].z;
            float frontier_phi_36_35_ladder;
            if (dot(float3(_1087, _1088, _1089), float3(_1087, _1088, _1089)) < (_53_m0[113u].x * _53_m0[113u].x))
            {
                float4 _1168 = _18.SampleLevel(_57, float3((dot(float3(_1087, _1088, _1089), float3(_53_m0[115u].xyz)) * _53_m0[118u].x) + 0.5f, (dot(float3(_1087, _1088, _1089), float3(_53_m0[116u].xyz)) * _53_m0[118u].y) + 0.5f, (dot(float3(_1087, _1088, _1089), float3(_53_m0[117u].xyz)) * _53_m0[118u].z) + 0.5f), 0.0f);
                float _1173 = _1168.w;
                float _1183 = _1173 * _1173;
                float _1184 = _1183 * 2.0f;
                frontier_phi_36_35_ladder = 1.0f - (exp2(log2(clamp((dot(float3((_1184 * _1168.x) - _1183, (_1184 * _1168.y) - _1183, (_1184 * _1168.z) - _1183), float3(dot(float3(_524, _527, _530), float3(_53_m0[115u].xyz)), dot(float3(_524, _527, _530), float3(_53_m0[116u].xyz)), dot(float3(_524, _527, _530), float3(_53_m0[117u].xyz)))) * _53_m0[113u].z) + _1183, 0.0f, 1.0f)) * _53_m0[113u].y) * _53_m0[113u].w);
            }
            else
            {
                frontier_phi_36_35_ladder = 1.0f;
            }
            _1095 = frontier_phi_36_35_ladder;
        }
        else
        {
            _1095 = 1.0f;
        }
        float _1097 = _1095 * _998;
        float _1098 = _1097 * _973;
        float _1101 = ((1.0f - _1098) * _442) + _1098;
        float _1109 = (1.0f - _558) - ((_1097 - _558) * asfloat(_10.Load((_398 * 115u) + 112u).x));
        float _1124 = (_48_m0[84u].z * _972) + 1.0f;
        float _1125 = (-0.0f) - _853;
        float _1126 = (-0.0f) - _854;
        float _1127 = (-0.0f) - _855;
        float _1128 = dot(float3(_1125, _1126, _1127), float3(_533, _535, _537));
        float _1296;
        float _1298;
        if (_580)
        {
            float _1201 = _1128 * 2.0f;
            float _1221 = max(max(exp2(log2(clamp(1.0f - _555, 0.0f, 1.0f)) * _53_m0[107u].y), _832), 0.00999999977648258209228515625f);
            float _1224 = sqrt(1.0f - _560);
            float _1228 = exp2((_1221 * _1221) * (-3.321929931640625f));
            float _1234 = acos(_1224);
            float _1235 = acos(_1228);
            float _1236 = acos((dot(float3(_533, _535, _537), float3(_1125 - (_1201 * _533), _1126 - (_1201 * _535), _1127 - (_1201 * _537))) * 0.5f) + 0.5f);
            float _1341;
            if (_1236 > (max(_1234, _1235) - min(_1234, _1235)))
            {
                float _1290 = _1235 + _1234;
                float frontier_phi_49_45_ladder;
                if (_1236 < _1290)
                {
                    float _1324 = abs(_1234 - _1235);
                    float _1332 = clamp(1.0f - clamp((_1236 - _1324) / max(_1290 - _1324, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f), 0.0f, 1.0f);
                    frontier_phi_49_45_ladder = ((_1332 * _1332) * (3.0f - (_1332 * 2.0f))) * (6.283185482025146484375f - (max(_1224, _1228) * 6.283185482025146484375f));
                }
                else
                {
                    frontier_phi_49_45_ladder = 0.0f;
                }
                _1341 = frontier_phi_49_45_ladder;
            }
            else
            {
                _1341 = 6.283185482025146484375f - (max(_1224, _1228) * 6.283185482025146484375f);
            }
            float _1297 = ((_941 * _941) * ((asuint(_48_m0[156u]).z != 0u) ? 1.0f : _1124)) * (_1341 / ((1.0f - _1228) * 6.283185482025146484375f));
            _1296 = _1297;
            _1298 = _1297;
        }
        else
        {
            float _1257 = clamp((((_48_m0[73u].z * (_1124 - _1097)) + _1097) - _48_m0[62u].x) / (_48_m0[62u].y - _48_m0[62u].x), 0.0f, 1.0f) * _560;
            float _1269 = clamp(((_854 + ((_1128 * 2.0f) * _535)) - _48_m0[75u].x) / (_48_m0[75u].y - _48_m0[75u].x), 0.0f, 1.0f);
            _1296 = (((((_1269 * _1269) * (3.0f - (_1269 * 2.0f))) + (-1.0f)) * _48_m0[75u].z) + 1.0f) * _1257;
            _1298 = _1257;
        }
        float _1299 = _996 * _998;
        float _1300 = _1299 * _968;
        float _1301 = _1299 * _969;
        float _1302 = _1299 * _970;
        float _1309 = (((_921.x / _48_m0[59u].x) - _1300) * _908) + _1300;
        float _1310 = (((_921.y / _48_m0[59u].x) - _1301) * _908) + _1301;
        float _1311 = (((_921.z / _48_m0[59u].x) - _1302) * _908) + _1302;
        float _1318 = ((_48_m0[128u].z * (_1095 + (-1.0f))) + 1.0f) * _561;
        float _1320 = (_584 && _581) ? _571 : 1.0f;
        float _1353;
        float _1354;
        float _1355;
        if ((_411 & 64u) == 0u)
        {
            _1353 = (1.0f - _548) * _539;
            _1354 = (1.0f - _551) * _542;
            _1355 = (1.0f - _553) * _545;
        }
        else
        {
            _1353 = _505;
            _1354 = _507;
            _1355 = _509;
        }
        float _1370;
        float _1375;
        float _1380;
        float _1385;
        float _1388;
        float _1391;
        if ((_411 & 1u) == 0u)
        {
            float frontier_phi_56_52_ladder;
            float frontier_phi_56_52_ladder_1;
            float frontier_phi_56_52_ladder_2;
            float frontier_phi_56_52_ladder_3;
            float frontier_phi_56_52_ladder_4;
            float frontier_phi_56_52_ladder_5;
            if (_580)
            {
                float frontier_phi_56_52_ladder_54_ladder;
                float frontier_phi_56_52_ladder_54_ladder_1;
                float frontier_phi_56_52_ladder_54_ladder_2;
                float frontier_phi_56_52_ladder_54_ladder_3;
                float frontier_phi_56_52_ladder_54_ladder_4;
                float frontier_phi_56_52_ladder_54_ladder_5;
                if ((_578 & 65536u) == 0u)
                {
                    float4 _1406 = _31[6u].SampleLevel(_60, float3(_524, _527, _530), 0.0f);
                    float _1386 = _1406.x * _48_m0[59u].y;
                    float _1389 = _1406.y * _48_m0[59u].y;
                    float _1392 = _1406.z * _48_m0[59u].y;
                    frontier_phi_56_52_ladder_54_ladder = _1392;
                    frontier_phi_56_52_ladder_54_ladder_1 = _1389;
                    frontier_phi_56_52_ladder_54_ladder_2 = _1386;
                    frontier_phi_56_52_ladder_54_ladder_3 = ((1.0f - _553) * _545) * _1392;
                    frontier_phi_56_52_ladder_54_ladder_4 = ((1.0f - _551) * _542) * _1389;
                    frontier_phi_56_52_ladder_54_ladder_5 = ((1.0f - _548) * _539) * _1386;
                }
                else
                {
                    float4 _1420 = _31[6u].SampleLevel(_60, float3(_524, _527, _530), 0.0f);
                    float _1387 = _1420.x * _48_m0[59u].y;
                    float _1390 = _1420.y * _48_m0[59u].y;
                    float _1393 = _1420.z * _48_m0[59u].y;
                    frontier_phi_56_52_ladder_54_ladder = _1393;
                    frontier_phi_56_52_ladder_54_ladder_1 = _1390;
                    frontier_phi_56_52_ladder_54_ladder_2 = _1387;
                    frontier_phi_56_52_ladder_54_ladder_3 = ((1.0f - _553) * _545) * _1393;
                    frontier_phi_56_52_ladder_54_ladder_4 = ((1.0f - _551) * _542) * _1390;
                    frontier_phi_56_52_ladder_54_ladder_5 = ((1.0f - _548) * _539) * _1387;
                }
                frontier_phi_56_52_ladder = frontier_phi_56_52_ladder_54_ladder;
                frontier_phi_56_52_ladder_1 = frontier_phi_56_52_ladder_54_ladder_1;
                frontier_phi_56_52_ladder_2 = frontier_phi_56_52_ladder_54_ladder_2;
                frontier_phi_56_52_ladder_3 = frontier_phi_56_52_ladder_54_ladder_3;
                frontier_phi_56_52_ladder_4 = frontier_phi_56_52_ladder_54_ladder_4;
                frontier_phi_56_52_ladder_5 = frontier_phi_56_52_ladder_54_ladder_5;
            }
            else
            {
                uint _1367 = (_403 + 50u) + 0u;
                float frontier_phi_56_52_ladder_55_ladder;
                float frontier_phi_56_52_ladder_55_ladder_1;
                float frontier_phi_56_52_ladder_55_ladder_2;
                float frontier_phi_56_52_ladder_55_ladder_3;
                float frontier_phi_56_52_ladder_55_ladder_4;
                float frontier_phi_56_52_ladder_55_ladder_5;
                if ((_578 & 65536u) == 0u)
                {
                    float4 _1434 = _31[NonUniformResourceIndex(_1367)].SampleLevel(_60, float3(_524, _527, _530), 0.0f);
                    frontier_phi_56_52_ladder_55_ladder = 0.0f;
                    frontier_phi_56_52_ladder_55_ladder_1 = 0.0f;
                    frontier_phi_56_52_ladder_55_ladder_2 = 0.0f;
                    frontier_phi_56_52_ladder_55_ladder_3 = (_1434.z * _48_m0[59u].y) * ((1.0f - _553) * _545);
                    frontier_phi_56_52_ladder_55_ladder_4 = (_1434.y * _48_m0[59u].y) * ((1.0f - _551) * _542);
                    frontier_phi_56_52_ladder_55_ladder_5 = (_1434.x * _48_m0[59u].y) * ((1.0f - _548) * _539);
                }
                else
                {
                    float4 _1451 = _31[NonUniformResourceIndex(_1367)].SampleLevel(_60, float3(_524, _527, _530), 0.0f);
                    frontier_phi_56_52_ladder_55_ladder = 0.0f;
                    frontier_phi_56_52_ladder_55_ladder_1 = 0.0f;
                    frontier_phi_56_52_ladder_55_ladder_2 = 0.0f;
                    frontier_phi_56_52_ladder_55_ladder_3 = ((_48_m0[59u].y * _545) * (1.0f - _553)) * _1451.z;
                    frontier_phi_56_52_ladder_55_ladder_4 = ((_48_m0[59u].y * _542) * (1.0f - _551)) * _1451.y;
                    frontier_phi_56_52_ladder_55_ladder_5 = ((_48_m0[59u].y * _539) * (1.0f - _548)) * _1451.x;
                }
                frontier_phi_56_52_ladder = frontier_phi_56_52_ladder_55_ladder;
                frontier_phi_56_52_ladder_1 = frontier_phi_56_52_ladder_55_ladder_1;
                frontier_phi_56_52_ladder_2 = frontier_phi_56_52_ladder_55_ladder_2;
                frontier_phi_56_52_ladder_3 = frontier_phi_56_52_ladder_55_ladder_3;
                frontier_phi_56_52_ladder_4 = frontier_phi_56_52_ladder_55_ladder_4;
                frontier_phi_56_52_ladder_5 = frontier_phi_56_52_ladder_55_ladder_5;
            }
            _1370 = frontier_phi_56_52_ladder_5;
            _1375 = frontier_phi_56_52_ladder_4;
            _1380 = frontier_phi_56_52_ladder_3;
            _1385 = frontier_phi_56_52_ladder_2;
            _1388 = frontier_phi_56_52_ladder_1;
            _1391 = frontier_phi_56_52_ladder;
        }
        else
        {
            _1370 = _511 / _48_m0[59u].x;
            _1375 = _513 / _48_m0[59u].x;
            _1380 = _515 / _48_m0[59u].x;
            _1385 = 0.0f;
            _1388 = 0.0f;
            _1391 = 0.0f;
        }
        float _1465;
        float _1467;
        float _1469;
        if ((_411 & 65u) == 0u)
        {
            _1465 = 0.0f;
            _1467 = 0.0f;
            _1469 = 0.0f;
        }
        else
        {
            _1465 = _517 / _48_m0[59u].x;
            _1467 = _519 / _48_m0[59u].x;
            _1469 = _521 / _48_m0[59u].x;
        }
        float _1475 = _1101 * (1.0f - _908);
        bool _1486 = asuint(_48_m0[156u]).z == 0u;
        float _2255;
        float _2257;
        float _2259;
        if (_580)
        {
            uint _1490;
            float _1491;
            float _1493;
            float _1495;
            if (_1486)
            {
                _1490 = 0u;
                _1491 = 0.0f;
                _1493 = 0.0f;
                _1495 = 0.0f;
            }
            else
            {
                float _1554 = (_1385 * _973) + _968;
                float _1555 = (_1388 * _973) + _969;
                float _1556 = (_1391 * _973) + _970;
                float _1569 = (((_1309 - _1554) * _908) + _1554) / _48_m0[59u].w;
                float _1570 = (((_1310 - _1555) * _908) + _1555) / _48_m0[59u].w;
                float _1571 = (((_1311 - _1556) * _908) + _1556) / _48_m0[59u].w;
                float _1575 = dot(float3(_1569, _1570, _1571), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                float _1599 = dot(float3(_31[5u].Sample(_60, float3(_524, _527, _530)).xyz), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                _1490 = 1u;
                _1491 = ((((((_1569 - _1575) * _48_m0[156u].w) + _1575) / _1599) + (-1.0f)) * _48_m0[169u].w) + 1.0f;
                _1493 = ((((((_1570 - _1575) * _48_m0[156u].w) + _1575) / _1599) + (-1.0f)) * _48_m0[169u].w) + 1.0f;
                _1495 = ((((((_1571 - _1575) * _48_m0[156u].w) + _1575) / _1599) + (-1.0f)) * _48_m0[169u].w) + 1.0f;
            }
            float _1531 = _1128 * 2.0f;
            float _1535 = _1125 - (_1531 * _533);
            float _1537 = _1127 - (_1531 * _537);
            float _1538 = (_1126 - (_1531 * _535)) * (((((exp2(log2(clamp((_692 - _48_m0[121u].y) * _48_m0[121u].z, 0.0f, 1.0f)) * _48_m0[121u].w) * asfloat(_10.Load((_398 * 115u) + 114u).x)) * exp2(log2(clamp((_345 - _48_m0[122u].x) * _48_m0[122u].y, 0.0f, 1.0f)) * _48_m0[122u].z)) * (1.0f - clamp(_527, 0.0f, 1.0f))) * (max(_48_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f);
            float _1542 = rsqrt(dot(float3(_1535, _1538, _1537), float3(_1535, _1538, _1537)));
            float _1543 = _1535 * _1542;
            float _1544 = _1538 * _1542;
            float _1545 = _1537 * _1542;
            float _1758;
            float _1759;
            float _1760;
            float _1761;
            if (_48_m0[176u].w > 0.0f)
            {
                float _1684 = 1.0f - _555;
                float _1687 = exp2(log2(_1684) * _48_m0[176u].w);
                float _1689 = clamp(1.0f - _1687, 0.0f, 1.0f);
                float _1693 = (sqrt(_1689 + 9.9999997473787516355514526367188e-05f) + _1687) * _1689;
                float _1700 = (_1693 * (_1543 - _533)) + _533;
                float _1701 = (_1693 * (_1544 - _535)) + _535;
                float _1702 = (_1693 * (_1545 - _537)) + _537;
                float _1706 = rsqrt(dot(float3(_1700, _1701, _1702), float3(_1700, _1701, _1702)));
                _1758 = _1684;
                _1759 = _1700 * _1706;
                _1760 = _1701 * _1706;
                _1761 = _1702 * _1706;
            }
            else
            {
                _1758 = 1.0f - _555;
                _1759 = _1543;
                _1760 = _1544;
                _1761 = _1545;
            }
            float _1763 = (_837 >= 0.0f) ? _837 : 8.0f;
            float _1822;
            if (_845)
            {
                _1822 = _1763 * _1758;
            }
            else
            {
                _1822 = max((_1763 + (-9.0f)) + ((_53_m0[107u].x * 9.0f) * exp2(log2(clamp(_1758, 0.0f, 1.0f)) * _53_m0[107u].z)), 0.0f);
            }
            float4 _1827 = _31[4u].SampleLevel(_58, float3(_1759, _1760, _1761), _1822);
            bool _1832 = _888 == 0u;
            float _1850;
            float _1852;
            float _1854;
            float _1856;
            if (_1832)
            {
                _1850 = 0.0f;
                _1852 = 0.0f;
                _1854 = 0.0f;
                _1856 = 0.0f;
            }
            else
            {
                float _1871 = min(_886, _48_m0[110u].z);
                float _1872 = min(_887, _48_m0[110u].w);
                float4 _1890 = _15[509u].SampleLevel(_56, float2(_1871, _1872), _15[557u].SampleLevel(_56, float2(_1871, _1872), 0.0f).x * _48_m0[72u].w);
                float _1892 = _1890.x;
                float _1893 = _1890.y;
                float _1894 = _1890.z;
                float _1914 = (1.0f / max(_48_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_1892, max(_1893, _1894))));
                _1850 = _1914 * _1892;
                _1852 = _1914 * _1893;
                _1854 = _1914 * _1894;
                _1856 = _1890.w;
            }
            float _1858 = 1.0f - _1856;
            float _1862 = (_1858 * _1827.x) + _1850;
            float _1863 = (_1858 * _1827.y) + _1852;
            float _1864 = (_1858 * _1827.z) + _1854;
            bool _1865 = _1490 == 0u;
            float _1984;
            float _1986;
            float _1988;
            if (_1865)
            {
                _1984 = _1862;
                _1986 = _1863;
                _1988 = _1864;
            }
            else
            {
                _1984 = min(1.0f, _1491) * _1862;
                _1986 = min(1.0f, _1493) * _1863;
                _1988 = min(1.0f, _1495) * _1864;
            }
            float _1999 = clamp(dot(float3(_533, _535, _537), float3(_853, _854, _855)), 0.0f, 1.0f);
            float _2008 = exp2(_1999 * (-9.27999973297119140625f));
            float _2015 = (((min(_555 * 0.4749999940395355224609375f, _2008) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _555) + (-0.015625f);
            float _2017 = ((_555 * 0.25f) + 0.75f) - _2015;
            float _2027 = (_48_m0[59u].w * _1984) * clamp((_2017 * _548) + _2015, 0.0f, 1.0f);
            float _2028 = (_48_m0[59u].w * _1986) * clamp((_2017 * _551) + _2015, 0.0f, 1.0f);
            float _2029 = (_48_m0[59u].w * _1988) * clamp((_2017 * _553) + _2015, 0.0f, 1.0f);
            float _2210;
            float _2211;
            float _2212;
            float _2213;
            if (_48_m0[176u].w > 0.0f)
            {
                float _2064 = 1.0f - _563;
                float _2067 = exp2(log2(_2064) * _48_m0[176u].w);
                float _2069 = clamp(1.0f - _2067, 0.0f, 1.0f);
                float _2073 = (sqrt(_2069 + 9.9999997473787516355514526367188e-05f) + _2067) * _2069;
                float _2080 = (_2073 * (_1543 - _533)) + _533;
                float _2081 = (_2073 * (_1544 - _535)) + _535;
                float _2082 = (_2073 * (_1545 - _537)) + _537;
                float _2086 = rsqrt(dot(float3(_2080, _2081, _2082), float3(_2080, _2081, _2082)));
                _2210 = _2064;
                _2211 = _2080 * _2086;
                _2212 = _2081 * _2086;
                _2213 = _2082 * _2086;
            }
            else
            {
                _2210 = 1.0f - _563;
                _2211 = _1543;
                _2212 = _1544;
                _2213 = _1545;
            }
            float _2321;
            if (_845)
            {
                _2321 = _1763 * _2210;
            }
            else
            {
                _2321 = max((_1763 + (-9.0f)) + ((_53_m0[107u].x * 9.0f) * exp2(log2(clamp(_2210, 0.0f, 1.0f)) * _53_m0[107u].z)), 0.0f);
            }
            float4 _2326 = _31[4u].SampleLevel(_58, float3(_2211, _2212, _2213), _2321);
            float _2478;
            float _2480;
            float _2482;
            float _2484;
            if (_1832)
            {
                _2478 = 0.0f;
                _2480 = 0.0f;
                _2482 = 0.0f;
                _2484 = 0.0f;
            }
            else
            {
                float _2497 = min(_886, _48_m0[110u].z);
                float _2498 = min(_887, _48_m0[110u].w);
                float4 _2514 = _15[509u].SampleLevel(_56, float2(_2497, _2498), _15[557u].SampleLevel(_56, float2(_2497, _2498), 0.0f).x * _48_m0[72u].w);
                float _2516 = _2514.x;
                float _2517 = _2514.y;
                float _2518 = _2514.z;
                float _2535 = (1.0f / max(_48_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_2516, max(_2517, _2518))));
                _2478 = _2535 * _2516;
                _2480 = _2535 * _2517;
                _2482 = _2535 * _2518;
                _2484 = _2514.w;
            }
            float _2486 = 1.0f - _2484;
            float _2490 = (_2486 * _2326.x) + _2478;
            float _2491 = (_2486 * _2326.y) + _2480;
            float _2492 = (_2486 * _2326.z) + _2482;
            float _2653;
            float _2655;
            float _2657;
            if (_1865)
            {
                _2653 = _2490;
                _2655 = _2491;
                _2657 = _2492;
            }
            else
            {
                _2653 = min(1.0f, _1491) * _2490;
                _2655 = min(1.0f, _1493) * _2491;
                _2657 = min(1.0f, _1495) * _2492;
            }
            float _2672 = (((min(_563 * 0.4749999940395355224609375f, _2008) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _563) + (-0.015625f);
            float _2673 = ((_563 * 0.25f) + 0.75f) - _2672;
            float _2697 = (_1999 * (_567 + (-1.0f))) + 1.0f;
            _2255 = ((((((_48_m0[59u].w * _2653) * clamp((_2673 * _548) + _2672, 0.0f, 1.0f)) - _2027) * _565) + _2027) * _2697) * _1296;
            _2257 = ((((((_48_m0[59u].w * _2655) * clamp((_2673 * _551) + _2672, 0.0f, 1.0f)) - _2028) * _565) + _2028) * _2697) * _1296;
            _2259 = ((((((_48_m0[59u].w * _2657) * clamp((_2673 * _553) + _2672, 0.0f, 1.0f)) - _2029) * _565) + _2029) * _2697) * _1296;
        }
        else
        {
            uint _1615;
            float _1616;
            float _1618;
            float _1620;
            if (_1486)
            {
                _1615 = 0u;
                _1616 = 0.0f;
                _1618 = 0.0f;
                _1620 = 0.0f;
            }
            else
            {
                float _1638 = _1309 / _48_m0[59u].w;
                float _1639 = _1310 / _48_m0[59u].w;
                float _1640 = _1311 / _48_m0[59u].w;
                float _1647 = dot(float3(_1638, _1639, _1640), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                float _1668 = dot(float3(_31[NonUniformResourceIndex((_403 + 52u) + 0u)].Sample(_60, float3(_524, _527, _530)).xyz), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                float _1674 = (_53_m0[1164u].x * _908) * _48_m0[169u].w;
                _1615 = 1u;
                _1616 = (_1674 * (((((_1638 - _1647) * _48_m0[156u].w) + _1647) / _1668) + (-1.0f))) + 1.0f;
                _1618 = ((((((_1639 - _1647) * _48_m0[156u].w) + _1647) / _1668) + (-1.0f)) * _1674) + 1.0f;
                _1620 = ((((((_1640 - _1647) * _48_m0[156u].w) + _1647) / _1668) + (-1.0f)) * _1674) + 1.0f;
            }
            uint _1711;
            float _1712;
            float _1714;
            float _1716;
            float _1717;
            float _1719;
            float _1721;
            float _1722;
            float _1724;
            float _1726;
            float _1728;
            if (((_411 & 4u) == 0u) || (asuint(_53_m0[157u]).y == 0u))
            {
                _1711 = 0u;
                _1712 = 0.0f;
                _1714 = 0.0f;
                _1716 = 1.0f;
                _1717 = 1.0f;
                _1719 = 1.0f;
                _1721 = _1296;
                _1722 = 0.0f;
                _1724 = 0.0f;
                _1726 = 0.0f;
                _1728 = 0.0f;
            }
            else
            {
                float4 _1744 = _15[2u].SampleLevel(_56, float2(_685, _686), 0.0f);
                _1711 = 1u;
                _1712 = _53_m0[157u].z;
                _1714 = _53_m0[157u].w;
                _1716 = _1296;
                _1717 = _53_m0[158u].x;
                _1719 = _53_m0[158u].y;
                _1721 = 1.0f;
                _1722 = _48_m0[59u].w * _1744.x;
                _1724 = _48_m0[59u].w * _1744.y;
                _1726 = _48_m0[59u].w * _1744.z;
                _1728 = clamp(_1744.w, 0.0f, 1.0f);
            }
            uint _1732 = (_403 + 48u) + 0u;
            float _1733 = _1128 * 2.0f;
            float _1737 = _1125 - (_1733 * _533);
            float _1738 = _1126 - (_1733 * _535);
            float _1739 = _1127 - (_1733 * _537);
            float _2214;
            float _2216;
            float _2218;
            if (_584)
            {
                float _1765 = clamp(_555, 0.0f, 1.0f);
                float _1767 = (_837 >= 0.0f) ? _837 : 8.0f;
                float _1768 = 1.0f - _1765;
                float _1833;
                if (_845)
                {
                    _1833 = _1768 * _1767;
                }
                else
                {
                    _1833 = max((_1767 + (-9.0f)) + ((_53_m0[107u].x * 9.0f) * exp2(log2(clamp(_1768, 0.0f, 1.0f)) * _53_m0[107u].z)), 0.0f);
                }
                float4 _1838 = _31[NonUniformResourceIndex(_1732)].SampleLevel(_58, float3(_1737, _1738, _1739), _1833);
                float _1915;
                float _1917;
                float _1919;
                float _1921;
                if (_888 == 0u)
                {
                    _1915 = 0.0f;
                    _1917 = 0.0f;
                    _1919 = 0.0f;
                    _1921 = 0.0f;
                }
                else
                {
                    float _1935 = min(_886, _48_m0[110u].z);
                    float _1936 = min(_887, _48_m0[110u].w);
                    float4 _1952 = _15[509u].SampleLevel(_56, float2(_1935, _1936), _15[557u].SampleLevel(_56, float2(_1935, _1936), 0.0f).x * _48_m0[72u].w);
                    float _1954 = _1952.x;
                    float _1955 = _1952.y;
                    float _1956 = _1952.z;
                    float _1973 = (1.0f / max(_48_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_1954, max(_1955, _1956))));
                    _1915 = _1973 * _1954;
                    _1917 = _1973 * _1955;
                    _1919 = _1973 * _1956;
                    _1921 = _1952.w;
                }
                float _1923 = 1.0f - _1921;
                float _1927 = (_1923 * _1838.x) + _1915;
                float _1928 = (_1923 * _1838.y) + _1917;
                float _1929 = (_1923 * _1838.z) + _1919;
                float _2037;
                float _2039;
                float _2041;
                if (_1615 == 0u)
                {
                    _2037 = _1927;
                    _2039 = _1928;
                    _2041 = _1929;
                }
                else
                {
                    _2037 = min(1.0f, _1616) * _1927;
                    _2039 = min(1.0f, _1618) * _1928;
                    _2041 = min(1.0f, _1620) * _1929;
                }
                float _2046 = _48_m0[59u].w * _2037;
                float _2047 = _48_m0[59u].w * _2039;
                float _2048 = _48_m0[59u].w * _2041;
                float _2091;
                float _2093;
                float _2095;
                if (_1711 == 0u)
                {
                    _2091 = _2046;
                    _2093 = _2047;
                    _2095 = _2048;
                }
                else
                {
                    float _2129 = clamp((_1765 - _1712) / (_1714 - _1712), 0.0f, 1.0f);
                    float _2133 = (_2129 * _2129) * (3.0f - (_2129 * 2.0f));
                    float _2138 = 1.0f - (_2133 * _1728);
                    float _2150 = (((_2133 * (_1719 - _1717)) + _1717) * (_1716 + (-1.0f))) + 1.0f;
                    _2091 = ((_2138 * _2046) + (_2133 * _1722)) * _2150;
                    _2093 = ((_2138 * _2047) + (_2133 * _1724)) * _2150;
                    _2095 = ((_2138 * _2048) + (_2133 * _1726)) * _2150;
                }
                float _2116 = (max((asuint(_48_m0[85u]).z == 0u) ? (_1765 * 0.75f) : _1765, _548) - _548) * exp2(log2(1.0f - clamp(dot(float3(_533, _535, _537), float3(_853, _854, _855)), 0.0f, 1.0f)) * 5.0f);
                _2214 = (_2091 * _1320) * (_2116 + _548);
                _2216 = (_2093 * _1320) * (_2116 + _548);
                _2218 = (_2095 * _1320) * (_2116 + _548);
            }
            else
            {
                float _1844;
                float _1845;
                float _1846;
                float _1847;
                if (_48_m0[176u].w > 0.0f)
                {
                    float _1795 = 1.0f - _555;
                    float _1798 = exp2(log2(_1795) * _48_m0[176u].w);
                    float _1800 = clamp(1.0f - _1798, 0.0f, 1.0f);
                    float _1804 = (sqrt(_1800 + 9.9999997473787516355514526367188e-05f) + _1798) * _1800;
                    float _1811 = (_1804 * (_1737 - _533)) + _533;
                    float _1812 = (_1804 * (_1738 - _535)) + _535;
                    float _1813 = (_1804 * (_1739 - _537)) + _537;
                    float _1817 = rsqrt(dot(float3(_1811, _1812, _1813), float3(_1811, _1812, _1813)));
                    _1844 = _1795;
                    _1845 = _1811 * _1817;
                    _1846 = _1812 * _1817;
                    _1847 = _1813 * _1817;
                }
                else
                {
                    _1844 = 1.0f - _555;
                    _1845 = _1737;
                    _1846 = _1738;
                    _1847 = _1739;
                }
                float _1849 = (_837 >= 0.0f) ? _837 : 8.0f;
                float _2053;
                if (_845)
                {
                    _2053 = _1849 * _1844;
                }
                else
                {
                    _2053 = max((_1849 + (-9.0f)) + ((_53_m0[107u].x * 9.0f) * exp2(log2(clamp(_1844, 0.0f, 1.0f)) * _53_m0[107u].z)), 0.0f);
                }
                float4 _2058 = _31[NonUniformResourceIndex(_1732)].SampleLevel(_58, float3(_1845, _1846, _1847), _2053);
                float _2151;
                float _2153;
                float _2155;
                float _2157;
                if (_888 == 0u)
                {
                    _2151 = 0.0f;
                    _2153 = 0.0f;
                    _2155 = 0.0f;
                    _2157 = 0.0f;
                }
                else
                {
                    float _2171 = min(_886, _48_m0[110u].z);
                    float _2172 = min(_887, _48_m0[110u].w);
                    float4 _2188 = _15[509u].SampleLevel(_56, float2(_2171, _2172), _15[557u].SampleLevel(_56, float2(_2171, _2172), 0.0f).x * _48_m0[72u].w);
                    float _2190 = _2188.x;
                    float _2191 = _2188.y;
                    float _2192 = _2188.z;
                    float _2209 = (1.0f / max(_48_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_2190, max(_2191, _2192))));
                    _2151 = _2209 * _2190;
                    _2153 = _2209 * _2191;
                    _2155 = _2209 * _2192;
                    _2157 = _2188.w;
                }
                float _2159 = 1.0f - _2157;
                float _2163 = (_2159 * _2058.x) + _2151;
                float _2164 = (_2159 * _2058.y) + _2153;
                float _2165 = (_2159 * _2058.z) + _2155;
                float _2229;
                float _2231;
                float _2233;
                if (_1615 == 0u)
                {
                    _2229 = _2163;
                    _2231 = _2164;
                    _2233 = _2165;
                }
                else
                {
                    _2229 = min(1.0f, _1616) * _2163;
                    _2231 = min(1.0f, _1618) * _2164;
                    _2233 = min(1.0f, _1620) * _2165;
                }
                float _2238 = _48_m0[59u].w * _2229;
                float _2239 = _48_m0[59u].w * _2231;
                float _2240 = _48_m0[59u].w * _2233;
                float _2263;
                float _2265;
                float _2267;
                if (_1711 == 0u)
                {
                    _2263 = _2238;
                    _2265 = _2239;
                    _2267 = _2240;
                }
                else
                {
                    float _2299 = clamp((_555 - _1712) / (_1714 - _1712), 0.0f, 1.0f);
                    float _2303 = (_2299 * _2299) * (3.0f - (_2299 * 2.0f));
                    float _2308 = 1.0f - (_2303 * _1728);
                    float _2320 = (((_2303 * (_1719 - _1717)) + _1717) * (_1716 + (-1.0f))) + 1.0f;
                    _2263 = ((_2308 * _2238) + (_2303 * _1722)) * _2320;
                    _2265 = ((_2308 * _2239) + (_2303 * _1724)) * _2320;
                    _2267 = ((_2308 * _2240) + (_2303 * _1726)) * _2320;
                }
                float _2282 = (((min(_555 * 0.4749999940395355224609375f, exp2(clamp(dot(float3(_533, _535, _537), float3(_853, _854, _855)), 0.0f, 1.0f) * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _555) + (-0.015625f);
                float _2283 = ((_555 * 0.25f) + 0.75f) - _2282;
                _2214 = (_2263 * _1320) * clamp((_2283 * _548) + _2282, 0.0f, 1.0f);
                _2216 = (_2265 * _1320) * clamp((_2283 * _551) + _2282, 0.0f, 1.0f);
                _2218 = (_2267 * _1320) * clamp((_2283 * _553) + _2282, 0.0f, 1.0f);
            }
            _2255 = (_2214 * _1721) + (_1465 * _1298);
            _2257 = (_2216 * _1721) + (_1467 * _1298);
            _2259 = (_2218 * _1721) + (_1469 * _1298);
        }
        float _2331;
        float _2333;
        float _2335;
        float _2337;
        float _2339;
        float _2341;
        if ((_411 & 2u) == 0u)
        {
            _2331 = 0.0f;
            _2333 = 0.0f;
            _2335 = 0.0f;
            _2337 = 0.0f;
            _2339 = 0.0f;
            _2341 = 0.0f;
        }
        else
        {
            float _2416 = _580 ? 1.0f : ((_722 * (_53_m0[1162u].y - _53_m0[1162u].x)) + _53_m0[1162u].x);
            float _2417 = _580 ? 1.0f : _53_m0[_693 + 1156u].w;
            bool _2420 = ((_386 & 2097152u) == 0u) && _581;
            float _2448 = clamp((_692 - _48_m0[109u].y) / (_48_m0[109u].z - _48_m0[109u].y), 0.0f, 1.0f);
            float _2464 = ((((clamp(exp2(log2(_1101) * _48_m0[162u].z), 0.0f, 1.0f) + (-1.0f)) * _48_m0[98u].y) + 1.0f) * _1318) * (((((_2448 * _2448) * _48_m0[109u].w) * (3.0f - (_2448 * 2.0f))) * (clamp(exp2(log2(_941) * _48_m0[109u].x), 0.0f, 1.0f) + (-1.0f))) + 1.0f);
            float _2465 = 1.0f - _548;
            float _2466 = 1.0f - _551;
            float _2467 = 1.0f - _553;
            float _2471 = clamp(dot(float3(_524, _527, _530), float3(_53_m0[0u].xyz)), 0.0f, 1.0f);
            float _2472 = clamp(_2471, 0.0f, 1.0f);
            float _2638;
            if (_48_m0[76u].w > 0.5f)
            {
                float _2601 = _853 + _53_m0[0u].x;
                float _2602 = _854 + _53_m0[0u].y;
                float _2603 = _855 + _53_m0[0u].z;
                float _2607 = rsqrt(dot(float3(_2601, _2602, _2603), float3(_2601, _2602, _2603)));
                float _2614 = clamp(dot(float3(_53_m0[0u].xyz), float3(_2607 * _2601, _2607 * _2602, _2607 * _2603)), 0.0f, 1.0f);
                float _2624 = ((((_2614 * _2614) * 2.0f) + 0.5f) * (1.0f - _555)) + (-1.0f);
                _2638 = ((exp2(log2(1.0f - clamp(dot(float3(_524, _527, _530), float3(_853, _854, _855)), 0.0f, 1.0f)) * 5.0f) * _2624) + 1.0f) * ((_2624 * exp2(log2(1.0f - _2471) * 5.0f)) + 1.0f);
            }
            else
            {
                _2638 = 1.0f;
            }
            bool _2652 = _2471 > 0.0f;
            float _2765;
            float _2767;
            float _2769;
            if (_2652)
            {
                float _2745 = max(exp2(log2(clamp(1.0f - _555, 0.0f, 1.0f)) * _53_m0[107u].y), _832);
                float _2746 = _853 + _53_m0[0u].x;
                float _2747 = _854 + _53_m0[0u].y;
                float _2748 = _855 + _53_m0[0u].z;
                float _2752 = rsqrt(dot(float3(_2746, _2747, _2748), float3(_2746, _2747, _2748)));
                float _2753 = _2752 * _2746;
                float _2754 = _2752 * _2747;
                float _2755 = _2752 * _2748;
                float _2759 = clamp(dot(float3(_533, _535, _537), float3(_53_m0[0u].xyz)), 0.0f, 1.0f);
                float _2763 = clamp(dot(float3(_533, _535, _537), float3(_2753, _2754, _2755)), 0.0f, 1.0f);
                float _2891;
                float _2892;
                float _2893;
                if (_2759 > 0.0f)
                {
                    float _2857 = _2745 * _2745;
                    float _2858 = _2857 * _2857;
                    float _2862 = (((_2763 * _2858) - _2763) * _2763) + 1.0f;
                    float _2867 = _2857 * 0.5f;
                    float _2868 = 1.0f - _2867;
                    float _2875 = 1.0f - clamp(dot(float3(_853, _854, _855), float3(_2753, _2754, _2755)), 0.0f, 1.0f);
                    float _2876 = _2875 * _2875;
                    float _2878 = (_2876 * _2876) * _2875;
                    float _2887 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_533, _535, _537), float3(_853, _854, _855)), 0.0f, 1.0f)) * _2868) + _2867) * ((_2759 * _2868) + _2867))) * (_2858 / ((_2862 * _2862) * 3.1415927410125732421875f)), _833) * _2416;
                    _2891 = _2887 * ((_2878 * _2465) + _548);
                    _2892 = _2887 * ((_2878 * _2466) + _551);
                    _2893 = _2887 * ((_2878 * _2467) + _553);
                }
                else
                {
                    _2891 = 0.0f;
                    _2892 = 0.0f;
                    _2893 = 0.0f;
                }
                float _2894 = min(_2891, 100000.0f);
                float _2896 = min(_2892, 100000.0f);
                float _2897 = min(_2893, 100000.0f);
                float _3061;
                float _3063;
                float _3065;
                if (_2420)
                {
                    _3061 = _2894;
                    _3063 = _2896;
                    _3065 = _2897;
                }
                else
                {
                    float _3067 = 1.0f - _2471;
                    float _3068 = _3067 * _3067;
                    float _3070 = 1.0f - (_3068 * _3068);
                    _3061 = _2894 * _3070;
                    _3063 = _2896 * _3070;
                    _3065 = _2897 * _3070;
                }
                _2765 = _3061 * _2471;
                _2767 = _3063 * _2471;
                _2769 = _3065 * _2471;
            }
            else
            {
                _2765 = 0.0f;
                _2767 = 0.0f;
                _2769 = 0.0f;
            }
            float _2923;
            float _2925;
            float _2927;
            if (_2652)
            {
                float _2903 = max(exp2(log2(clamp(1.0f - _563, 0.0f, 1.0f)) * _53_m0[107u].y), _832);
                float _2904 = _853 + _53_m0[0u].x;
                float _2905 = _854 + _53_m0[0u].y;
                float _2906 = _855 + _53_m0[0u].z;
                float _2910 = rsqrt(dot(float3(_2904, _2905, _2906), float3(_2904, _2905, _2906)));
                float _2911 = _2910 * _2904;
                float _2912 = _2910 * _2905;
                float _2913 = _2910 * _2906;
                float _2917 = clamp(dot(float3(_533, _535, _537), float3(_53_m0[0u].xyz)), 0.0f, 1.0f);
                float _2921 = clamp(dot(float3(_533, _535, _537), float3(_2911, _2912, _2913)), 0.0f, 1.0f);
                float _3113;
                float _3114;
                float _3115;
                if (_2917 > 0.0f)
                {
                    float _3080 = _2903 * _2903;
                    float _3081 = _3080 * _3080;
                    float _3085 = (((_2921 * _3081) - _2921) * _2921) + 1.0f;
                    float _3089 = _3080 * 0.5f;
                    float _3090 = 1.0f - _3089;
                    float _3097 = 1.0f - clamp(dot(float3(_853, _854, _855), float3(_2911, _2912, _2913)), 0.0f, 1.0f);
                    float _3098 = _3097 * _3097;
                    float _3100 = (_3098 * _3098) * _3097;
                    float _3109 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_533, _535, _537), float3(_853, _854, _855)), 0.0f, 1.0f)) * _3090) + _3089) * ((_2917 * _3090) + _3089))) * (_3081 / ((_3085 * _3085) * 3.1415927410125732421875f)), _833) * _2416;
                    _3113 = _3109 * ((_3100 * _2465) + _548);
                    _3114 = _3109 * ((_3100 * _2466) + _551);
                    _3115 = _3109 * ((_3100 * _2467) + _553);
                }
                else
                {
                    _3113 = 0.0f;
                    _3114 = 0.0f;
                    _3115 = 0.0f;
                }
                float _3116 = min(_3113, 100000.0f);
                float _3117 = min(_3114, 100000.0f);
                float _3118 = min(_3115, 100000.0f);
                float _3129;
                float _3131;
                float _3133;
                if (_2420)
                {
                    _3129 = _3116;
                    _3131 = _3117;
                    _3133 = _3118;
                }
                else
                {
                    float _3135 = 1.0f - _2471;
                    float _3136 = _3135 * _3135;
                    float _3138 = 1.0f - (_3136 * _3136);
                    _3129 = _3116 * _3138;
                    _3131 = _3117 * _3138;
                    _3133 = _3118 * _3138;
                }
                _2923 = _3129 * _2471;
                _2925 = _3131 * _2471;
                _2927 = _3133 * _2471;
            }
            else
            {
                _2923 = 0.0f;
                _2925 = 0.0f;
                _2927 = 0.0f;
            }
            float _2942 = log2(max(1.0f - _569, 1.1754943508222875079687365372222e-38f)) * (-6862156513617824209436672.0f);
            float _2944 = _2942 * _2942;
            float _2947 = exp2(_2944 * (-225.4210968017578125f));
            float _2956 = exp2(_2944 * (-29.8077487945556640625f));
            float _2968 = exp2(_2944 * (-7.714946269989013671875f));
            float _2977 = exp2(_2944 * (-2.5444357395172119140625f));
            float _2980 = _2977 * 0.007000000216066837310791015625f;
            float _2987 = exp2(_2944 * (-0.72497236728668212890625f));
            float _3008 = max(dot(float3((-0.0f) - _524, (-0.0f) - _527, (-0.0f) - _530), float3(_53_m0[0u].xyz)) + 0.300000011920928955078125f, 0.0f);
            float _3015 = _2464 * _834;
            float _3016 = _2464 * _835;
            float _3017 = _2464 * _836;
            float _3119;
            if (_580)
            {
                _3119 = 1.0f;
            }
            else
            {
                _3119 = _48_m0[677u].w;
            }
            float _3139;
            float _3142;
            float _3145;
            float _3148;
            float _3151;
            float _3154;
            float _3157;
            float _3159;
            float _3161;
            if (asuint(_53_m0[59u]).w == 0u)
            {
                _3139 = 0.0f;
                _3142 = 0.0f;
                _3145 = 0.0f;
                _3148 = 0.0f;
                _3151 = 0.0f;
                _3154 = 0.0f;
                _3157 = 0.0f;
                _3159 = 0.0f;
                _3161 = 0.0f;
            }
            else
            {
                uint _3213 = uint(int(_53_m0[58u].x * _279));
                uint _3214 = uint(int(_53_m0[58u].y * _281));
                uint _3215 = uint(int(max(_53_m0[60u].y - (_53_m0[60u].x * log2((_53_m0[58u].z * _278) + _53_m0[58u].w)), 0.0f)));
                uint4 _3217 = _34.Load(int4(uint3(_3213, _3214, _3215), 0u));
                uint _3220 = _3217.x;
                uint4 _3224 = asuint(_53_m0[89u]);
                uint _3236 = (((_3214 << (_3224.x & 31u)) + _3213) + (_3215 << (_3224.y & 31u))) << (_3224.z & 31u);
                float frontier_phi_151_152_ladder;
                float frontier_phi_151_152_ladder_1;
                float frontier_phi_151_152_ladder_2;
                float frontier_phi_151_152_ladder_3;
                float frontier_phi_151_152_ladder_4;
                float frontier_phi_151_152_ladder_5;
                float frontier_phi_151_152_ladder_6;
                float frontier_phi_151_152_ladder_7;
                float frontier_phi_151_152_ladder_8;
                if (_3220 == 0u)
                {
                    frontier_phi_151_152_ladder = 0.0f;
                    frontier_phi_151_152_ladder_1 = 0.0f;
                    frontier_phi_151_152_ladder_2 = 0.0f;
                    frontier_phi_151_152_ladder_3 = 0.0f;
                    frontier_phi_151_152_ladder_4 = 0.0f;
                    frontier_phi_151_152_ladder_5 = 0.0f;
                    frontier_phi_151_152_ladder_6 = 0.0f;
                    frontier_phi_151_152_ladder_7 = 0.0f;
                    frontier_phi_151_152_ladder_8 = 0.0f;
                }
                else
                {
                    float _3243 = _352 - _53_m0[59u].x;
                    float _3244 = _353 - _53_m0[59u].y;
                    float _3245 = _354 - _53_m0[59u].z;
                    uint4 _3256 = _35.Load(_3236);
                    uint _3257 = _3256.x;
                    uint _3258 = _3236 + (_3220 & 127u);
                    uint _3259 = _3258 + ((_3220 >> 7u) & 127u);
                    uint _3260 = _3259 + ((_3220 >> 20u) & 63u);
                    uint _3261 = _3260 + ((_3220 >> 14u) & 63u);
                    uint _3262 = _3261 + (_3220 >> 26u);
                    uint _3263 = _3236 + 1u;
                    float _3265;
                    float _3267;
                    float _3269;
                    float _3271;
                    float _3273;
                    float _3275;
                    uint _3277;
                    uint _3279;
                    if (_3263 > _3258)
                    {
                        _3265 = 0.0f;
                        _3267 = 0.0f;
                        _3269 = 0.0f;
                        _3271 = 0.0f;
                        _3273 = 0.0f;
                        _3275 = 0.0f;
                        _3277 = _3263;
                        _3279 = _3257;
                    }
                    else
                    {
                        float _3266;
                        float _3268;
                        float _3270;
                        float _3272;
                        float _3274;
                        float _3276;
                        float _3296 = 0.0f;
                        float _3297 = 0.0f;
                        float _3298 = 0.0f;
                        float _3299 = 0.0f;
                        float _3300 = 0.0f;
                        float _3301 = 0.0f;
                        uint _3302 = _3263;
                        uint _3303 = _3257;
                        uint _3278;
                        uint _3306;
                        uint _3321;
                        uint _3322;
                        float _3346;
                        float _3348;
                        float _3351;
                        float _3353;
                        uint _3355;
                        uint _3356;
                        bool _3359;
                        float _3361;
                        bool _3367;
                        for (;;)
                        {
                            _3278 = _3302 + 1u;
                            _3306 = _35.Load(_3302).x;
                            uint _3308 = _3303 * 4u;
                            uint4 _3320 = uint4(_36.Load(_3308).x, _36.Load(_3308 + 1u).x, _36.Load(_3308 + 2u).x, _36.Load(_3308 + 3u).x);
                            _3321 = _3320.x;
                            _3322 = _3320.y;
                            uint _3323 = _3320.z;
                            uint _3324 = _3320.w;
                            _3346 = spvUnpackHalf2x16(_3322 >> 16u).x;
                            _3348 = spvUnpackHalf2x16(_3323).x;
                            _3351 = spvUnpackHalf2x16(_3323 >> 16u).x;
                            _3353 = spvUnpackHalf2x16(_3324).x;
                            _3355 = (_3324 >> 16u) & 7u;
                            _3356 = _3324 & 524288u;
                            _3359 = int(uint4(_3327, _3328, _3329, _37.Load((_3303 * 4u) + 3u).x).w) < int(0u);
                            _3361 = spvUnpackHalf2x16(uint3(_3337, _3338, _38.Load((_3303 * 4u) + 2u).x).z).x;
                            _3367 = (_3324 < 3221225472u) && (((_436 & 255u) & (_3324 >> 22u)) != 0u);
                            float frontier_phi_163_pred;
                            float frontier_phi_163_pred_1;
                            float frontier_phi_163_pred_2;
                            float frontier_phi_163_pred_3;
                            float frontier_phi_163_pred_4;
                            float frontier_phi_163_pred_5;
                            if (_3367)
                            {
                                float _3484 = spvUnpackHalf2x16(_3321).x - _3243;
                                float _3485 = spvUnpackHalf2x16(_3321 >> 16u).x - _3244;
                                float _3486 = spvUnpackHalf2x16(_3322).x - _3245;
                                float _3492 = sqrt(((_3485 * _3485) + (_3486 * _3486)) + (_3484 * _3484));
                                float _3493 = _3492 * _3346;
                                float frontier_phi_163_pred_162_ladder;
                                float frontier_phi_163_pred_162_ladder_1;
                                float frontier_phi_163_pred_162_ladder_2;
                                float frontier_phi_163_pred_162_ladder_3;
                                float frontier_phi_163_pred_162_ladder_4;
                                float frontier_phi_163_pred_162_ladder_5;
                                if (_3493 < 1.0f)
                                {
                                    float _3624 = rsqrt(dot(float3(_3484, _3485, _3486), float3(_3484, _3485, _3486)));
                                    float _3625 = _3624 * _3484;
                                    float _3626 = _3624 * _3485;
                                    float _3627 = _3624 * _3486;
                                    float _3628 = _3492 * _3492;
                                    float _3630 = (_3346 * _3346) * _3628;
                                    float _3633 = clamp(1.0f - (_3630 * _3630), 0.0f, 1.0f);
                                    float _3883;
                                    if (_3355 == 0u)
                                    {
                                        _3883 = (_3633 * _3633) * (1.0f / (max(_3628, 9.9999997473787516355514526367188e-05f) + ((_3361 * _3361) * 0.5f)));
                                    }
                                    else
                                    {
                                        _3883 = max((1.0f / dot(float3(1.0f, _3493, _3493 * _3493), float3(_53_m0[_3355 + 60u].xyz))) * (1.0f - _3493), 0.0f);
                                    }
                                    float _3888 = max(float(_3359) * 16.0f, 1.0f) * _3883;
                                    float _3889 = _3888 * _3348;
                                    float _3890 = _3888 * _3351;
                                    float _3891 = _3888 * _3353;
                                    float _4230;
                                    float _4232;
                                    float _4234;
                                    float _4236;
                                    float _4238;
                                    float _4240;
                                    if (_3356 == 0u)
                                    {
                                        float _4052 = clamp(clamp(dot(float3(_524, _527, _530), float3(_3625, _3626, _3627)), 0.0f, 1.0f), 0.0f, 1.0f);
                                        _4230 = _4052 * ((_539 * 0.3183098733425140380859375f) * (1.0f - _548));
                                        _4232 = _4052 * ((_542 * 0.3183098733425140380859375f) * (1.0f - _551));
                                        _4234 = _4052 * ((_545 * 0.3183098733425140380859375f) * (1.0f - _553));
                                        _4236 = 0.0f;
                                        _4238 = 0.0f;
                                        _4240 = 0.0f;
                                    }
                                    else
                                    {
                                        float _4062 = 1.0f - _548;
                                        float _4063 = 1.0f - _551;
                                        float _4064 = 1.0f - _553;
                                        float _4068 = clamp(dot(float3(_524, _527, _530), float3(_3625, _3626, _3627)), 0.0f, 1.0f);
                                        float _4069 = clamp(_4068, 0.0f, 1.0f);
                                        float _4282;
                                        if (_48_m0[76u].w > 0.5f)
                                        {
                                            float _4245 = _3625 + _853;
                                            float _4246 = _3626 + _854;
                                            float _4247 = _3627 + _855;
                                            float _4251 = rsqrt(dot(float3(_4245, _4246, _4247), float3(_4245, _4246, _4247)));
                                            float _4258 = clamp(dot(float3(_3625, _3626, _3627), float3(_4251 * _4245, _4251 * _4246, _4251 * _4247)), 0.0f, 1.0f);
                                            float _4268 = ((((_4258 * _4258) * 2.0f) + 0.5f) * (1.0f - _555)) + (-1.0f);
                                            _4282 = ((exp2(log2(1.0f - clamp(dot(float3(_524, _527, _530), float3(_853, _854, _855)), 0.0f, 1.0f)) * 5.0f) * _4268) + 1.0f) * ((_4268 * exp2(log2(1.0f - _4068) * 5.0f)) + 1.0f);
                                        }
                                        else
                                        {
                                            _4282 = 1.0f;
                                        }
                                        bool _4292 = _4068 > 0.0f;
                                        float _4407;
                                        float _4409;
                                        float _4411;
                                        if (_4292)
                                        {
                                            float _4387 = max(exp2(log2(clamp(1.0f - _555, 0.0f, 1.0f)) * _53_m0[107u].y), _832);
                                            float _4388 = _3625 + _853;
                                            float _4389 = _3626 + _854;
                                            float _4390 = _3627 + _855;
                                            float _4394 = rsqrt(dot(float3(_4388, _4389, _4390), float3(_4388, _4389, _4390)));
                                            float _4395 = _4394 * _4388;
                                            float _4396 = _4394 * _4389;
                                            float _4397 = _4394 * _4390;
                                            float _4401 = clamp(dot(float3(_533, _535, _537), float3(_3625, _3626, _3627)), 0.0f, 1.0f);
                                            float _4405 = clamp(dot(float3(_533, _535, _537), float3(_4395, _4396, _4397)), 0.0f, 1.0f);
                                            float _4504;
                                            float _4505;
                                            float _4506;
                                            if (_4401 > 0.0f)
                                            {
                                                float _4471 = _4387 * _4387;
                                                float _4472 = _4471 * _4471;
                                                float _4476 = (((_4405 * _4472) - _4405) * _4405) + 1.0f;
                                                float _4480 = _4471 * 0.5f;
                                                float _4481 = 1.0f - _4480;
                                                float _4488 = 1.0f - clamp(dot(float3(_853, _854, _855), float3(_4395, _4396, _4397)), 0.0f, 1.0f);
                                                float _4489 = _4488 * _4488;
                                                float _4491 = (_4489 * _4489) * _4488;
                                                float _4500 = min((0.25f / (((_4401 * _4481) + _4480) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_533, _535, _537), float3(_853, _854, _855)), 0.0f, 1.0f)) * _4481) + _4480))) * (_4472 / ((_4476 * _4476) * 3.1415927410125732421875f)), _833) * _2416;
                                                _4504 = _4500 * ((_4491 * _4062) + _548);
                                                _4505 = _4500 * ((_4491 * _4063) + _551);
                                                _4506 = _4500 * ((_4491 * _4064) + _553);
                                            }
                                            else
                                            {
                                                _4504 = 0.0f;
                                                _4505 = 0.0f;
                                                _4506 = 0.0f;
                                            }
                                            float _4507 = min(_4504, 100000.0f);
                                            float _4508 = min(_4505, 100000.0f);
                                            float _4509 = min(_4506, 100000.0f);
                                            float _4681;
                                            float _4683;
                                            float _4685;
                                            if (_2420)
                                            {
                                                _4681 = _4507;
                                                _4683 = _4508;
                                                _4685 = _4509;
                                            }
                                            else
                                            {
                                                float _4687 = 1.0f - _4068;
                                                float _4688 = _4687 * _4687;
                                                float _4690 = 1.0f - (_4688 * _4688);
                                                _4681 = _4507 * _4690;
                                                _4683 = _4508 * _4690;
                                                _4685 = _4509 * _4690;
                                            }
                                            _4407 = _4681 * _4068;
                                            _4409 = _4683 * _4068;
                                            _4411 = _4685 * _4068;
                                        }
                                        else
                                        {
                                            _4407 = 0.0f;
                                            _4409 = 0.0f;
                                            _4411 = 0.0f;
                                        }
                                        float _4535;
                                        float _4537;
                                        float _4539;
                                        if (_4292)
                                        {
                                            float _4515 = max(exp2(log2(clamp(1.0f - _563, 0.0f, 1.0f)) * _53_m0[107u].y), _832);
                                            float _4516 = _3625 + _853;
                                            float _4517 = _3626 + _854;
                                            float _4518 = _3627 + _855;
                                            float _4522 = rsqrt(dot(float3(_4516, _4517, _4518), float3(_4516, _4517, _4518)));
                                            float _4523 = _4522 * _4516;
                                            float _4524 = _4522 * _4517;
                                            float _4525 = _4522 * _4518;
                                            float _4529 = clamp(dot(float3(_533, _535, _537), float3(_3625, _3626, _3627)), 0.0f, 1.0f);
                                            float _4533 = clamp(dot(float3(_533, _535, _537), float3(_4523, _4524, _4525)), 0.0f, 1.0f);
                                            float _4733;
                                            float _4734;
                                            float _4735;
                                            if (_4529 > 0.0f)
                                            {
                                                float _4700 = _4515 * _4515;
                                                float _4701 = _4700 * _4700;
                                                float _4705 = (((_4533 * _4701) - _4533) * _4533) + 1.0f;
                                                float _4709 = _4700 * 0.5f;
                                                float _4710 = 1.0f - _4709;
                                                float _4717 = 1.0f - clamp(dot(float3(_853, _854, _855), float3(_4523, _4524, _4525)), 0.0f, 1.0f);
                                                float _4718 = _4717 * _4717;
                                                float _4720 = (_4718 * _4718) * _4717;
                                                float _4729 = min((0.25f / (((_4529 * _4710) + _4709) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_533, _535, _537), float3(_853, _854, _855)), 0.0f, 1.0f)) * _4710) + _4709))) * (_4701 / ((_4705 * _4705) * 3.1415927410125732421875f)), _833) * _2416;
                                                _4733 = _4729 * ((_4720 * _4062) + _548);
                                                _4734 = _4729 * ((_4720 * _4063) + _551);
                                                _4735 = _4729 * ((_4720 * _4064) + _553);
                                            }
                                            else
                                            {
                                                _4733 = 0.0f;
                                                _4734 = 0.0f;
                                                _4735 = 0.0f;
                                            }
                                            float _4736 = min(_4733, 100000.0f);
                                            float _4737 = min(_4734, 100000.0f);
                                            float _4738 = min(_4735, 100000.0f);
                                            float _4797;
                                            float _4799;
                                            float _4801;
                                            if (_2420)
                                            {
                                                _4797 = _4736;
                                                _4799 = _4737;
                                                _4801 = _4738;
                                            }
                                            else
                                            {
                                                float _4803 = 1.0f - _4068;
                                                float _4804 = _4803 * _4803;
                                                float _4806 = 1.0f - (_4804 * _4804);
                                                _4797 = _4736 * _4806;
                                                _4799 = _4737 * _4806;
                                                _4801 = _4738 * _4806;
                                            }
                                            _4535 = _4797 * _4068;
                                            _4537 = _4799 * _4068;
                                            _4539 = _4801 * _4068;
                                        }
                                        else
                                        {
                                            _4535 = 0.0f;
                                            _4537 = 0.0f;
                                            _4539 = 0.0f;
                                        }
                                        _4230 = (((_539 * 0.3183098733425140380859375f) * _4062) * _4069) * _4282;
                                        _4232 = (((_542 * 0.3183098733425140380859375f) * _4063) * _4069) * _4282;
                                        _4234 = (((_545 * 0.3183098733425140380859375f) * _4064) * _4069) * _4282;
                                        _4236 = (((_4535 - _4407) * _565) + _4407) * _3889;
                                        _4238 = (((_4537 - _4409) * _565) + _4409) * _3890;
                                        _4240 = (((_4539 - _4411) * _565) + _4411) * _3891;
                                    }
                                    precise float _4242 = _4234 * _3891;
                                    precise float _4243 = _4232 * _3890;
                                    precise float _4244 = _4230 * _3889;
                                    frontier_phi_163_pred_162_ladder = _4242 + _3298;
                                    frontier_phi_163_pred_162_ladder_1 = _4244 + _3296;
                                    frontier_phi_163_pred_162_ladder_2 = _4243 + _3297;
                                    frontier_phi_163_pred_162_ladder_3 = _4236 + _3299;
                                    frontier_phi_163_pred_162_ladder_4 = _4238 + _3300;
                                    frontier_phi_163_pred_162_ladder_5 = _4240 + _3301;
                                }
                                else
                                {
                                    frontier_phi_163_pred_162_ladder = _3298;
                                    frontier_phi_163_pred_162_ladder_1 = _3296;
                                    frontier_phi_163_pred_162_ladder_2 = _3297;
                                    frontier_phi_163_pred_162_ladder_3 = _3299;
                                    frontier_phi_163_pred_162_ladder_4 = _3300;
                                    frontier_phi_163_pred_162_ladder_5 = _3301;
                                }
                                frontier_phi_163_pred = frontier_phi_163_pred_162_ladder;
                                frontier_phi_163_pred_1 = frontier_phi_163_pred_162_ladder_1;
                                frontier_phi_163_pred_2 = frontier_phi_163_pred_162_ladder_2;
                                frontier_phi_163_pred_3 = frontier_phi_163_pred_162_ladder_3;
                                frontier_phi_163_pred_4 = frontier_phi_163_pred_162_ladder_4;
                                frontier_phi_163_pred_5 = frontier_phi_163_pred_162_ladder_5;
                            }
                            else
                            {
                                frontier_phi_163_pred = _3298;
                                frontier_phi_163_pred_1 = _3296;
                                frontier_phi_163_pred_2 = _3297;
                                frontier_phi_163_pred_3 = _3299;
                                frontier_phi_163_pred_4 = _3300;
                                frontier_phi_163_pred_5 = _3301;
                            }
                            _3270 = frontier_phi_163_pred;
                            _3266 = frontier_phi_163_pred_1;
                            _3268 = frontier_phi_163_pred_2;
                            _3272 = frontier_phi_163_pred_3;
                            _3274 = frontier_phi_163_pred_4;
                            _3276 = frontier_phi_163_pred_5;
                            if (_3278 > _3258)
                            {
                                break;
                            }
                            else
                            {
                                _3296 = _3266;
                                _3297 = _3268;
                                _3298 = _3270;
                                _3299 = _3272;
                                _3300 = _3274;
                                _3301 = _3276;
                                _3302 = _3278;
                                _3303 = _3306;
                                continue;
                            }
                        }
                        _3265 = _3266;
                        _3267 = _3268;
                        _3269 = _3270;
                        _3271 = _3272;
                        _3273 = _3274;
                        _3275 = _3276;
                        _3277 = _3278;
                        _3279 = _3306;
                    }
                    float _3149;
                    float _3152;
                    float _3155;
                    float _3282;
                    float _3284;
                    float _3286;
                    uint _3291;
                    uint _3293;
                    if (_3277 > _3259)
                    {
                        _3282 = _3265;
                        _3284 = _3267;
                        _3286 = _3269;
                        _3149 = _3271;
                        _3152 = _3273;
                        _3155 = _3275;
                        _3291 = _3277;
                        _3293 = _3279;
                    }
                    else
                    {
                        float _3283;
                        float _3285;
                        float _3287;
                        float _3288;
                        float _3289;
                        float _3290;
                        float _3376 = _3265;
                        float _3377 = _3267;
                        float _3378 = _3269;
                        float _3379 = _3271;
                        float _3380 = _3273;
                        float _3381 = _3275;
                        uint _3382 = _3277;
                        uint _3383 = _3279;
                        uint _3292;
                        uint _3386;
                        uint _3401;
                        uint _3402;
                        float _3434;
                        float _3436;
                        float _3439;
                        float _3441;
                        uint _3443;
                        bool _3446;
                        float _3448;
                        float _3451;
                        float _3453;
                        float _3456;
                        float _3458;
                        float _3461;
                        float _3463;
                        float _3465;
                        uint _3466;
                        uint _3469;
                        uint _3470;
                        bool _3476;
                        for (;;)
                        {
                            _3292 = _3382 + 1u;
                            _3386 = _35.Load(_3382).x;
                            uint _3388 = _3383 * 4u;
                            uint4 _3400 = uint4(_36.Load(_3388).x, _36.Load(_3388 + 1u).x, _36.Load(_3388 + 2u).x, _36.Load(_3388 + 3u).x);
                            _3401 = _3400.x;
                            _3402 = _3400.y;
                            uint _3403 = _3400.z;
                            uint _3404 = _3400.w;
                            uint _3406 = _3383 * 4u;
                            uint4 _3418 = uint4(_37.Load(_3406).x, _37.Load(_3406 + 1u).x, _37.Load(_3406 + 2u).x, _37.Load(_3406 + 3u).x);
                            uint _3419 = _3418.x;
                            uint _3420 = _3418.y;
                            uint _3421 = _3418.z;
                            uint _3422 = _3418.w;
                            _3434 = spvUnpackHalf2x16(_3402 >> 16u).x;
                            _3436 = spvUnpackHalf2x16(_3403).x;
                            _3439 = spvUnpackHalf2x16(_3403 >> 16u).x;
                            _3441 = spvUnpackHalf2x16(_3404).x;
                            _3443 = (_3404 >> 16u) & 7u;
                            _3446 = (_3404 & 524288u) != 0u;
                            _3448 = spvUnpackHalf2x16(_3419).x;
                            _3451 = spvUnpackHalf2x16(_3419 >> 16u).x;
                            _3453 = spvUnpackHalf2x16(_3420).x;
                            _3456 = spvUnpackHalf2x16(_3420 >> 16u).x;
                            _3458 = spvUnpackHalf2x16(_3421).x;
                            _3461 = spvUnpackHalf2x16(_3421 >> 16u).x;
                            _3463 = spvUnpackHalf2x16(_3422).x;
                            _3465 = spvUnpackHalf2x16(uint3(_3425, _3426, _38.Load((_3383 * 4u) + 2u).x).z).x;
                            _3466 = _3422 & 8323072u;
                            _3469 = (_3422 >> 23u) & 31u;
                            _3470 = _3422 >> 28u;
                            _3476 = (_3404 < 3221225472u) && (((_436 & 255u) & (_3404 >> 22u)) != 0u);
                            float frontier_phi_168_pred;
                            float frontier_phi_168_pred_1;
                            float frontier_phi_168_pred_2;
                            float frontier_phi_168_pred_3;
                            float frontier_phi_168_pred_4;
                            float frontier_phi_168_pred_5;
                            if (_3476)
                            {
                                float _3603 = spvUnpackHalf2x16(_3401).x - _3243;
                                float _3604 = spvUnpackHalf2x16(_3401 >> 16u).x - _3244;
                                float _3605 = spvUnpackHalf2x16(_3402).x - _3245;
                                float _3611 = sqrt(((_3604 * _3604) + (_3605 * _3605)) + (_3603 * _3603));
                                float _3612 = _3611 * _3434;
                                float frontier_phi_168_pred_167_ladder;
                                float frontier_phi_168_pred_167_ladder_1;
                                float frontier_phi_168_pred_167_ladder_2;
                                float frontier_phi_168_pred_167_ladder_3;
                                float frontier_phi_168_pred_167_ladder_4;
                                float frontier_phi_168_pred_167_ladder_5;
                                if (_3612 < 1.0f)
                                {
                                    float _3714 = rsqrt(dot(float3(_3603, _3604, _3605), float3(_3603, _3604, _3605)));
                                    float _3715 = _3714 * _3603;
                                    float _3716 = _3714 * _3604;
                                    float _3717 = _3714 * _3605;
                                    float _3718 = _3611 * _3611;
                                    float _3720 = (_3434 * _3434) * _3718;
                                    float _3723 = clamp(1.0f - (_3720 * _3720), 0.0f, 1.0f);
                                    float _4015;
                                    if (_3443 == 0u)
                                    {
                                        _4015 = (_3723 * _3723) * (1.0f / (max(_3718, 9.9999997473787516355514526367188e-05f) + ((_3465 * _3465) * 0.5f)));
                                    }
                                    else
                                    {
                                        _4015 = max((1.0f / dot(float3(1.0f, _3612, _3612 * _3612), float3(_53_m0[_3443 + 60u].xyz))) * (1.0f - _3612), 0.0f);
                                    }
                                    float _4016 = (-0.0f) - _3448;
                                    float _4040 = clamp((clamp(dot(float3(((_3456 * _3451) - (_3453 * _4016)) * 2.0f, ((_3453 * _3451) - (_3456 * _3448)) * 2.0f, (((_3448 * _4016) - (_3451 * _3451)) * 2.0f) + 1.0f), float3((-0.0f) - _3715, (-0.0f) - _3716, (-0.0f) - _3717)), 0.0f, 1.0f) - _3461) / (_3458 - _3461), 0.0f, 1.0f);
                                    float _4168;
                                    float _4170;
                                    float _4172;
                                    if ((_3469 | _3466) == 0u)
                                    {
                                        _4168 = _3436;
                                        _4170 = _3439;
                                        _4172 = _3441;
                                    }
                                    else
                                    {
                                        float _4205 = (-0.0f) - _3451;
                                        float _4206 = (-0.0f) - _3453;
                                        float _4219 = ((_3605 * _4205) - (_3604 * _4206)) + (_3603 * _3456);
                                        float _4220 = ((_3603 * _4206) - (_3605 * _4016)) + (_3604 * _3456);
                                        float _4221 = ((_3604 * _4016) - (_3603 * _4205)) + (_3605 * _3456);
                                        float _4228 = (1.0f / ((((_4220 * _4016) - (_4219 * _4205)) * 2.0f) + _3605)) * _3463;
                                        float frontier_phi_202_203_ladder;
                                        float frontier_phi_202_203_ladder_1;
                                        float frontier_phi_202_203_ladder_2;
                                        if (_3469 == 0u)
                                        {
                                            frontier_phi_202_203_ladder = _3439;
                                            frontier_phi_202_203_ladder_1 = _3436;
                                            frontier_phi_202_203_ladder_2 = _3441;
                                        }
                                        else
                                        {
                                            uint _4360 = _3470 + 72u;
                                            float4 _4377 = _41.SampleLevel(_58, float3((_53_m0[_4360].x * ((_4228 * ((((_4221 * _4205) - (_4220 * _4206)) * 2.0f) + _3603)) + 0.5f)) + _53_m0[_4360].z, (_53_m0[_4360].y * (0.5f - (_4228 * ((((_4219 * _4206) - (_4221 * _4016)) * 2.0f) + _3604)))) + _53_m0[_4360].w, float(_3469 + 4294967295u)), 0.0f);
                                            frontier_phi_202_203_ladder = _4377.y * _3439;
                                            frontier_phi_202_203_ladder_1 = _4377.x * _3436;
                                            frontier_phi_202_203_ladder_2 = _4377.z * _3441;
                                        }
                                        _4168 = frontier_phi_202_203_ladder_1;
                                        _4170 = frontier_phi_202_203_ladder;
                                        _4172 = frontier_phi_202_203_ladder_2;
                                    }
                                    float _4184 = (((_4040 * _4040) * _4015) * (3.0f - (_4040 * 2.0f))) * max(float(_3446) * 16.0f, 1.0f);
                                    float _4185 = _4184 * _4168;
                                    float _4186 = _4184 * _4170;
                                    float _4187 = _4184 * _4172;
                                    float _4188 = (_3434 < 0.02857142873108386993408203125f) ? _53_m0[104u].w : 0.0f;
                                    float _4189 = 1.0f - _548;
                                    float _4190 = 1.0f - _551;
                                    float _4191 = 1.0f - _553;
                                    float _4195 = clamp(dot(float3(_524, _527, _530), float3(_3715, _3716, _3717)), 0.0f, 1.0f);
                                    float _4196 = 1.0f - _4188;
                                    float _4200 = clamp(((_4195 * _4196) + _4188) * _4196, 0.0f, 1.0f);
                                    float _4332;
                                    if (_48_m0[76u].w > 0.5f)
                                    {
                                        float _4295 = _3715 + _853;
                                        float _4296 = _3716 + _854;
                                        float _4297 = _3717 + _855;
                                        float _4301 = rsqrt(dot(float3(_4295, _4296, _4297), float3(_4295, _4296, _4297)));
                                        float _4308 = clamp(dot(float3(_3715, _3716, _3717), float3(_4301 * _4295, _4301 * _4296, _4301 * _4297)), 0.0f, 1.0f);
                                        float _4318 = ((((_4308 * _4308) * 2.0f) + 0.5f) * (1.0f - _555)) + (-1.0f);
                                        _4332 = ((exp2(log2(1.0f - clamp(dot(float3(_524, _527, _530), float3(_853, _854, _855)), 0.0f, 1.0f)) * 5.0f) * _4318) + 1.0f) * ((_4318 * exp2(log2(1.0f - _4195) * 5.0f)) + 1.0f);
                                    }
                                    else
                                    {
                                        _4332 = 1.0f;
                                    }
                                    bool _4345 = _4195 > 0.0f;
                                    float _4456;
                                    float _4458;
                                    float _4460;
                                    if (_4345)
                                    {
                                        float _4436 = max(exp2(log2(clamp(1.0f - _555, 0.0f, 1.0f)) * _53_m0[107u].y), _832);
                                        float _4437 = _3715 + _853;
                                        float _4438 = _3716 + _854;
                                        float _4439 = _3717 + _855;
                                        float _4443 = rsqrt(dot(float3(_4437, _4438, _4439), float3(_4437, _4438, _4439)));
                                        float _4444 = _4443 * _4437;
                                        float _4445 = _4443 * _4438;
                                        float _4446 = _4443 * _4439;
                                        float _4450 = clamp(dot(float3(_533, _535, _537), float3(_3715, _3716, _3717)), 0.0f, 1.0f);
                                        float _4454 = clamp(dot(float3(_533, _535, _537), float3(_4444, _4445, _4446)), 0.0f, 1.0f);
                                        float _4629;
                                        float _4630;
                                        float _4631;
                                        if (_4450 > 0.0f)
                                        {
                                            float _4596 = _4436 * _4436;
                                            float _4597 = _4596 * _4596;
                                            float _4601 = (((_4454 * _4597) - _4454) * _4454) + 1.0f;
                                            float _4605 = _4596 * 0.5f;
                                            float _4606 = 1.0f - _4605;
                                            float _4613 = 1.0f - clamp(dot(float3(_853, _854, _855), float3(_4444, _4445, _4446)), 0.0f, 1.0f);
                                            float _4614 = _4613 * _4613;
                                            float _4616 = (_4614 * _4614) * _4613;
                                            float _4625 = min((0.25f / (((_4450 * _4606) + _4605) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_533, _535, _537), float3(_853, _854, _855)), 0.0f, 1.0f)) * _4606) + _4605))) * (_4597 / ((_4601 * _4601) * 3.1415927410125732421875f)), _833) * _2416;
                                            _4629 = _4625 * ((_4616 * _4189) + _548);
                                            _4630 = _4625 * ((_4616 * _4190) + _551);
                                            _4631 = _4625 * ((_4616 * _4191) + _553);
                                        }
                                        else
                                        {
                                            _4629 = 0.0f;
                                            _4630 = 0.0f;
                                            _4631 = 0.0f;
                                        }
                                        float _4632 = min(_4629, 100000.0f);
                                        float _4633 = min(_4630, 100000.0f);
                                        float _4634 = min(_4631, 100000.0f);
                                        float _4739;
                                        float _4741;
                                        float _4743;
                                        if (_2420)
                                        {
                                            _4739 = _4632;
                                            _4741 = _4633;
                                            _4743 = _4634;
                                        }
                                        else
                                        {
                                            float _4745 = 1.0f - _4195;
                                            float _4746 = _4745 * _4745;
                                            float _4748 = 1.0f - (_4746 * _4746);
                                            _4739 = _4632 * _4748;
                                            _4741 = _4633 * _4748;
                                            _4743 = _4634 * _4748;
                                        }
                                        _4456 = _4739 * _4195;
                                        _4458 = _4741 * _4195;
                                        _4460 = _4743 * _4195;
                                    }
                                    else
                                    {
                                        _4456 = 0.0f;
                                        _4458 = 0.0f;
                                        _4460 = 0.0f;
                                    }
                                    float _4660;
                                    float _4662;
                                    float _4664;
                                    if (_4345)
                                    {
                                        float _4640 = max(exp2(log2(clamp(1.0f - _563, 0.0f, 1.0f)) * _53_m0[107u].y), _832);
                                        float _4641 = _3715 + _853;
                                        float _4642 = _3716 + _854;
                                        float _4643 = _3717 + _855;
                                        float _4647 = rsqrt(dot(float3(_4641, _4642, _4643), float3(_4641, _4642, _4643)));
                                        float _4648 = _4647 * _4641;
                                        float _4649 = _4647 * _4642;
                                        float _4650 = _4647 * _4643;
                                        float _4654 = clamp(dot(float3(_533, _535, _537), float3(_3715, _3716, _3717)), 0.0f, 1.0f);
                                        float _4658 = clamp(dot(float3(_533, _535, _537), float3(_4648, _4649, _4650)), 0.0f, 1.0f);
                                        float _4791;
                                        float _4792;
                                        float _4793;
                                        if (_4654 > 0.0f)
                                        {
                                            float _4758 = _4640 * _4640;
                                            float _4759 = _4758 * _4758;
                                            float _4763 = (((_4658 * _4759) - _4658) * _4658) + 1.0f;
                                            float _4767 = _4758 * 0.5f;
                                            float _4768 = 1.0f - _4767;
                                            float _4775 = 1.0f - clamp(dot(float3(_853, _854, _855), float3(_4648, _4649, _4650)), 0.0f, 1.0f);
                                            float _4776 = _4775 * _4775;
                                            float _4778 = (_4776 * _4776) * _4775;
                                            float _4787 = min((0.25f / (((_4654 * _4768) + _4767) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_533, _535, _537), float3(_853, _854, _855)), 0.0f, 1.0f)) * _4768) + _4767))) * (_4759 / ((_4763 * _4763) * 3.1415927410125732421875f)), _833) * _2416;
                                            _4791 = _4787 * ((_4778 * _4189) + _548);
                                            _4792 = _4787 * ((_4778 * _4190) + _551);
                                            _4793 = _4787 * ((_4778 * _4191) + _553);
                                        }
                                        else
                                        {
                                            _4791 = 0.0f;
                                            _4792 = 0.0f;
                                            _4793 = 0.0f;
                                        }
                                        float _4794 = min(_4791, 100000.0f);
                                        float _4795 = min(_4792, 100000.0f);
                                        float _4796 = min(_4793, 100000.0f);
                                        float _4807;
                                        float _4809;
                                        float _4811;
                                        if (_2420)
                                        {
                                            _4807 = _4794;
                                            _4809 = _4795;
                                            _4811 = _4796;
                                        }
                                        else
                                        {
                                            float _4813 = 1.0f - _4195;
                                            float _4814 = _4813 * _4813;
                                            float _4816 = 1.0f - (_4814 * _4814);
                                            _4807 = _4794 * _4816;
                                            _4809 = _4795 * _4816;
                                            _4811 = _4796 * _4816;
                                        }
                                        _4660 = _4807 * _4195;
                                        _4662 = _4809 * _4195;
                                        _4664 = _4811 * _4195;
                                    }
                                    else
                                    {
                                        _4660 = 0.0f;
                                        _4662 = 0.0f;
                                        _4664 = 0.0f;
                                    }
                                    frontier_phi_168_pred_167_ladder = (((((_545 * 0.3183098733425140380859375f) * _4191) * _4200) * _4332) * _4187) + _3378;
                                    frontier_phi_168_pred_167_ladder_1 = (((((_542 * 0.3183098733425140380859375f) * _4190) * _4200) * _4332) * _4186) + _3377;
                                    frontier_phi_168_pred_167_ladder_2 = (((((_539 * 0.3183098733425140380859375f) * _4189) * _4200) * _4332) * _4185) + _3376;
                                    frontier_phi_168_pred_167_ladder_3 = ((((_4664 - _4460) * _565) + _4460) * _4187) + _3381;
                                    frontier_phi_168_pred_167_ladder_4 = ((((_4662 - _4458) * _565) + _4458) * _4186) + _3380;
                                    frontier_phi_168_pred_167_ladder_5 = ((((_4660 - _4456) * _565) + _4456) * _4185) + _3379;
                                }
                                else
                                {
                                    frontier_phi_168_pred_167_ladder = _3378;
                                    frontier_phi_168_pred_167_ladder_1 = _3377;
                                    frontier_phi_168_pred_167_ladder_2 = _3376;
                                    frontier_phi_168_pred_167_ladder_3 = _3381;
                                    frontier_phi_168_pred_167_ladder_4 = _3380;
                                    frontier_phi_168_pred_167_ladder_5 = _3379;
                                }
                                frontier_phi_168_pred = frontier_phi_168_pred_167_ladder;
                                frontier_phi_168_pred_1 = frontier_phi_168_pred_167_ladder_1;
                                frontier_phi_168_pred_2 = frontier_phi_168_pred_167_ladder_2;
                                frontier_phi_168_pred_3 = frontier_phi_168_pred_167_ladder_3;
                                frontier_phi_168_pred_4 = frontier_phi_168_pred_167_ladder_4;
                                frontier_phi_168_pred_5 = frontier_phi_168_pred_167_ladder_5;
                            }
                            else
                            {
                                frontier_phi_168_pred = _3378;
                                frontier_phi_168_pred_1 = _3377;
                                frontier_phi_168_pred_2 = _3376;
                                frontier_phi_168_pred_3 = _3381;
                                frontier_phi_168_pred_4 = _3380;
                                frontier_phi_168_pred_5 = _3379;
                            }
                            _3287 = frontier_phi_168_pred;
                            _3285 = frontier_phi_168_pred_1;
                            _3283 = frontier_phi_168_pred_2;
                            _3290 = frontier_phi_168_pred_3;
                            _3289 = frontier_phi_168_pred_4;
                            _3288 = frontier_phi_168_pred_5;
                            if (_3292 > _3259)
                            {
                                break;
                            }
                            else
                            {
                                _3376 = _3283;
                                _3377 = _3285;
                                _3378 = _3287;
                                _3379 = _3288;
                                _3380 = _3289;
                                _3381 = _3290;
                                _3382 = _3292;
                                _3383 = _3386;
                                continue;
                            }
                        }
                        _3282 = _3283;
                        _3284 = _3285;
                        _3286 = _3287;
                        _3149 = _3288;
                        _3152 = _3289;
                        _3155 = _3290;
                        _3291 = _3292;
                        _3293 = _3386;
                    }
                    float _3140;
                    float _3143;
                    float _3146;
                    uint _3371;
                    uint _3373;
                    if (_3291 > _3260)
                    {
                        _3140 = _3282;
                        _3143 = _3284;
                        _3146 = _3286;
                        _3371 = _3291;
                        _3373 = _3293;
                    }
                    else
                    {
                        float _3368;
                        float _3369;
                        float _3370;
                        float _3510 = _3282;
                        float _3511 = _3284;
                        float _3512 = _3286;
                        uint _3513 = _3291;
                        uint _3514 = _3293;
                        uint _3372;
                        uint _3517;
                        uint _3532;
                        uint _3533;
                        float _3561;
                        float _3563;
                        float _3566;
                        float _3568;
                        uint _3570;
                        bool _3573;
                        float _3575;
                        float _3578;
                        float _3580;
                        float _3583;
                        float _3585;
                        float _3588;
                        float _3590;
                        bool _3595;
                        for (;;)
                        {
                            _3372 = _3513 + 1u;
                            _3517 = _35.Load(_3513).x;
                            uint _3519 = _3514 * 4u;
                            uint4 _3531 = uint4(_36.Load(_3519).x, _36.Load(_3519 + 1u).x, _36.Load(_3519 + 2u).x, _36.Load(_3519 + 3u).x);
                            _3532 = _3531.x;
                            _3533 = _3531.y;
                            uint _3534 = _3531.z;
                            uint _3535 = _3531.w;
                            uint _3537 = _3514 * 4u;
                            uint3 _3546 = uint3(_37.Load(_3537).x, _37.Load(_3537 + 1u).x, _37.Load(_3537 + 2u).x);
                            uint _3547 = _3546.x;
                            uint _3548 = _3546.y;
                            uint _3549 = _3546.z;
                            _3561 = spvUnpackHalf2x16(_3533 >> 16u).x;
                            _3563 = spvUnpackHalf2x16(_3534).x;
                            _3566 = spvUnpackHalf2x16(_3534 >> 16u).x;
                            _3568 = spvUnpackHalf2x16(_3535).x;
                            _3570 = (_3535 >> 16u) & 7u;
                            _3573 = (_3535 & 524288u) != 0u;
                            _3575 = spvUnpackHalf2x16(_3547).x;
                            _3578 = spvUnpackHalf2x16(_3547 >> 16u).x;
                            _3580 = spvUnpackHalf2x16(_3548).x;
                            _3583 = spvUnpackHalf2x16(_3548 >> 16u).x;
                            _3585 = spvUnpackHalf2x16(_3549).x;
                            _3588 = spvUnpackHalf2x16(_3549 >> 16u).x;
                            _3590 = spvUnpackHalf2x16(uint3(_3552, _3553, _38.Load((_3514 * 4u) + 2u).x).z).x;
                            _3595 = (_3535 < 3221225472u) && (((_436 & 255u) & (_3535 >> 22u)) != 0u);
                            float frontier_phi_174_pred;
                            float frontier_phi_174_pred_1;
                            float frontier_phi_174_pred_2;
                            if (_3595)
                            {
                                float _3696 = spvUnpackHalf2x16(_3532).x - _3243;
                                float _3697 = spvUnpackHalf2x16(_3532 >> 16u).x - _3244;
                                float _3698 = spvUnpackHalf2x16(_3533).x - _3245;
                                float _3704 = sqrt(((_3697 * _3697) + (_3698 * _3698)) + (_3696 * _3696));
                                float _3705 = _3704 * _3561;
                                float frontier_phi_174_pred_173_ladder;
                                float frontier_phi_174_pred_173_ladder_1;
                                float frontier_phi_174_pred_173_ladder_2;
                                if (_3705 < 1.0f)
                                {
                                    float _3851 = rsqrt(dot(float3(_3696, _3697, _3698), float3(_3696, _3697, _3698)));
                                    float _3852 = _3851 * _3696;
                                    float _3853 = _3851 * _3697;
                                    float _3854 = _3851 * _3698;
                                    float _3855 = _3704 * _3704;
                                    float _3857 = (_3561 * _3561) * _3855;
                                    float _3860 = clamp(1.0f - (_3857 * _3857), 0.0f, 1.0f);
                                    float _4100;
                                    if (_3570 == 0u)
                                    {
                                        _4100 = (_3860 * _3860) * (1.0f / (max(_3855, 9.9999997473787516355514526367188e-05f) + ((_3590 * _3590) * 0.5f)));
                                    }
                                    else
                                    {
                                        _4100 = max((1.0f / dot(float3(1.0f, _3705, _3705 * _3705), float3(_53_m0[_3570 + 60u].xyz))) * (1.0f - _3705), 0.0f);
                                    }
                                    float _4101 = (-0.0f) - _3575;
                                    float _4125 = clamp((clamp(dot(float3(((_3583 * _3578) - (_3580 * _4101)) * 2.0f, ((_3580 * _3578) - (_3583 * _3575)) * 2.0f, (((_3575 * _4101) - (_3578 * _3578)) * 2.0f) + 1.0f), float3((-0.0f) - _3852, (-0.0f) - _3853, (-0.0f) - _3854)), 0.0f, 1.0f) - _3588) / (_3585 - _3588), 0.0f, 1.0f);
                                    float _4139 = (((_4125 * _4125) * _4100) * max(float(_3573) * 16.0f, 1.0f)) * (3.0f - (_4125 * 2.0f));
                                    float _4143 = (_3561 < 0.02857142873108386993408203125f) ? _53_m0[104u].w : 0.0f;
                                    float _4151 = 1.0f - _4143;
                                    float _4155 = clamp(((_4151 * clamp(dot(float3(_524, _527, _530), float3(_3852, _3853, _3854)), 0.0f, 1.0f)) + _4143) * _4151, 0.0f, 1.0f);
                                    frontier_phi_174_pred_173_ladder = ((_4155 * ((_545 * 0.3183098733425140380859375f) * (1.0f - _553))) * (_4139 * _3568)) + _3512;
                                    frontier_phi_174_pred_173_ladder_1 = ((_4155 * ((_542 * 0.3183098733425140380859375f) * (1.0f - _551))) * (_4139 * _3566)) + _3511;
                                    frontier_phi_174_pred_173_ladder_2 = ((_4155 * ((_539 * 0.3183098733425140380859375f) * (1.0f - _548))) * (_4139 * _3563)) + _3510;
                                }
                                else
                                {
                                    frontier_phi_174_pred_173_ladder = _3512;
                                    frontier_phi_174_pred_173_ladder_1 = _3511;
                                    frontier_phi_174_pred_173_ladder_2 = _3510;
                                }
                                frontier_phi_174_pred = frontier_phi_174_pred_173_ladder;
                                frontier_phi_174_pred_1 = frontier_phi_174_pred_173_ladder_1;
                                frontier_phi_174_pred_2 = frontier_phi_174_pred_173_ladder_2;
                            }
                            else
                            {
                                frontier_phi_174_pred = _3512;
                                frontier_phi_174_pred_1 = _3511;
                                frontier_phi_174_pred_2 = _3510;
                            }
                            _3370 = frontier_phi_174_pred;
                            _3369 = frontier_phi_174_pred_1;
                            _3368 = frontier_phi_174_pred_2;
                            if (_3372 > _3260)
                            {
                                break;
                            }
                            else
                            {
                                _3510 = _3368;
                                _3511 = _3369;
                                _3512 = _3370;
                                _3513 = _3372;
                                _3514 = _3517;
                                continue;
                            }
                        }
                        _3140 = _3368;
                        _3143 = _3369;
                        _3146 = _3370;
                        _3371 = _3372;
                        _3373 = _3517;
                    }
                    float _3158;
                    float _3160;
                    float _3162;
                    uint _3505;
                    uint _3507;
                    if (_3371 > _3261)
                    {
                        _3158 = 0.0f;
                        _3160 = 0.0f;
                        _3162 = 0.0f;
                        _3505 = _3371;
                        _3507 = _3373;
                    }
                    else
                    {
                        float _3502;
                        float _3503;
                        float _3504;
                        float _3635 = 0.0f;
                        float _3636 = 0.0f;
                        float _3637 = 0.0f;
                        uint _3638 = _3371;
                        uint _3639 = _3373;
                        uint _3506;
                        uint _3642;
                        float _3672;
                        float _3675;
                        float _3677;
                        float _3680;
                        float _3684;
                        float _3687;
                        bool _3688;
                        for (;;)
                        {
                            _3506 = _3638 + 1u;
                            _3642 = _35.Load(_3638).x;
                            uint _3644 = _3639 * 4u;
                            uint4 _3656 = uint4(_36.Load(_3644).x, _36.Load(_3644 + 1u).x, _36.Load(_3644 + 2u).x, _36.Load(_3644 + 3u).x);
                            uint _3657 = _3656.x;
                            uint _3658 = _3656.y;
                            uint _3659 = _3656.z;
                            uint _3660 = _3656.w;
                            _3672 = spvUnpackHalf2x16(_3659).x;
                            _3675 = spvUnpackHalf2x16(_3659 >> 16u).x;
                            _3677 = spvUnpackHalf2x16(_3660).x;
                            _3680 = spvUnpackHalf2x16(_3660 >> 16u).x;
                            float _3681 = spvUnpackHalf2x16(_3657).x - _3243;
                            float _3682 = spvUnpackHalf2x16(_3657 >> 16u).x - _3244;
                            float _3683 = spvUnpackHalf2x16(_3658).x - _3245;
                            _3684 = dot(float3(_3681, _3682, _3683), float3(_3681, _3682, _3683));
                            _3687 = _3684 * spvUnpackHalf2x16(_3658 >> 16u).x;
                            _3688 = _3687 < 1.0f;
                            float frontier_phi_181_pred;
                            float frontier_phi_181_pred_1;
                            float frontier_phi_181_pred_2;
                            if (_3688)
                            {
                                float _4096;
                                if (_3680 != 0.0f)
                                {
                                    _4096 = (1.0f / ((_3687 * _3680) + 1.0f)) * (1.0f - _3687);
                                }
                                else
                                {
                                    float _3991 = clamp(1.0f - (_3687 * _3687), 0.0f, 1.0f);
                                    _4096 = (_3991 * _3991) * (1.0f / max(_3684, 9.9999997473787516355514526367188e-05f));
                                }
                                frontier_phi_181_pred = (_4096 * _3675) + _3636;
                                frontier_phi_181_pred_1 = (_4096 * _3672) + _3635;
                                frontier_phi_181_pred_2 = (_4096 * _3677) + _3637;
                            }
                            else
                            {
                                frontier_phi_181_pred = _3636;
                                frontier_phi_181_pred_1 = _3635;
                                frontier_phi_181_pred_2 = _3637;
                            }
                            _3503 = frontier_phi_181_pred;
                            _3502 = frontier_phi_181_pred_1;
                            _3504 = frontier_phi_181_pred_2;
                            if (_3506 > _3261)
                            {
                                break;
                            }
                            else
                            {
                                _3635 = _3502;
                                _3636 = _3503;
                                _3637 = _3504;
                                _3638 = _3506;
                                _3639 = _3642;
                                continue;
                            }
                        }
                        _3158 = _3502;
                        _3160 = _3503;
                        _3162 = _3504;
                        _3505 = _3506;
                        _3507 = _3642;
                    }
                    float frontier_phi_151_152_ladder_164_ladder;
                    float frontier_phi_151_152_ladder_164_ladder_1;
                    float frontier_phi_151_152_ladder_164_ladder_2;
                    float frontier_phi_151_152_ladder_164_ladder_3;
                    float frontier_phi_151_152_ladder_164_ladder_4;
                    float frontier_phi_151_152_ladder_164_ladder_5;
                    float frontier_phi_151_152_ladder_164_ladder_6;
                    float frontier_phi_151_152_ladder_164_ladder_7;
                    float frontier_phi_151_152_ladder_164_ladder_8;
                    if (_3505 > _3262)
                    {
                        frontier_phi_151_152_ladder_164_ladder = _3162;
                        frontier_phi_151_152_ladder_164_ladder_1 = _3140;
                        frontier_phi_151_152_ladder_164_ladder_2 = _3143;
                        frontier_phi_151_152_ladder_164_ladder_3 = _3146;
                        frontier_phi_151_152_ladder_164_ladder_4 = _3149;
                        frontier_phi_151_152_ladder_164_ladder_5 = _3152;
                        frontier_phi_151_152_ladder_164_ladder_6 = _3155;
                        frontier_phi_151_152_ladder_164_ladder_7 = _3158;
                        frontier_phi_151_152_ladder_164_ladder_8 = _3160;
                    }
                    else
                    {
                        float _3141;
                        float _3144;
                        float _3147;
                        float _3150;
                        float _3153;
                        float _3156;
                        float _3746 = _3140;
                        float _3747 = _3143;
                        float _3748 = _3146;
                        float _3749 = _3149;
                        float _3750 = _3152;
                        float _3751 = _3155;
                        uint _3752 = _3505;
                        uint _3754 = _3507;
                        uint _3753;
                        uint _3758;
                        uint _3773;
                        uint _3774;
                        float _3806;
                        float _3808;
                        float _3811;
                        float _3813;
                        uint _3815;
                        bool _3818;
                        float _3820;
                        float _3823;
                        float _3825;
                        float _3828;
                        float _3830;
                        float _3833;
                        float _3835;
                        float _3837;
                        bool _3842;
                        for (;;)
                        {
                            _3753 = _3752 + 1u;
                            _3758 = _35.Load(_3752).x;
                            uint _3760 = _3754 * 4u;
                            uint4 _3772 = uint4(_36.Load(_3760).x, _36.Load(_3760 + 1u).x, _36.Load(_3760 + 2u).x, _36.Load(_3760 + 3u).x);
                            _3773 = _3772.x;
                            _3774 = _3772.y;
                            uint _3775 = _3772.z;
                            uint _3776 = _3772.w;
                            uint _3778 = _3754 * 4u;
                            uint4 _3790 = uint4(_37.Load(_3778).x, _37.Load(_3778 + 1u).x, _37.Load(_3778 + 2u).x, _37.Load(_3778 + 3u).x);
                            uint _3791 = _3790.x;
                            uint _3792 = _3790.y;
                            uint _3793 = _3790.z;
                            _3806 = spvUnpackHalf2x16(_3774 >> 16u).x;
                            _3808 = spvUnpackHalf2x16(_3775).x;
                            _3811 = spvUnpackHalf2x16(_3775 >> 16u).x;
                            _3813 = spvUnpackHalf2x16(_3776).x;
                            _3815 = (_3776 >> 16u) & 7u;
                            _3818 = (_3776 & 524288u) != 0u;
                            _3820 = spvUnpackHalf2x16(_3791).x;
                            _3823 = spvUnpackHalf2x16(_3791 >> 16u).x;
                            _3825 = spvUnpackHalf2x16(_3792).x;
                            _3828 = spvUnpackHalf2x16(_3792 >> 16u).x;
                            _3830 = spvUnpackHalf2x16(_3793).x;
                            _3833 = spvUnpackHalf2x16(_3793 >> 16u).x;
                            _3835 = spvUnpackHalf2x16(_3790.w).x;
                            _3837 = spvUnpackHalf2x16(uint3(_3797, _3798, _38.Load((_3754 * 4u) + 2u).x).z).x;
                            _3842 = (_3776 < 3221225472u) && (((_436 & 255u) & (_3776 >> 22u)) != 0u);
                            float frontier_phi_188_pred;
                            float frontier_phi_188_pred_1;
                            float frontier_phi_188_pred_2;
                            float frontier_phi_188_pred_3;
                            float frontier_phi_188_pred_4;
                            float frontier_phi_188_pred_5;
                            if (_3842)
                            {
                                float _3900 = (-0.0f) - _3825;
                                float _3906 = (_3825 * _3900) - (_3820 * _3820);
                                float _3907 = _3828 * _3820;
                                float _3912 = (-0.0f) - _3820;
                                float _3925 = _3835 * ((_3823 * _3820) - (_3828 * _3825));
                                float _3928 = _3835 * (_3907 - (_3823 * _3900));
                                float _3929 = spvUnpackHalf2x16(_3773).x - _3925;
                                float _3930 = spvUnpackHalf2x16(_3773 >> 16u).x - (_3835 * (_3906 + 0.5f));
                                float _3931 = spvUnpackHalf2x16(_3774).x - _3928;
                                float _3935 = _3925 * 2.0f;
                                float _3936 = _3835 * ((_3906 * 2.0f) + 1.0f);
                                float _3937 = _3928 * 2.0f;
                                float _3945 = clamp(dot(float3(_3935, _3936, _3937), float3(_3243 - _3929, _3244 - _3930, _3245 - _3931)) / dot(float3(_3935, _3936, _3937), float3(_3935, _3936, _3937)), 0.0f, 1.0f);
                                float _3950 = (_3945 * _3935) + (_3929 - _3243);
                                float _3952 = (_3945 * _3936) + (_3930 - _3244);
                                float _3954 = (_3945 * _3937) + (_3931 - _3245);
                                float _3958 = rsqrt(dot(float3(_3950, _3952, _3954), float3(_3950, _3952, _3954)));
                                float _3959 = _3950 * _3958;
                                float _3960 = _3952 * _3958;
                                float _3961 = _3954 * _3958;
                                float _3967 = sqrt(((_3950 * _3950) + (_3952 * _3952)) + (_3954 * _3954));
                                float _3968 = _3967 * _3967;
                                float _3970 = (_3806 * _3806) * _3968;
                                float _3973 = clamp(1.0f - (_3970 * _3970), 0.0f, 1.0f);
                                float _4293;
                                if (_3815 == 0u)
                                {
                                    _4293 = (_3973 * _3973) * (1.0f / (max(_3968, 9.9999997473787516355514526367188e-05f) + ((_3837 * _3837) * 0.5f)));
                                }
                                else
                                {
                                    float _4081 = _3967 * _3806;
                                    _4293 = max((1.0f / dot(float3(1.0f, _4081, _4081 * _4081), float3(_53_m0[_3815 + 60u].xyz))) * (1.0f - _4081), 0.0f);
                                }
                                float frontier_phi_188_pred_207_ladder;
                                float frontier_phi_188_pred_207_ladder_1;
                                float frontier_phi_188_pred_207_ladder_2;
                                float frontier_phi_188_pred_207_ladder_3;
                                float frontier_phi_188_pred_207_ladder_4;
                                float frontier_phi_188_pred_207_ladder_5;
                                if (_4293 < 9.9999997473787516355514526367188e-06f)
                                {
                                    frontier_phi_188_pred_207_ladder = _3751;
                                    frontier_phi_188_pred_207_ladder_1 = _3750;
                                    frontier_phi_188_pred_207_ladder_2 = _3749;
                                    frontier_phi_188_pred_207_ladder_3 = _3748;
                                    frontier_phi_188_pred_207_ladder_4 = _3747;
                                    frontier_phi_188_pred_207_ladder_5 = _3746;
                                }
                                else
                                {
                                    float _4423 = clamp((clamp(dot(float3(((_3828 * _3823) - (_3825 * _3912)) * 2.0f, ((_3825 * _3823) - _3907) * 2.0f, (((_3820 * _3912) - (_3823 * _3823)) * 2.0f) + 1.0f), float3((-0.0f) - _3959, (-0.0f) - _3960, (-0.0f) - _3961)), 0.0f, 1.0f) - _3833) / (_3830 - _3833), 0.0f, 1.0f);
                                    float _4427 = (_4423 * _4423) * (3.0f - (_4423 * 2.0f));
                                    float _4429 = (_4427 * _4427) * _4293;
                                    float frontier_phi_188_pred_207_ladder_213_ladder;
                                    float frontier_phi_188_pred_207_ladder_213_ladder_1;
                                    float frontier_phi_188_pred_207_ladder_213_ladder_2;
                                    float frontier_phi_188_pred_207_ladder_213_ladder_3;
                                    float frontier_phi_188_pred_207_ladder_213_ladder_4;
                                    float frontier_phi_188_pred_207_ladder_213_ladder_5;
                                    if (_4429 < 9.9999997473787516355514526367188e-06f)
                                    {
                                        frontier_phi_188_pred_207_ladder_213_ladder = _3751;
                                        frontier_phi_188_pred_207_ladder_213_ladder_1 = _3750;
                                        frontier_phi_188_pred_207_ladder_213_ladder_2 = _3749;
                                        frontier_phi_188_pred_207_ladder_213_ladder_3 = _3748;
                                        frontier_phi_188_pred_207_ladder_213_ladder_4 = _3747;
                                        frontier_phi_188_pred_207_ladder_213_ladder_5 = _3746;
                                    }
                                    else
                                    {
                                        float _4554 = (_4429 * _3119) * max(float(_3818) * 16.0f, 1.0f);
                                        float _4555 = _4554 * _3808;
                                        float _4556 = _4554 * _3811;
                                        float _4557 = _4554 * _3813;
                                        float _4563 = clamp(clamp(dot(float3(_524, _527, _530), float3(_3959, _3960, _3961)), 0.0f, 1.0f), 0.0f, 1.0f) * 0.3183098733425140380859375f;
                                        float _4571 = (_48_m0[122u].w * _576) * _4563;
                                        frontier_phi_188_pred_207_ladder_213_ladder = ((_4571 * _553) * _4557) + _3751;
                                        frontier_phi_188_pred_207_ladder_213_ladder_1 = ((_4571 * _551) * _4556) + _3750;
                                        frontier_phi_188_pred_207_ladder_213_ladder_2 = ((_4571 * _548) * _4555) + _3749;
                                        frontier_phi_188_pred_207_ladder_213_ladder_3 = ((((1.0f - _553) * _545) * _4557) * _4563) + _3748;
                                        frontier_phi_188_pred_207_ladder_213_ladder_4 = ((((1.0f - _551) * _542) * _4556) * _4563) + _3747;
                                        frontier_phi_188_pred_207_ladder_213_ladder_5 = ((((1.0f - _548) * _539) * _4555) * _4563) + _3746;
                                    }
                                    frontier_phi_188_pred_207_ladder = frontier_phi_188_pred_207_ladder_213_ladder;
                                    frontier_phi_188_pred_207_ladder_1 = frontier_phi_188_pred_207_ladder_213_ladder_1;
                                    frontier_phi_188_pred_207_ladder_2 = frontier_phi_188_pred_207_ladder_213_ladder_2;
                                    frontier_phi_188_pred_207_ladder_3 = frontier_phi_188_pred_207_ladder_213_ladder_3;
                                    frontier_phi_188_pred_207_ladder_4 = frontier_phi_188_pred_207_ladder_213_ladder_4;
                                    frontier_phi_188_pred_207_ladder_5 = frontier_phi_188_pred_207_ladder_213_ladder_5;
                                }
                                frontier_phi_188_pred = frontier_phi_188_pred_207_ladder;
                                frontier_phi_188_pred_1 = frontier_phi_188_pred_207_ladder_1;
                                frontier_phi_188_pred_2 = frontier_phi_188_pred_207_ladder_2;
                                frontier_phi_188_pred_3 = frontier_phi_188_pred_207_ladder_3;
                                frontier_phi_188_pred_4 = frontier_phi_188_pred_207_ladder_4;
                                frontier_phi_188_pred_5 = frontier_phi_188_pred_207_ladder_5;
                            }
                            else
                            {
                                frontier_phi_188_pred = _3751;
                                frontier_phi_188_pred_1 = _3750;
                                frontier_phi_188_pred_2 = _3749;
                                frontier_phi_188_pred_3 = _3748;
                                frontier_phi_188_pred_4 = _3747;
                                frontier_phi_188_pred_5 = _3746;
                            }
                            _3156 = frontier_phi_188_pred;
                            _3153 = frontier_phi_188_pred_1;
                            _3150 = frontier_phi_188_pred_2;
                            _3147 = frontier_phi_188_pred_3;
                            _3144 = frontier_phi_188_pred_4;
                            _3141 = frontier_phi_188_pred_5;
                            if (_3753 > _3262)
                            {
                                break;
                            }
                            else
                            {
                                _3746 = _3141;
                                _3747 = _3144;
                                _3748 = _3147;
                                _3749 = _3150;
                                _3750 = _3153;
                                _3751 = _3156;
                                _3752 = _3753;
                                _3754 = _3758;
                                continue;
                            }
                        }
                        frontier_phi_151_152_ladder_164_ladder = _3162;
                        frontier_phi_151_152_ladder_164_ladder_1 = _3141;
                        frontier_phi_151_152_ladder_164_ladder_2 = _3144;
                        frontier_phi_151_152_ladder_164_ladder_3 = _3147;
                        frontier_phi_151_152_ladder_164_ladder_4 = _3150;
                        frontier_phi_151_152_ladder_164_ladder_5 = _3153;
                        frontier_phi_151_152_ladder_164_ladder_6 = _3156;
                        frontier_phi_151_152_ladder_164_ladder_7 = _3158;
                        frontier_phi_151_152_ladder_164_ladder_8 = _3160;
                    }
                    frontier_phi_151_152_ladder = frontier_phi_151_152_ladder_164_ladder;
                    frontier_phi_151_152_ladder_1 = frontier_phi_151_152_ladder_164_ladder_1;
                    frontier_phi_151_152_ladder_2 = frontier_phi_151_152_ladder_164_ladder_2;
                    frontier_phi_151_152_ladder_3 = frontier_phi_151_152_ladder_164_ladder_3;
                    frontier_phi_151_152_ladder_4 = frontier_phi_151_152_ladder_164_ladder_4;
                    frontier_phi_151_152_ladder_5 = frontier_phi_151_152_ladder_164_ladder_5;
                    frontier_phi_151_152_ladder_6 = frontier_phi_151_152_ladder_164_ladder_6;
                    frontier_phi_151_152_ladder_7 = frontier_phi_151_152_ladder_164_ladder_7;
                    frontier_phi_151_152_ladder_8 = frontier_phi_151_152_ladder_164_ladder_8;
                }
                _3139 = frontier_phi_151_152_ladder_1;
                _3142 = frontier_phi_151_152_ladder_2;
                _3145 = frontier_phi_151_152_ladder_3;
                _3148 = frontier_phi_151_152_ladder_4;
                _3151 = frontier_phi_151_152_ladder_5;
                _3154 = frontier_phi_151_152_ladder_6;
                _3157 = frontier_phi_151_152_ladder_7;
                _3159 = frontier_phi_151_152_ladder_8;
                _3161 = frontier_phi_151_152_ladder;
            }
            float _3164 = (_1318 * _2417) * ((_48_m0[98u].x * (_1097 + (-1.0f))) + 1.0f);
            float _3168 = _1101 * _2417;
            _2331 = ((_3015 * (((((((((_2956 * 0.100000001490116119384765625f) + (_2947 * 0.23299999535083770751953125f)) + (_2968 * 0.1180000007152557373046875f)) + (_2977 * 0.112999998033046722412109375f)) + (_2987 * 0.3580000102519989013671875f)) + (exp2(_2944 * (-0.1946956813335418701171875f)) * 0.078000001609325408935546875f)) * _539) * _3008) + ((((_539 * 0.3183098733425140380859375f) * _2465) * _2472) * _2638))) + (_3139 * _3164)) + ((_3157 * _3168) * ((1.0f - _548) * _539));
            _2333 = ((_3016 * ((((((((_2956 * 0.3359999954700469970703125f) + (_2947 * 0.4550000131130218505859375f)) + (_2968 * 0.19799999892711639404296875f)) + _2980) + (_2987 * 0.0040000001899898052215576171875f)) * _542) * _3008) + ((((_542 * 0.3183098733425140380859375f) * _2466) * _2472) * _2638))) + (_3142 * _3164)) + ((_3159 * _3168) * ((1.0f - _551) * _542));
            _2335 = ((_3017 * ((((((_2956 * 0.3440000116825103759765625f) + (_2947 * 0.648999989032745361328125f)) + _2980) * _545) * _3008) + ((((_545 * 0.3183098733425140380859375f) * _2467) * _2472) * _2638))) + (_3145 * _3164)) + ((_3161 * _3168) * ((1.0f - _553) * _545));
            _2337 = (_3148 * _3164) + ((((_2923 - _2765) * _565) + _2765) * _3015);
            _2339 = (_3151 * _3164) + ((((_2925 - _2767) * _565) + _2767) * _3016);
            _2341 = (_3154 * _3164) + ((((_2927 - _2769) * _565) + _2769) * _3017);
        }
        float _2346 = (((((_1475 * (1.0f / (1.0f - (_1109 * min(0.999000012874603271484375f, _573))))) * (_1370 - (_1370 * _442))) + (_1353 * _1309)) + _2255) + _2331) + _2337;
        float _2350 = (((((_1475 * (1.0f / (1.0f - (_1109 * min(0.999000012874603271484375f, _574))))) * (_1375 - (_1375 * _442))) + (_1354 * _1310)) + _2257) + _2333) + _2339;
        float _2354 = (((((_1475 * (1.0f / (1.0f - (_1109 * min(0.999000012874603271484375f, _575))))) * (_1380 - (_1380 * _442))) + (_1355 * _1311)) + _2259) + _2335) + _2341;
        float _2362 = _352 - _48_m0[8u].x;
        float _2363 = _353 - _48_m0[8u].y;
        float _2364 = _354 - _48_m0[8u].z;
        float _2368 = rsqrt(dot(float3(_2362, _2363, _2364), float3(_2362, _2363, _2364)));
        float _2374 = sqrt(((_2362 * _2362) + (_2363 * _2363)) + (_2364 * _2364));
        float _2384 = (_352 - _48_m0[111u].x) * _48_m0[111u].z;
        float _2386 = 1.0f - ((_354 - _48_m0[111u].y) * _48_m0[111u].w);
        float _2393 = _48_m0[112u].x - _353;
        float _2405 = _53_m0[53u].z - _53_m0[53u].y;
        float _2410 = 1.0f - exp2((-0.0f) - (_53_m0[53u].w * min(1000000.0f, max(0.0f, _2374 - _53_m0[53u].x))));
        bool _2415 = _53_m0[54u].y > 0.0f;
        float _655;
        float _2783;
        float _2785;
        float _2787;
        if ((asuint(_53_m0[_703]).z != 0u) && _581)
        {
            float4 _2540 = _15[584u].SampleLevel(_56, float2(_2384, _2386), 0.0f);
            float _2550 = _2368 * _2363;
            float _2554 = (((clamp(_2540.y, 0.0f, 1.0f) * _2405) * clamp(((_2393 - _48_m0[112u].w) + (_2540.x * _48_m0[112u].y)) / _48_m0[112u].z, 0.0f, 1.0f)) + _53_m0[53u].y) * _2410;
            float _2710;
            if (_2415)
            {
                float _2708 = min(1000000.0f, max(0.0f, _2374 - _53_m0[54u].w)) * 0.001000000047497451305389404296875f;
                float _2771;
                if (_2550 == 0.0f)
                {
                    _2771 = _2708;
                }
                else
                {
                    float _2776 = _53_m0[54u].y * _2550;
                    _2771 = (1.0f - exp2((-0.0f) - (_2776 * _2708))) / _2776;
                }
                _2710 = clamp((_2771 * _53_m0[54u].x) + _2554, 0.0f, 1.0f);
            }
            else
            {
                _2710 = _2554;
            }
            float frontier_phi_130_123_ladder;
            float frontier_phi_130_123_ladder_1;
            float frontier_phi_130_123_ladder_2;
            float frontier_phi_130_123_ladder_3;
            if (asuint(_53_m0[152u]).w == 0u)
            {
                frontier_phi_130_123_ladder = _2354;
                frontier_phi_130_123_ladder_1 = _2710;
                frontier_phi_130_123_ladder_2 = _2346;
                frontier_phi_130_123_ladder_3 = _2350;
            }
            else
            {
                float _2811 = ((-0.0f) - _53_m0[151u].z) / (_53_m0[150u].w * ((1.0f - _278) - _53_m0[151u].w));
                float _3021;
                if (_53_m0[152u].z < 0.001000000047497451305389404296875f)
                {
                    _3021 = _2811;
                }
                else
                {
                    _3021 = log2((_53_m0[152u].z * _2811) + 1.0f) / log2(_53_m0[152u].z + 1.0f);
                }
                frontier_phi_130_123_ladder = _2354;
                frontier_phi_130_123_ladder_1 = (_42.SampleLevel(_57, float3(_279 / (_53_m0[151u].x + (-1.0f)), _281 / (_53_m0[151u].y + (-1.0f)), _3021), 0.0f).w * (_2710 + (-1.0f))) + 1.0f;
                frontier_phi_130_123_ladder_2 = _2346;
                frontier_phi_130_123_ladder_3 = _2350;
            }
            _655 = frontier_phi_130_123_ladder_1;
            _2783 = frontier_phi_130_123_ladder_2;
            _2785 = frontier_phi_130_123_ladder_3;
            _2787 = frontier_phi_130_123_ladder;
        }
        else
        {
            float _2556 = _2363 * _2368;
            float4 _2561 = _15[584u].SampleLevel(_56, float2(_2384, _2386), 0.0f);
            float4 _2586 = _21.SampleLevel(_59, float3(_2362 * _2368, _2556, _2364 * _2368), (1.0f - clamp((_2374 - _53_m0[55u].x) / (_53_m0[55u].y - _53_m0[55u].x), 0.0f, 1.0f)) * _53_m0[55u].z);
            float _2600 = (((clamp(_2561.y, 0.0f, 1.0f) * _2405) * clamp(((_2393 - _48_m0[112u].w) + (_2561.x * _48_m0[112u].y)) / _48_m0[112u].z, 0.0f, 1.0f)) + _53_m0[53u].y) * _2410;
            float _2724;
            if (_2415)
            {
                float _2722 = min(1000000.0f, max(0.0f, _2374 - _53_m0[54u].w)) * 0.001000000047497451305389404296875f;
                float _2816;
                if (_2556 == 0.0f)
                {
                    _2816 = _2722;
                }
                else
                {
                    float _2821 = _53_m0[54u].y * _2556;
                    _2816 = (1.0f - exp2((-0.0f) - (_2821 * _2722))) / _2821;
                }
                _2724 = clamp((_2816 * _53_m0[54u].x) + _2600, 0.0f, 1.0f);
            }
            else
            {
                _2724 = _2600;
            }
            float _2732 = (_2724 * ((_48_m0[59u].z * _2586.x) - _2346)) + _2346;
            float _2733 = (_2724 * ((_48_m0[59u].z * _2586.y) - _2350)) + _2350;
            float _2734 = (_2724 * ((_48_m0[59u].z * _2586.z) - _2354)) + _2354;
            float frontier_phi_130_125_ladder;
            float frontier_phi_130_125_ladder_1;
            float frontier_phi_130_125_ladder_2;
            float frontier_phi_130_125_ladder_3;
            if (asuint(_53_m0[152u]).w == 0u)
            {
                frontier_phi_130_125_ladder = _2734;
                frontier_phi_130_125_ladder_1 = _2724;
                frontier_phi_130_125_ladder_2 = _2732;
                frontier_phi_130_125_ladder_3 = _2733;
            }
            else
            {
                float _2843 = ((-0.0f) - _53_m0[151u].z) / (_53_m0[150u].w * ((1.0f - _278) - _53_m0[151u].w));
                float _3036;
                if (_53_m0[152u].z < 0.001000000047497451305389404296875f)
                {
                    _3036 = _2843;
                }
                else
                {
                    _3036 = log2((_53_m0[152u].z * _2843) + 1.0f) / log2(_53_m0[152u].z + 1.0f);
                }
                float4 _3041 = _42.SampleLevel(_57, float3(_279 / (_53_m0[151u].x + (-1.0f)), _281 / (_53_m0[151u].y + (-1.0f)), _3036), 0.0f);
                float _3046 = _3041.w;
                frontier_phi_130_125_ladder = (_3041.z / _53_m0[152u].x) + (_3046 * _2734);
                frontier_phi_130_125_ladder_1 = (_3046 * (_2724 + (-1.0f))) + 1.0f;
                frontier_phi_130_125_ladder_2 = (_3046 * _2732) + (_3041.x / _53_m0[152u].x);
                frontier_phi_130_125_ladder_3 = (_3041.y / _53_m0[152u].x) + (_3046 * _2733);
            }
            _655 = frontier_phi_130_125_ladder_1;
            _2783 = frontier_phi_130_125_ladder_2;
            _2785 = frontier_phi_130_125_ladder_3;
            _2787 = frontier_phi_130_125_ladder;
        }
        _654 = _655;
        _656 = _48_m0[59u].x * _2783;
        _658 = _48_m0[59u].x * _2785;
        _660 = _48_m0[59u].x * _2787;
    }
    float _670 = clamp((_654 - _48_m0[94u].z) / (_48_m0[94u].w - _48_m0[94u].z), 0.0f, 1.0f);
    SV_Target_3.x = _670;
    SV_Target_3.y = _670;
    SV_Target_3.z = _670;
    SV_Target_3.w = 1.0f;
    float _778;
    float _780;
    float _782;
    if ((asuint(_656) & 2139095040u) == 2139095040u)
    {
        _778 = 0.0f;
        _780 = 0.0f;
        _782 = 0.0f;
    }
    else
    {
        float frontier_phi_15_16_ladder;
        float frontier_phi_15_16_ladder_1;
        float frontier_phi_15_16_ladder_2;
        if ((asuint(_658) & 2139095040u) == 2139095040u)
        {
            frontier_phi_15_16_ladder = 0.0f;
            frontier_phi_15_16_ladder_1 = 0.0f;
            frontier_phi_15_16_ladder_2 = 0.0f;
        }
        else
        {
            bool _831 = (asuint(_660) & 2139095040u) == 2139095040u;
            frontier_phi_15_16_ladder = _831 ? 0.0f : _656;
            frontier_phi_15_16_ladder_1 = _831 ? 0.0f : _660;
            frontier_phi_15_16_ladder_2 = _831 ? 0.0f : _658;
        }
        _778 = frontier_phi_15_16_ladder;
        _780 = frontier_phi_15_16_ladder_2;
        _782 = frontier_phi_15_16_ladder_1;
    }
    SV_Target.x = _778;
    SV_Target.y = _780;
    SV_Target.z = _782;
    SV_Target.w = 1.0f;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    gl_FragCoord = stage_input.gl_FragCoord;
    gl_FragCoord.w = 1.0 / gl_FragCoord.w;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.SV_Target = SV_Target;
    stage_output.SV_Target_3 = SV_Target_3;
    return stage_output;
}
