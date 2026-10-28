// Native harness: P2b on a real D3D11 WARP device (no game, no ReShade).
// (1) Arena copy: CopySubresourceRegion of a stride-160 structured slice and of 20 indirect-args bytes into the
//     raw arena. (2) The real world_dynamic_expand.cs_5_0.hlsl over a draw
//     table, checked against the CPU reference (CheckPoolWorld, ComputeMatrixInverse, TransformInst4x3RowDot).
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#include "gen/mock_base.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_resources.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_pool.hpp"

namespace bvh = falcom_world::bvh;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static const char kShaderDir[] = "src/games/falcomengine-plus/world/shaders/";
static const UINT kArenaElements = 64u;   // float4 elements of 160 bytes (bvh::kPoolInstanceStride) in the test arena
static const UINT kRecords = 32u;         // WorldInstanceGPU records
static const UINT kElementFloat4 = 10u;   // 160 bytes in float4

// Reads the #include files from the harness copy of the shaders folder.
struct FolderInclude : ID3DInclude {
  std::deque<std::string> loaded;
  HRESULT Open(D3D_INCLUDE_TYPE, LPCSTR name, LPCVOID, LPCVOID* data, UINT* bytes) override {
    std::ifstream file(std::string(kShaderDir) + name, std::ios::binary);
    if (!file) return E_FAIL;
    loaded.emplace_back((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    *data = loaded.back().data();
    *bytes = static_cast<UINT>(loaded.back().size());
    return S_OK;
  }
  HRESULT Close(LPCVOID) override { return S_OK; }
};

static void PrintMessages(ID3D11InfoQueue* queue, const char* tag) {
  if (queue == nullptr) return;
  const UINT64 count = queue->GetNumStoredMessages();
  for (UINT64 i = 0; i < count; ++i) {
    SIZE_T length = 0;
    queue->GetMessage(i, nullptr, &length);
    std::vector<uint8_t> raw(length);
    auto* message = reinterpret_cast<D3D11_MESSAGE*>(raw.data());
    if (SUCCEEDED(queue->GetMessage(i, message, &length))) {
      std::printf("  INFO %s: %.*s\n", tag, static_cast<int>(message->DescriptionByteLength), message->pDescription);
    }
  }
  queue->ClearStoredMessages();
}

static ID3D11Buffer* MakeBuffer(ID3D11Device* device, UINT bytes, UINT bind, UINT misc, UINT stride, const void* init) {
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = bytes;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = bind;
  desc.MiscFlags = misc;
  desc.StructureByteStride = stride;
  D3D11_SUBRESOURCE_DATA data = {init, 0u, 0u};
  ID3D11Buffer* buffer = nullptr;
  if (FAILED(device->CreateBuffer(&desc, init != nullptr ? &data : nullptr, &buffer))) return nullptr;
  return buffer;
}

static std::vector<uint8_t> Readback(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* source, UINT bytes) {
  std::vector<uint8_t> out(bytes, 0u);
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = bytes;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  ID3D11Buffer* staging = nullptr;
  if (FAILED(device->CreateBuffer(&desc, nullptr, &staging))) return out;
  context->CopyResource(staging, source);
  D3D11_MAPPED_SUBRESOURCE mapped = {};
  if (SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
    std::memcpy(out.data(), mapped.pData, bytes);
    context->Unmap(staging, 0);
  }
  staging->Release();
  return out;
}

static ID3D11ShaderResourceView* MakeSrv(ID3D11Device* device, ID3D11Buffer* buffer, UINT elements) {
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
  desc.Buffer.NumElements = elements;
  ID3D11ShaderResourceView* srv = nullptr;
  device->CreateShaderResourceView(buffer, &desc, &srv);
  return srv;
}

// The offline T2 draw table: one arena slice per draw, its matrices (elements 0..count-1) and the expected outcome.
struct MatrixCase {
  const char* name;
  float m[12];
};

static const float kNaN = std::numeric_limits<float>::quiet_NaN();

static const MatrixCase kCases[] = {
    {"identity", {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}},
    {"rotated_scaled", {1.732051f, -1.f, 0.f, 1.f, 1.f, 1.732051f, 0.f, 2.f, 0.f, 0.f, 2.f, 3.f}},
    {"mirrored", {-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}},
    {"zero_scale", {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}},
    {"nan", {kNaN, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}},
    {"translation_60000", {1, 0, 0, 60000.f, 0, 1, 0, 0, 0, 0, 1, 0}},
    {"skewed", {1, 0, 0, 0, 0.999f, 0.0447f, 0, 0, 0, 0, 1, 0}},
    {"scale_0.0005", {0.0005f, 0, 0, 0, 0, 0.0005f, 0, 0, 0, 0, 0.0005f, 0}},
    {"scale_20000", {20000.f, 0, 0, 0, 0, 20000.f, 0, 0, 0, 0, 20000.f, 0}},
};
static const size_t kCaseCount = sizeof(kCases) / sizeof(kCases[0]);

struct TestDraw {
  const char* name;
  uint32_t slice_element;   // first element of the slice in the arena
  uint32_t count;           // info.y
  uint32_t out;             // info.z
  uint32_t flags;           // info.w: bit 0 resident, bits 1-2 INSTANCE_* flags
  bool indirect;
  uint32_t args_element;    // indirect: element of the args (InstanceCount in .y)
  uint32_t args_count;
  uint32_t first_case;      // matrices kCases[first_case + i], wrapping
};

static const TestDraw kDraws[] = {
    {"direct_all_cases", 0u, 4u, 0u, 3u, false, 0u, 0u, 0u},
    {"direct_rest", 4u, 4u, 4u, 3u, false, 0u, 0u, 4u},
    {"direct_two", 8u, 2u, 8u, 3u, false, 0u, 0u, 8u},
    {"indirect_window", 10u, 4u, 10u, 3u, true, 14u, 2u, 0u},
    {"not_resident", 16u, 2u, 14u, 2u, false, 0u, 0u, 0u},
    {"camera_only", 20u, 2u, 16u, 3u, false, 0u, 0u, 1u},
    {"shadow_only", 24u, 2u, 18u, 5u, false, 0u, 0u, 2u},
};
static const size_t kDrawCount = sizeof(kDraws) / sizeof(kDraws[0]);

// G3 (world_gc_insert and world_gc_compact.cs_5_0.hlsl on WARP): the instance set over the scenario of the G3 plan.
// Each present runs the production order: clear, mark, rehash, insert (when draws), then compact. The set alternates
// its halves; the compare of the GPU records against the CPU reference is in test_live (TestGcCompareInstances).
static void TestGcInstanceShaders(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11InfoQueue* queue) {
  constexpr UINT kCapacity = 64u, kProbes = 8u, kMovingCapacity = 16u, kSlots = 16u, kElements = 512u;
  const auto compile = [](ID3D11Device* dev, const char* name) -> ID3D11ComputeShader* {
    std::string text;
    {
      std::ifstream file(std::string(kShaderDir) + name, std::ios::binary);
      text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
    FolderInclude include;
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    const HRESULT hr = D3DCompile(text.data(), text.size(), name, nullptr, &include, "main", "cs_5_0", 0u, 0u, &code, &errors);
    if (FAILED(hr)) {
      std::printf("G3 FAIL: compile %s hr=0x%08lx %s\n", name, static_cast<unsigned long>(hr),
                  errors != nullptr ? static_cast<const char*>(errors->GetBufferPointer()) : "");
      ++g_failures;
      return nullptr;
    }
    ID3D11ComputeShader* shader = nullptr;
    dev->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader);
    return shader;
  };
  ID3D11ComputeShader* insert_shader = compile(device, "world_gc_insert.cs_5_0.hlsl");
  ID3D11ComputeShader* compact_shader = compile(device, "world_gc_compact.cs_5_0.hlsl");
  CHECK(insert_shader != nullptr && compact_shader != nullptr, "G3 shaders compile");
  if (insert_shader == nullptr || compact_shader == nullptr) return;

  // One instance slice per element: world rows 0..2 (12 floats), prevWorld rows 3..5 (12 floats), 160 bytes.
  struct Mat12 {
    float v[12];
  };
  const auto translate = [](float x, float y, float z) {
    Mat12 m = {{1.f, 0.f, 0.f, x, 0.f, 1.f, 0.f, y, 0.f, 0.f, 1.f, z}};
    return m;
  };
  const Mat12 zero = {};
  std::vector<float> arena(kElements * 40u, 0.f);
  const auto put = [&arena](uint32_t element, const Mat12& world, const Mat12& prev) {
    std::memcpy(&arena[element * 40u], world.v, sizeof(world.v));
    std::memcpy(&arena[element * 40u + 12u], prev.v, sizeof(prev.v));
  };
  const Mat12 ma = translate(1.f, 2.f, 3.f), mb = translate(5.f, 0.f, 0.f), mc = translate(10.f, 0.f, 0.f), md = translate(20.f, 0.f, 0.f);
  const Mat12 me = translate(30.f, 0.f, 0.f), mf = translate(40.f, 0.f, 0.f), mg = translate(50.f, 0.f, 0.f);
  put(0u, ma, ma);
  put(1u, mb, mb);
  put(2u, ma, zero);
  put(3u, mc, translate(9.9f, 0.f, 0.f));
  put(4u, md, zero);
  put(5u, zero, zero);
  put(6u, me, me);
  put(7u, mf, mf);
  put(8u, mg, mf);
  for (uint32_t i = 0u; i < 100u; ++i) put(10u + i, translate(static_cast<float>(i), 0.f, 0.f), zero);
  // P9 (G3b): indirect args (5 uints, bytes 0-19 of their element) and the slices they point at.
  const auto put_args = [&arena](uint32_t element, const uint32_t (&args)[5]) { std::memcpy(&arena[element * 40u], args, sizeof(args)); };
  put_args(300u, {36u, 2u, 0u, 0u, 0u});
  put_args(305u, {36u, 6u, 0u, 0u, 0u});
  put_args(310u, {0u, 3u, 0u, 0u, 0u});
  put_args(311u, {36u, 2u, 72u, 0u, 0u});
  put_args(312u, {36u, 1u, 0u, 0xFFFFFFFFu, 0u});
  for (uint32_t i = 0u; i < 4u; ++i) {
    put(301u + i, translate(100.f + static_cast<float>(i), 0.f, 0.f), zero);
    put(306u + i, translate(110.f + static_cast<float>(i), 0.f, 0.f), zero);
  }
  put(313u, translate(120.f, 0.f, 0.f), zero);
  Mat12 identity = {{1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f}};
  Mat12 basis_step = translate(62.f, 0.f, 0.f);
  basis_step.v[0] = 1.f + 1e-6f;
  Mat12 basis_big = translate(80.f, 0.f, 0.f);
  basis_big.v[0] = 1.001f;
  Mat12 nan_prev = translate(70.f, 0.f, 0.f);
  nan_prev.v[0] = std::numeric_limits<float>::quiet_NaN();
  const Mat12 p7_world[8] = {translate(60.f, 0.f, 0.f), translate(61.f, 0.f, 0.f), basis_step, translate(63.00002f, 0.f, 0.f),
                             translate(40000.f, 0.f, 0.f), translate(40000.f, 0.f, 0.f), translate(70.f, 0.f, 0.f), basis_big};
  const Mat12 p7_prev[8] = {identity, zero, translate(62.f, 0.f, 0.f), translate(63.f, 0.f, 0.f),
                            translate(40000.004f, 0.f, 0.f), translate(40000.03f, 0.f, 0.f), nan_prev, translate(80.f, 0.f, 0.f)};
  for (uint32_t i = 0u; i < 8u; ++i) put(200u + i, p7_world[i], p7_prev[i]);
  for (uint32_t i = 0u; i < 5u; ++i) put(400u + i, translate(130.f + 10.f * static_cast<float>(i), 0.f, 0.f), zero);  // G4 P8

  const auto make_buffer = [device](UINT bytes, UINT bind, UINT misc, UINT stride, const void* init) {
    return MakeBuffer(device, bytes, bind, misc, stride, init);
  };
  const auto make_uav = [device](ID3D11Buffer* buffer, UINT elements) {
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.NumElements = elements;
    ID3D11UnorderedAccessView* view = nullptr;
    device->CreateUnorderedAccessView(buffer, &desc, &view);
    return view;
  };
  const std::vector<uint32_t> zero_meta(2u * kCapacity * 4u, 0u);
  const std::vector<uint32_t> zero_words(kSlots, 0u);
  const std::vector<uint32_t> zero_counters(bvh::kGcCounterCount, 0u);
  const std::vector<uint8_t> zero_entries(2u * kCapacity * sizeof(bvh::GcSetEntryGPU), 0u);
  const std::vector<uint8_t> zero_records((kCapacity + 2u * kMovingCapacity) * sizeof(bvh::WorldInstanceGPU), 0u);
  ID3D11Buffer* arena_buffer = make_buffer(kElements * 160u, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS, 0u,
                                           arena.data());
  ID3D11Buffer* draws_buffer = make_buffer(kSlots * sizeof(bvh::GcInstanceDrawGPU), D3D11_BIND_SHADER_RESOURCE,
                                           D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof(bvh::GcInstanceDrawGPU), nullptr);
  ID3D11Buffer* epochs_buffer = make_buffer(kSlots * 4u, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 4u,
                                            zero_words.data());
  ID3D11Buffer* meta_buffer = make_buffer(static_cast<UINT>(zero_meta.size() * 4u), D3D11_BIND_UNORDERED_ACCESS,
                                          D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 4u, zero_meta.data());
  ID3D11Buffer* entries_buffer = make_buffer(static_cast<UINT>(zero_entries.size()), D3D11_BIND_UNORDERED_ACCESS,
                                             D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof(bvh::GcSetEntryGPU), zero_entries.data());
  ID3D11Buffer* records_buffer = make_buffer(static_cast<UINT>(zero_records.size()), D3D11_BIND_UNORDERED_ACCESS,
                                             D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof(bvh::WorldInstanceGPU), zero_records.data());
  ID3D11Buffer* counters_buffer = make_buffer(bvh::kGcCounterCount * 4u, D3D11_BIND_UNORDERED_ACCESS, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 4u,
                                              zero_counters.data());
  ID3D11Buffer* moving_buffer = make_buffer(kSlots * 4u, D3D11_BIND_UNORDERED_ACCESS, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 4u,
                                            zero_words.data());
  ID3D11Buffer* materials_buffer = make_buffer(kSlots * 4u, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 4u,
                                               zero_words.data());
  ID3D11Buffer* cb_buffer = make_buffer(64u, D3D11_BIND_CONSTANT_BUFFER, 0u, 0u, nullptr);
  CHECK(arena_buffer && draws_buffer && epochs_buffer && meta_buffer && entries_buffer && records_buffer && counters_buffer
            && moving_buffer && materials_buffer && cb_buffer,
        "G3 buffers");
  if (arena_buffer == nullptr || draws_buffer == nullptr || epochs_buffer == nullptr || meta_buffer == nullptr
      || entries_buffer == nullptr || records_buffer == nullptr || counters_buffer == nullptr || moving_buffer == nullptr
      || materials_buffer == nullptr || cb_buffer == nullptr) {
    return;
  }
  D3D11_SHADER_RESOURCE_VIEW_DESC arena_desc = {};
  arena_desc.Format = DXGI_FORMAT_R32_TYPELESS;
  arena_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
  arena_desc.BufferEx.NumElements = kElements * 40u;
  arena_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
  ID3D11ShaderResourceView* arena_srv = nullptr;
  device->CreateShaderResourceView(arena_buffer, &arena_desc, &arena_srv);
  ID3D11ShaderResourceView* srvs[4] = {arena_srv, MakeSrv(device, draws_buffer, kSlots), MakeSrv(device, epochs_buffer, kSlots),
                                       MakeSrv(device, materials_buffer, kSlots)};
  ID3D11UnorderedAccessView* uavs[5] = {make_uav(meta_buffer, 2u * kCapacity * 4u), make_uav(entries_buffer, 2u * kCapacity),
                                        make_uav(records_buffer, kCapacity + 2u * kMovingCapacity),
                                        make_uav(counters_buffer, bvh::kGcCounterCount),
                                        make_uav(moving_buffer, kSlots)};

  std::vector<uint32_t> epochs(kSlots, 0u);
  uint32_t current = 0u;
  const auto bits = [](float value) { return std::bit_cast<uint32_t>(value); };
  // One present: uploads the draws, the epochs and zeroed counters, runs the five dispatches, unbinds, reads the counters.
  const auto present = [&](uint32_t frame, const std::vector<bvh::GcInstanceDrawGPU>& draws) {
    const uint32_t src = current * kCapacity;
    const uint32_t dst = (1u - current) * kCapacity;
    const UINT draw_count = static_cast<UINT>(draws.size());
    std::vector<bvh::GcInstanceDrawGPU> padded = draws;  // UpdateSubresource reads the whole buffer (ReShade-style, no box)
    padded.resize(kSlots);
    context->UpdateSubresource(draws_buffer, 0u, nullptr, padded.data(), 0u, 0u);
    context->UpdateSubresource(epochs_buffer, 0u, nullptr, epochs.data(), 0u, 0u);
    context->UpdateSubresource(counters_buffer, 0u, nullptr, zero_counters.data(), 0u, 0u);
    context->CSSetShaderResources(0u, 4u, srvs);
    context->CSSetUnorderedAccessViews(0u, 5u, uavs, nullptr);
    context->CSSetConstantBuffers(0u, 1u, &cb_buffer);
    uint32_t words[16] = {0u, frame, src, dst, draw_count, kCapacity, bits(bvh::kPoolMinScale), bits(bvh::kPoolMaxScale),
                          bits(bvh::kPoolMaxPosition), bvh::kPoolMovingHoldFrames, bvh::kPoolPruneAge, kProbes, kMovingCapacity,
                          0u, 0u, 0u};
    const auto dispatch = [&](uint32_t mode, UINT groups, ID3D11ComputeShader* shader) {
      words[0] = mode;
      context->UpdateSubresource(cb_buffer, 0u, nullptr, words, 0u, 0u);
      context->CSSetShader(shader, nullptr, 0u);
      context->Dispatch(groups, 1u, 1u);
    };
    dispatch(bvh::kGcModeClear, kCapacity / 64u, insert_shader);
    if (draw_count != 0u) dispatch(bvh::kGcModeMark, draw_count, insert_shader);
    dispatch(bvh::kGcModeRehash, kCapacity / 64u, insert_shader);
    if (draw_count != 0u) dispatch(bvh::kGcModeInsert, draw_count, insert_shader);
    dispatch(0u, kCapacity / 64u, compact_shader);
    ID3D11ShaderResourceView* nulls[4] = {nullptr, nullptr, nullptr, nullptr};
    ID3D11UnorderedAccessView* null_uavs[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    context->CSSetShaderResources(0u, 4u, nulls);
    context->CSSetUnorderedAccessViews(0u, 5u, null_uavs, nullptr);
    current = 1u - current;
    const std::vector<uint8_t> raw = Readback(device, context, counters_buffer, bvh::kGcCounterCount * 4u);
    std::vector<uint32_t> counters(bvh::kGcCounterCount, 0u);
    std::memcpy(counters.data(), raw.data(), raw.size());
    std::printf("G3 present %u: set %u inserted %u hits %u kept %u aged %u dropped %u/%u moving %u held %u invalid %u stale %u "
                "overflow %u/%u marked %u sightings %u\n",
                frame, counters[bvh::kGcCounterSet], counters[bvh::kGcCounterInserted], counters[bvh::kGcCounterHits],
                counters[bvh::kGcCounterKept], counters[bvh::kGcCounterAged], counters[bvh::kGcCounterDroppedEpoch],
                counters[bvh::kGcCounterDroppedMoving], counters[bvh::kGcCounterMovingRecords], counters[bvh::kGcCounterMovingHeld],
                counters[bvh::kGcCounterInvalid], counters[bvh::kGcCounterStaleEpoch], counters[bvh::kGcCounterSetOverflow],
                counters[bvh::kGcCounterRehashOverflow], counters[bvh::kGcCounterMarked], counters[bvh::kGcCounterSightings]);
    return counters;
  };
  const auto draw_of = [](uint32_t element, uint32_t count, uint32_t slot, uint32_t epoch, uint32_t pass) {
    bvh::GcInstanceDrawGPU draw;
    draw.arena_float4 = element * 10u;
    draw.count = count;
    draw.slot = slot;
    draw.epoch = epoch;
    draw.pass_bits = pass;
    return draw;
  };
  const auto words_of = [&](ID3D11Buffer* buffer, size_t count) {
    const std::vector<uint8_t> raw = Readback(device, context, buffer, static_cast<UINT>(count));
    return raw;
  };

  // P0 (frame 10): A and B (camera), A (light, a hit), C (moving camera), D (light, held), a zero-scale slice (invalid),
  // E (stale epoch). Seven sightings.
  const std::vector<bvh::GcInstanceDrawGPU> p0 = {draw_of(0u, 2u, 1u, 0u, 1u), draw_of(2u, 1u, 1u, 0u, 8u), draw_of(3u, 1u, 2u, 0u, 1u),
                                                  draw_of(4u, 1u, 2u, 0u, 8u), draw_of(5u, 1u, 3u, 0u, 1u), draw_of(6u, 1u, 4u, 5u, 1u)};
  std::vector<uint32_t> c = present(10u, p0);
  CHECK(c[bvh::kGcCounterSet] == 2u && c[bvh::kGcCounterInserted] == 2u && c[bvh::kGcCounterHits] == 1u, "P0 set 2, inserted 2, hit 1");
  CHECK(c[bvh::kGcCounterSightings] == 7u, "P0 sightings 7 (%u)", c[bvh::kGcCounterSightings]);
  CHECK(c[bvh::kGcCounterMovingRecords] == 2u && c[bvh::kGcCounterMovingHeld] == 1u && c[bvh::kGcCounterMarked] == 1u,
        "P0 moving 2, held 1, marked 1");
  CHECK(c[bvh::kGcCounterInvalid] == 1u && c[bvh::kGcCounterStaleEpoch] == 1u, "P0 invalid 1, stale 1");
  CHECK(c[bvh::kGcCounterSetOverflow] == 0u && c[bvh::kGcCounterRehashOverflow] == 0u, "P0 no overflow");
  {
    const uint32_t out = c[bvh::kGcCounterOut];
    CHECK(out == 4u, "P0 out 4 = set 2 + moving 2 (%u)", out);
    const std::vector<uint8_t> record_bytes = words_of(records_buffer, (kCapacity + 2u * kMovingCapacity) * sizeof(bvh::WorldInstanceGPU));
    const auto* records = reinterpret_cast<const bvh::WorldInstanceGPU*>(record_bytes.data());
    const bvh::WorldInstanceGPU* rec_a = nullptr;
    const bvh::WorldInstanceGPU* rec_b = nullptr;
    const bvh::WorldInstanceGPU* moving[2] = {};
    uint32_t moving_count = 0u;
    for (uint32_t r = 0u; r < out; ++r) {
      if (records[r].visibility[3] > 0.5f) {  // a moving record of this present (G4)
        if (moving_count < 2u) moving[moving_count] = &records[r];
        moving_count += 1u;
        continue;
      }
      if (records[r].world[3] == 1.f) rec_a = &records[r];
      if (records[r].world[3] == 5.f) rec_b = &records[r];
    }
    CHECK(rec_a != nullptr && rec_b != nullptr, "P0 set records for A and B");
    if (rec_a != nullptr && rec_b != nullptr) {
      CHECK(rec_a->header[0] == 1.f && rec_a->header[3] == 1.f && rec_a->visibility[2] == 6.f, "P0 record A: slot 1, flags 6");
      CHECK(rec_b->header[0] == 1.f && rec_b->visibility[2] == 2.f, "P0 record B: slot 1, flags 2");
      CHECK(rec_a->header[2] == 0.f && rec_b->header[2] == 0.f && rec_b->header[3] == 1.f, "P0 A and B: header.z 0, header.w 1");
      CHECK(rec_a->bounds_min[0] == 1.f && rec_a->bounds_max[1] == 2.f && rec_a->bounds_max[2] == 3.f, "P0 record A bounds: translation");
      float matrix[16] = {1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 2.f, 0.f, 0.f, 1.f, 3.f, 0.f, 0.f, 0.f, 1.f};
      float inverse[16] = {};
      falcom_world::ComputeMatrixInverse(matrix, inverse);
      bool inverse_ok = true;
      for (uint32_t k = 0u; k < 12u; ++k) {
        const float got = rec_a->inverse_world[k];
        if (std::fabs(got - inverse[k]) > 1e-4f) inverse_ok = false;
      }
      CHECK(inverse_ok, "P0 record A inverse matches ComputeMatrixInverse");
    }
    CHECK(moving_count == 2u && moving[0] != nullptr && moving[1] != nullptr, "P0 two moving records (%u)", moving_count);
    if (moving_count == 2u && moving[0] != nullptr && moving[1] != nullptr) {
      CHECK(moving[0]->header[0] == 2.f && moving[1]->header[0] == 2.f, "P0 moving records are slot 2");
      CHECK(moving[0]->visibility[2] + moving[1]->visibility[2] == 6.f
                && std::min(moving[0]->visibility[2], moving[1]->visibility[2]) == 2.f,
            "P0 moving record flags are camera and light");
    }
  }
  {
    const std::vector<uint8_t> entry_bytes = words_of(entries_buffer, 2u * kCapacity * sizeof(bvh::GcSetEntryGPU));
    const auto* entries = reinterpret_cast<const bvh::GcSetEntryGPU*>(entry_bytes.data());
    const std::vector<uint8_t> meta_bytes = words_of(meta_buffer, 2u * kCapacity * 4u * sizeof(uint32_t));
    const auto* meta = reinterpret_cast<const uint32_t*>(meta_bytes.data());
    uint32_t found = 0xFFFFFFFFu;
    for (uint32_t i = kCapacity; i < 2u * kCapacity; ++i) {
      if (meta[i * 4u] != 0u && entries[i].slot == 1u && entries[i].world[3] == 1.f) found = i;
    }
    CHECK(found != 0xFFFFFFFFu, "P0 dst entry for A");
    if (found != 0xFFFFFFFFu) {
      uint32_t hash[2] = {};
      const float a_world[12] = {1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 2.f, 0.f, 0.f, 1.f, 3.f};
      bvh::GcPoseHash(1u, a_world, hash);
      CHECK(entries[found].pose_lo == hash[0] && entries[found].pose_hi == hash[1], "P0 A pose words equal GcPoseHash(1, A)");
      CHECK(meta[found * 4u] == (hash[1] | 1u), "P0 A tag");
    }
    const std::vector<uint8_t> moving_bytes = words_of(moving_buffer, kSlots * 4u);
    const auto* moving = reinterpret_cast<const uint32_t*>(moving_bytes.data());
    CHECK(moving[2] == 11u, "P0 moving[2] = frame + 1 (%u)", moving[2]);
  }

  // P1 (frame 72): D is inserted (the hold of slot 2 has expired); A and B are kept.
  c = present(72u, {draw_of(4u, 1u, 2u, 0u, 8u)});
  CHECK(c[bvh::kGcCounterKept] == 2u && c[bvh::kGcCounterInserted] == 1u && c[bvh::kGcCounterSet] == 3u,
        "P1 kept 2, inserted 1, set 3");
  CHECK(c[bvh::kGcCounterOut] == 3u, "P1 out 3 (%u)", c[bvh::kGcCounterOut]);
  {
    const std::vector<uint8_t> record_bytes = words_of(records_buffer, (kCapacity + 2u * kMovingCapacity) * sizeof(bvh::WorldInstanceGPU));
    const auto* records = reinterpret_cast<const bvh::WorldInstanceGPU*>(record_bytes.data());
    CHECK(records[3].header[3] == 0.f && records[4].header[3] == 0.f, "P1 records 3 and 4 are cleared (header.w 0)");
  }
  // P2 (frame 611): A and B are aged out (600 presents); D is kept.
  c = present(611u, {});
  CHECK(c[bvh::kGcCounterAged] == 2u && c[bvh::kGcCounterKept] == 1u && c[bvh::kGcCounterSet] == 1u, "P2 aged 2, kept 1, set 1");
  // P3 (frame 612): slot 2's epoch moves on; D is dropped.
  epochs[2] = 1u;
  c = present(612u, {});
  CHECK(c[bvh::kGcCounterDroppedEpoch] == 1u && c[bvh::kGcCounterSet] == 0u, "P3 dropped 1 (epoch), set 0");
  // P4 (frame 700): a still camera sighting F inserts.
  c = present(700u, {draw_of(7u, 1u, 5u, 0u, 1u)});
  CHECK(c[bvh::kGcCounterInserted] == 1u && c[bvh::kGcCounterSet] == 1u, "P4 F inserted");
  // P5 (frame 701): G moves away from F; F is dropped while slot 5 is held.
  c = present(701u, {draw_of(8u, 1u, 5u, 0u, 1u)});
  CHECK(c[bvh::kGcCounterDroppedMoving] == 1u && c[bvh::kGcCounterMovingRecords] == 1u && c[bvh::kGcCounterSet] == 0u,
        "P5 dropped 1 (moving), moving 1, set 0");
  // P6 (frame 800): 100 instances at one draw; the set has 64 entries and 8 probes.
  c = present(800u, {draw_of(10u, 100u, 6u, 0u, 8u)});
  CHECK(c[bvh::kGcCounterInserted] + c[bvh::kGcCounterSetOverflow] == 100u, "P6 inserted + overflow == 100");
  CHECK(c[bvh::kGcCounterSet] == c[bvh::kGcCounterInserted] && c[bvh::kGcCounterInserted] <= kCapacity, "P6 set equals inserted");
  CHECK(c[bvh::kGcCounterInserted] >= 40u, "P6 inserted >= 40 (%u)", c[bvh::kGcCounterInserted]);
  {
    const std::vector<uint8_t> entry_bytes = words_of(entries_buffer, 2u * kCapacity * sizeof(bvh::GcSetEntryGPU));
    const auto* entries = reinterpret_cast<const bvh::GcSetEntryGPU*>(entry_bytes.data());
    const std::vector<uint8_t> meta_bytes = words_of(meta_buffer, 2u * kCapacity * 4u * sizeof(uint32_t));
    const auto* meta = reinterpret_cast<const uint32_t*>(meta_bytes.data());
    uint32_t live = 0u, mismatched = 0u;
    for (uint32_t i = kCapacity; i < 2u * kCapacity; ++i) {  // P6 dst is the second half
      if (meta[i * 4u] == 0u) continue;
      live += 1u;
      uint32_t hash[2] = {};
      bvh::GcPoseHash(6u, entries[i].world, hash);
      if (entries[i].pose_lo != hash[0] || entries[i].pose_hi != hash[1]) mismatched += 1u;
    }
    CHECK(live == c[bvh::kGcCounterSet] && live > 0u, "P6 live entries %u equal the set (%u)", live, c[bvh::kGcCounterSet]);
    CHECK(mismatched == 0u, "P6 every live entry hashes its own pose with slot 6 (%u mismatched)", mismatched);
  }
  const uint32_t p6_set = c[bvh::kGcCounterSet];
  // P6b (frame 801): slot 6's epoch moves on; the whole P6 set is dropped.
  epochs[6] = 1u;
  c = present(801u, {});
  CHECK(c[bvh::kGcCounterDroppedEpoch] == p6_set && c[bvh::kGcCounterSet] == 0u, "P6b drops the P6 set (%u)", p6_set);
  // P7 (frame 900): eight camera pairs; the moving ones are the PoolSightingMoves reference.
  std::vector<bvh::GcInstanceDrawGPU> p7;
  uint32_t expected_moving = 0u;
  for (uint32_t i = 0u; i < 8u; ++i) {
    p7.push_back(draw_of(200u + i, 1u, 8u + i, 0u, 1u));
    if (bvh::PoolSightingMoves(p7_world[i].v, p7_prev[i].v)) expected_moving += 1u;
  }
  c = present(900u, p7);
  CHECK(c[bvh::kGcCounterMovingRecords] == expected_moving, "P7 moving records %u (PoolSightingMoves %u)", c[bvh::kGcCounterMovingRecords],
        expected_moving);
  CHECK(c[bvh::kGcCounterInserted] == 8u - expected_moving && c[bvh::kGcCounterSet] == 8u - expected_moving,
        "P7 inserted and set are the rest (%u)", c[bvh::kGcCounterInserted]);
  CHECK(c[bvh::kGcCounterInvalid] == 0u && c[bvh::kGcCounterSetOverflow] == 0u, "P7 no invalid, no overflow");

  // P8 (frame 950, G4): slot 3 (alpha with slice 3), slot 4 (alpha, no slice), slot 0 (no CPU mesh), slot 2 (withheld),
  // slot 6 (alpha mesh, no slice). Records by x: 130 valid with slice 4 and flags 2; 140, 160, 170 invalid; 150 valid.
  {
    std::vector<uint32_t> materials(kSlots, 0u);
    materials[0] = bvh::kGcMaterialNoCpuMesh;
    materials[2] = bvh::kGcMaterialNoCpuMesh | bvh::kGcMaterialWithheld;
    materials[3] = 4u;
    materials[4] = 0u;
    materials[6] = bvh::kGcMaterialAlphaMesh;
    context->UpdateSubresource(materials_buffer, 0u, nullptr, materials.data(), 0u, 0u);
  }
  const std::vector<bvh::GcInstanceDrawGPU> p8 = {draw_of(400u, 1u, 3u, 0u, 17u), draw_of(401u, 1u, 4u, 0u, 17u),
                                                  draw_of(402u, 1u, 0u, 0u, 1u), draw_of(403u, 1u, 2u, 1u, 1u),
                                                  draw_of(404u, 1u, 6u, 1u, 1u)};
  c = present(950u, p8);
  CHECK(c[bvh::kGcCounterInserted] == 5u, "P8 inserted 5 (%u)", c[bvh::kGcCounterInserted]);
  // Alpha waiting follows the mesh-alpha rule (flag 16 only for an alpha mesh), matching M5.
  CHECK(c[bvh::kGcCounterAlphaWaiting] == 1u, "P8 alpha waiting 1 (%u)", c[bvh::kGcCounterAlphaWaiting]);
  CHECK(c[bvh::kGcCounterNoCpuMesh] == 2u, "P8 no CPU mesh 2 (%u)", c[bvh::kGcCounterNoCpuMesh]);
  CHECK(c[bvh::kGcCounterOut] == c[bvh::kGcCounterSet], "P8 out equals set, no moving (%u, %u)", c[bvh::kGcCounterOut],
        c[bvh::kGcCounterSet]);
  {
    const uint32_t out = c[bvh::kGcCounterOut];
    const std::vector<uint8_t> record_bytes = words_of(records_buffer, (kCapacity + 2u * kMovingCapacity) * sizeof(bvh::WorldInstanceGPU));
    const auto* records = reinterpret_cast<const bvh::WorldInstanceGPU*>(record_bytes.data());
    const bvh::WorldInstanceGPU* by_x[5] = {};
    for (uint32_t r = 0u; r < out; ++r) {
      for (uint32_t i = 0u; i < 5u; ++i) {
        if (records[r].world[3] == 130.f + 10.f * static_cast<float>(i)) by_x[i] = &records[r];
      }
    }
    CHECK(by_x[0] != nullptr && by_x[0]->header[2] == 4.f && by_x[0]->header[3] == 1.f && by_x[0]->visibility[2] == 2.f,
          "P8 x 130: slice 4, valid, flags 2 (camera; pass alpha dropped on a non-alpha mesh)");
    // A pass-alpha draw on a non-alpha mesh is traced solid, as M5 does.
    CHECK(by_x[1] != nullptr && by_x[1]->header[3] == 1.f, "P8 x 140: valid (solid, non-alpha mesh)");
    CHECK(by_x[2] != nullptr && by_x[2]->header[2] == 0.f && by_x[2]->header[3] == 1.f, "P8 x 150: slice 0, valid");
    CHECK(by_x[3] != nullptr && by_x[3]->header[3] == 0.f, "P8 x 160: invalid (withheld)");
    CHECK(by_x[4] != nullptr && by_x[4]->header[3] == 0.f, "P8 x 170: invalid (alpha mesh, no slice)");
  }

  // P9 (frame 1000): G3b indirect draws on slot 7 (light). The args decide the count: empty, mismatch and truncated each drop
  // or cut one draw; the last draw has vertex offset -1 and matches its args.
  const auto indirect_draw = [&draw_of](uint32_t args_element, uint32_t slice_element, uint32_t window, int32_t vertex_offset) {
    bvh::GcInstanceDrawGPU draw = draw_of(slice_element, window, 7u, 0u, 8u);
    draw.args_float4 = args_element * 10u;
    draw.index_count = 36u;
    draw.first_index = 0u;
    draw.vertex_offset = vertex_offset;
    return draw;
  };
  const std::vector<bvh::GcInstanceDrawGPU> p9 = {indirect_draw(300u, 301u, 4u, 0), indirect_draw(305u, 306u, 4u, 0),
                                                  indirect_draw(310u, 301u, 4u, 0), indirect_draw(311u, 301u, 4u, 0),
                                                  indirect_draw(312u, 313u, 1u, -1)};
  c = present(1000u, p9);
  CHECK(c[bvh::kGcCounterInserted] == 7u && c[bvh::kGcCounterSightings] == 7u && c[bvh::kGcCounterInvalid] == 0u,
        "P9 inserted 7, sightings 7, invalid 0 (%u, %u, %u)", c[bvh::kGcCounterInserted], c[bvh::kGcCounterSightings], c[bvh::kGcCounterInvalid]);
  CHECK(c[bvh::kGcCounterIndirectEmpty] == 1u && c[bvh::kGcCounterIndirectMismatch] == 1u && c[bvh::kGcCounterIndirectTruncated] == 1u,
        "P9 indirect empty 1, mismatch 1, truncated 1 (%u, %u, %u)", c[bvh::kGcCounterIndirectEmpty], c[bvh::kGcCounterIndirectMismatch],
        c[bvh::kGcCounterIndirectTruncated]);

  PrintMessages(queue, "gc instances");
  for (ID3D11Buffer* buffer : {arena_buffer, draws_buffer, epochs_buffer, meta_buffer, entries_buffer, records_buffer, counters_buffer,
                               moving_buffer, materials_buffer, cb_buffer}) {
    buffer->Release();
  }
  arena_srv->Release();
  srvs[1]->Release();
  srvs[2]->Release();
  srvs[3]->Release();
  for (ID3D11UnorderedAccessView* view : uavs) view->Release();
  insert_shader->Release();
  compact_shader->Release();
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

  // (1) Arena copy. Source: 8 structured stride-160 slices, element e holds 1000*e + k.
  const UINT kSourceBytes = 8u * bvh::kPoolInstanceStride;
  std::vector<float> source_floats(kSourceBytes / sizeof(float));
  for (UINT e = 0; e < 8u; ++e) {
    for (UINT k = 0; k < kElementFloat4 * 4u; ++k) source_floats[e * 40u + k] = 1000.f * e + static_cast<float>(k) + 0.5f;
  }
  const uint8_t* source_bytes = reinterpret_cast<const uint8_t*>(source_floats.data());
  ID3D11Buffer* source = MakeBuffer(device, kSourceBytes, D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
                                    bvh::kPoolInstanceStride, source_floats.data());
  CHECK(source != nullptr, "source buffer");
  ID3D11ShaderResourceView* source_srv = MakeSrv(device, source, 8u);
  if (source_srv != nullptr) context->CSSetShaderResources(0u, 1u, &source_srv);  // bound as in production

  const UINT kArenaBytes = kArenaElements * kElementFloat4 * 16u;
  const std::vector<uint8_t> filler(kArenaBytes, 0xEEu);
  const D3D11_BOX box = {160u, 0u, 0u, 800u, 1u, 1u};  // bytes [160, 800) = elements 1..4 of the source
  ID3D11Buffer* arena = MakeBuffer(device, kArenaBytes, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
                                   D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS, 0u, filler.data());
  if (arena == nullptr) {
    std::printf("FAIL raw arena\n");
    return 1;
  }
  if (queue != nullptr) queue->ClearStoredMessages();
  context->CopySubresourceRegion(arena, 0u, 0u, 0u, 0u, source, 0u, &box);
  const std::vector<uint8_t> got = Readback(device, context, arena, kArenaBytes);
  const bool landed = std::memcmp(got.data(), source_bytes + 160u, 640u) == 0;
  std::printf("COPY s160->raw %s\n", landed ? "LANDED" : "NOOP");
  PrintMessages(queue, "raw");
  CHECK(landed, "the copy from the stride-160 source into the raw arena must land");

  // The indirect args (20 bytes from a DRAWINDIRECT_ARGS buffer) copied to byte 320 of the same arena.
  const uint32_t args[5] = {36u, 3u, 0u, 0u, 0u};
  ID3D11Buffer* args_buffer = MakeBuffer(device, 20u, 0u, D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS, 0u, args);
  if (args_buffer == nullptr) {
    std::printf("FAIL indirect args buffer\n");
    return 1;
  }
  if (queue != nullptr) queue->ClearStoredMessages();
  context->CopySubresourceRegion(arena, 0u, 320u, 0u, 0u, args_buffer, 0u, nullptr);
  const std::vector<uint8_t> got_args = Readback(device, context, arena, kArenaBytes);
  const bool args_landed = std::memcmp(got_args.data() + 320u, args, 20u) == 0;
  std::printf("COPY indirect-args->raw %s\n", args_landed ? "LANDED" : "NOOP");
  PrintMessages(queue, "args");
  CHECK(args_landed, "the 20-byte indirect-args copy into the raw arena must land");

  // (2) T2: the real expansion shader over the raw arena (ByteAddressBuffer).
  // The arena content: element i holds the slice of the draw (kDraws) that starts there.
  std::vector<float> arena_floats(kArenaElements * kElementFloat4 * 4u, 0.f);
  auto element_at = [&](uint32_t element) { return arena_floats.data() + element * kElementFloat4 * 4u; };
  for (const TestDraw& draw : kDraws) {
    for (uint32_t i = 0; i < draw.count; ++i) {
      const MatrixCase& mc = kCases[(draw.first_case + i) % kCaseCount];
      float* slice = element_at(draw.slice_element + i);
      std::memcpy(slice, mc.m, sizeof(mc.m));  // rows 0..2 of the 3x4 (xyz, translation in w) are float4 0..2
    }
    if (draw.indirect) {
      float* args = element_at(draw.args_element);
      args[1] = std::bit_cast<float>(draw.args_count);  // InstanceCount
    }
  }
  if (queue != nullptr) queue->ClearStoredMessages();
  context->UpdateSubresource(arena, 0u, nullptr, arena_floats.data(), 0u, 0u);

  FolderInclude include;
  std::string text;
  {
    std::ifstream file(std::string(kShaderDir) + "world_dynamic_expand.cs_5_0.hlsl", std::ios::binary);
    text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  }
  ID3DBlob* code = nullptr;
  ID3DBlob* errors = nullptr;
  hr = D3DCompile(text.data(), text.size(), "world_dynamic_expand.cs_5_0.hlsl", nullptr, &include, "main", "cs_5_0", 0u, 0u, &code, &errors);
  if (FAILED(hr)) {
    std::printf("T2 FAIL: compile world_dynamic_expand.cs_5_0.hlsl hr=0x%08lx %s\n", static_cast<unsigned long>(hr),
                errors != nullptr ? static_cast<const char*>(errors->GetBufferPointer()) : "");
    ++g_failures;
  } else {
    ID3D11ComputeShader* shader = nullptr;
    device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader);

    std::vector<bvh::DynamicDrawGPU> table(kDrawCount);
    for (size_t d = 0; d < kDrawCount; ++d) {
      const TestDraw& draw = kDraws[d];
      bvh::DynamicDrawGPU& entry = table[d];
      entry.info[0] = draw.slice_element * kElementFloat4;
      entry.info[1] = draw.count;
      entry.info[2] = draw.out;
      entry.info[3] = draw.flags;
      entry.extra[0] = draw.indirect ? 1u : 0u;
      entry.extra[1] = draw.args_element * kElementFloat4;
      entry.extra[2] = static_cast<uint32_t>(3u + d);  // mesh id
      entry.extra[3] = 0u;
      for (int k = 0; k < 3; ++k) {
        entry.bbox_min[k] = -1.f * (k + 1);
        entry.bbox_max[k] = 1.f * (k + 1);
      }
      entry.bbox_min[3] = 0.f;
      entry.bbox_max[3] = 0.f;
    }
    ID3D11Buffer* draws = MakeBuffer(device, static_cast<UINT>(kDrawCount * sizeof(bvh::DynamicDrawGPU)),
                                     D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
                                     sizeof(bvh::DynamicDrawGPU), table.data());
    const std::vector<uint8_t> zero_records(kRecords * sizeof(bvh::WorldInstanceGPU), 0u);
    ID3D11Buffer* records = MakeBuffer(device, static_cast<UINT>(zero_records.size()), D3D11_BIND_UNORDERED_ACCESS,
                                       D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof(bvh::WorldInstanceGPU), zero_records.data());
    struct Constants { uint32_t draw_count; float min_scale; float max_scale; float max_position; };
    const Constants constants = {static_cast<uint32_t>(kDrawCount), bvh::kPoolMinScale, bvh::kPoolMaxScale, bvh::kPoolMaxPosition};
    ID3D11Buffer* cb = nullptr;
    {
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth = sizeof(Constants);
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      D3D11_SUBRESOURCE_DATA data = {&constants, 0u, 0u};
      device->CreateBuffer(&desc, &data, &cb);
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC arena_desc = {};
    arena_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    arena_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    arena_desc.BufferEx.NumElements = kArenaBytes / 4u;
    arena_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
    ID3D11ShaderResourceView* arena_srv = nullptr;
    device->CreateShaderResourceView(arena, &arena_desc, &arena_srv);
    ID3D11ShaderResourceView* draws_srv = MakeSrv(device, draws, static_cast<UINT>(kDrawCount));
    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.NumElements = kRecords;
    ID3D11UnorderedAccessView* records_uav = nullptr;
    device->CreateUnorderedAccessView(records, &uav_desc, &records_uav);
    CHECK(shader != nullptr && cb != nullptr && arena_srv != nullptr && draws_srv != nullptr && records_uav != nullptr,
          "expansion objects");

    if (shader != nullptr && records_uav != nullptr && arena_srv != nullptr && draws_srv != nullptr && cb != nullptr) {
      ID3D11ShaderResourceView* srvs[2] = {arena_srv, draws_srv};
      UINT initial_counts = 0u;
      context->CSSetShader(shader, nullptr, 0u);
      context->CSSetShaderResources(0u, 2u, srvs);
      context->CSSetUnorderedAccessViews(0u, 1u, &records_uav, &initial_counts);
      context->CSSetConstantBuffers(0u, 1u, &cb);
      if (queue != nullptr) queue->ClearStoredMessages();
      context->Dispatch(static_cast<UINT>(kDrawCount), 1u, 1u);
      ID3D11ShaderResourceView* nulls[2] = {nullptr, nullptr};
      ID3D11UnorderedAccessView* null_uav = nullptr;
      context->CSSetShaderResources(0u, 2u, nulls);
      context->CSSetUnorderedAccessViews(0u, 1u, &null_uav, &initial_counts);
      PrintMessages(queue, "expand");

      const std::vector<uint8_t> out_bytes = Readback(device, context, records, static_cast<UINT>(zero_records.size()));
      const auto* gpu = reinterpret_cast<const bvh::WorldInstanceGPU*>(out_bytes.data());

      // Reference per record: the draw that writes it (info.z + i for i < info.y), with CheckPoolWorld and the matrix.
      const auto* arena_f4 = reinterpret_cast<const float*>(arena_floats.data());
      uint32_t expected_valid_total = 0u, gpu_valid_total = 0u, valid_match = 0u, inverse_ok = 0u, bounds_ok = 0u, flags_ok = 0u;
      uint32_t checked = 0u, mismatch_total = 0u;
      for (size_t d = 0; d < kDrawCount; ++d) {
        const TestDraw& draw = kDraws[d];
        const bvh::DynamicDrawGPU& entry = table[d];
        uint32_t draw_expected = 0u, draw_gpu = 0u;
        uint32_t count = draw.count;
        if (draw.indirect) count = std::min(draw.count, draw.args_count);
        for (uint32_t i = 0; i < draw.count; ++i) {
          const uint32_t r = draw.out + i;
          const float* slice = arena_f4 + (draw.slice_element + i) * kElementFloat4 * 4u;
          const bool resident = (draw.flags & 1u) != 0u;
          const bool live = resident && i < count;
          float matrix[12] = {};
          if (live) std::memcpy(matrix, slice, sizeof(matrix));
          const float a0[3] = {matrix[0], matrix[1], matrix[2]};
          const float a1[3] = {matrix[4], matrix[5], matrix[6]};
          const float a2[3] = {matrix[8], matrix[9], matrix[10]};
          const float c0[3] = {a1[1] * a2[2] - a1[2] * a2[1], a1[2] * a2[0] - a1[0] * a2[2], a1[0] * a2[1] - a1[1] * a2[0]};
          const float det = a0[0] * c0[0] + a0[1] * c0[1] + a0[2] * c0[2];
          const bool expect_valid = live && std::fabs(det) >= 1e-12f && bvh::CheckPoolWorld(matrix) == bvh::PoolMatrixReject::None;
          const bvh::WorldInstanceGPU& g = gpu[r];
          const bool gpu_valid = g.header[3] > 0.5f;
          ++checked;
          draw_expected += expect_valid ? 1u : 0u;
          draw_gpu += gpu_valid ? 1u : 0u;
          if (gpu_valid == expect_valid) ++valid_match; else ++mismatch_total;
          if (g.header[0] != static_cast<float>(entry.extra[2]) || g.visibility[2] != static_cast<float>(draw.flags & 6u)) ++mismatch_total;
          else ++flags_ok;
          if (expect_valid) {
            float inv[16] = {};
            falcom_world::ComputeMatrixInverse(matrix, inv);
            bool inv_match = true;
            for (int row = 0; row < 3; ++row) {
              for (int col = 0; col < 4; ++col) {
                const float want = inv[row * 4 + col];
                const float got = g.inverse_world[row * 4 + col];
                if (!(std::fabs(got - want) <= 1e-4f * std::max(1.f, std::fabs(want)))) inv_match = false;
              }
            }
            inverse_ok += inv_match ? 1u : 0u;
            if (!inv_match) ++mismatch_total;
            float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
            for (int corner = 0; corner < 8; ++corner) {
              const float p[3] = {(corner & 1) ? entry.bbox_max[0] : entry.bbox_min[0], (corner & 2) ? entry.bbox_max[1] : entry.bbox_min[1],
                                  (corner & 4) ? entry.bbox_max[2] : entry.bbox_min[2]};
              float w[3];
              falcom_world::TransformInst4x3RowDot(matrix, p[0], p[1], p[2], w);
              for (int k = 0; k < 3; ++k) {
                lo[k] = std::min(lo[k], w[k]);
                hi[k] = std::max(hi[k], w[k]);
              }
            }
            bool bounds_match = true;
            for (int k = 0; k < 3; ++k) {
              // 1e-3 m plus the float rounding of the translation (about 2^-22 of its magnitude).
              const float tol_lo = 1e-3f + 2.4e-7f * std::fabs(lo[k]);
              const float tol_hi = 1e-3f + 2.4e-7f * std::fabs(hi[k]);
              if (!(std::fabs(g.bounds_min[k] - lo[k]) <= tol_lo) || !(std::fabs(g.bounds_max[k] - hi[k]) <= tol_hi)) bounds_match = false;
            }
            bounds_ok += bounds_match ? 1u : 0u;
            if (!bounds_match) ++mismatch_total;
          }
        }
        expected_valid_total += draw_expected;
        gpu_valid_total += draw_gpu;
        std::printf("T2 draw %-18s out %2u count %u expected_valid %u gpu_valid %u\n", draw.name, draw.out, count, draw_expected, draw_gpu);
      }
      std::printf("T2 records checked %u | valid exact %u/%u (expected %u, gpu %u) | inverse ok %u | bounds ok %u | flags ok %u\n",
                  checked, valid_match, checked, expected_valid_total, gpu_valid_total, inverse_ok, bounds_ok, flags_ok);
      CHECK(mismatch_total == 0u, "T2: %u mismatches (valid, inverse, bounds or flags)", mismatch_total);
      CHECK(expected_valid_total >= 9u, "T2: enough valid reference records (%u)", expected_valid_total);
    }
  }
  TestGcInstanceShaders(device, context, queue);
  if (queue != nullptr) queue->Release();
  context->Release();
  device->Release();
  if (g_failures == 0) std::printf("ALL PASS test_gpu_p2b\n");
  return g_failures == 0 ? 0 : 1;
}
