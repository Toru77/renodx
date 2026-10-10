Buffer<float4> _8 : register(t2, space0);
Buffer<uint4> _12 : register(t3, space0);
RWBuffer<float4> _15 : register(u0, space0);
RWBuffer<float4> _16 : register(u1, space0);

static uint3 gl_WorkGroupID;
static uint3 gl_LocalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_WorkGroupID : SV_GroupID;
    uint3 gl_LocalInvocationID : SV_GroupThreadID;
};

groupshared float _20[1152];

void comp_main()
{
    uint4 _49 = _12.Load((gl_WorkGroupID.x * 2u) + 1u);
    uint _50 = _49.x;
    float _57 = float(gl_LocalInvocationID.x < (_50 & 127u));
    uint _61 = (_12.Load(gl_WorkGroupID.x * 2u).x + gl_LocalInvocationID.x) * 6u;
    uint _62;
    _62 = 0u;
    float _37[6];
    float _38[6];
    float _39[6];
    for (;;)
    {
        float4 _67 = _8.Load(_62 + _61);
        _37[_62] = _67.x * _57;
        _38[_62] = _67.y * _57;
        _39[_62] = _67.z * _57;
        uint _63 = _62 + 1u;
        if (_63 == 6u)
        {
            break;
        }
        else
        {
            _62 = _63;
        }
    }
    _20[0u + ((0u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _37[0u];
    _20[1u + ((0u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _38[0u];
    _20[2u + ((0u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _39[0u];
    _20[0u + ((1u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _37[1u];
    _20[1u + ((1u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _38[1u];
    _20[2u + ((1u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _39[1u];
    _20[0u + ((2u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _37[2u];
    _20[1u + ((2u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _38[2u];
    _20[2u + ((2u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _39[2u];
    _20[0u + ((3u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _37[3u];
    _20[1u + ((3u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _38[3u];
    _20[2u + ((3u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _39[3u];
    _20[0u + ((4u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _37[4u];
    _20[1u + ((4u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _38[4u];
    _20[2u + ((4u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _39[4u];
    _20[0u + ((5u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _37[5u];
    _20[1u + ((5u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _38[5u];
    _20[2u + ((5u + (gl_LocalInvocationID.x * 6u)) * 3u)] = _39[5u];
    GroupMemoryBarrierWithGroupSync();
    if (gl_LocalInvocationID.x == 0u)
    {
        float _34[6];
        float _35[6];
        float _36[6];
        _34[0u] = _20[0u];
        _35[0u] = _20[1u];
        _36[0u] = _20[2u];
        _34[1u] = _20[3u];
        _35[1u] = _20[4u];
        _36[1u] = _20[5u];
        _34[2u] = _20[6u];
        _35[2u] = _20[7u];
        _36[2u] = _20[8u];
        _34[3u] = _20[9u];
        _35[3u] = _20[10u];
        _36[3u] = _20[11u];
        _34[4u] = _20[12u];
        _35[4u] = _20[13u];
        _36[4u] = _20[14u];
        _34[5u] = _20[15u];
        _35[5u] = _20[16u];
        _36[5u] = _20[17u];
        float _277;
        float _279;
        float _281;
        float _283;
        float _285;
        float _287;
        float _289;
        float _291;
        float _293;
        float _295;
        float _297;
        float _299;
        float _301;
        float _303;
        float _305;
        float _307;
        float _309;
        float _311;
        uint _313;
        _277 = _20[17u];
        _279 = _20[16u];
        _281 = _20[15u];
        _283 = _20[14u];
        _285 = _20[13u];
        _287 = _20[12u];
        _289 = _20[11u];
        _291 = _20[10u];
        _293 = _20[9u];
        _295 = _20[8u];
        _297 = _20[7u];
        _299 = _20[6u];
        _301 = _20[5u];
        _303 = _20[4u];
        _305 = _20[3u];
        _307 = _20[2u];
        _309 = _20[1u];
        _311 = _20[0u];
        _313 = 1u;
        float _308;
        float _310;
        float _312;
        for (;;)
        {
            _312 = _20[0u + ((0u + (_313 * 6u)) * 3u)] + _311;
            _310 = _20[1u + ((0u + (_313 * 6u)) * 3u)] + _309;
            _308 = _20[2u + ((0u + (_313 * 6u)) * 3u)] + _307;
            _34[0u] = _312;
            _35[0u] = _310;
            _36[0u] = _308;
            float _306 = _20[0u + ((1u + (_313 * 6u)) * 3u)] + _305;
            float _304 = _20[1u + ((1u + (_313 * 6u)) * 3u)] + _303;
            float _302 = _20[2u + ((1u + (_313 * 6u)) * 3u)] + _301;
            _34[1u] = _306;
            _35[1u] = _304;
            _36[1u] = _302;
            float _300 = _20[0u + ((2u + (_313 * 6u)) * 3u)] + _299;
            float _298 = _20[1u + ((2u + (_313 * 6u)) * 3u)] + _297;
            float _296 = _20[2u + ((2u + (_313 * 6u)) * 3u)] + _295;
            _34[2u] = _300;
            _35[2u] = _298;
            _36[2u] = _296;
            float _294 = _20[0u + ((3u + (_313 * 6u)) * 3u)] + _293;
            float _292 = _20[1u + ((3u + (_313 * 6u)) * 3u)] + _291;
            float _290 = _20[2u + ((3u + (_313 * 6u)) * 3u)] + _289;
            _34[3u] = _294;
            _35[3u] = _292;
            _36[3u] = _290;
            float _288 = _20[0u + ((4u + (_313 * 6u)) * 3u)] + _287;
            float _286 = _20[1u + ((4u + (_313 * 6u)) * 3u)] + _285;
            float _284 = _20[2u + ((4u + (_313 * 6u)) * 3u)] + _283;
            _34[4u] = _288;
            _35[4u] = _286;
            _36[4u] = _284;
            float _282 = _20[0u + ((5u + (_313 * 6u)) * 3u)] + _281;
            float _280 = _20[1u + ((5u + (_313 * 6u)) * 3u)] + _279;
            float _278 = _20[2u + ((5u + (_313 * 6u)) * 3u)] + _277;
            _34[5u] = _282;
            _35[5u] = _280;
            _36[5u] = _278;
            uint _314 = _313 + 1u;
            if (_314 == 64u)
            {
                break;
            }
            else
            {
                _277 = _278;
                _279 = _280;
                _281 = _282;
                _283 = _284;
                _285 = _286;
                _287 = _288;
                _289 = _290;
                _291 = _292;
                _293 = _294;
                _295 = _296;
                _297 = _298;
                _299 = _300;
                _301 = _302;
                _303 = _304;
                _305 = _306;
                _307 = _308;
                _309 = _310;
                _311 = _312;
                _313 = _314;
            }
        }
        uint _425 = (_50 >> 8u) * 6u;
        if ((_50 & 128u) == 0u)
        {
            float _427;
            float _429;
            float _431;
            uint _433;
            _427 = _308;
            _429 = _310;
            _431 = _312;
            _433 = 0u;
            uint _434;
            for (;;)
            {
                _15[_433 + _425] = float4(_431, _429, _427, _431);
                _434 = _433 + 1u;
                if (_434 == 6u)
                {
                    break;
                }
                else
                {
                    _427 = _36[_434];
                    _429 = _35[_434];
                    _431 = _34[_434];
                    _433 = _434;
                    continue;
                }
            }
        }
        else
        {
            float _439;
            float _441;
            float _443;
            uint _445;
            _439 = _308;
            _441 = _310;
            _443 = _312;
            _445 = 0u;
            uint _446;
            for (;;)
            {
                float _447 = _443 * 0.000318309874273836612701416015625f;
                _16[_445 + _425] = float4(_447, _441 * 0.000318309874273836612701416015625f, _439 * 0.000318309874273836612701416015625f, _447);
                _446 = _445 + 1u;
                if (_446 == 6u)
                {
                    break;
                }
                else
                {
                    _439 = _36[_446];
                    _441 = _35[_446];
                    _443 = _34[_446];
                    _445 = _446;
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
