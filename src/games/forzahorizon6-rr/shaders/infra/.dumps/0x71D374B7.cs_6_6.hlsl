cbuffer _24_26 : register(b3, space0)
{
    float4 _26_m0[69] : packoffset(c0);
};

RWBuffer<uint> _8 : register(u0, space0);
RWBuffer<uint4> _11 : register(u1, space0);
RWBuffer<uint> _12 : register(u2, space0);
RWBuffer<uint4> _13 : register(u3, space0);
RWBuffer<uint4> _14 : register(u4, space0);
RWBuffer<uint4> _15 : register(u5, space0);
RWBuffer<uint> _16 : register(u6, space0);
RWBuffer<uint4> _17 : register(u7, space0);
RWBuffer<uint> _18 : register(u8, space0);
RWBuffer<uint4> _19 : register(u9, space0);

void comp_main()
{
    uint4 _30 = _8[0u].xxxx;
    uint _31 = _30.x;
    uint _32 = _31 + 63u;
    uint _34 = _32 >> 6u;
    if (_32 > 4194303u)
    {
        _11[0u] = uint4(64u, 64u, 64u, 64u);
        _11[1u] = ((_34 + 63u) >> 6u).xxxx;
        _11[2u] = uint4(1u, 1u, 1u, 1u);
    }
    else
    {
        _11[0u] = _34.xxxx;
        _11[1u] = uint4(1u, 1u, 1u, 1u);
        _11[2u] = uint4(1u, 1u, 1u, 1u);
    }
    _8[0u] = uint4(0u, 0u, 0u, 0u).x;
    _8[1u] = _31.x;
    uint4 _58 = _12[0u].xxxx;
    uint _59 = _58.x;
    uint _60 = _59 + 63u;
    uint _61 = _60 >> 6u;
    if (_60 > 4194303u)
    {
        _13[0u] = uint4(64u, 64u, 64u, 64u);
        _13[1u] = ((_61 + 63u) >> 6u).xxxx;
        _13[2u] = uint4(1u, 1u, 1u, 1u);
    }
    else
    {
        _13[0u] = _61.xxxx;
        _13[1u] = uint4(1u, 1u, 1u, 1u);
        _13[2u] = uint4(1u, 1u, 1u, 1u);
    }
    _12[0u] = uint4(0u, 0u, 0u, 0u).x;
    _12[1u] = _59.x;
    uint4 _77 = _16[0u].xxxx;
    uint _78 = _77.x;
    uint _79 = _78 + 63u;
    uint _80 = _79 >> 6u;
    if (_79 > 4194303u)
    {
        _17[0u] = uint4(64u, 64u, 64u, 64u);
        _17[1u] = ((_80 + 63u) >> 6u).xxxx;
        _17[2u] = uint4(1u, 1u, 1u, 1u);
    }
    else
    {
        _17[0u] = _80.xxxx;
        _17[1u] = uint4(1u, 1u, 1u, 1u);
        _17[2u] = uint4(1u, 1u, 1u, 1u);
    }
    _16[0u] = uint4(0u, 0u, 0u, 0u).x;
    _16[1u] = _78.x;
    uint4 _96 = _18[0u].xxxx;
    uint _97 = _96.x;
    uint _98 = _97 + 63u;
    uint _99 = _98 >> 6u;
    if (_98 > 4194303u)
    {
        _19[0u] = uint4(64u, 64u, 64u, 64u);
        _19[1u] = ((_99 + 63u) >> 6u).xxxx;
        _19[2u] = uint4(1u, 1u, 1u, 1u);
    }
    else
    {
        _19[0u] = _99.xxxx;
        _19[1u] = uint4(1u, 1u, 1u, 1u);
        _19[2u] = uint4(1u, 1u, 1u, 1u);
    }
    _18[0u] = uint4(0u, 0u, 0u, 0u).x;
    _18[1u] = _97.x;
    if ((((_59 | uint(_31 != 0u)) | _78) | _97) == 0u)
    {
        _14[0u] = uint4(0u, 0u, 0u, 0u);
        _14[1u] = uint4(0u, 0u, 0u, 0u);
        _14[2u] = uint4(0u, 0u, 0u, 0u);
        _15[6u] = uint4(0u, 0u, 0u, 0u);
        _15[7u] = uint4(0u, 0u, 0u, 0u);
        _15[8u] = uint4(0u, 0u, 0u, 0u);
    }
    else
    {
        _14[0u] = asuint(_26_m0[60u]).w.xxxx;
        _14[1u] = asuint(_26_m0[61u]).x.xxxx;
        _14[2u] = uint4(1u, 1u, 1u, 1u);
    }
}

[numthreads(1, 1, 1)]
void main()
{
    comp_main();
}
