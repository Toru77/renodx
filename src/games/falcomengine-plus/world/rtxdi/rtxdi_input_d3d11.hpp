#pragma once

#include <algorithm>
#include <cstdint>

#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>

#include "rtxdi_input_validation.hpp"

namespace falcom_world::rtxdi {

struct SceneB0BindingEvidence {
  uint64_t native_buffer = 0u;
  uint32_t first_constant = 0u;
  uint32_t constant_count = 0u;
  uint32_t buffer_bytes = 0u;
  uint32_t bind_flags = 0u;
  HRESULT context_query_result = E_POINTER;
  bool context_query_available = false;
  bool buffer_available = false;
};

inline bool ValidateSceneB0Binding(ID3D11DeviceContext* context, ID3D11Buffer* expected_buffer,
                                   uint64_t expected_offset, uint32_t minimum_constants,
                                   SceneB0BindingEvidence* evidence = nullptr) {
  if (evidence != nullptr) *evidence = {};
  if (context == nullptr) return false;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context1;
  const HRESULT query_result = context->QueryInterface(IID_PPV_ARGS(&context1));
  if (evidence != nullptr) {
    evidence->context_query_result = query_result;
    evidence->context_query_available = SUCCEEDED(query_result);
  }
  if (FAILED(query_result)) return false;
  Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
  UINT first = 0u, count = 0u;
  context1->PSGetConstantBuffers1(0u, 1u, &buffer, &first, &count);
  if (evidence != nullptr) {
    evidence->native_buffer = reinterpret_cast<uint64_t>(buffer.Get());  // NOLINT(performance-no-int-to-ptr)
    evidence->first_constant = first;
    evidence->constant_count = count;
    evidence->buffer_available = buffer != nullptr;
    if (buffer != nullptr) {
      D3D11_BUFFER_DESC desc = {};
      buffer->GetDesc(&desc);
      evidence->buffer_bytes = desc.ByteWidth;
      evidence->bind_flags = desc.BindFlags;
    }
  }
  if (expected_buffer == nullptr || buffer.Get() != expected_buffer || uint64_t{first} * 16u != expected_offset
      || count < minimum_constants) return false;
  if (evidence != nullptr) return (evidence->bind_flags & D3D11_BIND_CONSTANT_BUFFER) != 0u;
  D3D11_BUFFER_DESC desc = {};
  buffer->GetDesc(&desc);
  return (desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) != 0u;
}

inline bool GetTextureViewExtent(ID3D11ShaderResourceView* view, uint32_t* width, uint32_t* height) {
  if (!view || !width || !height) return false;
  D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
  view->GetDesc(&srv);
  if (srv.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D || srv.Texture2D.MipLevels == 0u) return false;
  Microsoft::WRL::ComPtr<ID3D11Resource> resource;
  view->GetResource(&resource);
  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
  if (!resource || FAILED(resource.As(&texture))) return false;
  D3D11_TEXTURE2D_DESC desc = {};
  texture->GetDesc(&desc);
  const uint32_t mip = srv.Texture2D.MostDetailedMip;
  if (desc.SampleDesc.Count != 1u || desc.ArraySize != 1u || mip >= desc.MipLevels || mip >= 32u) return false;
  *width = (std::max)(desc.Width >> mip, 1u);
  *height = (std::max)(desc.Height >> mip, 1u);
  return true;
}

// The returned metadata is checked before a borrowed game view is bound to a pack pass.
inline bool ValidateTextureView(ID3D11ShaderResourceView* view, uint32_t width, uint32_t height, DXGI_FORMAT expected_format) {
  if (view == nullptr) return false;
  D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
  view->GetDesc(&srv);
  if (srv.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D || srv.Format != expected_format || srv.Texture2D.MipLevels == 0u) return false;
  Microsoft::WRL::ComPtr<ID3D11Resource> resource;
  view->GetResource(&resource);
  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
  if (!resource || FAILED(resource.As(&texture))) return false;
  D3D11_TEXTURE2D_DESC desc = {};
  texture->GetDesc(&desc);
  return ValidTextureInput({.width = desc.Width, .height = desc.Height, .mip_levels = desc.MipLevels, .mip = srv.Texture2D.MostDetailedMip, .samples = desc.SampleDesc.Count, .array_size = desc.ArraySize, .texture_2d_view = true}, width, height);
}

inline bool ValidateStructuredView(ID3D11ShaderResourceView* view, uint32_t stride, StructuredInputRange* out) {
  if (view == nullptr || out == nullptr) return false;
  D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
  view->GetDesc(&srv);
  if (srv.ViewDimension != D3D11_SRV_DIMENSION_BUFFER || srv.Format != DXGI_FORMAT_UNKNOWN) return false;
  Microsoft::WRL::ComPtr<ID3D11Resource> resource;
  view->GetResource(&resource);
  Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
  if (!resource || FAILED(resource.As(&buffer))) return false;
  D3D11_BUFFER_DESC desc = {};
  buffer->GetDesc(&desc);
  if ((desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) == 0u) return false;
  const StructuredInputRange range = {.buffer_bytes = desc.ByteWidth, .first_element = srv.Buffer.FirstElement, .element_count = srv.Buffer.NumElements, .stride = desc.StructureByteStride};
  if (!ValidStructuredInput(range, stride)) return false;
  *out = range;
  return true;
}

inline bool ValidateSceneBuffer(ID3D11Buffer* buffer, uint64_t offset, uint64_t range_bytes) {
  if (buffer == nullptr) return false;
  D3D11_BUFFER_DESC desc = {};
  buffer->GetDesc(&desc);
  return (desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) != 0u && ValidSceneInputRange(desc.ByteWidth, offset, range_bytes);
}

}  // namespace falcom_world::rtxdi
