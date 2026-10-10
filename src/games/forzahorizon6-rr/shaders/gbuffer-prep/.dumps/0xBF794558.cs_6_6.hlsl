cbuffer _19_21 : register(b0, space36)
{
    float4 _21_m0[50] : packoffset(c0);
};

Texture2D<uint4> _8 : register(t0, space36);
Texture2D<float4> _12 : register(t1, space36);
RWTexture2D<float4> _15 : register(u11, space36);

static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

void comp_main()
{
    uint _32 = gl_GlobalInvocationID.x >> 1u;
    uint _35 = gl_GlobalInvocationID.y << 1u;
    uint _45 = ((_32 & 2u) | (gl_GlobalInvocationID.x & 4294967289u)) | (_35 & 4u);
    uint _47 = ((gl_GlobalInvocationID.y & 4294967292u) | (_32 & 1u)) | (_35 & 2u);
    uint _56 = (_45 * 5u) + _47;
    uint _57 = _56 + 37u;
    uint _62 = ((_57 >> 8u) ^ _57) + 1759714724u;
    uint _66 = ((_62 << 8u) ^ _62) * 458671337u;
    uint _75 = _56 + 38u;
    uint _79 = ((_75 >> 8u) ^ _75) + 1759714724u;
    uint _82 = ((_79 << 8u) ^ _79) * 458671337u;
    uint _94 = uint(((float((_66 & 16777215u) ^ (_66 >> 8u)) * 5.9604644775390625e-08f) + float(_45)) * _21_m0[25u].x);
    uint _95 = uint(((float((_82 & 16777215u) ^ (_82 >> 8u)) * 5.9604644775390625e-08f) + float(_47)) * _21_m0[25u].y);
    float4 _97 = _12.Load(int3(uint2(_94, _95), 0u));
    float _100 = _97.x;
    float _116 = asfloat(_8.Load(int3(uint2(_94, _95), 0u)).x);
    _15[uint2(_45, _47)] = float4(_116, (_100 <= 0.0f) ? 0.0f : (_21_m0[0u].w / (_100 - _21_m0[0u].z)), _116, _116);
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
