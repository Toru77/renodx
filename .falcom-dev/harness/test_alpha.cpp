// Native harness, Stage B and C: the alpha atlas slice table and texel packing, and the pool's
// alpha rules: classification of the real alpha pixel shaders, the material read (fails closed),
// per-key and per-mesh conflicts, clearing, the admission refusals, and the default path (toggle
// OFF) through OnPoolScanDraw. The GPU stage (source copies, atlas, blit, freeing) runs on the mock
// device through UpdateLiveBvh. The trace is not exercised here.
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
#include <algorithm>
#include <map>
#include <mutex>
#include <span>
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "mesh_fixture.hpp"
#include "src/games/falcomengine-plus/world/bvh/alpha_atlas.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_pool.hpp"
#define __world_alpha_blit_EMBED_FILE
inline constexpr std::uint8_t __world_alpha_blit_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_alpha_blit{__world_alpha_blit_base};
#include "src/games/falcomengine-plus/world/bvh/bvh_live.hpp"

using namespace reshade::api;
namespace bvh = falcom_world;
namespace pool = falcom_world::bvh;
namespace contract = falcom_world::contract;
// Reference layout of the three contract-layout shaders (alphaTestThreshold_g is packoffset(c7)). Not read at runtime:
// the pool reads the offset each shader reports (ps traits alpha_offset).
static constexpr uint32_t kReferenceThresholdOffset = 112u;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Res { resource_desc desc; std::vector<uint8_t> bytes; };
static int g_bad_view_calls = 0;  // get_resource_from_view / get_resource_view_desc on a handle that is not a registered view
struct Device : mock::DeviceBase {
  std::map<uint64_t, Res> res;
  std::map<uint64_t, uint64_t> view_of;  // view handle -> resource handle
  std::map<uint64_t, resource_view_desc> view_desc;  // view handle -> the desc it was created with
  uint64_t next = 0x1000;
  int bad_copies = 0;
  int creates = 0;  // create_resource calls
  int texture_creates = 0;  // create_resource calls for 2D textures (the alpha proxies)
  std::map<uint64_t, uint64_t> uav_of_table;  // descriptor table -> the UAV in its binding (0 when null)
  void update_descriptor_tables(uint32_t count, const descriptor_table_update *updates) override {
    for (uint32_t i = 0; i < count; ++i) {
      if (updates[i].type == descriptor_type::unordered_access_view) {
        uav_of_table[updates[i].table.handle] = static_cast<const resource_view *>(updates[i].descriptors)[0].handle;
      }
    }
  }
  device_api get_api() const override { return device_api::d3d11; }
  bool fail_texture_create = false;  // create_resource fails for 2D textures (the copy-failed refusal)
  bool create_resource(const resource_desc &desc, const subresource_data *initial_data, resource_usage, resource *out, void ** = nullptr) override {
    if (fail_texture_create && desc.type == resource_type::texture_2d) return false;
    Res r; r.desc = desc;
    if (desc.type == resource_type::buffer) {
      r.bytes.assign(desc.buffer.size, 0);
      if (initial_data && initial_data->data) std::memcpy(r.bytes.data(), initial_data->data, desc.buffer.size);
    }
    creates += 1;
    if (desc.type == resource_type::texture_2d) texture_creates += 1;
    out->handle = next; next += 0x100; res[out->handle] = std::move(r);
    return true;
  }
  void destroy_resource(resource r) override { res.erase(r.handle); }
  bool create_resource_view(resource r, resource_usage, const resource_view_desc &desc, resource_view *out) override {
    out->handle = next; next += 0x100; view_of[out->handle] = r.handle; view_desc[out->handle] = desc; return true;
  }
  void destroy_resource_view(resource_view v) override { view_of.erase(v.handle); view_desc.erase(v.handle); }
  resource get_resource_from_view(resource_view v) const override {
    const auto it = view_of.find(v.handle);
    if (it == view_of.end()) g_bad_view_calls += 1;
    return resource{it == view_of.end() ? 0u : it->second};
  }
  resource_view_desc get_resource_view_desc(resource_view v) const override {
    const auto it = view_desc.find(v.handle);
    if (it == view_desc.end()) g_bad_view_calls += 1;
    return it == view_desc.end() ? resource_view_desc{} : it->second;
  }
  bool create_pipeline_layout(uint32_t, const pipeline_layout_param *, pipeline_layout *out) override { out->handle = next; next += 0x100; return true; }
  void destroy_pipeline_layout(pipeline_layout) override {}
  bool create_pipeline(pipeline_layout, uint32_t, const pipeline_subobject *, pipeline *out) override { out->handle = next; next += 0x100; return true; }
  void destroy_pipeline(pipeline) override {}
  bool allocate_descriptor_tables(uint32_t count, pipeline_layout, uint32_t, descriptor_table *out) override {
    for (uint32_t i = 0; i < count; ++i) out[i].handle = next++;
    return true;
  }
  void free_descriptor_tables(uint32_t, const descriptor_table *) override {}
  bool create_sampler(const sampler_desc &, sampler *out) override { out->handle = next; next += 0x100; return true; }
  void destroy_sampler(sampler) override {}
  resource_desc get_resource_desc(resource r) const override { auto it = res.find(r.handle); return it == res.end() ? resource_desc{} : it->second.desc; }
  bool map_buffer_region(resource r, uint64_t offset, uint64_t size, map_access, void **out) override {
    auto it = res.find(r.handle); if (it == res.end() || offset + size > it->second.bytes.size()) return false;
    *out = it->second.bytes.data() + offset; return true;
  }
  void unmap_buffer_region(resource) override {}
};
struct CmdList : mock::CommandListBase {
  Device* dev = nullptr;
  int dispatches = 0;
  int copies = 0;
  uint64_t u0 = 0u;  // the UAV at u0 after the last bind of compute set 2 (0 = null)
  void bind_descriptor_tables(shader_stage, pipeline_layout, uint32_t first, uint32_t count, const descriptor_table *tables) override {
    for (uint32_t i = 0; i < count; ++i) {
      if (first + i == 2u) u0 = dev->uav_of_table[tables[i].handle];
    }
  }
  device *get_device() override { return dev; }
  void dispatch(uint32_t, uint32_t, uint32_t) override { dispatches += 1; }
  void copy_texture_region(resource, uint32_t, const subresource_box *, resource, uint32_t, const subresource_box *, filter_mode = filter_mode::min_mag_mip_point) override { copies += 1; }
  void copy_buffer_region(resource src, uint64_t so, resource dst, uint64_t dof, uint64_t size) override {
    auto s = dev->res.find(src.handle), d = dev->res.find(dst.handle);
    if (s == dev->res.end() || d == dev->res.end() || so + size > s->second.bytes.size() || dof + size > d->second.bytes.size()) { dev->bad_copies++; return; }
    std::memcpy(d->second.bytes.data() + dof, s->second.bytes.data() + so, size);
  }
};
struct Queue : mock::QueueBase {
  Device* dev = nullptr; CmdList* cl = nullptr;
  device *get_device() override { return dev; }
  command_list *get_immediate_command_list() override { return cl; }
};

static std::vector<uint8_t> ReadFile(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static void Classify(Device* dev, uint64_t handle, const std::string& file, bool vertex) {
  auto code = ReadFile(std::string(FALCOM_BYTECODE_DIR) + file);
  if (code.empty()) { std::printf("missing %s\n", file.c_str()); std::exit(2); }
  shader_desc desc = {}; desc.code = code.data(); desc.code_size = code.size();
  pipeline_subobject sub = {vertex ? pipeline_subobject_type::vertex_shader : pipeline_subobject_type::pixel_shader, 1, &desc};
  contract::OnInitPipelineClassify(dev, {0}, 1, &sub, pipeline{handle});
}

static uint64_t SourceView(Device& dev, resource* texture);

// Alpha material with the given texture, threshold and first scroll; swizzle as given.
static pool::PoolAlphaMaterial Mat(uint64_t texture, float threshold, float scroll0 = 0.1f, uint32_t swizzle = 3u) {
  pool::PoolAlphaMaterial m;
  m.view = texture; m.threshold = threshold; m.scroll[0] = scroll0; m.scroll[1] = 0.2f; m.swizzle = swizzle;
  return m;
}

// Box mesh of half-width size, with UVs when asked for.
static pool::PoolDecodedMesh BoxMesh(float size, bool with_uv) {
  const fixture::TestMesh box = fixture::Box(size);
  pool::PoolDecodedMesh mesh;
  mesh.positions = box.positions;
  mesh.triangles = box.triangles;
  mesh.bbox_min = {-size, -1.f, -1.f};
  mesh.bbox_max = {size, 1.f, 1.f};
  if (with_uv) {
    for (const auto& p : box.positions) mesh.uvs.push_back({p[1] * 0.5f + 0.5f, p[2] * 0.5f + 0.5f});
  }
  return mesh;
}

// Captures a box mesh under a draw key (ApplyPoolMesh, as the mesh copy would).
static void Capture(uint64_t key, float size, bool with_uv) {
  pool::PoolDecodedMesh mesh = BoxMesh(size, with_uv);
  pool::ApplyPoolMesh(key, 1u, mesh);
}

static uint32_t MeshOf(uint64_t key) {
  const auto it = pool::g_pool.mesh_by_key.find(key);
  return it == pool::g_pool.mesh_by_key.end() ? UINT32_MAX : it->second;
}

// A constant buffer mirror (160 bytes tracked) holding the first `size` bytes of `data`.
static resource TrackedCb(Device& dev, const uint8_t* data, uint64_t size) {
  resource r;
  const resource_desc desc(160, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(desc, nullptr, resource_usage::general, &r);
  falcom_world::OnInitResourceCbTracker(&dev, desc, nullptr, resource_usage::general, r);
  falcom_world::OnUpdateBufferRegionCbTracker(&dev, data, r, 0, size);
  return r;
}

static size_t LogCount(const char* text) {
  std::lock_guard<std::mutex> lock(renodx::utils::log::g_lines_mutex);
  size_t count = 0u;
  for (const auto& line : renodx::utils::log::g_lines) count += line.second.find(text) != std::string::npos ? 1u : 0u;
  return count;
}

static void TestClassifier(Device* dev) {
  struct Case { const char* file; uint64_t handle; bool trait; int32_t offset; };
  const std::array<Case, 6> alpha = {{
      {"0x137F316A", 0xC101, true, 112}, {"0x81F5709F", 0xC102, true, 112}, {"0xAA835FE0", 0xC103, true, 112},
      {"0x049B0385", 0xC201, true, 128}, {"0x2807FFC9", 0xC202, true, 120}, {"0x2DADE2B8", 0xC203, true, 128}}};
  for (const Case& c : alpha) {
    Classify(dev, c.handle, std::string(c.file) + ".ps.cso", false);
    const contract::ShaderTraits traits = contract::LookupPixelTraits(c.handle);
    CHECK(traits.cls == static_cast<uint8_t>(contract::PsClass::AlphaTested), "%s is alpha tested", c.file);
    CHECK(((traits.flags & contract::kTraitAlphaMaterial) != 0u) == c.trait, "%s kTraitAlphaMaterial %d", c.file, c.trait);
    const auto reason = static_cast<contract::AlphaMaterialReason>(traits.alpha_reason);
    CHECK((reason == contract::AlphaMaterialReason::Ok) == c.trait, "%s alpha reason %s", c.file, contract::AlphaMaterialReasonName(reason));
    CHECK(traits.alpha_offset == c.offset, "%s threshold offset %d (want %d)", c.file, traits.alpha_offset, c.offset);
    std::printf("alpha reason %s: %s (threshold offset %d)\n", c.file, contract::AlphaMaterialReasonName(reason), traits.alpha_offset);
  }
  Classify(dev, 0xC300, "0x2162672F.ps.cso", false);
  CHECK(contract::LookupPixelClass(0xC300) == contract::PsClass::Opaque, "0x2162672F is opaque");
  CHECK((contract::LookupPixelTraits(0xC300).flags & contract::kTraitAlphaMaterial) == 0u, "opaque has no alpha trait");
}

static void TestNoteMaterial() {
  pool::ResetWorldPool();
  pool::g_pool.alpha_foliage.store(true);
  std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
  auto& keys = pool::g_pool.alpha_keys;
  const pool::PoolAlphaMaterial first = Mat(0x100, 0.5f);
  pool::NotePoolAlphaMaterial(0x9001, first);
  pool::NotePoolAlphaMaterial(0x9001, first);
  CHECK(!keys.at(0x9001).conflict && pool::g_pool.stats.alpha_conflicts == 0u, "same material: no conflict");
  pool::NotePoolAlphaMaterial(0x9002, first);
  pool::NotePoolAlphaMaterial(0x9002, Mat(0x100, 0.6f));
  CHECK(keys.at(0x9002).conflict, "threshold conflicts");
  CHECK(keys.at(0x9002).material.threshold == 0.5f, "first wins (threshold)");
  pool::NotePoolAlphaMaterial(0x9003, first);
  pool::NotePoolAlphaMaterial(0x9003, Mat(0x100, 0.5f, 0.7f));
  CHECK(keys.at(0x9003).conflict, "scroll conflicts");
  pool::NotePoolAlphaMaterial(0x9004, first);
  pool::NotePoolAlphaMaterial(0x9004, Mat(0x101, 0.5f));
  CHECK(keys.at(0x9004).conflict, "texture conflicts");
  pool::NotePoolAlphaMaterial(0x9005, first);
  pool::NotePoolAlphaMaterial(0x9005, Mat(0x100, 0.5f, 0.1f, 4u));
  CHECK(keys.at(0x9005).conflict, "swizzle conflicts");
  pool::NotePoolAlphaMaterial(0x9006, pool::PoolAlphaMaterial{});  // unreadable first
  pool::NotePoolAlphaMaterial(0x9006, first);
  CHECK(!keys.at(0x9006).conflict && keys.at(0x9006).material.view == 0x100u, "unreadable then readable upgrades");
  pool::NotePoolAlphaMaterial(0x9007, pool::PoolAlphaMaterial{});
  pool::NotePoolAlphaMaterial(0x9007, Mat(0x101, 0.9f));
  CHECK(!keys.at(0x9007).conflict && keys.at(0x9007).material.view == 0x101u, "unreadable never conflicts");
  CHECK(pool::g_pool.stats.alpha_conflicts == 4u, "four conflicting keys: %llu", (unsigned long long)pool::g_pool.stats.alpha_conflicts);
  pool::g_pool.alpha_foliage.store(false);
  Capture(0x9008, 16.f, true);
  pool::NotePoolAlphaMaterial(0x9008, first);  // recorded while ON, resolved while OFF
  CHECK(keys.count(0x9008) != 0u && pool::g_pool.meshes[MeshOf(0x9008)].alpha, "a Note from an ON record still flags while OFF");
  pool::g_pool.alpha_foliage.store(true);
}

static void TestReadDraw(Device& dev, CmdList& cl) {
  const uint32_t threshold_offset = 112u;
  std::array<uint8_t, 160> bytes = {};
  const float threshold = 0.5f, scroll[2] = {0.25f, 0.75f};
  std::memcpy(bytes.data() + threshold_offset, &threshold, sizeof(float));
  std::memcpy(bytes.data() + contract::kAlphaUvScrollOffset, scroll, sizeof(scroll));
  const uint32_t swizzle_value = 3u;
  std::array<uint8_t, 16> swizzle_bytes = {};
  std::memcpy(swizzle_bytes.data(), &swizzle_value, sizeof(swizzle_value));

  const resource material = TrackedCb(dev, bytes.data(), 160);
  const resource swizzle = TrackedCb(dev, swizzle_bytes.data(), 16);
  const resource partial = TrackedCb(dev, bytes.data(), 100);  // threshold (112..116) not held
  falcom_world::WorldCommandListData cl_data;
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  cl_data.ps_cb[contract::kAlphaSwizzleSlot] = swizzle;
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = 0x400u;
  cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = 0x500u;

  pool::PoolAlphaMaterial out;
  CHECK(pool::ReadPoolAlphaDraw(&cl, cl_data, 112u, &out), "full 160-byte material reads");
  CHECK(out.threshold == threshold && out.scroll[0] == 0.25f && out.scroll[1] == 0.75f, "threshold and scroll decoded");
  CHECK(out.swizzle == 1u && out.view == 0x500u, "swizzle (bit 0) and texture decoded");
  std::array<uint8_t, 160> bytes_128 = {};
  const float threshold_128 = 0.25f;
  std::memcpy(bytes_128.data() + 128u, &threshold_128, sizeof(float));
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = TrackedCb(dev, bytes_128.data(), 160);
  CHECK(pool::ReadPoolAlphaDraw(&cl, cl_data, 128u, &out) && out.threshold == threshold_128, "threshold read at offset 128: %f", out.threshold);
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;

  cl_data.ps_cb[contract::kAlphaMaterialSlot] = partial;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, 112u, &out) && out.view == 0u, "partial cb mirror fails closed");
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  cl_data.ps_cb[contract::kAlphaMaterialSlot].handle = 0u;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, 112u, &out) && out.view == 0u, "unbound b5 fails closed");
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  cl_data.ps_cb_offset[contract::kAlphaMaterialSlot] = 16u;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, 112u, &out) && out.view == 0u, "nonzero b5 offset fails closed");
  cl_data.ps_cb_offset[contract::kAlphaMaterialSlot] = 0u;
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = 0u;
  cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = 0u;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, 112u, &out) && out.view == 0u, "null t0 fails closed");
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = 0x400u;
  cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = 0x500u;
  cl_data.ps_cb_offset[contract::kAlphaSwizzleSlot] = 4u;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, 112u, &out) && out.view == 0u, "bad swizzle offset fails closed");
  cl_data.ps_cb_offset[contract::kAlphaSwizzleSlot] = 0u;
  {
    std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
    CHECK(pool::g_pool.stats.alpha_cb_unavailable == 5u, "five unreadable draws counted: %llu", (unsigned long long)pool::g_pool.stats.alpha_cb_unavailable);
  }
}

// The shader reads only bit 0 of swizzle_flags_g; other bits must not make a conflict.
static void TestSwizzleBits(Device& dev, CmdList& cl) {
  pool::ResetWorldPool();
  pool::g_pool.alpha_foliage.store(true);
  std::array<uint8_t, 160> bytes = {};
  const float threshold = 0.5f;
  std::memcpy(bytes.data() + kReferenceThresholdOffset, &threshold, sizeof(float));
  falcom_world::WorldCommandListData cl_data;
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = TrackedCb(dev, bytes.data(), 160);
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = 0x400u;
  cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = 0x500u;
  const auto read = [&](uint32_t value) {
    cl_data.ps_cb[contract::kAlphaSwizzleSlot] = TrackedCb(dev, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    pool::PoolAlphaMaterial out;
    CHECK(pool::ReadPoolAlphaDraw(&cl, cl_data, 112u, &out), "swizzle %u reads", value);
    CHECK(out.swizzle <= 1u, "swizzle %u stores bit 0 only: %u", value, out.swizzle);
    return out;
  };
  const uint32_t high_bits[4] = {0u, 2u, 4u, 6u};
  for (uint32_t value : high_bits) {
    const pool::PoolAlphaMaterial out = read(value);
    std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
    pool::NotePoolAlphaMaterial(0xA001, out);
  }
  {
    std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
    CHECK(!pool::g_pool.alpha_keys.at(0xA001).conflict, "swizzle 0/2/4/6 on one key: no conflict");
  }
  const pool::PoolAlphaMaterial bit0 = read(0u);
  const pool::PoolAlphaMaterial bit0_set = read(1u);
  std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
  pool::NotePoolAlphaMaterial(0xA002, bit0);
  pool::NotePoolAlphaMaterial(0xA002, bit0_set);
  CHECK(pool::g_pool.alpha_keys.at(0xA002).conflict, "swizzle bit 0 differing (0 vs 1): conflict");
}

static void TestSwitch() {
  pool::ResetWorldPool();
  pool::g_pool.alpha_foliage.store(true);
  std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
  const uint64_t key = 0x6001;
  Capture(key, 6.f, false);
  const uint32_t mesh_id = MeshOf(key);
  pool::NotePoolAlphaMaterial(key, Mat(0x100, 0.5f));
  CHECK(pool::g_pool.meshes[mesh_id].alpha, "flagged while ON");
  auto push = [&]() {
    pool::WorldInstance instance;
    instance.mesh_id = mesh_id;
    instance.mesh_key = key;
    instance.id = pool::g_pool.next_instance_id++;
    pool::g_pool.instances.push_back(instance);
  };
  push();
  pool::g_pool.alpha_foliage.store(false);
  pool::ApplyPoolAlphaSwitch();
  CHECK(pool::g_pool.stats.alpha_removed == 1u && pool::g_pool.instances.empty(), "OFF removes once: %llu", (unsigned long long)pool::g_pool.stats.alpha_removed);
  push();
  pool::ApplyPoolAlphaSwitch();
  CHECK(pool::g_pool.stats.alpha_removed == 1u && pool::g_pool.instances.size() == 1u, "a second OFF pass does nothing");
  pool::g_pool.alpha_foliage.store(true);
  pool::ApplyPoolAlphaSwitch();
  pool::g_pool.alpha_foliage.store(false);
  pool::ApplyPoolAlphaSwitch();
  CHECK(pool::g_pool.stats.alpha_removed == 2u && pool::g_pool.instances.empty(), "ON then OFF removes once more: %llu", (unsigned long long)pool::g_pool.stats.alpha_removed);
  pool::g_pool.alpha_foliage.store(true);
}

static void TestDecodeUv() {
  const uint32_t stride = 20u;
  std::vector<uint8_t> bytes(60u, 0u);
  const float positions[3][3] = {{0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}};
  for (uint32_t v = 0; v < 3u; ++v) {
    std::memcpy(bytes.data() + v * stride, positions[v], 12);
    const float uv[2] = {0.1f * v, 0.2f * v};
    std::memcpy(bytes.data() + v * stride + 12, uv, 8);
  }
  const std::vector<uint32_t> indices = {0u, 1u, 2u};
  const auto decode = [&](const std::vector<uint8_t>& data, uint64_t size, int32_t uv_offset, pool::PoolDecodedMesh* mesh) {
    return pool::DecodePoolMeshVertices(data.data(), size, stride, 0, reshade::api::format::r32g32b32_float, uv_offset,
                                        reshade::api::format::r32g32_float, indices, 0u, 2u, mesh);
  };
  pool::PoolDecodedMesh mesh;
  const char* error = decode(bytes, 60u, 12, &mesh);
  CHECK(error == nullptr && mesh.uvs.size() == 3u, "uv ok: %s uvs %zu", error ? error : "-", mesh.uvs.size());
  error = decode(bytes, 60u, 16, &mesh);
  CHECK(error == nullptr && mesh.positions.size() == 3u && mesh.uvs.empty(), "uv past stride: positions kept, uvs dropped");
  error = decode(bytes, 52u, 12, &mesh);
  CHECK(error == nullptr && mesh.positions.size() == 3u && mesh.uvs.empty(), "truncated copy with exact-size buffer: uvs dropped");
  error = decode(bytes, 60u, -1, &mesh);
  CHECK(error == nullptr && mesh.uvs.empty(), "uv_offset -1: no uvs");
  std::vector<uint8_t> nan_bytes = bytes;
  const float nan = std::numeric_limits<float>::quiet_NaN();
  std::memcpy(nan_bytes.data() + stride + 12, &nan, sizeof(float));
  error = decode(nan_bytes, 60u, 12, &mesh);
  CHECK(error == nullptr && mesh.positions.size() == 3u && mesh.uvs.empty(), "NaN uv: uvs dropped");
}

static void TestGate(Device& dev) {
  pool::ResetWorldPool();
  const uint64_t kWindVs = 0xD1, kBillboardVs = 0xD3, kRigidVs = 0xD5, kAlphaPs = 0xD4, kOpaquePs = 0xD6;
  Classify(&dev, kWindVs, "0x9FF8E4BE.vs.cso", true);
  Classify(&dev, kBillboardVs, "0x4E30FAF1.vs.cso", true);
  Classify(&dev, kRigidVs, "0x095017A3.vs.cso", true);
  Classify(&dev, kAlphaPs, "0x137F316A.ps.cso", false);
  Classify(&dev, kOpaquePs, "0x2162672F.ps.cso", false);
  falcom_world::DrawRecord draw;
  draw.method = 1; draw.has_index_buffer = true; draw.vb = {0x1111}; draw.vb_stride = fixture::kStride; draw.ib = {0x2222};
  draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {fixture::kLayout};
  draw.index_size = 2; draw.dsv = {0x77}; draw.index_count = 36; draw.first_index = 0; draw.instance_count = 1;
  draw.depth_enable = true; draw.depth_write = true; draw.topology = primitive_topology::triangle_list;
  auto gate = [&](uint64_t vs, uint64_t ps) {
    draw.vs_pipeline = vs; draw.ps_pipeline = ps; draw.vs_hash = static_cast<uint32_t>(vs);
    return pool::GatePoolDraw(draw, true);
  };
  for (const bool on : {false, true}) {
    pool::g_pool.alpha_foliage.store(on);
    pool::g_pool.alpha_wind_opaque.store(false);  // the WindOpaque refusal is the d2 OFF case (on by default)
    const auto wind_alpha = gate(kWindVs, kAlphaPs);
    const auto wind_opaque = gate(kWindVs, kOpaquePs);
    const auto billboard_alpha = gate(kBillboardVs, kAlphaPs);
    const auto billboard_opaque = gate(kBillboardVs, kOpaquePs);
    const auto rigid_alpha = gate(kRigidVs, kAlphaPs);
    CHECK(wind_alpha.vs_class == contract::VsClass::Wind && wind_opaque.vs_class == contract::VsClass::Wind, "wind classified");
    CHECK(billboard_alpha.vs_class == contract::VsClass::Billboard, "billboard classified");
    CHECK(billboard_alpha.skip == pool::PoolSkip::NotRigid && billboard_opaque.skip == pool::PoolSkip::NotRigid, "billboard NotRigid (%s)", on ? "on" : "off");
    if (on) {
      CHECK(wind_alpha.skip == pool::PoolSkip::None && wind_alpha.alpha_material, "wind + alpha ON: admitted as rest pose");
      CHECK(wind_opaque.skip == pool::PoolSkip::WindOpaque && !wind_opaque.alpha_material, "wind + opaque ON: WindOpaque");
      CHECK(rigid_alpha.skip == pool::PoolSkip::None && rigid_alpha.alpha_material, "rigid + alpha ON: admitted");
    } else {
      CHECK(wind_alpha.skip == pool::PoolSkip::NotRigid && wind_opaque.skip == pool::PoolSkip::NotRigid, "wind OFF: NotRigid");
      CHECK(rigid_alpha.skip == pool::PoolSkip::AlphaTested && !rigid_alpha.alpha_material, "rigid + alpha OFF: AlphaTested");
    }
  }
  pool::g_pool.alpha_foliage.store(true);
  const uint64_t kUnknownPs = 0xD7;  // never classified
  CHECK(gate(kRigidVs, kUnknownPs).skip == pool::PoolSkip::PixelUnknown, "unclassified pixel shader: PixelUnknown");

  // S5 (decision 2): wind with an opaque pixel shader is WindOpaque unless alpha_wind_opaque and alpha_foliage are on.
  pool::g_pool.alpha_wind_opaque.store(false);
  CHECK(gate(kWindVs, kOpaquePs).skip == pool::PoolSkip::WindOpaque, "wind + opaque, d2 OFF: WindOpaque");
  pool::g_pool.alpha_wind_opaque.store(true);
  const auto wind_rest = gate(kWindVs, kOpaquePs);
  CHECK(wind_rest.skip == pool::PoolSkip::None && wind_rest.wind_rest && (wind_rest.pass & pool::kPoolPassWind) != 0u,
        "wind + opaque, d2 ON: admitted as rest pose");
  pool::g_pool.alpha_foliage.store(false);
  CHECK(gate(kWindVs, kOpaquePs).skip == pool::PoolSkip::NotRigid, "main OFF: wind NotRigid whatever d2 says");
  pool::g_pool.alpha_foliage.store(true);
  pool::g_pool.alpha_wind_opaque.store(false);
}

// A PixelUnknown draw logs its (vs, ps) pair once, from the locked scan block, with the draw's hashes.
static void TestPixelUnknownLog(Device& dev, CmdList& cl) {
  pool::ResetWorldPool();
  pool::g_pool.scan_active.store(true);
  pool::g_pool.alpha_foliage.store(true);
  const uint64_t kRigidVs = 0xD5, kUnknownPs = 0xD7;
  Classify(&dev, kRigidVs, "0x095017A3.vs.cso", true);
  falcom_world::DrawRecord draw;
  draw.method = 1; draw.has_index_buffer = true; draw.vb = {0x1111}; draw.vb_stride = fixture::kStride; draw.ib = {0x2222};
  draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {fixture::kLayout};
  draw.index_size = 2; draw.dsv = {0x77}; draw.index_count = 36; draw.first_index = 0; draw.instance_count = 1;
  draw.depth_enable = true; draw.depth_write = true; draw.topology = primitive_topology::triangle_list;
  draw.vs_pipeline = kRigidVs; draw.ps_pipeline = kUnknownPs; draw.vs_hash = 0xE1u; draw.ps_hash = 0xD7u;
  falcom_world::WorldCommandListData cl_data;
  const size_t before = LogCount("pixel_unknown: vs=");
  pool::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
  pool::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
  CHECK(LogCount("pixel_unknown: vs=") == before + 1u, "pixel_unknown logged once per pair: %zu", LogCount("pixel_unknown: vs="));
  CHECK(LogCount("pixel_unknown: vs=0x000000E1 ps=0x000000D7 reason=unclassified") > 0u, "pixel_unknown carries the draw's vs and ps hashes");
  pool::g_pool.scan_active.store(false);
}

static void TestAdmit() {
  pool::ResetWorldPool();
  pool::g_pool.alpha_foliage.store(true);
  std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
  const pool::PoolAlphaMaterial material = Mat(0x100, 0.5f);
  auto admit = [&](uint64_t key) {
    pool::ObservedInstance observed;
    observed.matrix[0] = observed.matrix[5] = observed.matrix[10] = 1.f;
    observed.matrix[3] = 5.f;
    return pool::AdmitPoolInstance(observed, key, MeshOf(key));
  };
  auto& stats = pool::g_pool.stats;

  Capture(0xA1, 7.f, true);
  pool::NotePoolAlphaMaterial(0xA1, material);
  CHECK(admit(0xA1) && pool::g_pool.instances.size() == 1u, "ON, ready mesh (UVs, no conflict): admitted");
  pool::g_pool.alpha_foliage.store(false);
  CHECK(!admit(0xA1) && stats.alpha_refused_off == 1u, "OFF: refused_off");
  pool::g_pool.alpha_foliage.store(true);

  Capture(0xB1, 8.f, true);
  Capture(0xB2, 8.f, true);
  pool::NotePoolAlphaMaterial(0xB1, material);
  pool::NotePoolAlphaMaterial(0xB2, Mat(0x100, 0.7f));
  CHECK(pool::g_pool.meshes[MeshOf(0xB1)].alpha_state.conflict, "mesh conflict from two keys");
  CHECK(!admit(0xB1) && stats.alpha_conflict_refused == 1u, "conflict: conflict_refused");

  Capture(0xC1, 9.f, false);
  pool::g_pool.alpha_uv_requeued.insert(0xC1);  // requeued for its UVs already: the no-UV refusal applies
  pool::NotePoolAlphaMaterial(0xC1, material);
  CHECK(!admit(0xC1) && stats.alpha_no_uv == 1u, "no UVs: no_uv");

  Capture(0xD1, 10.f, false);
  CHECK(admit(0xD1) && pool::g_pool.instances.size() == 2u, "non-alpha mesh admitted");
  CHECK(stats.alpha_refused_off == 1u && stats.alpha_conflict_refused == 1u && stats.alpha_no_uv == 1u,
        "one count per refusal reason");

  // Admission order: refused_off, then conflict_refused, then no_uv; a ready mesh is admitted.
  Capture(0xE1, 16.f, false);
  Capture(0xE2, 16.f, false);
  pool::g_pool.alpha_uv_requeued.insert(0xE1);
  pool::g_pool.alpha_uv_requeued.insert(0xE2);
  pool::NotePoolAlphaMaterial(0xE1, material);
  pool::NotePoolAlphaMaterial(0xE2, Mat(0x100, 0.7f));
  CHECK(MeshOf(0xE2) == MeshOf(0xE1) && pool::g_pool.meshes[MeshOf(0xE1)].alpha_state.conflict, "no-UV mesh conflicts");
  CHECK(!admit(0xE1) && stats.alpha_conflict_refused == 2u && stats.alpha_no_uv == 1u, "conflict with no UVs: conflict_refused first");
  pool::g_pool.alpha_foliage.store(false);
  CHECK(!admit(0xE1) && stats.alpha_refused_off == 2u && stats.alpha_conflict_refused == 2u, "OFF with a conflict: refused_off first");
  pool::g_pool.alpha_foliage.store(true);
}

static void TestMeshConflict() {
  pool::ResetWorldPool();
  pool::g_pool.alpha_foliage.store(true);
  std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
  auto& p = pool::g_pool;
  const pool::PoolAlphaMaterial m1 = Mat(0x100, 0.5f), m2 = Mat(0x100, 0.6f);

  // Capture first, then the second key's material.
  Capture(0x1001, 11.f, true);
  pool::NotePoolAlphaMaterial(0x1001, m1);
  Capture(0x1002, 11.f, true);
  CHECK(MeshOf(0x1002) == MeshOf(0x1001), "same content merged");
  pool::NotePoolAlphaMaterial(0x1002, m2);
  CHECK(!p.alpha_keys.at(0x1002).conflict, "key 0x1002 alone: no key conflict");
  CHECK(p.meshes[MeshOf(0x1001)].alpha_state.conflict && p.stats.alpha_meshes_conflicted == 1u, "capture first: mesh conflict");

  // Both materials noted before either capture.
  pool::NotePoolAlphaMaterial(0x2001, m1);
  pool::NotePoolAlphaMaterial(0x2002, m2);
  Capture(0x2001, 12.f, true);
  CHECK(p.meshes[MeshOf(0x2001)].alpha && !p.meshes[MeshOf(0x2001)].alpha_state.conflict, "note first: flagged at capture, no conflict yet");
  Capture(0x2002, 12.f, true);
  CHECK(p.meshes[MeshOf(0x2001)].alpha_state.conflict, "note first: mesh conflict at the second capture");

  // Same material on both keys: no conflict.
  Capture(0x3001, 13.f, true);
  Capture(0x3002, 13.f, true);
  pool::NotePoolAlphaMaterial(0x3001, m1);
  pool::NotePoolAlphaMaterial(0x3002, m1);
  CHECK(p.meshes[MeshOf(0x3001)].alpha && !p.meshes[MeshOf(0x3001)].alpha_state.conflict, "same materials: no conflict");

  // Key-level conflict on one key.
  Capture(0x4001, 14.f, true);
  pool::NotePoolAlphaMaterial(0x4001, m1);
  pool::NotePoolAlphaMaterial(0x4001, m2);
  CHECK(p.alpha_keys.at(0x4001).conflict && p.meshes[MeshOf(0x4001)].alpha_state.conflict, "key conflict reaches its mesh");

  // The flag and conflict survive one key's invalidation while another alpha key maps to the mesh;
  // they clear at CompactPool once no alpha key maps to it (a non-alpha key keeps the mesh alive).
  Capture(0x5001, 15.f, true);
  pool::NotePoolAlphaMaterial(0x5001, m1);
  Capture(0x5002, 15.f, true);
  pool::NotePoolAlphaMaterial(0x5002, m2);
  Capture(0x5003, 15.f, true);
  pool::InvalidatePoolMeshKey(0x5001);
  pool::CompactPool();
  CHECK(p.meshes[MeshOf(0x5002)].alpha && p.meshes[MeshOf(0x5002)].alpha_state.conflict, "flag survives invalidation of one key");
  pool::InvalidatePoolMeshKey(0x5002);
  pool::CompactPool();
  CHECK(!p.meshes[MeshOf(0x5003)].alpha && !p.meshes[MeshOf(0x5003)].alpha_state.conflict, "CompactPool clears when no alpha key maps to it");

  pool::g_pool.alpha_foliage.store(true);
}

// Step 1: a mesh captured without UVs (its key was not flagged then) is requeued once for them.
static void TestRequeue() {
  pool::ResetWorldPool();
  std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
  const pool::PoolAlphaMaterial material = Mat(0x100, 0.5f);
  auto& stats = pool::g_pool.stats;

  pool::g_pool.alpha_foliage.store(false);
  Capture(0xF1, 11.f, false);
  const uint32_t old_mesh = MeshOf(0xF1);
  pool::NotePoolAlphaMaterial(0xF1, material);
  CHECK(stats.alpha_uv_requeues == 0u && MeshOf(0xF1) == old_mesh, "OFF: no requeue");
  pool::g_pool.alpha_foliage.store(true);
  pool::NotePoolAlphaMaterial(0xF1, material);
  CHECK(stats.alpha_uv_requeues == 1u && MeshOf(0xF1) == UINT32_MAX, "ON, no UVs: requeued, key unmapped");
  CHECK(pool::g_pool.meshes[old_mesh].live_keys == 0u && pool::g_pool.retire_pending, "old mesh has no keys left: retired at the next compaction");
  CHECK(pool::g_pool.alpha_uv_requeued.count(0xF1) == 1u && pool::g_pool.mesh_queued.count(0xF1) == 0u, "key requeued, record kept");

  Capture(0xF1, 11.f, true);
  CHECK(MeshOf(0xF1) != old_mesh && !pool::g_pool.meshes[MeshOf(0xF1)].uvs.empty(), "recapture carries UVs");
  pool::NotePoolAlphaMaterial(0xF1, material);
  CHECK(stats.alpha_uv_requeues == 1u, "UVs present: no second requeue");

  Capture(0xF2, 12.f, false);
  pool::NotePoolAlphaMaterial(0xF2, material);
  CHECK(stats.alpha_uv_requeues == 2u, "UV-less again: requeued");
  Capture(0xF2, 12.f, false);
  pool::NotePoolAlphaMaterial(0xF2, material);
  CHECK(stats.alpha_uv_requeues == 2u, "UV-less after its requeue: not requeued a second time");

  pool::InvalidatePoolMeshKey(0xF2);
  CHECK(pool::g_pool.alpha_uv_requeued.count(0xF2) == 0u, "invalidation clears the requeue record");
  pool::g_pool.alpha_foliage.store(true);
}

static void TestResetClears() {
  pool::ResetWorldPool();
  std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
  CHECK(pool::g_pool.meshes.empty() && pool::g_pool.alpha_keys.empty(), "ResetWorldPool clears meshes and alpha keys");
}

constexpr uint64_t kUvLayout = 0x5151u;
constexpr uint32_t kUvStride = 20u;

// Position and UV (offset 12) per vertex, registered under kUvLayout.
static void RegisterUvLayout() {
  renodx::utils::scene::InputLayoutInfo layout;
  renodx::utils::scene::InputElementCopy position;
  position.semantic = "POSITION";
  position.format = reshade::api::format::r32g32b32_float;
  position.stride = kUvStride;
  renodx::utils::scene::InputElementCopy uv;
  uv.semantic = "TEXCOORD";
  uv.format = reshade::api::format::r32g32_float;
  uv.offset = 12u;
  uv.stride = kUvStride;
  layout.elements.push_back(position);
  layout.elements.push_back(uv);
  renodx::utils::scene::shared.data->input_layouts[kUvLayout] = layout;
}

// Draws of one mesh: an opaque draw and an alpha-tested draw sharing vertex/index data (same draw key).
static void TestScanPath(Device& dev, CmdList& cl, Queue& queue) {
  RegisterUvLayout();

  pool::ResetWorldPool();
  auto& p = pool::g_pool;
  p.scan_active.store(true);
  p.exclude_moving.store(false);
  p.follow_moving.store(false);

  const uint64_t kRigid = 0xE1, kOpaque = 0xE2, kAlpha = 0xE3;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kOpaque, "0x2162672F.ps.cso", false);
  Classify(&dev, kAlpha, "0x137F316A.ps.cso", false);

  resource t15, b1, vb, ib, material;
  const uint64_t ring_elements = 4096;
  dev.create_resource(resource_desc(ring_elements * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::index_buffer), nullptr, resource_usage::general, &ib);
  const fixture::TestMesh box = fixture::Box(1.f);
  for (size_t i = 0; i < box.positions.size(); ++i) {
    const float vertex[5] = {box.positions[i][0], box.positions[i][1], box.positions[i][2],
                             box.positions[i][1] * 0.5f + 0.5f, box.positions[i][2] * 0.5f + 0.5f};
    std::memcpy(dev.res[vb.handle].bytes.data() + i * kUvStride, vertex, sizeof(vertex));
  }
  fixture::WriteIndices(dev.res[ib.handle].bytes);
  std::array<uint8_t, 160> material_bytes = {};
  const float threshold = 0.5f;
  std::memcpy(material_bytes.data() + kReferenceThresholdOffset, &threshold, sizeof(float));
  material = TrackedCb(dev, material_bytes.data(), 160);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  resource alpha_tex = {0};
  const uint64_t alpha_view = SourceView(dev, &alpha_tex);
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = alpha_tex.handle;
  cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = alpha_view;

  uint64_t cursor = 0;
  auto draw_once = [&](uint64_t ps) {
    if (cursor + 1 >= ring_elements) cursor = 0;
    float world[12] = {1, 0, 0, 10, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160, world, 48);
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160 + 48, world, 48);
    const int32_t base = static_cast<int32_t>(cursor);
    const int32_t cb[4] = {base, 0, 0, 0};
    std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
    falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);
    falcom_world::DrawRecord draw;
    draw.method = 1; draw.has_index_buffer = true; draw.vb = {vb.handle}; draw.vb_stride = kUvStride; draw.ib = ib;
    draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {kUvLayout};
    draw.index_size = 2; draw.dsv = {0x77}; draw.index_count = 36; draw.first_index = 0;
    draw.instance_count = 1; draw.depth_enable = true; draw.depth_write = true;
    draw.topology = primitive_topology::triangle_list; draw.vs_pipeline = kRigid; draw.ps_pipeline = ps;
    draw.vs_hash = static_cast<uint32_t>(kRigid);
    pool::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
    cursor += 8;
  };
  auto frames = [&](int count) {
    for (int i = 0; i < count; ++i) {
      draw_once(kOpaque);
      draw_once(kAlpha);
      falcom_world::g_state.frame.fetch_add(1u);
      pool::DrainPoolScan(&dev, &queue);
    }
  };
  auto mesh_state = [&](bool& alpha, size_t& instances) {
    std::lock_guard<std::mutex> lock(p.mutex);
    alpha = p.meshes.empty() ? false : p.meshes[0].alpha;
    instances = p.instances.size();
  };

  // OFF (default): the alpha draw is skipped at the gate and does not flag the shared mesh.
  p.alpha_foliage.store(false);
  frames(40);
  bool alpha = true;
  size_t instances = 0;
  mesh_state(alpha, instances);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.meshes.size() == 1u, "one shared mesh: %zu", p.meshes.size());
    CHECK(!alpha && instances >= 1u, "OFF: mesh not flagged and instance admitted (alpha %d, instances %zu)", alpha, instances);
    CHECK(p.alpha_keys.empty() && p.stats.alpha_refused_off == 0u, "OFF: no alpha key, no refusal");
    CHECK(p.stats.skips[static_cast<size_t>(pool::PoolSkip::AlphaTested)] > 0u, "OFF: alpha draw skipped as alpha_tested");
  CHECK(p.meshes[0].uvs.empty(), "OFF: mesh captured without UVs (no alpha key)");
  }

  // ON: the alpha draw passes; the mesh is flagged, its instances go and admissions wait for the GPU stage.
  p.alpha_foliage.store(true);
  frames(90);
  mesh_state(alpha, instances);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(alpha && instances >= 1u, "ON: mesh flagged and admitted after the requeue (alpha %d, instances %zu)", alpha, instances);
    // The mesh was captured while OFF (no alpha key then, so no UVs): the sighting requeues it once
    // for its UVs, so it is recaptured with UVs (not no_uv); its material is on the GPU at the next present.
    CHECK(p.stats.alpha_uv_requeues > 0u && p.stats.alpha_no_uv == 0u, "ON after an OFF capture: requeued (%llu), no_uv (%llu)",
          (unsigned long long)p.stats.alpha_uv_requeues, (unsigned long long)p.stats.alpha_no_uv);
  }

  // OFF again: the flag stays until Reset Pool, admissions are refused as refused_off.
  p.alpha_foliage.store(false);
  const uint64_t refused_before = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.stats.alpha_refused_off; }();
  frames(40);
  mesh_state(alpha, instances);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(alpha && instances == 0u, "OFF after ON: flag kept, no instances");
    CHECK(p.stats.alpha_refused_off > refused_before, "OFF after ON: refused_off counts");
  }
  // ON from the start: the alpha key is noted before the mesh is captured, so the mesh has its UVs
  // and is admitted (no refusal).
  pool::ResetWorldPool();
  p.scan_active.store(true);
  p.alpha_foliage.store(true);
  frames(40);
  mesh_state(alpha, instances);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(alpha && instances >= 1u && p.stats.alpha_no_uv == 0u, "ON from the start: mesh flagged and admitted (%zu), no_uv (%llu)",
          instances, (unsigned long long)p.stats.alpha_no_uv);
  }
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
}

// Indirect alpha draws (OnPoolScanIndirectDraw). The args describe the real draw (36 indices, one
// instance, first index 0, vertex offset 0), so the key the copy resolves to is PoolMeshKey of it.
static void TestIndirectAlpha(Device& dev, CmdList& cl, Queue& queue) {
  auto& p = pool::g_pool;
  p.alpha_indirect_source.store(false);  // (a), (b) and (d) are the d1 OFF cases (on by default); Tests 1 and 2 set it ON
  RegisterUvLayout();
  const uint64_t kRigid = 0xF1, kAlpha = 0xF3;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kAlpha, "0x137F316A.ps.cso", false);

  resource t15, b1, args, vb, ib, material;
  dev.create_resource(resource_desc(4096 * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::indirect_argument), nullptr, resource_usage::general, &args);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::index_buffer), nullptr, resource_usage::general, &ib);
  const fixture::TestMesh box = fixture::Box(1.f);
  for (size_t i = 0; i < box.positions.size(); ++i) {
    const float vertex[5] = {box.positions[i][0], box.positions[i][1], box.positions[i][2],
                             box.positions[i][1] * 0.5f + 0.5f, box.positions[i][2] * 0.5f + 0.5f};
    std::memcpy(dev.res[vb.handle].bytes.data() + i * kUvStride, vertex, sizeof(vertex));
  }
  fixture::WriteIndices(dev.res[ib.handle].bytes);
  std::array<uint8_t, 160> material_bytes = {};
  const float threshold = 0.5f;
  std::memcpy(material_bytes.data() + kReferenceThresholdOffset, &threshold, sizeof(float));
  material = TrackedCb(dev, material_bytes.data(), 160);
  const uint32_t args_values[5] = {36u, 1u, 0u, 0u, 0u};
  std::memcpy(dev.res[args.handle].bytes.data(), args_values, sizeof(args_values));
  const float world[12] = {1, 0, 0, 10, 0, 1, 0, 0, 0, 0, 1, 0};
  std::memcpy(dev.res[t15.handle].bytes.data(), world, 48);
  std::memcpy(dev.res[t15.handle].bytes.data() + 48, world, 48);
  const int32_t cb[4] = {0, 0, 0, 0};
  std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
  falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  resource alpha_tex = {0};
  const uint64_t alpha_view = SourceView(dev, &alpha_tex);
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = alpha_tex.handle;
  cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = alpha_view;

  const auto make_draw = [&](uint64_t ps) {
    falcom_world::DrawRecord draw;
    draw.method = 1; draw.has_index_buffer = true; draw.vb = {vb.handle}; draw.vb_stride = kUvStride; draw.ib = ib;
    draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {kUvLayout};
    draw.index_size = 2; draw.dsv = {0x77}; draw.depth_enable = true; draw.depth_write = true;
    draw.topology = primitive_topology::triangle_list; draw.vs_pipeline = kRigid; draw.ps_pipeline = ps;
    draw.vs_hash = static_cast<uint32_t>(kRigid);
    return draw;
  };
  const auto record = [&]() { pool::OnPoolScanIndirectDraw(&dev, &cl, make_draw(kAlpha), &cl_data, args, 0, 1u, 20u); };
  const auto frames = [&](int count) {
    for (int i = 0; i < count; ++i) {
      record();
      falcom_world::g_state.frame.fetch_add(1u);
      pool::DrainPoolScan(&dev, &queue);
    }
  };
  const auto reset = [&](bool on) {
    pool::ResetWorldPool();
    p.scan_active.store(true);
    p.exclude_moving.store(false);
    p.follow_moving.store(false);
    p.alpha_foliage.store(on);
  };
  falcom_world::DrawRecord real = make_draw(kAlpha);
  real.index_count = 36;
  const uint64_t real_key = pool::PoolMeshKey(real);

  // (a) ON: the copy carries the alpha pass and material; the real key flags its mesh, which is refused as not ready.
  reset(true);
  record();
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    const auto& copies = p.slots[p.write_slot].copies;
    CHECK(!copies.empty() && (copies.back().pass & pool::kPoolPassAlpha) != 0u && copies.back().alpha.view == alpha_view && copies.back().alpha.threshold == threshold,
          "indirect ON: copy carries the alpha pass and material");
  }
  frames(40);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    const auto mesh_it = p.mesh_by_key.find(real_key);
    CHECK(p.alpha_keys.count(real_key) != 0u && mesh_it != p.mesh_by_key.end() && p.meshes[mesh_it->second].alpha,
          "indirect ON: real key flags its mesh");
    CHECK(p.stats.alpha_no_uv == 0u && p.stats.alpha_conflict_refused == 0u, "indirect ON: not refused (UVs, no conflict)");
  }
  // F8: an indirect-only alpha mesh has no copy, so it has no slice: the TLAS counts it as waiting (fail closed), it is
  // neither admitted as opaque nor silently dropped.
  pool::BvhDeviceData* data = pool::GetBvhDeviceData(&dev);
  for (int i = 0; i < 10; ++i) {
    falcom_world::g_state.frame.fetch_add(1u);
    pool::UpdateLiveBvh(&dev, &queue);
  }
  CHECK(data->live.tlas_alpha_waiting == 1u && data->live.tlas_alpha_instances == 0u && data->tlas_instances_cpu.empty(),
        "indirect ON: alpha_waiting in the TLAS (waiting %u, alpha %u, instances %zu)", data->live.tlas_alpha_waiting,
        data->live.tlas_alpha_instances, data->tlas_instances_cpu.size());

  // (b) Recorded ON, the switch flips OFF before the resolve: the copy still flags the mesh, so it is never admitted as opaque.
  // (Admission needs kPoolStableObservations copies; this single copy only flags, so refused_off is not reached here.)
  reset(true);
  record();
  p.alpha_foliage.store(false);
  frames(40);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    const auto mesh_it = p.mesh_by_key.find(real_key);
    CHECK(mesh_it != p.mesh_by_key.end() && p.meshes[mesh_it->second].alpha, "ON record, OFF before resolve: mesh still flagged");
    CHECK(p.instances.empty(), "ON record, OFF before resolve: no instance admitted");
  }

  // (c) OFF: the alpha draw is skipped at the gate; no key, no flag, no alpha stats.
  reset(false);
  frames(40);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    bool any_alpha = false;
    for (const auto& mesh : p.meshes) any_alpha = any_alpha || mesh.alpha;
    CHECK(p.alpha_keys.empty() && !any_alpha, "indirect OFF: no alpha key, no flag");
    CHECK(p.stats.alpha_draws == 0u && p.stats.alpha_conflicts == 0u && p.stats.alpha_refused_off == 0u,
          "indirect OFF: no alpha stats");
    CHECK(p.stats.skips[static_cast<size_t>(pool::PoolSkip::AlphaTested)] > 0u, "indirect OFF: skipped as alpha_tested");
  }
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  pool::UpdateLiveBvh(&dev, &queue);  // empties the live store (the mesh above was resident)

  // (d) Indirect source copy (S3, CPU side). OFF (default): no keyless copy, no source for the indirect key.
  reset(true);
  record();
  frames(40);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_sources.empty() && p.alpha_indirect_keys.empty() && p.stats.alpha_indirect_copies == 0u,
          "indirect source OFF: no source, no keyless copy");
  }
  // Test 1: ON. One copy over one texture; the copy is attached to the key at resolve; a slice after 3 presents.
  reset(true);
  p.alpha_indirect_source.store(true);
  frames(40);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.stats.alpha_indirect_copies == 1u && p.alpha_sources.size() == 1u, "indirect ON: one copy over one texture (%llu)",
          static_cast<unsigned long long>(p.stats.alpha_indirect_copies));
    CHECK(p.alpha_indirect_keys.count(real_key) == 1u && p.alpha_key_source.count(real_key) == 1u, "indirect ON: the key has the source");
  }
  for (int i = 0; i < 3; ++i) {
    falcom_world::g_state.frame.fetch_add(1u);
    pool::UpdateLiveBvh(&dev, &queue);
  }
  CHECK(data->live.tlas_alpha_instances == 1u && data->live.tlas_alpha_waiting == 0u,
        "indirect ON: alpha instance in the TLAS after 3 presents (instances %u, waiting %u)", data->live.tlas_alpha_instances,
        data->live.tlas_alpha_waiting);

  // Test 5: three sub-draws over one texture make one copy; a sub-draw refused by the cooldown makes none.
  reset(true);
  p.alpha_indirect_source.store(true);
  falcom_world::g_state.frame.fetch_add(1u);
  pool::UpdateLiveBvh(&dev, &queue);
  pool::OnPoolScanIndirectDraw(&dev, &cl, make_draw(kAlpha), &cl_data, args, 0, 3u, 20u);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.stats.alpha_indirect_copies == 1u && p.alpha_sources.size() == 1u, "three sub-draws, one texture: one copy (%llu)",
          static_cast<unsigned long long>(p.stats.alpha_indirect_copies));
  }
  pool::OnPoolScanIndirectDraw(&dev, &cl, make_draw(kAlpha), &cl_data, args, 0, 1u, 20u);  // cooldown-refused in this frame
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.stats.alpha_indirect_copies == 1u && p.stats.alpha_source_copies == 1u, "a cooldown-refused sub-draw makes no copy");
  }

  // Test 3: a keyless copy with no resolve is held for kAlphaOrphanFrames presents, then freed and counted.
  reset(true);
  p.alpha_indirect_source.store(true);
  falcom_world::g_state.frame.fetch_add(1u);
  pool::UpdateLiveBvh(&dev, &queue);
  record();  // no DrainPoolScan: no resolve
  for (uint32_t i = 0; i < bvh::kAlphaOrphanFrames; ++i) {
    falcom_world::g_state.frame.fetch_add(1u);
    pool::UpdateLiveBvh(&dev, &queue);
  }
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_sources.size() == 1u && p.stats.alpha_orphans_expired == 0u, "orphan held at kAlphaOrphanFrames presents");
  }
  falcom_world::g_state.frame.fetch_add(1u);
  pool::UpdateLiveBvh(&dev, &queue);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_sources.empty() && p.stats.alpha_orphans_expired == 1u, "orphan freed after 601 presents (expired %llu)",
          static_cast<unsigned long long>(p.stats.alpha_orphans_expired));
  }
  // Test 2 (S4): indirect switch ON, then OFF. The copy is freed at the next present, the indirect slice goes, and the
  // TLAS has no instance on that present (the mesh waits again).
  reset(true);
  p.alpha_indirect_source.store(true);
  frames(40);
  for (int i = 0; i < 3; ++i) {
    falcom_world::g_state.frame.fetch_add(1u);
    pool::UpdateLiveBvh(&dev, &queue);
  }
  uint64_t real_uid = 0u;
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    real_uid = p.meshes[p.mesh_by_key.at(real_key)].uid;
  }
  CHECK(data->live.tlas_alpha_instances == 1u, "indirect ON before the switch-off: one alpha instance (%u)", data->live.tlas_alpha_instances);
  p.alpha_indirect_source.store(false);
  falcom_world::g_state.frame.fetch_add(1u);
  pool::DrainPoolScan(&dev, &queue);
  pool::UpdateLiveBvh(&dev, &queue);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_sources.empty() && p.alpha_indirect_keys.empty(), "indirect OFF: the orphan source is freed at the next present (sources %zu)",
          p.alpha_sources.size());
  }
  CHECK(data->alpha.slot_of_uid.count(real_uid) == 0u && data->tlas_instances_cpu.empty() && data->live.tlas_alpha_waiting == 1u,
        "indirect OFF: slice gone, no instance on the next present (slots %zu, instances %zu, waiting %u)",
        data->alpha.slot_of_uid.size(), data->tlas_instances_cpu.size(), data->live.tlas_alpha_waiting);

  // Test 4 (S4): the source texture is destroyed between the indirect draw and its resolve. The copy made before the
  // destroy still blits the mesh; the destroyed texture has no link, and later draws through its view make no copy.
  reset(true);
  p.alpha_indirect_source.store(true);
  record();
  dev.destroy_resource(alpha_tex);
  pool::OnDestroyResourcePool(&dev, alpha_tex);
  frames(40);
  for (int i = 0; i < 10; ++i) {
    falcom_world::g_state.frame.fetch_add(1u);
    pool::UpdateLiveBvh(&dev, &queue);
  }
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_source_by_texture.count(alpha_tex.handle) == 0u, "destroyed texture: no link left");
    CHECK(p.stats.alpha_indirect_copies == 1u, "destroyed texture: no copy of the dead texture (%llu)",
          static_cast<unsigned long long>(p.stats.alpha_indirect_copies));
    real_uid = p.meshes[p.mesh_by_key.at(real_key)].uid;  // the reset mesh has a new uid
  }
  CHECK(data->alpha.slot_of_uid.count(real_uid) == 1u && data->live.tlas_alpha_instances == 1u,
        "texture destroyed before resolve: the mesh still blits (slots %zu, instances %u)", data->alpha.slot_of_uid.size(),
        data->live.tlas_alpha_instances);

  // Teardown: OFF destroys the atlas and frees the copies; the indirect switch goes back to its default.
  pool::ResetWorldPool();
  p.alpha_indirect_source.store(false);
  p.alpha_foliage.store(false);
  pool::UpdateLiveBvh(&dev, &queue);
  p.alpha_foliage.store(true);
  pool::UpdateLiveBvh(&dev, &queue);
  cl.dispatches = 0;  // the blits above are not counted by TestAtlas
}


// GPU stage (mock device). A source copy is made at draw time; the blit fills one slice at the next
// present and frees the copy; a source whose texture dies is not blitted and its copy is freed at the
// present; a present blits at most kAlphaBlitsPerFrame slices; a full atlas keeps the copy; with
// alpha_foliage off there is no atlas, copy, blit or creation.
static uint64_t SourceView(Device& dev, resource* texture) {
  const resource_desc desc(resource_type::texture_2d, 64, 64, 1, 1, format::r8g8b8a8_unorm, 1, memory_heap::gpu_only, resource_usage::shader_resource);
  dev.create_resource(desc, nullptr, resource_usage::shader_resource, texture);
  resource_view view = {0};
  dev.create_resource_view(*texture, resource_usage::shader_resource, resource_view_desc(format::r8g8b8a8_unorm), &view);
  return view.handle;
}

static void TestAtlas(Device& dev, CmdList& cl, Queue& queue) {
  auto& p = pool::g_pool;
  pool::BvhDeviceData* data = pool::GetBvhDeviceData(&dev);
  auto present = [&] { falcom_world::g_state.frame.fetch_add(1u); pool::UpdateLiveBvh(&dev, &queue); };
  auto sources = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.alpha_sources.size(); };
  auto uid_of = [&](uint64_t key) { return p.meshes[MeshOf(key)].uid; };
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  falcom_world::g_state.frame.store(5000u);

  // 1. Lifecycle: the copy at draw time, the blit at the present, the copy freed, the atlas made on first use.
  resource tex1 = {0};
  const uint64_t view1 = SourceView(dev, &tex1);
  Capture(0x7001, 7.f, true);
  pool::NotePoolAlphaMaterial(0x7001, Mat(view1, 0.5f));
  const int copies_before = cl.copies;
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7001, view1, true);
  CHECK(sources() == 1u && cl.copies == copies_before + 1 && p.stats.alpha_source_copies == 1u, "copy made at draw time (%zu, %d)", sources(), cl.copies - copies_before);
  const uint64_t proxy1 = p.alpha_sources.at(p.alpha_key_source.at(0x7001)).proxy.handle;
  CHECK(data->alpha.atlas.handle == 0u, "no atlas before the first blit");
  present();
  CHECK(data->alpha.atlas.handle != 0u && cl.dispatches == 1, "atlas made and one blit at the present (%d)", cl.dispatches);
  CHECK(cl.u0 == 0u, "the atlas UAV is unbound at u0 after the blit (%llu)", (unsigned long long)cl.u0);
  CHECK(data->alpha.slot_of_uid.count(uid_of(0x7001)) == 1u && data->alpha.slices.used == 1u, "the mesh has its slice");
  CHECK(sources() == 1u && p.alpha_sources.at(p.alpha_key_source.at(0x7001)).blitted && dev.res.count(proxy1) == 1u,
        "copy kept by the blit (blitted, proxy alive): %zu sources", sources());
  CHECK(p.alpha_source_done.count(0x7001) == 0u, "a blitted key is not refused");

  // 2. The source texture dies before the blit: no blit, the copy is freed at the present.
  resource tex2 = {0};
  const uint64_t view2 = SourceView(dev, &tex2);
  Capture(0x7002, 8.f, true);
  pool::NotePoolAlphaMaterial(0x7002, Mat(view2, 0.5f));
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7002, view2, true);
  const uint64_t proxy2 = p.alpha_sources.at(p.alpha_key_source.at(0x7002)).proxy.handle;
  const int dispatch2 = cl.dispatches;
  dev.destroy_resource(tex2);
  pool::OnDestroyResourcePool(&dev, tex2);
  CHECK(dev.res.count(proxy2) == 1u, "proxy waits for the present (not freed inside the event)");
  present();
  CHECK(cl.dispatches == dispatch2 && dev.res.count(proxy2) == 0u, "dead source: no blit, proxy freed at the present (%d)", cl.dispatches - dispatch2);

  // 3. Atlas full: the mesh keeps its copy and gets no slice.
  while (data->alpha.slices.used < falcom_world::kAlphaAtlasSlices) data->alpha.slices.Acquire(0x9000u + data->alpha.slices.used);
  resource tex3 = {0};
  const uint64_t view3 = SourceView(dev, &tex3);
  Capture(0x7003, 9.f, true);
  pool::NotePoolAlphaMaterial(0x7003, Mat(view3, 0.5f));
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7003, view3, true);
  const int dispatch3 = cl.dispatches;
  const uint64_t cap_before = data->alpha.cap_refused;
  present();
  CHECK(cl.dispatches == dispatch3 && data->alpha.slot_of_uid.count(uid_of(0x7003)) == 0u, "full atlas: no slice, no blit");
  CHECK(data->alpha.cap_refused > cap_before && sources() == 2u, "full atlas: refused and the copy kept (%llu, %zu sources)",
        (unsigned long long)data->alpha.cap_refused, sources());
  const uint64_t cap_once = data->alpha.cap_refused;
  present();
  CHECK(data->alpha.cap_refused == cap_once, "a refused mesh is counted once, not per present (%llu)", (unsigned long long)data->alpha.cap_refused);
  for (uint32_t s = 0; s < falcom_world::kAlphaAtlasSlices; ++s) {
    if (data->alpha.slices.owner[s] >= 0x9000u) data->alpha.slices.Release(s);
  }

  // 4. At most falcom_world::kAlphaBlitsPerFrame blits a present, with the copies spread over two frames.
  for (int i = 0; i < 6; ++i) {
    if (i == 4) falcom_world::g_state.frame.fetch_add(1u);  // the copy budget renews
    resource t = {0};
    const uint64_t view = SourceView(dev, &t);
    Capture(0x7100 + i, 20.f + i, true);  // sizes apart from the other meshes (equal geometry shares a mesh)
    pool::NotePoolAlphaMaterial(0x7100 + i, Mat(view, 0.5f));
    pool::CapturePoolAlphaSource(&dev, &cl, 0x7100 + i, view, true);
  }
  CHECK(sources() == 8u, "six copies and the copies of 0x7001 and 0x7003 (%zu)", sources());
  int worst = 0;
  for (int i = 0; i < 16; ++i) {
    const int before = cl.dispatches;
    present();
    worst = (std::max)(worst, cl.dispatches - before);
  }
  CHECK(worst >= 1 && worst <= static_cast<int>(falcom_world::kAlphaBlitsPerFrame), "at most %u blits a present (worst %d)", falcom_world::kAlphaBlitsPerFrame, worst);
  CHECK(data->alpha.slot_of_uid.size() == 8u && sources() == 8u, "all blitted: 7001, 7003 and six more (%zu slots, %zu copies kept)",
        data->alpha.slot_of_uid.size(), sources());
  const int dispatch_settled = cl.dispatches;
  present();
  CHECK(cl.dispatches == dispatch_settled, "blitted copies are not blitted again while their slices are held");

  // 5. Off: the atlas, the slices and the copies are gone; nothing is created, copied or blitted.
  p.alpha_foliage.store(false);
  present();
  CHECK(data->alpha.atlas.handle == 0u && data->alpha.slot_of_uid.empty() && data->alpha.slices.used == 0u, "off: atlas, slices and slot map released");
  const int creates_off = dev.creates;
  const int dispatch_off = cl.dispatches;
  const int copies_off = cl.copies;
  for (int i = 0; i < 5; ++i) present();
  CHECK(dev.creates == creates_off && cl.dispatches == dispatch_off && cl.copies == copies_off, "off: no creation, blit or copy");
  CHECK(!data->alpha.failure_logged && data->alpha.cap_refused_uids.empty(), "off: the failure latch and the refused set are reset");
  p.alpha_foliage.store(true);
  pool::ResetWorldPool();
  present();
}

// Step 5, end to end on the mock device: an admitted alpha mesh with a texture waits in the TLAS (alpha_waiting)
// until its source copy is blitted into its slice; then it is in the TLAS with header.z = slice + 1. With
// alpha_foliage off it is out of the TLAS again, and nothing of the atlas exists.
static void TestEndToEnd(Device& dev, CmdList& cl, Queue& queue) {
  auto& p = pool::g_pool;
  pool::BvhDeviceData* data = pool::GetBvhDeviceData(&dev);
  auto present = [&] { falcom_world::g_state.frame.fetch_add(1u); pool::UpdateLiveBvh(&dev, &queue); };
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  resource tex = {0};
  const uint64_t view = SourceView(dev, &tex);
  Capture(0x7401, 13.f, true);
  pool::NotePoolAlphaMaterial(0x7401, Mat(view, 0.5f));
  pool::ObservedInstance observed;
  observed.matrix[0] = observed.matrix[5] = observed.matrix[10] = 1.f;
  observed.matrix[3] = 5.f;
  bool admitted = false;
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    admitted = pool::AdmitPoolInstance(observed, 0x7401, MeshOf(0x7401));
  }
  CHECK(admitted, "ON, UVs and a conflict-free material: admitted");
  for (int i = 0; i < 10; ++i) present();
  CHECK(data->live.tlas_alpha_waiting == 1u && data->tlas_instances_cpu.empty(), "no slice yet: left out of the TLAS (waiting %u, instances %zu)",
        data->live.tlas_alpha_waiting, data->tlas_instances_cpu.size());

  pool::CapturePoolAlphaSource(&dev, &cl, 0x7401, view, true);
  for (int i = 0; i < 10; ++i) present();
  const uint32_t slice = data->alpha.slot_of_uid.count(p.meshes[MeshOf(0x7401)].uid) != 0u ? data->alpha.slot_of_uid[p.meshes[MeshOf(0x7401)].uid] : UINT32_MAX;
  CHECK(slice != UINT32_MAX && data->tlas_instances_cpu.size() == 1u, "blitted: the mesh has a slice and one TLAS instance (slice %u, instances %zu)",
        slice, data->tlas_instances_cpu.size());
  CHECK(!data->tlas_instances_cpu.empty() && data->tlas_instances_cpu[0].header[2] == static_cast<float>(slice + 1u),
        "TLAS descriptor carries the material slot: header.z %f", data->tlas_instances_cpu.empty() ? -1.f : data->tlas_instances_cpu[0].header[2]);
  CHECK(data->live.tlas_alpha_instances == 1u && data->live.tlas_alpha_waiting == 0u, "alpha instance counted in the TLAS");

  // Live store reset (F1): the slice survives it (pool state), the mesh is uploaded again and keeps its header.z.
  const uint32_t uid = p.meshes[MeshOf(0x7401)].uid;
  pool::ResetLiveStore(&dev, data, "test");
  for (int i = 0; i < 10; ++i) present();
  CHECK(data->alpha.slot_of_uid.count(uid) == 1u && data->tlas_instances_cpu.size() == 1u
            && data->tlas_instances_cpu[0].header[2] == static_cast<float>(data->alpha.slot_of_uid[uid] + 1u),
        "after a live store reset: slice kept, one TLAS instance with header.z %f",
        data->tlas_instances_cpu.empty() ? -1.f : data->tlas_instances_cpu[0].header[2]);
  // A slice dropped after the reset is filled again from the copy the blit kept.
  data->alpha.slices.Release(data->alpha.slot_of_uid[uid]);
  data->alpha.slot_of_uid.erase(uid);
  const int dispatch_reblit = cl.dispatches;
  present();
  CHECK(cl.dispatches == dispatch_reblit + 1 && data->alpha.slot_of_uid.count(uid) == 1u && data->alpha.slices.used == 1u,
        "dropped slice: blitted again from the kept copy (%d blits)", cl.dispatches - dispatch_reblit);
  for (int i = 0; i < 3; ++i) present();
  CHECK(!data->tlas_instances_cpu.empty() && data->tlas_instances_cpu[0].header[2] == static_cast<float>(data->alpha.slot_of_uid[uid] + 1u),
        "re-blitted mesh: header.z %f", data->tlas_instances_cpu.empty() ? -1.f : data->tlas_instances_cpu[0].header[2]);

  // OFF (F2): the alpha instance leaves the TLAS on the first present after the switch.
  p.alpha_foliage.store(false);
  present();
  CHECK(data->tlas_instances_cpu.empty() && data->live.tlas_alpha_instances == 0u,
        "off: alpha instance dropped on the first present (instances %zu, alpha %u)", data->tlas_instances_cpu.size(),
        data->live.tlas_alpha_instances);
  for (int i = 0; i < 9; ++i) present();
  CHECK(data->tlas_instances_cpu.empty() && data->alpha.atlas.handle == 0u && data->alpha.slices.used == 0u,
        "off: no alpha instance in the TLAS, no atlas, no slices");
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  present();
}

// Source copies: the byte cap (F4), the view's format (F5), the copy budget of skipped draws (F3), and the live
// BVH off (F7). The copies are made at draw time on the immediate context, as in the game.
static void TestSourceGuards(Device& dev, CmdList& cl, Queue& queue) {
  RegisterUvLayout();
  auto& p = pool::g_pool;
  auto drain = [&] { p.alpha_foliage.store(false); pool::UpdateLiveBvh(&dev, &queue); p.alpha_foliage.store(true); };
  auto sources = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.alpha_sources.size(); };
  pool::ResetWorldPool();
  p.scan_active.store(true);
  p.exclude_moving.store(false);
  p.follow_moving.store(false);
  p.alpha_foliage.store(true);
  falcom_world::g_state.frame.store(9100u);

  // F4: copies of 4096 x 4096 RGBA8 (64 MB each): four fit in kAlphaSourceBytesMax, the fifth is refused and counted.
  for (int i = 0; i < 5; ++i) {
    if (i == 4) falcom_world::g_state.frame.fetch_add(1u);  // the copy budget renews
    resource t = {0};
    dev.create_resource(resource_desc(resource_type::texture_2d, 4096, 4096, 1, 1, format::r8g8b8a8_unorm, 1, memory_heap::gpu_only,
                                      resource_usage::shader_resource), nullptr, resource_usage::shader_resource, &t);
    resource_view v = {0};
    dev.create_resource_view(t, resource_usage::shader_resource, resource_view_desc(format::r8g8b8a8_unorm), &v);
    pool::CapturePoolAlphaSource(&dev, &cl, 0x7600 + i, v.handle, true);
  }
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_sources.size() == 4u && pool::PoolAlphaProxyBytes() == 4ull * 4096ull * 4096ull * 4ull
              && p.stats.alpha_source_refused_bytes == 1u,
          "byte cap: 4 copies (%zu sources, %llu bytes), fifth refused (%llu)", p.alpha_sources.size(),
          (unsigned long long)pool::PoolAlphaProxyBytes(), (unsigned long long)p.stats.alpha_source_refused_bytes);
  }
  drain();
  CHECK(sources() == 0u, "byte cap: copies freed by the switch (%zu)", sources());

  // F5: the proxy takes the bound view's format (the texture is typeless); an unknown view format is refused once.
  falcom_world::g_state.frame.fetch_add(1u);
  resource typeless = {0};
  dev.create_resource(resource_desc(resource_type::texture_2d, 64, 64, 1, 1, format::r8g8b8a8_typeless, 1, memory_heap::gpu_only,
                                    resource_usage::shader_resource), nullptr, resource_usage::shader_resource, &typeless);
  resource_view typed = {0};
  dev.create_resource_view(typeless, resource_usage::shader_resource, resource_view_desc(format::r8g8b8a8_unorm), &typed);
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7610, typed.handle, true);
  CHECK(p.alpha_key_source.count(0x7610) == 1u && p.alpha_sources.at(p.alpha_key_source.at(0x7610)).format == format::r8g8b8a8_unorm,
        "view format: the proxy has the bound view's format");
  resource_view unknown = {0};
  dev.create_resource_view(typeless, resource_usage::shader_resource, resource_view_desc(format::unknown), &unknown);
  falcom_world::g_state.frame.fetch_add(1u);
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7611, unknown.handle, true);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_key_source.count(0x7611) == 0u && p.alpha_source_done.count(0x7611) == 1u && p.stats.alpha_source_refused_format == 1u,
          "unknown view format: refused and counted (%llu)", (unsigned long long)p.stats.alpha_source_refused_format);
    const auto& s = p.stats;
    const uint64_t split = s.alpha_source_refused_cap + s.alpha_source_refused_deferred + s.alpha_source_refused_type + s.alpha_source_refused_failed;
    CHECK(s.alpha_source_refused_type == 1u && split == s.alpha_source_refused, "format refusal is a type refusal; reasons sum to the total (%llu of %llu)",
          (unsigned long long)split, (unsigned long long)s.alpha_source_refused);
  }
  drain();

  // F3: a draw skipped as TooManyInstances takes no copy; four such draws in one frame leave the budget for a valid draw.
  const uint64_t kRigid = 0xD1, kAlpha = 0xD3;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kAlpha, "0x137F316A.ps.cso", false);
  resource t15, b1, vb, ib, material;
  const uint64_t ring_elements = 4096;
  dev.create_resource(resource_desc(ring_elements * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::index_buffer), nullptr, resource_usage::general, &ib);
  const fixture::TestMesh box = fixture::Box(1.f);
  for (size_t i = 0; i < box.positions.size(); ++i) {
    const float vertex[5] = {box.positions[i][0], box.positions[i][1], box.positions[i][2],
                             box.positions[i][1] * 0.5f + 0.5f, box.positions[i][2] * 0.5f + 0.5f};
    std::memcpy(dev.res[vb.handle].bytes.data() + i * kUvStride, vertex, sizeof(vertex));
  }
  fixture::WriteIndices(dev.res[ib.handle].bytes);
  std::array<uint8_t, 160> material_bytes = {};
  const float threshold = 0.5f;
  std::memcpy(material_bytes.data() + kReferenceThresholdOffset, &threshold, sizeof(float));
  material = TrackedCb(dev, material_bytes.data(), 160);
  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  resource tex = {0};
  const uint64_t view = SourceView(dev, &tex);
  uint64_t cursor = 0;
  auto draw = [&](uint32_t count) {
    if (cursor + 1 >= ring_elements) cursor = 0;
    float world[12] = {1, 0, 0, 10, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160, world, 48);
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160 + 48, world, 48);
    const int32_t base = static_cast<int32_t>(cursor);
    const int32_t cb[4] = {base, 0, 0, 0};
    std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
    falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);
    cl_data.ps_srv[contract::kAlphaTexSlot].handle = tex.handle;
    cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = view;
    falcom_world::DrawRecord draw_record;
    draw_record.method = 1; draw_record.has_index_buffer = true; draw_record.vb = {vb.handle}; draw_record.vb_stride = kUvStride; draw_record.ib = ib;
    draw_record.vb_size = 4096; draw_record.ib_size = 4096; draw_record.input_layout = {kUvLayout};
    draw_record.index_size = 2; draw_record.dsv = {0x77}; draw_record.index_count = 36; draw_record.first_index = 0;
    draw_record.instance_count = count; draw_record.depth_enable = true; draw_record.depth_write = true;
    draw_record.topology = primitive_topology::triangle_list; draw_record.vs_pipeline = kRigid; draw_record.ps_pipeline = kAlpha;
    draw_record.vs_hash = static_cast<uint32_t>(kRigid);
    pool::OnPoolScanDraw(&dev, &cl, draw_record, &cl_data);
    cursor += 8;
  };
  falcom_world::g_state.frame.fetch_add(1u);
  const int copies_before = cl.copies;
  const uint64_t stat_copies_before = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.stats.alpha_source_copies; }();
  const uint64_t too_many_before = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.stats.skips[static_cast<size_t>(pool::PoolSkip::TooManyInstances)]; }();
  for (int i = 0; i < 4; ++i) draw(pool::kPoolMaxInstancesPerDraw + 1u);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.stats.skips[static_cast<size_t>(pool::PoolSkip::TooManyInstances)] == too_many_before + 4u
              && p.stats.alpha_source_copies == stat_copies_before && cl.copies == copies_before,
          "skipped draws take no copy (skips +%llu, copies +%llu)",
          (unsigned long long)(p.stats.skips[static_cast<size_t>(pool::PoolSkip::TooManyInstances)] - too_many_before),
          (unsigned long long)(p.stats.alpha_source_copies - stat_copies_before));
  }
  draw(1u);
  CHECK(sources() == 1u && cl.copies == copies_before + 1, "a valid draw after four skipped draws gets its copy (%zu, copies %d)", sources(), cl.copies - copies_before);
  drain();

  // F7: live BVH off: no copy is made, and the copy made before is freed at the next present.
  falcom_world::g_state.frame.fetch_add(1u);
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7700, view, true);
  const uint64_t live_proxy = p.alpha_sources.at(p.alpha_key_source.at(0x7700)).proxy.handle;
  pool::g_live_bvh.enabled.store(false);
  pool::UpdateLiveBvh(&dev, &queue);
  CHECK(sources() == 0u && dev.res.count(live_proxy) == 0u, "live off: the copy is freed at the present");
  const int copies_off = cl.copies;
  falcom_world::g_state.frame.fetch_add(1u);
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7701, view, true);
  CHECK(sources() == 0u && cl.copies == copies_off, "live off: no copy made");
  pool::g_live_bvh.enabled.store(true);
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  pool::UpdateLiveBvh(&dev, &queue);
}

// FIX E: copies are per source texture. 70 draw keys over 3 textures make 3 copies, every key shares one copy and
// gets a slice, and a texture's copy is freed with its last key.
static void TestSharedCopies(Device& dev, CmdList& cl, Queue& queue) {
  auto& p = pool::g_pool;
  pool::BvhDeviceData* data = pool::GetBvhDeviceData(&dev);
  auto present = [&] { falcom_world::g_state.frame.fetch_add(1u); pool::UpdateLiveBvh(&dev, &queue); };
  auto sources = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.alpha_sources.size(); };
  auto keys_mapped = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.alpha_key_source.size(); };
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  falcom_world::g_state.frame.store(7000u);
  resource textures[3] = {};
  uint64_t views[3] = {};
  for (int i = 0; i < 3; ++i) views[i] = SourceView(dev, &textures[i]);
  const int copies_before = cl.copies;
  for (uint64_t i = 0; i < 70; ++i) {
    Capture(0x7800 + i, 30.f + static_cast<float>(i), true);
    pool::NotePoolAlphaMaterial(0x7800 + i, Mat(views[i % 3], 0.5f));
    pool::CapturePoolAlphaSource(&dev, &cl, 0x7800 + i, views[i % 3], true);
  }
  CHECK(cl.copies - copies_before == 3, "70 keys over 3 textures: 3 copies (%d)", cl.copies - copies_before);
  CHECK(sources() == 3u && keys_mapped() == 70u, "3 copies, 70 keys mapped (%zu, %zu)", sources(), keys_mapped());
  for (int i = 0; i < 20; ++i) present();
  size_t slices = 0;
  for (uint64_t i = 0; i < 70; ++i) slices += data->alpha.slot_of_uid.count(p.meshes[MeshOf(0x7800 + i)].uid);
  CHECK(slices == 70u, "all 70 keys have a slice (%zu)", slices);
  CHECK(cl.u0 == 0u, "the atlas UAV is unbound at u0 (%llu)", (unsigned long long)cl.u0);
  // Texture 0 dies: its copy and its 24 keys go; the other two copies stay.
  pool::OnDestroyResourcePool(&dev, textures[0]);
  dev.destroy_resource(textures[0]);
  present();
  CHECK(sources() == 2u && keys_mapped() == 46u, "texture 0 dies: its copy and 24 keys freed (%zu copies, %zu keys)", sources(), keys_mapped());
  pool::ResetWorldPool();
  present();
}

// FIX D and the view guard. An alpha draw whose bound t0 is not the resource of its view is refused and counted,
// with no copy. A draw that is refused by the copy schedule (Cooldown) makes at most one copy over five presents.
static void TestDrawCopies(Device& dev, CmdList& cl, Queue& queue) {
  RegisterUvLayout();
  pool::ResetWorldPool();
  auto& p = pool::g_pool;
  p.scan_active.store(true);
  p.exclude_moving.store(false);
  p.follow_moving.store(false);
  p.alpha_foliage.store(true);
  falcom_world::g_state.frame.store(9000u);

  const uint64_t kRigid = 0xE1, kAlpha = 0xE3;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kAlpha, "0x137F316A.ps.cso", false);
  resource t15, b1, vb, ib, material;
  const uint64_t ring_elements = 4096;
  dev.create_resource(resource_desc(ring_elements * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::index_buffer), nullptr, resource_usage::general, &ib);
  const fixture::TestMesh box = fixture::Box(1.f);
  for (size_t i = 0; i < box.positions.size(); ++i) {
    const float vertex[5] = {box.positions[i][0], box.positions[i][1], box.positions[i][2],
                             box.positions[i][1] * 0.5f + 0.5f, box.positions[i][2] * 0.5f + 0.5f};
    std::memcpy(dev.res[vb.handle].bytes.data() + i * kUvStride, vertex, sizeof(vertex));
  }
  fixture::WriteIndices(dev.res[ib.handle].bytes);
  std::array<uint8_t, 160> material_bytes = {};
  const float threshold = 0.5f;
  std::memcpy(material_bytes.data() + kReferenceThresholdOffset, &threshold, sizeof(float));
  material = TrackedCb(dev, material_bytes.data(), 160);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  resource alpha_tex = {0};
  const uint64_t alpha_view = SourceView(dev, &alpha_tex);
  resource other_tex = {0};
  SourceView(dev, &other_tex);
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = alpha_tex.handle;
  cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = alpha_view;

  uint64_t cursor = 0;
  auto draw_once = [&]() {
    if (cursor + 1 >= ring_elements) cursor = 0;
    float world[12] = {1, 0, 0, 10, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160, world, 48);
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160 + 48, world, 48);
    const int32_t base = static_cast<int32_t>(cursor);
    const int32_t cb[4] = {base, 0, 0, 0};
    std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
    falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);
    falcom_world::DrawRecord draw;
    draw.method = 1; draw.has_index_buffer = true; draw.vb = {vb.handle}; draw.vb_stride = kUvStride; draw.ib = ib;
    draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {kUvLayout};
    draw.index_size = 2; draw.dsv = {0x77}; draw.index_count = 36; draw.first_index = 0;
    draw.instance_count = 1; draw.depth_enable = true; draw.depth_write = true;
    draw.topology = primitive_topology::triangle_list; draw.vs_pipeline = kRigid; draw.ps_pipeline = kAlpha;
    draw.vs_hash = static_cast<uint32_t>(kRigid);
    pool::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
    cursor += 8;
  };
  auto refused_view = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.stats.alpha_source_refused_view; };
  auto copies_stat = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.stats.alpha_source_copies; };

  // Guard: t0 is another resource than the one the view belongs to. Refused and counted; no copy.
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = other_tex.handle;
  const uint64_t refused_before = refused_view();
  const int copies_guard = cl.copies;
  draw_once();
  CHECK(refused_view() == refused_before + 1u && cl.copies == copies_guard, "view of another resource: refused and counted (%llu, copies %d)",
        (unsigned long long)(refused_view() - refused_before), cl.copies - copies_guard);
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = alpha_tex.handle;

  // Cooldown: the first queued draw copies; the same identity drawn over five presents copies at most once more.
  pool::ResetWorldPool();
  p.scan_active.store(true);
  p.alpha_foliage.store(true);
  const int copies_before = cl.copies;
  const int texture_creates_before = dev.texture_creates;
  const uint64_t copy_stat_before = copies_stat();
  for (int i = 0; i < 5; ++i) {
    draw_once();
    falcom_world::g_state.frame.fetch_add(1u);
    pool::DrainPoolScan(&dev, &queue);
  }
  CHECK(cl.copies - copies_before <= 1 && dev.texture_creates - texture_creates_before <= 1 && copies_stat() - copy_stat_before <= 1,
        "cooldown-refused key over 5 presents: at most one copy and one create (copies %d, creates %d)",
        cl.copies - copies_before, dev.texture_creates - texture_creates_before);
}

// FIX F. A capture without UVs against one with UVs is not a mismatch; a real UV difference still is.
static void TestCompare() {
  size_t first = std::numeric_limits<size_t>::max(), differing = 0;
  pool::ComparePoolMeshes(BoxMesh(1.f, false), BoxMesh(1.f, true), &first, &differing);
  CHECK(differing == 0u, "capture without UVs vs with UVs: no mismatch (%zu)", differing);
  first = std::numeric_limits<size_t>::max(); differing = 0;
  pool::PoolDecodedMesh moved = BoxMesh(1.f, true);
  moved.uvs[0][0] += 0.25f;
  pool::ComparePoolMeshes(BoxMesh(1.f, true), moved, &first, &differing);
  CHECK(differing > 0u, "a real UV difference is still counted (%zu)", differing);
}

// Diagnostics round: vertex input UV verdicts, conflict masks, the dump keys.
static renodx::utils::scene::InputElementCopy DiagElement(const char* semantic, uint32_t index, uint32_t slot, uint32_t offset,
                                                          reshade::api::format format) {
  renodx::utils::scene::InputElementCopy element;
  element.semantic = semantic;
  element.semantic_index = index;
  element.buffer_binding = slot;
  element.offset = offset;
  element.format = format;
  return element;
}

static renodx::utils::scene::InputLayoutInfo DiagLayout(const std::vector<renodx::utils::scene::InputElementCopy>& elements) {
  renodx::utils::scene::InputLayoutInfo layout;
  for (const auto& element : elements) layout.elements.push_back(element);
  return layout;
}

// The real 0x9FF8E4BE input signature (.falcom-dev/bytecode): POSITION0, NORMAL0, TEXCOORD0, COLOR1 (float4), SV_InstanceID.
// COLOR1 in slot 0 (interleaved) or in slot 1 must not move POSITION, NORMAL or TEXCOORD0: BuildMeshLayout reads each by its
// own offset, skips the other slots and ignores COLOR.
static void TestColor1Layout() {
  for (uint32_t color_slot = 0u; color_slot < 2u; ++color_slot) {
    const bool interleaved = color_slot == 0u;
    const auto color = DiagElement("COLOR", 1u, color_slot, interleaved ? 32u : 0u, format::r32g32b32a32_float);
    const auto info = DiagLayout({DiagElement("POSITION", 0u, 0u, 0u, format::r32g32b32_float),
                                  DiagElement("NORMAL", 0u, 0u, 12u, format::r32g32b32_float),
                                  DiagElement("TEXCOORD", 0u, 0u, 24u, format::r32g32_float), color});
    renodx::utils::scene::MeshLayout mesh_layout;
    const bool built = renodx::utils::scene::BuildMeshLayout(info, interleaved ? 48u : 32u, 2u, &mesh_layout);
    CHECK(built && mesh_layout.pos_off == 0 && mesh_layout.norm_off == 12 && mesh_layout.uv_off == 24 && !mesh_layout.heuristic,
          "COLOR1 in slot %u: pos %d normal %d uv %d (heuristic %d)", color_slot, mesh_layout.pos_off, mesh_layout.norm_off,
          mesh_layout.uv_off, mesh_layout.heuristic ? 1 : 0);
  }
}

static void TestUvLayoutDiag() {
  using pool::PoolUvVerdict;
  const auto position = DiagElement("POSITION", 0u, 0u, 0u, format::r32g32b32_float);
  const auto texcoord0 = DiagElement("TEXCOORD", 0u, 0u, 12u, format::r32g32_float);
  pool::PoolAlphaLayout layout;
  CHECK(layout.verdict == PoolUvVerdict::NotEvaluated && !layout.valid, "default layout is not evaluated");

  // Two streams: POSITION in slot 0, TEXCOORD in slot 1.
  const auto two_stream = DiagLayout({position, DiagElement("TEXCOORD", 0u, 1u, 0u, format::r32g32_float)});
  pool::ClassifyPoolUvLayout(two_stream, 12u, &layout);
  CHECK(layout.valid && layout.verdict == PoolUvVerdict::TexcoordOtherSlot, "two-stream: texcoord_other_slot");
  renodx::utils::scene::MeshLayout mesh_layout;
  CHECK(renodx::utils::scene::BuildMeshLayout(two_stream, 12u, 2u, &mesh_layout) && mesh_layout.uv_off == -1,
        "two-stream: BuildMeshLayout uv_off -1");
  renodx::utils::scene::shared.data->input_layouts[0x7A1u] = two_stream;
  falcom_world::DrawRecord draw;
  draw.method = 1; draw.has_index_buffer = true; draw.vb = {0x1111}; draw.vb_stride = 12u; draw.ib = {0x2222};
  draw.index_size = 2; draw.index_count = 36; draw.input_layout = {0x7A1u};
  int32_t pos_offset = -1, uv_offset = -2;
  format pos_format = format::unknown, uv_format = format::unknown;
  CHECK(pool::ResolvePoolMeshLayout(draw, &pos_offset, &pos_format, &uv_offset, &uv_format) == nullptr && uv_offset == -1,
        "two-stream: ResolvePoolMeshLayout uv_offset -1");

  pool::ClassifyPoolUvLayout(DiagLayout({position}), 12u, &layout);
  CHECK(layout.verdict == PoolUvVerdict::NoTexcoord && !layout.uv_exists, "no TEXCOORD: no_texcoord");
  CHECK(layout.element_total == 1u && layout.elements[0].slot == 0u && layout.elements[0].format == static_cast<uint32_t>(format::r32g32b32_float),
        "elements recorded (%u)", layout.element_total);

  pool::ClassifyPoolUvLayout(DiagLayout({position, texcoord0}), 20u, &layout);
  CHECK(layout.verdict == PoolUvVerdict::Ok && layout.uv_offset == 12u && layout.vertex_stride == 20u, "offset 12 stride 20: ok");
  pool::ClassifyPoolUvLayout(DiagLayout({position, texcoord0}), 16u, &layout);
  CHECK(layout.verdict == PoolUvVerdict::TexcoordBeyondStride, "offset 12 stride 16: texcoord_beyond_stride");
  pool::ClassifyPoolUvLayout(DiagLayout({DiagElement("TEXCOORD", 0u, 0u, 0u, format::r32_float)}), 8u, &layout);
  CHECK(layout.verdict == PoolUvVerdict::TexcoordUnsupportedFormat, "r32_float: texcoord_unsupported_format");
  pool::ClassifyPoolUvLayout(DiagLayout({DiagElement("texcoord", 0u, 0u, 0u, format::r32g32_float)}), 8u, &layout);
  CHECK(layout.verdict == PoolUvVerdict::Ok, "lower-case semantic matches (case-insensitive)");
}

// Conflict masks (kPoolAlphaDiff*) of a key from two notes, and the first-conflict record.
static void TestConflictMasks() {
  pool::ResetWorldPool();
  pool::g_pool.alpha_foliage.store(true);
  std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
  auto& p = pool::g_pool;
  const auto mask_of = [&](uint64_t key, const pool::PoolAlphaMaterial& first, const pool::PoolAlphaMaterial& second) {
    pool::NotePoolAlphaMaterial(key, first);
    pool::NotePoolAlphaMaterial(key, second);
    return p.alpha_keys.at(key).conflict_fields;
  };
  CHECK(mask_of(0xE001, Mat(0x100, 0.5f), Mat(0x101, 0.5f)) == 1u, "view only: mask 1");
  CHECK(mask_of(0xE002, Mat(0x100, 0.5f), Mat(0x100, 0.6f)) == 4u, "threshold only: mask 4");
  CHECK(mask_of(0xE003, Mat(0x100, 0.5f, 0.1f), Mat(0x100, 0.5f, 0.7f)) == 8u, "scroll only: mask 8");
  CHECK(mask_of(0xE004, Mat(0x100, 0.5f, 0.1f, 3u), Mat(0x100, 0.5f, 0.1f, 4u)) == 2u, "swizzle only: mask 2");
  pool::PoolAlphaMaterial other_resource = Mat(0x101, 0.5f);
  other_resource.resource = 0x201u;
  pool::PoolAlphaMaterial first_resource = Mat(0x100, 0.5f);
  first_resource.resource = 0x200u;
  CHECK(mask_of(0xE005, first_resource, other_resource) == 17u, "view with different resources: mask 17");
  CHECK(p.alpha_keys.at(0xE005).conflict, "a view conflict sets the conflict flag");

  // The first conflict is kept: a third differing note adds bits but does not replace conflict_incoming.
  pool::NotePoolAlphaMaterial(0xE006, Mat(0x100, 0.5f));
  pool::NotePoolAlphaMaterial(0xE006, Mat(0x100, 0.6f));
  pool::NotePoolAlphaMaterial(0xE006, Mat(0x100, 0.5f, 0.1f, 4u));
  CHECK(p.alpha_keys.at(0xE006).conflict_fields == 6u, "third note adds its bit: mask 6 (%u)", p.alpha_keys.at(0xE006).conflict_fields);
  CHECK(p.alpha_keys.at(0xE006).conflict_incoming.threshold == 0.6f && p.alpha_keys.at(0xE006).conflict_incoming.swizzle == 3u,
        "third note does not overwrite conflict_incoming");
  CHECK(p.alpha_keys.at(0xE006).material.threshold == 0.5f, "the first material is kept");

  // Two keys on one mesh: the second key's mask folds into the mesh with the first key's.
  Capture(0xE101, 16.f, true);
  CHECK(MeshOf(0xE101) != UINT32_MAX, "mesh captured");
  pool::NotePoolAlphaMaterial(0xE101, Mat(0x100, 0.5f));
  pool::NotePoolAlphaMaterial(0xE101, Mat(0x100, 0.6f));
  Capture(0xE102, 16.f, true);
  CHECK(MeshOf(0xE102) == MeshOf(0xE101), "same content merged");
  pool::NotePoolAlphaMaterial(0xE102, Mat(0x100, 0.5f, 0.1f, 4u));
  CHECK(p.meshes[MeshOf(0xE101)].alpha_state.conflict_fields == 6u, "mesh folds key masks and the second key's mask (%u)",
        p.meshes[MeshOf(0xE101)].alpha_state.conflict_fields);
}

// The dump carries the UV verdict and conflict fields of an alpha mesh (world_pool_meshes.json).
// Per-vertex UV of the two-stream fixture: distinct per vertex, so a wrong vertex or a wrong stride shows.
static std::array<float, 2> TwoStreamUv(size_t i) {
  return {0.01f + 0.1f * static_cast<float>(i), 0.5f + 0.05f * static_cast<float>(i)};
}

struct TwoStreamCase {
  uint32_t uv_stride = 8u;
  uint64_t uv_bytes = 4096u;        // the UV buffer size
  bool destroy_uv_in_uvs = false;   // destroy the UV buffer when its request reaches the Uvs phase
  uint64_t held_cap = 0u;           // 0: the pool default (alpha_held_bytes_max)
};

struct TwoStreamResult {
  bool saw_uvs = false;
  bool destroyed = false;
  size_t meshes = 0u;
  size_t requests = 0u;
  bool uvs_match = false;
  uint32_t failures = 0u;
  std::string error;
  uint64_t no_uv = 0u;
};

// POSITION in slot 0 (stride 12), TEXCOORD0 in slot 1 (case stride and size): the UV stream is copied on
// its own over the vertex range. Runs 60 scan frames of one alpha draw and reports what the pool did.
static TwoStreamResult RunTwoStream(Device& dev, CmdList& cl, Queue& queue, const TwoStreamCase& c) {
  TwoStreamResult result;
  pool::ResetWorldPool();
  auto& p = pool::g_pool;
  p.scan_active.store(true);
  p.exclude_moving.store(false);
  p.follow_moving.store(false);
  p.alpha_foliage.store(true);
  p.alpha_held_bytes_max = c.held_cap != 0u ? c.held_cap : uint64_t{64} << 20;
  const uint64_t kTwoStreamLayout = 0x7A2u, kRigid = 0xF101, kAlpha = 0xF102;
  renodx::utils::scene::InputLayoutInfo two;
  two.elements.push_back(DiagElement("POSITION", 0u, 0u, 0u, format::r32g32b32_float));
  two.elements.push_back(DiagElement("TEXCOORD", 0u, 1u, 0u, format::r32g32_float));
  renodx::utils::scene::shared.data->input_layouts[kTwoStreamLayout] = two;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kAlpha, "0x137F316A.ps.cso", false);

  resource t15, b1, vb0, vb1, ib, material;
  dev.create_resource(resource_desc(4096 * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb0);
  dev.create_resource(resource_desc(c.uv_bytes, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::index_buffer), nullptr, resource_usage::general, &ib);
  const fixture::TestMesh box = fixture::Box(1.f);
  for (size_t i = 0; i < box.positions.size(); ++i) {
    std::memcpy(dev.res[vb0.handle].bytes.data() + i * fixture::kStride, box.positions[i].data(), fixture::kStride);
    if ((i + 1u) * c.uv_stride > c.uv_bytes) continue;
    const auto uv = TwoStreamUv(i);
    std::memcpy(dev.res[vb1.handle].bytes.data() + i * c.uv_stride, uv.data(), sizeof(uv));
  }
  fixture::WriteIndices(dev.res[ib.handle].bytes);
  renodx::utils::scene::SceneCommandListData* scene_cl = nullptr;
  renodx::utils::data::CreateOrGet(static_cast<reshade::api::command_list*>(&cl), scene_cl);
  scene_cl->vertex_buffers.resize(2);
  scene_cl->vertex_buffers[0] = {vb0, 0u, fixture::kStride};
  scene_cl->vertex_buffers[1] = {vb1, 0u, c.uv_stride};

  std::array<uint8_t, 160> material_bytes = {};
  const float threshold = 0.5f;
  std::memcpy(material_bytes.data() + kReferenceThresholdOffset, &threshold, sizeof(float));
  material = TrackedCb(dev, material_bytes.data(), 160);
  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  resource alpha_tex = {0};
  const uint64_t alpha_view = SourceView(dev, &alpha_tex);
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = alpha_tex.handle;
  cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = alpha_view;

  falcom_world::DrawRecord draw;
  draw.method = 1; draw.has_index_buffer = true; draw.vb = {vb0.handle}; draw.vb_stride = fixture::kStride; draw.ib = ib;
  draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {kTwoStreamLayout};
  draw.index_size = 2; draw.dsv = {0x77}; draw.index_count = 36; draw.first_index = 0;
  draw.instance_count = 1; draw.depth_enable = true; draw.depth_write = true;
  draw.topology = primitive_topology::triangle_list; draw.vs_pipeline = kRigid; draw.ps_pipeline = kAlpha;
  draw.vs_hash = static_cast<uint32_t>(kRigid);
  for (uint32_t frame = 0; frame < 60u; ++frame) {
    const uint64_t cursor = frame % 4000u;
    float world[12] = {1, 0, 0, 10, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160, world, 48);
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160 + 48, world, 48);
    const int32_t cb[4] = {static_cast<int32_t>(cursor), 0, 0, 0};
    std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
    falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);
    pool::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
    falcom_world::g_state.frame.fetch_add(1u);
    pool::DrainPoolScan(&dev, &queue);
    bool destroy_now = false;
    {
      std::lock_guard<std::mutex> lock(p.mutex);
      const auto it = p.mesh_requests.find(pool::PoolMeshKey(draw));
      if (it != p.mesh_requests.end() && it->second.phase == pool::PoolMeshPhase::Uvs) {
        result.saw_uvs = true;
        destroy_now = c.destroy_uv_in_uvs && !result.destroyed;
      }
    }
    if (destroy_now) {
      // No further draws or drains: only the destroy event can drop the request.
      result.destroyed = true;
      dev.destroy_resource(vb1);
      pool::OnDestroyResourcePool(&dev, vb1);
      break;
    }
  }
  std::lock_guard<std::mutex> lock(p.mutex);
  result.meshes = p.meshes.size();
  result.requests = p.mesh_requests.size();
  result.failures = p.stats.mesh_failures;
  result.error = p.stats.last_mesh_error;
  result.no_uv = p.stats.alpha_no_uv;
  if (result.meshes == 1u) {
    const auto& mesh = p.meshes[0];
    result.uvs_match = mesh.uvs.size() == mesh.positions.size();
    for (size_t k = 0; result.uvs_match && k < mesh.positions.size(); ++k) {
      size_t i = 0;
      while (i < box.positions.size() && box.positions[i] != mesh.positions[k]) ++i;
      result.uvs_match = i < box.positions.size() && mesh.uvs[k] == TwoStreamUv(i);
    }
  }
  return result;
}

static void TestTwoStreamUv(Device& dev, CmdList& cl, Queue& queue) {
  // UVs come from slot 1: each mesh vertex carries the UV of its fixture vertex, and the Uvs phase ran.
  const TwoStreamResult good = RunTwoStream(dev, cl, queue, TwoStreamCase{});
  CHECK(good.saw_uvs, "two-stream: the request reached the uvs phase");
  CHECK(good.meshes == 1u && good.uvs_match, "two-stream: each vertex has its own UV (meshes %zu)", good.meshes);
  CHECK(good.no_uv == 0u, "two-stream: no no_uv refusal: %llu", static_cast<unsigned long long>(good.no_uv));

  // A UV stream of another stride (C3): read at its own stride.
  TwoStreamCase wide;
  wide.uv_stride = 16u;
  const TwoStreamResult stride16 = RunTwoStream(dev, cl, queue, wide);
  CHECK(stride16.meshes == 1u && stride16.uvs_match, "uv stride 16: each vertex has its own UV (meshes %zu)", stride16.meshes);

  // The UV buffer is shorter than the vertex range: refused and counted, no mesh.
  TwoStreamCase shortbuf;
  shortbuf.uv_bytes = 32u;
  const TwoStreamResult shorted = RunTwoStream(dev, cl, queue, shortbuf);
  CHECK(shorted.meshes == 0u && shorted.failures >= 1u && shorted.error.find("uv range outside") != std::string::npos,
        "short uv buffer: refused and counted (%u failures, '%s')", shorted.failures, shorted.error.c_str());
  CHECK(shorted.requests == 0u, "short uv buffer: its request and vertex bytes are dropped");

  // The UV buffer is destroyed while its request is in the Uvs phase: the request is dropped, no mesh.
  TwoStreamCase gone;
  gone.destroy_uv_in_uvs = true;
  const TwoStreamResult destroyed = RunTwoStream(dev, cl, queue, gone);
  CHECK(destroyed.saw_uvs && destroyed.destroyed, "destroy: the request reached the uvs phase");
  CHECK(destroyed.meshes == 0u && destroyed.requests == 0u, "destroy: request dropped, no mesh (meshes %zu, requests %zu)",
        destroyed.meshes, destroyed.requests);

  // Held vertex bytes over the cap (C4): the Vertices-to-Uvs step is refused and counted, the request dropped.
  TwoStreamCase capped;
  capped.held_cap = 8u;
  const TwoStreamResult over = RunTwoStream(dev, cl, queue, capped);
  CHECK(over.meshes == 0u && over.failures >= 1u && over.error.find("held vertex bytes") != std::string::npos,
        "held cap: refused and counted (%u failures, '%s')", over.failures, over.error.c_str());
  CHECK(over.requests == 0u, "held cap: refused request dropped with its vertex bytes");

  // The stride is part of the buffer key: a draw with another stride does not serve these copies (C3).
  falcom_world::DrawRecord key_draw;
  key_draw.vb = {0x1111}; key_draw.ib = {0x2222}; key_draw.index_size = 2;
  pool::PoolUvStream narrow;
  narrow.buffer = {0x9001}; narrow.stride = 8u; narrow.format = format::r32g32_float; narrow.size = 4096u;
  pool::PoolUvStream other = narrow;
  other.stride = 16u;
  CHECK(pool::PoolBufferKey(key_draw, narrow) != pool::PoolBufferKey(key_draw, other), "buffer key differs by uv stride");
}

// A queued key seen again with another TEXCOORD0 buffer is counted (its request keeps the first stream).
static void TestUvStreamMismatch() {
  pool::ResetWorldPool();
  auto& p = pool::g_pool;
  falcom_world::DrawRecord draw;
  draw.method = 1; draw.has_index_buffer = true; draw.vb = {0x1111}; draw.vb_stride = fixture::kStride; draw.ib = {0x2222};
  draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {fixture::kLayout};
  draw.index_size = 2; draw.index_count = 36; draw.first_index = 0; draw.instance_count = 1;
  pool::PoolUvStream first;
  first.buffer = {0x9001}; first.stride = 8u; first.format = format::r32g32_float; first.size = 4096u;
  pool::PoolUvStream second = first;
  second.buffer = {0x9002};
  std::lock_guard<std::mutex> lock(p.mutex);
  pool::QueuePoolMesh(0x7701, 0xE1, draw, false, first);
  CHECK(p.stats.alpha_uv_stream_mismatch == 0u, "same stream: not counted");
  pool::QueuePoolMesh(0x7701, 0xE1, draw, false, second);
  CHECK(p.stats.alpha_uv_stream_mismatch == 1u, "other TEXCOORD0 buffer: counted (%llu)", static_cast<unsigned long long>(p.stats.alpha_uv_stream_mismatch));
  pool::QueuePoolMesh(0x7701, 0xE1, draw, false, first);
  CHECK(p.stats.alpha_uv_stream_mismatch == 1u, "back to the request's buffer: not counted");
}

static void TestDumpEvidence() {
  pool::ResetWorldPool();
  pool::g_pool.alpha_foliage.store(true);
  {
    std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
    pool::NotePoolAlphaMaterial(0xF001, Mat(0x100, 0.5f));
    pool::NotePoolAlphaMaterial(0xF001, Mat(0x101, 0.5f));
    pool::NotePoolAlphaMaterial(0xF002, Mat(0x100, 0.5f));
  }
  Capture(0xF001, 17.f, true);
  pool::DumpWorldPool();
  std::ifstream meshes(pool::PoolOutputDir() / "world_pool_meshes.json");
  const std::string meshes_text((std::istreambuf_iterator<char>(meshes)), std::istreambuf_iterator<char>());
  CHECK(meshes_text.find("\"uv_verdict\": \"not_evaluated\"") != std::string::npos, "meshes: uv_verdict written");
  CHECK(meshes_text.find("\"conflict_fields\": 1, \"conflict_first\"") != std::string::npos, "meshes: conflict_fields written");
  CHECK(meshes_text.find("\"vbs_known\": false") != std::string::npos, "meshes: vbs_known written");
  std::ifstream summary(pool::PoolOutputDir() / "world_pool.json");
  const std::string summary_text((std::istreambuf_iterator<char>(summary)), std::istreambuf_iterator<char>());
  CHECK(summary_text.find("\"alpha_keys_total\": 2") != std::string::npos, "summary: alpha_keys_total 2");
  CHECK(summary_text.find("\"alpha_keys\": [") != std::string::npos && summary_text.find("\"mesh_id\": ") != std::string::npos,
        "summary: alpha_keys entries carry mesh_id");
  CHECK(LogCount("alpha diag: keys=") > 0u && LogCount("conflict_swizzle=") > 0u, "log: alpha diag line carries conflict_swizzle");
  CHECK(LogCount("conflict_scroll=") > 0u && LogCount("conflict_any=") > 0u, "log: alpha diag line carries conflict_scroll and conflict_any");
  CHECK(summary_text.find("\"alpha_reason\": \"ok\", \"alpha_threshold_offset\": 112") != std::string::npos,
        "summary: alpha_tested pixel_shaders carry alpha_reason and alpha_threshold_offset");
}

// Two textures get two source ids and two texture links; destroying one removes only its link.
static void TestSourceLinks(Device& dev, CmdList& cl) {
  auto& p = pool::g_pool;
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  resource tex_a = {0};
  resource tex_b = {0};
  const uint64_t view_a = SourceView(dev, &tex_a);
  const uint64_t view_b = SourceView(dev, &tex_b);
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7801, view_a, true);
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7802, view_b, true);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    const uint64_t id_a = p.alpha_key_source.at(0x7801);
    const uint64_t id_b = p.alpha_key_source.at(0x7802);
    CHECK(id_a != id_b && p.alpha_sources.size() == 2u, "two textures, two source ids (%llu, %llu)", (unsigned long long)id_a, (unsigned long long)id_b);
    CHECK(p.alpha_source_by_texture.at(tex_a.handle) == id_a && p.alpha_source_by_texture.at(tex_b.handle) == id_b,
          "each texture links to its own id");
  }
  dev.destroy_resource(tex_a);
  pool::OnDestroyResourcePool(&dev, tex_a);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_source_by_texture.count(tex_a.handle) == 0u, "destroyed texture: link removed");
    CHECK(p.alpha_source_by_texture.count(tex_b.handle) == 1u && p.alpha_sources.size() == 1u, "other texture keeps its link and copy");
  }
  pool::ResetWorldPool();
}

// Wind rest pose (opaque PS, alpha_wind_opaque and alpha_foliage on): the wind draw is admitted. Switching alpha_wind_opaque
// off retires its instances at the next present (ApplyPoolWindSwitch) and no wind draw is copied or admitted again. The rigid
// draw keeps its instance: it has its own vertex buffer, so its mesh key differs from the wind mesh.
static void TestWindScanPath(Device& dev, CmdList& cl, Queue& queue) {
  RegisterUvLayout();
  auto& p = pool::g_pool;
  pool::ResetWorldPool();
  p.scan_active.store(true);
  p.exclude_moving.store(false);
  p.follow_moving.store(false);
  p.alpha_foliage.store(true);
  p.alpha_wind_opaque.store(true);

  const uint64_t kWind = 0xC8, kRigid = 0xC9, kOpaque = 0xCA;
  Classify(&dev, kWind, "0x9FF8E4BE.vs.cso", true);
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kOpaque, "0x2162672F.ps.cso", false);

  resource t15, b1, vb_wind, vb_rigid, ib;
  dev.create_resource(resource_desc(4096 * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb_wind);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb_rigid);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::index_buffer), nullptr, resource_usage::general, &ib);
  // The rigid geometry differs from the wind geometry: identical content shares one mesh (signature dedup), and retiring
  // the wind mesh would then retire the rigid instance too.
  const auto fill = [&](uint64_t handle, const fixture::TestMesh& mesh) {
    for (size_t i = 0; i < mesh.positions.size(); ++i) {
      const float vertex[5] = {mesh.positions[i][0], mesh.positions[i][1], mesh.positions[i][2],
                               mesh.positions[i][1] * 0.5f + 0.5f, mesh.positions[i][2] * 0.5f + 0.5f};
      std::memcpy(dev.res[handle].bytes.data() + i * kUvStride, vertex, sizeof(vertex));
    }
  };
  fill(vb_wind.handle, fixture::Box(1.f));
  fill(vb_rigid.handle, fixture::Box(2.f));
  fixture::WriteIndices(dev.res[ib.handle].bytes);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;
  const auto make_draw = [&](const resource& vb, uint64_t vs) {
    falcom_world::DrawRecord draw;
    draw.method = 1; draw.has_index_buffer = true; draw.vb = {vb.handle}; draw.vb_stride = kUvStride; draw.ib = ib;
    draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {kUvLayout};
    draw.index_size = 2; draw.dsv = {0x77}; draw.index_count = 36; draw.first_index = 0; draw.instance_count = 1;
    draw.depth_enable = true; draw.depth_write = true; draw.topology = primitive_topology::triangle_list;
    draw.vs_pipeline = vs; draw.ps_pipeline = kOpaque; draw.vs_hash = static_cast<uint32_t>(vs);
    return draw;
  };
  uint64_t cursor = 0;
  const auto draw_once = [&](const resource& vb, uint64_t vs) {
    if (cursor + 1 >= 4096) cursor = 0;
    const float world[12] = {1, 0, 0, 10, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160, world, 48);
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160 + 48, world, 48);
    const int32_t base = static_cast<int32_t>(cursor);
    const int32_t cb[4] = {base, 0, 0, 0};
    std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
    falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);
    auto draw = make_draw(vb, vs);
    pool::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
    cursor += 8;
  };
  const auto frames = [&](int count) {
    for (int i = 0; i < count; ++i) {
      draw_once(vb_rigid, kRigid);
      draw_once(vb_wind, kWind);
      falcom_world::g_state.frame.fetch_add(1u);
      pool::DrainPoolScan(&dev, &queue);
    }
  };
  const uint64_t rigid_key = pool::PoolMeshKey(make_draw(vb_rigid, kRigid));
  const uint64_t wind_key = pool::PoolMeshKey(make_draw(vb_wind, kWind));
  const auto instances_of = [&](uint64_t key) {
    std::lock_guard<std::mutex> lock(p.mutex);
    size_t count = 0;
    const auto it = p.mesh_by_key.find(key);
    if (it == p.mesh_by_key.end()) return count;
    for (const auto& instance : p.instances) {
      if (instance.mesh_id == it->second) ++count;
    }
    return count;
  };

  frames(40);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.wind_keys.count(wind_key) == 1u && p.wind_keys.count(rigid_key) == 0u, "d2 ON: only the wind key is a wind rest pose (%zu keys)",
          p.wind_keys.size());
  }
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.stats.wind_rest_draws > 0u, "d2 ON: wind rest draws counted (%llu)", static_cast<unsigned long long>(p.stats.wind_rest_draws));
  }
  const size_t wind_on = instances_of(wind_key);
  const size_t rigid_on = instances_of(rigid_key);
  CHECK(wind_on >= 1u && rigid_on >= 1u, "d2 ON: wind rest pose and rigid admitted (wind %zu, rigid %zu)", wind_on, rigid_on);

  // d2 OFF: the next present retires the wind instance; the rigid instance stays.
  p.alpha_wind_opaque.store(false);
  const uint64_t removed_before = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.stats.alpha_removed; }();
  frames(1);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    // The key stays while OFF (AdmitPoolInstance keeps refusing it); only the instance is removed.
  CHECK(p.wind_keys.count(wind_key) == 1u && p.stats.alpha_removed > removed_before, "d2 OFF: wind key kept, instance removed (alpha_removed %llu)",
          static_cast<unsigned long long>(p.stats.alpha_removed));
  }
  CHECK(instances_of(wind_key) == 0u, "d2 OFF: wind instance gone at the next present (%zu)", instances_of(wind_key));
  CHECK(instances_of(rigid_key) >= 1u, "d2 OFF: rigid instance kept (%zu)", instances_of(rigid_key));
  frames(10);
  CHECK(instances_of(wind_key) == 0u, "d2 OFF, 10 more presents: no wind instance admitted (%zu)", instances_of(wind_key));
  CHECK(instances_of(rigid_key) >= 1u, "d2 OFF, 10 more presents: rigid instance still admitted (%zu)", instances_of(rigid_key));

  // B1: a wind copy recorded with d2 ON resolves after d2 (or the main switch) goes OFF. It is refused: no wind instance
  // appears in the 10 presents after the switch.
  for (const bool main_off : {false, true}) {
    pool::ResetWorldPool();
    p.scan_active.store(true);
    p.exclude_moving.store(false);
    p.follow_moving.store(false);
    p.alpha_foliage.store(true);
    p.alpha_wind_opaque.store(true);
    frames(31);  // the wind key is not admitted yet: its copies are still counting observations
    {
      std::lock_guard<std::mutex> lock(p.mutex);
      CHECK(p.wind_keys.count(wind_key) == 1u, "%s: wind key recorded before the switch", main_off ? "main OFF" : "d2 OFF");
    }
    draw_once(vb_wind, kWind);  // the copy in flight: recorded while ON
    if (main_off) {
      p.alpha_foliage.store(false);
    } else {
      p.alpha_wind_opaque.store(false);
    }
    falcom_world::g_state.frame.fetch_add(1u);
    pool::DrainPoolScan(&dev, &queue);
    size_t admitted = 0u;
    for (int i = 0; i < 10; ++i) {
      frames(1);
      admitted += instances_of(wind_key);
    }
    CHECK(admitted == 0u, "%s: copy in flight at the switch, wind instance never admitted in 10 presents (%zu)",
          main_off ? "main OFF" : "d2 OFF", admitted);
    p.alpha_foliage.store(true);
  }

  pool::ResetWorldPool();
  p.alpha_wind_opaque.store(false);
}

// COLOR1 in a wind draw's input layout (the 0x9FF8E4BE signature): interleaved in slot 0 (stride 48, offset 32) or in slot 1
// (slot 0 stride 32). The wind draw with an alpha PS and alpha_foliage on captures the slot-0 POSITION and TEXCOORD0 streams:
// the mesh is admitted with the box positions and one UV per vertex.
static void TestColor1ScanPath(Device& dev, CmdList& cl, Queue& queue, bool interleaved) {
  auto& p = pool::g_pool;
  pool::ResetWorldPool();
  p.scan_active.store(true);
  p.exclude_moving.store(false);
  p.follow_moving.store(false);
  p.alpha_foliage.store(true);
  p.alpha_wind_opaque.store(true);

  const uint64_t kWind = 0xCB, kAlpha = 0xCC, kColorLayout = 0x5201u;
  const uint32_t stride = interleaved ? 48u : 32u;
  Classify(&dev, kWind, "0x9FF8E4BE.vs.cso", true);
  Classify(&dev, kAlpha, "0x137F316A.ps.cso", false);
  renodx::utils::scene::InputLayoutInfo layout;
  layout.elements.push_back(DiagElement("POSITION", 0u, 0u, 0u, format::r32g32b32_float));
  layout.elements.push_back(DiagElement("NORMAL", 0u, 0u, 12u, format::r32g32b32_float));
  layout.elements.push_back(DiagElement("TEXCOORD", 0u, 0u, 24u, format::r32g32_float));
  layout.elements.push_back(interleaved ? DiagElement("COLOR", 1u, 0u, 32u, format::r32g32b32a32_float)
                                        : DiagElement("COLOR", 1u, 1u, 0u, format::r32g32b32a32_float));
  renodx::utils::scene::shared.data->input_layouts[kColorLayout] = layout;

  resource t15, b1, vb, ib, material;
  dev.create_resource(resource_desc(4096 * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::index_buffer), nullptr, resource_usage::general, &ib);
  const fixture::TestMesh box = fixture::Box(1.f);
  for (size_t i = 0; i < box.positions.size(); ++i) {
    const float slot0[8] = {box.positions[i][0], box.positions[i][1], box.positions[i][2], 0.f, 1.f, 0.f,
                            box.positions[i][1] * 0.5f + 0.5f, box.positions[i][2] * 0.5f + 0.5f};
    std::memcpy(dev.res[vb.handle].bytes.data() + i * stride, slot0, sizeof(slot0));
    if (interleaved) {
      const float color[4] = {0.1f, 0.2f, 0.3f, 1.f};
      std::memcpy(dev.res[vb.handle].bytes.data() + i * stride + 32, color, sizeof(color));
    }
  }
  fixture::WriteIndices(dev.res[ib.handle].bytes);
  std::array<uint8_t, 160> material_bytes = {};
  const float threshold = 0.5f;
  std::memcpy(material_bytes.data() + kReferenceThresholdOffset, &threshold, sizeof(float));
  material = TrackedCb(dev, material_bytes.data(), 160);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  resource alpha_tex = {0};
  const uint64_t alpha_view = SourceView(dev, &alpha_tex);
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = alpha_tex.handle;
  cl_data.ps_srv_view[contract::kAlphaTexSlot].handle = alpha_view;

  const auto make_draw = [&]() {
    falcom_world::DrawRecord draw;
    draw.method = 1; draw.has_index_buffer = true; draw.vb = {vb.handle}; draw.vb_stride = stride; draw.ib = ib;
    draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {kColorLayout};
    draw.index_size = 2; draw.dsv = {0x77}; draw.index_count = 36; draw.first_index = 0; draw.instance_count = 1;
    draw.depth_enable = true; draw.depth_write = true; draw.topology = primitive_topology::triangle_list;
    draw.vs_pipeline = kWind; draw.ps_pipeline = kAlpha; draw.vs_hash = static_cast<uint32_t>(kWind);
    return draw;
  };
  uint64_t cursor = 0;
  for (int i = 0; i < 40; ++i) {
    if (cursor + 1 >= 4096) cursor = 0;
    const float world[12] = {1, 0, 0, 10, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160, world, 48);
    std::memcpy(dev.res[t15.handle].bytes.data() + cursor * 160 + 48, world, 48);
    const int32_t base = static_cast<int32_t>(cursor);
    const int32_t cb[4] = {base, 0, 0, 0};
    std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
    falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);
    auto draw = make_draw();
    pool::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
    cursor += 8;
    falcom_world::g_state.frame.fetch_add(1u);
    pool::DrainPoolScan(&dev, &queue);
  }

  const uint64_t key = pool::PoolMeshKey(make_draw());
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    const auto it = p.mesh_by_key.find(key);
    CHECK(it != p.mesh_by_key.end(), "COLOR1 %s: mesh captured", interleaved ? "interleaved in slot 0" : "in slot 1");
    if (it != p.mesh_by_key.end()) {
      const auto& mesh = p.meshes[it->second];
      // The pool compacts vertices in index order, so each captured position is matched to a box vertex and its UV checked.
      bool positions_ok = mesh.positions.size() == box.positions.size() && mesh.uvs.size() == box.positions.size();
      for (size_t i = 0; positions_ok && i < mesh.positions.size(); ++i) {
        const auto& position = mesh.positions[i];
        const std::array<float, 2> uv = {position[1] * 0.5f + 0.5f, position[2] * 0.5f + 0.5f};
        positions_ok = std::find(box.positions.begin(), box.positions.end(), position) != box.positions.end() && mesh.uvs[i] == uv;
      }
      size_t instances = 0;
      for (const auto& instance : p.instances) {
        if (instance.mesh_id == it->second) ++instances;
      }
      CHECK(positions_ok, "COLOR1 %s: positions and one UV per vertex (%zu) match the box (%zu vertices)",
            interleaved ? "interleaved in slot 0" : "in slot 1", mesh.uvs.size(), box.positions.size());
      CHECK(mesh.alpha && instances >= 1u, "COLOR1 %s: alpha mesh admitted (alpha %d, %zu instances)",
            interleaved ? "interleaved in slot 0" : "in slot 1", mesh.alpha ? 1 : 0, instances);
    }
  }
  pool::ResetWorldPool();
  p.alpha_wind_opaque.store(false);
}

// B item 3: with alpha_foliage off, a store change makes no copy of the atlas maps (the dump's copies are taken with it on).
static void TestAlphaMapGate(Device& dev, Queue& queue) {
  auto& p = pool::g_pool;
  pool::ResetWorldPool();
  pool::BvhDeviceData* data = pool::GetBvhDeviceData(&dev);
  p.alpha_foliage.store(false);
  data->store_version += 5u;
  pool::UpdateLiveBvh(&dev, &queue);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_maps.store_version != data->store_version, "main OFF: store change copies no atlas maps (copy %llu, store %llu)",
          static_cast<unsigned long long>(p.alpha_maps.store_version), static_cast<unsigned long long>(data->store_version));
  }
  p.alpha_foliage.store(true);
  data->store_version += 5u;
  pool::UpdateLiveBvh(&dev, &queue);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_maps.store_version == data->store_version, "main ON: store change copies the atlas maps");
  }
  pool::ResetWorldPool();
}

// B item 2: the panel copies PoolAlphaGpuStats every UI frame, so it holds scalars only (the maps are dump-only).
static void TestPanelCopyScalars() {
  CHECK(sizeof(pool::PoolAlphaGpuStats) <= 96u, "PoolAlphaGpuStats holds scalars only (%zu bytes, 96 max)", sizeof(pool::PoolAlphaGpuStats));
}

// B item 4: a keyless (indirect, unresolved) source expires even when its shared path is captured every present, and
// keyless sources are capped at kAlphaKeylessSourcesMax so keyed sources still get copies.
static void TestKeylessSources(Device& dev, CmdList& cl, Queue& queue) {
  auto& p = pool::g_pool;
  auto sources = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.alpha_sources.size(); };
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  p.live_on.store(true);

  resource tex_a;
  const uint64_t view_a = SourceView(dev, &tex_a);
  pool::CapturePoolAlphaSource(&dev, &cl, 0u, view_a, true);  // the keyless copy
  uint64_t first_source = 0u;
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    const auto link = p.alpha_source_by_texture.find(tex_a.handle);
    first_source = link == p.alpha_source_by_texture.end() ? 0u : link->second;
  }
  CHECK(first_source != 0u, "first keyless capture made a copy (source %llu)", static_cast<unsigned long long>(first_source));
  for (uint32_t i = 0; i <= bvh::kAlphaOrphanFrames + 5u; ++i) {
    falcom_world::g_state.frame.fetch_add(1u);
    pool::CapturePoolAlphaSource(&dev, &cl, 0u, view_a, true);  // the same texture again: the shared path, no key
    pool::UpdateLiveBvh(&dev, &queue);
  }
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_sources.count(first_source) == 0u && p.stats.alpha_orphans_expired >= 1u,
          "keyless copy captured on the shared path every present still expires (expired %llu)",
          static_cast<unsigned long long>(p.stats.alpha_orphans_expired));
  }

  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  p.live_on.store(true);
  for (uint32_t i = 0; i < bvh::kAlphaKeylessSourcesMax; ++i) {
    if (i % 4u == 0u) falcom_world::g_state.frame.fetch_add(1u);  // kAlphaCopiesPerFrame
    resource tex;
    const uint64_t view = SourceView(dev, &tex);
    pool::CapturePoolAlphaSource(&dev, &cl, 0u, view, true);
  }
  CHECK(sources() == bvh::kAlphaKeylessSourcesMax, "%u keyless copies are made (sources %zu)", bvh::kAlphaKeylessSourcesMax, sources());
  falcom_world::g_state.frame.fetch_add(1u);
  resource tex_over;
  const uint64_t view_over = SourceView(dev, &tex_over);
  const uint64_t cap_before = [&] { std::lock_guard<std::mutex> lock(p.mutex); return p.stats.alpha_source_refused_cap; }();
  pool::CapturePoolAlphaSource(&dev, &cl, 0u, view_over, true);
  const size_t live = sources();
  uint64_t cap_after = 0u;
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    cap_after = p.stats.alpha_source_refused_cap;
  }
  CHECK(live == bvh::kAlphaKeylessSourcesMax && cap_after == cap_before + 1u,
        "keyless copy number %u refused and counted as cap (sources %zu, cap %llu)", bvh::kAlphaKeylessSourcesMax + 1u, live,
        static_cast<unsigned long long>(cap_after));
  resource tex_key;
  const uint64_t view_key = SourceView(dev, &tex_key);
  pool::CapturePoolAlphaSource(&dev, &cl, 0xA1u, view_key, true);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_key_source.count(0xA1u) == 1u, "keyed copy still made while %u keyless copies are live", bvh::kAlphaKeylessSourcesMax);
  }
  pool::ResetWorldPool();
}

// Refusal reasons (B item 6): each reason is produced by a real refusal, and the four sum to the total.
static void TestRefusalReasons(Device& dev, CmdList& cl) {
  auto& p = pool::g_pool;
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  p.live_on.store(true);
  resource tex_deferred;
  const uint64_t view_deferred = SourceView(dev, &tex_deferred);
  pool::CapturePoolAlphaSource(&dev, &cl, 0xB1u, view_deferred, false);  // deferred: no copy on a deferred list
  resource tex_type;
  const uint64_t view_type = SourceView(dev, &tex_type);
  resource_view unknown = {0u};
  dev.create_resource_view(tex_type, resource_usage::shader_resource, resource_view_desc(format::unknown), &unknown);
  falcom_world::g_state.frame.fetch_add(1u);
  pool::CapturePoolAlphaSource(&dev, &cl, 0xB2u, unknown.handle, true);  // type: the view format is unknown
  (void)view_type;
  falcom_world::g_state.frame.fetch_add(1u);
  for (uint64_t k = 0; k < 5u; ++k) {  // kAlphaCopiesPerFrame is 4: the fifth copy of the frame is refused as cap
    resource tex;
    const uint64_t view = SourceView(dev, &tex);
    pool::CapturePoolAlphaSource(&dev, &cl, 0xB3u + k, view, true);
  }
  resource tex_failed;
  const uint64_t view_failed = SourceView(dev, &tex_failed);
  const int texture_creates_before = dev.texture_creates;
  dev.fail_texture_create = true;
  falcom_world::g_state.frame.fetch_add(1u);
  pool::CapturePoolAlphaSource(&dev, &cl, 0xC0u, view_failed, true);  // failed: the copy cannot be created
  dev.fail_texture_create = false;

  std::lock_guard<std::mutex> lock(p.mutex);
  const auto& s = p.stats;
  const uint64_t split = s.alpha_source_refused_cap + s.alpha_source_refused_deferred + s.alpha_source_refused_type + s.alpha_source_refused_failed;
  std::printf("refusal reasons: total %llu = cap %llu + deferred %llu + type %llu + failed %llu\n",
              static_cast<unsigned long long>(s.alpha_source_refused), static_cast<unsigned long long>(s.alpha_source_refused_cap),
              static_cast<unsigned long long>(s.alpha_source_refused_deferred), static_cast<unsigned long long>(s.alpha_source_refused_type),
              static_cast<unsigned long long>(s.alpha_source_refused_failed));
  CHECK(s.alpha_source_refused_deferred == 1u && s.alpha_source_refused_type == 1u && s.alpha_source_refused_cap == 1u
            && s.alpha_source_refused_failed == 1u,
        "each reason produced once (deferred %llu, type %llu, cap %llu, failed %llu)",
        static_cast<unsigned long long>(s.alpha_source_refused_deferred), static_cast<unsigned long long>(s.alpha_source_refused_type),
        static_cast<unsigned long long>(s.alpha_source_refused_cap), static_cast<unsigned long long>(s.alpha_source_refused_failed));
  CHECK(split == s.alpha_source_refused && s.alpha_source_refused == 4u, "reasons sum to the total (%llu of %llu)",
        static_cast<unsigned long long>(split), static_cast<unsigned long long>(s.alpha_source_refused));
  bool texture_linked = p.alpha_source_by_texture.count(tex_failed.handle) != 0u;
  for (const auto& entry : p.alpha_sources) texture_linked = texture_linked || entry.second.texture == tex_failed.handle;
  CHECK(!texture_linked && p.alpha_source_done.count(0xC0u) == 1u && p.alpha_key_source.count(0xC0u) == 0u,
        "failed copy leaves no source, link or key for its texture");
  CHECK(dev.texture_creates == texture_creates_before, "failed copy created no proxy texture (%d before, %d after)", texture_creates_before,
        dev.texture_creates);
}

// Atlas maps are cleared with alpha OFF and by ResetWorldPool; a keyed share clears the orphan hold of the copy it shares.
static void TestMapsAndSharedHold(Device& dev, CmdList& cl, Queue& queue) {
  auto& p = pool::g_pool;
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    p.alpha_maps.slice_of_uid[5] = 3u;
    p.alpha_maps.resident_uids.insert(5u);
  }
  p.alpha_foliage.store(false);
  pool::UpdateLiveBvh(&dev, &queue);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_maps.slice_of_uid.empty() && p.alpha_maps.resident_uids.empty(), "alpha OFF: atlas maps cleared (%zu slices)",
          p.alpha_maps.slice_of_uid.size());
  }
  p.alpha_foliage.store(true);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    p.alpha_maps.slice_of_uid[6] = 1u;
  }
  pool::ResetWorldPool();
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_maps.slice_of_uid.empty(), "ResetWorldPool: atlas maps cleared (%zu slices)", p.alpha_maps.slice_of_uid.size());
  }

  p.alpha_foliage.store(true);
  p.live_on.store(true);
  resource tex;
  const uint64_t view = SourceView(dev, &tex);
  pool::CapturePoolAlphaSource(&dev, &cl, 0u, view, true);  // keyless copy: held for its resolve
  uint64_t source_id = 0u;
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    const auto link = p.alpha_source_by_texture.find(tex.handle);
    source_id = link == p.alpha_source_by_texture.end() ? 0u : link->second;
  }
  pool::CapturePoolAlphaSource(&dev, &cl, 0xD1u, view, true);  // a keyed draw shares the copy
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    const auto it = p.alpha_sources.find(source_id);
    if (it != p.alpha_sources.end()) it->second.keys.erase(0xD1u);  // the key goes away, as an invalidation would
  }
  uint64_t expired_before = 0u;
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    expired_before = p.stats.alpha_orphans_expired;
  }
  for (uint32_t i = 0; i <= bvh::kAlphaOrphanFrames + 5u; ++i) {
    falcom_world::g_state.frame.fetch_add(1u);
    pool::UpdateLiveBvh(&dev, &queue);
  }
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.stats.alpha_orphans_expired == expired_before, "keyed share: the copy is not counted as an orphan (expired %llu, was %llu)",
          static_cast<unsigned long long>(p.stats.alpha_orphans_expired), static_cast<unsigned long long>(expired_before));
  }
  pool::ResetWorldPool();
}

// Test 10 (S6 diagnostics): the dump order puts the alpha and wind rest-pose instances first beyond the instance cap, and
// the parallel vectors (visibility, last_seen) move with their instances.
static void TestDiagnosticsRound() {
  const size_t count = pool::kPoolDumpMaxInstances + 50u;
  std::vector<uint8_t> first(count, 0u);
  for (size_t i = count - 5u; i < count; ++i) first[i] = 1u;  // the last five instances are alpha or wind rest pose
  const std::vector<size_t> order = pool::PoolDumpOrder(first);
  bool flagged_first = order.size() == count;
  for (size_t i = 0; i < 5u; ++i) flagged_first = flagged_first && order[i] == count - 5u + i;
  CHECK(flagged_first && order[5] == 0u, "alpha instances first beyond the instance cap (order %zu, %zu, first unflagged %zu)", order[0],
        order[4], order[5]);

  const size_t n = 12;
  std::vector<uint8_t> flags(n, 0u);
  flags[7] = 1u;
  flags[10] = 1u;
  std::vector<uint32_t> ids(n), visibility(n), last_seen(n);
  for (size_t i = 0; i < n; ++i) {
    ids[i] = static_cast<uint32_t>(i);
    visibility[i] = static_cast<uint32_t>(i) * 10u + 1u;
    last_seen[i] = static_cast<uint32_t>(i) * 100u;
  }
  const std::vector<size_t> sorted = pool::PoolDumpOrder(flags);
  const auto ids2 = pool::PoolReorder(ids, sorted);
  const auto visibility2 = pool::PoolReorder(visibility, sorted);
  const auto last2 = pool::PoolReorder(last_seen, sorted);
  bool moved_together = true;
  for (size_t i = 0; i < n; ++i) {
    moved_together = moved_together && visibility2[i] == ids2[i] * 10u + 1u && last2[i] == ids2[i] * 100u;
  }
  CHECK(moved_together && ids2[0] == 7u && ids2[1] == 10u && ids2[2] == 0u,
        "visibility and last_seen move with their instances; flagged first (%u %u %u)", ids2[0], ids2[1], ids2[2]);
}

int main() {
  // Atlas (CPU bookkeeping for the GPU array).
  {
    bvh::AlphaSliceTable table;
    for (uint64_t uid = 1; uid <= bvh::kAlphaAtlasSlices; ++uid) {
      CHECK(table.Acquire(uid) == static_cast<int32_t>(uid - 1), "slice for uid %llu", static_cast<unsigned long long>(uid));
    }
    CHECK(table.used == bvh::kAlphaAtlasSlices, "all slices used: %u", table.used);
    CHECK(table.Acquire(1000u) == -1, "cap: refused with every slice taken");
    table.Release(10u);
    CHECK(table.used == bvh::kAlphaAtlasSlices - 1u, "release frees one");
    CHECK(table.Acquire(1001u) == 10, "freed slice is reused");
    table.Release(10u);
    table.Release(10u);
    CHECK(table.used == bvh::kAlphaAtlasSlices - 1u, "a second release of a free slice is ignored");
    table.Release(bvh::kAlphaAtlasSlices);
    CHECK(table.used == bvh::kAlphaAtlasSlices - 1u, "release out of range is ignored");

    const uint8_t texels[4] = {1u, 2u, 3u, 255u};
    CHECK(bvh::PackAlphaTexels(texels) == 0xFF030201u, "texel 0 in the low byte: %08X", bvh::PackAlphaTexels(texels));
    CHECK(sizeof(bvh::AlphaMaterialGPU) == 32u, "material is 32 bytes");
    CHECK(bvh::kAlphaSliceTexelsX == bvh::kAlphaSliceWords * 4u, "four texels per uint");
    std::printf("alpha atlas: %u slices, %u used after the cap checks\n", bvh::kAlphaAtlasSlices, table.used);
  }

  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  fixture::RegisterLayout();

  HStage("classifier");
  TestClassifier(&dev);
  HStage("note material");
  TestNoteMaterial();
  HStage("read material");
  TestReadDraw(dev, cl);
  TestSwizzleBits(dev, cl);
  HStage("switch");
  TestSwitch();
  HStage("decode uv");
  TestDecodeUv();
  HStage("gate");
  TestGate(dev);
  TestPixelUnknownLog(dev, cl);
  HStage("admission");
  TestAdmit();
  HStage("mesh conflict");
  TestMeshConflict();
  HStage("uv layout diagnostics");
  TestUvLayoutDiag();
  TestColor1Layout();
  TestConflictMasks();
  TestDumpEvidence();
  HStage("requeue");
  TestRequeue();
  HStage("reset");
  TestResetClears();
  HStage("scan path");
  TestScanPath(dev, cl, queue);
  HStage("two-stream uv");
  TestTwoStreamUv(dev, cl, queue);
  HStage("uv stream mismatch");
  TestUvStreamMismatch();
  HStage("wind scan path");
  TestWindScanPath(dev, cl, queue);
  HStage("color1 scan path");
  TestColor1ScanPath(dev, cl, queue, true);
  TestColor1ScanPath(dev, cl, queue, false);
  std::printf("sizeof PoolPendingCopy=%zu WorldMesh=%zu PoolAlphaState=%zu PoolAlphaMaterialState=%zu PoolIndirectAlpha=%zu\n",
              sizeof(pool::PoolPendingCopy), sizeof(pool::WorldMesh), sizeof(pool::PoolAlphaState),
              sizeof(pool::PoolAlphaMaterialState), sizeof(pool::PoolIndirectAlpha));
  // Regression guards (OFF memory): a copy carries no vertex-input or UV stream; a mesh carries no layout.
  CHECK(sizeof(pool::PoolPendingCopy) <= 120u, "PoolPendingCopy %zu bytes (120 max)", sizeof(pool::PoolPendingCopy));
  CHECK(sizeof(pool::WorldMesh) <= 288u, "WorldMesh %zu bytes (288 max)", sizeof(pool::WorldMesh));
  HStage("indirect alpha");
  TestIndirectAlpha(dev, cl, queue);
  HStage("atlas (GPU stage, mock)");
  TestAtlas(dev, cl, queue);
  TestSourceLinks(dev, cl);
  TestSharedCopies(dev, cl, queue);
  TestDrawCopies(dev, cl, queue);
  TestCompare();
  HStage("source guards (mock)");
  TestSourceGuards(dev, cl, queue);
  HStage("end to end (mock)");
  TestEndToEnd(dev, cl, queue);
  HStage("keyless sources");
  TestKeylessSources(dev, cl, queue);
  HStage("alpha map gate");
  TestAlphaMapGate(dev, queue);
  TestPanelCopyScalars();
  HStage("diagnostics round");
  TestDiagnosticsRound();
  HStage("refusal reasons");
  TestRefusalReasons(dev, cl);
  HStage("maps and shared hold");
  TestMapsAndSharedHold(dev, cl, queue);

  CHECK(g_bad_view_calls == 0, "%d view lookups on non-view handles", g_bad_view_calls);
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
