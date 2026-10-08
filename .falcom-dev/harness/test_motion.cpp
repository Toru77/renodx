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
    bvh::DrainPoolScan(&dev, &queue);
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
    CHECK(text.find("\"schema\": 11") != std::string::npos, "schema 11");
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
    for (const char* bad : {"nan,", "nan]", "nan}", "inf,", "inf]", "inf}"}) CHECK(text.find(bad) == std::string::npos, "no %s in JSON", bad);
    std::ofstream("motion_dump2.json") << text;
  }


  // ---- P2a: moving rigid objects ----
  bvh::g_pool.exclude_moving.store(true);

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
    run(200);  // stopped at x = 6 since frame 210: released after the hold, the pose is admitted
    s = snapshot();
    CHECK(s.dynamic_released == 1u && s.dynamic_meshes == 0u && s.dynamic_moving_keys == 0u,
          "released after the hold (released %u, flagged %zu, moving %zu)", s.dynamic_released, s.dynamic_meshes, s.dynamic_moving_keys);
    CHECK(s.admitted == 2u, "stopped pose admitted after release (%zu)", s.admitted);
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
    CHECK(text.find("\"dynamic\": {\"keys\": 1") != std::string::npos, "dynamic section");
    CHECK(text.find("\"retired_instances\": 1") != std::string::npos && text.find("\"mesh_flagged\": false") != std::string::npos,
          "entry evidence");
    CHECK(text.find("\"moving_keys\": 0, \"released\": 1") != std::string::npos && text.find("\"moving_now\": false, \"released\": 1") != std::string::npos,
          "release evidence");
    std::ofstream("motion_dump3.json") << text;
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
    run(120);  // frame 500: stopped again and released again
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
    CHECK(text.find("\"schema\": 11") != std::string::npos, "schema 11");
    CHECK(text.find("\"rule_stale\": ") != std::string::npos && text.find("\"rule_stale_samples\": ") != std::string::npos,
          "rule_stale fields");
    CHECK(text.find("\"moving_keys\": 0, \"released\": 2") != std::string::npos
              && text.find("\"moving_now\": false, \"released\": 2") != std::string::npos,
          "dump: release counters");
    std::ofstream("motion_dump4.json") << text;
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
    CHECK(text.find("\"schema\": 11") != std::string::npos, "dump: schema 11");
    CHECK(text.find("\"follow_moving\": true") != std::string::npos && text.find("\"follow\": {\"hits\"") != std::string::npos,
          "dump: follow object and switch");
    CHECK(text.find("\"follows\": ") != std::string::npos, "dump: per key follows");
    CHECK(std::count(text.begin(), text.end(), '{') == std::count(text.begin(), text.end(), '}')
              && std::count(text.begin(), text.end(), '[') == std::count(text.begin(), text.end(), ']'), "dump: balanced");
  }

  check_every_frame = true;

  HStage("29. Stopped prop: kept while seen; retired once when hidden 70 frames; re-admitted once.");
  // 29. Stopped prop: kept while seen; retired once when hidden 70 frames; re-admitted once.
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
    CHECK(snapshot().admitted == 0u && snapshot().orphans_retired == 1u, "hidden 70 frames: retired once (admitted %zu, retired %llu)",
          snapshot().admitted, (unsigned long long)snapshot().orphans_retired);
    objects.push_back({vb, 1, [start](uint32_t, uint32_t, float* w, float* p) { Place(w, 2.f); Place(p, 2.f); }});
    run(40);
    CHECK(snapshot().admitted == 1u && snapshot().orphans_retired == 1u, "re-admitted once, not retired again (admitted %zu, retired %llu)",
          snapshot().admitted, (unsigned long long)snapshot().orphans_retired);
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
    run(70);
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
    CHECK(snapshot().orphans_retired == 0u, "scan back on: grace, nothing retired yet");
    run(60);
    CHECK(snapshot().orphans_retired == 1u, "scan back on: retired after the grace (%llu)", (unsigned long long)snapshot().orphans_retired);
  }
  check_every_frame = false;

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

  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures == 0 ? 0 : 1;
}
