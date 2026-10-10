cbuffer _20_22 : register(b0, space0)
{
    float4 _22_m0[7] : packoffset(c0);
};

cbuffer _25_27 : register(b9, space0)
{
    float4 _27_m0[25] : packoffset(c0);
};

cbuffer _30_32 : register(b2, space0)
{
    float4 _32_m0[700] : packoffset(c0);
};

cbuffer _35_37 : register(b4, space0)
{
    float4 _37_m0[69] : packoffset(c0);
};

cbuffer _40_42 : register(b3, space0)
{
    float4 _42_m0[1138] : packoffset(c0);
};

Texture2D<float4> _8 : register(t18, space0);
Texture2D<float4> _9 : register(t21, space0);
Texture2D<float4> _12[] : register(t0, space6);
RWTexture2D<uint4> _16 : register(u1, space0);
SamplerState _45 : register(s1, space0);

static float4 gl_FragCoord;
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
static uint SV_Target;
static uint4 SV_Target_1;
static float4 SV_Target_2;

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
    float4 gl_FragCoord : SV_Position;
};

struct SPIRV_Cross_Output
{
    uint SV_Target : SV_Target0;
    uint4 SV_Target_1 : SV_Target1;
    float4 SV_Target_2 : SV_Target2;
};

static bool discard_state;

void discard_exit()
{
    if (discard_state)
    {
        discard;
    }
}

void frag_main()
{
    discard_state = false;
    if (!(asuint(_42_m0[1137u]).x == 0u))
    {
        uint _255 = uint(gl_FragCoord.x);
        uint _256 = uint(gl_FragCoord.y);
        bool _266 = ((_256 + _255) & 1u) == 0u;
        if ((dot(float2(ddx_coarse(gl_FragCoord.z), ddy_coarse(gl_FragCoord.z)), float2(_266 ? _32_m0[58u].x : _32_m0[58u].z, _266 ? _32_m0[58u].y : _32_m0[58u].w)) + gl_FragCoord.z) < _12[512u].Load(int3(uint2(_255, _256), 0u)).x)
        {
            discard_state = true;
        }
        if (!(asuint(_42_m0[1137u]).x == 0u))
        {
            _16[uint2(uint(gl_FragCoord.x * 0.125f), uint(gl_FragCoord.y * 0.125f))] = uint4(1u, 1u, 1u, 1u);
        }
    }
    float _132 = rsqrt(dot(float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z), float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z)));
    float _133 = _132 * WORLDNORMAL.x;
    float _134 = _132 * WORLDNORMAL.y;
    float _135 = _132 * WORLDNORMAL.z;
    float _139 = rsqrt(dot(float3(WORLDTANGENT.x, WORLDTANGENT.y, WORLDTANGENT.z), float3(WORLDTANGENT.x, WORLDTANGENT.y, WORLDTANGENT.z)));
    float _140 = _139 * WORLDTANGENT.x;
    float _141 = _139 * WORLDTANGENT.y;
    float _142 = _139 * WORLDTANGENT.z;
    float4 _161 = _8.Sample(_45, float2(_22_m0[2u].z * TEXCOORD.x, _22_m0[2u].w * TEXCOORD.y));
    float4 _173 = _9.Sample(_45, float2(_22_m0[3u].x * TEXCOORD.x, _22_m0[3u].y * TEXCOORD.y));
    float _180 = (_173.x * 2.0f) + (-1.0f);
    float _182 = (_173.y * 2.0f) + (-1.0f);
    float _183 = (-0.0f) - _182;
    float _191 = sqrt(clamp((1.0f - (_180 * _180)) - (_182 * _182), 0.0f, 1.0f));
    float _201 = ((_142 * _134) - (_141 * _135)) * WORLDTANGENT.w;
    float _202 = ((_140 * _135) - (_142 * _133)) * WORLDTANGENT.w;
    float _203 = ((_141 * _133) - (_140 * _134)) * WORLDTANGENT.w;
    float _207 = rsqrt(dot(float3(_201, _202, _203), float3(_201, _202, _203)));
    float _213 = mad(_191, _133, mad(_183, _207 * _201, _180 * _140));
    float _216 = mad(_191, _134, mad(_183, _207 * _202, _180 * _141));
    float _219 = mad(_191, _135, mad(_183, _207 * _203, _180 * _142));
    float _223 = rsqrt(dot(float3(_213, _216, _219), float3(_213, _216, _219)));
    float _224 = _223 * _213;
    float _225 = _223 * _216;
    float _226 = _223 * _219;
    float _240 = rsqrt(dot(float3(_224, _225, _226), float3(_224, _225, _226)));
    float _241 = _240 * _224;
    float _242 = _240 * _225;
    float _243 = _240 * _226;
    float _244 = 1.0f - clamp(1.0f - clamp((_37_m0[46u].w * 0.5f) + _161.w, 0.0f, 1.0f), 0.0f, 1.0f);
    float _245 = max(_244, _244);
    float _250 = (abs(_242) + abs(_241)) + abs(_243);
    float _251 = _241 / _250;
    float _252 = _242 / _250;
    float _293;
    float _294;
    if ((_243 / _250) < 0.0f)
    {
        _293 = (1.0f - abs(_252)) * ((_251 >= 0.0f) ? 1.0f : (-1.0f));
        _294 = (1.0f - abs(_251)) * ((_252 >= 0.0f) ? 1.0f : (-1.0f));
    }
    else
    {
        _293 = _251;
        _294 = _252;
    }
    SV_Target = ((uint(min(max(round((_294 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 20u) | (uint(min(max(round((_293 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 8u)) | (uint(clamp(_245, 0.0f, 1.0f) * 255.0f) & 255u);
    uint4 _343 = asuint(_42_m0[1136u]);
    uint _344 = _343.y;
    uint _348 = (uint(int(_344) >> int(31u)) & 3u) + 103u;
    float _357 = clamp((_245 - _32_m0[_348].x) / (_32_m0[_348].y - _32_m0[_348].x), 0.0f, 1.0f);
    SV_Target_1.x = 0u;
    SV_Target_1.y = _344 + 1u;
    SV_Target_1.z = (((_357 * _357) * (3.0f - (_357 * 2.0f))) > 0.0f) ? 10u : 2u;
    SV_Target_1.w = asuint(_27_m0[17u]).x | 128u;
    float _371 = (((MOTIONBLUR_POS.x / MOTIONBLUR_POS.w) + _32_m0[192u].w) - (MOTIONBLUR_POS_1.x / MOTIONBLUR_POS_1.w)) * 1280.0f;
    float _373 = (((MOTIONBLUR_POS.y / MOTIONBLUR_POS.w) + _32_m0[193u].w) - (MOTIONBLUR_POS_1.y / MOTIONBLUR_POS_1.w)) * (-720.0f);
    float _387 = abs(_371);
    float _388 = abs(_373);
    float _389 = max(_387, _388);
    float _403;
    float _404;
    if (_389 > 256.0f)
    {
        float _400 = 256.0f / _389;
        _403 = _400 * _387;
        _404 = _400 * _388;
    }
    else
    {
        _403 = _387;
        _404 = _388;
    }
    SV_Target_2.x = (((float(int(uint(_371 > 0.0f) - uint(_371 < 0.0f))) * 32.0f) * sqrt(_403)) + 512.0f) * 0.000977517105638980865478515625f;
    SV_Target_2.y = (((float(int(uint(_373 > 0.0f) - uint(_373 < 0.0f))) * 32.0f) * sqrt(_404)) + 512.0f) * 0.000977517105638980865478515625f;
    SV_Target_2.z = (((MOTIONBLUR_POS.z / MOTIONBLUR_POS.w) - (MOTIONBLUR_POS_1.z / MOTIONBLUR_POS_1.w)) * 32.0f) + 0.500488758087158203125f;
    SV_Target_2.w = 0.0f;
    if (!((((MOTIONBLUR_POS.x != 0.0f) || (MOTIONBLUR_POS.y != 0.0f)) || (MOTIONBLUR_POS.z != 0.0f)) || (MOTIONBLUR_POS.w != 0.0f)))
    {
        if (!((((MOTIONBLUR_POS_1.x != 0.0f) || (MOTIONBLUR_POS_1.y != 0.0f)) || (MOTIONBLUR_POS_1.z != 0.0f)) || (MOTIONBLUR_POS_1.w != 0.0f)))
        {
            SV_Target_2.x = 0.0f;
            SV_Target_2.y = 0.0f;
            SV_Target_2.z = 0.0f;
            SV_Target_2.w = 1.0f;
        }
    }
    discard_exit();
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    gl_FragCoord = stage_input.gl_FragCoord;
    gl_FragCoord.w = 1.0 / gl_FragCoord.w;
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
    return stage_output;
}
