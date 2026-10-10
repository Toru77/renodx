cbuffer _19_21 : register(b9, space0)
{
    float4 _21_m0[25] : packoffset(c0);
};

cbuffer _24_26 : register(b2, space0)
{
    float4 _26_m0[700] : packoffset(c0);
};

cbuffer _29_31 : register(b3, space0)
{
    float4 _31_m0[1138] : packoffset(c0);
};

Texture2D<float4> _8 : register(t18, space0);
Texture2D<float4> _11[] : register(t0, space6);
RWTexture2D<uint4> _15 : register(u1, space0);
SamplerState _34 : register(s1, space0);

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
    if (!(asuint(_31_m0[1137u]).x == 0u))
    {
        uint _207 = uint(gl_FragCoord.x);
        uint _208 = uint(gl_FragCoord.y);
        bool _218 = ((_208 + _207) & 1u) == 0u;
        if ((dot(float2(ddx_coarse(gl_FragCoord.z), ddy_coarse(gl_FragCoord.z)), float2(_218 ? _26_m0[58u].x : _26_m0[58u].z, _218 ? _26_m0[58u].y : _26_m0[58u].w)) + gl_FragCoord.z) < _11[512u].Load(int3(uint2(_207, _208), 0u)).x)
        {
            discard_state = true;
        }
        if (!(asuint(_31_m0[1137u]).x == 0u))
        {
            _15[uint2(uint(gl_FragCoord.x * 0.125f), uint(gl_FragCoord.y * 0.125f))] = uint4(1u, 1u, 1u, 1u);
        }
    }
    float _121 = rsqrt(dot(float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z), float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z)));
    float _122 = _121 * WORLDNORMAL.x;
    float _123 = _121 * WORLDNORMAL.y;
    float _124 = _121 * WORLDNORMAL.z;
    float _128 = rsqrt(dot(float3(WORLDTANGENT.x, WORLDTANGENT.y, WORLDTANGENT.z), float3(WORLDTANGENT.x, WORLDTANGENT.y, WORLDTANGENT.z)));
    float _129 = _128 * WORLDTANGENT.x;
    float _130 = _128 * WORLDTANGENT.y;
    float _131 = _128 * WORLDTANGENT.z;
    float4 _144 = _8.Sample(_34, float2(TEXCOORD.x, TEXCOORD.y));
    float _151 = (_144.x * 2.0f) + (-1.0f);
    float _153 = (_144.y * 2.0f) + (-1.0f);
    float _154 = (-0.0f) - _153;
    float _162 = sqrt(clamp((1.0f - (_151 * _151)) - (_153 * _153), 0.0f, 1.0f));
    float _172 = ((_131 * _123) - (_130 * _124)) * WORLDTANGENT.w;
    float _173 = ((_129 * _124) - (_131 * _122)) * WORLDTANGENT.w;
    float _174 = ((_130 * _122) - (_129 * _123)) * WORLDTANGENT.w;
    float _178 = rsqrt(dot(float3(_172, _173, _174), float3(_172, _173, _174)));
    float _184 = mad(_162, _122, mad(_154, _178 * _172, _151 * _129));
    float _187 = mad(_162, _123, mad(_154, _178 * _173, _151 * _130));
    float _190 = mad(_162, _124, mad(_154, _178 * _174, _151 * _131));
    float _194 = rsqrt(dot(float3(_184, _187, _190), float3(_184, _187, _190)));
    float _195 = _194 * _184;
    float _196 = _194 * _187;
    float _197 = _194 * _190;
    float _202 = (abs(_196) + abs(_195)) + abs(_197);
    float _203 = _195 / _202;
    float _204 = _196 / _202;
    float _245;
    float _246;
    if ((_197 / _202) < 0.0f)
    {
        _245 = (1.0f - abs(_204)) * ((_203 >= 0.0f) ? 1.0f : (-1.0f));
        _246 = (1.0f - abs(_203)) * ((_204 >= 0.0f) ? 1.0f : (-1.0f));
    }
    else
    {
        _245 = _203;
        _246 = _204;
    }
    SV_Target = (uint(min(max(round((_246 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 20u) | (uint(min(max(round((_245 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 8u);
    uint4 _288 = asuint(_31_m0[1136u]);
    uint _289 = _288.y;
    uint _293 = (uint(int(_289) >> int(31u)) & 3u) + 103u;
    float _302 = clamp(((-0.0f) - _26_m0[_293].x) / (_26_m0[_293].y - _26_m0[_293].x), 0.0f, 1.0f);
    SV_Target_1.x = 0u;
    SV_Target_1.y = _289 + 1u;
    SV_Target_1.z = (((_302 * _302) * (3.0f - (_302 * 2.0f))) > 0.0f) ? 8u : 0u;
    SV_Target_1.w = asuint(_21_m0[17u]).x | 128u;
    float _315 = (((MOTIONBLUR_POS.x / MOTIONBLUR_POS.w) + _26_m0[192u].w) - (MOTIONBLUR_POS_1.x / MOTIONBLUR_POS_1.w)) * 1280.0f;
    float _317 = (((MOTIONBLUR_POS.y / MOTIONBLUR_POS.w) + _26_m0[193u].w) - (MOTIONBLUR_POS_1.y / MOTIONBLUR_POS_1.w)) * (-720.0f);
    float _331 = abs(_315);
    float _332 = abs(_317);
    float _333 = max(_331, _332);
    float _347;
    float _348;
    if (_333 > 256.0f)
    {
        float _344 = 256.0f / _333;
        _347 = _344 * _331;
        _348 = _344 * _332;
    }
    else
    {
        _347 = _331;
        _348 = _332;
    }
    SV_Target_2.x = (((float(int(uint(_315 > 0.0f) - uint(_315 < 0.0f))) * 32.0f) * sqrt(_347)) + 512.0f) * 0.000977517105638980865478515625f;
    SV_Target_2.y = (((float(int(uint(_317 > 0.0f) - uint(_317 < 0.0f))) * 32.0f) * sqrt(_348)) + 512.0f) * 0.000977517105638980865478515625f;
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
