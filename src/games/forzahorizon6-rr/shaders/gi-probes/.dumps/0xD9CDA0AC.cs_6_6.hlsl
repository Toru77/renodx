cbuffer _17_19 : register(b0, space0)
{
    float4 _19_m0[1] : packoffset(c0);
};

Buffer<float4> _8 : register(t0, space0);
Buffer<float4> _9 : register(t1, space0);
RWBuffer<float4> _12 : register(u0, space0);

static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

void comp_main()
{
    uint _33 = gl_GlobalInvocationID.x * 6u;
    uint _34;
    _34 = 0u;
    float _30[6];
    float _31[6];
    float _32[6];
    for (;;)
    {
        uint _36 = _34 + _33;
        float4 _38 = _8.Load(_36);
        float4 _43 = _9.Load(_36);
        _30[_34] = (_19_m0[0u].x * _43.x) + _38.x;
        _31[_34] = (_19_m0[0u].x * _43.y) + _38.y;
        _32[_34] = (_19_m0[0u].x * _43.z) + _38.z;
        uint _35 = _34 + 1u;
        if (_35 == 6u)
        {
            break;
        }
        else
        {
            _34 = _35;
        }
    }
    uint _63;
    _63 = 0u;
    for (;;)
    {
        _12[_63 + _33] = float4(_30[_63], _31[_63], _32[_63], _30[_63]);
        uint _64 = _63 + 1u;
        if (_64 == 6u)
        {
            break;
        }
        else
        {
            _63 = _64;
        }
    }
}

[numthreads(64, 1, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
