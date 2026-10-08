// Native harness, Stage B (CPU side): the alpha atlas slice table and texel packing, and the pool's
// alpha rules: classification of the real alpha pixel shaders, the material read (fails closed),
// per-key and per-mesh conflicts, clearing, the admission refusals, and the default path (toggle
// OFF) through OnPoolScanDraw. The GPU atlas, the blit and the trace are not exercised here.
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "mesh_fixture.hpp"
#include "src/games/falcomengine-plus/world/bvh/alpha_atlas.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_pool.hpp"

using namespace reshade::api;
namespace bvh = falcom_world;
namespace pool = falcom_world::bvh;
namespace contract = falcom_world::contract;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Res { resource_desc desc; std::vector<uint8_t> bytes; };
struct Device : mock::DeviceBase {
  std::map<uint64_t, Res> res;
  uint64_t next = 0x1000;
  int bad_copies = 0;
  device_api get_api() const override { return device_api::d3d11; }
  bool create_resource(const resource_desc &desc, const subresource_data *initial_data, resource_usage, resource *out, void ** = nullptr) override {
    Res r; r.desc = desc; r.bytes.assign(desc.buffer.size, 0);
    if (initial_data && initial_data->data) std::memcpy(r.bytes.data(), initial_data->data, desc.buffer.size);
    out->handle = next; next += 0x100; res[out->handle] = std::move(r);
    return true;
  }
  void destroy_resource(resource r) override { res.erase(r.handle); }
  resource_desc get_resource_desc(resource r) const override { auto it = res.find(r.handle); return it == res.end() ? resource_desc{} : it->second.desc; }
  bool map_buffer_region(resource r, uint64_t offset, uint64_t size, map_access, void **out) override {
    auto it = res.find(r.handle); if (it == res.end() || offset + size > it->second.bytes.size()) return false;
    *out = it->second.bytes.data() + offset; return true;
  }
  void unmap_buffer_region(resource) override {}
};
struct CmdList : mock::CommandListBase {
  Device* dev = nullptr;
  device *get_device() override { return dev; }
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
  CHECK(!admit(0xA1) && stats.alpha_not_ready == 1u, "ON, ready mesh: not_ready");
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
  pool::NotePoolAlphaMaterial(0xC1, material);
  CHECK(!admit(0xC1) && stats.alpha_no_uv == 1u, "no UVs: no_uv");

  Capture(0xD1, 10.f, false);
  CHECK(admit(0xD1) && pool::g_pool.instances.size() == 1u, "non-alpha mesh admitted");
  CHECK(stats.alpha_refused_off == 1u && stats.alpha_not_ready == 1u && stats.alpha_conflict_refused == 1u && stats.alpha_no_uv == 1u,
        "one count per refusal reason");

  // Admission order: refused_off, then conflict_refused, then no_uv, then not_ready.
  Capture(0xE1, 16.f, false);
  Capture(0xE2, 16.f, false);
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
    CHECK(alpha && instances == 0u, "ON: mesh flagged, instances removed (alpha %d, instances %zu)", alpha, instances);
    // The mesh was captured while OFF (no alpha key then, so no UVs): it stays refused as no_uv.
    CHECK(p.stats.alpha_no_uv > 0u && p.stats.alpha_not_ready == 0u, "ON after an OFF capture: no_uv (%llu), not_ready (%llu)",
          (unsigned long long)p.stats.alpha_no_uv, (unsigned long long)p.stats.alpha_not_ready);
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
  // and the refusal is alpha_not_ready.
  pool::ResetWorldPool();
  p.scan_active.store(true);
  p.alpha_foliage.store(true);
  frames(40);
  mesh_state(alpha, instances);
  {
    std::lock_guard<std::mutex> lock(p.mutex);
    CHECK(alpha && instances == 0u, "ON from the start: mesh flagged, no instance");
    CHECK(p.stats.alpha_not_ready > 0u && p.stats.alpha_no_uv == 0u, "ON from the start: not_ready (%llu), no_uv (%llu)",
          (unsigned long long)p.stats.alpha_not_ready, (unsigned long long)p.stats.alpha_no_uv);
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
    CHECK(p.stats.alpha_not_ready > 0u && p.stats.alpha_no_uv == 0u && p.instances.empty(), "indirect ON: refused as not_ready (%llu), not admitted",
          (unsigned long long)p.stats.alpha_not_ready);
  }

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
    CHECK(p.stats.alpha_draws == 0u && p.stats.alpha_conflicts == 0u && p.stats.alpha_refused_off == 0u && p.stats.alpha_not_ready == 0u,
          "indirect OFF: no alpha stats");
    CHECK(p.stats.skips[static_cast<size_t>(pool::PoolSkip::AlphaTested)] > 0u, "indirect OFF: skipped as alpha_tested");
  }
  pool::ResetWorldPool();
  p.alpha_foliage.store(true);
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
  HStage("reset");
  TestResetClears();
  HStage("scan path");
  TestScanPath(dev, cl, queue);
  HStage("indirect alpha");
  TestIndirectAlpha(dev, cl, queue);

  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures != 0;
}
