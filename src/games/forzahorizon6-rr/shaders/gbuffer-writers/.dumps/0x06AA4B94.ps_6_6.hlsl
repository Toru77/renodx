static uint _592;

cbuffer _10_12 : register(b0, space2)
{
    float4 _12_m0[1] : packoffset(c0);
};

cbuffer _14_16 : register(b1, space2)
{
    float4 _16_m0[1] : packoffset(c0);
};

cbuffer _19_21 : register(b0, space3)
{
    float4 _21_m0[4096] : packoffset(c0);
};

cbuffer _24_26 : register(b9, space0)
{
    float4 _26_m0[25] : packoffset(c0);
};

cbuffer _29_31 : register(b2, space0)
{
    float4 _31_m0[700] : packoffset(c0);
};

cbuffer _34_36 : register(b7, space0)
{
    float4 _36_m0[65] : packoffset(c0);
};

cbuffer _39_41 : register(b3, space0)
{
    float4 _41_m0[1145] : packoffset(c0);
};


static float3 WORLDPOS;
static float3 WORLDNORMAL;
static float4 MOTIONBLUR_POS;
static float4 MOTIONBLUR_POS_1;
static float4 SV_Target;
static float4 SV_Target_1;
static uint4 SV_Target_2;
static uint SV_Target_3;
static float4 SV_Target_4;
static float4 SV_Target_5;

struct SPIRV_Cross_Input
{
    float3 WORLDPOS : TEXCOORD0;
    centroid float3 WORLDNORMAL : TEXCOORD1;
    float4 MOTIONBLUR_POS : TEXCOORD3;
    float4 MOTIONBLUR_POS_1 : TEXCOORD4;
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
    float _105 = (-0.0f) - WORLDPOS.x;
    float _107 = (-0.0f) - WORLDPOS.y;
    float _108 = (-0.0f) - WORLDPOS.z;
    float _113 = rsqrt(dot(float3(_105, _107, _108), float3(_105, _107, _108)));
    float _120 = rsqrt(dot(float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z), float3(WORLDNORMAL.x, WORLDNORMAL.y, WORLDNORMAL.z)));
    float _121 = _120 * WORLDNORMAL.x;
    float _122 = _120 * WORLDNORMAL.y;
    float _123 = _120 * WORLDNORMAL.z;
    uint _154 = uint(int(asuint(_12_m0[0u]).x) >> int(16u));
    uint _156 = _154 + 1u;
    float _166 = dot(float3(_21_m0[_156].xyz), float3(0.300000011920928955078125f, 0.589999973773956298828125f, 0.10999999940395355224609375f));
    float _174 = clamp(_122 + 1.0f, 0.0f, 1.0f) * (_36_m0[3u].z + (-1.0f));
    float _175 = _174 + 1.0f;
    float _181 = 1.0f - (_174 * 0.20000000298023223876953125f);
    float _188 = (((_175 * _21_m0[_156].x) - _166) * _181) + _166;
    float _189 = (((_175 * _21_m0[_156].y) - _166) * _181) + _166;
    float _190 = (((_175 * _21_m0[_156].z) - _166) * _181) + _166;
    float _195 = ddx_fine(_121);
    float _196 = ddx_fine(_122);
    float _197 = ddx_fine(_123);
    float _201 = ddy_fine(_121);
    float _202 = ddy_fine(_122);
    float _203 = ddy_fine(_123);
    float _216 = max(_31_m0[43u].x, 1.0f);
    float _249 = ((exp2(log2(clamp(max(_31_m0[44u].x, 1.0f), 0.0f, 1.0f)) * abs(_31_m0[92u].w)) + (-1.0f)) * _31_m0[92u].z) + 1.0f;
    float _253 = _249 * _249;
    float _268 = max(_188, _188);
    float _269 = max(_189, _189);
    float _270 = max(_190, _190);
    float _272 = clamp(_31_m0[22u].x / 0.0f, 0.0f, 1.0f);
    bool _286 = (1.0f - (float(int(asuint(_16_m0[0u]).x & 127u)) * 0.0078740157186985015869140625f)) <= 0.984615385532379150390625f;
    uint _295;
    if (_286)
    {
        _295 = uint(asuint(_31_m0[101u]).x != 0u);
    }
    else
    {
        _295 = 0u;
    }
    uint _320;
    uint _302;
    bool _308;
    for (;;)
    {
        uint _296 = _154 + 3u;
        _302 = asuint(_286 ? _21_m0[_296].y : _21_m0[_296].x);
        _308 = asuint(_41_m0[156u]).z == 0u;
        if (_308)
        {
            if (asuint(_41_m0[157u]).y == 0u)
            {
                _320 = 0u;
                break;
            }
        }
        _320 = asuint(_26_m0[17u]).x;
        break;
    }
    float _333 = (1.0f - clamp((sqrt(((_269 * _269) + (_268 * _268)) + (_270 * _270)) + (-0.0199999995529651641845703125f)) * 33.333332061767578125f, 0.0f, 1.0f)) * 0.4880338609218597412109375f;
    float _353 = _31_m0[59u].x * (((_272 * _272) * (_21_m0[_154 + 2u].x * _31_m0[11u].x)) * (3.0f - (_272 * 2.0f)));
    float _372 = clamp((exp2(log2(clamp(max(max(max(abs(_195), abs(_196)), abs(_197)), max(max(abs(_201), abs(_202)), abs(_203))), 0.0f, 1.0f)) * _31_m0[115u].w) - _31_m0[115u].y) / (_31_m0[115u].z - _31_m0[115u].y), 0.0f, 1.0f) * _31_m0[115u].x;
    float _379 = (_372 * ((_113 * _105) - _121)) + _121;
    float _380 = (_372 * ((_113 * _107) - _122)) + _122;
    float _381 = (_372 * ((_113 * _108) - _123)) + _123;
    float _385 = rsqrt(dot(float3(_379, _380, _381), float3(_379, _380, _381)));
    float _386 = _379 * _385;
    float _387 = _380 * _385;
    float _388 = _381 * _385;
    bool _392 = asuint(_31_m0[176u]).z != 0u;
    uint _398 = 102u - ((_320 >> 5u) | 4294967291u);
    float _408 = clamp((0.125f - _31_m0[_398].x) / (_31_m0[_398].y - _31_m0[_398].x), 0.0f, 1.0f);
    bool _417 = ((_408 * _408) * (3.0f - (_408 * 2.0f))) > 0.0f;
    uint _418 = uint(_417);
    uint _436;
    if ((asuint(_31_m0[_398]).w != 0u) && (_417 && (asuint(_31_m0[114u]).z != 0u)))
    {
        uint frontier_phi_8_7_ladder;
        if (abs((_31_m0[8u].y + WORLDPOS.y) - _31_m0[116u].w) < 0.0500000007450580596923828125f)
        {
            uint frontier_phi_8_7_ladder_9_ladder;
            if (dot(float3(_31_m0[116u].xyz), float3(_392 ? _121 : 0.0f, _392 ? _122 : 0.0f, _392 ? _123 : 0.0f)) > 0.999000012874603271484375f)
            {
                frontier_phi_8_7_ladder_9_ladder = _418;
            }
            else
            {
                frontier_phi_8_7_ladder_9_ladder = 0u;
            }
            frontier_phi_8_7_ladder = frontier_phi_8_7_ladder_9_ladder;
        }
        else
        {
            frontier_phi_8_7_ladder = 0u;
        }
        _436 = frontier_phi_8_7_ladder;
    }
    else
    {
        _436 = _418;
    }
    float _437 = (((MOTIONBLUR_POS.x / MOTIONBLUR_POS.w) - (MOTIONBLUR_POS_1.x / MOTIONBLUR_POS_1.w)) + _31_m0[192u].w) * 1280.0f;
    float _439 = (((MOTIONBLUR_POS.y / MOTIONBLUR_POS.w) - (MOTIONBLUR_POS_1.y / MOTIONBLUR_POS_1.w)) + _31_m0[193u].w) * (-720.0f);
    float _453 = abs(_437);
    float _454 = abs(_439);
    float _455 = max(_453, _454);
    float _469;
    float _470;
    if (_455 > 256.0f)
    {
        float _466 = 256.0f / _455;
        _469 = _466 * _453;
        _470 = _466 * _454;
    }
    else
    {
        _469 = _453;
        _470 = _454;
    }
    float _491 = (abs(_387) + abs(_386)) + abs(_388);
    float _492 = _386 / _491;
    float _493 = _387 / _491;
    float _506;
    float _507;
    if ((_388 / _491) < 0.0f)
    {
        _506 = (1.0f - abs(_493)) * ((_492 >= 0.0f) ? 1.0f : (-1.0f));
        _507 = (1.0f - abs(_492)) * ((_493 >= 0.0f) ? 1.0f : (-1.0f));
    }
    else
    {
        _506 = _492;
        _507 = _493;
    }
    uint _532 = uint(clamp(((1.0f - _253) * clamp((abs(dot(float3(_41_m0[0u].xyz), float3(_121, _122, _123))) + (-1.0f)) + (_253 * 2.0f), 0.0f, 1.0f)) + _253, 0.0f, 1.0f) * 127.0f);
    uint _538 = uint(clamp(((exp2(log2(clamp(_216, 0.0f, 1.0f)) * abs(_31_m0[92u].y)) + (-1.0f)) * _31_m0[92u].x) + 1.0f, 0.0f, 1.0f) * 127.0f);
    SV_Target.x = _353 * _21_m0[_156].x;
    SV_Target.y = _353 * _21_m0[_156].y;
    SV_Target.z = _353 * _21_m0[_156].z;
    SV_Target.w = 1.0f;
    SV_Target_1.x = (((float(int(uint(_437 > 0.0f) - uint(_437 < 0.0f))) * 32.0f) * sqrt(_469)) + 512.0f) * 0.000977517105638980865478515625f;
    SV_Target_1.y = (((float(int(uint(_439 > 0.0f) - uint(_439 < 0.0f))) * 32.0f) * sqrt(_470)) + 512.0f) * 0.000977517105638980865478515625f;
    SV_Target_1.z = (((MOTIONBLUR_POS.z / MOTIONBLUR_POS.w) - (MOTIONBLUR_POS_1.z / MOTIONBLUR_POS_1.w)) * 32.0f) + 0.500488758087158203125f;
    SV_Target_1.w = 0.0f;
    SV_Target_2.x = _302 >> 7u;
    SV_Target_2.y = (_295 != 0u) ? (_532 | 128u) : _532;
    SV_Target_2.z = (_436 != 0u) ? (_538 | 128u) : _538;
    SV_Target_2.w = _302 & 127u;
    SV_Target_3 = ((uint(min(max(round((_506 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 8u) | (uint(min(max(round((_507 * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 20u)) | 31u;
    SV_Target_4.x = (_333 * (0.039999999105930328369140625f - _268)) + _268;
    SV_Target_4.y = (_333 * (0.039999999105930328369140625f - _269)) + _269;
    SV_Target_4.z = (_333 * (0.039999999105930328369140625f - _270)) + _270;
    SV_Target_4.w = _333;
    SV_Target_5.x = 0.0f;
    SV_Target_5.y = float((_320 & 64u) == 0u);
    SV_Target_5.z = 0.0f;
    SV_Target_5.w = _216;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    WORLDPOS = stage_input.WORLDPOS;
    WORLDNORMAL = stage_input.WORLDNORMAL;
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
    return stage_output;
}
