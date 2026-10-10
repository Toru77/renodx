static uint _1655;
static float _1656;

cbuffer _22_24 : register(b0, space2)
{
    float4 _24_m0[1] : packoffset(c0);
};

struct _27
{
    float4 _m0[4096];
};

ConstantBuffer<_27> _30[] : register(b0, space4);
cbuffer _33_35 : register(b13, space0)
{
    float4 _35_m0[9] : packoffset(c0);
};

cbuffer _37_39 : register(b15, space0)
{
    float4 _39_m0[1] : packoffset(c0);
};

cbuffer _42_44 : register(b9, space0)
{
    float4 _44_m0[25] : packoffset(c0);
};

cbuffer _47_49 : register(b2, space0)
{
    float4 _49_m0[700] : packoffset(c0);
};

cbuffer _52_54 : register(b7, space0)
{
    float4 _54_m0[65] : packoffset(c0);
};

cbuffer _57_59 : register(b3, space0)
{
    float4 _59_m0[1145] : packoffset(c0);
};

Texture2D<float4> _9[] : register(t0, space1);
Buffer<uint4> _13 : register(t46, space0);
Buffer<uint4> _14 : register(t59, space0);
Texture2D<float4> _16 : register(t88, space0);
Texture2D<float4> _17 : register(t89, space0);
Texture2D<float4> _18 : register(t91, space0);
SamplerState _63[] : register(s0, space1);
SamplerState _65 : register(s1, space0);

static float4 gl_FragCoord;
static float2 TEXCOORD;
static float2 TEXCOORD_1;
static float3 WORLDPOS;
static float3 WORLDNORMAL;
static float TEXCOORD_4;
static float TEXCOORD4y;
static float4 WORLDTANGENT;
static float4 INSTANCE_VARIATION;
static float4 MOTIONBLUR_POS;
static float4 MOTIONBLUR_POS_1;
static float DITHERED_ALPHA;
static uint INSTANCE_INDEX;
static float4 SV_Target;
static float4 SV_Target_1;
static uint4 SV_Target_2;
static uint SV_Target_3;
static float4 SV_Target_4;
static float4 SV_Target_5;

struct SPIRV_Cross_Input
{
    float2 TEXCOORD : TEXCOORD0;
    float2 TEXCOORD_1 : TEXCOORD0;
    float3 WORLDPOS : TEXCOORD1;
    float TEXCOORD_4 : TEXCOORD1;
    centroid float3 WORLDNORMAL : TEXCOORD2;
    float TEXCOORD4y : TEXCOORD3;
    float4 WORLDTANGENT : TEXCOORD4;
    nointerpolation float4 INSTANCE_VARIATION : TEXCOORD6;
    float4 MOTIONBLUR_POS : TEXCOORD7;
    float4 MOTIONBLUR_POS_1 : TEXCOORD8;
    nointerpolation float DITHERED_ALPHA : TEXCOORD9;
    nointerpolation uint INSTANCE_INDEX : TEXCOORD9;
    float4 gl_FragCoord : SV_Position;
};

struct SPIRV_Cross_Output
{
    float4 SV_Target : SV_Target0;
    float4 SV_Target_1 : SV_Target1;
    uint4 SV_Target_2 : SV_Target2;
    uint SV_Target_3 : SV_Target3;
    float4 SV_Target_4 : SV_Target4;
    float4 SV_Target_5 : SV_Target5;
};

void frag_main()
{
    uint4 _168 = asuint(_39_m0[0u]);
    float _170 = (-0.0f) - WORLDPOS.x;
    float _172 = (-0.0f) - WORLDPOS.y;
    float _173 = (-0.0f) - WORLDPOS.z;
    float _178 = rsqrt(dot(float3(_170, _172, _173), float3(_170, _172, _173)));
    uint4 _186 = asuint(_35_m0[7u]);
    uint _187 = _186.z;
    uint _188 = INSTANCE_INDEX / _187;
    uint _189 = INSTANCE_INDEX % _187;
    uint _197;
    uint _199;
    if (_187 > 1u)
    {
        uint _191 = _168.z;
        uint _192 = _188 + _191;
        uint _404;
        if (_13.Load(_192).x > INSTANCE_INDEX)
        {
            _404 = _192;
        }
        else
        {
            uint _747 = _192;
            uint _748 = 0u;
            uint _405;
            for (;;)
            {
                _405 = (_747 + 1u) + ((INSTANCE_INDEX - _13.Load(_747).x) / _187);
                if (_13.Load(_405).x > INSTANCE_INDEX)
                {
                    break;
                }
                else
                {
                    uint _749 = _748 + 1u;
                    if (_749 < 256u)
                    {
                        _747 = _405;
                        _748 = _749;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            _404 = _405;
        }
        uint _198 = _404 - _191;
        uint frontier_phi_2_3_ladder;
        uint frontier_phi_2_3_ladder_1;
        if (_404 == _191)
        {
            frontier_phi_2_3_ladder = _189;
            frontier_phi_2_3_ladder_1 = _198;
        }
        else
        {
            frontier_phi_2_3_ladder = INSTANCE_INDEX - _13.Load(_404 + 4294967295u).x;
            frontier_phi_2_3_ladder_1 = _198;
        }
        _197 = frontier_phi_2_3_ladder_1;
        _199 = frontier_phi_2_3_ladder;
    }
    else
    {
        _197 = _188;
        _199 = _189;
    }
    uint _211 = min((_13.Load(_197 + _168.x).x + _199), asuint(_49_m0[119u]).z);
    uint _213 = _211 * 16u;
    uint3 _224 = uint3(_14.Load(_213).x, _14.Load(_213 + 1u).x, _14.Load(_213 + 2u).x);
    uint _225 = _224.x;
    uint _226 = _224.y;
    uint _227 = _224.z;
    uint _229 = (_211 * 16u) + 3u;
    float3 _239 = asfloat(uint3(_14.Load(_229).x, _14.Load(_229 + 1u).x, _14.Load(_229 + 2u).x));
    float _240 = _239.x;
    float _242 = _239.z;
    uint _245 = (_211 * 16u) + 6u;
    float3 _255 = asfloat(uint3(_14.Load(_245).x, _14.Load(_245 + 1u).x, _14.Load(_245 + 2u).x));
    uint _260 = (_211 * 16u) + 9u;
    float3 _270 = asfloat(uint3(_14.Load(_260).x, _14.Load(_260 + 1u).x, _14.Load(_260 + 2u).x));
    float _271 = _270.x;
    float _306 = ((float(_226 & 65535u) * 1.525902189314365386962890625e-05f) + float((_226 >> 16u) & 32767u)) * float(int(uint(int(_226) >> int(31u)) | 1u));
    uint _317 = _168.w + 0u;
    float _327 = _271 * _239.y;
    float _333 = rsqrt(dot(float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z), float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z)));
    float _334 = _333 * WORLDNORMAL.x;
    float _335 = _333 * WORLDNORMAL.y;
    float _336 = _333 * WORLDNORMAL.z;
    float _340 = rsqrt(dot(float3(WORLDTANGENT.x, WORLDTANGENT.y, WORLDTANGENT.z), float3(WORLDTANGENT.x, WORLDTANGENT.y, WORLDTANGENT.z)));
    float _341 = _340 * WORLDTANGENT.x;
    float _342 = _340 * WORLDTANGENT.y;
    float _343 = _340 * WORLDTANGENT.z;
    uint _372 = uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 13u;
    float _379 = _30[_317]._m0[_372].z + TEXCOORD.x;
    float _380 = _30[_317]._m0[_372].w + TEXCOORD.y;
    uint _388 = uint(int(asuint(_24_m0[0u]).x) >> int(16u));
    float _407;
    if (asuint(_30[_317]._m0[_388 + 41u].z) == 0u)
    {
        _407 = _380;
    }
    else
    {
        _407 = fmod(_30[_317]._m0[_388 + 26u].x * ((((((_255.x * _242) - (_255.z * _240)) * _270.z) + _327) + _306) + (((asuint(_30[_317]._m0[uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 41u].w) != 0u) ? 1.0f : _255.y) * _270.y)), 1.0f) + _380;
    }
    uint _408 = _388 + 13u;
    float4 _434 = _9[asuint(_30[_317]._m0[uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 51u].x) + 0u].Sample(_65, float2(_30[_317]._m0[_408].x * TEXCOORD_1.x, _30[_317]._m0[_408].y * TEXCOORD_1.y));
    uint _448 = uint(int(asuint(_24_m0[0u]).x) >> int(16u));
    uint _449 = _448 + 2u;
    uint _455 = _448 + 3u;
    float4 _481 = _9[asuint(_30[_317]._m0[uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 55u].z) + 0u].Sample(_65, float2(_30[_317]._m0[_455].x + (_30[_317]._m0[_449].x * (_49_m0[8u].x + WORLDPOS.x)), _30[_317]._m0[_455].y + (_30[_317]._m0[_449].y * (_49_m0[8u].z + WORLDPOS.z))));
    float _484 = _481.w;
    uint _497 = asuint(_30[_317]._m0[uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 50u].x) + 0u;
    uint _502 = uint(int(asuint(_24_m0[0u]).x) >> int(16u));
    uint _507 = _502 + 14u;
    uint _549 = uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 15u;
    float4 _563 = _9[asuint(_30[_317]._m0[uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 54u].x) + 0u].Sample(_63[_497], float2(_30[_317]._m0[_549].x * _379, _30[_317]._m0[_549].y * _407));
    float _570 = (_563.x * 2.0f) + (-1.0f);
    float _572 = (_563.y * 2.0f) + (-1.0f);
    float _573 = (-0.0f) - _572;
    float _579 = sqrt(clamp((1.0f - (_570 * _570)) - (_572 * _572), 0.0f, 1.0f));
    float _589 = ((_343 * _335) - (_342 * _336)) * WORLDTANGENT.w;
    float _590 = ((_341 * _336) - (_343 * _334)) * WORLDTANGENT.w;
    float _591 = ((_342 * _334) - (_341 * _335)) * WORLDTANGENT.w;
    float _595 = rsqrt(dot(float3(_589, _590, _591), float3(_589, _590, _591)));
    float _601 = mad(_579, _334, mad(_573, _595 * _589, _570 * _341));
    float _604 = mad(_579, _335, mad(_573, _595 * _590, _570 * _342));
    float _607 = mad(_579, _336, mad(_573, _595 * _591, _570 * _343));
    float _611 = rsqrt(dot(float3(_601, _604, _607), float3(_601, _604, _607)));
    float _612 = _611 * _601;
    float _613 = _611 * _604;
    float _614 = _611 * _607;
    uint _620 = uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 15u;
    float4 _633 = _9[asuint(_30[_317]._m0[_502 + 52u].z) + 0u].Sample(_63[_497], float2(_30[_317]._m0[_620].z * _379, _30[_317]._m0[_620].w * _407));
    uint _644 = uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 17u;
    float4 _658 = _9[asuint(_30[_317]._m0[uint(int(asuint(_24_m0[0u]).x) >> int(16u)) + 54u].z) + 0u].Sample(_63[_497], float2(_30[_317]._m0[_644].x * _379, _30[_317]._m0[_644].y * _407));
    float _661 = _658.y;
    float _663 = _658.w;
    float _664 = clamp(_658.x, 0.0f, 1.0f);
    uint _669 = uint(int(asuint(_24_m0[0u]).x) >> int(16u));
    float _685 = log2(clamp(_658.z, 0.0f, 1.0f));
    float _689 = (exp2(_685 * abs(_49_m0[93u].y)) + (-1.0f)) * _49_m0[93u].x;
    float _704 = (((exp2(log2(clamp(_434.x, 0.0f, 1.0f)) * abs(_49_m0[92u].y)) + (-1.0f)) * _49_m0[92u].x) + 1.0f) * (_689 + 1.0f);
    float _724 = min(clamp((1.21000003814697265625f / (exp2((_689 + 0.53999996185302734375f) * (-7.213470458984375f)) + 1.0f)) + (-0.108900010585784912109375f), 0.0f, 1.0f), 1.0f);
    float _735 = min(_481.z, _484);
    float _787;
    if ((dot(_484.xxxx, 1.0f.xxxx) > 0.00999999977648258209228515625f) && ((_54_m0[2u].x > 0.00999999977648258209228515625f) || (_735 > 0.00999999977648258209228515625f)))
    {
        float _771 = ((clamp((_563.w - _30[_317]._m0[_507].z) / (_30[_317]._m0[_507].w - _30[_317]._m0[_507].z), 0.0f, 1.0f) * 2.0f) + (-1.0f)) * _30[_317]._m0[_502 + 26u].y;
        float _785 = clamp(clamp((((_735 + (-0.699999988079071044921875f)) + ((max(_484, _484) - _735) * _54_m0[2u].x)) + ((_771 * (-0.300000011920928955078125f)) * (2.0f - abs((-0.0f) - _771)))) * 3.3333332538604736328125f, 0.0f, 1.0f), 0.0f, 1.0f);
        float frontier_phi_10_9_ladder;
        if (_785 > 0.00999999977648258209228515625f)
        {
            float _1122 = clamp(_49_m0[91u].y + (-9.9999997473787516355514526367188e-05f), 0.0f, 1.0f);
            frontier_phi_10_9_ladder = clamp((_785 * 6.666666507720947265625f) * clamp((_335 - _1122) / (1.0f - _1122), 0.0f, 1.0f), 0.0f, 1.0f);
        }
        else
        {
            frontier_phi_10_9_ladder = 0.0f;
        }
        _787 = frontier_phi_10_9_ladder;
    }
    else
    {
        _787 = 0.0f;
    }
    uint _799 = _669 + 33u;
    float _805 = clamp(_54_m0[2u].x / _30[_317]._m0[_799].x, 0.0f, 1.0f);
    uint _818 = _669 + 34u;
    float _831 = 1.0f - _661;
    float _832 = clamp(max(((clamp((_30[_317]._m0[_799].z * _704) - _30[_317]._m0[_799].w, 0.0f, 1.0f) + (clamp((_805 + (-0.5f)) * 2.0f, 0.0f, 1.0f) * _30[_317]._m0[_799].y)) * _805) * clamp(((_30[_317]._m0[_818].x + (-1.0f)) * _613) + 1.0f, 0.0f, 1.0f), 0.0f), 0.0f, 1.0f) * _831;
    float _833 = _663 * 0.699999988079071044921875f;
    float _836 = clamp(_832 / _833, 0.0f, 1.0f);
    float _841 = 1.0f - _836;
    float _849 = clamp(exp2(log2(_633.x) * 0.4545449912548065185546875f), 0.0f, 1.0f);
    float _850 = clamp(exp2(log2(_633.y) * 0.4545449912548065185546875f), 0.0f, 1.0f);
    float _851 = clamp(exp2(log2(_633.z) * 0.4545449912548065185546875f), 0.0f, 1.0f);
    float _852 = clamp(((_841 - exp2(log2(_841) * _49_m0[144u].x)) * _663) + _836, 0.0f, 1.0f) * _663;
    float _890 = clamp(clamp((_832 - _833) * 3.400000095367431640625f, 0.0f, 1.0f), 0.0f, 1.0f);
    float _902 = log2(_890);
    float _908 = exp2(_902 * 0.839999973773956298828125f);
    float _925 = 0.039999999105930328369140625f - (_890 * 0.0199999995529651641845703125f);
    float _934 = exp2(log2(((clamp((exp2(log2(_849) * 1.65999996662139892578125f) * 0.96700000762939453125f) + 0.032999999821186065673828125f, 0.0f, 1.0f) - _849) * _852) + _849) * 2.2000000476837158203125f);
    float _935 = exp2(log2(((clamp((exp2(log2(_850) * 1.65999996662139892578125f) * 0.96700000762939453125f) + 0.032999999821186065673828125f, 0.0f, 1.0f) - _850) * _852) + _850) * 2.2000000476837158203125f);
    float _936 = exp2(log2(((clamp((exp2(log2(_851) * 1.65999996662139892578125f) * 0.96700000762939453125f) + 0.032999999821186065673828125f, 0.0f, 1.0f) - _851) * _852) + _851) * 2.2000000476837158203125f);
    uint _944 = uint(int(asuint(_24_m0[0u]).x) >> int(16u));
    uint _945 = _944 + 45u;
    bool _952 = asuint(_30[_317]._m0[_945].x) != 0u;
    float _959 = clamp(INSTANCE_VARIATION.w * 25.6000003814697265625f, 0.0f, 1.0f) * ((INSTANCE_VARIATION.x * 25.6000003814697265625f) + (-12.80000019073486328125f));
    float _960 = _959 * _271;
    uint _983 = _944 + 7u;
    uint _989 = _944 + 8u;
    float _996 = (frac(((ceil((_959 * _327) + _306) * 10.47399997711181640625f) + (ceil((((float(_225 & 65535u) * 1.525902189314365386962890625e-05f) + float((_225 >> 16u) & 32767u)) * float(int(uint(int(_225) >> int(31u)) | 1u))) + (_960 * _240)) * 14.85299968719482421875f)) + (ceil((((float(_227 & 65535u) * 1.525902189314365386962890625e-05f) + float((_227 >> 16u) & 32767u)) * float(int(uint(int(_227) >> int(31u)) | 1u))) + (_960 * _242)) * 11.125f)) * 2.0f) + (-1.0f);
    float _997 = clamp(_996, 0.0f, 1.0f);
    float _999 = clamp((-0.0f) - _996, 0.0f, 1.0f);
    float _1006 = (_997 * (_30[_317]._m0[_983].x + (-1.0f))) + 1.0f;
    float _1007 = (_997 * (_30[_317]._m0[_983].y + (-1.0f))) + 1.0f;
    float _1008 = (_997 * (_30[_317]._m0[_983].z + (-1.0f))) + 1.0f;
    float _1018 = (((_30[_317]._m0[_989].x - _1006) * _999) + _1006) * 0.57735025882720947265625f;
    float _1020 = (((_30[_317]._m0[_989].y - _1007) * _999) + _1007) * 0.57735025882720947265625f;
    float _1021 = (((_30[_317]._m0[_989].z - _1008) * _999) + _1008) * 0.57735025882720947265625f;
    float _1028 = 1.73205077648162841796875f / sqrt(((_1018 * _1018) + (_1020 * _1020)) + (_1021 * _1021));
    uint _1036 = _944 + 34u;
    float4 _1063 = _9[asuint(_30[_317]._m0[_944 + 53u].z) + 0u].Sample(_65, float2(TEXCOORD_4, TEXCOORD4y));
    uint _1072 = uint(int(asuint(_24_m0[0u]).x) >> int(16u));
    uint _1073 = _1072 + 35u;
    float _1100 = (1.0f - _664) - ((clamp(((((exp2(_902 * 0.115000002086162567138671875f) - _908) * (1.0f / (exp2(log2((1.0f - _890) / _890) * 2.8169014453887939453125f) + 1.0f))) + _908) * (1.0f - (_663 * 0.2520000040531158447265625f))) + (min(_832, _833) * 0.36000001430511474609375f), 0.0f, 1.0f) * _30[_317]._m0[_818].y) * (min(_664, (1.0f - _49_m0[140u].y) * _664) - _664));
    float _1107 = ((_934 - _925) * _661) + _925;
    float _1108 = ((_935 - _925) * _661) + _925;
    float _1109 = ((_936 - _925) * _661) + _925;
    float _1110 = _934 * _831;
    float _1111 = _935 * _831;
    float _1112 = _936 * _831;
    float _1130;
    float _1132;
    float _1134;
    float _1136;
    float _1138;
    float _1140;
    float _1142;
    float _1144;
    float _1146;
    float _1148;
    float _1150;
    float _1152;
    float _1154;
    float _1156;
    float _1158;
    float _1160;
    if (_49_m0[145u].x != 0.0f)
    {
        float frontier_phi_15_14_ladder;
        float frontier_phi_15_14_ladder_1;
        float frontier_phi_15_14_ladder_2;
        float frontier_phi_15_14_ladder_3;
        float frontier_phi_15_14_ladder_4;
        float frontier_phi_15_14_ladder_5;
        float frontier_phi_15_14_ladder_6;
        float frontier_phi_15_14_ladder_7;
        float frontier_phi_15_14_ladder_8;
        float frontier_phi_15_14_ladder_9;
        float frontier_phi_15_14_ladder_10;
        float frontier_phi_15_14_ladder_11;
        float frontier_phi_15_14_ladder_12;
        float frontier_phi_15_14_ladder_13;
        float frontier_phi_15_14_ladder_14;
        float frontier_phi_15_14_ladder_15;
        if (asuint(_30[_317]._m0[_669 + 46u].y) == 0u)
        {
            frontier_phi_15_14_ladder = _1109;
            frontier_phi_15_14_ladder_1 = 0.0f;
            frontier_phi_15_14_ladder_2 = 1.0f;
            frontier_phi_15_14_ladder_3 = _614;
            frontier_phi_15_14_ladder_4 = _613;
            frontier_phi_15_14_ladder_5 = _612;
            frontier_phi_15_14_ladder_6 = _1100;
            frontier_phi_15_14_ladder_7 = _1108;
            frontier_phi_15_14_ladder_8 = _934;
            frontier_phi_15_14_ladder_9 = _935;
            frontier_phi_15_14_ladder_10 = _936;
            frontier_phi_15_14_ladder_11 = _661;
            frontier_phi_15_14_ladder_12 = _1110;
            frontier_phi_15_14_ladder_13 = _1111;
            frontier_phi_15_14_ladder_14 = _1112;
            frontier_phi_15_14_ladder_15 = _1107;
        }
        else
        {
            uint _1227 = uint(gl_FragCoord.x);
            uint _1228 = uint(gl_FragCoord.y);
            float4 _1230 = _16.Load(int3(uint2(_1227, _1228), 0u));
            float _1233 = _1230.x;
            float _1234 = _1230.y;
            float _1235 = _1230.z;
            float _1236 = _1230.w;
            float4 _1238 = _17.Load(int3(uint2(_1227, _1228), 0u));
            float _1240 = _1238.w;
            float4 _1242 = _18.Load(int3(uint2(_1227, _1228), 0u));
            float _1269;
            float _1270;
            float _1271;
            if (_1240 > 9.9999997473787516355514526367188e-05f)
            {
                float _1259 = _1238.x / _1240;
                float _1260 = _1238.y / _1240;
                float _1261 = _1238.z / _1240;
                float _1265 = rsqrt(dot(float3(_1259, _1260, _1261), float3(_1259, _1260, _1261)));
                _1269 = _1265 * _1259;
                _1270 = _1265 * _1260;
                _1271 = _1265 * _1261;
            }
            else
            {
                _1269 = 0.0f;
                _1270 = 0.0f;
                _1271 = 0.0f;
            }
            float _1272 = 1.0f - _787;
            float _1161 = _1240 * _1272;
            float _1274 = (1.0f - _1236) * _1272;
            float _1297 = ((_1269 - _612) * _1161) + _612;
            float _1298 = ((_1270 - _613) * _1161) + _613;
            float _1299 = ((_1271 - _614) * _1161) + _614;
            float _1303 = rsqrt(dot(float3(_1297, _1298, _1299), float3(_1297, _1298, _1299)));
            frontier_phi_15_14_ladder = (_1274 * (0.039999999105930328369140625f - _1109)) + _1109;
            frontier_phi_15_14_ladder_1 = _1161;
            frontier_phi_15_14_ladder_2 = max(_49_m0[43u].x, 1.0f - _1242.y);
            frontier_phi_15_14_ladder_3 = _1299 * _1303;
            frontier_phi_15_14_ladder_4 = _1298 * _1303;
            frontier_phi_15_14_ladder_5 = _1303 * _1297;
            frontier_phi_15_14_ladder_6 = (_1242.x * _1272) + ((1.0f - _1274) * _1100);
            frontier_phi_15_14_ladder_7 = (_1274 * (0.039999999105930328369140625f - _1108)) + _1108;
            frontier_phi_15_14_ladder_8 = (_1236 * _934) + _1233;
            frontier_phi_15_14_ladder_9 = (_1236 * _935) + _1234;
            frontier_phi_15_14_ladder_10 = (_1236 * _936) + _1235;
            frontier_phi_15_14_ladder_11 = _661 - (_1274 * _661);
            frontier_phi_15_14_ladder_12 = (_1236 * _1110) + _1233;
            frontier_phi_15_14_ladder_13 = (_1236 * _1111) + _1234;
            frontier_phi_15_14_ladder_14 = (_1236 * _1112) + _1235;
            frontier_phi_15_14_ladder_15 = (_1274 * (0.039999999105930328369140625f - _1107)) + _1107;
        }
        _1130 = frontier_phi_15_14_ladder_8;
        _1132 = frontier_phi_15_14_ladder_9;
        _1134 = frontier_phi_15_14_ladder_10;
        _1136 = frontier_phi_15_14_ladder_11;
        _1138 = frontier_phi_15_14_ladder_12;
        _1140 = frontier_phi_15_14_ladder_13;
        _1142 = frontier_phi_15_14_ladder_14;
        _1144 = frontier_phi_15_14_ladder_15;
        _1146 = frontier_phi_15_14_ladder_7;
        _1148 = frontier_phi_15_14_ladder;
        _1150 = frontier_phi_15_14_ladder_6;
        _1152 = frontier_phi_15_14_ladder_5;
        _1154 = frontier_phi_15_14_ladder_4;
        _1156 = frontier_phi_15_14_ladder_3;
        _1158 = frontier_phi_15_14_ladder_2;
        _1160 = frontier_phi_15_14_ladder_1;
    }
    else
    {
        _1130 = _934;
        _1132 = _935;
        _1134 = _936;
        _1136 = _661;
        _1138 = _1110;
        _1140 = _1111;
        _1142 = _1112;
        _1144 = _1107;
        _1146 = _1108;
        _1148 = _1109;
        _1150 = _1100;
        _1152 = _612;
        _1154 = _613;
        _1156 = _614;
        _1158 = 1.0f;
        _1160 = 0.0f;
    }
    float _1166 = max(_49_m0[43u].x, _704);
    float _1169 = ((_1158 - _1166) * _1160) + _1166;
    float _1179 = max(_49_m0[44u].x, _30[_317]._m0[_1072 + 37u].y * (((exp2(abs(_49_m0[93u].w) * _685) + (-1.0f)) * _49_m0[93u].z) + 1.0f));
    float _1183 = _1179 * _1179;
    float _1206 = clamp((_49_m0[22u].x - _30[_317]._m0[_1073].y) / (_30[_317]._m0[_1073].z - _30[_317]._m0[_1073].y), 0.0f, 1.0f);
    uint _1252 = _1072 + 49u;
    uint _1318;
    float _1319;
    if (abs(DITHERED_ALPHA) > 0.984615385532379150390625f)
    {
        _1318 = uint(asuint(_30[_317]._m0[_1072 + 46u].x) == 0u);
        _1319 = _30[_317]._m0[_1252].x;
    }
    else
    {
        _1318 = (asuint(_49_m0[101u]).x != 0u) ? 1u : uint(asuint(_30[_317]._m0[_1072 + 45u].w) == 0u);
        _1319 = _30[_317]._m0[_1252].y;
    }
    uint _1337;
    uint _1320;
    bool _1326;
    for (;;)
    {
        _1320 = asuint(_1319);
        _1326 = asuint(_59_m0[156u]).z == 0u;
        if (_1326)
        {
            if (asuint(_59_m0[157u]).y == 0u)
            {
                _1337 = 0u;
                break;
            }
        }
        _1337 = asuint(_44_m0[17u]).x;
        break;
    }
    float _1338 = max(_1150, _1150);
    float _1343 = ((((_1206 * _1206) * ((_49_m0[98u].z * (_1169 + (-1.0f))) + 1.0f)) * (3.0f - (_1206 * 2.0f))) * _49_m0[67u].x) * (((clamp((INSTANCE_VARIATION.y - _30[_317]._m0[_1036].z) / (_30[_317]._m0[_1036].w - _30[_317]._m0[_1036].z), 0.0f, 1.0f) + (-1.0f)) * float(asuint(_30[_317]._m0[_945].y) != 0u)) + 1.0f);
    float _1356 = ddx_fine(_334);
    float _1357 = ddx_fine(_335);
    float _1358 = ddx_fine(_336);
    float _1362 = ddy_fine(_334);
    float _1363 = ddy_fine(_335);
    float _1364 = ddy_fine(_336);
    float _1388 = clamp((exp2(log2(clamp(max(max(max(abs(_1356), abs(_1357)), abs(_1358)), max(max(abs(_1362), abs(_1363)), abs(_1364))), 0.0f, 1.0f)) * _49_m0[115u].w) - _49_m0[115u].y) / (_49_m0[115u].z - _49_m0[115u].y), 0.0f, 1.0f) * _49_m0[115u].x;
    float _1395 = (_1388 * ((_178 * _170) - _1152)) + _1152;
    float _1396 = (_1388 * ((_178 * _172) - _1154)) + _1154;
    float _1397 = (_1388 * ((_178 * _173) - _1156)) + _1156;
    float _1401 = rsqrt(dot(float3(_1395, _1396, _1397), float3(_1395, _1396, _1397)));
    float _1402 = _1395 * _1401;
    float _1403 = _1396 * _1401;
    float _1404 = _1397 * _1401;
    bool _1408 = asuint(_49_m0[176u]).z != 0u;
    uint _1415 = 102u - ((_1337 >> 5u) | 4294967291u);
    float _1424 = clamp(((_1338 * _724) - _49_m0[_1415].x) / (_49_m0[_1415].y - _49_m0[_1415].x), 0.0f, 1.0f);
    bool _1433 = ((_1424 * _1424) * (3.0f - (_1424 * 2.0f))) > 0.0f;
    uint _1434 = uint(_1433);
    uint _1452;
    if ((asuint(_49_m0[_1415]).w != 0u) && (_1433 && (asuint(_49_m0[114u]).z != 0u)))
    {
        uint frontier_phi_27_26_ladder;
        if (abs((_49_m0[8u].y + WORLDPOS.y) - _49_m0[116u].w) < 0.0500000007450580596923828125f)
        {
            uint frontier_phi_27_26_ladder_28_ladder;
            if (dot(float3(_49_m0[116u].xyz), float3(_1408 ? _334 : 0.0f, _1408 ? _335 : 0.0f, _1408 ? _336 : 0.0f)) > 0.999000012874603271484375f)
            {
                frontier_phi_27_26_ladder_28_ladder = _1434;
            }
            else
            {
                frontier_phi_27_26_ladder_28_ladder = 0u;
            }
            frontier_phi_27_26_ladder = frontier_phi_27_26_ladder_28_ladder;
        }
        else
        {
            frontier_phi_27_26_ladder = 0u;
        }
        _1452 = frontier_phi_27_26_ladder;
    }
    else
    {
        _1452 = _1434;
    }
    float _1453 = (((MOTIONBLUR_POS.x / MOTIONBLUR_POS.w) - (MOTIONBLUR_POS_1.x / MOTIONBLUR_POS_1.w)) + _49_m0[192u].w) * 1280.0f;
    float _1455 = (((MOTIONBLUR_POS.y / MOTIONBLUR_POS.w) - (MOTIONBLUR_POS_1.y / MOTIONBLUR_POS_1.w)) + _49_m0[193u].w) * (-720.0f);
    float _1469 = abs(_1453);
    float _1470 = abs(_1455);
    float _1471 = max(_1469, _1470);
    float _1485;
    float _1486;
    if (_1471 > 256.0f)
    {
        float _1482 = 256.0f / _1471;
        _1485 = _1482 * _1469;
        _1486 = _1482 * _1470;
    }
    else
    {
        _1485 = _1469;
        _1486 = _1470;
    }
    float _1507 = (abs(_1403) + abs(_1402)) + abs(_1404);
    float _1508 = _1402 / _1507;
    float _1509 = _1403 / _1507;
    float _1522;
    float _1523;
    if ((_1404 / _1507) < 0.0f)
    {
        _1522 = (1.0f - abs(_1509)) * ((_1508 >= 0.0f) ? 1.0f : (-1.0f));
        _1523 = (1.0f - abs(_1508)) * ((_1509 >= 0.0f) ? 1.0f : (-1.0f));
    }
    else
    {
        _1522 = _1508;
        _1523 = _1509;
    }
    uint _1553 = uint(clamp((clamp(((_1183 * 2.0f) + (-1.0f)) + abs(dot(float3(_59_m0[0u].xyz), float3(_1152, _1154, _1156))), 0.0f, 1.0f) * (1.0f - _1183)) + _1183, 0.0f, 1.0f) * 127.0f);
    uint _1559 = uint(clamp(_1169, 0.0f, 1.0f) * 127.0f);
    SV_Target.x = (((_1343 * (_952 ? (_1018 * _1028) : 1.0f)) * _1063.x) * (((1.0f - _1144) * _1138) + _30[_317]._m0[_1073].x)) * _49_m0[59u].x;
    SV_Target.y = (((_1343 * (_952 ? (_1020 * _1028) : 1.0f)) * _1063.y) * (((1.0f - _1146) * _1140) + _30[_317]._m0[_1073].x)) * _49_m0[59u].x;
    SV_Target.z = (((_1343 * (_952 ? (_1021 * _1028) : 1.0f)) * _1063.z) * (((1.0f - _1148) * _1142) + _30[_317]._m0[_1073].x)) * _49_m0[59u].x;
    SV_Target.w = _633.w;
    SV_Target_1.x = (((float(int(uint(_1453 > 0.0f) - uint(_1453 < 0.0f))) * 32.0f) * sqrt(_1485)) + 512.0f) * 0.000977517105638980865478515625f;
    SV_Target_1.y = (((float(int(uint(_1455 > 0.0f) - uint(_1455 < 0.0f))) * 32.0f) * sqrt(_1486)) + 512.0f) * 0.000977517105638980865478515625f;
    SV_Target_1.z = (((MOTIONBLUR_POS.z / MOTIONBLUR_POS.w) - (MOTIONBLUR_POS_1.z / MOTIONBLUR_POS_1.w)) * 32.0f) + 0.500488758087158203125f;
    SV_Target_1.w = 0.0f;
    SV_Target_2.x = _1320 >> 7u;
    SV_Target_2.y = (_1318 != 0u) ? (_1553 | 128u) : _1553;
    SV_Target_2.z = (_1452 != 0u) ? (_1559 | 128u) : _1559;
    SV_Target_2.w = _1320 & 127u;
    SV_Target_3 = ((uint(min(max(round((_1523 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 20u) | (uint(min(max(round((_1522 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 8u)) | (uint(clamp(_1338, 0.0f, 1.0f) * 255.0f) & 255u);
    SV_Target_4.x = _1130;
    SV_Target_4.y = _1132;
    SV_Target_4.z = _1134;
    SV_Target_4.w = _1136;
    SV_Target_5.x = 0.0f;
    SV_Target_5.y = ((_1337 & 64u) == 0u) ? _724 : 0.0f;
    SV_Target_5.z = 0.0f;
    SV_Target_5.w = _1166;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    gl_FragCoord = stage_input.gl_FragCoord;
    gl_FragCoord.w = 1.0 / gl_FragCoord.w;
    TEXCOORD = stage_input.TEXCOORD;
    TEXCOORD_1 = stage_input.TEXCOORD_1;
    WORLDPOS = stage_input.WORLDPOS;
    WORLDNORMAL = stage_input.WORLDNORMAL;
    TEXCOORD_4 = stage_input.TEXCOORD_4;
    TEXCOORD4y = stage_input.TEXCOORD4y;
    WORLDTANGENT = stage_input.WORLDTANGENT;
    INSTANCE_VARIATION = stage_input.INSTANCE_VARIATION;
    MOTIONBLUR_POS = stage_input.MOTIONBLUR_POS;
    MOTIONBLUR_POS_1 = stage_input.MOTIONBLUR_POS_1;
    DITHERED_ALPHA = stage_input.DITHERED_ALPHA;
    INSTANCE_INDEX = stage_input.INSTANCE_INDEX;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.SV_Target = SV_Target;
    stage_output.SV_Target_1 = SV_Target_1;
    stage_output.SV_Target_2 = SV_Target_2;
    stage_output.SV_Target_3 = SV_Target_3;
    stage_output.SV_Target_4 = SV_Target_4;
    stage_output.SV_Target_5 = SV_Target_5;
    return stage_output;
}
