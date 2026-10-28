#pragma once

// M3 DXR backend: RTAO pass A on a side D3D12 device (DXR 1.1 inline ray queries) against a D3D12 mirror of the live BVH
// store. Included by rtao.hpp (outside the harness), so everything here runs on the render thread (single-threaded
// D3D11). Lock rule: no graphics call while a module mutex is held. Sync is renodx::utils::gpu_fence only; no CPU wait
// on the GPU per frame (TeardownRt12 and the stuck check are the only waits, both bounded).

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <d3d11_4.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <include/reshade.hpp>

#include "../../../../utils/directx.hpp"
#include "../../../../utils/gpu_fence.hpp"
#include "../../../../utils/log.hpp"
#include "../../../../utils/path.hpp"
#include "../bvh/bvh_live.hpp"
#include "../rtao/rtao_resources.hpp"
#include "rt12_mirror.hpp"

namespace falcom_world::rt12 {

static_assert(bvh::kInstanceCameraVisible == 2u && bvh::kInstanceFoliage == 8u && bvh::kInstanceAlphaTested == 16u);
static_assert(kAlphaAtlasSlices == kRt12AlphaSlices);
static_assert(sizeof(AlphaMaterialGPU) == 32u);
static_assert(sizeof(bvh::WorldInstanceGPU) == 192u);
static_assert(sizeof(bvh::WorldMeshGPU) == 64u);
static_assert(kRt12MaskDynamic == 8u);
static_assert(static_cast<uint64_t>(kRt12CarrierWidth) * kRt12CarrierHeight * 12u >= bvh::kDynamicArenaBytes);
static_assert(static_cast<uint64_t>(kRt12CarrierWidth) * kRt12RecordCarrierHeight >= static_cast<uint64_t>(bvh::kPoolDynamicInstances) * kRt12RecordTexels);
static_assert(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) == 64u);
static_assert(static_cast<uint64_t>(kRt12CarrierWidth) * kRt12GcRecordCarrierHeight >= static_cast<uint64_t>(bvh::kGcSetCapacity + bvh::kGcMovingRecords) * kRt12RecordTexels);

inline constexpr uint32_t kRt12Slots = 3u;              // allocators per submission kind (frames in flight)
inline constexpr uint32_t kRt12StuckMs = 2000u;         // no fence progress this long with work pending: stuck
inline constexpr uint32_t kRt12TeardownMs = 2000u;      // CPU wait budget for a teardown
// Shader-visible descriptors per lighting slot (Step 10.5); 14 carrier SRV and 15 dynamic vertices UAV (13.1b); 16..19
// records SRV and descs / instance table UAV (13.1c).
inline constexpr uint32_t kRt12Descriptors = 20u;
inline constexpr uint32_t kRt12Timestamps = 7u;         // per slot: present 0..1, lighting 2..6
inline constexpr uint32_t kRt12StatsWidth = 32u;
inline constexpr uint64_t kRt12ArenaMinBytes = 1024ull * 1024ull;

enum class Rt12Reason : uint32_t {
  Off, NotCreated, CreateFailed, NoDxr, FenceError, Stuck, Discovery, Busy, ContextMismatch,
  TlasEmpty, AlphaMirror, TargetFailed, SubmitFailed, Characters, Records, Tracing, GpuCapture,
};
inline constexpr uint32_t kRt12ReasonCount = 17u;

inline const char* Rt12ReasonName(Rt12Reason reason) {
  switch (reason) {
    case Rt12Reason::Off: return "off";
    case Rt12Reason::NotCreated: return "not_created";
    case Rt12Reason::CreateFailed: return "create_failed";
    case Rt12Reason::NoDxr: return "no_dxr";
    case Rt12Reason::FenceError: return "fence_error";
    case Rt12Reason::Stuck: return "stuck";
    case Rt12Reason::Discovery: return "discovery";
    case Rt12Reason::Busy: return "busy";
    case Rt12Reason::ContextMismatch: return "context_mismatch";
    case Rt12Reason::TlasEmpty: return "tlas_empty";
    case Rt12Reason::AlphaMirror: return "alpha_mirror";
    case Rt12Reason::TargetFailed: return "target_failed";
    case Rt12Reason::SubmitFailed: return "submit_failed";
    case Rt12Reason::Characters: return "characters";
    case Rt12Reason::Records: return "records";
    case Rt12Reason::Tracing: return "tracing";
    case Rt12Reason::GpuCapture: return "gpu_capture";
  }
  return "unknown";
}

struct Rt12Shared {  // a D3D11-created shared texture opened in D3D12
  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture11;
  Microsoft::WRL::ComPtr<ID3D12Resource> texture12;
  reshade::api::resource_view srv11 = {0u};
  reshade::api::resource_view uav11 = {0u};
  uint32_t width = 0u;
  uint32_t height = 0u;
};

struct Rt12Arena {
  Microsoft::WRL::ComPtr<ID3D12Resource> buffer;  // DEFAULT heap; rest state COMMON (see "Buffer states" in 10.1)
  uint64_t capacity = 0u;  // elements
  uint64_t used = 0u;      // elements written (highest first_element + count seen)
};

struct Rt12Slot {
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
  uint64_t fence_value = 0u;  // 0 = never submitted
  Microsoft::WRL::ComPtr<ID3D12Resource> upload;  // UPLOAD heap, persistently mapped
  uint8_t* upload_data = nullptr;
  uint64_t upload_bytes = 0u;
  // Lighting slots only: this slot's TLAS, its scratch, its instance table (DEFAULT) and their sizes.
  Microsoft::WRL::ComPtr<ID3D12Resource> tlas, tlas_scratch, instance_table;
  uint64_t tlas_bytes = 0u, scratch_bytes = 0u, table_bytes = 0u;
  uint32_t instance_count = 0u;  // entries in instance_table
  bool timestamps = false;       // queries resolved into this slot's readback range
  bool dynamic_timed = false;    // timestamps 2..3 hold a character BLAS build (this frame had characters)
  Microsoft::WRL::ComPtr<ID3D12Resource> dynamic_scratch;  // character BLAS scratch: one range per character
  uint64_t dynamic_scratch_bytes = 0u;
  uint64_t table_key_version = ~0ull, table_key_blas = ~0ull;  // static keys (Rt12State) of instance_table's content
  Microsoft::WRL::ComPtr<ID3D12Resource> descs;      // 13.1c: the TLAS instance descs of a records frame (DEFAULT, UAV)
  uint64_t descs_bytes = 0u;
  Microsoft::WRL::ComPtr<ID3D12Resource> addresses;  // 13.1c: one uint64 BLAS address per live store slot (DEFAULT)
  uint64_t addresses_bytes = 0u;
  uint64_t addresses_generation = ~0ull;  // blas_generation the addresses were written for
  bool records_timed = false;             // timestamps 2..6 hold a records frame (13.1c)
  Microsoft::WRL::ComPtr<ID3D12Resource> gc_meshes, gc_addresses;  // G4 (D8): GPU-cache mesh and BLAS address tables (DEFAULT, COMMON)
  uint64_t gc_version = ~0ull;  // Rt12GcScene::version they hold
};

struct Rt12Blas {
  Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
  uint64_t bytes = 0u;
};

// The BLAS of one character (deform identity key), refit in place and rebuilt on the cadence of 13.1b.
struct Rt12DynamicBlas {
  Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
  uint64_t bytes = 0u;
  uint64_t build_scratch = 0u;   // ScratchDataSizeInBytes (256-aligned)
  uint64_t update_scratch = 0u;  // UpdateScratchDataSizeInBytes (256-aligned)
  uint32_t triangles = 0u;
  uint32_t built_frame = 0u;     // g_state.frame of the last full build
  uint32_t last_frame = 0u;      // g_state.frame of the last build or refit
};

struct Rt12Retired {
  uint64_t fence_value = 0u;
  Microsoft::WRL::ComPtr<IUnknown> object;
};

struct Rt12State {
  reshade::api::device* device = nullptr;  // the D3D11 device this state belongs to
  Microsoft::WRL::ComPtr<ID3D11Device> device11;
  Microsoft::WRL::ComPtr<ID3D11Device1> device11_1;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> ctx11;
  Microsoft::WRL::ComPtr<ID3D12Device> wrapper;
  Microsoft::WRL::ComPtr<ID3D12Device5> device12;
  bool wrapped = false;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue12;
  std::unique_ptr<renodx::utils::gpu_fence::Fence> fence11;  // D3D11 signals, D3D12 waits
  std::unique_ptr<renodx::utils::gpu_fence::Fence> fence12;  // D3D12 signals, D3D11 waits and the CPU polls
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> present_list, lighting_list;
  Rt12Slot present_slots[kRt12Slots];
  Rt12Slot lighting_slots[kRt12Slots];
  uint32_t present_next = 0u, lighting_next = 0u;
  int32_t scene_slot = -1;  // lighting slot whose TLAS and instance table are current (-1 none)
  uint64_t scene_key_version = ~0ull, scene_key_blas = ~0ull;  // tlas_cpu_version and blas_generation of that TLAS
  uint64_t last_signal = 0u;
  LARGE_INTEGER last_signal_qpc = {};
  Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;      // shader visible, kRt12Slots * kRt12Descriptors
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpu_heap;  // not shader visible, kRt12Slots (stats UAV for the clear)
  uint32_t descriptor_size = 0u;
  Microsoft::WRL::ComPtr<ID3D12QueryHeap> queries;        // kRt12Slots * kRt12Timestamps
  Microsoft::WRL::ComPtr<ID3D12Resource> query_readback;  // kRt12Slots * kRt12Timestamps * 8 bytes
  uint64_t timestamp_frequency = 0u;
  Rt12Arena arenas[3];  // kRt12Arena*
  Microsoft::WRL::ComPtr<ID3D12Resource> mesh_table;  // DEFAULT, data->mesh_descriptors bytes
  uint64_t mesh_table_bytes = 0u;
  uint32_t mesh_count = 0u;
  uint64_t mesh_store_version = ~0ull;
  Microsoft::WRL::ComPtr<ID3D12Resource> materials;  // DEFAULT, kRt12AlphaSlices * 32 bytes
  std::array<std::array<uint8_t, 32>, kRt12AlphaSlices> material_cpu = {};
  bool materials_dirty = true;
  uint64_t materials_version_seen = ~0ull;  // g_rt12_mirror.materials_version last copied (~0: copy at the first present)
  std::vector<Rt12Blas> blas;  // index = live store slot
  uint64_t blas_generation = 0u;
  uint64_t blas_bytes = 0u;
  Rt12Shared depth, normal, scene, noise, texel, raw, stats, atlas, carrier, records, gc_records;
  Microsoft::WRL::ComPtr<ID3D12Resource> dynamic_vertices;  // DEFAULT, kRt12CarrierWidth * kRt12CarrierHeight float4, COMMON
  Microsoft::WRL::ComPtr<ID3D12RootSignature> unpack_root;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> unpack_pso;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> records_root;  // 13.1c: b0 constants, records SRV t0..t1, UAV u0..u1
  Microsoft::WRL::ComPtr<ID3D12PipelineState> records_pso;
  std::unordered_map<uint64_t, Rt12DynamicBlas> dynamic_blas;  // by deform identity key
  uint32_t scene_dynamic_count = 0u;  // character instances in the TLAS of scene_slot
  uint32_t scene_record_count = 0u;   // P2b records in the TLAS of scene_slot
  uint64_t address_generation = ~0ull;  // blas_generation the per-slot address tables were last written for (see Rt12Slot)
  std::vector<D3D12_RAYTRACING_INSTANCE_DESC> static_descs;  // the static part of the TLAS, cached by its key
  uint64_t static_key_version = ~0ull, static_key_blas = ~0ull;
  reshade::api::pipeline pack_pipeline = {0u}, noise_pipeline = {0u}, resolve_pipeline = {0u}, carrier_pipeline = {0u},
                         records_carrier_pipeline = {0u};
  uint64_t noise_source = 0u;          // isfast view handle last copied into `noise`
  reshade::api::resource atlas_source = {0u};
  uint64_t atlas_blits = ~0ull;        // data->alpha.blits at the last atlas copy
  std::vector<Rt12Retired> retired;
  bool mirror_dropped = false;  // G4 (D2): the mirror's BLAS and arenas were dropped while the GPU scene traces
  bool stuck = false;
  bool failed = false;
};

inline std::atomic<bool> g_rt12_enabled{true};  // the panel switch (session only, not saved)
inline std::unique_ptr<Rt12State> g_rt12;        // one D3D11 device in this game

struct Rt12Panel {  // copied under g_rt12_panel_mutex for the panel and the JSON; written on the render thread
  Rt12Reason reason = Rt12Reason::Off;
  std::string failed_stage;
  HRESULT hr = S_OK;
  bool wrapped = false, stuck = false, leaked = false, alpha_mirror = false;
  uint64_t traced_frames = 0u, fallback_frames[kRt12ReasonCount] = {}, busy_frames = 0u;
  uint64_t present_submits = 0u, lighting_submits = 0u, present_skipped_busy = 0u;
  uint32_t blas_count = 0u, mesh_count = 0u, tlas_instances = 0u, tlas_no_blas = 0u, tlas_holes = 0u, tlas_bad_mesh = 0u;
  uint64_t blas_bytes = 0u, arena_bytes[3] = {}, tlas_bytes = 0u, sanitized_triangles = 0u;
  uint64_t atlas_copies = 0u, noise_copies = 0u;
  float blas_ms = -1.f, tlas_ms = -1.f, trace_ms = -1.f;  // last resolved D3D12 timestamps
  float blas_ms_sum = 0.f, tlas_ms_sum = 0.f, trace_ms_sum = 0.f;
  uint32_t blas_ms_n = 0u, tlas_ms_n = 0u, trace_ms_n = 0u;
  uint32_t dynamic_objects = 0u, dynamic_blas_count = 0u;  // characters of the last traced frame, character BLAS count
  uint64_t dynamic_vertices = 0u, dynamic_builds = 0u, dynamic_refits = 0u, dynamic_blas_bytes = 0u;
  float dynamic_ms = -1.f, dynamic_ms_sum = 0.f;
  uint32_t dynamic_ms_n = 0u;
  uint32_t records = 0u;  // P2b records of the last traced frame (13.1c)
  float records_ms = -1.f, records_ms_sum = 0.f;
  uint32_t records_ms_n = 0u;
  uint64_t gc_traced_frames = 0u;
  uint32_t gc_records = 0u;  // bound of the last GC frame
  std::string gc_not_ready;  // last gpu_capture sub-reason ("" after a traced GC frame)
};
inline std::mutex g_rt12_panel_mutex;
inline Rt12Panel g_rt12_panel;

// G4 (D7): the GPU-cache scene, published by rt12_capture.hpp OnGcPresent 12b and read by RunRtaoPassA (render thread).
// Raw pointers, valid until the next OnGcPresent: the cache state owns the objects.
struct Rt12GcScene {
  bool ready = false;
  const char* not_ready = "off";
  ID3D12Resource* indices = nullptr;  // GC arenas, rest state COMMON
  ID3D12Resource* uvs = nullptr;
  uint64_t index_count = 0u, uv_count = 0u;
  std::vector<float> meshes;        // bvh::kGcMaxEntries * 16 (Rt12GcMeshWords per slot)
  std::vector<uint64_t> addresses;  // bvh::kGcMaxEntries BLAS addresses, 0 none
  uint64_t version = 0u;
  reshade::api::resource_view records_srv = {0u};
  uint32_t bound = 0u;
};
inline Rt12GcScene g_rt12_gc_scene;
inline uint64_t g_rt12_gc_epoch = 0u;  // last Rt12GcScene::version handed out; never reset, so versions never repeat

inline bool GcOwnsRtao() {
  return bvh::g_gpu_capture.enabled.load(std::memory_order_relaxed) && bvh::g_gpu_capture.trace.load(std::memory_order_relaxed);
}

template <typename F>
inline void UpdatePanel(F&& update) {
  std::lock_guard lock(g_rt12_panel_mutex);
  update(g_rt12_panel);
}

// Counts a frame that falls back to the compute pass A, with its reason.
inline bool RtFallback(Rt12Reason reason) {
  UpdatePanel([reason](Rt12Panel& p) {
    p.reason = reason;
    p.fallback_frames[static_cast<uint32_t>(reason)] += 1u;
  });
  return false;
}

// Stops the mirror feed (every hook returns at once) and drops what it queued; the materials copy stays.
inline void DisableMirror() {
  std::lock_guard lock(g_rt12_mirror.mutex);
  g_rt12_mirror.active = false;
  g_rt12_mirror.reset = false;
  g_rt12_mirror.appends.clear();
  g_rt12_mirror.new_slots.clear();
  g_rt12_mirror.retired_slots.clear();
}

// A failure that ends the DXR path for the session (switch OFF and ON again recreates it).
inline void FailSession(Rt12State& s, Rt12Reason reason, const char* stage, HRESULT hr = E_FAIL) {
  s.failed = true;
  DisableMirror();
  renodx::utils::log::w("[world-rt12] failed at ", stage, " hr=", renodx::utils::log::AsHex(static_cast<uint32_t>(hr)));
  UpdatePanel([reason, stage, hr](Rt12Panel& p) {
    p.reason = reason;
    p.failed_stage = stage;
    p.hr = hr;
  });
}

inline uint64_t Align256(uint64_t value) { return (value + 255u) & ~uint64_t{255u}; }

inline LARGE_INTEGER QpcNow() {
  LARGE_INTEGER now = {};
  QueryPerformanceCounter(&now);
  return now;
}

inline double QpcMsSince(const LARGE_INTEGER& from) {
  static const LARGE_INTEGER frequency = [] {
    LARGE_INTEGER value = {};
    QueryPerformanceFrequency(&value);
    return value;
  }();
  const LARGE_INTEGER now = QpcNow();
  return static_cast<double>(now.QuadPart - from.QuadPart) * 1000.0 / static_cast<double>(frequency.QuadPart);
}

inline float TicksToMs(uint64_t ticks, uint64_t frequency) {
  if (frequency == 0u) return -1.f;
  return static_cast<float>(static_cast<double>(ticks) * 1000.0 / static_cast<double>(frequency));
}

inline HRESULT CreateRt12Buffer(ID3D12Device* device, D3D12_HEAP_TYPE heap_type, uint64_t bytes, D3D12_RESOURCE_STATES state,
                                D3D12_RESOURCE_FLAGS flags, Microsoft::WRL::ComPtr<ID3D12Resource>& out) {
  D3D12_HEAP_PROPERTIES heap = {};
  heap.Type = heap_type;
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = bytes;
  desc.Height = 1u;
  desc.DepthOrArraySize = 1u;
  desc.MipLevels = 1u;
  desc.SampleDesc.Count = 1u;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags = flags;
  return device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(out.ReleaseAndGetAddressOf()));
}

inline void Transition(std::vector<D3D12_RESOURCE_BARRIER>& barriers, ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
                       D3D12_RESOURCE_STATES after) {
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = resource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = before;
  barrier.Transition.StateAfter = after;
  barriers.push_back(barrier);
}

inline void UavBarrier(std::vector<D3D12_RESOURCE_BARRIER>& barriers, ID3D12Resource* resource) {
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.UAV.pResource = resource;
  barriers.push_back(barrier);
}

// Keeps a replaced GPU object alive until `fence` has completed (the caller assigns the fence once it is known).
template <typename T>
inline void RetireObject(Rt12State& s, uint64_t fence, const Microsoft::WRL::ComPtr<T>& object) {
  if (object.Get() == nullptr) return;
  Rt12Retired retired;
  retired.fence_value = fence;
  retired.object = object;
  s.retired.push_back(std::move(retired));
}

// One shared texture (both APIs) with its ReShade views; the old objects are kept until the last signal completes.
inline void RetireShared(Rt12State& s, Rt12Shared& shared, uint64_t fence) {
  if (shared.srv11.handle != 0u) s.device->destroy_resource_view(shared.srv11);
  if (shared.uav11.handle != 0u) s.device->destroy_resource_view(shared.uav11);
  RetireObject(s, fence, shared.texture11);
  RetireObject(s, fence, shared.texture12);
  shared = Rt12Shared{};
}

// A D3D11-created shared texture opened in D3D12 (10.2). Stops at the first failure.
inline HRESULT CreateRt12Shared(Rt12State& state, reshade::api::device* device, DXGI_FORMAT format, uint32_t width, uint32_t height,
                                uint32_t array_size, bool uav, Rt12Shared* out) {
  using Microsoft::WRL::ComPtr;
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1u;
  desc.ArraySize = array_size;
  desc.Format = format;
  desc.SampleDesc.Count = 1u;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (uav ? D3D11_BIND_UNORDERED_ACCESS : 0u);
  desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
  HRESULT hr = state.device11->CreateTexture2D(&desc, nullptr, &out->texture11);
  if (FAILED(hr)) return hr;

  ComPtr<IDXGIResource1> dxgi_resource;
  hr = out->texture11.As(&dxgi_resource);
  if (FAILED(hr)) return hr;
  HANDLE handle = nullptr;
  hr = dxgi_resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &handle);
  if (FAILED(hr)) return hr;
  hr = state.device12->OpenSharedHandle(handle, IID_PPV_ARGS(&out->texture12));
  CloseHandle(handle);
  if (FAILED(hr)) return hr;

  // ReShade views: the enum values of reshade::api::format are the DXGI_FORMAT values for these formats.
  const auto view_format = static_cast<reshade::api::format>(format);
  const auto view_type = array_size > 1u ? reshade::api::resource_view_type::texture_2d_array
                                         : reshade::api::resource_view_type::texture_2d;
  const reshade::api::resource resource = {reinterpret_cast<uint64_t>(out->texture11.Get())};
  if (!device->create_resource_view(resource, reshade::api::resource_usage::shader_resource,
                                    reshade::api::resource_view_desc(view_type, view_format, 0u, 1u, 0u, array_size),
                                    &out->srv11)) {
    return E_FAIL;
  }
  if (uav && !device->create_resource_view(resource, reshade::api::resource_usage::unordered_access,
                                           reshade::api::resource_view_desc(view_type, view_format, 0u, 1u, 0u, array_size),
                                           &out->uav11)) {
    return E_FAIL;
  }
  out->width = width;
  out->height = height;
  return S_OK;
}

// D3D11 compute pipelines of the pass A helpers (pack, noise, resolve), on RTAO's pipeline layout.
inline bool CreateRt12Pipelines(reshade::api::device* device, reshade::api::pipeline_layout layout, Rt12State& s) {
  const auto make = [&](auto& code, reshade::api::pipeline* out) {
    reshade::api::shader_desc shader = {};
    shader.code = code.data();
    shader.code_size = code.size();
    shader.entry_point = "main";
    reshade::api::pipeline_subobject subobject = {reshade::api::pipeline_subobject_type::compute_shader, 1, &shader};
    return device->create_pipeline(layout, 1, &subobject, out);
  };
  if (s.pack_pipeline.handle == 0u) {
#if defined(__world_rt12_pack_EMBED_FILE)
    if (!make(__world_rt12_pack, &s.pack_pipeline)) return false;
#else
    return false;
#endif
  }
  if (s.noise_pipeline.handle == 0u) {
#if defined(__world_rt12_noise_EMBED_FILE)
    if (!make(__world_rt12_noise, &s.noise_pipeline)) return false;
#else
    return false;
#endif
  }
  if (s.resolve_pipeline.handle == 0u) {
#if defined(__world_rt12_resolve_EMBED_FILE)
    if (!make(__world_rt12_resolve, &s.resolve_pipeline)) return false;
#else
    return false;
#endif
  }
  if (s.carrier_pipeline.handle == 0u) {
#if defined(__world_rt12_carrier_EMBED_FILE)
    if (!make(__world_rt12_carrier, &s.carrier_pipeline)) return false;
#else
    return false;
#endif
  }
  if (s.records_carrier_pipeline.handle == 0u) {
#if defined(__world_rt12_records_carrier_EMBED_FILE)
    if (!make(__world_rt12_records_carrier, &s.records_carrier_pipeline)) return false;
#else
    return false;
#endif
  }
  return true;
}

inline void DestroyRt12Pipelines(reshade::api::device* device, Rt12State& s) {
  if (s.pack_pipeline.handle != 0u) device->destroy_pipeline(s.pack_pipeline);
  if (s.noise_pipeline.handle != 0u) device->destroy_pipeline(s.noise_pipeline);
  if (s.resolve_pipeline.handle != 0u) device->destroy_pipeline(s.resolve_pipeline);
  if (s.carrier_pipeline.handle != 0u) device->destroy_pipeline(s.carrier_pipeline);
  if (s.records_carrier_pipeline.handle != 0u) device->destroy_pipeline(s.records_carrier_pipeline);
  s.pack_pipeline = {0u};
  s.noise_pipeline = {0u};
  s.resolve_pipeline = {0u};
  s.carrier_pipeline = {0u};
  s.records_carrier_pipeline = {0u};
}

// Stops the DXR path. A stuck scene is leaked on purpose (the GPU may still touch it).
inline void TeardownRt12(const char* why) {
  if (!g_rt12) return;
  g_rt12_gc_scene = {};
  {
    std::lock_guard lock(g_rt12_mirror.mutex);
    g_rt12_mirror.active = false;
    g_rt12_mirror.reset = false;
    g_rt12_mirror.appends.clear();
    g_rt12_mirror.new_slots.clear();
    g_rt12_mirror.retired_slots.clear();
  }
  Rt12State& s = *g_rt12;
  if (!s.stuck && s.last_signal != 0u && s.fence12) {
    const LARGE_INTEGER start = QpcNow();
    while (s.fence12->GetCompletedValue() < s.last_signal && QpcMsSince(start) < static_cast<double>(kRt12TeardownMs)) {
      Sleep(1);
    }
    if (s.fence12->GetCompletedValue() < s.last_signal) s.stuck = true;
  }
  if (s.stuck) {
    renodx::utils::log::w("[world-rt12] teardown (", why, "): GPU work still pending, objects leaked on purpose");
    UpdatePanel([](Rt12Panel& p) { p.leaked = true; });
    g_rt12.release();  // leaked on purpose (see above)
    return;
  }
  for (Rt12Shared* shared : {&s.depth, &s.normal, &s.scene, &s.noise, &s.texel, &s.raw, &s.stats, &s.atlas, &s.carrier, &s.records, &s.gc_records}) {
    if (shared->srv11.handle != 0u) s.device->destroy_resource_view(shared->srv11);
    if (shared->uav11.handle != 0u) s.device->destroy_resource_view(shared->uav11);
  }
  DestroyRt12Pipelines(s.device, s);
  renodx::utils::log::i("[world-rt12] teardown (", why, ")");
  g_rt12.reset();
}

inline void DestroyRt12(reshade::api::device* device) {
  if (g_rt12 && g_rt12->device == device) TeardownRt12("device destroyed");
}

// Creates the D3D12 side (10.3). Returns false and records the reason in the panel when it cannot run.
inline bool CreateRt12(reshade::api::device* device) {
#if defined(_WIN32)
  using Microsoft::WRL::ComPtr;
  if (device->get_api() != reshade::api::device_api::d3d11) {
    UpdatePanel([](Rt12Panel& p) { p.reason = Rt12Reason::NotCreated; });
    return false;
  }
  g_rt12 = std::make_unique<Rt12State>();
  Rt12State& s = *g_rt12;
  s.device = device;
  const auto fail = [&](Rt12Reason reason, const char* stage, HRESULT hr) {
    s.failed = true;
    renodx::utils::log::w("[world-rt12] create failed at ", stage, " hr=", renodx::utils::log::AsHex(static_cast<uint32_t>(hr)));
    UpdatePanel([reason, stage, hr](Rt12Panel& p) {
      p.reason = reason;
      p.failed_stage = stage;
      p.hr = hr;
    });
    return false;
  };

  // 2. Native D3D11 device and immediate context (the game's own device, not the ReShade wrapper).
  auto* native11 = reinterpret_cast<ID3D11Device*>(device->get_native());  // NOLINT(performance-no-int-to-ptr)
  s.device11 = native11;
  HRESULT hr = s.device11.As(&s.device11_1);
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "ID3D11Device1", hr);
  s.device11->GetImmediateContext(s.ctx11.GetAddressOf());

  // 3. Adapter.
  ComPtr<IDXGIDevice> dxgi_device;
  ComPtr<IDXGIAdapter> adapter;
  hr = s.device11.As(&dxgi_device);
  if (SUCCEEDED(hr)) hr = dxgi_device->GetAdapter(&adapter);
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "adapter", hr);

  // 4. d3d12.dll from System32, loaded once and never freed.
  static HMODULE d3d12_module = nullptr;
  if (d3d12_module == nullptr) d3d12_module = LoadLibraryExW(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (d3d12_module == nullptr) return fail(Rt12Reason::CreateFailed, "LoadLibrary d3d12.dll", HRESULT_FROM_WIN32(GetLastError()));
  const auto create_device = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(d3d12_module, "D3D12CreateDevice"));
  const auto serialize = reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(
      GetProcAddress(d3d12_module, "D3D12SerializeRootSignature"));
  if (create_device == nullptr || serialize == nullptr) return fail(Rt12Reason::CreateFailed, "GetProcAddress D3D12", E_FAIL);

  // 5. Device on the OS runtime, unwrapped from the ReShade wrapper (the wrapper stays alive in the state).
  hr = create_device(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&s.wrapper));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "D3D12CreateDevice", hr);
  ID3D12Device* native12 = s.wrapper.Get();
  s.wrapped = renodx::utils::directx::NativeFromReShadeProxy(&native12);
  hr = native12->QueryInterface(IID_PPV_ARGS(&s.device12));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "ID3D12Device5", hr);

  // 6. DXR 1.1 (tier 1.1) and shader model 6.5.
  D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
  hr = s.device12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CheckFeatureSupport OPTIONS5", hr);
  D3D12_FEATURE_DATA_SHADER_MODEL shader_model = {D3D_SHADER_MODEL_6_5};
  const HRESULT sm_hr = s.device12->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shader_model, sizeof(shader_model));
  if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_1 || FAILED(sm_hr)) {
    s.failed = true;
    UpdatePanel([](Rt12Panel& p) { p.reason = Rt12Reason::NoDxr; });
    renodx::utils::log::w("[world-rt12] create failed: no DXR 1.1 or no shader model 6.5");
    return false;
  }

  // 7. Direct queue.
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  hr = s.device12->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&s.queue12));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateCommandQueue", hr);
  s.queue12->GetTimestampFrequency(&s.timestamp_frequency);

  // 8. Fences (gpu_fence: one created on each API).
  try {
    s.fence11 = std::make_unique<renodx::utils::gpu_fence::Fence>(s.device11.Get());
    s.fence12 = std::make_unique<renodx::utils::gpu_fence::Fence>(static_cast<ID3D12Device*>(s.device12.Get()));
  } catch (const std::exception& e) {
    s.failed = true;
    UpdatePanel([&e](Rt12Panel& p) {
      p.reason = Rt12Reason::FenceError;
      p.failed_stage = e.what();
    });
    renodx::utils::log::w("[world-rt12] create failed: fence: ", e.what());
    return false;
  }

  // 9. Allocators per slot; one list per kind, created on slot 0 and closed.
  for (uint32_t k = 0u; k < kRt12Slots; ++k) {
    hr = s.device12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s.present_slots[k].allocator));
    if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateCommandAllocator present", hr);
    hr = s.device12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s.lighting_slots[k].allocator));
    if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateCommandAllocator lighting", hr);
  }
  hr = s.device12->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, s.present_slots[0].allocator.Get(), nullptr,
                                     IID_PPV_ARGS(&s.present_list));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateCommandList present", hr);
  hr = s.present_list->Close();
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "Close present", hr);
  hr = s.device12->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, s.lighting_slots[0].allocator.Get(), nullptr,
                                     IID_PPV_ARGS(&s.lighting_list));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateCommandList lighting", hr);
  hr = s.lighting_list->Close();
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "Close lighting", hr);

  // 10. Root signature: b12 constants (24 values), one table of 14 descriptors in register order.
  D3D12_DESCRIPTOR_RANGE ranges[4] = {};
  const auto set_range = [](D3D12_DESCRIPTOR_RANGE& range, D3D12_DESCRIPTOR_RANGE_TYPE type, UINT count, UINT base) {
    range.RangeType = type;
    range.NumDescriptors = count;
    range.BaseShaderRegister = base;
    range.RegisterSpace = 0u;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
  };
  set_range(ranges[0], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3u, 1u);
  set_range(ranges[1], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 14u);
  set_range(ranges[2], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 20u);
  set_range(ranges[3], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3u, 0u);
  D3D12_ROOT_PARAMETER parameters[2] = {};
  parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  parameters[0].Constants.ShaderRegister = 12u;
  parameters[0].Constants.RegisterSpace = 0u;
  parameters[0].Constants.Num32BitValues = rtao::kRtaoPushConstantCount;
  parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[1].DescriptorTable.NumDescriptorRanges = 4u;
  parameters[1].DescriptorTable.pDescriptorRanges = ranges;
  parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  D3D12_ROOT_SIGNATURE_DESC root_desc = {};
  root_desc.NumParameters = 2u;
  root_desc.pParameters = parameters;
  ComPtr<ID3DBlob> blob;
  ComPtr<ID3DBlob> error;
  hr = serialize(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
  if (FAILED(hr)) {
    if (error.Get() != nullptr) {
      renodx::utils::log::w("[world-rt12] root signature: ",
                            std::string(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize()));
    }
    return fail(Rt12Reason::CreateFailed, "D3D12SerializeRootSignature", hr);
  }
  hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&s.root));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateRootSignature", hr);

  // 10b. Unpack root signature: b0 one value (the vertex count), one table of the carrier SRV t0 and the vertex UAV u0.
  D3D12_DESCRIPTOR_RANGE unpack_ranges[2] = {};
  set_range(unpack_ranges[0], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 0u);
  set_range(unpack_ranges[1], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u);
  D3D12_ROOT_PARAMETER unpack_parameters[2] = {};
  unpack_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  unpack_parameters[0].Constants.ShaderRegister = 0u;
  unpack_parameters[0].Constants.RegisterSpace = 0u;
  unpack_parameters[0].Constants.Num32BitValues = 1u;
  unpack_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  unpack_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  unpack_parameters[1].DescriptorTable.NumDescriptorRanges = 2u;
  unpack_parameters[1].DescriptorTable.pDescriptorRanges = unpack_ranges;
  unpack_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  D3D12_ROOT_SIGNATURE_DESC unpack_desc = {};
  unpack_desc.NumParameters = 2u;
  unpack_desc.pParameters = unpack_parameters;
  hr = serialize(&unpack_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "D3D12SerializeRootSignature unpack", hr);
  hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&s.unpack_root));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateRootSignature unpack", hr);

  // 11. Compute PSO from the DXR shader.
#if defined(__world_rt12_rtao_EMBED_FILE)
  D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
  pso_desc.pRootSignature = s.root.Get();
  pso_desc.CS.pShaderBytecode = __world_rt12_rtao.data();
  pso_desc.CS.BytecodeLength = __world_rt12_rtao.size();
  hr = s.device12->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&s.pso));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateComputePipelineState", hr);
#else
  return fail(Rt12Reason::CreateFailed, "shader not embedded", E_FAIL);
#endif

  // 11b. Compute PSO of the character vertex unpack (13.1b).
#if defined(__world_rt12_deform_unpack_EMBED_FILE)
  D3D12_COMPUTE_PIPELINE_STATE_DESC unpack_pso_desc = {};
  unpack_pso_desc.pRootSignature = s.unpack_root.Get();
  unpack_pso_desc.CS.pShaderBytecode = __world_rt12_deform_unpack.data();
  unpack_pso_desc.CS.BytecodeLength = __world_rt12_deform_unpack.size();
  hr = s.device12->CreateComputePipelineState(&unpack_pso_desc, IID_PPV_ARGS(&s.unpack_pso));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateComputePipelineState unpack", hr);
#else
  return fail(Rt12Reason::CreateFailed, "unpack shader not embedded", E_FAIL);
#endif

  // 11c. Records root signature (13.1c): b0 four values, one table of the records SRV t0..t1 and the descs and instance UAV u0..u1.
  D3D12_DESCRIPTOR_RANGE records_ranges[2] = {};
  set_range(records_ranges[0], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2u, 0u);
  set_range(records_ranges[1], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2u, 0u);
  D3D12_ROOT_PARAMETER records_parameters[2] = {};
  records_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  records_parameters[0].Constants.ShaderRegister = 0u;
  records_parameters[0].Constants.RegisterSpace = 0u;
  records_parameters[0].Constants.Num32BitValues = 4u;
  records_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  records_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  records_parameters[1].DescriptorTable.NumDescriptorRanges = 2u;
  records_parameters[1].DescriptorTable.pDescriptorRanges = records_ranges;
  records_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  D3D12_ROOT_SIGNATURE_DESC records_desc = {};
  records_desc.NumParameters = 2u;
  records_desc.pParameters = records_parameters;
  hr = serialize(&records_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "D3D12SerializeRootSignature records", hr);
  hr = s.device12->CreateRootSignature(0u, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&s.records_root));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateRootSignature records", hr);
#if defined(__world_rt12_records_EMBED_FILE)
  D3D12_COMPUTE_PIPELINE_STATE_DESC records_pso_desc = {};
  records_pso_desc.pRootSignature = s.records_root.Get();
  records_pso_desc.CS.pShaderBytecode = __world_rt12_records.data();
  records_pso_desc.CS.BytecodeLength = __world_rt12_records.size();
  hr = s.device12->CreateComputePipelineState(&records_pso_desc, IID_PPV_ARGS(&s.records_pso));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateComputePipelineState records", hr);
#else
  return fail(Rt12Reason::CreateFailed, "records shader not embedded", E_FAIL);
#endif

  // 12. Descriptor heaps.
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.NumDescriptors = kRt12Slots * kRt12Descriptors;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  hr = s.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&s.heap));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateDescriptorHeap", hr);
  heap_desc.NumDescriptors = kRt12Slots;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  hr = s.device12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&s.cpu_heap));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateDescriptorHeap cpu", hr);
  s.descriptor_size = s.device12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  // 13. Timestamp queries and their readback buffer.
  D3D12_QUERY_HEAP_DESC query_desc = {};
  query_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
  query_desc.Count = kRt12Slots * kRt12Timestamps;
  hr = s.device12->CreateQueryHeap(&query_desc, IID_PPV_ARGS(&s.queries));
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "CreateQueryHeap", hr);
  hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_READBACK, kRt12Slots * kRt12Timestamps * 8u,
                        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_NONE, s.query_readback);
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "readback", hr);

  // 14. Materials (DEFAULT, COMMON).
  hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, kRt12AlphaSlices * 32u, D3D12_RESOURCE_STATE_COMMON,
                        D3D12_RESOURCE_FLAG_NONE, s.materials);
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "materials", hr);
  s.materials_dirty = true;

  // 15. Fixed shared textures. An atlas failure leaves the alpha mirror off (RTAO falls back while alpha foliage is on).
  hr = CreateRt12Shared(s, device, DXGI_FORMAT_R32G32B32A32_UINT, 16u, 1u, 1u, true, &s.scene);
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "scene texture", hr);
  hr = CreateRt12Shared(s, device, DXGI_FORMAT_R32_UINT, kRt12StatsWidth, 1u, 1u, true, &s.stats);
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "stats texture", hr);
  hr = CreateRt12Shared(s, device, DXGI_FORMAT_R32G32B32A32_UINT, 128u, 4096u, 1u, true, &s.noise);
  if (FAILED(hr)) return fail(Rt12Reason::CreateFailed, "noise texture", hr);
  hr = CreateRt12Shared(s, device, DXGI_FORMAT_R32_UINT, 256u, 256u, kRt12AlphaSlices, false, &s.atlas);
  if (FAILED(hr)) {
    renodx::utils::log::w("[world-rt12] alpha mirror not created hr=", renodx::utils::log::AsHex(static_cast<uint32_t>(hr)));
    s.atlas = Rt12Shared{};
  }

  // 17. Success: the mirror starts, and the live store re-streams every mesh through its hooks.
  {
    std::lock_guard lock(g_rt12_mirror.mutex);
    g_rt12_mirror.active = true;
    g_rt12_mirror.reset = false;
    g_rt12_mirror.appends.clear();
    g_rt12_mirror.new_slots.clear();
    g_rt12_mirror.retired_slots.clear();
  }
  bvh::g_live_bvh.reset_requested.store(true, std::memory_order_relaxed);
  UpdatePanel([&s](Rt12Panel& p) {
    p.wrapped = s.wrapped;
    p.alpha_mirror = s.atlas.texture11 != nullptr;
  });
  renodx::utils::log::i("[world-rt12] created (wrapped ", s.wrapped ? "yes" : "no", ", tier ",
                        static_cast<int>(options5.RaytracingTier), ", queue timestamp frequency ", s.timestamp_frequency, ")");
  return true;
#else
  return false;
#endif
}

// Reads the timestamps of [first, first + count) of the readback buffer (only after their fence completed).
inline bool ReadRt12Timestamps(Rt12State& s, uint32_t first, uint32_t count, uint64_t* out) {
  void* mapped = nullptr;
  const D3D12_RANGE read = {first * 8u, (first + count) * 8u};
  if (FAILED(s.query_readback->Map(0u, &read, &mapped))) return false;
  std::memcpy(out, static_cast<const uint8_t*>(mapped) + first * 8u, count * 8u);
  const D3D12_RANGE nothing = {0u, 0u};
  s.query_readback->Unmap(0u, &nothing);
  return true;
}

// The present submission (9.1 to 9.9): arenas, mesh table, materials, BLAS builds. Returns false when the session failed.
inline bool SubmitPresent(Rt12State& s, const bvh::BvhDeviceData* data, const std::vector<Rt12MirrorAppend>& appends,
                          const std::vector<Rt12MirrorSlot>& new_slots, bool table_upload, bool materials_copy) {
  using Microsoft::WRL::ComPtr;
  const uint32_t k = s.present_next;
  Rt12Slot& p = s.present_slots[k];
  const size_t retired_begin = s.retired.size();

  // 9.1 Upload layout: appends, then the mesh table, then the materials (each 256-aligned).
  std::vector<uint64_t> append_offsets(appends.size());
  uint64_t upload_bytes = 0u;
  for (size_t i = 0u; i < appends.size(); ++i) {
    append_offsets[i] = upload_bytes;
    upload_bytes += Align256(appends[i].bytes.size());
  }
  const uint64_t table_offset = upload_bytes;
  const uint64_t table_bytes = table_upload ? static_cast<uint64_t>(s.mesh_count) * sizeof(bvh::WorldMeshGPU) : 0u;
  if (table_upload) upload_bytes += Align256(table_bytes);
  const uint64_t materials_offset = upload_bytes;
  if (materials_copy) upload_bytes += Align256(sizeof(s.material_cpu));
  if (p.upload == nullptr || p.upload_bytes < upload_bytes) {
    const uint64_t bytes = std::max<uint64_t>({upload_bytes, 2u * p.upload_bytes, 256u});
    ComPtr<ID3D12Resource> upload;
    HRESULT hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, bytes, D3D12_RESOURCE_STATE_GENERIC_READ,
                                  D3D12_RESOURCE_FLAG_NONE, upload);
    void* mapped = nullptr;
    if (SUCCEEDED(hr)) hr = upload->Map(0u, nullptr, &mapped);
    if (FAILED(hr)) {
      FailSession(s, Rt12Reason::SubmitFailed, "present upload", hr);
      return false;
    }
    RetireObject(s, p.fence_value, p.upload);
    p.upload = upload;
    p.upload_bytes = bytes;
    p.upload_data = static_cast<uint8_t*>(mapped);
  }
  for (size_t i = 0u; i < appends.size(); ++i) {
    std::memcpy(p.upload_data + append_offsets[i], appends[i].bytes.data(), appends[i].bytes.size());
  }
  if (table_upload && table_bytes > 0u) std::memcpy(p.upload_data + table_offset, data->mesh_descriptors.data(), table_bytes);
  if (materials_copy) std::memcpy(p.upload_data + materials_offset, s.material_cpu.data(), sizeof(s.material_cpu));

  // 9.2 Recording starts; timestamp at the start.
  if (FAILED(p.allocator->Reset()) || FAILED(s.present_list->Reset(p.allocator.Get(), nullptr))) {
    FailSession(s, Rt12Reason::SubmitFailed, "present reset");
    return false;
  }
  ID3D12GraphicsCommandList4* list = s.present_list.Get();
  list->EndQuery(s.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, k * kRt12Timestamps);

  // 9.3 Arena growth (copy of the used part into a new buffer), and the mesh table when it is too small.
  uint64_t needed[3] = {};
  bool written[3] = {};
  for (const Rt12MirrorAppend& append : appends) {
    const uint64_t count = append.bytes.size() / kRt12ArenaStride[append.arena];
    written[append.arena] = true;
    needed[append.arena] = std::max(needed[append.arena], append.first_element + count);
  }
  ComPtr<ID3D12Resource> old_arena[3];
  uint64_t old_used[3] = {};
  bool grown[3] = {};
  for (uint32_t a = 0u; a < 3u; ++a) {
    Rt12Arena& arena = s.arenas[a];
    const uint64_t stride = kRt12ArenaStride[a];
    if (needed[a] <= arena.capacity) continue;
    const uint64_t capacity = std::max({needed[a], 2u * arena.capacity, kRt12ArenaMinBytes / stride});
    ComPtr<ID3D12Resource> buffer;
    if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, capacity * stride, D3D12_RESOURCE_STATE_COMMON,
                                D3D12_RESOURCE_FLAG_NONE, buffer))) {
      FailSession(s, Rt12Reason::SubmitFailed, "arena growth");
      return false;
    }
    old_arena[a] = arena.buffer;
    old_used[a] = arena.used;
    RetireObject(s, s.last_signal, arena.buffer);
    arena.buffer = buffer;
    arena.capacity = capacity;
    grown[a] = true;
  }
  ComPtr<ID3D12Resource> old_table;
  if (table_upload && (s.mesh_table == nullptr || s.mesh_table_bytes < std::max<uint64_t>(table_bytes, 64u))) {
    const uint64_t bytes = std::max<uint64_t>(table_bytes, 64u);
    ComPtr<ID3D12Resource> table;
    if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, bytes, D3D12_RESOURCE_STATE_COMMON,
                                D3D12_RESOURCE_FLAG_NONE, table))) {
      FailSession(s, Rt12Reason::SubmitFailed, "mesh table growth");
      return false;
    }
    RetireObject(s, s.last_signal, s.mesh_table);
    s.mesh_table = table;
    s.mesh_table_bytes = bytes;
  }

  // 9.4 Barrier batch 1: COMMON to the copy states (the old arenas are only read).
  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  for (uint32_t a = 0u; a < 3u; ++a) {
    if (grown[a] && old_used[a] > 0u) Transition(barriers, old_arena[a].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
    if (written[a]) Transition(barriers, s.arenas[a].buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  }
  if (table_upload) Transition(barriers, s.mesh_table.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  if (materials_copy) Transition(barriers, s.materials.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
  if (!barriers.empty()) list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

  // 9.5 Copies: grown arenas, appends at the D3D11 element offsets, mesh table, materials.
  for (uint32_t a = 0u; a < 3u; ++a) {
    if (grown[a] && old_used[a] > 0u) {
      list->CopyBufferRegion(s.arenas[a].buffer.Get(), 0u, old_arena[a].Get(), 0u, old_used[a] * kRt12ArenaStride[a]);
    }
  }
  for (size_t i = 0u; i < appends.size(); ++i) {
    const Rt12MirrorAppend& append = appends[i];
    Rt12Arena& arena = s.arenas[append.arena];
    const uint64_t stride = kRt12ArenaStride[append.arena];
    list->CopyBufferRegion(arena.buffer.Get(), append.first_element * stride, p.upload.Get(), append_offsets[i], append.bytes.size());
    arena.used = std::max(arena.used, append.first_element + append.bytes.size() / stride);
  }
  if (table_upload && table_bytes > 0u) list->CopyBufferRegion(s.mesh_table.Get(), 0u, p.upload.Get(), table_offset, table_bytes);
  if (materials_copy) {
    list->CopyBufferRegion(s.materials.Get(), 0u, p.upload.Get(), materials_offset, sizeof(s.material_cpu));
    s.materials_dirty = false;
  }

  // 9.6 Barrier batch 2: BLAS inputs to NON_PIXEL_SHADER_RESOURCE, everything else back to COMMON.
  bool any_blas = false;
  for (const Rt12MirrorSlot& slot : new_slots) any_blas = any_blas || slot.index_count / 3u > 0u;
  barriers.clear();
  for (uint32_t a = 0u; a < 2u && any_blas; ++a) {  // vertices, indices (the builds read them)
    Transition(barriers, s.arenas[a].buffer.Get(), written[a] ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_COMMON,
               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  for (uint32_t a = 0u; a < 3u; ++a) {
    if (written[a] && !(any_blas && a < 2u)) Transition(barriers, s.arenas[a].buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
  }
  if (table_upload) Transition(barriers, s.mesh_table.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
  if (materials_copy) Transition(barriers, s.materials.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
  if (!barriers.empty()) list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

  // 9.8 One BLAS per new slot with triangles. Scratch is one buffer; each build has its own range.
  struct BlasBuild {
    uint32_t slot = 0u;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    uint64_t result_bytes = 0u;
    uint64_t scratch_bytes = 0u;
    uint64_t scratch_offset = 0u;
    ComPtr<ID3D12Resource> result;
  };
  std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
  geometries.reserve(new_slots.size());
  std::vector<BlasBuild> builds;
  builds.reserve(new_slots.size());
  ID3D12Resource* vertex_arena = s.arenas[kRt12ArenaVertices].buffer.Get();
  ID3D12Resource* index_arena = s.arenas[kRt12ArenaIndices].buffer.Get();
  const uint64_t vertex_va = (vertex_arena != nullptr ? vertex_arena->GetGPUVirtualAddress() : 0u);
  const uint64_t index_va = (index_arena != nullptr ? index_arena->GetGPUVirtualAddress() : 0u);
  for (const Rt12MirrorSlot& slot : new_slots) {
    if (slot.index_count / 3u == 0u) continue;
    D3D12_RAYTRACING_GEOMETRY_DESC geometry = {};
    geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
    geometry.Triangles.VertexBuffer.StartAddress = vertex_va + static_cast<uint64_t>(slot.vertex_offset) * 16u;
    geometry.Triangles.VertexBuffer.StrideInBytes = 16u;
    geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometry.Triangles.VertexCount = slot.vertex_count;
    geometry.Triangles.IndexBuffer = index_va + static_cast<uint64_t>(slot.index_offset) * 4u;
    geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    geometry.Triangles.IndexCount = (slot.index_count / 3u) * 3u;
    geometries.push_back(geometry);
    BlasBuild build;
    build.slot = slot.slot;
    build.inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    build.inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    build.inputs.NumDescs = 1u;
    build.inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    build.inputs.pGeometryDescs = &geometries.back();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    s.device12->GetRaytracingAccelerationStructurePrebuildInfo(&build.inputs, &info);
    build.result_bytes = Align256(info.ResultDataMaxSizeInBytes);
    build.scratch_bytes = Align256(info.ScratchDataSizeInBytes);
    builds.push_back(build);
  }
  uint64_t scratch_total = 0u;
  for (BlasBuild& build : builds) {
    build.scratch_offset = scratch_total;
    scratch_total += build.scratch_bytes;
  }
  ComPtr<ID3D12Resource> scratch;
  if (!builds.empty()) {
    if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, scratch_total, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratch))) {
      FailSession(s, Rt12Reason::SubmitFailed, "blas scratch");
      return false;
    }
  }
  for (BlasBuild& build : builds) {
    if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, build.result_bytes,
                                D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, build.result))) {
      FailSession(s, Rt12Reason::SubmitFailed, "blas buffer");
      return false;
    }
  }
  for (const BlasBuild& build : builds) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = build.result->GetGPUVirtualAddress();
    desc.Inputs = build.inputs;
    desc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress() + build.scratch_offset;
    list->BuildRaytracingAccelerationStructure(&desc, 0u, nullptr);
  }
  if (!builds.empty()) {
    barriers.clear();
    UavBarrier(barriers, nullptr);
    Transition(barriers, s.arenas[kRt12ArenaVertices].buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    Transition(barriers, s.arenas[kRt12ArenaIndices].buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    RetireObject(s, s.last_signal, scratch);
  }
  for (const BlasBuild& build : builds) {
    if (s.blas.size() <= build.slot) s.blas.resize(build.slot + 1u);
    if (s.blas[build.slot].buffer) {
      RetireObject(s, s.last_signal, s.blas[build.slot].buffer);
      s.blas_bytes -= s.blas[build.slot].bytes;
    }
    s.blas[build.slot].buffer = build.result;
    s.blas[build.slot].bytes = build.result_bytes;
    s.blas_bytes += build.result_bytes;
    s.blas_generation += 1u;
  }

  // 9.9 End timestamp and resolve; the list closes.
  list->EndQuery(s.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, k * kRt12Timestamps + 1u);
  list->ResolveQueryData(s.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, k * kRt12Timestamps, 2u, s.query_readback.Get(),
                         static_cast<uint64_t>(k) * kRt12Timestamps * 8u);
  p.timestamps = true;
  if (FAILED(list->Close())) {
    FailSession(s, Rt12Reason::SubmitFailed, "present close");
    return false;
  }

  // 9.10 Ordering: a GPU wait on the previous D3D12 submission, then execute and signal. Nothing ran on a failure.
  if (s.last_signal != 0u) {
    bool ordered = false;
    try {
      ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence12.get(), s.last_signal);
    } catch (const std::exception&) {
      ordered = false;
    }
    if (!ordered) {
      FailSession(s, Rt12Reason::SubmitFailed, "present order wait");
      return false;
    }
  }
  ID3D12CommandList* lists[] = {list};
  s.queue12->ExecuteCommandLists(1u, lists);
  uint64_t value = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    value = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
  } catch (const std::exception&) {
    value = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  if (value == renodx::utils::gpu_fence::NO_SIGNAL) {
    FailSession(s, Rt12Reason::SubmitFailed, "present signal");
    return false;
  }
  p.fence_value = value;
  s.last_signal = value;
  s.last_signal_qpc = QpcNow();
  for (size_t i = retired_begin; i < s.retired.size(); ++i) s.retired[i].fence_value = value;
  s.present_next = (k + 1u) % kRt12Slots;
  UpdatePanel([](Rt12Panel& panel) { panel.present_submits += 1u; });
  return true;
}

// Called once per present, after UpdateLiveBvh (10.6).
inline void OnPresent(reshade::api::device* device) {
#if defined(_WIN32)
  if (!g_rt12_enabled.load(std::memory_order_relaxed)) {
    if (g_rt12) TeardownRt12("switch off");
    UpdatePanel([](Rt12Panel& p) { p.reason = Rt12Reason::Off; });
    return;
  }
  if (!g_rt12) {
    if (rtao::RtaoRequested()) CreateRt12(device);
    return;
  }
  Rt12State& s = *g_rt12;
  if (s.failed || s.stuck) return;

  // 2. Stuck detection: no fence progress for kRt12StuckMs with work pending.
  const uint64_t completed = s.fence12->GetCompletedValue();
  if (completed < s.last_signal && QpcMsSince(s.last_signal_qpc) > static_cast<double>(kRt12StuckMs)) {
    s.stuck = true;
    renodx::utils::log::w("[world-rt12] stuck: fence at ", completed, ", submitted ", s.last_signal);
    UpdatePanel([](Rt12Panel& p) { p.stuck = true; });
    DisableMirror();
    return;
  }

  // 2b. G4 (D2): the mirror is dropped while the GPU-cache scene traces, and re-streamed when it stops.
  const bool gc_owns = GcOwnsRtao();
  if (gc_owns && !s.mirror_dropped) {
    DisableMirror();
    for (Rt12Blas& blas : s.blas) {
      RetireObject(s, s.last_signal, blas.buffer);
      blas.buffer.Reset();
    }
    s.blas.clear();
    s.blas_bytes = 0u;
    s.blas_generation += 1u;
    for (Rt12Arena& arena : s.arenas) {
      RetireObject(s, s.last_signal, arena.buffer);
      arena.buffer.Reset();
      arena.capacity = 0u;
      arena.used = 0u;
    }
    s.scene_slot = -1;
    s.static_descs.clear();
    s.static_key_version = ~0ull;
    s.static_key_blas = ~0ull;
    s.mirror_dropped = true;
    renodx::utils::log::i("[world-rt12] mirror off (GPU capture traces)");
  } else if (!gc_owns && s.mirror_dropped) {
    {
      std::lock_guard lock(g_rt12_mirror.mutex);
      g_rt12_mirror.active = true;
      g_rt12_mirror.reset = false;
      g_rt12_mirror.appends.clear();
      g_rt12_mirror.new_slots.clear();
      g_rt12_mirror.retired_slots.clear();
    }
    bvh::g_live_bvh.reset_requested.store(true, std::memory_order_relaxed);
    s.mirror_dropped = false;
    renodx::utils::log::i("[world-rt12] mirror on (re-stream)");
  }

  // 3. Release retired objects whose fence completed.
  std::erase_if(s.retired, [completed](const Rt12Retired& retired) { return retired.fence_value <= completed; });

  // 3b. Character BLAS not built or refit for bvh::kDynamicRetireFrames frames: retired with the last submission.
  const uint32_t frame = g_state.frame.load();
  for (auto it = s.dynamic_blas.begin(); it != s.dynamic_blas.end();) {
    if (frame - it->second.last_frame > bvh::kDynamicRetireFrames) {
      RetireObject(s, s.last_signal, it->second.buffer);
      it = s.dynamic_blas.erase(it);
    } else {
      ++it;
    }
  }

  // 4. Timestamps of the slots whose submission completed.
  for (uint32_t k = 0u; k < kRt12Slots; ++k) {
    if (s.present_slots[k].timestamps && s.present_slots[k].fence_value <= completed) {
      uint64_t t[2] = {};
      if (ReadRt12Timestamps(s, k * kRt12Timestamps, 2u, t)) {
        const float ms = TicksToMs(t[1] - t[0], s.timestamp_frequency);
        UpdatePanel([ms](Rt12Panel& p) {
          p.blas_ms = ms;
          p.blas_ms_sum += ms;
          p.blas_ms_n += 1u;
        });
      }
      s.present_slots[k].timestamps = false;
    }
    if (s.lighting_slots[k].timestamps && s.lighting_slots[k].fence_value <= completed) {
      uint64_t t[5] = {};
      if (ReadRt12Timestamps(s, k * kRt12Timestamps + 2u, 5u, t)) {
        const bool dynamic_timed = s.lighting_slots[k].dynamic_timed;
        const bool records_timed = s.lighting_slots[k].records_timed;
        const float dynamic_ms = TicksToMs(t[1] - t[0], s.timestamp_frequency);
        const float records_ms = TicksToMs(t[2] - t[1], s.timestamp_frequency);
        const float tlas_ms = TicksToMs(t[3] - t[2], s.timestamp_frequency);
        const float trace_ms = TicksToMs(t[4] - t[3], s.timestamp_frequency);
        UpdatePanel([dynamic_timed, dynamic_ms, records_timed, records_ms, tlas_ms, trace_ms](Rt12Panel& p) {
          if (dynamic_timed) {
            p.dynamic_ms = dynamic_ms;
            p.dynamic_ms_sum += dynamic_ms;
            p.dynamic_ms_n += 1u;
          }
          if (records_timed) {
            p.records_ms = records_ms;
            p.records_ms_sum += records_ms;
            p.records_ms_n += 1u;
          }
          p.tlas_ms = tlas_ms;
          p.tlas_ms_sum += tlas_ms;
          p.tlas_ms_n += 1u;
          p.trace_ms = trace_ms;
          p.trace_ms_sum += trace_ms;
          p.trace_ms_n += 1u;
        });
      }
      s.lighting_slots[k].timestamps = false;
    }
  }

  // 5. Drain the mirror (moves only; the materials are copied on the CPU).
  std::vector<Rt12MirrorAppend> appends;
  std::vector<Rt12MirrorSlot> new_slots;
  std::vector<uint32_t> retired_slots;
  bool drained_reset = false;
  uint64_t sanitized_total = 0u;
  {
    std::lock_guard lock(g_rt12_mirror.mutex);
    drained_reset = g_rt12_mirror.reset;
    g_rt12_mirror.reset = false;
    appends.swap(g_rt12_mirror.appends);
    new_slots.swap(g_rt12_mirror.new_slots);
    retired_slots.swap(g_rt12_mirror.retired_slots);
    sanitized_total = g_rt12_mirror.sanitized_triangles;
    if (g_rt12_mirror.materials_version != s.materials_version_seen) {
      s.material_cpu = g_rt12_mirror.materials;
      s.materials_version_seen = g_rt12_mirror.materials_version;
      s.materials_dirty = true;
    }
  }
  UpdatePanel([sanitized_total](Rt12Panel& p) { p.sanitized_triangles = sanitized_total; });
  const bool materials_copy = s.materials_dirty;

  // 6. CPU-side changes: a store reset empties the scene, retired slots drop their BLAS.
  const uint32_t saved_mesh_count = s.mesh_count;
  const uint64_t saved_mesh_store_version = s.mesh_store_version;
  if (drained_reset) {
    for (Rt12Blas& blas : s.blas) {
      if (blas.buffer) RetireObject(s, s.last_signal, blas.buffer);
    }
    s.blas.clear();
    s.blas_bytes = 0u;
    s.blas_generation += 1u;
    for (Rt12Arena& arena : s.arenas) arena.used = 0u;
    s.mesh_count = 0u;
    s.mesh_store_version = ~0ull;
  }
  for (uint32_t slot : retired_slots) {
    if (slot < s.blas.size() && s.blas[slot].buffer) {
      RetireObject(s, s.last_signal, s.blas[slot].buffer);
      s.blas_bytes -= s.blas[slot].bytes;
      s.blas[slot] = Rt12Blas{};
      s.blas_generation += 1u;
    }
  }

  // 7. Mesh table: uploaded when the store version moved.
  const bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(device);
  bool table_upload = false;
  if (data != nullptr && data->store_version != s.mesh_store_version) {
    table_upload = true;
    s.mesh_count = static_cast<uint32_t>(data->mesh_descriptors.size());
    s.mesh_store_version = data->store_version;
  }

  // 8/9. Submission, unless nothing changed or the slot is still busy (then the work is put back, in order).
  const bool record = !appends.empty() || !new_slots.empty() || table_upload || materials_copy;
  if (record && s.present_slots[s.present_next].fence_value > completed) {
    {
      std::lock_guard lock(g_rt12_mirror.mutex);
      if (!g_rt12_mirror.reset) {
        g_rt12_mirror.appends.insert(g_rt12_mirror.appends.begin(), std::make_move_iterator(appends.begin()),
                                     std::make_move_iterator(appends.end()));
        g_rt12_mirror.new_slots.insert(g_rt12_mirror.new_slots.begin(), new_slots.begin(), new_slots.end());
        g_rt12_mirror.retired_slots.insert(g_rt12_mirror.retired_slots.begin(), retired_slots.begin(), retired_slots.end());
      }
      g_rt12_mirror.reset = g_rt12_mirror.reset || drained_reset;
    }
    s.mesh_count = saved_mesh_count;
    s.mesh_store_version = saved_mesh_store_version;
    UpdatePanel([](Rt12Panel& p) { p.present_skipped_busy += 1u; });
  } else if (record) {
    if (!SubmitPresent(s, data, appends, new_slots, table_upload, materials_copy)) return;
  }

  // 10. Alpha atlas mirror (D3D11): copied when the live atlas or its contents changed.
  if (s.atlas.texture11 && data != nullptr && data->alpha.atlas.handle != 0u
      && (data->alpha.atlas.handle != s.atlas_source.handle || data->alpha.blits != s.atlas_blits)) {
    s.ctx11->CopyResource(s.atlas.texture11.Get(),
                          reinterpret_cast<ID3D11Resource*>(data->alpha.atlas.handle));  // NOLINT(performance-no-int-to-ptr)
    s.atlas_source = data->alpha.atlas;
    s.atlas_blits = data->alpha.blits;
    UpdatePanel([](Rt12Panel& p) { p.atlas_copies += 1u; });
  }

  // 11. Panel counts.
  uint32_t blas_count = 0u;
  for (const Rt12Blas& blas : s.blas) blas_count += blas.buffer ? 1u : 0u;
  UpdatePanel([&s, blas_count](Rt12Panel& p) {
    p.blas_count = blas_count;
    p.blas_bytes = s.blas_bytes;
    p.mesh_count = s.mesh_count;
    for (uint32_t a = 0u; a < 3u; ++a) p.arena_bytes[a] = s.arenas[a].capacity * kRt12ArenaStride[a];
    p.alpha_mirror = s.atlas.texture11 != nullptr;
    p.wrapped = s.wrapped;
  });
#endif
}

// RTAO pass A on D3D12 (10.7). Returns true only when pass A's outputs were produced this frame; every false
// records the reason and leaves pass A's targets untouched, so the compute pass A runs instead.
inline bool RunRtaoPassA(reshade::api::device* device, reshade::api::command_list* cmd_list, rtao::RtaoDeviceData& rd,
                         const rtao::RtaoPushConstants& push, reshade::api::resource_view depth_view,
                         reshade::api::resource_view mrt_normal_view, reshade::api::resource_view isfast_view,
                         reshade::api::resource_view scene_cbv_view,
                         const reshade::api::resource_view (&uavs)[rtao::kRtaoUavCount], uint32_t trace_w, uint32_t trace_h) {
#if defined(_WIN32)
  using Microsoft::WRL::ComPtr;
  using DS = reshade::api::shader_stage;
  using DT = reshade::api::descriptor_type;
  // 1. Off, or the state belongs to another device.
  if (!g_rt12_enabled.load(std::memory_order_relaxed)) return RtFallback(Rt12Reason::Off);
  if (!g_rt12 || g_rt12->device != device) return RtFallback(Rt12Reason::NotCreated);
  Rt12State& s = *g_rt12;
  // 2. Stuck, or failed (the reason recorded when it failed).
  if (s.stuck) return RtFallback(Rt12Reason::Stuck);
  if (s.failed) {
    Rt12Reason recorded = Rt12Reason::CreateFailed;
    {
      std::lock_guard lock(g_rt12_panel_mutex);
      recorded = g_rt12_panel.reason;
    }
    return RtFallback(recorded);
  }
  // 3. Discovery mode always uses the compute pass.
  if ((static_cast<uint32_t>(push.fade[2]) & rtao::kRtaoFlagDiscovery) != 0u) return RtFallback(Rt12Reason::Discovery);
  // 4. The commands must be recorded on the game's immediate context.
  if (reinterpret_cast<ID3D11DeviceContext*>(cmd_list->get_native()) != s.ctx11.Get()) {  // NOLINT(performance-no-int-to-ptr)
    return RtFallback(Rt12Reason::ContextMismatch);
  }
  const bool gc = GcOwnsRtao();
  const Rt12GcScene& gc_scene = g_rt12_gc_scene;
  // 5. A built TLAS (GPU-cache scene: the GPU records, see 7c).
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(device);
  if (data == nullptr || (!gc && !data->tlas_built)) return RtFallback(Rt12Reason::TlasEmpty);
  if (gc && !gc_scene.ready) {
    const char* not_ready = gc_scene.not_ready;
    UpdatePanel([not_ready](Rt12Panel& p) { p.gc_not_ready = not_ready; });
    return RtFallback(Rt12Reason::GpuCapture);
  }
  // 6. Alpha foliage needs the atlas mirror.
  if (bvh::g_pool.alpha_foliage.load(std::memory_order_relaxed) && s.atlas.texture12 == nullptr) {
    return RtFallback(Rt12Reason::AlphaMirror);
  }
  // 7. The lighting slot must be free.
  const uint64_t completed = s.fence12->GetCompletedValue();
  const uint32_t k = s.lighting_next;
  Rt12Slot& L = s.lighting_slots[k];
  if (L.fence_value > completed) {
    UpdatePanel([](Rt12Panel& p) { p.busy_frames += 1u; });
    return RtFallback(Rt12Reason::Busy);
  }

  // 7b. The characters RefitDeformLiveCharacters traced this frame (the compute pass A's set, in the same arena).
  const uint32_t characters_requested = static_cast<uint32_t>(push.size[2] + 0.5f);
  std::vector<bvh::DynamicCharacter> characters;
  reshade::api::resource_view arena_srv = {0u};
  uint64_t vertex_end = 0u;
  if (characters_requested > 0u) {
    if (!bvh::GetDeformLiveCharacters(device, &characters, &arena_srv) || characters.size() != characters_requested) {
      return RtFallback(Rt12Reason::Characters);
    }
    std::vector<uint32_t> vertex_bases;
    std::vector<uint32_t> triangle_counts;
    for (const bvh::DynamicCharacter& character : characters) {
      vertex_bases.push_back(character.vertex_base);
      triangle_counts.push_back(character.triangles);
    }
    vertex_end = Rt12DynamicVertexEnd(vertex_bases.data(), triangle_counts.data(), characters.size());
    if (vertex_end == 0u || vertex_end > static_cast<uint64_t>(kRt12CarrierWidth) * kRt12CarrierHeight) {
      return RtFallback(Rt12Reason::Characters);
    }
  }
  // 7c. The P2b records of this present (the compute pass A traces the same t19 records). Over the capacity: compute.
  uint32_t records = 0u;
  std::memcpy(&records, &push.fade[3], sizeof(uint32_t));
  if (gc) {
    records = gc_scene.bound;
  } else if (records > 0u && (records > bvh::kPoolDynamicInstances || data->dynamic.instances_srv.handle == 0u || !data->dynamic.ready)) {
    return RtFallback(Rt12Reason::Records);
  }
  const uint32_t frame = g_state.frame.load();

  // 8. Trace-size targets and the D3D11 pipelines.
  const auto ensure_target = [&](Rt12Shared& shared, DXGI_FORMAT format) {
    if (shared.texture11 && shared.width == trace_w && shared.height == trace_h) return true;
    RetireShared(s, shared, s.last_signal);
    return SUCCEEDED(CreateRt12Shared(s, device, format, trace_w, trace_h, 1u, true, &shared));
  };
  if (!ensure_target(s.depth, DXGI_FORMAT_R32_UINT) || !ensure_target(s.normal, DXGI_FORMAT_R32G32B32A32_UINT)
      || !ensure_target(s.texel, DXGI_FORMAT_R32_UINT) || !ensure_target(s.raw, DXGI_FORMAT_R32_FLOAT)) {
    return RtFallback(Rt12Reason::TargetFailed);
  }
  if (!CreateRt12Pipelines(device, rd.layout, s)) return RtFallback(Rt12Reason::TargetFailed);
  if (!characters.empty()) {
    if (s.carrier.texture12 == nullptr
        && FAILED(CreateRt12Shared(s, device, DXGI_FORMAT_R32G32B32A32_UINT, kRt12CarrierWidth, kRt12CarrierHeight, 1u, true,
                                   &s.carrier))) {
      return RtFallback(Rt12Reason::TargetFailed);
    }
    if (s.dynamic_vertices == nullptr
        && FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, static_cast<uint64_t>(kRt12CarrierWidth) * kRt12CarrierHeight * 16u,
                                   D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, s.dynamic_vertices))) {
      return RtFallback(Rt12Reason::TargetFailed);
    }
  }
  Rt12Shared& records_carrier = (gc ? s.gc_records : s.records);
  const uint32_t records_height = (gc ? kRt12GcRecordCarrierHeight : kRt12RecordCarrierHeight);
  if (records > 0u && records_carrier.texture12 == nullptr
      && FAILED(CreateRt12Shared(s, device, DXGI_FORMAT_R32G32B32A32_UINT, kRt12CarrierWidth, records_height, 1u, true,
                                 &records_carrier))) {
    return RtFallback(Rt12Reason::TargetFailed);
  }
  const bool mirror_missing = s.arenas[kRt12ArenaIndices].buffer == nullptr || s.arenas[kRt12ArenaUvs].buffer == nullptr
                              || s.mesh_table == nullptr;
  if ((!gc && mirror_missing) || s.materials == nullptr) {
    return RtFallback(Rt12Reason::TlasEmpty);
  }

  // 9. The instance list (CPU). The static part is cached by its key (static_key_*); characters are appended in 13.1b.
  // Holes, instances without a BLAS and bad mesh ids are counted when the static part is rebuilt.
  const bool static_changed = data->tlas_cpu_version != s.static_key_version || s.blas_generation != s.static_key_blas;
  if (!gc && static_changed) {
    s.static_descs.clear();
    uint32_t holes = 0u, bad_mesh = 0u, no_blas = 0u;
    for (size_t i = 0u; i < data->tlas_instances_cpu.size(); ++i) {
      if (i >= data->tlas_info.size() || data->tlas_info[i].id == 0u) {
        holes += 1u;
        continue;
      }
      const bvh::WorldInstanceGPU& instance = data->tlas_instances_cpu[i];
      const uint32_t slot = static_cast<uint32_t>(instance.header[0]);
      if (slot >= s.mesh_count) {
        bad_mesh += 1u;
        continue;
      }
      if (slot >= s.blas.size() || !s.blas[slot].buffer) {
        no_blas += 1u;
        continue;
      }
      const Rt12InstanceFields fields = MakeRt12InstanceFields(instance.world, instance.visibility[2], instance.header[2]);
      D3D12_RAYTRACING_INSTANCE_DESC desc = {};
      std::memcpy(desc.Transform, fields.transform, sizeof(desc.Transform));
      desc.InstanceID = static_cast<UINT>(i);
      desc.InstanceMask = fields.mask;
      desc.InstanceContributionToHitGroupIndex = 0u;
      desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE
                   | (fields.opaque ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE : D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE);
      desc.AccelerationStructure = s.blas[slot].buffer->GetGPUVirtualAddress();
      s.static_descs.push_back(desc);
    }
    UpdatePanel([holes, bad_mesh, no_blas](Rt12Panel& p) {
      p.tlas_holes = holes;
      p.tlas_bad_mesh = bad_mesh;
      p.tlas_no_blas = no_blas;
    });
    s.static_key_version = data->tlas_cpu_version;
    s.static_key_blas = s.blas_generation;
  }
  const bool rebuild = gc || s.scene_slot < 0 || s.scene_key_version != s.static_key_version || s.scene_key_blas != s.static_key_blas
                       || !characters.empty() || s.scene_dynamic_count != 0u || records > 0u || s.scene_record_count != 0u;
  if (!gc && rebuild && s.static_descs.empty() && characters.empty()) return RtFallback(Rt12Reason::TlasEmpty);
  // This frame's descs: the static part, then one per character (13.1b).
  std::vector<D3D12_RAYTRACING_INSTANCE_DESC> descs;
  if (rebuild && !gc) descs = s.static_descs;
  // The instance table is uploaded only when the slot's table does not hold the current static part.
  // With records the instance table holds the static part and kPoolDynamicInstances records (13.1c): a smaller one is recreated.
  const uint64_t table_records_bytes =
      (static_cast<uint64_t>(data->tlas_instances_cpu.size()) + bvh::kPoolDynamicInstances) * sizeof(bvh::WorldInstanceGPU);
  const bool table_upload = !gc && rebuild && (L.table_key_version != s.static_key_version || L.table_key_blas != s.static_key_blas
                                        || L.instance_table == nullptr || (records > 0u && L.table_bytes < table_records_bytes));

  // Binds one of the helper passes on RTAO's layout: cb_scene, then the SRV and UAV tables as given.
  const auto bind_pass = [&](reshade::api::pipeline pipeline, const reshade::api::resource_view* srv_views,
                             const reshade::api::resource_view* uav_views) {
    const reshade::api::buffer_range scene_range = rtao::SceneCbvRange(scene_cbv_view);
    reshade::api::descriptor_table_update updates[3] = {
        {rd.cbv_table, 0, 0, 1, DT::constant_buffer, &scene_range},
        {rd.srv_table, 0, 0, rtao::kRtaoSrvCount, DT::shader_resource_view, srv_views},
        {rd.uav_table, 0, 0, rtao::kRtaoUavCount, DT::unordered_access_view, uav_views},
    };
    device->update_descriptor_tables(3, updates);
    cmd_list->bind_pipeline(reshade::api::pipeline_stage::all_compute, pipeline);
    const reshade::api::descriptor_table tables[3] = {rd.cbv_table, rd.srv_table, rd.uav_table};
    cmd_list->bind_descriptor_tables(DS::all_compute, rd.layout, 0, 3, tables);
  };

  // 10. D3D11 noise copy (once per IS-FAST source).
  if (isfast_view.handle != 0u && isfast_view.handle != s.noise_source) {
    reshade::api::resource_view srvs[rtao::kRtaoSrvCount] = {};
    srvs[18] = isfast_view;
    const reshade::api::resource_view uav_views[rtao::kRtaoUavCount] = {s.noise.uav11};
    bind_pass(s.noise_pipeline, srvs, uav_views);
    cmd_list->dispatch(16u, 512u, 1u);
    rtao::NullComputeSlots(cmd_list, rtao::kRtaoSrvCount, rtao::kRtaoUavCount, 0u);
    s.noise_source = isfast_view.handle;
    UpdatePanel([](Rt12Panel& p) { p.noise_copies += 1u; });
  }

  // 10b. D3D11 carrier: the characters' arena vertices [0, vertex_end) into the shared RGBA32_UINT carrier (t10 = arena).
  if (!characters.empty()) {
    reshade::api::resource_view srvs[rtao::kRtaoSrvCount] = {};
    srvs[10] = arena_srv;
    const reshade::api::resource_view uav_views[rtao::kRtaoUavCount] = {s.carrier.uav11};
    bind_pass(s.carrier_pipeline, srvs, uav_views);
    const uint32_t carrier_push[rtao::kRtaoPushConstantCount] = {static_cast<uint32_t>(vertex_end)};
    cmd_list->push_constants(DS::all_compute, rd.layout, 3, 0, rtao::kRtaoPushConstantCount, carrier_push);
    cmd_list->dispatch(static_cast<uint32_t>((vertex_end + 63u) / 64u), 1u, 1u);
    rtao::NullComputeSlots(cmd_list, rtao::kRtaoSrvCount, rtao::kRtaoUavCount, 0u);
  }

  // 10c. D3D11 records carrier: the P2b records of this present (t19) into the shared RGBA32_UINT records texture.
  if (records > 0u) {
    reshade::api::resource_view srvs[rtao::kRtaoSrvCount] = {};
    srvs[19] = (gc ? gc_scene.records_srv : data->dynamic.instances_srv);
    const reshade::api::resource_view uav_views[rtao::kRtaoUavCount] = {records_carrier.uav11};
    bind_pass(s.records_carrier_pipeline, srvs, uav_views);
    const uint32_t records_push[rtao::kRtaoPushConstantCount] = {records};
    cmd_list->push_constants(DS::all_compute, rd.layout, 3, 0, rtao::kRtaoPushConstantCount, records_push);
    cmd_list->dispatch((records + 63u) / 64u, 1u, 1u);
    rtao::NullComputeSlots(cmd_list, rtao::kRtaoSrvCount, rtao::kRtaoUavCount, 0u);
  }

  // 11. D3D11 pack: depth, normal and the cb_scene rows of the trace pixels into the shared textures.
  {
    reshade::api::resource_view srvs[rtao::kRtaoSrvCount] = {};
    srvs[9] = depth_view;
    srvs[17] = mrt_normal_view;
    const reshade::api::resource_view uav_views[rtao::kRtaoUavCount] = {s.depth.uav11, s.normal.uav11, s.scene.uav11};
    bind_pass(s.pack_pipeline, srvs, uav_views);
    cmd_list->push_constants(DS::all_compute, rd.layout, 3, 0, rtao::kRtaoPushConstantCount, &push);
    cmd_list->dispatch((trace_w + 7u) / 8u, (trace_h + 7u) / 8u, 1u);
    rtao::NullComputeSlots(cmd_list, rtao::kRtaoSrvCount, rtao::kRtaoUavCount, 0u);
  }

  // 12. Signal A (the D3D11 pack is ordered before it), flushed.
  uint64_t a = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    const bvh::PoolCpuTimer flush_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockRt12SignalFlush]};
    a = renodx::utils::gpu_fence::QueueSignal(s.ctx11.Get(), s.fence11.get());
    s.ctx11->Flush();
  } catch (const std::exception&) {
    a = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  if (a == renodx::utils::gpu_fence::NO_SIGNAL) {
    FailSession(s, Rt12Reason::FenceError, "fence11 signal");
    return RtFallback(Rt12Reason::FenceError);
  }

  // 13. Lighting submission. 13.1: recording starts; the slot is free (busy rule); timestamp.
  ID3D12GraphicsCommandList4* list = s.lighting_list.Get();
  if (FAILED(L.allocator->Reset()) || FAILED(list->Reset(L.allocator.Get(), nullptr))) {
    FailSession(s, Rt12Reason::SubmitFailed, "lighting reset");
    return RtFallback(Rt12Reason::SubmitFailed);
  }
  std::vector<D3D12_RESOURCE_BARRIER> barriers;
  UavBarrier(barriers, nullptr);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  list->EndQuery(s.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, k * kRt12Timestamps + 2u);
  const size_t retired_begin = s.retired.size();  // the objects 13.2 replaces get this submission fence in 13.9

  const uint32_t base = k * kRt12Descriptors;
  const auto cpu_at = [&s](uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = s.heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s.descriptor_size;
    return handle;
  };
  const auto gpu_at = [&s](uint32_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = s.heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(index) * s.descriptor_size;
    return handle;
  };
  const auto buffer_srv = [](uint32_t stride, uint64_t count) {
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = DXGI_FORMAT_UNKNOWN;
    view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Buffer.FirstElement = 0u;
    view.Buffer.NumElements = static_cast<UINT>(std::max<uint64_t>(count, 1u));
    view.Buffer.StructureByteStride = stride;
    view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    return view;
  };
  const auto texture_srv = [](DXGI_FORMAT format) {
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = format;
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Texture2D.MipLevels = 1u;
    return view;
  };
  const auto texture_uav = [](DXGI_FORMAT format) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
    view.Format = format;
    view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    return view;
  };
  ID3D12Device5* device12 = s.device12.Get();

  // 13.1b Characters: the carrier into the vertex buffer (D3D12), one BLAS per character (build or refit) and one desc each.
  // The plan and the new BLAS are committed in 13.9 only.
  struct DynamicBuild {
    uint64_t key = 0u;
    uint32_t triangles = 0u;
    bool build = false;  // a full build (else a refit of the existing BLAS)
    uint64_t bytes = 0u;
    uint64_t build_scratch = 0u;
    uint64_t update_scratch = 0u;
    uint64_t scratch_offset = 0u;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    ComPtr<ID3D12Resource> buffer;
  };
  std::vector<DynamicBuild> dynamic_plan;
  uint64_t character_builds = 0u, character_refits = 0u;
  if (!characters.empty()) {
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> character_geometries;
    character_geometries.reserve(characters.size());  // inputs point into this vector: it must not reallocate
    const uint64_t vertex_va = s.dynamic_vertices->GetGPUVirtualAddress();
    uint64_t scratch_total = 0u;
    for (const bvh::DynamicCharacter& character : characters) {
      const auto found = s.dynamic_blas.find(character.key);
      const bool has_blas = found != s.dynamic_blas.end();
      D3D12_RAYTRACING_GEOMETRY_DESC geometry = {};
      geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
      geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
      geometry.Triangles.VertexBuffer.StartAddress = vertex_va + static_cast<uint64_t>(character.vertex_base) * 16u;
      geometry.Triangles.VertexBuffer.StrideInBytes = 16u;
      geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
      geometry.Triangles.VertexCount = 3u * character.triangles;
      character_geometries.push_back(geometry);
      DynamicBuild build;
      build.key = character.key;
      build.triangles = character.triangles;
      build.inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
      build.inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
                           | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
      build.inputs.NumDescs = 1u;
      build.inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
      build.inputs.pGeometryDescs = &character_geometries.back();
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
      s.device12->GetRaytracingAccelerationStructurePrebuildInfo(&build.inputs, &info);
      build.build = Rt12DynamicNeedsBuild(has_blas, has_blas ? found->second.triangles : 0u, character.triangles,
                                          has_blas ? frame - found->second.built_frame : 0u);
      build.build_scratch = Align256(info.ScratchDataSizeInBytes);
      build.update_scratch = Align256(info.UpdateScratchDataSizeInBytes);
      build.scratch_offset = scratch_total;
      scratch_total += build.build ? build.build_scratch : build.update_scratch;
      const uint64_t result_bytes = Align256(info.ResultDataMaxSizeInBytes);
      if (build.build && (!has_blas || found->second.bytes < result_bytes)) {
        if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, result_bytes,
                                    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, build.buffer))) {
          FailSession(s, Rt12Reason::SubmitFailed, "character blas buffer");
          return RtFallback(Rt12Reason::SubmitFailed);
        }
        build.bytes = result_bytes;
      } else {
        build.buffer = found->second.buffer;
        build.bytes = found->second.bytes;
      }
      dynamic_plan.push_back(build);
    }
    if (L.dynamic_scratch == nullptr || L.dynamic_scratch_bytes < scratch_total) {
      ComPtr<ID3D12Resource> scratch;
      if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, scratch_total, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratch))) {
        FailSession(s, Rt12Reason::SubmitFailed, "character blas scratch");
        return RtFallback(Rt12Reason::SubmitFailed);
      }
      RetireObject(s, s.last_signal, L.dynamic_scratch);
      L.dynamic_scratch = scratch;
      L.dynamic_scratch_bytes = scratch_total;
    }

    // Descriptors 14 (carrier SRV) and 15 (vertex UAV) of this slot, for the unpack.
    const D3D12_SHADER_RESOURCE_VIEW_DESC carrier_srv = texture_srv(DXGI_FORMAT_R32G32B32A32_UINT);
    device12->CreateShaderResourceView(s.carrier.texture12.Get(), &carrier_srv, cpu_at(base + 14u));
    D3D12_UNORDERED_ACCESS_VIEW_DESC vertex_uav = {};
    vertex_uav.Format = DXGI_FORMAT_UNKNOWN;
    vertex_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    vertex_uav.Buffer.NumElements = kRt12CarrierWidth * kRt12CarrierHeight;
    vertex_uav.Buffer.StructureByteStride = 16u;
    device12->CreateUnorderedAccessView(s.dynamic_vertices.Get(), nullptr, &vertex_uav, cpu_at(base + 15u));

    ID3D12DescriptorHeap* heaps[] = {s.heap.Get()};
    list->SetDescriptorHeaps(1u, heaps);
    barriers.clear();
    Transition(barriers, s.carrier.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Transition(barriers, s.dynamic_vertices.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    list->SetComputeRootSignature(s.unpack_root.Get());
    list->SetPipelineState(s.unpack_pso.Get());
    list->SetComputeRoot32BitConstant(0u, static_cast<UINT>(vertex_end), 0u);
    list->SetComputeRootDescriptorTable(1u, gpu_at(base + 14u));
    list->Dispatch(static_cast<UINT>((vertex_end + 63u) / 64u), 1u, 1u);
    barriers.clear();
    UavBarrier(barriers, s.dynamic_vertices.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    barriers.clear();
    Transition(barriers, s.dynamic_vertices.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    // Full builds and in-place refits: each character in its own scratch range.
    for (const DynamicBuild& build : dynamic_plan) {
      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
      desc.DestAccelerationStructureData = build.buffer->GetGPUVirtualAddress();
      desc.Inputs = build.inputs;
      desc.ScratchAccelerationStructureData = L.dynamic_scratch->GetGPUVirtualAddress() + build.scratch_offset;
      if (!build.build) {
        desc.SourceAccelerationStructureData = build.buffer->GetGPUVirtualAddress();
        desc.Inputs.Flags = static_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS>(
            desc.Inputs.Flags | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE);
      }
      list->BuildRaytracingAccelerationStructure(&desc, 0u, nullptr);
    }
    barriers.clear();
    UavBarrier(barriers, nullptr);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    barriers.clear();
    Transition(barriers, s.dynamic_vertices.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    Transition(barriers, s.carrier.texture12.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    for (const DynamicBuild& build : dynamic_plan) {
      D3D12_RAYTRACING_INSTANCE_DESC desc = {};
      desc.Transform[0][0] = 1.f;
      desc.Transform[1][1] = 1.f;
      desc.Transform[2][2] = 1.f;
      desc.InstanceID = kRt12DynamicInstanceId;
      desc.InstanceMask = kRt12MaskDynamic;
      desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE | D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
      desc.AccelerationStructure = build.buffer->GetGPUVirtualAddress();
      descs.push_back(desc);
      if (build.build) {
        character_builds += 1u;
      } else {
        character_refits += 1u;
      }
    }
  }
  list->EndQuery(s.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, k * kRt12Timestamps + 3u);

  const uint32_t static_count = (gc ? 0u : static_cast<uint32_t>(data->tlas_instances_cpu.size()));
  // 13.1a The upload buffer of this slot (CPU, rebuild frames): the descs at 0, the static instance table at table_offset
  // when it is uploaded and, with records, the BLAS address table after them. 13.1c and 13.2 copy from it.
  const uint64_t desc_bytes = descs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
  const uint64_t table_offset = Align256(desc_bytes);
  const uint64_t table_bytes = static_cast<uint64_t>(data->tlas_instances_cpu.size()) * sizeof(bvh::WorldInstanceGPU);
  // G4 (D8): in GC mode the GPU-cache mesh words follow the descs and the address table has one entry per GC slot.
  const bool gc_write = gc && L.gc_version != gc_scene.version;
  const uint64_t gc_meshes_bytes = uint64_t{bvh::kGcMaxEntries} * sizeof(bvh::WorldMeshGPU);
  const uint64_t address_count = (gc ? uint64_t{bvh::kGcMaxEntries} : std::max<uint64_t>(s.blas.size(), 1u));
  const bool address_write = (gc ? gc_write : (records > 0u && (L.addresses_generation != s.blas_generation || L.addresses_bytes < address_count * 8u)));
  const uint64_t addresses_offset = (gc ? Align256(table_offset + gc_meshes_bytes) : Align256(table_offset + (table_upload ? table_bytes : 0u)));
  const uint64_t upload_bytes = address_write ? addresses_offset + address_count * 8u : table_offset + (table_upload ? table_bytes : 0u);
  if (rebuild) {
    if (L.upload == nullptr || L.upload_bytes < upload_bytes) {
      const uint64_t bytes = std::max<uint64_t>({upload_bytes, 2u * L.upload_bytes, 256u});
      ComPtr<ID3D12Resource> upload;
      HRESULT hr = CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_UPLOAD, bytes, D3D12_RESOURCE_STATE_GENERIC_READ,
                                    D3D12_RESOURCE_FLAG_NONE, upload);
      void* mapped = nullptr;
      if (SUCCEEDED(hr)) hr = upload->Map(0u, nullptr, &mapped);
      if (FAILED(hr)) {
        FailSession(s, Rt12Reason::SubmitFailed, "tlas upload", hr);
        return RtFallback(Rt12Reason::SubmitFailed);
      }
      RetireObject(s, s.last_signal, L.upload);
      L.upload = upload;
      L.upload_bytes = bytes;
      L.upload_data = static_cast<uint8_t*>(mapped);
    }
    std::memcpy(L.upload_data, descs.data(), desc_bytes);
    if (table_upload) {
      std::memcpy(L.upload_data + table_offset, data->tlas_instances_cpu.data(), table_bytes);
      const uint64_t table_need = records > 0u ? table_records_bytes : table_bytes;
      if (L.instance_table == nullptr || L.table_bytes < table_need) {
        const uint64_t bytes = std::max<uint64_t>(table_need, 192u);
        ComPtr<ID3D12Resource> table;
        if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, bytes, D3D12_RESOURCE_STATE_COMMON,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, table))) {
          FailSession(s, Rt12Reason::SubmitFailed, "instance table");
          return RtFallback(Rt12Reason::SubmitFailed);
        }
        RetireObject(s, s.last_signal, L.instance_table);
        L.instance_table = table;
        L.table_bytes = bytes;
      }
    }
    if (gc) {
      // G4 (D8): the GC records table (records entries, written by the records pass) and the GC mesh and address tables.
      const uint64_t gc_table_bytes = std::max<uint64_t>(uint64_t{records} * sizeof(bvh::WorldInstanceGPU), 192u);
      if (L.instance_table == nullptr || L.table_bytes < gc_table_bytes) {
        ComPtr<ID3D12Resource> table;
        if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, gc_table_bytes, D3D12_RESOURCE_STATE_COMMON,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, table))) {
          FailSession(s, Rt12Reason::SubmitFailed, "gc instance table");
          return RtFallback(Rt12Reason::SubmitFailed);
        }
        RetireObject(s, s.last_signal, L.instance_table);
        L.instance_table = table;
        L.table_bytes = gc_table_bytes;
      }
      L.table_key_version = ~0ull;
      if (L.gc_meshes == nullptr) {
        if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, gc_meshes_bytes, D3D12_RESOURCE_STATE_COMMON,
                                    D3D12_RESOURCE_FLAG_NONE, L.gc_meshes))) {
          FailSession(s, Rt12Reason::SubmitFailed, "gc mesh table");
          return RtFallback(Rt12Reason::SubmitFailed);
        }
      }
      if (L.gc_addresses == nullptr) {
        if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, address_count * 8u, D3D12_RESOURCE_STATE_COMMON,
                                    D3D12_RESOURCE_FLAG_NONE, L.gc_addresses))) {
          FailSession(s, Rt12Reason::SubmitFailed, "gc addresses");
          return RtFallback(Rt12Reason::SubmitFailed);
        }
      }
      if (gc_write) std::memcpy(L.upload_data + table_offset, gc_scene.meshes.data(), gc_meshes_bytes);
    }
    if (address_write) {
      if (gc) {
        std::memcpy(L.upload_data + addresses_offset, gc_scene.addresses.data(), address_count * 8u);
      } else {
        std::vector<uint64_t> addresses(address_count, 0u);
        for (size_t i = 0u; i < s.blas.size(); ++i) {
          if (s.blas[i].buffer) addresses[i] = s.blas[i].buffer->GetGPUVirtualAddress();
        }
        std::memcpy(L.upload_data + addresses_offset, addresses.data(), address_count * 8u);
      }
    }
  }

  // 13.1c P2b records (records > 0): the descs of the static and character part, the address table and (when uploaded)
  // the static instance table are copied into their buffers; then the records pass writes one desc per record and the
  // record itself at S + r (InstanceID).
  if (records > 0u) {
    const uint64_t descs_count = descs.size() + records;
    if (L.descs == nullptr || L.descs_bytes < descs_count * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)) {
      const uint64_t bytes = std::max<uint64_t>(descs_count * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), 256u);
      ComPtr<ID3D12Resource> descs_buffer;
      if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, bytes, D3D12_RESOURCE_STATE_COMMON,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, descs_buffer))) {
        FailSession(s, Rt12Reason::SubmitFailed, "records descs");
        return RtFallback(Rt12Reason::SubmitFailed);
      }
      RetireObject(s, s.last_signal, L.descs);
      L.descs = descs_buffer;
      L.descs_bytes = bytes;
    }
    if (!gc && address_write && (L.addresses == nullptr || L.addresses_bytes < address_count * 8u)) {
      const uint64_t bytes = std::max<uint64_t>(address_count * 8u, 256u);
      ComPtr<ID3D12Resource> addresses;
      if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, bytes, D3D12_RESOURCE_STATE_COMMON,
                                  D3D12_RESOURCE_FLAG_NONE, addresses))) {
        FailSession(s, Rt12Reason::SubmitFailed, "records addresses");
        return RtFallback(Rt12Reason::SubmitFailed);
      }
      RetireObject(s, s.last_signal, L.addresses);
      L.addresses = addresses;
      L.addresses_bytes = bytes;
    }
    ID3D12Resource* address_buffer = (gc ? L.gc_addresses.Get() : L.addresses.Get());

    // COMMON -> COPY_DEST for the copies, then the records pass, then NON_PIXEL_SHADER_RESOURCE for the TLAS and the trace.
    barriers.clear();
    Transition(barriers, L.descs.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    if (address_write) {
      Transition(barriers, address_buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    } else {
      Transition(barriers, address_buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    if (table_upload) {
      Transition(barriers, L.instance_table.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    } else {
      Transition(barriers, L.instance_table.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }
    if (gc_write) Transition(barriers, L.gc_meshes.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    Transition(barriers, records_carrier.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    list->CopyBufferRegion(L.descs.Get(), 0u, L.upload.Get(), 0u, desc_bytes);
    if (address_write) list->CopyBufferRegion(address_buffer, 0u, L.upload.Get(), addresses_offset, address_count * 8u);
    if (table_upload) list->CopyBufferRegion(L.instance_table.Get(), 0u, L.upload.Get(), table_offset, table_bytes);
    if (gc_write) list->CopyBufferRegion(L.gc_meshes.Get(), 0u, L.upload.Get(), table_offset, gc_meshes_bytes);
    barriers.clear();
    Transition(barriers, L.descs.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    if (address_write) {
      Transition(barriers, address_buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    if (table_upload) {
      Transition(barriers, L.instance_table.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }
    if (gc_write) Transition(barriers, L.gc_meshes.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    // Descriptors 16..19 of this slot: records SRV, address SRV, descs UAV (raw), instance table UAV (structured).
    const D3D12_SHADER_RESOURCE_VIEW_DESC records_srv = texture_srv(DXGI_FORMAT_R32G32B32A32_UINT);
    device12->CreateShaderResourceView(records_carrier.texture12.Get(), &records_srv, cpu_at(base + 16u));
    const D3D12_SHADER_RESOURCE_VIEW_DESC addresses_srv = buffer_srv(8u, address_count);
    device12->CreateShaderResourceView(address_buffer, &addresses_srv, cpu_at(base + 17u));
    D3D12_UNORDERED_ACCESS_VIEW_DESC descs_uav = {};
    descs_uav.Format = DXGI_FORMAT_R32_TYPELESS;
    descs_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    descs_uav.Buffer.NumElements = static_cast<UINT>(descs_count * 16u);
    descs_uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    device12->CreateUnorderedAccessView(L.descs.Get(), nullptr, &descs_uav, cpu_at(base + 18u));
    D3D12_UNORDERED_ACCESS_VIEW_DESC table_uav = {};
    table_uav.Format = DXGI_FORMAT_UNKNOWN;
    table_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    table_uav.Buffer.NumElements = static_cast<UINT>(static_count + records);
    table_uav.Buffer.StructureByteStride = sizeof(bvh::WorldInstanceGPU);
    device12->CreateUnorderedAccessView(L.instance_table.Get(), nullptr, &table_uav, cpu_at(base + 19u));

    ID3D12DescriptorHeap* heaps[] = {s.heap.Get()};
    list->SetDescriptorHeaps(1u, heaps);
    list->SetComputeRootSignature(s.records_root.Get());
    list->SetPipelineState(s.records_pso.Get());
    const UINT records_constants[4] = {records, static_count, static_cast<UINT>(descs.size()),
                                       static_cast<UINT>(gc ? bvh::kGcMaxEntries : s.blas.size())};
    list->SetComputeRoot32BitConstants(0u, 4u, records_constants, 0u);
    list->SetComputeRootDescriptorTable(1u, gpu_at(base + 16u));
    list->Dispatch((records + 63u) / 64u, 1u, 1u);
    barriers.clear();
    UavBarrier(barriers, L.descs.Get());
    UavBarrier(barriers, L.instance_table.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    barriers.clear();
    Transition(barriers, L.descs.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Transition(barriers, L.instance_table.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Transition(barriers, records_carrier.texture12.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  }
  list->EndQuery(s.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, k * kRt12Timestamps + 4u);

  // 13.2 TLAS rebuild into this slot when the key moved, from the descs (L.descs with records) and the FULL instance array.
  if (rebuild) {
    if (table_upload && records == 0u) {  // with records the table is copied in 13.1c
      barriers.clear();
      Transition(barriers, L.instance_table.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
      list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
      list->CopyBufferRegion(L.instance_table.Get(), 0u, L.upload.Get(), table_offset, table_bytes);
      barriers.clear();
      Transition(barriers, L.instance_table.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    inputs.NumDescs = static_cast<UINT>(descs.size() + records);
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.InstanceDescs = (records > 0u ? L.descs : L.upload)->GetGPUVirtualAddress();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    s.device12->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
    const uint64_t tlas_bytes = Align256(info.ResultDataMaxSizeInBytes);
    const uint64_t scratch_bytes = Align256(info.ScratchDataSizeInBytes);
    if (L.tlas == nullptr || L.tlas_bytes < tlas_bytes) {
      ComPtr<ID3D12Resource> tlas;
      if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, tlas_bytes,
                                  D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                  tlas))) {
        FailSession(s, Rt12Reason::SubmitFailed, "tlas buffer");
        return RtFallback(Rt12Reason::SubmitFailed);
      }
      RetireObject(s, s.last_signal, L.tlas);
      L.tlas = tlas;
      L.tlas_bytes = tlas_bytes;
    }
    if (L.tlas_scratch == nullptr || L.scratch_bytes < scratch_bytes) {
      ComPtr<ID3D12Resource> scratch;
      if (FAILED(CreateRt12Buffer(s.device12.Get(), D3D12_HEAP_TYPE_DEFAULT, scratch_bytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratch))) {
        FailSession(s, Rt12Reason::SubmitFailed, "tlas scratch");
        return RtFallback(Rt12Reason::SubmitFailed);
      }
      RetireObject(s, s.last_signal, L.tlas_scratch);
      L.tlas_scratch = scratch;
      L.scratch_bytes = scratch_bytes;
    }
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build = {};
    build.DestAccelerationStructureData = L.tlas->GetGPUVirtualAddress();
    build.Inputs = inputs;
    build.ScratchAccelerationStructureData = L.tlas_scratch->GetGPUVirtualAddress();
    list->BuildRaytracingAccelerationStructure(&build, 0u, nullptr);
    barriers.clear();
    UavBarrier(barriers, L.tlas.Get());
    list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

  }
  list->EndQuery(s.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, k * kRt12Timestamps + 5u);

  // 13.4 The 14 descriptors of 10.5 for this slot, from the TLAS and instance table this list traces.
  Rt12Slot& scene = s.lighting_slots[rebuild ? k : static_cast<uint32_t>(s.scene_slot)];
  const uint32_t scene_count = rebuild ? static_count + records : scene.instance_count;
  ID3D12Resource* index_arena = (gc ? gc_scene.indices : s.arenas[kRt12ArenaIndices].buffer.Get());
  const uint64_t index_count = (gc ? gc_scene.index_count : s.arenas[kRt12ArenaIndices].used);
  ID3D12Resource* uv_arena = (gc ? gc_scene.uvs : s.arenas[kRt12ArenaUvs].buffer.Get());
  const uint64_t uv_count = (gc ? gc_scene.uv_count : s.arenas[kRt12ArenaUvs].used);
  ID3D12Resource* mesh_table = (gc ? L.gc_meshes.Get() : s.mesh_table.Get());
  const uint32_t mesh_count = (gc ? static_cast<uint32_t>(bvh::kGcMaxEntries) : s.mesh_count);
  const D3D12_SHADER_RESOURCE_VIEW_DESC indices_srv = buffer_srv(kRt12ArenaStride[kRt12ArenaIndices], index_count);
  device12->CreateShaderResourceView(index_arena, &indices_srv, cpu_at(base + 0u));
  const D3D12_SHADER_RESOURCE_VIEW_DESC mesh_srv = buffer_srv(sizeof(bvh::WorldMeshGPU), mesh_count);
  device12->CreateShaderResourceView(mesh_table, &mesh_srv, cpu_at(base + 1u));
  const D3D12_SHADER_RESOURCE_VIEW_DESC instance_srv = buffer_srv(sizeof(bvh::WorldInstanceGPU), scene_count);
  device12->CreateShaderResourceView(scene.instance_table.Get(), &instance_srv, cpu_at(base + 2u));
  const D3D12_SHADER_RESOURCE_VIEW_DESC uv_srv = buffer_srv(kRt12ArenaStride[kRt12ArenaUvs], uv_count);
  device12->CreateShaderResourceView(uv_arena, &uv_srv, cpu_at(base + 3u));
  D3D12_SHADER_RESOURCE_VIEW_DESC atlas_srv = {};
  atlas_srv.Format = DXGI_FORMAT_R32_UINT;
  atlas_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  atlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  atlas_srv.Texture2DArray.MipLevels = 1u;
  atlas_srv.Texture2DArray.ArraySize = kRt12AlphaSlices;
  device12->CreateShaderResourceView(s.atlas.texture12.Get(), &atlas_srv, cpu_at(base + 4u));
  const D3D12_SHADER_RESOURCE_VIEW_DESC materials_srv = buffer_srv(32u, kRt12AlphaSlices);
  device12->CreateShaderResourceView(s.materials.Get(), &materials_srv, cpu_at(base + 5u));
  const D3D12_SHADER_RESOURCE_VIEW_DESC normal_srv = texture_srv(DXGI_FORMAT_R32G32B32A32_UINT);
  device12->CreateShaderResourceView(s.normal.texture12.Get(), &normal_srv, cpu_at(base + 6u));
  D3D12_SHADER_RESOURCE_VIEW_DESC tlas_srv = {};
  tlas_srv.Format = DXGI_FORMAT_UNKNOWN;
  tlas_srv.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
  tlas_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  tlas_srv.RaytracingAccelerationStructure.Location = scene.tlas->GetGPUVirtualAddress();
  device12->CreateShaderResourceView(nullptr, &tlas_srv, cpu_at(base + 7u));
  const D3D12_SHADER_RESOURCE_VIEW_DESC depth_srv = texture_srv(DXGI_FORMAT_R32_UINT);
  device12->CreateShaderResourceView(s.depth.texture12.Get(), &depth_srv, cpu_at(base + 8u));
  const D3D12_SHADER_RESOURCE_VIEW_DESC scene_srv = texture_srv(DXGI_FORMAT_R32G32B32A32_UINT);
  device12->CreateShaderResourceView(s.scene.texture12.Get(), &scene_srv, cpu_at(base + 9u));
  const D3D12_SHADER_RESOURCE_VIEW_DESC noise_srv = texture_srv(DXGI_FORMAT_R32G32B32A32_UINT);
  device12->CreateShaderResourceView(s.noise.texture12.Get(), &noise_srv, cpu_at(base + 10u));
  const D3D12_UNORDERED_ACCESS_VIEW_DESC texel_uav = texture_uav(DXGI_FORMAT_R32_UINT);
  device12->CreateUnorderedAccessView(s.texel.texture12.Get(), nullptr, &texel_uav, cpu_at(base + 11u));
  const D3D12_UNORDERED_ACCESS_VIEW_DESC stats_uav = texture_uav(DXGI_FORMAT_R32_UINT);
  device12->CreateUnorderedAccessView(s.stats.texture12.Get(), nullptr, &stats_uav, cpu_at(base + 12u));
  device12->CreateUnorderedAccessView(s.stats.texture12.Get(), nullptr, &stats_uav, s.cpu_heap->GetCPUDescriptorHandleForHeapStart());
  const D3D12_UNORDERED_ACCESS_VIEW_DESC raw_uav = texture_uav(DXGI_FORMAT_R32_FLOAT);
  device12->CreateUnorderedAccessView(s.raw.texture12.Get(), nullptr, &raw_uav, cpu_at(base + 13u));

  // 13.5 Barriers into the read and write states; the heap is bound before the stats clear (GPU handle in the heap).
  barriers.clear();
  if (!table_upload && records == 0u) Transition(barriers, scene.instance_table.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, index_arena, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, uv_arena, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, mesh_table, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, s.materials.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, s.depth.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, s.normal.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, s.scene.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, s.noise.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  if (s.atlas.texture12) Transition(barriers, s.atlas.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  Transition(barriers, s.texel.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(barriers, s.raw.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  Transition(barriers, s.stats.texture12.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  ID3D12DescriptorHeap* heaps[] = {s.heap.Get()};
  list->SetDescriptorHeaps(1u, heaps);
  const UINT zeros[4] = {0u, 0u, 0u, 0u};
  list->ClearUnorderedAccessViewUint(gpu_at(base + 12u), s.cpu_heap->GetCPUDescriptorHandleForHeapStart(), s.stats.texture12.Get(),
                                     zeros, 0u, nullptr);
  barriers.clear();
  UavBarrier(barriers, s.stats.texture12.Get());
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

  // 13.6 The trace.
  list->SetComputeRootSignature(s.root.Get());
  list->SetPipelineState(s.pso.Get());
  list->SetComputeRoot32BitConstants(0u, rtao::kRtaoPushConstantCount, &push, 0u);
  list->SetComputeRootDescriptorTable(1u, gpu_at(base));
  list->Dispatch((trace_w + 7u) / 8u, (trace_h + 7u) / 8u, 1u);

  // 13.7 Back to COMMON: the three UAV textures, the five buffers and the shared textures of 13.5.
  barriers.clear();
  Transition(barriers, s.texel.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, s.raw.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, s.stats.texture12.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, index_arena, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, uv_arena, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, mesh_table, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, s.materials.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, scene.instance_table.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  if (records > 0u) {
    Transition(barriers, L.descs.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    if (gc) {
      Transition(barriers, L.gc_addresses.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    } else {
      Transition(barriers, L.addresses.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
    }
  }
  Transition(barriers, s.depth.texture12.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, s.normal.texture12.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, s.scene.texture12.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  Transition(barriers, s.noise.texture12.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  if (s.atlas.texture12) Transition(barriers, s.atlas.texture12.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
  list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
  list->EndQuery(s.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, k * kRt12Timestamps + 6u);
  list->ResolveQueryData(s.queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, k * kRt12Timestamps + 2u, 5u, s.query_readback.Get(),
                         static_cast<uint64_t>(k * kRt12Timestamps + 2u) * 8u);
  L.timestamps = true;
  if (FAILED(list->Close())) {
    FailSession(s, Rt12Reason::SubmitFailed, "lighting close");
    return RtFallback(Rt12Reason::SubmitFailed);
  }

  // 13.8 Ordering: wait for the D3D11 signal A, then for the previous D3D12 submission; execute; signal B.
  // Nothing was executed when a wait fails (the compute pass A runs this frame).
  const auto submit_start = bvh::PerfNow();
  bool ordered = false;
  try {
    ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence11.get(), a);
    if (ordered && s.last_signal != 0u) {
      ordered = renodx::utils::gpu_fence::QueueWait(s.queue12.Get(), s.fence12.get(), s.last_signal);
    }
  } catch (const std::exception&) {
    ordered = false;
  }
  if (!ordered) {
    FailSession(s, Rt12Reason::SubmitFailed, "lighting order wait");
    return RtFallback(Rt12Reason::SubmitFailed);
  }
  ID3D12CommandList* lists[] = {list};
  s.queue12->ExecuteCommandLists(1u, lists);
  uint64_t b = renodx::utils::gpu_fence::NO_SIGNAL;
  try {
    b = renodx::utils::gpu_fence::QueueSignal(s.queue12.Get(), s.fence12.get());
  } catch (const std::exception&) {
    b = renodx::utils::gpu_fence::NO_SIGNAL;
  }
  if (b == renodx::utils::gpu_fence::NO_SIGNAL) {
    FailSession(s, Rt12Reason::SubmitFailed, "lighting signal");
    return RtFallback(Rt12Reason::SubmitFailed);
  }
  bvh::AddPerfBlockMs(bvh::kPerfBlockRt12Submit, bvh::LiveElapsedMs(submit_start));

  // 13.9 Bookkeeping.
  L.fence_value = b;
  for (size_t i = retired_begin; i < s.retired.size(); ++i) s.retired[i].fence_value = b;
  s.last_signal = b;
  s.last_signal_qpc = QpcNow();
  L.dynamic_timed = !characters.empty();
  L.records_timed = records > 0u;
  if (rebuild) {
    s.scene_slot = static_cast<int32_t>(k);
    if (gc) {
      s.scene_key_version = ~0ull;
      s.scene_key_blas = ~0ull;
      L.gc_version = gc_scene.version;
    } else {
      s.scene_key_version = s.static_key_version;
      s.scene_key_blas = s.static_key_blas;
    }
    s.scene_dynamic_count = static_cast<uint32_t>(characters.size());
    s.scene_record_count = records;
    if (!gc && address_write) L.addresses_generation = s.blas_generation;
    L.instance_count = static_count;
    if (table_upload) {
      L.table_key_version = s.static_key_version;
      L.table_key_blas = s.static_key_blas;
    }
    for (const DynamicBuild& build : dynamic_plan) {
      Rt12DynamicBlas& entry = s.dynamic_blas[build.key];
      if (entry.buffer.Get() != build.buffer.Get()) RetireObject(s, b, entry.buffer);
      entry.buffer = build.buffer;
      entry.bytes = build.bytes;
      entry.build_scratch = build.build_scratch;
      entry.update_scratch = build.update_scratch;
      entry.triangles = build.triangles;
      if (build.build) entry.built_frame = frame;
      entry.last_frame = frame;
    }
    const uint32_t instances = static_cast<uint32_t>(descs.size());
    const uint64_t bytes = L.tlas_bytes;
    UpdatePanel([instances, bytes](Rt12Panel& p) {
      p.tlas_instances = instances;
      p.tlas_bytes = bytes;
    });
  }
  s.lighting_next = (k + 1u) % kRt12Slots;
  UpdatePanel([](Rt12Panel& p) { p.lighting_submits += 1u; });

  // 14. The D3D11 side waits for B (GPU wait, no CPU wait) before the resolve.
  bool waited = false;
  try {
    const bvh::PoolCpuTimer wait_timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockRt12WaitB]};
    waited = renodx::utils::gpu_fence::QueueWait(s.ctx11.Get(), s.fence12.get(), b);
  } catch (const std::exception&) {
    waited = false;
  }
  if (!waited) {
    FailSession(s, Rt12Reason::SubmitFailed, "d3d11 wait on fence B");
    return RtFallback(Rt12Reason::SubmitFailed);
  }

  // 15. D3D11 resolve: the shared outputs into pass A's own targets (u0 AO or u2 raw, u1 stats).
  {
    reshade::api::resource_view srvs[rtao::kRtaoSrvCount] = {};
    srvs[0] = s.texel.srv11;
    srvs[1] = s.raw.srv11;
    srvs[2] = s.stats.srv11;
    bind_pass(s.resolve_pipeline, srvs, uavs);
    cmd_list->push_constants(DS::all_compute, rd.layout, 3, 0, rtao::kRtaoPushConstantCount, &push);
    cmd_list->dispatch((trace_w + 7u) / 8u, (trace_h + 7u) / 8u, 1u);
    rtao::NullComputeSlots(cmd_list, rtao::kRtaoSrvCount, rtao::kRtaoUavCount, 0u);
  }

  // 16. Panel.
  uint64_t dynamic_blas_bytes = 0u;
  for (const auto& item : s.dynamic_blas) dynamic_blas_bytes += item.second.bytes;
  const uint32_t dynamic_objects = static_cast<uint32_t>(characters.size());
  const uint32_t dynamic_blas_count = static_cast<uint32_t>(s.dynamic_blas.size());
  UpdatePanel([gc, records, dynamic_objects, dynamic_blas_count, dynamic_blas_bytes, vertex_end, character_builds,
               character_refits](Rt12Panel& p) {
    p.traced_frames += 1u;
    p.reason = Rt12Reason::Tracing;
    p.records = (gc ? 0u : records);
    if (gc) {
      p.gc_traced_frames += 1u;
      p.gc_records = records;
      p.gc_not_ready.clear();
    }
    p.dynamic_objects = dynamic_objects;
    p.dynamic_blas_count = dynamic_blas_count;
    p.dynamic_blas_bytes = dynamic_blas_bytes;
    p.dynamic_vertices = vertex_end;
    p.dynamic_builds += character_builds;
    p.dynamic_refits += character_refits;
  });
  return true;
#else
  return false;
#endif
}

// Writes the JSON dump (10.8) from a copy of the panel.
inline void DumpRt12Json() {
  Rt12Panel p;
  {
    std::lock_guard lock(g_rt12_panel_mutex);
    p = g_rt12_panel;
  }
  std::ostringstream out;
  out << "{\n  \"schema\": 1,\n";
  out << "  \"switch_on\": " << (g_rt12_enabled.load(std::memory_order_relaxed) ? "true" : "false") << ",\n";
  out << "  \"reason\": \"" << Rt12ReasonName(p.reason) << "\",\n";
  out << "  \"failed_stage\": \"" << bvh::PoolJsonEscape(p.failed_stage) << "\",\n";
  out << "  \"hr\": " << static_cast<uint32_t>(p.hr) << ",\n";
  out << "  \"wrapped\": " << (p.wrapped ? "true" : "false") << ",\n";
  out << "  \"stuck\": " << (p.stuck ? "true" : "false") << ",\n";
  out << "  \"leaked\": " << (p.leaked ? "true" : "false") << ",\n";
  out << "  \"alpha_mirror\": " << (p.alpha_mirror ? "true" : "false") << ",\n";
  out << "  \"traced_frames\": " << p.traced_frames << ",\n";
  out << "  \"fallback_frames\": {";
  bool first = true;
  for (uint32_t r = 0u; r < kRt12ReasonCount; ++r) {
    if (p.fallback_frames[r] == 0u) continue;
    out << (first ? "" : ", ") << "\"" << Rt12ReasonName(static_cast<Rt12Reason>(r)) << "\": " << p.fallback_frames[r];
    first = false;
  }
  out << "},\n";
  out << "  \"busy_frames\": " << p.busy_frames << ",\n";
  out << "  \"present_submits\": " << p.present_submits << ",\n";
  out << "  \"present_skipped_busy\": " << p.present_skipped_busy << ",\n";
  out << "  \"lighting_submits\": " << p.lighting_submits << ",\n";
  out << "  \"blas_count\": " << p.blas_count << ",\n";
  out << "  \"blas_bytes\": " << p.blas_bytes << ",\n";
  out << "  \"mesh_count\": " << p.mesh_count << ",\n";
  out << "  \"arena_bytes\": {\"vertices\": " << p.arena_bytes[0] << ", \"indices\": " << p.arena_bytes[1]
      << ", \"uvs\": " << p.arena_bytes[2] << "},\n";
  out << "  \"tlas_instances\": " << p.tlas_instances << ",\n";
  out << "  \"tlas_no_blas\": " << p.tlas_no_blas << ",\n";
  out << "  \"tlas_holes\": " << p.tlas_holes << ",\n";
  out << "  \"tlas_bad_mesh\": " << p.tlas_bad_mesh << ",\n";
  out << "  \"tlas_bytes\": " << p.tlas_bytes << ",\n";
  out << "  \"sanitized_triangles\": " << p.sanitized_triangles << ",\n";
  out << "  \"atlas_copies\": " << p.atlas_copies << ",\n";
  out << "  \"noise_copies\": " << p.noise_copies << ",\n";
  const auto avg = [](float sum, uint32_t n) { return n == 0u ? bvh::PoolJsonFloat{NAN} : bvh::PoolJsonFloat{sum / static_cast<float>(n)}; };
  out << "  \"gpu_ms\": {\"blas\": " << bvh::PoolJsonFloat{p.blas_ms} << ", \"tlas\": " << bvh::PoolJsonFloat{p.tlas_ms}
      << ", \"trace\": " << bvh::PoolJsonFloat{p.trace_ms} << ", \"dynamic\": {\"last\": " << bvh::PoolJsonFloat{p.dynamic_ms}
      << ", \"avg\": " << avg(p.dynamic_ms_sum, p.dynamic_ms_n) << "}"
      << ", \"records\": {\"last\": " << bvh::PoolJsonFloat{p.records_ms}
      << ", \"avg\": " << avg(p.records_ms_sum, p.records_ms_n) << "}"
      << ", \"avg\": {\"blas\": " << avg(p.blas_ms_sum, p.blas_ms_n)
      << ", \"tlas\": " << avg(p.tlas_ms_sum, p.tlas_ms_n) << ", \"trace\": " << avg(p.trace_ms_sum, p.trace_ms_n) << "}},\n";
  out << "  \"dynamic_objects\": " << p.dynamic_objects << ",\n";
  out << "  \"dynamic_vertices\": " << p.dynamic_vertices << ",\n";
  out << "  \"dynamic_builds\": " << p.dynamic_builds << ",\n";
  out << "  \"dynamic_refits\": " << p.dynamic_refits << ",\n";
  out << "  \"dynamic_blas_count\": " << p.dynamic_blas_count << ",\n";
  out << "  \"dynamic_blas_bytes\": " << p.dynamic_blas_bytes << ",\n";
  out << "  \"gpu_capture\": {\"traced_frames\": " << p.gc_traced_frames << ", \"records\": " << p.gc_records << ", \"not_ready\": \""
      << bvh::PoolJsonEscape(p.gc_not_ready) << "\"},\n";
  out << "  \"records\": " << p.records << "\n}\n";
  const std::filesystem::path path = bvh::PoolOutputDir() / "world_rt12.json";
  std::string text = out.str();
  renodx::utils::path::WriteTextFile(path, text);
  renodx::utils::log::i("[world-rt12] wrote world_rt12.json");
}

}  // namespace falcom_world::rt12
