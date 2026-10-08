#pragma once
// Harness stub of src/utils/scene.hpp: the format decoding, layout structs and
// BuildMeshLayout are copied verbatim from the real header (cross_addon
// containers replaced by std ones); SharedData, `shared` and ReadbackBuffer
// are minimal stand-ins (ReadbackBuffer copies through the mock device).
#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <include/reshade.hpp>

#include "./data.hpp"
#include "./hash.hpp"
#include "./log.hpp"
#include "./path.hpp"

namespace renodx::utils::scene {

// 64 MiB per-buffer capture safety limit (not a claim about max Falcom mesh size).
static constexpr uint64_t CAPTURE_SIZE_LIMIT = 64ull * 1024ull * 1024ull;
static constexpr size_t MAX_TRACKED_VERTEX_BUFFERS = 16u;

namespace falcom {
// Future Falcom-specific world-matrix heuristics live here so per-game patterns
// stay behind clearly named functions instead of hardcoded register numbers.
// Stubbed for milestone 1: geometry capture must not wait on transforms.
struct MatrixCandidate {
  uint32_t constant_buffer_slot = 0u;
  uint32_t row_offset = 0u;
  float score = 0.f;
};

static std::vector<MatrixCandidate> FindWorldMatrixCandidates() {
  return {};
}
}  // namespace falcom

// --- Format decoding (table-driven) ----------------------------------------

enum class AttributeKind : uint8_t {
  UNKNOWN = 0,
  FLOAT32,
  FLOAT16,
  UNORM8,
};

struct FormatInfo {
  reshade::api::format format = reshade::api::format::unknown;
  uint32_t byte_size = 0u;
  uint32_t component_count = 0u;
  AttributeKind kind = AttributeKind::UNKNOWN;
};

static const FormatInfo* FindFormatInfo(reshade::api::format format) {
  static const FormatInfo kTable[] = {
      {reshade::api::format::r32g32b32_float, 12u, 3u, AttributeKind::FLOAT32},
      {reshade::api::format::r32g32b32a32_float, 16u, 4u, AttributeKind::FLOAT32},
      {reshade::api::format::r32g32_float, 8u, 2u, AttributeKind::FLOAT32},
      {reshade::api::format::r16g16_float, 4u, 2u, AttributeKind::FLOAT16},
      {reshade::api::format::r16g16b16a16_float, 8u, 4u, AttributeKind::FLOAT16},
      {reshade::api::format::r8g8b8a8_unorm, 4u, 4u, AttributeKind::UNORM8},
  };
  for (const auto& entry : kTable) {
    if (entry.format == format) return &entry;
  }
  return nullptr;
}

static std::string FormatToString(reshade::api::format format) {
  switch (format) {
    case reshade::api::format::r32g32b32_float: return "R32G32B32_FLOAT";
    case reshade::api::format::r32g32b32a32_float: return "R32G32B32A32_FLOAT";
    case reshade::api::format::r32g32_float: return "R32G32_FLOAT";
    case reshade::api::format::r16g16_float: return "R16G16_FLOAT";
    case reshade::api::format::r16g16b16a16_float: return "R16G16B16A16_FLOAT";
    case reshade::api::format::r8g8b8a8_unorm: return "R8G8B8A8_UNORM";
    default: break;
  }
  std::ostringstream s;
  s << "format(" << static_cast<uint32_t>(format) << ")";
  return s.str();
}

static float HalfToFloat(uint16_t bits) {
  const uint32_t sign = (static_cast<uint32_t>(bits) & 0x8000u) << 16u;
  uint32_t exp = (static_cast<uint32_t>(bits) >> 10u) & 0x1Fu;
  uint32_t mantissa = static_cast<uint32_t>(bits) & 0x3FFu;
  uint32_t f32 = 0u;
  if (exp == 0u) {
    if (mantissa == 0u) {
      f32 = sign;
    } else {
      // Subnormal: normalize.
      exp = 1u;
      while ((mantissa & 0x400u) == 0u) {
        mantissa <<= 1u;
        exp++;
      }
      mantissa &= 0x3FFu;
      f32 = sign | ((exp + 112u) << 23u) | (mantissa << 13u);
    }
  } else if (exp == 31u) {
    f32 = sign | 0x7F800000u | (mantissa << 13u);
  } else {
    f32 = sign | ((exp + 112u) << 23u) | (mantissa << 13u);
  }
  float out = 0.f;
  std::memcpy(&out, &f32, sizeof(out));
  return out;
}

// Decodes up to 4 components at `src` into `out` (missing components -> 0, w -> 1).
static bool DecodeAttribute(const uint8_t* src, const FormatInfo& info, float* out) {
  if (src == nullptr || out == nullptr) return false;
  out[0] = 0.f;
  out[1] = 0.f;
  out[2] = 0.f;
  out[3] = 1.f;
  switch (info.kind) {
    case AttributeKind::FLOAT32: {
      const auto* f = reinterpret_cast<const float*>(src);
      for (uint32_t i = 0; i < info.component_count && i < 4u; ++i) {
        out[i] = f[i];
      }
      return true;
    }
    case AttributeKind::FLOAT16: {
      const auto* h = reinterpret_cast<const uint16_t*>(src);
      for (uint32_t i = 0; i < info.component_count && i < 4u; ++i) {
        out[i] = HalfToFloat(h[i]);
      }
      return true;
    }
    case AttributeKind::UNORM8: {
      for (uint32_t i = 0; i < info.component_count && i < 4u; ++i) {
        out[i] = static_cast<float>(src[i]) / 255.f;
      }
      return true;
    }
    default:
      return false;
  }
}

// --- MeshLayout --------------------------------------------------------------

struct MeshLayout {
  int32_t pos_off = -1;
  int32_t norm_off = -1;
  int32_t uv_off = -1;
  int32_t tangent_off = -1;
  reshade::api::format pos_format = reshade::api::format::unknown;
  reshade::api::format norm_format = reshade::api::format::unknown;
  reshade::api::format uv_format = reshade::api::format::unknown;
  reshade::api::format tangent_format = reshade::api::format::unknown;
  uint32_t stride = 0u;
  reshade::api::primitive_topology topology = reshade::api::primitive_topology::undefined;
  uint32_t index_size = 0u;
  bool heuristic = false;
};

// --- Records -----------------------------------------------------------------

struct VertexBufferBinding {
  reshade::api::resource handle = {0u};
  uint64_t offset = 0u;
  uint32_t stride = 0u;
};

struct IndexBufferBinding {
  reshade::api::resource handle = {0u};
  uint64_t offset = 0u;
  uint32_t index_size = 0u;
};

struct DrawRecord {
  reshade::api::device* device = nullptr;
  uint32_t frame_id = 0u;
  uint32_t draw_index = 0u;
  uint8_t method = 0u;  // 0 = draw, 1 = draw_indexed
  uint32_t vertex_count = 0u;
  uint32_t index_count = 0u;
  uint32_t instance_count = 0u;
  uint32_t first_vertex = 0u;
  uint32_t first_index = 0u;
  int32_t vertex_offset = 0;
  uint32_t first_instance = 0u;
  uint32_t vs_hash = 0u;
  uint32_t ps_hash = 0u;
  reshade::api::pipeline pipeline = {0u};
  // D3D11 input-layout handle from the bound input-assembler stage
  // (CreateInputLayout/init_pipeline handle). This is the lookup key for
  // input-layout decoding; `pipeline` above is the shader pipeline.
  reshade::api::pipeline input_layout = {0u};
  reshade::api::primitive_topology topology = reshade::api::primitive_topology::undefined;
  std::vector<VertexBufferBinding> vertex_buffers;
  IndexBufferBinding index_buffer;
  bool has_index_buffer = false;
  reshade::api::resource_view render_target = {0u};
  reshade::api::resource_view depth_stencil = {0u};
  reshade::api::viewport viewport = {};
  bool has_viewport = false;
  reshade::api::pipeline_layout layout = {0u};
  std::vector<reshade::api::descriptor_table> descriptor_tables;
};

struct InputElementCopy {
  std::string semantic;
  uint32_t semantic_index = 0u;
  reshade::api::format format = reshade::api::format::unknown;
  uint32_t buffer_binding = 0u;
  uint32_t offset = 0u;
  uint32_t stride = 0u;
};

struct InputLayoutInfo {
  std::vector<InputElementCopy> elements;
  reshade::api::primitive_topology topology = reshade::api::primitive_topology::undefined;
  bool has_topology = false;
};

// Stand-in for cross_addon::parallel_node_hash_map: the calls the world code
// and tests make.
template <typename K, typename V>
struct StubLockedMap {
  mutable std::shared_mutex mutex;
  std::unordered_map<K, V> map;
  template <typename F>
  bool if_contains(const K& key, F&& f) const {
    std::shared_lock lock(mutex);
    auto it = map.find(key);
    if (it == map.end()) return false;
    f(*it);
    return true;
  }
  V& operator[](const K& key) {
    std::unique_lock lock(mutex);
    return map[key];
  }
  void insert_or_assign(const K& key, V value) {
    std::unique_lock lock(mutex);
    map.insert_or_assign(key, std::move(value));
  }
  void erase(const K& key) {
    std::unique_lock lock(mutex);
    map.erase(key);
  }
  size_t size() const {
    std::shared_lock lock(mutex);
    return map.size();
  }
};

struct SharedData {
  StubLockedMap<uint64_t, InputLayoutInfo> input_layouts;
};

struct StubShared {
  SharedData storage;
  SharedData* data = &storage;
};

inline StubShared shared;

inline int g_readback_calls = 0;

// Copies through the mock command list into a mock staging buffer, then maps it.
static bool ReadbackBuffer(
    reshade::api::device* device,
    reshade::api::command_queue* queue,
    reshade::api::resource source,
    uint64_t source_offset,
    uint64_t size,
    std::vector<uint8_t>* out_bytes) {
  ++g_readback_calls;
  if (device == nullptr || queue == nullptr || source.handle == 0u || out_bytes == nullptr) return false;
  if (size == 0u || size > CAPTURE_SIZE_LIMIT) return false;
  reshade::api::resource staging = {0u};
  const reshade::api::resource_desc desc(size, reshade::api::memory_heap::gpu_to_cpu, reshade::api::resource_usage::copy_dest);
  if (!device->create_resource(desc, nullptr, reshade::api::resource_usage::copy_dest, &staging)) return false;
  auto* cmd_list = queue->get_immediate_command_list();
  if (cmd_list == nullptr) {
    device->destroy_resource(staging);
    return false;
  }
  cmd_list->copy_buffer_region(source, source_offset, staging, 0u, size);
  queue->flush_immediate_command_list();
  queue->wait_idle();
  void* mapped = nullptr;
  if (!device->map_buffer_region(staging, 0u, size, reshade::api::map_access::read_only, &mapped) || mapped == nullptr) {
    device->destroy_resource(staging);
    return false;
  }
  out_bytes->assign(static_cast<const uint8_t*>(mapped), static_cast<const uint8_t*>(mapped) + static_cast<size_t>(size));
  device->unmap_buffer_region(staging);
  device->destroy_resource(staging);
  return true;
}

// --- MeshLayout ---------------------------------------------------------------------

static bool BuildMeshLayout(const InputLayoutInfo& layout_info, uint32_t stride, uint32_t index_size, MeshLayout* out) {
  if (out == nullptr) return false;
  MeshLayout layout;
  layout.stride = stride;
  layout.index_size = index_size;
  if (layout_info.has_topology) {
    layout.topology = layout_info.topology;
  }

  for (const auto& element : layout_info.elements) {
    if (element.buffer_binding != 0u) continue;  // single-stream (slot 0) for milestone 1
    std::string semantic = element.semantic.c_str();
    for (auto& c : semantic) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
    const FormatInfo* info = FindFormatInfo(element.format);
    if (info == nullptr) continue;  // unsupported format: skip element, do not fail yet
    const bool is_position = (semantic == "POSITION" || semantic == "SV_POSITION" || semantic == "POSITIONT");
    const bool is_normal = (semantic == "NORMAL");
    const bool is_tangent = (semantic == "TANGENT" || semantic == "BINORMAL");
    const bool is_uv = (semantic == "TEXCOORD");
    if (is_position && layout.pos_off < 0) {
      layout.pos_off = static_cast<int32_t>(element.offset);
      layout.pos_format = element.format;
    } else if (is_normal && layout.norm_off < 0) {
      layout.norm_off = static_cast<int32_t>(element.offset);
      layout.norm_format = element.format;
    } else if (is_tangent && layout.tangent_off < 0) {
      layout.tangent_off = static_cast<int32_t>(element.offset);
      layout.tangent_format = element.format;
    } else if (is_uv && layout.uv_off < 0 && element.semantic_index == 0u) {
      // First TEXCOORD set feeds the initial OBJ export.
      layout.uv_off = static_cast<int32_t>(element.offset);
      layout.uv_format = element.format;
    }
  }

  if (layout.pos_off < 0) {
    // Controlled fallback: slot-0 stride-sized float3 at offset 0, never silent.
    layout.pos_off = 0;
    layout.pos_format = reshade::api::format::r32g32b32_float;
    layout.heuristic = true;
    renodx::utils::log::w("beyondthefalcom::scene: no POSITION semantic found; using heuristic float3 @ 0.");
  }
  *out = layout;
  return true;
}

}  // namespace renodx::utils::scene
