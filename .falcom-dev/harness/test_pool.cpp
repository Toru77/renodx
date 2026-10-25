// Native harness for the pool: mock device/command list execute copies
// immediately (as the GPU would at that point of the stream).
#include <cassert>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "mesh_fixture.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_pool.hpp"
#include "src/games/falcomengine-plus/world/bvh/placement_probe.hpp"

using namespace reshade::api;
namespace bvh = falcom_world::bvh;
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

struct Object {
  const char* name; uint64_t vb; uint32_t first_index; uint64_t vs; uint64_t ps;
  std::vector<std::array<float, 3>> translations; bool moving = false; bool blend = false;
  uint32_t first_instance = 0;
};

static void World(const float t[3], float out[12]) {
  // float4x3 column-major storage: world.x = dot(p4, out[0..3])
  const float m[12] = {1, 0, 0, t[0], 0, 1, 0, t[1], 0, 0, 1, t[2]};
  std::memcpy(out, m, sizeof(m));
}

// State golden of the pool (CompactPool, PrunePool): instances in order, admitted keys, observations, meshes, maps, counters.
static void Golden(const char* label) {
  std::lock_guard lock(bvh::g_pool.mutex);
  const auto& p = bvh::g_pool;
  uint64_t h = 1469598103934665603ull;
  const auto mix = [&h](uint64_t v) { h = (h ^ v) * 1099511628211ull; };
  for (const auto& inst : p.instances) { mix(inst.id); mix(inst.mesh_id); }
  std::vector<uint64_t> keys(p.admitted_keys.begin(), p.admitted_keys.end());
  std::sort(keys.begin(), keys.end());
  for (uint64_t k : keys) mix(k);
  std::map<std::pair<uint64_t, uint64_t>, uint32_t> observed;
  for (const auto& [key, o] : p.observations) observed[{key.mesh_key, key.matrix_hash}] = (o.admitted ? 1u : 0u) | (o.merged_mesh_key != 0u ? 2u : 0u);
  for (const auto& [key, flags] : observed) { mix(key.first); mix(key.second); mix(flags); }
  for (const auto& mesh : p.meshes) { mix(mesh.uid); mix(mesh.mesh_id); mix(mesh.live_keys); }
  const std::map<uint64_t, uint32_t> by_key(p.mesh_by_key.begin(), p.mesh_by_key.end());
  const std::map<uint64_t, uint32_t> by_signature(p.mesh_by_signature.begin(), p.mesh_by_signature.end());
  for (const auto& [key, id] : by_key) { mix(key); mix(id); }
  for (const auto& [sig, id] : by_signature) { mix(sig); mix(id); }
  std::vector<uint64_t> dynamic;
  for (const auto& [key, entry] : p.dynamic_keys) dynamic.push_back(key);
  std::sort(dynamic.begin(), dynamic.end());
  for (uint64_t k : dynamic) mix(k);
  mix(p.stats.meshes_retired);
  mix(p.stats.instances_retired);
  mix(p.stats.mass_retirements);
  mix(p.map_change_frame);
  mix(p.episode_serial);
  std::printf("GOLDEN %s %016llx instances %zu meshes %zu\n", label, (unsigned long long)h, p.instances.size(), p.meshes.size());
}

// PrunePool at a fixed frame on seeded state: stale and fresh observations, schedules and mesh requests.
static void PruneGolden() {
  const uint32_t frame = 1000u;
  std::lock_guard lock(bvh::g_pool.mutex);
  auto& p = bvh::g_pool;
  p.observations.clear();
  p.schedule.clear();
  p.dynamic_indirect.clear();
  p.mesh_requests.clear();
  p.mesh_queued.clear();
  const uint32_t stale = frame - bvh::kPoolPruneAge - 1u;
  const uint32_t fresh = frame - bvh::kPoolPruneAge + 1u;
  struct Seed { uint64_t mesh_key; uint64_t matrix_hash; bool admitted; uint32_t last; };
  for (const Seed& s : {Seed{1u, 1u, false, stale}, Seed{2u, 1u, false, fresh}, Seed{3u, 1u, true, stale}}) {
    bvh::ObservedInstance& o = p.observations[bvh::InstanceKey{s.mesh_key, s.matrix_hash}];
    o.admitted = s.admitted;
    o.last_frame = s.last;
  }
  p.schedule[10u].next_frame = stale;
  p.schedule[11u].next_frame = fresh;
  p.dynamic_indirect[10u] = 77u;
  p.mesh_requests[20u].last_frame = stale;
  p.mesh_requests[20u].buffer_key = 5u;
  p.mesh_requests[21u].last_frame = fresh;
  p.mesh_queued.insert(20u);
  p.mesh_queued.insert(21u);
  bvh::PrunePool(frame);
  uint64_t h = 1469598103934665603ull;
  uint64_t acc = 0u;
  for (const auto& [key, o] : p.observations) acc += (key.mesh_key * 1099511628211ull) ^ (key.matrix_hash + (o.admitted ? 1u : 0u));
  for (const auto& [key, s] : p.schedule) acc += key * 31u + s.next_frame;
  for (const auto& [key, v] : p.dynamic_indirect) acc += key * 37u + v;
  for (const auto& [key, r] : p.mesh_requests) acc += key * 41u + r.last_frame;
  for (uint64_t k : p.mesh_queued) acc += k * 43u;
  h = (h ^ acc) * 1099511628211ull;
  h = (h ^ p.stats.mesh_expired) * 1099511628211ull;
  std::printf("GOLDEN prune %016llx observations %zu schedule %zu dynamic_indirect %zu mesh_requests %zu mesh_queued %zu\n", (unsigned long long)h, p.observations.size(), p.schedule.size(), p.dynamic_indirect.size(), p.mesh_requests.size(), p.mesh_queued.size());
}

int main() {
  falcom_world::g_state.pool_capture_requested.store(true);
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  fixture::RegisterLayout();

  HStage("shaders and buffers");
  // Shaders (real game bytecode).
  const uint64_t kRigid = 0xA1, kRigidShadow = 0xA3, kSkinned = 0xA2, kOpaque = 0xB1, kAlpha = 0xB2;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kRigidShadow, "0x91CE438F.vs.cso", true);
  Classify(&dev, kSkinned, "0xCF6072C4.vs.cso", true);
  Classify(&dev, kOpaque, "0x2162672F.ps.cso", false);
  Classify(&dev, kAlpha, "0x049B0385.ps.cso", false);
  CHECK(contract::LookupVertexClass(kRigid) == contract::VsClass::Rigid, "rigid");
  CHECK(contract::LookupVertexClass(kRigidShadow) == contract::VsClass::Rigid, "rigid shadow");
  CHECK(contract::LookupVertexClass(kSkinned) == contract::VsClass::Skinned, "skinned");
  CHECK(contract::LookupPixelClass(kOpaque) == contract::PsClass::Opaque, "opaque");
  CHECK(contract::LookupPixelClass(kAlpha) == contract::PsClass::AlphaTested, "alpha");
  CHECK(contract::LookupPixelClass(0) == contract::PsClass::Opaque, "no ps");
  {  // input layouts never enter the registry
    auto code = ReadFile(std::string(FALCOM_BYTECODE_DIR) + "0x095017A3.vs.cso");
    shader_desc desc = {}; desc.code = code.data(); desc.code_size = code.size();
    pipeline_subobject subs[2] = {{pipeline_subobject_type::input_layout, 0, nullptr}, {pipeline_subobject_type::vertex_shader, 1, &desc}};
    contract::OnInitPipelineClassify(&dev, {0}, 2, subs, pipeline{0xC1});
    CHECK(contract::LookupVertexClass(0xC1) == contract::VsClass::Unclassified, "input layout skipped");
  }

  // Buffers.
  resource t15, b1, vb1, vb2, vb3, ib;
  const uint64_t ring_elements = 150000;
  dev.create_resource(resource_desc(ring_elements * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  for (resource* r : {&vb1, &vb2, &vb3, &ib}) dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, r);
  fixture::WriteVertices(dev.res[vb1.handle].bytes, fixture::Box(1.f));
  fixture::WriteVertices(dev.res[vb2.handle].bytes, fixture::Box(1.f));  // same content as vb1
  fixture::WriteVertices(dev.res[vb3.handle].bytes, fixture::Box(3.f));
  fixture::WriteIndices(dev.res[ib.handle].bytes);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;

  std::vector<Object> objects = {
      {"A", vb1.handle, 0, kRigid, kOpaque, {{10, 0, 0}}},
      {"B", vb1.handle, 0, kRigid, kOpaque, {{20, 0, 0}}},
      {"C", vb2.handle, 0, kRigid, kOpaque, {{30, 0, 0}}},
      {"D", vb3.handle, 0, kRigid, kOpaque, {{40, 0, 0}, {41, 0, 0}, {42, 0, 0}}},
      {"E alpha", vb3.handle, 36, kRigid, kAlpha, {{50, 0, 0}}},
      {"F skinned", vb1.handle, 36, kSkinned, kOpaque, {{60, 0, 0}}},
      {"G moving", vb1.handle, 72, kRigid, kOpaque, {{70, 0, 0}}, true},
      {"H blend", vb1.handle, 108, kRigid, kOpaque, {{80, 0, 0}}, false, true},
      {"K start5", vb3.handle, 144, kRigid, kOpaque, {{100, 0, 0}, {101, 0, 0}}, false, false, 5},
      // shadow pass: same objects, other rigid VS, no PS
      {"A shadow", vb1.handle, 0, kRigidShadow, 0, {{10, 0, 0}}},
      {"D shadow", vb3.handle, 0, kRigidShadow, 0, {{40, 0, 0}, {41, 0, 0}, {42, 0, 0}}},
  };

  uint64_t cursor = 12345;  // ring write cursor, keeps moving like the game's
  auto run_frame = [&](int frame_index, bool corrupt_one) {
    for (auto& object : objects) {
      const uint32_t count = static_cast<uint32_t>(object.translations.size());
      if (cursor + count >= ring_elements) cursor = 0;
      const int32_t base = static_cast<int32_t>(cursor);
      for (uint32_t i = 0; i < count; ++i) {
        float t[3] = {object.translations[i][0], object.translations[i][1], object.translations[i][2]};
        float prev[3] = {t[0], t[1], t[2]};
        if (object.moving) { t[1] = frame_index * 0.5f; prev[1] = (frame_index - 1) * 0.5f; }
        float world[12], prev_world[12];
        World(t, world); World(prev, prev_world);
        uint8_t* element = dev.res[t15.handle].bytes.data() + (cursor + i) * 160;
        std::memcpy(element, world, 48); std::memcpy(element + 48, prev_world, 48);
      }
      cursor += count + 7;
      int32_t cb[4] = {base, 0, 0, 0};
      std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
      const bool corrupt = corrupt_one && std::string(object.name) == "B";
      if (!corrupt) falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);

      falcom_world::DrawRecord draw;
      draw.method = 1; draw.has_index_buffer = true; draw.vb = {object.vb}; draw.vb_stride = fixture::kStride; draw.ib = ib;
      draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {fixture::kLayout};
      draw.index_size = 2; draw.dsv = {0x77}; draw.index_count = 36; draw.first_index = object.first_index;
      draw.instance_count = count; draw.first_instance = object.first_instance; draw.depth_enable = true; draw.depth_write = true; draw.blend_enable = object.blend;
      draw.topology = primitive_topology::triangle_list; draw.vs_pipeline = object.vs; draw.ps_pipeline = object.ps;
      draw.vs_hash = static_cast<uint32_t>(object.vs);
      bvh::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
    }
    falcom_world::g_state.frame.fetch_add(1u);
    bvh::DrainPoolScan(&dev, &queue);
  };

  bvh::g_pool.scan_active.store(true);
  HStage("admission");
  // Admission mechanics as before P2a: "G moving" shares its mesh content
  // with A/B/C, which the mesh-level P2a flag would take out (test_motion).
  bvh::g_pool.exclude_moving.store(false);
  bvh::g_pool.follow_moving.store(false);
  bvh::g_pool.alpha_foliage.store(false);  // this check predates alpha-tested foliage (on by default)
  int frame = 1;
  for (; frame <= 40; ++frame) run_frame(frame, false);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("after 40 frames: meshes %zu instances %zu copied %llu verified %llu mismatch %llu mesh_dedup %u dedup_inst %u moving %llu\n",
                p.meshes.size(), p.instances.size(), (unsigned long long)p.stats.copied_draws, (unsigned long long)p.stats.base_verified,
                (unsigned long long)p.stats.base_mismatch, p.stats.mesh_dedup, p.stats.dedup_instances, (unsigned long long)p.stats.moving_instances);
    for (size_t i = 1; i < p.stats.skips.size(); ++i) if (p.stats.skips[i]) std::printf("  skip %s %llu\n", bvh::PoolSkipName((bvh::PoolSkip)i), (unsigned long long)p.stats.skips[i]);
    CHECK(dev.bad_copies == 0, "bad copies %d", dev.bad_copies);
    CHECK(p.meshes.size() == 2, "meshes %zu", p.meshes.size());
    CHECK(p.instances.size() == 8, "instances %zu", p.instances.size());
    CHECK(p.stats.base_mismatch == 0, "mismatch");
    CHECK(p.stats.mesh_dedup == 3, "mesh dedup %u (vb2, G and K share content)", p.stats.mesh_dedup);
    CHECK(p.stats.skips[(size_t)bvh::PoolSkip::AlphaTested] > 0, "alpha skipped");
    CHECK(p.stats.skips[(size_t)bvh::PoolSkip::DrawState] > 0, "blend skipped");
    CHECK(p.stats.draws_by_vs_class[(size_t)contract::VsClass::Skinned] > 0, "skinned seen");
    CHECK(p.stats.moving_instances > 0, "moving seen");
    std::map<float, uint32_t> by_x;
    for (auto& inst : p.instances) {
      by_x[inst.matrix[3]] = inst.mesh_id;
      CHECK(inst.matrix[0] == 1.f && inst.matrix[5] == 1.f && inst.matrix[10] == 1.f && inst.matrix[7] == 0.f, "matrix");
      const auto& mesh = p.meshes[inst.mesh_id];
      CHECK(inst.bounds_min[0] == inst.matrix[3] + mesh.bbox_min[0], "bounds");
    }
    for (float x : {10.f, 20.f, 30.f, 40.f, 41.f, 42.f, 100.f, 101.f}) CHECK(by_x.count(x) == 1, "missing x=%f", x);
    CHECK(by_x[10.f] == by_x[30.f], "dedup mesh id");
    CHECK(by_x[10.f] != by_x[40.f], "different mesh");
  }

  // A stale CPU mirror is caught by the GPU b1 copy. Force B's identity due.
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.schedule.clear();
  }
  run_frame(frame++, true);
  for (int i = 0; i < 3; ++i) run_frame(frame++, false);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    std::printf("mismatch test: mismatch %llu (cpu %d gpu %d)\n", (unsigned long long)bvh::g_pool.stats.base_mismatch,
                bvh::g_pool.stats.last_mismatch_cpu, bvh::g_pool.stats.last_mismatch_gpu);
    CHECK(bvh::g_pool.stats.base_mismatch == 1, "one mismatch");
    CHECK(bvh::g_pool.instances.size() == 8, "instances unchanged %zu", bvh::g_pool.instances.size());
  }

  Golden("admission");
  HStage("release and retire");
  // Releasing vb3 retires D's mesh and its three instances; vb1/vb2 remain.
  bvh::OnDestroyResourcePool(&dev, vb3);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.vb == vb3.handle; }), objects.end());
  dev.destroy_resource(vb3);
  run_frame(frame++, false);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("after vb3 release: meshes %zu instances %zu retired meshes %u instances %u\n", p.meshes.size(), p.instances.size(), p.stats.meshes_retired, p.stats.instances_retired);
    CHECK(p.meshes.size() == 1, "meshes %zu", p.meshes.size());
    CHECK(p.instances.size() == 3, "instances %zu", p.instances.size());
    for (auto& inst : p.instances) CHECK(inst.mesh_id < p.meshes.size(), "mesh id range");
  }

  // Releasing vb1 drops A and B (admitted through vb1) but keeps C via vb2.
  bvh::OnDestroyResourcePool(&dev, vb1);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.vb == vb1.handle; }), objects.end());
  dev.destroy_resource(vb1);
  run_frame(frame++, false);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("after vb1 release: meshes %zu instances %zu\n", p.meshes.size(), p.instances.size());
    CHECK(p.meshes.size() == 1, "meshes %zu", p.meshes.size());
    CHECK(p.instances.size() == 1 && p.instances[0].matrix[3] == 30.f, "only C remains (%zu)", p.instances.size());
  }

  Golden("release");
  HStage("reused VB handle");
  // A reused VB handle with new content starts from scratch.
  resource vb4;
  dev.next = vb3.handle;  // force handle reuse
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb4);
  CHECK(vb4.handle == vb3.handle, "handle reuse");
  fixture::WriteVertices(dev.res[vb4.handle].bytes, fixture::Box(5.f));
  objects.push_back({"J reuse", vb4.handle, 0, kRigid, kOpaque, {{90, 0, 0}}});
  for (int i = 0; i < 40; ++i) run_frame(frame++, false);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("after reuse: meshes %zu instances %zu\n", p.meshes.size(), p.instances.size());
    CHECK(p.meshes.size() == 2, "meshes %zu", p.meshes.size());
    CHECK(p.instances.size() == 2, "instances %zu", p.instances.size());
    for (auto& inst : p.instances) {
      if (inst.matrix[3] == 90.f) CHECK(p.meshes[inst.mesh_id].bbox_max[0] == 5.f, "reused handle got new mesh");
    }
  }

  Golden("reuse");
  HStage("no-op compaction");
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.invalidated_keys.insert(0x5E5Eu);  // a draw key with no instance and no mesh
  }
  run_frame(frame++, false);
  Golden("no-op");
  HStage("map change retires stale static instances");
  // "S stale" is admitted, then no longer drawn; "C" and "J" keep being drawn.
  // Draws 30/90 are C/J: they are drawn every frame, so their copies cool down and must be released by the change.
  dev.next = 0x10000000000ull;  // fresh handles: the reuse test above rewinds next onto live pool buffers
  resource vb_s;
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb_s);
  fixture::WriteVertices(dev.res[vb_s.handle].bytes, fixture::Box(7.f));
  objects.push_back({"S stale", vb_s.handle, 0, kRigid, kOpaque, {{300, 0, 0}}});
  for (int i = 0; i < 40; ++i) run_frame(frame++, false);
  resource vb_s2;
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb_s2);
  fixture::WriteVertices(dev.res[vb_s2.handle].bytes, fixture::Box(8.f));
  objects.push_back({"S2 stale", vb_s2.handle, 0, kRigid, kOpaque, {{310, 0, 0}}});
  for (int i = 0; i < 40; ++i) run_frame(frame++, false);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.vb == vb_s.handle || o.vb == vb_s2.handle; }), objects.end());
  for (int i = 0; i < 40; ++i) run_frame(frame++, false);
  // 32 more meshes, then released in one compaction (a map change).
  std::vector<resource> mass(32);
  for (size_t i = 0; i < mass.size(); ++i) {
    dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &mass[i]);
    fixture::WriteVertices(dev.res[mass[i].handle].bytes, fixture::Box(20.f + static_cast<float>(i)));
    objects.push_back({"M map", mass[i].handle, 0, kRigid, kOpaque, {{400.f + static_cast<float>(i), 0, 0}}});
  }
  for (int i = 0; i < 100; ++i) run_frame(frame++, false);
  uint32_t pre_release_next = 0u;
  uint32_t pre_release_serial = 0u;
  size_t schedules_before = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    std::printf("before map change: meshes %zu instances %zu\n", bvh::g_pool.meshes.size(), bvh::g_pool.instances.size());
    CHECK(bvh::g_pool.stats.mass_retirements == 0u, "no map change yet (%u)", bvh::g_pool.stats.mass_retirements);
    for (auto& [key, schedule] : bvh::g_pool.schedule) pre_release_next = (std::max)(pre_release_next, schedule.next_frame);
    schedules_before = bvh::g_pool.schedule.size();
    pre_release_serial = bvh::g_pool.episode_serial;
  }
  // Pad so the sweep (change frame + settle) lands on a frame that is not a prune frame.
  while ((falcom_world::g_state.frame.load() + 1u + bvh::kPoolMapSettleFrames) % 60u == bvh::kPoolPrunePhase) run_frame(frame++, false);
  for (resource& r : mass) bvh::OnDestroyResourcePool(&dev, r);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.name == std::string("M map"); }), objects.end());
  for (resource& r : mass) dev.destroy_resource(r);
  // Race: S2 is drawn in the change frame only, so its last seen frame is the change frame; erased right after.
  objects.push_back({"S2 stale", vb_s2.handle, 0, kRigid, kOpaque, {{310, 0, 0}}});
  run_frame(frame++, false);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.vb == vb_s2.handle; }), objects.end());
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("after map change: mass_retirements %u map_change_frame %u map_instances_retired %u meshes %zu instances %zu\n",
                p.stats.mass_retirements, p.map_change_frame, p.stats.map_instances_retired, p.meshes.size(), p.instances.size());
    CHECK(p.stats.mass_retirements == 1u, "one map change (%u)", p.stats.mass_retirements);
    CHECK(p.episode_serial == pre_release_serial + 1u, "map change bumps the episode serial (%u)", p.episode_serial);
    CHECK(p.episode_fresh_from == p.map_change_frame + 1u, "map change: fresh from the frame after the change (%u)", p.episode_fresh_from);
    CHECK(std::strcmp(p.episode_reason, "map change") == 0, "map change reason (%s)", p.episode_reason);
    CHECK(p.map_change_frame == falcom_world::g_state.frame.load(), "map change frame (%u)", p.map_change_frame);
    CHECK(p.episode_start_frame == p.map_change_frame, "episode starts at the map change (%u)", p.episode_start_frame);
    CHECK(p.stats.map_instances_retired == 0u, "no sweep in change frame (%u)", p.stats.map_instances_retired);
    CHECK(p.instances.size() == 4, "instances %zu", p.instances.size());
    CHECK(pre_release_next > p.map_change_frame, "cooldown before the release (%u > %u)", pre_release_next, p.map_change_frame);
    size_t cooled = 0;
    for (auto& [key, schedule] : p.schedule) cooled += schedule.next_frame > p.map_change_frame + 1u ? 1u : 0u;
    CHECK(cooled == 0, "schedules released by the change (%zu cooled)", cooled);
    CHECK(p.schedule.size() == schedules_before, "schedules kept (%zu of %zu)", p.schedule.size(), schedules_before);
  }
  for (uint32_t i = 1; i < bvh::kPoolMapSettleFrames; ++i) run_frame(frame++, false);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("before settle: map_change_frame %u frame %u map_instances_retired %u instances %zu\n",
                p.map_change_frame, falcom_world::g_state.frame.load(), p.stats.map_instances_retired, p.instances.size());
    CHECK(p.map_change_frame == falcom_world::g_state.frame.load() - bvh::kPoolMapSettleFrames + 1u, "sweep pending (%u)", p.map_change_frame);
    CHECK(p.map_change_frame != 0u && p.episode_start_frame == p.map_change_frame, "map change pending until the sweep (%u, episode %u)", p.map_change_frame, p.episode_start_frame);
    CHECK(p.stats.map_instances_retired == 0u, "no sweep before settle (%u)", p.stats.map_instances_retired);
    CHECK(p.instances.size() == 4, "instances before settle %zu", p.instances.size());
    int live = 0;
    for (auto& inst : p.instances) {
      if (inst.matrix[3] != 30.f && inst.matrix[3] != 90.f) continue;
      live += 1;
      CHECK(bvh::PoolInstanceLastSeen(inst) > p.map_change_frame, "live instance seen after change (%.0f)", inst.matrix[3]);
    }
    CHECK(live == 2, "C and J present before settle (%d)", live);
  }
  run_frame(frame++, false);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("after settle: frame %u map_change_frame %u map_instances_retired %u meshes %zu instances %zu\n",
                falcom_world::g_state.frame.load(), p.map_change_frame, p.stats.map_instances_retired, p.meshes.size(), p.instances.size());
    CHECK(falcom_world::g_state.frame.load() % 60u != bvh::kPoolPrunePhase, "sweep frame is not a prune frame (%u)", falcom_world::g_state.frame.load());
    CHECK(p.map_change_frame == 0u, "sweep done (%u)", p.map_change_frame);
    CHECK(p.stats.map_instances_retired == 2u, "stale and change-frame instances dropped (%u)", p.stats.map_instances_retired);
    CHECK(p.instances.size() == 2, "instances %zu", p.instances.size());
    bool stale_present = false, recent_s2 = false, recent_c = false, recent_j = false;
    for (auto& inst : p.instances) {
      stale_present |= inst.matrix[3] == 300.f;
      recent_s2 |= inst.matrix[3] == 310.f;
      recent_c |= inst.matrix[3] == 30.f;
      recent_j |= inst.matrix[3] == 90.f;
    }
    CHECK(!stale_present, "stale instance gone");
    CHECK(!recent_s2, "S2 (last seen in the change frame) removed");
    CHECK(recent_c && recent_j, "recent instances survive");
    for (auto& [key, observed] : p.observations) {
      if (observed.matrix[3] == 300.f) CHECK(!observed.admitted, "stale observation not admitted");
    }
  }
  objects.push_back({"S stale", vb_s.handle, 0, kRigid, kOpaque, {{300, 0, 0}}});
  for (int i = 0; i < 40; ++i) run_frame(frame++, false);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    bool readmitted = false;
    for (auto& [key, observed] : p.observations) {
      if (observed.matrix[3] == 300.f) readmitted |= observed.admitted;
    }
    std::printf("after re-add: instances %zu readmitted %d\n", p.instances.size(), readmitted ? 1 : 0);
    CHECK(p.instances.size() == 3, "re-admitted instances %zu", p.instances.size());
    CHECK(readmitted, "stale object re-admitted");
  }

  Golden("map change");
  HStage("map change sweep waits while BudgetFull skips change");
  // "S3 stale" is drawn for 40 frames, then no longer drawn; the next map change must drop it.
  resource vb_s3;
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb_s3);
  fixture::WriteVertices(dev.res[vb_s3.handle].bytes, fixture::Box(9.f));
  objects.push_back({"S3 stale", vb_s3.handle, 0, kRigid, kOpaque, {{600, 0, 0}}});
  for (int i = 0; i < 40; ++i) run_frame(frame++, false);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.vb == vb_s3.handle; }), objects.end());
  const auto resight_count = [&]() {
    std::lock_guard lock(bvh::g_pool.mutex);
    return bvh::g_pool.resight.counters.resights;
  };
  const uint64_t resights_before_mass2 = resight_count();
  std::vector<resource> mass2(32);
  for (size_t i = 0; i < mass2.size(); ++i) {
    dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &mass2[i]);
    fixture::WriteVertices(dev.res[mass2[i].handle].bytes, fixture::Box(20.f + static_cast<float>(i)));
    objects.push_back({"M map2", mass2[i].handle, 0, kRigid, kOpaque, {{500.f + static_cast<float>(i), 0, 0}}});
  }
  for (int i = 0; i < 100; ++i) run_frame(frame++, false);
  // Derived: the 32 mass2 meshes are Box(20+i), the same geometry as the 32 meshes of the first map change (retired
  // then, tracked within the horizon). They are the only new meshes in this window, so each one re-sights once.
  CHECK(resight_count() - resights_before_mass2 == 32u, "mass2 re-sights the 32 retired mass meshes (%llu)",
        static_cast<unsigned long long>(resight_count() - resights_before_mass2));
  for (resource& r : mass2) bvh::OnDestroyResourcePool(&dev, r);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.name == std::string("M map2"); }), objects.end());
  for (resource& r : mass2) dev.destroy_resource(r);
  run_frame(frame++, false);
  uint64_t retired_before = 0u;
  uint32_t change_frame = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    CHECK(p.map_change_frame == falcom_world::g_state.frame.load(), "second map change frame (%u)", p.map_change_frame);
    retired_before = p.stats.map_instances_retired;
    change_frame = p.map_change_frame;
  }
  auto add_budget_skip = [&] {
    std::lock_guard lock(bvh::g_pool.mutex);
    ++bvh::g_pool.stats.skips[static_cast<size_t>(bvh::PoolSkip::BudgetFull)];
  };

  // Case A: BudgetFull skips grow every frame: the sweep stays deferred until the forced sweep at change + kPoolRecaptureFrames.
  for (uint32_t i = 1; i < bvh::kPoolRecaptureFrames; ++i) {
    add_budget_skip();
    run_frame(frame++, false);
  }
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    int live = 0;
    for (auto& inst : p.instances) live += (inst.matrix[3] == 30.f || inst.matrix[3] == 90.f) ? 1 : 0;
    std::printf("budget deferred: map_change_frame %u frame %u map_instances_retired %llu live %d\n",
                p.map_change_frame, falcom_world::g_state.frame.load(), static_cast<unsigned long long>(p.stats.map_instances_retired), live);
    CHECK(p.map_change_frame == change_frame, "sweep deferred while skips change (%u)", p.map_change_frame);
    CHECK(p.stats.map_instances_retired == retired_before, "no sweep while deferred (%llu)", static_cast<unsigned long long>(p.stats.map_instances_retired));
    CHECK(live == 2, "C and J remain while deferred (%d)", live);
  }
  add_budget_skip();
  run_frame(frame++, false);  // change + kPoolRecaptureFrames: forced
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("budget forced: frame %u map_instances_retired %llu instances %zu\n",
                falcom_world::g_state.frame.load(), static_cast<unsigned long long>(p.stats.map_instances_retired), p.instances.size());
    CHECK(falcom_world::g_state.frame.load() == change_frame + bvh::kPoolRecaptureFrames, "forced sweep frame (%u)", falcom_world::g_state.frame.load());
    CHECK(p.map_change_frame == 0u, "sweep done at the forced frame (%u)", p.map_change_frame);
    CHECK(p.stats.map_instances_retired == retired_before + 1u, "stale S3 dropped by the forced sweep (%llu)", static_cast<unsigned long long>(p.stats.map_instances_retired));
    CHECK(p.stats.mass_retirements == 2u, "second map change counted (%u)", p.stats.mass_retirements);
  }

  // Case B: one BudgetFull skip at change + settle - 1: the sweep waits kPoolStagingSlots after that skip.
  resource vb_s4;
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb_s4);
  fixture::WriteVertices(dev.res[vb_s4.handle].bytes, fixture::Box(10.f));
  objects.push_back({"S4 stale", vb_s4.handle, 0, kRigid, kOpaque, {{700, 0, 0}}});
  for (int i = 0; i < 40; ++i) run_frame(frame++, false);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.vb == vb_s4.handle; }), objects.end());
  std::vector<resource> mass3(32);
  for (size_t i = 0; i < mass3.size(); ++i) {
    dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &mass3[i]);
    fixture::WriteVertices(dev.res[mass3[i].handle].bytes, fixture::Box(40.f + static_cast<float>(i)));
    objects.push_back({"M map3", mass3[i].handle, 0, kRigid, kOpaque, {{800.f + static_cast<float>(i), 0, 0}}});
  }
  for (int i = 0; i < 100; ++i) run_frame(frame++, false);
  for (resource& r : mass3) bvh::OnDestroyResourcePool(&dev, r);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.name == std::string("M map3"); }), objects.end());
  for (resource& r : mass3) dev.destroy_resource(r);
  run_frame(frame++, false);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    CHECK(p.map_change_frame == falcom_world::g_state.frame.load(), "third map change frame (%u)", p.map_change_frame);
    retired_before = p.stats.map_instances_retired;
    change_frame = p.map_change_frame;
  }
  for (uint32_t i = 1; i < bvh::kPoolMapSettleFrames - 1u; ++i) run_frame(frame++, false);  // change + 1 .. settle - 2
  add_budget_skip();
  run_frame(frame++, false);  // change + settle - 1: the skip
  for (uint32_t i = 0; i < bvh::kPoolStagingSlots; ++i) {  // change + settle .. skip + slots: deferred
    run_frame(frame++, false);
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.map_change_frame == change_frame, "sweep waits kPoolStagingSlots after the skip (%u)", falcom_world::g_state.frame.load());
    CHECK(bvh::g_pool.stats.map_instances_retired == retired_before, "no sweep within the staging window (%llu)", static_cast<unsigned long long>(bvh::g_pool.stats.map_instances_retired));
  }
  run_frame(frame++, false);  // skip + slots + 1: sweep
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("budget settled: frame %u map_change_frame %u map_instances_retired %llu instances %zu\n",
                falcom_world::g_state.frame.load(), p.map_change_frame, static_cast<unsigned long long>(p.stats.map_instances_retired), p.instances.size());
    CHECK(p.map_change_frame == 0u, "sweep done once the skips settled (%u)", p.map_change_frame);
    CHECK(p.stats.map_instances_retired == retired_before + 1u, "stale S4 dropped after the staging window (%llu)", static_cast<unsigned long long>(p.stats.map_instances_retired));
    CHECK(p.stats.mass_retirements == 3u, "third map change counted (%u)", p.stats.mass_retirements);
  }

  {  // CB mirror: device update, deferred update, map/unmap, reset
    CmdList deferred; deferred.dev = &dev;
    int32_t v7[4] = {7}, v9[4] = {9}, v11[4] = {11};
    falcom_world::OnUpdateBufferRegionCommandCbTracker(&deferred, v7, b1, 0, 16);
    falcom_world::OnUpdateBufferRegionCbTracker(&dev, v9, b1, 0, UINT64_MAX);
    int32_t out = 0;
    CHECK(falcom_world::ReadTrackedCbInt(&deferred, b1, &out) && out == 7, "deferred sees own write (%d)", out);
    CHECK(falcom_world::ReadTrackedCbInt(&cl, b1, &out) && out == 9, "immediate sees device write (%d)", out);
    falcom_world::OnResetCommandListCbTracker(&deferred);
    CHECK(falcom_world::ReadTrackedCbInt(&deferred, b1, &out) && out == 9, "after reset (%d)", out);
    uint8_t mapped_bytes[16] = {};
    void* ptr = mapped_bytes;
    falcom_world::OnMapBufferRegionCbTracker(&dev, b1, 0, 16, map_access::write_discard, &ptr);
    std::memcpy(mapped_bytes, v11, 16);
    falcom_world::OnUnmapBufferRegionCbTracker(&dev, b1);
    CHECK(falcom_world::ReadTrackedCbInt(&cl, b1, &out) && out == 11, "map/unmap (%d)", out);
    int32_t partial = 5;
    falcom_world::OnUpdateBufferRegionCbTracker(&dev, &partial, b1, 4, 4);
    CHECK(falcom_world::ReadTrackedCbInt(&cl, b1, &out) && out == 11, "write past offset 0 keeps offset (%d)", out);
  }

  {  // Phase 0 cache diagnostics: episode serial and fields, placement probe (read-only snapshot, compare, files)
    uint32_t serial_before = 0u;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      serial_before = bvh::g_pool.episode_serial;
    }
    bvh::ResetWorldPool();
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      auto& p = bvh::g_pool;
      CHECK(p.episode_serial == serial_before + 1u, "reset bumps the episode serial (%u -> %u)", serial_before, p.episode_serial);
      CHECK(p.episode_fresh_from == 0u, "reset: any instance is fresh (%u)", p.episode_fresh_from);
      CHECK(std::strcmp(p.episode_reason, "pool reset") == 0, "reset reason (%s)", p.episode_reason);
      CHECK(p.episode_start_frame == falcom_world::g_state.frame.load(), "reset start frame (%u)", p.episode_start_frame);
    }
    // Placement fixtures: a camera at (1, 2, 3); static, dynamic, far and alpha instances; 6100 static near ones.
    float view[16] = {1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 2.f, 0.f, 0.f, 1.f, 3.f, 0.f, 0.f, 0.f, 1.f};
    falcom_world::g_state.camera.valid = true;
    std::memcpy(falcom_world::g_state.camera.view_inv, view, sizeof(view));
    constexpr uint32_t kVsStatic = 0xBEEFu;
    constexpr uint32_t kVsDynamic = 0xDEADu;
    uint64_t revision_before = 0u;
    size_t instances_before = 0u;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      auto& p = bvh::g_pool;
      p.meshes.clear();
      p.instances.clear();
      bvh::WorldMesh solid;
      solid.uid = 1u;
      solid.signature = 0x1111u;
      solid.triangle_count = 1u;
      p.meshes.push_back(solid);
      bvh::WorldMesh alpha = solid;
      alpha.uid = 2u;
      alpha.mesh_id = 1u;
      alpha.signature = 0x2222u;
      alpha.alpha = true;
      p.meshes.push_back(alpha);
      uint64_t next_id = 1u;
      const auto add = [&](uint32_t mesh_id, float x, float y, float z, uint32_t vs, bool dynamic) {
        bvh::WorldInstance instance;
        instance.mesh_id = mesh_id;
        instance.source_vs_hash = vs;
        instance.dynamic = dynamic;
        instance.id = next_id++;
        instance.matrix[0] = instance.matrix[5] = instance.matrix[10] = 1.f;
        instance.matrix[3] = x;
        instance.matrix[7] = y;
        instance.matrix[11] = z;
        instance.bounds_min[0] = x - 0.5f;
        instance.bounds_min[1] = y - 0.5f;
        instance.bounds_min[2] = z - 0.5f;
        instance.bounds_max[0] = x + 0.5f;
        instance.bounds_max[1] = y + 0.5f;
        instance.bounds_max[2] = z + 0.5f;
        p.instances.push_back(instance);
      };
      for (uint32_t i = 0u; i < 6100u; ++i) {
        add(0u, static_cast<float>(i % 61u) * 0.5f - 15.f, static_cast<float>((i / 61u) % 61u) * 0.5f - 15.f, 0.f, kVsStatic, false);
      }
      for (uint32_t i = 0u; i < 20u; ++i) add(0u, 3.f + static_cast<float>(i), 0.f, 0.f, kVsDynamic, true);
      add(0u, 200.f, 0.f, 0.f, kVsStatic, false);
      add(1u, 5.f, 5.f, 0.f, kVsStatic, false);
      revision_before = p.revision;
      instances_before = p.instances.size();
    }
    bvh::PlacementSnapshot snapshot;
    bvh::TakePlacementSnapshot(&snapshot);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      CHECK(bvh::g_pool.revision == revision_before, "snapshot changes nothing: revision (%llu -> %llu)",
            static_cast<unsigned long long>(revision_before), static_cast<unsigned long long>(bvh::g_pool.revision));
      CHECK(bvh::g_pool.instances.size() == instances_before, "snapshot changes nothing: instances (%zu -> %zu)", instances_before, bvh::g_pool.instances.size());
    }
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.mesh_requests.clear();
      for (uint64_t key = 1u; key <= 3u; ++key) {
        bvh::PoolMeshRequest request;
        request.mesh_key = key;
        bvh::g_pool.mesh_requests[key] = request;
      }
      bvh::g_pool.mesh_requests[1].in_flight = true;
    }
    bvh::PlacementSnapshot queued;
    bvh::TakePlacementSnapshot(&queued);
    CHECK(queued.mesh_queue == 2u && queued.mesh_in_flight == 1u,
          "snapshot counts the queue with no drain: queued %llu (want 2), in flight %llu (want 1)",
          static_cast<unsigned long long>(queued.mesh_queue), static_cast<unsigned long long>(queued.mesh_in_flight));
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.mesh_requests.clear();
    }
    size_t dynamic_records = 0u;
    size_t far_records = 0u;
    size_t alpha_records = 0u;
    for (const bvh::PlacementRecord& record : snapshot.records) {
      dynamic_records += record.vs_hash == kVsDynamic ? 1u : 0u;
      far_records += record.distance > bvh::kPlacementRadius ? 1u : 0u;
      alpha_records += record.alpha ? 1u : 0u;
    }
    CHECK(dynamic_records == 0u, "placement: dynamic instances left out (%zu)", dynamic_records);
    CHECK(far_records == 0u, "placement: instances beyond %.0f m left out (%zu)", static_cast<double>(bvh::kPlacementRadius), far_records);
    CHECK(alpha_records == 1u, "placement: alpha record kept and flagged (%zu)", alpha_records);
    CHECK(snapshot.records.size() == bvh::kPlacementCap, "placement: cap of %zu records (%zu)", bvh::kPlacementCap, snapshot.records.size());
    CHECK(snapshot.truncated, "placement: truncated flag set");
    CHECK(snapshot.radius == snapshot.records.back().distance, "placement: effective radius is the last kept distance (%.3f)", static_cast<double>(snapshot.radius));
    bool ordered = true;
    for (size_t i = 1u; i < snapshot.records.size(); ++i) ordered &= snapshot.records[i - 1u].distance <= snapshot.records[i].distance;
    CHECK(ordered, "placement: nearest first");
    CHECK(snapshot.frame == falcom_world::g_state.frame.load(), "placement: header frame (%u)", snapshot.frame);

    // Compare: identical, jitter, rebase by offset, outside-radius labels, too few, budget, hash guard.
    const auto rec = [](uint64_t sig, float x, float y, float z, uint64_t hash = 0u) {
      bvh::PlacementRecord r;
      r.signature = sig;
      const float m[12] = {1.f, 0.f, 0.f, x, 0.f, 1.f, 0.f, y, 0.f, 0.f, 1.f, z};
      std::memcpy(r.matrix, m, sizeof(m));
      r.matrix_hash = hash != 0u ? hash : falcom_world::MatrixHash(r.matrix, bvh::kPoolWorldFloats);
      r.distance = (std::max)(std::fabs(x), (std::max)(std::fabs(y), std::fabs(z)));
      const float bounds[6] = {x, y, z, x, y, z};
      std::memcpy(r.bounds, bounds, sizeof(bounds));
      return r;
    };
    const auto make = [](std::vector<bvh::PlacementRecord> records) {
      bvh::PlacementSnapshot s;
      s.records = std::move(records);
      s.build = "test";
      return s;
    };
    std::vector<bvh::PlacementRecord> grid;
    for (uint32_t i = 0u; i < 100u; ++i) grid.push_back(rec(0x42u, static_cast<float>(i % 10u) * 2.f, static_cast<float>(i / 10u) * 2.f, 0.f));
    const bvh::PlacementSnapshot grid_a = make(grid);
    const bvh::PlacementCompare same = bvh::ComparePlacementSnapshots(grid_a, grid_a);
    CHECK(same.verdict == "PASS exact-match 100.0% (100 of 100), only A 0.0%, only B 0.0%", "identical: %s", same.verdict.c_str());
    std::vector<bvh::PlacementRecord> jittered;
    for (const bvh::PlacementRecord& r : grid) jittered.push_back(rec(0x42u, r.matrix[3] + 5e-4f, r.matrix[7], r.matrix[11]));
    const bvh::PlacementCompare jitter = bvh::ComparePlacementSnapshots(grid_a, make(jittered));
    CHECK(jitter.verdict.rfind("WARN not PASS", 0) == 0, "jitter 5e-4: %s", jitter.verdict.c_str());
    std::vector<bvh::PlacementRecord> offset;
    for (const bvh::PlacementRecord& r : grid) offset.push_back(rec(0x42u, r.matrix[3] + 37.5f, r.matrix[7], r.matrix[11] - 12.f));
    const bvh::PlacementCompare rebase = bvh::ComparePlacementSnapshots(grid_a, make(offset));
    CHECK(rebase.verdict == "FAIL rebase-by-delta (37.5, 0, -12) m", "rebase by delta: %s", rebase.verdict.c_str());
    bvh::PlacementSnapshot short_b = make(grid);
    short_b.radius = 50.f;
    std::vector<bvh::PlacementRecord> far_a = grid;
    far_a.push_back(rec(0x42u, 60.f, 0.f, 0.f));
    const bvh::PlacementCompare outside = bvh::ComparePlacementSnapshots(make(far_a), short_b);
    CHECK(outside.lines[1].find("outside other radius, expected: A 1, B 0") != std::string::npos, "outside label: %s", outside.lines[1].c_str());
    // Moved camera with equal overlap: A's extra record is 90 m from A and 100 m from B (beyond 96 m): outside, not a mismatch.
    bvh::PlacementSnapshot moved_a = make(grid);
    moved_a.records.push_back(rec(0x42u, -90.f, 0.f, 0.f));
    bvh::PlacementSnapshot moved_b = make(grid);
    moved_b.camera[0] = 10.f;
    const bvh::PlacementCompare moved = bvh::ComparePlacementSnapshots(moved_a, moved_b);
    CHECK(moved.verdict == "PASS exact-match 100.0% (100 of 100), only A 0.0%, only B 0.0%", "moved camera, equal overlap: %s", moved.verdict.c_str());
    CHECK(moved.lines[0].find("camera A to B 10 m") != std::string::npos, "camera gap in the report: %s", moved.lines[0].c_str());
    // B much smaller than A: 100 of 1000 exact is not PASS (only A 90%).
    std::vector<bvh::PlacementRecord> big;
    for (uint32_t i = 0u; i < 1000u; ++i) big.push_back(rec(0x42u, static_cast<float>(i % 40u) * 2.f, static_cast<float>(i / 40u) * 2.f, 0.f));
    const bvh::PlacementCompare shrunk = bvh::ComparePlacementSnapshots(make(big), make(std::vector<bvh::PlacementRecord>(big.begin(), big.begin() + 100)));
    CHECK(shrunk.verdict.rfind("PASS", 0) != 0 && shrunk.verdict.find("only A 90.0%") != std::string::npos, "B much smaller is not PASS: %s", shrunk.verdict.c_str());
    std::vector<bvh::PlacementRecord> ten(grid.begin(), grid.begin() + 10);
    const bvh::PlacementCompare few = bvh::ComparePlacementSnapshots(make(ten), make(ten));
    CHECK(few.verdict == "INCONCLUSIVE 10 comparable instances (need 20)", "10 comparable: %s", few.verdict.c_str());
    std::vector<bvh::PlacementRecord> wide_a;
    std::vector<bvh::PlacementRecord> wide_b;
    uint32_t seed = 12345u;
    const auto next = [&seed]() {
      seed = seed * 1664525u + 1013904223u;
      return static_cast<float>(seed >> 8) / 16777216.f * 180.f - 90.f;
    };
    for (uint32_t i = 0u; i < 1200u; ++i) {
      wide_a.push_back(rec(0x42u, next(), next(), next()));
      wide_b.push_back(rec(0x42u, next(), next(), next()));
    }
    const bvh::PlacementCompare budget = bvh::ComparePlacementSnapshots(make(wide_a), make(wide_b));
    CHECK(budget.verdict == "INCONCLUSIVE (budget)", "budget: %s", budget.verdict.c_str());
    // Two disjoint scenes (different signatures, no partners): only B above 50%, no exact or near matches.
    std::vector<bvh::PlacementRecord> other_scene;
    for (uint32_t i = 0u; i < 300u; ++i) other_scene.push_back(rec(0x43u, static_cast<float>(i % 40u) * 2.f, static_cast<float>(i / 40u) * 2.f, 0.f));
    const bvh::PlacementCompare disjoint = bvh::ComparePlacementSnapshots(make(grid), make(other_scene));
    CHECK(disjoint.verdict == "INCONCLUSIVE different scene (only A 100.0%, only B 100.0%): not comparable, retake A in the same map and spot",
          "disjoint scenes: %s", disjoint.verdict.c_str());
    std::vector<bvh::PlacementRecord> collide;
    for (const bvh::PlacementRecord& r : grid) collide.push_back(rec(0x42u, r.matrix[3] + 1e-4f, r.matrix[7], r.matrix[11], grid_a.records[0].matrix_hash));
    std::vector<bvh::PlacementRecord> collide_a;
    for (const bvh::PlacementRecord& r : grid) collide_a.push_back(rec(0x42u, r.matrix[3], r.matrix[7], r.matrix[11], grid_a.records[0].matrix_hash));
    const bvh::PlacementCompare guard = bvh::ComparePlacementSnapshots(make(collide_a), make(collide));
    CHECK(guard.lines[2].rfind("Exact 0,", 0) == 0, "hash guard: same hash, other floats is not exact (%s)", guard.lines[2].c_str());

    // File round trip (bit-equal) and a corrupt line named in the error.
    const std::filesystem::path round = std::filesystem::temp_directory_path() / "falcom_placement_roundtrip.txt";
    CHECK(bvh::WritePlacementFile(round, snapshot), "write placement file");
    bvh::PlacementSnapshot back;
    std::string error;
    CHECK(bvh::ReadPlacementFile(round, &back, &error), "read placement file: %s", error.c_str());
    bool bit_equal = back.records.size() == snapshot.records.size() && back.build == snapshot.build && back.frame == snapshot.frame
                     && back.truncated == snapshot.truncated && std::memcmp(back.camera, snapshot.camera, sizeof(back.camera)) == 0
                     && std::memcmp(&back.radius, &snapshot.radius, sizeof(float)) == 0 && back.switches == snapshot.switches;
    for (size_t i = 0u; bit_equal && i < snapshot.records.size(); ++i) {
      const bvh::PlacementRecord& x = snapshot.records[i];
      const bvh::PlacementRecord& y = back.records[i];
      bit_equal = x.signature == y.signature && x.matrix_hash == y.matrix_hash && x.vs_hash == y.vs_hash && x.alpha == y.alpha
                  && std::memcmp(x.matrix, y.matrix, sizeof(x.matrix)) == 0 && std::memcmp(x.bounds, y.bounds, sizeof(x.bounds)) == 0
                  && std::memcmp(&x.distance, &y.distance, sizeof(float)) == 0;
    }
    CHECK(bit_equal, "file round trip is bit-equal (%zu records)", back.records.size());
    const std::filesystem::path again = std::filesystem::temp_directory_path() / "falcom_placement_roundtrip_2.txt";
    bvh::WritePlacementFile(again, back);
    CHECK(ReadFile(round.string()) == ReadFile(again.string()), "rewritten file is byte-equal");
    std::vector<std::string> text_lines;
    {
      std::ifstream in(round, std::ios::binary);
      for (std::string line; std::getline(in, line);) text_lines.push_back(line);
    }
    const size_t bad_line = 12u;
    text_lines[bad_line - 1u] = "rec zz 0 0 0 0 0";
    {
      std::ofstream out(round, std::ios::binary | std::ios::trunc);
      for (const std::string& line : text_lines) out << line << "\n";
    }
    bvh::PlacementSnapshot corrupt;
    CHECK(!bvh::ReadPlacementFile(round, &corrupt, &error), "corrupt line refused");
    CHECK(error == "line 12: bad record head", "corrupt line named (%s)", error.c_str());
    std::printf("cache diag placement: %zu records (truncated %d); identical: %s | jitter: %s | rebase: %s | budget: %s\n", snapshot.records.size(),
                snapshot.truncated ? 1 : 0, same.verdict.c_str(), jitter.verdict.c_str(), rebase.verdict.c_str(), budget.verdict.c_str());
    std::filesystem::remove(round);
    std::filesystem::remove(again);
  }

  // Single object cycle: admit, destroy, recreate identical content -> one re-sighting.
  const auto counters = []() {
    std::lock_guard lock(bvh::g_pool.mutex);
    return bvh::g_pool.resight.counters;
  };
  const auto make = [&](float size) {
    resource vb;
    dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb);
    fixture::WriteVertices(dev.res[vb.handle].bytes, fixture::Box(size));
    objects.push_back({"R cycle", vb.handle, 0, kRigid, kOpaque, {{950, 0, 0}}});
    for (int i = 0; i < 40; ++i) run_frame(frame++, false);
    return vb;
  };
  const auto drop = [&](resource& vb) {
    bvh::OnDestroyResourcePool(&dev, vb);
    objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.vb == vb.handle; }), objects.end());
    dev.destroy_resource(vb);
    run_frame(frame++, false);
  };
  resource cycle = make(30.f);
  drop(cycle);
  const bvh::PoolResightCounters before_recreate = counters();
  cycle = make(30.f);
  CHECK(counters().resights - before_recreate.resights == 1u, "single object recreated with identical content re-sights once (%llu)",
        static_cast<unsigned long long>(counters().resights - before_recreate.resights));
  drop(cycle);
  bvh::g_pool.resight.enabled.store(false);
  const bvh::PoolResightCounters off_before = counters();
  cycle = make(31.f);
  drop(cycle);
  const bvh::PoolResightCounters off_after = counters();
  bvh::g_pool.resight.enabled.store(true);
  CHECK(off_after.retired == off_before.retired && off_after.new_meshes == off_before.new_meshes && off_after.resights == off_before.resights,
        "tracker disabled: counters unchanged (retired %llu, new %llu, resights %llu)", static_cast<unsigned long long>(off_after.retired),
        static_cast<unsigned long long>(off_after.new_meshes), static_cast<unsigned long long>(off_after.resights));

  {
    std::lock_guard lock(bvh::g_pool.mutex);
    const bvh::PoolResightCounters& r = bvh::g_pool.resight.counters;
    uint64_t gap_sum = 0u;
    for (const uint64_t count : r.gap) gap_sum += count;
    std::printf("resight: retired %llu, new meshes %llu, resights %llu, count mismatch %llu, drift %llu, gap sum %llu\n",
                static_cast<unsigned long long>(r.retired), static_cast<unsigned long long>(r.new_meshes), static_cast<unsigned long long>(r.resights),
                static_cast<unsigned long long>(r.count_mismatch), static_cast<unsigned long long>(r.drift_near_misses),
                static_cast<unsigned long long>(gap_sum));
    CHECK(r.retired >= 32u && r.count_mismatch == 0u && r.drift_near_misses == 0u && gap_sum == r.resights,
          "resight counters after the mass retirements (retired %llu, resights %llu)", static_cast<unsigned long long>(r.retired),
          static_cast<unsigned long long>(r.resights));
  }

  bvh::DumpWorldPool();
  {
    size_t dynamic = 0u;
    {
      std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
      for (const auto& instance : bvh::g_pool.instances) dynamic += instance.dynamic ? 1u : 0u;
    }
    const std::vector<uint8_t> bytes = ReadFile((bvh::PoolOutputDir() / "world_pool.json").string());
    const std::string text(bytes.begin(), bytes.end());
    CHECK(text.find("\"following_instances\": " + std::to_string(dynamic) + ",") != std::string::npos, "world_pool.json following_instances equals dynamic instances (%zu)",
          dynamic);
  }
  {
    const std::vector<uint8_t> bytes = ReadFile((bvh::PoolOutputDir() / "world_pool.json").string());
    const std::string text(bytes.begin(), bytes.end());
    const size_t list = text.find("\"dynamic_candidates\": [");
    const size_t list_end = list == std::string::npos ? std::string::npos : text.find("\n  ],", list);
    CHECK(list != std::string::npos && list_end != std::string::npos, "world_pool.json dynamic_candidates list present");
    CHECK(text.find("\"verdict\": ", list) >= list_end || text.find("\"sighting_world_translation\": [", list) < list_end, "world_pool.json dynamic_candidates rows carry sighting_world_translation");
    const char* verdicts[] = {"\"alpha_skipped\"", "\"not_resident\"", "\"released_unflagged\"", "\"no_light_records_since_frame ", "\"p2b_light_records_ok\""};
    for (size_t at = text.find("\"verdict\": ", list); list != std::string::npos && at != std::string::npos && at < list_end;
         at = text.find("\"verdict\": ", at + 1)) {
      bool known = false;
      for (const char* verdict : verdicts) known = known || text.compare(at + 11, std::strlen(verdict), verdict) == 0;
      CHECK(known, "world_pool.json dynamic_candidates verdict is a known string (at %zu)", at);
    }
  }
  {
    const std::vector<uint8_t> bytes = ReadFile((bvh::PoolOutputDir() / "world_pool.json").string());
    const std::string text(bytes.begin(), bytes.end());
    const size_t list = text.find("\"shadow_coverage\": [");
    const size_t list_end = list == std::string::npos ? std::string::npos : text.find("\n  ],", list);
    CHECK(list != std::string::npos && list_end != std::string::npos, "world_pool.json shadow_coverage list present");
    const char* verdicts[] = {"\"no_mesh\"", "\"mesh_failed:", "\"alpha_dynamic_skipped\"", "\"covered_static\"", "\"covered_p2b\"",
                              "\"no_admitted_instance\"", "\"no_light_records_since_frame "};
    for (size_t at = text.find("\"verdict\": ", list); list != std::string::npos && at != std::string::npos && at < list_end;
         at = text.find("\"verdict\": ", at + 1)) {
      bool known = false;
      for (const char* verdict : verdicts) known = known || text.compare(at + 11, std::strlen(verdict), verdict) == 0;
      CHECK(known, "world_pool.json shadow_coverage verdict is a known string (at %zu)", at);
    }
  }
  bvh::OnDestroyDevicePool(&dev);
  std::printf("%s (%d failures)\n", g_failures == 0 ? "PASS" : "FAILED", g_failures);
  Golden("end");
  PruneGolden();

  HStage("housekeeping schedule");
  {
    // PrunePool runs only at frame % 60 == kPoolPrunePhase (off the orphan grid, frame % 10 == 0).
    bvh::g_pool.scan_active.store(true);
    int prune_mismatch = 0;
    int pruned_count = 0;
    for (uint32_t f = 1200u; f < 1320u; ++f) {
      const bvh::InstanceKey key{99u, f};
      {
        std::lock_guard lock(bvh::g_pool.mutex);
        bvh::g_pool.observations[key].last_frame = 0u;  // stale: the next prune removes it
      }
      falcom_world::g_state.frame.store(f);
      bvh::DrainPoolScan(&dev, &queue);
      std::lock_guard lock(bvh::g_pool.mutex);
      const bool pruned = bvh::g_pool.observations.count(key) == 0u;
      const bool on_phase = f % 60u == bvh::kPoolPrunePhase;
      pruned_count += pruned ? 1 : 0;
      prune_mismatch += pruned != on_phase ? 1 : 0;
    }
    CHECK(prune_mismatch == 0, "prune runs exactly at frame %% 60 == %u (%d mismatches)", bvh::kPoolPrunePhase, prune_mismatch);
    CHECK(pruned_count == 2, "two prune frames in 120 presents (%d)", pruned_count);

    // A TLAS rebuild in a present defers the orphan pass by at most kPoolOrphanDeferFrames presents, then it runs.
    const auto present = [&](uint32_t f, bool rebuild) {
      falcom_world::g_state.frame.store(f);
      bvh::g_pool.rebuild_this_present.store(rebuild);
      bvh::DrainPoolScan(&dev, &queue);
      std::lock_guard lock(bvh::g_pool.mutex);
      return bvh::g_pool.orphan_deferred;
    };
    CHECK(present(1300u, false) == 0u, "grid frame without a rebuild: the pass runs");
    CHECK(present(1310u, true) == 1u, "grid frame with a rebuild: deferred (1)");
    CHECK(present(1311u, true) == 2u, "deferred again (2)");
    CHECK(present(1312u, true) == 3u, "deferred again (3)");
    CHECK(present(1313u, true) == 0u, "after 3 deferred presents the pass runs");
    CHECK(present(1314u, false) == 0u, "no rebuild: nothing pending");
    CHECK(present(1315u, true) == 0u, "a rebuild off the grid defers nothing");
    CHECK(present(1320u, true) == 1u, "grid frame 1320 with a rebuild: deferred (1)");
    CHECK(present(1321u, false) == 0u, "the pending pass runs on the next present without a rebuild");
  }
  return g_failures == 0 ? 0 : 1;
}
