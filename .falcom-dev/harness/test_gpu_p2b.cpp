// Native harness: P2b on a real D3D11 WARP device (no game, no ReShade).
// (1) The arena copy: CopySubresourceRegion of a stride-160 structured slice into structured stride-16, raw and
//     structured stride-160 buffers (the arena kinds). (2) The real world_dynamic_expand.cs_5_0.hlsl over a draw
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
  struct Target {
    const char* label;
    UINT stride;
    UINT misc;
    ID3D11Buffer* buffer;
    bool landed;
  };
  Target targets[3] = {
      {"s16", 16u, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, nullptr, false},
      {"raw", 0u, D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS, nullptr, false},
      {"s160", bvh::kPoolInstanceStride, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, nullptr, false},
  };
  const D3D11_BOX box = {160u, 0u, 0u, 800u, 1u, 1u};  // bytes [160, 800) = elements 1..4 of the source
  for (Target& target : targets) {
    target.buffer = MakeBuffer(device, kArenaBytes, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, target.misc, target.stride, filler.data());
    CHECK(target.buffer != nullptr, "destination %s", target.label);
    if (target.buffer == nullptr) continue;
    if (queue != nullptr) queue->ClearStoredMessages();
    context->CopySubresourceRegion(target.buffer, 0u, 0u, 0u, 0u, source, 0u, &box);
    const std::vector<uint8_t> got = Readback(device, context, target.buffer, kArenaBytes);
    target.landed = std::memcmp(got.data(), source_bytes + 160u, 640u) == 0;
    std::printf("COPY s160->%s %s\n", target.label, target.landed ? "LANDED" : "NOOP");
    PrintMessages(queue, target.label);
  }
  const bool landed_b = targets[1].landed;
  const bool landed_c = targets[2].landed;
  CHECK(landed_b || landed_c, "the copy into the raw or the structured stride-160 arena must land");

  // (2) T2: the real expansion shader on the arena copy destination that landed (structured stride 16 when it did,
  // else stride 160; the raw buffer cannot be bound as the shader's StructuredBuffer).
  ID3D11Buffer* arena = targets[0].landed ? targets[0].buffer : (landed_c ? targets[2].buffer : nullptr);
  if (arena == nullptr) {
    std::printf("T2 SKIP: no structured destination landed (the raw buffer is not bound as the shader's StructuredBuffer)\n");
  } else {
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
      ID3D11ShaderResourceView* arena_srv = MakeSrv(device, arena, arena == targets[0].buffer ? kArenaBytes / 16u : kArenaElements);
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
  }
  if (queue != nullptr) queue->Release();
  context->Release();
  device->Release();
  if (g_failures == 0) std::printf("ALL PASS test_gpu_p2b\n");
  return g_failures == 0 ? 0 : 1;
}
