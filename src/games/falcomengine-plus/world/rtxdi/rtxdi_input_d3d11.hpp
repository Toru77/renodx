#pragma once

#include <algorithm>
#include <cstdint>

#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>

#include "rtxdi_diagnostics.hpp"
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

struct StructuredViewEvidence {
  uint64_t resource_identity = 0u;
  uint32_t expected_stride = 0u;
  uint32_t view_dimension = UINT32_MAX;
  uint32_t format = UINT32_MAX;
  uint32_t first_element = 0u;
  uint32_t element_count = 0u;
  uint32_t bufferex_flags = 0u;
  uint32_t bind_flags = 0u;
  uint32_t misc_flags = 0u;
  uint32_t structure_stride = 0u;
  uint64_t buffer_bytes = 0u;
  uint64_t element_offset = 0u;
  uint64_t element_bytes = 0u;
  uint32_t evidence_flags = 0u;
  DiagnosticStructuredFailure failure = DiagnosticStructuredFailure::None;
};

inline bool ValidateStructuredView(ID3D11ShaderResourceView* view, uint32_t stride, StructuredInputRange* out,
                                   StructuredViewEvidence* evidence = nullptr) {
  if (evidence != nullptr) {
    *evidence = {};
    evidence->expected_stride = stride;
    evidence->evidence_flags = 128u;
  }
  if (view == nullptr || out == nullptr) {
    if (evidence != nullptr) evidence->failure = DiagnosticStructuredFailure::NullArgument;
    return false;
  }
  D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
  view->GetDesc(&srv);
  uint32_t first_element = 0u;
  uint32_t element_count = 0u;
  bool supported_dimension = false;
  bool supported_bufferex_flags = true;
  if (srv.ViewDimension == D3D11_SRV_DIMENSION_BUFFER) {
    first_element = srv.Buffer.FirstElement;
    element_count = srv.Buffer.NumElements;
    supported_dimension = true;
  } else if (srv.ViewDimension == D3D11_SRV_DIMENSION_BUFFEREX) {
    first_element = srv.BufferEx.FirstElement;
    element_count = srv.BufferEx.NumElements;
    supported_dimension = true;
    supported_bufferex_flags = srv.BufferEx.Flags == 0u;
  }
  if (evidence != nullptr) {
    evidence->view_dimension = static_cast<uint32_t>(srv.ViewDimension);
    evidence->format = static_cast<uint32_t>(srv.Format);
    evidence->evidence_flags |= 1u;
    if (supported_dimension) {
      evidence->first_element = first_element;
      evidence->element_count = element_count;
    }
    if (srv.ViewDimension == D3D11_SRV_DIMENSION_BUFFEREX) {
      evidence->bufferex_flags = srv.BufferEx.Flags;
      evidence->evidence_flags |= 16u;
    }
  }
  const auto capture_buffer_desc = [&](const D3D11_BUFFER_DESC& desc) {
    evidence->evidence_flags |= 8u | 32u;
    evidence->buffer_bytes = desc.ByteWidth;
    evidence->bind_flags = desc.BindFlags;
    evidence->misc_flags = desc.MiscFlags;
    evidence->structure_stride = desc.StructureByteStride;
    evidence->element_offset = uint64_t{evidence->first_element} * evidence->expected_stride;
    evidence->element_bytes = uint64_t{evidence->element_count} * evidence->expected_stride;
    if (evidence->element_offset <= desc.ByteWidth
        && evidence->element_bytes <= desc.ByteWidth - evidence->element_offset) {
      evidence->evidence_flags |= 64u;
    }
  };
  if (!supported_dimension || !supported_bufferex_flags || srv.Format != DXGI_FORMAT_UNKNOWN) {
    if (evidence != nullptr) {
      evidence->failure = (!supported_dimension || !supported_bufferex_flags)
                              ? DiagnosticStructuredFailure::WrongViewDimension
                              : DiagnosticStructuredFailure::WrongFormat;
      Microsoft::WRL::ComPtr<ID3D11Resource> diagnostic_resource;
      view->GetResource(&diagnostic_resource);
      if (diagnostic_resource != nullptr) {
        evidence->evidence_flags |= 2u;
        evidence->resource_identity = reinterpret_cast<uint64_t>(diagnostic_resource.Get());
        Microsoft::WRL::ComPtr<ID3D11Buffer> diagnostic_buffer;
        if (SUCCEEDED(diagnostic_resource.As(&diagnostic_buffer))) {
          evidence->evidence_flags |= 4u;
          D3D11_BUFFER_DESC diagnostic_desc = {};
          diagnostic_buffer->GetDesc(&diagnostic_desc);
          capture_buffer_desc(diagnostic_desc);
        }
      }
    }
    return false;
  }
  Microsoft::WRL::ComPtr<ID3D11Resource> resource;
  view->GetResource(&resource);
  if (evidence != nullptr && resource != nullptr) {
    evidence->evidence_flags |= 2u;
    evidence->resource_identity = reinterpret_cast<uint64_t>(resource.Get());
  }
  if (!resource) {
    if (evidence != nullptr) evidence->failure = DiagnosticStructuredFailure::ResourceUnavailable;
    return false;
  }
  Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
  if (FAILED(resource.As(&buffer))) {
    if (evidence != nullptr) evidence->failure = DiagnosticStructuredFailure::ResourceNotBuffer;
    return false;
  }
  if (evidence != nullptr) evidence->evidence_flags |= 4u;
  D3D11_BUFFER_DESC desc = {};
  buffer->GetDesc(&desc);
  if (evidence != nullptr) {
    capture_buffer_desc(desc);
  }
  if ((desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) == 0u) {
    if (evidence != nullptr) evidence->failure = DiagnosticStructuredFailure::MissingStructuredFlag;
    return false;
  }
  const StructuredInputRange range = {.buffer_bytes = desc.ByteWidth, .first_element = first_element, .element_count = element_count, .stride = desc.StructureByteStride};
  const DiagnosticStructuredFailure validation_failure = StructuredInputFailure(range, stride);
  if (evidence != nullptr) {
    evidence->failure = validation_failure;
    if (evidence->element_offset <= desc.ByteWidth
        && evidence->element_bytes <= desc.ByteWidth - evidence->element_offset) {
      evidence->evidence_flags |= 64u;
    }
  }
  if (validation_failure != DiagnosticStructuredFailure::None) return false;
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
