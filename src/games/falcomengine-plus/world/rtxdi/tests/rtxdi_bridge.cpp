#include <basetsd.h>
#include <combaseapi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcommon.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgiformat.h>
#include <handleapi.h>
#include <minwindef.h>
#include <winerror.h>
#include <winnt.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

#include "../rtxdi_input_d3d11.hpp"
#include "../../pt/pt_regir_contract.hpp"
#include "world_regir_selection_contract.h"
#include "world_regir_register.h"
#include "world_regir_weights.h"
#include "world_regir_build.h"

#include "world_rtxdi_pack_inputs.h"
#include "world_rtxdi_pack_records.h"
#include "world_rtxdi_unpack_records.h"

using Microsoft::WRL::ComPtr;

int RunRtxdiDxrBridge();

namespace {
struct SharedTexture {
  ComPtr<ID3D11Texture2D> texture11;
  ComPtr<ID3D11UnorderedAccessView> uav;
  ComPtr<ID3D12Resource> texture12;
};
}  // namespace

int main() {
  ComPtr<ID3D12Debug> debug;
  if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
    std::fprintf(stderr, "FAIL D3D12 debug layer unavailable\n");
    return 1;
  }
  debug->EnableDebugLayer();
  // Whole-buffer native bindings are reported as 4096 constants; exercise the
  // physical 80/81/95/96-row extents through the exact descriptor intersection.
  for (const auto& test : std::array<std::array<UINT, 5>, 4>{{{{13, 9, 1, 0, 80}}, {{13, 9, 1, 1, 81}}, {{1920, 1080, 0, 1, 95}}, {{1920, 1080, 1, 0, 96}}}}) {
    const UINT width = test[0];
    const UINT height = test[1];
    const bool local = test[2] != 0;
    const bool zero_lights = test[3] != 0;
    const UINT scene_rows = test[4];
    const UINT64 descriptor_scene_bytes = scene_rows == 80u || scene_rows == 96u ? UINT64_MAX : uint64_t{scene_rows} * 16u;
    using namespace falcom_world::rtxdi;
    constexpr UINT offset = 0;
    RtxdiInputLayout layout = {};
    if (!MakeInputLayout(width, height, false, local, zero_lights ? 0u : 1u, 1u, 1u, &layout)
        || layout.motion_available != 0u) {
      std::fprintf(stderr, "FAIL absent motion input layout\n");
      return 1;
    }
    const auto check = [](HRESULT hr, const char* operation) {
      if (FAILED(hr)) std::fprintf(stderr, "FAIL %s: HRESULT 0x%08lx\n", operation, static_cast<unsigned long>(hr));
      return SUCCEEDED(hr);
    };
    ComPtr<IDXGIFactory1> factory;
    if (!check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1")) return 1;
    ComPtr<IDXGIAdapter1> adapter;
    if (!check(factory->EnumAdapters1(0, &adapter), "EnumAdapters1")) return 1;
    ComPtr<ID3D11Device> d11;
    ComPtr<ID3D11DeviceContext> ctx;
    if (!check(D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                 &d11, nullptr, &ctx),
               "D3D11CreateDevice")) return 1;
    ComPtr<ID3D11DeviceContext1> ctx1;
    if (!check(ctx.As(&ctx1), "D3D11.1 context")) return 1;
    ComPtr<ID3D12Device> d12;
    HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d12));
    if (FAILED(hr)) {
      std::fprintf(stderr, "SKIP D3D12 unavailable: 0x%08lx\n", static_cast<unsigned long>(hr));
      return 77;
    }
    ComPtr<ID3D12InfoQueue> info_queue;
    if (!check(d12.As(&info_queue), "query D3D12 InfoQueue")) return 1;
    info_queue->ClearStoredMessages();
    D3D12_FEATURE_DATA_SHADER_MODEL model = {D3D_SHADER_MODEL_6_5};
    hr = d12->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &model, sizeof(model));
    if (FAILED(hr) || model.HighestShaderModel < D3D_SHADER_MODEL_6_5) {
      std::fprintf(stderr, "SKIP D3D12 shader model 6.5 unavailable: 0x%08lx, model 0x%x\n", static_cast<unsigned long>(hr), model.HighestShaderModel);
      return 77;
    }
    ComPtr<ID3D11ComputeShader> pack_inputs;
    ComPtr<ID3D11ComputeShader> pack_records;
    if (!check(d11->CreateComputeShader(__world_rtxdi_pack_inputs.data(), __world_rtxdi_pack_inputs.size(), nullptr, &pack_inputs), "create input shader")
        || !check(d11->CreateComputeShader(__world_rtxdi_pack_records.data(), __world_rtxdi_pack_records.size(), nullptr, &pack_records), "create record shader")) return 1;
    const auto shared = [&](DXGI_FORMAT format, UINT w, UINT h, SharedTexture* out) {
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = w;
      desc.Height = h;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = format;
      desc.SampleDesc.Count = 1;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
      desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
      HRESULT result = d11->CreateTexture2D(&desc, nullptr, &out->texture11);
      if (FAILED(result)) {
        std::fprintf(stderr, "shared stage line %d: 0x%08lx\n", __LINE__, static_cast<unsigned long>(result));
        return result;
      }
      result = d11->CreateUnorderedAccessView(out->texture11.Get(), nullptr, &out->uav);
      if (FAILED(result)) {
        std::fprintf(stderr, "shared stage line %d: 0x%08lx\n", __LINE__, static_cast<unsigned long>(result));
        return result;
      }
      ComPtr<IDXGIResource1> resource;
      result = out->texture11.As(&resource);
      if (FAILED(result)) {
        std::fprintf(stderr, "shared stage line %d: 0x%08lx\n", __LINE__, static_cast<unsigned long>(result));
        return result;
      }
      HANDLE handle = nullptr;
      result = resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &handle);
      if (FAILED(result)) {
        std::fprintf(stderr, "shared stage line %d: 0x%08lx\n", __LINE__, static_cast<unsigned long>(result));
        return result;
      }
      result = d12->OpenSharedHandle(handle, IID_PPV_ARGS(&out->texture12));
      CloseHandle(handle);
      return result;
    };
    std::array<SharedTexture, 9> carriers;
    const DXGI_FORMAT formats[] = {
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_UINT,
    };
    for (UINT i = 0; i < carriers.size(); ++i) {
      const UINT w = (i == 5 ? 96 : (i >= 6 ? 4096 : width));
      const UINT h = (i >= 5 ? 1 : height);
      hr = shared(i == 4 ? DXGI_FORMAT_R32G32B32A32_UINT : formats[i], w, h, &carriers[i]);
      if (FAILED(hr)) {
        std::fprintf(stderr, "SKIP shared texture %u format %u: 0x%08lx\n", i, formats[i], static_cast<unsigned long>(hr));
        return 77;
      }
    }
    std::array<ComPtr<ID3D11ShaderResourceView>, 5> pixels;
    std::array<std::vector<uint32_t>, 5> pixel_words;
    const UINT components[] = {1, 4, 4, 4, 2};
    for (UINT i = 0; i < pixels.size(); ++i) {
      pixel_words[i].resize(width * height * components[i]);
      for (UINT j = 0; j < pixel_words[i].size(); ++j) pixel_words[i][j] = (i == 0 || i == 2 ? 0x3f000000u + j : 0x80000000u + (0x01000000u * i) + j);
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = width;
      desc.Height = height;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = formats[i];
      desc.SampleDesc.Count = 1;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      D3D11_SUBRESOURCE_DATA const initial = {.pSysMem = pixel_words[i].data(), .SysMemPitch = width * components[i] * 4, .SysMemSlicePitch = 0};
      ComPtr<ID3D11Texture2D> texture;
      if (!check(d11->CreateTexture2D(&desc, &initial, &texture), "create pixel source")
          || !check(d11->CreateShaderResourceView(texture.Get(), nullptr, &pixels[i]), "create pixel SRV")) return 1;
    }
    std::vector<uint32_t> scene((offset + scene_rows * 16u) / 4);
    for (UINT i = 0; i < scene.size(); ++i) scene[i] = 0x12340000u + i;
    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.ByteWidth = static_cast<UINT>(scene.size() * 4);
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA const initial_scene = {.pSysMem = scene.data(), .SysMemPitch = 0, .SysMemSlicePitch = 0};
    ComPtr<ID3D11Buffer> scene_cb;
    if (!check(d11->CreateBuffer(&cb_desc, &initial_scene, &scene_cb), "create scene CB")
        || !ValidateSceneBuffer(scene_cb.Get(), offset, scene_rows * 16u)) return 1;
    ID3D11Buffer* bound_scene = scene_cb.Get();
    const UINT first_scene_constant = 0u;
    const UINT scene_constant_count = 4096u;
    ctx->PSSetConstantBuffers(0u, 1u, &bound_scene);
    SceneB0BindingEvidence native_scene_evidence = {};
    if (!ValidateSceneB0Binding(ctx.Get(), scene_cb.Get(), offset, RTXDI_INPUT_SCENE_REQUIRED_ROWS, &native_scene_evidence)
        || !native_scene_evidence.context_query_available || !native_scene_evidence.buffer_available
        || native_scene_evidence.native_buffer != reinterpret_cast<uint64_t>(scene_cb.Get())
        || native_scene_evidence.first_constant != first_scene_constant
        || native_scene_evidence.constant_count != scene_constant_count || native_scene_evidence.buffer_bytes != cb_desc.ByteWidth
        || ValidateSceneB0Binding(ctx.Get(), scene_cb.Get(), 16u, RTXDI_INPUT_SCENE_REQUIRED_ROWS)) {
      std::fprintf(stderr, "FAIL native b0 range evidence offset=%u count=%u bytes=%u\n",
                   native_scene_evidence.first_constant, native_scene_evidence.constant_count,
                   native_scene_evidence.buffer_bytes);
      return 1;
    }
    std::printf("PASS native D3D11 b0 evidence offset=%u constants=%u byte_width=%u\n",
                native_scene_evidence.first_constant * 16u, native_scene_evidence.constant_count,
                native_scene_evidence.buffer_bytes);
    SceneCopyWindow scene_copy_window = {};
    if (!MakeSceneCopyWindow(cb_desc.ByteWidth, offset, descriptor_scene_bytes,
                             native_scene_evidence.first_constant, native_scene_evidence.constant_count,
                             &scene_copy_window)
        || scene_copy_window.copied_rows != scene_rows) {
      std::fprintf(stderr, "FAIL scene copy window rows=%u expected=%u\n", scene_copy_window.copied_rows, scene_rows);
      return 1;
    }
    if (scene_rows == 80u) {
      D3D11_BUFFER_DESC explicit_desc = {};
      explicit_desc.ByteWidth = 1536u;
      explicit_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      ComPtr<ID3D11Buffer> explicit_cb;
      if (!check(d11->CreateBuffer(&explicit_desc, nullptr, &explicit_cb), "create explicit-range scene CB")) return 1;
      ID3D11Buffer* explicit_buffer = explicit_cb.Get();
      UINT explicit_first = 16u;
      UINT explicit_count = 80u;
      ctx1->PSSetConstantBuffers1(0u, 1u, &explicit_buffer, &explicit_first, &explicit_count);
      SceneB0BindingEvidence explicit_evidence = {};
      SceneCopyWindow explicit_window = {};
      if (!ValidateSceneB0Binding(ctx.Get(), explicit_cb.Get(), 256u, 80u, &explicit_evidence)
          || explicit_evidence.first_constant != 16u || explicit_evidence.constant_count != 80u
          || explicit_evidence.buffer_bytes != 1536u
          || !MakeSceneCopyWindow(1536u, 256u, UINT64_MAX, explicit_evidence.first_constant,
                                  explicit_evidence.constant_count, &explicit_window)
          || explicit_window.copied_rows != 80u) return 1;
      explicit_first = 0u;
      explicit_count = 96u;
      ctx1->PSSetConstantBuffers1(0u, 1u, &explicit_buffer, &explicit_first, &explicit_count);
      explicit_evidence = {};
      explicit_window = {};
      if (!ValidateSceneB0Binding(ctx.Get(), explicit_cb.Get(), 0u, 80u, &explicit_evidence)
          || explicit_evidence.first_constant != 0u || explicit_evidence.constant_count != 96u
          || explicit_evidence.buffer_bytes != 1536u
          || !MakeSceneCopyWindow(1536u, 0u, UINT64_MAX, explicit_evidence.first_constant,
                                  explicit_evidence.constant_count, &explicit_window)
          || explicit_window.copied_rows != 96u) return 1;
      ctx->PSSetConstantBuffers(0u, 1u, &bound_scene);
      std::printf("PASS explicit D3D11 b0 ranges first=16/count=80 and first=0/count=96\n");
    }
    const SceneRangeSnapshot promoted = {.buffer = 1, .offset = offset, .size = descriptor_scene_bytes, .frame = 4, .normalized_size = uint64_t{scene_rows} * 16u, .command = 1, .native_first_constant = first_scene_constant, .native_constant_count = scene_constant_count, .copied_rows = scene_rows, .live = true, .known = true, .sequence = 1};
    if (CheckPromotedSceneBinding(promoted, promoted, 4, 1, cb_desc.ByteWidth) != SceneBindingStatus::Ready) {
      std::fprintf(stderr, "FAIL valid CBV binding rejected\n");
      return 1;
    }
    auto rebound = promoted;
    rebound.sequence++;
    if (CheckPromotedSceneBinding(promoted, rebound, 4, 1, cb_desc.ByteWidth) != SceneBindingStatus::SceneBindingChanged) {
      std::fprintf(stderr, "FAIL CBV rebind was accepted\n");
      return 1;
    }
    const UINT strides[] = {112, 576, 144};
    std::array<std::vector<uint32_t>, 3> records;
    std::array<ComPtr<ID3D11ShaderResourceView>, 3> record_views;
    for (UINT i = 0; i < records.size(); ++i) {
      records[i].resize(strides[i] / 4 * 2);
      for (UINT j = 0; j < records[i].size(); ++j) records[i][j] = 0xa0000000u + (i * 0x01000000u) + j;
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth = static_cast<UINT>(records[i].size() * 4);
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
      desc.StructureByteStride = strides[i];
      D3D11_SUBRESOURCE_DATA const data = {.pSysMem = records[i].data(), .SysMemPitch = 0, .SysMemSlicePitch = 0};
      ComPtr<ID3D11Buffer> buffer;
      D3D11_SHADER_RESOURCE_VIEW_DESC view = {};
      view.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      view.Buffer.FirstElement = 1;
      view.Buffer.NumElements = 1;
      if (!check(d11->CreateBuffer(&desc, &data, &buffer), "create structured buffer")
          || !check(d11->CreateShaderResourceView(buffer.Get(), &view, &record_views[i]), "create structured SRV")) return 1;
      StructuredInputRange range;
      StructuredViewEvidence evidence = {};
      if (!ValidateStructuredView(record_views[i].Get(), strides[i], &range, &evidence) || range.first_element != 1
          || evidence.failure != DiagnosticStructuredFailure::None || evidence.expected_stride != strides[i]
          || evidence.view_dimension != D3D11_SRV_DIMENSION_BUFFER || evidence.format != DXGI_FORMAT_UNKNOWN
          || evidence.first_element != 1u || evidence.element_count != 1u
          || (evidence.evidence_flags & 111u) != 111u || evidence.buffer_bytes != strides[i] * 2u
          || evidence.element_offset != strides[i] || evidence.element_bytes != strides[i]
          || evidence.structure_stride != strides[i]) {
        std::fprintf(stderr, "FAIL valid structured SRV evidence i=%u dim=%u fmt=%u flags=0x%x expected=%u actual=%u bytes=%llu first=%u count=%u offset=%llu length=%llu failure=%u\n",
                     i, evidence.view_dimension, evidence.format, evidence.evidence_flags, evidence.expected_stride,
                     evidence.structure_stride, static_cast<unsigned long long>(evidence.buffer_bytes),
                     evidence.first_element, evidence.element_count,
                     static_cast<unsigned long long>(evidence.element_offset),
                     static_cast<unsigned long long>(evidence.element_bytes), static_cast<uint32_t>(evidence.failure));
        return 1;
      }
    }
    {
      StructuredInputRange invalid_range = {};
      StructuredViewEvidence evidence = {};
      D3D11_TEXTURE2D_DESC texture_desc = {};
      texture_desc.Width = texture_desc.Height = texture_desc.MipLevels = texture_desc.ArraySize = 1u;
      texture_desc.Format = DXGI_FORMAT_R32_FLOAT;
      texture_desc.SampleDesc.Count = 1u;
      texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      ComPtr<ID3D11Texture2D> texture;
      ComPtr<ID3D11ShaderResourceView> texture_view;
      if (!check(d11->CreateTexture2D(&texture_desc, nullptr, &texture), "create wrong-dimension resource")
          || !check(d11->CreateShaderResourceView(texture.Get(), nullptr, &texture_view), "create wrong-dimension SRV")
          || ValidateStructuredView(texture_view.Get(), RTXDI_INPUT_LIGHT_STRIDE, &invalid_range, &evidence)
          || evidence.failure != DiagnosticStructuredFailure::WrongViewDimension
          || evidence.view_dimension != D3D11_SRV_DIMENSION_TEXTURE2D || evidence.format != DXGI_FORMAT_R32_FLOAT
          || (evidence.evidence_flags & 3u) != 3u || (evidence.evidence_flags & 4u) != 0u) {
        std::fprintf(stderr, "FAIL structured SRV wrong-dimension evidence\n");
        return 1;
      }
      std::printf("PASS structured SRV wrong dimension evidence includes resource type\n");

      D3D11_BUFFER_DESC raw_desc = {};
      raw_desc.ByteWidth = 16u;
      raw_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      raw_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
      ComPtr<ID3D11Buffer> raw_buffer;
      D3D11_SHADER_RESOURCE_VIEW_DESC raw_view_desc = {};
      raw_view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
      raw_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
      raw_view_desc.BufferEx.NumElements = 4u;
      raw_view_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
      ComPtr<ID3D11ShaderResourceView> raw_view;
      evidence = {};
      if (!check(d11->CreateBuffer(&raw_desc, nullptr, &raw_buffer), "create raw SRV buffer")
          || !check(d11->CreateShaderResourceView(raw_buffer.Get(), &raw_view_desc, &raw_view), "create raw SRV")
          || ValidateStructuredView(raw_view.Get(), RTXDI_INPUT_LIGHT_STRIDE, &invalid_range, &evidence)
          || evidence.failure != DiagnosticStructuredFailure::WrongViewDimension
          || evidence.view_dimension != D3D11_SRV_DIMENSION_BUFFEREX
          || evidence.bufferex_flags != D3D11_BUFFEREX_SRV_FLAG_RAW || (evidence.evidence_flags & 16u) == 0u
          || (evidence.evidence_flags & 15u) != 15u || evidence.buffer_bytes != raw_desc.ByteWidth
          || evidence.misc_flags != raw_desc.MiscFlags) {
        std::fprintf(stderr, "FAIL structured SRV BUFFEREX flags evidence\n");
        return 1;
      }
      std::printf("PASS structured SRV BUFFEREX raw flag evidence\n");

      D3D11_BUFFER_DESC typed_desc = {};
      typed_desc.ByteWidth = 16u;
      typed_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      ComPtr<ID3D11Buffer> typed_buffer;
      D3D11_SHADER_RESOURCE_VIEW_DESC typed_view_desc = {};
      typed_view_desc.Format = DXGI_FORMAT_R32_UINT;
      typed_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      typed_view_desc.Buffer.NumElements = 4u;
      ComPtr<ID3D11ShaderResourceView> typed_view;
      if (!check(d11->CreateBuffer(&typed_desc, nullptr, &typed_buffer), "create typed SRV buffer")
          || !check(d11->CreateShaderResourceView(typed_buffer.Get(), &typed_view_desc, &typed_view), "create typed SRV")
          || ValidateStructuredView(typed_view.Get(), RTXDI_INPUT_LIGHT_STRIDE, &invalid_range, &evidence)
          || evidence.failure != DiagnosticStructuredFailure::WrongFormat
          || evidence.view_dimension != D3D11_SRV_DIMENSION_BUFFER || evidence.format != DXGI_FORMAT_R32_UINT
          || (evidence.evidence_flags & 15u) != 15u || evidence.buffer_bytes != typed_desc.ByteWidth
          || evidence.bind_flags != typed_desc.BindFlags || evidence.misc_flags != typed_desc.MiscFlags) {
        std::fprintf(stderr, "FAIL structured SRV wrong-format evidence\n");
        return 1;
      }
      std::printf("PASS structured SRV wrong format evidence includes buffer descriptor\n");

      evidence = {};
      if (ValidateStructuredView(record_views[1].Get(), RTXDI_INPUT_LIGHT_STRIDE, &invalid_range, &evidence)
          || evidence.failure != DiagnosticStructuredFailure::StructureStrideMismatch
          || evidence.expected_stride != RTXDI_INPUT_LIGHT_STRIDE || evidence.structure_stride != RTXDI_INPUT_TILE_STRIDE
          || evidence.element_offset != RTXDI_INPUT_LIGHT_STRIDE || evidence.element_bytes != RTXDI_INPUT_LIGHT_STRIDE) {
        std::fprintf(stderr, "FAIL structured SRV stride evidence\n");
        return 1;
      }
      std::printf("PASS structured SRV stride mismatch evidence\n");

      evidence = {};
      if (ValidateStructuredView(nullptr, RTXDI_INPUT_LIGHT_STRIDE, &invalid_range, &evidence)
          || evidence.failure != DiagnosticStructuredFailure::NullArgument || evidence.evidence_flags != 128u) {
        std::fprintf(stderr, "FAIL structured SRV null-input evidence\n");
        return 1;
      }
      if (ValidStructuredInput({.buffer_bytes = 224u, .first_element = 1u, .element_count = 2u, .stride = 112u}, 112u)
          || ValidStructuredInput({.buffer_bytes = UINT64_MAX, .first_element = UINT32_MAX, .element_count = UINT32_MAX, .stride = UINT32_MAX}, UINT32_MAX)) {
        std::fprintf(stderr, "FAIL structured SRV element-bounds contract\n");
        return 1;
      }
      D3D11_BUFFER_DESC bounded_desc = {};
      bounded_desc.ByteWidth = 224u;
      bounded_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      bounded_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
      bounded_desc.StructureByteStride = 112u;
      ComPtr<ID3D11Buffer> bounded_buffer;
      D3D11_SHADER_RESOURCE_VIEW_DESC out_of_bounds_desc = {};
      out_of_bounds_desc.Format = DXGI_FORMAT_UNKNOWN;
      out_of_bounds_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      out_of_bounds_desc.Buffer.FirstElement = 1u;
      out_of_bounds_desc.Buffer.NumElements = 2u;
      ComPtr<ID3D11ShaderResourceView> out_of_bounds_view;
      if (!check(d11->CreateBuffer(&bounded_desc, nullptr, &bounded_buffer), "create bounded structured buffer")) return 1;
      const HRESULT out_of_bounds_create = d11->CreateShaderResourceView(bounded_buffer.Get(), &out_of_bounds_desc,
                                                                         &out_of_bounds_view);
      if (SUCCEEDED(out_of_bounds_create)) {
        evidence = {};
        if (ValidateStructuredView(out_of_bounds_view.Get(), 112u, &invalid_range, &evidence)
            || evidence.failure != DiagnosticStructuredFailure::ElementBoundsInvalid
            || evidence.element_offset != 112u || evidence.element_bytes != 224u
            || (evidence.evidence_flags & 32u) == 0u || (evidence.evidence_flags & 64u) != 0u) {
          std::fprintf(stderr, "FAIL structured SRV element-bounds evidence\n");
          return 1;
        }
        std::printf("PASS native structured SRV out-of-bounds descriptor rejected with byte-range evidence\n");
      } else {
        std::printf("PASS native D3D11 rejected out-of-bounds SRV creation; pure byte-range contract verified\n");
      }
      std::printf("PASS structured SRV invalid bounds, null input and arithmetic overflow guard\n");
    }
    D3D11_BUFFER_DESC constants_desc = {};
    constants_desc.ByteWidth = 32;
    constants_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ComPtr<ID3D11Buffer> constants;
    if (!check(d11->CreateBuffer(&constants_desc, nullptr, &constants), "create constants")) return 1;
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;
    if (!check(d12->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)), "create queue")
        || !check(d12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)), "create allocator")
        || !check(d12->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)), "create command list")
        || !check(d12->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "create fence")) return 1;
    D3D12_DESCRIPTOR_RANGE ranges[2] = {};
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = 4;
    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[1].NumDescriptors = 4;
    ranges[1].OffsetInDescriptorsFromTableStart = 4;
    D3D12_ROOT_PARAMETER root_params[2] = {};
    root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[0].Constants.Num32BitValues = 4;
    root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_params[1].DescriptorTable.NumDescriptorRanges = 2;
    root_params[1].DescriptorTable.pDescriptorRanges = ranges;
    D3D12_ROOT_SIGNATURE_DESC root_desc = {};
    root_desc.NumParameters = 2;
    root_desc.pParameters = root_params;
    ComPtr<ID3DBlob> root_blob;
    ComPtr<ID3DBlob> errors;
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pso;
    if (!check(D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_blob, &errors), "serialize root")
        || !check(d12->CreateRootSignature(0, root_blob->GetBufferPointer(), root_blob->GetBufferSize(), IID_PPV_ARGS(&root)), "create root")) return 1;
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = root.Get();
    pso_desc.CS = {.pShaderBytecode = __world_rtxdi_unpack_records.data(), .BytecodeLength = __world_rtxdi_unpack_records.size()};
    if (!check(d12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&pso)), "create unpack pipeline")) return 1;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = 8;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> heap;
    if (!check(d12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)), "create descriptor heap")) return 1;
    D3D12_DESCRIPTOR_HEAP_DESC clear_heap_desc = heap_desc;
    clear_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ComPtr<ID3D12DescriptorHeap> clear_heap;
    if (!check(d12->CreateDescriptorHeap(&clear_heap_desc, IID_PPV_ARGS(&clear_heap)), "create clear descriptor heap")) return 1;
    const UINT descriptor_size = d12->GetDescriptorHandleIncrementSize(heap_desc.Type);
    std::array<ComPtr<ID3D12Resource>, 4> output;
    std::array<ComPtr<ID3D12Resource>, 4> readback;
    const UINT sizes[] = {1536, 112, 576, 144};
    for (UINT i = 0; i < 4; ++i) {
      D3D12_HEAP_PROPERTIES props = {};
      props.Type = D3D12_HEAP_TYPE_DEFAULT;
      D3D12_RESOURCE_DESC desc = {};
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Width = sizes[i];
      desc.Height = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels = 1;
      desc.SampleDesc.Count = 1;
      desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
      if (!check(d12->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&output[i])), "create output buffer")) return 1;
      props.Type = D3D12_HEAP_TYPE_READBACK;
      desc.Flags = D3D12_RESOURCE_FLAG_NONE;
      if (!check(d12->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback[i])), "create readback buffer")) return 1;
      auto handle = heap->GetCPUDescriptorHandleForHeapStart();
      handle.ptr += static_cast<SIZE_T>(i) * descriptor_size;
      D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
      srv.Format = DXGI_FORMAT_R32G32B32A32_UINT;
      srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      srv.Texture2D.MipLevels = 1;
      d12->CreateShaderResourceView(carriers[5 + i].texture12.Get(), &srv, handle);
      handle.ptr += static_cast<SIZE_T>(4) * descriptor_size;
      D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
      uav.Format = DXGI_FORMAT_R32_TYPELESS;
      uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
      uav.Buffer.NumElements = sizes[i] / 4;
      uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
      d12->CreateUnorderedAccessView(output[i].Get(), nullptr, &uav, handle);
      auto clear_handle = clear_heap->GetCPUDescriptorHandleForHeapStart();
      clear_handle.ptr += static_cast<SIZE_T>(i) * descriptor_size;
      d12->CreateUnorderedAccessView(output[i].Get(), nullptr, &uav, clear_handle);
    }
    std::array<ComPtr<ID3D12Resource>, 5> pixel_readback;
    std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT, 5> footprints = {};
    for (UINT i = 0; i < 5; ++i) {
      UINT64 bytes = 0;
      const auto texture_desc = carriers[i].texture12->GetDesc();
      d12->GetCopyableFootprints(&texture_desc, 0, 1, 0, &footprints[i], nullptr, nullptr, &bytes);
      D3D12_HEAP_PROPERTIES props = {};
      props.Type = D3D12_HEAP_TYPE_READBACK;
      D3D12_RESOURCE_DESC desc = {};
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Width = bytes;
      desc.Height = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels = 1;
      desc.SampleDesc.Count = 1;
      desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      if (!check(d12->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pixel_readback[i])), "create pixel readback")) return 1;
    }
    UINT first = offset / 16;
    UINT count = 1536 / 16;
    ID3D11Buffer* scene_ptr = scene_cb.Get();
    ctx1->CSSetConstantBuffers1(0, 1, &scene_ptr, &first, &count);
    ID3D11Buffer* constants_ptr = constants.Get();
    ctx->CSSetConstantBuffers(12, 1, &constants_ptr);
    ID3D11ShaderResourceView* pixel_srvs[] = {pixels[0].Get(), pixels[1].Get(), pixels[2].Get(), pixels[3].Get(), pixels[4].Get(), nullptr};
    ID3D11UnorderedAccessView* pixel_uavs[] = {carriers[0].uav.Get(), carriers[1].uav.Get(), carriers[2].uav.Get(), carriers[3].uav.Get(), carriers[4].uav.Get(), carriers[5].uav.Get(), nullptr};
    UINT params[] = {width, height, local ? 1u : 0u, 0u, scene_rows};
    ctx->UpdateSubresource(constants.Get(), 0, nullptr, params, 0, 0);
    ctx->CSSetShader(pack_inputs.Get(), nullptr, 0);
    ctx->CSSetShaderResources(0, 6, pixel_srvs);
    ctx->CSSetUnorderedAccessViews(0, 7, pixel_uavs, nullptr);
    ctx->Dispatch((width + 7) / 8, (height + 7) / 8, 1);
    ID3D11ShaderResourceView* null_srvs[6] = {};
    ID3D11UnorderedAccessView* null_uavs[7] = {};
    ctx->CSSetShaderResources(0, 6, null_srvs);
    ctx->CSSetUnorderedAccessViews(0, 7, null_uavs, nullptr);
    ID3D11ShaderResourceView* record_srvs[] = {record_views[0].Get(), record_views[1].Get(), record_views[2].Get()};
    ID3D11UnorderedAccessView* record_uavs[] = {carriers[6].uav.Get(), carriers[7].uav.Get(), carriers[8].uav.Get()};
    UINT counts[] = {local && !zero_lights ? 1u : 0u, local ? 1u : 0u, local ? 1u : 0u, 0};
    ctx->UpdateSubresource(constants.Get(), 0, nullptr, counts, 0, 0);
    if (local) {
      ctx->CSSetShader(pack_records.Get(), nullptr, 0);
      ctx->CSSetShaderResources(0, 3, record_srvs);
      ctx->CSSetUnorderedAccessViews(0, 3, record_uavs, nullptr);
      ctx->Dispatch(1, 1, 1);
    }
    ctx->CSSetShaderResources(0, 3, null_srvs);
    ctx->CSSetUnorderedAccessViews(0, 3, null_uavs, nullptr);
    if (local && !zero_lights && width == 13u && height == 9u && scene_rows == 80u) {
      constexpr UINT kLightElementCount = 1024u;
      constexpr UINT kLightWordsPerElement = RTXDI_INPUT_LIGHT_STRIDE / sizeof(uint32_t);
      std::vector<uint32_t> large_lights(kLightElementCount * kLightWordsPerElement);
      for (UINT i = 0; i < large_lights.size(); ++i) large_lights[i] = 0x51000000u + i;
      D3D11_BUFFER_DESC large_light_desc = {};
      large_light_desc.ByteWidth = static_cast<UINT>(large_lights.size() * sizeof(uint32_t));
      large_light_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      large_light_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
      large_light_desc.StructureByteStride = RTXDI_INPUT_LIGHT_STRIDE;
      D3D11_SUBRESOURCE_DATA large_light_data = {.pSysMem = large_lights.data(), .SysMemPitch = 0, .SysMemSlicePitch = 0};
      ComPtr<ID3D11Buffer> large_light_buffer;
      if (!check(d11->CreateBuffer(&large_light_desc, &large_light_data, &large_light_buffer),
                 "create production-sized Lights buffer")) return 1;
      D3D11_SHADER_RESOURCE_VIEW_DESC buffer_view_desc = {};
      buffer_view_desc.Format = DXGI_FORMAT_UNKNOWN;
      buffer_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      buffer_view_desc.Buffer.FirstElement = 1u;
      buffer_view_desc.Buffer.NumElements = kLightElementCount - 1u;
      D3D11_SHADER_RESOURCE_VIEW_DESC bufferex_view_desc = {};
      bufferex_view_desc.Format = DXGI_FORMAT_UNKNOWN;
      bufferex_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
      bufferex_view_desc.BufferEx.FirstElement = buffer_view_desc.Buffer.FirstElement;
      bufferex_view_desc.BufferEx.NumElements = buffer_view_desc.Buffer.NumElements;
      bufferex_view_desc.BufferEx.Flags = 0u;
      std::array<ComPtr<ID3D11ShaderResourceView>, 2> large_light_views;
      if (!check(d11->CreateShaderResourceView(large_light_buffer.Get(), &buffer_view_desc, &large_light_views[0]),
                 "create production-sized Lights BUFFER SRV")
          || !check(d11->CreateShaderResourceView(large_light_buffer.Get(), &bufferex_view_desc, &large_light_views[1]),
                    "create production-sized Lights BUFFEREX SRV")) return 1;
      std::array<StructuredInputRange, 2> large_light_ranges = {};
      for (UINT i = 0; i < large_light_views.size(); ++i) {
        StructuredViewEvidence evidence = {};
        if (!ValidateStructuredView(large_light_views[i].Get(), RTXDI_INPUT_LIGHT_STRIDE, &large_light_ranges[i], &evidence)
            || large_light_ranges[i].buffer_bytes != 114688u || large_light_ranges[i].first_element != 1u
            || large_light_ranges[i].element_count != 1023u || large_light_ranges[i].stride != 112u
            || evidence.view_dimension != (i == 0 ? D3D11_SRV_DIMENSION_BUFFER : D3D11_SRV_DIMENSION_BUFFEREX)
            || evidence.format != DXGI_FORMAT_UNKNOWN || (i == 1 && evidence.bufferex_flags != 0u)) {
          std::fprintf(stderr, "FAIL production Lights view form=%u dim=%u flags=%u failure=%u bytes=%llu range=%u/%u stride=%u\n",
                       i, evidence.view_dimension, evidence.bufferex_flags, static_cast<uint32_t>(evidence.failure),
                       static_cast<unsigned long long>(large_light_ranges[i].buffer_bytes),
                       large_light_ranges[i].first_element, large_light_ranges[i].element_count, large_light_ranges[i].stride);
          return 1;
        }
      }
      if (large_light_ranges[0].buffer_bytes != large_light_ranges[1].buffer_bytes
          || large_light_ranges[0].first_element != large_light_ranges[1].first_element
          || large_light_ranges[0].element_count != large_light_ranges[1].element_count
          || large_light_ranges[0].stride != large_light_ranges[1].stride) {
        std::fprintf(stderr, "FAIL BUFFER and zero-flag BUFFEREX structured ranges differ\n");
        return 1;
      }
      D3D11_TEXTURE2D_DESC packed_desc = {};
      carriers[6].texture11->GetDesc(&packed_desc);
      packed_desc.Usage = D3D11_USAGE_STAGING;
      packed_desc.BindFlags = 0u;
      packed_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      packed_desc.MiscFlags = 0u;
      std::array<ComPtr<ID3D11Texture2D>, 2> packed_readbacks;
      for (auto& readback : packed_readbacks) {
        if (!check(d11->CreateTexture2D(&packed_desc, nullptr, &readback), "create Lights pack staging texture")) return 1;
      }
      UINT light_counts[] = {1u, 0u, 0u, 0u};
      ID3D11UnorderedAccessView* pack_uavs[] = {carriers[6].uav.Get(), carriers[7].uav.Get(), carriers[8].uav.Get()};
      for (UINT i = 0; i < large_light_views.size(); ++i) {
        ID3D11ShaderResourceView* pack_srvs[] = {large_light_views[i].Get(), record_views[1].Get(), record_views[2].Get()};
        ctx->UpdateSubresource(constants.Get(), 0, nullptr, light_counts, 0, 0);
        ctx->CSSetShader(pack_records.Get(), nullptr, 0);
        ctx->CSSetShaderResources(0, 3, pack_srvs);
        ctx->CSSetUnorderedAccessViews(0, 3, pack_uavs, nullptr);
        ctx->Dispatch(1u, 1u, 1u);
        ctx->CSSetShaderResources(0, 3, null_srvs);
        ctx->CSSetUnorderedAccessViews(0, 3, null_uavs, nullptr);
        ctx->CopyResource(packed_readbacks[i].Get(), carriers[6].texture11.Get());
      }
      ComPtr<ID3D11Query> pack_query;
      D3D11_QUERY_DESC pack_query_desc = {.Query = D3D11_QUERY_EVENT, .MiscFlags = 0};
      if (!check(d11->CreateQuery(&pack_query_desc, &pack_query), "create Lights pack query")) return 1;
      ctx->End(pack_query.Get());
      ctx->Flush();
      auto pack_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while ((hr = ctx->GetData(pack_query.Get(), nullptr, 0, 0)) == S_FALSE
             && std::chrono::steady_clock::now() < pack_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      if (hr != S_OK) {
        std::fprintf(stderr, "FAIL Lights pack completion: 0x%08lx\n", static_cast<unsigned long>(hr));
        return 1;
      }
      std::array<std::array<uint32_t, 28>, 2> packed_words = {};
      for (UINT i = 0; i < packed_readbacks.size(); ++i) {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (!check(ctx->Map(packed_readbacks[i].Get(), 0u, D3D11_MAP_READ, 0u, &mapped), "map Lights pack readback")) return 1;
        std::memcpy(packed_words[i].data(), mapped.pData, sizeof(packed_words[i]));
        ctx->Unmap(packed_readbacks[i].Get(), 0u);
      }
      std::array<uint32_t, 28> expected_words = {};
      std::copy_n(large_lights.data() + kLightWordsPerElement, expected_words.size(), expected_words.data());
      if (packed_words[0] != packed_words[1] || packed_words[0] != expected_words) {
        std::fprintf(stderr, "FAIL production pack readback differs for BUFFER vs zero-flag BUFFEREX\n");
        return 1;
      }
      UINT restore_counts[] = {local && !zero_lights ? 1u : 0u, local ? 1u : 0u, local ? 1u : 0u, 0u};
      ID3D11ShaderResourceView* restore_srvs[] = {record_views[0].Get(), record_views[1].Get(), record_views[2].Get()};
      ID3D11UnorderedAccessView* restore_uavs[] = {carriers[6].uav.Get(), carriers[7].uav.Get(), carriers[8].uav.Get()};
      ctx->UpdateSubresource(constants.Get(), 0, nullptr, restore_counts, 0, 0);
      ctx->CSSetShader(pack_records.Get(), nullptr, 0);
      ctx->CSSetShaderResources(0, 3, restore_srvs);
      ctx->CSSetUnorderedAccessViews(0, 3, restore_uavs, nullptr);
      ctx->Dispatch(1u, 1u, 1u);
      ctx->CSSetShaderResources(0, 3, null_srvs);
      ctx->CSSetUnorderedAccessViews(0, 3, null_uavs, nullptr);
      std::printf("PASS 1024x112 Lights BUFFER/BUFFEREX(flags=0) range and production pack parity\n");
    }
    D3D11_QUERY_DESC const query_desc = {.Query = D3D11_QUERY_EVENT, .MiscFlags = 0};
    ComPtr<ID3D11Query> query;
    if (!check(d11->CreateQuery(&query_desc, &query), "create D3D11 query")) return 1;
    ctx->End(query.Get());
    ctx->Flush();
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while ((hr = ctx->GetData(query.Get(), nullptr, 0, 0)) == S_FALSE && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (hr != S_OK) {
      std::fprintf(stderr, "FAIL D3D11 completion: 0x%08lx\n", static_cast<unsigned long>(hr));
      return 1;
    }
    std::array<D3D12_RESOURCE_BARRIER, 4> barriers = {};
    for (UINT i = 0; i < 4; ++i) {
      barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barriers[i].Transition.pResource = carriers[5 + i].texture12.Get();
      barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barriers[i].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      barriers[i].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    list->ResourceBarrier(4, barriers.data());
    ID3D12DescriptorHeap* heaps[] = {heap.Get()};
    list->SetDescriptorHeaps(1, heaps);
    constexpr UINT sentinel[] = {0xdeadbeefu, 0xdeadbeefu, 0xdeadbeefu, 0xdeadbeefu};
    for (UINT i = 0; i < 4; ++i) {
      auto cpu = clear_heap->GetCPUDescriptorHandleForHeapStart();
      auto gpu = heap->GetGPUDescriptorHandleForHeapStart();
      cpu.ptr += static_cast<SIZE_T>(i) * descriptor_size;
      gpu.ptr += static_cast<UINT64>(4 + i) * descriptor_size;
      list->ClearUnorderedAccessViewUint(gpu, cpu, output[i].Get(), sentinel, 0, nullptr);
      D3D12_RESOURCE_BARRIER barrier = {};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      barrier.UAV.pResource = output[i].Get();
      list->ResourceBarrier(1, &barrier);
    }
    list->SetComputeRootSignature(root.Get());
    list->SetPipelineState(pso.Get());
    list->SetComputeRoot32BitConstants(0, 4, counts, 0);
    list->SetComputeRootDescriptorTable(1, heap->GetGPUDescriptorHandleForHeapStart());
    list->Dispatch(2, 1, 1);
    for (UINT i = 0; i < 4; ++i) {
      barriers[i].Transition.pResource = output[i].Get();
      barriers[i].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barriers[i].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    list->ResourceBarrier(4, barriers.data());
    for (UINT i = 0; i < 4; ++i) list->CopyBufferRegion(readback[i].Get(), 0, output[i].Get(), 0, sizes[i]);
    for (UINT i = 0; i < 5; ++i) {
      if (!local && i >= 2) continue;
      D3D12_RESOURCE_BARRIER barrier = {};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = carriers[i].texture12.Get();
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
      list->ResourceBarrier(1, &barrier);
      D3D12_TEXTURE_COPY_LOCATION source = {};
      source.pResource = carriers[i].texture12.Get();
      source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      D3D12_TEXTURE_COPY_LOCATION dest = {};
      dest.pResource = pixel_readback[i].Get();
      dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      dest.PlacedFootprint = footprints[i];
      list->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
    }
    if (!check(list->Close(), "close command list")) return 1;
    ID3D12CommandList* lists[] = {list.Get()};
    queue->ExecuteCommandLists(1, lists);
    if (!check(queue->Signal(fence.Get(), 1), "signal fence")) return 1;
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (fence->GetCompletedValue() < 1 && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (fence->GetCompletedValue() != 1) {
      std::fprintf(stderr, "FAIL D3D12 fence timeout or device removal\n");
      return 1;
    }
    for (UINT i = 0; i < 4; ++i) {
      void* mapped = nullptr;
      if (!check(readback[i]->Map(0, nullptr, &mapped), "map readback")) return 1;
      std::vector<uint32_t> untouched(sizes[i] / 4, sentinel[0]);
      const void* expected = untouched.data();
      std::vector<uint32_t> expected_scene(1536u / 4u, 0u);
      std::copy_n(scene.data() + (offset / 4u), scene_rows * 4u, expected_scene.data());
      if (i == 0) expected = expected_scene.data();
      if (i > 0 && local && (i != 1 || !zero_lights)) expected = records[i - 1].data() + (strides[i - 1] / 4);
      const bool match = std::memcmp(mapped, expected, sizes[i]) == 0;
      readback[i]->Unmap(0, nullptr);
      if (!match) {
        std::fprintf(stderr, "FAIL readback %u differs (%u bytes)\n", i, sizes[i]);
        return 1;
      }
    }
    for (UINT i = 0; i < 5; ++i) {
      if (!local && i >= 2) continue;
      void* mapped = nullptr;
      if (!check(pixel_readback[i]->Map(0, nullptr, &mapped), "map pixel readback")) return 1;
      bool match = true;
      for (UINT y = 0; y < height && match; ++y) {
        const auto* row = reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(mapped) + footprints[i].Offset + (static_cast<SIZE_T>(y) * footprints[i].Footprint.RowPitch));
        for (UINT x = 0; x < width && match; ++x) {
          const UINT output_components = (i == 4 ? 4 : components[i]);
          for (UINT c = 0; c < output_components; ++c) {
            const uint32_t expected = (c < components[i] ? pixel_words[i][(((y * width) + x) * components[i]) + c] : 0u);
            if (row[(x * output_components) + c] != expected) {
              std::fprintf(stderr, "FAIL pixel %u (%u,%u) component %u: got %08x expected %08x\n", i, x, y, c, row[(x * output_components) + c], expected);
              match = false;
              break;
            }
          }
        }
      }
      pixel_readback[i]->Unmap(0, nullptr);
      if (!match) return 1;
    }
    if (width == 13u && height == 9u && scene_rows == 80u) {
      struct SelectionInput { uint32_t x24; uint32_t count; std::array<uint32_t, 8> slots; };
      struct SelectionOutput { uint32_t valid, index, c20, multiplicity; float pmf; };
      static_assert(sizeof(SelectionInput) == 40u && sizeof(SelectionOutput) == 20u);
      const std::array<SelectionInput, 9> selection_inputs = {{{0u, 3u, {0u, 0u, 1u, 1u, 1u, 2u, 2u, 2u}},
          {(1u << 20) - 1u, 3u, {0u, 0u, 1u, 1u, 1u, 2u, 2u, 2u}},
          {1u << 20, 3u, {0u, 0u, 1u, 1u, 1u, 2u, 2u, 2u}},
          {(1u << 23) - 1u, 3u, {0u, 0u, 1u, 1u, 1u, 2u, 2u, 2u}},
          {1u << 23, 3u, {0u, 0u, 1u, 1u, 1u, 2u, 2u, 2u}},
          {(1u << 24) - 1u, 3u, {0u, 0u, 1u, 1u, 1u, 2u, 2u, 2u}},
          {1u << 24, 3u, {}}, {0u, 0u, {}}, {0u, 2u, {0u, 0u, 0u, 0u, 0u, 0u, 0u, 2u}}}};
      ComPtr<ID3D12PipelineState> selection_pso;
      D3D12_COMPUTE_PIPELINE_STATE_DESC selection_pso_desc = {};
      selection_pso_desc.pRootSignature = root.Get();
      selection_pso_desc.CS = {__world_regir_selection_contract.data(), __world_regir_selection_contract.size()};
      if (!check(d12->CreateComputePipelineState(&selection_pso_desc, IID_PPV_ARGS(&selection_pso)), "create ReGIR selection PSO")) return 1;
      std::array<ComPtr<ID3D12Resource>, 3> selection_buffers;
      const uint64_t selection_sizes[] = {sizeof(selection_inputs), sizeof(SelectionOutput) * selection_inputs.size(), sizeof(SelectionOutput) * selection_inputs.size()};
      for (uint32_t i = 0u; i < 3u; ++i) {
        D3D12_HEAP_PROPERTIES properties = {};
        properties.Type = i == 0u ? D3D12_HEAP_TYPE_UPLOAD : (i == 1u ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_READBACK);
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = selection_sizes[i];
        desc.Height = 1u; desc.DepthOrArraySize = 1u; desc.MipLevels = 1u; desc.SampleDesc.Count = 1u;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = i == 1u ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        const D3D12_RESOURCE_STATES state = i == 0u ? D3D12_RESOURCE_STATE_GENERIC_READ : (i == 1u ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_COPY_DEST);
        if (!check(d12->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&selection_buffers[i])), "create ReGIR selection buffer")) return 1;
      }
      void* mapped = nullptr;
      if (!check(selection_buffers[0]->Map(0u, nullptr, &mapped), "map ReGIR selection input")) return 1;
      std::memcpy(mapped, selection_inputs.data(), sizeof(selection_inputs));
      selection_buffers[0]->Unmap(0u, nullptr);
      ComPtr<ID3D12DescriptorHeap> selection_heap;
      if (!check(d12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&selection_heap)), "create ReGIR selection heap")) return 1;
      D3D12_SHADER_RESOURCE_VIEW_DESC selection_srv = {};
      selection_srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      selection_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      selection_srv.Buffer.NumElements = static_cast<UINT>(selection_inputs.size());
      selection_srv.Buffer.StructureByteStride = sizeof(SelectionInput);
      auto selection_cpu = selection_heap->GetCPUDescriptorHandleForHeapStart();
      d12->CreateShaderResourceView(selection_buffers[0].Get(), &selection_srv, selection_cpu);
      selection_cpu.ptr += static_cast<SIZE_T>(4u) * descriptor_size;
      D3D12_UNORDERED_ACCESS_VIEW_DESC selection_uav = {};
      selection_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
      selection_uav.Buffer.NumElements = static_cast<UINT>(selection_inputs.size());
      selection_uav.Buffer.StructureByteStride = sizeof(SelectionOutput);
      d12->CreateUnorderedAccessView(selection_buffers[1].Get(), nullptr, &selection_uav, selection_cpu);
      if (!check(allocator->Reset(), "reset ReGIR allocator") || !check(list->Reset(allocator.Get(), nullptr), "reset ReGIR list")) return 1;
      ID3D12DescriptorHeap* selection_heaps[] = {selection_heap.Get()};
      list->SetDescriptorHeaps(1u, selection_heaps);
      list->SetComputeRootSignature(root.Get());
      list->SetPipelineState(selection_pso.Get());
      list->SetComputeRootDescriptorTable(1u, selection_heap->GetGPUDescriptorHandleForHeapStart());
      list->Dispatch(1u, 1u, 1u);
      D3D12_RESOURCE_BARRIER selection_barrier = {};
      selection_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      selection_barrier.Transition.pResource = selection_buffers[1].Get();
      selection_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      selection_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      selection_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
      list->ResourceBarrier(1u, &selection_barrier);
      list->CopyBufferRegion(selection_buffers[2].Get(), 0u, selection_buffers[1].Get(), 0u, selection_sizes[1]);
      if (!check(list->Close(), "close ReGIR selection list")) return 1;
      queue->ExecuteCommandLists(1u, lists);
      if (!check(queue->Signal(fence.Get(), 2u), "signal ReGIR selection")) return 1;
      deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (fence->GetCompletedValue() < 2u && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
      if (fence->GetCompletedValue() != 2u) return 1;
      if (!check(selection_buffers[2]->Map(0u, nullptr, &mapped), "map ReGIR selection output")) return 1;
      const auto* results = static_cast<const SelectionOutput*>(mapped);
      for (size_t i = 0u; i < selection_inputs.size(); ++i) {
        const auto expected = falcom_world::pt::PtReGirSelect(selection_inputs[i].x24, selection_inputs[i].count, selection_inputs[i].slots);
        if (results[i].valid != static_cast<uint32_t>(expected.valid) || results[i].index != expected.light_index
            || results[i].c20 != expected.c20 || results[i].multiplicity != expected.multiplicity
            || std::abs(results[i].pmf - expected.pmf) > 1e-7) {
          std::fprintf(stderr, "FAIL ReGIR GPU selection case %zu\n", i);
          selection_buffers[2]->Unmap(0u, nullptr);
          return 1;
        }
      }
      selection_buffers[2]->Unmap(0u, nullptr);
      std::printf("PASS ReGIR shared selector GPU mapping and PMF (tolerance 1e-7)\n");
    }
    if (width == 13u && !zero_lights) {
      struct GpuCell {
        int32_t key[3];
        uint32_t valid;
        uint32_t slots[8];
      };
      static_assert(sizeof(GpuCell) == 48);
      static_assert(offsetof(GpuCell, key) == 0);
      static_assert(offsetof(GpuCell, valid) == 12);
      static_assert(offsetof(GpuCell, slots) == 16);
      uint32_t header[4] = {1u, 0u, 0u, 1u};
      GpuCell cell = {};
      cell.valid = 1u;
      std::fill(std::begin(cell.slots), std::end(cell.slots), 0u);
      RtxdiLightTableEntry lights[2] = {};
      lights[0].radius = 1.f;
      lights[0].power = 1.f;
      lights[0].type = RTXDI_LIGHT_TABLE_TYPE_POINT;
      lights[1] = lights[0];
      lights[1].power = 3.f;
      constexpr UINT64 weight_bytes = RTXDI_LIGHT_TABLE_CAPACITY * sizeof(float);
      const UINT64 sizes[] = {sizeof(header), sizeof(cell), sizeof(lights), weight_bytes, sizeof(cell), weight_bytes, sizeof(cell)};
      std::array<ComPtr<ID3D12Resource>, 7> buffers;
      for (UINT i = 0; i < buffers.size(); ++i) {
        D3D12_HEAP_PROPERTIES properties = {};
        properties.Type = i < 3 ? D3D12_HEAP_TYPE_UPLOAD : (i < 5 ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_READBACK);
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = sizes[i];
        desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1; desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = i == 3 || i == 4 ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        const auto state = i < 3 ? D3D12_RESOURCE_STATE_GENERIC_READ : (i < 5 ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_COPY_DEST);
        if (!check(d12->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&buffers[i])), "create ReGIR production buffer")) return 1;
      }
      std::array<ComPtr<ID3D12DescriptorHeap>, 2> heaps;
      for (auto& fixture_heap : heaps) if (!check(d12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&fixture_heap)), "create ReGIR production heap")) return 1;
      const auto srv = [&](UINT pass, UINT slot, UINT resource, UINT count, UINT stride) {
        auto handle = heaps[pass]->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(slot) * descriptor_size;
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Buffer.NumElements = count;
        desc.Buffer.StructureByteStride = stride;
        d12->CreateShaderResourceView(buffers[resource].Get(), &desc, handle);
      };
      const auto uav = [&](UINT pass, UINT resource, UINT count, UINT stride) {
        auto handle = heaps[pass]->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(4) * descriptor_size;
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        desc.Buffer.NumElements = count;
        desc.Buffer.StructureByteStride = stride;
        d12->CreateUnorderedAccessView(buffers[resource].Get(), nullptr, &desc, handle);
      };
      srv(0, 0, 2, 2, sizeof(RtxdiLightTableEntry));
      srv(0, 1, 0, 1, sizeof(header));
      srv(0, 2, 1, 1, sizeof(cell));
      uav(0, 3, RTXDI_LIGHT_TABLE_CAPACITY, sizeof(float));
      srv(1, 0, 0, 1, sizeof(header));
      srv(1, 1, 3, RTXDI_LIGHT_TABLE_CAPACITY, sizeof(float));
      uav(1, 4, 1, sizeof(cell));
      std::array<ComPtr<ID3D12PipelineState>, 2> pipelines;
      D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc = {};
      pipeline_desc.pRootSignature = root.Get();
      pipeline_desc.CS = {__world_regir_weights.data(), __world_regir_weights.size()};
      if (!check(d12->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&pipelines[0])), "create ReGIR weights PSO")) return 1;
      pipeline_desc.CS = {__world_regir_build.data(), __world_regir_build.size()};
      if (!check(d12->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&pipelines[1])), "create ReGIR build PSO")) return 1;
      const char* case_names[] = {"two-light", "reordered two-light", "zero-light", "invalid-header", "all-zero/invalid-light", "over-capacity-count"};
      float original_weights[2] = {};
      for (UINT fixture_case = 0; fixture_case < 6; ++fixture_case) {
        if (fixture_case == 1) std::swap(lights[0], lights[1]);
        if (fixture_case == 3) header[3] = 0u;
        if (fixture_case == 4) {
          header[3] = 1u;
          lights[0].power = 0.f;
          lights[1].radius = 0.f;
        }
        if (fixture_case == 5) {
          header[3] = 1u;
          for (auto& light : lights) {
            light = {};
            light.radius = 1.f;
            light.power = 1.f;
            light.type = RTXDI_LIGHT_TABLE_TYPE_POINT;
          }
        }
        const void* initial[] = {header, &cell, lights};
        for (UINT i = 0; i < 3; ++i) {
          void* mapped = nullptr;
          if (!check(buffers[i]->Map(0, nullptr, &mapped), "map ReGIR fixture")) return 1;
          std::memcpy(mapped, initial[i], static_cast<size_t>(sizes[i]));
          buffers[i]->Unmap(0, nullptr);
        }
      if (!check(allocator->Reset(), "reset ReGIR production allocator") || !check(list->Reset(allocator.Get(), nullptr), "reset ReGIR production list")) return 1;
      if (fixture_case != 0) {
        D3D12_RESOURCE_BARRIER reset_barriers[2] = {};
        for (UINT index = 0; index < 2; ++index) {
          reset_barriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          reset_barriers[index].Transition.pResource = buffers[3 + index].Get();
          reset_barriers[index].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
          reset_barriers[index].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
          reset_barriers[index].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }
        list->ResourceBarrier(2, reset_barriers);
      }
      D3D12_RESOURCE_BARRIER initial_barrier = {};
      initial_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      initial_barrier.Transition.pResource = buffers[4].Get();
      initial_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      initial_barrier.Transition.StateBefore = fixture_case == 0 ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      initial_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
      list->ResourceBarrier(1, &initial_barrier);
      list->CopyBufferRegion(buffers[4].Get(), 0, buffers[1].Get(), 0, sizes[1]);
      initial_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      initial_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      list->ResourceBarrier(1, &initial_barrier);
      list->SetComputeRootSignature(root.Get());
      const uint32_t params[2] = {fixture_case == 2 ? 0u : (fixture_case == 5 ? RTXDI_LIGHT_TABLE_CAPACITY + 1u : 2u), 1u};
      list->SetComputeRoot32BitConstants(0, 2, params, 0);
      ID3D12DescriptorHeap* active_heap[] = {heaps[0].Get()};
      list->SetDescriptorHeaps(1, active_heap);
      list->SetComputeRootDescriptorTable(1, heaps[0]->GetGPUDescriptorHandleForHeapStart());
      list->SetPipelineState(pipelines[0].Get());
      list->Dispatch(16, 1, 1);
      D3D12_RESOURCE_BARRIER barrier = {};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = buffers[3].Get();
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      list->ResourceBarrier(1, &barrier);
      active_heap[0] = heaps[1].Get();
      list->SetDescriptorHeaps(1, active_heap);
      list->SetComputeRootDescriptorTable(1, heaps[1]->GetGPUDescriptorHandleForHeapStart());
      list->SetPipelineState(pipelines[1].Get());
      list->Dispatch(1, 1, 1);
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
      list->ResourceBarrier(1, &barrier);
      barrier.Transition.pResource = buffers[4].Get();
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      list->ResourceBarrier(1, &barrier);
      list->CopyBufferRegion(buffers[5].Get(), 0, buffers[3].Get(), 0, sizes[3]);
      list->CopyBufferRegion(buffers[6].Get(), 0, buffers[4].Get(), 0, sizes[4]);
      if (!check(list->Close(), "close ReGIR production list")) return 1;
      queue->ExecuteCommandLists(1, lists);
      const UINT64 production_fence = 3u + fixture_case;
      if (!check(queue->Signal(fence.Get(), production_fence), "signal ReGIR production")) return 1;
      deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (fence->GetCompletedValue() < production_fence && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
      if (fence->GetCompletedValue() < production_fence || fence->GetCompletedValue() == UINT64_MAX) return 1;
      void* mapped = nullptr;
      if (!check(buffers[5]->Map(0, nullptr, &mapped), "map ReGIR weights")) return 1;
      const auto* weights = static_cast<const float*>(mapped);
      bool weights_ok = std::isfinite(weights[0]) && std::isfinite(weights[1]);
      if (fixture_case == 0) {
        weights_ok = weights_ok && weights[0] > 0.f && weights[1] > weights[0];
        original_weights[0] = weights[0];
        original_weights[1] = weights[1];
      } else if (fixture_case == 1) {
        weights_ok = weights_ok && weights[0] == original_weights[1] && weights[1] == original_weights[0];
      } else if (fixture_case == 5) {
        weights_ok = weights_ok && weights[0] > 0.f && weights[1] > 0.f;
      } else {
        weights_ok = weights_ok && weights[0] == 0.f && weights[1] == 0.f;
      }
      const float current_weights[2] = {weights[0], weights[1]};
      buffers[5]->Unmap(0, nullptr);
      if (!check(buffers[6]->Map(0, nullptr, &mapped), "map ReGIR cells")) return 1;
      const auto* result = static_cast<const GpuCell*>(mapped);
      bool cells_ok = result->valid == (fixture_case < 2 ? 1u : 0u);
      for (uint32_t slot : result->slots) {
        cells_ok = cells_ok && (fixture_case < 2 ? (slot < 2 && current_weights[slot] > 0.f) : slot == RTXDI_LIGHT_TABLE_INVALID_INDEX);
      }
      buffers[6]->Unmap(0, nullptr);
      if (!weights_ok || !cells_ok) {
        std::fprintf(stderr, "FAIL ReGIR production %s GPU readback\n", case_names[fixture_case]);
        return 1;
      }
      std::printf("PASS ReGIR production %s GPU readback\n", case_names[fixture_case]);
      }
    }
    if ((width == 13u && !zero_lights) || (width == 1920u && !local)) {
      const float boundary_positions[] = {-1048576.f, 1048575.875f, 1048576.f, -4.001f, -4.f, 3.999f, 4.f};
      const UINT case_count = width == 13u ? 8u : 1u;
      for (UINT fixture_case = 0; fixture_case < case_count; ++fixture_case) {
      const bool overflow_case = width != 13u;
      const bool boundary_case = !overflow_case && fixture_case != 0u;
      std::vector<float> depth(width * height, 0.5f);
      if (!overflow_case && !boundary_case) depth[0] = 0.f;
      std::vector<std::array<uint32_t, 4>> normal(width * height, {32768u, 32768u, 0u, 0u});
      std::array<std::array<uint32_t, 4>, 96> fixture_scene = {};
      const auto set_float = [&](UINT row, UINT column, float value) { std::memcpy(&fixture_scene[row][column], &value, sizeof(value)); };
      for (UINT axis = 0; axis < 4; ++axis) {
        set_float(axis, axis, axis == 2 ? -1.f : 1.f);
        set_float(20 + axis, axis, 1.f);
      }
      if (boundary_case) {
        set_float(20, 0, 0.f);
        set_float(20, 3, boundary_positions[fixture_case - 1u]);
        set_float(21, 1, 0.f);
      } else if (overflow_case) {
        set_float(20, 0, 8192.f);
        set_float(21, 1, 8192.f);
      }
      set_float(6, 3, 3.f);
      set_float(49, 0, 1.f);
      set_float(49, 1, 1.f);
      ctx->UpdateSubresource(carriers[0].texture11.Get(), 0, nullptr, depth.data(), width * sizeof(float), 0);
      ctx->UpdateSubresource(carriers[1].texture11.Get(), 0, nullptr, normal.data(), width * 16u, 0);
      ctx->UpdateSubresource(carriers[5].texture11.Get(), 0, nullptr, fixture_scene.data(), 96u * 16u, 0);
      D3D11_QUERY_DESC query_desc = {D3D11_QUERY_EVENT, 0};
      ComPtr<ID3D11Query> upload_complete;
      if (!check(d11->CreateQuery(&query_desc, &upload_complete), "create registration upload query")) return 1;
      ctx->End(upload_complete.Get());
      ctx->Flush();
      deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      HRESULT upload_status = S_FALSE;
      while ((upload_status = ctx->GetData(upload_complete.Get(), nullptr, 0, 0)) == S_FALSE
             && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
      if (upload_status != S_OK) {
        std::fprintf(stderr, "FAIL registration upload synchronization: 0x%08lx\n", static_cast<unsigned long>(upload_status));
        return 1;
      }
      constexpr UINT cell_stride = 48u;
      const UINT64 bytes[] = {16u, 256u * cell_stride, 16u, 256u * cell_stride};
      std::array<ComPtr<ID3D12Resource>, 4> fixture_buffers;
      for (UINT index = 0; index < fixture_buffers.size(); ++index) {
        D3D12_HEAP_PROPERTIES properties = {};
        properties.Type = index < 2 ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = bytes[index]; desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1; desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = index < 2 ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        if (!check(d12->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc,
                    index < 2 ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr, IID_PPV_ARGS(&fixture_buffers[index])), "create ReGIR registration buffer")) return 1;
      }
      ComPtr<ID3D12DescriptorHeap> fixture_heap;
      if (!check(d12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&fixture_heap)), "create ReGIR registration heap")) return 1;
      const UINT texture_indices[] = {0u, 1u, 5u};
      for (UINT index = 0; index < 3; ++index) {
        auto handle = fixture_heap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(index) * descriptor_size;
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = index == 0 ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R32G32B32A32_UINT;
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MipLevels = 1;
        d12->CreateShaderResourceView(carriers[texture_indices[index]].texture12.Get(), &desc, handle);
      }
      for (UINT index = 0; index < 2; ++index) {
        auto handle = fixture_heap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(4u + index) * descriptor_size;
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        desc.Buffer.NumElements = index == 0 ? 1u : 256u;
        desc.Buffer.StructureByteStride = index == 0 ? 16u : cell_stride;
        d12->CreateUnorderedAccessView(fixture_buffers[index].Get(), nullptr, &desc, handle);
      }
      D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
      desc.pRootSignature = root.Get();
      desc.CS = {__world_regir_register.data(), __world_regir_register.size()};
      ComPtr<ID3D12PipelineState> fixture_pipeline;
      if (!check(d12->CreateComputePipelineState(&desc, IID_PPV_ARGS(&fixture_pipeline)), "create ReGIR registration PSO")
          || !check(allocator->Reset(), "reset registration allocator") || !check(list->Reset(allocator.Get(), nullptr), "reset registration list")) return 1;
      ID3D12DescriptorHeap* fixture_heaps[] = {fixture_heap.Get()};
      list->SetDescriptorHeaps(1, fixture_heaps);
      list->SetComputeRootSignature(root.Get());
      const UINT size[] = {width, height};
      list->SetComputeRoot32BitConstants(0, 2, size, 0);
      list->SetComputeRootDescriptorTable(1, fixture_heap->GetGPUDescriptorHandleForHeapStart());
      list->SetPipelineState(fixture_pipeline.Get());
      list->Dispatch(1, 1, 1);
      for (UINT index = 0; index < 2; ++index) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = fixture_buffers[index].Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        list->ResourceBarrier(1, &barrier);
        list->CopyBufferRegion(fixture_buffers[index + 2].Get(), 0, fixture_buffers[index].Get(), 0, bytes[index]);
      }
      if (!check(list->Close(), "close registration list")) return 1;
      queue->ExecuteCommandLists(1, lists);
      const UINT64 registration_fence = 9u + fixture_case;
      if (!check(queue->Signal(fence.Get(), registration_fence), "signal registration")) return 1;
      deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (fence->GetCompletedValue() < registration_fence && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
      if (fence->GetCompletedValue() < registration_fence) return 1;
      void* mapped = nullptr;
      if (!check(fixture_buffers[2]->Map(0, nullptr, &mapped), "map registration header")) return 1;
      const auto* header = static_cast<const uint32_t*>(mapped);
      const bool rejected_boundary = boundary_case && fixture_case == 3u;
      const UINT expected_count = overflow_case ? 256u : (boundary_case ? (rejected_boundary ? 0u : 1u) : 4u);
      const UINT expected_rejected = rejected_boundary ? width * height : (boundary_case || overflow_case ? 0u : 1u);
      const UINT expected_overflow = overflow_case ? 4096u - 256u : 0u;
      const bool valid_header = header[0] == expected_count && header[1] == expected_rejected
                                && header[2] == expected_overflow && header[3] == 1u;
      if (!valid_header) std::fprintf(stderr, "FAIL registration case %u header %u %u %u %u expected %u %u %u 1\n",
                                      fixture_case, header[0], header[1], header[2], header[3],
                                      expected_count, expected_rejected, expected_overflow);
      fixture_buffers[2]->Unmap(0, nullptr);
      if (!check(fixture_buffers[3]->Map(0, nullptr, &mapped), "map registration cells")) return 1;
      const auto* words = static_cast<const int32_t*>(mapped);
      bool keys[2][2] = {};
      bool valid_keys = true;
      for (UINT index = 0; index < expected_count; ++index) {
        const int32_t x = words[index * 12], y = words[index * 12 + 1], z = words[index * 12 + 2];
        if (overflow_case) {
          const uint64_t pixel = uint64_t(width) * height * index / 4096u;
          const float uv_x = (static_cast<float>(pixel % width) + 0.5f) / static_cast<float>(width);
          const float uv_y = (static_cast<float>(pixel / width) + 0.5f) / static_cast<float>(height);
          const int32_t expected_x = static_cast<int32_t>(std::floor((uv_x * 2.f - 1.f) * 2048.f));
          const int32_t expected_y = static_cast<int32_t>(std::floor((1.f - uv_y * 2.f) * 2048.f));
          if (x != expected_x || y != expected_y || z != 0) {
            std::fprintf(stderr, "FAIL registration overflow key %u: %d %d %d expected %d %d 0\n", index, x, y, z, expected_x, expected_y);
            valid_keys = false;
          }
        } else if (boundary_case) {
          const int32_t expected_x = static_cast<int32_t>(std::floor(boundary_positions[fixture_case - 1u] / 4.f));
          if (x != expected_x || y != 0 || z != 0) valid_keys = false;
        } else if (x < -1 || x > 0 || y < -1 || y > 0 || z != 0 || keys[x + 1][y + 1]) valid_keys = false;
        else keys[x + 1][y + 1] = true;
      }
      fixture_buffers[3]->Unmap(0, nullptr);
      if (!valid_header || !valid_keys) {
        std::fprintf(stderr, "FAIL ReGIR production registration readback header=%u keys=%u\n", valid_header, valid_keys);
        return 1;
      }
      std::printf("PASS ReGIR production registration %s case %u\n", overflow_case ? "256-cell overflow" : (boundary_case ? "boundary and same-cell dedup" : "reconstruction and rejection"), fixture_case);
      }
    }
    const UINT64 message_count = info_queue->GetNumStoredMessagesAllowedByRetrievalFilter();
    for (UINT64 index = 0; index < message_count; ++index) {
      SIZE_T size = 0;
      if (!check(info_queue->GetMessage(index, nullptr, &size), "size D3D12 message")) return 1;
      std::vector<uint8_t> storage(size);
      auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
      if (!check(info_queue->GetMessage(index, message, &size), "read D3D12 message")) return 1;
      if (message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING) {
        std::fprintf(stderr, "FAIL D3D12 InfoQueue: %s\n", message->pDescription);
        return 1;
      }
    }
    std::printf("PASS D3D12 producer InfoQueue clean (%llu messages)\n", static_cast<unsigned long long>(message_count));
    std::printf("PASS %ux%u local=%u zero_lights=%u scene_rows=%u motion=absent tail=zero\n", width, height,
                static_cast<int>(local), static_cast<int>(zero_lights), scene_rows);
  }
  return RunRtxdiDxrBridge();
}
