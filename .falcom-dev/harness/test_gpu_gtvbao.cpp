// Native harness: the GTVBAO main pass (AO and GI flavours), the AO input pass and the RTGI upsample on a real D3D11 WARP device.
// The live repository shaders are compared with the HEAD copies under reference/gtvbao_m0; every comparison is bit exact.
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXPackedVector.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#include "gpu_warp.hpp"

static uint32_t Hash(uint32_t x, uint32_t y, uint32_t salt) {
  uint32_t h = x * 73856093u ^ y * 19349663u ^ salt * 83492791u;
  h ^= h >> 13;
  h *= 0x5bd1e995u;
  h ^= h >> 15;
  return h;
}

static uint16_t Half(float v) { return DirectX::PackedVector::XMConvertFloatToHalf(v); }

template <typename T>
class Com {
 public:
  explicit Com(T* p = nullptr) : p_(p) {}
  ~Com() {
    if (p_ != nullptr) p_->Release();
  }
  Com(const Com&) = delete;
  Com& operator=(const Com&) = delete;
  T* get() const { return p_; }

 private:
  T* p_;
};

struct Level {
  std::vector<uint8_t> bytes;
  UINT pitch;
};

template <typename T>
static Level MakeLevel(const std::vector<T>& v, UINT pitch) {
  Level level;
  level.bytes.resize(v.size() * sizeof(T));
  std::memcpy(level.bytes.data(), v.data(), level.bytes.size());
  level.pitch = pitch;
  return level;
}

static ID3D11Texture2D* MakeTex(ID3D11Device* device, UINT w, UINT h, UINT mips, DXGI_FORMAT format, UINT bind,
                                const std::vector<Level>& levels) {
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = w;
  desc.Height = h;
  desc.MipLevels = mips;
  desc.ArraySize = 1u;
  desc.Format = format;
  desc.SampleDesc.Count = 1u;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = bind;
  std::vector<D3D11_SUBRESOURCE_DATA> data(levels.size());
  for (size_t i = 0; i < levels.size(); ++i) {
    data[i].pSysMem = levels[i].bytes.data();
    data[i].SysMemPitch = levels[i].pitch;
    data[i].SysMemSlicePitch = 0u;
  }
  ID3D11Texture2D* texture = nullptr;
  device->CreateTexture2D(&desc, data.data(), &texture);
  return texture;
}

static ID3D11ShaderResourceView* MakeSrvMips(ID3D11Device* device, ID3D11Texture2D* texture, DXGI_FORMAT format, UINT mips) {
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = format;
  desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  desc.Texture2D.MipLevels = mips;
  ID3D11ShaderResourceView* view = nullptr;
  device->CreateShaderResourceView(texture, &desc, &view);
  return view;
}

static ID3D11SamplerState* MakePointSampler(ID3D11Device* device) {
  D3D11_SAMPLER_DESC desc = {};
  desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  desc.MaxLOD = D3D11_FLOAT32_MAX;
  ID3D11SamplerState* sampler = nullptr;
  device->CreateSamplerState(&desc, &sampler);
  return sampler;
}

// The inputs shared by every case at one size: linear depth mips (t0), game depth (t4 of the AO pass), MRT normal (t1),
// radiance (t4 of the GI pass).
struct Fixture {
  UINT w = 0, h = 0;
  UINT mw[5] = {}, mh[5] = {};
  std::vector<float> mip[5];
  std::vector<float> game;
  std::vector<uint16_t> mrt;
  std::vector<uint16_t> radiance;
};

static Fixture MakeFixture(UINT w, UINT h) {
  Fixture f;
  f.w = w;
  f.h = h;
  f.mw[0] = w;
  f.mh[0] = h;
  for (int m = 1; m < 5; ++m) {
    f.mw[m] = std::max(1u, f.mw[m - 1] / 2u);
    f.mh[m] = std::max(1u, f.mh[m - 1] / 2u);
  }
  for (int m = 0; m < 5; ++m) f.mip[m].assign(static_cast<size_t>(f.mw[m]) * f.mh[m], 0.f);
  const size_t texels = static_cast<size_t>(w) * h;
  f.game.assign(texels, 0.f);
  f.mrt.assign(texels * 4u, 0u);
  f.radiance.assign(texels * 4u, 0u);
  for (UINT y = 0; y < h; ++y) {
    for (UINT x = 0; x < w; ++x) {
      const size_t i = static_cast<size_t>(y) * w + x;
      const bool sky = y < 4u || (x >= 40u && x <= 45u && y >= 10u && y <= 15u);
      float z = 5.f + 0.05f * static_cast<float>(x) + 0.03f * static_cast<float>(y);
      if (x >= 20u && x < 30u) z += 3.f;
      z += static_cast<float>(Hash(x, y, 1u) % 1000u) * 1e-5f;
      f.mip[0][i] = sky ? 65504.f : z;
      f.game[i] = sky ? 0.f : 0.1f / z;

      uint16_t* m = &f.mrt[i * 4u];
      m[0] = static_cast<uint16_t>(Hash(x, y, 2u) & 0xFFFFu);
      m[1] = static_cast<uint16_t>(16384u + Hash(x, y, 3u) % 32768u);
      m[2] = 0u;
      const bool foliage_block = x >= 5u && x <= 14u && y >= 30u && y <= 39u;
      m[3] = (foliage_block || Hash(x, y, 5u) % 100u < 3u) ? 0x8000u : 0u;
      if (Hash(x, y, 4u) % 100u == 0u) m[0] = m[1] = m[2] = m[3] = 0u;

      uint16_t* r = &f.radiance[i * 4u];
      r[0] = Half(static_cast<float>(Hash(x, y, 7u) % 2000u) / 1000.f);
      r[1] = Half(static_cast<float>(Hash(x, y, 17u) % 2000u) / 1000.f);
      r[2] = Half(static_cast<float>(Hash(x, y, 27u) % 2000u) / 1000.f);
      r[3] = Half(1.f);
    }
  }
  for (int m = 1; m < 5; ++m) {
    const UINT pw = f.mw[m - 1], ph = f.mh[m - 1];
    for (UINT y = 0; y < f.mh[m]; ++y) {
      for (UINT x = 0; x < f.mw[m]; ++x) {
        const UINT x0 = std::min(2u * x, pw - 1u), x1 = std::min(2u * x + 1u, pw - 1u);
        const UINT y0 = std::min(2u * y, ph - 1u), y1 = std::min(2u * y + 1u, ph - 1u);
        const float* p = f.mip[m - 1].data();
        const float sum = p[y0 * pw + x0] + p[y0 * pw + x1] + p[y1 * pw + x0] + p[y1 * pw + x1];
        f.mip[m][static_cast<size_t>(y) * f.mw[m] + x] = sum * 0.25f;
      }
    }
  }
  return f;
}

// cb_scene (b0, 50 float4): identity view, viewInv, projInv and viewProjInv; proj = diag(0.974279, 1.732051, 1, 1); c49 = (rs, rs).
static std::vector<float> SceneCb(float rs) {
  std::vector<float> s(50u * 4u, 0.f);
  const size_t identity_columns[] = {0u, 4u, 12u, 20u};
  for (size_t c : identity_columns) {
    for (size_t r = 0; r < 4u; ++r) s[(c + r) * 4u + r] = 1.f;
  }
  s[8u * 4u + 0u] = 0.974279f;
  s[9u * 4u + 1u] = 1.732051f;
  s[10u * 4u + 2u] = 1.f;
  s[11u * 4u + 3u] = 1.f;
  s[49u * 4u + 0u] = rs;
  s[49u * 4u + 1u] = rs;
  return s;
}

struct AoCase {
  const char* name;
  UINT d;
  float skip;
  float debug;
  float rs;
  int i0;
  float v0;
  int i1;
  float v1;
};

// b13 for the AO main pass (48 floats). c31 skip, c32/c33 full size, c34 d, c35 Half flag.
static std::vector<float> AoPush(const AoCase& k, UINT w, UINT h) {
  std::vector<float> c(48u, 0.f);
  c[0] = 2.f;
  c[1] = 0.4f;
  c[2] = 1.f;
  c[3] = 0.8f;
  c[4] = 1.5f;
  c[5] = 0.3f;
  c[6] = 3.5f;
  c[7] = 5.f;
  c[8] = k.debug;
  c[10] = 1.f;
  c[11] = 1.f;
  c[12] = 1.f;
  c[13] = 1.f;
  c[14] = 1.f;
  c[15] = 0.f;
  c[16] = 2.f;
  c[17] = 1.f;
  c[18] = 1.f;
  c[20] = 1.f;
  c[22] = 1.f;
  c[23] = 1.f;
  c[25] = 2.f;
  c[26] = 2.f;
  c[27] = 1.f;
  c[28] = 0.f;
  c[30] = 1.f;
  c[31] = k.skip;
  c[32] = static_cast<float>(w);
  c[33] = static_cast<float>(h);
  c[34] = static_cast<float>(k.d);
  c[35] = k.d > 1u ? 1.f : 0.f;
  if (k.i0 >= 0) c[k.i0] = k.v0;
  if (k.i1 >= 0) c[k.i1] = k.v1;
  return c;
}

// b13 for the GI main pass: the vbgi branch of BuildGTVBAOPushConstants (48 floats), plus the use_far tail (56 floats).
static std::vector<float> GiPush(bool use_far) {
  std::vector<float> c(use_far ? 56u : 48u, 0.f);
  c[0] = 2.f;
  c[1] = 0.4f;
  c[2] = 1.f;
  c[3] = 1.f;
  c[4] = 1.5f;
  c[5] = 0.3f;
  c[6] = 3.5f;
  c[7] = 37.f;
  c[10] = 1.f;
  c[12] = 1.f;
  c[13] = 1.f;
  c[14] = 1.f;
  c[16] = 1.f;
  c[18] = 1.f;
  c[25] = 2.f;
  c[26] = 2.f;
  c[43] = 1.f;
  c[46] = 65504.f;
  c[47] = 1.f;
  if (use_far) {
    c[48] = 200.f;
    c[49] = 16.f;
    c[50] = 15.f;
    c[51] = 1.f;
    c[52] = 0.f;
    c[53] = 2.f;
    c[54] = 4.f;
  }
  return c;
}

enum class Flavour { kAo,
                     kGi,
                     kGiFar };

// Binds the main pass (t0 depth mips, t1 MRT, t3 foliage (AO size), t4 game depth (AO) or radiance (GI); u0 AO or GI output, u1 debug,
// u2..u4 use_far outputs) and dispatches it. The AO flavour then runs the input pass on the r32_uint output (when input_cs is set).
// Returns the readbacks: AO: [0] rgba16f AO-size, [1] debug; GI: [0] rgba16f, [1] debug; GI use_far: plus [2] rgba32f, [3], [4] rgba16f.
static std::vector<std::vector<uint8_t>> RunMain(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11ComputeShader* main_cs,
                                                 ID3D11ComputeShader* input_cs, ID3D11SamplerState* sampler, const Fixture& f,
                                                 const std::vector<float>& push, float rs, UINT d, Flavour flavour) {
  const bool ao = flavour == Flavour::kAo;
  const UINT aw = ao ? (f.w + d - 1u) / d : f.w;
  const UINT ah = ao ? (f.h + d - 1u) / d : f.h;
  const UINT kBind = D3D11_BIND_SHADER_RESOURCE;
  const UINT kBindUav = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

  std::vector<Level> depth_levels;
  for (int m = 0; m < 5; ++m) depth_levels.push_back(MakeLevel(f.mip[m], f.mw[m] * 4u));
  Com<ID3D11Texture2D> depth(MakeTex(device, f.w, f.h, 5u, DXGI_FORMAT_R32_FLOAT, kBind, depth_levels));
  Com<ID3D11Texture2D> mrt(MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R16G16B16A16_UINT, kBind, {MakeLevel(f.mrt, f.w * 8u)}));
  std::vector<uint8_t> foliage(static_cast<size_t>(aw) * ah, 0u);
  for (UINT y = 0; y < ah; ++y) {
    for (UINT x = 0; x < aw; ++x) foliage[static_cast<size_t>(y) * aw + x] = Hash(x, y, 6u) % 7u == 0u ? 1u : 0u;
  }
  Com<ID3D11Texture2D> fol(MakeTex(device, aw, ah, 1u, DXGI_FORMAT_R8_UINT, kBind, {MakeLevel(foliage, aw)}));
  Com<ID3D11Texture2D> game(MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R32_FLOAT, kBind, {MakeLevel(f.game, f.w * 4u)}));
  Com<ID3D11Texture2D> radiance(
      MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT, kBind, {MakeLevel(f.radiance, f.w * 8u)}));

  const UINT ow = ao ? aw : f.w;
  const UINT oh = ao ? ah : f.h;
  const bool uint_out = ao && input_cs != nullptr;
  Com<ID3D11Texture2D> out(uint_out ? MakeTex(device, aw, ah, 1u, DXGI_FORMAT_R32_UINT, kBindUav,
                                              {MakeLevel(std::vector<uint32_t>(static_cast<size_t>(aw) * ah, 0u), aw * 4u)})
                                    : MakeTex(device, ow, oh, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT, kBindUav,
                                              {MakeLevel(std::vector<uint16_t>(static_cast<size_t>(ow) * oh * 4u, 0u), ow * 8u)}));
  Com<ID3D11Texture2D> dbg(MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, kBindUav,
                                   {MakeLevel(std::vector<uint8_t>(static_cast<size_t>(f.w) * f.h * 4u, 0u), f.w * 4u)}));
  Com<ID3D11Texture2D> far2(MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R32G32B32A32_FLOAT, kBindUav,
                                    {MakeLevel(std::vector<float>(static_cast<size_t>(f.w) * f.h * 4u, 0.f), f.w * 16u)}));
  Com<ID3D11Texture2D> far3(MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT, kBindUav,
                                    {MakeLevel(std::vector<uint16_t>(static_cast<size_t>(f.w) * f.h * 4u, 0u), f.w * 8u)}));
  Com<ID3D11Texture2D> far4(MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT, kBindUav,
                                    {MakeLevel(std::vector<uint16_t>(static_cast<size_t>(f.w) * f.h * 4u, 0u), f.w * 8u)}));

  Com<ID3D11ShaderResourceView> depth_srv(MakeSrvMips(device, depth.get(), DXGI_FORMAT_R32_FLOAT, 5u));
  Com<ID3D11ShaderResourceView> mrt_srv(MakeSrvMips(device, mrt.get(), DXGI_FORMAT_R16G16B16A16_UINT, 1u));
  Com<ID3D11ShaderResourceView> fol_srv(MakeSrvMips(device, fol.get(), DXGI_FORMAT_R8_UINT, 1u));
  Com<ID3D11ShaderResourceView> game_srv(MakeSrvMips(device, game.get(), DXGI_FORMAT_R32_FLOAT, 1u));
  Com<ID3D11ShaderResourceView> rad_srv(MakeSrvMips(device, radiance.get(), DXGI_FORMAT_R16G16B16A16_FLOAT, 1u));
  ID3D11ShaderResourceView* srvs[5] = {depth_srv.get(), mrt_srv.get(), nullptr, fol_srv.get(), ao ? game_srv.get() : rad_srv.get()};

  Com<ID3D11UnorderedAccessView> out_uav(MakeUav(device, out.get(), uint_out ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16G16B16A16_FLOAT));
  Com<ID3D11UnorderedAccessView> dbg_uav(MakeUav(device, dbg.get(), DXGI_FORMAT_R8G8B8A8_UNORM));
  Com<ID3D11UnorderedAccessView> far2_uav(MakeUav(device, far2.get(), DXGI_FORMAT_R32G32B32A32_FLOAT));
  Com<ID3D11UnorderedAccessView> far3_uav(MakeUav(device, far3.get(), DXGI_FORMAT_R16G16B16A16_FLOAT));
  Com<ID3D11UnorderedAccessView> far4_uav(MakeUav(device, far4.get(), DXGI_FORMAT_R16G16B16A16_FLOAT));
  ID3D11UnorderedAccessView* uavs[5] = {out_uav.get(), dbg_uav.get(), far2_uav.get(), far3_uav.get(), far4_uav.get()};

  const std::vector<float> scene = SceneCb(rs);
  Com<ID3D11Buffer> scene_cb(MakeConstantBuffer(device, scene.data(), static_cast<UINT>(scene.size() * sizeof(float))));
  Com<ID3D11Buffer> push_cb(MakeConstantBuffer(device, push.data(), static_cast<UINT>(push.size() * sizeof(float))));
  ID3D11Buffer* scene_buf = scene_cb.get();
  ID3D11Buffer* push_buf = push_cb.get();
  ID3D11SamplerState* samp = sampler;

  context->CSSetShader(main_cs, nullptr, 0u);
  context->CSSetConstantBuffers(0u, 1u, &scene_buf);
  context->CSSetConstantBuffers(13u, 1u, &push_buf);
  context->CSSetSamplers(0u, 1u, &samp);
  context->CSSetShaderResources(0u, 5u, srvs);
  context->CSSetUnorderedAccessViews(0u, 5u, uavs, nullptr);
  context->Dispatch((aw + 7u) / 8u, (ah + 7u) / 8u, 1u);
  ID3D11ShaderResourceView* null_srvs[5] = {};
  ID3D11UnorderedAccessView* null_uavs[5] = {};
  context->CSSetShaderResources(0u, 5u, null_srvs);
  context->CSSetUnorderedAccessViews(0u, 5u, null_uavs, nullptr);

  std::vector<std::vector<uint8_t>> result;
  if (ao && input_cs != nullptr) {
    Com<ID3D11Texture2D> f16(MakeTex(device, aw, ah, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT, kBindUav,
                                     {MakeLevel(std::vector<uint16_t>(static_cast<size_t>(aw) * ah * 4u, 0u), aw * 8u)}));
    Com<ID3D11ShaderResourceView> ao_srv(MakeSrvMips(device, out.get(), DXGI_FORMAT_R32_UINT, 1u));
    Com<ID3D11UnorderedAccessView> f16_uav(MakeUav(device, f16.get(), DXGI_FORMAT_R16G16B16A16_FLOAT));
    const float input_push[4] = {static_cast<float>(aw), static_cast<float>(ah), 0.f, 0.f};
    Com<ID3D11Buffer> input_cb(MakeConstantBuffer(device, input_push, sizeof(input_push)));
    ID3D11Buffer* input_buf = input_cb.get();
    ID3D11ShaderResourceView* input_srv = ao_srv.get();
    ID3D11UnorderedAccessView* input_uav = f16_uav.get();
    context->CSSetShader(input_cs, nullptr, 0u);
    context->CSSetConstantBuffers(12u, 1u, &input_buf);
    context->CSSetShaderResources(0u, 1u, &input_srv);
    context->CSSetUnorderedAccessViews(0u, 1u, &input_uav, nullptr);
    context->Dispatch((aw + 7u) / 8u, (ah + 7u) / 8u, 1u);
    ID3D11ShaderResourceView* null_srv = nullptr;
    ID3D11UnorderedAccessView* null_uav = nullptr;
    context->CSSetShaderResources(0u, 1u, &null_srv);
    context->CSSetUnorderedAccessViews(0u, 1u, &null_uav, nullptr);
    result.push_back(Readback(device, context, f16.get(), aw, ah, DXGI_FORMAT_R16G16B16A16_FLOAT, 8u));
  } else {
    result.push_back(Readback(device, context, out.get(), ow, oh, DXGI_FORMAT_R16G16B16A16_FLOAT, 8u));
  }
  result.push_back(Readback(device, context, dbg.get(), f.w, f.h, DXGI_FORMAT_R8G8B8A8_UNORM, 4u));
  if (flavour == Flavour::kGiFar) {
    result.push_back(Readback(device, context, far2.get(), f.w, f.h, DXGI_FORMAT_R32G32B32A32_FLOAT, 16u));
    result.push_back(Readback(device, context, far3.get(), f.w, f.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 8u));
    result.push_back(Readback(device, context, far4.get(), f.w, f.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 8u));
  }
  return result;
}

// Pass/fail per AO texel. Unread texels (the upsample does not read them; see the rule in the M1 plan) may be the reference value,
// or exactly the skip value (1.5, 1.5, 1.5, 1). Every other texel must be bit equal. The debug texture must be bit equal.
static size_t CompareAo(const Fixture& f, const AoCase& k, const std::vector<std::vector<uint8_t>>& want,
                        const std::vector<std::vector<uint8_t>>& got, size_t* unread_out, size_t* skipped_out) {
  static const uint8_t kSkipValue[8] = {0x00, 0x3E, 0x00, 0x3E, 0x00, 0x3E, 0x00, 0x3C};
  const UINT aw = (f.w + k.d - 1u) / k.d;
  const UINT ah = (f.h + k.d - 1u) / k.d;
  size_t mismatch = 0;
  size_t unread = 0;
  size_t skipped = 0;
  for (UINT y = 0; y < ah; ++y) {
    for (UINT x = 0; x < aw; ++x) {
      const size_t t = static_cast<size_t>(y) * aw + x;
      const uint8_t* rv = &want[0][t * 8u];
      const uint8_t* lv = &got[0][t * 8u];
      bool is_unread = false;
      if (k.skip > 0.f && k.debug < 22.f) {
        const UINT rx = std::min(x * k.d + k.d / 2u, f.w - 1u);
        const UINT ry = std::min(y * k.d + k.d / 2u, f.h - 1u);
        const float rep_depth = f.game[static_cast<size_t>(ry) * f.w + rx];
        is_unread = rep_depth <= 0.f || (k.skip > 1.5f && k.rs != 1.f);
        if (is_unread) ++unread;
      }
      const bool live_skip = std::memcmp(lv, kSkipValue, 8u) == 0;
      const bool equal = std::memcmp(rv, lv, 8u) == 0;
      if (is_unread && live_skip) ++skipped;
      if (is_unread ? (!equal && !live_skip) : !equal) ++mismatch;
    }
  }
  mismatch += CountMismatch(want[1], got[1], 1u);
  *unread_out = unread;
  *skipped_out = skipped;
  return mismatch;
}

static const AoCase kAoCases[] = {
    {"owner", 1u, 2.f, 0.f, 1.f, -1, 0.f, -1, 0.f},
    {"owner_half", 2u, 2.f, 0.f, 1.f, -1, 0.f, -1, 0.f},
    {"reproject", 1u, 1.f, 0.f, 1.f, -1, 0.f, -1, 0.f},
    {"skip_off", 1u, 0.f, 0.f, 1.f, -1, 0.f, -1, 0.f},
    {"foliage_half_value", 1u, 2.f, 0.f, 1.f, 28, 0.5f, -1, 0.f},
    {"exclude_foliage_off", 1u, 2.f, 0.f, 1.f, 27, 0.f, -1, 0.f},
    {"edge_rejection", 1u, 2.f, 0.f, 1.f, 15, 0.7f, -1, 0.f},
    {"neutral_normals", 1u, 2.f, 0.f, 1.f, 16, 1.f, 17, 0.f},
    {"quality_low", 1u, 2.f, 0.f, 1.f, 0, 0.f, -1, 0.f},
    {"quality_medium", 1u, 2.f, 0.f, 1.f, 0, 1.f, -1, 0.f},
    {"quality_ultra", 1u, 2.f, 0.f, 1.f, 0, 3.f, -1, 0.f},
    {"cosine_weight", 1u, 2.f, 0.f, 1.f, 26, 0.f, -1, 0.f},
    {"cosine_project", 1u, 2.f, 0.f, 1.f, 26, 1.f, -1, 0.f},
    {"uniform", 1u, 2.f, 0.f, 1.f, 36, 1.f, -1, 0.f},
    {"debug_22", 1u, 2.f, 22.f, 1.f, -1, 0.f, -1, 0.f},
    {"debug_24", 1u, 2.f, 24.f, 1.f, -1, 0.f, -1, 0.f},
    {"debug_25", 1u, 2.f, 25.f, 1.f, -1, 0.f, -1, 0.f},
    {"debug_26", 1u, 2.f, 26.f, 1.f, -1, 0.f, -1, 0.f},
    {"scaled", 1u, 2.f, 0.f, 0.5f, -1, 0.f, -1, 0.f},
    {"scaled_reproject", 1u, 1.f, 0.f, 0.5f, -1, 0.f, -1, 0.f},
};

static const UINT kSizes[][2] = {{64u, 64u}, {67u, 45u}};

static size_t TestAo(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11InfoQueue* queue) {
  const std::vector<std::string> ref_dirs = {"reference/gtvbao_m0/gtvbao/", "reference/gtvbao_m0/world/shaders/", "reference/gtvbao_m0/",
                                             kRepoDir};
  const std::vector<std::string> live_dirs = {std::string(kRepoDir) + "gtvbao/", std::string(kRepoDir) + "world/shaders/", kRepoDir};
  ID3D11ComputeShader* ref_main = CompileCs(device, LoadText("reference/gtvbao_m0/gtvbao/gtvbao_main.cs_5_0.hlsl"), "reference main ao",
                                            ref_dirs);
  ID3D11ComputeShader* ref_input = CompileCs(device, LoadText("reference/gtvbao_m0/world/shaders/gtvbao_svgf_input.cs_5_0.hlsl"),
                                             "reference svgf input", ref_dirs);
  ID3D11ComputeShader* live_main = CompileCs(device, LoadText(std::string(kRepoDir) + "gtvbao/gtvbao_main.cs_5_0.hlsl"), "live main ao",
                                             live_dirs);
  if (ref_main == nullptr || ref_input == nullptr || live_main == nullptr) {
    std::printf("FAIL ao: shader compile\n");
    ++g_failures;
    return 0u;
  }
  ID3D11SamplerState* sampler = MakePointSampler(device);
  size_t total = 0;
  for (const auto& size : kSizes) {
    const Fixture f = MakeFixture(size[0], size[1]);
    for (const AoCase& k : kAoCases) {
      const std::vector<float> push = AoPush(k, size[0], size[1]);
      const auto want = RunMain(device, context, ref_main, ref_input, sampler, f, push, k.rs, k.d, Flavour::kAo);
      const auto got = RunMain(device, context, live_main, nullptr, sampler, f, push, k.rs, k.d, Flavour::kAo);
      size_t unread = 0;
      size_t skipped = 0;
      const size_t mismatch = CompareAo(f, k, want, got, &unread, &skipped);
      total += mismatch;
      std::printf("case %s %ux%u mismatched %zu unread %zu skipped %zu\n", k.name, size[0], size[1], mismatch, unread, skipped);
      if (skipped != unread) {
        std::printf("FAIL ao %s %ux%u: skipped %zu != unread %zu\n", k.name, size[0], size[1], skipped, unread);
        ++g_failures;
      }
      CheckMessages(queue, "gtvbao ao");
    }
  }
  ref_main->Release();
  ref_input->Release();
  live_main->Release();
  sampler->Release();
  return total;
}

static size_t TestGi(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11InfoQueue* queue) {
  const std::vector<std::string> ref_dirs = {"reference/gtvbao_m0/gtvbao/", "reference/gtvbao_m0/world/shaders/", "reference/gtvbao_m0/",
                                             kRepoDir};
  const std::vector<std::string> live_dirs = {std::string(kRepoDir) + "gtvbao/", std::string(kRepoDir) + "world/shaders/", kRepoDir};
  ID3D11ComputeShader* ref_gi = CompileCs(device, LoadText("reference/gtvbao_m0/gtvbao/gtvbao_vbgi_main.cs_5_0.hlsl"), "reference gi",
                                          ref_dirs);
  ID3D11ComputeShader* live_gi = CompileCs(device, LoadText(std::string(kRepoDir) + "gtvbao/gtvbao_vbgi_main.cs_5_0.hlsl"), "live gi",
                                           live_dirs);
  ID3D11ComputeShader* ref_far = CompileCs(device, LoadText("reference/gtvbao_m0/gtvbao/gtvbao_vbgi_far_main.cs_5_0.hlsl"),
                                           "reference gi far", ref_dirs);
  ID3D11ComputeShader* live_far = CompileCs(device, LoadText(std::string(kRepoDir) + "gtvbao/gtvbao_vbgi_far_main.cs_5_0.hlsl"),
                                            "live gi far", live_dirs);
  if (ref_gi == nullptr || live_gi == nullptr || ref_far == nullptr || live_far == nullptr) {
    std::printf("FAIL gi: shader compile\n");
    ++g_failures;
    return 0u;
  }
  ID3D11SamplerState* sampler = MakePointSampler(device);
  size_t total = 0;
  for (const auto& size : kSizes) {
    const Fixture f = MakeFixture(size[0], size[1]);
    for (int k = 0; k < 2; ++k) {
      const bool use_far = k == 1;
      const std::vector<float> push = GiPush(use_far);
      const Flavour flavour = use_far ? Flavour::kGiFar : Flavour::kGi;
      const auto want = RunMain(device, context, use_far ? ref_far : ref_gi, nullptr, sampler, f, push, 1.f, 1u, flavour);
      const auto got = RunMain(device, context, use_far ? live_far : live_gi, nullptr, sampler, f, push, 1.f, 1u, flavour);
      size_t mismatch = CountMismatch(want[0], got[0], 2u) + CountMismatch(want[1], got[1], 1u);
      if (use_far) {
        mismatch += CountMismatch(want[2], got[2], 4u) + CountMismatch(want[3], got[3], 2u) + CountMismatch(want[4], got[4], 2u);
      }
      total += mismatch;
      std::printf("case %s %ux%u mismatched %zu unread 0 skipped 0\n", use_far ? "gi_far" : "gi", size[0], size[1], mismatch);
      CheckMessages(queue, "gtvbao gi");
    }
  }
  ref_gi->Release();
  live_gi->Release();
  ref_far->Release();
  live_far->Release();
  sampler->Release();
  return total;
}

// Upsample: one trace texel per d x d block. Game depth (t0, Texture2D<float4>), MRT (t1), trace GI (t2), null albedo (t3).
static std::vector<std::vector<uint8_t>> RunUpsample(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11ComputeShader* cs,
                                                     const Fixture& f, UINT d, float debug, bool ao) {
  const UINT tw = (f.w + d - 1u) / d;
  const UINT th = (f.h + d - 1u) / d;
  std::vector<float> trace(static_cast<size_t>(tw) * th * 4u, 0.f);
  for (UINT y = 0; y < th; ++y) {
    for (UINT x = 0; x < tw; ++x) {
      float* v = &trace[(static_cast<size_t>(y) * tw + x) * 4u];
      v[0] = static_cast<float>(Hash(x, y, 8u) % 1500u) / 1000.f;
      v[1] = static_cast<float>(Hash(x, y, 18u) % 1500u) / 1000.f;
      v[2] = static_cast<float>(Hash(x, y, 28u) % 1500u) / 1000.f;
      v[3] = Hash(x, y, 9u) % 10u == 0u ? -1.f : 1.f;
    }
  }
  const UINT kBind = D3D11_BIND_SHADER_RESOURCE;
  const UINT kBindUav = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
  Com<ID3D11Texture2D> depth(MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R32_FLOAT, kBind, {MakeLevel(f.game, f.w * 4u)}));
  Com<ID3D11Texture2D> mrt(MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R16G16B16A16_UINT, kBind, {MakeLevel(f.mrt, f.w * 8u)}));
  Com<ID3D11Texture2D> gin(MakeTex(device, tw, th, 1u, DXGI_FORMAT_R32G32B32A32_FLOAT, kBind, {MakeLevel(trace, tw * 16u)}));
  Com<ID3D11ShaderResourceView> depth_srv(MakeSrvMips(device, depth.get(), DXGI_FORMAT_R32_FLOAT, 1u));
  Com<ID3D11ShaderResourceView> mrt_srv(MakeSrvMips(device, mrt.get(), DXGI_FORMAT_R16G16B16A16_UINT, 1u));
  Com<ID3D11ShaderResourceView> gin_srv(MakeSrvMips(device, gin.get(), DXGI_FORMAT_R32G32B32A32_FLOAT, 1u));
  ID3D11ShaderResourceView* srvs[4] = {depth_srv.get(), mrt_srv.get(), gin_srv.get(), nullptr};

  Com<ID3D11Texture2D> out(ao ? MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R32_UINT, kBindUav,
                                        {MakeLevel(std::vector<uint32_t>(static_cast<size_t>(f.w) * f.h, 0u), f.w * 4u)})
                              : MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT, kBindUav,
                                        {MakeLevel(std::vector<uint16_t>(static_cast<size_t>(f.w) * f.h * 4u, 0u), f.w * 8u)}));
  Com<ID3D11Texture2D> dbg(MakeTex(device, f.w, f.h, 1u, DXGI_FORMAT_R16G16B16A16_FLOAT, kBindUav,
                                   {MakeLevel(std::vector<uint16_t>(static_cast<size_t>(f.w) * f.h * 4u, 0u), f.w * 8u)}));
  Com<ID3D11UnorderedAccessView> out_uav(MakeUav(device, out.get(), ao ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16G16B16A16_FLOAT));
  Com<ID3D11UnorderedAccessView> dbg_uav(MakeUav(device, dbg.get(), DXGI_FORMAT_R16G16B16A16_FLOAT));
  ID3D11UnorderedAccessView* uavs[2] = {out_uav.get(), dbg_uav.get()};

  const std::vector<float> scene = SceneCb(1.f);
  Com<ID3D11Buffer> scene_cb(MakeConstantBuffer(device, scene.data(), static_cast<UINT>(scene.size() * sizeof(float))));
  const float push[12] = {static_cast<float>(f.w), static_cast<float>(f.h), static_cast<float>(tw), static_cast<float>(th),
                          static_cast<float>(d), debug, 1.f, 1.f,
                          1.f, 0.f, 0.f, 0.f};
  Com<ID3D11Buffer> push_cb(MakeConstantBuffer(device, push, sizeof(push)));
  ID3D11Buffer* scene_buf = scene_cb.get();
  ID3D11Buffer* push_buf = push_cb.get();
  context->CSSetShader(cs, nullptr, 0u);
  context->CSSetConstantBuffers(0u, 1u, &scene_buf);
  context->CSSetConstantBuffers(12u, 1u, &push_buf);
  context->CSSetShaderResources(0u, 4u, srvs);
  context->CSSetUnorderedAccessViews(0u, 2u, uavs, nullptr);
  context->Dispatch((f.w + 7u) / 8u, (f.h + 7u) / 8u, 1u);
  ID3D11ShaderResourceView* null_srvs[4] = {};
  ID3D11UnorderedAccessView* null_uavs[2] = {};
  context->CSSetShaderResources(0u, 4u, null_srvs);
  context->CSSetUnorderedAccessViews(0u, 2u, null_uavs, nullptr);

  std::vector<std::vector<uint8_t>> result;
  result.push_back(ao ? Readback(device, context, out.get(), f.w, f.h, DXGI_FORMAT_R32_UINT, 4u)
                      : Readback(device, context, out.get(), f.w, f.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 8u));
  result.push_back(Readback(device, context, dbg.get(), f.w, f.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 8u));
  return result;
}

struct UpsampleFlavour {
  const char* name;
  const char* prefix;
  bool ao;
};

static size_t TestUpsample(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11InfoQueue* queue) {
  const std::vector<std::string> ref_dirs = {"reference/gtvbao_m0/gtvbao/", "reference/gtvbao_m0/world/shaders/", "reference/gtvbao_m0/",
                                             kRepoDir};
  const std::vector<std::string> live_dirs = {std::string(kRepoDir) + "gtvbao/", std::string(kRepoDir) + "world/shaders/", kRepoDir};
  const UpsampleFlavour flavours[] = {
      {"plain", "", false},
      {"vbgi", "#define RTGI_UPSAMPLE_NEAREST_FALLBACK\n", false},
      {"ao", "#define RTGI_UPSAMPLE_NEAREST_FALLBACK\n#define RTGI_UPSAMPLE_AO_BYTE\n", true},
  };
  ID3D11ComputeShader* ref[3] = {};
  ID3D11ComputeShader* live[3] = {};
  for (int i = 0; i < 3; ++i) {
    ref[i] = CompileCs(device, std::string(flavours[i].prefix) + LoadText("reference/gtvbao_m0/world/shaders/world_rtgi_upsample.cs_5_0.hlsl"),
                       flavours[i].name, ref_dirs);
    live[i] = CompileCs(device,
                        std::string(flavours[i].prefix) + LoadText(std::string(kRepoDir) + "world/shaders/world_rtgi_upsample.cs_5_0.hlsl"),
                        flavours[i].name, live_dirs);
    if (ref[i] == nullptr || live[i] == nullptr) {
      std::printf("FAIL upsample: shader compile %s\n", flavours[i].name);
      ++g_failures;
      return 0u;
    }
  }
  size_t total = 0;
  for (const auto& size : kSizes) {
    const Fixture f = MakeFixture(size[0], size[1]);
    for (int i = 0; i < 3; ++i) {
      for (UINT d : {1u, 2u, 4u}) {
        const auto want = RunUpsample(device, context, ref[i], f, d, 0.f, flavours[i].ao);
        const auto got = RunUpsample(device, context, live[i], f, d, 0.f, flavours[i].ao);
        const size_t mismatch = CountMismatch(want[0], got[0], flavours[i].ao ? 4u : 2u) + CountMismatch(want[1], got[1], 2u);
        total += mismatch;
        std::printf("case upsample_%s_d%u_debug0 %ux%u mismatched %zu unread 0 skipped 0\n", flavours[i].name, d, size[0], size[1],
                    mismatch);
        CheckMessages(queue, "gtvbao upsample");
      }
      if (flavours[i].ao) {
        const auto want = RunUpsample(device, context, ref[i], f, 1u, 6.f, true);
        const auto got = RunUpsample(device, context, live[i], f, 1u, 6.f, true);
        const size_t mismatch = CountMismatch(want[0], got[0], 4u) + CountMismatch(want[1], got[1], 2u);
        total += mismatch;
        std::printf("case upsample_ao_d1_debug6 %ux%u mismatched %zu unread 0 skipped 0\n", size[0], size[1], mismatch);
        CheckMessages(queue, "gtvbao upsample");
      }
    }
  }
  for (int i = 0; i < 3; ++i) {
    ref[i]->Release();
    live[i]->Release();
  }
  return total;
}

int main() {
  UINT flags = D3D11_CREATE_DEVICE_DEBUG;
  ID3D11Device* device = nullptr;
  ID3D11DeviceContext* context = nullptr;
  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
  if (FAILED(hr)) {
    flags = 0u;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
  }
  if (FAILED(hr) || device == nullptr) {
    std::printf("SKIP (no WARP) hr=0x%08lx\n", static_cast<unsigned long>(hr));
    return 0;
  }
  ID3D11InfoQueue* queue = nullptr;
  if (flags != 0u) device->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&queue));
  std::printf("WARP device created (debug layer %s)\n", queue != nullptr ? "on" : "off");

  size_t total = 0;
  total += TestAo(device, context, queue);
  total += TestGi(device, context, queue);
  total += TestUpsample(device, context, queue);
  if (queue != nullptr) queue->Release();
  context->Release();
  device->Release();
  if (total != 0u) ++g_failures;
  std::printf("gtvbao mismatches total %zu\n", total);
  return g_failures == 0 ? 0 : 1;
}
