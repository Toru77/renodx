cbuffer _28_30 : register(b0, space0)
{
    float4 _30_m0[24] : packoffset(c0);
};

cbuffer _33_35 : register(b9, space0)
{
    float4 _35_m0[25] : packoffset(c0);
};

cbuffer _38_40 : register(b2, space0)
{
    float4 _40_m0[700] : packoffset(c0);
};

cbuffer _43_45 : register(b4, space0)
{
    float4 _45_m0[69] : packoffset(c0);
};

cbuffer _48_50 : register(b11, space0)
{
    float4 _50_m0[5] : packoffset(c0);
};

cbuffer _53_55 : register(b3, space0)
{
    float4 _55_m0[1149] : packoffset(c0);
};

Texture2D<float4> _8 : register(t16, space0);
Texture2D<float4> _9 : register(t17, space0);
Texture2D<float4> _10 : register(t18, space0);
Texture2D<float4> _11 : register(t19, space0);
Texture2D<float4> _12 : register(t20, space0);
Texture2D<float4> _13 : register(t21, space0);
Texture2D<float4> _14 : register(t22, space0);
Texture2D<float4> _15 : register(t23, space0);
Texture2D<float4> _16 : register(t25, space0);
Texture2D<float4> _19[] : register(t0, space48);
TextureCube<float4> _22 : register(t1, space0);
Texture2D<float4> _23 : register(t7, space0);
SamplerState _58 : register(s12, space0);
SamplerState _59 : register(s14, space0);
SamplerState _60 : register(s0, space0);
SamplerState _61 : register(s5, space0);
SamplerState _62 : register(s1, space0);
SamplerState _63 : register(s3, space0);
SamplerState _64 : register(s11, space0);

static float2 TEXCOORD;
static float2 TEXCOORD_1;
static float2 TEXCOORD_2;
static float2 TEXCOORD_3;
static float3 WORLDPOS;
static float3 WORLDNORMAL;
static float TEXCOORD_4;
static float TEXCOORD4y;
static float4 WORLDTANGENT;
static float4 WORLDTANGENT_1;
static float4 WORLDTANGENT_2;
static float4 COLOR;
static float DAMAGEIMPACT;
static float DAMAGESCRAPE;
static float4 MOTIONBLUR_POS;
static float4 MOTIONBLUR_POS_1;
static float4 SV_Target;
static float4 SV_Target_1;
static uint4 SV_Target_2;
static uint SV_Target_3;
static float4 SV_Target_4;
static float4 SV_Target_5;
static uint4 SV_Target_6;
static uint4 SV_Target_7;

struct SPIRV_Cross_Input
{
    float2 TEXCOORD : TEXCOORD0;
    float2 TEXCOORD_1 : TEXCOORD0;
    float2 TEXCOORD_2 : TEXCOORD1;
    float2 TEXCOORD_3 : TEXCOORD1;
    float3 WORLDPOS : TEXCOORD2;
    float TEXCOORD_4 : TEXCOORD2;
    centroid float3 WORLDNORMAL : TEXCOORD3;
    float TEXCOORD4y : TEXCOORD4;
    float DAMAGEIMPACT : TEXCOORD4;
    float DAMAGESCRAPE : TEXCOORD4;
    float4 WORLDTANGENT : TEXCOORD5;
    float4 WORLDTANGENT_1 : TEXCOORD6;
    float4 WORLDTANGENT_2 : TEXCOORD7;
    float4 COLOR : TEXCOORD8;
    float4 MOTIONBLUR_POS : TEXCOORD10;
    float4 MOTIONBLUR_POS_1 : TEXCOORD11;
};

struct SPIRV_Cross_Output
{
    float4 SV_Target : SV_Target0;
    float4 SV_Target_1 : SV_Target1;
    uint4 SV_Target_2 : SV_Target2;
    uint SV_Target_3 : SV_Target3;
    float4 SV_Target_4 : SV_Target4;
    float4 SV_Target_5 : SV_Target5;
    uint4 SV_Target_6 : SV_Target6;
    uint4 SV_Target_7 : SV_Target7;
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

void frag_main()
{
    float _155 = (-0.0f) - WORLDPOS.x;
    float _159 = (-0.0f) - WORLDPOS.y;
    float _162 = (-0.0f) - WORLDPOS.z;
    float _191 = rsqrt(dot(float3(_155, _159, _162), float3(_155, _159, _162)));
    float _192 = _191 * _155;
    float _193 = _191 * _159;
    float _194 = _191 * _162;
    float _198 = rsqrt(dot(float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z), float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z)));
    float _199 = _198 * WORLDNORMAL.x;
    float _200 = _198 * WORLDNORMAL.y;
    float _201 = _198 * WORLDNORMAL.z;
    float _205 = rsqrt(dot(float3(WORLDTANGENT.x, WORLDTANGENT.y, WORLDTANGENT.z), float3(WORLDTANGENT.x, WORLDTANGENT.y, WORLDTANGENT.z)));
    float _206 = _205 * WORLDTANGENT.x;
    float _207 = _205 * WORLDTANGENT.y;
    float _208 = _205 * WORLDTANGENT.z;
    float _212 = rsqrt(dot(float3(WORLDTANGENT_1.x, WORLDTANGENT_1.y, WORLDTANGENT_1.z), float3(WORLDTANGENT_1.x, WORLDTANGENT_1.y, WORLDTANGENT_1.z)));
    float _213 = _212 * WORLDTANGENT_1.x;
    float _214 = _212 * WORLDTANGENT_1.y;
    float _215 = _212 * WORLDTANGENT_1.z;
    float _219 = rsqrt(dot(float3(WORLDTANGENT_2.x, WORLDTANGENT_2.y, WORLDTANGENT_2.z), float3(WORLDTANGENT_2.x, WORLDTANGENT_2.y, WORLDTANGENT_2.z)));
    float _220 = _219 * WORLDTANGENT_2.x;
    float _221 = _219 * WORLDTANGENT_2.y;
    float _222 = _219 * WORLDTANGENT_2.z;
    float _256 = _40_m0[8u].x + WORLDPOS.x;
    float _257 = _40_m0[8u].y + WORLDPOS.y;
    float _258 = _40_m0[8u].z + WORLDPOS.z;
    float _274 = mad(_258, _40_m0[7u].z, mad(_257, _40_m0[7u].y, _256 * _40_m0[7u].x)) + _40_m0[7u].w;
    float _326;
    float _328;
    float _330;
    float _332;
    uint _333;
    if (asuint(_50_m0[3u]).x == 0u)
    {
        float4 _311 = _19[0u].Sample(_60, float2(TEXCOORD_2.x, TEXCOORD_2.y));
        float _313 = _311.x;
        _326 = 0.0f;
        _328 = 0.0f;
        _330 = _313;
        _332 = _313;
        _333 = 0u;
    }
    else
    {
        float4 _317 = _19[1u].Sample(_60, float2(TEXCOORD_2.x, TEXCOORD_2.y));
        float _319 = _317.w;
        uint frontier_phi_3_2_ladder;
        float frontier_phi_3_2_ladder_1;
        float frontier_phi_3_2_ladder_2;
        float frontier_phi_3_2_ladder_3;
        float frontier_phi_3_2_ladder_4;
        if (asuint(_55_m0[1147u]).w == 0u)
        {
            frontier_phi_3_2_ladder = 0u;
            frontier_phi_3_2_ladder_1 = _319;
            frontier_phi_3_2_ladder_2 = 1.0f;
            frontier_phi_3_2_ladder_3 = 0.0f;
            frontier_phi_3_2_ladder_4 = 0.0f;
        }
        else
        {
            precise float _424 = _317.x * 2.0f;
            precise float _426 = _317.y * 2.0f;
            precise float _327 = _424 + (-1.0f);
            precise float _329 = _426 + (-1.0f);
            frontier_phi_3_2_ladder = 1u;
            frontier_phi_3_2_ladder_1 = _319;
            frontier_phi_3_2_ladder_2 = _317.z;
            frontier_phi_3_2_ladder_3 = _329;
            frontier_phi_3_2_ladder_4 = _327;
        }
        _326 = frontier_phi_3_2_ladder_4;
        _328 = frontier_phi_3_2_ladder_3;
        _330 = frontier_phi_3_2_ladder_2;
        _332 = frontier_phi_3_2_ladder_1;
        _333 = frontier_phi_3_2_ladder;
    }
    float4 _337 = _15.Sample(_62, float2(TEXCOORD.x, TEXCOORD.y));
    float4 _353 = _16.Sample(_62, float2(TEXCOORD.x, TEXCOORD.y));
    float _355 = _353.x;
    float _356 = _353.y;
    float _357 = _353.z;
    float _358 = _353.w;
    float _362 = clamp(_30_m0[9u].w + _30_m0[9u].z, 0.0f, 1.0f);
    float4 _370 = _10.Sample(_62, float2(TEXCOORD.x, TEXCOORD.y));
    float _385 = (_357 * (1.0f - _337.w)) * _30_m0[10u].x;
    float _386 = _30_m0[10u].y * _362;
    float _408 = min(max((clamp(exp2(log2(_370.x) * 3.0f), 0.0f, 1.0f) * ((_30_m0[10u].z * _362) - _362)) + _362, 0.0f), 0.9900000095367431640625f) * 0.5f;
    float4 _414 = _14.SampleLevel(_64, float2((asuint(_30_m0[22u].x) != 0u) ? (_408 + 0.5f) : _408, _30_m0[10u].x), 0.0f);
    float _428;
    float _431;
    float _434;
    switch (uint(asuint(_30_m0[22u].y) != 0u))
    {
        case 0u:
        {
            _428 = _414.x;
            _431 = _414.y;
            _434 = _414.z;
            break;
        }
        case 1u:
        {
            _428 = _30_m0[1u].x;
            _431 = _30_m0[1u].y;
            _434 = _30_m0[1u].z;
            break;
        }
        default:
        {
            _428 = 0.0f;
            _431 = 0.0f;
            _434 = 0.0f;
            break;
        }
    }
    float _469 = exp2(log2((exp2(log2(_428) * 0.4545454680919647216796875f) * 2.0f) * exp2(log2(clamp(((_386 * _355) * _385) + _355, 0.0f, 1.0f)) * 0.4545454680919647216796875f)) * 2.2000000476837158203125f);
    float _470 = exp2(log2((exp2(log2(_431) * 0.4545454680919647216796875f) * 2.0f) * exp2(log2(clamp(((_386 * _356) * _385) + _356, 0.0f, 1.0f)) * 0.4545454680919647216796875f)) * 2.2000000476837158203125f);
    float _471 = exp2(log2((exp2(log2(_434) * 0.4545454680919647216796875f) * 2.0f) * exp2(log2(clamp(((_386 * _357) * _385) + _357, 0.0f, 1.0f)) * 0.4545454680919647216796875f)) * 2.2000000476837158203125f);
    float _479 = ((_470 * 0.589999973773956298828125f) + (_469 * 0.300000011920928955078125f)) + (_471 * 0.10999999940395355224609375f);
    float _497 = (_30_m0[10u].x * _362) * clamp((_30_m0[10u].x + _362) * 0.5f, 0.0f, 1.0f);
    float _504 = (_497 * (clamp(((_469 - _479) * _30_m0[10u].w) + _479, 0.0f, 1.0f) - _469)) + _469;
    float _505 = (_497 * (clamp(((_470 - _479) * _30_m0[10u].w) + _479, 0.0f, 1.0f) - _470)) + _470;
    float _506 = (_497 * (clamp(((_471 - _479) * _30_m0[10u].w) + _479, 0.0f, 1.0f) - _471)) + _471;
    float _535 = _30_m0[8u].x * TEXCOORD_1.x;
    float _536 = _30_m0[8u].y * TEXCOORD_1.y;
    float _541 = _30_m0[8u].z * _30_m0[8u].w;
    float _545 = floor(min(_337.z, 0.9900000095367431640625f) * _541) / _30_m0[8u].z;
    float _549 = frac(abs(_545));
    float _551 = (_545 >= ((-0.0f) - _545)) ? _549 : ((-0.0f) - _549);
    bool _569 = _541 == 1.0f;
    float4 _575 = _12.Sample(_62, float2(TEXCOORD.x, TEXCOORD.y));
    float _579 = _575.z;
    float _580 = _575.x * 2.0f;
    float _582 = _580 + (-1.0f);
    float _583 = (_575.y * 2.0f) + (-1.0f);
    float _589 = sqrt(clamp((1.0f - (_582 * _582)) - (_583 * _583), 0.0f, 1.0f));
    float4 _593 = _13.Sample(_62, float2(TEXCOORD.x, TEXCOORD.y));
    float _595 = _593.x;
    float _596 = _593.y;
    float _598 = _593.w;
    float _601 = (_595 * 2.0f) + (-1.0f);
    float _602 = (_596 * 2.0f) + (-1.0f);
    float _615 = (_580 + (-1.5f)) + _595;
    float _616 = (0.5f - _596) - _583;
    float _617 = (((sqrt(clamp((1.0f - (_601 * _601)) - (_602 * _602), 0.0f, 1.0f)) + (-1.0f)) * 0.5f) + 1.0f) * _589;
    float _621 = rsqrt(dot(float3(_615, _616, _617), float3(_615, _616, _617)));
    float _622 = _621 * _615;
    float _623 = _621 * _616;
    float _624 = _617 * _621;
    float4 _643 = _11.Sample(_63, float2(_569 ? _535 : (((1.0f / _30_m0[8u].z) * (((frac(_535) + (-0.5f)) * _30_m0[11u].z) + 0.5f)) + _551), _569 ? _536 : (((1.0f / _30_m0[8u].w) * (((frac(_536) + (-0.5f)) * _30_m0[11u].z) + 0.5f)) + (floor(_545) / _30_m0[8u].w))));
    float _650 = (_643.x * 2.0f) + (-1.0f);
    float _651 = (_643.y * 2.0f) + (-1.0f);
    float _657 = sqrt(clamp(1.0f - dot(float2(_650, _651), float2(_650, _651)), 0.0f, 1.0f));
    float _658 = (-0.0f) - _651;
    float _668 = ((_215 * _200) - (_214 * _201)) * WORLDTANGENT_1.w;
    float _669 = ((_213 * _201) - (_215 * _199)) * WORLDTANGENT_1.w;
    float _670 = ((_214 * _199) - (_213 * _200)) * WORLDTANGENT_1.w;
    float _674 = rsqrt(dot(float3(_668, _669, _670), float3(_668, _669, _670)));
    float _680 = mad(_657, _199, mad(_658, _674 * _668, _650 * _213));
    float _683 = mad(_657, _200, mad(_658, _674 * _669, _650 * _214));
    float _686 = mad(_657, _201, mad(_658, _674 * _670, _650 * _215));
    float _690 = rsqrt(dot(float3(_680, _683, _686), float3(_680, _683, _686)));
    float _691 = _690 * _680;
    float _692 = _690 * _683;
    float _693 = _690 * _686;
    float _703 = ((_208 * _200) - (_207 * _201)) * WORLDTANGENT.w;
    float _704 = ((_206 * _201) - (_208 * _199)) * WORLDTANGENT.w;
    float _705 = ((_207 * _199) - (_206 * _200)) * WORLDTANGENT.w;
    float _709 = rsqrt(dot(float3(_703, _704, _705), float3(_703, _704, _705)));
    float _710 = _709 * _704;
    float _711 = _709 * _705;
    float _714 = mad(_693, _208, mad(_692, _207, _691 * _206));
    float _715 = _709 * _703;
    float _718 = mad(_693, _711, mad(_692, _710, _715 * _691));
    float _721 = mad(_693, _201, mad(_692, _200, _691 * _199));
    float _725 = rsqrt(dot(float3(_714, _718, _721), float3(_714, _718, _721)));
    float _731 = _30_m0[12u].w * _725;
    float _732 = _731 * _714;
    float _733 = _731 * _718;
    float _735 = (_30_m0[12u].w * ((_725 * _721) + (-1.0f))) + 1.0f;
    float _745 = ((_735 + _733) * _622) + (_732 * _624);
    float _748 = ((_735 + _732) * _623) + (_733 * _624);
    float _749 = (((-0.0f) - (_623 * _733)) - (_732 * _622)) + (_735 * _624);
    float _753 = rsqrt(dot(float3(_745, _748, _749), float3(_745, _748, _749)));
    float _754 = _745 * _753;
    float _755 = _748 * _753;
    float _756 = _749 * _753;
    float _757 = (-0.0f) - WORLDPOS.x;
    float _758 = (-0.0f) - WORLDPOS.y;
    float _759 = (-0.0f) - WORLDPOS.z;
    float _763 = rsqrt(dot(float3(_757, _758, _759), float3(_757, _758, _759)));
    float _773 = clamp(exp2(log2(abs(dot(float4(_199, _200, _201, 0.0f), float4(_763 * _757, _763 * _758, _763 * _759, 0.0f))))), 0.0f, 1.0f);
    float _780 = (_773 * (_754 - _622)) + _622;
    float _781 = (_773 * (_755 - _623)) + _623;
    float _782 = (_773 * (_756 - _624)) + _624;
    float _785 = mad(_782, _199, mad(_781, _715, _780 * _206));
    float _788 = mad(_782, _200, mad(_781, _710, _780 * _207));
    float _791 = mad(_782, _201, mad(_781, _711, _780 * _208));
    float _795 = rsqrt(dot(float3(_785, _788, _791), float3(_785, _788, _791)));
    float _796 = _795 * _785;
    float _797 = _795 * _788;
    float _798 = _795 * _791;
    float _799 = _643.z * _593.z;
    float _800 = 1.0f - _799;
    float _803 = ((_800 * _30_m0[11u].y) * _30_m0[12u].w) * _773;
    float _813 = ((_803 * (_30_m0[3u].x + (-1.0f))) + 1.0f) * (((_30_m0[2u].x - _504) * _30_m0[11u].x) + _504);
    float _814 = ((_803 * (_30_m0[3u].y + (-1.0f))) + 1.0f) * (((_30_m0[2u].y - _505) * _30_m0[11u].x) + _505);
    float _815 = ((_803 * (_30_m0[3u].z + (-1.0f))) + 1.0f) * (((_30_m0[2u].z - _506) * _30_m0[11u].x) + _506);
    float _821 = _30_m0[13u].y * _800;
    float _822 = (((_30_m0[12u].x - _30_m0[11u].w) * _598) + _30_m0[11u].w) + _821;
    float _823 = (((_30_m0[12u].z - _30_m0[12u].y) * _598) + _30_m0[12u].y) + _821;
    float _824 = _358 * 2.0f;
    float _830 = (1.0f - _358) * 2.0f;
    bool _835 = _358 > 0.5f;
    float _840 = (1.0f - (_835 ? (1.0f - ((1.0f - _822) * _830)) : (_822 * _824))) * _30_m0[13u].x;
    float _841 = (1.0f - (_835 ? (1.0f - ((1.0f - _823) * _830)) : (_823 * _824))) * _30_m0[13u].x;
    float _849 = min(max(TEXCOORD.x, 0.0f), 1.0f);
    float _850 = min(max(TEXCOORD.y, 0.0f), 1.0f);
    float4 _872 = _8.Sample(_58, float2(_849, _850));
    float _886 = (_30_m0[15u].y * _8.Sample(_58, float2(_849, _850)).x) * (1.0f - (_30_m0[14u].y * _800));
    float _893 = (_886 * (_30_m0[6u].x - _813)) + _813;
    float _894 = (_886 * (_30_m0[6u].y - _814)) + _814;
    float _895 = (_886 * (_30_m0[6u].z - _815)) + _815;
    float _906 = (_30_m0[15u].x * _8.Sample(_58, float2(_849, _850)).y) * (1.0f - (_30_m0[14u].x * _800));
    float _913 = ((_30_m0[5u].x - _893) * _906) + _893;
    float _914 = ((_30_m0[5u].y - _894) * _906) + _894;
    float _915 = ((_30_m0[5u].z - _895) * _906) + _895;
    float _928 = (_30_m0[14u].w * _8.Sample(_58, float2(_849, _850)).z) * (1.0f - (_30_m0[13u].w * _800));
    float _935 = ((_30_m0[7u].x - _913) * _928) + _913;
    float _936 = ((_30_m0[7u].y - _914) * _928) + _914;
    float _937 = ((_30_m0[7u].z - _915) * _928) + _915;
    float _948 = (_30_m0[14u].z * _872.w) * (1.0f - (_30_m0[13u].z * _800));
    float _960 = float(asuint(_30_m0[22u].z) != 0u);
    float _970 = (((_935 - _813) + ((_30_m0[4u].x - _935) * _948)) * _960) + _813;
    float _971 = (((_936 - _814) + ((_30_m0[4u].y - _936) * _948)) * _960) + _814;
    float _972 = (((_937 - _815) + ((_30_m0[4u].z - _937) * _948)) * _960) + _815;
    float _978 = (_886 * (_30_m0[15u].z - _840)) + _840;
    float _979 = (_886 * (_30_m0[15u].z - _841)) + _841;
    float _985 = ((_30_m0[15u].w - _978) * _906) + _978;
    float _986 = ((_30_m0[15u].w - _979) * _906) + _979;
    float _995 = ((_30_m0[16u].x - _985) * _928) + _985;
    float _996 = ((_30_m0[16u].x - _986) * _928) + _986;
    float _1008 = (((_995 - _840) + ((_30_m0[16u].y - _995) * _948)) * _960) + _840;
    float _1009 = (((_996 - _841) + ((_30_m0[16u].y - _996) * _948)) * _960) + _841;
    float4 _1013 = _9.Sample(_59, float2(TEXCOORD.x, TEXCOORD.y));
    float _1015 = _1013.x;
    float _1036 = ((_30_m0[16u].z * _1013.w) * (1.0f - (_30_m0[16u].w * _800))) * float(asuint(_30_m0[22u].w) != 0u);
    float _1056 = (1.0f - dot(float4(_1015, _1013.yzw), float4(0.2125999927520751953125f, 0.715200006961822509765625f, 0.072200000286102294921875f, 0.0f))) * (_30_m0[17u].y - _30_m0[17u].x);
    float _1063 = (_1036 * ((_30_m0[17u].x - _1008) + _1056)) + _1008;
    float _1064 = (_1036 * ((_30_m0[17u].x - _1009) + _1056)) + _1009;
    float _1072 = (_30_m0[17u].z * (_754 - _582)) + _582;
    float _1073 = (_30_m0[17u].z * (_755 + _583)) - _583;
    float _1074 = (_30_m0[17u].z * (_756 - _589)) + _589;
    float _1077 = mad(_1074, _199, mad(_1073, _715, _1072 * _206));
    float _1080 = mad(_1074, _200, mad(_1073, _710, _1072 * _207));
    float _1083 = mad(_1074, _201, mad(_1073, _711, _1072 * _208));
    float _1087 = rsqrt(dot(float3(_1077, _1080, _1083), float3(_1077, _1080, _1083)));
    float _1111 = (-0.0f) - _192;
    float _1112 = (-0.0f) - _193;
    float _1113 = (-0.0f) - _194;
    float _1117 = dot(float3(_1111, _1112, _1113), float3(_796, _797, _798)) * 2.0f;
    float _1121 = _1111 - (_1117 * _796);
    float _1122 = _1112 - (_1117 * _797);
    float _1123 = _1113 - (_1117 * _798);
    uint _1176;
    float _1177;
    float _1178;
    float _1179;
    float _1180;
    float _1181;
    float _1182;
    if (_333 == 0u)
    {
        _1176 = 0u;
        _1177 = _55_m0[1147u].x;
        _1178 = _55_m0[1146u].w;
        _1179 = _796;
        _1180 = _797;
        _1181 = _798;
        _1182 = _332;
    }
    else
    {
        float _1132 = (-0.0f) - _328;
        float _1138 = sqrt(clamp((1.0f - (_326 * _326)) - (_328 * _328), 0.0f, 1.0f));
        float _1148 = ((_222 * _200) - (_221 * _201)) * WORLDTANGENT_2.w;
        float _1149 = ((_220 * _201) - (_222 * _199)) * WORLDTANGENT_2.w;
        float _1150 = ((_221 * _199) - (_220 * _200)) * WORLDTANGENT_2.w;
        float _1154 = rsqrt(dot(float3(_1148, _1149, _1150), float3(_1148, _1149, _1150)));
        float _1160 = mad(_1138, _199, mad(_1132, _1154 * _1148, _326 * _220));
        float _1163 = mad(_1138, _200, mad(_1132, _1154 * _1149, _326 * _221));
        float _1166 = mad(_1138, _201, mad(_1132, _1154 * _1150, _326 * _222));
        float _1170 = rsqrt(dot(float3(_1160, _1163, _1166), float3(_1160, _1163, _1166)));
        _1176 = 1u;
        _1177 = _55_m0[1147u].z;
        _1178 = _55_m0[1147u].y;
        _1179 = _1170 * _1160;
        _1180 = _1170 * _1163;
        _1181 = _1170 * _1166;
        _1182 = _330;
    }
    float _1184;
    if (_1178 == 0.0f)
    {
        _1184 = 1.0f;
    }
    else
    {
        precise float _1283 = _579 * _1182;
        float _1286 = exp2(log2(_1283) * _1177);
        float _1291 = _1286 * _1286;
        float _1296 = clamp(((_1291 * 2.0f) + (-1.0f)) + abs(dot(float3(_1121, _1122, _1123), float3(_1179, _1180, _1181))), 0.0f, 1.0f);
        float _1450;
        if (_1176 == 0u)
        {
            _1450 = (_1296 * (1.0f - _1291)) + _1291;
        }
        else
        {
            _1450 = _1296;
        }
        _1184 = ((_1450 + (-1.0f)) * _1178) + 1.0f;
    }
    float _1195 = clamp((((-0.0f) - _1122) - _40_m0[75u].x) / (_40_m0[75u].y - _40_m0[75u].x), 0.0f, 1.0f);
    float _1232 = 1.0f - clamp((max(max(max(abs(ddx_fine(_796)), abs(ddx_fine(_797))), abs(ddx_fine(_798))), max(max(abs(ddy_fine(_796)), abs(ddy_fine(_797))), abs(ddy_fine(_798)))) - _55_m0[1144u].w) * _55_m0[1142u].w, 0.0f, 1.0f);
    float _1259 = ((asuint(((_40_m0[691u].y * (1.0f - clamp((((mad(_258, _40_m0[5u].z, mad(_257, _40_m0[5u].y, _256 * _40_m0[5u].x)) + _40_m0[5u].w) / _274) * 0.5f) + 0.5f, 0.0f, 1.0f))) + clamp((mad(_258, _40_m0[6u].z, mad(_257, _40_m0[6u].y, _256 * _40_m0[6u].x)) + _40_m0[6u].w) / _274, 0.0f, 1.0f)) + (_40_m0[691u].x * clamp((((mad(_258, _40_m0[4u].z, mad(_257, _40_m0[4u].y, _256 * _40_m0[4u].x)) + _40_m0[4u].w) / _274) * 0.5f) + 0.5f, 0.0f, 1.0f))) & 2139095040u) == 2139095040u) ? _332 : (_579 * _332);
    float _1468;
    float _1470;
    float _1472;
    float _1474;
    if (asuint(_55_m0[1148u]).z == 0u)
    {
        uint4 _1299 = asuint(_50_m0[0u]);
        float frontier_phi_18_13_ladder;
        float frontier_phi_18_13_ladder_1;
        float frontier_phi_18_13_ladder_2;
        float frontier_phi_18_13_ladder_3;
        if ((int(_1299.x) > int(4294967295u)) && (_1299.w == 0u))
        {
            float _1465 = clamp(dot(float3(_1121, _1122, _1123), float3(_35_m0[8u].x, _35_m0[9u].x, _35_m0[10u].x)), 0.0f, 1.0f);
            frontier_phi_18_13_ladder = 1.0f - (_1465 * _1465);
            frontier_phi_18_13_ladder_1 = 0.0f;
            frontier_phi_18_13_ladder_2 = 0.0f;
            frontier_phi_18_13_ladder_3 = 0.0f;
        }
        else
        {
            frontier_phi_18_13_ladder = 1.0f;
            frontier_phi_18_13_ladder_1 = 0.0f;
            frontier_phi_18_13_ladder_2 = 0.0f;
            frontier_phi_18_13_ladder_3 = 0.0f;
        }
        _1468 = frontier_phi_18_13_ladder_3;
        _1470 = frontier_phi_18_13_ladder_2;
        _1472 = frontier_phi_18_13_ladder_1;
        _1474 = frontier_phi_18_13_ladder;
    }
    else
    {
        float _1326 = ((_40_m0[8u].x + WORLDPOS.x) - _40_m0[24u].x) - _40_m0[23u].x;
        float _1328 = ((_40_m0[8u].y + WORLDPOS.y) - _40_m0[24u].y) - _40_m0[23u].y;
        float _1330 = ((_40_m0[8u].z + WORLDPOS.z) - _40_m0[24u].z) - _40_m0[23u].z;
        float _1348 = mad(_798, _40_m0[0u].z, mad(_797, _40_m0[0u].y, _40_m0[0u].x * _796));
        float _1351 = mad(_798, _40_m0[1u].z, mad(_797, _40_m0[1u].y, _40_m0[1u].x * _796));
        float _1354 = mad(_798, _40_m0[2u].z, mad(_797, _40_m0[2u].y, _40_m0[2u].x * _796));
        float _1357 = mad(_1330, _40_m0[0u].z, mad(_1328, _40_m0[0u].y, _40_m0[0u].x * _1326));
        float _1360 = mad(_1330, _40_m0[1u].z, mad(_1328, _40_m0[1u].y, _40_m0[1u].x * _1326));
        float _1363 = mad(_1330, _40_m0[2u].z, mad(_1328, _40_m0[2u].y, _40_m0[2u].x * _1326));
        float _1370 = min(max(dot(float3(_1357, _1360, _1363), float3(_1348, _1351, _1354)), -1000000000.0f), 0.0f) * 2.0f;
        float _1385 = (_40_m0[10u].y - _1363) / min(max(_1363 - (_1370 * _1354), 0.001000000047497451305389404296875f), 1000000000.0f);
        float _1386 = 1.0f / _40_m0[10u].y;
        float _1396 = clamp(((((_1385 * (_1360 - (_1370 * _1351))) + _1360) * _1386) + 1.0f) * 0.5f, 0.0f, 1.0f);
        float _1397 = (1.0f - (((_1385 * (_1357 - (_1370 * _1348))) + _1357) * _1386)) * 0.449999988079071044921875f;
        float _1422 = _55_m0[1146u].x * (0.949999988079071044921875f - _1397);
        float _1423 = _55_m0[1146u].y * (0.85000002384185791015625f - (_1396 * 0.85000002384185791015625f));
        float4 _1427 = _23.Sample(_60, float2(_1422, _1423));
        float _1432 = _1422 + (-0.5f);
        float _1433 = _1423 + (-0.5f);
        float _1441 = clamp((sqrt((_1432 * _1432) + (_1433 * _1433)) + (-0.3499999940395355224609375f)) * 9.090908050537109375f, 0.0f, 1.0f);
        float _1446 = 1.0f - _1063;
        float _1631;
        if (asuint(_40_m0[72u]).z == 0u)
        {
            float _1581_tmp = _22.CalculateLevelOfDetail(_61, float3(_1121, _1122, _1123));
            float2 _1581 = _1581_tmp.xx;
            _1631 = max((_40_m0[66u].y + (-9.0f)) + ((_55_m0[107u].x * 9.0f) * exp2(log2(clamp(_1446, 0.0f, 1.0f)) * _55_m0[107u].z)), _1581.x - _55_m0[1141u].y);
        }
        else
        {
            _1631 = _40_m0[66u].y * _1446;
        }
        float4 _1635 = _22.SampleLevel(_61, float3(_1121, _1122, _1123), _1631);
        float _1650 = ((1.0f - clamp((abs(_1397 + (-0.449999988079071044921875f)) + (-0.3499999940395355224609375f)) * 10.00000095367431640625f, 0.0f, 1.0f)) * clamp(8.49999904632568359375f - (_1396 * 8.49999904632568359375f), 0.0f, 1.0f)) * _40_m0[60u].x;
        float _1651 = _1650 * _1427.x;
        float _1652 = _1650 * _1427.y;
        float _1653 = _1650 * _1427.z;
        float _1660 = (((_40_m0[59u].w * _1635.x) - _1651) * _1441) + _1651;
        float _1661 = (((_40_m0[59u].w * _1635.y) - _1652) * _1441) + _1652;
        float _1662 = (((_40_m0[59u].w * _1635.z) - _1653) * _1441) + _1653;
        float _1663 = _1232 * _1063;
        float _1676 = exp2(clamp(dot(float3(_796, _797, _798), float3(_192, _193, _194)), 0.0f, 1.0f) * (-9.27999973297119140625f));
        float _1683 = (((min(_1663 * 0.4749999940395355224609375f, _1676) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _1663) + (-0.015625f);
        float _1689 = clamp(((((_1663 * 0.25f) + 0.75f) - _1683) * _30_m0[17u].w) + _1683, 0.0f, 1.0f) * _1663;
        float _1693 = _1232 * _1064;
        float _1701 = (((min(_1693 * 0.4749999940395355224609375f, _1676) * 1.04166662693023681640625f) + 0.01822916604578495025634765625f) * _1693) + (-0.015625f);
        float _1706 = clamp(((((_1693 * 0.25f) + 0.75f) - _1701) * _30_m0[17u].w) + _1701, 0.0f, 1.0f) * _1693;
        float _1721 = ((_1184 * _40_m0[73u].x) * (((((_1195 * _1195) * (3.0f - (_1195 * 2.0f))) + (-1.0f)) * _40_m0[75u].z) + 1.0f)) * clamp((((_40_m0[73u].z * (1.0f - _1259)) + _1259) - _40_m0[62u].x) / (_40_m0[62u].y - _40_m0[62u].x), 0.0f, 1.0f);
        _1468 = (((_1660 * (_1706 - _1689)) * _30_m0[18u].x) + (_1689 * _1660)) * _1721;
        _1470 = (((_1661 * (_1706 - _1689)) * _30_m0[18u].x) + (_1689 * _1661)) * _1721;
        _1472 = (((_1662 * (_1706 - _1689)) * _30_m0[18u].x) + (_1689 * _1662)) * _1721;
        _1474 = 1.0f;
    }
    uint4 _1481 = asuint(_35_m0[17u]);
    uint _1482 = _1481.x;
    uint _1487 = asuint(_30_m0[23u].x);
    float _1488 = max(_1063, _1063);
    float _1528 = clamp((exp2(log2(clamp(max(max(max(abs(ddx_fine(_199)), abs(ddx_fine(_200))), abs(ddx_fine(_201))), max(max(abs(ddy_fine(_199)), abs(ddy_fine(_200))), abs(ddy_fine(_201)))), 0.0f, 1.0f)) * _40_m0[115u].w) - _40_m0[115u].y) / (_40_m0[115u].z - _40_m0[115u].y), 0.0f, 1.0f) * _40_m0[115u].x;
    float _1535 = (_1528 * (_192 - _796)) + _796;
    float _1536 = (_1528 * (_193 - _797)) + _797;
    float _1537 = (_1528 * (_194 - _798)) + _798;
    float _1541 = rsqrt(dot(float3(_1535, _1536, _1537), float3(_1535, _1536, _1537)));
    float _1542 = _1535 * _1541;
    float _1543 = _1536 * _1541;
    float _1544 = _1537 * _1541;
    bool _1545 = asuint(_40_m0[176u]).z != 0u;
    float _1557 = clamp((_1488 - _40_m0[103u].x) / (_40_m0[103u].y - _40_m0[103u].x), 0.0f, 1.0f);
    bool _1566 = ((_1557 * _1557) * (3.0f - (_1557 * 2.0f))) > 0.0f;
    uint _1568 = uint(_1566);
    uint _1609;
    if ((asuint(_40_m0[103u]).w != 0u) && (_1566 && (asuint(_40_m0[114u]).z != 0u)))
    {
        uint frontier_phi_22_21_ladder;
        if (abs((_40_m0[8u].y + WORLDPOS.y) - _40_m0[116u].w) < 0.0500000007450580596923828125f)
        {
            uint frontier_phi_22_21_ladder_24_ladder;
            if (dot(float3(_40_m0[116u].xyz), float3(_1545 ? _199 : _796, _1545 ? _200 : _797, _1545 ? _201 : _798)) > 0.999000012874603271484375f)
            {
                frontier_phi_22_21_ladder_24_ladder = _1568;
            }
            else
            {
                frontier_phi_22_21_ladder_24_ladder = 0u;
            }
            frontier_phi_22_21_ladder = frontier_phi_22_21_ladder_24_ladder;
        }
        else
        {
            frontier_phi_22_21_ladder = 0u;
        }
        _1609 = frontier_phi_22_21_ladder;
    }
    else
    {
        _1609 = _1568;
    }
    float _1610 = (((MOTIONBLUR_POS.x / MOTIONBLUR_POS.w) + _40_m0[192u].w) - (MOTIONBLUR_POS_1.x / MOTIONBLUR_POS_1.w)) * 1280.0f;
    float _1612 = (((MOTIONBLUR_POS.y / MOTIONBLUR_POS.w) + _40_m0[193u].w) - (MOTIONBLUR_POS_1.y / MOTIONBLUR_POS_1.w)) * (-720.0f);
    float _1626 = abs(_1610);
    float _1627 = abs(_1612);
    float _1628 = max(_1626, _1627);
    float _1733;
    float _1734;
    if (_1628 > 256.0f)
    {
        float _1730 = 256.0f / _1628;
        _1733 = _1730 * _1626;
        _1734 = _1730 * _1627;
    }
    else
    {
        _1733 = _1626;
        _1734 = _1627;
    }
    float _1755 = (abs(_1543) + abs(_1542)) + abs(_1544);
    float _1756 = _1542 / _1755;
    float _1757 = _1543 / _1755;
    float _1770;
    float _1771;
    if ((_1544 / _1755) < 0.0f)
    {
        _1770 = (1.0f - abs(_1757)) * ((_1756 >= 0.0f) ? 1.0f : (-1.0f));
        _1771 = (1.0f - abs(_1756)) * ((_1757 >= 0.0f) ? 1.0f : (-1.0f));
    }
    else
    {
        _1770 = _1756;
        _1771 = _1757;
    }
    uint _1802 = uint(clamp(_1259, 0.0f, 1.0f) * 127.0f);
    precise float _1809 = clamp(exp2(log2(_30_m0[18u].y + _799) * _30_m0[18u].z), 0.0f, 1.0f) * 127.0f;
    float _1814;
    if ((_1482 & 32u) == 0u)
    {
        _1814 = 0.0f;
    }
    else
    {
        _1814 = dot(_30_m0[17u].w.xxx, float3(0.21267099678516387939453125f, 0.71516001224517822265625f, 0.072168998420238494873046875f));
    }
    bool _1818 = (_1482 & 64u) == 0u;
    uint _1862;
    if ((_1482 & 1u) == 0u)
    {
        uint _1831 = spvPackHalf2x16(float2(min(_30_m0[17u].w, 64512.0f), 0.0f));
        _1862 = ((((_1831 << 7u) + 1024u) & 4192256u) | (((_1831 + 8u) >> 4u) & 2047u)) | (((_1831 << 17u) + 2097152u) & 4290772992u);
    }
    else
    {
        uint _1849 = spvPackHalf2x16(0.0f.xx);
        _1862 = ((((_1849 << 7u) + 1024u) & 4192256u) | (((_1849 + 8u) >> 4u) & 2047u)) | (((_1849 << 17u) + 2097152u) & 4290772992u);
    }
    uint _1888 = ((((spvPackHalf2x16(float2(min(((_1087 * _1080) * 0.5f) + 0.5f, 64512.0f), 0.0f)) << 7u) + 1024u) & 4192256u) | (((spvPackHalf2x16(float2(min(((_1087 * _1077) * 0.5f) + 0.5f, 64512.0f), 0.0f)) + 8u) >> 4u) & 2047u)) | (((spvPackHalf2x16(float2(min(((_1087 * _1083) * 0.5f) + 0.5f, 64512.0f), 0.0f)) << 17u) + 2097152u) & 4290772992u);
    SV_Target.x = _40_m0[59u].x * _1468;
    SV_Target.y = _40_m0[59u].x * _1470;
    SV_Target.z = _40_m0[59u].x * _1472;
    SV_Target.w = _45_m0[28u].w;
    SV_Target_1.x = (((float(int(uint(_1610 > 0.0f) - uint(_1610 < 0.0f))) * 32.0f) * sqrt(_1733)) + 512.0f) * 0.000977517105638980865478515625f;
    SV_Target_1.y = (((float(int(uint(_1612 > 0.0f) - uint(_1612 < 0.0f))) * 32.0f) * sqrt(_1734)) + 512.0f) * 0.000977517105638980865478515625f;
    SV_Target_1.z = (((MOTIONBLUR_POS.z / MOTIONBLUR_POS.w) - (MOTIONBLUR_POS_1.z / MOTIONBLUR_POS_1.w)) * 32.0f) + 0.500488758087158203125f;
    SV_Target_1.w = 0.0f;
    SV_Target_2.x = _1487 >> 7u;
    SV_Target_2.y = uint(_1809);
    SV_Target_2.z = (_1609 != 0u) ? (_1802 | 128u) : _1802;
    SV_Target_2.w = _1487 & 127u;
    SV_Target_3 = ((uint(min(max(round((_1771 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 20u) | (uint(min(max(round((_1770 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 8u)) | (uint(clamp(_1488, 0.0f, 1.0f) * 255.0f) & 255u);
    SV_Target_4.x = (_1036 * (_1015 - _970)) + _970;
    SV_Target_4.y = (_1036 * (_1013.y - _971)) + _971;
    SV_Target_4.z = (_1036 * (_1013.z - _972)) + _972;
    SV_Target_4.w = _1814;
    SV_Target_5.x = _1818 ? (_30_m0[19u].y * _337.y) : 0.0f;
    SV_Target_5.y = _1818 ? _1064 : 0.0f;
    SV_Target_5.z = _1818 ? (((_30_m0[9u].y - _30_m0[9u].x) * _337.x) + _30_m0[9u].x) : 0.0f;
    SV_Target_5.w = _1474 * _1184;
    SV_Target_6.x = _1862;
    SV_Target_6.y = _1862;
    SV_Target_6.z = _1862;
    SV_Target_6.w = _1862;
    SV_Target_7.x = _1888;
    SV_Target_7.y = _1888;
    SV_Target_7.z = _1888;
    SV_Target_7.w = _1888;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    TEXCOORD = stage_input.TEXCOORD;
    TEXCOORD_1 = stage_input.TEXCOORD_1;
    TEXCOORD_2 = stage_input.TEXCOORD_2;
    TEXCOORD_3 = stage_input.TEXCOORD_3;
    WORLDPOS = stage_input.WORLDPOS;
    WORLDNORMAL = stage_input.WORLDNORMAL;
    TEXCOORD_4 = stage_input.TEXCOORD_4;
    TEXCOORD4y = stage_input.TEXCOORD4y;
    WORLDTANGENT = stage_input.WORLDTANGENT;
    WORLDTANGENT_1 = stage_input.WORLDTANGENT_1;
    WORLDTANGENT_2 = stage_input.WORLDTANGENT_2;
    COLOR = stage_input.COLOR;
    DAMAGEIMPACT = stage_input.DAMAGEIMPACT;
    DAMAGESCRAPE = stage_input.DAMAGESCRAPE;
    MOTIONBLUR_POS = stage_input.MOTIONBLUR_POS;
    MOTIONBLUR_POS_1 = stage_input.MOTIONBLUR_POS_1;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.SV_Target = SV_Target;
    stage_output.SV_Target_1 = SV_Target_1;
    stage_output.SV_Target_2 = SV_Target_2;
    stage_output.SV_Target_3 = SV_Target_3;
    stage_output.SV_Target_4 = SV_Target_4;
    stage_output.SV_Target_5 = SV_Target_5;
    stage_output.SV_Target_6 = SV_Target_6;
    stage_output.SV_Target_7 = SV_Target_7;
    return stage_output;
}
