#pragma once

// Stream-out probe for deforming vertex shaders (path 1, round S1: discovery).
//
// Skinned, wind, billboard and animated vertex shaders (contract::
// IsDeformingClass) move vertices with more than the instance world matrix,
// so the pool cannot rebuild their triangles from the vertex buffer. D3D11 can
// capture a vertex shader's outputs: a geometry shader created from the VS
// bytecode with a stream-output declaration (CreateGeometryShaderWithStream-
// Output, D3D11_SO_NO_RASTERIZED_STREAM) passes every vertex through to a
// stream-output buffer and rasterizes nothing.
//
// This probe re-issues sampled camera-view draws of such shaders with that
// geometry shader and a stream-output buffer bound, capturing SV_Position and
// every float output that holds at least xyz. Two presents later it reads the
// capture back (no GPU wait) and finds the output whose projection with the
// frame's viewProj (cb_scene, captured from the lighting pass of the same
// frame and stamped on the slot at that frame's present) reproduces
// SV_Position: the shader's world-space position. A stream-
// output statistics query per probe checks that every primitive was written.
// The queries are native D3D11 queries (like gpu_timer.hpp): ReShade 6.8
// (API 20) changed the signature of device::get_query_heap_results, so an
// add-on built against an older header passes shifted arguments to a newer
// runtime (seen in game: every read failed).
// Nothing enters the pool or the BVH yet; the results (per shader: which
// output, error, vertex counts, bounds, skip reasons) are for the next rounds.
//
// Game state: probes run only on the immediate context, only when the game
// has no geometry, hull or domain shader and no stream-output target bound
// (DeformBackend::check_game_state), and only after the game's draw state is
// complete (the draw event). The probe binds its geometry shader and target,
// draws with the draw's own arguments, then unbinds both, restoring exactly
// the state it checked. Nothing is rasterized, so render targets, depth,
// blend state and the pixel shader are not touched.
//
// Lock rule (bvh_pool.hpp): probes are reserved under g_deform.mutex and
// recorded after it is released; no graphics call happens under the mutex.

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <include/reshade.hpp>

#if defined(_WIN32)
#include <d3d11.h>
#endif

#include "../../../../utils/log.hpp"
#include "../../../../utils/path.hpp"
#include "../contract/shader_registry.hpp"
#include "../debug/pool_stage.hpp"
#include "../world_state.hpp"
#include "bvh_pool.hpp"

namespace falcom_world::bvh {

inline constexpr uint32_t kDeformSlots = 3u;                         // read back two presents later
inline constexpr uint64_t kDeformSlotBytes = 8ull * 1024ull * 1024ull;  // stream-output capture per frame
inline constexpr uint32_t kDeformProbesPerFrame = 4u;
inline constexpr uint32_t kDeformProbeInterval = 30u;   // frames between probes of one shader
inline constexpr uint32_t kDeformMaxStride = 2048u;     // D3D11 stream-output buffer stride limit
inline constexpr uint32_t kDeformMaxElements = 64u;
inline constexpr uint32_t kDeformErrorSamples = 4096u;  // vertices compared per probe
inline constexpr float kDeformMatchTolerance = 1e-3f;   // max |clip - SV_Position| / |w|
// An indirect draw's size is only known on the GPU: its capture gets the rest
// of the slot (stream output stops at the end of the buffer, so it cannot
// spill into another capture) and closes the slot for the frame.
inline constexpr uint64_t kDeformIndirectMinBytes = 256ull * 1024ull;

// Why a probe did not run (or its read was not evaluated).
enum class DeformSkip : uint8_t {
  None = 0,
  NotIndexed,          // non-indexed draw (not probed in this round)
  Topology,            // not a triangle list
  LightView,           // projects with the light (shadow pass): no camera to check against
  Deferred,            // deferred context: may run after the slot is read
  NoBytecode,          // the registry has no bytecode for this pipeline
  LayoutUnsupported,   // no SV_Position or no float xyz output
  ShaderCreateFailed,  // CreateGeometryShaderWithStreamOutput failed (HRESULT kept)
  Interval,            // probed recently
  FrameBudget,         // kDeformProbesPerFrame reached
  NoResources,         // stream-output or staging buffers missing
  SlotFull,            // capture would not fit this frame's slot
  TooLarge,            // capture larger than a whole slot
  GameGeometryShader,  // the game has a geometry shader bound
  GameTessellation,    // the game has a hull or domain shader bound
  GameStreamOut,       // the game has a stream-output target bound
  Count,
};

inline const char* DeformSkipName(DeformSkip skip) {
  switch (skip) {
    case DeformSkip::NotIndexed:         return "not_indexed";
    case DeformSkip::Topology:           return "topology";
    case DeformSkip::LightView:          return "light_view";
    case DeformSkip::Deferred:           return "deferred";
    case DeformSkip::NoBytecode:         return "no_bytecode";
    case DeformSkip::LayoutUnsupported:  return "layout_unsupported";
    case DeformSkip::ShaderCreateFailed: return "shader_create_failed";
    case DeformSkip::Interval:           return "interval";
    case DeformSkip::FrameBudget:        return "frame_budget";
    case DeformSkip::NoResources:        return "no_resources";
    case DeformSkip::SlotFull:           return "slot_full";
    case DeformSkip::TooLarge:           return "too_large";
    case DeformSkip::GameGeometryShader: return "game_geometry_shader";
    case DeformSkip::GameTessellation:   return "game_tessellation";
    case DeformSkip::GameStreamOut:      return "game_stream_out";
    default:                             return "none";
  }
}

// Why a read probe was not judged.
enum class DeformReadFail : uint8_t {
  None = 0,
  QueryNotReady,     // statistics not available yet (not counted as an error)
  QueryError,        // the statistics query could not be read
  Empty,             // indirect draw with no primitives (args zeroed by GPU culling)
  QueryMismatch,     // fewer primitives written than drawn (buffer overflow)
  NoCamera,          // no camera captured
  CameraFrame,       // camera from another frame than the probe
  NoMatch,           // no output projects to SV_Position
  Count,
};

inline const char* DeformReadFailName(DeformReadFail fail) {
  switch (fail) {
    case DeformReadFail::QueryNotReady: return "query_not_ready";
    case DeformReadFail::QueryError:    return "query_error";
    case DeformReadFail::Empty:         return "empty";
    case DeformReadFail::QueryMismatch: return "query_mismatch";
    case DeformReadFail::NoCamera:      return "no_camera";
    case DeformReadFail::CameraFrame:   return "camera_frame";
    case DeformReadFail::NoMatch:       return "no_match";
    default:                            return "none";
  }
}

// ---------------------------------------------------------------------------
// Stream-output layout and evaluation (pure, no graphics calls).

struct DeformSoElement {
  std::string semantic;
  uint32_t semantic_index = 0u;
  uint8_t start = 0u;      // first component (0 = x)
  uint8_t count = 0u;      // components captured
  uint32_t offset = 0u;    // floats from the start of a captured vertex
  bool position = false;   // SV_Position
  bool candidate = false;  // float output with xyz at components 0..2
};

struct DeformSoLayout {
  std::vector<DeformSoElement> elements;
  uint32_t stride = 0u;    // bytes per captured vertex
  int32_t position = -1;   // index of SV_Position in `elements`
  uint32_t candidates = 0u;
};

inline uint32_t DeformMaskCount(uint8_t mask) {
  uint32_t count = 0u;
  for (uint32_t bit = 0; bit < 4u; ++bit) count += (mask >> bit) & 1u;
  return count;
}

// SV_Position plus every float output whose components are contiguous.
// Returns nullptr when usable, else why not.
inline const char* BuildDeformLayout(const std::vector<dxbc::SignatureElement>& outputs, DeformSoLayout* out) {
  *out = {};
  for (const auto& output : outputs) {
    if (output.component_type != dxbc::kComponentFloat32) continue;
    const uint8_t mask = output.mask & 0xFu;
    if (mask == 0u) continue;
    uint8_t start = 0u;
    while (((mask >> start) & 1u) == 0u) ++start;
    const uint32_t count = DeformMaskCount(mask);
    if (((mask >> start) & ((1u << count) - 1u)) != ((1u << count) - 1u)) continue;  // gaps
    DeformSoElement element;
    element.semantic = output.semantic;
    element.semantic_index = output.semantic_index;
    element.start = start;
    element.count = static_cast<uint8_t>(count);
    element.offset = out->stride / 4u;
    element.position = output.system_value == dxbc::kSystemValuePosition;
    if (element.position && (start != 0u || count != 4u)) return "SV_Position is not xyzw";
    element.candidate = !element.position && output.system_value == 0u && start == 0u && count >= 3u;
    if (!element.position && !element.candidate) continue;
    if (element.position) out->position = static_cast<int32_t>(out->elements.size());
    if (element.candidate) out->candidates += 1u;
    out->stride += count * 4u;
    out->elements.push_back(std::move(element));
  }
  if (out->position < 0) return "no SV_Position output";
  if (out->candidates == 0u) return "no float xyz output";
  if (out->stride > kDeformMaxStride || out->elements.size() > kDeformMaxElements) return "too many outputs";
  return nullptr;
}

inline std::string DeformElementName(const DeformSoElement& element) {
  static const char* kComponents = "xyzw";
  std::string name = element.semantic + std::to_string(element.semantic_index) + ".";
  for (uint32_t c = 0; c < element.count; ++c) name.push_back(kComponents[element.start + c]);
  return name;
}

struct DeformCandidateError {
  float error[2] = {-1.f, -1.f};  // max |clip - SV_Position| / |w|: [0] contiguous rows, [1] transposed
  float w_error = 0.f;            // max |w - 1| when the output has a w component
};

struct DeformEvaluation {
  std::vector<DeformCandidateError> candidates;  // per element (unused for non-candidates)
  int32_t chosen = -1;     // element index of the world position
  uint8_t convention = 0u;
  float error = -1.f;      // chosen error
  uint32_t samples = 0u;
  uint32_t vertices = 0u;
  uint32_t triangles = 0u;
  uint32_t degenerate = 0u;  // zero-area triangles of the chosen output
  uint32_t nonfinite = 0u;   // vertices with a non-finite chosen position
  float bbox_min[3] = {0.f, 0.f, 0.f};
  float bbox_max[3] = {0.f, 0.f, 0.f};
};

// `data` holds `vertices` captured vertices (`layout.stride` bytes each),
// three per triangle in draw order. view_proj: cb_scene viewProj_g floats.
inline DeformEvaluation EvaluateDeformCapture(
    const uint8_t* data, uint32_t vertices, const DeformSoLayout& layout, const float* view_proj) {
  DeformEvaluation result;
  result.vertices = vertices;
  result.triangles = vertices / 3u;
  result.candidates.resize(layout.elements.size());
  if (vertices == 0u || layout.position < 0) return result;
  const uint32_t step = (std::max)(1u, vertices / kDeformErrorSamples);
  const uint32_t stride_floats = layout.stride / 4u;
  const auto vertex = [&](uint32_t index) {
    return reinterpret_cast<const float*>(data) + static_cast<size_t>(index) * stride_floats;
  };
  const uint32_t position_offset = layout.elements[static_cast<size_t>(layout.position)].offset;
  for (size_t e = 0; e < layout.elements.size(); ++e) {
    const DeformSoElement& element = layout.elements[e];
    if (!element.candidate) continue;
    DeformCandidateError& candidate = result.candidates[e];
    candidate.error[0] = 0.f;
    candidate.error[1] = 0.f;
    for (uint32_t i = 0; i < vertices; i += step) {
      float p[4] = {};
      float reference[4] = {};
      std::memcpy(p, vertex(i) + element.offset, sizeof(float) * 3u);
      p[3] = 1.f;
      std::memcpy(reference, vertex(i) + position_offset, sizeof(reference));
      const float scale = (std::max)(std::fabs(reference[3]), 1e-2f);
      for (int convention = 0; convention < 2; ++convention) {
        float worst = 0.f;
        for (int row = 0; row < 4; ++row) {
          float clip = 0.f;
          for (int k = 0; k < 4; ++k) clip += p[k] * (convention == 0 ? view_proj[row * 4 + k] : view_proj[k * 4 + row]);
          const float difference = std::fabs(clip - reference[row]) / scale;
          worst = std::isfinite(difference) ? (std::max)(worst, difference) : 1e30f;
        }
        candidate.error[convention] = (std::max)(candidate.error[convention], worst);
      }
      if (element.count == 4u) {
        candidate.w_error = (std::max)(candidate.w_error, std::fabs(vertex(i)[element.offset + 3u] - 1.f));
      }
    }
    for (int convention = 0; convention < 2; ++convention) {
      const float error = candidate.error[convention];
      if (error <= kDeformMatchTolerance && (result.chosen < 0 || error < result.error)) {
        result.chosen = static_cast<int32_t>(e);
        result.convention = static_cast<uint8_t>(convention);
        result.error = error;
      }
    }
  }
  result.samples = (vertices + step - 1u) / step;
  if (result.chosen < 0) return result;

  const uint32_t offset = layout.elements[static_cast<size_t>(result.chosen)].offset;
  bool have_bounds = false;
  for (uint32_t i = 0; i < vertices; ++i) {
    const float* p = vertex(i) + offset;
    if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) {
      result.nonfinite += 1u;
      continue;
    }
    for (int k = 0; k < 3; ++k) {
      result.bbox_min[k] = have_bounds ? (std::min)(result.bbox_min[k], p[k]) : p[k];
      result.bbox_max[k] = have_bounds ? (std::max)(result.bbox_max[k], p[k]) : p[k];
    }
    have_bounds = true;
  }
  for (uint32_t t = 0; t + 2u < vertices; t += 3u) {
    const float* a = vertex(t) + offset;
    const float* b = vertex(t + 1u) + offset;
    const float* c = vertex(t + 2u) + offset;
    const float e1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const float e2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    const float n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
    if (!(n[0] * n[0] + n[1] * n[1] + n[2] * n[2] > 0.f)) result.degenerate += 1u;
  }
  return result;
}

// ---------------------------------------------------------------------------
// Native operations the probe needs beyond the ReShade API (ReShade's
// create_pipeline does not implement stream_output_state on D3D11). The
// harness replaces them with mocks.

struct DeformBackend {
  // Pass-through stream-output geometry shader for a VS bytecode; 0 on
  // failure with the HRESULT in *hr.
  uint64_t (*create_so_shader)(reshade::api::device* device, const contract::ShaderCode& code,
                               const DeformSoLayout& layout, int32_t* hr) = nullptr;
  void (*destroy_so_shader)(uint64_t shader) = nullptr;
  // What the game has bound that a probe must not disturb (None = free).
  DeformSkip (*check_game_state)(reshade::api::command_list* cmd_list) = nullptr;
  // Stream-output statistics query (stream 0): create, destroy, begin/end on
  // a command list, and read on the immediate context without flushing:
  // 1 = ready (out[0] primitives written, out[1] storage needed), 0 = not
  // ready, -1 = error.
  uint64_t (*create_query)(reshade::api::device* device) = nullptr;
  void (*destroy_query)(uint64_t query) = nullptr;
  void (*begin_query)(reshade::api::command_list* cmd_list, uint64_t query) = nullptr;
  void (*end_query)(reshade::api::command_list* cmd_list, uint64_t query) = nullptr;
  int (*read_query)(reshade::api::command_list* immediate, uint64_t query, uint64_t* out) = nullptr;
};

#if defined(_WIN32)
inline uint64_t CreateDeformSoShaderD3D11(
    reshade::api::device* device, const contract::ShaderCode& code, const DeformSoLayout& layout, int32_t* hr) {
  *hr = 0;
  if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return 0u;
  auto* native = reinterpret_cast<ID3D11Device*>(static_cast<uintptr_t>(device->get_native()));  // NOLINT(performance-no-int-to-ptr)
  if (native == nullptr) return 0u;
  std::vector<D3D11_SO_DECLARATION_ENTRY> entries;
  entries.reserve(layout.elements.size());
  for (const DeformSoElement& element : layout.elements) {
    D3D11_SO_DECLARATION_ENTRY entry = {};
    entry.Stream = 0u;
    entry.SemanticName = element.semantic.c_str();
    entry.SemanticIndex = element.semantic_index;
    entry.StartComponent = element.start;
    entry.ComponentCount = element.count;
    entry.OutputSlot = 0u;
    entries.push_back(entry);
  }
  const UINT stride = layout.stride;
  ID3D11GeometryShader* shader = nullptr;
  const HRESULT result = native->CreateGeometryShaderWithStreamOutput(
      code.bytecode.data(), code.bytecode.size(), entries.data(), static_cast<UINT>(entries.size()), &stride, 1u,
      D3D11_SO_NO_RASTERIZED_STREAM, nullptr, &shader);
  *hr = static_cast<int32_t>(result);
  if (FAILED(result) || shader == nullptr) return 0u;
  return reinterpret_cast<uint64_t>(shader);
}

inline void DestroyDeformSoShaderD3D11(uint64_t shader) {
  if (shader != 0u) reinterpret_cast<ID3D11GeometryShader*>(shader)->Release();  // NOLINT(performance-no-int-to-ptr)
}

inline DeformSkip CheckDeformGameStateD3D11(reshade::api::command_list* cmd_list) {
  auto* context = reinterpret_cast<ID3D11DeviceContext*>(static_cast<uintptr_t>(cmd_list->get_native()));  // NOLINT(performance-no-int-to-ptr)
  if (context == nullptr) return DeformSkip::NoResources;
  DeformSkip skip = DeformSkip::None;
  ID3D11GeometryShader* geometry = nullptr;
  context->GSGetShader(&geometry, nullptr, nullptr);
  if (geometry != nullptr) {
    geometry->Release();
    skip = DeformSkip::GameGeometryShader;
  }
  ID3D11HullShader* hull = nullptr;
  context->HSGetShader(&hull, nullptr, nullptr);
  if (hull != nullptr) {
    hull->Release();
    if (skip == DeformSkip::None) skip = DeformSkip::GameTessellation;
  }
  ID3D11DomainShader* domain = nullptr;
  context->DSGetShader(&domain, nullptr, nullptr);
  if (domain != nullptr) {
    domain->Release();
    if (skip == DeformSkip::None) skip = DeformSkip::GameTessellation;
  }
  ID3D11Buffer* targets[D3D11_SO_BUFFER_SLOT_COUNT] = {};
  context->SOGetTargets(D3D11_SO_BUFFER_SLOT_COUNT, targets);
  for (ID3D11Buffer* target : targets) {
    if (target == nullptr) continue;
    target->Release();
    if (skip == DeformSkip::None) skip = DeformSkip::GameStreamOut;
  }
  return skip;
}

inline uint64_t CreateDeformQueryD3D11(reshade::api::device* device) {
  if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return 0u;
  auto* native = reinterpret_cast<ID3D11Device*>(static_cast<uintptr_t>(device->get_native()));  // NOLINT(performance-no-int-to-ptr)
  if (native == nullptr) return 0u;
  D3D11_QUERY_DESC desc = {};
  desc.Query = D3D11_QUERY_SO_STATISTICS_STREAM0;
  ID3D11Query* query = nullptr;
  if (FAILED(native->CreateQuery(&desc, &query)) || query == nullptr) return 0u;
  return reinterpret_cast<uint64_t>(query);
}

inline void DestroyDeformQueryD3D11(uint64_t query) {
  if (query != 0u) reinterpret_cast<ID3D11Query*>(query)->Release();  // NOLINT(performance-no-int-to-ptr)
}

inline ID3D11DeviceContext* DeformContextD3D11(reshade::api::command_list* cmd_list) {
  return reinterpret_cast<ID3D11DeviceContext*>(static_cast<uintptr_t>(cmd_list->get_native()));  // NOLINT(performance-no-int-to-ptr)
}

inline void BeginDeformQueryD3D11(reshade::api::command_list* cmd_list, uint64_t query) {
  if (auto* context = DeformContextD3D11(cmd_list); context != nullptr) {
    context->Begin(reinterpret_cast<ID3D11Query*>(query));  // NOLINT(performance-no-int-to-ptr)
  }
}

inline void EndDeformQueryD3D11(reshade::api::command_list* cmd_list, uint64_t query) {
  if (auto* context = DeformContextD3D11(cmd_list); context != nullptr) {
    context->End(reinterpret_cast<ID3D11Query*>(query));  // NOLINT(performance-no-int-to-ptr)
  }
}

inline int ReadDeformQueryD3D11(reshade::api::command_list* immediate, uint64_t query, uint64_t* out) {
  auto* context = DeformContextD3D11(immediate);
  if (context == nullptr || query == 0u) return -1;
  D3D11_QUERY_DATA_SO_STATISTICS data = {};
  const HRESULT result = context->GetData(reinterpret_cast<ID3D11Query*>(query), &data, sizeof(data),  // NOLINT(performance-no-int-to-ptr)
                                          D3D11_ASYNC_GETDATA_DONOTFLUSH);
  if (result == S_FALSE) return 0;
  if (result != S_OK) return -1;
  out[0] = data.NumPrimitivesWritten;
  out[1] = data.PrimitivesStorageNeeded;
  return 1;
}

inline DeformBackend DefaultDeformBackend() {
  return {CreateDeformSoShaderD3D11, DestroyDeformSoShaderD3D11, CheckDeformGameStateD3D11, CreateDeformQueryD3D11,
          DestroyDeformQueryD3D11,   BeginDeformQueryD3D11,      EndDeformQueryD3D11,       ReadDeformQueryD3D11};
}
#else
inline DeformBackend DefaultDeformBackend() { return {}; }
#endif

// ---------------------------------------------------------------------------
// State.

// Per VS pipeline handle.
struct DeformShader {
  uint32_t hash = 0u;
  uint8_t cls = 0u;
  bool layout_ok = false;
  std::string layout_error;
  DeformSoLayout layout;
  std::shared_ptr<const contract::ShaderCode> code;
  uint64_t so_shader = 0u;
  bool create_failed = false;
  int32_t create_hr = 0;
  uint32_t last_probe_frame = 0u;
  bool probed = false;
  // Results.
  uint32_t draws = 0u;   // camera-view draws seen while probing was on
  uint32_t probes = 0u;  // issued
  uint32_t reads = 0u;   // read back and judged
  uint32_t matched = 0u;
  std::array<uint32_t, static_cast<size_t>(DeformSkip::Count)> skips = {};
  std::array<uint32_t, static_cast<size_t>(DeformReadFail::Count)> read_fails = {};
  int32_t chosen = -1;        // element index of the latest match
  bool inconsistent = false;  // two matches chose different outputs
  uint8_t convention = 0u;
  float worst_error = 0.f;    // largest chosen error over matches
  DeformEvaluation last;      // latest judged read
  uint32_t last_index_count = 0u;
  uint32_t last_instance_count = 0u;
  bool last_indirect = false;
  uint32_t indirect_probes = 0u;
};

struct DeformPending {
  uint64_t vs_pipeline = 0u;
  uint32_t hash = 0u;
  uint64_t offset = 0u;     // bytes into the slot
  uint32_t vertices = 0u;
  uint32_t stride = 0u;
  uint32_t query = 0u;      // index into the slot's queries
  uint32_t frame = 0u;
  uint32_t index_count = 0u;
  uint32_t instance_count = 0u;
  bool issued = false;      // the draw was recorded (a failed state check leaves it false)
  bool indirect = false;    // vertices come from the statistics at read; `capacity` bytes reserved
  uint64_t capacity = 0u;
};

struct DeformSlot {
  reshade::api::resource so_buffer = {0u};
  reshade::api::resource staging = {0u};
  std::array<uint64_t, kDeformProbesPerFrame> queries = {};  // DeformBackend queries
  uint64_t used = 0u;
  bool copied = false;  // staging copy recorded at present
  bool resolving = false;
  bool closed = false;  // an indirect capture took the rest of the slot
  // Camera of the frame the probes ran in (stamped when the slot is closed at
  // that frame's present; the lighting pass captured it during the frame).
  bool camera_valid = false;
  uint32_t camera_frame = 0u;
  float view_proj[16] = {};
  std::vector<DeformPending> pending;
};

struct DeformStats {
  uint64_t probes = 0u;
  uint64_t reads = 0u;
  uint64_t matched = 0u;
  uint32_t resource_failures = 0u;
  uint32_t map_failures = 0u;
  uint32_t shaders_created = 0u;
  uint32_t shader_failures = 0u;
  std::array<uint64_t, static_cast<size_t>(DeformSkip::Count)> skips = {};
  std::array<uint64_t, static_cast<size_t>(DeformReadFail::Count)> read_fails = {};
};

struct DeformProbeState {
  std::atomic_bool enabled{true};
  std::mutex mutex;
  DeformBackend backend = DefaultDeformBackend();
  reshade::api::device* device = nullptr;
  std::array<DeformSlot, kDeformSlots> slots;
  uint32_t write_slot = 0u;
  uint32_t frame_probes = 0u;
  uint32_t probe_frame = 0u;  // frame `frame_probes` counts
  std::unordered_map<uint64_t, DeformShader> shaders;  // by VS pipeline handle
  std::vector<uint64_t> retired_so_shaders;            // released at the next present (outside the lock)
  DeformStats stats;
};

inline DeformProbeState g_deform;

// Caller holds g_deform.mutex. The entry for a pipeline, reset when its
// bytecode changed (handle reused).
inline DeformShader& DeformShaderEntry(uint64_t pipeline, const contract::ShaderCode& code,
                                       const std::shared_ptr<const contract::ShaderCode>& shared) {
  DeformShader& shader = g_deform.shaders[pipeline];
  if (shader.code == nullptr || shader.hash != code.hash) {
    if (shader.so_shader != 0u) g_deform.retired_so_shaders.push_back(shader.so_shader);
    shader = DeformShader{};
    shader.hash = code.hash;
    shader.cls = code.cls;
    shader.code = shared;
    const char* error = BuildDeformLayout(code.outputs, &shader.layout);
    shader.layout_ok = error == nullptr;
    if (error != nullptr) shader.layout_error = error;
  }
  return shader;
}

inline void CountDeformSkip(DeformShader* shader, DeformSkip skip) {
  g_deform.stats.skips[static_cast<size_t>(skip)] += 1u;
  if (shader != nullptr) shader->skips[static_cast<size_t>(skip)] += 1u;
}

// The args location of an indirect draw (one DrawIndexedInstancedIndirect).
struct DeformIndirectArgs {
  reshade::api::resource buffer = {0u};
  uint64_t offset = 0u;
  uint32_t stride = 20u;
};

// Draw event (immediate or deferred context), after the draw's state is set.
// `indirect` is null for a direct draw.
inline void DeformProbeDraw(
    reshade::api::device* device, reshade::api::command_list* cmd_list, const DrawRecord& draw,
    const DeformIndirectArgs* indirect) {
  if (!g_deform.enabled.load(std::memory_order_relaxed)) return;
  if (device == nullptr || cmd_list == nullptr) return;
  const contract::ShaderTraits traits = contract::LookupVertexTraits(draw.vs_pipeline);
  if (!contract::IsDeformingClass(static_cast<contract::VsClass>(traits.cls))) return;
  PoolStageScope stage("deform probe: reserve");

  const std::shared_ptr<const contract::ShaderCode> code = contract::LookupDeformingCode(draw.vs_pipeline);
  const bool deferred = IsPoolDeferredList(cmd_list);
  const uint32_t frame = draw.frame;

  enum class Action : uint8_t { None, CreateShader, Probe };
  Action action = Action::None;
  DeformSoLayout layout;  // copy for shader creation outside the lock
  uint64_t so_shader = 0u;
  DeformPending pending;
  reshade::api::resource so_buffer = {0u};
  uint64_t query = 0u;
  {
    std::lock_guard<std::mutex> lock(g_deform.mutex);
    if (code == nullptr) {
      CountDeformSkip(nullptr, DeformSkip::NoBytecode);
      return;
    }
    DeformShader& shader = DeformShaderEntry(draw.vs_pipeline, *code, code);
    shader.draws += 1u;
    if ((traits.flags & contract::kTraitCameraView) == 0u) {
      CountDeformSkip(&shader, DeformSkip::LightView);
      return;
    }
    if (draw.method != 1u || !draw.has_index_buffer) {
      CountDeformSkip(&shader, DeformSkip::NotIndexed);
      return;
    }
    if ((draw.topology != reshade::api::primitive_topology::undefined
         && draw.topology != reshade::api::primitive_topology::triangle_list)
        || (indirect == nullptr && draw.index_count < 3u)) {
      CountDeformSkip(&shader, DeformSkip::Topology);
      return;
    }
    if (!shader.layout_ok) {
      CountDeformSkip(&shader, DeformSkip::LayoutUnsupported);
      return;
    }
    if (shader.create_failed) {
      CountDeformSkip(&shader, DeformSkip::ShaderCreateFailed);
      return;
    }
    if (shader.probed && frame - shader.last_probe_frame < kDeformProbeInterval) {
      CountDeformSkip(&shader, DeformSkip::Interval);
      return;
    }
    if (deferred) {
      CountDeformSkip(&shader, DeformSkip::Deferred);
      return;
    }
    if (shader.so_shader == 0u) {
      if (g_deform.backend.create_so_shader == nullptr) {
        CountDeformSkip(&shader, DeformSkip::NoResources);
        return;
      }
      action = Action::CreateShader;
      layout = shader.layout;
    } else {
      if (g_deform.probe_frame != frame) {
        g_deform.probe_frame = frame;
        g_deform.frame_probes = 0u;
      }
      DeformSlot& slot = g_deform.slots[g_deform.write_slot];
      const uint32_t instances = draw.instance_count == 0u ? 1u : draw.instance_count;
      // Stream output writes each triangle's three vertices in draw order.
      const uint64_t vertices = static_cast<uint64_t>(draw.index_count / 3u) * 3u * instances;
      const uint64_t bytes = vertices * shader.layout.stride;
      if (g_deform.frame_probes >= kDeformProbesPerFrame) {
        CountDeformSkip(&shader, DeformSkip::FrameBudget);
        return;
      }
      if (g_deform.device != device || slot.so_buffer.handle == 0u || slot.queries[0] == 0u || slot.resolving
          || g_deform.backend.begin_query == nullptr || g_deform.backend.end_query == nullptr) {
        CountDeformSkip(&shader, DeformSkip::NoResources);
        return;
      }
      if (indirect == nullptr && (vertices == 0u || bytes > kDeformSlotBytes)) {
        CountDeformSkip(&shader, DeformSkip::TooLarge);
        return;
      }
      const uint64_t offset = (slot.used + 15u) & ~uint64_t{15};
      const uint64_t reserved = indirect != nullptr ? (offset < kDeformSlotBytes ? kDeformSlotBytes - offset : 0u) : bytes;
      if (slot.closed || offset + reserved > kDeformSlotBytes
          || (indirect != nullptr && reserved < kDeformIndirectMinBytes)) {
        CountDeformSkip(&shader, DeformSkip::SlotFull);
        return;
      }
      action = Action::Probe;
      so_shader = shader.so_shader;
      so_buffer = slot.so_buffer;
      query = slot.queries[g_deform.frame_probes];
      pending.vs_pipeline = draw.vs_pipeline;
      pending.hash = shader.hash;
      pending.offset = offset;
      pending.vertices = indirect != nullptr ? 0u : static_cast<uint32_t>(vertices);
      pending.stride = shader.layout.stride;
      pending.query = g_deform.frame_probes;
      pending.frame = frame;
      pending.index_count = indirect != nullptr ? 0u : draw.index_count;
      pending.instance_count = indirect != nullptr ? 0u : instances;
      pending.indirect = indirect != nullptr;
      pending.capacity = reserved;
      slot.used = offset + reserved;
      if (indirect != nullptr) slot.closed = true;
      g_deform.frame_probes += 1u;
    }
  }

  if (action == Action::CreateShader) {
    stage.Set("deform probe: create shader");
    int32_t hr = 0;
    const uint64_t created = g_deform.backend.create_so_shader(device, *code, layout, &hr);
    std::lock_guard<std::mutex> lock(g_deform.mutex);
    DeformShader& shader = DeformShaderEntry(draw.vs_pipeline, *code, code);
    shader.create_hr = hr;
    if (created == 0u) {
      shader.create_failed = true;
      g_deform.stats.shader_failures += 1u;
      CountDeformSkip(&shader, DeformSkip::ShaderCreateFailed);
    } else if (shader.so_shader != 0u) {
      g_deform.retired_so_shaders.push_back(created);  // created twice (another thread): keep the first
    } else {
      shader.so_shader = created;
      g_deform.stats.shaders_created += 1u;
    }
    return;
  }
  if (action != Action::Probe) return;

  stage.Set("deform probe: issue");
  const DeformSkip state =
      g_deform.backend.check_game_state != nullptr ? g_deform.backend.check_game_state(cmd_list) : DeformSkip::None;
  if (state == DeformSkip::None) {
    using reshade::api::pipeline_stage;
    const auto stages = pipeline_stage::geometry_shader | pipeline_stage::stream_output;
    const uint64_t offset = pending.offset;
    const uint64_t no_offset = 0u;
    const reshade::api::resource no_buffer = {0u};
    cmd_list->bind_pipeline(stages, reshade::api::pipeline{so_shader});
    cmd_list->bind_stream_output_buffers(0u, 1u, &so_buffer, &offset, nullptr, nullptr, nullptr);
    g_deform.backend.begin_query(cmd_list, query);
    if (indirect != nullptr) {
      cmd_list->draw_or_dispatch_indirect(reshade::api::indirect_command::draw_indexed, indirect->buffer,
                                          indirect->offset, 1u, indirect->stride);
    } else {
      cmd_list->draw_indexed(draw.index_count, pending.instance_count, draw.first_index, draw.vertex_offset,
                             draw.first_instance);
    }
    g_deform.backend.end_query(cmd_list, query);
    cmd_list->bind_stream_output_buffers(0u, 1u, &no_buffer, &no_offset, nullptr, nullptr, nullptr);
    cmd_list->bind_pipeline(stages, reshade::api::pipeline{0u});
    pending.issued = true;
  }

  std::lock_guard<std::mutex> lock(g_deform.mutex);
  DeformShader& shader = DeformShaderEntry(draw.vs_pipeline, *code, code);
  if (state != DeformSkip::None) {
    CountDeformSkip(&shader, state);
    return;  // the reserved bytes and query stay unused this frame
  }
  shader.probed = true;
  shader.last_probe_frame = frame;
  shader.probes += 1u;
  shader.last_index_count = pending.index_count;
  shader.last_instance_count = pending.instance_count;
  shader.last_indirect = pending.indirect;
  if (pending.indirect) shader.indirect_probes += 1u;
  g_deform.stats.probes += 1u;
  g_deform.slots[g_deform.write_slot].pending.push_back(pending);
}

inline void OnDeformProbeDraw(
    reshade::api::device* device, reshade::api::command_list* cmd_list, const DrawRecord& draw) {
  DeformProbeDraw(device, cmd_list, draw, nullptr);
}

// DrawIndexedInstancedIndirect (one draw; D3D11 reports stride 0).
inline void OnDeformProbeIndirectDraw(
    reshade::api::device* device, reshade::api::command_list* cmd_list, const DrawRecord& draw,
    reshade::api::resource args_buffer, uint64_t args_offset, uint32_t draw_count, uint32_t stride) {
  if (!g_deform.enabled.load(std::memory_order_relaxed)) return;
  if (draw.method != 1u || draw_count != 1u || args_buffer.handle == 0u) return;
  DeformIndirectArgs args;
  args.buffer = args_buffer;
  args.offset = args_offset;
  args.stride = stride == 0u ? 20u : stride;
  DeformProbeDraw(device, cmd_list, draw, &args);
}

// The game destroyed a VS pipeline: its stream-output shader goes with it.
inline void OnDestroyPipelineDeform(reshade::api::device* device, reshade::api::pipeline pipeline) {
  (void)device;
  std::lock_guard<std::mutex> lock(g_deform.mutex);
  const auto it = g_deform.shaders.find(pipeline.handle);
  if (it == g_deform.shaders.end()) return;
  if (it->second.so_shader != 0u) g_deform.retired_so_shaders.push_back(it->second.so_shader);
  g_deform.shaders.erase(it);
}

inline void ReleaseDeformShaders(const std::vector<uint64_t>& shaders) {
  if (g_deform.backend.destroy_so_shader == nullptr) return;
  for (const uint64_t shader : shaders) g_deform.backend.destroy_so_shader(shader);
}

inline void DestroyDeformResources(reshade::api::device* device, std::array<DeformSlot, kDeformSlots>& slots) {
  for (DeformSlot& slot : slots) {
    if (slot.so_buffer.handle != 0u) device->destroy_resource(slot.so_buffer);
    if (slot.staging.handle != 0u) device->destroy_resource(slot.staging);
    for (const uint64_t query : slot.queries) {
      if (query != 0u && g_deform.backend.destroy_query != nullptr) g_deform.backend.destroy_query(query);
    }
    slot = DeformSlot{};
  }
}

// Creates the slot resources; returns false on failure (nothing kept).
inline bool CreateDeformResources(reshade::api::device* device, std::array<DeformSlot, kDeformSlots>* slots) {
  using reshade::api::memory_heap;
  using reshade::api::resource_usage;
  for (DeformSlot& slot : *slots) {
    const bool ok =
        device->create_resource(reshade::api::resource_desc(kDeformSlotBytes, memory_heap::gpu_only,
                                                            resource_usage::stream_output | resource_usage::copy_source),
                                nullptr, resource_usage::copy_source, &slot.so_buffer)
        && device->create_resource(
            reshade::api::resource_desc(kDeformSlotBytes, memory_heap::gpu_to_cpu, resource_usage::copy_dest), nullptr,
            resource_usage::copy_dest, &slot.staging)
        && g_deform.backend.create_query != nullptr;
    bool queries_ok = ok;
    for (uint64_t& query : slot.queries) {
      if (!queries_ok) break;
      query = g_deform.backend.create_query(device);
      queries_ok = query != 0u;
    }
    if (!queries_ok) {
      DestroyDeformResources(device, *slots);
      return false;
    }
  }
  return true;
}

// Called on device destruction and when the probe is turned off.
inline void ReleaseDeformProbe(reshade::api::device* device) {
  std::array<DeformSlot, kDeformSlots> slots;
  std::vector<uint64_t> shaders;
  {
    std::lock_guard<std::mutex> lock(g_deform.mutex);
    if (device != nullptr && g_deform.device != device) return;
    slots = g_deform.slots;
    for (DeformSlot& slot : g_deform.slots) slot = DeformSlot{};
    for (auto& [pipeline, shader] : g_deform.shaders) {
      (void)pipeline;
      if (shader.so_shader != 0u) shaders.push_back(shader.so_shader);
      shader.so_shader = 0u;
    }
    shaders.insert(shaders.end(), g_deform.retired_so_shaders.begin(), g_deform.retired_so_shaders.end());
    g_deform.retired_so_shaders.clear();
    device = g_deform.device;
    g_deform.device = nullptr;
  }
  if (device != nullptr) DestroyDeformResources(device, slots);
  ReleaseDeformShaders(shaders);
}

inline void OnDestroyDeviceDeform(reshade::api::device* device) { ReleaseDeformProbe(device); }

struct DeformJudged {
  DeformPending pending;
  DeformReadFail fail = DeformReadFail::None;
  DeformEvaluation evaluation;
};

// Reads one slot whose probes the GPU finished two presents ago.
inline void ResolveDeformSlot(reshade::api::device* device, reshade::api::command_list* immediate, uint32_t slot_index) {
  std::vector<DeformPending> pending;
  bool camera_valid = false;
  uint32_t camera_frame = 0u;
  float view_proj[16] = {};
  reshade::api::resource staging = {0u};
  std::array<uint64_t, kDeformProbesPerFrame> queries = {};
  uint64_t used = 0u;
  bool copied = false;
  std::unordered_map<uint64_t, DeformSoLayout> layouts;
  {
    std::lock_guard<std::mutex> lock(g_deform.mutex);
    DeformSlot& slot = g_deform.slots[slot_index];
    pending.swap(slot.pending);
    staging = slot.staging;
    queries = slot.queries;
    used = slot.used;
    copied = slot.copied;
    camera_valid = slot.camera_valid;
    camera_frame = slot.camera_frame;
    std::memcpy(view_proj, slot.view_proj, sizeof(view_proj));
    if (pending.empty()) {
      slot.used = 0u;
      slot.copied = false;
      slot.closed = false;
      return;
    }
    slot.resolving = true;
    for (const DeformPending& entry : pending) {
      const auto it = g_deform.shaders.find(entry.vs_pipeline);
      if (it != g_deform.shaders.end() && it->second.hash == entry.hash) layouts[entry.vs_pipeline] = it->second.layout;
    }
  }

  std::vector<DeformJudged> judged;
  judged.reserve(pending.size());
  void* mapped = nullptr;
  const bool mapped_ok = copied && used != 0u && immediate != nullptr && g_deform.backend.read_query != nullptr
                         && device->map_buffer_region(staging, 0u, used, reshade::api::map_access::read_only, &mapped)
                         && mapped != nullptr;
  if (mapped_ok) {
    const auto* bytes = static_cast<const uint8_t*>(mapped);
    for (const DeformPending& entry : pending) {
      const auto layout = layouts.find(entry.vs_pipeline);
      if (layout == layouts.end()) continue;  // shader gone or replaced since
      DeformJudged result;
      result.pending = entry;
      // Read after the map: the map waited for the frame's GPU work, so the
      // statistics are normally ready.
      uint64_t statistics[2] = {};  // primitives written, storage needed
      const int read = g_deform.backend.read_query(immediate, queries[entry.query], statistics);
      if (read == 0) {
        result.fail = DeformReadFail::QueryNotReady;
      } else if (read < 0) {
        result.fail = DeformReadFail::QueryError;
      } else if (entry.indirect && statistics[0] == 0u && statistics[1] == 0u) {
        result.fail = DeformReadFail::Empty;
      } else if (entry.indirect
                     ? (statistics[1] != statistics[0] || statistics[0] * 3u * entry.stride > entry.capacity)
                     : (statistics[0] != entry.vertices / 3u || statistics[1] != statistics[0])) {
        result.fail = DeformReadFail::QueryMismatch;
      } else if (!camera_valid) {
        result.fail = DeformReadFail::NoCamera;
      } else if (camera_frame != entry.frame) {
        result.fail = DeformReadFail::CameraFrame;
      } else {
        if (entry.indirect) result.pending.vertices = static_cast<uint32_t>(statistics[0] * 3u);
        result.evaluation =
            EvaluateDeformCapture(bytes + entry.offset, result.pending.vertices, layout->second, view_proj);
        if (result.evaluation.chosen < 0) result.fail = DeformReadFail::NoMatch;
      }
      judged.push_back(std::move(result));
    }
    device->unmap_buffer_region(staging);
  }

  std::lock_guard<std::mutex> lock(g_deform.mutex);
  DeformSlot& slot = g_deform.slots[slot_index];
  slot.resolving = false;
  slot.used = 0u;
  slot.copied = false;
  slot.closed = false;
  if (!mapped_ok) {
    g_deform.stats.map_failures += 1u;
    return;
  }
  for (DeformJudged& result : judged) {
    g_deform.stats.read_fails[static_cast<size_t>(result.fail)] += 1u;
    const auto it = g_deform.shaders.find(result.pending.vs_pipeline);
    if (it == g_deform.shaders.end() || it->second.hash != result.pending.hash) continue;
    DeformShader& shader = it->second;
    shader.read_fails[static_cast<size_t>(result.fail)] += 1u;
    if (result.fail == DeformReadFail::QueryNotReady) continue;
    if (result.fail == DeformReadFail::Empty) {
      shader.probed = false;  // nothing drawn: the next draw of this shader may probe again
      continue;
    }
    g_deform.stats.reads += 1u;
    shader.reads += 1u;
    shader.last = result.evaluation;
    if (result.fail != DeformReadFail::None) continue;
    g_deform.stats.matched += 1u;
    shader.matched += 1u;
    if (shader.chosen >= 0 && shader.chosen != result.evaluation.chosen) shader.inconsistent = true;
    shader.chosen = result.evaluation.chosen;
    shader.convention = result.evaluation.convention;
    shader.worst_error = (std::max)(shader.worst_error, result.evaluation.error);
  }
}

// Present (after the frame's draws): copies this frame's captures to staging,
// advances the ring and reads the slot written two presents ago.
inline void DrainDeformProbe(reshade::api::device* device, reshade::api::command_queue* queue) {
  if (device == nullptr || queue == nullptr) return;
  if (!g_deform.enabled.load(std::memory_order_relaxed)) {
    if (g_deform.device != nullptr) ReleaseDeformProbe(nullptr);
    return;
  }
  PoolStageScope stage("present: deform probe");
  std::vector<uint64_t> retired;
  bool need_resources = false;
  {
    std::lock_guard<std::mutex> lock(g_deform.mutex);
    retired.swap(g_deform.retired_so_shaders);
    need_resources = g_deform.device != device;
  }
  ReleaseDeformShaders(retired);
  if (need_resources) {
    ReleaseDeformProbe(nullptr);
    std::array<DeformSlot, kDeformSlots> slots;
    const bool created = CreateDeformResources(device, &slots);
    std::lock_guard<std::mutex> lock(g_deform.mutex);
    if (!created) {
      g_deform.stats.resource_failures += 1u;
      return;
    }
    g_deform.slots = slots;
    g_deform.write_slot = 0u;
    g_deform.device = device;
    return;
  }

  // Copy the slot written this frame and stamp it with this frame's camera.
  CameraSnapshot camera;
  {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    camera = g_state.camera;
  }
  reshade::api::resource so_buffer = {0u};
  reshade::api::resource staging = {0u};
  uint64_t used = 0u;
  uint32_t written = 0u;
  {
    std::lock_guard<std::mutex> lock(g_deform.mutex);
    written = g_deform.write_slot;
    DeformSlot& slot = g_deform.slots[written];
    slot.camera_valid = camera.valid;
    slot.camera_frame = camera.frame;
    std::memcpy(slot.view_proj, camera.view_proj, sizeof(slot.view_proj));
    if (!slot.pending.empty() && slot.used != 0u) {
      so_buffer = slot.so_buffer;
      staging = slot.staging;
      used = slot.used;
    }
  }
  if (used != 0u) {
    if (auto* cmd_list = queue->get_immediate_command_list(); cmd_list != nullptr) {
      cmd_list->copy_buffer_region(so_buffer, 0u, staging, 0u, used);
      std::lock_guard<std::mutex> lock(g_deform.mutex);
      g_deform.slots[written].copied = true;
    }
  }

  uint32_t resolve = 0u;
  {
    std::lock_guard<std::mutex> lock(g_deform.mutex);
    g_deform.write_slot = (g_deform.write_slot + 1u) % kDeformSlots;
    resolve = g_deform.write_slot;
  }
  ResolveDeformSlot(device, queue->get_immediate_command_list(), resolve);
}

inline void ResetDeformProbeResults() {
  std::lock_guard<std::mutex> lock(g_deform.mutex);
  for (auto& [pipeline, shader] : g_deform.shaders) {
    (void)pipeline;
    const uint64_t so_shader = shader.so_shader;
    const auto code = shader.code;
    const uint32_t hash = shader.hash;
    const uint8_t cls = shader.cls;
    const bool layout_ok = shader.layout_ok;
    const std::string layout_error = shader.layout_error;
    const DeformSoLayout layout = shader.layout;
    const bool create_failed = shader.create_failed;
    const int32_t create_hr = shader.create_hr;
    shader = DeformShader{};
    shader.so_shader = so_shader;
    shader.code = code;
    shader.hash = hash;
    shader.cls = cls;
    shader.layout_ok = layout_ok;
    shader.layout_error = layout_error;
    shader.layout = layout;
    shader.create_failed = create_failed;
    shader.create_hr = create_hr;
  }
  g_deform.stats = {};
}

// ---------------------------------------------------------------------------
// Report.

struct DeformShaderRow {
  uint64_t pipeline = 0u;
  DeformShader shader;
};

// Rows by hash (pipelines sharing a bytecode are merged into the busiest one).
inline std::vector<DeformShaderRow> SnapshotDeformShaders(DeformStats* stats) {
  std::vector<DeformShaderRow> rows;
  std::lock_guard<std::mutex> lock(g_deform.mutex);
  if (stats != nullptr) *stats = g_deform.stats;
  rows.reserve(g_deform.shaders.size());
  for (const auto& [pipeline, shader] : g_deform.shaders) {
    DeformShaderRow row;
    row.pipeline = pipeline;
    row.shader = shader;
    row.shader.code = nullptr;  // the bytecode is not needed for display
    rows.push_back(std::move(row));
  }
  std::sort(rows.begin(), rows.end(), [](const DeformShaderRow& a, const DeformShaderRow& b) {
    if (a.shader.cls != b.shader.cls) return a.shader.cls < b.shader.cls;
    if (a.shader.hash != b.shader.hash) return a.shader.hash < b.shader.hash;
    return a.pipeline < b.pipeline;
  });
  return rows;
}

inline std::string DeformChosenText(const DeformShader& shader) {
  if (shader.chosen < 0 || static_cast<size_t>(shader.chosen) >= shader.layout.elements.size()) return "-";
  return DeformElementName(shader.layout.elements[static_cast<size_t>(shader.chosen)]);
}

inline void DumpDeformProbe() {
  DeformStats stats;
  const std::vector<DeformShaderRow> rows = SnapshotDeformShaders(&stats);
  std::ostringstream out;
  out << "{\n  \"schema\": 2,\n  \"generated_frame\": " << g_state.frame.load()
      << ",\n  \"enabled\": " << (g_deform.enabled.load() ? "true" : "false")
      << ",\n  \"stats\": {\"probes\": " << stats.probes << ", \"reads\": " << stats.reads
      << ", \"matched\": " << stats.matched << ", \"shaders_created\": " << stats.shaders_created
      << ", \"shader_failures\": " << stats.shader_failures << ", \"resource_failures\": " << stats.resource_failures
      << ", \"map_failures\": " << stats.map_failures << ", \"skips\": {";
  bool first = true;
  for (size_t i = 1; i < stats.skips.size(); ++i) {
    if (stats.skips[i] == 0u) continue;
    out << (first ? "" : ", ") << "\"" << DeformSkipName(static_cast<DeformSkip>(i)) << "\": " << stats.skips[i];
    first = false;
  }
  out << "}, \"read_fails\": {";
  first = true;
  for (size_t i = 1; i < stats.read_fails.size(); ++i) {
    if (stats.read_fails[i] == 0u) continue;
    out << (first ? "" : ", ") << "\"" << DeformReadFailName(static_cast<DeformReadFail>(i)) << "\": "
        << stats.read_fails[i];
    first = false;
  }
  out << "}},\n  \"shaders\": [";
  for (size_t r = 0; r < rows.size(); ++r) {
    const DeformShader& shader = rows[r].shader;
    out << (r == 0u ? "" : ",") << "\n    {\"hash\": \"" << PoolHashText(shader.hash) << "\", \"class\": \""
        << contract::VsClassName(static_cast<contract::VsClass>(shader.cls)) << "\", \"pipeline\": " << rows[r].pipeline
        << ", \"layout\": \"" << (shader.layout_ok ? "ok" : shader.layout_error) << "\", \"stride\": " << shader.layout.stride
        << ", \"create_hr\": " << shader.create_hr << ", \"draws\": " << shader.draws << ", \"probes\": " << shader.probes
        << ", \"reads\": " << shader.reads << ", \"matched\": " << shader.matched << ", \"world_output\": \""
        << DeformChosenText(shader) << "\", \"convention\": " << static_cast<uint32_t>(shader.convention)
        << ", \"inconsistent\": " << (shader.inconsistent ? "true" : "false")
        << ", \"worst_error\": " << PoolJsonFloat{shader.worst_error} << ", \"last_index_count\": " << shader.last_index_count
        << ", \"last_instance_count\": " << shader.last_instance_count << ", \"indirect_probes\": " << shader.indirect_probes
        << ", \"last_indirect\": " << (shader.last_indirect ? "true" : "false") << ", \"last\": {\"vertices\": "
        << shader.last.vertices << ", \"triangles\": " << shader.last.triangles << ", \"degenerate\": "
        << shader.last.degenerate << ", \"nonfinite\": " << shader.last.nonfinite << ", \"bbox_min\": ["
        << PoolJsonFloat{shader.last.bbox_min[0]} << ", " << PoolJsonFloat{shader.last.bbox_min[1]} << ", "
        << PoolJsonFloat{shader.last.bbox_min[2]} << "], \"bbox_max\": [" << PoolJsonFloat{shader.last.bbox_max[0]} << ", "
        << PoolJsonFloat{shader.last.bbox_max[1]} << ", " << PoolJsonFloat{shader.last.bbox_max[2]} << "], \"outputs\": [";
    for (size_t e = 0; e < shader.layout.elements.size(); ++e) {
      const DeformSoElement& element = shader.layout.elements[e];
      out << (e == 0u ? "" : ", ") << "{\"name\": \"" << DeformElementName(element) << "\"";
      if (element.position) out << ", \"position\": true";
      if (element.candidate && e < shader.last.candidates.size()) {
        out << ", \"error\": [" << PoolJsonFloat{shader.last.candidates[e].error[0]} << ", "
            << PoolJsonFloat{shader.last.candidates[e].error[1]} << "], \"w_error\": "
            << PoolJsonFloat{shader.last.candidates[e].w_error};
      }
      out << "}";
    }
    out << "]}, \"skips\": {";
    first = true;
    for (size_t i = 1; i < shader.skips.size(); ++i) {
      if (shader.skips[i] == 0u) continue;
      out << (first ? "" : ", ") << "\"" << DeformSkipName(static_cast<DeformSkip>(i)) << "\": " << shader.skips[i];
      first = false;
    }
    out << "}, \"read_fails\": {";
    first = true;
    for (size_t i = 1; i < shader.read_fails.size(); ++i) {
      if (shader.read_fails[i] == 0u) continue;
      out << (first ? "" : ", ") << "\"" << DeformReadFailName(static_cast<DeformReadFail>(i)) << "\": "
          << shader.read_fails[i];
      first = false;
    }
    out << "}}";
  }
  out << "\n  ]\n}\n";
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(PoolOutputDir() / "world_deform.json", text);
}

}  // namespace falcom_world::bvh
