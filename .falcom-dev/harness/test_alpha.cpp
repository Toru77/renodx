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

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Res { resource_desc desc; std::vector<uint8_t> bytes; };
struct Device : mock::DeviceBase {
  std::map<uint64_t, Res> res;
  std::map<uint64_t, uint64_t> view_of;  // view handle -> resource handle
  std::map<uint64_t, resource_view_desc> view_desc;  // view handle -> the desc it was created with
  uint64_t next = 0x1000;
  int bad_copies = 0;
  int creates = 0;  // create_resource calls
  device_api get_api() const override { return device_api::d3d11; }
  bool create_resource(const resource_desc &desc, const subresource_data *initial_data, resource_usage, resource *out, void ** = nullptr) override {
    Res r; r.desc = desc;
    if (desc.type == resource_type::buffer) {
      r.bytes.assign(desc.buffer.size, 0);
      if (initial_data && initial_data->data) std::memcpy(r.bytes.data(), initial_data->data, desc.buffer.size);
    }
    creates += 1;
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
    return resource{it == view_of.end() ? 0u : it->second};
  }
  resource_view_desc get_resource_view_desc(resource_view v) const override {
    const auto it = view_desc.find(v.handle);
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

// Alpha material with the given texture, threshold and first scroll; swizzle as given.
static pool::PoolAlphaMaterial Mat(uint64_t texture, float threshold, float scroll0 = 0.1f, uint32_t swizzle = 3u) {
  pool::PoolAlphaMaterial m;
  m.texture = texture; m.threshold = threshold; m.scroll[0] = scroll0; m.scroll[1] = 0.2f; m.swizzle = swizzle;
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

static void TestClassifier(Device* dev) {
  struct Case { const char* file; uint64_t handle; bool trait; };
  const std::array<Case, 6> alpha = {{
      {"0x137F316A", 0xC101, true}, {"0x81F5709F", 0xC102, true}, {"0xAA835FE0", 0xC103, true},
      {"0x049B0385", 0xC201, false}, {"0x2807FFC9", 0xC202, false}, {"0x2DADE2B8", 0xC203, false}}};
  for (const Case& c : alpha) {
    Classify(dev, c.handle, std::string(c.file) + ".ps.cso", false);
    const contract::ShaderTraits traits = contract::LookupPixelTraits(c.handle);
    CHECK(traits.cls == static_cast<uint8_t>(contract::PsClass::AlphaTested), "%s is alpha tested", c.file);
    CHECK(((traits.flags & contract::kTraitAlphaMaterial) != 0u) == c.trait, "%s kTraitAlphaMaterial %d", c.file, c.trait);
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
  CHECK(!keys.at(0x9006).conflict && keys.at(0x9006).material.texture == 0x100u, "unreadable then readable upgrades");
  pool::NotePoolAlphaMaterial(0x9007, pool::PoolAlphaMaterial{});
  pool::NotePoolAlphaMaterial(0x9007, Mat(0x101, 0.9f));
  CHECK(!keys.at(0x9007).conflict && keys.at(0x9007).material.texture == 0x101u, "unreadable never conflicts");
  CHECK(pool::g_pool.stats.alpha_conflicts == 4u, "four conflicting keys: %llu", (unsigned long long)pool::g_pool.stats.alpha_conflicts);
  pool::g_pool.alpha_foliage.store(false);
  Capture(0x9008, 16.f, true);
  pool::NotePoolAlphaMaterial(0x9008, first);  // recorded while ON, resolved while OFF
  CHECK(keys.count(0x9008) != 0u && pool::g_pool.meshes[MeshOf(0x9008)].alpha, "a Note from an ON record still flags while OFF");
  pool::g_pool.alpha_foliage.store(true);
}

static void TestReadDraw(Device& dev, CmdList& cl) {
  std::array<uint8_t, 160> bytes = {};
  const float threshold = 0.5f, scroll[2] = {0.25f, 0.75f};
  std::memcpy(bytes.data() + contract::kAlphaThresholdOffset, &threshold, sizeof(float));
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
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = 0x500u;

  pool::PoolAlphaMaterial out;
  CHECK(pool::ReadPoolAlphaDraw(&cl, cl_data, &out), "full 160-byte material reads");
  CHECK(out.threshold == threshold && out.scroll[0] == 0.25f && out.scroll[1] == 0.75f, "threshold and scroll decoded");
  CHECK(out.swizzle == 3u && out.texture == 0x500u, "swizzle and texture decoded");

  cl_data.ps_cb[contract::kAlphaMaterialSlot] = partial;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, &out) && out.texture == 0u, "partial cb mirror fails closed");
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  cl_data.ps_cb[contract::kAlphaMaterialSlot].handle = 0u;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, &out) && out.texture == 0u, "unbound b5 fails closed");
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  cl_data.ps_cb_offset[contract::kAlphaMaterialSlot] = 16u;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, &out) && out.texture == 0u, "nonzero b5 offset fails closed");
  cl_data.ps_cb_offset[contract::kAlphaMaterialSlot] = 0u;
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = 0u;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, &out) && out.texture == 0u, "null t0 fails closed");
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = 0x500u;
  cl_data.ps_cb_offset[contract::kAlphaSwizzleSlot] = 4u;
  CHECK(!pool::ReadPoolAlphaDraw(&cl, cl_data, &out) && out.texture == 0u, "bad swizzle offset fails closed");
  cl_data.ps_cb_offset[contract::kAlphaSwizzleSlot] = 0u;
  {
    std::lock_guard<std::mutex> lock(pool::g_pool.mutex);
    CHECK(pool::g_pool.stats.alpha_cb_unavailable == 5u, "five unreadable draws counted: %llu", (unsigned long long)pool::g_pool.stats.alpha_cb_unavailable);
  }
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
  std::memcpy(material_bytes.data() + contract::kAlphaThresholdOffset, &threshold, sizeof(float));
  material = TrackedCb(dev, material_bytes.data(), 160);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;
  cl_data.ps_cb[contract::kAlphaMaterialSlot] = material;
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = 0x500u;

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
  std::memcpy(material_bytes.data() + contract::kAlphaThresholdOffset, &threshold, sizeof(float));
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
  cl_data.ps_srv[contract::kAlphaTexSlot].handle = 0x500u;

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
    CHECK(!copies.empty() && (copies.back().pass & pool::kPoolPassAlpha) != 0u && copies.back().alpha.texture == 0x500u && copies.back().alpha.threshold == threshold,
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
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7001, view1, true);
  CHECK(sources() == 1u && cl.copies == 1 && p.stats.alpha_source_copies == 1u, "copy made at draw time (%zu, %d)", sources(), cl.copies);
  const uint64_t proxy1 = p.alpha_sources.at(0x7001).proxy.handle;
  CHECK(data->alpha.atlas.handle == 0u, "no atlas before the first blit");
  present();
  CHECK(data->alpha.atlas.handle != 0u && cl.dispatches == 1, "atlas made and one blit at the present (%d)", cl.dispatches);
  CHECK(data->alpha.slot_of_uid.count(uid_of(0x7001)) == 1u && data->alpha.slices.used == 1u, "the mesh has its slice");
  CHECK(sources() == 1u && p.alpha_sources.at(0x7001).blitted && dev.res.count(proxy1) == 1u,
        "copy kept by the blit (blitted, proxy alive): %zu sources", sources());
  CHECK(p.alpha_source_done.count(0x7001) == 0u, "a blitted key is not refused");

  // 2. The source texture dies before the blit: no blit, the copy is freed at the present.
  resource tex2 = {0};
  const uint64_t view2 = SourceView(dev, &tex2);
  Capture(0x7002, 8.f, true);
  pool::NotePoolAlphaMaterial(0x7002, Mat(view2, 0.5f));
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7002, view2, true);
  const uint64_t proxy2 = p.alpha_sources.at(0x7002).proxy.handle;
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
  CHECK(p.alpha_sources.count(0x7610) == 1u && p.alpha_sources.at(0x7610).format == format::r8g8b8a8_unorm,
        "view format: the proxy has the bound view's format");
  resource_view unknown = {0};
  dev.create_resource_view(typeless, resource_usage::shader_resource, resource_view_desc(format::unknown), &unknown);
  falcom_world::g_state.frame.fetch_add(1u);
  pool::CapturePoolAlphaSource(&dev, &cl, 0x7611, unknown.handle, true);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(p.alpha_sources.count(0x7611) == 0u && p.alpha_source_done.count(0x7611) == 1u && p.stats.alpha_source_refused_format == 1u,
          "unknown view format: refused and counted (%llu)", (unsigned long long)p.stats.alpha_source_refused_format);
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
  std::memcpy(material_bytes.data() + contract::kAlphaThresholdOffset, &threshold, sizeof(float));
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
    cl_data.ps_srv[contract::kAlphaTexSlot].handle = view;
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
  const uint64_t live_proxy = p.alpha_sources.at(0x7700).proxy.handle;
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
  HStage("switch");
  TestSwitch();
  HStage("decode uv");
  TestDecodeUv();
  HStage("gate");
  TestGate(dev);
  HStage("admission");
  TestAdmit();
  HStage("mesh conflict");
  TestMeshConflict();
  HStage("requeue");
  TestRequeue();
  HStage("reset");
  TestResetClears();
  HStage("scan path");
  TestScanPath(dev, cl, queue);
  HStage("indirect alpha");
  TestIndirectAlpha(dev, cl, queue);
  HStage("atlas (GPU stage, mock)");
  TestAtlas(dev, cl, queue);
  HStage("source guards (mock)");
  TestSourceGuards(dev, cl, queue);
  HStage("end to end (mock)");
  TestEndToEnd(dev, cl, queue);

  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
