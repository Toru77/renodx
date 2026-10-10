static uint _2274;
static uint _2275;
static uint _2362;
static uint _2363;
static uint _2364;
static uint _2374;
static uint _2672;
static uint _2904;
static uint _3304;

cbuffer _60_62 : register(b2, space0)
{
    float4 _62_m0[700] : packoffset(c0);
};

cbuffer _65_67 : register(b3, space0)
{
    float4 _67_m0[1136] : packoffset(c0);
};

cbuffer _70_72 : register(b0, space0)
{
    float4 _72_m0[15] : packoffset(c0);
};

cbuffer _74_76 : register(b1, space0)
{
    float4 _76_m0[15] : packoffset(c0);
};

cbuffer _79_81 : register(b4, space0)
{
    float4 _81_m0[21] : packoffset(c0);
};

cbuffer _84_86 : register(b5, space0)
{
    float4 _86_m0[257] : packoffset(c0);
};

cbuffer _89_91 : register(b6, space0)
{
    float4 _91_m0[17] : packoffset(c0);
};

Texture2D<float4> _9[] : register(t0, space1);
Texture2D<float4> _12[] : register(t0, space6);
Texture3D<float4> _16[] : register(t0, space7);
Texture2DArray<float4> _20[] : register(t0, space9);
Texture2D<float4> _22 : register(t115, space0);
TextureCube<float4> _25 : register(t117, space0);
Buffer<uint4> _29 : register(t108, space0);
Buffer<uint4> _30 : register(t109, space0);
Texture3D<uint4> _33 : register(t0, space12);
Buffer<uint4> _34 : register(t1, space12);
Buffer<uint4> _35 : register(t2, space12);
Buffer<uint4> _36 : register(t3, space12);
Buffer<uint4> _37 : register(t9, space12);
Texture2DArray<float4> _39 : register(t4, space12);
Buffer<uint4> _40 : register(t5, space12);
Buffer<uint4> _41 : register(t8, space12);
TextureCube<float4> _42 : register(t0, space0);
Texture3D<float4> _44 : register(t16, space0);
Texture3D<float4> _45 : register(t17, space0);
Texture2D<float4> _46 : register(t20, space0);
Texture3D<float4> _47 : register(t21, space0);
Texture2D<float4> _48 : register(t22, space0);
Texture3D<uint4> _49 : register(t23, space0);
Buffer<uint4> _50 : register(t24, space0);
Buffer<uint4> _51 : register(t25, space0);
RWTexture3D<float4> _54 : register(u0, space0);
RWTexture3D<float4> _55 : register(u1, space0);
RWTexture3D<float4> _56 : register(u2, space0);
SamplerState _94 : register(s0, space0);
SamplerState _95 : register(s2, space0);
SamplerState _96 : register(s5, space0);
SamplerState _97 : register(s1, space0);
SamplerState _98 : register(s3, space0);
SamplerComparisonState _99 : register(s10, space0);
SamplerComparisonState _100 : register(s7, space0);

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
    float _117[3];
    uint _107;
    uint _110;
    uint _113;
    uint _124;
    float4 _128;
    float _129;
    for (;;)
    {
        _107 = gl_GlobalInvocationID.x;
        _110 = gl_GlobalInvocationID.y;
        _113 = gl_GlobalInvocationID.z;
        uint4 _123 = asuint(_86_m0[_113 + 1u]);
        _124 = _123.x;
        _128 = _72_m0[9u];
        _129 = _128.x;
        if (float(int(_124)) < _129)
        {
            bool _136 = _113 < asuint(_86_m0[0u]).x;
            bool _142 = asuint(_81_m0[5u]).z != 0u;
            float _143 = float(_107);
            float _144 = float(_110);
            float _145 = float(_124);
            float _153;
            float _155;
            float _157;
            float _159;
            float _161;
            float _163;
            float _165;
            uint _168;
            if (_136 && (_81_m0[4u].y <= 0.0f))
            {
                _153 = 0.0f;
                _155 = 0.0f;
                _157 = 0.0f;
                _159 = 0.0f;
                _161 = 0.0f;
                _163 = 0.0f;
                _165 = 1.0f;
                _168 = 0u;
            }
            else
            {
                float _183 = (_143 + 0.5f) / _129;
                float _184 = (_144 + 0.5f) / _128.y;
                float _185 = (_145 + 0.5f) / _128.z;
                float _202 = ((_72_m0[2u].x - _72_m0[1u].x) * _183) + _72_m0[1u].x;
                float _203 = ((_72_m0[2u].y - _72_m0[1u].y) * _183) + _72_m0[1u].y;
                float _204 = ((_72_m0[2u].z - _72_m0[1u].z) * _183) + _72_m0[1u].z;
                float _302;
                if (_72_m0[11u].z < 0.001000000047497451305389404296875f)
                {
                    _302 = _185;
                }
                else
                {
                    _302 = (exp2(log2(_72_m0[11u].z + 1.0f) * _185) + (-1.0f)) / _72_m0[11u].z;
                }
                float _305 = _302 * _128.w;
                float _306 = _305 * ((((_72_m0[3u].x - _202) + ((_72_m0[4u].x - _72_m0[3u].x) * _183)) * _184) + _202);
                float _307 = _305 * ((((_72_m0[3u].y - _203) + ((_72_m0[4u].y - _72_m0[3u].y) * _183)) * _184) + _203);
                float _308 = _305 * ((((_72_m0[3u].z - _204) + ((_72_m0[4u].z - _72_m0[3u].z) * _183)) * _184) + _204);
                float _322 = (_306 + _72_m0[0u].x) - _76_m0[0u].x;
                float _323 = (_307 + _72_m0[0u].y) - _76_m0[0u].y;
                float _324 = (_308 + _72_m0[0u].z) - _76_m0[0u].z;
                float _349 = dot(float3(_322, _323, _324), float3(_76_m0[7u].xyz));
                float _355 = _349 / _76_m0[9u].w;
                float _394;
                if (_76_m0[11u].z < 0.001000000047497451305389404296875f)
                {
                    _394 = _355;
                }
                else
                {
                    _394 = log2((_76_m0[11u].z * _355) + 1.0f) / log2(_76_m0[11u].z + 1.0f);
                }
                float _404 = ((dot(float3(_322, _323, _324), float3(_76_m0[5u].xyz)) / _349) - _76_m0[8u].x) / _76_m0[8u].y;
                float _408 = ((dot(float3(_322, _323, _324), float3(_76_m0[6u].xyz)) / _349) - _76_m0[8u].z) / _76_m0[8u].w;
                uint frontier_phi_3_11_ladder;
                float frontier_phi_3_11_ladder_1;
                float frontier_phi_3_11_ladder_2;
                float frontier_phi_3_11_ladder_3;
                float frontier_phi_3_11_ladder_4;
                float frontier_phi_3_11_ladder_5;
                float frontier_phi_3_11_ladder_6;
                float frontier_phi_3_11_ladder_7;
                if ((_394 <= 1.0f) && ((_394 >= 0.0f) && ((_408 <= 1.0f) && (((_404 >= 0.0f) && (_404 <= 1.0f)) && (_408 >= 0.0f)))))
                {
                    float4 _543 = _44.SampleLevel(_94, float3(_404, _408, _394), 0.0f);
                    float _154 = _543.x;
                    float _571;
                    if (_136)
                    {
                        _571 = (_185 * (_81_m0[4u].y - _81_m0[4u].x)) + _81_m0[4u].x;
                    }
                    else
                    {
                        _571 = 1.0f;
                    }
                    float4 _574 = _45.SampleLevel(_94, float3(_404, _408, _394), 0.0f);
                    float _164 = _574.x;
                    float _162;
                    if (_142)
                    {
                        float _719 = clamp((sqrt(((_306 * _306) + (_307 * _307)) + (_308 * _308)) - _81_m0[5u].y) / (_81_m0[5u].x - _81_m0[5u].y), 0.0f, 1.0f);
                        _162 = _571 - (clamp(((_719 * _719) * _164) * (3.0f - (_719 * 2.0f)), 0.0f, 1.0f) * _571);
                    }
                    else
                    {
                        _162 = _571;
                    }
                    if (!(_162 < 1.0f))
                    {
                        _54[uint3(_107, _110, _124)] = float4(_154, _543.yzw);
                        if (!(asuint(_81_m0[19u]).x == 0u))
                        {
                            _56[uint3(_107, _110, _124)] = float4(_154, _543.yzw);
                        }
                        if (!_142)
                        {
                            break;
                        }
                        _55[uint3(_107, _110, _124)] = float4(_164, _574.y, _164, _164);
                        break;
                    }
                    frontier_phi_3_11_ladder = 1u;
                    frontier_phi_3_11_ladder_1 = _574.y;
                    frontier_phi_3_11_ladder_2 = _164;
                    frontier_phi_3_11_ladder_3 = _162;
                    frontier_phi_3_11_ladder_4 = _543.w;
                    frontier_phi_3_11_ladder_5 = _543.z;
                    frontier_phi_3_11_ladder_6 = _543.y;
                    frontier_phi_3_11_ladder_7 = _154;
                }
                else
                {
                    frontier_phi_3_11_ladder = 0u;
                    frontier_phi_3_11_ladder_1 = 1.0f;
                    frontier_phi_3_11_ladder_2 = 0.0f;
                    frontier_phi_3_11_ladder_3 = 0.0f;
                    frontier_phi_3_11_ladder_4 = 0.0f;
                    frontier_phi_3_11_ladder_5 = 0.0f;
                    frontier_phi_3_11_ladder_6 = 0.0f;
                    frontier_phi_3_11_ladder_7 = 0.0f;
                }
                _153 = frontier_phi_3_11_ladder_7;
                _155 = frontier_phi_3_11_ladder_6;
                _157 = frontier_phi_3_11_ladder_5;
                _159 = frontier_phi_3_11_ladder_4;
                _161 = frontier_phi_3_11_ladder_3;
                _163 = frontier_phi_3_11_ladder_2;
                _165 = frontier_phi_3_11_ladder_1;
                _168 = frontier_phi_3_11_ladder;
            }
            uint _172 = ((_110 << 1u) & 2u) | (_107 & 1u);
            float _242;
            if (_124 == 0u)
            {
                _242 = max(_81_m0[_172].x, 0.00999999977648258209228515625f);
            }
            else
            {
                _242 = _81_m0[_172].x;
            }
            float _248 = (_143 + 0.5f) / _129;
            float _249 = (_144 + 0.5f) / _128.y;
            float _250 = (_242 + _145) / _128.z;
            float _267 = ((_72_m0[2u].x - _72_m0[1u].x) * _248) + _72_m0[1u].x;
            float _268 = ((_72_m0[2u].y - _72_m0[1u].y) * _248) + _72_m0[1u].y;
            float _269 = ((_72_m0[2u].z - _72_m0[1u].z) * _248) + _72_m0[1u].z;
            bool _301 = _72_m0[11u].z < 0.001000000047497451305389404296875f;
            float _366;
            if (_301)
            {
                _366 = _250;
            }
            else
            {
                _366 = (exp2(log2(_72_m0[11u].z + 1.0f) * _250) + (-1.0f)) / _72_m0[11u].z;
            }
            float _369 = _366 * _128.w;
            float _370 = _369 * ((((_72_m0[3u].x - _267) + ((_72_m0[4u].x - _72_m0[3u].x) * _248)) * _249) + _267);
            float _371 = _369 * ((((_72_m0[3u].y - _268) + ((_72_m0[4u].y - _72_m0[3u].y) * _248)) * _249) + _268);
            float _372 = (-0.0f) - _371;
            float _374 = _369 * ((((_72_m0[3u].z - _269) + ((_72_m0[4u].z - _72_m0[3u].z) * _248)) * _249) + _269);
            float _380 = sqrt(((_370 * _370) + (_371 * _371)) + (_374 * _374));
            float _386 = _72_m0[0u].x + _370;
            float _387 = _72_m0[0u].y + _371;
            float _388 = _72_m0[0u].z + _374;
            float _425;
            if (_301)
            {
                _425 = _250;
            }
            else
            {
                _425 = (exp2(log2(_72_m0[11u].z + 1.0f) * _250) + (-1.0f)) / _72_m0[11u].z;
            }
            float _437 = (_72_m0[10u].x + (-1.0f)) * _248;
            float _440 = (_72_m0[10u].y + (-1.0f)) * _249;
            float _441 = 1.0f - (_72_m0[10u].w - (_72_m0[10u].z / (_425 * _128.w)));
            uint4 _529 = asuint(_91_m0[16u]);
            uint _657;
            float _659;
            float _663;
            float _667;
            float _671;
            float _675;
            float _679;
            float _683;
            if (asuint(_81_m0[17u]).x == 0u)
            {
                uint _643;
                float _644;
                float _645;
                float _646;
                float _647;
                float _648;
                float _649;
                float _650;
                if (_81_m0[12u].y > 0.0f)
                {
                    float _579 = clamp((_380 - _81_m0[10u].y) / (_81_m0[10u].z - _81_m0[10u].y), 0.0f, 1.0f);
                    float _587 = max(_81_m0[9u].w, ((_579 * _579) * _81_m0[10u].x) * (3.0f - (_579 * 2.0f)));
                    float _588 = _81_m0[8u].x + _386;
                    float4 _596 = _48.SampleLevel(_97, float2((_588 + _81_m0[8u].z) * _81_m0[9u].y, _81_m0[9u].y * _387), _587);
                    float4 _618 = _47.SampleLevel(_97, float3(((((_596.x * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _588) * _81_m0[9u].x, ((((_596.y * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _387) * _81_m0[9u].x, ((_81_m0[8u].y + _388) + (((_596.z * 2.0f) + (-1.0f)) * _81_m0[9u].z)) * _81_m0[9u].x), _587);
                    float _620 = _618.x;
                    float _623 = (_81_m0[10u].w == 0.0f) ? (1.0f - _620) : _620;
                    float _624 = _623 * _81_m0[12u].z;
                    float _630 = clamp((min(max(_81_m0[12u].y, _624), 1.0f) - _624) / (1.0f - _624), 0.0f, 1.0f);
                    float _631 = _630 * _81_m0[11u].x;
                    float _632 = _630 * _81_m0[11u].y;
                    float _633 = _630 * _81_m0[11u].z;
                    float _634 = _630 * _81_m0[12u].x;
                    float _641 = sqrt(((_631 * _631) + (_632 * _632)) + (_633 * _633)) + _634;
                    _643 = 1u;
                    _644 = _623;
                    _645 = _641;
                    _646 = _641 * _81_m0[11u].w;
                    _647 = _634;
                    _648 = _631;
                    _649 = _632;
                    _650 = _633;
                }
                else
                {
                    _643 = 0u;
                    _644 = 0.0f;
                    _645 = 0.0f;
                    _646 = 0.0f;
                    _647 = 0.0f;
                    _648 = 0.0f;
                    _649 = 0.0f;
                    _650 = 0.0f;
                }
                float frontier_phi_23_21_ladder;
                uint frontier_phi_23_21_ladder_1;
                float frontier_phi_23_21_ladder_2;
                float frontier_phi_23_21_ladder_3;
                float frontier_phi_23_21_ladder_4;
                float frontier_phi_23_21_ladder_5;
                float frontier_phi_23_21_ladder_6;
                float frontier_phi_23_21_ladder_7;
                if (_81_m0[15u].y > 0.0f)
                {
                    float _662;
                    if (_643 == 0u)
                    {
                        float _893 = clamp((_380 - _81_m0[10u].y) / (_81_m0[10u].z - _81_m0[10u].y), 0.0f, 1.0f);
                        float _899 = max(_81_m0[9u].w, ((_893 * _893) * _81_m0[10u].x) * (3.0f - (_893 * 2.0f)));
                        float _900 = _81_m0[8u].x + _386;
                        float4 _908 = _48.SampleLevel(_97, float2((_900 + _81_m0[8u].z) * _81_m0[9u].y, _81_m0[9u].y * _387), _899);
                        float4 _930 = _47.SampleLevel(_97, float3(((((_908.x * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _900) * _81_m0[9u].x, ((((_908.y * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _387) * _81_m0[9u].x, ((_81_m0[8u].y + _388) + (((_908.z * 2.0f) + (-1.0f)) * _81_m0[9u].z)) * _81_m0[9u].x), _899);
                        float _932 = _930.x;
                        float frontier_phi_32_31_ladder;
                        if (_81_m0[10u].w == 0.0f)
                        {
                            frontier_phi_32_31_ladder = 1.0f - _932;
                        }
                        else
                        {
                            frontier_phi_32_31_ladder = _932;
                        }
                        _662 = frontier_phi_32_31_ladder;
                    }
                    else
                    {
                        _662 = _644;
                    }
                    float _935 = _662 * _81_m0[15u].z;
                    float _941 = clamp((min(max(_81_m0[15u].y, _935), 1.0f) - _935) / (1.0f - _935), 0.0f, 1.0f);
                    float _942 = _941 * _81_m0[14u].x;
                    float _943 = _941 * _81_m0[14u].y;
                    float _944 = _941 * _81_m0[14u].z;
                    float _945 = _941 * _81_m0[15u].x;
                    float _952 = sqrt(((_942 * _942) + (_943 * _943)) + (_944 * _944)) + _945;
                    frontier_phi_23_21_ladder = _952 + _645;
                    frontier_phi_23_21_ladder_1 = 1u;
                    frontier_phi_23_21_ladder_2 = _662;
                    frontier_phi_23_21_ladder_3 = (_952 * _81_m0[14u].w) + _646;
                    frontier_phi_23_21_ladder_4 = _945 + _647;
                    frontier_phi_23_21_ladder_5 = _942 + _648;
                    frontier_phi_23_21_ladder_6 = _943 + _649;
                    frontier_phi_23_21_ladder_7 = _944 + _650;
                }
                else
                {
                    frontier_phi_23_21_ladder = _645;
                    frontier_phi_23_21_ladder_1 = _643;
                    frontier_phi_23_21_ladder_2 = _644;
                    frontier_phi_23_21_ladder_3 = _646;
                    frontier_phi_23_21_ladder_4 = _647;
                    frontier_phi_23_21_ladder_5 = _648;
                    frontier_phi_23_21_ladder_6 = _649;
                    frontier_phi_23_21_ladder_7 = _650;
                }
                _657 = frontier_phi_23_21_ladder_1;
                _659 = frontier_phi_23_21_ladder_2;
                _663 = frontier_phi_23_21_ladder;
                _667 = frontier_phi_23_21_ladder_3;
                _671 = frontier_phi_23_21_ladder_4;
                _675 = frontier_phi_23_21_ladder_5;
                _679 = frontier_phi_23_21_ladder_6;
                _683 = frontier_phi_23_21_ladder_7;
            }
            else
            {
                float4 _562 = _46.SampleLevel(_94, float2(clamp((_386 - _81_m0[17u].z) / _81_m0[18u].x, 0.0f, 1.0f), clamp((_388 - _81_m0[17u].w) / _81_m0[18u].y, 0.0f, 1.0f)), 0.0f);
                float _565 = _562.y;
                float frontier_phi_23_17_ladder;
                uint frontier_phi_23_17_ladder_1;
                float frontier_phi_23_17_ladder_2;
                float frontier_phi_23_17_ladder_3;
                float frontier_phi_23_17_ladder_4;
                float frontier_phi_23_17_ladder_5;
                float frontier_phi_23_17_ladder_6;
                float frontier_phi_23_17_ladder_7;
                if (_565 > 0.0f)
                {
                    float _655 = (_562.x * (_81_m0[18u].w - _81_m0[18u].z)) + _81_m0[18u].z;
                    float frontier_phi_23_17_ladder_22_ladder;
                    uint frontier_phi_23_17_ladder_22_ladder_1;
                    float frontier_phi_23_17_ladder_22_ladder_2;
                    float frontier_phi_23_17_ladder_22_ladder_3;
                    float frontier_phi_23_17_ladder_22_ladder_4;
                    float frontier_phi_23_17_ladder_22_ladder_5;
                    float frontier_phi_23_17_ladder_22_ladder_6;
                    float frontier_phi_23_17_ladder_22_ladder_7;
                    if (_387 < _655)
                    {
                        frontier_phi_23_17_ladder_22_ladder = 0.0f;
                        frontier_phi_23_17_ladder_22_ladder_1 = 0u;
                        frontier_phi_23_17_ladder_22_ladder_2 = 0.0f;
                        frontier_phi_23_17_ladder_22_ladder_3 = 0.0f;
                        frontier_phi_23_17_ladder_22_ladder_4 = 0.0f;
                        frontier_phi_23_17_ladder_22_ladder_5 = 0.0f;
                        frontier_phi_23_17_ladder_22_ladder_6 = 0.0f;
                        frontier_phi_23_17_ladder_22_ladder_7 = 0.0f;
                    }
                    else
                    {
                        float _733 = clamp(((_387 - _81_m0[12u].w) - _655) / ((-0.0f) - _81_m0[12u].w), 0.0f, 1.0f);
                        float _737 = (_733 * _733) * (3.0f - (_733 * 2.0f));
                        float _738 = _565 * _81_m0[12u].y;
                        uint _658;
                        float _661;
                        float _665;
                        float _669;
                        float _673;
                        float _677;
                        float _681;
                        float _685;
                        if ((_737 * _738) > 0.0f)
                        {
                            float _961 = clamp((_380 - _81_m0[10u].y) / (_81_m0[10u].z - _81_m0[10u].y), 0.0f, 1.0f);
                            float _967 = max(_81_m0[9u].w, ((_961 * _961) * _81_m0[10u].x) * (3.0f - (_961 * 2.0f)));
                            float _968 = _81_m0[8u].x + _386;
                            float4 _976 = _48.SampleLevel(_97, float2((_968 + _81_m0[8u].z) * _81_m0[9u].y, _81_m0[9u].y * _387), _967);
                            float4 _998 = _47.SampleLevel(_97, float3(((((_976.x * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _968) * _81_m0[9u].x, ((((_976.y * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _387) * _81_m0[9u].x, ((_81_m0[8u].y + _388) + (((_976.z * 2.0f) + (-1.0f)) * _81_m0[9u].z)) * _81_m0[9u].x), _967);
                            float _1000 = _998.x;
                            float _1003 = (_81_m0[10u].w == 0.0f) ? (1.0f - _1000) : _1000;
                            float _1004 = _1003 * _81_m0[12u].z;
                            float _1010 = clamp((min(max(exp2(log2(_737) * _81_m0[13u].y) * _738, _1004), 1.0f) - _1004) / (1.0f - _1004), 0.0f, 1.0f);
                            float _1011 = _1010 * _81_m0[11u].x;
                            float _1012 = _1010 * _81_m0[11u].y;
                            float _1013 = _1010 * _81_m0[11u].z;
                            float _1014 = _1010 * _81_m0[12u].x;
                            float _1021 = sqrt(((_1011 * _1011) + (_1012 * _1012)) + (_1013 * _1013)) + _1014;
                            _658 = 1u;
                            _661 = _1003;
                            _665 = _1021;
                            _669 = _1021 * _81_m0[11u].w;
                            _673 = _1014;
                            _677 = _1011;
                            _681 = _1012;
                            _685 = _1013;
                        }
                        else
                        {
                            _658 = 0u;
                            _661 = 0.0f;
                            _665 = 0.0f;
                            _669 = 0.0f;
                            _673 = 0.0f;
                            _677 = 0.0f;
                            _681 = 0.0f;
                            _685 = 0.0f;
                        }
                        float _1026 = clamp(((_387 - _81_m0[15u].w) - _655) / ((-0.0f) - _81_m0[15u].w), 0.0f, 1.0f);
                        float _1030 = (_1026 * _1026) * (3.0f - (_1026 * 2.0f));
                        float _1031 = _565 * _81_m0[15u].y;
                        float frontier_phi_23_17_ladder_22_ladder_34_ladder;
                        uint frontier_phi_23_17_ladder_22_ladder_34_ladder_1;
                        float frontier_phi_23_17_ladder_22_ladder_34_ladder_2;
                        float frontier_phi_23_17_ladder_22_ladder_34_ladder_3;
                        float frontier_phi_23_17_ladder_22_ladder_34_ladder_4;
                        float frontier_phi_23_17_ladder_22_ladder_34_ladder_5;
                        float frontier_phi_23_17_ladder_22_ladder_34_ladder_6;
                        float frontier_phi_23_17_ladder_22_ladder_34_ladder_7;
                        if ((_1030 * _1031) > 0.0f)
                        {
                            float _660;
                            if (_658 == 0u)
                            {
                                float _1258 = clamp((_380 - _81_m0[10u].y) / (_81_m0[10u].z - _81_m0[10u].y), 0.0f, 1.0f);
                                float _1264 = max(_81_m0[9u].w, ((_1258 * _1258) * _81_m0[10u].x) * (3.0f - (_1258 * 2.0f)));
                                float _1265 = _81_m0[8u].x + _386;
                                float4 _1273 = _48.SampleLevel(_97, float2((_1265 + _81_m0[8u].z) * _81_m0[9u].y, _81_m0[9u].y * _387), _1264);
                                float4 _1295 = _47.SampleLevel(_97, float3(((((_1273.x * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _1265) * _81_m0[9u].x, ((((_1273.y * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _387) * _81_m0[9u].x, ((_81_m0[8u].y + _388) + (((_1273.z * 2.0f) + (-1.0f)) * _81_m0[9u].z)) * _81_m0[9u].x), _1264);
                                float _1297 = _1295.x;
                                float frontier_phi_51_50_ladder;
                                if (_81_m0[10u].w == 0.0f)
                                {
                                    frontier_phi_51_50_ladder = 1.0f - _1297;
                                }
                                else
                                {
                                    frontier_phi_51_50_ladder = _1297;
                                }
                                _660 = frontier_phi_51_50_ladder;
                            }
                            else
                            {
                                _660 = _661;
                            }
                            float _1300 = _660 * _81_m0[15u].z;
                            float _1306 = clamp((min(max(exp2(log2(_1030) * _81_m0[16u].y) * _1031, _1300), 1.0f) - _1300) / (1.0f - _1300), 0.0f, 1.0f);
                            float _1307 = _1306 * _81_m0[14u].x;
                            float _1308 = _1306 * _81_m0[14u].y;
                            float _1309 = _1306 * _81_m0[14u].z;
                            float _1310 = _1306 * _81_m0[15u].x;
                            float _1317 = sqrt(((_1307 * _1307) + (_1308 * _1308)) + (_1309 * _1309)) + _1310;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder = _1317 + _665;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_1 = 1u;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_2 = _660;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_3 = (_1317 * _81_m0[14u].w) + _669;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_4 = _1310 + _673;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_5 = _1307 + _677;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_6 = _1308 + _681;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_7 = _1309 + _685;
                        }
                        else
                        {
                            frontier_phi_23_17_ladder_22_ladder_34_ladder = _665;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_1 = _658;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_2 = _661;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_3 = _669;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_4 = _673;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_5 = _677;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_6 = _681;
                            frontier_phi_23_17_ladder_22_ladder_34_ladder_7 = _685;
                        }
                        frontier_phi_23_17_ladder_22_ladder = frontier_phi_23_17_ladder_22_ladder_34_ladder;
                        frontier_phi_23_17_ladder_22_ladder_1 = frontier_phi_23_17_ladder_22_ladder_34_ladder_1;
                        frontier_phi_23_17_ladder_22_ladder_2 = frontier_phi_23_17_ladder_22_ladder_34_ladder_2;
                        frontier_phi_23_17_ladder_22_ladder_3 = frontier_phi_23_17_ladder_22_ladder_34_ladder_3;
                        frontier_phi_23_17_ladder_22_ladder_4 = frontier_phi_23_17_ladder_22_ladder_34_ladder_4;
                        frontier_phi_23_17_ladder_22_ladder_5 = frontier_phi_23_17_ladder_22_ladder_34_ladder_5;
                        frontier_phi_23_17_ladder_22_ladder_6 = frontier_phi_23_17_ladder_22_ladder_34_ladder_6;
                        frontier_phi_23_17_ladder_22_ladder_7 = frontier_phi_23_17_ladder_22_ladder_34_ladder_7;
                    }
                    frontier_phi_23_17_ladder = frontier_phi_23_17_ladder_22_ladder;
                    frontier_phi_23_17_ladder_1 = frontier_phi_23_17_ladder_22_ladder_1;
                    frontier_phi_23_17_ladder_2 = frontier_phi_23_17_ladder_22_ladder_2;
                    frontier_phi_23_17_ladder_3 = frontier_phi_23_17_ladder_22_ladder_3;
                    frontier_phi_23_17_ladder_4 = frontier_phi_23_17_ladder_22_ladder_4;
                    frontier_phi_23_17_ladder_5 = frontier_phi_23_17_ladder_22_ladder_5;
                    frontier_phi_23_17_ladder_6 = frontier_phi_23_17_ladder_22_ladder_6;
                    frontier_phi_23_17_ladder_7 = frontier_phi_23_17_ladder_22_ladder_7;
                }
                else
                {
                    frontier_phi_23_17_ladder = 0.0f;
                    frontier_phi_23_17_ladder_1 = 0u;
                    frontier_phi_23_17_ladder_2 = 0.0f;
                    frontier_phi_23_17_ladder_3 = 0.0f;
                    frontier_phi_23_17_ladder_4 = 0.0f;
                    frontier_phi_23_17_ladder_5 = 0.0f;
                    frontier_phi_23_17_ladder_6 = 0.0f;
                    frontier_phi_23_17_ladder_7 = 0.0f;
                }
                _657 = frontier_phi_23_17_ladder_1;
                _659 = frontier_phi_23_17_ladder_2;
                _663 = frontier_phi_23_17_ladder;
                _667 = frontier_phi_23_17_ladder_3;
                _671 = frontier_phi_23_17_ladder_4;
                _675 = frontier_phi_23_17_ladder_5;
                _679 = frontier_phi_23_17_ladder_6;
                _683 = frontier_phi_23_17_ladder_7;
            }
            uint _696 = uint(int(_91_m0[12u].x * _437));
            uint _697 = uint(int(_91_m0[12u].y * _440));
            uint _698 = uint(int(max(_91_m0[4u].w - (log2((_91_m0[12u].z * _441) + _91_m0[12u].w) * float(_529.z)), 0.0f)));
            uint _703 = _49.Load(int4(uint3(_696, _697, _698), 0u)).x & 255u;
            float _741;
            float _743;
            float _745;
            float _747;
            float _749;
            float _751;
            float _753;
            float _755;
            float _757;
            float _759;
            float _761;
            float _763;
            float _765;
            if (_703 == 0u)
            {
                _741 = 0.0f;
                _743 = 0.0f;
                _745 = 0.0f;
                _747 = 0.0f;
                _749 = 0.0f;
                _751 = 0.0f;
                _753 = 0.0f;
                _755 = 0.0f;
                _757 = 0.0f;
                _759 = 0.0f;
                _761 = 0.0f;
                _763 = 0.0f;
                _765 = 0.0f;
            }
            else
            {
                float _1088;
                if (_657 == 0u)
                {
                    float _1047 = clamp((_380 - _81_m0[10u].y) / (_81_m0[10u].z - _81_m0[10u].y), 0.0f, 1.0f);
                    float _1053 = max(_81_m0[9u].w, ((_1047 * _1047) * _81_m0[10u].x) * (3.0f - (_1047 * 2.0f)));
                    float _1054 = _81_m0[8u].x + _386;
                    float4 _1062 = _48.SampleLevel(_97, float2((_1054 + _81_m0[8u].z) * _81_m0[9u].y, _81_m0[9u].y * _387), _1053);
                    float4 _1084 = _47.SampleLevel(_97, float3(((((_1062.x * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _1054) * _81_m0[9u].x, ((((_1062.y * 2.0f) + (-1.0f)) * _81_m0[9u].z) + _387) * _81_m0[9u].x, ((_81_m0[8u].y + _388) + (((_1062.z * 2.0f) + (-1.0f)) * _81_m0[9u].z)) * _81_m0[9u].x), _1053);
                    float _1086 = _1084.x;
                    float frontier_phi_38_37_ladder;
                    if (_81_m0[10u].w == 0.0f)
                    {
                        frontier_phi_38_37_ladder = 1.0f - _1086;
                    }
                    else
                    {
                        frontier_phi_38_37_ladder = _1086;
                    }
                    _1088 = frontier_phi_38_37_ladder;
                }
                else
                {
                    _1088 = _659;
                }
                uint _1098 = (((_697 << (asuint(_91_m0[15u]).w & 31u)) + _696) + (_698 << (_529.x & 31u))) << (_529.y & 31u);
                uint _1099 = _703 + _1098;
                uint _1100 = _1098 + 1u;
                float frontier_phi_28_38_ladder;
                float frontier_phi_28_38_ladder_1;
                float frontier_phi_28_38_ladder_2;
                float frontier_phi_28_38_ladder_3;
                float frontier_phi_28_38_ladder_4;
                float frontier_phi_28_38_ladder_5;
                float frontier_phi_28_38_ladder_6;
                float frontier_phi_28_38_ladder_7;
                float frontier_phi_28_38_ladder_8;
                float frontier_phi_28_38_ladder_9;
                float frontier_phi_28_38_ladder_10;
                float frontier_phi_28_38_ladder_11;
                float frontier_phi_28_38_ladder_12;
                if (_1100 > _1099)
                {
                    frontier_phi_28_38_ladder = 0.0f;
                    frontier_phi_28_38_ladder_1 = 0.0f;
                    frontier_phi_28_38_ladder_2 = 0.0f;
                    frontier_phi_28_38_ladder_3 = 0.0f;
                    frontier_phi_28_38_ladder_4 = 0.0f;
                    frontier_phi_28_38_ladder_5 = 0.0f;
                    frontier_phi_28_38_ladder_6 = 0.0f;
                    frontier_phi_28_38_ladder_7 = 0.0f;
                    frontier_phi_28_38_ladder_8 = 0.0f;
                    frontier_phi_28_38_ladder_9 = 0.0f;
                    frontier_phi_28_38_ladder_10 = 0.0f;
                    frontier_phi_28_38_ladder_11 = 0.0f;
                    frontier_phi_28_38_ladder_12 = 0.0f;
                }
                else
                {
                    uint4 _1250 = _50.Load(_1098);
                    uint _1251 = _1250.x;
                    float _742;
                    float _744;
                    float _746;
                    float _748;
                    float _750;
                    float _752;
                    float _754;
                    float _756;
                    float _758;
                    float _760;
                    float _762;
                    float _764;
                    float _766;
                    float _1569 = 0.0f;
                    float _1570 = 0.0f;
                    float _1571 = 0.0f;
                    float _1572 = 0.0f;
                    float _1573 = 0.0f;
                    float _1574 = 0.0f;
                    float _1575 = 0.0f;
                    float _1576 = 0.0f;
                    float _1577 = 0.0f;
                    float _1578 = 0.0f;
                    float _1579 = 0.0f;
                    float _1580 = 0.0f;
                    float _1581 = 0.0f;
                    uint _1582 = _1100;
                    uint _1584 = _1251;
                    uint _1583;
                    uint _1588;
                    float _1668;
                    float _1671;
                    float _1674;
                    float _1677;
                    float _1678;
                    uint _1679;
                    uint _1681;
                    float _1683;
                    float _1686;
                    uint _1687;
                    float _1695;
                    float _1742;
                    float _1743;
                    float _1744;
                    bool _1746;
                    for (;;)
                    {
                        _1583 = _1582 + 1u;
                        _1588 = _50.Load(_1582).x;
                        uint _1590 = _1584 * 8u;
                        uint4 _1602 = uint4(_51.Load(_1590).x, _51.Load(_1590 + 1u).x, _51.Load(_1590 + 2u).x, _51.Load(_1590 + 3u).x);
                        uint _1603 = _1602.x;
                        uint _1604 = _1602.y;
                        uint _1605 = _1602.z;
                        uint _1606 = _1602.w;
                        uint _1608 = (_1584 * 8u) + 4u;
                        uint4 _1620 = uint4(_51.Load(_1608).x, _51.Load(_1608 + 1u).x, _51.Load(_1608 + 2u).x, _51.Load(_1608 + 3u).x);
                        uint _1621 = _1620.x;
                        uint _1622 = _1620.y;
                        uint _1623 = _1620.z;
                        uint _1624 = _1620.w;
                        float2 _1652 = spvUnpackHalf2x16(_1606 >> 16u);
                        float _1653 = _1652.x;
                        float2 _1655 = spvUnpackHalf2x16(_1606 & 65535u);
                        float _1656 = _1655.x;
                        float2 _1658 = spvUnpackHalf2x16(_1621 >> 16u);
                        float _1659 = _1658.x;
                        float _1665 = sqrt(clamp(1.0f - dot(float3(_1653, _1656, _1659), float3(_1653, _1656, _1659)), 0.0f, 1.0f));
                        _1668 = spvUnpackHalf2x16(_1622 >> 16u).x;
                        _1671 = spvUnpackHalf2x16(_1622 & 65535u).x;
                        _1674 = spvUnpackHalf2x16(_1623 >> 16u).x;
                        _1677 = spvUnpackHalf2x16(_1623 & 65535u).x;
                        _1678 = float(_1624 >> 24u);
                        _1679 = (_1624 >> 16u) & 127u;
                        _1681 = _1679 + 4294967295u;
                        _1683 = float((_1624 >> 8u) & 255u) * 0.0039215688593685626983642578125f;
                        _1686 = float(_1624 & 255u) * 0.0039215688593685626983642578125f;
                        _1687 = _1624 & 8388608u;
                        _1695 = min(max((float((_1621 >> 8u) & 255u) * 0.007843137718737125396728515625f) + (-1.0f), -0.9900000095367431640625f), 0.9900000095367431640625f);
                        float _1698 = (_386 - _91_m0[13u].x) - spvUnpackHalf2x16(_1603 >> 16u).x;
                        float _1700 = (_387 - _91_m0[13u].y) - spvUnpackHalf2x16(_1603 & 65535u).x;
                        float _1702 = (_388 - _91_m0[13u].z) - spvUnpackHalf2x16(_1604 >> 16u).x;
                        float _1703 = (-0.0f) - _1653;
                        float _1704 = (-0.0f) - _1656;
                        float _1705 = (-0.0f) - _1659;
                        float _1718 = (_1665 * _1698) + ((_1702 * _1704) - (_1700 * _1705));
                        float _1719 = (_1665 * _1700) + ((_1698 * _1705) - (_1702 * _1703));
                        float _1720 = (_1665 * _1702) + ((_1700 * _1703) - (_1698 * _1704));
                        _1742 = ((((((_1720 * _1704) - (_1719 * _1705)) * 2.0f) + _1698) / spvUnpackHalf2x16(_1604 & 65535u).x) + 1.0f) * 0.5f;
                        _1743 = ((((((_1718 * _1705) - (_1720 * _1703)) * 2.0f) + _1700) / spvUnpackHalf2x16(_1605 >> 16u).x) + 1.0f) * 0.5f;
                        _1744 = ((((((_1719 * _1703) - (_1718 * _1704)) * 2.0f) + _1702) / spvUnpackHalf2x16(_1605 & 65535u).x) + 1.0f) * 0.5f;
                        _1746 = _1742 == clamp(_1742, 0.0f, 1.0f);
                        float frontier_phi_71_pred;
                        float frontier_phi_71_pred_1;
                        float frontier_phi_71_pred_2;
                        float frontier_phi_71_pred_3;
                        float frontier_phi_71_pred_4;
                        float frontier_phi_71_pred_5;
                        float frontier_phi_71_pred_6;
                        float frontier_phi_71_pred_7;
                        float frontier_phi_71_pred_8;
                        float frontier_phi_71_pred_9;
                        float frontier_phi_71_pred_10;
                        float frontier_phi_71_pred_11;
                        float frontier_phi_71_pred_12;
                        if (_1746)
                        {
                            float frontier_phi_71_pred_70_ladder;
                            float frontier_phi_71_pred_70_ladder_1;
                            float frontier_phi_71_pred_70_ladder_2;
                            float frontier_phi_71_pred_70_ladder_3;
                            float frontier_phi_71_pred_70_ladder_4;
                            float frontier_phi_71_pred_70_ladder_5;
                            float frontier_phi_71_pred_70_ladder_6;
                            float frontier_phi_71_pred_70_ladder_7;
                            float frontier_phi_71_pred_70_ladder_8;
                            float frontier_phi_71_pred_70_ladder_9;
                            float frontier_phi_71_pred_70_ladder_10;
                            float frontier_phi_71_pred_70_ladder_11;
                            float frontier_phi_71_pred_70_ladder_12;
                            if (_1743 == clamp(_1743, 0.0f, 1.0f))
                            {
                                float frontier_phi_71_pred_70_ladder_76_ladder;
                                float frontier_phi_71_pred_70_ladder_76_ladder_1;
                                float frontier_phi_71_pred_70_ladder_76_ladder_2;
                                float frontier_phi_71_pred_70_ladder_76_ladder_3;
                                float frontier_phi_71_pred_70_ladder_76_ladder_4;
                                float frontier_phi_71_pred_70_ladder_76_ladder_5;
                                float frontier_phi_71_pred_70_ladder_76_ladder_6;
                                float frontier_phi_71_pred_70_ladder_76_ladder_7;
                                float frontier_phi_71_pred_70_ladder_76_ladder_8;
                                float frontier_phi_71_pred_70_ladder_76_ladder_9;
                                float frontier_phi_71_pred_70_ladder_76_ladder_10;
                                float frontier_phi_71_pred_70_ladder_76_ladder_11;
                                float frontier_phi_71_pred_70_ladder_76_ladder_12;
                                if (_1744 == clamp(_1744, 0.0f, 1.0f))
                                {
                                    float _2543;
                                    float _2545;
                                    if (_1679 == 0u)
                                    {
                                        _2543 = _1686;
                                        _2545 = _1683;
                                    }
                                    else
                                    {
                                        float4 _2573 = _9[_1681 + 0u].SampleLevel(_94, float2(_1742, _1744), 0.0f);
                                        float _2575 = _2573.x;
                                        float _2576 = _2573.y;
                                        float _2577 = _2573.z;
                                        float frontier_phi_95_96_ladder;
                                        float frontier_phi_95_96_ladder_1;
                                        if ((_1743 >= _2575) && (_1743 < _2576))
                                        {
                                            float _2843 = clamp((_1743 - _2575) / (_2576 - _2575), 0.0f, 1.0f);
                                            float _2850 = (((_2843 * _2843) * (_2573.w - _2577)) * (3.0f - (_2843 * 2.0f))) + _2577;
                                            frontier_phi_95_96_ladder = _2850 * _1683;
                                            frontier_phi_95_96_ladder_1 = _2850 * _1686;
                                        }
                                        else
                                        {
                                            frontier_phi_95_96_ladder = 0.0f;
                                            frontier_phi_95_96_ladder_1 = 0.0f;
                                        }
                                        _2543 = frontier_phi_95_96_ladder_1;
                                        _2545 = frontier_phi_95_96_ladder;
                                    }
                                    float _2548 = (_1088 * 0.0039215688593685626983642578125f) * _1678;
                                    float _2554 = clamp((min(max(_2545, _2548), 1.0f) - _2548) / (1.0f - _2548), 0.0f, 1.0f);
                                    float _2556 = _2554 * _1668;
                                    float _2557 = _2554 * _1671;
                                    float _2558 = _2554 * _1674;
                                    float _2559 = _2554 * _1677;
                                    float _2566 = sqrt(((_2556 * _2556) + (_2557 * _2557)) + (_2558 * _2558)) + _2559;
                                    float _2567 = _2566 * _1695;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_1;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_2;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_3;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_4;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_5;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_6;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_7;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_8;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_9;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_10;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_11;
                                    float frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_12;
                                    if (_1687 == 0u)
                                    {
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder = _1577;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_1 = _1569;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_2 = _2566 + _1570;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_3 = _1571;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_4 = _2567 + _1572;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_5 = _2559 + _1573;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_6 = _2556 + _1574;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_7 = _2557 + _1575;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_8 = _2558 + _1576;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_9 = _1578;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_10 = _1579;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_11 = _1580;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_12 = _1581;
                                    }
                                    else
                                    {
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder = _2567 + _1577;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_1 = (_2543 * (1.0f - _1569)) + _1569;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_2 = _1570;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_3 = _2566 + _1571;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_4 = _1572;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_5 = _1573;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_6 = _1574;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_7 = _1575;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_8 = _1576;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_9 = _2559 + _1578;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_10 = _2556 + _1579;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_11 = _2557 + _1580;
                                        frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_12 = _2558 + _1581;
                                    }
                                    frontier_phi_71_pred_70_ladder_76_ladder = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder;
                                    frontier_phi_71_pred_70_ladder_76_ladder_1 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_1;
                                    frontier_phi_71_pred_70_ladder_76_ladder_2 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_2;
                                    frontier_phi_71_pred_70_ladder_76_ladder_3 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_3;
                                    frontier_phi_71_pred_70_ladder_76_ladder_4 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_4;
                                    frontier_phi_71_pred_70_ladder_76_ladder_5 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_5;
                                    frontier_phi_71_pred_70_ladder_76_ladder_6 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_6;
                                    frontier_phi_71_pred_70_ladder_76_ladder_7 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_7;
                                    frontier_phi_71_pred_70_ladder_76_ladder_8 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_8;
                                    frontier_phi_71_pred_70_ladder_76_ladder_9 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_9;
                                    frontier_phi_71_pred_70_ladder_76_ladder_10 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_10;
                                    frontier_phi_71_pred_70_ladder_76_ladder_11 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_11;
                                    frontier_phi_71_pred_70_ladder_76_ladder_12 = frontier_phi_71_pred_70_ladder_76_ladder_95_ladder_12;
                                }
                                else
                                {
                                    frontier_phi_71_pred_70_ladder_76_ladder = _1577;
                                    frontier_phi_71_pred_70_ladder_76_ladder_1 = _1569;
                                    frontier_phi_71_pred_70_ladder_76_ladder_2 = _1570;
                                    frontier_phi_71_pred_70_ladder_76_ladder_3 = _1571;
                                    frontier_phi_71_pred_70_ladder_76_ladder_4 = _1572;
                                    frontier_phi_71_pred_70_ladder_76_ladder_5 = _1573;
                                    frontier_phi_71_pred_70_ladder_76_ladder_6 = _1574;
                                    frontier_phi_71_pred_70_ladder_76_ladder_7 = _1575;
                                    frontier_phi_71_pred_70_ladder_76_ladder_8 = _1576;
                                    frontier_phi_71_pred_70_ladder_76_ladder_9 = _1578;
                                    frontier_phi_71_pred_70_ladder_76_ladder_10 = _1579;
                                    frontier_phi_71_pred_70_ladder_76_ladder_11 = _1580;
                                    frontier_phi_71_pred_70_ladder_76_ladder_12 = _1581;
                                }
                                frontier_phi_71_pred_70_ladder = frontier_phi_71_pred_70_ladder_76_ladder;
                                frontier_phi_71_pred_70_ladder_1 = frontier_phi_71_pred_70_ladder_76_ladder_1;
                                frontier_phi_71_pred_70_ladder_2 = frontier_phi_71_pred_70_ladder_76_ladder_2;
                                frontier_phi_71_pred_70_ladder_3 = frontier_phi_71_pred_70_ladder_76_ladder_3;
                                frontier_phi_71_pred_70_ladder_4 = frontier_phi_71_pred_70_ladder_76_ladder_4;
                                frontier_phi_71_pred_70_ladder_5 = frontier_phi_71_pred_70_ladder_76_ladder_5;
                                frontier_phi_71_pred_70_ladder_6 = frontier_phi_71_pred_70_ladder_76_ladder_6;
                                frontier_phi_71_pred_70_ladder_7 = frontier_phi_71_pred_70_ladder_76_ladder_7;
                                frontier_phi_71_pred_70_ladder_8 = frontier_phi_71_pred_70_ladder_76_ladder_8;
                                frontier_phi_71_pred_70_ladder_9 = frontier_phi_71_pred_70_ladder_76_ladder_9;
                                frontier_phi_71_pred_70_ladder_10 = frontier_phi_71_pred_70_ladder_76_ladder_10;
                                frontier_phi_71_pred_70_ladder_11 = frontier_phi_71_pred_70_ladder_76_ladder_11;
                                frontier_phi_71_pred_70_ladder_12 = frontier_phi_71_pred_70_ladder_76_ladder_12;
                            }
                            else
                            {
                                frontier_phi_71_pred_70_ladder = _1577;
                                frontier_phi_71_pred_70_ladder_1 = _1569;
                                frontier_phi_71_pred_70_ladder_2 = _1570;
                                frontier_phi_71_pred_70_ladder_3 = _1571;
                                frontier_phi_71_pred_70_ladder_4 = _1572;
                                frontier_phi_71_pred_70_ladder_5 = _1573;
                                frontier_phi_71_pred_70_ladder_6 = _1574;
                                frontier_phi_71_pred_70_ladder_7 = _1575;
                                frontier_phi_71_pred_70_ladder_8 = _1576;
                                frontier_phi_71_pred_70_ladder_9 = _1578;
                                frontier_phi_71_pred_70_ladder_10 = _1579;
                                frontier_phi_71_pred_70_ladder_11 = _1580;
                                frontier_phi_71_pred_70_ladder_12 = _1581;
                            }
                            frontier_phi_71_pred = frontier_phi_71_pred_70_ladder;
                            frontier_phi_71_pred_1 = frontier_phi_71_pred_70_ladder_1;
                            frontier_phi_71_pred_2 = frontier_phi_71_pred_70_ladder_2;
                            frontier_phi_71_pred_3 = frontier_phi_71_pred_70_ladder_3;
                            frontier_phi_71_pred_4 = frontier_phi_71_pred_70_ladder_4;
                            frontier_phi_71_pred_5 = frontier_phi_71_pred_70_ladder_5;
                            frontier_phi_71_pred_6 = frontier_phi_71_pred_70_ladder_6;
                            frontier_phi_71_pred_7 = frontier_phi_71_pred_70_ladder_7;
                            frontier_phi_71_pred_8 = frontier_phi_71_pred_70_ladder_8;
                            frontier_phi_71_pred_9 = frontier_phi_71_pred_70_ladder_9;
                            frontier_phi_71_pred_10 = frontier_phi_71_pred_70_ladder_10;
                            frontier_phi_71_pred_11 = frontier_phi_71_pred_70_ladder_11;
                            frontier_phi_71_pred_12 = frontier_phi_71_pred_70_ladder_12;
                        }
                        else
                        {
                            frontier_phi_71_pred = _1577;
                            frontier_phi_71_pred_1 = _1569;
                            frontier_phi_71_pred_2 = _1570;
                            frontier_phi_71_pred_3 = _1571;
                            frontier_phi_71_pred_4 = _1572;
                            frontier_phi_71_pred_5 = _1573;
                            frontier_phi_71_pred_6 = _1574;
                            frontier_phi_71_pred_7 = _1575;
                            frontier_phi_71_pred_8 = _1576;
                            frontier_phi_71_pred_9 = _1578;
                            frontier_phi_71_pred_10 = _1579;
                            frontier_phi_71_pred_11 = _1580;
                            frontier_phi_71_pred_12 = _1581;
                        }
                        _758 = frontier_phi_71_pred;
                        _742 = frontier_phi_71_pred_1;
                        _744 = frontier_phi_71_pred_2;
                        _746 = frontier_phi_71_pred_3;
                        _748 = frontier_phi_71_pred_4;
                        _750 = frontier_phi_71_pred_5;
                        _752 = frontier_phi_71_pred_6;
                        _754 = frontier_phi_71_pred_7;
                        _756 = frontier_phi_71_pred_8;
                        _760 = frontier_phi_71_pred_9;
                        _762 = frontier_phi_71_pred_10;
                        _764 = frontier_phi_71_pred_11;
                        _766 = frontier_phi_71_pred_12;
                        if (_1583 > _1099)
                        {
                            break;
                        }
                        else
                        {
                            _1569 = _742;
                            _1570 = _744;
                            _1571 = _746;
                            _1572 = _748;
                            _1573 = _750;
                            _1574 = _752;
                            _1575 = _754;
                            _1576 = _756;
                            _1577 = _758;
                            _1578 = _760;
                            _1579 = _762;
                            _1580 = _764;
                            _1581 = _766;
                            _1582 = _1583;
                            _1584 = _1588;
                            continue;
                        }
                    }
                    frontier_phi_28_38_ladder = _748;
                    frontier_phi_28_38_ladder_1 = _744;
                    frontier_phi_28_38_ladder_2 = _746;
                    frontier_phi_28_38_ladder_3 = _742;
                    frontier_phi_28_38_ladder_4 = _750;
                    frontier_phi_28_38_ladder_5 = _752;
                    frontier_phi_28_38_ladder_6 = _754;
                    frontier_phi_28_38_ladder_7 = _758;
                    frontier_phi_28_38_ladder_8 = _756;
                    frontier_phi_28_38_ladder_9 = _766;
                    frontier_phi_28_38_ladder_10 = _764;
                    frontier_phi_28_38_ladder_11 = _762;
                    frontier_phi_28_38_ladder_12 = _760;
                }
                _741 = frontier_phi_28_38_ladder_3;
                _743 = frontier_phi_28_38_ladder_1;
                _745 = frontier_phi_28_38_ladder_2;
                _747 = frontier_phi_28_38_ladder;
                _749 = frontier_phi_28_38_ladder_4;
                _751 = frontier_phi_28_38_ladder_5;
                _753 = frontier_phi_28_38_ladder_6;
                _755 = frontier_phi_28_38_ladder_8;
                _757 = frontier_phi_28_38_ladder_7;
                _759 = frontier_phi_28_38_ladder_12;
                _761 = frontier_phi_28_38_ladder_11;
                _763 = frontier_phi_28_38_ladder_10;
                _765 = frontier_phi_28_38_ladder_9;
            }
            float _769 = (_663 > 0.0f) ? (_667 / _663) : _667;
            float _782 = ((_761 - _675) * _741) + _675;
            float _783 = ((_763 - _679) * _741) + _679;
            float _784 = ((_765 - _683) * _741) + _683;
            float _787 = ((_759 - _671) * _741) + _671;
            float _797 = sqrt(((_783 * _783) + (_782 * _782)) + (_784 * _784)) + _787;
            float _802 = 1.0f - clamp(_797 * float(asuint(_81_m0[20u]).z), 0.0f, 1.0f);
            float _803 = _802 * _751;
            float _804 = _802 * _753;
            float _805 = _802 * _755;
            float _806 = _802 * _749;
            float _813 = _806 + sqrt(((_803 * _803) + (_804 * _804)) + (_805 * _805));
            float _815 = _803 + _782;
            float _816 = _804 + _783;
            float _817 = _805 + _784;
            float _818 = _806 + _787;
            float _819 = (_813 * ((_743 > 0.0f) ? (_747 / _743) : _747)) + ((((((_745 > 0.0f) ? (_757 / _745) : _757) - _769) * _741) + _769) * _797);
            float _820 = _813 + _797;
            float _823 = (_820 > 0.0f) ? (_819 / _820) : _819;
            uint4 _826 = asuint(_81_m0[5u]);
            uint _827 = _826.w;
            uint4 _830 = asuint(_81_m0[6u]);
            uint _832 = _830.y;
            float _850 = ((_81_m0[20u].x - _81_m0[19u].w) * clamp((_380 - _81_m0[19u].y) / (_81_m0[19u].z - _81_m0[19u].y), 0.0f, 1.0f)) + _81_m0[19u].w;
            uint4 _861 = asuint(_62_m0[69u]);
            uint _867 = uint(min(int(_861.z + 4294967295u), int(_830.x)));
            uint _868 = uint(min(int(_861.y + 4294967295u), int(_830.z)));
            float _875 = (clamp(_250 / (_81_m0[7u].y + 9.9999997473787516355514526367188e-06f), 0.0f, 1.0f) * (_81_m0[7u].x - _81_m0[6u].w)) + _81_m0[6u].w;
            float _1036;
            float _1038;
            if (_62_m0[30u].x > 0.0f)
            {
                float _1214;
                if (_861.x == 1u)
                {
                    float _1121 = _370 * _67_m0[160u].y;
                    float _1123 = _374 * _67_m0[160u].y;
                    float _1132 = (_67_m0[160u].w + _67_m0[160u].x) + sqrt(_67_m0[160u].z - dot(float3(_1121, _67_m0[160u].x, _1123), float3(_1121, _67_m0[160u].x, _1123)));
                    float _1136 = rsqrt(dot(float3(_1121, _1132, _1123), float3(_1121, _1132, _1123)));
                    float _1138 = _1132 * _1136;
                    float4 _1144 = _25.SampleLevel(_97, float3(_1136 * _1121, _1138, _1136 * _1123), 0.0f);
                    float _1146 = _1144.x;
                    float _1154 = ((_372 - _62_m0[8u].y) + _67_m0[164u].w) / _67_m0[164u].y;
                    float _1213 = (((1.0f - _1146) + ((((1.0f - clamp(exp2(log2(clamp(1.0f - dot(float4(_22.SampleLevel(_97, float2((((_67_m0[164u].x * _1154) + (_62_m0[8u].x + _370)) * _67_m0[161u].x) + _67_m0[161u].z, (((_67_m0[164u].z * _1154) + (_62_m0[8u].z + _374)) * _67_m0[161u].x) + _67_m0[161u].w), 0.0f)), float4(_67_m0[163u])), 0.0f, 1.0f)) * _67_m0[162u].z), 0.0f, 1.0f)) * _67_m0[162u].w) + (-1.0f)) * _67_m0[161u].y)) * clamp((_1138 - _67_m0[162u].x) * _67_m0[162u].y, 0.0f, 1.0f)) + _1146;
                    _1214 = _1213;
                }
                else
                {
                    _1214 = 1.0f;
                }
                float _1319;
                if (int(_868) < int(0u))
                {
                    _1319 = 1.0f;
                }
                else
                {
                    float4 _1348 = _12[584u].SampleLevel(_94, float2(((_62_m0[8u].x + _370) - _62_m0[111u].x) * _62_m0[111u].z, 1.0f - (((_62_m0[8u].z + _374) - _62_m0[111u].y) * _62_m0[111u].w)), 0.0f);
                    float _1322 = 1.0f - (clamp(((((_372 - _62_m0[8u].y) + _62_m0[112u].x) - _62_m0[112u].w) + (_62_m0[112u].y * _1348.x)) / _62_m0[112u].z, 0.0f, 1.0f) * clamp(_1348.y, 0.0f, 1.0f));
                    float frontier_phi_52_53_ladder;
                    if (int(asuint(_62_m0[69u]).y) > int(0u))
                    {
                        float frontier_phi_89;
                        uint _1883 = 0u;
                        bool _1887;
                        for (;;)
                        {
                            _1887 = (int(_1883) >= int(_832)) && (int(_1883) <= int(_868));
                            if (_1887)
                            {
                                uint _1966 = _1883 * 36u;
                                float3 _1977 = asfloat(uint3(_30.Load(_1966).x, _30.Load(_1966 + 1u).x, _30.Load(_1966 + 2u).x));
                                uint _1981 = (_1883 * 36u) + 4u;
                                float3 _1991 = asfloat(uint3(_30.Load(_1981).x, _30.Load(_1981 + 1u).x, _30.Load(_1981 + 2u).x));
                                uint _1995 = (_1883 * 36u) + 8u;
                                float3 _2005 = asfloat(uint3(_30.Load(_1995).x, _30.Load(_1995 + 1u).x, _30.Load(_1995 + 2u).x));
                                uint _2009 = (_1883 * 36u) + 12u;
                                float3 _2019 = asfloat(uint3(_30.Load(_2009).x, _30.Load(_2009 + 1u).x, _30.Load(_2009 + 2u).x));
                                uint _2023 = (_1883 * 36u) + 20u;
                                float3 _2033 = asfloat(uint3(_30.Load(_2023).x, _30.Load(_2023 + 1u).x, _30.Load(_2023 + 2u).x));
                                uint _2038 = (_1883 * 36u) + 24u;
                                float3 _2048 = asfloat(uint3(_30.Load(_2038).x, _30.Load(_2038 + 1u).x, _30.Load(_2038 + 2u).x));
                                float _2072 = (((_370 - _2033.x) - _2048.x) + _62_m0[23u].x) + _62_m0[24u].x;
                                float _2076 = (((_371 - _2033.y) - _2048.y) + _62_m0[23u].y) + _62_m0[24u].y;
                                float _2080 = (((_374 - _2033.z) - _2048.z) + _62_m0[23u].z) + _62_m0[24u].z;
                                float _2084 = mad(_2005.x, _2080, mad(_1991.x, _2076, _2072 * _1977.x)) + _2019.x;
                                float _2088 = mad(_2005.y, _2080, mad(_1991.y, _2076, _2072 * _1977.y)) + _2019.y;
                                if ((clamp(_2084, 0.0f, 1.0f) == _2084) && (clamp(_2088, 0.0f, 1.0f) == _2088))
                                {
                                    float4 _2304 = _12[NonUniformResourceIndex((_1883 + 40u) + 0u)].SampleCmpLevelZero(_99, float2(_2084, _2088), clamp(mad(_2005.z, _2080, mad(_1991.z, _2076, _2072 * _1977.z)) + _2019.z, 0.0f, 1.0f) - (asfloat(uint3(_2274, _2275, _30.Load(((_1883 * 36u) + 16u) + 2u).x)).z * _875)).xxxx;
                                    float _1320 = _2304.x;
                                    if (!(_1883 == (asuint(_62_m0[69u]).y + 4294967295u)))
                                    {
                                        frontier_phi_89 = _1320;
                                        break;
                                    }
                                    frontier_phi_89 = (clamp(min(min(_2084, _2088), min(1.0f - _2084, 1.0f - _2088)) / asfloat(_30.Load((_1883 * 36u) + 32u).x), 0.0f, 1.0f) * (_1320 + (-1.0f))) + 1.0f;
                                    break;
                                }
                            }
                            uint _1884 = _1883 + 1u;
                            if (int(_1884) < int(asuint(_62_m0[69u]).y))
                            {
                                _1883 = _1884;
                                continue;
                            }
                            else
                            {
                                frontier_phi_89 = _1322;
                                break;
                            }
                        }
                        frontier_phi_52_53_ladder = frontier_phi_89;
                    }
                    else
                    {
                        frontier_phi_52_53_ladder = _1322;
                    }
                    _1319 = frontier_phi_52_53_ladder;
                }
                float _1747;
                if (int(_867) < int(0u))
                {
                    _1747 = 1.0f;
                }
                else
                {
                    float frontier_phi_60_61_ladder;
                    if (int(asuint(_62_m0[69u]).z) > int(0u))
                    {
                        float frontier_phi_98;
                        uint _1960 = 0u;
                        bool _1964;
                        for (;;)
                        {
                            _1964 = (int(_1960) >= int(_827)) && (int(_1960) <= int(_867));
                            if (_1964)
                            {
                                uint _2147 = _1960 * 36u;
                                float3 _2157 = asfloat(uint3(_29.Load(_2147).x, _29.Load(_2147 + 1u).x, _29.Load(_2147 + 2u).x));
                                uint _2161 = (_1960 * 36u) + 4u;
                                float3 _2171 = asfloat(uint3(_29.Load(_2161).x, _29.Load(_2161 + 1u).x, _29.Load(_2161 + 2u).x));
                                uint _2175 = (_1960 * 36u) + 8u;
                                float3 _2185 = asfloat(uint3(_29.Load(_2175).x, _29.Load(_2175 + 1u).x, _29.Load(_2175 + 2u).x));
                                uint _2189 = (_1960 * 36u) + 12u;
                                float3 _2199 = asfloat(uint3(_29.Load(_2189).x, _29.Load(_2189 + 1u).x, _29.Load(_2189 + 2u).x));
                                uint _2203 = (_1960 * 36u) + 20u;
                                float3 _2213 = asfloat(uint3(_29.Load(_2203).x, _29.Load(_2203 + 1u).x, _29.Load(_2203 + 2u).x));
                                uint _2218 = (_1960 * 36u) + 24u;
                                float3 _2228 = asfloat(uint3(_29.Load(_2218).x, _29.Load(_2218 + 1u).x, _29.Load(_2218 + 2u).x));
                                float _2245 = (((_370 - _2213.x) - _2228.x) + _62_m0[23u].x) + _62_m0[24u].x;
                                float _2249 = (((_371 - _2213.y) - _2228.y) + _62_m0[23u].y) + _62_m0[24u].y;
                                float _2253 = (((_374 - _2213.z) - _2228.z) + _62_m0[23u].z) + _62_m0[24u].z;
                                float _2257 = mad(_2185.x, _2253, mad(_2171.x, _2249, _2245 * _2157.x)) + _2199.x;
                                float _2261 = mad(_2185.y, _2253, mad(_2171.y, _2249, _2245 * _2157.y)) + _2199.y;
                                if ((clamp(_2257, 0.0f, 1.0f) == _2257) && (clamp(_2261, 0.0f, 1.0f) == _2261))
                                {
                                    frontier_phi_98 = _12[NonUniformResourceIndex((_1960 + 60u) + 0u)].SampleCmpLevelZero(_99, float2(_2257, _2261), clamp(mad(_2185.z, _2253, mad(_2171.z, _2249, _2245 * _2157.z)) + _2199.z, 0.0f, 1.0f)).xxxx.x;
                                    break;
                                }
                            }
                            uint _1961 = _1960 + 1u;
                            if (int(_1961) < int(asuint(_62_m0[69u]).z))
                            {
                                _1960 = _1961;
                                continue;
                            }
                            else
                            {
                                frontier_phi_98 = 1.0f;
                                break;
                            }
                        }
                        frontier_phi_60_61_ladder = frontier_phi_98;
                    }
                    else
                    {
                        frontier_phi_60_61_ladder = 1.0f;
                    }
                    _1747 = frontier_phi_60_61_ladder;
                }
                float _1754 = _62_m0[30u].x * min(_1319, min(_1747, 1.0f));
                _1036 = _1754 + _62_m0[30u].y;
                _1038 = (_1754 * _1214) + _62_m0[30u].y;
            }
            else
            {
                _1036 = 1.0f;
                _1038 = 1.0f;
            }
            float _1233;
            float _1236;
            float _1238;
            float _1240;
            if (dot(float3(_815, _816, _817), float3(_815, _816, _817)) > 0.0f)
            {
                float _1216 = (-0.0f) - _370;
                float _1217 = (-0.0f) - _374;
                float _1221 = rsqrt(dot(float3(_1216, _372, _1217), float3(_1216, _372, _1217)));
                float _1222 = _1221 * _1216;
                float _1223 = _1221 * _372;
                float _1224 = _1221 * _1217;
                float _1383;
                float _1385;
                float _1387;
                float _1390;
                float _1393;
                float _1396;
                float _1399;
                if (_62_m0[66u].x > 0.0f)
                {
                    float frontier_phi_55_54_ladder;
                    float frontier_phi_55_54_ladder_1;
                    float frontier_phi_55_54_ladder_2;
                    float frontier_phi_55_54_ladder_3;
                    float frontier_phi_55_54_ladder_4;
                    float frontier_phi_55_54_ladder_5;
                    float frontier_phi_55_54_ladder_6;
                    if (asuint(_67_m0[59u]).w == 0u)
                    {
                        frontier_phi_55_54_ladder = 0.0f;
                        frontier_phi_55_54_ladder_1 = 0.0f;
                        frontier_phi_55_54_ladder_2 = 0.0f;
                        frontier_phi_55_54_ladder_3 = 0.0f;
                        frontier_phi_55_54_ladder_4 = 0.0f;
                        frontier_phi_55_54_ladder_5 = _1038;
                        frontier_phi_55_54_ladder_6 = 0.0f;
                    }
                    else
                    {
                        uint _1782 = uint(int(_67_m0[58u].x * _437));
                        uint _1783 = uint(int(_67_m0[58u].y * _440));
                        uint _1784 = uint(int(max(_67_m0[60u].y - (_67_m0[60u].x * log2((_67_m0[58u].z * _441) + _67_m0[58u].w)), 0.0f)));
                        uint4 _1786 = _33.Load(int4(uint3(_1782, _1783, _1784), 0u));
                        uint _1788 = _1786.x;
                        uint4 _1792 = asuint(_67_m0[89u]);
                        uint _1803 = (((_1783 << (_1792.x & 31u)) + _1782) + (_1784 << (_1792.y & 31u))) << (_1792.z & 31u);
                        float frontier_phi_55_54_ladder_63_ladder;
                        float frontier_phi_55_54_ladder_63_ladder_1;
                        float frontier_phi_55_54_ladder_63_ladder_2;
                        float frontier_phi_55_54_ladder_63_ladder_3;
                        float frontier_phi_55_54_ladder_63_ladder_4;
                        float frontier_phi_55_54_ladder_63_ladder_5;
                        float frontier_phi_55_54_ladder_63_ladder_6;
                        if (_1788 == 0u)
                        {
                            frontier_phi_55_54_ladder_63_ladder = 0.0f;
                            frontier_phi_55_54_ladder_63_ladder_1 = 0.0f;
                            frontier_phi_55_54_ladder_63_ladder_2 = 0.0f;
                            frontier_phi_55_54_ladder_63_ladder_3 = 0.0f;
                            frontier_phi_55_54_ladder_63_ladder_4 = 0.0f;
                            frontier_phi_55_54_ladder_63_ladder_5 = _1038;
                            frontier_phi_55_54_ladder_63_ladder_6 = 0.0f;
                        }
                        else
                        {
                            float _1893 = _386 - _67_m0[59u].x;
                            float _1894 = _387 - _67_m0[59u].y;
                            float _1895 = _388 - _67_m0[59u].z;
                            uint4 _1907 = _34.Load(_1803);
                            uint _1908 = _1907.x;
                            uint _1909 = _1803 + (_1788 & 127u);
                            uint _1910 = _1909 + ((_1788 >> 7u) & 127u);
                            uint _1911 = _1910 + ((_1788 >> 20u) & 63u);
                            uint _1912 = _1911 + ((_1788 >> 14u) & 63u);
                            uint _1913 = _1912 + (_1788 >> 26u);
                            uint _1914 = _1803 + 1u;
                            float _2099;
                            float _2101;
                            float _2103;
                            float _2105;
                            float _2107;
                            float _2109;
                            float _2111;
                            uint _2113;
                            uint _2115;
                            if (_1914 > _1909)
                            {
                                _2099 = 0.0f;
                                _2101 = 0.0f;
                                _2103 = 0.0f;
                                _2105 = 0.0f;
                                _2107 = _1038;
                                _2109 = 0.0f;
                                _2111 = 0.0f;
                                _2113 = _1914;
                                _2115 = _1908;
                            }
                            else
                            {
                                float _2100;
                                float _2102;
                                float _2104;
                                float _2106;
                                float _2108;
                                float _2110;
                                float _2112;
                                float _2330 = 0.0f;
                                float _2331 = 0.0f;
                                float _2332 = 0.0f;
                                float _2333 = 0.0f;
                                float _2334 = _1038;
                                float _2335 = 0.0f;
                                float _2336 = 0.0f;
                                uint _2337 = _1914;
                                uint _2338 = _1908;
                                uint _2114;
                                uint _2341;
                                uint _2356;
                                uint _2357;
                                float _2383;
                                float _2385;
                                float _2388;
                                float _2390;
                                uint _2392;
                                uint _2394;
                                uint _2396;
                                bool _2399;
                                float _2401;
                                float _2403;
                                bool _2406;
                                for (;;)
                                {
                                    _2114 = _2337 + 1u;
                                    _2341 = _34.Load(_2337).x;
                                    uint _2343 = _2338 * 4u;
                                    uint4 _2355 = uint4(_35.Load(_2343).x, _35.Load(_2343 + 1u).x, _35.Load(_2343 + 2u).x, _35.Load(_2343 + 3u).x);
                                    _2356 = _2355.x;
                                    _2357 = _2355.y;
                                    uint _2358 = _2355.z;
                                    uint _2359 = _2355.w;
                                    uint _2371 = _2338 * 4u;
                                    uint3 _2378 = uint3(_37.Load(_2371).x, _2374, _37.Load(_2371 + 2u).x);
                                    _2383 = spvUnpackHalf2x16(_2357 >> 16u).x;
                                    _2385 = spvUnpackHalf2x16(_2358).x;
                                    _2388 = spvUnpackHalf2x16(_2358 >> 16u).x;
                                    _2390 = spvUnpackHalf2x16(_2359).x;
                                    _2392 = (_2359 >> 16u) & 7u;
                                    _2394 = (_2359 >> 20u) & 3u;
                                    uint _2395 = uint4(_2362, _2363, _2364, _36.Load((_2338 * 4u) + 3u).x).w >> 16u;
                                    _2396 = _2395 & 127u;
                                    _2399 = (_2395 & 32768u) != 0u;
                                    _2401 = spvUnpackHalf2x16(_2378.x).x;
                                    _2403 = spvUnpackHalf2x16(_2378.z).x;
                                    _2406 = (_2359 & 1077936128u) == 4194304u;
                                    float frontier_phi_104_pred;
                                    float frontier_phi_104_pred_1;
                                    float frontier_phi_104_pred_2;
                                    float frontier_phi_104_pred_3;
                                    float frontier_phi_104_pred_4;
                                    float frontier_phi_104_pred_5;
                                    float frontier_phi_104_pred_6;
                                    if (_2406)
                                    {
                                        float _2732 = spvUnpackHalf2x16(_2356).x - _1893;
                                        float _2733 = spvUnpackHalf2x16(_2356 >> 16u).x - _1894;
                                        float _2734 = spvUnpackHalf2x16(_2357).x - _1895;
                                        float _2740 = sqrt(((_2733 * _2733) + (_2734 * _2734)) + (_2732 * _2732));
                                        float _2741 = _2740 * _2383;
                                        float frontier_phi_104_pred_103_ladder;
                                        float frontier_phi_104_pred_103_ladder_1;
                                        float frontier_phi_104_pred_103_ladder_2;
                                        float frontier_phi_104_pred_103_ladder_3;
                                        float frontier_phi_104_pred_103_ladder_4;
                                        float frontier_phi_104_pred_103_ladder_5;
                                        float frontier_phi_104_pred_103_ladder_6;
                                        if (_2741 < 1.0f)
                                        {
                                            float _2977 = rsqrt(dot(float3(_2732, _2733, _2734), float3(_2732, _2733, _2734)));
                                            float _2978 = _2740 * _2740;
                                            float _2980 = (_2383 * _2383) * _2978;
                                            float _2983 = clamp(1.0f - (_2980 * _2980), 0.0f, 1.0f);
                                            float _3383;
                                            if (_2392 == 0u)
                                            {
                                                _3383 = (1.0f / (max(_2978, 9.9999997473787516355514526367188e-05f) + ((_2403 * _2403) * 0.5f))) * (_2983 * _2983);
                                            }
                                            else
                                            {
                                                _3383 = max((1.0f / dot(float3(1.0f, _2741, _2741 * _2741), float3(_67_m0[_2392 + 60u].xyz))) * (1.0f - _2741), 0.0f);
                                            }
                                            float _2747;
                                            float _3539;
                                            if (_2396 == 0u)
                                            {
                                                _3539 = 1.0f;
                                                _2747 = _3383;
                                            }
                                            else
                                            {
                                                uint _3603 = (_2396 * 28u) + 4294967273u;
                                                float3 _3613 = asfloat(uint3(_41.Load(_3603).x, _41.Load(_3603 + 1u).x, _41.Load(_3603 + 2u).x));
                                                uint _3616 = (_2396 * 28u) + 4294967276u;
                                                float3 _3626 = asfloat(uint3(_41.Load(_3616).x, _41.Load(_3616 + 1u).x, _41.Load(_3616 + 2u).x));
                                                float _3643 = asfloat(_41.Load((_2396 * 28u) + 4294967288u).x);
                                                float _3654 = asfloat(_41.Load((_2396 * 28u) + 4294967290u).x);
                                                float _3540;
                                                if (_41.Load((_2396 * 28u) + 4294967270u).x < 4u)
                                                {
                                                    float _3912 = (_386 - _3626.x) - _3613.x;
                                                    float _3914 = (_387 - _3626.y) - _3613.y;
                                                    float _3916 = (_388 - _3626.z) - _3613.z;
                                                    _117[0u] = _3912;
                                                    _117[1u] = _3914;
                                                    _117[2u] = _3916;
                                                    float _3921 = abs(_3912);
                                                    float _3922 = abs(_3914);
                                                    float _3923 = abs(_3916);
                                                    uint _3929 = (_3921 > _3922) ? ((_3921 > _3923) ? 0u : 2u) : ((_3922 > _3923) ? 1u : 2u);
                                                    uint _3935 = (_3929 << 1u) | uint(_117[_3929] < 0.0f);
                                                    uint _3936 = _3935 + _41.Load((_2396 * 28u) + 4294967268u).x;
                                                    uint _3938 = _3936 * 16u;
                                                    float4 _3951 = asfloat(uint4(_40.Load(_3938).x, _40.Load(_3938 + 1u).x, _40.Load(_3938 + 2u).x, _40.Load(_3938 + 3u).x));
                                                    uint _3957 = (_3936 * 16u) + 4u;
                                                    float4 _3970 = asfloat(uint4(_40.Load(_3957).x, _40.Load(_3957 + 1u).x, _40.Load(_3957 + 2u).x, _40.Load(_3957 + 3u).x));
                                                    uint _3976 = (_3936 * 16u) + 8u;
                                                    float4 _3989 = asfloat(uint4(_40.Load(_3976).x, _40.Load(_3976 + 1u).x, _40.Load(_3976 + 2u).x, _40.Load(_3976 + 3u).x));
                                                    uint _3991 = (_3936 * 16u) + 12u;
                                                    float4 _4004 = asfloat(uint4(_40.Load(_3991).x, _40.Load(_3991 + 1u).x, _40.Load(_3991 + 2u).x, _40.Load(_3991 + 3u).x));
                                                    float _4020 = mad(_3916, _4004.z, mad(_3914, _4004.y, _4004.x * _3912)) + _4004.w;
                                                    float _4023 = ((mad(_3916, _3951.z, mad(_3914, _3951.y, _3951.x * _3912)) + _3951.w) / _4020) * 0.5f;
                                                    float _4024 = ((mad(_3916, _3970.z, mad(_3914, _3970.y, _3970.x * _3912)) + _3970.w) / _4020) * (-0.5f);
                                                    float _4026 = _4023 + 0.5f;
                                                    float _4027 = _4024 + 0.5f;
                                                    float frontier_phi_155_154_ladder;
                                                    if (((_4026 < 0.0f) || (_4026 > 1.0f)) || ((_4027 < 0.0f) || (_4027 > 1.0f)))
                                                    {
                                                        frontier_phi_155_154_ladder = 1.0f;
                                                    }
                                                    else
                                                    {
                                                        float _4191;
                                                        if (_41.Load((_2396 * 28u) + 4294967287u).x == 0u)
                                                        {
                                                            _4191 = 1.0f;
                                                        }
                                                        else
                                                        {
                                                            float _4204 = clamp(((max(abs((-0.0f) - _4023), abs((-0.0f) - _4024)) * 2.0f) - _3643) / (0.999000012874603271484375f - _3643), 0.0f, 1.0f);
                                                            _4191 = 1.0f - ((_4204 * _4204) * (3.0f - (_4204 * 2.0f)));
                                                        }
                                                        float _4363;
                                                        if (_41.Load((_2396 * 28u) + 4294967289u).x == 0u)
                                                        {
                                                            _4363 = _4191;
                                                        }
                                                        else
                                                        {
                                                            float _4374 = clamp((((-0.0f) - (_3916 * asfloat(_41.Load((_2396 * 28u) + 4294967292u).x))) - _3654) / (asfloat(_41.Load((_2396 * 28u) + 4294967291u).x) - _3654), 0.0f, 1.0f);
                                                            _4363 = (1.0f - ((_4374 * _4374) * (3.0f - (_4374 * 2.0f)))) * _4191;
                                                        }
                                                        frontier_phi_155_154_ladder = ((_4363 * (1.0f - asfloat(_41.Load((_2396 * 28u) + 4294967286u).x))) * (_20[NonUniformResourceIndex((_2396 + 79u) + 0u)].SampleCmpLevelZero(_100, float3(_4026, _4027, float(int(_3935))), clamp((mad(_3916, _3989.z, mad(_3914, _3989.y, _3989.x * _3912)) + _3989.w) / _4020, 0.0f, 1.0f) - _62_m0[125u].w).xxxx.x + (-1.0f))) + 1.0f;
                                                    }
                                                    _3540 = frontier_phi_155_154_ladder;
                                                }
                                                else
                                                {
                                                    _3540 = 1.0f;
                                                }
                                                _3539 = _3540;
                                                _2747 = _3540 * _3383;
                                            }
                                            float _2746 = float(_2394) * 0.3333333432674407958984375f;
                                            float _3565 = min(max(_823, -0.9900000095367431640625f), 0.9900000095367431640625f);
                                            float _3571 = (min(max(dot(float3((-0.0f) - (_2732 * _2977), (-0.0f) - (_2733 * _2977), (-0.0f) - (_2734 * _2977)), float3(_1222, _1223, _1224)), -1.0f), 1.0f) * _3565) + 1.0f;
                                            float _3574 = (1.0f - (_3565 * _3565)) / ((_3571 * _3571) * 12.56000041961669921875f);
                                            float _3576 = (_62_m0[166u].y * clamp(1.0f - (_2747 * _850), 0.0f, 1.0f)) * (max(float(_2399) * 16.0f, 1.0f) * _3383);
                                            float _3578 = (_3576 * _2385) * _3574;
                                            float _3580 = (_3576 * _2388) * _3574;
                                            float _3582 = (_3576 * _2390) * _3574;
                                            float _3586 = dot(float3(_3578, _3580, _3582), float3(0.2125000059604644775390625f, 0.7153999805450439453125f, 0.07209999859333038330078125f)) * _2747;
                                            float _3590 = _3539 * _2401;
                                            frontier_phi_104_pred_103_ladder = _3586 + _2336;
                                            frontier_phi_104_pred_103_ladder_1 = (((_2746 * _81_m0[20u].y) * _2747) * _3586) + _2335;
                                            frontier_phi_104_pred_103_ladder_2 = _2747;
                                            frontier_phi_104_pred_103_ladder_3 = _2746;
                                            frontier_phi_104_pred_103_ladder_4 = (_3582 * _3590) + _2332;
                                            frontier_phi_104_pred_103_ladder_5 = (_3580 * _3590) + _2331;
                                            frontier_phi_104_pred_103_ladder_6 = (_3578 * _3590) + _2330;
                                        }
                                        else
                                        {
                                            frontier_phi_104_pred_103_ladder = _2336;
                                            frontier_phi_104_pred_103_ladder_1 = _2335;
                                            frontier_phi_104_pred_103_ladder_2 = _2334;
                                            frontier_phi_104_pred_103_ladder_3 = _2333;
                                            frontier_phi_104_pred_103_ladder_4 = _2332;
                                            frontier_phi_104_pred_103_ladder_5 = _2331;
                                            frontier_phi_104_pred_103_ladder_6 = _2330;
                                        }
                                        frontier_phi_104_pred = frontier_phi_104_pred_103_ladder;
                                        frontier_phi_104_pred_1 = frontier_phi_104_pred_103_ladder_1;
                                        frontier_phi_104_pred_2 = frontier_phi_104_pred_103_ladder_2;
                                        frontier_phi_104_pred_3 = frontier_phi_104_pred_103_ladder_3;
                                        frontier_phi_104_pred_4 = frontier_phi_104_pred_103_ladder_4;
                                        frontier_phi_104_pred_5 = frontier_phi_104_pred_103_ladder_5;
                                        frontier_phi_104_pred_6 = frontier_phi_104_pred_103_ladder_6;
                                    }
                                    else
                                    {
                                        frontier_phi_104_pred = _2336;
                                        frontier_phi_104_pred_1 = _2335;
                                        frontier_phi_104_pred_2 = _2334;
                                        frontier_phi_104_pred_3 = _2333;
                                        frontier_phi_104_pred_4 = _2332;
                                        frontier_phi_104_pred_5 = _2331;
                                        frontier_phi_104_pred_6 = _2330;
                                    }
                                    _2112 = frontier_phi_104_pred;
                                    _2110 = frontier_phi_104_pred_1;
                                    _2108 = frontier_phi_104_pred_2;
                                    _2106 = frontier_phi_104_pred_3;
                                    _2104 = frontier_phi_104_pred_4;
                                    _2102 = frontier_phi_104_pred_5;
                                    _2100 = frontier_phi_104_pred_6;
                                    if (_2114 > _1909)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        _2330 = _2100;
                                        _2331 = _2102;
                                        _2332 = _2104;
                                        _2333 = _2106;
                                        _2334 = _2108;
                                        _2335 = _2110;
                                        _2336 = _2112;
                                        _2337 = _2114;
                                        _2338 = _2341;
                                        continue;
                                    }
                                }
                                _2099 = _2100;
                                _2101 = _2102;
                                _2103 = _2104;
                                _2105 = _2106;
                                _2107 = _2108;
                                _2109 = _2110;
                                _2111 = _2112;
                                _2113 = _2114;
                                _2115 = _2341;
                            }
                            float _2311;
                            float _2313;
                            float _2315;
                            float _2317;
                            float _2319;
                            float _2321;
                            float _2323;
                            uint _2325;
                            uint _2327;
                            if (_2113 > _1910)
                            {
                                _2311 = _2099;
                                _2313 = _2101;
                                _2315 = _2103;
                                _2317 = _2105;
                                _2319 = _2107;
                                _2321 = _2109;
                                _2323 = _2111;
                                _2325 = _2113;
                                _2327 = _2115;
                            }
                            else
                            {
                                float _2312;
                                float _2314;
                                float _2316;
                                float _2318;
                                float _2320;
                                float _2322;
                                float _2324;
                                float _2620 = _2099;
                                float _2621 = _2101;
                                float _2622 = _2103;
                                float _2623 = _2105;
                                float _2624 = _2107;
                                float _2625 = _2109;
                                float _2626 = _2111;
                                uint _2627 = _2113;
                                uint _2628 = _2115;
                                uint _2326;
                                uint _2631;
                                uint _2646;
                                uint _2647;
                                float _2681;
                                float _2683;
                                float _2686;
                                float _2688;
                                uint _2690;
                                uint _2694;
                                bool _2695;
                                float _2697;
                                float _2700;
                                float _2702;
                                float _2705;
                                float _2707;
                                float _2710;
                                float _2712;
                                float _2714;
                                float _2716;
                                uint _2718;
                                uint _2720;
                                uint _2721;
                                bool _2724;
                                for (;;)
                                {
                                    _2326 = _2627 + 1u;
                                    _2631 = _34.Load(_2627).x;
                                    uint _2633 = _2628 * 4u;
                                    uint4 _2645 = uint4(_35.Load(_2633).x, _35.Load(_2633 + 1u).x, _35.Load(_2633 + 2u).x, _35.Load(_2633 + 3u).x);
                                    _2646 = _2645.x;
                                    _2647 = _2645.y;
                                    uint _2648 = _2645.z;
                                    uint _2649 = _2645.w;
                                    uint _2651 = _2628 * 4u;
                                    uint4 _2663 = uint4(_36.Load(_2651).x, _36.Load(_2651 + 1u).x, _36.Load(_2651 + 2u).x, _36.Load(_2651 + 3u).x);
                                    uint _2664 = _2663.x;
                                    uint _2665 = _2663.y;
                                    uint _2666 = _2663.z;
                                    uint _2667 = _2663.w;
                                    uint _2669 = _2628 * 4u;
                                    uint3 _2676 = uint3(_37.Load(_2669).x, _2672, _37.Load(_2669 + 2u).x);
                                    _2681 = spvUnpackHalf2x16(_2647 >> 16u).x;
                                    _2683 = spvUnpackHalf2x16(_2648).x;
                                    _2686 = spvUnpackHalf2x16(_2648 >> 16u).x;
                                    _2688 = spvUnpackHalf2x16(_2649).x;
                                    _2690 = (_2649 >> 16u) & 7u;
                                    _2694 = (_2649 >> 20u) & 3u;
                                    _2695 = (_2649 & 524288u) != 0u;
                                    _2697 = spvUnpackHalf2x16(_2664).x;
                                    _2700 = spvUnpackHalf2x16(_2664 >> 16u).x;
                                    _2702 = spvUnpackHalf2x16(_2665).x;
                                    _2705 = spvUnpackHalf2x16(_2665 >> 16u).x;
                                    _2707 = spvUnpackHalf2x16(_2666).x;
                                    _2710 = spvUnpackHalf2x16(_2666 >> 16u).x;
                                    _2712 = spvUnpackHalf2x16(_2667).x;
                                    _2714 = spvUnpackHalf2x16(_2676.x).x;
                                    _2716 = spvUnpackHalf2x16(_2676.z).x;
                                    _2718 = (_2667 >> 16u) & 127u;
                                    _2720 = (_2667 >> 23u) & 31u;
                                    _2721 = _2667 >> 28u;
                                    _2724 = (_2649 & 1077936128u) == 4194304u;
                                    float frontier_phi_115_pred;
                                    float frontier_phi_115_pred_1;
                                    float frontier_phi_115_pred_2;
                                    float frontier_phi_115_pred_3;
                                    float frontier_phi_115_pred_4;
                                    float frontier_phi_115_pred_5;
                                    float frontier_phi_115_pred_6;
                                    if (_2724)
                                    {
                                        float _2955 = spvUnpackHalf2x16(_2646).x - _1893;
                                        float _2956 = spvUnpackHalf2x16(_2646 >> 16u).x - _1894;
                                        float _2957 = spvUnpackHalf2x16(_2647).x - _1895;
                                        float _2963 = sqrt(((_2956 * _2956) + (_2957 * _2957)) + (_2955 * _2955));
                                        float _2964 = _2963 * _2681;
                                        float frontier_phi_115_pred_114_ladder;
                                        float frontier_phi_115_pred_114_ladder_1;
                                        float frontier_phi_115_pred_114_ladder_2;
                                        float frontier_phi_115_pred_114_ladder_3;
                                        float frontier_phi_115_pred_114_ladder_4;
                                        float frontier_phi_115_pred_114_ladder_5;
                                        float frontier_phi_115_pred_114_ladder_6;
                                        if (_2964 < 1.0f)
                                        {
                                            float _3079 = rsqrt(dot(float3(_2955, _2956, _2957), float3(_2955, _2956, _2957)));
                                            float _3080 = _2963 * _2963;
                                            float _3082 = (_2681 * _2681) * _3080;
                                            float _3085 = clamp(1.0f - (_3082 * _3082), 0.0f, 1.0f);
                                            float _3502;
                                            if (_2690 == 0u)
                                            {
                                                _3502 = (1.0f / (max(_3080, 9.9999997473787516355514526367188e-05f) + ((_2716 * _2716) * 0.5f))) * (_3085 * _3085);
                                            }
                                            else
                                            {
                                                _3502 = max((1.0f / dot(float3(1.0f, _2964, _2964 * _2964), float3(_67_m0[_2690 + 60u].xyz))) * (1.0f - _2964), 0.0f);
                                            }
                                            float _3503 = (-0.0f) - _2697;
                                            float _3518 = (-0.0f) - (_2955 * _3079);
                                            float _3520 = (-0.0f) - (_2956 * _3079);
                                            float _3522 = (-0.0f) - (_2957 * _3079);
                                            float _3530 = clamp((clamp(dot(float3(((_2705 * _2700) - (_2702 * _3503)) * 2.0f, ((_2702 * _2700) - (_2705 * _2697)) * 2.0f, (((_2697 * _3503) - (_2700 * _2700)) * 2.0f) + 1.0f), float3(_3518, _3520, _3522)), 0.0f, 1.0f) - _2710) / (_2707 - _2710), 0.0f, 1.0f);
                                            float _3535 = ((_3530 * _3530) * (3.0f - (_3530 * 2.0f))) * _3502;
                                            float _2970;
                                            float _3820;
                                            float _3823;
                                            float _3825;
                                            float _3827;
                                            if ((_2720 | _2718) == 0u)
                                            {
                                                _3820 = 1.0f;
                                                _2970 = _3535;
                                                _3823 = _2683;
                                                _3825 = _2686;
                                                _3827 = _2688;
                                            }
                                            else
                                            {
                                                float _3875 = (-0.0f) - _2700;
                                                float _3876 = (-0.0f) - _2702;
                                                float _3889 = ((_2957 * _3875) - (_2956 * _3876)) + (_2955 * _2705);
                                                float _3890 = ((_2955 * _3876) - (_2957 * _3503)) + (_2956 * _2705);
                                                float _3891 = ((_2956 * _3503) - (_2955 * _3875)) + (_2957 * _2705);
                                                float _3898 = (1.0f / ((((_3890 * _3503) - (_3889 * _3875)) * 2.0f) + _2957)) * _2712;
                                                float _3824;
                                                float _3826;
                                                float _3828;
                                                if (_2720 == 0u)
                                                {
                                                    _3824 = _2683;
                                                    _3826 = _2686;
                                                    _3828 = _2688;
                                                }
                                                else
                                                {
                                                    uint _4056 = _2721 + 72u;
                                                    float4 _4074 = _39.SampleLevel(_96, float3((_67_m0[_4056].x * ((_3898 * ((((_3891 * _3875) - (_3890 * _3876)) * 2.0f) + _2955)) + 0.5f)) + _67_m0[_4056].z, (_67_m0[_4056].y * (0.5f - (_3898 * ((((_3889 * _3876) - (_3891 * _3503)) * 2.0f) + _2956)))) + _67_m0[_4056].w, float(_2720 + 4294967295u)), 0.0f);
                                                    _3824 = _4074.x * _2683;
                                                    _3826 = _4074.y * _2686;
                                                    _3828 = _4074.z * _2688;
                                                }
                                                float frontier_phi_152_157_ladder;
                                                float frontier_phi_152_157_ladder_1;
                                                float frontier_phi_152_157_ladder_2;
                                                float frontier_phi_152_157_ladder_3;
                                                float frontier_phi_152_157_ladder_4;
                                                if (_2718 == 0u)
                                                {
                                                    frontier_phi_152_157_ladder = 1.0f;
                                                    frontier_phi_152_157_ladder_1 = _3535;
                                                    frontier_phi_152_157_ladder_2 = _3824;
                                                    frontier_phi_152_157_ladder_3 = _3828;
                                                    frontier_phi_152_157_ladder_4 = _3826;
                                                }
                                                else
                                                {
                                                    uint4 _4127 = _41.Load((_2718 * 28u) + 4294967268u);
                                                    uint _4128 = _4127.x;
                                                    uint _4134 = (_2718 * 28u) + 4294967273u;
                                                    float3 _4144 = asfloat(uint3(_41.Load(_4134).x, _41.Load(_4134 + 1u).x, _41.Load(_4134 + 2u).x));
                                                    uint _4146 = (_2718 * 28u) + 4294967276u;
                                                    float3 _4156 = asfloat(uint3(_41.Load(_4146).x, _41.Load(_4146 + 1u).x, _41.Load(_4146 + 2u).x));
                                                    float _4170 = asfloat(_41.Load((_2718 * 28u) + 4294967288u).x);
                                                    float _4179 = asfloat(_41.Load((_2718 * 28u) + 4294967290u).x);
                                                    float _3821;
                                                    if (_41.Load((_2718 * 28u) + 4294967270u).x < 4u)
                                                    {
                                                        float _4260 = (_386 - _4156.x) - _4144.x;
                                                        float _4262 = (_387 - _4156.y) - _4144.y;
                                                        float _4264 = (_388 - _4156.z) - _4144.z;
                                                        uint _4266 = _4128 * 16u;
                                                        float4 _4279 = asfloat(uint4(_40.Load(_4266).x, _40.Load(_4266 + 1u).x, _40.Load(_4266 + 2u).x, _40.Load(_4266 + 3u).x));
                                                        uint _4285 = (_4128 * 16u) + 4u;
                                                        float4 _4298 = asfloat(uint4(_40.Load(_4285).x, _40.Load(_4285 + 1u).x, _40.Load(_4285 + 2u).x, _40.Load(_4285 + 3u).x));
                                                        uint _4304 = (_4128 * 16u) + 8u;
                                                        float4 _4317 = asfloat(uint4(_40.Load(_4304).x, _40.Load(_4304 + 1u).x, _40.Load(_4304 + 2u).x, _40.Load(_4304 + 3u).x));
                                                        uint _4319 = (_4128 * 16u) + 12u;
                                                        float4 _4332 = asfloat(uint4(_40.Load(_4319).x, _40.Load(_4319 + 1u).x, _40.Load(_4319 + 2u).x, _40.Load(_4319 + 3u).x));
                                                        float _4348 = mad(_4264, _4332.z, mad(_4262, _4332.y, _4332.x * _4260)) + _4332.w;
                                                        float _4351 = ((mad(_4264, _4279.z, mad(_4262, _4279.y, _4279.x * _4260)) + _4279.w) / _4348) * 0.5f;
                                                        float _4352 = ((mad(_4264, _4298.z, mad(_4262, _4298.y, _4298.x * _4260)) + _4298.w) / _4348) * (-0.5f);
                                                        float _4353 = _4351 + 0.5f;
                                                        float _4354 = _4352 + 0.5f;
                                                        float frontier_phi_166_165_ladder;
                                                        if (((_4353 < 0.0f) || (_4353 > 1.0f)) || ((_4354 < 0.0f) || (_4354 > 1.0f)))
                                                        {
                                                            frontier_phi_166_165_ladder = 1.0f;
                                                        }
                                                        else
                                                        {
                                                            float _4402;
                                                            if (_41.Load((_2718 * 28u) + 4294967287u).x == 0u)
                                                            {
                                                                _4402 = 1.0f;
                                                            }
                                                            else
                                                            {
                                                                float _4414 = clamp(((max(abs((-0.0f) - _4351), abs((-0.0f) - _4352)) * 2.0f) - _4170) / (0.999000012874603271484375f - _4170), 0.0f, 1.0f);
                                                                _4402 = 1.0f - ((_4414 * _4414) * (3.0f - (_4414 * 2.0f)));
                                                            }
                                                            float _4419;
                                                            if (_41.Load((_2718 * 28u) + 4294967289u).x == 0u)
                                                            {
                                                                _4419 = _4402;
                                                            }
                                                            else
                                                            {
                                                                float _4430 = clamp((((-0.0f) - (_4264 * asfloat(_41.Load((_2718 * 28u) + 4294967292u).x))) - _4179) / (asfloat(_41.Load((_2718 * 28u) + 4294967291u).x) - _4179), 0.0f, 1.0f);
                                                                _4419 = (1.0f - ((_4430 * _4430) * (3.0f - (_4430 * 2.0f)))) * _4402;
                                                            }
                                                            frontier_phi_166_165_ladder = ((_4419 * (1.0f - asfloat(_41.Load((_2718 * 28u) + 4294967286u).x))) * (_12[NonUniformResourceIndex((_2718 + 79u) + 0u)].SampleCmpLevelZero(_100, float2(_4353, _4354), clamp((mad(_4264, _4317.z, mad(_4262, _4317.y, _4317.x * _4260)) + _4317.w) / _4348, 0.0f, 1.0f) - _62_m0[125u].w).xxxx.x + (-1.0f))) + 1.0f;
                                                        }
                                                        _3821 = frontier_phi_166_165_ladder;
                                                    }
                                                    else
                                                    {
                                                        _3821 = 1.0f;
                                                    }
                                                    frontier_phi_152_157_ladder = _3821;
                                                    frontier_phi_152_157_ladder_1 = _3821 * _3535;
                                                    frontier_phi_152_157_ladder_2 = _3824;
                                                    frontier_phi_152_157_ladder_3 = _3828;
                                                    frontier_phi_152_157_ladder_4 = _3826;
                                                }
                                                _3820 = frontier_phi_152_157_ladder;
                                                _2970 = frontier_phi_152_157_ladder_1;
                                                _3823 = frontier_phi_152_157_ladder_2;
                                                _3825 = frontier_phi_152_157_ladder_4;
                                                _3827 = frontier_phi_152_157_ladder_3;
                                            }
                                            float _2969 = float(_2694) * 0.3333333432674407958984375f;
                                            float _3843 = min(max(_823, -0.9900000095367431640625f), 0.9900000095367431640625f);
                                            float _3849 = (min(max(dot(float3(_3518, _3520, _3522), float3(_1222, _1223, _1224)), -1.0f), 1.0f) * _3843) + 1.0f;
                                            float _3852 = (1.0f - (_3843 * _3843)) / ((_3849 * _3849) * 12.56000041961669921875f);
                                            float _3854 = clamp(1.0f - (_2970 * _850), 0.0f, 1.0f) * (max(float(_2695) * 16.0f, 1.0f) * _3535);
                                            float _3857 = ((_3854 * _3823) * _62_m0[166u].y) * _3852;
                                            float _3860 = ((_3854 * _3825) * _62_m0[166u].y) * _3852;
                                            float _3863 = ((_3854 * _3827) * _62_m0[166u].y) * _3852;
                                            float _3867 = dot(float3(_3857, _3860, _3863), float3(0.2125000059604644775390625f, 0.7153999805450439453125f, 0.07209999859333038330078125f)) * _2970;
                                            float _3871 = _3820 * _2714;
                                            frontier_phi_115_pred_114_ladder = _3867 + _2626;
                                            frontier_phi_115_pred_114_ladder_1 = (((_2969 * _81_m0[20u].y) * _2970) * _3867) + _2625;
                                            frontier_phi_115_pred_114_ladder_2 = _2970;
                                            frontier_phi_115_pred_114_ladder_3 = _2969;
                                            frontier_phi_115_pred_114_ladder_4 = (_3863 * _3871) + _2622;
                                            frontier_phi_115_pred_114_ladder_5 = (_3860 * _3871) + _2621;
                                            frontier_phi_115_pred_114_ladder_6 = (_3857 * _3871) + _2620;
                                        }
                                        else
                                        {
                                            frontier_phi_115_pred_114_ladder = _2626;
                                            frontier_phi_115_pred_114_ladder_1 = _2625;
                                            frontier_phi_115_pred_114_ladder_2 = _2624;
                                            frontier_phi_115_pred_114_ladder_3 = _2623;
                                            frontier_phi_115_pred_114_ladder_4 = _2622;
                                            frontier_phi_115_pred_114_ladder_5 = _2621;
                                            frontier_phi_115_pred_114_ladder_6 = _2620;
                                        }
                                        frontier_phi_115_pred = frontier_phi_115_pred_114_ladder;
                                        frontier_phi_115_pred_1 = frontier_phi_115_pred_114_ladder_1;
                                        frontier_phi_115_pred_2 = frontier_phi_115_pred_114_ladder_2;
                                        frontier_phi_115_pred_3 = frontier_phi_115_pred_114_ladder_3;
                                        frontier_phi_115_pred_4 = frontier_phi_115_pred_114_ladder_4;
                                        frontier_phi_115_pred_5 = frontier_phi_115_pred_114_ladder_5;
                                        frontier_phi_115_pred_6 = frontier_phi_115_pred_114_ladder_6;
                                    }
                                    else
                                    {
                                        frontier_phi_115_pred = _2626;
                                        frontier_phi_115_pred_1 = _2625;
                                        frontier_phi_115_pred_2 = _2624;
                                        frontier_phi_115_pred_3 = _2623;
                                        frontier_phi_115_pred_4 = _2622;
                                        frontier_phi_115_pred_5 = _2621;
                                        frontier_phi_115_pred_6 = _2620;
                                    }
                                    _2324 = frontier_phi_115_pred;
                                    _2322 = frontier_phi_115_pred_1;
                                    _2320 = frontier_phi_115_pred_2;
                                    _2318 = frontier_phi_115_pred_3;
                                    _2316 = frontier_phi_115_pred_4;
                                    _2314 = frontier_phi_115_pred_5;
                                    _2312 = frontier_phi_115_pred_6;
                                    if (_2326 > _1910)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        _2620 = _2312;
                                        _2621 = _2314;
                                        _2622 = _2316;
                                        _2623 = _2318;
                                        _2624 = _2320;
                                        _2625 = _2322;
                                        _2626 = _2324;
                                        _2627 = _2326;
                                        _2628 = _2631;
                                        continue;
                                    }
                                }
                                _2311 = _2312;
                                _2313 = _2314;
                                _2315 = _2316;
                                _2317 = _2318;
                                _2319 = _2320;
                                _2321 = _2322;
                                _2323 = _2324;
                                _2325 = _2326;
                                _2327 = _2631;
                            }
                            float _1384;
                            float _1386;
                            float _1388;
                            float _1391;
                            float _1394;
                            float _1397;
                            float _1400;
                            uint _2615;
                            uint _2617;
                            if (_2325 > _1911)
                            {
                                _1394 = _2311;
                                _1397 = _2313;
                                _1400 = _2315;
                                _1384 = _2317;
                                _1386 = _2319;
                                _1388 = _2321;
                                _1391 = _2323;
                                _2615 = _2325;
                                _2617 = _2327;
                            }
                            else
                            {
                                float _2608;
                                float _2609;
                                float _2610;
                                float _2611;
                                float _2612;
                                float _2613;
                                float _2614;
                                float _2856 = _2311;
                                float _2857 = _2313;
                                float _2858 = _2315;
                                float _2859 = _2317;
                                float _2860 = _2319;
                                float _2861 = _2321;
                                float _2862 = _2323;
                                uint _2863 = _2325;
                                uint _2864 = _2327;
                                uint _2616;
                                uint _2867;
                                uint _2882;
                                uint _2883;
                                float _2913;
                                float _2915;
                                float _2918;
                                float _2920;
                                uint _2922;
                                uint _2925;
                                bool _2926;
                                float _2928;
                                float _2931;
                                float _2933;
                                float _2936;
                                float _2938;
                                float _2941;
                                float _2943;
                                float _2945;
                                bool _2947;
                                for (;;)
                                {
                                    _2616 = _2863 + 1u;
                                    _2867 = _34.Load(_2863).x;
                                    uint _2869 = _2864 * 4u;
                                    uint4 _2881 = uint4(_35.Load(_2869).x, _35.Load(_2869 + 1u).x, _35.Load(_2869 + 2u).x, _35.Load(_2869 + 3u).x);
                                    _2882 = _2881.x;
                                    _2883 = _2881.y;
                                    uint _2884 = _2881.z;
                                    uint _2885 = _2881.w;
                                    uint _2887 = _2864 * 4u;
                                    uint3 _2896 = uint3(_36.Load(_2887).x, _36.Load(_2887 + 1u).x, _36.Load(_2887 + 2u).x);
                                    uint _2897 = _2896.x;
                                    uint _2898 = _2896.y;
                                    uint _2899 = _2896.z;
                                    uint _2901 = _2864 * 4u;
                                    uint3 _2908 = uint3(_37.Load(_2901).x, _2904, _37.Load(_2901 + 2u).x);
                                    _2913 = spvUnpackHalf2x16(_2883 >> 16u).x;
                                    _2915 = spvUnpackHalf2x16(_2884).x;
                                    _2918 = spvUnpackHalf2x16(_2884 >> 16u).x;
                                    _2920 = spvUnpackHalf2x16(_2885).x;
                                    _2922 = (_2885 >> 16u) & 7u;
                                    _2925 = (_2885 >> 20u) & 3u;
                                    _2926 = (_2885 & 524288u) != 0u;
                                    _2928 = spvUnpackHalf2x16(_2897).x;
                                    _2931 = spvUnpackHalf2x16(_2897 >> 16u).x;
                                    _2933 = spvUnpackHalf2x16(_2898).x;
                                    _2936 = spvUnpackHalf2x16(_2898 >> 16u).x;
                                    _2938 = spvUnpackHalf2x16(_2899).x;
                                    _2941 = spvUnpackHalf2x16(_2899 >> 16u).x;
                                    _2943 = spvUnpackHalf2x16(_2908.x).x;
                                    _2945 = spvUnpackHalf2x16(_2908.z).x;
                                    _2947 = (_2885 & 1077936128u) == 4194304u;
                                    float frontier_phi_124_pred;
                                    float frontier_phi_124_pred_1;
                                    float frontier_phi_124_pred_2;
                                    float frontier_phi_124_pred_3;
                                    float frontier_phi_124_pred_4;
                                    float frontier_phi_124_pred_5;
                                    float frontier_phi_124_pred_6;
                                    if (_2947)
                                    {
                                        float _3057 = spvUnpackHalf2x16(_2882).x - _1893;
                                        float _3058 = spvUnpackHalf2x16(_2882 >> 16u).x - _1894;
                                        float _3059 = spvUnpackHalf2x16(_2883).x - _1895;
                                        float _3065 = sqrt(((_3058 * _3058) + (_3059 * _3059)) + (_3057 * _3057));
                                        float _3066 = _3065 * _2913;
                                        float frontier_phi_124_pred_123_ladder;
                                        float frontier_phi_124_pred_123_ladder_1;
                                        float frontier_phi_124_pred_123_ladder_2;
                                        float frontier_phi_124_pred_123_ladder_3;
                                        float frontier_phi_124_pred_123_ladder_4;
                                        float frontier_phi_124_pred_123_ladder_5;
                                        float frontier_phi_124_pred_123_ladder_6;
                                        if (_3066 < 1.0f)
                                        {
                                            float _3354 = rsqrt(dot(float3(_3057, _3058, _3059), float3(_3057, _3058, _3059)));
                                            float _3355 = _3065 * _3065;
                                            float _3357 = (_2913 * _2913) * _3355;
                                            float _3360 = clamp(1.0f - (_3357 * _3357), 0.0f, 1.0f);
                                            float _3742;
                                            if (_2922 == 0u)
                                            {
                                                _3742 = (_3360 * _3360) * (1.0f / (max(_3355, 9.9999997473787516355514526367188e-05f) + ((_2945 * _2945) * 0.5f)));
                                            }
                                            else
                                            {
                                                _3742 = max((1.0f / dot(float3(1.0f, _3066, _3066 * _3066), float3(_67_m0[_2922 + 60u].xyz))) * (1.0f - _3066), 0.0f);
                                            }
                                            float _3743 = (-0.0f) - _2928;
                                            float _3758 = (-0.0f) - (_3057 * _3354);
                                            float _3760 = (-0.0f) - (_3058 * _3354);
                                            float _3762 = (-0.0f) - (_3059 * _3354);
                                            float _3770 = clamp((clamp(dot(float3(((_2936 * _2931) - (_2933 * _3743)) * 2.0f, ((_2933 * _2931) - (_2936 * _2928)) * 2.0f, (((_2928 * _3743) - (_2931 * _2931)) * 2.0f) + 1.0f), float3(_3758, _3760, _3762)), 0.0f, 1.0f) - _2941) / (_2938 - _2941), 0.0f, 1.0f);
                                            float _3072 = ((_3770 * _3770) * _3742) * (3.0f - (_3770 * 2.0f));
                                            float _3071 = float(_2925) * 0.3333333432674407958984375f;
                                            float _3789 = min(max(_823, -0.9900000095367431640625f), 0.9900000095367431640625f);
                                            float _3795 = (min(max(dot(float3(_3758, _3760, _3762), float3(_1222, _1223, _1224)), -1.0f), 1.0f) * _3789) + 1.0f;
                                            float _3798 = (1.0f - (_3789 * _3789)) / ((_3795 * _3795) * 12.56000041961669921875f);
                                            float _3800 = _3072 * (clamp(1.0f - (_3072 * _850), 0.0f, 1.0f) * max(float(_2926) * 16.0f, 1.0f));
                                            float _3803 = ((_3800 * _2915) * _62_m0[166u].y) * _3798;
                                            float _3806 = ((_3800 * _2918) * _62_m0[166u].y) * _3798;
                                            float _3809 = ((_3800 * _2920) * _62_m0[166u].y) * _3798;
                                            float _3813 = dot(float3(_3803, _3806, _3809), float3(0.2125000059604644775390625f, 0.7153999805450439453125f, 0.07209999859333038330078125f)) * _3072;
                                            frontier_phi_124_pred_123_ladder = (((_3071 * _81_m0[20u].y) * _3072) * _3813) + _2861;
                                            frontier_phi_124_pred_123_ladder_1 = (_3803 * _2943) + _2856;
                                            frontier_phi_124_pred_123_ladder_2 = (_3806 * _2943) + _2857;
                                            frontier_phi_124_pred_123_ladder_3 = (_3809 * _2943) + _2858;
                                            frontier_phi_124_pred_123_ladder_4 = _3072;
                                            frontier_phi_124_pred_123_ladder_5 = _3071;
                                            frontier_phi_124_pred_123_ladder_6 = _3813 + _2862;
                                        }
                                        else
                                        {
                                            frontier_phi_124_pred_123_ladder = _2861;
                                            frontier_phi_124_pred_123_ladder_1 = _2856;
                                            frontier_phi_124_pred_123_ladder_2 = _2857;
                                            frontier_phi_124_pred_123_ladder_3 = _2858;
                                            frontier_phi_124_pred_123_ladder_4 = _2860;
                                            frontier_phi_124_pred_123_ladder_5 = _2859;
                                            frontier_phi_124_pred_123_ladder_6 = _2862;
                                        }
                                        frontier_phi_124_pred = frontier_phi_124_pred_123_ladder;
                                        frontier_phi_124_pred_1 = frontier_phi_124_pred_123_ladder_1;
                                        frontier_phi_124_pred_2 = frontier_phi_124_pred_123_ladder_2;
                                        frontier_phi_124_pred_3 = frontier_phi_124_pred_123_ladder_3;
                                        frontier_phi_124_pred_4 = frontier_phi_124_pred_123_ladder_4;
                                        frontier_phi_124_pred_5 = frontier_phi_124_pred_123_ladder_5;
                                        frontier_phi_124_pred_6 = frontier_phi_124_pred_123_ladder_6;
                                    }
                                    else
                                    {
                                        frontier_phi_124_pred = _2861;
                                        frontier_phi_124_pred_1 = _2856;
                                        frontier_phi_124_pred_2 = _2857;
                                        frontier_phi_124_pred_3 = _2858;
                                        frontier_phi_124_pred_4 = _2860;
                                        frontier_phi_124_pred_5 = _2859;
                                        frontier_phi_124_pred_6 = _2862;
                                    }
                                    _2613 = frontier_phi_124_pred;
                                    _2608 = frontier_phi_124_pred_1;
                                    _2609 = frontier_phi_124_pred_2;
                                    _2610 = frontier_phi_124_pred_3;
                                    _2612 = frontier_phi_124_pred_4;
                                    _2611 = frontier_phi_124_pred_5;
                                    _2614 = frontier_phi_124_pred_6;
                                    if (_2616 > _1911)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        _2856 = _2608;
                                        _2857 = _2609;
                                        _2858 = _2610;
                                        _2859 = _2611;
                                        _2860 = _2612;
                                        _2861 = _2613;
                                        _2862 = _2614;
                                        _2863 = _2616;
                                        _2864 = _2867;
                                        continue;
                                    }
                                }
                                _1394 = _2608;
                                _1397 = _2609;
                                _1400 = _2610;
                                _1384 = _2611;
                                _1386 = _2612;
                                _1388 = _2613;
                                _1391 = _2614;
                                _2615 = _2616;
                                _2617 = _2867;
                            }
                            uint _2851;
                            uint _2853;
                            if (_2615 > _1912)
                            {
                                _2851 = _2615;
                                _2853 = _2617;
                            }
                            else
                            {
                                uint _3048;
                                _3048 = _2615;
                                uint _2852;
                                for (;;)
                                {
                                    _2852 = _3048 + 1u;
                                    if (_2852 > _1912)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        _3048 = _2852;
                                    }
                                }
                                _2851 = _2852;
                                _2853 = _34.Load(_3048).x;
                            }
                            float frontier_phi_55_54_ladder_63_ladder_111_ladder;
                            float frontier_phi_55_54_ladder_63_ladder_111_ladder_1;
                            float frontier_phi_55_54_ladder_63_ladder_111_ladder_2;
                            float frontier_phi_55_54_ladder_63_ladder_111_ladder_3;
                            float frontier_phi_55_54_ladder_63_ladder_111_ladder_4;
                            float frontier_phi_55_54_ladder_63_ladder_111_ladder_5;
                            float frontier_phi_55_54_ladder_63_ladder_111_ladder_6;
                            if (_2851 > _1913)
                            {
                                frontier_phi_55_54_ladder_63_ladder_111_ladder = _1400;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_1 = _1397;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_2 = _1394;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_3 = _1391;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_4 = _1388;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_5 = _1386;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_6 = _1384;
                            }
                            else
                            {
                                float _1389;
                                float _1392;
                                float _1395;
                                float _1398;
                                float _1401;
                                float _3252 = _1394;
                                float _3253 = _1397;
                                float _3254 = _1400;
                                float _3255 = _1388;
                                float _3256 = _1391;
                                uint _3257 = _2851;
                                uint _3259 = _2853;
                                uint _3258;
                                uint _3263;
                                uint _3278;
                                uint _3279;
                                float _3313;
                                float _3315;
                                float _3318;
                                float _3320;
                                uint _3322;
                                bool _3324;
                                float _3326;
                                float _3329;
                                float _3331;
                                float _3334;
                                float _3336;
                                float _3339;
                                float _3341;
                                float _3343;
                                float _3345;
                                bool _3347;
                                for (;;)
                                {
                                    _3258 = _3257 + 1u;
                                    _3263 = _34.Load(_3257).x;
                                    uint _3265 = _3259 * 4u;
                                    uint4 _3277 = uint4(_35.Load(_3265).x, _35.Load(_3265 + 1u).x, _35.Load(_3265 + 2u).x, _35.Load(_3265 + 3u).x);
                                    _3278 = _3277.x;
                                    _3279 = _3277.y;
                                    uint _3280 = _3277.z;
                                    uint _3281 = _3277.w;
                                    uint _3283 = _3259 * 4u;
                                    uint4 _3295 = uint4(_36.Load(_3283).x, _36.Load(_3283 + 1u).x, _36.Load(_3283 + 2u).x, _36.Load(_3283 + 3u).x);
                                    uint _3296 = _3295.x;
                                    uint _3297 = _3295.y;
                                    uint _3298 = _3295.z;
                                    uint _3301 = _3259 * 4u;
                                    uint3 _3308 = uint3(_37.Load(_3301).x, _3304, _37.Load(_3301 + 2u).x);
                                    _3313 = spvUnpackHalf2x16(_3279 >> 16u).x;
                                    _3315 = spvUnpackHalf2x16(_3280).x;
                                    _3318 = spvUnpackHalf2x16(_3280 >> 16u).x;
                                    _3320 = spvUnpackHalf2x16(_3281).x;
                                    _3322 = (_3281 >> 16u) & 7u;
                                    _3324 = (_3281 & 524288u) != 0u;
                                    _3326 = spvUnpackHalf2x16(_3296).x;
                                    _3329 = spvUnpackHalf2x16(_3296 >> 16u).x;
                                    _3331 = spvUnpackHalf2x16(_3297).x;
                                    _3334 = spvUnpackHalf2x16(_3297 >> 16u).x;
                                    _3336 = spvUnpackHalf2x16(_3298).x;
                                    _3339 = spvUnpackHalf2x16(_3298 >> 16u).x;
                                    _3341 = spvUnpackHalf2x16(_3295.w).x;
                                    _3343 = spvUnpackHalf2x16(_3308.x).x;
                                    _3345 = spvUnpackHalf2x16(_3308.z).x;
                                    _3347 = (_3281 & 1077936128u) == 4194304u;
                                    float frontier_phi_141_pred;
                                    float frontier_phi_141_pred_1;
                                    float frontier_phi_141_pred_2;
                                    float frontier_phi_141_pred_3;
                                    float frontier_phi_141_pred_4;
                                    if (_3347)
                                    {
                                        float _3403 = (-0.0f) - _3331;
                                        float _3409 = (_3331 * _3403) - (_3326 * _3326);
                                        float _3410 = _3334 * _3326;
                                        float _3415 = (-0.0f) - _3326;
                                        float _3428 = _3341 * ((_3329 * _3326) - (_3334 * _3331));
                                        float _3431 = _3341 * (_3410 - (_3329 * _3403));
                                        float _3432 = spvUnpackHalf2x16(_3278).x - _3428;
                                        float _3433 = spvUnpackHalf2x16(_3278 >> 16u).x - (_3341 * (_3409 + 0.5f));
                                        float _3434 = spvUnpackHalf2x16(_3279).x - _3431;
                                        float _3438 = _3428 * 2.0f;
                                        float _3439 = _3341 * ((_3409 * 2.0f) + 1.0f);
                                        float _3440 = _3431 * 2.0f;
                                        float _3448 = clamp(dot(float3(_3438, _3439, _3440), float3(_1893 - _3432, _1894 - _3433, _1895 - _3434)) / dot(float3(_3438, _3439, _3440), float3(_3438, _3439, _3440)), 0.0f, 1.0f);
                                        float _3453 = (_3448 * _3438) + (_3432 - _1893);
                                        float _3455 = (_3448 * _3439) + (_3433 - _1894);
                                        float _3457 = (_3448 * _3440) + (_3434 - _1895);
                                        float _3467 = sqrt(((_3453 * _3453) + (_3455 * _3455)) + (_3457 * _3457));
                                        float _3468 = _3467 * _3467;
                                        float _3470 = (_3313 * _3313) * _3468;
                                        float _3473 = clamp(1.0f - (_3470 * _3470), 0.0f, 1.0f);
                                        float _4036;
                                        if (_3322 == 0u)
                                        {
                                            _4036 = (_3473 * _3473) * (1.0f / (max(_3468, 9.9999997473787516355514526367188e-05f) + ((_3345 * _3345) * 0.5f)));
                                        }
                                        else
                                        {
                                            float _3727 = _3467 * _3313;
                                            _4036 = max((1.0f / dot(float3(1.0f, _3727, _3727 * _3727), float3(_67_m0[_3322 + 60u].xyz))) * (1.0f - _3727), 0.0f);
                                        }
                                        float frontier_phi_141_pred_156_ladder;
                                        float frontier_phi_141_pred_156_ladder_1;
                                        float frontier_phi_141_pred_156_ladder_2;
                                        float frontier_phi_141_pred_156_ladder_3;
                                        float frontier_phi_141_pred_156_ladder_4;
                                        if (_4036 < 9.9999997473787516355514526367188e-06f)
                                        {
                                            frontier_phi_141_pred_156_ladder = _3252;
                                            frontier_phi_141_pred_156_ladder_1 = _3253;
                                            frontier_phi_141_pred_156_ladder_2 = _3254;
                                            frontier_phi_141_pred_156_ladder_3 = _3256;
                                            frontier_phi_141_pred_156_ladder_4 = _3255;
                                        }
                                        else
                                        {
                                            float _4105 = (-0.0f) - rsqrt(dot(float3(_3453, _3455, _3457), float3(_3453, _3455, _3457)));
                                            float _4106 = _3453 * _4105;
                                            float _4107 = _3455 * _4105;
                                            float _4108 = _3457 * _4105;
                                            float _4116 = clamp((clamp(dot(float3(((_3334 * _3329) - (_3331 * _3415)) * 2.0f, ((_3331 * _3329) - _3410) * 2.0f, (((_3326 * _3415) - (_3329 * _3329)) * 2.0f) + 1.0f), float3(_4106, _4107, _4108)), 0.0f, 1.0f) - _3339) / (_3336 - _3339), 0.0f, 1.0f);
                                            float _4120 = (_4116 * _4116) * (3.0f - (_4116 * 2.0f));
                                            float _4122 = (_4120 * _4120) * _4036;
                                            float frontier_phi_141_pred_156_ladder_160_ladder;
                                            float frontier_phi_141_pred_156_ladder_160_ladder_1;
                                            float frontier_phi_141_pred_156_ladder_160_ladder_2;
                                            float frontier_phi_141_pred_156_ladder_160_ladder_3;
                                            float frontier_phi_141_pred_156_ladder_160_ladder_4;
                                            if (_4122 < 9.9999997473787516355514526367188e-06f)
                                            {
                                                frontier_phi_141_pred_156_ladder_160_ladder = _3252;
                                                frontier_phi_141_pred_156_ladder_160_ladder_1 = _3253;
                                                frontier_phi_141_pred_156_ladder_160_ladder_2 = _3254;
                                                frontier_phi_141_pred_156_ladder_160_ladder_3 = _3256;
                                                frontier_phi_141_pred_156_ladder_160_ladder_4 = _3255;
                                            }
                                            else
                                            {
                                                float _4222 = min(max(_823, -0.9900000095367431640625f), 0.9900000095367431640625f);
                                                float _4228 = (min(max(dot(float3(_4106, _4107, _4108), float3(_1222, _1223, _1224)), -1.0f), 1.0f) * _4222) + 1.0f;
                                                float _4231 = (1.0f - (_4222 * _4222)) / ((_4228 * _4228) * 12.56000041961669921875f);
                                                float _4233 = clamp(1.0f - (_1386 * _850), 0.0f, 1.0f) * (max(float(_3324) * 16.0f, 1.0f) * _4122);
                                                float _4236 = ((_4233 * _3315) * _62_m0[166u].y) * _4231;
                                                float _4239 = ((_4233 * _3318) * _62_m0[166u].y) * _4231;
                                                float _4242 = ((_4233 * _3320) * _62_m0[166u].y) * _4231;
                                                float _4246 = dot(float3(_4236, _4239, _4242), float3(0.2125000059604644775390625f, 0.7153999805450439453125f, 0.07209999859333038330078125f)) * _1386;
                                                frontier_phi_141_pred_156_ladder_160_ladder = (_4236 * _3343) + _3252;
                                                frontier_phi_141_pred_156_ladder_160_ladder_1 = (_4239 * _3343) + _3253;
                                                frontier_phi_141_pred_156_ladder_160_ladder_2 = (_4242 * _3343) + _3254;
                                                frontier_phi_141_pred_156_ladder_160_ladder_3 = _4246 + _3256;
                                                frontier_phi_141_pred_156_ladder_160_ladder_4 = (((_1384 * _81_m0[20u].y) * _1386) * _4246) + _3255;
                                            }
                                            frontier_phi_141_pred_156_ladder = frontier_phi_141_pred_156_ladder_160_ladder;
                                            frontier_phi_141_pred_156_ladder_1 = frontier_phi_141_pred_156_ladder_160_ladder_1;
                                            frontier_phi_141_pred_156_ladder_2 = frontier_phi_141_pred_156_ladder_160_ladder_2;
                                            frontier_phi_141_pred_156_ladder_3 = frontier_phi_141_pred_156_ladder_160_ladder_3;
                                            frontier_phi_141_pred_156_ladder_4 = frontier_phi_141_pred_156_ladder_160_ladder_4;
                                        }
                                        frontier_phi_141_pred = frontier_phi_141_pred_156_ladder;
                                        frontier_phi_141_pred_1 = frontier_phi_141_pred_156_ladder_1;
                                        frontier_phi_141_pred_2 = frontier_phi_141_pred_156_ladder_2;
                                        frontier_phi_141_pred_3 = frontier_phi_141_pred_156_ladder_3;
                                        frontier_phi_141_pred_4 = frontier_phi_141_pred_156_ladder_4;
                                    }
                                    else
                                    {
                                        frontier_phi_141_pred = _3252;
                                        frontier_phi_141_pred_1 = _3253;
                                        frontier_phi_141_pred_2 = _3254;
                                        frontier_phi_141_pred_3 = _3256;
                                        frontier_phi_141_pred_4 = _3255;
                                    }
                                    _1395 = frontier_phi_141_pred;
                                    _1398 = frontier_phi_141_pred_1;
                                    _1401 = frontier_phi_141_pred_2;
                                    _1392 = frontier_phi_141_pred_3;
                                    _1389 = frontier_phi_141_pred_4;
                                    if (_3258 > _1913)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        _3252 = _1395;
                                        _3253 = _1398;
                                        _3254 = _1401;
                                        _3255 = _1389;
                                        _3256 = _1392;
                                        _3257 = _3258;
                                        _3259 = _3263;
                                        continue;
                                    }
                                }
                                frontier_phi_55_54_ladder_63_ladder_111_ladder = _1401;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_1 = _1398;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_2 = _1395;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_3 = _1392;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_4 = _1389;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_5 = _1386;
                                frontier_phi_55_54_ladder_63_ladder_111_ladder_6 = _1384;
                            }
                            frontier_phi_55_54_ladder_63_ladder = frontier_phi_55_54_ladder_63_ladder_111_ladder;
                            frontier_phi_55_54_ladder_63_ladder_1 = frontier_phi_55_54_ladder_63_ladder_111_ladder_1;
                            frontier_phi_55_54_ladder_63_ladder_2 = frontier_phi_55_54_ladder_63_ladder_111_ladder_2;
                            frontier_phi_55_54_ladder_63_ladder_3 = frontier_phi_55_54_ladder_63_ladder_111_ladder_3;
                            frontier_phi_55_54_ladder_63_ladder_4 = frontier_phi_55_54_ladder_63_ladder_111_ladder_4;
                            frontier_phi_55_54_ladder_63_ladder_5 = frontier_phi_55_54_ladder_63_ladder_111_ladder_5;
                            frontier_phi_55_54_ladder_63_ladder_6 = frontier_phi_55_54_ladder_63_ladder_111_ladder_6;
                        }
                        frontier_phi_55_54_ladder = frontier_phi_55_54_ladder_63_ladder;
                        frontier_phi_55_54_ladder_1 = frontier_phi_55_54_ladder_63_ladder_1;
                        frontier_phi_55_54_ladder_2 = frontier_phi_55_54_ladder_63_ladder_2;
                        frontier_phi_55_54_ladder_3 = frontier_phi_55_54_ladder_63_ladder_3;
                        frontier_phi_55_54_ladder_4 = frontier_phi_55_54_ladder_63_ladder_4;
                        frontier_phi_55_54_ladder_5 = frontier_phi_55_54_ladder_63_ladder_5;
                        frontier_phi_55_54_ladder_6 = frontier_phi_55_54_ladder_63_ladder_6;
                    }
                    _1383 = frontier_phi_55_54_ladder_6;
                    _1385 = frontier_phi_55_54_ladder_5;
                    _1387 = frontier_phi_55_54_ladder_4;
                    _1390 = frontier_phi_55_54_ladder_3;
                    _1393 = frontier_phi_55_54_ladder_2;
                    _1396 = frontier_phi_55_54_ladder_1;
                    _1399 = frontier_phi_55_54_ladder;
                }
                else
                {
                    _1383 = 0.0f;
                    _1385 = _1038;
                    _1387 = 0.0f;
                    _1390 = 0.0f;
                    _1393 = 0.0f;
                    _1396 = 0.0f;
                    _1399 = 0.0f;
                }
                float _1412 = _62_m0[8u].x + _370;
                float _1413 = _62_m0[8u].y + _371;
                float _1414 = _62_m0[8u].z + _374;
                uint _1425 = uint((_12[503u].Load(int3(uint2(0u, 0u), 0u)).x * 255.0f) + 0.5f);
                float _1431 = _62_m0[8u].x - _1412;
                float _1432 = _62_m0[8u].y - _1413;
                float _1433 = _62_m0[8u].z - _1414;
                uint _1434 = _1425 + 422u;
                uint _1443 = ((asuint(_62_m0[_1434]).w >> 24u) + 8u) + 0u;
                uint4 _1446 = asuint(_62_m0[_1434]);
                uint _1450 = _1446.y;
                uint _1451 = _1446.x;
                float _1452 = _1222 * _1222;
                float _1453 = _1223 * _1223;
                float _1454 = _1224 * _1224;
                float _1458 = float(_1222 > (-0.0f));
                float _1459 = (_1223 > (-0.0f)) ? 3.0f : 2.0f;
                float _1460 = (_1224 > (-0.0f)) ? 5.0f : 4.0f;
                float4 _1478 = _12[584u].SampleLevel(_94, float2((_1412 - _62_m0[111u].x) * _62_m0[111u].z, 1.0f - ((_1414 - _62_m0[111u].y) * _62_m0[111u].w)), 0.0f);
                float _1495 = clamp((((_62_m0[112u].x - _1413) - _62_m0[112u].w) + (_62_m0[112u].y * _1478.x)) / _62_m0[112u].z, 0.0f, 1.0f) * clamp(_1478.y, 0.0f, 1.0f);
                float _1507 = (1.0f - _1223) * 0.5f;
                float _1916;
                if (asuint(_62_m0[119u]).x == 0u)
                {
                    _1916 = ((_62_m0[166u].w - _62_m0[166u].z) * _1495) + _62_m0[166u].z;
                }
                else
                {
                    _1916 = _12[586u].SampleLevel(_94, float2((_1412 - _62_m0[118u].x) * _62_m0[118u].z, (_1414 - _62_m0[118u].y) * _62_m0[118u].w), 0.0f).w;
                }
                precise float _1920 = exp2(log2(_1916 * _1507)) + (-1.0f);
                precise float _1921 = _1920 * _62_m0[159u].y;
                float _1922 = _1921 + 1.0f;
                uint _1923 = _1451 * 6u;
                uint _1924 = _1923 + 457u;
                uint _1931 = _1923 + 458u;
                uint _1938 = _1923 + 459u;
                uint _1945 = _1923 + 455u;
                uint _1952 = _1923 + 456u;
                uint _2118;
                if ((_1446.w & 2u) == 0u)
                {
                    _2118 = 0u;
                }
                else
                {
                    _2118 = uint(min(max(floor(log2(max(abs((_1431 * 2.0f) * _62_m0[_1945].x), max(abs((_62_m0[_1945].y * 2.0f) * (_62_m0[158u].w + _1432)), abs((_1433 * 2.0f) * _62_m0[_1945].z))) / _62_m0[_1945].w)) + 1.0f, 0.0f), float(_1450)));
                }
                float _2408;
                if (_2118 == _1450)
                {
                    _2408 = _1922;
                }
                else
                {
                    uint _2503 = (_1450 == 1u) ? 0u : _2118;
                    float _2506 = float(1u << (_2503 & 31u));
                    uint _2508 = (_2503 + _1451) * 6u;
                    uint _2509 = _2508 + 454u;
                    float _2526 = (_62_m0[_1945].x / _2506) * _62_m0[_1924].x;
                    float _2527 = _2526 * (_1412 - _62_m0[_2509].x);
                    float _2528 = (_62_m0[_1945].y / _2506) * _62_m0[_1924].y;
                    float _2529 = _2528 * (_1413 - _62_m0[_2509].y);
                    float _2530 = (_62_m0[_1945].z / _2506) * _62_m0[_1924].z;
                    float _2531 = _2530 * (_1414 - _62_m0[_2509].z);
                    float _2532 = _2526 * 0.5f;
                    float _2533 = _2528 * 0.5f;
                    float _2534 = _2530 * 0.5f;
                    float frontier_phi_93_94_ladder;
                    if (((_2527 < ((-0.0f) - _2532)) || (_2529 < ((-0.0f) - _2533))) || (_2531 < ((-0.0f) - _2534)))
                    {
                        frontier_phi_93_94_ladder = _1922;
                    }
                    else
                    {
                        float frontier_phi_93_94_ladder_107_ladder;
                        if (((_2527 > ((_62_m0[_1938].x + _62_m0[_1931].x) + _2532)) || (_2529 > ((_62_m0[_1938].y + _62_m0[_1931].y) + _2533))) || (_2531 > ((_62_m0[_1938].z + _62_m0[_1931].z) + _2534)))
                        {
                            frontier_phi_93_94_ladder_107_ladder = _1922;
                        }
                        else
                        {
                            float _3018 = min(max(_2527, _62_m0[_1931].x), _62_m0[_1938].x) + _62_m0[_2508 + 456u].x;
                            float _3019 = min(max(_2529, _62_m0[_1931].y), _62_m0[_1938].y) + _62_m0[_1952].y;
                            float _3020 = min(max(_2531, _62_m0[_1931].z), _62_m0[_1938].z) + _62_m0[_1952].z;
                            frontier_phi_93_94_ladder_107_ladder = (_62_m0[159u].y * (dot(float3(_1452, _1453, _1454), float3(_16[NonUniformResourceIndex(_1443)].SampleLevel(_95, float3(_3018, _3019, _3020 + (_62_m0[_1924].z * _1458)), 0.0f).x, _16[NonUniformResourceIndex(_1443)].SampleLevel(_95, float3(_3018, _3019, _3020 + (_62_m0[_1924].z * _1459)), 0.0f).x, _16[NonUniformResourceIndex(_1443)].SampleLevel(_95, float3(_3018, _3019, _3020 + (_62_m0[_1924].z * _1460)), 0.0f).x)) + (-1.0f))) + 1.0f;
                        }
                        frontier_phi_93_94_ladder = frontier_phi_93_94_ladder_107_ladder;
                    }
                    _2408 = frontier_phi_93_94_ladder;
                }
                float _2420 = clamp((_1413 - _62_m0[165u].x) / (_62_m0[165u].y - _62_m0[165u].x), 0.0f, 1.0f);
                float _2424 = (_2420 * _2420) * (3.0f - (_2420 * 2.0f));
                float _2443 = ((_62_m0[159u].y * (min(_2408, ((_2424 * (1.0f - _62_m0[165u].w)) + _62_m0[165u].w) * exp2(((_2424 * (1.0f - _62_m0[165u].z)) + _62_m0[165u].z) * log2(_1507))) + (-1.0f))) + 1.0f) * (1.0f - _1495);
                uint _2444 = _1425 + 198u;
                uint _2452 = ((asuint(_62_m0[_2444]).w >> 24u) + 24u) + 0u;
                uint4 _2455 = asuint(_62_m0[_2444]);
                uint _2459 = _2455.y;
                uint _2460 = _2455.x;
                float4 _2479 = _12[584u].SampleLevel(_94, float2((_1412 - _62_m0[111u].x) * _62_m0[111u].z, 1.0f - ((_1414 - _62_m0[111u].y) * _62_m0[111u].w)), 0.0f);
                float _2496 = clamp((((_62_m0[112u].x - _1413) - _62_m0[112u].w) + (_62_m0[112u].y * _2479.x)) / _62_m0[112u].z, 0.0f, 1.0f) * clamp(_2479.y, 0.0f, 1.0f);
                float _2788;
                float _2789;
                float _2790;
                if (asuint(_62_m0[119u]).x == 0u)
                {
                    float _2770 = 0.3183098733425140380859375f / _62_m0[59u].w;
                    float _2772 = (((_62_m0[164u].y - _62_m0[160u].x) * _2496) + _62_m0[160u].x) * _2770;
                    float _2773 = (((_62_m0[164u].z - _62_m0[160u].y) * _2496) + _62_m0[160u].y) * _2770;
                    float _2774 = (((_62_m0[164u].w - _62_m0[160u].z) * _2496) + _62_m0[160u].z) * _2770;
                    float _2775 = clamp(_1507, 0.0f, 1.0f);
                    _2788 = (_2772 - (_2772 * _2775)) * _62_m0[159u].z;
                    _2789 = (_2773 - (_2773 * _2775)) * _62_m0[159u].z;
                    _2790 = (_2774 - (_2774 * _2775)) * _62_m0[159u].z;
                }
                else
                {
                    _2788 = 0.0f;
                    _2789 = 0.0f;
                    _2790 = 0.0f;
                }
                uint _2791 = _2460 * 6u;
                uint _2792 = _2791 + 233u;
                uint _2799 = _2791 + 234u;
                uint _2806 = _2791 + 235u;
                uint _2813 = _2791 + 231u;
                uint _2820 = _2791 + 232u;
                uint _2985;
                if ((_2455.w & 2u) == 0u)
                {
                    _2985 = 0u;
                }
                else
                {
                    _2985 = uint(min(max(floor(log2(max(abs((_1431 * 2.0f) * _62_m0[_2813].x), max(abs((_62_m0[_2813].y * 2.0f) * (_62_m0[158u].w + _1432)), abs((_1433 * 2.0f) * _62_m0[_2813].z))) / _62_m0[_2813].w)) + 1.0f, float(asuint(_62_m0[85u]).y)), float(_2459)));
                }
                float _3109;
                float _3111;
                float _3113;
                if (_2985 == _2459)
                {
                    _3109 = _2788;
                    _3111 = _2789;
                    _3113 = _2790;
                }
                else
                {
                    uint _3212 = (_2459 == 1u) ? 0u : _2985;
                    float _3215 = float(1u << (_3212 & 31u));
                    uint _3217 = (_3212 + _2460) * 6u;
                    uint _3218 = _3217 + 230u;
                    float _3235 = (_62_m0[_2813].x / _3215) * _62_m0[_2792].x;
                    float _3236 = _3235 * (_1412 - _62_m0[_3218].x);
                    float _3237 = (_62_m0[_2813].y / _3215) * _62_m0[_2792].y;
                    float _3238 = _3237 * (_1413 - _62_m0[_3218].y);
                    float _3239 = (_62_m0[_2813].z / _3215) * _62_m0[_2792].z;
                    float _3240 = _3239 * (_1414 - _62_m0[_3218].z);
                    float _3241 = _3235 * 0.5f;
                    float _3242 = _3237 * 0.5f;
                    float _3243 = _3239 * 0.5f;
                    float frontier_phi_129_130_ladder;
                    float frontier_phi_129_130_ladder_1;
                    float frontier_phi_129_130_ladder_2;
                    if (((_3236 < ((-0.0f) - _3241)) || (_3238 < ((-0.0f) - _3242))) || (_3240 < ((-0.0f) - _3243)))
                    {
                        frontier_phi_129_130_ladder = _2790;
                        frontier_phi_129_130_ladder_1 = _2789;
                        frontier_phi_129_130_ladder_2 = _2788;
                    }
                    else
                    {
                        float frontier_phi_129_130_ladder_139_ladder;
                        float frontier_phi_129_130_ladder_139_ladder_1;
                        float frontier_phi_129_130_ladder_139_ladder_2;
                        if (((_3236 > ((_62_m0[_2806].x + _62_m0[_2799].x) + _3241)) || (_3238 > ((_62_m0[_2806].y + _62_m0[_2799].y) + _3242))) || (_3240 > ((_62_m0[_2806].z + _62_m0[_2799].z) + _3243)))
                        {
                            frontier_phi_129_130_ladder_139_ladder = _2790;
                            frontier_phi_129_130_ladder_139_ladder_1 = _2789;
                            frontier_phi_129_130_ladder_139_ladder_2 = _2788;
                        }
                        else
                        {
                            float _3674 = min(max(_3236, _62_m0[_2799].x), _62_m0[_2806].x) + _62_m0[_3217 + 232u].x;
                            float _3675 = min(max(_3238, _62_m0[_2799].y), _62_m0[_2806].y) + _62_m0[_2820].y;
                            float _3676 = min(max(_3240, _62_m0[_2799].z), _62_m0[_2806].z) + _62_m0[_2820].z;
                            float4 _3687 = _16[NonUniformResourceIndex(_2452)].SampleLevel(_95, float3(_3674, _3675, _3676 + (_62_m0[_2792].z * _1458)), 0.0f);
                            float4 _3692 = _16[NonUniformResourceIndex(_2452)].SampleLevel(_95, float3(_3674, _3675, _3676 + (_62_m0[_2792].z * _1459)), 0.0f);
                            float4 _3697 = _16[NonUniformResourceIndex(_2452)].SampleLevel(_95, float3(_3674, _3675, _3676 + (_62_m0[_2792].z * _1460)), 0.0f);
                            frontier_phi_129_130_ladder_139_ladder = (((_3692.z * _1453) + (_3687.z * _1452)) + (_3697.z * _1454)) * _62_m0[159u].z;
                            frontier_phi_129_130_ladder_139_ladder_1 = (((_3692.y * _1453) + (_3687.y * _1452)) + (_3697.y * _1454)) * _62_m0[159u].z;
                            frontier_phi_129_130_ladder_139_ladder_2 = (((_3692.x * _1453) + (_3687.x * _1452)) + (_3697.x * _1454)) * _62_m0[159u].z;
                        }
                        frontier_phi_129_130_ladder = frontier_phi_129_130_ladder_139_ladder;
                        frontier_phi_129_130_ladder_1 = frontier_phi_129_130_ladder_139_ladder_1;
                        frontier_phi_129_130_ladder_2 = frontier_phi_129_130_ladder_139_ladder_2;
                    }
                    _3109 = frontier_phi_129_130_ladder_2;
                    _3111 = frontier_phi_129_130_ladder_1;
                    _3113 = frontier_phi_129_130_ladder;
                }
                float _3134 = ((((_62_m0[84u].x * (_2443 + (-1.0f))) + 1.0f) * _62_m0[76u].x) * ((_62_m0[84u].y * (_1038 + (-1.0f))) + 1.0f)) * ((1.0f - _2496) * _62_m0[59u].w);
                float4 _3141 = _42.SampleLevel(_98, float3((-0.0f) - _1222, (-0.0f) - _1223, (-0.0f) - _1224), 0.0f);
                float _3147 = _62_m0[59u].y * _2443;
                float _3171 = min(max(_823, -0.9900000095367431640625f), 0.9900000095367431640625f);
                float _3177 = (min(max(dot(float3((-0.0f) - _67_m0[0u].x, (-0.0f) - _67_m0[0u].y, (-0.0f) - _67_m0[0u].z), float3(_1222, _1223, _1224)), -1.0f), 1.0f) * _3171) + 1.0f;
                float _3182 = ((1.0f - (_3171 * _3171)) / ((_3177 * _3177) * 12.56000041961669921875f)) * _62_m0[166u].y;
                float _3183 = _3182 * _67_m0[1u].x;
                float _3184 = _3182 * _67_m0[1u].y;
                float _3185 = _3182 * _67_m0[1u].z;
                float _3189 = dot(float3(_3183, _3184, _3185), float3(0.2125000059604644775390625f, 0.7153999805450439453125f, 0.07209999859333038330078125f)) * _1385;
                float _1235 = (((_1383 * _81_m0[20u].y) * _1385) * _3189) + _1387;
                float _3193 = _3189 + _1390;
                float _1237 = (((((_3147 * _3141.x) + (_3134 * _3109)) * 0.0796178281307220458984375f) + _1393) + (_3183 * _1038)) * _815;
                float _1239 = (((((_3147 * _3141.y) + (_3134 * _3111)) * 0.0796178281307220458984375f) + _1396) + (_3184 * _1038)) * _816;
                float _1241 = (((((_3147 * _3141.z) + (_3134 * _3113)) * 0.0796178281307220458984375f) + _1399) + (_3185 * _1038)) * _817;
                float frontier_phi_46_129_ladder;
                float frontier_phi_46_129_ladder_1;
                float frontier_phi_46_129_ladder_2;
                float frontier_phi_46_129_ladder_3;
                if (_3193 > 0.0f)
                {
                    frontier_phi_46_129_ladder = _1241;
                    frontier_phi_46_129_ladder_1 = _1239;
                    frontier_phi_46_129_ladder_2 = _1237;
                    frontier_phi_46_129_ladder_3 = _1235 / _3193;
                }
                else
                {
                    frontier_phi_46_129_ladder = _1241;
                    frontier_phi_46_129_ladder_1 = _1239;
                    frontier_phi_46_129_ladder_2 = _1237;
                    frontier_phi_46_129_ladder_3 = _1235;
                }
                _1233 = frontier_phi_46_129_ladder_3;
                _1236 = frontier_phi_46_129_ladder_2;
                _1238 = frontier_phi_46_129_ladder_1;
                _1240 = frontier_phi_46_129_ladder;
            }
            else
            {
                _1233 = 0.0f;
                _1236 = 0.0f;
                _1238 = 0.0f;
                _1240 = 0.0f;
            }
            float _1245 = _72_m0[11u].x * _1236;
            float _1246 = _72_m0[11u].x * _1238;
            float _1247 = _72_m0[11u].x * _1240;
            float _1508;
            float _1510;
            float _1512;
            float _1514;
            float _1516;
            float _1518;
            if (_168 == 0u)
            {
                _1508 = _1233;
                _1510 = _1036;
                _1512 = _1245;
                _1514 = _1246;
                _1516 = _1247;
                _1518 = _818;
            }
            else
            {
                float _1568 = clamp((dot(float3(_1236, _1238, _1240), float3(0.2125000059604644775390625f, 0.7153999805450439453125f, 0.07209999859333038330078125f)) - _81_m0[7u].z) / (_81_m0[7u].w - _81_m0[7u].z), 0.0f, 1.0f) * _161;
                float _1509;
                if (_142)
                {
                    _1509 = (_81_m0[4u].y * (_163 - _1233)) + _1233;
                }
                else
                {
                    _1509 = _1233;
                }
                _1508 = _1509;
                _1510 = (_1568 * (_165 - _1036)) + _1036;
                _1512 = (_1568 * (_153 - _1245)) + _1245;
                _1514 = (_1568 * (_155 - _1246)) + _1246;
                _1516 = (_1568 * (_157 - _1247)) + _1247;
                _1518 = (_1568 * (_159 - _818)) + _818;
            }
            float _1521 = min(max(_1512, 0.0f), 65504.0f);
            float _1527 = ((asuint(_1521) & 2139095040u) == 2139095040u) ? 0.0f : _1521;
            float _1529 = min(max(_1514, 0.0f), 65504.0f);
            float _1533 = ((asuint(_1529) & 2139095040u) == 2139095040u) ? 0.0f : _1529;
            float _1535 = min(max(_1516, 0.0f), 65504.0f);
            float _1539 = ((asuint(_1535) & 2139095040u) == 2139095040u) ? 0.0f : _1535;
            float _1541 = min(max(_1518, 0.0f), 65504.0f);
            float _1545 = ((asuint(_1541) & 2139095040u) == 2139095040u) ? 0.0f : _1541;
            _54[uint3(_107, _110, _124)] = float4(_1527, _1533, _1539, _1545);
            if (!(asuint(_81_m0[19u]).x == 0u))
            {
                _56[uint3(_107, _110, _124)] = float4(_1527, _1533, _1539, _1545);
            }
            float _1834 = min(max(_142 ? _1508 : 0.0f, 0.0f), 65504.0f);
            float _1838 = ((asuint(_1834) & 2139095040u) == 2139095040u) ? 0.0f : _1834;
            float _1840 = min(max(_1510, 0.0f), 65504.0f);
            _55[uint3(_107, _110, _124)] = float4(_1838, ((asuint(_1840) & 2139095040u) == 2139095040u) ? 1.0f : _1840, _1838, _1838);
            break;
        }
        else
        {
            break;
        }
    }
}

[numthreads(4, 4, 4)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
