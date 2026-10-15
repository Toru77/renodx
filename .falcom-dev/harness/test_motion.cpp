// Native harness: instance motion probe (path 2 discovery). Each object's
// pose function writes world and prevWorld per frame; the test checks the
// repeat/first split, the ULP and meter histograms, moved counts per draw,
// samples, the ghost listing in the dump, and that admission is unchanged.
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "json_check.hpp"
#include "mesh_fixture.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_pool.hpp"
#include "src/games/falcomengine-plus/world/bvh/p2b_diag.hpp"

using namespace reshade::api;
namespace bvh = falcom_world::bvh;
namespace contract = falcom_world::contract;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Res { resource_desc desc; std::vector<uint8_t> bytes; };
struct Device : mock::DeviceBase {
  std::map<uint64_t, Res> res;
  uint64_t next = 0x1000;
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
    if (s == dev->res.end() || d == dev->res.end() || so + size > s->second.bytes.size() || dof + size > d->second.bytes.size()) return;
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

// world/prev for element `i` at frame `f` (12 floats each, Inst4x3Row).
using Pose = std::function<void(uint32_t f, uint32_t i, float* world, float* prev)>;
struct Object {
  uint64_t vb;
  uint32_t instances = 1;
  Pose pose;
  uint64_t vs = 0u;  // pipeline; 0 = the camera rigid VS
  uint32_t first_index = UINT32_MAX;  // UINT32_MAX = 36 * object index (own draw key)
};

static void Place(float* m, float x, float y = 0.f, float z = 0.f, float rot = 0.f) {
  const float c = std::cos(rot), s = std::sin(rot);
  const float values[12] = {c, 0, s, x, 0, 1, 0, y, -s, 0, c, z};
  std::memcpy(m, values, sizeof(values));
}

int main() {
  HStage("0. ULP distance and buckets.");
  // 0. ULP distance and buckets.
  {
    const float one = 1.f, next = std::nextafter(1.f, 2.f);
    CHECK(bvh::PoolUlpDistance(one, next) == 1u, "1 ulp");
    CHECK(bvh::PoolUlpDistance(next, one) == 1u, "symmetric");
    CHECK(bvh::PoolUlpDistance(0.f, -0.f) == 0u, "+0/-0");
    CHECK(bvh::PoolUlpDistance(std::nextafter(0.f, 1.f), std::nextafter(0.f, -1.f)) == 2u, "across zero");
    CHECK(bvh::PoolUlpDistance(-1.f, 1.f) == 2u * 0x3F800000u, "-1..1 (%u)", bvh::PoolUlpDistance(-1.f, 1.f));
    CHECK(bvh::PoolUlpDistance(-FLT_MAX, FLT_MAX) == 2u * 0x7F7FFFFFu, "-max..max (no finite pair reaches the cap)");
    CHECK(bvh::PoolUlpDistance(NAN, 1.f) == UINT32_MAX && bvh::PoolUlpDistance(1.f, INFINITY) == UINT32_MAX, "non-finite");
    const uint32_t ulps[] = {0, 1, 2, 3, 4, 15, 16, 255, 256, 65535, 65536, UINT32_MAX};
    const size_t buckets[] = {0, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6};
    for (size_t i = 0; i < 12; ++i) CHECK(bvh::PoolMotionUlpBucket(ulps[i]) == buckets[i], "ulp bucket %u", ulps[i]);
    const float meters[] = {0.f, 1e-7f, 1e-6f, 2e-6f, 5e-4f, 1e-3f, 0.005f, 0.05f, 0.5f, 1.f, 2.f, NAN, INFINITY};
    const size_t mb[] = {0, 1, 1, 2, 4, 4, 5, 6, 7, 7, 8, 8, 8};
    for (size_t i = 0; i < 13; ++i) CHECK(bvh::PoolMotionMeterBucket(meters[i]) == mb[i], "meter bucket %g", meters[i]);
    float a[12], b[12];
    Place(a, 5.f); Place(b, 5.f);
    auto m = bvh::MeasurePoolMotion(a, b);
    CHECK(m.exact && !m.moved && m.ulps == 0u && m.meters == 0.f && m.basis == 0.f && m.finite, "identical");
    Place(b, 5.f - 0.002f);
    m = bvh::MeasurePoolMotion(a, b);
    CHECK(!m.exact && m.moved && std::fabs(m.meters - 0.002f) < 1e-5f && m.basis == 0.f, "2 mm moved (%g)", m.meters);
    Place(b, 5.f - 0.0005f);
    CHECK(!bvh::MeasurePoolMotion(a, b).moved, "0.5 mm not moved");
    Place(b, 5.f, 0.f, 0.f, 0.01f);
    m = bvh::MeasurePoolMotion(a, b);
    CHECK(m.moved && m.meters == 0.f && m.basis > 1e-4f, "rotation moved (basis %g)", m.basis);
    Place(b, 5.f); b[1] = NAN;
    m = bvh::MeasurePoolMotion(a, b);
    CHECK(!m.finite && !m.moved && m.ulps == UINT32_MAX && std::isnan(m.meters), "non-finite prev");
  }

  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  fixture::RegisterLayout();
  const uint64_t kRigid = 0xA1, kOpaque = 0xB1, kLightVs = 0xA2;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kLightVs, "0x062CCA2C.vs.cso", true);
  Classify(&dev, kOpaque, "0x2162672F.ps.cso", false);

  resource t15, b1, ib;
  const uint64_t ring_elements = 150000;
  dev.create_resource(resource_desc(ring_elements * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::index_buffer), nullptr, resource_usage::general, &ib);
  fixture::WriteIndices(dev.res[ib.handle].bytes);
  auto make_vb = [&](float sx) {
    resource vb;
    dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, &vb);
    fixture::WriteVertices(dev.res[vb.handle].bytes, fixture::Box(sx));
    return vb;
  };

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;

  uint64_t cursor = 777;
  std::vector<Object> objects;
  bool check_every_frame = false;
  // Ids strictly ascending; one admitted key per instance; every instance's key present.
  auto check_invariants = [&]() {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (size_t i = 1; i < bvh::g_pool.instances.size(); ++i) {
      CHECK(bvh::g_pool.instances[i - 1].id < bvh::g_pool.instances[i].id, "ids ascending at %zu", i);
    }
    CHECK(bvh::g_pool.admitted_keys.size() == bvh::g_pool.instances.size(), "admitted keys %zu, instances %zu",
          bvh::g_pool.admitted_keys.size(), bvh::g_pool.instances.size());
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      CHECK(bvh::g_pool.admitted_keys.count(bvh::PoolAdmittedKey(inst.mesh_id, falcom_world::MatrixHash(inst.matrix, bvh::kPoolWorldFloats))) != 0u,
            "instance key present (id %llu)", (unsigned long long)inst.id);
    }
  };
  std::vector<bvh::PoolDynamicDraw> dynamic_frame;  // P2b: the last present's flagged draws
  auto run_frame = [&]() {
    const uint32_t frame = falcom_world::g_state.frame.load();
    for (size_t index = 0; index < objects.size(); ++index) {
      const Object& object = objects[index];
      if (cursor + object.instances >= ring_elements) cursor = 0;
      const int32_t base = static_cast<int32_t>(cursor);
      for (uint32_t i = 0; i < object.instances; ++i) {
        float world[12], prev[12];
        object.pose(frame, i, world, prev);
        uint8_t* element = dev.res[t15.handle].bytes.data() + (cursor + i) * 160;
        std::memcpy(element, world, 48);
        std::memcpy(element + 48, prev, 48);
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
      draw.vs_pipeline = object.vs != 0u ? object.vs : kRigid; draw.ps_pipeline = kOpaque;
      draw.vs_hash = 0x1000u + static_cast<uint32_t>(index);
      draw.frame = frame;
      draw.index_count = 36;
      draw.first_index = object.first_index != UINT32_MAX ? object.first_index : static_cast<uint32_t>(index) * 36u;
      draw.instance_count = object.instances;
      bvh::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
    }
    falcom_world::g_state.frame.fetch_add(1u);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      dynamic_frame = bvh::g_pool.dynamic_draws;  // P2b: this present's flagged draws, before the drain clears them
    }
    bvh::DrainPoolScan(&dev, &queue);
    CHECK(bvh::g_pool.dynamic_draws.empty(), "P2b list cleared by the present");
    if (check_every_frame) check_invariants();
  };
  auto run = [&](int frames) { for (int i = 0; i < frames; ++i) run_frame(); };
  auto snapshot = [&]() {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::UpdatePoolStats();
    return bvh::g_pool.stats;
  };
  auto detail = [&]() {
    std::lock_guard lock(bvh::g_pool.mutex);
    return bvh::g_pool.motion_detail;
  };
  auto family = [&](uint32_t vs) {
    std::lock_guard lock(bvh::g_pool.mutex);
    return bvh::g_pool.families[vs];
  };
  auto fresh = [&]() {
    bvh::ResetWorldPool();
    objects.clear();
  };
  auto read_dump = [&]() {
    const std::filesystem::path json_path = bvh::PoolOutputDir() / "world_pool.json";
    std::filesystem::remove(json_path);
    bvh::DumpWorldPool();
    std::ifstream json(json_path);
    return std::string((std::istreambuf_iterator<char>(json)), std::istreambuf_iterator<char>());
  };

  bvh::g_pool.scan_active.store(true);
  CHECK(!bvh::g_pool.exclude_moving.load() && bvh::g_pool.follow_moving.load(), "keep moving objects out off, follow on by default");
  // Sections 1-6 test the probe alone: P2a and follow off (admission as before).
  bvh::g_pool.exclude_moving.store(false);
  bvh::g_pool.follow_moving.store(false);

  HStage("1. Static, prevWorld == world: every sighting after the first frame is a");
  // 1. Static, prevWorld == world: every sighting after the first frame is a
  // repeat in ULP bucket 0; nothing moves; admission as before.
  {
    fresh();
    objects.push_back({make_vb(1.f).handle, 2, [](uint32_t, uint32_t i, float* w, float* p) {
      Place(w, 10.f + 3.f * i); Place(p, 10.f + 3.f * i);
    }});
    run(120);
    const auto s = snapshot();
    const auto& m = s.motion;
    std::printf("static exact: repeat %llu first %llu exact %llu moved %llu admitted %zu\n", (unsigned long long)m.repeat,
                (unsigned long long)m.first, (unsigned long long)m.exact, (unsigned long long)m.moved, s.admitted);
    CHECK(m.first == 2u, "first sightings = one per instance (%llu)", (unsigned long long)m.first);
    CHECK(m.repeat >= 4u && m.repeat_ulps[0] == m.repeat && m.repeat_meters[0] == m.repeat, "repeat all exact");
    CHECK(m.exact == m.repeat + m.first, "exact count");
    CHECK(m.moved == 0u && m.moving_draws == 0u && m.moved_meshes == 0u && m.max_repeat_ulps == 0u, "nothing moved");
    CHECK(s.admitted == 2u, "admitted (%zu)", s.admitted);
    CHECK(s.moving_instances == 0u, "legacy counter unchanged");
    const auto f = family(0x1000u);
    CHECK(f.motion_repeat == m.repeat && f.motion_first == 2u && f.motion_moved == 0u, "family counts");
  }

  HStage("2. Static with prevWorld one ULP off in the translation: repeat in bucket");
  // 2. Static with prevWorld one ULP off in the translation: repeat in bucket
  // 1, not moved; the legacy bitwise counter counts it.
  {
    fresh();
    objects.push_back({make_vb(1.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) {
      Place(w, 1000.f); Place(p, 1000.f); p[3] = std::nextafter(1000.f, 2000.f);
    }});
    run(120);
    const auto s = snapshot();
    const auto& m = s.motion;
    CHECK(m.repeat >= 2u && m.repeat_ulps[1] == m.repeat && m.max_repeat_ulps == 1u, "1-ulp repeat (max %u)", m.max_repeat_ulps);
    CHECK(m.repeat_meters[3] == m.repeat, "6e-5 m bucket <=1e-4 (%llu of %llu)", (unsigned long long)m.repeat_meters[3],
          (unsigned long long)m.repeat);
    CHECK(m.moved == 0u && m.exact == 0u && s.moving_instances == m.repeat + m.first, "not moved, not exact");
    CHECK(detail().repeat_samples.empty(), "1 ulp is noise: no repeat sample");
    CHECK(s.admitted == 1u, "admitted");
  }

  HStage("3. A moving object (5 cm per frame, prevWorld = last frame's world):");
  // 3. A moving object (5 cm per frame, prevWorld = last frame's world):
  // every sighting is a first sighting that moved; never admitted.
  {
    fresh();
    objects.push_back({make_vb(1.f).handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      Place(w, 0.05f * f, 0.f, 4.f); Place(p, 0.05f * (f - 1u), 0.f, 4.f);
    }});
    run(200);
    const auto s = snapshot();
    const auto& m = s.motion;
    const auto d = detail();
    std::printf("moving: first %llu repeat %llu moved %llu draws %llu all %llu meshes %u admitted %zu samples %zu\n",
                (unsigned long long)m.first, (unsigned long long)m.repeat, (unsigned long long)m.moved,
                (unsigned long long)m.moving_draws, (unsigned long long)m.moving_draws_all, m.moved_meshes, s.admitted,
                d.moved_samples.size());
    CHECK(m.first >= 4u && m.repeat == 0u && m.moved == m.first, "all first, all moved");
    CHECK(m.first_meters[6] == m.first, "5 cm bucket <=0.1");
    CHECK(m.moving_draws == m.moved && m.moving_draws_all == m.moving_draws && m.moving_draws_indirect == 0u, "draw counts");
    CHECK(m.moved_camera == m.moved, "camera view (rigid camera VS)");
    CHECK(m.moved_meshes == 1u && d.moved_mesh_keys.size() == 1u, "one moving mesh");
    CHECK(s.admitted == 0u, "never admitted while moving");
    CHECK(d.moved_samples.size() == bvh::kPoolMotionSamplesPerFamily, "samples capped per family (%zu)", d.moved_samples.size());
    if (!d.moved_samples.empty()) {
      const auto& sample = d.moved_samples[0];
      CHECK(!sample.repeat && !sample.admitted && sample.draw_instances == 1u && sample.draw_moved == 1u
                && std::fabs(sample.meters - 0.05f) < 1e-3f && sample.vs_hash == 0x1000u && !sample.indirect,
            "sample fields (meters %g)", sample.meters);
      CHECK(sample.prev_world[3] == sample.world[3] - 0.05f * 1.f || std::fabs(sample.prev_world[3] - (sample.world[3] - 0.05f)) < 1e-4f,
            "sample matrices");
    }
    const auto f = family(0x1000u);
    CHECK(f.motion_moved == m.moved && std::fabs(f.motion_max_moved_meters - 0.05f) < 1e-3f && f.motion_moved_samples == 2u,
          "family moved");
  }

  HStage("4. Ghosts: moves, stops (admitted there), moves, stops elsewhere: two");
  // 4. Ghosts: moves, stops (admitted there), moves, stops elsewhere: two
  // admitted instances of one mesh; the dump lists them.
  {
    fresh();
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(1.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      auto x = [](uint32_t t) { return t < 60u ? 0.1f * t : t < 200u ? 6.f : t < 260u ? 6.f + 0.1f * (t - 200u) : 12.f; };
      Place(w, x(f)); Place(p, x(f == 0u ? 0u : f - 1u));
    }});
    run(400);
    const auto s = snapshot();
    const auto& m = s.motion;
    std::printf("ghosts: moved %llu repeat %llu admitted %zu meshes %u\n", (unsigned long long)m.moved,
                (unsigned long long)m.repeat, s.admitted, m.moved_meshes);
    CHECK(m.moved > 0u && m.repeat > 0u && m.moved_repeat == 0u, "moved and stopped");
    CHECK(s.admitted == 2u, "admitted at both stops (%zu)", s.admitted);
    const std::string text = read_dump();
    CHECK(text.find("\"schema\": 15") != std::string::npos, "schema 15");
    CHECK(text.find("\"admitted_of_moved_meshes\": 2") != std::string::npos, "ghost count in dump");
    CHECK(text.find("\"vs_hash\": \"0x00001000\", \"admitted\": 2}") != std::string::npos
              || text.find("\"admitted\": 2}") != std::string::npos, "moved mesh listed with 2 admitted");
    std::ofstream("motion_dump.json") << text;
  }

  HStage("5. Mixed instanced draw: element 1 of 3 moves; draw counted as moving");
  // 5. Mixed instanced draw: element 1 of 3 moves; draw counted as moving
  // but not all-moving; the sample says 1 of 3.
  {
    fresh();
    objects.push_back({make_vb(1.f).handle, 3, [](uint32_t f, uint32_t i, float* w, float* p) {
      if (i == 1u) { Place(w, 20.f + 0.02f * f); Place(p, 20.f + 0.02f * (f - 1u)); }
      else { Place(w, 30.f + i); Place(p, 30.f + i); }
    }});
    run(150);
    const auto s = snapshot();
    const auto& m = s.motion;
    const auto d = detail();
    CHECK(m.moving_draws >= 2u && m.moving_draws_all == 0u && m.moving_draw_instances == 3u * m.moving_draws,
          "mixed draws (%llu, all %llu, inst %llu)", (unsigned long long)m.moving_draws,
          (unsigned long long)m.moving_draws_all, (unsigned long long)m.moving_draw_instances);
    CHECK(!d.moved_samples.empty() && d.moved_samples[0].draw_instances == 3u && d.moved_samples[0].draw_moved == 1u
              && d.moved_samples[0].element == 1u, "sample 1 of 3, element 1");
    CHECK(s.admitted == 2u, "static siblings admitted (%zu)", s.admitted);
  }

  HStage("6. Rotation in place, a non-finite prevWorld, and a static object whose");
  // 6. Rotation in place, a non-finite prevWorld, and a static object whose
  // prevWorld is far off (contradiction: repeat sample, moved_repeat).
  {
    fresh();
    objects.push_back({make_vb(1.f).handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      Place(w, 40.f, 0.f, 0.f, 0.01f * f); Place(p, 40.f, 0.f, 0.f, 0.01f * (f - 1u));
    }});
    objects.push_back({make_vb(1.5f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) {
      Place(w, 50.f); Place(p, 50.f); p[0] = NAN;
    }});
    objects.push_back({make_vb(2.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) {
      Place(w, 60.f); Place(p, 61.f);
    }});
    run(150);
    const auto s = snapshot();
    const auto& m = s.motion;
    const auto d = detail();
    const auto rotating = family(0x1000u), nonfinite = family(0x1001u), contradiction = family(0x1002u);
    std::printf("mixed: moved %llu nonfinite %llu moved_repeat %llu repeat samples %zu\n", (unsigned long long)m.moved,
                (unsigned long long)m.nonfinite, (unsigned long long)m.moved_repeat, d.repeat_samples.size());
    CHECK(rotating.motion_moved == rotating.motion_first && rotating.motion_moved > 0u && rotating.motion_max_moved_meters == 0.f,
          "rotation counts as moved");
    CHECK(nonfinite.motion_moved == 0u && m.nonfinite == nonfinite.motion_first + nonfinite.motion_repeat, "non-finite prev");
    CHECK(nonfinite.motion_max_repeat_ulps == UINT32_MAX, "non-finite in last ulp bucket");
    CHECK(contradiction.motion_repeat > 0u && m.moved_repeat == contradiction.motion_repeat, "moved although repeated");
    bool found = false;
    for (const auto* list : {&d.repeat_samples, &d.rule_stale_samples}) {
      for (const auto& sample : *list) {
        if (sample.vs_hash == 0x1002u) found = found || (sample.repeat && sample.observed_frames >= 1u && std::fabs(sample.meters - 1.f) < 1e-4f);
      }
    }
    CHECK(m.rule_stale == contradiction.motion_repeat, "the contradiction is stale on every repeat");
    CHECK(found, "contradiction sampled");
    const std::string text = read_dump();
    CHECK(JsonError(text).empty(), "world_pool.json is valid JSON: %s", JsonError(text).c_str());
    CHECK(!HasNonFiniteToken(text), "world_pool.json has no nan or inf token");
    std::ofstream("motion_dump.json") << text;
  }


  // ---- P2a: moving rigid objects ----
  bvh::g_pool.exclude_moving.store(true);
  // P2b: the arena the draws copy into (EnsureDynamicExpand creates it in the live stage, not run here).
  resource dynamic_arena = {0u};
  dev.create_resource(resource_desc(bvh::kPoolDynamicArenaBytes, memory_heap::gpu_only, resource_usage::shader_resource | resource_usage::copy_dest),
                      nullptr, resource_usage::general, &dynamic_arena);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.dynamic_arena = dynamic_arena;
  }

  HStage("8. The rule.");
  // 8. The rule.
  {
    float w[12], p[12];
    Place(w, 3.f); std::memset(p, 0, sizeof(p));
    CHECK(!bvh::PoolSightingMoves(w, p), "zero prevWorld: unknown, not moving");
    float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    CHECK(!bvh::PoolPrevWorldFilled(identity) && !bvh::PoolSightingMoves(w, identity), "identity prevWorld: not filled, not moving");
    Place(p, 3.f);
    CHECK(!bvh::PoolSightingMoves(w, p), "equal");
    p[1] = 3e-8f;
    CHECK(!bvh::PoolSightingMoves(w, p), "near-zero basis noise");
    Place(w, 1000.f); Place(p, 1000.f); p[3] = std::nextafter(1000.f, 2000.f);
    CHECK(!bvh::PoolSightingMoves(w, p), "1 ULP at 1000 m");
    Place(w, 1.f); Place(p, 1.f - 2e-5f);
    CHECK(bvh::PoolSightingMoves(w, p), "2e-5 m");
    Place(w, 1.f); Place(p, 1.f - 5e-6f);
    CHECK(!bvh::PoolSightingMoves(w, p), "5e-6 m");
    Place(w, 1.f, 0.f, 0.f, 2e-5f); Place(p, 1.f);
    CHECK(bvh::PoolSightingMoves(w, p), "rotation 2e-5 rad");
    Place(w, 1.f); Place(p, 1.f);
    for (int i : {0, 5, 10}) { w[i] = 100.f; p[i] = 100.f; }
    p[1] = 5e-4f;
    CHECK(!bvh::PoolSightingMoves(w, p), "basis noise relative to scale 100");
    p[1] = 2e-3f;
    CHECK(bvh::PoolSightingMoves(w, p), "basis change at scale 100");
    Place(w, 1.f); Place(p, 2.f); p[0] = NAN;
    CHECK(!bvh::PoolSightingMoves(w, p), "non-finite prevWorld");
  }

  HStage("P2b. Expansion math: transcribed record vs CheckPoolWorld, ComputeMatrixInverse, brute force bounds.");
  {
    // Transcription of world_dynamic_expand.cs_5_0.hlsl (one record): validity, closed-form inverse, bounds.
    struct Record {
      bool valid = false;
      float inverse[12] = {};
      float lo[3] = {};
      float hi[3] = {};
    };
    auto expand = [](const float* m, float min_scale, float max_scale, const float* bmin, const float* bmax) {
      Record e;
      for (int i = 0; i < 12; ++i) {
        if (!std::isfinite(m[i])) return e;
      }
      const float t[3] = {m[3], m[7], m[11]};
      for (int k = 0; k < 3; ++k) {
        if (!(std::fabs(t[k]) <= bvh::kPoolMaxPosition)) return e;
      }
      float len[3];
      for (int k = 0; k < 3; ++k) {
        len[k] = std::sqrt(m[4 * k] * m[4 * k] + m[4 * k + 1] * m[4 * k + 1] + m[4 * k + 2] * m[4 * k + 2]);
        if (!(len[k] >= 1e-6f) || !(len[k] >= min_scale) || !(len[k] < max_scale)) return e;
      }
      for (int a = 0; a < 3; ++a) {
        for (int b = a + 1; b < 3; ++b) {
          const float d = (m[4 * a] * m[4 * b] + m[4 * a + 1] * m[4 * b + 1] + m[4 * a + 2] * m[4 * b + 2]) / (len[a] * len[b]);
          if (!(std::fabs(d) < 0.995f)) return e;
        }
      }
      const float a0[3] = {m[0], m[1], m[2]}, a1[3] = {m[4], m[5], m[6]}, a2[3] = {m[8], m[9], m[10]};
      const float c0[3] = {a1[1] * a2[2] - a1[2] * a2[1], a1[2] * a2[0] - a1[0] * a2[2], a1[0] * a2[1] - a1[1] * a2[0]};
      const float c1[3] = {a2[1] * a0[2] - a2[2] * a0[1], a2[2] * a0[0] - a2[0] * a0[2], a2[0] * a0[1] - a2[1] * a0[0]};
      const float c2[3] = {a0[1] * a1[2] - a0[2] * a1[1], a0[2] * a1[0] - a0[0] * a1[2], a0[0] * a1[1] - a0[1] * a1[0]};
      const float det = a0[0] * c0[0] + a0[1] * c0[1] + a0[2] * c0[2];
      if (!(std::fabs(det) >= 1e-12f)) return e;
      for (int k = 0; k < 3; ++k) {
        for (int j = 0; j < 3; ++j) {
          const float c = j == 0 ? c0[k] : j == 1 ? c1[k] : c2[k];
          e.inverse[4 * k + j] = c / det;
        }
        e.inverse[4 * k + 3] = -(e.inverse[4 * k] * t[0] + e.inverse[4 * k + 1] * t[1] + e.inverse[4 * k + 2] * t[2]);
      }
      for (int k = 0; k < 3; ++k) {
        e.lo[k] = t[k];
        e.hi[k] = t[k];
        for (int j = 0; j < 3; ++j) {
          const float p = m[4 * k + j] * bmin[j], q = m[4 * k + j] * bmax[j];
          e.lo[k] += std::min(p, q);
          e.hi[k] += std::max(p, q);
        }
      }
      e.valid = true;
      return e;
    };
    uint32_t rng = 12345u;
    auto next = [&rng]() {
      rng = rng * 1664525u + 1013904223u;
      return static_cast<float>(rng >> 8) / static_cast<float>(1u << 24);
    };
    const float bmin[3] = {-1.f, -2.f, -0.5f}, bmax[3] = {1.f, 2.f, 0.5f};
    uint32_t total = 0u, valid_count = 0u, validity_mismatch = 0u, inverse_bad = 0u, bounds_bad = 0u;
    float worst_inverse = 0.f;
    for (uint32_t n = 0u; n < 20000u; ++n) {
      float m[12];
      for (float& v : m) v = (next() * 2.f - 1.f) * 3.f;
      for (int k = 0; k < 3; ++k) m[4 * k + 3] = (next() * 2.f - 1.f) * 100.f;
      const uint32_t kind = n % 10u;
      if (kind == 1u) m[4] = m[5] = m[6] = 0.f;                 // zero row
      if (kind == 2u) m[0] = NAN;                               // non-finite
      if (kind == 3u) m[11] = 60000.f;                          // far away
      if (kind == 4u) { for (int j = 0; j < 4; ++j) m[4 + j] = 2.f * m[j]; }  // collinear rows
      if (kind == 5u) { for (int j = 0; j < 3; ++j) m[j] *= 1e-6f; }          // tiny scale
      if (kind == 6u) { for (int j = 0; j < 3; ++j) m[8 + j] *= 1e4f; }       // large scale
      if (kind == 7u) { m[0] = 1.f; m[1] = m[2] = m[4] = m[6] = m[8] = m[9] = 0.f; m[5] = m[10] = 1.f; }  // identity basis
      const Record e = expand(m, bvh::kPoolMinScale, bvh::kPoolMaxScale, bmin, bmax);
      const bool cpu_valid = bvh::CheckPoolWorld(m) == bvh::PoolMatrixReject::None;
      total += 1u;
      if (e.valid) valid_count += 1u;
      if (e.valid != cpu_valid) validity_mismatch += 1u;
      if (!cpu_valid) continue;
      float inv16[16] = {};
      falcom_world::ComputeMatrixInverse(m, inv16);
      for (int k = 0; k < 3; ++k) {
        for (int j = 0; j < 4; ++j) {
          const float diff = std::fabs(e.inverse[4 * k + j] - inv16[4 * k + j]);
          worst_inverse = std::max(worst_inverse, diff / std::max(1.f, std::fabs(inv16[4 * k + j])));
          if (diff > 1e-3f * std::max(1.f, std::fabs(inv16[4 * k + j]))) inverse_bad += 1u;
        }
      }
      // Brute force: the eight corners of the local box, transformed.
      for (int k = 0; k < 3; ++k) {
        float lo = 1e30f, hi = -1e30f;
        for (int corner = 0; corner < 8; ++corner) {
          const float local[3] = {(corner & 1) ? bmax[0] : bmin[0], (corner & 2) ? bmax[1] : bmin[1], (corner & 4) ? bmax[2] : bmin[2]};
          const float p = m[4 * k] * local[0] + m[4 * k + 1] * local[1] + m[4 * k + 2] * local[2] + m[4 * k + 3];
          lo = std::min(lo, p);
          hi = std::max(hi, p);
        }
        if (std::fabs(lo - e.lo[k]) > 1e-5f * std::max(1.f, std::fabs(lo)) || std::fabs(hi - e.hi[k]) > 1e-5f * std::max(1.f, std::fabs(hi))) bounds_bad += 1u;
      }
    }
    std::printf("p2b expand: %u matrices, %u valid, validity mismatches %u, inverse mismatches %u (worst rel %.2e), bounds mismatches %u\n",
                total, valid_count, validity_mismatch, inverse_bad, static_cast<double>(worst_inverse), bounds_bad);
    CHECK(validity_mismatch == 0u, "P2b: validity equals CheckPoolWorld (%u)", validity_mismatch);
    CHECK(inverse_bad == 0u && valid_count > 1000u, "P2b: inverse equals ComputeMatrixInverse (%u bad, %u valid)", inverse_bad, valid_count);
    CHECK(bounds_bad == 0u, "P2b: bounds equal the brute-force corners (%u)", bounds_bad);
  }

  HStage("9. Static, then moves, then stops: the static instance is removed when it");
  // 9. Static, then moves, then stops: the static instance is removed when it
  // is seen moving and no pose is admitted afterwards; a static neighbor is
  // unaffected; the dump lists the moving key with its evidence.
  {
    fresh();
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(1.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      auto x = [](uint32_t t) { return t < 150u ? 0.f : t < 210u ? 0.1f * (t - 150u) : 6.f; };
      Place(w, x(f)); Place(p, x(f == 0u ? 0u : f - 1u));
    }});
    objects.push_back({make_vb(2.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) { Place(w, 50.f); Place(p, 50.f); }});
    run(140);
    auto s = snapshot();
    CHECK(s.admitted == 2u && s.dynamic_live_keys == 0u, "both static at first (%zu)", s.admitted);
    run(80);
    s = snapshot();
    std::printf("p2a moving: admitted %zu keys %zu moving %zu meshes %zu retired %u blocked %llu rule %llu stale %llu\n", s.admitted,
                s.dynamic_live_keys, s.dynamic_moving_keys, s.dynamic_meshes, s.dynamic_retired, (unsigned long long)s.dynamic_blocked,
                (unsigned long long)s.motion.rule_moving, (unsigned long long)s.motion.rule_stale);
    CHECK(s.dynamic_live_keys == 1u && s.dynamic_meshes == 1u && s.dynamic_retired == 1u, "flagged, static pose removed");
    CHECK(s.admitted == 1u, "neighbor stays (%zu)", s.admitted);
    CHECK(s.dynamic_moving_keys == 1u && s.dynamic_released == 0u, "still moving within the hold");
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      const uint64_t flagged = bvh::g_pool.dynamic_keys.begin()->first;
      CHECK(dynamic_frame.size() == 1u, "P2b: one record for the flagged mesh per present (%zu)", dynamic_frame.size());
      if (dynamic_frame.size() == 1u) {
        const bvh::PoolDynamicDraw& d = dynamic_frame[0];
        CHECK(d.mesh_key == flagged && d.vs_hash == 0x1000u && !d.indirect && d.count == 1u, "P2b record: key, VS, direct, count");
        CHECK(d.pass != 0u, "P2b record: gate pass");
        CHECK(d.buffer.handle == dynamic_arena.handle && d.arena == 0u && d.out == 0u, "P2b record: arena and record offsets");
        const auto& arena_bytes = dev.res[dynamic_arena.handle].bytes;
        const auto& ring_bytes = dev.res[t15.handle].bytes;
        CHECK(std::memcmp(arena_bytes.data() + d.arena, ring_bytes.data() + static_cast<size_t>(d.base) * 160u, d.count * 160u) == 0,
              "P2b copy: arena holds the drawn slice");
      }
    }
    run(200);  // stopped at x = 6 since frame 210: released after the hold, the pose is admitted
    s = snapshot();
    CHECK(s.dynamic_released == 1u && s.dynamic_meshes == 0u && s.dynamic_moving_keys == 0u,
          "released after the hold (released %u, flagged %zu, moving %zu)", s.dynamic_released, s.dynamic_meshes, s.dynamic_moving_keys);
    CHECK(s.admitted == 2u, "stopped pose admitted after release (%zu)", s.admitted);
    CHECK(dynamic_frame.empty(), "P2b: no record once released (%zu)", dynamic_frame.size());
    CHECK(s.motion.rule_moving > 0u && s.motion.rule_stale == 0u, "rule never fires stale on unmoved sightings");
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (const auto& instance : bvh::g_pool.instances) CHECK(instance.matrix[3] == 50.f || instance.matrix[3] == 6.f, "neighbor and stopped pose");
      for (const auto& [key, observed] : bvh::g_pool.observations) {
        const auto m = bvh::g_pool.mesh_by_key.find(key.mesh_key);
        if (m != bvh::g_pool.mesh_by_key.end() && bvh::g_pool.meshes[m->second].dynamic) CHECK(!observed.admitted, "no admitted observation of a dynamic mesh");
      }
    }
    const std::string text = read_dump();
    CHECK(text.find("\"exclude_moving\": true") != std::string::npos, "switch in dump");
    CHECK(text.find("\"p2b\": {\"per_frame\": true") != std::string::npos, "p2b block with the switch");
    CHECK(text.find("\"copied_total\": 0,") == std::string::npos && text.find("\"refused_total\": 0, \"deferred_total\": 0,") != std::string::npos,
          "p2b block: copies recorded, none refused or deferred");
    CHECK(text.find("\"dynamic\": {\"keys\": 1") != std::string::npos, "dynamic section");
    CHECK(text.find("\"retired_instances\": 1") != std::string::npos && text.find("\"mesh_flagged\": false") != std::string::npos,
          "entry evidence");
    CHECK(text.find("\"moving_keys\": 0, \"released\": 1") != std::string::npos && text.find("\"moving_now\": false, \"released\": 1") != std::string::npos,
          "release evidence");
    std::ofstream("motion_dump.json") << text;
  }

  HStage("10. Switch off: the stopped object's pose admits again at its next");
  // 10. Switch off: the stopped object's pose admits again at its next
  // sighting; on again: removed again.
  {
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(false);
    run(80);
    auto s = snapshot();
    CHECK(s.admitted == 2u && s.dynamic_meshes == 0u, "off: re-admitted (%zu, flagged %zu)", s.admitted, s.dynamic_meshes);
    bvh::g_pool.exclude_moving.store(true);
    run(1);
    s = snapshot();
    CHECK(s.admitted == 2u && s.dynamic_meshes == 0u, "on again: released key not re-flagged (%zu)", s.admitted);
  }

  HStage("P2b alpha: a flagged alpha-tested mesh is not recorded (counted in dynamic_alpha_skipped).");
  {
    fresh();
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(1.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      auto x = [](uint32_t t) { return t < 150u ? 0.f : t < 210u ? 0.1f * (t - 150u) : 6.f; };
      Place(w, x(f)); Place(p, x(f == 0u ? 0u : f - 1u));
    }});
    run(220);
    uint64_t skipped_before = 0u;
    uint64_t arena_before = 0u;
    uint64_t out_before = 0u;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      CHECK(bvh::g_pool.dynamic_keys.size() == 1u, "alpha stage: the object is flagged moving (%zu)", bvh::g_pool.dynamic_keys.size());
      const uint64_t key = bvh::g_pool.dynamic_keys.begin()->first;
      bvh::MarkPoolMeshAlpha(bvh::g_pool.mesh_by_key.at(key), bvh::g_pool.alpha_keys[key]);
      skipped_before = bvh::g_pool.stats.dynamic_alpha_skipped;
      arena_before = bvh::g_pool.dynamic_arena_used;
      out_before = bvh::g_pool.dynamic_out_used;
    }
    run(1);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      CHECK(dynamic_frame.empty(), "P2b alpha: no record for the alpha-tested mesh (%zu)", dynamic_frame.size());
      CHECK(bvh::g_pool.dynamic_arena_used == arena_before && bvh::g_pool.dynamic_out_used == out_before,
            "P2b alpha: arena not reserved (%llu, %llu)", (unsigned long long)bvh::g_pool.dynamic_arena_used,
            (unsigned long long)bvh::g_pool.dynamic_out_used);
      CHECK(bvh::g_pool.stats.dynamic_alpha_skipped == skipped_before + 1u, "P2b alpha: counted once (%llu)",
            (unsigned long long)bvh::g_pool.stats.dynamic_alpha_skipped);
    }
    bvh::g_pool.dynamic_per_frame.store(false);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      skipped_before = bvh::g_pool.stats.dynamic_alpha_skipped;
    }
    run(1);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      CHECK(bvh::g_pool.stats.dynamic_alpha_skipped == skipped_before, "P2b alpha: switch off, not counted (%llu)",
            (unsigned long long)bvh::g_pool.stats.dynamic_alpha_skipped);
    }
    bvh::g_pool.dynamic_per_frame.store(true);
  }

  HStage("11. Same mesh content through another draw key: the static copy goes");
  // 11. Same mesh content through another draw key: the static copy goes
  // with it (mesh-level flag); a light-view draw of a static object with zero
  // prevWorld flags nothing.
  {
    fresh();
    const resource shared = make_vb(1.f);
    objects.push_back({shared.handle, 1, [](uint32_t, uint32_t, float* w, float* p) { Place(w, 70.f); Place(p, 70.f); }});
    objects.push_back({make_vb(3.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) {
      Place(w, 80.f); std::memset(p, 0, 48);
    }, kLightVs});
    run(140);
    auto s = snapshot();
    CHECK(s.admitted == 2u && s.dynamic_live_keys == 0u, "static + light-view static admitted, nothing flagged (%zu, %zu)",
          s.admitted, s.dynamic_live_keys);
    CHECK(s.motion.light_zero_prev > 0u, "zero prev counted for the light view");
    objects.push_back({shared.handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      Place(w, 90.f + 0.01f * f); Place(p, 90.f + 0.01f * (f - 1u));
    }, 0u, 72u});  // own draw key (first index 72), same content as object 0
    run(120);
    s = snapshot();
    std::printf("shared: admitted %zu keys %zu meshes %zu retired %u\n", s.admitted, s.dynamic_live_keys, s.dynamic_meshes,
                s.dynamic_retired);
    CHECK(s.dynamic_live_keys == 1u && s.dynamic_meshes == 1u, "one moving key, its shared mesh flagged");
    CHECK(s.admitted == 1u && s.dynamic_retired == 1u, "static copy of the same mesh removed; light-view object stays");
  }

  HStage("12. Seen moving before its mesh is captured: flagged at capture, never");
  // 12. Seen moving before its mesh is captured: flagged at capture, never
  // admitted.
  {
    fresh();
    objects.push_back({make_vb(4.f).handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      Place(w, 0.02f * f, 1.f); Place(p, 0.02f * (f - 1u), 1.f);
    }});
    run(150);
    const auto s = snapshot();
    CHECK(s.meshes == 1u && s.dynamic_meshes == 1u && s.admitted == 0u && s.dynamic_retired == 0u,
          "flagged at capture (meshes %zu flagged %zu admitted %zu)", s.meshes, s.dynamic_meshes, s.admitted);
  }

  HStage("13. The buffer is released: its key leaves the dynamic set with it.");
  // 13. The buffer is released: its key leaves the dynamic set with it.
  {
    std::vector<uint64_t> vbs;
    for (const auto& object : objects) vbs.push_back(object.vb);
    objects.clear();
    for (uint64_t vb : vbs) bvh::OnDestroyResourcePool(&dev, resource{vb});
    run(2);
    const auto s = snapshot();
    CHECK(s.dynamic_live_keys == 0u && s.meshes == 0u, "released (keys %zu meshes %zu)", s.dynamic_live_keys, s.meshes);
  }

  HStage("14. An unmoved object whose prevWorld is off (the M1 contradiction case)");
  // 14. An unmoved object whose prevWorld is off (the M1 contradiction case)
  // is flagged too, and counted and sampled as a rule failure.
  {
    fresh();
    objects.push_back({make_vb(5.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) { Place(w, 60.f); Place(p, 61.f); }});
    run(150);
    const auto s = snapshot();
    const auto d = detail();
    CHECK(s.motion.rule_stale > 0u && !d.rule_stale_samples.empty() && d.rule_stale_samples[0].repeat,
          "stale rule sightings counted and sampled (%llu)", (unsigned long long)s.motion.rule_stale);
  }

  HStage("15. Identity prevWorld (not filled) never moves an object.");
  // 15. Identity prevWorld (not filled) never moves an object.
  {
    fresh();
    objects.push_back({make_vb(6.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) {
      Place(w, 20.f); std::memset(p, 0, 48); p[0] = p[5] = p[10] = 1.f;
    }});
    run(150);
    const auto s = snapshot();
    CHECK(s.admitted == 1u && s.dynamic_live_keys == 0u && s.motion.rule_moving == 0u,
          "identity prevWorld: static over 150 frames (admitted %zu, keys %zu)", s.admitted, s.dynamic_live_keys);
  }

  HStage("16. Two instances under one draw key: the moving one flags the key's mesh;");
  // 16. Two instances under one draw key: the moving one flags the key's mesh;
  // the static one is not admitted with it.
  {
    fresh();
    objects.push_back({make_vb(7.f).handle, 2, [](uint32_t f, uint32_t i, float* w, float* p) {
      if (i == 0u) { Place(w, 30.f); Place(p, 30.f); return; }
      Place(w, 40.f + 0.01f * f); Place(p, 40.f + 0.01f * (f == 0u ? 0u : f - 1u));
    }});
    run(150);
    const auto s = snapshot();
    CHECK(s.dynamic_live_keys == 1u && s.dynamic_meshes == 1u && s.admitted == 0u && s.dynamic_blocked > 0u,
          "one key, one mesh flagged, nothing admitted (keys %zu, flagged %zu, admitted %zu, blocked %llu)", s.dynamic_live_keys,
          s.dynamic_meshes, s.admitted, (unsigned long long)s.dynamic_blocked);
  }

  HStage("17. Two keys share a mesh: one stops and is released; the other still");
  // 17. Two keys share a mesh: one stops and is released; the other still
  // moves, so the shared mesh stays flagged.
  {
    fresh();
    const uint32_t start = falcom_world::g_state.frame.load();
    const resource shared = make_vb(8.f);
    objects.push_back({shared.handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      Place(w, 100.f + 0.01f * f); Place(p, 100.f + 0.01f * (f == 0u ? 0u : f - 1u));
    }});
    objects.push_back({shared.handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      auto x = [](uint32_t t) { return t < 60u ? 200.f + 0.1f * t : 206.f; };
      Place(w, x(f)); Place(p, x(f == 0u ? 0u : f - 1u));
    }, 0u, 72u});
    run(200);
    const auto s = snapshot();
    CHECK(s.dynamic_live_keys == 2u && s.dynamic_moving_keys == 1u && s.dynamic_meshes == 1u && s.dynamic_released == 0u,
          "shared mesh held by the moving key (keys %zu, moving %zu, flagged %zu, released %u)", s.dynamic_live_keys,
          s.dynamic_moving_keys, s.dynamic_meshes, s.dynamic_released);
  }

  HStage("18. Release, then moving again: the key re-arms and is flagged again; a");
  // 18. Release, then moving again: the key re-arms and is flagged again; a
  // second stop releases it again. ApplyPoolDynamicKey does not flag a
  // released key.
  {
    fresh();
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(9.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      auto x = [](uint32_t t) {
        return t < 150u ? 10.f : t < 210u ? 10.f + 0.1f * (t - 150u) : t < 330u ? 16.f : t < 390u ? 16.f + 0.1f * (t - 330u) : 22.f;
      };
      Place(w, x(f)); Place(p, x(f == 0u ? 0u : f - 1u));
    }});
    run(140);
    run(80);  // frame 220: moving, not released yet
    auto s = snapshot();
    CHECK(s.dynamic_moving_keys == 1u && s.dynamic_released == 0u, "frame 220: moving, not released");
    run(160);  // frame 380: released at the first still sample after the hold, moving again
    s = snapshot();
    CHECK(s.dynamic_moving_keys == 1u && s.dynamic_meshes == 1u && s.dynamic_released == 1u,
          "re-armed: moving %zu flagged %zu released %u", s.dynamic_moving_keys, s.dynamic_meshes, s.dynamic_released);
    run(120);  // frame 500: stopped at frame 390, released before: still inside the 300-frame hot hold
    s = snapshot();
    CHECK(s.dynamic_moving_keys == 1u && s.dynamic_meshes == 1u && s.dynamic_released == 1u,
          "held after the second stop (moving %zu flagged %zu released %u)", s.dynamic_moving_keys, s.dynamic_meshes, s.dynamic_released);
    run(200);  // frame 700: released after the hot hold
    s = snapshot();
    CHECK(s.dynamic_moving_keys == 0u && s.dynamic_meshes == 0u && s.dynamic_released == 2u,
          "second release (moving %zu flagged %zu released %u)", s.dynamic_moving_keys, s.dynamic_meshes, s.dynamic_released);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (auto& [key, entry] : bvh::g_pool.dynamic_keys) {
        CHECK(!entry.moving_now, "released entry");
        bvh::ApplyPoolDynamicKey(key, entry);
      }
      for (const auto& mesh : bvh::g_pool.meshes) CHECK(!mesh.dynamic, "ApplyPoolDynamicKey does not flag a released key");
    }
    const std::string text = read_dump();
    CHECK(text.find("\"schema\": 15") != std::string::npos, "schema 15");
    CHECK(text.find("\"rule_stale\": ") != std::string::npos && text.find("\"rule_stale_samples\": ") != std::string::npos,
          "rule_stale fields");
    CHECK(text.find("\"moving_keys\": 0, \"released\": 2") != std::string::npos
              && text.find("\"moving_now\": false, \"released\": 2") != std::string::npos,
          "dump: release counters");
    std::ofstream("motion_dump.json") << text;
  }

  HStage("19. The moving object's buffer is released while its key is still");
  // 19. The moving object's buffer is released while its key is still
  // flagged: the mesh is unflagged at compaction (the static key still holds
  // it), and the static pose admits again.
  {
    fresh();
    const resource vb_static = make_vb(8.f), vb_moving = make_vb(8.f);
    objects.push_back({vb_static.handle, 1, [](uint32_t, uint32_t, float* w, float* p) { Place(w, 40.f); Place(p, 40.f); }, 0u, 0u});
    objects.push_back({vb_moving.handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      Place(w, 300.f + 0.01f * f); Place(p, 300.f + 0.01f * (f == 0u ? 0u : f - 1u));
    }, 0u, 36u});
    run(150);
    auto s = snapshot();
    CHECK(s.meshes == 1u && s.dynamic_live_keys == 1u && s.dynamic_moving_keys == 1u && s.dynamic_meshes == 1u && s.admitted == 0u,
          "moving and flagged (meshes %zu, keys %zu, moving %zu, flagged %zu, admitted %zu)", s.meshes, s.dynamic_live_keys,
          s.dynamic_moving_keys, s.dynamic_meshes, s.admitted);
    bvh::OnDestroyResourcePool(&dev, vb_moving);
    objects.pop_back();
    run(1);
    s = snapshot();
    CHECK(s.dynamic_live_keys == 0u && s.dynamic_moving_keys == 0u && s.dynamic_meshes == 0u && s.meshes == 1u,
          "released key unflags the shared mesh (keys %zu, moving %zu, flagged %zu, meshes %zu)", s.dynamic_live_keys,
          s.dynamic_moving_keys, s.dynamic_meshes, s.meshes);
    const uint64_t blocked = s.dynamic_blocked;
    run(70);
    s = snapshot();
    CHECK(s.admitted == 1u && s.dynamic_meshes == 0u && s.dynamic_blocked == blocked, "static admitted again (admitted %zu, flagged %zu)",
          s.admitted, s.dynamic_meshes);
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.instances.size() == 1u && bvh::g_pool.instances[0].matrix[3] == 40.f, "only the static pose (%zu)",
          bvh::g_pool.instances.size());
  }

  HStage("20. Two moving keys share a mesh: releasing one keeps the mesh flagged for");
  // 20. Two moving keys share a mesh: releasing one keeps the mesh flagged for
  // the other; releasing the last unflags it.
  {
    fresh();
    const resource vb_static = make_vb(8.f), vb_a = make_vb(8.f), vb_b = make_vb(8.f);
    objects.push_back({vb_static.handle, 1, [](uint32_t, uint32_t, float* w, float* p) { Place(w, 40.f); Place(p, 40.f); }, 0u, 0u});
    objects.push_back({vb_a.handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      Place(w, 300.f + 0.01f * f); Place(p, 300.f + 0.01f * (f == 0u ? 0u : f - 1u));
    }, 0u, 36u});
    objects.push_back({vb_b.handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      Place(w, 400.f + 0.01f * f); Place(p, 400.f + 0.01f * (f == 0u ? 0u : f - 1u));
    }, 0u, 72u});
    run(150);
    auto s = snapshot();
    CHECK(s.meshes == 1u && s.dynamic_live_keys == 2u && s.dynamic_moving_keys == 2u && s.dynamic_meshes == 1u && s.admitted == 0u,
          "two moving keys, one flagged mesh (meshes %zu, keys %zu, moving %zu, flagged %zu, admitted %zu)", s.meshes,
          s.dynamic_live_keys, s.dynamic_moving_keys, s.dynamic_meshes, s.admitted);
    bvh::OnDestroyResourcePool(&dev, vb_a);
    objects.erase(objects.begin() + 1);
    run(1);
    s = snapshot();
    CHECK(s.dynamic_live_keys == 1u && s.dynamic_moving_keys == 1u && s.dynamic_meshes == 1u && s.admitted == 0u,
          "one key left, mesh still flagged (keys %zu, moving %zu, flagged %zu, admitted %zu)", s.dynamic_live_keys,
          s.dynamic_moving_keys, s.dynamic_meshes, s.admitted);
    bvh::OnDestroyResourcePool(&dev, vb_b);
    objects.pop_back();
    run(1);
    s = snapshot();
    CHECK(s.dynamic_live_keys == 0u && s.dynamic_meshes == 0u && s.meshes == 1u, "no moving key, mesh unflagged (keys %zu, flagged %zu, meshes %zu)",
          s.dynamic_live_keys, s.dynamic_meshes, s.meshes);
    run(70);
    s = snapshot();
    CHECK(s.admitted == 1u && s.dynamic_meshes == 0u, "static admitted (admitted %zu, flagged %zu)", s.admitted, s.dynamic_meshes);
  }

  HStage("20b. Door: moves A -> B, stops, releases; moves back to A within the hot hold;");
  // 20b. A door moves A -> B and stops; released after the hold. It moves back
  // to A within kPoolHotHoldFrames of that release and stops. Sightings are
  // taken once per kPoolRecaptureFrames per draw identity, so the re-flag and
  // the release come within one capture interval of their trigger. The door has
  // a TLAS representation (flagged record or admitted instance) on every frame
  // from the move's first flag on; the handover gap is counted and printed.
  // Idle past the hot hold: released, admitted at A.
  {
    fresh();
    const uint32_t start = falcom_world::g_state.frame.load();
    auto door_x = [](uint32_t t) {
      return t < 150u ? 1.f : t < 210u ? 1.f + 0.1f * (t - 150u) : t < 300u ? 7.f : t < 386u ? 7.f - 0.07f * (t - 300u) : 1.f;
    };
    objects.push_back({make_vb(1.f).handle, 1, [start, door_x](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      Place(w, door_x(f)); Place(p, door_x(f == 0u ? 0u : f - 1u));
    }});
    auto admitted_at = [&](float x) {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (const auto& instance : bvh::g_pool.instances) {
        if (instance.matrix[3] == x) return true;
      }
      return false;
    };
    auto represented = [&]() {
      std::lock_guard lock(bvh::g_pool.mutex);
      return !dynamic_frame.empty() || !bvh::g_pool.instances.empty();
    };
    auto rel = [&]() { return falcom_world::g_state.frame.load() - start; };
    auto step = [&]() {
      run(1);
      return represented();
    };
    run(140);
    CHECK(admitted_at(1.f), "door static at A before the move");
    uint32_t gaps = 0u, static_at_b_stop = 0u, move_lag = 0u, move_gaps = 0u;
    while (rel() < 290u) {  // move A -> B, stop at B, released after the hold
      const bool represented_now = step();
      if (rel() > 210u) {
        if (!represented_now) gaps += 1u;
        if (admitted_at(1.f)) static_at_b_stop += 1u;
      } else {
        if (!represented_now) move_gaps += 1u;
        if (admitted_at(1.f)) move_lag += 1u;
      }
    }
    auto s = snapshot();
    CHECK(s.dynamic_released == 1u && s.dynamic_meshes == 0u && admitted_at(7.f), "stopped at B and released (released %u, flagged %zu)",
          s.dynamic_released, s.dynamic_meshes);
    CHECK(static_at_b_stop == 0u, "no static instance at A during the stop at B (%u)", static_at_b_stop);
    std::printf("20b move A->B: %u frames with the old static pose at A before the flag, %u frames with no TLAS representation\n", move_lag, move_gaps);
    CHECK(gaps == 0u, "20b: the door has a representation every frame of the stop at B (%u gaps)", gaps);
    while (rel() < 301u) step();  // stopped at B, released
    uint32_t reflag = 0u;
    while (snapshot().dynamic_meshes == 0u && rel() < 360u) {  // move back: flagged on its first capture
      step();
      reflag += 1u;
    }
    s = snapshot();
    std::printf("20b re-flag after the move back: %u frames after the first moved frame\n", reflag);
    CHECK(s.dynamic_moving_keys == 1u && s.dynamic_meshes == 1u && reflag <= bvh::kPoolRecaptureFrames,
          "re-flagged within one capture interval (moving %zu, flagged %zu, lag %u)", s.dynamic_moving_keys, s.dynamic_meshes, reflag);
    uint32_t records_missing = 0u, gaps_back = 0u, static_at_a = 0u;
    while (rel() < 386u) {  // move B -> A, stop at A
      if (!step()) gaps_back += 1u;
      if (dynamic_frame.size() != 1u) records_missing += 1u;
      if (admitted_at(1.f)) static_at_a += 1u;
    }
    CHECK(records_missing == 0u, "a flagged record every frame from the flag to the stop at A (%u missing)", records_missing);
    CHECK(gaps_back == 0u && static_at_a == 0u, "no gap and no static at A from the flag to the stop (gaps %u, static %u)", gaps_back, static_at_a);
    uint32_t idle_gaps = 0u, first_admitted = 0u;
    while (rel() < 780u) {  // stopped at A: flagged until the hot hold passes, then released and admitted
      if (!step() && rel() > 386u) idle_gaps += 1u;
      if (first_admitted == 0u && admitted_at(1.f)) first_admitted = rel();
    }
    s = snapshot();
    uint32_t last_moving = 0u;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      last_moving = bvh::g_pool.dynamic_keys.begin()->second.last_frame - start;
    }
    std::printf("20b idle at A: last moving sighting %u, admitted at A at frame %u, %u frames with no TLAS representation\n",
                last_moving, first_admitted, idle_gaps);
    CHECK(first_admitted >= last_moving + 300u && first_admitted <= last_moving + 300u + bvh::kPoolRecaptureFrames,
          "20b: admitted at A one capture interval after the hot hold (admitted %u, last moving %u)", first_admitted, last_moving);
    CHECK(s.dynamic_released == 2u && s.dynamic_meshes == 0u && admitted_at(1.f), "second release, admitted at A (released %u, admitted %zu)",
          s.dynamic_released, s.admitted);
  }

  HStage("21. prevWorld trace: A (far flip), B (sway 2 cm per frame), C (static) are");
  // 21. prevWorld trace: A (far flip), B (sway 2 cm per frame), C (static) are
  // selected, each records 120 consecutive frames; the trace is read-only.
  {
    struct Scenario {
      bvh::PoolStats stats;
      bvh::PoolPrevTrace trace;
    };
    auto scenario = [&](bool armed) {
      fresh();
      const resource vb_a = make_vb(8.f), vb_b = make_vb(9.f), vb_c = make_vb(10.f);
      objects.push_back({vb_a.handle, 1, [](uint32_t, uint32_t, float* w, float* p) {
        Place(w, 5.f, 0.f, 0.f, 3.14159265f); Place(p, 55.f);
      }, 0u, 0u});
      objects.push_back({vb_b.handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
        const auto x = [](uint32_t g) { return 20.f + 0.02f * static_cast<float>(g); };
        Place(w, x(f)); Place(p, x(f == 0u ? 0u : f - 1u));
      }, 0u, 36u});
      objects.push_back({vb_c.handle, 1, [](uint32_t, uint32_t, float* w, float* p) { Place(w, 40.f); Place(p, 40.f); }, 0u, 72u});
      falcom_world::g_state.frame.store(1000u);
      if (armed) {
        std::lock_guard lock(bvh::g_pool.mutex);
        bvh::g_pool.prev_trace = {};
        bvh::g_pool.prev_trace.armed_frame = falcom_world::g_state.frame.load();
      }
      run(400);
      Scenario result;
      result.stats = snapshot();
      std::lock_guard lock(bvh::g_pool.mutex);
      result.trace = bvh::g_pool.prev_trace;
      return result;
    };
    const Scenario traced = scenario(true);
    const Scenario plain = scenario(false);

    // Read-only: the counters the trace must not move, armed or not.
    const auto& t = traced.stats;
    const auto& p = plain.stats;
    CHECK(t.copied_draws == p.copied_draws && t.instances_seen == p.instances_seen && t.admitted == p.admitted
              && t.base_verified == p.base_verified,
          "copied %llu/%llu seen %llu/%llu admitted %zu/%zu verified %llu/%llu", (unsigned long long)t.copied_draws,
          (unsigned long long)p.copied_draws, (unsigned long long)t.instances_seen, (unsigned long long)p.instances_seen,
          t.admitted, p.admitted, (unsigned long long)t.base_verified, (unsigned long long)p.base_verified);
    CHECK(t.motion.first == p.motion.first && t.motion.repeat == p.motion.repeat && t.motion.moved == p.motion.moved
              && t.motion.rule_moving == p.motion.rule_moving && t.motion.rule_stale == p.motion.rule_stale,
          "motion counters identical");
    CHECK(t.dynamic_live_keys == p.dynamic_live_keys && t.dynamic_moving_keys == p.dynamic_moving_keys
              && t.dynamic_meshes == p.dynamic_meshes && t.dynamic_retired == p.dynamic_retired
              && t.dynamic_blocked == p.dynamic_blocked && t.dynamic_released == p.dynamic_released,
          "dynamic counters identical");
    CHECK(plain.trace.armed_frame == 0u && plain.trace.records.empty(), "unarmed: no trace");

    // Selection: keys[0] = A, keys[1] = B, keys[2] = C, each Done with 120 frames.
    const auto& keys = traced.trace.keys;
    CHECK(traced.trace.written, "trace written");
    CHECK(keys[0].state == bvh::PoolPrevTrace::KeyState::Done && keys[0].frames_recorded == 120u, "A done (%u)", keys[0].frames_recorded);
    CHECK(keys[1].state == bvh::PoolPrevTrace::KeyState::Done && keys[1].frames_recorded == 120u, "B done (%u)", keys[1].frames_recorded);
    CHECK(keys[2].state == bvh::PoolPrevTrace::KeyState::Done && keys[2].frames_recorded == 120u, "C done (%u)", keys[2].frames_recorded);
    CHECK(keys[0].mesh_key != keys[1].mesh_key && keys[1].mesh_key != keys[2].mesh_key && keys[0].mesh_key != keys[2].mesh_key,
          "three distinct keys");
    CHECK(keys[0].max_meters >= 1.f && keys[1].max_meters < 0.1f && keys[2].camera_sightings >= 4u, "evidence");
    for (size_t k = 0; k < 3; ++k) {
      std::vector<bvh::PoolPrevTrace::Record> rows;
      for (const auto& record : traced.trace.records) if (record.key == k) rows.push_back(record);
      CHECK(rows.size() == 120u, "key %zu: 120 records (%zu)", k, rows.size());
      bool consecutive = true;
      for (size_t i = 1; i < rows.size(); ++i) if (rows[i].frame != rows[i - 1].frame + 1u) consecutive = false;
      CHECK(consecutive && keys[k].missed == 0u, "key %zu: consecutive frames, missed %u", k, keys[k].missed);
      if (k == 0u) continue;  // A: prevWorld is a flip, not the last world
      size_t compared = 0u;
      for (const auto& record : rows) {
        if (!record.has_last) continue;
        compared += 1u;
        CHECK(record.prev_vs_last.exact, "key %zu frame %u: prev equals last world", k, record.frame);
      }
      CHECK(compared >= 118u, "key %zu: compared %zu", k, compared);
    }
    CHECK(traced.trace.records.size() == 360u, "records (%zu)", traced.trace.records.size());
  }

  HStage("21b. A traced key with 64 instances: trace copies are capped at");
  // 21b. A traced key with 64 instances: trace copies are capped at
  // kPoolTraceMaxElements per frame and leave the normal copy counters alone.
  {
    struct Scenario {
      bvh::PoolStats stats;
      bvh::PoolPrevTrace trace;
    };
    auto big = [&](bool armed) {
      fresh();
      objects.push_back({make_vb(8.f).handle, 64, [](uint32_t f, uint32_t i, float* w, float* p) {
        Place(w, 5.f + 0.05f * f + 0.5f * i); Place(p, 5.f + 0.05f * (f == 0u ? 0u : f - 1u) + 0.5f * i);
      }});
      falcom_world::g_state.frame.store(2000u);
      if (armed) {
        std::lock_guard lock(bvh::g_pool.mutex);
        bvh::g_pool.prev_trace = {};
        bvh::g_pool.prev_trace.armed_frame = falcom_world::g_state.frame.load();
      }
      run(300);
      Scenario result;
      result.stats = snapshot();
      std::lock_guard lock(bvh::g_pool.mutex);
      result.trace = bvh::g_pool.prev_trace;
      return result;
    };
    const Scenario traced = big(true);
    const Scenario plain = big(false);
    const auto& t = traced.stats;
    const auto& p = plain.stats;
    CHECK(t.copied_draws == p.copied_draws && t.instances_seen == p.instances_seen && t.admitted == p.admitted
              && t.base_verified == p.base_verified && t.queued == p.queued,
          "big: copied %llu/%llu seen %llu/%llu admitted %zu/%zu", (unsigned long long)t.copied_draws,
          (unsigned long long)p.copied_draws, (unsigned long long)t.instances_seen, (unsigned long long)p.instances_seen,
          t.admitted, p.admitted);
    CHECK(plain.trace.records.empty(), "big: unarmed no trace");
    CHECK(!traced.trace.records.empty(), "big: trace records");
    std::map<uint32_t, uint32_t> per_frame;
    for (const auto& record : traced.trace.records) per_frame[record.frame] += 1u;
    uint32_t most = 0u;
    for (const auto& [frame, rows] : per_frame) most = (std::max)(most, rows);
    CHECK(most > 0u && most <= bvh::kPoolTraceMaxElements, "big: records per frame %u (max %u)", most, bvh::kPoolTraceMaxElements);
  }

  HStage("21c. A key whose motion stopped more than kPoolMovingHoldFrames + 30 frames");
  // 21c. A key whose motion stopped more than kPoolMovingHoldFrames + 30 frames
  // ago is not selected by the trace.
  {
    fresh();
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(8.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      auto x = [](uint32_t t) { return t < 40u ? 0.05f * t : 2.f; };
      Place(w, x(f)); Place(p, x(f == 0u ? 0u : f - 1u));
    }});
    run(40);
    run(bvh::kPoolMovingHoldFrames + 30u);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.prev_trace = {};
      bvh::g_pool.prev_trace.armed_frame = falcom_world::g_state.frame.load();
    }
    run(120);
    std::lock_guard lock(bvh::g_pool.mutex);
    const auto& trace = bvh::g_pool.prev_trace;
    // A and B (moving keys) stay waiting; C (static, admitted) may be selected.
    CHECK(trace.keys[0].state == bvh::PoolPrevTrace::KeyState::Waiting && trace.keys[1].state == bvh::PoolPrevTrace::KeyState::Waiting,
          "stopped long ago: no moving key selected (A %d, B %d)", (int)trace.keys[0].state, (int)trace.keys[1].state);
  }

  // ---- Follow mode (P2a off, follow on): a moving object keeps its instance and it follows the pose ----
  check_every_frame = true;
  HStage("22. One mover at 5 cm/frame: one instance per frame after warm-up, no ghost;");
  // 22. One mover at 5 cm/frame: one instance per frame after warm-up, no ghost;
  // its matrix is the pose of its last follow; inverse and bounds are recomputed;
  // one admitted observation for the whole pose chain.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(9.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      Place(w, 0.05f * f); Place(p, 0.05f * (f == 0u ? 0u : f - 1u));
    }});
    run(60);
    size_t fewest = SIZE_MAX;
    size_t most = 0u;
    for (int i = 0; i < 140; ++i) {
      run(1);
      std::lock_guard lock(bvh::g_pool.mutex);
      fewest = (std::min)(fewest, bvh::g_pool.instances.size());
      most = (std::max)(most, bvh::g_pool.instances.size());
    }
    CHECK(fewest == 1u && most == 1u, "follow: instances per frame %zu..%zu (want 1)", fewest, most);
    std::lock_guard lock(bvh::g_pool.mutex);
    const bvh::WorldInstance& inst = bvh::g_pool.instances[0];
    float expect[12];
    Place(expect, 0.05f * static_cast<float>(inst.last_follow_frame - start));
    CHECK(std::memcmp(inst.matrix, expect, sizeof(expect)) == 0, "follow: matrix is the pose of its last follow (%u)", inst.last_follow_frame);
    float inverse[16] = {};
    falcom_world::ComputeMatrixInverse(inst.matrix, inverse);
    float bounds_min[3] = {};
    float bounds_max[3] = {};
    bvh::ComputePoolInstanceBounds(bvh::g_pool.meshes[inst.mesh_id], inst.matrix, bounds_min, bounds_max);
    CHECK(std::memcmp(inverse, inst.inverse_world, sizeof(inverse)) == 0, "follow: inverse recomputed");
    CHECK(std::memcmp(bounds_min, inst.bounds_min, sizeof(bounds_min)) == 0
              && std::memcmp(bounds_max, inst.bounds_max, sizeof(bounds_max)) == 0, "follow: bounds recomputed");
    size_t admitted_observations = 0u;
    for (const auto& [key, observed] : bvh::g_pool.observations) {
      if (observed.admitted) admitted_observations += 1u;
    }
    CHECK(admitted_observations == 1u, "follow: admitted observations %zu (want 1)", admitted_observations);
    CHECK(bvh::g_pool.stats.follow_admits == 1u && bvh::g_pool.stats.follow_hits >= 100u,
          "follow: admits %llu hits %llu", (unsigned long long)bvh::g_pool.stats.follow_admits,
          (unsigned long long)bvh::g_pool.stats.follow_hits);
  }

  HStage("23. A mover and a static neighbour under one draw key: the mover's start");
  // 23. A mover and a static neighbour under one draw key: the mover's start
  // pose and the neighbour are purged when the mesh is flagged; the neighbour
  // re-admits and its matrix never changes.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(10.f).handle, 2, [start](uint32_t f, uint32_t i, float* w, float* p) {
      f -= start;
      if (i == 0u) {
        Place(w, f < 40u ? 0.f : 0.05f * static_cast<float>(f - 40u));
        Place(p, f <= 40u ? 0.f : 0.05f * static_cast<float>(f - 41u));
      } else {
        Place(w, -5.f); Place(p, -5.f);
      }
    }});
    run(120);
    std::lock_guard lock(bvh::g_pool.mutex);
    size_t statics = 0u;
    size_t others = 0u;
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      float at3[12];
      Place(at3, -5.f);
      if (std::memcmp(inst.matrix, at3, sizeof(at3)) == 0) statics += 1u;
      else others += 1u;
    }
    CHECK(statics == 1u && others == 1u, "neighbour: static %zu, mover %zu (want 1 and 1)", statics, others);
    CHECK(bvh::g_pool.stats.dynamic_retired >= 1u, "neighbour: purged with the mesh flag (%llu)",
          (unsigned long long)bvh::g_pool.stats.dynamic_retired);
  }

  HStage("24. A mover not drawn for one frame (its pose rejected): no orphan at once.");
  // 24. A mover not drawn for one frame (its pose rejected): the gap relinks.
  // One instance, one relink, no orphan, however long it runs after the gap.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    uint32_t gap_from = UINT32_MAX;
    uint32_t gap_frames = 0u;
    objects.push_back({make_vb(11.f).handle, 1, [start, &gap_from, &gap_frames](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      const bool gap = f >= gap_from && f < gap_from + gap_frames;
      Place(w, gap ? NAN : 0.05f * f);
      Place(p, gap ? NAN : 0.05f * (f == 0u ? 0u : f - 1u));
    }});
    run(60);
    gap_from = falcom_world::g_state.frame.load() - start;
    gap_frames = 1u;
    run(1);
    CHECK(snapshot().orphans == 0u, "gap: no orphan after one frame");
    gap_frames = 0u;
    run(bvh::kPoolMovingHoldFrames + 10u);
    const auto s = snapshot();
    CHECK(s.follow_relinks == 1u && s.admitted == 1u && s.orphans == 0u,
          "gap relinks: relinks %llu admitted %zu orphans %llu (want 1, 1, 0)", (unsigned long long)s.follow_relinks, s.admitted,
          (unsigned long long)s.orphans);
  }

  HStage("24b. A gap frame writes NaN into prevWorld only: the mover relinks, no second instance.");
  // 24b. A gap frame writes NaN into prevWorld only: the mover relinks, no second instance.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(13.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      Place(w, 0.05f * f);
      Place(p, f == 60u ? NAN : 0.05f * (f == 0u ? 0u : f - 1u));
    }});
    run(120);
    const auto s = snapshot();
    CHECK(s.follow_relinks == 1u && s.admitted == 1u && s.orphans == 0u,
          "NaN prev gap: relinks %llu admitted %zu orphans %llu (want 1, 1, 0)", (unsigned long long)s.follow_relinks, s.admitted,
          (unsigned long long)s.orphans);
  }

  HStage("25. Move, stop until released, move again: one instance, at the current pose.");
  // 25. Move, stop until released, move again: one instance, at the current pose.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(12.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      auto x = [](uint32_t t) { return t < 40u ? 0.05f * t : (t < 120u ? 2.f : 2.f + 0.05f * (t - 120u)); };
      Place(w, x(f)); Place(p, x(f == 0u ? 0u : f - 1u));
    }});
    run(200);
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.instances.size() == 1u, "stop and move: instances %zu (want 1)", bvh::g_pool.instances.size());
    if (bvh::g_pool.instances.size() == 1u) {
      const bvh::WorldInstance& inst = bvh::g_pool.instances[0];
      float expect[12];
      const uint32_t t = inst.last_follow_frame - start;
      Place(expect, t < 40u ? 0.05f * t : (t < 120u ? 2.f : 2.f + 0.05f * (t - 120u)));
      CHECK(std::memcmp(inst.matrix, expect, sizeof(expect)) == 0, "stop and move: matrix at the current pose (%u)", t);
    }
  }

  check_every_frame = false;
  HStage("26. Copies: a follow key takes one copy a frame; a static key one per");
  // 26. Copies: a follow key takes one copy a frame; a static key one per
  // kPoolRecaptureFrames. A follow copy may fill at most half a staging slot.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(13.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      Place(w, 0.05f * f); Place(p, 0.05f * (f == 0u ? 0u : f - 1u));
    }});
    run(40);
    const uint64_t moving_before = snapshot().copied_draws;
    run(60);
    const uint64_t moving_copies = snapshot().copied_draws - moving_before;
    fresh();
    objects.push_back({make_vb(14.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) { Place(w, 3.f); Place(p, 3.f); }});
    run(40);
    const uint64_t static_before = snapshot().copied_draws;
    run(60);
    const uint64_t static_copies = snapshot().copied_draws - static_before;
    CHECK(moving_copies == 60u, "copies: moving %llu per 60 frames (want 60)", (unsigned long long)moving_copies);
    CHECK(static_copies <= 60u / bvh::kPoolRecaptureFrames + 1u, "copies: static %llu per 60 frames",
          (unsigned long long)static_copies);
    fresh();
    run(5);
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::PoolStagingSlot& slot = bvh::g_pool.slots[bvh::g_pool.write_slot];
    const uint64_t used = slot.used, follow_used = slot.follow_used;
    const uint64_t skips = bvh::g_pool.stats.follow_budget_skips;
    const uint64_t cap = bvh::kPoolStagingSlotBytes / bvh::kPoolFollowSlotShare;
    bvh::PoolSchedule* out = nullptr;
    auto reserve = [&](bvh::PoolCopyKind kind, uint64_t bytes) {
      return bvh::ReservePoolCopy(&dev, 0x1234u, falcom_world::g_state.frame.load(), bytes, &out, kind);
    };
    // Normal bytes past half a slot do not refuse a follow copy that fits the follow cap.
    slot.used = bvh::kPoolStagingSlotBytes / 2u + 1u;
    slot.follow_used = 0u;
    CHECK(reserve(bvh::PoolCopyKind::Follow, cap / 2u) == bvh::PoolSkip::None
              && bvh::g_pool.stats.follow_budget_skips == skips,
          "follow copy under the follow cap accepted past half a slot");
    slot.used = cap - 100u;
    slot.follow_used = cap - 100u;
    CHECK(reserve(bvh::PoolCopyKind::Follow, 200u) == bvh::PoolSkip::BudgetFull
              && bvh::g_pool.stats.follow_budget_skips == skips + 1u,
          "follow copy over the follow cap refused and counted");
    CHECK(reserve(bvh::PoolCopyKind::Trace, 200u) == bvh::PoolSkip::None
              && bvh::g_pool.stats.follow_budget_skips == skips + 1u,
          "trace copy at the follow cap accepted and not counted");
    slot.follow_used = 0u;
    CHECK(reserve(bvh::PoolCopyKind::Normal, 200u) == bvh::PoolSkip::None, "normal copy unchanged");
    slot.used = used;
    slot.follow_used = follow_used;
    for (const auto& s : bvh::g_pool.slots) {
      if (s.used == 0u) CHECK(s.follow_used == 0u, "follow_used zero on an empty slot");
    }
  }

  HStage("27. Exclude on with follow on: the P2a path, the mover is never admitted.");
  // 27. Exclude on with follow on: the P2a path, the mover is never admitted.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(true);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(15.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      Place(w, 0.05f * f); Place(p, 0.05f * (f == 0u ? 0u : f - 1u));
    }});
    run(150);
    const auto s = snapshot();
    CHECK(s.follow_hits == 0u && s.follow_admits == 0u && s.admitted == 0u,
          "exclude on: follow %llu/%llu admitted %zu", (unsigned long long)s.follow_hits,
          (unsigned long long)s.follow_admits, s.admitted);
    bvh::g_pool.exclude_moving.store(false);
  }

  HStage("28. The dump: schema 10, the follow object, switch values, per-key follows, balanced braces.");
  // 28. The dump: schema 10, the follow object, switch values, per-key follows, balanced braces.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(16.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      Place(w, 0.05f * f); Place(p, 0.05f * (f == 0u ? 0u : f - 1u));
    }});
    run(120);
    const std::string text = read_dump();
    CHECK(text.find("\"schema\": 15") != std::string::npos, "dump: schema 15");
    CHECK(text.find("\"follow_moving\": true") != std::string::npos && text.find("\"follow\": {\"hits\"") != std::string::npos,
          "dump: follow object and switch");
    CHECK(text.find("\"follows\": ") != std::string::npos, "dump: per key follows");
    CHECK(std::count(text.begin(), text.end(), '{') == std::count(text.begin(), text.end(), '}')
              && std::count(text.begin(), text.end(), '[') == std::count(text.begin(), text.end(), ']'), "dump: balanced");
  }

  check_every_frame = true;

  HStage("29. Stopped prop: kept while seen; a lone hidden one is kept 70 frames; a new pose retires the old one at 60.");
  // 29. Stopped prop: kept while seen; a lone hidden one is kept 70 frames; a new pose retires the old one at 60.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    const uint32_t vb = make_vb(17.f).handle;
    auto x = [](uint32_t t) { return t < 40u ? 0.05f * static_cast<float>(t) : 2.f; };
    objects.push_back({vb, 1, [start, x](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      Place(w, x(f)); Place(p, x(f == 0u ? 0u : f - 1u));
    }});
    run(300);
    CHECK(snapshot().admitted == 1u && snapshot().orphans_retired == 0u, "prop kept while seen (admitted %zu, retired %llu)",
          snapshot().admitted, (unsigned long long)snapshot().orphans_retired);
    objects.clear();
    run(70);
    CHECK(snapshot().admitted == 1u && snapshot().orphans_retired == 0u, "lone hidden 70 frames: kept (admitted %zu, retired %llu)",
          snapshot().admitted, (unsigned long long)snapshot().orphans_retired);
  }

  HStage("29b. Orphans: a superseded dynamic instance retires at 60 frames; a lone one is kept to 600.");
  // 29b. Orphans: a superseded dynamic instance retires at 60 frames; a lone one is kept to 600.
  {
    fresh();
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.instances.clear();
    bvh::WorldInstance old_pose;
    old_pose.mesh_key = 0x77u;
    old_pose.dynamic = true;
    old_pose.admit_frame = 100u;
    bvh::WorldInstance new_pose = old_pose;
    new_pose.admit_frame = 200u;
    bvh::WorldInstance lone = old_pose;
    lone.mesh_key = 0x88u;
    bvh::g_pool.instances = {old_pose, new_pose, lone};
    std::vector<std::string> orphan_lines;
    bvh::RetirePoolOrphans(160u, bvh::PoolNewestSeenByKey(), &orphan_lines);
    CHECK(bvh::g_pool.instances.size() == 3u && bvh::g_pool.stats.orphans_retired == 0u, "60 frames: nothing retired yet");
    bvh::RetirePoolOrphans(161u, bvh::PoolNewestSeenByKey(), &orphan_lines);
    CHECK(bvh::g_pool.instances.size() == 2u && bvh::g_pool.stats.orphans_retired == 1u
              && bvh::g_pool.instances[0].admit_frame == 200u,
          "superseded at 61 unseen frames: retired, the lone one kept (%zu left, retired %llu)", bvh::g_pool.instances.size(),
          (unsigned long long)bvh::g_pool.stats.orphans_retired);
    bvh::RetirePoolOrphans(701u, bvh::PoolNewestSeenByKey(), &orphan_lines);
    CHECK(bvh::g_pool.instances.size() == 1u && bvh::g_pool.stats.orphans_retired == 2u,
          "lone one retired after 600 unseen frames (%zu left, retired %llu)", bvh::g_pool.instances.size(),
          (unsigned long long)bvh::g_pool.stats.orphans_retired);
  }

  HStage("29c. Orphan bounds: a static or dynamic instance is retired only by a fresher same-key pose with overlapping bounds.");
  // 29c. Static orphans (overlap, same key, fresher by more than kPoolGhostHoldFrames); touching faces and an exact hold are kept.
  {
    fresh();
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.meshes.resize(1);
    const auto make = [](uint64_t key, bool dynamic, uint32_t seen, float min_x) {
      bvh::WorldInstance instance;
      instance.mesh_key = key;
      instance.dynamic = dynamic;
      instance.admit_frame = seen;
      instance.bounds_min[0] = min_x;
      instance.bounds_max[0] = min_x + 1.f;
      for (size_t k = 1; k < 3; ++k) {
        instance.bounds_min[k] = 0.f;
        instance.bounds_max[k] = 1.f;
      }
      return instance;
    };
    const auto retire = [](std::vector<bvh::WorldInstance> instances, uint32_t frame) {
      bvh::g_pool.instances = std::move(instances);
      std::vector<std::string> orphan_lines;
      bvh::RetirePoolOrphans(frame, bvh::PoolNewestSeenByKey(), &orphan_lines);
      return bvh::g_pool.instances;
    };
    const auto a = retire({make(0x91u, false, 100u, 0.f), make(0x91u, false, 170u, 0.1f)}, 200u);
    CHECK(a.size() == 1u && a[0].admit_frame == 170u, "(a) overlapping fresher static: static ghost retired (%zu left)", a.size());
    const auto b = retire({make(0x92u, true, 100u, 0.f), make(0x92u, false, 170u, 0.1f)}, 200u);
    CHECK(b.size() == 1u && !b[0].dynamic && b[0].admit_frame == 170u, "(b) overlapping fresher static: dynamic ghost retired (%zu left)", b.size());
    const auto c = retire({make(0x93u, false, 100u, 0.f), make(0x93u, true, 190u, 5.f)}, 200u);
    CHECK(c.size() == 2u, "(c) disjoint live dynamic sibling: static kept and dynamic kept (%zu left)", c.size());
    const auto d = retire({make(0x94u, false, 100u, 0.f), make(0x94u, false, 170u, 1.f)}, 200u);
    CHECK(d.size() == 2u, "(d) boxes touching on a face only: both kept (%zu left)", d.size());
    const auto e = retire({make(0x95u, false, 100u, 0.f), make(0x95u, false, 140u, 0.5f)}, 200u);
    CHECK(e.size() == 2u, "(e) fresher by exactly kPoolGhostHoldFrames: both kept (%zu left)", e.size());
    const auto f = retire({make(0x96u, false, 100u, 0.f), make(0x96u, false, 170u, 0.97f)}, 200u);
    CHECK(f.size() == 2u, "(f) static off-screen sibling, 3%% overlap of the smaller box: both kept (%zu left)", f.size());
    const auto g = retire({make(0x97u, false, 100u, 0.f), make(0x97u, false, 170u, 0.1f)}, 200u);
    CHECK(g.size() == 1u && g[0].admit_frame == 170u, "(g) static ghost, 90%% overlap with a fresher pose: retired (%zu left)", g.size());
    const auto h = retire({make(0x98u, false, 100u, 0.f), make(0x98u, false, 140u, 0.f)}, 300u);
    CHECK(h.size() == 2u, "(h) pose only 40 newer than instance (within hold), overlapping: kept (%zu left)", h.size());
    const auto i = retire({make(0x99u, false, 100u, 0.f), make(0x99u, false, 170u, 0.5f)}, 200u);
    CHECK(i.size() == 1u && i[0].admit_frame == 170u, "(i) static, 50%% overlap of the smaller box, at least 0.05: retired (%zu left)", i.size());
    bvh::WorldInstance slab_a = make(0x9Au, false, 100u, 0.f);
    slab_a.bounds_max[1] = 2.f;
    slab_a.bounds_max[2] = 0.1f;
    bvh::WorldInstance slab_b = make(0x9Au, false, 170u, 0.f);
    slab_b.bounds_max[0] = 0.1f;
    slab_b.bounds_max[1] = 2.f;
    const auto hinge = retire({slab_a, slab_b}, 200u);
    CHECK(hinge.size() == 1u && hinge[0].admit_frame == 170u, "(j) slab hinge, 10%% overlap of the smaller box: older slab retired (%zu left)", hinge.size());
    const auto k = retire({make(0x9Bu, false, 100u, 0.f), make(0x9Bu, false, 170u, 0.97f)}, 200u);
    CHECK(k.size() == 2u, "(k) two props, 3%% overlap of the smaller box: both kept (%zu left)", k.size());
    bvh::WorldInstance hinge_pose = slab_b;
    hinge_pose.admit_frame = 145u;
    bvh::g_pool.instances = {slab_a, hinge_pose};
    std::vector<std::string> hinge_lines;
    uint32_t hinge_retired_at = 0u;
    for (uint32_t frame = 110u; frame <= 150u && hinge_retired_at == 0u; frame += 10u) {
      if (frame >= 145u) bvh::g_pool.instances.back().admit_frame = frame;
      bvh::RetirePoolOrphans(frame, bvh::PoolNewestSeenByKey(), &hinge_lines);
      if (bvh::g_pool.instances.size() == 1u) hinge_retired_at = frame;
    }
    CHECK(hinge_retired_at == 150u, "(l) door-like hinge ghost seen at 100, hinge pose seen from 145: retired by 150 (at %u)", hinge_retired_at);
    bvh::g_pool.instances = {make(0x9Du, false, 0u, 0.f), make(0x9Du, false, 0u, 0.5f)};
    std::vector<std::string> live_lines;
    for (uint32_t frame = 10u; frame <= 300u; frame += 10u) {
      bvh::g_pool.instances[0].admit_frame = frame - frame % 30u;
      bvh::g_pool.instances[1].admit_frame = frame >= 15u ? (frame - 15u) / 30u * 30u + 15u : 0u;
      bvh::RetirePoolOrphans(frame, bvh::PoolNewestSeenByKey(), &live_lines);
    }
    CHECK(bvh::g_pool.instances.size() == 2u, "(m) live same-key neighbour, 50%% overlap, refreshed every 30 frames offset by 15: none retired over 300 frames (%zu left)", bvh::g_pool.instances.size());
    const auto n = retire({make(0x9Eu, true, 100u, 0.f), make(0x9Eu, true, 150u, 0.1f)}, 141u);
    CHECK(n.size() == 2u, "(n) dynamic ghost 50 frames older, overlapping fresher pose, 41 unseen frames: kept (%zu left)", n.size());
    const auto o = retire({make(0x9Fu, true, 100u, 0.f), make(0x9Fu, true, 170u, 0.1f)}, 171u);
    CHECK(o.size() == 1u && o[0].admit_frame == 170u, "(o) dynamic ghost 70 frames older, overlapping fresher pose: retired (%zu left)", o.size());
  }

  HStage("29e. Stutter cap: 10,000 same-key statics, some fresher by more than kPoolMovingHoldFrames; no overlap retirement for the key.");
  {
    fresh();
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.meshes.resize(1);
    for (uint32_t n = 0u; n < 10000u; ++n) {
      bvh::WorldInstance instance;
      instance.mesh_key = 0xA0u;
      instance.admit_frame = n % 2u ? 100u : 170u;
      for (size_t k = 0; k < 3; ++k) {
        instance.bounds_min[k] = 0.f;
        instance.bounds_max[k] = 1.f;
      }
      instance.bounds_min[0] = 2.f * static_cast<float>(n);
      instance.bounds_max[0] = instance.bounds_min[0] + 1.f;
      bvh::g_pool.instances.push_back(instance);
    }
    std::vector<std::string> cap_lines;
    const auto t0 = std::chrono::steady_clock::now();
    bvh::RetirePoolOrphans(200u, bvh::PoolNewestSeenByKey(), &cap_lines);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    CHECK(bvh::g_pool.instances.size() == 10000u, "10k same-key statics: none retired (%zu left)", bvh::g_pool.instances.size());
    std::printf("10k same-key RetirePoolOrphans: %.2f ms\n", ms);
#if !defined(__SANITIZE_ADDRESS__)
    if (std::getenv("FALCOM_QUICK")) CHECK(ms <= 20.0, "10k same-key RetirePoolOrphans took %.2f ms (limit 20)", ms);
#endif
  }

  HStage("29d. Static object moved to an overlapping pose with prevWorld == world: only the new pose remains.");
  // 29d. The old pose is superseded by the new one (overlapping), so it retires once the hold runs out.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    const uint32_t vb = make_vb(1.f).handle;
    objects.push_back({vb, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      const float x = f - start < 40u ? 0.f : 0.5f;
      Place(w, x); Place(p, x);
    }});
    run(240);
    const auto s = snapshot();
    float x = -1.f;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      if (!bvh::g_pool.instances.empty()) x = bvh::g_pool.instances[0].matrix[3];
    }
    CHECK(s.admitted == 1u && std::fabs(x - 0.5f) < 1e-4f, "moved static: only the new pose remains (admitted %zu, x %g, retired %llu)",
          s.admitted, x, (unsigned long long)s.orphans_retired);
  }

  HStage("30. Retire, then destroy the VB, CompactPool and MarkPoolMeshDynamic: no second removal.");
  // 30. Retire, then destroy the VB, CompactPool and MarkPoolMeshDynamic: no second removal.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    const uint64_t vb = make_vb(18.f).handle;
    objects.push_back({vb, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      Place(w, 0.05f * static_cast<float>(f)); Place(p, 0.05f * static_cast<float>(f == 0u ? 0u : f - 1u));
    }});
    run(50);
    objects.clear();
    run(610);  // a lone hidden instance is held for kPoolPruneAge (600) frames
    const auto before = snapshot();
    CHECK(before.orphans_retired == 1u && before.admitted == 0u, "retired once (retired %llu)", (unsigned long long)before.orphans_retired);
    bvh::OnDestroyResourcePool(&dev, resource{vb});
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::CompactPool();
      for (const auto& [key, entry] : bvh::g_pool.dynamic_keys) {
        (void)entry;
        const auto mesh_it = bvh::g_pool.mesh_by_key.find(key);
        if (mesh_it != bvh::g_pool.mesh_by_key.end()) {
          CHECK(bvh::MarkPoolMeshDynamic(mesh_it->second) == 0u, "mark after retire removes nothing");
        }
      }
    }
    const auto after = snapshot();
    CHECK(after.orphans_retired == 1u && after.dynamic_retired == before.dynamic_retired, "no double removal (retired %llu, dynamic_retired %llu)",
          (unsigned long long)after.orphans_retired, (unsigned long long)after.dynamic_retired);
  }

  HStage("31. Scan off 100 frames retires nothing; scan back on gives a grace period.");
  // 31. Scan off 100 frames retires nothing; scan back on gives a grace period.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(true);
    const uint32_t start = falcom_world::g_state.frame.load();
    objects.push_back({make_vb(19.f).handle, 1, [start](uint32_t f, uint32_t, float* w, float* p) {
      f -= start;
      Place(w, 0.05f * static_cast<float>(f)); Place(p, 0.05f * static_cast<float>(f == 0u ? 0u : f - 1u));
    }});
    run(40);
    objects.clear();
    bvh::g_pool.scan_active.store(false);
    run(100);
    CHECK(snapshot().orphans_retired == 0u, "scan off: nothing retired (%llu)", (unsigned long long)snapshot().orphans_retired);
    bvh::g_pool.scan_active.store(true);
    run(30);
    CHECK(snapshot().orphans_retired == 0u, "scan back on: nothing retired yet");
    run(500);
    CHECK(snapshot().orphans_retired == 1u, "scan back on: lone hidden instance retired after 600 frames (%llu)", (unsigned long long)snapshot().orphans_retired);
  }
  check_every_frame = false;

  HStage("32. Non-finite values in the dump; manifest sizes; capped lists count what they drop.");
  // 32. A NaN and an inf world matrix (rejected samples), a NaN mesh bbox, instance bounds and region size, then 300 failed meshes.
  {
    fresh();
    bvh::g_pool.exclude_moving.store(false);
    bvh::g_pool.follow_moving.store(false);
    objects.push_back({make_vb(21.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) {
      Place(w, 70.f); Place(p, 70.f); w[0] = NAN;
    }});
    objects.push_back({make_vb(22.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) {
      Place(w, 72.f); Place(p, 72.f); w[3] = INFINITY;
    }});
    objects.push_back({make_vb(23.f).handle, 1, [](uint32_t, uint32_t, float* w, float* p) {
      Place(w, 74.f); Place(p, 74.f);
    }});
    run(150);
    float saved_region = 0.f, saved_bbox = 0.f, saved_bounds = 0.f;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      CHECK(!bvh::g_pool.meshes.empty() && !bvh::g_pool.instances.empty(), "meshes and instances admitted");
      saved_region = bvh::g_pool.region_size;
      saved_bbox = bvh::g_pool.meshes[0].bbox_min[0];
      saved_bounds = bvh::g_pool.instances[0].bounds_min[0];
      bvh::g_pool.region_size = NAN;
      bvh::g_pool.meshes[0].bbox_min[0] = NAN;
      bvh::g_pool.instances[0].bounds_min[0] = NAN;
    }
    const std::filesystem::path dir = bvh::PoolOutputDir();
    const auto read_file = [](const std::filesystem::path& path) {
      std::ifstream f(path, std::ios::binary);
      return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    };
    // Value of "field" in the manifest entry named `name` ("" when absent).
    const auto manifest_value = [](const std::string& manifest, const std::string& name, const std::string& field) {
      const size_t entry = manifest.find("\"name\": \"" + name + "\"");
      const size_t at = entry == std::string::npos ? std::string::npos : manifest.find("\"" + field + "\": ", entry);
      if (at == std::string::npos) return std::string();
      const size_t begin = at + field.size() + 4u;
      return manifest.substr(begin, manifest.find_first_of(",}", begin) - begin);
    };
    const std::string world_text = read_dump();
    const std::string meshes_text = read_file(dir / "world_pool_meshes.json");
    const std::string instances_text = read_file(dir / "world_pool_instances.json");
    const std::string manifest_text = read_file(dir / "world_pool_manifest.json");
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.region_size = saved_region;
      bvh::g_pool.meshes[0].bbox_min[0] = saved_bbox;
      bvh::g_pool.instances[0].bounds_min[0] = saved_bounds;
    }
    const std::pair<const char*, const std::string*> dumps[] = {
        {"world_pool.json", &world_text}, {"world_pool_meshes.json", &meshes_text},
        {"world_pool_instances.json", &instances_text}, {"world_pool_manifest.json", &manifest_text}};
    for (const auto& [name, text] : dumps) {
      CHECK(JsonError(*text).empty(), "%s is valid JSON: %s", name, JsonError(*text).c_str());
      CHECK(!HasNonFiniteToken(*text), "%s has no nan or inf token", name);
    }
    for (const char* name : {"world_pool.json", "world_pool_meshes.json", "world_pool_instances.json"}) {
      CHECK(manifest_value(manifest_text, name, "bytes") == std::to_string(std::filesystem::file_size(dir / name)),
            "%s manifest bytes %s", name, manifest_value(manifest_text, name, "bytes").c_str());
    }
    CHECK(meshes_text.find("\"bbox_min\": [null") != std::string::npos, "NaN mesh bbox written as null");
    CHECK(instances_text.find("\"bounds_min\": [null") != std::string::npos, "NaN instance bounds written as null");
    CHECK(world_text.find("\"region\": {\"size\": null") != std::string::npos, "NaN region size written as null");
    const size_t reject_at = world_text.find("\"reject_samples\": [");
    CHECK(reject_at != std::string::npos && world_text.find("\"world\": [null", reject_at) != std::string::npos,
          "NaN matrix written as a null reject sample");
    CHECK(manifest_value(manifest_text, "world_pool.json", "dropped") == "0", "no drops before the flood");
    CHECK(world_text.find("\"truncated\": false,\n") != std::string::npos, "header not truncated before the flood");
    // P2b fields (WritePoolP2b) and the manifest's build id and switch states.
    for (const char* field : {"ready", "expand_presents", "expand_empty_presents", "expand_failed_presents", "instance_count",
                              "trace_instance_count", "trace_frame", "tlas_per_frame", "tlas_dynamic_partition", "tlas_flagged_skipped",
                              "flagged_meshes_total", "flagged_meshes_resident"}) {
      CHECK(world_text.find(std::string("\"") + field + "\": ") != std::string::npos, "world_pool.json p2b field %s", field);
    }
    CHECK(manifest_text.find("\"build\": \"unknown\"") == std::string::npos && manifest_text.find(bvh::kWorldBuildId) != std::string::npos,
          "manifest carries the build id");
    for (const char* field : {"dynamic_per_frame", "alpha_wind_sway", "exclude_moving", "live_on", "legacy_scale"}) {
      CHECK(manifest_text.find(std::string("\"") + field + "\": ") != std::string::npos, "manifest switch %s", field);
    }

    // A NaN capture record (failed mesh history) is written as null, not as a nan token.
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::PoolMeshFailure& failure = bvh::g_pool.failed_meshes[0xFB00u];
      failure.history.emplace_back();
      failure.history[0].before[0] = NAN;
      failure.history[0].after[0] = NAN;
    }
    const std::string history_text = read_dump();
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.failed_meshes.erase(0xFB00u);
    }
    CHECK(JsonError(history_text).empty(), "NaN capture record dump is valid JSON: %s", JsonError(history_text).c_str());
    CHECK(!HasNonFiniteToken(history_text), "NaN capture record dump has no nan or inf token");
    CHECK(history_text.find("\"before\": [null") != std::string::npos, "NaN capture record before[0] written as null");

    // Camera position: GetPoolCameraInfo rejects a non-finite camera, so it is covered by reading the code only.
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (uint64_t k = 0; k < 300u; ++k) bvh::g_pool.failed_meshes[0xFA00u + k] = bvh::PoolMeshFailure{};
    }
    const std::string flood_text = read_dump();
    const std::string flood_manifest = read_file(dir / "world_pool_manifest.json");
    CHECK(JsonError(flood_text).empty() && JsonError(flood_manifest).empty(), "flood dump is valid JSON");
    CHECK(std::atoll(manifest_value(flood_manifest, "world_pool.json", "dropped").c_str()) >= 44, "failed_meshes drops counted (%s)",
          manifest_value(flood_manifest, "world_pool.json", "dropped").c_str());
    CHECK(manifest_value(flood_manifest, "world_pool.json", "truncated") == "true", "manifest truncated");
    CHECK(flood_text.find("\"truncated\": true,\n") != std::string::npos, "header truncated");
    CHECK(manifest_value(flood_manifest, "world_pool.json", "bytes") == std::to_string(std::filesystem::file_size(dir / "world_pool.json")),
          "world_pool.json manifest bytes after the flood");
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (uint64_t k = 0; k < 300u; ++k) bvh::g_pool.failed_meshes.erase(0xFA00u + k);
    }
  }

  HStage("7. Reset clears the probe and the moving keys.");
  // 7. Reset clears the probe and the moving keys.
  {
    bvh::ResetWorldPool();
    const auto s = snapshot();
    const auto d = detail();
    CHECK(s.motion.repeat == 0u && s.motion.first == 0u && s.motion.moved == 0u && s.motion.moved_meshes == 0u
              && d.moved_samples.empty() && d.repeat_samples.empty() && d.moved_mesh_keys.empty()
              && d.rule_stale_samples.empty() && s.dynamic_live_keys == 0u && s.dynamic_retired == 0u, "reset");
  }

  HStage("33. P2b diagnostics: the verdict of each synthetic snapshot (BuildP2bDiagJson), valid JSON, field names.");
  const int failures_before_33 = g_failures;
  {
    const auto check_case = [](const char* name, const bvh::P2bDiagInput& in, const char* want) {
      std::string verdict;
      const std::string text = bvh::BuildP2bDiagJson(in, &verdict, nullptr);
      CHECK(JsonError(text).empty(), "%s: valid JSON: %s", name, JsonError(text).c_str());
      CHECK(!HasNonFiniteToken(text), "%s: no nan or inf token", name);
      CHECK(verdict == want, "%s: verdict '%s' (want '%s')", name, verdict.c_str(), want);
      for (const char* field : {"\"build\"", "\"switches\"", "\"expand\"", "\"trace\"", "\"tlas\"", "\"flagged_meshes\"", "\"readback\"",
                                "\"probe\"", "\"draws\"", "\"records\"", "\"summary\"", "\"verdict_hint\""}) {
        CHECK(text.find(field) != std::string::npos, "%s: field %s", name, field);
      }
    };
    const auto make_record = [](bool valid) {
      bvh::WorldInstanceGPU r = {};
      const float identity[12] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f};
      r.header[3] = valid ? 1.f : 0.f;
      std::memcpy(r.world, identity, sizeof(identity));
      r.world[15] = 1.f;
      falcom_world::ComputeMatrixInverse(identity, r.inverse_world);
      for (int k = 0; k < 3; ++k) {
        r.bounds_min[k] = -1.f;
        r.bounds_max[k] = 1.f;
      }
      r.visibility[2] = 2.f;
      return r;
    };
    // One resident camera draw of one identity instance; arena and readbacks as a healthy present.
    const auto base = [] {
      bvh::P2bDiagInput in;
      in.build = bvh::kWorldBuildId;
      in.generated_frame = 100u;
      in.armed_frame = 90u;
      in.arena.assign(64u * 40u, 0.f);
      in.arena[0] = 1.f;
      in.arena[5] = 1.f;
      in.arena[10] = 1.f;
      bvh::DynamicDrawGPU draw = {};
      draw.info[1] = 1u;
      draw.info[3] = 3u;
      for (int k = 0; k < 3; ++k) {
        draw.bbox_min[k] = -1.f;
        draw.bbox_max[k] = 1.f;
      }
      in.table.push_back(draw);
      in.table_readback_ok = true;
      in.table_matches = true;
      in.readback_ok = true;
      in.probe_ran = true;
      in.probe_tail_read = true;
      in.probe_plain_read = true;
      in.probe_arena_tail_landed = true;
      in.probe_plain_landed = true;
      in.counters.ready = true;
      in.counters.expand_presents = 5u;
      in.counters.instance_count = 1u;
      in.counters.trace_count = 1u;
      in.counters.trace_valid = true;
      in.fresh_trace = true;
      in.switches = {{"dynamic_per_frame", true}, {"live_on", true}};
      return in;
    };
    {  // (a) the arena is zero and the arena-tail copy did not land while the plain copy did
      bvh::P2bDiagInput in = base();
      std::fill(in.arena.begin(), in.arena.end(), 0.f);
      in.probe_arena_tail_landed = false;
      check_case("(a) zero arena, tail not landed", in, "H1b: structured-160 to structured-16 copy dropped");
    }
    {  // (a2) the arena-tail readback failed while the plain one succeeded: not H1b, the reason names the tail readback
      bvh::P2bDiagInput in = base();
      std::fill(in.arena.begin(), in.arena.end(), 0.f);
      in.probe_tail_read = false;
      in.probe_arena_tail_landed = false;
      in.probe_reason = "tail readback failed";
      const std::string text = bvh::BuildP2bDiagJson(in, nullptr, nullptr);
      check_case("(a2) tail readback failed, plain ok", in, "open");
      CHECK(text.find("\"reason\": \"tail readback failed\"") != std::string::npos, "(a2) probe reason names the tail readback");
    }
    {  // (a3) the plain readback failed while the tail one succeeded: not H1b
      bvh::P2bDiagInput in = base();
      std::fill(in.arena.begin(), in.arena.end(), 0.f);
      in.probe_plain_read = false;
      in.probe_plain_landed = false;
      in.probe_reason = "plain readback failed";
      check_case("(a3) plain readback failed, tail ok", in, "open");
    }
    {  // (b) the arena is zero although the probe landed
      bvh::P2bDiagInput in = base();
      std::fill(in.arena.begin(), in.arena.end(), 0.f);
      check_case("(b) zero arena, probe landed", in, "H1c: wrong source or offset");
    }
    {  // (b2) the arena is zero, the probe tail landed, but the plain readback failed: H1c needs both readbacks
      bvh::P2bDiagInput in = base();
      std::fill(in.arena.begin(), in.arena.end(), 0.f);
      in.probe_plain_read = false;
      in.probe_plain_landed = false;
      check_case("(b2) zero arena, plain readback failed", in, "open");
    }
    {  // (c) the GPU rejects a record the CPU accepts
      bvh::P2bDiagInput in = base();
      in.records.push_back(make_record(false));
      check_case("(c) GPU invalid, CPU valid", in, "H2: expansion shader or constants");
    }
    {  // (c2) the same GPU rejection with the trace not fresh: H2 is not gated by the trace
      bvh::P2bDiagInput in = base();
      in.records.push_back(make_record(false));
      in.fresh_trace = false;
      in.timed_out = true;
      check_case("(c2) GPU invalid, trace not fresh", in, "H2: expansion shader or constants");
    }
    {  // (c3) the draw table does not read back equal to the uploaded table
      bvh::P2bDiagInput in = base();
      in.table_matches = false;
      check_case("(c3) draw table differs", in, "H2: draw upload");
    }
    {  // (d) a valid record whose box the ray enters; the trace counted nothing
      bvh::P2bDiagInput in = base();
      in.records.push_back(make_record(true));
      in.ray_valid = true;
      in.ray_origin[2] = -10.f;
      in.ray_direction[2] = 1.f;
      in.counters.trace_count = 1u;
      in.counters.p2b_tested = 0u;
      check_case("(d) box crossed, trace counters 0", in, "H3a: count path or loop not reached");
    }
    {  // (d2) a draw whose arena instance count is zero: the arena is wrong
      bvh::P2bDiagInput in = base();
      in.table[0].extra[0] = 1u;
      in.table[0].extra[1] = 0u;
      check_case("(d2) indirect draw with zero instances", in, "H1d: wrong base or stride or stale buffer");
    }
    {  // (d3) a box the ray enters, the trace tested records, none hit the box
      bvh::P2bDiagInput in = base();
      in.records.push_back(make_record(true));
      in.ray_valid = true;
      in.ray_origin[2] = -10.f;
      in.ray_direction[2] = 1.f;
      in.counters.p2b_tested = 5u;
      in.counters.p2b_skipped = 0u;
      in.counters.p2b_box_hits = 0u;
      check_case("(d3) box crossed, no P2b box hit", in, "H3c: bounds or ray convention");
    }
    {  // (d4) the trace hit the P2b boxes but no ray won on a P2b record
      bvh::P2bDiagInput in = base();
      in.counters.p2b_tested = 5u;
      in.counters.p2b_skipped = 0u;
      in.counters.p2b_box_hits = 2u;
      in.counters.p2b_wins = 0u;
      check_case("(d4) box hits, no P2b win", in, "H3d: BLAS, slot or visibility");
    }
    {  // (e) the trace bound fewer records than the expansion wrote
      bvh::P2bDiagInput in = base();
      in.counters.instance_count = 3u;
      in.counters.trace_count = 2u;
      check_case("(e) instance count mismatch", in, "H4: second present or timing");
    }
    {  // (e2) the trace-count term is gated by a fresh trace: not fresh, no H4
      bvh::P2bDiagInput in = base();
      in.counters.instance_count = 3u;
      in.counters.trace_count = 2u;
      in.fresh_trace = false;
      check_case("(e2) instance count mismatch, trace not fresh", in, "open (no fresh trace)");
    }
    {  // (e3) the empty-present term is not gated by the trace: H4 with a stale trace
      bvh::P2bDiagInput in = base();
      in.counters.expand_presents = 5u;
      in.counters.expand_empty = 3u;
      in.fresh_trace = false;
      in.timed_out = true;
      check_case("(e3) empty presents, trace not fresh", in, "H4: second present or timing");
    }
    {  // (e4) nothing fires on a healthy fresh present
      bvh::P2bDiagInput in = base();
      check_case("(e4) healthy present", in, "open");
    }
    {  // (f) the arena read was skipped (zero bytes this present): the reason is in the readback block
      bvh::P2bDiagInput in = base();
      in.arena.clear();
      in.arena_reason = "skipped: arena bytes 0 this present";
      const std::string text = bvh::BuildP2bDiagJson(in, nullptr, nullptr);
      CHECK(text.find("\"arena_reason\": \"skipped: arena bytes 0 this present\"") != std::string::npos, "(f) arena_reason in readback");
      CHECK(text.find("\"records_reason\": \"ok\"") != std::string::npos, "(f) records_reason ok by default");
    }
    {  // (g) a box the ray enters, but the trace stats are stale (timed out): no trace row, verdict says so
      bvh::P2bDiagInput in = base();
      in.records.push_back(make_record(true));
      in.ray_valid = true;
      in.ray_origin[2] = -10.f;
      in.ray_direction[2] = 1.f;
      in.counters.p2b_tested = 0u;
      in.fresh_trace = false;
      in.timed_out = true;
      std::string verdict;
      const std::string text = bvh::BuildP2bDiagJson(in, &verdict, nullptr);
      CHECK(verdict == "open (no fresh trace)", "(g) stale trace verdict '%s'", verdict.c_str());
      CHECK(text.find("\"fresh\": false") != std::string::npos, "(g) trace fresh false");
      CHECK(text.find("\"timed_out\": true") != std::string::npos, "(g) trace timed_out true");
      CHECK(text.find("\"armed_frame\": 90") != std::string::npos, "(g) trace armed_frame");
    }
    {  // (h) almost every record skipped (1 tested, 4e9 skipped): the 9/10 ratio needs 64-bit math to be seen
      bvh::P2bDiagInput in = base();
      in.counters.p2b_tested = 1u;
      in.counters.p2b_skipped = 4000000000u;
      check_case("(h) skipped above 9/10 (no 32-bit wrap)", in, "H3b: t17 reads zeros or mesh_id out of range");
    }
    {  // (h2) the same counts without a fresh trace: the H3b row is gated
      bvh::P2bDiagInput in = base();
      in.counters.p2b_tested = 1u;
      in.counters.p2b_skipped = 4000000000u;
      in.fresh_trace = false;
      in.timed_out = true;
      check_case("(h2) skipped above 9/10, trace not fresh", in, "open (no fresh trace)");
    }
    {  // (h3) tested + skipped = 6e9 overflows 32 bits; the ratio is 1/2, so the decision is not H3b
      bvh::P2bDiagInput in = base();
      in.counters.p2b_tested = 3000000000u;
      in.counters.p2b_skipped = 3000000000u;
      check_case("(h3) tested 3e9 skipped 3e9 (sum overflows 32 bits)", in, "open");
    }
  }
  std::printf("33 P2b: exact-verdict cases run, %d failed\n", g_failures - failures_before_33);

  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures == 0 ? 0 : 1;
}
