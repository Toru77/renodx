// Native harness: mesh capture verification (round 12) and the legacy
// instance scale switch. A mesh enters the pool only after two captures in a
// row decode to the same mesh; buffer contents the pool cannot see change
// (GPU writes) are simulated by editing the mock buffers between frames.
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
  uint64_t vb;
  uint32_t instances = 1;
  float scale = 1.f;
  float x = 0.f;
  bool deferred = false;
  uint32_t vb_usage = 0u;
  uint32_t vb_flags = 0u;
};

static size_t CountLines(const char* needle, char level = 0) {
  size_t n = 0;
  for (const auto& [lvl, line] : renodx::utils::log::g_lines) {
    if ((level == 0 || lvl == level) && line.find(needle) != std::string::npos) ++n;
  }
  return n;
}
static std::string LastLine(char level) {
  for (auto it = renodx::utils::log::g_lines.rbegin(); it != renodx::utils::log::g_lines.rend(); ++it) {
    if (it->first == level) return it->second;
  }
  return {};
}

struct RequestState {
  bool found = false;
  uint32_t captures = 0u;
  bool has_previous = false;
};
static RequestState Request(uint64_t vb) {
  std::lock_guard lock(bvh::g_pool.mutex);
  for (const auto& [key, request] : bvh::g_pool.mesh_requests) {
    if (request.draw.vb.handle == vb) return {true, request.captures, request.has_previous};
  }
  return {};
}
static bool MeshFor(uint64_t vb, bvh::WorldMesh* out) {
  std::lock_guard lock(bvh::g_pool.mutex);
  for (const bvh::WorldMesh& mesh : bvh::g_pool.meshes) {
    if (mesh.source_vb == vb) {
      *out = mesh;
      return true;
    }
  }
  return false;
}

int main() {
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  CmdList deferred_cl; deferred_cl.dev = &dev;
  fixture::RegisterLayout();
  const uint64_t kRigid = 0xA1, kOpaque = 0xB1;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
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
  falcom_world::WorldCommandListData deferred_data;
  deferred_data.vs_srv[15] = t15;
  deferred_data.vs_cb[1] = b1;

  uint64_t cursor = 777;
  std::vector<Object> objects;
  auto run_frame = [&]() {
    for (size_t index = 0; index < objects.size(); ++index) {
      const Object& object = objects[index];
      if (cursor + object.instances >= ring_elements) cursor = 0;
      const int32_t base = static_cast<int32_t>(cursor);
      for (uint32_t i = 0; i < object.instances; ++i) {
        const float m[12] = {object.scale, 0, 0, object.x + static_cast<float>(i) * 3.f, 0, object.scale, 0, 0, 0, 0, object.scale, 0};
        uint8_t* element = dev.res[t15.handle].bytes.data() + (cursor + i) * 160;
        std::memcpy(element, m, 48);
        std::memcpy(element + 48, m, 48);
      }
      cursor += object.instances + 5;
      int32_t cb[4] = {base, 0, 0, 0};
      std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
      falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);

      falcom_world::DrawRecord draw;
      draw.method = 1; draw.has_index_buffer = true; draw.vb = {object.vb}; draw.vb_stride = fixture::kStride; draw.ib = ib;
      draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {fixture::kLayout};
      draw.vb_usage = object.vb_usage; draw.vb_flags = object.vb_flags;
      draw.index_size = 2; draw.dsv = {0x77}; draw.depth_enable = true; draw.depth_write = true;
      draw.topology = primitive_topology::triangle_list;
      draw.vs_pipeline = kRigid; draw.ps_pipeline = kOpaque; draw.vs_hash = 0x1000u + static_cast<uint32_t>(index);
      draw.frame = falcom_world::g_state.frame.load();
      draw.index_count = 36; draw.first_index = static_cast<uint32_t>(index) * 36u; draw.instance_count = object.instances;
      bvh::OnPoolScanDraw(&dev, object.deferred ? &deferred_cl : &cl, draw, object.deferred ? &deferred_data : &cl_data);
    }
    falcom_world::g_state.frame.fetch_add(1u);
    bvh::DrainPoolScan(&dev, &queue);
  };
  // Runs frames until `done` holds; returns the frames run, or -1.
  auto run_until = [&](auto done, int max_frames) {
    for (int i = 0; i < max_frames; ++i) {
      if (done()) return i;
      run_frame();
    }
    return done() ? max_frames : -1;
  };
  auto snapshot = [&]() {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::UpdatePoolStats(bvh::PoolNewestSeenByKey());
    return bvh::g_pool.stats;
  };
  auto fresh = [&]() {
    bvh::ResetWorldPool();
    objects.clear();
    renodx::utils::log::g_lines.clear();
  };

  bvh::g_pool.scan_active.store(true);

  HStage("1. Defaults.");
  // 1. Defaults.
  CHECK(bvh::g_pool.verify_meshes.load() && !bvh::g_pool.legacy_scale.load(), "defaults: verify on, legacy scale off");

  HStage("2. A stable mesh takes two identical captures: two index and two vertex");
  // 2. A stable mesh takes two identical captures: two index and two vertex
  // copies, then it is added and its instances admit.
  {
    fresh();
    const resource vb = make_vb(1.f);
    objects.push_back({vb.handle, 2});
    objects.back().vb_usage = static_cast<uint32_t>(resource_usage::vertex_buffer | resource_usage::unordered_access);
    const int frames = run_until([&] { return snapshot().meshes == 1u; }, 40);
    const auto s = snapshot();
    bvh::WorldMesh mesh;
    const bool found = MeshFor(vb.handle, &mesh);
    std::printf("stable: %d frames, copies %llu+%llu, verified %u, mismatches %u, captures %u\n", frames,
                (unsigned long long)s.mesh_index_copies, (unsigned long long)s.mesh_vertex_copies, s.mesh_verified,
                s.mesh_capture_mismatches, found ? mesh.captures : 0u);
    CHECK(frames > 0, "mesh added");
    CHECK(s.mesh_index_copies == 2u && s.mesh_vertex_copies == 2u, "two captures (%llu+%llu)",
          (unsigned long long)s.mesh_index_copies, (unsigned long long)s.mesh_vertex_copies);
    CHECK(s.mesh_verified == 1u && s.mesh_capture_mismatches == 0u && s.mesh_unstable == 0u, "verified once");
    CHECK(found && mesh.captures == 2u && mesh.verified, "mesh provenance (captures %u, verified %d)", mesh.captures, (int)mesh.verified);
    CHECK(found && bvh::PoolBufferText(mesh.source_vb_usage, mesh.source_vb_flags) == "default+uav", "vb text '%s'",
          bvh::PoolBufferText(mesh.source_vb_usage, mesh.source_vb_flags).c_str());
    CHECK(!Request(vb.handle).found, "request done");
    CHECK(CountLines("", 'w') == 0u, "no warning");
    run_until([&] { return snapshot().admitted == 2u; }, 80);
    CHECK(snapshot().admitted == 2u, "instances admitted (%zu)", snapshot().admitted);
  }

  HStage("3. Contents change once between the first and second capture: the second");
  // 3. Contents change once between the first and second capture: the second
  // differs (one mismatch, logged), the third repeats it and is applied.
  {
    fresh();
    const resource vb = make_vb(2.f);
    objects.push_back({vb.handle, 1});
    CHECK(run_until([&] { return Request(vb.handle).has_previous; }, 40) > 0, "first capture complete");
    CHECK(Request(vb.handle).captures == 1u, "one capture (%u)", Request(vb.handle).captures);
    CHECK(snapshot().meshes == 0u, "not added after one capture");
    fixture::WriteVertices(dev.res[vb.handle].bytes, fixture::Box(2.5f));  // no write event: like a GPU write
    const int frames = run_until([&] { return snapshot().meshes == 1u; }, 60);
    const auto s = snapshot();
    bvh::WorldMesh mesh;
    const bool found = MeshFor(vb.handle, &mesh);
    std::printf("changed once: %d frames, mismatches %u, verified %u, captures %u, bbox max x %.2f\n", frames,
                s.mesh_capture_mismatches, s.mesh_verified, found ? mesh.captures : 0u, found ? mesh.bbox_max[0] : 0.f);
    CHECK(frames > 0, "added");
    CHECK(s.mesh_capture_mismatches == 1u && s.mesh_verified == 1u && s.mesh_unstable == 0u, "one mismatch");
    CHECK(found && mesh.captures == 3u && mesh.verified && mesh.bbox_max[0] == 2.5f, "third capture applied");
    CHECK(CountLines("differs from the previous one", 'w') == 1u, "mismatch logged");
    const std::string line = LastLine('w');
    std::printf("  %s\n", line.c_str());
    CHECK(line.find("indices same") != std::string::npos && line.find("vertex range 0..7 -> 0..7") != std::string::npos,
          "index facts");
    CHECK(line.find("differing triangles 12") != std::string::npos && line.find("first at 0") != std::string::npos,
          "triangle facts");
    CHECK(line.find("by direct draw") != std::string::npos && line.find("vb ") != std::string::npos
              && line.find(" default stride 12") != std::string::npos, "copy and buffer facts");
    CHECK(line.find("triangle 0 was (-2, -1, -1)") != std::string::npos && line.find("now (-2.5, -1, -1)") != std::string::npos,
          "first differing triangle");
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.families[0x1000u].mesh_mismatches == 1u, "family mismatch count");
  }

  HStage("4. Index contents change between captures: \"indices differ\".");
  // 4. Index contents change between captures: "indices differ".
  {
    fresh();
    const resource vb = make_vb(3.f);
    objects.push_back({vb.handle, 1});
    CHECK(run_until([&] { return Request(vb.handle).has_previous; }, 40) > 0, "first capture complete");
    uint16_t* indices = reinterpret_cast<uint16_t*>(dev.res[ib.handle].bytes.data());
    std::swap(indices[0], indices[1]);
    CHECK(run_until([&] { return snapshot().meshes == 1u; }, 60) > 0, "added");
    bvh::WorldMesh mesh;
    CHECK(MeshFor(vb.handle, &mesh) && mesh.captures == 3u, "third capture applied");
    CHECK(snapshot().mesh_capture_mismatches == 1u, "one mismatch");
    CHECK(LastLine('w').find("indices differ") != std::string::npos, "indices differ: %s", LastLine('w').c_str());
    std::swap(indices[0], indices[1]);
  }

  HStage("5. Contents change every frame: no two captures agree, the mesh is");
  // 5. Contents change every frame: no two captures agree. Each round of
  // kPoolMeshMaxCaptures captures is retried kPoolMeshRetryRounds times, 180
  // frames apart; the last round rejects the mesh and its instances never admit.
  {
    fresh();
    const resource vb = make_vb(4.f);
    objects.push_back({vb.handle, 2});
    float sx = 4.f;
    for (int i = 0; i < 1500 && snapshot().mesh_unstable == 0u; ++i) {
      sx += 0.01f;
      fixture::WriteVertices(dev.res[vb.handle].bytes, fixture::Box(sx));
      run_frame();
    }
    for (int i = 0; i < 80; ++i) run_frame();  // instances keep being seen
    const auto s = snapshot();
    std::printf("unstable: mismatches %u unstable %u retries %u failures %u meshes %zu admitted %zu error '%s'\n",
                s.mesh_capture_mismatches, s.mesh_unstable, s.mesh_retries, s.mesh_failures, s.meshes, s.admitted,
                s.last_mesh_error.c_str());
    constexpr uint32_t kRounds = bvh::kPoolMeshRetryRounds + 1u;
    CHECK(s.mesh_unstable == 1u && s.mesh_retries == bvh::kPoolMeshRetryRounds
              && s.mesh_capture_mismatches == (bvh::kPoolMeshMaxCaptures - 1u) * kRounds,
          "rejected on round %u: %u retries, %u mismatches", kRounds, s.mesh_retries, s.mesh_capture_mismatches);
    CHECK(s.meshes == 0u && s.admitted == 0u && s.mesh_failures == 1u, "nothing added");
    CHECK(s.last_mesh_error.find("unstable") != std::string::npos, "reason");
    CHECK(s.mesh_index_copies == bvh::kPoolMeshMaxCaptures * kRounds && s.mesh_vertex_copies == bvh::kPoolMeshMaxCaptures * kRounds,
          "no copy after the rejection (%llu+%llu)", (unsigned long long)s.mesh_index_copies,
          (unsigned long long)s.mesh_vertex_copies);
    CHECK(!Request(vb.handle).found, "request gone");
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.families[0x1000u].mesh_unstable == 1u, "family unstable count");
  }

  HStage("5b. Contents change during the first round only: the next round is stable and admits.");
  // 5b. Contents change during the first round only: the next round is stable and admits.
  {
    fresh();
    const resource vb = make_vb(4.f);
    objects.push_back({vb.handle, 2});
    float sx = 4.f;
    for (int i = 0; i < 1500 && snapshot().mesh_retries == 0u; ++i) {
      sx += 0.01f;
      fixture::WriteVertices(dev.res[vb.handle].bytes, fixture::Box(sx));
      run_frame();
    }
    CHECK(run_until([&] { return snapshot().meshes == 1u; }, 600) > 0, "stable on a later round: admitted");
    const auto s = snapshot();
    CHECK(s.mesh_retries == 1u && s.mesh_unstable == 0u && s.mesh_failures == 0u && s.admitted == 2u,
          "one retry, not rejected (retries %u, unstable %u, admitted %zu)", s.mesh_retries, s.mesh_unstable, s.admitted);
  }

  HStage("6. The mismatch log is capped per pool reset; counting goes on.");
  // 6. The mismatch log is capped per pool reset; counting goes on.
  {
    fresh();
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.mismatch_lines_logged = bvh::kPoolMismatchLogLines;
    }
    const resource vb = make_vb(5.f);
    objects.push_back({vb.handle, 1});
    float sx = 5.f;
    for (int i = 0; i < 120 && snapshot().mesh_unstable == 0u; ++i) {
      sx += 0.01f;
      fixture::WriteVertices(dev.res[vb.handle].bytes, fixture::Box(sx));
      run_frame();
    }
    CHECK(snapshot().mesh_capture_mismatches == bvh::kPoolMeshMaxCaptures - 1u && CountLines("", 'w') == 0u, "capped");
    bvh::ResetWorldPool();
    CHECK(bvh::g_pool.mismatch_lines_logged == 0u, "reset clears the cap");
  }

  HStage("7. Verification off: one capture is applied as before.");
  // 7. Verification off: one capture is applied as before.
  {
    fresh();
    bvh::g_pool.verify_meshes.store(false);
    const resource vb = make_vb(6.f);
    objects.push_back({vb.handle, 1});
    CHECK(run_until([&] { return snapshot().meshes == 1u; }, 40) > 0, "added");
    const auto s = snapshot();
    bvh::WorldMesh mesh;
    CHECK(MeshFor(vb.handle, &mesh) && mesh.captures == 1u && !mesh.verified, "one capture, not verified");
    CHECK(s.mesh_index_copies == 1u && s.mesh_vertex_copies == 1u && s.mesh_verified == 0u, "single copies");
    bvh::g_pool.verify_meshes.store(true);
  }

  HStage("8. Buffer released during the second capture: the request goes, nothing");
  // 8. Buffer released during the second capture: the request goes, nothing
  // is added, copies in flight are dropped.
  {
    fresh();
    const resource vb = make_vb(7.f);
    objects.push_back({vb.handle, 1});
    CHECK(run_until([&] { return Request(vb.handle).has_previous; }, 40) > 0, "first capture complete");
    run_frame();  // second index copy in flight
    objects.clear();
    bvh::OnDestroyResourcePool(&dev, vb);
    dev.destroy_resource(vb);
    for (int i = 0; i < 6; ++i) run_frame();
    const auto s = snapshot();
    CHECK(!Request(vb.handle).found && s.meshes == 0u && s.mesh_queue == 0u && s.mesh_in_flight == 0u, "released");
    CHECK(dev.bad_copies == 0, "copies in range (%d)", dev.bad_copies);
  }

  HStage("9. Deferred-context draws do not serve the second capture either.");
  // 9. Deferred-context draws do not serve the second capture either.
  {
    fresh();
    const resource vb = make_vb(8.f);
    objects.push_back({vb.handle, 1});
    CHECK(run_until([&] { return Request(vb.handle).has_previous; }, 40) > 0, "first capture complete");
    run_until([&] { return snapshot().mesh_in_flight == 0u; }, 10);
    const auto before = snapshot();
    objects.back().deferred = true;
    for (int i = 0; i < 8; ++i) run_frame();
    const auto mid = snapshot();
    CHECK(mid.mesh_index_copies == before.mesh_index_copies && mid.mesh_vertex_copies == before.mesh_vertex_copies
              && mid.meshes == 0u && Request(vb.handle).captures == 1u, "no copy at deferred draws");
    CHECK(mid.mesh_deferred_skips > before.mesh_deferred_skips, "skips counted");
    objects.back().deferred = false;
    CHECK(run_until([&] { return snapshot().meshes == 1u; }, 40) > 0, "verified at immediate draws");
  }

  HStage("10. Legacy scale limits: 0.02 and 60 are admitted by default and rejected");
  // 10. Legacy scale limits: 0.02 and 60 are admitted by default and rejected
  // with the switch on; the dump counts admitted instances outside them.
  {
    fresh();
    const resource small = make_vb(1.f);
    const resource large = make_vb(1.5f);
    objects.push_back({small.handle, 1, 0.02f, 0.f});
    objects.push_back({large.handle, 1, 60.f, 500.f});
    run_until([&] { return snapshot().admitted == 2u; }, 120);
    CHECK(snapshot().admitted == 2u, "admitted by default (%zu)", snapshot().admitted);
    const std::filesystem::path json_path = bvh::PoolOutputDir() / "world_pool.json";
    std::filesystem::remove(json_path);
    bvh::DumpWorldPool();
    std::ifstream json(json_path);
    const std::string text((std::istreambuf_iterator<char>(json)), std::istreambuf_iterator<char>());
    CHECK(text.find("\"schema\": 13") != std::string::npos, "schema 13");
    CHECK(text.find("\"verify_meshes\": true, \"legacy_scale\": false, \"exclude_moving\": false, \"follow_moving\": true}") != std::string::npos, "switches");
    CHECK(text.find("\"admitted_outside_legacy_scale\": 2") != std::string::npos, "outside legacy count");
    CHECK(text.find("\"outside_legacy_scale\": 1") != std::string::npos, "family count");
    CHECK(text.find("\"mesh_verified\": 2, \"mesh_capture_mismatches\": 0, \"mesh_unstable\": 0") != std::string::npos, "stats");
    CHECK(text.find("\"captures\": 2, \"verified\": true, \"vb\": \"default\", \"ib\": \"default\"") != std::string::npos,
          "mesh entries");
    CHECK(text.find("\"mesh_mismatches\": 0, \"mesh_unstable\": 0") != std::string::npos, "family mesh counts");

    fresh();
    bvh::g_pool.legacy_scale.store(true);
    objects.push_back({small.handle, 1, 0.02f, 0.f});
    objects.push_back({large.handle, 1, 60.f, 500.f});
    for (int i = 0; i < 120; ++i) run_frame();
    const auto s = snapshot();
    std::printf("legacy: admitted %zu small %llu large %llu meshes %zu\n", s.admitted,
                (unsigned long long)s.matrix_rejects[(size_t)bvh::PoolMatrixReject::SmallScale],
                (unsigned long long)s.matrix_rejects[(size_t)bvh::PoolMatrixReject::LargeScale], s.meshes);
    CHECK(s.admitted == 0u, "rejected with the switch on");
    CHECK(s.matrix_rejects[(size_t)bvh::PoolMatrixReject::SmallScale] > 0u
              && s.matrix_rejects[(size_t)bvh::PoolMatrixReject::LargeScale] > 0u, "reasons");
    CHECK(bvh::PoolOutsideLegacyScale(std::array<float, 12>{0.05f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 49.9f, 0}.data()) == false
              && bvh::PoolOutsideLegacyScale(std::array<float, 12>{1, 0, 0, 0, 0, 50.f, 0, 0, 0, 0, 1, 0}.data()),
          "legacy bounds [0.05, 50)");
    bvh::g_pool.legacy_scale.store(false);
  }

  HStage("11. Switch log line carries the new switches.");
  // 11. Switch log line carries the new switches.
  {
    renodx::utils::log::g_lines.clear();
    bvh::g_pool.log_captures.store(true);
    bvh::LogPoolSwitches();
    CHECK(CountLines("verify mesh captures on, legacy scale limits off", 'i') == 1u, "switch line");
    bvh::g_pool.log_captures.store(false);
  }

  std::printf("%s (%d failures)\n", g_failures == 0 ? "PASS" : "FAILED", g_failures);
  return g_failures == 0 ? 0 : 1;
}
