// Native harness: indirect draws, draw-state reasons, matrix reject reasons,
// near misses. Mock device/command list execute copies immediately.
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

static void World(const float t[3], float scale, float out[12]) {
  const float m[12] = {scale, 0, 0, t[0], 0, scale, 0, t[1], 0, 0, scale, t[2]};
  std::memcpy(out, m, sizeof(m));
}

struct Object {
  const char* name;
  uint64_t vb;
  uint32_t instances;          // instances written to t15
  bool indirect = false;
  uint32_t args_instances = 0; // instance_count in the args (indirect)
  uint32_t args_index_count = 36;
  float scale = 1.f;
  float jitter = 0.f;          // per-frame translation change
  bool blend = false;
  bool depth_write = true;
  float x = 0.f;
  uint64_t args_offset = 0;
};

int main() {
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  fixture::RegisterLayout();
  const uint64_t kRigid = 0xA1, kOpaque = 0xB1;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kOpaque, "0x2162672F.ps.cso", false);

  resource t15, b1, args, ib, vb_a, vb_b, vb_c, vb_dead;
  const uint64_t ring_elements = 150000;
  dev.create_resource(resource_desc(ring_elements * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::indirect_argument), nullptr, resource_usage::general, &args);
  for (resource* r : {&ib, &vb_a, &vb_b, &vb_c, &vb_dead}) dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, r);
  fixture::WriteVertices(dev.res[vb_a.handle].bytes, fixture::Box(1.f));
  fixture::WriteVertices(dev.res[vb_b.handle].bytes, fixture::Box(2.f));
  fixture::WriteVertices(dev.res[vb_c.handle].bytes, fixture::Box(3.f));
  fixture::WriteVertices(dev.res[vb_dead.handle].bytes, fixture::Box(4.f));
  fixture::WriteIndices(dev.res[ib.handle].bytes);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;

  std::vector<Object> objects;
  { Object o{"indirect 3", vb_a.handle, 3}; o.indirect = true; o.args_instances = 3; o.x = 100; o.args_offset = 0; objects.push_back(o); }
  { Object o{"indirect truncated", vb_b.handle, 600}; o.indirect = true; o.args_instances = 600; o.x = 1000; o.args_offset = 64; objects.push_back(o); }
  { Object o{"indirect empty", vb_c.handle, 1}; o.indirect = true; o.args_instances = 0; o.x = 3000; o.args_offset = 128; objects.push_back(o); }
  { Object o{"indirect bad index count", vb_c.handle, 1}; o.indirect = true; o.args_instances = 1; o.args_index_count = 35; o.x = 3100; o.args_offset = 192; objects.push_back(o); }
  { Object o{"direct zero scale", vb_c.handle, 1}; o.scale = 0.f; o.x = 4000; objects.push_back(o); }
  { Object o{"direct large scale", vb_c.handle, 1}; o.scale = 100.f; o.x = 4100; objects.push_back(o); }
  { Object o{"direct jitter", vb_c.handle, 1}; o.jitter = 1e-5f; o.x = 4200; objects.push_back(o); }
  { Object o{"direct blend", vb_c.handle, 1}; o.blend = true; o.x = 4300; objects.push_back(o); }
  { Object o{"direct no depth write", vb_c.handle, 1}; o.depth_write = false; o.x = 4400; objects.push_back(o); }
  { Object o{"direct huge scale", vb_c.handle, 1}; o.scale = 20000.f; o.x = 4500; objects.push_back(o); }
  { Object o{"direct far away", vb_c.handle, 1}; o.x = 60000; objects.push_back(o); }
  { Object o{"direct small scale", vb_c.handle, 1}; o.scale = 0.02f; o.x = 4600; objects.push_back(o); }

  uint64_t cursor = 777;
  auto run_frame = [&](int frame_index, const std::vector<Object>& objs) {
    for (size_t index = 0; index < objs.size(); ++index) {
      const Object& object = objs[index];
      if (cursor + object.instances >= ring_elements) cursor = 0;
      const int32_t base = static_cast<int32_t>(cursor);
      for (uint32_t i = 0; i < object.instances; ++i) {
        float t[3] = {object.x + static_cast<float>(i) * 3.f, 0.f, 0.f};
        if (object.jitter != 0.f) t[1] = static_cast<float>(frame_index) * object.jitter + 5.f;
        float world[12];
        World(t, object.scale, world);
        uint8_t* element = dev.res[t15.handle].bytes.data() + (cursor + i) * 160;
        std::memcpy(element, world, 48);
        std::memcpy(element + 48, world, 48);
      }
      cursor += object.instances + 5;
      int32_t cb[4] = {base, 0, 0, 0};
      std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
      falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);

      falcom_world::DrawRecord draw;
      draw.method = 1; draw.has_index_buffer = true; draw.vb = {object.vb}; draw.vb_stride = fixture::kStride; draw.ib = ib;
      draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {fixture::kLayout};
      draw.index_size = 2; draw.dsv = {0x77}; draw.depth_enable = true; draw.depth_write = object.depth_write;
      draw.blend_enable = object.blend; draw.topology = primitive_topology::triangle_list;
      draw.vs_pipeline = kRigid; draw.ps_pipeline = kOpaque; draw.vs_hash = 0x1000u + static_cast<uint32_t>(index);
      if (object.indirect) {
        const uint32_t a[5] = {object.args_index_count, object.args_instances, static_cast<uint32_t>(index) * 36u, 0u, 0u};
        std::memcpy(dev.res[args.handle].bytes.data() + object.args_offset, a, sizeof(a));
        bvh::OnPoolScanIndirectDraw(&dev, &cl, draw, &cl_data, args, object.args_offset, 1u, 20u);
      } else {
        draw.index_count = 36; draw.first_index = static_cast<uint32_t>(index) * 36u; draw.instance_count = object.instances;
        bvh::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
      }
    }
    falcom_world::g_state.frame.fetch_add(1u);
    bvh::DrainPoolScan(&dev, &queue);
  };

  bvh::g_pool.scan_active.store(true);
  int frame = 1;
  for (; frame <= 80; ++frame) run_frame(frame, objects);

  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    auto& s = p.stats;
    std::printf("instances %zu meshes %zu indirect copied %llu resolved %llu empty %llu truncated %llu dead %llu near %llu\n",
                p.instances.size(), p.meshes.size(), (unsigned long long)s.indirect_copied, (unsigned long long)s.indirect_resolved,
                (unsigned long long)s.indirect_empty, (unsigned long long)s.indirect_truncated, (unsigned long long)s.indirect_dead,
                (unsigned long long)s.near_misses);
    CHECK(dev.bad_copies == 0, "bad copies %d", dev.bad_copies);
    CHECK(s.base_mismatch == 0, "mismatch");
    std::map<float, int> xs;
    for (auto& inst : p.instances) xs[inst.matrix[3]]++;
    for (float x : {100.f, 103.f, 106.f}) CHECK(xs.count(x) == 1, "indirect instance x=%f", x);
    // 600 requested, window 512: the first 512 are admitted
    int truncated_admitted = 0;
    for (auto& [x, n] : xs) if (x >= 1000.f && x < 3000.f) truncated_admitted += n;
    CHECK(truncated_admitted == 512, "truncated admitted %d", truncated_admitted);
    CHECK(s.indirect_truncated > 0, "truncated counted");
    CHECK(s.indirect_empty > 0, "empty counted");
    CHECK(s.draw_state[(size_t)bvh::PoolDrawState::IndexCount] > 0, "index count reason");
    CHECK(xs.count(3000.f) == 0 && xs.count(3100.f) == 0, "empty/bad index not admitted");
    CHECK(s.matrix_rejects[(size_t)bvh::PoolMatrixReject::ZeroScale] > 0, "zero scale");
    CHECK(s.matrix_rejects[(size_t)bvh::PoolMatrixReject::LargeScale] > 0, "huge scale");
    CHECK(s.matrix_rejects[(size_t)bvh::PoolMatrixReject::FarAway] > 0, "far away");
    CHECK(s.matrix_rejects[(size_t)bvh::PoolMatrixReject::SmallScale] == 0, "no small-scale reject at 0.02");
    CHECK(xs.count(4000.f) == 0 && xs.count(4500.f) == 0 && xs.count(60000.f) == 0, "rejected not admitted");
    CHECK(xs.count(4100.f) == 1, "scale 100 admitted");
    CHECK(xs.count(4600.f) == 1, "scale 0.02 admitted");
    CHECK(s.mesh_failures == 0, "mesh failures %u (%s)", s.mesh_failures, s.last_mesh_error.c_str());
    // Adaptive windows, per indirect identity.
    const auto window_of = [&](uint64_t vb, uint64_t args_offset) {
      falcom_world::DrawRecord key_draw;
      key_draw.vb = {vb};
      key_draw.ib = ib;
      const auto it = p.schedule.find(bvh::PoolIndirectScheduleKey(key_draw, args, args_offset));
      return it == p.schedule.end() ? 0u : it->second.indirect_window;
    };
    std::printf("windows: 3 instances %u, 600 instances %u, empty %u, bad index count %u, avg %.1f\n",
                window_of(vb_a.handle, 0), window_of(vb_b.handle, 64), window_of(vb_c.handle, 128),
                window_of(vb_c.handle, 192), (double)s.indirect_window_instances / (double)s.indirect_copied);
    CHECK(window_of(vb_a.handle, 0) == 32u, "3 instances -> 32");
    CHECK(window_of(vb_b.handle, 64) == 512u, "600 instances -> 512");
    CHECK(window_of(vb_c.handle, 128) == 16u, "empty -> 16");
    CHECK(s.indirect_window_instances < 512u * s.indirect_copied, "windows shrank");
    {
      uint64_t identities = 0u;
      for (const auto& [key, schedule] : p.schedule) if (schedule.indirect_window != 0u) ++identities;
      std::printf("first-time copies %llu, identities %llu, copied %llu\n", (unsigned long long)s.indirect_first_copies,
                  (unsigned long long)identities, (unsigned long long)s.indirect_copied);
      CHECK(s.indirect_first_copies == identities && s.indirect_copied > identities, "one full-window copy per identity");
    }
    CHECK(s.near_misses > 0, "near miss");
    CHECK(xs.count(4200.f) == 0, "jitter not admitted");
    CHECK(s.draw_state[(size_t)bvh::PoolDrawState::Blend] > 0, "blend reason");
    CHECK(s.draw_state[(size_t)bvh::PoolDrawState::NoDepthWrite] > 0, "no depth write reason");
    CHECK(s.indirect_by_vs_class[(size_t)contract::VsClass::Rigid] > 0, "indirect class count");
    CHECK(p.indirect_refs.empty() || p.indirect_refs.size() <= 4, "refs bounded (%zu)", p.indirect_refs.size());
    const auto& fam = p.families[0x1000u + 6u];
    CHECK(fam.near_misses > 0 && fam.near_miss_samples.size() >= 2, "family near samples");
    const auto& fam_zero = p.families[0x1000u + 4u];
    CHECK(!fam_zero.reject_samples.empty() && fam_zero.reject_samples[0].reason == (uint8_t)bvh::PoolMatrixReject::ZeroScale, "reject sample");
  }

  HStage("released VB");
  // A VB released between an indirect draw and its readback: no mesh capture.
  {
    std::vector<Object> dead_objects;
    Object o{"indirect dead", vb_dead.handle, 2}; o.indirect = true; o.args_instances = 2; o.x = 5000; o.args_offset = 256;
    dead_objects.push_back(o);
    const uint64_t copies_before = bvh::g_pool.stats.mesh_index_copies + bvh::g_pool.stats.mesh_vertex_copies;
    // record the draw, then release the buffer before the slot is read back
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.schedule.clear();
    }
    run_frame(frame++, dead_objects);
    bvh::OnDestroyResourcePool(&dev, vb_dead);
    dev.destroy_resource(vb_dead);
    for (int i = 0; i < 4; ++i) run_frame(frame++, {});
    std::lock_guard lock(bvh::g_pool.mutex);
    const uint64_t copies_after = bvh::g_pool.stats.mesh_index_copies + bvh::g_pool.stats.mesh_vertex_copies;
    std::printf("dead: indirect_dead %llu mesh copies %llu->%llu refs %zu dead set %zu\n",
                (unsigned long long)bvh::g_pool.stats.indirect_dead, (unsigned long long)copies_before,
                (unsigned long long)copies_after, bvh::g_pool.indirect_refs.size(), bvh::g_pool.indirect_dead.size());
    CHECK(bvh::g_pool.stats.indirect_dead == 1, "dead counted");
    CHECK(copies_after == copies_before, "no mesh copy for a released buffer");
    CHECK(bvh::g_pool.indirect_dead.empty(), "dead set cleared");
  }

  HStage("instance growth");
  // A draw whose instance count grows: the window follows (32, then 144) and
  // every instance is admitted once it fits.
  {
    std::vector<Object> grow;
    Object o{"indirect grows", vb_b.handle, 100}; o.indirect = true; o.args_instances = 3; o.x = 7000; o.args_offset = 320;
    grow.push_back(o);
    for (int i = 0; i < 40; ++i) run_frame(frame++, grow);
    falcom_world::DrawRecord key_draw;
    key_draw.vb = vb_b;
    key_draw.ib = ib;
    const uint64_t key = bvh::PoolIndirectScheduleKey(key_draw, args, 320);
    uint32_t small_window = 0u;
    { std::lock_guard lock(bvh::g_pool.mutex); small_window = bvh::g_pool.schedule[key].indirect_window; }
    grow[0].args_instances = 100;
    for (int i = 0; i < 130; ++i) run_frame(frame++, grow);
    std::lock_guard lock(bvh::g_pool.mutex);
    int admitted = 0;
    for (auto& inst : bvh::g_pool.instances) if (inst.matrix[3] >= 7000.f && inst.matrix[3] < 7300.f) ++admitted;
    std::printf("grow: window %u -> %u, admitted %d\n", small_window, bvh::g_pool.schedule[key].indirect_window, admitted);
    CHECK(small_window == 32u, "small window %u", small_window);
    CHECK(bvh::g_pool.schedule[key].indirect_window == 144u, "grown window %u", bvh::g_pool.schedule[key].indirect_window);
    CHECK(admitted == 100, "all grown instances admitted (%d)", admitted);
  }

  HStage("reset in flight");
  // Reset with copies in flight releases their references.
  {
    { std::lock_guard lock(bvh::g_pool.mutex); bvh::g_pool.schedule.clear(); }
    std::vector<Object> one = {objects[0]};
    for (auto& o : one) { o.x = 9000; }
    for (int i = 0; i < 1; ++i) {
      // record without draining
      const Object& object = one[0];
      int32_t cb[4] = {0, 0, 0, 0};
      std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
      falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);
      falcom_world::DrawRecord draw;
      draw.method = 1; draw.has_index_buffer = true; draw.vb = {object.vb}; draw.ib = ib; draw.dsv = {0x77};
      draw.depth_enable = true; draw.depth_write = true; draw.topology = primitive_topology::triangle_list;
      draw.vs_pipeline = kRigid; draw.ps_pipeline = kOpaque;
      bvh::OnPoolScanIndirectDraw(&dev, &cl, draw, &cl_data, args, 0, 1u, 20u);
    }
    bvh::ResetWorldPool();
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.indirect_refs.empty(), "refs released on reset (%zu)", bvh::g_pool.indirect_refs.size());
  }

  bvh::DumpWorldPool();
  bvh::OnDestroyDevicePool(&dev);
  std::printf("%s (%d failures)\n", g_failures == 0 ? "PASS" : "FAILED", g_failures);
  return g_failures == 0 ? 0 : 1;
}
