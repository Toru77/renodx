cbuffer _21_23 : register(b0, space0)
{
    float4 _23_m0[1] : packoffset(c0);
};

Buffer<float4> _8 : register(t0, space0);
Buffer<uint4> _12 : register(t1, space0);
Buffer<uint4> _13 : register(t3, space0);
RWBuffer<float4> _16 : register(u0, space0);
RWBuffer<float4> _17 : register(u1, space0);

static uint3 gl_WorkGroupID;
static uint3 gl_LocalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint3 gl_LocalInvocationID : SV_GroupThreadID;
};

groupshared float _27[1152];

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
    uint4 _52 = _13.Load((gl_WorkGroupID.x * 2u) + 1u);
    uint _53 = _52.x;
    float _62 = float(gl_LocalInvocationID.x < (_53 & 127u));
    uint _65 = _13.Load(gl_WorkGroupID.x * 2u).x + gl_LocalInvocationID.x;
    uint _90;
    uint _91;
    uint _92;
    uint _93;
    if (_65 < asuint(_23_m0[0u]).x)
    {
        _90 = _12.Load((_65 * 4u) + 3u).x;
        _91 = _12.Load((_65 * 4u) + 2u).x;
        _92 = _12.Load((_65 * 4u) + 1u).x;
        _93 = _12.Load(_65 * 4u).x;
    }
    else
    {
        _90 = 0u;
        _91 = 0u;
        _92 = 0u;
        _93 = 0u;
    }
    float4 _95 = _8.Load(_93);
    float2 _103 = spvUnpackHalf2x16(_92 >> 16u);
    float _104 = _103.x;
    float2 _107 = spvUnpackHalf2x16(_92 & 65535u);
    float _108 = _107.x;
    float2 _110 = spvUnpackHalf2x16(_91 >> 16u);
    float _111 = _110.x;
    float2 _113 = spvUnpackHalf2x16(_91 & 65535u);
    float _114 = _113.x;
    float2 _116 = spvUnpackHalf2x16(_90 >> 16u);
    float _117 = _116.x;
    float2 _119 = spvUnpackHalf2x16(_90 & 65535u);
    float _120 = _119.x;
    float _121 = _95.x * _62;
    float _122 = _95.y * _62;
    float _123 = _95.z * _62;
    _27[0u + ((0u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _121 * _104;
    _27[1u + ((0u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _122 * _104;
    _27[2u + ((0u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _123 * _104;
    _27[0u + ((1u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _121 * _108;
    _27[1u + ((1u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _122 * _108;
    _27[2u + ((1u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _123 * _108;
    _27[0u + ((2u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _111 * _121;
    _27[1u + ((2u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _111 * _122;
    _27[2u + ((2u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _111 * _123;
    _27[0u + ((3u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _114 * _121;
    _27[1u + ((3u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _114 * _122;
    _27[2u + ((3u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _114 * _123;
    _27[0u + ((4u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _117 * _121;
    _27[1u + ((4u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _117 * _122;
    _27[2u + ((4u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _117 * _123;
    _27[0u + ((5u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _120 * _121;
    _27[1u + ((5u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _120 * _122;
    _27[2u + ((5u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _120 * _123;
    GroupMemoryBarrierWithGroupSync();
    if (gl_LocalInvocationID.x == 0u)
    {
        float _41[6];
        float _42[6];
        float _43[6];
        _41[0u] = _27[0u];
        _42[0u] = _27[1u];
        _43[0u] = _27[2u];
        _41[1u] = _27[3u];
        _42[1u] = _27[4u];
        _43[1u] = _27[5u];
        _41[2u] = _27[6u];
        _42[2u] = _27[7u];
        _43[2u] = _27[8u];
        _41[3u] = _27[9u];
        _42[3u] = _27[10u];
        _43[3u] = _27[11u];
        _41[4u] = _27[12u];
        _42[4u] = _27[13u];
        _43[4u] = _27[14u];
        _41[5u] = _27[15u];
        _42[5u] = _27[16u];
        _43[5u] = _27[17u];
        float _300;
        float _302;
        float _304;
        float _306;
        float _308;
        float _310;
        float _312;
        float _314;
        float _316;
        float _318;
        float _320;
        float _322;
        float _324;
        float _326;
        float _328;
        float _330;
        float _332;
        float _334;
        uint _336;
        _300 = _27[17u];
        _302 = _27[16u];
        _304 = _27[15u];
        _306 = _27[14u];
        _308 = _27[13u];
        _310 = _27[12u];
        _312 = _27[11u];
        _314 = _27[10u];
        _316 = _27[9u];
        _318 = _27[8u];
        _320 = _27[7u];
        _322 = _27[6u];
        _324 = _27[5u];
        _326 = _27[4u];
        _328 = _27[3u];
        _330 = _27[2u];
        _332 = _27[1u];
        _334 = _27[0u];
        _336 = 1u;
        float _331;
        float _333;
        float _335;
        for (;;)
        {
            _335 = _27[0u + ((0u + (_336 * 6u)) * 3u)] + _334;
            _333 = _27[1u + ((0u + (_336 * 6u)) * 3u)] + _332;
            _331 = _27[2u + ((0u + (_336 * 6u)) * 3u)] + _330;
            _41[0u] = _335;
            _42[0u] = _333;
            _43[0u] = _331;
            float _329 = _27[0u + ((1u + (_336 * 6u)) * 3u)] + _328;
            float _327 = _27[1u + ((1u + (_336 * 6u)) * 3u)] + _326;
            float _325 = _27[2u + ((1u + (_336 * 6u)) * 3u)] + _324;
            _41[1u] = _329;
            _42[1u] = _327;
            _43[1u] = _325;
            float _323 = _27[0u + ((2u + (_336 * 6u)) * 3u)] + _322;
            float _321 = _27[1u + ((2u + (_336 * 6u)) * 3u)] + _320;
            float _319 = _27[2u + ((2u + (_336 * 6u)) * 3u)] + _318;
            _41[2u] = _323;
            _42[2u] = _321;
            _43[2u] = _319;
            float _317 = _27[0u + ((3u + (_336 * 6u)) * 3u)] + _316;
            float _315 = _27[1u + ((3u + (_336 * 6u)) * 3u)] + _314;
            float _313 = _27[2u + ((3u + (_336 * 6u)) * 3u)] + _312;
            _41[3u] = _317;
            _42[3u] = _315;
            _43[3u] = _313;
            float _311 = _27[0u + ((4u + (_336 * 6u)) * 3u)] + _310;
            float _309 = _27[1u + ((4u + (_336 * 6u)) * 3u)] + _308;
            float _307 = _27[2u + ((4u + (_336 * 6u)) * 3u)] + _306;
            _41[4u] = _311;
            _42[4u] = _309;
            _43[4u] = _307;
            float _305 = _27[0u + ((5u + (_336 * 6u)) * 3u)] + _304;
            float _303 = _27[1u + ((5u + (_336 * 6u)) * 3u)] + _302;
            float _301 = _27[2u + ((5u + (_336 * 6u)) * 3u)] + _300;
            _41[5u] = _305;
            _42[5u] = _303;
            _43[5u] = _301;
            uint _337 = _336 + 1u;
            if (_337 == 64u)
            {
                break;
            }
            else
            {
                _300 = _301;
                _302 = _303;
                _304 = _305;
                _306 = _307;
                _308 = _309;
                _310 = _311;
                _312 = _313;
                _314 = _315;
                _316 = _317;
                _318 = _319;
                _320 = _321;
                _322 = _323;
                _324 = _325;
                _326 = _327;
                _328 = _329;
                _330 = _331;
                _332 = _333;
                _334 = _335;
                _336 = _337;
            }
        }
        uint _448 = (_53 >> 8u) * 6u;
        if ((_53 & 128u) == 0u)
        {
            float _450;
            float _452;
            float _454;
            uint _456;
            _450 = _331;
            _452 = _333;
            _454 = _335;
            _456 = 0u;
            uint _457;
            for (;;)
            {
                _16[_456 + _448] = float4(_454, _452, _450, _454);
                _457 = _456 + 1u;
                if (_457 == 6u)
                {
                    break;
                }
                else
                {
                    _450 = _43[_457];
                    _452 = _42[_457];
                    _454 = _41[_457];
                    _456 = _457;
                    continue;
                }
            }
        }
        else
        {
            float _462;
            float _464;
            float _466;
            uint _468;
            _462 = _331;
            _464 = _333;
            _466 = _335;
            _468 = 0u;
            uint _469;
            for (;;)
            {
                float _470 = _466 * 0.000318309874273836612701416015625f;
                _17[_468 + _448] = float4(_470, _464 * 0.000318309874273836612701416015625f, _462 * 0.000318309874273836612701416015625f, _470);
                _469 = _468 + 1u;
                if (_469 == 6u)
                {
                    break;
                }
                else
                {
                    _462 = _43[_469];
                    _464 = _42[_469];
                    _466 = _41[_469];
                    _468 = _469;
                    continue;
                }
            }
        }
    }
}

[numthreads(64, 1, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_WorkGroupID = stage_input.gl_WorkGroupID;
    gl_LocalInvocationID = stage_input.gl_LocalInvocationID;
    comp_main();
}
