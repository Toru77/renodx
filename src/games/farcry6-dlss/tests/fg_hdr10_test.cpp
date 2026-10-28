/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

// Runs the HUD-less HDR compute shader on a small synthetic image and checks
// the resulting PQ values. No game or Streamline resources are used.
#include <Windows.h>
#include <d3d12.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <embed/fg_hdr10_convert.h>

using Microsoft::WRL::ComPtr;

int main() {
  ComPtr<ID3D12Device> device;
  if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) return 1;
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  ComPtr<ID3D12CommandQueue> queue;
  ComPtr<ID3D12CommandAllocator> allocator;
  ComPtr<ID3D12GraphicsCommandList> list;
  if (FAILED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)))
      || FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)))
      || FAILED(device->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)))) return 2;

  D3D12_DESCRIPTOR_RANGE ranges[2] = {};
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  ranges[0].NumDescriptors = ranges[1].NumDescriptors = 1u;
  D3D12_ROOT_PARAMETER params[3] = {};
  for (uint32_t i = 0u; i < 2u; ++i) {
    params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[i].DescriptorTable = {1u, &ranges[i]};
  }
  params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  params[2].Constants = {0u, 0u, 3u};
  D3D12_ROOT_SIGNATURE_DESC signature_desc = {};
  signature_desc.NumParameters = 3u;
  signature_desc.pParameters = params;
  ComPtr<ID3DBlob> signature_blob, errors;
  if (FAILED(D3D12SerializeRootSignature(&signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &errors))) return 3;
  ComPtr<ID3D12RootSignature> signature;
  if (FAILED(device->CreateRootSignature(0u, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(&signature)))) return 4;
  D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc = {};
  pipeline_desc.pRootSignature = signature.Get();
  pipeline_desc.CS = {__fg_hdr10_convert.data(), __fg_hdr10_convert.size()};
  ComPtr<ID3D12PipelineState> pipeline;
  if (FAILED(device->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline)))) return 5;

  D3D12_HEAP_PROPERTIES gpu_heap = {};
  gpu_heap.Type = D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC texture = {};
  texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  texture.Width = 16u;
  texture.Height = 16u;
  texture.DepthOrArraySize = 1u;
  texture.MipLevels = 1u;
  texture.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
  texture.SampleDesc.Count = 1u;
  texture.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  ComPtr<ID3D12Resource> source, output;
  if (FAILED(device->CreateCommittedResource(&gpu_heap, D3D12_HEAP_FLAG_NONE, &texture, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                              nullptr, IID_PPV_ARGS(&source)))) return 6;
  texture.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
  texture.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if (FAILED(device->CreateCommittedResource(&gpu_heap, D3D12_HEAP_FLAG_NONE, &texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                              nullptr, IID_PPV_ARGS(&output)))) return 7;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
  UINT64 readback_size = 0u;
  device->GetCopyableFootprints(&texture, 0u, 1u, 0u, &footprint, nullptr, nullptr, &readback_size);
  D3D12_HEAP_PROPERTIES readback_heap = {};
  readback_heap.Type = D3D12_HEAP_TYPE_READBACK;
  D3D12_RESOURCE_DESC buffer = {};
  buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  buffer.Width = readback_size;
  buffer.Height = 1u;
  buffer.DepthOrArraySize = 1u;
  buffer.MipLevels = 1u;
  buffer.SampleDesc.Count = 1u;
  buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  ComPtr<ID3D12Resource> readback;
  if (FAILED(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST,
                                              nullptr, IID_PPV_ARGS(&readback)))) return 8;

  D3D12_DESCRIPTOR_HEAP_DESC rtv_desc = {};
  rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_desc.NumDescriptors = 1u;
  D3D12_DESCRIPTOR_HEAP_DESC views_desc = {};
  views_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  views_desc.NumDescriptors = 2u;
  views_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ComPtr<ID3D12DescriptorHeap> rtv_heap, views_heap;
  if (FAILED(device->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&rtv_heap)))
      || FAILED(device->CreateDescriptorHeap(&views_desc, IID_PPV_ARGS(&views_heap)))) return 9;
  const auto rtv = rtv_heap->GetCPUDescriptorHandleForHeapStart();
  device->CreateRenderTargetView(source.Get(), nullptr, rtv);
  auto cpu = views_heap->GetCPUDescriptorHandleForHeapStart();
  D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
  srv.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
  srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv.Texture2D.MipLevels = 1u;
  device->CreateShaderResourceView(source.Get(), &srv, cpu);
  const auto stride = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  cpu.ptr += stride;
  D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
  uav.Format = texture.Format;
  uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  device->CreateUnorderedAccessView(output.Get(), nullptr, &uav, cpu);

  const float color[4] = {0.1f, 0.5f, 1.f, 1.f};
  list->ClearRenderTargetView(rtv, color, 0u, nullptr);
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.pResource = source.Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  list->ResourceBarrier(1u, &barrier);
  ID3D12DescriptorHeap* heaps[] = {views_heap.Get()};
  list->SetDescriptorHeaps(1u, heaps);
  list->SetComputeRootSignature(signature.Get());
  list->SetPipelineState(pipeline.Get());
  auto gpu = views_heap->GetGPUDescriptorHandleForHeapStart();
  list->SetComputeRootDescriptorTable(0u, gpu);
  gpu.ptr += stride;
  list->SetComputeRootDescriptorTable(1u, gpu);
  const struct { uint32_t width, height; float brightness; } constants = {16u, 16u, 170.f};
  list->SetComputeRoot32BitConstants(2u, 3u, &constants, 0u);
  list->Dispatch(2u, 2u, 1u);
  barrier.Transition.pResource = output.Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
  list->ResourceBarrier(1u, &barrier);
  D3D12_TEXTURE_COPY_LOCATION dst = {};
  dst.pResource = readback.Get();
  dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  dst.PlacedFootprint = footprint;
  D3D12_TEXTURE_COPY_LOCATION src = {};
  src.pResource = output.Get();
  src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  list->CopyTextureRegion(&dst, 0u, 0u, 0u, &src, nullptr);
  if (FAILED(list->Close())) return 10;
  ID3D12CommandList* lists[] = {list.Get()};
  queue->ExecuteCommandLists(1u, lists);
  ComPtr<ID3D12Fence> fence;
  if (FAILED(device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))) || FAILED(queue->Signal(fence.Get(), 1u))) return 11;
  HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (event == nullptr || FAILED(fence->SetEventOnCompletion(1u, event)) || WaitForSingleObject(event, 5000u) != WAIT_OBJECT_0) return 12;
  CloseHandle(event);
  void* mapped = nullptr;
  const D3D12_RANGE range{0u, static_cast<SIZE_T>(readback_size)};
  if (FAILED(readback->Map(0u, &range, &mapped)) || mapped == nullptr) return 13;
  uint32_t pixel = 0u;
  std::memcpy(&pixel, mapped, sizeof(pixel));
  const D3D12_RANGE written{0u, 0u};
  readback->Unmap(0u, &written);
  float linear[3] = {};
  for (uint32_t i = 0u; i < 3u; ++i) linear[i] = std::pow((color[i] + 0.055f) * 0.9478673338890076f, 2.4f) * 170.f * 0.0002f;
  const float intermediate[3] = {
      0.64304f * linear[0] + 0.31119f * linear[1] + 0.04578f * linear[2],
      0.05927f * linear[0] + 0.93144f * linear[1] + 0.0093f * linear[2],
      0.00596f * linear[0] + 0.06392f * linear[1] + 0.93012f * linear[2]};
  const float display[3] = {
      0.966957986f * intermediate[0] + 0.0251869f * intermediate[1] - 0.00181166f * intermediate[2],
      0.0168175f * intermediate[0] + 0.981899f * intermediate[1] + 0.00182386f * intermediate[2],
      0.0140284f * intermediate[0] + 0.0234305f * intermediate[1] + 0.942865f * intermediate[2]};
  for (uint32_t i = 0u; i < 3u; ++i) {
    const float value = std::pow((std::max)(0.f, (std::min)(1.f, display[i])), 0.1593017578125f);
    const float expected = std::pow((value * 18.8515625f + 0.8359375f) / (value * 18.6875f + 1.f), 78.84375f);
    const float actual = static_cast<float>((pixel >> (i * 10u)) & 1023u) / 1023.f;
    if (std::abs(actual - expected) > 0.002f) return 14;
  }
  if ((pixel >> 30u) != 3u) return 15;
  std::puts("HUD-less HDR 10-bit conversion GPU check passed");
  return 0;
}
