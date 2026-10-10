cbuffer _26_28 : register(b0, space36)
{
    float4 _28_m0[50] : packoffset(c0);
};

Texture2D<uint4> _8 : register(t0, space36);
Texture2D<float4> _12 : register(t1, space36);
Texture2D<float4> _13 : register(t22, space36);
Texture2D<float4> _14 : register(t23, space36);
Texture2D<int4> _18 : register(t28, space36);
RWTexture2D<float4> _21 : register(u2, space36);
RWTexture2D<float4> _22 : register(u3, space36);

static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

void comp_main()
{
    uint _52;
    uint _54;
    uint _101;
    uint _102;
    float _107;
    float _108;
    float _109;
    float _110;
    float _114;
    float _115;
    float _116;
    float _117;
    float _166;
    float _167;
    float _168;
    float4 _170;
    float _174;
    bool _175;
    for (;;)
    {
        uint _39 = gl_GlobalInvocationID.x >> 1u;
        uint _42 = gl_GlobalInvocationID.y << 1u;
        _52 = ((_39 & 2u) | (gl_GlobalInvocationID.x & 4294967289u)) | (_42 & 4u);
        _54 = ((gl_GlobalInvocationID.y & 4294967292u) | (_39 & 1u)) | (_42 & 2u);
        uint _63 = (_52 * 5u) + _54;
        uint _64 = _63 + 37u;
        uint _69 = ((_64 >> 8u) ^ _64) + 1759714724u;
        uint _73 = ((_69 << 8u) ^ _69) * 458671337u;
        uint _82 = _63 + 38u;
        uint _86 = ((_82 >> 8u) ^ _82) + 1759714724u;
        uint _89 = ((_86 << 8u) ^ _86) * 458671337u;
        _101 = uint(((float((_73 & 16777215u) ^ (_73 >> 8u)) * 5.9604644775390625e-08f) + float(_52)) * _28_m0[25u].x);
        _102 = uint(((float((_89 & 16777215u) ^ (_89 >> 8u)) * 5.9604644775390625e-08f) + float(_54)) * _28_m0[25u].y);
        float4 _104 = _14.Load(int3(uint2(_52, _54), 0u));
        _107 = _104.x;
        _108 = _104.y;
        _109 = _104.z;
        _110 = _104.w;
        float4 _112 = _13.Load(int3(uint2(_52, _54), 0u));
        _114 = _112.x;
        _115 = _112.y;
        _116 = _112.z;
        _117 = _112.w;
        uint4 _120 = _8.Load(int3(uint2(_101, _102), 0u));
        uint _122 = _120.x;
        float _133 = (float((_122 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _135 = (float(_122 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
        float _141 = (1.0f - abs(_133)) - abs(_135);
        float _145 = clamp((-0.0f) - _141, 0.0f, 1.0f);
        float _146 = (-0.0f) - _145;
        float _152 = ((_133 >= 0.0f) ? _146 : _145) + _133;
        float _153 = ((_135 >= 0.0f) ? _146 : _145) + _135;
        float _158 = rsqrt(dot(float3(_152, _153, _141), float3(_152, _153, _141)));
        float _159 = _152 * _158;
        float _160 = _153 * _158;
        float _161 = _158 * _141;
        float _165 = rsqrt(dot(float3(_159, _160, _161), float3(_159, _160, _161)));
        _166 = _159 * _165;
        _167 = _160 * _165;
        _168 = _165 * _161;
        _170 = _28_m0[0u];
        _174 = _12.Load(int3(uint2(_101, _102), 0u)).x;
        _175 = _174 > 0.0f;
        if (_175)
        {
            float _179 = _170.w / (_174 - _170.z);
            if (_179 > 0.0f)
            {
                float _205 = mad(_168, _28_m0[4u].z, mad(_167, _28_m0[4u].y, _28_m0[4u].x * _166));
                float _208 = mad(_168, _28_m0[5u].z, mad(_167, _28_m0[5u].y, _28_m0[5u].x * _166));
                float _211 = mad(_168, _28_m0[6u].z, mad(_167, _28_m0[6u].y, _28_m0[6u].x * _166));
                uint4 _215 = asuint(_28_m0[17u]);
                float _235 = (((((float(_101) + 0.5f) / float(_215.x)) * 2.0f) + (-1.0f)) * _179) / _170.x;
                float _237 = ((1.0f - (((float(_102) + 0.5f) / float(_215.y)) * 2.0f)) * _179) / _170.y;
                float _239 = (_211 >= 0.0f) ? 1.0f : (-1.0f);
                float _241 = (-1.0f) / (_239 + _211);
                float _242 = _241 * _208;
                float _243 = _242 * _205;
                float _244 = _239 * _205;
                float _247 = ((_244 * _205) * _241) + 1.0f;
                float _249 = _239 + (_242 * _208);
                float _272 = ((_28_m0[20u].z - _28_m0[20u].w) * clamp(float((uint4(_18.Load(int3(uint2(_52, _54), 0u))).x + 4294967295u) / asuint(_28_m0[23u]).z), 0.0f, 1.0f)) + _28_m0[20u].w;
                float _274;
                float _276;
                float _278;
                float _280;
                float _282;
                float _284;
                float _286;
                float _288;
                float _290;
                float _273 = _107;
                float _275 = _108;
                float _277 = _109;
                float _279 = _110;
                float _281 = _114;
                float _283 = _115;
                float _285 = _116;
                float _287 = _117;
                float _289 = 1.0f;
                uint _291 = 0u;
                float _340;
                float _368;
                float _369;
                float _376;
                float _377;
                uint _388;
                uint _389;
                float _416;
                float4 _458;
                float4 _461;
                float _466;
                bool _470;
                for (;;)
                {
                    uint _303 = asuint(_28_m0[22u]).x + (((_54 * 198491317u) + _52) * 3039394381u);
                    uint _306 = ((_303 >> 8u) ^ _303) + 1759714724u;
                    uint _309 = ((_306 << 8u) ^ _306) * 458671337u;
                    uint _313 = (((_309 >> 8u) ^ _309) * 3039394381u) + _291;
                    uint _314 = _313 + 90303u;
                    uint _318 = ((_314 >> 8u) ^ _314) + 1759714724u;
                    uint _321 = ((_318 << 8u) ^ _318) * 458671337u;
                    uint _326 = _313 + 90304u;
                    uint _330 = ((_326 >> 8u) ^ _326) + 1759714724u;
                    uint _333 = ((_330 << 8u) ^ _330) * 458671337u;
                    float _338 = float((_321 & 16777215u) ^ (_321 >> 8u)) * 3.7450703871400037314742803573608e-07f;
                    _340 = _272 * _179;
                    float _342 = (_340 * 5.9604644775390625e-08f) * float((_333 & 16777215u) ^ (_333 >> 8u));
                    float _345 = _342 * cos(_338);
                    float _346 = _342 * sin(_338);
                    float _359 = (_179 - (_244 * _345)) - (_346 * _208);
                    uint4 _363 = asuint(_28_m0[14u]);
                    uint _364 = _363.x;
                    uint _365 = _363.y;
                    _368 = _28_m0[0u].x;
                    _369 = _28_m0[0u].y;
                    _376 = float(_364);
                    _377 = float(_365);
                    float _378 = ((((((_345 * _247) + _235) + (_346 * _243)) * _368) / _359) + 1.0f) * 0.5f;
                    float _380 = (1.0f - (((((_346 * _249) + _237) + ((_345 * _239) * _243)) * _369) / _359)) * 0.5f;
                    _388 = uint(max(int(0u), int(uint(min(int(_364), int(uint(int(floor(_378 * _376)))))))));
                    _389 = uint(max(int(0u), int(uint(min(int(_365), int(uint(int(floor(_380 * _377)))))))));
                    uint4 _392 = asuint(_28_m0[17u]);
                    uint _393 = _392.x;
                    uint _394 = _392.y;
                    uint _405 = uint(max(int(0u), int(uint(min(int(_393), int(uint(int(floor(_378 * float(_393))))))))));
                    uint _406 = uint(max(int(0u), int(uint(min(int(_394), int(uint(int(floor(_380 * float(_394))))))))));
                    float4 _410 = _12.Load(int3(uint2(_405, _406), 0u));
                    float _412 = _410.x;
                    _416 = (_412 <= 0.0f) ? 0.0f : (_28_m0[0u].w / (_412 - _28_m0[0u].z));
                    uint4 _418 = _8.Load(int3(uint2(_405, _406), 0u));
                    uint _420 = _418.x;
                    float _428 = (float((_420 >> 8u) & 4095u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _429 = (float(_420 >> 20u) * 0.0004884005174972116947174072265625f) + (-1.0f);
                    float _433 = (1.0f - abs(_428)) - abs(_429);
                    float _435 = clamp((-0.0f) - _433, 0.0f, 1.0f);
                    float _436 = (-0.0f) - _435;
                    float _441 = ((_428 >= 0.0f) ? _436 : _435) + _428;
                    float _442 = ((_429 >= 0.0f) ? _436 : _435) + _429;
                    float _446 = rsqrt(dot(float3(_441, _442, _433), float3(_441, _442, _433)));
                    float _447 = _441 * _446;
                    float _448 = _442 * _446;
                    float _449 = _446 * _433;
                    float _453 = rsqrt(dot(float3(_447, _448, _449), float3(_447, _448, _449)));
                    _458 = _14.Load(int3(uint2(_388, _389), 0u));
                    _461 = _13.Load(int3(uint2(_388, _389), 0u));
                    _466 = clamp(dot(float3(_447 * _453, _448 * _453, _453 * _449), float3(_166, _167, _168)), 0.0f, 1.0f);
                    _470 = (_416 <= 0.0f) || (_466 < 0.7070000171661376953125f);
                    float frontier_phi_6_pred;
                    float frontier_phi_6_pred_1;
                    float frontier_phi_6_pred_2;
                    float frontier_phi_6_pred_3;
                    float frontier_phi_6_pred_4;
                    float frontier_phi_6_pred_5;
                    float frontier_phi_6_pred_6;
                    float frontier_phi_6_pred_7;
                    float frontier_phi_6_pred_8;
                    if (_470)
                    {
                        frontier_phi_6_pred = _273;
                        frontier_phi_6_pred_1 = _289;
                        frontier_phi_6_pred_2 = _287;
                        frontier_phi_6_pred_3 = _285;
                        frontier_phi_6_pred_4 = _283;
                        frontier_phi_6_pred_5 = _281;
                        frontier_phi_6_pred_6 = _279;
                        frontier_phi_6_pred_7 = _277;
                        frontier_phi_6_pred_8 = _275;
                    }
                    else
                    {
                        float _506 = (((((((float(_388) + 0.5f) / _376) * 2.0f) + (-1.0f)) * _416) / _368) - _235) / _340;
                        float _507 = ((((1.0f - (((float(_389) + 0.5f) / _377) * 2.0f)) * _416) / _369) - _237) / _340;
                        float _508 = (_416 - _179) / _340;
                        float _528 = (clamp(sqrt(((_507 * _507) + (_506 * _506)) + (_508 * _508)), 0.0f, 1.0f) * _466) * clamp(1.0f - (_28_m0[47u].x * abs(dot(float3(_506, _507, _508), float3(_205, _208, _211)))), 0.0f, 1.0f);
                        frontier_phi_6_pred = (_528 * _458.x) + _273;
                        frontier_phi_6_pred_1 = _528 + _289;
                        frontier_phi_6_pred_2 = (_528 * _461.w) + _287;
                        frontier_phi_6_pred_3 = (_528 * _461.z) + _285;
                        frontier_phi_6_pred_4 = (_528 * _461.y) + _283;
                        frontier_phi_6_pred_5 = (_528 * _461.x) + _281;
                        frontier_phi_6_pred_6 = (_528 * _458.w) + _279;
                        frontier_phi_6_pred_7 = (_528 * _458.z) + _277;
                        frontier_phi_6_pred_8 = (_528 * _458.y) + _275;
                    }
                    _274 = frontier_phi_6_pred;
                    _290 = frontier_phi_6_pred_1;
                    _288 = frontier_phi_6_pred_2;
                    _286 = frontier_phi_6_pred_3;
                    _284 = frontier_phi_6_pred_4;
                    _282 = frontier_phi_6_pred_5;
                    _280 = frontier_phi_6_pred_6;
                    _278 = frontier_phi_6_pred_7;
                    _276 = frontier_phi_6_pred_8;
                    uint _292 = _291 + 1u;
                    if (_292 == 4u)
                    {
                        break;
                    }
                    else
                    {
                        _273 = _274;
                        _275 = _276;
                        _277 = _278;
                        _279 = _280;
                        _281 = _282;
                        _283 = _284;
                        _285 = _286;
                        _287 = _288;
                        _289 = _290;
                        _291 = _292;
                        continue;
                    }
                }
                float _537 = 1.0f / _290;
                _21[uint2(_52, _54)] = float4(_537 * _282, _537 * _284, _537 * _286, _537 * _288);
                _22[uint2(_52, _54)] = float4(_537 * _274, _537 * _276, _537 * _278, _537 * _280);
                break;
            }
        }
        _21[uint2(_52, _54)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        _22[uint2(_52, _54)] = 0.0f.xxxx;
        break;
    }
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
