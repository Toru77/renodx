// Native harness for the pool: mock device/command list execute copies
// immediately (as the GPU would at that point of the stream).
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "mesh_fixture.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_pool.hpp"

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

int main() {
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
  size_t schedules_before = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    std::printf("before map change: meshes %zu instances %zu\n", bvh::g_pool.meshes.size(), bvh::g_pool.instances.size());
    CHECK(bvh::g_pool.stats.mass_retirements == 0u, "no map change yet (%u)", bvh::g_pool.stats.mass_retirements);
    for (auto& [key, schedule] : bvh::g_pool.schedule) pre_release_next = (std::max)(pre_release_next, schedule.next_frame);
    schedules_before = bvh::g_pool.schedule.size();
  }
  // Pad so the sweep (change frame + settle) lands on a frame that is not a multiple of 60.
  while ((falcom_world::g_state.frame.load() + 1u + bvh::kPoolMapSettleFrames) % 60u == 0u) run_frame(frame++, false);
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
    CHECK(p.map_change_frame == falcom_world::g_state.frame.load(), "map change frame (%u)", p.map_change_frame);
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
    CHECK(falcom_world::g_state.frame.load() % 60u != 0u, "sweep frame is not a prune frame (%u)", falcom_world::g_state.frame.load());
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

  HStage("map change sweep waits while BudgetFull skips change");
  // "S3 stale" is drawn for 40 frames, then no longer drawn; the next map change must drop it.
  resource vb_s3;
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb_s3);
  fixture::WriteVertices(dev.res[vb_s3.handle].bytes, fixture::Box(9.f));
  objects.push_back({"S3 stale", vb_s3.handle, 0, kRigid, kOpaque, {{600, 0, 0}}});
  for (int i = 0; i < 40; ++i) run_frame(frame++, false);
  objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.vb == vb_s3.handle; }), objects.end());
  std::vector<resource> mass2(32);
  for (size_t i = 0; i < mass2.size(); ++i) {
    dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &mass2[i]);
    fixture::WriteVertices(dev.res[mass2[i].handle].bytes, fixture::Box(20.f + static_cast<float>(i)));
    objects.push_back({"M map2", mass2[i].handle, 0, kRigid, kOpaque, {{500.f + static_cast<float>(i), 0, 0}}});
  }
  for (int i = 0; i < 100; ++i) run_frame(frame++, false);
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

  bvh::DumpWorldPool();
  bvh::OnDestroyDevicePool(&dev);
  std::printf("%s (%d failures)\n", g_failures == 0 ? "PASS" : "FAILED", g_failures);
  return g_failures == 0 ? 0 : 1;
}
