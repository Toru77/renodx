#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

inline int g_failures = 0;
inline constexpr char kRepoDir[] = "../../src/games/falcomengine-plus/";  // taa/ and include/ are not in the harness copy

// Reads the #include files. The harness copy holds world/ only: the other folders (taa, include, the falcomengine-plus root) are
// read from the repository, with the leading "../" segments of the name dropped.
struct FolderInclude : ID3DInclude {
  std::deque<std::string> loaded;
  std::vector<std::string> dirs;
  static bool ReadFile(const std::string& path, std::string* out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    out->assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return true;
  }
  HRESULT Open(D3D_INCLUDE_TYPE, LPCSTR name, LPCVOID, LPCVOID* data, UINT* bytes) override {
    std::string n = name;
    while (n.rfind("../", 0) == 0) n = n.substr(3);
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

inline std::string LoadText(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

inline ID3D11ComputeShader* CompileCs(ID3D11Device* device, const std::string& text, const char* name, const std::vector<std::string>& include_dirs) {
  FolderInclude include;
  include.dirs = include_dirs;
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

inline void CheckMessages(ID3D11InfoQueue* queue, const char* tag) {
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

inline ID3D11Texture2D* MakeTexture(ID3D11Device* device, UINT w, UINT h, DXGI_FORMAT format, const void* init, UINT pitch) {
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

inline ID3D11ShaderResourceView* MakeSrv(ID3D11Device* device, ID3D11Texture2D* texture, DXGI_FORMAT format) {
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = format;
  desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  desc.Texture2D.MipLevels = 1u;
  ID3D11ShaderResourceView* view = nullptr;
  device->CreateShaderResourceView(texture, &desc, &view);
  return view;
}

inline ID3D11UnorderedAccessView* MakeUav(ID3D11Device* device, ID3D11Texture2D* texture, DXGI_FORMAT format) {
  D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
  desc.Format = format;
  desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
  ID3D11UnorderedAccessView* view = nullptr;
  device->CreateUnorderedAccessView(texture, &desc, &view);
  return view;
}

inline ID3D11Buffer* MakeConstantBuffer(ID3D11Device* device, const void* init, UINT bytes) {
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
inline std::vector<uint8_t> Readback(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* texture, UINT w, UINT h,
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
inline size_t CountMismatch(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, size_t word_bytes) {
  size_t mismatch = 0;
  if (a.size() != b.size()) return a.size() + b.size();
  for (size_t i = 0; i < a.size(); i += word_bytes) {
    if (std::memcmp(&a[i], &b[i], word_bytes) != 0) ++mismatch;
  }
  return mismatch;
}
