// Native harness: pool diagnostic switches (capture meshes, scan indirect
// draws, log mesh captures), destroy during capture, OBJ grouping.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <thread>
#include "gen/mock_base.hpp"
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
  // Lock-rule probe: counts copies recorded while g_pool.mutex is held and,
  // when it is free, re-enters the destroy handler like D3D11 deferred
  // destruction would.
  bool probe_lock = false;
  int copies_under_lock = 0;
  int reentries = 0;
  resource reenter_resource = {0u};
  device *get_device() override { return dev; }
  void copy_buffer_region(resource src, uint64_t so, resource dst, uint64_t dof, uint64_t size) override {
    if (probe_lock) {
      if (falcom_world::bvh::g_pool.mutex.try_lock()) {
        falcom_world::bvh::g_pool.mutex.unlock();
        if (reenter_resource.handle != 0u) {
          falcom_world::bvh::OnDestroyResourcePool(dev, reenter_resource);
          ++reentries;
        }
      } else {
        ++copies_under_lock;
      }
    }
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
  bool deferred = false;       // drawn on a deferred command list
};

static size_t CountLines(const char* needle, char level = 0) {
  size_t n = 0;
  for (const auto& [lvl, line] : renodx::utils::log::g_lines) {
    if ((level == 0 || lvl == level) && line.find(needle) != std::string::npos) ++n;
  }
  return n;
}

int main() {
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  fixture::RegisterLayout();
  const uint64_t kRigid = 0xA1, kOpaque = 0xB1;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kOpaque, "0x2162672F.ps.cso", false);

  resource t15, b1, args, ib, vb_a, vb_b, vb_c, vb_d, vb_e;
  const uint64_t ring_elements = 150000;
  dev.create_resource(resource_desc(ring_elements * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::indirect_argument), nullptr, resource_usage::general, &args);
  for (resource* r : {&ib, &vb_a, &vb_b, &vb_c, &vb_d, &vb_e}) dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, r);
  fixture::WriteVertices(dev.res[vb_a.handle].bytes, fixture::Box(1.f));
  fixture::WriteVertices(dev.res[vb_b.handle].bytes, fixture::Box(2.f));
  fixture::WriteVertices(dev.res[vb_c.handle].bytes, fixture::Box(3.f));
  fixture::WriteVertices(dev.res[vb_d.handle].bytes, fixture::Box(4.f));
  fixture::WriteVertices(dev.res[vb_e.handle].bytes, fixture::Box(5.f));
  fixture::WriteIndices(dev.res[ib.handle].bytes);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;
  CmdList deferred_cl; deferred_cl.dev = &dev;
  falcom_world::WorldCommandListData deferred_data;
  deferred_data.vs_srv[15] = t15;
  deferred_data.vs_cb[1] = b1;

  uint64_t cursor = 777;
  auto run_frame = [&](int frame_index, const std::vector<Object>& objs) {
    for (size_t index = 0; index < objs.size(); ++index) {
      const Object& object = objs[index];
      if (cursor + object.instances >= ring_elements) cursor = 0;
      const int32_t base = static_cast<int32_t>(cursor);
      for (uint32_t i = 0; i < object.instances; ++i) {
        float t[3] = {object.x + static_cast<float>(i) * 3.f, 0.f, 0.f};
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
      draw.index_size = 2; draw.dsv = {0x77}; draw.depth_enable = true; draw.depth_write = true;
      draw.topology = primitive_topology::triangle_list;
      draw.vs_pipeline = kRigid; draw.ps_pipeline = kOpaque; draw.vs_hash = object.indirect ? 0x2000u : 0x1000u + static_cast<uint32_t>(index);
      draw.frame = falcom_world::g_state.frame.load();
      if (object.indirect) {
        const uint32_t a[5] = {object.args_index_count, object.args_instances, static_cast<uint32_t>(index) * 36u, 0u, 0u};
        std::memcpy(dev.res[args.handle].bytes.data() + object.args_offset, a, sizeof(a));
        bvh::OnPoolScanIndirectDraw(&dev, &cl, draw, &cl_data, args, object.args_offset, 1u, 20u);
      } else {
        draw.index_count = 36; draw.first_index = static_cast<uint32_t>(index) * 36u; draw.instance_count = object.instances;
        if (object.deferred) {
          bvh::OnPoolScanDraw(&dev, &deferred_cl, draw, &deferred_data);
        } else {
          bvh::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
        }
      }
    }
    falcom_world::g_state.frame.fetch_add(1u);
    bvh::DrainPoolScan(&dev, &queue);
  };
  auto snapshot = [&]() {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::UpdatePoolStats();
    return bvh::g_pool.stats;
  };

  std::vector<Object> objects;
  { Object o{"direct a", vb_a.handle, 2}; o.x = 10; objects.push_back(o); }
  { Object o{"direct b", vb_b.handle, 1}; o.x = 30; objects.push_back(o); }
  { Object o{"indirect c", vb_c.handle, 3}; o.indirect = true; o.args_instances = 3; o.x = 50; o.args_offset = 0; objects.push_back(o); }

  bvh::g_pool.scan_active.store(true);
  int frame = 1;

  // 1. Defaults: capture on, indirect on, log off, verification on. This test
  // covers single-capture mechanics; verification has test_verify.cpp.
  CHECK(bvh::g_pool.capture_meshes.load() && bvh::g_pool.scan_indirect.load() && !bvh::g_pool.log_captures.load(), "defaults");
  CHECK(bvh::g_pool.verify_meshes.load() && !bvh::g_pool.legacy_scale.load(), "verification defaults");
  bvh::g_pool.verify_meshes.store(false);

  // 2. Capture meshes off: draws and instances are still observed, no mesh is copied.
  bvh::g_pool.capture_meshes.store(false);
  for (int i = 0; i < 40; ++i) run_frame(frame++, objects);
  {
    const auto s = snapshot();
    std::printf("capture off: index copies %llu meshes %zu waiting %zu observed %zu admitted %zu\n",
                (unsigned long long)s.mesh_index_copies, s.meshes, s.mesh_queue, s.observed, s.admitted);
    CHECK(s.mesh_index_copies == 0 && s.mesh_vertex_copies == 0, "no mesh copy while off");
    CHECK(s.meshes == 0 && s.admitted == 0, "nothing admitted without meshes");
    CHECK(s.mesh_queue == 3, "three meshes wait (%zu)", s.mesh_queue);
    CHECK(s.observed == 6, "six instances observed (%zu)", s.observed);
    CHECK(s.indirect_copied > 0, "indirect scanned");
  }
  CHECK(renodx::utils::log::g_lines.empty(), "no log while the log switch is off (%zu)", renodx::utils::log::g_lines.size());

  // 3. Capture back on: index copy, then vertex copy, then the instances admit.
  bvh::g_pool.capture_meshes.store(true);
  for (int i = 0; i < 8; ++i) run_frame(frame++, objects);
  {
    const auto s = snapshot();
    std::printf("capture on: copies %llu+%llu meshes %zu waiting %zu in flight %zu admitted %zu\n",
                (unsigned long long)s.mesh_index_copies, (unsigned long long)s.mesh_vertex_copies, s.meshes,
                s.mesh_queue, s.mesh_in_flight, s.admitted);
    CHECK(s.meshes == 3 && s.mesh_queue == 0 && s.mesh_in_flight == 0, "meshes read (%zu, waiting %zu)", s.meshes, s.mesh_queue);
    CHECK(s.admitted == 6, "admitted %zu", s.admitted);
    CHECK(s.mesh_index_copies == 3 && s.mesh_vertex_copies == 3, "copies %llu+%llu",
          (unsigned long long)s.mesh_index_copies, (unsigned long long)s.mesh_vertex_copies);
    CHECK(s.mesh_failures == 0, "failures %u (%s)", s.mesh_failures, s.last_mesh_error.c_str());
  }
  CHECK(renodx::utils::log::g_lines.empty(), "still no log (%zu)", renodx::utils::log::g_lines.size());

  // 4. Scan indirect draws off: indirect draws leave no trace.
  {
    const auto before = snapshot();
    bvh::g_pool.scan_indirect.store(false);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.schedule.clear();  // everything due again
    }
    for (int i = 0; i < 4; ++i) run_frame(frame++, objects);
    const auto after = snapshot();
    const uint64_t indirect_before = before.indirect_by_vs_class[(size_t)contract::VsClass::Rigid];
    const uint64_t indirect_after = after.indirect_by_vs_class[(size_t)contract::VsClass::Rigid];
    std::printf("indirect off: indirect seen %llu -> %llu copied %llu -> %llu direct copied %llu -> %llu\n",
                (unsigned long long)indirect_before, (unsigned long long)indirect_after,
                (unsigned long long)before.indirect_copied, (unsigned long long)after.indirect_copied,
                (unsigned long long)before.copied_draws, (unsigned long long)after.copied_draws);
    CHECK(indirect_after == indirect_before && after.indirect_copied == before.indirect_copied, "indirect ignored");
    CHECK(after.copied_draws > before.copied_draws, "direct draws still copied");
    bvh::g_pool.scan_indirect.store(true);
    run_frame(frame++, objects);
    run_frame(frame++, objects);
    const auto again = snapshot();
    CHECK(again.indirect_by_vs_class[(size_t)contract::VsClass::Rigid] > indirect_after, "indirect back on");
  }

  // 5. Switch log: nothing while logging is off, one line when on.
  bvh::LogPoolSwitches();
  CHECK(renodx::utils::log::g_lines.empty(), "switch log gated");
  bvh::g_pool.log_captures.store(true);
  bvh::LogPoolSwitches();
  CHECK(CountLines("switches: pool scan on, capture meshes on, scan indirect draws on, verify mesh captures off", 'i') == 1, "switch line");

  // 6. Mesh log: a line per copy issued and per copy read.
  {
    const size_t lines = renodx::utils::log::g_lines.size();
    Object o{"direct d", vb_d.handle, 1}; o.x = 70; objects.push_back(o);
    for (int i = 0; i < 8; ++i) run_frame(frame++, objects);
    const auto s = snapshot();
    std::printf("log on: meshes %zu, lines %zu -> %zu\n", s.meshes, lines, renodx::utils::log::g_lines.size());
    CHECK(s.meshes == 4, "d added (%zu)", s.meshes);
    CHECK(CountLines("indices copy issued", 'i') == 1, "index copy line");
    CHECK(CountLines("indices read: frame", 'i') == 1 && CountLines("36 indices, vertices 0..7", 'i') == 1, "index read line");
    CHECK(CountLines("vertices copy issued", 'i') == 1, "vertex copy line");
    CHECK(CountLines("| added | 12 triangles", 'i') == 1, "vertex read line");
    CHECK(renodx::utils::log::g_lines.size() == lines + 4, "exactly four lines (%zu)", renodx::utils::log::g_lines.size() - lines);
    for (size_t i = lines; i < renodx::utils::log::g_lines.size(); ++i) std::printf("  %s\n", renodx::utils::log::g_lines[i].second.c_str());
  }

  // 7. A buffer released while its mesh copy is in flight: the copy is dropped
  // when read, nothing is added.
  {
    Object o{"direct e", vb_e.handle, 1}; o.x = 90; objects.push_back(o);
    run_frame(frame++, objects);  // queues e and issues its index copy
    objects.pop_back();           // the game no longer draws e
#ifdef THREADED
    std::thread([&] { bvh::OnDestroyResourcePool(&dev, vb_e); }).join();
#else
    bvh::OnDestroyResourcePool(&dev, vb_e);
#endif
    for (int i = 0; i < 4; ++i) run_frame(frame++, objects);
    const auto s = snapshot();
    std::printf("released in flight: dropped %u index copies %llu meshes %zu\n", s.mesh_dropped,
                (unsigned long long)s.mesh_index_copies, s.meshes);
    CHECK(s.mesh_dropped == 1, "dropped (%u)", s.mesh_dropped);
    CHECK(s.mesh_index_copies == 5, "e's index copy was issued (%llu)", (unsigned long long)s.mesh_index_copies);
    CHECK(CountLines("indices read: frame", 'i') == 2 && CountLines("dropped (mesh released or re-queued)", 'i') == 1, "dropped line");
    CHECK(s.meshes == 4, "e not added (%zu)", s.meshes);
    CHECK(CountLines("1 tracked vertex/index buffers released", 'i') == 1, "release summary");
  }

  // 8. Release summary is gated, and turning the log on later does not dump old counts.
  {
    bvh::g_pool.log_captures.store(false);
    const size_t lines = renodx::utils::log::g_lines.size();
    bvh::OnDestroyResourcePool(&dev, vb_d);
    objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const Object& o) { return o.vb == vb_d.handle; }), objects.end());
    run_frame(frame++, objects);
    CHECK(renodx::utils::log::g_lines.size() == lines, "no line while off");
    bvh::g_pool.log_captures.store(true);
    run_frame(frame++, objects);
    CHECK(renodx::utils::log::g_lines.size() == lines, "no stale release line (%zu)", renodx::utils::log::g_lines.size() - lines);
  }

  // 9. OBJ: one object per vertex-shader family, each once.
  {
    bvh::DumpWorldPoolObj();
    std::ifstream obj(bvh::PoolOutputDir() / "world_pool_region.obj");
    std::string line;
    std::vector<std::string> objects_seen;
    size_t faces = 0;
    while (std::getline(obj, line)) {
      if (line.rfind("o ", 0) == 0) objects_seen.push_back(line);
      if (line.rfind("f ", 0) == 0) ++faces;
    }
    for (auto& o : objects_seen) std::printf("  %s\n", o.c_str());
    std::vector<std::string> expected = {"o vs_0x00001000", "o vs_0x00001001", "o vs_0x00002000"};
    CHECK(objects_seen == expected, "objects %zu", objects_seen.size());
    const auto s = snapshot();
    CHECK(faces == s.region * 12u, "faces %zu region %zu", faces, s.region);
  }

  // 10. JSON dump carries the switches and the new counters.
  {
    bvh::g_pool.capture_meshes.store(false);
    std::filesystem::remove(bvh::PoolOutputDir() / "world_pool.json");
    bvh::DumpWorldPool();
    std::ifstream json(bvh::PoolOutputDir() / "world_pool.json");
    const std::string text((std::istreambuf_iterator<char>(json)), std::istreambuf_iterator<char>());
    CHECK(text.find("\"switches\": {\"capture_meshes\": false, \"scan_indirect\": true, \"verify_meshes\": false, "
                    "\"legacy_scale\": false, \"exclude_moving\": true}") != std::string::npos, "switches in dump");
    CHECK(text.find("\"mesh_dropped\": 1") != std::string::npos, "dropped in dump");
    CHECK(text.find("\"mesh_index_copies\": 5, \"mesh_vertex_copies\": 4") != std::string::npos, "copies in dump");
    bvh::g_pool.capture_meshes.store(true);
  }

  // 11. Deferred-context counter and stage markers.
  {
    CmdList deferred; deferred.dev = &dev;
    falcom_world::WorldCommandListData deferred_data;
    deferred_data.vs_srv[15] = t15;
    deferred_data.vs_cb[1] = b1;
    const auto before = snapshot();
    CHECK(bvh::g_pool.immediate_cmd_list.load() == reinterpret_cast<uint64_t>(static_cast<command_list*>(&cl)), "immediate recorded");
    falcom_world::DrawRecord draw;
    draw.method = 1; draw.has_index_buffer = true; draw.vb = vb_a; draw.vb_stride = 32; draw.ib = ib;
    draw.index_size = 2; draw.dsv = {0x77}; draw.depth_enable = true; draw.depth_write = true;
    draw.topology = primitive_topology::triangle_list; draw.vs_pipeline = kRigid; draw.ps_pipeline = kOpaque;
    draw.index_count = 36; draw.instance_count = 1; draw.vs_hash = 0x3000u;
    bvh::OnPoolScanDraw(&dev, &deferred, draw, &deferred_data);
    bvh::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
    bvh::OnPoolScanIndirectDraw(&dev, &deferred, draw, &deferred_data, args, 0u, 1u, 20u);
    const auto after = snapshot();
    std::printf("deferred: draws %llu -> %llu, indirect %llu -> %llu\n",
                (unsigned long long)before.draws_on_deferred, (unsigned long long)after.draws_on_deferred,
                (unsigned long long)before.indirect_on_deferred, (unsigned long long)after.indirect_on_deferred);
    CHECK(after.draws_on_deferred == before.draws_on_deferred + 2u, "two deferred draws");
    CHECK(after.indirect_on_deferred == before.indirect_on_deferred + 1u, "one deferred indirect");
    CHECK(falcom_world::CurrentPoolStage() == nullptr, "stage restored");
    {
      falcom_world::PoolStageScope outer("outer");
      bvh::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
      CHECK(std::string(falcom_world::CurrentPoolStage()) == "outer", "nested stage restored");
    }
  }

  // 11b. Mesh copies are taken only at draws on the immediate context.
  {
    resource vb_f;
    dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb_f);
    fixture::WriteVertices(dev.res[vb_f.handle].bytes, fixture::Box(6.f));
    bvh::g_pool.capture_meshes.store(false);
    { Object o{"direct f", vb_f.handle, 1}; o.x = 110; objects.push_back(o); }
    for (int i = 0; i < 2; ++i) run_frame(frame++, objects);  // f queued, waiting
    bvh::g_pool.capture_meshes.store(true);
    objects.back().deferred = true;
    const auto before = snapshot();
    for (int i = 0; i < 6; ++i) run_frame(frame++, objects);
    const auto mid = snapshot();
    std::printf("deferred only: skips %llu -> %llu, index copies %llu -> %llu, meshes %zu, waiting %zu\n",
                (unsigned long long)before.mesh_deferred_skips, (unsigned long long)mid.mesh_deferred_skips,
                (unsigned long long)before.mesh_index_copies, (unsigned long long)mid.mesh_index_copies, mid.meshes,
                mid.mesh_queue);
    CHECK(mid.mesh_deferred_skips >= before.mesh_deferred_skips + 6u, "deferred draws skipped");
    CHECK(mid.mesh_index_copies == before.mesh_index_copies && mid.meshes == before.meshes, "no copy from deferred");
    CHECK(mid.mesh_queue == 1u, "f waits (%zu)", mid.mesh_queue);
    objects.back().deferred = false;
    for (int i = 0; i < 8; ++i) run_frame(frame++, objects);
    const auto after = snapshot();
    std::printf("immediate again: meshes %zu -> %zu, waiting %zu\n", mid.meshes, after.meshes, after.mesh_queue);
    CHECK(after.meshes == mid.meshes + 1u && after.mesh_queue == 0u, "f captured at an immediate draw");
    CHECK(after.mesh_deferred_skips == mid.mesh_deferred_skips, "no skips once captured");
    CHECK(dev.bad_copies == 0, "copies in range (%d)", dev.bad_copies);
  }

  // 11c. A waiting mesh that is still drawn does not expire; one no longer
  // drawn does, and queueing it again maps its buffer once.
  {
    resource vb_g;
    dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb_g);
    fixture::WriteVertices(dev.res[vb_g.handle].bytes, fixture::Box(7.f));
    bvh::g_pool.capture_meshes.store(false);
    { Object o{"direct g", vb_g.handle, 1}; o.x = 130; objects.push_back(o); }
    const auto before = snapshot();
    for (int i = 0; i < 700; ++i) run_frame(frame++, objects);
    const auto drawn = snapshot();
    CHECK(drawn.mesh_expired == before.mesh_expired && drawn.mesh_queue == 1u, "drawn mesh kept (expired %u, waiting %zu)",
          drawn.mesh_expired, drawn.mesh_queue);
    objects.pop_back();
    for (int i = 0; i < 700; ++i) run_frame(frame++, objects);
    const auto gone = snapshot();
    CHECK(gone.mesh_expired == before.mesh_expired + 1u && gone.mesh_queue == 0u, "undrawn mesh expired (%u, waiting %zu)",
          gone.mesh_expired, gone.mesh_queue);
    { Object o{"direct g", vb_g.handle, 1}; o.x = 130; objects.push_back(o); }
    run_frame(frame++, objects);
    size_t mapped = 0u;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      const auto it = bvh::g_pool.keys_by_resource.find(vb_g.handle);
      mapped = it == bvh::g_pool.keys_by_resource.end() ? 0u : it->second.size();
    }
    CHECK(mapped == 1u, "buffer mapped once (%zu)", mapped);
    bvh::g_pool.capture_meshes.store(true);
    for (int i = 0; i < 8; ++i) run_frame(frame++, objects);
    const auto after = snapshot();
    std::printf("expiry: expired %u -> %u, mapped %zu, meshes %zu -> %zu\n", before.mesh_expired, gone.mesh_expired, mapped,
                before.meshes, after.meshes);
    CHECK(after.meshes == before.meshes + 1u && after.mesh_queue == 0u, "g captured after re-queue");
    objects.pop_back();
  }

  // 12. Lock rule: copies are recorded outside g_pool.mutex, and a destroy
  // event raised inside a copy (D3D11 deferred destruction) re-enters safely.
  {
    resource unrelated;
    dev.create_resource(resource_desc(64, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &unrelated);
    cl.probe_lock = true;
    cl.reenter_resource = unrelated;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.schedule.clear();  // everything due again
    }
    const auto before = snapshot();
    for (int i = 0; i < 3; ++i) run_frame(frame++, objects);
    const auto after = snapshot();
    std::printf("lock rule on: copies under lock %d, re-entries %d, copied %llu -> %llu, mismatch %llu\n", cl.copies_under_lock,
                cl.reentries, (unsigned long long)before.copied_draws, (unsigned long long)after.copied_draws,
                (unsigned long long)after.base_mismatch);
    CHECK(cl.copies_under_lock == 0, "no copy under the lock (%d)", cl.copies_under_lock);
    CHECK(cl.reentries > 0, "destroy re-entered during copies");
    CHECK(after.copied_draws > before.copied_draws && after.indirect_copied > before.indirect_copied, "direct and indirect copied");
    CHECK(after.base_mismatch == 0, "b1 still verified");
    CHECK(dev.bad_copies == 0, "copies in range (%d)", dev.bad_copies);

    cl.probe_lock = false;
  }

  // 13. Reset clears the release baseline.
  bvh::ResetWorldPool();
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.logged_invalidations == 0u && bvh::g_pool.stats.mesh_index_copies == 0u && bvh::g_pool.mesh_requests.empty() && bvh::g_pool.mesh_waiting.empty(), "reset");
  }

  bvh::OnDestroyDevicePool(&dev);
  std::printf("%s (%d failures)\n", g_failures == 0 ? "PASS" : "FAILED", g_failures);
  return g_failures == 0 ? 0 : 1;
}
