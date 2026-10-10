static uint _8690;
static uint _8691;
static uint _8692;
static uint _8700;
static uint _8701;
static uint _8803;
static uint _8804;
static uint _8933;
static uint _8934;
static uint _9181;
static uint _9182;
static float _14895;
static float _14897;
static float _14899;
static float _14901;
static float _14903;
static float _14905;
static bool _14907;
static float _14909;
static float _14911;
static float _14912;
static float _14914;
static float _14915;
static float _14917;
static float _14918;
static float _14920;
static float _14921;
static uint _14923;
static uint _14924;
static float _14926;
static float _14927;
static float _14929;
static float _14930;
static float _14932;
static float _14933;
static uint _16074;
static uint _16075;
static uint _16078;
static bool _16079;
static float _16080;
static float _16081;
static float _16082;
static float _16086;
static float _16087;
static float _16088;
static float _16089;
static float _16090;
static float _16091;
static float _16092;
static float _16093;
static float _16094;
static float _16096;
static float _16097;
static float _16098;
static float _16099;
static float _16100;
static float _16101;

static const uint _99[8] = { 1u, 0u, 4294967295u, 0u, 0u, 1u, 0u, 4294967295u };

cbuffer _62_64 : register(b2, space0)
{
    float4 _64_m0[700] : packoffset(c0);
};

cbuffer _67_69 : register(b0, space0)
{
    float4 _69_m0[1165] : packoffset(c0);
};

Buffer<uint4> _8 : register(t96, space0);
Buffer<uint4> _9 : register(t97, space0);
Buffer<uint4> _10 : register(t98, space0);
Texture2D<float4> _15[] : register(t0, space6);
Texture3D<float4> _19[] : register(t0, space7);
Texture3D<float4> _21 : register(t123, space0);
TextureCube<float4> _24 : register(t118, space0);
Texture2D<float4> _26 : register(t38, space0);
Texture2D<float4> _29[] : register(t0, space37);
Texture2D<uint4> _33[] : register(t0, space38);
TextureCube<float4> _36[] : register(t0, space41);
Texture2D<float4> _39[] : register(t0, space1);
Texture2DArray<float4> _43[] : register(t0, space4);
Texture3D<uint4> _46 : register(t0, space12);
Buffer<uint4> _47 : register(t1, space12);
Buffer<uint4> _48 : register(t2, space12);
Buffer<uint4> _49 : register(t3, space12);
Buffer<uint4> _50 : register(t9, space12);
Texture2DArray<float4> _52 : register(t4, space12);
Buffer<uint4> _53 : register(t5, space12);
Texture2D<float4> _54 : register(t6, space12);
Texture2D<float4> _55 : register(t7, space12);
Buffer<uint4> _56 : register(t8, space12);
Texture2D<float4> _57 : register(t8, space0);
Texture3D<float4> _58 : register(t124, space0);
SamplerState _72 : register(s0, space0);
SamplerState _73 : register(s2, space0);
SamplerState _74 : register(s5, space0);
SamplerState _75 : register(s1, space0);
SamplerState _76 : register(s3, space0);
SamplerState _77 : register(s11, space0);

static float4 gl_FragCoord;
static float4 SV_Target;
static float4 SV_Target_1;
static uint4 SV_Target_2;
static float4 SV_Target_5;

struct SPIRV_Cross_Input
{
    float4 gl_FragCoord : SV_Position;
};

struct SPIRV_Cross_Output
{
    float4 SV_Target : SV_Target0;
    float4 SV_Target_1 : SV_Target1;
    uint4 SV_Target_2 : SV_Target2;
    float4 SV_Target_5 : SV_Target5;
};

static uint _90[26];
static uint _91[26];
static uint _92[26];
static uint _93[26];

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
    _90[0u] = 2097152u;
    _91[0u] = 0u;
    _92[0u] = 4228890621u;
    _93[0u] = 4294963171u;
    _90[1u] = 67108864u;
    _91[1u] = 0u;
    _92[1u] = 4226793471u;
    _93[1u] = 4294967295u;
    _90[2u] = 0u;
    _91[2u] = 0u;
    _92[2u] = 4294950911u;
    _93[2u] = 4294967295u;
    _90[3u] = 0u;
    _91[3u] = 0u;
    _92[3u] = 4294950911u;
    _93[3u] = 4294967295u;
    _90[4u] = 134234112u;
    _91[4u] = 512u;
    _92[4u] = 4127178751u;
    _93[4u] = 4294965755u;
    _90[5u] = 134234112u;
    _91[5u] = 512u;
    _92[5u] = 4127178751u;
    _93[5u] = 4294963711u;
    _90[6u] = 0u;
    _91[6u] = 0u;
    _92[6u] = 4294950901u;
    _93[6u] = 4294963167u;
    _90[7u] = 512u;
    _91[7u] = 0u;
    _92[7u] = 4294950389u;
    _93[7u] = 4294963167u;
    _90[8u] = 49228u;
    _91[8u] = 256u;
    _92[8u] = 4294852529u;
    _93[8u] = 4294962943u;
    _90[9u] = 180300u;
    _91[9u] = 0u;
    _92[9u] = 4294721201u;
    _93[9u] = 4294963199u;
    _90[10u] = 33092u;
    _91[10u] = 0u;
    _92[10u] = 4294590129u;
    _93[10u] = 4294963071u;
    _90[11u] = 2072u;
    _91[11u] = 0u;
    _92[11u] = 4294965221u;
    _93[11u] = 4294963199u;
    _90[12u] = 2072u;
    _91[12u] = 1u;
    _92[12u] = 4294965221u;
    _93[12u] = 4294963198u;
    _90[13u] = 2072u;
    _91[13u] = 2u;
    _92[13u] = 4294965221u;
    _93[13u] = 4294963197u;
    _90[14u] = 2147485784u;
    _91[14u] = 0u;
    _92[14u] = 2147481509u;
    _93[14u] = 4294963199u;
    _90[15u] = 0u;
    _91[15u] = 64u;
    _92[15u] = 4294950901u;
    _93[15u] = 4294963103u;
    _90[16u] = 27279360u;
    _91[16u] = 8212u;
    _92[16u] = 4267687933u;
    _93[16u] = 4294954987u;
    _90[17u] = 0u;
    _91[17u] = 0u;
    _92[17u] = 467664896u;
    _93[17u] = 4294962712u;
    _90[18u] = 0u;
    _91[18u] = 0u;
    _92[18u] = 159383552u;
    _93[18u] = 4294962688u;
    _90[19u] = 0u;
    _91[19u] = 2097152u;
    _92[19u] = 4294967292u;
    _93[19u] = 4292849631u;
    _90[20u] = 0u;
    _91[20u] = 0u;
    _92[20u] = 4292853756u;
    _93[20u] = 4288638927u;
    _90[21u] = 0u;
    _91[21u] = 8388608u;
    _92[21u] = 4292853756u;
    _93[21u] = 3212767183u;
    _90[22u] = 0u;
    _91[22u] = 0u;
    _92[22u] = 4292853756u;
    _93[22u] = 1023274975u;
    _90[23u] = 0u;
    _91[23u] = 0u;
    _92[23u] = 4292853756u;
    _93[23u] = 3206426575u;
    _90[24u] = 0u;
    _91[24u] = 0u;
    _92[24u] = 4292853756u;
    _93[24u] = 941109199u;
    _90[25u] = 0u;
    _91[25u] = 0u;
    _92[25u] = 2145367972u;
    _93[25u] = 403713740u;
    float _300 = floor(gl_FragCoord.x);
    float _301 = floor(gl_FragCoord.y);
    uint _302 = uint(_300);
    uint _303 = uint(_301);
    float4 _306 = _29[0u].Load(int3(uint2(_302, _303), 0u));
    float _309 = _306.x;
    float _312 = _300 + 0.5f;
    float _314 = _301 + 0.5f;
    float _329 = (((float(_302) + 0.5f) / _64_m0[691u].x) * 2.0f) + (-1.0f);
    float _331 = 1.0f - (((float(_303) + 0.5f) / _64_m0[691u].y) * 2.0f);
    float _376 = mad(_309, _69_m0[1139u].z, mad(_331, _69_m0[1139u].y, _69_m0[1139u].x * _329)) + _69_m0[1139u].w;
    float _377 = (mad(_309, _69_m0[1136u].z, mad(_331, _69_m0[1136u].y, _69_m0[1136u].x * _329)) + _69_m0[1136u].w) / _376;
    float _378 = (mad(_309, _69_m0[1137u].z, mad(_331, _69_m0[1137u].y, _69_m0[1137u].x * _329)) + _69_m0[1137u].w) / _376;
    float _379 = (mad(_309, _69_m0[1138u].z, mad(_331, _69_m0[1138u].y, _69_m0[1138u].x * _329)) + _69_m0[1138u].w) / _376;
    float _385 = _64_m0[8u].x + _377;
    float _386 = _64_m0[8u].y + _378;
    float _387 = _64_m0[8u].z + _379;
    uint4 _391 = _33[2u].Load(int3(uint2(_302, _303), 0u));
    uint _393 = _391.x;
    uint _394 = _391.w;
    uint4 _399 = _33[15u].Load(int3(uint2(_302, _303), 0u));
    uint _401 = _399.y;
    uint _410 = ((_401 & 64u) != 0u) ? uint((_401 & 4294967167u) != 66u) : 4294967295u;
    uint _411 = _394 & 128u;
    uint _414 = (_411 != 0u) ? 1u : ((_393 << 7u) | _394);
    uint4 _417 = _8.Load(_414 * 4u);
    uint _418 = _417.x;
    uint4 _421 = _8.Load((_414 * 4u) + 1u);
    uint _422 = _421.x;
    uint4 _425 = _8.Load((_414 * 4u) + 2u);
    uint _426 = _425.x;
    uint4 _429 = _8.Load((_414 * 4u) + 3u);
    uint _430 = _429.x;
    uint _433 = ((_418 & 1u) != 0u) ? 0u : 18u;
    uint _435 = uint(min(int(uint(max(int(_410), int(0u)))), int(1u)));
    uint _447;
    uint _448;
    if (_411 == 0u)
    {
        _447 = (((_418 & 2097152u) != 0u) && (_410 == _435)) ? (_433 | 128u) : _433;
        _448 = _418;
    }
    else
    {
        _447 = _394;
        _448 = _418 | ((_393 << 20u) & 134217728u);
    }
    uint _457 = _422 & 16u;
    uint _459;
    if (_457 == 0u)
    {
        _459 = 0u;
    }
    else
    {
        _459 = uint(asuint(_69_m0[1148u]).y != 0u);
    }
    bool _461 = _459 != 0u;
    float _473 = asfloat(_10.Load((_430 * 115u) + 1u).x);
    float _478 = asfloat(_10.Load((_430 * 115u) + 5u).x);
    float _483 = asfloat(_10.Load((_430 * 115u) + 6u).x);
    uint _485 = (_430 * 115u) + 7u;
    float3 _497 = asfloat(uint3(_10.Load(_485).x, _10.Load(_485 + 1u).x, _10.Load(_485 + 2u).x));
    float _516 = asfloat(_10.Load((_430 * 115u) + 56u).x);
    uint4 _520 = _10.Load((_430 * 115u) + 59u);
    uint _521 = _520.x;
    float _532 = asfloat(_10.Load((_430 * 115u) + 92u).x);
    float _538 = asfloat(_10.Load((_430 * 115u) + 93u).x);
    float _544 = asfloat(_10.Load((_430 * 115u) + 94u).x);
    uint4 _548 = _10.Load((_430 * 115u) + 104u);
    uint _549 = _548.x;
    float _555 = asfloat(_10.Load((_430 * 115u) + 105u).x);
    float _946;
    float _948;
    float _950;
    float _952;
    float _954;
    float _956;
    float _958;
    float _960;
    float _962;
    float _964;
    float _966;
    float _968;
    float _970;
    float _972;
    float _974;
    float _976;
    float _978;
    float _980;
    float _982;
    float _984;
    float _986;
    float _988;
    float _990;
    float _992;
    float _994;
    uint _996;
    float _997;
    float _998;
    float _999;
    float _1000;
    float _1004;
    float _1008;
    float _1012;
    float _1015;
    float _1018;
    float _1021;
    float _1023;
    float _1025;
    float _1030;
    float _1035;
    float _1040;
    float _1045;
    float _1048;
    float _1051;
    float _1056;
    float _1060;
    float _1061;
    float _1065;
    float _1067;
    float _1070;
    float _1073;
    float _1076;
    float _1079;
    float _1081;
    float _1084;
    uint _1086;
    float _1088;
    float _1090;
    float _1092;
    float _1094;
    float _1096;
    float _1097;
    float _1098;
    float _1099;
    float _1100;
    float _1102;
    float _1104;
    uint _1106;
    float _1107;
    float _1109;
    float _1111;
    float _1113;
    float _1115;
    float _1117;
    uint _1119;
    float _1121;
    float _1124;
    float _1126;
    float _1128;
    float _1130;
    float _1132;
    float _1135;
    float _1137;
    float _1139;
    float _1141;
    float _1143;
    float _1145;
    float _1147;
    uint _1149;
    uint _1151;
    uint _1153;
    float _1155;
    float _1157;
    float _1159;
    float _1161;
    float _1163;
    float _1165;
    float _1167;
    float _1169;
    float _1171;
    uint _1173;
    uint _1174;
    if (((_448 & 1u) | (_422 & 1032192u)) == 0u)
    {
        uint4 _575 = _33[2u].Load(int3(uint2(_302, _303), 0u));
        uint _578 = _575.z;
        uint _579 = _447 & 128u;
        bool _580 = _579 == 0u;
        float _661;
        uint _662;
        uint _663;
        if (_580)
        {
            _661 = 0.0f;
            _662 = _448 >> 4u;
            _663 = 0u;
        }
        else
        {
            _661 = float(_575.x & 127u) * 0.0078740157186985015869140625f;
            _662 = _447;
            _663 = 1u;
        }
        precise float _671 = float(_578 & 127u) * 0.0078740157186985015869140625f;
        precise float _672 = float(_575.y & 127u) * 0.0078740157186985015869140625f;
        float4 _675 = _29[3u].Load(int3(uint2(_302, _303), 0u));
        float _677 = _675.x;
        float _678 = _675.y;
        float _679 = _675.z;
        float _680 = _675.w;
        uint4 _683 = _33[1u].Load(int3(uint2(_302, _303), 0u));
        uint _685 = _683.x;
        float4 _688 = _29[8u].Load(int3(uint2(_302, _303), 0u));
        float _690 = _688.x;
        float _691 = _688.y;
        uint _839;
        if ((_579 | (_662 & 1u)) == 0u)
        {
            _839 = 0u;
        }
        else
        {
            _839 = _33[9u].Load(int3(uint2(_302, _303), 0u)).x;
        }
        uint _1184;
        if (_663 == 0u)
        {
            _1184 = 0u;
        }
        else
        {
            _1184 = _33[10u].Load(int3(uint2(_302, _303), 0u)).x;
        }
        float _1194 = (float((_685 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _1195 = (float(_685 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _1199 = (1.0f - abs(_1194)) - abs(_1195);
        float _1201 = clamp((-0.0f) - _1199, 0.0f, 1.0f);
        float _1202 = (-0.0f) - _1201;
        float _1207 = ((_1194 >= 0.0f) ? _1202 : _1201) + _1194;
        float _1208 = ((_1195 >= 0.0f) ? _1202 : _1201) + _1195;
        float _1212 = rsqrt(dot(float3(_1207, _1208, _1199), float3(_1207, _1208, _1199)));
        float _1003 = _1207 * _1212;
        float _1007 = _1208 * _1212;
        float _1011 = _1212 * _1199;
        float _1055 = float(_685 & 255u) * 0.0039215688593685626983642578125f;
        float _1432;
        float _1433;
        float _1436;
        float _1438;
        float _1440;
        if (_580)
        {
            _1432 = _680;
            _1433 = 0.039999999105930328369140625f;
            _1436 = 0.039999999105930328369140625f;
            _1438 = 0.039999999105930328369140625f;
            _1440 = 0.039999999105930328369140625f;
        }
        else
        {
            float _1441 = ((_447 & 32u) != 0u) ? _680 : 0.039999999105930328369140625f;
            float frontier_phi_30_31_ladder;
            float frontier_phi_30_31_ladder_1;
            float frontier_phi_30_31_ladder_2;
            float frontier_phi_30_31_ladder_3;
            float frontier_phi_30_31_ladder_4;
            if ((_447 & 1u) == 0u)
            {
                frontier_phi_30_31_ladder = spvUnpackHalf2x16((_839 >> 7u) & 32752u).x;
                frontier_phi_30_31_ladder_1 = 0.0f;
                frontier_phi_30_31_ladder_2 = spvUnpackHalf2x16((_839 << 4u) & 32752u).x;
                frontier_phi_30_31_ladder_3 = spvUnpackHalf2x16((_839 >> 17u) & 32736u).x;
                frontier_phi_30_31_ladder_4 = _1441;
            }
            else
            {
                frontier_phi_30_31_ladder = 0.039999999105930328369140625f;
                frontier_phi_30_31_ladder_1 = 0.0f;
                frontier_phi_30_31_ladder_2 = 0.039999999105930328369140625f;
                frontier_phi_30_31_ladder_3 = 0.039999999105930328369140625f;
                frontier_phi_30_31_ladder_4 = _1441;
            }
            _1432 = frontier_phi_30_31_ladder_1;
            _1433 = frontier_phi_30_31_ladder_2;
            _1436 = frontier_phi_30_31_ladder;
            _1438 = frontier_phi_30_31_ladder_3;
            _1440 = frontier_phi_30_31_ladder_4;
        }
        bool _1443 = (_447 & 64u) != 0u;
        uint _1444 = _448 & 64u;
        float _1103;
        float _1650;
        if (_1444 == 0u)
        {
            _1650 = 1.0f;
            _1103 = 0.0f;
        }
        else
        {
            float frontier_phi_40_41_ladder;
            float frontier_phi_40_41_ladder_1;
            if (int(_448) < int(0u))
            {
                float _1759 = _690 * 2.0f;
                frontier_phi_40_41_ladder = clamp(_1759 + (-1.0f), 0.0f, 1.0f);
                frontier_phi_40_41_ladder_1 = clamp(_1759, 0.0f, 1.0f);
            }
            else
            {
                frontier_phi_40_41_ladder = 0.0f;
                frontier_phi_40_41_ladder_1 = _690;
            }
            _1650 = frontier_phi_40_41_ladder_1;
            _1103 = frontier_phi_40_41_ladder;
        }
        float _1105 = ((_422 & 3u) != 0u) ? _690 : 0.0f;
        float _1066 = ((_448 & 8u) != 0u) ? _691 : 1.0f;
        float _985;
        float _987;
        float _989;
        float _1933;
        float _1935;
        uint _1937;
        if ((_447 & 1u) == 0u)
        {
            uint frontier_phi_54_47_ladder;
            float frontier_phi_54_47_ladder_1;
            float frontier_phi_54_47_ladder_2;
            float frontier_phi_54_47_ladder_3;
            float frontier_phi_54_47_ladder_4;
            float frontier_phi_54_47_ladder_5;
            if ((_448 & 16u) == 0u)
            {
                frontier_phi_54_47_ladder = 0u;
                frontier_phi_54_47_ladder_1 = 0.0f;
                frontier_phi_54_47_ladder_2 = 0.0f;
                frontier_phi_54_47_ladder_3 = 0.0f;
                frontier_phi_54_47_ladder_4 = 0.0f;
                frontier_phi_54_47_ladder_5 = 0.0f;
            }
            else
            {
                float _1949 = (float(_839 & 65535u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                float _1950 = (float(_839 >> 16u) * 3.05180437862873077392578125e-05f) + (-1.0f);
                float _1954 = (1.0f - abs(_1949)) - abs(_1950);
                float _1956 = clamp((-0.0f) - _1954, 0.0f, 1.0f);
                float _1957 = (-0.0f) - _1956;
                float _1962 = ((_1949 >= 0.0f) ? _1957 : _1956) + _1949;
                float _1963 = ((_1950 >= 0.0f) ? _1957 : _1956) + _1950;
                float _1967 = rsqrt(dot(float3(_1962, _1963, _1954), float3(_1962, _1963, _1954)));
                frontier_phi_54_47_ladder = 1u;
                frontier_phi_54_47_ladder_1 = _1967 * _1954;
                frontier_phi_54_47_ladder_2 = _1962 * _1967;
                frontier_phi_54_47_ladder_3 = 0.0f;
                frontier_phi_54_47_ladder_4 = 0.0f;
                frontier_phi_54_47_ladder_5 = 0.0f;
            }
            _985 = frontier_phi_54_47_ladder_5;
            _987 = frontier_phi_54_47_ladder_4;
            _989 = frontier_phi_54_47_ladder_3;
            _1933 = frontier_phi_54_47_ladder_2;
            _1935 = frontier_phi_54_47_ladder_1;
            _1937 = frontier_phi_54_47_ladder;
        }
        else
        {
            _985 = spvUnpackHalf2x16((_839 << 4u) & 32752u).x;
            _987 = spvUnpackHalf2x16((_839 >> 7u) & 32752u).x;
            _989 = spvUnpackHalf2x16((_839 >> 17u) & 32736u).x;
            _1933 = 0.0f;
            _1935 = 0.0f;
            _1937 = 0u;
        }
        float _991;
        float _993;
        float _995;
        float _1014;
        float _1017;
        float _1020;
        if ((_447 & 65u) == 0u)
        {
            float frontier_phi_66_61_ladder;
            float frontier_phi_66_61_ladder_1;
            float frontier_phi_66_61_ladder_2;
            float frontier_phi_66_61_ladder_3;
            float frontier_phi_66_61_ladder_4;
            float frontier_phi_66_61_ladder_5;
            if (_663 == 0u)
            {
                frontier_phi_66_61_ladder = _1011;
                frontier_phi_66_61_ladder_1 = _1003;
                frontier_phi_66_61_ladder_2 = _1007;
                frontier_phi_66_61_ladder_3 = 0.0f;
                frontier_phi_66_61_ladder_4 = 0.0f;
                frontier_phi_66_61_ladder_5 = 0.0f;
            }
            else
            {
                float2 _2164 = spvUnpackHalf2x16((_1184 >> 17u) & 32736u);
                float _2165 = _2164.x;
                float _2168 = (spvUnpackHalf2x16((_1184 << 4u) & 32752u).x * 2.0f) + (-1.0f);
                float _2169 = (spvUnpackHalf2x16((_1184 >> 7u) & 32752u).x * 2.0f) + (-1.0f);
                float _2173 = (1.0f - abs(_2168)) - abs(_2169);
                float _2175 = clamp((-0.0f) - _2173, 0.0f, 1.0f);
                float _2176 = (-0.0f) - _2175;
                float _2181 = ((_2168 >= 0.0f) ? _2176 : _2175) + _2168;
                float _2182 = ((_2169 >= 0.0f) ? _2176 : _2175) + _2169;
                float _2186 = rsqrt(dot(float3(_2181, _2182, _2173), float3(_2181, _2182, _2173)));
                float _2196 = (((_2181 * _2186) - _1003) * _2165) + _1003;
                float _2197 = (((_2182 * _2186) - _1007) * _2165) + _1007;
                float _2198 = (((_2186 * _2173) - _1011) * _2165) + _1011;
                float _2202 = rsqrt(dot(float3(_2196, _2197, _2198), float3(_2196, _2197, _2198)));
                frontier_phi_66_61_ladder = _2198 * _2202;
                frontier_phi_66_61_ladder_1 = _2196 * _2202;
                frontier_phi_66_61_ladder_2 = _2197 * _2202;
                frontier_phi_66_61_ladder_3 = 0.0f;
                frontier_phi_66_61_ladder_4 = 0.0f;
                frontier_phi_66_61_ladder_5 = 0.0f;
            }
            _1014 = frontier_phi_66_61_ladder_1;
            _1017 = frontier_phi_66_61_ladder_2;
            _1020 = frontier_phi_66_61_ladder;
            _991 = frontier_phi_66_61_ladder_3;
            _993 = frontier_phi_66_61_ladder_4;
            _995 = frontier_phi_66_61_ladder_5;
        }
        else
        {
            _1014 = _1003;
            _1017 = _1007;
            _1020 = _1011;
            _991 = spvUnpackHalf2x16((_1184 << 4u) & 32752u).x;
            _993 = spvUnpackHalf2x16((_1184 >> 7u) & 32752u).x;
            _995 = spvUnpackHalf2x16((_1184 >> 17u) & 32736u).x;
        }
        bool _2143 = _1937 != 0u;
        bool _2144 = asuint(_64_m0[176u]).z != 0u;
        float _2147 = 1.0f - _1432;
        bool _2151 = _457 != 0u;
        bool _2152 = _2151 && _461;
        float _1112 = _2152 ? 1.0f : (_2147 * _677);
        float _1114 = _2152 ? 1.0f : (_2147 * _678);
        float _1116 = _2152 ? 1.0f : (_2147 * _679);
        float _2272;
        float _2274;
        float _2276;
        if (_473 != 0.0f)
        {
            float _2269 = max(max(_677, _678), _679);
            float frontier_phi_74_73_ladder;
            float frontier_phi_74_73_ladder_1;
            float frontier_phi_74_73_ladder_2;
            if (_2269 > 0.001000000047497451305389404296875f)
            {
                float _2339 = _473 / min(_473, _2269);
                frontier_phi_74_73_ladder = _2339 * _679;
                frontier_phi_74_73_ladder_1 = _2339 * _678;
                frontier_phi_74_73_ladder_2 = _2339 * _677;
            }
            else
            {
                frontier_phi_74_73_ladder = _473;
                frontier_phi_74_73_ladder_1 = _473;
                frontier_phi_74_73_ladder_2 = _473;
            }
            _2272 = frontier_phi_74_73_ladder_2;
            _2274 = frontier_phi_74_73_ladder_1;
            _2276 = frontier_phi_74_73_ladder;
        }
        else
        {
            _2272 = _677;
            _2274 = _678;
            _2276 = _679;
        }
        float _1044;
        float _1047;
        float _1050;
        if ((_447 & 32u) == 0u)
        {
            _1044 = ((_2272 - _1433) * _1432) + _1433;
            _1047 = ((_2274 - _1436) * _1432) + _1436;
            _1050 = ((_2276 - _1438) * _1432) + _1438;
        }
        else
        {
            _1044 = _1440;
            _1047 = _1440;
            _1050 = _1440;
        }
        float _1069;
        if ((_448 & 2048u) == 0u)
        {
            _1069 = _1066;
        }
        else
        {
            float frontier_phi_86_87_ladder;
            if (_64_m0[88u].w > 0.0f)
            {
                float _2603 = clamp((_1055 - _64_m0[88u].x) / (_64_m0[88u].y - _64_m0[88u].x), 0.0f, 1.0f);
                frontier_phi_86_87_ladder = exp2(log2((_2603 * _2603) * (3.0f - (_2603 * 2.0f))) * _64_m0[88u].z) * _1066;
            }
            else
            {
                frontier_phi_86_87_ladder = _1066;
            }
            _1069 = frontier_phi_86_87_ladder;
        }
        bool _2437 = _1444 != 0u;
        float _1080;
        float _1083;
        float _1085;
        if (int(_448) < int(0u))
        {
            _1080 = _64_m0[153u].x;
            _1083 = _64_m0[153u].x;
            _1085 = _64_m0[153u].y * _1650;
        }
        else
        {
            _1080 = asfloat(_10.Load((_430 * 115u) + 10u).x);
            _1083 = asfloat(_10.Load((_430 * 115u) + 11u).x);
            _1085 = _1650;
        }
        float _1029;
        float _1034;
        float _1039;
        float _1108;
        float _1110;
        if ((_422 & 1u) == 0u)
        {
            _1029 = _1112;
            _1034 = _1114;
            _1039 = _1116;
            _1108 = 0.0f;
            _1110 = ((_422 & 2u) != 0u) ? _1105 : 0.0f;
        }
        else
        {
            float _2648 = clamp((dot(float3(_1112, _1114, _1116), 1.0f.xxx) + (-1.7000000476837158203125f)) * 2.0f, 0.0f, 1.0f) * _1105;
            _1029 = (_2648 * (0.839999973773956298828125f - _1112)) + _1112;
            _1034 = (_2648 * (0.87999999523162841796875f - _1114)) + _1114;
            _1039 = (_2648 * (0.980000019073486328125f - _1116)) + _1116;
            _1108 = _1105;
            _1110 = 0.0f;
        }
        bool _2708 = (_422 & 256u) != 0u;
        bool _2710 = _2151 && (_516 > 0.0f);
        _946 = 0.0f;
        _948 = 0.0f;
        _950 = 0.0f;
        _952 = 0.0f;
        _954 = 0.0f;
        _956 = 0.0f;
        _958 = 0.0f;
        _960 = 0.0f;
        _962 = 0.0f;
        _964 = 0.0f;
        _966 = 0.0f;
        _968 = 0.0f;
        _970 = 0.0f;
        _972 = 0.0f;
        _974 = 0.0f;
        _976 = 0.0f;
        _978 = _1443 ? _690 : 0.0f;
        _980 = _1443 ? _691 : 0.0f;
        _982 = _1443 ? _688.z : 0.0f;
        _984 = _985;
        _986 = _987;
        _988 = _989;
        _990 = _991;
        _992 = _993;
        _994 = _995;
        _996 = (_578 >> 7u) & 1u;
        _997 = _385;
        _998 = _386;
        _999 = _387;
        _1000 = _1003;
        _1004 = _1007;
        _1008 = _1011;
        _1012 = _1014;
        _1015 = _1017;
        _1018 = _1020;
        _1021 = (_2144 && _2143) ? _1933 : _1014;
        _1023 = (_2144 && _2143) ? _1935 : _1020;
        _1025 = _1029;
        _1030 = _1034;
        _1035 = _1039;
        _1040 = _1044;
        _1045 = _1047;
        _1048 = _1050;
        _1051 = _1055;
        _1056 = _671;
        _1060 = _688.w;
        _1061 = _672;
        _1065 = _1066;
        _1067 = _1069;
        _1070 = (_2437 ? (_1112 * _497.x) : 0.0f) * _1650;
        _1073 = (_2437 ? (_1114 * _497.y) : 0.0f) * _1650;
        _1076 = (_2437 ? (_1116 * _497.z) : 0.0f) * _1650;
        _1079 = _1080;
        _1081 = _1083;
        _1084 = _1085;
        _1086 = 0u;
        _1088 = 0.0f;
        _1090 = 0.0f;
        _1092 = 0.0f;
        _1094 = 0.0f;
        _1096 = 0.0f;
        _1097 = 0.0f;
        _1098 = 0.0f;
        _1099 = 0.0f;
        _1100 = _580 ? 0.0f : _661;
        _1102 = _1103;
        _1104 = _1105;
        _1106 = _10.Load((_430 * 115u) + 60u).x;
        _1107 = _1108;
        _1109 = _1110;
        _1111 = _1112;
        _1113 = _1114;
        _1115 = _1116;
        _1117 = 1.0f - _1055;
        _1119 = _2710 ? _521 : 0u;
        _1121 = _2710 ? _516 : 0.0f;
        _1124 = _2708 ? _532 : 0.0f;
        _1126 = _2708 ? _538 : 0.0f;
        _1128 = _2708 ? _544 : 0.0f;
        _1130 = 0.0f;
        _1132 = 0.0f;
        _1135 = 0.0f;
        _1137 = 0.0f;
        _1139 = 0.0f;
        _1141 = 0.0f;
        _1143 = 0.0f;
        _1145 = 0.0f;
        _1147 = 0.0f;
        _1149 = 0u;
        _1151 = 0u;
        _1153 = 0u;
        _1155 = 0.0f;
        _1157 = 0.0f;
        _1159 = 0.0f;
        _1161 = 0.0f;
        _1163 = 0.0f;
        _1165 = 0.0f;
        _1167 = 0.0f;
        _1169 = 0.0f;
        _1171 = 0.0f;
        _1173 = _448;
        _1174 = _422;
    }
    else
    {
        uint frontier_phi_21_7_ladder;
        float frontier_phi_21_7_ladder_1;
        float frontier_phi_21_7_ladder_2;
        float frontier_phi_21_7_ladder_3;
        float frontier_phi_21_7_ladder_4;
        float frontier_phi_21_7_ladder_5;
        float frontier_phi_21_7_ladder_6;
        float frontier_phi_21_7_ladder_7;
        float frontier_phi_21_7_ladder_8;
        float frontier_phi_21_7_ladder_9;
        float frontier_phi_21_7_ladder_10;
        float frontier_phi_21_7_ladder_11;
        float frontier_phi_21_7_ladder_12;
        float frontier_phi_21_7_ladder_13;
        float frontier_phi_21_7_ladder_14;
        float frontier_phi_21_7_ladder_15;
        float frontier_phi_21_7_ladder_16;
        float frontier_phi_21_7_ladder_17;
        float frontier_phi_21_7_ladder_18;
        float frontier_phi_21_7_ladder_19;
        float frontier_phi_21_7_ladder_20;
        float frontier_phi_21_7_ladder_21;
        float frontier_phi_21_7_ladder_22;
        float frontier_phi_21_7_ladder_23;
        float frontier_phi_21_7_ladder_24;
        float frontier_phi_21_7_ladder_25;
        float frontier_phi_21_7_ladder_26;
        float frontier_phi_21_7_ladder_27;
        float frontier_phi_21_7_ladder_28;
        float frontier_phi_21_7_ladder_29;
        float frontier_phi_21_7_ladder_30;
        float frontier_phi_21_7_ladder_31;
        float frontier_phi_21_7_ladder_32;
        float frontier_phi_21_7_ladder_33;
        float frontier_phi_21_7_ladder_34;
        float frontier_phi_21_7_ladder_35;
        float frontier_phi_21_7_ladder_36;
        float frontier_phi_21_7_ladder_37;
        float frontier_phi_21_7_ladder_38;
        float frontier_phi_21_7_ladder_39;
        float frontier_phi_21_7_ladder_40;
        float frontier_phi_21_7_ladder_41;
        float frontier_phi_21_7_ladder_42;
        float frontier_phi_21_7_ladder_43;
        float frontier_phi_21_7_ladder_44;
        float frontier_phi_21_7_ladder_45;
        float frontier_phi_21_7_ladder_46;
        float frontier_phi_21_7_ladder_47;
        float frontier_phi_21_7_ladder_48;
        float frontier_phi_21_7_ladder_49;
        float frontier_phi_21_7_ladder_50;
        float frontier_phi_21_7_ladder_51;
        uint frontier_phi_21_7_ladder_52;
        float frontier_phi_21_7_ladder_53;
        float frontier_phi_21_7_ladder_54;
        float frontier_phi_21_7_ladder_55;
        float frontier_phi_21_7_ladder_56;
        float frontier_phi_21_7_ladder_57;
        float frontier_phi_21_7_ladder_58;
        float frontier_phi_21_7_ladder_59;
        float frontier_phi_21_7_ladder_60;
        float frontier_phi_21_7_ladder_61;
        float frontier_phi_21_7_ladder_62;
        float frontier_phi_21_7_ladder_63;
        uint frontier_phi_21_7_ladder_64;
        float frontier_phi_21_7_ladder_65;
        uint frontier_phi_21_7_ladder_66;
        float frontier_phi_21_7_ladder_67;
        float frontier_phi_21_7_ladder_68;
        float frontier_phi_21_7_ladder_69;
        float frontier_phi_21_7_ladder_70;
        float frontier_phi_21_7_ladder_71;
        float frontier_phi_21_7_ladder_72;
        float frontier_phi_21_7_ladder_73;
        float frontier_phi_21_7_ladder_74;
        float frontier_phi_21_7_ladder_75;
        uint frontier_phi_21_7_ladder_76;
        uint frontier_phi_21_7_ladder_77;
        float frontier_phi_21_7_ladder_78;
        float frontier_phi_21_7_ladder_79;
        float frontier_phi_21_7_ladder_80;
        float frontier_phi_21_7_ladder_81;
        float frontier_phi_21_7_ladder_82;
        uint frontier_phi_21_7_ladder_83;
        float frontier_phi_21_7_ladder_84;
        float frontier_phi_21_7_ladder_85;
        float frontier_phi_21_7_ladder_86;
        float frontier_phi_21_7_ladder_87;
        float frontier_phi_21_7_ladder_88;
        float frontier_phi_21_7_ladder_89;
        float frontier_phi_21_7_ladder_90;
        float frontier_phi_21_7_ladder_91;
        uint frontier_phi_21_7_ladder_92;
        float frontier_phi_21_7_ladder_93;
        float frontier_phi_21_7_ladder_94;
        float frontier_phi_21_7_ladder_95;
        float frontier_phi_21_7_ladder_96;
        float frontier_phi_21_7_ladder_97;
        float frontier_phi_21_7_ladder_98;
        uint frontier_phi_21_7_ladder_99;
        float frontier_phi_21_7_ladder_100;
        float frontier_phi_21_7_ladder_101;
        if ((_422 & 16384u) == 0u)
        {
            uint frontier_phi_21_7_ladder_10_ladder;
            float frontier_phi_21_7_ladder_10_ladder_1;
            float frontier_phi_21_7_ladder_10_ladder_2;
            float frontier_phi_21_7_ladder_10_ladder_3;
            float frontier_phi_21_7_ladder_10_ladder_4;
            float frontier_phi_21_7_ladder_10_ladder_5;
            float frontier_phi_21_7_ladder_10_ladder_6;
            float frontier_phi_21_7_ladder_10_ladder_7;
            float frontier_phi_21_7_ladder_10_ladder_8;
            float frontier_phi_21_7_ladder_10_ladder_9;
            float frontier_phi_21_7_ladder_10_ladder_10;
            float frontier_phi_21_7_ladder_10_ladder_11;
            float frontier_phi_21_7_ladder_10_ladder_12;
            float frontier_phi_21_7_ladder_10_ladder_13;
            float frontier_phi_21_7_ladder_10_ladder_14;
            float frontier_phi_21_7_ladder_10_ladder_15;
            float frontier_phi_21_7_ladder_10_ladder_16;
            float frontier_phi_21_7_ladder_10_ladder_17;
            float frontier_phi_21_7_ladder_10_ladder_18;
            float frontier_phi_21_7_ladder_10_ladder_19;
            float frontier_phi_21_7_ladder_10_ladder_20;
            float frontier_phi_21_7_ladder_10_ladder_21;
            float frontier_phi_21_7_ladder_10_ladder_22;
            float frontier_phi_21_7_ladder_10_ladder_23;
            float frontier_phi_21_7_ladder_10_ladder_24;
            float frontier_phi_21_7_ladder_10_ladder_25;
            float frontier_phi_21_7_ladder_10_ladder_26;
            float frontier_phi_21_7_ladder_10_ladder_27;
            float frontier_phi_21_7_ladder_10_ladder_28;
            float frontier_phi_21_7_ladder_10_ladder_29;
            float frontier_phi_21_7_ladder_10_ladder_30;
            float frontier_phi_21_7_ladder_10_ladder_31;
            float frontier_phi_21_7_ladder_10_ladder_32;
            float frontier_phi_21_7_ladder_10_ladder_33;
            float frontier_phi_21_7_ladder_10_ladder_34;
            float frontier_phi_21_7_ladder_10_ladder_35;
            float frontier_phi_21_7_ladder_10_ladder_36;
            float frontier_phi_21_7_ladder_10_ladder_37;
            float frontier_phi_21_7_ladder_10_ladder_38;
            float frontier_phi_21_7_ladder_10_ladder_39;
            float frontier_phi_21_7_ladder_10_ladder_40;
            float frontier_phi_21_7_ladder_10_ladder_41;
            float frontier_phi_21_7_ladder_10_ladder_42;
            float frontier_phi_21_7_ladder_10_ladder_43;
            float frontier_phi_21_7_ladder_10_ladder_44;
            float frontier_phi_21_7_ladder_10_ladder_45;
            float frontier_phi_21_7_ladder_10_ladder_46;
            float frontier_phi_21_7_ladder_10_ladder_47;
            float frontier_phi_21_7_ladder_10_ladder_48;
            float frontier_phi_21_7_ladder_10_ladder_49;
            float frontier_phi_21_7_ladder_10_ladder_50;
            float frontier_phi_21_7_ladder_10_ladder_51;
            uint frontier_phi_21_7_ladder_10_ladder_52;
            float frontier_phi_21_7_ladder_10_ladder_53;
            float frontier_phi_21_7_ladder_10_ladder_54;
            float frontier_phi_21_7_ladder_10_ladder_55;
            float frontier_phi_21_7_ladder_10_ladder_56;
            float frontier_phi_21_7_ladder_10_ladder_57;
            float frontier_phi_21_7_ladder_10_ladder_58;
            float frontier_phi_21_7_ladder_10_ladder_59;
            float frontier_phi_21_7_ladder_10_ladder_60;
            float frontier_phi_21_7_ladder_10_ladder_61;
            float frontier_phi_21_7_ladder_10_ladder_62;
            float frontier_phi_21_7_ladder_10_ladder_63;
            uint frontier_phi_21_7_ladder_10_ladder_64;
            float frontier_phi_21_7_ladder_10_ladder_65;
            uint frontier_phi_21_7_ladder_10_ladder_66;
            float frontier_phi_21_7_ladder_10_ladder_67;
            float frontier_phi_21_7_ladder_10_ladder_68;
            float frontier_phi_21_7_ladder_10_ladder_69;
            float frontier_phi_21_7_ladder_10_ladder_70;
            float frontier_phi_21_7_ladder_10_ladder_71;
            float frontier_phi_21_7_ladder_10_ladder_72;
            float frontier_phi_21_7_ladder_10_ladder_73;
            float frontier_phi_21_7_ladder_10_ladder_74;
            float frontier_phi_21_7_ladder_10_ladder_75;
            uint frontier_phi_21_7_ladder_10_ladder_76;
            uint frontier_phi_21_7_ladder_10_ladder_77;
            float frontier_phi_21_7_ladder_10_ladder_78;
            float frontier_phi_21_7_ladder_10_ladder_79;
            float frontier_phi_21_7_ladder_10_ladder_80;
            float frontier_phi_21_7_ladder_10_ladder_81;
            float frontier_phi_21_7_ladder_10_ladder_82;
            uint frontier_phi_21_7_ladder_10_ladder_83;
            float frontier_phi_21_7_ladder_10_ladder_84;
            float frontier_phi_21_7_ladder_10_ladder_85;
            float frontier_phi_21_7_ladder_10_ladder_86;
            float frontier_phi_21_7_ladder_10_ladder_87;
            float frontier_phi_21_7_ladder_10_ladder_88;
            float frontier_phi_21_7_ladder_10_ladder_89;
            float frontier_phi_21_7_ladder_10_ladder_90;
            float frontier_phi_21_7_ladder_10_ladder_91;
            uint frontier_phi_21_7_ladder_10_ladder_92;
            float frontier_phi_21_7_ladder_10_ladder_93;
            float frontier_phi_21_7_ladder_10_ladder_94;
            float frontier_phi_21_7_ladder_10_ladder_95;
            float frontier_phi_21_7_ladder_10_ladder_96;
            float frontier_phi_21_7_ladder_10_ladder_97;
            float frontier_phi_21_7_ladder_10_ladder_98;
            uint frontier_phi_21_7_ladder_10_ladder_99;
            float frontier_phi_21_7_ladder_10_ladder_100;
            float frontier_phi_21_7_ladder_10_ladder_101;
            if ((_422 & 524288u) == 0u)
            {
                uint frontier_phi_21_7_ladder_10_ladder_13_ladder;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_1;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_2;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_3;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_4;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_5;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_6;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_7;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_8;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_9;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_10;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_11;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_12;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_13;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_14;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_15;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_16;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_17;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_18;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_19;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_20;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_21;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_22;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_23;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_24;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_25;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_26;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_27;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_28;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_29;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_30;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_31;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_32;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_33;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_34;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_35;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_36;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_37;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_38;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_39;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_40;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_41;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_42;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_43;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_44;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_45;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_46;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_47;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_48;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_49;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_50;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_51;
                uint frontier_phi_21_7_ladder_10_ladder_13_ladder_52;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_53;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_54;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_55;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_56;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_57;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_58;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_59;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_60;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_61;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_62;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_63;
                uint frontier_phi_21_7_ladder_10_ladder_13_ladder_64;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_65;
                uint frontier_phi_21_7_ladder_10_ladder_13_ladder_66;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_67;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_68;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_69;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_70;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_71;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_72;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_73;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_74;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_75;
                uint frontier_phi_21_7_ladder_10_ladder_13_ladder_76;
                uint frontier_phi_21_7_ladder_10_ladder_13_ladder_77;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_78;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_79;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_80;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_81;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_82;
                uint frontier_phi_21_7_ladder_10_ladder_13_ladder_83;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_84;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_85;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_86;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_87;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_88;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_89;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_90;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_91;
                uint frontier_phi_21_7_ladder_10_ladder_13_ladder_92;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_93;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_94;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_95;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_96;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_97;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_98;
                uint frontier_phi_21_7_ladder_10_ladder_13_ladder_99;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_100;
                float frontier_phi_21_7_ladder_10_ladder_13_ladder_101;
                if ((_422 & 32768u) == 0u)
                {
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_1;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_2;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_3;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_4;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_5;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_6;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_7;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_8;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_9;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_10;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_11;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_12;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_13;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_14;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_15;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_16;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_17;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_18;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_19;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_20;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_21;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_22;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_23;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_25;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_26;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_27;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_28;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_29;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_30;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_31;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_32;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_33;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_34;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_35;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_36;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_37;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_38;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_39;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_40;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_41;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_42;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_43;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_44;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_45;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_46;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_47;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_48;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_49;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_50;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_51;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_52;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_53;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_54;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_55;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_56;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_57;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_58;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_59;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_60;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_61;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_62;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_63;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_64;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_65;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_66;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_67;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_68;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_69;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_70;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_71;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_72;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_73;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_74;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_75;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_76;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_77;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_78;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_79;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_80;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_81;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_82;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_83;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_84;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_85;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_86;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_87;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_88;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_89;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_90;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_91;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_92;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_93;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_94;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_95;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_96;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_97;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_98;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_99;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_100;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_101;
                    if ((_422 & 65536u) == 0u)
                    {
                        uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_1;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_2;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_3;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_4;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_5;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_6;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_7;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_8;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_9;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_10;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_11;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_12;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_13;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_14;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_15;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_16;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_17;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_18;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_19;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_20;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_21;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_22;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_23;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_24;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_25;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_26;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_27;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_28;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_29;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_30;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_31;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_33;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_34;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_35;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_36;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_37;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_38;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_39;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_40;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_41;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_42;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_43;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_44;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_45;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_46;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_47;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_48;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_49;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_50;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_51;
                        uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_52;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_53;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_54;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_55;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_56;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_57;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_58;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_59;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_60;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_61;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_62;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_63;
                        uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_64;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_65;
                        uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_66;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_67;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_68;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_69;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_70;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_71;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_72;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_73;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_74;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_75;
                        uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_76;
                        uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_77;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_78;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_79;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_80;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_81;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_82;
                        uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_83;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_84;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_85;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_86;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_87;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_88;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_89;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_90;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_91;
                        uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_92;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_93;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_94;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_95;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_96;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_97;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_98;
                        uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_99;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_100;
                        float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_101;
                        if ((_422 & 262144u) == 0u)
                        {
                            uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_1;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_2;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_3;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_4;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_5;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_6;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_7;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_8;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_9;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_10;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_11;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_12;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_13;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_14;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_15;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_16;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_17;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_18;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_19;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_20;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_21;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_22;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_23;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_24;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_25;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_26;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_27;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_28;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_29;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_30;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_31;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_32;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_33;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_34;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_35;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_36;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_37;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_38;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_39;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_40;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_41;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_42;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_43;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_44;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_45;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_46;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_47;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_48;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_49;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_50;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_51;
                            uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_52;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_53;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_54;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_55;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_56;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_57;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_58;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_59;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_60;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_61;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_62;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_63;
                            uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_64;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_65;
                            uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_66;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_67;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_68;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_69;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_70;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_71;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_72;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_73;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_74;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_75;
                            uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_76;
                            uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_77;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_78;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_79;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_80;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_81;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_82;
                            uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_83;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_84;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_85;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_86;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_87;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_88;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_89;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_90;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_91;
                            uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_92;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_93;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_94;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_95;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_96;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_97;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_98;
                            uint frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_99;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_100;
                            float frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_101;
                            if ((_422 & 131072u) == 0u)
                            {
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_1 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_2 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_3 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_4 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_5 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_6 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_7 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_8 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_9 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_10 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_11 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_12 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_13 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_14 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_15 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_16 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_17 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_18 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_19 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_20 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_21 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_22 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_23 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_24 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_25 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_26 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_27 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_28 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_29 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_30 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_31 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_32 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_33 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_34 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_35 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_36 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_37 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_38 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_39 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_40 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_41 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_42 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_43 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_44 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_45 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_46 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_47 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_48 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_49 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_50 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_51 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_52 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_53 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_54 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_55 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_56 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_57 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_58 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_59 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_60 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_61 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_62 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_63 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_64 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_65 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_66 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_67 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_68 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_69 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_70 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_71 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_72 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_73 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_74 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_75 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_76 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_77 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_78 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_79 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_80 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_81 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_82 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_83 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_84 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_85 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_86 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_87 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_88 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_89 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_90 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_91 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_92 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_93 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_94 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_95 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_96 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_97 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_98 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_99 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_100 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_101 = 0.0f;
                            }
                            else
                            {
                                uint4 _1674 = _33[2u].Load(int3(uint2(_302, _303), 0u));
                                float4 _1680 = _29[3u].Load(int3(uint2(_302, _303), 0u));
                                float _1682 = _1680.x;
                                float _1683 = _1680.y;
                                float _1684 = _1680.z;
                                uint4 _1688 = _33[1u].Load(int3(uint2(_302, _303), 0u));
                                uint _1690 = _1688.x;
                                uint4 _1693 = _33[10u].Load(int3(uint2(_302, _303), 0u));
                                uint _1695 = _1693.x;
                                bool _1696 = int(_422) < int(0u);
                                float _1773;
                                float _1774;
                                float _1775;
                                uint _1776;
                                if (_1696)
                                {
                                    float4 _1763 = _29[8u].Load(int3(uint2(_302, _303), 0u));
                                    _1773 = _1763.x;
                                    _1774 = _1763.y;
                                    _1775 = _1763.z;
                                    _1776 = _33[9u].Load(int3(uint2(_302, _303), 0u)).x;
                                }
                                else
                                {
                                    _1773 = 0.0f;
                                    _1774 = 0.0f;
                                    _1775 = 0.0f;
                                    _1776 = 0u;
                                }
                                float _1785 = (float((_1690 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _1786 = (float(_1690 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _1790 = (1.0f - abs(_1785)) - abs(_1786);
                                float _1792 = clamp((-0.0f) - _1790, 0.0f, 1.0f);
                                float _1793 = (-0.0f) - _1792;
                                float _1798 = ((_1785 >= 0.0f) ? _1793 : _1792) + _1785;
                                float _1799 = ((_1786 >= 0.0f) ? _1793 : _1792) + _1786;
                                float _1803 = rsqrt(dot(float3(_1798, _1799, _1790), float3(_1798, _1799, _1790)));
                                float _959 = _1798 * _1803;
                                float _953 = _1799 * _1803;
                                float _947 = _1803 * _1790;
                                float _1821 = (float((_1695 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _1822 = (float(_1695 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                                float _1826 = (1.0f - abs(_1821)) - abs(_1822);
                                float _1828 = clamp((-0.0f) - _1826, 0.0f, 1.0f);
                                float _1829 = (-0.0f) - _1828;
                                float _1834 = ((_1821 >= 0.0f) ? _1829 : _1828) + _1821;
                                float _1835 = ((_1822 >= 0.0f) ? _1829 : _1828) + _1822;
                                float _1839 = rsqrt(dot(float3(_1834, _1835, _1826), float3(_1834, _1835, _1826)));
                                float _963 = _1834 * _1839;
                                float _957 = _1835 * _1839;
                                float _951 = _1839 * _1826;
                                float _965;
                                float _967;
                                float _969;
                                float _971;
                                float _973;
                                float _975;
                                if (_1696)
                                {
                                    float _1979 = float(_1776 & 255u) * 0.0039215688593685626983642578125f;
                                    float _1980 = float((_1776 >> 8u) & 255u) * 0.0039215688593685626983642578125f;
                                    float _1981 = float((_1776 >> 16u) & 255u) * 0.0039215688593685626983642578125f;
                                    _965 = _1773 * _1773;
                                    _967 = _1774 * _1774;
                                    _969 = _1775 * _1775;
                                    _971 = _1979 * _1979;
                                    _973 = _1980 * _1980;
                                    _975 = _1981 * _1981;
                                }
                                else
                                {
                                    _965 = 0.0f;
                                    _967 = 0.0f;
                                    _969 = 0.0f;
                                    _971 = 0.0f;
                                    _973 = 0.0f;
                                    _975 = 0.0f;
                                }
                                uint4 _1988 = _10.Load((_430 * 115u) + 85u);
                                uint _1989 = _1988.x;
                                uint _2016 = _1989 + 692u;
                                float _2022 = (_957 * _947) - (_951 * _953);
                                float _2025 = (_951 * _959) - (_963 * _947);
                                float _2028 = (_963 * _953) - (_957 * _959);
                                float _2032 = rsqrt(dot(float3(_2022, _2025, _2028), float3(_2022, _2025, _2028)));
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_1 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_2 = _385;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_3 = _386;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_4 = _387;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_5 = _959;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_6 = _953;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_7 = _947;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_8 = _959;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_9 = _953;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_10 = _947;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_11 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_12 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_13 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_14 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_15 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_16 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_17 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_18 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_19 = float(_1690 & 255u) * 0.0039215688593685626983642578125f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_20 = float(_1674.y) * 0.0039215688593685626983642578125f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_21 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_22 = float(_1674.z) * 0.0039215688593685626983642578125f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_23 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_24 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_25 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_26 = _969;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_27 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_28 = _947;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_29 = _2032 * _2028;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_30 = _951;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_31 = _953;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_32 = _2032 * _2025;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_33 = _957;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_34 = _959;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_35 = _2032 * _2022;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_36 = _963;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_37 = _965;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_38 = _967;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_39 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_40 = _971;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_41 = _973;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_42 = _975;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_43 = asfloat(_10.Load((_430 * 115u) + 89u).x);
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_44 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_45 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_46 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_47 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_48 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_49 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_50 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_51 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_52 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_53 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_54 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_55 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_56 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_57 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_58 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_59 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_60 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_61 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_62 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_63 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_64 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_65 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_66 = _1989;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_67 = ((-0.0f) - _1682) / (_1682 + (-1.0f));
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_68 = ((-0.0f) - _1683) / (_1683 + (-1.0f));
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_69 = ((-0.0f) - _1684) / (_1684 + (-1.0f));
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_70 = (_1680.w * 0.00999999977648258209228515625f) * asfloat(_10.Load((_430 * 115u) + 88u).x);
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_71 = float(_1695 & 255u) * 0.0039215688593685626983642578125f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_72 = asfloat(_10.Load((_430 * 115u) + 86u).x);
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_73 = asfloat(_10.Load((_430 * 115u) + 87u).x);
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_74 = _64_m0[_2016].x;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_75 = _64_m0[_2016].y;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_76 = _448;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_77 = _422;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_78 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_79 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_80 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_81 = 1.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_82 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_83 = _422 >> 31u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_84 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_85 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_86 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_87 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_88 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_89 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_90 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_91 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_92 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_93 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_94 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_95 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_96 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_97 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_98 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_99 = 0u;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_100 = 0.0f;
                                frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_101 = 0.0f;
                            }
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_1 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_1;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_2 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_2;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_3 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_3;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_4 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_4;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_5 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_5;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_6 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_6;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_7 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_7;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_8 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_8;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_9 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_9;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_10 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_10;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_11 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_11;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_12 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_12;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_13 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_13;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_14 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_14;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_15 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_15;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_16 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_16;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_17 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_17;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_18 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_18;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_19 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_19;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_20 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_20;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_21 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_21;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_22 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_22;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_23 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_23;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_24 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_24;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_25 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_25;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_26 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_26;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_27 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_27;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_28 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_28;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_29 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_29;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_30 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_30;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_31 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_31;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_32;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_33 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_33;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_34 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_34;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_35 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_35;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_36 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_36;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_37 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_37;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_38 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_38;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_39 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_39;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_40 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_40;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_41 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_41;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_42 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_42;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_43 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_43;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_44 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_44;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_45 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_45;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_46 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_46;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_47 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_47;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_48 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_48;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_49 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_49;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_50 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_50;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_51 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_51;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_52 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_52;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_53 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_53;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_54 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_54;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_55 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_55;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_56 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_56;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_57 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_57;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_58 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_58;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_59 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_59;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_60 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_60;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_61 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_61;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_62 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_62;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_63 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_63;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_64 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_64;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_65 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_65;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_66 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_66;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_67 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_67;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_68 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_68;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_69 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_69;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_70 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_70;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_71 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_71;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_72 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_72;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_73 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_73;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_74 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_74;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_75 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_75;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_76 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_76;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_77 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_77;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_78 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_78;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_79 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_79;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_80 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_80;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_81 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_81;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_82 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_82;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_83 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_83;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_84 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_84;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_85 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_85;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_86 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_86;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_87 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_87;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_88 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_88;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_89 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_89;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_90 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_90;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_91 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_91;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_92 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_92;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_93 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_93;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_94 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_94;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_95 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_95;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_96 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_96;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_97 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_97;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_98 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_98;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_99 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_99;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_100 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_100;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_101 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32_ladder_101;
                        }
                        else
                        {
                            uint4 _1455 = _33[2u].Load(int3(uint2(_302, _303), 0u));
                            float4 _1461 = _29[3u].Load(int3(uint2(_302, _303), 0u));
                            uint4 _1468 = _33[1u].Load(int3(uint2(_302, _303), 0u));
                            uint _1470 = _1468.x;
                            uint4 _1473 = _33[9u].Load(int3(uint2(_302, _303), 0u));
                            uint _1475 = _1473.x;
                            uint4 _1478 = _33[10u].Load(int3(uint2(_302, _303), 0u));
                            uint _1480 = _1478.x;
                            float _1489 = (float((_1470 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1490 = (float(_1470 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1494 = (1.0f - abs(_1489)) - abs(_1490);
                            float _1496 = clamp((-0.0f) - _1494, 0.0f, 1.0f);
                            float _1497 = (-0.0f) - _1496;
                            float _1502 = ((_1489 >= 0.0f) ? _1497 : _1496) + _1489;
                            float _1503 = ((_1490 >= 0.0f) ? _1497 : _1496) + _1490;
                            float _1507 = rsqrt(dot(float3(_1502, _1503, _1494), float3(_1502, _1503, _1494)));
                            float _1058 = float(_1455.y) * 0.0039215688593685626983642578125f;
                            float _1519 = (float((_1475 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1520 = (float(_1475 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1524 = (1.0f - abs(_1519)) - abs(_1520);
                            float _1526 = clamp((-0.0f) - _1524, 0.0f, 1.0f);
                            float _1527 = (-0.0f) - _1526;
                            float _1532 = ((_1519 >= 0.0f) ? _1527 : _1526) + _1519;
                            float _1533 = ((_1520 >= 0.0f) ? _1527 : _1526) + _1520;
                            float _1537 = rsqrt(dot(float3(_1532, _1533, _1524), float3(_1532, _1533, _1524)));
                            float _1547 = (float((_1480 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1548 = (float(_1480 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                            float _1552 = (1.0f - abs(_1547)) - abs(_1548);
                            float _1554 = clamp((-0.0f) - _1552, 0.0f, 1.0f);
                            float _1555 = (-0.0f) - _1554;
                            float _1560 = ((_1547 >= 0.0f) ? _1555 : _1554) + _1547;
                            float _1561 = ((_1548 >= 0.0f) ? _1555 : _1554) + _1548;
                            float _1565 = rsqrt(dot(float3(_1560, _1561, _1552), float3(_1560, _1561, _1552)));
                            float _1570 = asfloat(_10.Load(_430 * 115u).x);
                            bool _1593 = (_457 != 0u) && _461;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder = 0u;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_1 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_2 = _385;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_3 = _386;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_4 = _387;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_5 = _1502 * _1507;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_6 = _1503 * _1507;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_7 = _1507 * _1494;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_8 = _1532 * _1537;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_9 = _1533 * _1537;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_10 = _1537 * _1524;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_11 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_12 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_13 = _1593 ? 1.0f : _1461.x;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_14 = _1593 ? 1.0f : _1461.y;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_15 = _1593 ? 1.0f : _1461.z;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_16 = _1570;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_17 = _1570;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_18 = _1570;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_19 = float(_1470 & 255u) * 0.0039215688593685626983642578125f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_20 = _1058;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_21 = 1.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_22 = _1058;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_23 = 1.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_24 = float(_1455.z) * 0.0039215688593685626983642578125f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_25 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_26 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_27 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_28 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_29 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_30 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_31 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_33 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_34 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_35 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_36 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_37 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_38 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_39 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_40 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_41 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_42 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_43 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_44 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_45 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_46 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_47 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_48 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_49 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_50 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_51 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_52 = _10.Load((_430 * 115u) + 84u).x;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_53 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_54 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_55 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_56 = asfloat(_10.Load((_430 * 115u) + 95u).x);
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_57 = float(_1475 & 255u) * 0.0039215688593685626983642578125f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_58 = float(_1480 & 255u) * 0.0039215688593685626983642578125f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_59 = _1560 * _1565;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_60 = _1561 * _1565;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_61 = _1565 * _1552;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_62 = _1461.w;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_63 = asfloat(_10.Load((_430 * 115u) + 82u).x);
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_64 = _10.Load((_430 * 115u) + 83u).x;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_65 = 1.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_66 = 0u;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_67 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_68 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_69 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_70 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_71 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_72 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_73 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_74 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_75 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_76 = _448;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_77 = _422;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_78 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_79 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_80 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_81 = 1.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_82 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_83 = 1u;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_84 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_85 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_86 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_87 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_88 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_89 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_90 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_91 = _516;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_92 = _521;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_93 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_94 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_95 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_96 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_97 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_98 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_99 = 0u;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_100 = 0.0f;
                            frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_101 = 0.0f;
                        }
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_1 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_1;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_2 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_2;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_3 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_3;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_4 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_4;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_5 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_5;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_6 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_6;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_7 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_7;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_8 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_8;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_9 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_9;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_10 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_10;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_11 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_11;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_12 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_12;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_13 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_13;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_14 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_14;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_15 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_15;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_16 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_16;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_17 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_17;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_18 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_18;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_19 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_19;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_20 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_20;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_21 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_21;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_22 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_22;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_23 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_23;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_24;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_25 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_25;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_26 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_26;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_27 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_27;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_28 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_28;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_29 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_29;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_30 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_30;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_31 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_31;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_32 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_32;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_33 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_33;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_34 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_34;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_35 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_35;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_36 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_36;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_37 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_37;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_38 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_38;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_39 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_39;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_40 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_40;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_41 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_41;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_42 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_42;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_43 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_43;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_44 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_44;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_45 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_45;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_46 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_46;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_47 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_47;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_48 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_48;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_49 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_49;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_50 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_50;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_51 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_51;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_52 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_52;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_53 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_53;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_54 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_54;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_55 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_55;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_56 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_56;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_57 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_57;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_58 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_58;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_59 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_59;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_60 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_60;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_61 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_61;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_62 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_62;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_63 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_63;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_64 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_64;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_65 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_65;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_66 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_66;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_67 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_67;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_68 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_68;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_69 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_69;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_70 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_70;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_71 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_71;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_72 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_72;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_73 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_73;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_74 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_74;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_75 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_75;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_76 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_76;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_77 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_77;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_78 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_78;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_79 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_79;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_80 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_80;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_81 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_81;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_82 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_82;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_83 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_83;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_84 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_84;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_85 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_85;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_86 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_86;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_87 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_87;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_88 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_88;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_89 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_89;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_90 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_90;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_91 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_91;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_92 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_92;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_93 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_93;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_94 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_94;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_95 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_95;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_96 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_96;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_97 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_97;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_98 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_98;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_99 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_99;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_100 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_100;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_101 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24_ladder_101;
                    }
                    else
                    {
                        uint4 _1223 = _33[2u].Load(int3(uint2(_302, _303), 0u));
                        float4 _1229 = _29[3u].Load(int3(uint2(_302, _303), 0u));
                        uint4 _1236 = _33[1u].Load(int3(uint2(_302, _303), 0u));
                        uint _1238 = _1236.x;
                        float4 _1241 = _29[8u].Load(int3(uint2(_302, _303), 0u));
                        float _1252 = (float((_1238 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1253 = (float(_1238 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                        float _1257 = (1.0f - abs(_1252)) - abs(_1253);
                        float _1259 = clamp((-0.0f) - _1257, 0.0f, 1.0f);
                        float _1260 = (-0.0f) - _1259;
                        float _1265 = ((_1252 >= 0.0f) ? _1260 : _1259) + _1252;
                        float _1266 = ((_1253 >= 0.0f) ? _1260 : _1259) + _1253;
                        float _1270 = rsqrt(dot(float3(_1265, _1266, _1257), float3(_1265, _1266, _1257)));
                        float _1001 = _1265 * _1270;
                        float _1005 = _1266 * _1270;
                        float _1009 = _1270 * _1257;
                        float _1280 = asfloat(_10.Load(_430 * 115u).x);
                        bool _1288 = (_457 != 0u) && _461;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_1 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_2 = _385;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_3 = _386;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_4 = _387;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_5 = _1001;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_6 = _1005;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_7 = _1009;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_8 = _1001;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_9 = _1005;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_10 = _1009;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_11 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_12 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_13 = _1288 ? 1.0f : _1229.x;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_14 = _1288 ? 1.0f : _1229.y;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_15 = _1288 ? 1.0f : _1229.z;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_16 = _1280;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_17 = _1280;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_18 = _1280;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_19 = float(_1238 & 255u) * 0.0039215688593685626983642578125f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_20 = float(_1223.y) * 0.0039215688593685626983642578125f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_21 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_22 = float(_1223.z) * 0.0039215688593685626983642578125f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_23 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_25 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_26 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_27 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_28 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_29 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_30 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_31 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_32 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_33 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_34 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_35 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_36 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_37 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_38 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_39 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_40 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_41 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_42 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_43 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_44 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_45 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_46 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_47 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_48 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_49 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_50 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_51 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_52 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_53 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_54 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_55 = exp2(log2(_1241.w) * 2.2000000476837158203125f);
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_56 = asfloat(_10.Load((_430 * 115u) + 95u).x);
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_57 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_58 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_59 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_60 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_61 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_62 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_63 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_64 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_65 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_66 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_67 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_68 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_69 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_70 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_71 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_72 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_73 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_74 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_75 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_76 = _448;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_77 = _422;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_78 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_79 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_80 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_81 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_82 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_83 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_84 = _1229.w;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_85 = _1241.x;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_86 = _1241.y;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_87 = _1241.z;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_88 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_89 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_90 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_91 = _516;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_92 = _521;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_93 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_94 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_95 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_96 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_97 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_98 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_99 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_100 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_101 = 0.0f;
                    }
                    frontier_phi_21_7_ladder_10_ladder_13_ladder = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_1 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_1;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_2 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_2;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_3 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_3;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_4 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_4;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_5 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_5;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_6 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_6;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_7 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_7;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_8 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_8;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_9 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_9;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_10 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_10;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_11 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_11;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_12 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_12;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_13 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_13;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_14 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_14;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_15 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_15;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_16 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_16;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_17 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_17;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_18 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_18;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_19 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_19;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_20 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_20;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_21 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_21;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_22 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_22;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_23 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_23;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_24 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_24;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_25 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_25;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_26 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_26;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_27 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_27;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_28 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_28;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_29 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_29;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_30 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_30;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_31 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_31;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_32 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_32;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_33 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_33;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_34 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_34;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_35 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_35;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_36 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_36;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_37 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_37;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_38 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_38;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_39 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_39;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_40 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_40;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_41 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_41;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_42 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_42;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_43 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_43;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_44 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_44;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_45 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_45;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_46 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_46;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_47 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_47;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_48 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_48;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_49 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_49;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_50 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_50;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_51 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_51;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_52 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_52;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_53 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_53;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_54 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_54;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_55 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_55;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_56 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_56;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_57 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_57;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_58 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_58;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_59 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_59;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_60 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_60;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_61 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_61;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_62 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_62;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_63 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_63;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_64 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_64;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_65 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_65;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_66 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_66;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_67 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_67;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_68 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_68;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_69 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_69;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_70 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_70;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_71 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_71;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_72 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_72;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_73 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_73;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_74 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_74;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_75 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_75;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_76 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_76;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_77 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_77;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_78 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_78;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_79 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_79;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_80 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_80;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_81 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_81;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_82 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_82;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_83 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_83;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_84 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_84;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_85 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_85;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_86 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_86;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_87 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_87;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_88 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_88;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_89 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_89;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_90 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_90;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_91 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_91;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_92 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_92;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_93 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_93;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_94 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_94;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_95 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_95;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_96 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_96;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_97 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_97;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_98 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_98;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_99 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_99;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_100 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_100;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_101 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19_ladder_101;
                }
                else
                {
                    uint4 _851 = _33[2u].Load(int3(uint2(_302, _303), 0u));
                    float4 _857 = _29[3u].Load(int3(uint2(_302, _303), 0u));
                    float _859 = _857.x;
                    float _860 = _857.y;
                    float _861 = _857.z;
                    float _862 = _857.w;
                    uint4 _865 = _33[1u].Load(int3(uint2(_302, _303), 0u));
                    uint _867 = _865.x;
                    float4 _870 = _29[8u].Load(int3(uint2(_302, _303), 0u));
                    float _872 = _870.x;
                    float _873 = _870.y;
                    float _874 = _870.z;
                    float _875 = _870.w;
                    float _884 = (float((_867 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _885 = (float(_867 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _889 = (1.0f - abs(_884)) - abs(_885);
                    float _891 = clamp((-0.0f) - _889, 0.0f, 1.0f);
                    float _892 = (-0.0f) - _891;
                    float _897 = ((_884 >= 0.0f) ? _892 : _891) + _884;
                    float _898 = ((_885 >= 0.0f) ? _892 : _891) + _885;
                    float _902 = rsqrt(dot(float3(_897, _898, _889), float3(_897, _898, _889)));
                    float _903 = _897 * _902;
                    float _904 = _898 * _902;
                    float _905 = _902 * _889;
                    float _907 = float(_867 & 255u) * 0.0039215688593685626983642578125f;
                    float _909 = float(_851.y) * 0.0039215688593685626983642578125f;
                    float _940 = (_872 <= 0.040449999272823333740234375f) ? (_872 * 0.077399380505084991455078125f) : exp2(log2((abs(_872) + 0.054999999701976776123046875f) * 0.947867333889007568359375f) * 2.400000095367431640625f);
                    float _941 = (_873 <= 0.040449999272823333740234375f) ? (_873 * 0.077399380505084991455078125f) : exp2(log2((abs(_873) + 0.054999999701976776123046875f) * 0.947867333889007568359375f) * 2.400000095367431640625f);
                    float _942 = (_874 <= 0.040449999272823333740234375f) ? (_874 * 0.077399380505084991455078125f) : exp2(log2((abs(_874) + 0.054999999701976776123046875f) * 0.947867333889007568359375f) * 2.400000095367431640625f);
                    bool _945 = (_422 & 536870912u) == 0u;
                    float _1082;
                    float _1289;
                    if (_945)
                    {
                        _1289 = 1.0f;
                        _1082 = 1.0f;
                    }
                    else
                    {
                        uint4 _1325 = _33[9u].Load(int3(uint2(_302, _303), 0u));
                        uint _1327 = _1325.x;
                        _1289 = float((_1327 >> 8u) & 255u) * 0.0039215688593685626983642578125f;
                        _1082 = float(_1327 & 255u) * 0.0039215688593685626983642578125f;
                    }
                    float _1295 = asfloat(_10.Load(_430 * 115u).x);
                    uint _1297 = (_430 * 115u) + 2u;
                    float3 _1307 = asfloat(uint3(_10.Load(_1297).x, _10.Load(_1297 + 1u).x, _10.Load(_1297 + 2u).x));
                    float _1028;
                    float _1033;
                    float _1038;
                    if ((_457 != 0u) && _461)
                    {
                        _1028 = 1.0f;
                        _1033 = 1.0f;
                        _1038 = 1.0f;
                    }
                    else
                    {
                        float _1607 = 1.0f - _862;
                        _1028 = _1607 * _859;
                        _1033 = _1607 * _860;
                        _1038 = _1607 * _861;
                    }
                    float _1043 = ((_859 - _1295) * _862) + _1295;
                    float _1046 = ((_860 - _1295) * _862) + _1295;
                    float _1049 = ((_861 - _1295) * _862) + _1295;
                    float _1064 = exp2(log2(asfloat(_10.Load((_430 * 115u) + 96u).x) + _909) * asfloat(_10.Load((_430 * 115u) + 97u).x));
                    float _1122 = (_516 * 0.0039215688593685626983642578125f) * float(_851.z);
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_1;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_2;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_3;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_4;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_5;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_6;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_7;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_8;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_9;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_10;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_11;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_12;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_13;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_14;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_15;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_16;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_17;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_18;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_19;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_20;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_21;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_22;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_23;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_24;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_25;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_26;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_27;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_28;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_29;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_30;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_31;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_32;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_33;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_34;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_35;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_36;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_37;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_38;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_39;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_40;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_41;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_42;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_43;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_44;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_45;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_46;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_47;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_48;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_49;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_50;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_51;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_52;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_53;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_54;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_55;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_56;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_57;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_58;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_59;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_60;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_61;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_62;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_63;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_64;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_65;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_66;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_67;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_68;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_69;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_70;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_71;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_72;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_73;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_74;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_75;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_76;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_77;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_78;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_79;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_80;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_81;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_82;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_83;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_84;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_85;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_86;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_87;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_88;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_89;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_90;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_91;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_92;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_93;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_94;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_95;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_96;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_97;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_98;
                    uint frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_99;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_100;
                    float frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_101;
                    if (_945)
                    {
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_1 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_2 = _385;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_3 = _386;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_4 = _387;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_5 = _903;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_6 = _904;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_7 = _905;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_8 = _903;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_9 = _904;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_10 = _905;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_11 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_12 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_13 = _1028;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_14 = _1033;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_15 = _1038;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_16 = _1043;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_17 = _1046;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_18 = _1049;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_19 = _907;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_20 = _909;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_21 = _909;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_22 = _1064;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_23 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_24 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_25 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_26 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_27 = _941;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_28 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_29 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_30 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_31 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_32 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_33 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_34 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_35 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_36 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_37 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_38 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_39 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_40 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_41 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_42 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_43 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_44 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_45 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_46 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_47 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_48 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_49 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_50 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_51 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_52 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_53 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_54 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_55 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_56 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_57 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_58 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_59 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_60 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_61 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_62 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_63 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_64 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_65 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_66 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_67 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_68 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_69 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_70 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_71 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_72 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_73 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_74 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_75 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_76 = _448;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_77 = _422;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_78 = _940;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_79 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_80 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_81 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_82 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_83 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_84 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_85 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_86 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_87 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_88 = _942;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_89 = _875;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_90 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_91 = _1122;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_92 = _521;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_93 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_94 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_95 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_96 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_97 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_98 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_99 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_100 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_101 = 0.0f;
                    }
                    else
                    {
                        float _1701 = ((_1033 * 0.589999973773956298828125f) + (_1028 * 0.300000011920928955078125f)) + (_1038 * 0.10999999940395355224609375f);
                        float _1711 = _1289 * _483;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_1 = (_1307.y * _1711) * (((_1033 - _1701) * _478) + _1701);
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_2 = _385;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_3 = _386;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_4 = _387;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_5 = _903;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_6 = _904;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_7 = _905;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_8 = _903;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_9 = _904;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_10 = _905;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_11 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_12 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_13 = _1028;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_14 = _1033;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_15 = _1038;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_16 = _1043;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_17 = _1046;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_18 = _1049;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_19 = _907;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_20 = _909;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_21 = _909;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_22 = _1064;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_23 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_24 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_25 = (_1307.x * _1711) * (((_1028 - _1701) * _478) + _1701);
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_26 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_27 = _941;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_28 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_29 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_30 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_31 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_32 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_33 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_34 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_35 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_36 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_37 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_38 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_39 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_40 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_41 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_42 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_43 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_44 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_45 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_46 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_47 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_48 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_49 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_50 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_51 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_52 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_53 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_54 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_55 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_56 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_57 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_58 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_59 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_60 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_61 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_62 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_63 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_64 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_65 = 1.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_66 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_67 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_68 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_69 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_70 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_71 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_72 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_73 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_74 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_75 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_76 = _448;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_77 = _422;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_78 = _940;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_79 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_80 = (_1307.z * _1711) * (((_1038 - _1701) * _478) + _1701);
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_81 = _1082;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_82 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_83 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_84 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_85 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_86 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_87 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_88 = _942;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_89 = _875;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_90 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_91 = _1122;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_92 = _521;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_93 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_94 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_95 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_96 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_97 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_98 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_99 = 0u;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_100 = 0.0f;
                        frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_101 = 0.0f;
                    }
                    frontier_phi_21_7_ladder_10_ladder_13_ladder = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_1 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_1;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_2 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_2;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_3 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_3;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_4 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_4;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_5 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_5;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_6 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_6;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_7 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_7;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_8 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_8;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_9 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_9;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_10 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_10;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_11 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_11;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_12 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_12;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_13 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_13;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_14 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_14;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_15 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_15;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_16 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_16;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_17 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_17;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_18 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_18;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_19 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_19;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_20 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_20;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_21 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_21;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_22 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_22;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_23 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_23;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_24 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_24;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_25 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_25;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_26 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_26;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_27 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_27;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_28 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_28;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_29 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_29;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_30 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_30;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_31 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_31;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_32 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_32;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_33 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_33;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_34 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_34;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_35 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_35;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_36 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_36;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_37 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_37;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_38 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_38;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_39 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_39;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_40 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_40;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_41 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_41;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_42 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_42;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_43 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_43;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_44 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_44;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_45 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_45;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_46 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_46;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_47 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_47;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_48 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_48;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_49 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_49;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_50 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_50;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_51 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_51;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_52 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_52;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_53 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_53;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_54 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_54;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_55 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_55;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_56 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_56;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_57 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_57;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_58 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_58;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_59 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_59;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_60 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_60;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_61 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_61;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_62 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_62;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_63 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_63;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_64 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_64;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_65 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_65;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_66 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_66;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_67 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_67;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_68 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_68;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_69 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_69;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_70 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_70;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_71 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_71;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_72 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_72;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_73 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_73;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_74 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_74;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_75 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_75;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_76 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_76;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_77 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_77;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_78 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_78;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_79 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_79;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_80 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_80;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_81 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_81;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_82 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_82;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_83 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_83;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_84 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_84;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_85 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_85;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_86 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_86;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_87 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_87;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_88 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_88;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_89 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_89;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_90 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_90;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_91 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_91;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_92 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_92;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_93 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_93;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_94 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_94;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_95 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_95;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_96 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_96;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_97 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_97;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_98 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_98;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_99 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_99;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_100 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_100;
                    frontier_phi_21_7_ladder_10_ladder_13_ladder_101 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34_ladder_101;
                }
                frontier_phi_21_7_ladder_10_ladder = frontier_phi_21_7_ladder_10_ladder_13_ladder;
                frontier_phi_21_7_ladder_10_ladder_1 = frontier_phi_21_7_ladder_10_ladder_13_ladder_1;
                frontier_phi_21_7_ladder_10_ladder_2 = frontier_phi_21_7_ladder_10_ladder_13_ladder_2;
                frontier_phi_21_7_ladder_10_ladder_3 = frontier_phi_21_7_ladder_10_ladder_13_ladder_3;
                frontier_phi_21_7_ladder_10_ladder_4 = frontier_phi_21_7_ladder_10_ladder_13_ladder_4;
                frontier_phi_21_7_ladder_10_ladder_5 = frontier_phi_21_7_ladder_10_ladder_13_ladder_5;
                frontier_phi_21_7_ladder_10_ladder_6 = frontier_phi_21_7_ladder_10_ladder_13_ladder_6;
                frontier_phi_21_7_ladder_10_ladder_7 = frontier_phi_21_7_ladder_10_ladder_13_ladder_7;
                frontier_phi_21_7_ladder_10_ladder_8 = frontier_phi_21_7_ladder_10_ladder_13_ladder_8;
                frontier_phi_21_7_ladder_10_ladder_9 = frontier_phi_21_7_ladder_10_ladder_13_ladder_9;
                frontier_phi_21_7_ladder_10_ladder_10 = frontier_phi_21_7_ladder_10_ladder_13_ladder_10;
                frontier_phi_21_7_ladder_10_ladder_11 = frontier_phi_21_7_ladder_10_ladder_13_ladder_11;
                frontier_phi_21_7_ladder_10_ladder_12 = frontier_phi_21_7_ladder_10_ladder_13_ladder_12;
                frontier_phi_21_7_ladder_10_ladder_13 = frontier_phi_21_7_ladder_10_ladder_13_ladder_13;
                frontier_phi_21_7_ladder_10_ladder_14 = frontier_phi_21_7_ladder_10_ladder_13_ladder_14;
                frontier_phi_21_7_ladder_10_ladder_15 = frontier_phi_21_7_ladder_10_ladder_13_ladder_15;
                frontier_phi_21_7_ladder_10_ladder_16 = frontier_phi_21_7_ladder_10_ladder_13_ladder_16;
                frontier_phi_21_7_ladder_10_ladder_17 = frontier_phi_21_7_ladder_10_ladder_13_ladder_17;
                frontier_phi_21_7_ladder_10_ladder_18 = frontier_phi_21_7_ladder_10_ladder_13_ladder_18;
                frontier_phi_21_7_ladder_10_ladder_19 = frontier_phi_21_7_ladder_10_ladder_13_ladder_19;
                frontier_phi_21_7_ladder_10_ladder_20 = frontier_phi_21_7_ladder_10_ladder_13_ladder_20;
                frontier_phi_21_7_ladder_10_ladder_21 = frontier_phi_21_7_ladder_10_ladder_13_ladder_21;
                frontier_phi_21_7_ladder_10_ladder_22 = frontier_phi_21_7_ladder_10_ladder_13_ladder_22;
                frontier_phi_21_7_ladder_10_ladder_23 = frontier_phi_21_7_ladder_10_ladder_13_ladder_23;
                frontier_phi_21_7_ladder_10_ladder_24 = frontier_phi_21_7_ladder_10_ladder_13_ladder_24;
                frontier_phi_21_7_ladder_10_ladder_25 = frontier_phi_21_7_ladder_10_ladder_13_ladder_25;
                frontier_phi_21_7_ladder_10_ladder_26 = frontier_phi_21_7_ladder_10_ladder_13_ladder_26;
                frontier_phi_21_7_ladder_10_ladder_27 = frontier_phi_21_7_ladder_10_ladder_13_ladder_27;
                frontier_phi_21_7_ladder_10_ladder_28 = frontier_phi_21_7_ladder_10_ladder_13_ladder_28;
                frontier_phi_21_7_ladder_10_ladder_29 = frontier_phi_21_7_ladder_10_ladder_13_ladder_29;
                frontier_phi_21_7_ladder_10_ladder_30 = frontier_phi_21_7_ladder_10_ladder_13_ladder_30;
                frontier_phi_21_7_ladder_10_ladder_31 = frontier_phi_21_7_ladder_10_ladder_13_ladder_31;
                frontier_phi_21_7_ladder_10_ladder_32 = frontier_phi_21_7_ladder_10_ladder_13_ladder_32;
                frontier_phi_21_7_ladder_10_ladder_33 = frontier_phi_21_7_ladder_10_ladder_13_ladder_33;
                frontier_phi_21_7_ladder_10_ladder_34 = frontier_phi_21_7_ladder_10_ladder_13_ladder_34;
                frontier_phi_21_7_ladder_10_ladder_35 = frontier_phi_21_7_ladder_10_ladder_13_ladder_35;
                frontier_phi_21_7_ladder_10_ladder_36 = frontier_phi_21_7_ladder_10_ladder_13_ladder_36;
                frontier_phi_21_7_ladder_10_ladder_37 = frontier_phi_21_7_ladder_10_ladder_13_ladder_37;
                frontier_phi_21_7_ladder_10_ladder_38 = frontier_phi_21_7_ladder_10_ladder_13_ladder_38;
                frontier_phi_21_7_ladder_10_ladder_39 = frontier_phi_21_7_ladder_10_ladder_13_ladder_39;
                frontier_phi_21_7_ladder_10_ladder_40 = frontier_phi_21_7_ladder_10_ladder_13_ladder_40;
                frontier_phi_21_7_ladder_10_ladder_41 = frontier_phi_21_7_ladder_10_ladder_13_ladder_41;
                frontier_phi_21_7_ladder_10_ladder_42 = frontier_phi_21_7_ladder_10_ladder_13_ladder_42;
                frontier_phi_21_7_ladder_10_ladder_43 = frontier_phi_21_7_ladder_10_ladder_13_ladder_43;
                frontier_phi_21_7_ladder_10_ladder_44 = frontier_phi_21_7_ladder_10_ladder_13_ladder_44;
                frontier_phi_21_7_ladder_10_ladder_45 = frontier_phi_21_7_ladder_10_ladder_13_ladder_45;
                frontier_phi_21_7_ladder_10_ladder_46 = frontier_phi_21_7_ladder_10_ladder_13_ladder_46;
                frontier_phi_21_7_ladder_10_ladder_47 = frontier_phi_21_7_ladder_10_ladder_13_ladder_47;
                frontier_phi_21_7_ladder_10_ladder_48 = frontier_phi_21_7_ladder_10_ladder_13_ladder_48;
                frontier_phi_21_7_ladder_10_ladder_49 = frontier_phi_21_7_ladder_10_ladder_13_ladder_49;
                frontier_phi_21_7_ladder_10_ladder_50 = frontier_phi_21_7_ladder_10_ladder_13_ladder_50;
                frontier_phi_21_7_ladder_10_ladder_51 = frontier_phi_21_7_ladder_10_ladder_13_ladder_51;
                frontier_phi_21_7_ladder_10_ladder_52 = frontier_phi_21_7_ladder_10_ladder_13_ladder_52;
                frontier_phi_21_7_ladder_10_ladder_53 = frontier_phi_21_7_ladder_10_ladder_13_ladder_53;
                frontier_phi_21_7_ladder_10_ladder_54 = frontier_phi_21_7_ladder_10_ladder_13_ladder_54;
                frontier_phi_21_7_ladder_10_ladder_55 = frontier_phi_21_7_ladder_10_ladder_13_ladder_55;
                frontier_phi_21_7_ladder_10_ladder_56 = frontier_phi_21_7_ladder_10_ladder_13_ladder_56;
                frontier_phi_21_7_ladder_10_ladder_57 = frontier_phi_21_7_ladder_10_ladder_13_ladder_57;
                frontier_phi_21_7_ladder_10_ladder_58 = frontier_phi_21_7_ladder_10_ladder_13_ladder_58;
                frontier_phi_21_7_ladder_10_ladder_59 = frontier_phi_21_7_ladder_10_ladder_13_ladder_59;
                frontier_phi_21_7_ladder_10_ladder_60 = frontier_phi_21_7_ladder_10_ladder_13_ladder_60;
                frontier_phi_21_7_ladder_10_ladder_61 = frontier_phi_21_7_ladder_10_ladder_13_ladder_61;
                frontier_phi_21_7_ladder_10_ladder_62 = frontier_phi_21_7_ladder_10_ladder_13_ladder_62;
                frontier_phi_21_7_ladder_10_ladder_63 = frontier_phi_21_7_ladder_10_ladder_13_ladder_63;
                frontier_phi_21_7_ladder_10_ladder_64 = frontier_phi_21_7_ladder_10_ladder_13_ladder_64;
                frontier_phi_21_7_ladder_10_ladder_65 = frontier_phi_21_7_ladder_10_ladder_13_ladder_65;
                frontier_phi_21_7_ladder_10_ladder_66 = frontier_phi_21_7_ladder_10_ladder_13_ladder_66;
                frontier_phi_21_7_ladder_10_ladder_67 = frontier_phi_21_7_ladder_10_ladder_13_ladder_67;
                frontier_phi_21_7_ladder_10_ladder_68 = frontier_phi_21_7_ladder_10_ladder_13_ladder_68;
                frontier_phi_21_7_ladder_10_ladder_69 = frontier_phi_21_7_ladder_10_ladder_13_ladder_69;
                frontier_phi_21_7_ladder_10_ladder_70 = frontier_phi_21_7_ladder_10_ladder_13_ladder_70;
                frontier_phi_21_7_ladder_10_ladder_71 = frontier_phi_21_7_ladder_10_ladder_13_ladder_71;
                frontier_phi_21_7_ladder_10_ladder_72 = frontier_phi_21_7_ladder_10_ladder_13_ladder_72;
                frontier_phi_21_7_ladder_10_ladder_73 = frontier_phi_21_7_ladder_10_ladder_13_ladder_73;
                frontier_phi_21_7_ladder_10_ladder_74 = frontier_phi_21_7_ladder_10_ladder_13_ladder_74;
                frontier_phi_21_7_ladder_10_ladder_75 = frontier_phi_21_7_ladder_10_ladder_13_ladder_75;
                frontier_phi_21_7_ladder_10_ladder_76 = frontier_phi_21_7_ladder_10_ladder_13_ladder_76;
                frontier_phi_21_7_ladder_10_ladder_77 = frontier_phi_21_7_ladder_10_ladder_13_ladder_77;
                frontier_phi_21_7_ladder_10_ladder_78 = frontier_phi_21_7_ladder_10_ladder_13_ladder_78;
                frontier_phi_21_7_ladder_10_ladder_79 = frontier_phi_21_7_ladder_10_ladder_13_ladder_79;
                frontier_phi_21_7_ladder_10_ladder_80 = frontier_phi_21_7_ladder_10_ladder_13_ladder_80;
                frontier_phi_21_7_ladder_10_ladder_81 = frontier_phi_21_7_ladder_10_ladder_13_ladder_81;
                frontier_phi_21_7_ladder_10_ladder_82 = frontier_phi_21_7_ladder_10_ladder_13_ladder_82;
                frontier_phi_21_7_ladder_10_ladder_83 = frontier_phi_21_7_ladder_10_ladder_13_ladder_83;
                frontier_phi_21_7_ladder_10_ladder_84 = frontier_phi_21_7_ladder_10_ladder_13_ladder_84;
                frontier_phi_21_7_ladder_10_ladder_85 = frontier_phi_21_7_ladder_10_ladder_13_ladder_85;
                frontier_phi_21_7_ladder_10_ladder_86 = frontier_phi_21_7_ladder_10_ladder_13_ladder_86;
                frontier_phi_21_7_ladder_10_ladder_87 = frontier_phi_21_7_ladder_10_ladder_13_ladder_87;
                frontier_phi_21_7_ladder_10_ladder_88 = frontier_phi_21_7_ladder_10_ladder_13_ladder_88;
                frontier_phi_21_7_ladder_10_ladder_89 = frontier_phi_21_7_ladder_10_ladder_13_ladder_89;
                frontier_phi_21_7_ladder_10_ladder_90 = frontier_phi_21_7_ladder_10_ladder_13_ladder_90;
                frontier_phi_21_7_ladder_10_ladder_91 = frontier_phi_21_7_ladder_10_ladder_13_ladder_91;
                frontier_phi_21_7_ladder_10_ladder_92 = frontier_phi_21_7_ladder_10_ladder_13_ladder_92;
                frontier_phi_21_7_ladder_10_ladder_93 = frontier_phi_21_7_ladder_10_ladder_13_ladder_93;
                frontier_phi_21_7_ladder_10_ladder_94 = frontier_phi_21_7_ladder_10_ladder_13_ladder_94;
                frontier_phi_21_7_ladder_10_ladder_95 = frontier_phi_21_7_ladder_10_ladder_13_ladder_95;
                frontier_phi_21_7_ladder_10_ladder_96 = frontier_phi_21_7_ladder_10_ladder_13_ladder_96;
                frontier_phi_21_7_ladder_10_ladder_97 = frontier_phi_21_7_ladder_10_ladder_13_ladder_97;
                frontier_phi_21_7_ladder_10_ladder_98 = frontier_phi_21_7_ladder_10_ladder_13_ladder_98;
                frontier_phi_21_7_ladder_10_ladder_99 = frontier_phi_21_7_ladder_10_ladder_13_ladder_99;
                frontier_phi_21_7_ladder_10_ladder_100 = frontier_phi_21_7_ladder_10_ladder_13_ladder_100;
                frontier_phi_21_7_ladder_10_ladder_101 = frontier_phi_21_7_ladder_10_ladder_13_ladder_101;
            }
            else
            {
                uint4 _701 = _33[2u].Load(int3(uint2(_302, _303), 0u));
                float4 _707 = _29[3u].Load(int3(uint2(_302, _303), 0u));
                uint4 _715 = _33[1u].Load(int3(uint2(_302, _303), 0u));
                uint _717 = _715.x;
                float _726 = (float((_717 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                float _727 = (float(_717 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                float _731 = (1.0f - abs(_726)) - abs(_727);
                float _733 = clamp((-0.0f) - _731, 0.0f, 1.0f);
                float _734 = (-0.0f) - _733;
                float _739 = ((_726 >= 0.0f) ? _734 : _733) + _726;
                float _740 = ((_727 >= 0.0f) ? _734 : _733) + _727;
                float _744 = rsqrt(dot(float3(_739, _740, _731), float3(_739, _740, _731)));
                float _745 = _739 * _744;
                float _746 = _740 * _744;
                float _747 = _744 * _731;
                float _751 = float(_701.y) * 0.0039215688593685626983642578125f;
                float _756 = asfloat(_10.Load(_430 * 115u).x);
                uint _758 = (_430 * 115u) + 2u;
                float3 _768 = asfloat(uint3(_10.Load(_758).x, _10.Load(_758 + 1u).x, _10.Load(_758 + 2u).x));
                bool _785 = (_457 != 0u) && _461;
                float _786 = _785 ? 1.0f : _707.x;
                float _787 = _785 ? 1.0f : _707.y;
                float _788 = _785 ? 1.0f : _707.z;
                float _800 = ((_788 * 0.10999999940395355224609375f) + (_786 * 0.300000011920928955078125f)) + (_787 * 0.589999973773956298828125f);
                float _811 = (_483 * 0.0039215688593685626983642578125f) * float(_701.z);
                bool _819 = (_422 & 256u) != 0u;
                frontier_phi_21_7_ladder_10_ladder = 0u;
                frontier_phi_21_7_ladder_10_ladder_1 = ((((_787 - _800) * _478) + _800) * _811) * _768.y;
                frontier_phi_21_7_ladder_10_ladder_2 = _385;
                frontier_phi_21_7_ladder_10_ladder_3 = _386;
                frontier_phi_21_7_ladder_10_ladder_4 = _387;
                frontier_phi_21_7_ladder_10_ladder_5 = _745;
                frontier_phi_21_7_ladder_10_ladder_6 = _746;
                frontier_phi_21_7_ladder_10_ladder_7 = _747;
                frontier_phi_21_7_ladder_10_ladder_8 = _745;
                frontier_phi_21_7_ladder_10_ladder_9 = _746;
                frontier_phi_21_7_ladder_10_ladder_10 = _747;
                frontier_phi_21_7_ladder_10_ladder_11 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_12 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_13 = _786;
                frontier_phi_21_7_ladder_10_ladder_14 = _787;
                frontier_phi_21_7_ladder_10_ladder_15 = _788;
                frontier_phi_21_7_ladder_10_ladder_16 = _756;
                frontier_phi_21_7_ladder_10_ladder_17 = _756;
                frontier_phi_21_7_ladder_10_ladder_18 = _756;
                frontier_phi_21_7_ladder_10_ladder_19 = float(_717 & 255u) * 0.0039215688593685626983642578125f;
                frontier_phi_21_7_ladder_10_ladder_20 = _751;
                frontier_phi_21_7_ladder_10_ladder_21 = 1.0f;
                frontier_phi_21_7_ladder_10_ladder_22 = exp2(log2(asfloat(_10.Load((_430 * 115u) + 96u).x) + _751) * asfloat(_10.Load((_430 * 115u) + 97u).x));
                frontier_phi_21_7_ladder_10_ladder_23 = 1.0f;
                frontier_phi_21_7_ladder_10_ladder_24 = 1.0f;
                frontier_phi_21_7_ladder_10_ladder_25 = ((((_786 - _800) * _478) + _800) * _811) * _768.x;
                frontier_phi_21_7_ladder_10_ladder_26 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_27 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_28 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_29 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_30 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_31 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_32 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_33 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_34 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_35 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_36 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_37 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_38 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_39 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_40 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_41 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_42 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_43 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_44 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_45 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_46 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_47 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_48 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_49 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_50 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_51 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_52 = 0u;
                frontier_phi_21_7_ladder_10_ladder_53 = _819 ? _538 : 0.0f;
                frontier_phi_21_7_ladder_10_ladder_54 = _819 ? _544 : 0.0f;
                frontier_phi_21_7_ladder_10_ladder_55 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_56 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_57 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_58 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_59 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_60 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_61 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_62 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_63 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_64 = 0u;
                frontier_phi_21_7_ladder_10_ladder_65 = 1.0f;
                frontier_phi_21_7_ladder_10_ladder_66 = 0u;
                frontier_phi_21_7_ladder_10_ladder_67 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_68 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_69 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_70 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_71 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_72 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_73 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_74 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_75 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_76 = _448;
                frontier_phi_21_7_ladder_10_ladder_77 = _422;
                frontier_phi_21_7_ladder_10_ladder_78 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_79 = _819 ? _532 : 0.0f;
                frontier_phi_21_7_ladder_10_ladder_80 = ((((_788 - _800) * _478) + _800) * _811) * _768.z;
                frontier_phi_21_7_ladder_10_ladder_81 = _707.w;
                frontier_phi_21_7_ladder_10_ladder_82 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_83 = 0u;
                frontier_phi_21_7_ladder_10_ladder_84 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_85 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_86 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_87 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_88 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_89 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_90 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_91 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_92 = 0u;
                frontier_phi_21_7_ladder_10_ladder_93 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_94 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_95 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_96 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_97 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_98 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_99 = 0u;
                frontier_phi_21_7_ladder_10_ladder_100 = 0.0f;
                frontier_phi_21_7_ladder_10_ladder_101 = 0.0f;
            }
            frontier_phi_21_7_ladder = frontier_phi_21_7_ladder_10_ladder;
            frontier_phi_21_7_ladder_1 = frontier_phi_21_7_ladder_10_ladder_1;
            frontier_phi_21_7_ladder_2 = frontier_phi_21_7_ladder_10_ladder_2;
            frontier_phi_21_7_ladder_3 = frontier_phi_21_7_ladder_10_ladder_3;
            frontier_phi_21_7_ladder_4 = frontier_phi_21_7_ladder_10_ladder_4;
            frontier_phi_21_7_ladder_5 = frontier_phi_21_7_ladder_10_ladder_5;
            frontier_phi_21_7_ladder_6 = frontier_phi_21_7_ladder_10_ladder_6;
            frontier_phi_21_7_ladder_7 = frontier_phi_21_7_ladder_10_ladder_7;
            frontier_phi_21_7_ladder_8 = frontier_phi_21_7_ladder_10_ladder_8;
            frontier_phi_21_7_ladder_9 = frontier_phi_21_7_ladder_10_ladder_9;
            frontier_phi_21_7_ladder_10 = frontier_phi_21_7_ladder_10_ladder_10;
            frontier_phi_21_7_ladder_11 = frontier_phi_21_7_ladder_10_ladder_11;
            frontier_phi_21_7_ladder_12 = frontier_phi_21_7_ladder_10_ladder_12;
            frontier_phi_21_7_ladder_13 = frontier_phi_21_7_ladder_10_ladder_13;
            frontier_phi_21_7_ladder_14 = frontier_phi_21_7_ladder_10_ladder_14;
            frontier_phi_21_7_ladder_15 = frontier_phi_21_7_ladder_10_ladder_15;
            frontier_phi_21_7_ladder_16 = frontier_phi_21_7_ladder_10_ladder_16;
            frontier_phi_21_7_ladder_17 = frontier_phi_21_7_ladder_10_ladder_17;
            frontier_phi_21_7_ladder_18 = frontier_phi_21_7_ladder_10_ladder_18;
            frontier_phi_21_7_ladder_19 = frontier_phi_21_7_ladder_10_ladder_19;
            frontier_phi_21_7_ladder_20 = frontier_phi_21_7_ladder_10_ladder_20;
            frontier_phi_21_7_ladder_21 = frontier_phi_21_7_ladder_10_ladder_21;
            frontier_phi_21_7_ladder_22 = frontier_phi_21_7_ladder_10_ladder_22;
            frontier_phi_21_7_ladder_23 = frontier_phi_21_7_ladder_10_ladder_23;
            frontier_phi_21_7_ladder_24 = frontier_phi_21_7_ladder_10_ladder_24;
            frontier_phi_21_7_ladder_25 = frontier_phi_21_7_ladder_10_ladder_25;
            frontier_phi_21_7_ladder_26 = frontier_phi_21_7_ladder_10_ladder_26;
            frontier_phi_21_7_ladder_27 = frontier_phi_21_7_ladder_10_ladder_27;
            frontier_phi_21_7_ladder_28 = frontier_phi_21_7_ladder_10_ladder_28;
            frontier_phi_21_7_ladder_29 = frontier_phi_21_7_ladder_10_ladder_29;
            frontier_phi_21_7_ladder_30 = frontier_phi_21_7_ladder_10_ladder_30;
            frontier_phi_21_7_ladder_31 = frontier_phi_21_7_ladder_10_ladder_31;
            frontier_phi_21_7_ladder_32 = frontier_phi_21_7_ladder_10_ladder_32;
            frontier_phi_21_7_ladder_33 = frontier_phi_21_7_ladder_10_ladder_33;
            frontier_phi_21_7_ladder_34 = frontier_phi_21_7_ladder_10_ladder_34;
            frontier_phi_21_7_ladder_35 = frontier_phi_21_7_ladder_10_ladder_35;
            frontier_phi_21_7_ladder_36 = frontier_phi_21_7_ladder_10_ladder_36;
            frontier_phi_21_7_ladder_37 = frontier_phi_21_7_ladder_10_ladder_37;
            frontier_phi_21_7_ladder_38 = frontier_phi_21_7_ladder_10_ladder_38;
            frontier_phi_21_7_ladder_39 = frontier_phi_21_7_ladder_10_ladder_39;
            frontier_phi_21_7_ladder_40 = frontier_phi_21_7_ladder_10_ladder_40;
            frontier_phi_21_7_ladder_41 = frontier_phi_21_7_ladder_10_ladder_41;
            frontier_phi_21_7_ladder_42 = frontier_phi_21_7_ladder_10_ladder_42;
            frontier_phi_21_7_ladder_43 = frontier_phi_21_7_ladder_10_ladder_43;
            frontier_phi_21_7_ladder_44 = frontier_phi_21_7_ladder_10_ladder_44;
            frontier_phi_21_7_ladder_45 = frontier_phi_21_7_ladder_10_ladder_45;
            frontier_phi_21_7_ladder_46 = frontier_phi_21_7_ladder_10_ladder_46;
            frontier_phi_21_7_ladder_47 = frontier_phi_21_7_ladder_10_ladder_47;
            frontier_phi_21_7_ladder_48 = frontier_phi_21_7_ladder_10_ladder_48;
            frontier_phi_21_7_ladder_49 = frontier_phi_21_7_ladder_10_ladder_49;
            frontier_phi_21_7_ladder_50 = frontier_phi_21_7_ladder_10_ladder_50;
            frontier_phi_21_7_ladder_51 = frontier_phi_21_7_ladder_10_ladder_51;
            frontier_phi_21_7_ladder_52 = frontier_phi_21_7_ladder_10_ladder_52;
            frontier_phi_21_7_ladder_53 = frontier_phi_21_7_ladder_10_ladder_53;
            frontier_phi_21_7_ladder_54 = frontier_phi_21_7_ladder_10_ladder_54;
            frontier_phi_21_7_ladder_55 = frontier_phi_21_7_ladder_10_ladder_55;
            frontier_phi_21_7_ladder_56 = frontier_phi_21_7_ladder_10_ladder_56;
            frontier_phi_21_7_ladder_57 = frontier_phi_21_7_ladder_10_ladder_57;
            frontier_phi_21_7_ladder_58 = frontier_phi_21_7_ladder_10_ladder_58;
            frontier_phi_21_7_ladder_59 = frontier_phi_21_7_ladder_10_ladder_59;
            frontier_phi_21_7_ladder_60 = frontier_phi_21_7_ladder_10_ladder_60;
            frontier_phi_21_7_ladder_61 = frontier_phi_21_7_ladder_10_ladder_61;
            frontier_phi_21_7_ladder_62 = frontier_phi_21_7_ladder_10_ladder_62;
            frontier_phi_21_7_ladder_63 = frontier_phi_21_7_ladder_10_ladder_63;
            frontier_phi_21_7_ladder_64 = frontier_phi_21_7_ladder_10_ladder_64;
            frontier_phi_21_7_ladder_65 = frontier_phi_21_7_ladder_10_ladder_65;
            frontier_phi_21_7_ladder_66 = frontier_phi_21_7_ladder_10_ladder_66;
            frontier_phi_21_7_ladder_67 = frontier_phi_21_7_ladder_10_ladder_67;
            frontier_phi_21_7_ladder_68 = frontier_phi_21_7_ladder_10_ladder_68;
            frontier_phi_21_7_ladder_69 = frontier_phi_21_7_ladder_10_ladder_69;
            frontier_phi_21_7_ladder_70 = frontier_phi_21_7_ladder_10_ladder_70;
            frontier_phi_21_7_ladder_71 = frontier_phi_21_7_ladder_10_ladder_71;
            frontier_phi_21_7_ladder_72 = frontier_phi_21_7_ladder_10_ladder_72;
            frontier_phi_21_7_ladder_73 = frontier_phi_21_7_ladder_10_ladder_73;
            frontier_phi_21_7_ladder_74 = frontier_phi_21_7_ladder_10_ladder_74;
            frontier_phi_21_7_ladder_75 = frontier_phi_21_7_ladder_10_ladder_75;
            frontier_phi_21_7_ladder_76 = frontier_phi_21_7_ladder_10_ladder_76;
            frontier_phi_21_7_ladder_77 = frontier_phi_21_7_ladder_10_ladder_77;
            frontier_phi_21_7_ladder_78 = frontier_phi_21_7_ladder_10_ladder_78;
            frontier_phi_21_7_ladder_79 = frontier_phi_21_7_ladder_10_ladder_79;
            frontier_phi_21_7_ladder_80 = frontier_phi_21_7_ladder_10_ladder_80;
            frontier_phi_21_7_ladder_81 = frontier_phi_21_7_ladder_10_ladder_81;
            frontier_phi_21_7_ladder_82 = frontier_phi_21_7_ladder_10_ladder_82;
            frontier_phi_21_7_ladder_83 = frontier_phi_21_7_ladder_10_ladder_83;
            frontier_phi_21_7_ladder_84 = frontier_phi_21_7_ladder_10_ladder_84;
            frontier_phi_21_7_ladder_85 = frontier_phi_21_7_ladder_10_ladder_85;
            frontier_phi_21_7_ladder_86 = frontier_phi_21_7_ladder_10_ladder_86;
            frontier_phi_21_7_ladder_87 = frontier_phi_21_7_ladder_10_ladder_87;
            frontier_phi_21_7_ladder_88 = frontier_phi_21_7_ladder_10_ladder_88;
            frontier_phi_21_7_ladder_89 = frontier_phi_21_7_ladder_10_ladder_89;
            frontier_phi_21_7_ladder_90 = frontier_phi_21_7_ladder_10_ladder_90;
            frontier_phi_21_7_ladder_91 = frontier_phi_21_7_ladder_10_ladder_91;
            frontier_phi_21_7_ladder_92 = frontier_phi_21_7_ladder_10_ladder_92;
            frontier_phi_21_7_ladder_93 = frontier_phi_21_7_ladder_10_ladder_93;
            frontier_phi_21_7_ladder_94 = frontier_phi_21_7_ladder_10_ladder_94;
            frontier_phi_21_7_ladder_95 = frontier_phi_21_7_ladder_10_ladder_95;
            frontier_phi_21_7_ladder_96 = frontier_phi_21_7_ladder_10_ladder_96;
            frontier_phi_21_7_ladder_97 = frontier_phi_21_7_ladder_10_ladder_97;
            frontier_phi_21_7_ladder_98 = frontier_phi_21_7_ladder_10_ladder_98;
            frontier_phi_21_7_ladder_99 = frontier_phi_21_7_ladder_10_ladder_99;
            frontier_phi_21_7_ladder_100 = frontier_phi_21_7_ladder_10_ladder_100;
            frontier_phi_21_7_ladder_101 = frontier_phi_21_7_ladder_10_ladder_101;
        }
        else
        {
            uint4 _596 = _33[2u].Load(int3(uint2(_302, _303), 0u));
            float4 _602 = _29[3u].Load(int3(uint2(_302, _303), 0u));
            float _604 = _602.x;
            float _605 = _602.y;
            float _606 = _602.z;
            float _607 = _602.w;
            uint4 _610 = _33[1u].Load(int3(uint2(_302, _303), 0u));
            uint _612 = _610.x;
            float _624 = (float((_612 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
            float _625 = (float(_612 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
            float _629 = (1.0f - abs(_624)) - abs(_625);
            float _633 = clamp((-0.0f) - _629, 0.0f, 1.0f);
            float _634 = (-0.0f) - _633;
            float _639 = ((_624 >= 0.0f) ? _634 : _633) + _624;
            float _640 = ((_625 >= 0.0f) ? _634 : _633) + _625;
            float _644 = rsqrt(dot(float3(_639, _640, _629), float3(_639, _640, _629)));
            float _645 = _639 * _644;
            float _646 = _640 * _644;
            float _647 = _644 * _629;
            float _652 = float(_596.y) * 0.0039215688593685626983642578125f;
            float _658 = asfloat(_10.Load(_430 * 115u).x);
            float _823;
            float _825;
            float _827;
            if ((_457 != 0u) && _461)
            {
                _823 = 1.0f;
                _825 = 1.0f;
                _827 = 1.0f;
            }
            else
            {
                float _838 = 1.0f - _607;
                _823 = _838 * _604;
                _825 = _838 * _605;
                _827 = _838 * _606;
            }
            frontier_phi_21_7_ladder = 0u;
            frontier_phi_21_7_ladder_1 = 0.0f;
            frontier_phi_21_7_ladder_2 = _385;
            frontier_phi_21_7_ladder_3 = _386;
            frontier_phi_21_7_ladder_4 = _387;
            frontier_phi_21_7_ladder_5 = _645;
            frontier_phi_21_7_ladder_6 = _646;
            frontier_phi_21_7_ladder_7 = _647;
            frontier_phi_21_7_ladder_8 = _645;
            frontier_phi_21_7_ladder_9 = _646;
            frontier_phi_21_7_ladder_10 = _647;
            frontier_phi_21_7_ladder_11 = 0.0f;
            frontier_phi_21_7_ladder_12 = 0.0f;
            frontier_phi_21_7_ladder_13 = _823;
            frontier_phi_21_7_ladder_14 = _825;
            frontier_phi_21_7_ladder_15 = _827;
            frontier_phi_21_7_ladder_16 = ((_604 - _658) * _607) + _658;
            frontier_phi_21_7_ladder_17 = ((_605 - _658) * _607) + _658;
            frontier_phi_21_7_ladder_18 = ((_606 - _658) * _607) + _658;
            frontier_phi_21_7_ladder_19 = float(_612 & 255u) * 0.0039215688593685626983642578125f;
            frontier_phi_21_7_ladder_20 = _652;
            frontier_phi_21_7_ladder_21 = _652;
            frontier_phi_21_7_ladder_22 = float(_596.z) * 0.0039215688593685626983642578125f;
            frontier_phi_21_7_ladder_23 = 1.0f;
            frontier_phi_21_7_ladder_24 = 1.0f;
            frontier_phi_21_7_ladder_25 = 0.0f;
            frontier_phi_21_7_ladder_26 = 0.0f;
            frontier_phi_21_7_ladder_27 = 0.0f;
            frontier_phi_21_7_ladder_28 = 0.0f;
            frontier_phi_21_7_ladder_29 = 0.0f;
            frontier_phi_21_7_ladder_30 = 0.0f;
            frontier_phi_21_7_ladder_31 = 0.0f;
            frontier_phi_21_7_ladder_32 = 0.0f;
            frontier_phi_21_7_ladder_33 = 0.0f;
            frontier_phi_21_7_ladder_34 = 0.0f;
            frontier_phi_21_7_ladder_35 = 0.0f;
            frontier_phi_21_7_ladder_36 = 0.0f;
            frontier_phi_21_7_ladder_37 = 0.0f;
            frontier_phi_21_7_ladder_38 = 0.0f;
            frontier_phi_21_7_ladder_39 = 0.0f;
            frontier_phi_21_7_ladder_40 = 0.0f;
            frontier_phi_21_7_ladder_41 = 0.0f;
            frontier_phi_21_7_ladder_42 = 0.0f;
            frontier_phi_21_7_ladder_43 = 0.0f;
            frontier_phi_21_7_ladder_44 = 0.0f;
            frontier_phi_21_7_ladder_45 = 0.0f;
            frontier_phi_21_7_ladder_46 = 0.0f;
            frontier_phi_21_7_ladder_47 = 0.0f;
            frontier_phi_21_7_ladder_48 = 0.0f;
            frontier_phi_21_7_ladder_49 = 0.0f;
            frontier_phi_21_7_ladder_50 = 0.0f;
            frontier_phi_21_7_ladder_51 = 0.0f;
            frontier_phi_21_7_ladder_52 = 0u;
            frontier_phi_21_7_ladder_53 = 0.0f;
            frontier_phi_21_7_ladder_54 = 0.0f;
            frontier_phi_21_7_ladder_55 = 0.0f;
            frontier_phi_21_7_ladder_56 = 0.0f;
            frontier_phi_21_7_ladder_57 = 0.0f;
            frontier_phi_21_7_ladder_58 = 0.0f;
            frontier_phi_21_7_ladder_59 = 0.0f;
            frontier_phi_21_7_ladder_60 = 0.0f;
            frontier_phi_21_7_ladder_61 = 0.0f;
            frontier_phi_21_7_ladder_62 = 0.0f;
            frontier_phi_21_7_ladder_63 = 0.0f;
            frontier_phi_21_7_ladder_64 = 0u;
            frontier_phi_21_7_ladder_65 = 1.0f;
            frontier_phi_21_7_ladder_66 = 0u;
            frontier_phi_21_7_ladder_67 = 0.0f;
            frontier_phi_21_7_ladder_68 = 0.0f;
            frontier_phi_21_7_ladder_69 = 0.0f;
            frontier_phi_21_7_ladder_70 = 0.0f;
            frontier_phi_21_7_ladder_71 = 0.0f;
            frontier_phi_21_7_ladder_72 = 0.0f;
            frontier_phi_21_7_ladder_73 = 0.0f;
            frontier_phi_21_7_ladder_74 = 0.0f;
            frontier_phi_21_7_ladder_75 = 0.0f;
            frontier_phi_21_7_ladder_76 = _448;
            frontier_phi_21_7_ladder_77 = _422;
            frontier_phi_21_7_ladder_78 = 0.0f;
            frontier_phi_21_7_ladder_79 = 0.0f;
            frontier_phi_21_7_ladder_80 = 0.0f;
            frontier_phi_21_7_ladder_81 = 1.0f;
            frontier_phi_21_7_ladder_82 = 0.0f;
            frontier_phi_21_7_ladder_83 = 0u;
            frontier_phi_21_7_ladder_84 = 0.0f;
            frontier_phi_21_7_ladder_85 = 0.0f;
            frontier_phi_21_7_ladder_86 = 0.0f;
            frontier_phi_21_7_ladder_87 = 0.0f;
            frontier_phi_21_7_ladder_88 = 0.0f;
            frontier_phi_21_7_ladder_89 = 0.0f;
            frontier_phi_21_7_ladder_90 = 0.0f;
            frontier_phi_21_7_ladder_91 = 0.0f;
            frontier_phi_21_7_ladder_92 = 0u;
            frontier_phi_21_7_ladder_93 = 0.0f;
            frontier_phi_21_7_ladder_94 = 0.0f;
            frontier_phi_21_7_ladder_95 = 0.0f;
            frontier_phi_21_7_ladder_96 = 0.0f;
            frontier_phi_21_7_ladder_97 = 0.0f;
            frontier_phi_21_7_ladder_98 = 0.0f;
            frontier_phi_21_7_ladder_99 = 0u;
            frontier_phi_21_7_ladder_100 = 0.0f;
            frontier_phi_21_7_ladder_101 = 0.0f;
        }
        _946 = frontier_phi_21_7_ladder_28;
        _948 = frontier_phi_21_7_ladder_29;
        _950 = frontier_phi_21_7_ladder_30;
        _952 = frontier_phi_21_7_ladder_31;
        _954 = frontier_phi_21_7_ladder_32;
        _956 = frontier_phi_21_7_ladder_33;
        _958 = frontier_phi_21_7_ladder_34;
        _960 = frontier_phi_21_7_ladder_35;
        _962 = frontier_phi_21_7_ladder_36;
        _964 = frontier_phi_21_7_ladder_37;
        _966 = frontier_phi_21_7_ladder_38;
        _968 = frontier_phi_21_7_ladder_26;
        _970 = frontier_phi_21_7_ladder_40;
        _972 = frontier_phi_21_7_ladder_41;
        _974 = frontier_phi_21_7_ladder_42;
        _976 = frontier_phi_21_7_ladder_43;
        _978 = frontier_phi_21_7_ladder_44;
        _980 = frontier_phi_21_7_ladder_45;
        _982 = frontier_phi_21_7_ladder_46;
        _984 = frontier_phi_21_7_ladder_47;
        _986 = frontier_phi_21_7_ladder_48;
        _988 = frontier_phi_21_7_ladder_49;
        _990 = frontier_phi_21_7_ladder_50;
        _992 = frontier_phi_21_7_ladder_51;
        _994 = frontier_phi_21_7_ladder_39;
        _996 = frontier_phi_21_7_ladder;
        _997 = frontier_phi_21_7_ladder_2;
        _998 = frontier_phi_21_7_ladder_3;
        _999 = frontier_phi_21_7_ladder_4;
        _1000 = frontier_phi_21_7_ladder_5;
        _1004 = frontier_phi_21_7_ladder_6;
        _1008 = frontier_phi_21_7_ladder_7;
        _1012 = frontier_phi_21_7_ladder_8;
        _1015 = frontier_phi_21_7_ladder_9;
        _1018 = frontier_phi_21_7_ladder_10;
        _1021 = frontier_phi_21_7_ladder_11;
        _1023 = frontier_phi_21_7_ladder_12;
        _1025 = frontier_phi_21_7_ladder_13;
        _1030 = frontier_phi_21_7_ladder_14;
        _1035 = frontier_phi_21_7_ladder_15;
        _1040 = frontier_phi_21_7_ladder_16;
        _1045 = frontier_phi_21_7_ladder_17;
        _1048 = frontier_phi_21_7_ladder_18;
        _1051 = frontier_phi_21_7_ladder_19;
        _1056 = frontier_phi_21_7_ladder_20;
        _1060 = frontier_phi_21_7_ladder_21;
        _1061 = frontier_phi_21_7_ladder_22;
        _1065 = frontier_phi_21_7_ladder_23;
        _1067 = frontier_phi_21_7_ladder_24;
        _1070 = frontier_phi_21_7_ladder_25;
        _1073 = frontier_phi_21_7_ladder_1;
        _1076 = frontier_phi_21_7_ladder_80;
        _1079 = frontier_phi_21_7_ladder_65;
        _1081 = frontier_phi_21_7_ladder_81;
        _1084 = frontier_phi_21_7_ladder_82;
        _1086 = frontier_phi_21_7_ladder_83;
        _1088 = frontier_phi_21_7_ladder_84;
        _1090 = frontier_phi_21_7_ladder_85;
        _1092 = frontier_phi_21_7_ladder_86;
        _1094 = frontier_phi_21_7_ladder_87;
        _1096 = frontier_phi_21_7_ladder_78;
        _1097 = frontier_phi_21_7_ladder_27;
        _1098 = frontier_phi_21_7_ladder_88;
        _1099 = frontier_phi_21_7_ladder_89;
        _1100 = frontier_phi_21_7_ladder_94;
        _1102 = frontier_phi_21_7_ladder_101;
        _1104 = frontier_phi_21_7_ladder_100;
        _1106 = frontier_phi_21_7_ladder_99;
        _1107 = frontier_phi_21_7_ladder_98;
        _1109 = frontier_phi_21_7_ladder_97;
        _1111 = frontier_phi_21_7_ladder_96;
        _1113 = frontier_phi_21_7_ladder_95;
        _1115 = frontier_phi_21_7_ladder_90;
        _1117 = frontier_phi_21_7_ladder_93;
        _1119 = frontier_phi_21_7_ladder_92;
        _1121 = frontier_phi_21_7_ladder_91;
        _1124 = frontier_phi_21_7_ladder_79;
        _1126 = frontier_phi_21_7_ladder_53;
        _1128 = frontier_phi_21_7_ladder_54;
        _1130 = frontier_phi_21_7_ladder_55;
        _1132 = frontier_phi_21_7_ladder_56;
        _1135 = frontier_phi_21_7_ladder_57;
        _1137 = frontier_phi_21_7_ladder_58;
        _1139 = frontier_phi_21_7_ladder_59;
        _1141 = frontier_phi_21_7_ladder_60;
        _1143 = frontier_phi_21_7_ladder_61;
        _1145 = frontier_phi_21_7_ladder_62;
        _1147 = frontier_phi_21_7_ladder_63;
        _1149 = frontier_phi_21_7_ladder_64;
        _1151 = frontier_phi_21_7_ladder_52;
        _1153 = frontier_phi_21_7_ladder_66;
        _1155 = frontier_phi_21_7_ladder_67;
        _1157 = frontier_phi_21_7_ladder_68;
        _1159 = frontier_phi_21_7_ladder_69;
        _1161 = frontier_phi_21_7_ladder_70;
        _1163 = frontier_phi_21_7_ladder_71;
        _1165 = frontier_phi_21_7_ladder_72;
        _1167 = frontier_phi_21_7_ladder_73;
        _1169 = frontier_phi_21_7_ladder_74;
        _1171 = frontier_phi_21_7_ladder_75;
        _1173 = frontier_phi_21_7_ladder_76;
        _1174 = frontier_phi_21_7_ladder_77;
    }
    bool _1176 = (_447 & 128u) == 0u;
    bool _1177 = !_1176;
    bool _1180 = (_447 & 32u) != 0u;
    float _1333;
    uint _1335;
    uint _1337;
    float _1339;
    float _1341;
    float _1343;
    float _1345;
    float _1347;
    float _1349;
    if ((_447 & 144u) == 0u)
    {
        _1333 = 0.0f;
        _1335 = 0u;
        _1337 = 0u;
        _1339 = 0.0f;
        _1341 = 0.0f;
        _1343 = 0.0f;
        _1345 = 0.0f;
        _1347 = 0.0f;
        _1349 = 0.0f;
    }
    else
    {
        float _1373 = _312 / _69_m0[1140u].x;
        float _1374 = _314 / _69_m0[1140u].y;
        float _1380 = sqrt(((_378 * _378) + (_377 * _377)) + (_379 * _379));
        uint _1381 = _435 * 3u;
        uint _1382 = _1381 + 1155u;
        uint _1391 = _1381 + 1157u;
        uint4 _1395 = asuint(_69_m0[_1391]);
        uint _1396 = _1395.z;
        float _1410 = clamp(((_1380 / _69_m0[1162u].z) - _69_m0[1161u].x) / (_69_m0[1161u].y - _69_m0[1161u].x), 0.0f, 1.0f);
        float _1718;
        float _1719;
        float _1720;
        float _1721;
        float _1722;
        float _1723;
        if (_1176)
        {
            _1718 = _69_m0[107u].w;
            _1719 = _69_m0[106u].z;
            _1720 = _69_m0[1u].x;
            _1721 = _69_m0[1u].y;
            _1722 = _69_m0[1u].z;
            _1723 = _64_m0[66u].z;
        }
        else
        {
            _1718 = ((_69_m0[1161u].w - _69_m0[1161u].z) * _1410) + _69_m0[1161u].z;
            _1719 = _69_m0[_1391].y;
            _1720 = _69_m0[_1382].x;
            _1721 = _69_m0[_1382].y;
            _1722 = _69_m0[_1382].z;
            _1723 = _64_m0[66u].y;
        }
        uint4 _1727 = asuint(_64_m0[72u]);
        bool _1731 = (_1176 ? _1727.y : _1727.z) != 0u;
        float _1732 = (-0.0f) - _377;
        float _1733 = (-0.0f) - _378;
        float _1734 = (-0.0f) - _379;
        float _1738 = rsqrt(dot(float3(_1732, _1733, _1734), float3(_1732, _1733, _1734)));
        float _1739 = _1738 * _1732;
        float _1740 = _1738 * _1733;
        float _1741 = _1738 * _1734;
        float _1841;
        float _1842;
        uint _1843;
        if ((_996 == 0u) || (_64_m0[178u].w == 0.0f))
        {
            _1841 = 0.0f;
            _1842 = 0.0f;
            _1843 = 0u;
        }
        else
        {
            float frontier_phi_52_53_ladder;
            float frontier_phi_52_53_ladder_1;
            uint frontier_phi_52_53_ladder_2;
            if (_29[21u].SampleLevel(_77, float2(_1373, _1374), 0.0f).x < _309)
            {
                frontier_phi_52_53_ladder = _1374;
                frontier_phi_52_53_ladder_1 = _1373;
                frontier_phi_52_53_ladder_2 = 1u;
            }
            else
            {
                frontier_phi_52_53_ladder = 0.0f;
                frontier_phi_52_53_ladder_1 = 0.0f;
                frontier_phi_52_53_ladder_2 = 0u;
            }
            _1841 = frontier_phi_52_53_ladder_1;
            _1842 = frontier_phi_52_53_ladder;
            _1843 = frontier_phi_52_53_ladder_2;
        }
        float _1861 = _64_m0[100u].x / (_64_m0[100u].w - (_64_m0[100u].y * _309));
        uint _1864 = uint(int(uint(int(_312))) / int(2u));
        uint _1865 = uint(int(uint(int(_314))) / int(2u));
        float _1874 = _64_m0[100u].x / (_64_m0[100u].w - (_29[46u].Load(int3(uint2(_1864, _1865), 0u)).x * _64_m0[100u].y));
        float _1892 = mad(_1008, _64_m0[0u].z, mad(_1004, _64_m0[0u].y, _64_m0[0u].x * _1000));
        float _1895 = mad(_1008, _64_m0[1u].z, mad(_1004, _64_m0[1u].y, _64_m0[1u].x * _1000));
        float _1898 = mad(_1008, _64_m0[2u].z, mad(_1004, _64_m0[2u].y, _64_m0[2u].x * _1000));
        float _1901 = 1.0f - clamp((-0.0f) - _1898, 0.0f, 1.0f);
        float _1921 = (dot(float3(_1892, _1895, _1898), float3(_29[47u].Load(int3(uint2(_1864, _1865), 0u)).xyz)) < _69_m0[1154u].x) ? 9899999600270360182784.0f : abs(_1874 - _1861);
        uint _2034;
        uint _2036;
        if (_1921 > ((((_1901 * _1901) * (_69_m0[1153u].z - _69_m0[1153u].w)) + _69_m0[1153u].w) * _1874))
        {
            uint _2090;
            uint _2091;
            float _2092;
            uint _2094;
            _2090 = 0u;
            _2091 = 0u;
            _2092 = _1921;
            _2094 = 0u;
            uint _2035;
            uint _2037;
            for (;;)
            {
                uint _2097 = 0u + (_2094 * 2u);
                uint _2101 = 1u + (_2094 * 2u);
                uint _2109 = _99[_2097] + _1864;
                uint _2110 = _99[_2101] + _1865;
                float _2132 = (dot(float3(_1892, _1895, _1898), float3(_29[47u].Load(int3(uint2(_2109, _2110), 0u)).xyz)) < _69_m0[1154u].x) ? 9899999600270360182784.0f : abs((_64_m0[100u].x / (_64_m0[100u].w - (_29[46u].Load(int3(uint2(_2109, _2110), 0u)).x * _64_m0[100u].y))) - _1861);
                bool _2133 = _2132 < _2092;
                _2035 = _2133 ? _99[_2097] : _2090;
                _2037 = _2133 ? _99[_2101] : _2091;
                uint _2095 = _2094 + 1u;
                if (_2095 == 4u)
                {
                    break;
                }
                else
                {
                    _2090 = _2035;
                    _2091 = _2037;
                    _2092 = min(_2092, _2132);
                    _2094 = _2095;
                }
            }
            _2034 = _2035;
            _2036 = _2037;
        }
        else
        {
            _2034 = 0u;
            _2036 = 0u;
        }
        float _2203;
        float _2205;
        float _2207;
        float _2209;
        uint _2211;
        float _2213;
        float _2215;
        float _2217;
        uint _2038;
        uint _2039;
        bool _2042;
        bool _2052;
        uint _2058;
        uint _2061;
        float _2064;
        float _2065;
        float _2069;
        float _2070;
        float _2071;
        float _2075;
        bool _2076;
        for (;;)
        {
            _2038 = _2034 + _1864;
            _2039 = _2036 + _1865;
            _2042 = (_422 & 50331648u) == 0u;
            _2052 = _64_m0[162u].w > 0.0f;
            uint4 _2055 = asuint(_69_m0[1154u]);
            uint _2056 = _2055.z;
            _2058 = (_2056 == 2u) ? 1u : _2056;
            _2061 = (_9.Load((_426 * 57u) + 25u).x == 0u) ? _2055.y : 1u;
            _2064 = _69_m0[1140u].x;
            _2065 = _69_m0[1140u].y;
            _2069 = _64_m0[161u].x;
            _2070 = _64_m0[161u].y;
            _2071 = _64_m0[161u].z;
            _2075 = _64_m0[160u].w;
            _2076 = _2061 == 0u;
            bool ladder_phi_85;
            float frontier_phi_85_pred;
            float frontier_phi_85_pred_1;
            float frontier_phi_85_pred_2;
            float frontier_phi_85_pred_3;
            float frontier_phi_85_pred_4;
            float frontier_phi_85_pred_5;
            bool frontier_phi_85_pred_6;
            float frontier_phi_85_pred_7;
            float frontier_phi_85_pred_8;
            float frontier_phi_85_pred_9;
            float frontier_phi_85_pred_10;
            float frontier_phi_85_pred_11;
            uint frontier_phi_85_pred_12;
            float frontier_phi_85_pred_13;
            float frontier_phi_85_pred_14;
            float frontier_phi_85_pred_15;
            for (;;)
            {
                float _2285;
                bool _2287;
                float _2290;
                float _2292;
                float _2294;
                float _2296;
                float _2297;
                if (_2076)
                {
                    if (_2058 == 0u)
                    {
                        ladder_phi_85 = true;
                        frontier_phi_85_pred = _14895;
                        frontier_phi_85_pred_1 = _14897;
                        frontier_phi_85_pred_2 = _14899;
                        frontier_phi_85_pred_3 = _14901;
                        frontier_phi_85_pred_4 = _14903;
                        frontier_phi_85_pred_5 = _14905;
                        frontier_phi_85_pred_6 = _14907;
                        frontier_phi_85_pred_7 = _14909;
                        frontier_phi_85_pred_8 = 0.0f;
                        frontier_phi_85_pred_9 = 0.0f;
                        frontier_phi_85_pred_10 = 0.0f;
                        frontier_phi_85_pred_11 = 1.0f;
                        frontier_phi_85_pred_12 = 0u;
                        frontier_phi_85_pred_13 = 0.0f;
                        frontier_phi_85_pred_14 = 0.0f;
                        frontier_phi_85_pred_15 = 0.0f;
                        break;
                    }
                    _2285 = 1.0f;
                    _2287 = false;
                    _2290 = 0.0f;
                    _2292 = 0.0f;
                    _2294 = 0.0f;
                    _2296 = _312 / _2064;
                    _2297 = _314 / _2065;
                }
                else
                {
                    float _2136 = _312 / _2064;
                    float _2137 = _314 / _2065;
                    bool _2289;
                    float _2291;
                    float _2293;
                    float _2295;
                    float _2317;
                    if (_2061 == 2u)
                    {
                        _2291 = 0.0f;
                        _2293 = 0.0f;
                        _2295 = 0.0f;
                        _2317 = _57.Load(int3(uint2(_2038, _2039), 0u)).x;
                        _2289 = false;
                    }
                    else
                    {
                        float4 _2265 = _57.SampleLevel(_72, float2(_2136, _2137), 0.0f);
                        float _2267 = _2265.x;
                        bool frontier_phi_78_72_ladder;
                        float frontier_phi_78_72_ladder_1;
                        float frontier_phi_78_72_ladder_2;
                        float frontier_phi_78_72_ladder_3;
                        float frontier_phi_78_72_ladder_4;
                        if (_2052)
                        {
                            float4 _2329 = _15[506u].SampleLevel(_72, float2(_2136, _2137), 0.0f);
                            float _2331 = _2329.x;
                            float _2337 = rsqrt(dot(float3(_2331, _2329.yz), float3(_2331, _2329.yz)));
                            frontier_phi_78_72_ladder = true;
                            frontier_phi_78_72_ladder_1 = _2267;
                            frontier_phi_78_72_ladder_2 = _2337 * _2329.z;
                            frontier_phi_78_72_ladder_3 = _2337 * _2329.y;
                            frontier_phi_78_72_ladder_4 = _2337 * _2331;
                        }
                        else
                        {
                            frontier_phi_78_72_ladder = false;
                            frontier_phi_78_72_ladder_1 = _2267;
                            frontier_phi_78_72_ladder_2 = 0.0f;
                            frontier_phi_78_72_ladder_3 = 0.0f;
                            frontier_phi_78_72_ladder_4 = 0.0f;
                        }
                        _2291 = frontier_phi_78_72_ladder_4;
                        _2293 = frontier_phi_78_72_ladder_3;
                        _2295 = frontier_phi_78_72_ladder_2;
                        _2317 = frontier_phi_78_72_ladder_1;
                        _2289 = frontier_phi_78_72_ladder;
                    }
                    float _2286 = (exp2(log2(_2317) * _2071) * (_2070 - _2069)) + _2069;
                    if (_2058 == 0u)
                    {
                        ladder_phi_85 = false;
                        frontier_phi_85_pred = 0.0f;
                        frontier_phi_85_pred_1 = 0.0f;
                        frontier_phi_85_pred_2 = 0.0f;
                        frontier_phi_85_pred_3 = _2295;
                        frontier_phi_85_pred_4 = _2293;
                        frontier_phi_85_pred_5 = _2291;
                        frontier_phi_85_pred_6 = _2289;
                        frontier_phi_85_pred_7 = _2286;
                        frontier_phi_85_pred_8 = _14912;
                        frontier_phi_85_pred_9 = _14915;
                        frontier_phi_85_pred_10 = _14918;
                        frontier_phi_85_pred_11 = _14921;
                        frontier_phi_85_pred_12 = _14924;
                        frontier_phi_85_pred_13 = _14927;
                        frontier_phi_85_pred_14 = _14930;
                        frontier_phi_85_pred_15 = _14933;
                        break;
                    }
                    _2285 = _2286;
                    _2287 = _2289;
                    _2290 = _2291;
                    _2292 = _2293;
                    _2294 = _2295;
                    _2296 = _2136;
                    _2297 = _2137;
                }
                float4 _2303 = _15[504u].SampleLevel(_72, float2(_2296, _2297), 0.0f);
                ladder_phi_85 = false;
                frontier_phi_85_pred = exp2(log2(_2303.z));
                frontier_phi_85_pred_1 = exp2(log2(_2303.y));
                frontier_phi_85_pred_2 = exp2(log2(_2303.x));
                frontier_phi_85_pred_3 = _2294;
                frontier_phi_85_pred_4 = _2292;
                frontier_phi_85_pred_5 = _2290;
                frontier_phi_85_pred_6 = _2287;
                frontier_phi_85_pred_7 = _2285;
                frontier_phi_85_pred_8 = _14911;
                frontier_phi_85_pred_9 = _14914;
                frontier_phi_85_pred_10 = _14917;
                frontier_phi_85_pred_11 = _14920;
                frontier_phi_85_pred_12 = _14923;
                frontier_phi_85_pred_13 = _14926;
                frontier_phi_85_pred_14 = _14929;
                frontier_phi_85_pred_15 = _14932;
                break;
            }
            float _2432 = frontier_phi_85_pred;
            float _2431 = frontier_phi_85_pred_1;
            float _2430 = frontier_phi_85_pred_2;
            float _2218 = frontier_phi_85_pred_3;
            float _2216 = frontier_phi_85_pred_4;
            float _2214 = frontier_phi_85_pred_5;
            bool _2429 = frontier_phi_85_pred_6;
            float _2204 = frontier_phi_85_pred_7;
            if (ladder_phi_85)
            {
                _2203 = frontier_phi_85_pred_11;
                _2205 = frontier_phi_85_pred_10;
                _2207 = frontier_phi_85_pred_9;
                _2209 = frontier_phi_85_pred_8;
                _2211 = frontier_phi_85_pred_12;
                _2213 = frontier_phi_85_pred_13;
                _2215 = frontier_phi_85_pred_14;
                _2217 = frontier_phi_85_pred_15;
                break;
            }
            _2203 = _2204;
            _2205 = _2430 / _64_m0[59u].x;
            _2207 = _2431 / _64_m0[59u].x;
            _2209 = _2432 / _64_m0[59u].x;
            _2211 = uint(_2429);
            _2213 = _2214;
            _2215 = _2216;
            _2217 = _2218;
            break;
        }
        float _2219 = _2042 ? _2203 : 1.0f;
        float4 _2222 = _29[14u].Load(int3(uint2(_2038, _2039), 0u));
        float _2224 = _2222.x;
        float4 _2227 = _29[13u].Load(int3(uint2(_2038, _2039), 0u));
        float _2235 = _2227.x / _64_m0[59u].x;
        float _2236 = _2227.y / _64_m0[59u].x;
        float _2237 = _2227.z / _64_m0[59u].x;
        bool _2243 = asuint(_69_m0[1154u]).w != 0u;
        float _2249 = _2235 - (_2235 * _555);
        float _2250 = _2236 - (_2236 * _555);
        float _2251 = _2237 - (_2237 * _555);
        float _2253 = ((_2224 * _2224) + (-1.0f)) * asfloat(_9.Load((_426 * 57u) + 19u).x);
        float _2254 = _2253 + 1.0f;
        float _2283;
        if ((_2058 != 0u) && (_2061 != 0u))
        {
            _2283 = ((_2203 + (-1.0f)) * _2075) + 1.0f;
        }
        else
        {
            _2283 = 1.0f;
        }
        float _2284 = _2219 * _1056;
        float _2389;
        if (_1176)
        {
            float _2381 = _385 - _69_m0[114u].x;
            float _2382 = _386 - _69_m0[114u].y;
            float _2383 = _387 - _69_m0[114u].z;
            float frontier_phi_84_83_ladder;
            if (dot(float3(_2381, _2382, _2383), float3(_2381, _2382, _2383)) < (_69_m0[113u].x * _69_m0[113u].x))
            {
                float4 _2474 = _21.SampleLevel(_73, float3((dot(float3(_2381, _2382, _2383), float3(_69_m0[115u].xyz)) * _69_m0[118u].x) + 0.5f, (dot(float3(_2381, _2382, _2383), float3(_69_m0[116u].xyz)) * _69_m0[118u].y) + 0.5f, (dot(float3(_2381, _2382, _2383), float3(_69_m0[117u].xyz)) * _69_m0[118u].z) + 0.5f), 0.0f);
                float _2479 = _2474.w;
                float _2489 = _2479 * _2479;
                float _2490 = _2489 * 2.0f;
                frontier_phi_84_83_ladder = 1.0f - (exp2(log2(clamp((dot(float3((_2490 * _2474.x) - _2489, (_2490 * _2474.y) - _2489, (_2490 * _2474.z) - _2489), float3(dot(float3(_1000, _1004, _1008), float3(_69_m0[115u].xyz)), dot(float3(_1000, _1004, _1008), float3(_69_m0[116u].xyz)), dot(float3(_1000, _1004, _1008), float3(_69_m0[117u].xyz)))) * _69_m0[113u].z) + _2489, 0.0f, 1.0f)) * _69_m0[113u].y) * _69_m0[113u].w);
            }
            else
            {
                frontier_phi_84_83_ladder = 1.0f;
            }
            _2389 = frontier_phi_84_83_ladder;
        }
        else
        {
            _2389 = 1.0f;
        }
        float _2391 = _2389 * _2284;
        float _2392 = _2391 * _2254;
        float _2394 = _2042 ? _2392 : (_2392 * _2203);
        float _2397 = ((1.0f - _2394) * _555) + _2394;
        float _2405 = (1.0f - _1056) - ((_2391 - _1056) * asfloat(_10.Load((_430 * 115u) + 112u).x));
        float _2422 = (_64_m0[84u].z * _2253) + 1.0f;
        float _2423 = (-0.0f) - _1739;
        float _2424 = (-0.0f) - _1740;
        float _2425 = (-0.0f) - _1741;
        float _2426 = dot(float3(_2423, _2424, _2425), float3(_1012, _1015, _1018));
        float _2617;
        float _2619;
        if (_1176)
        {
            float _2507 = _2426 * 2.0f;
            bool _2514 = _2211 != 0u;
            float _2531 = max(max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718), 0.00999999977648258209228515625f);
            float _2533 = sqrt(1.0f - _1060);
            float _2537 = exp2((_2531 * _2531) * (-3.321929931640625f));
            float _2543 = acos(_2533);
            float _2544 = acos(_2537);
            float _2545 = acos((dot(float3(_2514 ? _2213 : _1012, _2514 ? _2215 : _1015, _2514 ? _2217 : _1018), float3(_2423 - (_2507 * _1012), _2424 - (_2507 * _1015), _2425 - (_2507 * _1018))) * 0.5f) + 0.5f);
            float _2679;
            if (_2545 > (max(_2543, _2544) - min(_2543, _2544)))
            {
                float _2611 = _2544 + _2543;
                float frontier_phi_100_94_ladder;
                if (_2545 < _2611)
                {
                    float _2662 = abs(_2543 - _2544);
                    float _2670 = clamp(1.0f - clamp((_2545 - _2662) / max(_2611 - _2662, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f), 0.0f, 1.0f);
                    frontier_phi_100_94_ladder = ((_2670 * _2670) * (3.0f - (_2670 * 2.0f))) * (6.283185482025146484375f - (max(_2533, _2537) * 6.283185482025146484375f));
                }
                else
                {
                    frontier_phi_100_94_ladder = 0.0f;
                }
                _2679 = frontier_phi_100_94_ladder;
            }
            else
            {
                _2679 = 6.283185482025146484375f - (max(_2533, _2537) * 6.283185482025146484375f);
            }
            float _2618 = ((_2219 * _2219) * ((asuint(_64_m0[156u]).z != 0u) ? 1.0f : _2422)) * (_2679 / ((1.0f - _2537) * 6.283185482025146484375f));
            _2617 = _2618;
            _2619 = _2618;
        }
        else
        {
            float _2566 = clamp((((_64_m0[73u].z * (_2422 - _2391)) + _2391) - _64_m0[62u].x) / (_64_m0[62u].y - _64_m0[62u].x), 0.0f, 1.0f) * _1060;
            float _2578 = clamp(((_1740 + ((_2426 * 2.0f) * _1015)) - _64_m0[75u].x) / (_64_m0[75u].y - _64_m0[75u].x), 0.0f, 1.0f);
            _2617 = (((((_2578 * _2578) * (3.0f - (_2578 * 2.0f))) + (-1.0f)) * _64_m0[75u].z) + 1.0f) * _2566;
            _2619 = _2566;
        }
        float _2620 = _2283 * _2284;
        float _2633 = ((_64_m0[128u].z * (_2389 + (-1.0f))) + 1.0f) * _1061;
        float _2635 = (_1180 && _1177) ? _1100 : 1.0f;
        float _2688;
        float _2693;
        float _2698;
        if ((_447 & 64u) == 0u)
        {
            float frontier_phi_102_101_ladder;
            float frontier_phi_102_101_ladder_1;
            float frontier_phi_102_101_ladder_2;
            if ((_1174 & 4194304u) == 0u)
            {
                float frontier_phi_102_101_ladder_104_ladder;
                float frontier_phi_102_101_ladder_104_ladder_1;
                float frontier_phi_102_101_ladder_104_ladder_2;
                if ((_1174 & 67108864u) == 0u)
                {
                    float frontier_phi_102_101_ladder_104_ladder_108_ladder;
                    float frontier_phi_102_101_ladder_104_ladder_108_ladder_1;
                    float frontier_phi_102_101_ladder_104_ladder_108_ladder_2;
                    if ((_1174 & 50331648u) == 0u)
                    {
                        frontier_phi_102_101_ladder_104_ladder_108_ladder = (1.0f - _1048) * _1035;
                        frontier_phi_102_101_ladder_104_ladder_108_ladder_1 = (1.0f - _1045) * _1030;
                        frontier_phi_102_101_ladder_104_ladder_108_ladder_2 = (1.0f - _1040) * _1025;
                    }
                    else
                    {
                        float _2827 = ((_1051 * 0.75f) + 1.25f) + ((((_1163 * _1163) * 9000.0f) * _1161) * _1163);
                        float _2853 = (_1167 * 0.3183098733425140380859375f) * (sqrt(((_1157 * _1157) + (_1155 * _1155)) + (_1159 * _1159)) + 1.0f);
                        frontier_phi_102_101_ladder_104_ladder_108_ladder = exp2(log2(exp2(_1159 * (-4.616624355316162109375f))) * _2827) * _2853;
                        frontier_phi_102_101_ladder_104_ladder_108_ladder_1 = exp2(log2(exp2(_1157 * (-4.616624355316162109375f))) * _2827) * _2853;
                        frontier_phi_102_101_ladder_104_ladder_108_ladder_2 = exp2(log2(exp2(_1155 * (-4.616624355316162109375f))) * _2827) * _2853;
                    }
                    frontier_phi_102_101_ladder_104_ladder = frontier_phi_102_101_ladder_104_ladder_108_ladder;
                    frontier_phi_102_101_ladder_104_ladder_1 = frontier_phi_102_101_ladder_104_ladder_108_ladder_1;
                    frontier_phi_102_101_ladder_104_ladder_2 = frontier_phi_102_101_ladder_104_ladder_108_ladder_2;
                }
                else
                {
                    float _2752 = (((_39[NonUniformResourceIndex(_1151 + 0u)].SampleLevel(_76, float2(_1135, _1137), 0.0f).x * _1147) + (-1.0f)) * _1145) + 1.0f;
                    frontier_phi_102_101_ladder_104_ladder = ((1.0f - _1048) * _1035) * _2752;
                    frontier_phi_102_101_ladder_104_ladder_1 = ((1.0f - _1045) * _1030) * _2752;
                    frontier_phi_102_101_ladder_104_ladder_2 = ((1.0f - _1040) * _1025) * _2752;
                }
                frontier_phi_102_101_ladder = frontier_phi_102_101_ladder_104_ladder;
                frontier_phi_102_101_ladder_1 = frontier_phi_102_101_ladder_104_ladder_1;
                frontier_phi_102_101_ladder_2 = frontier_phi_102_101_ladder_104_ladder_2;
            }
            else
            {
                frontier_phi_102_101_ladder = ((1.0f - _1048) - (_1099 * (_1098 - _1048))) * _1035;
                frontier_phi_102_101_ladder_1 = ((1.0f - _1045) - (_1099 * (_1097 - _1045))) * _1030;
                frontier_phi_102_101_ladder_2 = ((1.0f - _1040) - (_1099 * (_1096 - _1040))) * _1025;
            }
            _2688 = frontier_phi_102_101_ladder_2;
            _2693 = frontier_phi_102_101_ladder_1;
            _2698 = frontier_phi_102_101_ladder;
        }
        else
        {
            _2688 = _978;
            _2693 = _980;
            _2698 = _982;
        }
        float _2756;
        float _2767;
        float _2778;
        float _2789;
        float _2795;
        float _2801;
        if ((_447 & 1u) == 0u)
        {
            bool _2726 = (_1174 & 4194304u) == 0u;
            float frontier_phi_112_106_ladder;
            float frontier_phi_112_106_ladder_1;
            float frontier_phi_112_106_ladder_2;
            float frontier_phi_112_106_ladder_3;
            float frontier_phi_112_106_ladder_4;
            float frontier_phi_112_106_ladder_5;
            if (_1176)
            {
                float frontier_phi_112_106_ladder_110_ladder;
                float frontier_phi_112_106_ladder_110_ladder_1;
                float frontier_phi_112_106_ladder_110_ladder_2;
                float frontier_phi_112_106_ladder_110_ladder_3;
                float frontier_phi_112_106_ladder_110_ladder_4;
                float frontier_phi_112_106_ladder_110_ladder_5;
                if (_2726)
                {
                    float frontier_phi_112_106_ladder_110_ladder_115_ladder;
                    float frontier_phi_112_106_ladder_110_ladder_115_ladder_1;
                    float frontier_phi_112_106_ladder_110_ladder_115_ladder_2;
                    float frontier_phi_112_106_ladder_110_ladder_115_ladder_3;
                    float frontier_phi_112_106_ladder_110_ladder_115_ladder_4;
                    float frontier_phi_112_106_ladder_110_ladder_115_ladder_5;
                    if ((_1174 & 67108864u) == 0u)
                    {
                        float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder;
                        float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_1;
                        float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_2;
                        float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_3;
                        float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_4;
                        float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_5;
                        if ((_1174 & 50331648u) == 0u)
                        {
                            float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder;
                            float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_1;
                            float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_2;
                            float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_3;
                            float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_4;
                            float frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_5;
                            if ((_1174 & 1032192u) == 0u)
                            {
                                float4 _3195 = _36[6u].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                                float _2790 = _3195.x * _64_m0[59u].y;
                                float _2796 = _3195.y * _64_m0[59u].y;
                                float _2802 = _3195.z * _64_m0[59u].y;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder = _2790;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_1 = ((1.0f - _1040) * _1025) * _2790;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_2 = ((1.0f - _1045) * _1030) * _2796;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_3 = ((1.0f - _1048) * _1035) * _2802;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_4 = _2796;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_5 = _2802;
                            }
                            else
                            {
                                float4 _3209 = _36[6u].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                                float _2791 = _3209.x * _64_m0[59u].y;
                                float _2797 = _3209.y * _64_m0[59u].y;
                                float _2803 = _3209.z * _64_m0[59u].y;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder = _2791;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_1 = ((1.0f - _1040) * _1025) * _2791;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_2 = ((1.0f - _1045) * _1030) * _2797;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_3 = ((1.0f - _1048) * _1035) * _2803;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_4 = _2797;
                                frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_5 = _2803;
                            }
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_1 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_1;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_2 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_2;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_3 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_3;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_4 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_4;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_5 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_127_ladder_5;
                        }
                        else
                        {
                            float _3043 = ((_1051 * 0.75f) + 1.25f) + ((((_1163 * _1163) * 9000.0f) * _1161) * _1163);
                            float4 _3069 = _36[6u].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                            float _2792 = _3069.x * _64_m0[59u].y;
                            float _2798 = _3069.y * _64_m0[59u].y;
                            float _2804 = _3069.z * _64_m0[59u].y;
                            float _3075 = (_1167 * 0.3183098733425140380859375f) * (sqrt(((_1157 * _1157) + (_1155 * _1155)) + (_1159 * _1159)) + 1.0f);
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder = _2792;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_1 = (exp2(log2(exp2(_1155 * (-4.616624355316162109375f))) * _3043) * _3075) * _2792;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_2 = (exp2(log2(exp2(_1157 * (-4.616624355316162109375f))) * _3043) * _3075) * _2798;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_3 = (exp2(log2(exp2(_1159 * (-4.616624355316162109375f))) * _3043) * _3075) * _2804;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_4 = _2798;
                            frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_5 = _2804;
                        }
                        frontier_phi_112_106_ladder_110_ladder_115_ladder = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_1 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_1;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_2 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_2;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_3 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_3;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_4 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_4;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_5 = frontier_phi_112_106_ladder_110_ladder_115_ladder_121_ladder_5;
                    }
                    else
                    {
                        float4 _2960 = _36[6u].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                        float _2793 = _2960.x * _64_m0[59u].y;
                        float _2799 = _2960.y * _64_m0[59u].y;
                        float _2805 = _2960.z * _64_m0[59u].y;
                        float _2983 = (((_39[NonUniformResourceIndex(_1151 + 0u)].SampleLevel(_76, float2(_1135, _1137), 0.0f).x * _1147) + (-1.0f)) * _1145) + 1.0f;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder = _2793;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_1 = (((1.0f - _1040) * _1025) * _2793) * _2983;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_2 = (((1.0f - _1045) * _1030) * _2799) * _2983;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_3 = (((1.0f - _1048) * _1035) * _2805) * _2983;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_4 = _2799;
                        frontier_phi_112_106_ladder_110_ladder_115_ladder_5 = _2805;
                    }
                    frontier_phi_112_106_ladder_110_ladder = frontier_phi_112_106_ladder_110_ladder_115_ladder;
                    frontier_phi_112_106_ladder_110_ladder_1 = frontier_phi_112_106_ladder_110_ladder_115_ladder_1;
                    frontier_phi_112_106_ladder_110_ladder_2 = frontier_phi_112_106_ladder_110_ladder_115_ladder_2;
                    frontier_phi_112_106_ladder_110_ladder_3 = frontier_phi_112_106_ladder_110_ladder_115_ladder_3;
                    frontier_phi_112_106_ladder_110_ladder_4 = frontier_phi_112_106_ladder_110_ladder_115_ladder_4;
                    frontier_phi_112_106_ladder_110_ladder_5 = frontier_phi_112_106_ladder_110_ladder_115_ladder_5;
                }
                else
                {
                    float4 _2861 = _36[6u].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                    float _2794 = _2861.x * _64_m0[59u].y;
                    float _2800 = _2861.y * _64_m0[59u].y;
                    float _2806 = _2861.z * _64_m0[59u].y;
                    float4 _2875 = _15[502u].SampleLevel(_72, float2(clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f), 1.0f - _1051), 0.0f);
                    float _2877 = _2875.x;
                    frontier_phi_112_106_ladder_110_ladder = _2794;
                    frontier_phi_112_106_ladder_110_ladder_1 = (_2794 * _1025) * ((1.0f - _1040) - (((_2877 * _1096) - _1040) * _1099));
                    frontier_phi_112_106_ladder_110_ladder_2 = (_2800 * _1030) * ((1.0f - _1045) - (((_2877 * _1097) - _1045) * _1099));
                    frontier_phi_112_106_ladder_110_ladder_3 = (_2806 * _1035) * ((1.0f - _1048) - (((_2877 * _1098) - _1048) * _1099));
                    frontier_phi_112_106_ladder_110_ladder_4 = _2800;
                    frontier_phi_112_106_ladder_110_ladder_5 = _2806;
                }
                frontier_phi_112_106_ladder = frontier_phi_112_106_ladder_110_ladder;
                frontier_phi_112_106_ladder_1 = frontier_phi_112_106_ladder_110_ladder_1;
                frontier_phi_112_106_ladder_2 = frontier_phi_112_106_ladder_110_ladder_2;
                frontier_phi_112_106_ladder_3 = frontier_phi_112_106_ladder_110_ladder_3;
                frontier_phi_112_106_ladder_4 = frontier_phi_112_106_ladder_110_ladder_4;
                frontier_phi_112_106_ladder_5 = frontier_phi_112_106_ladder_110_ladder_5;
            }
            else
            {
                uint _2755 = (_435 + 50u) + 0u;
                float frontier_phi_112_106_ladder_111_ladder;
                float frontier_phi_112_106_ladder_111_ladder_1;
                float frontier_phi_112_106_ladder_111_ladder_2;
                float frontier_phi_112_106_ladder_111_ladder_3;
                float frontier_phi_112_106_ladder_111_ladder_4;
                float frontier_phi_112_106_ladder_111_ladder_5;
                if (_2726)
                {
                    float frontier_phi_112_106_ladder_111_ladder_117_ladder;
                    float frontier_phi_112_106_ladder_111_ladder_117_ladder_1;
                    float frontier_phi_112_106_ladder_111_ladder_117_ladder_2;
                    float frontier_phi_112_106_ladder_111_ladder_117_ladder_3;
                    float frontier_phi_112_106_ladder_111_ladder_117_ladder_4;
                    float frontier_phi_112_106_ladder_111_ladder_117_ladder_5;
                    if ((_1174 & 67108864u) == 0u)
                    {
                        float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder;
                        float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_1;
                        float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_2;
                        float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_3;
                        float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_4;
                        float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_5;
                        if ((_1174 & 50331648u) == 0u)
                        {
                            float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder;
                            float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_1;
                            float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_2;
                            float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_3;
                            float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_4;
                            float frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_5;
                            if ((_1174 & 1032192u) == 0u)
                            {
                                float4 _3223 = _36[NonUniformResourceIndex(_2755)].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder = 0.0f;
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_1 = (_3223.x * _64_m0[59u].y) * ((1.0f - _1040) * _1025);
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_2 = (_3223.y * _64_m0[59u].y) * ((1.0f - _1045) * _1030);
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_3 = (_3223.z * _64_m0[59u].y) * ((1.0f - _1048) * _1035);
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_4 = 0.0f;
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_5 = 0.0f;
                            }
                            else
                            {
                                float4 _3240 = _36[NonUniformResourceIndex(_2755)].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder = 0.0f;
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_1 = ((_64_m0[59u].y * _1025) * (1.0f - _1040)) * _3240.x;
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_2 = ((_64_m0[59u].y * _1030) * (1.0f - _1045)) * _3240.y;
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_3 = ((_64_m0[59u].y * _1035) * (1.0f - _1048)) * _3240.z;
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_4 = 0.0f;
                                frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_5 = 0.0f;
                            }
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_1 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_1;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_2 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_2;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_3 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_3;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_4 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_4;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_5 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_129_ladder_5;
                        }
                        else
                        {
                            float _3087 = ((_1051 * 0.75f) + 1.25f) + ((((_1163 * _1163) * 9000.0f) * _1161) * _1163);
                            float4 _3113 = _36[NonUniformResourceIndex(_2755)].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                            float _3120 = ((_1167 * 0.3183098733425140380859375f) * (sqrt(((_1157 * _1157) + (_1155 * _1155)) + (_1159 * _1159)) + 1.0f)) * _64_m0[59u].y;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder = 0.0f;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_1 = (_3120 * exp2(log2(exp2(_1155 * (-4.616624355316162109375f))) * _3087)) * _3113.x;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_2 = (_3120 * exp2(log2(exp2(_1157 * (-4.616624355316162109375f))) * _3087)) * _3113.y;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_3 = (_3120 * exp2(log2(exp2(_1159 * (-4.616624355316162109375f))) * _3087)) * _3113.z;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_4 = 0.0f;
                            frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_5 = 0.0f;
                        }
                        frontier_phi_112_106_ladder_111_ladder_117_ladder = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_1 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_1;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_2 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_2;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_3 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_3;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_4 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_4;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_5 = frontier_phi_112_106_ladder_111_ladder_117_ladder_123_ladder_5;
                    }
                    else
                    {
                        float4 _2990 = _36[NonUniformResourceIndex(_2755)].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                        float _3008 = ((((_39[NonUniformResourceIndex(_1151 + 0u)].SampleLevel(_76, float2(_1135, _1137), 0.0f).x * _1147) + (-1.0f)) * _1145) + 1.0f) * _64_m0[59u].y;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder = 0.0f;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_1 = (((1.0f - _1040) * _1025) * _2990.x) * _3008;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_2 = (((1.0f - _1045) * _1030) * _2990.y) * _3008;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_3 = (((1.0f - _1048) * _1035) * _2990.z) * _3008;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_4 = 0.0f;
                        frontier_phi_112_106_ladder_111_ladder_117_ladder_5 = 0.0f;
                    }
                    frontier_phi_112_106_ladder_111_ladder = frontier_phi_112_106_ladder_111_ladder_117_ladder;
                    frontier_phi_112_106_ladder_111_ladder_1 = frontier_phi_112_106_ladder_111_ladder_117_ladder_1;
                    frontier_phi_112_106_ladder_111_ladder_2 = frontier_phi_112_106_ladder_111_ladder_117_ladder_2;
                    frontier_phi_112_106_ladder_111_ladder_3 = frontier_phi_112_106_ladder_111_ladder_117_ladder_3;
                    frontier_phi_112_106_ladder_111_ladder_4 = frontier_phi_112_106_ladder_111_ladder_117_ladder_4;
                    frontier_phi_112_106_ladder_111_ladder_5 = frontier_phi_112_106_ladder_111_ladder_117_ladder_5;
                }
                else
                {
                    float4 _2902 = _36[NonUniformResourceIndex(_2755)].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                    float4 _2915 = _15[502u].SampleLevel(_72, float2(clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f), 1.0f - _1051), 0.0f);
                    float _2917 = _2915.x;
                    frontier_phi_112_106_ladder_111_ladder = 0.0f;
                    frontier_phi_112_106_ladder_111_ladder_1 = ((_64_m0[59u].y * _1025) * _2902.x) * ((1.0f - _1040) - (((_2917 * _1096) - _1040) * _1099));
                    frontier_phi_112_106_ladder_111_ladder_2 = ((_64_m0[59u].y * _1030) * _2902.y) * ((1.0f - _1045) - (((_2917 * _1097) - _1045) * _1099));
                    frontier_phi_112_106_ladder_111_ladder_3 = ((_64_m0[59u].y * _1035) * _2902.z) * ((1.0f - _1048) - (((_2917 * _1098) - _1048) * _1099));
                    frontier_phi_112_106_ladder_111_ladder_4 = 0.0f;
                    frontier_phi_112_106_ladder_111_ladder_5 = 0.0f;
                }
                frontier_phi_112_106_ladder = frontier_phi_112_106_ladder_111_ladder;
                frontier_phi_112_106_ladder_1 = frontier_phi_112_106_ladder_111_ladder_1;
                frontier_phi_112_106_ladder_2 = frontier_phi_112_106_ladder_111_ladder_2;
                frontier_phi_112_106_ladder_3 = frontier_phi_112_106_ladder_111_ladder_3;
                frontier_phi_112_106_ladder_4 = frontier_phi_112_106_ladder_111_ladder_4;
                frontier_phi_112_106_ladder_5 = frontier_phi_112_106_ladder_111_ladder_5;
            }
            _2756 = frontier_phi_112_106_ladder_1;
            _2767 = frontier_phi_112_106_ladder_2;
            _2778 = frontier_phi_112_106_ladder_3;
            _2789 = frontier_phi_112_106_ladder;
            _2795 = frontier_phi_112_106_ladder_4;
            _2801 = frontier_phi_112_106_ladder_5;
        }
        else
        {
            _2756 = _984 / _64_m0[59u].x;
            _2767 = _986 / _64_m0[59u].x;
            _2778 = _988 / _64_m0[59u].x;
            _2789 = 0.0f;
            _2795 = 0.0f;
            _2801 = 0.0f;
        }
        float _2939;
        float _2941;
        float _2943;
        if ((_447 & 65u) == 0u)
        {
            _2939 = 0.0f;
            _2941 = 0.0f;
            _2943 = 0.0f;
        }
        else
        {
            _2939 = _990 / _64_m0[59u].x;
            _2941 = _992 / _64_m0[59u].x;
            _2943 = _994 / _64_m0[59u].x;
        }
        float _3015;
        float _3018;
        float _3021;
        if ((((_448 & 64u) | (_422 & 536870912u)) == 0u) || (int(_1173) < int(0u)))
        {
            _3015 = 0.0f;
            _3018 = 0.0f;
            _3021 = 0.0f;
        }
        else
        {
            float frontier_phi_125_126_ladder;
            float frontier_phi_125_126_ladder_1;
            float frontier_phi_125_126_ladder_2;
            if ((_1174 & 536870912u) == 0u)
            {
                float4 _3146 = _36[6u].SampleLevel(_76, float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), 0.0f);
                float _3154 = (_2397 * _1081) * _64_m0[59u].y;
                frontier_phi_125_126_ladder = (_3146.z * _1076) * _3154;
                frontier_phi_125_126_ladder_1 = (_3146.y * _1073) * _3154;
                frontier_phi_125_126_ladder_2 = (_3146.x * _1070) * _3154;
            }
            else
            {
                float4 _3165 = _36[6u].SampleLevel(_76, float3(_1000, _1004, _1008), 0.0f);
                float _3176 = _64_m0[59u].y * _2397;
                frontier_phi_125_126_ladder = (_1081 * _1076) * (((((1.0f - _1048) * _1035) * _3165.z) * _3176) + (_2391 * _2251));
                frontier_phi_125_126_ladder_1 = (_1081 * _1073) * (((((1.0f - _1045) * _1030) * _3165.y) * _3176) + (_2391 * _2250));
                frontier_phi_125_126_ladder_2 = (_1081 * _1070) * (((((1.0f - _1040) * _1025) * _3165.x) * _3176) + (_2391 * _2249));
            }
            _3015 = frontier_phi_125_126_ladder_2;
            _3018 = frontier_phi_125_126_ladder_1;
            _3021 = frontier_phi_125_126_ladder;
        }
        float _4501;
        float _4502;
        float _4503;
        if (_1176)
        {
            uint _3254;
            float _3255;
            float _3257;
            float _3259;
            if (asuint(_64_m0[156u]).z == 0u)
            {
                _3254 = 0u;
                _3255 = 0.0f;
                _3257 = 0.0f;
                _3259 = 0.0f;
            }
            else
            {
                float _3306 = ((_2789 * _2254) + _2249) / _64_m0[59u].w;
                float _3307 = ((_2795 * _2254) + _2250) / _64_m0[59u].w;
                float _3308 = ((_2801 * _2254) + _2251) / _64_m0[59u].w;
                float _3312 = dot(float3(_3306, _3307, _3308), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                float _3336 = dot(float3(_36[5u].Sample(_76, float3(_1000, _1004, _1008)).xyz), float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
                _3254 = 1u;
                _3255 = ((((((_3306 - _3312) * _64_m0[156u].w) + _3312) / _3336) + (-1.0f)) * _64_m0[169u].w) + 1.0f;
                _3257 = ((((((_3307 - _3312) * _64_m0[156u].w) + _3312) / _3336) + (-1.0f)) * _64_m0[169u].w) + 1.0f;
                _3259 = ((((((_3308 - _3312) * _64_m0[156u].w) + _3312) / _3336) + (-1.0f)) * _64_m0[169u].w) + 1.0f;
            }
            float _3294 = ((((exp2(log2(clamp((_1380 - _64_m0[121u].y) * _64_m0[121u].z, 0.0f, 1.0f)) * _64_m0[121u].w) * asfloat(_10.Load((_430 * 115u) + 114u).x)) * exp2(log2(clamp((_378 - _64_m0[122u].x) * _64_m0[122u].y, 0.0f, 1.0f)) * _64_m0[122u].z)) * (1.0f - clamp(_1004, 0.0f, 1.0f))) * (max(_64_m0[121u].x, 1.0f) + (-1.0f))) + 1.0f;
            float _4198;
            float _4204;
            float _4210;
            if ((_1174 & 4194304u) == 0u)
            {
                float frontier_phi_196_143_ladder;
                float frontier_phi_196_143_ladder_1;
                float frontier_phi_196_143_ladder_2;
                if ((_1174 & 8388608u) == 0u)
                {
                    float frontier_phi_196_143_ladder_147_ladder;
                    float frontier_phi_196_143_ladder_147_ladder_1;
                    float frontier_phi_196_143_ladder_147_ladder_2;
                    if ((_1174 & 50331648u) == 0u)
                    {
                        float frontier_phi_196_143_ladder_147_ladder_155_ladder;
                        float frontier_phi_196_143_ladder_147_ladder_155_ladder_1;
                        float frontier_phi_196_143_ladder_147_ladder_155_ladder_2;
                        if ((_1174 & 1032192u) == 0u)
                        {
                            float frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder;
                            float frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder_1;
                            float frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder_2;
                            if ((_1173 & 16u) == 0u)
                            {
                                float _3869 = _2426 * 2.0f;
                                float _3873 = _2423 - (_3869 * _1012);
                                float _3875 = _2425 - (_3869 * _1018);
                                float _3876 = (_2424 - (_3869 * _1015)) * _3294;
                                float _3880 = rsqrt(dot(float3(_3873, _3876, _3875), float3(_3873, _3876, _3875)));
                                float _3881 = _3873 * _3880;
                                float _3882 = _3876 * _3880;
                                float _3883 = _3875 * _3880;
                                float _4302;
                                float _4303;
                                float _4304;
                                float _4305;
                                if (_64_m0[176u].w > 0.0f)
                                {
                                    float _4105 = 1.0f - _1051;
                                    float _4108 = exp2(log2(_4105) * _64_m0[176u].w);
                                    float _4110 = clamp(1.0f - _4108, 0.0f, 1.0f);
                                    float _4114 = (sqrt(_4110 + 9.9999997473787516355514526367188e-05f) + _4108) * _4110;
                                    float _4121 = (_4114 * (_3881 - _1012)) + _1012;
                                    float _4122 = (_4114 * (_3882 - _1015)) + _1015;
                                    float _4123 = (_4114 * (_3883 - _1018)) + _1018;
                                    float _4127 = rsqrt(dot(float3(_4121, _4122, _4123), float3(_4121, _4122, _4123)));
                                    _4302 = _4105;
                                    _4303 = _4121 * _4127;
                                    _4304 = _4122 * _4127;
                                    _4305 = _4123 * _4127;
                                }
                                else
                                {
                                    _4302 = 1.0f - _1051;
                                    _4303 = _3881;
                                    _4304 = _3882;
                                    _4305 = _3883;
                                }
                                float _4307 = (_1723 >= 0.0f) ? _1723 : 8.0f;
                                float _4749;
                                if (_1731)
                                {
                                    _4749 = _4307 * _4302;
                                }
                                else
                                {
                                    _4749 = max((_4307 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_4302, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                                }
                                float4 _4754 = _36[4u].SampleLevel(_74, float3(_4303, _4304, _4305), _4749);
                                float _4906;
                                float _4908;
                                float _4910;
                                float _4912;
                                if (_1843 == 0u)
                                {
                                    _4906 = 0.0f;
                                    _4908 = 0.0f;
                                    _4910 = 0.0f;
                                    _4912 = 0.0f;
                                }
                                else
                                {
                                    float _4926 = min(_1841, _64_m0[110u].z);
                                    float _4927 = min(_1842, _64_m0[110u].w);
                                    float4 _4943 = _15[509u].SampleLevel(_72, float2(_4926, _4927), _15[557u].SampleLevel(_72, float2(_4926, _4927), 0.0f).x * _64_m0[72u].w);
                                    float _4945 = _4943.x;
                                    float _4946 = _4943.y;
                                    float _4947 = _4943.z;
                                    float _4964 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4945, max(_4946, _4947))));
                                    _4906 = _4964 * _4945;
                                    _4908 = _4964 * _4946;
                                    _4910 = _4964 * _4947;
                                    _4912 = _4943.w;
                                }
                                float _4914 = 1.0f - _4912;
                                float _4918 = (_4914 * _4754.x) + _4906;
                                float _4919 = (_4914 * _4754.y) + _4908;
                                float _4920 = (_4914 * _4754.z) + _4910;
                                float _5322;
                                float _5324;
                                float _5326;
                                if (_3254 == 0u)
                                {
                                    _5322 = _4918;
                                    _5324 = _4919;
                                    _5326 = _4920;
                                }
                                else
                                {
                                    _5322 = min(1.0f, _3255) * _4918;
                                    _5324 = min(1.0f, _3257) * _4919;
                                    _5326 = min(1.0f, _3259) * _4920;
                                }
                                float _5347 = (((min(_1051 * 0.4749999940395355224609375f, exp2(clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f) * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _1051) + (-0.015625f);
                                float _5348 = ((_1051 * 0.25f) + 0.75f) - _5347;
                                frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder = (_64_m0[59u].w * _5326) * clamp((_5348 * _1048) + _5347, 0.0f, 1.0f);
                                frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder_1 = (_64_m0[59u].w * _5324) * clamp((_5348 * _1045) + _5347, 0.0f, 1.0f);
                                frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder_2 = (_64_m0[59u].w * _5322) * clamp((_5348 * _1040) + _5347, 0.0f, 1.0f);
                            }
                            else
                            {
                                float _3896 = _2426 * 2.0f;
                                float _3900 = _2423 - (_3896 * _1012);
                                float _3902 = _2425 - (_3896 * _1018);
                                float _3903 = (_2424 - (_3896 * _1015)) * _3294;
                                float _3907 = rsqrt(dot(float3(_3900, _3903, _3902), float3(_3900, _3903, _3902)));
                                float _3908 = _3900 * _3907;
                                float _3909 = _3903 * _3907;
                                float _3910 = _3902 * _3907;
                                float _4308;
                                float _4309;
                                float _4310;
                                float _4311;
                                if (_64_m0[176u].w > 0.0f)
                                {
                                    float _4132 = 1.0f - _1051;
                                    float _4135 = exp2(log2(_4132) * _64_m0[176u].w);
                                    float _4137 = clamp(1.0f - _4135, 0.0f, 1.0f);
                                    float _4141 = (sqrt(_4137 + 9.9999997473787516355514526367188e-05f) + _4135) * _4137;
                                    float _4148 = (_4141 * (_3908 - _1012)) + _1012;
                                    float _4149 = (_4141 * (_3909 - _1015)) + _1015;
                                    float _4150 = (_4141 * (_3910 - _1018)) + _1018;
                                    float _4154 = rsqrt(dot(float3(_4148, _4149, _4150), float3(_4148, _4149, _4150)));
                                    _4308 = _4132;
                                    _4309 = _4148 * _4154;
                                    _4310 = _4149 * _4154;
                                    _4311 = _4150 * _4154;
                                }
                                else
                                {
                                    _4308 = 1.0f - _1051;
                                    _4309 = _3908;
                                    _4310 = _3909;
                                    _4311 = _3910;
                                }
                                float _4313 = (_1723 >= 0.0f) ? _1723 : 8.0f;
                                float _4760;
                                if (_1731)
                                {
                                    _4760 = _4313 * _4308;
                                }
                                else
                                {
                                    _4760 = max((_4313 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_4308, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                                }
                                float4 _4765 = _36[4u].SampleLevel(_74, float3(_4309, _4310, _4311), _4760);
                                float _4965;
                                float _4967;
                                float _4969;
                                float _4971;
                                if (_1843 == 0u)
                                {
                                    _4965 = 0.0f;
                                    _4967 = 0.0f;
                                    _4969 = 0.0f;
                                    _4971 = 0.0f;
                                }
                                else
                                {
                                    float _4991 = min((_64_m0[110u].x * (_69_m0[1140u].z * (_1012 - _1021))) + _1841, _64_m0[110u].z);
                                    float _4992 = min((_64_m0[110u].y * (_69_m0[1140u].w * (_1018 - _1023))) + _1842, _64_m0[110u].w);
                                    float4 _5008 = _15[509u].SampleLevel(_72, float2(_4991, _4992), _15[557u].SampleLevel(_72, float2(_4991, _4992), 0.0f).x * _64_m0[72u].w);
                                    float _5010 = _5008.x;
                                    float _5011 = _5008.y;
                                    float _5012 = _5008.z;
                                    float _5029 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_5010, max(_5011, _5012))));
                                    _4965 = _5029 * _5010;
                                    _4967 = _5029 * _5011;
                                    _4969 = _5029 * _5012;
                                    _4971 = _5008.w;
                                }
                                float _4973 = 1.0f - _4971;
                                float _4977 = (_4973 * _4765.x) + _4965;
                                float _4978 = (_4973 * _4765.y) + _4967;
                                float _4979 = (_4973 * _4765.z) + _4969;
                                float _5361;
                                float _5363;
                                float _5365;
                                if (_3254 == 0u)
                                {
                                    _5361 = _4977;
                                    _5363 = _4978;
                                    _5365 = _4979;
                                }
                                else
                                {
                                    _5361 = min(1.0f, _3255) * _4977;
                                    _5363 = min(1.0f, _3257) * _4978;
                                    _5365 = min(1.0f, _3259) * _4979;
                                }
                                float _5386 = (((min(_1051 * 0.4749999940395355224609375f, exp2(clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f) * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _1051) + (-0.015625f);
                                float _5387 = ((_1051 * 0.25f) + 0.75f) - _5386;
                                frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder = (_64_m0[59u].w * _5365) * clamp((_5387 * _1048) + _5386, 0.0f, 1.0f);
                                frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder_1 = (_64_m0[59u].w * _5363) * clamp((_5387 * _1045) + _5386, 0.0f, 1.0f);
                                frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder_2 = (_64_m0[59u].w * _5361) * clamp((_5387 * _1040) + _5386, 0.0f, 1.0f);
                            }
                            frontier_phi_196_143_ladder_147_ladder_155_ladder = frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder;
                            frontier_phi_196_143_ladder_147_ladder_155_ladder_1 = frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder_1;
                            frontier_phi_196_143_ladder_147_ladder_155_ladder_2 = frontier_phi_196_143_ladder_147_ladder_155_ladder_162_ladder_2;
                        }
                        else
                        {
                            float _3577 = _2426 * 2.0f;
                            float _3581 = _2423 - (_3577 * _1012);
                            float _3583 = _2425 - (_3577 * _1018);
                            float _3584 = (_2424 - (_3577 * _1015)) * _3294;
                            float _3588 = rsqrt(dot(float3(_3581, _3584, _3583), float3(_3581, _3584, _3583)));
                            float _3589 = _3581 * _3588;
                            float _3590 = _3584 * _3588;
                            float _3591 = _3583 * _3588;
                            float _4159;
                            float _4160;
                            float _4161;
                            float _4162;
                            if (_64_m0[176u].w > 0.0f)
                            {
                                float _3915 = 1.0f - _1051;
                                float _3918 = exp2(log2(_3915) * _64_m0[176u].w);
                                float _3920 = clamp(1.0f - _3918, 0.0f, 1.0f);
                                float _3924 = (sqrt(_3920 + 9.9999997473787516355514526367188e-05f) + _3918) * _3920;
                                float _3931 = (_3924 * (_3589 - _1012)) + _1012;
                                float _3932 = (_3924 * (_3590 - _1015)) + _1015;
                                float _3933 = (_3924 * (_3591 - _1018)) + _1018;
                                float _3937 = rsqrt(dot(float3(_3931, _3932, _3933), float3(_3931, _3932, _3933)));
                                _4159 = _3915;
                                _4160 = _3931 * _3937;
                                _4161 = _3932 * _3937;
                                _4162 = _3933 * _3937;
                            }
                            else
                            {
                                _4159 = 1.0f - _1051;
                                _4160 = _3589;
                                _4161 = _3590;
                                _4162 = _3591;
                            }
                            float _4164 = (_1723 >= 0.0f) ? _1723 : 8.0f;
                            float _4588;
                            if (_1731)
                            {
                                _4588 = _4164 * _4159;
                            }
                            else
                            {
                                _4588 = max((_4164 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_4159, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                            }
                            float4 _4593 = _36[4u].SampleLevel(_74, float3(_4160, _4161, _4162), _4588);
                            float _4771;
                            float _4773;
                            float _4775;
                            float _4777;
                            if (_1843 == 0u)
                            {
                                _4771 = 0.0f;
                                _4773 = 0.0f;
                                _4775 = 0.0f;
                                _4777 = 0.0f;
                            }
                            else
                            {
                                float _4791 = min(_1841, _64_m0[110u].z);
                                float _4792 = min(_1842, _64_m0[110u].w);
                                float4 _4808 = _15[509u].SampleLevel(_72, float2(_4791, _4792), _15[557u].SampleLevel(_72, float2(_4791, _4792), 0.0f).x * _64_m0[72u].w);
                                float _4810 = _4808.x;
                                float _4811 = _4808.y;
                                float _4812 = _4808.z;
                                float _4829 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4810, max(_4811, _4812))));
                                _4771 = _4829 * _4810;
                                _4773 = _4829 * _4811;
                                _4775 = _4829 * _4812;
                                _4777 = _4808.w;
                            }
                            float _4779 = 1.0f - _4777;
                            float _4783 = (_4779 * _4593.x) + _4771;
                            float _4784 = (_4779 * _4593.y) + _4773;
                            float _4785 = (_4779 * _4593.z) + _4775;
                            float _5030;
                            float _5032;
                            float _5034;
                            if (_3254 == 0u)
                            {
                                _5030 = _4783;
                                _5032 = _4784;
                                _5034 = _4785;
                            }
                            else
                            {
                                _5030 = min(1.0f, _3255) * _4783;
                                _5032 = min(1.0f, _3257) * _4784;
                                _5034 = min(1.0f, _3259) * _4785;
                            }
                            float _5055 = (((min(_1051 * 0.4749999940395355224609375f, exp2(clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f) * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _1051) + (-0.015625f);
                            float _5056 = ((_1051 * 0.25f) + 0.75f) - _5055;
                            frontier_phi_196_143_ladder_147_ladder_155_ladder = (_64_m0[59u].w * _5034) * clamp((_5056 * _1048) + _5055, 0.0f, 1.0f);
                            frontier_phi_196_143_ladder_147_ladder_155_ladder_1 = (_64_m0[59u].w * _5032) * clamp((_5056 * _1045) + _5055, 0.0f, 1.0f);
                            frontier_phi_196_143_ladder_147_ladder_155_ladder_2 = (_64_m0[59u].w * _5030) * clamp((_5056 * _1040) + _5055, 0.0f, 1.0f);
                        }
                        frontier_phi_196_143_ladder_147_ladder = frontier_phi_196_143_ladder_147_ladder_155_ladder;
                        frontier_phi_196_143_ladder_147_ladder_1 = frontier_phi_196_143_ladder_147_ladder_155_ladder_1;
                        frontier_phi_196_143_ladder_147_ladder_2 = frontier_phi_196_143_ladder_147_ladder_155_ladder_2;
                    }
                    else
                    {
                        float _3498 = 1.0f - _1051;
                        float _3499 = (-0.0f) - _962;
                        float _3500 = (-0.0f) - _956;
                        float _3501 = (-0.0f) - _950;
                        float _3503 = (1.0f - _1165) * _3498;
                        float _3506 = _3498 * 0.85000002384185791015625f;
                        float _3517 = (_1723 >= 0.0f) ? _1723 : 8.0f;
                        float _3518 = (_3503 * 0.89999997615814208984375f) + 0.10000002384185791015625f;
                        float _6152;
                        float _6153;
                        float _6154;
                        float _6155;
                        float _6156;
                        float _6157;
                        float _6158;
                        float _6159;
                        float _6160;
                        if (int(_1174) < int(0u))
                        {
                            float _3598 = (_1015 * _950) - (_1018 * _956);
                            float _3601 = (_1018 * _962) - (_1012 * _950);
                            float _3604 = (_1012 * _956) - (_1015 * _962);
                            float _3618 = float(int(asuint(_64_m0[677u]).x & 63u)) * 5.588237762451171875f;
                            float _3627 = ((_3618 + float(int(uint(int(_300))))) * 0.067110560834407806396484375f) + ((_3618 + float(int(uint(int(_301))))) * 0.005837149918079376220703125f);
                            float _3631 = frac(abs(_3627));
                            float _3634 = ((_3627 >= ((-0.0f) - _3627)) ? _3631 : ((-0.0f) - _3631)) * 52.98291778564453125f;
                            float _3639 = frac(abs(_3634));
                            float _3644 = (((_3634 >= ((-0.0f) - _3634)) ? _3639 : ((-0.0f) - _3639)) * 1.57079637050628662109375f) + 0.785398185253143310546875f;
                            float _3646 = sin(_3644);
                            float _3651 = cos(_3644) * rsqrt(dot(float3(_3598, _3601, _3604), float3(_3598, _3601, _3604)));
                            float _3655 = (_3651 * _3598) + (_3646 * _1012);
                            float _3656 = (_3651 * _3601) + (_3646 * _1015);
                            float _3657 = (_3651 * _3604) + (_3646 * _1018);
                            float _3661 = rsqrt(dot(float3(_3655, _3656, _3657), float3(_3655, _3656, _3657)));
                            float _3662 = _3655 * _3661;
                            float _3663 = _3656 * _3661;
                            float _3664 = _3657 * _3661;
                            float _3668 = dot(float3(_2423, _2424, _2425), float3(_3662, _3663, _3664)) * 2.0f;
                            float _3672 = _2423 - (_3668 * _3662);
                            float _3673 = _2424 - (_3668 * _3663);
                            float _3674 = _2425 - (_3668 * _3664);
                            float _4165;
                            if (_1731)
                            {
                                _4165 = _3517 * _3518;
                            }
                            else
                            {
                                _4165 = max((_3517 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_3518, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                            }
                            float4 _4170 = _36[4u].SampleLevel(_74, float3(_3672, _3673, _3674), _4165);
                            bool _4175 = _1843 == 0u;
                            float _4324;
                            float _4326;
                            float _4328;
                            float _4330;
                            if (_4175)
                            {
                                _4324 = 0.0f;
                                _4326 = 0.0f;
                                _4328 = 0.0f;
                                _4330 = 0.0f;
                            }
                            else
                            {
                                float _4344 = min(_1841, _64_m0[110u].z);
                                float _4345 = min(_1842, _64_m0[110u].w);
                                float4 _4361 = _15[509u].SampleLevel(_72, float2(_4344, _4345), _15[557u].SampleLevel(_72, float2(_4344, _4345), 0.0f).x * _64_m0[72u].w);
                                float _4363 = _4361.x;
                                float _4364 = _4361.y;
                                float _4365 = _4361.z;
                                float _4382 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4363, max(_4364, _4365))));
                                _4324 = _4382 * _4363;
                                _4326 = _4382 * _4364;
                                _4328 = _4382 * _4365;
                                _4330 = _4361.w;
                            }
                            float _4332 = 1.0f - _4330;
                            float _4336 = (_4332 * _4170.x) + _4324;
                            float _4337 = (_4332 * _4170.y) + _4326;
                            float _4338 = (_4332 * _4170.z) + _4328;
                            bool _4339 = _3254 == 0u;
                            float _4599;
                            float _4601;
                            float _4603;
                            if (_4339)
                            {
                                _4599 = _4336;
                                _4601 = _4337;
                                _4603 = _4338;
                            }
                            else
                            {
                                _4599 = min(1.0f, _3255) * _4336;
                                _4601 = min(1.0f, _3257) * _4337;
                                _4603 = min(1.0f, _3259) * _4338;
                            }
                            float _4608 = _3506 + 0.14999997615814208984375f;
                            float _5069;
                            if (_1731)
                            {
                                _5069 = _3517 * _4608;
                            }
                            else
                            {
                                _5069 = max((_3517 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_4608, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                            }
                            float4 _5074 = _36[4u].SampleLevel(_74, float3(_3672, _3673, _3674), _5069);
                            float _5400;
                            float _5402;
                            float _5404;
                            float _5406;
                            if (_4175)
                            {
                                _5400 = 0.0f;
                                _5402 = 0.0f;
                                _5404 = 0.0f;
                                _5406 = 0.0f;
                            }
                            else
                            {
                                float _5419 = min(_1841, _64_m0[110u].z);
                                float _5420 = min(_1842, _64_m0[110u].w);
                                float4 _5436 = _15[509u].SampleLevel(_72, float2(_5419, _5420), _15[557u].SampleLevel(_72, float2(_5419, _5420), 0.0f).x * _64_m0[72u].w);
                                float _5438 = _5436.x;
                                float _5439 = _5436.y;
                                float _5440 = _5436.z;
                                float _5457 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_5438, max(_5439, _5440))));
                                _5400 = _5457 * _5438;
                                _5402 = _5457 * _5439;
                                _5404 = _5457 * _5440;
                                _5406 = _5436.w;
                            }
                            float _5408 = 1.0f - _5406;
                            float _5412 = (_5408 * _5074.x) + _5400;
                            float _5413 = (_5408 * _5074.y) + _5402;
                            float _5414 = (_5408 * _5074.z) + _5404;
                            float _5576;
                            float _5578;
                            float _5580;
                            if (_4339)
                            {
                                _5576 = _5412;
                                _5578 = _5413;
                                _5580 = _5414;
                            }
                            else
                            {
                                _5576 = min(1.0f, _3255) * _5412;
                                _5578 = min(1.0f, _3257) * _5413;
                                _5580 = min(1.0f, _3259) * _5414;
                            }
                            float _5585 = dot(float3(_3672, _3673, _3674), float3(_958, _952, _946));
                            float _5588 = dot(float3(_3672, _3673, _3674), float3(_960, _954, _948));
                            float _5591 = dot(float3(_3672, _3673, _3674), float3(_962, _956, _950));
                            float _5594 = abs(_5585);
                            float _5595 = abs(_5588);
                            float _5596 = abs(_5591);
                            float _5598 = (_5595 + _5594) + _5596;
                            float _5613 = 1.0f - _1163;
                            float _5625 = dot(float3(_1739, _1740, _1741), float3(_3672, _3673, _3674));
                            float _5628 = dot(float3(_3499, _3500, _3501), float3(_3672, _3673, _3674));
                            float _5634 = dot(float3(_3499, _3500, _3501), float3(_1739, _1740, _1741));
                            float _5640 = _3672 - (_5628 * _3499);
                            float _5641 = _3673 - (_5628 * _3500);
                            float _5642 = _3674 - (_5628 * _3501);
                            float _5646 = _1739 - (_5634 * _3499);
                            float _5647 = _1740 - (_5634 * _3500);
                            float _5648 = _1741 - (_5634 * _3501);
                            float _5661 = rsqrt((dot(float3(_5646, _5647, _5648), float3(_5646, _5647, _5648)) * dot(float3(_5640, _5641, _5642), float3(_5640, _5641, _5642))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_5640, _5641, _5642), float3(_5646, _5647, _5648));
                            float _5663 = (_5661 * 0.5f) + 0.5f;
                            float _5665 = sqrt(clamp(_5663, 0.0f, 1.0f));
                            float _5671 = cos(abs(asin(_5634) - asin(_5628)) * 0.5f);
                            float _5677 = 1.0f / ((1.190000057220458984375f / _5671) + (_5671 * 0.36000001430511474609375f));
                            float _5678 = _5634 * 0.645161330699920654296875f;
                            float _5682 = sqrt(1.0f - (_5678 * _5678));
                            float _5689 = max(_3498 * 0.5f, 0.00999999977648258209228515625f) + 0.300000011920928955078125f;
                            float _5690 = max(_3498 * 2.0f, 0.00999999977648258209228515625f) + 0.300000011920928955078125f;
                            float _5693 = (-0.0f) - _1171;
                            float _5697 = sin(_5693);
                            float _5708 = _5634 + _5628;
                            float _5709 = _5708 - ((_5697 * 2.0f) * (((cos(_5693) * _5665) * sqrt(1.0f - (_5634 * _5634))) + (_5697 * _5634)));
                            float _5712 = (_5665 * 1.41421353816986083984375f) * (max(_3503, 0.00999999977648258209228515625f) + 0.300000011920928955078125f);
                            float _5738 = ((_5665 * 0.25f) * (exp2((((_5709 * _5709) * (-0.5f)) / (_5712 * _5712)) * 1.44269502162933349609375f) / (_5712 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(_5625, 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f);
                            float _5739 = _5708 - (_1171 * 0.5f);
                            float _5757 = clamp((-0.0f) - _5625, 0.0f, 1.0f);
                            float _5758 = _5757 * _5757;
                            float _5761 = (_5758 * _5758) * (_5757 * (1.0f - clamp(_1161 * 200.0f, 0.0f, 1.0f)));
                            float _5768 = (cos(asin((_5677 * _5665) * ((_5677 * (0.60000002384185791015625f - (_5661 * 0.800000011920928955078125f))) + 1.0f)) * 2.0f) + 1.0f) * (-2.8853900432586669921875f);
                            float _5773 = exp2(_5768 * (_1155 / _5682));
                            float _5774 = exp2(_5768 * (_1157 / _5682));
                            float _5775 = exp2(_5768 * (_1159 / _5682));
                            float _5790 = 0.95347940921783447265625f - (exp2(log2(1.0f - _5671) * 5.0f) * 0.95347940921783447265625f);
                            float _5792 = _5671 * 0.5f;
                            float4 _5801 = _15[(_1153 + 513u) + 0u].SampleLevel(_72, float2(_5792 + 0.5f, _5663), 0.0f);
                            float _5803 = _5801.x;
                            float _5804 = _5708 - (_1171 * 1.5f);
                            float _5830 = exp2(log2(1.0f - _5792) * 5.0f) * 0.95347940921783447265625f;
                            float _5832 = 0.95347940921783447265625f - _5830;
                            float _5834 = (_5832 * _5832) * (_5830 + 0.0465205647051334381103515625f);
                            float _5837 = clamp((1.0f - max(_1169, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                            float _5845 = abs(sqrt(1.0f - (_5628 * _5628)));
                            float _5846 = _5845 * clamp(dot(float3(_3672, _3673, _3674), float3(_3672, _3673, _3674)), 0.0f, 1.0f);
                            float _5848 = ((1.0f - clamp(_1161 * 66.6666717529296875f, 0.0f, 1.0f)) * 2.0f) * (1.0f - clamp(((dot(float3(_5594 / _5598, _5595 / _5598, _5596 / _5598), float3((_5585 < 0.0f) ? _970 : _964, (_5588 < 0.0f) ? _972 : _966, (_5591 < 0.0f) ? _974 : _968)) * _976) + _1161) / (((_5613 * _5613) * 0.0900000035762786865234375f) + 0.00999999977648258209228515625f), 0.0f, 1.0f));
                            float _5849 = _5848 * _64_m0[59u].w;
                            float _5859 = _5848 * _64_m0[59u].w;
                            float _5860 = _5859 * _5576;
                            float _5861 = (_5790 * _5790) * (exp2((((_5739 * _5739) * (-0.5f)) / (_5689 * _5689)) * 1.44269502162933349609375f) / (_5689 * 2.5066282749176025390625f));
                            float _5866 = _5859 * _5578;
                            float _5871 = _5859 * _5580;
                            float _5876 = (exp2(_5837 * ((_5661 * 24.5258159637451171875f) + (-24.208423614501953125f))) * _5837) * (exp2((((_5804 * _5804) * (-0.5f)) / (_5690 * _5690)) * 1.44269502162933349609375f) / (_5690 * 2.5066282749176025390625f));
                            _6152 = ((_5849 * _4599) * _5738) * _5846;
                            _6153 = ((_5849 * _4601) * _5738) * _5846;
                            _6154 = ((_5849 * _4603) * _5738) * _5846;
                            _6155 = (((_5861 * _5860) * (((1.0f - _5773) * _5761) + _5773)) * _5803) * _5845;
                            _6156 = (((_5861 * _5866) * (((1.0f - _5774) * _5761) + _5774)) * _5803) * _5845;
                            _6157 = (((_5861 * _5871) * (((1.0f - _5775) * _5761) + _5775)) * _5803) * _5845;
                            _6158 = (((_5876 * _5860) * exp2(((_1155 * (-3.2000000476837158203125f)) / _5671) * 1.44269502162933349609375f)) * _5834) * _5846;
                            _6159 = (((_5876 * _5866) * exp2(((_1157 * (-3.2000000476837158203125f)) / _5671) * 1.44269502162933349609375f)) * _5834) * _5846;
                            _6160 = (((_5876 * _5871) * exp2(((_1159 * (-3.2000000476837158203125f)) / _5671) * 1.44269502162933349609375f)) * _5834) * _5846;
                        }
                        else
                        {
                            float _3677 = (_1741 * _956) - (_1740 * _950);
                            float _3680 = (_1739 * _950) - (_1741 * _962);
                            float _3683 = (_1740 * _962) - (_1739 * _956);
                            float _3703 = (((((-0.0f) - _1012) - (_3683 * _956)) + (_3680 * _950)) * 0.60000002384185791015625f) + _1012;
                            float _3704 = (((((-0.0f) - _1015) - (_3677 * _950)) + (_3683 * _962)) * 0.60000002384185791015625f) + _1015;
                            float _3705 = (((((-0.0f) - _1018) - (_3680 * _962)) + (_3677 * _956)) * 0.60000002384185791015625f) + _1018;
                            float _3709 = rsqrt(dot(float3(_3703, _3704, _3705), float3(_3703, _3704, _3705)));
                            float _3710 = _3709 * _3703;
                            float _3711 = _3709 * _3704;
                            float _3712 = _3709 * _3705;
                            float _3716 = dot(float3(_2423, _2424, _2425), float3(_3710, _3711, _3712)) * 2.0f;
                            float _3720 = _2423 - (_3716 * _3710);
                            float _3721 = _2424 - (_3716 * _3711);
                            float _3722 = _2425 - (_3716 * _3712);
                            float _4176;
                            if (_1731)
                            {
                                _4176 = _3517 * _3518;
                            }
                            else
                            {
                                _4176 = max((_3517 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_3518, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                            }
                            float4 _4181 = _36[4u].SampleLevel(_74, float3(_3720, _3721, _3722), _4176);
                            bool _4186 = _1843 == 0u;
                            float _4383;
                            float _4385;
                            float _4387;
                            float _4389;
                            if (_4186)
                            {
                                _4383 = 0.0f;
                                _4385 = 0.0f;
                                _4387 = 0.0f;
                                _4389 = 0.0f;
                            }
                            else
                            {
                                float _4403 = min(_1841, _64_m0[110u].z);
                                float _4404 = min(_1842, _64_m0[110u].w);
                                float4 _4420 = _15[509u].SampleLevel(_72, float2(_4403, _4404), _15[557u].SampleLevel(_72, float2(_4403, _4404), 0.0f).x * _64_m0[72u].w);
                                float _4422 = _4420.x;
                                float _4423 = _4420.y;
                                float _4424 = _4420.z;
                                float _4441 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4422, max(_4423, _4424))));
                                _4383 = _4441 * _4422;
                                _4385 = _4441 * _4423;
                                _4387 = _4441 * _4424;
                                _4389 = _4420.w;
                            }
                            float _4391 = 1.0f - _4389;
                            float _4395 = (_4391 * _4181.x) + _4383;
                            float _4396 = (_4391 * _4181.y) + _4385;
                            float _4397 = (_4391 * _4181.z) + _4387;
                            bool _4398 = _3254 == 0u;
                            float _4613;
                            float _4615;
                            float _4617;
                            if (_4398)
                            {
                                _4613 = _4395;
                                _4615 = _4396;
                                _4617 = _4397;
                            }
                            else
                            {
                                _4613 = min(1.0f, _3255) * _4395;
                                _4615 = min(1.0f, _3257) * _4396;
                                _4617 = min(1.0f, _3259) * _4397;
                            }
                            float _4622 = _3506 + 0.14999997615814208984375f;
                            float _5079;
                            if (_1731)
                            {
                                _5079 = _3517 * _4622;
                            }
                            else
                            {
                                _5079 = max((_3517 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_4622, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                            }
                            float4 _5084 = _36[4u].SampleLevel(_74, float3(_3720, _3721, _3722), _5079);
                            float _5458;
                            float _5460;
                            float _5462;
                            float _5464;
                            if (_4186)
                            {
                                _5458 = 0.0f;
                                _5460 = 0.0f;
                                _5462 = 0.0f;
                                _5464 = 0.0f;
                            }
                            else
                            {
                                float _5477 = min(_1841, _64_m0[110u].z);
                                float _5478 = min(_1842, _64_m0[110u].w);
                                float4 _5494 = _15[509u].SampleLevel(_72, float2(_5477, _5478), _15[557u].SampleLevel(_72, float2(_5477, _5478), 0.0f).x * _64_m0[72u].w);
                                float _5496 = _5494.x;
                                float _5497 = _5494.y;
                                float _5498 = _5494.z;
                                float _5515 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_5496, max(_5497, _5498))));
                                _5458 = _5515 * _5496;
                                _5460 = _5515 * _5497;
                                _5462 = _5515 * _5498;
                                _5464 = _5494.w;
                            }
                            float _5466 = 1.0f - _5464;
                            float _5470 = (_5466 * _5084.x) + _5458;
                            float _5471 = (_5466 * _5084.y) + _5460;
                            float _5472 = (_5466 * _5084.z) + _5462;
                            float _5892;
                            float _5894;
                            float _5896;
                            if (_4398)
                            {
                                _5892 = _5470;
                                _5894 = _5471;
                                _5896 = _5472;
                            }
                            else
                            {
                                _5892 = min(1.0f, _3255) * _5470;
                                _5894 = min(1.0f, _3257) * _5471;
                                _5896 = min(1.0f, _3259) * _5472;
                            }
                            float _5908 = dot(float3(_3499, _3500, _3501), float3(_3720, _3721, _3722));
                            float _5914 = dot(float3(_3499, _3500, _3501), float3(_1739, _1740, _1741));
                            float _5920 = _3720 - (_5908 * _3499);
                            float _5921 = _3721 - (_5908 * _3500);
                            float _5922 = _3722 - (_5908 * _3501);
                            float _5926 = _1739 - (_5914 * _3499);
                            float _5927 = _1740 - (_5914 * _3500);
                            float _5928 = _1741 - (_5914 * _3501);
                            float _5941 = rsqrt((dot(float3(_5926, _5927, _5928), float3(_5926, _5927, _5928)) * dot(float3(_5920, _5921, _5922), float3(_5920, _5921, _5922))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_5920, _5921, _5922), float3(_5926, _5927, _5928));
                            float _5945 = sqrt(clamp((_5941 * 0.5f) + 0.5f, 0.0f, 1.0f));
                            float _5951 = cos(abs(asin(_5914) - asin(_5908)) * 0.5f);
                            float _5956 = max(_3498 * 2.0f, 0.00999999977648258209228515625f) + 0.300000011920928955078125f;
                            float _5958 = (-0.0f) - _1171;
                            float _5960 = sin(_5958);
                            float _5971 = _5914 + _5908;
                            float _5972 = _5971 - ((_5960 * 2.0f) * (((cos(_5958) * _5945) * sqrt(1.0f - (_5914 * _5914))) + (_5960 * _5914)));
                            float _5974 = (_5945 * 1.41421353816986083984375f) * (max(_3503, 0.00999999977648258209228515625f) + 0.300000011920928955078125f);
                            float _5993 = _5971 - (_1171 * 1.5f);
                            float _6019 = exp2(log2(1.0f - (_5951 * 0.5f)) * 5.0f) * 0.95347940921783447265625f;
                            float _6021 = 0.95347940921783447265625f - _6019;
                            float _6023 = (_6021 * _6021) * (_6019 + 0.0465205647051334381103515625f);
                            float _6026 = clamp((1.0f - max(_1169, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                            float _6033 = abs(sqrt(1.0f - (_5908 * _5908))) * clamp(dot(float3(_1012, _1015, _1018), float3(_3720, _3721, _3722)), 0.0f, 1.0f);
                            float _6038 = _6033 * ((((_64_m0[59u].w * 0.25f) * _5945) * (exp2((((_5972 * _5972) * (-0.5f)) / (_5974 * _5974)) * 1.44269502162933349609375f) / (_5974 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(dot(float3(_1739, _1740, _1741), float3(_3720, _3721, _3722)), 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f));
                            float _6043 = (exp2(_6026 * ((_5941 * 24.5258159637451171875f) + (-24.208423614501953125f))) * _6026) * ((exp2((((_5993 * _5993) * (-0.5f)) / (_5956 * _5956)) * 1.44269502162933349609375f) / (_5956 * 2.5066282749176025390625f)) * _64_m0[59u].w);
                            _6152 = _6038 * _4613;
                            _6153 = _6038 * _4615;
                            _6154 = _6038 * _4617;
                            _6155 = 0.0f;
                            _6156 = 0.0f;
                            _6157 = 0.0f;
                            _6158 = (((_6043 * _5892) * exp2(((_1155 * (-3.2000000476837158203125f)) / _5951) * 1.44269502162933349609375f)) * _6023) * _6033;
                            _6159 = (((_6043 * _5894) * exp2(((_1157 * (-3.2000000476837158203125f)) / _5951) * 1.44269502162933349609375f)) * _6023) * _6033;
                            _6160 = (((_6043 * _5896) * exp2(((_1159 * (-3.2000000476837158203125f)) / _5951) * 1.44269502162933349609375f)) * _6023) * _6033;
                        }
                        frontier_phi_196_143_ladder_147_ladder = ((_6157 + _6154) + _6160) * 3.1415927410125732421875f;
                        frontier_phi_196_143_ladder_147_ladder_1 = ((_6156 + _6153) + _6159) * 3.1415927410125732421875f;
                        frontier_phi_196_143_ladder_147_ladder_2 = ((_6155 + _6152) + _6158) * 3.1415927410125732421875f;
                    }
                    frontier_phi_196_143_ladder = frontier_phi_196_143_ladder_147_ladder;
                    frontier_phi_196_143_ladder_1 = frontier_phi_196_143_ladder_147_ladder_1;
                    frontier_phi_196_143_ladder_2 = frontier_phi_196_143_ladder_147_ladder_2;
                }
                else
                {
                    float _3427 = _2426 * 2.0f;
                    float _3431 = _2423 - (_3427 * _1012);
                    float _3433 = _2425 - (_3427 * _1018);
                    float _3434 = (_2424 - (_3427 * _1015)) * _3294;
                    float _3438 = rsqrt(dot(float3(_3431, _3434, _3433), float3(_3431, _3434, _3433)));
                    float _3439 = _3431 * _3438;
                    float _3440 = _3434 * _3438;
                    float _3441 = _3433 * _3438;
                    float _3723;
                    float _3724;
                    float _3725;
                    float _3726;
                    if (_64_m0[176u].w > 0.0f)
                    {
                        float _3520 = 1.0f - _1051;
                        float _3523 = exp2(log2(_3520) * _64_m0[176u].w);
                        float _3525 = clamp(1.0f - _3523, 0.0f, 1.0f);
                        float _3529 = (sqrt(_3525 + 9.9999997473787516355514526367188e-05f) + _3523) * _3525;
                        float _3536 = (_3529 * (_3439 - _1012)) + _1012;
                        float _3537 = (_3529 * (_3440 - _1015)) + _1015;
                        float _3538 = (_3529 * (_3441 - _1018)) + _1018;
                        float _3542 = rsqrt(dot(float3(_3536, _3537, _3538), float3(_3536, _3537, _3538)));
                        _3723 = _3520;
                        _3724 = _3536 * _3542;
                        _3725 = _3537 * _3542;
                        _3726 = _3538 * _3542;
                    }
                    else
                    {
                        _3723 = 1.0f - _1051;
                        _3724 = _3439;
                        _3725 = _3440;
                        _3726 = _3441;
                    }
                    float _3728 = (_1723 >= 0.0f) ? _1723 : 8.0f;
                    float _4187;
                    if (_1731)
                    {
                        _4187 = _3728 * _3723;
                    }
                    else
                    {
                        _4187 = max((_3728 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_3723, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                    }
                    float4 _4192 = _36[4u].SampleLevel(_74, float3(_3724, _3725, _3726), _4187);
                    bool _4197 = _1843 == 0u;
                    float _4442;
                    float _4444;
                    float _4446;
                    float _4448;
                    if (_4197)
                    {
                        _4442 = 0.0f;
                        _4444 = 0.0f;
                        _4446 = 0.0f;
                        _4448 = 0.0f;
                    }
                    else
                    {
                        float _4462 = min(_1841, _64_m0[110u].z);
                        float _4463 = min(_1842, _64_m0[110u].w);
                        float4 _4479 = _15[509u].SampleLevel(_72, float2(_4462, _4463), _15[557u].SampleLevel(_72, float2(_4462, _4463), 0.0f).x * _64_m0[72u].w);
                        float _4481 = _4479.x;
                        float _4482 = _4479.y;
                        float _4483 = _4479.z;
                        float _4500 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4481, max(_4482, _4483))));
                        _4442 = _4500 * _4481;
                        _4444 = _4500 * _4482;
                        _4446 = _4500 * _4483;
                        _4448 = _4479.w;
                    }
                    float _4450 = 1.0f - _4448;
                    float _4454 = (_4450 * _4192.x) + _4442;
                    float _4455 = (_4450 * _4192.y) + _4444;
                    float _4456 = (_4450 * _4192.z) + _4446;
                    bool _4457 = _3254 == 0u;
                    float _4626;
                    float _4628;
                    float _4630;
                    if (_4457)
                    {
                        _4626 = _4454;
                        _4628 = _4455;
                        _4630 = _4456;
                    }
                    else
                    {
                        _4626 = min(1.0f, _3255) * _4454;
                        _4628 = min(1.0f, _3257) * _4455;
                        _4630 = min(1.0f, _3259) * _4456;
                    }
                    float _4641 = clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f);
                    float _4646 = exp2(_4641 * (-9.27999973297119140625f));
                    float _4651 = (((min(_1051 * 0.4749999940395355224609375f, _4646) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _1051) + (-0.015625f);
                    float _4652 = ((_1051 * 0.25f) + 0.75f) - _4651;
                    float _4662 = (_64_m0[59u].w * _4626) * clamp((_4652 * _1040) + _4651, 0.0f, 1.0f);
                    float _4663 = (_64_m0[59u].w * _4628) * clamp((_4652 * _1045) + _4651, 0.0f, 1.0f);
                    float _4664 = (_64_m0[59u].w * _4630) * clamp((_4652 * _1048) + _4651, 0.0f, 1.0f);
                    float _5089;
                    float _5090;
                    float _5091;
                    float _5092;
                    if (_64_m0[176u].w > 0.0f)
                    {
                        float _4850 = 1.0f - _1088;
                        float _4853 = exp2(log2(_4850) * _64_m0[176u].w);
                        float _4855 = clamp(1.0f - _4853, 0.0f, 1.0f);
                        float _4859 = (sqrt(_4855 + 9.9999997473787516355514526367188e-05f) + _4853) * _4855;
                        float _4866 = (_4859 * (_3439 - _1012)) + _1012;
                        float _4867 = (_4859 * (_3440 - _1015)) + _1015;
                        float _4868 = (_4859 * (_3441 - _1018)) + _1018;
                        float _4872 = rsqrt(dot(float3(_4866, _4867, _4868), float3(_4866, _4867, _4868)));
                        _5089 = _4850;
                        _5090 = _4866 * _4872;
                        _5091 = _4867 * _4872;
                        _5092 = _4868 * _4872;
                    }
                    else
                    {
                        _5089 = 1.0f - _1088;
                        _5090 = _3439;
                        _5091 = _3440;
                        _5092 = _3441;
                    }
                    float _6059;
                    if (_1731)
                    {
                        _6059 = _3728 * _5089;
                    }
                    else
                    {
                        _6059 = max((_3728 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_5089, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                    }
                    float4 _6064 = _36[4u].SampleLevel(_74, float3(_5090, _5091, _5092), _6059);
                    float _6168;
                    float _6170;
                    float _6172;
                    float _6174;
                    if (_4197)
                    {
                        _6168 = 0.0f;
                        _6170 = 0.0f;
                        _6172 = 0.0f;
                        _6174 = 0.0f;
                    }
                    else
                    {
                        float _6187 = min(_1841, _64_m0[110u].z);
                        float _6188 = min(_1842, _64_m0[110u].w);
                        float4 _6204 = _15[509u].SampleLevel(_72, float2(_6187, _6188), _15[557u].SampleLevel(_72, float2(_6187, _6188), 0.0f).x * _64_m0[72u].w);
                        float _6206 = _6204.x;
                        float _6207 = _6204.y;
                        float _6208 = _6204.z;
                        float _6225 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_6206, max(_6207, _6208))));
                        _6168 = _6225 * _6206;
                        _6170 = _6225 * _6207;
                        _6172 = _6225 * _6208;
                        _6174 = _6204.w;
                    }
                    float _6176 = 1.0f - _6174;
                    float _6180 = (_6176 * _6064.x) + _6168;
                    float _6181 = (_6176 * _6064.y) + _6170;
                    float _6182 = (_6176 * _6064.z) + _6172;
                    float _6302;
                    float _6304;
                    float _6306;
                    if (_4457)
                    {
                        _6302 = _6180;
                        _6304 = _6181;
                        _6306 = _6182;
                    }
                    else
                    {
                        _6302 = min(1.0f, _3255) * _6180;
                        _6304 = min(1.0f, _3257) * _6181;
                        _6306 = min(1.0f, _3259) * _6182;
                    }
                    float _6321 = (((min(_1088 * 0.4749999940395355224609375f, _4646) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _1088) + (-0.015625f);
                    float _6322 = ((_1088 * 0.25f) + 0.75f) - _6321;
                    float _6346 = (_4641 * (_1092 + (-1.0f))) + 1.0f;
                    frontier_phi_196_143_ladder = (((((_64_m0[59u].w * _6306) * clamp((_6322 * _1048) + _6321, 0.0f, 1.0f)) - _4664) * _1090) + _4664) * _6346;
                    frontier_phi_196_143_ladder_1 = (((((_64_m0[59u].w * _6304) * clamp((_6322 * _1045) + _6321, 0.0f, 1.0f)) - _4663) * _1090) + _4663) * _6346;
                    frontier_phi_196_143_ladder_2 = (((((_64_m0[59u].w * _6302) * clamp((_6322 * _1040) + _6321, 0.0f, 1.0f)) - _4662) * _1090) + _4662) * _6346;
                }
                _4198 = frontier_phi_196_143_ladder_2;
                _4204 = frontier_phi_196_143_ladder_1;
                _4210 = frontier_phi_196_143_ladder;
            }
            else
            {
                float _3401 = 1.0f - _1051;
                float _3402 = _2426 * 2.0f;
                float _3412 = clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f);
                float _3414 = (_1723 >= 0.0f) ? _1723 : 8.0f;
                float _3547;
                if (_1731)
                {
                    _3547 = _3414 * _3401;
                }
                else
                {
                    _3547 = max((_3414 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_3401, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                }
                float4 _3552 = _36[4u].SampleLevel(_74, float3(_2423 - (_3402 * _1012), _2424 - (_3402 * _1015), _2425 - (_3402 * _1018)), _3547);
                float _3729;
                float _3731;
                float _3733;
                float _3735;
                if (_1843 == 0u)
                {
                    _3729 = 0.0f;
                    _3731 = 0.0f;
                    _3733 = 0.0f;
                    _3735 = 0.0f;
                }
                else
                {
                    float _3750 = min(_1841, _64_m0[110u].z);
                    float _3751 = min(_1842, _64_m0[110u].w);
                    float4 _3769 = _15[509u].SampleLevel(_72, float2(_3750, _3751), _15[557u].SampleLevel(_72, float2(_3750, _3751), 0.0f).x * _64_m0[72u].w);
                    float _3771 = _3769.x;
                    float _3772 = _3769.y;
                    float _3773 = _3769.z;
                    float _3793 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_3771, max(_3772, _3773))));
                    _3729 = _3793 * _3771;
                    _3731 = _3793 * _3772;
                    _3733 = _3793 * _3773;
                    _3735 = _3769.w;
                }
                float _3737 = 1.0f - _3735;
                float _3741 = (_3737 * _3552.x) + _3729;
                float _3742 = (_3737 * _3552.y) + _3731;
                float _3743 = (_3737 * _3552.z) + _3733;
                float _3972;
                float _3974;
                float _3976;
                if (_3254 == 0u)
                {
                    _3972 = _3741;
                    _3974 = _3742;
                    _3976 = _3743;
                }
                else
                {
                    _3972 = min(1.0f, _3255) * _3741;
                    _3974 = min(1.0f, _3257) * _3742;
                    _3976 = min(1.0f, _3259) * _3743;
                }
                float4 _3988 = _15[502u].SampleLevel(_72, float2(_3412, _3401), 0.0f);
                float _3990 = _3988.x;
                float _4008 = (((min(_1051 * 0.4749999940395355224609375f, exp2(_3412 * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _1051) + (-0.015625f);
                float _4010 = ((_1051 * 0.25f) + 0.75f) - _4008;
                float _4017 = clamp((_4010 * _1040) + _4008, 0.0f, 1.0f);
                float _4018 = clamp((_4010 * _1045) + _4008, 0.0f, 1.0f);
                float _4019 = clamp((_4010 * _1048) + _4008, 0.0f, 1.0f);
                _4198 = (_64_m0[59u].w * _3972) * ((((_3990 * _1096) - _4017) * _1099) + _4017);
                _4204 = (_64_m0[59u].w * _3974) * ((((_3990 * _1097) - _4018) * _1099) + _4018);
                _4210 = (_64_m0[59u].w * _3976) * ((((_3990 * _1098) - _4019) * _1099) + _4019);
            }
            precise float _4216 = _1067 * _4210;
            precise float _4217 = _1067 * _4204;
            precise float _4218 = _1067 * _4198;
            _4501 = _4218 * _2617;
            _4502 = _4217 * _2617;
            _4503 = _4216 * _2617;
        }
        else
        {
            float _3352;
            uint _3353;
            float _3354;
            float _3356;
            float _3358;
            float _3359;
            float _3361;
            float _3363;
            float _3365;
            float _3367;
            float _3369;
            if (((_447 & 4u) == 0u) || (asuint(_69_m0[157u]).y == 0u))
            {
                _3352 = _2617;
                _3353 = 0u;
                _3354 = 0.0f;
                _3356 = 0.0f;
                _3358 = 1.0f;
                _3359 = 1.0f;
                _3361 = 1.0f;
                _3363 = 0.0f;
                _3365 = 0.0f;
                _3367 = 0.0f;
                _3369 = 0.0f;
            }
            else
            {
                float4 _3385 = _15[2u].SampleLevel(_72, float2(_1373, _1374), 0.0f);
                _3352 = 1.0f;
                _3353 = 1u;
                _3354 = _69_m0[157u].z;
                _3356 = _69_m0[157u].w;
                _3358 = _2617;
                _3359 = _69_m0[158u].x;
                _3361 = _69_m0[158u].y;
                _3363 = _64_m0[59u].w * _3385.x;
                _3365 = _64_m0[59u].w * _3385.y;
                _3367 = _64_m0[59u].w * _3385.z;
                _3369 = clamp(_3385.w, 0.0f, 1.0f);
            }
            uint _3373 = (_435 + 48u) + 0u;
            float _3374 = _2426 * 2.0f;
            float _3378 = _2423 - (_3374 * _1012);
            float _3379 = _2424 - (_3374 * _1015);
            float _3380 = _2425 - (_3374 * _1018);
            float _4222;
            float _4224;
            float _4226;
            if (_1180)
            {
                float _3416 = _2635 * _1067;
                float _3417 = clamp(_1051, 0.0f, 1.0f);
                float _3419 = (_1723 >= 0.0f) ? _1723 : 8.0f;
                float _3420 = 1.0f - _3417;
                float _3558;
                if (_1731)
                {
                    _3558 = _3420 * _3419;
                }
                else
                {
                    _3558 = max((_3419 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_3420, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                }
                float4 _3563 = _36[NonUniformResourceIndex(_3373)].SampleLevel(_74, float3(_3378, _3379, _3380), _3558);
                float _3794;
                float _3796;
                float _3798;
                float _3800;
                if (_1843 == 0u)
                {
                    _3794 = 0.0f;
                    _3796 = 0.0f;
                    _3798 = 0.0f;
                    _3800 = 0.0f;
                }
                else
                {
                    float _3820 = min(_1841, _64_m0[110u].z);
                    float _3821 = min(_1842, _64_m0[110u].w);
                    float4 _3837 = _15[509u].SampleLevel(_72, float2(_3820, _3821), _15[557u].SampleLevel(_72, float2(_3820, _3821), 0.0f).x * _64_m0[72u].w);
                    float _3839 = _3837.x;
                    float _3840 = _3837.y;
                    float _3841 = _3837.z;
                    float _3858 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_3839, max(_3840, _3841))));
                    _3794 = _3858 * _3839;
                    _3796 = _3858 * _3840;
                    _3798 = _3858 * _3841;
                    _3800 = _3837.w;
                }
                float _3802 = 1.0f - _3800;
                float _3812 = ((_3802 * _3563.x) + _3794) * _64_m0[59u].w;
                float _3813 = ((_3802 * _3563.y) + _3796) * _64_m0[59u].w;
                float _3814 = ((_3802 * _3563.z) + _3798) * _64_m0[59u].w;
                float _4035;
                float _4037;
                float _4039;
                if (_3353 == 0u)
                {
                    _4035 = _3812;
                    _4037 = _3813;
                    _4039 = _3814;
                }
                else
                {
                    float _4072 = clamp((_3417 - _3354) / (_3356 - _3354), 0.0f, 1.0f);
                    float _4076 = (_4072 * _4072) * (3.0f - (_4072 * 2.0f));
                    float _4081 = 1.0f - (_4076 * _3369);
                    float _4093 = (((_4076 * (_3361 - _3359)) + _3359) * (_3358 + (-1.0f))) + 1.0f;
                    _4035 = ((_4081 * _3812) + (_4076 * _3363)) * _4093;
                    _4037 = ((_4081 * _3813) + (_4076 * _3365)) * _4093;
                    _4039 = ((_4081 * _3814) + (_4076 * _3367)) * _4093;
                }
                float _4059 = (max((asuint(_64_m0[85u]).z == 0u) ? (_3417 * 0.75f) : _3417, _1040) - _1040) * exp2(log2(1.0f - clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f);
                _4222 = (_4035 * _3416) * (_4059 + _1040);
                _4224 = (_4037 * _3416) * (_4059 + _1040);
                _4226 = (_4039 * _3416) * (_4059 + _1040);
            }
            else
            {
                float _3569;
                float _3570;
                float _3571;
                float _3572;
                if (_64_m0[176u].w > 0.0f)
                {
                    float _3468 = 1.0f - _1051;
                    float _3471 = exp2(log2(_3468) * _64_m0[176u].w);
                    float _3473 = clamp(1.0f - _3471, 0.0f, 1.0f);
                    float _3477 = (sqrt(_3473 + 9.9999997473787516355514526367188e-05f) + _3471) * _3473;
                    float _3484 = (_3477 * (_3378 - _1012)) + _1012;
                    float _3485 = (_3477 * (_3379 - _1015)) + _1015;
                    float _3486 = (_3477 * (_3380 - _1018)) + _1018;
                    float _3490 = rsqrt(dot(float3(_3484, _3485, _3486), float3(_3484, _3485, _3486)));
                    _3569 = _3468;
                    _3570 = _3484 * _3490;
                    _3571 = _3485 * _3490;
                    _3572 = _3486 * _3490;
                }
                else
                {
                    _3569 = 1.0f - _1051;
                    _3570 = _3378;
                    _3571 = _3379;
                    _3572 = _3380;
                }
                float _3574 = (_1723 >= 0.0f) ? _1723 : 8.0f;
                float _4094;
                if (_1731)
                {
                    _4094 = _3574 * _3569;
                }
                else
                {
                    _4094 = max((_3574 + (-9.0f)) + ((_69_m0[107u].x * 9.0f) * exp2(log2(clamp(_3569, 0.0f, 1.0f)) * _69_m0[107u].z)), 0.0f);
                }
                float4 _4099 = _36[NonUniformResourceIndex(_3373)].SampleLevel(_74, float3(_3570, _3571, _3572), _4094);
                float _4237;
                float _4239;
                float _4241;
                float _4243;
                if (_1843 == 0u)
                {
                    _4237 = 0.0f;
                    _4239 = 0.0f;
                    _4241 = 0.0f;
                    _4243 = 0.0f;
                }
                else
                {
                    float _4263 = min(_1841, _64_m0[110u].z);
                    float _4264 = min(_1842, _64_m0[110u].w);
                    float4 _4280 = _15[509u].SampleLevel(_72, float2(_4263, _4264), _15[557u].SampleLevel(_72, float2(_4263, _4264), 0.0f).x * _64_m0[72u].w);
                    float _4282 = _4280.x;
                    float _4283 = _4280.y;
                    float _4284 = _4280.z;
                    float _4301 = (1.0f / max(_64_m0[59u].w * _15[582u].Load(int3(uint2(1u, 0u), 0u)).x, 1.0000000133514319600180897396058e-10f)) * (1.0f / max(9.9999997473787516355514526367188e-06f, 1.0f - max(_4282, max(_4283, _4284))));
                    _4237 = _4301 * _4282;
                    _4239 = _4301 * _4283;
                    _4241 = _4301 * _4284;
                    _4243 = _4280.w;
                }
                float _4245 = 1.0f - _4243;
                float _4255 = ((_4245 * _4099.x) + _4237) * _64_m0[59u].w;
                float _4256 = ((_4245 * _4099.y) + _4239) * _64_m0[59u].w;
                float _4257 = ((_4245 * _4099.z) + _4241) * _64_m0[59u].w;
                float _4509;
                float _4511;
                float _4513;
                if (_3353 == 0u)
                {
                    _4509 = _4255;
                    _4511 = _4256;
                    _4513 = _4257;
                }
                else
                {
                    float _4546 = clamp((_1051 - _3354) / (_3356 - _3354), 0.0f, 1.0f);
                    float _4550 = (_4546 * _4546) * (3.0f - (_4546 * 2.0f));
                    float _4555 = 1.0f - (_4550 * _3369);
                    float _4567 = (((_4550 * (_3361 - _3359)) + _3359) * (_3358 + (-1.0f))) + 1.0f;
                    _4509 = ((_4555 * _4255) + (_4550 * _3363)) * _4567;
                    _4511 = ((_4555 * _4256) + (_4550 * _3365)) * _4567;
                    _4513 = ((_4555 * _4257) + (_4550 * _3367)) * _4567;
                }
                float _4528 = (((min(_1051 * 0.4749999940395355224609375f, exp2(clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f) * (-9.27999973297119140625f))) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _1051) + (-0.015625f);
                float _4529 = ((_1051 * 0.25f) + 0.75f) - _4528;
                float _4539 = _2635 * _1067;
                _4222 = (_4509 * _4539) * clamp((_4529 * _1040) + _4528, 0.0f, 1.0f);
                _4224 = (_4511 * _4539) * clamp((_4529 * _1045) + _4528, 0.0f, 1.0f);
                _4226 = (_4513 * _4539) * clamp((_4529 * _1048) + _4528, 0.0f, 1.0f);
            }
            _4501 = (_4222 * _3352) + (_2939 * _2619);
            _4502 = (_4224 * _3352) + (_2941 * _2619);
            _4503 = (_4226 * _3352) + (_2943 * _2619);
        }
        float _4504 = (((_2397 * (1.0f / (1.0f - (_2405 * min(0.999000012874603271484375f, _1111))))) * (_2756 - (_2756 * _555))) + (_2688 * ((_2620 * _2249) + _2205))) + _3015;
        float _4505 = (((_2397 * (1.0f / (1.0f - (_2405 * min(0.999000012874603271484375f, _1113))))) * (_2767 - (_2767 * _555))) + (_2693 * ((_2620 * _2250) + _2207))) + _3018;
        float _4506 = (((_2397 * (1.0f / (1.0f - (_2405 * min(0.999000012874603271484375f, _1115))))) * (_2778 - (_2778 * _555))) + (_2698 * ((_2620 * _2251) + _2209))) + _3021;
        float _4672;
        float _4674;
        float _4676;
        float _4678;
        float _4680;
        float _4682;
        if ((_447 & 2u) == 0u)
        {
            _4672 = 0.0f;
            _4674 = 0.0f;
            _4676 = 0.0f;
            _4678 = 0.0f;
            _4680 = 0.0f;
            _4682 = 0.0f;
        }
        else
        {
            float _4688 = _1176 ? 1.0f : ((_1410 * (_69_m0[1162u].y - _69_m0[1162u].x)) + _69_m0[1162u].x);
            float _4689 = _1176 ? 1.0f : _69_m0[_1381 + 1156u].w;
            bool _4692 = ((_448 & 2097152u) == 0u) && _1177;
            float _4719 = clamp((_1380 - _64_m0[109u].y) / (_64_m0[109u].z - _64_m0[109u].y), 0.0f, 1.0f);
            uint4 _4737 = _9.Load((_426 * 57u) + 2u);
            uint _4738 = _4737.x;
            float _4743 = asfloat(_9.Load((_426 * 57u) + 3u).x);
            float _4897;
            float _4899;
            float _4901;
            if (_64_m0[30u].x > 0.0f)
            {
                float _4900;
                float _4902;
                float _5562;
                float _5563;
                if (_2243)
                {
                    float4 _5295 = _26.Load(int3(uint2(_2243 ? _2038 : 0u, _2243 ? _2039 : 0u), 0u));
                    _5562 = _5295.x;
                    _5563 = _5295.y;
                    _4902 = _5295.z;
                    _4900 = _5295.w;
                }
                else
                {
                    float4 _5307 = _26.SampleLevel(_77, float2((1.0f / _69_m0[1140u].x) * _312, (1.0f / _69_m0[1140u].y) * _314), 0.0f);
                    _5562 = _5307.x;
                    _5563 = _5307.y;
                    _4902 = _5307.z;
                    _4900 = _5307.w;
                }
                _4897 = ((_4902 * _64_m0[30u].x) * min(_5562, min(_5563, 1.0f))) + _64_m0[30u].y;
                _4899 = _4900;
                _4901 = _4902;
            }
            else
            {
                _4897 = 1.0f;
                _4899 = 1.0f;
                _4901 = 0.0f;
            }
            float _5569;
            if ((_1174 & 1073741824u) == 0u)
            {
                float frontier_phi_272_255_ladder;
                if ((int(_1174) > int(4294967295u)) || (_4738 == 0u))
                {
                    frontier_phi_272_255_ladder = 9899999600270360182784.0f;
                }
                else
                {
                    frontier_phi_272_255_ladder = ((1.0f - _4899) / (1.00000095367431640625f - _4897)) * _4743;
                }
                _5569 = frontier_phi_272_255_ladder;
            }
            else
            {
                _5569 = max(_1130, ((1.0f - _4899) / (1.00000095367431640625f - _4897)) * _4743);
            }
            bool _5572 = (_1174 & 8388608u) == 0u;
            float _6268;
            float _6271;
            float _6272;
            if (_5572)
            {
                float frontier_phi_296_286_ladder;
                float frontier_phi_296_286_ladder_1;
                float frontier_phi_296_286_ladder_2;
                if ((_1174 & 67108864u) == 0u)
                {
                    float frontier_phi_296_286_ladder_295_ladder;
                    float frontier_phi_296_286_ladder_295_ladder_1;
                    float frontier_phi_296_286_ladder_295_ladder_2;
                    if ((_1174 & 50331648u) == 0u)
                    {
                        float frontier_phi_296_286_ladder_295_ladder_299_ladder;
                        float frontier_phi_296_286_ladder_295_ladder_299_ladder_1;
                        float frontier_phi_296_286_ladder_295_ladder_299_ladder_2;
                        if ((_1174 & 256u) == 0u)
                        {
                            frontier_phi_296_286_ladder_295_ladder_299_ladder = _4897;
                            frontier_phi_296_286_ladder_295_ladder_299_ladder_1 = _4897;
                            frontier_phi_296_286_ladder_295_ladder_299_ladder_2 = _4897;
                        }
                        else
                        {
                            float _6386 = 1.0f - _4897;
                            float _6270 = ((_6386 * _1128) * exp2(log2(1.0f - (clamp((((1.0f - _4899) / (1.00000095367431640625f - _4897)) * _4743) / _1124, 0.0f, 1.0f) * _6386)) * _1126)) + _4897;
                            frontier_phi_296_286_ladder_295_ladder_299_ladder = _6270;
                            frontier_phi_296_286_ladder_295_ladder_299_ladder_1 = _4897;
                            frontier_phi_296_286_ladder_295_ladder_299_ladder_2 = _6270;
                        }
                        frontier_phi_296_286_ladder_295_ladder = frontier_phi_296_286_ladder_295_ladder_299_ladder;
                        frontier_phi_296_286_ladder_295_ladder_1 = frontier_phi_296_286_ladder_295_ladder_299_ladder_1;
                        frontier_phi_296_286_ladder_295_ladder_2 = frontier_phi_296_286_ladder_295_ladder_299_ladder_2;
                    }
                    else
                    {
                        float _6269 = min(_4897, 1.0f - clamp(_5569 * 66.6666717529296875f, 0.0f, 1.0f));
                        frontier_phi_296_286_ladder_295_ladder = _6269;
                        frontier_phi_296_286_ladder_295_ladder_1 = _6269;
                        frontier_phi_296_286_ladder_295_ladder_2 = 1.0f - exp2(log2(1.0f - _4899) * 3.0f);
                    }
                    frontier_phi_296_286_ladder = frontier_phi_296_286_ladder_295_ladder;
                    frontier_phi_296_286_ladder_1 = frontier_phi_296_286_ladder_295_ladder_1;
                    frontier_phi_296_286_ladder_2 = frontier_phi_296_286_ladder_295_ladder_2;
                }
                else
                {
                    frontier_phi_296_286_ladder = _4897;
                    frontier_phi_296_286_ladder_1 = _4897;
                    frontier_phi_296_286_ladder_2 = _4897;
                }
                _6268 = frontier_phi_296_286_ladder;
                _6271 = frontier_phi_296_286_ladder_1;
                _6272 = frontier_phi_296_286_ladder_2;
            }
            else
            {
                _6268 = _4897;
                _6271 = _4897;
                _6272 = 1.0f - clamp(_5569 / max(_1132, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f);
            }
            float _6275 = ((((clamp(exp2(log2(_2397) * _64_m0[162u].z), 0.0f, 1.0f) + (-1.0f)) * _64_m0[98u].y) + 1.0f) * _2633) * (((((_4719 * _4719) * _64_m0[109u].w) * (3.0f - (_4719 * 2.0f))) * (clamp(exp2(log2(_2219) * _64_m0[109u].x), 0.0f, 1.0f) + (-1.0f))) + 1.0f);
            float _6276 = _6268 * _6275;
            float _6277 = _6271 * _6275;
            float _6279 = (_4901 * _6275) * _6272;
            float _6284 = cos(_64_m0[125u].x);
            float _6285 = _2426 * 2.0f;
            float _6289 = _2423 - (_6285 * _1012);
            float _6290 = _2424 - (_6285 * _1015);
            float _6291 = _2425 - (_6285 * _1018);
            float _6292 = dot(float3(_69_m0[0u].xyz), float3(_6289, _6290, _6291));
            float _6298 = _6289 - (_6292 * _69_m0[0u].x);
            float _6299 = _6290 - (_6292 * _69_m0[0u].y);
            float _6300 = _6291 - (_6292 * _69_m0[0u].z);
            float _6381;
            float _6382;
            float _6383;
            if (_6292 < _6284)
            {
                float _6367 = rsqrt(dot(float3(_6298, _6299, _6300), float3(_6298, _6299, _6300))) * sin(_64_m0[125u].x);
                float _6371 = (_6367 * _6298) + (_6284 * _69_m0[0u].x);
                float _6372 = (_6367 * _6299) + (_6284 * _69_m0[0u].y);
                float _6373 = (_6367 * _6300) + (_6284 * _69_m0[0u].z);
                float _6377 = rsqrt(dot(float3(_6371, _6372, _6373), float3(_6371, _6372, _6373)));
                _6381 = _6371 * _6377;
                _6382 = _6372 * _6377;
                _6383 = _6373 * _6377;
            }
            else
            {
                _6381 = _6289;
                _6382 = _6290;
                _6383 = _6291;
            }
            float _6532;
            float _6537;
            float _6542;
            float _6547;
            float _6553;
            float _6559;
            float _6565;
            if ((_1174 & 4194304u) == 0u)
            {
                float frontier_phi_309_304_ladder;
                float frontier_phi_309_304_ladder_1;
                float frontier_phi_309_304_ladder_2;
                float frontier_phi_309_304_ladder_3;
                float frontier_phi_309_304_ladder_4;
                float frontier_phi_309_304_ladder_5;
                float frontier_phi_309_304_ladder_6;
                if (_5572)
                {
                    float frontier_phi_309_304_ladder_306_ladder;
                    float frontier_phi_309_304_ladder_306_ladder_1;
                    float frontier_phi_309_304_ladder_306_ladder_2;
                    float frontier_phi_309_304_ladder_306_ladder_3;
                    float frontier_phi_309_304_ladder_306_ladder_4;
                    float frontier_phi_309_304_ladder_306_ladder_5;
                    float frontier_phi_309_304_ladder_306_ladder_6;
                    if ((_1174 & 67108864u) == 0u)
                    {
                        float frontier_phi_309_304_ladder_306_ladder_310_ladder;
                        float frontier_phi_309_304_ladder_306_ladder_310_ladder_1;
                        float frontier_phi_309_304_ladder_306_ladder_310_ladder_2;
                        float frontier_phi_309_304_ladder_306_ladder_310_ladder_3;
                        float frontier_phi_309_304_ladder_306_ladder_310_ladder_4;
                        float frontier_phi_309_304_ladder_306_ladder_310_ladder_5;
                        float frontier_phi_309_304_ladder_306_ladder_310_ladder_6;
                        if ((_1174 & 50331648u) == 0u)
                        {
                            float frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder;
                            float frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_1;
                            float frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_2;
                            float frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_3;
                            float frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_4;
                            float frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_5;
                            float frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_6;
                            if ((_1174 & 1032192u) == 0u)
                            {
                                float _6536;
                                float _6541;
                                float _6546;
                                float _6566;
                                if ((_1104 > 0.0f) && ((_1174 & 3u) != 0u))
                                {
                                    float _7397 = max(max(max(max(_1107, 0.0f), _1104), _1109), 0.0f);
                                    float _7403 = _64_m0[8u].x - _997;
                                    float _7404 = _64_m0[8u].y - _998;
                                    float _7405 = _64_m0[8u].z - _999;
                                    float _7417 = clamp(2.0f - (clamp(sqrt(((_7403 * _7403) + (_7404 * _7404)) + (_7405 * _7405)) * 0.02500000037252902984619140625f, 0.0f, 1.0f) * 2.0f), 0.0f, 1.0f);
                                    float _7418 = _7417 * _7397;
                                    float _7424 = _64_m0[21u].x - _997;
                                    float _7425 = _64_m0[21u].y - _998;
                                    float _7426 = _64_m0[21u].z - _999;
                                    float _7433 = log2(sqrt(((_7424 * _7424) + (_7425 * _7425)) + (_7426 * _7426)));
                                    float _7434 = _7433 * 0.85000002384185791015625f;
                                    float _8063;
                                    float _8064;
                                    float _8065;
                                    float _8066;
                                    if (_7418 > 0.00999999977648258209228515625f)
                                    {
                                        uint _7888 = _1106 + 0u;
                                        float _7889 = ceil(_7434);
                                        float _7893 = _997 * 5.0f;
                                        float _7894 = _999 * 5.0f;
                                        float _7896 = exp2((-0.0f) - max(1.0f, _7889));
                                        float _7902 = exp2((-0.0f) - max(1.0f, _7889 + 1.0f));
                                        float4 _7911 = _39[NonUniformResourceIndex(_7888)].SampleLevel(_77, float2(frac(_7896 * _7893), frac(_7896 * _7894)), 0.0f);
                                        float4 _7916 = _39[NonUniformResourceIndex(_7888)].SampleLevel(_77, float2(frac(_7902 * _7893), frac(_7902 * _7894)), 0.0f);
                                        float _7921 = frac(_7434);
                                        float _7923 = (_7921 + 0.5f) * 0.5f;
                                        float _7935 = (_7911.x + (-0.5f)) * 2.0f;
                                        float _7936 = (_7911.y + (-0.5f)) * 2.0f;
                                        float _7942 = sqrt(clamp((1.0f - (_7935 * _7935)) - (_7936 * _7936), 0.0f, 1.0f));
                                        float _7949 = (_7916.x + (-0.5f)) * 2.0f;
                                        float _7950 = (_7916.y + (-0.5f)) * 2.0f;
                                        float _7956 = sqrt(clamp((1.0f - (_7949 * _7949)) - (_7950 * _7950), 0.0f, 1.0f));
                                        float _7962 = rsqrt(dot(float3(_7935, _7942, _7936), float3(_7935, _7942, _7936))) * (1.0f - _7921);
                                        float _7965 = rsqrt(dot(float3(_7949, _7956, _7950), float3(_7949, _7956, _7950))) * _7921;
                                        float _7968 = (_7965 * _7949) + (_7962 * _7935);
                                        float _7969 = (_7965 * _7950) + (_7962 * _7936);
                                        float _7973 = rsqrt(dot(float3(_7968, _7969, 1.0f), float3(_7968, _7969, 1.0f)));
                                        float _7976 = mad(_7973, _1000, _7968 * _7973);
                                        float _7977 = mad(_7973, _1004, 0.0f);
                                        float _7978 = mad(_7973, _1008, _7969 * _7973);
                                        float _7982 = rsqrt(dot(float3(_7976, _7977, _7978), float3(_7976, _7977, _7978)));
                                        float _7983 = _7982 * _7976;
                                        float _7984 = _7982 * _7977;
                                        float _7985 = _7982 * _7978;
                                        float _7986 = _1740 + 1.0f;
                                        float _7990 = rsqrt(dot(float3(_1739, _7986, _1741), float3(_1739, _7986, _1741)));
                                        float _8041 = (((((_7418 * 2.2000000476837158203125f) * max(clamp(_7923 * (_7911.z - _7921), 0.0f, 1.0f), clamp((1.0f - _7923) * ((_7921 + (-1.0f)) + _7916.z), 0.0f, 1.0f))) * exp2((800.0f - (clamp(clamp((_7433 * 0.425000011920928955078125f) + (-1.0f), 0.0f, 1.0f), 0.0f, 1.0f) * 800.0f)) * log2(((_7397 * (0.004999999888241291046142578125f - (_1109 * 0.00299999979324638843536376953125f))) + 0.00200000009499490261077880859375f) + max(dot(float3(_7990 * _1739, _7990 * _7986, _7990 * _1741), float3(_7983, _7984, _7985)), 0.0f)))) * exp2(log2(clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 1.2000000476837158203125f)) * clamp((dot(float3(_6381, _6382, _6383), float3(_1000, _1004, _1008)) * 10.0f) + 0.5f, 0.0f, 1.0f)) * _64_m0[166u].y;
                                        float _8044 = (_1109 * 0.4000000059604644775390625f) * _7417;
                                        float _8047 = ((_7985 + (-1.0f)) * _8044) + 1.0f;
                                        float _8050 = _8044 * _1008;
                                        float _8053 = (_8047 * _1000) + (_7983 * _8050);
                                        float _8054 = (_8047 * _1004) + (_7984 * _8050);
                                        float _8055 = _8047 * _1008;
                                        float _8059 = rsqrt(dot(float3(_8053, _8054, _8055), float3(_8053, _8054, _8055)));
                                        _8063 = _8041;
                                        _8064 = _8059 * _8053;
                                        _8065 = _8059 * _8054;
                                        _8066 = _8059 * _8055;
                                    }
                                    else
                                    {
                                        _8063 = 0.0f;
                                        _8064 = _1000;
                                        _8065 = _1004;
                                        _8066 = _1008;
                                    }
                                    float _8067 = dot(float3(_8064, _8065, _8066), float3(_69_m0[0u].xyz));
                                    float _8073 = (1.0f - clamp(_8067 * 5.0f, 0.0f, 1.0f)) * _1107;
                                    float _8080 = (_8073 * (_1025 - _1111)) + _1111;
                                    float _8081 = (_8073 * (_1030 - _1113)) + _1113;
                                    float _8082 = (_8073 * (_1035 - _1115)) + _1115;
                                    float _8083 = 1.0f - _1040;
                                    float _8084 = 1.0f - _1045;
                                    float _8085 = 1.0f - _1048;
                                    float _8093 = exp2(log2(1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_8064, _8065, _8066)), 0.0f, 1.0f)) * 5.0f);
                                    float _8100 = _1109 * 0.550000011920928955078125f;
                                    float _8102 = ((_8093 * _8083) + _1040) * _8100;
                                    float _8103 = ((_8093 * _8084) + _1045) * _8100;
                                    float _8104 = ((_8093 * _8085) + _1048) * _8100;
                                    float _8128 = clamp(_8067, 0.0f, 1.0f);
                                    float _8386;
                                    if (_64_m0[76u].w > 0.5f)
                                    {
                                        float _8349 = _1739 + _69_m0[0u].x;
                                        float _8350 = _1740 + _69_m0[0u].y;
                                        float _8351 = _1741 + _69_m0[0u].z;
                                        float _8355 = rsqrt(dot(float3(_8349, _8350, _8351), float3(_8349, _8350, _8351)));
                                        float _8362 = clamp(dot(float3(_69_m0[0u].xyz), float3(_8355 * _8349, _8355 * _8350, _8355 * _8351)), 0.0f, 1.0f);
                                        float _8372 = ((((_8362 * _8362) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                        _8386 = ((exp2(log2(1.0f - clamp(dot(float3(_8064, _8065, _8066), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _8372) + 1.0f) * ((_8372 * exp2(log2(1.0f - _8128) * 5.0f)) + 1.0f);
                                    }
                                    else
                                    {
                                        _8386 = 1.0f;
                                    }
                                    float _8388 = _8386 * (clamp((_8128 * 0.722500026226043701171875f) + 0.12750001251697540283203125f, 0.0f, 1.0f) * 0.3183098733425140380859375f);
                                    _6536 = (_8388 * _8083) * (((_8102 * _8102) * ((0.949999988079071044921875f - (_1107 * 0.5f)) - _8080)) + _8080);
                                    _6541 = (_8388 * _8084) * (((_8103 * _8103) * ((0.810000002384185791015625f - (_1107 * 0.069999992847442626953125f)) - _8081)) + _8081);
                                    _6546 = (_8388 * _8085) * (((_8104 * _8104) * (((_1107 * 0.37999999523162841796875f) + 0.569999992847442626953125f) - _8082)) + _8082);
                                    _6566 = _8063;
                                }
                                else
                                {
                                    float _7442 = clamp(dot(float3(_1000, _1004, _1008), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                                    float _7443 = clamp(_7442, 0.0f, 1.0f);
                                    float _8175;
                                    if (_64_m0[76u].w > 0.5f)
                                    {
                                        float _8138 = _1739 + _69_m0[0u].x;
                                        float _8139 = _1740 + _69_m0[0u].y;
                                        float _8140 = _1741 + _69_m0[0u].z;
                                        float _8144 = rsqrt(dot(float3(_8138, _8139, _8140), float3(_8138, _8139, _8140)));
                                        float _8151 = clamp(dot(float3(_69_m0[0u].xyz), float3(_8144 * _8138, _8144 * _8139, _8144 * _8140)), 0.0f, 1.0f);
                                        float _8161 = ((((_8151 * _8151) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                        _8175 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _8161) + 1.0f) * ((_8161 * exp2(log2(1.0f - _7442) * 5.0f)) + 1.0f);
                                    }
                                    else
                                    {
                                        _8175 = 1.0f;
                                    }
                                    _6536 = (((_1025 * 0.3183098733425140380859375f) * (1.0f - _1040)) * _7443) * _8175;
                                    _6541 = (((_1030 * 0.3183098733425140380859375f) * (1.0f - _1045)) * _7443) * _8175;
                                    _6546 = (((_1035 * 0.3183098733425140380859375f) * (1.0f - _1048)) * _7443) * _8175;
                                    _6566 = 0.0f;
                                }
                                float _8398 = clamp(dot(float3(_1012, _1015, _1018), float3(_6381, _6382, _6383)), 0.0f, 1.0f);
                                float _8589;
                                float _8591;
                                float _8593;
                                if (_8398 > 0.0f)
                                {
                                    float _8526 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                    float _8527 = _6381 + _1739;
                                    float _8528 = _6382 + _1740;
                                    float _8529 = _6383 + _1741;
                                    float _8533 = rsqrt(dot(float3(_8527, _8528, _8529), float3(_8527, _8528, _8529)));
                                    float _8534 = _8533 * _8527;
                                    float _8535 = _8533 * _8528;
                                    float _8536 = _8533 * _8529;
                                    float _8545 = clamp(dot(float3(_1012, _1015, _1018), float3(_8534, _8535, _8536)), 0.0f, 1.0f);
                                    float _8550 = _8526 * _8526;
                                    float _8551 = _8550 * _8550;
                                    float _8555 = (((_8545 * _8551) - _8545) * _8545) + 1.0f;
                                    float _8559 = _8550 * 0.5f;
                                    float _8560 = 1.0f - _8559;
                                    float _8567 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_8534, _8535, _8536)), 0.0f, 1.0f);
                                    float _8568 = _8567 * _8567;
                                    float _8570 = (_8568 * _8568) * _8567;
                                    float _8582 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _8560) + _8559) * ((_8560 * _8398) + _8559))) * (_8551 / ((_8555 * _8555) * 3.1415927410125732421875f)), _1719) * _4688;
                                    float _8586 = min(_8582 * ((_8570 * (1.0f - _1040)) + _1040), 100000.0f);
                                    float _8587 = min(_8582 * ((_8570 * (1.0f - _1045)) + _1045), 100000.0f);
                                    float _8588 = min(_8582 * ((_8570 * (1.0f - _1048)) + _1048), 100000.0f);
                                    float _8733;
                                    float _8735;
                                    float _8737;
                                    if (_4692)
                                    {
                                        _8733 = _8586;
                                        _8735 = _8587;
                                        _8737 = _8588;
                                    }
                                    else
                                    {
                                        float _8739 = 1.0f - _8398;
                                        float _8740 = _8739 * _8739;
                                        float _8742 = 1.0f - (_8740 * _8740);
                                        _8733 = _8586 * _8742;
                                        _8735 = _8587 * _8742;
                                        _8737 = _8588 * _8742;
                                    }
                                    _8589 = _8733 * _8398;
                                    _8591 = _8735 * _8398;
                                    _8593 = _8737 * _8398;
                                }
                                else
                                {
                                    _8589 = 0.0f;
                                    _8591 = 0.0f;
                                    _8593 = 0.0f;
                                }
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder = _6536;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_1 = _6541;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_2 = _6546;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_3 = _8589 * _1065;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_4 = _8593 * _1065;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_5 = _6566;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_6 = _8591 * _1065;
                            }
                            else
                            {
                                float _7035 = 1.0f - _1040;
                                float _7036 = 1.0f - _1045;
                                float _7037 = 1.0f - _1048;
                                float _7041 = clamp(dot(float3(_1000, _1004, _1008), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                                float _7042 = clamp(_7041, 0.0f, 1.0f);
                                float _7485;
                                if (_64_m0[76u].w > 0.5f)
                                {
                                    float _7448 = _1739 + _69_m0[0u].x;
                                    float _7449 = _1740 + _69_m0[0u].y;
                                    float _7450 = _1741 + _69_m0[0u].z;
                                    float _7454 = rsqrt(dot(float3(_7448, _7449, _7450), float3(_7448, _7449, _7450)));
                                    float _7461 = clamp(dot(float3(_69_m0[0u].xyz), float3(_7454 * _7448, _7454 * _7449, _7454 * _7450)), 0.0f, 1.0f);
                                    float _7471 = ((((_7461 * _7461) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                    _7485 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _7471) + 1.0f) * ((_7471 * exp2(log2(1.0f - _7041) * 5.0f)) + 1.0f);
                                }
                                else
                                {
                                    _7485 = 1.0f;
                                }
                                float _8213;
                                float _8215;
                                float _8217;
                                if (_7041 > 0.0f)
                                {
                                    float _8193 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                    float _8194 = _1739 + _69_m0[0u].x;
                                    float _8195 = _1740 + _69_m0[0u].y;
                                    float _8196 = _1741 + _69_m0[0u].z;
                                    float _8200 = rsqrt(dot(float3(_8194, _8195, _8196), float3(_8194, _8195, _8196)));
                                    float _8201 = _8200 * _8194;
                                    float _8202 = _8200 * _8195;
                                    float _8203 = _8200 * _8196;
                                    float _8207 = clamp(dot(float3(_1012, _1015, _1018), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                                    float _8211 = clamp(dot(float3(_1012, _1015, _1018), float3(_8201, _8202, _8203)), 0.0f, 1.0f);
                                    float _8442;
                                    float _8443;
                                    float _8444;
                                    if (_8207 > 0.0f)
                                    {
                                        float _8409 = _8193 * _8193;
                                        float _8410 = _8409 * _8409;
                                        float _8414 = (((_8211 * _8410) - _8211) * _8211) + 1.0f;
                                        float _8418 = _8409 * 0.5f;
                                        float _8419 = 1.0f - _8418;
                                        float _8426 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_8201, _8202, _8203)), 0.0f, 1.0f);
                                        float _8427 = _8426 * _8426;
                                        float _8429 = (_8427 * _8427) * _8426;
                                        float _8438 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _8419) + _8418) * ((_8207 * _8419) + _8418))) * (_8410 / ((_8414 * _8414) * 3.1415927410125732421875f)), _1719) * _4688;
                                        _8442 = _8438 * ((_8429 * _7035) + _1040);
                                        _8443 = _8438 * ((_8429 * _7036) + _1045);
                                        _8444 = _8438 * ((_8429 * _7037) + _1048);
                                    }
                                    else
                                    {
                                        _8442 = 0.0f;
                                        _8443 = 0.0f;
                                        _8444 = 0.0f;
                                    }
                                    float _8445 = min(_8442, 100000.0f);
                                    float _8446 = min(_8443, 100000.0f);
                                    float _8447 = min(_8444, 100000.0f);
                                    float _8595;
                                    float _8597;
                                    float _8599;
                                    if (_4692)
                                    {
                                        _8595 = _8445;
                                        _8597 = _8446;
                                        _8599 = _8447;
                                    }
                                    else
                                    {
                                        float _8601 = 1.0f - _7041;
                                        float _8602 = _8601 * _8601;
                                        float _8604 = 1.0f - (_8602 * _8602);
                                        _8595 = _8445 * _8604;
                                        _8597 = _8446 * _8604;
                                        _8599 = _8447 * _8604;
                                    }
                                    _8213 = _8595 * _7041;
                                    _8215 = _8597 * _7041;
                                    _8217 = _8599 * _7041;
                                }
                                else
                                {
                                    _8213 = 0.0f;
                                    _8215 = 0.0f;
                                    _8217 = 0.0f;
                                }
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder = (((_1025 * 0.3183098733425140380859375f) * _7035) * _7042) * _7485;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_1 = (((_1030 * 0.3183098733425140380859375f) * _7036) * _7042) * _7485;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_2 = (((_1035 * 0.3183098733425140380859375f) * _7037) * _7042) * _7485;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_3 = _8213 * _1065;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_4 = _8217 * _1065;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_5 = 0.0f;
                                frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_6 = _8215 * _1065;
                            }
                            frontier_phi_309_304_ladder_306_ladder_310_ladder = frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_1 = frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_1;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_2 = frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_2;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_3 = frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_3;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_4 = frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_4;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_5 = frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_5;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_6 = frontier_phi_309_304_ladder_306_ladder_310_ladder_316_ladder_6;
                        }
                        else
                        {
                            float _6703 = 1.0f - _1051;
                            float _6704 = (-0.0f) - _962;
                            float _6705 = (-0.0f) - _956;
                            float _6706 = (-0.0f) - _950;
                            float _6714 = dot(float3(_6704, _6705, _6706), float3(_69_m0[0u].xyz));
                            float _6720 = dot(float3(_6704, _6705, _6706), float3(_1739, _1740, _1741));
                            float _6726 = _69_m0[0u].x - (_6714 * _6704);
                            float _6727 = _69_m0[0u].y - (_6714 * _6705);
                            float _6728 = _69_m0[0u].z - (_6714 * _6706);
                            float _6732 = _1739 - (_6720 * _6704);
                            float _6733 = _1740 - (_6720 * _6705);
                            float _6734 = _1741 - (_6720 * _6706);
                            float _6747 = rsqrt((dot(float3(_6732, _6733, _6734), float3(_6732, _6733, _6734)) * dot(float3(_6726, _6727, _6728), float3(_6726, _6727, _6728))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_6726, _6727, _6728), float3(_6732, _6733, _6734));
                            float _6751 = sqrt(clamp((_6747 * 0.5f) + 0.5f, 0.0f, 1.0f));
                            float _6757 = cos(abs(asin(_6720) - asin(_6714)) * 0.5f);
                            float _6762 = max(_6703 * 2.0f, 0.00999999977648258209228515625f);
                            float _6764 = (-0.0f) - _1171;
                            float _6766 = sin(_6764);
                            float _6777 = _6720 + _6714;
                            float _6778 = _6777 - ((_6766 * 2.0f) * (((cos(_6764) * _6751) * sqrt(1.0f - (_6720 * _6720))) + (_6766 * _6720)));
                            float _6780 = (_6751 * 1.41421353816986083984375f) * max((1.0f - _1165) * _6703, 0.00999999977648258209228515625f);
                            float _6802 = _6777 - (_1171 * 1.5f);
                            float _6828 = exp2(log2(1.0f - (_6757 * 0.5f)) * 5.0f) * 0.95347940921783447265625f;
                            float _6830 = 0.95347940921783447265625f - _6828;
                            float _6835 = clamp((1.0f - max(_1169, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                            float _6841 = abs(sqrt(1.0f - (_6714 * _6714))) * clamp(dot(float3(_1000, _1004, _1008), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                            float _6842 = (((_6751 * 0.25f) * (exp2((((_6778 * _6778) * (-0.5f)) / (_6780 * _6780)) * 1.44269502162933349609375f) / (_6780 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(dot(float3(_1739, _1740, _1741), float3(_69_m0[0u].xyz)), 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f)) * _6841;
                            float _6844 = (_6835 * (exp2((((_6802 * _6802) * (-0.5f)) / (_6762 * _6762)) * 1.44269502162933349609375f) / (_6762 * 2.5066282749176025390625f))) * exp2(_6835 * ((_6747 * 24.5258159637451171875f) + (-24.208423614501953125f)));
                            float _6845 = _6841 * ((_6830 * _6830) * (_6828 + 0.0465205647051334381103515625f));
                            frontier_phi_309_304_ladder_306_ladder_310_ladder = 0.0f;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_1 = 0.0f;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_2 = 0.0f;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_3 = ((_6845 * exp2(((_1155 * (-3.2000000476837158203125f)) / _6757) * 1.44269502162933349609375f)) * _6844) + _6842;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_4 = ((_6845 * exp2(((_1159 * (-3.2000000476837158203125f)) / _6757) * 1.44269502162933349609375f)) * _6844) + _6842;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_5 = 0.0f;
                            frontier_phi_309_304_ladder_306_ladder_310_ladder_6 = ((_6845 * exp2(((_1157 * (-3.2000000476837158203125f)) / _6757) * 1.44269502162933349609375f)) * _6844) + _6842;
                        }
                        frontier_phi_309_304_ladder_306_ladder = frontier_phi_309_304_ladder_306_ladder_310_ladder;
                        frontier_phi_309_304_ladder_306_ladder_1 = frontier_phi_309_304_ladder_306_ladder_310_ladder_1;
                        frontier_phi_309_304_ladder_306_ladder_2 = frontier_phi_309_304_ladder_306_ladder_310_ladder_2;
                        frontier_phi_309_304_ladder_306_ladder_3 = frontier_phi_309_304_ladder_306_ladder_310_ladder_3;
                        frontier_phi_309_304_ladder_306_ladder_4 = frontier_phi_309_304_ladder_306_ladder_310_ladder_4;
                        frontier_phi_309_304_ladder_306_ladder_5 = frontier_phi_309_304_ladder_306_ladder_310_ladder_5;
                        frontier_phi_309_304_ladder_306_ladder_6 = frontier_phi_309_304_ladder_306_ladder_310_ladder_6;
                    }
                    else
                    {
                        float _6572 = clamp(dot(float3(_1000, _1004, _1008), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                        float _6549;
                        float _6555;
                        float _6561;
                        if (_6572 > 0.0f)
                        {
                            float _6857 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                            float _6858 = _1739 + _69_m0[0u].x;
                            float _6859 = _1740 + _69_m0[0u].y;
                            float _6860 = _1741 + _69_m0[0u].z;
                            float _6864 = rsqrt(dot(float3(_6858, _6859, _6860), float3(_6858, _6859, _6860)));
                            float _6865 = _6864 * _6858;
                            float _6866 = _6864 * _6859;
                            float _6867 = _6864 * _6860;
                            float _6871 = clamp(dot(float3(_1012, _1015, _1018), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                            float _6875 = clamp(dot(float3(_1012, _1015, _1018), float3(_6865, _6866, _6867)), 0.0f, 1.0f);
                            float _7092;
                            float _7093;
                            float _7094;
                            if (_6871 > 0.0f)
                            {
                                float _7056 = _6857 * _6857;
                                float _7057 = _7056 * _7056;
                                float _7061 = (((_6875 * _7057) - _6875) * _6875) + 1.0f;
                                float _7065 = _7056 * 0.5f;
                                float _7066 = 1.0f - _7065;
                                float _7073 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_6865, _6866, _6867)), 0.0f, 1.0f);
                                float _7074 = _7073 * _7073;
                                float _7076 = (_7074 * _7074) * _7073;
                                float _7088 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _7066) + _7065) * ((_6871 * _7066) + _7065))) * (_7057 / ((_7061 * _7061) * 3.1415927410125732421875f)), _1719) * _4688;
                                _7092 = _7088 * ((_7076 * (1.0f - _1040)) + _1040);
                                _7093 = _7088 * ((_7076 * (1.0f - _1045)) + _1045);
                                _7094 = _7088 * ((_7076 * (1.0f - _1048)) + _1048);
                            }
                            else
                            {
                                _7092 = 0.0f;
                                _7093 = 0.0f;
                                _7094 = 0.0f;
                            }
                            float _7095 = min(_7092, 100000.0f);
                            float _7097 = min(_7093, 100000.0f);
                            float _7098 = min(_7094, 100000.0f);
                            float _7496;
                            float _7498;
                            float _7500;
                            if (_4692)
                            {
                                _7496 = _7095;
                                _7498 = _7097;
                                _7500 = _7098;
                            }
                            else
                            {
                                float _7502 = 1.0f - _6572;
                                float _7503 = _7502 * _7502;
                                float _7505 = 1.0f - (_7503 * _7503);
                                _7496 = _7095 * _7505;
                                _7498 = _7097 * _7505;
                                _7500 = _7098 * _7505;
                            }
                            _6549 = _7496 * _6572;
                            _6555 = _7498 * _6572;
                            _6561 = _7500 * _6572;
                        }
                        else
                        {
                            _6549 = 0.0f;
                            _6555 = 0.0f;
                            _6561 = 0.0f;
                        }
                        float _6883 = clamp(_6572, 0.0f, 1.0f);
                        float _7136;
                        if (_64_m0[76u].w > 0.5f)
                        {
                            float _7099 = _1739 + _69_m0[0u].x;
                            float _7100 = _1740 + _69_m0[0u].y;
                            float _7101 = _1741 + _69_m0[0u].z;
                            float _7105 = rsqrt(dot(float3(_7099, _7100, _7101), float3(_7099, _7100, _7101)));
                            float _7112 = clamp(dot(float3(_69_m0[0u].xyz), float3(_7105 * _7099, _7105 * _7100, _7105 * _7101)), 0.0f, 1.0f);
                            float _7122 = ((((_7112 * _7112) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                            _7136 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _7122) + 1.0f) * ((_7122 * exp2(log2(1.0f - _6572) * 5.0f)) + 1.0f);
                        }
                        else
                        {
                            _7136 = 1.0f;
                        }
                        float _7137 = 1.0f - _1145;
                        frontier_phi_309_304_ladder_306_ladder = ((((_1025 * 0.3183098733425140380859375f) * (1.0f - _1040)) * _7137) * _6883) * _7136;
                        frontier_phi_309_304_ladder_306_ladder_1 = ((((_1030 * 0.3183098733425140380859375f) * (1.0f - _1045)) * _7137) * _6883) * _7136;
                        frontier_phi_309_304_ladder_306_ladder_2 = ((((_1035 * 0.3183098733425140380859375f) * (1.0f - _1048)) * _7137) * _6883) * _7136;
                        frontier_phi_309_304_ladder_306_ladder_3 = _6549;
                        frontier_phi_309_304_ladder_306_ladder_4 = _6561;
                        frontier_phi_309_304_ladder_306_ladder_5 = 0.0f;
                        frontier_phi_309_304_ladder_306_ladder_6 = _6555;
                    }
                    frontier_phi_309_304_ladder = frontier_phi_309_304_ladder_306_ladder;
                    frontier_phi_309_304_ladder_1 = frontier_phi_309_304_ladder_306_ladder_1;
                    frontier_phi_309_304_ladder_2 = frontier_phi_309_304_ladder_306_ladder_2;
                    frontier_phi_309_304_ladder_3 = frontier_phi_309_304_ladder_306_ladder_3;
                    frontier_phi_309_304_ladder_4 = frontier_phi_309_304_ladder_306_ladder_4;
                    frontier_phi_309_304_ladder_5 = frontier_phi_309_304_ladder_306_ladder_5;
                    frontier_phi_309_304_ladder_6 = frontier_phi_309_304_ladder_306_ladder_6;
                }
                else
                {
                    float _6407 = 1.0f - _1040;
                    float _6408 = 1.0f - _1045;
                    float _6409 = 1.0f - _1048;
                    float _6413 = clamp(dot(float3(_1000, _1004, _1008), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                    float _6414 = clamp(_6413, 0.0f, 1.0f);
                    float _6611;
                    if (_64_m0[76u].w > 0.5f)
                    {
                        float _6574 = _1739 + _69_m0[0u].x;
                        float _6575 = _1740 + _69_m0[0u].y;
                        float _6576 = _1741 + _69_m0[0u].z;
                        float _6580 = rsqrt(dot(float3(_6574, _6575, _6576), float3(_6574, _6575, _6576)));
                        float _6587 = clamp(dot(float3(_69_m0[0u].xyz), float3(_6580 * _6574, _6580 * _6575, _6580 * _6576)), 0.0f, 1.0f);
                        float _6597 = ((((_6587 * _6587) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                        _6611 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _6597) + 1.0f) * ((_6597 * exp2(log2(1.0f - _6413) * 5.0f)) + 1.0f);
                    }
                    else
                    {
                        _6611 = 1.0f;
                    }
                    bool _6621 = _6413 > 0.0f;
                    float _6913;
                    float _6915;
                    float _6917;
                    if (_6621)
                    {
                        float _6893 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                        float _6894 = _1739 + _69_m0[0u].x;
                        float _6895 = _1740 + _69_m0[0u].y;
                        float _6896 = _1741 + _69_m0[0u].z;
                        float _6900 = rsqrt(dot(float3(_6894, _6895, _6896), float3(_6894, _6895, _6896)));
                        float _6901 = _6900 * _6894;
                        float _6902 = _6900 * _6895;
                        float _6903 = _6900 * _6896;
                        float _6907 = clamp(dot(float3(_1012, _1015, _1018), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                        float _6911 = clamp(dot(float3(_1012, _1015, _1018), float3(_6901, _6902, _6903)), 0.0f, 1.0f);
                        float _7192;
                        float _7193;
                        float _7194;
                        if (_6907 > 0.0f)
                        {
                            float _7159 = _6893 * _6893;
                            float _7160 = _7159 * _7159;
                            float _7164 = (((_6911 * _7160) - _6911) * _6911) + 1.0f;
                            float _7168 = _7159 * 0.5f;
                            float _7169 = 1.0f - _7168;
                            float _7176 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_6901, _6902, _6903)), 0.0f, 1.0f);
                            float _7177 = _7176 * _7176;
                            float _7179 = (_7177 * _7177) * _7176;
                            float _7188 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _7169) + _7168) * ((_6907 * _7169) + _7168))) * (_7160 / ((_7164 * _7164) * 3.1415927410125732421875f)), _1719) * _4688;
                            _7192 = _7188 * ((_7179 * _6407) + _1040);
                            _7193 = _7188 * ((_7179 * _6408) + _1045);
                            _7194 = _7188 * ((_7179 * _6409) + _1048);
                        }
                        else
                        {
                            _7192 = 0.0f;
                            _7193 = 0.0f;
                            _7194 = 0.0f;
                        }
                        float _7195 = min(_7192, 100000.0f);
                        float _7196 = min(_7193, 100000.0f);
                        float _7197 = min(_7194, 100000.0f);
                        float _7506;
                        float _7508;
                        float _7510;
                        if (_4692)
                        {
                            _7506 = _7195;
                            _7508 = _7196;
                            _7510 = _7197;
                        }
                        else
                        {
                            float _7512 = 1.0f - _6413;
                            float _7513 = _7512 * _7512;
                            float _7515 = 1.0f - (_7513 * _7513);
                            _7506 = _7195 * _7515;
                            _7508 = _7196 * _7515;
                            _7510 = _7197 * _7515;
                        }
                        _6913 = _7506 * _6413;
                        _6915 = _7508 * _6413;
                        _6917 = _7510 * _6413;
                    }
                    else
                    {
                        _6913 = 0.0f;
                        _6915 = 0.0f;
                        _6917 = 0.0f;
                    }
                    float _7223;
                    float _7225;
                    float _7227;
                    if (_6621)
                    {
                        float _7203 = max(exp2(log2(clamp(1.0f - _1088, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                        float _7204 = _1739 + _69_m0[0u].x;
                        float _7205 = _1740 + _69_m0[0u].y;
                        float _7206 = _1741 + _69_m0[0u].z;
                        float _7210 = rsqrt(dot(float3(_7204, _7205, _7206), float3(_7204, _7205, _7206)));
                        float _7211 = _7210 * _7204;
                        float _7212 = _7210 * _7205;
                        float _7213 = _7210 * _7206;
                        float _7217 = clamp(dot(float3(_1012, _1015, _1018), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                        float _7221 = clamp(dot(float3(_1012, _1015, _1018), float3(_7211, _7212, _7213)), 0.0f, 1.0f);
                        float _7558;
                        float _7559;
                        float _7560;
                        if (_7217 > 0.0f)
                        {
                            float _7525 = _7203 * _7203;
                            float _7526 = _7525 * _7525;
                            float _7530 = (((_7221 * _7526) - _7221) * _7221) + 1.0f;
                            float _7534 = _7525 * 0.5f;
                            float _7535 = 1.0f - _7534;
                            float _7542 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_7211, _7212, _7213)), 0.0f, 1.0f);
                            float _7543 = _7542 * _7542;
                            float _7545 = (_7543 * _7543) * _7542;
                            float _7554 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _7535) + _7534) * ((_7217 * _7535) + _7534))) * (_7526 / ((_7530 * _7530) * 3.1415927410125732421875f)), _1719) * _4688;
                            _7558 = _7554 * ((_7545 * _6407) + _1040);
                            _7559 = _7554 * ((_7545 * _6408) + _1045);
                            _7560 = _7554 * ((_7545 * _6409) + _1048);
                        }
                        else
                        {
                            _7558 = 0.0f;
                            _7559 = 0.0f;
                            _7560 = 0.0f;
                        }
                        float _7561 = min(_7558, 100000.0f);
                        float _7562 = min(_7559, 100000.0f);
                        float _7563 = min(_7560, 100000.0f);
                        float _8219;
                        float _8221;
                        float _8223;
                        if (_4692)
                        {
                            _8219 = _7561;
                            _8221 = _7562;
                            _8223 = _7563;
                        }
                        else
                        {
                            float _8225 = 1.0f - _6413;
                            float _8226 = _8225 * _8225;
                            float _8228 = 1.0f - (_8226 * _8226);
                            _8219 = _7561 * _8228;
                            _8221 = _7562 * _8228;
                            _8223 = _7563 * _8228;
                        }
                        _7223 = _8219 * _6413;
                        _7225 = _8221 * _6413;
                        _7227 = _8223 * _6413;
                    }
                    else
                    {
                        _7223 = 0.0f;
                        _7225 = 0.0f;
                        _7227 = 0.0f;
                    }
                    frontier_phi_309_304_ladder = (((_1025 * 0.3183098733425140380859375f) * _6407) * _6414) * _6611;
                    frontier_phi_309_304_ladder_1 = (((_1030 * 0.3183098733425140380859375f) * _6408) * _6414) * _6611;
                    frontier_phi_309_304_ladder_2 = (((_1035 * 0.3183098733425140380859375f) * _6409) * _6414) * _6611;
                    frontier_phi_309_304_ladder_3 = ((_7223 - _6913) * _1090) + _6913;
                    frontier_phi_309_304_ladder_4 = ((_7227 - _6917) * _1090) + _6917;
                    frontier_phi_309_304_ladder_5 = 0.0f;
                    frontier_phi_309_304_ladder_6 = ((_7225 - _6915) * _1090) + _6915;
                }
                _6532 = frontier_phi_309_304_ladder;
                _6537 = frontier_phi_309_304_ladder_1;
                _6542 = frontier_phi_309_304_ladder_2;
                _6547 = frontier_phi_309_304_ladder_3;
                _6553 = frontier_phi_309_304_ladder_6;
                _6559 = frontier_phi_309_304_ladder_4;
                _6565 = frontier_phi_309_304_ladder_5;
            }
            else
            {
                float _6403 = clamp(dot(float3(_1000, _1004, _1008), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                float frontier_phi_309_305_ladder;
                float frontier_phi_309_305_ladder_1;
                float frontier_phi_309_305_ladder_2;
                float frontier_phi_309_305_ladder_3;
                float frontier_phi_309_305_ladder_4;
                float frontier_phi_309_305_ladder_5;
                float frontier_phi_309_305_ladder_6;
                if (_6403 > 0.0f)
                {
                    float _6421 = _1739 + _69_m0[0u].x;
                    float _6422 = _1740 + _69_m0[0u].y;
                    float _6423 = _1741 + _69_m0[0u].z;
                    float _6427 = rsqrt(dot(float3(_6421, _6422, _6423), float3(_6421, _6422, _6423)));
                    float _6428 = _6427 * _6421;
                    float _6429 = _6427 * _6422;
                    float _6430 = _6427 * _6423;
                    float _6435 = max(clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f), 0.001000000047497451305389404296875f);
                    float _6439 = clamp(dot(float3(_1012, _1015, _1018), float3(_6428, _6429, _6430)), 0.0f, 1.0f);
                    float _6444 = max(1.0f - _1051, 0.04500000178813934326171875f);
                    float _6446 = _6444 * _6444;
                    float _6447 = 1.0f / _6446;
                    float _6462 = (((_6447 + 2.0f) * 0.15915493667125701904296875f) * exp2((_6447 * 0.5f) * log2(1.0f - (_6439 * _6439)))) * (0.25f / ((_6435 + _6403) - (_6435 * _6403)));
                    float _6466 = _6446 * _6446;
                    float _6470 = (((_6466 * _6439) - _6439) * _6439) + 1.0f;
                    float _6474 = _6446 * 0.5f;
                    float _6475 = 1.0f - _6474;
                    float _6482 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_6428, _6429, _6430)), 0.0f, 1.0f);
                    float _6483 = _6482 * _6482;
                    float _6485 = (_6483 * _6483) * _6482;
                    float _6497 = min((0.25f / (((_6475 * _6435) + _6474) * ((_6475 * _6403) + _6474))) * (_6466 / ((_6470 * _6470) * 3.1415927410125732421875f)), _1719) * _4688;
                    float _6498 = _6497 * ((_6485 * (1.0f - _1040)) + _1040);
                    float _6499 = _6497 * ((_6485 * (1.0f - _1045)) + _1045);
                    float _6500 = _6497 * ((_6485 * (1.0f - _1048)) + _1048);
                    float _6504 = (1.0f - _1096) * _1025;
                    float _6505 = (1.0f - _1097) * _1030;
                    float _6506 = (1.0f - _1098) * _1035;
                    float _6528 = _6403 * 0.3183098733425140380859375f;
                    frontier_phi_309_305_ladder = _6528 * (((_1025 - _6504) * _1099) + _6504);
                    frontier_phi_309_305_ladder_1 = _6528 * (((_1030 - _6505) * _1099) + _6505);
                    frontier_phi_309_305_ladder_2 = _6528 * (((_1035 - _6506) * _1099) + _6506);
                    frontier_phi_309_305_ladder_3 = ((((_6462 * _1096) - _6498) * _1099) + _6498) * _6403;
                    frontier_phi_309_305_ladder_4 = ((((_6462 * _1098) - _6500) * _1099) + _6500) * _6403;
                    frontier_phi_309_305_ladder_5 = 0.0f;
                    frontier_phi_309_305_ladder_6 = ((((_6462 * _1097) - _6499) * _1099) + _6499) * _6403;
                }
                else
                {
                    frontier_phi_309_305_ladder = 0.0f;
                    frontier_phi_309_305_ladder_1 = 0.0f;
                    frontier_phi_309_305_ladder_2 = 0.0f;
                    frontier_phi_309_305_ladder_3 = 0.0f;
                    frontier_phi_309_305_ladder_4 = 0.0f;
                    frontier_phi_309_305_ladder_5 = 0.0f;
                    frontier_phi_309_305_ladder_6 = 0.0f;
                }
                _6532 = frontier_phi_309_305_ladder;
                _6537 = frontier_phi_309_305_ladder_1;
                _6542 = frontier_phi_309_305_ladder_2;
                _6547 = frontier_phi_309_305_ladder_3;
                _6553 = frontier_phi_309_305_ladder_6;
                _6559 = frontier_phi_309_305_ladder_4;
                _6565 = frontier_phi_309_305_ladder_5;
            }
            float _6994;
            float _6999;
            float _7004;
            if (_5572)
            {
                float frontier_phi_324_314_ladder;
                float frontier_phi_324_314_ladder_1;
                float frontier_phi_324_314_ladder_2;
                if ((_1174 & 67108864u) == 0u)
                {
                    float frontier_phi_324_314_ladder_322_ladder;
                    float frontier_phi_324_314_ladder_322_ladder_1;
                    float frontier_phi_324_314_ladder_322_ladder_2;
                    if ((_1174 & 50331648u) == 0u)
                    {
                        float frontier_phi_324_314_ladder_322_ladder_335_ladder;
                        float frontier_phi_324_314_ladder_322_ladder_335_ladder_1;
                        float frontier_phi_324_314_ladder_322_ladder_335_ladder_2;
                        if ((_1174 & 536870912u) == 0u)
                        {
                            float frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder;
                            float frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder_1;
                            float frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder_2;
                            if (int(_1173) < int(0u))
                            {
                                float _8236 = dot(float3(-1.27600002288818359375f, 2.4965000152587890625f, 0.296999990940093994140625f), float3(_64_m0[47u].y * _64_m0[47u].y, (-0.0f) - _64_m0[47u].y, 1.0f));
                                float _8256 = clamp(clamp((dot(float3(_64_m0[47u].xyz), float3(_1739, _1740, _1741)) * 0.5f) + 0.5f, 0.0f, 1.0f), 0.0f, 1.0f);
                                float _8270 = exp2(log2((_8256 * _8256) * (3.0f - (_8256 * 2.0f))) * 3.0f) * exp2(log2(1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_1000, _1004, _1008)), 0.0f, 1.0f)) * 1.5f);
                                float _8274 = _1084 * 0.5f;
                                frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder = ((((((_1079 * _1035) * _1056) * _8274) * (1.0f - (_1102 * 0.699999988079071044921875f))) * _8236) * _8270) * _64_m0[166u].y;
                                frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder_1 = (((((_1079 * _1030) * _1056) * _8274) * _8236) * _8270) * _64_m0[166u].y;
                                frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder_2 = ((((((_1079 * _1025) * _1056) * _8274) * (1.0f - (_1102 * 0.199999988079071044921875f))) * _8236) * _8270) * _64_m0[166u].y;
                            }
                            else
                            {
                                float _8302 = clamp((dot(float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), float3(_69_m0[0u].xyz)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_1079 * 0.3183098733425140380859375f);
                                frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder = _8302 * _1076;
                                frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder_1 = _8302 * _1073;
                                frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder_2 = _8302 * _1070;
                            }
                            frontier_phi_324_314_ladder_322_ladder_335_ladder = frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder;
                            frontier_phi_324_314_ladder_322_ladder_335_ladder_1 = frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder_1;
                            frontier_phi_324_314_ladder_322_ladder_335_ladder_2 = frontier_phi_324_314_ladder_322_ladder_335_ladder_349_ladder_2;
                        }
                        else
                        {
                            float _7575 = clamp((dot(float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), float3(_69_m0[0u].xyz)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_1079 * 0.3183098733425140380859375f);
                            frontier_phi_324_314_ladder_322_ladder_335_ladder = _7575 * _1076;
                            frontier_phi_324_314_ladder_322_ladder_335_ladder_1 = _7575 * _1073;
                            frontier_phi_324_314_ladder_322_ladder_335_ladder_2 = _7575 * _1070;
                        }
                        frontier_phi_324_314_ladder_322_ladder = frontier_phi_324_314_ladder_322_ladder_335_ladder;
                        frontier_phi_324_314_ladder_322_ladder_1 = frontier_phi_324_314_ladder_322_ladder_335_ladder_1;
                        frontier_phi_324_314_ladder_322_ladder_2 = frontier_phi_324_314_ladder_322_ladder_335_ladder_2;
                    }
                    else
                    {
                        float _7240 = 1.0f - _1051;
                        float _7241 = (-0.0f) - _962;
                        float _7242 = (-0.0f) - _956;
                        float _7243 = (-0.0f) - _950;
                        float _7246 = 1.0f - clamp(_1161 * 66.6666717529296875f, 0.0f, 1.0f);
                        float _7250 = dot(float3(_7241, _7242, _7243), float3(_69_m0[0u].xyz));
                        float _7256 = dot(float3(_7241, _7242, _7243), float3(_1739, _1740, _1741));
                        float _7262 = _69_m0[0u].x - (_7250 * _7241);
                        float _7263 = _69_m0[0u].y - (_7250 * _7242);
                        float _7264 = _69_m0[0u].z - (_7250 * _7243);
                        float _7268 = _1739 - (_7256 * _7241);
                        float _7269 = _1740 - (_7256 * _7242);
                        float _7270 = _1741 - (_7256 * _7243);
                        float _7283 = rsqrt((dot(float3(_7268, _7269, _7270), float3(_7268, _7269, _7270)) * dot(float3(_7262, _7263, _7264), float3(_7262, _7263, _7264))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_7262, _7263, _7264), float3(_7268, _7269, _7270));
                        float _7285 = (_7283 * 0.5f) + 0.5f;
                        float _7288 = asin(_7256);
                        float _7289 = asin(_7250);
                        float _7293 = cos(abs(_7288 - _7289) * 0.5f);
                        float _7297 = 1.0f / ((1.190000057220458984375f / _7293) + (_7293 * 0.36000001430511474609375f));
                        float _7298 = _7256 * 0.645161330699920654296875f;
                        float _7301 = sqrt(1.0f - (_7298 * _7298));
                        float _7303 = max(_7240 * 0.5f, 0.00999999977648258209228515625f);
                        float _7306 = _7256 + (_7250 - (_1171 * 0.5f));
                        float _7314 = exp2((((_7306 * _7306) * (-0.5f)) / (_7303 * _7303)) * 1.44269502162933349609375f) / (_7303 * 2.5066282749176025390625f);
                        float _7323 = clamp((-0.0f) - dot(float3(_1739, _1740, _1741), float3(_69_m0[0u].xyz)), 0.0f, 1.0f);
                        float _7324 = _7323 * _7323;
                        float _7327 = (_7324 * _7324) * (_7323 * _7246);
                        float _7334 = (cos(asin((_7297 * sqrt(clamp(_7285, 0.0f, 1.0f))) * ((_7297 * (0.60000002384185791015625f - (_7283 * 0.800000011920928955078125f))) + 1.0f)) * 2.0f) + 1.0f) * (-2.8853900432586669921875f);
                        float _7338 = exp2(_7334 * (_1155 / _7301));
                        float _7339 = exp2(_7334 * (_1157 / _7301));
                        float _7340 = exp2(_7334 * (_1159 / _7301));
                        float _7355 = 0.95347940921783447265625f - (exp2(log2(1.0f - _7293) * 5.0f) * 0.95347940921783447265625f);
                        float _7356 = _7355 * _7355;
                        float _7358 = (_7293 * 0.5f) + 0.5f;
                        float _7371 = abs(sqrt(1.0f - (_7250 * _7250))) * _15[(_1153 + 513u) + 0u].SampleLevel(_72, float2(_7358, _7285), 0.0f).x;
                        float _7372 = _7371 * ((_7246 * _6268) * _7314);
                        float _7379 = _5569 + _1161;
                        float _7380 = _1051 * 0.75f;
                        float _7384 = (((_1163 * _1163) * 9000.0f) * _1163) * _7379;
                        float _8303;
                        float _8304;
                        float _8305;
                        if ((_1174 & 33554432u) == 0u)
                        {
                            float _7576 = max(_7384, _7380);
                            float _7582 = _7289 + _7288;
                            float _7583 = _7582 * 0.5f;
                            float _7585 = _1155 * 0.5f;
                            float _7586 = _1157 * 0.5f;
                            float _7587 = _1159 * 0.5f;
                            float _7588 = max(_7240, 0.0500000007450580596923828125f);
                            float _7591 = (cos(_7583) * 0.5f) + 0.5f;
                            uint _7594 = (_1153 + 521u) + 0u;
                            float4 _7599 = _19[_7594].SampleLevel(_72, float3(_7591, _7588, _7585), 0.0f);
                            float _7601 = _7599.x;
                            float4 _7602 = _19[_7594].SampleLevel(_72, float3(_7591, _7588, _7586), 0.0f);
                            float _7604 = _7602.x;
                            float4 _7605 = _19[_7594].SampleLevel(_72, float3(_7591, _7588, _7587), 0.0f);
                            float _7607 = _7605.x;
                            float4 _7608 = _19[_7594].SampleLevel(_72, float3(_7358, _7588, _7585), 0.0f);
                            float4 _7612 = _19[_7594].SampleLevel(_72, float3(_7358, _7588, _7586), 0.0f);
                            float4 _7616 = _19[_7594].SampleLevel(_72, float3(_7358, _7588, _7587), 0.0f);
                            uint _7622 = (_1153 + 545u) + 0u;
                            float _7637 = (_7604 + _7601) + _7607;
                            float _7644 = dot(float3(max((1.0f - _1165) * _7240, 0.00999999977648258209228515625f), _7303, max(_7240 * 2.0f, 0.00999999977648258209228515625f)), float3(_7601 / _7637, _7604 / _7637, _7607 / _7637)) * _7576;
                            float _7655 = (_7293 * _7293) * 3.1415927410125732421875f;
                            float _7659 = _7644 * 0.5f;
                            float _7660 = _7659 + _7608.z;
                            float _7661 = _7659 + _7612.z;
                            float _7662 = _7659 + _7616.z;
                            float _7665 = (_7582 * (-0.25f)) * _7583;
                            float _7687 = ((_7608.y * 2.0f) * (exp2((_7665 / (_7660 * _7660)) * 1.44269502162933349609375f) / (_7660 * 2.5066282749176025390625f))) / _7655;
                            float _7688 = ((_7612.y * 2.0f) * (exp2((_7665 / (_7661 * _7661)) * 1.44269502162933349609375f) / (_7661 * 2.5066282749176025390625f))) / _7655;
                            float _7689 = ((_7616.y * 2.0f) * (exp2((_7665 / (_7662 * _7662)) * 1.44269502162933349609375f) / (_7662 * 2.5066282749176025390625f))) / _7655;
                            float _7690 = _7582 - _1171;
                            float _7693 = max(max(_7644, _7644), _7644) + _7303;
                            float _7702 = exp2((((_7690 * _7690) * (-0.125f)) / (_7693 * _7693)) * 1.44269502162933349609375f) / (_7693 * 2.5066282749176025390625f);
                            float _7706 = _6268 * 0.3499999940395355224609375f;
                            _8303 = (((exp2(log2(_7601) * _7576) * 0.4899999797344207763671875f) * ((_7702 * _15[_7622].SampleLevel(_72, float2(_7358, _7585), 0.0f).y) + (_7687 * 2.19911479949951171875f))) + (_7687 * _7706)) * _1167;
                            _8304 = (((exp2(log2(_7604) * _7576) * 0.4899999797344207763671875f) * ((_7702 * _15[_7622].SampleLevel(_72, float2(_7358, _7586), 0.0f).y) + (_7688 * 2.19911479949951171875f))) + (_7688 * _7706)) * _1167;
                            _8305 = (((exp2(log2(_7607) * _7576) * 0.4899999797344207763671875f) * ((_7702 * _15[_7622].SampleLevel(_72, float2(_7358, _7587), 0.0f).y) + (_7689 * 2.19911479949951171875f))) + (_7689 * _7706)) * _1167;
                        }
                        else
                        {
                            float _7735 = 1.0f - clamp((_7379 * 66.6666717529296875f) * max(_1163, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f);
                            float _7736 = _7735 * _7735;
                            float _7738 = (_7736 * _7736) * _7735;
                            float _7739 = _7356 * _7314;
                            float _7747 = (_7380 + 1.25f) + _7384;
                            float _7772 = dot(float3(_1739, _1740, _1741), float3(_7241, _7242, _7243));
                            float _7778 = _1739 - (_7772 * _7241);
                            float _7779 = _1740 - (_7772 * _7242);
                            float _7780 = _1741 - (_7772 * _7243);
                            float _7784 = rsqrt(dot(float3(_7778, _7779, _7780), float3(_7778, _7779, _7780)));
                            float _7792 = (dot(float3(_7778 * _7784, _7779 * _7784, _7780 * _7784), float3(_69_m0[0u].xyz)) + 1.0f) * 0.25f;
                            float _7799 = (_1167 * 0.2228169143199920654296875f) * ((((1.0f - abs(_7250)) - _7792) * 0.3300000131130218505859375f) + _7792);
                            _8303 = ((_7799 * exp2(log2(exp2(((_1155 * (-3.7999999523162841796875f)) / _7293) * 1.44269502162933349609375f)) * _7747)) + ((_7371 * _7338) * _7739)) * _7738;
                            _8304 = ((_7799 * exp2(log2(exp2(((_1157 * (-3.7999999523162841796875f)) / _7293) * 1.44269502162933349609375f)) * _7747)) + ((_7371 * _7339) * _7739)) * _7738;
                            _8305 = ((_7799 * exp2(log2(exp2(((_1159 * (-3.7999999523162841796875f)) / _7293) * 1.44269502162933349609375f)) * _7747)) + ((_7371 * _7340) * _7739)) * _7738;
                        }
                        frontier_phi_324_314_ladder_322_ladder = _8305 + ((_7372 * (((1.0f - _7340) * _7327) + _7340)) * _7356);
                        frontier_phi_324_314_ladder_322_ladder_1 = _8304 + ((_7372 * (((1.0f - _7339) * _7327) + _7339)) * _7356);
                        frontier_phi_324_314_ladder_322_ladder_2 = _8303 + ((_7372 * (((1.0f - _7338) * _7327) + _7338)) * _7356);
                    }
                    frontier_phi_324_314_ladder = frontier_phi_324_314_ladder_322_ladder;
                    frontier_phi_324_314_ladder_1 = frontier_phi_324_314_ladder_322_ladder_1;
                    frontier_phi_324_314_ladder_2 = frontier_phi_324_314_ladder_322_ladder_2;
                }
                else
                {
                    uint _6921 = _1149 + 0u;
                    float _6922 = dot(float3(_69_m0[0u].xyz), float3(_1139, _1141, _1143));
                    float _6937 = dot(float3(_1000, _1004, _1008), float3((_69_m0[0u].y * _1143) - (_69_m0[0u].z * _1141), (_69_m0[0u].z * _1139) - (_69_m0[0u].x * _1143), (_69_m0[0u].x * _1141) - (_69_m0[0u].y * _1139)));
                    float _6946 = float(int(uint(_6937 > 0.0f) - uint(_6937 < 0.0f))) * sqrt(1.0f - (_6922 * _6922));
                    float _6947 = _1135 + (-0.5f);
                    float _6948 = _1137 + (-0.5f);
                    float _6954 = mad(_6948, _6946, _6922 * _6947) + 0.5f;
                    float _6955 = mad(_6948, _6922, (-0.0f) - (_6947 * _6946)) + 0.5f;
                    float _6961 = (1.0f - clamp(dot(float3(_69_m0[0u].xyz), float3(_1000, _1004, _1008)), 0.0f, 1.0f)) * 5.0f;
                    uint _6962 = uint(int(_6961));
                    float4 _6970 = _43[NonUniformResourceIndex(_6921)].SampleLevel(_76, float3(_6954, _6955, float(int(_6962))), 0.0f);
                    float _6972 = _6970.x;
                    float _6981 = ((_43[NonUniformResourceIndex(_6921)].SampleLevel(_76, float3(_6954, _6955, min(float(int(_6962 + 1u)), 5.0f)), 0.0f).x - _6972) * frac(_6961)) + _6972;
                    frontier_phi_324_314_ladder = (((_1035 * 0.3183098733425140380859375f) * _1145) * _1147) * _6981;
                    frontier_phi_324_314_ladder_1 = (((_1030 * 0.3183098733425140380859375f) * _1145) * _1147) * _6981;
                    frontier_phi_324_314_ladder_2 = (((_1025 * 0.3183098733425140380859375f) * _1145) * _1147) * _6981;
                }
                _6994 = frontier_phi_324_314_ladder_2;
                _6999 = frontier_phi_324_314_ladder_1;
                _7004 = frontier_phi_324_314_ladder;
            }
            else
            {
                float _6630 = (_5569 * (-693.14715576171875f)) * log2(max(1.0f - _1094, 1.1754943508222875079687365372222e-38f));
                float _6631 = _6630 * _6630;
                float _6634 = exp2(_6631 * (-225.4210968017578125f));
                float _6643 = exp2(_6631 * (-29.8077487945556640625f));
                float _6655 = exp2(_6631 * (-7.714946269989013671875f));
                float _6664 = exp2(_6631 * (-2.5444357395172119140625f));
                float _6667 = _6664 * 0.007000000216066837310791015625f;
                float _6674 = exp2(_6631 * (-0.72497236728668212890625f));
                float _6694 = max(dot(float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), float3(_69_m0[0u].xyz)) + 0.300000011920928955078125f, 0.0f);
                _6994 = (((((((_6643 * 0.100000001490116119384765625f) + (_6634 * 0.23299999535083770751953125f)) + (_6655 * 0.1180000007152557373046875f)) + (_6664 * 0.112999998033046722412109375f)) + (_6674 * 0.3580000102519989013671875f)) + (exp2(_6631 * (-0.1946956813335418701171875f)) * 0.078000001609325408935546875f)) * _1025) * _6694;
                _6999 = ((((((_6643 * 0.3359999954700469970703125f) + (_6634 * 0.4550000131130218505859375f)) + (_6655 * 0.19799999892711639404296875f)) + _6667) + (_6674 * 0.0040000001899898052215576171875f)) * _1030) * _6694;
                _7004 = ((((_6643 * 0.3440000116825103759765625f) + (_6634 * 0.648999989032745361328125f)) + _6667) * _1035) * _6694;
            }
            bool _7030 = _4738 != 0u;
            float _7385;
            if (_1176)
            {
                _7385 = 1.0f;
            }
            else
            {
                _7385 = _64_m0[677u].w;
            }
            float _7809;
            float _7812;
            float _7815;
            float _7818;
            float _7821;
            float _7824;
            float _7827;
            float _7829;
            float _7831;
            float _7833;
            float _7835;
            float _7837;
            if (asuint(_69_m0[59u]).w == 0u)
            {
                _7809 = 0.0f;
                _7812 = 0.0f;
                _7815 = 0.0f;
                _7818 = 0.0f;
                _7821 = 0.0f;
                _7824 = 0.0f;
                _7827 = 0.0f;
                _7829 = 0.0f;
                _7831 = 0.0f;
                _7833 = 0.0f;
                _7835 = 0.0f;
                _7837 = 0.0f;
            }
            else
            {
                uint _7866 = uint(int(_69_m0[58u].x * _312));
                uint _7867 = uint(int(_69_m0[58u].y * _314));
                uint _7868 = uint(int(max(_69_m0[60u].y - (_69_m0[60u].x * log2((_69_m0[58u].z * _309) + _69_m0[58u].w)), 0.0f)));
                uint4 _7870 = _46.Load(int4(uint3(_7866, _7867, _7868), 0u));
                uint _7872 = _7870.x;
                uint4 _7875 = asuint(_69_m0[89u]);
                uint _7886 = (((_7867 << (_7875.x & 31u)) + _7866) + (_7868 << (_7875.y & 31u))) << (_7875.z & 31u);
                float frontier_phi_353_354_ladder;
                float frontier_phi_353_354_ladder_1;
                float frontier_phi_353_354_ladder_2;
                float frontier_phi_353_354_ladder_3;
                float frontier_phi_353_354_ladder_4;
                float frontier_phi_353_354_ladder_5;
                float frontier_phi_353_354_ladder_6;
                float frontier_phi_353_354_ladder_7;
                float frontier_phi_353_354_ladder_8;
                float frontier_phi_353_354_ladder_9;
                float frontier_phi_353_354_ladder_10;
                float frontier_phi_353_354_ladder_11;
                if (_7872 == 0u)
                {
                    frontier_phi_353_354_ladder = 0.0f;
                    frontier_phi_353_354_ladder_1 = 0.0f;
                    frontier_phi_353_354_ladder_2 = 0.0f;
                    frontier_phi_353_354_ladder_3 = 0.0f;
                    frontier_phi_353_354_ladder_4 = 0.0f;
                    frontier_phi_353_354_ladder_5 = 0.0f;
                    frontier_phi_353_354_ladder_6 = 0.0f;
                    frontier_phi_353_354_ladder_7 = 0.0f;
                    frontier_phi_353_354_ladder_8 = 0.0f;
                    frontier_phi_353_354_ladder_9 = 0.0f;
                    frontier_phi_353_354_ladder_10 = 0.0f;
                    frontier_phi_353_354_ladder_11 = 0.0f;
                }
                else
                {
                    float _8328 = _385 - _69_m0[59u].x;
                    float _8329 = _386 - _69_m0[59u].y;
                    float _8330 = _387 - _69_m0[59u].z;
                    uint4 _8340 = _47.Load(_7886);
                    uint _8341 = _8340.x;
                    uint _8342 = _7886 + (_7872 & 127u);
                    uint _8343 = _8342 + ((_7872 >> 7u) & 127u);
                    uint _8344 = _8343 + ((_7872 >> 20u) & 63u);
                    uint _8345 = _8344 + ((_7872 >> 14u) & 63u);
                    uint _8346 = _8345 + (_7872 >> 26u);
                    uint _8347 = _7886 + 1u;
                    float _8498;
                    float _8500;
                    float _8502;
                    float _8504;
                    float _8506;
                    float _8508;
                    float _8510;
                    float _8512;
                    float _8514;
                    uint _8516;
                    uint _8518;
                    if (_8347 > _8342)
                    {
                        _8498 = 0.0f;
                        _8500 = 0.0f;
                        _8502 = 0.0f;
                        _8504 = 0.0f;
                        _8506 = 0.0f;
                        _8508 = 0.0f;
                        _8510 = 0.0f;
                        _8512 = 0.0f;
                        _8514 = 0.0f;
                        _8516 = _8347;
                        _8518 = _8341;
                    }
                    else
                    {
                        float _8499;
                        float _8501;
                        float _8503;
                        float _8505;
                        float _8507;
                        float _8509;
                        float _8511;
                        float _8513;
                        float _8515;
                        float _110[3];
                        float _8656 = 0.0f;
                        float _8657 = 0.0f;
                        float _8658 = 0.0f;
                        float _8659 = 0.0f;
                        float _8660 = 0.0f;
                        float _8661 = 0.0f;
                        float _8662 = 0.0f;
                        float _8663 = 0.0f;
                        float _8664 = 0.0f;
                        uint _8665 = _8347;
                        uint _8666 = _8341;
                        uint _8517;
                        uint _8669;
                        uint _8684;
                        uint _8685;
                        float _8709;
                        float _8711;
                        float _8714;
                        float _8716;
                        uint _8718;
                        uint _8719;
                        uint _8722;
                        bool _8724;
                        float _8726;
                        bool _8732;
                        for (;;)
                        {
                            _8517 = _8665 + 1u;
                            _8669 = _47.Load(_8665).x;
                            uint _8671 = _8666 * 4u;
                            uint4 _8683 = uint4(_48.Load(_8671).x, _48.Load(_8671 + 1u).x, _48.Load(_8671 + 2u).x, _48.Load(_8671 + 3u).x);
                            _8684 = _8683.x;
                            _8685 = _8683.y;
                            uint _8686 = _8683.z;
                            uint _8687 = _8683.w;
                            _8709 = spvUnpackHalf2x16(_8685 >> 16u).x;
                            _8711 = spvUnpackHalf2x16(_8686).x;
                            _8714 = spvUnpackHalf2x16(_8686 >> 16u).x;
                            _8716 = spvUnpackHalf2x16(_8687).x;
                            _8718 = (_8687 >> 16u) & 7u;
                            _8719 = _8687 & 524288u;
                            uint _8721 = uint4(_8690, _8691, _8692, _49.Load((_8666 * 4u) + 3u).x).w >> 16u;
                            _8722 = _8721 & 127u;
                            _8724 = (_8721 & 32768u) != 0u;
                            _8726 = spvUnpackHalf2x16(uint3(_8700, _8701, _50.Load((_8666 * 4u) + 2u).x).z).x;
                            _8732 = (_8687 < 3221225472u) && (((_549 & 255u) & (_8687 >> 22u)) != 0u);
                            float frontier_phi_394_pred;
                            float frontier_phi_394_pred_1;
                            float frontier_phi_394_pred_2;
                            float frontier_phi_394_pred_3;
                            float frontier_phi_394_pred_4;
                            float frontier_phi_394_pred_5;
                            float frontier_phi_394_pred_6;
                            float frontier_phi_394_pred_7;
                            float frontier_phi_394_pred_8;
                            if (_8732)
                            {
                                float _8862 = spvUnpackHalf2x16(_8684).x - _8328;
                                float _8863 = spvUnpackHalf2x16(_8684 >> 16u).x - _8329;
                                float _8864 = spvUnpackHalf2x16(_8685).x - _8330;
                                float _8870 = sqrt(((_8863 * _8863) + (_8864 * _8864)) + (_8862 * _8862));
                                float _8871 = _8870 * _8709;
                                float frontier_phi_394_pred_393_ladder;
                                float frontier_phi_394_pred_393_ladder_1;
                                float frontier_phi_394_pred_393_ladder_2;
                                float frontier_phi_394_pred_393_ladder_3;
                                float frontier_phi_394_pred_393_ladder_4;
                                float frontier_phi_394_pred_393_ladder_5;
                                float frontier_phi_394_pred_393_ladder_6;
                                float frontier_phi_394_pred_393_ladder_7;
                                float frontier_phi_394_pred_393_ladder_8;
                                if (_8871 < 1.0f)
                                {
                                    float _9008 = rsqrt(dot(float3(_8862, _8863, _8864), float3(_8862, _8863, _8864)));
                                    float _9009 = _9008 * _8862;
                                    float _9010 = _9008 * _8863;
                                    float _9011 = _9008 * _8864;
                                    float _9012 = _8870 * _8870;
                                    float _9014 = (_8709 * _8709) * _9012;
                                    float _9017 = clamp(1.0f - (_9014 * _9014), 0.0f, 1.0f);
                                    float _9267;
                                    if (_8718 == 0u)
                                    {
                                        _9267 = (1.0f / (max(_9012, 9.9999997473787516355514526367188e-05f) + ((_8726 * _8726) * 0.5f))) * (_9017 * _9017);
                                    }
                                    else
                                    {
                                        _9267 = max((1.0f / dot(float3(1.0f, _8871, _8871 * _8871), float3(_69_m0[_8718 + 60u].xyz))) * (1.0f - _8871), 0.0f);
                                    }
                                    uint _9425;
                                    bool _9427;
                                    float _9428;
                                    float _9430;
                                    float _9434;
                                    if (_8722 == 0u)
                                    {
                                        _9425 = 4294967295u;
                                        _9427 = false;
                                        _9428 = 1.0f;
                                        _9430 = 9899999600270360182784.0f;
                                        _9434 = 1.0f;
                                    }
                                    else
                                    {
                                        uint _9426 = _8722 + 4294967295u;
                                        float frontier_phi_426_427_ladder;
                                        float frontier_phi_426_427_ladder_1;
                                        float frontier_phi_426_427_ladder_2;
                                        bool frontier_phi_426_427_ladder_3;
                                        uint frontier_phi_426_427_ladder_4;
                                        if (_1176)
                                        {
                                            uint4 _9618 = _56.Load((_8722 * 28u) + 4294967270u);
                                            uint _9619 = _9618.x;
                                            uint _9622 = (_8722 * 28u) + 4294967273u;
                                            float3 _9632 = asfloat(uint3(_56.Load(_9622).x, _56.Load(_9622 + 1u).x, _56.Load(_9622 + 2u).x));
                                            uint _9635 = (_8722 * 28u) + 4294967276u;
                                            float3 _9645 = asfloat(uint3(_56.Load(_9635).x, _56.Load(_9635 + 1u).x, _56.Load(_9635 + 2u).x));
                                            float _9662 = asfloat(_56.Load((_8722 * 28u) + 4294967288u).x);
                                            float _9673 = asfloat(_56.Load((_8722 * 28u) + 4294967290u).x);
                                            float _9429;
                                            float _9435;
                                            float _9923;
                                            if ((_9619 < 4u) && (_64_m0[125u].y != 0.0f))
                                            {
                                                float _9785 = float(_9619 == 0u);
                                                float _9787 = float(_9619 == 1u);
                                                float _9789 = float(_9619 == 2u);
                                                float _9791 = float(_9619 == 3u);
                                                uint _9792 = uint(_312);
                                                uint _9793 = uint(_314);
                                                float _9801 = dot(float4(_54.Load(int3(uint2(_9792, _9793), 0u))), float4(_9785, _9787, _9789, _9791));
                                                float _9811 = dot(float4(_55.Load(int3(uint2(_9792, _9793), 0u))), float4(_9785, _9787, _9789, _9791));
                                                float _9815 = (_385 - _9645.x) - _9632.x;
                                                float _9817 = (_386 - _9645.y) - _9632.y;
                                                float _9819 = (_387 - _9645.z) - _9632.z;
                                                _110[0u] = _9815;
                                                _110[1u] = _9817;
                                                _110[2u] = _9819;
                                                float _9824 = abs(_9815);
                                                float _9825 = abs(_9817);
                                                float _9826 = abs(_9819);
                                                uint _9832 = (_9824 > _9825) ? ((_9824 > _9826) ? 0u : 2u) : ((_9825 > _9826) ? 1u : 2u);
                                                uint _9839 = ((_9832 << 1u) | uint(_110[_9832] < 0.0f)) + _56.Load((_8722 * 28u) + 4294967268u).x;
                                                uint _9841 = _9839 * 16u;
                                                float4 _9854 = asfloat(uint4(_53.Load(_9841).x, _53.Load(_9841 + 1u).x, _53.Load(_9841 + 2u).x, _53.Load(_9841 + 3u).x));
                                                uint _9860 = (_9839 * 16u) + 4u;
                                                float4 _9873 = asfloat(uint4(_53.Load(_9860).x, _53.Load(_9860 + 1u).x, _53.Load(_9860 + 2u).x, _53.Load(_9860 + 3u).x));
                                                uint _9879 = (_9839 * 16u) + 12u;
                                                float4 _9892 = asfloat(uint4(_53.Load(_9879).x, _53.Load(_9879 + 1u).x, _53.Load(_9879 + 2u).x, _53.Load(_9879 + 3u).x));
                                                float _9908 = mad(_9819, _9892.z, mad(_9817, _9892.y, _9892.x * _9815)) + _9892.w;
                                                float _9911 = ((mad(_9819, _9854.z, mad(_9817, _9854.y, _9854.x * _9815)) + _9854.w) / _9908) * 0.5f;
                                                float _9912 = ((mad(_9819, _9873.z, mad(_9817, _9873.y, _9873.x * _9815)) + _9873.w) / _9908) * (-0.5f);
                                                float _9913 = _9911 + 0.5f;
                                                float _9914 = _9912 + 0.5f;
                                                float frontier_phi_447_446_ladder;
                                                float frontier_phi_447_446_ladder_1;
                                                float frontier_phi_447_446_ladder_2;
                                                if (((_9913 < 0.0f) || (_9913 > 1.0f)) || ((_9914 < 0.0f) || (_9914 > 1.0f)))
                                                {
                                                    frontier_phi_447_446_ladder = 0.0f;
                                                    frontier_phi_447_446_ladder_1 = _9801;
                                                    frontier_phi_447_446_ladder_2 = _9811;
                                                }
                                                else
                                                {
                                                    float _10533;
                                                    if (_56.Load((_8722 * 28u) + 4294967287u).x == 0u)
                                                    {
                                                        _10533 = 1.0f;
                                                    }
                                                    else
                                                    {
                                                        float _10545 = clamp(((max(abs((-0.0f) - _9911), abs((-0.0f) - _9912)) * 2.0f) - _9662) / (0.999000012874603271484375f - _9662), 0.0f, 1.0f);
                                                        _10533 = 1.0f - ((_10545 * _10545) * (3.0f - (_10545 * 2.0f)));
                                                    }
                                                    float _11146;
                                                    if (_56.Load((_8722 * 28u) + 4294967289u).x == 0u)
                                                    {
                                                        _11146 = _10533;
                                                    }
                                                    else
                                                    {
                                                        float _11154 = clamp((((-0.0f) - (_9819 * asfloat(_56.Load((_8722 * 28u) + 4294967292u).x))) - _9673) / (asfloat(_56.Load((_8722 * 28u) + 4294967291u).x) - _9673), 0.0f, 1.0f);
                                                        _11146 = (1.0f - ((_11154 * _11154) * (3.0f - (_11154 * 2.0f)))) * _10533;
                                                    }
                                                    frontier_phi_447_446_ladder = _11146 * (1.0f - asfloat(_56.Load((_8722 * 28u) + 4294967286u).x));
                                                    frontier_phi_447_446_ladder_1 = _9801;
                                                    frontier_phi_447_446_ladder_2 = _9811;
                                                }
                                                _9435 = frontier_phi_447_446_ladder;
                                                _9429 = frontier_phi_447_446_ladder_1;
                                                _9923 = frontier_phi_447_446_ladder_2;
                                            }
                                            else
                                            {
                                                _9435 = 0.0f;
                                                _9429 = 1.0f;
                                                _9923 = 1.0f;
                                            }
                                            float _9930 = (_56.Load((_8722 * 28u) + 4294967293u).x == 1u) ? _9435 : 0.0f;
                                            float frontier_phi_426_427_ladder_447_ladder;
                                            float frontier_phi_426_427_ladder_447_ladder_1;
                                            float frontier_phi_426_427_ladder_447_ladder_2;
                                            bool frontier_phi_426_427_ladder_447_ladder_3;
                                            uint frontier_phi_426_427_ladder_447_ladder_4;
                                            if ((_1174 & 1073741824u) == 0u)
                                            {
                                                float frontier_phi_426_427_ladder_447_ladder_459_ladder;
                                                float frontier_phi_426_427_ladder_447_ladder_459_ladder_1;
                                                float frontier_phi_426_427_ladder_447_ladder_459_ladder_2;
                                                bool frontier_phi_426_427_ladder_447_ladder_459_ladder_3;
                                                uint frontier_phi_426_427_ladder_447_ladder_459_ladder_4;
                                                if (int(_1174) < int(0u))
                                                {
                                                    float _9431;
                                                    if (_7030 && (_9930 > 0.0f))
                                                    {
                                                        _9431 = ((1.0f - _9923) / (1.00000095367431640625f - _9429)) * _4743;
                                                    }
                                                    else
                                                    {
                                                        _9431 = 9899999600270360182784.0f;
                                                    }
                                                    float frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder;
                                                    float frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_1;
                                                    float frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_2;
                                                    bool frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_3;
                                                    uint frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_4;
                                                    if (_9930 < 1.0f)
                                                    {
                                                        float _11695 = dot(float3(_9009, _9010, _9011), float3(_958, _952, _946));
                                                        float _11698 = dot(float3(_9009, _9010, _9011), float3(_960, _954, _948));
                                                        float _11701 = dot(float3(_9009, _9010, _9011), float3(_962, _956, _950));
                                                        float _11704 = abs(_11695);
                                                        float _11705 = abs(_11698);
                                                        float _11706 = abs(_11701);
                                                        float _11708 = (_11705 + _11704) + _11706;
                                                        float _11721 = dot(float3(_11704 / _11708, _11705 / _11708, _11706 / _11708), float3((_11695 < 0.0f) ? _970 : _964, (_11698 < 0.0f) ? _972 : _966, (_11701 < 0.0f) ? _974 : _968)) * _976;
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder = _9435;
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_1 = ((_9431 - _11721) * _9930) + _11721;
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_2 = _9429;
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_3 = _1176;
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_4 = _9426;
                                                    }
                                                    else
                                                    {
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder = _9435;
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_1 = _9431;
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_2 = _9429;
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_3 = _1176;
                                                        frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_4 = _9426;
                                                    }
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder = frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder;
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder_1 = frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_1;
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder_2 = frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_2;
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder_3 = frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_3;
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder_4 = frontier_phi_426_427_ladder_447_ladder_459_ladder_499_ladder_4;
                                                }
                                                else
                                                {
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder = _9435;
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder_1 = 9899999600270360182784.0f;
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder_2 = _9429;
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder_3 = _1176;
                                                    frontier_phi_426_427_ladder_447_ladder_459_ladder_4 = _9426;
                                                }
                                                frontier_phi_426_427_ladder_447_ladder = frontier_phi_426_427_ladder_447_ladder_459_ladder;
                                                frontier_phi_426_427_ladder_447_ladder_1 = frontier_phi_426_427_ladder_447_ladder_459_ladder_1;
                                                frontier_phi_426_427_ladder_447_ladder_2 = frontier_phi_426_427_ladder_447_ladder_459_ladder_2;
                                                frontier_phi_426_427_ladder_447_ladder_3 = frontier_phi_426_427_ladder_447_ladder_459_ladder_3;
                                                frontier_phi_426_427_ladder_447_ladder_4 = frontier_phi_426_427_ladder_447_ladder_459_ladder_4;
                                            }
                                            else
                                            {
                                                float frontier_phi_426_427_ladder_447_ladder_460_ladder;
                                                float frontier_phi_426_427_ladder_447_ladder_460_ladder_1;
                                                float frontier_phi_426_427_ladder_447_ladder_460_ladder_2;
                                                bool frontier_phi_426_427_ladder_447_ladder_460_ladder_3;
                                                uint frontier_phi_426_427_ladder_447_ladder_460_ladder_4;
                                                if (_9930 != 0.0f)
                                                {
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder = _9435;
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder_1 = max(_1130, ((1.0f - _9923) / (1.00000095367431640625f - _9429)) * _4743);
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder_2 = _9429;
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder_3 = _1176;
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder_4 = _9426;
                                                }
                                                else
                                                {
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder = _9435;
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder_1 = 9899999600270360182784.0f;
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder_2 = _9429;
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder_3 = _1176;
                                                    frontier_phi_426_427_ladder_447_ladder_460_ladder_4 = _9426;
                                                }
                                                frontier_phi_426_427_ladder_447_ladder = frontier_phi_426_427_ladder_447_ladder_460_ladder;
                                                frontier_phi_426_427_ladder_447_ladder_1 = frontier_phi_426_427_ladder_447_ladder_460_ladder_1;
                                                frontier_phi_426_427_ladder_447_ladder_2 = frontier_phi_426_427_ladder_447_ladder_460_ladder_2;
                                                frontier_phi_426_427_ladder_447_ladder_3 = frontier_phi_426_427_ladder_447_ladder_460_ladder_3;
                                                frontier_phi_426_427_ladder_447_ladder_4 = frontier_phi_426_427_ladder_447_ladder_460_ladder_4;
                                            }
                                            frontier_phi_426_427_ladder = frontier_phi_426_427_ladder_447_ladder;
                                            frontier_phi_426_427_ladder_1 = frontier_phi_426_427_ladder_447_ladder_1;
                                            frontier_phi_426_427_ladder_2 = frontier_phi_426_427_ladder_447_ladder_2;
                                            frontier_phi_426_427_ladder_3 = frontier_phi_426_427_ladder_447_ladder_3;
                                            frontier_phi_426_427_ladder_4 = frontier_phi_426_427_ladder_447_ladder_4;
                                        }
                                        else
                                        {
                                            frontier_phi_426_427_ladder = 1.0f;
                                            frontier_phi_426_427_ladder_1 = 9899999600270360182784.0f;
                                            frontier_phi_426_427_ladder_2 = 1.0f;
                                            frontier_phi_426_427_ladder_3 = _1176;
                                            frontier_phi_426_427_ladder_4 = _9426;
                                        }
                                        _9425 = frontier_phi_426_427_ladder_4;
                                        _9427 = frontier_phi_426_427_ladder_3;
                                        _9428 = frontier_phi_426_427_ladder_2;
                                        _9430 = frontier_phi_426_427_ladder_1;
                                        _9434 = frontier_phi_426_427_ladder;
                                    }
                                    float _9739;
                                    float _9741;
                                    float _9743;
                                    float _9745;
                                    float _9747;
                                    float _9749;
                                    float _9751;
                                    float _9753;
                                    float _9755;
                                    float _9441;
                                    float _9442;
                                    float _9443;
                                    bool _9444;
                                    for (;;)
                                    {
                                        float _9440 = max(float(_8724) * 16.0f, 1.0f) * _9267;
                                        _9441 = _9440 * _8711;
                                        _9442 = _9440 * _8714;
                                        _9443 = _9440 * _8716;
                                        _9444 = _8719 == 0u;
                                        if (_9444)
                                        {
                                            float _9599 = clamp(clamp(dot(float3(_1000, _1004, _1008), float3(_9009, _9010, _9011)), 0.0f, 1.0f), 0.0f, 1.0f);
                                            _9739 = (_9599 * ((_1025 * 0.3183098733425140380859375f) * (1.0f - _1040))) * _9441;
                                            _9741 = (_9599 * ((_1030 * 0.3183098733425140380859375f) * (1.0f - _1045))) * _9442;
                                            _9743 = (_9599 * ((_1035 * 0.3183098733425140380859375f) * (1.0f - _1048))) * _9443;
                                            _9745 = 0.0f;
                                            _9747 = 0.0f;
                                            _9749 = 0.0f;
                                            _9751 = 0.0f;
                                            _9753 = 0.0f;
                                            _9755 = 0.0f;
                                            break;
                                        }
                                        else
                                        {
                                            float _10254;
                                            float _10259;
                                            float _10264;
                                            float _10269;
                                            float _10275;
                                            float _10281;
                                            float _10287;
                                            float _10289;
                                            float _10291;
                                            if ((_1174 & 4194304u) == 0u)
                                            {
                                                float frontier_phi_457_444_ladder;
                                                float frontier_phi_457_444_ladder_1;
                                                float frontier_phi_457_444_ladder_2;
                                                float frontier_phi_457_444_ladder_3;
                                                float frontier_phi_457_444_ladder_4;
                                                float frontier_phi_457_444_ladder_5;
                                                float frontier_phi_457_444_ladder_6;
                                                float frontier_phi_457_444_ladder_7;
                                                float frontier_phi_457_444_ladder_8;
                                                if ((_1174 & 8388608u) == 0u)
                                                {
                                                    float frontier_phi_457_444_ladder_454_ladder;
                                                    float frontier_phi_457_444_ladder_454_ladder_1;
                                                    float frontier_phi_457_444_ladder_454_ladder_2;
                                                    float frontier_phi_457_444_ladder_454_ladder_3;
                                                    float frontier_phi_457_444_ladder_454_ladder_4;
                                                    float frontier_phi_457_444_ladder_454_ladder_5;
                                                    float frontier_phi_457_444_ladder_454_ladder_6;
                                                    float frontier_phi_457_444_ladder_454_ladder_7;
                                                    float frontier_phi_457_444_ladder_454_ladder_8;
                                                    if ((_1174 & 67108864u) == 0u)
                                                    {
                                                        float frontier_phi_457_444_ladder_454_ladder_469_ladder;
                                                        float frontier_phi_457_444_ladder_454_ladder_469_ladder_1;
                                                        float frontier_phi_457_444_ladder_454_ladder_469_ladder_2;
                                                        float frontier_phi_457_444_ladder_454_ladder_469_ladder_3;
                                                        float frontier_phi_457_444_ladder_454_ladder_469_ladder_4;
                                                        float frontier_phi_457_444_ladder_454_ladder_469_ladder_5;
                                                        float frontier_phi_457_444_ladder_454_ladder_469_ladder_6;
                                                        float frontier_phi_457_444_ladder_454_ladder_469_ladder_7;
                                                        float frontier_phi_457_444_ladder_454_ladder_469_ladder_8;
                                                        if ((_1174 & 50331648u) == 0u)
                                                        {
                                                            float frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder;
                                                            float frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_1;
                                                            float frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_2;
                                                            float frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_3;
                                                            float frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_4;
                                                            float frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_5;
                                                            float frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_6;
                                                            float frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_7;
                                                            float frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_8;
                                                            if ((_1174 & 1032192u) == 0u)
                                                            {
                                                                float _10258;
                                                                float _10263;
                                                                float _10268;
                                                                float _10288;
                                                                float _10290;
                                                                float _10292;
                                                                if ((_1104 > 0.0f) && ((_1174 & 3u) != 0u))
                                                                {
                                                                    float _12010 = max(max(max(max(_1107, 0.0f), _1104), _1109), 0.0f);
                                                                    float _12016 = _64_m0[8u].x - _997;
                                                                    float _12017 = _64_m0[8u].y - _998;
                                                                    float _12018 = _64_m0[8u].z - _999;
                                                                    float _12029 = clamp(2.0f - (clamp(sqrt(((_12016 * _12016) + (_12017 * _12017)) + (_12018 * _12018)) * 0.02500000037252902984619140625f, 0.0f, 1.0f) * 2.0f), 0.0f, 1.0f);
                                                                    float _12030 = _12029 * _12010;
                                                                    float _12036 = _64_m0[21u].x - _997;
                                                                    float _12037 = _64_m0[21u].y - _998;
                                                                    float _12038 = _64_m0[21u].z - _999;
                                                                    float _12045 = log2(sqrt(((_12036 * _12036) + (_12037 * _12037)) + (_12038 * _12038)));
                                                                    float _12046 = _12045 * 0.85000002384185791015625f;
                                                                    float _12929;
                                                                    float _12930;
                                                                    float _12931;
                                                                    float _12932;
                                                                    if (_12030 > 0.00999999977648258209228515625f)
                                                                    {
                                                                        uint _12763 = _1106 + 0u;
                                                                        float _12764 = ceil(_12046);
                                                                        float _12768 = _997 * 5.0f;
                                                                        float _12769 = _999 * 5.0f;
                                                                        float _12771 = exp2((-0.0f) - max(1.0f, _12764));
                                                                        float _12777 = exp2((-0.0f) - max(1.0f, _12764 + 1.0f));
                                                                        float4 _12786 = _39[NonUniformResourceIndex(_12763)].SampleLevel(_77, float2(frac(_12771 * _12768), frac(_12771 * _12769)), 0.0f);
                                                                        float4 _12791 = _39[NonUniformResourceIndex(_12763)].SampleLevel(_77, float2(frac(_12777 * _12768), frac(_12777 * _12769)), 0.0f);
                                                                        float _12796 = frac(_12046);
                                                                        float _12798 = (_12796 + 0.5f) * 0.5f;
                                                                        float _12810 = (_12786.x + (-0.5f)) * 2.0f;
                                                                        float _12811 = (_12786.y + (-0.5f)) * 2.0f;
                                                                        float _12817 = sqrt(clamp((1.0f - (_12810 * _12810)) - (_12811 * _12811), 0.0f, 1.0f));
                                                                        float _12824 = (_12791.x + (-0.5f)) * 2.0f;
                                                                        float _12825 = (_12791.y + (-0.5f)) * 2.0f;
                                                                        float _12831 = sqrt(clamp((1.0f - (_12824 * _12824)) - (_12825 * _12825), 0.0f, 1.0f));
                                                                        float _12837 = rsqrt(dot(float3(_12810, _12817, _12811), float3(_12810, _12817, _12811))) * (1.0f - _12796);
                                                                        float _12840 = rsqrt(dot(float3(_12824, _12831, _12825), float3(_12824, _12831, _12825))) * _12796;
                                                                        float _12843 = (_12840 * _12824) + (_12837 * _12810);
                                                                        float _12844 = (_12840 * _12825) + (_12837 * _12811);
                                                                        float _12848 = rsqrt(dot(float3(_12843, _12844, 1.0f), float3(_12843, _12844, 1.0f)));
                                                                        float _12851 = mad(_12848, _1000, _12843 * _12848);
                                                                        float _12852 = mad(_12848, _1004, 0.0f);
                                                                        float _12853 = mad(_12848, _1008, _12844 * _12848);
                                                                        float _12857 = rsqrt(dot(float3(_12851, _12852, _12853), float3(_12851, _12852, _12853)));
                                                                        float _12858 = _12857 * _12851;
                                                                        float _12859 = _12857 * _12852;
                                                                        float _12860 = _12857 * _12853;
                                                                        float _12861 = _1740 + 1.0f;
                                                                        float _12865 = rsqrt(dot(float3(_1739, _12861, _1741), float3(_1739, _12861, _1741)));
                                                                        float _12908 = (((((_12030 * 2.2000000476837158203125f) * max(clamp(_12798 * (_12786.z - _12796), 0.0f, 1.0f), clamp((1.0f - _12798) * ((_12796 + (-1.0f)) + _12791.z), 0.0f, 1.0f))) * exp2((800.0f - (clamp(clamp((_12045 * 0.425000011920928955078125f) + (-1.0f), 0.0f, 1.0f), 0.0f, 1.0f) * 800.0f)) * log2(((_12010 * (0.004999999888241291046142578125f - (_1109 * 0.00299999979324638843536376953125f))) + 0.00200000009499490261077880859375f) + max(dot(float3(_12865 * _1739, _12865 * _12861, _12865 * _1741), float3(_12858, _12859, _12860)), 0.0f)))) * exp2(log2(clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 1.2000000476837158203125f)) * clamp((dot(float3(_9009, _9010, _9011), float3(_1000, _1004, _1008)) * 10.0f) + 0.5f, 0.0f, 1.0f)) * _64_m0[166u].y;
                                                                        float _12910 = (_1109 * 0.4000000059604644775390625f) * _12029;
                                                                        float _12913 = ((_12860 + (-1.0f)) * _12910) + 1.0f;
                                                                        float _12916 = _12910 * _1008;
                                                                        float _12919 = (_12913 * _1000) + (_12858 * _12916);
                                                                        float _12920 = (_12913 * _1004) + (_12859 * _12916);
                                                                        float _12921 = _12913 * _1008;
                                                                        float _12925 = rsqrt(dot(float3(_12919, _12920, _12921), float3(_12919, _12920, _12921)));
                                                                        _12929 = _12908;
                                                                        _12930 = _12925 * _12919;
                                                                        _12931 = _12925 * _12920;
                                                                        _12932 = _12925 * _12921;
                                                                    }
                                                                    else
                                                                    {
                                                                        _12929 = 0.0f;
                                                                        _12930 = _1000;
                                                                        _12931 = _1004;
                                                                        _12932 = _1008;
                                                                    }
                                                                    float _12936 = dot(float3(_12930, _12931, _12932), float3(_9009, _9010, _9011));
                                                                    float _12942 = (1.0f - clamp(_12936 * 5.0f, 0.0f, 1.0f)) * _1107;
                                                                    float _12949 = (_12942 * (_1025 - _1111)) + _1111;
                                                                    float _12950 = (_12942 * (_1030 - _1113)) + _1113;
                                                                    float _12951 = (_12942 * (_1035 - _1115)) + _1115;
                                                                    float _12952 = 1.0f - _1040;
                                                                    float _12953 = 1.0f - _1045;
                                                                    float _12954 = 1.0f - _1048;
                                                                    float _12962 = exp2(log2(1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_12930, _12931, _12932)), 0.0f, 1.0f)) * 5.0f);
                                                                    float _12969 = _1109 * 0.550000011920928955078125f;
                                                                    float _12970 = ((_12962 * _12952) + _1040) * _12969;
                                                                    float _12971 = ((_12962 * _12953) + _1045) * _12969;
                                                                    float _12972 = ((_12962 * _12954) + _1048) * _12969;
                                                                    float _12991 = clamp(_12936, 0.0f, 1.0f);
                                                                    float _13648;
                                                                    if (_64_m0[76u].w > 0.5f)
                                                                    {
                                                                        float _13611 = _9009 + _1739;
                                                                        float _13612 = _9010 + _1740;
                                                                        float _13613 = _9011 + _1741;
                                                                        float _13617 = rsqrt(dot(float3(_13611, _13612, _13613), float3(_13611, _13612, _13613)));
                                                                        float _13624 = clamp(dot(float3(_9009, _9010, _9011), float3(_13617 * _13611, _13617 * _13612, _13617 * _13613)), 0.0f, 1.0f);
                                                                        float _13634 = ((((_13624 * _13624) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                                        _13648 = ((exp2(log2(1.0f - clamp(dot(float3(_12930, _12931, _12932), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _13634) + 1.0f) * ((_13634 * exp2(log2(1.0f - _12991) * 5.0f)) + 1.0f);
                                                                    }
                                                                    else
                                                                    {
                                                                        _13648 = 1.0f;
                                                                    }
                                                                    float _13650 = _13648 * (clamp((_12991 * 0.722500026226043701171875f) + 0.12750001251697540283203125f, 0.0f, 1.0f) * 0.3183098733425140380859375f);
                                                                    _10258 = (_13650 * _12952) * (((_12970 * _12970) * ((0.949999988079071044921875f - (_1107 * 0.5f)) - _12949)) + _12949);
                                                                    _10263 = (_13650 * _12953) * (((_12971 * _12971) * ((0.810000002384185791015625f - (_1107 * 0.069999992847442626953125f)) - _12950)) + _12950);
                                                                    _10268 = (_13650 * _12954) * (((_12972 * _12972) * (((_1107 * 0.37999999523162841796875f) + 0.569999992847442626953125f) - _12951)) + _12951);
                                                                    _10288 = _12929 * 0.449999988079071044921875f;
                                                                    _10290 = _12929 * 0.449999988079071044921875f;
                                                                    _10292 = _12929 * 0.449999988079071044921875f;
                                                                }
                                                                else
                                                                {
                                                                    float _12054 = clamp(dot(float3(_1000, _1004, _1008), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                                    float _12055 = clamp(_12054, 0.0f, 1.0f);
                                                                    float _13036;
                                                                    if (_64_m0[76u].w > 0.5f)
                                                                    {
                                                                        float _12999 = _9009 + _1739;
                                                                        float _13000 = _9010 + _1740;
                                                                        float _13001 = _9011 + _1741;
                                                                        float _13005 = rsqrt(dot(float3(_12999, _13000, _13001), float3(_12999, _13000, _13001)));
                                                                        float _13012 = clamp(dot(float3(_9009, _9010, _9011), float3(_13005 * _12999, _13005 * _13000, _13005 * _13001)), 0.0f, 1.0f);
                                                                        float _13022 = ((((_13012 * _13012) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                                        _13036 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _13022) + 1.0f) * ((_13022 * exp2(log2(1.0f - _12054) * 5.0f)) + 1.0f);
                                                                    }
                                                                    else
                                                                    {
                                                                        _13036 = 1.0f;
                                                                    }
                                                                    _10258 = (((_1025 * 0.3183098733425140380859375f) * (1.0f - _1040)) * _12055) * _13036;
                                                                    _10263 = (((_1030 * 0.3183098733425140380859375f) * (1.0f - _1045)) * _12055) * _13036;
                                                                    _10268 = (((_1035 * 0.3183098733425140380859375f) * (1.0f - _1048)) * _12055) * _13036;
                                                                    _10288 = 0.0f;
                                                                    _10290 = 0.0f;
                                                                    _10292 = 0.0f;
                                                                }
                                                                float _13660 = clamp(dot(float3(_1012, _1015, _1018), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                                float _14109;
                                                                float _14111;
                                                                float _14113;
                                                                if (_13660 > 0.0f)
                                                                {
                                                                    float _14046 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                                    float _14047 = _9009 + _1739;
                                                                    float _14048 = _9010 + _1740;
                                                                    float _14049 = _9011 + _1741;
                                                                    float _14053 = rsqrt(dot(float3(_14047, _14048, _14049), float3(_14047, _14048, _14049)));
                                                                    float _14054 = _14053 * _14047;
                                                                    float _14055 = _14053 * _14048;
                                                                    float _14056 = _14053 * _14049;
                                                                    float _14065 = clamp(dot(float3(_1012, _1015, _1018), float3(_14054, _14055, _14056)), 0.0f, 1.0f);
                                                                    float _14070 = _14046 * _14046;
                                                                    float _14071 = _14070 * _14070;
                                                                    float _14075 = (((_14065 * _14071) - _14065) * _14065) + 1.0f;
                                                                    float _14079 = _14070 * 0.5f;
                                                                    float _14080 = 1.0f - _14079;
                                                                    float _14087 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_14054, _14055, _14056)), 0.0f, 1.0f);
                                                                    float _14088 = _14087 * _14087;
                                                                    float _14090 = (_14088 * _14088) * _14087;
                                                                    float _14102 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _14080) + _14079) * ((_14080 * _13660) + _14079))) * (_14071 / ((_14075 * _14075) * 3.1415927410125732421875f)), _1719) * _4688;
                                                                    float _14106 = min(_14102 * ((_14090 * (1.0f - _1040)) + _1040), 100000.0f);
                                                                    float _14107 = min(_14102 * ((_14090 * (1.0f - _1045)) + _1045), 100000.0f);
                                                                    float _14108 = min(_14102 * ((_14090 * (1.0f - _1048)) + _1048), 100000.0f);
                                                                    float _14138;
                                                                    float _14140;
                                                                    float _14142;
                                                                    if (_4692)
                                                                    {
                                                                        _14138 = _14106;
                                                                        _14140 = _14107;
                                                                        _14142 = _14108;
                                                                    }
                                                                    else
                                                                    {
                                                                        float _14144 = 1.0f - _13660;
                                                                        float _14145 = _14144 * _14144;
                                                                        float _14147 = 1.0f - (_14145 * _14145);
                                                                        _14138 = _14106 * _14147;
                                                                        _14140 = _14107 * _14147;
                                                                        _14142 = _14108 * _14147;
                                                                    }
                                                                    _14109 = _14138 * _13660;
                                                                    _14111 = _14140 * _13660;
                                                                    _14113 = _14142 * _13660;
                                                                }
                                                                else
                                                                {
                                                                    _14109 = 0.0f;
                                                                    _14111 = 0.0f;
                                                                    _14113 = 0.0f;
                                                                }
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder = _10288;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_1 = _10258;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_2 = _10263;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_3 = _10268;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_4 = _14109 * _1065;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_5 = _14111 * _1065;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_6 = _14113 * _1065;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_7 = _10290;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_8 = _10292;
                                                            }
                                                            else
                                                            {
                                                                float _11438 = 1.0f - _1040;
                                                                float _11439 = 1.0f - _1045;
                                                                float _11440 = 1.0f - _1048;
                                                                float _11444 = clamp(dot(float3(_1000, _1004, _1008), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                                float _11445 = clamp(_11444, 0.0f, 1.0f);
                                                                float _12097;
                                                                if (_64_m0[76u].w > 0.5f)
                                                                {
                                                                    float _12060 = _9009 + _1739;
                                                                    float _12061 = _9010 + _1740;
                                                                    float _12062 = _9011 + _1741;
                                                                    float _12066 = rsqrt(dot(float3(_12060, _12061, _12062), float3(_12060, _12061, _12062)));
                                                                    float _12073 = clamp(dot(float3(_9009, _9010, _9011), float3(_12066 * _12060, _12066 * _12061, _12066 * _12062)), 0.0f, 1.0f);
                                                                    float _12083 = ((((_12073 * _12073) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                                    _12097 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _12083) + 1.0f) * ((_12083 * exp2(log2(1.0f - _11444) * 5.0f)) + 1.0f);
                                                                }
                                                                else
                                                                {
                                                                    _12097 = 1.0f;
                                                                }
                                                                float _13074;
                                                                float _13076;
                                                                float _13078;
                                                                if (_11444 > 0.0f)
                                                                {
                                                                    float _13054 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                                    float _13055 = _9009 + _1739;
                                                                    float _13056 = _9010 + _1740;
                                                                    float _13057 = _9011 + _1741;
                                                                    float _13061 = rsqrt(dot(float3(_13055, _13056, _13057), float3(_13055, _13056, _13057)));
                                                                    float _13062 = _13061 * _13055;
                                                                    float _13063 = _13061 * _13056;
                                                                    float _13064 = _13061 * _13057;
                                                                    float _13068 = clamp(dot(float3(_1012, _1015, _1018), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                                    float _13072 = clamp(dot(float3(_1012, _1015, _1018), float3(_13062, _13063, _13064)), 0.0f, 1.0f);
                                                                    float _13704;
                                                                    float _13705;
                                                                    float _13706;
                                                                    if (_13068 > 0.0f)
                                                                    {
                                                                        float _13671 = _13054 * _13054;
                                                                        float _13672 = _13671 * _13671;
                                                                        float _13676 = (((_13072 * _13672) - _13072) * _13072) + 1.0f;
                                                                        float _13680 = _13671 * 0.5f;
                                                                        float _13681 = 1.0f - _13680;
                                                                        float _13688 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_13062, _13063, _13064)), 0.0f, 1.0f);
                                                                        float _13689 = _13688 * _13688;
                                                                        float _13691 = (_13689 * _13689) * _13688;
                                                                        float _13700 = min((0.25f / (((_13068 * _13681) + _13680) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _13681) + _13680))) * (_13672 / ((_13676 * _13676) * 3.1415927410125732421875f)), _1719) * _4688;
                                                                        _13704 = _13700 * ((_13691 * _11438) + _1040);
                                                                        _13705 = _13700 * ((_13691 * _11439) + _1045);
                                                                        _13706 = _13700 * ((_13691 * _11440) + _1048);
                                                                    }
                                                                    else
                                                                    {
                                                                        _13704 = 0.0f;
                                                                        _13705 = 0.0f;
                                                                        _13706 = 0.0f;
                                                                    }
                                                                    float _13707 = min(_13704, 100000.0f);
                                                                    float _13708 = min(_13705, 100000.0f);
                                                                    float _13709 = min(_13706, 100000.0f);
                                                                    float _14115;
                                                                    float _14117;
                                                                    float _14119;
                                                                    if (_4692)
                                                                    {
                                                                        _14115 = _13707;
                                                                        _14117 = _13708;
                                                                        _14119 = _13709;
                                                                    }
                                                                    else
                                                                    {
                                                                        float _14121 = 1.0f - _11444;
                                                                        float _14122 = _14121 * _14121;
                                                                        float _14124 = 1.0f - (_14122 * _14122);
                                                                        _14115 = _13707 * _14124;
                                                                        _14117 = _13708 * _14124;
                                                                        _14119 = _13709 * _14124;
                                                                    }
                                                                    _13074 = _14115 * _11444;
                                                                    _13076 = _14117 * _11444;
                                                                    _13078 = _14119 * _11444;
                                                                }
                                                                else
                                                                {
                                                                    _13074 = 0.0f;
                                                                    _13076 = 0.0f;
                                                                    _13078 = 0.0f;
                                                                }
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder = 0.0f;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_1 = (((_1025 * 0.3183098733425140380859375f) * _11438) * _11445) * _12097;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_2 = (((_1030 * 0.3183098733425140380859375f) * _11439) * _11445) * _12097;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_3 = (((_1035 * 0.3183098733425140380859375f) * _11440) * _11445) * _12097;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_4 = _13074 * _1065;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_5 = _13076 * _1065;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_6 = _13078 * _1065;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_7 = 0.0f;
                                                                frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_8 = 0.0f;
                                                            }
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder = frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_1 = frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_1;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_2 = frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_2;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_3 = frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_3;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_4 = frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_4;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_5 = frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_5;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_6 = frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_6;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_7 = frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_7;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_8 = frontier_phi_457_444_ladder_454_ladder_469_ladder_489_ladder_8;
                                                        }
                                                        else
                                                        {
                                                            float _10930 = 1.0f - _1051;
                                                            float _10931 = (-0.0f) - _962;
                                                            float _10932 = (-0.0f) - _956;
                                                            float _10933 = (-0.0f) - _950;
                                                            float _10941 = dot(float3(_10931, _10932, _10933), float3(_9009, _9010, _9011));
                                                            float _10947 = dot(float3(_10931, _10932, _10933), float3(_1739, _1740, _1741));
                                                            float _10953 = _9009 - (_10941 * _10931);
                                                            float _10954 = _9010 - (_10941 * _10932);
                                                            float _10955 = _9011 - (_10941 * _10933);
                                                            float _10959 = _1739 - (_10947 * _10931);
                                                            float _10960 = _1740 - (_10947 * _10932);
                                                            float _10961 = _1741 - (_10947 * _10933);
                                                            float _10974 = rsqrt((dot(float3(_10959, _10960, _10961), float3(_10959, _10960, _10961)) * dot(float3(_10953, _10954, _10955), float3(_10953, _10954, _10955))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_10953, _10954, _10955), float3(_10959, _10960, _10961));
                                                            float _10978 = sqrt(clamp((_10974 * 0.5f) + 0.5f, 0.0f, 1.0f));
                                                            float _10984 = cos(abs(asin(_10947) - asin(_10941)) * 0.5f);
                                                            float _10989 = max(_10930 * 2.0f, 0.00999999977648258209228515625f);
                                                            float _10991 = (-0.0f) - _1171;
                                                            float _10993 = sin(_10991);
                                                            float _11004 = _10947 + _10941;
                                                            float _11005 = _11004 - ((_10993 * 2.0f) * (((cos(_10991) * _10978) * sqrt(1.0f - (_10947 * _10947))) + (_10993 * _10947)));
                                                            float _11007 = (_10978 * 1.41421353816986083984375f) * max((1.0f - _1165) * _10930, 0.00999999977648258209228515625f);
                                                            float _11029 = _11004 - (_1171 * 1.5f);
                                                            float _11055 = exp2(log2(1.0f - (_10984 * 0.5f)) * 5.0f) * 0.95347940921783447265625f;
                                                            float _11057 = 0.95347940921783447265625f - _11055;
                                                            float _11062 = clamp((1.0f - max(_1169, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                                                            float _11068 = abs(sqrt(1.0f - (_10941 * _10941))) * clamp(dot(float3(_1000, _1004, _1008), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                            float _11069 = (((_10978 * 0.25f) * (exp2((((_11005 * _11005) * (-0.5f)) / (_11007 * _11007)) * 1.44269502162933349609375f) / (_11007 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(dot(float3(_1739, _1740, _1741), float3(_9009, _9010, _9011)), 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f)) * _11068;
                                                            float _11071 = (_11062 * (exp2((((_11029 * _11029) * (-0.5f)) / (_10989 * _10989)) * 1.44269502162933349609375f) / (_10989 * 2.5066282749176025390625f))) * exp2(_11062 * ((_10974 * 24.5258159637451171875f) + (-24.208423614501953125f)));
                                                            float _11072 = _11068 * ((_11057 * _11057) * (_11055 + 0.0465205647051334381103515625f));
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder = 0.0f;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_1 = 0.0f;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_2 = 0.0f;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_3 = 0.0f;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_4 = ((_11072 * exp2(((_1155 * (-3.2000000476837158203125f)) / _10984) * 1.44269502162933349609375f)) * _11071) + _11069;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_5 = ((_11072 * exp2(((_1157 * (-3.2000000476837158203125f)) / _10984) * 1.44269502162933349609375f)) * _11071) + _11069;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_6 = ((_11072 * exp2(((_1159 * (-3.2000000476837158203125f)) / _10984) * 1.44269502162933349609375f)) * _11071) + _11069;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_7 = 0.0f;
                                                            frontier_phi_457_444_ladder_454_ladder_469_ladder_8 = 0.0f;
                                                        }
                                                        frontier_phi_457_444_ladder_454_ladder = frontier_phi_457_444_ladder_454_ladder_469_ladder;
                                                        frontier_phi_457_444_ladder_454_ladder_1 = frontier_phi_457_444_ladder_454_ladder_469_ladder_1;
                                                        frontier_phi_457_444_ladder_454_ladder_2 = frontier_phi_457_444_ladder_454_ladder_469_ladder_2;
                                                        frontier_phi_457_444_ladder_454_ladder_3 = frontier_phi_457_444_ladder_454_ladder_469_ladder_3;
                                                        frontier_phi_457_444_ladder_454_ladder_4 = frontier_phi_457_444_ladder_454_ladder_469_ladder_4;
                                                        frontier_phi_457_444_ladder_454_ladder_5 = frontier_phi_457_444_ladder_454_ladder_469_ladder_5;
                                                        frontier_phi_457_444_ladder_454_ladder_6 = frontier_phi_457_444_ladder_454_ladder_469_ladder_6;
                                                        frontier_phi_457_444_ladder_454_ladder_7 = frontier_phi_457_444_ladder_454_ladder_469_ladder_7;
                                                        frontier_phi_457_444_ladder_454_ladder_8 = frontier_phi_457_444_ladder_454_ladder_469_ladder_8;
                                                    }
                                                    else
                                                    {
                                                        float _10476 = clamp(dot(float3(_1000, _1004, _1008), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                        float _10271;
                                                        float _10277;
                                                        float _10283;
                                                        if (_10476 > 0.0f)
                                                        {
                                                            float _11084 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                            float _11085 = _9009 + _1739;
                                                            float _11086 = _9010 + _1740;
                                                            float _11087 = _9011 + _1741;
                                                            float _11091 = rsqrt(dot(float3(_11085, _11086, _11087), float3(_11085, _11086, _11087)));
                                                            float _11092 = _11091 * _11085;
                                                            float _11093 = _11091 * _11086;
                                                            float _11094 = _11091 * _11087;
                                                            float _11098 = clamp(dot(float3(_1012, _1015, _1018), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                            float _11102 = clamp(dot(float3(_1012, _1015, _1018), float3(_11092, _11093, _11094)), 0.0f, 1.0f);
                                                            float _11495;
                                                            float _11496;
                                                            float _11497;
                                                            if (_11098 > 0.0f)
                                                            {
                                                                float _11459 = _11084 * _11084;
                                                                float _11460 = _11459 * _11459;
                                                                float _11464 = (((_11102 * _11460) - _11102) * _11102) + 1.0f;
                                                                float _11468 = _11459 * 0.5f;
                                                                float _11469 = 1.0f - _11468;
                                                                float _11476 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_11092, _11093, _11094)), 0.0f, 1.0f);
                                                                float _11477 = _11476 * _11476;
                                                                float _11479 = (_11477 * _11477) * _11476;
                                                                float _11491 = min((0.25f / (((_11098 * _11469) + _11468) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _11469) + _11468))) * (_11460 / ((_11464 * _11464) * 3.1415927410125732421875f)), _1719) * _4688;
                                                                _11495 = _11491 * ((_11479 * (1.0f - _1040)) + _1040);
                                                                _11496 = _11491 * ((_11479 * (1.0f - _1045)) + _1045);
                                                                _11497 = _11491 * ((_11479 * (1.0f - _1048)) + _1048);
                                                            }
                                                            else
                                                            {
                                                                _11495 = 0.0f;
                                                                _11496 = 0.0f;
                                                                _11497 = 0.0f;
                                                            }
                                                            float _11498 = min(_11495, 100000.0f);
                                                            float _11499 = min(_11496, 100000.0f);
                                                            float _11500 = min(_11497, 100000.0f);
                                                            float _12108;
                                                            float _12110;
                                                            float _12112;
                                                            if (_4692)
                                                            {
                                                                _12108 = _11498;
                                                                _12110 = _11499;
                                                                _12112 = _11500;
                                                            }
                                                            else
                                                            {
                                                                float _12114 = 1.0f - _10476;
                                                                float _12115 = _12114 * _12114;
                                                                float _12117 = 1.0f - (_12115 * _12115);
                                                                _12108 = _11498 * _12117;
                                                                _12110 = _11499 * _12117;
                                                                _12112 = _11500 * _12117;
                                                            }
                                                            _10271 = _12108 * _10476;
                                                            _10277 = _12110 * _10476;
                                                            _10283 = _12112 * _10476;
                                                        }
                                                        else
                                                        {
                                                            _10271 = 0.0f;
                                                            _10277 = 0.0f;
                                                            _10283 = 0.0f;
                                                        }
                                                        float _11110 = clamp(_10476, 0.0f, 1.0f);
                                                        float _11538;
                                                        if (_64_m0[76u].w > 0.5f)
                                                        {
                                                            float _11501 = _9009 + _1739;
                                                            float _11502 = _9010 + _1740;
                                                            float _11503 = _9011 + _1741;
                                                            float _11507 = rsqrt(dot(float3(_11501, _11502, _11503), float3(_11501, _11502, _11503)));
                                                            float _11514 = clamp(dot(float3(_9009, _9010, _9011), float3(_11507 * _11501, _11507 * _11502, _11507 * _11503)), 0.0f, 1.0f);
                                                            float _11524 = ((((_11514 * _11514) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                            _11538 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _11524) + 1.0f) * ((_11524 * exp2(log2(1.0f - _10476) * 5.0f)) + 1.0f);
                                                        }
                                                        else
                                                        {
                                                            _11538 = 1.0f;
                                                        }
                                                        float _11539 = 1.0f - _1145;
                                                        frontier_phi_457_444_ladder_454_ladder = 0.0f;
                                                        frontier_phi_457_444_ladder_454_ladder_1 = ((((_1025 * 0.3183098733425140380859375f) * (1.0f - _1040)) * _11539) * _11110) * _11538;
                                                        frontier_phi_457_444_ladder_454_ladder_2 = ((((_1030 * 0.3183098733425140380859375f) * (1.0f - _1045)) * _11539) * _11110) * _11538;
                                                        frontier_phi_457_444_ladder_454_ladder_3 = ((((_1035 * 0.3183098733425140380859375f) * (1.0f - _1048)) * _11539) * _11110) * _11538;
                                                        frontier_phi_457_444_ladder_454_ladder_4 = _10271;
                                                        frontier_phi_457_444_ladder_454_ladder_5 = _10277;
                                                        frontier_phi_457_444_ladder_454_ladder_6 = _10283;
                                                        frontier_phi_457_444_ladder_454_ladder_7 = 0.0f;
                                                        frontier_phi_457_444_ladder_454_ladder_8 = 0.0f;
                                                    }
                                                    frontier_phi_457_444_ladder = frontier_phi_457_444_ladder_454_ladder;
                                                    frontier_phi_457_444_ladder_1 = frontier_phi_457_444_ladder_454_ladder_1;
                                                    frontier_phi_457_444_ladder_2 = frontier_phi_457_444_ladder_454_ladder_2;
                                                    frontier_phi_457_444_ladder_3 = frontier_phi_457_444_ladder_454_ladder_3;
                                                    frontier_phi_457_444_ladder_4 = frontier_phi_457_444_ladder_454_ladder_4;
                                                    frontier_phi_457_444_ladder_5 = frontier_phi_457_444_ladder_454_ladder_5;
                                                    frontier_phi_457_444_ladder_6 = frontier_phi_457_444_ladder_454_ladder_6;
                                                    frontier_phi_457_444_ladder_7 = frontier_phi_457_444_ladder_454_ladder_7;
                                                    frontier_phi_457_444_ladder_8 = frontier_phi_457_444_ladder_454_ladder_8;
                                                }
                                                else
                                                {
                                                    float _10132 = 1.0f - _1040;
                                                    float _10133 = 1.0f - _1045;
                                                    float _10134 = 1.0f - _1048;
                                                    float _10138 = clamp(dot(float3(_1000, _1004, _1008), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                    float _10139 = clamp(_10138, 0.0f, 1.0f);
                                                    float _10515;
                                                    if (_64_m0[76u].w > 0.5f)
                                                    {
                                                        float _10478 = _9009 + _1739;
                                                        float _10479 = _9010 + _1740;
                                                        float _10480 = _9011 + _1741;
                                                        float _10484 = rsqrt(dot(float3(_10478, _10479, _10480), float3(_10478, _10479, _10480)));
                                                        float _10491 = clamp(dot(float3(_9009, _9010, _9011), float3(_10484 * _10478, _10484 * _10479, _10484 * _10480)), 0.0f, 1.0f);
                                                        float _10501 = ((((_10491 * _10491) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                        _10515 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _10501) + 1.0f) * ((_10501 * exp2(log2(1.0f - _10138) * 5.0f)) + 1.0f);
                                                    }
                                                    else
                                                    {
                                                        _10515 = 1.0f;
                                                    }
                                                    bool _10525 = _10138 > 0.0f;
                                                    float _11140;
                                                    float _11142;
                                                    float _11144;
                                                    if (_10525)
                                                    {
                                                        float _11120 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                        float _11121 = _9009 + _1739;
                                                        float _11122 = _9010 + _1740;
                                                        float _11123 = _9011 + _1741;
                                                        float _11127 = rsqrt(dot(float3(_11121, _11122, _11123), float3(_11121, _11122, _11123)));
                                                        float _11128 = _11127 * _11121;
                                                        float _11129 = _11127 * _11122;
                                                        float _11130 = _11127 * _11123;
                                                        float _11134 = clamp(dot(float3(_1012, _1015, _1018), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                        float _11138 = clamp(dot(float3(_1012, _1015, _1018), float3(_11128, _11129, _11130)), 0.0f, 1.0f);
                                                        float _11594;
                                                        float _11595;
                                                        float _11596;
                                                        if (_11134 > 0.0f)
                                                        {
                                                            float _11561 = _11120 * _11120;
                                                            float _11562 = _11561 * _11561;
                                                            float _11566 = (((_11138 * _11562) - _11138) * _11138) + 1.0f;
                                                            float _11570 = _11561 * 0.5f;
                                                            float _11571 = 1.0f - _11570;
                                                            float _11578 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_11128, _11129, _11130)), 0.0f, 1.0f);
                                                            float _11579 = _11578 * _11578;
                                                            float _11581 = (_11579 * _11579) * _11578;
                                                            float _11590 = min((0.25f / (((_11134 * _11571) + _11570) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _11571) + _11570))) * (_11562 / ((_11566 * _11566) * 3.1415927410125732421875f)), _1719) * _4688;
                                                            _11594 = _11590 * ((_11581 * _10132) + _1040);
                                                            _11595 = _11590 * ((_11581 * _10133) + _1045);
                                                            _11596 = _11590 * ((_11581 * _10134) + _1048);
                                                        }
                                                        else
                                                        {
                                                            _11594 = 0.0f;
                                                            _11595 = 0.0f;
                                                            _11596 = 0.0f;
                                                        }
                                                        float _11597 = min(_11594, 100000.0f);
                                                        float _11598 = min(_11595, 100000.0f);
                                                        float _11599 = min(_11596, 100000.0f);
                                                        float _12118;
                                                        float _12120;
                                                        float _12122;
                                                        if (_4692)
                                                        {
                                                            _12118 = _11597;
                                                            _12120 = _11598;
                                                            _12122 = _11599;
                                                        }
                                                        else
                                                        {
                                                            float _12124 = 1.0f - _10138;
                                                            float _12125 = _12124 * _12124;
                                                            float _12127 = 1.0f - (_12125 * _12125);
                                                            _12118 = _11597 * _12127;
                                                            _12120 = _11598 * _12127;
                                                            _12122 = _11599 * _12127;
                                                        }
                                                        _11140 = _12118 * _10138;
                                                        _11142 = _12120 * _10138;
                                                        _11144 = _12122 * _10138;
                                                    }
                                                    else
                                                    {
                                                        _11140 = 0.0f;
                                                        _11142 = 0.0f;
                                                        _11144 = 0.0f;
                                                    }
                                                    float _11625;
                                                    float _11627;
                                                    float _11629;
                                                    if (_10525)
                                                    {
                                                        float _11605 = max(exp2(log2(clamp(1.0f - _1088, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                        float _11606 = _9009 + _1739;
                                                        float _11607 = _9010 + _1740;
                                                        float _11608 = _9011 + _1741;
                                                        float _11612 = rsqrt(dot(float3(_11606, _11607, _11608), float3(_11606, _11607, _11608)));
                                                        float _11613 = _11612 * _11606;
                                                        float _11614 = _11612 * _11607;
                                                        float _11615 = _11612 * _11608;
                                                        float _11619 = clamp(dot(float3(_1012, _1015, _1018), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                        float _11623 = clamp(dot(float3(_1012, _1015, _1018), float3(_11613, _11614, _11615)), 0.0f, 1.0f);
                                                        float _12170;
                                                        float _12171;
                                                        float _12172;
                                                        if (_11619 > 0.0f)
                                                        {
                                                            float _12137 = _11605 * _11605;
                                                            float _12138 = _12137 * _12137;
                                                            float _12142 = (((_11623 * _12138) - _11623) * _11623) + 1.0f;
                                                            float _12146 = _12137 * 0.5f;
                                                            float _12147 = 1.0f - _12146;
                                                            float _12154 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_11613, _11614, _11615)), 0.0f, 1.0f);
                                                            float _12155 = _12154 * _12154;
                                                            float _12157 = (_12155 * _12155) * _12154;
                                                            float _12166 = min((0.25f / (((_11619 * _12147) + _12146) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _12147) + _12146))) * (_12138 / ((_12142 * _12142) * 3.1415927410125732421875f)), _1719) * _4688;
                                                            _12170 = _12166 * ((_12157 * _10132) + _1040);
                                                            _12171 = _12166 * ((_12157 * _10133) + _1045);
                                                            _12172 = _12166 * ((_12157 * _10134) + _1048);
                                                        }
                                                        else
                                                        {
                                                            _12170 = 0.0f;
                                                            _12171 = 0.0f;
                                                            _12172 = 0.0f;
                                                        }
                                                        float _12173 = min(_12170, 100000.0f);
                                                        float _12174 = min(_12171, 100000.0f);
                                                        float _12175 = min(_12172, 100000.0f);
                                                        float _13080;
                                                        float _13082;
                                                        float _13084;
                                                        if (_4692)
                                                        {
                                                            _13080 = _12173;
                                                            _13082 = _12174;
                                                            _13084 = _12175;
                                                        }
                                                        else
                                                        {
                                                            float _13086 = 1.0f - _10138;
                                                            float _13087 = _13086 * _13086;
                                                            float _13089 = 1.0f - (_13087 * _13087);
                                                            _13080 = _12173 * _13089;
                                                            _13082 = _12174 * _13089;
                                                            _13084 = _12175 * _13089;
                                                        }
                                                        _11625 = _13080 * _10138;
                                                        _11627 = _13082 * _10138;
                                                        _11629 = _13084 * _10138;
                                                    }
                                                    else
                                                    {
                                                        _11625 = 0.0f;
                                                        _11627 = 0.0f;
                                                        _11629 = 0.0f;
                                                    }
                                                    frontier_phi_457_444_ladder = 0.0f;
                                                    frontier_phi_457_444_ladder_1 = (((_1025 * 0.3183098733425140380859375f) * _10132) * _10139) * _10515;
                                                    frontier_phi_457_444_ladder_2 = (((_1030 * 0.3183098733425140380859375f) * _10133) * _10139) * _10515;
                                                    frontier_phi_457_444_ladder_3 = (((_1035 * 0.3183098733425140380859375f) * _10134) * _10139) * _10515;
                                                    frontier_phi_457_444_ladder_4 = ((_11625 - _11140) * _1090) + _11140;
                                                    frontier_phi_457_444_ladder_5 = ((_11627 - _11142) * _1090) + _11142;
                                                    frontier_phi_457_444_ladder_6 = ((_11629 - _11144) * _1090) + _11144;
                                                    frontier_phi_457_444_ladder_7 = 0.0f;
                                                    frontier_phi_457_444_ladder_8 = 0.0f;
                                                }
                                                _10254 = frontier_phi_457_444_ladder_1;
                                                _10259 = frontier_phi_457_444_ladder_2;
                                                _10264 = frontier_phi_457_444_ladder_3;
                                                _10269 = frontier_phi_457_444_ladder_4;
                                                _10275 = frontier_phi_457_444_ladder_5;
                                                _10281 = frontier_phi_457_444_ladder_6;
                                                _10287 = frontier_phi_457_444_ladder;
                                                _10289 = frontier_phi_457_444_ladder_7;
                                                _10291 = frontier_phi_457_444_ladder_8;
                                            }
                                            else
                                            {
                                                float _9771 = clamp(dot(float3(_1000, _1004, _1008), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                float frontier_phi_457_445_ladder;
                                                float frontier_phi_457_445_ladder_1;
                                                float frontier_phi_457_445_ladder_2;
                                                float frontier_phi_457_445_ladder_3;
                                                float frontier_phi_457_445_ladder_4;
                                                float frontier_phi_457_445_ladder_5;
                                                float frontier_phi_457_445_ladder_6;
                                                float frontier_phi_457_445_ladder_7;
                                                float frontier_phi_457_445_ladder_8;
                                                if (_9771 > 0.0f)
                                                {
                                                    float _10145 = _9009 + _1739;
                                                    float _10146 = _9010 + _1740;
                                                    float _10147 = _9011 + _1741;
                                                    float _10151 = rsqrt(dot(float3(_10145, _10146, _10147), float3(_10145, _10146, _10147)));
                                                    float _10152 = _10151 * _10145;
                                                    float _10153 = _10151 * _10146;
                                                    float _10154 = _10151 * _10147;
                                                    float _10159 = max(clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f), 0.001000000047497451305389404296875f);
                                                    float _10163 = clamp(dot(float3(_1012, _1015, _1018), float3(_10152, _10153, _10154)), 0.0f, 1.0f);
                                                    float _10168 = max(1.0f - _1051, 0.04500000178813934326171875f);
                                                    float _10169 = _10168 * _10168;
                                                    float _10170 = 1.0f / _10169;
                                                    float _10184 = (((_10170 + 2.0f) * 0.15915493667125701904296875f) * exp2((_10170 * 0.5f) * log2(1.0f - (_10163 * _10163)))) * (0.25f / ((_10159 + _9771) - (_10159 * _9771)));
                                                    float _10188 = _10169 * _10169;
                                                    float _10192 = (((_10188 * _10163) - _10163) * _10163) + 1.0f;
                                                    float _10196 = _10169 * 0.5f;
                                                    float _10197 = 1.0f - _10196;
                                                    float _10204 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_10152, _10153, _10154)), 0.0f, 1.0f);
                                                    float _10205 = _10204 * _10204;
                                                    float _10207 = (_10205 * _10205) * _10204;
                                                    float _10219 = min((0.25f / (((_10197 * _10159) + _10196) * ((_10197 * _9771) + _10196))) * (_10188 / ((_10192 * _10192) * 3.1415927410125732421875f)), _1719) * _4688;
                                                    float _10220 = _10219 * ((_10207 * (1.0f - _1040)) + _1040);
                                                    float _10221 = _10219 * ((_10207 * (1.0f - _1045)) + _1045);
                                                    float _10222 = _10219 * ((_10207 * (1.0f - _1048)) + _1048);
                                                    float _10226 = (1.0f - _1096) * _1025;
                                                    float _10227 = (1.0f - _1097) * _1030;
                                                    float _10228 = (1.0f - _1098) * _1035;
                                                    float _10250 = _9771 * 0.3183098733425140380859375f;
                                                    frontier_phi_457_445_ladder = 0.0f;
                                                    frontier_phi_457_445_ladder_1 = _10250 * (((_1025 - _10226) * _1099) + _10226);
                                                    frontier_phi_457_445_ladder_2 = _10250 * (((_1030 - _10227) * _1099) + _10227);
                                                    frontier_phi_457_445_ladder_3 = _10250 * (((_1035 - _10228) * _1099) + _10228);
                                                    frontier_phi_457_445_ladder_4 = ((((_10184 * _1096) - _10220) * _1099) + _10220) * _9771;
                                                    frontier_phi_457_445_ladder_5 = ((((_10184 * _1097) - _10221) * _1099) + _10221) * _9771;
                                                    frontier_phi_457_445_ladder_6 = ((((_10184 * _1098) - _10222) * _1099) + _10222) * _9771;
                                                    frontier_phi_457_445_ladder_7 = 0.0f;
                                                    frontier_phi_457_445_ladder_8 = 0.0f;
                                                }
                                                else
                                                {
                                                    frontier_phi_457_445_ladder = 0.0f;
                                                    frontier_phi_457_445_ladder_1 = 0.0f;
                                                    frontier_phi_457_445_ladder_2 = 0.0f;
                                                    frontier_phi_457_445_ladder_3 = 0.0f;
                                                    frontier_phi_457_445_ladder_4 = 0.0f;
                                                    frontier_phi_457_445_ladder_5 = 0.0f;
                                                    frontier_phi_457_445_ladder_6 = 0.0f;
                                                    frontier_phi_457_445_ladder_7 = 0.0f;
                                                    frontier_phi_457_445_ladder_8 = 0.0f;
                                                }
                                                _10254 = frontier_phi_457_445_ladder_1;
                                                _10259 = frontier_phi_457_445_ladder_2;
                                                _10264 = frontier_phi_457_445_ladder_3;
                                                _10269 = frontier_phi_457_445_ladder_4;
                                                _10275 = frontier_phi_457_445_ladder_5;
                                                _10281 = frontier_phi_457_445_ladder_6;
                                                _10287 = frontier_phi_457_445_ladder;
                                                _10289 = frontier_phi_457_445_ladder_7;
                                                _10291 = frontier_phi_457_445_ladder_8;
                                            }
                                            float _9740 = _10254 * _9441;
                                            float _9742 = _10259 * _9442;
                                            float _9744 = _10264 * _9443;
                                            float _9746 = (_10269 + _10287) * _9441;
                                            float _9748 = (_10275 + _10289) * _9442;
                                            float _9750 = (_10281 + _10291) * _9443;
                                            bool _10296 = _1086 != 0u;
                                            if (_9427)
                                            {
                                                if (!(_10296 || (_56.Load((_9425 * 28u) + 25u).x == 1u)))
                                                {
                                                    _9739 = _9740;
                                                    _9741 = _9742;
                                                    _9743 = _9744;
                                                    _9745 = _9746;
                                                    _9747 = _9748;
                                                    _9749 = _9750;
                                                    _9751 = 0.0f;
                                                    _9753 = 0.0f;
                                                    _9755 = 0.0f;
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                if (!_10296)
                                                {
                                                    _9739 = _9740;
                                                    _9741 = _9742;
                                                    _9743 = _9744;
                                                    _9745 = _9746;
                                                    _9747 = _9748;
                                                    _9749 = _9750;
                                                    _9751 = 0.0f;
                                                    _9753 = 0.0f;
                                                    _9755 = 0.0f;
                                                    break;
                                                }
                                            }
                                            float _12250;
                                            float _12254;
                                            float _12258;
                                            if (_5572)
                                            {
                                                float frontier_phi_557_525_ladder;
                                                float frontier_phi_557_525_ladder_1;
                                                float frontier_phi_557_525_ladder_2;
                                                if ((_1174 & 67108864u) == 0u)
                                                {
                                                    float frontier_phi_557_525_ladder_555_ladder;
                                                    float frontier_phi_557_525_ladder_555_ladder_1;
                                                    float frontier_phi_557_525_ladder_555_ladder_2;
                                                    if ((_1174 & 50331648u) == 0u)
                                                    {
                                                        float frontier_phi_557_525_ladder_555_ladder_580_ladder;
                                                        float frontier_phi_557_525_ladder_555_ladder_580_ladder_1;
                                                        float frontier_phi_557_525_ladder_555_ladder_580_ladder_2;
                                                        if ((_1174 & 536870912u) == 0u)
                                                        {
                                                            float _13720 = clamp((dot(float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), float3(_9009, _9010, _9011)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_1079 * 0.3183098733425140380859375f);
                                                            frontier_phi_557_525_ladder_555_ladder_580_ladder = _13720 * _1073;
                                                            frontier_phi_557_525_ladder_555_ladder_580_ladder_1 = _13720 * _1070;
                                                            frontier_phi_557_525_ladder_555_ladder_580_ladder_2 = _13720 * _1076;
                                                        }
                                                        else
                                                        {
                                                            float _13731 = clamp((dot(float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), float3(_9009, _9010, _9011)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_1079 * 0.3183098733425140380859375f);
                                                            frontier_phi_557_525_ladder_555_ladder_580_ladder = _13731 * _1073;
                                                            frontier_phi_557_525_ladder_555_ladder_580_ladder_1 = _13731 * _1070;
                                                            frontier_phi_557_525_ladder_555_ladder_580_ladder_2 = _13731 * _1076;
                                                        }
                                                        frontier_phi_557_525_ladder_555_ladder = frontier_phi_557_525_ladder_555_ladder_580_ladder;
                                                        frontier_phi_557_525_ladder_555_ladder_1 = frontier_phi_557_525_ladder_555_ladder_580_ladder_1;
                                                        frontier_phi_557_525_ladder_555_ladder_2 = frontier_phi_557_525_ladder_555_ladder_580_ladder_2;
                                                    }
                                                    else
                                                    {
                                                        float _13094 = 1.0f - _1051;
                                                        float _13095 = (-0.0f) - _962;
                                                        float _13096 = (-0.0f) - _956;
                                                        float _13097 = (-0.0f) - _950;
                                                        float _13100 = 1.0f - clamp(_1161 * 66.6666717529296875f, 0.0f, 1.0f);
                                                        float _13104 = dot(float3(_13095, _13096, _13097), float3(_9009, _9010, _9011));
                                                        float _13110 = dot(float3(_13095, _13096, _13097), float3(_1739, _1740, _1741));
                                                        float _13116 = _9009 - (_13104 * _13095);
                                                        float _13117 = _9010 - (_13104 * _13096);
                                                        float _13118 = _9011 - (_13104 * _13097);
                                                        float _13122 = _1739 - (_13110 * _13095);
                                                        float _13123 = _1740 - (_13110 * _13096);
                                                        float _13124 = _1741 - (_13110 * _13097);
                                                        float _13137 = rsqrt((dot(float3(_13122, _13123, _13124), float3(_13122, _13123, _13124)) * dot(float3(_13116, _13117, _13118), float3(_13116, _13117, _13118))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_13116, _13117, _13118), float3(_13122, _13123, _13124));
                                                        float _13139 = (_13137 * 0.5f) + 0.5f;
                                                        float _13142 = asin(_13110);
                                                        float _13143 = asin(_13104);
                                                        float _13147 = cos(abs(_13142 - _13143) * 0.5f);
                                                        float _13151 = 1.0f / ((1.190000057220458984375f / _13147) + (_13147 * 0.36000001430511474609375f));
                                                        float _13152 = _13110 * 0.645161330699920654296875f;
                                                        float _13155 = sqrt(1.0f - (_13152 * _13152));
                                                        float _13157 = max(_13094 * 0.5f, 0.00999999977648258209228515625f);
                                                        float _13160 = _13110 + (_13104 - (_1171 * 0.5f));
                                                        float _13168 = exp2((((_13160 * _13160) * (-0.5f)) / (_13157 * _13157)) * 1.44269502162933349609375f) / (_13157 * 2.5066282749176025390625f);
                                                        float _13177 = clamp((-0.0f) - dot(float3(_1739, _1740, _1741), float3(_9009, _9010, _9011)), 0.0f, 1.0f);
                                                        float _13178 = _13177 * _13177;
                                                        float _13181 = (_13178 * _13178) * (_13177 * _13100);
                                                        float _13188 = (cos(asin((_13151 * sqrt(clamp(_13139, 0.0f, 1.0f))) * ((_13151 * (0.60000002384185791015625f - (_13137 * 0.800000011920928955078125f))) + 1.0f)) * 2.0f) + 1.0f) * (-2.8853900432586669921875f);
                                                        float _13192 = exp2(_13188 * (_1155 / _13155));
                                                        float _13193 = exp2(_13188 * (_1157 / _13155));
                                                        float _13194 = exp2(_13188 * (_1159 / _13155));
                                                        float _13209 = 0.95347940921783447265625f - (exp2(log2(1.0f - _13147) * 5.0f) * 0.95347940921783447265625f);
                                                        float _13210 = _13209 * _13209;
                                                        float _13212 = (_13147 * 0.5f) + 0.5f;
                                                        float _13225 = abs(sqrt(1.0f - (_13104 * _13104))) * _15[(_1153 + 513u) + 0u].SampleLevel(_72, float2(_13212, _13139), 0.0f).x;
                                                        float _13226 = _13225 * ((_13100 * _9428) * _13168);
                                                        float _13233 = _9430 + _1161;
                                                        float _13234 = _1051 * 0.75f;
                                                        float _13238 = (((_1163 * _1163) * 9000.0f) * _1163) * _13233;
                                                        float _14125;
                                                        float _14126;
                                                        float _14127;
                                                        if ((_1174 & 33554432u) == 0u)
                                                        {
                                                            float _13732 = max(_13238, _13234);
                                                            float _13738 = _13143 + _13142;
                                                            float _13739 = _13738 * 0.5f;
                                                            float _13741 = _1155 * 0.5f;
                                                            float _13742 = _1157 * 0.5f;
                                                            float _13743 = _1159 * 0.5f;
                                                            float _13744 = max(_13094, 0.0500000007450580596923828125f);
                                                            float _13746 = (cos(_13739) * 0.5f) + 0.5f;
                                                            uint _13748 = (_1153 + 521u) + 0u;
                                                            float4 _13753 = _19[_13748].SampleLevel(_72, float3(_13746, _13744, _13741), 0.0f);
                                                            float _13755 = _13753.x;
                                                            float4 _13756 = _19[_13748].SampleLevel(_72, float3(_13746, _13744, _13742), 0.0f);
                                                            float _13758 = _13756.x;
                                                            float4 _13759 = _19[_13748].SampleLevel(_72, float3(_13746, _13744, _13743), 0.0f);
                                                            float _13761 = _13759.x;
                                                            float4 _13762 = _19[_13748].SampleLevel(_72, float3(_13212, _13744, _13741), 0.0f);
                                                            float4 _13766 = _19[_13748].SampleLevel(_72, float3(_13212, _13744, _13742), 0.0f);
                                                            float4 _13770 = _19[_13748].SampleLevel(_72, float3(_13212, _13744, _13743), 0.0f);
                                                            uint _13775 = (_1153 + 545u) + 0u;
                                                            float _13790 = (_13758 + _13755) + _13761;
                                                            float _13797 = dot(float3(max((1.0f - _1165) * _13094, 0.00999999977648258209228515625f), _13157, max(_13094 * 2.0f, 0.00999999977648258209228515625f)), float3(_13755 / _13790, _13758 / _13790, _13761 / _13790)) * _13732;
                                                            float _13808 = (_13147 * _13147) * 3.1415927410125732421875f;
                                                            float _13812 = _13797 * 0.5f;
                                                            float _13813 = _13812 + _13762.z;
                                                            float _13814 = _13812 + _13766.z;
                                                            float _13815 = _13812 + _13770.z;
                                                            float _13817 = (_13738 * (-0.25f)) * _13739;
                                                            float _13839 = ((_13762.y * 2.0f) * (exp2((_13817 / (_13813 * _13813)) * 1.44269502162933349609375f) / (_13813 * 2.5066282749176025390625f))) / _13808;
                                                            float _13840 = ((_13766.y * 2.0f) * (exp2((_13817 / (_13814 * _13814)) * 1.44269502162933349609375f) / (_13814 * 2.5066282749176025390625f))) / _13808;
                                                            float _13841 = ((_13770.y * 2.0f) * (exp2((_13817 / (_13815 * _13815)) * 1.44269502162933349609375f) / (_13815 * 2.5066282749176025390625f))) / _13808;
                                                            float _13842 = _13738 - _1171;
                                                            float _13845 = max(max(_13797, _13797), _13797) + _13157;
                                                            float _13853 = exp2((((_13842 * _13842) * (-0.125f)) / (_13845 * _13845)) * 1.44269502162933349609375f) / (_13845 * 2.5066282749176025390625f);
                                                            float _13857 = _9428 * 0.3499999940395355224609375f;
                                                            _14125 = (((exp2(log2(_13755) * _13732) * 0.4899999797344207763671875f) * ((_13853 * _15[_13775].SampleLevel(_72, float2(_13212, _13741), 0.0f).y) + (_13839 * 2.19911479949951171875f))) + (_13839 * _13857)) * _1167;
                                                            _14126 = (((exp2(log2(_13758) * _13732) * 0.4899999797344207763671875f) * ((_13853 * _15[_13775].SampleLevel(_72, float2(_13212, _13742), 0.0f).y) + (_13840 * 2.19911479949951171875f))) + (_13840 * _13857)) * _1167;
                                                            _14127 = (((exp2(log2(_13761) * _13732) * 0.4899999797344207763671875f) * ((_13853 * _15[_13775].SampleLevel(_72, float2(_13212, _13743), 0.0f).y) + (_13841 * 2.19911479949951171875f))) + (_13841 * _13857)) * _1167;
                                                        }
                                                        else
                                                        {
                                                            float _13883 = 1.0f - clamp((_13233 * 66.6666717529296875f) * max(_1163, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f);
                                                            float _13884 = _13883 * _13883;
                                                            float _13886 = (_13884 * _13884) * _13883;
                                                            float _13887 = _13210 * _13168;
                                                            float _13895 = (_13234 + 1.25f) + _13238;
                                                            float _13919 = dot(float3(_1739, _1740, _1741), float3(_13095, _13096, _13097));
                                                            float _13925 = _1739 - (_13919 * _13095);
                                                            float _13926 = _1740 - (_13919 * _13096);
                                                            float _13927 = _1741 - (_13919 * _13097);
                                                            float _13931 = rsqrt(dot(float3(_13925, _13926, _13927), float3(_13925, _13926, _13927)));
                                                            float _13939 = (dot(float3(_13925 * _13931, _13926 * _13931, _13927 * _13931), float3(_9009, _9010, _9011)) + 1.0f) * 0.25f;
                                                            float _13944 = (_1167 * 0.2228169143199920654296875f) * ((((1.0f - abs(_13104)) - _13939) * 0.3300000131130218505859375f) + _13939);
                                                            _14125 = ((_13944 * exp2(log2(exp2(((_1155 * (-3.7999999523162841796875f)) / _13147) * 1.44269502162933349609375f)) * _13895)) + ((_13225 * _13192) * _13887)) * _13886;
                                                            _14126 = ((_13944 * exp2(log2(exp2(((_1157 * (-3.7999999523162841796875f)) / _13147) * 1.44269502162933349609375f)) * _13895)) + ((_13225 * _13193) * _13887)) * _13886;
                                                            _14127 = ((_13944 * exp2(log2(exp2(((_1159 * (-3.7999999523162841796875f)) / _13147) * 1.44269502162933349609375f)) * _13895)) + ((_13225 * _13194) * _13887)) * _13886;
                                                        }
                                                        frontier_phi_557_525_ladder_555_ladder = _14126 + ((_13226 * (((1.0f - _13193) * _13181) + _13193)) * _13210);
                                                        frontier_phi_557_525_ladder_555_ladder_1 = _14125 + ((_13226 * (((1.0f - _13192) * _13181) + _13192)) * _13210);
                                                        frontier_phi_557_525_ladder_555_ladder_2 = _14127 + ((_13226 * (((1.0f - _13194) * _13181) + _13194)) * _13210);
                                                    }
                                                    frontier_phi_557_525_ladder = frontier_phi_557_525_ladder_555_ladder;
                                                    frontier_phi_557_525_ladder_1 = frontier_phi_557_525_ladder_555_ladder_1;
                                                    frontier_phi_557_525_ladder_2 = frontier_phi_557_525_ladder_555_ladder_2;
                                                }
                                                else
                                                {
                                                    uint _12178 = _1149 + 0u;
                                                    float _12179 = dot(float3(_9009, _9010, _9011), float3(_1139, _1141, _1143));
                                                    float _12194 = dot(float3(_1000, _1004, _1008), float3((_9010 * _1143) - (_9011 * _1141), (_9011 * _1139) - (_9009 * _1143), (_9009 * _1141) - (_9010 * _1139)));
                                                    float _12203 = float(int(uint(_12194 > 0.0f) - uint(_12194 < 0.0f))) * sqrt(1.0f - (_12179 * _12179));
                                                    float _12204 = _1135 + (-0.5f);
                                                    float _12205 = _1137 + (-0.5f);
                                                    float _12211 = mad(_12205, _12203, _12179 * _12204) + 0.5f;
                                                    float _12212 = mad(_12205, _12179, (-0.0f) - (_12204 * _12203)) + 0.5f;
                                                    float _12218 = (1.0f - clamp(dot(float3(_9009, _9010, _9011), float3(_1000, _1004, _1008)), 0.0f, 1.0f)) * 5.0f;
                                                    uint _12219 = uint(int(_12218));
                                                    float4 _12226 = _43[NonUniformResourceIndex(_12178)].SampleLevel(_76, float3(_12211, _12212, float(int(_12219))), 0.0f);
                                                    float _12228 = _12226.x;
                                                    float _12237 = ((_43[NonUniformResourceIndex(_12178)].SampleLevel(_76, float3(_12211, _12212, min(float(int(_12219 + 1u)), 5.0f)), 0.0f).x - _12228) * frac(_12218)) + _12228;
                                                    frontier_phi_557_525_ladder = (((_1030 * 0.3183098733425140380859375f) * _1145) * _1147) * _12237;
                                                    frontier_phi_557_525_ladder_1 = (((_1025 * 0.3183098733425140380859375f) * _1145) * _1147) * _12237;
                                                    frontier_phi_557_525_ladder_2 = (((_1035 * 0.3183098733425140380859375f) * _1145) * _1147) * _12237;
                                                }
                                                _12250 = frontier_phi_557_525_ladder_1;
                                                _12254 = frontier_phi_557_525_ladder;
                                                _12258 = frontier_phi_557_525_ladder_2;
                                            }
                                            else
                                            {
                                                float _11643 = (_9430 * (-693.14715576171875f)) * log2(max(1.0f - _1094, 1.1754943508222875079687365372222e-38f));
                                                float _11644 = _11643 * _11643;
                                                float _11646 = exp2(_11644 * (-225.4210968017578125f));
                                                float _11651 = exp2(_11644 * (-29.8077487945556640625f));
                                                float _11659 = exp2(_11644 * (-7.714946269989013671875f));
                                                float _11665 = exp2(_11644 * (-2.5444357395172119140625f));
                                                float _11667 = _11665 * 0.007000000216066837310791015625f;
                                                float _11672 = exp2(_11644 * (-0.72497236728668212890625f));
                                                float _11688 = max(dot(float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), float3(_9009, _9010, _9011)) + 0.300000011920928955078125f, 0.0f);
                                                _12250 = (((((((_11651 * 0.100000001490116119384765625f) + (_11646 * 0.23299999535083770751953125f)) + (_11659 * 0.1180000007152557373046875f)) + (_11665 * 0.112999998033046722412109375f)) + (_11672 * 0.3580000102519989013671875f)) + (exp2(_11644 * (-0.1946956813335418701171875f)) * 0.078000001609325408935546875f)) * _1025) * _11688;
                                                _12254 = ((((((_11651 * 0.3359999954700469970703125f) + (_11646 * 0.4550000131130218505859375f)) + (_11659 * 0.19799999892711639404296875f)) + _11667) + (_11672 * 0.0040000001899898052215576171875f)) * _1030) * _11688;
                                                _12258 = ((((_11651 * 0.3440000116825103759765625f) + (_11646 * 0.648999989032745361328125f)) + _11667) * _1035) * _11688;
                                            }
                                            float _12266 = (_10296 ? 1.0f : _9434) * _9428;
                                            _9739 = _9740;
                                            _9741 = _9742;
                                            _9743 = _9744;
                                            _9745 = _9746;
                                            _9747 = _9748;
                                            _9749 = _9750;
                                            _9751 = (_12250 * _9441) * _12266;
                                            _9753 = (_12254 * _9442) * _12266;
                                            _9755 = (_12258 * _9443) * _12266;
                                            break;
                                        }
                                    }
                                    frontier_phi_394_pred_393_ladder = (_9753 * _9428) + _8663;
                                    frontier_phi_394_pred_393_ladder_1 = (_9739 * _9428) + _8656;
                                    frontier_phi_394_pred_393_ladder_2 = (_9741 * _9428) + _8657;
                                    frontier_phi_394_pred_393_ladder_3 = (_9743 * _9428) + _8658;
                                    frontier_phi_394_pred_393_ladder_4 = (_9745 * _9428) + _8659;
                                    frontier_phi_394_pred_393_ladder_5 = (_9747 * _9428) + _8660;
                                    frontier_phi_394_pred_393_ladder_6 = (_9749 * _9428) + _8661;
                                    frontier_phi_394_pred_393_ladder_7 = (_9751 * _9428) + _8662;
                                    frontier_phi_394_pred_393_ladder_8 = (_9755 * _9428) + _8664;
                                }
                                else
                                {
                                    frontier_phi_394_pred_393_ladder = _8663;
                                    frontier_phi_394_pred_393_ladder_1 = _8656;
                                    frontier_phi_394_pred_393_ladder_2 = _8657;
                                    frontier_phi_394_pred_393_ladder_3 = _8658;
                                    frontier_phi_394_pred_393_ladder_4 = _8659;
                                    frontier_phi_394_pred_393_ladder_5 = _8660;
                                    frontier_phi_394_pred_393_ladder_6 = _8661;
                                    frontier_phi_394_pred_393_ladder_7 = _8662;
                                    frontier_phi_394_pred_393_ladder_8 = _8664;
                                }
                                frontier_phi_394_pred = frontier_phi_394_pred_393_ladder;
                                frontier_phi_394_pred_1 = frontier_phi_394_pred_393_ladder_1;
                                frontier_phi_394_pred_2 = frontier_phi_394_pred_393_ladder_2;
                                frontier_phi_394_pred_3 = frontier_phi_394_pred_393_ladder_3;
                                frontier_phi_394_pred_4 = frontier_phi_394_pred_393_ladder_4;
                                frontier_phi_394_pred_5 = frontier_phi_394_pred_393_ladder_5;
                                frontier_phi_394_pred_6 = frontier_phi_394_pred_393_ladder_6;
                                frontier_phi_394_pred_7 = frontier_phi_394_pred_393_ladder_7;
                                frontier_phi_394_pred_8 = frontier_phi_394_pred_393_ladder_8;
                            }
                            else
                            {
                                frontier_phi_394_pred = _8663;
                                frontier_phi_394_pred_1 = _8656;
                                frontier_phi_394_pred_2 = _8657;
                                frontier_phi_394_pred_3 = _8658;
                                frontier_phi_394_pred_4 = _8659;
                                frontier_phi_394_pred_5 = _8660;
                                frontier_phi_394_pred_6 = _8661;
                                frontier_phi_394_pred_7 = _8662;
                                frontier_phi_394_pred_8 = _8664;
                            }
                            _8513 = frontier_phi_394_pred;
                            _8499 = frontier_phi_394_pred_1;
                            _8501 = frontier_phi_394_pred_2;
                            _8503 = frontier_phi_394_pred_3;
                            _8505 = frontier_phi_394_pred_4;
                            _8507 = frontier_phi_394_pred_5;
                            _8509 = frontier_phi_394_pred_6;
                            _8511 = frontier_phi_394_pred_7;
                            _8515 = frontier_phi_394_pred_8;
                            if (_8517 > _8342)
                            {
                                break;
                            }
                            else
                            {
                                _8656 = _8499;
                                _8657 = _8501;
                                _8658 = _8503;
                                _8659 = _8505;
                                _8660 = _8507;
                                _8661 = _8509;
                                _8662 = _8511;
                                _8663 = _8513;
                                _8664 = _8515;
                                _8665 = _8517;
                                _8666 = _8669;
                                continue;
                            }
                        }
                        _8498 = _8499;
                        _8500 = _8501;
                        _8502 = _8503;
                        _8504 = _8505;
                        _8506 = _8507;
                        _8508 = _8509;
                        _8510 = _8511;
                        _8512 = _8513;
                        _8514 = _8515;
                        _8516 = _8517;
                        _8518 = _8669;
                    }
                    float _7819;
                    float _7822;
                    float _7825;
                    float _7828;
                    float _7830;
                    float _7832;
                    float _8639;
                    float _8641;
                    float _8643;
                    uint _8651;
                    uint _8653;
                    if (_8516 > _8343)
                    {
                        _8639 = _8498;
                        _8641 = _8500;
                        _8643 = _8502;
                        _7819 = _8504;
                        _7822 = _8506;
                        _7825 = _8508;
                        _7828 = _8510;
                        _7830 = _8512;
                        _7832 = _8514;
                        _8651 = _8516;
                        _8653 = _8518;
                    }
                    else
                    {
                        float _8640;
                        float _8642;
                        float _8644;
                        float _8645;
                        float _8646;
                        float _8647;
                        float _8648;
                        float _8649;
                        float _8650;
                        float _8751 = _8498;
                        float _8752 = _8500;
                        float _8753 = _8502;
                        float _8754 = _8504;
                        float _8755 = _8506;
                        float _8756 = _8508;
                        float _8757 = _8510;
                        float _8758 = _8512;
                        float _8759 = _8514;
                        uint _8760 = _8516;
                        uint _8761 = _8518;
                        uint _8652;
                        uint _8764;
                        uint _8779;
                        uint _8780;
                        float _8812;
                        float _8814;
                        float _8817;
                        float _8819;
                        uint _8821;
                        bool _8824;
                        float _8826;
                        float _8829;
                        float _8831;
                        float _8834;
                        float _8836;
                        float _8839;
                        float _8841;
                        float _8843;
                        uint _8845;
                        uint _8847;
                        uint _8848;
                        bool _8854;
                        for (;;)
                        {
                            _8652 = _8760 + 1u;
                            _8764 = _47.Load(_8760).x;
                            uint _8766 = _8761 * 4u;
                            uint4 _8778 = uint4(_48.Load(_8766).x, _48.Load(_8766 + 1u).x, _48.Load(_8766 + 2u).x, _48.Load(_8766 + 3u).x);
                            _8779 = _8778.x;
                            _8780 = _8778.y;
                            uint _8781 = _8778.z;
                            uint _8782 = _8778.w;
                            uint _8784 = _8761 * 4u;
                            uint4 _8796 = uint4(_49.Load(_8784).x, _49.Load(_8784 + 1u).x, _49.Load(_8784 + 2u).x, _49.Load(_8784 + 3u).x);
                            uint _8797 = _8796.x;
                            uint _8798 = _8796.y;
                            uint _8799 = _8796.z;
                            uint _8800 = _8796.w;
                            _8812 = spvUnpackHalf2x16(_8780 >> 16u).x;
                            _8814 = spvUnpackHalf2x16(_8781).x;
                            _8817 = spvUnpackHalf2x16(_8781 >> 16u).x;
                            _8819 = spvUnpackHalf2x16(_8782).x;
                            _8821 = (_8782 >> 16u) & 7u;
                            _8824 = (_8782 & 524288u) != 0u;
                            _8826 = spvUnpackHalf2x16(_8797).x;
                            _8829 = spvUnpackHalf2x16(_8797 >> 16u).x;
                            _8831 = spvUnpackHalf2x16(_8798).x;
                            _8834 = spvUnpackHalf2x16(_8798 >> 16u).x;
                            _8836 = spvUnpackHalf2x16(_8799).x;
                            _8839 = spvUnpackHalf2x16(_8799 >> 16u).x;
                            _8841 = spvUnpackHalf2x16(_8800).x;
                            _8843 = spvUnpackHalf2x16(uint3(_8803, _8804, _50.Load((_8761 * 4u) + 2u).x).z).x;
                            _8845 = (_8800 >> 16u) & 127u;
                            _8847 = (_8800 >> 23u) & 31u;
                            _8848 = _8800 >> 28u;
                            _8854 = (_8782 < 3221225472u) && (((_549 & 255u) & (_8782 >> 22u)) != 0u);
                            float frontier_phi_399_pred;
                            float frontier_phi_399_pred_1;
                            float frontier_phi_399_pred_2;
                            float frontier_phi_399_pred_3;
                            float frontier_phi_399_pred_4;
                            float frontier_phi_399_pred_5;
                            float frontier_phi_399_pred_6;
                            float frontier_phi_399_pred_7;
                            float frontier_phi_399_pred_8;
                            if (_8854)
                            {
                                float _8984 = spvUnpackHalf2x16(_8779).x - _8328;
                                float _8985 = spvUnpackHalf2x16(_8779 >> 16u).x - _8329;
                                float _8986 = spvUnpackHalf2x16(_8780).x - _8330;
                                float _8992 = sqrt(((_8985 * _8985) + (_8986 * _8986)) + (_8984 * _8984));
                                float _8993 = _8992 * _8812;
                                float frontier_phi_399_pred_398_ladder;
                                float frontier_phi_399_pred_398_ladder_1;
                                float frontier_phi_399_pred_398_ladder_2;
                                float frontier_phi_399_pred_398_ladder_3;
                                float frontier_phi_399_pred_398_ladder_4;
                                float frontier_phi_399_pred_398_ladder_5;
                                float frontier_phi_399_pred_398_ladder_6;
                                float frontier_phi_399_pred_398_ladder_7;
                                float frontier_phi_399_pred_398_ladder_8;
                                if (_8993 < 1.0f)
                                {
                                    float _9098 = rsqrt(dot(float3(_8984, _8985, _8986), float3(_8984, _8985, _8986)));
                                    float _9099 = _9098 * _8984;
                                    float _9100 = _9098 * _8985;
                                    float _9101 = _9098 * _8986;
                                    float _9102 = _8992 * _8992;
                                    float _9104 = (_8812 * _8812) * _9102;
                                    float _9107 = clamp(1.0f - (_9104 * _9104), 0.0f, 1.0f);
                                    float _9391;
                                    if (_8821 == 0u)
                                    {
                                        _9391 = (1.0f / (max(_9102, 9.9999997473787516355514526367188e-05f) + ((_8843 * _8843) * 0.5f))) * (_9107 * _9107);
                                    }
                                    else
                                    {
                                        _9391 = max((1.0f / dot(float3(1.0f, _8993, _8993 * _8993), float3(_69_m0[_8821 + 60u].xyz))) * (1.0f - _8993), 0.0f);
                                    }
                                    float _9392 = (-0.0f) - _8826;
                                    float _9416 = clamp((clamp(dot(float3(((_8834 * _8829) - (_8831 * _9392)) * 2.0f, ((_8831 * _8829) - (_8834 * _8826)) * 2.0f, (((_8826 * _9392) - (_8829 * _8829)) * 2.0f) + 1.0f), float3((-0.0f) - _9099, (-0.0f) - _9100, (-0.0f) - _9101)), 0.0f, 1.0f) - _8839) / (_8836 - _8839), 0.0f, 1.0f);
                                    float _9539;
                                    float _9541;
                                    float _9545;
                                    float _9547;
                                    float _9549;
                                    uint _9551;
                                    float _9552;
                                    if ((_8847 | _8845) == 0u)
                                    {
                                        _9539 = 1.0f;
                                        _9541 = 9899999600270360182784.0f;
                                        _9545 = _8814;
                                        _9547 = _8817;
                                        _9549 = _8819;
                                        _9551 = 0u;
                                        _9552 = 1.0f;
                                    }
                                    else
                                    {
                                        float _9567 = (-0.0f) - _8829;
                                        float _9568 = (-0.0f) - _8831;
                                        float _9581 = ((_8986 * _9567) - (_8985 * _9568)) + (_8984 * _8834);
                                        float _9582 = ((_8984 * _9568) - (_8986 * _9392)) + (_8985 * _8834);
                                        float _9583 = ((_8985 * _9392) - (_8984 * _9567)) + (_8986 * _8834);
                                        float _9590 = (1.0f / ((((_9582 * _9392) - (_9581 * _9567)) * 2.0f) + _8986)) * _8841;
                                        float _9546;
                                        float _9548;
                                        float _9550;
                                        if (_8847 == 0u)
                                        {
                                            _9546 = _8814;
                                            _9548 = _8817;
                                            _9550 = _8819;
                                        }
                                        else
                                        {
                                            uint _9718 = _8848 + 72u;
                                            float4 _9734 = _52.SampleLevel(_74, float3((_69_m0[_9718].x * ((_9590 * ((((_9583 * _9567) - (_9582 * _9568)) * 2.0f) + _8984)) + 0.5f)) + _69_m0[_9718].z, (_69_m0[_9718].y * (0.5f - (_9590 * ((((_9581 * _9568) - (_9583 * _9392)) * 2.0f) + _8985)))) + _69_m0[_9718].w, float(_8847 + 4294967295u)), 0.0f);
                                            _9546 = _9734.x * _8814;
                                            _9548 = _9734.y * _8817;
                                            _9550 = _9734.z * _8819;
                                        }
                                        float frontier_phi_433_441_ladder;
                                        uint frontier_phi_433_441_ladder_1;
                                        float frontier_phi_433_441_ladder_2;
                                        float frontier_phi_433_441_ladder_3;
                                        float frontier_phi_433_441_ladder_4;
                                        float frontier_phi_433_441_ladder_5;
                                        float frontier_phi_433_441_ladder_6;
                                        if (_8845 == 0u)
                                        {
                                            frontier_phi_433_441_ladder = 1.0f;
                                            frontier_phi_433_441_ladder_1 = 0u;
                                            frontier_phi_433_441_ladder_2 = _9550;
                                            frontier_phi_433_441_ladder_3 = _9548;
                                            frontier_phi_433_441_ladder_4 = _9546;
                                            frontier_phi_433_441_ladder_5 = 9899999600270360182784.0f;
                                            frontier_phi_433_441_ladder_6 = 1.0f;
                                        }
                                        else
                                        {
                                            float frontier_phi_433_441_ladder_453_ladder;
                                            uint frontier_phi_433_441_ladder_453_ladder_1;
                                            float frontier_phi_433_441_ladder_453_ladder_2;
                                            float frontier_phi_433_441_ladder_453_ladder_3;
                                            float frontier_phi_433_441_ladder_453_ladder_4;
                                            float frontier_phi_433_441_ladder_453_ladder_5;
                                            float frontier_phi_433_441_ladder_453_ladder_6;
                                            if (_1176)
                                            {
                                                uint4 _10402 = _56.Load((_8845 * 28u) + 4294967268u);
                                                uint _10403 = _10402.x;
                                                uint4 _10406 = _56.Load((_8845 * 28u) + 4294967270u);
                                                uint _10407 = _10406.x;
                                                uint _10409 = (_8845 * 28u) + 4294967273u;
                                                float3 _10419 = asfloat(uint3(_56.Load(_10409).x, _56.Load(_10409 + 1u).x, _56.Load(_10409 + 2u).x));
                                                uint _10421 = (_8845 * 28u) + 4294967276u;
                                                float3 _10431 = asfloat(uint3(_56.Load(_10421).x, _56.Load(_10421 + 1u).x, _56.Load(_10421 + 2u).x));
                                                float _10445 = asfloat(_56.Load((_8845 * 28u) + 4294967288u).x);
                                                float _10454 = asfloat(_56.Load((_8845 * 28u) + 4294967290u).x);
                                                float _9540;
                                                float _9553;
                                                float _10919;
                                                if ((_10407 < 4u) && (_64_m0[125u].y != 0.0f))
                                                {
                                                    float _10801 = float(_10407 == 0u);
                                                    float _10803 = float(_10407 == 1u);
                                                    float _10805 = float(_10407 == 2u);
                                                    float _10807 = float(_10407 == 3u);
                                                    uint _10808 = uint(_312);
                                                    uint _10809 = uint(_314);
                                                    float _10817 = dot(float4(_54.Load(int3(uint2(_10808, _10809), 0u))), float4(_10801, _10803, _10805, _10807));
                                                    float _10827 = dot(float4(_55.Load(int3(uint2(_10808, _10809), 0u))), float4(_10801, _10803, _10805, _10807));
                                                    float _10831 = (_385 - _10431.x) - _10419.x;
                                                    float _10833 = (_386 - _10431.y) - _10419.y;
                                                    float _10835 = (_387 - _10431.z) - _10419.z;
                                                    uint _10837 = _10403 * 16u;
                                                    float4 _10850 = asfloat(uint4(_53.Load(_10837).x, _53.Load(_10837 + 1u).x, _53.Load(_10837 + 2u).x, _53.Load(_10837 + 3u).x));
                                                    uint _10856 = (_10403 * 16u) + 4u;
                                                    float4 _10869 = asfloat(uint4(_53.Load(_10856).x, _53.Load(_10856 + 1u).x, _53.Load(_10856 + 2u).x, _53.Load(_10856 + 3u).x));
                                                    uint _10875 = (_10403 * 16u) + 12u;
                                                    float4 _10888 = asfloat(uint4(_53.Load(_10875).x, _53.Load(_10875 + 1u).x, _53.Load(_10875 + 2u).x, _53.Load(_10875 + 3u).x));
                                                    float _10904 = mad(_10835, _10888.z, mad(_10833, _10888.y, _10888.x * _10831)) + _10888.w;
                                                    float _10907 = ((mad(_10835, _10850.z, mad(_10833, _10850.y, _10850.x * _10831)) + _10850.w) / _10904) * 0.5f;
                                                    float _10908 = ((mad(_10835, _10869.z, mad(_10833, _10869.y, _10869.x * _10831)) + _10869.w) / _10904) * (-0.5f);
                                                    float _10909 = _10907 + 0.5f;
                                                    float _10910 = _10908 + 0.5f;
                                                    float frontier_phi_488_487_ladder;
                                                    float frontier_phi_488_487_ladder_1;
                                                    float frontier_phi_488_487_ladder_2;
                                                    if (((_10909 < 0.0f) || (_10909 > 1.0f)) || ((_10910 < 0.0f) || (_10910 > 1.0f)))
                                                    {
                                                        frontier_phi_488_487_ladder = _10827;
                                                        frontier_phi_488_487_ladder_1 = _10817;
                                                        frontier_phi_488_487_ladder_2 = 0.0f;
                                                    }
                                                    else
                                                    {
                                                        float _11984;
                                                        if (_56.Load((_8845 * 28u) + 4294967287u).x == 0u)
                                                        {
                                                            _11984 = 1.0f;
                                                        }
                                                        else
                                                        {
                                                            float _11996 = clamp(((max(abs((-0.0f) - _10907), abs((-0.0f) - _10908)) * 2.0f) - _10445) / (0.999000012874603271484375f - _10445), 0.0f, 1.0f);
                                                            _11984 = 1.0f - ((_11996 * _11996) * (3.0f - (_11996 * 2.0f)));
                                                        }
                                                        float _12744;
                                                        if (_56.Load((_8845 * 28u) + 4294967289u).x == 0u)
                                                        {
                                                            _12744 = _11984;
                                                        }
                                                        else
                                                        {
                                                            float _12752 = clamp((((-0.0f) - (_10835 * asfloat(_56.Load((_8845 * 28u) + 4294967292u).x))) - _10454) / (asfloat(_56.Load((_8845 * 28u) + 4294967291u).x) - _10454), 0.0f, 1.0f);
                                                            _12744 = (1.0f - ((_12752 * _12752) * (3.0f - (_12752 * 2.0f)))) * _11984;
                                                        }
                                                        frontier_phi_488_487_ladder = _10827;
                                                        frontier_phi_488_487_ladder_1 = _10817;
                                                        frontier_phi_488_487_ladder_2 = _12744 * (1.0f - asfloat(_56.Load((_8845 * 28u) + 4294967286u).x));
                                                    }
                                                    _9553 = frontier_phi_488_487_ladder_2;
                                                    _9540 = frontier_phi_488_487_ladder_1;
                                                    _10919 = frontier_phi_488_487_ladder;
                                                }
                                                else
                                                {
                                                    _9553 = 0.0f;
                                                    _9540 = 1.0f;
                                                    _10919 = 1.0f;
                                                }
                                                float _10925 = (_56.Load((_8845 * 28u) + 4294967293u).x == 1u) ? _9553 : 0.0f;
                                                float frontier_phi_433_441_ladder_453_ladder_488_ladder;
                                                uint frontier_phi_433_441_ladder_453_ladder_488_ladder_1;
                                                float frontier_phi_433_441_ladder_453_ladder_488_ladder_2;
                                                float frontier_phi_433_441_ladder_453_ladder_488_ladder_3;
                                                float frontier_phi_433_441_ladder_453_ladder_488_ladder_4;
                                                float frontier_phi_433_441_ladder_453_ladder_488_ladder_5;
                                                float frontier_phi_433_441_ladder_453_ladder_488_ladder_6;
                                                if ((_1174 & 1073741824u) == 0u)
                                                {
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder;
                                                    uint frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_1;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_2;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_3;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_4;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_5;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_6;
                                                    if (int(_1174) < int(0u))
                                                    {
                                                        float _9542;
                                                        if (_7030 && (_10925 > 0.0f))
                                                        {
                                                            _9542 = ((1.0f - _10919) / (1.00000095367431640625f - _9540)) * _4743;
                                                        }
                                                        else
                                                        {
                                                            _9542 = 9899999600270360182784.0f;
                                                        }
                                                        float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder;
                                                        uint frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_1;
                                                        float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_2;
                                                        float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_3;
                                                        float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_4;
                                                        float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_5;
                                                        float frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_6;
                                                        if (_10925 < 1.0f)
                                                        {
                                                            float _13582 = dot(float3(_9099, _9100, _9101), float3(_958, _952, _946));
                                                            float _13585 = dot(float3(_9099, _9100, _9101), float3(_960, _954, _948));
                                                            float _13588 = dot(float3(_9099, _9100, _9101), float3(_962, _956, _950));
                                                            float _13591 = abs(_13582);
                                                            float _13592 = abs(_13585);
                                                            float _13593 = abs(_13588);
                                                            float _13595 = (_13592 + _13591) + _13593;
                                                            float _13608 = dot(float3(_13591 / _13595, _13592 / _13595, _13593 / _13595), float3((_13582 < 0.0f) ? _970 : _964, (_13585 < 0.0f) ? _972 : _966, (_13588 < 0.0f) ? _974 : _968)) * _976;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder = _9553;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_1 = 1u;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_2 = _9550;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_3 = _9548;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_4 = _9546;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_5 = ((_9542 - _13608) * _10925) + _13608;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_6 = _9540;
                                                        }
                                                        else
                                                        {
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder = _9553;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_1 = 1u;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_2 = _9550;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_3 = _9548;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_4 = _9546;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_5 = _9542;
                                                            frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_6 = _9540;
                                                        }
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_1 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_1;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_2 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_2;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_3 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_3;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_4 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_4;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_5 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_5;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_6 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_571_ladder_6;
                                                    }
                                                    else
                                                    {
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder = _9553;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_1 = 1u;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_2 = _9550;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_3 = _9548;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_4 = _9546;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_5 = 9899999600270360182784.0f;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_6 = _9540;
                                                    }
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_1 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_1;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_2 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_2;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_3 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_3;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_4 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_4;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_5 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_5;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_6 = frontier_phi_433_441_ladder_453_ladder_488_ladder_513_ladder_6;
                                                }
                                                else
                                                {
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder;
                                                    uint frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_1;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_2;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_3;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_4;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_5;
                                                    float frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_6;
                                                    if (_10925 != 0.0f)
                                                    {
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder = _9553;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_1 = 1u;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_2 = _9550;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_3 = _9548;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_4 = _9546;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_5 = max(_1130, ((1.0f - _10919) / (1.00000095367431640625f - _9540)) * _4743);
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_6 = _9540;
                                                    }
                                                    else
                                                    {
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder = _9553;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_1 = 1u;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_2 = _9550;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_3 = _9548;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_4 = _9546;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_5 = 9899999600270360182784.0f;
                                                        frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_6 = _9540;
                                                    }
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder = frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_1 = frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_1;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_2 = frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_2;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_3 = frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_3;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_4 = frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_4;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_5 = frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_5;
                                                    frontier_phi_433_441_ladder_453_ladder_488_ladder_6 = frontier_phi_433_441_ladder_453_ladder_488_ladder_514_ladder_6;
                                                }
                                                frontier_phi_433_441_ladder_453_ladder = frontier_phi_433_441_ladder_453_ladder_488_ladder;
                                                frontier_phi_433_441_ladder_453_ladder_1 = frontier_phi_433_441_ladder_453_ladder_488_ladder_1;
                                                frontier_phi_433_441_ladder_453_ladder_2 = frontier_phi_433_441_ladder_453_ladder_488_ladder_2;
                                                frontier_phi_433_441_ladder_453_ladder_3 = frontier_phi_433_441_ladder_453_ladder_488_ladder_3;
                                                frontier_phi_433_441_ladder_453_ladder_4 = frontier_phi_433_441_ladder_453_ladder_488_ladder_4;
                                                frontier_phi_433_441_ladder_453_ladder_5 = frontier_phi_433_441_ladder_453_ladder_488_ladder_5;
                                                frontier_phi_433_441_ladder_453_ladder_6 = frontier_phi_433_441_ladder_453_ladder_488_ladder_6;
                                            }
                                            else
                                            {
                                                frontier_phi_433_441_ladder_453_ladder = 1.0f;
                                                frontier_phi_433_441_ladder_453_ladder_1 = 0u;
                                                frontier_phi_433_441_ladder_453_ladder_2 = _9550;
                                                frontier_phi_433_441_ladder_453_ladder_3 = _9548;
                                                frontier_phi_433_441_ladder_453_ladder_4 = _9546;
                                                frontier_phi_433_441_ladder_453_ladder_5 = 9899999600270360182784.0f;
                                                frontier_phi_433_441_ladder_453_ladder_6 = 1.0f;
                                            }
                                            frontier_phi_433_441_ladder = frontier_phi_433_441_ladder_453_ladder;
                                            frontier_phi_433_441_ladder_1 = frontier_phi_433_441_ladder_453_ladder_1;
                                            frontier_phi_433_441_ladder_2 = frontier_phi_433_441_ladder_453_ladder_2;
                                            frontier_phi_433_441_ladder_3 = frontier_phi_433_441_ladder_453_ladder_3;
                                            frontier_phi_433_441_ladder_4 = frontier_phi_433_441_ladder_453_ladder_4;
                                            frontier_phi_433_441_ladder_5 = frontier_phi_433_441_ladder_453_ladder_5;
                                            frontier_phi_433_441_ladder_6 = frontier_phi_433_441_ladder_453_ladder_6;
                                        }
                                        _9539 = frontier_phi_433_441_ladder_6;
                                        _9541 = frontier_phi_433_441_ladder_5;
                                        _9545 = frontier_phi_433_441_ladder_4;
                                        _9547 = frontier_phi_433_441_ladder_3;
                                        _9549 = frontier_phi_433_441_ladder_2;
                                        _9551 = frontier_phi_433_441_ladder_1;
                                        _9552 = frontier_phi_433_441_ladder;
                                    }
                                    bool _9554 = _8812 < 0.02857142873108386993408203125f;
                                    float _9561 = (((_9416 * _9416) * (3.0f - (_9416 * 2.0f))) * _9391) * max(float(_8824) * 16.0f, 1.0f);
                                    float _9562 = _9561 * _9545;
                                    float _9563 = _9561 * _9547;
                                    float _9564 = _9561 * _9549;
                                    float _10080;
                                    float _10085;
                                    float _10090;
                                    float _10095;
                                    float _10101;
                                    float _10107;
                                    float _10113;
                                    float _10115;
                                    float _10117;
                                    if ((_1174 & 4194304u) == 0u)
                                    {
                                        float frontier_phi_452_439_ladder;
                                        float frontier_phi_452_439_ladder_1;
                                        float frontier_phi_452_439_ladder_2;
                                        float frontier_phi_452_439_ladder_3;
                                        float frontier_phi_452_439_ladder_4;
                                        float frontier_phi_452_439_ladder_5;
                                        float frontier_phi_452_439_ladder_6;
                                        float frontier_phi_452_439_ladder_7;
                                        float frontier_phi_452_439_ladder_8;
                                        if ((_1174 & 8388608u) == 0u)
                                        {
                                            float frontier_phi_452_439_ladder_449_ladder;
                                            float frontier_phi_452_439_ladder_449_ladder_1;
                                            float frontier_phi_452_439_ladder_449_ladder_2;
                                            float frontier_phi_452_439_ladder_449_ladder_3;
                                            float frontier_phi_452_439_ladder_449_ladder_4;
                                            float frontier_phi_452_439_ladder_449_ladder_5;
                                            float frontier_phi_452_439_ladder_449_ladder_6;
                                            float frontier_phi_452_439_ladder_449_ladder_7;
                                            float frontier_phi_452_439_ladder_449_ladder_8;
                                            if ((_1174 & 67108864u) == 0u)
                                            {
                                                float frontier_phi_452_439_ladder_449_ladder_462_ladder;
                                                float frontier_phi_452_439_ladder_449_ladder_462_ladder_1;
                                                float frontier_phi_452_439_ladder_449_ladder_462_ladder_2;
                                                float frontier_phi_452_439_ladder_449_ladder_462_ladder_3;
                                                float frontier_phi_452_439_ladder_449_ladder_462_ladder_4;
                                                float frontier_phi_452_439_ladder_449_ladder_462_ladder_5;
                                                float frontier_phi_452_439_ladder_449_ladder_462_ladder_6;
                                                float frontier_phi_452_439_ladder_449_ladder_462_ladder_7;
                                                float frontier_phi_452_439_ladder_449_ladder_462_ladder_8;
                                                if ((_1174 & 50331648u) == 0u)
                                                {
                                                    float frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder;
                                                    float frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_1;
                                                    float frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_2;
                                                    float frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_3;
                                                    float frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_4;
                                                    float frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_5;
                                                    float frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_6;
                                                    float frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_7;
                                                    float frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_8;
                                                    if ((_1174 & 1032192u) == 0u)
                                                    {
                                                        float _10084;
                                                        float _10089;
                                                        float _10094;
                                                        float _10114;
                                                        float _10116;
                                                        float _10118;
                                                        if ((_1104 > 0.0f) && ((_1174 & 3u) != 0u))
                                                        {
                                                            float _11727 = max(max(max(max(_1107, 0.0f), _1104), _1109), 0.0f);
                                                            float _11733 = _64_m0[8u].x - _997;
                                                            float _11734 = _64_m0[8u].y - _998;
                                                            float _11735 = _64_m0[8u].z - _999;
                                                            float _11746 = clamp(2.0f - (clamp(sqrt(((_11733 * _11733) + (_11734 * _11734)) + (_11735 * _11735)) * 0.02500000037252902984619140625f, 0.0f, 1.0f) * 2.0f), 0.0f, 1.0f);
                                                            float _11747 = _11746 * _11727;
                                                            float _11753 = _64_m0[21u].x - _997;
                                                            float _11754 = _64_m0[21u].y - _998;
                                                            float _11755 = _64_m0[21u].z - _999;
                                                            float _11762 = log2(sqrt(((_11753 * _11753) + (_11754 * _11754)) + (_11755 * _11755)));
                                                            float _11763 = _11762 * 0.85000002384185791015625f;
                                                            float _12433;
                                                            float _12434;
                                                            float _12435;
                                                            float _12436;
                                                            if (_11747 > 0.00999999977648258209228515625f)
                                                            {
                                                                uint _12267 = _1106 + 0u;
                                                                float _12268 = ceil(_11763);
                                                                float _12272 = _997 * 5.0f;
                                                                float _12273 = _999 * 5.0f;
                                                                float _12275 = exp2((-0.0f) - max(1.0f, _12268));
                                                                float _12281 = exp2((-0.0f) - max(1.0f, _12268 + 1.0f));
                                                                float4 _12290 = _39[NonUniformResourceIndex(_12267)].SampleLevel(_77, float2(frac(_12275 * _12272), frac(_12275 * _12273)), 0.0f);
                                                                float4 _12295 = _39[NonUniformResourceIndex(_12267)].SampleLevel(_77, float2(frac(_12281 * _12272), frac(_12281 * _12273)), 0.0f);
                                                                float _12300 = frac(_11763);
                                                                float _12302 = (_12300 + 0.5f) * 0.5f;
                                                                float _12314 = (_12290.x + (-0.5f)) * 2.0f;
                                                                float _12315 = (_12290.y + (-0.5f)) * 2.0f;
                                                                float _12321 = sqrt(clamp((1.0f - (_12314 * _12314)) - (_12315 * _12315), 0.0f, 1.0f));
                                                                float _12328 = (_12295.x + (-0.5f)) * 2.0f;
                                                                float _12329 = (_12295.y + (-0.5f)) * 2.0f;
                                                                float _12335 = sqrt(clamp((1.0f - (_12328 * _12328)) - (_12329 * _12329), 0.0f, 1.0f));
                                                                float _12341 = rsqrt(dot(float3(_12314, _12321, _12315), float3(_12314, _12321, _12315))) * (1.0f - _12300);
                                                                float _12344 = rsqrt(dot(float3(_12328, _12335, _12329), float3(_12328, _12335, _12329))) * _12300;
                                                                float _12347 = (_12344 * _12328) + (_12341 * _12314);
                                                                float _12348 = (_12344 * _12329) + (_12341 * _12315);
                                                                float _12352 = rsqrt(dot(float3(_12347, _12348, 1.0f), float3(_12347, _12348, 1.0f)));
                                                                float _12355 = mad(_12352, _1000, _12347 * _12352);
                                                                float _12356 = mad(_12352, _1004, 0.0f);
                                                                float _12357 = mad(_12352, _1008, _12348 * _12352);
                                                                float _12361 = rsqrt(dot(float3(_12355, _12356, _12357), float3(_12355, _12356, _12357)));
                                                                float _12362 = _12361 * _12355;
                                                                float _12363 = _12361 * _12356;
                                                                float _12364 = _12361 * _12357;
                                                                float _12365 = _1740 + 1.0f;
                                                                float _12369 = rsqrt(dot(float3(_1739, _12365, _1741), float3(_1739, _12365, _1741)));
                                                                float _12412 = (((((_11747 * 2.2000000476837158203125f) * max(clamp(_12302 * (_12290.z - _12300), 0.0f, 1.0f), clamp((1.0f - _12302) * ((_12300 + (-1.0f)) + _12295.z), 0.0f, 1.0f))) * exp2((800.0f - (clamp(clamp((_11762 * 0.425000011920928955078125f) + (-1.0f), 0.0f, 1.0f), 0.0f, 1.0f) * 800.0f)) * log2(((_11727 * (0.004999999888241291046142578125f - (_1109 * 0.00299999979324638843536376953125f))) + 0.00200000009499490261077880859375f) + max(dot(float3(_12369 * _1739, _12369 * _12365, _12369 * _1741), float3(_12362, _12363, _12364)), 0.0f)))) * exp2(log2(clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 1.2000000476837158203125f)) * clamp((dot(float3(_9099, _9100, _9101), float3(_1000, _1004, _1008)) * 10.0f) + 0.5f, 0.0f, 1.0f)) * _64_m0[166u].y;
                                                                float _12414 = (_1109 * 0.4000000059604644775390625f) * _11746;
                                                                float _12417 = ((_12364 + (-1.0f)) * _12414) + 1.0f;
                                                                float _12420 = _12414 * _1008;
                                                                float _12423 = (_12417 * _1000) + (_12362 * _12420);
                                                                float _12424 = (_12417 * _1004) + (_12363 * _12420);
                                                                float _12425 = _12417 * _1008;
                                                                float _12429 = rsqrt(dot(float3(_12423, _12424, _12425), float3(_12423, _12424, _12425)));
                                                                _12433 = _12412;
                                                                _12434 = _12429 * _12423;
                                                                _12435 = _12429 * _12424;
                                                                _12436 = _12429 * _12425;
                                                            }
                                                            else
                                                            {
                                                                _12433 = 0.0f;
                                                                _12434 = _1000;
                                                                _12435 = _1004;
                                                                _12436 = _1008;
                                                            }
                                                            float _12441 = dot(float3(_12434, _12435, _12436), float3(_9099, _9100, _9101));
                                                            float _12447 = (1.0f - clamp(_12441 * 5.0f, 0.0f, 1.0f)) * _1107;
                                                            float _12454 = (_12447 * (_1025 - _1111)) + _1111;
                                                            float _12455 = (_12447 * (_1030 - _1113)) + _1113;
                                                            float _12456 = (_12447 * (_1035 - _1115)) + _1115;
                                                            float _12457 = 1.0f - _1040;
                                                            float _12458 = 1.0f - _1045;
                                                            float _12459 = 1.0f - _1048;
                                                            float _12467 = exp2(log2(1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_12434, _12435, _12436)), 0.0f, 1.0f)) * 5.0f);
                                                            float _12474 = _1109 * 0.550000011920928955078125f;
                                                            float _12475 = ((_12467 * _12457) + _1040) * _12474;
                                                            float _12476 = ((_12467 * _12458) + _1045) * _12474;
                                                            float _12477 = ((_12467 * _12459) + _1048) * _12474;
                                                            float _12496 = clamp(_12441, 0.0f, 1.0f);
                                                            float _13276;
                                                            if (_64_m0[76u].w > 0.5f)
                                                            {
                                                                float _13239 = _9099 + _1739;
                                                                float _13240 = _9100 + _1740;
                                                                float _13241 = _9101 + _1741;
                                                                float _13245 = rsqrt(dot(float3(_13239, _13240, _13241), float3(_13239, _13240, _13241)));
                                                                float _13252 = clamp(dot(float3(_9099, _9100, _9101), float3(_13245 * _13239, _13245 * _13240, _13245 * _13241)), 0.0f, 1.0f);
                                                                float _13262 = ((((_13252 * _13252) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                                _13276 = ((exp2(log2(1.0f - clamp(dot(float3(_12434, _12435, _12436), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _13262) + 1.0f) * ((_13262 * exp2(log2(1.0f - _12496) * 5.0f)) + 1.0f);
                                                            }
                                                            else
                                                            {
                                                                _13276 = 1.0f;
                                                            }
                                                            float _13278 = _13276 * (clamp((_12496 * 0.722500026226043701171875f) + 0.12750001251697540283203125f, 0.0f, 1.0f) * 0.3183098733425140380859375f);
                                                            _10084 = (_13278 * _12457) * (((_12475 * _12475) * ((0.949999988079071044921875f - (_1107 * 0.5f)) - _12454)) + _12454);
                                                            _10089 = (_13278 * _12458) * (((_12476 * _12476) * ((0.810000002384185791015625f - (_1107 * 0.069999992847442626953125f)) - _12455)) + _12455);
                                                            _10094 = (_13278 * _12459) * (((_12477 * _12477) * (((_1107 * 0.37999999523162841796875f) + 0.569999992847442626953125f) - _12456)) + _12456);
                                                            _10114 = _12433 * 0.449999988079071044921875f;
                                                            _10116 = _12433 * 0.449999988079071044921875f;
                                                            _10118 = _12433 * 0.449999988079071044921875f;
                                                        }
                                                        else
                                                        {
                                                            float _11771 = clamp(dot(float3(_1000, _1004, _1008), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                            float _11772 = clamp(_11771, 0.0f, 1.0f);
                                                            float _12541;
                                                            if (_64_m0[76u].w > 0.5f)
                                                            {
                                                                float _12504 = _9099 + _1739;
                                                                float _12505 = _9100 + _1740;
                                                                float _12506 = _9101 + _1741;
                                                                float _12510 = rsqrt(dot(float3(_12504, _12505, _12506), float3(_12504, _12505, _12506)));
                                                                float _12517 = clamp(dot(float3(_9099, _9100, _9101), float3(_12510 * _12504, _12510 * _12505, _12510 * _12506)), 0.0f, 1.0f);
                                                                float _12527 = ((((_12517 * _12517) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                                _12541 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _12527) + 1.0f) * ((_12527 * exp2(log2(1.0f - _11771) * 5.0f)) + 1.0f);
                                                            }
                                                            else
                                                            {
                                                                _12541 = 1.0f;
                                                            }
                                                            _10084 = (((_1025 * 0.3183098733425140380859375f) * (1.0f - _1040)) * _11772) * _12541;
                                                            _10089 = (((_1030 * 0.3183098733425140380859375f) * (1.0f - _1045)) * _11772) * _12541;
                                                            _10094 = (((_1035 * 0.3183098733425140380859375f) * (1.0f - _1048)) * _11772) * _12541;
                                                            _10114 = 0.0f;
                                                            _10116 = 0.0f;
                                                            _10118 = 0.0f;
                                                        }
                                                        float _13288 = clamp(dot(float3(_1012, _1015, _1018), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                        float _14022;
                                                        float _14024;
                                                        float _14026;
                                                        if (_13288 > 0.0f)
                                                        {
                                                            float _13959 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                            float _13960 = _9099 + _1739;
                                                            float _13961 = _9100 + _1740;
                                                            float _13962 = _9101 + _1741;
                                                            float _13966 = rsqrt(dot(float3(_13960, _13961, _13962), float3(_13960, _13961, _13962)));
                                                            float _13967 = _13966 * _13960;
                                                            float _13968 = _13966 * _13961;
                                                            float _13969 = _13966 * _13962;
                                                            float _13978 = clamp(dot(float3(_1012, _1015, _1018), float3(_13967, _13968, _13969)), 0.0f, 1.0f);
                                                            float _13983 = _13959 * _13959;
                                                            float _13984 = _13983 * _13983;
                                                            float _13988 = (((_13978 * _13984) - _13978) * _13978) + 1.0f;
                                                            float _13992 = _13983 * 0.5f;
                                                            float _13993 = 1.0f - _13992;
                                                            float _14000 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_13967, _13968, _13969)), 0.0f, 1.0f);
                                                            float _14001 = _14000 * _14000;
                                                            float _14003 = (_14001 * _14001) * _14000;
                                                            float _14015 = min((0.25f / (((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _13993) + _13992) * ((_13993 * _13288) + _13992))) * (_13984 / ((_13988 * _13988) * 3.1415927410125732421875f)), _1719) * _4688;
                                                            float _14019 = min(_14015 * ((_14003 * (1.0f - _1040)) + _1040), 100000.0f);
                                                            float _14020 = min(_14015 * ((_14003 * (1.0f - _1045)) + _1045), 100000.0f);
                                                            float _14021 = min(_14015 * ((_14003 * (1.0f - _1048)) + _1048), 100000.0f);
                                                            float _14128;
                                                            float _14130;
                                                            float _14132;
                                                            if (_4692)
                                                            {
                                                                _14128 = _14019;
                                                                _14130 = _14020;
                                                                _14132 = _14021;
                                                            }
                                                            else
                                                            {
                                                                float _14134 = 1.0f - _13288;
                                                                float _14135 = _14134 * _14134;
                                                                float _14137 = 1.0f - (_14135 * _14135);
                                                                _14128 = _14019 * _14137;
                                                                _14130 = _14020 * _14137;
                                                                _14132 = _14021 * _14137;
                                                            }
                                                            _14022 = _14128 * _13288;
                                                            _14024 = _14130 * _13288;
                                                            _14026 = _14132 * _13288;
                                                        }
                                                        else
                                                        {
                                                            _14022 = 0.0f;
                                                            _14024 = 0.0f;
                                                            _14026 = 0.0f;
                                                        }
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder = _10089;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_1 = _10084;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_2 = _10094;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_3 = _14022 * _1065;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_4 = _14024 * _1065;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_5 = _10114;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_6 = _10116;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_7 = _10118;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_8 = _14026 * _1065;
                                                    }
                                                    else
                                                    {
                                                        float _11169 = _9554 ? _69_m0[104u].w : 0.0f;
                                                        float _11170 = 1.0f - _1040;
                                                        float _11171 = 1.0f - _1045;
                                                        float _11172 = 1.0f - _1048;
                                                        float _11176 = clamp(dot(float3(_1000, _1004, _1008), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                        float _11177 = 1.0f - _11169;
                                                        float _11181 = clamp(((_11176 * _11177) + _11169) * _11177, 0.0f, 1.0f);
                                                        float _11814;
                                                        if (_64_m0[76u].w > 0.5f)
                                                        {
                                                            float _11777 = _9099 + _1739;
                                                            float _11778 = _9100 + _1740;
                                                            float _11779 = _9101 + _1741;
                                                            float _11783 = rsqrt(dot(float3(_11777, _11778, _11779), float3(_11777, _11778, _11779)));
                                                            float _11790 = clamp(dot(float3(_9099, _9100, _9101), float3(_11783 * _11777, _11783 * _11778, _11783 * _11779)), 0.0f, 1.0f);
                                                            float _11800 = ((((_11790 * _11790) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                            _11814 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _11800) + 1.0f) * ((_11800 * exp2(log2(1.0f - _11176) * 5.0f)) + 1.0f);
                                                        }
                                                        else
                                                        {
                                                            _11814 = 1.0f;
                                                        }
                                                        float _12579;
                                                        float _12581;
                                                        float _12583;
                                                        if (_11176 > 0.0f)
                                                        {
                                                            float _12559 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                            float _12560 = _9099 + _1739;
                                                            float _12561 = _9100 + _1740;
                                                            float _12562 = _9101 + _1741;
                                                            float _12566 = rsqrt(dot(float3(_12560, _12561, _12562), float3(_12560, _12561, _12562)));
                                                            float _12567 = _12566 * _12560;
                                                            float _12568 = _12566 * _12561;
                                                            float _12569 = _12566 * _12562;
                                                            float _12573 = clamp(dot(float3(_1012, _1015, _1018), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                            float _12577 = clamp(dot(float3(_1012, _1015, _1018), float3(_12567, _12568, _12569)), 0.0f, 1.0f);
                                                            float _13332;
                                                            float _13333;
                                                            float _13334;
                                                            if (_12573 > 0.0f)
                                                            {
                                                                float _13299 = _12559 * _12559;
                                                                float _13300 = _13299 * _13299;
                                                                float _13304 = (((_12577 * _13300) - _12577) * _12577) + 1.0f;
                                                                float _13308 = _13299 * 0.5f;
                                                                float _13309 = 1.0f - _13308;
                                                                float _13316 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_12567, _12568, _12569)), 0.0f, 1.0f);
                                                                float _13317 = _13316 * _13316;
                                                                float _13319 = (_13317 * _13317) * _13316;
                                                                float _13328 = min((0.25f / (((_12573 * _13309) + _13308) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _13309) + _13308))) * (_13300 / ((_13304 * _13304) * 3.1415927410125732421875f)), _1719) * _4688;
                                                                _13332 = _13328 * ((_13319 * _11170) + _1040);
                                                                _13333 = _13328 * ((_13319 * _11171) + _1045);
                                                                _13334 = _13328 * ((_13319 * _11172) + _1048);
                                                            }
                                                            else
                                                            {
                                                                _13332 = 0.0f;
                                                                _13333 = 0.0f;
                                                                _13334 = 0.0f;
                                                            }
                                                            float _13335 = min(_13332, 100000.0f);
                                                            float _13336 = min(_13333, 100000.0f);
                                                            float _13337 = min(_13334, 100000.0f);
                                                            float _14028;
                                                            float _14030;
                                                            float _14032;
                                                            if (_4692)
                                                            {
                                                                _14028 = _13335;
                                                                _14030 = _13336;
                                                                _14032 = _13337;
                                                            }
                                                            else
                                                            {
                                                                float _14034 = 1.0f - _11176;
                                                                float _14035 = _14034 * _14034;
                                                                float _14037 = 1.0f - (_14035 * _14035);
                                                                _14028 = _13335 * _14037;
                                                                _14030 = _13336 * _14037;
                                                                _14032 = _13337 * _14037;
                                                            }
                                                            _12579 = _14028 * _11176;
                                                            _12581 = _14030 * _11176;
                                                            _12583 = _14032 * _11176;
                                                        }
                                                        else
                                                        {
                                                            _12579 = 0.0f;
                                                            _12581 = 0.0f;
                                                            _12583 = 0.0f;
                                                        }
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder = (((_1030 * 0.3183098733425140380859375f) * _11171) * _11181) * _11814;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_1 = (((_1025 * 0.3183098733425140380859375f) * _11170) * _11181) * _11814;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_2 = (((_1035 * 0.3183098733425140380859375f) * _11172) * _11181) * _11814;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_3 = _12579 * _1065;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_4 = _12581 * _1065;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_5 = 0.0f;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_6 = 0.0f;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_7 = 0.0f;
                                                        frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_8 = _12583 * _1065;
                                                    }
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder = frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_1 = frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_1;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_2 = frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_2;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_3 = frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_3;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_4 = frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_4;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_5 = frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_5;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_6 = frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_6;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_7 = frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_7;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_8 = frontier_phi_452_439_ladder_449_ladder_462_ladder_479_ladder_8;
                                                }
                                                else
                                                {
                                                    float _10558 = 1.0f - _1051;
                                                    float _10559 = (-0.0f) - _962;
                                                    float _10560 = (-0.0f) - _956;
                                                    float _10561 = (-0.0f) - _950;
                                                    float _10569 = dot(float3(_10559, _10560, _10561), float3(_9099, _9100, _9101));
                                                    float _10575 = dot(float3(_10559, _10560, _10561), float3(_1739, _1740, _1741));
                                                    float _10581 = _9099 - (_10569 * _10559);
                                                    float _10582 = _9100 - (_10569 * _10560);
                                                    float _10583 = _9101 - (_10569 * _10561);
                                                    float _10587 = _1739 - (_10575 * _10559);
                                                    float _10588 = _1740 - (_10575 * _10560);
                                                    float _10589 = _1741 - (_10575 * _10561);
                                                    float _10602 = rsqrt((dot(float3(_10587, _10588, _10589), float3(_10587, _10588, _10589)) * dot(float3(_10581, _10582, _10583), float3(_10581, _10582, _10583))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_10581, _10582, _10583), float3(_10587, _10588, _10589));
                                                    float _10606 = sqrt(clamp((_10602 * 0.5f) + 0.5f, 0.0f, 1.0f));
                                                    float _10612 = cos(abs(asin(_10575) - asin(_10569)) * 0.5f);
                                                    float _10617 = max(_10558 * 2.0f, 0.00999999977648258209228515625f);
                                                    float _10619 = (-0.0f) - _1171;
                                                    float _10621 = sin(_10619);
                                                    float _10632 = _10575 + _10569;
                                                    float _10633 = _10632 - ((_10621 * 2.0f) * (((cos(_10619) * _10606) * sqrt(1.0f - (_10575 * _10575))) + (_10621 * _10575)));
                                                    float _10635 = (_10606 * 1.41421353816986083984375f) * max((1.0f - _1165) * _10558, 0.00999999977648258209228515625f);
                                                    float _10657 = _10632 - (_1171 * 1.5f);
                                                    float _10683 = exp2(log2(1.0f - (_10612 * 0.5f)) * 5.0f) * 0.95347940921783447265625f;
                                                    float _10685 = 0.95347940921783447265625f - _10683;
                                                    float _10690 = clamp((1.0f - max(_1169, 0.01200000010430812835693359375f)) * 1.5f, 0.0f, 1.0f);
                                                    float _10696 = abs(sqrt(1.0f - (_10569 * _10569))) * clamp(dot(float3(_1000, _1004, _1008), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                    float _10697 = (((_10606 * 0.25f) * (exp2((((_10633 * _10633) * (-0.5f)) / (_10635 * _10635)) * 1.44269502162933349609375f) / (_10635 * 2.5066282749176025390625f))) * ((exp2(log2(1.0f - sqrt((clamp(dot(float3(_1739, _1740, _1741), float3(_9099, _9100, _9101)), 0.0f, 1.0f) * 0.5f) + 0.5f)) * 5.0f) * 0.95347940921783447265625f) + 0.0465205647051334381103515625f)) * _10696;
                                                    float _10699 = (_10690 * (exp2((((_10657 * _10657) * (-0.5f)) / (_10617 * _10617)) * 1.44269502162933349609375f) / (_10617 * 2.5066282749176025390625f))) * exp2(_10690 * ((_10602 * 24.5258159637451171875f) + (-24.208423614501953125f)));
                                                    float _10700 = _10696 * ((_10685 * _10685) * (_10683 + 0.0465205647051334381103515625f));
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder = 0.0f;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_1 = 0.0f;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_2 = 0.0f;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_3 = ((_10700 * exp2(((_1155 * (-3.2000000476837158203125f)) / _10612) * 1.44269502162933349609375f)) * _10699) + _10697;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_4 = ((_10700 * exp2(((_1157 * (-3.2000000476837158203125f)) / _10612) * 1.44269502162933349609375f)) * _10699) + _10697;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_5 = 0.0f;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_6 = 0.0f;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_7 = 0.0f;
                                                    frontier_phi_452_439_ladder_449_ladder_462_ladder_8 = ((_10700 * exp2(((_1159 * (-3.2000000476837158203125f)) / _10612) * 1.44269502162933349609375f)) * _10699) + _10697;
                                                }
                                                frontier_phi_452_439_ladder_449_ladder = frontier_phi_452_439_ladder_449_ladder_462_ladder;
                                                frontier_phi_452_439_ladder_449_ladder_1 = frontier_phi_452_439_ladder_449_ladder_462_ladder_1;
                                                frontier_phi_452_439_ladder_449_ladder_2 = frontier_phi_452_439_ladder_449_ladder_462_ladder_2;
                                                frontier_phi_452_439_ladder_449_ladder_3 = frontier_phi_452_439_ladder_449_ladder_462_ladder_3;
                                                frontier_phi_452_439_ladder_449_ladder_4 = frontier_phi_452_439_ladder_449_ladder_462_ladder_4;
                                                frontier_phi_452_439_ladder_449_ladder_5 = frontier_phi_452_439_ladder_449_ladder_462_ladder_5;
                                                frontier_phi_452_439_ladder_449_ladder_6 = frontier_phi_452_439_ladder_449_ladder_462_ladder_6;
                                                frontier_phi_452_439_ladder_449_ladder_7 = frontier_phi_452_439_ladder_449_ladder_462_ladder_7;
                                                frontier_phi_452_439_ladder_449_ladder_8 = frontier_phi_452_439_ladder_449_ladder_462_ladder_8;
                                            }
                                            else
                                            {
                                                float _10342 = clamp(dot(float3(_1000, _1004, _1008), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                float _10097;
                                                float _10103;
                                                float _10109;
                                                if (_10342 > 0.0f)
                                                {
                                                    float _10712 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                    float _10713 = _9099 + _1739;
                                                    float _10714 = _9100 + _1740;
                                                    float _10715 = _9101 + _1741;
                                                    float _10719 = rsqrt(dot(float3(_10713, _10714, _10715), float3(_10713, _10714, _10715)));
                                                    float _10720 = _10719 * _10713;
                                                    float _10721 = _10719 * _10714;
                                                    float _10722 = _10719 * _10715;
                                                    float _10726 = clamp(dot(float3(_1012, _1015, _1018), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                    float _10730 = clamp(dot(float3(_1012, _1015, _1018), float3(_10720, _10721, _10722)), 0.0f, 1.0f);
                                                    float _11231;
                                                    float _11232;
                                                    float _11233;
                                                    if (_10726 > 0.0f)
                                                    {
                                                        float _11195 = _10712 * _10712;
                                                        float _11196 = _11195 * _11195;
                                                        float _11200 = (((_10730 * _11196) - _10730) * _10730) + 1.0f;
                                                        float _11204 = _11195 * 0.5f;
                                                        float _11205 = 1.0f - _11204;
                                                        float _11212 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_10720, _10721, _10722)), 0.0f, 1.0f);
                                                        float _11213 = _11212 * _11212;
                                                        float _11215 = (_11213 * _11213) * _11212;
                                                        float _11227 = min((0.25f / (((_10726 * _11205) + _11204) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _11205) + _11204))) * (_11196 / ((_11200 * _11200) * 3.1415927410125732421875f)), _1719) * _4688;
                                                        _11231 = _11227 * ((_11215 * (1.0f - _1040)) + _1040);
                                                        _11232 = _11227 * ((_11215 * (1.0f - _1045)) + _1045);
                                                        _11233 = _11227 * ((_11215 * (1.0f - _1048)) + _1048);
                                                    }
                                                    else
                                                    {
                                                        _11231 = 0.0f;
                                                        _11232 = 0.0f;
                                                        _11233 = 0.0f;
                                                    }
                                                    float _11234 = min(_11231, 100000.0f);
                                                    float _11235 = min(_11232, 100000.0f);
                                                    float _11236 = min(_11233, 100000.0f);
                                                    float _11825;
                                                    float _11827;
                                                    float _11829;
                                                    if (_4692)
                                                    {
                                                        _11825 = _11234;
                                                        _11827 = _11235;
                                                        _11829 = _11236;
                                                    }
                                                    else
                                                    {
                                                        float _11831 = 1.0f - _10342;
                                                        float _11832 = _11831 * _11831;
                                                        float _11834 = 1.0f - (_11832 * _11832);
                                                        _11825 = _11234 * _11834;
                                                        _11827 = _11235 * _11834;
                                                        _11829 = _11236 * _11834;
                                                    }
                                                    _10097 = _11825 * _10342;
                                                    _10103 = _11827 * _10342;
                                                    _10109 = _11829 * _10342;
                                                }
                                                else
                                                {
                                                    _10097 = 0.0f;
                                                    _10103 = 0.0f;
                                                    _10109 = 0.0f;
                                                }
                                                float _10735 = _9554 ? _69_m0[104u].w : 0.0f;
                                                float _10739 = 1.0f - _10735;
                                                float _10743 = clamp(((_10342 * _10739) + _10735) * _10739, 0.0f, 1.0f);
                                                float _11274;
                                                if (_64_m0[76u].w > 0.5f)
                                                {
                                                    float _11237 = _9099 + _1739;
                                                    float _11238 = _9100 + _1740;
                                                    float _11239 = _9101 + _1741;
                                                    float _11243 = rsqrt(dot(float3(_11237, _11238, _11239), float3(_11237, _11238, _11239)));
                                                    float _11250 = clamp(dot(float3(_9099, _9100, _9101), float3(_11243 * _11237, _11243 * _11238, _11243 * _11239)), 0.0f, 1.0f);
                                                    float _11260 = ((((_11250 * _11250) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                    _11274 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _11260) + 1.0f) * ((_11260 * exp2(log2(1.0f - _10342) * 5.0f)) + 1.0f);
                                                }
                                                else
                                                {
                                                    _11274 = 1.0f;
                                                }
                                                float _11275 = 1.0f - _1145;
                                                frontier_phi_452_439_ladder_449_ladder = ((((_1030 * 0.3183098733425140380859375f) * (1.0f - _1045)) * _11275) * _10743) * _11274;
                                                frontier_phi_452_439_ladder_449_ladder_1 = ((((_1025 * 0.3183098733425140380859375f) * (1.0f - _1040)) * _11275) * _10743) * _11274;
                                                frontier_phi_452_439_ladder_449_ladder_2 = ((((_1035 * 0.3183098733425140380859375f) * (1.0f - _1048)) * _11275) * _10743) * _11274;
                                                frontier_phi_452_439_ladder_449_ladder_3 = _10097;
                                                frontier_phi_452_439_ladder_449_ladder_4 = _10103;
                                                frontier_phi_452_439_ladder_449_ladder_5 = 0.0f;
                                                frontier_phi_452_439_ladder_449_ladder_6 = 0.0f;
                                                frontier_phi_452_439_ladder_449_ladder_7 = 0.0f;
                                                frontier_phi_452_439_ladder_449_ladder_8 = _10109;
                                            }
                                            frontier_phi_452_439_ladder = frontier_phi_452_439_ladder_449_ladder;
                                            frontier_phi_452_439_ladder_1 = frontier_phi_452_439_ladder_449_ladder_1;
                                            frontier_phi_452_439_ladder_2 = frontier_phi_452_439_ladder_449_ladder_2;
                                            frontier_phi_452_439_ladder_3 = frontier_phi_452_439_ladder_449_ladder_3;
                                            frontier_phi_452_439_ladder_4 = frontier_phi_452_439_ladder_449_ladder_4;
                                            frontier_phi_452_439_ladder_5 = frontier_phi_452_439_ladder_449_ladder_5;
                                            frontier_phi_452_439_ladder_6 = frontier_phi_452_439_ladder_449_ladder_6;
                                            frontier_phi_452_439_ladder_7 = frontier_phi_452_439_ladder_449_ladder_7;
                                            frontier_phi_452_439_ladder_8 = frontier_phi_452_439_ladder_449_ladder_8;
                                        }
                                        else
                                        {
                                            float _9953 = _9554 ? _69_m0[104u].w : 0.0f;
                                            float _9954 = 1.0f - _1040;
                                            float _9955 = 1.0f - _1045;
                                            float _9956 = 1.0f - _1048;
                                            float _9960 = clamp(dot(float3(_1000, _1004, _1008), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                            float _9961 = 1.0f - _9953;
                                            float _9965 = clamp(((_9960 * _9961) + _9953) * _9961, 0.0f, 1.0f);
                                            float _10381;
                                            if (_64_m0[76u].w > 0.5f)
                                            {
                                                float _10344 = _9099 + _1739;
                                                float _10345 = _9100 + _1740;
                                                float _10346 = _9101 + _1741;
                                                float _10350 = rsqrt(dot(float3(_10344, _10345, _10346), float3(_10344, _10345, _10346)));
                                                float _10357 = clamp(dot(float3(_9099, _9100, _9101), float3(_10350 * _10344, _10350 * _10345, _10350 * _10346)), 0.0f, 1.0f);
                                                float _10367 = ((((_10357 * _10357) * 2.0f) + 0.5f) * (1.0f - _1051)) + (-1.0f);
                                                _10381 = ((exp2(log2(1.0f - clamp(dot(float3(_1000, _1004, _1008), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * 5.0f) * _10367) + 1.0f) * ((_10367 * exp2(log2(1.0f - _9960) * 5.0f)) + 1.0f);
                                            }
                                            else
                                            {
                                                _10381 = 1.0f;
                                            }
                                            bool _10391 = _9960 > 0.0f;
                                            float _10773;
                                            float _10775;
                                            float _10777;
                                            if (_10391)
                                            {
                                                float _10753 = max(exp2(log2(clamp(1.0f - _1051, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                float _10754 = _9099 + _1739;
                                                float _10755 = _9100 + _1740;
                                                float _10756 = _9101 + _1741;
                                                float _10760 = rsqrt(dot(float3(_10754, _10755, _10756), float3(_10754, _10755, _10756)));
                                                float _10761 = _10760 * _10754;
                                                float _10762 = _10760 * _10755;
                                                float _10763 = _10760 * _10756;
                                                float _10767 = clamp(dot(float3(_1012, _1015, _1018), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                float _10771 = clamp(dot(float3(_1012, _1015, _1018), float3(_10761, _10762, _10763)), 0.0f, 1.0f);
                                                float _11330;
                                                float _11331;
                                                float _11332;
                                                if (_10767 > 0.0f)
                                                {
                                                    float _11297 = _10753 * _10753;
                                                    float _11298 = _11297 * _11297;
                                                    float _11302 = (((_10771 * _11298) - _10771) * _10771) + 1.0f;
                                                    float _11306 = _11297 * 0.5f;
                                                    float _11307 = 1.0f - _11306;
                                                    float _11314 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_10761, _10762, _10763)), 0.0f, 1.0f);
                                                    float _11315 = _11314 * _11314;
                                                    float _11317 = (_11315 * _11315) * _11314;
                                                    float _11326 = min((0.25f / (((_10767 * _11307) + _11306) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _11307) + _11306))) * (_11298 / ((_11302 * _11302) * 3.1415927410125732421875f)), _1719) * _4688;
                                                    _11330 = _11326 * ((_11317 * _9954) + _1040);
                                                    _11331 = _11326 * ((_11317 * _9955) + _1045);
                                                    _11332 = _11326 * ((_11317 * _9956) + _1048);
                                                }
                                                else
                                                {
                                                    _11330 = 0.0f;
                                                    _11331 = 0.0f;
                                                    _11332 = 0.0f;
                                                }
                                                float _11333 = min(_11330, 100000.0f);
                                                float _11334 = min(_11331, 100000.0f);
                                                float _11335 = min(_11332, 100000.0f);
                                                float _11835;
                                                float _11837;
                                                float _11839;
                                                if (_4692)
                                                {
                                                    _11835 = _11333;
                                                    _11837 = _11334;
                                                    _11839 = _11335;
                                                }
                                                else
                                                {
                                                    float _11841 = 1.0f - _9960;
                                                    float _11842 = _11841 * _11841;
                                                    float _11844 = 1.0f - (_11842 * _11842);
                                                    _11835 = _11333 * _11844;
                                                    _11837 = _11334 * _11844;
                                                    _11839 = _11335 * _11844;
                                                }
                                                _10773 = _11835 * _9960;
                                                _10775 = _11837 * _9960;
                                                _10777 = _11839 * _9960;
                                            }
                                            else
                                            {
                                                _10773 = 0.0f;
                                                _10775 = 0.0f;
                                                _10777 = 0.0f;
                                            }
                                            float _11361;
                                            float _11363;
                                            float _11365;
                                            if (_10391)
                                            {
                                                float _11341 = max(exp2(log2(clamp(1.0f - _1088, 0.0f, 1.0f)) * _69_m0[107u].y), _1718);
                                                float _11342 = _9099 + _1739;
                                                float _11343 = _9100 + _1740;
                                                float _11344 = _9101 + _1741;
                                                float _11348 = rsqrt(dot(float3(_11342, _11343, _11344), float3(_11342, _11343, _11344)));
                                                float _11349 = _11348 * _11342;
                                                float _11350 = _11348 * _11343;
                                                float _11351 = _11348 * _11344;
                                                float _11355 = clamp(dot(float3(_1012, _1015, _1018), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                float _11359 = clamp(dot(float3(_1012, _1015, _1018), float3(_11349, _11350, _11351)), 0.0f, 1.0f);
                                                float _11887;
                                                float _11888;
                                                float _11889;
                                                if (_11355 > 0.0f)
                                                {
                                                    float _11854 = _11341 * _11341;
                                                    float _11855 = _11854 * _11854;
                                                    float _11859 = (((_11359 * _11855) - _11359) * _11359) + 1.0f;
                                                    float _11863 = _11854 * 0.5f;
                                                    float _11864 = 1.0f - _11863;
                                                    float _11871 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_11349, _11350, _11351)), 0.0f, 1.0f);
                                                    float _11872 = _11871 * _11871;
                                                    float _11874 = (_11872 * _11872) * _11871;
                                                    float _11883 = min((0.25f / (((_11355 * _11864) + _11863) * ((max(0.001000000047497451305389404296875f, clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f)) * _11864) + _11863))) * (_11855 / ((_11859 * _11859) * 3.1415927410125732421875f)), _1719) * _4688;
                                                    _11887 = _11883 * ((_11874 * _9954) + _1040);
                                                    _11888 = _11883 * ((_11874 * _9955) + _1045);
                                                    _11889 = _11883 * ((_11874 * _9956) + _1048);
                                                }
                                                else
                                                {
                                                    _11887 = 0.0f;
                                                    _11888 = 0.0f;
                                                    _11889 = 0.0f;
                                                }
                                                float _11890 = min(_11887, 100000.0f);
                                                float _11891 = min(_11888, 100000.0f);
                                                float _11892 = min(_11889, 100000.0f);
                                                float _12585;
                                                float _12587;
                                                float _12589;
                                                if (_4692)
                                                {
                                                    _12585 = _11890;
                                                    _12587 = _11891;
                                                    _12589 = _11892;
                                                }
                                                else
                                                {
                                                    float _12591 = 1.0f - _9960;
                                                    float _12592 = _12591 * _12591;
                                                    float _12594 = 1.0f - (_12592 * _12592);
                                                    _12585 = _11890 * _12594;
                                                    _12587 = _11891 * _12594;
                                                    _12589 = _11892 * _12594;
                                                }
                                                _11361 = _12585 * _9960;
                                                _11363 = _12587 * _9960;
                                                _11365 = _12589 * _9960;
                                            }
                                            else
                                            {
                                                _11361 = 0.0f;
                                                _11363 = 0.0f;
                                                _11365 = 0.0f;
                                            }
                                            frontier_phi_452_439_ladder = (((_1030 * 0.3183098733425140380859375f) * _9955) * _9965) * _10381;
                                            frontier_phi_452_439_ladder_1 = (((_1025 * 0.3183098733425140380859375f) * _9954) * _9965) * _10381;
                                            frontier_phi_452_439_ladder_2 = (((_1035 * 0.3183098733425140380859375f) * _9956) * _9965) * _10381;
                                            frontier_phi_452_439_ladder_3 = ((_11361 - _10773) * _1090) + _10773;
                                            frontier_phi_452_439_ladder_4 = ((_11363 - _10775) * _1090) + _10775;
                                            frontier_phi_452_439_ladder_5 = 0.0f;
                                            frontier_phi_452_439_ladder_6 = 0.0f;
                                            frontier_phi_452_439_ladder_7 = 0.0f;
                                            frontier_phi_452_439_ladder_8 = ((_11365 - _10777) * _1090) + _10777;
                                        }
                                        _10080 = frontier_phi_452_439_ladder_1;
                                        _10085 = frontier_phi_452_439_ladder;
                                        _10090 = frontier_phi_452_439_ladder_2;
                                        _10095 = frontier_phi_452_439_ladder_3;
                                        _10101 = frontier_phi_452_439_ladder_4;
                                        _10107 = frontier_phi_452_439_ladder_8;
                                        _10113 = frontier_phi_452_439_ladder_5;
                                        _10115 = frontier_phi_452_439_ladder_6;
                                        _10117 = frontier_phi_452_439_ladder_7;
                                    }
                                    else
                                    {
                                        float _9698 = clamp(dot(float3(_1000, _1004, _1008), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                        float frontier_phi_452_440_ladder;
                                        float frontier_phi_452_440_ladder_1;
                                        float frontier_phi_452_440_ladder_2;
                                        float frontier_phi_452_440_ladder_3;
                                        float frontier_phi_452_440_ladder_4;
                                        float frontier_phi_452_440_ladder_5;
                                        float frontier_phi_452_440_ladder_6;
                                        float frontier_phi_452_440_ladder_7;
                                        float frontier_phi_452_440_ladder_8;
                                        if (_9698 > 0.0f)
                                        {
                                            float _9971 = _9099 + _1739;
                                            float _9972 = _9100 + _1740;
                                            float _9973 = _9101 + _1741;
                                            float _9977 = rsqrt(dot(float3(_9971, _9972, _9973), float3(_9971, _9972, _9973)));
                                            float _9978 = _9977 * _9971;
                                            float _9979 = _9977 * _9972;
                                            float _9980 = _9977 * _9973;
                                            float _9985 = max(clamp(dot(float3(_1012, _1015, _1018), float3(_1739, _1740, _1741)), 0.0f, 1.0f), 0.001000000047497451305389404296875f);
                                            float _9989 = clamp(dot(float3(_1012, _1015, _1018), float3(_9978, _9979, _9980)), 0.0f, 1.0f);
                                            float _9994 = max(1.0f - _1051, 0.04500000178813934326171875f);
                                            float _9995 = _9994 * _9994;
                                            float _9996 = 1.0f / _9995;
                                            float _10010 = (((_9996 + 2.0f) * 0.15915493667125701904296875f) * exp2((_9996 * 0.5f) * log2(1.0f - (_9989 * _9989)))) * (0.25f / ((_9985 + _9698) - (_9985 * _9698)));
                                            float _10014 = _9995 * _9995;
                                            float _10018 = (((_10014 * _9989) - _9989) * _9989) + 1.0f;
                                            float _10022 = _9995 * 0.5f;
                                            float _10023 = 1.0f - _10022;
                                            float _10030 = 1.0f - clamp(dot(float3(_1739, _1740, _1741), float3(_9978, _9979, _9980)), 0.0f, 1.0f);
                                            float _10031 = _10030 * _10030;
                                            float _10033 = (_10031 * _10031) * _10030;
                                            float _10045 = min((0.25f / (((_10023 * _9985) + _10022) * ((_10023 * _9698) + _10022))) * (_10014 / ((_10018 * _10018) * 3.1415927410125732421875f)), _1719) * _4688;
                                            float _10046 = _10045 * ((_10033 * (1.0f - _1040)) + _1040);
                                            float _10047 = _10045 * ((_10033 * (1.0f - _1045)) + _1045);
                                            float _10048 = _10045 * ((_10033 * (1.0f - _1048)) + _1048);
                                            float _10052 = (1.0f - _1096) * _1025;
                                            float _10053 = (1.0f - _1097) * _1030;
                                            float _10054 = (1.0f - _1098) * _1035;
                                            float _10076 = _9698 * 0.3183098733425140380859375f;
                                            frontier_phi_452_440_ladder = _10076 * (((_1030 - _10053) * _1099) + _10053);
                                            frontier_phi_452_440_ladder_1 = _10076 * (((_1025 - _10052) * _1099) + _10052);
                                            frontier_phi_452_440_ladder_2 = _10076 * (((_1035 - _10054) * _1099) + _10054);
                                            frontier_phi_452_440_ladder_3 = ((((_10010 * _1096) - _10046) * _1099) + _10046) * _9698;
                                            frontier_phi_452_440_ladder_4 = ((((_10010 * _1097) - _10047) * _1099) + _10047) * _9698;
                                            frontier_phi_452_440_ladder_5 = 0.0f;
                                            frontier_phi_452_440_ladder_6 = 0.0f;
                                            frontier_phi_452_440_ladder_7 = 0.0f;
                                            frontier_phi_452_440_ladder_8 = ((((_10010 * _1098) - _10048) * _1099) + _10048) * _9698;
                                        }
                                        else
                                        {
                                            frontier_phi_452_440_ladder = 0.0f;
                                            frontier_phi_452_440_ladder_1 = 0.0f;
                                            frontier_phi_452_440_ladder_2 = 0.0f;
                                            frontier_phi_452_440_ladder_3 = 0.0f;
                                            frontier_phi_452_440_ladder_4 = 0.0f;
                                            frontier_phi_452_440_ladder_5 = 0.0f;
                                            frontier_phi_452_440_ladder_6 = 0.0f;
                                            frontier_phi_452_440_ladder_7 = 0.0f;
                                            frontier_phi_452_440_ladder_8 = 0.0f;
                                        }
                                        _10080 = frontier_phi_452_440_ladder_1;
                                        _10085 = frontier_phi_452_440_ladder;
                                        _10090 = frontier_phi_452_440_ladder_2;
                                        _10095 = frontier_phi_452_440_ladder_3;
                                        _10101 = frontier_phi_452_440_ladder_4;
                                        _10107 = frontier_phi_452_440_ladder_8;
                                        _10113 = frontier_phi_452_440_ladder_5;
                                        _10115 = frontier_phi_452_440_ladder_6;
                                        _10117 = frontier_phi_452_440_ladder_7;
                                    }
                                    float _10779;
                                    float _10781;
                                    float _10783;
                                    float _10119;
                                    float _10120;
                                    float _10121;
                                    float _10125;
                                    float _10126;
                                    float _10127;
                                    bool _10128;
                                    bool _10129;
                                    for (;;)
                                    {
                                        _10119 = _10080 * _9562;
                                        _10120 = _10085 * _9563;
                                        _10121 = _10090 * _9564;
                                        _10125 = (_10095 + _10113) * _9562;
                                        _10126 = (_10101 + _10115) * _9563;
                                        _10127 = (_10107 + _10117) * _9564;
                                        _10128 = _1086 != 0u;
                                        _10129 = _9551 == 0u;
                                        if (_10129)
                                        {
                                            if (!_10128)
                                            {
                                                _10779 = 0.0f;
                                                _10781 = 0.0f;
                                                _10783 = 0.0f;
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            if (!(_10128 || (_56.Load((_8845 * 28u) + 4294967293u).x == 1u)))
                                            {
                                                _10779 = 0.0f;
                                                _10781 = 0.0f;
                                                _10783 = 0.0f;
                                                break;
                                            }
                                        }
                                        float _11967;
                                        float _11971;
                                        float _11975;
                                        if (_5572)
                                        {
                                            float frontier_phi_540_510_ladder;
                                            float frontier_phi_540_510_ladder_1;
                                            float frontier_phi_540_510_ladder_2;
                                            if ((_1174 & 67108864u) == 0u)
                                            {
                                                float frontier_phi_540_510_ladder_538_ladder;
                                                float frontier_phi_540_510_ladder_538_ladder_1;
                                                float frontier_phi_540_510_ladder_538_ladder_2;
                                                if ((_1174 & 50331648u) == 0u)
                                                {
                                                    float frontier_phi_540_510_ladder_538_ladder_566_ladder;
                                                    float frontier_phi_540_510_ladder_538_ladder_566_ladder_1;
                                                    float frontier_phi_540_510_ladder_538_ladder_566_ladder_2;
                                                    if ((_1174 & 536870912u) == 0u)
                                                    {
                                                        float _13348 = clamp((dot(float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), float3(_9099, _9100, _9101)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_1079 * 0.3183098733425140380859375f);
                                                        frontier_phi_540_510_ladder_538_ladder_566_ladder = _13348 * _1076;
                                                        frontier_phi_540_510_ladder_538_ladder_566_ladder_1 = _13348 * _1073;
                                                        frontier_phi_540_510_ladder_538_ladder_566_ladder_2 = _13348 * _1070;
                                                    }
                                                    else
                                                    {
                                                        float _13359 = clamp((dot(float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), float3(_9099, _9100, _9101)) + 1.0f) * 0.5f, 0.0f, 1.0f) * (_1079 * 0.3183098733425140380859375f);
                                                        frontier_phi_540_510_ladder_538_ladder_566_ladder = _13359 * _1076;
                                                        frontier_phi_540_510_ladder_538_ladder_566_ladder_1 = _13359 * _1073;
                                                        frontier_phi_540_510_ladder_538_ladder_566_ladder_2 = _13359 * _1070;
                                                    }
                                                    frontier_phi_540_510_ladder_538_ladder = frontier_phi_540_510_ladder_538_ladder_566_ladder;
                                                    frontier_phi_540_510_ladder_538_ladder_1 = frontier_phi_540_510_ladder_538_ladder_566_ladder_1;
                                                    frontier_phi_540_510_ladder_538_ladder_2 = frontier_phi_540_510_ladder_538_ladder_566_ladder_2;
                                                }
                                                else
                                                {
                                                    float _12599 = 1.0f - _1051;
                                                    float _12600 = (-0.0f) - _962;
                                                    float _12601 = (-0.0f) - _956;
                                                    float _12602 = (-0.0f) - _950;
                                                    float _12605 = 1.0f - clamp(_1161 * 66.6666717529296875f, 0.0f, 1.0f);
                                                    float _12609 = dot(float3(_12600, _12601, _12602), float3(_9099, _9100, _9101));
                                                    float _12615 = dot(float3(_12600, _12601, _12602), float3(_1739, _1740, _1741));
                                                    float _12621 = _9099 - (_12609 * _12600);
                                                    float _12622 = _9100 - (_12609 * _12601);
                                                    float _12623 = _9101 - (_12609 * _12602);
                                                    float _12627 = _1739 - (_12615 * _12600);
                                                    float _12628 = _1740 - (_12615 * _12601);
                                                    float _12629 = _1741 - (_12615 * _12602);
                                                    float _12642 = rsqrt((dot(float3(_12627, _12628, _12629), float3(_12627, _12628, _12629)) * dot(float3(_12621, _12622, _12623), float3(_12621, _12622, _12623))) + 9.9999997473787516355514526367188e-05f) * dot(float3(_12621, _12622, _12623), float3(_12627, _12628, _12629));
                                                    float _12644 = (_12642 * 0.5f) + 0.5f;
                                                    float _12647 = asin(_12615);
                                                    float _12648 = asin(_12609);
                                                    float _12652 = cos(abs(_12647 - _12648) * 0.5f);
                                                    float _12656 = 1.0f / ((1.190000057220458984375f / _12652) + (_12652 * 0.36000001430511474609375f));
                                                    float _12657 = _12615 * 0.645161330699920654296875f;
                                                    float _12660 = sqrt(1.0f - (_12657 * _12657));
                                                    float _12662 = max(_12599 * 0.5f, 0.00999999977648258209228515625f);
                                                    float _12665 = _12615 + (_12609 - (_1171 * 0.5f));
                                                    float _12673 = exp2((((_12665 * _12665) * (-0.5f)) / (_12662 * _12662)) * 1.44269502162933349609375f) / (_12662 * 2.5066282749176025390625f);
                                                    float _12682 = clamp((-0.0f) - dot(float3(_1739, _1740, _1741), float3(_9099, _9100, _9101)), 0.0f, 1.0f);
                                                    float _12683 = _12682 * _12682;
                                                    float _12686 = (_12683 * _12683) * (_12682 * _12605);
                                                    float _12693 = (cos(asin((_12656 * sqrt(clamp(_12644, 0.0f, 1.0f))) * ((_12656 * (0.60000002384185791015625f - (_12642 * 0.800000011920928955078125f))) + 1.0f)) * 2.0f) + 1.0f) * (-2.8853900432586669921875f);
                                                    float _12697 = exp2(_12693 * (_1155 / _12660));
                                                    float _12698 = exp2(_12693 * (_1157 / _12660));
                                                    float _12699 = exp2(_12693 * (_1159 / _12660));
                                                    float _12714 = 0.95347940921783447265625f - (exp2(log2(1.0f - _12652) * 5.0f) * 0.95347940921783447265625f);
                                                    float _12715 = _12714 * _12714;
                                                    float _12717 = (_12652 * 0.5f) + 0.5f;
                                                    float _12730 = abs(sqrt(1.0f - (_12609 * _12609))) * _15[(_1153 + 513u) + 0u].SampleLevel(_72, float2(_12717, _12644), 0.0f).x;
                                                    float _12731 = _12730 * ((_12605 * _9539) * _12673);
                                                    float _12738 = _9541 + _1161;
                                                    float _12739 = _1051 * 0.75f;
                                                    float _12743 = (((_1163 * _1163) * 9000.0f) * _1163) * _12738;
                                                    float _14038;
                                                    float _14039;
                                                    float _14040;
                                                    if ((_1174 & 33554432u) == 0u)
                                                    {
                                                        float _13360 = max(_12743, _12739);
                                                        float _13366 = _12648 + _12647;
                                                        float _13367 = _13366 * 0.5f;
                                                        float _13369 = _1155 * 0.5f;
                                                        float _13370 = _1157 * 0.5f;
                                                        float _13371 = _1159 * 0.5f;
                                                        float _13372 = max(_12599, 0.0500000007450580596923828125f);
                                                        float _13374 = (cos(_13367) * 0.5f) + 0.5f;
                                                        uint _13376 = (_1153 + 521u) + 0u;
                                                        float4 _13381 = _19[_13376].SampleLevel(_72, float3(_13374, _13372, _13369), 0.0f);
                                                        float _13383 = _13381.x;
                                                        float4 _13384 = _19[_13376].SampleLevel(_72, float3(_13374, _13372, _13370), 0.0f);
                                                        float _13386 = _13384.x;
                                                        float4 _13387 = _19[_13376].SampleLevel(_72, float3(_13374, _13372, _13371), 0.0f);
                                                        float _13389 = _13387.x;
                                                        float4 _13390 = _19[_13376].SampleLevel(_72, float3(_12717, _13372, _13369), 0.0f);
                                                        float4 _13394 = _19[_13376].SampleLevel(_72, float3(_12717, _13372, _13370), 0.0f);
                                                        float4 _13398 = _19[_13376].SampleLevel(_72, float3(_12717, _13372, _13371), 0.0f);
                                                        uint _13403 = (_1153 + 545u) + 0u;
                                                        float _13418 = (_13386 + _13383) + _13389;
                                                        float _13425 = dot(float3(max((1.0f - _1165) * _12599, 0.00999999977648258209228515625f), _12662, max(_12599 * 2.0f, 0.00999999977648258209228515625f)), float3(_13383 / _13418, _13386 / _13418, _13389 / _13418)) * _13360;
                                                        float _13436 = (_12652 * _12652) * 3.1415927410125732421875f;
                                                        float _13440 = _13425 * 0.5f;
                                                        float _13441 = _13440 + _13390.z;
                                                        float _13442 = _13440 + _13394.z;
                                                        float _13443 = _13440 + _13398.z;
                                                        float _13445 = (_13366 * (-0.25f)) * _13367;
                                                        float _13467 = ((_13390.y * 2.0f) * (exp2((_13445 / (_13441 * _13441)) * 1.44269502162933349609375f) / (_13441 * 2.5066282749176025390625f))) / _13436;
                                                        float _13468 = ((_13394.y * 2.0f) * (exp2((_13445 / (_13442 * _13442)) * 1.44269502162933349609375f) / (_13442 * 2.5066282749176025390625f))) / _13436;
                                                        float _13469 = ((_13398.y * 2.0f) * (exp2((_13445 / (_13443 * _13443)) * 1.44269502162933349609375f) / (_13443 * 2.5066282749176025390625f))) / _13436;
                                                        float _13470 = _13366 - _1171;
                                                        float _13473 = max(max(_13425, _13425), _13425) + _12662;
                                                        float _13481 = exp2((((_13470 * _13470) * (-0.125f)) / (_13473 * _13473)) * 1.44269502162933349609375f) / (_13473 * 2.5066282749176025390625f);
                                                        float _13485 = _9539 * 0.3499999940395355224609375f;
                                                        _14038 = (((exp2(log2(_13383) * _13360) * 0.4899999797344207763671875f) * ((_13481 * _15[_13403].SampleLevel(_72, float2(_12717, _13369), 0.0f).y) + (_13467 * 2.19911479949951171875f))) + (_13467 * _13485)) * _1167;
                                                        _14039 = (((exp2(log2(_13386) * _13360) * 0.4899999797344207763671875f) * ((_13481 * _15[_13403].SampleLevel(_72, float2(_12717, _13370), 0.0f).y) + (_13468 * 2.19911479949951171875f))) + (_13468 * _13485)) * _1167;
                                                        _14040 = (((exp2(log2(_13389) * _13360) * 0.4899999797344207763671875f) * ((_13481 * _15[_13403].SampleLevel(_72, float2(_12717, _13371), 0.0f).y) + (_13469 * 2.19911479949951171875f))) + (_13469 * _13485)) * _1167;
                                                    }
                                                    else
                                                    {
                                                        float _13511 = 1.0f - clamp((_12738 * 66.6666717529296875f) * max(_1163, 9.9999997473787516355514526367188e-05f), 0.0f, 1.0f);
                                                        float _13512 = _13511 * _13511;
                                                        float _13514 = (_13512 * _13512) * _13511;
                                                        float _13515 = _12715 * _12673;
                                                        float _13523 = (_12739 + 1.25f) + _12743;
                                                        float _13547 = dot(float3(_1739, _1740, _1741), float3(_12600, _12601, _12602));
                                                        float _13553 = _1739 - (_13547 * _12600);
                                                        float _13554 = _1740 - (_13547 * _12601);
                                                        float _13555 = _1741 - (_13547 * _12602);
                                                        float _13559 = rsqrt(dot(float3(_13553, _13554, _13555), float3(_13553, _13554, _13555)));
                                                        float _13567 = (dot(float3(_13553 * _13559, _13554 * _13559, _13555 * _13559), float3(_9099, _9100, _9101)) + 1.0f) * 0.25f;
                                                        float _13572 = (_1167 * 0.2228169143199920654296875f) * ((((1.0f - abs(_12609)) - _13567) * 0.3300000131130218505859375f) + _13567);
                                                        _14038 = ((_13572 * exp2(log2(exp2(((_1155 * (-3.7999999523162841796875f)) / _12652) * 1.44269502162933349609375f)) * _13523)) + ((_12730 * _12697) * _13515)) * _13514;
                                                        _14039 = ((_13572 * exp2(log2(exp2(((_1157 * (-3.7999999523162841796875f)) / _12652) * 1.44269502162933349609375f)) * _13523)) + ((_12730 * _12698) * _13515)) * _13514;
                                                        _14040 = ((_13572 * exp2(log2(exp2(((_1159 * (-3.7999999523162841796875f)) / _12652) * 1.44269502162933349609375f)) * _13523)) + ((_12730 * _12699) * _13515)) * _13514;
                                                    }
                                                    frontier_phi_540_510_ladder_538_ladder = _14040 + ((_12731 * (((1.0f - _12699) * _12686) + _12699)) * _12715);
                                                    frontier_phi_540_510_ladder_538_ladder_1 = _14039 + ((_12731 * (((1.0f - _12698) * _12686) + _12698)) * _12715);
                                                    frontier_phi_540_510_ladder_538_ladder_2 = _14038 + ((_12731 * (((1.0f - _12697) * _12686) + _12697)) * _12715);
                                                }
                                                frontier_phi_540_510_ladder = frontier_phi_540_510_ladder_538_ladder;
                                                frontier_phi_540_510_ladder_1 = frontier_phi_540_510_ladder_538_ladder_1;
                                                frontier_phi_540_510_ladder_2 = frontier_phi_540_510_ladder_538_ladder_2;
                                            }
                                            else
                                            {
                                                uint _11895 = _1149 + 0u;
                                                float _11896 = dot(float3(_9099, _9100, _9101), float3(_1139, _1141, _1143));
                                                float _11911 = dot(float3(_1000, _1004, _1008), float3((_9100 * _1143) - (_9101 * _1141), (_9101 * _1139) - (_9099 * _1143), (_9099 * _1141) - (_9100 * _1139)));
                                                float _11920 = float(int(uint(_11911 > 0.0f) - uint(_11911 < 0.0f))) * sqrt(1.0f - (_11896 * _11896));
                                                float _11921 = _1135 + (-0.5f);
                                                float _11922 = _1137 + (-0.5f);
                                                float _11928 = mad(_11922, _11920, _11896 * _11921) + 0.5f;
                                                float _11929 = mad(_11922, _11896, (-0.0f) - (_11921 * _11920)) + 0.5f;
                                                float _11935 = (1.0f - clamp(dot(float3(_9099, _9100, _9101), float3(_1000, _1004, _1008)), 0.0f, 1.0f)) * 5.0f;
                                                uint _11936 = uint(int(_11935));
                                                float4 _11943 = _43[NonUniformResourceIndex(_11895)].SampleLevel(_76, float3(_11928, _11929, float(int(_11936))), 0.0f);
                                                float _11945 = _11943.x;
                                                float _11954 = ((_43[NonUniformResourceIndex(_11895)].SampleLevel(_76, float3(_11928, _11929, min(float(int(_11936 + 1u)), 5.0f)), 0.0f).x - _11945) * frac(_11935)) + _11945;
                                                frontier_phi_540_510_ladder = (((_1035 * 0.3183098733425140380859375f) * _1145) * _1147) * _11954;
                                                frontier_phi_540_510_ladder_1 = (((_1030 * 0.3183098733425140380859375f) * _1145) * _1147) * _11954;
                                                frontier_phi_540_510_ladder_2 = (((_1025 * 0.3183098733425140380859375f) * _1145) * _1147) * _11954;
                                            }
                                            _11967 = frontier_phi_540_510_ladder_2;
                                            _11971 = frontier_phi_540_510_ladder_1;
                                            _11975 = frontier_phi_540_510_ladder;
                                        }
                                        else
                                        {
                                            float _11379 = (_9541 * (-693.14715576171875f)) * log2(max(1.0f - _1094, 1.1754943508222875079687365372222e-38f));
                                            float _11380 = _11379 * _11379;
                                            float _11382 = exp2(_11380 * (-225.4210968017578125f));
                                            float _11387 = exp2(_11380 * (-29.8077487945556640625f));
                                            float _11395 = exp2(_11380 * (-7.714946269989013671875f));
                                            float _11401 = exp2(_11380 * (-2.5444357395172119140625f));
                                            float _11403 = _11401 * 0.007000000216066837310791015625f;
                                            float _11408 = exp2(_11380 * (-0.72497236728668212890625f));
                                            float _11424 = max(dot(float3((-0.0f) - _1000, (-0.0f) - _1004, (-0.0f) - _1008), float3(_9099, _9100, _9101)) + 0.300000011920928955078125f, 0.0f);
                                            _11967 = (((((((_11387 * 0.100000001490116119384765625f) + (_11382 * 0.23299999535083770751953125f)) + (_11395 * 0.1180000007152557373046875f)) + (_11401 * 0.112999998033046722412109375f)) + (_11408 * 0.3580000102519989013671875f)) + (exp2(_11380 * (-0.1946956813335418701171875f)) * 0.078000001609325408935546875f)) * _1025) * _11424;
                                            _11971 = ((((((_11387 * 0.3359999954700469970703125f) + (_11382 * 0.4550000131130218505859375f)) + (_11395 * 0.19799999892711639404296875f)) + _11403) + (_11408 * 0.0040000001899898052215576171875f)) * _1030) * _11424;
                                            _11975 = ((((_11387 * 0.3440000116825103759765625f) + (_11382 * 0.648999989032745361328125f)) + _11403) * _1035) * _11424;
                                        }
                                        float _11983 = (_10128 ? 1.0f : _9552) * _9539;
                                        _10779 = (_11967 * _9562) * _11983;
                                        _10781 = (_11971 * _9563) * _11983;
                                        _10783 = (_11975 * _9564) * _11983;
                                        break;
                                    }
                                    frontier_phi_399_pred_398_ladder = (_10783 * _9539) + _8759;
                                    frontier_phi_399_pred_398_ladder_1 = (_10781 * _9539) + _8758;
                                    frontier_phi_399_pred_398_ladder_2 = (_10779 * _9539) + _8757;
                                    frontier_phi_399_pred_398_ladder_3 = (_10127 * _9539) + _8756;
                                    frontier_phi_399_pred_398_ladder_4 = (_10126 * _9539) + _8755;
                                    frontier_phi_399_pred_398_ladder_5 = (_10125 * _9539) + _8754;
                                    frontier_phi_399_pred_398_ladder_6 = (_10121 * _9539) + _8753;
                                    frontier_phi_399_pred_398_ladder_7 = (_10120 * _9539) + _8752;
                                    frontier_phi_399_pred_398_ladder_8 = (_10119 * _9539) + _8751;
                                }
                                else
                                {
                                    frontier_phi_399_pred_398_ladder = _8759;
                                    frontier_phi_399_pred_398_ladder_1 = _8758;
                                    frontier_phi_399_pred_398_ladder_2 = _8757;
                                    frontier_phi_399_pred_398_ladder_3 = _8756;
                                    frontier_phi_399_pred_398_ladder_4 = _8755;
                                    frontier_phi_399_pred_398_ladder_5 = _8754;
                                    frontier_phi_399_pred_398_ladder_6 = _8753;
                                    frontier_phi_399_pred_398_ladder_7 = _8752;
                                    frontier_phi_399_pred_398_ladder_8 = _8751;
                                }
                                frontier_phi_399_pred = frontier_phi_399_pred_398_ladder;
                                frontier_phi_399_pred_1 = frontier_phi_399_pred_398_ladder_1;
                                frontier_phi_399_pred_2 = frontier_phi_399_pred_398_ladder_2;
                                frontier_phi_399_pred_3 = frontier_phi_399_pred_398_ladder_3;
                                frontier_phi_399_pred_4 = frontier_phi_399_pred_398_ladder_4;
                                frontier_phi_399_pred_5 = frontier_phi_399_pred_398_ladder_5;
                                frontier_phi_399_pred_6 = frontier_phi_399_pred_398_ladder_6;
                                frontier_phi_399_pred_7 = frontier_phi_399_pred_398_ladder_7;
                                frontier_phi_399_pred_8 = frontier_phi_399_pred_398_ladder_8;
                            }
                            else
                            {
                                frontier_phi_399_pred = _8759;
                                frontier_phi_399_pred_1 = _8758;
                                frontier_phi_399_pred_2 = _8757;
                                frontier_phi_399_pred_3 = _8756;
                                frontier_phi_399_pred_4 = _8755;
                                frontier_phi_399_pred_5 = _8754;
                                frontier_phi_399_pred_6 = _8753;
                                frontier_phi_399_pred_7 = _8752;
                                frontier_phi_399_pred_8 = _8751;
                            }
                            _8650 = frontier_phi_399_pred;
                            _8649 = frontier_phi_399_pred_1;
                            _8648 = frontier_phi_399_pred_2;
                            _8647 = frontier_phi_399_pred_3;
                            _8646 = frontier_phi_399_pred_4;
                            _8645 = frontier_phi_399_pred_5;
                            _8644 = frontier_phi_399_pred_6;
                            _8642 = frontier_phi_399_pred_7;
                            _8640 = frontier_phi_399_pred_8;
                            if (_8652 > _8343)
                            {
                                break;
                            }
                            else
                            {
                                _8751 = _8640;
                                _8752 = _8642;
                                _8753 = _8644;
                                _8754 = _8645;
                                _8755 = _8646;
                                _8756 = _8647;
                                _8757 = _8648;
                                _8758 = _8649;
                                _8759 = _8650;
                                _8760 = _8652;
                                _8761 = _8764;
                                continue;
                            }
                        }
                        _8639 = _8640;
                        _8641 = _8642;
                        _8643 = _8644;
                        _7819 = _8645;
                        _7822 = _8646;
                        _7825 = _8647;
                        _7828 = _8648;
                        _7830 = _8649;
                        _7832 = _8650;
                        _8651 = _8652;
                        _8653 = _8764;
                    }
                    float _7810;
                    float _7813;
                    float _7816;
                    uint _8746;
                    uint _8748;
                    if (_8651 > _8344)
                    {
                        _7810 = _8639;
                        _7813 = _8641;
                        _7816 = _8643;
                        _8746 = _8651;
                        _8748 = _8653;
                    }
                    else
                    {
                        float _8743;
                        float _8744;
                        float _8745;
                        float _8891 = _8639;
                        float _8892 = _8641;
                        float _8893 = _8643;
                        uint _8894 = _8651;
                        uint _8895 = _8653;
                        uint _8747;
                        uint _8898;
                        uint _8913;
                        uint _8914;
                        float _8942;
                        float _8944;
                        float _8947;
                        float _8949;
                        uint _8951;
                        bool _8954;
                        float _8956;
                        float _8959;
                        float _8961;
                        float _8964;
                        float _8966;
                        float _8969;
                        float _8971;
                        bool _8976;
                        for (;;)
                        {
                            _8747 = _8894 + 1u;
                            _8898 = _47.Load(_8894).x;
                            uint _8900 = _8895 * 4u;
                            uint4 _8912 = uint4(_48.Load(_8900).x, _48.Load(_8900 + 1u).x, _48.Load(_8900 + 2u).x, _48.Load(_8900 + 3u).x);
                            _8913 = _8912.x;
                            _8914 = _8912.y;
                            uint _8915 = _8912.z;
                            uint _8916 = _8912.w;
                            uint _8918 = _8895 * 4u;
                            uint3 _8927 = uint3(_49.Load(_8918).x, _49.Load(_8918 + 1u).x, _49.Load(_8918 + 2u).x);
                            uint _8928 = _8927.x;
                            uint _8929 = _8927.y;
                            uint _8930 = _8927.z;
                            _8942 = spvUnpackHalf2x16(_8914 >> 16u).x;
                            _8944 = spvUnpackHalf2x16(_8915).x;
                            _8947 = spvUnpackHalf2x16(_8915 >> 16u).x;
                            _8949 = spvUnpackHalf2x16(_8916).x;
                            _8951 = (_8916 >> 16u) & 7u;
                            _8954 = (_8916 & 524288u) != 0u;
                            _8956 = spvUnpackHalf2x16(_8928).x;
                            _8959 = spvUnpackHalf2x16(_8928 >> 16u).x;
                            _8961 = spvUnpackHalf2x16(_8929).x;
                            _8964 = spvUnpackHalf2x16(_8929 >> 16u).x;
                            _8966 = spvUnpackHalf2x16(_8930).x;
                            _8969 = spvUnpackHalf2x16(_8930 >> 16u).x;
                            _8971 = spvUnpackHalf2x16(uint3(_8933, _8934, _50.Load((_8895 * 4u) + 2u).x).z).x;
                            _8976 = (_8916 < 3221225472u) && (((_549 & 255u) & (_8916 >> 22u)) != 0u);
                            float frontier_phi_405_pred;
                            float frontier_phi_405_pred_1;
                            float frontier_phi_405_pred_2;
                            if (_8976)
                            {
                                float _9080 = spvUnpackHalf2x16(_8913).x - _8328;
                                float _9081 = spvUnpackHalf2x16(_8913 >> 16u).x - _8329;
                                float _9082 = spvUnpackHalf2x16(_8914).x - _8330;
                                float _9088 = sqrt(((_9081 * _9081) + (_9082 * _9082)) + (_9080 * _9080));
                                float _9089 = _9088 * _8942;
                                float frontier_phi_405_pred_404_ladder;
                                float frontier_phi_405_pred_404_ladder_1;
                                float frontier_phi_405_pred_404_ladder_2;
                                if (_9089 < 1.0f)
                                {
                                    float _9235 = rsqrt(dot(float3(_9080, _9081, _9082), float3(_9080, _9081, _9082)));
                                    float _9236 = _9235 * _9080;
                                    float _9237 = _9235 * _9081;
                                    float _9238 = _9235 * _9082;
                                    float _9239 = _9088 * _9088;
                                    float _9241 = (_8942 * _8942) * _9239;
                                    float _9244 = clamp(1.0f - (_9241 * _9241), 0.0f, 1.0f);
                                    float _9471;
                                    if (_8951 == 0u)
                                    {
                                        _9471 = (_9244 * _9244) * (1.0f / (max(_9239, 9.9999997473787516355514526367188e-05f) + ((_8971 * _8971) * 0.5f)));
                                    }
                                    else
                                    {
                                        _9471 = max((1.0f / dot(float3(1.0f, _9089, _9089 * _9089), float3(_69_m0[_8951 + 60u].xyz))) * (1.0f - _9089), 0.0f);
                                    }
                                    float _9472 = (-0.0f) - _8956;
                                    float _9496 = clamp((clamp(dot(float3(((_8964 * _8959) - (_8961 * _9472)) * 2.0f, ((_8961 * _8959) - (_8964 * _8956)) * 2.0f, (((_8956 * _9472) - (_8959 * _8959)) * 2.0f) + 1.0f), float3((-0.0f) - _9236, (-0.0f) - _9237, (-0.0f) - _9238)), 0.0f, 1.0f) - _8969) / (_8966 - _8969), 0.0f, 1.0f);
                                    float _9510 = (((_9496 * _9496) * _9471) * max(float(_8954) * 16.0f, 1.0f)) * (3.0f - (_9496 * 2.0f));
                                    float _9514 = (_8942 < 0.02857142873108386993408203125f) ? _69_m0[104u].w : 0.0f;
                                    float _9522 = 1.0f - _9514;
                                    float _9526 = clamp(((_9522 * clamp(dot(float3(_1000, _1004, _1008), float3(_9236, _9237, _9238)), 0.0f, 1.0f)) + _9514) * _9522, 0.0f, 1.0f);
                                    frontier_phi_405_pred_404_ladder = ((_9526 * ((_1030 * 0.3183098733425140380859375f) * (1.0f - _1045))) * (_9510 * _8947)) + _8892;
                                    frontier_phi_405_pred_404_ladder_1 = ((_9526 * ((_1025 * 0.3183098733425140380859375f) * (1.0f - _1040))) * (_9510 * _8944)) + _8891;
                                    frontier_phi_405_pred_404_ladder_2 = ((_9526 * ((_1035 * 0.3183098733425140380859375f) * (1.0f - _1048))) * (_9510 * _8949)) + _8893;
                                }
                                else
                                {
                                    frontier_phi_405_pred_404_ladder = _8892;
                                    frontier_phi_405_pred_404_ladder_1 = _8891;
                                    frontier_phi_405_pred_404_ladder_2 = _8893;
                                }
                                frontier_phi_405_pred = frontier_phi_405_pred_404_ladder;
                                frontier_phi_405_pred_1 = frontier_phi_405_pred_404_ladder_1;
                                frontier_phi_405_pred_2 = frontier_phi_405_pred_404_ladder_2;
                            }
                            else
                            {
                                frontier_phi_405_pred = _8892;
                                frontier_phi_405_pred_1 = _8891;
                                frontier_phi_405_pred_2 = _8893;
                            }
                            _8744 = frontier_phi_405_pred;
                            _8743 = frontier_phi_405_pred_1;
                            _8745 = frontier_phi_405_pred_2;
                            if (_8747 > _8344)
                            {
                                break;
                            }
                            else
                            {
                                _8891 = _8743;
                                _8892 = _8744;
                                _8893 = _8745;
                                _8894 = _8747;
                                _8895 = _8898;
                                continue;
                            }
                        }
                        _7810 = _8743;
                        _7813 = _8744;
                        _7816 = _8745;
                        _8746 = _8747;
                        _8748 = _8898;
                    }
                    float _7834;
                    float _7836;
                    float _7838;
                    uint _8886;
                    uint _8888;
                    if (_8746 > _8345)
                    {
                        _7834 = 0.0f;
                        _7836 = 0.0f;
                        _7838 = 0.0f;
                        _8886 = _8746;
                        _8888 = _8748;
                    }
                    else
                    {
                        float _8883;
                        float _8884;
                        float _8885;
                        float _9019 = 0.0f;
                        float _9020 = 0.0f;
                        float _9021 = 0.0f;
                        uint _9022 = _8746;
                        uint _9023 = _8748;
                        uint _8887;
                        uint _9026;
                        float _9056;
                        float _9059;
                        float _9061;
                        float _9064;
                        float _9068;
                        float _9071;
                        bool _9072;
                        for (;;)
                        {
                            _8887 = _9022 + 1u;
                            _9026 = _47.Load(_9022).x;
                            uint _9028 = _9023 * 4u;
                            uint4 _9040 = uint4(_48.Load(_9028).x, _48.Load(_9028 + 1u).x, _48.Load(_9028 + 2u).x, _48.Load(_9028 + 3u).x);
                            uint _9041 = _9040.x;
                            uint _9042 = _9040.y;
                            uint _9043 = _9040.z;
                            uint _9044 = _9040.w;
                            _9056 = spvUnpackHalf2x16(_9043).x;
                            _9059 = spvUnpackHalf2x16(_9043 >> 16u).x;
                            _9061 = spvUnpackHalf2x16(_9044).x;
                            _9064 = spvUnpackHalf2x16(_9044 >> 16u).x;
                            float _9065 = spvUnpackHalf2x16(_9041).x - _8328;
                            float _9066 = spvUnpackHalf2x16(_9041 >> 16u).x - _8329;
                            float _9067 = spvUnpackHalf2x16(_9042).x - _8330;
                            _9068 = dot(float3(_9065, _9066, _9067), float3(_9065, _9066, _9067));
                            _9071 = _9068 * spvUnpackHalf2x16(_9042 >> 16u).x;
                            _9072 = _9071 < 1.0f;
                            float frontier_phi_412_pred;
                            float frontier_phi_412_pred_1;
                            float frontier_phi_412_pred_2;
                            if (_9072)
                            {
                                float _9467;
                                if (_9064 != 0.0f)
                                {
                                    _9467 = (1.0f / ((_9071 * _9064) + 1.0f)) * (1.0f - _9071);
                                }
                                else
                                {
                                    float _9367 = clamp(1.0f - (_9071 * _9071), 0.0f, 1.0f);
                                    _9467 = (_9367 * _9367) * (1.0f / max(_9068, 9.9999997473787516355514526367188e-05f));
                                }
                                frontier_phi_412_pred = (_9467 * _9059) + _9020;
                                frontier_phi_412_pred_1 = (_9467 * _9056) + _9019;
                                frontier_phi_412_pred_2 = (_9467 * _9061) + _9021;
                            }
                            else
                            {
                                frontier_phi_412_pred = _9020;
                                frontier_phi_412_pred_1 = _9019;
                                frontier_phi_412_pred_2 = _9021;
                            }
                            _8884 = frontier_phi_412_pred;
                            _8883 = frontier_phi_412_pred_1;
                            _8885 = frontier_phi_412_pred_2;
                            if (_8887 > _8345)
                            {
                                break;
                            }
                            else
                            {
                                _9019 = _8883;
                                _9020 = _8884;
                                _9021 = _8885;
                                _9022 = _8887;
                                _9023 = _9026;
                                continue;
                            }
                        }
                        _7834 = _8883;
                        _7836 = _8884;
                        _7838 = _8885;
                        _8886 = _8887;
                        _8888 = _9026;
                    }
                    float frontier_phi_353_354_ladder_395_ladder;
                    float frontier_phi_353_354_ladder_395_ladder_1;
                    float frontier_phi_353_354_ladder_395_ladder_2;
                    float frontier_phi_353_354_ladder_395_ladder_3;
                    float frontier_phi_353_354_ladder_395_ladder_4;
                    float frontier_phi_353_354_ladder_395_ladder_5;
                    float frontier_phi_353_354_ladder_395_ladder_6;
                    float frontier_phi_353_354_ladder_395_ladder_7;
                    float frontier_phi_353_354_ladder_395_ladder_8;
                    float frontier_phi_353_354_ladder_395_ladder_9;
                    float frontier_phi_353_354_ladder_395_ladder_10;
                    float frontier_phi_353_354_ladder_395_ladder_11;
                    if (_8886 > _8346)
                    {
                        frontier_phi_353_354_ladder_395_ladder = _7830;
                        frontier_phi_353_354_ladder_395_ladder_1 = _7813;
                        frontier_phi_353_354_ladder_395_ladder_2 = _7816;
                        frontier_phi_353_354_ladder_395_ladder_3 = _7819;
                        frontier_phi_353_354_ladder_395_ladder_4 = _7822;
                        frontier_phi_353_354_ladder_395_ladder_5 = _7810;
                        frontier_phi_353_354_ladder_395_ladder_6 = _7825;
                        frontier_phi_353_354_ladder_395_ladder_7 = _7832;
                        frontier_phi_353_354_ladder_395_ladder_8 = _7834;
                        frontier_phi_353_354_ladder_395_ladder_9 = _7836;
                        frontier_phi_353_354_ladder_395_ladder_10 = _7838;
                        frontier_phi_353_354_ladder_395_ladder_11 = _7828;
                    }
                    else
                    {
                        float _7811;
                        float _7814;
                        float _7817;
                        float _7820;
                        float _7823;
                        float _7826;
                        float _9130 = _7810;
                        float _9131 = _7813;
                        float _9132 = _7816;
                        float _9133 = _7819;
                        float _9134 = _7822;
                        float _9135 = _7825;
                        uint _9136 = _8886;
                        uint _9138 = _8888;
                        uint _9137;
                        uint _9142;
                        uint _9157;
                        uint _9158;
                        float _9190;
                        float _9192;
                        float _9195;
                        float _9197;
                        uint _9199;
                        bool _9202;
                        float _9204;
                        float _9207;
                        float _9209;
                        float _9212;
                        float _9214;
                        float _9217;
                        float _9219;
                        float _9221;
                        bool _9226;
                        for (;;)
                        {
                            _9137 = _9136 + 1u;
                            _9142 = _47.Load(_9136).x;
                            uint _9144 = _9138 * 4u;
                            uint4 _9156 = uint4(_48.Load(_9144).x, _48.Load(_9144 + 1u).x, _48.Load(_9144 + 2u).x, _48.Load(_9144 + 3u).x);
                            _9157 = _9156.x;
                            _9158 = _9156.y;
                            uint _9159 = _9156.z;
                            uint _9160 = _9156.w;
                            uint _9162 = _9138 * 4u;
                            uint4 _9174 = uint4(_49.Load(_9162).x, _49.Load(_9162 + 1u).x, _49.Load(_9162 + 2u).x, _49.Load(_9162 + 3u).x);
                            uint _9175 = _9174.x;
                            uint _9176 = _9174.y;
                            uint _9177 = _9174.z;
                            _9190 = spvUnpackHalf2x16(_9158 >> 16u).x;
                            _9192 = spvUnpackHalf2x16(_9159).x;
                            _9195 = spvUnpackHalf2x16(_9159 >> 16u).x;
                            _9197 = spvUnpackHalf2x16(_9160).x;
                            _9199 = (_9160 >> 16u) & 7u;
                            _9202 = (_9160 & 524288u) != 0u;
                            _9204 = spvUnpackHalf2x16(_9175).x;
                            _9207 = spvUnpackHalf2x16(_9175 >> 16u).x;
                            _9209 = spvUnpackHalf2x16(_9176).x;
                            _9212 = spvUnpackHalf2x16(_9176 >> 16u).x;
                            _9214 = spvUnpackHalf2x16(_9177).x;
                            _9217 = spvUnpackHalf2x16(_9177 >> 16u).x;
                            _9219 = spvUnpackHalf2x16(_9174.w).x;
                            _9221 = spvUnpackHalf2x16(uint3(_9181, _9182, _50.Load((_9138 * 4u) + 2u).x).z).x;
                            _9226 = (_9160 < 3221225472u) && (((_549 & 255u) & (_9160 >> 22u)) != 0u);
                            float frontier_phi_419_pred;
                            float frontier_phi_419_pred_1;
                            float frontier_phi_419_pred_2;
                            float frontier_phi_419_pred_3;
                            float frontier_phi_419_pred_4;
                            float frontier_phi_419_pred_5;
                            if (_9226)
                            {
                                float _9276 = (-0.0f) - _9209;
                                float _9282 = (_9209 * _9276) - (_9204 * _9204);
                                float _9283 = _9212 * _9204;
                                float _9288 = (-0.0f) - _9204;
                                float _9301 = _9219 * ((_9207 * _9204) - (_9212 * _9209));
                                float _9304 = _9219 * (_9283 - (_9207 * _9276));
                                float _9305 = spvUnpackHalf2x16(_9157).x - _9301;
                                float _9306 = spvUnpackHalf2x16(_9157 >> 16u).x - (_9219 * (_9282 + 0.5f));
                                float _9307 = spvUnpackHalf2x16(_9158).x - _9304;
                                float _9311 = _9301 * 2.0f;
                                float _9312 = _9219 * ((_9282 * 2.0f) + 1.0f);
                                float _9313 = _9304 * 2.0f;
                                float _9321 = clamp(dot(float3(_9311, _9312, _9313), float3(_8328 - _9305, _8329 - _9306, _8330 - _9307)) / dot(float3(_9311, _9312, _9313), float3(_9311, _9312, _9313)), 0.0f, 1.0f);
                                float _9326 = (_9321 * _9311) + (_9305 - _8328);
                                float _9328 = (_9321 * _9312) + (_9306 - _8329);
                                float _9330 = (_9321 * _9313) + (_9307 - _8330);
                                float _9334 = rsqrt(dot(float3(_9326, _9328, _9330), float3(_9326, _9328, _9330)));
                                float _9335 = _9326 * _9334;
                                float _9336 = _9328 * _9334;
                                float _9337 = _9330 * _9334;
                                float _9343 = sqrt(((_9326 * _9326) + (_9328 * _9328)) + (_9330 * _9330));
                                float _9344 = _9343 * _9343;
                                float _9346 = (_9190 * _9190) * _9344;
                                float _9349 = clamp(1.0f - (_9346 * _9346), 0.0f, 1.0f);
                                float _9691;
                                if (_9199 == 0u)
                                {
                                    _9691 = (_9349 * _9349) * (1.0f / (max(_9344, 9.9999997473787516355514526367188e-05f) + ((_9221 * _9221) * 0.5f)));
                                }
                                else
                                {
                                    float _9452 = _9343 * _9190;
                                    _9691 = max((1.0f / dot(float3(1.0f, _9452, _9452 * _9452), float3(_69_m0[_9199 + 60u].xyz))) * (1.0f - _9452), 0.0f);
                                }
                                float frontier_phi_419_pred_438_ladder;
                                float frontier_phi_419_pred_438_ladder_1;
                                float frontier_phi_419_pred_438_ladder_2;
                                float frontier_phi_419_pred_438_ladder_3;
                                float frontier_phi_419_pred_438_ladder_4;
                                float frontier_phi_419_pred_438_ladder_5;
                                if (_9691 < 9.9999997473787516355514526367188e-06f)
                                {
                                    frontier_phi_419_pred_438_ladder = _9132;
                                    frontier_phi_419_pred_438_ladder_1 = _9130;
                                    frontier_phi_419_pred_438_ladder_2 = _9131;
                                    frontier_phi_419_pred_438_ladder_3 = _9133;
                                    frontier_phi_419_pred_438_ladder_4 = _9134;
                                    frontier_phi_419_pred_438_ladder_5 = _9135;
                                }
                                else
                                {
                                    float _9943 = clamp((clamp(dot(float3(((_9212 * _9207) - (_9209 * _9288)) * 2.0f, ((_9209 * _9207) - _9283) * 2.0f, (((_9204 * _9288) - (_9207 * _9207)) * 2.0f) + 1.0f), float3((-0.0f) - _9335, (-0.0f) - _9336, (-0.0f) - _9337)), 0.0f, 1.0f) - _9217) / (_9214 - _9217), 0.0f, 1.0f);
                                    float _9947 = (_9943 * _9943) * (3.0f - (_9943 * 2.0f));
                                    float _9949 = (_9947 * _9947) * _9691;
                                    float frontier_phi_419_pred_438_ladder_448_ladder;
                                    float frontier_phi_419_pred_438_ladder_448_ladder_1;
                                    float frontier_phi_419_pred_438_ladder_448_ladder_2;
                                    float frontier_phi_419_pred_438_ladder_448_ladder_3;
                                    float frontier_phi_419_pred_438_ladder_448_ladder_4;
                                    float frontier_phi_419_pred_438_ladder_448_ladder_5;
                                    if (_9949 < 9.9999997473787516355514526367188e-06f)
                                    {
                                        frontier_phi_419_pred_438_ladder_448_ladder = _9132;
                                        frontier_phi_419_pred_438_ladder_448_ladder_1 = _9130;
                                        frontier_phi_419_pred_438_ladder_448_ladder_2 = _9131;
                                        frontier_phi_419_pred_438_ladder_448_ladder_3 = _9133;
                                        frontier_phi_419_pred_438_ladder_448_ladder_4 = _9134;
                                        frontier_phi_419_pred_438_ladder_448_ladder_5 = _9135;
                                    }
                                    else
                                    {
                                        float _10304 = (_9949 * _7385) * max(float(_9202) * 16.0f, 1.0f);
                                        float _10305 = _10304 * _9192;
                                        float _10306 = _10304 * _9195;
                                        float _10307 = _10304 * _9197;
                                        float _10313 = clamp(clamp(dot(float3(_1000, _1004, _1008), float3(_9335, _9336, _9337)), 0.0f, 1.0f), 0.0f, 1.0f) * 0.3183098733425140380859375f;
                                        float _10321 = (_64_m0[122u].w * _1117) * _10313;
                                        frontier_phi_419_pred_438_ladder_448_ladder = ((((1.0f - _1048) * _1035) * _10307) * _10313) + _9132;
                                        frontier_phi_419_pred_438_ladder_448_ladder_1 = ((((1.0f - _1040) * _1025) * _10305) * _10313) + _9130;
                                        frontier_phi_419_pred_438_ladder_448_ladder_2 = ((((1.0f - _1045) * _1030) * _10306) * _10313) + _9131;
                                        frontier_phi_419_pred_438_ladder_448_ladder_3 = ((_10321 * _1040) * _10305) + _9133;
                                        frontier_phi_419_pred_438_ladder_448_ladder_4 = ((_10321 * _1045) * _10306) + _9134;
                                        frontier_phi_419_pred_438_ladder_448_ladder_5 = ((_10321 * _1048) * _10307) + _9135;
                                    }
                                    frontier_phi_419_pred_438_ladder = frontier_phi_419_pred_438_ladder_448_ladder;
                                    frontier_phi_419_pred_438_ladder_1 = frontier_phi_419_pred_438_ladder_448_ladder_1;
                                    frontier_phi_419_pred_438_ladder_2 = frontier_phi_419_pred_438_ladder_448_ladder_2;
                                    frontier_phi_419_pred_438_ladder_3 = frontier_phi_419_pred_438_ladder_448_ladder_3;
                                    frontier_phi_419_pred_438_ladder_4 = frontier_phi_419_pred_438_ladder_448_ladder_4;
                                    frontier_phi_419_pred_438_ladder_5 = frontier_phi_419_pred_438_ladder_448_ladder_5;
                                }
                                frontier_phi_419_pred = frontier_phi_419_pred_438_ladder;
                                frontier_phi_419_pred_1 = frontier_phi_419_pred_438_ladder_1;
                                frontier_phi_419_pred_2 = frontier_phi_419_pred_438_ladder_2;
                                frontier_phi_419_pred_3 = frontier_phi_419_pred_438_ladder_3;
                                frontier_phi_419_pred_4 = frontier_phi_419_pred_438_ladder_4;
                                frontier_phi_419_pred_5 = frontier_phi_419_pred_438_ladder_5;
                            }
                            else
                            {
                                frontier_phi_419_pred = _9132;
                                frontier_phi_419_pred_1 = _9130;
                                frontier_phi_419_pred_2 = _9131;
                                frontier_phi_419_pred_3 = _9133;
                                frontier_phi_419_pred_4 = _9134;
                                frontier_phi_419_pred_5 = _9135;
                            }
                            _7817 = frontier_phi_419_pred;
                            _7811 = frontier_phi_419_pred_1;
                            _7814 = frontier_phi_419_pred_2;
                            _7820 = frontier_phi_419_pred_3;
                            _7823 = frontier_phi_419_pred_4;
                            _7826 = frontier_phi_419_pred_5;
                            if (_9137 > _8346)
                            {
                                break;
                            }
                            else
                            {
                                _9130 = _7811;
                                _9131 = _7814;
                                _9132 = _7817;
                                _9133 = _7820;
                                _9134 = _7823;
                                _9135 = _7826;
                                _9136 = _9137;
                                _9138 = _9142;
                                continue;
                            }
                        }
                        frontier_phi_353_354_ladder_395_ladder = _7830;
                        frontier_phi_353_354_ladder_395_ladder_1 = _7814;
                        frontier_phi_353_354_ladder_395_ladder_2 = _7817;
                        frontier_phi_353_354_ladder_395_ladder_3 = _7820;
                        frontier_phi_353_354_ladder_395_ladder_4 = _7823;
                        frontier_phi_353_354_ladder_395_ladder_5 = _7811;
                        frontier_phi_353_354_ladder_395_ladder_6 = _7826;
                        frontier_phi_353_354_ladder_395_ladder_7 = _7832;
                        frontier_phi_353_354_ladder_395_ladder_8 = _7834;
                        frontier_phi_353_354_ladder_395_ladder_9 = _7836;
                        frontier_phi_353_354_ladder_395_ladder_10 = _7838;
                        frontier_phi_353_354_ladder_395_ladder_11 = _7828;
                    }
                    frontier_phi_353_354_ladder = frontier_phi_353_354_ladder_395_ladder;
                    frontier_phi_353_354_ladder_1 = frontier_phi_353_354_ladder_395_ladder_1;
                    frontier_phi_353_354_ladder_2 = frontier_phi_353_354_ladder_395_ladder_2;
                    frontier_phi_353_354_ladder_3 = frontier_phi_353_354_ladder_395_ladder_3;
                    frontier_phi_353_354_ladder_4 = frontier_phi_353_354_ladder_395_ladder_4;
                    frontier_phi_353_354_ladder_5 = frontier_phi_353_354_ladder_395_ladder_5;
                    frontier_phi_353_354_ladder_6 = frontier_phi_353_354_ladder_395_ladder_6;
                    frontier_phi_353_354_ladder_7 = frontier_phi_353_354_ladder_395_ladder_7;
                    frontier_phi_353_354_ladder_8 = frontier_phi_353_354_ladder_395_ladder_8;
                    frontier_phi_353_354_ladder_9 = frontier_phi_353_354_ladder_395_ladder_9;
                    frontier_phi_353_354_ladder_10 = frontier_phi_353_354_ladder_395_ladder_10;
                    frontier_phi_353_354_ladder_11 = frontier_phi_353_354_ladder_395_ladder_11;
                }
                _7809 = frontier_phi_353_354_ladder_5;
                _7812 = frontier_phi_353_354_ladder_1;
                _7815 = frontier_phi_353_354_ladder_2;
                _7818 = frontier_phi_353_354_ladder_3;
                _7821 = frontier_phi_353_354_ladder_4;
                _7824 = frontier_phi_353_354_ladder_6;
                _7827 = frontier_phi_353_354_ladder_11;
                _7829 = frontier_phi_353_354_ladder;
                _7831 = frontier_phi_353_354_ladder_7;
                _7833 = frontier_phi_353_354_ladder_8;
                _7835 = frontier_phi_353_354_ladder_9;
                _7837 = frontier_phi_353_354_ladder_10;
            }
            float _7840 = (_2633 * _4689) * ((_64_m0[98u].x * (_2391 + (-1.0f))) + 1.0f);
            float _7841 = _2397 * _4689;
            float _8471;
            float _8474;
            float _8477;
            if ((_1174 & 4194304u) == 0u)
            {
                float frontier_phi_376_366_ladder;
                float frontier_phi_376_366_ladder_1;
                float frontier_phi_376_366_ladder_2;
                if ((_1174 & 67108864u) == 0u)
                {
                    float frontier_phi_376_366_ladder_374_ladder;
                    float frontier_phi_376_366_ladder_374_ladder_1;
                    float frontier_phi_376_366_ladder_374_ladder_2;
                    if ((_1174 & 50331648u) == 0u)
                    {
                        frontier_phi_376_366_ladder_374_ladder = (1.0f - _1048) * _1035;
                        frontier_phi_376_366_ladder_374_ladder_1 = (1.0f - _1045) * _1030;
                        frontier_phi_376_366_ladder_374_ladder_2 = (1.0f - _1040) * _1025;
                    }
                    else
                    {
                        float _8614 = ((_1051 * 0.75f) + 1.25f) + ((((_1163 * _1163) * 9000.0f) * _1161) * _1163);
                        float _8638 = (_1167 * 0.3183098733425140380859375f) * (sqrt(((_1157 * _1157) + (_1155 * _1155)) + (_1159 * _1159)) + 1.0f);
                        frontier_phi_376_366_ladder_374_ladder = exp2(log2(exp2(_1159 * (-4.616624355316162109375f))) * _8614) * _8638;
                        frontier_phi_376_366_ladder_374_ladder_1 = exp2(log2(exp2(_1157 * (-4.616624355316162109375f))) * _8614) * _8638;
                        frontier_phi_376_366_ladder_374_ladder_2 = exp2(log2(exp2(_1155 * (-4.616624355316162109375f))) * _8614) * _8638;
                    }
                    frontier_phi_376_366_ladder = frontier_phi_376_366_ladder_374_ladder;
                    frontier_phi_376_366_ladder_1 = frontier_phi_376_366_ladder_374_ladder_1;
                    frontier_phi_376_366_ladder_2 = frontier_phi_376_366_ladder_374_ladder_2;
                }
                else
                {
                    float _8467 = (((_39[NonUniformResourceIndex(_1151 + 0u)].SampleLevel(_76, float2(_1135, _1137), 0.0f).x * _1147) + (-1.0f)) * _1145) + 1.0f;
                    frontier_phi_376_366_ladder = ((1.0f - _1048) * _1035) * _8467;
                    frontier_phi_376_366_ladder_1 = ((1.0f - _1045) * _1030) * _8467;
                    frontier_phi_376_366_ladder_2 = ((1.0f - _1040) * _1025) * _8467;
                }
                _8471 = frontier_phi_376_366_ladder_2;
                _8474 = frontier_phi_376_366_ladder_1;
                _8477 = frontier_phi_376_366_ladder;
            }
            else
            {
                _8471 = ((1.0f - _1040) - (_1099 * (_1096 - _1040))) * _1025;
                _8474 = ((1.0f - _1045) - (_1099 * (_1097 - _1045))) * _1030;
                _8477 = ((1.0f - _1048) - (_1099 * (_1098 - _1048))) * _1035;
            }
            _4672 = ((((_6279 * _1720) * _6994) + ((_6276 * _1720) * _6532)) + ((_7833 * _7841) * _8471)) + ((_7827 + _7809) * _7840);
            _4674 = ((((_6279 * _1721) * _6999) + ((_6276 * _1721) * _6537)) + ((_7835 * _7841) * _8474)) + ((_7829 + _7812) * _7840);
            _4676 = ((((_6279 * _1722) * _7004) + ((_6276 * _1722) * _6542)) + ((_7837 * _7841) * _8477)) + ((_7831 + _7815) * _7840);
            _4678 = (_7818 * _7840) + ((_6277 * _1720) * (_6565 + _6547));
            _4680 = (_7821 * _7840) + ((_6277 * _1721) * (_6565 + _6553));
            _4682 = (_7824 * _7840) + ((_6277 * _1722) * (_6565 + _6559));
        }
        float _1334;
        uint _1336;
        uint _1338;
        float _1340;
        float _1342;
        float _1344;
        float _6081;
        float _6083;
        float _6085;
        bool _4684;
        float _4685;
        float _4686;
        float _4687;
        for (;;)
        {
            _4684 = _459 == 0u;
            _4685 = _4504 + _4672;
            _4686 = _4505 + _4674;
            _4687 = _4506 + _4676;
            uint _5093;
            uint _5095;
            float _5097;
            float _5098;
            float _5099;
            float _5100;
            float _5102;
            float _5104;
            if (_4684)
            {
                float _4878 = (_4678 + _4501) + _4685;
                float _4880 = (_4680 + _4502) + _4686;
                float _4882 = (_4682 + _4503) + _4687;
                if (!((_1396 != 0u) && _1177))
                {
                    float _5189 = _385 - _64_m0[8u].x;
                    float _5190 = _386 - _64_m0[8u].y;
                    float _5191 = _387 - _64_m0[8u].z;
                    float _5195 = rsqrt(dot(float3(_5189, _5190, _5191), float3(_5189, _5190, _5191)));
                    float _5197 = _5190 * _5195;
                    float _5204 = sqrt(((_5189 * _5189) + (_5190 * _5190)) + (_5191 * _5191));
                    float4 _5220 = _15[584u].SampleLevel(_72, float2((_385 - _64_m0[111u].x) * _64_m0[111u].z, 1.0f - ((_387 - _64_m0[111u].y) * _64_m0[111u].w)), 0.0f);
                    float4 _5253 = _24.SampleLevel(_75, float3(_5189 * _5195, _5197, _5191 * _5195), (1.0f - clamp((_5204 - _69_m0[55u].x) / (_69_m0[55u].y - _69_m0[55u].x), 0.0f, 1.0f)) * _69_m0[55u].z);
                    float _5280 = (((clamp((((_64_m0[112u].x - _386) - _64_m0[112u].w) + (_64_m0[112u].y * _5220.x)) / _64_m0[112u].z, 0.0f, 1.0f) * clamp(_5220.y, 0.0f, 1.0f)) * (_69_m0[53u].z - _69_m0[53u].y)) + _69_m0[53u].y) * (1.0f - exp2((-0.0f) - (_69_m0[53u].w * min(1000000.0f, max(0.0f, _5204 - _69_m0[53u].x)))));
                    float _5546;
                    if (_69_m0[54u].y > 0.0f)
                    {
                        float _5544 = min(1000000.0f, max(0.0f, _5204 - _69_m0[54u].w)) * 0.001000000047497451305389404296875f;
                        float _6114;
                        if (_5197 == 0.0f)
                        {
                            _6114 = _5544;
                        }
                        else
                        {
                            float _6119 = _69_m0[54u].y * _5197;
                            _6114 = (1.0f - exp2((-0.0f) - (_6119 * _5544))) / _6119;
                        }
                        _5546 = clamp((_6114 * _69_m0[54u].x) + _5280, 0.0f, 1.0f);
                    }
                    else
                    {
                        _5546 = _5280;
                    }
                    float _5554 = (_5546 * ((_64_m0[59u].z * _5253.x) - _4878)) + _4878;
                    float _5555 = (_5546 * ((_64_m0[59u].z * _5253.y) - _4880)) + _4880;
                    float _5556 = (_5546 * ((_64_m0[59u].z * _5253.z) - _4882)) + _4882;
                    if (asuint(_69_m0[152u]).w == 0u)
                    {
                        _1340 = 0.0f;
                        _1342 = 0.0f;
                        _1344 = 0.0f;
                        _1336 = 0u;
                        _1338 = 0u;
                        _1334 = _5546;
                        _6081 = _5554;
                        _6083 = _5555;
                        _6085 = _5556;
                        break;
                    }
                    float _6141 = ((-0.0f) - _69_m0[151u].z) / (_69_m0[150u].w * ((1.0f - _309) - _69_m0[151u].w));
                    float _6241;
                    if (_69_m0[152u].z < 0.001000000047497451305389404296875f)
                    {
                        _6241 = _6141;
                    }
                    else
                    {
                        _6241 = log2((_69_m0[152u].z * _6141) + 1.0f) / log2(_69_m0[152u].z + 1.0f);
                    }
                    float4 _6246 = _58.SampleLevel(_73, float3(_312 / (_69_m0[151u].x + (-1.0f)), _314 / (_69_m0[151u].y + (-1.0f)), _6241), 0.0f);
                    float _6251 = _6246.w;
                    _1340 = 0.0f;
                    _1342 = 0.0f;
                    _1344 = 0.0f;
                    _1336 = 0u;
                    _1338 = 0u;
                    _1334 = (_6251 * (_5546 + (-1.0f))) + 1.0f;
                    _6081 = (_6251 * _5554) + (_6246.x / _69_m0[152u].x);
                    _6083 = (_6246.y / _69_m0[152u].x) + (_6251 * _5555);
                    _6085 = (_6246.z / _69_m0[152u].x) + (_6251 * _5556);
                    break;
                }
                _5093 = 0u;
                _5095 = 0u;
                _5097 = 0.0f;
                _5098 = 0.0f;
                _5099 = 0.0f;
                _5100 = _4878;
                _5102 = _4880;
                _5104 = _4882;
            }
            else
            {
                uint4 _4893 = asuint(_69_m0[1148u]);
                uint _4894 = _4893.w;
                float _5285;
                uint _5287;
                if (_4894 == 0u)
                {
                    _5285 = _1121;
                    _5287 = _1119;
                }
                else
                {
                    _5285 = _69_m0[1148u].z;
                    _5287 = _4894 + 4294967295u;
                }
                _5093 = uint(clamp(_5285, 0.0f, 1.0f) * 255.0f);
                _5095 = (_5287 & 63u) | 128u;
                _5097 = _64_m0[59u].x * _4685;
                _5098 = _64_m0[59u].x * _4686;
                _5099 = _64_m0[59u].x * _4687;
                _5100 = _4678 + _4501;
                _5102 = _4680 + _4502;
                _5104 = _4682 + _4503;
            }
            float _5111 = _385 - _64_m0[8u].x;
            float _5112 = _386 - _64_m0[8u].y;
            float _5113 = _387 - _64_m0[8u].z;
            float4 _5131 = _15[584u].SampleLevel(_72, float2((_385 - _64_m0[111u].x) * _64_m0[111u].z, 1.0f - ((_387 - _64_m0[111u].y) * _64_m0[111u].w)), 0.0f);
            float _5154 = sqrt(((_5111 * _5111) + (_5112 * _5112)) + (_5113 * _5113));
            float _5159 = rsqrt(dot(float3(_5111, _5112, _5113), float3(_5111, _5112, _5113))) * _5112;
            float _5178 = (((clamp((((_64_m0[112u].x - _386) - _64_m0[112u].w) + (_64_m0[112u].y * _5131.x)) / _64_m0[112u].z, 0.0f, 1.0f) * clamp(_5131.y, 0.0f, 1.0f)) * (_69_m0[53u].z - _69_m0[53u].y)) + _69_m0[53u].y) * (1.0f - exp2((-0.0f) - (_69_m0[53u].w * min(1000000.0f, max(0.0f, _5154 - _69_m0[53u].x)))));
            float _5532;
            if (_69_m0[54u].y > 0.0f)
            {
                float _5530 = min(1000000.0f, max(0.0f, _5154 - _69_m0[54u].w)) * 0.001000000047497451305389404296875f;
                float _6069;
                if (_5159 == 0.0f)
                {
                    _6069 = _5530;
                }
                else
                {
                    float _6074 = _69_m0[54u].y * _5159;
                    _6069 = (1.0f - exp2((-0.0f) - (_6074 * _5530))) / _6074;
                }
                _5532 = clamp((_6069 * _69_m0[54u].x) + _5178, 0.0f, 1.0f);
            }
            else
            {
                _5532 = _5178;
            }
            if (asuint(_69_m0[152u]).w == 0u)
            {
                _1340 = _5097;
                _1342 = _5098;
                _1344 = _5099;
                _1336 = _5093;
                _1338 = _5095;
                _1334 = _5532;
                _6081 = _5100;
                _6083 = _5102;
                _6085 = _5104;
                break;
            }
            float _6109 = ((-0.0f) - _69_m0[151u].z) / (_69_m0[150u].w * ((1.0f - _309) - _69_m0[151u].w));
            float _6226;
            if (_69_m0[152u].z < 0.001000000047497451305389404296875f)
            {
                _6226 = _6109;
            }
            else
            {
                _6226 = log2((_69_m0[152u].z * _6109) + 1.0f) / log2(_69_m0[152u].z + 1.0f);
            }
            _1340 = _5097;
            _1342 = _5098;
            _1344 = _5099;
            _1336 = _5093;
            _1338 = _5095;
            _1334 = (_58.SampleLevel(_73, float3(_312 / (_69_m0[151u].x + (-1.0f)), _314 / (_69_m0[151u].y + (-1.0f)), _6226), 0.0f).w * (_5532 + (-1.0f))) + 1.0f;
            _6081 = _5100;
            _6083 = _5102;
            _6085 = _5104;
            break;
        }
        _1333 = _1334;
        _1335 = _1336;
        _1337 = _1338;
        _1339 = _1340;
        _1341 = _1342;
        _1343 = _1344;
        _1345 = _64_m0[59u].x * _6081;
        _1347 = _64_m0[59u].x * _6083;
        _1349 = _64_m0[59u].x * _6085;
    }
    float _1358 = clamp((_1333 - _64_m0[94u].z) / (_64_m0[94u].w - _64_m0[94u].z), 0.0f, 1.0f);
    SV_Target_5.x = _1358;
    SV_Target_5.y = _1358;
    SV_Target_5.z = _1358;
    SV_Target_5.w = 1.0f;
    float _1608;
    float _1610;
    float _1612;
    if ((asuint(_1345) & 2139095040u) == 2139095040u)
    {
        _1608 = 0.0f;
        _1610 = 0.0f;
        _1612 = 0.0f;
    }
    else
    {
        float frontier_phi_36_37_ladder;
        float frontier_phi_36_37_ladder_1;
        float frontier_phi_36_37_ladder_2;
        if ((asuint(_1347) & 2139095040u) == 2139095040u)
        {
            frontier_phi_36_37_ladder = 0.0f;
            frontier_phi_36_37_ladder_1 = 0.0f;
            frontier_phi_36_37_ladder_2 = 0.0f;
        }
        else
        {
            bool _1717 = (asuint(_1349) & 2139095040u) == 2139095040u;
            frontier_phi_36_37_ladder = _1717 ? 0.0f : _1345;
            frontier_phi_36_37_ladder_1 = _1717 ? 0.0f : _1349;
            frontier_phi_36_37_ladder_2 = _1717 ? 0.0f : _1347;
        }
        _1608 = frontier_phi_36_37_ladder;
        _1610 = frontier_phi_36_37_ladder_2;
        _1612 = frontier_phi_36_37_ladder_1;
    }
    SV_Target_1.x = _1339;
    SV_Target_1.y = _1341;
    SV_Target_1.z = _1343;
    SV_Target_1.w = 1.0f;
    SV_Target_2.x = _1335;
    SV_Target_2.y = _1337;
    SV_Target_2.z = 0u;
    SV_Target_2.w = 1u;
    SV_Target.x = _1608;
    SV_Target.y = _1610;
    SV_Target.z = _1612;
    SV_Target.w = 1.0f;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    gl_FragCoord = stage_input.gl_FragCoord;
    gl_FragCoord.w = 1.0 / gl_FragCoord.w;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.SV_Target = SV_Target;
    stage_output.SV_Target_1 = SV_Target_1;
    stage_output.SV_Target_2 = SV_Target_2;
    stage_output.SV_Target_5 = SV_Target_5;
    return stage_output;
}
