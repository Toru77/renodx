// Native harness: the SVGF spatial passes on a real D3D11 WARP device (no game, no ReShade).
// (a) The tile a-trous kernels (world_svgf_atrous_s1, _s2, _s4) against the M2 a-trous (reference/world_svgf_atrous_m2) at the
//     same step. (b) The live a-trous (world_svgf_atrous) at steps 1, 2, 4, 8 and 16 against the M2 copy. (c) The live
//     variance (world_svgf_variance) against the M2 copy, outputs u0 and u1 at debug 0 and 11. Every comparison is bit exact:
//     any mismatching 16-bit (or 8-bit) word is a failure.
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXPackedVector.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <utility>
#include <vector>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

static int g_failures = 0;
static const char kShaderDir[] = "src/games/falcomengine-plus/world/shaders/";
static const char kRepoDir[] = "../../src/games/falcomengine-plus/";  // taa/ and include/ are not in the harness copy

// Reads the #include files. The harness copy holds world/ only: the other folders (taa, include, the falcomengine-plus root) are
// read from the repository, with the leading "../" segments of the name dropped.
struct FolderInclude : ID3DInclude {
  std::deque<std::string> loaded;
  static bool ReadFile(const std::string& path, std::string* out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    out->assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return true;
  }
  HRESULT Open(D3D_INCLUDE_TYPE, LPCSTR name, LPCVOID, LPCVOID* data, UINT* bytes) override {
    std::string n = name;
    while (n.rfind("../", 0) == 0) n = n.substr(3);
    const std::string dirs[] = {kShaderDir, kRepoDir, std::string(kRepoDir) + "taa/", std::string(kRepoDir) + "include/"};
    std::string text;
    bool found = false;
    for (const std::string& dir : dirs) {
      if (ReadFile(dir + n, &text)) {
        found = true;
        break;
      }
    }
    if (!found) return E_FAIL;
    loaded.push_back(std::move(text));
    *data = loaded.back().data();
    *bytes = static_cast<UINT>(loaded.back().size());
    return S_OK;
  }
  HRESULT Close(LPCVOID) override { return S_OK; }
};

static std::string LoadText(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

static ID3D11ComputeShader* CompileCs(ID3D11Device* device, const std::string& text, const char* name) {
  FolderInclude include;
  ID3DBlob* code = nullptr;
  ID3DBlob* errors = nullptr;
  const HRESULT hr = D3DCompile(text.data(), text.size(), name, nullptr, &include, "main", "cs_5_0", 0u, 0u, &code, &errors);
  if (FAILED(hr)) {
    std::printf("FAIL compile %s hr=0x%08lx %s\n", name, static_cast<unsigned long>(hr),
                errors != nullptr ? static_cast<const char*>(errors->GetBufferPointer()) : "");
    ++g_failures;
    if (errors != nullptr) errors->Release();
    return nullptr;
  }
  ID3D11ComputeShader* shader = nullptr;
  device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader);
  code->Release();
  if (errors != nullptr) errors->Release();
  return shader;
}

static void CheckMessages(ID3D11InfoQueue* queue, const char* tag) {
  if (queue == nullptr) return;
  const UINT64 count = queue->GetNumStoredMessages();
  for (UINT64 i = 0; i < count; ++i) {
    SIZE_T length = 0;
    queue->GetMessage(i, nullptr, &length);
    std::vector<uint8_t> raw(length);
    auto* message = reinterpret_cast<D3D11_MESSAGE*>(raw.data());
    if (SUCCEEDED(queue->GetMessage(i, message, &length)) && message->Severity <= D3D11_MESSAGE_SEVERITY_ERROR) {
      ++g_failures;
      std::printf("FAIL %s: debug layer %.*s\n", tag, static_cast<int>(message->DescriptionByteLength), message->pDescription);
    }
  }
  queue->ClearStoredMessages();
}

static ID3D11Texture2D* MakeTexture(ID3D11Device* device, UINT w, UINT h, DXGI_FORMAT format, const void* init, UINT pitch) {
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = w;
  desc.Height = h;
  desc.MipLevels = 1u;
  desc.ArraySize = 1u;
  desc.Format = format;
  desc.SampleDesc.Count = 1u;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
  D3D11_SUBRESOURCE_DATA data = {init, pitch, 0u};
  ID3D11Texture2D* texture = nullptr;
  device->CreateTexture2D(&desc, init != nullptr ? &data : nullptr, &texture);
  return texture;
}

static ID3D11ShaderResourceView* MakeSrv(ID3D11Device* device, ID3D11Texture2D* texture, DXGI_FORMAT format) {
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = format;
  desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  desc.Texture2D.MipLevels = 1u;
  ID3D11ShaderResourceView* view = nullptr;
  device->CreateShaderResourceView(texture, &desc, &view);
  return view;
}

static ID3D11UnorderedAccessView* MakeUav(ID3D11Device* device, ID3D11Texture2D* texture, DXGI_FORMAT format) {
  D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
  desc.Format = format;
  desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
  ID3D11UnorderedAccessView* view = nullptr;
  device->CreateUnorderedAccessView(texture, &desc, &view);
  return view;
}

static ID3D11Buffer* MakeConstantBuffer(ID3D11Device* device, const void* init, UINT bytes) {
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = bytes;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  D3D11_SUBRESOURCE_DATA data = {init, 0u, 0u};
  ID3D11Buffer* buffer = nullptr;
  device->CreateBuffer(&desc, &data, &buffer);
  return buffer;
}

// Copies a texture to the CPU; bytes_per_pixel rows are tightly packed.
static std::vector<uint8_t> Readback(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* texture, UINT w, UINT h,
                                     DXGI_FORMAT format, UINT bytes_per_pixel) {
  std::vector<uint8_t> out(static_cast<size_t>(w) * h * bytes_per_pixel, 0u);
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = w;
  desc.Height = h;
  desc.MipLevels = 1u;
  desc.ArraySize = 1u;
  desc.Format = format;
  desc.SampleDesc.Count = 1u;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  ID3D11Texture2D* staging = nullptr;
  if (FAILED(device->CreateTexture2D(&desc, nullptr, &staging))) return out;
  context->CopyResource(staging, texture);
  D3D11_MAPPED_SUBRESOURCE mapped = {};
  if (SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
    for (UINT row = 0; row < h; ++row) {
      std::memcpy(&out[static_cast<size_t>(row) * w * bytes_per_pixel], static_cast<const uint8_t*>(mapped.pData) + row * mapped.RowPitch,
                  static_cast<size_t>(w) * bytes_per_pixel);
    }
    context->Unmap(staging, 0);
  }
  staging->Release();
  return out;
}

// Number of differing bytes of two equal-size outputs, compared word by word (word_bytes = 2 for rgba16f, 1 for rgba8).
static size_t CountMismatch(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, size_t word_bytes) {
  size_t mismatch = 0;
  if (a.size() != b.size()) return a.size() + b.size();
  for (size_t i = 0; i < a.size(); i += word_bytes) {
    if (std::memcmp(&a[i], &b[i], word_bytes) != 0) ++mismatch;
  }
  return mismatch;
}

// The inputs of one case: colour rgba (float, uploaded as half), guide rgba32f, normal rg (uploaded as half).
struct Scene {
  UINT w = 0, h = 0;
  std::vector<float> colour, guide, normal;
  float res_scale[2] = {1.f, 1.f};
  float luminance[3] = {0.f, 0.f, 0.f};
};

static uint16_t Half(float v) { return DirectX::PackedVector::XMConvertFloatToHalf(v); }

// Shared runner: t0 colour (rgba16f), t1 guide (rgba32f), t2 normal (rg16f), u0 output (rgba16f), b0 50 float4 (c49.xy = resolution
// scaling), b12 16 floats (c0 size, c1 (divisor 1, debug 0, history 0, step), c2 (0.2, 0.2, 0, 4), c3 luminance weights).
static std::vector<uint8_t> RunAtrous(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11ComputeShader* shader, const Scene& s,
                                      float step) {
  const size_t pixels = static_cast<size_t>(s.w) * s.h;
  std::vector<uint16_t> colour_half(pixels * 4u);
  for (size_t i = 0; i < colour_half.size(); ++i) colour_half[i] = Half(s.colour[i]);
  std::vector<uint16_t> normal_half(pixels * 2u);
  for (size_t i = 0; i < normal_half.size(); ++i) normal_half[i] = Half(s.normal[i]);
  ID3D11Texture2D* colour_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R16G16B16A16_FLOAT, colour_half.data(), s.w * 8u);
  ID3D11Texture2D* guide_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R32G32B32A32_FLOAT, s.guide.data(), s.w * 16u);
  ID3D11Texture2D* normal_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R16G16_FLOAT, normal_half.data(), s.w * 4u);
  ID3D11Texture2D* out_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R16G16B16A16_FLOAT, nullptr, 0u);
  ID3D11ShaderResourceView* srvs[3] = {MakeSrv(device, colour_tex, DXGI_FORMAT_R16G16B16A16_FLOAT),
                                       MakeSrv(device, guide_tex, DXGI_FORMAT_R32G32B32A32_FLOAT),
                                       MakeSrv(device, normal_tex, DXGI_FORMAT_R16G16_FLOAT)};
  ID3D11UnorderedAccessView* uav = MakeUav(device, out_tex, DXGI_FORMAT_R16G16B16A16_FLOAT);

  float scene[50 * 4] = {};
  scene[49 * 4 + 0] = s.res_scale[0];
  scene[49 * 4 + 1] = s.res_scale[1];
  const float push[16] = {static_cast<float>(s.w), static_cast<float>(s.h), static_cast<float>(s.w), static_cast<float>(s.h),
                          1.f, 0.f, 0.f, step,
                          0.2f, 0.2f, 0.f, 4.f,
                          s.luminance[0], s.luminance[1], s.luminance[2], 0.f};
  ID3D11Buffer* cb0 = MakeConstantBuffer(device, scene, sizeof(scene));
  ID3D11Buffer* cb12 = MakeConstantBuffer(device, push, sizeof(push));

  context->CSSetShader(shader, nullptr, 0u);
  context->CSSetConstantBuffers(0u, 1u, &cb0);
  context->CSSetConstantBuffers(12u, 1u, &cb12);
  context->CSSetShaderResources(0u, 3u, srvs);
  context->CSSetUnorderedAccessViews(0u, 1u, &uav, nullptr);
  context->Dispatch((s.w + 7u) / 8u, (s.h + 7u) / 8u, 1u);
  ID3D11ShaderResourceView* null_srvs[3] = {};
  ID3D11UnorderedAccessView* null_uav = nullptr;
  context->CSSetShaderResources(0u, 3u, null_srvs);
  context->CSSetUnorderedAccessViews(0u, 1u, &null_uav, nullptr);

  std::vector<uint8_t> out = Readback(device, context, out_tex, s.w, s.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 8u);
  for (ID3D11ShaderResourceView* v : srvs) v->Release();
  uav->Release();
  cb0->Release();
  cb12->Release();
  colour_tex->Release();
  guide_tex->Release();
  normal_tex->Release();
  out_tex->Release();
  return out;
}

// Variance runner: t0 integrated (rgba16f), t1 moments (rgba32f), t2 guide (rgba32f), t3 normal (rg16f); u0 output (rgba16f),
// u1 debug (rgba8unorm). b12 c1 = (1, debug, 0, 0). Returns u0 followed by u1.
struct VarianceInputs {
  Scene scene;
  std::vector<float> moments;  // mu1, mu2, h, 0
};

static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> RunVariance(ID3D11Device* device, ID3D11DeviceContext* context,
                                                                         ID3D11ComputeShader* shader, const VarianceInputs& in,
                                                                         float debug) {
  const Scene& s = in.scene;
  const size_t pixels = static_cast<size_t>(s.w) * s.h;
  std::vector<uint16_t> colour_half(pixels * 4u);
  for (size_t i = 0; i < colour_half.size(); ++i) colour_half[i] = Half(s.colour[i]);
  std::vector<uint16_t> normal_half(pixels * 2u);
  for (size_t i = 0; i < normal_half.size(); ++i) normal_half[i] = Half(s.normal[i]);
  ID3D11Texture2D* integrated_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R16G16B16A16_FLOAT, colour_half.data(), s.w * 8u);
  ID3D11Texture2D* moments_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R32G32B32A32_FLOAT, in.moments.data(), s.w * 16u);
  ID3D11Texture2D* guide_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R32G32B32A32_FLOAT, s.guide.data(), s.w * 16u);
  ID3D11Texture2D* normal_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R16G16_FLOAT, normal_half.data(), s.w * 4u);
  ID3D11Texture2D* out_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R16G16B16A16_FLOAT, nullptr, 0u);
  ID3D11Texture2D* debug_tex = MakeTexture(device, s.w, s.h, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr, 0u);
  ID3D11ShaderResourceView* srvs[4] = {MakeSrv(device, integrated_tex, DXGI_FORMAT_R16G16B16A16_FLOAT),
                                       MakeSrv(device, moments_tex, DXGI_FORMAT_R32G32B32A32_FLOAT),
                                       MakeSrv(device, guide_tex, DXGI_FORMAT_R32G32B32A32_FLOAT),
                                       MakeSrv(device, normal_tex, DXGI_FORMAT_R16G16_FLOAT)};
  ID3D11UnorderedAccessView* uavs[2] = {MakeUav(device, out_tex, DXGI_FORMAT_R16G16B16A16_FLOAT),
                                        MakeUav(device, debug_tex, DXGI_FORMAT_R8G8B8A8_UNORM)};

  float scene[50 * 4] = {};
  scene[49 * 4 + 0] = 1.f;
  scene[49 * 4 + 1] = 1.f;
  const float push[16] = {static_cast<float>(s.w), static_cast<float>(s.h), static_cast<float>(s.w), static_cast<float>(s.h),
                          1.f, debug, 0.f, 0.f,
                          0.2f, 0.2f, 0.f, 4.f,
                          0.f, 0.f, 0.f, 0.f};
  ID3D11Buffer* cb0 = MakeConstantBuffer(device, scene, sizeof(scene));
  ID3D11Buffer* cb12 = MakeConstantBuffer(device, push, sizeof(push));

  context->CSSetShader(shader, nullptr, 0u);
  context->CSSetConstantBuffers(0u, 1u, &cb0);
  context->CSSetConstantBuffers(12u, 1u, &cb12);
  context->CSSetShaderResources(0u, 4u, srvs);
  context->CSSetUnorderedAccessViews(0u, 2u, uavs, nullptr);
  context->Dispatch((s.w + 7u) / 8u, (s.h + 7u) / 8u, 1u);
  ID3D11ShaderResourceView* null_srvs[4] = {};
  ID3D11UnorderedAccessView* null_uavs[2] = {};
  context->CSSetShaderResources(0u, 4u, null_srvs);
  context->CSSetUnorderedAccessViews(0u, 2u, null_uavs, nullptr);

  std::pair<std::vector<uint8_t>, std::vector<uint8_t>> out;
  out.first = Readback(device, context, out_tex, s.w, s.h, DXGI_FORMAT_R16G16B16A16_FLOAT, 8u);
  out.second = Readback(device, context, debug_tex, s.w, s.h, DXGI_FORMAT_R8G8B8A8_UNORM, 4u);
  for (ID3D11ShaderResourceView* v : srvs) v->Release();
  for (ID3D11UnorderedAccessView* v : uavs) v->Release();
  cb0->Release();
  cb12->Release();
  integrated_tex->Release();
  moments_tex->Release();
  guide_tex->Release();
  normal_tex->Release();
  out_tex->Release();
  debug_tex->Release();
  return out;
}

// Case generator: fixed seed per (case, size). weights and the colour / alpha pattern follow the case.
enum class Pattern { kRgb, kAo, kAllNegative, kOneActive, kResolutionScaling };

static Scene MakeScene(UINT w, UINT h, Pattern pattern, uint32_t seed) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> u01(0.f, 1.f);
  Scene s;
  s.w = w;
  s.h = h;
  const size_t pixels = static_cast<size_t>(w) * h;
  s.colour.assign(pixels * 4u, 0.f);
  s.guide.assign(pixels * 4u, 0.f);
  s.normal.assign(pixels * 2u, 0.f);
  const size_t centre = static_cast<size_t>(h / 2u) * w + w / 2u;
  for (size_t i = 0; i < pixels; ++i) {
    float* c = &s.colour[i * 4u];
    c[0] = 4.f * u01(rng);
    c[1] = 4.f * u01(rng);
    c[2] = 4.f * u01(rng);
    if (pattern == Pattern::kAo) c[1] = c[2] = c[0];
    const float alpha = u01(rng);
    if (pattern == Pattern::kAllNegative) c[3] = -1.f;
    else if (pattern == Pattern::kOneActive) c[3] = i == centre ? alpha : -1.f;
    else c[3] = alpha < 0.2f ? -1.f : u01(rng);
    float* g = &s.guide[i * 4u];
    g[0] = u01(rng) < 0.1f ? 0.f : 0.1f + 99.9f * u01(rng);
    g[1] = 2.f * u01(rng) - 1.f;
    g[2] = 2.f * u01(rng) - 1.f;
    g[3] = u01(rng);
    s.normal[i * 2u] = 2.f * u01(rng) - 1.f;
    s.normal[i * 2u + 1u] = 2.f * u01(rng) - 1.f;
  }
  if (pattern == Pattern::kRgb || pattern == Pattern::kAo) {
    const float weights[3] = {0.2126f, 0.7152f, 0.0722f};
    const float ao[3] = {1.f, 0.f, 0.f};
    const float* lw = pattern == Pattern::kAo ? ao : weights;
    s.luminance[0] = lw[0];
    s.luminance[1] = lw[1];
    s.luminance[2] = lw[2];
  } else {
    s.luminance[0] = 0.2126f;
    s.luminance[1] = 0.7152f;
    s.luminance[2] = 0.0722f;
  }
  if (pattern == Pattern::kResolutionScaling) {
    s.res_scale[0] = 0.5f;
    s.res_scale[1] = 0.5f;
  }
  return s;
}

static const UINT kSizes[][2] = {{1, 1}, {5, 3}, {13, 11}, {67, 33}, {333, 187}, {641, 363}};
static const char* const kCaseNames[] = {"rgb_weights", "ao_weights", "all_alpha_negative", "one_active", "resolution_scaling"};
static const Pattern kPatterns[] = {Pattern::kRgb, Pattern::kAo, Pattern::kAllNegative, Pattern::kOneActive, Pattern::kResolutionScaling};
static const float kTileSteps[] = {1.f, 2.f, 4.f};
static const float kLiveSteps[] = {1.f, 2.f, 4.f, 8.f, 16.f};

static void TestAtrous(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11InfoQueue* queue) {
  ID3D11ComputeShader* ref = CompileCs(device, LoadText("reference/world_svgf_atrous_m2.cs_5_0.hlsl"), "reference atrous m2");
  ID3D11ComputeShader* live = CompileCs(device, LoadText(std::string(kShaderDir) + "world_svgf_atrous.cs_5_0.hlsl"), "live atrous");
  ID3D11ComputeShader* tile[3] = {};
  const char* tile_files[3] = {"world_svgf_atrous_s1.cs_5_0.hlsl", "world_svgf_atrous_s2.cs_5_0.hlsl", "world_svgf_atrous_s4.cs_5_0.hlsl"};
  for (int k = 0; k < 3; ++k) tile[k] = CompileCs(device, LoadText(std::string(kShaderDir) + tile_files[k]), tile_files[k]);
  if (ref == nullptr || live == nullptr || tile[0] == nullptr || tile[1] == nullptr || tile[2] == nullptr) {
    std::printf("FAIL atrous: shader compile\n");
    ++g_failures;
    return;
  }
  size_t total = 0;
  for (size_t c = 0; c < sizeof(kPatterns) / sizeof(kPatterns[0]); ++c) {
    for (size_t z = 0; z < sizeof(kSizes) / sizeof(kSizes[0]); ++z) {
      const uint32_t seed = static_cast<uint32_t>(1000u * c + z + 1u);
      const Scene s = MakeScene(kSizes[z][0], kSizes[z][1], kPatterns[c], seed);
      size_t tile_mismatch[3] = {};
      size_t live_mismatch[5] = {};
      for (int k = 0; k < 3; ++k) {
        const std::vector<uint8_t> want = RunAtrous(device, context, ref, s, kTileSteps[k]);
        const std::vector<uint8_t> got = RunAtrous(device, context, tile[k], s, kTileSteps[k]);
        tile_mismatch[k] = CountMismatch(want, got, 2u);
        total += tile_mismatch[k];
      }
      for (int k = 0; k < 5; ++k) {
        const std::vector<uint8_t> want = RunAtrous(device, context, ref, s, kLiveSteps[k]);
        const std::vector<uint8_t> got = RunAtrous(device, context, live, s, kLiveSteps[k]);
        live_mismatch[k] = CountMismatch(want, got, 2u);
        total += live_mismatch[k];
      }
      std::printf("atrous %-20s %3ux%-3u tile s1 %zu s2 %zu s4 %zu | live s1 %zu s2 %zu s4 %zu s8 %zu s16 %zu\n", kCaseNames[c],
                  kSizes[z][0], kSizes[z][1], tile_mismatch[0], tile_mismatch[1], tile_mismatch[2], live_mismatch[0], live_mismatch[1],
                  live_mismatch[2], live_mismatch[3], live_mismatch[4]);
      CheckMessages(queue, "atrous");
    }
  }
  std::printf("atrous mismatches total %zu\n", total);
  if (total != 0u) ++g_failures;
  ref->Release();
  live->Release();
  for (ID3D11ComputeShader* shader : tile) shader->Release();
}

// Variance inputs: integrated colour and alpha (20% not traced), moments with h from the pattern, the guide and normal.
enum class HistoryPattern { kBelowFour, kMixed, kNone };

static VarianceInputs MakeVariance(UINT w, UINT h, HistoryPattern pattern, uint32_t seed) {
  VarianceInputs in;
  in.scene = MakeScene(w, h, Pattern::kRgb, seed);
  std::mt19937 rng(seed * 7u + 3u);
  std::uniform_real_distribution<float> u01(0.f, 1.f);
  const float below[4] = {0.f, 1.f, 2.f, 3.f};
  const float mixed[6] = {0.f, 1.f, 2.f, 3.f, 4.f, 8.f};
  const float none[2] = {4.f, 8.f};
  const size_t pixels = static_cast<size_t>(w) * h;
  in.moments.assign(pixels * 4u, 0.f);
  for (size_t i = 0; i < pixels; ++i) {
    const float mu1 = 2.f * u01(rng);
    const float mu2 = mu1 * mu1 + u01(rng);
    float hist = 0.f;
    if (pattern == HistoryPattern::kBelowFour) hist = below[static_cast<size_t>(u01(rng) * 4.f) % 4u];
    else if (pattern == HistoryPattern::kMixed) hist = mixed[static_cast<size_t>(u01(rng) * 6.f) % 6u];
    else hist = none[static_cast<size_t>(u01(rng) * 2.f) % 2u];
    in.moments[i * 4u] = mu1;
    in.moments[i * 4u + 1u] = mu2;
    in.moments[i * 4u + 2u] = hist;
    in.moments[i * 4u + 3u] = 0.f;
  }
  return in;
}

static void TestVariance(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11InfoQueue* queue) {
  ID3D11ComputeShader* ref = CompileCs(device, LoadText("reference/world_svgf_variance_m2.cs_5_0.hlsl"), "reference variance m2");
  ID3D11ComputeShader* live = CompileCs(device, LoadText(std::string(kShaderDir) + "world_svgf_variance.cs_5_0.hlsl"), "live variance");
  if (ref == nullptr || live == nullptr) {
    std::printf("FAIL variance: shader compile\n");
    ++g_failures;
    return;
  }
  const char* pattern_names[] = {"h_below_4", "mixed", "none"};
  const HistoryPattern patterns[] = {HistoryPattern::kBelowFour, HistoryPattern::kMixed, HistoryPattern::kNone};
  size_t total = 0;
  for (size_t p = 0; p < 3; ++p) {
    for (size_t z = 0; z < sizeof(kSizes) / sizeof(kSizes[0]); ++z) {
      const uint32_t seed = static_cast<uint32_t>(5000u + 10u * p + z);
      const VarianceInputs in = MakeVariance(kSizes[z][0], kSizes[z][1], patterns[p], seed);
      size_t mismatch[2] = {};
      const float debugs[2] = {0.f, 11.f};
      for (size_t d = 0; d < 2; ++d) {
        const auto want = RunVariance(device, context, ref, in, debugs[d]);
        const auto got = RunVariance(device, context, live, in, debugs[d]);
        mismatch[d] = CountMismatch(want.first, got.first, 2u) + CountMismatch(want.second, got.second, 1u);
        total += mismatch[d];
      }
      std::printf("variance %-10s %3ux%-3u debug 0 %zu debug 11 %zu\n", pattern_names[p], kSizes[z][0], kSizes[z][1], mismatch[0],
                  mismatch[1]);
      CheckMessages(queue, "variance");
    }
  }
  std::printf("variance mismatches total %zu\n", total);
  if (total != 0u) ++g_failures;
  ref->Release();
  live->Release();
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

  TestAtrous(device, context, queue);
  TestVariance(device, context, queue);
  if (queue != nullptr) queue->Release();
  context->Release();
  device->Release();
  if (g_failures == 0) std::printf("ALL PASS test_gpu_svgf\n");
  return g_failures == 0 ? 0 : 1;
}
