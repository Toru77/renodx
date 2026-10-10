static uint _8229;
static uint _8230;
static uint _8231;
static uint _8239;
static uint _8240;
static uint _8339;
static uint _8340;
static uint _8469;
static uint _8470;
static uint _8717;
static uint _8718;

cbuffer _55_57 : register(b2, space0)
{
    float4 _57_m0[700] : packoffset(c0);
};

cbuffer _60_62 : register(b0, space0)
{
    float4 _62_m0[1165] : packoffset(c0);
};

Buffer<uint4> _8 : register(t96, space0);
Buffer<uint4> _9 : register(t97, space0);
Buffer<uint4> _10 : register(t98, space0);
Texture2D<float4> _15[] : register(t0, space6);
Texture3D<float4> _19[] : register(t0, space7);
Texture3D<float4> _21 : register(t123, space0);
TextureCube<float4> _24 : register(t118, space0);
Texture2D<float4> _27[] : register(t0, space37);
Texture2D<uint4> _31[] : register(t0, space38);
TextureCube<float4> _34[] : register(t0, space41);
Texture2D<float4> _37[] : register(t0, space1);
Texture2DArray<float4> _41[] : register(t0, space4);
Texture3D<uint4> _44 : register(t0, space12);
Buffer<uint4> _45 : register(t1, space12);
Buffer<uint4> _46 : register(t2, space12);
Buffer<uint4> _47 : register(t3, space12);
Buffer<uint4> _48 : register(t9, space12);
Texture2DArray<float4> _50 : register(t4, space12);
Texture3D<float4> _51 : register(t124, space0);
SamplerState _65 : register(s0, space0);
SamplerState _66 : register(s2, space0);
SamplerState _67 : register(s5, space0);
SamplerState _68 : register(s1, space0);
SamplerState _69 : register(s3, space0);
SamplerState _70 : register(s11, space0);

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

static uint _79[26];
static uint _80[26];
static uint _81[26];
static uint _82[26];

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
    _79[0u] = 2097152u;
    _80[0u] = 0u;
    _81[0u] = 4228890621u;
    _82[0u] = 4294963171u;
    _79[1u] = 67108864u;
    _80[1u] = 0u;
    _81[1u] = 4226793471u;
    _82[1u] = 4294967295u;
    _79[2u] = 0u;
    _80[2u] = 0u;
    _81[2u] = 4294950911u;
    _82[2u] = 4294967295u;
    _79[3u] = 0u;
    _80[3u] = 0u;
    _81[3u] = 4294950911u;
    _82[3u] = 4294967295u;
    _79[4u] = 134234112u;
    _80[4u] = 512u;
    _81[4u] = 4127178751u;
    _82[4u] = 4294965755u;
    _79[5u] = 134234112u;
    _80[5u] = 512u;
    _81[5u] = 4127178751u;
    _82[5u] = 4294963711u;
    _79[6u] = 0u;
    _80[6u] = 0u;
    _81[6u] = 4294950901u;
    _82[6u] = 4294963167u;
    _79[7u] = 512u;
    _80[7u] = 0u;
    _81[7u] = 4294950389u;
    _82[7u] = 4294963167u;
    _79[8u] = 49228u;
    _80[8u] = 256u;
    _81[8u] = 4294852529u;
    _82[8u] = 4294962943u;
    _79[9u] = 180300u;
    _80[9u] = 0u;
    _81[9u] = 4294721201u;
    _82[9u] = 4294963199u;
    _79[10u] = 33092u;
    _80[10u] = 0u;
    _81[10u] = 4294590129u;
    _82[10u] = 4294963071u;
    _79[11u] = 2072u;
    _80[11u] = 0u;
    _81[11u] = 4294965221u;
    _82[11u] = 4294963199u;
    _79[12u] = 2072u;
    _80[12u] = 1u;
    _81[12u] = 4294965221u;
    _82[12u] = 4294963198u;
    _79[13u] = 2072u;
    _80[13u] = 2u;
    _81[13u] = 4294965221u;
    _82[13u] = 4294963197u;
    _79[14u] = 2147485784u;
    _80[14u] = 0u;
    _81[14u] = 2147481509u;
    _82[14u] = 4294963199u;
    _79[15u] = 0u;
    _80[15u] = 64u;
    _81[15u] = 4294950901u;
    _82[15u] = 4294963103u;
    _79[16u] = 27279360u;
    _80[16u] = 8212u;
    _81[16u] = 4267687933u;
    _82[16u] = 4294954987u;
    _79[17u] = 0u;
    _80[17u] = 0u;
    _81[17u] = 467664896u;
    _82[17u] = 4294962712u;
    _79[18u] = 0u;
    _80[18u] = 0u;
    _81[18u] = 159383552u;
    _82[18u] = 4294962688u;
    _79[19u] = 0u;
    _80[19u] = 2097152u;
    _81[19u] = 4294967292u;
    _82[19u] = 4292849631u;
    _79[20u] = 0u;
    _80[20u] = 0u;
    _81[20u] = 4292853756u;
    _82[20u] = 4288638927u;
    _79[21u] = 0u;
    _80[21u] = 8388608u;
    _81[21u] = 4292853756u;
    _82[21u] = 3212767183u;
    _79[22u] = 0u;
    _80[22u] = 0u;
    _81[22u] = 4292853756u;
    _82[22u] = 1023274975u;
    _79[23u] = 0u;
    _80[23u] = 0u;
    _81[23u] = 4292853756u;
    _82[23u] = 3206426575u;
    _79[24u] = 0u;
    _80[24u] = 0u;
    _81[24u] = 4292853756u;
    _82[24u] = 941109199u;
    _79[25u] = 0u;
    _80[25u] = 0u;
    _81[25u] = 2145367972u;
    _82[25u] = 403713740u;
    float _283 = floor(gl_FragCoord.x);
    float _284 = floor(gl_FragCoord.y);
    uint _285 = uint(_283);
    uint _286 = uint(_284);
    float4 _290 = _27[0u].Load(int3(uint2(_285, _286), 0u));
    float _293 = _290.x;
    float _296 = _283 + 0.5f;
    float _298 = _284 + 0.5f;
    float _313 = (((float(_285) + 0.5f) / _57_m0[691u].x) * 2.0f) + (-1.0f);
    float _315 = 1.0f - (((float(_286) + 0.5f) / _57_m0[691u].y) * 2.0f);
    float _360 = mad(_293, _62_m0[1139u].z, mad(_315, _62_m0[1139u].y, _62_m0[1139u].x * _313)) + _62_m0[1139u].w;
    float _361 = (mad(_293, _62_m0[1136u].z, mad(_315, _62_m0[1136u].y, _62_m0[1136u].x * _313)) + _62_m0[1136u].w) / _360;
    float _362 = (mad(_293, _62_m0[1137u].z, mad(_315, _62_m0[1137u].y, _62_m0[1137u].x * _313)) + _62_m0[1137u].w) / _360;
    float _363 = (mad(_293, _62_m0[1138u].z, mad(_315, _62_m0[1138u].y, _62_m0[1138u].x * _313)) + _62_m0[1138u].w) / _360;
    float _369 = _57_m0[8u].x + _361;
    float _370 = _57_m0[8u].y + _362;
    float _371 = _57_m0[8u].z + _363;
    uint4 _375 = _31[2u].Load(int3(uint2(_285, _286), 0u));
    uint _377 = _375.x;
    uint _378 = _375.w;
    uint4 _383 = _31[15u].Load(int3(uint2(_285, _286), 0u));
    uint _385 = _383.y;
    uint _394 = ((_385 & 64u) != 0u) ? uint((_385 & 4294967167u) != 66u) : 4294967295u;
    uint _395 = _378 & 128u;
    uint _398 = (_395 != 0u) ? 1u : ((_377 << 7u) | _378);
    uint4 _401 = _8.Load(_398 * 4u);
    uint _402 = _401.x;
    uint4 _405 = _8.Load((_398 * 4u) + 1u);
    uint _406 = _405.x;
    uint4 _413 = _8.Load((_398 * 4u) + 3u);
    uint _414 = _413.x;
    uint _417 = ((_402 & 1u) != 0u) ? 0u : 18u;
    uint _419 = uint(min(int(uint(max(int(_394), int(0u)))), int(1u)));
    uint _431;
    uint _432;
    if (_395 == 0u)
    {
        _431 = (((_402 & 2097152u) != 0u) && (_394 == _419)) ? (_417 | 128u) : _417;
        _432 = _402;
    }
    else
    {
        _431 = _378;
        _432 = _402 | ((_377 << 20u) & 134217728u);
    }
    float _452 = asfloat(_10.Load((_414 * 115u) + 1u).x);
    float _457 = asfloat(_10.Load((_414 * 115u) + 5u).x);
    float _462 = asfloat(_10.Load((_414 * 115u) + 6u).x);
    uint _464 = (_414 * 115u) + 7u;
    float3 _476 = asfloat(uint3(_10.Load(_464).x, _10.Load(_464 + 1u).x, _10.Load(_464 + 2u).x));
    uint4 _498 = _10.Load((_414 * 115u) + 104u);
    uint _499 = _498.x;
    float _505 = asfloat(_10.Load((_414 * 115u) + 105u).x);
    float _768;
    float _770;
    float _772;
    float _774;
    float _776;
    float _778;
    float _780;
    float _782;
    float _784;
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
    float _810;
    float _812;
    float _814;
    float _816;
    uint _818;
    float _819;
    float _820;
    float _821;
    float _822;
    float _827;
    float _832;
    float _837;
    float _840;
    float _843;
    float _846;
    float _848;
    float _850;
    float _855;
    float _860;
    float _865;
    float _870;
    float _873;
    float _876;
    float _882;
    float _887;
    float _888;
    float _892;
    float _894;
    float _897;
    float _900;
    float _903;
    float _906;
    float _908;
    float _911;
    uint _913;
    float _915;
    float _917;
    float _919;
    float _921;
    float _923;
    float _925;
    float _927;
    float _929;
    float _931;
    float _933;
    float _935;
    uint _937;
    float _938;
    float _940;
    float _942;
    float _944;
    float _946;
    float _948;
    float _950;
    float _952;
    float _954;
    float _956;
    float _958;
    float _960;
    float _962;
    uint _964;
    uint _966;
    uint _968;
    float _970;
    float _972;
    float _974;
    float _976;
    float _978;
    float _980;
    float _982;
    float _984;
    float _986;
    uint _988;
    uint _989;
    if (((_432 & 1u) | (_406 & 1032192u)) == 0u)
    {
        uint4 _519 = _31[2u].Load(int3(uint2(_285, _286), 0u));
        uint _522 = _519.z;
        uint _523 = _431 & 128u;
        bool _524 = _523 == 0u;
        float _616;
        uint _617;
        uint _618;
        if (_524)
        {
            _616 = 0.0f;
            _617 = _432 >> 4u;
            _618 = 0u;
        }
        else
        {
            _616 = float(_519.x & 127u) * 0.0078740157186985015869140625f;
            _617 = _431;
            _618 = 1u;
        }
        precise float _626 = float(_522 & 127u) * 0.0078740157186985015869140625f;
        precise float _627 = float(_519.y & 127u) * 0.0078740157186985015869140625f;
        float4 _630 = _27[3u].Load(int3(uint2(_285, _286), 0u));
        float _632 = _630.x;
        float _633 = _630.y;
        float _634 = _630.z;
        float _635 = _630.w;
        uint4 _638 = _31[1u].Load(int3(uint2(_285, _286), 0u));
        uint _640 = _638.x;
        float4 _643 = _27[8u].Load(int3(uint2(_285, _286), 0u));
        float _645 = _643.x;
        float _646 = _643.y;
        uint _999;
        if ((_523 | (_617 & 1u)) == 0u)
        {
            _999 = 0u;
        }
        else
        {
            _999 = _31[9u].Load(int3(uint2(_285, _286), 0u)).x;
        }
        uint _1185;
        if (_618 == 0u)
        {
            _1185 = 0u;
        }
        else
        {
            _1185 = _31[10u].Load(int3(uint2(_285, _286), 0u)).x;
        }
        float _1195 = (float((_640 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _1196 = (float(_640 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _1200 = (1.0f - abs(_1195)) - abs(_1196);
        float _1202 = clamp((-0.0f) - _1200, 0.0f, 1.0f);
        float _1203 = (-0.0f) - _1202;
        float _1208 = ((_1195 >= 0.0f) ? _1203 : _1202) + _1195;
        float _1209 = ((_1196 >= 0.0f) ? _1203 : _1202) + _1196;
        float _1213 = rsqrt(dot(float3(_1208, _1209, _1200), float3(_1208, _1209, _1200)));
        float _826 = _1208 * _1213;
        float _831 = _1209 * _1213;
        float _836 = _1213 * _1200;
        float _881 = float(_640 & 255u) * 0.0039215688593685626983642578125f;
        float _1357;
        float _1358;
        float _1361;
        float _1363;
        float _1365;
        if (_524)
        {
            _1357 = _635;
            _1358 = 0.039999999105930328369140625f;
            _1361 = 0.039999999105930328369140625f;
            _1363 = 0.039999999105930328369140625f;
            _1365 = 0.039999999105930328369140625f;
        }
        else
        {
            float _1366 = ((_431 & 32u) != 0u) ? _635 : 0.039999999105930328369140625f;
            float frontier_phi_30_31_ladder;
            float frontier_phi_30_31_ladder_1;
            float frontier_phi_30_31_ladder_2;
            float frontier_phi_30_31_ladder_3;
            float frontier_phi_30_31_ladder_4;
            if ((_431 & 1u) == 0u)
            {
                frontier_phi_30_31_ladder = _1366;
                frontier_phi_30_31_ladder_1 = spvUnpackHalf2x16((_999 >> 17u) & 32736u).x;
                frontier_phi_30_31_ladder_2 = spvUnpackHalf2x16((_999 >> 7u) & 32752u).x;
                frontier_phi_30_31_ladder_3 = spvUnpackHalf2x16((_999 << 4u) & 32752u).x;
                frontier_phi_30_31_ladder_4 = 0.0f;
            }
            else
            {
                frontier_phi_30_31_ladder = _1366;
                frontier_phi_30_31_ladder_1 = 0.039999999105930328369140625f;
                frontier_phi_30_31_ladder_2 = 0.039999999105930328369140625f;
                frontier_phi_30_31_ladder_3 = 0.039999999105930328369140625f;
                frontier_phi_30_31_ladder_4 = 0.0f;
            }
            _1357 = frontier_phi_30_31_ladder_4;
            _1358 = frontier_phi_30_31_ladder_3;
            _1361 = frontier_phi_30_31_ladder_2;
            _1363 = frontier_phi_30_31_ladder_1;
            _1365 = frontier_phi_30_31_ladder;
        }
        bool _1368 = (_431 & 64u) != 0u;
        uint _1369 = _432 & 64u;
        float _934;
        float _1560;
        if (_1369 == 0u)
        {
            _1560 = 1.0f;
            _934 = 0.0f;
        }
        else
        {
            float frontier_phi_37_38_ladder;
            float frontier_phi_37_38_ladder_1;
            if (int(_432) < int(0u))
            {
                float _1722 = _645 * 2.0f;
                frontier_phi_37_38_ladder = clamp(_1722 + (-1.0f), 0.0f, 1.0f);
                frontier_phi_37_38_ladder_1 = clamp(_1722, 0.0f, 1.0f);
            }
            else
            {
                frontier_phi_37_38_ladder = 0.0f;
                frontier_phi_37_38_ladder_1 = _645;
            }
            _1560 = frontier_phi_37_38_ladder_1;
            _934 = frontier_phi_37_38_ladder;
        }
        float _936 = ((_406 & 3u) != 0u) ? _645 : 0.0f;
        float _893 = ((_432 & 8u) != 0u) ? _646 : 1.0f;
        float _807;
        float _809;
        float _811;
        float _1812;
        float _1814;
        uint _1816;
        if ((_431 & 1u) == 0u)
        {
            uint frontier_phi_51_43_ladder;
            float frontier_phi_51_43_ladder_1;
            float frontier_phi_51_43_ladder_2;
            float frontier_phi_51_43_ladder_3;
            float frontier_phi_51_43_ladder_4;
            float frontier_phi_51_43_ladder_5;
            if ((_432 & 16u) == 0u)
            {
                frontier_phi_51_43_ladder = 0u;
                frontier_phi_51_43_ladder_1 = 0.0f;
                frontier_phi_51_43_ladder_2 = 0.0f;
                frontier_phi_51_43_ladder_3 = 0.0f;
                frontier_phi_51_43_ladder_4 = 0.0f;
                frontier_phi_51_43_ladder_5 = 0.0f;
            }
            else
            {
                float _1828 = (float(_999 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                float _1829 = (float(_999 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                float _1833 = (1.0f - abs(_1828)) - abs(_1829);
                float _1835 = clamp((-0.0f) - _1833, 0.0f, 1.0f);
                float _1836 = (-0.0f) - _1835;
                float _1841 = ((_1828 >= 0.0f) ? _1836 : _1835) + _1828;
                float _1842 = ((_1829 >= 0.0f) ? _1836 : _1835) + _1829;
                float _1846 = rsqrt(dot(float3(_1841, _1842, _1833), float3(_1841, _1842, _1833)));
                frontier_phi_51_43_ladder = 1u;
                frontier_phi_51_43_ladder_1 = _1846 * _1833;
                frontier_phi_51_43_ladder_2 = _1841 * _1846;
                frontier_phi_51_43_ladder_3 = 0.0f;
                frontier_phi_51_43_ladder_4 = 0.0f;
                frontier_phi_51_43_ladder_5 = 0.0f;
            }
            _807 = frontier_phi_51_43_ladder_5;
            _809 = frontier_phi_51_43_ladder_4;
            _811 = frontier_phi_51_43_ladder_3;
            _1812 = frontier_phi_51_43_ladder_2;
            _1814 = frontier_phi_51_43_ladder_1;
            _1816 = frontier_phi_51_43_ladder;
        }
        else
        {
            _807 = spvUnpackHalf2x16((_999 << 4u) & 32752u).x;
            _809 = spvUnpackHalf2x16((_999 >> 7u) & 32752u).x;
            _811 = spvUnpackHalf2x16((_999 >> 17u) & 32736u).x;
            _1812 = 0.0f;
            _1814 = 0.0f;
            _1816 = 0u;
        }
        float _813;
        float _815;
        float _817;
        float _839;
        float _842;
        float _845;
        if ((_431 & 65u) == 0u)
        {
            float frontier_phi_62_57_ladder;
            float frontier_phi_62_57_ladder_1;
            float frontier_phi_62_57_ladder_2;
            float frontier_phi_62_57_ladder_3;
            float frontier_phi_62_57_ladder_4;
            float frontier_phi_62_57_ladder_5;
            if (_618 == 0u)
            {
                frontier_phi_62_57_ladder = _826;
                frontier_phi_62_57_ladder_1 = _831;
                frontier_phi_62_57_ladder_2 = _836;
                frontier_phi_62_57_ladder_3 = 0.0f;
                frontier_phi_62_57_ladder_4 = 0.0f;
                frontier_phi_62_57_ladder_5 = 0.0f;
            }
            else
            {
                float2 _2154 = spvUnpackHalf2x16((_1185 >> 17u) & 32736u);
                float _2155 = _2154.x;
                float _2158 = (spvUnpackHalf2x16((_1185 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                float _2159 = (spvUnpackHalf2x16((_1185 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                float _2163 = (1.0f - abs(_2158)) - abs(_2159);
                float _2165 = clamp((-0.0f) - _2163, 0.0f, 1.0f);
                float _2166 = (-0.0f) - _2165;
                float _2171 = ((_2158 >= 0.0f) ? _2166 : _2165) + _2158;
                float _2172 = ((_2159 >= 0.0f) ? _2166 : _2165) + _2159;
                float _2176 = rsqrt(dot(float3(_2171, _2172, _2163), float3(_2171, _2172, _2163)));
                float _2186 = (((_2171 * _2176) - _826) * _2155) + _826;
                float _2187 = (((_2172 * _2176) - _831) * _2155) + _831;
                float _2188 = (((_2176 * _2163) - _836) * _2155) + _836;
                float _2192 = rsqrt(dot(float3(_2186, _2187, _2188), float3(_2186, _2187, _2188)));
                frontier_phi_62_57_ladder = _2186 * _2192;
                frontier_phi_62_57_ladder_1 = _2187 * _2192;
                frontier_phi_62_57_ladder_2 = _2188 * _2192;
                frontier_phi_62_57_ladder_3 = 0.0f;
                frontier_phi_62_57_ladder_4 = 0.0f;
                frontier_phi_62_57_ladder_5 = 0.0f;
            }
            _839 = frontier_phi_62_57_ladder;
            _842 = frontier_phi_62_57_ladder_1;
            _845 = frontier_phi_62_57_ladder_2;
            _813 = frontier_phi_62_57_ladder_3;
            _815 = frontier_phi_62_57_ladder_4;
            _817 = frontier_phi_62_57_ladder_5;
        }
        else
        {
            _839 = _826;
            _842 = _831;
            _845 = _836;
            _813 = spvUnpackHalf2x16((_1185 << 4u) & 32752u).x;
            _815 = spvUnpackHalf2x16((_1185 >> 7u) & 32752u).x;
            _817 = spvUnpackHalf2x16((_1185 >> 17u) & 32736u).x;
        }
        bool _2138 = _1816 != 0u;
        bool _2139 = asuint(_57_m0[176u]).z != 0u;
        float _2142 = 1.0f - _1357;
        float _943 = _2142 * _632;
        float _945 = _2142 * _633;
        float _947 = _2142 * _634;
        float _2230;
        float _2232;
        float _2234;
        if (_452 != 0.0f)
        {
            float _2227 = max(max(_632, _633), _634);
            float frontier_phi_68_67_ladder;
            float frontier_phi_68_67_ladder_1;
            float frontier_phi_68_67_ladder_2;
            if (_2227 > 0.001000000047497451305389404296875f)
            {
                float _2285 = _452 / min(_452, _2227);
                frontier_phi_68_67_ladder = _2285 * _634;
                frontier_phi_68_67_ladder_1 = _2285 * _633;
                frontier_phi_68_67_ladder_2 = _2285 * _632;
            }
            else
            {
                frontier_phi_68_67_ladder = _452;
                frontier_phi_68_67_ladder_1 = _452;
                frontier_phi_68_67_ladder_2 = _452;
            }
            _2230 = frontier_phi_68_67_ladder_2;
            _2232 = frontier_phi_68_67_ladder_1;
            _2234 = frontier_phi_68_67_ladder;
        }
        else
        {
            _2230 = _632;
            _2232 = _633;
            _2234 = _634;
        }
        float _869;
        float _872;
        float _875;
        if ((_431 & 32u) == 0u)
        {
            _869 = ((_2230 - _1358) * _1357) + _1358;
            _872 = ((_2232 - _1361) * _1357) + _1361;
            _875 = ((_2234 - _1363) * _1357) + _1363;
        }
        else
        {
            _869 = _1365;
            _872 = _1365;
            _875 = _1365;
        }
        float _896;
        if ((_432 & 2048u) == 0u)
        {
            _896 = _893;
        }
        else
        {
            float frontier_phi_80_81_ladder;
            if (_57_m0[88u].w > 0.0f)
            {
                float _2429 = clamp((_881 - _57_m0[88u].x) / (_57_m0[88u].y - _57_m0[88u].x), 0.0f, 1.0f);
                frontier_phi_80_81_ladder = exp2(log2((_2429 * _2429) * (3.0f - (_2429 * 2.0f))) * _57_m0[88u].z) * _893;
            }
            else
            {
                frontier_phi_80_81_ladder = _893;
            }
            _896 = frontier_phi_80_81_ladder;
        }
        bool _2321 = _1369 != 0u;
        float _907;
        float _910;
        float _912;
        if (int(_432) < int(0u))
        {
            _907 = _57_m0[153u].x;
            _910 = _57_m0[153u].x;
            _912 = _57_m0[153u].y * _1560;
        }
        else
        {
            _907 = asfloat(_10.Load((_414 * 115u) + 10u).x);
            _910 = asfloat(_10.Load((_414 * 115u) + 11u).x);
            _912 = _1560;
        }
        float _854;
        float _859;
        float _864;
        float _939;
        float _941;
        if ((_406 & 1u) == 0u)
        {
            _854 = _943;
            _859 = _945;
            _864 = _947;
            _939 = 0.0f;
            _941 = ((_406 & 2u) != 0u) ? _936 : 0.0f;
        }
        else
        {
            float _2586 = clamp((dot(float3(_943, _945, _947), 1.0f.xxx) + (-1.7000000476837158203125f)) * 2.0f, 0.0f, 1.0f) * _936;
            _854 = (_2586 * (0.839999973773956298828125f - _943)) + _943;
            _859 = (_2586 * (0.87999999523162841796875f - _945)) + _945;
            _864 = (_2586 * (0.980000019073486328125f - _947)) + _947;
            _939 = _936;
            _941 = 0.0f;
        }
        _768 = 0.0f;
        _770 = 0.0f;
        _772 = 0.0f;
        _774 = 0.0f;
        _776 = 0.0f;
        _778 = 0.0f;
        _780 = 0.0f;
        _782 = 0.0f;
        _784 = 0.0f;
        _786 = 0.0f;
        _788 = 0.0f;
        _790 = 0.0f;
        _792 = 0.0f;
        _794 = 0.0f;
        _796 = 0.0f;
        _798 = 0.0f;
        _800 = _1368 ? _645 : 0.0f;
        _802 = _1368 ? _646 : 0.0f;
        _804 = _1368 ? _643.z : 0.0f;
        _806 = _807;
        _808 = _809;
        _810 = _811;
        _812 = _813;
        _814 = _815;
        _816 = _817;
        _818 = (_522 >> 7u) & 1u;
        _819 = _369;
        _820 = _370;
        _821 = _371;
        _822 = _826;
        _827 = _831;
        _832 = _836;
        _837 = _839;
        _840 = _842;
        _843 = _845;
        _846 = (_2139 && _2138) ? _1812 : _839;
        _848 = (_2139 && _2138) ? _1814 : _845;
        _850 = _854;
        _855 = _859;
        _860 = _864;
        _865 = _869;
        _870 = _872;
        _873 = _875;
        _876 = _881;
        _882 = _626;
        _887 = _643.w;
        _888 = _627;
        _892 = _893;
        _894 = _896;
        _897 = (_2321 ? (_943 * _476.x) : 0.0f) * _1560;
        _900 = (_2321 ? (_945 * _476.y) : 0.0f) * _1560;
        _903 = (_2321 ? (_947 * _476.z) : 0.0f) * _1560;
        _906 = _907;
        _908 = _910;
        _911 = _912;
        _913 = 0u;
        _915 = 0.0f;
        _917 = 0.0f;
        _919 = 0.0f;
        _921 = 0.0f;
        _923 = 0.0f;
        _925 = 0.0f;
        _927 = 0.0f;
        _929 = 0.0f;
        _931 = _524 ? 0.0f : _616;
        _933 = _934;
        _935 = _936;
        _937 = _10.Load((_414 * 115u) + 60u).x;
        _938 = _939;
        _940 = _941;
        _942 = _943;
        _944 = _945;
        _946 = _947;
        _948 = 1.0f - _881;
        _950 = 0.0f;
        _952 = 0.0f;
        _954 = 0.0f;
        _956 = 0.0f;
        _958 = 0.0f;
        _960 = 0.0f;
        _962 = 0.0f;
        _964 = 0u;
        _966 = 0u;
        _968 = 0u;
        _970 = 0.0f;
        _972 = 0.0f;
        _974 = 0.0f;
        _976 = 0.0f;
        _978 = 0.0f;
        _980 = 0.0f;
        _982 = 0.0f;
        _984 = 0.0f;
        _986 = 0.0f;
        _988 = _432;
        _989 = _406;
    }
    else
    {
        float frontier_phi_13_5_ladder;
        float frontier_phi_13_5_ladder_1;
        uint frontier_phi_13_5_ladder_2;
        float frontier_phi_13_5_ladder_3;
        float frontier_phi_13_5_ladder_4;
        float frontier_phi_13_5_ladder_5;
        float frontier_phi_13_5_ladder_6;
        float frontier_phi_13_5_ladder_7;
        float frontier_phi_13_5_ladder_8;
        float frontier_phi_13_5_ladder_9;
        float frontier_phi_13_5_ladder_10;
        float frontier_phi_13_5_ladder_11;
        float frontier_phi_13_5_ladder_12;
        float frontier_phi_13_5_ladder_13;
        float frontier_phi_13_5_ladder_14;
        float frontier_phi_13_5_ladder_15;
        float frontier_phi_13_5_ladder_16;
        float frontier_phi_13_5_ladder_17;
        float frontier_phi_13_5_ladder_18;
        float frontier_phi_13_5_ladder_19;
        float frontier_phi_13_5_ladder_20;
        float frontier_phi_13_5_ladder_21;
        float frontier_phi_13_5_ladder_22;
        float frontier_phi_13_5_ladder_23;
        float frontier_phi_13_5_ladder_24;
        float frontier_phi_13_5_ladder_25;
        float frontier_phi_13_5_ladder_26;
        float frontier_phi_13_5_ladder_27;
        float frontier_phi_13_5_ladder_28;
        float frontier_phi_13_5_ladder_29;
        float frontier_phi_13_5_ladder_30;
        float frontier_phi_13_5_ladder_31;
        float frontier_phi_13_5_ladder_32;
        float frontier_phi_13_5_ladder_33;
        float frontier_phi_13_5_ladder_34;
        float frontier_phi_13_5_ladder_35;
        float frontier_phi_13_5_ladder_36;
        float frontier_phi_13_5_ladder_37;
        float frontier_phi_13_5_ladder_38;
        float frontier_phi_13_5_ladder_39;
        float frontier_phi_13_5_ladder_40;
        float frontier_phi_13_5_ladder_41;
        float frontier_phi_13_5_ladder_42;
        float frontier_phi_13_5_ladder_43;
        float frontier_phi_13_5_ladder_44;
        float frontier_phi_13_5_ladder_45;
        float frontier_phi_13_5_ladder_46;
        float frontier_phi_13_5_ladder_47;
        uint frontier_phi_13_5_ladder_48;
        float frontier_phi_13_5_ladder_49;
        float frontier_phi_13_5_ladder_50;
        float frontier_phi_13_5_ladder_51;
        float frontier_phi_13_5_ladder_52;
        float frontier_phi_13_5_ladder_53;
        float frontier_phi_13_5_ladder_54;
        float frontier_phi_13_5_ladder_55;
        float frontier_phi_13_5_ladder_56;
        float frontier_phi_13_5_ladder_57;
        uint frontier_phi_13_5_ladder_58;
        uint frontier_phi_13_5_ladder_59;
        float frontier_phi_13_5_ladder_60;
        float frontier_phi_13_5_ladder_61;
        float frontier_phi_13_5_ladder_62;
        float frontier_phi_13_5_ladder_63;
        float frontier_phi_13_5_ladder_64;
        float frontier_phi_13_5_ladder_65;
        float frontier_phi_13_5_ladder_66;
        float frontier_phi_13_5_ladder_67;
        float frontier_phi_13_5_ladder_68;
        float frontier_phi_13_5_ladder_69;
        uint frontier_phi_13_5_ladder_70;
        uint frontier_phi_13_5_ladder_71;
        float frontier_phi_13_5_ladder_72;
        float frontier_phi_13_5_ladder_73;
        float frontier_phi_13_5_ladder_74;
        float frontier_phi_13_5_ladder_75;
        float frontier_phi_13_5_ladder_76;
        float frontier_phi_13_5_ladder_77;
        float frontier_phi_13_5_ladder_78;
        float frontier_phi_13_5_ladder_79;
        float frontier_phi_13_5_ladder_80;
        uint frontier_phi_13_5_ladder_81;
        float frontier_phi_13_5_ladder_82;
        float frontier_phi_13_5_ladder_83;
        float frontier_phi_13_5_ladder_84;
        float frontier_phi_13_5_ladder_85;
        float frontier_phi_13_5_ladder_86;
        float frontier_phi_13_5_ladder_87;
        float frontier_phi_13_5_ladder_88;
        float frontier_phi_13_5_ladder_89;
        float frontier_phi_13_5_ladder_90;
        float frontier_phi_13_5_ladder_91;
        uint frontier_phi_13_5_ladder_92;
        float frontier_phi_13_5_ladder_93;
        float frontier_phi_13_5_ladder_94;
        if ((_406 & 16384u) == 0u)
        {
            float frontier_phi_13_5_ladder_8_ladder;
            float frontier_phi_13_5_ladder_8_ladder_1;
            uint frontier_phi_13_5_ladder_8_ladder_2;
            float frontier_phi_13_5_ladder_8_ladder_3;
            float frontier_phi_13_5_ladder_8_ladder_4;
            float frontier_phi_13_5_ladder_8_ladder_5;
            float frontier_phi_13_5_ladder_8_ladder_6;
            float frontier_phi_13_5_ladder_8_ladder_7;
            float frontier_phi_13_5_ladder_8_ladder_8;
            float frontier_phi_13_5_ladder_8_ladder_9;
            float frontier_phi_13_5_ladder_8_ladder_10;
            float frontier_phi_13_5_ladder_8_ladder_11;
            float frontier_phi_13_5_ladder_8_ladder_12;
            float frontier_phi_13_5_ladder_8_ladder_13;
            float frontier_phi_13_5_ladder_8_ladder_14;
            float frontier_phi_13_5_ladder_8_ladder_15;
            float frontier_phi_13_5_ladder_8_ladder_16;
            float frontier_phi_13_5_ladder_8_ladder_17;
            float frontier_phi_13_5_ladder_8_ladder_18;
            float frontier_phi_13_5_ladder_8_ladder_19;
            float frontier_phi_13_5_ladder_8_ladder_20;
            float frontier_phi_13_5_ladder_8_ladder_21;
            float frontier_phi_13_5_ladder_8_ladder_22;
            float frontier_phi_13_5_ladder_8_ladder_23;
            float frontier_phi_13_5_ladder_8_ladder_24;
            float frontier_phi_13_5_ladder_8_ladder_25;
            float frontier_phi_13_5_ladder_8_ladder_26;
            float frontier_phi_13_5_ladder_8_ladder_27;
            float frontier_phi_13_5_ladder_8_ladder_28;
            float frontier_phi_13_5_ladder_8_ladder_29;
            float frontier_phi_13_5_ladder_8_ladder_30;
            float frontier_phi_13_5_ladder_8_ladder_31;
            float frontier_phi_13_5_ladder_8_ladder_32;
            float frontier_phi_13_5_ladder_8_ladder_33;
            float frontier_phi_13_5_ladder_8_ladder_34;
            float frontier_phi_13_5_ladder_8_ladder_35;
            float frontier_phi_13_5_ladder_8_ladder_36;
            float frontier_phi_13_5_ladder_8_ladder_37;
            float frontier_phi_13_5_ladder_8_ladder_38;
            float frontier_phi_13_5_ladder_8_ladder_39;
            float frontier_phi_13_5_ladder_8_ladder_40;
            float frontier_phi_13_5_ladder_8_ladder_41;
            float frontier_phi_13_5_ladder_8_ladder_42;
            float frontier_phi_13_5_ladder_8_ladder_43;
            float frontier_phi_13_5_ladder_8_ladder_44;
            float frontier_phi_13_5_ladder_8_ladder_45;
            float frontier_phi_13_5_ladder_8_ladder_46;
            float frontier_phi_13_5_ladder_8_ladder_47;
            uint frontier_phi_13_5_ladder_8_ladder_48;
            float frontier_phi_13_5_ladder_8_ladder_49;
            float frontier_phi_13_5_ladder_8_ladder_50;
            float frontier_phi_13_5_ladder_8_ladder_51;
            float frontier_phi_13_5_ladder_8_ladder_52;
            float frontier_phi_13_5_ladder_8_ladder_53;
            float frontier_phi_13_5_ladder_8_ladder_54;
            float frontier_phi_13_5_ladder_8_ladder_55;
            float frontier_phi_13_5_ladder_8_ladder_56;
            float frontier_phi_13_5_ladder_8_ladder_57;
            uint frontier_phi_13_5_ladder_8_ladder_58;
            uint frontier_phi_13_5_ladder_8_ladder_59;
            float frontier_phi_13_5_ladder_8_ladder_60;
            float frontier_phi_13_5_ladder_8_ladder_61;
            float frontier_phi_13_5_ladder_8_ladder_62;
            float frontier_phi_13_5_ladder_8_ladder_63;
            float frontier_phi_13_5_ladder_8_ladder_64;
            float frontier_phi_13_5_ladder_8_ladder_65;
            float frontier_phi_13_5_ladder_8_ladder_66;
            float frontier_phi_13_5_ladder_8_ladder_67;
            float frontier_phi_13_5_ladder_8_ladder_68;
            float frontier_phi_13_5_ladder_8_ladder_69;
            uint frontier_phi_13_5_ladder_8_ladder_70;
            uint frontier_phi_13_5_ladder_8_ladder_71;
            float frontier_phi_13_5_ladder_8_ladder_72;
            float frontier_phi_13_5_ladder_8_ladder_73;
            float frontier_phi_13_5_ladder_8_ladder_74;
            float frontier_phi_13_5_ladder_8_ladder_75;
            float frontier_phi_13_5_ladder_8_ladder_76;
            float frontier_phi_13_5_ladder_8_ladder_77;
            float frontier_phi_13_5_ladder_8_ladder_78;
            float frontier_phi_13_5_ladder_8_ladder_79;
            float frontier_phi_13_5_ladder_8_ladder_80;
            uint frontier_phi_13_5_ladder_8_ladder_81;
            float frontier_phi_13_5_ladder_8_ladder_82;
            float frontier_phi_13_5_ladder_8_ladder_83;
            float frontier_phi_13_5_ladder_8_ladder_84;
            float frontier_phi_13_5_ladder_8_ladder_85;
            float frontier_phi_13_5_ladder_8_ladder_86;
            float frontier_phi_13_5_ladder_8_ladder_87;
            float frontier_phi_13_5_ladder_8_ladder_88;
            float frontier_phi_13_5_ladder_8_ladder_89;
            float frontier_phi_13_5_ladder_8_ladder_90;
            float frontier_phi_13_5_ladder_8_ladder_91;
            uint frontier_phi_13_5_ladder_8_ladder_92;
            float frontier_phi_13_5_ladder_8_ladder_93;
            float frontier_phi_13_5_ladder_8_ladder_94;
            if ((_406 & 524288u) == 0u)
            {
                float frontier_phi_13_5_ladder_8_ladder_11_ladder;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_1;
                uint frontier_phi_13_5_ladder_8_ladder_11_ladder_2;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_3;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_4;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_5;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_6;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_7;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_8;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_9;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_10;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_11;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_12;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_13;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_14;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_15;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_16;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_17;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_18;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_19;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_20;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_21;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_22;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_23;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_24;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_25;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_26;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_27;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_28;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_29;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_30;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_31;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_32;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_33;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_34;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_35;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_36;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_37;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_38;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_39;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_40;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_41;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_42;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_43;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_44;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_45;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_46;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_47;
                uint frontier_phi_13_5_ladder_8_ladder_11_ladder_48;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_49;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_50;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_51;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_52;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_53;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_54;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_55;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_56;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_57;
                uint frontier_phi_13_5_ladder_8_ladder_11_ladder_58;
                uint frontier_phi_13_5_ladder_8_ladder_11_ladder_59;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_60;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_61;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_62;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_63;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_64;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_65;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_66;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_67;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_68;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_69;
                uint frontier_phi_13_5_ladder_8_ladder_11_ladder_70;
                uint frontier_phi_13_5_ladder_8_ladder_11_ladder_71;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_72;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_73;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_74;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_75;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_76;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_77;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_78;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_79;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_80;
                uint frontier_phi_13_5_ladder_8_ladder_11_ladder_81;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_82;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_83;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_84;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_85;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_86;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_87;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_88;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_89;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_90;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_91;
                uint frontier_phi_13_5_ladder_8_ladder_11_ladder_92;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_93;
                float frontier_phi_13_5_ladder_8_ladder_11_ladder_94;
                if ((_406 & 32768u) == 0u)
                {
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_1;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_2;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_3;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_4;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_5;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_6;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_7;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_8;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_9;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_10;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_11;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_12;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_13;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_14;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_15;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_16;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_17;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_18;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_19;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_20;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_21;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_23;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_24;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_25;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_26;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_27;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_28;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_29;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_30;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_31;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_32;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_33;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_34;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_35;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_36;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_37;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_38;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_39;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_40;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_41;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_42;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_43;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_44;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_45;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_46;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_47;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_48;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_49;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_50;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_51;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_52;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_53;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_54;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_55;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_56;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_57;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_58;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_59;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_60;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_61;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_62;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_63;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_64;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_65;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_66;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_67;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_68;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_69;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_70;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_71;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_72;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_73;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_74;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_75;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_76;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_77;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_78;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_79;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_80;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_81;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_82;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_83;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_84;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_85;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_86;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_87;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_88;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_89;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_90;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_91;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_92;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_93;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_94;
                    if ((_406 & 65536u) == 0u)
                    {
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_1;
                        uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_2;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_3;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_4;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_5;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_6;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_7;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_8;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_9;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_10;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_11;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_12;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_13;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_14;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_15;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_16;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_17;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_18;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_19;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_20;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_21;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_22;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_23;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_24;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_25;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_26;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_27;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_28;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_29;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_30;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_31;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_33;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_34;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_35;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_36;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_37;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_38;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_39;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_40;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_41;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_42;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_43;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_44;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_45;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_46;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_47;
                        uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_48;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_49;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_50;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_51;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_52;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_53;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_54;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_55;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_56;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_57;
                        uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_58;
                        uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_59;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_60;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_61;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_62;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_63;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_64;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_65;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_66;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_67;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_68;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_69;
                        uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_70;
                        uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_71;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_72;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_73;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_74;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_75;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_76;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_77;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_78;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_79;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_80;
                        uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_81;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_82;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_83;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_84;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_85;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_86;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_87;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_88;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_89;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_90;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_91;
                        uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_92;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_93;
                        float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_94;
                        if ((_406 & 262144u) == 0u)
                        {
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_1;
                            uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_2;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_3;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_4;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_5;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_6;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_7;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_8;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_9;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_10;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_11;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_12;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_13;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_14;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_15;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_16;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_17;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_18;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_19;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_20;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_21;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_22;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_23;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_24;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_25;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_26;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_27;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_28;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_29;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_30;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_31;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_32;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_33;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_34;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_35;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_36;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_37;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_38;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_39;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_40;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_41;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_42;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_43;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_44;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_45;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_46;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_47;
                            uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_48;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_49;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_50;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_51;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_52;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_53;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_54;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_55;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_56;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_57;
                            uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_58;
                            uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_59;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_60;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_61;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_62;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_63;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_64;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_65;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_66;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_67;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_68;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_69;
                            uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_70;
                            uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_71;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_72;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_73;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_74;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_75;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_76;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_77;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_78;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_79;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_80;
                            uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_81;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_82;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_83;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_84;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_85;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_86;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_87;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_88;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_89;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_90;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_91;
                            uint frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_92;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_93;
                            float frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_94;
                            if ((_406 & 131072u) == 0u)
                            {
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_1 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_2 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_3 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_4 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_5 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_6 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_7 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_8 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_9 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_10 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_11 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_12 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_13 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_14 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_15 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_16 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_17 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_18 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_19 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_20 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_21 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_22 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_23 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_24 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_25 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_26 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_27 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_28 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_29 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_30 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_31 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_32 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_33 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_34 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_35 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_36 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_37 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_38 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_39 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_40 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_41 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_42 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_43 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_44 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_45 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_46 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_47 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_48 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_49 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_50 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_51 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_52 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_53 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_54 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_55 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_56 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_57 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_58 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_59 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_60 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_61 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_62 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_63 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_64 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_65 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_66 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_67 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_68 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_69 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_70 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_71 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_72 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_73 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_74 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_75 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_76 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_77 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_78 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_79 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_80 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_81 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_82 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_83 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_84 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_85 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_86 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_87 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_88 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_89 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_90 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_91 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_92 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_93 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_94 = 0.0f;
                            }
                            else
                            {
                                uint4 _1584 = _31[2u].Load(int3(uint2(_285, _286), 0u));
                                float4 _1590 = _27[3u].Load(int3(uint2(_285, _286), 0u));
                                float _1592 = _1590.x;
                                float _1593 = _1590.y;
                                float _1594 = _1590.z;
                                uint4 _1598 = _31[1u].Load(int3(uint2(_285, _286), 0u));
                                uint _1600 = _1598.x;
                                uint4 _1603 = _31[10u].Load(int3(uint2(_285, _286), 0u));
                                uint _1605 = _1603.x;
                                bool _1606 = int(_406) < int(0u);
                                float _1736;
                                float _1737;
                                float _1738;
                                uint _1739;
                                if (_1606)
                                {
                                    float4 _1726 = _27[8u].Load(int3(uint2(_285, _286), 0u));
                                    _1736 = _1726.x;
                                    _1737 = _1726.y;
                                    _1738 = _1726.z;
                                    _1739 = _31[9u].Load(int3(uint2(_285, _286), 0u)).x;
                                }
                                else
                                {
                                    _1736 = 0.0f;
                                    _1737 = 0.0f;
                                    _1738 = 0.0f;
                                    _1739 = 0u;
                                }
                                float _1748 = (float((_1600 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _1749 = (float(_1600 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _1753 = (1.0f - abs(_1748)) - abs(_1749);
                                float _1755 = clamp((-0.0f) - _1753, 0.0f, 1.0f);
                                float _1756 = (-0.0f) - _1755;
                                float _1761 = ((_1748 >= 0.0f) ? _1756 : _1755) + _1748;
                                float _1762 = ((_1749 >= 0.0f) ? _1756 : _1755) + _1749;
                                float _1766 = rsqrt(dot(float3(_1761, _1762, _1753), float3(_1761, _1762, _1753)));
                                float _781 = _1761 * _1766;
                                float _775 = _1762 * _1766;
                                float _769 = _1766 * _1753;
                                float _1784 = (float((_1605 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _1785 = (float(_1605 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _1789 = (1.0f - abs(_1784)) - abs(_1785);
                                float _1791 = clamp((-0.0f) - _1789, 0.0f, 1.0f);
                                float _1792 = (-0.0f) - _1791;
                                float _1797 = ((_1784 >= 0.0f) ? _1792 : _1791) + _1784;
                                float _1798 = ((_1785 >= 0.0f) ? _1792 : _1791) + _1785;
                                float _1802 = rsqrt(dot(float3(_1797, _1798, _1789), float3(_1797, _1798, _1789)));
                                float _785 = _1797 * _1802;
                                float _779 = _1798 * _1802;
                                float _773 = _1802 * _1789;
                                float _787;
                                float _789;
                                float _791;
                                float _793;
                                float _795;
                                float _797;
                                if (_1606)
                                {
                                    float _1858 = float(_1739 & 255u) * 0.0039215688593685626983642578125f;
                                    float _1859 = float((_1739 >> 8u) & 255u) * 0.0039215688593685626983642578125f;
                                    float _1860 = float((_1739 >> 16u) & 255u) * 0.0039215688593685626983642578125f;
                                    _787 = _1736 * _1736;
                                    _789 = _1737 * _1737;
                                    _791 = _1738 * _1738;
                                    _793 = _1858 * _1858;
                                    _795 = _1859 * _1859;
                                    _797 = _1860 * _1860;
                                }
                                else
                                {
                                    _787 = 0.0f;
                                    _789 = 0.0f;
                                    _791 = 0.0f;
                                    _793 = 0.0f;
                                    _795 = 0.0f;
                                    _797 = 0.0f;
                                }
                                uint4 _1867 = _10.Load((_414 * 115u) + 85u);
                                uint _1868 = _1867.x;
                                uint _1895 = _1868 + 692u;
                                float _1901 = (_779 * _769) - (_773 * _775);
                                float _1904 = (_773 * _781) - (_785 * _769);
                                float _1907 = (_785 * _775) - (_779 * _781);
                                float _1911 = rsqrt(dot(float3(_1901, _1904, _1907), float3(_1901, _1904, _1907)));
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_1 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_2 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_3 = _369;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_4 = _370;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_5 = _371;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_6 = _781;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_7 = _775;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_8 = _769;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_9 = _781;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_10 = _775;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_11 = _769;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_12 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_13 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_14 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_15 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_16 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_17 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_18 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_19 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_20 = float(_1600 & 255u) * 0.0039215688593685626983642578125f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_21 = float(_1584.y) * 0.0039215688593685626983642578125f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_22 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_23 = float(_1584.z) * 0.0039215688593685626983642578125f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_24 = _789;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_25 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_26 = _769;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_27 = _1911 * _1907;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_28 = _773;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_29 = _775;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_30 = _1911 * _1904;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_31 = _779;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_32 = _781;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_33 = _1911 * _1901;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_34 = _785;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_35 = _787;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_36 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_37 = _791;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_38 = _793;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_39 = _795;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_40 = _797;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_41 = asfloat(_10.Load((_414 * 115u) + 89u).x);
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_42 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_43 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_44 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_45 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_46 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_47 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_48 = _1868;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_49 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_50 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_51 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_52 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_53 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_54 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_55 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_56 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_57 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_58 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_59 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_60 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_61 = ((-0.0f) - _1592) / (_1592 + (-1.0f));
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_62 = ((-0.0f) - _1593) / (_1593 + (-1.0f));
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_63 = ((-0.0f) - _1594) / (_1594 + (-1.0f));
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_64 = (_1590.w * 0.00999999977648258209228515625f) * asfloat(_10.Load((_414 * 115u) + 88u).x);
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_65 = float(_1605 & 255u) * 0.0039215688593685626983642578125f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_66 = asfloat(_10.Load((_414 * 115u) + 86u).x);
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_67 = asfloat(_10.Load((_414 * 115u) + 87u).x);
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_68 = _57_m0[_1895].x;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_69 = _57_m0[_1895].y;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_70 = _432;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_71 = _406;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_72 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_73 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_74 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_75 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_76 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_77 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_78 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_79 = 1.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_80 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_81 = _406 >> 31u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_82 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_83 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_84 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_85 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_86 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_87 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_88 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_89 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_90 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_91 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_92 = 0u;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_93 = 0.0f;
                                frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_94 = 0.0f;
                            }
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_1 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_1;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_2 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_2;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_3 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_3;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_4 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_4;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_5 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_5;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_6 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_6;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_7 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_7;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_8 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_8;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_9 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_9;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_10 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_10;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_11 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_11;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_12 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_12;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_13 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_13;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_14 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_14;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_15 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_15;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_16 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_16;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_17 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_17;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_18 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_18;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_19 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_19;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_20 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_20;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_21 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_21;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_22 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_22;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_23 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_23;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_24 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_24;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_25 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_25;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_26 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_26;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_27 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_27;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_28 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_28;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_29 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_29;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_30 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_30;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_31 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_31;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_32;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_33 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_33;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_34 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_34;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_35 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_35;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_36 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_36;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_37 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_37;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_38 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_38;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_39 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_39;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_40 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_40;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_41 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_41;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_42 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_42;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_43 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_43;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_44 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_44;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_45 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_45;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_46 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_46;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_47 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_47;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_48 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_48;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_49 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_49;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_50 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_50;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_51 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_51;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_52 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_52;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_53 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_53;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_54 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_54;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_55 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_55;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_56 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_56;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_57 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_57;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_58 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_58;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_59 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_59;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_60 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_60;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_61 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_61;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_62 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_62;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_63 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_63;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_64 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_64;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_65 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_65;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_66 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_66;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_67 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_67;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_68 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_68;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_69 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_69;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_70 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_70;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_71 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_71;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_72 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_72;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_73 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_73;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_74 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_74;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_75 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_75;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_76 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_76;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_77 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_77;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_78 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_78;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_79 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_79;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_80 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_80;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_81 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_81;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_82 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_82;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_83 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_83;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_84 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_84;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_85 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_85;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_86 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_86;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_87 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_87;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_88 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_88;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_89 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_89;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_90 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_90;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_91 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_91;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_92 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_92;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_93 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_93;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_94 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32_ladder_94;
                        }
                        else
                        {
                            uint4 _1380 = _31[2u].Load(int3(uint2(_285, _286), 0u));
                            float4 _1386 = _27[3u].Load(int3(uint2(_285, _286), 0u));
                            uint4 _1390 = _31[1u].Load(int3(uint2(_285, _286), 0u));
                            uint _1392 = _1390.x;
                            uint4 _1395 = _31[9u].Load(int3(uint2(_285, _286), 0u));
                            uint _1397 = _1395.x;
                            uint4 _1400 = _31[10u].Load(int3(uint2(_285, _286), 0u));
                            uint _1402 = _1400.x;
                            float _1411 = (float((_1392 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1412 = (float(_1392 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1416 = (1.0f - abs(_1411)) - abs(_1412);
                            float _1418 = clamp((-0.0f) - _1416, 0.0f, 1.0f);
                            float _1419 = (-0.0f) - _1418;
                            float _1424 = ((_1411 >= 0.0f) ? _1419 : _1418) + _1411;
                            float _1425 = ((_1412 >= 0.0f) ? _1419 : _1418) + _1412;
                            float _1429 = rsqrt(dot(float3(_1424, _1425, _1416), float3(_1424, _1425, _1416)));
                            float _884 = float(_1380.y) * 0.0039215688593685626983642578125f;
                            float _1441 = (float((_1397 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1442 = (float(_1397 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1446 = (1.0f - abs(_1441)) - abs(_1442);
                            float _1448 = clamp((-0.0f) - _1446, 0.0f, 1.0f);
                            float _1449 = (-0.0f) - _1448;
                            float _1454 = ((_1441 >= 0.0f) ? _1449 : _1448) + _1441;
                            float _1455 = ((_1442 >= 0.0f) ? _1449 : _1448) + _1442;
                            float _1459 = rsqrt(dot(float3(_1454, _1455, _1446), float3(_1454, _1455, _1446)));
                            float _1469 = (float((_1402 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1470 = (float(_1402 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1474 = (1.0f - abs(_1469)) - abs(_1470);
                            float _1476 = clamp((-0.0f) - _1474, 0.0f, 1.0f);
                            float _1477 = (-0.0f) - _1476;
                            float _1482 = ((_1469 >= 0.0f) ? _1477 : _1476) + _1469;
                            float _1483 = ((_1470 >= 0.0f) ? _1477 : _1476) + _1470;
                            float _1487 = rsqrt(dot(float3(_1482, _1483, _1474), float3(_1482, _1483, _1474)));
                            float _1492 = asfloat(_10.Load(_414 * 115u).x);
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_1 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_2 = 0u;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_3 = _369;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_4 = _370;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_5 = _371;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_6 = _1424 * _1429;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_7 = _1425 * _1429;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_8 = _1429 * _1416;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_9 = _1454 * _1459;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_10 = _1455 * _1459;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_11 = _1459 * _1446;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_12 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_13 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_14 = _1386.x;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_15 = _1386.y;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_16 = _1386.z;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_17 = _1492;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_18 = _1492;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_19 = _1492;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_20 = float(_1392 & 255u) * 0.0039215688593685626983642578125f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_21 = _884;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_22 = 1.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_23 = _884;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_24 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_25 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_26 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_27 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_28 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_29 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_30 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_31 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_33 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_34 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_35 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_36 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_37 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_38 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_39 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_40 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_41 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_42 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_43 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_44 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_45 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_46 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_47 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_48 = 0u;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_49 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_50 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_51 = float(_1397 & 255u) * 0.0039215688593685626983642578125f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_52 = float(_1402 & 255u) * 0.0039215688593685626983642578125f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_53 = _1482 * _1487;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_54 = _1483 * _1487;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_55 = _1487 * _1474;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_56 = _1386.w;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_57 = asfloat(_10.Load((_414 * 115u) + 82u).x);
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_58 = _10.Load((_414 * 115u) + 83u).x;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_59 = _10.Load((_414 * 115u) + 84u).x;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_60 = float(_1380.z) * 0.0039215688593685626983642578125f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_61 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_62 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_63 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_64 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_65 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_66 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_67 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_68 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_69 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_70 = _432;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_71 = _406;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_72 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_73 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_74 = 1.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_75 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_76 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_77 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_78 = 1.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_79 = 1.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_80 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_81 = 1u;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_82 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_83 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_84 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_85 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_86 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_87 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_88 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_89 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_90 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_91 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_92 = 0u;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_93 = 0.0f;
                            frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_94 = 0.0f;
                        }
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_1 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_1;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_2 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_2;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_3 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_3;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_4 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_4;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_5 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_5;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_6 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_6;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_7 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_7;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_8 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_8;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_9 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_9;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_10 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_10;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_11 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_11;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_12 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_12;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_13 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_13;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_14 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_14;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_15 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_15;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_16 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_16;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_17 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_17;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_18 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_18;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_19 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_19;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_20 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_20;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_21 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_21;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_22;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_23 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_23;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_24 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_24;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_25 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_25;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_26 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_26;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_27 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_27;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_28 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_28;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_29 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_29;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_30 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_30;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_31 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_31;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_32 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_32;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_33 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_33;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_34 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_34;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_35 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_35;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_36 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_36;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_37 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_37;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_38 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_38;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_39 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_39;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_40 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_40;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_41 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_41;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_42 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_42;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_43 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_43;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_44 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_44;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_45 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_45;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_46 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_46;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_47 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_47;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_48 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_48;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_49 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_49;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_50 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_50;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_51 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_51;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_52 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_52;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_53 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_53;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_54 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_54;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_55 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_55;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_56 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_56;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_57 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_57;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_58 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_58;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_59 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_59;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_60 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_60;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_61 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_61;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_62 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_62;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_63 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_63;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_64 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_64;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_65 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_65;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_66 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_66;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_67 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_67;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_68 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_68;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_69 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_69;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_70 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_70;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_71 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_71;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_72 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_72;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_73 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_73;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_74 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_74;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_75 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_75;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_76 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_76;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_77 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_77;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_78 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_78;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_79 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_79;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_80 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_80;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_81 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_81;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_82 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_82;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_83 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_83;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_84 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_84;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_85 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_85;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_86 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_86;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_87 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_87;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_88 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_88;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_89 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_89;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_90 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_90;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_91 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_91;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_92 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_92;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_93 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_93;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_94 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22_ladder_94;
                    }
                    else
                    {
                        uint4 _1224 = _31[2u].Load(int3(uint2(_285, _286), 0u));
                        float4 _1230 = _27[3u].Load(int3(uint2(_285, _286), 0u));
                        uint4 _1234 = _31[1u].Load(int3(uint2(_285, _286), 0u));
                        uint _1236 = _1234.x;
                        float4 _1239 = _27[8u].Load(int3(uint2(_285, _286), 0u));
                        float _1249 = (float((_1236 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1250 = (float(_1236 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1254 = (1.0f - abs(_1249)) - abs(_1250);
                        float _1256 = clamp((-0.0f) - _1254, 0.0f, 1.0f);
                        float _1257 = (-0.0f) - _1256;
                        float _1262 = ((_1249 >= 0.0f) ? _1257 : _1256) + _1249;
                        float _1263 = ((_1250 >= 0.0f) ? _1257 : _1256) + _1250;
                        float _1267 = rsqrt(dot(float3(_1262, _1263, _1254), float3(_1262, _1263, _1254)));
                        float _823 = _1262 * _1267;
                        float _828 = _1263 * _1267;
                        float _833 = _1267 * _1254;
                        float _1274 = asfloat(_10.Load(_414 * 115u).x);
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_1 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_2 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_3 = _369;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_4 = _370;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_5 = _371;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_6 = _823;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_7 = _828;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_8 = _833;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_9 = _823;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_10 = _828;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_11 = _833;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_12 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_13 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_14 = _1230.x;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_15 = _1230.y;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_16 = _1230.z;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_17 = _1274;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_18 = _1274;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_19 = _1274;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_20 = float(_1236 & 255u) * 0.0039215688593685626983642578125f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_21 = float(_1224.y) * 0.0039215688593685626983642578125f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_23 = float(_1224.z) * 0.0039215688593685626983642578125f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_24 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_25 = _1239.y;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_26 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_27 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_28 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_29 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_30 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_31 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_32 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_33 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_34 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_35 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_36 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_37 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_38 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_39 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_40 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_41 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_42 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_43 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_44 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_45 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_46 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_47 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_48 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_49 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_50 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_51 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_52 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_53 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_54 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_55 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_56 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_57 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_58 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_59 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_60 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_61 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_62 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_63 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_64 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_65 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_66 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_67 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_68 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_69 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_70 = _432;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_71 = _406;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_72 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_73 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_74 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_75 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_76 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_77 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_78 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_79 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_80 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_81 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_82 = _1230.w;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_83 = _1239.x;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_84 = _1239.z;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_85 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_86 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_87 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_88 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_89 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_90 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_91 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_92 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_93 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_94 = 0.0f;
                    }
                    frontier_phi_13_5_ladder_8_ladder_11_ladder = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_1 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_1;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_2 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_2;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_3 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_3;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_4 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_4;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_5 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_5;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_6 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_6;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_7 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_7;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_8 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_8;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_9 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_9;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_10 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_10;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_11 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_11;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_12 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_12;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_13 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_13;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_14 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_14;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_15 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_15;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_16 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_16;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_17 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_17;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_18 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_18;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_19 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_19;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_20 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_20;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_21 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_21;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_22 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_22;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_23 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_23;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_24 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_24;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_25 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_25;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_26 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_26;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_27 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_27;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_28 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_28;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_29 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_29;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_30 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_30;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_31 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_31;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_32 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_32;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_33 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_33;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_34 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_34;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_35 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_35;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_36 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_36;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_37 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_37;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_38 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_38;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_39 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_39;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_40 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_40;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_41 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_41;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_42 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_42;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_43 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_43;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_44 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_44;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_45 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_45;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_46 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_46;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_47 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_47;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_48 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_48;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_49 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_49;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_50 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_50;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_51 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_51;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_52 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_52;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_53 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_53;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_54 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_54;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_55 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_55;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_56 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_56;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_57 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_57;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_58 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_58;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_59 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_59;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_60 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_60;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_61 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_61;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_62 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_62;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_63 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_63;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_64 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_64;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_65 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_65;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_66 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_66;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_67 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_67;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_68 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_68;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_69 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_69;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_70 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_70;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_71 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_71;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_72 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_72;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_73 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_73;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_74 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_74;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_75 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_75;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_76 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_76;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_77 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_77;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_78 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_78;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_79 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_79;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_80 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_80;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_81 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_81;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_82 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_82;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_83 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_83;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_84 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_84;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_85 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_85;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_86 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_86;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_87 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_87;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_88 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_88;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_89 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_89;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_90 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_90;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_91 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_91;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_92 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_92;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_93 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_93;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_94 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16_ladder_94;
                }
                else
                {
                    float4 _1016 = _27[3u].Load(int3(uint2(_285, _286), 0u));
                    float _1018 = _1016.x;
                    float _1019 = _1016.y;
                    float _1020 = _1016.z;
                    float _1021 = _1016.w;
                    uint4 _1024 = _31[1u].Load(int3(uint2(_285, _286), 0u));
                    uint _1026 = _1024.x;
                    float4 _1029 = _27[8u].Load(int3(uint2(_285, _286), 0u));
                    float _1031 = _1029.x;
                    float _1032 = _1029.y;
                    float _1033 = _1029.z;
                    float _930 = _1029.w;
                    float _1042 = (float((_1026 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1043 = (float(_1026 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _1047 = (1.0f - abs(_1042)) - abs(_1043);
                    float _1049 = clamp((-0.0f) - _1047, 0.0f, 1.0f);
                    float _1050 = (-0.0f) - _1049;
                    float _1055 = ((_1042 >= 0.0f) ? _1050 : _1049) + _1042;
                    float _1056 = ((_1043 >= 0.0f) ? _1050 : _1049) + _1043;
                    float _1060 = rsqrt(dot(float3(_1055, _1056, _1047), float3(_1055, _1056, _1047)));
                    float _825 = _1055 * _1060;
                    float _830 = _1056 * _1060;
                    float _835 = _1060 * _1047;
                    float _880 = float(_1026 & 255u) * 0.0039215688593685626983642578125f;
                    float _886 = float(_31[2u].Load(int3(uint2(_285, _286), 0u)).y) * 0.0039215688593685626983642578125f;
                    float _924 = (_1031 <= 0.040449999272823333740234375f) ? (_1031 * 0.077399380505084991455078125f) : exp2(log2((abs(_1031) + 0.054999999701976776123046875f) * 0.947867333889007568359375f) * 2.400000095367431640625f);
                    float _926 = (_1032 <= 0.040449999272823333740234375f) ? (_1032 * 0.077399380505084991455078125f) : exp2(log2((abs(_1032) + 0.054999999701976776123046875f) * 0.947867333889007568359375f) * 2.400000095367431640625f);
                    float _928 = (_1033 <= 0.040449999272823333740234375f) ? (_1033 * 0.077399380505084991455078125f) : exp2(log2((abs(_1033) + 0.054999999701976776123046875f) * 0.947867333889007568359375f) * 2.400000095367431640625f);
                    bool _1094 = (_406 & 536870912u) == 0u;
                    float _909;
                    float _1275;
                    if (_1094)
                    {
                        _1275 = 1.0f;
                        _909 = 1.0f;
                    }
                    else
                    {
                        uint4 _1316 = _31[9u].Load(int3(uint2(_285, _286), 0u));
                        uint _1318 = _1316.x;
                        _1275 = float((_1318 >> 8u) & 255u) * 0.0039215688593685626983642578125f;
                        _909 = float(_1318 & 255u) * 0.0039215688593685626983642578125f;
                    }
                    float _1281 = asfloat(_10.Load(_414 * 115u).x);
                    uint _1283 = (_414 * 115u) + 2u;
                    float3 _1293 = asfloat(uint3(_10.Load(_1283).x, _10.Load(_1283 + 1u).x, _10.Load(_1283 + 2u).x));
                    float _1304 = 1.0f - _1021;
                    float _853 = _1304 * _1018;
                    float _858 = _1304 * _1019;
                    float _863 = _1304 * _1020;
                    float _868 = ((_1018 - _1281) * _1021) + _1281;
                    float _871 = ((_1019 - _1281) * _1021) + _1281;
                    float _874 = ((_1020 - _1281) * _1021) + _1281;
                    float _891 = exp2(log2(asfloat(_10.Load((_414 * 115u) + 96u).x) + _886) * asfloat(_10.Load((_414 * 115u) + 97u).x));
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_1;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_2;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_3;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_4;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_5;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_6;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_7;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_8;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_9;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_10;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_11;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_12;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_13;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_14;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_15;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_16;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_17;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_18;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_19;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_20;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_21;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_22;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_23;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_24;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_25;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_26;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_27;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_28;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_29;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_30;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_31;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_32;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_33;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_34;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_35;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_36;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_37;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_38;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_39;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_40;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_41;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_42;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_43;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_44;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_45;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_46;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_47;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_48;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_49;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_50;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_51;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_52;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_53;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_54;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_55;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_56;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_57;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_58;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_59;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_60;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_61;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_62;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_63;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_64;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_65;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_66;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_67;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_68;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_69;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_70;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_71;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_72;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_73;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_74;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_75;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_76;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_77;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_78;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_79;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_80;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_81;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_82;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_83;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_84;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_85;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_86;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_87;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_88;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_89;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_90;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_91;
                    uint frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_92;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_93;
                    float frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_94;
                    if (_1094)
                    {
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_1 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_2 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_3 = _369;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_4 = _370;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_5 = _371;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_6 = _825;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_7 = _830;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_8 = _835;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_9 = _825;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_10 = _830;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_11 = _835;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_12 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_13 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_14 = _853;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_15 = _858;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_16 = _863;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_17 = _868;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_18 = _871;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_19 = _874;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_20 = _880;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_21 = _886;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_22 = _886;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_23 = _891;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_24 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_25 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_26 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_27 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_28 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_29 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_30 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_31 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_32 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_33 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_34 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_35 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_36 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_37 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_38 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_39 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_40 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_41 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_42 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_43 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_44 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_45 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_46 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_47 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_48 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_49 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_50 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_51 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_52 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_53 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_54 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_55 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_56 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_57 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_58 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_59 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_60 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_61 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_62 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_63 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_64 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_65 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_66 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_67 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_68 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_69 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_70 = _432;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_71 = _406;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_72 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_73 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_74 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_75 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_76 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_77 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_78 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_79 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_80 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_81 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_82 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_83 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_84 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_85 = _924;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_86 = _926;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_87 = _928;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_88 = _930;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_89 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_90 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_91 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_92 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_93 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_94 = 0.0f;
                    }
                    else
                    {
                        float _1516 = ((_863 * 0.10999999940395355224609375f) + (_853 * 0.300000011920928955078125f)) + (_858 * 0.589999973773956298828125f);
                        float _1526 = _1275 * _462;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_1 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_2 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_3 = _369;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_4 = _370;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_5 = _371;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_6 = _825;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_7 = _830;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_8 = _835;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_9 = _825;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_10 = _830;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_11 = _835;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_12 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_13 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_14 = _853;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_15 = _858;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_16 = _863;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_17 = _868;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_18 = _871;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_19 = _874;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_20 = _880;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_21 = _886;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_22 = _886;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_23 = _891;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_24 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_25 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_26 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_27 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_28 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_29 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_30 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_31 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_32 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_33 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_34 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_35 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_36 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_37 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_38 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_39 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_40 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_41 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_42 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_43 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_44 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_45 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_46 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_47 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_48 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_49 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_50 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_51 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_52 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_53 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_54 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_55 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_56 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_57 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_58 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_59 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_60 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_61 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_62 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_63 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_64 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_65 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_66 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_67 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_68 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_69 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_70 = _432;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_71 = _406;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_72 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_73 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_74 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_75 = (_1526 * (((_853 - _1516) * _457) + _1516)) * _1293.x;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_76 = (_1526 * (((_858 - _1516) * _457) + _1516)) * _1293.y;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_77 = (_1526 * (((_863 - _1516) * _457) + _1516)) * _1293.z;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_78 = 1.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_79 = _909;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_80 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_81 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_82 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_83 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_84 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_85 = _924;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_86 = _926;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_87 = _928;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_88 = _930;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_89 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_90 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_91 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_92 = 0u;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_93 = 0.0f;
                        frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_94 = 0.0f;
                    }
                    frontier_phi_13_5_ladder_8_ladder_11_ladder = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_1 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_1;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_2 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_2;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_3 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_3;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_4 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_4;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_5 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_5;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_6 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_6;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_7 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_7;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_8 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_8;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_9 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_9;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_10 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_10;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_11 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_11;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_12 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_12;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_13 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_13;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_14 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_14;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_15 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_15;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_16 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_16;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_17 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_17;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_18 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_18;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_19 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_19;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_20 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_20;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_21 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_21;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_22 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_22;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_23 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_23;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_24 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_24;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_25 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_25;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_26 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_26;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_27 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_27;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_28 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_28;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_29 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_29;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_30 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_30;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_31 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_31;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_32 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_32;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_33 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_33;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_34 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_34;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_35 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_35;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_36 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_36;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_37 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_37;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_38 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_38;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_39 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_39;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_40 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_40;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_41 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_41;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_42 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_42;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_43 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_43;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_44 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_44;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_45 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_45;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_46 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_46;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_47 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_47;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_48 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_48;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_49 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_49;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_50 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_50;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_51 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_51;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_52 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_52;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_53 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_53;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_54 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_54;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_55 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_55;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_56 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_56;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_57 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_57;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_58 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_58;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_59 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_59;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_60 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_60;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_61 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_61;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_62 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_62;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_63 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_63;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_64 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_64;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_65 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_65;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_66 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_66;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_67 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_67;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_68 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_68;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_69 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_69;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_70 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_70;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_71 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_71;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_72 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_72;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_73 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_73;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_74 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_74;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_75 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_75;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_76 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_76;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_77 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_77;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_78 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_78;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_79 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_79;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_80 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_80;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_81 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_81;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_82 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_82;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_83 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_83;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_84 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_84;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_85 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_85;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_86 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_86;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_87 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_87;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_88 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_88;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_89 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_89;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_90 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_90;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_91 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_91;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_92 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_92;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_93 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_93;
                    frontier_phi_13_5_ladder_8_ladder_11_ladder_94 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24_ladder_94;
                }
                frontier_phi_13_5_ladder_8_ladder = frontier_phi_13_5_ladder_8_ladder_11_ladder;
                frontier_phi_13_5_ladder_8_ladder_1 = frontier_phi_13_5_ladder_8_ladder_11_ladder_1;
                frontier_phi_13_5_ladder_8_ladder_2 = frontier_phi_13_5_ladder_8_ladder_11_ladder_2;
                frontier_phi_13_5_ladder_8_ladder_3 = frontier_phi_13_5_ladder_8_ladder_11_ladder_3;
                frontier_phi_13_5_ladder_8_ladder_4 = frontier_phi_13_5_ladder_8_ladder_11_ladder_4;
                frontier_phi_13_5_ladder_8_ladder_5 = frontier_phi_13_5_ladder_8_ladder_11_ladder_5;
                frontier_phi_13_5_ladder_8_ladder_6 = frontier_phi_13_5_ladder_8_ladder_11_ladder_6;
                frontier_phi_13_5_ladder_8_ladder_7 = frontier_phi_13_5_ladder_8_ladder_11_ladder_7;
                frontier_phi_13_5_ladder_8_ladder_8 = frontier_phi_13_5_ladder_8_ladder_11_ladder_8;
                frontier_phi_13_5_ladder_8_ladder_9 = frontier_phi_13_5_ladder_8_ladder_11_ladder_9;
                frontier_phi_13_5_ladder_8_ladder_10 = frontier_phi_13_5_ladder_8_ladder_11_ladder_10;
                frontier_phi_13_5_ladder_8_ladder_11 = frontier_phi_13_5_ladder_8_ladder_11_ladder_11;
                frontier_phi_13_5_ladder_8_ladder_12 = frontier_phi_13_5_ladder_8_ladder_11_ladder_12;
                frontier_phi_13_5_ladder_8_ladder_13 = frontier_phi_13_5_ladder_8_ladder_11_ladder_13;
                frontier_phi_13_5_ladder_8_ladder_14 = frontier_phi_13_5_ladder_8_ladder_11_ladder_14;
                frontier_phi_13_5_ladder_8_ladder_15 = frontier_phi_13_5_ladder_8_ladder_11_ladder_15;
                frontier_phi_13_5_ladder_8_ladder_16 = frontier_phi_13_5_ladder_8_ladder_11_ladder_16;
                frontier_phi_13_5_ladder_8_ladder_17 = frontier_phi_13_5_ladder_8_ladder_11_ladder_17;
                frontier_phi_13_5_ladder_8_ladder_18 = frontier_phi_13_5_ladder_8_ladder_11_ladder_18;
                frontier_phi_13_5_ladder_8_ladder_19 = frontier_phi_13_5_ladder_8_ladder_11_ladder_19;
                frontier_phi_13_5_ladder_8_ladder_20 = frontier_phi_13_5_ladder_8_ladder_11_ladder_20;
                frontier_phi_13_5_ladder_8_ladder_21 = frontier_phi_13_5_ladder_8_ladder_11_ladder_21;
                frontier_phi_13_5_ladder_8_ladder_22 = frontier_phi_13_5_ladder_8_ladder_11_ladder_22;
                frontier_phi_13_5_ladder_8_ladder_23 = frontier_phi_13_5_ladder_8_ladder_11_ladder_23;
                frontier_phi_13_5_ladder_8_ladder_24 = frontier_phi_13_5_ladder_8_ladder_11_ladder_24;
                frontier_phi_13_5_ladder_8_ladder_25 = frontier_phi_13_5_ladder_8_ladder_11_ladder_25;
                frontier_phi_13_5_ladder_8_ladder_26 = frontier_phi_13_5_ladder_8_ladder_11_ladder_26;
                frontier_phi_13_5_ladder_8_ladder_27 = frontier_phi_13_5_ladder_8_ladder_11_ladder_27;
                frontier_phi_13_5_ladder_8_ladder_28 = frontier_phi_13_5_ladder_8_ladder_11_ladder_28;
                frontier_phi_13_5_ladder_8_ladder_29 = frontier_phi_13_5_ladder_8_ladder_11_ladder_29;
                frontier_phi_13_5_ladder_8_ladder_30 = frontier_phi_13_5_ladder_8_ladder_11_ladder_30;
                frontier_phi_13_5_ladder_8_ladder_31 = frontier_phi_13_5_ladder_8_ladder_11_ladder_31;
                frontier_phi_13_5_ladder_8_ladder_32 = frontier_phi_13_5_ladder_8_ladder_11_ladder_32;
                frontier_phi_13_5_ladder_8_ladder_33 = frontier_phi_13_5_ladder_8_ladder_11_ladder_33;
                frontier_phi_13_5_ladder_8_ladder_34 = frontier_phi_13_5_ladder_8_ladder_11_ladder_34;
                frontier_phi_13_5_ladder_8_ladder_35 = frontier_phi_13_5_ladder_8_ladder_11_ladder_35;
                frontier_phi_13_5_ladder_8_ladder_36 = frontier_phi_13_5_ladder_8_ladder_11_ladder_36;
                frontier_phi_13_5_ladder_8_ladder_37 = frontier_phi_13_5_ladder_8_ladder_11_ladder_37;
                frontier_phi_13_5_ladder_8_ladder_38 = frontier_phi_13_5_ladder_8_ladder_11_ladder_38;
                frontier_phi_13_5_ladder_8_ladder_39 = frontier_phi_13_5_ladder_8_ladder_11_ladder_39;
                frontier_phi_13_5_ladder_8_ladder_40 = frontier_phi_13_5_ladder_8_ladder_11_ladder_40;
                frontier_phi_13_5_ladder_8_ladder_41 = frontier_phi_13_5_ladder_8_ladder_11_ladder_41;
                frontier_phi_13_5_ladder_8_ladder_42 = frontier_phi_13_5_ladder_8_ladder_11_ladder_42;
                frontier_phi_13_5_ladder_8_ladder_43 = frontier_phi_13_5_ladder_8_ladder_11_ladder_43;
                frontier_phi_13_5_ladder_8_ladder_44 = frontier_phi_13_5_ladder_8_ladder_11_ladder_44;
                frontier_phi_13_5_ladder_8_ladder_45 = frontier_phi_13_5_ladder_8_ladder_11_ladder_45;
                frontier_phi_13_5_ladder_8_ladder_46 = frontier_phi_13_5_ladder_8_ladder_11_ladder_46;
                frontier_phi_13_5_ladder_8_ladder_47 = frontier_phi_13_5_ladder_8_ladder_11_ladder_47;
                frontier_phi_13_5_ladder_8_ladder_48 = frontier_phi_13_5_ladder_8_ladder_11_ladder_48;
                frontier_phi_13_5_ladder_8_ladder_49 = frontier_phi_13_5_ladder_8_ladder_11_ladder_49;
                frontier_phi_13_5_ladder_8_ladder_50 = frontier_phi_13_5_ladder_8_ladder_11_ladder_50;
                frontier_phi_13_5_ladder_8_ladder_51 = frontier_phi_13_5_ladder_8_ladder_11_ladder_51;
                frontier_phi_13_5_ladder_8_ladder_52 = frontier_phi_13_5_ladder_8_ladder_11_ladder_52;
                frontier_phi_13_5_ladder_8_ladder_53 = frontier_phi_13_5_ladder_8_ladder_11_ladder_53;
                frontier_phi_13_5_ladder_8_ladder_54 = frontier_phi_13_5_ladder_8_ladder_11_ladder_54;
                frontier_phi_13_5_ladder_8_ladder_55 = frontier_phi_13_5_ladder_8_ladder_11_ladder_55;
                frontier_phi_13_5_ladder_8_ladder_56 = frontier_phi_13_5_ladder_8_ladder_11_ladder_56;
                frontier_phi_13_5_ladder_8_ladder_57 = frontier_phi_13_5_ladder_8_ladder_11_ladder_57;
                frontier_phi_13_5_ladder_8_ladder_58 = frontier_phi_13_5_ladder_8_ladder_11_ladder_58;
                frontier_phi_13_5_ladder_8_ladder_59 = frontier_phi_13_5_ladder_8_ladder_11_ladder_59;
                frontier_phi_13_5_ladder_8_ladder_60 = frontier_phi_13_5_ladder_8_ladder_11_ladder_60;
                frontier_phi_13_5_ladder_8_ladder_61 = frontier_phi_13_5_ladder_8_ladder_11_ladder_61;
                frontier_phi_13_5_ladder_8_ladder_62 = frontier_phi_13_5_ladder_8_ladder_11_ladder_62;
                frontier_phi_13_5_ladder_8_ladder_63 = frontier_phi_13_5_ladder_8_ladder_11_ladder_63;
                frontier_phi_13_5_ladder_8_ladder_64 = frontier_phi_13_5_ladder_8_ladder_11_ladder_64;
                frontier_phi_13_5_ladder_8_ladder_65 = frontier_phi_13_5_ladder_8_ladder_11_ladder_65;
                frontier_phi_13_5_ladder_8_ladder_66 = frontier_phi_13_5_ladder_8_ladder_11_ladder_66;
                frontier_phi_13_5_ladder_8_ladder_67 = frontier_phi_13_5_ladder_8_ladder_11_ladder_67;
                frontier_phi_13_5_ladder_8_ladder_68 = frontier_phi_13_5_ladder_8_ladder_11_ladder_68;
                frontier_phi_13_5_ladder_8_ladder_69 = frontier_phi_13_5_ladder_8_ladder_11_ladder_69;
                frontier_phi_13_5_ladder_8_ladder_70 = frontier_phi_13_5_ladder_8_ladder_11_ladder_70;
                frontier_phi_13_5_ladder_8_ladder_71 = frontier_phi_13_5_ladder_8_ladder_11_ladder_71;
                frontier_phi_13_5_ladder_8_ladder_72 = frontier_phi_13_5_ladder_8_ladder_11_ladder_72;
                frontier_phi_13_5_ladder_8_ladder_73 = frontier_phi_13_5_ladder_8_ladder_11_ladder_73;
                frontier_phi_13_5_ladder_8_ladder_74 = frontier_phi_13_5_ladder_8_ladder_11_ladder_74;
                frontier_phi_13_5_ladder_8_ladder_75 = frontier_phi_13_5_ladder_8_ladder_11_ladder_75;
                frontier_phi_13_5_ladder_8_ladder_76 = frontier_phi_13_5_ladder_8_ladder_11_ladder_76;
                frontier_phi_13_5_ladder_8_ladder_77 = frontier_phi_13_5_ladder_8_ladder_11_ladder_77;
                frontier_phi_13_5_ladder_8_ladder_78 = frontier_phi_13_5_ladder_8_ladder_11_ladder_78;
                frontier_phi_13_5_ladder_8_ladder_79 = frontier_phi_13_5_ladder_8_ladder_11_ladder_79;
                frontier_phi_13_5_ladder_8_ladder_80 = frontier_phi_13_5_ladder_8_ladder_11_ladder_80;
                frontier_phi_13_5_ladder_8_ladder_81 = frontier_phi_13_5_ladder_8_ladder_11_ladder_81;
                frontier_phi_13_5_ladder_8_ladder_82 = frontier_phi_13_5_ladder_8_ladder_11_ladder_82;
                frontier_phi_13_5_ladder_8_ladder_83 = frontier_phi_13_5_ladder_8_ladder_11_ladder_83;
                frontier_phi_13_5_ladder_8_ladder_84 = frontier_phi_13_5_ladder_8_ladder_11_ladder_84;
                frontier_phi_13_5_ladder_8_ladder_85 = frontier_phi_13_5_ladder_8_ladder_11_ladder_85;
                frontier_phi_13_5_ladder_8_ladder_86 = frontier_phi_13_5_ladder_8_ladder_11_ladder_86;
                frontier_phi_13_5_ladder_8_ladder_87 = frontier_phi_13_5_ladder_8_ladder_11_ladder_87;
                frontier_phi_13_5_ladder_8_ladder_88 = frontier_phi_13_5_ladder_8_ladder_11_ladder_88;
                frontier_phi_13_5_ladder_8_ladder_89 = frontier_phi_13_5_ladder_8_ladder_11_ladder_89;
                frontier_phi_13_5_ladder_8_ladder_90 = frontier_phi_13_5_ladder_8_ladder_11_ladder_90;
                frontier_phi_13_5_ladder_8_ladder_91 = frontier_phi_13_5_ladder_8_ladder_11_ladder_91;
                frontier_phi_13_5_ladder_8_ladder_92 = frontier_phi_13_5_ladder_8_ladder_11_ladder_92;
                frontier_phi_13_5_ladder_8_ladder_93 = frontier_phi_13_5_ladder_8_ladder_11_ladder_93;
                frontier_phi_13_5_ladder_8_ladder_94 = frontier_phi_13_5_ladder_8_ladder_11_ladder_94;
            }
            else
            {
                uint4 _656 = _31[2u].Load(int3(uint2(_285, _286), 0u));
                float4 _662 = _27[3u].Load(int3(uint2(_285, _286), 0u));
                float _664 = _662.x;
                float _665 = _662.y;
                float _666 = _662.z;
                uint4 _670 = _31[1u].Load(int3(uint2(_285, _286), 0u));
                uint _672 = _670.x;
                float _681 = (float((_672 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                float _682 = (float(_672 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                float _686 = (1.0f - abs(_681)) - abs(_682);
                float _688 = clamp((-0.0f) - _686, 0.0f, 1.0f);
                float _689 = (-0.0f) - _688;
                float _694 = ((_681 >= 0.0f) ? _689 : _688) + _681;
                float _695 = ((_682 >= 0.0f) ? _689 : _688) + _682;
                float _699 = rsqrt(dot(float3(_694, _695, _686), float3(_694, _695, _686)));
                float _700 = _694 * _699;
                float _701 = _695 * _699;
                float _702 = _699 * _686;
                float _706 = float(_656.y) * 0.0039215688593685626983642578125f;
                float _711 = asfloat(_10.Load(_414 * 115u).x);
                uint _713 = (_414 * 115u) + 2u;
                float3 _723 = asfloat(uint3(_10.Load(_713).x, _10.Load(_713 + 1u).x, _10.Load(_713 + 2u).x));
                float _750 = ((_666 * 0.10999999940395355224609375f) + (_664 * 0.300000011920928955078125f)) + (_665 * 0.589999973773956298828125f);
                float _761 = (_462 * 0.0039215688593685626983642578125f) * float(_656.z);
                frontier_phi_13_5_ladder_8_ladder = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_1 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_2 = 0u;
                frontier_phi_13_5_ladder_8_ladder_3 = _369;
                frontier_phi_13_5_ladder_8_ladder_4 = _370;
                frontier_phi_13_5_ladder_8_ladder_5 = _371;
                frontier_phi_13_5_ladder_8_ladder_6 = _700;
                frontier_phi_13_5_ladder_8_ladder_7 = _701;
                frontier_phi_13_5_ladder_8_ladder_8 = _702;
                frontier_phi_13_5_ladder_8_ladder_9 = _700;
                frontier_phi_13_5_ladder_8_ladder_10 = _701;
                frontier_phi_13_5_ladder_8_ladder_11 = _702;
                frontier_phi_13_5_ladder_8_ladder_12 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_13 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_14 = _664;
                frontier_phi_13_5_ladder_8_ladder_15 = _665;
                frontier_phi_13_5_ladder_8_ladder_16 = _666;
                frontier_phi_13_5_ladder_8_ladder_17 = _711;
                frontier_phi_13_5_ladder_8_ladder_18 = _711;
                frontier_phi_13_5_ladder_8_ladder_19 = _711;
                frontier_phi_13_5_ladder_8_ladder_20 = float(_672 & 255u) * 0.0039215688593685626983642578125f;
                frontier_phi_13_5_ladder_8_ladder_21 = _706;
                frontier_phi_13_5_ladder_8_ladder_22 = 1.0f;
                frontier_phi_13_5_ladder_8_ladder_23 = exp2(log2(asfloat(_10.Load((_414 * 115u) + 96u).x) + _706) * asfloat(_10.Load((_414 * 115u) + 97u).x));
                frontier_phi_13_5_ladder_8_ladder_24 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_25 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_26 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_27 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_28 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_29 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_30 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_31 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_32 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_33 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_34 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_35 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_36 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_37 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_38 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_39 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_40 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_41 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_42 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_43 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_44 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_45 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_46 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_47 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_48 = 0u;
                frontier_phi_13_5_ladder_8_ladder_49 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_50 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_51 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_52 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_53 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_54 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_55 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_56 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_57 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_58 = 0u;
                frontier_phi_13_5_ladder_8_ladder_59 = 0u;
                frontier_phi_13_5_ladder_8_ladder_60 = 1.0f;
                frontier_phi_13_5_ladder_8_ladder_61 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_62 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_63 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_64 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_65 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_66 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_67 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_68 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_69 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_70 = _432;
                frontier_phi_13_5_ladder_8_ladder_71 = _406;
                frontier_phi_13_5_ladder_8_ladder_72 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_73 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_74 = 1.0f;
                frontier_phi_13_5_ladder_8_ladder_75 = ((((_664 - _750) * _457) + _750) * _761) * _723.x;
                frontier_phi_13_5_ladder_8_ladder_76 = ((((_665 - _750) * _457) + _750) * _761) * _723.y;
                frontier_phi_13_5_ladder_8_ladder_77 = ((((_666 - _750) * _457) + _750) * _761) * _723.z;
                frontier_phi_13_5_ladder_8_ladder_78 = 1.0f;
                frontier_phi_13_5_ladder_8_ladder_79 = _662.w;
                frontier_phi_13_5_ladder_8_ladder_80 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_81 = 0u;
                frontier_phi_13_5_ladder_8_ladder_82 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_83 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_84 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_85 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_86 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_87 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_88 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_89 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_90 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_91 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_92 = 0u;
                frontier_phi_13_5_ladder_8_ladder_93 = 0.0f;
                frontier_phi_13_5_ladder_8_ladder_94 = 0.0f;
            }
            frontier_phi_13_5_ladder = frontier_phi_13_5_ladder_8_ladder;
            frontier_phi_13_5_ladder_1 = frontier_phi_13_5_ladder_8_ladder_1;
            frontier_phi_13_5_ladder_2 = frontier_phi_13_5_ladder_8_ladder_2;
            frontier_phi_13_5_ladder_3 = frontier_phi_13_5_ladder_8_ladder_3;
            frontier_phi_13_5_ladder_4 = frontier_phi_13_5_ladder_8_ladder_4;
            frontier_phi_13_5_ladder_5 = frontier_phi_13_5_ladder_8_ladder_5;
            frontier_phi_13_5_ladder_6 = frontier_phi_13_5_ladder_8_ladder_6;
            frontier_phi_13_5_ladder_7 = frontier_phi_13_5_ladder_8_ladder_7;
            frontier_phi_13_5_ladder_8 = frontier_phi_13_5_ladder_8_ladder_8;
            frontier_phi_13_5_ladder_9 = frontier_phi_13_5_ladder_8_ladder_9;
            frontier_phi_13_5_ladder_10 = frontier_phi_13_5_ladder_8_ladder_10;
            frontier_phi_13_5_ladder_11 = frontier_phi_13_5_ladder_8_ladder_11;
            frontier_phi_13_5_ladder_12 = frontier_phi_13_5_ladder_8_ladder_12;
            frontier_phi_13_5_ladder_13 = frontier_phi_13_5_ladder_8_ladder_13;
            frontier_phi_13_5_ladder_14 = frontier_phi_13_5_ladder_8_ladder_14;
            frontier_phi_13_5_ladder_15 = frontier_phi_13_5_ladder_8_ladder_15;
            frontier_phi_13_5_ladder_16 = frontier_phi_13_5_ladder_8_ladder_16;
            frontier_phi_13_5_ladder_17 = frontier_phi_13_5_ladder_8_ladder_17;
            frontier_phi_13_5_ladder_18 = frontier_phi_13_5_ladder_8_ladder_18;
            frontier_phi_13_5_ladder_19 = frontier_phi_13_5_ladder_8_ladder_19;
            frontier_phi_13_5_ladder_20 = frontier_phi_13_5_ladder_8_ladder_20;
            frontier_phi_13_5_ladder_21 = frontier_phi_13_5_ladder_8_ladder_21;
            frontier_phi_13_5_ladder_22 = frontier_phi_13_5_ladder_8_ladder_22;
            frontier_phi_13_5_ladder_23 = frontier_phi_13_5_ladder_8_ladder_23;
            frontier_phi_13_5_ladder_24 = frontier_phi_13_5_ladder_8_ladder_24;
            frontier_phi_13_5_ladder_25 = frontier_phi_13_5_ladder_8_ladder_25;
            frontier_phi_13_5_ladder_26 = frontier_phi_13_5_ladder_8_ladder_26;
            frontier_phi_13_5_ladder_27 = frontier_phi_13_5_ladder_8_ladder_27;
            frontier_phi_13_5_ladder_28 = frontier_phi_13_5_ladder_8_ladder_28;
            frontier_phi_13_5_ladder_29 = frontier_phi_13_5_ladder_8_ladder_29;
            frontier_phi_13_5_ladder_30 = frontier_phi_13_5_ladder_8_ladder_30;
            frontier_phi_13_5_ladder_31 = frontier_phi_13_5_ladder_8_ladder_31;
            frontier_phi_13_5_ladder_32 = frontier_phi_13_5_ladder_8_ladder_32;
            frontier_phi_13_5_ladder_33 = frontier_phi_13_5_ladder_8_ladder_33;
            frontier_phi_13_5_ladder_34 = frontier_phi_13_5_ladder_8_ladder_34;
            frontier_phi_13_5_ladder_35 = frontier_phi_13_5_ladder_8_ladder_35;
            frontier_phi_13_5_ladder_36 = frontier_phi_13_5_ladder_8_ladder_36;
            frontier_phi_13_5_ladder_37 = frontier_phi_13_5_ladder_8_ladder_37;
            frontier_phi_13_5_ladder_38 = frontier_phi_13_5_ladder_8_ladder_38;
            frontier_phi_13_5_ladder_39 = frontier_phi_13_5_ladder_8_ladder_39;
            frontier_phi_13_5_ladder_40 = frontier_phi_13_5_ladder_8_ladder_40;
            frontier_phi_13_5_ladder_41 = frontier_phi_13_5_ladder_8_ladder_41;
            frontier_phi_13_5_ladder_42 = frontier_phi_13_5_ladder_8_ladder_42;
            frontier_phi_13_5_ladder_43 = frontier_phi_13_5_ladder_8_ladder_43;
            frontier_phi_13_5_ladder_44 = frontier_phi_13_5_ladder_8_ladder_44;
            frontier_phi_13_5_ladder_45 = frontier_phi_13_5_ladder_8_ladder_45;
            frontier_phi_13_5_ladder_46 = frontier_phi_13_5_ladder_8_ladder_46;
            frontier_phi_13_5_ladder_47 = frontier_phi_13_5_ladder_8_ladder_47;
            frontier_phi_13_5_ladder_48 = frontier_phi_13_5_ladder_8_ladder_48;
            frontier_phi_13_5_ladder_49 = frontier_phi_13_5_ladder_8_ladder_49;
            frontier_phi_13_5_ladder_50 = frontier_phi_13_5_ladder_8_ladder_50;
            frontier_phi_13_5_ladder_51 = frontier_phi_13_5_ladder_8_ladder_51;
            frontier_phi_13_5_ladder_52 = frontier_phi_13_5_ladder_8_ladder_52;
            frontier_phi_13_5_ladder_53 = frontier_phi_13_5_ladder_8_ladder_53;
            frontier_phi_13_5_ladder_54 = frontier_phi_13_5_ladder_8_ladder_54;
            frontier_phi_13_5_ladder_55 = frontier_phi_13_5_ladder_8_ladder_55;
            frontier_phi_13_5_ladder_56 = frontier_phi_13_5_ladder_8_ladder_56;
            frontier_phi_13_5_ladder_57 = frontier_phi_13_5_ladder_8_ladder_57;
            frontier_phi_13_5_ladder_58 = frontier_phi_13_5_ladder_8_ladder_58;
            frontier_phi_13_5_ladder_59 = frontier_phi_13_5_ladder_8_ladder_59;
            frontier_phi_13_5_ladder_60 = frontier_phi_13_5_ladder_8_ladder_60;
            frontier_phi_13_5_ladder_61 = frontier_phi_13_5_ladder_8_ladder_61;
            frontier_phi_13_5_ladder_62 = frontier_phi_13_5_ladder_8_ladder_62;
            frontier_phi_13_5_ladder_63 = frontier_phi_13_5_ladder_8_ladder_63;
            frontier_phi_13_5_ladder_64 = frontier_phi_13_5_ladder_8_ladder_64;
            frontier_phi_13_5_ladder_65 = frontier_phi_13_5_ladder_8_ladder_65;
            frontier_phi_13_5_ladder_66 = frontier_phi_13_5_ladder_8_ladder_66;
            frontier_phi_13_5_ladder_67 = frontier_phi_13_5_ladder_8_ladder_67;
            frontier_phi_13_5_ladder_68 = frontier_phi_13_5_ladder_8_ladder_68;
            frontier_phi_13_5_ladder_69 = frontier_phi_13_5_ladder_8_ladder_69;
            frontier_phi_13_5_ladder_70 = frontier_phi_13_5_ladder_8_ladder_70;
            frontier_phi_13_5_ladder_71 = frontier_phi_13_5_ladder_8_ladder_71;
            frontier_phi_13_5_ladder_72 = frontier_phi_13_5_ladder_8_ladder_72;
            frontier_phi_13_5_ladder_73 = frontier_phi_13_5_ladder_8_ladder_73;
            frontier_phi_13_5_ladder_74 = frontier_phi_13_5_ladder_8_ladder_74;
            frontier_phi_13_5_ladder_75 = frontier_phi_13_5_ladder_8_ladder_75;
            frontier_phi_13_5_ladder_76 = frontier_phi_13_5_ladder_8_ladder_76;
            frontier_phi_13_5_ladder_77 = frontier_phi_13_5_ladder_8_ladder_77;
            frontier_phi_13_5_ladder_78 = frontier_phi_13_5_ladder_8_ladder_78;
            frontier_phi_13_5_ladder_79 = frontier_phi_13_5_ladder_8_ladder_79;
            frontier_phi_13_5_ladder_80 = frontier_phi_13_5_ladder_8_ladder_80;
            frontier_phi_13_5_ladder_81 = frontier_phi_13_5_ladder_8_ladder_81;
            frontier_phi_13_5_ladder_82 = frontier_phi_13_5_ladder_8_ladder_82;
            frontier_phi_13_5_ladder_83 = frontier_phi_13_5_ladder_8_ladder_83;
            frontier_phi_13_5_ladder_84 = frontier_phi_13_5_ladder_8_ladder_84;
            frontier_phi_13_5_ladder_85 = frontier_phi_13_5_ladder_8_ladder_85;
            frontier_phi_13_5_ladder_86 = frontier_phi_13_5_ladder_8_ladder_86;
            frontier_phi_13_5_ladder_87 = frontier_phi_13_5_ladder_8_ladder_87;
            frontier_phi_13_5_ladder_88 = frontier_phi_13_5_ladder_8_ladder_88;
            frontier_phi_13_5_ladder_89 = frontier_phi_13_5_ladder_8_ladder_89;
            frontier_phi_13_5_ladder_90 = frontier_phi_13_5_ladder_8_ladder_90;
            frontier_phi_13_5_ladder_91 = frontier_phi_13_5_ladder_8_ladder_91;
            frontier_phi_13_5_ladder_92 = frontier_phi_13_5_ladder_8_ladder_92;
            frontier_phi_13_5_ladder_93 = frontier_phi_13_5_ladder_8_ladder_93;
            frontier_phi_13_5_ladder_94 = frontier_phi_13_5_ladder_8_ladder_94;
        }
        else
        {
            uint4 _540 = _31[2u].Load(int3(uint2(_285, _286), 0u));
            float4 _546 = _27[3u].Load(int3(uint2(_285, _286), 0u));
            float _548 = _546.x;
            float _549 = _546.y;
            float _550 = _546.z;
            float _551 = _546.w;
            uint4 _554 = _31[1u].Load(int3(uint2(_285, _286), 0u));
            uint _556 = _554.x;
            float _568 = (float((_556 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
            float _569 = (float(_556 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
            float _573 = (1.0f - abs(_568)) - abs(_569);
            float _577 = clamp((-0.0f) - _573, 0.0f, 1.0f);
            float _578 = (-0.0f) - _577;
            float _583 = ((_568 >= 0.0f) ? _578 : _577) + _568;
            float _584 = ((_569 >= 0.0f) ? _578 : _577) + _569;
            float _588 = rsqrt(dot(float3(_583, _584, _573), float3(_583, _584, _573)));
            float _589 = _583 * _588;
            float _590 = _584 * _588;
            float _591 = _588 * _573;
            float _596 = float(_540.y) * 0.0039215688593685626983642578125f;
            float _602 = asfloat(_10.Load(_414 * 115u).x);
            float _603 = 1.0f - _551;
            frontier_phi_13_5_ladder = 0.0f;
            frontier_phi_13_5_ladder_1 = 0.0f;
            frontier_phi_13_5_ladder_2 = 0u;
            frontier_phi_13_5_ladder_3 = _369;
            frontier_phi_13_5_ladder_4 = _370;
            frontier_phi_13_5_ladder_5 = _371;
            frontier_phi_13_5_ladder_6 = _589;
            frontier_phi_13_5_ladder_7 = _590;
            frontier_phi_13_5_ladder_8 = _591;
            frontier_phi_13_5_ladder_9 = _589;
            frontier_phi_13_5_ladder_10 = _590;
            frontier_phi_13_5_ladder_11 = _591;
            frontier_phi_13_5_ladder_12 = 0.0f;
            frontier_phi_13_5_ladder_13 = 0.0f;
            frontier_phi_13_5_ladder_14 = _603 * _548;
            frontier_phi_13_5_ladder_15 = _603 * _549;
            frontier_phi_13_5_ladder_16 = _603 * _550;
            frontier_phi_13_5_ladder_17 = ((_548 - _602) * _551) + _602;
            frontier_phi_13_5_ladder_18 = ((_549 - _602) * _551) + _602;
            frontier_phi_13_5_ladder_19 = ((_550 - _602) * _551) + _602;
            frontier_phi_13_5_ladder_20 = float(_556 & 255u) * 0.0039215688593685626983642578125f;
            frontier_phi_13_5_ladder_21 = _596;
            frontier_phi_13_5_ladder_22 = _596;
            frontier_phi_13_5_ladder_23 = float(_540.z) * 0.0039215688593685626983642578125f;
            frontier_phi_13_5_ladder_24 = 0.0f;
            frontier_phi_13_5_ladder_25 = 0.0f;
            frontier_phi_13_5_ladder_26 = 0.0f;
            frontier_phi_13_5_ladder_27 = 0.0f;
            frontier_phi_13_5_ladder_28 = 0.0f;
            frontier_phi_13_5_ladder_29 = 0.0f;
            frontier_phi_13_5_ladder_30 = 0.0f;
            frontier_phi_13_5_ladder_31 = 0.0f;
            frontier_phi_13_5_ladder_32 = 0.0f;
            frontier_phi_13_5_ladder_33 = 0.0f;
            frontier_phi_13_5_ladder_34 = 0.0f;
            frontier_phi_13_5_ladder_35 = 0.0f;
            frontier_phi_13_5_ladder_36 = 0.0f;
            frontier_phi_13_5_ladder_37 = 0.0f;
            frontier_phi_13_5_ladder_38 = 0.0f;
            frontier_phi_13_5_ladder_39 = 0.0f;
            frontier_phi_13_5_ladder_40 = 0.0f;
            frontier_phi_13_5_ladder_41 = 0.0f;
            frontier_phi_13_5_ladder_42 = 0.0f;
            frontier_phi_13_5_ladder_43 = 0.0f;
            frontier_phi_13_5_ladder_44 = 0.0f;
            frontier_phi_13_5_ladder_45 = 0.0f;
            frontier_phi_13_5_ladder_46 = 0.0f;
            frontier_phi_13_5_ladder_47 = 0.0f;
            frontier_phi_13_5_ladder_48 = 0u;
            frontier_phi_13_5_ladder_49 = 0.0f;
            frontier_phi_13_5_ladder_50 = 0.0f;
            frontier_phi_13_5_ladder_51 = 0.0f;
            frontier_phi_13_5_ladder_52 = 0.0f;
            frontier_phi_13_5_ladder_53 = 0.0f;
            frontier_phi_13_5_ladder_54 = 0.0f;
            frontier_phi_13_5_ladder_55 = 0.0f;
            frontier_phi_13_5_ladder_56 = 0.0f;
            frontier_phi_13_5_ladder_57 = 0.0f;
            frontier_phi_13_5_ladder_58 = 0u;
            frontier_phi_13_5_ladder_59 = 0u;
            frontier_phi_13_5_ladder_60 = 1.0f;
            frontier_phi_13_5_ladder_61 = 0.0f;
            frontier_phi_13_5_ladder_62 = 0.0f;
            frontier_phi_13_5_ladder_63 = 0.0f;
            frontier_phi_13_5_ladder_64 = 0.0f;
            frontier_phi_13_5_ladder_65 = 0.0f;
            frontier_phi_13_5_ladder_66 = 0.0f;
            frontier_phi_13_5_ladder_67 = 0.0f;
            frontier_phi_13_5_ladder_68 = 0.0f;
            frontier_phi_13_5_ladder_69 = 0.0f;
            frontier_phi_13_5_ladder_70 = _432;
            frontier_phi_13_5_ladder_71 = _406;
            frontier_phi_13_5_ladder_72 = 0.0f;
            frontier_phi_13_5_ladder_73 = 0.0f;
            frontier_phi_13_5_ladder_74 = 1.0f;
            frontier_phi_13_5_ladder_75 = 0.0f;
            frontier_phi_13_5_ladder_76 = 0.0f;
            frontier_phi_13_5_ladder_77 = 0.0f;
            frontier_phi_13_5_ladder_78 = 1.0f;
            frontier_phi_13_5_ladder_79 = 1.0f;
            frontier_phi_13_5_ladder_80 = 0.0f;
            frontier_phi_13_5_ladder_81 = 0u;
            frontier_phi_13_5_ladder_82 = 0.0f;
            frontier_phi_13_5_ladder_83 = 0.0f;
            frontier_phi_13_5_ladder_84 = 0.0f;
            frontier_phi_13_5_ladder_85 = 0.0f;
            frontier_phi_13_5_ladder_86 = 0.0f;
            frontier_phi_13_5_ladder_87 = 0.0f;
            frontier_phi_13_5_ladder_88 = 0.0f;
            frontier_phi_13_5_ladder_89 = 0.0f;
            frontier_phi_13_5_ladder_90 = 0.0f;
            frontier_phi_13_5_ladder_91 = 0.0f;
            frontier_phi_13_5_ladder_92 = 0u;
            frontier_phi_13_5_ladder_93 = 0.0f;
            frontier_phi_13_5_ladder_94 = 0.0f;
        }
        _768 = frontier_phi_13_5_ladder_26;
        _770 = frontier_phi_13_5_ladder_27;
        _772 = frontier_phi_13_5_ladder_28;
        _774 = frontier_phi_13_5_ladder_29;
        _776 = frontier_phi_13_5_ladder_30;
        _778 = frontier_phi_13_5_ladder_31;
        _780 = frontier_phi_13_5_ladder_32;
        _782 = frontier_phi_13_5_ladder_33;
        _784 = frontier_phi_13_5_ladder_34;
        _786 = frontier_phi_13_5_ladder_35;
        _788 = frontier_phi_13_5_ladder_24;
        _790 = frontier_phi_13_5_ladder_37;
        _792 = frontier_phi_13_5_ladder_38;
        _794 = frontier_phi_13_5_ladder_39;
        _796 = frontier_phi_13_5_ladder_40;
        _798 = frontier_phi_13_5_ladder_41;
        _800 = frontier_phi_13_5_ladder_42;
        _802 = frontier_phi_13_5_ladder_43;
        _804 = frontier_phi_13_5_ladder_44;
        _806 = frontier_phi_13_5_ladder_45;
        _808 = frontier_phi_13_5_ladder_46;
        _810 = frontier_phi_13_5_ladder_47;
        _812 = frontier_phi_13_5_ladder_12;
        _814 = frontier_phi_13_5_ladder_36;
        _816 = frontier_phi_13_5_ladder_1;
        _818 = frontier_phi_13_5_ladder_2;
        _819 = frontier_phi_13_5_ladder_3;
        _820 = frontier_phi_13_5_ladder_4;
        _821 = frontier_phi_13_5_ladder_5;
        _822 = frontier_phi_13_5_ladder_6;
        _827 = frontier_phi_13_5_ladder_7;
        _832 = frontier_phi_13_5_ladder_8;
        _837 = frontier_phi_13_5_ladder_9;
        _840 = frontier_phi_13_5_ladder_10;
        _843 = frontier_phi_13_5_ladder_11;
        _846 = frontier_phi_13_5_ladder;
        _848 = frontier_phi_13_5_ladder_13;
        _850 = frontier_phi_13_5_ladder_14;
        _855 = frontier_phi_13_5_ladder_15;
        _860 = frontier_phi_13_5_ladder_16;
        _865 = frontier_phi_13_5_ladder_17;
        _870 = frontier_phi_13_5_ladder_18;
        _873 = frontier_phi_13_5_ladder_19;
        _876 = frontier_phi_13_5_ladder_20;
        _882 = frontier_phi_13_5_ladder_21;
        _887 = frontier_phi_13_5_ladder_22;
        _888 = frontier_phi_13_5_ladder_23;
        _892 = frontier_phi_13_5_ladder_74;
        _894 = frontier_phi_13_5_ladder_60;
        _897 = frontier_phi_13_5_ladder_75;
        _900 = frontier_phi_13_5_ladder_76;
        _903 = frontier_phi_13_5_ladder_77;
        _906 = frontier_phi_13_5_ladder_78;
        _908 = frontier_phi_13_5_ladder_79;
        _911 = frontier_phi_13_5_ladder_80;
        _913 = frontier_phi_13_5_ladder_81;
        _915 = frontier_phi_13_5_ladder_82;
        _917 = frontier_phi_13_5_ladder_83;
        _919 = frontier_phi_13_5_ladder_25;
        _921 = frontier_phi_13_5_ladder_84;
        _923 = frontier_phi_13_5_ladder_85;
        _925 = frontier_phi_13_5_ladder_86;
        _927 = frontier_phi_13_5_ladder_87;
        _929 = frontier_phi_13_5_ladder_88;
        _931 = frontier_phi_13_5_ladder_89;
        _933 = frontier_phi_13_5_ladder_90;
        _935 = frontier_phi_13_5_ladder_91;
        _937 = frontier_phi_13_5_ladder_92;
        _938 = frontier_phi_13_5_ladder_93;
        _940 = frontier_phi_13_5_ladder_94;
        _942 = frontier_phi_13_5_ladder_72;
        _944 = frontier_phi_13_5_ladder_73;
        _946 = frontier_phi_13_5_ladder_49;
        _948 = frontier_phi_13_5_ladder_50;
        _950 = frontier_phi_13_5_ladder_51;
        _952 = frontier_phi_13_5_ladder_52;
        _954 = frontier_phi_13_5_ladder_53;
        _956 = frontier_phi_13_5_ladder_54;
        _958 = frontier_phi_13_5_ladder_55;
        _960 = frontier_phi_13_5_ladder_56;
        _962 = frontier_phi_13_5_ladder_57;
        _964 = frontier_phi_13_5_ladder_58;
        _966 = frontier_phi_13_5_ladder_59;
        _968 = frontier_phi_13_5_ladder_48;
        _970 = frontier_phi_13_5_ladder_61;
        _972 = frontier_phi_13_5_ladder_62;
        _974 = frontier_phi_13_5_ladder_63;
        _976 = frontier_phi_13_5_ladder_64;
        _978 = frontier_phi_13_5_ladder_65;
        _980 = frontier_phi_13_5_ladder_66;
        _982 = frontier_phi_13_5_ladder_67;
        _984 = frontier_phi_13_5_ladder_68;
        _986 = frontier_phi_13_5_ladder_69;
        _988 = frontier_phi_13_5_ladder_70;
        _989 = frontier_phi_13_5_ladder_71;
    }
    bool _991 = (_431 & 128u) == 0u;
    bool _992 = !_991;
    bool _995 = (_431 & 32u) != 0u;
    float _1095;
    float _1097;
    float _1099;
    float _1101;
    if ((_431 & 144u) == 0u)
    {
        _1095 = 0.0f;
        _1097 = 0.0f;
        _1099 = 0.0f;
        _1101 = 0.0f;
    }
    else
    {
        float _1126 = _296 / _62_m0[1140u].x;
        float _1127 = _298 / _62_m0[1140u].y;
        float _1133 = sqrt(((_362 * _362) + (_361 * _361)) + (_363 * _363));
        uint _1134 = _419 * 3u;
        uint _1135 = _1134 + 1155u;
        uint _1144 = _1134 + 1157u;
        float _1163 = clamp(((_1133 / _62_m0[1162u].z) - _62_m0[1161u].x) / (_62_m0[1161u].y - _62_m0[1161u].x), 0.0f, 1.0f);
        float _1533;
        float _1534;
        float _1535;
        float _1536;
        float _1537;
        float _1538;
        if (_991)
        {
            _1533 = _62_m0[107u].w;
            _1534 = _62_m0[106u].z;
            _1535 = _62_m0[1u].x;
            _1536 = _62_m0[1u].y;
            _1537 = _62_m0[1u].z;
            _1538 = _57_m0[66u].z;
        }
        else
        {
            _1533 = ((_62_m0[1161u].w - _62_m0[1161u].z) * _1163) + _62_m0[1161u].z;
            _1534 = _62_m0[_1144].y;
            _1535 = _62_m0[_1135].x;
            _1536 = _62_m0[_1135].y;
            _1537 = _62_m0[_1135].z;
            _1538 = _57_m0[66u].y;
        }
        uint4 _1542 = asuint(_57_m0[72u]);
        bool _1546 = (_991 ? _1542.y : _1542.z) != 0u;
        float _1547 = (-0.0f) - _361;
        float _1548 = (-0.0f) - _362;
        float _1549 = (-0.0f) - _363;
        float _1553 = rsqrt(dot(float3(_1547, _1548, _1549), float3(_1547, _1548, _1549)));
        float _1554 = _1553 * _1547;
        float _1555 = _1553 * _1548;
        float _1556 = _1553 * _1549;
        float _1607;
        float _1608;
        uint _1609;
        if ((_818 == 0u) || (_57_m0[178u].w == 0.0f))
        {
            _1607 = 0.0f;
            _1608 = 0.0f;
            _1609 = 0u;
        }
        else
        {
            uint frontier_phi_41_42_ladder;
            float frontier_phi_41_42_ladder_1;
            float frontier_phi_41_42_ladder_2;
            if (_27[21u].SampleLevel(_70, float2(_1126, _1127), 0.0f).x < _293)
            {
                frontier_phi_41_42_ladder = 1u;
                frontier_phi_41_42_ladder_1 = _1127;
                frontier_phi_41_42_ladder_2 = _1126;
            }
            else
            {
                frontier_phi_41_42_ladder = 0u;
                frontier_phi_41_42_ladder_1 = 0.0f;
                frontier_phi_41_42_ladder_2 = 0.0f;
            }
            _1607 = frontier_phi_41_42_ladder_2;
            _1608 = frontier_phi_41_42_ladder_1;
            _1609 = frontier_phi_41_42_ladder;
        }
        float _1618 = clamp((_1133 - _62_m0[1164u].y) / (_62_m0[1164u].z - _62_m0[1164u].y), 0.0f, 1.0f);
        float _1629 = _62_m0[1163u].w - (((_1618 * _1618) * (3.0f - (_1618 * 2.0f))) * _62_m0[1163u].w);
        float4 _1642 = _15[585u].SampleLevel(_65, float2(_296 / _62_m0[1140u].x, _298 / _62_m0[1140u].y), 0.0f);
        bool _1657 = (_406 & 50331648u) == 0u;
        float _1664 = (_1642.w + (-1.0f)) * _1629;
        float _1665 = _1664 + 1.0f;
        float _1666 = _1657 ? _1665 : 1.0f;
        float4 _1671 = _27[14u].SampleLevel(_65, float2(_1126, _1127), 0.0f);
        float _1673 = _1671.x;
        float4 _1678 = _27[13u].SampleLevel(_65, float2(_1126, _1127), 0.0f);
        float _1686 = _1678.x / _57_m0[59u].x;
        float _1687 = _1678.y / _57_m0[59u].x;
        float _1688 = _1678.z / _57_m0[59u].x;
        float _1693 = _1686 - (_1686 * _505);
        float _1694 = _1687 - (_1687 * _505);
        float _1695 = _1688 - (_1688 * _505);
        float _1697 = ((_1673 * _1673) + (-1.0f)) * asfloat(_9.Load((_8.Load((_398 * 4u) + 2u).x * 57u) + 19u).x);
        float _1698 = _1697 + 1.0f;
        float _1804;
        if (asuint(_62_m0[1154u]).z == 0u)
        {
            _1804 = 1.0f;
        }
        else
        {
            _1804 = (_57_m0[160u].w * _1664) + 1.0f;
        }
        float _1806 = _1666 * _882;
        float _1950;
        if (_991)
        {
            float _1942 = _369 - _62_m0[114u].x;
            float _1943 = _370 - _62_m0[114u].y;
            float _1944 = _371 - _62_m0[114u].z;
            float frontier_phi_56_55_ladder;
            if (dot(float3(_1942, _1943, _1944), float3(_1942, _1943, _1944)) < (_62_m0[113u].x * _62_m0[113u].x))
            {
                float4 _2025 = _21.SampleLevel(_66, float3((dot(float3(_1942, _1943, _1944), float3(_62_m0[115u].xyz)) * _62_m0[118u].x) + 0.5f, (dot(float3(_1942, _1943, _1944), float3(_62_m0[116u].xyz)) * _62_m0[118u].y) + 0.5f, (dot(float3(_1942, _1943, _1944), float3(_62_m0[117u].xyz)) * _62_m0[118u].z) + 0.5f), 0.0f);
                float _2030 = _2025.w;
                float _2040 = _2030 * _2030;
                float _2041 = _2040 * 2.0f;
                frontier_phi_56_55_ladder = 1.0f - (exp2(log2(clamp((dot(float3((_2041 * _2025.x) - _2040, (_2041 * _2025.y) - _2040, (_2041 * _2025.z) - _2040), float3(dot(float3(_822, _827, _832), float3(_62_m0[115u].xyz)), dot(float3(_822, _827, _832), float3(_62_m0[116u].xyz)), dot(float3(_822, _827, _832), float3(_62_m0[117u].xyz)))) * _62_m0[113u].z) + _2040, 0.0f, 1.0f)) * _62_m0[113u].y) * _62_m0[113u].w);
            }
            else
            {
                frontier_phi_56_55_ladder = 1.0f;
            }
            _1950 = frontier_phi_56_55_ladder;
        }
        else
        {
            _1950 = 1.0f;
        }
        float _1952 = _1950 * _1806;
        float _1953 = _1952 * _1698;
        float _1955 = _1657 ? _1953 : (_1953 * _1665);
        float _1958 = ((1.0f - _1955) * _505) + _1955;
        float _1966 = (1.0f - _882) - ((_1952 - _882) * asfloat(_10.Load((_414 * 115u) + 112u).x));
        float _1980 = (_57_m0[84u].z * _1697) + 1.0f;
        float _1981 = (-0.0f) - _1554;
        float _1982 = (-0.0f) - _1555;
        float _1983 = (-0.0f) - _1556;
        float _1984 = dot(float3(_1981, _1982, _1983), float3(_837, _840, _843));
        float _2199;
        float _2201;
        if (_991)
        {
            float _2058 = _1984 * 2.0f;
            float _2078 = max(max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533), 0.00999999977648258209228515625f);
            float _2080 = sqrt(1.0f - _887);
            float _2084 = exp2((_2078 * _2078) * (-3.321929931640625f));
            float _2090 = acos(_2080);
            float _2091 = acos(_2084);
            float _2092 = acos((dot(float3(_837, _840, _843), float3(_1981 - (_2058 * _837), _1982 - (_2058 * _840), _1983 - (_2058 * _843))) * 0.5f) + 0.5f);
            float _2256;
            if (_2092 > (max(_2090, _2091) - min(_2090, _2091)))
            {
                float _2193 = _2091 + _2090;
                float frontier_phi_70_64_ladder;
                if (_2092 < _2193)
                {
                    float _2239 = abs(_2090 - _2091);
                    float _2247 = clamp(1.0f - clamp((_2092 - _2239) / max(_2193 - _2239, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f), 0.0f, 1.0f);
                    frontier_phi_70_64_ladder = ((_2247 * _2247) * (3.0f - (_2247 * 2.0f))) * (6.283185482025146484375f - (max(_2080, _2084) * 6.283185482025146484375f));
                }
                else
                {
                    frontier_phi_70_64_ladder = 0.0f;
                }
                _2256 = frontier_phi_70_64_ladder;
            }
            else
            {
                _2256 = 6.283185482025146484375f - (max(_2080, _2084) * 6.283185482025146484375f);
            }
            float _2200 = ((_1666 * _1666) * ((asuint(_57_m0[156u]).z != 0u) ? 1.0f : _1980)) * (_2256 / ((1.0f - _2084) * 6.283185482025146484375f));
            _2199 = _2200;
            _2201 = _2200;
        }
        else
        {
            float _2113 = clamp((((_57_m0[73u].z * (_1980 - _1952)) + _1952) - _57_m0[62u].x) / (_57_m0[62u].y - _57_m0[62u].x), 0.0f, 1.0f) * _887;
            float _2125 = clamp(((_1555 + ((_1984 * 2.0f) * _840)) - _57_m0[75u].x) / (_57_m0[75u].y - _57_m0[75u].x), 0.0f, 1.0f);
            _2199 = (((((_2125 * _2125) * (3.0f - (_2125 * 2.0f))) + (-1.0f)) * _57_m0[75u].z) + 1.0f) * _2113;
            _2201 = _2113;
        }
        float _2202 = _1804 * _1806;
        float _2203 = _2202 * _1693;
        float _2204 = _2202 * _1694;
        float _2205 = _2202 * _1695;
        float _2212 = (((_1642.x / _57_m0[59u].x) - _2203) * _1629) + _2203;
        float _2213 = (((_1642.y / _57_m0[59u].x) - _2204) * _1629) + _2204;
        float _2214 = (((_1642.z / _57_m0[59u].x) - _2205) * _1629) + _2205;
        float _2221 = ((_57_m0[128u].z * (_1950 + (-1.0f))) + 1.0f) * _888;
        float _2223 = (_995 && _992) ? _931 : 1.0f;
        float _2265;
        float _2270;
        float _2275;
        if ((_431 & 64u) == 0u)
        {
            float frontier_phi_72_71_ladder;
            float frontier_phi_72_71_ladder_1;
            float frontier_phi_72_71_ladder_2;
            if ((_989 & 4194304u) == 0u)
            {
                float frontier_phi_72_71_ladder_76_ladder;
                float frontier_phi_72_71_ladder_76_ladder_1;
                float frontier_phi_72_71_ladder_76_ladder_2;
                if ((_989 & 67108864u) == 0u)
                {
                    float frontier_phi_72_71_ladder_76_ladder_82_ladder;
                    float frontier_phi_72_71_ladder_76_ladder_82_ladder_1;
                    float frontier_phi_72_71_ladder_76_ladder_82_ladder_2;
                    if ((_989 & 50331648u) == 0u)
                    {
                        frontier_phi_72_71_ladder_76_ladder_82_ladder = (1.0f - _865) * _850;
                        frontier_phi_72_71_ladder_76_ladder_82_ladder_1 = (1.0f - _870) * _855;
                        frontier_phi_72_71_ladder_76_ladder_82_ladder_2 = (1.0f - _873) * _860;
                    }
                    else
                    {
                        float _2449 = ((_876 * 0.75f) + 1.25f) + ((((_978 * _978) * 9000.0f) * _976) * _978);
                        float _2475 = (_982 * 0.3183098733425140380859375f) * (sqrt(((_972 * _972) + (_970 * _970)) + (_974 * _974)) + 1.0f);
                        frontier_phi_72_71_ladder_76_ladder_82_ladder = exp2(log2(exp2(_970 * (-4.616624355316162109375f))) * _2449) * _2475;
                        frontier_phi_72_71_ladder_76_ladder_82_ladder_1 = exp2(log2(exp2(_972 * (-4.616624355316162109375f))) * _2449) * _2475;
                        frontier_phi_72_71_ladder_76_ladder_82_ladder_2 = exp2(log2(exp2(_974 * (-4.616624355316162109375f))) * _2449) * _2475;
                    }
                    frontier_phi_72_71_ladder_76_ladder = frontier_phi_72_71_ladder_76_ladder_82_ladder;
                    frontier_phi_72_71_ladder_76_ladder_1 = frontier_phi_72_71_ladder_76_ladder_82_ladder_1;
                    frontier_phi_72_71_ladder_76_ladder_2 = frontier_phi_72_71_ladder_76_ladder_82_ladder_2;
                }
                else
                {
                    float _2352 = (((_37[NonUniformResourceIndex(_966 + 0u)].SampleLevel(_69, float2(_950, _952), 0.0f).x * _962) + (-1.0f)) * _960) + 1.0f;
                    frontier_phi_72_71_ladder_76_ladder = ((1.0f - _865) * _850) * _2352;
                    frontier_phi_72_71_ladder_76_ladder_1 = ((1.0f - _870) * _855) * _2352;
                    frontier_phi_72_71_ladder_76_ladder_2 = ((1.0f - _873) * _860) * _2352;
                }
                frontier_phi_72_71_ladder = frontier_phi_72_71_ladder_76_ladder;
                frontier_phi_72_71_ladder_1 = frontier_phi_72_71_ladder_76_ladder_1;
                frontier_phi_72_71_ladder_2 = frontier_phi_72_71_ladder_76_ladder_2;
            }
            else
            {
                frontier_phi_72_71_ladder = ((1.0f - _865) - (_929 * (_923 - _865))) * _850;
                frontier_phi_72_71_ladder_1 = ((1.0f - _870) - (_929 * (_925 - _870))) * _855;
                frontier_phi_72_71_ladder_2 = ((1.0f - _873) - (_929 * (_927 - _873))) * _860;
            }
            _2265 = frontier_phi_72_71_ladder;
            _2270 = frontier_phi_72_71_ladder_1;
            _2275 = frontier_phi_72_71_ladder_2;
        }
        else
        {
            _2265 = _800;
            _2270 = _802;
            _2275 = _804;
        }
        float _2356;
        float _2367;
        float _2378;
        float _2389;
        float _2395;
        float _2401;
        if ((_431 & 1u) == 0u)
        {
            bool _2313 = (_989 & 4194304u) == 0u;
            float frontier_phi_86_78_ladder;
            float frontier_phi_86_78_ladder_1;
            float frontier_phi_86_78_ladder_2;
            float frontier_phi_86_78_ladder_3;
            float frontier_phi_86_78_ladder_4;
            float frontier_phi_86_78_ladder_5;
            if (_991)
            {
                float frontier_phi_86_78_ladder_84_ladder;
                float frontier_phi_86_78_ladder_84_ladder_1;
                float frontier_phi_86_78_ladder_84_ladder_2;
                float frontier_phi_86_78_ladder_84_ladder_3;
                float frontier_phi_86_78_ladder_84_ladder_4;
                float frontier_phi_86_78_ladder_84_ladder_5;
                if (_2313)
                {
                    float frontier_phi_86_78_ladder_84_ladder_92_ladder;
                    float frontier_phi_86_78_ladder_84_ladder_92_ladder_1;
                    float frontier_phi_86_78_ladder_84_ladder_92_ladder_2;
                    float frontier_phi_86_78_ladder_84_ladder_92_ladder_3;
                    float frontier_phi_86_78_ladder_84_ladder_92_ladder_4;
                    float frontier_phi_86_78_ladder_84_ladder_92_ladder_5;
                    if ((_989 & 67108864u) == 0u)
                    {
                        float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder;
                        float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_1;
                        float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_2;
                        float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_3;
                        float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_4;
                        float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_5;
                        if ((_989 & 50331648u) == 0u)
                        {
                            float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder;
                            float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_1;
                            float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_2;
                            float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_3;
                            float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_4;
                            float frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_5;
                            if ((_989 & 1032192u) == 0u)
                            {
                                float4 _2835 = _34[6u].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                                float _2390 = _2835.x * _57_m0[59u].y;
                                float _2396 = _2835.y * _57_m0[59u].y;
                                float _2402 = _2835.z * _57_m0[59u].y;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder = _2402;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_1 = _2396;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_2 = _2390;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_3 = ((1.0f - _873) * _860) * _2402;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_4 = ((1.0f - _870) * _855) * _2396;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_5 = ((1.0f - _865) * _850) * _2390;
                            }
                            else
                            {
                                float4 _2849 = _34[6u].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                                float _2391 = _2849.x * _57_m0[59u].y;
                                float _2397 = _2849.y * _57_m0[59u].y;
                                float _2403 = _2849.z * _57_m0[59u].y;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder = _2403;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_1 = _2397;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_2 = _2391;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_3 = ((1.0f - _873) * _860) * _2403;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_4 = ((1.0f - _870) * _855) * _2397;
                                frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_5 = ((1.0f - _865) * _850) * _2391;
                            }
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_1 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_1;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_2 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_2;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_3 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_3;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_4 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_4;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_5 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_107_ladder_5;
                        }
                        else
                        {
                            float _2695 = ((_876 * 0.75f) + 1.25f) + ((((_978 * _978) * 9000.0f) * _976) * _978);
                            float4 _2721 = _34[6u].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                            float _2392 = _2721.x * _57_m0[59u].y;
                            float _2398 = _2721.y * _57_m0[59u].y;
                            float _2404 = _2721.z * _57_m0[59u].y;
                            float _2727 = (_982 * 0.3183098733425140380859375f) * (sqrt(((_972 * _972) + (_970 * _970)) + (_974 * _974)) + 1.0f);
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder = _2404;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_1 = _2398;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_2 = _2392;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_3 = (exp2(log2(exp2(_974 * (-4.616624355316162109375f))) * _2695) * _2727) * _2404;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_4 = (exp2(log2(exp2(_972 * (-4.616624355316162109375f))) * _2695) * _2727) * _2398;
                            frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_5 = (exp2(log2(exp2(_970 * (-4.616624355316162109375f))) * _2695) * _2727) * _2392;
                        }
                        frontier_phi_86_78_ladder_84_ladder_92_ladder = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_1 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_1;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_2 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_2;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_3 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_3;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_4 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_4;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_5 = frontier_phi_86_78_ladder_84_ladder_92_ladder_100_ladder_5;
                    }
                    else
                    {
                        float4 _2605 = _34[6u].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                        float _2393 = _2605.x * _57_m0[59u].y;
                        float _2399 = _2605.y * _57_m0[59u].y;
                        float _2405 = _2605.z * _57_m0[59u].y;
                        float _2628 = (((_37[NonUniformResourceIndex(_966 + 0u)].SampleLevel(_69, float2(_950, _952), 0.0f).x * _962) + (-1.0f)) * _960) + 1.0f;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder = _2405;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_1 = _2399;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_2 = _2393;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_3 = (((1.0f - _873) * _860) * _2405) * _2628;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_4 = (((1.0f - _870) * _855) * _2399) * _2628;
                        frontier_phi_86_78_ladder_84_ladder_92_ladder_5 = (((1.0f - _865) * _850) * _2393) * _2628;
                    }
                    frontier_phi_86_78_ladder_84_ladder = frontier_phi_86_78_ladder_84_ladder_92_ladder;
                    frontier_phi_86_78_ladder_84_ladder_1 = frontier_phi_86_78_ladder_84_ladder_92_ladder_1;
                    frontier_phi_86_78_ladder_84_ladder_2 = frontier_phi_86_78_ladder_84_ladder_92_ladder_2;
                    frontier_phi_86_78_ladder_84_ladder_3 = frontier_phi_86_78_ladder_84_ladder_92_ladder_3;
                    frontier_phi_86_78_ladder_84_ladder_4 = frontier_phi_86_78_ladder_84_ladder_92_ladder_4;
                    frontier_phi_86_78_ladder_84_ladder_5 = frontier_phi_86_78_ladder_84_ladder_92_ladder_5;
                }
                else
                {
                    float4 _2483 = _34[6u].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                    float _2394 = _2483.x * _57_m0[59u].y;
                    float _2400 = _2483.y * _57_m0[59u].y;
                    float _2406 = _2483.z * _57_m0[59u].y;
                    float4 _2497 = _15[502u].SampleLevel(_65, float2(clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f), 1.0f - _876), 0.0f);
                    float _2499 = _2497.x;
                    frontier_phi_86_78_ladder_84_ladder = _2406;
                    frontier_phi_86_78_ladder_84_ladder_1 = _2400;
                    frontier_phi_86_78_ladder_84_ladder_2 = _2394;
                    frontier_phi_86_78_ladder_84_ladder_3 = (_2406 * _860) * ((1.0f - _873) - (((_2499 * _927) - _873) * _929));
                    frontier_phi_86_78_ladder_84_ladder_4 = (_2400 * _855) * ((1.0f - _870) - (((_2499 * _925) - _870) * _929));
                    frontier_phi_86_78_ladder_84_ladder_5 = (_2394 * _850) * ((1.0f - _865) - (((_2499 * _923) - _865) * _929));
                }
                frontier_phi_86_78_ladder = frontier_phi_86_78_ladder_84_ladder;
                frontier_phi_86_78_ladder_1 = frontier_phi_86_78_ladder_84_ladder_1;
                frontier_phi_86_78_ladder_2 = frontier_phi_86_78_ladder_84_ladder_2;
                frontier_phi_86_78_ladder_3 = frontier_phi_86_78_ladder_84_ladder_3;
                frontier_phi_86_78_ladder_4 = frontier_phi_86_78_ladder_84_ladder_4;
                frontier_phi_86_78_ladder_5 = frontier_phi_86_78_ladder_84_ladder_5;
            }
            else
            {
                uint _2355 = (_419 + 50u) + 0u;
                float frontier_phi_86_78_ladder_85_ladder;
                float frontier_phi_86_78_ladder_85_ladder_1;
                float frontier_phi_86_78_ladder_85_ladder_2;
                float frontier_phi_86_78_ladder_85_ladder_3;
                float frontier_phi_86_78_ladder_85_ladder_4;
                float frontier_phi_86_78_ladder_85_ladder_5;
                if (_2313)
                {
                    float frontier_phi_86_78_ladder_85_ladder_94_ladder;
                    float frontier_phi_86_78_ladder_85_ladder_94_ladder_1;
                    float frontier_phi_86_78_ladder_85_ladder_94_ladder_2;
                    float frontier_phi_86_78_ladder_85_ladder_94_ladder_3;
                    float frontier_phi_86_78_ladder_85_ladder_94_ladder_4;
                    float frontier_phi_86_78_ladder_85_ladder_94_ladder_5;
                    if ((_989 & 67108864u) == 0u)
                    {
                        float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder;
                        float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_1;
                        float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_2;
                        float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_3;
                        float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_4;
                        float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_5;
                        if ((_989 & 50331648u) == 0u)
                        {
                            float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder;
                            float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_1;
                            float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_2;
                            float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_3;
                            float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_4;
                            float frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_5;
                            if ((_989 & 1032192u) == 0u)
                            {
                                float4 _2863 = _34[NonUniformResourceIndex(_2355)].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder = 0.0f;
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_1 = 0.0f;
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_2 = 0.0f;
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_3 = (_2863.z * _57_m0[59u].y) * ((1.0f - _873) * _860);
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_4 = (_2863.y * _57_m0[59u].y) * ((1.0f - _870) * _855);
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_5 = (_2863.x * _57_m0[59u].y) * ((1.0f - _865) * _850);
                            }
                            else
                            {
                                float4 _2880 = _34[NonUniformResourceIndex(_2355)].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder = 0.0f;
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_1 = 0.0f;
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_2 = 0.0f;
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_3 = ((_57_m0[59u].y * _860) * (1.0f - _873)) * _2880.z;
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_4 = ((_57_m0[59u].y * _855) * (1.0f - _870)) * _2880.y;
                                frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_5 = ((_57_m0[59u].y * _850) * (1.0f - _865)) * _2880.x;
                            }
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_1 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_1;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_2 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_2;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_3 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_3;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_4 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_4;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_5 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_109_ladder_5;
                        }
                        else
                        {
                            float _2739 = ((_876 * 0.75f) + 1.25f) + ((((_978 * _978) * 9000.0f) * _976) * _978);
                            float4 _2765 = _34[NonUniformResourceIndex(_2355)].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                            float _2772 = ((_982 * 0.3183098733425140380859375f) * (sqrt(((_972 * _972) + (_970 * _970)) + (_974 * _974)) + 1.0f)) * _57_m0[59u].y;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder = 0.0f;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_1 = 0.0f;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_2 = 0.0f;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_3 = (_2772 * exp2(log2(exp2(_974 * (-4.616624355316162109375f))) * _2739)) * _2765.z;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_4 = (_2772 * exp2(log2(exp2(_972 * (-4.616624355316162109375f))) * _2739)) * _2765.y;
                            frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_5 = (_2772 * exp2(log2(exp2(_970 * (-4.616624355316162109375f))) * _2739)) * _2765.x;
                        }
                        frontier_phi_86_78_ladder_85_ladder_94_ladder = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_1 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_1;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_2 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_2;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_3 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_3;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_4 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_4;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_5 = frontier_phi_86_78_ladder_85_ladder_94_ladder_102_ladder_5;
                    }
                    else
                    {
                        float4 _2635 = _34[NonUniformResourceIndex(_2355)].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                        float _2653 = ((((_37[NonUniformResourceIndex(_966 + 0u)].SampleLevel(_69, float2(_950, _952), 0.0f).x * _962) + (-1.0f)) * _960) + 1.0f) * _57_m0[59u].y;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder = 0.0f;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_1 = 0.0f;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_2 = 0.0f;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_3 = (((1.0f - _873) * _860) * _2635.z) * _2653;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_4 = (((1.0f - _870) * _855) * _2635.y) * _2653;
                        frontier_phi_86_78_ladder_85_ladder_94_ladder_5 = (((1.0f - _865) * _850) * _2635.x) * _2653;
                    }
                    frontier_phi_86_78_ladder_85_ladder = frontier_phi_86_78_ladder_85_ladder_94_ladder;
                    frontier_phi_86_78_ladder_85_ladder_1 = frontier_phi_86_78_ladder_85_ladder_94_ladder_1;
                    frontier_phi_86_78_ladder_85_ladder_2 = frontier_phi_86_78_ladder_85_ladder_94_ladder_2;
                    frontier_phi_86_78_ladder_85_ladder_3 = frontier_phi_86_78_ladder_85_ladder_94_ladder_3;
                    frontier_phi_86_78_ladder_85_ladder_4 = frontier_phi_86_78_ladder_85_ladder_94_ladder_4;
                    frontier_phi_86_78_ladder_85_ladder_5 = frontier_phi_86_78_ladder_85_ladder_94_ladder_5;
                }
                else
                {
                    float4 _2524 = _34[NonUniformResourceIndex(_2355)].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                    float4 _2537 = _15[502u].SampleLevel(_65, float2(clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f), 1.0f - _876), 0.0f);
                    float _2539 = _2537.x;
                    frontier_phi_86_78_ladder_85_ladder = 0.0f;
                    frontier_phi_86_78_ladder_85_ladder_1 = 0.0f;
                    frontier_phi_86_78_ladder_85_ladder_2 = 0.0f;
                    frontier_phi_86_78_ladder_85_ladder_3 = ((_57_m0[59u].y * _860) * _2524.z) * ((1.0f - _873) - (((_2539 * _927) - _873) * _929));
                    frontier_phi_86_78_ladder_85_ladder_4 = ((_57_m0[59u].y * _855) * _2524.y) * ((1.0f - _870) - (((_2539 * _925) - _870) * _929));
                    frontier_phi_86_78_ladder_85_ladder_5 = ((_57_m0[59u].y * _850) * _2524.x) * ((1.0f - _865) - (((_2539 * _923) - _865) * _929));
                }
                frontier_phi_86_78_ladder = frontier_phi_86_78_ladder_85_ladder;
                frontier_phi_86_78_ladder_1 = frontier_phi_86_78_ladder_85_ladder_1;
                frontier_phi_86_78_ladder_2 = frontier_phi_86_78_ladder_85_ladder_2;
                frontier_phi_86_78_ladder_3 = frontier_phi_86_78_ladder_85_ladder_3;
                frontier_phi_86_78_ladder_4 = frontier_phi_86_78_ladder_85_ladder_4;
                frontier_phi_86_78_ladder_5 = frontier_phi_86_78_ladder_85_ladder_5;
            }
            _2356 = frontier_phi_86_78_ladder_5;
            _2367 = frontier_phi_86_78_ladder_4;
            _2378 = frontier_phi_86_78_ladder_3;
            _2389 = frontier_phi_86_78_ladder_2;
            _2395 = frontier_phi_86_78_ladder_1;
            _2401 = frontier_phi_86_78_ladder;
        }
        else
        {
            _2356 = _806 / _57_m0[59u].x;
            _2367 = _808 / _57_m0[59u].x;
            _2378 = _810 / _57_m0[59u].x;
            _2389 = 0.0f;
            _2395 = 0.0f;
            _2401 = 0.0f;
        }
        float _2561;
        float _2563;
        float _2565;
        if ((_431 & 65u) == 0u)
        {
            _2561 = 0.0f;
            _2563 = 0.0f;
            _2565 = 0.0f;
        }
        else
        {
            _2561 = _812 / _57_m0[59u].x;
            _2563 = _814 / _57_m0[59u].x;
            _2565 = _816 / _57_m0[59u].x;
        }
        float _2660;
        float _2663;
        float _2666;
        if ((((_432 & 64u) | (_406 & 536870912u)) == 0u) || (int(_988) < int(0u)))
        {
            _2660 = 0.0f;
            _2663 = 0.0f;
            _2666 = 0.0f;
        }
        else
        {
            float frontier_phi_104_105_ladder;
            float frontier_phi_104_105_ladder_1;
            float frontier_phi_104_105_ladder_2;
            if ((_989 & 536870912u) == 0u)
            {
                float4 _2784 = _34[6u].SampleLevel(_69, float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), 0.0f);
                float _2792 = (_1958 * _908) * _57_m0[59u].y;
                frontier_phi_104_105_ladder = (_2784.z * _903) * _2792;
                frontier_phi_104_105_ladder_1 = (_2784.y * _900) * _2792;
                frontier_phi_104_105_ladder_2 = (_2784.x * _897) * _2792;
            }
            else
            {
                float _2797 = _1952 * (1.0f - _1629);
                float4 _2805 = _34[6u].SampleLevel(_69, float3(_822, _827, _832), 0.0f);
                float _2816 = _57_m0[59u].y * _1958;
                frontier_phi_104_105_ladder = (_908 * _903) * (((((1.0f - _873) * _860) * _2805.z) * _2816) + (_2797 * _1695));
                frontier_phi_104_105_ladder_1 = (_908 * _900) * (((((1.0f - _870) * _855) * _2805.y) * _2816) + (_2797 * _1694));
                frontier_phi_104_105_ladder_2 = (_908 * _897) * (((((1.0f - _865) * _850) * _2805.x) * _2816) + (_2797 * _1693));
            }
            _2660 = frontier_phi_104_105_ladder_2;
            _2663 = frontier_phi_104_105_ladder_1;
            _2666 = frontier_phi_104_105_ladder;
        }
        float _2673 = _1958 * (1.0f - _1629);
        bool _2684 = asuint(_57_m0[156u]).z == 0u;
        float _4090;
        float _4092;
        float _4094;
        if (_991)
        {
            uint _2894;
            float _2895;
            float _2897;
            float _2899;
            if (_2684)
            {
                _2894 = 0u;
                _2895 = 0.0f;
                _2897 = 0.0f;
                _2899 = 0.0f;
            }
            else
            {
                float _2940 = (_2389 * _1698) + _1693;
                float _2941 = (_2395 * _1698) + _1694;
                float _2942 = (_2401 * _1698) + _1695;
                float _2955 = (((_2212 - _2940) * _1629) + _2940) / _57_m0[59u].w;
                float _2956 = (((_2213 - _2941) * _1629) + _2941) / _57_m0[59u].w;
                float _2957 = (((_2214 - _2942) * _1629) + _2942) / _57_m0[59u].w;
                float _2961 = dot(float3(_2955, _2956, _2957), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                float _2985 = dot(float3(_34[5u].Sample(_69, float3(_822, _827, _832)).xyz), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                _2894 = 1u;
                _2895 = ((((((_2955 - _2961) * _57_m0[156u].w) + _2961) / _2985) + (-1.0f)) * _57_m0[169u].w) + 1.0f;
                _2897 = ((((((_2956 - _2961) * _57_m0[156u].w) + _2961) / _2985) + (-1.0f)) * _57_m0[169u].w) + 1.0f;
                _2899 = ((((((_2957 - _2961) * _57_m0[156u].w) + _2961) / _2985) + (-1.0f)) * _57_m0[169u].w) + 1.0f;
            }
            float _2934 = ((((exp2(log2(clamp((_1133 - _57_m0[121u].y) * _57_m0[121u].z, 0.0f, 1.0f)) * _57_m0[121u].w) * asfloat(_10.Load((_414 * 115u) + 114u).x)) * exp2(log2(clamp((_362 - _57_m0[122u].x) * _57_m0[122u].y, 0.0f, 1.0f)) * _57_m0[122u].z)) * (1.0f - clamp(_827, 0.0f, 1.0f))) * (max(_57_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
            float _3840;
            float _3846;
            float _3852;
            if ((_989 & 4194304u) == 0u)
            {
                float frontier_phi_175_123_ladder;
                float frontier_phi_175_123_ladder_1;
                float frontier_phi_175_123_ladder_2;
                if ((_989 & 8388608u) == 0u)
                {
                    float frontier_phi_175_123_ladder_127_ladder;
                    float frontier_phi_175_123_ladder_127_ladder_1;
                    float frontier_phi_175_123_ladder_127_ladder_2;
                    if ((_989 & 50331648u) == 0u)
                    {
                        float frontier_phi_175_123_ladder_127_ladder_133_ladder;
                        float frontier_phi_175_123_ladder_127_ladder_133_ladder_1;
                        float frontier_phi_175_123_ladder_127_ladder_133_ladder_2;
                        if ((_989 & 1032192u) == 0u)
                        {
                            float frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder;
                            float frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder_1;
                            float frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder_2;
                            if ((_988 & 16u) == 0u)
                            {
                                float _3512 = _1984 * 2.0f;
                                float _3516 = _1981 - (_3512 * _837);
                                float _3518 = _1983 - (_3512 * _843);
                                float _3519 = (_1982 - (_3512 * _840)) * _2934;
                                float _3523 = rsqrt(dot(float3(_3516, _3519, _3518), float3(_3516, _3519, _3518)));
                                float _3524 = _3516 * _3523;
                                float _3525 = _3519 * _3523;
                                float _3526 = _3518 * _3523;
                                float _3891;
                                float _3892;
                                float _3893;
                                float _3894;
                                if (_57_m0[176u].w > 0.0f)
                                {
                                    float _3747 = 1.0f - _876;
                                    float _3750 = exp2(log2(_3747) * _57_m0[176u].w);
                                    float _3752 = clamp(1.0f - _3750, 0.0f, 1.0f);
                                    float _3756 = (sqrt(_3752 + 9.9999997473787516355514526367188e-05f) + _3750) * _3752;
                                    float _3763 = (_3756 * (_3524 - _837)) + _837;
                                    float _3764 = (_3756 * (_3525 - _840)) + _840;
                                    float _3765 = (_3756 * (_3526 - _843)) + _843;
                                    float _3769 = rsqrt(dot(float3(_3763, _3764, _3765), float3(_3763, _3764, _3765)));
                                    _3891 = _3747;
                                    _3892 = _3763 * _3769;
                                    _3893 = _3764 * _3769;
                                    _3894 = _3765 * _3769;
                                }
                                else
                                {
                                    _3891 = 1.0f - _876;
                                    _3892 = _3524;
                                    _3893 = _3525;
                                    _3894 = _3526;
                                }
                                float _3896 = (_1538 >= 0.0f) ? _1538 : 8.0f;
                                float _4507;
                                if (_1546)
                                {
                                    _4507 = _3896 * _3891;
                                }
                                else
                                {
                                    _4507 = max((_3896 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_3891, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                                }
                                float4 _4512 = _34[4u].SampleLevel(_67, float3(_3892, _3893, _3894), _4507);
                                float _4786;
                                float _4788;
                                float _4790;
                                float _4792;
                                if (_1609 == 0u)
                                {
                                    _4786 = 0.0f;
                                    _4788 = 0.0f;
                                    _4790 = 0.0f;
                                    _4792 = 0.0f;
                                }
                                else
                                {
                                    float _4806 = min(_1607, _57_m0[110u].z);
                                    float _4807 = min(_1608, _57_m0[110u].w);
                                    float4 _4823 = _15[509u].SampleLevel(_65, float2(_4806, _4807), _15[557u].SampleLevel(_65, float2(_4806, _4807), 0.0f).x * _57_m0[72u].w);
                                    float _4825 = _4823.x;
                                    float _4826 = _4823.y;
                                    float _4827 = _4823.z;
                                    float _4844 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4825, max(_4826, _4827))));
                                    _4786 = _4844 * _4825;
                                    _4788 = _4844 * _4826;
                                    _4790 = _4844 * _4827;
                                    _4792 = _4823.w;
                                }
                                float _4794 = 1.0f - _4792;
                                float _4798 = (_4794 * _4512.x) + _4786;
                                float _4799 = (_4794 * _4512.y) + _4788;
                                float _4800 = (_4794 * _4512.z) + _4790;
                                float _5016;
                                float _5018;
                                float _5020;
                                if (_2894 == 0u)
                                {
                                    _5016 = _4798;
                                    _5018 = _4799;
                                    _5020 = _4800;
                                }
                                else
                                {
                                    _5016 = min(1.0f, _2895) * _4798;
                                    _5018 = min(1.0f, _2897) * _4799;
                                    _5020 = min(1.0f, _2899) * _4800;
                                }
                                float _5041 = (((min(_876 * 0.4749999940395355224609375f, exp2(clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f) * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _876) + (-0.015625f);
                                float _5042 = ((_876 * 0.25f) + 0.75f) - _5041;
                                frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder = (_57_m0[59u].w * _5020) * clamp((_5042 * _873) + _5041, 0.0f, 1.0f);
                                frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder_1 = (_57_m0[59u].w * _5016) * clamp((_5042 * _865) + _5041, 0.0f, 1.0f);
                                frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder_2 = (_57_m0[59u].w * _5018) * clamp((_5042 * _870) + _5041, 0.0f, 1.0f);
                            }
                            else
                            {
                                float _3539 = _1984 * 2.0f;
                                float _3543 = _1981 - (_3539 * _837);
                                float _3545 = _1983 - (_3539 * _843);
                                float _3546 = (_1982 - (_3539 * _840)) * _2934;
                                float _3550 = rsqrt(dot(float3(_3543, _3546, _3545), float3(_3543, _3546, _3545)));
                                float _3551 = _3543 * _3550;
                                float _3552 = _3546 * _3550;
                                float _3553 = _3545 * _3550;
                                float _3897;
                                float _3898;
                                float _3899;
                                float _3900;
                                if (_57_m0[176u].w > 0.0f)
                                {
                                    float _3774 = 1.0f - _876;
                                    float _3777 = exp2(log2(_3774) * _57_m0[176u].w);
                                    float _3779 = clamp(1.0f - _3777, 0.0f, 1.0f);
                                    float _3783 = (sqrt(_3779 + 9.9999997473787516355514526367188e-05f) + _3777) * _3779;
                                    float _3790 = (_3783 * (_3551 - _837)) + _837;
                                    float _3791 = (_3783 * (_3552 - _840)) + _840;
                                    float _3792 = (_3783 * (_3553 - _843)) + _843;
                                    float _3796 = rsqrt(dot(float3(_3790, _3791, _3792), float3(_3790, _3791, _3792)));
                                    _3897 = _3774;
                                    _3898 = _3790 * _3796;
                                    _3899 = _3791 * _3796;
                                    _3900 = _3792 * _3796;
                                }
                                else
                                {
                                    _3897 = 1.0f - _876;
                                    _3898 = _3551;
                                    _3899 = _3552;
                                    _3900 = _3553;
                                }
                                float _3902 = (_1538 >= 0.0f) ? _1538 : 8.0f;
                                float _4518;
                                if (_1546)
                                {
                                    _4518 = _3902 * _3897;
                                }
                                else
                                {
                                    _4518 = max((_3902 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_3897, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                                }
                                float4 _4523 = _34[4u].SampleLevel(_67, float3(_3898, _3899, _3900), _4518);
                                float _4845;
                                float _4847;
                                float _4849;
                                float _4851;
                                if (_1609 == 0u)
                                {
                                    _4845 = 0.0f;
                                    _4847 = 0.0f;
                                    _4849 = 0.0f;
                                    _4851 = 0.0f;
                                }
                                else
                                {
                                    float _4871 = min((_57_m0[110u].x * (_62_m0[1140u].z * (_837 - _846))) + _1607, _57_m0[110u].z);
                                    float _4872 = min((_57_m0[110u].y * (_62_m0[1140u].w * (_843 - _848))) + _1608, _57_m0[110u].w);
                                    float4 _4888 = _15[509u].SampleLevel(_65, float2(_4871, _4872), _15[557u].SampleLevel(_65, float2(_4871, _4872), 0.0f).x * _57_m0[72u].w);
                                    float _4890 = _4888.x;
                                    float _4891 = _4888.y;
                                    float _4892 = _4888.z;
                                    float _4909 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4890, max(_4891, _4892))));
                                    _4845 = _4909 * _4890;
                                    _4847 = _4909 * _4891;
                                    _4849 = _4909 * _4892;
                                    _4851 = _4888.w;
                                }
                                float _4853 = 1.0f - _4851;
                                float _4857 = (_4853 * _4523.x) + _4845;
                                float _4858 = (_4853 * _4523.y) + _4847;
                                float _4859 = (_4853 * _4523.z) + _4849;
                                float _5055;
                                float _5057;
                                float _5059;
                                if (_2894 == 0u)
                                {
                                    _5055 = _4857;
                                    _5057 = _4858;
                                    _5059 = _4859;
                                }
                                else
                                {
                                    _5055 = min(1.0f, _2895) * _4857;
                                    _5057 = min(1.0f, _2897) * _4858;
                                    _5059 = min(1.0f, _2899) * _4859;
                                }
                                float _5080 = (((min(_876 * 0.4749999940395355224609375f, exp2(clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f) * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _876) + (-0.015625f);
                                float _5081 = ((_876 * 0.25f) + 0.75f) - _5080;
                                frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder = (_57_m0[59u].w * _5059) * clamp((_5081 * _873) + _5080, 0.0f, 1.0f);
                                frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder_1 = (_57_m0[59u].w * _5055) * clamp((_5081 * _865) + _5080, 0.0f, 1.0f);
                                frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder_2 = (_57_m0[59u].w * _5057) * clamp((_5081 * _870) + _5080, 0.0f, 1.0f);
                            }
                            frontier_phi_175_123_ladder_127_ladder_133_ladder = frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder;
                            frontier_phi_175_123_ladder_127_ladder_133_ladder_1 = frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder_1;
                            frontier_phi_175_123_ladder_127_ladder_133_ladder_2 = frontier_phi_175_123_ladder_127_ladder_133_ladder_142_ladder_2;
                        }
                        else
                        {
                            float _3278 = _1984 * 2.0f;
                            float _3282 = _1981 - (_3278 * _837);
                            float _3284 = _1983 - (_3278 * _843);
                            float _3285 = (_1982 - (_3278 * _840)) * _2934;
                            float _3289 = rsqrt(dot(float3(_3282, _3285, _3284), float3(_3282, _3285, _3284)));
                            float _3290 = _3282 * _3289;
                            float _3291 = _3285 * _3289;
                            float _3292 = _3284 * _3289;
                            float _3801;
                            float _3802;
                            float _3803;
                            float _3804;
                            if (_57_m0[176u].w > 0.0f)
                            {
                                float _3558 = 1.0f - _876;
                                float _3561 = exp2(log2(_3558) * _57_m0[176u].w);
                                float _3563 = clamp(1.0f - _3561, 0.0f, 1.0f);
                                float _3567 = (sqrt(_3563 + 9.9999997473787516355514526367188e-05f) + _3561) * _3563;
                                float _3574 = (_3567 * (_3290 - _837)) + _837;
                                float _3575 = (_3567 * (_3291 - _840)) + _840;
                                float _3576 = (_3567 * (_3292 - _843)) + _843;
                                float _3580 = rsqrt(dot(float3(_3574, _3575, _3576), float3(_3574, _3575, _3576)));
                                _3801 = _3558;
                                _3802 = _3574 * _3580;
                                _3803 = _3575 * _3580;
                                _3804 = _3576 * _3580;
                            }
                            else
                            {
                                _3801 = 1.0f - _876;
                                _3802 = _3290;
                                _3803 = _3291;
                                _3804 = _3292;
                            }
                            float _3806 = (_1538 >= 0.0f) ? _1538 : 8.0f;
                            float _4236;
                            if (_1546)
                            {
                                _4236 = _3806 * _3801;
                            }
                            else
                            {
                                _4236 = max((_3806 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_3801, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                            }
                            float4 _4241 = _34[4u].SampleLevel(_67, float3(_3802, _3803, _3804), _4236);
                            float _4529;
                            float _4531;
                            float _4533;
                            float _4535;
                            if (_1609 == 0u)
                            {
                                _4529 = 0.0f;
                                _4531 = 0.0f;
                                _4533 = 0.0f;
                                _4535 = 0.0f;
                            }
                            else
                            {
                                float _4549 = min(_1607, _57_m0[110u].z);
                                float _4550 = min(_1608, _57_m0[110u].w);
                                float4 _4566 = _15[509u].SampleLevel(_65, float2(_4549, _4550), _15[557u].SampleLevel(_65, float2(_4549, _4550), 0.0f).x * _57_m0[72u].w);
                                float _4568 = _4566.x;
                                float _4569 = _4566.y;
                                float _4570 = _4566.z;
                                float _4587 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4568, max(_4569, _4570))));
                                _4529 = _4587 * _4568;
                                _4531 = _4587 * _4569;
                                _4533 = _4587 * _4570;
                                _4535 = _4566.w;
                            }
                            float _4537 = 1.0f - _4535;
                            float _4541 = (_4537 * _4241.x) + _4529;
                            float _4542 = (_4537 * _4241.y) + _4531;
                            float _4543 = (_4537 * _4241.z) + _4533;
                            float _4910;
                            float _4912;
                            float _4914;
                            if (_2894 == 0u)
                            {
                                _4910 = _4541;
                                _4912 = _4542;
                                _4914 = _4543;
                            }
                            else
                            {
                                _4910 = min(1.0f, _2895) * _4541;
                                _4912 = min(1.0f, _2897) * _4542;
                                _4914 = min(1.0f, _2899) * _4543;
                            }
                            float _4935 = (((min(_876 * 0.4749999940395355224609375f, exp2(clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f) * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _876) + (-0.015625f);
                            float _4936 = ((_876 * 0.25f) + 0.75f) - _4935;
                            frontier_phi_175_123_ladder_127_ladder_133_ladder = (_57_m0[59u].w * _4914) * clamp((_4936 * _873) + _4935, 0.0f, 1.0f);
                            frontier_phi_175_123_ladder_127_ladder_133_ladder_1 = (_57_m0[59u].w * _4910) * clamp((_4936 * _865) + _4935, 0.0f, 1.0f);
                            frontier_phi_175_123_ladder_127_ladder_133_ladder_2 = (_57_m0[59u].w * _4912) * clamp((_4936 * _870) + _4935, 0.0f, 1.0f);
                        }
                        frontier_phi_175_123_ladder_127_ladder = frontier_phi_175_123_ladder_127_ladder_133_ladder;
                        frontier_phi_175_123_ladder_127_ladder_1 = frontier_phi_175_123_ladder_127_ladder_133_ladder_1;
                        frontier_phi_175_123_ladder_127_ladder_2 = frontier_phi_175_123_ladder_127_ladder_133_ladder_2;
                    }
                    else
                    {
                        float _3179 = 1.0f - _876;
                        float _3180 = (-0.0f) - _784;
                        float _3181 = (-0.0f) - _778;
                        float _3182 = (-0.0f) - _772;
                        float _3184 = (1.0f - _980) * _3179;
                        float _3187 = _3179 * 0.85000002384185791015625f;
                        float _3198 = (_1538 >= 0.0f) ? _1538 : 8.0f;
                        float _3199 = (_3184 * 0.89999997615814208984375f) + 0.10000002384185791015625f;
                        float _6128;
                        float _6129;
                        float _6130;
                        float _6131;
                        float _6132;
                        float _6133;
                        float _6134;
                        float _6135;
                        float _6136;
                        if (int(_989) < int(0u))
                        {
                            float _3299 = (_840 * _772) - (_843 * _778);
                            float _3302 = (_843 * _784) - (_837 * _772);
                            float _3305 = (_837 * _778) - (_840 * _784);
                            float _3319 = float(int(asuint(_57_m0[677u]).x & 63u)) * 5.588237762451171875f;
                            float _3328 = ((_3319 + float(int(uint(int(_283))))) * 0.067110560834407806396484375f) + ((_3319 + float(int(uint(int(_284))))) * 0.005837149918079376220703125f);
                            float _3332 = frac(abs(_3328));
                            float _3335 = ((_3328 >= ((-0.0f) - _3328)) ? _3332 : ((-0.0f) - _3332)) * 52.98291778564453125f;
                            float _3340 = frac(abs(_3335));
                            float _3345 = (((_3335 >= ((-0.0f) - _3335)) ? _3340 : ((-0.0f) - _3340)) * 1.57079637050628662109375f) + 0.785398185253143310546875f;
                            float _3347 = sin(_3345);
                            float _3352 = cos(_3345) * rsqrt(dot(float3(_3299, _3302, _3305), float3(_3299, _3302, _3305)));
                            float _3356 = (_3352 * _3299) + (_3347 * _837);
                            float _3357 = (_3352 * _3302) + (_3347 * _840);
                            float _3358 = (_3352 * _3305) + (_3347 * _843);
                            float _3362 = rsqrt(dot(float3(_3356, _3357, _3358), float3(_3356, _3357, _3358)));
                            float _3363 = _3356 * _3362;
                            float _3364 = _3357 * _3362;
                            float _3365 = _3358 * _3362;
                            float _3369 = dot(float3(_1981, _1982, _1983), float3(_3363, _3364, _3365)) * 2.0f;
                            float _3373 = _1981 - (_3369 * _3363);
                            float _3374 = _1982 - (_3369 * _3364);
                            float _3375 = _1983 - (_3369 * _3365);
                            float _3807;
                            if (_1546)
                            {
                                _3807 = _3198 * _3199;
                            }
                            else
                            {
                                _3807 = max((_3198 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_3199, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                            }
                            float4 _3812 = _34[4u].SampleLevel(_67, float3(_3373, _3374, _3375), _3807);
                            bool _3817 = _1609 == 0u;
                            float _3913;
                            float _3915;
                            float _3917;
                            float _3919;
                            if (_3817)
                            {
                                _3913 = 0.0f;
                                _3915 = 0.0f;
                                _3917 = 0.0f;
                                _3919 = 0.0f;
                            }
                            else
                            {
                                float _3933 = min(_1607, _57_m0[110u].z);
                                float _3934 = min(_1608, _57_m0[110u].w);
                                float4 _3950 = _15[509u].SampleLevel(_65, float2(_3933, _3934), _15[557u].SampleLevel(_65, float2(_3933, _3934), 0.0f).x * _57_m0[72u].w);
                                float _3952 = _3950.x;
                                float _3953 = _3950.y;
                                float _3954 = _3950.z;
                                float _3971 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_3952, max(_3953, _3954))));
                                _3913 = _3971 * _3952;
                                _3915 = _3971 * _3953;
                                _3917 = _3971 * _3954;
                                _3919 = _3950.w;
                            }
                            float _3921 = 1.0f - _3919;
                            float _3925 = (_3921 * _3812.x) + _3913;
                            float _3926 = (_3921 * _3812.y) + _3915;
                            float _3927 = (_3921 * _3812.z) + _3917;
                            bool _3928 = _2894 == 0u;
                            float _4247;
                            float _4249;
                            float _4251;
                            if (_3928)
                            {
                                _4247 = _3925;
                                _4249 = _3926;
                                _4251 = _3927;
                            }
                            else
                            {
                                _4247 = min(1.0f, _2895) * _3925;
                                _4249 = min(1.0f, _2897) * _3926;
                                _4251 = min(1.0f, _2899) * _3927;
                            }
                            float _4256 = _3187 + 0.14999997615814208984375f;
                            float _4949;
                            if (_1546)
                            {
                                _4949 = _3198 * _4256;
                            }
                            else
                            {
                                _4949 = max((_3198 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_4256, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                            }
                            float4 _4954 = _34[4u].SampleLevel(_67, float3(_3373, _3374, _3375), _4949);
                            float _5094;
                            float _5096;
                            float _5098;
                            float _5100;
                            if (_3817)
                            {
                                _5094 = 0.0f;
                                _5096 = 0.0f;
                                _5098 = 0.0f;
                                _5100 = 0.0f;
                            }
                            else
                            {
                                float _5113 = min(_1607, _57_m0[110u].z);
                                float _5114 = min(_1608, _57_m0[110u].w);
                                float4 _5130 = _15[509u].SampleLevel(_65, float2(_5113, _5114), _15[557u].SampleLevel(_65, float2(_5113, _5114), 0.0f).x * _57_m0[72u].w);
                                float _5132 = _5130.x;
                                float _5133 = _5130.y;
                                float _5134 = _5130.z;
                                float _5151 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_5132, max(_5133, _5134))));
                                _5094 = _5151 * _5132;
                                _5096 = _5151 * _5133;
                                _5098 = _5151 * _5134;
                                _5100 = _5130.w;
                            }
                            float _5102 = 1.0f - _5100;
                            float _5106 = (_5102 * _4954.x) + _5094;
                            float _5107 = (_5102 * _4954.y) + _5096;
                            float _5108 = (_5102 * _4954.z) + _5098;
                            float _5462;
                            float _5464;
                            float _5466;
                            if (_3928)
                            {
                                _5462 = _5106;
                                _5464 = _5107;
                                _5466 = _5108;
                            }
                            else
                            {
                                _5462 = min(1.0f, _2895) * _5106;
                                _5464 = min(1.0f, _2897) * _5107;
                                _5466 = min(1.0f, _2899) * _5108;
                            }
                            float _5471 = dot(float3(_3373, _3374, _3375), float3(_780, _774, _768));
                            float _5474 = dot(float3(_3373, _3374, _3375), float3(_782, _776, _770));
                            float _5477 = dot(float3(_3373, _3374, _3375), float3(_784, _778, _772));
                            float _5480 = abs(_5471);
                            float _5481 = abs(_5474);
                            float _5482 = abs(_5477);
                            float _5484 = (_5481 + _5480) + _5482;
                            float _5499 = 1.0f - _978;
                            float _5511 = dot(float3(_1554, _1555, _1556), float3(_3373, _3374, _3375));
                            float _5514 = dot(float3(_3180, _3181, _3182), float3(_3373, _3374, _3375));
                            float _5520 = dot(float3(_3180, _3181, _3182), float3(_1554, _1555, _1556));
                            float _5526 = _3373 - (_5514 * _3180);
                            float _5527 = _3374 - (_5514 * _3181);
                            float _5528 = _3375 - (_5514 * _3182);
                            float _5532 = _1554 - (_5520 * _3180);
                            float _5533 = _1555 - (_5520 * _3181);
                            float _5534 = _1556 - (_5520 * _3182);
                            float _5547 = rsqrt((dot(float3(_5532, _5533, _5534), float3(_5532, _5533, _5534)) * dot(float3(_5526, _5527, _5528), float3(_5526, _5527, _5528))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_5526, _5527, _5528), float3(_5532, _5533, _5534));
                            float _5549 = (_5547 * 0.5f) + 0.5f;
                            float _5551 = sqrt(clamp(_5549, 0.0f, 1.0f));
                            float _5557 = cos(abs(asin(_5520) - asin(_5514)) * 0.5f);
                            float _5563 = 1.0f / ((1.190000057220458984375f / _5557) + (_5557 * 0.36000001430511474609375f));
                            float _5564 = _5520 * 0.645161330699920654296875f;
                            float _5568 = sqrt(1.0f - (_5564 * _5564));
                            float _5575 = max(_3179 * 0.5f, 0.00999999977648258209228515625f) + 0.300000011920928955078125f;
                            float _5576 = max(_3179 * 2.0f, 0.00999999977648258209228515625f) + 0.300000011920928955078125f;
                            float _5579 = (-0.0f) - _986;
                            float _5583 = sin(_5579);
                            float _5594 = _5520 + _5514;
                            float _5595 = _5594 - ((_5583 * 2.0f) * (((cos(_5579) * _5551) * sqrt(1.0f - (_5520 * _5520))) + (_5583 * _5520)));
                            float _5598 = (_5551 * 1.41421353816986083984375f) * (max(_3184, 0.00999999977648258209228515625f) + 0.300000011920928955078125f);
                            float _5624 = ((_5551 * 0.25f) * (exp2((((_5595 * _5595) * (-0.5f)) / (_5598 * _5598)) * 1.44269502162933349609375f) / (_5598 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(_5511, 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f);
                            float _5625 = _5594 - (_986 * 0.5f);
                            float _5643 = clamp((-0.0f) - _5511, 0.0f, 1.0f);
                            float _5644 = _5643 * _5643;
                            float _5647 = (_5644 * _5644) * (_5643 * (1.0f - clamp(_976 * 200.0f, 0.0f, 1.0f)));
                            float _5654 = (cos(asin((_5563 * _5551) * ((_5563 * (0.60000002384185791015625f - (_5547 * 0.800000011920928955078125f))) + 1.0f)) * 2.0f) + 1.0f) * (-2.8853900432586669921875f);
                            float _5659 = exp2(_5654 * (_970 / _5568));
                            float _5660 = exp2(_5654 * (_972 / _5568));
                            float _5661 = exp2(_5654 * (_974 / _5568));
                            float _5676 = 0.95347940921783447265625f - (exp2(log2(1.0f - _5557) * 5.0f) * 0.95347940921783447265625f);
                            float _5678 = _5557 * 0.5f;
                            float4 _5687 = _15[(_968 + 513u) + 0u].SampleLevel(_65, float2(_5678 + 0.5f, _5549), 0.0f);
                            float _5689 = _5687.x;
                            float _5690 = _5594 - (_986 * 1.5f);
                            float _5716 = exp2(log2(1.0f - _5678) * 5.0f) * 0.95347940921783447265625f;
                            float _5718 = 0.95347940921783447265625f - _5716;
                            float _5720 = (_5718 * _5718) * (_5716 + 0.0465205647051334381103515625f);
                            float _5723 = clamp((1.0f - max(_984, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                            float _5731 = abs(sqrt(1.0f - (_5514 * _5514)));
                            float _5732 = _5731 * clamp(dot(float3(_3373, _3374, _3375), float3(_3373, _3374, _3375)), 0.0f, 1.0f);
                            float _5734 = ((1.0f - clamp(_976 * 66.6666717529296875f, 0.0f, 1.0f)) * 2.0f) * (1.0f - clamp(((dot(float3(_5480 / _5484, _5481 / _5484, _5482 / _5484), float3((_5471 < 0.0f) ? _792 : _786, (_5474 < 0.0f) ? _794 : _788, (_5477 < 0.0f) ? _796 : _790)) * _798) + _976) / (((_5499 * _5499) * 0.0900000035762786865234375f) + 0.00999999977648258209228515625f), 0.0f, 1.0f));
                            float _5735 = _5734 * _57_m0[59u].w;
                            float _5745 = _5734 * _57_m0[59u].w;
                            float _5746 = _5745 * _5462;
                            float _5747 = (_5676 * _5676) * (exp2((((_5625 * _5625) * (-0.5f)) / (_5575 * _5575)) * 1.44269502162933349609375f) / (_5575 * 2.5066282749176025390625f));
                            float _5752 = _5745 * _5464;
                            float _5757 = _5745 * _5466;
                            float _5762 = (exp2(_5723 * ((_5547 * 24.5258159637451171875f) + (-24.208423614501953125f))) * _5723) * (exp2((((_5690 * _5690) * (-0.5f)) / (_5576 * _5576)) * 1.44269502162933349609375f) / (_5576 * 2.5066282749176025390625f));
                            _6128 = ((_5735 * _4247) * _5624) * _5732;
                            _6129 = ((_5735 * _4249) * _5624) * _5732;
                            _6130 = ((_5735 * _4251) * _5624) * _5732;
                            _6131 = (((_5747 * _5746) * (((1.0f - _5659) * _5647) + _5659)) * _5689) * _5731;
                            _6132 = (((_5747 * _5752) * (((1.0f - _5660) * _5647) + _5660)) * _5689) * _5731;
                            _6133 = (((_5747 * _5757) * (((1.0f - _5661) * _5647) + _5661)) * _5689) * _5731;
                            _6134 = (((_5762 * _5746) * exp2(((_970 * (-3.2000000476837158203125f)) / _5557) * 1.44269502162933349609375f)) * _5720) * _5732;
                            _6135 = (((_5762 * _5752) * exp2(((_972 * (-3.2000000476837158203125f)) / _5557) * 1.44269502162933349609375f)) * _5720) * _5732;
                            _6136 = (((_5762 * _5757) * exp2(((_974 * (-3.2000000476837158203125f)) / _5557) * 1.44269502162933349609375f)) * _5720) * _5732;
                        }
                        else
                        {
                            float _3378 = (_1556 * _778) - (_1555 * _772);
                            float _3381 = (_1554 * _772) - (_1556 * _784);
                            float _3384 = (_1555 * _784) - (_1554 * _778);
                            float _3404 = (((((-0.0f) - _837) - (_3384 * _778)) + (_3381 * _772)) * 0.60000002384185791015625f) + _837;
                            float _3405 = (((((-0.0f) - _840) - (_3378 * _772)) + (_3384 * _784)) * 0.60000002384185791015625f) + _840;
                            float _3406 = (((((-0.0f) - _843) - (_3381 * _784)) + (_3378 * _778)) * 0.60000002384185791015625f) + _843;
                            float _3410 = rsqrt(dot(float3(_3404, _3405, _3406), float3(_3404, _3405, _3406)));
                            float _3411 = _3410 * _3404;
                            float _3412 = _3410 * _3405;
                            float _3413 = _3410 * _3406;
                            float _3417 = dot(float3(_1981, _1982, _1983), float3(_3411, _3412, _3413)) * 2.0f;
                            float _3421 = _1981 - (_3417 * _3411);
                            float _3422 = _1982 - (_3417 * _3412);
                            float _3423 = _1983 - (_3417 * _3413);
                            float _3818;
                            if (_1546)
                            {
                                _3818 = _3198 * _3199;
                            }
                            else
                            {
                                _3818 = max((_3198 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_3199, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                            }
                            float4 _3823 = _34[4u].SampleLevel(_67, float3(_3421, _3422, _3423), _3818);
                            bool _3828 = _1609 == 0u;
                            float _3972;
                            float _3974;
                            float _3976;
                            float _3978;
                            if (_3828)
                            {
                                _3972 = 0.0f;
                                _3974 = 0.0f;
                                _3976 = 0.0f;
                                _3978 = 0.0f;
                            }
                            else
                            {
                                float _3992 = min(_1607, _57_m0[110u].z);
                                float _3993 = min(_1608, _57_m0[110u].w);
                                float4 _4009 = _15[509u].SampleLevel(_65, float2(_3992, _3993), _15[557u].SampleLevel(_65, float2(_3992, _3993), 0.0f).x * _57_m0[72u].w);
                                float _4011 = _4009.x;
                                float _4012 = _4009.y;
                                float _4013 = _4009.z;
                                float _4030 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4011, max(_4012, _4013))));
                                _3972 = _4030 * _4011;
                                _3974 = _4030 * _4012;
                                _3976 = _4030 * _4013;
                                _3978 = _4009.w;
                            }
                            float _3980 = 1.0f - _3978;
                            float _3984 = (_3980 * _3823.x) + _3972;
                            float _3985 = (_3980 * _3823.y) + _3974;
                            float _3986 = (_3980 * _3823.z) + _3976;
                            bool _3987 = _2894 == 0u;
                            float _4261;
                            float _4263;
                            float _4265;
                            if (_3987)
                            {
                                _4261 = _3984;
                                _4263 = _3985;
                                _4265 = _3986;
                            }
                            else
                            {
                                _4261 = min(1.0f, _2895) * _3984;
                                _4263 = min(1.0f, _2897) * _3985;
                                _4265 = min(1.0f, _2899) * _3986;
                            }
                            float _4270 = _3187 + 0.14999997615814208984375f;
                            float _4959;
                            if (_1546)
                            {
                                _4959 = _3198 * _4270;
                            }
                            else
                            {
                                _4959 = max((_3198 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_4270, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                            }
                            float4 _4964 = _34[4u].SampleLevel(_67, float3(_3421, _3422, _3423), _4959);
                            float _5152;
                            float _5154;
                            float _5156;
                            float _5158;
                            if (_3828)
                            {
                                _5152 = 0.0f;
                                _5154 = 0.0f;
                                _5156 = 0.0f;
                                _5158 = 0.0f;
                            }
                            else
                            {
                                float _5171 = min(_1607, _57_m0[110u].z);
                                float _5172 = min(_1608, _57_m0[110u].w);
                                float4 _5188 = _15[509u].SampleLevel(_65, float2(_5171, _5172), _15[557u].SampleLevel(_65, float2(_5171, _5172), 0.0f).x * _57_m0[72u].w);
                                float _5190 = _5188.x;
                                float _5191 = _5188.y;
                                float _5192 = _5188.z;
                                float _5209 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_5190, max(_5191, _5192))));
                                _5152 = _5209 * _5190;
                                _5154 = _5209 * _5191;
                                _5156 = _5209 * _5192;
                                _5158 = _5188.w;
                            }
                            float _5160 = 1.0f - _5158;
                            float _5164 = (_5160 * _4964.x) + _5152;
                            float _5165 = (_5160 * _4964.y) + _5154;
                            float _5166 = (_5160 * _4964.z) + _5156;
                            float _5778;
                            float _5780;
                            float _5782;
                            if (_3987)
                            {
                                _5778 = _5164;
                                _5780 = _5165;
                                _5782 = _5166;
                            }
                            else
                            {
                                _5778 = min(1.0f, _2895) * _5164;
                                _5780 = min(1.0f, _2897) * _5165;
                                _5782 = min(1.0f, _2899) * _5166;
                            }
                            float _5794 = dot(float3(_3180, _3181, _3182), float3(_3421, _3422, _3423));
                            float _5800 = dot(float3(_3180, _3181, _3182), float3(_1554, _1555, _1556));
                            float _5806 = _3421 - (_5794 * _3180);
                            float _5807 = _3422 - (_5794 * _3181);
                            float _5808 = _3423 - (_5794 * _3182);
                            float _5812 = _1554 - (_5800 * _3180);
                            float _5813 = _1555 - (_5800 * _3181);
                            float _5814 = _1556 - (_5800 * _3182);
                            float _5827 = rsqrt((dot(float3(_5812, _5813, _5814), float3(_5812, _5813, _5814)) * dot(float3(_5806, _5807, _5808), float3(_5806, _5807, _5808))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_5806, _5807, _5808), float3(_5812, _5813, _5814));
                            float _5831 = sqrt(clamp((_5827 * 0.5f) + 0.5f, 0.0f, 1.0f));
                            float _5837 = cos(abs(asin(_5800) - asin(_5794)) * 0.5f);
                            float _5842 = max(_3179 * 2.0f, 0.00999999977648258209228515625f) + 0.300000011920928955078125f;
                            float _5844 = (-0.0f) - _986;
                            float _5846 = sin(_5844);
                            float _5857 = _5800 + _5794;
                            float _5858 = _5857 - ((_5846 * 2.0f) * (((cos(_5844) * _5831) * sqrt(1.0f - (_5800 * _5800))) + (_5846 * _5800)));
                            float _5860 = (_5831 * 1.41421353816986083984375f) * (max(_3184, 0.00999999977648258209228515625f) + 0.300000011920928955078125f);
                            float _5879 = _5857 - (_986 * 1.5f);
                            float _5905 = exp2(log2(1.0f - (_5837 * 0.5f)) * 5.0f) * 0.95347940921783447265625f;
                            float _5907 = 0.95347940921783447265625f - _5905;
                            float _5909 = (_5907 * _5907) * (_5905 + 0.0465205647051334381103515625f);
                            float _5912 = clamp((1.0f - max(_984, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                            float _5919 = abs(sqrt(1.0f - (_5794 * _5794))) * clamp(dot(float3(_837, _840, _843), float3(_3421, _3422, _3423)), 0.0f, 1.0f);
                            float _5924 = _5919 * ((((_57_m0[59u].w * 0.25f) * _5831) * (exp2((((_5858 * _5858) * (-0.5f)) / (_5860 * _5860)) * 1.44269502162933349609375f) / (_5860 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(dot(float3(_1554, _1555, _1556), float3(_3421, _3422, _3423)), 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f));
                            float _5929 = (exp2(_5912 * ((_5827 * 24.5258159637451171875f) + (-24.208423614501953125f))) * _5912) * ((exp2((((_5879 * _5879) * (-0.5f)) / (_5842 * _5842)) * 1.44269502162933349609375f) / (_5842 * 2.5066282749176025390625f)) * _57_m0[59u].w);
                            _6128 = _5924 * _4261;
                            _6129 = _5924 * _4263;
                            _6130 = _5924 * _4265;
                            _6131 = 0.0f;
                            _6132 = 0.0f;
                            _6133 = 0.0f;
                            _6134 = (((_5929 * _5778) * exp2(((_970 * (-3.2000000476837158203125f)) / _5837) * 1.44269502162933349609375f)) * _5909) * _5919;
                            _6135 = (((_5929 * _5780) * exp2(((_972 * (-3.2000000476837158203125f)) / _5837) * 1.44269502162933349609375f)) * _5909) * _5919;
                            _6136 = (((_5929 * _5782) * exp2(((_974 * (-3.2000000476837158203125f)) / _5837) * 1.44269502162933349609375f)) * _5909) * _5919;
                        }
                        frontier_phi_175_123_ladder_127_ladder = ((_6133 + _6130) + _6136) * 3.1415927410125732421875f;
                        frontier_phi_175_123_ladder_127_ladder_1 = ((_6131 + _6128) + _6134) * 3.1415927410125732421875f;
                        frontier_phi_175_123_ladder_127_ladder_2 = ((_6132 + _6129) + _6135) * 3.1415927410125732421875f;
                    }
                    frontier_phi_175_123_ladder = frontier_phi_175_123_ladder_127_ladder;
                    frontier_phi_175_123_ladder_1 = frontier_phi_175_123_ladder_127_ladder_1;
                    frontier_phi_175_123_ladder_2 = frontier_phi_175_123_ladder_127_ladder_2;
                }
                else
                {
                    float _3136 = _1984 * 2.0f;
                    float _3140 = _1981 - (_3136 * _837);
                    float _3142 = _1983 - (_3136 * _843);
                    float _3143 = (_1982 - (_3136 * _840)) * _2934;
                    float _3147 = rsqrt(dot(float3(_3140, _3143, _3142), float3(_3140, _3143, _3142)));
                    float _3148 = _3140 * _3147;
                    float _3149 = _3143 * _3147;
                    float _3150 = _3142 * _3147;
                    float _3424;
                    float _3425;
                    float _3426;
                    float _3427;
                    if (_57_m0[176u].w > 0.0f)
                    {
                        float _3201 = 1.0f - _876;
                        float _3204 = exp2(log2(_3201) * _57_m0[176u].w);
                        float _3206 = clamp(1.0f - _3204, 0.0f, 1.0f);
                        float _3210 = (sqrt(_3206 + 9.9999997473787516355514526367188e-05f) + _3204) * _3206;
                        float _3217 = (_3210 * (_3148 - _837)) + _837;
                        float _3218 = (_3210 * (_3149 - _840)) + _840;
                        float _3219 = (_3210 * (_3150 - _843)) + _843;
                        float _3223 = rsqrt(dot(float3(_3217, _3218, _3219), float3(_3217, _3218, _3219)));
                        _3424 = _3201;
                        _3425 = _3217 * _3223;
                        _3426 = _3218 * _3223;
                        _3427 = _3219 * _3223;
                    }
                    else
                    {
                        _3424 = 1.0f - _876;
                        _3425 = _3148;
                        _3426 = _3149;
                        _3427 = _3150;
                    }
                    float _3429 = (_1538 >= 0.0f) ? _1538 : 8.0f;
                    float _3829;
                    if (_1546)
                    {
                        _3829 = _3429 * _3424;
                    }
                    else
                    {
                        _3829 = max((_3429 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_3424, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                    }
                    float4 _3834 = _34[4u].SampleLevel(_67, float3(_3425, _3426, _3427), _3829);
                    bool _3839 = _1609 == 0u;
                    float _4031;
                    float _4033;
                    float _4035;
                    float _4037;
                    if (_3839)
                    {
                        _4031 = 0.0f;
                        _4033 = 0.0f;
                        _4035 = 0.0f;
                        _4037 = 0.0f;
                    }
                    else
                    {
                        float _4051 = min(_1607, _57_m0[110u].z);
                        float _4052 = min(_1608, _57_m0[110u].w);
                        float4 _4068 = _15[509u].SampleLevel(_65, float2(_4051, _4052), _15[557u].SampleLevel(_65, float2(_4051, _4052), 0.0f).x * _57_m0[72u].w);
                        float _4070 = _4068.x;
                        float _4071 = _4068.y;
                        float _4072 = _4068.z;
                        float _4089 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4070, max(_4071, _4072))));
                        _4031 = _4089 * _4070;
                        _4033 = _4089 * _4071;
                        _4035 = _4089 * _4072;
                        _4037 = _4068.w;
                    }
                    float _4039 = 1.0f - _4037;
                    float _4043 = (_4039 * _3834.x) + _4031;
                    float _4044 = (_4039 * _3834.y) + _4033;
                    float _4045 = (_4039 * _3834.z) + _4035;
                    bool _4046 = _2894 == 0u;
                    float _4274;
                    float _4276;
                    float _4278;
                    if (_4046)
                    {
                        _4274 = _4043;
                        _4276 = _4044;
                        _4278 = _4045;
                    }
                    else
                    {
                        _4274 = min(1.0f, _2895) * _4043;
                        _4276 = min(1.0f, _2897) * _4044;
                        _4278 = min(1.0f, _2899) * _4045;
                    }
                    float _4289 = clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f);
                    float _4294 = exp2(_4289 * (-9.27999973297119140625f));
                    float _4299 = (((min(_876 * 0.4749999940395355224609375f, _4294) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _876) + (-0.015625f);
                    float _4300 = ((_876 * 0.25f) + 0.75f) - _4299;
                    float _4310 = (_57_m0[59u].w * _4274) * clamp((_4300 * _865) + _4299, 0.0f, 1.0f);
                    float _4311 = (_57_m0[59u].w * _4276) * clamp((_4300 * _870) + _4299, 0.0f, 1.0f);
                    float _4312 = (_57_m0[59u].w * _4278) * clamp((_4300 * _873) + _4299, 0.0f, 1.0f);
                    float _4969;
                    float _4970;
                    float _4971;
                    float _4972;
                    if (_57_m0[176u].w > 0.0f)
                    {
                        float _4608 = 1.0f - _915;
                        float _4611 = exp2(log2(_4608) * _57_m0[176u].w);
                        float _4613 = clamp(1.0f - _4611, 0.0f, 1.0f);
                        float _4617 = (sqrt(_4613 + 9.9999997473787516355514526367188e-05f) + _4611) * _4613;
                        float _4624 = (_4617 * (_3148 - _837)) + _837;
                        float _4625 = (_4617 * (_3149 - _840)) + _840;
                        float _4626 = (_4617 * (_3150 - _843)) + _843;
                        float _4630 = rsqrt(dot(float3(_4624, _4625, _4626), float3(_4624, _4625, _4626)));
                        _4969 = _4608;
                        _4970 = _4624 * _4630;
                        _4971 = _4625 * _4630;
                        _4972 = _4626 * _4630;
                    }
                    else
                    {
                        _4969 = 1.0f - _915;
                        _4970 = _3148;
                        _4971 = _3149;
                        _4972 = _3150;
                    }
                    float _5945;
                    if (_1546)
                    {
                        _5945 = _3429 * _4969;
                    }
                    else
                    {
                        _5945 = max((_3429 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_4969, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                    }
                    float4 _5950 = _34[4u].SampleLevel(_67, float3(_4970, _4971, _4972), _5945);
                    float _6143;
                    float _6145;
                    float _6147;
                    float _6149;
                    if (_3839)
                    {
                        _6143 = 0.0f;
                        _6145 = 0.0f;
                        _6147 = 0.0f;
                        _6149 = 0.0f;
                    }
                    else
                    {
                        float _6162 = min(_1607, _57_m0[110u].z);
                        float _6163 = min(_1608, _57_m0[110u].w);
                        float4 _6179 = _15[509u].SampleLevel(_65, float2(_6162, _6163), _15[557u].SampleLevel(_65, float2(_6162, _6163), 0.0f).x * _57_m0[72u].w);
                        float _6181 = _6179.x;
                        float _6182 = _6179.y;
                        float _6183 = _6179.z;
                        float _6200 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_6181, max(_6182, _6183))));
                        _6143 = _6200 * _6181;
                        _6145 = _6200 * _6182;
                        _6147 = _6200 * _6183;
                        _6149 = _6179.w;
                    }
                    float _6151 = 1.0f - _6149;
                    float _6155 = (_6151 * _5950.x) + _6143;
                    float _6156 = (_6151 * _5950.y) + _6145;
                    float _6157 = (_6151 * _5950.z) + _6147;
                    float _6518;
                    float _6520;
                    float _6522;
                    if (_4046)
                    {
                        _6518 = _6155;
                        _6520 = _6156;
                        _6522 = _6157;
                    }
                    else
                    {
                        _6518 = min(1.0f, _2895) * _6155;
                        _6520 = min(1.0f, _2897) * _6156;
                        _6522 = min(1.0f, _2899) * _6157;
                    }
                    float _6537 = (((min(_915 * 0.4749999940395355224609375f, _4294) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _915) + (-0.015625f);
                    float _6538 = ((_915 * 0.25f) + 0.75f) - _6537;
                    float _6562 = (_4289 * (_919 + (-1.0f))) + 1.0f;
                    frontier_phi_175_123_ladder = (((((_57_m0[59u].w * _6522) * clamp((_6538 * _873) + _6537, 0.0f, 1.0f)) - _4312) * _917) + _4312) * _6562;
                    frontier_phi_175_123_ladder_1 = (((((_57_m0[59u].w * _6518) * clamp((_6538 * _865) + _6537, 0.0f, 1.0f)) - _4310) * _917) + _4310) * _6562;
                    frontier_phi_175_123_ladder_2 = (((((_57_m0[59u].w * _6520) * clamp((_6538 * _870) + _6537, 0.0f, 1.0f)) - _4311) * _917) + _4311) * _6562;
                }
                _3840 = frontier_phi_175_123_ladder_1;
                _3846 = frontier_phi_175_123_ladder_2;
                _3852 = frontier_phi_175_123_ladder;
            }
            else
            {
                float _3072 = 1.0f - _876;
                float _3073 = _1984 * 2.0f;
                float _3083 = clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f);
                float _3085 = (_1538 >= 0.0f) ? _1538 : 8.0f;
                float _3228;
                if (_1546)
                {
                    _3228 = _3085 * _3072;
                }
                else
                {
                    _3228 = max((_3085 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_3072, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                }
                float4 _3233 = _34[4u].SampleLevel(_67, float3(_1981 - (_3073 * _837), _1982 - (_3073 * _840), _1983 - (_3073 * _843)), _3228);
                float _3430;
                float _3432;
                float _3434;
                float _3436;
                if (_1609 == 0u)
                {
                    _3430 = 0.0f;
                    _3432 = 0.0f;
                    _3434 = 0.0f;
                    _3436 = 0.0f;
                }
                else
                {
                    float _3451 = min(_1607, _57_m0[110u].z);
                    float _3452 = min(_1608, _57_m0[110u].w);
                    float4 _3470 = _15[509u].SampleLevel(_65, float2(_3451, _3452), _15[557u].SampleLevel(_65, float2(_3451, _3452), 0.0f).x * _57_m0[72u].w);
                    float _3472 = _3470.x;
                    float _3473 = _3470.y;
                    float _3474 = _3470.z;
                    float _3494 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_3472, max(_3473, _3474))));
                    _3430 = _3494 * _3472;
                    _3432 = _3494 * _3473;
                    _3434 = _3494 * _3474;
                    _3436 = _3470.w;
                }
                float _3438 = 1.0f - _3436;
                float _3442 = (_3438 * _3233.x) + _3430;
                float _3443 = (_3438 * _3233.y) + _3432;
                float _3444 = (_3438 * _3233.z) + _3434;
                float _3615;
                float _3617;
                float _3619;
                if (_2894 == 0u)
                {
                    _3615 = _3442;
                    _3617 = _3443;
                    _3619 = _3444;
                }
                else
                {
                    _3615 = min(1.0f, _2895) * _3442;
                    _3617 = min(1.0f, _2897) * _3443;
                    _3619 = min(1.0f, _2899) * _3444;
                }
                float4 _3631 = _15[502u].SampleLevel(_65, float2(_3083, _3072), 0.0f);
                float _3633 = _3631.x;
                float _3651 = (((min(_876 * 0.4749999940395355224609375f, exp2(_3083 * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _876) + (-0.015625f);
                float _3653 = ((_876 * 0.25f) + 0.75f) - _3651;
                float _3660 = clamp((_3653 * _865) + _3651, 0.0f, 1.0f);
                float _3661 = clamp((_3653 * _870) + _3651, 0.0f, 1.0f);
                float _3662 = clamp((_3653 * _873) + _3651, 0.0f, 1.0f);
                _3840 = (_57_m0[59u].w * _3615) * ((((_3633 * _923) - _3660) * _929) + _3660);
                _3846 = (_57_m0[59u].w * _3617) * ((((_3633 * _925) - _3661) * _929) + _3661);
                _3852 = (_57_m0[59u].w * _3619) * ((((_3633 * _927) - _3662) * _929) + _3662);
            }
            precise float _3858 = _894 * _3852;
            precise float _3859 = _894 * _3846;
            precise float _3860 = _894 * _3840;
            _4090 = _3860 * _2199;
            _4092 = _3859 * _2199;
            _4094 = _3858 * _2199;
        }
        else
        {
            uint _3001;
            float _3002;
            float _3004;
            float _3006;
            if (_2684)
            {
                _3001 = 0u;
                _3002 = 0.0f;
                _3004 = 0.0f;
                _3006 = 0.0f;
            }
            else
            {
                float _3024 = _2212 / _57_m0[59u].w;
                float _3025 = _2213 / _57_m0[59u].w;
                float _3026 = _2214 / _57_m0[59u].w;
                float _3033 = dot(float3(_3024, _3025, _3026), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                float _3054 = dot(float3(_34[NonUniformResourceIndex((_419 + 52u) + 0u)].Sample(_69, float3(_822, _827, _832)).xyz), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                float _3060 = (_62_m0[1164u].x * _1629) * _57_m0[169u].w;
                _3001 = 1u;
                _3002 = (_3060 * (((((_3024 - _3033) * _57_m0[156u].w) + _3033) / _3054) + (-1.0f))) + 1.0f;
                _3004 = ((((((_3025 - _3033) * _57_m0[156u].w) + _3033) / _3054) + (-1.0f)) * _3060) + 1.0f;
                _3006 = ((((((_3026 - _3033) * _57_m0[156u].w) + _3033) / _3054) + (-1.0f)) * _3060) + 1.0f;
            }
            uint _3087;
            float _3088;
            float _3090;
            float _3092;
            float _3093;
            float _3095;
            float _3097;
            float _3098;
            float _3100;
            float _3102;
            float _3104;
            if (((_431 & 4u) == 0u) || (asuint(_62_m0[157u]).y == 0u))
            {
                _3087 = 0u;
                _3088 = 0.0f;
                _3090 = 0.0f;
                _3092 = 1.0f;
                _3093 = 1.0f;
                _3095 = 1.0f;
                _3097 = _2199;
                _3098 = 0.0f;
                _3100 = 0.0f;
                _3102 = 0.0f;
                _3104 = 0.0f;
            }
            else
            {
                float4 _3120 = _15[2u].SampleLevel(_65, float2(_1126, _1127), 0.0f);
                _3087 = 1u;
                _3088 = _62_m0[157u].z;
                _3090 = _62_m0[157u].w;
                _3092 = _2199;
                _3093 = _62_m0[158u].x;
                _3095 = _62_m0[158u].y;
                _3097 = 1.0f;
                _3098 = _57_m0[59u].w * _3120.x;
                _3100 = _57_m0[59u].w * _3120.y;
                _3102 = _57_m0[59u].w * _3120.z;
                _3104 = clamp(_3120.w, 0.0f, 1.0f);
            }
            uint _3108 = (_419 + 48u) + 0u;
            float _3109 = _1984 * 2.0f;
            float _3113 = _1981 - (_3109 * _837);
            float _3114 = _1982 - (_3109 * _840);
            float _3115 = _1983 - (_3109 * _843);
            float _4479;
            float _4481;
            float _4483;
            if (_995)
            {
                float _3167 = _2223 * _894;
                float _3168 = clamp(_876, 0.0f, 1.0f);
                float _3170 = (_1538 >= 0.0f) ? _1538 : 8.0f;
                float _3171 = 1.0f - _3168;
                float _3495;
                if (_1546)
                {
                    _3495 = _3171 * _3170;
                }
                else
                {
                    _3495 = max((_3170 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_3171, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                }
                float4 _3500 = _34[NonUniformResourceIndex(_3108)].SampleLevel(_67, float3(_3113, _3114, _3115), _3495);
                float _3678;
                float _3680;
                float _3682;
                float _3684;
                if (_1609 == 0u)
                {
                    _3678 = 0.0f;
                    _3680 = 0.0f;
                    _3682 = 0.0f;
                    _3684 = 0.0f;
                }
                else
                {
                    float _3698 = min(_1607, _57_m0[110u].z);
                    float _3699 = min(_1608, _57_m0[110u].w);
                    float4 _3715 = _15[509u].SampleLevel(_65, float2(_3698, _3699), _15[557u].SampleLevel(_65, float2(_3698, _3699), 0.0f).x * _57_m0[72u].w);
                    float _3717 = _3715.x;
                    float _3718 = _3715.y;
                    float _3719 = _3715.z;
                    float _3736 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_3717, max(_3718, _3719))));
                    _3678 = _3736 * _3717;
                    _3680 = _3736 * _3718;
                    _3682 = _3736 * _3719;
                    _3684 = _3715.w;
                }
                float _3686 = 1.0f - _3684;
                float _3690 = (_3686 * _3500.x) + _3678;
                float _3691 = (_3686 * _3500.y) + _3680;
                float _3692 = (_3686 * _3500.z) + _3682;
                float _3864;
                float _3866;
                float _3868;
                if (_3001 == 0u)
                {
                    _3864 = _3690;
                    _3866 = _3691;
                    _3868 = _3692;
                }
                else
                {
                    _3864 = min(1.0f, _3002) * _3690;
                    _3866 = min(1.0f, _3004) * _3691;
                    _3868 = min(1.0f, _3006) * _3692;
                }
                float _3873 = _57_m0[59u].w * _3864;
                float _3874 = _57_m0[59u].w * _3866;
                float _3875 = _57_m0[59u].w * _3868;
                float _4098;
                float _4100;
                float _4102;
                if (_3087 == 0u)
                {
                    _4098 = _3873;
                    _4100 = _3874;
                    _4102 = _3875;
                }
                else
                {
                    float _4135 = clamp((_3168 - _3088) / (_3090 - _3088), 0.0f, 1.0f);
                    float _4139 = (_4135 * _4135) * (3.0f - (_4135 * 2.0f));
                    float _4144 = 1.0f - (_4139 * _3104);
                    float _4156 = (((_4139 * (_3095 - _3093)) + _3093) * (_3092 + (-1.0f))) + 1.0f;
                    _4098 = ((_4144 * _3873) + (_4139 * _3098)) * _4156;
                    _4100 = ((_4144 * _3874) + (_4139 * _3100)) * _4156;
                    _4102 = ((_4144 * _3875) + (_4139 * _3102)) * _4156;
                }
                float _4122 = (max((asuint(_57_m0[85u]).z == 0u) ? (_3168 * 0.75f) : _3168, _865) - _865) * exp2(log2(1.0f - clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f);
                _4479 = (_4098 * _3167) * (_4122 + _865);
                _4481 = (_4100 * _3167) * (_4122 + _865);
                _4483 = (_4102 * _3167) * (_4122 + _865);
            }
            else
            {
                float _3506;
                float _3507;
                float _3508;
                float _3509;
                if (_57_m0[176u].w > 0.0f)
                {
                    float _3249 = 1.0f - _876;
                    float _3252 = exp2(log2(_3249) * _57_m0[176u].w);
                    float _3254 = clamp(1.0f - _3252, 0.0f, 1.0f);
                    float _3258 = (sqrt(_3254 + 9.9999997473787516355514526367188e-05f) + _3252) * _3254;
                    float _3265 = (_3258 * (_3113 - _837)) + _837;
                    float _3266 = (_3258 * (_3114 - _840)) + _840;
                    float _3267 = (_3258 * (_3115 - _843)) + _843;
                    float _3271 = rsqrt(dot(float3(_3265, _3266, _3267), float3(_3265, _3266, _3267)));
                    _3506 = _3249;
                    _3507 = _3265 * _3271;
                    _3508 = _3266 * _3271;
                    _3509 = _3267 * _3271;
                }
                else
                {
                    _3506 = 1.0f - _876;
                    _3507 = _3113;
                    _3508 = _3114;
                    _3509 = _3115;
                }
                float _3511 = (_1538 >= 0.0f) ? _1538 : 8.0f;
                float _3880;
                if (_1546)
                {
                    _3880 = _3511 * _3506;
                }
                else
                {
                    _3880 = max((_3511 + (-9.0f)) + ((_62_m0[107u].x * 9.0f) * exp2(log2(clamp(_3506, 0.0f, 1.0f)) * _62_m0[107u].z)), 0.0f);
                }
                float4 _3885 = _34[NonUniformResourceIndex(_3108)].SampleLevel(_67, float3(_3507, _3508, _3509), _3880);
                float _4157;
                float _4159;
                float _4161;
                float _4163;
                if (_1609 == 0u)
                {
                    _4157 = 0.0f;
                    _4159 = 0.0f;
                    _4161 = 0.0f;
                    _4163 = 0.0f;
                }
                else
                {
                    float _4177 = min(_1607, _57_m0[110u].z);
                    float _4178 = min(_1608, _57_m0[110u].w);
                    float4 _4194 = _15[509u].SampleLevel(_65, float2(_4177, _4178), _15[557u].SampleLevel(_65, float2(_4177, _4178), 0.0f).x * _57_m0[72u].w);
                    float _4196 = _4194.x;
                    float _4197 = _4194.y;
                    float _4198 = _4194.z;
                    float _4215 = (1.0f / max(_57_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4196, max(_4197, _4198))));
                    _4157 = _4215 * _4196;
                    _4159 = _4215 * _4197;
                    _4161 = _4215 * _4198;
                    _4163 = _4194.w;
                }
                float _4165 = 1.0f - _4163;
                float _4169 = (_4165 * _3885.x) + _4157;
                float _4170 = (_4165 * _3885.y) + _4159;
                float _4171 = (_4165 * _3885.z) + _4161;
                float _4491;
                float _4493;
                float _4495;
                if (_3001 == 0u)
                {
                    _4491 = _4169;
                    _4493 = _4170;
                    _4495 = _4171;
                }
                else
                {
                    _4491 = min(1.0f, _3002) * _4169;
                    _4493 = min(1.0f, _3004) * _4170;
                    _4495 = min(1.0f, _3006) * _4171;
                }
                float _4500 = _57_m0[59u].w * _4491;
                float _4501 = _57_m0[59u].w * _4493;
                float _4502 = _57_m0[59u].w * _4495;
                float _4727;
                float _4729;
                float _4731;
                if (_3087 == 0u)
                {
                    _4727 = _4500;
                    _4729 = _4501;
                    _4731 = _4502;
                }
                else
                {
                    float _4764 = clamp((_876 - _3088) / (_3090 - _3088), 0.0f, 1.0f);
                    float _4768 = (_4764 * _4764) * (3.0f - (_4764 * 2.0f));
                    float _4773 = 1.0f - (_4768 * _3104);
                    float _4785 = (((_4768 * (_3095 - _3093)) + _3093) * (_3092 + (-1.0f))) + 1.0f;
                    _4727 = ((_4773 * _4500) + (_4768 * _3098)) * _4785;
                    _4729 = ((_4773 * _4501) + (_4768 * _3100)) * _4785;
                    _4731 = ((_4773 * _4502) + (_4768 * _3102)) * _4785;
                }
                float _4746 = (((min(_876 * 0.4749999940395355224609375f, exp2(clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f) * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _876) + (-0.015625f);
                float _4747 = ((_876 * 0.25f) + 0.75f) - _4746;
                float _4757 = _2223 * _894;
                _4479 = (_4727 * _4757) * clamp((_4747 * _865) + _4746, 0.0f, 1.0f);
                _4481 = (_4729 * _4757) * clamp((_4747 * _870) + _4746, 0.0f, 1.0f);
                _4483 = (_4731 * _4757) * clamp((_4747 * _873) + _4746, 0.0f, 1.0f);
            }
            _4090 = (_4479 * _3097) + (_2561 * _2201);
            _4092 = (_4481 * _3097) + (_2563 * _2201);
            _4094 = (_4483 * _3097) + (_2565 * _2201);
        }
        float _4320;
        float _4322;
        float _4324;
        float _4326;
        float _4328;
        float _4330;
        if ((_431 & 2u) == 0u)
        {
            _4320 = 0.0f;
            _4322 = 0.0f;
            _4324 = 0.0f;
            _4326 = 0.0f;
            _4328 = 0.0f;
            _4330 = 0.0f;
        }
        else
        {
            float _4408 = _991 ? 1.0f : ((_1163 * (_62_m0[1162u].y - _62_m0[1162u].x)) + _62_m0[1162u].x);
            float _4409 = _991 ? 1.0f : _62_m0[_1134 + 1156u].w;
            bool _4412 = ((_432 & 2097152u) == 0u) && _992;
            float _4440 = clamp((_1133 - _57_m0[109u].y) / (_57_m0[109u].z - _57_m0[109u].y), 0.0f, 1.0f);
            float _4456 = ((((clamp(exp2(log2(_1958) * _57_m0[162u].z), 0.0f, 1.0f) + (-1.0f)) * _57_m0[98u].y) + 1.0f) * _2221) * (((((_4440 * _4440) * _57_m0[109u].w) * (3.0f - (_4440 * 2.0f))) * (clamp(exp2(log2(_1666) * _57_m0[109u].x), 0.0f, 1.0f) + (-1.0f))) + 1.0f);
            float _4461 = cos(_57_m0[125u].x);
            float _4462 = _1984 * 2.0f;
            float _4466 = _1981 - (_4462 * _837);
            float _4467 = _1982 - (_4462 * _840);
            float _4468 = _1983 - (_4462 * _843);
            float _4469 = dot(float3(_62_m0[0u].xyz), float3(_4466, _4467, _4468));
            float _4475 = _4466 - (_4469 * _62_m0[0u].x);
            float _4476 = _4467 - (_4469 * _62_m0[0u].y);
            float _4477 = _4468 - (_4469 * _62_m0[0u].z);
            float _4722;
            float _4723;
            float _4724;
            if (_4469 < _4461)
            {
                float _4708 = rsqrt(dot(float3(_4475, _4476, _4477), float3(_4475, _4476, _4477))) * sin(_57_m0[125u].x);
                float _4712 = (_4708 * _4475) + (_4461 * _62_m0[0u].x);
                float _4713 = (_4708 * _4476) + (_4461 * _62_m0[0u].y);
                float _4714 = (_4708 * _4477) + (_4461 * _62_m0[0u].z);
                float _4718 = rsqrt(dot(float3(_4712, _4713, _4714), float3(_4712, _4713, _4714)));
                _4722 = _4712 * _4718;
                _4723 = _4713 * _4718;
                _4724 = _4714 * _4718;
            }
            else
            {
                _4722 = _4466;
                _4723 = _4467;
                _4724 = _4468;
            }
            float _5425;
            float _5430;
            float _5435;
            float _5440;
            float _5446;
            float _5452;
            float _5458;
            if ((_989 & 4194304u) == 0u)
            {
                float frontier_phi_261_239_ladder;
                float frontier_phi_261_239_ladder_1;
                float frontier_phi_261_239_ladder_2;
                float frontier_phi_261_239_ladder_3;
                float frontier_phi_261_239_ladder_4;
                float frontier_phi_261_239_ladder_5;
                float frontier_phi_261_239_ladder_6;
                if ((_989 & 8388608u) == 0u)
                {
                    float frontier_phi_261_239_ladder_258_ladder;
                    float frontier_phi_261_239_ladder_258_ladder_1;
                    float frontier_phi_261_239_ladder_258_ladder_2;
                    float frontier_phi_261_239_ladder_258_ladder_3;
                    float frontier_phi_261_239_ladder_258_ladder_4;
                    float frontier_phi_261_239_ladder_258_ladder_5;
                    float frontier_phi_261_239_ladder_258_ladder_6;
                    if ((_989 & 67108864u) == 0u)
                    {
                        float frontier_phi_261_239_ladder_258_ladder_271_ladder;
                        float frontier_phi_261_239_ladder_258_ladder_271_ladder_1;
                        float frontier_phi_261_239_ladder_258_ladder_271_ladder_2;
                        float frontier_phi_261_239_ladder_258_ladder_271_ladder_3;
                        float frontier_phi_261_239_ladder_258_ladder_271_ladder_4;
                        float frontier_phi_261_239_ladder_258_ladder_271_ladder_5;
                        float frontier_phi_261_239_ladder_258_ladder_271_ladder_6;
                        if ((_989 & 50331648u) == 0u)
                        {
                            float frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder;
                            float frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_1;
                            float frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_2;
                            float frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_3;
                            float frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_4;
                            float frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_5;
                            float frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_6;
                            if ((_989 & 1032192u) == 0u)
                            {
                                float _5429;
                                float _5434;
                                float _5439;
                                float _5459;
                                if ((_935 > 0.0f) && ((_989 & 3u) != 0u))
                                {
                                    float _6932 = max(max(max(max(_938, 0.0f), _935), _940), 0.0f);
                                    float _6938 = _57_m0[8u].x - _819;
                                    float _6939 = _57_m0[8u].y - _820;
                                    float _6940 = _57_m0[8u].z - _821;
                                    float _6952 = clamp(2.0f - (clamp(sqrt(((_6938 * _6938) + (_6939 * _6939)) + (_6940 * _6940)) * 0.02500000037252902984619140625f, 0.0f, 1.0f) * 2.0f), 0.0f, 1.0f);
                                    float _6953 = _6952 * _6932;
                                    float _6959 = _57_m0[21u].x - _819;
                                    float _6960 = _57_m0[21u].y - _820;
                                    float _6961 = _57_m0[21u].z - _821;
                                    float _6968 = log2(sqrt(((_6959 * _6959) + (_6960 * _6960)) + (_6961 * _6961)));
                                    float _6969 = _6968 * 0.85000002384185791015625f;
                                    float _7598;
                                    float _7599;
                                    float _7600;
                                    float _7601;
                                    if (_6953 > 0.00999999977648258209228515625f)
                                    {
                                        uint _7422 = _937 + 0u;
                                        float _7423 = ceil(_6969);
                                        float _7427 = _819 * 5.0f;
                                        float _7428 = _821 * 5.0f;
                                        float _7430 = exp2((-0.0f) - max(1.0f, _7423));
                                        float _7436 = exp2((-0.0f) - max(1.0f, _7423 + 1.0f));
                                        float4 _7445 = _37[NonUniformResourceIndex(_7422)].SampleLevel(_70, float2(frac(_7430 * _7427), frac(_7430 * _7428)), 0.0f);
                                        float4 _7450 = _37[NonUniformResourceIndex(_7422)].SampleLevel(_70, float2(frac(_7436 * _7427), frac(_7436 * _7428)), 0.0f);
                                        float _7455 = frac(_6969);
                                        float _7457 = (_7455 + 0.5f) * 0.5f;
                                        float _7469 = (_7445.x + (-0.5f)) * 2.0f;
                                        float _7470 = (_7445.y + (-0.5f)) * 2.0f;
                                        float _7476 = sqrt(clamp((1.0f - (_7469 * _7469)) - (_7470 * _7470), 0.0f, 1.0f));
                                        float _7483 = (_7450.x + (-0.5f)) * 2.0f;
                                        float _7484 = (_7450.y + (-0.5f)) * 2.0f;
                                        float _7490 = sqrt(clamp((1.0f - (_7483 * _7483)) - (_7484 * _7484), 0.0f, 1.0f));
                                        float _7496 = rsqrt(dot(float3(_7469, _7476, _7470), float3(_7469, _7476, _7470))) * (1.0f - _7455);
                                        float _7499 = rsqrt(dot(float3(_7483, _7490, _7484), float3(_7483, _7490, _7484))) * _7455;
                                        float _7502 = (_7499 * _7483) + (_7496 * _7469);
                                        float _7503 = (_7499 * _7484) + (_7496 * _7470);
                                        float _7507 = rsqrt(dot(float3(_7502, _7503, 1.0f), float3(_7502, _7503, 1.0f)));
                                        float _7510 = mad(_7507, _822, _7502 * _7507);
                                        float _7511 = mad(_7507, _827, 0.0f);
                                        float _7512 = mad(_7507, _832, _7503 * _7507);
                                        float _7516 = rsqrt(dot(float3(_7510, _7511, _7512), float3(_7510, _7511, _7512)));
                                        float _7517 = _7516 * _7510;
                                        float _7518 = _7516 * _7511;
                                        float _7519 = _7516 * _7512;
                                        float _7520 = _1555 + 1.0f;
                                        float _7524 = rsqrt(dot(float3(_1554, _7520, _1556), float3(_1554, _7520, _1556)));
                                        float _7576 = (((((_6953 * 2.2000000476837158203125f) * max(clamp(_7457 * (_7445.z - _7455), 0.0f, 1.0f), clamp((1.0f - _7457) * ((_7455 + (-1.0f)) + _7450.z), 0.0f, 1.0f))) * exp2((800.0f - (clamp(clamp((_6968 * 0.425000011920928955078125f) + (-1.0f), 0.0f, 1.0f), 0.0f, 1.0f) * 800.0f)) * log2(((_6932 * (0.004999999888241291046142578125f - (_940 * 0.00299999979324638843536376953125f))) + 0.00200000009499490261077880859375f) + max(dot(float3(_7524 * _1554, _7524 * _7520, _7524 * _1556), float3(_7517, _7518, _7519)), 0.0f)))) * exp2(log2(clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 1.2000000476837158203125f)) * clamp((dot(float3(_4722, _4723, _4724), float3(_822, _827, _832)) * 10.0f) + 0.5f, 0.0f, 1.0f)) * _57_m0[166u].y;
                                        float _7579 = (_940 * 0.4000000059604644775390625f) * _6952;
                                        float _7582 = ((_7519 + (-1.0f)) * _7579) + 1.0f;
                                        float _7585 = _7579 * _832;
                                        float _7588 = (_7582 * _822) + (_7517 * _7585);
                                        float _7589 = (_7582 * _827) + (_7518 * _7585);
                                        float _7590 = _7582 * _832;
                                        float _7594 = rsqrt(dot(float3(_7588, _7589, _7590), float3(_7588, _7589, _7590)));
                                        _7598 = _7576;
                                        _7599 = _7594 * _7588;
                                        _7600 = _7594 * _7589;
                                        _7601 = _7594 * _7590;
                                    }
                                    else
                                    {
                                        _7598 = 0.0f;
                                        _7599 = _822;
                                        _7600 = _827;
                                        _7601 = _832;
                                    }
                                    float _7602 = dot(float3(_7599, _7600, _7601), float3(_62_m0[0u].xyz));
                                    float _7608 = (1.0f - clamp(_7602 * 5.0f, 0.0f, 1.0f)) * _938;
                                    float _7615 = (_7608 * (_850 - _942)) + _942;
                                    float _7616 = (_7608 * (_855 - _944)) + _944;
                                    float _7617 = (_7608 * (_860 - _946)) + _946;
                                    float _7618 = 1.0f - _865;
                                    float _7619 = 1.0f - _870;
                                    float _7620 = 1.0f - _873;
                                    float _7628 = exp2(log2(1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_7599, _7600, _7601)), 0.0f, 1.0f)) * 5.0f);
                                    float _7635 = _940 * 0.550000011920928955078125f;
                                    float _7637 = ((_7628 * _7618) + _865) * _7635;
                                    float _7638 = ((_7628 * _7619) + _870) * _7635;
                                    float _7639 = ((_7628 * _7620) + _873) * _7635;
                                    float _7663 = clamp(_7602, 0.0f, 1.0f);
                                    float _7922;
                                    if (_57_m0[76u].w > 0.5f)
                                    {
                                        float _7885 = _1554 + _62_m0[0u].x;
                                        float _7886 = _1555 + _62_m0[0u].y;
                                        float _7887 = _1556 + _62_m0[0u].z;
                                        float _7891 = rsqrt(dot(float3(_7885, _7886, _7887), float3(_7885, _7886, _7887)));
                                        float _7898 = clamp(dot(float3(_62_m0[0u].xyz), float3(_7891 * _7885, _7891 * _7886, _7891 * _7887)), 0.0f, 1.0f);
                                        float _7908 = ((((_7898 * _7898) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                        _7922 = ((exp2(log2(1.0f - clamp(dot(float3(_7599, _7600, _7601), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _7908) + 1.0f) * ((_7908 * exp2(log2(1.0f - _7663) * 5.0f)) + 1.0f);
                                    }
                                    else
                                    {
                                        _7922 = 1.0f;
                                    }
                                    float _7924 = _7922 * (clamp((_7663 * 0.722500026226043701171875f) + 0.12750001251697540283203125f, 0.0f, 1.0f) * 0.3183098733425140380859375f);
                                    _5429 = (_7924 * _7618) * (((_7637 * _7637) * ((0.949999988079071044921875f - (_938 * 0.5f)) - _7615)) + _7615);
                                    _5434 = (_7924 * _7619) * (((_7638 * _7638) * ((0.810000002384185791015625f - (_938 * 0.069999992847442626953125f)) - _7616)) + _7616);
                                    _5439 = (_7924 * _7620) * (((_7639 * _7639) * (((_938 * 0.37999999523162841796875f) + 0.569999992847442626953125f) - _7617)) + _7617);
                                    _5459 = _7598;
                                }
                                else
                                {
                                    float _6977 = clamp(dot(float3(_822, _827, _832), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                                    float _6978 = clamp(_6977, 0.0f, 1.0f);
                                    float _7710;
                                    if (_57_m0[76u].w > 0.5f)
                                    {
                                        float _7673 = _1554 + _62_m0[0u].x;
                                        float _7674 = _1555 + _62_m0[0u].y;
                                        float _7675 = _1556 + _62_m0[0u].z;
                                        float _7679 = rsqrt(dot(float3(_7673, _7674, _7675), float3(_7673, _7674, _7675)));
                                        float _7686 = clamp(dot(float3(_62_m0[0u].xyz), float3(_7679 * _7673, _7679 * _7674, _7679 * _7675)), 0.0f, 1.0f);
                                        float _7696 = ((((_7686 * _7686) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                        _7710 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _7696) + 1.0f) * ((_7696 * exp2(log2(1.0f - _6977) * 5.0f)) + 1.0f);
                                    }
                                    else
                                    {
                                        _7710 = 1.0f;
                                    }
                                    _5429 = (((_850 * 0.3183098733425140380859375f) * (1.0f - _865)) * _6978) * _7710;
                                    _5434 = (((_855 * 0.3183098733425140380859375f) * (1.0f - _870)) * _6978) * _7710;
                                    _5439 = (((_860 * 0.3183098733425140380859375f) * (1.0f - _873)) * _6978) * _7710;
                                    _5459 = 0.0f;
                                }
                                float _7934 = clamp(dot(float3(_837, _840, _843), float3(_4722, _4723, _4724)), 0.0f, 1.0f);
                                float _8128;
                                float _8130;
                                float _8132;
                                if (_7934 > 0.0f)
                                {
                                    float _8065 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                    float _8066 = _4722 + _1554;
                                    float _8067 = _4723 + _1555;
                                    float _8068 = _4724 + _1556;
                                    float _8072 = rsqrt(dot(float3(_8066, _8067, _8068), float3(_8066, _8067, _8068)));
                                    float _8073 = _8072 * _8066;
                                    float _8074 = _8072 * _8067;
                                    float _8075 = _8072 * _8068;
                                    float _8084 = clamp(dot(float3(_837, _840, _843), float3(_8073, _8074, _8075)), 0.0f, 1.0f);
                                    float _8089 = _8065 * _8065;
                                    float _8090 = _8089 * _8089;
                                    float _8094 = (((_8084 * _8090) - _8084) * _8084) + 1.0f;
                                    float _8098 = _8089 * 0.5f;
                                    float _8099 = 1.0f - _8098;
                                    float _8106 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_8073, _8074, _8075)), 0.0f, 1.0f);
                                    float _8107 = _8106 * _8106;
                                    float _8109 = (_8107 * _8107) * _8106;
                                    float _8121 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _8099) + _8098) * ((_8099 * _7934) + _8098))) * (_8090 / ((_8094 * _8094) * 3.1415927410125732421875f)), _1534) * _4408;
                                    float _8125 = min(_8121 * ((_8109 * (1.0f - _865)) + _865), 100000.0f);
                                    float _8126 = min(_8121 * ((_8109 * (1.0f - _870)) + _870), 100000.0f);
                                    float _8127 = min(_8121 * ((_8109 * (1.0f - _873)) + _873), 100000.0f);
                                    float _8269;
                                    float _8271;
                                    float _8273;
                                    if (_4412)
                                    {
                                        _8269 = _8125;
                                        _8271 = _8126;
                                        _8273 = _8127;
                                    }
                                    else
                                    {
                                        float _8275 = 1.0f - _7934;
                                        float _8276 = _8275 * _8275;
                                        float _8278 = 1.0f - (_8276 * _8276);
                                        _8269 = _8125 * _8278;
                                        _8271 = _8126 * _8278;
                                        _8273 = _8127 * _8278;
                                    }
                                    _8128 = _8269 * _7934;
                                    _8130 = _8271 * _7934;
                                    _8132 = _8273 * _7934;
                                }
                                else
                                {
                                    _8128 = 0.0f;
                                    _8130 = 0.0f;
                                    _8132 = 0.0f;
                                }
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder = _8128 * _892;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_1 = _5434;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_2 = _5429;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_3 = _5439;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_4 = _8130 * _892;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_5 = _8132 * _892;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_6 = _5459;
                            }
                            else
                            {
                                float _6570 = 1.0f - _865;
                                float _6571 = 1.0f - _870;
                                float _6572 = 1.0f - _873;
                                float _6576 = clamp(dot(float3(_822, _827, _832), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                                float _6577 = clamp(_6576, 0.0f, 1.0f);
                                float _7020;
                                if (_57_m0[76u].w > 0.5f)
                                {
                                    float _6983 = _1554 + _62_m0[0u].x;
                                    float _6984 = _1555 + _62_m0[0u].y;
                                    float _6985 = _1556 + _62_m0[0u].z;
                                    float _6989 = rsqrt(dot(float3(_6983, _6984, _6985), float3(_6983, _6984, _6985)));
                                    float _6996 = clamp(dot(float3(_62_m0[0u].xyz), float3(_6989 * _6983, _6989 * _6984, _6989 * _6985)), 0.0f, 1.0f);
                                    float _7006 = ((((_6996 * _6996) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                    _7020 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _7006) + 1.0f) * ((_7006 * exp2(log2(1.0f - _6576) * 5.0f)) + 1.0f);
                                }
                                else
                                {
                                    _7020 = 1.0f;
                                }
                                float _7748;
                                float _7750;
                                float _7752;
                                if (_6576 > 0.0f)
                                {
                                    float _7728 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                    float _7729 = _1554 + _62_m0[0u].x;
                                    float _7730 = _1555 + _62_m0[0u].y;
                                    float _7731 = _1556 + _62_m0[0u].z;
                                    float _7735 = rsqrt(dot(float3(_7729, _7730, _7731), float3(_7729, _7730, _7731)));
                                    float _7736 = _7735 * _7729;
                                    float _7737 = _7735 * _7730;
                                    float _7738 = _7735 * _7731;
                                    float _7742 = clamp(dot(float3(_837, _840, _843), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                                    float _7746 = clamp(dot(float3(_837, _840, _843), float3(_7736, _7737, _7738)), 0.0f, 1.0f);
                                    float _7978;
                                    float _7979;
                                    float _7980;
                                    if (_7742 > 0.0f)
                                    {
                                        float _7945 = _7728 * _7728;
                                        float _7946 = _7945 * _7945;
                                        float _7950 = (((_7746 * _7946) - _7746) * _7746) + 1.0f;
                                        float _7954 = _7945 * 0.5f;
                                        float _7955 = 1.0f - _7954;
                                        float _7962 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_7736, _7737, _7738)), 0.0f, 1.0f);
                                        float _7963 = _7962 * _7962;
                                        float _7965 = (_7963 * _7963) * _7962;
                                        float _7974 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _7955) + _7954) * ((_7742 * _7955) + _7954))) * (_7946 / ((_7950 * _7950) * 3.1415927410125732421875f)), _1534) * _4408;
                                        _7978 = _7974 * ((_7965 * _6570) + _865);
                                        _7979 = _7974 * ((_7965 * _6571) + _870);
                                        _7980 = _7974 * ((_7965 * _6572) + _873);
                                    }
                                    else
                                    {
                                        _7978 = 0.0f;
                                        _7979 = 0.0f;
                                        _7980 = 0.0f;
                                    }
                                    float _7981 = min(_7978, 100000.0f);
                                    float _7982 = min(_7979, 100000.0f);
                                    float _7983 = min(_7980, 100000.0f);
                                    float _8134;
                                    float _8136;
                                    float _8138;
                                    if (_4412)
                                    {
                                        _8134 = _7981;
                                        _8136 = _7982;
                                        _8138 = _7983;
                                    }
                                    else
                                    {
                                        float _8140 = 1.0f - _6576;
                                        float _8141 = _8140 * _8140;
                                        float _8143 = 1.0f - (_8141 * _8141);
                                        _8134 = _7981 * _8143;
                                        _8136 = _7982 * _8143;
                                        _8138 = _7983 * _8143;
                                    }
                                    _7748 = _8134 * _6576;
                                    _7750 = _8136 * _6576;
                                    _7752 = _8138 * _6576;
                                }
                                else
                                {
                                    _7748 = 0.0f;
                                    _7750 = 0.0f;
                                    _7752 = 0.0f;
                                }
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder = _7748 * _892;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_1 = (((_855 * 0.3183098733425140380859375f) * _6571) * _6577) * _7020;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_2 = (((_850 * 0.3183098733425140380859375f) * _6570) * _6577) * _7020;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_3 = (((_860 * 0.3183098733425140380859375f) * _6572) * _6577) * _7020;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_4 = _7750 * _892;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_5 = _7752 * _892;
                                frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_6 = 0.0f;
                            }
                            frontier_phi_261_239_ladder_258_ladder_271_ladder = frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_1 = frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_1;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_2 = frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_2;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_3 = frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_3;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_4 = frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_4;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_5 = frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_5;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_6 = frontier_phi_261_239_ladder_258_ladder_271_ladder_280_ladder_6;
                        }
                        else
                        {
                            float _6203 = 1.0f - _876;
                            float _6204 = (-0.0f) - _784;
                            float _6205 = (-0.0f) - _778;
                            float _6206 = (-0.0f) - _772;
                            float _6214 = dot(float3(_6204, _6205, _6206), float3(_62_m0[0u].xyz));
                            float _6220 = dot(float3(_6204, _6205, _6206), float3(_1554, _1555, _1556));
                            float _6226 = _62_m0[0u].x - (_6214 * _6204);
                            float _6227 = _62_m0[0u].y - (_6214 * _6205);
                            float _6228 = _62_m0[0u].z - (_6214 * _6206);
                            float _6232 = _1554 - (_6220 * _6204);
                            float _6233 = _1555 - (_6220 * _6205);
                            float _6234 = _1556 - (_6220 * _6206);
                            float _6247 = rsqrt((dot(float3(_6232, _6233, _6234), float3(_6232, _6233, _6234)) * dot(float3(_6226, _6227, _6228), float3(_6226, _6227, _6228))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_6226, _6227, _6228), float3(_6232, _6233, _6234));
                            float _6251 = sqrt(clamp((_6247 * 0.5f) + 0.5f, 0.0f, 1.0f));
                            float _6257 = cos(abs(asin(_6220) - asin(_6214)) * 0.5f);
                            float _6262 = max(_6203 * 2.0f, 0.00999999977648258209228515625f);
                            float _6264 = (-0.0f) - _986;
                            float _6266 = sin(_6264);
                            float _6277 = _6220 + _6214;
                            float _6278 = _6277 - ((_6266 * 2.0f) * (((cos(_6264) * _6251) * sqrt(1.0f - (_6220 * _6220))) + (_6266 * _6220)));
                            float _6280 = (_6251 * 1.41421353816986083984375f) * max((1.0f - _980) * _6203, 0.00999999977648258209228515625f);
                            float _6302 = _6277 - (_986 * 1.5f);
                            float _6328 = exp2(log2(1.0f - (_6257 * 0.5f)) * 5.0f) * 0.95347940921783447265625f;
                            float _6330 = 0.95347940921783447265625f - _6328;
                            float _6335 = clamp((1.0f - max(_984, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                            float _6341 = abs(sqrt(1.0f - (_6214 * _6214))) * clamp(dot(float3(_822, _827, _832), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                            float _6342 = (((_6251 * 0.25f) * (exp2((((_6278 * _6278) * (-0.5f)) / (_6280 * _6280)) * 1.44269502162933349609375f) / (_6280 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(dot(float3(_1554, _1555, _1556), float3(_62_m0[0u].xyz)), 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f)) * _6341;
                            float _6344 = (_6335 * (exp2((((_6302 * _6302) * (-0.5f)) / (_6262 * _6262)) * 1.44269502162933349609375f) / (_6262 * 2.5066282749176025390625f))) * exp2(_6335 * ((_6247 * 24.5258159637451171875f) + (-24.208423614501953125f)));
                            float _6345 = _6341 * ((_6330 * _6330) * (_6328 + 0.0465205647051334381103515625f));
                            frontier_phi_261_239_ladder_258_ladder_271_ladder = ((_6345 * exp2(((_970 * (-3.2000000476837158203125f)) / _6257) * 1.44269502162933349609375f)) * _6344) + _6342;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_1 = 0.0f;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_2 = 0.0f;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_3 = 0.0f;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_4 = ((_6345 * exp2(((_972 * (-3.2000000476837158203125f)) / _6257) * 1.44269502162933349609375f)) * _6344) + _6342;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_5 = ((_6345 * exp2(((_974 * (-3.2000000476837158203125f)) / _6257) * 1.44269502162933349609375f)) * _6344) + _6342;
                            frontier_phi_261_239_ladder_258_ladder_271_ladder_6 = 0.0f;
                        }
                        frontier_phi_261_239_ladder_258_ladder = frontier_phi_261_239_ladder_258_ladder_271_ladder;
                        frontier_phi_261_239_ladder_258_ladder_1 = frontier_phi_261_239_ladder_258_ladder_271_ladder_1;
                        frontier_phi_261_239_ladder_258_ladder_2 = frontier_phi_261_239_ladder_258_ladder_271_ladder_2;
                        frontier_phi_261_239_ladder_258_ladder_3 = frontier_phi_261_239_ladder_258_ladder_271_ladder_3;
                        frontier_phi_261_239_ladder_258_ladder_4 = frontier_phi_261_239_ladder_258_ladder_271_ladder_4;
                        frontier_phi_261_239_ladder_258_ladder_5 = frontier_phi_261_239_ladder_258_ladder_271_ladder_5;
                        frontier_phi_261_239_ladder_258_ladder_6 = frontier_phi_261_239_ladder_258_ladder_271_ladder_6;
                    }
                    else
                    {
                        float _6000 = clamp(dot(float3(_822, _827, _832), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                        float _5442;
                        float _5448;
                        float _5454;
                        if (_6000 > 0.0f)
                        {
                            float _6357 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                            float _6358 = _1554 + _62_m0[0u].x;
                            float _6359 = _1555 + _62_m0[0u].y;
                            float _6360 = _1556 + _62_m0[0u].z;
                            float _6364 = rsqrt(dot(float3(_6358, _6359, _6360), float3(_6358, _6359, _6360)));
                            float _6365 = _6364 * _6358;
                            float _6366 = _6364 * _6359;
                            float _6367 = _6364 * _6360;
                            float _6371 = clamp(dot(float3(_837, _840, _843), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                            float _6375 = clamp(dot(float3(_837, _840, _843), float3(_6365, _6366, _6367)), 0.0f, 1.0f);
                            float _6627;
                            float _6628;
                            float _6629;
                            if (_6371 > 0.0f)
                            {
                                float _6591 = _6357 * _6357;
                                float _6592 = _6591 * _6591;
                                float _6596 = (((_6375 * _6592) - _6375) * _6375) + 1.0f;
                                float _6600 = _6591 * 0.5f;
                                float _6601 = 1.0f - _6600;
                                float _6608 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_6365, _6366, _6367)), 0.0f, 1.0f);
                                float _6609 = _6608 * _6608;
                                float _6611 = (_6609 * _6609) * _6608;
                                float _6623 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _6601) + _6600) * ((_6371 * _6601) + _6600))) * (_6592 / ((_6596 * _6596) * 3.1415927410125732421875f)), _1534) * _4408;
                                _6627 = _6623 * ((_6611 * (1.0f - _865)) + _865);
                                _6628 = _6623 * ((_6611 * (1.0f - _870)) + _870);
                                _6629 = _6623 * ((_6611 * (1.0f - _873)) + _873);
                            }
                            else
                            {
                                _6627 = 0.0f;
                                _6628 = 0.0f;
                                _6629 = 0.0f;
                            }
                            float _6630 = min(_6627, 100000.0f);
                            float _6632 = min(_6628, 100000.0f);
                            float _6633 = min(_6629, 100000.0f);
                            float _7031;
                            float _7033;
                            float _7035;
                            if (_4412)
                            {
                                _7031 = _6630;
                                _7033 = _6632;
                                _7035 = _6633;
                            }
                            else
                            {
                                float _7037 = 1.0f - _6000;
                                float _7038 = _7037 * _7037;
                                float _7040 = 1.0f - (_7038 * _7038);
                                _7031 = _6630 * _7040;
                                _7033 = _6632 * _7040;
                                _7035 = _6633 * _7040;
                            }
                            _5442 = _7031 * _6000;
                            _5448 = _7033 * _6000;
                            _5454 = _7035 * _6000;
                        }
                        else
                        {
                            _5442 = 0.0f;
                            _5448 = 0.0f;
                            _5454 = 0.0f;
                        }
                        float _6383 = clamp(_6000, 0.0f, 1.0f);
                        float _6671;
                        if (_57_m0[76u].w > 0.5f)
                        {
                            float _6634 = _1554 + _62_m0[0u].x;
                            float _6635 = _1555 + _62_m0[0u].y;
                            float _6636 = _1556 + _62_m0[0u].z;
                            float _6640 = rsqrt(dot(float3(_6634, _6635, _6636), float3(_6634, _6635, _6636)));
                            float _6647 = clamp(dot(float3(_62_m0[0u].xyz), float3(_6640 * _6634, _6640 * _6635, _6640 * _6636)), 0.0f, 1.0f);
                            float _6657 = ((((_6647 * _6647) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                            _6671 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _6657) + 1.0f) * ((_6657 * exp2(log2(1.0f - _6000) * 5.0f)) + 1.0f);
                        }
                        else
                        {
                            _6671 = 1.0f;
                        }
                        float _6672 = 1.0f - _960;
                        frontier_phi_261_239_ladder_258_ladder = _5442;
                        frontier_phi_261_239_ladder_258_ladder_1 = ((((_855 * 0.3183098733425140380859375f) * (1.0f - _870)) * _6672) * _6383) * _6671;
                        frontier_phi_261_239_ladder_258_ladder_2 = ((((_850 * 0.3183098733425140380859375f) * (1.0f - _865)) * _6672) * _6383) * _6671;
                        frontier_phi_261_239_ladder_258_ladder_3 = ((((_860 * 0.3183098733425140380859375f) * (1.0f - _873)) * _6672) * _6383) * _6671;
                        frontier_phi_261_239_ladder_258_ladder_4 = _5448;
                        frontier_phi_261_239_ladder_258_ladder_5 = _5454;
                        frontier_phi_261_239_ladder_258_ladder_6 = 0.0f;
                    }
                    frontier_phi_261_239_ladder = frontier_phi_261_239_ladder_258_ladder;
                    frontier_phi_261_239_ladder_1 = frontier_phi_261_239_ladder_258_ladder_1;
                    frontier_phi_261_239_ladder_2 = frontier_phi_261_239_ladder_258_ladder_2;
                    frontier_phi_261_239_ladder_3 = frontier_phi_261_239_ladder_258_ladder_3;
                    frontier_phi_261_239_ladder_4 = frontier_phi_261_239_ladder_258_ladder_4;
                    frontier_phi_261_239_ladder_5 = frontier_phi_261_239_ladder_258_ladder_5;
                    frontier_phi_261_239_ladder_6 = frontier_phi_261_239_ladder_258_ladder_6;
                }
                else
                {
                    float _5299 = 1.0f - _865;
                    float _5300 = 1.0f - _870;
                    float _5301 = 1.0f - _873;
                    float _5305 = clamp(dot(float3(_822, _827, _832), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                    float _5306 = clamp(_5305, 0.0f, 1.0f);
                    float _6039;
                    if (_57_m0[76u].w > 0.5f)
                    {
                        float _6002 = _1554 + _62_m0[0u].x;
                        float _6003 = _1555 + _62_m0[0u].y;
                        float _6004 = _1556 + _62_m0[0u].z;
                        float _6008 = rsqrt(dot(float3(_6002, _6003, _6004), float3(_6002, _6003, _6004)));
                        float _6015 = clamp(dot(float3(_62_m0[0u].xyz), float3(_6008 * _6002, _6008 * _6003, _6008 * _6004)), 0.0f, 1.0f);
                        float _6025 = ((((_6015 * _6015) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                        _6039 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _6025) + 1.0f) * ((_6025 * exp2(log2(1.0f - _5305) * 5.0f)) + 1.0f);
                    }
                    else
                    {
                        _6039 = 1.0f;
                    }
                    bool _6049 = _5305 > 0.0f;
                    float _6413;
                    float _6415;
                    float _6417;
                    if (_6049)
                    {
                        float _6393 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                        float _6394 = _1554 + _62_m0[0u].x;
                        float _6395 = _1555 + _62_m0[0u].y;
                        float _6396 = _1556 + _62_m0[0u].z;
                        float _6400 = rsqrt(dot(float3(_6394, _6395, _6396), float3(_6394, _6395, _6396)));
                        float _6401 = _6400 * _6394;
                        float _6402 = _6400 * _6395;
                        float _6403 = _6400 * _6396;
                        float _6407 = clamp(dot(float3(_837, _840, _843), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                        float _6411 = clamp(dot(float3(_837, _840, _843), float3(_6401, _6402, _6403)), 0.0f, 1.0f);
                        float _6727;
                        float _6728;
                        float _6729;
                        if (_6407 > 0.0f)
                        {
                            float _6694 = _6393 * _6393;
                            float _6695 = _6694 * _6694;
                            float _6699 = (((_6411 * _6695) - _6411) * _6411) + 1.0f;
                            float _6703 = _6694 * 0.5f;
                            float _6704 = 1.0f - _6703;
                            float _6711 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_6401, _6402, _6403)), 0.0f, 1.0f);
                            float _6712 = _6711 * _6711;
                            float _6714 = (_6712 * _6712) * _6711;
                            float _6723 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _6704) + _6703) * ((_6407 * _6704) + _6703))) * (_6695 / ((_6699 * _6699) * 3.1415927410125732421875f)), _1534) * _4408;
                            _6727 = _6723 * ((_6714 * _5299) + _865);
                            _6728 = _6723 * ((_6714 * _5300) + _870);
                            _6729 = _6723 * ((_6714 * _5301) + _873);
                        }
                        else
                        {
                            _6727 = 0.0f;
                            _6728 = 0.0f;
                            _6729 = 0.0f;
                        }
                        float _6730 = min(_6727, 100000.0f);
                        float _6731 = min(_6728, 100000.0f);
                        float _6732 = min(_6729, 100000.0f);
                        float _7041;
                        float _7043;
                        float _7045;
                        if (_4412)
                        {
                            _7041 = _6730;
                            _7043 = _6731;
                            _7045 = _6732;
                        }
                        else
                        {
                            float _7047 = 1.0f - _5305;
                            float _7048 = _7047 * _7047;
                            float _7050 = 1.0f - (_7048 * _7048);
                            _7041 = _6730 * _7050;
                            _7043 = _6731 * _7050;
                            _7045 = _6732 * _7050;
                        }
                        _6413 = _7041 * _5305;
                        _6415 = _7043 * _5305;
                        _6417 = _7045 * _5305;
                    }
                    else
                    {
                        _6413 = 0.0f;
                        _6415 = 0.0f;
                        _6417 = 0.0f;
                    }
                    float _6758;
                    float _6760;
                    float _6762;
                    if (_6049)
                    {
                        float _6738 = max(exp2(log2(clamp(1.0f - _915, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                        float _6739 = _1554 + _62_m0[0u].x;
                        float _6740 = _1555 + _62_m0[0u].y;
                        float _6741 = _1556 + _62_m0[0u].z;
                        float _6745 = rsqrt(dot(float3(_6739, _6740, _6741), float3(_6739, _6740, _6741)));
                        float _6746 = _6745 * _6739;
                        float _6747 = _6745 * _6740;
                        float _6748 = _6745 * _6741;
                        float _6752 = clamp(dot(float3(_837, _840, _843), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                        float _6756 = clamp(dot(float3(_837, _840, _843), float3(_6746, _6747, _6748)), 0.0f, 1.0f);
                        float _7093;
                        float _7094;
                        float _7095;
                        if (_6752 > 0.0f)
                        {
                            float _7060 = _6738 * _6738;
                            float _7061 = _7060 * _7060;
                            float _7065 = (((_6756 * _7061) - _6756) * _6756) + 1.0f;
                            float _7069 = _7060 * 0.5f;
                            float _7070 = 1.0f - _7069;
                            float _7077 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_6746, _6747, _6748)), 0.0f, 1.0f);
                            float _7078 = _7077 * _7077;
                            float _7080 = (_7078 * _7078) * _7077;
                            float _7089 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _7070) + _7069) * ((_6752 * _7070) + _7069))) * (_7061 / ((_7065 * _7065) * 3.1415927410125732421875f)), _1534) * _4408;
                            _7093 = _7089 * ((_7080 * _5299) + _865);
                            _7094 = _7089 * ((_7080 * _5300) + _870);
                            _7095 = _7089 * ((_7080 * _5301) + _873);
                        }
                        else
                        {
                            _7093 = 0.0f;
                            _7094 = 0.0f;
                            _7095 = 0.0f;
                        }
                        float _7096 = min(_7093, 100000.0f);
                        float _7097 = min(_7094, 100000.0f);
                        float _7098 = min(_7095, 100000.0f);
                        float _7754;
                        float _7756;
                        float _7758;
                        if (_4412)
                        {
                            _7754 = _7096;
                            _7756 = _7097;
                            _7758 = _7098;
                        }
                        else
                        {
                            float _7760 = 1.0f - _5305;
                            float _7761 = _7760 * _7760;
                            float _7763 = 1.0f - (_7761 * _7761);
                            _7754 = _7096 * _7763;
                            _7756 = _7097 * _7763;
                            _7758 = _7098 * _7763;
                        }
                        _6758 = _7754 * _5305;
                        _6760 = _7756 * _5305;
                        _6762 = _7758 * _5305;
                    }
                    else
                    {
                        _6758 = 0.0f;
                        _6760 = 0.0f;
                        _6762 = 0.0f;
                    }
                    frontier_phi_261_239_ladder = ((_6758 - _6413) * _917) + _6413;
                    frontier_phi_261_239_ladder_1 = (((_855 * 0.3183098733425140380859375f) * _5300) * _5306) * _6039;
                    frontier_phi_261_239_ladder_2 = (((_850 * 0.3183098733425140380859375f) * _5299) * _5306) * _6039;
                    frontier_phi_261_239_ladder_3 = (((_860 * 0.3183098733425140380859375f) * _5301) * _5306) * _6039;
                    frontier_phi_261_239_ladder_4 = ((_6760 - _6415) * _917) + _6415;
                    frontier_phi_261_239_ladder_5 = ((_6762 - _6417) * _917) + _6417;
                    frontier_phi_261_239_ladder_6 = 0.0f;
                }
                _5425 = frontier_phi_261_239_ladder_2;
                _5430 = frontier_phi_261_239_ladder_1;
                _5435 = frontier_phi_261_239_ladder_3;
                _5440 = frontier_phi_261_239_ladder;
                _5446 = frontier_phi_261_239_ladder_4;
                _5452 = frontier_phi_261_239_ladder_5;
                _5458 = frontier_phi_261_239_ladder_6;
            }
            else
            {
                float _5014 = clamp(dot(float3(_822, _827, _832), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                float frontier_phi_261_240_ladder;
                float frontier_phi_261_240_ladder_1;
                float frontier_phi_261_240_ladder_2;
                float frontier_phi_261_240_ladder_3;
                float frontier_phi_261_240_ladder_4;
                float frontier_phi_261_240_ladder_5;
                float frontier_phi_261_240_ladder_6;
                if (_5014 > 0.0f)
                {
                    float _5313 = _1554 + _62_m0[0u].x;
                    float _5314 = _1555 + _62_m0[0u].y;
                    float _5315 = _1556 + _62_m0[0u].z;
                    float _5319 = rsqrt(dot(float3(_5313, _5314, _5315), float3(_5313, _5314, _5315)));
                    float _5320 = _5319 * _5313;
                    float _5321 = _5319 * _5314;
                    float _5322 = _5319 * _5315;
                    float _5327 = max(clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f), 0.001000000047497451305389404296875f);
                    float _5331 = clamp(dot(float3(_837, _840, _843), float3(_5320, _5321, _5322)), 0.0f, 1.0f);
                    float _5336 = max(1.0f - _876, 0.04500000178813934326171875f);
                    float _5338 = _5336 * _5336;
                    float _5339 = 1.0f / _5338;
                    float _5354 = (((_5339 + 2.0f) * 0.15915493667125701904296875f) * exp2((_5339 * 0.5f) * log2(1.0f - (_5331 * _5331)))) * (0.25f / ((_5327 + _5014) - (_5327 * _5014)));
                    float _5358 = _5338 * _5338;
                    float _5362 = (((_5358 * _5331) - _5331) * _5331) + 1.0f;
                    float _5367 = _5338 * 0.5f;
                    float _5368 = 1.0f - _5367;
                    float _5375 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_5320, _5321, _5322)), 0.0f, 1.0f);
                    float _5376 = _5375 * _5375;
                    float _5378 = (_5376 * _5376) * _5375;
                    float _5390 = min((0.25f / (((_5368 * _5327) + _5367) * ((_5368 * _5014) + _5367))) * (_5358 / ((_5362 * _5362) * 3.1415927410125732421875f)), _1534) * _4408;
                    float _5391 = _5390 * ((_5378 * (1.0f - _865)) + _865);
                    float _5392 = _5390 * ((_5378 * (1.0f - _870)) + _870);
                    float _5393 = _5390 * ((_5378 * (1.0f - _873)) + _873);
                    float _5397 = (1.0f - _923) * _850;
                    float _5398 = (1.0f - _925) * _855;
                    float _5399 = (1.0f - _927) * _860;
                    float _5421 = _5014 * 0.3183098733425140380859375f;
                    frontier_phi_261_240_ladder = ((((_5354 * _923) - _5391) * _929) + _5391) * _5014;
                    frontier_phi_261_240_ladder_1 = _5421 * (((_855 - _5398) * _929) + _5398);
                    frontier_phi_261_240_ladder_2 = _5421 * (((_850 - _5397) * _929) + _5397);
                    frontier_phi_261_240_ladder_3 = _5421 * (((_860 - _5399) * _929) + _5399);
                    frontier_phi_261_240_ladder_4 = ((((_5354 * _925) - _5392) * _929) + _5392) * _5014;
                    frontier_phi_261_240_ladder_5 = ((((_5354 * _927) - _5393) * _929) + _5393) * _5014;
                    frontier_phi_261_240_ladder_6 = 0.0f;
                }
                else
                {
                    frontier_phi_261_240_ladder = 0.0f;
                    frontier_phi_261_240_ladder_1 = 0.0f;
                    frontier_phi_261_240_ladder_2 = 0.0f;
                    frontier_phi_261_240_ladder_3 = 0.0f;
                    frontier_phi_261_240_ladder_4 = 0.0f;
                    frontier_phi_261_240_ladder_5 = 0.0f;
                    frontier_phi_261_240_ladder_6 = 0.0f;
                }
                _5425 = frontier_phi_261_240_ladder_2;
                _5430 = frontier_phi_261_240_ladder_1;
                _5435 = frontier_phi_261_240_ladder_3;
                _5440 = frontier_phi_261_240_ladder;
                _5446 = frontier_phi_261_240_ladder_4;
                _5452 = frontier_phi_261_240_ladder_5;
                _5458 = frontier_phi_261_240_ladder_6;
            }
            float _6494;
            float _6499;
            float _6504;
            if ((_989 & 8388608u) == 0u)
            {
                float frontier_phi_288_275_ladder;
                float frontier_phi_288_275_ladder_1;
                float frontier_phi_288_275_ladder_2;
                if ((_989 & 67108864u) == 0u)
                {
                    float frontier_phi_288_275_ladder_286_ladder;
                    float frontier_phi_288_275_ladder_286_ladder_1;
                    float frontier_phi_288_275_ladder_286_ladder_2;
                    if ((_989 & 50331648u) == 0u)
                    {
                        float frontier_phi_288_275_ladder_286_ladder_301_ladder;
                        float frontier_phi_288_275_ladder_286_ladder_301_ladder_1;
                        float frontier_phi_288_275_ladder_286_ladder_301_ladder_2;
                        if ((_989 & 536870912u) == 0u)
                        {
                            float frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder;
                            float frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder_1;
                            float frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder_2;
                            if (int(_988) < int(0u))
                            {
                                float _7772 = dot(float3(-1.27600002288818359375f, 2.4965000152587890625f, 0.296999990940093994140625f), float3(_57_m0[47u].y * _57_m0[47u].y, (-0.0f) - _57_m0[47u].y, 1.0f));
                                float _7792 = clamp(clamp((dot(float3(_57_m0[47u].xyz), float3(_1554, _1555, _1556)) * 0.5f) + 0.5f, 0.0f, 1.0f), 0.0f, 1.0f);
                                float _7806 = exp2(log2((_7792 * _7792) * (3.0f - (_7792 * 2.0f))) * 3.0f) * exp2(log2(1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_822, _827, _832)), 0.0f, 1.0f)) * 1.5f);
                                float _7810 = _911 * 0.5f;
                                frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder = ((((((_906 * _860) * _882) * _7810) * (1.0f - (_933 * 0.699999988079071044921875f))) * _7772) * _7806) * _57_m0[166u].y;
                                frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder_1 = (((((_906 * _855) * _882) * _7810) * _7772) * _7806) * _57_m0[166u].y;
                                frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder_2 = ((((((_906 * _850) * _882) * _7810) * (1.0f - (_933 * 0.199999988079071044921875f))) * _7772) * _7806) * _57_m0[166u].y;
                            }
                            else
                            {
                                float _7838 = clamp((dot(float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), float3(_62_m0[0u].xyz)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_906 * 0.3183098733425140380859375f);
                                frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder = _7838 * _903;
                                frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder_1 = _7838 * _900;
                                frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder_2 = _7838 * _897;
                            }
                            frontier_phi_288_275_ladder_286_ladder_301_ladder = frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder;
                            frontier_phi_288_275_ladder_286_ladder_301_ladder_1 = frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder_1;
                            frontier_phi_288_275_ladder_286_ladder_301_ladder_2 = frontier_phi_288_275_ladder_286_ladder_301_ladder_315_ladder_2;
                        }
                        else
                        {
                            float _7110 = clamp((dot(float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), float3(_62_m0[0u].xyz)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_906 * 0.3183098733425140380859375f);
                            frontier_phi_288_275_ladder_286_ladder_301_ladder = _7110 * _903;
                            frontier_phi_288_275_ladder_286_ladder_301_ladder_1 = _7110 * _900;
                            frontier_phi_288_275_ladder_286_ladder_301_ladder_2 = _7110 * _897;
                        }
                        frontier_phi_288_275_ladder_286_ladder = frontier_phi_288_275_ladder_286_ladder_301_ladder;
                        frontier_phi_288_275_ladder_286_ladder_1 = frontier_phi_288_275_ladder_286_ladder_301_ladder_1;
                        frontier_phi_288_275_ladder_286_ladder_2 = frontier_phi_288_275_ladder_286_ladder_301_ladder_2;
                    }
                    else
                    {
                        float _6775 = 1.0f - _876;
                        float _6776 = (-0.0f) - _784;
                        float _6777 = (-0.0f) - _778;
                        float _6778 = (-0.0f) - _772;
                        float _6781 = 1.0f - clamp(_976 * 66.6666717529296875f, 0.0f, 1.0f);
                        float _6785 = dot(float3(_6776, _6777, _6778), float3(_62_m0[0u].xyz));
                        float _6791 = dot(float3(_6776, _6777, _6778), float3(_1554, _1555, _1556));
                        float _6797 = _62_m0[0u].x - (_6785 * _6776);
                        float _6798 = _62_m0[0u].y - (_6785 * _6777);
                        float _6799 = _62_m0[0u].z - (_6785 * _6778);
                        float _6803 = _1554 - (_6791 * _6776);
                        float _6804 = _1555 - (_6791 * _6777);
                        float _6805 = _1556 - (_6791 * _6778);
                        float _6818 = rsqrt((dot(float3(_6803, _6804, _6805), float3(_6803, _6804, _6805)) * dot(float3(_6797, _6798, _6799), float3(_6797, _6798, _6799))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_6797, _6798, _6799), float3(_6803, _6804, _6805));
                        float _6820 = (_6818 * 0.5f) + 0.5f;
                        float _6823 = asin(_6791);
                        float _6824 = asin(_6785);
                        float _6828 = cos(abs(_6823 - _6824) * 0.5f);
                        float _6832 = 1.0f / ((1.190000057220458984375f / _6828) + (_6828 * 0.36000001430511474609375f));
                        float _6833 = _6791 * 0.645161330699920654296875f;
                        float _6836 = sqrt(1.0f - (_6833 * _6833));
                        float _6838 = max(_6775 * 0.5f, 0.00999999977648258209228515625f);
                        float _6841 = _6791 + (_6785 - (_986 * 0.5f));
                        float _6849 = exp2((((_6841 * _6841) * (-0.5f)) / (_6838 * _6838)) * 1.44269502162933349609375f) / (_6838 * 2.5066282749176025390625f);
                        float _6858 = clamp((-0.0f) - dot(float3(_1554, _1555, _1556), float3(_62_m0[0u].xyz)), 0.0f, 1.0f);
                        float _6859 = _6858 * _6858;
                        float _6862 = (_6859 * _6859) * (_6858 * _6781);
                        float _6869 = (cos(asin((_6832 * sqrt(clamp(_6820, 0.0f, 1.0f))) * ((_6832 * (0.60000002384185791015625f - (_6818 * 0.800000011920928955078125f))) + 1.0f)) * 2.0f) + 1.0f) * (-2.8853900432586669921875f);
                        float _6873 = exp2(_6869 * (_970 / _6836));
                        float _6874 = exp2(_6869 * (_972 / _6836));
                        float _6875 = exp2(_6869 * (_974 / _6836));
                        float _6890 = 0.95347940921783447265625f - (exp2(log2(1.0f - _6828) * 5.0f) * 0.95347940921783447265625f);
                        float _6891 = _6890 * _6890;
                        float _6893 = (_6828 * 0.5f) + 0.5f;
                        float _6905 = abs(sqrt(1.0f - (_6785 * _6785))) * _15[(_968 + 513u) + 0u].SampleLevel(_65, float2(_6893, _6820), 0.0f).x;
                        float _6906 = _6905 * (_6849 * _6781);
                        float _6913 = _976 + 9899999600270360182784.0f;
                        float _6915 = _876 * 0.75f;
                        float _6919 = (((_978 * _978) * 9000.0f) * _978) * _6913;
                        float _7839;
                        float _7840;
                        float _7841;
                        if ((_989 & 33554432u) == 0u)
                        {
                            float _7111 = max(_6919, _6915);
                            float _7117 = _6824 + _6823;
                            float _7118 = _7117 * 0.5f;
                            float _7120 = _970 * 0.5f;
                            float _7121 = _972 * 0.5f;
                            float _7122 = _974 * 0.5f;
                            float _7123 = max(_6775, 0.0500000007450580596923828125f);
                            float _7126 = (cos(_7118) * 0.5f) + 0.5f;
                            uint _7129 = (_968 + 521u) + 0u;
                            float4 _7134 = _19[_7129].SampleLevel(_65, float3(_7126, _7123, _7120), 0.0f);
                            float _7136 = _7134.x;
                            float4 _7137 = _19[_7129].SampleLevel(_65, float3(_7126, _7123, _7121), 0.0f);
                            float _7139 = _7137.x;
                            float4 _7140 = _19[_7129].SampleLevel(_65, float3(_7126, _7123, _7122), 0.0f);
                            float _7142 = _7140.x;
                            float4 _7143 = _19[_7129].SampleLevel(_65, float3(_6893, _7123, _7120), 0.0f);
                            float4 _7147 = _19[_7129].SampleLevel(_65, float3(_6893, _7123, _7121), 0.0f);
                            float4 _7151 = _19[_7129].SampleLevel(_65, float3(_6893, _7123, _7122), 0.0f);
                            uint _7157 = (_968 + 545u) + 0u;
                            float _7172 = (_7139 + _7136) + _7142;
                            float _7179 = dot(float3(max((1.0f - _980) * _6775, 0.00999999977648258209228515625f), _6838, max(_6775 * 2.0f, 0.00999999977648258209228515625f)), float3(_7136 / _7172, _7139 / _7172, _7142 / _7172)) * _7111;
                            float _7190 = (_6828 * _6828) * 3.1415927410125732421875f;
                            float _7194 = _7179 * 0.5f;
                            float _7195 = _7194 + _7143.z;
                            float _7196 = _7194 + _7147.z;
                            float _7197 = _7194 + _7151.z;
                            float _7200 = (_7117 * (-0.25f)) * _7118;
                            float _7222 = ((_7143.y * 2.0f) * (exp2((_7200 / (_7195 * _7195)) * 1.44269502162933349609375f) / (_7195 * 2.5066282749176025390625f))) / _7190;
                            float _7223 = ((_7147.y * 2.0f) * (exp2((_7200 / (_7196 * _7196)) * 1.44269502162933349609375f) / (_7196 * 2.5066282749176025390625f))) / _7190;
                            float _7224 = ((_7151.y * 2.0f) * (exp2((_7200 / (_7197 * _7197)) * 1.44269502162933349609375f) / (_7197 * 2.5066282749176025390625f))) / _7190;
                            float _7225 = _7117 - _986;
                            float _7228 = max(max(_7179, _7179), _7179) + _6838;
                            float _7237 = exp2((((_7225 * _7225) * (-0.125f)) / (_7228 * _7228)) * 1.44269502162933349609375f) / (_7228 * 2.5066282749176025390625f);
                            _7839 = (((exp2(log2(_7136) * _7111) * 0.4899999797344207763671875f) * ((_7237 * _15[_7157].SampleLevel(_65, float2(_6893, _7120), 0.0f).y) + (_7222 * 2.19911479949951171875f))) + (_7222 * 0.3499999940395355224609375f)) * _982;
                            _7840 = (((exp2(log2(_7139) * _7111) * 0.4899999797344207763671875f) * ((_7237 * _15[_7157].SampleLevel(_65, float2(_6893, _7121), 0.0f).y) + (_7223 * 2.19911479949951171875f))) + (_7223 * 0.3499999940395355224609375f)) * _982;
                            _7841 = (((exp2(log2(_7142) * _7111) * 0.4899999797344207763671875f) * ((_7237 * _15[_7157].SampleLevel(_65, float2(_6893, _7122), 0.0f).y) + (_7224 * 2.19911479949951171875f))) + (_7224 * 0.3499999940395355224609375f)) * _982;
                        }
                        else
                        {
                            float _7269 = 1.0f - clamp((_6913 * 66.6666717529296875f) * max(_978, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f);
                            float _7270 = _7269 * _7269;
                            float _7272 = (_7270 * _7270) * _7269;
                            float _7273 = _6891 * _6849;
                            float _7281 = (_6915 + 1.25f) + _6919;
                            float _7306 = dot(float3(_1554, _1555, _1556), float3(_6776, _6777, _6778));
                            float _7312 = _1554 - (_7306 * _6776);
                            float _7313 = _1555 - (_7306 * _6777);
                            float _7314 = _1556 - (_7306 * _6778);
                            float _7318 = rsqrt(dot(float3(_7312, _7313, _7314), float3(_7312, _7313, _7314)));
                            float _7326 = (dot(float3(_7312 * _7318, _7313 * _7318, _7314 * _7318), float3(_62_m0[0u].xyz)) + 1.0f) * 0.25f;
                            float _7333 = (_982 * 0.2228169143199920654296875f) * ((((1.0f - abs(_6785)) - _7326) * 0.3300000131130218505859375f) + _7326);
                            _7839 = ((_7333 * exp2(log2(exp2(((_970 * (-3.7999999523162841796875f)) / _6828) * 1.44269502162933349609375f)) * _7281)) + ((_6905 * _6873) * _7273)) * _7272;
                            _7840 = ((_7333 * exp2(log2(exp2(((_972 * (-3.7999999523162841796875f)) / _6828) * 1.44269502162933349609375f)) * _7281)) + ((_6905 * _6874) * _7273)) * _7272;
                            _7841 = ((_7333 * exp2(log2(exp2(((_974 * (-3.7999999523162841796875f)) / _6828) * 1.44269502162933349609375f)) * _7281)) + ((_6905 * _6875) * _7273)) * _7272;
                        }
                        frontier_phi_288_275_ladder_286_ladder = _7841 + ((_6906 * (((1.0f - _6875) * _6862) + _6875)) * _6891);
                        frontier_phi_288_275_ladder_286_ladder_1 = _7840 + ((_6906 * (((1.0f - _6874) * _6862) + _6874)) * _6891);
                        frontier_phi_288_275_ladder_286_ladder_2 = _7839 + ((_6906 * (((1.0f - _6873) * _6862) + _6873)) * _6891);
                    }
                    frontier_phi_288_275_ladder = frontier_phi_288_275_ladder_286_ladder;
                    frontier_phi_288_275_ladder_1 = frontier_phi_288_275_ladder_286_ladder_1;
                    frontier_phi_288_275_ladder_2 = frontier_phi_288_275_ladder_286_ladder_2;
                }
                else
                {
                    uint _6421 = _964 + 0u;
                    float _6422 = dot(float3(_62_m0[0u].xyz), float3(_954, _956, _958));
                    float _6437 = dot(float3(_822, _827, _832), float3((_62_m0[0u].y * _958) - (_62_m0[0u].z * _956), (_62_m0[0u].z * _954) - (_62_m0[0u].x * _958), (_62_m0[0u].x * _956) - (_62_m0[0u].y * _954)));
                    float _6446 = float(int(uint(_6437 > 0.0f) - uint(_6437 < 0.0f))) * sqrt(1.0f - (_6422 * _6422));
                    float _6447 = _950 + (-0.5f);
                    float _6448 = _952 + (-0.5f);
                    float _6454 = mad(_6448, _6446, _6422 * _6447) + 0.5f;
                    float _6455 = mad(_6448, _6422, (-0.0f) - (_6447 * _6446)) + 0.5f;
                    float _6461 = (1.0f - clamp(dot(float3(_62_m0[0u].xyz), float3(_822, _827, _832)), 0.0f, 1.0f)) * 5.0f;
                    uint _6462 = uint(int(_6461));
                    float4 _6470 = _41[NonUniformResourceIndex(_6421)].SampleLevel(_69, float3(_6454, _6455, float(int(_6462))), 0.0f);
                    float _6472 = _6470.x;
                    float _6481 = ((_41[NonUniformResourceIndex(_6421)].SampleLevel(_69, float3(_6454, _6455, min(float(int(_6462 + 1u)), 5.0f)), 0.0f).x - _6472) * frac(_6461)) + _6472;
                    frontier_phi_288_275_ladder = (((_860 * 0.3183098733425140380859375f) * _960) * _962) * _6481;
                    frontier_phi_288_275_ladder_1 = (((_855 * 0.3183098733425140380859375f) * _960) * _962) * _6481;
                    frontier_phi_288_275_ladder_2 = (((_850 * 0.3183098733425140380859375f) * _960) * _962) * _6481;
                }
                _6494 = frontier_phi_288_275_ladder_2;
                _6499 = frontier_phi_288_275_ladder_1;
                _6504 = frontier_phi_288_275_ladder;
            }
            else
            {
                float _6056 = log2(max(1.0f - _921, 1.1754943508222875079687365372222e-38f)) * (-6862156513617824209436672.0f);
                float _6058 = _6056 * _6056;
                float _6061 = exp2(_6058 * (-225.4210968017578125f));
                float _6070 = exp2(_6058 * (-29.8077487945556640625f));
                float _6082 = exp2(_6058 * (-7.714946269989013671875f));
                float _6091 = exp2(_6058 * (-2.5444357395172119140625f));
                float _6094 = _6091 * 0.007000000216066837310791015625f;
                float _6101 = exp2(_6058 * (-0.72497236728668212890625f));
                float _6121 = max(dot(float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), float3(_62_m0[0u].xyz)) + 0.300000011920928955078125f, 0.0f);
                _6494 = (((((((_6070 * 0.100000001490116119384765625f) + (_6061 * 0.23299999535083770751953125f)) + (_6082 * 0.1180000007152557373046875f)) + (_6091 * 0.112999998033046722412109375f)) + (_6101 * 0.3580000102519989013671875f)) + (exp2(_6058 * (-0.1946956813335418701171875f)) * 0.078000001609325408935546875f)) * _850) * _6121;
                _6499 = ((((((_6070 * 0.3359999954700469970703125f) + (_6061 * 0.4550000131130218505859375f)) + (_6082 * 0.19799999892711639404296875f)) + _6094) + (_6101 * 0.0040000001899898052215576171875f)) * _855) * _6121;
                _6504 = ((((_6070 * 0.3440000116825103759765625f) + (_6061 * 0.648999989032745361328125f)) + _6094) * _860) * _6121;
            }
            float _6509 = _4456 * _1535;
            float _6510 = _4456 * _1536;
            float _6511 = _4456 * _1537;
            float _6920;
            if (_991)
            {
                _6920 = 1.0f;
            }
            else
            {
                _6920 = _57_m0[677u].w;
            }
            float _7343;
            float _7346;
            float _7349;
            float _7352;
            float _7355;
            float _7358;
            float _7361;
            float _7363;
            float _7365;
            float _7367;
            float _7369;
            float _7371;
            if (asuint(_62_m0[59u]).w == 0u)
            {
                _7343 = 0.0f;
                _7346 = 0.0f;
                _7349 = 0.0f;
                _7352 = 0.0f;
                _7355 = 0.0f;
                _7358 = 0.0f;
                _7361 = 0.0f;
                _7363 = 0.0f;
                _7365 = 0.0f;
                _7367 = 0.0f;
                _7369 = 0.0f;
                _7371 = 0.0f;
            }
            else
            {
                uint _7400 = uint(int(_62_m0[58u].x * _296));
                uint _7401 = uint(int(_62_m0[58u].y * _298));
                uint _7402 = uint(int(max(_62_m0[60u].y - (_62_m0[60u].x * log2((_62_m0[58u].z * _293) + _62_m0[58u].w)), 0.0f)));
                uint4 _7404 = _44.Load(int4(uint3(_7400, _7401, _7402), 0u));
                uint _7406 = _7404.x;
                uint4 _7409 = asuint(_62_m0[89u]);
                uint _7420 = (((_7401 << (_7409.x & 31u)) + _7400) + (_7402 << (_7409.y & 31u))) << (_7409.z & 31u);
                float frontier_phi_319_320_ladder;
                float frontier_phi_319_320_ladder_1;
                float frontier_phi_319_320_ladder_2;
                float frontier_phi_319_320_ladder_3;
                float frontier_phi_319_320_ladder_4;
                float frontier_phi_319_320_ladder_5;
                float frontier_phi_319_320_ladder_6;
                float frontier_phi_319_320_ladder_7;
                float frontier_phi_319_320_ladder_8;
                float frontier_phi_319_320_ladder_9;
                float frontier_phi_319_320_ladder_10;
                float frontier_phi_319_320_ladder_11;
                if (_7406 == 0u)
                {
                    frontier_phi_319_320_ladder = 0.0f;
                    frontier_phi_319_320_ladder_1 = 0.0f;
                    frontier_phi_319_320_ladder_2 = 0.0f;
                    frontier_phi_319_320_ladder_3 = 0.0f;
                    frontier_phi_319_320_ladder_4 = 0.0f;
                    frontier_phi_319_320_ladder_5 = 0.0f;
                    frontier_phi_319_320_ladder_6 = 0.0f;
                    frontier_phi_319_320_ladder_7 = 0.0f;
                    frontier_phi_319_320_ladder_8 = 0.0f;
                    frontier_phi_319_320_ladder_9 = 0.0f;
                    frontier_phi_319_320_ladder_10 = 0.0f;
                    frontier_phi_319_320_ladder_11 = 0.0f;
                }
                else
                {
                    float _7864 = _369 - _62_m0[59u].x;
                    float _7865 = _370 - _62_m0[59u].y;
                    float _7866 = _371 - _62_m0[59u].z;
                    uint4 _7876 = _45.Load(_7420);
                    uint _7877 = _7876.x;
                    uint _7878 = _7420 + (_7406 & 127u);
                    uint _7879 = _7878 + ((_7406 >> 7u) & 127u);
                    uint _7880 = _7879 + ((_7406 >> 20u) & 63u);
                    uint _7881 = _7880 + ((_7406 >> 14u) & 63u);
                    uint _7882 = _7881 + (_7406 >> 26u);
                    uint _7883 = _7420 + 1u;
                    float _8037;
                    float _8039;
                    float _8041;
                    float _8043;
                    float _8045;
                    float _8047;
                    float _8049;
                    float _8051;
                    float _8053;
                    uint _8055;
                    uint _8057;
                    if (_7883 > _7878)
                    {
                        _8037 = 0.0f;
                        _8039 = 0.0f;
                        _8041 = 0.0f;
                        _8043 = 0.0f;
                        _8045 = 0.0f;
                        _8047 = 0.0f;
                        _8049 = 0.0f;
                        _8051 = 0.0f;
                        _8053 = 0.0f;
                        _8055 = _7883;
                        _8057 = _7877;
                    }
                    else
                    {
                        float _8038;
                        float _8040;
                        float _8042;
                        float _8044;
                        float _8046;
                        float _8048;
                        float _8050;
                        float _8052;
                        float _8054;
                        float _8195 = 0.0f;
                        float _8196 = 0.0f;
                        float _8197 = 0.0f;
                        float _8198 = 0.0f;
                        float _8199 = 0.0f;
                        float _8200 = 0.0f;
                        float _8201 = 0.0f;
                        float _8202 = 0.0f;
                        float _8203 = 0.0f;
                        uint _8204 = _7883;
                        uint _8205 = _7877;
                        uint _8056;
                        uint _8208;
                        uint _8223;
                        uint _8224;
                        float _8248;
                        float _8250;
                        float _8253;
                        float _8255;
                        uint _8257;
                        uint _8258;
                        bool _8260;
                        float _8262;
                        bool _8268;
                        for (;;)
                        {
                            _8056 = _8204 + 1u;
                            _8208 = _45.Load(_8204).x;
                            uint _8210 = _8205 * 4u;
                            uint4 _8222 = uint4(_46.Load(_8210).x, _46.Load(_8210 + 1u).x, _46.Load(_8210 + 2u).x, _46.Load(_8210 + 3u).x);
                            _8223 = _8222.x;
                            _8224 = _8222.y;
                            uint _8225 = _8222.z;
                            uint _8226 = _8222.w;
                            _8248 = spvUnpackHalf2x16(_8224 >> 16u).x;
                            _8250 = spvUnpackHalf2x16(_8225).x;
                            _8253 = spvUnpackHalf2x16(_8225 >> 16u).x;
                            _8255 = spvUnpackHalf2x16(_8226).x;
                            _8257 = (_8226 >> 16u) & 7u;
                            _8258 = _8226 & 524288u;
                            _8260 = int(uint4(_8229, _8230, _8231, _47.Load((_8205 * 4u) + 3u).x).w) < int(0u);
                            _8262 = spvUnpackHalf2x16(uint3(_8239, _8240, _48.Load((_8205 * 4u) + 2u).x).z).x;
                            _8268 = (_8226 < 3221225472u) && (((_499 & 255u) & (_8226 >> 22u)) != 0u);
                            float frontier_phi_360_pred;
                            float frontier_phi_360_pred_1;
                            float frontier_phi_360_pred_2;
                            float frontier_phi_360_pred_3;
                            float frontier_phi_360_pred_4;
                            float frontier_phi_360_pred_5;
                            float frontier_phi_360_pred_6;
                            float frontier_phi_360_pred_7;
                            float frontier_phi_360_pred_8;
                            if (_8268)
                            {
                                float _8398 = spvUnpackHalf2x16(_8223).x - _7864;
                                float _8399 = spvUnpackHalf2x16(_8223 >> 16u).x - _7865;
                                float _8400 = spvUnpackHalf2x16(_8224).x - _7866;
                                float _8406 = sqrt(((_8399 * _8399) + (_8400 * _8400)) + (_8398 * _8398));
                                float _8407 = _8406 * _8248;
                                float frontier_phi_360_pred_359_ladder;
                                float frontier_phi_360_pred_359_ladder_1;
                                float frontier_phi_360_pred_359_ladder_2;
                                float frontier_phi_360_pred_359_ladder_3;
                                float frontier_phi_360_pred_359_ladder_4;
                                float frontier_phi_360_pred_359_ladder_5;
                                float frontier_phi_360_pred_359_ladder_6;
                                float frontier_phi_360_pred_359_ladder_7;
                                float frontier_phi_360_pred_359_ladder_8;
                                if (_8407 < 1.0f)
                                {
                                    float _8544 = rsqrt(dot(float3(_8398, _8399, _8400), float3(_8398, _8399, _8400)));
                                    float _8545 = _8544 * _8398;
                                    float _8546 = _8544 * _8399;
                                    float _8547 = _8544 * _8400;
                                    float _8548 = _8406 * _8406;
                                    float _8550 = (_8248 * _8248) * _8548;
                                    float _8553 = clamp(1.0f - (_8550 * _8550), 0.0f, 1.0f);
                                    float _8803;
                                    if (_8257 == 0u)
                                    {
                                        _8803 = (_8553 * _8553) * (1.0f / (max(_8548, 9.9999997473787516355514526367188e-05f) + ((_8262 * _8262) * 0.5f)));
                                    }
                                    else
                                    {
                                        _8803 = max((1.0f / dot(float3(1.0f, _8407, _8407 * _8407), float3(_62_m0[_8257 + 60u].xyz))) * (1.0f - _8407), 0.0f);
                                    }
                                    float _8808 = max(float(_8260) * 16.0f, 1.0f) * _8803;
                                    float _8809 = _8808 * _8250;
                                    float _8810 = _8808 * _8253;
                                    float _8811 = _8808 * _8255;
                                    float _9128;
                                    float _9130;
                                    float _9132;
                                    float _9134;
                                    float _9136;
                                    float _9138;
                                    float _9140;
                                    float _9142;
                                    float _9144;
                                    if (_8258 == 0u)
                                    {
                                        float _8972 = clamp(clamp(dot(float3(_822, _827, _832), float3(_8545, _8546, _8547)), 0.0f, 1.0f), 0.0f, 1.0f);
                                        _9128 = (_8972 * ((_850 * 0.3183098733425140380859375f) * (1.0f - _865))) * _8809;
                                        _9130 = (_8972 * ((_855 * 0.3183098733425140380859375f) * (1.0f - _870))) * _8810;
                                        _9132 = (_8972 * ((_860 * 0.3183098733425140380859375f) * (1.0f - _873))) * _8811;
                                        _9134 = 0.0f;
                                        _9136 = 0.0f;
                                        _9138 = 0.0f;
                                        _9140 = 0.0f;
                                        _9142 = 0.0f;
                                        _9144 = 0.0f;
                                    }
                                    else
                                    {
                                        float _9321;
                                        float _9326;
                                        float _9331;
                                        float _9336;
                                        float _9342;
                                        float _9348;
                                        float _9354;
                                        float _9356;
                                        float _9358;
                                        if ((_989 & 4194304u) == 0u)
                                        {
                                            float frontier_phi_411_402_ladder;
                                            float frontier_phi_411_402_ladder_1;
                                            float frontier_phi_411_402_ladder_2;
                                            float frontier_phi_411_402_ladder_3;
                                            float frontier_phi_411_402_ladder_4;
                                            float frontier_phi_411_402_ladder_5;
                                            float frontier_phi_411_402_ladder_6;
                                            float frontier_phi_411_402_ladder_7;
                                            float frontier_phi_411_402_ladder_8;
                                            if ((_989 & 8388608u) == 0u)
                                            {
                                                float frontier_phi_411_402_ladder_408_ladder;
                                                float frontier_phi_411_402_ladder_408_ladder_1;
                                                float frontier_phi_411_402_ladder_408_ladder_2;
                                                float frontier_phi_411_402_ladder_408_ladder_3;
                                                float frontier_phi_411_402_ladder_408_ladder_4;
                                                float frontier_phi_411_402_ladder_408_ladder_5;
                                                float frontier_phi_411_402_ladder_408_ladder_6;
                                                float frontier_phi_411_402_ladder_408_ladder_7;
                                                float frontier_phi_411_402_ladder_408_ladder_8;
                                                if ((_989 & 67108864u) == 0u)
                                                {
                                                    float frontier_phi_411_402_ladder_408_ladder_417_ladder;
                                                    float frontier_phi_411_402_ladder_408_ladder_417_ladder_1;
                                                    float frontier_phi_411_402_ladder_408_ladder_417_ladder_2;
                                                    float frontier_phi_411_402_ladder_408_ladder_417_ladder_3;
                                                    float frontier_phi_411_402_ladder_408_ladder_417_ladder_4;
                                                    float frontier_phi_411_402_ladder_408_ladder_417_ladder_5;
                                                    float frontier_phi_411_402_ladder_408_ladder_417_ladder_6;
                                                    float frontier_phi_411_402_ladder_408_ladder_417_ladder_7;
                                                    float frontier_phi_411_402_ladder_408_ladder_417_ladder_8;
                                                    if ((_989 & 50331648u) == 0u)
                                                    {
                                                        float frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder;
                                                        float frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_1;
                                                        float frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_2;
                                                        float frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_3;
                                                        float frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_4;
                                                        float frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_5;
                                                        float frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_6;
                                                        float frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_7;
                                                        float frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_8;
                                                        if ((_989 & 1032192u) == 0u)
                                                        {
                                                            float _9325;
                                                            float _9330;
                                                            float _9335;
                                                            float _9355;
                                                            float _9357;
                                                            float _9359;
                                                            if ((_935 > 0.0f) && ((_989 & 3u) != 0u))
                                                            {
                                                                float _10858 = max(max(max(max(_938, 0.0f), _935), _940), 0.0f);
                                                                float _10864 = _57_m0[8u].x - _819;
                                                                float _10865 = _57_m0[8u].y - _820;
                                                                float _10866 = _57_m0[8u].z - _821;
                                                                float _10877 = clamp(2.0f - (clamp(sqrt(((_10864 * _10864) + (_10865 * _10865)) + (_10866 * _10866)) * 0.02500000037252902984619140625f, 0.0f, 1.0f) * 2.0f), 0.0f, 1.0f);
                                                                float _10878 = _10877 * _10858;
                                                                float _10884 = _57_m0[21u].x - _819;
                                                                float _10885 = _57_m0[21u].y - _820;
                                                                float _10886 = _57_m0[21u].z - _821;
                                                                float _10893 = log2(sqrt(((_10884 * _10884) + (_10885 * _10885)) + (_10886 * _10886)));
                                                                float _10894 = _10893 * 0.85000002384185791015625f;
                                                                float _11655;
                                                                float _11656;
                                                                float _11657;
                                                                float _11658;
                                                                if (_10878 > 0.00999999977648258209228515625f)
                                                                {
                                                                    uint _11489 = _937 + 0u;
                                                                    float _11490 = ceil(_10894);
                                                                    float _11494 = _819 * 5.0f;
                                                                    float _11495 = _821 * 5.0f;
                                                                    float _11497 = exp2((-0.0f) - max(1.0f, _11490));
                                                                    float _11503 = exp2((-0.0f) - max(1.0f, _11490 + 1.0f));
                                                                    float4 _11512 = _37[NonUniformResourceIndex(_11489)].SampleLevel(_70, float2(frac(_11497 * _11494), frac(_11497 * _11495)), 0.0f);
                                                                    float4 _11517 = _37[NonUniformResourceIndex(_11489)].SampleLevel(_70, float2(frac(_11503 * _11494), frac(_11503 * _11495)), 0.0f);
                                                                    float _11522 = frac(_10894);
                                                                    float _11524 = (_11522 + 0.5f) * 0.5f;
                                                                    float _11536 = (_11512.x + (-0.5f)) * 2.0f;
                                                                    float _11537 = (_11512.y + (-0.5f)) * 2.0f;
                                                                    float _11543 = sqrt(clamp((1.0f - (_11536 * _11536)) - (_11537 * _11537), 0.0f, 1.0f));
                                                                    float _11550 = (_11517.x + (-0.5f)) * 2.0f;
                                                                    float _11551 = (_11517.y + (-0.5f)) * 2.0f;
                                                                    float _11557 = sqrt(clamp((1.0f - (_11550 * _11550)) - (_11551 * _11551), 0.0f, 1.0f));
                                                                    float _11563 = rsqrt(dot(float3(_11536, _11543, _11537), float3(_11536, _11543, _11537))) * (1.0f - _11522);
                                                                    float _11566 = rsqrt(dot(float3(_11550, _11557, _11551), float3(_11550, _11557, _11551))) * _11522;
                                                                    float _11569 = (_11566 * _11550) + (_11563 * _11536);
                                                                    float _11570 = (_11566 * _11551) + (_11563 * _11537);
                                                                    float _11574 = rsqrt(dot(float3(_11569, _11570, 1.0f), float3(_11569, _11570, 1.0f)));
                                                                    float _11577 = mad(_11574, _822, _11569 * _11574);
                                                                    float _11578 = mad(_11574, _827, 0.0f);
                                                                    float _11579 = mad(_11574, _832, _11570 * _11574);
                                                                    float _11583 = rsqrt(dot(float3(_11577, _11578, _11579), float3(_11577, _11578, _11579)));
                                                                    float _11584 = _11583 * _11577;
                                                                    float _11585 = _11583 * _11578;
                                                                    float _11586 = _11583 * _11579;
                                                                    float _11587 = _1555 + 1.0f;
                                                                    float _11591 = rsqrt(dot(float3(_1554, _11587, _1556), float3(_1554, _11587, _1556)));
                                                                    float _11634 = (((((_10878 * 2.2000000476837158203125f) * max(clamp(_11524 * (_11512.z - _11522), 0.0f, 1.0f), clamp((1.0f - _11524) * ((_11522 + (-1.0f)) + _11517.z), 0.0f, 1.0f))) * exp2((800.0f - (clamp(clamp((_10893 * 0.425000011920928955078125f) + (-1.0f), 0.0f, 1.0f), 0.0f, 1.0f) * 800.0f)) * log2(((_10858 * (0.004999999888241291046142578125f - (_940 * 0.00299999979324638843536376953125f))) + 0.00200000009499490261077880859375f) + max(dot(float3(_11591 * _1554, _11591 * _11587, _11591 * _1556), float3(_11584, _11585, _11586)), 0.0f)))) * exp2(log2(clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 1.2000000476837158203125f)) * clamp((dot(float3(_8545, _8546, _8547), float3(_822, _827, _832)) * 10.0f) + 0.5f, 0.0f, 1.0f)) * _57_m0[166u].y;
                                                                    float _11636 = (_940 * 0.4000000059604644775390625f) * _10877;
                                                                    float _11639 = ((_11586 + (-1.0f)) * _11636) + 1.0f;
                                                                    float _11642 = _11636 * _832;
                                                                    float _11645 = (_11639 * _822) + (_11584 * _11642);
                                                                    float _11646 = (_11639 * _827) + (_11585 * _11642);
                                                                    float _11647 = _11639 * _832;
                                                                    float _11651 = rsqrt(dot(float3(_11645, _11646, _11647), float3(_11645, _11646, _11647)));
                                                                    _11655 = _11634;
                                                                    _11656 = _11651 * _11645;
                                                                    _11657 = _11651 * _11646;
                                                                    _11658 = _11651 * _11647;
                                                                }
                                                                else
                                                                {
                                                                    _11655 = 0.0f;
                                                                    _11656 = _822;
                                                                    _11657 = _827;
                                                                    _11658 = _832;
                                                                }
                                                                float _11663 = dot(float3(_11656, _11657, _11658), float3(_8545, _8546, _8547));
                                                                float _11669 = (1.0f - clamp(_11663 * 5.0f, 0.0f, 1.0f)) * _938;
                                                                float _11676 = (_11669 * (_850 - _942)) + _942;
                                                                float _11677 = (_11669 * (_855 - _944)) + _944;
                                                                float _11678 = (_11669 * (_860 - _946)) + _946;
                                                                float _11679 = 1.0f - _865;
                                                                float _11680 = 1.0f - _870;
                                                                float _11681 = 1.0f - _873;
                                                                float _11689 = exp2(log2(1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_11656, _11657, _11658)), 0.0f, 1.0f)) * 5.0f);
                                                                float _11696 = _940 * 0.550000011920928955078125f;
                                                                float _11697 = ((_11689 * _11679) + _865) * _11696;
                                                                float _11698 = ((_11689 * _11680) + _870) * _11696;
                                                                float _11699 = ((_11689 * _11681) + _873) * _11696;
                                                                float _11718 = clamp(_11663, 0.0f, 1.0f);
                                                                float _12667;
                                                                if (_57_m0[76u].w > 0.5f)
                                                                {
                                                                    float _12630 = _8545 + _1554;
                                                                    float _12631 = _8546 + _1555;
                                                                    float _12632 = _8547 + _1556;
                                                                    float _12636 = rsqrt(dot(float3(_12630, _12631, _12632), float3(_12630, _12631, _12632)));
                                                                    float _12643 = clamp(dot(float3(_8545, _8546, _8547), float3(_12636 * _12630, _12636 * _12631, _12636 * _12632)), 0.0f, 1.0f);
                                                                    float _12653 = ((((_12643 * _12643) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                                    _12667 = ((exp2(log2(1.0f - clamp(dot(float3(_11656, _11657, _11658), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _12653) + 1.0f) * ((_12653 * exp2(log2(1.0f - _11718) * 5.0f)) + 1.0f);
                                                                }
                                                                else
                                                                {
                                                                    _12667 = 1.0f;
                                                                }
                                                                float _12669 = _12667 * (clamp((_11718 * 0.722500026226043701171875f) + 0.12750001251697540283203125f, 0.0f, 1.0f) * 0.3183098733425140380859375f);
                                                                _9325 = (_12669 * _11679) * (((_11697 * _11697) * ((0.949999988079071044921875f - (_938 * 0.5f)) - _11676)) + _11676);
                                                                _9330 = (_12669 * _11680) * (((_11698 * _11698) * ((0.810000002384185791015625f - (_938 * 0.069999992847442626953125f)) - _11677)) + _11677);
                                                                _9335 = (_12669 * _11681) * (((_11699 * _11699) * (((_938 * 0.37999999523162841796875f) + 0.569999992847442626953125f) - _11678)) + _11678);
                                                                _9355 = _11655 * 0.449999988079071044921875f;
                                                                _9357 = _11655 * 0.449999988079071044921875f;
                                                                _9359 = _11655 * 0.449999988079071044921875f;
                                                            }
                                                            else
                                                            {
                                                                float _10902 = clamp(dot(float3(_822, _827, _832), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                                float _10903 = clamp(_10902, 0.0f, 1.0f);
                                                                float _11763;
                                                                if (_57_m0[76u].w > 0.5f)
                                                                {
                                                                    float _11726 = _8545 + _1554;
                                                                    float _11727 = _8546 + _1555;
                                                                    float _11728 = _8547 + _1556;
                                                                    float _11732 = rsqrt(dot(float3(_11726, _11727, _11728), float3(_11726, _11727, _11728)));
                                                                    float _11739 = clamp(dot(float3(_8545, _8546, _8547), float3(_11732 * _11726, _11732 * _11727, _11732 * _11728)), 0.0f, 1.0f);
                                                                    float _11749 = ((((_11739 * _11739) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                                    _11763 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _11749) + 1.0f) * ((_11749 * exp2(log2(1.0f - _10902) * 5.0f)) + 1.0f);
                                                                }
                                                                else
                                                                {
                                                                    _11763 = 1.0f;
                                                                }
                                                                _9325 = (((_850 * 0.3183098733425140380859375f) * (1.0f - _865)) * _10903) * _11763;
                                                                _9330 = (((_855 * 0.3183098733425140380859375f) * (1.0f - _870)) * _10903) * _11763;
                                                                _9335 = (((_860 * 0.3183098733425140380859375f) * (1.0f - _873)) * _10903) * _11763;
                                                                _9355 = 0.0f;
                                                                _9357 = 0.0f;
                                                                _9359 = 0.0f;
                                                            }
                                                            float _12679 = clamp(dot(float3(_837, _840, _843), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                            float _12902;
                                                            float _12904;
                                                            float _12906;
                                                            if (_12679 > 0.0f)
                                                            {
                                                                float _12839 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                                float _12840 = _8545 + _1554;
                                                                float _12841 = _8546 + _1555;
                                                                float _12842 = _8547 + _1556;
                                                                float _12846 = rsqrt(dot(float3(_12840, _12841, _12842), float3(_12840, _12841, _12842)));
                                                                float _12847 = _12846 * _12840;
                                                                float _12848 = _12846 * _12841;
                                                                float _12849 = _12846 * _12842;
                                                                float _12858 = clamp(dot(float3(_837, _840, _843), float3(_12847, _12848, _12849)), 0.0f, 1.0f);
                                                                float _12863 = _12839 * _12839;
                                                                float _12864 = _12863 * _12863;
                                                                float _12868 = (((_12858 * _12864) - _12858) * _12858) + 1.0f;
                                                                float _12872 = _12863 * 0.5f;
                                                                float _12873 = 1.0f - _12872;
                                                                float _12880 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_12847, _12848, _12849)), 0.0f, 1.0f);
                                                                float _12881 = _12880 * _12880;
                                                                float _12883 = (_12881 * _12881) * _12880;
                                                                float _12895 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _12873) + _12872) * ((_12873 * _12679) + _12872))) * (_12864 / ((_12868 * _12868) * 3.1415927410125732421875f)), _1534) * _4408;
                                                                float _12899 = min(_12895 * ((_12883 * (1.0f - _865)) + _865), 100000.0f);
                                                                float _12900 = min(_12895 * ((_12883 * (1.0f - _870)) + _870), 100000.0f);
                                                                float _12901 = min(_12895 * ((_12883 * (1.0f - _873)) + _873), 100000.0f);
                                                                float _13002;
                                                                float _13004;
                                                                float _13006;
                                                                if (_4412)
                                                                {
                                                                    _13002 = _12899;
                                                                    _13004 = _12900;
                                                                    _13006 = _12901;
                                                                }
                                                                else
                                                                {
                                                                    float _13008 = 1.0f - _12679;
                                                                    float _13009 = _13008 * _13008;
                                                                    float _13011 = 1.0f - (_13009 * _13009);
                                                                    _13002 = _12899 * _13011;
                                                                    _13004 = _12900 * _13011;
                                                                    _13006 = _12901 * _13011;
                                                                }
                                                                _12902 = _13002 * _12679;
                                                                _12904 = _13004 * _12679;
                                                                _12906 = _13006 * _12679;
                                                            }
                                                            else
                                                            {
                                                                _12902 = 0.0f;
                                                                _12904 = 0.0f;
                                                                _12906 = 0.0f;
                                                            }
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder = _12904 * _892;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_1 = _12902 * _892;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_2 = _9335;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_3 = _9325;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_4 = _9330;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_5 = _9355;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_6 = _9357;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_7 = _12906 * _892;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_8 = _9359;
                                                        }
                                                        else
                                                        {
                                                            float _10276 = 1.0f - _865;
                                                            float _10277 = 1.0f - _870;
                                                            float _10278 = 1.0f - _873;
                                                            float _10282 = clamp(dot(float3(_822, _827, _832), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                            float _10283 = clamp(_10282, 0.0f, 1.0f);
                                                            float _10945;
                                                            if (_57_m0[76u].w > 0.5f)
                                                            {
                                                                float _10908 = _8545 + _1554;
                                                                float _10909 = _8546 + _1555;
                                                                float _10910 = _8547 + _1556;
                                                                float _10914 = rsqrt(dot(float3(_10908, _10909, _10910), float3(_10908, _10909, _10910)));
                                                                float _10921 = clamp(dot(float3(_8545, _8546, _8547), float3(_10914 * _10908, _10914 * _10909, _10914 * _10910)), 0.0f, 1.0f);
                                                                float _10931 = ((((_10921 * _10921) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                                _10945 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _10931) + 1.0f) * ((_10931 * exp2(log2(1.0f - _10282) * 5.0f)) + 1.0f);
                                                            }
                                                            else
                                                            {
                                                                _10945 = 1.0f;
                                                            }
                                                            float _11801;
                                                            float _11803;
                                                            float _11805;
                                                            if (_10282 > 0.0f)
                                                            {
                                                                float _11781 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                                float _11782 = _8545 + _1554;
                                                                float _11783 = _8546 + _1555;
                                                                float _11784 = _8547 + _1556;
                                                                float _11788 = rsqrt(dot(float3(_11782, _11783, _11784), float3(_11782, _11783, _11784)));
                                                                float _11789 = _11788 * _11782;
                                                                float _11790 = _11788 * _11783;
                                                                float _11791 = _11788 * _11784;
                                                                float _11795 = clamp(dot(float3(_837, _840, _843), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                                float _11799 = clamp(dot(float3(_837, _840, _843), float3(_11789, _11790, _11791)), 0.0f, 1.0f);
                                                                float _12723;
                                                                float _12724;
                                                                float _12725;
                                                                if (_11795 > 0.0f)
                                                                {
                                                                    float _12690 = _11781 * _11781;
                                                                    float _12691 = _12690 * _12690;
                                                                    float _12695 = (((_11799 * _12691) - _11799) * _11799) + 1.0f;
                                                                    float _12699 = _12690 * 0.5f;
                                                                    float _12700 = 1.0f - _12699;
                                                                    float _12707 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_11789, _11790, _11791)), 0.0f, 1.0f);
                                                                    float _12708 = _12707 * _12707;
                                                                    float _12710 = (_12708 * _12708) * _12707;
                                                                    float _12719 = min((0.25f / (((_11795 * _12700) + _12699) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _12700) + _12699))) * (_12691 / ((_12695 * _12695) * 3.1415927410125732421875f)), _1534) * _4408;
                                                                    _12723 = _12719 * ((_12710 * _10276) + _865);
                                                                    _12724 = _12719 * ((_12710 * _10277) + _870);
                                                                    _12725 = _12719 * ((_12710 * _10278) + _873);
                                                                }
                                                                else
                                                                {
                                                                    _12723 = 0.0f;
                                                                    _12724 = 0.0f;
                                                                    _12725 = 0.0f;
                                                                }
                                                                float _12726 = min(_12723, 100000.0f);
                                                                float _12727 = min(_12724, 100000.0f);
                                                                float _12728 = min(_12725, 100000.0f);
                                                                float _12908;
                                                                float _12910;
                                                                float _12912;
                                                                if (_4412)
                                                                {
                                                                    _12908 = _12726;
                                                                    _12910 = _12727;
                                                                    _12912 = _12728;
                                                                }
                                                                else
                                                                {
                                                                    float _12914 = 1.0f - _10282;
                                                                    float _12915 = _12914 * _12914;
                                                                    float _12917 = 1.0f - (_12915 * _12915);
                                                                    _12908 = _12726 * _12917;
                                                                    _12910 = _12727 * _12917;
                                                                    _12912 = _12728 * _12917;
                                                                }
                                                                _11801 = _12908 * _10282;
                                                                _11803 = _12910 * _10282;
                                                                _11805 = _12912 * _10282;
                                                            }
                                                            else
                                                            {
                                                                _11801 = 0.0f;
                                                                _11803 = 0.0f;
                                                                _11805 = 0.0f;
                                                            }
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder = _11803 * _892;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_1 = _11801 * _892;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_2 = (((_860 * 0.3183098733425140380859375f) * _10278) * _10283) * _10945;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_3 = (((_850 * 0.3183098733425140380859375f) * _10276) * _10283) * _10945;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_4 = (((_855 * 0.3183098733425140380859375f) * _10277) * _10283) * _10945;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_5 = 0.0f;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_6 = 0.0f;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_7 = _11805 * _892;
                                                            frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_8 = 0.0f;
                                                        }
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder = frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_1 = frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_1;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_2 = frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_2;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_3 = frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_3;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_4 = frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_4;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_5 = frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_5;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_6 = frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_6;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_7 = frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_7;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_8 = frontier_phi_411_402_ladder_408_ladder_417_ladder_429_ladder_8;
                                                    }
                                                    else
                                                    {
                                                        float _9719 = 1.0f - _876;
                                                        float _9720 = (-0.0f) - _784;
                                                        float _9721 = (-0.0f) - _778;
                                                        float _9722 = (-0.0f) - _772;
                                                        float _9730 = dot(float3(_9720, _9721, _9722), float3(_8545, _8546, _8547));
                                                        float _9736 = dot(float3(_9720, _9721, _9722), float3(_1554, _1555, _1556));
                                                        float _9742 = _8545 - (_9730 * _9720);
                                                        float _9743 = _8546 - (_9730 * _9721);
                                                        float _9744 = _8547 - (_9730 * _9722);
                                                        float _9748 = _1554 - (_9736 * _9720);
                                                        float _9749 = _1555 - (_9736 * _9721);
                                                        float _9750 = _1556 - (_9736 * _9722);
                                                        float _9763 = rsqrt((dot(float3(_9748, _9749, _9750), float3(_9748, _9749, _9750)) * dot(float3(_9742, _9743, _9744), float3(_9742, _9743, _9744))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_9742, _9743, _9744), float3(_9748, _9749, _9750));
                                                        float _9767 = sqrt(clamp((_9763 * 0.5f) + 0.5f, 0.0f, 1.0f));
                                                        float _9773 = cos(abs(asin(_9736) - asin(_9730)) * 0.5f);
                                                        float _9778 = max(_9719 * 2.0f, 0.00999999977648258209228515625f);
                                                        float _9780 = (-0.0f) - _986;
                                                        float _9782 = sin(_9780);
                                                        float _9793 = _9736 + _9730;
                                                        float _9794 = _9793 - ((_9782 * 2.0f) * (((cos(_9780) * _9767) * sqrt(1.0f - (_9736 * _9736))) + (_9782 * _9736)));
                                                        float _9796 = (_9767 * 1.41421353816986083984375f) * max((1.0f - _980) * _9719, 0.00999999977648258209228515625f);
                                                        float _9818 = _9793 - (_986 * 1.5f);
                                                        float _9844 = exp2(log2(1.0f - (_9773 * 0.5f)) * 5.0f) * 0.95347940921783447265625f;
                                                        float _9846 = 0.95347940921783447265625f - _9844;
                                                        float _9851 = clamp((1.0f - max(_984, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                                                        float _9857 = abs(sqrt(1.0f - (_9730 * _9730))) * clamp(dot(float3(_822, _827, _832), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                        float _9858 = (((_9767 * 0.25f) * (exp2((((_9794 * _9794) * (-0.5f)) / (_9796 * _9796)) * 1.44269502162933349609375f) / (_9796 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(dot(float3(_1554, _1555, _1556), float3(_8545, _8546, _8547)), 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f)) * _9857;
                                                        float _9860 = (_9851 * (exp2((((_9818 * _9818) * (-0.5f)) / (_9778 * _9778)) * 1.44269502162933349609375f) / (_9778 * 2.5066282749176025390625f))) * exp2(_9851 * ((_9763 * 24.5258159637451171875f) + (-24.208423614501953125f)));
                                                        float _9861 = _9857 * ((_9846 * _9846) * (_9844 + 0.0465205647051334381103515625f));
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder = ((_9861 * exp2(((_972 * (-3.2000000476837158203125f)) / _9773) * 1.44269502162933349609375f)) * _9860) + _9858;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_1 = ((_9861 * exp2(((_970 * (-3.2000000476837158203125f)) / _9773) * 1.44269502162933349609375f)) * _9860) + _9858;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_2 = 0.0f;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_3 = 0.0f;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_4 = 0.0f;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_5 = 0.0f;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_6 = 0.0f;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_7 = ((_9861 * exp2(((_974 * (-3.2000000476837158203125f)) / _9773) * 1.44269502162933349609375f)) * _9860) + _9858;
                                                        frontier_phi_411_402_ladder_408_ladder_417_ladder_8 = 0.0f;
                                                    }
                                                    frontier_phi_411_402_ladder_408_ladder = frontier_phi_411_402_ladder_408_ladder_417_ladder;
                                                    frontier_phi_411_402_ladder_408_ladder_1 = frontier_phi_411_402_ladder_408_ladder_417_ladder_1;
                                                    frontier_phi_411_402_ladder_408_ladder_2 = frontier_phi_411_402_ladder_408_ladder_417_ladder_2;
                                                    frontier_phi_411_402_ladder_408_ladder_3 = frontier_phi_411_402_ladder_408_ladder_417_ladder_3;
                                                    frontier_phi_411_402_ladder_408_ladder_4 = frontier_phi_411_402_ladder_408_ladder_417_ladder_4;
                                                    frontier_phi_411_402_ladder_408_ladder_5 = frontier_phi_411_402_ladder_408_ladder_417_ladder_5;
                                                    frontier_phi_411_402_ladder_408_ladder_6 = frontier_phi_411_402_ladder_408_ladder_417_ladder_6;
                                                    frontier_phi_411_402_ladder_408_ladder_7 = frontier_phi_411_402_ladder_408_ladder_417_ladder_7;
                                                    frontier_phi_411_402_ladder_408_ladder_8 = frontier_phi_411_402_ladder_408_ladder_417_ladder_8;
                                                }
                                                else
                                                {
                                                    float _9565 = clamp(dot(float3(_822, _827, _832), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                    float _9338;
                                                    float _9344;
                                                    float _9350;
                                                    if (_9565 > 0.0f)
                                                    {
                                                        float _9873 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                        float _9874 = _8545 + _1554;
                                                        float _9875 = _8546 + _1555;
                                                        float _9876 = _8547 + _1556;
                                                        float _9880 = rsqrt(dot(float3(_9874, _9875, _9876), float3(_9874, _9875, _9876)));
                                                        float _9881 = _9880 * _9874;
                                                        float _9882 = _9880 * _9875;
                                                        float _9883 = _9880 * _9876;
                                                        float _9887 = clamp(dot(float3(_837, _840, _843), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                        float _9891 = clamp(dot(float3(_837, _840, _843), float3(_9881, _9882, _9883)), 0.0f, 1.0f);
                                                        float _10333;
                                                        float _10334;
                                                        float _10335;
                                                        if (_9887 > 0.0f)
                                                        {
                                                            float _10297 = _9873 * _9873;
                                                            float _10298 = _10297 * _10297;
                                                            float _10302 = (((_9891 * _10298) - _9891) * _9891) + 1.0f;
                                                            float _10306 = _10297 * 0.5f;
                                                            float _10307 = 1.0f - _10306;
                                                            float _10314 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_9881, _9882, _9883)), 0.0f, 1.0f);
                                                            float _10315 = _10314 * _10314;
                                                            float _10317 = (_10315 * _10315) * _10314;
                                                            float _10329 = min((0.25f / (((_9887 * _10307) + _10306) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _10307) + _10306))) * (_10298 / ((_10302 * _10302) * 3.1415927410125732421875f)), _1534) * _4408;
                                                            _10333 = _10329 * ((_10317 * (1.0f - _865)) + _865);
                                                            _10334 = _10329 * ((_10317 * (1.0f - _870)) + _870);
                                                            _10335 = _10329 * ((_10317 * (1.0f - _873)) + _873);
                                                        }
                                                        else
                                                        {
                                                            _10333 = 0.0f;
                                                            _10334 = 0.0f;
                                                            _10335 = 0.0f;
                                                        }
                                                        float _10336 = min(_10333, 100000.0f);
                                                        float _10337 = min(_10334, 100000.0f);
                                                        float _10338 = min(_10335, 100000.0f);
                                                        float _10956;
                                                        float _10958;
                                                        float _10960;
                                                        if (_4412)
                                                        {
                                                            _10956 = _10336;
                                                            _10958 = _10337;
                                                            _10960 = _10338;
                                                        }
                                                        else
                                                        {
                                                            float _10962 = 1.0f - _9565;
                                                            float _10963 = _10962 * _10962;
                                                            float _10965 = 1.0f - (_10963 * _10963);
                                                            _10956 = _10336 * _10965;
                                                            _10958 = _10337 * _10965;
                                                            _10960 = _10338 * _10965;
                                                        }
                                                        _9338 = _10956 * _9565;
                                                        _9344 = _10958 * _9565;
                                                        _9350 = _10960 * _9565;
                                                    }
                                                    else
                                                    {
                                                        _9338 = 0.0f;
                                                        _9344 = 0.0f;
                                                        _9350 = 0.0f;
                                                    }
                                                    float _9899 = clamp(_9565, 0.0f, 1.0f);
                                                    float _10376;
                                                    if (_57_m0[76u].w > 0.5f)
                                                    {
                                                        float _10339 = _8545 + _1554;
                                                        float _10340 = _8546 + _1555;
                                                        float _10341 = _8547 + _1556;
                                                        float _10345 = rsqrt(dot(float3(_10339, _10340, _10341), float3(_10339, _10340, _10341)));
                                                        float _10352 = clamp(dot(float3(_8545, _8546, _8547), float3(_10345 * _10339, _10345 * _10340, _10345 * _10341)), 0.0f, 1.0f);
                                                        float _10362 = ((((_10352 * _10352) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                        _10376 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _10362) + 1.0f) * ((_10362 * exp2(log2(1.0f - _9565) * 5.0f)) + 1.0f);
                                                    }
                                                    else
                                                    {
                                                        _10376 = 1.0f;
                                                    }
                                                    float _10377 = 1.0f - _960;
                                                    frontier_phi_411_402_ladder_408_ladder = _9344;
                                                    frontier_phi_411_402_ladder_408_ladder_1 = _9338;
                                                    frontier_phi_411_402_ladder_408_ladder_2 = ((((_860 * 0.3183098733425140380859375f) * (1.0f - _873)) * _10377) * _9899) * _10376;
                                                    frontier_phi_411_402_ladder_408_ladder_3 = ((((_850 * 0.3183098733425140380859375f) * (1.0f - _865)) * _10377) * _9899) * _10376;
                                                    frontier_phi_411_402_ladder_408_ladder_4 = ((((_855 * 0.3183098733425140380859375f) * (1.0f - _870)) * _10377) * _9899) * _10376;
                                                    frontier_phi_411_402_ladder_408_ladder_5 = 0.0f;
                                                    frontier_phi_411_402_ladder_408_ladder_6 = 0.0f;
                                                    frontier_phi_411_402_ladder_408_ladder_7 = _9350;
                                                    frontier_phi_411_402_ladder_408_ladder_8 = 0.0f;
                                                }
                                                frontier_phi_411_402_ladder = frontier_phi_411_402_ladder_408_ladder;
                                                frontier_phi_411_402_ladder_1 = frontier_phi_411_402_ladder_408_ladder_1;
                                                frontier_phi_411_402_ladder_2 = frontier_phi_411_402_ladder_408_ladder_2;
                                                frontier_phi_411_402_ladder_3 = frontier_phi_411_402_ladder_408_ladder_3;
                                                frontier_phi_411_402_ladder_4 = frontier_phi_411_402_ladder_408_ladder_4;
                                                frontier_phi_411_402_ladder_5 = frontier_phi_411_402_ladder_408_ladder_5;
                                                frontier_phi_411_402_ladder_6 = frontier_phi_411_402_ladder_408_ladder_6;
                                                frontier_phi_411_402_ladder_7 = frontier_phi_411_402_ladder_408_ladder_7;
                                                frontier_phi_411_402_ladder_8 = frontier_phi_411_402_ladder_408_ladder_8;
                                            }
                                            else
                                            {
                                                float _9199 = 1.0f - _865;
                                                float _9200 = 1.0f - _870;
                                                float _9201 = 1.0f - _873;
                                                float _9205 = clamp(dot(float3(_822, _827, _832), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                float _9206 = clamp(_9205, 0.0f, 1.0f);
                                                float _9604;
                                                if (_57_m0[76u].w > 0.5f)
                                                {
                                                    float _9567 = _8545 + _1554;
                                                    float _9568 = _8546 + _1555;
                                                    float _9569 = _8547 + _1556;
                                                    float _9573 = rsqrt(dot(float3(_9567, _9568, _9569), float3(_9567, _9568, _9569)));
                                                    float _9580 = clamp(dot(float3(_8545, _8546, _8547), float3(_9573 * _9567, _9573 * _9568, _9573 * _9569)), 0.0f, 1.0f);
                                                    float _9590 = ((((_9580 * _9580) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                    _9604 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _9590) + 1.0f) * ((_9590 * exp2(log2(1.0f - _9205) * 5.0f)) + 1.0f);
                                                }
                                                else
                                                {
                                                    _9604 = 1.0f;
                                                }
                                                bool _9614 = _9205 > 0.0f;
                                                float _9929;
                                                float _9931;
                                                float _9933;
                                                if (_9614)
                                                {
                                                    float _9909 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                    float _9910 = _8545 + _1554;
                                                    float _9911 = _8546 + _1555;
                                                    float _9912 = _8547 + _1556;
                                                    float _9916 = rsqrt(dot(float3(_9910, _9911, _9912), float3(_9910, _9911, _9912)));
                                                    float _9917 = _9916 * _9910;
                                                    float _9918 = _9916 * _9911;
                                                    float _9919 = _9916 * _9912;
                                                    float _9923 = clamp(dot(float3(_837, _840, _843), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                    float _9927 = clamp(dot(float3(_837, _840, _843), float3(_9917, _9918, _9919)), 0.0f, 1.0f);
                                                    float _10432;
                                                    float _10433;
                                                    float _10434;
                                                    if (_9923 > 0.0f)
                                                    {
                                                        float _10399 = _9909 * _9909;
                                                        float _10400 = _10399 * _10399;
                                                        float _10404 = (((_9927 * _10400) - _9927) * _9927) + 1.0f;
                                                        float _10408 = _10399 * 0.5f;
                                                        float _10409 = 1.0f - _10408;
                                                        float _10416 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_9917, _9918, _9919)), 0.0f, 1.0f);
                                                        float _10417 = _10416 * _10416;
                                                        float _10419 = (_10417 * _10417) * _10416;
                                                        float _10428 = min((0.25f / (((_9923 * _10409) + _10408) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _10409) + _10408))) * (_10400 / ((_10404 * _10404) * 3.1415927410125732421875f)), _1534) * _4408;
                                                        _10432 = _10428 * ((_10419 * _9199) + _865);
                                                        _10433 = _10428 * ((_10419 * _9200) + _870);
                                                        _10434 = _10428 * ((_10419 * _9201) + _873);
                                                    }
                                                    else
                                                    {
                                                        _10432 = 0.0f;
                                                        _10433 = 0.0f;
                                                        _10434 = 0.0f;
                                                    }
                                                    float _10435 = min(_10432, 100000.0f);
                                                    float _10436 = min(_10433, 100000.0f);
                                                    float _10437 = min(_10434, 100000.0f);
                                                    float _10966;
                                                    float _10968;
                                                    float _10970;
                                                    if (_4412)
                                                    {
                                                        _10966 = _10435;
                                                        _10968 = _10436;
                                                        _10970 = _10437;
                                                    }
                                                    else
                                                    {
                                                        float _10972 = 1.0f - _9205;
                                                        float _10973 = _10972 * _10972;
                                                        float _10975 = 1.0f - (_10973 * _10973);
                                                        _10966 = _10435 * _10975;
                                                        _10968 = _10436 * _10975;
                                                        _10970 = _10437 * _10975;
                                                    }
                                                    _9929 = _10966 * _9205;
                                                    _9931 = _10968 * _9205;
                                                    _9933 = _10970 * _9205;
                                                }
                                                else
                                                {
                                                    _9929 = 0.0f;
                                                    _9931 = 0.0f;
                                                    _9933 = 0.0f;
                                                }
                                                float _10463;
                                                float _10465;
                                                float _10467;
                                                if (_9614)
                                                {
                                                    float _10443 = max(exp2(log2(clamp(1.0f - _915, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                    float _10444 = _8545 + _1554;
                                                    float _10445 = _8546 + _1555;
                                                    float _10446 = _8547 + _1556;
                                                    float _10450 = rsqrt(dot(float3(_10444, _10445, _10446), float3(_10444, _10445, _10446)));
                                                    float _10451 = _10450 * _10444;
                                                    float _10452 = _10450 * _10445;
                                                    float _10453 = _10450 * _10446;
                                                    float _10457 = clamp(dot(float3(_837, _840, _843), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                    float _10461 = clamp(dot(float3(_837, _840, _843), float3(_10451, _10452, _10453)), 0.0f, 1.0f);
                                                    float _11018;
                                                    float _11019;
                                                    float _11020;
                                                    if (_10457 > 0.0f)
                                                    {
                                                        float _10985 = _10443 * _10443;
                                                        float _10986 = _10985 * _10985;
                                                        float _10990 = (((_10461 * _10986) - _10461) * _10461) + 1.0f;
                                                        float _10994 = _10985 * 0.5f;
                                                        float _10995 = 1.0f - _10994;
                                                        float _11002 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_10451, _10452, _10453)), 0.0f, 1.0f);
                                                        float _11003 = _11002 * _11002;
                                                        float _11005 = (_11003 * _11003) * _11002;
                                                        float _11014 = min((0.25f / (((_10457 * _10995) + _10994) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _10995) + _10994))) * (_10986 / ((_10990 * _10990) * 3.1415927410125732421875f)), _1534) * _4408;
                                                        _11018 = _11014 * ((_11005 * _9199) + _865);
                                                        _11019 = _11014 * ((_11005 * _9200) + _870);
                                                        _11020 = _11014 * ((_11005 * _9201) + _873);
                                                    }
                                                    else
                                                    {
                                                        _11018 = 0.0f;
                                                        _11019 = 0.0f;
                                                        _11020 = 0.0f;
                                                    }
                                                    float _11021 = min(_11018, 100000.0f);
                                                    float _11022 = min(_11019, 100000.0f);
                                                    float _11023 = min(_11020, 100000.0f);
                                                    float _11807;
                                                    float _11809;
                                                    float _11811;
                                                    if (_4412)
                                                    {
                                                        _11807 = _11021;
                                                        _11809 = _11022;
                                                        _11811 = _11023;
                                                    }
                                                    else
                                                    {
                                                        float _11813 = 1.0f - _9205;
                                                        float _11814 = _11813 * _11813;
                                                        float _11816 = 1.0f - (_11814 * _11814);
                                                        _11807 = _11021 * _11816;
                                                        _11809 = _11022 * _11816;
                                                        _11811 = _11023 * _11816;
                                                    }
                                                    _10463 = _11807 * _9205;
                                                    _10465 = _11809 * _9205;
                                                    _10467 = _11811 * _9205;
                                                }
                                                else
                                                {
                                                    _10463 = 0.0f;
                                                    _10465 = 0.0f;
                                                    _10467 = 0.0f;
                                                }
                                                frontier_phi_411_402_ladder = ((_10465 - _9931) * _917) + _9931;
                                                frontier_phi_411_402_ladder_1 = ((_10463 - _9929) * _917) + _9929;
                                                frontier_phi_411_402_ladder_2 = (((_860 * 0.3183098733425140380859375f) * _9201) * _9206) * _9604;
                                                frontier_phi_411_402_ladder_3 = (((_850 * 0.3183098733425140380859375f) * _9199) * _9206) * _9604;
                                                frontier_phi_411_402_ladder_4 = (((_855 * 0.3183098733425140380859375f) * _9200) * _9206) * _9604;
                                                frontier_phi_411_402_ladder_5 = 0.0f;
                                                frontier_phi_411_402_ladder_6 = 0.0f;
                                                frontier_phi_411_402_ladder_7 = ((_10467 - _9933) * _917) + _9933;
                                                frontier_phi_411_402_ladder_8 = 0.0f;
                                            }
                                            _9321 = frontier_phi_411_402_ladder_3;
                                            _9326 = frontier_phi_411_402_ladder_4;
                                            _9331 = frontier_phi_411_402_ladder_2;
                                            _9336 = frontier_phi_411_402_ladder_1;
                                            _9342 = frontier_phi_411_402_ladder;
                                            _9348 = frontier_phi_411_402_ladder_7;
                                            _9354 = frontier_phi_411_402_ladder_5;
                                            _9356 = frontier_phi_411_402_ladder_6;
                                            _9358 = frontier_phi_411_402_ladder_8;
                                        }
                                        else
                                        {
                                            float _9151 = clamp(dot(float3(_822, _827, _832), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                            float frontier_phi_411_403_ladder;
                                            float frontier_phi_411_403_ladder_1;
                                            float frontier_phi_411_403_ladder_2;
                                            float frontier_phi_411_403_ladder_3;
                                            float frontier_phi_411_403_ladder_4;
                                            float frontier_phi_411_403_ladder_5;
                                            float frontier_phi_411_403_ladder_6;
                                            float frontier_phi_411_403_ladder_7;
                                            float frontier_phi_411_403_ladder_8;
                                            if (_9151 > 0.0f)
                                            {
                                                float _9212 = _8545 + _1554;
                                                float _9213 = _8546 + _1555;
                                                float _9214 = _8547 + _1556;
                                                float _9218 = rsqrt(dot(float3(_9212, _9213, _9214), float3(_9212, _9213, _9214)));
                                                float _9219 = _9218 * _9212;
                                                float _9220 = _9218 * _9213;
                                                float _9221 = _9218 * _9214;
                                                float _9226 = max(clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f), 0.001000000047497451305389404296875f);
                                                float _9230 = clamp(dot(float3(_837, _840, _843), float3(_9219, _9220, _9221)), 0.0f, 1.0f);
                                                float _9235 = max(1.0f - _876, 0.04500000178813934326171875f);
                                                float _9236 = _9235 * _9235;
                                                float _9237 = 1.0f / _9236;
                                                float _9251 = (((_9237 + 2.0f) * 0.15915493667125701904296875f) * exp2((_9237 * 0.5f) * log2(1.0f - (_9230 * _9230)))) * (0.25f / ((_9226 + _9151) - (_9226 * _9151)));
                                                float _9255 = _9236 * _9236;
                                                float _9259 = (((_9255 * _9230) - _9230) * _9230) + 1.0f;
                                                float _9263 = _9236 * 0.5f;
                                                float _9264 = 1.0f - _9263;
                                                float _9271 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_9219, _9220, _9221)), 0.0f, 1.0f);
                                                float _9272 = _9271 * _9271;
                                                float _9274 = (_9272 * _9272) * _9271;
                                                float _9286 = min((0.25f / (((_9264 * _9226) + _9263) * ((_9264 * _9151) + _9263))) * (_9255 / ((_9259 * _9259) * 3.1415927410125732421875f)), _1534) * _4408;
                                                float _9287 = _9286 * ((_9274 * (1.0f - _865)) + _865);
                                                float _9288 = _9286 * ((_9274 * (1.0f - _870)) + _870);
                                                float _9289 = _9286 * ((_9274 * (1.0f - _873)) + _873);
                                                float _9293 = (1.0f - _923) * _850;
                                                float _9294 = (1.0f - _925) * _855;
                                                float _9295 = (1.0f - _927) * _860;
                                                float _9317 = _9151 * 0.3183098733425140380859375f;
                                                frontier_phi_411_403_ladder = ((((_9251 * _925) - _9288) * _929) + _9288) * _9151;
                                                frontier_phi_411_403_ladder_1 = ((((_9251 * _923) - _9287) * _929) + _9287) * _9151;
                                                frontier_phi_411_403_ladder_2 = _9317 * (((_860 - _9295) * _929) + _9295);
                                                frontier_phi_411_403_ladder_3 = _9317 * (((_850 - _9293) * _929) + _9293);
                                                frontier_phi_411_403_ladder_4 = _9317 * (((_855 - _9294) * _929) + _9294);
                                                frontier_phi_411_403_ladder_5 = 0.0f;
                                                frontier_phi_411_403_ladder_6 = 0.0f;
                                                frontier_phi_411_403_ladder_7 = ((((_9251 * _927) - _9289) * _929) + _9289) * _9151;
                                                frontier_phi_411_403_ladder_8 = 0.0f;
                                            }
                                            else
                                            {
                                                frontier_phi_411_403_ladder = 0.0f;
                                                frontier_phi_411_403_ladder_1 = 0.0f;
                                                frontier_phi_411_403_ladder_2 = 0.0f;
                                                frontier_phi_411_403_ladder_3 = 0.0f;
                                                frontier_phi_411_403_ladder_4 = 0.0f;
                                                frontier_phi_411_403_ladder_5 = 0.0f;
                                                frontier_phi_411_403_ladder_6 = 0.0f;
                                                frontier_phi_411_403_ladder_7 = 0.0f;
                                                frontier_phi_411_403_ladder_8 = 0.0f;
                                            }
                                            _9321 = frontier_phi_411_403_ladder_3;
                                            _9326 = frontier_phi_411_403_ladder_4;
                                            _9331 = frontier_phi_411_403_ladder_2;
                                            _9336 = frontier_phi_411_403_ladder_1;
                                            _9342 = frontier_phi_411_403_ladder;
                                            _9348 = frontier_phi_411_403_ladder_7;
                                            _9354 = frontier_phi_411_403_ladder_5;
                                            _9356 = frontier_phi_411_403_ladder_6;
                                            _9358 = frontier_phi_411_403_ladder_8;
                                        }
                                        float _9129 = _9321 * _8809;
                                        float _9131 = _9326 * _8810;
                                        float _9133 = _9331 * _8811;
                                        float _9135 = (_9336 + _9354) * _8809;
                                        float _9137 = (_9342 + _9356) * _8810;
                                        float _9139 = (_9348 + _9358) * _8811;
                                        float frontier_phi_401_411_ladder;
                                        float frontier_phi_401_411_ladder_1;
                                        float frontier_phi_401_411_ladder_2;
                                        float frontier_phi_401_411_ladder_3;
                                        float frontier_phi_401_411_ladder_4;
                                        float frontier_phi_401_411_ladder_5;
                                        float frontier_phi_401_411_ladder_6;
                                        float frontier_phi_401_411_ladder_7;
                                        float frontier_phi_401_411_ladder_8;
                                        if (_913 == 0u)
                                        {
                                            frontier_phi_401_411_ladder = 0.0f;
                                            frontier_phi_401_411_ladder_1 = _9129;
                                            frontier_phi_401_411_ladder_2 = _9131;
                                            frontier_phi_401_411_ladder_3 = _9133;
                                            frontier_phi_401_411_ladder_4 = _9135;
                                            frontier_phi_401_411_ladder_5 = _9137;
                                            frontier_phi_401_411_ladder_6 = _9139;
                                            frontier_phi_401_411_ladder_7 = 0.0f;
                                            frontier_phi_401_411_ladder_8 = 0.0f;
                                        }
                                        else
                                        {
                                            float _10549;
                                            float _10553;
                                            float _10557;
                                            if ((_989 & 8388608u) == 0u)
                                            {
                                                float frontier_phi_457_435_ladder;
                                                float frontier_phi_457_435_ladder_1;
                                                float frontier_phi_457_435_ladder_2;
                                                if ((_989 & 67108864u) == 0u)
                                                {
                                                    float frontier_phi_457_435_ladder_455_ladder;
                                                    float frontier_phi_457_435_ladder_455_ladder_1;
                                                    float frontier_phi_457_435_ladder_455_ladder_2;
                                                    if ((_989 & 50331648u) == 0u)
                                                    {
                                                        float frontier_phi_457_435_ladder_455_ladder_481_ladder;
                                                        float frontier_phi_457_435_ladder_455_ladder_481_ladder_1;
                                                        float frontier_phi_457_435_ladder_455_ladder_481_ladder_2;
                                                        if ((_989 & 536870912u) == 0u)
                                                        {
                                                            float _11827 = clamp((dot(float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), float3(_8545, _8546, _8547)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_906 * 0.3183098733425140380859375f);
                                                            frontier_phi_457_435_ladder_455_ladder_481_ladder = _11827 * _903;
                                                            frontier_phi_457_435_ladder_455_ladder_481_ladder_1 = _11827 * _900;
                                                            frontier_phi_457_435_ladder_455_ladder_481_ladder_2 = _11827 * _897;
                                                        }
                                                        else
                                                        {
                                                            float _11838 = clamp((dot(float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), float3(_8545, _8546, _8547)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_906 * 0.3183098733425140380859375f);
                                                            frontier_phi_457_435_ladder_455_ladder_481_ladder = _11838 * _903;
                                                            frontier_phi_457_435_ladder_455_ladder_481_ladder_1 = _11838 * _900;
                                                            frontier_phi_457_435_ladder_455_ladder_481_ladder_2 = _11838 * _897;
                                                        }
                                                        frontier_phi_457_435_ladder_455_ladder = frontier_phi_457_435_ladder_455_ladder_481_ladder;
                                                        frontier_phi_457_435_ladder_455_ladder_1 = frontier_phi_457_435_ladder_455_ladder_481_ladder_1;
                                                        frontier_phi_457_435_ladder_455_ladder_2 = frontier_phi_457_435_ladder_455_ladder_481_ladder_2;
                                                    }
                                                    else
                                                    {
                                                        float _11028 = 1.0f - _876;
                                                        float _11029 = (-0.0f) - _784;
                                                        float _11030 = (-0.0f) - _778;
                                                        float _11031 = (-0.0f) - _772;
                                                        float _11034 = 1.0f - clamp(_976 * 66.6666717529296875f, 0.0f, 1.0f);
                                                        float _11038 = dot(float3(_11029, _11030, _11031), float3(_8545, _8546, _8547));
                                                        float _11044 = dot(float3(_11029, _11030, _11031), float3(_1554, _1555, _1556));
                                                        float _11050 = _8545 - (_11038 * _11029);
                                                        float _11051 = _8546 - (_11038 * _11030);
                                                        float _11052 = _8547 - (_11038 * _11031);
                                                        float _11056 = _1554 - (_11044 * _11029);
                                                        float _11057 = _1555 - (_11044 * _11030);
                                                        float _11058 = _1556 - (_11044 * _11031);
                                                        float _11071 = rsqrt((dot(float3(_11056, _11057, _11058), float3(_11056, _11057, _11058)) * dot(float3(_11050, _11051, _11052), float3(_11050, _11051, _11052))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_11050, _11051, _11052), float3(_11056, _11057, _11058));
                                                        float _11073 = (_11071 * 0.5f) + 0.5f;
                                                        float _11076 = asin(_11044);
                                                        float _11077 = asin(_11038);
                                                        float _11081 = cos(abs(_11076 - _11077) * 0.5f);
                                                        float _11085 = 1.0f / ((1.190000057220458984375f / _11081) + (_11081 * 0.36000001430511474609375f));
                                                        float _11086 = _11044 * 0.645161330699920654296875f;
                                                        float _11089 = sqrt(1.0f - (_11086 * _11086));
                                                        float _11091 = max(_11028 * 0.5f, 0.00999999977648258209228515625f);
                                                        float _11094 = _11044 + (_11038 - (_986 * 0.5f));
                                                        float _11102 = exp2((((_11094 * _11094) * (-0.5f)) / (_11091 * _11091)) * 1.44269502162933349609375f) / (_11091 * 2.5066282749176025390625f);
                                                        float _11111 = clamp((-0.0f) - dot(float3(_1554, _1555, _1556), float3(_8545, _8546, _8547)), 0.0f, 1.0f);
                                                        float _11112 = _11111 * _11111;
                                                        float _11115 = (_11112 * _11112) * (_11111 * _11034);
                                                        float _11122 = (cos(asin((_11085 * sqrt(clamp(_11073, 0.0f, 1.0f))) * ((_11085 * (0.60000002384185791015625f - (_11071 * 0.800000011920928955078125f))) + 1.0f)) * 2.0f) + 1.0f) * (-2.8853900432586669921875f);
                                                        float _11126 = exp2(_11122 * (_970 / _11089));
                                                        float _11127 = exp2(_11122 * (_972 / _11089));
                                                        float _11128 = exp2(_11122 * (_974 / _11089));
                                                        float _11143 = 0.95347940921783447265625f - (exp2(log2(1.0f - _11081) * 5.0f) * 0.95347940921783447265625f);
                                                        float _11144 = _11143 * _11143;
                                                        float _11146 = (_11081 * 0.5f) + 0.5f;
                                                        float _11158 = abs(sqrt(1.0f - (_11038 * _11038))) * _15[(_968 + 513u) + 0u].SampleLevel(_65, float2(_11146, _11073), 0.0f).x;
                                                        float _11159 = _11158 * (_11102 * _11034);
                                                        float _11166 = _976 + 9899999600270360182784.0f;
                                                        float _11167 = _876 * 0.75f;
                                                        float _11171 = (((_978 * _978) * 9000.0f) * _978) * _11166;
                                                        float _12729;
                                                        float _12730;
                                                        float _12731;
                                                        if ((_989 & 33554432u) == 0u)
                                                        {
                                                            float _11839 = max(_11171, _11167);
                                                            float _11845 = _11077 + _11076;
                                                            float _11846 = _11845 * 0.5f;
                                                            float _11848 = _970 * 0.5f;
                                                            float _11849 = _972 * 0.5f;
                                                            float _11850 = _974 * 0.5f;
                                                            float _11851 = max(_11028, 0.0500000007450580596923828125f);
                                                            float _11853 = (cos(_11846) * 0.5f) + 0.5f;
                                                            uint _11855 = (_968 + 521u) + 0u;
                                                            float4 _11860 = _19[_11855].SampleLevel(_65, float3(_11853, _11851, _11848), 0.0f);
                                                            float _11862 = _11860.x;
                                                            float4 _11863 = _19[_11855].SampleLevel(_65, float3(_11853, _11851, _11849), 0.0f);
                                                            float _11865 = _11863.x;
                                                            float4 _11866 = _19[_11855].SampleLevel(_65, float3(_11853, _11851, _11850), 0.0f);
                                                            float _11868 = _11866.x;
                                                            float4 _11869 = _19[_11855].SampleLevel(_65, float3(_11146, _11851, _11848), 0.0f);
                                                            float4 _11873 = _19[_11855].SampleLevel(_65, float3(_11146, _11851, _11849), 0.0f);
                                                            float4 _11877 = _19[_11855].SampleLevel(_65, float3(_11146, _11851, _11850), 0.0f);
                                                            uint _11882 = (_968 + 545u) + 0u;
                                                            float _11897 = (_11865 + _11862) + _11868;
                                                            float _11904 = dot(float3(max((1.0f - _980) * _11028, 0.00999999977648258209228515625f), _11091, max(_11028 * 2.0f, 0.00999999977648258209228515625f)), float3(_11862 / _11897, _11865 / _11897, _11868 / _11897)) * _11839;
                                                            float _11915 = (_11081 * _11081) * 3.1415927410125732421875f;
                                                            float _11919 = _11904 * 0.5f;
                                                            float _11920 = _11919 + _11869.z;
                                                            float _11921 = _11919 + _11873.z;
                                                            float _11922 = _11919 + _11877.z;
                                                            float _11924 = (_11845 * (-0.25f)) * _11846;
                                                            float _11946 = ((_11869.y * 2.0f) * (exp2((_11924 / (_11920 * _11920)) * 1.44269502162933349609375f) / (_11920 * 2.5066282749176025390625f))) / _11915;
                                                            float _11947 = ((_11873.y * 2.0f) * (exp2((_11924 / (_11921 * _11921)) * 1.44269502162933349609375f) / (_11921 * 2.5066282749176025390625f))) / _11915;
                                                            float _11948 = ((_11877.y * 2.0f) * (exp2((_11924 / (_11922 * _11922)) * 1.44269502162933349609375f) / (_11922 * 2.5066282749176025390625f))) / _11915;
                                                            float _11949 = _11845 - _986;
                                                            float _11952 = max(max(_11904, _11904), _11904) + _11091;
                                                            float _11960 = exp2((((_11949 * _11949) * (-0.125f)) / (_11952 * _11952)) * 1.44269502162933349609375f) / (_11952 * 2.5066282749176025390625f);
                                                            _12729 = (((exp2(log2(_11862) * _11839) * 0.4899999797344207763671875f) * ((_11960 * _15[_11882].SampleLevel(_65, float2(_11146, _11848), 0.0f).y) + (_11946 * 2.19911479949951171875f))) + (_11946 * 0.3499999940395355224609375f)) * _982;
                                                            _12730 = (((exp2(log2(_11865) * _11839) * 0.4899999797344207763671875f) * ((_11960 * _15[_11882].SampleLevel(_65, float2(_11146, _11849), 0.0f).y) + (_11947 * 2.19911479949951171875f))) + (_11947 * 0.3499999940395355224609375f)) * _982;
                                                            _12731 = (((exp2(log2(_11868) * _11839) * 0.4899999797344207763671875f) * ((_11960 * _15[_11882].SampleLevel(_65, float2(_11146, _11850), 0.0f).y) + (_11948 * 2.19911479949951171875f))) + (_11948 * 0.3499999940395355224609375f)) * _982;
                                                        }
                                                        else
                                                        {
                                                            float _11989 = 1.0f - clamp((_11166 * 66.6666717529296875f) * max(_978, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f);
                                                            float _11990 = _11989 * _11989;
                                                            float _11992 = (_11990 * _11990) * _11989;
                                                            float _11993 = _11144 * _11102;
                                                            float _12001 = (_11167 + 1.25f) + _11171;
                                                            float _12025 = dot(float3(_1554, _1555, _1556), float3(_11029, _11030, _11031));
                                                            float _12031 = _1554 - (_12025 * _11029);
                                                            float _12032 = _1555 - (_12025 * _11030);
                                                            float _12033 = _1556 - (_12025 * _11031);
                                                            float _12037 = rsqrt(dot(float3(_12031, _12032, _12033), float3(_12031, _12032, _12033)));
                                                            float _12045 = (dot(float3(_12031 * _12037, _12032 * _12037, _12033 * _12037), float3(_8545, _8546, _8547)) + 1.0f) * 0.25f;
                                                            float _12050 = (_982 * 0.2228169143199920654296875f) * ((((1.0f - abs(_11038)) - _12045) * 0.3300000131130218505859375f) + _12045);
                                                            _12729 = ((_12050 * exp2(log2(exp2(((_970 * (-3.7999999523162841796875f)) / _11081) * 1.44269502162933349609375f)) * _12001)) + ((_11158 * _11126) * _11993)) * _11992;
                                                            _12730 = ((_12050 * exp2(log2(exp2(((_972 * (-3.7999999523162841796875f)) / _11081) * 1.44269502162933349609375f)) * _12001)) + ((_11158 * _11127) * _11993)) * _11992;
                                                            _12731 = ((_12050 * exp2(log2(exp2(((_974 * (-3.7999999523162841796875f)) / _11081) * 1.44269502162933349609375f)) * _12001)) + ((_11158 * _11128) * _11993)) * _11992;
                                                        }
                                                        frontier_phi_457_435_ladder_455_ladder = _12731 + ((_11159 * (((1.0f - _11128) * _11115) + _11128)) * _11144);
                                                        frontier_phi_457_435_ladder_455_ladder_1 = _12730 + ((_11159 * (((1.0f - _11127) * _11115) + _11127)) * _11144);
                                                        frontier_phi_457_435_ladder_455_ladder_2 = _12729 + ((_11159 * (((1.0f - _11126) * _11115) + _11126)) * _11144);
                                                    }
                                                    frontier_phi_457_435_ladder = frontier_phi_457_435_ladder_455_ladder;
                                                    frontier_phi_457_435_ladder_1 = frontier_phi_457_435_ladder_455_ladder_1;
                                                    frontier_phi_457_435_ladder_2 = frontier_phi_457_435_ladder_455_ladder_2;
                                                }
                                                else
                                                {
                                                    uint _10477 = _964 + 0u;
                                                    float _10478 = dot(float3(_8545, _8546, _8547), float3(_954, _956, _958));
                                                    float _10493 = dot(float3(_822, _827, _832), float3((_8546 * _958) - (_8547 * _956), (_8547 * _954) - (_8545 * _958), (_8545 * _956) - (_8546 * _954)));
                                                    float _10502 = float(int(uint(_10493 > 0.0f) - uint(_10493 < 0.0f))) * sqrt(1.0f - (_10478 * _10478));
                                                    float _10503 = _950 + (-0.5f);
                                                    float _10504 = _952 + (-0.5f);
                                                    float _10510 = mad(_10504, _10502, _10478 * _10503) + 0.5f;
                                                    float _10511 = mad(_10504, _10478, (-0.0f) - (_10503 * _10502)) + 0.5f;
                                                    float _10517 = (1.0f - clamp(dot(float3(_8545, _8546, _8547), float3(_822, _827, _832)), 0.0f, 1.0f)) * 5.0f;
                                                    uint _10518 = uint(int(_10517));
                                                    float4 _10525 = _41[NonUniformResourceIndex(_10477)].SampleLevel(_69, float3(_10510, _10511, float(int(_10518))), 0.0f);
                                                    float _10527 = _10525.x;
                                                    float _10536 = ((_41[NonUniformResourceIndex(_10477)].SampleLevel(_69, float3(_10510, _10511, min(float(int(_10518 + 1u)), 5.0f)), 0.0f).x - _10527) * frac(_10517)) + _10527;
                                                    frontier_phi_457_435_ladder = (((_860 * 0.3183098733425140380859375f) * _960) * _962) * _10536;
                                                    frontier_phi_457_435_ladder_1 = (((_855 * 0.3183098733425140380859375f) * _960) * _962) * _10536;
                                                    frontier_phi_457_435_ladder_2 = (((_850 * 0.3183098733425140380859375f) * _960) * _962) * _10536;
                                                }
                                                _10549 = frontier_phi_457_435_ladder_2;
                                                _10553 = frontier_phi_457_435_ladder_1;
                                                _10557 = frontier_phi_457_435_ladder;
                                            }
                                            else
                                            {
                                                float _9940 = log2(max(1.0f - _921, 1.1754943508222875079687365372222e-38f)) * (-6862156513617824209436672.0f);
                                                float _9941 = _9940 * _9940;
                                                float _9943 = exp2(_9941 * (-225.4210968017578125f));
                                                float _9948 = exp2(_9941 * (-29.8077487945556640625f));
                                                float _9956 = exp2(_9941 * (-7.714946269989013671875f));
                                                float _9962 = exp2(_9941 * (-2.5444357395172119140625f));
                                                float _9964 = _9962 * 0.007000000216066837310791015625f;
                                                float _9969 = exp2(_9941 * (-0.72497236728668212890625f));
                                                float _9985 = max(dot(float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), float3(_8545, _8546, _8547)) + 0.300000011920928955078125f, 0.0f);
                                                _10549 = (((((((_9948 * 0.100000001490116119384765625f) + (_9943 * 0.23299999535083770751953125f)) + (_9956 * 0.1180000007152557373046875f)) + (_9962 * 0.112999998033046722412109375f)) + (_9969 * 0.3580000102519989013671875f)) + (exp2(_9941 * (-0.1946956813335418701171875f)) * 0.078000001609325408935546875f)) * _850) * _9985;
                                                _10553 = ((((((_9948 * 0.3359999954700469970703125f) + (_9943 * 0.4550000131130218505859375f)) + (_9956 * 0.19799999892711639404296875f)) + _9964) + (_9969 * 0.0040000001899898052215576171875f)) * _855) * _9985;
                                                _10557 = ((((_9948 * 0.3440000116825103759765625f) + (_9943 * 0.648999989032745361328125f)) + _9964) * _860) * _9985;
                                            }
                                            frontier_phi_401_411_ladder = _10553 * _8810;
                                            frontier_phi_401_411_ladder_1 = _9129;
                                            frontier_phi_401_411_ladder_2 = _9131;
                                            frontier_phi_401_411_ladder_3 = _9133;
                                            frontier_phi_401_411_ladder_4 = _9135;
                                            frontier_phi_401_411_ladder_5 = _9137;
                                            frontier_phi_401_411_ladder_6 = _9139;
                                            frontier_phi_401_411_ladder_7 = _10549 * _8809;
                                            frontier_phi_401_411_ladder_8 = _10557 * _8811;
                                        }
                                        _9128 = frontier_phi_401_411_ladder_1;
                                        _9130 = frontier_phi_401_411_ladder_2;
                                        _9132 = frontier_phi_401_411_ladder_3;
                                        _9134 = frontier_phi_401_411_ladder_4;
                                        _9136 = frontier_phi_401_411_ladder_5;
                                        _9138 = frontier_phi_401_411_ladder_6;
                                        _9140 = frontier_phi_401_411_ladder_7;
                                        _9142 = frontier_phi_401_411_ladder;
                                        _9144 = frontier_phi_401_411_ladder_8;
                                    }
                                    frontier_phi_360_pred_359_ladder = _9140 + _8201;
                                    frontier_phi_360_pred_359_ladder_1 = _9130 + _8196;
                                    frontier_phi_360_pred_359_ladder_2 = _9132 + _8197;
                                    frontier_phi_360_pred_359_ladder_3 = _9128 + _8195;
                                    frontier_phi_360_pred_359_ladder_4 = _9134 + _8198;
                                    frontier_phi_360_pred_359_ladder_5 = _9138 + _8200;
                                    frontier_phi_360_pred_359_ladder_6 = _9142 + _8202;
                                    frontier_phi_360_pred_359_ladder_7 = _9144 + _8203;
                                    frontier_phi_360_pred_359_ladder_8 = _9136 + _8199;
                                }
                                else
                                {
                                    frontier_phi_360_pred_359_ladder = _8201;
                                    frontier_phi_360_pred_359_ladder_1 = _8196;
                                    frontier_phi_360_pred_359_ladder_2 = _8197;
                                    frontier_phi_360_pred_359_ladder_3 = _8195;
                                    frontier_phi_360_pred_359_ladder_4 = _8198;
                                    frontier_phi_360_pred_359_ladder_5 = _8200;
                                    frontier_phi_360_pred_359_ladder_6 = _8202;
                                    frontier_phi_360_pred_359_ladder_7 = _8203;
                                    frontier_phi_360_pred_359_ladder_8 = _8199;
                                }
                                frontier_phi_360_pred = frontier_phi_360_pred_359_ladder;
                                frontier_phi_360_pred_1 = frontier_phi_360_pred_359_ladder_1;
                                frontier_phi_360_pred_2 = frontier_phi_360_pred_359_ladder_2;
                                frontier_phi_360_pred_3 = frontier_phi_360_pred_359_ladder_3;
                                frontier_phi_360_pred_4 = frontier_phi_360_pred_359_ladder_4;
                                frontier_phi_360_pred_5 = frontier_phi_360_pred_359_ladder_5;
                                frontier_phi_360_pred_6 = frontier_phi_360_pred_359_ladder_6;
                                frontier_phi_360_pred_7 = frontier_phi_360_pred_359_ladder_7;
                                frontier_phi_360_pred_8 = frontier_phi_360_pred_359_ladder_8;
                            }
                            else
                            {
                                frontier_phi_360_pred = _8201;
                                frontier_phi_360_pred_1 = _8196;
                                frontier_phi_360_pred_2 = _8197;
                                frontier_phi_360_pred_3 = _8195;
                                frontier_phi_360_pred_4 = _8198;
                                frontier_phi_360_pred_5 = _8200;
                                frontier_phi_360_pred_6 = _8202;
                                frontier_phi_360_pred_7 = _8203;
                                frontier_phi_360_pred_8 = _8199;
                            }
                            _8050 = frontier_phi_360_pred;
                            _8040 = frontier_phi_360_pred_1;
                            _8042 = frontier_phi_360_pred_2;
                            _8038 = frontier_phi_360_pred_3;
                            _8044 = frontier_phi_360_pred_4;
                            _8048 = frontier_phi_360_pred_5;
                            _8052 = frontier_phi_360_pred_6;
                            _8054 = frontier_phi_360_pred_7;
                            _8046 = frontier_phi_360_pred_8;
                            if (_8056 > _7878)
                            {
                                break;
                            }
                            else
                            {
                                _8195 = _8038;
                                _8196 = _8040;
                                _8197 = _8042;
                                _8198 = _8044;
                                _8199 = _8046;
                                _8200 = _8048;
                                _8201 = _8050;
                                _8202 = _8052;
                                _8203 = _8054;
                                _8204 = _8056;
                                _8205 = _8208;
                                continue;
                            }
                        }
                        _8037 = _8038;
                        _8039 = _8040;
                        _8041 = _8042;
                        _8043 = _8044;
                        _8045 = _8046;
                        _8047 = _8048;
                        _8049 = _8050;
                        _8051 = _8052;
                        _8053 = _8054;
                        _8055 = _8056;
                        _8057 = _8208;
                    }
                    float _7353;
                    float _7356;
                    float _7359;
                    float _7362;
                    float _7364;
                    float _7366;
                    float _8178;
                    float _8180;
                    float _8182;
                    uint _8190;
                    uint _8192;
                    if (_8055 > _7879)
                    {
                        _8178 = _8037;
                        _8180 = _8039;
                        _8182 = _8041;
                        _7353 = _8043;
                        _7356 = _8045;
                        _7359 = _8047;
                        _7362 = _8049;
                        _7364 = _8051;
                        _7366 = _8053;
                        _8190 = _8055;
                        _8192 = _8057;
                    }
                    else
                    {
                        float _8179;
                        float _8181;
                        float _8183;
                        float _8184;
                        float _8185;
                        float _8186;
                        float _8187;
                        float _8188;
                        float _8189;
                        float _8287 = _8037;
                        float _8288 = _8039;
                        float _8289 = _8041;
                        float _8290 = _8043;
                        float _8291 = _8045;
                        float _8292 = _8047;
                        float _8293 = _8049;
                        float _8294 = _8051;
                        float _8295 = _8053;
                        uint _8296 = _8055;
                        uint _8297 = _8057;
                        uint _8191;
                        uint _8300;
                        uint _8315;
                        uint _8316;
                        float _8348;
                        float _8350;
                        float _8353;
                        float _8355;
                        uint _8357;
                        bool _8360;
                        float _8362;
                        float _8365;
                        float _8367;
                        float _8370;
                        float _8372;
                        float _8375;
                        float _8377;
                        float _8379;
                        uint _8380;
                        uint _8383;
                        uint _8384;
                        bool _8390;
                        for (;;)
                        {
                            _8191 = _8296 + 1u;
                            _8300 = _45.Load(_8296).x;
                            uint _8302 = _8297 * 4u;
                            uint4 _8314 = uint4(_46.Load(_8302).x, _46.Load(_8302 + 1u).x, _46.Load(_8302 + 2u).x, _46.Load(_8302 + 3u).x);
                            _8315 = _8314.x;
                            _8316 = _8314.y;
                            uint _8317 = _8314.z;
                            uint _8318 = _8314.w;
                            uint _8320 = _8297 * 4u;
                            uint4 _8332 = uint4(_47.Load(_8320).x, _47.Load(_8320 + 1u).x, _47.Load(_8320 + 2u).x, _47.Load(_8320 + 3u).x);
                            uint _8333 = _8332.x;
                            uint _8334 = _8332.y;
                            uint _8335 = _8332.z;
                            uint _8336 = _8332.w;
                            _8348 = spvUnpackHalf2x16(_8316 >> 16u).x;
                            _8350 = spvUnpackHalf2x16(_8317).x;
                            _8353 = spvUnpackHalf2x16(_8317 >> 16u).x;
                            _8355 = spvUnpackHalf2x16(_8318).x;
                            _8357 = (_8318 >> 16u) & 7u;
                            _8360 = (_8318 & 524288u) != 0u;
                            _8362 = spvUnpackHalf2x16(_8333).x;
                            _8365 = spvUnpackHalf2x16(_8333 >> 16u).x;
                            _8367 = spvUnpackHalf2x16(_8334).x;
                            _8370 = spvUnpackHalf2x16(_8334 >> 16u).x;
                            _8372 = spvUnpackHalf2x16(_8335).x;
                            _8375 = spvUnpackHalf2x16(_8335 >> 16u).x;
                            _8377 = spvUnpackHalf2x16(_8336).x;
                            _8379 = spvUnpackHalf2x16(uint3(_8339, _8340, _48.Load((_8297 * 4u) + 2u).x).z).x;
                            _8380 = _8336 & 8323072u;
                            _8383 = (_8336 >> 23u) & 31u;
                            _8384 = _8336 >> 28u;
                            _8390 = (_8318 < 3221225472u) && (((_499 & 255u) & (_8318 >> 22u)) != 0u);
                            float frontier_phi_365_pred;
                            float frontier_phi_365_pred_1;
                            float frontier_phi_365_pred_2;
                            float frontier_phi_365_pred_3;
                            float frontier_phi_365_pred_4;
                            float frontier_phi_365_pred_5;
                            float frontier_phi_365_pred_6;
                            float frontier_phi_365_pred_7;
                            float frontier_phi_365_pred_8;
                            if (_8390)
                            {
                                float _8520 = spvUnpackHalf2x16(_8315).x - _7864;
                                float _8521 = spvUnpackHalf2x16(_8315 >> 16u).x - _7865;
                                float _8522 = spvUnpackHalf2x16(_8316).x - _7866;
                                float _8528 = sqrt(((_8521 * _8521) + (_8522 * _8522)) + (_8520 * _8520));
                                float _8529 = _8528 * _8348;
                                float frontier_phi_365_pred_364_ladder;
                                float frontier_phi_365_pred_364_ladder_1;
                                float frontier_phi_365_pred_364_ladder_2;
                                float frontier_phi_365_pred_364_ladder_3;
                                float frontier_phi_365_pred_364_ladder_4;
                                float frontier_phi_365_pred_364_ladder_5;
                                float frontier_phi_365_pred_364_ladder_6;
                                float frontier_phi_365_pred_364_ladder_7;
                                float frontier_phi_365_pred_364_ladder_8;
                                if (_8529 < 1.0f)
                                {
                                    float _8634 = rsqrt(dot(float3(_8520, _8521, _8522), float3(_8520, _8521, _8522)));
                                    float _8635 = _8634 * _8520;
                                    float _8636 = _8634 * _8521;
                                    float _8637 = _8634 * _8522;
                                    float _8638 = _8528 * _8528;
                                    float _8640 = (_8348 * _8348) * _8638;
                                    float _8643 = clamp(1.0f - (_8640 * _8640), 0.0f, 1.0f);
                                    float _8935;
                                    if (_8357 == 0u)
                                    {
                                        _8935 = (_8643 * _8643) * (1.0f / (max(_8638, 9.9999997473787516355514526367188e-05f) + ((_8379 * _8379) * 0.5f)));
                                    }
                                    else
                                    {
                                        _8935 = max((1.0f / dot(float3(1.0f, _8529, _8529 * _8529), float3(_62_m0[_8357 + 60u].xyz))) * (1.0f - _8529), 0.0f);
                                    }
                                    float _8936 = (-0.0f) - _8362;
                                    float _8960 = clamp((clamp(dot(float3(((_8370 * _8365) - (_8367 * _8936)) * 2.0f, ((_8367 * _8365) - (_8370 * _8362)) * 2.0f, (((_8362 * _8936) - (_8365 * _8365)) * 2.0f) + 1.0f), float3((-0.0f) - _8635, (-0.0f) - _8636, (-0.0f) - _8637)), 0.0f, 1.0f) - _8375) / (_8372 - _8375), 0.0f, 1.0f);
                                    float _9081;
                                    float _9083;
                                    float _9085;
                                    if ((_8383 | _8380) == 0u)
                                    {
                                        _9081 = _8350;
                                        _9083 = _8353;
                                        _9085 = _8355;
                                    }
                                    else
                                    {
                                        float _9103 = (-0.0f) - _8365;
                                        float _9104 = (-0.0f) - _8367;
                                        float _9117 = ((_8522 * _9103) - (_8521 * _9104)) + (_8520 * _8370);
                                        float _9118 = ((_8520 * _9104) - (_8522 * _8936)) + (_8521 * _8370);
                                        float _9119 = ((_8521 * _8936) - (_8520 * _9103)) + (_8522 * _8370);
                                        float _9126 = (1.0f / ((((_9118 * _8936) - (_9117 * _9103)) * 2.0f) + _8522)) * _8377;
                                        float frontier_phi_399_400_ladder;
                                        float frontier_phi_399_400_ladder_1;
                                        float frontier_phi_399_400_ladder_2;
                                        if (_8383 == 0u)
                                        {
                                            frontier_phi_399_400_ladder = _8355;
                                            frontier_phi_399_400_ladder_1 = _8353;
                                            frontier_phi_399_400_ladder_2 = _8350;
                                        }
                                        else
                                        {
                                            uint _9176 = _8384 + 72u;
                                            float4 _9192 = _50.SampleLevel(_67, float3((_62_m0[_9176].x * ((_9126 * ((((_9119 * _9103) - (_9118 * _9104)) * 2.0f) + _8520)) + 0.5f)) + _62_m0[_9176].z, (_62_m0[_9176].y * (0.5f - (_9126 * ((((_9117 * _9104) - (_9119 * _8936)) * 2.0f) + _8521)))) + _62_m0[_9176].w, float(_8383 + 4294967295u)), 0.0f);
                                            frontier_phi_399_400_ladder = _9192.z * _8355;
                                            frontier_phi_399_400_ladder_1 = _9192.y * _8353;
                                            frontier_phi_399_400_ladder_2 = _9192.x * _8350;
                                        }
                                        _9081 = frontier_phi_399_400_ladder_2;
                                        _9083 = frontier_phi_399_400_ladder_1;
                                        _9085 = frontier_phi_399_400_ladder;
                                    }
                                    bool _9087 = _8348 < 0.02857142873108386993408203125f;
                                    float _9097 = (((_8960 * _8960) * _8935) * (3.0f - (_8960 * 2.0f))) * max(float(_8360) * 16.0f, 1.0f);
                                    float _9098 = _9097 * _9081;
                                    float _9099 = _9097 * _9083;
                                    float _9100 = _9097 * _9085;
                                    float _9511;
                                    float _9516;
                                    float _9521;
                                    float _9526;
                                    float _9532;
                                    float _9538;
                                    float _9544;
                                    float _9546;
                                    float _9548;
                                    if ((_989 & 4194304u) == 0u)
                                    {
                                        float frontier_phi_416_405_ladder;
                                        float frontier_phi_416_405_ladder_1;
                                        float frontier_phi_416_405_ladder_2;
                                        float frontier_phi_416_405_ladder_3;
                                        float frontier_phi_416_405_ladder_4;
                                        float frontier_phi_416_405_ladder_5;
                                        float frontier_phi_416_405_ladder_6;
                                        float frontier_phi_416_405_ladder_7;
                                        float frontier_phi_416_405_ladder_8;
                                        if ((_989 & 8388608u) == 0u)
                                        {
                                            float frontier_phi_416_405_ladder_413_ladder;
                                            float frontier_phi_416_405_ladder_413_ladder_1;
                                            float frontier_phi_416_405_ladder_413_ladder_2;
                                            float frontier_phi_416_405_ladder_413_ladder_3;
                                            float frontier_phi_416_405_ladder_413_ladder_4;
                                            float frontier_phi_416_405_ladder_413_ladder_5;
                                            float frontier_phi_416_405_ladder_413_ladder_6;
                                            float frontier_phi_416_405_ladder_413_ladder_7;
                                            float frontier_phi_416_405_ladder_413_ladder_8;
                                            if ((_989 & 67108864u) == 0u)
                                            {
                                                float frontier_phi_416_405_ladder_413_ladder_423_ladder;
                                                float frontier_phi_416_405_ladder_413_ladder_423_ladder_1;
                                                float frontier_phi_416_405_ladder_413_ladder_423_ladder_2;
                                                float frontier_phi_416_405_ladder_413_ladder_423_ladder_3;
                                                float frontier_phi_416_405_ladder_413_ladder_423_ladder_4;
                                                float frontier_phi_416_405_ladder_413_ladder_423_ladder_5;
                                                float frontier_phi_416_405_ladder_413_ladder_423_ladder_6;
                                                float frontier_phi_416_405_ladder_413_ladder_423_ladder_7;
                                                float frontier_phi_416_405_ladder_413_ladder_423_ladder_8;
                                                if ((_989 & 50331648u) == 0u)
                                                {
                                                    float frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder;
                                                    float frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_1;
                                                    float frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_2;
                                                    float frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_3;
                                                    float frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_4;
                                                    float frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_5;
                                                    float frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_6;
                                                    float frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_7;
                                                    float frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_8;
                                                    if ((_989 & 1032192u) == 0u)
                                                    {
                                                        float _9515;
                                                        float _9520;
                                                        float _9525;
                                                        float _9545;
                                                        float _9547;
                                                        float _9549;
                                                        if ((_935 > 0.0f) && ((_989 & 3u) != 0u))
                                                        {
                                                            float _11175 = max(max(max(max(_938, 0.0f), _935), _940), 0.0f);
                                                            float _11181 = _57_m0[8u].x - _819;
                                                            float _11182 = _57_m0[8u].y - _820;
                                                            float _11183 = _57_m0[8u].z - _821;
                                                            float _11194 = clamp(2.0f - (clamp(sqrt(((_11181 * _11181) + (_11182 * _11182)) + (_11183 * _11183)) * 0.02500000037252902984619140625f, 0.0f, 1.0f) * 2.0f), 0.0f, 1.0f);
                                                            float _11195 = _11194 * _11175;
                                                            float _11201 = _57_m0[21u].x - _819;
                                                            float _11202 = _57_m0[21u].y - _820;
                                                            float _11203 = _57_m0[21u].z - _821;
                                                            float _11210 = log2(sqrt(((_11201 * _11201) + (_11202 * _11202)) + (_11203 * _11203)));
                                                            float _11211 = _11210 * 0.85000002384185791015625f;
                                                            float _12226;
                                                            float _12227;
                                                            float _12228;
                                                            float _12229;
                                                            if (_11195 > 0.00999999977648258209228515625f)
                                                            {
                                                                uint _12060 = _937 + 0u;
                                                                float _12061 = ceil(_11211);
                                                                float _12065 = _819 * 5.0f;
                                                                float _12066 = _821 * 5.0f;
                                                                float _12068 = exp2((-0.0f) - max(1.0f, _12061));
                                                                float _12074 = exp2((-0.0f) - max(1.0f, _12061 + 1.0f));
                                                                float4 _12083 = _37[NonUniformResourceIndex(_12060)].SampleLevel(_70, float2(frac(_12068 * _12065), frac(_12068 * _12066)), 0.0f);
                                                                float4 _12088 = _37[NonUniformResourceIndex(_12060)].SampleLevel(_70, float2(frac(_12074 * _12065), frac(_12074 * _12066)), 0.0f);
                                                                float _12093 = frac(_11211);
                                                                float _12095 = (_12093 + 0.5f) * 0.5f;
                                                                float _12107 = (_12083.x + (-0.5f)) * 2.0f;
                                                                float _12108 = (_12083.y + (-0.5f)) * 2.0f;
                                                                float _12114 = sqrt(clamp((1.0f - (_12107 * _12107)) - (_12108 * _12108), 0.0f, 1.0f));
                                                                float _12121 = (_12088.x + (-0.5f)) * 2.0f;
                                                                float _12122 = (_12088.y + (-0.5f)) * 2.0f;
                                                                float _12128 = sqrt(clamp((1.0f - (_12121 * _12121)) - (_12122 * _12122), 0.0f, 1.0f));
                                                                float _12134 = rsqrt(dot(float3(_12107, _12114, _12108), float3(_12107, _12114, _12108))) * (1.0f - _12093);
                                                                float _12137 = rsqrt(dot(float3(_12121, _12128, _12122), float3(_12121, _12128, _12122))) * _12093;
                                                                float _12140 = (_12137 * _12121) + (_12134 * _12107);
                                                                float _12141 = (_12137 * _12122) + (_12134 * _12108);
                                                                float _12145 = rsqrt(dot(float3(_12140, _12141, 1.0f), float3(_12140, _12141, 1.0f)));
                                                                float _12148 = mad(_12145, _822, _12140 * _12145);
                                                                float _12149 = mad(_12145, _827, 0.0f);
                                                                float _12150 = mad(_12145, _832, _12141 * _12145);
                                                                float _12154 = rsqrt(dot(float3(_12148, _12149, _12150), float3(_12148, _12149, _12150)));
                                                                float _12155 = _12154 * _12148;
                                                                float _12156 = _12154 * _12149;
                                                                float _12157 = _12154 * _12150;
                                                                float _12158 = _1555 + 1.0f;
                                                                float _12162 = rsqrt(dot(float3(_1554, _12158, _1556), float3(_1554, _12158, _1556)));
                                                                float _12205 = (((((_11195 * 2.2000000476837158203125f) * max(clamp(_12095 * (_12083.z - _12093), 0.0f, 1.0f), clamp((1.0f - _12095) * ((_12093 + (-1.0f)) + _12088.z), 0.0f, 1.0f))) * exp2((800.0f - (clamp(clamp((_11210 * 0.425000011920928955078125f) + (-1.0f), 0.0f, 1.0f), 0.0f, 1.0f) * 800.0f)) * log2(((_11175 * (0.004999999888241291046142578125f - (_940 * 0.00299999979324638843536376953125f))) + 0.00200000009499490261077880859375f) + max(dot(float3(_12162 * _1554, _12162 * _12158, _12162 * _1556), float3(_12155, _12156, _12157)), 0.0f)))) * exp2(log2(clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 1.2000000476837158203125f)) * clamp((dot(float3(_8635, _8636, _8637), float3(_822, _827, _832)) * 10.0f) + 0.5f, 0.0f, 1.0f)) * _57_m0[166u].y;
                                                                float _12207 = (_940 * 0.4000000059604644775390625f) * _11194;
                                                                float _12210 = ((_12157 + (-1.0f)) * _12207) + 1.0f;
                                                                float _12213 = _12207 * _832;
                                                                float _12216 = (_12210 * _822) + (_12155 * _12213);
                                                                float _12217 = (_12210 * _827) + (_12156 * _12213);
                                                                float _12218 = _12210 * _832;
                                                                float _12222 = rsqrt(dot(float3(_12216, _12217, _12218), float3(_12216, _12217, _12218)));
                                                                _12226 = _12205;
                                                                _12227 = _12222 * _12216;
                                                                _12228 = _12222 * _12217;
                                                                _12229 = _12222 * _12218;
                                                            }
                                                            else
                                                            {
                                                                _12226 = 0.0f;
                                                                _12227 = _822;
                                                                _12228 = _827;
                                                                _12229 = _832;
                                                            }
                                                            float _12233 = dot(float3(_12227, _12228, _12229), float3(_8635, _8636, _8637));
                                                            float _12239 = (1.0f - clamp(_12233 * 5.0f, 0.0f, 1.0f)) * _938;
                                                            float _12246 = (_12239 * (_850 - _942)) + _942;
                                                            float _12247 = (_12239 * (_855 - _944)) + _944;
                                                            float _12248 = (_12239 * (_860 - _946)) + _946;
                                                            float _12249 = 1.0f - _865;
                                                            float _12250 = 1.0f - _870;
                                                            float _12251 = 1.0f - _873;
                                                            float _12259 = exp2(log2(1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_12227, _12228, _12229)), 0.0f, 1.0f)) * 5.0f);
                                                            float _12266 = _940 * 0.550000011920928955078125f;
                                                            float _12267 = ((_12259 * _12249) + _865) * _12266;
                                                            float _12268 = ((_12259 * _12250) + _870) * _12266;
                                                            float _12269 = ((_12259 * _12251) + _873) * _12266;
                                                            float _12288 = clamp(_12233, 0.0f, 1.0f);
                                                            float _12769;
                                                            if (_57_m0[76u].w > 0.5f)
                                                            {
                                                                float _12732 = _8635 + _1554;
                                                                float _12733 = _8636 + _1555;
                                                                float _12734 = _8637 + _1556;
                                                                float _12738 = rsqrt(dot(float3(_12732, _12733, _12734), float3(_12732, _12733, _12734)));
                                                                float _12745 = clamp(dot(float3(_8635, _8636, _8637), float3(_12738 * _12732, _12738 * _12733, _12738 * _12734)), 0.0f, 1.0f);
                                                                float _12755 = ((((_12745 * _12745) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                                _12769 = ((exp2(log2(1.0f - clamp(dot(float3(_12227, _12228, _12229), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _12755) + 1.0f) * ((_12755 * exp2(log2(1.0f - _12288) * 5.0f)) + 1.0f);
                                                            }
                                                            else
                                                            {
                                                                _12769 = 1.0f;
                                                            }
                                                            float _12771 = _12769 * (clamp((_12288 * 0.722500026226043701171875f) + 0.12750001251697540283203125f, 0.0f, 1.0f) * 0.3183098733425140380859375f);
                                                            _9515 = (_12771 * _12249) * (((_12267 * _12267) * ((0.949999988079071044921875f - (_938 * 0.5f)) - _12246)) + _12246);
                                                            _9520 = (_12771 * _12250) * (((_12268 * _12268) * ((0.810000002384185791015625f - (_938 * 0.069999992847442626953125f)) - _12247)) + _12247);
                                                            _9525 = (_12771 * _12251) * (((_12269 * _12269) * (((_938 * 0.37999999523162841796875f) + 0.569999992847442626953125f) - _12248)) + _12248);
                                                            _9545 = _12226 * 0.449999988079071044921875f;
                                                            _9547 = _12226 * 0.449999988079071044921875f;
                                                            _9549 = _12226 * 0.449999988079071044921875f;
                                                        }
                                                        else
                                                        {
                                                            float _11219 = clamp(dot(float3(_822, _827, _832), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                            float _11220 = clamp(_11219, 0.0f, 1.0f);
                                                            float _12333;
                                                            if (_57_m0[76u].w > 0.5f)
                                                            {
                                                                float _12296 = _8635 + _1554;
                                                                float _12297 = _8636 + _1555;
                                                                float _12298 = _8637 + _1556;
                                                                float _12302 = rsqrt(dot(float3(_12296, _12297, _12298), float3(_12296, _12297, _12298)));
                                                                float _12309 = clamp(dot(float3(_8635, _8636, _8637), float3(_12302 * _12296, _12302 * _12297, _12302 * _12298)), 0.0f, 1.0f);
                                                                float _12319 = ((((_12309 * _12309) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                                _12333 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _12319) + 1.0f) * ((_12319 * exp2(log2(1.0f - _11219) * 5.0f)) + 1.0f);
                                                            }
                                                            else
                                                            {
                                                                _12333 = 1.0f;
                                                            }
                                                            _9515 = (((_850 * 0.3183098733425140380859375f) * (1.0f - _865)) * _11220) * _12333;
                                                            _9520 = (((_855 * 0.3183098733425140380859375f) * (1.0f - _870)) * _11220) * _12333;
                                                            _9525 = (((_860 * 0.3183098733425140380859375f) * (1.0f - _873)) * _11220) * _12333;
                                                            _9545 = 0.0f;
                                                            _9547 = 0.0f;
                                                            _9549 = 0.0f;
                                                        }
                                                        float _12781 = clamp(dot(float3(_837, _840, _843), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                        float _12986;
                                                        float _12988;
                                                        float _12990;
                                                        if (_12781 > 0.0f)
                                                        {
                                                            float _12923 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                            float _12924 = _8635 + _1554;
                                                            float _12925 = _8636 + _1555;
                                                            float _12926 = _8637 + _1556;
                                                            float _12930 = rsqrt(dot(float3(_12924, _12925, _12926), float3(_12924, _12925, _12926)));
                                                            float _12931 = _12930 * _12924;
                                                            float _12932 = _12930 * _12925;
                                                            float _12933 = _12930 * _12926;
                                                            float _12942 = clamp(dot(float3(_837, _840, _843), float3(_12931, _12932, _12933)), 0.0f, 1.0f);
                                                            float _12947 = _12923 * _12923;
                                                            float _12948 = _12947 * _12947;
                                                            float _12952 = (((_12942 * _12948) - _12942) * _12942) + 1.0f;
                                                            float _12956 = _12947 * 0.5f;
                                                            float _12957 = 1.0f - _12956;
                                                            float _12964 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_12931, _12932, _12933)), 0.0f, 1.0f);
                                                            float _12965 = _12964 * _12964;
                                                            float _12967 = (_12965 * _12965) * _12964;
                                                            float _12979 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _12957) + _12956) * ((_12957 * _12781) + _12956))) * (_12948 / ((_12952 * _12952) * 3.1415927410125732421875f)), _1534) * _4408;
                                                            float _12983 = min(_12979 * ((_12967 * (1.0f - _865)) + _865), 100000.0f);
                                                            float _12984 = min(_12979 * ((_12967 * (1.0f - _870)) + _870), 100000.0f);
                                                            float _12985 = min(_12979 * ((_12967 * (1.0f - _873)) + _873), 100000.0f);
                                                            float _13012;
                                                            float _13014;
                                                            float _13016;
                                                            if (_4412)
                                                            {
                                                                _13012 = _12983;
                                                                _13014 = _12984;
                                                                _13016 = _12985;
                                                            }
                                                            else
                                                            {
                                                                float _13018 = 1.0f - _12781;
                                                                float _13019 = _13018 * _13018;
                                                                float _13021 = 1.0f - (_13019 * _13019);
                                                                _13012 = _12983 * _13021;
                                                                _13014 = _12984 * _13021;
                                                                _13016 = _12985 * _13021;
                                                            }
                                                            _12986 = _13012 * _12781;
                                                            _12988 = _13014 * _12781;
                                                            _12990 = _13016 * _12781;
                                                        }
                                                        else
                                                        {
                                                            _12986 = 0.0f;
                                                            _12988 = 0.0f;
                                                            _12990 = 0.0f;
                                                        }
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder = _9525;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_1 = _9515;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_2 = _9520;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_3 = _12986 * _892;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_4 = _12988 * _892;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_5 = _9545;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_6 = _9547;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_7 = _9549;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_8 = _12990 * _892;
                                                    }
                                                    else
                                                    {
                                                        float _10565 = _9087 ? _62_m0[104u].w : 0.0f;
                                                        float _10566 = 1.0f - _865;
                                                        float _10567 = 1.0f - _870;
                                                        float _10568 = 1.0f - _873;
                                                        float _10572 = clamp(dot(float3(_822, _827, _832), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                        float _10573 = 1.0f - _10565;
                                                        float _10577 = clamp(((_10572 * _10573) + _10565) * _10573, 0.0f, 1.0f);
                                                        float _11262;
                                                        if (_57_m0[76u].w > 0.5f)
                                                        {
                                                            float _11225 = _8635 + _1554;
                                                            float _11226 = _8636 + _1555;
                                                            float _11227 = _8637 + _1556;
                                                            float _11231 = rsqrt(dot(float3(_11225, _11226, _11227), float3(_11225, _11226, _11227)));
                                                            float _11238 = clamp(dot(float3(_8635, _8636, _8637), float3(_11231 * _11225, _11231 * _11226, _11231 * _11227)), 0.0f, 1.0f);
                                                            float _11248 = ((((_11238 * _11238) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                            _11262 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _11248) + 1.0f) * ((_11248 * exp2(log2(1.0f - _10572) * 5.0f)) + 1.0f);
                                                        }
                                                        else
                                                        {
                                                            _11262 = 1.0f;
                                                        }
                                                        float _12371;
                                                        float _12373;
                                                        float _12375;
                                                        if (_10572 > 0.0f)
                                                        {
                                                            float _12351 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                            float _12352 = _8635 + _1554;
                                                            float _12353 = _8636 + _1555;
                                                            float _12354 = _8637 + _1556;
                                                            float _12358 = rsqrt(dot(float3(_12352, _12353, _12354), float3(_12352, _12353, _12354)));
                                                            float _12359 = _12358 * _12352;
                                                            float _12360 = _12358 * _12353;
                                                            float _12361 = _12358 * _12354;
                                                            float _12365 = clamp(dot(float3(_837, _840, _843), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                            float _12369 = clamp(dot(float3(_837, _840, _843), float3(_12359, _12360, _12361)), 0.0f, 1.0f);
                                                            float _12825;
                                                            float _12826;
                                                            float _12827;
                                                            if (_12365 > 0.0f)
                                                            {
                                                                float _12792 = _12351 * _12351;
                                                                float _12793 = _12792 * _12792;
                                                                float _12797 = (((_12369 * _12793) - _12369) * _12369) + 1.0f;
                                                                float _12801 = _12792 * 0.5f;
                                                                float _12802 = 1.0f - _12801;
                                                                float _12809 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_12359, _12360, _12361)), 0.0f, 1.0f);
                                                                float _12810 = _12809 * _12809;
                                                                float _12812 = (_12810 * _12810) * _12809;
                                                                float _12821 = min((0.25f / (((_12365 * _12802) + _12801) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _12802) + _12801))) * (_12793 / ((_12797 * _12797) * 3.1415927410125732421875f)), _1534) * _4408;
                                                                _12825 = _12821 * ((_12812 * _10566) + _865);
                                                                _12826 = _12821 * ((_12812 * _10567) + _870);
                                                                _12827 = _12821 * ((_12812 * _10568) + _873);
                                                            }
                                                            else
                                                            {
                                                                _12825 = 0.0f;
                                                                _12826 = 0.0f;
                                                                _12827 = 0.0f;
                                                            }
                                                            float _12828 = min(_12825, 100000.0f);
                                                            float _12829 = min(_12826, 100000.0f);
                                                            float _12830 = min(_12827, 100000.0f);
                                                            float _12992;
                                                            float _12994;
                                                            float _12996;
                                                            if (_4412)
                                                            {
                                                                _12992 = _12828;
                                                                _12994 = _12829;
                                                                _12996 = _12830;
                                                            }
                                                            else
                                                            {
                                                                float _12998 = 1.0f - _10572;
                                                                float _12999 = _12998 * _12998;
                                                                float _13001 = 1.0f - (_12999 * _12999);
                                                                _12992 = _12828 * _13001;
                                                                _12994 = _12829 * _13001;
                                                                _12996 = _12830 * _13001;
                                                            }
                                                            _12371 = _12992 * _10572;
                                                            _12373 = _12994 * _10572;
                                                            _12375 = _12996 * _10572;
                                                        }
                                                        else
                                                        {
                                                            _12371 = 0.0f;
                                                            _12373 = 0.0f;
                                                            _12375 = 0.0f;
                                                        }
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder = (((_860 * 0.3183098733425140380859375f) * _10568) * _10577) * _11262;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_1 = (((_850 * 0.3183098733425140380859375f) * _10566) * _10577) * _11262;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_2 = (((_855 * 0.3183098733425140380859375f) * _10567) * _10577) * _11262;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_3 = _12371 * _892;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_4 = _12373 * _892;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_5 = 0.0f;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_6 = 0.0f;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_7 = 0.0f;
                                                        frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_8 = _12375 * _892;
                                                    }
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder = frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_1 = frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_1;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_2 = frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_2;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_3 = frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_3;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_4 = frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_4;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_5 = frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_5;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_6 = frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_6;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_7 = frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_7;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_8 = frontier_phi_416_405_ladder_413_ladder_423_ladder_437_ladder_8;
                                                }
                                                else
                                                {
                                                    float _9994 = 1.0f - _876;
                                                    float _9995 = (-0.0f) - _784;
                                                    float _9996 = (-0.0f) - _778;
                                                    float _9997 = (-0.0f) - _772;
                                                    float _10005 = dot(float3(_9995, _9996, _9997), float3(_8635, _8636, _8637));
                                                    float _10011 = dot(float3(_9995, _9996, _9997), float3(_1554, _1555, _1556));
                                                    float _10017 = _8635 - (_10005 * _9995);
                                                    float _10018 = _8636 - (_10005 * _9996);
                                                    float _10019 = _8637 - (_10005 * _9997);
                                                    float _10023 = _1554 - (_10011 * _9995);
                                                    float _10024 = _1555 - (_10011 * _9996);
                                                    float _10025 = _1556 - (_10011 * _9997);
                                                    float _10038 = rsqrt((dot(float3(_10023, _10024, _10025), float3(_10023, _10024, _10025)) * dot(float3(_10017, _10018, _10019), float3(_10017, _10018, _10019))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_10017, _10018, _10019), float3(_10023, _10024, _10025));
                                                    float _10042 = sqrt(clamp((_10038 * 0.5f) + 0.5f, 0.0f, 1.0f));
                                                    float _10048 = cos(abs(asin(_10011) - asin(_10005)) * 0.5f);
                                                    float _10053 = max(_9994 * 2.0f, 0.00999999977648258209228515625f);
                                                    float _10055 = (-0.0f) - _986;
                                                    float _10057 = sin(_10055);
                                                    float _10068 = _10011 + _10005;
                                                    float _10069 = _10068 - ((_10057 * 2.0f) * (((cos(_10055) * _10042) * sqrt(1.0f - (_10011 * _10011))) + (_10057 * _10011)));
                                                    float _10071 = (_10042 * 1.41421353816986083984375f) * max((1.0f - _980) * _9994, 0.00999999977648258209228515625f);
                                                    float _10093 = _10068 - (_986 * 1.5f);
                                                    float _10119 = exp2(log2(1.0f - (_10048 * 0.5f)) * 5.0f) * 0.95347940921783447265625f;
                                                    float _10121 = 0.95347940921783447265625f - _10119;
                                                    float _10126 = clamp((1.0f - max(_984, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                                                    float _10132 = abs(sqrt(1.0f - (_10005 * _10005))) * clamp(dot(float3(_822, _827, _832), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                    float _10133 = (((_10042 * 0.25f) * (exp2((((_10069 * _10069) * (-0.5f)) / (_10071 * _10071)) * 1.44269502162933349609375f) / (_10071 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(dot(float3(_1554, _1555, _1556), float3(_8635, _8636, _8637)), 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f)) * _10132;
                                                    float _10135 = (_10126 * (exp2((((_10093 * _10093) * (-0.5f)) / (_10053 * _10053)) * 1.44269502162933349609375f) / (_10053 * 2.5066282749176025390625f))) * exp2(_10126 * ((_10038 * 24.5258159637451171875f) + (-24.208423614501953125f)));
                                                    float _10136 = _10132 * ((_10121 * _10121) * (_10119 + 0.0465205647051334381103515625f));
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder = 0.0f;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_1 = 0.0f;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_2 = 0.0f;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_3 = ((_10136 * exp2(((_970 * (-3.2000000476837158203125f)) / _10048) * 1.44269502162933349609375f)) * _10135) + _10133;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_4 = ((_10136 * exp2(((_972 * (-3.2000000476837158203125f)) / _10048) * 1.44269502162933349609375f)) * _10135) + _10133;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_5 = 0.0f;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_6 = 0.0f;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_7 = 0.0f;
                                                    frontier_phi_416_405_ladder_413_ladder_423_ladder_8 = ((_10136 * exp2(((_974 * (-3.2000000476837158203125f)) / _10048) * 1.44269502162933349609375f)) * _10135) + _10133;
                                                }
                                                frontier_phi_416_405_ladder_413_ladder = frontier_phi_416_405_ladder_413_ladder_423_ladder;
                                                frontier_phi_416_405_ladder_413_ladder_1 = frontier_phi_416_405_ladder_413_ladder_423_ladder_1;
                                                frontier_phi_416_405_ladder_413_ladder_2 = frontier_phi_416_405_ladder_413_ladder_423_ladder_2;
                                                frontier_phi_416_405_ladder_413_ladder_3 = frontier_phi_416_405_ladder_413_ladder_423_ladder_3;
                                                frontier_phi_416_405_ladder_413_ladder_4 = frontier_phi_416_405_ladder_413_ladder_423_ladder_4;
                                                frontier_phi_416_405_ladder_413_ladder_5 = frontier_phi_416_405_ladder_413_ladder_423_ladder_5;
                                                frontier_phi_416_405_ladder_413_ladder_6 = frontier_phi_416_405_ladder_413_ladder_423_ladder_6;
                                                frontier_phi_416_405_ladder_413_ladder_7 = frontier_phi_416_405_ladder_413_ladder_423_ladder_7;
                                                frontier_phi_416_405_ladder_413_ladder_8 = frontier_phi_416_405_ladder_413_ladder_423_ladder_8;
                                            }
                                            else
                                            {
                                                float _9659 = clamp(dot(float3(_822, _827, _832), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                float _9528;
                                                float _9534;
                                                float _9540;
                                                if (_9659 > 0.0f)
                                                {
                                                    float _10148 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                    float _10149 = _8635 + _1554;
                                                    float _10150 = _8636 + _1555;
                                                    float _10151 = _8637 + _1556;
                                                    float _10155 = rsqrt(dot(float3(_10149, _10150, _10151), float3(_10149, _10150, _10151)));
                                                    float _10156 = _10155 * _10149;
                                                    float _10157 = _10155 * _10150;
                                                    float _10158 = _10155 * _10151;
                                                    float _10162 = clamp(dot(float3(_837, _840, _843), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                    float _10166 = clamp(dot(float3(_837, _840, _843), float3(_10156, _10157, _10158)), 0.0f, 1.0f);
                                                    float _10627;
                                                    float _10628;
                                                    float _10629;
                                                    if (_10162 > 0.0f)
                                                    {
                                                        float _10591 = _10148 * _10148;
                                                        float _10592 = _10591 * _10591;
                                                        float _10596 = (((_10166 * _10592) - _10166) * _10166) + 1.0f;
                                                        float _10600 = _10591 * 0.5f;
                                                        float _10601 = 1.0f - _10600;
                                                        float _10608 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_10156, _10157, _10158)), 0.0f, 1.0f);
                                                        float _10609 = _10608 * _10608;
                                                        float _10611 = (_10609 * _10609) * _10608;
                                                        float _10623 = min((0.25f / (((_10162 * _10601) + _10600) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _10601) + _10600))) * (_10592 / ((_10596 * _10596) * 3.1415927410125732421875f)), _1534) * _4408;
                                                        _10627 = _10623 * ((_10611 * (1.0f - _865)) + _865);
                                                        _10628 = _10623 * ((_10611 * (1.0f - _870)) + _870);
                                                        _10629 = _10623 * ((_10611 * (1.0f - _873)) + _873);
                                                    }
                                                    else
                                                    {
                                                        _10627 = 0.0f;
                                                        _10628 = 0.0f;
                                                        _10629 = 0.0f;
                                                    }
                                                    float _10630 = min(_10627, 100000.0f);
                                                    float _10631 = min(_10628, 100000.0f);
                                                    float _10632 = min(_10629, 100000.0f);
                                                    float _11273;
                                                    float _11275;
                                                    float _11277;
                                                    if (_4412)
                                                    {
                                                        _11273 = _10630;
                                                        _11275 = _10631;
                                                        _11277 = _10632;
                                                    }
                                                    else
                                                    {
                                                        float _11279 = 1.0f - _9659;
                                                        float _11280 = _11279 * _11279;
                                                        float _11282 = 1.0f - (_11280 * _11280);
                                                        _11273 = _10630 * _11282;
                                                        _11275 = _10631 * _11282;
                                                        _11277 = _10632 * _11282;
                                                    }
                                                    _9528 = _11273 * _9659;
                                                    _9534 = _11275 * _9659;
                                                    _9540 = _11277 * _9659;
                                                }
                                                else
                                                {
                                                    _9528 = 0.0f;
                                                    _9534 = 0.0f;
                                                    _9540 = 0.0f;
                                                }
                                                float _10171 = _9087 ? _62_m0[104u].w : 0.0f;
                                                float _10175 = 1.0f - _10171;
                                                float _10179 = clamp(((_9659 * _10175) + _10171) * _10175, 0.0f, 1.0f);
                                                float _10670;
                                                if (_57_m0[76u].w > 0.5f)
                                                {
                                                    float _10633 = _8635 + _1554;
                                                    float _10634 = _8636 + _1555;
                                                    float _10635 = _8637 + _1556;
                                                    float _10639 = rsqrt(dot(float3(_10633, _10634, _10635), float3(_10633, _10634, _10635)));
                                                    float _10646 = clamp(dot(float3(_8635, _8636, _8637), float3(_10639 * _10633, _10639 * _10634, _10639 * _10635)), 0.0f, 1.0f);
                                                    float _10656 = ((((_10646 * _10646) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                    _10670 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _10656) + 1.0f) * ((_10656 * exp2(log2(1.0f - _9659) * 5.0f)) + 1.0f);
                                                }
                                                else
                                                {
                                                    _10670 = 1.0f;
                                                }
                                                float _10671 = 1.0f - _960;
                                                frontier_phi_416_405_ladder_413_ladder = ((((_860 * 0.3183098733425140380859375f) * (1.0f - _873)) * _10671) * _10179) * _10670;
                                                frontier_phi_416_405_ladder_413_ladder_1 = ((((_850 * 0.3183098733425140380859375f) * (1.0f - _865)) * _10671) * _10179) * _10670;
                                                frontier_phi_416_405_ladder_413_ladder_2 = ((((_855 * 0.3183098733425140380859375f) * (1.0f - _870)) * _10671) * _10179) * _10670;
                                                frontier_phi_416_405_ladder_413_ladder_3 = _9528;
                                                frontier_phi_416_405_ladder_413_ladder_4 = _9534;
                                                frontier_phi_416_405_ladder_413_ladder_5 = 0.0f;
                                                frontier_phi_416_405_ladder_413_ladder_6 = 0.0f;
                                                frontier_phi_416_405_ladder_413_ladder_7 = 0.0f;
                                                frontier_phi_416_405_ladder_413_ladder_8 = _9540;
                                            }
                                            frontier_phi_416_405_ladder = frontier_phi_416_405_ladder_413_ladder;
                                            frontier_phi_416_405_ladder_1 = frontier_phi_416_405_ladder_413_ladder_1;
                                            frontier_phi_416_405_ladder_2 = frontier_phi_416_405_ladder_413_ladder_2;
                                            frontier_phi_416_405_ladder_3 = frontier_phi_416_405_ladder_413_ladder_3;
                                            frontier_phi_416_405_ladder_4 = frontier_phi_416_405_ladder_413_ladder_4;
                                            frontier_phi_416_405_ladder_5 = frontier_phi_416_405_ladder_413_ladder_5;
                                            frontier_phi_416_405_ladder_6 = frontier_phi_416_405_ladder_413_ladder_6;
                                            frontier_phi_416_405_ladder_7 = frontier_phi_416_405_ladder_413_ladder_7;
                                            frontier_phi_416_405_ladder_8 = frontier_phi_416_405_ladder_413_ladder_8;
                                        }
                                        else
                                        {
                                            float _9384 = _9087 ? _62_m0[104u].w : 0.0f;
                                            float _9385 = 1.0f - _865;
                                            float _9386 = 1.0f - _870;
                                            float _9387 = 1.0f - _873;
                                            float _9391 = clamp(dot(float3(_822, _827, _832), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                            float _9392 = 1.0f - _9384;
                                            float _9396 = clamp(((_9391 * _9392) + _9384) * _9392, 0.0f, 1.0f);
                                            float _9698;
                                            if (_57_m0[76u].w > 0.5f)
                                            {
                                                float _9661 = _8635 + _1554;
                                                float _9662 = _8636 + _1555;
                                                float _9663 = _8637 + _1556;
                                                float _9667 = rsqrt(dot(float3(_9661, _9662, _9663), float3(_9661, _9662, _9663)));
                                                float _9674 = clamp(dot(float3(_8635, _8636, _8637), float3(_9667 * _9661, _9667 * _9662, _9667 * _9663)), 0.0f, 1.0f);
                                                float _9684 = ((((_9674 * _9674) * 2.0f) + 0.5f) * (1.0f - _876)) + (-1.0f);
                                                _9698 = ((exp2(log2(1.0f - clamp(dot(float3(_822, _827, _832), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * 5.0f) * _9684) + 1.0f) * ((_9684 * exp2(log2(1.0f - _9391) * 5.0f)) + 1.0f);
                                            }
                                            else
                                            {
                                                _9698 = 1.0f;
                                            }
                                            bool _9708 = _9391 > 0.0f;
                                            float _10209;
                                            float _10211;
                                            float _10213;
                                            if (_9708)
                                            {
                                                float _10189 = max(exp2(log2(clamp(1.0f - _876, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                float _10190 = _8635 + _1554;
                                                float _10191 = _8636 + _1555;
                                                float _10192 = _8637 + _1556;
                                                float _10196 = rsqrt(dot(float3(_10190, _10191, _10192), float3(_10190, _10191, _10192)));
                                                float _10197 = _10196 * _10190;
                                                float _10198 = _10196 * _10191;
                                                float _10199 = _10196 * _10192;
                                                float _10203 = clamp(dot(float3(_837, _840, _843), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                float _10207 = clamp(dot(float3(_837, _840, _843), float3(_10197, _10198, _10199)), 0.0f, 1.0f);
                                                float _10726;
                                                float _10727;
                                                float _10728;
                                                if (_10203 > 0.0f)
                                                {
                                                    float _10693 = _10189 * _10189;
                                                    float _10694 = _10693 * _10693;
                                                    float _10698 = (((_10207 * _10694) - _10207) * _10207) + 1.0f;
                                                    float _10702 = _10693 * 0.5f;
                                                    float _10703 = 1.0f - _10702;
                                                    float _10710 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_10197, _10198, _10199)), 0.0f, 1.0f);
                                                    float _10711 = _10710 * _10710;
                                                    float _10713 = (_10711 * _10711) * _10710;
                                                    float _10722 = min((0.25f / (((_10203 * _10703) + _10702) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _10703) + _10702))) * (_10694 / ((_10698 * _10698) * 3.1415927410125732421875f)), _1534) * _4408;
                                                    _10726 = _10722 * ((_10713 * _9385) + _865);
                                                    _10727 = _10722 * ((_10713 * _9386) + _870);
                                                    _10728 = _10722 * ((_10713 * _9387) + _873);
                                                }
                                                else
                                                {
                                                    _10726 = 0.0f;
                                                    _10727 = 0.0f;
                                                    _10728 = 0.0f;
                                                }
                                                float _10729 = min(_10726, 100000.0f);
                                                float _10730 = min(_10727, 100000.0f);
                                                float _10731 = min(_10728, 100000.0f);
                                                float _11283;
                                                float _11285;
                                                float _11287;
                                                if (_4412)
                                                {
                                                    _11283 = _10729;
                                                    _11285 = _10730;
                                                    _11287 = _10731;
                                                }
                                                else
                                                {
                                                    float _11289 = 1.0f - _9391;
                                                    float _11290 = _11289 * _11289;
                                                    float _11292 = 1.0f - (_11290 * _11290);
                                                    _11283 = _10729 * _11292;
                                                    _11285 = _10730 * _11292;
                                                    _11287 = _10731 * _11292;
                                                }
                                                _10209 = _11283 * _9391;
                                                _10211 = _11285 * _9391;
                                                _10213 = _11287 * _9391;
                                            }
                                            else
                                            {
                                                _10209 = 0.0f;
                                                _10211 = 0.0f;
                                                _10213 = 0.0f;
                                            }
                                            float _10757;
                                            float _10759;
                                            float _10761;
                                            if (_9708)
                                            {
                                                float _10737 = max(exp2(log2(clamp(1.0f - _915, 0.0f, 1.0f)) * _62_m0[107u].y), _1533);
                                                float _10738 = _8635 + _1554;
                                                float _10739 = _8636 + _1555;
                                                float _10740 = _8637 + _1556;
                                                float _10744 = rsqrt(dot(float3(_10738, _10739, _10740), float3(_10738, _10739, _10740)));
                                                float _10745 = _10744 * _10738;
                                                float _10746 = _10744 * _10739;
                                                float _10747 = _10744 * _10740;
                                                float _10751 = clamp(dot(float3(_837, _840, _843), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                float _10755 = clamp(dot(float3(_837, _840, _843), float3(_10745, _10746, _10747)), 0.0f, 1.0f);
                                                float _11335;
                                                float _11336;
                                                float _11337;
                                                if (_10751 > 0.0f)
                                                {
                                                    float _11302 = _10737 * _10737;
                                                    float _11303 = _11302 * _11302;
                                                    float _11307 = (((_10755 * _11303) - _10755) * _10755) + 1.0f;
                                                    float _11311 = _11302 * 0.5f;
                                                    float _11312 = 1.0f - _11311;
                                                    float _11319 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_10745, _10746, _10747)), 0.0f, 1.0f);
                                                    float _11320 = _11319 * _11319;
                                                    float _11322 = (_11320 * _11320) * _11319;
                                                    float _11331 = min((0.25f / (((_10751 * _11312) + _11311) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f)) * _11312) + _11311))) * (_11303 / ((_11307 * _11307) * 3.1415927410125732421875f)), _1534) * _4408;
                                                    _11335 = _11331 * ((_11322 * _9385) + _865);
                                                    _11336 = _11331 * ((_11322 * _9386) + _870);
                                                    _11337 = _11331 * ((_11322 * _9387) + _873);
                                                }
                                                else
                                                {
                                                    _11335 = 0.0f;
                                                    _11336 = 0.0f;
                                                    _11337 = 0.0f;
                                                }
                                                float _11338 = min(_11335, 100000.0f);
                                                float _11339 = min(_11336, 100000.0f);
                                                float _11340 = min(_11337, 100000.0f);
                                                float _12377;
                                                float _12379;
                                                float _12381;
                                                if (_4412)
                                                {
                                                    _12377 = _11338;
                                                    _12379 = _11339;
                                                    _12381 = _11340;
                                                }
                                                else
                                                {
                                                    float _12383 = 1.0f - _9391;
                                                    float _12384 = _12383 * _12383;
                                                    float _12386 = 1.0f - (_12384 * _12384);
                                                    _12377 = _11338 * _12386;
                                                    _12379 = _11339 * _12386;
                                                    _12381 = _11340 * _12386;
                                                }
                                                _10757 = _12377 * _9391;
                                                _10759 = _12379 * _9391;
                                                _10761 = _12381 * _9391;
                                            }
                                            else
                                            {
                                                _10757 = 0.0f;
                                                _10759 = 0.0f;
                                                _10761 = 0.0f;
                                            }
                                            frontier_phi_416_405_ladder = (((_860 * 0.3183098733425140380859375f) * _9387) * _9396) * _9698;
                                            frontier_phi_416_405_ladder_1 = (((_850 * 0.3183098733425140380859375f) * _9385) * _9396) * _9698;
                                            frontier_phi_416_405_ladder_2 = (((_855 * 0.3183098733425140380859375f) * _9386) * _9396) * _9698;
                                            frontier_phi_416_405_ladder_3 = ((_10757 - _10209) * _917) + _10209;
                                            frontier_phi_416_405_ladder_4 = ((_10759 - _10211) * _917) + _10211;
                                            frontier_phi_416_405_ladder_5 = 0.0f;
                                            frontier_phi_416_405_ladder_6 = 0.0f;
                                            frontier_phi_416_405_ladder_7 = 0.0f;
                                            frontier_phi_416_405_ladder_8 = ((_10761 - _10213) * _917) + _10213;
                                        }
                                        _9511 = frontier_phi_416_405_ladder_1;
                                        _9516 = frontier_phi_416_405_ladder_2;
                                        _9521 = frontier_phi_416_405_ladder;
                                        _9526 = frontier_phi_416_405_ladder_3;
                                        _9532 = frontier_phi_416_405_ladder_4;
                                        _9538 = frontier_phi_416_405_ladder_8;
                                        _9544 = frontier_phi_416_405_ladder_5;
                                        _9546 = frontier_phi_416_405_ladder_6;
                                        _9548 = frontier_phi_416_405_ladder_7;
                                    }
                                    else
                                    {
                                        float _9160 = clamp(dot(float3(_822, _827, _832), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                        float frontier_phi_416_406_ladder;
                                        float frontier_phi_416_406_ladder_1;
                                        float frontier_phi_416_406_ladder_2;
                                        float frontier_phi_416_406_ladder_3;
                                        float frontier_phi_416_406_ladder_4;
                                        float frontier_phi_416_406_ladder_5;
                                        float frontier_phi_416_406_ladder_6;
                                        float frontier_phi_416_406_ladder_7;
                                        float frontier_phi_416_406_ladder_8;
                                        if (_9160 > 0.0f)
                                        {
                                            float _9402 = _8635 + _1554;
                                            float _9403 = _8636 + _1555;
                                            float _9404 = _8637 + _1556;
                                            float _9408 = rsqrt(dot(float3(_9402, _9403, _9404), float3(_9402, _9403, _9404)));
                                            float _9409 = _9408 * _9402;
                                            float _9410 = _9408 * _9403;
                                            float _9411 = _9408 * _9404;
                                            float _9416 = max(clamp(dot(float3(_837, _840, _843), float3(_1554, _1555, _1556)), 0.0f, 1.0f), 0.001000000047497451305389404296875f);
                                            float _9420 = clamp(dot(float3(_837, _840, _843), float3(_9409, _9410, _9411)), 0.0f, 1.0f);
                                            float _9425 = max(1.0f - _876, 0.04500000178813934326171875f);
                                            float _9426 = _9425 * _9425;
                                            float _9427 = 1.0f / _9426;
                                            float _9441 = (((_9427 + 2.0f) * 0.15915493667125701904296875f) * exp2((_9427 * 0.5f) * log2(1.0f - (_9420 * _9420)))) * (0.25f / ((_9416 + _9160) - (_9416 * _9160)));
                                            float _9445 = _9426 * _9426;
                                            float _9449 = (((_9445 * _9420) - _9420) * _9420) + 1.0f;
                                            float _9453 = _9426 * 0.5f;
                                            float _9454 = 1.0f - _9453;
                                            float _9461 = 1.0f - clamp(dot(float3(_1554, _1555, _1556), float3(_9409, _9410, _9411)), 0.0f, 1.0f);
                                            float _9462 = _9461 * _9461;
                                            float _9464 = (_9462 * _9462) * _9461;
                                            float _9476 = min((0.25f / (((_9454 * _9416) + _9453) * ((_9454 * _9160) + _9453))) * (_9445 / ((_9449 * _9449) * 3.1415927410125732421875f)), _1534) * _4408;
                                            float _9477 = _9476 * ((_9464 * (1.0f - _865)) + _865);
                                            float _9478 = _9476 * ((_9464 * (1.0f - _870)) + _870);
                                            float _9479 = _9476 * ((_9464 * (1.0f - _873)) + _873);
                                            float _9483 = (1.0f - _923) * _850;
                                            float _9484 = (1.0f - _925) * _855;
                                            float _9485 = (1.0f - _927) * _860;
                                            float _9507 = _9160 * 0.3183098733425140380859375f;
                                            frontier_phi_416_406_ladder = _9507 * (((_860 - _9485) * _929) + _9485);
                                            frontier_phi_416_406_ladder_1 = _9507 * (((_850 - _9483) * _929) + _9483);
                                            frontier_phi_416_406_ladder_2 = _9507 * (((_855 - _9484) * _929) + _9484);
                                            frontier_phi_416_406_ladder_3 = ((((_9441 * _923) - _9477) * _929) + _9477) * _9160;
                                            frontier_phi_416_406_ladder_4 = ((((_9441 * _925) - _9478) * _929) + _9478) * _9160;
                                            frontier_phi_416_406_ladder_5 = 0.0f;
                                            frontier_phi_416_406_ladder_6 = 0.0f;
                                            frontier_phi_416_406_ladder_7 = 0.0f;
                                            frontier_phi_416_406_ladder_8 = ((((_9441 * _927) - _9479) * _929) + _9479) * _9160;
                                        }
                                        else
                                        {
                                            frontier_phi_416_406_ladder = 0.0f;
                                            frontier_phi_416_406_ladder_1 = 0.0f;
                                            frontier_phi_416_406_ladder_2 = 0.0f;
                                            frontier_phi_416_406_ladder_3 = 0.0f;
                                            frontier_phi_416_406_ladder_4 = 0.0f;
                                            frontier_phi_416_406_ladder_5 = 0.0f;
                                            frontier_phi_416_406_ladder_6 = 0.0f;
                                            frontier_phi_416_406_ladder_7 = 0.0f;
                                            frontier_phi_416_406_ladder_8 = 0.0f;
                                        }
                                        _9511 = frontier_phi_416_406_ladder_1;
                                        _9516 = frontier_phi_416_406_ladder_2;
                                        _9521 = frontier_phi_416_406_ladder;
                                        _9526 = frontier_phi_416_406_ladder_3;
                                        _9532 = frontier_phi_416_406_ladder_4;
                                        _9538 = frontier_phi_416_406_ladder_8;
                                        _9544 = frontier_phi_416_406_ladder_5;
                                        _9546 = frontier_phi_416_406_ladder_6;
                                        _9548 = frontier_phi_416_406_ladder_7;
                                    }
                                    float _9709;
                                    float _9711;
                                    float _9713;
                                    if (_913 == 0u)
                                    {
                                        _9709 = 0.0f;
                                        _9711 = 0.0f;
                                        _9713 = 0.0f;
                                    }
                                    else
                                    {
                                        float _10843;
                                        float _10847;
                                        float _10851;
                                        if ((_989 & 8388608u) == 0u)
                                        {
                                            float frontier_phi_470_443_ladder;
                                            float frontier_phi_470_443_ladder_1;
                                            float frontier_phi_470_443_ladder_2;
                                            if ((_989 & 67108864u) == 0u)
                                            {
                                                float frontier_phi_470_443_ladder_468_ladder;
                                                float frontier_phi_470_443_ladder_468_ladder_1;
                                                float frontier_phi_470_443_ladder_468_ladder_2;
                                                if ((_989 & 50331648u) == 0u)
                                                {
                                                    float frontier_phi_470_443_ladder_468_ladder_493_ladder;
                                                    float frontier_phi_470_443_ladder_468_ladder_493_ladder_1;
                                                    float frontier_phi_470_443_ladder_468_ladder_493_ladder_2;
                                                    if ((_989 & 536870912u) == 0u)
                                                    {
                                                        float _12397 = clamp((dot(float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), float3(_8635, _8636, _8637)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_906 * 0.3183098733425140380859375f);
                                                        frontier_phi_470_443_ladder_468_ladder_493_ladder = _12397 * _903;
                                                        frontier_phi_470_443_ladder_468_ladder_493_ladder_1 = _12397 * _900;
                                                        frontier_phi_470_443_ladder_468_ladder_493_ladder_2 = _12397 * _897;
                                                    }
                                                    else
                                                    {
                                                        float _12408 = clamp((dot(float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), float3(_8635, _8636, _8637)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_906 * 0.3183098733425140380859375f);
                                                        frontier_phi_470_443_ladder_468_ladder_493_ladder = _12408 * _903;
                                                        frontier_phi_470_443_ladder_468_ladder_493_ladder_1 = _12408 * _900;
                                                        frontier_phi_470_443_ladder_468_ladder_493_ladder_2 = _12408 * _897;
                                                    }
                                                    frontier_phi_470_443_ladder_468_ladder = frontier_phi_470_443_ladder_468_ladder_493_ladder;
                                                    frontier_phi_470_443_ladder_468_ladder_1 = frontier_phi_470_443_ladder_468_ladder_493_ladder_1;
                                                    frontier_phi_470_443_ladder_468_ladder_2 = frontier_phi_470_443_ladder_468_ladder_493_ladder_2;
                                                }
                                                else
                                                {
                                                    float _11345 = 1.0f - _876;
                                                    float _11346 = (-0.0f) - _784;
                                                    float _11347 = (-0.0f) - _778;
                                                    float _11348 = (-0.0f) - _772;
                                                    float _11351 = 1.0f - clamp(_976 * 66.6666717529296875f, 0.0f, 1.0f);
                                                    float _11355 = dot(float3(_11346, _11347, _11348), float3(_8635, _8636, _8637));
                                                    float _11361 = dot(float3(_11346, _11347, _11348), float3(_1554, _1555, _1556));
                                                    float _11367 = _8635 - (_11355 * _11346);
                                                    float _11368 = _8636 - (_11355 * _11347);
                                                    float _11369 = _8637 - (_11355 * _11348);
                                                    float _11373 = _1554 - (_11361 * _11346);
                                                    float _11374 = _1555 - (_11361 * _11347);
                                                    float _11375 = _1556 - (_11361 * _11348);
                                                    float _11388 = rsqrt((dot(float3(_11373, _11374, _11375), float3(_11373, _11374, _11375)) * dot(float3(_11367, _11368, _11369), float3(_11367, _11368, _11369))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_11367, _11368, _11369), float3(_11373, _11374, _11375));
                                                    float _11390 = (_11388 * 0.5f) + 0.5f;
                                                    float _11393 = asin(_11361);
                                                    float _11394 = asin(_11355);
                                                    float _11398 = cos(abs(_11393 - _11394) * 0.5f);
                                                    float _11402 = 1.0f / ((1.190000057220458984375f / _11398) + (_11398 * 0.36000001430511474609375f));
                                                    float _11403 = _11361 * 0.645161330699920654296875f;
                                                    float _11406 = sqrt(1.0f - (_11403 * _11403));
                                                    float _11408 = max(_11345 * 0.5f, 0.00999999977648258209228515625f);
                                                    float _11411 = _11361 + (_11355 - (_986 * 0.5f));
                                                    float _11419 = exp2((((_11411 * _11411) * (-0.5f)) / (_11408 * _11408)) * 1.44269502162933349609375f) / (_11408 * 2.5066282749176025390625f);
                                                    float _11428 = clamp((-0.0f) - dot(float3(_1554, _1555, _1556), float3(_8635, _8636, _8637)), 0.0f, 1.0f);
                                                    float _11429 = _11428 * _11428;
                                                    float _11432 = (_11429 * _11429) * (_11428 * _11351);
                                                    float _11439 = (cos(asin((_11402 * sqrt(clamp(_11390, 0.0f, 1.0f))) * ((_11402 * (0.60000002384185791015625f - (_11388 * 0.800000011920928955078125f))) + 1.0f)) * 2.0f) + 1.0f) * (-2.8853900432586669921875f);
                                                    float _11443 = exp2(_11439 * (_970 / _11406));
                                                    float _11444 = exp2(_11439 * (_972 / _11406));
                                                    float _11445 = exp2(_11439 * (_974 / _11406));
                                                    float _11460 = 0.95347940921783447265625f - (exp2(log2(1.0f - _11398) * 5.0f) * 0.95347940921783447265625f);
                                                    float _11461 = _11460 * _11460;
                                                    float _11463 = (_11398 * 0.5f) + 0.5f;
                                                    float _11475 = abs(sqrt(1.0f - (_11355 * _11355))) * _15[(_968 + 513u) + 0u].SampleLevel(_65, float2(_11463, _11390), 0.0f).x;
                                                    float _11476 = _11475 * (_11419 * _11351);
                                                    float _11483 = _976 + 9899999600270360182784.0f;
                                                    float _11484 = _876 * 0.75f;
                                                    float _11488 = (((_978 * _978) * 9000.0f) * _978) * _11483;
                                                    float _12831;
                                                    float _12832;
                                                    float _12833;
                                                    if ((_989 & 33554432u) == 0u)
                                                    {
                                                        float _12409 = max(_11488, _11484);
                                                        float _12415 = _11394 + _11393;
                                                        float _12416 = _12415 * 0.5f;
                                                        float _12418 = _970 * 0.5f;
                                                        float _12419 = _972 * 0.5f;
                                                        float _12420 = _974 * 0.5f;
                                                        float _12421 = max(_11345, 0.0500000007450580596923828125f);
                                                        float _12423 = (cos(_12416) * 0.5f) + 0.5f;
                                                        uint _12425 = (_968 + 521u) + 0u;
                                                        float4 _12430 = _19[_12425].SampleLevel(_65, float3(_12423, _12421, _12418), 0.0f);
                                                        float _12432 = _12430.x;
                                                        float4 _12433 = _19[_12425].SampleLevel(_65, float3(_12423, _12421, _12419), 0.0f);
                                                        float _12435 = _12433.x;
                                                        float4 _12436 = _19[_12425].SampleLevel(_65, float3(_12423, _12421, _12420), 0.0f);
                                                        float _12438 = _12436.x;
                                                        float4 _12439 = _19[_12425].SampleLevel(_65, float3(_11463, _12421, _12418), 0.0f);
                                                        float4 _12443 = _19[_12425].SampleLevel(_65, float3(_11463, _12421, _12419), 0.0f);
                                                        float4 _12447 = _19[_12425].SampleLevel(_65, float3(_11463, _12421, _12420), 0.0f);
                                                        uint _12452 = (_968 + 545u) + 0u;
                                                        float _12467 = (_12435 + _12432) + _12438;
                                                        float _12474 = dot(float3(max((1.0f - _980) * _11345, 0.00999999977648258209228515625f), _11408, max(_11345 * 2.0f, 0.00999999977648258209228515625f)), float3(_12432 / _12467, _12435 / _12467, _12438 / _12467)) * _12409;
                                                        float _12485 = (_11398 * _11398) * 3.1415927410125732421875f;
                                                        float _12489 = _12474 * 0.5f;
                                                        float _12490 = _12489 + _12439.z;
                                                        float _12491 = _12489 + _12443.z;
                                                        float _12492 = _12489 + _12447.z;
                                                        float _12494 = (_12415 * (-0.25f)) * _12416;
                                                        float _12516 = ((_12439.y * 2.0f) * (exp2((_12494 / (_12490 * _12490)) * 1.44269502162933349609375f) / (_12490 * 2.5066282749176025390625f))) / _12485;
                                                        float _12517 = ((_12443.y * 2.0f) * (exp2((_12494 / (_12491 * _12491)) * 1.44269502162933349609375f) / (_12491 * 2.5066282749176025390625f))) / _12485;
                                                        float _12518 = ((_12447.y * 2.0f) * (exp2((_12494 / (_12492 * _12492)) * 1.44269502162933349609375f) / (_12492 * 2.5066282749176025390625f))) / _12485;
                                                        float _12519 = _12415 - _986;
                                                        float _12522 = max(max(_12474, _12474), _12474) + _11408;
                                                        float _12530 = exp2((((_12519 * _12519) * (-0.125f)) / (_12522 * _12522)) * 1.44269502162933349609375f) / (_12522 * 2.5066282749176025390625f);
                                                        _12831 = (((exp2(log2(_12432) * _12409) * 0.4899999797344207763671875f) * ((_12530 * _15[_12452].SampleLevel(_65, float2(_11463, _12418), 0.0f).y) + (_12516 * 2.19911479949951171875f))) + (_12516 * 0.3499999940395355224609375f)) * _982;
                                                        _12832 = (((exp2(log2(_12435) * _12409) * 0.4899999797344207763671875f) * ((_12530 * _15[_12452].SampleLevel(_65, float2(_11463, _12419), 0.0f).y) + (_12517 * 2.19911479949951171875f))) + (_12517 * 0.3499999940395355224609375f)) * _982;
                                                        _12833 = (((exp2(log2(_12438) * _12409) * 0.4899999797344207763671875f) * ((_12530 * _15[_12452].SampleLevel(_65, float2(_11463, _12420), 0.0f).y) + (_12518 * 2.19911479949951171875f))) + (_12518 * 0.3499999940395355224609375f)) * _982;
                                                    }
                                                    else
                                                    {
                                                        float _12559 = 1.0f - clamp((_11483 * 66.6666717529296875f) * max(_978, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f);
                                                        float _12560 = _12559 * _12559;
                                                        float _12562 = (_12560 * _12560) * _12559;
                                                        float _12563 = _11461 * _11419;
                                                        float _12571 = (_11484 + 1.25f) + _11488;
                                                        float _12595 = dot(float3(_1554, _1555, _1556), float3(_11346, _11347, _11348));
                                                        float _12601 = _1554 - (_12595 * _11346);
                                                        float _12602 = _1555 - (_12595 * _11347);
                                                        float _12603 = _1556 - (_12595 * _11348);
                                                        float _12607 = rsqrt(dot(float3(_12601, _12602, _12603), float3(_12601, _12602, _12603)));
                                                        float _12615 = (dot(float3(_12601 * _12607, _12602 * _12607, _12603 * _12607), float3(_8635, _8636, _8637)) + 1.0f) * 0.25f;
                                                        float _12620 = (_982 * 0.2228169143199920654296875f) * ((((1.0f - abs(_11355)) - _12615) * 0.3300000131130218505859375f) + _12615);
                                                        _12831 = ((_12620 * exp2(log2(exp2(((_970 * (-3.7999999523162841796875f)) / _11398) * 1.44269502162933349609375f)) * _12571)) + ((_11475 * _11443) * _12563)) * _12562;
                                                        _12832 = ((_12620 * exp2(log2(exp2(((_972 * (-3.7999999523162841796875f)) / _11398) * 1.44269502162933349609375f)) * _12571)) + ((_11475 * _11444) * _12563)) * _12562;
                                                        _12833 = ((_12620 * exp2(log2(exp2(((_974 * (-3.7999999523162841796875f)) / _11398) * 1.44269502162933349609375f)) * _12571)) + ((_11475 * _11445) * _12563)) * _12562;
                                                    }
                                                    frontier_phi_470_443_ladder_468_ladder = _12833 + ((_11476 * (((1.0f - _11445) * _11432) + _11445)) * _11461);
                                                    frontier_phi_470_443_ladder_468_ladder_1 = _12832 + ((_11476 * (((1.0f - _11444) * _11432) + _11444)) * _11461);
                                                    frontier_phi_470_443_ladder_468_ladder_2 = _12831 + ((_11476 * (((1.0f - _11443) * _11432) + _11443)) * _11461);
                                                }
                                                frontier_phi_470_443_ladder = frontier_phi_470_443_ladder_468_ladder;
                                                frontier_phi_470_443_ladder_1 = frontier_phi_470_443_ladder_468_ladder_1;
                                                frontier_phi_470_443_ladder_2 = frontier_phi_470_443_ladder_468_ladder_2;
                                            }
                                            else
                                            {
                                                uint _10771 = _964 + 0u;
                                                float _10772 = dot(float3(_8635, _8636, _8637), float3(_954, _956, _958));
                                                float _10787 = dot(float3(_822, _827, _832), float3((_8636 * _958) - (_8637 * _956), (_8637 * _954) - (_8635 * _958), (_8635 * _956) - (_8636 * _954)));
                                                float _10796 = float(int(uint(_10787 > 0.0f) - uint(_10787 < 0.0f))) * sqrt(1.0f - (_10772 * _10772));
                                                float _10797 = _950 + (-0.5f);
                                                float _10798 = _952 + (-0.5f);
                                                float _10804 = mad(_10798, _10796, _10772 * _10797) + 0.5f;
                                                float _10805 = mad(_10798, _10772, (-0.0f) - (_10797 * _10796)) + 0.5f;
                                                float _10811 = (1.0f - clamp(dot(float3(_8635, _8636, _8637), float3(_822, _827, _832)), 0.0f, 1.0f)) * 5.0f;
                                                uint _10812 = uint(int(_10811));
                                                float4 _10819 = _41[NonUniformResourceIndex(_10771)].SampleLevel(_69, float3(_10804, _10805, float(int(_10812))), 0.0f);
                                                float _10821 = _10819.x;
                                                float _10830 = ((_41[NonUniformResourceIndex(_10771)].SampleLevel(_69, float3(_10804, _10805, min(float(int(_10812 + 1u)), 5.0f)), 0.0f).x - _10821) * frac(_10811)) + _10821;
                                                frontier_phi_470_443_ladder = (((_860 * 0.3183098733425140380859375f) * _960) * _962) * _10830;
                                                frontier_phi_470_443_ladder_1 = (((_855 * 0.3183098733425140380859375f) * _960) * _962) * _10830;
                                                frontier_phi_470_443_ladder_2 = (((_850 * 0.3183098733425140380859375f) * _960) * _962) * _10830;
                                            }
                                            _10843 = frontier_phi_470_443_ladder_2;
                                            _10847 = frontier_phi_470_443_ladder_1;
                                            _10851 = frontier_phi_470_443_ladder;
                                        }
                                        else
                                        {
                                            float _10220 = log2(max(1.0f - _921, 1.1754943508222875079687365372222e-38f)) * (-6862156513617824209436672.0f);
                                            float _10221 = _10220 * _10220;
                                            float _10223 = exp2(_10221 * (-225.4210968017578125f));
                                            float _10228 = exp2(_10221 * (-29.8077487945556640625f));
                                            float _10236 = exp2(_10221 * (-7.714946269989013671875f));
                                            float _10242 = exp2(_10221 * (-2.5444357395172119140625f));
                                            float _10244 = _10242 * 0.007000000216066837310791015625f;
                                            float _10249 = exp2(_10221 * (-0.72497236728668212890625f));
                                            float _10265 = max(dot(float3((-0.0f) - _822, (-0.0f) - _827, (-0.0f) - _832), float3(_8635, _8636, _8637)) + 0.300000011920928955078125f, 0.0f);
                                            _10843 = (((((((_10228 * 0.100000001490116119384765625f) + (_10223 * 0.23299999535083770751953125f)) + (_10236 * 0.1180000007152557373046875f)) + (_10242 * 0.112999998033046722412109375f)) + (_10249 * 0.3580000102519989013671875f)) + (exp2(_10221 * (-0.1946956813335418701171875f)) * 0.078000001609325408935546875f)) * _850) * _10265;
                                            _10847 = ((((((_10228 * 0.3359999954700469970703125f) + (_10223 * 0.4550000131130218505859375f)) + (_10236 * 0.19799999892711639404296875f)) + _10244) + (_10249 * 0.0040000001899898052215576171875f)) * _855) * _10265;
                                            _10851 = ((((_10228 * 0.3440000116825103759765625f) + (_10223 * 0.648999989032745361328125f)) + _10244) * _860) * _10265;
                                        }
                                        _9709 = _10843 * _9098;
                                        _9711 = _10847 * _9099;
                                        _9713 = _10851 * _9100;
                                    }
                                    frontier_phi_365_pred_364_ladder = _9713 + _8295;
                                    frontier_phi_365_pred_364_ladder_1 = _9711 + _8294;
                                    frontier_phi_365_pred_364_ladder_2 = _9709 + _8293;
                                    frontier_phi_365_pred_364_ladder_3 = ((_9538 + _9548) * _9100) + _8292;
                                    frontier_phi_365_pred_364_ladder_4 = ((_9532 + _9546) * _9099) + _8291;
                                    frontier_phi_365_pred_364_ladder_5 = ((_9526 + _9544) * _9098) + _8290;
                                    frontier_phi_365_pred_364_ladder_6 = (_9521 * _9100) + _8289;
                                    frontier_phi_365_pred_364_ladder_7 = (_9516 * _9099) + _8288;
                                    frontier_phi_365_pred_364_ladder_8 = (_9511 * _9098) + _8287;
                                }
                                else
                                {
                                    frontier_phi_365_pred_364_ladder = _8295;
                                    frontier_phi_365_pred_364_ladder_1 = _8294;
                                    frontier_phi_365_pred_364_ladder_2 = _8293;
                                    frontier_phi_365_pred_364_ladder_3 = _8292;
                                    frontier_phi_365_pred_364_ladder_4 = _8291;
                                    frontier_phi_365_pred_364_ladder_5 = _8290;
                                    frontier_phi_365_pred_364_ladder_6 = _8289;
                                    frontier_phi_365_pred_364_ladder_7 = _8288;
                                    frontier_phi_365_pred_364_ladder_8 = _8287;
                                }
                                frontier_phi_365_pred = frontier_phi_365_pred_364_ladder;
                                frontier_phi_365_pred_1 = frontier_phi_365_pred_364_ladder_1;
                                frontier_phi_365_pred_2 = frontier_phi_365_pred_364_ladder_2;
                                frontier_phi_365_pred_3 = frontier_phi_365_pred_364_ladder_3;
                                frontier_phi_365_pred_4 = frontier_phi_365_pred_364_ladder_4;
                                frontier_phi_365_pred_5 = frontier_phi_365_pred_364_ladder_5;
                                frontier_phi_365_pred_6 = frontier_phi_365_pred_364_ladder_6;
                                frontier_phi_365_pred_7 = frontier_phi_365_pred_364_ladder_7;
                                frontier_phi_365_pred_8 = frontier_phi_365_pred_364_ladder_8;
                            }
                            else
                            {
                                frontier_phi_365_pred = _8295;
                                frontier_phi_365_pred_1 = _8294;
                                frontier_phi_365_pred_2 = _8293;
                                frontier_phi_365_pred_3 = _8292;
                                frontier_phi_365_pred_4 = _8291;
                                frontier_phi_365_pred_5 = _8290;
                                frontier_phi_365_pred_6 = _8289;
                                frontier_phi_365_pred_7 = _8288;
                                frontier_phi_365_pred_8 = _8287;
                            }
                            _8189 = frontier_phi_365_pred;
                            _8188 = frontier_phi_365_pred_1;
                            _8187 = frontier_phi_365_pred_2;
                            _8186 = frontier_phi_365_pred_3;
                            _8185 = frontier_phi_365_pred_4;
                            _8184 = frontier_phi_365_pred_5;
                            _8183 = frontier_phi_365_pred_6;
                            _8181 = frontier_phi_365_pred_7;
                            _8179 = frontier_phi_365_pred_8;
                            if (_8191 > _7879)
                            {
                                break;
                            }
                            else
                            {
                                _8287 = _8179;
                                _8288 = _8181;
                                _8289 = _8183;
                                _8290 = _8184;
                                _8291 = _8185;
                                _8292 = _8186;
                                _8293 = _8187;
                                _8294 = _8188;
                                _8295 = _8189;
                                _8296 = _8191;
                                _8297 = _8300;
                                continue;
                            }
                        }
                        _8178 = _8179;
                        _8180 = _8181;
                        _8182 = _8183;
                        _7353 = _8184;
                        _7356 = _8185;
                        _7359 = _8186;
                        _7362 = _8187;
                        _7364 = _8188;
                        _7366 = _8189;
                        _8190 = _8191;
                        _8192 = _8300;
                    }
                    float _7344;
                    float _7347;
                    float _7350;
                    uint _8282;
                    uint _8284;
                    if (_8190 > _7880)
                    {
                        _7344 = _8178;
                        _7347 = _8180;
                        _7350 = _8182;
                        _8282 = _8190;
                        _8284 = _8192;
                    }
                    else
                    {
                        float _8279;
                        float _8280;
                        float _8281;
                        float _8427 = _8178;
                        float _8428 = _8180;
                        float _8429 = _8182;
                        uint _8430 = _8190;
                        uint _8431 = _8192;
                        uint _8283;
                        uint _8434;
                        uint _8449;
                        uint _8450;
                        float _8478;
                        float _8480;
                        float _8483;
                        float _8485;
                        uint _8487;
                        bool _8490;
                        float _8492;
                        float _8495;
                        float _8497;
                        float _8500;
                        float _8502;
                        float _8505;
                        float _8507;
                        bool _8512;
                        for (;;)
                        {
                            _8283 = _8430 + 1u;
                            _8434 = _45.Load(_8430).x;
                            uint _8436 = _8431 * 4u;
                            uint4 _8448 = uint4(_46.Load(_8436).x, _46.Load(_8436 + 1u).x, _46.Load(_8436 + 2u).x, _46.Load(_8436 + 3u).x);
                            _8449 = _8448.x;
                            _8450 = _8448.y;
                            uint _8451 = _8448.z;
                            uint _8452 = _8448.w;
                            uint _8454 = _8431 * 4u;
                            uint3 _8463 = uint3(_47.Load(_8454).x, _47.Load(_8454 + 1u).x, _47.Load(_8454 + 2u).x);
                            uint _8464 = _8463.x;
                            uint _8465 = _8463.y;
                            uint _8466 = _8463.z;
                            _8478 = spvUnpackHalf2x16(_8450 >> 16u).x;
                            _8480 = spvUnpackHalf2x16(_8451).x;
                            _8483 = spvUnpackHalf2x16(_8451 >> 16u).x;
                            _8485 = spvUnpackHalf2x16(_8452).x;
                            _8487 = (_8452 >> 16u) & 7u;
                            _8490 = (_8452 & 524288u) != 0u;
                            _8492 = spvUnpackHalf2x16(_8464).x;
                            _8495 = spvUnpackHalf2x16(_8464 >> 16u).x;
                            _8497 = spvUnpackHalf2x16(_8465).x;
                            _8500 = spvUnpackHalf2x16(_8465 >> 16u).x;
                            _8502 = spvUnpackHalf2x16(_8466).x;
                            _8505 = spvUnpackHalf2x16(_8466 >> 16u).x;
                            _8507 = spvUnpackHalf2x16(uint3(_8469, _8470, _48.Load((_8431 * 4u) + 2u).x).z).x;
                            _8512 = (_8452 < 3221225472u) && (((_499 & 255u) & (_8452 >> 22u)) != 0u);
                            float frontier_phi_371_pred;
                            float frontier_phi_371_pred_1;
                            float frontier_phi_371_pred_2;
                            if (_8512)
                            {
                                float _8616 = spvUnpackHalf2x16(_8449).x - _7864;
                                float _8617 = spvUnpackHalf2x16(_8449 >> 16u).x - _7865;
                                float _8618 = spvUnpackHalf2x16(_8450).x - _7866;
                                float _8624 = sqrt(((_8617 * _8617) + (_8618 * _8618)) + (_8616 * _8616));
                                float _8625 = _8624 * _8478;
                                float frontier_phi_371_pred_370_ladder;
                                float frontier_phi_371_pred_370_ladder_1;
                                float frontier_phi_371_pred_370_ladder_2;
                                if (_8625 < 1.0f)
                                {
                                    float _8771 = rsqrt(dot(float3(_8616, _8617, _8618), float3(_8616, _8617, _8618)));
                                    float _8772 = _8771 * _8616;
                                    float _8773 = _8771 * _8617;
                                    float _8774 = _8771 * _8618;
                                    float _8775 = _8624 * _8624;
                                    float _8777 = (_8478 * _8478) * _8775;
                                    float _8780 = clamp(1.0f - (_8777 * _8777), 0.0f, 1.0f);
                                    float _9013;
                                    if (_8487 == 0u)
                                    {
                                        _9013 = (_8780 * _8780) * (1.0f / (max(_8775, 9.9999997473787516355514526367188e-05f) + ((_8507 * _8507) * 0.5f)));
                                    }
                                    else
                                    {
                                        _9013 = max((1.0f / dot(float3(1.0f, _8625, _8625 * _8625), float3(_62_m0[_8487 + 60u].xyz))) * (1.0f - _8625), 0.0f);
                                    }
                                    float _9014 = (-0.0f) - _8492;
                                    float _9038 = clamp((clamp(dot(float3(((_8500 * _8495) - (_8497 * _9014)) * 2.0f, ((_8497 * _8495) - (_8500 * _8492)) * 2.0f, (((_8492 * _9014) - (_8495 * _8495)) * 2.0f) + 1.0f), float3((-0.0f) - _8772, (-0.0f) - _8773, (-0.0f) - _8774)), 0.0f, 1.0f) - _8505) / (_8502 - _8505), 0.0f, 1.0f);
                                    float _9052 = (((_9038 * _9038) * _9013) * max(float(_8490) * 16.0f, 1.0f)) * (3.0f - (_9038 * 2.0f));
                                    float _9056 = (_8478 < 0.02857142873108386993408203125f) ? _62_m0[104u].w : 0.0f;
                                    float _9064 = 1.0f - _9056;
                                    float _9068 = clamp(((_9064 * clamp(dot(float3(_822, _827, _832), float3(_8772, _8773, _8774)), 0.0f, 1.0f)) + _9056) * _9064, 0.0f, 1.0f);
                                    frontier_phi_371_pred_370_ladder = ((_9068 * ((_860 * 0.3183098733425140380859375f) * (1.0f - _873))) * (_9052 * _8485)) + _8429;
                                    frontier_phi_371_pred_370_ladder_1 = ((_9068 * ((_855 * 0.3183098733425140380859375f) * (1.0f - _870))) * (_9052 * _8483)) + _8428;
                                    frontier_phi_371_pred_370_ladder_2 = ((_9068 * ((_850 * 0.3183098733425140380859375f) * (1.0f - _865))) * (_9052 * _8480)) + _8427;
                                }
                                else
                                {
                                    frontier_phi_371_pred_370_ladder = _8429;
                                    frontier_phi_371_pred_370_ladder_1 = _8428;
                                    frontier_phi_371_pred_370_ladder_2 = _8427;
                                }
                                frontier_phi_371_pred = frontier_phi_371_pred_370_ladder;
                                frontier_phi_371_pred_1 = frontier_phi_371_pred_370_ladder_1;
                                frontier_phi_371_pred_2 = frontier_phi_371_pred_370_ladder_2;
                            }
                            else
                            {
                                frontier_phi_371_pred = _8429;
                                frontier_phi_371_pred_1 = _8428;
                                frontier_phi_371_pred_2 = _8427;
                            }
                            _8281 = frontier_phi_371_pred;
                            _8280 = frontier_phi_371_pred_1;
                            _8279 = frontier_phi_371_pred_2;
                            if (_8283 > _7880)
                            {
                                break;
                            }
                            else
                            {
                                _8427 = _8279;
                                _8428 = _8280;
                                _8429 = _8281;
                                _8430 = _8283;
                                _8431 = _8434;
                                continue;
                            }
                        }
                        _7344 = _8279;
                        _7347 = _8280;
                        _7350 = _8281;
                        _8282 = _8283;
                        _8284 = _8434;
                    }
                    float _7368;
                    float _7370;
                    float _7372;
                    uint _8422;
                    uint _8424;
                    if (_8282 > _7881)
                    {
                        _7368 = 0.0f;
                        _7370 = 0.0f;
                        _7372 = 0.0f;
                        _8422 = _8282;
                        _8424 = _8284;
                    }
                    else
                    {
                        float _8419;
                        float _8420;
                        float _8421;
                        float _8555 = 0.0f;
                        float _8556 = 0.0f;
                        float _8557 = 0.0f;
                        uint _8558 = _8282;
                        uint _8559 = _8284;
                        uint _8423;
                        uint _8562;
                        float _8592;
                        float _8595;
                        float _8597;
                        float _8600;
                        float _8604;
                        float _8607;
                        bool _8608;
                        for (;;)
                        {
                            _8423 = _8558 + 1u;
                            _8562 = _45.Load(_8558).x;
                            uint _8564 = _8559 * 4u;
                            uint4 _8576 = uint4(_46.Load(_8564).x, _46.Load(_8564 + 1u).x, _46.Load(_8564 + 2u).x, _46.Load(_8564 + 3u).x);
                            uint _8577 = _8576.x;
                            uint _8578 = _8576.y;
                            uint _8579 = _8576.z;
                            uint _8580 = _8576.w;
                            _8592 = spvUnpackHalf2x16(_8579).x;
                            _8595 = spvUnpackHalf2x16(_8579 >> 16u).x;
                            _8597 = spvUnpackHalf2x16(_8580).x;
                            _8600 = spvUnpackHalf2x16(_8580 >> 16u).x;
                            float _8601 = spvUnpackHalf2x16(_8577).x - _7864;
                            float _8602 = spvUnpackHalf2x16(_8577 >> 16u).x - _7865;
                            float _8603 = spvUnpackHalf2x16(_8578).x - _7866;
                            _8604 = dot(float3(_8601, _8602, _8603), float3(_8601, _8602, _8603));
                            _8607 = _8604 * spvUnpackHalf2x16(_8578 >> 16u).x;
                            _8608 = _8607 < 1.0f;
                            float frontier_phi_378_pred;
                            float frontier_phi_378_pred_1;
                            float frontier_phi_378_pred_2;
                            if (_8608)
                            {
                                float _9009;
                                if (_8600 != 0.0f)
                                {
                                    _9009 = (1.0f / ((_8607 * _8600) + 1.0f)) * (1.0f - _8607);
                                }
                                else
                                {
                                    float _8911 = clamp(1.0f - (_8607 * _8607), 0.0f, 1.0f);
                                    _9009 = (_8911 * _8911) * (1.0f / max(_8604, 9.9999997473787516355514526367188e-05f));
                                }
                                frontier_phi_378_pred = (_9009 * _8597) + _8557;
                                frontier_phi_378_pred_1 = (_9009 * _8595) + _8556;
                                frontier_phi_378_pred_2 = (_9009 * _8592) + _8555;
                            }
                            else
                            {
                                frontier_phi_378_pred = _8557;
                                frontier_phi_378_pred_1 = _8556;
                                frontier_phi_378_pred_2 = _8555;
                            }
                            _8421 = frontier_phi_378_pred;
                            _8420 = frontier_phi_378_pred_1;
                            _8419 = frontier_phi_378_pred_2;
                            if (_8423 > _7881)
                            {
                                break;
                            }
                            else
                            {
                                _8555 = _8419;
                                _8556 = _8420;
                                _8557 = _8421;
                                _8558 = _8423;
                                _8559 = _8562;
                                continue;
                            }
                        }
                        _7368 = _8419;
                        _7370 = _8420;
                        _7372 = _8421;
                        _8422 = _8423;
                        _8424 = _8562;
                    }
                    float frontier_phi_319_320_ladder_361_ladder;
                    float frontier_phi_319_320_ladder_361_ladder_1;
                    float frontier_phi_319_320_ladder_361_ladder_2;
                    float frontier_phi_319_320_ladder_361_ladder_3;
                    float frontier_phi_319_320_ladder_361_ladder_4;
                    float frontier_phi_319_320_ladder_361_ladder_5;
                    float frontier_phi_319_320_ladder_361_ladder_6;
                    float frontier_phi_319_320_ladder_361_ladder_7;
                    float frontier_phi_319_320_ladder_361_ladder_8;
                    float frontier_phi_319_320_ladder_361_ladder_9;
                    float frontier_phi_319_320_ladder_361_ladder_10;
                    float frontier_phi_319_320_ladder_361_ladder_11;
                    if (_8422 > _7882)
                    {
                        frontier_phi_319_320_ladder_361_ladder = _7372;
                        frontier_phi_319_320_ladder_361_ladder_1 = _7344;
                        frontier_phi_319_320_ladder_361_ladder_2 = _7347;
                        frontier_phi_319_320_ladder_361_ladder_3 = _7350;
                        frontier_phi_319_320_ladder_361_ladder_4 = _7353;
                        frontier_phi_319_320_ladder_361_ladder_5 = _7356;
                        frontier_phi_319_320_ladder_361_ladder_6 = _7359;
                        frontier_phi_319_320_ladder_361_ladder_7 = _7362;
                        frontier_phi_319_320_ladder_361_ladder_8 = _7364;
                        frontier_phi_319_320_ladder_361_ladder_9 = _7366;
                        frontier_phi_319_320_ladder_361_ladder_10 = _7368;
                        frontier_phi_319_320_ladder_361_ladder_11 = _7370;
                    }
                    else
                    {
                        float _7345;
                        float _7348;
                        float _7351;
                        float _7354;
                        float _7357;
                        float _7360;
                        float _8666 = _7344;
                        float _8667 = _7347;
                        float _8668 = _7350;
                        float _8669 = _7353;
                        float _8670 = _7356;
                        float _8671 = _7359;
                        uint _8672 = _8422;
                        uint _8674 = _8424;
                        uint _8673;
                        uint _8678;
                        uint _8693;
                        uint _8694;
                        float _8726;
                        float _8728;
                        float _8731;
                        float _8733;
                        uint _8735;
                        bool _8738;
                        float _8740;
                        float _8743;
                        float _8745;
                        float _8748;
                        float _8750;
                        float _8753;
                        float _8755;
                        float _8757;
                        bool _8762;
                        for (;;)
                        {
                            _8673 = _8672 + 1u;
                            _8678 = _45.Load(_8672).x;
                            uint _8680 = _8674 * 4u;
                            uint4 _8692 = uint4(_46.Load(_8680).x, _46.Load(_8680 + 1u).x, _46.Load(_8680 + 2u).x, _46.Load(_8680 + 3u).x);
                            _8693 = _8692.x;
                            _8694 = _8692.y;
                            uint _8695 = _8692.z;
                            uint _8696 = _8692.w;
                            uint _8698 = _8674 * 4u;
                            uint4 _8710 = uint4(_47.Load(_8698).x, _47.Load(_8698 + 1u).x, _47.Load(_8698 + 2u).x, _47.Load(_8698 + 3u).x);
                            uint _8711 = _8710.x;
                            uint _8712 = _8710.y;
                            uint _8713 = _8710.z;
                            _8726 = spvUnpackHalf2x16(_8694 >> 16u).x;
                            _8728 = spvUnpackHalf2x16(_8695).x;
                            _8731 = spvUnpackHalf2x16(_8695 >> 16u).x;
                            _8733 = spvUnpackHalf2x16(_8696).x;
                            _8735 = (_8696 >> 16u) & 7u;
                            _8738 = (_8696 & 524288u) != 0u;
                            _8740 = spvUnpackHalf2x16(_8711).x;
                            _8743 = spvUnpackHalf2x16(_8711 >> 16u).x;
                            _8745 = spvUnpackHalf2x16(_8712).x;
                            _8748 = spvUnpackHalf2x16(_8712 >> 16u).x;
                            _8750 = spvUnpackHalf2x16(_8713).x;
                            _8753 = spvUnpackHalf2x16(_8713 >> 16u).x;
                            _8755 = spvUnpackHalf2x16(_8710.w).x;
                            _8757 = spvUnpackHalf2x16(uint3(_8717, _8718, _48.Load((_8674 * 4u) + 2u).x).z).x;
                            _8762 = (_8696 < 3221225472u) && (((_499 & 255u) & (_8696 >> 22u)) != 0u);
                            float frontier_phi_385_pred;
                            float frontier_phi_385_pred_1;
                            float frontier_phi_385_pred_2;
                            float frontier_phi_385_pred_3;
                            float frontier_phi_385_pred_4;
                            float frontier_phi_385_pred_5;
                            if (_8762)
                            {
                                float _8820 = (-0.0f) - _8745;
                                float _8826 = (_8745 * _8820) - (_8740 * _8740);
                                float _8827 = _8748 * _8740;
                                float _8832 = (-0.0f) - _8740;
                                float _8845 = _8755 * ((_8743 * _8740) - (_8748 * _8745));
                                float _8848 = _8755 * (_8827 - (_8743 * _8820));
                                float _8849 = spvUnpackHalf2x16(_8693).x - _8845;
                                float _8850 = spvUnpackHalf2x16(_8693 >> 16u).x - (_8755 * (_8826 + 0.5f));
                                float _8851 = spvUnpackHalf2x16(_8694).x - _8848;
                                float _8855 = _8845 * 2.0f;
                                float _8856 = _8755 * ((_8826 * 2.0f) + 1.0f);
                                float _8857 = _8848 * 2.0f;
                                float _8865 = clamp(dot(float3(_8855, _8856, _8857), float3(_7864 - _8849, _7865 - _8850, _7866 - _8851)) / dot(float3(_8855, _8856, _8857), float3(_8855, _8856, _8857)), 0.0f, 1.0f);
                                float _8870 = (_8865 * _8855) + (_8849 - _7864);
                                float _8872 = (_8865 * _8856) + (_8850 - _7865);
                                float _8874 = (_8865 * _8857) + (_8851 - _7866);
                                float _8878 = rsqrt(dot(float3(_8870, _8872, _8874), float3(_8870, _8872, _8874)));
                                float _8879 = _8870 * _8878;
                                float _8880 = _8872 * _8878;
                                float _8881 = _8874 * _8878;
                                float _8887 = sqrt(((_8870 * _8870) + (_8872 * _8872)) + (_8874 * _8874));
                                float _8888 = _8887 * _8887;
                                float _8890 = (_8726 * _8726) * _8888;
                                float _8893 = clamp(1.0f - (_8890 * _8890), 0.0f, 1.0f);
                                float _9153;
                                if (_8735 == 0u)
                                {
                                    _9153 = (_8893 * _8893) * (1.0f / (max(_8888, 9.9999997473787516355514526367188e-05f) + ((_8757 * _8757) * 0.5f)));
                                }
                                else
                                {
                                    float _8994 = _8887 * _8726;
                                    _9153 = max((1.0f / dot(float3(1.0f, _8994, _8994 * _8994), float3(_62_m0[_8735 + 60u].xyz))) * (1.0f - _8994), 0.0f);
                                }
                                float frontier_phi_385_pred_404_ladder;
                                float frontier_phi_385_pred_404_ladder_1;
                                float frontier_phi_385_pred_404_ladder_2;
                                float frontier_phi_385_pred_404_ladder_3;
                                float frontier_phi_385_pred_404_ladder_4;
                                float frontier_phi_385_pred_404_ladder_5;
                                if (_9153 < 9.9999997473787516355514526367188e-06f)
                                {
                                    frontier_phi_385_pred_404_ladder = _8668;
                                    frontier_phi_385_pred_404_ladder_1 = _8667;
                                    frontier_phi_385_pred_404_ladder_2 = _8666;
                                    frontier_phi_385_pred_404_ladder_3 = _8671;
                                    frontier_phi_385_pred_404_ladder_4 = _8670;
                                    frontier_phi_385_pred_404_ladder_5 = _8669;
                                }
                                else
                                {
                                    float _9374 = clamp((clamp(dot(float3(((_8748 * _8743) - (_8745 * _8832)) * 2.0f, ((_8745 * _8743) - _8827) * 2.0f, (((_8740 * _8832) - (_8743 * _8743)) * 2.0f) + 1.0f), float3((-0.0f) - _8879, (-0.0f) - _8880, (-0.0f) - _8881)), 0.0f, 1.0f) - _8753) / (_8750 - _8753), 0.0f, 1.0f);
                                    float _9378 = (_9374 * _9374) * (3.0f - (_9374 * 2.0f));
                                    float _9380 = (_9378 * _9378) * _9153;
                                    float frontier_phi_385_pred_404_ladder_412_ladder;
                                    float frontier_phi_385_pred_404_ladder_412_ladder_1;
                                    float frontier_phi_385_pred_404_ladder_412_ladder_2;
                                    float frontier_phi_385_pred_404_ladder_412_ladder_3;
                                    float frontier_phi_385_pred_404_ladder_412_ladder_4;
                                    float frontier_phi_385_pred_404_ladder_412_ladder_5;
                                    if (_9380 < 9.9999997473787516355514526367188e-06f)
                                    {
                                        frontier_phi_385_pred_404_ladder_412_ladder = _8668;
                                        frontier_phi_385_pred_404_ladder_412_ladder_1 = _8667;
                                        frontier_phi_385_pred_404_ladder_412_ladder_2 = _8666;
                                        frontier_phi_385_pred_404_ladder_412_ladder_3 = _8671;
                                        frontier_phi_385_pred_404_ladder_412_ladder_4 = _8670;
                                        frontier_phi_385_pred_404_ladder_412_ladder_5 = _8669;
                                    }
                                    else
                                    {
                                        float _9621 = (_9380 * _6920) * max(float(_8738) * 16.0f, 1.0f);
                                        float _9622 = _9621 * _8728;
                                        float _9623 = _9621 * _8731;
                                        float _9624 = _9621 * _8733;
                                        float _9630 = clamp(clamp(dot(float3(_822, _827, _832), float3(_8879, _8880, _8881)), 0.0f, 1.0f), 0.0f, 1.0f) * 0.3183098733425140380859375f;
                                        float _9638 = (_57_m0[122u].w * _948) * _9630;
                                        frontier_phi_385_pred_404_ladder_412_ladder = ((((1.0f - _873) * _860) * _9624) * _9630) + _8668;
                                        frontier_phi_385_pred_404_ladder_412_ladder_1 = ((((1.0f - _870) * _855) * _9623) * _9630) + _8667;
                                        frontier_phi_385_pred_404_ladder_412_ladder_2 = ((((1.0f - _865) * _850) * _9622) * _9630) + _8666;
                                        frontier_phi_385_pred_404_ladder_412_ladder_3 = ((_9638 * _873) * _9624) + _8671;
                                        frontier_phi_385_pred_404_ladder_412_ladder_4 = ((_9638 * _870) * _9623) + _8670;
                                        frontier_phi_385_pred_404_ladder_412_ladder_5 = ((_9638 * _865) * _9622) + _8669;
                                    }
                                    frontier_phi_385_pred_404_ladder = frontier_phi_385_pred_404_ladder_412_ladder;
                                    frontier_phi_385_pred_404_ladder_1 = frontier_phi_385_pred_404_ladder_412_ladder_1;
                                    frontier_phi_385_pred_404_ladder_2 = frontier_phi_385_pred_404_ladder_412_ladder_2;
                                    frontier_phi_385_pred_404_ladder_3 = frontier_phi_385_pred_404_ladder_412_ladder_3;
                                    frontier_phi_385_pred_404_ladder_4 = frontier_phi_385_pred_404_ladder_412_ladder_4;
                                    frontier_phi_385_pred_404_ladder_5 = frontier_phi_385_pred_404_ladder_412_ladder_5;
                                }
                                frontier_phi_385_pred = frontier_phi_385_pred_404_ladder;
                                frontier_phi_385_pred_1 = frontier_phi_385_pred_404_ladder_1;
                                frontier_phi_385_pred_2 = frontier_phi_385_pred_404_ladder_2;
                                frontier_phi_385_pred_3 = frontier_phi_385_pred_404_ladder_3;
                                frontier_phi_385_pred_4 = frontier_phi_385_pred_404_ladder_4;
                                frontier_phi_385_pred_5 = frontier_phi_385_pred_404_ladder_5;
                            }
                            else
                            {
                                frontier_phi_385_pred = _8668;
                                frontier_phi_385_pred_1 = _8667;
                                frontier_phi_385_pred_2 = _8666;
                                frontier_phi_385_pred_3 = _8671;
                                frontier_phi_385_pred_4 = _8670;
                                frontier_phi_385_pred_5 = _8669;
                            }
                            _7351 = frontier_phi_385_pred;
                            _7348 = frontier_phi_385_pred_1;
                            _7345 = frontier_phi_385_pred_2;
                            _7360 = frontier_phi_385_pred_3;
                            _7357 = frontier_phi_385_pred_4;
                            _7354 = frontier_phi_385_pred_5;
                            if (_8673 > _7882)
                            {
                                break;
                            }
                            else
                            {
                                _8666 = _7345;
                                _8667 = _7348;
                                _8668 = _7351;
                                _8669 = _7354;
                                _8670 = _7357;
                                _8671 = _7360;
                                _8672 = _8673;
                                _8674 = _8678;
                                continue;
                            }
                        }
                        frontier_phi_319_320_ladder_361_ladder = _7372;
                        frontier_phi_319_320_ladder_361_ladder_1 = _7345;
                        frontier_phi_319_320_ladder_361_ladder_2 = _7348;
                        frontier_phi_319_320_ladder_361_ladder_3 = _7351;
                        frontier_phi_319_320_ladder_361_ladder_4 = _7354;
                        frontier_phi_319_320_ladder_361_ladder_5 = _7357;
                        frontier_phi_319_320_ladder_361_ladder_6 = _7360;
                        frontier_phi_319_320_ladder_361_ladder_7 = _7362;
                        frontier_phi_319_320_ladder_361_ladder_8 = _7364;
                        frontier_phi_319_320_ladder_361_ladder_9 = _7366;
                        frontier_phi_319_320_ladder_361_ladder_10 = _7368;
                        frontier_phi_319_320_ladder_361_ladder_11 = _7370;
                    }
                    frontier_phi_319_320_ladder = frontier_phi_319_320_ladder_361_ladder;
                    frontier_phi_319_320_ladder_1 = frontier_phi_319_320_ladder_361_ladder_1;
                    frontier_phi_319_320_ladder_2 = frontier_phi_319_320_ladder_361_ladder_2;
                    frontier_phi_319_320_ladder_3 = frontier_phi_319_320_ladder_361_ladder_3;
                    frontier_phi_319_320_ladder_4 = frontier_phi_319_320_ladder_361_ladder_4;
                    frontier_phi_319_320_ladder_5 = frontier_phi_319_320_ladder_361_ladder_5;
                    frontier_phi_319_320_ladder_6 = frontier_phi_319_320_ladder_361_ladder_6;
                    frontier_phi_319_320_ladder_7 = frontier_phi_319_320_ladder_361_ladder_7;
                    frontier_phi_319_320_ladder_8 = frontier_phi_319_320_ladder_361_ladder_8;
                    frontier_phi_319_320_ladder_9 = frontier_phi_319_320_ladder_361_ladder_9;
                    frontier_phi_319_320_ladder_10 = frontier_phi_319_320_ladder_361_ladder_10;
                    frontier_phi_319_320_ladder_11 = frontier_phi_319_320_ladder_361_ladder_11;
                }
                _7343 = frontier_phi_319_320_ladder_1;
                _7346 = frontier_phi_319_320_ladder_2;
                _7349 = frontier_phi_319_320_ladder_3;
                _7352 = frontier_phi_319_320_ladder_4;
                _7355 = frontier_phi_319_320_ladder_5;
                _7358 = frontier_phi_319_320_ladder_6;
                _7361 = frontier_phi_319_320_ladder_7;
                _7363 = frontier_phi_319_320_ladder_8;
                _7365 = frontier_phi_319_320_ladder_9;
                _7367 = frontier_phi_319_320_ladder_10;
                _7369 = frontier_phi_319_320_ladder_11;
                _7371 = frontier_phi_319_320_ladder;
            }
            float _7374 = (_2221 * _4409) * ((_57_m0[98u].x * (_1952 + (-1.0f))) + 1.0f);
            float _7375 = _1958 * _4409;
            float _8007;
            float _8010;
            float _8013;
            if ((_989 & 4194304u) == 0u)
            {
                float frontier_phi_342_332_ladder;
                float frontier_phi_342_332_ladder_1;
                float frontier_phi_342_332_ladder_2;
                if ((_989 & 67108864u) == 0u)
                {
                    float frontier_phi_342_332_ladder_340_ladder;
                    float frontier_phi_342_332_ladder_340_ladder_1;
                    float frontier_phi_342_332_ladder_340_ladder_2;
                    if ((_989 & 50331648u) == 0u)
                    {
                        frontier_phi_342_332_ladder_340_ladder = (1.0f - _873) * _860;
                        frontier_phi_342_332_ladder_340_ladder_1 = (1.0f - _870) * _855;
                        frontier_phi_342_332_ladder_340_ladder_2 = (1.0f - _865) * _850;
                    }
                    else
                    {
                        float _8153 = ((_876 * 0.75f) + 1.25f) + ((((_978 * _978) * 9000.0f) * _976) * _978);
                        float _8177 = (_982 * 0.3183098733425140380859375f) * (sqrt(((_972 * _972) + (_970 * _970)) + (_974 * _974)) + 1.0f);
                        frontier_phi_342_332_ladder_340_ladder = exp2(log2(exp2(_974 * (-4.616624355316162109375f))) * _8153) * _8177;
                        frontier_phi_342_332_ladder_340_ladder_1 = exp2(log2(exp2(_972 * (-4.616624355316162109375f))) * _8153) * _8177;
                        frontier_phi_342_332_ladder_340_ladder_2 = exp2(log2(exp2(_970 * (-4.616624355316162109375f))) * _8153) * _8177;
                    }
                    frontier_phi_342_332_ladder = frontier_phi_342_332_ladder_340_ladder;
                    frontier_phi_342_332_ladder_1 = frontier_phi_342_332_ladder_340_ladder_1;
                    frontier_phi_342_332_ladder_2 = frontier_phi_342_332_ladder_340_ladder_2;
                }
                else
                {
                    float _8003 = (((_37[NonUniformResourceIndex(_966 + 0u)].SampleLevel(_69, float2(_950, _952), 0.0f).x * _962) + (-1.0f)) * _960) + 1.0f;
                    frontier_phi_342_332_ladder = ((1.0f - _873) * _860) * _8003;
                    frontier_phi_342_332_ladder_1 = ((1.0f - _870) * _855) * _8003;
                    frontier_phi_342_332_ladder_2 = ((1.0f - _865) * _850) * _8003;
                }
                _8007 = frontier_phi_342_332_ladder_2;
                _8010 = frontier_phi_342_332_ladder_1;
                _8013 = frontier_phi_342_332_ladder;
            }
            else
            {
                _8007 = ((1.0f - _865) - (_929 * (_923 - _865))) * _850;
                _8010 = ((1.0f - _870) - (_929 * (_925 - _870))) * _855;
                _8013 = ((1.0f - _873) - (_929 * (_927 - _873))) * _860;
            }
            _4320 = ((_6509 * (_6494 + _5425)) + ((_7367 * _7375) * _8007)) + ((_7361 + _7343) * _7374);
            _4322 = ((_6510 * (_6499 + _5430)) + ((_7369 * _7375) * _8010)) + ((_7363 + _7346) * _7374);
            _4324 = ((_6511 * (_6504 + _5435)) + ((_7371 * _7375) * _8013)) + ((_7365 + _7349) * _7374);
            _4326 = (_7352 * _7374) + ((_5458 + _5440) * _6509);
            _4328 = (_7355 * _7374) + ((_5458 + _5446) * _6510);
            _4330 = (_7358 * _7374) + ((_5458 + _5452) * _6511);
        }
        float _4336 = ((((((_2673 * (1.0f / (1.0f - (_1966 * min(0.999000012874603271484375f, _942))))) * (_2356 - (_2356 * _505))) + (_2265 * _2212)) + _2660) + _4090) + _4320) + _4326;
        float _4341 = ((((((_2673 * (1.0f / (1.0f - (_1966 * min(0.999000012874603271484375f, _944))))) * (_2367 - (_2367 * _505))) + (_2270 * _2213)) + _2663) + _4092) + _4322) + _4328;
        float _4346 = ((((((_2673 * (1.0f / (1.0f - (_1966 * min(0.999000012874603271484375f, _946))))) * (_2378 - (_2378 * _505))) + (_2275 * _2214)) + _2666) + _4094) + _4324) + _4330;
        float _4354 = _369 - _57_m0[8u].x;
        float _4355 = _370 - _57_m0[8u].y;
        float _4356 = _371 - _57_m0[8u].z;
        float _4360 = rsqrt(dot(float3(_4354, _4355, _4356), float3(_4354, _4355, _4356)));
        float _4366 = sqrt(((_4354 * _4354) + (_4355 * _4355)) + (_4356 * _4356));
        float _4376 = (_369 - _57_m0[111u].x) * _57_m0[111u].z;
        float _4378 = 1.0f - ((_371 - _57_m0[111u].y) * _57_m0[111u].w);
        float _4385 = _57_m0[112u].x - _370;
        float _4397 = _62_m0[53u].z - _62_m0[53u].y;
        float _4402 = 1.0f - exp2((-0.0f) - (_62_m0[53u].w * min(1000000.0f, max(0.0f, _4366 - _62_m0[53u].x))));
        bool _4407 = _62_m0[54u].y > 0.0f;
        float _1096;
        float _5232;
        float _5234;
        float _5236;
        if ((asuint(_62_m0[_1144]).z != 0u) && _992)
        {
            float4 _4639 = _15[584u].SampleLevel(_65, float2(_4376, _4378), 0.0f);
            float _4649 = _4360 * _4355;
            float _4653 = (((clamp(_4639.y, 0.0f, 1.0f) * _4397) * clamp(((_4385 - _57_m0[112u].w) + (_4639.x * _57_m0[112u].y)) / _57_m0[112u].z, 0.0f, 1.0f)) + _62_m0[53u].y) * _4402;
            float _4979;
            if (_4407)
            {
                float _4977 = min(1000000.0f, max(0.0f, _4366 - _62_m0[54u].w)) * 0.001000000047497451305389404296875f;
                float _5220;
                if (_4649 == 0.0f)
                {
                    _5220 = _4977;
                }
                else
                {
                    float _5225 = _62_m0[54u].y * _4649;
                    _5220 = (1.0f - exp2((-0.0f) - (_5225 * _4977))) / _5225;
                }
                _4979 = clamp((_5220 * _62_m0[54u].x) + _4653, 0.0f, 1.0f);
            }
            else
            {
                _4979 = _4653;
            }
            float frontier_phi_253_236_ladder;
            float frontier_phi_253_236_ladder_1;
            float frontier_phi_253_236_ladder_2;
            float frontier_phi_253_236_ladder_3;
            if (asuint(_62_m0[152u]).w == 0u)
            {
                frontier_phi_253_236_ladder = _4346;
                frontier_phi_253_236_ladder_1 = _4979;
                frontier_phi_253_236_ladder_2 = _4336;
                frontier_phi_253_236_ladder_3 = _4341;
            }
            else
            {
                float _5260 = ((-0.0f) - _62_m0[151u].z) / (_62_m0[150u].w * ((1.0f - _293) - _62_m0[151u].w));
                float _5955;
                if (_62_m0[152u].z < 0.001000000047497451305389404296875f)
                {
                    _5955 = _5260;
                }
                else
                {
                    _5955 = log2((_62_m0[152u].z * _5260) + 1.0f) / log2(_62_m0[152u].z + 1.0f);
                }
                frontier_phi_253_236_ladder = _4346;
                frontier_phi_253_236_ladder_1 = (_51.SampleLevel(_66, float3(_296 / (_62_m0[151u].x + (-1.0f)), _298 / (_62_m0[151u].y + (-1.0f)), _5955), 0.0f).w * (_4979 + (-1.0f))) + 1.0f;
                frontier_phi_253_236_ladder_2 = _4336;
                frontier_phi_253_236_ladder_3 = _4341;
            }
            _1096 = frontier_phi_253_236_ladder_1;
            _5232 = frontier_phi_253_236_ladder_2;
            _5234 = frontier_phi_253_236_ladder_3;
            _5236 = frontier_phi_253_236_ladder;
        }
        else
        {
            float _4655 = _4355 * _4360;
            float4 _4660 = _15[584u].SampleLevel(_65, float2(_4376, _4378), 0.0f);
            float4 _4685 = _24.SampleLevel(_68, float3(_4354 * _4360, _4655, _4356 * _4360), (1.0f - clamp((_4366 - _62_m0[55u].x) / (_62_m0[55u].y - _62_m0[55u].x), 0.0f, 1.0f)) * _62_m0[55u].z);
            float _4699 = (((clamp(_4660.y, 0.0f, 1.0f) * _4397) * clamp(((_4385 - _57_m0[112u].w) + (_4660.x * _57_m0[112u].y)) / _57_m0[112u].z, 0.0f, 1.0f)) + _62_m0[53u].y) * _4402;
            float _4993;
            if (_4407)
            {
                float _4991 = min(1000000.0f, max(0.0f, _4366 - _62_m0[54u].w)) * 0.001000000047497451305389404296875f;
                float _5265;
                if (_4655 == 0.0f)
                {
                    _5265 = _4991;
                }
                else
                {
                    float _5270 = _62_m0[54u].y * _4655;
                    _5265 = (1.0f - exp2((-0.0f) - (_5270 * _4991))) / _5270;
                }
                _4993 = clamp((_5265 * _62_m0[54u].x) + _4699, 0.0f, 1.0f);
            }
            else
            {
                _4993 = _4699;
            }
            float _5001 = (_4993 * ((_57_m0[59u].z * _4685.x) - _4336)) + _4336;
            float _5002 = (_4993 * ((_57_m0[59u].z * _4685.y) - _4341)) + _4341;
            float _5003 = (_4993 * ((_57_m0[59u].z * _4685.z) - _4346)) + _4346;
            float frontier_phi_253_238_ladder;
            float frontier_phi_253_238_ladder_1;
            float frontier_phi_253_238_ladder_2;
            float frontier_phi_253_238_ladder_3;
            if (asuint(_62_m0[152u]).w == 0u)
            {
                frontier_phi_253_238_ladder = _5003;
                frontier_phi_253_238_ladder_1 = _4993;
                frontier_phi_253_238_ladder_2 = _5001;
                frontier_phi_253_238_ladder_3 = _5002;
            }
            else
            {
                float _5292 = ((-0.0f) - _62_m0[151u].z) / (_62_m0[150u].w * ((1.0f - _293) - _62_m0[151u].w));
                float _5970;
                if (_62_m0[152u].z < 0.001000000047497451305389404296875f)
                {
                    _5970 = _5292;
                }
                else
                {
                    _5970 = log2((_62_m0[152u].z * _5292) + 1.0f) / log2(_62_m0[152u].z + 1.0f);
                }
                float4 _5975 = _51.SampleLevel(_66, float3(_296 / (_62_m0[151u].x + (-1.0f)), _298 / (_62_m0[151u].y + (-1.0f)), _5970), 0.0f);
                float _5980 = _5975.w;
                frontier_phi_253_238_ladder = (_5975.z / _62_m0[152u].x) + (_5980 * _5003);
                frontier_phi_253_238_ladder_1 = (_5980 * (_4993 + (-1.0f))) + 1.0f;
                frontier_phi_253_238_ladder_2 = (_5980 * _5001) + (_5975.x / _62_m0[152u].x);
                frontier_phi_253_238_ladder_3 = (_5975.y / _62_m0[152u].x) + (_5980 * _5002);
            }
            _1096 = frontier_phi_253_238_ladder_1;
            _5232 = frontier_phi_253_238_ladder_2;
            _5234 = frontier_phi_253_238_ladder_3;
            _5236 = frontier_phi_253_238_ladder;
        }
        _1095 = _1096;
        _1097 = _57_m0[59u].x * _5232;
        _1099 = _57_m0[59u].x * _5234;
        _1101 = _57_m0[59u].x * _5236;
    }
    float _1111 = clamp((_1095 - _57_m0[94u].z) / (_57_m0[94u].w - _57_m0[94u].z), 0.0f, 1.0f);
    SV_Target_3.x = _1111;
    SV_Target_3.y = _1111;
    SV_Target_3.z = _1111;
    SV_Target_3.w = 1.0f;
    float _1324;
    float _1326;
    float _1328;
    if ((asuint(_1097) & 2139095040u) == 2139095040u)
    {
        _1324 = 0.0f;
        _1326 = 0.0f;
        _1328 = 0.0f;
    }
    else
    {
        float frontier_phi_26_27_ladder;
        float frontier_phi_26_27_ladder_1;
        float frontier_phi_26_27_ladder_2;
        if ((asuint(_1099) & 2139095040u) == 2139095040u)
        {
            frontier_phi_26_27_ladder = 0.0f;
            frontier_phi_26_27_ladder_1 = 0.0f;
            frontier_phi_26_27_ladder_2 = 0.0f;
        }
        else
        {
            bool _1532 = (asuint(_1101) & 2139095040u) == 2139095040u;
            frontier_phi_26_27_ladder = _1532 ? 0.0f : _1097;
            frontier_phi_26_27_ladder_1 = _1532 ? 0.0f : _1101;
            frontier_phi_26_27_ladder_2 = _1532 ? 0.0f : _1099;
        }
        _1324 = frontier_phi_26_27_ladder;
        _1326 = frontier_phi_26_27_ladder_2;
        _1328 = frontier_phi_26_27_ladder_1;
    }
    SV_Target.x = _1324;
    SV_Target.y = _1326;
    SV_Target.z = _1328;
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
