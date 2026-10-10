cbuffer _21_23 : register(b0, space36)
{
    float4 _23_m0[50] : packoffset(c0);
};

Texture2D<uint4> _8 : register(t0, space36);
Texture2D<float4> _12 : register(t1, space36);
Texture2D<float4> _13 : register(t22, space36);
Texture2D<float4> _14 : register(t23, space36);
RWTexture2D<float4> _17 : register(u0, space36);

static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

void comp_main()
{
    float4 _40 = _12.Load(int3(uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), 0u));
    float _43 = _40.x;
    uint4 _52 = _8.Load(int3(uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), 0u));
    uint _54 = _52.x;
    float _66 = (float((_54 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
    float _68 = (float(_54 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
    float _74 = (1.0f - abs(_66)) - abs(_68);
    float _77 = clamp((-0.0f) - _74, 0.0f, 1.0f);
    float _78 = (-0.0f) - _77;
    float _83 = ((_66 >= 0.0f) ? _78 : _77) + _66;
    float _84 = ((_68 >= 0.0f) ? _78 : _77) + _68;
    float _89 = rsqrt(dot(float3(_83, _84, _74), float3(_83, _84, _74)));
    float _90 = _83 * _89;
    float _91 = _84 * _89;
    float _92 = _89 * _74;
    float _96 = rsqrt(dot(float3(_90, _91, _92), float3(_90, _91, _92)));
    if (((_43 <= 0.0f) ? 0.0f : (_23_m0[0u].w / (_43 - _23_m0[0u].z))) > 9.9999999747524270787835121154785e-07f)
    {
        uint4 _105 = asuint(_23_m0[20u]);
        uint _108 = gl_GlobalInvocationID.x / _105.x;
        uint _109 = gl_GlobalInvocationID.y / _105.y;
        float4 _111 = _13.Load(int3(uint2(_108, _109), 0u));
        float _113 = _111.x;
        float _114 = _111.y;
        float4 _118 = _14.Load(int3(uint2(_108, _109), 0u));
        float _127 = dot(float3(_90 * _96, _91 * _96, _96 * _92), float3(_118.xyz)) + _118.w;
        _17[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = float4((_113 - _114) + _127, _127 + _114, (((-0.0f) - _114) - _113) + _127, _111.w);
    }
    else
    {
        _17[uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
