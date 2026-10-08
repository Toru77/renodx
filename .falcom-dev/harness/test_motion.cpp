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
  CHECK(bvh::g_pool.exclude_moving.load(), "keep moving objects out: on by default");
  // Sections 1-6 test the probe alone: P2a off (admission as before).
  bvh::g_pool.exclude_moving.store(false);

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
    CHECK(text.find("\"schema\": 8") != std::string::npos, "schema 8");
    CHECK(text.find("\"admitted_of_moved_meshes\": 2") != std::string::npos, "ghost count in dump");
    CHECK(text.find("\"vs_hash\": \"0x00001000\", \"admitted\": 2}") != std::string::npos
              || text.find("\"admitted\": 2}") != std::string::npos, "moved mesh listed with 2 admitted");
    std::ofstream("motion_dump.json") << text;
  }

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
    CHECK(s.dynamic_blocked > 0u, "poses refused while moving (blocked %llu)", (unsigned long long)s.dynamic_blocked);
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

  // 10. Switch off: the stopped object's pose admits again at its next
  // sighting; on again: removed again.
  {
    bvh::g_pool.exclude_moving.store(false);
    run(80);
    auto s = snapshot();
    CHECK(s.admitted == 2u && s.dynamic_meshes == 0u, "off: re-admitted (%zu, flagged %zu)", s.admitted, s.dynamic_meshes);
    bvh::g_pool.exclude_moving.store(true);
    run(1);
    s = snapshot();
    CHECK(s.admitted == 2u && s.dynamic_meshes == 0u, "on again: released key not re-flagged (%zu)", s.admitted);
  }

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
    CHECK(s.dynamic_live_keys == 1u && s.dynamic_meshes == 1u && s.admitted == 0u,
          "one key, one mesh flagged, nothing admitted (keys %zu, flagged %zu, admitted %zu)", s.dynamic_live_keys,
          s.dynamic_meshes, s.admitted);
  }

  // 17. Two keys share a mesh: one stops and is released; the other still
  // moves, so the shared mesh stays flagged.
  {
    fresh();
    const resource shared = make_vb(8.f);
    objects.push_back({shared.handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      Place(w, 100.f + 0.01f * f); Place(p, 100.f + 0.01f * (f == 0u ? 0u : f - 1u));
    }});
    objects.push_back({shared.handle, 1, [](uint32_t f, uint32_t, float* w, float* p) {
      const float x = f < 60u ? 200.f + 0.1f * f : 206.f;
      const float q = f == 0u ? x : (f - 1u < 60u ? 200.f + 0.1f * (f - 1u) : 206.f);
      Place(w, x); Place(p, q);
    }, 0u, 72u});
    run(200);
    const auto s = snapshot();
    CHECK(s.dynamic_live_keys == 2u && s.dynamic_moving_keys == 1u && s.dynamic_meshes == 1u && s.dynamic_released == 0u,
          "shared mesh held by the moving key (keys %zu, moving %zu, flagged %zu, released %u)", s.dynamic_live_keys,
          s.dynamic_moving_keys, s.dynamic_meshes, s.dynamic_released);
  }

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
    CHECK(text.find("\"schema\": 9") != std::string::npos, "schema 9");
    CHECK(text.find("\"rule_stale\": ") != std::string::npos && text.find("\"rule_stale_samples\": ") != std::string::npos,
          "rule_stale fields");
    CHECK(text.find("\"moving_keys\": 0, \"released\": 2") != std::string::npos
              && text.find("\"moving_now\": false, \"released\": 2") != std::string::npos,
          "dump: release counters");
    std::ofstream("motion_dump4.json") << text;
  }

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
