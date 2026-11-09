#include <basetsd.h>
#include <combaseapi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgiformat.h>
#include <handleapi.h>
#include <minwindef.h>
#include <winerror.h>
#include <winnt.h>
#include <wrl/client.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

#include "../rtxdi_input_d3d11.hpp"

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
  for (const auto& test : std::array<std::array<UINT, 4>, 4>{{{{13, 9, 1, 0}}, {{13, 9, 1, 1}}, {{1920, 1080, 0, 1}}, {{1920, 1080, 1, 0}}}}) {
    const UINT width = test[0];
    const UINT height = test[1];
    const bool local = test[2] != 0;
    const bool zero_lights = test[3] != 0;
    using namespace falcom_world::rtxdi;
    constexpr UINT offset = 256;
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
    std::vector<uint32_t> scene((offset + 1536) / 4);
    for (UINT i = 0; i < scene.size(); ++i) scene[i] = 0x12340000u + i;
    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.ByteWidth = static_cast<UINT>(scene.size() * 4);
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA const initial_scene = {.pSysMem = scene.data(), .SysMemPitch = 0, .SysMemSlicePitch = 0};
    ComPtr<ID3D11Buffer> scene_cb;
    if (!check(d11->CreateBuffer(&cb_desc, &initial_scene, &scene_cb), "create scene CB") || !ValidateSceneBuffer(scene_cb.Get(), offset, 1536)) return 1;
    const SceneRangeSnapshot promoted = {.buffer = 1, .offset = offset, .size = 1536, .frame = 4, .command = 1, .live = true, .known = true, .sequence = 1};
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
      if (!ValidateStructuredView(record_views[i].Get(), strides[i], &range) || range.first_element != 1) return 1;
    }
    D3D11_BUFFER_DESC constants_desc = {};
    constants_desc.ByteWidth = 16;
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
      if (!check(d12->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&output[i])), "create output buffer")) return 1;
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
    UINT params[] = {width, height, local ? 1u : 0u, 0};
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
      auto cpu = heap->GetCPUDescriptorHandleForHeapStart();
      auto gpu = heap->GetGPUDescriptorHandleForHeapStart();
      cpu.ptr += static_cast<SIZE_T>(4 + i) * descriptor_size;
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
      if (i == 0) expected = scene.data() + (offset / 4);
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
    std::printf("PASS %ux%u local=%u zero_lights=%u motion=absent\n", width, height, static_cast<int>(local), static_cast<int>(zero_lights));
  }
  return RunRtxdiDxrBridge();
}
